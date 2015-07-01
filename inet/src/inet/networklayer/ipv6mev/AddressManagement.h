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

class INET_API AddressManagement : public cSimpleModule
{
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

  public:
    AddressManagement();
    virtual ~AddressManagement();
    static const int SEQ_FIELD_SIZE = 64;

  public:
    // A list of IPv6 addresses stored as vector
    typedef std::vector<IPv6Address> IPv6AddressList;
    // A map containing a list of IPv6 addresses (IPv6AddressList) and
    // associated to a sequence number (key)
    typedef std::map<uint,IPv6AddressList> SequenceTable;
    // A structure that represents the address map of a vehicle agent
    // with current sequence number, last acknowledgement, sequence table
    struct AddressMapEntry
    {
        uint64 mobileID; // unnecessary
        uint currentSequenceNumber;
        uint lastAcknowledgement;
        SequenceTable sequenceTable;
        SimTime timestamp;

    };
    friend std::ostream& operator<<(std::ostream& os, const AddressMapEntry& ame);
    // A map that contains all elements of vehicle agents
    typedef std::map<uint64,AddressMapEntry> AddressMap;
    // The address map variable
    AddressMap addressMap;

    uint initiateAddressMap(uint64 id); // for VA
    uint initiateAddressMap(uint64 id, int seed); // for VA
    void initiateAddressMap(uint64 id, uint seqno, IPv6Address& addr); // for CA+DA
    void addIPv6AddressToAddressMap(uint64 id, IPv6Address& addr);
    void removeIPv6AddressFromAddressMap(uint64 id, IPv6Address& addr);
//    void addIPv6AddressToAddressMap(L3Address& id, IPv6AddressList& addr);
//    void removeIPv6AddressfromAddressMap(L3Address& id, IPv6AddressList& addr);

    uint getCurrentSequenceNumber(uint64 id);
    uint getLastAcknowledgemnt(uint64 id);
    void setLastAcknowledgemnt(uint64 id, uint seqno);
    bool isLastSequenceNumberAcknowledged(uint64 id);
    bool isAddressMapOfMobileIDProvided(uint64 id);

    struct AddressChange
    {
        uint addedAddresses = 0;
        uint removedAddresses = 0;
        IPv6AddressList getUnacknowledgedAddedIPv6AddressList;
        IPv6AddressList getUnacknowledgedRemovedIPv6AddressList;
    };
    AddressChange getUnacknowledgedIPv6AddressList(uint64 id, uint ack, uint seq);

    std::string to_string();
    std::string to_string(IPv6AddressList addrList);
    std::string to_string(SequenceTable seqTable);
    std::string to_string(AddressMapEntry addrMapEntry);
    std::string to_string(AddressMap addrMap);
    std::string to_string(AddressChange addrChange);
};
} //namespace

#endif
