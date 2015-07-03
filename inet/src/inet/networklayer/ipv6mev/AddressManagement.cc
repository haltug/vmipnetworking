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

#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "inet/networklayer/ipv6mev/AddressManagement.h"
#include "inet/networklayer/ipv6mev/VehicleIdentification.h"

namespace inet {


Define_Module(AddressManagement);

std::ostream& operator<<(std::ostream& os, const AddressManagement::AddressMapEntry& ame)
{
    // TODO Fix output to watch variables of seq table
//    os << "ID:" << ame.mobileID << " SEQ: " << ame.currentSequenceNumber << " ACK: " << ame.lastAcknowledgement << " SEQ_TABLE: " << ame.sequenceTable << " TS: " << SIMTIME_STR(ame.timestamp) << "\n";
      os << "ID:" << ame.mobileID << " SEQ: " << ame.currentSequenceNumber << " ACK: " << ame.lastAcknowledgement << "\n";
    return os;
}

void AddressManagement::initialize()
{
    WATCH_MAP(addressMap);
}

void AddressManagement::handleMessage(cMessage *msg)
{
    throw cRuntimeError("This module doesn't process messages");
}

AddressManagement::AddressManagement()
{
}

AddressManagement::~AddressManagement()
{
}
uint AddressManagement::initiateAddressMap(uint64 id)
{
    return AddressManagement::initiateAddressMap(id,0);
}

// initialzation for VA
uint AddressManagement::initiateAddressMap(uint64 id, int seed)
{
// TODO check if map exists and is filled. if so delete all entries.
    AddressMapEntry addressEntry;
    addressEntry.mobileID = id; // setting mobile id
//    if(seed == 0) {
//        srand (time(NULL));
//    } else {
//        srand (seed);
//    }
//    uint seqno = (uint) ((rand() % (SEQ_FIELD_SIZE - 1)) + 1); // initiating any number except from 0
    uint seqno = (uint) (seed  % (SEQ_FIELD_SIZE - 1)) + 1; // this line should be deleted and the above commented code reimplemented
    addressEntry.currentSequenceNumber = seqno; // initialize random seq number
    addressEntry.lastAcknowledgement = 0; // not acknowledged
    IPv6AddressList ipv6addressList; // create empty list
    SequenceTable seqTable; // create seq table
    seqTable[seqno] = ipv6addressList; // insert list at seqno
    addressEntry.sequenceTable = seqTable; // set seq table in object
    addressEntry.timestamp = simTime(); // set timestamp
    addressMap[id] = addressEntry; // assign map to id
    return seqno; // return id to initialize at CA
}

// initialization for CA and DA
void AddressManagement::initiateAddressMap(uint64 id, uint seqno, IPv6Address& addr)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        // should a error occur?
    }
    AddressMapEntry addressEntry;
//    addressEntry.mobileID = id.toID().getId(); // setting mobile id
    addressEntry.currentSequenceNumber = seqno; // set seqno
    addressEntry.lastAcknowledgement = seqno; // acknowledge given seq
    IPv6AddressList ipv6AddressList; // creating new list
    ipv6AddressList.push_back(addr); // adding address into list
    addressEntry.sequenceTable[seqno] = ipv6AddressList; // assigning to seq table
    addressEntry.timestamp = simTime(); // setting timestamp
    addressMap[id] = addressEntry; // adding into map
}
// Adds an IPv6 address in list of id
void AddressManagement::addIPv6AddressToAddressMap(uint64 id, IPv6Address& addr)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        SequenceTable seqTable (addressMap[id].sequenceTable); // get current sequence table
        if(!seqTable.count(addressMap[id].currentSequenceNumber)) // check if seq table with given seq number exists
            throw cRuntimeError("AddIPv6:Sequence Table with seqNo does not exist.");
        IPv6AddressList newIPv6AddressList (seqTable[addressMap[id].currentSequenceNumber]); // copy current ipaddrList in new list
        if (std::find(newIPv6AddressList.begin(), newIPv6AddressList.end(), addr) != newIPv6AddressList.end()) // check if at any position given ip addr exists
        {
            EV_INFO << "IP address exists in address map. Should it be inserted twice?" << endl;
            return;
        }
        newIPv6AddressList.push_back(addr); // add new addr at the end of list
        addressMap[id].currentSequenceNumber = (addressMap[id].currentSequenceNumber + 1) % SEQ_FIELD_SIZE;
