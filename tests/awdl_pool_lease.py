"""Exercise the actual lease probe with preparation-sensitive pool mocks."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'ForbiddenItlwm/AirportItlwmV2.cpp').read_text()
a=s.index('    int poolProbe = 0;');b=s.index('    IOSkywalkEthernetInterface::RegistrationInfo info',a)
body=s[a:b]
program=r'''
#include <AWDLDataPayload.hpp>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
using UInt32=unsigned;using IOReturn=unsigned;
enum { kIOReturnSuccess=0,kIOReturnUnsupported=1 };
static bool failPreparation;
struct IOEventSource {};
struct ItlSkywalkPacketView {
    uint8_t bytes[56]={};
    uint64_t getDataVirtualAddress() { assert(prepared);return reinterpret_cast<uint64_t>(bytes); }
    bool prepared=false;unsigned length=0,completed=0;
    IOReturn prepareWithQueue(void *q,unsigned d,unsigned o) {
        assert(q && !d && !o && !prepared);prepared=true;return failPreparation?2:0;
    }
    unsigned getPacketBufferCount() { return prepared?1:0; }
    unsigned getDataLength() { return length; }
    IOReturn setDataLength(unsigned n) { assert(prepared);length=n;return 0; }
    IOReturn completeWithQueue(void *q,unsigned d,unsigned o) {
        assert(q && !d && !o && prepared && !length);for(auto b:bytes) assert(b==0);prepared=false;++completed;return 0;
    }
};
struct ItlSkywalkPoolView {
    ItlSkywalkPacketView packet;unsigned allocated=0,returned=0;
    IOReturn allocatePacket(unsigned n,ItlSkywalkPacketView **p,unsigned o) {
        assert(n==1 && o==1 && !allocated);++allocated;*p=&packet;return 0;
    }
    void deallocatePacket(ItlSkywalkPacketView *p) {
        assert(p==&packet && !p->prepared && p->completed==1 && !returned);++returned;
    }
};
using IOSkywalkPacketBufferPool=ItlSkywalkPoolView;
struct OSNumber {
    static OSNumber *withNumber(unsigned,unsigned) { return new OSNumber; }
    void release() { delete this; }
};
struct OSDictionary {
    static OSDictionary *withCapacity(unsigned) { return new OSDictionary; }
    void setObject(const char*,OSNumber*) {} void release() { delete this; }
};
static bool PE_parse_boot_argn(const char*,void*,unsigned) { return true; }
struct Owner {
    ItlSkywalkPoolView tx,rx;
    IOSkywalkPacketBufferPool *txPool=&tx,*rxPool=&rx;
    IOEventSource queue;IOEventSource *packetQueues[3]={&queue,&queue,&queue};
    template<typename... T> void setProperty(T...) {}
    void run();
};
void Owner::run() {
'''+body+r'''
}
int main() {
    for(unsigned fail=0;fail<2;++fail) {
        Owner owner;failPreparation=fail;owner.run();
        assert(owner.tx.allocated==1 && owner.tx.returned==1 && owner.tx.packet.completed==1);
        assert(owner.rx.allocated==1 && owner.rx.returned==1 && owner.rx.packet.completed==1);
    }
    puts("native pool lease prepare/complete ordering: passed (host mocks only)");
}
'''
with tempfile.TemporaryDirectory(prefix='awdl-lease-') as directory:
    p=Path(directory)/'test.cpp';p.write_text(program);e=Path(directory)/'test'
    subprocess.run(['xcrun','clang++','-std=c++14','-I'+str(root/'include'),'-fsanitize=address,undefined',str(p),'-o',str(e)],check=True)
    subprocess.run([str(e)],check=True)
