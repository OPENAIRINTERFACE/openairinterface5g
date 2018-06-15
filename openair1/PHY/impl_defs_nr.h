/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/***********************************************************************
*
* FILENAME    :  impl_defs_nr.h
*
* MODULE      :
*
* DESCRIPTION :  NR Physical channel configuration and variable structure definitions
*                This is an interface between ue physical layer and RRC message from network
*                see TS 38.336 Radio Resource Control (RRC) protocol specification
*
************************************************************************/

#ifndef PHY_IMPL_DEFS_NR_H
#define PHY_IMPL_DEFS_NR_H

#include "types.h"

#ifdef DEFINE_VARIABLES_PHY_IMPLEMENTATION_DEFS_NR_H
#define EXTERN
#define INIT_VARIABLES_PHY_IMPLEMENTATION_DEFS_NR_H
#else
#define EXTERN  extern
#undef INIT_VARIABLES_PHY_IMPLEMENTATION_DEFS_NR_H
#endif

#ifdef PHY_DBG_DEV_TST
  #define PHY_DBG_DEV_TST_PRINTF(...)      printf(__VA_ARGS__)
#else
  #define PHY_DBG_DEV_TST_PRINTF(...)
#endif

/* to set for UE capabilities */
#define MAX_NR_OF_SRS_RESOURCE_SET         (1)
#define MAX_NR_OF_SRS_RESOURCES_PER_SET    (1)

#define NR_NUMBER_OF_SUBFRAMES_PER_FRAME   (10)
#define MAX_NROFSRS_PORTS                  (4)

/* TS 38.211 Table 4.3.2-1: Number of OFDM symbols per slot, slots per frame, and slots per subframe for normal cyclic prefix */
#define MU_NUMBER                          (5)
EXTERN const uint8_t N_slot_subframe[MU_NUMBER]
#ifdef INIT_VARIABLES_PHY_IMPLEMENTATION_DEFS_NR_H
= { 1, 2, 4, 8, 16}
#endif
;

/***********************************************************************
*
* FUNCTIONALITY    :  Time Division Duplex
*
* DESCRIPTION      :  interface for FDD/TDD configuration
*
************************************************************************/

#define MAX_NR_OF_SLOTS                    (320)    /* maximum number of slots for tdd configuration which is periodic */

#define NR_TDD_DOWNLINK_SLOT               (0x0000)
#define NR_TDD_UPLINK_SLOT                 (0x3FFF) /* uplink bitmap for each symbol, there are 14 symbols per slots */
#define NR_TDD_SET_ALL_SYMBOLS             (0x3FFF)

#define FRAME_DURATION_MICRO_SEC           (10000)  /* frame duration in microsecond */

typedef enum {
  SLOT_DL = 0,
  SLOT_UL = 1,
} nr_slot_t;

typedef enum {
  ms0p5    = 500,                 /* duration is given in microsecond */
  ms0p625  = 625,
  ms1      = 1000,
  ms1p25   = 1250,
  ms2      = 2000,
  ms2p5    = 2500,
  ms5      = 5000,
  ms10     = 10000,
} dl_UL_TransmissionPeriodicity_t;

typedef struct {
  /// Reference SCS used to determine the time domain boundaries in the UL-DL pattern which must be common across all subcarrier specific
  /// virtual carriers, i.e., independent of the actual subcarrier spacing using for data transmission.
  /// Only the values 15 or 30 kHz  (<6GHz), 60 or 120 kHz (>6GHz) are applicable.
  /// Corresponds to L1 parameter 'reference-SCS' (see 38.211, section FFS_Section)
  /// \ subcarrier spacing
  uint8_t referenceSubcarrierSpacing;
  /// \ Periodicity of the DL-UL pattern. Corresponds to L1 parameter 'DL-UL-transmission-periodicity' (see 38.211, section FFS_Section)
  dl_UL_TransmissionPeriodicity_t dl_UL_TransmissionPeriodicity;
  /// \ Number of consecutive full DL slots at the beginning of each DL-UL pattern.
  /// Corresponds to L1 parameter 'number-of-DL-slots' (see 38.211, Table 4.3.2-1)
  uint8_t nrofDownlinkSlots;
  /// \ Number of consecutive DL symbols in the beginning of the slot following the last full DL slot (as derived from nrofDownlinkSlots).
  /// If the field is absent or released, there is no partial-downlink slot.
  /// Corresponds to L1 parameter 'number-of-DL-symbols-common' (see 38.211, section FFS_Section).
  uint8_t nrofDownlinkSymbols;
  /// \ Number of consecutive full UL slots at the end of each DL-UL pattern.
  /// Corresponds to L1 parameter 'number-of-UL-slots' (see 38.211, Table 4.3.2-1)
  uint8_t nrofUplinkSlots;
  /// \ Number of consecutive UL symbols in the end of the slot preceding the first full UL slot (as derived from nrofUplinkSlots).
  /// If the field is absent or released, there is no partial-uplink slot.
  /// Corresponds to L1 parameter 'number-of-UL-symbols-common' (see 38.211, section FFS_Section)
  uint8_t nrofUplinkSymbols;
  /// \ for setting a sequence
  struct TDD_UL_DL_configCommon_t *p_next_TDD_UL_DL_configCommon_t;
} TDD_UL_DL_configCommon_t;

