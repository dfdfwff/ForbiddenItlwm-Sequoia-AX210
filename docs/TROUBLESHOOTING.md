# Troubleshooting

Start with `bash tools/compat/check_compat.sh`. Then read the driver's own state:

```bash
ioreg -l -w0 | grep -E '"ItlwmAWDL(InterfaceBuild|BuildCheck)"|"ITLAWDLCoex(Added|Channel|OwnWidth|CountryLearned|CountryNotices|PeerSyncFrames|PeerPresentEntries|TxOK|TxSubmitErrors)"'
```

| Key | Healthy value |
| --- | --- |
| `ItlwmAWDLInterfaceBuild` | `forbidden-0.4.0-beta1`, or `forbidden-0.4.0-beta1-sequoia` |
| `ItlwmAWDLBuildCheck` | `verified`, or on Sequoia 15.8.1 `expected: macOS 15.8.x kernel not checked offline`. Anything starting with `blocked` means AirDrop is off on this macOS build: see below |
| `ITLAWDLCoexAdded` | Yes: the AirDrop listener exists |
| `ITLAWDLCoexChannel`, `ITLAWDLCoexOwnWidth` | 149 and 80 |
| `ITLAWDLCoexCountryLearned` | Yes |
| `ITLAWDLCoexCountryNotices` | A small number. It climbs fast if the firmware keeps restarting |
| `ITLAWDLCoexPeerSyncFrames` | Rises while an Apple device with AirDrop open is nearby |
| `ITLAWDLCoexPeerPresentEntries` | How many of the iPhone's 16 time slots it spends on the listener's channel |
| `ITLAWDLCoexTxSubmitErrors` | 0 |

The driver's log doesn't reach the unified log, so its state is only in the I/O Registry, and it's lost at reboot.

## `ItlwmAWDLBuildCheck` says blocked

