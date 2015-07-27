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
#include "inet/transportlayer/udp/UDPPacket.h"
#include "inet/transportlayer/tcp_common/TCPSegment.h"

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
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processOutgoingUdpPacket(msg, controlInfo);
        }
        else if(msg->getKind() == MSG_TCP_RETRANSMIT) { // from MA
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processOutgoingTcpPacket(msg, controlInfo);
        }
        else if(msg->getKind() == MSG_INTERFACE_DELAY) { // from MA
            InterfaceInit *ii = (InterfaceInit *) msg->getContextPointer();
            updateAddressTable(ii->id, ii->iu);
            delete msg;
        }
        else
            throw cRuntimeError("handleMessage: Unknown timer expired. Which timer msg is unknown?");
    }
    else if(msg->arrivedOn("fromUDP")) {
        cObject *ctrl = msg->removeControlInfo();
        if (dynamic_cast<IPv6ControlInfo *>(ctrl) != nullptr) {
//            EV << "MA: Received packet fromUDP" << endl;
            IPv6ControlInfo *controlInfo = (IPv6ControlInfo *) ctrl;
            processOutgoingUdpPacket(msg, controlInfo);
        }
//        else
//            EV << "MA: Received udp packet but not with ipv6 ctrl info." << endl;
    }
    else if(msg->arrivedOn("fromTCP")) {
        cObject *ctrl = msg->removeControlInfo();
        if (dynamic_cast<IPv6ControlInfo *>(ctrl) != nullptr) {
//            EV << "MA: Received packet fromTCP" << endl;
            IPv6ControlInfo *controlInfo = (IPv6ControlInfo *) ctrl;
            processOutgoingTcpPacket(msg, controlInfo);
        }
//        else
//            EV << "MA: Received tcp packet but not with ipv6 ctrl info." << endl;
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
    EV << "MA: Send CA_init" << endl;
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
    EV << "MA: Send Seq_init_to_CA" << endl;
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
        scheduleAt(sut->nextScheduledTime+4, msg);
    }
}

void MobileAgent::sendSequenceUpdate(cMessage* msg) {
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
    const IPv6Address &dest =  sut->dest;
    sut->nextScheduledTime = simTime() + sut->ackTimeout;
    sut->ackTimeout = (sut->ackTimeout)*2;
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

//void Agent::createRedirection

void MobileAgent::createFlowRequest(FlowTuple &tuple) {
//    EV << "MA: Send FlowRequest" << endl;
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
    EV << "MA: Sending flow request." << endl;
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
        EV << "MA: Flow request responsed by CA. Request process successfully established." << endl;
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
            EV << "MA: Received update acknowledgment from CA. Removed timer." << endl;
        } else {
            EV << "MA: Received update acknowledgment from CA contains older sequence value. No operation." << endl;
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
//        EV << "IP2UDP: Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " If=" << controlInfo->getInterfaceId() << " Pkt=" << udpPacket->getByteLength() << endl;
        send(udpPacket, outgate);
    } else
        throw cRuntimeError("MA:procDAmsg: UDP packet could not be cast.");
}

void MobileAgent::performIncomingTcpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *controlInfo)
{
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
        ipControlInfo->setInterfaceId(100);
        ipControlInfo->setHopLimit(controlInfo->getHopLimit());
        ipControlInfo->setTrafficClass(controlInfo->getTrafficClass());
        tcpseg->setControlInfo(ipControlInfo);
        cGate *outgate = gate("toTCP");
//        EV << "IP2TCP: Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " If=" << controlInfo->getInterfaceId() << " PktSize=" << tcpseg->getByteLength() << endl;
        send(tcpseg, outgate);
    } else
        throw cRuntimeError("MA:procDAmsg: TCP packet could not be cast.");
}

