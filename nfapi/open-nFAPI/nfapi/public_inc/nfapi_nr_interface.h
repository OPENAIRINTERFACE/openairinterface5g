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
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface_scf.h"

#define NFAPI_NR_MAX_NB_CCE_AGGREGATION_LEVELS 5
#define NFAPI_NR_MAX_NB_TCI_STATES_PDCCH 64
#define NFAPI_NR_MAX_NB_CORESETS 12
#define NFAPI_NR_MAX_NB_SEARCH_SPACES 40



// nFAPI enums


//These TLVs are used exclusively by nFAPI
#define NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV4_TAG 0x0100
#define NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV6_TAG 0x0101
#define NFAPI_NR_NFAPI_P7_VNF_PORT_TAG 0x0102
#define NFAPI_NR_NFAPI_P7_PNF_ADDRESS_IPV4_TAG 0x0103
#define NFAPI_NR_NFAPI_P7_PNF_ADDRESS_IPV6_TAG 0x0104
#define NFAPI_NR_NFAPI_P7_PNF_PORT_TAG 0x0105
#define NFAPI_NR_NFAPI_DL_TTI_TIMING_OFFSET 0x0106
#define NFAPI_NR_NFAPI_UL_TTI_TIMING_OFFSET 0x0107
#define NFAPI_NR_NFAPI_UL_DCI_TIMING_OFFSET 0x0108
#define NFAPI_NR_NFAPI_TX_DATA_TIMING_OFFSET 0x0109
#define NFAPI_NR_NFAPI_TIMING_WINDOW_TAG 0x011E
#define NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG 0x011F
#define NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG 0x0120

/*
#define NFAPI_NR_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG 0x510A
#define NFAPI_NR_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG 0x510B
#define NFAPI_NR_NFAPI_RF_BANDS_TAG 0x5114
#define NFAPI_NR_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG 0x5128
#define NFAPI_NR_NFAPI_NRARFCN_TAG 0x5129
#define NFAPI_NR_NFAPI_NMM_GSM_FREQUENCY_BANDS_TAG 0x5130
#define NFAPI_NR_NFAPI_NMM_UMTS_FREQUENCY_BANDS_TAG 0x5131
#define NFAPI_NR_NFAPI_NMM_LTE_FREQUENCY_BANDS_TAG 0x5132
#define NFAPI_NR_NFAPI_NMM_UPLINK_RSSI_SUPPORTED_TAG 0x5133
*/
// P5 Message Structures

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
  NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED=1,
  NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED=0
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

// P7 Sub Structures

/*
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
*/
//#define NFAPI_NR_DL_CONFIG_REQUEST_DCI_DL_PDU_REL15_TAG 0x????



/*
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

*/


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
