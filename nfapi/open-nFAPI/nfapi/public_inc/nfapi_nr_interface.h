/*
                                nfapi_nr_interface.h
                             -------------------
  AUTHOR  : Raymond Knopp, Guy de Souza, WEI-TAI CHEN
  COMPANY : EURECOM, NTUST
  EMAIL   : Lionel.Gauthier@eurecom.fr, desouza@eurecom.fr, kroempa@gmail.com
*/

#ifndef _NFAPI_NR_INTERFACE_H_
#define _NFAPI_NR_INTERFACE_H_

#include "nfapi_interface.h"


#define NFAPI_NR_MAX_NB_CCE_AGGREGATION_LEVELS 5
#define NFAPI_NR_MAX_NB_TCI_STATES_PDCCH 64
#define NFAPI_NR_MAX_NB_CORESETS 12
#define NFAPI_NR_MAX_NB_SEARCH_SPACES 40

// Extension to the generic structures for single tlv values
typedef struct {
	nfapi_tl_t tl;
	int32_t value;
} nfapi_int32_tlv_t;

typedef struct {
	nfapi_tl_t tl;
	uint32_t value;
} nfapi_uint32_tlv_t;

/*typedef struct {
	nfapi_tl_t tl;
	int64_t value;
} nfapi_int64_tlv_t;*/

typedef struct {
	nfapi_tl_t tl;
	uint64_t value;
} nfapi_uint64_tlv_t;

// nFAPI enums
typedef enum {
  NFAPI_NR_DL_CONFIG_DCI_DL_PDU_TYPE = 0,
  NFAPI_NR_DL_CONFIG_BCH_PDU_TYPE,
  NFAPI_NR_DL_CONFIG_DLSCH_PDU_TYPE,
  NFAPI_NR_DL_CONFIG_PCH_PDU_TYPE,
} nfapi_nr_dl_config_pdu_type_e;

// nFAPI enums
typedef enum {
  NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE = 0,
  NFAPI_NR_UL_CONFIG_ULSCH_PDU_TYPE,
  NFAPI_NR_UL_CONFIG_UCI_PDU_TYPE,
  NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE,
} nfapi_nr_ul_config_pdu_type_e;

//These TLVs are used exclusively by nFAPI
typedef struct
{
  // These TLVs are used to setup the transport connection between VNF and PNF
  // nfapi_ipv4_address_t p7_vnf_address_ipv4;
  // nfapi_ipv6_address_t p7_vnf_address_ipv6;
  // nfapi_uint16_tlv_t p7_vnf_port;

  // nfapi_ipv4_address_t p7_pnf_address_ipv4;
  // nfapi_ipv6_address_t p7_pnf_address_ipv6;
  // nfapi_uint16_tlv_t p7_pnf_port;
  
  // // These TLVs are used to setup the transport connection between VNF and PNF
  // nfapi_uint8_tlv_t dl_ue_per_sf;
  // nfapi_uint8_tlv_t ul_ue_per_sf;

  // These TLVs are used by PNF to report its RF capabilities to the VNF software
  nfapi_rf_bands_t rf_bands;

  // These TLVs are used by the VNF to configure the synchronization with the PNF.
  // nfapi_uint8_tlv_t timing_window;
  // nfapi_uint8_tlv_t timing_info_mode;
  // nfapi_uint8_tlv_t timing_info_period;

  // These TLVs are used by the VNF to configure the RF in the PNF
  // nfapi_uint16_tlv_t max_transmit_power;
  nfapi_uint32_tlv_t nrarfcn;

  // nfapi_nmm_frequency_bands_t nmm_gsm_frequency_bands;
  // nfapi_nmm_frequency_bands_t nmm_umts_frequency_bands;
  // nfapi_nmm_frequency_bands_t nmm_lte_frequency_bands;
  // nfapi_uint8_tlv_t nmm_uplink_rssi_supported;

} nfapi_nr_nfapi_t;

#define NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV4_TAG 0x5100
#define NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV6_TAG 0x5101
#define NFAPI_NR_NFAPI_P7_VNF_PORT_TAG 0x5102
#define NFAPI_NR_NFAPI_P7_PNF_ADDRESS_IPV4_TAG 0x5103
#define NFAPI_NR_NFAPI_P7_PNF_ADDRESS_IPV6_TAG 0x5104
#define NFAPI_NR_NFAPI_P7_PNF_PORT_TAG 0x5105

#define NFAPI_NR_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG 0x510A
#define NFAPI_NR_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG 0x510B
#define NFAPI_NR_NFAPI_RF_BANDS_TAG 0x5114
#define NFAPI_NR_NFAPI_TIMING_WINDOW_TAG 0x511E
#define NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG 0x511F
#define NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG 0x5120
#define NFAPI_NR_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG 0x5128
#define NFAPI_NR_NFAPI_NRARFCN_TAG 0x5129
#define NFAPI_NR_NFAPI_NMM_GSM_FREQUENCY_BANDS_TAG 0x5130
#define NFAPI_NR_NFAPI_NMM_UMTS_FREQUENCY_BANDS_TAG 0x5131
#define NFAPI_NR_NFAPI_NMM_LTE_FREQUENCY_BANDS_TAG 0x5132
#define NFAPI_NR_NFAPI_NMM_UPLINK_RSSI_SUPPORTED_TAG 0x5133

// P5 Message Structures

