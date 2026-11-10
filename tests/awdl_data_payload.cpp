#include <AWDLDataPayload.hpp>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
int main() {
    using namespace ItlAWDL;
    uint8_t ip[1500]={0x60}, frame[1518]; size_t n=999;
    assert(buildIPv6DataPayload(ip,40,0x1234,frame+1,1516,&n)==BuildResult::Ready && n==56);
    const uint8_t expected[]={0xaa,0xaa,3,0,0x17,0xf2,8,0,3,4,0x34,0x12,0,0,0x86,0xdd};
    assert(!memcmp(frame+1,expected,16) && !memcmp(frame+17,ip,40));
    IPv6PayloadView view;
    assert(parseIPv6DataPayload(frame+1,n,&view)==ParseResult::Valid);
    assert(view.sequence==0x1234 && view.bytes==frame+17 && view.length==40);
    for(size_t cap=0;cap<56;++cap) {
        memset(frame,0xa5,sizeof(frame)); n=999;
        assert(buildIPv6DataPayload(ip,40,0,frame,cap,&n)==BuildResult::NoSpace && n==999);
        for(auto b:frame) assert(b==0xa5);
    }
    ip[4]=5;ip[5]=0xb4; // 1460 bytes following the fixed IPv6 header
    assert(buildIPv6DataPayload(ip,1500,65535,frame,1516,&n)==BuildResult::Ready && n==1516);
    assert(parseIPv6DataPayload(frame,n,&view)==ParseResult::Valid && view.length==1500);
    for(size_t len=0;len<n;++len) {
        IPv6PayloadView rejected={ip,7,9};
        assert(parseIPv6DataPayload(frame,len,&rejected)!=ParseResult::Valid);
        assert(rejected.bytes==ip && rejected.length==7 && rejected.sequence==9);
    }
    for(unsigned offset: {0U,3U,6U,8U,12U,14U,16U,20U}) {
        frame[offset]^=(offset==16 ? 0x10 : 1);
        assert(parseIPv6DataPayload(frame,n,&view)!=ParseResult::Valid);
        frame[offset]^=(offset==16 ? 0x10 : 1);
    }
    ip[0]=0x40;
    assert(buildIPv6DataPayload(ip,1500,0,frame,1516,&n)==BuildResult::Invalid);
    uint32_t seed=1;
    for(unsigned k=0;k<25000;++k) {
        uint8_t bytes[128];
        for(auto &b:bytes) { seed=seed*1664525+1013904223;b=uint8_t(seed>>24); }
        const size_t len=seed%129;
        if(parseIPv6DataPayload(bytes,len,&view)==ParseResult::Valid)
            assert(view.bytes==bytes+16 && view.length==len-16);
    }
    {
        // The outer header as captured from a phone, around a full IPv6 packet of 1497 bytes.
        static uint8_t big[1600];uint8_t packet[1497]={0x60};packet[4]=(1497-40)>>8;packet[5]=(1497-40)&255;
        const uint8_t outer[8]={0xaa,0xaa,3,0,0,0,0x05,0xe9};
        memcpy(big,outer,8);size_t inner=0;
        assert(buildIPv6DataPayload(packet,1497,0x28,big+8,1592,&inner)==BuildResult::Ready&&inner==1513);
        IPv6PayloadView wrapped{};
        assert(hasOuterDataHeader(big,1521));
        assert(parseIPv6DataPayload(big,1521,&wrapped)==ParseResult::Valid);
        assert(wrapped.bytes==big+24&&wrapped.length==1497&&wrapped.sequence==0x28);
        // The length must cover the rest exactly, so a frame with more after it is refused.
        assert(parseIPv6DataPayload(big,1520,&wrapped)==ParseResult::Unsupported);
        assert(parseIPv6DataPayload(big,1522,&wrapped)==ParseResult::Unsupported);
        // An outer header with too little inside, or something other than AWDL inside.
        uint8_t tiny[16]={0xaa,0xaa,3,0,0,0,0,8};
        assert(parseIPv6DataPayload(tiny,16,&wrapped)==ParseResult::Malformed);
        big[12]=0;assert(parseIPv6DataPayload(big,1521,&wrapped)==ParseResult::NotAWDL);big[12]=0x17;
        // A bad IPv6 packet inside is still refused.
        big[24+5]^=1;assert(parseIPv6DataPayload(big,1521,&wrapped)==ParseResult::Malformed);big[24+5]^=1;
        assert(!hasOuterDataHeader(big+8,1513)&&!hasOuterDataHeader(big,7)&&!hasOuterDataHeader(nullptr,20));
    }
    puts("AWDL IPv6 payload codec: passed (offline only)");
}
