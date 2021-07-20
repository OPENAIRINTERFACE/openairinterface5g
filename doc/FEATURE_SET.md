**Table of Contents**

1. [Functional Split Architecture](#functional-split-architecture)
2. [OpenAirInterface Block Diagram](#openairinterface-block-diagram)
2. [OpenAirInterface 4G-LTE eNB Feature Set](#openairinterface-4g-lte-enb-feature-set)
   1. [eNB PHY Layer](#enb-phy-layer)
   2. [eNB MAC Layer](#enb-mac-layer)
   3. [eNB RLC Layer](#enb-rlc-layer)
   4. [eNB PDCP Layer](#enb-pdcp-layer)
   5. [eNB RRC Layer](#enb-rrc-layer)
   6. [eNB X2AP](#enb-x2ap)
   7. [eNB/MCE M2AP](#enbmce-m2ap)
   8. [MCE/MME M3AP](#mcemme-m3ap)
3. [OpenAirInterface 4G-LTE UE Feature Set](#openairinterface-4g-lte-ue-feature-set)
   1.  [LTE UE PHY Layer](#lte-ue-phy-layer)
   2.  [LTE UE MAC Layer](#lte-ue-mac-layer)
   3.  [LTE UE RLC Layer](#lte-ue-rlc-layer)
   4.  [LTE UE PDCP Layer](#lte-ue-pdcp-layer)
   5.  [LTE UE RRC Layer](#lte-ue-rrc-layer)
4. [OpenAirInterface 5G-NR gNB Feature Set](#openairinterface-5g-nr-feature-set)
   1. [General Parameters](#general-parameters)
   2. [gNB Physical Layer](#gnb-phy-layer)
   3. [gNB Higher Layers](#gnb-higher-layers)
5. [OpenAirInterface 5G-NR UE Feature Set](#openairinterface-5g-nr-ue-feature-set)
   1. [UE Physical Layer](#ue-phy-layer)
   2. [UE Higher Layers](#ue-higher-layers)


# Functional Split Architecture #

-  RCC: Radio-Cloud Center
-  RAU: Radio-Access Unit
-  RRU: Remote Radio-Unit
-  IF4.5 / IF5 : similar to IEEE P1914.1
-  FAPI (IF2)  : specified by Small Cell Forum (open-nFAPI implementation)
-  IF1         : F1 in 3GPP Release 15

![Functional Split Architecture](./oai_enb_func_split_arch.png)


# OpenAirInterface Block Diagram #

![Block Diagram](./oai_enb_block_diagram.png)

# OpenAirInterface 4G LTE eNB Feature Set #

## eNB PHY Layer ##

The Physical layer implements **3GPP 36.211**, **36.212**, **36.213** and provides the following features:

- LTE release 8.6 compliant, and implements a subset of release 10
- FDD and TDD configurations: 1 (experimental) and 3
- Bandwidth: 5, 10, and 20 MHz
- Transmission modes: 1, 2 (stable), 3, 4, 5, 6, 7 (experimental)
- Max number of antennas: 2
- CQI/PMI reporting: aperiodic, feedback mode 3 - 0 and 3 - 1
- PRACH preamble format 0
- Downlink (DL) channels are supported: PSS, SSS, PBCH, PCFICH, PHICH, PDCCH, PDSCH, PMCH, MPDCCH
- Uplink (UL) channels are supported: PRACH, PUSCH, PUCCH (format 1/1a/1b), SRS, DRS
- HARQ support (UL and DL)
- Highly optimized base band processing (including turbo decoder)
- Multi-RRU support: over the air synchro b/ multi RRU in TDD mode
- Support for CE-modeA for LTE-M. Limited support for repeatition, single-LTE-M connection, legacy-LTE UE attach is disabled.

### Performances ###

**Transmission Mode, Bandwidth** | **Expected Throughput** | **Measured Throughput** | **Measurement Conditions**
-------------------------------- | ----------------------- | ------------------------| ----------------:
FDD DL: 5 MHz, 25 PRBS/ MCS 28   | 16 - 17 Mbit/s          | TM1: 17.0 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
FDD DL: 10 MHz, 50 PRBS/ MCS 28  | 34 - 35 Mbit/s          | TM1: 34.0 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
FDD DL: 20 MHz, 100 PRBS/ MCS 28 | 70 Mbit/s               | TM1: 69.9 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
 |  |  | 
FDD UL: 5 MHz, 25 PRBS/ MCS 20   | 9 Mbit/s                | TM1: 8.28 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
FDD UL: 10 MHz, 50 PRBS/ MCS 20  | 17 Mbit/s               | TM1: 18.3 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
FDD UL: 20 MHz, 100 PRBS/ MCS 20 | 35 Mbit/s               | TM1: 18.6 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
 |  | 
TDD DL: 5 MHz, 25 PRBS/ MCS **XX**   | 6.5 Mbit/s          | TM1: 6.71 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
TDD DL: 10 MHz, 50 PRBS/ MCS **XX**  | 13.5 Mbit/s         | TM1: 13.6 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
TDD DL: 20 MHz, 100 PRBS/ MCS **XX** | 28.0 Mbit/s         | TM1: 27.2 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
 |  | | 
TDD UL: 5 MHz, 25 PRBS/ MCS **XX**   | 2.0 Mbit/s          | TM1: 3.31 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
TDD UL: 10 MHz, 50 PRBS/ MCS **XX**  | 2.0 Mbit/s          | TM1: 7.25 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)
TDD UL: 20 MHz, 100 PRBS/ MCS **XX** | 3.0 Mbit/s          | TM1: 4.21 Mbits/s       | COTS-UE Cat 4 (150/50 Mbps)

### Number of supported UEs ###

* 16 by default
* up to 256 when compiling with dedicated compile flag
* was tested with 40 COTS-UE

## eNB MAC Layer ##

The MAC layer implements a subset of the **3GPP 36.321** release v8.6 in support of BCH, DLSCH, RACH, and ULSCH channels. 

- RRC interface for CCCH, DCCH, and DTCH
- Proportional fair scheduler (round robin scheduler soon), with the following improvements:
	- Up to 30 users tested in the L2 simulator, CCE allocation in the preprocessor ; the scheduler was also simplified and made more modular
	- Adaptative UL-HARQ
	- Remove out-of-sync UEs
	- No use of the `first_rb` in the UL scheduler ; respects `vrb_map_UL` and `vrb_map` in the DL
- DCI generation
- HARQ Support
- RA procedures and RNTI management
- RLC interface (AM, UM)
- UL power control
- Link adaptation
- Connected DRX (CDRX) support for FDD LTE UE. Compatible with R13 from 3GPP. Support for Cat-M1 UE comming soon.  

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

The RRC layer is based on **3GPP 36.331** v15.6 and implements the following functions:

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
- RRC inactivity timer (release of UE after a period of data inactivity)

## eNB X2AP ##

The X2AP layer is based on **3GPP 36.423** v14.6.0 and implements the following functions:

 - X2 Setup Request
 - X2 Setup Response 
 - X2 Setup Failure
 - Handover Request 
 - Handover Request Acknowledge
 - UE Context Release
 - X2 timers (t_reloc_prep, tx2_reloc_overall)
 - Handover Cancel
 - X2-U interface implemented
 - EN-DC is implemented
 - X2AP : Handling of SgNB Addition Request / Addition Request Acknowledge / Reconfiguration Complete
 - RRC  : Handling of RRC Connection Reconfiguration with 5G cell info, configuration of 5G-NR measurements
 - S1AP : Handling of E-RAB Modification Indication / Confirmation 

## eNB/MCE M2AP ##

The M2AP layer is based on **3GPP 36.443** v14.0.1:
 - M2 Setup Request
 - M2 Setup Response 
 - M2 Setup Failure
 - M2 Scheduling Information
 - M2 Scheduling Information Response 
 - M2 Session Start Request
 - M2 Session Start Response

## MCE/MME M3AP ##

The M3AP layer is based on **3GPP 36.444** v14.0.1:
 - M3 Setup Request
 - M3 Setup Response 
 - M3 Setup Failure
 - M3 Session Start Request
 - M3 Session Start Response


# OpenAirInterface 4G LTE UE Feature Set #

## LTE UE PHY Layer ##

The Physical layer implements **3GPP 36.211**, **36.212**, **36.213** and provides the following features:

- LTE release 8.6 compliant, and implements a subset of release 10
- FDD and TDD configurations: 1 (experimental) and 3
- Bandwidth: 5, 10, and 20 MHz
- Transmission modes: 1, 2 (stable)
- Max number of antennas: 2
- CQI/PMI reporting: aperiodic, feedback mode 3 - 0 and 3 - 1
- PRACH preamble format 0
- All downlink (DL) channels are supported: PSS, SSS, PBCH, PCFICH, PHICH, PDCCH, PDSCH, PMCH
- All uplink (UL) channels are supported: PRACH, PUSCH, PUCCH (format 1/1a/1b), SRS, DRS
- LTE MBMS-dedicated cell (feMBMS) procedures subset for LTE release 14 (experimental)  
- LTE non-MBSFN subframe (feMBMS) Carrier Adquistion Subframe-CAS procedures (PSS/SSS/PBCH/PDSH) (experimental)
- LTE MBSFN MBSFN subframe channel (feMBMS): PMCH (CS@1.25KHz) (channel estimation for 25MHz bandwidth) (experimental) 

## LTE UE MAC Layer ##

The MAC layer implements a subset of the **3GPP 36.321** release v8.6 in support of BCH, DLSCH, RACH, and ULSCH channels. 

- RRC interface for CCCH, DCCH, and DTCH
- HARQ Support
- RA procedures and RNTI management
- RLC interface (AM, UM)
- UL power control
- Link adaptation
- MBMS-dedicated cell (feMBMS) RRC interface for BCCH 
- eMBMS and MBMS-dedicated cell (feMBMS) RRC interface for MCCH, MTCH

## LTE UE RLC Layer ##

The RLC layer implements a full specification of the 3GPP 36.322 release v9.3.

## LTE UE PDCP Layer ##

The current PDCP layer is header compliant with **3GPP 36.323** Rel 10.1.0.

## LTE UE RRC Layer ##

The RRC layer is based on **3GPP 36.331** v14.3.0 and implements the following functions:

- System Information decoding
- RRC connection establishment
- MBMS-dedicated cell (feMBMS) SI-MBMS/SIB1-MBMS management

## LTE UE NAS Layer ##

The NAS layer is based on **3GPP 24.301** and implements the following functions:

- EMM attach/detach, authentication, tracking area update, and more
- ESM default/dedicated bearer, PDN connectivity, and more


# OpenAirInterface 5G-NR Feature Set #

## General Parameters ##

The following features are valid for the gNB and the 5G-NR UE.

*  Static TDD, 
*  FDD
*  Normal CP
*  30 kHz subcarrier spacing
*  Bandwidths: 10, 20, 40, 80, 100MHz (273 Physical Resource Blocks)
*  Intermediate downlink and uplink frequencies to interface with IF equipment
*  Single antenna port (single beam)
*  Slot format: 14 OFDM symbols in UL or DL
*  Highly efficient 3GPP compliant LDPC encoder and decoder (BG1 and BG2 supported)
*  Highly efficient 3GPP compliant polar encoder and decoder
*  Encoder and decoder for short blocks
*  Support for UL transform precoding (SC-FDMA)


## gNB PHY Layer ##

*  30KHz SCS for FR1 and 120 KHz SCS for FR2
*  Generation of NR-PSS/NR-SSS
*  NR-PBCH supports multiple SSBs and flexible periodicity
*  Generation of NR-PDCCH for SIB1 (including generation of DCI, polar encoding, scrambling, modulation, RB mapping, etc)
   - common search space configured by MIB
   - user-specific search space configured by RRC
   - DCI formats: 00, 10 (01 and 11 **under integration**)
*  Generation of NR-PDSCH (including Segmentation, LDPC encoding, rate matching, scrambling, modulation, RB mapping, etc).
   - PDSCH mapping type A and B
   - DMRS configuration type 1 and 2
   - Single and multiple DMRS symbols
   - PTRS support
   - Support for 1, 2 and 4 TX antennas
   - Support for up to 2 layers (currently limited to DMRS configuration type 2)
*  NR-CSIRS Generation of sequence at PHY
*  NR-PUSCH (including Segmentation, LDPC encoding, rate matching, scrambling, modulation, RB mapping, etc).
   - PUSCH mapping type A and B
   - DMRS configuration type 1 and 2
   - Single and multiple DMRS symbols
   - PTRS support
   - Support for 1 RX antenna
   - Support for 1 layer
*  NR-PUCCH 
   - Format 0 (2 bits, mainly for ACK/NACK)
   - Format 2 (up to 64 bits, mainly for CSI feedback)
*  NR-PRACH
   - Formats 0,1,2,3, A1-A3, B1-B3
*  Highly efficient 3GPP compliant LDPC encoder and decoder (BG1 and BG2 are supported)
*  Highly efficient 3GPP compliant polar encoder and decoder
*  Encoder and decoder for short block
   
## gNB Higher Layers ##

**gNB RRC**  
- NR RRC (38.331) Rel 15 messages using new asn1c 
- LTE RRC (36.331) also updated to Rel 15 
- Generation of CellGroupConfig (for eNB) and MIB
- Generation of system information block 1 (SIB1)
- Application to read configuration file and program gNB RRC
- RRC can configure PDCP, RLC, MAC

**gNB X2AP**
- X2 setup with eNB
- Handling of SgNB Addition Request / Addition Request Acknowledge / Reconfiguration Complete 

**gNB MAC**
- MAC -> PHY configuration using NR FAPI P5 interface
- MAC <-> PHY data interface using FAPI P7 interface for BCH PDU, DCI PDU, PDSCH PDU
- Scheduler procedures for SIB1
- Scheduler procedures for RA
- Scheduler procedures for CSI-RS
- MAC downlink scheduler (fixed allocations)
- MAC header generation (including timing advance)
- ACK / NACK handling and HARQ procedures for downlink
- **As of May 2020** only DL was validated with COTS phone ; UL in progress, validated with OAI UE in noS1 mode

# OpenAirInterface 5G-NR UE Feature Set #

**as of May 2020** only supporting "noS1" mode (DL):
- Creates TUN interface to PDCP to inject and receive user-place traffic
- Will only work with OAI gNB configured in the same mode

##  NR UE PHY Layer ##

*  Initial synchronization
*  Time tracking based on PBCH DMRS
*  Frequency offset estimation
*  30KHz SCS for FR1 and 120 KHz SCS for FR2
*  Reception of NR-PSS/NR-SSS
*  NR-PBCH supports multiple SSBs and flexible periodicity
*  Reception of NR-PDCCH for SIB1 (including reception of DCI, polar decoding, de-scrambling, de-modulation, RB de-mapping, etc)
   - common search space configured by MIB
   - user-specific search space configured by RRC
   - DCI formats: 00, 10 (01 and 11 **under integration**)
*  Reception of NR-PDSCH (including Segmentation, LDPC decoding, rate de-matching, de-scrambling, de-modulation, RB de-mapping, etc).
   - PDSCH mapping type A and B
   - DMRS configuration type 1 and 2
   - Single and multiple DMRS symbols
   - PTRS support
   - Support for 1, 2 and 4 RX antennas
   - Support for up to 2 layers (currently limited to DMRS configuration type 2)
*  NR-PUSCH (including Segmentation, LDPC encoding, rate matching, scrambling, modulation, RB mapping, etc).
   - PUSCH mapping type A and B
   - DMRS configuration type 1 and 2
   - Single and multiple DMRS symbols
   - PTRS support
   - Support for 1 TX antenna
   - Support for 1 layer
*  NR-PUCCH 
   - Format 0 (2 bits, mainly for ACK/NACK)
   - Format 2 (up to 64 bits, mainly for CSI feedback)
*  NR-PRACH
   - Formats 0,1,2,3, A1-A3, B1-B3
*  Highly efficient 3GPP compliant LDPC encoder and decoder (BG1 and BG2 are supported)
*  Highly efficient 3GPP compliant polar encoder and decoder
*  Encoder and decoder for short block


## NR UE Higher Layers ##

**UE MAC**
*  Minimum system information (MSI)
    - Initial sync and MIB detection
    - System information block 1 (SIB1) reception
*  MAC -> PHY configuration of PHY via UE FAPI P5 interface
*  Basic MAC to control PHY via UE FAPI P7 interface
*  Random access procedure


**RLC**

**PDCP**



[OAI wiki home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)

[OAI softmodem build procedure](BUILD.md)

[Running the OAI softmodem ](RUNMODEM.md)
