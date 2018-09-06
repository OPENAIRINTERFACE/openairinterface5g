How to configure wireshark for dissecting LTE protocols:
- start the wireshark as a sudoers
- goto analyze->enabled prototols
  => enable mac_lte_udp and rlc_lte_udp
- goto edit/preferences and expand Protocols
- select UDP and check "try heuristic sub-dissectors first"
- select MAC-LTE, and check all the options (checkboxes), and set the "which layer info to show in info column" to "MAC info"
- select RLC-LTE, and check all the options except the "May see RLC headers only", and set the "call PDCP dissector for DRB PDUs" to "12-bit SN". Optionally you may select the sequence analysis for RLC AM/UM.

How to use
- start eNB or UE with option -W (or -P if you want to make a pcap file)
- capture on local interface "lo" 
- filter out the ICMP/DNS/TCP messages (e.g. "!icmp && !dns && !tcp") 
