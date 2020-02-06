# OpenAirInterface License #

OpenAirInterface is under OpenAirInterface Software Alliance license.

 *  [OAI License Model](http://www.openairinterface.org/?page_id=101)
 *  [OAI License v1.1 on our website](http://www.openairinterface.org/?page_id=698)

It is distributed under **OAI Public License V1.1**.

The license information is distributed under [LICENSE](LICENSE) file in the same directory.

Please see [NOTICE](NOTICE.md) file for third party software that is included in the sources.

# Where to Start #

 *  [How to build](./doc/BUILD.md)
 *  [How to run the modems](./doc/RUNMODEM.md)

# RAN repository structure #

The OpenAirInterface (OAI) software is composed of the following parts: 

<pre>
openairinterface5g
├── ci-scripts: Meta-scripts used by the OSA CI process. Contains also configuration files used day-to-day by CI.
├── cmake_targets: Build utilities to compile (simulation, emulation and real-time platforms), and generated build files
├── common : Some common OAI utilities, other tools can be found at openair2/UTILS
├── doc : Contains an up-to-date feature set list
├── LICENSE
├── maketags : Script to generate emacs tags
├── nfapi : Contains the NFAPI code. A local Readme file provides more details.
├── openair1 : 3GPP LTE Rel-10/12 PHY layer + PHY RF simulation. A local Readme file provides more details.
├── openair2 : 3GPP LTE Rel-10 RLC/MAC/PDCP/RRC/X2AP + LTE Rel-14 M2AP implementation.
    ├── COMMON
    ├── DOCS
    ├── ENB_APP
    ├── LAYER2/RLC/ with the following subdirectories: UM_v9.3.0, TM_v9.3.0, and AM_v9.3.0. 
    ├── LAYER2/PDCP/PDCP_v10.1.0.
    ├── NETWORK_DRIVER
    ├── PHY_INTERFACE
    ├── RRC/LITE
    ├── UTIL
    ├── X2AP
    ├── M2AP
    ├── MCE_APP
├── openair3: 3GPP LTE Rel10 for S1AP, NAS GTPV1-U for both ENB and UE.
    ├── COMMON
    ├── DOCS
    ├── GTPV1-U
    ├── NAS
    ├── S1AP
    ├── M3AP
    ├── SCTP
    ├── SECU
    ├── UDP
    ├── UTILS
└── targets: Top-level wrappers for unitary simulation for PHY channels, system-level emulation (eNB-UE with and without S1), and realtime eNB and UE and RRH GW.
</pre>

