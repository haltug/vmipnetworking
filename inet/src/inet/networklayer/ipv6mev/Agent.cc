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
#include "inet/networklayer/ipv6mev/AddressManagement.h"
#include "inet/networklayer/ipv6mev/IdHeader.h"

namespace inet {

Define_Module(Agent);

void Agent::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER) {
        isMA = par("isMA").boolValue();
        isCA = par("isCA").boolValue();
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        if(isMA) {
            state = UNASSOCIATED;
//            srand(1234);
//            mobileId.IPv6Address((uint64)29, (uint64) rand()); // TODO should be placed in future
            mobileId = 255;
        }
        if(isCA) {
        }
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
        IPSocket ipSocket(gate("toLowerLayer"));
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
            createCAInitialization();
        }
        else if(msg->getKind() == MSG_CA_INIT) {
            EV << "CA init msg received" << endl;
            sendCAInitialization(msg);
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
    else if (dynamic_cast<IdentificationHeader *> (msg)) {
        EV << " Received ID header message" << endl;
        IPv6ControlInfo *ctrlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
        IdentificationHeader *idHdr = (IdentificationHeader *) msg;
        if (dynamic_cast<ControlAgentHeader *>(idHdr)) {
            ControlAgentHeader *ca = (ControlAgentHeader *) idHdr;
            processCAMessages(ca, ctrlInfo);
        } else if (dynamic_cast<DataAgentHeader *>(idHdr)) {
//            DataAgentHeader *da = (DataAgentHeader *) idHdr;
//            processDAMessages(da, ctrlInfo);
        } else if (dynamic_cast<MobileAgentHeader *>(idHdr)) {
            MobileAgentHeader *ma = (MobileAgentHeader *) idHdr;
            processMAMessages(ma,ctrlInfo);
        }
        else
            throw cRuntimeError("VA:handleMsg: Extension Hdr Type not known. What did you send?");
    }
    else if (dynamic_cast<IPv6Datagram *>(msg)) {
        IPv6ExtensionHeader *eh = (IPv6ExtensionHeader *)msg->getContextPointer();
        if(dynamic_cast<MobileAgentOptionHeader *>(eh)) {
            EV << "MobileAgentOptionHeader received." << endl;
            processMAMessages((IPv6Datagram *)msg, (MobileAgentOptionHeader *)eh);
        }//TODO implement this to end and implement idHeader with CaHdr
        else if(dynamic_cast<ControlAgentOptionHeader *>(eh)) {
            EV << "MobileAgentOptionHeader received." << endl;
            processMAMessages((IPv6Datagram *)msg, (MobileAgentOptionHeader *)eh);
        }
    }
    else
        throw cRuntimeError("VA:handleMsg: cMessage Type not known. What did you send?");
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

void Agent::createCAInitialization() {
    EV << "createCAInitialization" << endl;
    const char *ctrlAgentAddr = par("controlAgentAddress");
    L3Address caAddr;
    L3AddressResolver().tryResolve(ctrlAgentAddr, caAddr);
    CA_Address = caAddr.toIPv6();
    if(isMA && state == UNASSOCIATED)
        state = INITIALIZE;
    cMessage *msg = new cMessage("sendingCAinit",MSG_CA_INIT);
    InterfaceEntry *ie;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp())
                break;
    }
    if(!ie)
        throw cRuntimeError("No interface exists.");
    TimerKey key(CA_Address,ie->getInterfaceId(),TIMERKEY_CA_INIT);
    InitMessageTimer *imt = (InitMessageTimer *) getExpiryTimer(key,TIMERTYPE_INIT_MSG);
    imt->dest = CA_Address;
    imt->ie = ie;
    imt->timer = msg;
    imt->ackTimeout = TIME_CA_INIT;
    imt->nextScheduledTime = simTime();
    msg->setContextPointer(imt);
    scheduleAt(simTime(),msg);
}

void Agent::sendCAInitialization(cMessage* msg) {
    EV << "sendCAInitialization" << endl;
    InitMessageTimer *imt = (InitMessageTimer *) msg->getContextPointer();
    InterfaceEntry *ie = imt->ie;
    IPv6Address &dest = imt->dest;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress(); // to get src ip
    imt->nextScheduledTime = simTime() + imt->ackTimeout;
    imt->ackTimeout = (imt->ackTimeout)*2;
    MobileAgentHeader *mah = new MobileAgentHeader("CA init");
    mah->setIdentificationHeaderType(MOBILE_AGENT);
    mah->setIdInit(true);
    mah->setIdAck(false);
    mah->setId(mobileId);
    mah->setHeaderLength(FD_MIN_HEADER_SIZE);
    sendToLowerLayer(mah, dest, src);
    scheduleAt(imt->nextScheduledTime, msg);
}