typedef struct{
  nfapi_tl_t tl;
  uint16_t  corset_ID;                           ///// L1 parameter 'CORESET-ID'
  uint16_t  freq_dom;                       ///// L1 parameter 'CORESET-freq-dom'
  uint16_t  duration;                                       ///// L1 parameter 'CORESET-time-duration'
  uint16_t  reg_type;                            ///// L1 parameter 'CORESET-CCE-REG-mapping-type'
  uint16_t  reg_bundlesize;                                 ///// L1 parameter 'CORESET-REG-bundle-size'
  uint16_t  interleaversize;                                ///// L1 parameter 'CORESET-interleaver-size'
  uint16_t  shiftindex;                                     ///// L1 parameter 'CORESET-shift-index'
  uint16_t  precodergranularity;                            ///// L1 parameter 'CORESET-precoder-granuality'
  uint16_t  tci_stateId;                                    ///// L1 parameter 'TCI-StatesPDCCH'
  uint16_t  tci_present;                               ///// L1 parameter 'TCI-PresentInDCI'
  uint16_t  dmrs_scramblingID;                        ///// L1 parameter 'PDCCH-DMRS-Scrambling-ID'
} nfapi_nr_ControlResourcesSet_t;

typedef struct{
  nfapi_tl_t tl;
  nfapi_nr_ControlResourcesSet_t coreset;
  uint16_t  monitoringSlotPeriodicityAndOffset;             ///// L1 parameters 'Montoring-periodicity-PDCCH-slot'
  uint16_t  monitoringSymbolsWithinSlot;                    ///// L1 parameter 'Montoring-symbols-PDCCH-within-slot'
  uint16_t  nrofCand_aggLevel1;                             ///// L1 parameter 'Aggregation-level-1'
  uint16_t  nrofCand_aggLevel2;                             ///// L1 parameter 'Aggregation-level-2'
  uint16_t  nrofCand_aggLevel4;                             ///// L1 parameter 'Aggregation-level-4'
  uint16_t  nrofCand_aggLevel8;                             ///// L1 parameter 'Aggregation-level-8'
  uint16_t  nrofCand_aggLevel16;                            ///// L1 parameter 'Aggregation-level-16'
  uint16_t  sfi_agg_fmt2_0;    ///// L1 parameters 'SFI-Num-PDCCH-cand' and 'SFI-Aggregation-Level'
  uint16_t  monitoringperiodicity_fmt2_3;        ///// L1 parameter 'SRS-monitoring-periodicity'
  uint16_t  nrof_candidates_fmt_2_3;         ///// L1 parameter 'SRS-Num-PDCCH-cand'
} nfapi_nr_SearchSpace_t;


typedef struct {
  nfapi_uint16_tlv_t  numerology_index_mu;
  nfapi_uint16_tlv_t  duplex_mode;
  nfapi_uint16_tlv_t  dl_cyclic_prefix_type;
  nfapi_uint16_tlv_t  ul_cyclic_prefix_type;
} nfapi_nr_subframe_config_t;

#define NFAPI_NR_SUBFRAME_CONFIG_DUPLEX_MODE_TAG 0x5001
#define NFAPI_NR_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG 0x5002
#define NFAPI_NR_SUBFRAME_CONFIG_PB_TAG 0x5003
#define NFAPI_NR_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG 0x5004
#define NFAPI_NR_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG 0x5005
#define NFAPI_NR_SUBFRAME_CONFIG_NUMEROLOGY_INDEX_MU_TAG 0x5006

typedef struct {
  nfapi_uint16_tlv_t  dl_carrier_bandwidth;
  nfapi_uint16_tlv_t  ul_carrier_bandwidth;
  nfapi_uint16_tlv_t  dl_absolutefrequencypointA;
  nfapi_uint16_tlv_t  ul_absolutefrequencypointA;
  nfapi_uint16_tlv_t  dl_offsettocarrier;
  nfapi_uint16_tlv_t  ul_offsettocarrier;
  nfapi_uint16_tlv_t  dl_subcarrierspacing;
  nfapi_uint16_tlv_t  ul_subcarrierspacing;
  nfapi_uint16_tlv_t  dl_specificcarrier_k0;
  nfapi_uint16_tlv_t  ul_specificcarrier_k0;
  nfapi_uint16_tlv_t  NIA_subcarrierspacing;
} nfapi_nr_rf_config_t;

#define NFAPI_NR_RF_CONFIG_DL_CARRIER_BANDWIDTH_TAG 0x500A
#define NFAPI_NR_RF_CONFIG_UL_CARRIER_BANDWIDTH_TAG 0x500B
#define NFAPI_NR_RF_CONFIG_DL_SUBCARRIERSPACING_TAG 0x500C
#define NFAPI_NR_RF_CONFIG_UL_SUBCARRIERSPACING_TAG 0x500D
#define NFAPI_NR_RF_CONFIG_DL_OFFSETTOCARRIER_TAG   0x500E
#define NFAPI_NR_RF_CONFIG_UL_OFFSETTOCARRIER_TAG   0x500F

typedef struct {
  nfapi_uint16_tlv_t  physical_cell_id;
  nfapi_uint16_tlv_t  half_frame_index;
  nfapi_uint16_tlv_t  ssb_subcarrier_offset;
  nfapi_uint16_tlv_t  ssb_sib1_position_in_burst; // in sib1
  nfapi_uint64_tlv_t  ssb_scg_position_in_burst;  // in servingcellconfigcommon 
  nfapi_uint16_tlv_t  ssb_periodicity;
  nfapi_uint16_tlv_t  ss_pbch_block_power;
  nfapi_uint16_tlv_t  n_ssb_crb;
} nfapi_nr_sch_config_t;

typedef struct {
  nfapi_uint16_tlv_t  dl_bandwidth;
  nfapi_uint16_tlv_t  ul_bandwidth;
  nfapi_uint16_tlv_t  dl_offset;
  nfapi_uint16_tlv_t  ul_offset;
  nfapi_uint16_tlv_t  dl_subcarrierSpacing;
  nfapi_uint16_tlv_t  ul_subcarrierSpacing;
} nfapi_nr_initialBWP_config_t;

