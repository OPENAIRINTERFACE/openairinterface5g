OpenAirInterface is under OpenAirInterface Software Alliance license.
├── http://www.openairinterface.org/?page_id=101
├── http://www.openairinterface.org/?page_id=698

It is distributed under OAI Public License V1.0. 
The license information is distributed under LICENSE file in the same directory.
Please see NOTICE.txt for third party software that is included in the sources.

The OpenAirInterface (OAI) software is composed of the following parts: 

openairinterface5g
├── cmake_targets: build utilities to compile (simulation, emulation and real-time platforms), and generated build files
├── common : some common OAI utilities, other tools can be found at openair2/UTILS
├── LICENSE
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


RELEASE NOTES:

v0.1 -> Last stable commit on develop branch before enhancement-10-harmony
v0.2 -> Merge of enhancement-10-harmony to include NGFI RRH + New Interface for RF/BBU
v0.3 -> Last stable commit on develop branch before the merge of feature-131-new-license. This is the last commit with GPL License
v0.4 -> Merge of feature-131-new-license. It closes issue#131 and changes the license to OAI Public License V1.0
v0.5 -> Merge of enhancement-10-harmony-lts. It includes fixes for Ubuntu 16.04 support
v0.5.1 -> Merge of bugfix-137-uplink-fixes. It includes stablity fixes for eNB
v0.5.2 -> Last version with old code for oaisim (abstraction mode works)
v0.6 -> RRH functionality, UE greatly improved, better TDD support,
        a lot of bugs fixed. WARNING: oaisim in PHY abstraction mode does not
        work, you need to use v0.5.2 for that.
v0.6.1 -> Mostly bugfixes. This is the last version without NFAPI.
