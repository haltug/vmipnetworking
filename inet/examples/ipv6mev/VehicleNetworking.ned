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

package inet.examples.ipv6mev;

import inet.linklayer.ethernet.EtherHub;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.networklayer.configurator.ipv6.FlatNetworkConfigurator6;
import inet.node.ipv6.*;
import inet.node.wireless.AccessPoint;

channel ethernetline extends ned.DatarateChannel
{
    parameters:
        delay = 0.1us;
        datarate = 100Mbps;
}

network vehicleNetworking
{
    parameters:
        double total_mn;
        double total_cn;
        //	    **.mgmt.numChannels = 5;
        @display("bgb=1176,812;bgi=background/streetmap,s");

    submodules:
        configurator: FlatNetworkConfigurator6 {
            parameters:
                @display("p=48,20");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=48,75");
        }
        R_1: Router6 {
            parameters:
                @display("p=230,230");
        }
//        R_2: Router6 {
//            parameters:
//                @display("p=630,230");
//        }
//        R_3: Router6 {
//            parameters:
//                @display("p=230,570");
//        }
//        R_4: Router6 {
//            parameters:
//                @display("p=630,570");
//        }
        R_5: Router6 {
            parameters:
                @display("p=430,30");
        }
        AP_1: AccessPoint {
            parameters:
                @display("p=230,300;i=device/accesspoint_s");
        }
//        AP_2: AccessPoint {
//            parameters:
//                @display("p=630,300;i=device/accesspoint_s");
//        }
//        AP_3: AccessPoint {
//        	parameters:
//        	    @display("p=430,500;i=device/accesspoint_s");
//        }
//        AP_4: AccessPoint {
//        	parameters:
//        	    @display("p=830,500;i=device/accesspoint_s");
//        }
        HUB_1: EtherHub {
            parameters:
                @display("p=730,30;i=device/hub_s");
        }
        CN[total_cn]: StandardHost6 {
            parameters:
                @display("p=1030,30");
        }
        MN[total_mn]: StandardHost6 {
            parameters:
                @display("p=100,400;i=device/wifilaptop");
                numRadios = default(1);
        }
    connections allowunconnected:
        R_1.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_1.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
//        R_2.ethg$o++ --> ethernetline --> R_5.ethg$i++;
//        R_2.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
//        R_3.ethg$o++ --> ethernetline --> R_5.ethg$i++;
//        R_3.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
//        R_4.ethg$o++ --> ethernetline --> R_5.ethg$i++;
//        R_4.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        AP_1.ethg$o++ --> ethernetline --> R_1.ethg$i++;
        AP_1.ethg$i++ <-- ethernetline <-- R_1.ethg$o++;
//        AP_2.ethg$o++ --> ethernetline --> R_2.ethg$i++;
//        AP_2.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
//        AP_3.ethg$o++ --> ethernetline --> R_3.ethg$i++;
//        AP_3.ethg$i++ <-- ethernetline <-- R_3.ethg$o++;
//        AP_4.ethg$o++ --> ethernetline --> R_4.ethg$i++;
//        AP_4.ethg$i++ <-- ethernetline <-- R_4.ethg$o++;
        HUB_1.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        HUB_1.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        for i=0..total_cn-1 {
            CN[i].ethg$o++ --> ethernetline --> HUB_1.ethg$i++;
            CN[i].ethg$i++ <-- ethernetline <-- HUB_1.ethg$o++;
        }
}
