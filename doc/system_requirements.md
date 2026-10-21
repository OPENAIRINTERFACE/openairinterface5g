<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# System Requirements for Using OAI Stack

This document describes the minimal and performant system requirements for OpenAirInterface (OAI) 4G/5G software stack (UE and gNB stack). The information provided in this document is based on experimentation, if you have a feedback then open an issue or send an email on the mailing list. 

**Table of Contents**

[[_TOC_]]

## Supported CPU Architecture

|Architecture                               |
|------------------------------------------ |
|x86_64 (Intel, AMD)                        |
|aarch64 (Neoverse-N1, N2 and Grace hopper) |

**Note**: 

- On `x86_64` platform the CPU should support `avx2` instruction set (Minimum requirement).

## Supported Operating System

|Operating System |
|-----------------|
|Ubuntu 22/24/26  |
|RHEL 9           |
|Fedora 44        |
|Debian 11        |
|Rocky 9          |


## Minimum Hardware Requirement for x86_64 Platforms

The minimum hardware requirements depends on the radio unit you would like to use or the test case that you would like to execute.  

### Simulated Radio

OAI offers an inbuilt simulated radio, [RFSimulator](../radio/rfsimulator/README.md). It can be used to familiarize oneself with OAI, for development and debugging, and offers the possibility to use basic channel models. It is not designed to do high performance testing. The below requirements are valid for both 4G and 5G RAN and UE Stack. 

The following requirements are minimum requirements for all use cases:

#### Minimum requirements for gNB Stack

- CPU: 2
- Minimum frequency > 2GHz
- Memory: 4Gi
- `avx2` instruction set
- `sctp` protocol should be enabled

**NOTE**: We have not tested on Intel Atom or Celeron processors, and they
likely won't work well.

#### Minimum requirements for UE Stack

- CPU: 2
- Minimum frequency > 2GHz
- Memory: 3Gi
- `avx2` instruction set

**NOTE**: We have not tested on Intel Atom or Celeron processors 

### USRP B2XX or Blade RF

USRP B2XX or Blade RF are USB based radios recommended to use with USB 3.0. You can choose a minimum hardware to do functional testing and performance hardware for performance testing. This hardware you can find in Mini-PCs or laptops.

The minimum requirements stated in [simulated radio](#simulated-radio) apply.

#### Minimum requirements for both gNB and UE Stack

- CPU: 3
- Memory: 5Gi
- Disable CPU sleep states
- USB 3.0
- Intel i5, AMD Ryzen 5

#### Recommended for performance for gNB and UE Stack

- CPU: 4
- Boost Frequency > 3GHz 
- Use only performance cores
- Disable hyper-threading
- Disable CPU sleep states
- DDR4 or DDR5 RAM, minimum 5Gi
- USB 3.0

Apart from this you should follow [tuning and security tips](./tuning_and_security.md) to 
tune your system to get high performance. 

### USRP N3XX/X3XX/X4XX/AW2S

USRP N3XX/X3XX/X4XX requires two dedicated 10G SFP+ connections. For these radios we only recommend having performance hardware. This hardware you can find in Desktop servers or rack/blade servers. For the gNB, the same applies in case of using AW2S radios.

The minimum requirements stated in [simulated radio](#simulated-radio) apply.

Apart from this you should follow [tuning and security tips](./tuning_and_security.md) to 
tune your system to get high performance. 

**NOTE**: In case you are using Mellanox NIC cards then you have to download `mlnx-ofed` and configure your NIC for performance. 

#### Recommended for performance for gNB Stack

- CPU: 8-10
- Boost Frequency > 4GHz
- Use only performance cores
- Disable CPU sleep states
- DDR4 or DDR5 RAM, 5 Gi
- (Optional) Realtime kernel to have better Jitter statistics
- Intel x710/xx710/E-810 or Mellanox connect 5x or 6x

#### Recommended for performance for UE Stack

- CPU: 4
- Boost Frequency > 4GHz 
- DDR4 or DDR5 RAM, 5Gi
- Use only performance cores
- Disable CPU sleep states
- (Optional) Realtime kernel to have better Jitter statistics
- Intel x710/xx710/E-810 or Mellanox connect 5x or 6x

### O-RAN Radio Units

We have dedicated documentation for O-RAN Radio Units. [Refer to 7.2 FH documentation](./ORAN_FHI7.2_Tutorial.md) before purchasing a Desktop server or rack/blade server. 

The minimum requirements stated in [simulated radio](#simulated-radio) apply.
