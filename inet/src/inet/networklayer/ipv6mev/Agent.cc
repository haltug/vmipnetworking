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
#include "inet/networklayer/ipv6mev/Agent.h"

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

#include "inet/common/NotifierConsts.h"
#include "inet/common/lifecycle/NodeStatus.h"

namespace inet {

Define_Module(Agent);

void Agent::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER) {
        isMA = par("isMA").boolValue();
        isCA = par("isCA").boolValue();
        isDA = par("isDA").boolValue();
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        if(isMA) {
            sessionState = UNASSOCIATED;
            seqnoState = UNASSOCIATED;
            srand(123); // TODO must be changed
            mobileId = (uint64) rand(); // TODO should be placed in future
            am.initiateAddressMap(mobileId, 10);
        }
        if(isCA) {

        }
        if(isDA) {}
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
            IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
            ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID);
        if(isMA){
            interfaceNotifier = getContainingNode(this);
//            interfaceNotifier->subscribe(NF_INTERFACE_STATE_CHANGED,this); // register signal listener
            interfaceNotifier->subscribe(NF_INTERFACE_IPv6CONFIG_CHANGED,this);
        }
    }
    if (stage == INITSTAGE_APPLICATION_LAYER) {
        if(isMA) {
            startTime = par("startTime");
            cMessage *timeoutMsg = new cMessage("sessionStart");
            timeoutMsg->setKind(MSG_START_TIME);
//            scheduleAt(startTime, timeoutMsg); // delaying start
        }
    }

    WATCH(mobileId);
    WATCH(CA_Address);
//    WATCH(isMA);
//    WATCH(isCA);
//    WATCHMAP(interfaceToIPv6AddressList);
//    WATCHMAP(directAddressList);
}

void Agent::handleMessage(cMessage *msg)
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
            createSequenceUpdate();
            delete msg;
        }
        else if(msg->getKind() == MSG_FLOW_REQ) { // from MA
            sendFlowRequest(msg);
        }
        else if(msg->getKind() == MSG_UDP_RETRANSMIT) { // from MA
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processIncomingUDPPacket(msg, controlInfo);
        }
        //======================================== messages from CA
        else if(msg->getKind() == MSG_MA_INIT) { // from CA
            sendAgentInit(msg);
        }
        else if(msg->getKind() == MSG_MA_INIT_DELAY) { // from CA for retransmitting of da_init
            MobileInitTimer *mit = (MobileInitTimer *) msg->getContextPointer();
            uint64 mobileId = mit->id;
            createAgentInit(mobileId);
            delete msg;
        }
        else if(msg->getKind() == MSG_AGENT_UPDATE) { // from CA for retransmitting of da_init
            sendAgentUpdate(msg);
        }
        else if(msg->getKind() == MSG_SEQ_UPDATE_NOTIFY) {
            UpdateNotifierTimer *unt = (UpdateNotifierTimer *) msg->getContextPointer();
            uint64 mobileId = unt->id;
            uint seq = unt->seq;
            uint ack = unt->ack;
            sendSequenceUpdateNotification(mobileId, ack, seq);
            delete msg;
        }
        else
            throw cRuntimeError("handleMessage: Unknown timer expired. Which timer msg is unknown?");
    }
    else if(msg->arrivedOn("fromUDP")) {
        EV << "A: Received fromUDP" << endl;
        IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
        processIncomingUDPPacket(msg, controlInfo);
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
            } else if (dynamic_cast<MobileAgentHeader *>(idHdr)) {
                MobileAgentHeader *ma = (MobileAgentHeader *) idHdr;
                processMobileAgentMessage(ma,controlInfo);
            } else {
                throw cRuntimeError("A:handleMsg: Extension Hdr Type not known. What did you send?");
            }
        }
    }

    else
        throw cRuntimeError("A:handleMsg: cMessage Type not known. What did you send?");
}

Agent::Agent() {}

Agent::~Agent() {
    auto it = expiredTimerList.begin();
    while(it != expiredTimerList.end()) {
        TimerKey key = it->first;
        it++;
        cancelAndDeleteExpiryTimer(key.dest,key.interfaceID,key.type);
    }
}

