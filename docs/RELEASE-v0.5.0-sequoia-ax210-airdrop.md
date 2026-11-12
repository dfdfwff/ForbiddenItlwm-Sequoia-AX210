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
