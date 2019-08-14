/*
                                nfapi_nr_interface.h
                             -------------------
  AUTHOR  : Chenyu Zhang, Florian Kaltenberger
  COMPANY : BUPT, EURECOM
  EMAIL   : octopus@bupt.edu.cn, florian.kaltenberger@eurecom.fr
*/

#ifndef _NFAPI_NR_INTERFACE_SCF_H_
#define _NFAPI_NR_INTERFACE_SCF_H_

#include "stddef.h"
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

// 2019.8
// SCF222_5G-FAPI_PHY_SPI_Specificayion.pdf Section 3.2

//PHY API message types

typedef enum {
  NFAPI_NR_PHY_MSG_TYPE_PARAM_REQUEST=  0x00,
  NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE= 0x01,
  NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST= 0x02,
  NFAPI_NR_PHY_MSG_TYPE_CONFIG_RESPONSE=0X03,
  NFAPI_NR_PHY_MSG_TYPE_START_REQUEST=  0X04,
  NFAPI_NR_PHY_MSG_TYPE_STOP_REQUEST=   0X05,
  NFAPI_NR_PHY_MSG_TYPE_STOP_INDICATION=0X06,
  NFAPI_NR_PHY_MSG_TYPE_ERROR_INDICATION=0X07,
  //RESERVED 0X08 ~ 0X7F
  NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST= 0X80,
  NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST= 0X81,
  NFAPI_NR_PHY_MSG_TYPE_SLOT_INDICATION=0X82,
  NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST= 0X83,
  NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST=0X54,
  NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION=0X85,
  NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION= 0X86,
  NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION= 0X87,
  NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION= 0X88,
  NFAPI_NR_PHY_MSG_TYPE_PACH_INDICATION= 0X89
  //RESERVED 0X8a ~ 0xff
} nfapi_nr_phy_msg_type_e;

// SCF222_5G-FAPI_PHY_SPI_Specificayion.pdf Section 3.3

//3.3.1 PARAM

typedef struct {
	nfapi_tl_t tl;
	uint16_t value;

} nfapi_nr_param_tlv_t;

//same with nfapi_param_request_t
typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_param_request_t;

typedef enum {
  NFAPI_NR_PARAM_MSG_OK = 0,
	NFAPI_NR_PARAM_MSG_INVALID_STATE

} nfapi_nr_param_errors_e;

typedef struct {
  nfapi_nr_param_errors_e error_code;
  //Number of TLVs contained in the message body.
  uint8_t number_of_tlvs;
  nfapi_nr_param_tlv_t TLV;
} nfapi_nr_param_response_t;


//PARAM and CONFIG TLVs are used in the PARAM and CONFIG message exchanges, respectively


//nfapi_nr_param_tlv_format_t cell param ~ measurement_param:

//table 3-9

