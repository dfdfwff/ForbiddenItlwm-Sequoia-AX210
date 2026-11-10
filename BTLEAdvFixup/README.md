# BTLEAdvFixup

A Lilu plugin that lets macOS start Bluetooth LE advertising, which AirDrop needs, on an Intel Bluetooth controller.

- **Turn it on** with the boot flag `-btleadv`, and off with `-btleadvoff`. Without the flag it does nothing.
- **What it changes:** it patches `/usr/sbin/bluetoothd` in memory as its pages are validated, so `bluetoothd` takes its "extended advertising supported" path. It also adjusts some outgoing HCI commands, such as the random address of each advertising set, and does IntelBTPatcher's fix-up before the first scan. Nothing on disk changes.
- **One macOS build:** the byte patterns match macOS 26.7.1 (25G241). On another build they don't match, and nothing is patched.
- **Disable IntelBTPatcher.** Both hook the same function, and Lilu panics if two plugins do. BTLEAdvFixup takes over IntelBTPatcher's job.

Build it with `tools/build_btleadv.py`, as described in [docs/INSTALL.md](../docs/INSTALL.md).

## License

GPL-3.0-only. The HCI hook follows IntelBTPatcher (OpenIntelWireless/IntelBluetoothFirmware, GPL-3.0), and the page hook follows BlueToolFixup (acidanthera/BrcmPatchRAM). The full text is in [LICENSE](LICENSE).