void Agent::createSessionInit() {
    EV << "MA: Create CA_init" << endl;
    L3Address caAddr;
    const char *controlAgentAddr;
    controlAgentAddr = par("controlAgentAddress");
    L3AddressResolver().tryResolve(controlAgentAddr, caAddr);
    CA_Address = caAddr.toIPv6();
    if(CA_Address.isGlobal()) {
        if(isMA && sessionState == UNASSOCIATED) sessionState = INITIALIZING;
        InterfaceEntry *ie = getInterface(CA_Address); // TODO ie not correctly set
        if(!ie) { throw cRuntimeError("MA: No interface exists."); }
        cMessage *msg = new cMessage("sendingCAinit",MSG_SESSION_INIT);
        TimerKey key(CA_Address,ie->getInterfaceId(),TIMERKEY_SESSION_INIT);
        SessionInitTimer *sit = (SessionInitTimer *) getExpiryTimer(key,TIMERTYPE_SESSION_INIT);
        sit->dest = CA_Address;
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

void Agent::sendSessionInit(cMessage* msg) {
    EV << "MA: Send CA_init" << endl;
    SessionInitTimer *sit = (SessionInitTimer *) msg->getContextPointer();
    InterfaceEntry *ie = sit->ie;
// TODO check for global address if not skip this round
    const IPv6Address &dest = sit->dest = CA_Address;;
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

void Agent::createSequenceInit() { // does not support interface check
    EV << "MA: Create CA_seq_init" << endl;
    if(!isMA && sessionState != ASSOCIATED) { throw cRuntimeError("MA: Not registered at CA. Cannot run seq init."); }
    if(isMA && seqnoState == UNASSOCIATED) { seqnoState = INITIALIZING; }
    cMessage *msg = new cMessage("sendingCAseqInit", MSG_SEQNO_INIT);
    InterfaceEntry *ie = getInterface(CA_Address);
    if(!ie) { throw cRuntimeError("MA: No interface exists."); }
    TimerKey key(CA_Address,ie->getInterfaceId(),TIMERKEY_SEQNO_INIT);
    SequenceInitTimer *sit = (SequenceInitTimer *) getExpiryTimer(key, TIMERTYPE_SEQNO_INIT);
    sit->dest = CA_Address;
    sit->ie = ie;
    sit->timer = msg;
    sit->ackTimeout = TIMEOUT_SEQNO_INIT;
    sit->nextScheduledTime = simTime();
    msg->setContextPointer(sit);
    scheduleAt(sit->nextScheduledTime, msg);
}

void Agent::sendSequenceInit(cMessage *msg) {
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

void Agent::createSequenceUpdate() {
    EV << "MA: Create sequence update to DA" << endl;
    if(!isMA && sessionState != ASSOCIATED &&  seqnoState != ASSOCIATED) { throw cRuntimeError("MA: Not registered at CA. Cannot run seq init."); }
    cMessage *msg = new cMessage("sendingCAseqUpdate", MSG_SEQ_UPDATE);
    InterfaceEntry *ie = getInterface(CA_Address);
    if(!ie) {
        EV << "MA: Delaying seq update. no interface provided." << endl;
        cMessage *timeoutMsg = new cMessage("sequenceDelay");
        timeoutMsg->setKind(MSG_SEQ_UPDATE_DELAYED);
        scheduleAt(simTime()+TIMEOUT_SEQ_UPDATE, timeoutMsg);
    } else {
        EV << "MA: Sending seq update without delay. interface provided." << endl;
        TimerKey key(CA_Address, -1, TIMERKEY_SEQ_UPDATE);
        SequenceUpdateTimer *sut = (SequenceUpdateTimer *) getExpiryTimer(key, TIMERTYPE_SEQ_UPDATE);
        sut->dest = CA_Address;
        sut->ie = ie;
        sut->timer = msg;
        sut->ackTimeout = TIMEOUT_SEQ_UPDATE;
        sut->nextScheduledTime = simTime();
        msg->setContextPointer(sut);
        scheduleAt(sut->nextScheduledTime, msg);
    }
}

void Agent::sendSequenceUpdate(cMessage* msg)
{
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
    mah->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*(ac.addedAddresses+ac.removedAddresses)));
    sendToLowerLayer(mah, dest, src); // TODO select interface
    scheduleAt(sut->nextScheduledTime, msg);
}

//void Agent::createRedirection

void Agent::createFlowRequest(FlowTuple &tuple)
{
    EV << "MA: Send FlowRequest" << endl;
    if(!isMA && sessionState != ASSOCIATED &&  seqnoState != ASSOCIATED) { throw cRuntimeError("MA: Not registered at CA. Cannot run seq init."); }
    FlowUnit *funit = getFlowUnit(tuple);
    funit->state = REGISTERING; // changing state of flowUnit
    cMessage *msg = new cMessage("sendingCAflowReq", MSG_FLOW_REQ);
    TimerKey key(CA_Address, -1, TIMERKEY_FLOW_REQ);
    FlowRequestTimer *frt = (FlowRequestTimer *) getExpiryTimer(key, TIMERTYPE_FLOW_REQ);
    frt->dest = CA_Address;
    frt->tuple = &tuple;
    frt->timer = msg;
    frt->ackTimeout = TIMEDELAY_FLOW_REQ;
    InterfaceEntry *ie = getInterface(CA_Address);
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

void Agent::sendFlowRequest(cMessage *msg)
{
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
    const IPv6Address nodeAddress = frt->tuple->destAddress;
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

void Agent::createAgentInit(uint64 mobileId)
{
    EV << "CA: Create DA_MobileId_init" << endl;
    const char *dataAgentAddr = par("dataAgentAddress");
    cStringTokenizer tokenizer(dataAgentAddr);
    while (tokenizer.hasMoreTokens()) {
        L3Address daAddr;
        L3AddressResolver().tryResolve(tokenizer.nextToken(), daAddr);
        if(daAddr.toIPv6().isGlobal()) {
            cMessage *msg = new cMessage("sendingDAinit", MSG_MA_INIT);
            InterfaceEntry *ife = getInterface(daAddr.toIPv6());
            TimerKey key(daAddr.toIPv6(),ife->getInterfaceId(),TIMERKEY_MA_INIT, mobileId);
            if(std::find(nodeAddressList.begin(), nodeAddressList.end(), daAddr.toIPv6()) == nodeAddressList.end())
                nodeAddressList.push_back(daAddr.toIPv6());
            MobileInitTimer *mit = (MobileInitTimer *) getExpiryTimer(key,TIMERTYPE_MA_INIT);
            mit->dest = daAddr.toIPv6();
            mit->ie = ife;
            mit->timer = msg;
            mit->ackTimeout = TIMEOUT_SESSION_INIT;
            mit->nextScheduledTime = simTime();
            mit->id = mobileId;
            msg->setContextPointer(mit);
            scheduleAt(mit->nextScheduledTime,msg);
        } else {
            cMessage *msg = new cMessage("sendingDAinit", MSG_MA_INIT_DELAY);
            MobileInitTimer *mit = new MobileInitTimer();
            mit->id = mobileId;
            msg->setContextPointer(mit);
            scheduleAt(simTime()+TIMEDELAY_MA_INIT, msg);
        }
    }
}

void Agent::sendAgentInit(cMessage *msg)
{
    EV << "CA: Send init_to_DA" << endl;
    MobileInitTimer *mit = (MobileInitTimer *) msg->getContextPointer();
    InterfaceEntry *ie = mit->ie;
    const IPv6Address &dest = mit->dest;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress();
    mit->nextScheduledTime = simTime() + mit->ackTimeout;
    mit->ackTimeout = (mit->ackTimeout)*2;
    for(auto &item : am.getAddressMap()) {
        MobileAgentHeader *mah = new MobileAgentHeader("da_seq_init");
        mah->setId(item.first);
        mah->setIdInit(true);
        mah->setIdAck(false);
        mah->setSeqValid(true);
        mah->setAckValid(false);
        mah->setAddValid(true);
        mah->setRemValid(false);
        mah->setNextHeader(IP_PROT_NONE);
        mah->setIpSequenceNumber(am.getCurrentSequenceNumber(item.first));
        mah->setIpAcknowledgementNumber(0);
        mah->setIpAddingField(1);
        mah->setAddedAddressesArraySize(1);
        mah->setAddedAddresses(0,src);
        mah->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR);
        sendToLowerLayer(mah, dest, src);
    }
    scheduleAt(mit->nextScheduledTime, msg);
}

void Agent::createAgentUpdate(uint64 mobileId)
{
    EV << "CA: Create Agent update message to all DA's" << endl;
    for(IPv6Address nodeAddr : nodeAddressList) {
        cMessage *msg = new cMessage("sendingDAseqUpdate", MSG_AGENT_UPDATE);
        InterfaceEntry *ie = getInterface(nodeAddr);
        TimerKey key(nodeAddr, ie->getInterfaceId(), TIMERKEY_SEQ_UPDATE, mobileId);
        SequenceUpdateTimer *sut = (SequenceUpdateTimer *) getExpiryTimer(key, TIMERTYPE_SEQ_UPDATE);
        sut->dest = nodeAddr;
        sut->ie = ie;
        sut->timer = msg;
        sut->id = mobileId;
        sut->ackTimeout = TIMEOUT_SEQ_UPDATE;
        sut->nextScheduledTime = simTime();
        msg->setContextPointer(sut);
        scheduleAt(sut->nextScheduledTime, msg);
    }
}

void Agent::sendAgentUpdate(cMessage *msg)
{
    EV << "CA: Send agent update" << endl;
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
    InterfaceEntry *ie = sut->ie;
    const IPv6Address &dest =  sut->dest;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress(); // to get src ip
    sut->nextScheduledTime = simTime() + sut->ackTimeout;
    sut->ackTimeout = (sut->ackTimeout)*2;
    MobileAgentHeader *mah = new MobileAgentHeader("da_seq_upd");
    mah->setId(sut->id);
    mah->setIdInit(true);
    mah->setIdAck(false);
    mah->setSeqValid(true);
    mah->setAckValid(false);
    mah->setAddValid(true);
    mah->setRemValid(false);
    mah->setNextHeader(IP_PROT_NONE);
    mah->setIpSequenceNumber(am.getCurrentSequenceNumber(sut->id));
    mah->setIpAcknowledgementNumber(0);
    uint seq = am.getCurrentSequenceNumber(sut->id);
    AddressManagement::AddressChange ac = am.getAddessEntriesOfSequenceNumber(sut->id,seq);
    mah->setIpAddingField(ac.addedAddresses);
    mah->setAddedAddressesArraySize(ac.addedAddresses);
    if(ac.addedAddresses > 0) {
        if(ac.addedAddresses != ac.getUnacknowledgedAddedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqUpd: value of Add list must have size of integer.");
        for(int i=0; i<ac.addedAddresses; i++) {
            mah->setAddedAddresses(i,ac.getUnacknowledgedAddedIPv6AddressList.at(i));
        }
    }
    mah->setIpRemovingField(0);
    mah->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*ac.addedAddresses));
    sendToLowerLayer(mah, dest, src); // TODO select interface
    scheduleAt(sut->nextScheduledTime, msg);
}

void Agent::sendSequenceUpdateAck(uint64 mobileId)
{
    InterfaceEntry *ie = getAnyInterface();
    IPv6Address sourceAddr = ie->ipv6Data()->getPreferredAddress();
    AddressManagement::IPv6AddressList addressList = am.getCurrentAddressList(mobileId);
    for (IPv6Address ip : addressList ) {
        IPv6Address destAddr = ip; // address to be responsed
        ControlAgentHeader *cah = new ControlAgentHeader("ma_seq_upd_ack");
        cah->setIdInit(true);
        cah->setIdAck(true);
        cah->setSeqValid(true);
        cah->setCacheAddrAck(false);
        cah->setForwardAddrAck(false);
        cah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
        sendToLowerLayer(cah,destAddr, sourceAddr);
    }
}

void Agent::sendSessionInitResponse(IPv6Address destAddr, IPv6Address sourceAddr)
{
    ControlAgentHeader *cah = new ControlAgentHeader("ma_init_ack");
    cah->setIdInit(true);
    cah->setIdAck(true);
    cah->setSeqValid(false);
    cah->setIpSequenceNumber(0);
    sendToLowerLayer(cah,destAddr,sourceAddr);
}

void Agent::sendSequenceInitResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId)
{
    ControlAgentHeader *cah = new ControlAgentHeader("ma_seq_ack");
    cah->setIdInit(true);
    cah->setIdAck(true);
    cah->setSeqValid(true);
    cah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    sendToLowerLayer(cah,destAddr, sourceAddr);
    createAgentInit(mobileId);
}

void Agent::sendSequenceUpdateResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId)
{
    ControlAgentHeader *cah = new ControlAgentHeader("ma_seq_update");
    cah->setIdInit(true);
    cah->setIdAck(true);
    cah->setSeqValid(true);
    cah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    sendToLowerLayer(cah,destAddr,sourceAddr);
//    createAgentUpdate(mobileId); // we notify all dataAgents when we response to mobile agents seq update
}

