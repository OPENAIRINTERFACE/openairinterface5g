OpenAirInterface is under OpenAirInterface Software Alliance license.
├── http://www.openairinterface.org/?page_id=101
├── http://www.openairinterface.org/?page_id=698

The OpenAirInterface (OAI) software is composed of the following parts: 

openairinterface5g
├── cmake_targets: build utilities to compile (simulation, emulation and real-time platforms), and generated build files
├── common : some common OAI utilities, other tools can be found at openair2/UTILS
├── COPYING
├── maketags : script to generate emacs tags
├── openair1 : 3GPP LTE Rel-10 PHY layer + PHY RF simulation and a subset of Rel 12 Features.
├── openair2 :3GPP LTE Rel-10 RLC/MAC/PDCP/RRC/X2AP implementation. 
    ├── LAYER2/RLC/ with the following subdirectories: UM_v9.3.0, TM_v9.3.0, and AM_v9.3.0. 
    ├── LAYER2/PDCP/PDCP_v10.1.0. 
    ├── RRC/LITE
    ├── PHY_INTERFACE
    ├── X2AP
    ├── ENB_APP 
├── openair3: 3GPP LTE Rel10 for S1AP, NAS GTPV1-U for both ENB and UE.
    ├── GTPV1-U
    ├── NAS 
    ├── S1AP
    ├── SCTP
    ├── SECU
    ├── UDP
└── targets: top level wrapper for unitary simulation for PHY channels, system-level emulation (eNB-UE with and without S1), and realtime eNB and UE and RRH GW.
