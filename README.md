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