void Agent::sendFlowRequestResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId, IPv6Address nodeAddr, IPv6Address agentAddr)
{
    ControlAgentHeader *cah = new ControlAgentHeader("ma_flow_req");
    cah->setIdInit(true);
    cah->setIdAck(true);
    cah->setSeqValid(true);
    cah->setCacheAddrAck(true);
    cah->setForwardAddrAck(false);
    cah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    cah->setNodeAddress(nodeAddr);
    cah->setAgentAddress(agentAddr);
    cah->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR*2);
    sendToLowerLayer(cah,destAddr,sourceAddr);
}

void Agent::sendAgentInitResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId)
{
    ControlAgentHeader *cah = new ControlAgentHeader("da_seq_init_ack");
    cah->setIdInit(true);
    cah->setIdAck(true);
    cah->setSeqValid(true);
    cah->setIdValid(true);
    cah->setId(mobileId);
    cah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    cah->setByteLength(SIZE_AGENT_HEADER);
    sendToLowerLayer(cah,destAddr, sourceAddr);
}

void Agent::sendAgentUpdateResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId)
{
    sendAgentInitResponse(destAddr, sourceAddr, mobileId);
}

void Agent::createSequenceUpdateNotificaiton(uint64 mobileId)
{
    cMessage *msg = new cMessage("sendingCAseqUpd", MSG_SEQ_UPDATE_NOTIFY);
    if(!CA_Address.isGlobal())
        throw cRuntimeError("DA:SeqUpdNotify: CA_Address is not set. Cannot send update message without address");
    TimerKey key(CA_Address,-1,TIMERKEY_SEQ_UPDATE_NOT, mobileId, am.getCurrentSequenceNumber(mobileId));
    UpdateNotifierTimer *unt = (UpdateNotifierTimer *) getExpiryTimer(key,TIMERTYPE_SEQ_UPDATE_NOT);
    unt->dest = daAddr.toIPv6();
    unt->ie = -1;
    unt->timer = msg;
    unt->ackTimeout = TIMEOUT_SEQ_UPDATE;
    unt->nextScheduledTime = simTime();
    unt->id = mobileId;
    unt->ack = am.getLastAcknowledgemnt(mobileId);
    unt->seq = am.getCurrentSequenceNumber(mobileId);
    msg->setContextPointer(unt);
    scheduleAt(unt->nextScheduledTime,msg);
}