#define NFAPI_INITIALBWP_DL_BANDWIDTH_TAG          0x5010
#define NFAPI_INITIALBWP_DL_OFFSET_TAG             0x5011
#define NFAPI_INITIALBWP_DL_SUBCARRIERSPACING_TAG  0x5012
#define NFAPI_INITIALBWP_UL_BANDWIDTH_TAG          0x5013
#define NFAPI_INITIALBWP_UL_OFFSET_TAG             0x5014
#define NFAPI_INITIALBWP_UL_SUBCARRIERSPACING_TAG  0x5015


#define NFAPI_NR_SCH_CONFIG_PHYSICAL_CELL_ID_TAG 0x501E
#define NFAPI_NR_SCH_CONFIG_HALF_FRAME_INDEX_TAG 0x501F
#define NFAPI_NR_SCH_CONFIG_SSB_SUBCARRIER_OFFSET_TAG 0x5020
#define NFAPI_NR_SCH_CONFIG_SSB_POSITION_IN_BURST 0x5021
#define NFAPI_NR_SCH_CONFIG_SSB_PERIODICITY 0x5022
#define NFAPI_NR_SCH_CONFIG_SS_PBCH_BLOCK_POWER 0x5023
#define NFAPI_NR_SCH_CONFIG_N_SSB_CRB 0x5024
#define NFAPI_NR_PDSCH_CONFIG_MAXALLOCATIONS 16
#define NFAPI_NR_PUSCH_CONFIG_MAXALLOCATIONS 16

typedef struct {
  nfapi_uint16_tlv_t  dmrs_TypeA_Position;
  nfapi_uint16_tlv_t  num_PDSCHTimeDomainResourceAllocations;
  nfapi_uint16_tlv_t  PDSCHTimeDomainResourceAllocation_k0[NFAPI_NR_PDSCH_CONFIG_MAXALLOCATIONS];         
  nfapi_uint16_tlv_t  PDSCHTimeDomainResourceAllocation_mappingType[NFAPI_NR_PDSCH_CONFIG_MAXALLOCATIONS]; 
  nfapi_uint16_tlv_t  PDSCHTimeDomainResourceAllocation_startSymbolAndLength[NFAPI_NR_PDSCH_CONFIG_MAXALLOCATIONS];
} nfapi_nr_pdsch_config_t;
#define NFAPI_NR_PDSCH_CONFIG_TAG


typedef struct {
  nfapi_uint16_tlv_t  prach_RootSequenceIndex;                                        ///// L1 parameter 'PRACHRootSequenceIndex'
  nfapi_uint16_tlv_t  prach_msg1_SubcarrierSpacing;                                   ///// L1 parameter 'prach-Msg1SubcarrierSpacing'
  nfapi_uint16_tlv_t  restrictedSetConfig;
  nfapi_uint16_tlv_t  msg3_transformPrecoding;                                        ///// L1 parameter 'msg3-tp'
  nfapi_uint16_tlv_t  ssb_perRACH_OccasionAndCB_PreamblesPerSSB;
  nfapi_uint16_tlv_t  ra_ContentionResolutionTimer;
  nfapi_uint16_tlv_t  rsrp_ThresholdSSB;
  /////////////////--------------------NR RACH-ConfigGeneric--------------------/////////////////
  nfapi_uint16_tlv_t  prach_ConfigurationIndex;                                       ///// L1 parameter 'PRACHConfigurationIndex'
  nfapi_uint16_tlv_t  prach_msg1_FDM;                                                 ///// L1 parameter 'prach-FDM'
  nfapi_uint16_tlv_t  prach_msg1_FrequencyStart;                                      ///// L1 parameter 'prach-frequency-start'
  nfapi_uint16_tlv_t  zeroCorrelationZoneConfig;
  nfapi_uint16_tlv_t  preambleReceivedTargetPower;
  nfapi_uint16_tlv_t  preambleTransMax;
  nfapi_uint16_tlv_t  powerRampingStep;
  nfapi_uint16_tlv_t  ra_ResponseWindow;
} nfapi_nr_rach_config_t;

typedef struct {
  nfapi_uint16_tlv_t  groupHoppingEnabledTransformPrecoding;                          ///// L1 parameter 'Group-hopping-enabled-Transform-precoding'
  nfapi_uint16_tlv_t  msg3_DeltaPreamble;                                             ///// L1 parameter 'Delta-preamble-msg3' 
  nfapi_uint16_tlv_t  p0_NominalWithGrant;                                            ///// L1 parameter 'p0-nominal-pusch-withgrant'
  nfapi_uint16_tlv_t  dmrs_TypeA_Position;
  nfapi_uint16_tlv_t  num_PUSCHTimeDomainResourceAllocations;
  nfapi_uint16_tlv_t  PUSCHTimeDomainResourceAllocation_k2[NFAPI_NR_PUSCH_CONFIG_MAXALLOCATIONS];                                ///// L1 parameter 'K2' 
  nfapi_uint16_tlv_t  PUSCHTimeDomainResourceAllocation_mappingType[NFAPI_NR_PUSCH_CONFIG_MAXALLOCATIONS];                       ///// L1 parameter 'Mapping-type'
  nfapi_uint16_tlv_t  PUSCHTimeDomainResourceAllocation_startSymbolAndLength[NFAPI_NR_PUSCH_CONFIG_MAXALLOCATIONS];
} nfapi_nr_pusch_config_t;

typedef struct {
  uint8_t pucch_resource_common;
  nfapi_uint16_tlv_t  pucch_GroupHopping;                                             ///// L1 parameter 'PUCCH-GroupHopping' 
  nfapi_uint16_tlv_t  p0_nominal;                                                     ///// L1 parameter 'p0-nominal-pucch'
} nfapi_nr_pucch_config_t;



