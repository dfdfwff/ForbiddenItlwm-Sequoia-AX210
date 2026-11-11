# Installing ForbiddenItlwm

This beta has been tested on one machine: an HP Envy x360 13-bf with an Intel AX211, on macOS 26.7.1 (25G241). 26.7.2 (25G332) and 26.7.3 (25G342) were verified offline. It was booted with OpenCore 1.0.8, with an iPhone 11. The details are in [TESTED-HARDWARE.md](TESTED-HARDWARE.md). Read the whole page before you start.

## Before you start

- **Check your macOS build:** `sw_vers -buildVersion` should print `25G241`, `25G332` or `25G342`. On any other build the driver keeps AirDrop off and works as plain Wi-Fi.
- **Run the checker:** `bash tools/compat/check_compat.sh`. It tells you whether your card and Bluetooth are likely to work.
- **Use a USB stick.** Copy your working EFI folder to a USB stick and make every change there. Your internal EFI stays as it is, so if anything goes wrong you just boot without the stick.
- **Know the rollback:** boot from your internal EFI, or put the original `AirportItlwm.kext` and `config.plist` back on the stick.

## What you need

| Kext | Where from | Notes |
| --- | --- | --- |
| `ForbiddenItlwm.kext` | This repository | Replaces `AirportItlwm.kext` |
| `BTLEAdvFixup.kext` | This repository | Lets macOS start Bluetooth LE advertising on an Intel controller |
| `Lilu.kext` | [acidanthera/Lilu](https://github.com/acidanthera/Lilu) | Tested with 1.7.2 |
| `IntelBluetoothFirmware.kext` | [OpenIntelWireless/IntelBluetoothFirmware](https://github.com/OpenIntelWireless/IntelBluetoothFirmware) | Tested with 2.5.1 |
| `BlueToolFixup.kext` | [acidanthera/BrcmPatchRAM](https://github.com/acidanthera/BrcmPatchRAM) | Tested with 2.7.3 |

**Disable `IntelBTPatcher.kext`.** It and BTLEAdvFixup hook the same function, and Lilu panics if two plugins do. BTLEAdvFixup takes over IntelBTPatcher's job.

## Building

You need the Xcode Command Line Tools (`xcode-select --install`). Full Xcode isn't needed. The build was tested with Python 3 on an Intel Mac.

1. Put MacKernelSDK in the repository folder. Commit `3f75008` is the one tested:

   ```bash
   git clone https://github.com/acidanthera/MacKernelSDK
   ```

2. Build the Wi-Fi driver:

   ```bash
   python3 tools/build.py --out build
   ```

   The result is `build/ForbiddenItlwm.kext`. The first build also generates `include/FwBinary.cpp` from the firmware in `itlwm/firmware`.

3. Build BTLEAdvFixup. It needs the Lilu SDK, which comes with Lilu's **DEBUG** release. Make a folder holding `MacKernelSDK` and the DEBUG `Lilu.kext`, then:

   ```bash
   python3 tools/build_btleadv.py --out build/btleadv --tree /path/to/that/folder
   ```

   The result is `build/btleadv/BTLEAdvFixup.kext`.

4. Optional, for developers: run the offline tests and the ABI audit. The audit checks every virtual slot of the AWDL interface against macOS 26.7.1's own table, and must report 568 of 568:

   ```bash
   python3 tools/test_awdl.py
   ```

   ```bash
   python3 tools/abi/audit_vtable.py --lifecycle-overrides
   ```

## OpenCore setup

All of this goes on the **USB stick's** EFI.

1. **Kexts:** in `EFI/OC/Kexts`, remove `AirportItlwm.kext` and copy in `ForbiddenItlwm.kext` and `BTLEAdvFixup.kext`.

2. **`Kernel` → `Add`:** keep this order, so each kext comes after what it depends on.

   | Order | BundlePath | ExecutablePath | Notes |
   | --- | --- | --- | --- |
   | 1 | `Lilu.kext` | `Contents/MacOS/Lilu` | |
   | 2 | `IntelBluetoothFirmware.kext` | `Contents/MacOS/IntelBluetoothFirmware` | |
   | 3 | `BlueToolFixup.kext` | `Contents/MacOS/BlueToolFixup` | `MinKernel` 21.0.0 |
   | 4 | `BTLEAdvFixup.kext` | `Contents/MacOS/BTLEAdvFixup` | `MinKernel` 21.0.0 |
   | 5 | `ForbiddenItlwm.kext` | `Contents/MacOS/ForbiddenItlwm` | Replaces the old `AirportItlwm.kext` entry |

   Set `IntelBTPatcher.kext` to `Enabled` = false if you have it. This is required. Make sure no `AirportItlwm.kext` or `itlwm.kext` entry is still enabled.

3. **Boot arguments:** add the flags from [BOOT-FLAGS.md](BOOT-FLAGS.md) to `boot-args` under `NVRAM` → `Add` → `7C436110-AB2A-4BBB-A880-FE41995C9F82`. Keep your existing arguments. Because OpenCore only writes NVRAM keys listed under `Delete`, check that `boot-args` is listed there for the same GUID.

4. **Validate:** `plutil -lint config.plist`, and `ocvalidate` if you have it.

## First boot

1. Boot from the USB stick.
2. Join your normal Wi-Fi network as usual.
3. Wait a minute, then check that the driver and its AirDrop listener are running:

   ```bash
   ioreg -l -w0 | grep -E '"ItlwmAWDLInterfaceBuild"|"ITLAWDLCoex(Added|Channel|CountryLearned)"'
   ```

   You should see `forbidden-0.2.0-beta1`, `Added` Yes, `Channel` 149 and `CountryLearned` Yes. `Channel` 6 means the firmware hasn't opened channel 149: see [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

4. Or run `bash tools/compat/check_compat.sh` again. It now also reads the listener's state.
5. Turn on AirDrop on both devices. Contacts Only works when the iPhone and the Mac share an Apple Account or know each other as contacts. Otherwise use Everyone for 10 Minutes.
6. Send a file each way.

## Uninstalling

On the stick (or your EFI, if you moved it there):
1. Remove `ForbiddenItlwm.kext` and `BTLEAdvFixup.kext`, and their `Kernel` → `Add` entries.
2. Put `AirportItlwm.kext` back.
3. Remove the `-itlawdl*`, `itlawdl*` and `-btleadv` boot flags.
4. Reboot.

The driver writes nothing to disk. BTLEAdvFixup patches `bluetoothd` in memory only.