typedef struct {
  /// \ Identifies a slot within a dl-UL-TransmissionPeriodicity (given in tdd-UL-DL-configurationCommon)
  uint16_t slotIndex;
  /// \ The direction (downlink or uplink) for the symbols in this slot. "allDownlink" indicates that all symbols in this slot are used
  /// for downlink; "allUplink" indicates that all symbols in this slot are used for uplink; "explicit" indicates explicitly how many symbols
  /// in the beginning and end of this slot are allocated to downlink and uplink, respectively.
  /// Number of consecutive DL symbols in the beginning of the slot identified by slotIndex.
  /// If the field is absent the UE assumes that there are no leading DL symbols.
  /// Corresponds to L1 parameter 'number-of-DL-symbols-dedicated' (see 38.211, section FFS_Section)
  uint16_t nrofDownlinkSymbols;
  /// Number of consecutive UL symbols in the end of the slot identified by slotIndex.
  /// If the field is absent the UE assumes that there are no trailing UL symbols.
  /// Corresponds to L1 parameter 'number-of-UL-symbols-dedicated' (see 38.211, section FFS_Section)
  uint16_t nrofUplinkSymbols;
  /// \ for setting a sequence
  struct TDD_UL_DL_SlotConfig_t *p_next_TDD_UL_DL_SlotConfig;
} TDD_UL_DL_SlotConfig_t;

/***********************************************************************
*
* FUNCTIONALITY    :  Sounding Reference Signal
*
* DESCRIPTION      :  interface description for managing uplink SRS signals from UE
*                     SRS are generated by UE and transmit to network
*                     depending on configuration
*
************************************************************************/

EXTERN const int16_t SRS_antenna_port[MAX_NROFSRS_PORTS]
#ifdef INIT_VARIABLES_PHY_IMPLEMENTATION_DEFS_NR_H
= { 1000, 1001, 1002, 1003 }
#endif
;

typedef enum {
  port1           = 1,
  port2           = 2,
  port4           = 4
} nrof_Srs_Ports_t;

typedef enum {
  neitherHopping  = 0,
  groupHopping    = 1,
  sequenceHopping = 2
} groupOrSequenceHopping_t;

typedef enum {
  aperiodic       = 0,
  semi_persistent = 1,
  periodic        = 2
} resourceType_t;

typedef enum {
  sl1    = 0,
  sl2    = 1,
  sl4    = 2,
  sl5    = 3,
  sl8    = 4,
  sl10   = 5,
  sl16   = 6,
  sl20   = 7,
  sl32   = 8,
  sl40   = 9,
  sl64   = 10,
  sl80   = 11,
  sl160  = 12,
  sl320  = 13,
  sl640  = 14,
  sl1280 = 15,
  sl2560 = 16
} SRS_Periodicity_t;

#define NB_SRS_PERIOD         (17)

const uint16_t srs_period[NB_SRS_PERIOD]
#ifdef INIT_VARIABLES_PHY_IMPLEMENTATION_DEFS_NR_H
= { 1, 2, 4, 5, 8, 10, 16, 20, 32, 40, 64, 80, 160, 320, 640, 1280, 2560}
#endif
;

