<h1 align="center">
    <a href="https://openairinterface.org/"><img src="https://openairinterface.org/wp-content/uploads/2015/06/cropped-oai_final_logo.png" alt="OAI" width="550"></a>
</h1>

<p align="center">
    <a href="https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/master/LICENSE"><img src="https://img.shields.io/badge/license-OAI--Public--V1.1-blue" alt="License"></a>
    <a href="https://releases.ubuntu.com/18.04/"><img src="https://img.shields.io/badge/OS-Ubuntu18-Green" alt="Supported OS"></a>
    <a href="https://www.redhat.com/en/enterprise-linux-8"><img src="https://img.shields.io/badge/OS-RHEL8-Green" alt="Supported OS"></a>
</p>

<p align="center">
    <a href="https://jenkins-oai.eurecom.fr/job/RAN-Container-Parent/"><img src="https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fjenkins-oai.eurecom.fr%2Fjob%2FRAN-Container-Parent%2F&label=build%20Images"></a>
</p>

<p align="center">
  <a href="https://hub.docker.com/r/rdefosseoai/oai-enb"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/rdefosseoai/oai-enb?label=eNB%20docker%20pulls"></a>
  <a href="https://hub.docker.com/r/rdefosseoai/oai-lte-ue"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/rdefosseoai/oai-lte-ue?label=LTE-UE%20docker%20pulls"></a>
  <a href="https://hub.docker.com/r/rdefosseoai/oai-gnb"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/rdefosseoai/oai-gnb?label=gNB%20docker%20pulls"></a>
  <a href="https://hub.docker.com/r/rdefosseoai/oai-nr-ue"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/rdefosseoai/oai-nr-ue?label=NR-UE%20docker%20pulls"></a>
</p>

# OpenAirInterface License #

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
