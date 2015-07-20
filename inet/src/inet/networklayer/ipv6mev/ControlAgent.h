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

#ifndef __INET_CONTROLAGENT_H_
#define __INET_CONTROLAGENT_H_
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
class ControlAgent : public Agent
{

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    IInterfaceTable *ift = nullptr; // for recognizing changes etc
    std::vector<uint64>  mobileIdList; // lists all id of mobile nodes
    std::vector<IPv6Address> nodeAddressList; // lists all data agents
  public:
    // CA function
    void createAgentInit(uint64 mobileId); // used by CA
    void sendAgentInit(cMessage *msg); // used by CA to init DA
    void createAgentUpdate(uint64 mobileId); // used by CA to update all its specific data agents
    void sendAgentUpdate(cMessage *msg);
    void sendSequenceUpdateAck(uint64 mobileId); // confirm to MA its new
    void sendSessionInitResponse(IPv6Address dest, IPv6Address source);
    void sendSequenceInitResponse(IPv6Address dest, IPv6Address source, uint64 mobileId);
    void sendSequenceUpdateResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId);
    void sendFlowRequestResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId, IPv6Address nodeAddr, IPv6Address agentAddr);
    void processMobileAgentMessage(MobileAgentHeader *agentHdr, IPv6ControlInfo *ipCtrlInfo);
    void processDataAgentMessage(DataAgentHeader *agentHdr, IPv6ControlInfo *ipCtrlInfo);
    // INTERFACE
    InterfaceEntry *getInterface(IPv6Address destAddr = IPv6Address(), int destPort = -1, int sourcePort = -1, short protocol = -1); //const ,
    void sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr = IPv6Address::UNSPECIFIED_ADDRESS, int interfaceId = -1, simtime_t sendTime = 0); // resend after timer expired

};

} //namespace

#endif