void Agent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr, int interfaceId, simtime_t delayTime) {
    EV << "Appending ControlInfo to mobility message" << endl;
    IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
    ctrlInfo->setProtocol(IP_PROT_IPv6EXT_ID); // todo must be adjusted
    ctrlInfo->setDestAddr(destAddr);
    ctrlInfo->setSrcAddr(srcAddr);
    ctrlInfo->setHopLimit(255);
    ctrlInfo->setInterfaceId(interfaceId);
    msg->setControlInfo(ctrlInfo);
    cGate *outgate = gate("toLowerLayer");
    EV << "ctrlInfo: DestAddr=" << ctrlInfo->getDestAddr() << " SrcAddr=" << ctrlInfo->getSrcAddr() << " InterfaceId=" << ctrlInfo->getInterfaceId() << endl;
    if (delayTime > 0) {
        EV << "delayed sending" << endl;
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

void Agent::processMAMessages(MobileAgentHeader* agentHdr, IPv6ControlInfo* ipCtrlInfo) {
    EV << "CA: Entered MA processing" << endl;
    if(isCA) {
        IPv6Address destAddr = ipCtrlInfo->getSrcAddr(); // address to be responsed
        IPv6Address sourceAddr = ipCtrlInfo->getDestAddr();
        if(agentHdr->getIdInit() && !agentHdr->getIdAck()) { // init process request
            if(agentHdr->isName("CA init"))
                EV << "CA: name of msg is as expected." << endl;
//            std::find(newIPv6AddressList.begin(), newIPv6AddressList.end(), addr) != newIPv6AddressList.end()) // check if at any position given ip addr exists

            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHdr->getId()) != mobileIdList.end()) {
                EV << "CA id already exists. check for mistakes" << endl;
            } else {
                EV << "CA: adding id to list" << endl;
                mobileIdList.push_back(agentHdr->getId());
                ControlAgentHeader *cah = new ControlAgentHeader("MA Init Ack");
                cah->setIdentificationHeaderType(CONTROL_AGENT);
                cah->setIdInit(true);
                cah->setIdAck(true);
                cah->setHeaderLength(FD_MIN_HEADER_SIZE);
                // TODO remove below assignment from here
                ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
                InterfaceEntry *ie = ift->getInterfaceById(ipCtrlInfo->getInterfaceId());
                sendToLowerLayer(cah,destAddr, sourceAddr, ie->getInterfaceId());
            }
        }
    } else {
        delete agentHdr;
        delete ipCtrlInfo;
        throw cRuntimeError("Received message that is supposed to be received by control agent. check message type that has to be processed.");
    }
    delete agentHdr;
    delete ipCtrlInfo;
}

void Agent::processCAMessages(ControlAgentHeader* agentHdr, IPv6ControlInfo* ipCtrlInfo) {
    EV << "MA: Entering CA processing message" << endl;
    if(isMA) {
        if(agentHdr->getIdInit() && agentHdr->getIdAck()) { // session init
            if(state == INITIALIZE) {
                state = REGISTERED;
                EV << "session start confirmed. " << endl;
                IPv6Address &caAddr = ipCtrlInfo->getSrcAddr();
                InterfaceEntry *ie = ift->getInterfaceById(ipCtrlInfo->getInterfaceId());
                cancelExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_CA_INIT);
                EV << "session start confirmed and from timer removed. " << endl;
            } else {
                EV << "session created yet." << endl;
            }
        } else {

        }
    }
}

//============================ Timer =======================

// Returns the corresponding timer. If a timer does not exist, it is created and inserted to list.
// If a timer exists, it is canceled and should be overwritten
Agent::ExpiryTimer *Agent::getExpiryTimer(TimerKey& key, int timerType) {
    ExpiryTimer *timer;
    auto pos = expiredTimerList.find(key);
    if(pos != expiredTimerList.end()) {
        if(dynamic_cast<InitMessageTimer *>(pos->second)) {
            InitMessageTimer *imt = (InitMessageTimer *) pos->second;
            cancelAndDelete(imt->timer);
            timer = imt;
        } else {
            timer->timer = nullptr;
        }
    } else { // no timer exist
        switch(timerType) {
            case TIMERTYPE_INIT_MSG:
                timer = new InitMessageTimer();
                break;
            case TIMERTYPE_SESSION_REQ:
                timer = new SessionRequestMessageTimer();
                break;
            case TIMERTYPE_SEQ_UPDATE:
                timer = new SequenceUpdateMessageTimer();
                break;
            case TIMERTYPE_LOC_UPDATE:
                timer = new LocationUpdateMessageTimer();
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
    delete timerToDelete;
    EV << "Timer deleted." << endl;
    return true;
}

bool Agent::pendingExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType) {
    TimerKey key(dest,interfaceId, timerType);
    auto pos = expiredTimerList.find(key);
    return pos != expiredTimerList.end();
}

} //namespace
