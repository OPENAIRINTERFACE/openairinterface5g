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

#include <stdbool.h>
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


#define  NB_DL_DATA_TO_UL_ACK              (8) /* size of table TS 38.213 Table 9.2.3-1 */


/***********************************************************************
*
* FUNCTIONALITY    :  System information type 1
*
* DESCRIPTION      :  parameters provided by system information 1
*
************************************************************************/
typedef struct {

  int N_BWP_SIZE; /* size of bandwidth part */
}
SystemInformationBlockType1_nr_t;

/***********************************************************************
*
* FUNCTIONALITY    :  Time Division Duplex
*
* DESCRIPTION      :  interface for FDD/TDD configuration
*
************************************************************************/

#define NR_TDD_DOWNLINK_SLOT               (0x0000)
#define NR_TDD_UPLINK_SLOT                 (0x3FFF) /* uplink bitmap for each symbol, there are 14 symbols per slots */
#define NR_TDD_SET_ALL_SYMBOLS             (0x3FFF)

#define NR_DOWNLINK_SLOT                   (0x01)
#define NR_UPLINK_SLOT                     (0x02)
#define NR_MIXED_SLOT                      (0x03)

#define FRAME_DURATION_MICRO_SEC           (10000)  /* frame duration in microsecond */

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

typedef struct TDD_UL_DL_configCommon_s {
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
  struct TDD_UL_DL_configCommon_s *p_next;
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
  srs_sl1    = 0,
  srs_sl2    = 1,
  srs_sl4    = 2,
  srs_sl5    = 3,
  srs_sl8    = 4,
  srs_sl10   = 5,
  srs_sl16   = 6,
  srs_sl20   = 7,
  srs_sl32   = 8,
  srs_sl40   = 9,
  srs_sl64   = 10,
  srs_sl80   = 11,
  srs_sl160  = 12,
  srs_sl320  = 13,
  srs_sl640  = 14,
  srs_sl1280 = 15,
  srs_sl2560 = 16
} SRS_Periodicity_t;

#define NB_SRS_PERIOD         (17)

static const uint16_t srs_period[NB_SRS_PERIOD]
= { 1, 2, 4, 5, 8, 10, 16, 20, 32, 40, 64, 80, 160, 320, 640, 1280, 2560}
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

/***********************************************************************
*
* FUNCTIONALITY    :  Physical Downlink Shared Channel PDSCH
*
* DESCRIPTION      :  interface description for PSDCH configuration
*
************************************************************************/

#define MAX_NR_RATE_MATCH_PATTERNS            4
#define MAX_NR_ZP_CSI_RS_RESOURCES           32
#define MAX_NR_OF_DL_ALLOCATIONS             16
#define MAX_NR_OF_UL_ALLOCATIONS            (16)

typedef enum {
  typeA = 0,
  typeB = 1
} mappingType_t;

typedef enum {
  pdsch_dmrs_pos0 = 0,
  pdsch_dmrs_pos1 = 1,
  pdsch_dmrs_pos2 = 2,
  pdsch_dmrs_pos3 = 3,
} pdsch_dmrs_AdditionalPosition_t;

/***********************************************************************
*
* FUNCTIONALITY    :  Packed Uplink Shared Channel
*
* DESCRIPTION      :  configuration for PUSCH
*
************************************************************************/

typedef enum {
  pusch_dmrs_type1 = 0,
  pusch_dmrs_type2 = 1
} pusch_dmrs_type_t;
typedef enum {
  pusch_dmrs_pos0 = 0,
  pusch_dmrs_pos1 = 1,
  pusch_dmrs_pos2 = 2,
  pusch_dmrs_pos3 = 3,
} pusch_dmrs_AdditionalPosition_t;
typedef enum {
  pusch_len1 = 1,
  pusch_len2 = 2
} pusch_maxLength_t;

