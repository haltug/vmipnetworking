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

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv6mev/VehicleAgent.h"
#include "inet/common/ModuleAccess.h"

namespace inet {

#define SEND_CA_INIT              1
#define SEND_CA_SESSION_REQUEST   2
#define SEND_CA_SEQ_UPDATE        3
#define SEND_CA_LOC_UPDATE        4

Define_Module(VehicleAgent);

VehicleAgent::~VehicleAgent()
{
    // do nothing for the moment. TODO
    if(!ca)
        delete ca;
}

void VehicleAgent::initialize(int stage) // added int stage param
{
    if (stage == INITSTAGE_NETWORK_LAYER) // start init module when network layer is initialized
    {
        cSimpleModule::initialize(stage); // purpose?

        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        ipv6nd = getModuleFromPar<IPv6NeighbourDiscovery>(par("ipv6NeighbourDiscoveryModule"), this);
        am = getModuleFromPar<AddressManagement>(par("addressManagementModule"), this);
        state = VehicleAgent::NONE;

        WATCH_MAP(interfaceToIPv6AddressList);
        WATCH_MAP(targetToAgentList);
    }
}

void VehicleAgent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()) {
        if(msg->getKind() == RESEND_CA_INIT)
            resendCAInitialization(msg);
        else if (msg->getKind() == RESEND_CA_SEQ_UPDATE)
            resendCASequenceUpdate(msg);
        else if (msg->getKind() == RESEND_CA_SESSION_REQUEST)
            resendCASessionRequest(msg);
        else if (msg->getKind() == RESEND_CA_LOC_UPDATE)
            resendCALocationUpdate(msg);
        else
            throw cRuntimeError("VA:handleMsg: Unknown timer expired. Which timer msg is unknown?");
    }
    // maybe it's better to use seperate extenstion headers instead of extending destination options
    else if (dynamic_cast<IPv6Datagram *> (msg)) {
        IPv6ExtensionHeader *eh = (IPv6ExtensionHeader *)msg->getContextPointer();
        if (dynamic_cast<ControlAgentHeader *>(eh))
            processCAMessages((IPv6Datagram *)msg, (ControlAgentHeader *)eh);
        else if (dynamic_cast<DataAgentHeader *>(eh))
            processDAMessages((IPv6Datagram *)msg, (DataAgentHeader *)eh);
        else
            throw cRuntimeError("VA:handleMsg: Extension Hdr Type not known. What did you send?");
    }
    else
        throw cRuntimeError("VA:handleMsg: cMessage Type not known. What did you send?");
}

void VehicleAgent::processCAMessages(IPv6Datagram *datagram, ControlAgentHeader *ctrlAgentHdr)
{
    if(state != VehicleAgent::REGISTERED)
    {

    } else
    {

    }
}

void VehicleAgent::processDAMessages(IPv6Datagram *datagram, DataAgentHeader *dataAgentHdr)
{
    if(state != VehicleAgent::REGISTERED)
    {

    } else
    {

    }
}

void VehicleAgent::sendCAInitialization()
{
    EV_INFO << "Sending CA init." << endl;
    if(!ca) {
        ca = new IPv6Address();
        ca->set(L3AddressResolver().resolve("CA*").toIPv6().str().c_str());
    }
    state = VehicleAgent::INITIALIZE;
//    cMessage *cMsg = new cMessage("sendCAInit", SEND_CA_INIT);
    VehicleAgentHeader *vaHdr = new VehicleAgentHeader("Init");
    vaHdr->setIdInit(true);
    vaHdr->setIdAck(false);
    vaHdr->setSeqValid(false);
    vaHdr->setAckValid(false);
    vaHdr->setAddValid(false);
    vaHdr->setRemValid(false);
    vaHdr->setDirectAddrInit(false);
    vaHdr->setRedirectAddrInit(false);
}

void VehicleAgent::resendCAInitialization(cMessage *msg)
{

}

void VehicleAgent::resendCASequenceUpdate(cMessage *msg)
{

}

void VehicleAgent::resendCASessionRequest(cMessage *msg)
{

}

void VehicleAgent::resendCALocationUpdate(cMessage *msg)
{

}
} //namespace
