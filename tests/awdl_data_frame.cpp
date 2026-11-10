#include <AWDLDataFrame.hpp>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
int main() {
    using namespace ItlAWDL;
    uint8_t frame[84]={8},ip[40]={0x60};
    const uint8_t bssid[]={0,0x25,0,0xff,0x94,0x73};
    memset(frame+4,255,6);frame[10]=2;frame[15]=1;
    memcpy(frame+16,bssid,6);write16(frame+22,0xabc0);
    size_t length;
    assert(buildIPv6DataPayload(ip,40,0x1234,frame+24,56,&length)==BuildResult::Ready);
    DataFrameView view;
    assert(parseDataFrame(frame,84,true,&view)==ParseResult::Valid);
    assert(view.wifiSequence==0xabc && view.ip.sequence==0x1234 && view.ip.length==40);
    uint8_t ethernet[54]; size_t ethernetLength=999;
    assert(copyDataEthernet(view,ethernet,sizeof(ethernet),&ethernetLength)==BuildResult::Ready);
    assert(ethernetLength==54 && !memcmp(ethernet,frame+4,6) && !memcmp(ethernet+6,frame+10,6));
    assert(ethernet[12]==0x86 && ethernet[13]==0xdd && !memcmp(ethernet+14,ip,40));
    for(unsigned cap=0;cap<54;++cap) {
        memset(ethernet,0xa5,sizeof(ethernet));ethernetLength=999;
        assert(copyDataEthernet(view,ethernet,cap,&ethernetLength)==BuildResult::NoSpace && ethernetLength==999);
        for(auto byte:ethernet) assert(byte==0xa5);
    }
    assert(view.source==frame+10 && view.destination==frame+4 && view.ip.bytes==frame+40);
    assert(parseDataFrame(frame,80,false,&view)==ParseResult::Valid);
    assert(parseDataFrame(frame,84,false,&view)==ParseResult::Malformed);
    for(unsigned n=0;n<84;++n) {
        DataFrameView output={{ip,7,9},ip,ip,10};
        assert(parseDataFrame(frame,n,true,&output)!=ParseResult::Valid);
        assert(output.source==ip && output.ip.length==7 && output.wifiSequence==10);
    }
    for(unsigned flag: {1U,2U,4U,0x40U,0x80U}) {
        frame[1]=flag;assert(parseDataFrame(frame,84,true,&view)==ParseResult::Unsupported);
    }
    frame[1]=0;frame[22]|=1;
    assert(parseDataFrame(frame,84,true,&view)==ParseResult::Unsupported);frame[22]&=0xf0;
    frame[10]=1;assert(parseDataFrame(frame,84,true,&view)==ParseResult::Malformed);frame[10]=2;
    frame[16]=1;assert(parseDataFrame(frame,84,true,&view)==ParseResult::NotAWDL);frame[16]=0;
    uint8_t qos[86]={};memcpy(qos,frame,24);qos[0]=0x88;memcpy(qos+26,frame+24,60);
    assert(parseDataFrame(qos,86,true,&view)==ParseResult::Valid && view.ip.bytes==qos+42);
    qos[24]=0x80;assert(parseDataFrame(qos,86,true,&view)==ParseResult::Unsupported);
    uint32_t random=1;
    for(unsigned i=0;i<25000;++i) {
        uint8_t bytes[128];for(auto &b:bytes) {random=random*1664525+1013904223;b=random>>24;}
        DataFrameView decoded;
        if(parseDataFrame(bytes,random%129,i&1,&decoded)==ParseResult::Valid)
            assert(decoded.ip.length>=40 && decoded.ip.length<=1500);
    }
    {
        // A whole 802.11 frame with the outer header, as it arrives with FCS.
        static uint8_t w[1600];const uint8_t head[26]={0x88,0,0x2c,0,2,1,1,1,1,1,2,2,2,2,2,2,0,0x25,0,0xff,0x94,0x73,0x80,2,0,0};
        memcpy(w,head,26);const uint8_t outer[8]={0xaa,0xaa,3,0,0,0,0x05,0xe9};memcpy(w+26,outer,8);
        uint8_t inner[1497]={0x60};inner[4]=(1497-40)>>8;inner[5]=(1497-40)&255;size_t n2=0;
        assert(buildIPv6DataPayload(inner,1497,0x28,w+34,1500+16,&n2)==BuildResult::Ready);
        DataFrameView v2{};
        assert(parseDataFrame(w,26+8+n2+4,true,&v2)==ParseResult::Valid);
        assert(v2.ip.bytes==w+50&&v2.ip.length==1497&&v2.source==w+10&&v2.destination==w+4);
    }
    puts("AWDL outer data frame/FCS/QoS decoding: passed (offline only)");
}