typedef struct {
  nfapi_tl_t tl;

  nfapi_uint16_tlv_t  controlResourceSetZero;
  nfapi_uint16_tlv_t  searchSpaceZero;
  
  //  nfapi_nr_SearchSpace_t           sib1searchSpace;
  //  nfapi_nr_SearchSpace_t           sibssearchSpace; 
  //  nfapi_nr_SearchSpace_t           ra_SearchSpace;
}nfapi_nr_pdcch_config_t;

typedef struct {
//NR TDD-UL-DL-ConfigCommon                ///// L1 parameter 'UL-DL-configuration-common'
  nfapi_uint16_tlv_t  referenceSubcarrierSpacing;                                     ///// L1 parameter 'reference-SCS'
  nfapi_uint16_tlv_t  dl_ul_periodicity;                                  ///// L1 parameter 'DL-UL-transmission-periodicity'
  nfapi_uint16_tlv_t  nrofDownlinkSlots;                                              ///// L1 parameter 'number-of-DL-slots'
  nfapi_uint16_tlv_t  nrofDownlinkSymbols;                                            ///// L1 parameter 'number-of-DL-symbols-common'
  nfapi_uint16_tlv_t  nrofUplinkSlots;                                                ///// L1 parameter 'number-of-UL-slots'
  nfapi_uint16_tlv_t  nrofUplinkSymbols;                                              ///// L1 parameter 'number-of-UL-symbols-common'
  nfapi_uint16_tlv_t  Pattern2Present;
  nfapi_uint16_tlv_t  Pattern2_dl_ul_periodicity;                                  ///// L1 parameter 'DL-UL-transmission-periodicity'
  nfapi_uint16_tlv_t  Pattern2_nrofDownlinkSlots;                                              ///// L1 parameter 'number-of-DL-slots'
  nfapi_uint16_tlv_t  Pattern2_nrofDownlinkSymbols;                                            ///// L1 parameter 'number-of-DL-symbols-common'
  nfapi_uint16_tlv_t  Pattern2_nrofUplinkSlots;                                                ///// L1 parameter 'number-of-UL-slots'
  nfapi_uint16_tlv_t  Pattern2_nrofUplinkSymbols;                                              ///// L1 parameter 'number-of-UL-symbols-common'
} nfapi_nr_tdd_ul_dl_config_t;

typedef struct {
 //RateMatchPattern  is used to configure one rate matching pattern for PDSCH    ///// L1 parameter 'Resource-set-cekk'             
  nfapi_uint16_tlv_t  Match_Id;
  nfapi_uint16_tlv_t  patternType;
  nfapi_uint16_tlv_t  symbolsInResourceBlock;                  ///// L1 parameter 'rate-match-PDSCH-bitmap2
  nfapi_uint16_tlv_t  periodicityAndPattern;                   ///// L1 parameter 'rate-match-PDSCH-bitmap3'
  nfapi_uint16_tlv_t  controlResourceSet;
  nfapi_uint16_tlv_t  subcarrierSpacing;                       ///// L1 parameter 'resource-pattern-scs'
  nfapi_uint16_tlv_t  mode; 
} nfapi_nr_ratematchpattern_t;

typedef struct {
  //NR  RateMatchPatternLTE-CRS
  nfapi_uint16_tlv_t  carrierfreqDL;                           ///// L1 parameter 'center-subcarrier-location'
  nfapi_uint16_tlv_t  dl_bandwidth;                      ///// L1 parameter 'BW'
  nfapi_uint16_tlv_t  nrofcrs_Ports;                           ///// L1 parameter 'rate-match-resources-numb-LTE-CRS-antenna-port'
  nfapi_uint16_tlv_t  v_Shift;                                 ///// L1 parameter 'rate-match-resources-LTE-CRS-v-shift'
  nfapi_uint16_tlv_t  frame_Period;
  nfapi_uint16_tlv_t  frame_Offset;
} nfapi_nr_ratematchpattern_lte_crs_t;

typedef struct {
  nfapi_p4_p5_message_header_t              header;
  uint8_t num_tlv;
  nfapi_nr_subframe_config_t                subframe_config;
  nfapi_nr_rf_config_t                      rf_config;
  nfapi_nr_sch_config_t                     sch_config;
  nfapi_nr_initialBWP_config_t              initialBWP_config;
  nfapi_nr_pdsch_config_t                   pdsch_config;
  nfapi_nr_rach_config_t                    rach_config;
  nfapi_nr_pusch_config_t                   pusch_config;
  nfapi_nr_pucch_config_t                   pucch_config;
  nfapi_nr_pdcch_config_t                   pdcch_config;
  nfapi_nr_tdd_ul_dl_config_t               tdd_ul_dl_config;
  nfapi_nr_ratematchpattern_t               ratematchpattern;
  nfapi_nr_ratematchpattern_lte_crs_t       ratematchpattern_lte_crs;
  nfapi_nr_nfapi_t                          nfapi_config;

  nfapi_vendor_extension_tlv_t              vendor_extension;
} nfapi_nr_config_request_t;



typedef enum {
  NFAPI_NR_DL_DCI_FORMAT_1_0 = 0,
  NFAPI_NR_DL_DCI_FORMAT_1_1,
  NFAPI_NR_DL_DCI_FORMAT_2_0,
  NFAPI_NR_DL_DCI_FORMAT_2_1,
  NFAPI_NR_DL_DCI_FORMAT_2_2,
  NFAPI_NR_DL_DCI_FORMAT_2_3,
  NFAPI_NR_UL_DCI_FORMAT_0_0,
  NFAPI_NR_UL_DCI_FORMAT_0_1
} nfapi_nr_dci_format_e;

