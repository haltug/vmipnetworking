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
        mobileId = (uint64) rand(); // TODO should be placed in future
        am.initiateAddressMap(mobileId, 10);
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
        IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
        ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID);
        interfaceNotifier = getContainingNode(this);
        interfaceNotifier->subscribe(NF_INTERFACE_IPv6CONFIG_CHANGED,this);
    }
    if (stage == INITSTAGE_APPLICATION_LAYER) {
        simtime_t startTime = par("startTime");
        cMessage *timeoutMsg = new cMessage("sessionStart");
        timeoutMsg->setKind(MSG_START_TIME);
//            scheduleAt(startTime, timeoutMsg); // delaying start
    }

    WATCH(mobileId);
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
            processIncomingUDPPacket(msg, controlInfo);
        }
        else if(msg->getKind() == MSG_TCP_RETRANSMIT) { // from MA
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processIncomingTCPPacket(msg, controlInfo);
        }
        else
            throw cRuntimeError("handleMessage: Unknown timer expired. Which timer msg is unknown?");
    }
    else if(msg->arrivedOn("fromUDP")) {
        cObject *ctrl = msg->removeControlInfo();
        if (dynamic_cast<IPv6ControlInfo *>(ctrl) != nullptr) {
//            EV << "MA: Received packet fromUDP" << endl;
            IPv6ControlInfo *controlInfo = (IPv6ControlInfo *) ctrl;
            processIncomingUDPPacket(msg, controlInfo);
        }
        else { EV << "MA: Received udp packet but not with ipv6 ctrl info." << endl; }
    }
    else if(msg->arrivedOn("fromTCP")) {
        cObject *ctrl = msg->removeControlInfo();
        if (dynamic_cast<IPv6ControlInfo *>(ctrl) != nullptr) {
            EV << "MA: Received packet fromTCP" << endl;
            IPv6ControlInfo *controlInfo = (IPv6ControlInfo *) ctrl;
            processIncomingTCPPacket(msg, controlInfo);
        }
        else { EV << "MA: Received tcp packet but not with ipv6 ctrl info." << endl; }
    }
    else if(msg->arrivedOn("fromLowerLayer")) {
        if (dynamic_cast<IdentificationHeader *> (msg)) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            IdentificationHeader *idHdr = (IdentificationHeader *) msg;
            if (dynamic_cast<ControlAgentHeader *>(idHdr)) {
                ControlAgentHeader *ca = (ControlAgentHeader *) idHdr;
                processControlAgentMessage(ca, controlInfo);
            } else if (dynamic_cast<DataAgentHeader *>(idHdr)) {
                DataAgentHeader *da = (DataAgentHeader *) idHdr;
                processDataAgentMessage(da, controlInfo);
            } else {
                throw cRuntimeError("A:handleMsg: Extension Hdr Type not known. What did you send?");
            }
        }
    }
    else
        throw cRuntimeError("A:handleMsg: cMessage Type not known. What did you send?");
}

