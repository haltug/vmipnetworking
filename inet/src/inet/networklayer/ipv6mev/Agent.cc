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
            srand(1234); // TODO must be changed
            mobileId = (uint64) rand(); // TODO should be placed in future
            am.initiateAddressMap(mobileId, rand());
        }
        if(isCA) {
        }
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
        IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
        ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID);
    }
    if (stage == INITSTAGE_APPLICATION_LAYER) {
        if(isMA) {
            startTime = par("startTime");
            timeoutMsg = new cMessage("timer");
            timeoutMsg->setKind(MSG_START_TIME);
            scheduleAt(startTime, timeoutMsg);
        }
    }
    WATCH(mobileId);
    WATCH(CA_Address);
    WATCH(isMA);
    WATCH(isCA);
//    WATCHMAP(interfaceToIPv6AddressList);
//    WATCHMAP(directAddressList);
}

void Agent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()) {
        EV << "Self message received: " << msg->getKind() << endl;
        if(msg->getKind() == MSG_START_TIME) {
            EV << "Starter msg received" << endl;
            createSessionInit();
        }
        else if(msg->getKind() == MSG_SESSION_INIT) {
            EV << "CA_init_msg received" << endl;
            sendSessionInit(msg);
        }
        else if(msg->getKind() == MSG_SEQNO_INIT) {
            EV << "CA_init_msg received" << endl;
            sendSequenceInit(msg);
        }
//        else if (msg->getKind() == SEND_CA_SEQ_UPDATE)
//            resendCASequenceUpdate(msg);
//        else if (msg->getKind() == SEND_CA_SESSION_REQUEST)
//            resendCASessionRequest(msg);
//        else if (msg->getKind() == SEND_CA_LOC_UPDATE)
//            resendCALocationUpdate(msg);
        else
            throw cRuntimeError("handleMessage: Unknown timer expired. Which timer msg is unknown?");
    }
//    else if (dynamic_cast<IPv6Datagram *>(msg)) {
//        EV << "IPv6Datagram received." << endl;
//        IPv6ExtensionHeader *eh = (IPv6ExtensionHeader *)msg->getContextPointer();
//        IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
//        IPv6Datagram *datagram = (IPv6Datagram *)msg;
//        if(dynamic_cast<MobileAgentOptionHeader *>(eh)) {
//            EV << "MobileAgentOptionHeader received." << endl;
//            MobileAgentOptionHeader *optHeader = (MobileAgentOptionHeader *)eh;
//            messageProcessingUnitMA(optHeader, datagram, controlInfo);
//        }
//        else if(dynamic_cast<ControlAgentOptionHeader *>(eh)) {
//            EV << "ControlAgentOptionHeader received." << endl;
//            ControlAgentOptionHeader *optHeader = (ControlAgentOptionHeader *)eh;
//            messageProcessingUnitCA(optHeader, datagram, controlInfo);
//        }
//        else if(dynamic_cast<ControlAgentOptionHeader *>(eh)) {
//            EV << "DataAgentOptionHeader received. Not implemented yet." << endl;
//        }
//    }
    else if (dynamic_cast<IdentificationHeader *> (msg)) {
        EV << "A: Received id_header" << endl;
        IPv6ControlInfo *ctrlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
        IdentificationHeader *idHdr = (IdentificationHeader *) msg;
        if (dynamic_cast<ControlAgentHeader *>(idHdr)) {
            ControlAgentHeader *ca = (ControlAgentHeader *) idHdr;
            processControlAgentMessage(ca, ctrlInfo);
        } else if (dynamic_cast<DataAgentHeader *>(idHdr)) {
//            DataAgentHeader *da = (DataAgentHeader *) idHdr;
//            processDAMessages(da, ctrlInfo);
        } else if (dynamic_cast<MobileAgentHeader *>(idHdr)) {
            MobileAgentHeader *ma = (MobileAgentHeader *) idHdr;
            processMobileAgentMessages(ma,ctrlInfo);
        } else {
            throw cRuntimeError("A:handleMsg: Extension Hdr Type not known. What did you send?");
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
        cancelExpiryTimer(key.dest,key.interfaceID,key.type);
    }
}

