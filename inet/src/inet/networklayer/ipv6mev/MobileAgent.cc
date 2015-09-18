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
#include "inet/applications/pingapp/PingPayload_m.h"
#include "inet/common/NotifierConsts.h"
#include "inet/common/lifecycle/NodeStatus.h"

namespace inet {

Define_Module(MobileAgent);

simsignal_t MobileAgent::controlSignalLoad = registerSignal("controlSignalLoad");
simsignal_t MobileAgent::dataSignalLoad = registerSignal("dataSignalLoad");
simsignal_t MobileAgent::interfaceSnir = registerSignal("interfaceSnir");
simsignal_t MobileAgent::interfaceId = registerSignal("interfaceId");
simsignal_t MobileAgent::sequenceUpdateCa = registerSignal("sequenceUpdateCa");
simsignal_t MobileAgent::sequenceUpdateDa = registerSignal("sequenceUpdateDa");
simsignal_t MobileAgent::flowRequest = registerSignal("flowRequest");
simsignal_t MobileAgent::flowRequestDelay = registerSignal("flowRequestDelay");

MobileAgent::~MobileAgent() {
    auto it = expiredTimerList.begin();
    while(it != expiredTimerList.end()) {
        TimerKey key = it->first;
        it++;
        cancelAndDeleteExpiryTimer(key.dest,key.interfaceID,key.type);
    }
//    auto it2 = linkTable.begin();
//    while(it2 != linkTable.end()) {
//        LinkBuffer *lb = it2->second;
//        while(!lb->empty())
//            lb->pop();
//        it2++;
//        delete lb;
//    }
    auto it4 = packetQueue.begin();
    while(it4 != packetQueue.end()) {
        PacketTimerKey packet = it4->first;
        it4++;
        deletePacketTimerEntry(packet);
    }
}

void MobileAgent::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER) {
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        rt = getModuleFromPar<IPv6RoutingTable>(par("routingTableModule"), this);
        isIdLayerEnabled = par("isIdLayerEnabled").boolValue();
        sessionState = UNASSOCIATED;
        seqnoState = UNASSOCIATED;
//        srand(time(0));
        agentId = (uint64) rand();
        initAddressMap(agentId, (rand() % 128) + 1);

        // statistics
        flowRequestSignal = -1;
        WATCH(agentId);
        WATCH(sessionState);
        WATCH(seqnoState);
        WATCH(dataSignalLoadStat);
        WATCH(ControlAgentAddress);
        WATCH(sequenceUpdateCaStat);
        WATCH(sequenceUpdateDaStat);
        WATCH(flowRequestStat);
        WATCH(flowResponseStat);
        WATCH(interfaceSnirStat);
        WATCH(interfaceIdStat);
        WATCH_MAP(flowTable);
        WATCH(*this);
        if(hasPar("enableNodeRequesting"))
            enableNodeRequesting = par("enableNodeRequesting").boolValue();

    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
        if(isIdLayerEnabled) {
            IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
            ipSocket.registerProtocol(IP_PROT_IPv6_ICMP); // only ICMP and ID packets can received
            ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID); // other packets are not parsed.
            interfaceNotifier = getContainingNode(this);
            interfaceNotifier->subscribe(NF_INTERFACE_IPv6CONFIG_CHANGED,this);
            interfaceNotifier->subscribe(NF_INTERFACE_STATE_CHANGED,this);
            interfaceNotifier->subscribe(physicallayer::Radio::minSNIRSignal, this);
            interfaceNotifier->subscribe(physicallayer::Radio::packetErrorRateSignal, this);
        } else {
            EV << "MA: Id layer is disabled." << endl;
        }
    }
    if (stage == INITSTAGE_APPLICATION_LAYER) {
//        simtime_t startTime = par("startTime");
//        cMessage *timeoutMsg = new cMessage("sessionStart");
//        timeoutMsg->setKind(MSG_START_TIME);
//        scheduleAt(startTime, timeoutMsg); // delaying start
    }

}


void MobileAgent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage() && isIdLayerEnabled) {
        if(msg->getKind() == MSG_START_TIME) {
            createSessionInit();
            delete msg;
            return;
        }
        else if(msg->getKind() == MSG_SESSION_INIT) {
            sendSessionInit(msg);
        }
        else if(msg->getKind() == MSG_SEQNO_INIT) {
            sendSequenceInit(msg);
        }
        else if(msg->getKind() == MSG_IF_DOWN) {
            handleInterfaceDown(msg);
        }
        else if(msg->getKind() == MSG_IF_UP) {
            handleInterfaceUp(msg);
        }
        else if(msg->getKind() == MSG_IF_CHANGE) {
            handleInterfaceChange(msg);
        }
        else if(msg->getKind() == MSG_SEQ_UPDATE) { // from
            sendSequenceUpdate(msg);
        }
//        else if(msg->getKind() == MSG_SEQ_UPDATE_DELAYED) {
//            SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
//            uint64 id = sut->id;
//            uint seq = sut->seq;
//            uint ack = sut->ack;
//            createSequenceUpdate(id, seq, ack);
//            cancelAndDeleteExpiryTimer(ControlAgentAddress,-1, TIMERKEY_SEQ_UPDATE, agentId, agentHeader->getIpSequenceNumber(), getAckNo(agentId));
//            delete msg;
//            return;
//        }
        else if(msg->getKind() == MSG_FLOW_REQ) { // from MA
            sendFlowRequest(msg);
        }
        else if(msg->getKind() == MSG_UDP_RETRANSMIT) { // from MA
            processOutgoingUdpPacket(msg);
        }
        else if(msg->getKind() == MSG_TCP_RETRANSMIT) { // from MA
            processOutgoingTcpPacket(msg);
        }
        else if(msg->getKind() == MSG_ICMP_RETRANSMIT) { // from MA
            processOutgoingIcmpPacket(msg);
        }
        else {
            throw cRuntimeError("MA_handleMessage: Unknown selfMessage received. Kind: %s ClassName: %s", msg->getKind(), msg->getClassName());
        }
    }
    else if(msg->arrivedOn("fromUDP")) {
        if(isIdLayerEnabled)
            processOutgoingUdpPacket(msg);
        else
            send(msg, "udpIpOut");
    }
    else if(msg->arrivedOn("fromTCP")) {
        if(isIdLayerEnabled)
            processOutgoingTcpPacket(msg);
        else
            send(msg, "tcpIpOut");
    }
    else if(msg->arrivedOn("fromICMP")) { // icmp messages from ICMP module
        if(isIdLayerEnabled)
            processOutgoingIcmpPacket(msg);
        else
            send(msg, "icmpIpOut");
    }
    else if(msg->arrivedOn("icmpIpIn")) {
        if(!isIdLayerEnabled)
            send(msg, "toICMP");
    }
    else if(msg->arrivedOn("udpIpIn")) { // icmp messages from ICMP module
        if(!isIdLayerEnabled)
            send(msg, "toUDP");
    }
    else if(msg->arrivedOn("tcpIpIn")) { // icmp messages from ICMP module
        if(!isIdLayerEnabled)
            send(msg, "toTCP");
    }
    else if(msg->arrivedOn("fromLowerLayer")) {
        if (dynamic_cast<IdentificationHeader *> (msg)) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processAgentMessage((IdentificationHeader *) msg, controlInfo);
        } else
        if (dynamic_cast<ICMPv6Message *> (msg)) { // icmp messages from IP module
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processIncomingIcmpPacket((ICMPv6Message *) msg, controlInfo);
        } else {
            throw cRuntimeError("MA: IP module should only forward ID packages.");
        }
    }
    else
        if(isIdLayerEnabled)
            throw cRuntimeError("MA->handleMessage: cMessage Type not known. What did you send?");
        else
            throw cRuntimeError("MA->handleMessage: received selfMessage while id layer is disabled.");
}