typedef enum {
  txConfig_codebook = 1,
  txConfig_nonCodebook = 2
} txConfig_t;
typedef enum {
  f_hop_mode1 = 1,
  f_hop_mode2 = 2
} frequencyHopping_t;
typedef enum{
  ul_resourceAllocationType0 = 1,
  ul_resourceAllocationType1 = 2,
  ul_dynamicSwitch = 3
} ul_resourceAllocation_t;
typedef enum{
  ul_rgb_config1 = 1,
  ul_rgb_config2 = 2
} ul_rgb_Size_t;
/* Aligned values of this enum to other tranform precoder enums 
 * eg: as defined in fapi_nr_ue_interface.h for transform_precoder_t*/
typedef enum { 
  transformPrecoder_enabled = 0,
  transformPrecoder_disabled = 1
} transformPrecoder_t;
typedef enum {
  codebookSubset_fullyAndPartialAndNonCoherent = 1,
  codebookSubset_partialAndNonCoherent = 2,
  codebookSubset_nonCoherent = 3
} codebookSubset_t;
typedef enum{
  betaOffset_dynamic = 1,
  betaOffset_semiStatic = 2
}betaOffset_type_t;
typedef struct{

} betaOffset_t;
typedef struct {
  betaOffset_type_t betaOffset_type;
  betaOffset_t betaOffset;
} uci_onPusch_t;

/***********************************************************************
*
* FUNCTIONALITY    :  Pucch Power Control
*
* DESCRIPTION      :  configuration for pucch power control
*
************************************************************************/

#define NUMBER_PUCCH_FORMAT_NR                    (5)

typedef int8_t power_level_t;      /* INTEGER (-16..15) */


/***********************************************************************
*
* FUNCTIONALITY    :  Packed Uplink Control Channel aka PUCCH
*
* DESCRIPTION      :  interface description for managing PUCCH
*                     There are two main configurations:
*                     - a first one in SIB1 which gives configuration (common) for initial access
*                     - a second one which is send by network for dedicated mode
*
************************************************************************/

typedef enum {
  neither = 0,
  enable  = 1,
  disable = 2,
} pucch_GroupHopping_t;

typedef struct {
/*
  PUCCH-ConfigCommon ::=          SEQUENCE {
  -- An entry into a 16-row table where each row configures a set of cell-specific PUCCH resources/parameters. The UE uses
  -- those PUCCH resources during initial access on the initial uplink BWP. Once the network provides a dedicated PUCCH-Config
  -- for that bandwidth part the UE applies that one instead of the one provided in this field.
  -- Corresponds to L1 parameter 'PUCCH-resource-common' (see 38.213, section 9.2)
  pucch-ResourceCommon          BIT STRING (SIZE (4))                             OPTIONAL, -- Need R
*/
  uint16_t pucch_ResourceCommon;
/*
  -- Configuration of group- and sequence hopping for all the PUCCH formats 0, 1, 3 and 4. "neither" implies neither group
  -- or sequence hopping is enabled. "enable" enables group hopping and disables sequence hopping. "disable"” disables group
  -- hopping and enables sequence hopping. Corresponds to L1 parameter 'PUCCH-GroupHopping' (see 38.211, section 6.4.1.3)
  pucch-GroupHopping            ENUMERATED { neither, enable, disable },
*/
  pucch_GroupHopping_t   pucch_GroupHopping;
/*
  -- Cell-Specific scrambling ID for group hoppping and sequence hopping if enabled.
  -- Corresponds to L1 parameter 'HoppingID' (see 38.211, section 6.3.2.2)
  hoppingId               BIT STRING (SIZE (10))                              OPTIONAL,   -- Need R
*/
  uint32_t hoppingId;
/*
  -- Power control parameter P0 for PUCCH transmissions. Value in dBm. Only even values (step size 2) allowed.
  -- Corresponds to L1 parameter 'p0-nominal-pucch' (see 38.213, section 7.2)
  p0-nominal                INTEGER (-202..24)                                OPTIONAL,   -- Need R
*/
  int8_t p0_nominal;
} PUCCH_ConfigCommon_nr_t;

