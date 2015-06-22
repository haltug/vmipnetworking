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
#include "inet/networklayer/contract/ipv6/IPv6Address.h"
#include "inet/networklayer/ipv6tunneling/IPv6Tunneling.h"

namespace inet {

class InterfaceEntry;
class IPv6ControlInfo;
class IPv6Datagram;
class IPv6NeighbourDiscovery;
class IPv6Tunneling;
class IPv6RoutingTable;

#define KEY_SEQ_CHANGE_CA               0 // Sequence Update to CA
#define KEY_SEQ_CHANGE_DA               1 // Sequence Update to CA

class VehicleAgent : public cSimpleModule
{
  public:
    virtual ~VehicleAgent();

  protected:
    IInterfaceTable *ift;
    IPv6RoutingTable *rt6;
    IPv6NeighbourDiscovery *ipv6nd;

    class RetransmitTimer
    {
      public:
        cMessage *timer;
        virtual ~RetransmitTimer() {};
        IPv6Address dest;    // the address (HA or CN(s) for which the message is sent
        simtime_t ackTimeout;    // timeout for the Ack
        simtime_t nextScheduledTime;    // time when the corrsponding message is supposed to be sent
        InterfaceEntry *ifEntry;    // interface from which the message will be transmitted

    };

    struct Key // TODO
    {
        int type;    // type of the message (BU, HoTI, CoTI) stored in the map, indexed by this key
        int interfaceID;    // ID of the interface over which the message is sent
        IPv6Address dest;    // the address of either the HA or the CN
        Key(IPv6Address _dest, int _interfaceID, int _type)
        {
            dest = _dest;
            interfaceID = _interfaceID;
            type = _type;
        }
        bool operator<(const Key& b) const
        {
            if (type == b.type)
                return interfaceID == b.interfaceID ? dest < b.dest : interfaceID < b.interfaceID;
            else
                return type < b.type;
        }
    };
    // FOR THE PURPOSE OF WHAT?
    typedef std::map<Key, TimerIfEntry *> TransmitIfList;
    TransmitIfList transmitIfList;

    /** holds the tuples of currently available {InterfaceID, NetworkAddress} pairs */
    typedef std::map<int, IPv6Address> InterfaceLocalAddressList;
    InterfaceLocalAddressList interfaceLocalAddressList;

    struct ConnectionSession
    {
        IPv6Address corrNode;
        IPv6Address dataAgent;

    };
    typedef std::vector<ConnectionSession> CorrNodeDataAgentList;
    CorrNodeDataAgentList corrNodeDataAgentList;
    CorrNodeDataAgentList::iterator itCorrNodeDataAgentList;

    class CAInitializationTimer : public RetransmitTimer
    {
    };
    class CASessionRequestTimer : public RetransmitTimer
    {
    };
    class CASequenceUpdateTimer : public RetransmitTimer
    {
      public:
        uint caSequenceNumber;    // sequence number of the CA
        uint lifeTime;    // lifetime of the BU sent, 4.9.07 - CB   //Time variable related to the time at which BU was sent
        simtime_t presentSentTimeCA;    //stores the present time at which BU is/was sent
    };
    class CALocationUpdateTimer : public RetransmitTimer
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

    RetransmitTimer *getRetransmitTimer(Key& key);

    RetransmitTimer *lookupRetransmitTimer(IPv6Address& ip);

    RetransmitTimer *removeRetransmitTimer(IPv6Address& ip);

    void cancelRetransmitTimers();





















    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

} //namespace

#endif