#define NFAPI_NR_PARAM_TLV_RELEASE_CAPABILITY_TAG 0x0001
#define  NFAPI_NR_PARAM_TLV_PHY_STATE_TAG         0x0002
#define  NFAPI_NR_PARAM_TLV_SKIP_BLANK_DL_CONFIG_TAG 0x0003
#define  NFAPI_NR_PARAM_TLV_SKIP_BLANK_UL_CONFIG_TAG 0x0004
#define  NFAPI_NR_PARAM_TLV_NUM_CONFIG_TLVS_TO_REPORT_TAG 0x0005
#define  NFAPI_NR_PARAM_TLV_CYCLIC_PREFIX_TAG 0x0006
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_SUBCARRIER_SPACINGS_DL_TAG 0x0007
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_BANDWIDTH_DL_TAG 0x0008
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_SUBCARRIER_SPACINGS_UL_TAG 0x0009
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_BANDWIDTH_UL_TAG 0x000A
#define  NFAPI_NR_PARAM_TLV_CCE_MAPPING_TYPE_TAG 0x000B
#define  NFAPI_NR_PARAM_TLV_CORESET_OUTSIDE_FIRST_3_OFDM_SYMS_OF_SLOT_TAG 0x000C
#define  NFAPI_NR_PARAM_TLV_PRECODER_GRANULARITY_CORESET_TAG 0x000D
#define  NFAPI_NR_PARAM_TLV_PDCCH_MU_MIMO_TAG 0x000E
#define  NFAPI_NR_PARAM_TLV_PDCCH_PRECODER_CYCLING_TAG 0x000F
#define  NFAPI_NR_PARAM_TLV_MAX_PDCCHS_PER_SLOT_TAG 0x0010
#define  NFAPI_NR_PARAM_TLV_PUCCH_FORMATS_TAG 0x0011
#define  NFAPI_NR_PARAM_TLV_MAX_PUCCHS_PER_SLOT_TAG 0x0012
#define  NFAPI_NR_PARAM_TLV_PDSCH_MAPPING_TYPE_TAG 0x0013
#define  NFAPI_NR_PARAM_TLV_PDSCH_ALLOCATION_TYPES_TAG 0x0014
#define  NFAPI_NR_PARAM_TLV_PDSCH_VRB_TO_PRB_MAPPING_TAG 0x0015
#define  NFAPI_NR_PARAM_TLV_PDSCH_CBG_TAG 0x0016
#define  NFAPI_NR_PARAM_TLV_PDSCH_DMRS_CONFIG_TYPES_TAG 0x0017
#define  NFAPI_NR_PARAM_TLV_PDSCH_DMRS_MAX_LENGTH_TAG 0x0018
#define  NFAPI_NR_PARAM_TLV_PDSCH_DMRS_ADDITIONAL_POS_TAG 0x0019
#define  NFAPI_NR_PARAM_TLV_MAX_PDSCH_S_YBS_PER_SLOT_TAG 0x001A
#define  NFAPI_NR_PARAM_TLV_MAX_NUMBER_MIMO_LAYERS_PDSCH_TAG 0x001B
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_MAX_MODULATION_ORDER_DL_TAG 0x001C
#define  NFAPI_NR_PARAM_TLV_MAX_MU_MIMO_USERS_DL_TAG 0x001D
#define  NFAPI_NR_PARAM_TLV_PDSCH_DATA_IN_DMRS_SYMBOLS_TAG 0x001E
#define  NFAPI_NR_PARAM_TLV_PREMPTION_SUPPORT_TAG 0x001F
#define  NFAPI_NR_PARAM_TLV_PDSCH_NON_SLOT_SUPPORT_TAG 0x0020
#define  NFAPI_NR_PARAM_TLV_UCI_MUX_ULSCH_IN_PUSCH_TAG 0x0021
#define  NFAPI_NR_PARAM_TLV_UCI_ONLY_PUSCH_TAG 0x0022
#define  NFAPI_NR_PARAM_TLV_PUSCH_FREQUENCY_HOPPING_TAG 0x0023
#define  NFAPI_NR_PARAM_TLV_PUSCH_DMRS_CONFIG_TYPES_TAG 0x0024
#define  NFAPI_NR_PARAM_TLV_PUSCH_DMRS_MAX_LEN_TAG 0x0025
#define  NFAPI_NR_PARAM_TLV_PUSCH_DMRS_ADDITIONAL_POS_TAG 0x0026
#define  NFAPI_NR_PARAM_TLV_PUSCH_CBG_TAG 0x0027
#define  NFAPI_NR_PARAM_TLV_PUSCH_MAPPING_TYPE_TAG 0x0028
#define  NFAPI_NR_PARAM_TLV_PUSCH_ALLOCATION_TYPES_TAG 0x0029
#define  NFAPI_NR_PARAM_TLV_PUSCH_VRB_TO_PRB_MAPPING_TAG 0x002A
#define  NFAPI_NR_PARAM_TLV_PUSCH_MAX_PTRS_PORTS_TAG 0x002B
#define  NFAPI_NR_PARAM_TLV_MAX_PDUSCHS_TBS_PER_SLOT_TAG 0x002C
#define  NFAPI_NR_PARAM_TLV_MAX_NUMBER_MIMO_LAYERS_NON_CB_PUSCH_TAG 0x002D
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_MODULATION_ORDER_UL_TAG 0x002E
#define  NFAPI_NR_PARAM_TLV_MAX_MU_MIMO_USERS_UL_TAG 0x002F
#define  NFAPI_NR_PARAM_TLV_DFTS_OFDM_SUPPORT_TAG 0x0030
#define  NFAPI_NR_PARAM_TLV_PUSCH_AGGREGATION_FACTOR_TAG 0x0031
#define  NFAPI_NR_PARAM_TLV_PRACH_LONG_FORMATS_TAG 0x0032
#define  NFAPI_NR_PARAM_TLV_PRACH_SHORT_FORMATS_TAG 0x0033
#define  NFAPI_NR_PARAM_TLV_PRACH_RESTRICTED_SETS_TAG 0x0034
#define  NFAPI_NR_PARAM_TLV_MAX_PRACH_FD_OCCASIONS_IN_A_SLOT_TAG 0x0035
#define  NFAPI_NR_PARAM_TLV_RSSI_MEASUREMENT_SUPPORT_TAG 0x0036

