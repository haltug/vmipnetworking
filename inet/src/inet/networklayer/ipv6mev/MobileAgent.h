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
#include "inet/networklayer/icmpv6/ICMPv6Message_m.h"
#include "inet/networklayer/ipv6mev/IdentificationHeader.h"
#include "inet/networklayer/ipv6mev/AddressManagement.h"
#include "inet/networklayer/common/InterfaceEntry.h"

namespace inet {

/**
 * TODO - Generated class
 */
class MobileAgent : public cListener, public Agent
{
    virtual ~MobileAgent();
  protected:
    IInterfaceTable *ift = nullptr; // for recognizing changes etc
    cModule *interfaceNotifier = nullptr; // listens for changes in interfacetable
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    class InterfaceUnit { // represents the entry of addressTable for interface up/down
    public:
        virtual ~InterfaceUnit() {};
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

    class LinkUnit { // a tuple to represent the quality of a link.
    public:
        virtual ~LinkUnit() {};
        double snir; // stored unit is something around 10^-12 W. Could be more or less.
        double per;
    };
    typedef std::map<MACAddress, LinkUnit *> LinkTable; // table of mac address. value contains link parameter
    LinkTable linkTable;

    class PacketTimer {
    public:
        virtual ~PacketTimer() {};
        cMessage *packet;
        int prot;
        int destPort;
        int sourcePort;
        IPv6Address destAddress;
        simtime_t nextScheduledTime;
    };
    class PacketTimerKey {
    public:
        virtual ~PacketTimerKey() {};
        int prot;
        int destPort;
        int sourcePort;
        IPv6Address destAddress;
        PacketTimerKey(int _prot, IPv6Address _destAddress, int _destPort, int _sourcePort) {
            prot = _prot;
            destPort = _destPort;
            sourcePort = _sourcePort;
            destAddress = _destAddress;
        }
        bool operator<(const PacketTimerKey& b) const {
            if(destAddress == b.destAddress) {
                if(destPort == b.destPort) {
                    if(sourcePort == b.sourcePort) {
                        return prot < b.prot;
                    } else
                        return sourcePort < b.sourcePort;
                } else
                    return destPort < b.destPort;
            } else
                return destAddress < b.destAddress;
        }
    };

    typedef std::map<PacketTimerKey, PacketTimer *> PacketQueue; // ProtocolType
    PacketQueue packetQueue;

    typedef std::map<long, int> PingMap;
    PingMap pingMap;
  public:
    //  AGENT MANAGEMENT
    void createSessionInit();
    void sendSessionInit(cMessage *msg); // send initialization message to CA
    void createSequenceInit();
    void sendSequenceInit(cMessage *msg); // for sending sequence msg to CA
    void createSequenceUpdate(uint64 mobileId, uint seq, uint ack);
    void sendSequenceUpdate(cMessage *msg);
    void createFlowRequest(FlowTuple &tuple);
    void sendFlowRequest(cMessage *msg);
//  PACKET PROCESSING INCOMING MESSAGES
    void processAgentMessage(IdentificationHeader *agentHeader, IPv6ControlInfo *ipCtrlInfo); // starting point of any receiving msg. assigns msg according header
    void performSessionInitResponse(IdentificationHeader *agentHeader, IPv6Address destAddr); // process response of CA when session initialization was triggered
    void performSequenceInitResponse(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void performFlowRequestResponse(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void performSequenceUpdateResponse(IdentificationHeader *agentHeader, IPv6Address destAddr);
    void performIncomingUdpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *ipCtrlInfo);
    void performIncomingTcpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *ipCtrlInfo);
    void processIncomingIcmpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *ipCtrlInfo);
    void processIncomingIcmpPacket(ICMPv6Message *icmp, IPv6ControlInfo *ipCtrlInfo);
//  PACKET PROCESSING OUTGOING MESSAGES
    void processOutgoingUdpPacket(cMessage *msg); // handles udp packet that is coming from upper layer
    void processOutgoingTcpPacket(cMessage *msg); // handles tcp packet that is coming from upper layer
    void processOutgoingIcmpPacket(cMessage *msg); // handles ping packets from ping application
    void sendUpperLayerPacket(cPacket *packet, IPv6ControlInfo *controlInfo, IPv6Address agentAddr, short prot);

//  INTERFACE LISTENER FUNCTIONS
    InterfaceUnit* getInterfaceUnit(int id); // returns the instance of the interfaceId for setting interface configuration
    void createInterfaceDownMessage(int id);
    void handleInterfaceDownMessage(cMessage *msg); // is called when interface gets disconnected. Changes are stored in interface map.
    void createInterfaceUpMessage(int id);
    void handleInterfaceUpMessage(cMessage *msg); // is called when interface gets connected. Changes are stored in interface map.
    void updateAddressTable(int id, InterfaceUnit *iu); // changing address map when node gets out of reachability. Is called by handleInterfaceUpMsg...
//  LISTENER FUNCTIONS: handling interface up/down
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, double d) override;
//  LINK CONFIGURATION
    LinkUnit* getLinkUnit(MACAddress mac); // returns the instance of LinkUnit for link configuration such as SNR, PER,..
    InterfaceEntry *getInterface(IPv6Address destAddr = IPv6Address::UNSPECIFIED_ADDRESS, int destPort = -1, int sourcePort = -1, short protocol = -1);
    bool isInterfaceUp();
//  PACKET PROCESSING
    void sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, simtime_t sendTime = 0); // resend after timer expired
//  PACKET QUEUEING
    void sendAllPacketsInQueue();
    PacketTimer *getPacketTimer(PacketTimerKey& key);
    bool isPacketTimerQueued(PacketTimerKey& key);
    void cancelPacketTimer(PacketTimerKey& key);
    void deletePacketTimer(PacketTimerKey& key);
    void deletePacketTimerEntry(PacketTimerKey& key);

};

} //namespace

#endif
