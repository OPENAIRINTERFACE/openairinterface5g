<h1 align="center">
    <a href="https://openairinterface.org/"><img src="https://openairinterface.org/wp-content/uploads/2015/06/cropped-oai_final_logo.png" alt="OAI" width="550"></a>
</h1>

<p align="center">
    <a href="https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/master/LICENSE"><img src="https://img.shields.io/badge/license-OAI--Public--V1.1-blue" alt="License"></a>
    <a href="https://releases.ubuntu.com/18.04/"><img src="https://img.shields.io/badge/OS-Ubuntu18-Green" alt="Supported OS Ubuntu 18"></a>
    <a href="https://releases.ubuntu.com/20.04/"><img src="https://img.shields.io/badge/OS-Ubuntu20-Green" alt="Supported OS Ubuntu 20"></a>
    <a href="https://releases.ubuntu.com/22.04/"><img src="https://img.shields.io/badge/OS-Ubuntu22-Green" alt="Supported OS Ubuntu 22"></a>
    <a href="https://www.redhat.com/en/technologies/linux-platforms/enterprise-linux"><img src="https://img.shields.io/badge/OS-RHEL8-Green" alt="Supported OS RHEL8"></a>
    <a href="https://www.redhat.com/en/technologies/linux-platforms/enterprise-linux"><img src="https://img.shields.io/badge/OS-RHEL9-Green" alt="Supported OS RELH9"></a>
    <a href="https://getfedora.org/en/workstation/"><img src="https://img.shields.io/badge/OS-Fedore37-Green" alt="Supported OS Fedora 37"></a>
</p>

<p align="center">
    <a href="https://gitlab.eurecom.fr/oai/openairinterface5g/-/releases"><img alt="GitLab Release (custom instance)" src="https://img.shields.io/gitlab/v/release/oai/openairinterface5g?gitlab_url=https%3A%2F%2Fgitlab.eurecom.fr&include_prereleases&sort=semver"></a>
</p>

<p align="center">
    <a href="https://jenkins-oai.eurecom.fr/job/RAN-Container-Parent/"><img src="https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fjenkins-oai.eurecom.fr%2Fjob%2FRAN-Container-Parent%2F&label=build%20Images"></a>
</p>

<p align="center">
  <a href="https://hub.docker.com/r/oaisoftwarealliance/oai-gnb"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/oaisoftwarealliance/oai-gnb?label=gNB%20docker%20pulls"></a>
  <a href="https://hub.docker.com/r/oaisoftwarealliance/oai-nr-ue"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/oaisoftwarealliance/oai-nr-ue?label=NR-UE%20docker%20pulls"></a>
  <a href="https://hub.docker.com/r/oaisoftwarealliance/oai-enb"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/oaisoftwarealliance/oai-enb?label=eNB%20docker%20pulls"></a>
  <a href="https://hub.docker.com/r/oaisoftwarealliance/oai-lte-ue"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/oaisoftwarealliance/oai-lte-ue?label=LTE-UE%20docker%20pulls"></a>
</p>

# OpenAirInterface License #

 *  [OAI License Model](http://www.openairinterface.org/?page_id=101)
 *  [OAI License v1.1 on our website](http://www.openairinterface.org/?page_id=698)

It is distributed under **OAI Public License V1.1**.

The license information is distributed under [LICENSE](LICENSE) file in the same directory.

Please see [NOTICE](NOTICE.md) file for third party software that is included in the sources.

# Where to Start #

 *  [General overview of documentation](./doc/README.md)
 *  [The implemented features](./doc/FEATURE_SET.md)
 *  [How to build](./doc/BUILD.md)
 *  [How to run the modems](./doc/RUNMODEM.md)

Not all information is available in a central place, and information for
specific sub-systems might be available in the corresponding sub-directories.
To find all READMEs, this command might be handy:

```
find . -iname "readme*"
```

# RAN repository structure #

The OpenAirInterface (OAI) software is composed of the following parts: 

```
openairinterface5g
├── charts
├── ci-scripts        : Meta-scripts used by the OSA CI process. Contains also configuration files used day-to-day by CI.
├── CMakeLists.txt    : Top-level CMakeLists.txt for building
├── cmake_targets     : Build utilities to compile (simulation, emulation and real-time platforms), and generated build files.
├── common            : Some common OAI utilities, some other tools can be found at openair2/UTILS.
├── doc               : Documentation
├── docker            : Dockerfiles to build for Ubuntu and RHEL
├── executables       : Top-level executable source files (gNB, eNB, ...)
├── maketags          : Script to generate emacs tags.
├── nfapi             : (n)FAPI code for MAC-PHY interface
├── openair1          : 3GPP LTE Rel-10/12 PHY layer / 3GPP NR Rel-15 layer. A local Readme file provides more details.
├── openair2          : 3GPP LTE Rel-10 RLC/MAC/PDCP/RRC/X2AP + LTE Rel-14 M2AP implementation. Also 3GPP NR Rel-15 RLC/MAC/PDCP/RRC/X2AP.
├── openair3          : 3GPP LTE Rel10 for S1AP, NAS GTPV1-U for both ENB and UE.
├── openshift         : OpenShift helm charts for some deployment options of OAI
├── radio             : Drivers for various radios such as USRP, AW2S, RFsim, ...
└── targets           : Some configuration files; only historical relevance, and might be deleted in the future
```