void Agent::createSessionInit() {
    EV << "MA: Create CA_init" << endl;
    const char *ctrlAgentAddr = par("controlAgentAddress");
    L3Address caAddr;
    L3AddressResolver().tryResolve(ctrlAgentAddr, caAddr);
    CA_Address = caAddr.toIPv6();
    if(isMA && sessionState == UNASSOCIATED) {
        sessionState = INITIALIZE;
    }
    cMessage *msg = new cMessage("sendingCAinit",MSG_SESSION_INIT);
    InterfaceEntry *ie = getInterface(CA_Address);
    if(!ie)
        throw cRuntimeError("MA: No interface exists.");
    TimerKey key(CA_Address,ie->getInterfaceId(),TIMERKEY_SESSION_INIT);
    SessionInitTimer *sit = (SessionInitTimer *) getExpiryTimer(key,TIMERTYPE_SESSION_INIT);
    sit->dest = CA_Address;
    sit->ie = ie;
    sit->timer = msg;
    sit->ackTimeout = TIMEOUT_SESSION_INIT;
    sit->nextScheduledTime = simTime();
    msg->setContextPointer(sit);
    scheduleAt(simTime(),msg);
}

void Agent::createSequenceInit() {
    EV << "MA: Create CA_seq_init" << endl;
    if(!isMA && sessionState != REGISTERED)
        throw cRuntimeError("MA: Not registered at CA. Cannot run seq init.");
    if(isMA && seqnoState == UNASSOCIATED) {
        seqnoState = INITIALIZE;
    }
    cMessage *msg = new cMessage("sendingCAseqInit", MSG_SEQNO_INIT);
    InterfaceEntry *ie = getInterface(CA_Address);
    if(!ie)
        throw cRuntimeError("MA: No interface exists.");
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

void Agent::sendSessionInit(cMessage* msg) {
    EV << "Send CA_init" << endl;
//    if(true) { // send as extension header
//        InitMessageTimer *imt = (InitMessageTimer *) msg->getContextPointer();
//        InterfaceEntry *ie = imt->ie;
//        IPv6Address &dest = imt->dest;
//        const IPv6Address &src = ie->ipv6Data()->getPreferredAddress(); // to get src ip
//        imt->nextScheduledTime = simTime() + imt->ackTimeout;
//        imt->ackTimeout = (imt->ackTimeout)*2; // double ack time
//        MobileAgentOptionHeader *mah = new MobileAgentOptionHeader();
//        mah->setIdInit(true);
//        mah->setIdAck(false);
//        mah->setId(mobileId);
//        mah->setByteLength(SIZE_AGENT_HEADER);
//        sendToLowerLayer(mah, dest, src);
//        scheduleAt(imt->nextScheduledTime, msg);
//        return;
//    }
        SessionInitTimer *sit = (SessionInitTimer *) msg->getContextPointer();
        InterfaceEntry *ie = sit->ie;
        const IPv6Address &dest = sit->dest;
        const IPv6Address &src = ie->ipv6Data()->getPreferredAddress(); // to get src ip
        sit->nextScheduledTime = simTime() + sit->ackTimeout;
        sit->ackTimeout = (sit->ackTimeout)*2;
        MobileAgentHeader *mah = new MobileAgentHeader("ca_init");
        mah->setIdInit(true);
        mah->setIdAck(false);
        mah->setId(mobileId);
        mah->setByteLength(SIZE_AGENT_HEADER);
        sendToLowerLayer(mah, dest, src);
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
    mah->setIdInit(true);
    mah->setIdAck(true);
    mah->setSeqValid(true);
    mah->setAckValid(false);
    mah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    mah->setIpAcknowledgementNumber(0);
    mah->setAddedAddressesArraySize(1);
    mah->setAddedAddresses(0,src);
    mah->setId(mobileId);
    mah->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR);
    sendToLowerLayer(mah, dest, src);
    scheduleAt(sit->nextScheduledTime, msg);
}