typedef struct{
  uint16_t num_config_tlvs_to_report;
  nfapi_nr_param_tlv_t tlv;
} nfapi_nr_num_config_tlvs_to_report;

typedef struct 
{
  uint16_t release_capability;//TAG 0x0001
  uint16_t phy_state;
  uint8_t  skip_blank_dl_config;
  uint8_t  skip_blank_ul_config;
  nfapi_nr_num_config_tlvs_to_report* num_config_tlvs_to_report_list;
} nfapi_nr_cell_param_t;

//table 3-10 Carrier parameters
typedef struct 
{
  uint8_t  cyclic_prefix;//TAG 0x0006
  uint8_t  supported_subcarrier_spacings_dl;
  uint16_t supported_bandwidth_dl;
  uint8_t  supported_subcarrier_spacings_ul;
  uint16_t supported_bandwidth_ul;
  
} nfapi_nr_carrier_param_t;

//table 3-11 PDCCH parameters
typedef struct 
{
  uint8_t  cce_mapping_type;
  uint8_t  coreset_outside_first_3_of_ofdm_syms_of_slot;
  uint8_t  coreset_precoder_granularity_coreset;
  uint8_t  pdcch_mu_mimo;
  uint8_t  pdcch_precoder_cycling;
  uint8_t  max_pdcch_per_slot;//TAG 0x0010

} nfapi_nr_pdcch_param_t;

//table 3-12 PUCCH parameters
typedef struct 
{
  uint8_t pucch_formats;
  uint8_t max_pucchs_per_slot;

} nfapi_nr_pucch_param_t;

//table 3-13 PDSCH parameters
typedef struct 
{
  uint8_t pdsch_mapping_type;
  uint8_t pdsch_allocation_types;
  uint8_t pdsch_vrb_to_prb_mapping;
  uint8_t pdsch_cbg;
  uint8_t pdsch_dmrs_config_types;
  uint8_t pdsch_dmrs_max_length;
  uint8_t pdsch_dmrs_additional_pos;
  uint8_t max_pdsch_tbs_per_slot;
  uint8_t max_number_mimo_layers_pdsch;
  uint8_t supported_max_modulation_order_dl;
  uint8_t max_mu_mimo_users_dl;
  uint8_t pdsch_data_in_dmrs_symbols;
  uint8_t premption_support;//TAG 0x001F
  uint8_t pdsch_non_slot_support;

} nfapi_nr_pdsch_param_t;

//table 3-14
typedef struct 
{
  uint8_t uci_mux_ulsch_in_pusch;
  uint8_t uci_only_pusch;
  uint8_t pusch_frequency_hopping;
  uint8_t pusch_dmrs_config_types;
  uint8_t pusch_dmrs_max_len;
  uint8_t pusch_dmrs_additional_pos;
  uint8_t pusch_cbg;
  uint8_t pusch_mapping_type;
  uint8_t pusch_allocation_types;
  uint8_t pusch_vrb_to_prb_mapping;
  //uint8 ? ↓ see table 3-14
  uint8_t pusch_max_ptrs_ports;
  uint8_t max_pduschs_tbs_per_slot;
  uint8_t max_number_mimo_layers_non_cb_pusch;
  uint8_t supported_modulation_order_ul;
  uint8_t max_mu_mimo_users_ul;
  uint8_t dfts_ofdm_support;
  uint8_t pusch_aggregation_factor;//TAG 0x0031

} nfapi_nr_pusch_param_t;

//table 3-15
typedef struct 
{
  uint8_t prach_long_formats;
  uint8_t prach_short_formats;
  uint8_t prach_restricted_sets;
  uint8_t max_prach_fd_occasions_in_a_slot;

} nfapi_nr_prach_param_t;

