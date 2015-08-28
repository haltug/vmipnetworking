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

package frankfurt_airport_fair;

import inet.linklayer.ethernet.EtherHub;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.environment.common.PhysicalEnvironment;
import inet.networklayer.configurator.ipv6.FlatNetworkConfigurator6;
import inet.node.ipv6.*;
import inet.node.ipv6mev.*;
import inet.node.wireless.AccessPoint;

import inet.mobility.traci.TraCIScenarioManagerLaunchd;

channel ethernetline extends ned.DatarateChannel
{
    parameters:
        delay = 0.1us;
        datarate = 1000Mbps;
}

network frankfurt_airport_fair
{
    parameters:
        double total_mn;
        double total_cn;
        @display("bgb=6565,9602;bgi=background/frankfurtAirportFair,s");

    submodules:
        environment: PhysicalEnvironment {
            parameters:
                @display("p=50,180");
        }
        manager: TraCIScenarioManagerLaunchd {
            parameters:
                @display("p=50,260");
        }
        configurator: FlatNetworkConfigurator6 {
            parameters:
                @display("p=50,20");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=50,100");
        }
        R_1: Router6 {
            parameters:
                @display("p=140,230");
        }
        R_2: Router6 {
            parameters:
                @display("p=700,230");
        }
        R_3: Router6 {
            parameters:
                @display("p=230,570");
        }
        R_4: Router6 {
            parameters:
                @display("p=757,570");
        }
        R_5: Router6 {
            parameters:
                @display("p=430,30");
        }
        AP_1: AccessPoint {
            parameters:
                @display("p=3816,1960;i=device/accesspoint_s;is=vl");
        }
        //        AP_2: AccessPoint {
        //            parameters:
        //                @display("p=630,300;i=device/accesspoint_s");
        //        }
        AP_3: AccessPoint {
            parameters:
                @display("p=3504,1170;i=device/accesspoint_s;is=vl");
        }
        AP_4: AccessPoint {
            parameters:
                @display("p=4494,1062;i=device/accesspoint_s;is=vl");
        }
        CN[total_cn]: StandardHost6 {
            parameters:
                @display("p=1030,30");
        }
        CA: ControlAgent {
            parameters:
                @display("p=140,30");
        }
        DA: DataAgent {
            parameters:
                @display("p=700,30");
        }
    connections allowunconnected:
        R_1.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_1.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_2.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_2.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_3.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_3.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_4.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_4.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        CA.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        CA.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        DA.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        DA.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        AP_1.ethg$o++ --> ethernetline --> R_1.ethg$i++;
        AP_1.ethg$i++ <-- ethernetline <-- R_1.ethg$o++;
        //        AP_2.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        //        AP_2.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        AP_3.ethg$o++ --> ethernetline --> R_3.ethg$i++;
        AP_3.ethg$i++ <-- ethernetline <-- R_3.ethg$o++;
        AP_4.ethg$o++ --> ethernetline --> R_4.ethg$i++;
        AP_4.ethg$i++ <-- ethernetline <-- R_4.ethg$o++;
        //        HUB_1.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        //        HUB_1.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        for i=0..total_cn-1 {
            CN[i].ethg$o++ --> ethernetline --> R_2.ethg$i++;
            CN[i].ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        }
}
