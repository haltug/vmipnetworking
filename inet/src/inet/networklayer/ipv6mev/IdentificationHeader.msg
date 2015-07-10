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
    int nextHeader = IP_PROT_IPv6EXT_ID;
    int byteLength;
}

// Identification headers are represented as IPv6 extension of destination type
// 
packet MobileAgentHeader extends IdentificationHeader 
{
	   byteLength = 16;
	   bool idInit;
	   bool idAck;
	   bool seqValid;
	   bool ackValid;
	   bool addValid;
	   bool remValid;
	   bool directAddrInit;
	   bool redirectAddrInit;
	   uint8 ipSequenceNumber;
	   uint8 ipAcknowledgementNumber;
	   uint8 ipAddingField;
	   uint8 ipRemovingField;
	   uint8 functionField;
	   uint64 id;
	   IPv6Address addedAddresses[];
	   IPv6Address removedAddresses[];
	   IPv6Address redirectAddress;
}

packet ControlAgentHeader extends IdentificationHeader
{
	    byteLength = 8;
	    bool idInit;
	    bool idAck;
	    bool seqValid;
	    bool directAddrAck;
	    bool redirectAddrAck;
    	uint8 ipSequenceNumber;
    	uint8 functionField1;
    	uint8 functionField2;
    	uint8 functionField3;	   
    	IPv6Address redirectAddress[];
}

packet DataAgentHeader extends IdentificationHeader
{
	    byteLength = 8;
	    bool idInit;
	    bool idAck;
	    bool seqValid;
	    bool directAddrAck;
	    bool redirectAddrAck;
    	uint8 ipSequenceNumber;
    	uint8 functionField1;
    	uint8 functionField2;
    	uint8 functionField3;		   
    	IPv6Address redirectAddress[];
}