void MobileAgent::processOutgoingUdpPacket(cMessage *msg, IPv6ControlInfo *controlInfo)
{
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
        IdentificationHeader *ih = getAgentHeader(1, IP_PROT_UDP, am.getSeqNo(agentId), am.getAckNo(agentId), agentId);
        ih->setIsIdInitialized(true);
        ih->setIsIdAcked(true);
        ih->setIsSeqValid(true);
        ih->setIsAckValid(true);
        ih->setIsWithNodeAddr(true);
        if(am.getSeqNo(agentId) != am.getAckNo(agentId))
            ih->setIsIpModified(true);
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
        ih->encapsulate(udpPacket);
        // TODO set here scheduler, which decides the outgoing interface
        controlInfo->setProtocol(IP_PROT_IPv6EXT_ID);
        controlInfo->setDestinationAddress(funit->dataAgent);
        controlInfo->setInterfaceId(getInterface(funit->dataAgent, tuple.destPort, tuple.sourcePort)->getInterfaceId()); // just override existing entries
        controlInfo->setSourceAddress(getInterface(funit->dataAgent, tuple.destPort, tuple.sourcePort)->ipv6Data()->getPreferredAddress());
        ih->setControlInfo(controlInfo); // make copy before setting param
        cGate *outgate = gate("toLowerLayer");
//        EV << "UDP2IP: Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " Pkt=" << ih->getByteLength() << endl;
        send(ih, outgate);
    } else
        throw cRuntimeError("MA: Incoming message should be UPD packet.");
}

