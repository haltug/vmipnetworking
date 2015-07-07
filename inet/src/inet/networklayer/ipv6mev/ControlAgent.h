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

#ifndef __INET_CONTROLAGENT_H_
#define __INET_CONTROLAGENT_H_

#include <omnetpp.h>
#include <vector>
#include <map>

#include "inet/common/INETDefs.h"
#include "inet/networklayer/contract/ipv6/IPv6Address.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/ipv6tunneling/IPv6Tunneling.h"
#include "inet/networklayer/icmpv6/IPv6NeighbourDiscovery.h"


namespace inet {

/**
 * TODO - Generated class
 */
class ControlAgent : public cSimpleModule
{
  protected:
    IInterfaceTable *ift;
    IPv6RoutingTable *rt6;
    IPv6NeighbourDiscovery *ipv6nd;

    struct TargetToAgentAddressMapping
    {
        IPv6Address CN_Address;
        IPv6Address DA_Address;

        TargetToAgentAddressMapping(IPv6Address _cn, IPv6Address _da)
        {
            CN_Address = _cn;
            DA_Address = _da;
        }
    };

    typedef std::vector<TargetToAgentAddressMapping> TargetToAgentMappingList;
    typedef std::map<L3Address,TargetToAgentMappingList> TargetToAgentListOfVehicle;
    TargetToAgentListOfVehicle targetToAgentListOfVehicle;

    typedef std::vector<IPv6Address> DataAgentList;
    typedef std::map<L3Address, DataAgentList> ActiveDataAgentList;
    ActiveDataAgentList activeDataAgentList;




















  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

} //namespace

#endif
