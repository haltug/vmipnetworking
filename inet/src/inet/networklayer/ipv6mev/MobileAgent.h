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
#include <deque>
#include "inet/common/INETDefs.h"
#include "inet/networklayer/contract/ipv6/IPv6Address.h"
#include "inet/networklayer/icmpv6/ICMPv6Message_m.h"
#include "inet/networklayer/ipv6mev/IdentificationHeader.h"
#include "inet/networklayer/common/InterfaceEntry.h"

namespace inet {

class MobileAgent : public cListener, public Agent
{
    virtual ~MobileAgent();
    bool isIdLayerEnabled;
    bool enableNodeRequesting;
  protected:
    IInterfaceTable *ift = nullptr; // for recognizing changes etc
    cModule *interfaceNotifier = nullptr; // listens for changes in interfacetable
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    class LinkUnit { // a tuple to represent the quality of a link.
    public:
        virtual ~LinkUnit() {};
        double snir; // stored unit is something around 10^-12 W. Could be more or less.
//        double per;
        simtime_t timestamp;
        LinkUnit(double _snir, simtime_t _timestamp) {
            snir = _snir;
//            per = _per;
            timestamp = _timestamp;
        }
    };
    typedef std::deque<LinkUnit *> LinkBuffer;
    LinkBuffer linkBuffer;
    typedef std::map<MACAddress,LinkBuffer *> LinkTable; // table of mac address. value contains link parameter
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

    // statistics
    static simsignal_t controlSignalLoad;
    static simsignal_t dataSignalLoad;
    static simsignal_t interfaceSnir;
    static simsignal_t interfaceId;
    static simsignal_t sequenceUpdateCa;
    static simsignal_t sequenceUpdateDa;
    static simsignal_t flowRequest;
    static simsignal_t flowRequestDelay;
    simtime_t flowRequestSignal;
    long sequenceUpdateCaStat = 0;
    long sequenceUpdateDaStat = 0;
    long flowRequestStat = 0;
    long flowResponseStat = 0;
    double interfaceSnirStat = 0;
    long interfaceIdStat = 0;

  public:
    //  AGENT MANAGEMENT
    void createSessionInit();
    void sendSessionInit(cMessage *msg); // send initialization message to CA
    void createSequenceInit();
    void sendSequenceInit(cMessage *msg); // for sending sequence msg to CA
    void createSequenceUpdate(uint64 mobileId, uint seq, uint ack);
    void sendSequenceUpdate(cMessage *msg);
    bool createFlowRequest(FlowTuple &tuple);
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
    void processInterfaceDown(int id);
    void handleInterfaceDown(cMessage *msg); // is called when interface gets disconnected. Changes are stored in interface map.
    void processInterfaceUp(int id);
    void handleInterfaceUp(cMessage *msg); // is called when interface gets connected. Changes are stored in interface map.
    void processInterfaceChange(int id);
    void handleInterfaceChange(cMessage *msg);

//  LISTENER FUNCTIONS: handling interface up/down
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, double d) override;

//  LINK CONFIGURATION
    LinkBuffer* getLinkBuffer(MACAddress mac);
    LinkBuffer* getLinkBuffer(InterfaceEntry *ie);
    void addLinkUnit(LinkBuffer* buffer, LinkUnit *unit);
    double getMeanSnir(InterfaceEntry *ie);
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