//table 3-16
typedef struct 
{
  uint8_t rssi_measurement_support;
} nfapi_nr_measurement_param_t;

//-------------------------------------------//
//3.3.2 CONFIG
typedef struct {
	nfapi_tl_t tl;
	uint16_t value;
} nfapi_nr_config_tlv_t;

typedef struct {
	uint8_t number_of_tlvs;
	nfapi_nr_config_tlv_t tlv;
} nfapi_nr_config_request_t;

typedef enum {
  NFAPI_NR_CONFIG_MSG_OK = 0,
	NFAPI_NR_CONFIG_MSG_INVALID_CONFIG  //The configuration provided has missing mandatory TLVs, or TLVs that are invalid or unsupported in this state.
} nfapi_nr_config_errors_e;

typedef struct {
	nfapi_nr_config_errors_e error_code;
  uint8_t number_of_invalid_tlvs_that_can_only_be_configured_in_idle;
  uint8_t unmber_of_missing_tlvs;
  //? ↓
  nfapi_nr_config_tlv_t* tlv_invalid_list;
  nfapi_nr_config_tlv_t* tlv_invalid_idle_list;
  nfapi_nr_config_tlv_t* tlv_invalid_running_list;
  nfapi_nr_config_tlv_t* tlv_missing_list;

} nfapi_nr_config_response_t;

//nfapi_nr_config_tlv_format_t carrier config ~ precoding config:

#define NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG 0x1001
#define NFAPI_NR_CONFIG_DL_FREQUENCY_TAG 0x1002
#define NFAPI_NR_CONFIG_DL_K0_TAG 0x1003
#define NFAPI_NR_CONFIG_DL_GRID_SIZE_TAG 0x1004
#define NFAPI_NR_CONFIG_NUM_TX_ANT_TAG 0x1005
#define NFAPI_NR_CONFIG_UPLINK_BANDWIDTH_TAG 0x1006
#define NFAPI_NR_CONFIG_UPLINK_FREQUENCY_TAG 0x1007
#define NFAPI_NR_CONFIG_UL_K0_TAG 0x1008
#define NFAPI_NR_CONFIG_UL_GRID_SIZE_TAG 0x1009
#define NFAPI_NR_CONFIG_NUM_RX_ANT_TAG 0x100A
#define NFAPI_NR_CONFIG_FREQUENCY_SHIFT_7P5KHZ_TAG 0x100B

#define NFAPI_NR_CONFIG_PHY_CELL_ID_TAG 0x100C
#define NFAPI_NR_CONFIG_FRAME_DUPLEX_TYPE_TAG 0x100D

#define NFAPI_NR_CONFIG_SS_PBCH_POWER_TAG 0x100E
#define NFAPI_NR_CONFIG_BCH_PAYLOAD_TAG 0x100F
#define NFAPI_NR_CONFIG_SCS_COMMON_TAG 0x1010

#define NFAPI_NR_CONFIG_PRACH_SEQUENCE_LENGTH_TAG 0x1011
#define NFAPI_NR_CONFIG_PRACH_SUB_C_SPACING_TAG 0x1012
#define NFAPI_NR_CONFIG_RESTRICTED_SET_CONFIG_TAG 0x1013
#define NFAPI_NR_CONFIG_NUM_PRACH_FD_OCCASIONS_TAG 0x1014
#define NFAPI_NR_CONFIG_PRACH_ROOT_SEQUENCE_INDEX_TAG 0x1015
#define NFAPI_NR_CONFIG_NUM_ROOT_SEQUENCES_TAG 0x1016
#define NFAPI_NR_CONFIG_K1_TAG 0x1017
#define NFAPI_NR_CONFIG_PRACH_ZERO_CORR_CONF_TAG 0x1018
#define NFAPI_NR_CONFIG_NUM_UNUSED_ROOT_SEQUENCES_TAG 0x1019
#define NFAPI_NR_CONFIG_UNUSED_ROOT_SEQUENCES_TAG 0x101A
#define NFAPI_NR_CONFIG_SSB_PER_RACH_TAG 0x101B
#define NFAPI_NR_CONFIG_PRACH_MULTIPLE_CARRIERS_IN_A_BAND_TAG 0x101C