//        uint newSeqno = (++addressMap[id].currentSequenceNumber % SEQ_FIELD_SIZE); // increment seq no
        seqTable[addressMap[id].currentSequenceNumber] = newIPv6AddressList; // insert (or replace is exists) new addr list in seq table
        addressMap[id].sequenceTable = seqTable; // set modified seq table to map
        addressMap[id].timestamp = simTime(); // set a current timestamp
    } else {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}
// Removes an IPv6 addresses in list of id. TODO SEQ exceeds max size of 64
void AddressManagement::removeIPv6AddressFromAddressMap(uint64 id, IPv6Address& addr)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        SequenceTable seqTable (addressMap[id].sequenceTable); // get current sequence table
        if(!seqTable.count(addressMap[id].currentSequenceNumber)) // check if seq table with given seq number exists
            throw cRuntimeError("RemIPv6:Sequence Table with seqNo does not exist.");
        IPv6AddressList newIPv6AddressList (seqTable[addressMap[id].currentSequenceNumber]); // copy current ipaddrList in new list
        if (std::find(newIPv6AddressList.begin(), newIPv6AddressList.end(), addr) != newIPv6AddressList.end()) // check if addr exists in list
        {
            newIPv6AddressList.erase(std::remove(newIPv6AddressList.begin(), newIPv6AddressList.end(), addr), newIPv6AddressList.end()); // remove addr at
            addressMap[id].currentSequenceNumber = (addressMap[id].currentSequenceNumber + 1) % SEQ_FIELD_SIZE;
//          uint newSeqno = (++addressMap[id].currentSequenceNumber % SEQ_FIELD_SIZE); // increment seq no
            seqTable[addressMap[id].currentSequenceNumber] = newIPv6AddressList; // insert (or replace is exists) new addr list in seq table
            addressMap[id].sequenceTable = seqTable; // set modified seq table to map
            addressMap[id].timestamp = simTime(); // set a current timestamp

        } else {
            throw cRuntimeError("IP address does not exist in address map. Selected wrong addr?");
        }
    } else {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}
