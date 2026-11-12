# Sequoia AX210 AirDrop verified build

This release contains the first locally validated Sequoia build with native
AWDL working end to end.

## Verified

- macOS Sequoia 15.8.1 (24H32)
- Intel AX210 `8086:2725`
- iPhone 15 Pro
- Two-way AirDrop
- Continuity Camera
- Normal Wi-Fi remains connected while AWDL is active

A 108 MB iPhone to Mac transfer completed in a few seconds after the usual
approximately five-second AirDrop handshake. Mac to iPhone is working but
currently slower because Sequoia TX Block Ack aggregation is still being
investigated.

iPhone Mirroring is not available because this machine does not have a T2
security chip.

## Known issues

- **Mac → iPhone TX aggregation is not stable.** The iPhone ACKs the
  driver's ADDBA request but does not reliably keep the reply/session in the
  `agreed` state. Bulk traffic therefore often falls back to the single-frame
  queue. A 108 MB test can complete successfully, but Mac → iPhone remains
  noticeably slower than iPhone → Mac.
- **The first AirDrop connection usually needs about five seconds.** Device
  discovery, AWDL synchronization, Bonjour service resolution and QUIC setup
  happen before file data moves.
- **Fast iPhone → Mac throughput requires channel 149 at 80 MHz in the tested
  environment.** Channel 6 at 20 MHz works but is much slower.
- **iPhone Mirroring is unavailable** because the tested machine has no T2
  security chip.

## Packages

```text
ForbiddenItlwm.kext
SHA-256 497ec376fa1dc3cb658e7e7110d9de958d5f7ad44814e40fede072fc3b3f4e76

BTLEAdvFixup.kext
SHA-256 0f4f89b02aa70dd79128112ddcf9c9489bdd0b95780eb8406483ac9748c85162
```

## Important

Install from a USB EFI first. Keep a known-good EFI without these kexts.
Kernel extensions can cause boot loops, kernel panics or loss of Wi-Fi and
Bluetooth.

See `README.md` inside the archive for boot arguments and the Sequoia pool-type
fix. The stable source is branch `sequoia-ax210-airdrop`. The unverified
ADDBA A-MSDU experiment is isolated on `experiment/addba-amsdu`.

## Credits

This build is based on the work of:

- retropineaple and the ForbiddenItlwm contributors
- zxystd and the OpenIntelWireless itlwm / AirportItlwm contributors
- laobamac for AirportItlwm 2.4.1 on Tahoe and Sequoia
- kgp-macPro / AirportItlwm-Tahoe for the upstream DMA design reference
- OpenBSD for net80211 and the iwx, iwm and iwn drivers
- Linux iwlwifi and Intel for firmware and implementation references
- FranciumOS-org/AirPort_RTW89 for the native AWDL integration approach
- Acidanthera for Lilu, MacKernelSDK and BlueToolFixup
- OpenIntelWireless for IntelBluetoothFirmware / IntelBTPatcher

AWDL framing, channel sequencing, synchronization windows and scheduling come
from the reverse-engineering work published by Open Wireless Link, especially
Milan Stute, David Kreitschmann and Matthias Hollick:

- *One Billion Apples' Secret Sauce: Recipe for the Apple Wireless Direct Link
  Ad hoc Protocol*, MobiCom 2018.
- The Open Wireless Link project and `seemoo-lab/owl`.

See the repository README for the complete attribution list. The Wi-Fi driver
remains GPL-2.0, BTLEAdvFixup is GPL-3.0-only, and Intel firmware keeps its
own redistribution licence.