typedef enum {
	NFAPI_NR_RNTI_new = 0,
	NFAPI_NR_RNTI_C,
	NFAPI_NR_RNTI_RA,
	NFAPI_NR_RNTI_P,
	NFAPI_NR_RNTI_CS,
	NFAPI_NR_RNTI_TC,
	NFAPI_NR_RNTI_SP_CSI,
	NFAPI_NR_RNTI_SI,
	NFAPI_NR_RNTI_SFI,
	NFAPI_NR_RNTI_INT,
	NFAPI_NR_RNTI_TPC_PUSCH,
	NFAPI_NR_RNTI_TPC_PUCCH,
	NFAPI_NR_RNTI_TPC_SRS
} nfapi_nr_rnti_type_e;

typedef enum {
  NFAPI_NR_USS_FORMAT_0_0_AND_1_0,
  NFAPI_NR_USS_FORMAT_0_1_AND_1_1,
} nfapi_nr_uss_dci_formats_e;

typedef enum {
  NFAPI_NR_SEARCH_SPACE_TYPE_COMMON=0,
  NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC
} nfapi_nr_search_space_type_e;

typedef enum {
  NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_0=0,
  NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_0A,
  NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_1,
  NFAPI_NR_COMMON_SEARCH_SPACE_TYPE_2
} nfapi_nr_common_search_space_type_e;

typedef enum {
  NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE1=0,
  NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE2,
  NFAPI_NR_SSB_AND_CSET_MUX_PATTERN_TYPE3
} nfapi_nr_ssb_and_cset_mux_pattern_type_e;

typedef enum {
  NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED=0,
  NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED
} nfapi_nr_cce_reg_mapping_type_e;

typedef enum {
  NFAPI_NR_CSET_CONFIG_MIB_SIB1=0,
  NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG, // implicit assumption of coreset Id other than 0
  NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG_CSET_0
} nfapi_nr_coreset_config_type_e;

typedef enum {
  NFAPI_NR_CSET_SAME_AS_REG_BUNDLE=0,
  NFAPI_NR_CSET_ALL_CONTIGUOUS_RBS
} nfapi_nr_coreset_precoder_granularity_type_e;

typedef enum {
  NFAPI_NR_QCL_TYPE_A=0,
  NFAPI_NR_QCL_TYPE_B,
  NFAPI_NR_QCL_TYPE_C,
  NFAPI_NR_QCL_TYPE_D
} nfapi_nr_qcl_type_e;

typedef enum {
  NFAPI_NR_SS_PERIODICITY_SL1=1,
  NFAPI_NR_SS_PERIODICITY_SL2=2,
  NFAPI_NR_SS_PERIODICITY_SL4=4,
  NFAPI_NR_SS_PERIODICITY_SL5=5,
  NFAPI_NR_SS_PERIODICITY_SL8=8,
  NFAPI_NR_SS_PERIODICITY_SL10=10,
  NFAPI_NR_SS_PERIODICITY_SL16=16,
  NFAPI_NR_SS_PERIODICITY_SL20=20,
  NFAPI_NR_SS_PERIODICITY_SL40=40,
  NFAPI_NR_SS_PERIODICITY_SL80=80,
  NFAPI_NR_SS_PERIODICITY_SL160=160,
  NFAPI_NR_SS_PERIODICITY_SL320=320,
  NFAPI_NR_SS_PERIODICITY_SL640=640,
  NFAPI_NR_SS_PERIODICITY_SL1280=1280,
  NFAPI_NR_SS_PERIODICITY_SL2560=2560
} nfapi_nr_search_space_monitoring_periodicity_e;

typedef enum {
  NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_A=0,
  NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_B,
  NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_DEFAULT_C,
  NFAPI_NR_PDSCH_TIME_DOMAIN_ALLOC_TYPE_ALLOC_LIST
} nfapi_nr_pdsch_time_domain_alloc_type_e;

typedef enum {
  NFAPI_NR_PDSCH_MAPPING_TYPE_A=0,
  NFAPI_NR_PDSCH_MAPPING_TYPE_B
} nfapi_nr_pdsch_mapping_type_e;

typedef enum {
  NFAPI_NR_PDSCH_RBG_CONFIG_TYPE1=0,
  NFAPI_NR_PDSCH_RBG_CONFIG_TYPE2
} nfapi_nr_pdsch_rbg_config_type_e;

typedef enum {
  NFAPI_NR_PRG_GRANULARITY_2=2,
  NFAPI_NR_PRG_GRANULARITY_4=4,
  NFAPI_NR_PRG_GRANULARITY_WIDEBAND
} nfapi_nr_prg_granularity_e;

typedef enum {
  NFAPI_NR_PRB_BUNDLING_TYPE_STATIC=0,
  NFAPI_NR_PRB_BUNDLING_TYPE_DYNAMIC
} nfapi_nr_prb_bundling_type_e;

typedef enum {
  NFAPI_NR_MCS_TABLE_QAM64_LOW_SE=0,
  NFAPI_NR_MCS_TABLE_QAM256
} nfapi_nr_pdsch_mcs_table_e;

typedef enum {
  NFAPI_NR_DMRS_TYPE1=0,
  NFAPI_NR_DMRS_TYPE2
} nfapi_nr_dmrs_type_e;

// P7 Sub Structures