void MobileAgent::createSessionInit() {
    if(sessionState == UNASSOCIATED)
        sessionState = INITIALIZING;
    if(sessionState == ASSOCIATED) {
        createSequenceInit();
        return;
    }
    L3Address caAddr;
    const char *controlAgentAddr;
    controlAgentAddr = par("controlAgent");
    L3AddressResolver().tryResolve(controlAgentAddr, caAddr);
    ControlAgentAddress = caAddr.toIPv6();
    if(ControlAgentAddress.isGlobal()) {
        cMessage *msg = new cMessage("createSessionInit",MSG_SESSION_INIT);
        TimerKey key(ControlAgentAddress,-1,TIMERKEY_SESSION_INIT);
        SessionInitTimer *sit = (SessionInitTimer *) getExpiryTimer(key,TIMERTYPE_SESSION_INIT);
        sit->dest = ControlAgentAddress;
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
    SessionInitTimer *sit = (SessionInitTimer *) msg->getContextPointer();
    const IPv6Address &dest = sit->dest;
    sit->nextScheduledTime = simTime() + sit->ackTimeout;
    sit->ackTimeout = (sit->ackTimeout)*1;
    if(sit->dest.isGlobal()) { // not necessary
        IdentificationHeader *ih = createAgentHeader(1, IP_PROT_NONE, 0, 0, agentId);
        ih->setIsIdInitialized(true);
        ih->setByteLength(SIZE_AGENT_HEADER);
        ih->setName(msg->getName());
        long size = ih->getByteLength();
        emit(controlSignalLoad, size);
        sendToLowerLayer(ih, dest);
        EV_INFO << "MA: Sending session initialization to Control Agent at " << dest.str() << endl;
    }
    scheduleAt(sit->nextScheduledTime, msg);
}

void MobileAgent::createSequenceInit() { // does not support interface check
    if(sessionState != ASSOCIATED)
        throw cRuntimeError("MA_createSequenceUpdate: Session initialization not completed. Sequence initialization not possible.");
//    if(seqnoState == UNASSOCIATED) {
    seqnoState = INITIALIZING;
    cMessage *msg = new cMessage("createSequenceInit", MSG_SEQNO_INIT);
    TimerKey key(ControlAgentAddress,-1,TIMERKEY_SEQNO_INIT);
    SequenceInitTimer *sit = (SequenceInitTimer *) getExpiryTimer(key, TIMERTYPE_SEQNO_INIT);
    sit->dest = ControlAgentAddress;
    sit->timer = msg;
    sit->ackTimeout = TIMEOUT_SEQNO_INIT;
    sit->nextScheduledTime = simTime();
    msg->setContextPointer(sit);
    scheduleAt(sit->nextScheduledTime, msg);
//    }
}

void MobileAgent::sendSequenceInit(cMessage *msg) {
    SequenceInitTimer *sit = (SequenceInitTimer *) msg->getContextPointer();
    const IPv6Address &dest = sit->dest;
//    InterfaceEntry *ie = getInterface();
    sit->nextScheduledTime = simTime() + sit->ackTimeout;
    sit->ackTimeout = (sit->ackTimeout)*1;
    if(!isInterfaceAssociated()) {
        scheduleAt(sit->nextScheduledTime, msg);
        EV_WARN << "MA_sendSequenceInit: Currently no interface available for sending sequence initialization" << endl;
        return;
//        throw cRuntimeError("MA: no interface provided in sendSeqInit.");
    }
    IdentificationHeader *ih = createAgentHeader(1, IP_PROT_NONE, getSeqNo(agentId), 0, agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsIpModified(true);
    ih->setIpAddingField(1);
    ih->setIpRemovingField(0);
    ih->setIpAcknowledgementNumber(0);
    AddressDiff ad = getAddressList(agentId, getSeqNo(agentId));
    ih->setIPaddressesArraySize(ad.insertedList.size());
    ih->setIpAddingField(ad.insertedList.size());
    if(ad.insertedList.size() > 0) {
        for(int idx=0; idx < ad.insertedList.size(); idx++)
            ih->setIPaddresses(idx, ad.insertedList.at(idx).address);
        ih->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR);
        ih->setName(msg->getName());
        long size = ih->getByteLength();
        emit(controlSignalLoad, size);
        sendToLowerLayer(ih, dest);
    } else {
        EV_DEBUG << "MA_sendSequenceInit: No IP address provided for sequence number initialization: " << endl;
    }
    scheduleAt(sit->nextScheduledTime, msg);
    EV_INFO << "MA_sendSequenceInit: Sending sequence number initialization: " << getSeqNo(agentId) << endl;
}

// Creates a periodic timer of update messages to Control Agent containing the current sequence number.
void MobileAgent::createSequenceUpdate(uint seq, uint ack) {
    if(sessionState != ASSOCIATED)
        throw cRuntimeError("MA_createSequenceUpdate: Session initialization not completed. Sequence update not possible.");
    if(seqnoState != ASSOCIATED)
        throw cRuntimeError("MA_createSequenceUpdate: Sequence initialization not completed. Sequence update not possible.");
    if(seq == ack) // nothing to update
        return;
    bool timer = cancelAndDeleteExpiryTimer(ControlAgentAddress,-1, TIMERKEY_SEQ_UPDATE, agentId, getSeqNo(agentId), getAckNo(agentId));
    if(timer)
        EV<< "A timer was configured" << endl;
    cMessage *msg = new cMessage("createSequenceUpdate", MSG_SEQ_UPDATE);
    TimerKey key(ControlAgentAddress, -1, TIMERKEY_SEQ_UPDATE, agentId, seq);
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) getExpiryTimer(key, TIMERTYPE_SEQ_UPDATE);
    sut->dest = ControlAgentAddress;
    sut->ackTimeout = TIMEOUT_SEQ_UPDATE;
    sut->nextScheduledTime = simTime();
    sut->seq = seq;
    sut->ack = ack;
    sut->id = agentId;
    sut->timer = msg;
    msg->setContextPointer(sut);
    scheduleAt(sut->nextScheduledTime, msg);
//    if(!isInterfaceAssociated()) {
//        cMessage *msg = new cMessage("createSequenceUpdate", MSG_SEQ_UPDATE_DELAYED);
//        sut->timer = msg;
//        msg->setContextPointer(sut);
//        scheduleAt(simTime()+TIMEOUT_SEQ_UPDATE, msg);
//    } else {
////        EV << "MA: Sending seq update." << endl;
//    }
}

// Sends to Control Agent an update message of the current sequence number.
void MobileAgent::sendSequenceUpdate(cMessage* msg) {
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
    const IPv6Address &dest =  sut->dest;
    sut->nextScheduledTime = simTime() + sut->ackTimeout;
    sut->ackTimeout = (sut->ackTimeout)*1;
    IdentificationHeader *ih = createAgentHeader(1, IP_PROT_NONE, getSeqNo(agentId), getAckNo(agentId), agentId);
    ih->setName(msg->getName());
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsAckValid(true);
    ih->setIsIpModified(true);
        int s = getSeqNo(agentId); int a = getAckNo(agentId);
        EV_DEBUG << "MA_sendSequenceUpdate: Send sequence update (" << "s: " << s << " a: " << a << ") message to Control Agent." <<  endl;
    AddressDiff ad = getAddressList(agentId,sut->seq,sut->ack);
    ih->setIPaddressesArraySize(ad.insertedList.size()+ad.deletedList.size());
    ih->setIpAddingField(ad.insertedList.size());
    if(ad.insertedList.size() > 0) {
        for(int idx=0; idx < ad.insertedList.size(); idx++)
            ih->setIPaddresses(idx, ad.insertedList.at(idx).address);
    }
    ih->setIpRemovingField(ad.deletedList.size());
    if(ad.deletedList.size() > 0) {
        for(int idx=0; idx < ad.deletedList.size(); idx++)
            ih->setIPaddresses(idx+ad.insertedList.size(), ad.deletedList.at(idx).address);
    }
    ih->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR*(ad.insertedList.size()+ad.deletedList.size()));
    sequenceUpdateCaStat++;
    emit(sequenceUpdateCa, sequenceUpdateCaStat);
    long size = ih->getByteLength();
    emit(controlSignalLoad, size);
    sendToLowerLayer(ih, dest);
    scheduleAt(sut->nextScheduledTime, msg);
}

bool MobileAgent::createFlowRequest(FlowTuple &tuple) {
    if(sessionState != ASSOCIATED ||  seqnoState != ASSOCIATED) {
        EV_WARN << "MA_createFlowRequest: Session/Sequence initialization not completed. Flow request dropped." << endl;
        if(isInterfaceAssociated()) {
            if(sessionState != ASSOCIATED)
                createSessionInit();
            else
                createSequenceInit();
        }
        return false;
    }
    cMessage *msg = new cMessage("createFlowRequest", MSG_FLOW_REQ);
    TimerKey key(ControlAgentAddress, -1, TIMERKEY_FLOW_REQ);
    FlowRequestTimer *frt = (FlowRequestTimer *) getExpiryTimer(key, TIMERTYPE_FLOW_REQ);
    FlowUnit *funit = getFlowUnit(tuple);
//    if(funit->state != REGISTERING)
//        throw cRuntimeError("MA: No match for incoming TCP packet");
    frt->dest = ControlAgentAddress;
    frt->tuple = tuple;
    frt->timer = msg;
    frt->ackTimeout = TIMEDELAY_FLOW_REQ;
    frt->nodeAddress = funit->nodeAddress;
//    InterfaceEntry *ie = getInterface(ControlAgentAddress);
//    if(!ie) {
//        EV_INFO << "MA_createFlowRequest: Delaying flow request. No interface provided." << endl;
//        frt->ie = nullptr;
//        frt->nextScheduledTime = simTime()+TIMEDELAY_FLOW_REQ;
//    } else {
//    }
//    frt->ie = ie;
    frt->nextScheduledTime = simTime();
    msg->setContextPointer(frt);
    scheduleAt(frt->nextScheduledTime, msg);
    return true;
}

