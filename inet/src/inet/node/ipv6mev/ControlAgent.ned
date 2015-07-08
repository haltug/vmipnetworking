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

package inet.node.ipv6mev;

import inet.networklayer.ipv6mev.Agent;
import inet.node.inet.NodeBase;
//
// TODO auto-generated type
//
network ControlAgent extends NodeBase
{
     parameters:
        @display("i=abstract/router2");
        forwarding = false;
        xml routes = default(xml("<routingTable/>"));
        networkLayerType = "IPv6NetworkLayer";
        routingTableType = "IPv6RoutingTable";
        routingTable.routes = routes;
        networkLayer.proxyARP = default(false);
	submodules:
	    idLayer : Agent {
	    	parameters:
                @display("p=280,208");
                isMA = false;
                isCA = true;
	    }
	connections allowunconnected:
        idLayer.tcpOut --> networkLayer.transportIn++;
        idLayer.tcpIn <-- networkLayer.transportOut++;
        idLayer.udpOut --> networkLayer.transportIn++;
		idLayer.udpIn <-- networkLayer.transportOut++;    

}
