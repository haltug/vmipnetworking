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

cplusplus {{
#include "inet/common/INETDefs.h"
#include "inet/common/ByteArray.h"
#include "inet/networklayer/contract/ipv6/IPv6Address.h"
#include "inet/networklayer/ipv6/IPv6Datagram.h" // added by CB
#include "inet/networklayer/ipv6/IPv6ExtensionHeaders.h" // 17.10.07 - CB
}}

cplusplus {{
const int IP_PROT_IPv6EXT_ID = 140;
const int MAX_ID_HEADER_LENGTH = 1500;
}}


namespace inet;

class noncobject IPv6Address;
class noncobject IPv6DestinationOptionsHeader;
class noncobject IPv6ExtensionHeader;

// extends IPv6ExtensionHeader TODO
packet IdentificationHeader
{
    short nextHeader = IP_PROT_IPv6EXT_ID;
	byteLength = 16; // first 8 octet (64 bit = 8 byte) is not included but it is here declared for simplicity reason
    // Flag field 1
	bool isMobileAgent;
	bool isControlAgent;
	bool isDataAgent;
	bool isIdInitialized;
    bool isIdAcked;
    bool isSeqValid;
    bool isAckValid;
    bool isIpModified;
	// 
	uint8 ipSequenceNumber;
	uint8 ipAcknowledgementNumber;
	uint8 ipAddingField;
	uint8 ipRemovingField;
	//  flag field 2
	bool isWithAgentAddr;
	bool isWithNodeAddr;
	bool isWithReturnAddr;
	bool isReturnAddrCached;
	//
	uint8 functionField;
	uint64 id;
	IPv6Address iPaddresses[];
	   
    
}