The driver only starts AirDrop on macOS builds whose Wi-Fi stack was checked: Tahoe 26.7.1 to 26.7.3, and Sequoia 15.8 to 15.8.3 (15.8.2 shares 15.8.3's kernel; 15.8.1 starts as "expected").
- **Nothing at all, and no ForbiddenItlwm keys:** check that you used the kexts for your macOS version. A ForbiddenItlwm built for the other version refuses to start, and logs `this kext is for macOS …`.
- **`blocked: unverified macOS build`:** your build isn't on the list yet. Normal Wi-Fi still works. Please open an issue with your build (`sw_vers -buildVersion`) so it can be checked. `-itlawdlanybuild` forces AirDrop on anyway, at the risk of a kernel panic.
- **`blocked: IO80211AWDLProtocol table differs`:** the Wi-Fi stack isn't the one the driver was checked against. If you applied OpenCore Legacy Patcher's "Modern Wireless" root patches, revert them. Otherwise Apple changed the part of its Wi-Fi stack the driver plugs into. Starting AirDrop would likely panic, so no flag overrides this. Please report your build.

## The listener is on channel 6, or `CountryLearned` is No

The firmware opens channel 149 only after a nearby network tells it the country.
- Make sure some 5 GHz network that advertises a country is in range. Switching on an iPhone's Personal Hotspot nearby, without joining it, has worked.
- Wait a minute after it appears.
- Some countries don't allow channels 149 to 161, or allow them only at 20 or 40 MHz. There AirDrop stays on channel 6. The checker warns about known ones.
- The driver never overrides this decision, and `itlwm_cc` doesn't help.

## The iPhone doesn't list the Mac

- **Bluetooth:** it must be on, with BTLEAdvFixup for your macOS version loaded, `-btleadv` set and IntelBTPatcher disabled. AirDrop finds devices over Bluetooth LE first. BTLEAdvFixup's patterns only match specific builds (26.7.1 and, from 0.8.1, 26.7.3 on Tahoe; 15.8.3 on Sequoia): `check_compat.sh` shows whether they match yours.
- **AirDrop settings:** with Contacts Only, the two devices must share an Apple Account or know each other as contacts. Try Everyone for 10 Minutes to rule this out.
- **Is the iPhone heard?** Read `ITLAWDLCoexPeerSyncFrames` twice, a few seconds apart, with the iPhone's share sheet open. If it rises, the radios hear each other.
- **Unlock the iPhone and keep the share sheet open** for 10 to 15 seconds.
- **The iPhone itself can get stuck.** During testing there was a period when the iPhone listed no Mac on any build, including builds that had worked earlier that day, while the Mac could still see and send to the iPhone. Several restarts didn't fix it. **Reset Network Settings on the iPhone did.** A factory reset also helped, but only for a while. If the Mac's counters look healthy, look at the iPhone before the driver. Try these in order:
  1. Restart the iPhone.
  2. Settings → General → Transfer or Reset iPhone → Reset → Reset Network Settings. This forgets saved Wi-Fi networks.
  3. As a last resort, back up the iPhone and factory reset it. Afterwards the iPhone is a new device to AirDrop: Contacts Only won't match until it's signed in to its Apple Account again and has synced, so use Everyone for 10 Minutes meanwhile. Finder may list it twice for a while.

## The first iPhone → Mac send is slow to connect, or sits on "Waiting"

This is the main known problem. The time goes into the iPhone finding the Mac's AirDrop service before any data moves, and it's worst when both devices have been idle. Cancelling and sending again usually connects quickly. Mac → iPhone isn't affected.

## Transfers are slower than expected

- **Check the iPhone's Wi-Fi first.** AirDrop gets the share of the iPhone's radio time that its own Wi-Fi leaves over. If the iPhone isn't on Wi-Fi, or is on channel 149, it gets 15 of 16 slots (about 22 MB/s). On a quiet network on another channel it gets 11 (about 15 MB/s), on a busy one 7 (about 6 MB/s). `ITLAWDLCoexPeerPresentEntries` shows the current number.
- **Check the channel:** on 6, everything is very slow.
- **The very first send after boot** is often slower while rate control settles.

## Normal Wi-Fi stopped working

- Remove any flag listed under "Don't use" in [BOOT-FLAGS.md](BOOT-FLAGS.md).
- A fast-rising `ITLAWDLCoexCountryNotices`, with `ITLAWDLCoexAdded` flipping between Yes and No, means the firmware is restarting around the listener. Boot without the `-itlawdl*` flags to confirm the station works on its own, then report it.
- Use USB tethering from the iPhone to get online while you test.

## iMessage or other Apple services don't work over Wi-Fi

Check the link quality macOS sees:

```bash
printf 'show State:/Network/Interface/en0/LinkQuality\n' | scutil
```

- **It should be 100 (or 50 or 10 on a weak signal).** ForbiddenItlwm 0.2.0 and later set it on Tahoe, and 0.3.0 and later on Sequoia. With -2 ("off"), Apple Push won't connect over Wi-Fi, and iMessage, FaceTime and other push-based services fail. Older builds and the original AirportItlwm for Tahoe leave it at -2.
- **If iMessage on the Mac only sends as text messages** (relayed through the iPhone) and they fail, re-enable the Mac on the iPhone: Settings → Apps → Messages → Text Message Forwarding. A new SMBIOS ROM makes the Mac a new device to Apple.
- **For iMessage on a Hackintosh in general:** use a serial that Apple's coverage check calls invalid. Set ROM to the Wi-Fi card's permanent MAC address (`ifconfig en0 | grep ether`).

## Reporting a bug

Include:
- the output of `bash tools/compat/collect_hw_macos.sh` (read it first; it leaves out addresses and names)
- the `ioreg` lines above, taken right after the problem
- what you did, and the time

Don't post `-itlawdlsamples` output or iPhone sysdiagnoses publicly: they contain device names, addresses and personal data.
