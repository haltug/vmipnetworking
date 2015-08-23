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

package inet.networklayer.ipv6mev;

//
// TODO auto-generated module
//
simple MobileAgent
{
    parameters:
        @display("i=block/join");
        bool isIdLayerEnabled = default(true);
        string interfaceTableModule;
        string controlAgentAddress = default("");  // server address (may be symbolic)
//        double startTime @unit(s) = default(5s);
        *.interfaceTableModule = default(absPath(interfaceTableModule));
	gates:
	    input fromLowerLayer @loose; // connection to network layer module
	    output toLowerLayer @loose;
	    input fromTCP; // connection to tcp module
	    output toTCP;
	    input fromUDP; // connection to udp module
	    output toUDP;
        input icmpIn; // connection to icmp module
        output icmpOut;
        input icmpIpIn; // used for direct message exchange between icmp module and ipv6 module,
        output icmpIpOut; // because ipv6 modules has defined gates for icmp messages
        input tcpIpIn @loose; // used only for disabled id layer
        output tcpIpOut @loose; // used only for disabled id layer
        input udpIpIn @loose; // used only for disabled id layer
        output udpIpOut @loose; // used only for disabled id layer
}
