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
#include "inet/networklayer/common/InterfaceEntry.h"



namespace inet {

/**
 * TODO - Generated class
 */
class ControlAgent : public Agent
{
public:
    virtual ~ControlAgent();
  protected:
    IInterfaceTable *ift = nullptr; // for recognizing changes etc
//    std::vector<uint64>  mobileIdList; // lists all id of mobile nodes
//    std::vector<IPv6Address> agentAddressList; // lists all data agents
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    // statistics
    static simsignal_t numDataAgents;
    static simsignal_t numMobileAgents;
    static simsignal_t numFlowRequests;
    static simsignal_t numSequenceUpdate;
    static simsignal_t numSequenceResponse;
    static simsignal_t  txTraffic;
    static simsignal_t  rxTraffic;
    long numDataAgentsStat = 0;
    long numMobileAgentsStat = 0;
    long numFlowRequestsStat = 0;
    long numSequenceUpdateStat = 0;
    long numSequenceResponseStat = 0;
    long txTrafficStat = 0;
    long rxTrafficStat = 0;

  public:
    // CA FUNCTION
    void createAgentInit(uint64 mobileId); // used by CA
    void sendAgentInit(cMessage *msg); // used by CA to init DA
    void createAgentUpdate(uint64 mobileId, uint seq); // used by CA to update all its specific data agents
    void sendAgentUpdate(cMessage *msg);

    void createSequenceUpdateAck(uint64 mobileId);
    void sendSequenceUpdateAck(cMessage *msg); // confirm to MA its new
    void sendSessionInitResponse(IPv6Address dest);
    void sendSequenceInitResponse(IPv6Address dest, uint64 mobileId, uint seq);
    void sendSequenceUpdateResponse(IPv6Address destAddr, uint64 mobileId, uint seq);
    void sendFlowRequestResponse(IPv6Address destAddr, uint64 mobileId, uint seq, IPv6Address nodeAddr, IPv6Address agentAddr);
    // MSG PROCESSING
    void processAgentMessage(IdentificationHeader *agentHeader, IPv6ControlInfo *ipCtrlInfo);
    void initializeSession(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void initializeSequence(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void performSeqUpdate(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void performFlowRequest(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void performAgentInitResponse(IdentificationHeader *agentHeader, IPv6Address sourceAddr);
    void performAgentUpdateResponse(IdentificationHeader *agentHeader, IPv6Address sourceAddr);

    // INTERFACE
    InterfaceEntry *getInterface(); //const ,
    void sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, simtime_t sendTime = 0);

    // UTIL
    int getIndexFromModule(IPv6Address addr);
};

} //namespace

#endif