void Agent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr, int interfaceId, simtime_t delayTime) {
    EV << "A: Creating IPv6ControlInfo to lower layer" << endl;
    IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
    ctrlInfo->setProtocol(IP_PROT_IPv6EXT_ID); // todo must be adjusted
    ctrlInfo->setDestAddr(destAddr);
    ctrlInfo->setSrcAddr(srcAddr);
    ctrlInfo->setHopLimit(255);
    InterfaceEntry *ie = getInterface(destAddr);
    ctrlInfo->setInterfaceId(ie->getInterfaceId());
    msg->setControlInfo(ctrlInfo);
    cGate *outgate = gate("toLowerLayer");
    EV << "IPv6ControlInfo: DestAddr=" << ctrlInfo->getDestAddr() << " SrcAddr=" << ctrlInfo->getSrcAddr() << " InterfaceId=" << ctrlInfo->getInterfaceId() << endl;
    if (delayTime > 0) {
        EV << "delayed sending" << endl;
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

void Agent::processMobileAgentMessages(MobileAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    EV << "CA: processing message from MA" << endl;
    if(isCA)
    {
        IPv6Address destAddr = controlInfo->getSrcAddr(); // address to be responsed
        IPv6Address sourceAddr = controlInfo->getDestAddr();
        if(agentHeader->getIdInit() && !agentHeader->getIdAck())
        { // init process request
            if(agentHeader->isName("ca_init"))
                EV << "CA: Received init message from MA." << endl;
//            std::find(newIPv6AddressList.begin(), newIPv6AddressList.end(), addr) != newIPv6AddressList.end()) // check if at any position given ip addr exists
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
                EV << "CA: Id already exists. check for mistakes" << endl;
            } else {
                EV << "CA: Adding id to list" << endl;
                mobileIdList.push_back(agentHeader->getId());
                ControlAgentHeader *cah = new ControlAgentHeader("ma_init_ack");
                cah->setIdInit(true);
                cah->setIdAck(true);
//                cah->setHeaderLength(SIZE_AGENT_HEADER); // not necessary since 8 B is defined as default
                // TODO remove below assignment from here
                sendToLowerLayer(cah,destAddr, sourceAddr);
            }
        } else if (agentHeader->getSeqValid() && !agentHeader->getAckValid()) // || agentHdr->getIdInit() && agentHdr->getIdAck()
        { // init sequence number/address management
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
                EV << "CA: Received sequence initialize message from MA. Inserting in AddrMgmt-Unit." << endl;
                bool addrMgmtEntry = am.insertNewId(agentHeader->getId(), agentHeader->getIpSequenceNumber(), destAddr);
                if(addrMgmtEntry) {
                    EV << "CA: Initialized sequence number" << endl;
                    ControlAgentHeader *cah = new ControlAgentHeader("ma_seq_ack");
                    cah->setIdInit(true);
                    cah->setIdAck(true);
                    cah->setSeqValid(true);
                    cah->setIpSequenceNumber(am.getCurrentSequenceNumber(agentHeader->getId()));
                    sendToLowerLayer(cah,destAddr, sourceAddr);
                } else {
                    EV << "ERROR CA: Initialization of sequence number failed ERROR" << endl;
                }
            } else {
                throw cRuntimeError("CA should initialize sequence number but cannot find id in list.");
            }
        }
    } else
        throw cRuntimeError("Received message that is supposed to be received by control agent. check message type that has to be processed.");
    delete agentHeader;
    delete controlInfo;
}