#define MAX_NB_OF_PUCCH_RESOURCE_SETS             (4)
#define MAX_NB_OF_PUCCH_RESOURCES_PER_SET         (32)
#define MAX_NB_OF_PUCCH_RESOURCES_PER_SET_NOT_0   (8)
#define MAX_NB_OF_PUCCH_RESOURCES                 (128)
#define NB_INITIAL_PUCCH_RESOURCE                 (16)
#define MAX_PUCCH_RESOURCE_INDICATOR              (8)
#define MAX_NB_CYCLIC_SHIFT                       (4)
#define MAX_NR_OF_SPATIAL_RELATION_INFOS          (8)

typedef enum {
  pucch_format0_nr  = 1,
  pucch_format1_nr  = 2,
  pucch_format2_nr  = 3,
  pucch_format3_nr  = 4,
  pucch_format4_nr  = 5
} pucch_format_nr_t;

typedef enum {
  feature_disabled = 0,
  feature_enabled  = 1,
} feature_status_t;

typedef enum {
  disable_feature = 0,
  enable_feature  = 1
} enable_feature_t;

typedef enum {
  zeroDot08 = 0,
  zeroDot15 = 1,
  zeroDot25 = 2,
  zeroDot35 = 3,
  zeroDot45 = 4,
  zeroDot60 = 5,
  zeroDot80 = 6,
  reserved  = 7
} PUCCH_MaxCodeRate_t;

typedef struct {
  pucch_format_nr_t format;
  uint8_t                startingSymbolIndex;
  uint8_t                nrofSymbols;
  uint16_t               PRB_offset;
  uint8_t                nb_CS_indexes;
  uint8_t                initial_CS_indexes[MAX_NB_CYCLIC_SHIFT];
} initial_pucch_resource_t;


/* structure with all possible fields for pucch format from 0 to 4  */
typedef struct {
  pucch_format_nr_t      format;              /* format   0    1    2    3    4    */
  uint8_t                initialCyclicShift;  /*          x    x                   */
  uint8_t                nrofSymbols;         /*          x    x    x    x    x    */
  uint8_t                startingSymbolIndex; /*          x    x    x    x    x    */
  uint8_t                timeDomainOCC;       /*               x                   */
  uint8_t                nrofPRBs;            /*                    x    x         */
  uint8_t                occ_length;          /*                              x    */
  uint8_t                occ_Index;           /*                              x    */
} PUCCH_format_t;

typedef struct {
  uint8_t                pucch_ResourceId;           /*  maxNrofPUCCH-Resources = 128 */
  uint16_t               startingPRB;                /* maxNrofPhysicalResourceBlocks  = 275 */
  feature_status_t       intraSlotFrequencyHopping;
  uint16_t               secondHopPRB;
  PUCCH_format_t         format_parameters;
} PUCCH_Resource_t;

typedef struct {
  uint8_t                pucch_ResourceSetId; /* maxNrofPUCCH-ResourceSets = 4 */
/*
  -- PUCCH resources of format0 and format1 are only allowed in the first PUCCH reosurce set,
  -- i.e., in a PUCCH-ResourceSet with pucch-ResourceSetId = 0. This set may contain between 8 and 32 resources.
  -- PUCCH resources of format2, format3 and format4 are only allowed  in a PUCCH-ReosurceSet with pucch-ResourceSetId > 0. If present, these sets must contain 8 resources each.
  -- The UE chooses a PUCCH-Resource from this list based on the 3-bit PUCCH resource indicator field in DCI as
  -- speciied in 38.213, FFS_section.
  -- Note that this list contains only a list of resource IDs. The actual resources are configured in PUCCH-Config.
*/
  uint8_t                pucch_resource_id[MAX_NB_OF_PUCCH_RESOURCES_PER_SET];  /* pucch resources belonging to current set is a 32 bit map to address maxNrofPUCCH-ResourcesPerSet = 32 resources */

  uint8_t                first_resources_set_R_PUCCH;  /* size of first resource set which can be higher than 8 */
/*
  -- Maximum number of payload bits minus 1 that the UE may transmit using this PUCCH resource set. In a PUCCH occurrence, the UE
  -- chooses the first of its PUCCH-ResourceSet which supports the number of bits that the UE wants to transmit.
  -- The field is not present in the first set (Set0) since the maximum Size of Set0 is specified to be 3 bit.
  -- The field is not present in the last configured set since the UE derives its maximum payload size as specified in 38.213.
  -- This field can take integer values that are multiples of 4. Corresponds to L1 parameter 'N_2' or 'N_3' (see 38.213, section 9.2)
*/
  uint16_t               maxPayloadMinus1;  /* INTEGER (4..256) */
} PUCCH_ResourceSet_t;

