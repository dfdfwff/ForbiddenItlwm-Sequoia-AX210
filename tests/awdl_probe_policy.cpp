#include <AWDLProbePolicy.hpp>
#include <cassert>
#include <cstdio>
static void testDataPath() {
    using namespace ItlAWDL;
    DataRateState s;
    assert(s.step == 1 && dataRateMbps(s.step) == 24);
    // A clean run of 64 steps up, one step at a time, and stops at the top.
    for (unsigned i = 0; i < 63; ++i) noteDataResult(s, true);
    assert(s.step == 1);
    noteDataResult(s, true);
    assert(s.step == 2 && !s.frames);
    for (unsigned i = 0; i < 64 * 4; ++i) noteDataResult(s, true);
    assert(s.step == kLegacyRateTop && dataRateMbps(s.step) == 54);
    // One failure in a run, as a peer's absence causes, doesn't step down.
    noteDataResult(s, false);
    for (unsigned i = 0; i < 63; ++i) noteDataResult(s, true);
    assert(s.step == 3 && !s.frames);
    // Eight in one run does, and never below the lowest step.
    for (unsigned i = 0; i < 8; ++i) noteDataResult(s, false);
    assert(s.step == 2);
    for (unsigned i = 0; i < 64; ++i) noteDataResult(s, false);
    assert(s.step == 0 && dataRateMbps(s.step) == 12);
    // Two failures in a run holds the rate.
    for (unsigned i = 0; i < 2; ++i) noteDataResult(s, false);
    for (unsigned i = 0; i < 62; ++i) noteDataResult(s, true);
    assert(s.step == 0);
    // HT steps: the rates and MCS indexes line up, and legacy has none.
    assert(htMcsForStep(kLegacyRateTop) == -1 && htMcsForStep(4) == 7 && dataRateMbps(4) == 65);
    assert(htMcsForStep(5) == 12 && dataRateMbps(5) == 78 && htMcsForStep(8) == 15 && dataRateMbps(8) == 130);
    {
        const auto clean = [](DataRateState &d, unsigned windows) {
            for (unsigned i = 0; i < 64 * windows; ++i) noteDataResult(d, true);
        };
        const auto fail = [](DataRateState &d) {
            for (unsigned i = 0; i < 8; ++i) noteDataResult(d, false);
        };
        DataRateState h;
        h.top = kDataRateSteps - 1;
        clean(h, 7);
        assert(h.step == 8);
        clean(h, 3);
        assert(h.step == 8);   // Stops at the top.
        // A rate the peer can't take: each retry of it waits longer, up to the limit.
        h = DataRateState{};
        h.top = kDataRateSteps - 1;
        h.step = 5;
        fail(h);
        assert(h.step == 4 && h.holdWindows == 1 && h.holdLength == 2);
        clean(h, 1);
        assert(h.step == 5);
        fail(h);
        assert(h.step == 4 && h.holdWindows == 2);
        clean(h, 1);
        assert(h.step == 4);
        clean(h, 1);
        assert(h.step == 5);
        for (unsigned n = 0; n < 8; ++n) {
            fail(h);
            const unsigned wait = h.holdWindows;
            assert(h.step == 4 && wait);
            clean(h, wait - 1);
            assert(h.step == 4);
            clean(h, 1);
            assert(h.step == 5);
        }
        assert(h.holdLength == kHoldWindowsMax);
        // A clean window at that rate means it works, so the wait resets.
        clean(h, 1);
        assert(h.holdLength == 1 && h.failedStep == kDataRateSteps);
        // Legacy steps never wait, even with a hold pending.
        h = DataRateState{};
        h.top = kDataRateSteps - 1;
        h.step = 4;
        fail(h);
        h.holdWindows = 5;
        fail(h);
        assert(h.step == 2);
        clean(h, 1);
        assert(h.step == 3);
        assert(h.holdWindows >= 3);
        clean(h, 1);
        assert(h.step == 3);   // Entering HT does wait.
    }
    // Resend: not held means no resend.
    ResendState r;
    assert(!resendAfterFailure(r, 1000) && !r.pending);
    // Held: tries at once, then backs off.
    r.held = true; r.firstUS = 1000;
    assert(resendAfterFailure(r, 1000) && r.pending && r.attempts == 1 && r.notBeforeUS == 1000);
    assert(resendAfterFailure(r, 2000) && r.attempts == 2 && r.notBeforeUS == 4000);
    assert(resendAfterFailure(r, 5000) && r.attempts == 3 && r.notBeforeUS == 10000);
    // Past the window, or with a clock going backwards, it's given up and clear.
    assert(!resendExpired(r, 1000 + kResendWindowUS - 1) && resendExpired(r, 1000 + kResendWindowUS) && resendExpired(r, 999));
    assert(!resendAfterFailure(r, 1000 + kResendWindowUS) && !r.held && !r.pending && !r.attempts);
    // And after too many tries, even inside the window.
    r = ResendState{}; r.held = true; r.firstUS = 1000; r.attempts = kResendMaxAttempts;
    assert(!resendAfterFailure(r, 1001) && !r.held);
    // Idle sync pacing.
    assert(!syncIdleDue(1000 + kSyncIdleIntervalUS - 1, 1000) && syncIdleDue(1000 + kSyncIdleIntervalUS, 1000));
    assert(syncIdleDue(5, 10));
    // The active window for fast polling.
    assert(!dataRecentlyActive(false, 5, 5));
    assert(dataRecentlyActive(true, 999999, 0) && !dataRecentlyActive(true, 1000000, 0));
    assert(!dataRecentlyActive(true, 5, 10));
    assert(!syncBeforeData(false, 1000000, 0));
    assert(!syncBeforeData(true, 112639, 0) && syncBeforeData(true, 112640, 0));
    assert(!syncBeforeData(true, 5, 10));
    assert(monitorQueueShouldAllocate(false, -1, -1, 1));
    assert(!monitorQueueShouldAllocate(true, 3, 1, 1));
    assert(monitorQueueShouldAllocate(true, 3, 1, 2));
    assert(!monitorQueueShouldAllocate(true, -1, 1, 2)); // A failure is not retried.
}
int main() {
    testDataPath();
    using namespace ItlAWDL;
    assert(fixedChannelFor(false, true, false) == 6);
    assert(fixedChannelFor(true, true, false) == 149);
    assert(fixedChannelFor(true, true, true) == 6); // Passive stays off limits.
    assert(fixedChannelFor(true, false, false) == 6);
    assert(btCoexModeFor(1, 3) == 1 && btCoexModeFor(2, 3) == 2 && btCoexModeFor(3, 1) == 3);
    assert(btCoexModeFor(0, 3) == 3 && btCoexModeFor(4, 3) == 3 && btCoexModeFor(-1, 3) == 3);
    assert(validCountry("FR") && validCountry("ZZ"));
    assert(!validCountry("in") && !validCountry("I") && !validCountry("IND") && !validCountry(nullptr) && !validCountry(""));
    using namespace ItlAWDL;
    assert(probeLimit(false)==8 && probeLimit(true)==64);
    assert(probeDelayUS(true,0,false,false)==6000000);
    assert(probeDelayUS(false,1,false,false)==6000000);
    for(unsigned n=1;n<64;++n) {
        assert(probeDelayUS(true,n,false,false)==112640);
        assert(probeDelayUS(true,n,true,true)==112640);
        assert(probeDelayUS(true,n,false,true)==6000000);
    }
    assert(probeDelayUS(true,64,false,false)==6000000);
    assert(probeDelayUS(true,65,true,true)==6000000);
    puts("bounded probe cadence policy: passed");
}
