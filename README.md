# VLAN_changer

This application performs the following task:

1) Capture a network packet with two vlan tags (QinQ) from the incoming interface.
2) In accordance with the rule received from the database, delete one tag and change the second. After that, forward the packet to the outgoing interface.

The libraries Pcap++ and DPDK are used.

There is an unresolved issue with processor cache coherency and data loss. A possible solution is to isolate the cores.
