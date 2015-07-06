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

#ifndef __INET_VEHICLEAGENT_H_
#define __INET_VEHICLEAGENT_H_

#include <omnetpp.h>
#include <vector>
#include <map>

#include "inet/common/INETDefs.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/contract/ipv6/IPv6Address.h"
#include "inet/networklayer/contract/ipv6/IPv6ControlInfo.h"
#include "inet/networklayer/icmpv6/IPv6NeighbourDiscovery.h"
#include "inet/networklayer/ipv6/IPv6Datagram.h"
#include "inet/networklayer/ipv6/IPv6RoutingTable.h"
#include "inet/networklayer/ipv6mev/AddressManagement.h"
#include "inet/networklayer/ipv6mev/IdentificationHeader.h"
#include "inet/networklayer/ipv6mev/Utils.h"
#include "inet/networklayer/ipv6tunneling/IPv6Tunneling.h"

namespace inet {

class InterfaceEntry;
class IPv6ControlInfo;
class IPv6Datagram;
class IPv6NeighbourDiscovery;
class IPv6RoutingTable;
class IPv6Tunneling;



class VehicleAgent : public cSimpleModule
{
  public:
    virtual ~VehicleAgent();
    enum AgentState {
        NONE       = 0,
        INITIALIZE = 1,
        REGISTERED = 2
    };
    AgentState state;

  protected:
    IInterfaceTable *ift;
    IPv6RoutingTable *rt6;
    IPv6NeighbourDiscovery *ipv6nd;
    VehicleIdentification *vid;
    AddressManagement *am;
    IPv6Address *ca;

    // FOR THE PURPOSE OF WHAT?
//    typedef std::map<Utils::Key, Utils::RetransmitTimer *> TransmitIfList;
//    TransmitIfList transmitIfList;

    /** holds the tuples of currently available {InterfaceID, NetworkAddress} pairs */
    typedef std::map<int, IPv6Address> InterfaceToIPv6AddressList;
    InterfaceToIPv6AddressList interfaceToIPv6AddressList;

//    struct ConnectionSession
//    {
//        IPv6Address corrNode;
//        IPv6Address dataAgent;
//
//    };
//    typedef std::vector<ConnectionSession> CorrNodeDataAgentList;
//    CorrNodeDataAgentList corrNodeDataAgentList;
//    CorrNodeDataAgentList::iterator itCorrNodeDataAgentList;

    // a map that holds the association of correspondent node with data agents
    typedef std::map<IPv6Address, IPv6Address> TargetToAgentList; // IPv6address should be replaced with DataAgent <cn,da>
    TargetToAgentList targetToAgentList;

    class CAInitializationTimer : public Utils::RetransmitTimer
    {
    };
    class CASessionRequestTimer : public Utils::RetransmitTimer
    {
    };
    class CASequenceUpdateTimer : public Utils::RetransmitTimer
    {
      public:
        uint caSequenceNumber;    // sequence number of the CA
        uint lifeTime;    // lifetime of the BU sent, 4.9.07 - CB   //Time variable related to the time at which BU was sent
        simtime_t presentSentTimeCA;    //stores the present time at which BU is/was sent
    };
    class CALocationUpdateTimer : public Utils::RetransmitTimer
    {
    };
//    DataAgent does not need timer since TCP should handle such cases.
//    class DASessionRequestTimer : public RetransmitTimer
//    {
//    };
//    class DASequenceUpdateTimer : public RetransmitTimer
//    {
//    };
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    void sendCAInitialization(); // send initialization message to CA
    void resendCAInitialization(cMessage *msg); // resend after timer expired

    void sendCASequenceUpdate(); // for informing of net addr changes
    void resendCASequenceUpdate(cMessage *msg); // resend after timer expired

    void sendCASessionRequest(); // for getting ip address of data agent
    void resendCASessionRequest(cMessage *msg); // resend after timer expired

    void sendCALocationUpdate(); // for latter implementation
    void resendCALocationUpdate(cMessage *msg); // resend after timer expired

    void processCAMessages(IPv6Datagram *datagram, ControlAgentHeader *ctrlAgentHdr);

    void createDARegularTransmissionHeader(); // nothing to say

    void createDASequenceUpdateHeader(); // for updating network address changes

    void createDASessionRequestHeader(); // for establishing a stable session

    void createDARelayingRequestHeader(); // for relaying to attached target address

    void processDAMessages(IPv6Datagram *datagram, DataAgentHeader *dataAgentHdr);


    bool cancelRetransmitTimer(const IPv6Address& dest, int interfaceID, int msgType);

    void cancelRetransmitTimers();

};

} //namespace

#endif