// ==============================================================================
// ============================ HEADER PROCESSING   =============================
void Agent::processMobileAgentMessage(MobileAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    if(isCA)
    {
//        EV << "CA: processing message from MA" << endl;
        // ================================================================================
        // controlAgent message processing
        // ================================================================================
        IPv6Address destAddr = controlInfo->getSrcAddr(); // address to be responsed
        IPv6Address sourceAddr = controlInfo->getDestAddr();
        if(agentHeader->getIdInit() && !agentHeader->getIdAck()) // check for other bits
        {   // init process request
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
                EV << "CA: Id already exists. Skipping initializing, check for mistakes." << endl;
            } else {
                EV << "CA: Received session Init message. Adding id to list: " << agentHeader->getId() << endl;
                mobileIdList.push_back(agentHeader->getId()); // adding id to list (registering id)
                sendSessionInitResponse(destAddr, sourceAddr);
            }
        } else
            if (agentHeader->getSeqValid() && !agentHeader->getAckValid() && agentHeader->getIdInit() && agentHeader->getIdAck()) // || agentHdr->getIdInit() && agentHdr->getIdAck()
        {   // init sequence number/address management
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
                bool addrMgmtEntry = am.insertNewId(agentHeader->getId(), agentHeader->getIpSequenceNumber(), agentHeader->getAddedAddresses(0));//first number
                if(addrMgmtEntry) { // check if seq and id is inserted
                    EV << "CA: Received sequence initialize message from MA. Initialized sequence number: " << agentHeader->getIpSequenceNumber() << "; ip: " << agentHeader->getAddedAddresses(0) << endl;
                    sendSequenceInitResponse(destAddr, sourceAddr, agentHeader->getId());
                }
                else {
                    if(am.isIdInitialized(agentHeader->getId()))
                        EV << "+++++ERROR+++++ CA: Id has been initialized before. Why do you send me again an init message?" << endl;
                    else
                        throw cRuntimeError("CA: Initialization of sequence number failed, CA could not insert id in AddrMgmt-Unit.");
                }
            }
            else
                throw cRuntimeError("CA should initialize sequence number but cannot find id in list.");
        } else
            if(agentHeader->getSeqValid() && agentHeader->getAckValid() && agentHeader->getIdInit() && agentHeader->getIdAck()) {
                // Responsing to seqNo update
                if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end())
                { // updating seq no
                    if(agentHeader->getAddValid() || agentHeader->getRemValid()) { // indicate a sequence update
                        if(agentHeader->getIpSequenceNumber() != am.getCurrentSequenceNumber(agentHeader->getId())) {
                            if(agentHeader->getIpAddingField() > 0) { // check size of field
                                if(agentHeader->getIpAddingField() != agentHeader->getAddedAddressesArraySize()) throw cRuntimeError("CA:Hdr: field of array and field not same. Check header.");
                                for(int i=0; i<agentHeader->getIpAddingField(); i++){
                                    am.addIPv6AddressToAddressMap(agentHeader->getId(), agentHeader->getAddedAddresses(i));
                                }
                            }
                            if(agentHeader->getIpRemovingField() > 0) {
                                if(agentHeader->getIpRemovingField() != agentHeader->getRemovedAddressesArraySize()) throw cRuntimeError("CA:Hdr: field of array and field not same. Check header.");
                                for(int i=0; i<agentHeader->getIpRemovingField(); i++) {
                                    am.removeIPv6AddressFromAddressMap(agentHeader->getId(), agentHeader->getRemovedAddresses(i));
                                }
                            }
                            am.setLastAcknowledgemnt(agentHeader->getId(), agentHeader->getIpSequenceNumber());
                            EV << "CA: Received update message. update to seq: " << agentHeader->getIpSequenceNumber() << endl;
                            createAgentUpdate(agentHeader->getId()); // we notify all dataAgents when we response to mobile agents seq update but we first start with DA and then inform MA
//                            sendSequenceUpdateResponse(destAddr, sourceAddr, agentHeader->getId());
                        }
                    }
                    // flow request response
                    if(agentHeader->getCacheAddrInit() || agentHeader->getForwardAddrInit()) { // check if redirect address is requested
                        IPv6Address nodeAddress = agentHeader->getNodeAddress();
                        // you would here lookup for the best data agent, returning for simplicity just one address
                        L3Address daAddr;
                        const char *dataAgentAddr;
                        dataAgentAddr = par("dataAgentAddress");
                        L3AddressResolver().tryResolve(dataAgentAddr, daAddr);
                        sendFlowRequestResponse(destAddr, sourceAddr, agentHeader->getId(), nodeAddress, daAddr.toIPv6());
                    }
            } else { throw cRuntimeError("CA: Mobile id not known."); }
        } else { throw cRuntimeError("CA: Message not known. Parameter bits of header are not set correctly."); }
    } else
        if(isDA)
        {
            // ================================================================================
            // dataAgent message processing
            // ================================================================================
            IPv6Address destAddr = controlInfo->getSrcAddr(); // address to be responsed
            IPv6Address sourceAddr = controlInfo->getDestAddr();
            if(CA_Address.isUnspecified()) { CA_Address = controlInfo->getSrcAddr(); }
            // header flag bits 101010 + NextHeader=NONE
            if(agentHeader->getIdInit() && !agentHeader->getIdAck() && agentHeader->getSeqValid() && !agentHeader->getAckValid()  && agentHeader->getAddValid() && !agentHeader->getRemValid() && (agentHeader->getNextHeader() == IP_PROT_NONE)) // check for other bits
            { // INIT PROCESSING
                if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) == mobileIdList.end())
                { // if mobile is not registered. insert a new entry in map
                    EV << "DA: Received agent init message from CA. Adding id to list: " << agentHeader->getId() << endl;
                    mobileIdList.push_back(agentHeader->getId());
                        if(agentHeader->getIpAddingField() > 0) { // check size of field
                            bool addrMgmtEntry = am.insertNewId(agentHeader->getId(), agentHeader->getIpSequenceNumber(), agentHeader->getAddedAddresses(0)); //first number
                            if(!addrMgmtEntry) // if we receive for the first time an init message, then response to this one
                                throw cRuntimeError("DA:procMAmsg: Could not insert id.");
                        } else
                            throw cRuntimeError("DA:procMAmsg: Init message must contain the start message of mobile agent.");
                        am.setLastAcknowledgemnt(agentHeader->getId(), agentHeader->getIpSequenceNumber());
                        sendAgentInitResponse(destAddr, sourceAddr, agentHeader->getId());
                } else
                { // IF NOT AGENT INIT then JUST SEQ UPDATE
                    EV << "DA: Received message from CA. Id already exists. Skipping init and update just map." << endl;
                    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
                        AddressManagement::IPv6AddressList ipList;
                        if(agentHeader->getIpAddingField() > 0) { // check size of field
                            if(agentHeader->getIpAddingField() != agentHeader->getAddedAddressesArraySize())
                                throw cRuntimeError("CA:Hdr: field of array and field not same. Check header.");
                            for(int i=0; i<agentHeader->getIpAddingField(); i++){ // copy array in vector list
                                ipList.push_back(agentHeader->getAddedAddresses(i)); // inserting elements of array into vector list due to function
                            }
                        }
                        am.insertSequenceTableToAddressMap(agentHeader->getId(), ipList, agentHeader->getIpSequenceNumber());
                        am.setLastAcknowledgemnt(agentHeader->getId(),agentHeader->getIpSequenceNumber()); //
                        sendAgentUpdateResponse(destAddr, sourceAddr, agentHeader->getId());
                    } else
                        throw cRuntimeError("DA:procMA: Id is not inserted in map. Register id before seq update.");
                }
            }
            else if(agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid() && agentHeader->getAckValid())
            { // regular message from mobile agent
                if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
                   // insert new address
                    // and then check if ip addr is in map
                    if(agentHeader->getNextHeader() == IP_PROT_UDP) {
                        // process here UDP packets from mobile agent
                    } else if(agentHeader->getNextHeader() == IP_PROT_TCP) {
                        // process here TCP packets from mobile agent
                    } else { throw cRuntimeError("DA:procMAmsg: header declaration not known."); }
                } else { throw cRuntimeError("DA:procMAmsg: DA is not initialized with given ID."); }
            }


    } else { throw cRuntimeError("Received message that is supposed to be received by control agent. check message type that has to be processed."); }
    if(am.isIdInitialized(agentHeader->getId()))
    delete agentHeader;
    delete controlInfo;
}