void MobileAgent::createSessionInit() {
    EV << "MA: Create CA_init" << endl;
    L3Address caAddr;
    const char *controlAgentAddr;
    controlAgentAddr = par("controlAgentAddress");
    L3AddressResolver().tryResolve(controlAgentAddr, caAddr);
    caAddress = caAddr.toIPv6();
    if(caAddress.isGlobal()) {
        if(sessionState == UNASSOCIATED) sessionState = INITIALIZING;
        InterfaceEntry *ie = getInterface(caAddress); // TODO ie not correctly set
        if(!ie) { throw cRuntimeError("MA: No interface exists."); }
        cMessage *msg = new cMessage("sendingCAinit",MSG_SESSION_INIT);
        TimerKey key(caAddress,ie->getInterfaceId(),TIMERKEY_SESSION_INIT);
        SessionInitTimer *sit = (SessionInitTimer *) getExpiryTimer(key,TIMERTYPE_SESSION_INIT);
        sit->dest = caAddress;
        sit->ie = ie;
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
    InterfaceEntry *ie = sit->ie;
// TODO check for global address if not skip this round
    const IPv6Address &dest = sit->dest = caAddress;;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress(); // to get src ip
    sit->nextScheduledTime = simTime() + sit->ackTimeout;
    sit->ackTimeout = (sit->ackTimeout)*2;
    if(sit->dest.isGlobal()) { // not necessary
        MobileAgentHeader *mah = new MobileAgentHeader("ca_init");
        mah->setId(mobileId);
        mah->setIdInit(true);
        mah->setIdAck(false);
        mah->setSeqValid(false);
        mah->setAckValid(false);
        mah->setAddValid(false);
        mah->setRemValid(false);
        mah->setNextHeader(IP_PROT_NONE);
        mah->setByteLength(SIZE_AGENT_HEADER);
        sendToLowerLayer(mah, dest, src);
    }
    scheduleAt(sit->nextScheduledTime, msg);
}

void MobileAgent::createSequenceInit() { // does not support interface check
    EV << "MA: Create CA_seq_init" << endl;
    if(sessionState != ASSOCIATED) { throw cRuntimeError("MA: Not registered at CA. Cannot run seq init."); }
    if(seqnoState == UNASSOCIATED) { seqnoState = INITIALIZING; }
    cMessage *msg = new cMessage("sendingCAseqInit", MSG_SEQNO_INIT);
    InterfaceEntry *ie = getInterface(caAddress);
    if(!ie) { throw cRuntimeError("MA: No interface exists."); }
    TimerKey key(caAddress,ie->getInterfaceId(),TIMERKEY_SEQNO_INIT);
    SequenceInitTimer *sit = (SequenceInitTimer *) getExpiryTimer(key, TIMERTYPE_SEQNO_INIT);
    sit->dest = caAddress;
    sit->ie = ie;
    sit->timer = msg;
    sit->ackTimeout = TIMEOUT_SEQNO_INIT;
    sit->nextScheduledTime = simTime();
    msg->setContextPointer(sit);
    scheduleAt(sit->nextScheduledTime, msg);
}

void MobileAgent::sendSequenceInit(cMessage *msg) {
    EV << "MA: Send Seq_init_to_CA" << endl;
    SequenceInitTimer *sit = (SequenceInitTimer *) msg->getContextPointer();
    InterfaceEntry *ie = sit->ie;
    const IPv6Address &dest = sit->dest;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress();
    sit->nextScheduledTime = simTime() + sit->ackTimeout;
    sit->ackTimeout = (sit->ackTimeout)*2;
    MobileAgentHeader *mah = new MobileAgentHeader("ca_seq_init");
    mah->setId(mobileId);
    mah->setIdInit(true);
    mah->setIdAck(true);
    mah->setSeqValid(true);
    mah->setAckValid(false);
    mah->setAddValid(true);
    mah->setRemValid(false);
    mah->setNextHeader(IP_PROT_NONE);
    mah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    mah->setIpAddingField(1);
    mah->setIpRemovingField(0);
    mah->setIpAcknowledgementNumber(0);
    mah->setAddedAddressesArraySize(1);
    mah->setAddedAddresses(0,src);
    mah->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR);
    sendToLowerLayer(mah, dest, src);
    scheduleAt(sit->nextScheduledTime, msg);
}

void MobileAgent::createSequenceUpdate(uint64 mobileId, uint seq, uint ack) {
    EV << "MA: Create sequence update to CA" << endl;
    if(sessionState != ASSOCIATED &&  seqnoState != ASSOCIATED) { throw cRuntimeError("MA: Not registered at CA. Cannot run seq init."); }
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
        EV << "MA: Sending seq update without delay. interface provided." << endl;
        cMessage *msg = new cMessage("sendingCAseqUpdate", MSG_SEQ_UPDATE);
        sut->timer = msg;
        msg->setContextPointer(sut);
        scheduleAt(sut->nextScheduledTime, msg);
    }
}

