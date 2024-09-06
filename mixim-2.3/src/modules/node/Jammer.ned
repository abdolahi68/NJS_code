

package org.mixim.modules.node;

import org.mixim.modules.nic.IWirelessNicUpperCtrl;
import org.mixim.base.modules.IBaseArp;
import org.mixim.base.modules.NetwJammer;

module Jammer
{
    parameters:
        string networkType = default("NetwJammer");       //type of the network layer
        string mobilityType = default("StationaryMobility"); //type of the mobility module
        string arpType = default("BaseArp");                 //type of address resolution module
        string nicType = "NicJammer";
        @display("bgb=188,266,white;i=device/accesspoint_l");
        @node();
        
    gates:
        input radioIn; // gate for sendDirect
    types:
    submodules:
        arp: <arpType> like IBaseArp {
                @display("p=140,100;i=block/network2");
        }
        mobility: <mobilityType> like inet.mobility.IMobility {
            parameters:
                @display("p=140,30;i=block/cogwheel");
        }
        nic: <nicType> like IWirelessNicUpperCtrl {
            parameters:
                @display("p=72,190;i=block/wrxtx,#008000");
        }
        netwJammer: NetwJammer {
                @display("p=72,87");
        }
    connections:
        radioIn --> nic.radioIn;
        nic.upperLayerOut --> netwJammer.lowerLayerIn;
        nic.upperLayerIn <-- netwJammer.lowerLayerOut;
        nic.upperControlOut --> { @display("ls=red;m=m,70,0,70,0"); } --> netwJammer.lowerControlIn;
        nic.upperControlIn <-- { @display("ls=red;m=m,70,0,70,0"); } <-- netwJammer.lowerControlOut;
        
        
}


