**Table of Contents**

1. [OpenAirInterface eNB Feature Set](#openairinterface-enb-feature-set)
   1. [eNB PHY Layer](#enb-phy-layer)
   2. [eNB MAC Layer](#enb-mac-layer)
   3. [eNB RLC Layer](#enb-rlc-layer)
   4. [eNB PDCP Layer](#enb-pdcp-layer)
   5. [eNB RRC Layer](#enb-rrc-layer)
   6. [eNB X2AP](#enb-x2ap)
   7. [eNB Advanced Features](#enb-advanced-features)
2. [OpenAirInterface UE Feature Set](#openairinterface-ue-feature-set)

# OpenAirInterface Block diagram #

![Block Diagram](./oai_enb_block_diagram.png)

# OpenAirInterface eNB Feature Set #

## eNB PHY Layer ##

The Physical layer implements **3GPP 36.211**, **36.212**, **36.213** and provides the following features:

- LTE release 8.6 compliant, and implements a subset of release 10
- FDD and TDD configurations: 1 (experimental) and 3
- Bandwidth: 5, 10, and 20 MHz
- Transmission modes: 1, 2 (stable), 3, 4, 5, 6, 7 (experimental)
- Max number of antennas: 2
- CQI/PMI reporting: aperiodic, feedback mode 3 - 0 and 3 - 1
- PRACH preamble format 0
- All downlink (DL) channels are supported: PSS, SSS, PBCH, PCFICH, PHICH, PDCCH, PDSCH, PMCH
- All uplink (UL) channels are supported: PRACH, PUSCH, PUCCH (format 1/1a/1b), SRS, DRS
- HARQ support (UL and DL)
- Highly optimized base band processing (including turbo decoder)

### Performances ###

**Transmission Mode, Bandwidth** | **Expected Throughput** | **Conditions**
-------------------------------- | ----------------------- | ----------------:
FDD DL: 5 MHz, 25 PRBS/ MCS 28   | 16 - 17 Mbit/s          | measured with COTS UE Cat 3/4
FDD DL: 10 MHz, 50 PRBS/ MCS 28  | 34 - 35 Mbit/s          | measured with COTS UE Cat 3/4
FDD DL: 20 MHz, 100 PRBS/ MCS 28 | 70 Mbit/s               | measured with COTS UE Cat 3/4
 |  | 
FDD UL: 5 MHz, 25 PRBS/ MCS 20   | 9 Mbit/s                | measured with COTS UE Cat 3/4
FDD UL: 10 MHz, 50 PRBS/ MCS 20  | 17 Mbit/s               | measured with COTS UE Cat 3/4
FDD UL: 20 MHz, 100 PRBS/ MCS 20 | 35 Mbit/s               | measured with COTS UE Cat 3/4
 |  | 
TDD DL: 5 MHz, 25 PRBS/ MCS **XX**   | **TBC** Mbit/s | 
TDD DL: 10 MHz, 50 PRBS/ MCS **XX**  | **TBC** Mbit/s | 
TDD DL: 20 MHz, 100 PRBS/ MCS **XX** | **TBC** Mbit/s | 
 |  | 
TDD UL: 5 MHz, 25 PRBS/ MCS **XX**   | **TBC** Mbit/s | 
TDD UL: 10 MHz, 50 PRBS/ MCS **XX**  | **TBC** Mbit/s | 
TDD UL: 20 MHz, 100 PRBS/ MCS **XX** | **TBC** Mbit/s | 

- Number of supported UEs: **To Be Completed**

## eNB MAC Layer ##

The MAC layer implements a subset of the **3GPP 36.321** release v8.6 in support of BCH, DLSCH, RACH, and ULSCH channels. 

- RRC interface for CCCH, DCCH, and DTCH
- Proportional fair scheduler (round robin scheduler soon)
- DCI generation
- HARQ Support
- RA procedures and RNTI management
- RLC interface (AM, UM)
- UL power control
- Link adaptation

## eNB RLC Layer ##

The RLC layer implements a full specification of the 3GPP 36.322 release v9.3.

- RLC TM (mainly used for BCCH and CCCH) 
  * Neither segment nor concatenate RLC SDUs
  * Do not include a RLC header in the RLC PDU
  * Delivery of received RLC PDUs to upper layers
- RLC UM (mainly used for DTCH) 
  * Segment or concatenate RLC SDUs according to the TB size selected by MAC
  * Include a RLC header in the RLC PDU
  * Duplication detection
  * PDU reordering and reassembly
- RLC AM, compatible with 9.3 
  * Segmentation, re-segmentation, concatenation, and reassembly
  * Padding
  * Data transfer to the user
  * RLC PDU retransmission in support of error control and correction
  * Generation of data/control PDUs

## eNB PDCP Layer ##

The current PDCP layer is header compliant with **3GPP 36.323** Rel 10.1.0 and implements the following functions:

- User and control data transfer
- Sequence number management
- RB association with PDCP entity
- PDCP entity association with one or two RLC entities
- Integrity check and encryption using the AES and Snow3G algorithms

## eNB RRC Layer ##

The RRC layer is based on **3GPP 36.331** v14.3.0 and implements the following functions:

- System Information broadcast (SIB 1, 2, 3, and 13)
  * SIB1: Up to 6 PLMN IDs broadcast
- RRC connection establishment
- RRC connection reconfiguration (addition and removal of radio bearers, connection release)
- RRC connection release
- RRC connection re-establishment
- Inter-frequency measurement collection and reporting (experimental)
- eMBMS for multicast and broadcast (experimental)
- Handover (experimental)
- Paging (soon)

## eNB X2AP ##

**To be completed**

## eNB Advanced Features ##

**To be completed**

# OpenAirInterface UE Feature Set #

**To be completed**

