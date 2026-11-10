# Changelog

## 0.1.0-beta1

First public beta.

- AirDrop between an Intel Wi-Fi Mac and an iPhone, both ways, while normal Wi-Fi stays connected.
- Tested on an Intel AX211 with macOS 26.7.1 (25G241). About 22 MB/s each way when the iPhone's schedule allows it.
- Based on AirportItlwm 2.4.1 for macOS Tahoe.
- The kext is renamed from `AirportItlwm.kext` to `ForbiddenItlwm.kext`, with bundle identifier `org.forbiddenitlwm.ForbiddenItlwm`. It replaces AirportItlwm, and the two can't be loaded together.
- BTLEAdvFixup 0.8.0, bundle identifier `org.forbiddenitlwm.BTLEAdvFixup`.
- `-itlawdlcoexlowlat` is disabled because it broke Wi-Fi on the AX211.

Known issues: cold iPhone → Mac connects take 3 to 10 s and occasionally fail. One macOS build only. See the README.
