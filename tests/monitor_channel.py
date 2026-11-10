"""Check actual monitor retune bookkeeping against a fallible mock firmware.

Does not validate radio operation or regulatory eligibility for transmission.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "itlwm/hal_iwx/ItlIwx.cpp").read_text()
start = source.index("int ItlIwx::iwx_awdl_monitor_channel(")
body = source[start:source.index("\nint ItlIwx::", start + 8)]
program = r'''
#include <cassert>
#include <cerrno>
#include <cstdio>
enum { IEEE80211_CHAN_MAX = 255, IEEE80211_M_MONITOR = 8 };
enum { IEEE80211_CHAN_2GHZ = 0x80, IEEE80211_CHAN_5GHZ = 0x100, IWX_FLAG_BINDING_ACTIVE = 1 };
struct ieee80211_channel { unsigned ic_freq = 0; unsigned ic_flags = 0; };
struct Rates { unsigned rs_nrates = 0; unsigned rs_rates[12] = {}; };
struct Node { ieee80211_channel *ni_chan = nullptr; Rates ni_rates; unsigned ni_txrate = 9; };
struct ieee80211com {
    unsigned ic_opmode = IEEE80211_M_MONITOR;
    Node node; Node *ic_bss = &node;
    ieee80211_channel ic_channels[256];
    bool ic_chan_active[256] = {};
    Rates ic_sup_rates[2];
    ieee80211_channel *ic_ibss_chan = nullptr;
};
struct Phy { ieee80211_channel *channel = nullptr; };
struct iwx_softc { ieee80211com sc_ic; Phy sc_phyctxt[1]; unsigned sc_flags = 0; };
static bool awdlRx = true;
static bool isset(const bool *p, unsigned n) { return p[n]; }
static unsigned ieee80211_chan2mode(ieee80211com*, const ieee80211_channel *c) {
    return c->ic_freq > 3000 ? 1 : 0;
}
struct Loop { bool gated = true; bool inGate() { return gated; } };
class ItlIwx {
public:
    Loop loop; bool hasLoop = true; void *awdlMonitorPending = nullptr;
    int error = 0; unsigned calls = 0;
    Loop *getMainWorkLoop() { return hasLoop ? &loop : nullptr; }
    int iwx_phy_ctxt_update(iwx_softc*, Phy *p, ieee80211_channel *c, int, int, int) {
        ++calls; p->channel = c; return error;
    }
    int iwx_awdl_monitor_channel(iwx_softc*, unsigned);
};
'''
program += body
program += r'''
int main() {
    ItlIwx driver; iwx_softc sc; auto &ic = sc.sc_ic;
    for (unsigned n : {6U, 44U, 149U}) {
        ic.ic_channels[n].ic_freq = n == 6 ? 2437 : 5000 + n * 5;
        ic.ic_channels[n].ic_flags = n == 6 ? 0x704e0 : 0x8030340;
        ic.ic_chan_active[n] = true;
    }
    ic.ic_sup_rates[0].rs_nrates = 12; ic.ic_sup_rates[0].rs_rates[0] = 2;
    ic.ic_sup_rates[1].rs_nrates = 8; ic.ic_sup_rates[1].rs_rates[0] = 12;
    for (unsigned n : {149U, 6U, 44U, 6U, 149U}) {
        const unsigned flags = ic.ic_channels[n].ic_flags;
        assert(driver.iwx_awdl_monitor_channel(&sc, n) == 0);
        assert(ic.node.ni_chan == &ic.ic_channels[n] && ic.ic_ibss_chan == ic.node.ni_chan);
        assert(sc.sc_phyctxt[0].channel == ic.node.ni_chan && ic.node.ni_txrate == 0);
        assert(ic.node.ni_rates.rs_nrates == (n == 6 ? 12U : 8U));
        assert(ic.node.ni_rates.rs_rates[0] == (n == 6 ? 2U : 12U));
        assert(ic.ic_channels[n].ic_flags == flags); // passive RX remains passive
    }
    for (unsigned mode = 0; mode < 8; ++mode) {
        driver.hasLoop = mode != 0; driver.loop.gated = mode != 1;
        awdlRx = mode != 2; ic.ic_opmode = mode == 3 ? 1 : IEEE80211_M_MONITOR;
        ic.ic_bss = mode == 4 ? nullptr : &ic.node;
        driver.awdlMonitorPending = mode == 5 ? &driver : nullptr;
        ic.ic_chan_active[6] = mode != 6;
        ic.ic_sup_rates[0].rs_nrates = mode == 7 ? 0 : 12;
        const auto calls = driver.calls;
        assert(driver.iwx_awdl_monitor_channel(&sc, 6) != 0 && calls == driver.calls);
    }
    ic.ic_sup_rates[0].rs_nrates = 12;
    assert(driver.iwx_awdl_monitor_channel(&sc, 256) != 0);
    assert(driver.iwx_awdl_monitor_channel(&sc, 7) == EINVAL);
    driver.error = EIO;
    assert(driver.iwx_awdl_monitor_channel(&sc, 6) == EIO);
    assert(ic.node.ni_rates.rs_nrates == 0 && ic.node.ni_chan == &ic.ic_channels[149]);
    driver.error = 0;
    assert(driver.iwx_awdl_monitor_channel(&sc, 149) == 0 && ic.node.ni_rates.rs_nrates == 8);
    sc.sc_flags = IWX_FLAG_BINDING_ACTIVE;
    const auto calls = driver.calls;
    assert(driver.iwx_awdl_monitor_channel(&sc, 6) == EOPNOTSUPP);
    assert(driver.calls == calls && ic.node.ni_chan == &ic.ic_channels[149]);
    assert(ic.node.ni_rates.rs_nrates == 8);
    assert(driver.iwx_awdl_monitor_channel(&sc, 44) == 0);
    assert(ic.node.ni_chan == &ic.ic_channels[44] && ic.node.ni_rates.rs_nrates == 8);
    sc.sc_flags = 0;
    assert(driver.iwx_awdl_monitor_channel(&sc, 6) == 0);
    sc.sc_flags = IWX_FLAG_BINDING_ACTIVE;
    const auto calls2 = driver.calls;
    assert(driver.iwx_awdl_monitor_channel(&sc, 149) == EOPNOTSUPP && driver.calls == calls2);
    puts("monitor channel bookkeeping/failure tests passed");
}
'''
# initializer_list is needed by the deliberately tiny stand-alone mocks.
program = "#include <initializer_list>\n" + program
with tempfile.TemporaryDirectory(prefix="forbiddenitlwm-channel-") as directory:
    src = Path(directory) / "test.cpp"
    src.write_text(program)
    output = Path(directory) / "test"
    subprocess.run(["xcrun", "clang++", "-std=c++14", "-fsanitize=address,undefined",
                    str(src), "-o", str(output)], check=True)
    subprocess.run([str(output)], check=True)
