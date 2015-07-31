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

#include "inet/networklayer/ipv6mev/MobileAgent.h"

#include <algorithm>
#include <stdlib.h>
#include "inet/common/ModuleAccess.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/common/IPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv6/IPv6ControlInfo.h"
#include "inet/networklayer/ipv6/IPv6Datagram.h"
#include "inet/networklayer/ipv6/IPv6ExtensionHeaders.h"
#include "inet/networklayer/ipv6/IPv6InterfaceData.h"
#include "inet/physicallayer/common/packetlevel/Radio.h"
#include "inet/transportlayer/udp/UDPPacket.h"
#include "inet/transportlayer/tcp_common/TCPSegment.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211Radio.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"

#include "inet/common/NotifierConsts.h"
#include "inet/common/lifecycle/NodeStatus.h"

namespace inet {

Define_Module(MobileAgent);

void MobileAgent::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER) {
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        sessionState = UNASSOCIATED;
        seqnoState = UNASSOCIATED;
        srand(123); // TODO must be changed
        agentId = (uint64) rand(); // TODO should be placed in future
        am.initiateAddressMap(agentId, 22);
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
        IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
        ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID);
        interfaceNotifier = getContainingNode(this);
        interfaceNotifier->subscribe(NF_INTERFACE_IPv6CONFIG_CHANGED,this);
        interfaceNotifier->subscribe(physicallayer::Radio::minSNIRSignal, this);
        interfaceNotifier->subscribe(physicallayer::Radio::packetErrorRateSignal, this);
    }
    if (stage == INITSTAGE_APPLICATION_LAYER) {
//        simtime_t startTime = par("startTime");
//        cMessage *timeoutMsg = new cMessage("sessionStart");
//        timeoutMsg->setKind(MSG_START_TIME);
//        scheduleAt(startTime, timeoutMsg); // delaying start
    }

    WATCH(agentId);
    WATCH(caAddress);
//    WATCH(isMA);
//    WATCH(isCA);
//    WATCHMAP(interfaceToIPv6AddressList);
//    WATCHMAP(directAddressList);
}


void MobileAgent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()) {
//        EV << "Self message received: " << msg->getKind() << endl;
        if(msg->getKind() == MSG_START_TIME) {
//            EV << "Starter msg received" << endl;
            createSessionInit();
            delete msg;
        }
        else if(msg->getKind() == MSG_SESSION_INIT) {
//            EV << "A: CA_init_msg received" << endl;
            sendSessionInit(msg);
        }
        else if(msg->getKind() == MSG_SEQNO_INIT) {
//            EV << "A: CA_init_msg received" << endl;
            sendSequenceInit(msg);
        }
        else if(msg->getKind() == MSG_IF_DOWN) {
//            EV << "MA: Interface down timer received" << endl;
            handleInterfaceDownMessage(msg);
        }
        else if(msg->getKind() == MSG_IF_UP) {
//            EV << "MA: Interface up timer received" << endl;
            handleInterfaceUpMessage(msg);
        }
        else if(msg->getKind() == MSG_SEQ_UPDATE) { // from
            sendSequenceUpdate(msg);
        }
        else if(msg->getKind() == MSG_SEQ_UPDATE_DELAYED) {
            SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
            uint64 id = sut->id;
            uint seq = sut->seq;
            uint ack = sut->ack;
            createSequenceUpdate(id, seq, ack);
            delete msg;
        }
        else if(msg->getKind() == MSG_FLOW_REQ) { // from MA
            sendFlowRequest(msg);
        }
        else if(msg->getKind() == MSG_UDP_RETRANSMIT) { // from MA
            processOutgoingUdpPacket(msg);
        }
        else if(msg->getKind() == MSG_TCP_RETRANSMIT) { // from MA
            processOutgoingTcpPacket(msg);
        }
        else if(msg->getKind() == MSG_INTERFACE_DELAY) { // from MA
            InterfaceInit *ii = (InterfaceInit *) msg->getContextPointer();
            updateAddressTable(ii->id, ii->iu);
            delete msg;
        }
        else {
            if(msg)
                EV << "ERROR_MA: kind: " << msg->getKind() << " className:" << msg->getClassName() << endl;
            throw cRuntimeError("handleMessage: Unknown timer expired. Which timer msg is unknown?");
        }
    }
    else if(msg->arrivedOn("fromUDP")) {
        processOutgoingUdpPacket(msg);
    }
    else if(msg->arrivedOn("fromTCP")) {
        processOutgoingTcpPacket(msg);
    }
    else if(msg->arrivedOn("fromLowerLayer")) {
        if (dynamic_cast<IdentificationHeader *> (msg)) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processAgentMessage( (IdentificationHeader *) msg, controlInfo);
        }
    }
    else
        throw cRuntimeError("A:handleMsg: cMessage Type not known. What did you send?");
}

void MobileAgent::createSessionInit() {
    if(sessionState == UNASSOCIATED)
        sessionState = INITIALIZING;
    L3Address caAddr;
    const char *controlAgentAddr;
    controlAgentAddr = par("controlAgentAddress");
    L3AddressResolver().tryResolve(controlAgentAddr, caAddr);
    caAddress = caAddr.toIPv6();
    if(caAddress.isGlobal()) {
//        EV << "MA: Create CA_init" << endl;
        cMessage *msg = new cMessage("sendingCAinit",MSG_SESSION_INIT);
        TimerKey key(caAddress,-1,TIMERKEY_SESSION_INIT);
        SessionInitTimer *sit = (SessionInitTimer *) getExpiryTimer(key,TIMERTYPE_SESSION_INIT);
        sit->dest = caAddress;
        sit->timer = msg;
        sit->ackTimeout = TIMEOUT_SESSION_INIT;
        sit->nextScheduledTime = simTime();
        msg->setContextPointer(sit);
        scheduleAt(simTime(),msg);
    } else {
        EV << "MA: Create CA_init is delayed." << endl;
        cMessage *timeoutMsg = new cMessage("sessionStart");
        timeoutMsg->setKind(MSG_START_TIME);
        scheduleAt(simTime()+TIMEOUT_SESSION_INIT, timeoutMsg);
    }
}

