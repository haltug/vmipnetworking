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
simple ControlAgent
{
    parameters:
        @display("i=block/join");
        string dataAgents = default("");
        int indexOperator = default(0);
        string interfaceTableModule;
        *.interfaceTableModule = default(absPath(interfaceTableModule));
		@statistic[numDataAgents](title="numDataAgents"; record=last,vector; interpolationmode=none);
        @statistic[numMobileAgents](title="numMobileAgents"; record=last,vector; interpolationmode=none);		
        @statistic[numFlowRequests](title="numFlowRequests"; record=last,vector; interpolationmode=none);
        @statistic[numSequenceUpdate](title="numSequenceUpdate"; record=last,vector; interpolationmode=none);
        @statistic[numSequenceResponse](title="numSequenceResponse"; record=last,vector; interpolationmode=none);
        @statistic[txTraffic](title="txTraffic"; record=count,vector; interpolationmode=none);
        @statistic[rxTraffic](title="rxTraffic"; record=count,vector; interpolationmode=none);
	gates:
	    input fromLowerLayer;
	    output toLowerLayer;
}