void MobileAgent::sendFlowRequest(cMessage *msg) {
    EV_INFO << "MA_sendFlowRequest: Preparing flow request." << endl;
    FlowRequestTimer *frt = (FlowRequestTimer *) msg->getContextPointer();
    frt->ackTimeout = (frt->ackTimeout)*1;
    frt->nextScheduledTime = simTime()+frt->ackTimeout;
//    if(!frt->ie) { // if interface is provided, send message, else delay transmission
//        frt->ie = getInterface(frt->dest); // check if interface is now provided
//        if(!frt->ie) { // no interface yet
//            EV_INFO << "MA_sendFlowRequest: Delaying flow request due to missing associated interface." << endl;
//            scheduleAt(frt->nextScheduledTime, msg);
//            return;
//        }
//    }
    flowRequestSignal = simTime();
    flowRequestStat++;
    emit(flowRequest, flowRequestStat);
    EV_INFO << "MA_sendFlowRequest: Sending flow request." << endl;
    const IPv6Address nodeAddress = frt->nodeAddress;
    const IPv6Address &dest =  frt->dest;
    IdentificationHeader *ih = createAgentHeader(1, IP_PROT_NONE, getSeqNo(agentId), getAckNo(agentId), agentId);
    ih->setName(msg->getName());
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsAckValid(true);
    ih->setIsWithNodeAddr(true);
    ih->setIPaddressesArraySize(1);
    ih->setIPaddresses(0,nodeAddress);
    ih->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR);
    long size = ih->getByteLength();
    emit(controlSignalLoad, size);
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
                performFlowRequestResponse(agentHeader, destAddr);
            } else
                performSequenceUpdateResponse(agentHeader, destAddr);
        }
    } else if(agentHeader->getIsDataAgent() && agentHeader->getNextHeader() == IP_PROT_UDP && agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid())
        performIncomingUdpPacket(agentHeader, controlInfo);
    else if (agentHeader->getIsDataAgent() && agentHeader->getNextHeader() == IP_PROT_TCP && agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid())
        performIncomingTcpPacket(agentHeader, controlInfo);
    else if (agentHeader->getIsDataAgent() && agentHeader->getNextHeader() == IP_PROT_IPv6_ICMP && agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid())
        processIncomingIcmpPacket(agentHeader, controlInfo);
    else
        throw cRuntimeError("MA: Header parameter is wrong. Received messagt cannot be assigned to any function.");
    delete agentHeader;
    delete controlInfo;
}

void MobileAgent::performSessionInitResponse(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(agentHeader->getByteLength() == SIZE_AGENT_HEADER) {
        if(ControlAgentAddress != destAddr) // just for the beginning
            throw cRuntimeError("MA: Received message should be from CA, but it is received from somewhere else or caAddress is set wrongly.");
        sessionState = ASSOCIATED;
        cancelAndDeleteExpiryTimer(ControlAgentAddress,-1, TIMERKEY_SESSION_INIT);
        EV_INFO << "MA: Received session initialization acknowledgment from Control Agent. " << endl;
        createSequenceInit();
    } else {
        throw cRuntimeError("MA: Byte length does not match the expected size. SessionInitResponse.");
    }
}

void MobileAgent::performSequenceInitResponse(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(agentHeader->getByteLength() == SIZE_AGENT_HEADER) {
        seqnoState = ASSOCIATED;
        setAckNo(agentId, agentHeader->getIpSequenceNumber());
        int s = getSeqNo(agentId);
        int a = getAckNo(agentId);
        bool t = cancelAndDeleteExpiryTimer(ControlAgentAddress,-1, TIMERKEY_SEQNO_INIT);
        EV_INFO << "MA: Received AckNo. Timer existed? => " << t << ". SeqNo initialized." << " s:" << s << " a:" << a << endl;
    } else {
        throw cRuntimeError("MA_performSequenceInitResponse: Byte length does not match the expected size. Received length is %d. Expected length is %d", agentHeader->getByteLength(), SIZE_AGENT_HEADER);
    }
}


void MobileAgent::performFlowRequestResponse(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(agentHeader->getByteLength() == (SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR*2)) {
        flowResponseStat++;
        if(flowRequestSignal > 0) {
            simtime_t delay = simTime() - flowRequestSignal;
            emit(flowRequestDelay, delay);
            flowRequestSignal = -1; // reseting, preventing multiple insertion
        }
        IPv6Address node = agentHeader->getIPaddresses(0);
        IPv6Address agent = agentHeader->getIPaddresses(1);
        addressAssociation.insert(std::make_pair(node,agent));
        EV_INFO << "MA_performFlowRequestResponse: Received request of Data Agent " << agent << " for connection establishment with Correspondent Node " << node << endl;
        cancelAndDeleteExpiryTimer(ControlAgentAddress,-1, TIMERKEY_FLOW_REQ);
        sendAllPacketsInQueue();
    } else {
        throw cRuntimeError("MA: Byte length does not match the expected size.FlowRequest");
    }
}

void MobileAgent::performSequenceUpdateResponse(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(agentHeader->getByteLength() == SIZE_AGENT_HEADER) {
        if(agentHeader->getIpSequenceNumber() > getAckNo(agentId)) {
            cancelAndDeleteExpiryTimer(ControlAgentAddress,-1, TIMERKEY_SEQ_UPDATE, agentId, agentHeader->getIpSequenceNumber(), getAckNo(agentId));
            setAckNo(agentId, agentHeader->getIpSequenceNumber());
            int s = getSeqNo(agentId);
            EV_DEBUG << "MA_performSequenceUpdateResponse: Received update acknowledgment from CA. Removed timer. SeqNo=" << s << endl;
        } else {
            EV_DEBUG << "MA_performSequenceUpdateResponse: Received update acknowledgment from CA contains older sequence value. Dropping message. SeqNo="<< getSeqNo(agentId) << endl;
        }
    } else {
        throw cRuntimeError("MA_performSequenceUpdateResponse: Byte length does not match the expected size.");
    }
}

void MobileAgent::performIncomingUdpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *controlInfo)
{
    cPacket *packet = agentHeader->decapsulate();
    if (dynamic_cast<UDPPacket *>(packet) != nullptr) {
        UDPPacket *udpPacket =  (UDPPacket *) packet;
        FlowTuple tuple;
        tuple.protocol = IP_PROT_UDP;
        if(enableNodeRequesting) {
            tuple.destPort = 0;
            tuple.sourcePort = 0;
        } else {
            tuple.destPort = udpPacket->getSourcePort();
            tuple.sourcePort = udpPacket->getDestinationPort();
        }
        tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
        tuple.interfaceId = agentHeader->getId();
        FlowUnit *funit = getFlowUnit(tuple);
        if(funit->state == UNREGISTERED) {
            if(!enableNodeRequesting) {
                EV_WARN << "MA_performIncomingUdpPacket: No flowUnit match for incoming UDP packet" << endl;
                return;
            }
        } else {
            funit->state = REGISTERED;
        }
        IPv6Address fakeIp;
        fakeIp.set(0x1D << 24, 0, (agentId >> 32) & 0xFFFFFFFF, agentId & 0xFFFFFFFF);
        IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
        ipControlInfo->setProtocol(IP_PROT_UDP);
        ipControlInfo->setDestAddr(fakeIp);
        ipControlInfo->setSrcAddr(funit->nodeAddress);
        ipControlInfo->setInterfaceId(100);
        ipControlInfo->setHopLimit(controlInfo->getHopLimit());
        ipControlInfo->setTrafficClass(controlInfo->getTrafficClass());
        ipControlInfo->setOrigDatagram(controlInfo->removeOrigDatagram());
        udpPacket->setControlInfo(ipControlInfo);
        cGate *outgate = gate("toUDP");
        send(udpPacket, outgate);
    } else
        throw cRuntimeError("MA:procDAmsg: UDP packet could not be cast.");
}

