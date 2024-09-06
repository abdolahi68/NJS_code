 
package org.mixim.examples.Mac80211;

import org.mixim.base.modules.BaseNetwork;
import org.mixim.modules.node.Host80211;
//
//This example shows how to use MiXiMs 802.11 protocol implementation. 
 
network Mac80211ExampleNet extends BaseNetwork
{
    parameters:
        int numHosts; // total number of hosts in the network  

    submodules:
        host[numHosts]: Host80211 {
            @display("p=137,95");
        }

    connections allowunconnected:
    // all connections and gates are to be generated dynamically
}