/// SRS_Resource of SRS_Config information element from 38.331 RRC specifications
typedef struct {
  /// \brief srs resource identity.
  uint8_t srs_ResourceId;
  /// \brief number of srs ports.
  nrof_Srs_Ports_t nrof_SrsPorts;
  /// \brief index of prts port index.
  uint8_t ptrs_PortIndex;
  /// \brief srs transmission comb see parameter SRS-TransmissionComb 38.214 section &6.2.1.
  uint8_t transmissionComb;
  /// \brief comb offset.
  uint8_t combOffset;
  /// \brief cyclic shift configuration - see parameter SRS-CyclicShiftConfig 38.214 &6.2.1.
  uint8_t cyclicShift;
  /// \brief OFDM symbol location of the SRS resource within a slot.
  // Corresponds to L1 parameter 'SRS-ResourceMapping' (see 38.214, section 6.2.1 and 38.211, section 6.4.1.4).
  // startPosition (SRSSymbolStartPosition = 0..5; "0" refers to the last symbol, "1" refers to the second last symbol.
  uint8_t resourceMapping_startPosition;
  /// \brief number of OFDM symbols (N = 1, 2 or 4 per SRS resource).
  uint8_t resourceMapping_nrofSymbols;
  /// \brief RepetitionFactor (r = 1, 2 or 4).
  uint8_t resourceMapping_repetitionFactor;
  /// \brief Parameter(s) defining frequency domain position and configurable shift to align SRS allocation to 4 PRB grid.
  // Corresponds to L1 parameter 'SRS-FreqDomainPosition' (see 38.214, section 6.2.1)
  uint8_t  freqDomainPosition;    // INTEGER (0..67),
  uint16_t freqDomainShift;       // INTEGER (0..268),
  /// \brief Includes  parameters capturing SRS frequency hopping
  // Corresponds to L1 parameter 'SRS-FreqHopping' (see 38.214, section 6.2.1)
  uint8_t freqHopping_c_SRS;      // INTEGER (0..63),
  uint8_t freqHopping_b_SRS;      // INTEGER (0..3),
  uint8_t freqHopping_b_hop;      // INTEGER (0..3)
  // Parameter(s) for configuring group or sequence hopping
  // Corresponds to L1 parameter 'SRS-GroupSequenceHopping' see 38.211
  groupOrSequenceHopping_t groupOrSequenceHopping;
  /// \brief Time domain behavior of SRS resource configuration.
  // Corresponds to L1 parameter 'SRS-ResourceConfigType' (see 38.214, section 6.2.1).
  // For codebook based uplink transmission, the network configures SRS resources in the same resource set with the same
  // time domain behavior on periodic, aperiodic and semi-persistent SRS
  SRS_Periodicity_t SRS_Periodicity;
  uint16_t          SRS_Offset;
  /// \brief Sequence ID used to initialize psedo random group and sequence hopping.
  // Corresponds to L1 parameter 'SRS-SequenceId' (see 38.214, section 6.2.1).
  uint16_t sequenceId;            // BIT STRING (SIZE (10))
  /// \brief Configuration of the spatial relation between a reference RS and the target SRS. Reference RS can be SSB/CSI-RS/SRS.
  // Corresponds to L1 parameter 'SRS-SpatialRelationInfo' (see 38.214, section 6.2.1)
  uint8_t spatialRelationInfo_ssb_Index;      // SSB-Index,
  uint8_t spatialRelationInfo_csi_RS_Index;   // NZP-CSI-RS-ResourceId,
  uint8_t spatialRelationInfo_srs_Id;         // SRS-ResourceId
} SRS_Resource_t;

typedef enum {
  beamManagement     = 0,
  codebook           = 1,
  nonCodebook        = 2,
  antennaSwitching   = 3,
} SRS_resourceSet_usage_t;

typedef enum {
  sameAsFci2         = 0,
  separateClosedLoop = 1
} srs_PowerControlAdjustmentStates_t;

typedef enum {
  ssb_Index          = 0,
  csi_RS_Index       = 1,
} pathlossReferenceRS_t;