void Agent::processControlAgentMessage(ControlAgentHeader* agentHeader, IPv6ControlInfo* controlInfo) {
    EV << "MA: Entering CA processing message" << endl;
    if(isMA) {
        if(agentHeader->getIdInit() && agentHeader->getIdAck() && !agentHeader->getSeqValid()) { // session init
            if(sessionState == INITIALIZE) {
                sessionState = REGISTERED;
                if(agentHeader->isName("ma_init_ack"))
                    EV << "MA: Received session ack. Session started. " << endl;
                IPv6Address &caAddr = controlInfo->getSrcAddr();
                InterfaceEntry *ie = ift->getInterfaceById(controlInfo->getInterfaceId());
                cancelExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_SESSION_INIT);
                EV << "MA: Session init timer removed. Process successfully finished." << endl;
                createSequenceInit();
            } else {
                EV << "MA: Session created yet. Why do I received it again?" << endl;
            }
        } else if (agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid()) {
            if(seqnoState == INITIALIZE) {
                seqnoState = REGISTERED;
                if(agentHeader->isName("ma_seq_ack"))
                    EV << "MA: Received seqno ack. SeqNo initialized." << endl;
                IPv6Address &caAddr = controlInfo->getSrcAddr();
                InterfaceEntry *ie = ift->getInterfaceById(controlInfo->getInterfaceId());
                cancelExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_SESSION_INIT);
                EV << "MA: Seqno init timer removed. Process successfully finished." << endl;
            } else { // if seqno is initialized, it should be a upper layer packet inside or seq confirmation or whatelse

            }
        }
    }
    delete agentHeader; // delete at this point because it's not used any more
    delete controlInfo;
}

// TODO this function must be adjusted for signal strentgh. it's returning any interface currently.
InterfaceEntry *Agent::getInterface(IPv6Address destAddr, int destPort, int sourcePort, short protocol) { // const IPv6Address &destAddr,
    InterfaceEntry *ie;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp())
            return ie;
    }
    return ie;
}

//============================ Timer =======================
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
        }
        else {
            timer->timer = nullptr;
        }
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
            default:
                throw cRuntimeError("Timer is not known. Type of key is wrong, check that.");
        }
        timer->timer = nullptr;
        timer->ie = nullptr;
        expiredTimerList.insert(std::make_pair(key,timer));
    }
    return timer;
}

bool Agent::cancelExpiryTimer(const IPv6Address &dest, int interfaceId, int timerType) {
    TimerKey key(dest, interfaceId, timerType);
    auto pos = expiredTimerList.find(key);
    if(pos == expiredTimerList.end()) {
        return false; // list is empty
    }
    ExpiryTimer *timerToDelete = (pos->second);
    cancelAndDelete(timerToDelete->timer);
    timerToDelete->timer = nullptr;
    expiredTimerList.erase(key);
    EV << "Deleted timer (type): " << key.type << endl;
    delete timerToDelete;
    return true;
}

bool Agent::pendingExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType) {
    TimerKey key(dest,interfaceId, timerType);
    auto pos = expiredTimerList.find(key);
    return pos != expiredTimerList.end();
}

} //namespace

// ============================================================================================
// ============================================================================================
// ============================================================================================
// ============================================================================================