void Agent::processControlAgentMessage(ControlAgentHeader* agentHeader, IPv6ControlInfo* controlInfo) {
//    EV << "MA: Entering CA processing message" << endl;
    if(isMA)
    { // processing messages from control agent
        IPv6Address &caAddr = controlInfo->getSrcAddr();
        InterfaceEntry *ie = ift->getInterfaceById(controlInfo->getInterfaceId());
        if(agentHeader->getIdInit() && agentHeader->getIdAck() && !agentHeader->getSeqValid()) { // session init
            if(sessionState == INITIALIZING) {
                sessionState = ASSOCIATED;
//                if(agentHeader->isName("ma_init_ack")) { EV << "MA: Received session ack from CA. Session started.: " << endl; }
                cancelAndDeleteExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_SESSION_INIT);
                EV << "MA: Received CA ack. Session init timer removed. Session init process successfully finished." << endl;
                createSequenceInit();
            }
            else { EV << "MA: Received CA ack. Session created yet. Why do I received it again?" << endl; }
        }
        else if (agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid()) {
            if(seqnoState == INITIALIZING) {
                seqnoState = ASSOCIATED;
                am.setLastAcknowledgemnt(mobileId, agentHeader->getIpSequenceNumber());
                cancelAndDeleteExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_SEQNO_INIT);
                EV << "MA: Received CA ack. Seqno init timer removed if there were one. Seqno init process successfully finished." << endl;
            }
            else {
                if(agentHeader->getCacheAddrAck() || agentHeader->getForwardAddrAck()) { // if request is responsed
                    IPv6Address cn = agentHeader->getNodeAddress();
                    IPv6Address ag = agentHeader->getAgentAddress();
                    addressAssociation.insert(std::make_pair(cn,ag));
                    cancelAndDeleteExpiryTimer(caAddr,-1, TIMERKEY_FLOW_REQ);
                    EV << "MA: Flow request responsed by CA. Request process successfully established." << endl;
                } else { // it's a sequence update
                    am.setLastAcknowledgemnt(mobileId, agentHeader->getIpSequenceNumber());
                    cancelAndDeleteExpiryTimer(caAddr,-1, TIMERKEY_SEQ_UPDATE); // TODO interface should be set to -1 because the ack should independent of src. fix this also at sent method.
                    EV << "MA: Received CA ack. Seqno update timer removed if there were one. Seq update process successfully finished." << endl;
                }
            }
        }
    }
    if(isCA)
    { // processing messages from dataAgent
        IPv6Address &caAddr = controlInfo->getSrcAddr();
        InterfaceEntry *ie = ift->getInterfaceById(controlInfo->getInterfaceId());
        if(agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid() && agentHeader->getIdValid()) {
            if(!cancelAndDeleteExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_MA_INIT, agentHeader->getId())) { // check with pending
                // if this is true, then a timer key existed. so a timer can only exist when the data agent were not initialized.
                // if false, then data agents has been initialized. only a timer for update can exists.
                cancelAndDeleteExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_SEQ_UPDATE, agentHeader->getId());
                EV << "CA: Received DA update msg. Agent update timer removed. Process successfully finished." << endl;
            } else
                EV << "CA: Received DA ack. Agent init timer removed if there was one. Agent init process successfully finished." << endl;
            bool allDataAgentsUpdated = true;
            for(IPv6Address ip : nodeAddressList) {
                if(pendingExpiryTimer(ip,ie->getInterfaceId(),TIMERKEY_SEQ_UPDATE, agentHeader->getId()))
                    allDataAgentsUpdated = false;
            }
            if(allDataAgentsUpdated) {
                IPv6Address sourceAddr = controlInfo->getDestAddr();
                AddressManagement::AddressChange ac = am.getAddessEntriesOfSequenceNumber(agentHeader->getId(),am.getCurrentSequenceNumber(agentHeader->getId()));
                if(ac.addedAddresses > 0) { // any interafce is provided but it's not sure if these are also reachable. so mobile node has to ensure that seq update is receiveable
                    for(int i=0; i<ac.addedAddresses; i++) {
                        sendSequenceUpdateResponse(ac.getUnacknowledgedAddedIPv6AddressList.at(i), sourceAddr, agentHeader->getId());
                    }
                }
            }
        } else
            EV << "MA: Session created yet. Why do I received it again?" << endl;
    }
    delete agentHeader; // delete at this point because it's not used any more
    delete controlInfo;
}

