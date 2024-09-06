# NJS_code
 Number of Jammed Slots Metric

This code is related to the following papers:
- Abdollahi, M., Malekinasab, K., Tu, W., & Bag-Mohammadi, M. (2023). Physical-layer Jammer Detection in Multi-hop IoT Networks. IEEE Internet of Things Journal.
- Abdollahi, M., Malekinasab, K., Tu, W., & Bag-Mohammadi, M. (2021, October). An efficient metric for physical-layer jammer detection in Internet of Things networks. In 2021 IEEE 46th Conference on Local Computer Networks (LCN) (pp. 209-216). IEEE.

How to Run:

- MIXIM framework is compatible with omnet++ 4.3.

- edit the following paths related to positions of wireless nodes (I have attached the Positions.txt file ):

 ---  \mixim-2.3\src\base\modules\TestApplLayer.cc
 
---  \mixim-2.3\src\inet_stub\mobility\models\MobilityBase.cc

Also, for changing the type of jammer (reactive/proactive) change the JamLoc/omnetpp.ini


The presence of a jammer in an IoT network severely degrades all communication efforts between adjacent wireless devices. The situation is getting worse due to retransmission attempts made by affected devices. Therefore, jammers must be detected or localized quickly to activate a series of corrective countermeasures so as to ensure the robust operation of the IoT network. This paper proposes a novel metric called the number of jammed slots (NJS). It can detect and localize both reactive and proactive jammers that follow arbitrary jamming attack patterns. NJS is applicable to all communication paradigms such as unicast, broadcast, and multicast. In NJS, the wireless medium status is monitored by IoT devices and summarized reports are sent to a central node. Then, the central node determines the jamming duration, the affected nodes, and the approximate location of the jammer(s).