typedef struct {

nfapi_tl_t tl;

uint8_t format_indicator; //1 bit
uint16_t frequency_domain_assignment; //up to 16 bits
uint8_t time_domain_assignment; // 4 bits
uint8_t frequency_hopping_flag; //1 bit

uint8_t ra_preamble_index; //6 bits
uint8_t ss_pbch_index; //6 bits
uint8_t prach_mask_index; //4 bits

uint8_t vrb_to_prb_mapping; //0 or 1 bit
uint8_t mcs; //5 bits
uint8_t ndi; //1 bit
uint8_t rv; //2 bits
uint8_t harq_pid; //4 bits
uint8_t dai; //0, 2 or 4 bits
uint8_t dai1; //1 or 2 bits
uint8_t dai2; //0 or 2 bits
uint8_t tpc; //2 bits
uint8_t pucch_resource_indicator; //3 bits
uint8_t pdsch_to_harq_feedback_timing_indicator; //0, 1, 2 or 3 bits

uint8_t short_messages_indicator; //2 bits
uint8_t short_messages; //8 bits
uint8_t tb_scaling; //2 bits

uint8_t carrier_indicator; //0 or 3 bits
uint8_t bwp_indicator; //0, 1 or 2 bits
uint8_t prb_bundling_size_indicator; //0 or 1 bits
uint8_t rate_matching_indicator; //0, 1 or 2 bits
uint8_t zp_csi_rs_trigger; //0, 1 or 2 bits
uint8_t transmission_configuration_indication; //0 or 3 bits
uint8_t srs_request; //2 bits
uint8_t cbgti; //CBG Transmission Information: 0, 2, 4, 6 or 8 bits
uint8_t cbgfi; //CBG Flushing Out Information: 0 or 1 bit
uint8_t dmrs_sequence_initialization; //0 or 1 bit

uint8_t srs_resource_indicator;
uint8_t precoding_information;
uint8_t csi_request;
uint8_t ptrs_dmrs_association;
uint8_t beta_offset_indicator; //0 or 2 bits

uint8_t slot_format_indicator_count;
uint8_t *slot_format_indicators;

uint8_t pre_emption_indication_count;
uint16_t *pre_emption_indications; //14 bit

uint8_t block_number_count;
uint8_t *block_numbers;

uint8_t ul_sul_indicator; //0 or 1 bit
uint8_t antenna_ports;

uint16_t reserved; //1_0/C-RNTI:10 bits, 1_0/P-RNTI: 6 bits, 1_0/SI-&RA-RNTI: 16 bits
uint16_t padding;

} nfapi_nr_dl_config_dci_dl_pdu_rel15_t;

//#define NFAPI_NR_DL_CONFIG_REQUEST_DCI_DL_PDU_REL15_TAG 0x????

typedef struct{
  nfapi_tl_t tl;
  uint8_t  coreset_id;
  uint64_t  frequency_domain_resources;
  uint8_t  duration;
  uint8_t  cce_reg_mapping_type;
  uint8_t  reg_bundle_size;
  uint8_t  interleaver_size;
  uint8_t  shift_index;
  uint8_t  precoder_granularity;
  uint8_t  tci_state_id;
  uint8_t  tci_present_in_dci;
  uint32_t dmrs_scrambling_id;
} nfapi_nr_coreset_t;

typedef struct{
  nfapi_tl_t tl;
  uint8_t   search_space_id;
  uint8_t   coreset_id;
  uint8_t   search_space_type;
  uint8_t   duration;
  uint8_t   css_formats_0_0_and_1_0;
  uint8_t   css_format_2_0;
  uint8_t   css_format_2_1;
  uint8_t   css_format_2_2;
  uint8_t   css_format_2_3;
  uint8_t   uss_dci_formats;
  uint16_t  srs_monitoring_periodicity;
  uint16_t  slot_monitoring_periodicity;
  uint16_t  slot_monitoring_offset;
  uint32_t  monitoring_symbols_in_slot;
  uint16_t  number_of_candidates[NFAPI_NR_MAX_NB_CCE_AGGREGATION_LEVELS];
} nfapi_nr_search_space_t;

typedef struct {
  nfapi_tl_t tl;
  uint16_t rnti;
  uint8_t rnti_type;
  uint8_t dci_format;
  uint16_t cce_index;
  /// Number of CRB in BWP that this DCI configures 
  uint16_t n_RB_BWP;
  uint8_t config_type;
  uint8_t search_space_type;
  uint8_t common_search_space_type;  
  uint8_t aggregation_level;
  uint8_t n_rb;
  uint8_t n_symb;
  int8_t rb_offset;
  uint8_t cr_mapping_type;
  uint8_t reg_bundle_size;
  uint8_t interleaver_size;
  uint8_t shift_index;
  uint8_t mux_pattern;
  uint8_t precoder_granularity;
  uint8_t first_slot;
  uint8_t first_symbol;
  uint8_t nb_ss_sets_per_slot;
  uint8_t nb_slots;
  uint8_t sfn_mod2;
  uint16_t scrambling_id;
  nfapi_bf_vector_t   bf_vector;
} nfapi_nr_dl_config_pdcch_parameters_rel15_t;

typedef struct {
  nfapi_tl_t tl;
  uint16_t length;
  int16_t  pdu_index;
  uint16_t transmission_power;
} nfapi_nr_dl_config_bch_pdu_rel15_t;
#define NFAPI_NR_DL_CONFIG_REQUEST_BCH_PDU_REL15_TAG 0x5025

typedef struct {
  nfapi_nr_dl_config_bch_pdu_rel15_t bch_pdu_rel15;
} nfapi_nr_dl_config_bch_pdu;

typedef struct {
  /// Number of PRGs spanning this allocation. Value : 1->275
  uint16_t numPRGs;
  /// Size in RBs of a precoding resource block group (PRG) – to which same precoding and digital beamforming gets applied. Value: 1->275
  uint16_t prgSize;
  /// Number of STD ant ports (parallel streams) feeding into the digBF Value: 0->255
  uint8_t digBFInterfaces;
  uint16_t PMIdx[275];
  uint16_t *beamIdx[275];
} nr_beamforming_t;

