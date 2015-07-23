//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __INET_DATAAGENT_H_
#define __INET_DATAAGENT_H_

#include "inet/networklayer/ipv6mev/Agent.h"

#include <map>
#include <vector>
#include "inet/common/INETDefs.h"
#include "inet/networklayer/contract/ipv6/IPv6Address.h"
#include "inet/networklayer/ipv6mev/IdentificationHeader.h"
#include "inet/networklayer/ipv6mev/AddressManagement.h"
#include "inet/networklayer/common/InterfaceEntry.h"

namespace inet {

/**
 * TODO - Generated class
 */
class DataAgent : public Agent
{
    virtual ~DataAgent() {};
  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    IInterfaceTable *ift = nullptr; // for recognizing changes etc
  public:
    void createSequenceUpdateNotificaiton(uint64 mobileId, uint seq);
    void sendSequenceUpdateNotification(cMessage *msg); // used by DA to notify CA of changes

    void sendAgentInitResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId, uint seq);
    void sendAgentUpdateResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId, uint seq);

    void processMobileAgentMessage(MobileAgentHeader *agentHdr, IPv6ControlInfo *ipCtrlInfo);
    void processControlAgentMessage(ControlAgentHeader *agentHdr, IPv6ControlInfo *ipCtrlInfo);
    void proccessRegularNodeMessage(cMessage *msg, short protocol); // forwards message to CN
    // INTERFACE
    InterfaceEntry *getInterface(IPv6Address destAddr = IPv6Address(), int destPort = -1, int sourcePort = -1, short protocol = -1); //const ,
    void sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, simtime_t sendTime = 0); // resend after timer expired
};

} //namespace

#endif