void MobileAgent::processOutgoingTcpPacket(cMessage *msg, IPv6ControlInfo *controlInfo)
{
    if (dynamic_cast<tcp::TCPSegment *>(msg) != nullptr) {
//        EV << "MA:tcpIn: processing tcp pkt. Node: " << controlInfo->getDestinationAddress().toIPv6() << endl;
        tcp::TCPSegment *tcpseg = (tcp::TCPSegment *) msg;
        FlowTuple tuple;
        tuple.protocol = IP_PROT_TCP;
        tuple.destPort = tcpseg->getDestinationPort();
        tuple.sourcePort = tcpseg->getSourcePort();
        tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
        tuple.interfaceId = controlInfo->getDestinationAddress().toIPv6().getInterfaceId();
        FlowUnit *funit = getFlowUnit(tuple);
        if(funit->state == UNREGISTERED) {
            EV << "MA: Tuple creating. IP: " << controlInfo->getDestinationAddress().toIPv6() << " Pd:" << tcpseg->getDestinationPort() << " Ps:" << tcpseg->getSourcePort() << " Id:" << controlInfo->getDestinationAddress().toIPv6().getInterfaceId() << endl;
            funit->lifetime = MAX_PKT_LIFETIME;
            funit->state = REGISTERING;
            funit->nodeAddress = controlInfo->getDestinationAddress().toIPv6();
            EV << "MA:tcpIn: delaying processing of ougoing tcp packet, creating flow request and waiting for completion." << endl;
            createFlowRequest(tuple); // invoking flow initialization and delaying pkt by sending in queue of this object (hint: scheduleAt with delay)
            tcpseg->setKind(MSG_TCP_RETRANSMIT);
            tcpseg->setControlInfo(controlInfo); // appending own
            scheduleAt(simTime()+TIMEDELAY_PKT_PROCESS, tcpseg);
            return;
        } else {
            if(funit->state == REGISTERING) {
                EV << "MA: Tuple registering. IP: " << controlInfo->getDestinationAddress().toIPv6() << " Pd:" << tcpseg->getDestinationPort() << " Ps:" << tcpseg->getSourcePort() << " Id:" << controlInfo->getDestinationAddress().toIPv6().getInterfaceId() << endl;
                EV << "MA:tcpIn starting registering process of first tcp packet: "<< funit->lifetime << endl;
                funit->lifetime--; // not tested if working!
                if(funit->lifetime < 1) {
                    EV << "MA:tcpIn: deleting tcp packet because of exceeding lifetime." << endl;
                    delete msg;
                    delete controlInfo;
                    return;
                } // TODO discarding all packets of tuple
                if(isAddressAssociated(controlInfo->getDestinationAddress().toIPv6())) {
                    EV << "MA:tcpIn: creating address association. only done one time actually." << endl;
                    funit->lifetime = MAX_PKT_LIFETIME;
                    IPv6Address *ag = getAssociatedAddress(controlInfo->getDestinationAddress().toIPv6());
                    if(ag) {
                        EV << "MA:tcpIn: creating flow unit." << endl;
                        funit->state = REGISTERED;
                        funit->isFlowActive = true;
                        funit->isAddressCached = true;
                        funit->dataAgent = *ag;
                        funit->nodeAddress = controlInfo->getDestinationAddress().toIPv6();
                        funit->id = agentId;
                    } else {
                        throw cRuntimeError("MA:tcpIn: getAssociatedAddr fetched empty pointer instead of address.");
                    }
                } else {
                    EV << "MA:tcpIn sending tcp packet in queue, such that it can be processed in next round" << endl;
                    tcpseg->setKind(MSG_TCP_RETRANSMIT);
                    tcpseg->setControlInfo(controlInfo); // appending own
                    scheduleAt(simTime()+TIMEDELAY_PKT_PROCESS, tcpseg);
                    return;
                }
            } else
                if(funit->state == REGISTERED) {
                    funit->lifetime = MAX_PKT_LIFETIME;
                // what should here be done?
                } else
                    throw cRuntimeError("MA:tcpIn: Not known state of flow. What should be done?");
        }
//        EV << "MA:tcpIn: flow is registered. starting sending process." << endl;
        IdentificationHeader *ih = getAgentHeader(1, IP_PROT_TCP, am.getSeqNo(agentId), am.getAckNo(agentId), agentId);
        ih->setIsIdInitialized(true);
        ih->setIsIdAcked(true);
        ih->setIsSeqValid(true);
        ih->setIsAckValid(true);
        ih->setIsWithNodeAddr(true);
        if(am.getSeqNo(agentId) != am.getAckNo(agentId)) {
            ih->setIsIpModified(true);
            EV << "MA: SENDING PACKET WITH IP CHANGES! s:" << am.getSeqNo(agentId) << " a:" << am.getAckNo(agentId) << endl;
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
        ih->encapsulate(tcpseg);
        // TODO set here scheduler, which decides the outgoing interface
        controlInfo->setProtocol(IP_PROT_IPv6EXT_ID);
        controlInfo->setDestinationAddress(funit->dataAgent);
        controlInfo->setInterfaceId(getInterface(funit->dataAgent, tuple.destPort, tuple.sourcePort)->getInterfaceId()); // just override existing entries
        controlInfo->setSourceAddress(getInterface(funit->dataAgent, tuple.destPort, tuple.sourcePort)->ipv6Data()->getPreferredAddress());
        ih->setControlInfo(controlInfo); // make copy before setting param
        cGate *outgate = gate("toLowerLayer");
//        EV << "TCP2IP: Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " Node=" << controlInfo->getDestinationAddress().toIPv6() << " Pkt=" << ih->getByteLength() << endl;
        send(ih, outgate);
    } else
        throw cRuntimeError("MA: Incoming message should be TCP packet.");
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
//    EV << "MA: handle interface down message. Addr: " << iu->careOfAddress.str() << endl;
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
//    EV << "MA: Handle interface up message. Addr: " << iu->careOfAddress.str() << endl;
    updateAddressTable(ie->getInterfaceId(), iu);
    cancelExpiryTimer(ip.UNSPECIFIED_ADDRESS,ie->getInterfaceId(),TIMERKEY_IF_UP);
    delete msg;
}

void MobileAgent::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    Enter_Method_Silent();
    if(signalID == NF_INTERFACE_STATE_CHANGED) { // is triggered when carrier setting is changed
//        if(dynamic_cast<InterfaceEntryChangeDetails *>(obj)) {
//            InterfaceEntry *ie = check_and_cast<const InterfaceEntryChangeDetails *>(obj)->getInterfaceEntry();
//        }
    }
    if(signalID == NF_INTERFACE_IPv6CONFIG_CHANGED) { // is triggered when carrier setting is changed
        if(dynamic_cast<InterfaceEntryChangeDetails *>(obj)) {
            InterfaceEntry *ie = check_and_cast<const InterfaceEntryChangeDetails *>(obj)->getInterfaceEntry();
            if(ie->isUp()) { createInterfaceUpMessage(ie->getInterfaceId()); }
            else { createInterfaceDownMessage(ie->getInterfaceId()); }
        }
    }

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

InterfaceEntry *MobileAgent::getInterface(IPv6Address destAddr, int destPort, int sourcePort, short protocol) { // const IPv6Address &destAddr,
    InterfaceEntry *ie = nullptr;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp() && ie->ipv6Data()->getPreferredAddress().isGlobal()) { return ie; }
    }
    return ie;
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
//    EV << "MA2IP: Dest=" << ctrlInfo->getDestAddr() << " Src=" << ctrlInfo->getSrcAddr() << " If=" << ctrlInfo->getInterfaceId() << endl;
    if (delayTime > 0) {
//        EV << "delayed sending" << endl;
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

} //namespace
