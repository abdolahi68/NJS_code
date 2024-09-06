
package jamloc;



import org.mixim.base.modules.BaseNetwork;
import org.mixim.modules.node.Jammer;
import org.mixim.modules.node.Host80211;

import org.mixim.base.connectionManager.IConnectionManager;
import org.mixim.base.modules.IWorldUtility;

network Jamnet extends BaseNetwork
{
    parameters:
        int numHosts; // total number of hosts in the network  
        int numJammer;



    submodules:
        host[numJammer]: Jammer {
            @display("p=137,95");
        }

        MeshHost[numHosts]: Host80211 {

            @display("p=55,143");
        }


    connections allowunconnected:
        // all connections and gates are to be generated dynamically
}
