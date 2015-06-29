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

#ifndef VEHICLEIDENTIFICATIONTYPE_H_
#define VEHICLEIDENTIFICATIONTYPE_H_

#include "inet/common/INETDefs.h"
#include "inet/networklayer/contract/IL3AddressType.h"
#include "inet/networklayer/contract/generic/GenericNetworkProtocolControlInfo.h"

namespace inet {

class INET_API VehicleIdentificationType : public IL3AddressType
{
  public:
    static VehicleIdentificationType INSTANCE;

  public:
    VehicleIdentificationType() {}
    virtual ~VehicleIdentificationType() {}
    // hier kann eine anpassung erforderlich sein. siehe dazu interface von tl
    virtual int getAddressBitLength() const override { return 64; }
    virtual int getMaxPrefixLength() const override { return 0; }
    virtual L3Address getUnspecifiedAddress() const override { return VehicleIdentification(); }
    virtual L3Address getBroadcastAddress() const override { return VehicleIdentification(); }
    virtual L3Address getLinkLocalManetRoutersMulticastAddress() const override { return VehicleIdentification(); }
    virtual L3Address getLinkLocalRIPRoutersMulticastAddress() const override { return VehicleIdentification(); }
    virtual INetworkProtocolControlInfo *createNetworkProtocolControlInfo() const override { return new GenericNetworkProtocolControlInfo(); }
    virtual L3Address getLinkLocalAddress(const InterfaceEntry *ie) const override { return VehicleIdentification(); }
};

} /* namespace inet */

#endif /* VEHICLEIDENTIFICATIONTYPE_H_ */
