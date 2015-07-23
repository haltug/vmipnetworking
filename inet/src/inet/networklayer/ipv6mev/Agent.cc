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
#include "inet/networklayer/contract/ipv6/IPv6ControlInfo.h"
#include "inet/networklayer/ipv6/IPv6Datagram.h"
#include "inet/networklayer/ipv6/IPv6ExtensionHeaders.h"
#include "inet/networklayer/ipv6/IPv6InterfaceData.h"

namespace inet {

//Agent::~Agent() {
//    auto it = expiredTimerList.begin();
//    while(it != expiredTimerList.end()) {
//        TimerKey key = it->first;
//        it++;
//        cancelAndDeleteExpiryTimer(key.dest,key.interfaceID,key.type);
//    }
//}

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
        fu->id = 0;
        fu->lifetime = 0;
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

IPv6Address *Agent::getAssociatedAddressInv(IPv6Address &dest)
{
    auto i = addressAssociationInv.find(dest);
    IPv6Address *ip;
    if (i == addressAssociationInv.end()) {
        ip = nullptr;
    } else {
        ip = &(i->second);
    }
    return ip;
}

bool Agent::isAddressAssociatedInv(IPv6Address &dest)
{
    auto i = addressAssociationInv.find(dest);
    return (i != addressAssociationInv.end());
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
//            cancelAndDelete(unt->timer); // is explicitly removed by createSeqUpdate functions.
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

bool Agent::cancelExpiryTimer(const IPv6Address &dest, int interfaceId, int timerType, uint64 id, uint seq, uint ack)
{   // remove based on seq and ack numbering
    if(ack > seq) throw cRuntimeError("CancelAndDeleteTimer: Ack cannot be greater as seq.");
    if(ack == seq) {
        TimerKey key(dest, interfaceId, timerType, id, seq);
        auto pos = expiredTimerList.find(key);
        if(pos == expiredTimerList.end()) {
            return false; // list is empty
        }
        ExpiryTimer *timerToDelete = (pos->second);
        cancelEvent(timerToDelete->timer);
        expiredTimerList.erase(key);
        return true;
    } else {
        bool timerExists = false;
        for(uint i=ack; i<=seq; i++) {
            TimerKey key(dest, interfaceId, timerType, id, i);
            auto pos = expiredTimerList.find(key);
            if(pos == expiredTimerList.end()) continue;
            ExpiryTimer *timerToDelete = (pos->second);
            cancelEvent(timerToDelete->timer);
            expiredTimerList.erase(key);
            timerExists = true;
        }
        return timerExists;
    }
}

bool Agent::cancelAndDeleteExpiryTimer(const IPv6Address &dest, int interfaceId, int timerType, uint64 id, uint seq, uint ack)
{   // remove based on seq and ack numbering
    if(ack > seq) throw cRuntimeError("CancelAndDeleteTimer: Ack cannot be greater as seq.");
    if(ack == seq) {
        TimerKey key(dest, interfaceId, timerType, id, seq);
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
    } else {
        bool timerExists = false;
        for(uint i=ack; i<=seq; i++) {
            TimerKey key(dest, interfaceId, timerType, id, i);
            auto pos = expiredTimerList.find(key);
            if(pos == expiredTimerList.end()) continue;
            ExpiryTimer *timerToDelete = (pos->second);
            cancelAndDelete(timerToDelete->timer);
            timerToDelete->timer = nullptr;
            expiredTimerList.erase(key);
            delete timerToDelete;
            timerExists = true;
        }
        return timerExists;
    }
}
// if a timer exists, returns true
bool Agent::pendingExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType, uint64 id, uint seq) {
    TimerKey key(dest,interfaceId, timerType, id, seq);
    auto pos = expiredTimerList.find(key);
    return pos != expiredTimerList.end();
}

} //namespace
