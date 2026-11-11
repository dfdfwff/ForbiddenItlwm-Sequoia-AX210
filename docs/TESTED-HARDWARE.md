# Where it was tested

Every result in these docs comes from this one setup. Anything else is untested.

## The Mac

| | |
| --- | --- |
| Laptop | HP Envy x360 13-bf (2022, convertible) |
| CPU | Intel Core i7-1250U (12th gen, Alder Lake-U, 2 P-cores + 8 E-cores, 12 threads) |
| Memory | 16 GB |
| Graphics | Intel Iris Xe (internal 2880 × 1800 OLED) |
| Wi-Fi | Intel AX211 (Wi-Fi 6E, CNVi, PCI device 8086:51F0), firmware `iwlwifi-so-a0-gf-a0-68` |
| Bluetooth | Intel AX211 Bluetooth on USB, 8087:0033 (Bluetooth 5.3), firmware loaded by IntelBluetoothFirmware |
| macOS | macOS Tahoe 26.7.1 (25G241). 26.7.2 (25G332) and 26.7.3 (25G342) verified offline only. Sequoia 15.8.x not tested |
| SMBIOS | MacBookPro16,2 |
| Bootloader | OpenCore 1.0.8 (release build) |

Kexts used with ForbiddenItlwm:

| Kext | Version |
| --- | --- |
| ForbiddenItlwm | 0.1.0-beta1 and 0.2.0-beta1 (Tahoe) |
| BTLEAdvFixup | 0.8.0 |
| Lilu | 1.7.2 |
| IntelBluetoothFirmware | 2.5.1 |
| BlueToolFixup | 2.7.3 |
| IntelBTPatcher | Disabled |

The laptop also loads kexts for its own hardware (graphics, audio, touchscreen, input, battery). They aren't part of ForbiddenItlwm. The laptop's complete OpenCore EFI, with ForbiddenItlwm in it, and its setup guide are published at [retropineaple/whateverxe-hp-envy-x360](https://github.com/retropineaple/whateverxe-hp-envy-x360).

## The iPhone

| | |
| --- | --- |
| Model | iPhone 11 |
| iOS | iOS 27.2 (24B119) |
| Wi-Fi and Bluetooth | Built in (Wi-Fi 6, Bluetooth 5.0) |
| AirDrop setting | Contacts Only on both devices for most tests, Everyone for 10 Minutes for some |

## Conditions

- **The Mac's Wi-Fi:** connected to a normal 5 GHz network on channel 36 at 80 MHz for the coexistence results. AirDrop ran on channel 149 at 80 MHz beside it.
- **The country:** the firmware learned it from nearby networks.
- **The iPhone's Wi-Fi:** tested in three states, which change the speed (see the README):
  - not associated
  - associated on channel 149
  - associated on channel 36
- **Transfers:** mostly 100 MB files, plus iPhone sysdiagnose files of up to about 500 MB, in both directions.
- **When:** November 2026.

## Not tested

- Other Intel cards, and other laptops with an AX211
- Other macOS builds, including 26.7.2 and 26.7.3
- **macOS Sequoia: the Sequoia build hasn't been booted on any hardware.** It was verified offline from the 15.8 and 15.8.3 Kernel Debug Kits
- iPads, other iPhones, other Macs
- Sleep and wake during a transfer, and long-term stability