void Agent::processDataAgentMessage(DataAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    EV << "DA: processing message from MA. Length:" << agentHeader->getByteLength() << endl;
    if(isCA)
    {
        if(agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid() && (agentHeader->getNextHeader() == IP_PROT_NONE)) {

        }
    } else if (isMA) {
        if(agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid() && (agentHeader->getNextHeader() == IP_PROT_UDP)) {
            // process here incoming UDP packets
        } else if(agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid() && (agentHeader->getNextHeader() == IP_PROT_UDP)) {
            // process here incoming TCP packets
        }
    }
    delete agentHeader;
    delete controlInfo;
}

void Agent::processIncomingUDPPacket(cMessage *msg, IPv6ControlInfo *controlInfo)
{
    EV << "MA: processing incoming message from UDP " << endl;
    if(isMA)
    {
//        IPv6Address src = controlInfo->getSrcAddr(); // can be used as check in second round
        if (dynamic_cast<UDPPacket *>(msg) != nullptr) {
            UDPPacket *udpPacket =  (UDPPacket *) msg;
            FlowTuple tuple;
            tuple.destAddress = controlInfo->getDestAddr();
            tuple.sourcePort = udpPacket->getSourcePort();
            tuple.destPort = udpPacket->getDestinationPort();
            tuple.protocol = IP_PROT_UDP;
            udpPacket->getKind();
            FlowUnit *funit = getFlowUnit(tuple);
            if(funit->state == UNREGISTERED) {
                tuple.lifetime = MAX_PKT_LIFETIME;
                funit->state = REGISTERING;
                createFlowRequest(tuple); // invoking flow initialization and delaying pkt by sending in queue of this object (hint: scheduleAt with delay)
                udpPacket->setKind(MSG_UDP_RETRANSMIT);
                udpPacket->setControlInfo(controlInfo); // appending own
                scheduleAt(simTime()+TIMEDELAY_FLOW_REQ, udpPacket);
                return;
            } else if(funit->state == REGISTERING) {
                tuple.lifetime--;
                if(tuple.lifetime < 1) { delete msg; delete controlInfo; return;} // TODO discarding all packets of tuple
                if(isAddressAssociated(tuple.destAddress)) {
                    tuple.lifetime = MAX_PKT_LIFETIME;
                    IPv6Address *ag = getAssociatedAddress(tuple.destAddress);
                    if(ag) {
                        funit->state = REGISTERED;
                        funit->active = true;
                        funit->dataAgent = *ag;
                        funit->cacheAddress = true; // details if cache should be used
                        funit->cachingActive = false; // specify if data agent has cached the addr
                        funit->loadSharing = false;
                        funit->locationUpdate = false;
                    } else {
                        throw cRuntimeError("MA:UDPin: getAssociatedAddr fetched empty pointer instead of address.");
                    }
                } else {
                    udpPacket->setKind(MSG_UDP_RETRANSMIT);
                    udpPacket->setControlInfo(controlInfo); // appending own
                    scheduleAt(simTime()+TIMEDELAY_FLOW_REQ, udpPacket);
                    return;
                }
            } else if(funit->state == REGISTERED) {
                tuple.lifetime = MAX_PKT_LIFETIME;
                // what should here be done?
            } else { throw cRuntimeError("MA:UDPin: Not known state of flow. What should be done?"); }
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
            controlInfo->setProtocol(IP_PROT_IPv6EXT_ID);
            controlInfo->setDestinationAddress(funit->dataAgent);
            // TODO set here scheduler, which decides the outgoing interface
            if(funit->loadSharing) {
                throw cRuntimeError("MA: Load sharing not implemented yet.");
            } else {
                controlInfo->setInterfaceId(getInterface(funit->dataAgent)->getInterfaceId()); // just override existing entries
                mah->setControlInfo(controlInfo); // make copy before setting param
                mah->encapsulate(udpPacket);
                cGate *outgate = gate("toLowerLayer");
                EV << "UDP2IP: Dest=" << controlInfo->getDestAddr() << " Src=" << controlInfo->getSrcAddr() << " If=" << controlInfo->getInterfaceId() << " Pkt=" << mah->getByteLength() << endl;
                send(msg, outgate);
            }
        } else { throw cRuntimeError("MA: Incoming message should be UPD packet."); }
    }
    else { throw cRuntimeError("Message should be processed by MA. Check why an agent received the message"); }
}