// SRS_Config information element from 38.331 RRC specifications from 38.331 RRC specifications.
typedef struct {
  /// \brief The ID of this resource set. It is unique in the context of the BWP in which the parent SRS-Config is defined.
  uint8_t srs_ResourceSetId;
  /// \brief number of resources in the resource set
  uint8_t number_srs_Resource;
  /// \brief The IDs of the SRS-Resources used in this SRS-ResourceSet.
  /// in fact this is an array of pointers to resource structures
  SRS_Resource_t *p_srs_ResourceList[MAX_NR_OF_SRS_RESOURCES_PER_SET];
  resourceType_t resourceType;
  /// \brief The DCI "code point" upon which the UE shall transmit SRS according to this SRS resource set configuration.
  // Corresponds to L1 parameter 'AperiodicSRS-ResourceTrigger' (see 38.214, section 6.1.1.2)
  uint8_t aperiodicSRS_ResourceTrigger;		 // INTEGER (0..maxNrofSRS-TriggerStates-1) : [0:3]
  /// \brief ID of CSI-RS resource associated with this SRS resource set. (see 38.214, section 6.1.1.2).
  uint8_t NZP_CSI_RS_ResourceId;
  /// \brief An offset in number of slots between the triggering DCI and the actual transmission of this SRS-ResourceSet.
  // If the field is absent the UE applies no offset (value 0)
  uint8_t aperiodic_slotOffset; // INTEGER (1..8)
  /// \brief Indicates if the SRS resource set is used for beam management vs. used for either codebook based or non-codebook based transmission.
  // Corresponds to L1 parameter 'SRS-SetUse' (see 38.214, section 6.2.1)
  // FFS_CHECK: Isn't codebook/noncodebook already known from the ulTxConfig in the SRS-Config? If so, isn't the only distinction
  // in the set between BeamManagement, AtennaSwitching and "Other”? Or what happens if SRS-Config=Codebook but a Set=NonCodebook?
  SRS_resourceSet_usage_t usage;
  /// \brief alpha value for SRS power control. Corresponds to L1 parameter 'alpha-srs' (see 38.213, section 7.3).
  // When the field is absent the UE applies the value 1
  uint8_t alpha;
  /// \brief P0 value for SRS power control. The value is in dBm. Only even values (step size 2) are allowed.
  // Corresponds to L1 parameter 'p0-srs' (see 38.213, section 7.3)
  int8_t p0;          // INTEGER (-202..24)
  /// \brief A reference signal (e.g. a CSI-RS config or a SSblock) to be used for SRS path loss estimation.
  /// Corresponds to L1 parameter 'srs-pathlossReference-rs-config' (see 38.213, section 7.3)
  pathlossReferenceRS_t pathlossReferenceRS_t;
  uint8_t path_loss_SSB_Index;
  uint8_t path_loss_NZP_CSI_RS_ResourceId;
  /// \brief Indicates whether hsrs,c(i) = fc(i,1) or hsrs,c(i) = fc(i,2) (if twoPUSCH-PC-AdjustmentStates are configured)
  /// or separate close loop is configured for SRS. This parameter is applicable only for Uls on which UE also transmits PUSCH.
  /// If absent or release, the UE applies the value sameAs-Fci1
  /// Corresponds to L1 parameter 'srs-pcadjustment-state-config' (see 38.213, section 7.3)
  srs_PowerControlAdjustmentStates_t srs_PowerControlAdjustmentStates;
} SRS_ResourceSet_t;

typedef struct {
  uint8_t active_srs_Resource_Set;  /* ue implementation specific */
  uint8_t number_srs_Resource_Set;  /* ue implementation specific */
  SRS_ResourceSet_t *p_SRS_ResourceSetList[MAX_NR_OF_SRS_RESOURCE_SET]; /* ue implementation specific */
} SRS_NR;

//#define RX_NB_TH_MAX 3
//#define RX_NB_TH 3

#define LTE_SLOTS_PER_SUBFRAME 2

#define LTE_NUMBER_OF_SUBFRAMES_PER_FRAME 10
#define LTE_SLOTS_PER_FRAME  20
#define LTE_CE_FILTER_LENGTH 5
#define LTE_CE_OFFSET LTE_CE_FILTER_LENGTH
#define TX_RX_SWITCH_SYMBOL (NUMBER_OF_SYMBOLS_PER_FRAME>>1)
#define PBCH_PDU_SIZE 3 //bytes

#define PRACH_SYMBOL 3 //position of the UL PSS wrt 2nd slot of special subframe

#define NUMBER_OF_FREQUENCY_GROUPS (lte_frame_parms->N_RB_DL)

#define SSS_AMP 1148

#define MAX_NUM_PHICH_GROUPS 56  //110 RBs Ng=2, p.60 36-212, Sec. 6.9

#define MAX_MBSFN_AREA 8

#undef EXTERN
#undef INIT_VARIABLES_PHY_IMPLEMENTATION_DEFS_NR_H
#endif /* PHY_IMPL_DEFS_NR_H */
