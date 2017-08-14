This utility allows you to inject packets into the Openair4G stack (UE for the moment) or generate packets 
from different parts of the stack.

To compile:

   make pt

To get help

   ./pt --help

Some pregenerated messages (RRC and MAC) can be found in the PACKET_TRACER/messages directory.  Here are some 
usage examples: 

To generate RRC messages (e.g. SIBx)

./pt --eNB_RRC --RRCSIB2_3

./pt --eNB_RRC --RRCConnectionReconfiguration

./pt --UE_RRC --RRCConnectionRequest


To inject packets into the OpenAir4G stack  (This works for UE receiver only for now, with control-plane messasges)

./pt --eNB_MAC -I messages/Msg4.txt

./pt -I messages/rrcconnectionsetup.txt -J messages/rrcconnectionreconfiguration.txt --eNB_RRC --RRCConnectionReconfiguration

./pt -I messages/rrcconnectionsetup.txt --eNB_RRC --RRCConnectionSetup


To be done:

 - user-plane tests, in particular out-of-order segments to (MAC->RLC)
