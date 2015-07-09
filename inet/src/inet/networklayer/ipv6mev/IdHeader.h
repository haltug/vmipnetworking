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

#ifndef IDHEADER_H_
#define IDHEADER_H_

#include "inet/common/INETDefs.h"
#include "inet/networklayer/ipv6mev/IdHeader_m.h"

namespace inet {

class INET_API MobileAgentOptionHeader : public MobileAgentOptionHeader_Base
{
public:
    MobileAgentOptionHeader() : MobileAgentOptionHeader_Base() {}
    MobileAgentOptionHeader(const MobileAgentOptionHeader& other) : MobileAgentOptionHeader_Base(other) {}
    MobileAgentOptionHeader& operator=(const MobileAgentOptionHeader& other) { MobileAgentOptionHeader_Base::operator =(other); return *this; }
    virtual MobileAgentOptionHeader *dup() const override { return new MobileAgentOptionHeader(*this); }
};

} /* namespace inet */


#endif /* IDHEADER_H_ */
