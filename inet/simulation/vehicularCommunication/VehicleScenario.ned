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
package vehicularCommunication;

import inet.networklayer.configurator.ipv6.FlatNetworkConfigurator6;
import inet.node.ipv6.*;

channel ethernetline extends ned.DatarateChannel
{
    parameters:
        delay = 0.1us;
        datarate = 100Mbps;
}

network VehicleScenario
{
    parameters:
        @display("bgb=2500,2500;");
    submodules:
        configurator: FlatNetworkConfigurator6 {
            parameters:
                @display("p=1050,100");
        }
        R_1: Router6 {
            parameters:
                @display("p=600,600");
        }
        CN: StandardHost6 {
            parameters:
                @display("p=300,300");
        }
    connections allowunconnected:
        CN.ethg$o++ --> ethernetline --> R_1.ethg$i++;
        CN.ethg$i++ <-- ethernetline <-- R_1.ethg$o++;
}
