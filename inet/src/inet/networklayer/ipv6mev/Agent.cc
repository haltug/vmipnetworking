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

std::ostream& operator<<(std::ostream& os, const Agent& a)
{
    os << a.str(a.addressMap);
    return os;
};

std::ostream& operator<<(std::ostream& os, const Agent::FlowTuple& ft)
{
    os << "Tuple {\nP: " << ft.protocol << " \nDest: "<< ft.destAddress << ":" << ft.destPort << " \nSrc: _:" << ft.sourcePort << " \nId: " << ft.interfaceId << "\n}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Agent::FlowUnit& fu)
{
    os << "\nUnit {\nState: " << fu.state << " \nId: " << fu.id << " \nMobileAgent: " << fu.mobileAgent.str() << " \nDataAgent: " << fu.dataAgent << " \nCorrNode: " << fu.nodeAddress << "\n}";
    return os;
}

// TODO implement destructor
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
        fu->isFlowActive = false;
        fu->isAddressCached = false; // should be determined by CA
        fu->dataAgent = IPv6Address::UNSPECIFIED_ADDRESS; // should be set by CA
        fu->id = 0;
        fu->lifetime = 0;
    }
    else {
        fu = &(i->second);
    }
    return fu;
}

IPv6Address *Agent::getAssociatedAddress(const IPv6Address &dest)
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

bool Agent::isAddressAssociated(const IPv6Address &dest)
{
    auto i = addressAssociation.find(dest);
    return (i != addressAssociation.end());
}

IdentificationHeader *Agent::getAgentHeader(short type, short protocol, uint seq, uint ack, uint64 id) {
    IdentificationHeader *ih = new IdentificationHeader();
    ih->setNextHeader(protocol);
    ih->setByteLength(SIZE_AGENT_HEADER);
    if(type == 1) {
        ih->setIsMobileAgent(true);
        ih->setIsControlAgent(false);
        ih->setIsDataAgent(false);
    } else if (type == 2) {
        ih->setIsMobileAgent(false);
        ih->setIsControlAgent(true);
        ih->setIsDataAgent(false);
    } else if (type == 3) {
        ih->setIsMobileAgent(false);
        ih->setIsControlAgent(false);
        ih->setIsDataAgent(true);
    } else { throw cRuntimeError("HeaderConfig: Agent type not known"); }
    ih->setIsIdInitialized(false);
    ih->setIsIdAcked(false);
    ih->setIsSeqValid(false);
    ih->setIsAckValid(false);
    ih->setIsIpModified(false);

    ih->setIpSequenceNumber(seq);
    ih->setIpAcknowledgementNumber(ack);
    ih->setIpAddingField(0);
    ih->setIpRemovingField(0);

    ih->setIsWithAgentAddr(false);
    ih->setIsWithNodeAddr(false);
    ih->setIsWithReturnAddr(false);
    ih->setIsReturnAddrCached(false);

    ih->setFunctionField(0);
    ih->setId(id);
    ih->setIPaddressesArraySize(0);
    return ih;
}

