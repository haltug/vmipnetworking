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

#ifndef __INET_ADDRESSSYNCHRONIZER_H_
#define __INET_ADDRESSSYNCHRONIZER_H_

#include <map>
#include <omnetpp.h>
#include <string>
#include <vector>

#include "inet/common/INETDefs.h"
#include "inet/networklayer/contract/ipv6/IPv6Address.h"

namespace inet {

class AddressManagement
{

  public:
    AddressManagement();
    virtual ~AddressManagement();
    friend std::ostream& operator<<(std::ostream& os, const AddressManagement& am);
    static const int SEQ_FIELD_SIZE = 256;

    // A list of IPv6 addresses stored as vector
    typedef std::vector<IPv6Address> IPv6AddressList;
    // A map containing a list of IPv6 addresses (IPv6AddressList) and
    // associated to a sequence number (key)
    typedef std::map<uint,IPv6AddressList> SequenceTable;
    // A structure that represents the address map of a vehicle agent
    // with current sequence number, last acknowledgement, sequence table

    struct AddressMapEntry
    {
        uint64 mobileID;
        uint currentSequenceNumber;
        uint lastAcknowledgement;
        uint activeLinks;
        SequenceTable sequenceTable;
        SimTime timestamp;
    };

    struct AddressChange
    {
        uint addedAddresses = 0;
        uint removedAddresses = 0;
        IPv6AddressList getAddedIPv6AddressList;
        IPv6AddressList getRemovedIPv6AddressList;
    };
    AddressChange getAddressChange(uint64 id, uint ack, uint seq); // getAddressChange(uint64 id, uint ack, uint seq)
    AddressChange getAddressEntriesOfSeqNo(uint64 id, uint seq); // getAddressEntriesOfSeqNo

    // A map that contains all elements of vehicle agents
    typedef std::map<uint64,AddressMapEntry> AddressMap;
    // The address map variable
    AddressMap addressMap;

    const AddressMap getAddressMap() { return addressMap; }
    uint initiateAddressMap(uint64 id, int seq); // for VA
    bool insertNewId(uint64 id, uint seqno, IPv6Address& addr); // for CA+DA
    void addIpToMap(uint64 id, IPv6Address& addr); // for adding of new ip by MA and CA. a new value increments the seqNo
    void removeIpFromMap(uint64 id, IPv6Address& addr); // for removing existing ip in last seq entry
    void insertSeqTableToMap(uint64 id, IPv6AddressList& addr, uint seq); // insert a complete seqTable to address map


    uint getSeqNo(const uint64 id) const; // returns the last sequence number
    uint getAckNo(uint64 id) const;     // returns the last ack number
    void setAckNo(uint64 id, uint seqno);
    void setSeqNo(uint64 id, uint seqno);
    bool isSeqNoAcknowledged(uint64 id) const;
    bool isIdInitialized(uint64 id)  const;
    bool isIpRegistered(uint64 id, IPv6Address& dest, uint seq);
    IPv6AddressList getAddressList(uint64 id); // getCurrentAddressList(uint64)

    // prints given parameter in string form
    std::string to_string() const;
    std::string to_string(IPv6AddressList addrList) const;
    std::string to_string(SequenceTable seqTable) const;
    std::string to_string(AddressMapEntry addrMapEntry) const;
    std::string to_string(AddressMap addrMap) const;
    std::string to_string(AddressChange addrChange) const;
};
} //namespace

#endif