typedef struct {
/*
  -- Enabling inter-slot frequency hopping when PUCCH Format 1, 3 or 4 is repeated over multiple slots.
  -- The field is not applicable for format 2.
 */
  feature_status_t       interslotFrequencyHopping;
/*
  -- Enabling 2 DMRS symbols per hop of a PUCCH Format 3 or 4 if both hops are more than X symbols when FH is enabled (X=4).
  -- Enabling 4 DMRS sybmols for a PUCCH Format 3 or 4 with more than 2X+1 symbols when FH is disabled (X=4).
  -- Corresponds to L1 parameter 'PUCCH-F3-F4-additional-DMRS' (see 38.213, section 9.2.1)
  -- The field is not applicable for format 1 and 2.
*/
  enable_feature_t       additionalDMRS;
/*
  -- Max coding rate to determine how to feedback UCI on PUCCH for format 2, 3 or 4
  -- Corresponds to L1 parameter 'PUCCH-F2-maximum-coderate', 'PUCCH-F3-maximum-coderate' and 'PUCCH-F4-maximum-coderate'
  -- (see 38.213, section 9.2.5)
  -- The field is not applicable for format 1.
*/
  PUCCH_MaxCodeRate_t    maxCodeRate;
/*
  -- Number of slots with the same PUCCH F1, F3 or F4. When the field is absent the UE applies the value n1.
  -- Corresponds to L1 parameter 'PUCCH-F1-number-of-slots', 'PUCCH-F3-number-of-slots' and 'PUCCH-F4-number-of-slots'
  -- (see 38.213, section 9.2.6)
  -- The field is not applicable for format 2.
*/
  uint8_t                nrofSlots;
/*
  -- Enabling pi/2 BPSK for UCI symbols instead of QPSK for PUCCH.
  -- Corresponds to L1 parameter 'PUCCH-PF3-PF4-pi/2PBSK' (see 38.213, section 9.2.5)
  -- The field is not applicable for format 1 and 2.
*/
  feature_status_t       pi2PBSK;
/*
  -- Enabling simultaneous transmission of CSI and HARQ-ACK feedback with or without SR with PUCCH Format 2, 3 or 4
  -- Corresponds to L1 parameter 'PUCCH-F2-Simultaneous-HARQ-ACK-CSI', 'PUCCH-F3-Simultaneous-HARQ-ACK-CSI' and
  -- 'PUCCH-F4-Simultaneous-HARQ-ACK-CSI' (see 38.213, section 9.2.5)
  -- When the field is absent the UE applies the value OFF
  -- The field is not applicable for format 1.
*/
  enable_feature_t       simultaneousHARQ_ACK_CSI;
} PUCCH_FormatConfig_t;

