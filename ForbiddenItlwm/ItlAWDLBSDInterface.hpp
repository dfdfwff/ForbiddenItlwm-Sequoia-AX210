#pragma once

#include "AirportItlwmEthernetInterface.hpp"
#include "AirportAWDLInterface.hpp"

// BSD-side awdl0. Sequoia's native IOSkywalk registration creates the
// management object but no BSD ifnet, so attach a normal IOEthernetInterface
// to the same AWDL management object. This class deliberately owns no radio
// state; it only gives configd and the network stack a real interface.
class ItlAWDLBSDInterface : public AirportItlwmEthernetInterface {
    OSDeclareDefaultStructors(ItlAWDLBSDInterface)

public:
    bool initWithAWDL(IONetworkController *controller, ItlAWDLInterface *owner,
                      const ether_addr &address);
    const char *getNamePrefix() const override;
    bool initIfnetParams(struct ifnet_init_params *params) override;
    IOReturn attachToDataLinkLayer(IOOptionBits options, void *parameter) override;
    IOReturn getInterfaceAddress(IOEthernetAddress *addr);
    SInt32 performCommand(IONetworkController *controller, unsigned long cmd,
                          void *arg0, void *arg1) override;
    void ensureIPv6Address(unsigned attempt);
    unsigned getIPv6Attempts() const { return fIPv6Attempts; }
    UInt32 outputAWDL(mbuf_t packet, void *param);

private:
    ItlAWDLInterface *fAWDLOwner;
    ether_addr fAddress;
    unsigned fIPv6Attempts = 0;
    SInt32 fLastIPv6Result = -1;
};
