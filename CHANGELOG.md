# Changelog

## Unreleased

- **AirDrop on setups that pad AWDL frames.** On some machines every AirDrop frame from the iPhone arrived with zero bytes after its last field, so the driver rejected all of them and transfers stayed on "Waiting". Seen on an AX211 (8086:51F1) in a ThinkPad E16 Gen 3 with an iPhone 16 Pro. The driver now cuts that padding, and only that: nothing but trailing zero bytes is removed, and only when the frame is otherwise invalid and parses without them. `ITLAWDLCoexZeroPadTrimmed` counts the frames fixed. Diagnosis and approach by keenerdavid18 (issue #6). Not tested on that hardware with this exact build yet.
- **BTLEAdvFixup 0.8.1 (Tahoe): macOS 26.7.3 support.** The Tahoe build now also carries `bluetoothd` patterns for 26.7.3 (25G342), taken from Apple's installer and verified offline. They're the same four patches as on 26.7.1. Not tested on hardware yet. Thanks to the issue #5 report, and to mingww64, whose PR #2 independently found three of the four sites.
- **`check_compat.sh`** finds discrete Wi-Fi cards behind a PCI root port (most desktops and M.2 slots). It used to report "No Intel Wi-Fi card found" for them. Thanks to alikaylan (issue #5) for the diagnosis. It also checks the 26.7.3 Bluetooth patterns.

## 0.3.0-beta1

- **macOS Sequoia 15.8.x support, as separate kexts.** Each macOS version now has its own ForbiddenItlwm and BTLEAdvFixup. Releases have Tahoe zips and Sequoia zips: use the ones for your macOS.
  - Built with `tools/build.py --target sequoia` and `tools/build_btleadv.py --target sequoia`.
  - Verified offline from Apple's Kernel Debug Kits for 15.8 (24H23) and 15.8.3 (24H241): every virtual table slot of the AirDrop interface (563 of 563) and every kernel import. 15.8.1 and 15.8.2 are expected to work: AirDrop starts there as "expected" if Apple's Wi-Fi tables still match at boot.
  - BTLEAdvFixup's Sequoia patterns are exact bytes from 15.8.3's `bluetoothd`. On 15.8 to 15.8.2 they may not match, and then iPhones may not list the Mac. `check_compat.sh` tells you.
  - It uses the Wi-Fi stack that ships with Sequoia, like laobamac's native Sequoia AirportItlwm. Revert OpenCore Legacy Patcher's "Modern Wireless" root patches first if you applied them.
  - **Not tested on any hardware yet.** Testers wanted.
- **Each kext refuses the wrong macOS.** The Tahoe ForbiddenItlwm now refuses to start outside macOS 26, and the Sequoia one outside 15.2 or newer Sequoia, instead of risking a kernel panic.
- **iMessage and push over Wi-Fi on Sequoia:** the 0.2.0 link-quality fix applies to the Sequoia build too.
- **Tahoe:** apart from the version check, the Tahoe kexts behave as in 0.2.0-beta1. The Tahoe build of BTLEAdvFixup is unchanged (0.8.0).
- **Tools:** the ABI audit takes `--target sequoia`, the BTLEAdvFixup pattern test takes `--target` and `--binary`, and `check_compat.sh` recognises Sequoia.

## 0.2.0-beta1

- **More Tahoe builds:** AirDrop now also starts on macOS 26.7.2 (25G332) and 26.7.3 (25G342). Both were verified offline from Apple's Kernel Debug Kits:
  - IOSkywalkFamily is identical to 26.7.1.
  - IO80211Family's AirDrop-related code is instruction for instruction the same as 26.7.1.
  - All virtual table slots and all kernel imports match.

  Neither has been tested on hardware yet. BTLEAdvFixup's `bluetoothd` patterns are still exact bytes from 26.7.1, so on these builds the Mac may not advertise AirDrop over Bluetooth, and iPhones may not list it. `check_compat.sh` tells you. Reports welcome.
- **Build check:** the driver now refuses to start AirDrop on a macOS build it doesn't know, and on a build where Apple's AWDL function table has changed. On those builds it works as plain Wi-Fi instead of risking a kernel panic. The result is shown in `ItlwmAWDLBuildCheck`. `-itlawdlanybuild` overrides the build list, but not the table check.
- **iMessage, push and other Apple services work over Wi-Fi.** The Tahoe driver never set the Wi-Fi interface's link quality, so macOS rated Wi-Fi "off" (LinkQuality -2), and Apple Push wouldn't connect over it. That broke iMessage and anything else that needs push. It also kept AirDrop between your own devices (same Apple Account) from working properly. The driver now reports link quality from the signal strength (good, poor or bad), and also gives the signal strength to macOS's Wi-Fi stack. This came from the original AirportItlwm for Tahoe.
- **Tester check:** `check_compat.sh` now recognises the supported builds and checks BTLEAdvFixup's patterns against the tester's own `bluetoothd`. BTLEAdvFixup itself is unchanged at 0.8.0.
- **Source layout:** the source folder `AirportItlwm/` is now `ForbiddenItlwm/`. The kext is byte-identical to before apart from the changes above.
- **Troubleshooting:** when the iPhone can't see the Mac but the Mac can see the iPhone, Reset Network Settings on the iPhone fixed it in testing. If iMessage on the Mac only sends as text messages, re-enable the Mac under the iPhone's Text Message Forwarding.

## 0.1.0-beta1

First public beta.

- AirDrop between an Intel Wi-Fi Mac and an iPhone, both ways, while normal Wi-Fi stays connected.
- Tested on an Intel AX211 with macOS 26.7.1 (25G241). About 22 MB/s each way when the iPhone's schedule allows it.
- Based on AirportItlwm 2.4.1 for macOS Tahoe.
- The kext is renamed from `AirportItlwm.kext` to `ForbiddenItlwm.kext`, with bundle identifier `org.forbiddenitlwm.ForbiddenItlwm`. It replaces AirportItlwm, and the two can't be loaded together.
- BTLEAdvFixup 0.8.0, bundle identifier `org.forbiddenitlwm.BTLEAdvFixup`.
- `-itlawdlcoexlowlat` is disabled because it broke Wi-Fi on the AX211.

Known issues: cold iPhone → Mac connects take 3 to 10 s and occasionally fail. One macOS build only. See the README.
