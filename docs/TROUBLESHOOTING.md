# Troubleshooting

Start with `bash tools/compat/check_compat.sh`. Then read the driver's own state:

```bash
ioreg -l -w0 | grep -E '"ItlwmAWDLInterfaceBuild"|"ITLAWDLCoex(Added|Channel|OwnWidth|CountryLearned|CountryNotices|PeerSyncFrames|PeerPresentEntries|TxOK|TxSubmitErrors)"'
```

| Key | Healthy value |
| --- | --- |
| `ItlwmAWDLInterfaceBuild` | `forbidden-0.1.0-beta1` |
| `ITLAWDLCoexAdded` | Yes: the AirDrop listener exists |
| `ITLAWDLCoexChannel`, `ITLAWDLCoexOwnWidth` | 149 and 80 |
| `ITLAWDLCoexCountryLearned` | Yes |
| `ITLAWDLCoexCountryNotices` | A small number. It climbs fast if the firmware keeps restarting |
| `ITLAWDLCoexPeerSyncFrames` | Rises while an Apple device with AirDrop open is nearby |
| `ITLAWDLCoexPeerPresentEntries` | How many of the iPhone's 16 time slots it spends on the listener's channel |
| `ITLAWDLCoexTxSubmitErrors` | 0 |

The driver's log doesn't reach the unified log, so its state is only in the I/O Registry, and it's lost at reboot.

## The listener is on channel 6, or `CountryLearned` is No

The firmware opens channel 149 only after a nearby network tells it the country.
- Make sure some 5 GHz network that advertises a country is in range. Switching on an iPhone's Personal Hotspot nearby, without joining it, has worked.
- Wait a minute after it appears.
- Some countries don't allow channels 149 to 161, or allow them only at 20 or 40 MHz. There AirDrop stays on channel 6. The checker warns about known ones.
- The driver never overrides this decision, and `itlwm_cc` doesn't help.

## The iPhone doesn't list the Mac

- **Bluetooth:** it must be on, with BTLEAdvFixup loaded, `-btleadv` set and IntelBTPatcher disabled. AirDrop finds devices over Bluetooth LE first.
- **AirDrop settings:** with Contacts Only, the two devices must share an Apple Account or know each other as contacts. Try Everyone for 10 Minutes to rule this out.
- **Is the iPhone heard?** Read `ITLAWDLCoexPeerSyncFrames` twice, a few seconds apart, with the iPhone's share sheet open. If it rises, the radios hear each other.
- **Unlock the iPhone and keep the share sheet open** for 10 to 15 seconds.
- **The iPhone itself can get stuck.** During testing there was a period when the iPhone listed no Mac on any build, including builds that had worked earlier that day. Several restarts didn't fix it, and a factory reset of the iPhone did. If the Mac's counters look healthy, look at the iPhone before the driver. Try these in order:
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

## Reporting a bug

Include:
- the output of `bash tools/compat/collect_hw_macos.sh` (read it first; it leaves out addresses and names)
- the `ioreg` lines above, taken right after the problem
- what you did, and the time

Don't post `-itlawdlsamples` output or iPhone sysdiagnoses publicly: they contain device names, addresses and personal data.