void MobileAgent::sendSequenceUpdate(cMessage* msg) {
    EV << "MA: Send SeqUpdate" << endl;
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
    InterfaceEntry *ie = sut->ie;
    const IPv6Address &dest =  sut->dest;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress(); // to get src ip
    sut->nextScheduledTime = simTime() + sut->ackTimeout;
    sut->ackTimeout = (sut->ackTimeout)*2;
    MobileAgentHeader *mah = new MobileAgentHeader("ca_seq_upd");
    mah->setId(mobileId);
    mah->setIdInit(true);
    mah->setIdAck(true);
    mah->setSeqValid(true);
    mah->setAckValid(true);
    mah->setAddValid(true);
    mah->setRemValid(true);
    mah->setNextHeader(IP_PROT_NONE);
    mah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    mah->setIpAcknowledgementNumber(am.getLastAcknowledgemnt(mobileId));
    uint ack = sut->ack;
    uint seq = sut->seq;
    AddressManagement::AddressChange ac = am.getUnacknowledgedIPv6AddressList(mobileId,ack,seq);
    mah->setIpAddingField(ac.addedAddresses);
    mah->setAddedAddressesArraySize(ac.addedAddresses);
    if(ac.addedAddresses > 0) {
        if(ac.addedAddresses != ac.getUnacknowledgedAddedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqUpd: value of Add list must have size of integer.");
        for(int i=0; i<ac.addedAddresses; i++) {
            mah->setAddedAddresses(i,ac.getUnacknowledgedAddedIPv6AddressList.at(i));
        }
    }
    mah->setIpRemovingField(ac.removedAddresses);
    mah->setRemovedAddressesArraySize(ac.removedAddresses);
    if(ac.removedAddresses > 0) {
        if(ac.removedAddresses != ac.getUnacknowledgedRemovedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqUpd: value of Rem list must have size of integer.");
        for(int i=0; i<ac.removedAddresses; i++) {
            mah->setRemovedAddresses(i,ac.getUnacknowledgedRemovedIPv6AddressList.at(i));
        }
    }
    mah->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*(ac.addedAddresses+ac.removedAddresses)));
    sendToLowerLayer(mah, dest, src); // TODO select interface
    scheduleAt(sut->nextScheduledTime, msg);
}

//void Agent::createRedirection

void MobileAgent::createFlowRequest(FlowTuple &tuple) {
    EV << "MA: Send FlowRequest" << endl;
    if(sessionState != ASSOCIATED &&  seqnoState != ASSOCIATED) { throw cRuntimeError("MA: Not registered at CA. Cannot run seq init."); }
    FlowUnit *funit = getFlowUnit(tuple);
    funit->state = REGISTERING; // changing state of flowUnit
    cMessage *msg = new cMessage("sendingCAflowReq", MSG_FLOW_REQ);
    TimerKey key(caAddress, -1, TIMERKEY_FLOW_REQ);
    FlowRequestTimer *frt = (FlowRequestTimer *) getExpiryTimer(key, TIMERTYPE_FLOW_REQ);
    frt->dest = caAddress;
    frt->tuple = tuple;
    frt->timer = msg;
    frt->ackTimeout = TIMEDELAY_FLOW_REQ;
    InterfaceEntry *ie = getInterface(caAddress);
    if(!ie) {
        EV << "MA: Delaying flow request. no interface provided." << endl;
        frt->ie = nullptr;
        frt->nextScheduledTime = simTime()+TIMEDELAY_FLOW_REQ;
    } else {
        EV << "MA: Sending flow request without delay. interface provided." << endl;
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
    const IPv6Address nodeAddress = frt->tuple.destAddress;
    const IPv6Address &dest =  frt->dest;
    const IPv6Address &src = frt->ie->ipv6Data()->getPreferredAddress(); // to get src ip
    MobileAgentHeader *mah = new MobileAgentHeader("ca_flow_req");
    mah->setId(mobileId);
    mah->setIdInit(true);
    mah->setIdAck(true);
    mah->setSeqValid(true);
    mah->setAckValid(true);
    mah->setAddValid(false);
    mah->setRemValid(false);
    mah->setCacheAddrInit(true);
    mah->setForwardAddrInit(false);
    mah->setNextHeader(IP_PROT_NONE);
    mah->setNodeAddress(nodeAddress);
    mah->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR);
    sendToLowerLayer(mah, dest, src);
    scheduleAt(frt->nextScheduledTime, msg);
}

