# Description #
The purpose of this repository is to provide an updated version of the original OAI code base. The original code base can be found here: https://gitlab.eurecom.fr/oai/openairinterface5g. The updates to the OAI code base removed some latent bugs, added multi-UE scalability, and were tested with a standard bypass proxy between the UE(s) and eNB. The proxy is available at: https://github.com/EpiSci/oai-lte-multi-ue-proxy. With this package, various multi-UE scenarios can be tested without the overhead of PHY-layer features of underlying radios. This proxy was created to allow users to test and utilize EpiSci's updated version of the OAI code.
### Where this all began: ###
OAI code was used to baseline this work. The initial framework for this development was started from the follwoing commit: https://gitlab.eurecom.fr/oai/openairinterface5g/-/commit/362da7c9205691a7314de56bbe8ec369f636da7b.

### Included Fixes: ###
- Ease of use of gprof and address sanitizer for debugging purposes
- Updated json files to allow for GDB, real-time debugging capabilities
- Updated logging features to minimally log only key connection milestones. This imroves scalability of multiple UEs.
- Updated logging to include time stamp for timing analysis
- Updated memory allocation procedures to correct size requirements
- Added debugging features to handle signal terminations
- nfapi.c pullarray8 fix invalid pointer math
- Overlapping destination and source memory in memcpy, so updated to memmove to check for this bug
- Advanced error checking mechanisms in critical pack and unpack functions
- Created option for CPU assignment to UE to improve scalability
- Added EPC integration to allow multiple individual UE entities to each have their USIM information parced by the executables
- Updated random value seeds to minimize probability of error in generation of random values
- Enables capability round robin scheduler if desired
- Enables capability real time scheduler if desired
- Added new standalone functions to the UE phy-layer (phy_stub_ue.c) to incorporate individual UE entities
- Updated sending and packing functions in UE (lte_ue.c) to incorporate new standalone changes
- Incorporated semaphores to control timing of incoming downlink packets
- Implemented new queuing system to handle message exchange from UE to eNB and vice versa
- Updated global value in nFAPI for size of subframe
- Updated global value to increase scalability in system


# Where to Start #

 *  [The implemented features](./doc/FEATURE_SET.md)
 *  [How to build](./doc/BUILD.md)
 *  [How to run the modems](./doc/RUNMODEM.md)

# RAN repository structure #

The OpenAirInterface (OAI) software is composed of the following parts: 

<pre>
openairinterface5g
├── ci-scripts        : Meta-scripts used by the OSA CI process. Contains also configuration files used day-to-day by CI.
├── cmake_targets     : Build utilities to compile (simulation, emulation and real-time platforms), and generated build files.
├── common            : Some common OAI utilities, other tools can be found at openair2/UTILS.
├── doc               : Contains an up-to-date feature set list and starting tutorials.
├── executables       : Top-level executable source files.
├── LICENSE           : License file.
├── maketags          : Script to generate emacs tags.
├── nfapi             : Contains the NFAPI code. A local Readme file provides more details.
├── openair1          : 3GPP LTE Rel-10/12 PHY layer / 3GPP NR Rel-15 layer. A local Readme file provides more details.
│   ├── PHY
│   ├── SCHED
│   ├── SCHED_NBIOT
│   ├── SCHED_NR
│   ├── SCHED_NR_UE
│   ├── SCHED_UE
│   └── SIMULATION    : PHY RF simulation.
├── openair2          : 3GPP LTE Rel-10 RLC/MAC/PDCP/RRC/X2AP + LTE Rel-14 M2AP implementation. Also 3GPP NR Rel-15 RLC/MAC/PDCP/RRC/X2AP.
│   ├── COMMON
│   ├── DOCS
│   ├── ENB_APP
│   ├── F1AP
│   ├── GNB_APP
│   ├── LAYER2/RLC/   : with the following subdirectories: UM_v9.3.0, TM_v9.3.0, and AM_v9.3.0.
│   ├── LAYER2/PDCP/PDCP_v10.1.0
│   ├── M2AP
│   ├── MCE_APP
│   ├── NETWORK_DRIVER
│   ├── NR_PHY_INTERFACE
│   ├── NR_UE_PHY_INTERFACE
│   ├── PHY_INTERFACE
│   ├── RRC
│   ├── UTIL
│   └── X2AP
├── openair3          : 3GPP LTE Rel10 for S1AP, NAS GTPV1-U for both ENB and UE.
│   ├── COMMON
│   ├── DOCS
│   ├── GTPV1-U
│   ├── M3AP
│   ├── MME_APP
│   ├── NAS
│   ├── S1AP
│   ├── SCTP
│   ├── SECU
│   ├── TEST
│   ├── UDP
│   └── UTILS
└── targets           : Top-level wrappers for unitary simulation for PHY channels, system-level emulation (eNB-UE with and without S1), and realtime eNB and UE and RRH GW.
</pre>