void MobileAgent::sendSessionInit(cMessage* msg) {
//    EV << "MA: Send CA_init" << endl;
    SessionInitTimer *sit = (SessionInitTimer *) msg->getContextPointer();
    const IPv6Address &dest = sit->dest;
    sit->nextScheduledTime = simTime() + sit->ackTimeout;
    sit->ackTimeout = (sit->ackTimeout)*2;
    if(sit->dest.isGlobal()) { // not necessary
        IdentificationHeader *ih = getAgentHeader(1, IP_PROT_NONE, 0, 0, agentId);
        ih->setIsIdInitialized(true);
        ih->setByteLength(SIZE_AGENT_HEADER);
        sendToLowerLayer(ih, dest);
    }
    scheduleAt(sit->nextScheduledTime, msg);
}

void MobileAgent::createSequenceInit() { // does not support interface check
//    EV << "MA: Create CA_seq_init" << endl;
    if(sessionState != ASSOCIATED)
        throw cRuntimeError("MA: Not registered at CA. Cannot run seq init.");
    if(seqnoState == UNASSOCIATED) {
        seqnoState = INITIALIZING;
        cMessage *msg = new cMessage("sendingCAseqInit", MSG_SEQNO_INIT);
        TimerKey key(caAddress,-1,TIMERKEY_SEQNO_INIT);
        SequenceInitTimer *sit = (SequenceInitTimer *) getExpiryTimer(key, TIMERTYPE_SEQNO_INIT);
        sit->dest = caAddress;
        sit->timer = msg;
        sit->ackTimeout = TIMEOUT_SEQNO_INIT;
        sit->nextScheduledTime = simTime();
        msg->setContextPointer(sit);
        scheduleAt(sit->nextScheduledTime, msg);
    }
}

void MobileAgent::sendSequenceInit(cMessage *msg) {
//    EV << "MA: Send Seq_init_to_CA" << endl;
    SequenceInitTimer *sit = (SequenceInitTimer *) msg->getContextPointer();
    const IPv6Address &dest = sit->dest;
    InterfaceEntry *ie = getInterface();
    if(!ie)
        throw cRuntimeError("MA: no interface provided in sendSeqInit.");
    sit->nextScheduledTime = simTime() + sit->ackTimeout;
    sit->ackTimeout = (sit->ackTimeout)*2;
    IdentificationHeader *ih = getAgentHeader(1, IP_PROT_NONE, am.getSeqNo(agentId), 0, agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsIpModified(true);
    ih->setIpAddingField(1);
    ih->setIpRemovingField(0);
    ih->setIpAcknowledgementNumber(0);
    ih->setIPaddressesArraySize(1);
    ih->setIPaddresses(0,ie->ipv6Data()->getPreferredAddress());
    ih->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR);
    sendToLowerLayer(ih, dest);
    scheduleAt(sit->nextScheduledTime, msg);
}

void MobileAgent::createSequenceUpdate(uint64 mobileId, uint seq, uint ack) {
//    EV << "MA: Create sequence update to CA" << endl;
    if(sessionState != ASSOCIATED &&  seqnoState != ASSOCIATED)
        throw cRuntimeError("MA: Not registered at CA. Cannot run seq init.");
    InterfaceEntry *ie = getInterface(caAddress);
    TimerKey key(caAddress, -1, TIMERKEY_SEQ_UPDATE, mobileId, seq);
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) getExpiryTimer(key, TIMERTYPE_SEQ_UPDATE);
    sut->dest = caAddress;
    sut->ackTimeout = TIMEOUT_SEQ_UPDATE;
    sut->nextScheduledTime = simTime();
    sut->seq = seq;
    sut->ack = ack;
    sut->id = mobileId;
    sut->ie = ie;
    if(!ie) {
        EV << "MA: Delaying seq update. no interface provided." << endl;
        cMessage *msg = new cMessage("sequenceDelay", MSG_SEQ_UPDATE_DELAYED);
        sut->timer = msg;
        msg->setContextPointer(sut);
        scheduleAt(simTime()+TIMEOUT_SEQ_UPDATE, msg);
    } else {
//        EV << "MA: Sending seq update." << endl;
        cMessage *msg = new cMessage("sendingCAseqUpdate", MSG_SEQ_UPDATE);
        sut->timer = msg;
        msg->setContextPointer(sut);
        scheduleAt(sut->nextScheduledTime, msg);
    }
}

