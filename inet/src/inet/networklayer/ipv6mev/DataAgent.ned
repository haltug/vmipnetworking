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
simple DataAgent
{
    parameters:
        @display("i=block/join");
        string interfaceTableModule;
        *.interfaceTableModule = default(absPath(interfaceTableModule));
        @statistic[numMobileAgents](title="numMobileAgents"; record=last,vector; interpolationmode=none);
        @statistic[numFlows](title="numFlows"; record=last,vector; interpolationmode=none);
        @statistic[incomingTrafficPktNode](title="incomingTrafficPktNode"; record=last,vector,histogram; interpolationmode=none);
        @statistic[outgoingTrafficPktNode](title="outgoingTrafficPktNode"; record=last,vector,histogram; interpolationmode=none);
		@statistic[incomingTrafficPktAgent](title="incomingTrafficPktAgent"; record=last,vector,histogram; interpolationmode=none);
        @statistic[outgoingTrafficPktAgent](title="outgoingTrafficPktAgent"; record=last,vector,histogram; interpolationmode=none);
        @statistic[incomingTrafficSizeNode](title="incomingTrafficSizeNode"; record=count,vector,histogram; interpolationmode=none);
        @statistic[outgoingTrafficSizeNode](title="outgoingTrafficSizeNode"; record=count,vector,histogram; interpolationmode=none);
        @statistic[incomingTrafficSizeAgent](title="incomingTrafficSizeAgent"; record=count,vector,histogram; interpolationmode=none);
        @statistic[outgoingTrafficSizeAgent](title="outgoingTrafficSizeAgent"; record=count,vector,histogram; interpolationmode=none);
	gates:
	    input fromLowerLayer;
	    output toLowerLayer;
	    input udpIn;
	    output udpOut;
        input tcpIn;
        output tcpOut;
        input icmpIn; 
        output icmpOut;
        input icmpIpIn;
        output icmpIpOut;
}
