#include <AWDLRegulatory.hpp>
#include <cassert>
#include <cstdio>
int main() {
    using namespace ItlAWDL;
    const uint8_t table[] = {1, 6, 36, 149};
    const uint32_t flags[] = {10, 60, 360, 1490};
    uint32_t out = 0;
    assert(regulatoryChannelFlags(table, 4, 149, flags, 4, &out) && out == 1490);
    assert(regulatoryChannelFlags(table, 4, 6, flags, 4, &out) && out == 60);
    // A channel missing from the table, or past a short reply, isn't reported.
    out = 7;
    assert(!regulatoryChannelFlags(table, 4, 44, flags, 4, &out) && out == 7);
    assert(!regulatoryChannelFlags(table, 4, 149, flags, 3, &out) && out == 7);
    assert(!regulatoryChannelFlags(nullptr, 4, 6, flags, 4, &out));
    assert(!regulatoryChannelFlags(table, 4, 6, nullptr, 4, &out));
    assert(!regulatoryChannelFlags(table, 4, 6, flags, 4, nullptr));
    MccNotice notices[2] = {};
    unsigned count = 0;
    assert(regulatoryAppend(notices, 2, count, MccNotice{1, 2, 5, 0}));
    assert(regulatoryAppend(notices, 2, count, MccNotice{3, 4, 5, 0}));
    assert(!regulatoryAppend(notices, 2, count, MccNotice{5, 6, 5, 0}) && count == 2 && notices[1].mcc == 4);
    assert(mccEchoDue(false, 5, 5));
    assert(!mccEchoDue(true, 34, 5) && mccEchoDue(true, 35, 5));
    assert(mccEchoDue(true, 4, 5));
    assert(countryNoticeUsable('F' << 8 | 'R', 5));
    assert(!countryNoticeUsable('F' << 8 | 'R', 2)); // Only what the firmware learned from Wi-Fi.
    assert(!countryNoticeUsable('Z' << 8 | 'Z', 5));
    assert(!countryNoticeUsable(0x3030, 5));
    assert(!countryNoticeUsable('f' << 8 | 'R', 5));
    {
        uint8_t beacon[64] = {0x80};
        beacon[36] = 0; beacon[37] = 2; beacon[38] = 'x'; beacon[39] = 'y'; // SSID first.
        beacon[40] = 7; beacon[41] = 3; beacon[42] = 'F'; beacon[43] = 'R'; beacon[44] = ' ';
        uint8_t cc[2] = {};
        assert(beaconCountry(beacon, 45 + 4, cc) && cc[0] == 'F' && cc[1] == 'R');
        // Without the element, or with it cut off by the end of the frame.
        assert(beaconCountry(beacon, 40 + 4, cc) && cc[0] == '-' && cc[1] == '-');
        assert(beaconCountry(beacon, 44 + 4, cc) && cc[0] == '-');
        // Not a beacon, too short, or a length running past the end.
        beacon[0] = 0xd0; assert(!beaconCountry(beacon, 49, cc)); beacon[0] = 0x50;
        assert(beaconCountry(beacon, 49, cc) && cc[0] == 'F');
        assert(!beaconCountry(beacon, 39, cc) && !beaconCountry(nullptr, 49, cc));
        beacon[37] = 200; assert(beaconCountry(beacon, 49, cc) && cc[0] == '-');
        BeaconCountry seen[2] = {}; unsigned n = 0; uint32_t over = 0;
        const uint8_t in[2] = {'F', 'R'}, none[2] = {'-', '-'};
        noteBeaconCountry(seen, 2, n, over, in, 36); noteBeaconCountry(seen, 2, n, over, in, 36);
        noteBeaconCountry(seen, 2, n, over, none, 36); noteBeaconCountry(seen, 2, n, over, in, 149);
        assert(n == 2 && over == 1 && seen[0].count == 2 && seen[1].first == '-');
    }
    {
        uint8_t f[80] = {};
        const uint8_t tlvs[] = {4, 2, 0, 9, 9, 12, 5, 0, 0x23, 0xbf, 'F', 'R', 0};
        for (unsigned i = 0; i < sizeof(tlvs); ++i) f[40 + i] = tlvs[i];
        uint16_t c = 0;
        assert(peerCountryFromAction(f, 40 + sizeof(tlvs), &c) && c == uint16_t('F' << 8 | 'R'));
        // Cut short, too small, the world domain, lower case, none there, or bad arguments.
        assert(!peerCountryFromAction(f, 40 + sizeof(tlvs) - 1, &c));
        f[46] = 3; assert(!peerCountryFromAction(f, 40 + sizeof(tlvs), &c)); f[46] = 5;
        f[50] = 'Z'; f[51] = 'Z'; assert(!peerCountryFromAction(f, 40 + sizeof(tlvs), &c));
        f[50] = 'f'; f[51] = 'R'; assert(!peerCountryFromAction(f, 40 + sizeof(tlvs), &c));
        f[50] = 'F'; f[45] = 13; assert(!peerCountryFromAction(f, 40 + sizeof(tlvs), &c)); f[45] = 12;
        assert(!peerCountryFromAction(f, 39, &c) && !peerCountryFromAction(nullptr, 80, &c) && !peerCountryFromAction(f, 80, nullptr));
        PeerCountryState p;
        const uint16_t fr = 'F' << 8 | 'R', de = 'D' << 8 | 'E';
        for (unsigned i = 0; i < 4; ++i) notePeerCountry(p, fr);
        assert(!p.confirmed);
        notePeerCountry(p, fr);
        assert(p.confirmed == fr && !p.changes);
        // A different country needs its own run, and the old one stands until then.
        notePeerCountry(p, de);
        assert(p.confirmed == fr && p.changes == 1 && p.run == 1);
        for (unsigned i = 0; i < 4; ++i) notePeerCountry(p, de);
        assert(p.confirmed == de);
    }
    {
        const uint64_t first = kBackgroundScanFirstUS, every = kBackgroundScanEveryUS, idle = kChannelMoveIdleUS;
        // Never on 149, however long it's been.
        assert(!backgroundScanDue(false, false, first, 0, 0));
        assert(!backgroundScanDue(false, false, every * 1000, 1000, 0));
        // A long stay on 149 means a scan is due as soon as it's back on 6.
        assert(backgroundScanDue(true, false, every * 1000, 1000, 0));
        // A restart on 149 skips the scan only while the firmware still allows it.
        assert(!startupScanNeeded(149, true) && startupScanNeeded(149, false));
        assert(startupScanNeeded(6, true) && startupScanNeeded(6, false));
        assert(!forcedMoveDue(0, false, 149, first) && !forcedMoveDue(10, true, 149, first) && !forcedMoveDue(10, false, 6, first));
        assert(!forcedMoveDue(10, false, 149, 9999999) && forcedMoveDue(10, false, 149, 10000000));
        assert(!backgroundScanDue(true, true, first * 9, 0, 0));   // Never while busy.
        assert(!backgroundScanDue(true, false, first - 1, 0, 0) && backgroundScanDue(true, false, first, 0, 0));
        assert(!backgroundScanDue(true, false, 1000 + every - 1, 1000, 0) && backgroundScanDue(true, false, 1000 + every, 1000, 0));
        // Not soon after data, in either direction.
        assert(!backgroundScanDue(true, false, first + 5, 0, first + 5 - idle + 1) && backgroundScanDue(true, false, first + 5, 0, first + 5 - idle));
        assert(channelMoveUpAllowed(5, 0) && !channelMoveUpAllowed(kChannelMoveUpEveryUS, 1) && channelMoveUpAllowed(1 + kChannelMoveUpEveryUS, 1));
        assert(!channelMoveReady(true, idle * 9, 0) && !channelMoveReady(false, idle - 1, 0) && channelMoveReady(false, idle, 0));
    }
    puts("AWDL regulatory record: passed (offline only)");
}