void MobileAgent::sendSequenceUpdate(cMessage* msg) {
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
    const IPv6Address &dest =  sut->dest;
    sut->nextScheduledTime = simTime() + sut->ackTimeout;
    sut->ackTimeout = (sut->ackTimeout)*1.5;
    IdentificationHeader *ih = getAgentHeader(1, IP_PROT_NONE, am.getSeqNo(agentId), am.getAckNo(agentId), agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsAckValid(true);
    ih->setIsIpModified(true);
    int seq = am.getSeqNo(agentId);
    int ack = am.getAckNo(agentId);
    EV << "MA: Send SeqUpdate for" << " s:" << seq << " a:" << ack << endl;
    AddressManagement::AddressChange ac = am.getAddressChange(agentId,sut->ack,sut->seq);
    ih->setIPaddressesArraySize(ac.addedAddresses+ac.removedAddresses);
    ih->setIpAddingField(ac.addedAddresses);
    if(ac.addedAddresses > 0) {
        if(ac.addedAddresses != ac.getAddedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqTcp: value of Add list must have size of integer.");
        for(int i=0; i<ac.addedAddresses; i++) {
            ih->setIPaddresses(i,ac.getAddedIPv6AddressList.at(i));
        }
    }
    ih->setIpRemovingField(ac.removedAddresses);
    if(ac.removedAddresses > 0) {
        if(ac.removedAddresses != ac.getRemovedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqTcp: value of Rem list must have size of integer.");
        for(int i=0; i<ac.removedAddresses; i++) {
            ih->setIPaddresses(i+ac.addedAddresses,ac.getRemovedIPv6AddressList.at(i));
        }
    }
    ih->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*(ac.addedAddresses+ac.removedAddresses)));
    sendToLowerLayer(ih, dest); // TODO select interface and remove delay
    scheduleAt(sut->nextScheduledTime, msg);
}

void MobileAgent::createFlowRequest(FlowTuple &tuple) {
    if(sessionState != ASSOCIATED &&  seqnoState != ASSOCIATED)
        throw cRuntimeError("MA: Not registered at CA. Cannot run seq init.");
    cMessage *msg = new cMessage("sendingCAflowReq", MSG_FLOW_REQ);
    TimerKey key(caAddress, -1, TIMERKEY_FLOW_REQ);
    FlowRequestTimer *frt = (FlowRequestTimer *) getExpiryTimer(key, TIMERTYPE_FLOW_REQ);
    FlowUnit *funit = getFlowUnit(tuple);
    if(funit->state != REGISTERING)
        throw cRuntimeError("MA: No match for incoming TCP packet");
    frt->dest = caAddress;
    frt->tuple = tuple;
    frt->timer = msg;
    frt->ackTimeout = TIMEDELAY_FLOW_REQ;
    frt->nodeAddress = funit->nodeAddress;
    InterfaceEntry *ie = getInterface(caAddress);
    if(!ie) {
        EV << "MA: Delaying flow request. no interface provided." << endl;
        frt->ie = nullptr;
        frt->nextScheduledTime = simTime()+TIMEDELAY_FLOW_REQ;
    } else {
        frt->ie = ie;
        frt->nextScheduledTime = simTime();
    }
    msg->setContextPointer(frt);
    scheduleAt(frt->nextScheduledTime, msg);
}

void MobileAgent::sendFlowRequest(cMessage *msg) {
    FlowRequestTimer *frt = (FlowRequestTimer *) msg->getContextPointer();
    frt->ackTimeout = (frt->ackTimeout)*2;
    frt->nextScheduledTime = simTime()+frt->ackTimeout;
    if(!frt->ie) { // if interface is provided, send message, else delay transmission
        frt->ie = getInterface(frt->dest); // check if interface is now provided
        if(!frt->ie) { // no interface yet
            scheduleAt(frt->nextScheduledTime, msg);
            return;
        }
    }
    EV << "MA: Sending flow request." << endl;
    const IPv6Address nodeAddress = frt->nodeAddress;
    const IPv6Address &dest =  frt->dest;
    IdentificationHeader *ih = getAgentHeader(1, IP_PROT_NONE, am.getSeqNo(agentId), am.getAckNo(agentId), agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsAckValid(true);
//    ih->setIsIpModified(false); // this signalizes that
    ih->setIsWithNodeAddr(true);
    ih->setIPaddressesArraySize(1);
    ih->setIPaddresses(0,nodeAddress);
    ih->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR);
    sendToLowerLayer(ih, dest);
    scheduleAt(frt->nextScheduledTime, msg);
}

void MobileAgent::processAgentMessage(IdentificationHeader *agentHeader, IPv6ControlInfo *controlInfo)
{
    IPv6Address destAddr = controlInfo->getSourceAddress().toIPv6();
    if(agentHeader->getIsControlAgent() && agentHeader->getNextHeader() == IP_PROT_NONE) {
        if(sessionState == INITIALIZING && agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && !agentHeader->getIsSeqValid())
            performSessionInitResponse(agentHeader, destAddr);
        else if (sessionState == ASSOCIATED && seqnoState == INITIALIZING && agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid())
            performSequenceInitResponse(agentHeader,destAddr);
        else if (sessionState == ASSOCIATED && seqnoState == ASSOCIATED && agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid()) {
            if(agentHeader->getIsWithAgentAddr() && agentHeader->getIsWithNodeAddr()) {
                EV << "MA: received flow response. start message processing." << endl;
                performFlowRequestResponse(agentHeader, destAddr);
            } else
                performSequenceUpdateResponse(agentHeader, destAddr);
        }
    } else if(agentHeader->getIsDataAgent() && agentHeader->getNextHeader() == IP_PROT_UDP && agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid())
        performIncomingUdpPacket(agentHeader, controlInfo);
    else if (agentHeader->getIsDataAgent() && agentHeader->getNextHeader() == IP_PROT_TCP && agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid())
        performIncomingTcpPacket(agentHeader, controlInfo);
    else
        throw cRuntimeError("MA: Header parameter is wrong. Received messagt cannot be assigned to any function.");
    delete agentHeader;
    delete controlInfo;
}

void MobileAgent::performSessionInitResponse(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(agentHeader->getByteLength() == SIZE_AGENT_HEADER) {
        if(caAddress != destAddr) // just for the beginning
            throw cRuntimeError("MA: Received message should be from CA, but it is received from somewhere else or caAddress is set wrongly.");
        sessionState = ASSOCIATED;
        cancelAndDeleteExpiryTimer(caAddress,-1, TIMERKEY_SESSION_INIT);
        EV << "MA: Received CA ack. Removed session timer. Session Init finished. Starting SeqInit." << endl;
        createSequenceInit();
    } else {
        throw cRuntimeError("MA: Byte length does not match the expected size. SessionInitResponse.");
    }
}

void MobileAgent::performSequenceInitResponse(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(agentHeader->getByteLength() == SIZE_AGENT_HEADER) {
        seqnoState = ASSOCIATED;
        am.setAckNo(agentId, agentHeader->getIpSequenceNumber()); // ack is set the first time
        int seq = am.getSeqNo(agentId);
        int ack = am.getAckNo(agentId);
        cancelAndDeleteExpiryTimer(caAddress,-1, TIMERKEY_SEQNO_INIT);
        EV << "MA: Received SeqNo Ack. Removed timer. SeqNo initialized." << " s:" << seq << " a:" << ack << endl;
    } else {
        throw cRuntimeError("MA: Byte length does not match the expected size.SequenceInitResponse");
    }
}


void MobileAgent::performFlowRequestResponse(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(agentHeader->getByteLength() == (SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR*2)) {
        IPv6Address node = agentHeader->getIPaddresses(0);
        IPv6Address agent = agentHeader->getIPaddresses(1);
        addressAssociation.insert(std::make_pair(node,agent));
        cancelAndDeleteExpiryTimer(caAddress,-1, TIMERKEY_FLOW_REQ);
        sendAllPacketsInQueue();
        EV << "MA: Flow request responsed by CA. Request process finished." << endl;
    } else {
        throw cRuntimeError("MA: Byte length does not match the expected size.FlowRequest");
    }
}

void MobileAgent::performSequenceUpdateResponse(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(agentHeader->getByteLength() == SIZE_AGENT_HEADER) {
        if(agentHeader->getIpSequenceNumber() > am.getAckNo(agentId)) {
            cancelAndDeleteExpiryTimer(caAddress,-1, TIMERKEY_SEQ_UPDATE, agentId, agentHeader->getIpSequenceNumber(), am.getAckNo(agentId));
            am.setAckNo(agentId, agentHeader->getIpSequenceNumber());
//            EV << "MA: Received update acknowledgment from CA. Removed timer." << endl;
        } else {
//            EV << "MA: Received update acknowledgment from CA contains older sequence value. Dropping message." << endl;
        }
    } else {
        throw cRuntimeError("MA: Byte length does not match the expected size.");
    }
}

void MobileAgent::performIncomingUdpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *controlInfo)
{
    cPacket *packet = agentHeader->decapsulate();
    if (dynamic_cast<UDPPacket *>(packet) != nullptr) {
        UDPPacket *udpPacket =  (UDPPacket *) packet;
        FlowTuple tuple;
        tuple.protocol = IP_PROT_UDP;
        tuple.destPort = udpPacket->getSourcePort();
        tuple.sourcePort = udpPacket->getDestinationPort();
        tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
        tuple.interfaceId = agentHeader->getId();
        FlowUnit *funit = getFlowUnit(tuple);
        if(funit->state == UNREGISTERED) {
            throw cRuntimeError("MA: No match for incoming UDP packet");
        } else {
            funit->state = REGISTERED;
        }
        IPv6Address fakeIp;
        fakeIp.set(0x1D << 24, 0, (agentId >> 32) & 0xFFFFFFFF, agentId & 0xFFFFFFFF);
//        EV << "MA:IPv6: " << fakeIp.str() << endl;
        IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
        ipControlInfo->setProtocol(IP_PROT_UDP);
        ipControlInfo->setDestAddr(fakeIp);
        ipControlInfo->setSrcAddr(funit->nodeAddress);
        ipControlInfo->setInterfaceId(100);
        ipControlInfo->setHopLimit(controlInfo->getHopLimit());
        ipControlInfo->setTrafficClass(controlInfo->getTrafficClass());
        udpPacket->setControlInfo(ipControlInfo);
        cGate *outgate = gate("toUDP");
//        EV << "to_UDP: Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " If=" << controlInfo->getInterfaceId() << " Pkt=" << udpPacket->getByteLength() << endl;
        send(udpPacket, outgate);
    } else
        throw cRuntimeError("MA:procDAmsg: UDP packet could not be cast.");
}

