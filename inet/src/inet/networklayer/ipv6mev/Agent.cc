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
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/InterfaceTable.h"

namespace inet {

#define SEND_CA_INIT 1

Define_Module(Agent);

void Agent::initialize()
{
    isMA = par("isMA").boolValue();
    isCA = par("isCA").boolValue();
    if(isMA)
        state = UNASSOCIATED;
}

void Agent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()) {
        EV << "Self message received!" << endl;
        if(msg->getKind() == SEND_CA_INIT)
            resendCAInitialization(msg);
        else if (msg->getKind() == SEND_CA_SEQ_UPDATE)
            resendCASequenceUpdate(msg);
        else if (msg->getKind() == SEND_CA_SESSION_REQUEST)
            resendCASessionRequest(msg);
        else if (msg->getKind() == SEND_CA_LOC_UPDATE)
            resendCALocationUpdate(msg);
        else
            throw cRuntimeError("VA:handleMsg: Unknown timer expired. Which timer msg is unknown?");
    }
    // maybe it's better to use seperate extenstion headers instead of extending destination options
    else if (dynamic_cast<IdentificationHeader *> (msg)) {
        EV << " Received ID header message" << endl;
        IPv6ControlInfo *ctrlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
        IdentificationHeader *idHdr = (IdentificationHeader *) msg;
        if (dynamic_cast<ControlAgentHeader *>(idHdr)) {
            ControlAgentHeader *ca = (ControlAgentHeader *) idHdr;
            processCAMessages(ca, ctrlInfo);
        }
//        else if (dynamic_cast<DataAgentHeader *>(eh))
//            processDAMessages((IPv6Datagram *)msg, (DataAgentHeader *)eh);
//        else if (dynamic_cast<DataAgentHeader *>(eh))
//            processMAMessages((IPv6Datagram *)msg, (DataAgentHeader *)eh);
        else
            throw cRuntimeError("VA:handleMsg: Extension Hdr Type not known. What did you send?");
    }
    else
        throw cRuntimeError("VA:handleMsg: cMessage Type not known. What did you send?");
}

Agent::Agent() {}

Agent::~Agent() {}

Agent::createCAInitialization() {
    EV << "createCAInitialization" << endl;
    const char *ctrlAgentAddr = par("controlAgentAddress");
    L3Address caAddr;
    L3AddressResolver().tryResolve(ctrlAgentAddr, caAddr);
    CA_Address = caAddr.toIPv6();
    if(isMA && state == UNASSOCIATED)
        state = INITIALIZE;
    cMessage *msg = new cMessage("sendingCAinit",SEND_CA_INIT);
    InterfaceTable* ift = (InterfaceTable*)InterfaceTableAccess().get();
    InterfaceEntry *ie;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && !(ie->isDown())) {
            if(ie->isConnected())
                break;
        }
    }
    TimerKey key(CA_Address,ie->getInterfaceId(),KEY_CA_INIT);
    InitMessageTimer *imt = (InitMessageTimer *) getExpiryTimer(key,MSG_CA_INIT);
    imt->dest = CA_Address;
    imt->ie = ie;
    imt->timer = msg;
    imt->ackTimeout = TIM_CA_INIT;
    msg->setControlInfo(imt);
    scheduleAt(simTime(),msg);
}

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
            case MSG_CA_INIT:
                timer = new InitMessageTimer();
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

} //namespace
