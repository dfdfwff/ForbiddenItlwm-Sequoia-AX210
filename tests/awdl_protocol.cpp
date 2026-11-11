#include <stddef.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <vector>
#include <Airport/apple80211_ioctl.h>
#include <AWDLFrameParser.hpp>
#include <AWDLManagementFrame.hpp>
#include "../ForbiddenItlwm/AWDLInactiveProtocol.hpp"

static void checkInactiveState()
{
    struct Guarded {
        uint32_t before;
        apple80211_awdl_sync_state state;
        uint32_t after;
    } guarded;
    memset(&guarded, 0xa5, sizeof(guarded));
    assert(itlAWDLGetInactiveState(&guarded.state) == kIOReturnSuccess);
    assert(guarded.before == 0xa5a5a5a5 && guarded.after == 0xa5a5a5a5);
    assert(guarded.state.version == 1 && guarded.state.state == 1);
    assert(guarded.state.flags == 0 && guarded.state.driver_flags == 0);
    assert(itlAWDLGetInactiveState(nullptr) == kIOReturnBadArgument);
    assert(itlAWDLSetInactiveState(nullptr) == kIOReturnBadArgument);
    {
        // Off: identical to the inactive behaviour. On: requests are recorded and read back.
        apple80211_awdl_sync_enabled on = {};on.version = 1;on.enabled = 1;
        apple80211_awdl_sync_enabled off = {};off.version = 1;
        apple80211_awdl_sync_enabled bad = {};bad.version = 1;bad.enabled = 2;
        apple80211_awdl_sync_enabled later = {};later.version = APPLE80211_VERSION + 1;
        apple80211_awdl_sync_enabled reply;
        apple80211_awdl_sync_state s = {};s.version = 1;
        apple80211_awdl_sync_state sr;
        ItlAWDLNativeState n;
        assert(n.setEnabled(&on) == kIOReturnUnsupported);
        assert(n.getEnabled(&reply) == kIOReturnSuccess && !reply.enabled && reply.version == APPLE80211_VERSION);
        assert(n.setEnabled(&off) == kIOReturnSuccess);
        for (uint32_t state : {2U, 3U}) { s.state = state; assert(n.setState(&s) == kIOReturnUnsupported); }
        assert(n.getState(&sr) == kIOReturnSuccess && sr.state == 1);
        s.state = 1;assert(n.setState(&s) == kIOReturnSuccess);
        n.accept = true;
        assert(n.setEnabled(&on) == kIOReturnSuccess);
        assert(n.getEnabled(&reply) == kIOReturnSuccess && reply.enabled == 1 && !reply.unk1);
        assert(n.setEnabled(&bad) == kIOReturnBadArgument && n.setEnabled(&later) == kIOReturnBadArgument);
        assert(n.setEnabled(nullptr) == kIOReturnBadArgument && n.getEnabled(nullptr) == kIOReturnBadArgument);
        assert(n.getEnabled(&reply) == kIOReturnSuccess && reply.enabled == 1);
        assert(n.setEnabled(&off) == kIOReturnSuccess && n.getEnabled(&reply) == kIOReturnSuccess && !reply.enabled);
        for (uint32_t state : {2U, 3U, 1U}) {
            s.state = state;
            assert(n.setState(&s) == kIOReturnSuccess);
            assert(n.getState(&sr) == kIOReturnSuccess && sr.state == state);
        }
        s.state = 4;assert(n.setState(&s) == kIOReturnBadArgument);
        s.state = 0;assert(n.setState(&s) == kIOReturnBadArgument);
        assert(n.setState(nullptr) == kIOReturnBadArgument && n.getState(nullptr) == kIOReturnBadArgument);
        assert(n.getState(&sr) == kIOReturnSuccess && sr.state == 1);
    }
    for (uint32_t version = 0; version <= 2; ++version) {
        guarded.state.version = version;
        for (uint32_t state = 0; state <= 4; ++state) {
            guarded.state.state = state;
            const IOReturn expected = version > 1 || state < 1 || state > 3 ?
                kIOReturnBadArgument : state == 1 ? kIOReturnSuccess : kIOReturnUnsupported;
            assert(itlAWDLSetInactiveState(&guarded.state) == expected);
        }
    }
}

