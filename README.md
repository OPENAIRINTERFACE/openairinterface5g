<!-- SPDX-License-Identifier: CC-BY-4.0 -->

<h1 align="center">
    <a href="https://lfnetworking.org/projects/duranta/"><img src="https://raw.githubusercontent.com/duranta-project/governance/main/logos/Duranta-Logo-Color.png" alt="Duranta OAI" width="550"></a>
</h1>

<p align="center">
    <a href="https://github.com/duranta-project/openairinterface5g/releases/tag/v3.0.0"><img src="https://img.shields.io/badge/release-v3.0.0-rebeccapurple?style=flat-square&logo=github" alt="Release v3.0.0"></a>
    <a href="https://github.com/duranta-project/openairinterface5g/blob/develop/LICENSE"><img src="https://img.shields.io/badge/license-CSSL--v1.0-rebeccapurple" alt="License"></a>
</p>

<p align="center">
    <a href="https://releases.ubuntu.com/22.04/"><img src="https://img.shields.io/badge/OS-Ubuntu22-Green" alt="Supported OS Ubuntu 22"></a>
    <a href="https://releases.ubuntu.com/24.04/"><img src="https://img.shields.io/badge/OS-Ubuntu24-Green" alt="Supported OS Ubuntu 24"></a>
    <a href="https://releases.ubuntu.com/26.04/"><img src="https://img.shields.io/badge/OS-Ubuntu26-Green" alt="Supported OS Ubuntu 26"></a>
    <a href="https://www.redhat.com/en/technologies/linux-platforms/enterprise-linux"><img src="https://img.shields.io/badge/OS-RHEL9-Green" alt="Supported OS RHEL9"></a>
    <a href="https://getfedora.org/en/workstation/"><img src="https://img.shields.io/badge/OS-Fedora44-Green" alt="Supported OS Fedora 44"></a>
</p>

<p align="center">
    <a href="https://jenkins-oai.eurecom.fr/job/RAN-Ubuntu-Image-Builder/"><img src="https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fjenkins-oai.eurecom.fr%2Fjob%2FRAN-Ubuntu-Image-Builder%2F&label=build-Ubuntu-x86%20Images"></a>
    <a href="https://jenkins-oai.eurecom.fr/job/RAN-RHEL-Cluster-Image-Builder/"><img src="https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fjenkins-oai.eurecom.fr%2Fjob%2FRAN-RHEL-Cluster-Image-Builder%2F&label=build-RHEL-Cluster%20Images"></a>
    <a href="https://jenkins-oai.eurecom.fr/job/RAN-Ubuntu-ARM-Image-Builder/"><img src="https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fjenkins-oai.eurecom.fr%2Fjob%2FRAN-Ubuntu-ARM-Image-Builder%2F&label=build-Ubuntu-ARM%20Images"></a>
</p>

<p align="center">
  <a href="https://hub.docker.com/r/oaisoftwarealliance/oai-gnb"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/oaisoftwarealliance/oai-gnb?label=gNB%20docker%20pulls"></a>
  <a href="https://hub.docker.com/r/oaisoftwarealliance/oai-nr-ue"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/oaisoftwarealliance/oai-nr-ue?label=NR-UE%20docker%20pulls"></a>
  <a href="https://hub.docker.com/r/oaisoftwarealliance/oai-enb"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/oaisoftwarealliance/oai-enb?label=eNB%20docker%20pulls"></a>
  <a href="https://hub.docker.com/r/oaisoftwarealliance/oai-lte-ue"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/oaisoftwarealliance/oai-lte-ue?label=LTE-UE%20docker%20pulls"></a>
  <a href="https://hub.docker.com/r/oaisoftwarealliance/oai-nr-cuup"><img alt="Docker Pulls" src="https://img.shields.io/docker/pulls/oaisoftwarealliance/oai-nr-cuup?label=NR-CUUP%20docker%20pulls"></a>
</p>

# Duranta - OpenAirInterface

Duranta OpenAirInterface RAN delivers and maintains an open-source cellular
wireless software stack for 4G, 5G and future networking technologies. It
supports simulation, prototyping, and end-to-end deployments on
Commercial-Off-The-Shelf (COTS) hardware. Built for research and
experimentation, it provides standard-compliant interfaces and is released
under the Collaborative Standards Software License (CSSL).

## License

 *  [OAI License Model](http://www.openairinterface.org/?page_id=101)
 *  [CSSL v1.0](http://www.openairinterface.org/?page_id=698)

The source code is distributed under [**CSSL v1.0**](LICENSE).
Some files, such as for orchestration, are distributed under
[MIT license](./LICENSES/preferred/MIT.txt). Documentation is distributed under
[Creative Commons Attribution 4.0 International license](LICENSES/preferred/CC-BY-4.0.txt).

All the files without an explicit copyright header have an implicit "Copyright
of OpenAirInterface Authors".

Please see [NOTICE](NOTICE) for other licenses which are used in the software.

In the past OAI source code has been re-licensed sometimes, here is the
history:

1. CSSL v1.0 starting tag 2026.w14
2. OAI Public License v1.1 starting tag v1.0 till af4b0d53
3. OAI Public License v1.0: starting tag v.04 till v1.0
4. GPL 3: starting tag v.0 till v.04 (only initial implementation of 4G)

## Where to Start

 *  [General overview of documentation](./doc/README.md)
 *  [The implemented features](./doc/FEATURE_SET.md)
 *  [System Requirements for Using OAI Stack](./doc/system_requirements.md)
 *  [How to build](./doc/BUILD.md)
 *  [How to run the modems](./doc/RUNMODEM.md)

Not all information is available in a central place, and information for
specific sub-systems might be available in the corresponding sub-directories.
To find all READMEs, this command might be handy:

```
find . -iname "readme*"
```

## RAN repository structure

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
├── openair1          : Layer 1 (3GPP LTE Rel-10/12 PHY, NR Rel-15 PHY)
├── openair2          : Layer 2 (3GPP LTE Rel-10 MAC/RLC/PDCP/RRC/X2AP, LTE Rel-14 M2AP, NR Rel-15+ MAC/RLC/PDCP/SDAP/RRC/X2AP/F1AP/E1AP), E2AP
├── openair3          : Layer 3 (3GPP LTE Rel-10 S1AP/GTP, NR Rel-15 NGAP/GTP)
├── openshift         : OpenShift helm charts for some deployment options of OAI
├── radio             : Drivers for various radios such as USRP, AW2S, RFsim, 7.2 FHI, ...
├── targets           : Some configuration files; only historical relevance, and might be deleted in the future
└── tools             : Tools for use by the developers/ci machines: code analysis and formatting
```

## How to get support from the Community

You can ask your question on the [mailing lists](https://github.com/duranta-project/openairinterface5g/wiki/MailingList).

Your email should contain below information:

- A clear subject in your email.
- For all the queries there should be [Query\] in the subject of the email and for problems there should be [Problem\].
- In case of a problem, add a small description.
- Do not share any photos unless you want to share a diagram.
- OAI gNB/DU/CU/CU-CP/CU-UP configuration file in `.conf` format only.
- Logs of OAI gNB/DU/CU/CU-CP/CU-UP in `.log` or `.txt` format only.
- In case your question is related to performance, include a small description of the machine (Operating System, Kernel version, CPU, RAM and networking card) and diagram of your testing environment.
- Known/open issues are present on [Github](https://github.com/duranta-project/openairinterface5g/issues), so keep checking.

Always remember a structured email will help us understand your issues quickly.
