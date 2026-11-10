"""Exercise the actual sender wrapper/completion bodies with host ownership mocks.

This does not exercise DMA, interrupts, firmware, or physical transmission.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "itlwm/hal_iwx/ItlIwx.cpp").read_text()
start = source.index("int ItlIwx::iwx_tx_monitor_frame(")
sender = source[start:source.index("\nint ItlIwx::", start + 8)]
start = source.index("void ItlIwx::\niwx_txd_done(")
completion = source[start:source.index("\nvoid ItlIwx::", start + 8)]

reset_start = source.index("iwx_reset_tx_ring(struct")
reset_start = source.index("if (data == awdlMonitorPending)", reset_start)
reset_end = source.index("{", reset_start)
depth = 1
reset_end += 1
while depth:
    depth += (source[reset_end] == "{") - (source[reset_end] == "}")
    reset_end += 1
reset_block = source[reset_start:reset_end]


program = r'''
#include <AWDLProbePolicy.hpp>
#include <cassert>
#include <cerrno>
#include <cstdio>
struct Packet { unsigned frees = 0; };
using mbuf_t = Packet*;
static void mbuf_freem(Packet *p) { assert(p && !p->frees); ++p->frees; }
struct ieee80211_node { unsigned references = 0; };
struct iwx_node { ieee80211_node in_ni; };
struct iwx_tx_data { Packet *m = nullptr; iwx_node *in = nullptr; };
struct Ifnet { int if_timer = 0; };
struct ieee80211com { ieee80211_node *ic_bss = nullptr; Ifnet interface; };
struct iwx_softc { ieee80211com sc_ic; int sc_generation = 1; int sc_tx_timer = 0; };
#define IC2IFP(p) (&(p)->interface)
#define KASSERT(p, message) assert(p)
static ieee80211_node *ieee80211_ref_node(ieee80211_node *p) { ++p->references; return p; }
static void ieee80211_release_node(ieee80211com*, ieee80211_node *p) { assert(p->references); --p->references; }
static void awdlTxLog(const char*, ...) {}
static bool awdlRx = true, awdlSyncTrain = false, awdlFastProbe = false, awdlPeerTrain = false;
static ItlAWDL::AckProbe awdlAckPending = ItlAWDL::AckProbe::None;
static unsigned awdlDataInFlight = 0;
static bool awdlDataTx = false;
static bool awdlKickTransmit = false;
static int awdlRxTimer, timerKicks;
static void timeout_add_usec(int *timer, int) { assert(timer == &awdlRxTimer); ++timerKicks; }
static unsigned awdlTrainLimit() { return awdlDataTx ? 0x40000000U : ItlAWDL::probeLimit(awdlFastProbe, awdlPeerTrain); }
enum { EDCA_AC_BE = 0 };
struct Loop { bool gated = true; bool inGate() { return gated; } };
class ItlIwx {
public:
    Loop loop; bool hasLoop = true;
    bool awdlMonitorTxArmed = true, awdlMonitorTxAttempted = false;
    bool awdlMonitorCompletedOK = false, awdlMonitorProbeStarted = true;
    unsigned awdlMonitorSubmitted = 0;
    int awdlMonitorGeneration = 1, awdlMonitorQueue = 1;
    iwx_tx_data *awdlMonitorPending = nullptr;
    iwx_tx_data descriptor;
    int injectedError = 0, submissions = 0;
    Loop *getMainWorkLoop() { return hasLoop ? &loop : nullptr; }
    int iwx_tx_monitor_frame(iwx_softc*, mbuf_t, ieee80211_node*);
    void iwx_txd_done(iwx_softc*, iwx_tx_data*);
    void resetProbeData(iwx_softc*, iwx_tx_data*);
    int iwx_tx(iwx_softc*, Packet *p, ieee80211_node *n, int, int queue) {
        assert(queue == 1 && n->references == 1); ++submissions;
        if (injectedError) { mbuf_freem(p); return injectedError; }
        descriptor.m = p; descriptor.in = reinterpret_cast<iwx_node*>(n);
        awdlMonitorPending = &descriptor;
        return 0;
    }
};
'''
program += sender + "\n" + completion + "\nvoid ItlIwx::resetProbeData(iwx_softc *sc, iwx_tx_data *data) {" + reset_block + "}\n"
program += r'''
int main() {
    for (int mode = 0; mode < 9; ++mode) {
        ItlIwx driver; iwx_softc sc; iwx_node node; Packet packet;
        sc.sc_ic.ic_bss = &node.in_ni;
        awdlRx = mode != 0;
        if (mode == 1) driver.awdlMonitorTxArmed = false;
        if (mode == 2) driver.awdlMonitorTxAttempted = true;
        if (mode == 3) driver.loop.gated = false;
        if (mode == 4) driver.hasLoop = false;
        if (mode == 5) driver.awdlMonitorGeneration = 0;
        if (mode == 6) driver.awdlMonitorPending = &driver.descriptor;
        ieee80211_node other;
        auto *argument = mode == 7 ? &other : mode == 8 ? nullptr : &node.in_ni;
        assert(driver.iwx_tx_monitor_frame(&sc, &packet, argument) == EOPNOTSUPP);
        assert(packet.frees == 1 && node.in_ni.references == 0 && driver.submissions == 0);
    }
    awdlRx = true;
    unsigned successfulTests = 0;
    for (int error : {0, 0, EACCES, ENOBUFS, ENXIO}) {
        ItlIwx driver; iwx_softc sc; iwx_node node; Packet packet;
        sc.sc_ic.ic_bss = &node.in_ni; driver.injectedError = error;
        assert(driver.iwx_tx_monitor_frame(&sc, &packet, &node.in_ni) == error);
        assert(driver.submissions == 1 && driver.awdlMonitorTxAttempted);
        if (error) assert(packet.frees == 1 && node.in_ni.references == 0);
        else {
            assert(packet.frees == 0 && node.in_ni.references == 1);
            assert(sc.sc_tx_timer == 15 && sc.sc_ic.interface.if_timer == 1);
            if (!successfulTests++) driver.iwx_txd_done(&sc, &driver.descriptor);
            else driver.resetProbeData(&sc, &driver.descriptor);
            assert(packet.frees == 1 && node.in_ni.references == 0);
            assert(!driver.awdlMonitorPending && !driver.descriptor.m && !driver.descriptor.in);
        }
    }
    awdlSyncTrain = true;
    for (bool fast : {false,true}) {
        awdlFastProbe=fast;
        const unsigned limit=ItlAWDL::probeLimit(fast);
        ItlIwx driver; iwx_softc sc; iwx_node node;
        sc.sc_ic.ic_bss=&node.in_ni;
        for(unsigned i=1;i<=limit;++i) {
            Packet packet;
            assert(driver.iwx_tx_monitor_frame(&sc,&packet,&node.in_ni)==0);
            assert(driver.awdlMonitorSubmitted==i);
            driver.awdlMonitorCompletedOK=true;
            driver.iwx_txd_done(&sc,&driver.descriptor);
            assert(packet.frees==1 && !node.in_ni.references);
            assert(driver.awdlMonitorTxAttempted==(i==limit));
            assert(driver.awdlMonitorProbeStarted==(i==limit));
            driver.awdlMonitorProbeStarted=true;
        }
        Packet extra;
        assert(driver.iwx_tx_monitor_frame(&sc,&extra,&node.in_ni)==EOPNOTSUPP && extra.frees==1);
    }
    {
        // A failed unicast probe rearms the train; a failed broadcast never does.
        ItlIwx driver; iwx_softc sc; iwx_node node; Packet probe, plain;
        sc.sc_ic.ic_bss=&node.in_ni;
        assert(driver.iwx_tx_monitor_frame(&sc,&probe,&node.in_ni)==0);
        awdlAckPending=ItlAWDL::AckProbe::ControlAWDL; driver.awdlMonitorCompletedOK=false;
        driver.iwx_txd_done(&sc,&driver.descriptor);
        assert(probe.frees==1 && !driver.awdlMonitorTxAttempted && awdlAckPending==ItlAWDL::AckProbe::None);
        assert(driver.iwx_tx_monitor_frame(&sc,&plain,&node.in_ni)==0);
        driver.awdlMonitorCompletedOK=false;
        driver.iwx_txd_done(&sc,&driver.descriptor);
        assert(plain.frees==1 && driver.awdlMonitorTxAttempted);
    }
    {
        // A failed data frame keeps the path open, and the larger limit applies.
        ItlIwx driver; iwx_softc sc; iwx_node node; Packet one, two;
        sc.sc_ic.ic_bss=&node.in_ni;awdlDataTx=true;driver.awdlMonitorSubmitted=20000;
        assert(driver.iwx_tx_monitor_frame(&sc,&one,&node.in_ni)==0);
        awdlDataInFlight=2;driver.awdlMonitorCompletedOK=false;timerKicks=0;
        driver.iwx_txd_done(&sc,&driver.descriptor);
        assert(one.frees==1 && !driver.awdlMonitorTxAttempted && !awdlDataInFlight);
        // The next queued frame goes without waiting for the timer, once the slot is reclaimed.
        assert(awdlKickTransmit && !timerKicks);awdlKickTransmit=false;
        // Once in flight state is clear, a failure is a real transmit fault again.
        assert(driver.iwx_tx_monitor_frame(&sc,&two,&node.in_ni)==0);
        driver.awdlMonitorCompletedOK=false;
        driver.iwx_txd_done(&sc,&driver.descriptor);
        assert(two.frees==1 && driver.awdlMonitorTxAttempted);
        assert(!awdlKickTransmit);
        awdlDataTx=false;
    }
    for(unsigned mode=0;mode<3;++mode) {
        ItlIwx driver; iwx_softc sc; iwx_node node; Packet packet;
        sc.sc_ic.ic_bss=&node.in_ni;
        assert(driver.iwx_tx_monitor_frame(&sc,&packet,&node.in_ni)==0);
        driver.awdlMonitorCompletedOK=mode!=0;
        if(mode==1) ++sc.sc_generation;
        if(mode==2) driver.resetProbeData(&sc,&driver.descriptor);
        else driver.iwx_txd_done(&sc,&driver.descriptor);
        assert(driver.awdlMonitorTxAttempted && driver.awdlMonitorProbeStarted);
        assert(packet.frees==1 && !node.in_ni.references);
    }
    puts("Monitor TX wrapper/completion ownership: passed (host mocks only)");
}
'''
program = "#include <initializer_list>\n" + program
with tempfile.TemporaryDirectory(prefix="itlwm-tx-ownership-") as directory:
    path = Path(directory) / "test.cpp"
    path.write_text(program)
    output = Path(directory) / "test"
    subprocess.run(["xcrun", "clang++", "-std=c++14", "-I"+str(root/"include"), "-fsanitize=address,undefined",
                    str(path), "-o", str(output)], check=True)
    subprocess.run([str(output)], check=True)