static void checkTemplate()
{
    ItlAWDLFrameTemplate frame;
    std::vector<uint8_t> source(ItlAWDLFrameTemplate::capacity, 0x5a);
    std::vector<uint8_t> target(source.size()+2, 0xa5);
    assert(frame.size() == 0);
    assert(frame.set(source.data(), source.size()) == kIOReturnSuccess);
    source.assign(source.size(), 0);
    assert(frame.copy(target.data()+1, frame.size()) == kIOReturnSuccess);
    assert(target.front() == 0xa5 && target.back() == 0xa5);
    for (size_t i = 1; i+1 < target.size(); ++i)
        assert(target[i] == 0x5a);
    assert(frame.set(nullptr, 1) == kIOReturnBadArgument);
    assert(frame.set(source.data(), size_t(-1)) == kIOReturnBadArgument);
    assert(frame.size() == ItlAWDLFrameTemplate::capacity);
    target.assign(target.size(), 0xa5);
    assert(frame.copy(target.data(), frame.size()-1) == kIOReturnNoSpace);
    for (auto byte : target)
        assert(byte == 0xa5);
    assert(frame.copy(nullptr, frame.size()) == kIOReturnNoSpace);
    assert(frame.set(nullptr, 0) == kIOReturnSuccess);
    assert(frame.size() == 0 && frame.copy(nullptr, 0) == kIOReturnSuccess);
}

static std::vector<uint8_t> syncFrame()
{
    std::vector<uint8_t> frame(40+3+27, 0);
    frame[0] = 0xd0;
    const uint8_t bssid[6] = {0, 0x25, 0, 0xff, 0x94, 0x73};
    memcpy(frame.data()+16, bssid, 6);
    const uint8_t action[8] = {0x7f, 0, 0x17, 0xf2, 8, 0x10, 0, 0};
    memcpy(frame.data()+24, action, 8);
    frame[32] = 0x78; frame[33] = 0x56; frame[34] = 0x34; frame[35] = 0x12;
    frame[36] = 0x10; frame[37] = 0x32; frame[38] = 0x54; frame[39] = 0x76;
    frame[40] = 4; frame[41] = 27;
    auto *p = frame.data()+43;
    p[0] = 44; p[1] = 0x34; p[2] = 0x12; p[3] = 149;
    p[5] = 16; p[7] = 110; p[15] = 7; p[21] = 0x42;
    return frame;
}

static void checkParser()
{
    using namespace ItlAWDL;
    auto frame = syncFrame();
    ActionFrame decoded;
    assert(parseAction(frame.data(), frame.size(), &decoded) == ParseResult::Valid);
    assert(decoded.hasSync && decoded.masterChannel == 149 && decoded.nextChannel == 44);
    assert(decoded.awPeriod == 16 && decoded.afPeriod == 110 && decoded.remainingAW == 7);
    assert(decoded.txDownCounter == 0x1234 && decoded.masterAddress[0] == 0x42);
    assert(decoded.phyTxTime == 0x12345678 && decoded.targetTxTime == 0x76543210);
    // Intel monitor mode appends FCS. Do not decode it as a trailing TLV or
    // include it in the action body delivered to the native receiver.
    auto captured = frame;
    captured.insert(captured.end(), {0x12, 0x34, 0x56, 0x78});
    const auto originalCapture = captured;
    size_t frameLength = 0;
    assert(parseAction(captured.data(), captured.size(), &decoded) == ParseResult::Malformed);
    assert(parseCapturedAction(captured.data(), captured.size(), true, &decoded, &frameLength) == ParseResult::Valid);
    assert(frameLength == frame.size() && captured == originalCapture);
    assert(decoded.hasSync && decoded.awPeriod == 16);
    assert(parseCapturedAction(frame.data(), frame.size(), false, &decoded, &frameLength) == ParseResult::Valid);
    assert(frameLength == frame.size());
    frameLength = 0xa5;
    assert(parseCapturedAction(captured.data(), 3, true, &decoded, &frameLength) == ParseResult::Malformed);
    assert(frameLength == 0xa5);
    // Explicit format flag is required: no heuristic accepts appended junk.
    assert(parseCapturedAction(captured.data(), captured.size(), false, &decoded, &frameLength) == ParseResult::Malformed);
    assert(frameLength == 0xa5);
    for (size_t n = 0; n < frame.size(); ++n) {
        ActionFrame output;
        memset(&output, 0xa5, sizeof(output));
        const auto untouched = output;
        const auto result = parseAction(frame.data(), n, &output);
        // A header without any TLVs is a complete, valid action frame.
        if (n == 40)
            assert(result == ParseResult::Valid && !output.hasSync);
        else {
            assert(result != ParseResult::Valid);
            assert(memcmp(&output, &untouched, sizeof(output)) == 0);
        }
    }
    auto bad = frame;
    bad[41] = bad[42] = 0xff;
    assert(parseAction(bad.data(), bad.size(), &decoded) == ParseResult::Malformed);
    bad = frame; bad[29] = 0xff;
    assert(parseAction(bad.data(), bad.size(), &decoded) == ParseResult::Unsupported);
    bad = frame; bad[1] = 0x40;
    assert(parseAction(bad.data(), bad.size(), &decoded) == ParseResult::Unsupported);
    bad = frame; bad[22] = 1;
    assert(parseAction(bad.data(), bad.size(), &decoded) == ParseResult::Unsupported);
    bad = frame; bad.insert(bad.end(), frame.begin()+40, frame.end());
    assert(parseAction(bad.data(), bad.size(), &decoded) == ParseResult::Malformed);
    bad = frame; bad.insert(bad.end(), {0xfe, 0, 0});
    assert(parseAction(bad.data(), bad.size(), &decoded) == ParseResult::Valid);

    // Deterministic fuzzing also covers unaligned input and all buffer lengths.
    uint32_t random = 0x87123456;
    for (unsigned trial = 0; trial < 25000; ++trial) {
        auto next = [&random]() { random ^= random << 13; random ^= random >> 17;
                                 random ^= random << 5; return random; };
        const size_t n = next()%97;
        std::vector<uint8_t> bytes(n+1);
        for (size_t i = 0; i < n; ++i)
            bytes[i+1] = i < frame.size() && (trial & 1) ? frame[i] : uint8_t(next());
        if (n)
            bytes[1+next()%n] ^= uint8_t(next());
        ActionFrame output;
        memset(&output, 0xa5, sizeof(output));
        const auto untouched = output;
        const auto result = parseAction(bytes.data()+1, n, &output);
        if (result != ParseResult::Valid)
            assert(memcmp(&output, &untouched, sizeof(output)) == 0);
        // The padding trim never grows a frame, removes only zero bytes, and only returns a shorter
        // length that parses.
        const size_t trimmed = trimZeroPadding(bytes.data()+1, n);
        assert(trimmed <= n);
        if (trimmed != n) {
            assert(trimmed > 40 && parseAction(bytes.data()+1, trimmed, &output) == ParseResult::Valid);
            for (size_t i = trimmed; i < n; ++i)
                assert(bytes[1+i] == 0);
        }
    }
}