#define NFAPI_NR_CONFIG_SSB_OFFSET_POINT_A_TAG 0x101D
#define NFAPI_NR_CONFIG_BETA_PSS_TAG 0x101E
#define NFAPI_NR_CONFIG_SSB_PERIOD_TAG 0x101F
#define NFAPI_NR_CONFIG_SSB_SUBCARRIER_OFFSET_TAG 0x1020
#define NFAPI_NR_CONFIG_MIB_TAG 0x1021
#define NFAPI_NR_CONFIG_SSB_MASK_TAG 0x1022
#define NFAPI_NR_CONFIG_BEAM_ID_TAG 0x1023
#define NFAPI_NR_CONFIG_SS_PBCH_MULTIPLE_CARRIERS_IN_A_BAND_TAG 0x1024
#define NFAPI_NR_CONFIG_MULTIPLE_CELLS_SS_PBCH_IN_A_CARRIER_TAG 0x1025
#define NFAPI_NR_CONFIG_TDD_PERIOD_TAG 0x1026
#define NFAPI_NR_CONFIG_SLOT_CONFIG_TAG 0x1027

#define NFAPI_NR_CONFIG_RSSI_MEASUREMENT_TAG 0x1028

//table 3-21
typedef struct 
{
  uint16_t dl_bandwidth;//Carrier bandwidth for DL in MHz [38.104, sec 5.3.2] Values: 5, 10, 15, 20, 25, 30, 40,50, 60, 70, 80,90,100,200,400
  uint32_t dl_frequency; //Absolute frequency of DL point A in KHz [38.104, sec5.2 and 38.211 sec 4.4.4.2] Value: 450000 -> 52600000
  uint16_t dl_k0[5];//𝑘_{0}^{𝜇} for each of the numerologies [38.211, sec 5.3.1] Value: 0 ->23699
  uint16_t dl_grid_size[5];//Grid size 𝑁_{𝑔𝑟𝑖𝑑}^{𝑠𝑖𝑧𝑒,𝜇} for each of the numerologies [38.211, sec 4.4.2] Value: 0->275 0 = this numerology not used
  uint16_t num_tx_ant;//Number of Tx antennas
  uint16_t uplink_bandwidth;//Carrier bandwidth for UL in MHz. [38.104, sec 5.3.2] Values: 5, 10, 15, 20, 25, 30, 40,50, 60, 70, 80,90,100,200,400
  uint32_t uplink_frequency;//Absolute frequency of UL point A in KHz [38.104, sec5.2 and 38.211 sec 4.4.4.2] Value: 450000 -> 52600000
  uint16_t ul_k0[5];//𝑘0 𝜇 for each of the numerologies [38.211, sec 5.3.1] Value: : 0 ->23699
  uint16_t ul_grid_size[5];//Grid size 𝑁𝑔𝑟𝑖𝑑 𝑠𝑖𝑧𝑒,𝜇 for each of the numerologies [38.211, sec 4.4.2]. Value: 0->275 0 = this numerology not used
  uint16_t num_rx_ant;//
  uint8_t  frequency_shift_7p5khz;//Indicates presence of 7.5KHz frequency shift. Value: 0 = false 1 = true

} nfapi_nr_carrier_config_t; 

//table 3-22
typedef struct 
{
  uint8_t phy_cell_id;//Physical Cell ID, 𝑁_{𝐼𝐷}^{𝑐𝑒𝑙𝑙} [38.211, sec 7.4.2.1] Value: 0 ->1007
  uint8_t frame_duplex_type;//Frame duplex type Value: 0 = FDD 1 = TDD

} nfapi_nr_cell_config_t;

//table 3-23
typedef struct 
{
  uint32_t ss_pbch_power;//SSB Block Power Value: TBD (-60..50 dBm)
  uint8_t  bch_payload;//Defines option selected for generation of BCH payload, see Table 3-13 (v0.0.011 Value: 0: MAC generates the full PBCH payload 1: PHY generates the timing PBCH bits 2: PHY generates the full PBCH payload
  uint8_t  scs_common;//subcarrierSpacing for common, used for initial access and broadcast message. [38.211 sec 4.2] Value:0->3

} nfapi_nr_ssb_config_t;

//table 3-24

typedef struct 
{
  uint8_t unused_root_sequences;//Unused root sequence or sequences per FD occasion. Required for noise estimation.

} nfapi_nr_num_unused_root_sequences_t;