void MobileAgent::performIncomingTcpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *controlInfo)
{
//    EV << "MA: TCP packet received. Sending up ";
    cPacket *packet = agentHeader->decapsulate();
    if (dynamic_cast<tcp::TCPSegment *>(packet) != nullptr) {
        tcp::TCPSegment *tcpseg =  (tcp::TCPSegment *) packet;
//        IPv6Address agentAddress = controlInfo->getSourceAddress().toIPv6(); // the address of data agent can be used as an assignment parameter
        FlowTuple tuple;
        tuple.protocol = IP_PROT_TCP;
        tuple.destPort = tcpseg->getSourcePort();
        tuple.sourcePort = tcpseg->getDestinationPort();
        tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
        tuple.interfaceId = agentHeader->getId();
        FlowUnit *funit = getFlowUnit(tuple);
        if(funit->state == UNREGISTERED) {
            throw cRuntimeError("MA: No match for incoming TCP packet");
        } else {
            funit->state = REGISTERED;
        }
        IPv6Address fakeIp;
        fakeIp.set(0x1D << 24, 0, (agentId >> 32) & 0xFFFFFFFF, agentId & 0xFFFFFFFF);
//        EV << "MA:IPv6: " << fakeIp.str() << endl;
        IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
        ipControlInfo->setProtocol(IP_PROT_TCP);
        ipControlInfo->setDestAddr(fakeIp);
        ipControlInfo->setSrcAddr(funit->nodeAddress);
        ipControlInfo->setInterfaceId(101);
        ipControlInfo->setHopLimit(controlInfo->getHopLimit());
        ipControlInfo->setTrafficClass(controlInfo->getTrafficClass());
        tcpseg->setControlInfo(ipControlInfo);
        cGate *outgate = gate("toTCP");
        EV << "MA: <<==== TCP:" << " Pkt=" << tcpseg->getByteLength() << " Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " If=" << controlInfo->getInterfaceId() << endl;
        send(tcpseg, outgate);
    } else
        throw cRuntimeError("MA:procDAmsg: TCP packet could not be cast.");
}

void MobileAgent::processOutgoingUdpPacket(cMessage *msg)
{
    cObject *ctrl = msg->removeControlInfo();
    if (dynamic_cast<IPv6ControlInfo *>(ctrl) != nullptr) {
        IPv6ControlInfo *controlInfo = (IPv6ControlInfo *) ctrl;
        if (dynamic_cast<UDPPacket *>(msg) != nullptr) {
    //        EV << "MA:udpIn: processsing udp pkt." << endl;
            UDPPacket *udpPacket =  (UDPPacket *) msg;
            FlowTuple tuple;
            tuple.protocol = IP_PROT_UDP;
            tuple.destPort = udpPacket->getDestinationPort();
            tuple.sourcePort = udpPacket->getSourcePort();
            tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
            tuple.interfaceId = controlInfo->getDestinationAddress().toIPv6().getInterfaceId();
            FlowUnit *funit = getFlowUnit(tuple);
            if(funit->state == UNREGISTERED) {
                funit->lifetime = MAX_PKT_LIFETIME;
                funit->state = REGISTERING;
                funit->nodeAddress = controlInfo->getDestinationAddress().toIPv6();
                createFlowRequest(tuple); // invoking flow initialization and delaying pkt by sending in queue of this object (hint: scheduleAt with delay)
                udpPacket->setKind(MSG_UDP_RETRANSMIT);
                udpPacket->setControlInfo(controlInfo); // appending own
                EV << "MA:udpIn: delaying process, creating flow request." << endl;
                scheduleAt(simTime()+TIMEDELAY_PKT_PROCESS, udpPacket);
                return;
            } else if(funit->state == REGISTERING) {
                EV << "MA:udpIn starting registering process of first udp packet: "<< funit->lifetime << endl;
                funit->lifetime--;
                if(funit->lifetime < 1) {
                    EV << "MA:udpIn: deleting udp packet because of exceeding lifetime." << endl;
                    delete msg;
                    delete controlInfo;
                    return;
                } // TODO discarding all packets of tuple
                if(isAddressAssociated(controlInfo->getDestinationAddress().toIPv6())) {
                    EV << "MA:udpIn: creating address association. only done one time actually." << endl;
                    funit->lifetime = MAX_PKT_LIFETIME;
                    IPv6Address *ag = getAssociatedAddress(controlInfo->getDestinationAddress().toIPv6());
                    if(ag) {
                        EV << "MA:udpIn: creating flow unit." << endl;
                        funit->state = REGISTERED;
                        funit->isFlowActive = true;
                        funit->isAddressCached = true;
                        funit->dataAgent = *ag;
                        funit->nodeAddress = controlInfo->getDestinationAddress().toIPv6();
                        funit->id = agentId;
                    } else {
                        throw cRuntimeError("MA:UDPin: getAssociatedAddr fetched empty pointer instead of address.");
                    }
                } else {
                    EV << "MA:udpIn sending udp packet in queue, such that it can be processed in next round" << endl;
                    udpPacket->setKind(MSG_UDP_RETRANSMIT);
                    udpPacket->setControlInfo(controlInfo); // appending own
                    scheduleAt(simTime()+TIMEDELAY_PKT_PROCESS, udpPacket);
                    return;
                }
            } else
                if(funit->state == REGISTERED) {
                funit->lifetime = MAX_PKT_LIFETIME;
                // what should here be done?
                } else { throw cRuntimeError("MA:UDPin: Not known state of flow. What should be done?"); }
    //        EV << "MA:udpIn: flow is registered. starting sending process." << endl;
            sendUpperLayerPacket(udpPacket, controlInfo, funit->dataAgent, IP_PROT_UDP);
        } else
            throw cRuntimeError("MA: Incoming message should be UPD packet.");
    }
}