// Issue #6: listener frames with zero padding after the last TLV.
static void checkZeroPadding()
{
    using namespace ItlAWDL;
    ActionFrame decoded;
    const auto frame = syncFrame();
    // As reported: 8 zero bytes after the last TLV. Malformed as given, cut back to the frame.
    auto padded = frame;
    padded.insert(padded.end(), 8, 0);
    assert(parseAction(padded.data(), padded.size(), &decoded) == ParseResult::Malformed);
    assert(trimZeroPadding(padded.data(), padded.size()) == frame.size());
    assert(parseAction(padded.data(), frame.size(), &decoded) == ParseResult::Valid);
    assert(decoded.hasSync && decoded.masterChannel == 149 && decoded.awPeriod == 16);
    // Any padding length: if it already parses (whole empty TLVs), it's left alone; otherwise it's cut.
    for (size_t pad = 1; pad <= 32; ++pad) {
        auto f = frame;
        f.insert(f.end(), pad, 0);
        const size_t trimmed = trimZeroPadding(f.data(), f.size());
        if (parseAction(f.data(), f.size(), &decoded) == ParseResult::Valid)
            assert(trimmed == f.size());
        else
            assert(trimmed == frame.size());
    }
    // A last TLV whose own value ends in zeros is kept whole: only the padding after it goes.
    auto zeroTail = frame;
    zeroTail.insert(zeroTail.end(), {0x10, 3, 0, 0x55, 0, 0});
    const size_t unpadded = zeroTail.size();
    zeroTail.insert(zeroTail.end(), 8, 0);
    assert(trimZeroPadding(zeroTail.data(), zeroTail.size()) == unpadded);
    // Left alone: a valid frame, a non-zero trailer (an FCS), a non-zero byte inside the padding, a
    // header with only padding, a frame that isn't AWDL, a null pointer, and every shorter length.
    assert(trimZeroPadding(frame.data(), frame.size()) == frame.size());
    auto fcs = frame;
    fcs.insert(fcs.end(), {0x12, 0x34, 0x56, 0x78});
    assert(trimZeroPadding(fcs.data(), fcs.size()) == fcs.size());
    auto dirty = padded;
    dirty[frame.size()+5] = 1;
    assert(trimZeroPadding(dirty.data(), dirty.size()) == dirty.size());
    std::vector<uint8_t> header(frame.begin(), frame.begin()+40);
    header.insert(header.end(), 8, 0);
    assert(trimZeroPadding(header.data(), header.size()) == header.size());
    auto other = padded;
    other[16] = 0x02;
    assert(trimZeroPadding(other.data(), other.size()) == other.size());
    assert(trimZeroPadding(nullptr, 64) == 64);
    for (size_t n = 0; n <= padded.size(); ++n) {
        const size_t trimmed = trimZeroPadding(padded.data(), n);
        assert(trimmed == n || (n > frame.size() && trimmed == frame.size()));
    }
}

