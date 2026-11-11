<p align="center"><img src="docs/images/logo-512.png" alt="ForbiddenItlwm logo" width="200"></p>

<h1 align="center">ForbiddenItlwm</h1>

<p align="center"><em>Because AirDrop was never supposed to work on Intel Wi-Fi.</em></p>

**AirDrop for Intel Wi-Fi cards on macOS.** ForbiddenItlwm is a fork of AirportItlwm that adds AWDL, the peer-to-peer Wi-Fi link AirDrop runs on. Apple's own Wi-Fi stack does the rest. Normal Wi-Fi stays connected while AirDrop works.

For now it supports only macOS Tahoe (26.7.1). Support for more macOS versions is planned.

> **Status: first public beta (0.1.0-beta1). Experimental.** It has been tested on one machine with one card (Intel AX211) on one macOS build (26.7.1, 25G241): an HP Envy x360 13-bf with an iPhone 11. See [where it was tested](docs/TESTED-HARDWARE.md). Expect rough edges. Test it from a USB stick first, never from your only working EFI.

## What works

Measured on the [test machine](docs/TESTED-HARDWARE.md) (HP Envy x360 13-bf with an AX211, macOS 26.7.1, iPhone 11), sending 100 MB files to and from an iPhone, with the Mac connected to normal 5 GHz Wi-Fi:

| | Result |
| --- | --- |
| Mac → iPhone | Works. About 22 MB/s typical, 16 to 28 MB/s. Connects in 1 to 3.6 s |
| iPhone → Mac | Works. About 20 MB/s typical, 15 to 30 MB/s. A cold connect usually takes 3 to 10 s. Now and then one takes longer or fails, and a retry works |
| Normal Wi-Fi at the same time | Works. The Mac stays connected throughout |
| Large transfers | No stalls in 21 transfers in a row on the release build |

**The speed depends on the iPhone's own Wi-Fi.** AirDrop gets the time the iPhone's radio gives it.
- **Full speed** (about 22 MB/s): the iPhone isn't connected to Wi-Fi, or is connected on channel 149.
- **About 15 MB/s:** the iPhone is connected on another 5 GHz channel and its connection is quiet.
- **About 6 MB/s:** that connection is busy.

### Other Continuity features

Tested with the same Mac and iPhone:

| Feature | State |
| --- | --- |
| Handoff | Works in both directions |
| Universal Clipboard | Works |
| Instant Hotspot | Doesn't work yet |
| Continuity Camera | Doesn't work yet |
| iPhone Mirroring | Doesn't work. Probably can't without a T2 chip or Apple silicon |

## Requirements

- **macOS 26.7.1 (25G241) exactly.** The driver hooks into macOS's Wi-Fi stack at places checked on this build only. On other builds it may fail to load or panic. Don't use it on any other version.
- **An Intel Wi-Fi card that AirportItlwm supports.** Only the AX211 is tested. The AX210 and AX411, and CNVi radios on Alder Lake and newer, are close relatives and may work. See the [compatibility table](tools/compat/README.md).
- **An Intel Bluetooth controller** with [IntelBluetoothFirmware](https://github.com/OpenIntelWireless/IntelBluetoothFirmware) and BlueToolFixup ([BrcmPatchRAM](https://github.com/acidanthera/BrcmPatchRAM)), plus [Lilu](https://github.com/acidanthera/Lilu) and this project's **BTLEAdvFixup**. AirDrop finds devices over Bluetooth LE before it uses Wi-Fi.
- **OpenCore.**
- **A nearby Wi-Fi network that advertises a country, in a country that allows channel 149.** Without it, AirDrop falls back to channel 6, which is very slow. See [Channel 149 and your country](#channel-149-and-your-country).

## Channel 149 and your country

AirDrop is fast only on channel 149 (5 GHz, 80 MHz). The Wi-Fi card's firmware decides whether that channel may be used.
- **It has to learn your country first.** The firmware starts in a "world" mode where channel 149 is listen-only, and opens it after it hears the country from a nearby Wi-Fi network. Most routers advertise their country. An iPhone's Personal Hotspot switched on nearby (you don't need to join it) has also worked.
- **Your country's radio laws still apply.** In the Linux wireless regulatory database, 44 of 183 countries have no Wi-Fi rule for channels 149 to 161, for example Japan, Turkey, Egypt and Saudi Arabia. A few allow them only at 20 or 40 MHz, for example Kenya, the Central African Republic, Guam, El Salvador and North Korea. Intel's own tables follow the same laws and can be stricter.
- **Otherwise AirDrop uses channel 6.** It still works there, but very slowly.
- **The driver never overrides the firmware's decision,** and there's no country setting to change it. `itlwm_cc` only changes the code macOS displays.
- **The checker tells you:** `tools/compat/check_compat.sh` warns about these countries, and shows whether the firmware actually opened channel 149.

## Will it work on my Mac?

Run the read-only checker first. It changes nothing and sends nothing anywhere:

```bash
bash tools/compat/check_compat.sh
```

See [tools/compat/README.md](tools/compat/README.md). If your card isn't supported yet, the collectors there write a hardware report you can share.

## Install

See **[docs/INSTALL.md](docs/INSTALL.md)**. In short:
1. Build the two kexts, or take them from a release.
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

- **One macOS build.** 26.7.1 (25G241) only, and no check refuses other builds yet. Don't install it on anything else.
- **One tested card.** Everything except the AX211 is untested.
- **Cold iPhone → Mac connects are slow,** usually 3 to 10 s and sometimes longer. The time goes into the iPhone finding the Mac before any data moves. This is the main open problem.
- **No sleep and wake testing** beyond normal use on the test machine.
- **Channel 6 fallback is very slow.** It happens when the firmware hasn't learned a country that allows channel 149.
- **iPhone only.** iPads and other Macs are expected to work the same way, but haven't been tried.
- **Boot flags.** The beta still runs on a long list of them. Simpler defaults are planned.

## Disclaimer

**This is beta software.** It is experimental, has been tested on a single machine, and may have serious bugs.
- **What can go wrong:** it runs in the macOS kernel, so a bug can cause kernel panics, lost Wi-Fi or Bluetooth, failed transfers or a Mac that won't boot from that EFI.
- **Protect yourself:** keep backups, test from a USB stick, and keep a working EFI you can boot without it.
- **Your risk:** you use it at your own risk. As the GPL says, it comes with no warranty of any kind, and the authors aren't liable for any damage or data loss.

**AI was used to make this project.** Much of the work was done with an AI assistant (Claude, by Anthropic):
- the driver code
- reverse-engineering and analysis
- the test tools
- this documentation

The project's maintainer directed the work, decided what to test, and ran every hardware test on real devices. All the results reported here come from those tests. Even so, AI-written code can contain mistakes that testing on one machine won't catch. Treat it accordingly, and please report anything that looks wrong.

## Credits

ForbiddenItlwm builds on a lot of other people's work:

- **[itlwm / AirportItlwm](https://github.com/OpenIntelWireless/itlwm)** by zxystd and the OpenIntelWireless contributors: the Intel Wi-Fi driver this is built on.
- **laobamac**, for AirportItlwm on macOS Tahoe (version 2.4.1), the exact code this fork starts from.
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
