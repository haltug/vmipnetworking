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

#include "inet/networklayer/ipv6mev/VehicleAgent.h"

namespace inet {

Define_Module(VehicleAgent);

VehicleAgent::~VehicleAgent()
{
    // do nothing for the moment. TODO
}

void VehicleAgent::initialize(int stage) // added int stage param
{
    cSimpleModule::initialize(stage); // purpose?

    ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    ipv6nd = getModuleFromPar<IPv6NeighbourDiscovery>(par("ipv6NeighbourDiscoveryModule"), this);    //Zarrar Yousaf 17.07.07
    am = getModuleFromPar<AddressManagement>(par("addressManagementModule"), this);

    WATCH_MAP(interfaceToIPv6AddressList);
    WATCH_MAP(targetToAgentList);
}

void VehicleAgent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()) {

    }
    else if (dynamic_cast<IPv6Datagram *> (msg)) {
        IPv6ExtensionHeader *eh = (IPv6ExtensionHeader *)msg->getContextPointer();
        if (dynamic_cast<ControlAgentHeader *>(eh))
            processControlAgentMessages((IPv6Datagram *)msg, (ControlAgentHeader *)eh);
        else if (dynamic_cast<DataAgentHeader *>(eh))
            processDataAgentMessages((IPv6Datagram *)msg, (DataAgentHeader *)eh);
        else
            throw cRuntimeError("VA:handleMsg: Extension Hdr Type not known. What did you send?");
    }
    else
        throw cRuntimeError("VA:handleMsg: cMessage Type not known. What did you send?");
}

} //namespace