void MobileAgent::processOutgoingUdpPacket(cMessage *msg)
{
    cObject *ctrl = msg->removeControlInfo();
    if (dynamic_cast<IPv6ControlInfo *>(ctrl) != nullptr) {
        IPv6ControlInfo *controlInfo = (IPv6ControlInfo *) ctrl;
        if (dynamic_cast<UDPPacket *>(msg) != nullptr) {
    //        EV << "MA:udpIn: processsing udp pkt." << endl;
            UDPPacket *udpPacket =  (UDPPacket *) msg;
            PacketTimerKey packet(IP_PROT_UDP,controlInfo->getDestinationAddress().toIPv6(),udpPacket->getDestinationPort(),udpPacket->getSourcePort());
//            if(isInterfaceUp()) {
            if(true) { // packet queueing is moved into IPv6 layer
                FlowTuple tuple;
                tuple.protocol = IP_PROT_UDP;
                if(enableNodeRequesting) {
                    tuple.destPort = 0;
                    tuple.sourcePort = 0;
                } else {
                    tuple.destPort = udpPacket->getDestinationPort();
                    tuple.sourcePort = udpPacket->getSourcePort();
                }
                tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
                tuple.interfaceId = controlInfo->getDestinationAddress().toIPv6().getInterfaceId();
                FlowUnit *funit = getFlowUnit(tuple);
                if(funit->state == UNREGISTERED) {
                    EV_DEBUG << "==> Unregistered flow detected. Creating flow request." << endl;
                    funit->nodeAddress = controlInfo->getDestinationAddress().toIPv6(); // need node address here, else flow request is wrong
                    if(createFlowRequest(tuple)) // invoking flow initialization and delaying pkt by sending in queue of this object (hint: scheduleAt with delay)
                        funit->state = REGISTERING;
                    PacketTimer *pkt = getPacketTimer(packet); // set packet in queue
                    pkt->packet = msg; // replaced by new packet
                    pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    msg->setKind(MSG_UDP_RETRANSMIT);
                    msg->setControlInfo(controlInfo);
                    msg->setContextPointer(pkt);
                    scheduleAt(pkt->nextScheduledTime, msg);
                } else if(funit->state == REGISTERING) {
                    EV_DEBUG << "==> Received UDP packet. Registering flow. Checking for response." << endl;
                    if(isAddressAssociated(controlInfo->getDestinationAddress().toIPv6())) {
                        IPv6Address *ag = getAssociatedAddress(controlInfo->getDestinationAddress().toIPv6());
                        if(!ag)
                            throw cRuntimeError("MA_processOutgoingUdpPacket:tcpIn: getAssociatedAddr fetched empty pointer instead of address.");
                        EV_DEBUG << "==> Flow response received. Restarting UDP transmission with Data Agent " << ag->str() << " and Correspondetn node " <<  controlInfo->getDestinationAddress().toIPv6().str() << endl;
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
                        EV_DEBUG << "==> Flow response still not received. Waiting for response by CA." << endl;
                        if(isPacketTimerQueued(packet)) { // check is packet is in queue
                            PacketTimer *pkt = getPacketTimer(packet);
                            if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                                EV_DEBUG << "==> New message from upper layer arrived. Replacing message and setting scheduling time." << endl;
                                cancelPacketTimer(packet); // packet is in queue, cancel scheduling
                                delete pkt->packet; // old packet is...
                                pkt->packet = msg; // replaced by new packet
                                pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                            } else {
                                EV_DEBUG << "==> processing same message (not new msg from upper layer). Just setting scheduling time." << endl;
                                pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                            }
                            msg->setContextPointer(pkt);
                            msg->setKind(MSG_UDP_RETRANSMIT);
                            msg->setControlInfo(controlInfo);
                            scheduleAt(pkt->nextScheduledTime+1, msg);
                        } else
                            throw cRuntimeError("MA:tcpIn: Message is not in queue during phase of flow registering.");
                    }
                } else if(funit->state == REGISTERED) {
                    if(isPacketTimerQueued(packet)) { // check is packet is in queue
                        PacketTimer *pkt = getPacketTimer(packet); // pop the relating packet from queue
                        if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                            deletePacketTimer(packet); // deletes older message and removes the key from the queue.
                        } else { EV_DEBUG << "==> Deleting entry in queue as this message is same to previous one and arrived at time:"<< msg->getCreationTime() << " ";
                            deletePacketTimerEntry(packet); // removes just from the queue but does not delete message
                        }
                    } else {
                        // TODO?
                    }
                    sendUpperLayerPacket(udpPacket, controlInfo, funit->dataAgent, IP_PROT_UDP);
                } else
                    throw cRuntimeError("MA_processOutgoingUdpPacket: Not known state of flow. What should be done?");
            } else
            { // interface is down, so schedule for later transmission
                if(isPacketTimerQueued(packet)) { // check if packet is in queue
                    PacketTimer *pkt = getPacketTimer(packet); // packet is cancelled
                    if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                        EV_DEBUG << "==> Interface is down and a packet is in queue. Replacing new message with older one." << endl;
                        cancelPacketTimer(packet); // packet is in queue, cancel scheduling
                        delete pkt->packet; // old packet is...
                        pkt->packet = msg; // replaced by new packet
                        pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    } else {
                        EV_DEBUG << "==> Interface is down and this packet is already in queue. Setting new schedule time." << endl;
                        pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    }
                    msg->setKind(MSG_UDP_RETRANSMIT);
                    msg->setContextPointer(pkt);
                    msg->setControlInfo(controlInfo);
                    scheduleAt(pkt->nextScheduledTime, msg);
                } else { // first packet of socket
                    EV_DEBUG << "==> Interface is down and (first) packet of flow is inserted in queue." << endl;
                    PacketTimer *pkt = getPacketTimer(packet); // inserting packet into queue
                    pkt->packet = msg;
                    pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    msg->setKind(MSG_UDP_RETRANSMIT);
                    msg->setContextPointer(pkt);
                    msg->setControlInfo(controlInfo);
                    scheduleAt(pkt->nextScheduledTime, msg);
                }
            }
        } else
            throw cRuntimeError("MA_processOutgoingUdpPacket: Incoming message could not be casted to a UDP packet.");
    } else {
        EV_WARN << "MA_processOutgoingUdpPacket: ControlInfo could not be extracted from UDP packet." << endl;
        cGate *outgate = gate("toLowerLayer");
        send(msg, outgate);
    }

}

void MobileAgent::performIncomingTcpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *controlInfo)
{
    cPacket *packet = agentHeader->decapsulate();
    if (dynamic_cast<tcp::TCPSegment *>(packet) != nullptr) {
        tcp::TCPSegment *tcpseg =  (tcp::TCPSegment *) packet;
        FlowTuple tuple;
        tuple.protocol = IP_PROT_TCP;
        if(enableNodeRequesting) {
            tuple.destPort = 0;
            tuple.sourcePort = 0;
        } else {
            tuple.destPort = tcpseg->getSourcePort();
            tuple.sourcePort = tcpseg->getDestinationPort();
        }
        tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
        tuple.interfaceId = agentHeader->getId();
        FlowUnit *funit = getFlowUnit(tuple);
        if(funit->state == UNREGISTERED) {
            if(!enableNodeRequesting) {
                EV_WARN << "MA_performIncomingTcpPacket: No flowUnit match for incoming TCP packet" << endl;
                return;
            }
        } else {
            funit->state = REGISTERED;
        }
        IPv6Address fakeIp;
        fakeIp.set(0x1D << 24, 0, (agentId >> 32) & 0xFFFFFFFF, agentId & 0xFFFFFFFF);
        IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
        ipControlInfo->setProtocol(IP_PROT_TCP);
        ipControlInfo->setDestAddr(fakeIp);
        ipControlInfo->setSrcAddr(funit->nodeAddress);
        ipControlInfo->setInterfaceId(101);
        ipControlInfo->setHopLimit(controlInfo->getHopLimit());
        ipControlInfo->setTrafficClass(controlInfo->getTrafficClass());
        ipControlInfo->setOrigDatagram(controlInfo->removeOrigDatagram());
        tcpseg->setControlInfo(ipControlInfo);
        cGate *outgate = gate("toTCP");
        send(tcpseg, outgate);
    } else
        throw cRuntimeError("MA:procDAmsg: TCP packet could not be cast.");
}

