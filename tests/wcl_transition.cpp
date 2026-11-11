// WPA2/WPA3 transition networks as shown to macOS (WCL.hpp, selectWPA2TransitionMode).
#include <WCL.hpp>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using AirportWCL::selectWPA2TransitionMode;

// One RSN element: version 1, CCMP group and pairwise, the given AKM suite types, RSN capabilities.
static std::vector<uint8_t> rsn(std::initializer_list<uint8_t> akms, uint16_t caps) {
    std::vector<uint8_t> e = {48, 0, 1, 0, 0, 0x0f, 0xac, 4, 1, 0, 0, 0x0f, 0xac, 4, uint8_t(akms.size()), 0};
    for (uint8_t a : akms) { e.push_back(0); e.push_back(0x0f); e.push_back(0xac); e.push_back(a); }
    e.push_back(uint8_t(caps)); e.push_back(uint8_t(caps >> 8));
    e[1] = uint8_t(e.size() - 2);
    return e;
}
static std::vector<uint8_t> join(std::initializer_list<std::vector<uint8_t>> parts) {
    std::vector<uint8_t> out;
    for (const auto &p : parts) out.insert(out.end(), p.begin(), p.end());
    return out;
}
// Runs the conversion and returns the resulting IEs.
static std::vector<uint8_t> shown(std::vector<uint8_t> ies) {
    ies.resize(ies.size() + 8, 0xee);   // guard bytes: never read as IEs, never written
    const size_t length = selectWPA2TransitionMode(ies.data(), ies.size() - 8);
    for (size_t i = ies.size() - 8; i < ies.size(); ++i) assert(ies[i] == 0xee);
    ies.resize(length);
    return ies;
}
static const std::vector<uint8_t> ssid = {0, 3, 'n', 'e', 't'};
static const std::vector<uint8_t> rsnxe = {244, 1, 0x20};
static const std::vector<uint8_t> vendor = {221, 4, 0x00, 0x50, 0xf2, 2};

int main() {
    const auto psk = rsn({2}, 0x0080);
    // The network from issue #9: PSK, PSK-SHA256 and SAE, PMF capable. Shown as plain PSK.
    assert(shown(join({ssid, rsn({2, 6, 8}, 0x0080), vendor})) == join({ssid, psk, vendor}));
    // The PSK + SAE case that already worked still does.
    assert(shown(join({ssid, rsn({2, 8}, 0x0080)})) == join({ssid, psk}));
    // SAE with group-dependent hash (24) counts as SAE.
    assert(shown(join({ssid, rsn({2, 24}, 0x0080)})) == join({ssid, psk}));
    // After a conversion, the RSN Extension element goes too, wherever it is; other elements stay.
    assert(shown(join({ssid, rsnxe, rsn({2, 6, 8}, 0x0080), rsnxe, vendor})) == join({ssid, psk, vendor}));

    // Left exactly as they are:
    for (const auto &ies : {
             join({ssid, rsn({2, 8}, 0x00c0), rsnxe}),   // PMF required: WPA3 only in practice
             join({ssid, rsn({8}, 0x0080), rsnxe}),      // SAE only
             join({ssid, rsn({6, 8}, 0x0080)}),          // no plain PSK to fall back on
             join({ssid, rsn({2, 6}, 0x0080)}),          // no SAE: not a transition network
             join({ssid, rsn({2, 1, 8}, 0x0080)}),       // 802.1X mixed in
             join({ssid, rsn({2}, 0x0000), rsnxe}),      // plain WPA2: RSNXE untouched
             join({ssid, vendor}),                       // open
         })
        assert(shown(ies) == ies);
    // A malformed element list is returned unchanged.
    auto broken = join({ssid, rsn({2, 6, 8}, 0x0080)});
    broken[1] = 40;
    assert(shown(broken) == broken);
    puts("WCL WPA2/WPA3 transition networks: passed (host only)");
}
