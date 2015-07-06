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

#ifndef IDENTIFICATIONHEADER_H_
#define IDENTIFICATIONHEADER_H_

#include "inet/common/INETDefs.h"
#include "inet/networklayer/ipv6mev/IdentificationHeader_m.h"

namespace inet {

class INET_API VehicleAgentHeader : public VehicleAgentHeader_Base
{
    public:
        VehicleAgentHeader() : VehicleAgentHeader_Base() {}
        VehicleAgentHeader(const VehicleAgentHeader& other) : VehicleAgentHeader_Base(other) {}
        VehicleAgentHeader& operator=(const VehicleAgentHeader& other) {VehicleAgentHeader_Base::operator=(other); return *this;}
        virtual VehicleAgentHeader *dup() const override { return new VehicleAgentHeader(*this); }
};

class INET_API ControlAgentHeader : public ControlAgentHeader_Base
{
    public:
        ControlAgentHeader() : ControlAgentHeader_Base() {}
        ControlAgentHeader(const ControlAgentHeader& other) : ControlAgentHeader_Base(other) {}
        ControlAgentHeader& operator=(const ControlAgentHeader& other) {ControlAgentHeader_Base::operator=(other); return *this;}
        virtual ControlAgentHeader *dup() const override { return new ControlAgentHeader(*this); }
};

class INET_API DataAgentHeader : public DataAgentHeader_Base
{
    public:
        DataAgentHeader() : DataAgentHeader_Base() {}
        DataAgentHeader(const DataAgentHeader& other) : DataAgentHeader_Base(other) {}
        DataAgentHeader& operator=(const DataAgentHeader& other) {DataAgentHeader_Base::operator=(other); return *this;}
        virtual DataAgentHeader *dup() const override { return new DataAgentHeader(*this); }
};

} /* namespace inet */

#endif /* IDENTIFICATIONHEADER_H_ */