void MobileAgent::processOutgoingTcpPacket(cMessage *msg)
{
    cObject *controlInfoPtr = msg->removeControlInfo();
    if (dynamic_cast<IPv6ControlInfo *>(controlInfoPtr) != nullptr)
    {
        IPv6ControlInfo *controlInfo = (IPv6ControlInfo *) controlInfoPtr;
        if (dynamic_cast<tcp::TCPSegment *>(msg) != nullptr)
        {
            tcp::TCPSegment *tcpseg = (tcp::TCPSegment *) msg;
            PacketTimerKey packet(IP_PROT_TCP,controlInfo->getDestinationAddress().toIPv6(),tcpseg->getDestinationPort(),tcpseg->getSourcePort());
            if(true) { // TODO check impact
                FlowTuple tuple;
                tuple.protocol = IP_PROT_TCP;
                if(enableNodeRequesting) {
                    tuple.destPort = 0;
                    tuple.sourcePort = 0;
                } else {
                    tuple.destPort = tcpseg->getDestinationPort();
                    tuple.sourcePort = tcpseg->getSourcePort();
                }
                tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
                tuple.interfaceId = controlInfo->getDestinationAddress().toIPv6().getInterfaceId();
                FlowUnit *funit = getFlowUnit(tuple);
                if(funit->state == UNREGISTERED) {
                    EV_DEBUG << "==> Unregistered flow detected. Creating flow request." << endl;
                    funit->nodeAddress = controlInfo->getDestinationAddress().toIPv6();
                    if(createFlowRequest(tuple))
                        funit->state = REGISTERING;
                    EV_DEBUG << "==> tuple: IP= " << controlInfo->getDestinationAddress().toIPv6() << " Pd=" << tcpseg->getDestinationPort() << " Ps=" << tcpseg->getSourcePort() << " Id=" << controlInfo->getDestinationAddress().toIPv6().getInterfaceId() << endl;
//                    createFlowRequest(tuple); // invoking flow initialization and delaying pkt by sending in queue of this object (hint: scheduleAt with delay)
                    PacketTimer *pkt = getPacketTimer(packet); // set packet in queue
                    pkt->packet = msg; // replaced by new packet
                    pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    msg->setKind(MSG_TCP_RETRANSMIT);
                    msg->setControlInfo(controlInfo);
                    msg->setContextPointer(pkt);
                    scheduleAt(pkt->nextScheduledTime, msg);
                } else if(funit->state == REGISTERING) {
                    EV_DEBUG << "==> Registering flow. Checking for response." << endl;
                    if(isAddressAssociated(controlInfo->getDestinationAddress().toIPv6())) {
                        IPv6Address *ag = getAssociatedAddress(controlInfo->getDestinationAddress().toIPv6());
                        if(!ag)
                            throw cRuntimeError("MA:tcpIn: getAssociatedAddr fetched empty pointer instead of address.");
                        EV_DEBUG << "==> flow response received. Restarting TCP process with associated agent=" << ag->str() << " node=" <<  controlInfo->getDestinationAddress().toIPv6().str() << endl;
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
                        EV_DEBUG << "==> Flow response still not received. Waiting for response by CA." << endl;
                        if(isPacketTimerQueued(packet)) { // check is packet is in queue
                            PacketTimer *pkt = getPacketTimer(packet);
                            if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                                EV_DEBUG << "==> new message from upper layer arrived. Replacing message and setting scheduling time." << endl;
                                cancelPacketTimer(packet); // packet is in queue, cancel scheduling
                                delete pkt->packet; // old packet is...
                                pkt->packet = msg; // replaced by new packet
                                pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                            } else {
                                EV_DEBUG << "==> processing same message (not new msg from upper layer). Just setting scheduling time." << endl;
                                pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                            }
                            msg->setContextPointer(pkt);
                            msg->setKind(MSG_TCP_RETRANSMIT);
                            msg->setControlInfo(controlInfo);
                            scheduleAt(pkt->nextScheduledTime+1, msg);
                        } else
                            throw cRuntimeError("MA:tcpIn: Message is not in queue during phase of flow registering.");
                    }
                } else if(funit->state == REGISTERED) {
                    if(isPacketTimerQueued(packet)) { // check is packet is in queue
                        PacketTimer *pkt = getPacketTimer(packet); // pop the relating packet from queue
                        if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                            deletePacketTimer(packet); // deletes older message and removes the key from the queue.
                        } else {
                            deletePacketTimerEntry(packet); // removes just from the queue but does not delete message
                        }
                    } else {
                    }
                    sendUpperLayerPacket(tcpseg, controlInfo, funit->dataAgent, IP_PROT_TCP);
                } else
                    throw cRuntimeError("MA:tcpIn: Not known state of flow. What should be done?");
            } else
            { // interface is down, so schedule for later transmission
                if(isPacketTimerQueued(packet)) { // check if packet is in queue
                    PacketTimer *pkt = getPacketTimer(packet); // packet is cancelled
                    if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                        EV_DEBUG << "==> Interface is down and a packet is in queue. Replacing new message with older one." << endl;
                        cancelPacketTimer(packet); // packet is in queue, cancel scheduling
                        delete pkt->packet; // old packet is...
                        pkt->packet = msg; // replaced by new packet
                        pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    } else {
                        EV_DEBUG << "==> Interface is down and this packet is already in queue. Setting new schedule time." << endl;
                        pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                    }
                    msg->setKind(MSG_TCP_RETRANSMIT);
                    msg->setContextPointer(pkt);
                    msg->setControlInfo(controlInfo);
                    scheduleAt(pkt->nextScheduledTime, msg);
                } else { // first packet of socket
                    EV_DEBUG << "==> Interface is down and (first) packet of flow is inserted in queue." << endl;
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
    } else {
        EV_WARN << "MA_processOutgoingTcpPacket: ControlInfo could not be extracted from TCP packet." << endl;
        cGate *outgate = gate("toLowerLayer");
        send(msg, outgate);
    }
}

void MobileAgent::processIncomingIcmpPacket(IdentificationHeader *agentHeader, IPv6ControlInfo *controlInfo)
{
    cPacket *packet = agentHeader->decapsulate();
    if (dynamic_cast<ICMPv6Message *>(packet) != nullptr) {
        ICMPv6Message *icmp =  (ICMPv6Message *) packet;
        FlowTuple tuple;
        tuple.protocol = IP_PROT_IPv6_ICMP;
        tuple.destPort = 0;
        tuple.sourcePort = 0;
        tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
        tuple.interfaceId = agentHeader->getId();
        FlowUnit *funit = getFlowUnit(tuple);
        if(funit->state == UNREGISTERED) {
            if(!enableNodeRequesting) {
                EV_WARN << "MA_processIncomingIcmpPacket: No flowUnit match for incoming ICMP packet" << endl;
                return;
            }
        } else {
            funit->state = REGISTERED;
        }
        IPv6Address fakeIp;
        fakeIp.set(0x1D << 24, 0, (agentId >> 32) & 0xFFFFFFFF, agentId & 0xFFFFFFFF);
        IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
        ipControlInfo->setProtocol(IP_PROT_IPv6_ICMP);
        ipControlInfo->setDestAddr(fakeIp);
        ipControlInfo->setSrcAddr(funit->nodeAddress);
        ipControlInfo->setInterfaceId(100);
        ipControlInfo->setHopLimit(controlInfo->getHopLimit());
        ipControlInfo->setTrafficClass(controlInfo->getTrafficClass());
        ipControlInfo->setOrigDatagram(controlInfo->removeOrigDatagram());
        processIncomingIcmpPacket(icmp, ipControlInfo);
    } else
        throw cRuntimeError("MA_processIncomingIcmpPacket: ICMP packet could not be cast.");
}

void MobileAgent::processIncomingIcmpPacket(ICMPv6Message *icmp, IPv6ControlInfo *controlInfo)
{
    icmp->setControlInfo(controlInfo);
    cGate *outgate = gate("toICMP");
    send(icmp, outgate);
}

