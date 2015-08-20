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

import inet.networklayer.ipv6mev.DataAgent;
import inet.networklayer.icmpv6.ICMPv6;
import inet.networklayer.ipv6.IPv6ErrorHandling;
import inet.node.inet.NodeBaseIdModification;
import inet.applications.contract.IPingApp;
//
// TODO auto-generated type
//
network DataAgent extends NodeBaseIdModification
{
    parameters:
        @display("i=abstract/switch");
        xml routes = default(xml("<routingTable/>"));
        networkLayerType = "IPv6NetworkLayerIdModification";
        routingTableType = "IPv6RoutingTable";
        routingTable.routes = routes;
        forwarding = false;
		*.icmpv6Module = default(absPath(".icmpv6"));
    submodules:
        idLayer: DataAgent {
            parameters:
                @display("p=280,219");
        }
        icmpv6: ICMPv6 {
            parameters:
                @display("p=280,142");
        }
        ipv6ErrorHandling: IPv6ErrorHandling {
            parameters:
                @display("p=176,142");
        }
    connections allowunconnected:
        idLayer.toLowerLayer --> networkLayer.transportIn++;
        idLayer.fromLowerLayer <-- networkLayer.transportOut++;
        idLayer.udpOut --> networkLayer.transportIn++;
        idLayer.udpIn <-- networkLayer.transportOut++;
        idLayer.tcpOut --> networkLayer.transportIn++;
        idLayer.tcpIn <-- networkLayer.transportOut++;
        idLayer.icmpIpOut --> networkLayer.icmpIpIn;
        idLayer.icmpIpIn <-- networkLayer.icmpIpOut;
        
        idLayer.icmpOut --> icmpv6.ipv6In;
        idLayer.icmpIn <-- icmpv6.ipv6Out; 
        icmpv6.errorOut --> ipv6ErrorHandling.in;
}
