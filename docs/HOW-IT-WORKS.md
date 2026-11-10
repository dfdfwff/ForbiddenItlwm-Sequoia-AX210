# How ForbiddenItlwm works

AirDrop finds a device over Bluetooth LE, then moves the data over **AWDL** (Apple Wireless Direct Link), a peer-to-peer Wi-Fi link. On a real Mac, Apple's Wi-Fi stack runs most of AWDL itself: elections, timing, service discovery and the data path. It needs a driver that can create an AWDL interface and move its frames on the right channel at the right time.

ForbiddenItlwm makes macOS Tahoe's native stack (IO80211Family with Skywalk) believe it has a capable AWDL radio, and does the radio work Intel firmware doesn't do for it. The approach follows FranciumOS-org/AirPort_RTW89, which did this on a Realtek card with the older Wi-Fi stack.

## The pieces

```
           macOS (sharingd, rapportd, mDNSResponder, wifip2pd)
                               │
          IO80211 AWDL peer manager  (Apple's, unchanged)
                               │
   ItlAWDLInterface: awdl0  ── AirportItlwm/AirportItlwmV2.cpp
                               │  9 itlAWDL* calls
   AWDL listener, schedule, ── itlwm/hal_iwx/ItlIwx.cpp
   Block Ack, send gate
                               │
            Intel firmware: station context (normal Wi-Fi)
                          + listener context on 149/80 (AirDrop)
```

### 1. The interface: `awdl0`

- `wifip2pd` asks for an AWDL interface only if the card reports the AWDL capability. `-itlawdliface` sets it.
- `AirportItlwm::createVirtualInterface` then builds `ItlAWDLInterface`, a concrete subclass of Tahoe's `IO80211VirtualInterface` and `IO80211AWDLProtocol`. Apple's classes are abstract, so the subclass has to match Apple's virtual table slot for slot. `tools/abi/audit_vtable.py` checks all 568 slots against 25G241's table. This is why the driver is tied to one macOS build.
- Setup creates the Skywalk packet pools and queues and registers `awdl0`.
- Apple's peer manager then pushes its settings and its frame template (the "MI" frame with the Mac's name and services) through `setAWDL_*` calls. The driver keeps copies.

### 2. The radio: a second firmware context