void MobileAgent::processOutgoingTcpPacket(cMessage *msg)
{
    //        EV << "MA:tcpIn: processing tcp pkt. Node: " << controlInfo->getDestinationAddress().toIPv6() << endl;
    cObject *controlInfoPtr = msg->removeControlInfo();
    if (dynamic_cast<IPv6ControlInfo *>(controlInfoPtr) != nullptr)
    {
        IPv6ControlInfo *controlInfo = (IPv6ControlInfo *) controlInfoPtr;
        if (dynamic_cast<tcp::TCPSegment *>(msg) != nullptr)
        {
//            EV << "MA:--------> TCP PROCESSING" << endl;
            tcp::TCPSegment *tcpseg = (tcp::TCPSegment *) msg;
            PacketTimerKey packet(IP_PROT_TCP,controlInfo->getDestinationAddress().toIPv6(),tcpseg->getDestinationPort(),tcpseg->getSourcePort());
            if(isInterfaceUp()) {
                FlowTuple tuple;
                tuple.protocol = IP_PROT_TCP;
                tuple.destPort = tcpseg->getDestinationPort();
                tuple.sourcePort = tcpseg->getSourcePort();
                tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
                tuple.interfaceId = controlInfo->getDestinationAddress().toIPv6().getInterfaceId();
                FlowUnit *funit = getFlowUnit(tuple);
                if(funit->state == UNREGISTERED) {
                    EV << "MA:==> Unregistered flow detected. Creating flow request." << endl;
                    EV << "MA:==> tuple: IP= " << controlInfo->getDestinationAddress().toIPv6() << " Pd=" << tcpseg->getDestinationPort() << " Ps=" << tcpseg->getSourcePort() << " Id=" << controlInfo->getDestinationAddress().toIPv6().getInterfaceId() << endl;
                    funit->state = REGISTERING;
                    funit->nodeAddress = controlInfo->getDestinationAddress().toIPv6();
                    createFlowRequest(tuple); // invoking flow initialization and delaying pkt by sending in queue of this object (hint: scheduleAt with delay)
                    PacketTimer *pkt = getPacketTimer(packet); // set packet in queue
                    pkt->packet = msg; // replaced by new packet
                    pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    msg->setKind(MSG_TCP_RETRANSMIT);
                    msg->setControlInfo(controlInfo);
                    msg->setContextPointer(pkt);
                    scheduleAt(pkt->nextScheduledTime, msg);
                } else if(funit->state == REGISTERING) {
                    EV << "MA:==> Registering flow. Checking for response." << endl;
                    if(isAddressAssociated(controlInfo->getDestinationAddress().toIPv6())) {
                        IPv6Address *ag = getAssociatedAddress(controlInfo->getDestinationAddress().toIPv6());
                        if(!ag)
                            throw cRuntimeError("MA:tcpIn: getAssociatedAddr fetched empty pointer instead of address.");
                        EV << "MA:==> flow response received. Restarting TCP process with associated agent=" << ag->str() << " node=" <<  controlInfo->getDestinationAddress().toIPv6().str() << endl;
                        funit->state = REGISTERED;
                        funit->isFlowActive = true;
                        funit->isAddressCached = true;
                        funit->dataAgent = *ag;
                        funit->nodeAddress = controlInfo->getDestinationAddress().toIPv6();
                        funit->id = agentId;
                        PacketTimer *pkt = getPacketTimer(packet);
                        pkt->nextScheduledTime = simTime(); // restart tcp processing
                        msg->setControlInfo(controlInfo);
                        scheduleAt(pkt->nextScheduledTime, msg);
                    } else
                    { // if code is here, then flow process is not finished
                        EV << "MA:==> Flow response not yet received. Waiting for response by CA." << endl;
                        if(isPacketTimerQueued(packet)) { // check is packet is in queue
                            PacketTimer *pkt = getPacketTimer(packet);
                            if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                                EV << "MA:==> new message from upper layer arrived. Replacing message and setting scheduling time." << endl;
                                cancelPacketTimer(packet); // packet is in queue, cancel scheduling
                                delete pkt->packet; // old packet is...
                                pkt->packet = msg; // replaced by new packet
                                pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                            } else {
                                EV << "MA:==> processing same message (not new msg from upper layer). Just setting scheduling time." << endl;
                                pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                            }
                            msg->setControlInfo(controlInfo);
                            scheduleAt(pkt->nextScheduledTime+1, msg);
                        } else
                            throw cRuntimeError("MA:tcpIn: Message is not in queue during phase of flow registering.");
                    }
                } else if(funit->state == REGISTERED) {
//                    EV << "MA:==> registered flow. preparing transmission." << endl;
                    if(isPacketTimerQueued(packet)) { // check is packet is in queue
                        PacketTimer *pkt = getPacketTimer(packet); // pop the relating packet from queue
                        if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                            EV << "MA:=> Deleting older packet. ";
                            deletePacketTimer(packet); // deletes older message and removes the key from the queue.
                        } else {
                            EV << "MA:=> Deleting entry in queue as this message is same to previous one and arrived at time:"<< msg->getCreationTime() << " ";
                            deletePacketTimerEntry(packet); // removes just from the queue but does not delete message
                        }
                    } else {
                        EV << "MA:=> New packet from upper layer arrived. No packet in queue. ";
                    }
                    EV << "Sending TCP packet ";
                    sendUpperLayerPacket(tcpseg, controlInfo, funit->dataAgent, IP_PROT_TCP);
                } else
                    throw cRuntimeError("MA:tcpIn: Not known state of flow. What should be done?");
            } else
            { // interface is down, so schedule for later transmission
                if(isPacketTimerQueued(packet)) { // check if packet is in queue
                    PacketTimer *pkt = getPacketTimer(packet); // packet is cancelled
                    if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                        EV << "MA:==> Interface is down and a packet is in queue. Replacing new message with older one." << endl;
                        cancelPacketTimer(packet); // packet is in queue, cancel scheduling
                        delete pkt->packet; // old packet is...
                        pkt->packet = msg; // replaced by new packet
                        pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    } else {
                        EV << "MA:==> Interface is down and this packet is already in queue. Setting new schedule time." << endl;
                        pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    }
                    msg->setKind(MSG_TCP_RETRANSMIT);
                    msg->setContextPointer(pkt);
                    msg->setControlInfo(controlInfo);
                    scheduleAt(pkt->nextScheduledTime, msg);
                } else { // first packet of socket
                    EV << "MA:==> Interface is down and (first) packet of flow is inserted in queue." << endl;
                    PacketTimer *pkt = getPacketTimer(packet); // inserting packet into queue
                    pkt->packet = msg;
                    pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    msg->setKind(MSG_TCP_RETRANSMIT);
                    msg->setContextPointer(pkt);
                    msg->setControlInfo(controlInfo);
                    scheduleAt(pkt->nextScheduledTime, msg);
                }
            }
        } else
            throw cRuntimeError("MA: Incoming message could not be casted to a TCP packet.");
    } else
        EV << "MA: ControlInfo could not be extracted from TCP packet." << endl;
}