void MobileAgent::processControlAgentMessage(ControlAgentHeader* agentHeader, IPv6ControlInfo* controlInfo) {
//    EV << "MA: Entering CA processing message" << endl;
// processing messages from control agent
    IPv6Address &caAddr = controlInfo->getSrcAddr();
    if(caAddress != caAddr) throw cRuntimeError("MA: Received message should be from CA, but it is received from somewhere else or caAddress is set wrongly.");
    InterfaceEntry *ie = ift->getInterfaceById(controlInfo->getInterfaceId());
    if(agentHeader->getIdInit() && agentHeader->getIdAck() && !agentHeader->getSeqValid()) { // session init
        if(sessionState == INITIALIZING) {
            sessionState = ASSOCIATED;
//                if(agentHeader->isName("ma_init_ack")) { EV << "MA: Received session ack from CA. Session started.: " << endl; }
            cancelAndDeleteExpiryTimer(caAddress,ie->getInterfaceId(), TIMERKEY_SESSION_INIT);
            EV << "MA: Received CA ack. Session init timer removed. Session init process successfully finished." << endl;
            createSequenceInit();
        }
        else { EV << "MA: Received CA ack. Session created yet. Why do I received it again?" << endl; }
    }
    else if (agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid()) {
        if(seqnoState == INITIALIZING) {
            seqnoState = ASSOCIATED;
            am.setLastAcknowledgemnt(mobileId, agentHeader->getIpSequenceNumber()); // ack is set the first time
            cancelAndDeleteExpiryTimer(caAddress,ie->getInterfaceId(), TIMERKEY_SEQNO_INIT);
            EV << "MA: Received CA ack. Seqno init timer removed if there were one. Seqno init process successfully finished." << endl;
        }
        else {
            if(agentHeader->getCacheAddrAck() || agentHeader->getForwardAddrAck()) { // if request is responsed
                IPv6Address cn = agentHeader->getNodeAddress();
                IPv6Address ag = agentHeader->getAgentAddress();
                addressAssociation.insert(std::make_pair(cn,ag));
                addressAssociationInv.insert(std::make_pair(ag,cn));
                cancelAndDeleteExpiryTimer(caAddress,-1, TIMERKEY_FLOW_REQ);
                EV << "MA: Flow request responsed by CA. Request process successfully established." << endl;
            } else { // it's a sequence update
                if(agentHeader->getIpSequenceNumber() > am.getLastAcknowledgemnt(mobileId)) {
                    cancelAndDeleteExpiryTimer(caAddress,-1, TIMERKEY_SEQ_UPDATE, mobileId, agentHeader->getIpSequenceNumber(), am.getLastAcknowledgemnt(mobileId));
                    am.setLastAcknowledgemnt(mobileId, agentHeader->getIpSequenceNumber());
                    EV << "MA: Received CA ack. Seqno update timer removed if there were one. Seq update process successfully finished." << endl;
                } else {
                    EV << "MA: Received old sequence acknowledgment. Current value is higher as the acknowledged value." << endl;
                }
            }
        }
    }
    delete agentHeader; // delete at this point because it's not used any more
    delete controlInfo;
}

void MobileAgent::processDataAgentMessage(DataAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
//    EV << "MA: incoming UDP packet" << endl;
    if(agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid()) {
        if(agentHeader->getNextHeader() == IP_PROT_UDP) {
            cPacket *packet = agentHeader->decapsulate();
            if (dynamic_cast<UDPPacket *>(packet) != nullptr) {
                UDPPacket *udpPacket =  (UDPPacket *) packet;
                IPv6Address *nodeAddress;
                IPv6Address agentAddress = controlInfo->getSourceAddress().toIPv6();
                if(isAddressAssociatedInv(agentAddress))
                    nodeAddress = getAssociatedAddressInv(agentAddress);
                else
                    throw cRuntimeError("MA: DA-address is not associated with node address");
                FlowTuple tuple;
                tuple.protocol = IP_PROT_UDP;
                tuple.destPort = udpPacket->getSourcePort();
                tuple.sourcePort = udpPacket->getDestinationPort();
                tuple.destAddress = *nodeAddress;
                FlowUnit *funit = getFlowUnit(tuple);
                if(funit->state == UNREGISTERED) {
                    throw cRuntimeError("MA: No match for incoming UDP packet");
                } else {
                    funit->state = REGISTERED;
                }
                IPv6Address fakeIp(29<<56,mobileId);
                EV << "MA:IPv6: " << fakeIp.str() << endl;
                IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
                ipControlInfo->setProtocol(IP_PROT_UDP);
                ipControlInfo->setSrcAddr(funit->nodeAddress);
                ipControlInfo->setDestAddr(fakeIp);
                ipControlInfo->setInterfaceId(1);
                ipControlInfo->setHopLimit(controlInfo->getHopLimit());
                ipControlInfo->setTrafficClass(controlInfo->getTrafficClass());
                udpPacket->setControlInfo(ipControlInfo);
                cGate *outgate = gate("toUDP");
                EV << "IP2UDP: Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " If=" << controlInfo->getInterfaceId() << " Pkt=" << udpPacket->getByteLength() << endl;
                send(udpPacket, outgate);
            } else { throw cRuntimeError("MA:procDAmsg: UDP packet could not be cast."); }
        } else
            if(agentHeader->getNextHeader() == IP_PROT_TCP) {
                cPacket *packet = agentHeader->decapsulate();
                if (dynamic_cast<tcp::TCPSegment *>(packet) != nullptr) {
                    tcp::TCPSegment *tcpseg =  (tcp::TCPSegment *) packet;
                    IPv6Address *nodeAddress;
                    IPv6Address agentAddress = controlInfo->getSourceAddress().toIPv6();
                    if(isAddressAssociatedInv(agentAddress))
                        nodeAddress = getAssociatedAddressInv(agentAddress);
                    else
                        throw cRuntimeError("MA: DA-address is not associated with node address");
                    FlowTuple tuple;
                    tuple.protocol = IP_PROT_TCP;
                    tuple.destPort = tcpseg->getSourcePort();
                    tuple.sourcePort = tcpseg->getDestinationPort();
                    tuple.destAddress = *nodeAddress;
                    FlowUnit *funit = getFlowUnit(tuple);
                    if(funit->state == UNREGISTERED) {
                        throw cRuntimeError("MA: No match for incoming TCP packet");
                    } else {
                        funit->state = REGISTERED;
                    }
                    IPv6Address fakeIp(29<<56,mobileId);
                    EV << "MA:IPv6: " << fakeIp.str() << endl;
                    IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
                    ipControlInfo->setProtocol(IP_PROT_TCP);
                    ipControlInfo->setSrcAddr(funit->nodeAddress);
                    ipControlInfo->setDestAddr(fakeIp);
                    ipControlInfo->setInterfaceId(1);
                    ipControlInfo->setHopLimit(controlInfo->getHopLimit());
                    ipControlInfo->setTrafficClass(controlInfo->getTrafficClass());
                    tcpseg->setControlInfo(ipControlInfo);
                    cGate *outgate = gate("toTCP");
                    EV << "IP2TCP: Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " If=" << controlInfo->getInterfaceId() << " PktSize=" << tcpseg->getByteLength() << endl;
                    send(tcpseg, outgate);
                } else { throw cRuntimeError("MA:procDAmsg: TCP packet could not be cast."); }
            } else { throw cRuntimeError("MA:procDAmsg: next header field of incoming packet not known."); }
    } else { throw cRuntimeError("MA:procDAmsg: header of incoming packet not correct."); }
    delete agentHeader;
    delete controlInfo;
}