Normal Wi-Fi uses the firmware's station context. ForbiddenItlwm adds a second MAC context, a **listener**, with:
- its own PHY context on channel 149 at 80 MHz (channel 6 when 149 isn't allowed)
- its own firmware station and transmit queue
- the AWDL address

The firmware time-shares the radio between the station's channel and the listener's. So normal Wi-Fi stays connected while AirDrop runs.

- **Receive:** the driver picks AWDL action frames and data frames out of what the listener receives. It hands action frames to Apple's peer manager with real RSSI metadata, which the peer manager needs before it accepts a peer. IPv6 data goes into `awdl0`.
- **Transmit:** macOS's sync frames are built from Apple's template and sent every 110 TU. With `-itlawdlsvcmerge` its service records are merged in, and with `-itlawdlawsync` idle frames are timed to the iPhone's windows. `awdl0`'s IPv6 packets go out the listener's queue with AWDL data framing.

### 3. Following the iPhone's schedule

An iPhone doesn't stay on one channel. Its sync frames carry a 16-slot channel sequence, and on each slot it is on our channel, on its own Wi-Fi network's channel, or dozing.
- **Tracking:** `include/AWDLPeerSchedule.hpp` tracks that sequence and its timing. It assumes offset 0 until the iPhone's own frames prove otherwise.
- **The send gate:** frames for the iPhone go out only while it is on our channel. While it is idle, they go only at the start of its availability windows. Before the gate, frames sent while the iPhone was away were lost. The losses made QUIC slow itself down, so Mac → iPhone ran at a fraction of the link speed.
- **Peer pause:** while the iPhone is away, its firmware station queue is paused, so the firmware doesn't burn retries on it.

### 4. Speed: Block Ack and aggregation

With `-itlawdlcoexbatx` the driver negotiates Block Ack with the iPhone and lets the firmware send A-MPDU aggregates at VHT 80 MHz two-stream rates. Receive Block Ack works too. `-itlawdlcaps` advertises the matching capabilities, so the iPhone sends at the same rates.

The `awdl0` transmit pool is 2048 packets. With the 128 used during bring-up, long Mac → iPhone transfers ran the pool dry and stalled, because the flowswitch drops on allocation failure without telling QUIC.

### 5. Bluetooth: BTLEAdvFixup

`bluetoothd` only turns on extended LE advertising after chip checks that a third-party controller fails, so on an Intel controller the Mac never advertises AirDrop. BTLEAdvFixup is a small Lilu plugin that, with `-btleadv`:
- patches `bluetoothd`'s pages in memory as they're validated (the way BlueToolFixup does) to take the "supported" path
- adjusts outgoing HCI commands, such as the random address of each advertising set, and takes over IntelBTPatcher's fix-up before the first scan (IntelBTPatcher must be disabled, since Lilu panics if two plugins hook the same function)

Its byte patterns match 25G241's `bluetoothd` only. On another build they don't match, and nothing is changed.

### 6. Country and channel 149

Intel firmware starts in world mode, where channels 149 to 161 are listen-only. It opens them after it learns the country from a nearby network's beacons, if that country's rules allow 80 MHz there. The driver asks the firmware, follows its answer and never overrides it. Until then the listener uses channel 6, which works but is slow: the iPhone spends few slots there, and 2.4 GHz rates are low.

## Why speed and connect time vary

- **Speed follows the iPhone's own Wi-Fi.** Its channel sequence gives AirDrop the slots its station doesn't need:
  - about 15 of 16 slots on our channel (about 22 MB/s) when it isn't associated or is associated on 149
  - about 11 (15 MB/s) on a quiet network on another channel
  - about 7 (6 MB/s) on a busy one
- **Cold iPhone → Mac connects** spend their time in AWDL service discovery, before any data connection:
  - The iPhone and Mac exchange service records in action frames.
  - While the iPhone is idle it sits on our channel in only a few slots, and the Mac's radio is often on the station's channel then.
  - The current reading is that the firmware's time-sharing favours the station while the listener is idle. Improving this is the main open work.

## Things worth knowing

These each took one or more test boots to learn.

- **AWDL services travel in TLV type 2 in MI frames,** not as ordinary mDNS on the air. Names are compressed (`c0 07` is `_airdrop._tcp.local`), so a text search misses them.
- **Peer creation needs real RSSI** in the frame metadata. With 0 there, the peer manager never adds the peer.
- **`awdl0` is receive-restricted.** Ordinary sockets can't receive on it, so `nc` and similar tools time out even when replies arrive. Judge connectivity with a packet capture.
- **Use `/usr/bin/log`.** In zsh, `log` is a built-in, so a bare `log show` prints nothing useful. `sharingd`, `rapportd`, `mDNSResponder` and `wifip2pd` logs show what AirDrop is doing.
- **The firmware gives a listener context group data only in promiscuous mode,** whatever the multicast filter says.
- **Cross-band retunes on a live binding crash the firmware.** Band changes are refused.
- **Firmware TX status depends on the frame:** for broadcasts it only means the frame left the queue.
- **ACKs must match the MAC context's address.** The listener carries the AWDL address so the firmware credits the peer's ACKs.
- **`LOW_LATENCY_CMD` fails on the AX211's firmware** and takes Wi-Fi down with it, so it is disabled.

## Source layout

| Path | Contents |
| --- | --- |
| `AirportItlwm/AirportItlwmV2.cpp` | The macOS side: interface setup, Skywalk queues and pools, sync frame bridge, data in and out of `awdl0` |
| `AirportItlwm/AirportAWDLInterface.hpp`, `AWDLInactiveProtocol.hpp`, `AWDLRequestedValue.hpp` | `ItlAWDLInterface` and its stored settings |
| `itlwm/hal_iwx/ItlIwx.cpp` | The radio side: the listener context, country handling, receive and transmit, Block Ack, the send gate |
| `include/AWDL*.hpp` | Frame parsing and building, data framing, peer schedule, regulatory rules. Mostly host-testable |
| `include/Airport/macOS26/` | Tahoe's native class declarations, for 25G241 |
| `BTLEAdvFixup/` | The Bluetooth LE advertising plugin |
| `tests/`, `tools/test_awdl.py` | Offline tests, many of them on real driver functions extracted into mocks |
| `tools/build.py`, `tools/sources.txt` | Command Line Tools build of `ForbiddenItlwm.kext` |
| `tools/build_btleadv.py` | Build of `BTLEAdvFixup.kext` |
| `tools/abi/` | The virtual table audit against 25G241 |
| `tools/awdl_*.py` | Readers for the driver's registry diagnostics |
| `tools/compat/` | Tester scripts |

Everything else (`itl80211`, `itlwm/hal_iwm`, `itlwm/hal_iwn`, `include/HAL`) is AirportItlwm's normal Wi-Fi driver.
