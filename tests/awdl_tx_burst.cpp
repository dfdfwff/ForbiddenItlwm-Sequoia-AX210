#include <AWDLTxBurst.hpp>
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
    using namespace ItlAWDL;
    static TxBurst b;
    b.reset();
    assert(txWindowFor(0) == 1 && txWindowFor(-3) == 1 && txWindowFor(4) == 4 && txWindowFor(99) == 8);
    uint8_t f[4] = {1, 0, 0, 0};
    assert(b.canSend(1) && !b.front());
    // The window limits what's in flight, and a window over capacity is clamped.
    assert(b.push(f, 4, 100, 3));
    assert(b.front()->submitUS == 100 && b.front()->step == 3);
    assert(!b.canSend(1) && b.canSend(2));
    f[0] = 2; assert(b.push(f, 4, 101, 3));
    b.markNewestWideRate();
    f[0] = 3; assert(b.push(f, 4, 102, 2));
    assert(!b.canSend(3) && b.canSend(4));
    // Completion follows send order.
    assert(b.front()->bytes[0] == 1 && !b.front()->requestedWideRate);
    b.complete(true);
    assert(b.count == 2 && b.front()->bytes[0] == 2 && b.front()->requestedWideRate && !b.retryCount && b.canSend(3));
    // A failure stops new frames, and retries wait until those in flight finish.
    b.complete(false);
    assert(b.retryCount == 1 && !b.canSend(4) && !b.readyForRetry());
    uint8_t out[8]; size_t n = 0;
    assert(!b.takeRetry(out, 8, &n));
    b.complete(false);
    assert(b.readyForRetry() && b.retryCount == 2 && !b.count);
    // Resent oldest first, and new frames resume only when none are left.
    assert(!b.takeRetry(out, 3, &n));
    assert(b.takeRetry(out, 8, &n) && n == 4 && out[0] == 2);
    assert(!b.canSend(4));
    assert(b.takeRetry(out, 8, &n) && out[0] == 3);
    assert(!b.readyForRetry() && b.canSend(4) && !b.takeRetry(out, 8, &n));
    // Bad input, a refused frame, and an empty completion.
    assert(!b.push(nullptr, 4, 0, 0) && !b.push(f, 0, 0, 0));
    static uint8_t big[kTxFrameBytes + 1];
    assert(!b.push(big, sizeof(big), 0, 0) && b.push(big, kTxFrameBytes, 0, 0));
    b.dropNewest(); assert(!b.count);
    b.dropNewest(); assert(!b.count);
    b.complete(false); assert(!b.retryCount);
    // The ring wraps over many frames without losing order, up to capacity.
    for (unsigned i = 0; i < 100; ++i) {
        f[0] = uint8_t(i);
        assert(b.push(f, 4, i, 0));
        assert(b.front()->bytes[0] == uint8_t(i) && !b.front()->requestedWideRate);
        b.complete(true);
    }
    for (unsigned i = 0; i < kTxBurstCapacity; ++i) { f[0] = uint8_t(i); assert(b.push(f, 4, 0, 0)); }
    assert(!b.push(f, 4, 0, 0) && !b.canSend(99));
    for (unsigned i = 0; i < kTxBurstCapacity; ++i) { assert(b.front()->bytes[0] == i); b.complete(false); }
    assert(b.retryCount == kTxBurstCapacity);
    for (unsigned i = 0; i < kTxBurstCapacity; ++i) { assert(b.takeRetry(out, 8, &n) && out[0] == i); }
    b.reset(); assert(!b.count && !b.retryCount);
    // A resend that fails doesn't return to the retry list; the caller decides.
    assert(b.push(f, 4, 0, 0, true) && b.front()->isRetry);
    b.complete(false);
    assert(!b.count && !b.retryCount);
    // Stall detection.
    assert(!txStalled(false, kTxStallUS * 9, 1, 0));
    assert(!txStalled(true, kTxStallUS * 9, 0, 0));
    assert(!txStalled(true, 1 + kTxStallUS - 1, 1, 0) && txStalled(true, 1 + kTxStallUS, 1, 0));
    assert(!txStalled(true, 1000 + kTxStallUS - 1, 1, 1000) && txStalled(true, 1000 + kTxStallUS, 1, 1000));
    assert(!txStalled(true, 5, 10, 0));
    // Spaced copies of group frames.
    assert(multicastRepeatsFor(0) == 1 && multicastRepeatsFor(16) == 16 && multicastRepeatsFor(99) == 32);
    static MulticastRepeats m;
    m.reset();
    uint8_t g[3] = {9, 8, 7};
    assert(!m.due(1000000));
    m.add(g, 3, 1000, 1); assert(!m.due(1000000)); // One copy in total means no repeats.
    m.add(nullptr, 3, 1000, 3); m.add(g, 0, 1000, 3); m.add(g, kTxFrameBytes + 1, 1000, 3); assert(!m.due(1000000));
    m.add(g, 3, 1000, 3);
    assert(!m.due(1000 + kMulticastRepeatUS - 1));
    auto *d = m.due(1000 + kMulticastRepeatUS);
    assert(d && d->left == 2 && d->length == 3 && d->bytes[2] == 7);
    MulticastRepeats::sent(*d, 200000);
    assert(!m.due(200000 + kMulticastRepeatUS - 1));
    d = m.due(200000 + kMulticastRepeatUS);
    assert(d && d->left == 1);
    MulticastRepeats::sent(*d, 300000);
    assert(!m.due(99999999)); // All copies sent.
    // The oldest due goes first, and a full table overwrites the next slot, counted.
    m.reset();
    g[0] = 1; m.add(g, 3, 100, 2); g[0] = 2; m.add(g, 3, 50, 2);
    assert(m.due(9999999)->bytes[0] == 2);
    g[0] = 3; m.add(g, 3, 0, 2); g[0] = 4; m.add(g, 3, 0, 2);
    assert(!m.overwritten);
    g[0] = 5; m.add(g, 3, 0, 2);
    assert(m.overwritten == 1);
    m.reset(); assert(!m.due(99999999));
    puts("AWDL transmit window: passed (offline only)");
}
