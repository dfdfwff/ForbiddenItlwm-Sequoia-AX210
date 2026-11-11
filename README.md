<p align="center"><img src="docs/images/icon-512.png" alt="ForbiddenItlwm logo" width="180"></p>

<h1 align="center">ForbiddenItlwm</h1>

<p align="center"><em>Because AirDrop was never supposed to work on Intel Wi-Fi.</em></p>

**AirDrop for Intel Wi-Fi cards on macOS.** ForbiddenItlwm is a fork of AirportItlwm that adds AWDL, the peer-to-peer Wi-Fi link AirDrop runs on. Apple's own Wi-Fi stack does the rest. Normal Wi-Fi stays connected while AirDrop works.

It supports macOS Tahoe 26.7.x and, from 0.3.0, macOS Sequoia 15.8.x. **Each macOS version has its own kexts:** download the ones for your macOS.

> **Status: public beta (0.4.0-beta1). Experimental.** The project tests on one machine with one card (Intel AX211) on macOS 26.7.1 (25G241): an HP Envy x360 13-bf with an iPhone 11. Users report AirDrop working on 26.7.2 and 26.7.3 too. **The Sequoia build hasn't been confirmed working on any hardware yet**, and testers are needed. See [where it was tested](docs/TESTED-HARDWARE.md). Expect rough edges. Test it from a USB stick first, never from your only working EFI.

> **Update from any earlier version.** 0.4.0-beta1 fixes security bugs in every earlier release, including one in the Wi-Fi password handshake code that came from the original itlwm. See the [changelog](CHANGELOG.md).

## ⚠️ Read this first

> [!WARNING]
> **Use it at your own risk.** ForbiddenItlwm is experimental beta software that runs inside the macOS kernel. A bug can cause kernel panics, bootloops, lost Wi-Fi or Bluetooth, failed transfers, or a hackintosh that won't boot from that EFI. It comes with no warranty of any kind, and I am not liable for any damage or data loss.

> [!IMPORTANT]
> **This project was built with the use of AI.** Most of the driver code, the reverse-engineering and the test tools were written with the help of AI (Claude). I am not a professional kernel developer. I closely monitored and directed the work, researched and reviewed it, and ran every hardware test on my own hardware, but AI written code can still contain mistakes that testing on a few machines won't catch. If you'd rather not run AI-assisted kernel code, please don't install it.

