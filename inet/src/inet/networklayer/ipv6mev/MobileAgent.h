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

#ifndef __INET_MOBILEAGENT_H_
#define __INET_MOBILEAGENT_H_
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
class MobileAgent : public cListener, public Agent
{
    virtual ~MobileAgent() {};
  protected:
    IInterfaceTable *ift = nullptr; // for recognizing changes etc
    cModule *interfaceNotifier = nullptr; // listens for changes in interfacetable
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    class InterfaceUnit { // represents the entry of addressTable
    public:
        bool active;
        int priority;
        IPv6Address careOfAddress;
    };
    typedef std::map<int, InterfaceUnit *> AddressTable; // represents the address table
    AddressTable addressTable;

    struct InterfaceInit {
        int id;
        InterfaceUnit *iu;
    };

  public:
    void createSessionInit();
    void sendSessionInit(cMessage *msg); // send initialization message to CA
    void createSequenceInit();
    void sendSequenceInit(cMessage *msg);
    void createSequenceUpdate(uint64 mobileId, uint seq, uint ack);
    void sendSequenceUpdate(cMessage *msg);
    void createFlowRequest(FlowTuple &tuple);
    void sendFlowRequest(cMessage *msg);
//  PACKET PROCESSING
    void processAgentMessage(IdentificationHeader *agentHeader, IPv6ControlInfo *ipCtrlInfo);
    void performSessionInitResponse(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void performSequenceInitResponse(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void performFlowRequestResponse(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void performSequenceUpdateResponse(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void performIncomingUdpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *ipCtrlInfo);
    void performIncomingTcpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *ipCtrlInfo);
    //
    void processOutgoingUdpPacket(cMessage *msg, IPv6ControlInfo *ipCtrlInfo);
    void processOutgoingTcpPacket(cMessage *msg, IPv6ControlInfo *ipCtrlInfo);
//  INTERFACE LISTENER FUNCTIONS
    void createInterfaceDownMessage(int id);
    void handleInterfaceDownMessage(cMessage *msg);
    void createInterfaceUpMessage(int id);
    void handleInterfaceUpMessage(cMessage *msg);
    void updateAddressTable(int id, InterfaceUnit *iu);
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj) override;
    InterfaceUnit* getInterfaceUnit(int id);
// INTERFACE
    InterfaceEntry *getAnyInterface();
    InterfaceEntry *getInterface(IPv6Address destAddr = IPv6Address::UNSPECIFIED_ADDRESS, int destPort = -1, int sourcePort = -1, short protocol = -1); //const ,
    void sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr = IPv6Address::UNSPECIFIED_ADDRESS, int interfaceId = -1, simtime_t sendTime = 0); // resend after timer expired

};

} //namespace

#endif