void MobileAgent::sendUpperLayerPacket(cPacket *packet, IPv6ControlInfo *controlInfo, IPv6Address agentAddr, short prot)
{
//        EV << "MA:In: Flow is registered. starting sending process." << endl;
    IdentificationHeader *ih = getAgentHeader(1, prot, am.getSeqNo(agentId), am.getAckNo(agentId), agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsAckValid(true);
    ih->setIsWithNodeAddr(true);
    if(am.getSeqNo(agentId) != am.getAckNo(agentId)) {
        ih->setIsIpModified(true);
//            EV << "MA: SENDING PACKET WITH IP CHANGES! s:" << am.getSeqNo(agentId) << " a:" << am.getAckNo(agentId) << endl;
    }
    AddressManagement::AddressChange ac = am.getAddressChange(agentId,am.getAckNo(agentId),am.getSeqNo(agentId));
    ih->setIpAddingField(ac.addedAddresses);
    ih->setIPaddressesArraySize(1+ac.addedAddresses+ac.removedAddresses);
    ih->setIPaddresses(0,controlInfo->getDestinationAddress().toIPv6());
    if(ac.addedAddresses > 0) {
        if(ac.addedAddresses != ac.getAddedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqTcp: value of Add list must have size of integer.");
        for(int i=0; i<ac.addedAddresses; i++) {
            ih->setIPaddresses(i+1,ac.getAddedIPv6AddressList.at(i));
        }
    }
    ih->setIpRemovingField(ac.removedAddresses);
    if(ac.removedAddresses > 0) {
        if(ac.removedAddresses != ac.getRemovedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqTcp: value of Rem list must have size of integer.");
        for(int i=0; i<ac.removedAddresses; i++) {
            ih->setIPaddresses(i+1+ac.addedAddresses,ac.getRemovedIPv6AddressList.at(i));
        }
    }
    ih->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*(ac.addedAddresses+ac.removedAddresses+1)));
    ih->encapsulate(packet);
    controlInfo->setProtocol(IP_PROT_IPv6EXT_ID);
    controlInfo->setDestinationAddress(agentAddr);
    // Set here scheduler configuration
    InterfaceEntry *ie = getInterface(controlInfo->getDestinationAddress().toIPv6());
    controlInfo->setInterfaceId(ie->getInterfaceId()); // just override existing entries
    controlInfo->setSourceAddress(ie->ipv6Data()->getPreferredAddress());
    ih->setControlInfo(controlInfo); // make copy before setting param
    cGate *outgate = gate("toLowerLayer");
        LinkUnit *lu = getLinkUnit(ie->getMacAddress());
        EV << "to IP: DA=" << controlInfo->getDestAddr() << " MA=" << controlInfo->getSrcAddr() << " Pkt=" << ih->getByteLength() << " IF=" << ie->getInterfaceId() << " SNIR=" << lu->snir << endl;
    send(ih, outgate);
}

MobileAgent::InterfaceUnit *MobileAgent::getInterfaceUnit(int id)
{
    InterfaceUnit *iu;
    auto it = addressTable.find(id);
    if(it != addressTable.end()) { // addressTable contains an instance of interfaceUnit
        iu = it->second;
        return iu;
    } else {
        iu = new InterfaceUnit();
        iu->active = false;
        iu->priority = -1;
        return iu;
    }
}

void MobileAgent::createInterfaceDownMessage(int id)
{
    IPv6Address ip;
    if(!cancelAndDeleteExpiryTimer(ip.UNSPECIFIED_ADDRESS, id,TIMERKEY_IF_DOWN)) // no l2 disassociation timer exists
    {
        TimerKey key(ip.UNSPECIFIED_ADDRESS,id,TIMERKEY_IF_DOWN);
        InterfaceDownTimer *l2dt = (InterfaceDownTimer*) getExpiryTimer(key, TIMERTYPE_IF_DOWN);
        int i;
        for (i=0; i<ift->getNumInterfaces(); i++) {
            if(ift->getInterface(i)->getInterfaceId() == id) break;
        }
        cMessage *msg = new cMessage("InterfaceDownTimer", MSG_IF_DOWN);
        l2dt->ie = ift->getInterface(i);
        l2dt->timer = msg;
        l2dt->nextScheduledTime = simTime()+TIMEDELAY_IF_DOWN;
        msg->setContextPointer(l2dt);
        scheduleAt(l2dt->nextScheduledTime, msg);
    }
    EV << "MA: Create interface down timer message: " << id << endl;
}

void MobileAgent::handleInterfaceDownMessage(cMessage *msg)
{
    IPv6Address ip;
    InterfaceDownTimer *l2dt = (InterfaceDownTimer *) msg->getContextPointer();
    InterfaceEntry *ie = l2dt->ie;
    InterfaceUnit *iu = getInterfaceUnit(ie->getInterfaceId());
    iu->active = false;
    iu->priority = -1;
    EV << "MA: handle interface down message. Addr: " << iu->careOfAddress.str() << endl;
    updateAddressTable(ie->getInterfaceId(), iu);
    cancelExpiryTimer(ip.UNSPECIFIED_ADDRESS,ie->getInterfaceId(),TIMERKEY_IF_DOWN);
    delete msg;
}

void MobileAgent::createInterfaceUpMessage(int id)
{
    IPv6Address ip;
    if(!cancelAndDeleteExpiryTimer(ip.UNSPECIFIED_ADDRESS, id,TIMERKEY_IF_DOWN)) // no l2 disassociation timer exists
    {
        TimerKey key(ip.UNSPECIFIED_ADDRESS, id, TIMERKEY_IF_UP);
        InterfaceUpTimer *l2at = new InterfaceUpTimer();
        int i;
        for (i=0; i<ift->getNumInterfaces(); i++) {
            if(ift->getInterface(i)->getInterfaceId() == id) break;
        }
        cMessage *msg = new cMessage("L2AssociaitonTimer", MSG_IF_UP);
        l2at->ie = ift->getInterface(i);
        l2at->timer = msg;
        l2at->nextScheduledTime = simTime()+TIMEDELAY_IF_UP;
        msg->setContextPointer(l2at);
        scheduleAt(l2at->nextScheduledTime, msg);
    } else {
        EV << "Timer was configured. Is deleted." << endl;
    }
    EV << "MA: create interface up timer message" << endl;
}

void MobileAgent::handleInterfaceUpMessage(cMessage *msg)
{
    IPv6Address ip;
    InterfaceUpTimer *l2at = (InterfaceUpTimer *) msg->getContextPointer();
    InterfaceEntry *ie = l2at->ie;
    InterfaceUnit *iu = getInterfaceUnit(ie->getInterfaceId());
    iu->active = true;
    iu->priority = 0;
    iu->careOfAddress = ie->ipv6Data()->getPreferredAddress();
    EV << "MA: Handle interface up message. Addr: " << iu->careOfAddress.str() << endl;
    updateAddressTable(ie->getInterfaceId(), iu);
    cancelExpiryTimer(ip.UNSPECIFIED_ADDRESS,ie->getInterfaceId(),TIMERKEY_IF_UP);
    delete msg;
}

void MobileAgent::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    Enter_Method_Silent();
    if(signalID == NF_INTERFACE_IPv6CONFIG_CHANGED) { // is triggered when carrier setting is changed
        if(dynamic_cast<InterfaceEntryChangeDetails *>(obj)) {
            InterfaceEntry *ie = check_and_cast<const InterfaceEntryChangeDetails *>(obj)->getInterfaceEntry();
            if(ie->isUp())
                createInterfaceUpMessage(ie->getInterfaceId());
            else
                createInterfaceDownMessage(ie->getInterfaceId());
        }
    EV << "-------- RECEIVED SIGNAL ---------" << endl;
    }
}