typedef struct {
  nfapi_tl_t tl;
  uint16_t pduBitmap;
  uint16_t rnti;
  uint16_t pduIndex;
  // BWP  [TS38.213 sec 12]
  /// Bandwidth part size [TS38.213 sec12]. Number of contiguous PRBs allocated to the BWP, Value: 1->275
  uint16_t BWPSize;
  /// bandwidth part start RB index from reference CRB [TS38.213 sec 12],Value: 0->274
  uint16_t BWPStart;
  /// subcarrierSpacing [TS38.211 sec 4.2], Value:0->4
  uint8_t SubcarrierSpacing;
  /// Cyclic prefix type [TS38.211 sec 4.2], 0: Normal; 1: Extended
  uint8_t CyclicPrefix;
  // Codeword information
  /// Number of code words for this RNTI (UE), Value: 1 -> 2
  uint8_t NrOfCodewords;
  /// Target coding rate [TS38.212 sec 5.4.2.1 and 38.214 sec 5.1.3.1]. This is the number of information bits per 1024 coded bits expressed in 0.1 bit units
  uint16_t targetCodeRate[2]; 
  /// QAM modulation [TS38.212 sec 5.4.2.1 and 38.214 sec 5.1.3.1], Value: 2,4,6,8
  uint8_t qamModOrder[2];
  ///  MCS index [TS38.214, sec 5.1.3.1], should match value sent in DCI Value : 0->31
  uint8_t mcsIndex[2];
  /// MCS-Table-PDSCH [TS38.214, sec 5.1.3.1] 0: notqam256, 1: qam256, 2: qam64LowSE
  uint8_t mcsTable[2];   
  /// Redundancy version index [TS38.212, Table 5.4.2.1-2 and 38.214, Table 5.1.2.1-2], should match value sent in DCI Value : 0->3
  uint8_t rvIndex[2];
  /// Transmit block size (in bytes) [TS38.214 sec 5.1.3.2], Value: 0->65535
  uint32_t TBSize[2];
  /// dataScramblingIdentityPdsch [TS38.211, sec 7.3.1.1], It equals the higher-layer parameter Datascrambling-Identity if configured and the RNTI equals the C-RNTI, otherwise L2 needs to set it to physical cell id. Value: 0->65535
  uint16_t dataScramblingId;
  /// Number of layers [TS38.211, sec 7.3.1.3]. Value : 1->8
  uint8_t nrOfLayers;
  /// PDSCH transmission schemes [TS38.214, sec5.1.1] 0: Up to 8 transmission layers
  uint8_t transmissionScheme;
  /// Reference point for PDSCH DMRS "k" - used for tone mapping [TS38.211, sec 7.4.1.1.2] Resource block bundles [TS38.211, sec 7.3.1.6] Value: 0 -> 1 If 0, the 0 reference point for PDSCH DMRS is at Point A [TS38.211 sec 4.4.4.2]. Resource block bundles generated per sub-bullets 2 and 3 in [TS38.211, sec 7.3.1.6]. For sub-bullet 2, the start of bandwidth part must be set to the start of actual bandwidth part +NstartCORESET and the bandwidth of the bandwidth part must be set to the bandwidth of the initial bandwidth part. If 1, the DMRS reference point is at the lowest VRB/PRB of the allocation. Resource block bundles generated per sub-bullets 1 [TS38.211, sec 7.3.1.6]
  uint8_t refPoint;
  // DMRS  [TS38.211 sec 7.4.1.1]
  /// DMRS symbol positions [TS38.211, sec 7.4.1.1.2 and Tables 7.4.1.1.2-3 and 7.4.1.1.2-4] Bitmap occupying the 14 LSBs with: bit 0: first symbol and for each bit 0: no DMRS 1: DMRS
  uint16_t dlDmrsSymbPos;  
  /// DL DMRS config type [TS38.211, sec 7.4.1.1.2] 0: type 1,  1: type 2
  uint8_t dmrsConfigType;
  /// DL-DMRS-Scrambling-ID [TS38.211, sec 7.4.1.1.2 ] If provided by the higher-layer and the PDSCH is scheduled by PDCCH with CRC scrambled by CRNTI or CS-RNTI, otherwise, L2 should set this to physical cell id. Value: 0->65535
  uint16_t dlDmrsScramblingId;
  /// DMRS sequence initialization [TS38.211, sec 7.4.1.1.2]. Should match what is sent in DCI 1_1, otherwise set to 0. Value : 0->1
  uint8_t SCID;
  /// Number of DM-RS CDM groups without data [TS38.212 sec 7.3.1.2.2] [TS38.214 Table 4.1-1] it determines the ratio of PDSCH EPRE to DM-RS EPRE. Value: 1->3
  uint8_t numDmrsCdmGrpsNoData;
  /// DMRS ports. [TS38.212 7.3.1.2.2] provides description between DCI 1-1 content and DMRS ports. Bitmap occupying the 11 LSBs with: bit 0: antenna port 1000 bit 11: antenna port 1011 and for each bit 0: DMRS port not used 1: DMRS port used
  uint16_t dmrsPorts;
  // Pdsch Allocation in frequency domain [TS38.214, sec 5.1.2.2]
  /// Resource Allocation Type [TS38.214, sec 5.1.2.2] 0: Type 0, 1: Type 1
  uint8_t resourceAlloc;
  /// For resource alloc type 0. TS 38.212 V15.0.x, 7.3.1.2.2 bitmap of RBs, 273 rounded up to multiple of 32. This bitmap is in units of VRBs. LSB of byte 0 of the bitmap represents the first RB of the bwp 
  uint8_t rbBitmap[36];
  /// For resource allocation type 1. [TS38.214, sec 5.1.2.2.2] The starting resource block within the BWP for this PDSCH. Value: 0->274
  uint16_t rbStart;
  /// For resource allocation type 1. [TS38.214, sec 5.1.2.2.2] The number of resource block within for this PDSCH. Value: 1->275
  uint16_t rbSize;
  /// VRB-to-PRB-mapping [TS38.211, sec 7.3.1.6] 0: non-interleaved 1: interleaved with RB size 2 2: Interleaved with RB size 4
  uint8_t VRBtoPRBMapping;
  // Resource Allocation in time domain [TS38.214, sec 5.1.2.1]
  /// Start symbol index of PDSCH mapping from the start of the slot, S. [TS38.214, Table 5.1.2.1-1] Value: 0->13
  uint8_t StartSymbolIndex;
  /// PDSCH duration in symbols, L [TS38.214, Table 5.1.2.1-1] Value: 1->14
  uint8_t NrOfSymbols;
  // PTRS [TS38.214, sec 5.1.6.3]
  /// PT-RS antenna ports [TS38.214, sec 5.1.6.3] [TS38.211, table 7.4.1.2.2-1] Bitmap occupying the 6 LSBs with: bit 0: antenna port 1000 bit 5: antenna port 1005 and for each bit 0: PTRS port not used 1: PTRS port used
  uint8_t PTRSPortIndex ;
  /// PT-RS time density [TS38.214, table 5.1.6.3-1] 0: 1 1: 2 2: 4
  uint8_t PTRSTimeDensity;
  /// PT-RS frequency density [TS38.214, table 5.1.6.3-2] 0: 2 1: 4
  uint8_t PTRSFreqDensity;
  /// PT-RS resource element offset [TS38.211, table 7.4.1.2.2-1] Value: 0->3
  uint8_t PTRSReOffset;
  ///  PT-RS-to-PDSCH EPRE ratio [TS38.214, table 4.1-2] Value :0->3
  uint8_t nEpreRatioOfPDSCHToPTRS;
  // Beamforming
  nr_beamforming_t precodingAndBeamforming;
}nfapi_nr_dl_config_dlsch_pdu_rel15_t;
#define NFAPI_NR_DL_CONFIG_REQUEST_DLSCH_PDU_REL15_TAG