void MobileAgent::processIncomingUDPPacket(cMessage *msg, IPv6ControlInfo *controlInfo)
{
//        IPv6Address src = controlInfo->getSrcAddr(); // can be used as check in second round
    if (dynamic_cast<UDPPacket *>(msg) != nullptr) {
//        EV << "MA:udpIn: processsing udp pkt." << endl;
        UDPPacket *udpPacket =  (UDPPacket *) msg;
        FlowTuple tuple;
        tuple.protocol = IP_PROT_UDP;
        tuple.destPort = udpPacket->getDestinationPort();
        tuple.sourcePort = udpPacket->getSourcePort();
        tuple.destAddress = controlInfo->getDestAddr();
        FlowUnit *funit = getFlowUnit(tuple);
        if(funit->state == UNREGISTERED) {
            funit->lifetime = MAX_PKT_LIFETIME;
            funit->state = REGISTERING; // TODO check code below
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
            if(isAddressAssociated(tuple.destAddress)) {
                EV << "MA:udpIn: creating address association. only done one time actually." << endl;
                funit->lifetime = MAX_PKT_LIFETIME;
                IPv6Address *ag = getAssociatedAddress(tuple.destAddress);
                if(ag) {
                    EV << "MA:udpIn: creating flow unit." << endl;
                    funit->state = REGISTERED;
                    funit->active = true;
                    funit->dataAgent = *ag;
                    funit->cacheAddress = false; // details if cache should be used
                    funit->cachingActive = false; // specify if data agent has cached the addr
                    funit->loadSharing = false;
                    funit->locationUpdate = false;
                    funit->nodeAddress = tuple.destAddress;
                    funit->id = mobileId;
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
        } else if(funit->state == REGISTERED) {
            funit->lifetime = MAX_PKT_LIFETIME;
            // what should here be done?
        } else { throw cRuntimeError("MA:UDPin: Not known state of flow. What should be done?"); }
//        EV << "MA:udpIn: flow is registered. starting sending process." << endl;
        MobileAgentHeader *mah = new MobileAgentHeader("udp_id");
        uint64 ADDED_CONFIG = 0;
        if(funit->cacheAddress) { // check if address should be cached. It should be cached.
            if(!funit->cachingActive) { // first message to data agent.
                mah->setCacheAddrInit(true);
                mah->setForwardAddrInit(false);
                mah->setNodeAddress(tuple.destAddress);
                ADDED_CONFIG += SIZE_ADDING_ADDR_TO_HDR;
            } else {
                mah->setCacheAddrInit(false);
                mah->setForwardAddrInit(false);
//                    mah->setNodeAddress(); is not set since address is cached
            }
        } else { // no data agent cache desired
            mah->setCacheAddrInit(false);
            mah->setForwardAddrInit(true);
            mah->setNodeAddress(tuple.destAddress);
            ADDED_CONFIG += SIZE_ADDING_ADDR_TO_HDR;
        }
        mah->setNextHeader(IP_PROT_UDP);
        mah->setIdInit(true);
        mah->setIdAck(true);
        mah->setSeqValid(true);
        mah->setAckValid(true);
        mah->setAddValid(true);
        mah->setRemValid(true);
        mah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
        mah->setIpAcknowledgementNumber(am.getLastAcknowledgemnt(mobileId));
        uint ack = am.getLastAcknowledgemnt(mobileId);
        uint seq = am.getCurrentSequenceNumber(mobileId);
        AddressManagement::AddressChange ac = am.getUnacknowledgedIPv6AddressList(mobileId,ack,seq);
        mah->setIpAddingField(ac.addedAddresses);
        mah->setAddedAddressesArraySize(ac.addedAddresses);
        if(ac.addedAddresses > 0) {
            if(ac.addedAddresses != ac.getUnacknowledgedAddedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqUpd: value of Add list must have size of integer.");
            for(int i=0; i<ac.addedAddresses; i++) {
                mah->setAddedAddresses(i,ac.getUnacknowledgedAddedIPv6AddressList.at(i));
            }
        }
        mah->setIpRemovingField(ac.removedAddresses);
        mah->setRemovedAddressesArraySize(ac.removedAddresses);
        if(ac.removedAddresses > 0) {
            if(ac.removedAddresses != ac.getUnacknowledgedRemovedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqUpd: value of Rem list must have size of integer.");
            for(int i=0; i<ac.removedAddresses; i++) {
                mah->setRemovedAddresses(i,ac.getUnacknowledgedRemovedIPv6AddressList.at(i));
            }
        }
        mah->setId(mobileId);
        mah->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*(ac.addedAddresses+ac.removedAddresses))+ADDED_CONFIG);
        mah->encapsulate(udpPacket);
        controlInfo->setProtocol(IP_PROT_IPv6EXT_ID);
        controlInfo->setDestinationAddress(funit->dataAgent);
        // TODO set here scheduler, which decides the outgoing interface
        if(funit->loadSharing) {
            throw cRuntimeError("MA: Load sharing not implemented yet.");
        } else {
            controlInfo->setInterfaceId(getInterface(funit->dataAgent)->getInterfaceId()); // just override existing entries
            controlInfo->setSourceAddress(getInterface(funit->dataAgent)->ipv6Data()->getPreferredAddress());
            mah->setControlInfo(controlInfo); // make copy before setting param
            cGate *outgate = gate("toLowerLayer");
            EV << "UDP2IP: Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " If=" << controlInfo->getInterfaceId() << " Pkt=" << mah->getByteLength() << endl;
            send(mah, outgate);
        }
    } else { throw cRuntimeError("MA: Incoming message should be UPD packet."); }
}

void MobileAgent::processIncomingTCPPacket(cMessage *msg, IPv6ControlInfo *controlInfo)
{
//        IPv6Address src = controlInfo->getSrcAddr(); // can be used as check in second round
    if (dynamic_cast<tcp::TCPSegment *>(msg) != nullptr) {
        EV << "MA:tcpIn: processing tcp pkt." << endl;
        tcp::TCPSegment *tcpseg = (tcp::TCPSegment *) msg;
        FlowTuple tuple;
        tuple.protocol = IP_PROT_TCP;
        tuple.destPort = tcpseg->getDestinationPort();
        tuple.sourcePort = tcpseg->getSourcePort();
        tuple.destAddress = controlInfo->getDestAddr();
        FlowUnit *funit = getFlowUnit(tuple);
        if(funit->state == UNREGISTERED) {
            funit->lifetime = MAX_PKT_LIFETIME;
            funit->state = REGISTERING; // TODO check code below
            createFlowRequest(tuple); // invoking flow initialization and delaying pkt by sending in queue of this object (hint: scheduleAt with delay)
            tcpseg->setKind(MSG_TCP_RETRANSMIT);
            tcpseg->setControlInfo(controlInfo); // appending own
            EV << "MA:tcpIn: delaying process, creating flow request." << endl;
            scheduleAt(simTime()+TIMEDELAY_PKT_PROCESS, tcpseg);
            return;
        } else if(funit->state == REGISTERING) {
            EV << "MA:tcpIn starting registering process of first tcp packet: "<< funit->lifetime << endl;
            funit->lifetime--;
            if(funit->lifetime < 1) {
                EV << "MA:tcpIn: deleting tcp packet because of exceeding lifetime." << endl;
                delete msg;
                delete controlInfo;
                return;
            } // TODO discarding all packets of tuple
            if(isAddressAssociated(tuple.destAddress)) {
                EV << "MA:tcpIn: creating address association. only done one time actually." << endl;
                funit->lifetime = MAX_PKT_LIFETIME;
                IPv6Address *ag = getAssociatedAddress(tuple.destAddress);
                if(ag) {
                    EV << "MA:tcpIn: creating flow unit." << endl;
                    funit->state = REGISTERED;
                    funit->active = true;
                    funit->dataAgent = *ag;
                    funit->cacheAddress = false; // details if cache should be used
                    funit->cachingActive = false; // specify if data agent has cached the addr
                    funit->loadSharing = false;
                    funit->locationUpdate = false;
                    funit->nodeAddress = tuple.destAddress;
                    funit->id = mobileId;
                } else {
                    throw cRuntimeError("MA:tcpin: getAssociatedAddr fetched empty pointer instead of address.");
                }
            } else {
                EV << "MA:tcpIn sending udp packet in queue, such that it can be processed in next round" << endl;
                tcpseg->setKind(MSG_TCP_RETRANSMIT);
                tcpseg->setControlInfo(controlInfo); // appending own
                scheduleAt(simTime()+TIMEDELAY_PKT_PROCESS, tcpseg);
                return;
            }
        } else if(funit->state == REGISTERED) {
            funit->lifetime = MAX_PKT_LIFETIME;
            // what should here be done?
        } else { throw cRuntimeError("MA:tcpin: Not known state of flow. What should be done?"); }
//        EV << "MA:udpIn: flow is registered. starting sending process." << endl;
        MobileAgentHeader *mah = new MobileAgentHeader("tcp_id");
        uint64 ADDED_CONFIG = 0;
        if(funit->cacheAddress) { // check if address should be cached. It should be cached.
            if(!funit->cachingActive) { // first message to data agent.
                mah->setCacheAddrInit(true);
                mah->setForwardAddrInit(false);
                mah->setNodeAddress(tuple.destAddress);
                ADDED_CONFIG += SIZE_ADDING_ADDR_TO_HDR;
            } else {
                mah->setCacheAddrInit(false);
                mah->setForwardAddrInit(false);
//                    mah->setNodeAddress(); is not set since address is cached
            }
        } else { // no data agent cache desired
            mah->setCacheAddrInit(false);
            mah->setForwardAddrInit(true);
            mah->setNodeAddress(tuple.destAddress);
            ADDED_CONFIG += SIZE_ADDING_ADDR_TO_HDR;
        }
        mah->setNextHeader(IP_PROT_TCP);
        mah->setIdInit(true);
        mah->setIdAck(true);
        mah->setSeqValid(true);
        mah->setAckValid(true);
        mah->setAddValid(true);
        mah->setRemValid(true);
        mah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
        mah->setIpAcknowledgementNumber(am.getLastAcknowledgemnt(mobileId));
        uint ack = am.getLastAcknowledgemnt(mobileId);
        uint seq = am.getCurrentSequenceNumber(mobileId);
        AddressManagement::AddressChange ac = am.getUnacknowledgedIPv6AddressList(mobileId,ack,seq);
        mah->setIpAddingField(ac.addedAddresses);
        mah->setAddedAddressesArraySize(ac.addedAddresses);
        if(ac.addedAddresses > 0) {
            if(ac.addedAddresses != ac.getUnacknowledgedAddedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqTCP: value of Add list must have size of integer.");
            for(int i=0; i<ac.addedAddresses; i++) {
                mah->setAddedAddresses(i,ac.getUnacknowledgedAddedIPv6AddressList.at(i));
            }
        }
        mah->setIpRemovingField(ac.removedAddresses);
        mah->setRemovedAddressesArraySize(ac.removedAddresses);
        if(ac.removedAddresses > 0) {
            if(ac.removedAddresses != ac.getUnacknowledgedRemovedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqTCP: value of Rem list must have size of integer.");
            for(int i=0; i<ac.removedAddresses; i++) {
                mah->setRemovedAddresses(i,ac.getUnacknowledgedRemovedIPv6AddressList.at(i));
            }
        }
        mah->setId(mobileId);
        mah->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*(ac.addedAddresses+ac.removedAddresses))+ADDED_CONFIG);
        mah->encapsulate(tcpseg);
        controlInfo->setProtocol(IP_PROT_IPv6EXT_ID);
        controlInfo->setDestinationAddress(funit->dataAgent);
        // TODO set here scheduler, which decides the outgoing interface
        if(funit->loadSharing) {
            throw cRuntimeError("MA: Load sharing not implemented yet.");
        } else {
            controlInfo->setInterfaceId(getInterface(funit->dataAgent)->getInterfaceId()); // just override existing entries
            controlInfo->setSourceAddress(getInterface(funit->dataAgent)->ipv6Data()->getPreferredAddress());
            mah->setControlInfo(controlInfo); // make copy before setting param
            cGate *outgate = gate("toLowerLayer");
            EV << "TCP2IP: Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " If=" << controlInfo->getInterfaceId() << " PktSize=" << mah->getByteLength() << endl;
            send(mah, outgate);
        }
    } else { throw cRuntimeError("MA: Incoming message should be TCP packet."); }
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
    auto it = addressTable.find(id);
    if(it != addressTable.end()) { // check if interface is provided in address table
        if(it->first != id) throw cRuntimeError("ERROR in updateAddressTable: provided id should be same with entry");
        (it->second)->active = iu->active;
        (it->second)->priority = iu->priority;
        (it->second)->careOfAddress = iu->careOfAddress;
        if(iu->active) {    // presents an interface that has been associated
            am.addIPv6AddressToAddressMap(mobileId, iu->careOfAddress);
        } else { // presents an interface has been disassociated
            am.removeIPv6AddressFromAddressMap(mobileId, iu->careOfAddress);
        }
        createSequenceUpdate(mobileId, am.getCurrentSequenceNumber(mobileId), am.getLastAcknowledgemnt(mobileId));
    } else {
        addressTable.insert(std::make_pair(id,iu)); // if not, include this new
        am.addIPv6AddressToAddressMap(mobileId, iu->careOfAddress);
        if(sessionState == UNASSOCIATED) { createSessionInit(); } // first address is initialzed with session init
        else if(seqnoState == ASSOCIATED) { createSequenceUpdate(mobileId, am.getCurrentSequenceNumber(mobileId), am.getLastAcknowledgemnt(mobileId)); } // next address must be updated by seq update
        else if(seqnoState == INITIALIZING) { throw cRuntimeError("ERROR updateAddressTable: not inserted interface id should be added when the seqnoState is registered."); }
    }
//    EV << "AM_MA: " << am.to_string() << endl;
    EV << "AM_MA: " << am.getLastAcknowledgemnt(mobileId) << endl  << " -> " << am.getCurrentSequenceNumber(mobileId);
}

InterfaceEntry *MobileAgent::getInterface(IPv6Address destAddr, int destPort, int sourcePort, short protocol) { // const IPv6Address &destAddr,
    InterfaceEntry *ie = nullptr;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp() && ie->ipv6Data()->getPreferredAddress().isGlobal()) { return ie; }
    }
    return ie;
}


void MobileAgent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr, int interfaceId, simtime_t delayTime) {
//    EV << "A: Creating IPv6ControlInfo to lower layer" << endl;

    IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
    ctrlInfo->setProtocol(IP_PROT_IPv6EXT_ID); // todo must be adjusted
    ctrlInfo->setDestAddr(destAddr);
    ctrlInfo->setSrcAddr(srcAddr);
    ctrlInfo->setHopLimit(255);
    InterfaceEntry *ie = getInterface(destAddr);
    if(ie) { ctrlInfo->setInterfaceId(ie->getInterfaceId()); }
    msg->setControlInfo(ctrlInfo);
    cGate *outgate = gate("toLowerLayer");
    EV << "MA2IP: Dest=" << ctrlInfo->getDestAddr() << " Src=" << ctrlInfo->getSrcAddr() << " If=" << ctrlInfo->getInterfaceId() << endl;
    if (delayTime > 0) {
        EV << "delayed sending" << endl;
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

} //namespace