void MobileAgent::receiveSignal(cComponent *source, simsignal_t signalID, double d)
{
    Enter_Method_Silent();
    if (dynamic_cast<physicallayer::Ieee80211Radio *>(source) != nullptr) {
        physicallayer::Ieee80211Radio *radio = (physicallayer::Ieee80211Radio *) source;
        if(radio) {
            ieee80211::Ieee80211Mac *mac = (ieee80211::Ieee80211Mac *) radio->getParentModule()->getSubmodule("mac");
            if(mac) {
                LinkUnit *lu = getLinkUnit(mac->getMacAddress());
                if(signalID == physicallayer::Radio::minSNIRSignal) {
                    lu->snir = d;
            //        EV << "MA: SNR=" << d << " MAC= " << mac->getMacAddress() << endl;
                }
                if(signalID == physicallayer::Radio::packetErrorRateSignal) {
                    LinkUnit *lu = getLinkUnit(mac->getMacAddress());
                    lu->per = d;
            //        EV << "MA: PER=" << d << " MAC= " << mac->getMacAddress() << endl;
                }
            }
        }
    }
//    if(radio) {
//        ieee80211::Ieee80211Mac *mac = (ieee80211::Ieee80211Mac *) radio->getParentModule()->getSubmodule("mac");
//        if(mac) {
//        }
//        ieee80211::Ieee80211MgmtSTA *mgmt = (ieee80211::Ieee80211MgmtSTA *) radio->getParentModule()->getSubmodule("mgmt");
//        if(mgmt) {
//        }
//    }
}


void MobileAgent::updateAddressTable(int id, InterfaceUnit *iu)
{
    if(seqnoState == ASSOCIATED) {
        auto it = addressTable.find(id);
        if(it != addressTable.end()) { // updating interface
            if(it->first != id) throw cRuntimeError("ERROR in updateAddressTable: provided id should be same with entry");
            (it->second)->active = iu->active;
            (it->second)->priority = iu->priority;
            (it->second)->careOfAddress = iu->careOfAddress;
            if(iu->active) {    // presents an interface that has been associated
                am.addIpToMap(agentId, iu->careOfAddress);
                sendAllPacketsInQueue();
            } else { // presents an interface has been disassociated
                am.removeIpFromMap(agentId, iu->careOfAddress);
            }
            // *** for output
            int seq = am.getSeqNo(agentId);
            int ack = am.getAckNo(agentId);
            if(iu->active) {    // presents an interface that has been associated
                EV << "MA: Adding CoA-IP: " << iu->careOfAddress << " s:" << seq << " a:" << ack << endl;
            } else { // presents an interface has been disassociated
                EV << "MA: Removing CoA-IP: " << iu->careOfAddress << " s:" << seq << " a:" << ack << endl;
            }
            // ***
        } else { // adding interface in list if not known
            addressTable.insert(std::make_pair(id,iu)); // if not, include this new
            am.addIpToMap(agentId, iu->careOfAddress);
        }
        createSequenceUpdate(agentId, am.getSeqNo(agentId), am.getAckNo(agentId)); // next address must be updated by seq update
    } else
        if(sessionState == UNASSOCIATED) {
            addressTable.insert(std::make_pair(id,iu)); // if not, include this new
            am.addIpToMap(agentId, iu->careOfAddress);
            int seq = am.getSeqNo(agentId);
            int ack = am.getAckNo(agentId);
            EV << "MA: Interface-IP: " << iu->careOfAddress << " s:" << seq << " a:" << ack << endl;
            createSessionInit(); // first address is initialzed with session init
        } else { // delayin process if one were started
            cMessage *msg = new cMessage("interfaceDelay", MSG_INTERFACE_DELAY);
            InterfaceInit *ii = new InterfaceInit();
            ii->id = id;
            ii->iu = iu;
            msg->setContextPointer(ii);
            scheduleAt(simTime()+TIMEDELAY_IFACE_INIT, msg);
        }
}