static void checkManagementOwnership()
{
    struct Sink {
        struct Buffer { uint8_t bytes[128]; } buffer;
        size_t available = sizeof(buffer.bytes), length = 0;
        unsigned acquired = 0, returned = 0, submitted = 0;
        bool exhausted = false, nullBytes = false;
        ItlAWDL::ManagementMetadata metadata = {};
        Buffer *acquire() { if (exhausted) return nullptr; ++acquired; return &buffer; }
        void *bytes(Buffer *p) { return nullBytes ? nullptr : p->bytes; }
        size_t capacity(Buffer*) { return available; }
        void giveBack(Buffer*) { ++returned; }
        void setLength(Buffer*, size_t n) { length = n; }
        void submit(Buffer*, ItlAWDL::ManagementMetadata *m) { ++submitted; metadata = *m; }
    } sink;
    auto frame = syncFrame();
    frame[10] = 0x42;
    assert(ItlAWDL::copyManagementFrame(sink, frame.data(), frame.size(), 149, 16) == kIOReturnSuccess);
    assert(sink.acquired == 1 && sink.submitted == 1 && sink.returned == 0);
    assert(sink.length == frame.size()-24 && sink.metadata.peer[0] == 0x42);
    assert(sink.metadata.rssi == 0);
    decltype(sink) rssiSink;
    assert(ItlAWDL::copyManagementFrame(rssiSink, frame.data(), frame.size(), 6, 8, -37) == kIOReturnSuccess);
    assert(rssiSink.metadata.rssi == -37 && rssiSink.metadata.channel == 6);
    assert(reinterpret_cast<const uint8_t *>(&rssiSink.metadata)[0x2c] == 0xdb);
    assert(sink.metadata.version == 1 && sink.metadata.channel == 149);
    assert(sink.buffer.bytes[0] == 0x7f && sink.buffer.bytes[1] == 0);
    assert(sink.buffer.bytes[2] == 0x17 && sink.buffer.bytes[3] == 0xf2);
    assert(sink.buffer.bytes[4] == 8 && sink.buffer.bytes[5] == 0x10);
    Sink fcsSink = {};
    auto captured = frame;
    captured.insert(captured.end(), {0x12, 0x34, 0x56, 0x78});
    ItlAWDL::ActionFrame decoded;
    size_t normalizedLength = 0;
    assert(ItlAWDL::parseCapturedAction(captured.data(), captured.size(), true,
        &decoded, &normalizedLength) == ItlAWDL::ParseResult::Valid);
    assert(ItlAWDL::copyManagementFrame(fcsSink, captured.data(), normalizedLength, 149, 16) == kIOReturnSuccess);
    assert(fcsSink.length == frame.size()-24);
    assert(memcmp(fcsSink.buffer.bytes, frame.data()+24, fcsSink.length) == 0);
    frame[24] = 0; // Caller can reuse its storage; the native buffer owns a copy.
    assert(sink.buffer.bytes[0] == 0x7f);
    frame[24] = 0x7f;
    sink.available = frame.size()-25;
    assert(ItlAWDL::copyManagementFrame(sink, frame.data(), frame.size(), 149, 16) == kIOReturnNoSpace);
    assert(sink.acquired == 2 && sink.submitted == 1 && sink.returned == 1);
    sink.available = 128; sink.nullBytes = true;
    assert(ItlAWDL::copyManagementFrame(sink, frame.data(), frame.size(), 149, 16) == kIOReturnNoSpace);
    assert(sink.acquired == 3 && sink.submitted == 1 && sink.returned == 2);
    sink.exhausted = true;
    assert(ItlAWDL::copyManagementFrame(sink, frame.data(), frame.size(), 149, 16) == kIOReturnNoResources);
    sink.exhausted = false;
    assert(ItlAWDL::copyManagementFrame(sink, frame.data(), frame.size(), 0, 16) == kIOReturnBadArgument);
    frame[41] = 0xff;
    assert(ItlAWDL::copyManagementFrame(sink, frame.data(), frame.size(), 149, 16) == kIOReturnBadArgument);
    assert(sink.acquired == 3 && sink.submitted == 1 && sink.returned == 2);
}

int main()
{
    checkInactiveState();
    checkTemplate();
    checkParser();
    checkZeroPadding();
    checkManagementOwnership();
    puts("AWDL state, template, parser and 25000 fuzz cases: passed");
}
