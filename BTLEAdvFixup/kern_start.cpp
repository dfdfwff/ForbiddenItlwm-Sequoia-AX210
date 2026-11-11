// SPDX-License-Identifier: GPL-3.0-only
// The HCI hook follows IntelBTPatcher (OpenIntelWireless/IntelBluetoothFirmware, GPLv3),
// the page hook follows BlueToolFixup (acidanthera/BrcmPatchRAM).
//
// Separate Lilu plugin: lets bluetoothd use extended LE advertising on a third-party
// controller. It only patches a process in memory, and does nothing without its boot
// flag. See docs/HOW-IT-WORKS.md. Nothing on disk changes.
#include <Headers/kern_api.hpp>
#include <Headers/kern_user.hpp>
#include <Headers/kern_util.hpp>
#include <Headers/plugin_start.hpp>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/usb/IOUSBHostDevice.h>
#include "BTLEAdvPatches.hpp"
#include "BTLEAdvHci.hpp"

// Patches bluetoothd's pages as they're validated, the same way BlueToolFixup does.
// Lilu's onProcLoad route was tried first and didn't apply to this process.
static mach_vm_address_t orig_cs_validate {};
static bool patchScanning {};

static void patched_cs_validate_page(vnode_t vp, memory_object_t pager, memory_object_offset_t page_offset,
                                     const void *data, int *validated_p, int *tainted_p, int *nx_p) {
    char path[PATH_MAX];
    int pathlen = PATH_MAX;
    FunctionCast(patched_cs_validate_page, orig_cs_validate)(vp, pager, page_offset, data, validated_p, tainted_p, nx_p);
    if (vn_getpath(vp, path, &pathlen) != 0 || LIKELY(strcmp(path, "/usr/sbin/bluetoothd") != 0))
        return;
    // Each set only matches its own bluetoothd build; on any other build nothing changes.
    for (const auto &patch : BTLEAdv::kPatches)
        if (!patch.scanningOnly || patchScanning)
            KernelPatcher::findAndReplace(const_cast<void *>(data), PAGE_SIZE, patch.find, patch.size,
                                          patch.replace, patch.size);
}

// bluetoothd talks to the controller over USB control requests. Commands it doesn't send in
// the required order are added here, after the command they depend on. Their completion
// events reach bluetoothd as unmatched, as they already do for IntelBTPatcher's command.
static mach_vm_address_t orig_device_request {};
static BTLEAdv::State hciState {};

static IOReturn patched_device_request(void *that, IOService *provider, StandardUSB::DeviceRequest &request, void *data,
                                       IOMemoryDescriptor *descriptor, unsigned int &length,
                                       IOUSBHostCompletion *completion, uint32_t timeout);

// Evidence for tests, in the I/O Registry root. Set only on rare events, in thread context.
static uint32_t extraCommands {}, setAddresses {};
static void noteHook(const char *key, uint32_t value) {
    IORegistryEntry::getRegistryRoot()->setProperty(key, value, 32);
}

static bool isIntelBluetooth(void *that) {
    auto *device = static_cast<IORegistryEntry *>(that);
    auto *vendor = OSDynamicCast(OSNumber, device->getProperty("idVendor"));
    auto *name = OSDynamicCast(OSString, device->getProperty("USB Product Name"));
    return vendor && vendor->unsigned32BitValue() == 0x8087 && (!name || name->isEqualTo("Bluetooth USB Host Controller"));
}

static void sendExtra(void *that, IOService *provider, const StandardUSB::DeviceRequest &request,
                      const uint8_t *bytes, size_t size, uint32_t timeout) {
    auto *extra = IOBufferMemoryDescriptor::withBytes(bytes, size, kIODirectionOut);
    if (!extra)
        return;
    StandardUSB::DeviceRequest extraRequest = request;
    extraRequest.wLength = uint16_t(size);
    unsigned int extraLength = uint32_t(size);
    if (extra->prepare(kIODirectionOut) == kIOReturnSuccess) {
        FunctionCast(patched_device_request, orig_device_request)(that, provider, extraRequest, nullptr, extra, extraLength, nullptr, timeout);
        extra->complete(kIODirectionOut);
    }
    extra->release();
}