//void Agent::sendToLowerLayer(cObject *obj, const IPv6Address& destAddr, const IPv6Address& srcAddr, int interfaceId, simtime_t delayTime)
//{
//    EV << "Creating IPv6ControlInfo for sending to IP and appending Extension header." << endl;
//    IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
//    ctrlInfo->setProtocol(IP_PROT_IPv6EXT_ID); // todo must be adjusted
//    ctrlInfo->setDestAddr(destAddr);
//    ctrlInfo->setSrcAddr(srcAddr);
//    ctrlInfo->setHopLimit(255);
//    ctrlInfo->setInterfaceId(interfaceId);
//    IPv6ExtensionHeader *extHeader = (IPv6ExtensionHeader *) obj;
//    ctrlInfo->addExtensionHeader(extHeader);
//    IdentificationHeader *msg = new IdentificationHeader("Empty pckt..");
//    msg->setControlInfo(ctrlInfo);
//    cGate *outgate = gate("toLowerLayer");
//    EV << "ctrlInfo: DestAddr=" << ctrlInfo->getDestAddr() << " SrcAddr=" << ctrlInfo->getSrcAddr() << " InterfaceId=" << ctrlInfo->getInterfaceId() << endl;
//    if (delayTime > 0) {
//        EV << "delayed sending" << endl;
//        sendDelayed(msg, delayTime, outgate);
//    }
//    else {
//        send(msg, outgate);
//    }
//}
// message processing unit to be used by control agent and data agents. msg sent by mobile agent is of type extension header
//void Agent::messageProcessingUnitMA(MobileAgentOptionHeader *optHeader, IPv6Datagram *datagram, IPv6ControlInfo *controlInfo)
//{
//    EV << "CA: Entered msg processing unit: parse msg of type extension hdr." << endl;
//    if(isCA) {// check if you are ca or da
//        IPv6Address &destAddr = datagram->getSrcAddress(); // address to be responsed
//        IPv6Address &sourceAddr = datagram->getDestAddress(); // address address from sender
//        if(optHeader->getIdInit() && !optHeader->getIdAck()) { // init process request
//            EV << "CA: name of msg is as expected." << endl;
////            std::find(newIPv6AddressList.begin(), newIPv6AddressList.end(), addr) != newIPv6AddressList.end()) // check if at any position given ip addr exists
//            if(std::find(mobileIdList.begin(), mobileIdList.end(), optHeader->getId()) != mobileIdList.end()) {
//                EV << "CA id already exists. check for mistakes" << endl;
//            } else {
//                EV << "CA: adding id to list and sending response." << endl;
//                mobileIdList.push_back(optHeader->getId());
//                ControlAgentOptionHeader *option = new ControlAgentOptionHeader();
//                option->setIdInit(true);
//                option->setIdAck(true);
//                option->setByteLength(SIZE_AGENT_HEADER);
////                    ControlAgentHeader *cah = new ControlAgentHeader("MA Init Ack");
////                    cah->setIdentificationHeaderType(CONTROL_AGENT);
////                    cah->setIdInit(true);
////                    cah->setIdAck(true);
////                    cah->setHeaderLength(SIZE_AGENT_HEADER);
//                    // TODO remove below assignment from here
////                    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
////                    InterfaceEntry *ie = ift->getInterfaceById(datagram->getInterfaceId());
//                    sendToLowerLayer(option, destAddr, sourceAddr);
//                }
//            }
//    }
//    delete optHeader;
//    delete datagram;
//    delete controlInfo;
//}

//void Agent::messageProcessingUnitCA(ControlAgentOptionHeader *optHeader, IPv6Datagram *datagram, IPv6ControlInfo *controlInfo)
//{
//    EV << "MA: Entering msg processing unit: parse ext hdr" << endl;
//    if(isMA) {
//        if(optHeader->getIdInit() && optHeader->getIdAck()) { // session init
//            if(state == INITIALIZE) {
//                state = REGISTERED;
//                EV << "session start confirmed. " << endl;
//                IPv6Address &caAddr = datagram->getSrcAddress();
//                InterfaceEntry *ie = ift->getInterfaceById(controlInfo->getInterfaceId());
//                cancelExpiryTimer(caAddr, ie->getInterfaceId(), TIMERKEY_CA_INIT);
//                EV << "session start confirmed and from timer removed. END. " << endl;
//            } else {
//                EV << "session created yet." << endl;
//            }
//        } else {
//        }
//    }
//    delete optHeader; // delete at this point because it's not used any more
//    delete datagram;
//    delete controlInfo;
//}
