#include "ItlAWDLBSDInterface.hpp"

#include <sys/_if_ether.h>

struct in6_aliasreq;
extern "C" int in6_ifattach_aliasreq(struct __ifnet *, struct __ifnet *,
                                     struct in6_aliasreq *);

#define super AirportItlwmEthernetInterface
OSDefineMetaClassAndStructors(ItlAWDLBSDInterface, AirportItlwmEthernetInterface)

static bool
awdlIoctlIs(unsigned long cmd, int number)
{
    return ((cmd >> 8) & 0xff) == 'i' && (cmd & 0xff) == number;
}

bool ItlAWDLBSDInterface::initWithAWDL(IONetworkController *controller,
                                       ItlAWDLInterface *owner,
                                       const ether_addr &address)
{
    if (!owner || !super::initWithSkywalkInterfaceAndProvider(controller, owner)) {
        setProperty("ItlwmAWDLBSDInterfaceStage", "init-failed");
        return false;
    }
    fAWDLOwner = owner;
    fAddress = address;
    setProperty(kIOMACAddress, &fAddress, sizeof(fAddress));
    setProperty("ItlwmAWDLBSDInterface", true);
    setProperty("ItlwmAWDLBSDInterfaceStage", "initialized");
    return true;
}

const char *ItlAWDLBSDInterface::getNamePrefix() const
{
    return "awdl";
}

bool ItlAWDLBSDInterface::initIfnetParams(struct ifnet_init_params *params)
{
    if (!super::initIfnetParams(params)) {
        setProperty("ItlwmAWDLBSDInterfaceStage", "ifnet-params-failed");
        return false;
    }
    setProperty(kIOMACAddress, &fAddress, sizeof(fAddress));
    params->uniqueid = &fAddress;
    params->uniqueid_len = sizeof(fAddress);
    setProperty("ItlwmAWDLBSDInterfaceStage", "ifnet-params-ready");
    return true;
}

IOReturn ItlAWDLBSDInterface::attachToDataLinkLayer(IOOptionBits options,
                                                    void *parameter)
{
    IOReturn result = super::attachToDataLinkLayer(options, parameter);
    setProperty("ItlwmAWDLBSDAttachResult", result, 32);
    if (result == kIOReturnSuccess && getIfnet()) {
        ifnet_set_lladdr(getIfnet(), &fAddress, sizeof(fAddress));
        ifnet_set_flags(getIfnet(), IFF_UP | IFF_RUNNING,
                        IFF_UP | IFF_RUNNING);
        ensureIPv6Address(0);
        setLinkState(kIO80211NetworkLinkUp);
        fAWDLOwner->setRunningState(true);
        setProperty("ItlwmAWDLBSDInterfaceAttached", true);
        setProperty("ItlwmAWDLBSDInterfaceStage", "data-link-running");
    }
    return result;
}

void ItlAWDLBSDInterface::ensureIPv6Address(unsigned attempt)
{
    if (!getIfnet())
        return;
    fIPv6Attempts = attempt + 1;
    const SInt32 result = in6_ifattach_aliasreq(getIfnet(), nullptr, nullptr);
    if (result != fLastIPv6Result) {
        fLastIPv6Result = result;
        setProperty("ItlwmAWDLIPv6AttachResult", result, 32);
    }
    setProperty("ItlwmAWDLIPv6AttachAttempts", fIPv6Attempts, 32);
}

IOReturn ItlAWDLBSDInterface::getInterfaceAddress(IOEthernetAddress *address)
{
    if (!address)
        return kIOReturnBadArgument;
    memcpy(address, &fAddress, sizeof(fAddress));
    return kIOReturnSuccess;
}

SInt32 ItlAWDLBSDInterface::performCommand(IONetworkController *controller,
                                           unsigned long cmd, void *, void *)
{
    if (awdlIoctlIs(cmd, 16)) {
        if (getIfnet()) {
            uint32_t up = (ifnet_flags(getIfnet()) & IFF_UP) != 0;
            ifnet_set_flags(getIfnet(), up ? IFF_RUNNING : 0, IFF_RUNNING);
        }
        return kIOReturnSuccess;
    }
    if (awdlIoctlIs(cmd, 49) || awdlIoctlIs(cmd, 50))
        return kIOReturnSuccess;
    return controller ? kIOReturnSuccess : kIOReturnNotReady;
}

UInt32 ItlAWDLBSDInterface::outputAWDL(mbuf_t packet, void *)
{
    // Establish the BSD ifnet first; attach the native AWDL data bridge next.
    if (packet && mbuf_type(packet) != MBUF_TYPE_FREE)
        mbuf_freem(packet);
    return kIOReturnSuccess;
}