void Agent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr, int interfaceId, simtime_t delayTime) {
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
    EV << "ControlInfo: Dest=" << ctrlInfo->getDestAddr() << " Src=" << ctrlInfo->getSrcAddr() << " If=" << ctrlInfo->getInterfaceId() << endl;
    if (delayTime > 0) {
        EV << "delayed sending" << endl;
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

// ==============================================================================
// ==============================================================================
Agent::FlowUnit *Agent::getFlowUnit(FlowTuple &tuple)
{
    auto i = flowTable.find(tuple);
    FlowUnit *fu = nullptr;
    if (i == flowTable.end()) {
        fu = &flowTable[tuple];
        fu->state = UNREGISTERED;
        fu->active = false;
        fu->cacheAddress = false; // should be determined by CA
        fu->cachingActive = false; // should be set by CA
        fu->dataAgent = tuple.destAddress.UNSPECIFIED_ADDRESS; // should be set by CA
        fu->loadSharing = false;
        fu->locationUpdate = false;
    }
    else {
        fu = &(i->second);
    }
    return fu;
}

IPv6Address *Agent::getAssociatedAddress(IPv6Address &dest)
{
    auto i = addressAssociation.find(dest);
    IPv6Address *ip;
    if (i == addressAssociation.end()) {
        ip = nullptr;
    } else {
        ip = &(i->second);
    }
    return ip;
}

bool Agent::isAddressAssociated(IPv6Address &dest)
{
    auto i = addressAssociation.find(dest);
    return (i != addressAssociation.end());
}

// TODO this function must be adjusted for signal strentgh. it's returning any interface currently.
InterfaceEntry *Agent::getInterface(IPv6Address destAddr, int destPort, int sourcePort, short protocol) { // const IPv6Address &destAddr,
    InterfaceEntry *ie = nullptr;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp() && ie->ipv6Data()->getPreferredAddress().isGlobal()) { return ie; }
    }
    return ie;
}

InterfaceEntry *Agent::getAnyInterface()
{
    InterfaceEntry *ie = nullptr;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp() && ie->ipv6Data()->getPreferredAddress().isGlobal()) { return ie; }
    }
    return ie;
}

void Agent::createInterfaceDownMessage(int id)
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

void Agent::handleInterfaceDownMessage(cMessage *msg)
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

void Agent::createInterfaceUpMessage(int id)
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

void Agent::handleInterfaceUpMessage(cMessage *msg)
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

void Agent::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    if(!isMA) { return; } // just execute for mobile node
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

Agent::InterfaceUnit *Agent::getInterfaceUnit(int id)
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

void Agent::updateAddressTable(int id, InterfaceUnit *iu)
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
        createSequenceUpdate();
    } else {
        addressTable.insert(std::make_pair(id,iu)); // if not, include this new
        am.addIPv6AddressToAddressMap(mobileId, iu->careOfAddress);
        if(sessionState == UNASSOCIATED) { createSessionInit(); } // first address is initialzed with session init
        else if(seqnoState == ASSOCIATED) { createSequenceUpdate(); } // next address must be updated by seq update
        else if(seqnoState == INITIALIZING) { throw cRuntimeError("ERROR updateAddressTable: not inserted interface id should be added when the seqnoState is registered."); }
    }