typedef struct 
{
  uint8_t  num_prach_fd_occasions;
  uint16_t prach_root_sequence_index;//Starting logical root sequence index, 𝑖, equivalent to higher layer parameter prach-RootSequenceIndex [38.211, sec 6.3.3.1] Value: 0 -> 837
  uint8_t  num_root_sequences;//Number of root sequences for a particular FD occasion that are required to generate the necessary number of preambles
  uint16_t k1;//Frequency offset (from UL bandwidth part) for each FD. [38.211, sec 6.3.3.2] Value: from 0 to 272
  uint8_t  prach_zero_corr_conf;//PRACH Zero CorrelationZone Config which is used to dervive 𝑁𝑐𝑠 [38.211, sec 6.3.3.1] Value: from 0 to 15
  uint8_t  num_unused_root_sequences;//Number of unused sequences available for noise estimation per FD occasion. At least one unused root sequence is required per FD occasion.
  nfapi_nr_num_unused_root_sequences_t* num_unused_root_sequences_list;

} nfapi_nr_num_prach_fd_occasions_t;

typedef struct 
{
  uint8_t prach_sequence_length;//RACH sequence length. Long or Short sequence length. Only short sequence length is supported for FR2. [38.211, sec 6.3.3.1] Value: 0 = Long sequence 1 = Short sequence
  uint8_t prach_sub_c_spacing;//Subcarrier spacing of PRACH. [38.211 sec 4.2] Value:0->4
  uint8_t restricted_set_config;//PRACH restricted set config Value: 0: unrestricted 1: restricted set type A 2: restricted set type B
  uint8_t num_prach_fd_occasions;//Number of RACH frequency domain occasions. Corresponds to the parameter 𝑀 in [38.211, sec 6.3.3.2] which equals the higher layer parameter msg1FDM Value: 1,2,4,8
  nfapi_nr_num_prach_fd_occasions_t* num_prach_fd_occasions_list;
  uint8_t ssb_per_rach;//SSB-per-RACH-occasion Value: 0: 1/8 1:1/4, 2:1/2 3:1 4:2 5:4, 6:8 7:16
  uint8_t prach_multiple_carriers_in_a_band;//0 = disabled 1 = enabled

} nfapi_nr_prach_config_t;

//table 3-25
typedef struct 
{
  uint32_t ssb_mask;//Bitmap for actually transmitted SSB. MSB->LSB of first 32 bit number corresponds to SSB 0 to SSB 31 MSB->LSB of second 32 bit number corresponds to SSB 32 to SSB 63 Value for each bit: 0: not transmitted 1: transmitted

} nfapi_nr_ssb_mask_size_2_t;

typedef struct 
{
  uint8_t beam_id[64];//BeamID for each SSB in SsbMask. For example, if SSB mask bit 26 is set to 1, then BeamId[26] will be used to indicate beam ID of SSB 26. Value: from 0 to 63

} nfapi_nr_ssb_mask_size_64_t;

typedef struct 
{
  uint16_t ssb_offset_point_a;//Offset of lowest subcarrier of lowest resource block used for SS/PBCH block. Given in PRB [38.211, section 4.4.4.2] Value: 0->2199
  uint8_t  beta_pss;//PSS EPRE to SSS EPRE in a SS/PBCH block [38.213, sec 4.1] Values: 0 = 0dB
  uint8_t  ssb_period;//SSB periodicity in msec Value: 0: ms5 1: ms10 2: ms20 3: ms40 4: ms80 5: ms160
  uint8_t  ssb_subcarrier_offset;//ssbSubcarrierOffset or 𝑘𝑆𝑆𝐵 (38.211, section 7.4.3.1) Value: 0->31
  uint32_t MIB;//MIB payload, where the 24 MSB are used and represent the MIB in [38.331 MIB IE] and represent 0 1 2 3 1 , , , ,..., A− a a a a a [38.212, sec 7.1.1]
  nfapi_nr_ssb_mask_size_2_t* ssb_mask_size_2_list;//2
  nfapi_nr_ssb_mask_size_64_t* ssb_mask_size_64_list;//64
  uint8_t  ss_pbch_multiple_carriers_in_a_band;//0 = disabled 1 = enabled
  uint8_t  multiple_cells_ss_pbch_in_a_carrier;//Indicates that multiple cells will be supported in a single carrier 0 = disabled 1 = enabled

} nfapi_nr_ssb_table_t;