void MobileAgent::processOutgoingIcmpPacket(cMessage *msg)
{
    cObject *ctrl = msg->removeControlInfo();
    if (dynamic_cast<IPv6ControlInfo *>(ctrl) != nullptr) {
        IPv6ControlInfo *controlInfo = (IPv6ControlInfo *) ctrl;
        if (dynamic_cast<ICMPv6Message *>(msg) != nullptr) {
            ICMPv6Message *icmp = (ICMPv6Message *) msg;
//            if(simTime() >= 10) {
////                EV << "ID:" << funit->id << endl;
////    //            EV << "MA:" << funit->mobileAgent << endl;
////                EV << "CN:" << funit->nodeAddress << endl;
////                EV << "DEST:" << controlInfo->getDestinationAddress() << endl;
////                EV << "SRC:" << controlInfo->getSourceAddress() << endl;
//                throw cRuntimeError("CHECK POINT");
//            }
            if(icmp->getType() == ICMPv6_ECHO_REQUEST || icmp->getType() == ICMPv6_ECHO_REPLY) {
                PacketTimerKey packet(IP_PROT_IPv6_ICMP,controlInfo->getDestinationAddress().toIPv6(),0,0);
                if(isInterfaceUp()) {
                    FlowTuple tuple;
                    tuple.protocol = IP_PROT_IPv6_ICMP;
                    tuple.destPort = 0;
                    tuple.sourcePort = 0;
                    tuple.destAddress = IPv6Address::UNSPECIFIED_ADDRESS;
                    tuple.interfaceId = controlInfo->getDestinationAddress().toIPv6().getInterfaceId();
                    FlowUnit *funit = getFlowUnit(tuple);
                    if(funit->state == UNREGISTERED) {
                        funit->nodeAddress = controlInfo->getDestinationAddress().toIPv6();
                        if(createFlowRequest(tuple))
                            funit->state = REGISTERING;
//                        createFlowRequest(tuple); // invoking flow initialization and delaying pkt by sending in queue of this object (hint: scheduleAt with delay)
                        PacketTimer *pkt = getPacketTimer(packet); // set packet in queue
                        pkt->packet = msg; // replaced by new packet
                        pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                        msg->setKind(MSG_ICMP_RETRANSMIT);
                        msg->setContextPointer(pkt);
                        msg->setControlInfo(controlInfo);
                        scheduleAt(pkt->nextScheduledTime, msg);
                    } else if(funit->state == REGISTERING) {
                        EV_DEBUG << "==> Registering Ping flow. Checking for response." << endl;
                        if(isAddressAssociated(controlInfo->getDestinationAddress().toIPv6())) {
                            IPv6Address *ag = getAssociatedAddress(controlInfo->getDestinationAddress().toIPv6());
                            if(!ag)
                                throw cRuntimeError("MA:tcpIn: getAssociatedAddr fetched empty pointer instead of address.");
                            EV_DEBUG << "==> flow response received. Restarting Ping process with associated agent=" << ag->str() << " node=" <<  controlInfo->getDestinationAddress().toIPv6().str() << endl;
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
                            EV_DEBUG << "==> Flow response not yet received. Waiting for response by CA." << endl;
                            if(isPacketTimerQueued(packet)) { // check is packet is in queue
                                PacketTimer *pkt = getPacketTimer(packet);
                                if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                                    EV_DEBUG << "==> new message from upper layer arrived. Replacing message and setting scheduling time." << endl;
                                    cancelPacketTimer(packet); // packet is in queue, cancel scheduling
                                    delete pkt->packet; // old packet is...
                                    pkt->packet = msg; // replaced by new packet
                                    pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                                } else {
                                    EV_DEBUG << "==> processing same message (not new msg from upper layer). Just setting scheduling time." << endl;
                                    pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                                }
                                msg->setKind(MSG_ICMP_RETRANSMIT);
                                msg->setContextPointer(pkt);
                                msg->setControlInfo(controlInfo);
                                scheduleAt(pkt->nextScheduledTime+0.1, msg); // PING RETRANSMISSION 0.1s
                            } else
                                throw cRuntimeError("MA:pingIn: Message is not in queue during phase of flow registering.");
                        }
                    } else if(funit->state == REGISTERED) {
                        if(isPacketTimerQueued(packet)) { // check is packet is in queue
                            PacketTimer *pkt = getPacketTimer(packet); // pop the relating packet from queue
                            if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                                deletePacketTimer(packet); // deletes older message and removes the key from the queue.
                            } else {
                                deletePacketTimerEntry(packet); // removes just from the queue but does not delete message
                            }
//                        } else {
                        }
                        sendUpperLayerPacket(icmp, controlInfo, funit->dataAgent, IP_PROT_IPv6_ICMP);
                    } else
                        throw cRuntimeError("MA:pingIn: Not known state of flow. What should be done?");
                } else
                { // interface is down, so schedule for later transmission
                    if(isPacketTimerQueued(packet)) { // check if packet is in queue
                        PacketTimer *pkt = getPacketTimer(packet); // packet is cancelled
                        if(msg->getCreationTime() > pkt->packet->getCreationTime()) { // indicates that message is newer,
                            EV_DEBUG << "==> Interface is down and a packet is in queue. Replacing new message with older one." << endl;
                            cancelPacketTimer(packet); // packet is in queue, cancel scheduling
                            delete pkt->packet; // old packet is...
                            pkt->packet = msg; // replaced by new packet
                            pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                        } else {
                            EV_DEBUG << "==> Interface is down and this packet is already in queue. Setting new schedule time." << endl;
                            pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                        }
                        msg->setKind(MSG_ICMP_RETRANSMIT);
                        msg->setContextPointer(pkt);
                        msg->setControlInfo(controlInfo);
                        scheduleAt(pkt->nextScheduledTime, msg);
                    } else { // first packet of socket
                        EV_DEBUG << "==> Interface is down and (first) packet of flow is inserted in queue." << endl;
                        PacketTimer *pkt = getPacketTimer(packet); // inserting packet into queue
                        pkt->packet = msg;
                        pkt->nextScheduledTime = simTime()+TIMEDELAY_PKT_PROCESS;
                        msg->setKind(MSG_ICMP_RETRANSMIT);
                        msg->setContextPointer(pkt);
                        msg->setControlInfo(controlInfo);
                        scheduleAt(pkt->nextScheduledTime, msg);
                    }
                }
            } else { // regular ICMP messages, so forward directly
                icmp->setControlInfo(controlInfo);
                cGate *outgate = gate("icmpIpOut");
                send(icmp, outgate);
            }
        } else {
            throw cRuntimeError("MA: Incoming message could not be casted to a ICMP packet.");
        }
    } else {
        EV_WARN << "MA_processOutgoingIcmpPacket: ControlInfo could not be extracted from ICMP packet." << endl;
        cGate *outgate = gate("toLowerLayer");
        send(msg, outgate);
    }
}

void MobileAgent::sendUpperLayerPacket(cPacket *packet, IPv6ControlInfo *controlInfo, IPv6Address agentAddr, short prot)
{
    if(sessionState == ASSOCIATED && seqnoState == ASSOCIATED) {
        IdentificationHeader *ih = createAgentHeader(1, prot, getSeqNo(agentId), getAckNo(agentId), agentId);
        ih->setIsIdInitialized(true);
        ih->setIsIdAcked(true);
        ih->setIsSeqValid(true);
        ih->setIsAckValid(true);
        ih->setIsWithNodeAddr(true);
        if(getSeqNo(agentId) != getAckNo(agentId)) {
            ih->setIsIpModified(true);
            sequenceUpdateDaStat++;
            emit(sequenceUpdateDa, sequenceUpdateDaStat);
        }
        AddressDiff ad = getAddressList(agentId,getSeqNo(agentId),getAckNo(agentId));
        ih->setIPaddressesArraySize(ad.insertedList.size()+ad.deletedList.size()+1); // +1 because we send node address
        ih->setIpAddingField(ad.insertedList.size());
        ih->setIPaddresses(0,controlInfo->getDestinationAddress().toIPv6());
        if(ad.insertedList.size() > 0) {
            for(int idx=0; idx < ad.insertedList.size(); idx++)
                ih->setIPaddresses(idx+1, ad.insertedList.at(idx).address);
        }
        ih->setIpRemovingField(ad.deletedList.size());
        if(ad.deletedList.size() > 0) {
            for(int idx=0; idx < ad.deletedList.size(); idx++)
                ih->setIPaddresses(idx+1+ad.insertedList.size(), ad.deletedList.at(idx).address);
        }
        ih->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR*(ad.insertedList.size()+ad.deletedList.size()+1));
        ih->encapsulate(packet);
        ih->setName(packet->getName());
        controlInfo->setProtocol(IP_PROT_IPv6EXT_ID);
        controlInfo->setDestinationAddress(agentAddr);
        // Set here scheduler configuration
        controlInfo->setInterfaceId(-1);
        controlInfo->setSrcAddr(IPv6Address::UNSPECIFIED_ADDRESS);
        determineInterface(IPv6Address::UNSPECIFIED_ADDRESS);
        ih->setControlInfo(controlInfo); // make copy before setting param
        cGate *outgate = gate("toLowerLayer");
        dataSignalLoadStat += ih->getByteLength();
        emit(dataSignalLoad, dataSignalLoadStat);
        send(ih, outgate);
    } else if(sessionState != ASSOCIATED)
        createSessionInit();
    else if(seqnoState != ASSOCIATED)
        createSequenceInit();
    else
        throw cRuntimeError("MA_sendUpperLayerPacket: Not possible condition");
}

void MobileAgent::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    Enter_Method_Silent();
    if(signalID == NF_INTERFACE_IPv6CONFIG_CHANGED) { // is triggered when address is modified
        if(dynamic_cast<InterfaceEntryChangeDetails *>(obj)) {
            InterfaceEntry *ie = check_and_cast<const InterfaceEntryChangeDetails *>(obj)->getInterfaceEntry();
//            EV_DEBUG << "------- CONFIG_CHANGED ------- :\nInfo:" << obj->info() << endl;
            processInterfaceChange(ie->getInterfaceId()); // dadTimeout event
        }
    }
    if(signalID == NF_INTERFACE_STATE_CHANGED) { // is triggered when carrier setting is changed
        if(dynamic_cast<InterfaceEntryChangeDetails *>(obj)) {
            InterfaceEntry *ie = check_and_cast<const InterfaceEntryChangeDetails *>(obj)->getInterfaceEntry();
//            EV_DEBUG << "------- STATE_CHANGED ------- :\nInfo:" << obj->info() << endl;
            if(ie->isUp())
                processInterfaceUp(ie->getInterfaceId()); // assocResp-OK event
            else
                processInterfaceDown(ie->getInterfaceId()); // beaconTimeout event
        }
    }
}

void MobileAgent::processInterfaceUp(int id)
{
    EV << "MA_processInterfaceUp: Create Interface UP Timer message for Id=" << id << endl;
    if(!cancelAndDeleteExpiryTimer(IPv6Address::UNSPECIFIED_ADDRESS, id,TIMERKEY_IF_DOWN)) // no l2 disassociation timer exists
    {
        int i;
        for (i=0; i<ift->getNumInterfaces(); i++) {
            if(ift->getInterface(i)->getInterfaceId() == id) break;
        }
        TimerKey key(IPv6Address::UNSPECIFIED_ADDRESS, id, TIMERKEY_IF_UP);
        InterfaceUpTimer *iut = new InterfaceUpTimer();
        cMessage *msg = new cMessage("InterfaceUpTimer", MSG_IF_UP);
        iut->ie = ift->getInterface(i);
        iut->timer = msg;
        iut->nextScheduledTime = simTime()+TIMEDELAY_IF_UP; // TIMEDELAY_IF_UP = 0
        msg->setContextPointer(iut);
        scheduleAt(iut->nextScheduledTime, msg);
    } else {
        EV << "MA_processInterfaceUp: A timer for event 'InterfaceDownTimer' was configured before. Now deleted." << endl;
    }
}