MobileAgent::LinkUnit *MobileAgent::getLinkUnit(MACAddress mac)
{
    LinkUnit *lu;
    auto it = linkTable.find(mac);
    if(it != linkTable.end()) { // linkTable contains an instance of linkUnit
        lu = it->second;
        return lu;
    } else {
        lu = new LinkUnit();
        lu->snir = 0;
        lu->per = 0;
        linkTable.insert(std::make_pair(mac,lu));
        EV << "MA: Created MAC entry: " << mac << endl;
        return lu;
    }
}

InterfaceEntry *MobileAgent::getInterface(IPv6Address destAddr, int destPort, int sourcePort, short protocol)
{ // const IPv6Address &destAddr,
    InterfaceEntry *ie = nullptr;
    double maxSnr = 0;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        if(!(ift->getInterface(i)->isLoopback()) && ift->getInterface(i)->isUp()) {// && ift->getInterface(i)->ipv6Data()->getPreferredAddress().isGlobal()) {
            LinkUnit *lu = getLinkUnit(ift->getInterface(i)->getMacAddress());
            if(lu->snir >= maxSnr) {
                maxSnr = lu->snir; // selecting highest snr interface
                ie=ift->getInterface(i);
            }
        }
    }
    if(ie)
        return ie;
    else {
    // if an interface with carrier does not exist return any interface but not loopback
        for (int i=0; i<ift->getNumInterfaces(); i++) {
            if(!(ift->getInterface(i)->isLoopback())) {// && ift->getInterface(i)->ipv6Data()->getPreferredAddress().isGlobal()) {
                return ift->getInterface(i);
            }
        }
    }
    return ie;
}

bool MobileAgent::isInterfaceUp()
{
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        if(!(ift->getInterface(i)->isLoopback()) && ift->getInterface(i)->isUp() && ift->getInterface(i)->ipv6Data()->getPreferredAddress().isGlobal()) {// && ift->getInterface(i)->ipv6Data()->getPreferredAddress().isGlobal()) {
            return true;
        }
    }
    return false;
}


void MobileAgent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, simtime_t delayTime) {
//    EV << "A: Creating IPv6ControlInfo to lower layer" << endl;

    IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
    ctrlInfo->setProtocol(IP_PROT_IPv6EXT_ID); // todo must be adjusted
    ctrlInfo->setDestAddr(destAddr);
    ctrlInfo->setHopLimit(255);
    InterfaceEntry *ie = getInterface(destAddr);
    if(ie) {
        ctrlInfo->setInterfaceId(ie->getInterfaceId());
        ctrlInfo->setSrcAddr(ie->ipv6Data()->getPreferredAddress());
    }
    msg->setControlInfo(ctrlInfo);
    cGate *outgate = gate("toLowerLayer");
    LinkUnit *lu = getLinkUnit(ie->getMacAddress());
    EV << "MA2IP: Dest=" << ctrlInfo->getDestAddr() << " Src=" << ctrlInfo->getSrcAddr() << " If=" << ctrlInfo->getInterfaceId() << " SNIR=" << lu->snir << endl;
    if (delayTime > 0) {
//        EV << "delayed sending" << endl;
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

void MobileAgent::sendAllPacketsInQueue()
{
    EV << "MA: SENDING ALL PACKETS IN QUEUE." << endl;
    for(auto it : packetQueue) {
        PacketTimerKey key = it.first;
        cancelPacketTimer(key);
        PacketTimer *pkt = it.second;
//        EV << "MA: iterating and schedulting through queue. Destined schedule time: " << pkt->nextScheduledTime << endl;
        pkt->nextScheduledTime = simTime();
        if(pkt->packet->getKind() == MSG_TCP_RETRANSMIT || pkt->packet->getKind() == MSG_UDP_RETRANSMIT)
            scheduleAt(pkt->nextScheduledTime, pkt->packet);
        else
            throw cRuntimeError("MA: message type is unknown. Set kind to tcp or upd.");
    }
//    EV << "MA: End of looping." << endl;
}

MobileAgent::PacketTimer *MobileAgent::getPacketTimer(PacketTimerKey& key)
{
    PacketTimer *msg;
    auto it = packetQueue.find(key);
    if(it != packetQueue.end()) {
        msg = it->second;
//        cancelEvent(msg->packet);
    } else {
        msg = new PacketTimer();
        msg->packet = nullptr;
        msg->destAddress = key.destAddress;
        msg->destPort = key.destPort;
        msg->sourcePort = key.sourcePort;
        msg->prot = key.prot;
        EV << "MA: INSERT ENTRY IN PACKET QUEUE." << endl;
        packetQueue.insert(std::make_pair(key, msg));
    }
    return msg;
}

bool MobileAgent::isPacketTimerQueued(PacketTimerKey& key)
{
    return packetQueue.count(key);
}

void MobileAgent::cancelPacketTimer(PacketTimerKey& key)
{
    auto it = packetQueue.find(key);
    if(it == packetQueue.end())
        return;
    PacketTimer *msg = it->second;
    cancelEvent(msg->packet);
}

void MobileAgent::deletePacketTimer(PacketTimerKey& key)
{
    auto it = packetQueue.find(key);
    if(it == packetQueue.end())
        return;
    PacketTimer *msg = it->second;
    cancelAndDelete(msg->packet);
    msg->packet = nullptr;
    packetQueue.erase(key);
    delete msg;
}

void MobileAgent::deletePacketTimerEntry(PacketTimerKey& key)
{
    auto it = packetQueue.find(key);
    if(it == packetQueue.end())
        return;
    PacketTimer *msg = it->second;
    cancelEvent(msg->packet);
    packetQueue.erase(key);
    delete msg;
}

} //namespace
