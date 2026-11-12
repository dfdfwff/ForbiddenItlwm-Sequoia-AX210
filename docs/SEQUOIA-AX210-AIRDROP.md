# Sequoia AX210 AirDrop / Continuity Camera

This branch contains the Sequoia build validated on the configuration below.
It is based on ForbiddenItlwm 0.5.0-test1 and retains the upstream GPL-2.0
licence and attribution.

## Verified result

- Mac to iPhone and iPhone to Mac AirDrop both work.
- Continuity Camera works through `ContinuityCaptureAgent`.
- A 108 MB iPhone to Mac transfer completed in a few seconds after the
  approximately five-second AirDrop/Wi-Fi handshake.
- Normal Wi-Fi remains connected while AWDL is active.
- iPhone Mirroring remains unavailable because the machine has no T2 chip.

Mac to iPhone currently works but is slower than iPhone to Mac. The Sequoia
build still spends most outbound data on the single-frame queue because the
iPhone does not consistently keep the transmit Block Ack session in the
`agreed` state. The separate `experiment/addba-amsdu` branch contains an
untested ADDBA capability experiment for that issue.

## Tested configuration

```text
macOS:        Sequoia 15.8.1 (24H32)
CPU:          AMD Ryzen 7 5800H
Wi-Fi:        Intel AX210 8086:2725
Bluetooth:    Intel Bluetooth with IntelBluetoothFirmware
iPhone:       iPhone 15 Pro
OpenCore:     injected kexts
```

Verified stable driver:

```text
ForbiddenItlwm 0.5.0
Build marker: forbidden-0.5.0-test1-sequoia
SHA-256:      497ec376fa1dc3cb658e7e7110d9de958d5f7ad44814e40fede072fc3b3f4e76
```

## Root fix

Sequoia's Skywalk nexus requires packet buffer pool type `1` for the AWDL
network provider. Upstream 0.5.0 uses type `0`; that creates the AWDL object
but leaves the native BSD interface unattached, Apple PeerManager never
supplies the synchronization template, and transfers remain on "Waiting".

This branch defaults to:

```cpp
IOSkywalkPacketBufferPool::withName("itlwm-awdl-tx", this, 1, &txOptions);
IOSkywalkPacketBufferPool::withName("itlwm-awdl-rx", this, 1, &options);
```

`itlawdlpooltype=0` restores the upstream value for A/B testing.

## Required hotfix

```text
-btleadv -itlawdlcoexprobe -itlawdliface -itlawdlnativeup
-itlawdlcoexrx -itlawdlcoextx -itlawdlcoexmulti -itlawdlcoexdata
-itlawdlmitx -itlawdlelicit -itlawdlcaps -itlawdlsvcmerge
-itlawdlawsync itlawdlintcoal=4 -itlawdlcoexbatx
-itlawdlcoexpeerpause -itlawdlcoexbatrace itlawdltxqueue=4096
```

Install both `ForbiddenItlwm.kext` and `BTLEAdvFixup.kext`. Remove or disable
the old `AirportItlwm`, `AirportItlwmAWDL`, injected `IOSkywalkFamily`,
`IO80211FamilyLegacy`, and `IntelBTPatcher`.

## Fast channel

AWDL is significantly faster on channel 149 at 80 MHz. In the validated setup,
an OpenWrt router was configured with:

```text
country: CN
channel: 149
width:   80 MHz
```

Observed healthy state:

```text
ITLAWDLCoexChannel         = 149
ITLAWDLCoexOwnWidth        = 80
ITLAWDLCoexBeaconHints     = 15
ITLAWDLCoexRefusedReason   = 0
```

Channel 6 still works, but receive throughput is much lower.

## Safety

Test from a USB EFI first. Keep a known-good EFI that does not contain these
kexts. These are kernel extensions and can cause boot loops, kernel panics,
or loss of Wi-Fi and Bluetooth.

## Credits

- retropineaple and the ForbiddenItlwm contributors
- zxystd and the OpenIntelWireless itlwm / AirportItlwm contributors
- laobamac for the macOS Tahoe/Sequoia AirportItlwm 2.4.1 code
- kgp-macPro / AirportItlwm-Tahoe for the DMA design referenced upstream
- OpenBSD for net80211, iwx, iwm and iwn
- Linux iwlwifi and Intel for firmware and implementation references
- FranciumOS-org/AirPort_RTW89 for the native AWDL integration approach
- Acidanthera for Lilu, MacKernelSDK and BlueToolFixup
- OpenIntelWireless IntelBluetoothFirmware / IntelBTPatcher for the HCI hook
- seemoo-lab OWL and the Open Wireless Link research, especially Milan Stute,
  David Kreitschmann and Matthias Hollick's AWDL reverse-engineering work and
  *One Billion Apples' Secret Sauce: Recipe for the Apple Wireless Direct Link
  Ad hoc Protocol* (MobiCom 2018)

The Wi-Fi driver remains GPL-2.0. BTLEAdvFixup is GPL-3.0-only, and Intel
firmware keeps its own redistribution licence.