void MobileAgent::handleInterfaceUp(cMessage *msg)
{
    InterfaceUpTimer *l2at = (InterfaceUpTimer *) msg->getContextPointer();
    InterfaceEntry *ie = l2at->ie;
    if(ie->ipv6Data()->getPreferredAddress().isGlobal()) {
        // check if ip address has changed after interface has been attached to an AP.
        // if so, update sequence. otherwise rollback sequence update because it is not needed to update. + delete update seq.
        if(isInterfaceInserted(agentId, getSeqNo(agentId) - 1, ie->getInterfaceId()) && (seqnoState == ASSOCIATED)) {
            AddressTuple tuple = getAddressTuple(agentId, getSeqNo(agentId) - 1, ie->getInterfaceId());
            if(tuple.address == ie->ipv6Data()->getPreferredAddress()) {
                EV_DEBUG << "MA_handleInterfaceUp: Rolling table entry back to previous sequence number." << endl;
                cancelAndDeleteExpiryTimer(ControlAgentAddress, -1,TIMERKEY_SEQ_UPDATE, agentId, getSeqNo(agentId), getAckNo(agentId));
                if(getSeqNo(agentId) == getAckNo(agentId)) { // this is the case when we use multipe interfaces at same time
                    if(!isInterfaceInserted(agentId, getSeqNo(agentId), ie->getInterfaceId())) {
                        EV_DEBUG << "MA_handleInterfaceUp: Address of interface does not exist in table. Inserting Interface." << endl;
                        insertAddress(agentId, ie->getInterfaceId(), ie->ipv6Data()->getPreferredAddress());
                        createSequenceUpdate(getSeqNo(agentId), getAckNo(agentId)); // next address must be updated by seq update
                    }
                } else {
                    setSeqNo(agentId, getSeqNo(agentId) - 1);
                }
            } else {
                EV_DEBUG << "MA_handleInterfaceUp: Address of interface changed. skipping operation and waiting for address configuration." << endl;
            }
        } else {
            if(!isInterfaceInserted(agentId, getSeqNo(agentId), ie->getInterfaceId())) {
                EV_DEBUG << "MA_handleInterfaceUp: Address of interface does not exist in table. Inserting Interface." << endl;
                insertAddress(agentId, ie->getInterfaceId(), ie->ipv6Data()->getPreferredAddress());
            }
            if(seqnoState == ASSOCIATED) {
                createSequenceUpdate(getSeqNo(agentId), getAckNo(agentId)); // next address must be updated by seq update
                sendAllPacketsInQueue();
            } else {
                createSessionInit();
            }
        }
    } else {
        EV_DEBUG << "MA_handleInterfaceUp: Deleted Interface UP Timer, Interface has not acquired a global address. Address=" << ie->ipv6Data()->getPreferredAddress()  << endl;
    }
    cancelExpiryTimer(IPv6Address::UNSPECIFIED_ADDRESS,ie->getInterfaceId(),TIMERKEY_IF_UP);
    delete msg;
}

// Sets up a timer to process a link disassociation. The timer expires in three seconds. If
void MobileAgent::processInterfaceDown(int id)
{
    EV_DEBUG << "MA: Create Interface DOWN Timer message for Id=" << id << endl;
//    if(!cancelAndDeleteExpiryTimer(IPv6Address::UNSPECIFIED_ADDRESS, id,TIMERKEY_IF_DOWN)) {// no l2 disassociation timer exists
    if(!pendingExpiryTimer(IPv6Address::UNSPECIFIED_ADDRESS, id,TIMERKEY_IF_DOWN)) {
        int i;
        for (i=0; i<ift->getNumInterfaces(); i++) {
            if(ift->getInterface(i)->getInterfaceId() == id) break;
        }
        TimerKey key(IPv6Address::UNSPECIFIED_ADDRESS,id,TIMERKEY_IF_DOWN);
        InterfaceDownTimer *idt = (InterfaceDownTimer*) getExpiryTimer(key, TIMERTYPE_IF_DOWN);
        cMessage *msg = new cMessage("InterfaceDownTimer", MSG_IF_DOWN);
        idt->ie = ift->getInterface(i);
        idt->timer = msg;
        idt->nextScheduledTime = simTime()+TIMEDELAY_IF_DOWN;
        msg->setContextPointer(idt);
        scheduleAt(idt->nextScheduledTime, msg);
    } else {
        EV_WARN << "MA_processInterfaceDown: A timer for 'InterfaceDownTimer' was set before. Should not happen as an interface cannot disassociate from a link when it was already disassociated." << endl;
    }
}

// Handles interface changes
void MobileAgent::handleInterfaceDown(cMessage *msg)
{
    InterfaceDownTimer *idt = (InterfaceDownTimer *) msg->getContextPointer();
    InterfaceEntry *ie = idt->ie;
    if(ie->ipv6Data()->getPreferredAddress().isGlobal()) {
        EV_DEBUG << "MA: Received deleteRequest for interface " << ie->getInterfaceId() << endl;
        deleteAddress(agentId, ie->getInterfaceId(), ie->ipv6Data()->getPreferredAddress());
        if(seqnoState == ASSOCIATED ){
            if(isInterfaceAssociated())
                createSequenceUpdate(getSeqNo(agentId), getAckNo(agentId)); // next address must be updated by seq update
        } else {
            createSessionInit();
        }
    } else {
        EV << "MA: Interface down signal received but IP address is not global. Address=" << ie->ipv6Data()->getPreferredAddress()  << endl;
    }
    cancelExpiryTimer(IPv6Address::UNSPECIFIED_ADDRESS,ie->getInterfaceId(),TIMERKEY_IF_DOWN);
    delete msg;
}

// Sets up a timer for interface changes.
void MobileAgent::processInterfaceChange(int id)
{
    EV_DEBUG << "MA_processInterfaceChange: Create Interface CHANGE Timer message for Id=" << id << endl;
    if(!cancelAndDeleteExpiryTimer(IPv6Address::UNSPECIFIED_ADDRESS, id,TIMERKEY_IF_CHANGE)) {// no l2 disassociation timer exists
    // no timer exists
        EV << "MA_processInterfaceChange: Interface change timer does not exist." << endl;
    } else {
        EV << "MA_processInterfaceChange: Interface change timer canceled." << endl;
    }
    int i;
    for (i=0; i<ift->getNumInterfaces(); i++) {
        if(ift->getInterface(i)->getInterfaceId() == id) break;
    }
    TimerKey key(IPv6Address::UNSPECIFIED_ADDRESS, id, TIMERKEY_IF_CHANGE);
    InterfaceChangeTimer *ict = (InterfaceChangeTimer*) getExpiryTimer(key, TIMERTYPE_IF_CHANGE);
    cMessage *msg = new cMessage("InterfaceChangeTimer", MSG_IF_CHANGE);
    ict->ie = ift->getInterface(i);
    ict->timer = msg;
    ict->nextScheduledTime = simTime()+TIMEDELAY_IF_CHANGE; // delay is 0
    msg->setContextPointer(ict);
    scheduleAt(ict->nextScheduledTime, msg);
}

