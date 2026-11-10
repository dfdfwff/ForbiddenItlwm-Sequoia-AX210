#include <AWDLSyncPrefix.hpp>
#include <cassert>
#include <cstring>
#include <cstdio>
int main()
{
    using namespace ItlAWDL;
    SyncPrefix p = {6,6,1,0x1234,16,110,0x1000,16,16,7,3,4,5,6,{2,1,2,3,4,5}};
    uint8_t bytes[29]; memset(bytes,0xa5,sizeof(bytes)); size_t n = 999;
    assert(buildSyncPrefix(p,bytes+1,27,&n)==BuildResult::Ready && n==27);
    const uint8_t expected[] = {6,0x34,0x12,6,1,16,0,110,0,0,0x10,16,0,16,0,7,0,3,4,5,6,2,1,2,3,4,5};
    assert(!memcmp(bytes+1,expected,27) && bytes[0]==0xa5 && bytes[28]==0xa5);
    LocalFrame frame = {{2,1,2,3,4,5},{255,255,255,255,255,255},3,1,100,100,bytes+1,27,nullptr,0};
    uint8_t packet[70]; size_t length;
    assert(buildLocalFrame(frame,packet,sizeof(packet),&length)==BuildResult::Ready);
    ActionFrame parsed;
    assert(parseAction(packet,length,&parsed)==ParseResult::Valid && parsed.hasSync);
    assert(parsed.txDownCounter==0x1234 && parsed.remainingAW==7 && parsed.awPeriod==16 && parsed.afPeriod==110);
    assert(!memcmp(parsed.masterAddress,p.master,6));
    for (size_t cap=0;cap<27;++cap) {
        memset(bytes,0xa5,sizeof(bytes)); n=999;
        assert(buildSyncPrefix(p,bytes,cap,&n)==BuildResult::NoSpace && n==999);
        for (auto b:bytes) assert(b==0xa5);
    }
    p.master[0]=1; assert(buildSyncPrefix(p,bytes,27,&n)==BuildResult::Invalid);
    p.master[0]=2; p.awPeriod=0;
    assert(buildSyncPrefix(p,bytes,27,&n)==BuildResult::Invalid);
    puts("AWDL sync prefix encoding: passed (offline; no scheduler)");
}
