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

package veins.nodes;

import veins.base.modules.*;
import veins.modules.nic.Nic80211p;
import inet.common.lifecycle.NodeStatus;
import inet.mobility.contract.IMobility;
//import inet.node.ipv6.*;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.linklayer.contract.IWirelessNic;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.configurator.ipv6.FlatNetworkConfigurator6;

module BasicCar
{
    parameters:
        @node();
        @labels(wireless-node);
        @display("i=device/adam");
        string veinsmobilityType;  //type of the mobility module
        string mobilityType;       // INET mobility
        int numRadios = default(0);
        bool hasStatus = default(false);
        string nicType = default("Nic80211p"); // type of network interface card
        xml routes = default(xml("<routingTable/>"));
        string networkLayerType = default("IPv6NetworkLayer");
        string routingTableType = default("IPv6RoutingTable");
        routingTable.forwarding = default(false);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");
        networkLayer.proxyARP = default(false);
        routingTable.routes = routes;
    gates:
        input radioIn[numRadios] @directIn; // TODO: LTE input gate;  
    submodules:
        veinsmobility: <veinsmobilityType> like veins.base.modules.IMobility {
            parameters:
                @display("p=50,40;i=block/cogwheel");
        }
        interfaceTable: InterfaceTable {
            @display("p=50,280;is=s");
        }
        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
            parameters:
                @display("p=50,200;is=s");
        }
        status: NodeStatus if hasStatus {
            @display("p=50,360");
        }
        nic[numRadios]: <nicType> like veins.modules.nic.INic80211p {
            parameters:
                @display("p=200,400");
        }        
        networkLayer: <networkLayerType> like INetworkLayer {
            parameters:
                @display("p=300,250;q=queue");
        }        
//        wlan[numRadios]: <default("Ieee80211Nic")> like IWirelessNic {
//            parameters:
//                @display("p=400,400,row,60;q=queue");
//        }
//		decisionMaker: ExampleDecisionMaker {
//            parameters:
//                @display("p=196,140;i=block/fork");
//        }
        // NOTE: instance must be named "nic", therefore the other nic is called nic80211p
    connections allowunconnected:
		for i=0..sizeof(radioIn)-1 {
            radioIn[i] --> { @display("m=s"); } --> nic[i].radioIn;
            nic[i].upperLayerOut --> networkLayer.ifIn++;
            nic[i].upperLayerIn <-- networkLayer.ifOut++;
        }
//		for i=0..sizeof(radioIn)-1 {
//            radioIn[i] --> { @display("m=s"); } --> wlan[i].radioIn;
//            wlan[i].upperLayerOut --> networkLayer.ifIn++;
//            wlan[i].upperLayerIn <-- networkLayer.ifOut++;
//        }
        
}