typedef struct {
  PUCCH_Resource_t       *PUCCH_Resource[MAX_NB_OF_PUCCH_RESOURCES];
  PUCCH_ResourceSet_t    *PUCCH_ResourceSet[MAX_NB_OF_PUCCH_RESOURCE_SETS];
  PUCCH_FormatConfig_t   *formatConfig[NUMBER_PUCCH_FORMAT_NR-1];   /* format 0 is not there */
  uint8_t                dl_DataToUL_ACK[NB_DL_DATA_TO_UL_ACK];     /* table TS 38.213 Table 9.2.3-1: Mapping of PSDCH-to-HARQ_feedback timing indicator field values to numbers of slots */
  void                   *spatial_Relation_Info[MAX_NR_OF_SPATIAL_RELATION_INFOS];
} PUCCH_Config_t;



/***********************************************************************
*
* FUNCTIONALITY    :  Scheduling Request Configuration (SR)
*
* DESCRIPTION      :  Configuration for Scheduling Request send on PUCCH
*                     In the case that scheduling should be send alone
*                     in a PUCCH, it should used its configuration.
*
************************************************************************/

#define MAX_NR_OF_SR_CONFIG_PER_CELL_GROUP          (8)

#define NB_SR_PERIOD    (15)


typedef enum {
  sr_sym2     = 0,
  sr_sym6or7  = 1,
  sr_sl1      = 2,
  sr_sl2      = 3,
  sr_sl4      = 4,
  sr_sl5      = 5,
  sr_sl8      = 6,
  sr_sl10     = 7,
  sr_sl16     = 8,
  sr_sl20     = 9,
  sr_sl40     = 10,
  sr_sl80     = 11,
  sr_sl160    = 12,
  sr_sl320    = 13,
  sr_sl640    = 14
} sr_periodicity_t;

typedef struct {

  uint8_t                schedulingRequestResourceId;
/*
  -- The ID of the SchedulingRequestConfig that uses this scheduling request resource.
 */
  uint8_t                schedulingRequestID;

/*
  -- SR periodicity and offset in number of slots. Corresponds to L1 parameter 'SR-periodicity' and 'SR-offset' (see 38.213, section 9.2.2)
  -- The following periodicities may be configured depending on the chosen subcarrier spacing:
  -- SCS =  15 kHz: 2sym, 7sym, 1sl, 2sl, 4sl, 5sl, 8sl, 10sl, 16sl, 20sl, 40sl, 80sl
  -- SCS =  30 kHz: 2sym, 7sym, 1sl, 2sl, 4sl, 8sl, 10sl, 16sl, 20sl, 40sl, 80sl, 160sl
  -- SCS =  60 kHz: 2sym, 7sym/6sym, 1sl, 2sl, 4sl, 8sl, 16sl, 20sl, 40sl, 80sl, 160sl, 320sl
  -- SCS = 120 kHz: 2sym, 7sym, 1sl, 2sl, 4sl, 8sl, 16sl, 40sl, 80sl, 160sl, 320sl, sl640
  -- sym6or7 corresponds to 6 symbols if extended cyclic prefix and a SCS of 60 kHz are configured, otherwise it corresponds to 7 symbols.
  -- For periodicities sym2, sym7 and sl1 the UE assumes an offset of 0 slots.
*/
  sr_periodicity_t       periodicity;
  uint16_t               offset;

/*
  -- ID of the PUCCH resource in which the UE shall send the scheduling request. The
  -- actual PUCCH-Resource is configured in PUCCH-Config of the same UL BWP and serving cell as this SchedulingRequestResourceConfig.
  -- The network configures a PUCCH-Resource of PUCCH-format0 or PUCCH-format1
  -- (other formats not supported). Corresponds to L1 parameter 'SR-resource' (see 38.213, section 9.2.2)
 */
  uint8_t                resource;

} SchedulingRequestResourceConfig_t;

typedef struct {
  int                                active_sr_id;
  SchedulingRequestResourceConfig_t  *sr_ResourceConfig[MAX_NR_OF_SR_CONFIG_PER_CELL_GROUP];
} scheduling_request_config_t;


#undef EXTERN
#undef INIT_VARIABLES_PHY_IMPLEMENTATION_DEFS_NR_H
#endif /* PHY_IMPL_DEFS_NR_H */