typedef struct {
	nfapi_nr_dl_config_dlsch_pdu_rel15_t dlsch_pdu_rel15;
} nfapi_nr_dl_config_dlsch_pdu;

typedef struct {
  nfapi_tl_t tl;
  nfapi_nr_search_space_t           pagingSearchSpace;
  nfapi_nr_coreset_t   pagingControlResourceSets;
}nfapi_nr_dl_config_pch_pdu_rel15_t;

typedef struct {
  nfapi_nr_dl_config_dci_dl_pdu_rel15_t     dci_dl_pdu_rel15;
  nfapi_nr_dl_config_pdcch_parameters_rel15_t pdcch_params_rel15;
} nfapi_nr_dl_config_dci_dl_pdu;


typedef struct {
  uint8_t pdu_type;
  uint8_t pdu_size;

  union {
  nfapi_nr_dl_config_dci_dl_pdu             dci_dl_pdu;
  nfapi_nr_dl_config_bch_pdu_rel15_t        bch_pdu_rel15;
  nfapi_nr_dl_config_dlsch_pdu              dlsch_pdu;
  nfapi_nr_dl_config_pch_pdu_rel15_t        pch_pdu_rel15;
  };
} nfapi_nr_dl_config_request_pdu_t;

 
typedef struct {
  nfapi_tl_t tl;
  uint8_t   number_dci;
  uint8_t   number_pdu;
  uint8_t   number_pdsch_rnti;
  nfapi_nr_dl_config_request_pdu_t *dl_config_pdu_list;
} nfapi_nr_dl_config_request_body_t;

typedef struct {
  nfapi_p7_message_header_t header;
  uint16_t sfn_sf;
  nfapi_nr_dl_config_request_body_t dl_config_request_body;
  nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_dl_config_request_t;


typedef enum {nr_pusch_freq_hopping_disabled = 0 , 
              nr_pusch_freq_hopping_enabled = 1
              } nr_pusch_freq_hopping_t;

typedef struct{
    uint8_t aperiodicSRS_ResourceTrigger;
} nfapi_nr_ul_srs_config_t;

typedef struct {
    uint8_t bandwidth_part_ind;
    uint16_t number_rbs;
    uint16_t start_rb;
    uint8_t frame_offset;
    uint16_t number_symbols;
    uint16_t start_symbol;
    uint8_t nb_re_dmrs;
    uint8_t length_dmrs;
    nr_pusch_freq_hopping_t pusch_freq_hopping;
    uint8_t mcs;
    uint8_t Qm;
    uint16_t R;
    uint8_t ndi;
    uint8_t rv;
    int8_t accumulated_delta_PUSCH;
    int8_t absolute_delta_PUSCH;
    uint8_t n_layers;
    uint8_t tpmi;
    uint8_t n_dmrs_cdm_groups;
    uint8_t dmrs_ports[4];
    uint8_t n_front_load_symb;
    nfapi_nr_ul_srs_config_t srs_config;
    uint8_t csi_reportTriggerSize;
    uint8_t maxCodeBlockGroupsPerTransportBlock;
    uint8_t ptrs_dmrs_association_port;
    uint8_t beta_offset_ind;
} nfapi_nr_ul_config_ulsch_pdu_rel15_t;





typedef struct {
  uint16_t rnti;
  nfapi_nr_ul_config_ulsch_pdu_rel15_t ulsch_pdu_rel15;
} nfapi_nr_ul_config_ulsch_pdu;

typedef struct {
  uint8_t pdu_type;
  uint8_t pdu_size;

  union {
    //    nfapi_nr_ul_config_uci_pdu uci_pdu;
    nfapi_nr_ul_config_ulsch_pdu ulsch_pdu;
    //    nfapi_nr_ul_config_srs_pdu srs_pdu;
  };
} nfapi_nr_ul_config_request_pdu_t;

typedef struct {
  nfapi_tl_t tl;
  uint8_t   number_pdu;
  nfapi_nr_ul_config_request_pdu_t *ul_config_pdu_list;
} nfapi_nr_ul_config_request_body_t;

typedef struct {
  nfapi_p7_message_header_t header;
  uint16_t sfn_sf;
  nfapi_nr_ul_config_request_body_t ul_config_request_body;
  nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_ul_config_request_t;

#endif
