# Will AirDrop work on my Intel Wi-Fi card?

These scripts are for testers. All three are read-only: they change nothing on your computer, need no extra software, and send nothing anywhere. The two collectors each write one text file that you read first and then share with the project.

| Script | Runs on | What it does |
| --- | --- | --- |
| `check_compat.sh` | macOS | Checks the macOS build, your Intel Wi-Fi and Bluetooth hardware and the kexts you have loaded, and says how likely the driver is to work |
| `collect_hw_macos.sh` | macOS | Writes a hardware report for adding or debugging support |
| `collect_hw_linux.sh` | Linux | Writes a richer hardware report, from a Linux live USB on the same laptop, for cards the driver doesn't support yet |

## 1. Check

```bash
bash check_compat.sh
```

It works before you install anything. If ForbiddenItlwm is already running, it also checks the AirDrop listener's channel and the boot flags.

It also checks channel 149 and the country:
- **Country code:** the code the Wi-Fi driver reports to macOS. `ZZ` means the firmware hasn't learned a country yet.
- **Channel map:** whether channels 149, 153, 157 and 161 are in it.
- **Your network:** which channel your Wi-Fi network is on.
- **Nearby networks:** how many nearby networks macOS lists, and how many are on 149 to 165.
- **With the AirDrop build running,** also whether the firmware learned a country this boot, and why it refused the listener's channel if it did.

The check prints your country code on screen. Leave it out if you paste the output somewhere public.

**Channel 149 also depends on the country's radio laws.** Learning the country isn't always enough: the firmware still decides from its rules for that country.
- **Not allowed at all:** in the Linux wireless regulatory database (read 2026-11-10), 44 of its 183 country entries have no Wi-Fi rule for channels 149 to 161, for example Japan, Turkey, Egypt and Saudi Arabia.
- **Only narrow:** a few countries (Kenya, the Central African Republic, Guam, El Salvador, North Korea) allow those channels only at 20 or 40 MHz. The fast mode needs 80 MHz.
- **What happens there:** AirDrop is expected to stay on channel 6, which is very slow.
- **The check:** when it sees one of these country codes, it says so.

Intel firmware uses its own built-in tables, not that database. They follow the same laws but can be stricter, so the firmware may also keep 149 closed in a country the database allows. When the AirDrop build is running, the check reports whether the firmware actually opened 149 after learning the country.

What the verdict means:

| Verdict | Cards | Meaning |
| --- | --- | --- |
| Tested | AX211 | The card this was developed on |
| Close relative | AX210, AX411, and AX201/AX203 or 9560-class radios on Alder Lake and newer | Same family and firmware generation as the AX211. Should work as is. Users report the AX210 and an AX1690i (AX411 class) working |
| Older family | AX200, and AX201/AX101 or 9560/9461/9462 on Ice, Comet or Tiger Lake | Untested code paths. Will probably need small fixes. One-antenna cards (AX101, 9461, 9462) need more |
| Large port | 7260, 7265, 3160, 3165, 3168, 8260, 8265, 9260, and the 9560/9461/9462 on Coffee and Cannon Lake | Not supported |
| Unsupported | Pre-2013 cards, Wi-Fi 7 (BE200, BE201), Meteor Lake CNVi | Not supported |

Two things stop it working whatever the card:
- **The macOS build.** With the Tahoe kexts, AirDrop starts on macOS 26.7.1 (25G241), 26.7.2 (25G332) and 26.7.3 (25G342). With the Sequoia kexts, on 15.8 (24H23), 15.8.2 (24H232, 15.8.3's kernel) and 15.8.3 (24H241), and as "expected" on other 15.8.x builds. With the Sonoma kexts (test build), on 14.8.9 (23J631). On other builds the driver keeps AirDrop off and works as plain Wi-Fi. The check also tests whether BTLEAdvFixup's patterns for your macOS version (taken from 26.7.1, 26.7.3, 15.8.3 or 14.8.9; the Sequoia set also matches 15.8 and 15.8.2) match your `bluetoothd`.
- **Bluetooth.** AirDrop finds devices over Bluetooth LE first, so it needs a working Intel Bluetooth controller.

A good card can still be slow. The firmware opens channel 149, the fast mode, only after a nearby network tells it which country it is in. Until then the driver falls back to channel 6, which is very slow. Most home and office routers advertise their country. In the project's tests, an iPhone's Personal Hotspot switched on nearby (without joining it) also taught the firmware. Where the firmware marks 149 to 161 passive (China, for example), 0.5.0 uses them once it has heard a network's beacon there, as Linux does. Otherwise the driver follows the firmware's choice, and setting `itlwm_cc` doesn't help: it only changes the code macOS displays.

**User reports:** users have reported the Tahoe kext working with an AX201 and with AX211s in other machines (26.7.1), the AX210 (26.7.2, 26.7.3) and an AX1690i (26.7.2), and Wi-Fi working with the Sequoia kext on AX201s. These reports aren't verified, and the table above is unchanged. See [docs/TESTED-HARDWARE.md](../../docs/TESTED-HARDWARE.md#user-reports).

On a CNVi laptop (Alder Lake and newer) the PCI ID alone can't tell an AX211 from an AX201 or a 9560. Load ForbiddenItlwm (or AirportItlwm) and run the check again: the firmware it loads names the radio.

## 2. Collect

On macOS:

```bash
bash collect_hw_macos.sh
```

The report is saved on your **Desktop** as `airdrop-compat-macos-<date>-<time>.txt`. To save it somewhere else, name a folder: `bash collect_hw_macos.sh ~/Downloads`. The script prints the full path when it starts and when it finishes, with commands to show the file in Finder or open it.

For a card that isn't supported yet, also boot a recent Linux live USB on the same laptop and run:

```bash
sudo bash collect_hw_linux.sh
```

The report is saved in the folder you run it from as `airdrop-compat-linux-<date>-<time>.txt`, unless you name another folder. The script prints the full path. On a live USB, copy the file to a USB stick or cloud folder before shutting down, or it will be lost.

Linux's driver reports the card's exact name, radio, firmware version and firmware API. `iw list` shows the bands, channel widths, antennas and how many channels the card can serve at once. That is what adding a card to the macOS driver needs. Install `iw` first if your distribution doesn't include it.

### What the reports contain

- **System:** macOS or Linux version, the SMBIOS model identifier (the Mac model the hackintosh is set up as), the laptop or motherboard maker and model, CPU name.
- **Wi-Fi card:** PCI IDs, the firmware the driver loaded, the driver's version and properties.
- **Bluetooth:** the Intel controller's USB IDs and the controller summary.
- **Loaded kexts:** the names and versions of third-party kexts.
- **Boot flags:** only the AirDrop and Bluetooth ones.
- **Wi-Fi interface:** supported channels, plus the current channel and width.
- **Channel 149 and country:** whether the firmware has learned a country (the code itself only with `--include-country`), and the channels of nearby networks, without their names. On Linux, the state of channels 149 to 165.
- **AirDrop driver:** its status counters, if its build is running.

### What they leave out

- serial numbers and hardware UUIDs
- MAC and Bluetooth addresses
- network names, and paired or nearby devices
- your user name, home folder and host name
- the Wi-Fi country code, unless you add `--include-country`

Anything shaped like a MAC address or UUID is replaced. Read the file before sharing anyway, especially:
- **Kext names:** some kexts carry their author's name in their identifier.
- **Supported channels:** the list hints at your region.