**Before you install:**
- Test from a USB stick with a copy of your EFI, never your only working one.
- Keep backups, and a working EFI you can boot without it.
- Check the [requirements](#requirements) and run the [compatibility checker](#will-it-work-on-my-hackintosh) first.
- **Before you update macOS, boot once without `-btleadv`.** While BTLEAdvFixup is active, `bluetoothd` reads as modified, and an incremental macOS update that checks it can fail.

All the hardware results the project reports come from the maintainer's own tests. Reports from other users are marked as such, and the Sequoia build has only been checked offline so far. Please report anything that looks wrong.

## What works

Measured on the [test machine](docs/TESTED-HARDWARE.md) (HP Envy x360 13-bf with an AX211, macOS 26.7.1, iPhone 11), sending 100 MB files to and from an iPhone, with the hackintosh connected to normal 5 GHz Wi-Fi:

| | Result |
| --- | --- |
| Mac → iPhone | Works. About 22 MB/s typical, 16 to 28 MB/s. Connects in 1 to 3.6 s |
| iPhone → Mac | Works. About 20 MB/s typical, 15 to 30 MB/s. A cold connect usually takes 3 to 10 s. Now and then one takes longer or fails, and a retry works |
| Normal Wi-Fi at the same time | Works. The hackintosh stays connected throughout |
| Large transfers | No stalls in 21 transfers in a row on the release build |

**The speed depends on the iPhone's own Wi-Fi.** AirDrop gets the time the iPhone's radio gives it.
- **Full speed** (about 22 MB/s): the iPhone isn't connected to Wi-Fi, or is connected on channel 149.
- **About 15 MB/s:** the iPhone is connected on another 5 GHz channel and its connection is quiet.
- **About 6 MB/s:** that connection is busy.

### Other Continuity features

Tested with the same hackintosh and iPhone, on Tahoe 26.7.1:

| Feature | State |
| --- | --- |
| Handoff | Works in both directions |
| Universal Clipboard | Works |
| iMessage and other push-based services over Wi-Fi | Work (fixed in 0.2.0: the original Tahoe driver left Wi-Fi rated "off", so Apple Push never connected) |
| Instant Hotspot | Doesn't work yet |
| Continuity Camera | Doesn't work yet |
| iPhone Mirroring | Doesn't work. Probably can't without a T2 chip or Apple silicon |
| Sidecar | Not tried yet |

## Requirements

- **macOS Tahoe 26.7.x or macOS Sequoia 15.8.x, with the matching kexts.** The driver hooks into macOS's Wi-Fi stack, so each macOS version gets its own build of ForbiddenItlwm and BTLEAdvFixup, and each build has to be checked. A kext for the wrong macOS version refuses to start.

  | macOS | Build | Status |
  | --- | --- | --- |
  | Tahoe | 26.7.1 (25G241) | Tested on hardware |
  | Tahoe | 26.7.3 (25G342) | Reported working by a user (AX210). Wi-Fi side verified offline from Apple's Kernel Debug Kit, BTLEAdvFixup's patterns from Apple's installer |
  | Tahoe | 26.7.2 (25G332) | Reported working by a user (AX210). Wi-Fi side verified offline from Apple's Kernel Debug Kit. BTLEAdvFixup's patterns are the same bytes as 26.7.3's |
  | Sequoia | 15.8 (24H23), 15.8.3 (24H241) | Wi-Fi side verified offline from Apple's Kernel Debug Kits. BTLEAdvFixup's patterns match 15.8.3. Not confirmed working on hardware yet |
  | Sequoia | 15.8.2 (24H232) | Same kernel as 15.8.3, so the build check passes. One user report (AX201): the Mac sees the iPhone, but sending doesn't work yet ([issue #7](https://github.com/retropineaple/ForbiddenItlwm/issues/7)). BTLEAdvFixup's patterns may not match |
  | Sequoia | 15.8.1 (24H32) | Expected to work (same kernel family), not checked offline. BTLEAdvFixup's patterns may not match |
  | Any other build | | AirDrop stays off and normal Wi-Fi works |

  The driver checks the build itself when it starts. On a build it doesn't know, it keeps AirDrop off instead of risking a kernel panic. BTLEAdvFixup's patterns are exact bytes from specific `bluetoothd` builds (26.7.1, and 26.7.2 and 26.7.3, which share theirs, on Tahoe; 15.8.3 on Sequoia). On other builds, `tools/compat/check_compat.sh` shows whether they still match.
- **An Intel Wi-Fi card that AirportItlwm supports.** Only the AX211 is tested by the project. Users have reported the Tahoe kext working on an AX201 and on other AX211 machines on 26.7.1, and on an AX210 on 26.7.2 and 26.7.3 (see [user reports](docs/TESTED-HARDWARE.md#user-reports)). The AX411, and CNVi radios on Alder Lake and newer, are close relatives and may work. See the [compatibility table](tools/compat/README.md).
- **An Intel Bluetooth controller** with [IntelBluetoothFirmware](https://github.com/OpenIntelWireless/IntelBluetoothFirmware) and BlueToolFixup ([BrcmPatchRAM](https://github.com/acidanthera/BrcmPatchRAM)), plus [Lilu](https://github.com/acidanthera/Lilu) and this project's **BTLEAdvFixup**. AirDrop finds devices over Bluetooth LE before it uses Wi-Fi.
- **OpenCore.**
- **macOS's own Wi-Fi stack.** Like laobamac's native AirportItlwm builds it's based on, ForbiddenItlwm uses the IO80211Family that ships with macOS. If you applied OpenCore Legacy Patcher's "Modern Wireless" root patches for an older AirportItlwm, revert them first: they replace the Wi-Fi stack the driver was checked against.
- **A nearby Wi-Fi network that advertises a country, in a country that allows channel 149.** Without it, AirDrop falls back to channel 6, which is very slow. See [Channel 149 and your country](#channel-149-and-your-country).

## Channel 149 and your country

AirDrop is fast only on channel 149 (5 GHz, 80 MHz). The Wi-Fi card's firmware decides whether that channel may be used.
- **It has to learn your country first.** The firmware starts in a "world" mode where channel 149 is listen-only, and opens it after it hears the country from a nearby Wi-Fi network. Most routers advertise their country. An iPhone's Personal Hotspot switched on nearby (you don't need to join it) has also worked.
- **Your country's radio laws still apply.** In the Linux wireless regulatory database, 44 of 183 countries have no Wi-Fi rule for channels 149 to 161, for example Japan, Turkey, Egypt and Saudi Arabia. A few allow them only at 20 or 40 MHz, for example Kenya, the Central African Republic, Guam, El Salvador and North Korea. Intel's own tables follow the same laws and can be stricter.
- **Otherwise AirDrop uses channel 6.** It still works there, but very slowly.
- **The driver never overrides the firmware's decision,** and there's no country setting to change it. `itlwm_cc` only changes the code macOS displays.
- **The checker tells you:** `tools/compat/check_compat.sh` warns about these countries, and shows whether the firmware actually opened channel 149.

## Will it work on my hackintosh?

Run the read-only checker first. It changes nothing and sends nothing anywhere:

```bash
bash tools/compat/check_compat.sh
```

See [tools/compat/README.md](tools/compat/README.md). If your card isn't supported yet, the collectors there write a hardware report you can share.

## Install

See **[docs/INSTALL.md](docs/INSTALL.md)**. In short:
1. Build the two kexts for your macOS version, or take them from a release: the Tahoe zips for 26.7.x, the Sequoia zips for 15.8.x.
2. On a USB stick with a copy of your EFI, replace `AirportItlwm.kext` with `ForbiddenItlwm.kext` and add `BTLEAdvFixup.kext`.
3. Add the boot flags from [docs/BOOT-FLAGS.md](docs/BOOT-FLAGS.md).
4. Boot from the stick and check.

**ForbiddenItlwm replaces AirportItlwm.** Don't load both: they share class names, and only one can run.

## Documentation

| Document | What's in it |
| --- | --- |
| [docs/TESTED-HARDWARE.md](docs/TESTED-HARDWARE.md) | The laptop, iPhone, versions and conditions every result comes from |
| [docs/INSTALL.md](docs/INSTALL.md) | Building, OpenCore setup, first boot, checks and uninstalling |
| [docs/BOOT-FLAGS.md](docs/BOOT-FLAGS.md) | The flags the beta needs, and the ones to leave alone |
| [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) | When the iPhone can't see the Mac, it's slow, or Wi-Fi breaks |
| [docs/HOW-IT-WORKS.md](docs/HOW-IT-WORKS.md) | How the driver gives macOS an AWDL radio, and what was learned on the way |
| [tools/compat/README.md](tools/compat/README.md) | The compatibility checker and hardware collectors |
| [CHANGELOG.md](CHANGELOG.md) | Releases |

## Known limitations

- **Few macOS builds.** Tahoe 26.7.1 to 26.7.3 and Sequoia 15.8 to 15.8.3. On any other build the driver keeps AirDrop off. Users report 26.7.2 and 26.7.3 working. **Sequoia isn't confirmed working yet:** on the one report so far, the Mac sees the iPhone but can't send ([issue #7](https://github.com/retropineaple/ForbiddenItlwm/issues/7)).
- **One card tested by the project.** Users have reported the AX201 and other AX211 machines working on Tahoe 26.7.1, and the AX210 on 26.7.2 and 26.7.3. Everything else is untested.
- **Connects can be slow.** A cold iPhone → Mac connect usually takes 3 to 10 s, and sometimes longer: the time goes into the iPhone finding the Mac before any data moves. Mac → iPhone usually takes a few seconds, and now and then a first try fails while the Mac looks up the iPhone. This is the main open problem.
- **Sleep isn't tested.** Sleep is disabled on the test machine.
- **macOS updates with BTLEAdvFixup active can fail.** Boot once without `-btleadv` before updating macOS.
- **Channel 6 fallback is very slow.** It happens when the firmware hasn't learned a country that allows channel 149.
- **iPhone only.** iPads and other Macs are expected to work the same way, but haven't been tried.
- **Boot flags.** The beta still runs on a long list of them. Simpler defaults are planned.

## Planned

These are plans, not promises. There are no dates, and the order isn't fixed.

- **Instant Hotspot:** joining the iPhone's hotspot from the Wi-Fi menu without typing the password.
- **Continuity Camera:** using the iPhone as the Mac's webcam.
- **Faster connects:** cutting the time a transfer spends before any data moves, in both directions.
- **macOS Sonoma 14 and Ventura 13:** separate kexts for each, like Tahoe and Sequoia. Each needs its own study of that version's Wi-Fi stack first, so support isn't certain.
- **Fewer boot flags:** sensible defaults, so the beta doesn't need its long list of flags.
- **A unified Bluetooth kext that survives macOS updates:** a Bluetooth kext made for ForbiddenItlwm, replacing BTLEAdvFixup, IntelBluetoothFirmware and BlueToolFixup, so all wireless connectivity needs just two kexts besides Lilu. Its patches would keep working across small macOS updates.
- **AirPlay to the hackintosh, Sidecar and Universal Control:** these also use Apple's peer-to-peer links. None has been tried yet.

## Credits

ForbiddenItlwm builds on a lot of other people's work:

- **[itlwm / AirportItlwm](https://github.com/OpenIntelWireless/itlwm)** by zxystd and the OpenIntelWireless contributors: the Intel Wi-Fi driver this is built on.
- **laobamac**, for AirportItlwm on macOS Tahoe and Sequoia (version 2.4.1), the exact code this fork starts from, including the native Sequoia Wi-Fi support the Sequoia build is based on.
- **kgp-macPro/AirportItlwm-Tahoe**, named in the base code's notes as the basis of its DMA design.
- **[OpenBSD](https://www.openbsd.org)** for net80211 and the iwx, iwm and iwn drivers.
- **[Linux iwlwifi](https://wireless.wiki.kernel.org/en/users/drivers/iwlwifi)** and **Intel**, for the firmware and as a reference. No code was copied.
- **[FranciumOS-org/AirPort_RTW89](https://github.com/FranciumOS-org/AirPort_RTW89)** (GPL-2.0): it showed the approach and was used to cross-check the wire format.
- **[seemoo-lab/owl](https://github.com/seemoo-lab/owl)** and the Open Wireless Link research, used to cross-check AWDL frame formats and timing. No code was copied.
- **[Acidanthera](https://github.com/acidanthera)**, for Lilu, MacKernelSDK and BlueToolFixup. BTLEAdvFixup's page hook follows BlueToolFixup.
- **[IntelBluetoothFirmware / IntelBTPatcher](https://github.com/OpenIntelWireless/IntelBluetoothFirmware)** (GPL-3.0): BTLEAdvFixup's HCI hook follows IntelBTPatcher.
- Apple's XNU sources were read as a reference.

ForbiddenItlwm isn't affiliated with or endorsed by Apple or Intel. AirDrop is a trademark of Apple Inc.

## License

- **The Wi-Fi driver:** GPL-2.0, see [LICENSE](LICENSE). Files from OpenBSD keep their own ISC and BSD notices.
- **BTLEAdvFixup:** GPL-3.0-only, see [BTLEAdvFixup/LICENSE](BTLEAdvFixup/LICENSE).
- **Intel firmware** in `itlwm/firmware`: Intel's redistribution licence, see [iwlwifi-firmware-license](iwlwifi-firmware-license).
