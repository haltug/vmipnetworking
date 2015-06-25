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

#include <omnetpp.h>
#include <vector>
#include <map>
#include "inet/networklayer/contract/ipv6/IPv6Address.h"
#include "inet/networklayer/common/L3Address.h"

namespace inet {

class IPv6AddressManagement : public cSimpleModule
{
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
  private:
  public:
    const int SEQ_FIELD_SIZE = 64;
    typedef std::vector<IPv6Address> IPv6AddressList;
    typedef std::map<uint,IPv6AddressList> SequenceTable;
    struct IPv6AddressMapEntry
    {
        L3Address mobileID;
        uint currentSequenceNumber;
        uint lastAcknowledgement;
        SequenceTable sequenceTable;
        SimTime timestamp;

    };
    typedef std::map<L3Address,IPv6AddressMapEntry> IPv6AddressMap;
    IPv6AddressMap ipv6AddressMap;

    uint initiateAddressMap(L3Address& id); // for VA
    void initiateAddressMap(L3Address& id, uint seqNo, IPv6Address& addr); // for CA+DA
    void addIPv6AddressToAddressMap(L3Address& id, IPv6Address& addr);
    void addIPv6AddressToAddressMap(L3Address& id, IPv6AddressList& addr);
    void removeIPv6AddressfromAddressMap(L3Address& id, IPv6Address& addr);
    void removeIPv6AddressfromAddressMap(L3Address& id, IPv6AddressList& addr);

    uint getCurrentSequenceNumber(L3Address& id);
    uint getLastAcknowledgemnt(L3Address& id);
    void setLastAcknowledgemnt(L3Address& id, uint seqno);
    bool isLastSequenceNumberAcknowledged(L3Address& id);
    bool isAddressMapOfMobileIDProvided(L3Address& id);

    struct IPv6AddressChange
    {
        uint addedAddresses = 0;
        uint removedAddresses = 0;
        IPv6AddressList getUnacknowledgedAddedIPv6AddressList;
        IPv6AddressList getUnacknowledgedRemovedIPv6AddressList
    };
    IPv6AddressChange getUnacknowledgedIPv6AddressList(L3Address& id, uint ack, uint seq);


};
} //namespace

#endif