//=========================================================================================================
//=========================================================================================================
//============================ Timer ==========================
// Returns the corresponding timer. If a timer does not exist, it is created and inserted to list.
// If a timer exists, it is canceled and will be overwritten
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
        } else if(dynamic_cast<UpdateAckTimer *>(pos->second)) {
            UpdateAckTimer *uat = (UpdateAckTimer *) pos->second;
            cancelAndDelete(uat->timer);
            timer = uat;
        } else if(dynamic_cast<SequenceUpdateAckTimer *> (pos->second)) {
            SequenceUpdateAckTimer *suat = (SequenceUpdateAckTimer *) pos->second;
            cancelAndDelete(suat->timer);
            timer = suat;
        } else if(dynamic_cast<UpdateNotifierTimer *>(pos->second)) {
            UpdateNotifierTimer *unt = (UpdateNotifierTimer *) pos->second;
//            cancelAndDelete(unt->timer); // is explicitly removed by createSeqUpdate functions.
            timer = unt;
        } else if(dynamic_cast<InterfaceDownTimer *>(pos->second)) {
            throw cRuntimeError("ERROR Invoked InterfaceDownTimer timer creation although one in map exists. There shouldn't be an entry in the list");
        } else if(dynamic_cast<InterfaceUpTimer *>(pos->second)) {
            throw cRuntimeError("ERROR Invoked InterfaceUpTimer timer creation although one in map exists. There shouldn't be an entry in the list");
        } else if(dynamic_cast<InterfaceChangeTimer *>(pos->second)) {
            throw cRuntimeError("ERROR Invoked InterfaceChangeTimer timer creation although one in map exists. There shouldn't be an entry in the list");
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
            case TIMERTYPE_IF_CHANGE:
                timer = new InterfaceChangeTimer();
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
            case TIMERTYPE_UPDATE_ACK:
                timer = new UpdateAckTimer();
                break;
            case TIMERTYPE_SEQ_UPDATE_ACK:
                timer = new SequenceUpdateAckTimer();
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

//=========================================================================================================
//=========================================================================================================
//============================ Address Control System ==========================

void Agent::initAddressMap(uint64 id, uint seq)
{
    uint seqno = (uint) (seq  % (SEQ_FIELD_LENGTH - 1)) + 1;
    AddressList list;
    AddressTable table;
    table[seqno] = list;
    AddressMapEntry entry (id, seqno, 1, table);
    addressMap[id] = entry;
}

bool Agent::initAddressMap(uint64 id, uint seq, IPv6Address addr)
{
    if(addressMap.count(id)) { // check if id exists in map
        return false; // return false if id has been inserted previously
    } else {
        AddressTuple tuple(0, addr);
        AddressList list; // creating new list
        list.push_back(tuple); // adding address into list
        AddressTable table;
        table[seq] = list;
        AddressMapEntry entry(id, seq, seq, table);
        addressMap[id] = entry; // adding into map
        return true;
    }
}

void Agent::insertAddress(uint64 id, int iface, IPv6Address addr)
{
    if(addressMap.count(id)) { // check if id exists in map
        if(!addressMap[id].addressTable.count(addressMap[id].seqNo)) // check if seq table with given seq number exists
            throw cRuntimeError("AM_insertAddress:Sequence Table with seqNo does not exist.");
        AddressTuple tuple(iface, addr);
        std::vector<AddressTuple>::iterator it = std::find(addressMap[id].addressTable[addressMap[id].seqNo].begin(), addressMap[id].addressTable[addressMap[id].seqNo].end(), tuple);
        if(it != addressMap[id].addressTable[addressMap[id].seqNo].end()) {
            EV_WARN << "AM_insertAddress: Inserting IP address=" << it->address << " with if=" << it->interface << " again. This should not happen." << endl;
//            return;
//        } else { // interface is not assigned with an ip address
        }
        AddressList list(addressMap[id].addressTable[addressMap[id].seqNo]); // copying old list
        list.push_back(tuple); // inserting new addr with interface id
        addressMap[id].seqNo = (addressMap[id].seqNo + 1) % SEQ_FIELD_LENGTH; // incrementing seqno
        addressMap[id].addressTable[addressMap[id].seqNo] = list; // appending list to table
    } else {
        throw cRuntimeError("AM_insertAddress: ID is not found in AddressMap. Create entry for ID.");
    }
}

void Agent::deleteAddress(uint64 id, int iface, IPv6Address addr) {
    if(addressMap.count(id)) { // check if id exists in map
        if(!addressMap[id].addressTable.count(addressMap[id].seqNo)) // check if seq table with given seq number exists
            throw cRuntimeError("AM_deleteAddress: AddressTable with seqNo does not exist.");
        AddressTuple tuple(iface, addr);
        AddressList list(addressMap[id].addressTable[addressMap[id].seqNo]); // copying old list
        std::vector<AddressTuple>::iterator it = std::find(list.begin(), list.end(), tuple);
        if(it != list.end()) {
            list.erase(it); // deleting addr with interface at position it
            addressMap[id].seqNo = (addressMap[id].seqNo + 1) % SEQ_FIELD_LENGTH; // incrementing seqno
            addressMap[id].addressTable[addressMap[id].seqNo] = list;
        } else { // interface is not assigned with an ip address
            std::string a = addr.str();
            throw cRuntimeError("AM_deleteAddress: Interface/Address has not been assigned. Id: %d Addr: %s Iface: %d .", id, a.c_str(), iface);
        }
    } else {
        throw cRuntimeError("AM_deleteAddress: is not found in AddressMap. Create entry for ID.");
    }
}

void Agent::insertTable(uint64 id, uint seq, AddressList addr)
{
    if(addressMap.count(id)) { // check if id exists in map
        addressMap[id].seqNo = seq;
        addressMap[id].ackNo = seq;
        addressMap[id].addressTable[addressMap[id].seqNo] = addr; // replacing/inserting new addr
    } else {
        throw cRuntimeError("AM_insertTable: ID is not found in AddressMap. Create entry for ID.");
    }
}

Agent::AddressDiff Agent::getAddressList(uint64 id, uint seq, uint ack)
{
    if(addressMap.count(id)) { // check if id exists in map
       AddressDiff diff;
       if(ack > seq) { // Modulo operation if seq and ack exceeds max SEQ_FIELD_SIZE = 64 ; 61 > 1
           seq += (uint) SEQ_FIELD_LENGTH;
       }
       for(uint idx=ack; idx<seq; idx++) {
           if(!addressMap[id].addressTable.count(idx % (uint) SEQ_FIELD_LENGTH)) // check if seq table with given seq number exists
               throw cRuntimeError("AM_getAddressList: Table with seqNo= %d does not exist.", idx % (uint) SEQ_FIELD_LENGTH);
           if(!addressMap[id].addressTable.count((idx+1) % (uint) SEQ_FIELD_LENGTH)) // check if seq table with given seq number exists
               throw cRuntimeError("AM_getAddressList: Table with seqNo= %d does not exist.",(idx+1) % (uint) SEQ_FIELD_LENGTH);
           AddressList currList (addressMap[id].addressTable[idx % (uint) SEQ_FIELD_LENGTH]);
           AddressList nextList (addressMap[id].addressTable[(idx+1) % (uint) SEQ_FIELD_LENGTH]);
           if(currList.size() < nextList.size()) { // added case
               AddressList difference(1);
               std::set_difference(nextList.begin(), nextList.end(), currList.begin(), currList.end(), difference.begin()); // get difference of both vectors
               if(difference.size() != 1) { // check how many differences are discovered
                   throw cRuntimeError("AM_getAddressList: inserted address; Size of address list difference should be one.");
               } else {
                   diff.insertedList.push_back(difference.front()); // put the difference address in local variable
               }
           } else if(currList.size() > nextList.size()) { // deleted case
               AddressList difference(1);
               std::set_difference(currList.begin(), currList.end(), nextList.begin(), nextList.end(),difference.begin());
               if(difference.size() != 1) {
                  throw cRuntimeError("AM_getAddressList: deleted address; Size of address list difference should be one.");
               } else {
                  diff.deletedList.push_back(difference.front()); // put the difference address in local variable
               }
           } else {
               throw cRuntimeError("AM_getAddressList: Size of current and next IPv6 address list is same. Should be at least a difference.");
           }
       }
       return diff;
    } else {
        throw cRuntimeError("AM_getAddressList: ID is not found in AddressMap. Create entry for ID.");
    }
}

Agent::AddressDiff Agent::getAddressList(uint64 id, uint seq) {
    if(addressMap.count(id)) { // check if id exists in map
        if(!addressMap[id].addressTable.count(seq)) // check if seq table with given seq number exists
            throw cRuntimeError("AM_getAddressList2: AddressTable with seqNo= %d does not exist.", seq);
        AddressDiff diff;
        diff.insertedList = addressMap[id].addressTable[seq];
        return diff;
    } else {
        throw cRuntimeError("AM_getAddressList2: ID is not found in AddressMap. Create entry for ID.");
    }
}

Agent::AddressDiff Agent::getAddressList(uint64 id) {
    return getAddressList(id, addressMap[id].seqNo);
}

bool Agent::isAddressInserted(uint64 id, uint seq, IPv6Address dest) {
    if(addressMap.count(id)) { // check if id exists in map
        if(!addressMap[id].addressTable.count(seq)) // check if seq table with given seq number exists
            throw cRuntimeError("AM_isAddressInserted: AddressTable with seqNo= %d does not exist.", seq);
        for (auto &it : addressMap[id].addressTable[seq]) {
            if(it.address == dest)
                return true;
        }
        return false;
    } else {
        throw cRuntimeError("AM_isAddressInserted: ID is not found in AddressMap. Create entry for ID.");
    }
}

Agent::AddressTuple Agent::getAddressTuple(uint64 id, uint seq, IPv6Address addr) {
    if(addressMap.count(id)) { // check if id exists in map
        if(!addressMap[id].addressTable.count(seq)) // check if seq table with given seq number exists
            throw cRuntimeError("AM_getAddressTuple: AddressTable with seqNo= %d does not exist.", seq);
        for (auto &it : addressMap[id].addressTable[seq]) {
            if(it.address == addr)
                return it;
        }
    } else {
        throw cRuntimeError("AM_getAddressTuple: ID is not found in AddressMap. Create entry for ID.");
    }
    throw cRuntimeError("AM_getAddressTuple: No addressTuple for address could be found.");
}

Agent::AddressTuple Agent::getAddressTuple(uint64 id, uint seq, int iface) {
    if(addressMap.count(id)) { // check if id exists in map
        if(!addressMap[id].addressTable.count(seq)) // check if seq table with given seq number exists
            throw cRuntimeError("AM_getAddressTuple2: AddressTable with seqNo= %d does not exist.", seq);
        for (auto &it : addressMap[id].addressTable[seq]) {
            if(it.interface == iface)
                return it;
        }
    } else {
        throw cRuntimeError("AM_getAddressTuple2: ID is not found in AddressMap. Create entry for ID.");
    }
    throw cRuntimeError("AM_getAddressTuple2: No addressTuple for interface could be found.");
}

bool Agent::isInterfaceInserted(uint64 id, uint seq, int iface) {
    if(addressMap.count(id)) { // check if id exists in map
        if(!addressMap[id].addressTable.count(seq)) // check if seq table with given seq number exists
            throw cRuntimeError("AM_isAddressInserted: AddressTable with seqNo= %d does not exist.", seq);
        for (auto &it : addressMap[id].addressTable[seq]) {
            if(it.interface == iface)
                return true;
        }
        return false;
    } else {
        throw cRuntimeError("AM_isAddressInserted: ID is not found in AddressMap. Create entry for ID.");
    }
}

bool Agent::isSeqNoAcknowledged(uint64 id) {
    if(addressMap.count(id)) // check if id exists in map
        return (addressMap.find(id)->second.ackNo==addressMap.find(id)->second.seqNo);
    else
        throw cRuntimeError("AM_getSeqNo: ID is not found in AddressMap. Create entry for ID.");
}

bool Agent::isIdInitialized(uint64 id) {
    return addressMap.count(id);
}

bool Agent::isSeqNoInitialized(uint64 id) {
    if(addressMap.count(id))
        if(!addressMap[id].addressTable.count(addressMap[id].seqNo))
            return true;
        else
            return false;
    else
        return false;
//        throw cRuntimeError("AM_getSeqNo: ID is not found in AddressMap. Create entry for ID.");
}

uint Agent::getSeqNo(uint64 id) {
    if(addressMap.count(id)) // check if id exists in map
        return addressMap[id].seqNo;
    else
        throw cRuntimeError("AM_getSeqNo: ID is not found in AddressMap. Create entry for ID.");
}
void Agent::setSeqNo(uint64 id, uint seqno) {
    if(addressMap.count(id)) // check if id exists in map
        addressMap[id].seqNo = seqno;
    else
        throw cRuntimeError("AM_setSeqNo: ID is not found in AddressMap. Create entry for ID.");
}
uint Agent::getAckNo(uint64 id) {
    if(addressMap.count(id)) // check if id exists in map
        return addressMap[id].ackNo;
    else
        throw cRuntimeError("AM_getAckNo: ID is not found in AddressMap. Create entry for ID.");
}
void Agent::setAckNo(uint64 id, uint ackno) {
    if(addressMap.count(id)) // check if id exists in map
        addressMap[id].ackNo = ackno;
    else
        throw cRuntimeError("AM_getSeqNo: ID is not found in AddressMap. Create entry for ID.");
}

// Prints a given addressMap
std::string Agent::str(AddressMap addrMap) const
{
    std::string str="";
    for( auto& item : addrMap )
    {
        str.append("ID=");
        char buf[16+1]; // length in char
        sprintf(buf, "%016llX", item.first);
        std::string str2 = std::string(buf);
        str2.insert(16/2, ":");
        str.append(str2); // uint number to string
        str.append(Agent::str(item.second));
    }
    return str;
}
// Prints elements of AddressMapEntry: current seqNo, last ack, entire seqTable
std::string Agent::str(AddressMapEntry entry) const
{
    std::string str="";
    str.append("; SEQ=");
    str.append(std::to_string(entry.seqNo));
    str.append("; ACK=");
    str.append(std::to_string(entry.ackNo));
    str.append(";\n");
    str.append(Agent::str(entry.addressTable));
    return str;
}

// Prints sequence table: seqNo + associated IPv6AddressList
std::string Agent::str(AddressTable table) const
{
    std::string str="";
    for( auto& item : table )
    {
        str.append(std::to_string(item.first)); // uint number to string
        str.append(":");
        str.append(Agent::str(item.second));
        str.append("\n");
    }
    return str;
}

// Prints all IPv6 Addresses in IPv6AddressList
std::string Agent::str(AddressList addrList) const
{
    std::string str="$";
    bool first = true;
    for (AddressTuple tuple : addrList )
    {
        if(!first)
            str.append("->");
        str.append("[");
        str.append(std::to_string(tuple.interface));
        str.append("]:");
        str.append(tuple.address.str());
        first = false;
    }
    return str;
}

} //namespace