// Processes changes of the interface state.
void MobileAgent::handleInterfaceChange(cMessage *msg)
{
    EV_INFO << "MA_handleInterfaceChange: Handling Interface change signal."<< endl;
    InterfaceChangeTimer *ict = (InterfaceChangeTimer *) msg->getContextPointer();
    InterfaceEntry *ie = ict->ie;
    if(ie->ipv6Data()->getPreferredAddress().isGlobal()) {
        if(seqnoState == ASSOCIATED && sessionState == ASSOCIATED) {
            EV << "seqnoState : " << seqnoState << " sessionState : " << sessionState << endl;
            if(isInterfaceInserted(agentId, getSeqNo(agentId), ie->getInterfaceId())) {
                // interface is already inserted => check if address is same.
                if(isAddressInserted(agentId, getSeqNo(agentId), ie->ipv6Data()->getPreferredAddress())) {
                    // address is already in ADS => do nothing (nothing changed)
                    EV << "MA_handleInterfaceChange: Interface change signal received but nothing to update." << endl;
                } else {
                    // ip change
                    AddressTuple oldAddress = getAddressTuple(agentId, getSeqNo(agentId), ie->getInterfaceId());
                    deleteAddress(agentId, ie->getInterfaceId(), oldAddress.address);
                    insertAddress(agentId, ie->getInterfaceId(), ie->ipv6Data()->getPreferredAddress());
//                    sendAllPacketsInQueue(); // should be empty at this stage
                    // debug
                    int s = getSeqNo(agentId);
                    int a = getAckNo(agentId);
                    EV_DEBUG << "MA_handleInterfaceChange: S=" << s << " A=" << a << " Del=" << oldAddress.address.str() << " Add=" << ie->ipv6Data()->getPreferredAddress() << " If=" << ie->getInterfaceId() << endl;
                }
            } else {
                // interface is not inserted
                insertAddress(agentId, ie->getInterfaceId(), ie->ipv6Data()->getPreferredAddress());
                // debug
                int s = getSeqNo(agentId);
                int a = getAckNo(agentId);
                EV_DEBUG << "MA_handleInterfaceChange: S=" << s << " A=" << a << " Add=" << ie->ipv6Data()->getPreferredAddress() << " If=" << ie->getInterfaceId() << endl;
            }
            sendAllPacketsInQueue();
            createSequenceUpdate(getSeqNo(agentId), getAckNo(agentId)); // next address must be updated by seq update
            cancelExpiryTimer(IPv6Address::UNSPECIFIED_ADDRESS,ie->getInterfaceId(),TIMERKEY_IF_DOWN);
        } else if(sessionState == UNASSOCIATED) {
            EV_DEBUG << "MA_handleInterfaceChange: creating session init for first associated interface." << endl;
            insertAddress(agentId, ie->getInterfaceId(), ie->ipv6Data()->getPreferredAddress());
            createSessionInit(); // first address is initialzed with session init
        } else { // delaying process if one were started
            EV_DEBUG << "MA: delaying interface init." << endl;
            scheduleAt(simTime()+TIMEDELAY_IFACE_INIT, msg);
            return;
        }
    } else {
        EV << "MA_handleInterfaceChange: Interface change signal received but IP address is not global. Address=" << ie->ipv6Data()->getPreferredAddress() << endl;
    }
    cancelExpiryTimer(IPv6Address::UNSPECIFIED_ADDRESS,ie->getInterfaceId(),TIMERKEY_IF_CHANGE);
    delete msg;
}

void MobileAgent::receiveSignal(cComponent *source, simsignal_t signalID, double d)
{
    Enter_Method_Silent();
    if (dynamic_cast<physicallayer::Ieee80211Radio *>(source) != nullptr) {
        physicallayer::Ieee80211Radio *radio = (physicallayer::Ieee80211Radio *) source;
        if(radio) {
            ieee80211::Ieee80211Mac *mac = (ieee80211::Ieee80211Mac *) radio->getParentModule()->getSubmodule("mac");
            if(mac) {
                if(signalID == physicallayer::Radio::minSNIRSignal) {
                    LinkBuffer *lb = getLinkBuffer(mac->getMacAddress());
                    LinkUnit *lu = new LinkUnit(d, simTime());
                    addLinkUnit(lb,lu);
                    emit(interfaceSnir, d);
                    interfaceSnirStat = d;
                }
//                if(signalID == physicallayer::Radio::packetErrorRateSignal) {
//                    LinkUnit *lu = getLinkUnit(mac->getMacAddress());
//                    lu->per = d;
//                    EV << "MA: PER=" << d << " MAC= " << mac->getMacAddress() << endl;
//                }
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

// Returns the queue for the given mac address
MobileAgent::LinkBuffer *MobileAgent::getLinkBuffer(MACAddress mac)
{
    LinkBuffer *lb;
    auto it = linkTable.find(mac);
    if(it != linkTable.end()) { // linkTable contains an instance of linkBuffer
        lb = it->second;
        return lb;
    } else {
        lb = new LinkBuffer();
        linkTable.insert(std::make_pair(mac,lb));
        EV << "MA_getLinkBuffer: Created MAC Entry: " << mac << endl;
        return lb;
    }
}

// Returns the queue for the given interface
MobileAgent::LinkBuffer *MobileAgent::getLinkBuffer(InterfaceEntry *ie)
{
    LinkBuffer *lb;
    auto it = linkTable.find(ie->getMacAddress());
    if(it != linkTable.end()) { // linkTable contains an instance of linkBuffer
        lb = it->second;
        return lb;
    }
    return lb = nullptr;
}

// Pushes the SNIR value of the link into the queue and removes values from the queue that are older then two seconds.
void MobileAgent::addLinkUnit(LinkBuffer* lb, LinkUnit *lu)
{
    if(!lb || !lu)
        throw cRuntimeError("MA_addLinkUnit: Pointer of addLinkUnit function should not be a nullptr.");
    if(lb->empty()) {
        lb->push_back(lu);
    } else {
        LinkUnit *unit = lb->front();
        if(!unit)
            throw cRuntimeError("MA_addLinkUnit: Pointer of LinkUnit should not be a nullptr.");
        while(unit) {
            if(unit->timestamp + INTERVAL_LINK_BUFFER < simTime()) {
                lb->pop_front();
                if(lb->empty())
                    break;
                unit = lb->front();
            } else {
                break;
            }
        }
        lb->push_back(lu);
    }
}

// Calculates the mean of SNIR of the given interface.
double MobileAgent::getMeanSnir(InterfaceEntry *ie)
{
    LinkBuffer *lb = getLinkBuffer(ie->getMacAddress());
    if(lb) {
        if(lb->empty())
            return 0;
        else {
            double snir = 0;
            double size = lb->size();
            std::deque<LinkUnit *>::iterator it = lb->begin();
            while(it != lb->end()) {
                snir += (*it++)->snir;
            }
            return snir/size;
        }
    } else {
        return 0;
    }
}

// Returns the interface with the highest SNIR based on the mean within the last two seconds.
void MobileAgent::determineInterface(IPv6Address destAddr, int destPort, int sourcePort, short protocol)
{ // const IPv6Address &destAddr,
    if(ift->getNumInterfaces() < 3)
        return;
    InterfaceEntry *ie = nullptr;
    double maxSnr
    = 0;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        if(!(ift->getInterface(i)->isLoopback()) && ift->getInterface(i)->isUp() && ift->getInterface(i)->ipv6Data()->getPreferredAddress().isGlobal()) {
            double snr = getMeanSnir(ift->getInterface(i));
            if(snr >= maxSnr) {
                maxSnr = snr; // selecting highest snr interface
                ie=ift->getInterface(i);
            }
        }
    }
    if(ie) {
        // setting interface to use.
        interfaceIdStat = ie->getInterfaceId();
        emit(interfaceId, interfaceIdStat);
        emit(NF_INTERFACE_ROUTING, interfaceIdStat); // selecting interface for routing
    }
}

// Returns true if any interface is associated to any link and acquired a global address. Otherwise it returns false.
bool MobileAgent::isInterfaceUp()
{
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        if(!(ift->getInterface(i)->isLoopback()) && ift->getInterface(i)->isUp()) {// && ift->getInterface(i)->ipv6Data()->getPreferredAddress().isGlobal()) {
            return true;
        }
    }
    return false;
}

bool MobileAgent::isInterfaceAssociated()
{
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        if(!(ift->getInterface(i)->isLoopback()) && ift->getInterface(i)->isUp() && ift->getInterface(i)->ipv6Data()->getPreferredAddress().isGlobal()) {
            return true;
        }
    }
    return false;
}

// Allows to send delayed/undelayed packets over any link.
void MobileAgent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, simtime_t delayTime)
{
    IPv6ControlInfo *controlInfo = new IPv6ControlInfo();
    controlInfo->setProtocol(IP_PROT_IPv6EXT_ID); // todo must be adjusted
    controlInfo->setDestAddr(destAddr);
    controlInfo->setHopLimit(32);
//    InterfaceEntry *ie = getInterface(destAddr); // returns the interface with highest SNIR
//    if(ie) {
//        controlInfo->setInterfaceId(ie->getInterfaceId());
//        controlInfo->setSrcAddr(ie->ipv6Data()->getPreferredAddress());
//    }
    controlInfo->setInterfaceId(-1);
    controlInfo->setSrcAddr(IPv6Address::UNSPECIFIED_ADDRESS);
    msg->setControlInfo(controlInfo);
    cGate *outgate = gate("toLowerLayer");
    if (delayTime > 0) {
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

// Pops and sends packets that were stored during Data Agent query
void MobileAgent::sendAllPacketsInQueue()
{
    for(auto it : packetQueue) {
        PacketTimerKey key = it.first;
        cancelPacketTimer(key);
        PacketTimer *pkt = it.second;
        pkt->nextScheduledTime = simTime();
        if(pkt->packet->getKind() == MSG_TCP_RETRANSMIT || pkt->packet->getKind() == MSG_UDP_RETRANSMIT || pkt->packet->getKind() == MSG_ICMP_RETRANSMIT)
            scheduleAt(pkt->nextScheduledTime, pkt->packet);
//        else
//            throw cRuntimeError("MA_sendAllPacketsInQueue: message type is unknown. Set kind to tcp or upd.");
    }
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