// Returns the change of IPv6 addresses as lists from ack up to the seq
AddressManagement::AddressChange AddressManagement::getUnacknowledgedIPv6AddressList(uint64 id, uint ack, uint seq)
{
    if(addressMap.count(id)) // check if id exists in map
    {
       AddressChange addressChange;
       if(ack > seq) // 61 > 1
           seq += (uint) SEQ_FIELD_SIZE; // Modulo operation if seq and ack exceeds max SEQ_FIELD_SIZE = 64
       for(uint idx=ack; idx<seq; idx++)
       {
           SequenceTable seqTable (addressMap[id].sequenceTable); // get current sequence table
           if(!seqTable.count(idx % (uint) SEQ_FIELD_SIZE )) // check if seq table with given seq number exists
               throw cRuntimeError("Sequence table with index (seqNo) does not exist.");
           if(!seqTable.count((idx+1) % (uint) SEQ_FIELD_SIZE)) // check if seq table with given seq number exists. serves as comparison.
               throw cRuntimeError("Sequence table with index+1 (seqNo=1) does not exist.");
           IPv6AddressList currIPv6AddressList (seqTable[idx % (uint) SEQ_FIELD_SIZE]); // get address list of index
           IPv6AddressList nextIPv6AddressList (seqTable[(idx+1) % (uint) SEQ_FIELD_SIZE]); // get address list of index + 1
           if(currIPv6AddressList.size() < nextIPv6AddressList.size()) // added case
           {
               addressChange.addedAddresses++;
               IPv6AddressList difference(1);
               std::set_difference(nextIPv6AddressList.begin(), nextIPv6AddressList.end(), currIPv6AddressList.begin(), currIPv6AddressList.end(), difference.begin()); // get difference of both vectors
               if(difference.size() != 1) // check how many differences are discovered
               {
                   throw cRuntimeError( "AddressChangeAdd: Size of address list difference should be one.");
               } else
               {
                   addressChange.getUnacknowledgedAddedIPv6AddressList.push_back(difference.front()); // put the difference address in local variable
               }
           } else if(currIPv6AddressList.size() > nextIPv6AddressList.size())
           { // deleted case
              addressChange.removedAddresses++;
              IPv6AddressList difference(1);
              std::set_difference(currIPv6AddressList.begin(), currIPv6AddressList.end(), nextIPv6AddressList.begin(), nextIPv6AddressList.end(),difference.begin());
              if(difference.size() != 1)
              {
                  throw cRuntimeError("AddressChangeRem: Size of address list difference should be one.");
              } else
              {
                  addressChange.getUnacknowledgedRemovedIPv6AddressList.push_back(difference.front()); // put the difference address in local variable
              }
           } else
           {
               throw cRuntimeError("Size of current and next IPv6 address list is same. Should be at least a difference.");
           }
       }
       return addressChange;
    } else
    {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

uint AddressManagement::getCurrentSequenceNumber(const uint64 id) const
{
    if(addressMap.count(id)) // check if id exists in map
    {
        return addressMap.find(id)->second.currentSequenceNumber;
    } else
    {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

uint AddressManagement::getLastAcknowledgemnt(const uint64 id) const
{
    if(addressMap.count(id)) // check if id exists in map
    {
        return addressMap.find(id)->second.lastAcknowledgement;
    } else {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

void AddressManagement::setLastAcknowledgemnt(uint64 id, uint seqno)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        addressMap[id].lastAcknowledgement = seqno;
    } else {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

bool AddressManagement::isLastSequenceNumberAcknowledged(uint64 id) const
{
    if(addressMap.count(id)) // check if id exists in map
    {
        return (addressMap.find(id)->second.lastAcknowledgement==addressMap.find(id)->second.currentSequenceNumber);
    } else {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

bool AddressManagement::isAddressMapOfMobileIDProvided(uint64 id) const
{
    return addressMap.count(id);
}

// Prints all IPv6 Addresses in IPv6AddressList
std::string AddressManagement::to_string(IPv6AddressList addrList) const
{
    std::string str="";
    bool first = true;
    for (IPv6Address ip : addrList )
    {
        if(!first)
            str.append("->");
        str.append(ip.str());
        first = false;
    }
    return str;
}

// Prints sequence table: seqNo + associated IPv6AddressList
std::string AddressManagement::to_string(SequenceTable seqTable) const
{
    std::string str="";
    for( auto& item : seqTable )
    {
//        str.append("SeqTab:");
        str.append(std::to_string(item.first)); // uint number to string
        str.append(":");
        str.append(AddressManagement::to_string(item.second));
        str.append("\n");
    }
    return str;
}
// Prints elements of AddressMapEntry: current seqNo, last ack, entire seqTable
std::string AddressManagement::to_string(AddressMapEntry addrMapEntry) const
{
    std::string str="";
//    str.append("Id:");
//    str.append(addrMapEntry.mobileID.str());
    str.append(";Seq:");
    str.append(std::to_string(addrMapEntry.currentSequenceNumber));
    str.append(";Ack:");
    str.append(std::to_string(addrMapEntry.lastAcknowledgement));
    str.append(";\n");
    str.append(AddressManagement::to_string(addrMapEntry.sequenceTable));
    return str;
}
// Prints a given addressMap
std::string AddressManagement::to_string(AddressMap addrMap) const
{
    std::string str="";
    for( auto& item : addrMap )
    {
        str.append("Id:");
        char buf[VehicleIdentification::ID_SIZE+1];
        sprintf(buf, "%016llX", item.first);
        std::string str2 = std::string(buf);
        str2.insert(VehicleIdentification::ID_SIZE/2, ":");
        str.append(str2); // uint number to string
        str.append(AddressManagement::to_string(item.second));
    }
    return str;
}
// Prints a addressMap of this object
std::string AddressManagement::to_string() const
{
    return AddressManagement::to_string(addressMap);
}
// Prints a list modified addresses
std::string AddressManagement::to_string(AddressChange addrChange) const
{
    std::string str = "";
    str.append("Add:");
    str.append(std::to_string(addrChange.addedAddresses));
    str.append(";Rem:");
    str.append(std::to_string(addrChange.removedAddresses));
    str.append(";#:");
    std::string add = AddressManagement::to_string(addrChange.getUnacknowledgedAddedIPv6AddressList);
    str.append(add);
    if(add.size()>1)
        str.append("->");
    str.append(AddressManagement::to_string(addrChange.getUnacknowledgedRemovedIPv6AddressList));
    str.append("\n");
    return str;
}


} //namespace
