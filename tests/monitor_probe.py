"""Exercise the actual one-shot caller with mocked radio state and mbufs.

Does not test firmware, DMA or over-the-air reception.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "itlwm/hal_iwx/ItlIwx.cpp").read_text()
start = source.index("void ItlIwx::iwx_awdl_transport_probe(")
body = source[start:source.index("\nint ItlIwx::", start + 8)]
program = r'''
#include <AWDLFrameBuilder.hpp>
#include <AWDLObservation.hpp>
#include <AWDLPeerProbe.hpp>
#include <AWDLProbePolicy.hpp>
#include <AWDLTxBurst.hpp>
#include <initializer_list>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
enum { IEEE80211_M_MONITOR = 8, IEEE80211_S_RUN = 4,
       IWX_FLAG_MAC_ACTIVE = 1, IWX_FLAG_STA_ACTIVE = 2,
       IWX_FLAG_HW_ERR = 4, IWX_FLAG_SHUTDOWN = 8, IWX_FLAG_RFKILL = 16,
       IEEE80211_CHAN_PASSIVE = 0x200, IEEE80211_CHAN_DFS = 0x80000,
       IEEE80211_CHAN_NOADHOC = 0x200000, MBUF_DONTWAIT = 0, MBUF_TYPE_DATA = 1 };
struct Channel { unsigned number = 6, ic_flags = 0; };
struct Node { Channel *ni_chan; struct { unsigned rs_nrates = 12; } ni_rates; };
struct ieee80211com {
    Node *ic_bss = nullptr;
    unsigned ic_state = IEEE80211_S_RUN, ic_opmode = IEEE80211_M_MONITOR;
    bool ic_chan_active[256] = {};
    uint8_t ic_myaddr[6] = {2,0,0,0,0,1};
};
struct iwx_softc {
    ieee80211com sc_ic;
    struct { Channel *channel = nullptr; } sc_phyctxt[1];
    int sc_generation = 1;
    unsigned sc_flags = IWX_FLAG_MAC_ACTIVE | IWX_FLAG_STA_ACTIVE;
};
struct Packet { uint8_t bytes[128] = {}; size_t length = 0, pkthdr = 0; unsigned frees = 0; };
using mbuf_t = Packet*;
static Packet storage;
static unsigned allocations;
static bool failAllocation;
static size_t capacity;
static int mbuf_gethdr(int, int, mbuf_t *p) {
    ++allocations; if (failAllocation) return ENOMEM; *p = &storage; return 0;
}
static int mbuf_getpacket(int, mbuf_t *p) {
    if (failAllocation) return ENOMEM;
    storage = {}; *p = &storage; return 0;
}
static mbuf_t mbuf_next(mbuf_t) { return nullptr; }
static size_t mbuf_maxlen(mbuf_t) { return capacity; }
static void *mbuf_data(mbuf_t p) { return p->bytes; }
static void mbuf_setlen(mbuf_t p, size_t n) { p->length = n; }
static void mbuf_pkthdr_setlen(mbuf_t p, size_t n) { p->pkthdr = n; }
static void mbuf_freem(mbuf_t p) { assert(p && !p->frees); ++p->frees; }
static bool isset(const bool *p, unsigned n) { return p[n]; }
static unsigned ieee80211_chan2ieee(ieee80211com*, Channel *p) { return p->number; }
static int ieee80211_min_basic_rate(ieee80211com*) { return 2; }
static void clock_get_uptime(uint64_t *p) { *p = 12345678; }
static void absolutetime_to_nanoseconds(uint64_t t, uint64_t *p) { *p = t * 1000; }
static void awdlTxLog(const char*, ...) {}
static void awdlSessLog(const char*, ...) {}
static uint8_t awdlMacOverride[6];
static uint64_t awdlMacLastTryUS;
static uint32_t awdlMacAddressChanges;
static bool awdlAwSync;
static uint32_t awdlAwSyncFrames;
static const uint64_t kAwdlAwSyncWindowUS = 2000, kAwdlAwSyncMinGapUS = 8000;
static struct { bool inPresentAwStart(uint64_t, uint64_t) const { return false; } } awdlPeerSchedule;
static bool awdlRx = true, awdlRxChannel6 = true, awdlSyncProbe = false;
static bool awdlPeerTrain = false, peerAudible = false;
static bool awdlPeerAudible() { return peerAudible; }
static uint8_t awdlOwnAddress[6];
static bool awdlAckProbe = false;
static ItlAWDL::AckProbe awdlAckPending = ItlAWDL::AckProbe::None;
static uint8_t awdlLastPeer[6];
static int awdlLastPeerRssi, awdlAckTargetRssi;
static uint8_t awdlAckTarget[6];
static bool awdlMacProbe = false, awdlMacTried = false, awdlElicit = false;
static uint8_t awdlLearnedPeer[6], awdlLearnedPeerIP[16];
static uint16_t awdlRequestSequence;
static bool awdlForeignControl = false;
static bool awdlDataTx = false;
static const unsigned awdlFixedChannel = 6;
static uint64_t mockUptimeUS, awdlLastSyncUS;
static uint64_t awdlUptimeUS() { return mockUptimeUS; }
static unsigned awdlDataInFlight, awdlDataSent[2], transmitFramesReady;
static struct { unsigned step = 1; } awdlDataRate;
static unsigned awdlDataStepInFlight;
static size_t awdlBaResponseLength;
static uint8_t awdlBaResponse[33];
static bool awdlBaResponseInFlight;
static unsigned awdlBaResponsesSent;
static unsigned awdlBaResponsesRefused;
static bool awdlBaTxSend(iwx_softc *, mbuf_t) { return false; }
static bool awdlBaTxQueueFull(iwx_softc *) { return false; }
static void awdlNoteServices(const uint8_t *, size_t, bool) {}
static bool awdlMcastUnicast;
static uint64_t awdlPeerLastHeardUS;
static uint32_t awdlMcastUnicastCopies, awdlMcastUnicastSkipped;
static uint32_t awdlTxWindowFullExits, awdlTxStackEmptyExits;
static bool awdlPeerPause;
static struct { bool paused; uint32_t framesHeldBack; } awdlPeerPauseState;
static const uint64_t kAwdlMcastPeerRecentUS = 10000000;
static unsigned awdlRateResults[9][4];
static ItlAWDL::ResendState awdlResend;
static uint8_t awdlResendFrame[1600];
static size_t awdlResendLength;
static unsigned awdlResendSent, awdlResendGaveUp;
static uint64_t awdlSubmitUS, awdlIdleSumUS, awdlLastDoneUS;
static unsigned awdlWindow = 1;
static ItlAWDL::TxBurst awdlBurst;
static mbuf_t awdlDeferredMulticast;
static bool awdlBackgroundScanActive;
static unsigned awdlMulticastCopies = 1, awdlMulticastRepeatsSent;
static ItlAWDL::MulticastRepeats awdlMulticastRepeats;
static bool awdlDataEverActive;
static uint64_t awdlLastDataUS;

static unsigned samplesKept;
static void awdlKeepSample(const char*, const uint8_t*, size_t) { ++samplesKept; }
using IOReturn = unsigned;
static const IOReturn kIOReturnSuccess = 0, kIOReturnNotReady = 2;
static unsigned syncBuildFails = 0;
static IOReturn itlAWDLBuildTransmit(void*, mbuf_t *packet) {
    if(!transmitFramesReady) return 3;
    --transmitFramesReady;storage={};storage.length=storage.pkthdr=80;storage.bytes[0]=8;
    storage.bytes[4]=awdlDataSent[0]+awdlDataSent[1] ? 2 : 0x33;
    *packet=&storage;return 0;
}
static IOReturn itlAWDLBuildSyncProbe(void*, mbuf_t *packet, uint64_t) {
    if (syncBuildFails) return syncBuildFails;
    uint8_t sync[27] = {}; sync[0]=6; sync[3]=6; sync[5]=16;
    ItlAWDL::LocalFrame f={{2,0,0,0,0,1},{255,255,255,255,255,255},3,0,12345678,12345678,
        sync,27,nullptr,0};
    size_t length;
    assert(ItlAWDL::buildLocalFrame(f,storage.bytes,sizeof(storage.bytes),&length)==ItlAWDL::BuildResult::Ready);
    storage.length=storage.pkthdr=length; *packet=&storage; return 0;
}
static size_t mbuf_pkthdr_len(mbuf_t p) { return p->pkthdr; }
static size_t mbuf_len(mbuf_t p) { return p->length; }
struct Loop { bool gated = true; bool inGate() { return gated; } };
class ItlIwx {
public:
    Loop loop; bool hasLoop = true;
    bool awdlMonitorTxArmed = true, awdlMonitorProbeStarted = false, awdlMonitorTxAttempted = false;
    unsigned awdlMonitorSubmitted = 0, macApplied = 0, macAppliedAtSubmission = 0;
    uint8_t macAddress[6] = {};
    void awdlApplyMacAddress(iwx_softc*, const uint8_t *a) { ++macApplied; macAppliedAtSubmission = submissions; memcpy(macAddress, a, 6); }
    int awdlMonitorGeneration = 1, awdlMonitorQueue = 1;
    void *awdlMonitorPending = nullptr;
    unsigned submissions = 0, awdlMonitorReadinessPolls = 0;
    void *getController() { return this; }
    Loop *getMainWorkLoop() { return hasLoop ? &loop : nullptr; }
    void iwx_awdl_transport_probe(iwx_softc*);
    bool refuseSubmission = false;
    unsigned burstSubmissions = 0;
    int iwx_tx_monitor_burst(iwx_softc*, mbuf_t p, Node*) {
        if (refuseSubmission) { p->frees++; return EACCES; }
        ++burstSubmissions; p->frees++; return 0;
    }
    int iwx_tx_monitor_frame(iwx_softc *sc, mbuf_t p, Node *node) {
        if (refuseSubmission) { awdlMonitorTxAttempted = true; p->frees++; return EACCES; }
        // Elicit frames are data, so the action-frame checks below don't apply; the test checks them.
        if (awdlElicit) { ++submissions; p->frees++; return 0; }
        assert(node == sc->sc_ic.ic_bss && p->length == (awdlSyncProbe ? 70U : 40U) && p->pkthdr == p->length);
        assert(awdlAckProbe || !memcmp(p->bytes+10, sc->sc_ic.ic_myaddr, 6));
        ItlAWDL::ActionFrame frame;
        assert(ItlAWDL::parseAction(p->bytes, p->length, &frame) == ItlAWDL::ParseResult::Valid);
        assert(frame.hasSync == awdlSyncProbe && frame.phyTxTime == 12345678);
        ++submissions; mbuf_freem(p); return 0;
    }
};
'''
program += body
program += r'''
int main() {
    for (unsigned mode = 0; mode < 22; ++mode) {
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss = &node; sc.sc_phyctxt[0].channel = &channel;
        sc.sc_ic.ic_chan_active[6] = true;
        awdlRx = true; awdlRxChannel6 = true;
        storage = {}; allocations = 0; failAllocation = false; capacity = 128;
        switch (mode) {
        case 0: awdlRx = false; break;
        case 1: awdlRxChannel6 = false; break;
        case 2: driver.awdlMonitorTxArmed = false; break;
        case 3: driver.loop.gated = false; break;
        case 4: driver.hasLoop = false; break;
        case 5: driver.awdlMonitorGeneration = 0; break;
        case 6: driver.awdlMonitorQueue = -1; break;
        case 7: driver.awdlMonitorPending = &driver; break;
        case 8: sc.sc_ic.ic_bss = nullptr; break;
        case 9: sc.sc_ic.ic_state = 0; break;
        case 10: sc.sc_flags = IWX_FLAG_MAC_ACTIVE; break;
        case 11: sc.sc_flags |= IWX_FLAG_HW_ERR; break;
        case 12: channel.number = 149; break;
        case 13: node.ni_rates.rs_nrates = 0; break;
        case 14: sc.sc_ic.ic_chan_active[6] = false; break;
        case 15: channel.ic_flags = IEEE80211_CHAN_PASSIVE; break;
        case 16: driver.awdlMonitorTxAttempted = true; break;
        case 17: sc.sc_ic.ic_myaddr[0] = 1; break;
        case 18: failAllocation = true; break;
        case 19: capacity = 39; break;
        default: break;
        }
        driver.iwx_awdl_transport_probe(&sc);
        if (mode < 17) {
            assert(!driver.awdlMonitorProbeStarted && !allocations && !driver.submissions);
        } else {
            assert(driver.awdlMonitorProbeStarted);
            assert(driver.submissions == (mode >= 20 ? 1U : 0U));
            assert(allocations == (mode == 17 ? 0U : 1U));
            assert(storage.frees == (mode >= 19 ? 1U : 0U));
            const auto calls = allocations;
            driver.iwx_awdl_transport_probe(&sc);
            ++sc.sc_generation; driver.awdlMonitorGeneration = sc.sc_generation;
            driver.iwx_awdl_transport_probe(&sc);
            assert(allocations == calls && driver.submissions <= 1);
        }
    }
    awdlRx = awdlRxChannel6 = awdlSyncProbe = true;
    for (unsigned fail=0; fail<2; ++fail) {
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        storage={}; syncBuildFails=fail;
        driver.iwx_awdl_transport_probe(&sc);
        assert(driver.awdlMonitorProbeStarted && driver.submissions == (fail ? 0U : 1U));
        assert(storage.frees == (fail ? 0U : 1U));
        driver.iwx_awdl_transport_probe(&sc);
        assert(driver.submissions == (fail ? 0U : 1U));
    }
    {
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        storage={}; syncBuildFails=kIOReturnNotReady;
        for (unsigned i=0;i<3;++i) driver.iwx_awdl_transport_probe(&sc);
        assert(!driver.awdlMonitorProbeStarted && driver.awdlMonitorReadinessPolls==3 && !driver.submissions);
        syncBuildFails=0; driver.iwx_awdl_transport_probe(&sc);
        assert(driver.awdlMonitorProbeStarted && driver.submissions==1 && storage.frees==1);
        driver.iwx_awdl_transport_probe(&sc); assert(driver.submissions==1);
    }
    {
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        storage={}; syncBuildFails=kIOReturnNotReady;
        for (unsigned i=0;i<30;++i) driver.iwx_awdl_transport_probe(&sc);
        assert(driver.awdlMonitorProbeStarted && driver.awdlMonitorReadinessPolls==20 && !driver.submissions && !storage.frees);
    }
    {
        // Peer-gated train: silent and not consumed until a peer is audible,
        // then records the address it transmitted with for echo rejection.
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        storage={}; syncBuildFails=0; awdlPeerTrain=true; peerAudible=false; samplesKept=0;
        memset(awdlOwnAddress,0,6);
        for (unsigned i=0;i<30;++i) driver.iwx_awdl_transport_probe(&sc);
        assert(!driver.awdlMonitorProbeStarted && !driver.awdlMonitorReadinessPolls && !driver.submissions);
        assert(!samplesKept && !awdlOwnAddress[0]);
        peerAudible=true; driver.iwx_awdl_transport_probe(&sc);
        assert(driver.awdlMonitorProbeStarted && driver.submissions==1 && samplesKept==1);
        assert(awdlOwnAddress[0]==2 && awdlOwnAddress[5]==1);
        awdlPeerTrain=false;
    }
    {
        // Probe frames change only the addresses, and the own address recorded
        // for echo rejection stays the AWDL one.
        using ItlAWDL::AckProbe;
        const uint8_t peer[6]={2,9,9,9,9,9},control[6]={2,0x11,0x22,0x33,0x44,0x55},awdl[6]={2,0,0,0,0,1};
        for (unsigned frame : {1U,5U,9U,13U,2U}) {
            ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
            sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
            storage={}; syncBuildFails=0; awdlAckProbe=true; awdlAckPending=AckProbe::None;
            memcpy(awdlLastPeer,peer,6); driver.awdlMonitorSubmitted=frame;
            memcpy(sc.sc_ic.ic_myaddr,"\x02\xaa\xbb\xcc\xdd\xee",6);
            driver.iwx_awdl_transport_probe(&sc);
            assert(driver.submissions==1 && !memcmp(awdlOwnAddress,awdl,6));
            const bool probe=frame%4==1;
            assert((awdlAckPending!=AckProbe::None)==probe);
            if(probe) assert(!memcmp(storage.bytes+4,frame<8?peer:control,6));
            else assert(storage.bytes[4]==255);
            const bool hw=frame==5||frame==13;
            assert(!memcmp(storage.bytes+10,hw?sc.sc_ic.ic_myaddr:awdl,6));
        }
        awdlAckProbe=false; awdlAckPending=AckProbe::None;
    }
    {
        // The address change happens once, before the first frame leaves, with
        // the AWDL address. Hardware-source probes still send the real address.
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        const uint8_t awdl[6]={2,0,0,0,0,1};
        storage={}; syncBuildFails=0; awdlAckProbe=true; awdlMacProbe=true; awdlMacTried=false;
        memcpy(sc.sc_ic.ic_myaddr,"\x02\xaa\xbb\xcc\xdd\xee",6);
        driver.iwx_awdl_transport_probe(&sc);
        assert(driver.macApplied==1 && !driver.macAppliedAtSubmission && driver.submissions==1);
        assert(!memcmp(driver.macAddress,awdl,6));
        driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; storage={};
        driver.iwx_awdl_transport_probe(&sc);
        assert(driver.macApplied==1 && driver.submissions==2);
        awdlAckProbe=awdlMacProbe=false; awdlAckPending=ItlAWDL::AckProbe::None;
    }
    {
        // Elicit slots become well-formed unicast data; a learned peer IP wins.
        using ItlAWDL::AckProbe;
        const uint8_t peer[6]={2,9,9,9,9,9},awdl[6]={2,0,0,0,0,1};
        for (unsigned frame : {5U,13U}) for (bool learned : {false,true}) {
            ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
            sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
            storage={}; syncBuildFails=0; awdlAckProbe=awdlElicit=true; awdlAckPending=AckProbe::None;
            memcpy(awdlLastPeer,peer,6); driver.awdlMonitorSubmitted=frame;
            memset(awdlLearnedPeer,0,6);
            uint8_t want[16]; ItlAWDL::linkLocalFromAddress(peer,want);
            if(learned) { memcpy(awdlLearnedPeer,peer,6); memset(awdlLearnedPeerIP,0x77,16); memset(want,0x77,16); }
            driver.iwx_awdl_transport_probe(&sc);
            assert(driver.submissions==1 && awdlAckPending==(frame==5?AckProbe::PeerNeighbor:AckProbe::PeerEcho));
            ItlAWDL::DataFrameView view;
            assert(ItlAWDL::parseDataFrame(storage.bytes,storage.length,false,&view)==ItlAWDL::ParseResult::Valid);
            assert(storage.length==storage.pkthdr && !memcmp(view.destination,peer,6) && !memcmp(view.source,awdl,6));
            assert(!memcmp(view.ip.bytes+24,want,16));
        }
        // Foreign control: a Neighbor Solicitation that claims an address we never ACK,
        // while the address recorded for echo rejection stays ours.
        {
            ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
            sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
            storage={}; awdlForeignControl=true; awdlAckPending=AckProbe::None;
            memcpy(awdlLastPeer,peer,6); driver.awdlMonitorSubmitted=1;
            driver.iwx_awdl_transport_probe(&sc);
            assert(awdlAckPending==AckProbe::PeerForeign && !memcmp(awdlOwnAddress,awdl,6));
            ItlAWDL::DataFrameView view;
            assert(ItlAWDL::parseDataFrame(storage.bytes,storage.length,false,&view)==ItlAWDL::ParseResult::Valid);
            assert(!memcmp(view.source,ItlAWDL::foreignAddress(),6) && !memcmp(view.destination,peer,6));
            assert(view.ip.bytes[40]==135 && !memcmp(view.ip.bytes+66,ItlAWDL::foreignAddress(),6));
            awdlForeignControl=false;
        }
        awdlAckProbe=awdlElicit=false; awdlAckPending=AckProbe::None;
    }
    {
        // Under continuous output a sync frame still goes when due, and then data resumes.
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        const bool savedSync=awdlSyncProbe;
        awdlDataTx=awdlElicit=awdlPeerTrain=peerAudible=awdlSyncProbe=true; transmitFramesReady=8; storage={};
        awdlDataSent[0]=awdlDataSent[1]=0; awdlDataInFlight=0; mockUptimeUS=500000; awdlLastSyncUS=0;
        driver.iwx_awdl_transport_probe(&sc);
        assert(!awdlDataInFlight && awdlLastSyncUS==500000 && transmitFramesReady==8);
        driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; driver.awdlMonitorPending=nullptr; storage={};
        mockUptimeUS=500200; driver.iwx_awdl_transport_probe(&sc);
        assert(awdlDataInFlight && transmitFramesReady==7);
        awdlSyncProbe=savedSync; awdlDataTx=awdlElicit=awdlPeerTrain=peerAudible=false; awdlDataInFlight=0; mockUptimeUS=awdlLastSyncUS=0; transmitFramesReady=0;
        awdlDataSent[0]=awdlDataSent[1]=0; storage={};
    }
    {
        // With nothing to send, sync frames keep their own pace however often the timer runs.
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        const bool savedSync=awdlSyncProbe;
        awdlDataTx=awdlElicit=awdlPeerTrain=peerAudible=awdlSyncProbe=true; transmitFramesReady=0; storage={};
        mockUptimeUS=700000; awdlLastSyncUS=0;
        driver.iwx_awdl_transport_probe(&sc);
        assert(awdlLastSyncUS==700000);
        for (uint64_t now : {uint64_t(700001), uint64_t(701000), uint64_t(744999)}) {
            driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; driver.awdlMonitorPending=nullptr; storage={};
            const unsigned before=driver.submissions;
            mockUptimeUS=now; driver.iwx_awdl_transport_probe(&sc);
            assert(awdlLastSyncUS==700000 && driver.submissions==before);
        }
        driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; driver.awdlMonitorPending=nullptr; storage={};
        mockUptimeUS=745000; driver.iwx_awdl_transport_probe(&sc);
        assert(awdlLastSyncUS==745000);
        awdlSyncProbe=savedSync; awdlDataTx=awdlElicit=awdlPeerTrain=peerAudible=false; mockUptimeUS=awdlLastSyncUS=0; storage={};
    }
    {
        // A held frame goes first, marked as a retry, backs off, and is given up in time.
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        const size_t savedCapacity=capacity; capacity=256;
        auto ready=[&]{ driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; driver.awdlMonitorPending=nullptr; awdlDataInFlight=0; storage={}; };
        awdlDataTx=awdlElicit=true; storage={}; awdlDataSent[0]=1; awdlDataSent[1]=0; awdlDataInFlight=0;
        awdlResend=ItlAWDL::ResendState{}; awdlResendSent=awdlResendGaveUp=0; mockUptimeUS=900000; transmitFramesReady=1;
        driver.iwx_awdl_transport_probe(&sc);
        assert(awdlDataInFlight==2 && awdlResend.held && !awdlResend.pending && awdlResendLength==80 && awdlResendFrame[0]==8);
        ready(); transmitFramesReady=1;
        assert(ItlAWDL::resendAfterFailure(awdlResend, mockUptimeUS));
        driver.iwx_awdl_transport_probe(&sc);
        assert(awdlResendSent==1 && (storage.pkthdr==80) && (storage.bytes[1]&8) && transmitFramesReady==1 && awdlDataInFlight==2 && !awdlResend.pending);
        ready();
        assert(ItlAWDL::resendAfterFailure(awdlResend, mockUptimeUS) && awdlResend.attempts==2);
        driver.iwx_awdl_transport_probe(&sc); // Waiting out the backoff, and nothing newer jumps ahead.
        assert(awdlResendSent==1 && transmitFramesReady==1 && !awdlDataInFlight);
        ready(); mockUptimeUS+=2000; driver.iwx_awdl_transport_probe(&sc);
        assert(awdlResendSent==2 && transmitFramesReady==1);
        ready();
        assert(ItlAWDL::resendAfterFailure(awdlResend, mockUptimeUS));
        mockUptimeUS=900000+ItlAWDL::kResendWindowUS; driver.iwx_awdl_transport_probe(&sc); // Too old, so the next frame goes.
        assert(awdlResendGaveUp==1 && awdlResendSent==2 && !transmitFramesReady && awdlDataInFlight==2 && awdlResend.held && !awdlResend.attempts);
        // Allocation failure gives up, without leaving the gate closed.
        ready();
        assert(ItlAWDL::resendAfterFailure(awdlResend, mockUptimeUS));
        failAllocation=true; driver.iwx_awdl_transport_probe(&sc); failAllocation=false;
        assert(awdlResendGaveUp==2 && !awdlResend.held && !driver.awdlMonitorProbeStarted && !awdlDataInFlight);
        // A refused resend gives up too, and the gate reopens.
        ready(); awdlResend=ItlAWDL::ResendState{}; awdlResend.held=true; awdlResend.firstUS=mockUptimeUS;
        assert(ItlAWDL::resendAfterFailure(awdlResend, mockUptimeUS));
        driver.refuseSubmission=true; driver.iwx_awdl_transport_probe(&sc); driver.refuseSubmission=false;
        assert(awdlResendGaveUp==3 && !awdlResend.held && !driver.awdlMonitorProbeStarted && !awdlDataInFlight);
        capacity=savedCapacity; awdlDataTx=awdlElicit=false; awdlDataSent[0]=awdlDataSent[1]=0; awdlResend=ItlAWDL::ResendState{}; storage={}; mockUptimeUS=awdlLastSyncUS=0; transmitFramesReady=0;
    }
    {
        // With a window: frames go up to its size, and a failure stops new ones until resent.
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        const size_t savedCapacity=capacity; capacity=256;
        awdlResendSent=0; awdlDataTx=awdlElicit=true; awdlWindow=2; awdlBurst.reset(); awdlResend=ItlAWDL::ResendState{};
        awdlDeferredMulticast=nullptr; awdlDataSent[0]=1; awdlDataSent[1]=0; storage={}; mockUptimeUS=900000; transmitFramesReady=3;
        driver.iwx_awdl_transport_probe(&sc);
        assert(driver.burstSubmissions==2 && awdlBurst.count==2 && transmitFramesReady==1 && !driver.awdlMonitorProbeStarted && !driver.awdlMonitorPending);
        driver.iwx_awdl_transport_probe(&sc); // Window full.
        assert(driver.burstSubmissions==2 && transmitFramesReady==1);
        awdlBurst.complete(true);
        driver.iwx_awdl_transport_probe(&sc); // Room for one more.
        assert(driver.burstSubmissions==3 && awdlBurst.count==2 && !transmitFramesReady);
        awdlBurst.complete(false); transmitFramesReady=1;
        driver.iwx_awdl_transport_probe(&sc); // One failed and one in flight: nothing new.
        assert(driver.burstSubmissions==3 && transmitFramesReady==1 && !awdlResendSent);
        awdlBurst.complete(true); storage={}; awdlResendSent=0;
        driver.iwx_awdl_transport_probe(&sc); // Now the failed frame, marked as a retry, in the window's own path.
        assert(awdlResendSent==1 && (storage.bytes[1]&8) && !awdlDataInFlight && transmitFramesReady==1 && !driver.awdlMonitorProbeStarted);
        assert(awdlBurst.count==1 && awdlBurst.front()->isRetry && (awdlBurst.front()->bytes[1]&8) && driver.burstSubmissions==4 && awdlResend.held && !awdlResend.pending);
        driver.iwx_awdl_transport_probe(&sc); // In flight, so nothing else goes.
        assert(driver.burstSubmissions==4 && transmitFramesReady==1);
        // A failed resend waits its backoff, and a sync frame due meanwhile isn't held back.
        awdlBurst.complete(false); assert(!awdlBurst.retryCount);
        assert(ItlAWDL::resendAfterFailure(awdlResend, mockUptimeUS) && awdlResend.attempts==2);
        driver.iwx_awdl_transport_probe(&sc);
        assert(driver.burstSubmissions==4 && transmitFramesReady==1 && !awdlBurst.count);
        // Group frames wait for the window, then go alone.
        driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; awdlDataInFlight=0; awdlResend=ItlAWDL::ResendState{}; awdlBurst.reset(); storage={};
        awdlDataSent[0]=awdlDataSent[1]=0; transmitFramesReady=1;
        driver.iwx_awdl_transport_probe(&sc);
        assert(awdlDataSent[0]==1 && awdlDataInFlight==1 && driver.burstSubmissions==4);
        // A group frame set to repeat goes again 64 TU later, without counting as a new frame.
        driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; awdlDataInFlight=0; storage={};
        awdlMulticastCopies=3; awdlMulticastRepeats.reset(); awdlMulticastRepeatsSent=0; awdlDataSent[0]=awdlDataSent[1]=0; transmitFramesReady=1;
        driver.iwx_awdl_transport_probe(&sc);
        assert(awdlDataSent[0]==1 && awdlDataInFlight==1 && !awdlMulticastRepeatsSent);
        driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; awdlDataInFlight=0; storage={};
        const uint64_t savedNow=mockUptimeUS; mockUptimeUS+=ItlAWDL::kMulticastRepeatUS;
        driver.iwx_awdl_transport_probe(&sc);
        assert(awdlMulticastRepeatsSent==1 && awdlDataSent[0]==1 && awdlDataInFlight==1 && storage.pkthdr==80);
        // Not while unicast frames are in flight.
        driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; awdlDataInFlight=0; storage={};
        mockUptimeUS+=ItlAWDL::kMulticastRepeatUS; const uint8_t oneFrame[4]={8,0,0,0}; assert(awdlBurst.push(oneFrame,4,mockUptimeUS,1));
        driver.iwx_awdl_transport_probe(&sc);
        assert(awdlMulticastRepeatsSent==1);
        awdlBurst.reset(); awdlMulticastCopies=1; awdlMulticastRepeats.reset(); mockUptimeUS=savedNow;
        // Nothing goes while the radio is away scanning.
        driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; awdlDataInFlight=0; storage={};
        awdlBackgroundScanActive=true; const unsigned sentBefore=driver.burstSubmissions; transmitFramesReady=1; awdlDataSent[0]=1;
        driver.iwx_awdl_transport_probe(&sc);
        assert(driver.burstSubmissions==sentBefore && transmitFramesReady==1 && !awdlBackgroundScanActive==false);
        awdlBackgroundScanActive=false; transmitFramesReady=0;
        // A refused frame leaves nothing behind.
        driver.awdlMonitorProbeStarted=driver.awdlMonitorTxAttempted=false; awdlDataInFlight=0; storage={}; awdlDataSent[0]=1;
        transmitFramesReady=1; driver.refuseSubmission=true; awdlResendGaveUp=0;
        driver.iwx_awdl_transport_probe(&sc); driver.refuseSubmission=false;
        assert(!awdlBurst.count && awdlResendGaveUp==1 && !transmitFramesReady && !awdlDataInFlight);
        capacity=savedCapacity; awdlWindow=1; awdlBurst.reset(); awdlDataTx=awdlElicit=false; awdlDataSent[0]=awdlDataSent[1]=0; awdlResend=ItlAWDL::ResendState{}; storage={}; mockUptimeUS=awdlLastSyncUS=0; transmitFramesReady=0;
    }
    {
        // A refused data frame has no completion, so it must not close the gate for good.
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        awdlDataTx=true; transmitFramesReady=1; storage={}; driver.refuseSubmission=true;
        driver.iwx_awdl_transport_probe(&sc);
        assert(!driver.awdlMonitorProbeStarted && !driver.awdlMonitorTxAttempted && !awdlDataInFlight);
        awdlDataTx=false; awdlDataSent[0]=awdlDataSent[1]=0; storage={};
    }
    {
        // With data transmit, readiness outlasts the fast timer: 100 polls don't close it.
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        awdlDataTx=awdlElicit=true; transmitFramesReady=0; storage={}; syncBuildFails=kIOReturnNotReady;
        for (unsigned i=0;i<100;++i) driver.iwx_awdl_transport_probe(&sc);
        assert(!driver.awdlMonitorProbeStarted && driver.awdlMonitorReadinessPolls==100 && !driver.submissions);
        syncBuildFails=0; driver.iwx_awdl_transport_probe(&sc);
        assert(driver.awdlMonitorProbeStarted && driver.submissions==1);
        // Bounded: it still gives up eventually, on the larger limit.
        ItlIwx later; later.awdlMonitorReadinessPolls=2398; syncBuildFails=kIOReturnNotReady;
        later.iwx_awdl_transport_probe(&sc); assert(!later.awdlMonitorProbeStarted);
        later.iwx_awdl_transport_probe(&sc); assert(later.awdlMonitorProbeStarted);
        awdlDataTx=awdlElicit=false; syncBuildFails=0; storage={};
    }
    {
        // macOS's output goes before any sync frame, even with no peer audible, and it
        // is classified by destination. With nothing queued, the train's rules apply.
        ItlIwx driver; iwx_softc sc; Channel channel; Node node{&channel};
        sc.sc_ic.ic_bss=&node; sc.sc_phyctxt[0].channel=&channel; sc.sc_ic.ic_chan_active[6]=true;
        awdlDataTx=awdlElicit=awdlPeerTrain=true; peerAudible=false; transmitFramesReady=2; storage={};
        driver.iwx_awdl_transport_probe(&sc);
        assert(driver.submissions==1 && awdlDataInFlight==1 && awdlDataSent[0]==1 && driver.awdlMonitorProbeStarted);
        driver.awdlMonitorProbeStarted=false;
        driver.iwx_awdl_transport_probe(&sc);
        assert(driver.submissions==2 && awdlDataInFlight==2 && awdlDataSent[1]==1);
        driver.awdlMonitorProbeStarted=false;awdlDataInFlight=0;
        driver.iwx_awdl_transport_probe(&sc);
        assert(driver.submissions==2 && !driver.awdlMonitorProbeStarted && !awdlDataInFlight);
        awdlDataTx=awdlElicit=awdlPeerTrain=false;
    }
    puts("single transport-probe caller gating/ownership: passed (host mocks only)");
}
'''
with tempfile.TemporaryDirectory(prefix="forbiddenitlwm-probe-") as directory:
    src = Path(directory) / "test.cpp"
    src.write_text(program)
    output = Path(directory) / "test"
    subprocess.run(["xcrun", "clang++", "-std=c++14", "-DAIRPORT_WCL", "-D__IO80211_TARGET=__MAC_26_0", "-I" + str(root / "include"),
                    "-fsanitize=address,undefined", str(src), "-o", str(output)], check=True)
    subprocess.run([str(output)], check=True)
