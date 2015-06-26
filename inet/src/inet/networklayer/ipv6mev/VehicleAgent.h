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

  protected:
    IInterfaceTable *ift;
    IPv6RoutingTable *rt6;
    IPv6NeighbourDiscovery *ipv6nd;
    L3Address *mobileID;

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

    typedef std::map<IPv6Address, IPv6Address> TargetToAgentList;
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
    void createControlAgentInitializationHeader();

    void createControlAgentSessionRequestHeader(); // for getting ip address of data agent

    void createControlAgentSequenceUpdateHeader(); // for informing of net addr changes

    void createControlAgentLocationUpdateHeader(); // for latter implementation

    void createDataAgentRegularTransmissionHeader(); // nothing to say

    void createDataAgentSequenceUpdateHeader(); // for updating network address changes

    void createDataAgentSessionRequestHeader(); // for establishing a stable session

    void createDataAgentRelayingRequestHeader(); // for relaying to attached target address

    void processControlAgentMessages(IdentificationHeader *idHdr,  IPv6ControlInfo *ipCtrlInfo);

    void processDataAgentMessages(IdentificationHeader *idHdr,  IPv6ControlInfo *ipCtrlInfo);

    bool cancelRetransmitTimer(const IPv6Address& dest, int interfaceID, int msgType);

    void cancelRetransmitTimers();




    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
};

} //namespace

#endif
