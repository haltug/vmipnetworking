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

#include "inet/networklayer/ipv6mev/AddressManagement.h"

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

// initialzation for VA
uint AddressManagement::initiateAddressMap(L3Address& id)
{
// TODO check if map exists and is filled. if so delete all entries.
    AddressMapEntry addressEntry;
    addressEntry.mobileID = id; // setting mobile id
    uint seqno = (uint) ((rand() % (AddressManagement::SEQ_FIELD_SIZE - 1)) + 1); // initiating any number except from 0
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
void AddressManagement::initiateAddressMap(L3Address& id, uint seqno, IPv6Address& addr)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        // should a error occur?
    }
    AddressMapEntry addressEntry;
    addressEntry.mobileID = id; // setting mobile id
    addressEntry.currentSequenceNumber = seqno; // set seqno
    addressEntry.lastAcknowledgement = seqno; // acknowledge given seq
    IPv6AddressList ipv6AddressList; // creating new list
    ipv6AddressList.push_back(addr); // adding address into list
    addressEntry.sequenceTable[seqno] = ipv6AddressList; // assigning to seq table
    addressEntry.timestamp = simTime(); // setting timestamp
    addressMap[id] = addressEntry; // adding into map
}

void AddressManagement::addIPv6AddressToAddressMap(L3Address& id, IPv6Address& addr)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        SequenceTable seqTable (addressMap[id].sequenceTable); // get current sequence table
        if(!seqTable.count(addressMap[id].currentSequenceNumber)) // check if seq table with given seq number exists
            throw cRuntimeError("Sequence Table with seqNo does not exist.");
        IPv6AddressList newIPv6AddressList (seqTable[addressMap[id].currentSequenceNumber]); // copy current ipaddrList in new list
        if (std::find(newIPv6AddressList.begin(), newIPv6AddressList.end(), addr) != newIPv6AddressList.end()) // check if at any position given ip addr exists
        {
            EV_INFO << "IP address exists in address map. Should it be inserted twice?";
            return;
        }
        newIPv6AddressList.push_back(addr); // add new addr at the end of list
        uint newSeqno = addressMap[id].currentSequenceNumber++; // increment seq no
        seqTable[newSeqno] = newIPv6AddressList; // insert (or replace is exists) new addr list in seq table
        addressMap[id].sequenceTable = seqTable; // set modified seq table to map
        addressMap[id].timestamp = simTime(); // set a current timestamp
    } else {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

void AddressManagement::removeIPv6AddressfromAddressMap(L3Address& id, IPv6Address& addr)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        SequenceTable seqTable (addressMap[id].sequenceTable); // get current sequence table
        if(!seqTable.count(addressMap[id].currentSequenceNumber)) // check if seq table with given seq number exists
            throw cRuntimeError("Sequence Table with seqNo does not exist.");
        IPv6AddressList newIPv6AddressList (seqTable[addressMap[id].currentSequenceNumber]); // copy current ipaddrList in new list
        if (std::find(newIPv6AddressList.begin(), newIPv6AddressList.end(), addr) != newIPv6AddressList.end()) // check if addr exists in list
        {
            newIPv6AddressList.erase(std::remove(newIPv6AddressList.begin(), newIPv6AddressList.end(), addr), newIPv6AddressList.end()); // remove addr at
            uint newSeqno = addressMap[id].currentSequenceNumber++; // increment seq no
            seqTable[newSeqno] = newIPv6AddressList; // insert (or replace is exists) new addr list in seq table
            addressMap[id].sequenceTable = seqTable; // set modified seq table to map
            addressMap[id].timestamp = simTime(); // set a current timestamp

        } else {
            throw cRuntimeError("IP address does not exist in address map. Selected wrong addr?");
        }
    } else {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

AddressManagement::AddressChange AddressManagement::getUnacknowledgedIPv6AddressList(L3Address& id, uint ack, uint seq)
{
    if(addressMap.count(id)) // check if id exists in map
    {
       AddressChange addressChange;
       for(uint idx=ack; idx<seq; idx++)
       {
           SequenceTable seqTable (addressMap[id].sequenceTable); // get current sequence table
           if(!seqTable.count(idx)) // check if seq table with given seq number exists
               throw cRuntimeError("Sequence table with index (seqNo) does not exist.");
           if(!seqTable.count(idx+1)) // check if seq table with given seq number exists. serves as comparison.
               throw cRuntimeError("Sequence table with index+1 (seqNo=1) does not exist.");
           IPv6AddressList currIPv6AddressList (seqTable[idx]); // get address list of index
           IPv6AddressList nextIPv6AddressList (seqTable[idx+1]); // get address list of index + 1
           if(currIPv6AddressList.size() < nextIPv6AddressList.size()) // added case
           {
               addressChange.addedAddresses++;
               IPv6AddressList difference;
               std::set_difference(currIPv6AddressList.begin(), currIPv6AddressList.end(), nextIPv6AddressList.begin(), nextIPv6AddressList.end(),std::inserter(difference, difference.begin())); // get difference of both vectors
               if(difference.size() != 1) // check how many differences are discovered
               {
                   throw cRuntimeError("Size of address list difference should be one. ");
               } else
               {
                   addressChange.getUnacknowledgedAddedIPv6AddressList.push_back(difference.front()); // put the difference address in local variable
               }
           } else if(currIPv6AddressList.size() > nextIPv6AddressList.size())
           { // deleted case
              addressChange.removedAddresses++;
              IPv6AddressList difference;
              std::set_difference(currIPv6AddressList.begin(), currIPv6AddressList.end(), nextIPv6AddressList.begin(), nextIPv6AddressList.end(),std::inserter(difference, difference.begin()));
              if(difference.size() != 1)
              {
                  throw cRuntimeError("Size of address list difference should be 1. ");
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

uint AddressManagement::getCurrentSequenceNumber(L3Address& id)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        return addressMap[id].currentSequenceNumber;
    } else
    {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

uint AddressManagement::getLastAcknowledgemnt(L3Address& id)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        return addressMap[id].lastAcknowledgement;
    } else {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

void AddressManagement::setLastAcknowledgemnt(L3Address& id, uint seqno)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        addressMap[id].lastAcknowledgement = seqno;
    } else {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

bool AddressManagement::isLastSequenceNumberAcknowledged(L3Address& id)
{
    if(addressMap.count(id)) // check if id exists in map
    {
        return (addressMap[id].lastAcknowledgement==addressMap[id].currentSequenceNumber);
    } else {
        throw cRuntimeError("ID is not found in AddressMap. Create entry for ID.");
    }
}

bool AddressManagement::isAddressMapOfMobileIDProvided(L3Address& id)
{
    return addressMap.count(id);
}


std::string AddressManagement::to_string(IPv6AddressList addrList)
{
    std::string str="";
    for (IPv6Address ip : addrList )
    {
        str.append(ip.str());
        str.append("\n");
    }
    return str;
}

std::string AddressManagement::to_string(SequenceTable seqTable)
{
    std::string str="";
    for( auto& item : seqTable )
    {
        str.append("=== SEQ: ");
        str.append(std::to_string(item.first)); // uint number to string
        str.append("===\n");
        str.append(AddressManagement::to_string(item.second));
    }
    return str;
}

std::string AddressManagement::to_string(AddressMapEntry addrMapEntry)
{
    std::string str="";
    str.append("Mobile ID: ");
    str.append(addrMapEntry.mobileID.str());
    str.append("; Current Seq: ");
    str.append(std::to_string(addrMapEntry.currentSequenceNumber));
    str.append("; Last Ack: ");
    str.append(std::to_string(addrMapEntry.lastAcknowledgement));
    str.append("; Sequence Table: \n");
    str.append(AddressManagement::to_string(addrMapEntry.sequenceTable));
    return str;
}

std::string AddressManagement::to_string(AddressMap addrMap)
{
    std::string str="";
    for( auto& item : addrMap )
    {
        str.append("=== ID: ");
        str.append(item.first.str()); // uint number to string
        str.append("===\n");
        str.append(AddressManagement::to_string(item.second));
    }
    return str;
}

std::string AddressManagement::to_string()
{
    return AddressManagement::to_string(addressMap);
}

} //namespace