static IOReturn patched_device_request(void *that, IOService *provider, StandardUSB::DeviceRequest &request, void *data,
                                       IOMemoryDescriptor *descriptor, unsigned int &length,
                                       IOUSBHostCompletion *completion, uint32_t timeout) {
    uint8_t hci[32] = {};
    size_t hciLength = 0;
    // Every USB device comes through here, so only look at the Intel Bluetooth controller.
    // Class requests to the device carry HCI commands; bRequest isn't reliable for this.
    if ((request.bmRequestType & 0x80) == 0 && (request.bmRequestType & 0x60) == 0x20 && isIntelBluetooth(that)) {
        if (data) {
            hciLength = min(size_t(request.wLength), sizeof(hci));
            lilu_os_memcpy(hci, data, hciLength);
        } else if (descriptor && descriptor->prepare(kIODirectionOut) == kIOReturnSuccess) {
            hciLength = min(size_t(descriptor->getLength()), sizeof(hci));
            descriptor->readBytes(0, hci, hciLength);
            descriptor->complete(kIODirectionOut);
        }
    }
    if (BTLEAdv::wantsLegacyScanAddress(hciState, hci, hciLength)) {
        // IntelBTPatcher's workaround, sent before the scan command like the original.
        uint8_t fixed[9] = {0x05, 0x20, 0x06};
        lilu_os_memcpy(fixed + 3, BTLEAdv::kFixedPatcherAddress, 6);
        sendExtra(that, provider, request, fixed, sizeof(fixed), timeout);
    }
    static bool sawCommand {};
    if (hciLength >= 3 && !sawCommand) {
        sawCommand = true;
        noteHook("BTLEAdvFirstRequest", request.bRequest);
    }
    const auto action = BTLEAdv::onCommand(hciState, hci, hciLength, patchScanning);
    const IOReturn result = FunctionCast(patched_device_request, orig_device_request)(that, provider, request, data, descriptor, length, completion, timeout);
    if (action.sendSetAddress || action.sendEventMask) {
        sendExtra(that, provider, request, action.output, action.outputLength, timeout);
        noteHook("BTLEAdvExtraCommands", ++extraCommands);
        if (action.sendSetAddress)
            noteHook("BTLEAdvSetAddresses", ++setAddresses);
    }
    return result;
}

static const char *usbHostPaths[] {"/System/Library/Extensions/IOUSBHostFamily.kext/Contents/MacOS/IOUSBHostFamily"};
static KernelPatcher::KextInfo usbHostInfo {"com.apple.iokit.IOUSBHostFamily", usbHostPaths, 1, {true}, {}, KernelPatcher::KextInfo::Unloaded};

static const char *bootargOff[] = {"-btleadvoff"};
static const char *bootargDebug[] = {"-btleadvdbg"};
static const char *bootargBeta[] = {"-btleadvbeta"};

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
    bootargOff, arrsize(bootargOff),
    bootargDebug, arrsize(bootargDebug),
    bootargBeta, arrsize(bootargBeta),
    KernelVersion::Tahoe,
    KernelVersion::Tahoe,
    []() {
        // Off unless asked for, so this kext can sit in the EFI doing nothing.
        if (!checkKernelArgument("-btleadv"))
            return;
        // Advertising alone, to compare with both.
        patchScanning = !checkKernelArgument("-btlenoscan");
        lilu.onPatcherLoadForce([](void *, KernelPatcher &patcher) {
            KernelPatcher::RouteRequest route("_cs_validate_page", patched_cs_validate_page, orig_cs_validate);
            if (!patcher.routeMultipleLong(KernelPatcher::KernelID, &route, 1))
                SYSLOG("btleadv", "failed to route cs validation");
        });
        lilu.onKextLoadForce(&usbHostInfo, 1, [](void *, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
            if (index != usbHostInfo.loadIndex)
                return;
            KernelPatcher::RouteRequest route(
                "__ZN15IOUSBHostDevice13deviceRequestEP9IOServiceRN11StandardUSB13DeviceRequestEPvP18IOMemoryDescriptorRjP19IOUSBHostCompletionj",
                patched_device_request, orig_device_request);
            const bool routed = patcher.routeMultiple(index, &route, 1, address, size);
            if (!routed) {
                SYSLOG("btleadv", "failed to route device request");
                patcher.clearError();
            }
            noteHook("BTLEAdvHookRouted", routed ? 1 : 0);
        });
    }
};

// Match Xcode's kext lifecycle. The kernel runs C++ initializers; libkmod
// forwards the start/stop callbacks through _realmain and _antimain.
extern "C" {
extern kern_return_t _start(kmod_info_t *, void *);
extern kern_return_t _stop(kmod_info_t *, void *);
extern kern_return_t BTLEAdvFixup_kern_start(kmod_info_t *, void *);
extern kern_return_t BTLEAdvFixup_kern_stop(kmod_info_t *, void *);
__attribute__((visibility("default"))) KMOD_EXPLICIT_DECL(org.forbiddenitlwm.BTLEAdvFixup, "0.8.1", _start, _stop)
__attribute__((visibility("hidden"))) kmod_start_func_t *_realmain = BTLEAdvFixup_kern_start;
__attribute__((visibility("hidden"))) kmod_stop_func_t *_antimain = BTLEAdvFixup_kern_stop;
__attribute__((visibility("hidden"))) int _kext_apple_cc = __APPLE_CC__;
}