//table 3-26

//? 
typedef struct 
{
  uint8_t slot_config;//For each symbol in each slot a uint8_t value is provided indicating: 0: DL slot 1: UL slot 2: Guard slot

} nfapi_nr_max_num_of_symbol_per_slot_t;

typedef struct 
{
  nfapi_nr_max_num_of_symbol_per_slot_t* max_num_of_symbol_per_slot_list;

} nfapi_nr_max_tdd_periodicity_t;

typedef struct 
{
  uint8_t tdd_period;//DL UL Transmission Periodicity. Value:0: ms0p5 1: ms0p625 2: ms1 3: ms1p25 4: ms2 5: ms2p5 6: ms5 7: ms10 
  nfapi_nr_max_tdd_periodicity_t* max_tdd_periodicity_list;

} nfapi_nr_tdd_table_t;

//table 3-27
typedef struct 
{
  uint8_t rssi_measurement;//RSSI measurement unit. See Table 3-16 for RSSI definition. Value: 0: Do not report RSSI 1: dBm 2: dBFS

} nfapi_nr_measurement_config_t;

//------------------------------//
//3.3.3 START

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_start_request_t;

typedef enum {
	NFAPI_NR_START_MSG_INVALID_STATE
} nfapi_nr_start_errors_e;

//3.3.4 STOP

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_stop_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_stop_indication_t;

typedef enum {
	NFAPI_NR_STOP_MSG_INVALID_STATE
} nfapi_nr_stop_errors_e;

//3.3.5 PHY Notifications
typedef enum {
  NFAPI_NR_PHY_API_MSG_OK              =0x0,
	NFAPI_NR_PHY_API_MSG_INVALID_STATE   =0x1,
  NFAPI_NR_PHY_API_MSG_INVALID_CONFIG  =0x2,
  NFAPI_NR_PHY_API_SFN_OUT_OF_SYNC     =0X3,
  NFAPI_NR_PHY_API_MSG_SLOR_ERR        =0X4,
  NFAPI_NR_PHY_API_MSG_BCH_MISSING     =0X5,
  NFAPI_NR_PHY_API_MSG_INVALID_SFN     =0X6,
  NFAPI_NR_PHY_API_MSG_UL_DCI_ERR      =0X7,
  NFAPI_NR_PHY_API_MSG_TX_ERR          =0X8
} nfapi_nr_phy_notifications_errors_e;

typedef struct {
	uint16_t sfn; //0~1023
  uint16_t slot;//0~319
  nfapi_nr_phy_msg_type_e msg_id;//Indicate which message received by the PHY has an error. Values taken from Table 3-4.
  nfapi_nr_phy_notifications_errors_e error_code;
} nfapi_nr_phy_notifications_error_indicate_t;

//-----------------------//
//3.3.6 Storing Precoding and Beamforming Tables

//table 3-32
//? 
typedef struct {
	uint16_t beam_idx;     //0~65535
} nfapi_nr_dig_beam_t;

typedef struct {
	uint16_t dig_beam_weight_Re;
  uint16_t dig_beam_weight_Im;
} nfapi_nr_txru_t;

typedef struct {
	uint16_t num_dig_beams; //0~65535
  uint16_t num_txrus;    //0~65535
  nfapi_nr_dig_beam_t* dig_beam_list;
  nfapi_nr_txru_t*  txru_list;
} nfapi_nr_dbt_pdu_t;


//table 3-33
//?
typedef struct {
  uint16_t num_ant_ports;
	int16_t precoder_weight_Re;
  int16_t precoder_weight_Im;
} nfapi_nr_num_ant_ports_t;

typedef struct {
  uint16_t numLayers;   //0~65535
	nfapi_nr_num_ant_ports_t* num_ant_ports_list;
} nfapi_nr_num_layers_t;

typedef struct {
	uint16_t pm_idx;       //0~65535
  nfapi_nr_num_layers_t* num_layers_list;   //0~65535
  //nfapi_nr_num_ant_ports_t* num_ant_ports_list;
} nfapi_nr_pm_pdu_t;

// Section 3.4

#endif
