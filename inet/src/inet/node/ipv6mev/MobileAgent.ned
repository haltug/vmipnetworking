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

import inet.applications.contract.IPingApp;
import inet.applications.contract.ISCTPApp;
import inet.applications.contract.ITCPApp;
import inet.applications.contract.ITunApp;
import inet.applications.contract.IUDPApp;
import inet.networklayer.icmpv6.ICMPv6;
import inet.networklayer.ipv6mev.MobileAgent;
import inet.networklayer.ipv6.IPv6ErrorHandling;
import inet.node.inet.NodeBaseIdModification;
import inet.transportlayer.contract.ISCTP;
import inet.transportlayer.contract.ITCP;
import inet.transportlayer.contract.IUDP;

//
// TODO auto-generated type
//
module MobileAgent extends NodeBaseIdModification
{
    parameters:
        //        @display("i=device/"); // 
        int numTcpApps = default(0);  // no of TCP apps. Specify the app types in INI file with tcpApp[0..1].typename="TCPEchoApp" syntax
        int numUdpApps = default(0);  // no of UDP apps. Specify the app types in INI file with udpApp[0..1].typename="UDPVideoStreamCli" syntax
        int numSctpApps = default(0); // no of SCTP apps. Specify the app types in INI file with sctpApp[0..1].typename="SCTPServer" syntax
        int numPingApps = default(0);  // no of PING apps. Specify the app types in INI file with pingApp[0..1].typename="PingApp" syntax
        bool hasTcp = default(numTcpApps > 0);
        bool hasUdp = default(numUdpApps > 0);
        bool hasSctp = default(numSctpApps > 0);
        bool hasTun = default(numTunInterfaces > 0);
        string tcpType = default(firstAvailableOrEmpty("TCP", "TCP_lwIP", "TCP_NSC"));  // tcp implementation (e.g. ~TCP, ~TCP_lwIP, ~TCP_NSC) or ~TCPSpoof
        string udpType = default(firstAvailableOrEmpty("UDP"));
        string sctpType = default(firstAvailableOrEmpty("SCTP"));
        forwarding = default(false);  // disable routing by default
        xml routes = default(xml("<routingTable/>"));
        networkLayerType = "IPv6NetworkLayerIdModification";
        routingTableType = "IPv6RoutingTable";
        routingTable.routes = routes;
        networkLayer.proxyARP = default(false);
        *.icmpv6Module = default(absPath(".icmpv6"));
        @display("i=old/penguin;bgb=,-");
    submodules:
        tunApp[numTunInterfaces]: <> like ITunApp {
            parameters:
                @display("p=147,258");
        }
        tcpApp[numTcpApps]: <> like ITCPApp {
            parameters:
                @display("p=147,54,row,60");
        }
        tcp: <tcpType> like ITCP if hasTcp {
            parameters:
                @display("p=147,141");
        }
        udpApp[numUdpApps]: <> like IUDPApp {
            parameters:
                @display("p=495,54,row,60");
        }
        udp: <udpType> like IUDP if hasUdp {
            parameters:
                @display("p=495,141");
        }
        sctpApp[numSctpApps]: <> like ISCTPApp {
            parameters:
                @display("p=629,141,row,60");
        }
        sctp: <sctpType> like ISCTP if hasSctp {
            @display("p=629,232");
        }
        idLayer: MobileAgent {
            parameters:
                @display("p=329,208");
        }
        icmpv6: ICMPv6 {
            parameters:
                @display("p=329,142");
        }
        ipv6ErrorHandling: IPv6ErrorHandling {
            parameters:
                @display("p=239,111");
        }
        pingApp[numPingApps]: <default("PingApp")> like IPingApp {
            parameters:
                @display("p=329,54,row,60");
        }
    connections allowunconnected:
        idLayer.fromLowerLayer <-- networkLayer.transportOut++ if idLayer.isIdLayerEnabled;
        idLayer.toLowerLayer --> networkLayer.transportIn++ if idLayer.isIdLayerEnabled;

        idLayer.icmpIpIn <-- networkLayer.transportOut++;
        idLayer.icmpIpOut --> networkLayer.transportIn++;
        idLayer.tcpIpIn <-- networkLayer.transportOut++ if !idLayer.isIdLayerEnabled;
        idLayer.tcpIpOut --> networkLayer.transportIn++ if !idLayer.isIdLayerEnabled;
        idLayer.udpIpIn <-- networkLayer.transportOut++ if !idLayer.isIdLayerEnabled;
        idLayer.udpIpOut --> networkLayer.transportIn++ if !idLayer.isIdLayerEnabled;

        for i=0..numTcpApps-1 {
            tcpApp[i].tcpOut --> tcp.appIn++;
            tcpApp[i].tcpIn <-- tcp.appOut++;
        }

        tcp.ipOut --> idLayer.fromTCP if hasTcp;
        tcp.ipIn <-- idLayer.toTCP if hasTcp;

        for i=0..numUdpApps-1 {
            udpApp[i].udpOut --> udp.appIn++;
            udpApp[i].udpIn <-- udp.appOut++;
        }

        udp.ipOut --> idLayer.fromUDP if hasUdp;
        udp.ipIn <-- idLayer.toUDP if hasUdp;

        icmpv6.ipv6Out --> idLayer.fromICMP;
        icmpv6.ipv6In <-- idLayer.toICMP;

        for i=0..numSctpApps-1 {
            sctpApp[i].sctpOut --> sctp.from_appl++;
            sctp.to_appl++ --> sctpApp[i].sctpIn;
        }
        sctp.to_ip --> networkLayer.transportIn++ if hasSctp;
        networkLayer.transportOut++ --> sctp.from_ip if hasSctp;

        for i=0..numTunInterfaces-1 {
            tunApp[i].tunOut --> tun[i].appIn if hasTun;
            tunApp[i].tunIn <-- tun[i].appOut if hasTun;
        }

        // ICMPv6 to IPv6ErrorHandling
        icmpv6.errorOut --> ipv6ErrorHandling.in;

        for i=0..numPingApps-1 {
            icmpv6.pingOut++ --> pingApp[i].pingIn;
            icmpv6.pingIn++ <-- pingApp[i].pingOut;
        }
}