//    EV << "AM_MA: " << am.to_string() << endl;
    EV << "AM_MA: " << am.getCurrentSequenceNumber(mobileId) << " -> " << am.getLastAcknowledgemnt(mobileId) << endl;
}

//=========================================================================================================
//=========================================================================================================
//============================ Timer ==========================
// Returns the corresponding timer. If a timer does not exist, it is created and inserted to list.
// If a timer exists, it is canceled and should be overwritten
Agent::ExpiryTimer *Agent::getExpiryTimer(TimerKey& key, int timerType) {
    ExpiryTimer *timer;
    auto pos = expiredTimerList.find(key);
    if(pos != expiredTimerList.end()) {
        if(dynamic_cast<SessionInitTimer *>(pos->second)) {
            SessionInitTimer *sit = (SessionInitTimer *) pos->second;
            cancelAndDelete(sit->timer);
            timer = sit;
        } else if(dynamic_cast<SequenceInitTimer *>(pos->second)) {
            SequenceInitTimer *sit = (SequenceInitTimer *) pos->second;
            cancelAndDelete(sit->timer);
            timer = sit;
        } else if(dynamic_cast<SequenceUpdateTimer *>(pos->second)) {
            SequenceUpdateTimer *sut = (SequenceUpdateTimer *) pos->second;
            cancelAndDelete(sut->timer);
            timer = sut;
        } else if(dynamic_cast<LocationUpdateTimer *>(pos->second)) {
            LocationUpdateTimer *lut = (LocationUpdateTimer *) pos->second;
            cancelAndDelete(lut->timer);
            timer = lut;
        } else if(dynamic_cast<FlowRequestTimer *>(pos->second)) {
            FlowRequestTimer *frt = (FlowRequestTimer *) pos->second;
            cancelAndDelete(frt->timer);
            timer = frt;
        } else if(dynamic_cast<MobileInitTimer *>(pos->second)) {
            MobileInitTimer *mit = (MobileInitTimer *) pos->second;
            cancelAndDelete(mit->timer);
            timer = mit;
        } else if(dynamic_cast<UpdateNotifierTimer *>(pos->second)) {
            UpdateNotifierTimer *unt = (UpdateNotifierTimer *) pos->second;
            cancelAndDelete(unt->timer);
            timer = unt;
        } else if(dynamic_cast<InterfaceDownTimer *>(pos->second)) {
            throw cRuntimeError("ERROR Invoked InterfaceDownTimer timer creation although one in map exists. There shouldn't be an entry in the list");
        } else if(dynamic_cast<InterfaceUpTimer *>(pos->second)) {
            throw cRuntimeError("ERROR Invoked InterfaceUpTimer timer creation although one in map exists. There shouldn't be an entry in the list");
        } else { throw cRuntimeError("ERROR Received timer not known. It's not a subclass of ExpiryTimer. Therefore getExpir... throwed this exception."); }
        timer->timer = nullptr;
    }
    else { // no timer exist
        switch(timerType) {
            case TIMERTYPE_SESSION_INIT:
                timer = new SessionInitTimer();
                break;
            case TIMERTYPE_SEQNO_INIT:
                timer = new SequenceInitTimer();
                break;
            case TIMERTYPE_SEQ_UPDATE:
                timer = new SequenceUpdateTimer();
                break;
            case TIMERTYPE_LOC_UPDATE:
                timer = new LocationUpdateTimer();
                break;
            case TIMERTYPE_IF_DOWN:
                timer = new InterfaceDownTimer();
                break;
            case TIMERTYPE_IF_UP:
                timer = new InterfaceUpTimer();
                break;
            case TIMERTYPE_FLOW_REQ:
                timer = new FlowRequestTimer();
                break;
            case TIMERTYPE_MA_INIT:
                timer = new MobileInitTimer();
                break;
            case TIMERTYPE_SEQ_UPDATE_NOT:
                timer = new UpdateNotifierTimer();
                break;
            default:
                throw cRuntimeError("Timer is not known. Type of key is wrong, check that.");
        }
        timer->timer = nullptr; // setting new timer to null
        timer->ie = nullptr; // setting new timer to null
        expiredTimerList.insert(std::make_pair(key,timer)); // inserting timer in list
    }
    return timer;
}

bool Agent::cancelExpiryTimer(const IPv6Address &dest, int interfaceId, int timerType, uint64 id)
{
    TimerKey key(dest, interfaceId, timerType, id);
    auto pos = expiredTimerList.find(key);
    if(pos == expiredTimerList.end()) {
        return false; // list is empty
    }
    ExpiryTimer *timerToDelete = (pos->second);
    cancelEvent(timerToDelete->timer);
    expiredTimerList.erase(key);
    return true;
}

bool Agent::cancelAndDeleteExpiryTimer(const IPv6Address &dest, int interfaceId, int timerType, uint64 id)
{
    TimerKey key(dest, interfaceId, timerType, id);
    auto pos = expiredTimerList.find(key);
    if(pos == expiredTimerList.end()) {
        return false; // list is empty
    }
    ExpiryTimer *timerToDelete = (pos->second);
    cancelAndDelete(timerToDelete->timer);
    timerToDelete->timer = nullptr;
    expiredTimerList.erase(key);
    delete timerToDelete;
    return true;
}
// if a timer exists, returns true
bool Agent::pendingExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType, uint64 id) {
    TimerKey key(dest,interfaceId, timerType, id);
    auto pos = expiredTimerList.find(key);
    return pos != expiredTimerList.end();
}

} //namespace
