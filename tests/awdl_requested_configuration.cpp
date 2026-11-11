// User-space checks using the same ioctl record declarations as the driver.
#include <stddef.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <Airport/apple80211_ioctl.h>
#include "../ForbiddenItlwm/AWDLRequestedValue.hpp"

int main()
{
    ItlAWDLRequestedValue<apple80211_awdl_bssid> bssid;
    apple80211_awdl_bssid reply;
    memset(&reply, 0xa5, sizeof(reply));
    const auto untouched = reply;
    assert(bssid.get(&reply) == kIOReturnNotReady);
    assert(memcmp(&reply, &untouched, sizeof(reply)) == 0);
    assert(bssid.get(nullptr) == kIOReturnBadArgument);
    assert(bssid.set(nullptr) == kIOReturnBadArgument);

    apple80211_awdl_bssid request = {};
    request.version = APPLE80211_VERSION;
    const unsigned char address[] = {0x00, 0x25, 0x00, 0xff, 0x94, 0x73};
    memcpy(request.bssid, address, sizeof(address));
    assert(bssid.set(&request) == kIOReturnSuccess);
    memset(&request, 0, sizeof(request)); // Storage must own a copy.
    assert(bssid.get(&reply) == kIOReturnSuccess);
    assert(reply.version == APPLE80211_VERSION);
    assert(memcmp(reply.bssid, address, sizeof(address)) == 0);
    // Native internal BSSID requests leave the version word zero.
    request = reply;
    request.version = 0;
    assert(bssid.set(&request) == kIOReturnSuccess);
    assert(bssid.get(&reply) == kIOReturnSuccess);
    assert(reply.version == APPLE80211_VERSION);
    request.version = 2; // Invalid updates must preserve accepted state.
    assert(bssid.set(&request) == kIOReturnBadArgument);
    assert(bssid.get(&reply) == kIOReturnSuccess);
    assert(memcmp(reply.bssid, address, sizeof(address)) == 0);

    // Exercise the largest stored record, including its last channel entry.
    ItlAWDLRequestedValue<apple80211_awdl_sync_channel_sequence> sequence;
    apple80211_awdl_sync_channel_sequence plan = {};
    plan.version = APPLE80211_VERSION;
    plan.length = APPLE80211_MAX_CHANNELS;
    plan.seqs[APPLE80211_MAX_CHANNELS-1].flags = 0x1234;
    assert(sequence.set(&plan) == kIOReturnSuccess);
    plan.seqs[APPLE80211_MAX_CHANNELS-1].flags = 0;
    assert(sequence.get(&plan) == kIOReturnSuccess);
    assert(plan.seqs[APPLE80211_MAX_CHANNELS-1].flags == 0x1234);
    // Reading one record must not affect another record.
    assert(bssid.get(&reply) == kIOReturnSuccess);
    assert(memcmp(reply.bssid, address, sizeof(address)) == 0);
    puts("AWDL requested configuration: passed");
}
