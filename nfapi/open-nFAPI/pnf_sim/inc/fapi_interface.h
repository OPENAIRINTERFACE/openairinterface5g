/*
 * Copyright 2017 Cisco Systems, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _FAPI_INTERFACE_H_
#define _FAPI_INTERFACE_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef signed char		int8_t;
typedef unsigned char	uint8_t;
typedef signed short	int16_t;
typedef unsigned short	uint16_t;
typedef signed int		int32_t;
typedef unsigned int	uint32_t;

#define FAPI_PARAM_REQUEST									0x00
#define FAPI_PARAM_RESPONSE									0x01
#define FAPI_CONFIG_REQUEST									0x02
#define FAPI_CONFIG_RESPONSE								0x03
#define FAPI_START_REQUEST									0x04
#define FAPI_STOP_REQUEST									0x05
#define FAPI_STOP_INDICATION								0x06
#define FAPI_UE_CONFIG_REQUEST								0x07
#define FAPI_UE_CONFIG_RESPONSE								0x08
#define FAPI_ERROR_INDICATION								0x09
#define FAPI_UE_RELEASE_REQUEST								0x0A
#define FAPI_UE_RELEASE_RESPONSE							0x0B
#define FAPI_DL_CONFIG_REQUEST								0x80
#define FAPI_UL_CONFIG_REQUEST								0x81
#define FAPI_SUBFRAME_INDICATION							0x82
#define FAPI_HI_DCI0_REQUEST								0x83
#define FAPI_TX_REQUEST										0x84
#define FAPI_HARQ_INDICATION								0x85
#define FAPI_CRC_INDICATION									0x86
#define FAPI_RX_ULSCH_INDICATION							0x87
#define FAPI_RACH_INDICATION								0x88
#define FAPI_SRS_INDICATION									0x89
#define FAPI_RX_SR_INDICATION								0x8A
#define FAPI_RX_CQI_INDICATION								0x8B
#define FAPI_LBT_DL_CONFIG_REQUEST							0x8C
#define FAPI_LBT_DL_INDICATION								0x8D
#define FAPI_NB_HARQ_INDICATION								0x8E
#define FAPI_NRACH_INDICATION								0x8F




#define FAPI_SUBFRAME_DUPLEX_MODE_TAG						0x01
#define FAPI_SUBFRAME_PCFICH_POWER_OFFSET_TAG				0x02
#define FAPI_SUBFRAME_PB_TAG								0x03
#define FAPI_SUBFRAME_DL_CYCLIC_PREFIX_TYPE_TAG				0x04
#define FAPI_SUBFRAME_UL_CYCLIC_PREFIX_TYPE_TAG				0x05
#define FAPI_RF_DL_CHANNEL_BANDWIDTH_TAG					0x0A
#define FAPI_RF_UL_CHANNEL_BANDWIDTH_TAG					0x0B
#define FAPI_RF_REFERENCE_SIGNAL_POWER_TAG					0x0C
#define FAPI_RF_TX_ANTENNA_PORTS_TAG						0x0D
#define FAPI_RF_RX_ANTENNA_PORTS_TAG						0x0E
#define FAPI_PHICH_RESOURCE_TAG								0x14
#define FAPI_PHICH_DURATION_TAG								0x15
#define FAPI_PHICH_POWER_OFFSET_TAG							0x16
#define FAPI_SCH_PRIMARY_SYNC_SIGNAL_TAG					0x1E
#define FAPI_SCH_SECONDARY_SYNC_SIGNAL_TAG					0x1F
#define FAPI_SCH_PHYSICAL_CELL_ID_TAG						0x20
#define FAPI_PRACH_CONFIGURATION_INDEX_TAG					0x28
#define FAPI_PRACH_ROOT_SEQUENCE_INDEX_TAG					0x29
#define FAPI_PRACH_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG	0x2A
#define FAPI_PRACH_HIGH_SPEED_FLAG_TAG						0x2B
#define FAPI_PRACH_FREQUENCY_OFFSET_TAG						0x2C
#define FAPI_PUSCH_HOPPING_MODE_TAG							0x32
#define FAPI_PUSCH_HOPPING_OFFSET_TAG						0x33
#define FAPI_PUSCH_NUMBER_OF_SUBBANDS_TAG					0x34
#define FAPI_PUCCH_DELTA_PUCCH_SHIFT_TAG					0x3C
#define FAPI_PUCCH_N_CQI_RB_TAG								0x3D
#define FAPI_PUCCH_N_AN_CS_TAG								0x3E
#define FAPI_PUCCH_N1_PUCCH_AN_TAG							0x3F
#define FAPI_SRS_BANDWIDTH_CONFIGURATION_TAG				0x46
#define FAPI_SRS_MAX_UP_PTS_TAG								0x47
#define FAPI_SRS_SUBFRAME_CONFIGURATION_TAG					0x48
#define FAPI_SRS_ACK_NACK_SRS_SIMULTANEOUS_TX_TAG			0x49
#define FAPI_UL_REF_SIG_UPLINK_RS_HOPPING_TAG				0x50
#define FAPI_UL_REF_SIG_GROUP_ASSIGNMENT_TAG				0x51
#define FAPI_UL_REF_SIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG			0x52
#define FAPI_TDD_SUBFRAME_ASSIGNMENT_TAG					0x5A
#define FAPI_TDD_SPECIAL_SUBFRAME_PATTERNS_TAG				0x5B

#define FAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_PDSCH_TAG	0x64
#define FAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_DRS_TAG	0x65
#define FAPI_LAA_CONFIG_PD_THRESHOLD_TAG					0x66
#define FAPI_LAA_CONFIG_MULTI_CARRIER_TYPE_TAG				0x67
#define FAPI_LAA_CONFIG_MULTI_CARRIER_TX_TAG				0x68
#define FAPI_LAA_CONFIG_MULTI_CARRIER_FREEZE_TAG			0x69
#define FAPI_LAA_CONFIG_TX_ANTENNA_PORTS_FOR_DRS_TAG		0x6A
#define FAPI_LAA_CONFIG_TRANSMISSION_POWER_FOR_DRS_TAG		0x6B

#define FAPI_EMTC_CONFIG_PBCH_REPETITIONS_ENABLED_R13_TAG						0x78
#define FAPI_EMTC_CONFIG_PRACH_CAT_M_ROOT_SEQUENCY_INDEX_TAG					0x79
#define FAPI_EMTC_CONFIG_PRACH_CAT_M_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG	0x7A
#define FAPI_EMTC_CONFIG_PRACH_CAT_M_HIGH_SPEED_FLAG_TAG						0x7B
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_ENABLE_TAG							0x7C
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_CONFIGURATION_INDEX_TAG				0x7D
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_FREQUENCY_OFFSET_TAG					0x7E
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG	0x7F
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_STARTING_SUBFRAME_PERIODICITY_TAG		0x80
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_ENABLE_TAG					0x81
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_OFFSET_TAG					0x82
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_ENABLE_TAG							0x83
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_CONFIGURATION_INDEX_TAG				0x84
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_FREQUENCY_OFFSET_TAG					0x85
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG	0x86
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_STARTING_SUBFRAME_PERIODICITY_TAG		0x87
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_ENABLE_TAG					0x88
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_OFFSET_TAG					0x89
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_ENABLE_TAG							0x8A
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_CONFIGURATION_INDEX_TAG				0x8B
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_FREQUENCY_OFFSET_TAG					0x8C
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG	0x8D
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_STARTING_SUBFRAME_PERIODICITY_TAG		0x8E
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_ENABLE_TAG					0x8F
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_OFFSET_TAG					0x90
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_ENABLE_TAG							0x91
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_CONFIGURATION_INDEX_TAG				0x92
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_FREQUENCY_OFFSET_TAG					0x93
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG	0x94
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_STARTING_SUBFRAME_PERIODICITY_TAG		0x95
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_ENABLE_TAG					0x96
#define FAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_OFFSET_TAG					0x97
#define FAPI_EMTC_CONFIG_PRACH_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEA_TAG	0x98
#define FAPI_EMTC_CONFIG_PRACH_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEB_TAG	0x99

#define FAPI_NB_IOT_OPERATING_MODE_TAG											0xA5
#define FAPI_NB_IOT_ANCHOR_TAG													0xA6
#define FAPI_NB_IOT_PRB_INDEX_TAG												0xA7
#define FAPI_NB_IOT_CONTROL_REGION_SIZE_TAG										0xA8
#define FAPI_NB_IOT_ASSUMED_CRS_APS_TAG											0xA9
#define FAPI_NB_IOT_NPRACH_CONFIG_0_ENABLE_TAG									0xAA
#define FAPI_NB_IOT_NPRACH_CONFIG_0_SF_PERIODCITY_TAG							0xAB
#define FAPI_NB_IOT_NPRACH_CONFIG_0_START_TIME_TAG								0xAC
#define FAPI_NB_IOT_NPRACH_CONFIG_0_SUBCARRIER_OFFSET_TAG						0xAD
#define FAPI_NB_IOT_NPRACH_CONFIG_0_NUMBER_OF_SUBCARRIERS_TAG					0xAE
#define FAPI_NB_IOT_NPRACH_CONFIG_0_CP_LENGTH_TAG								0xAD
#define FAPI_NB_IOT_NPRACH_CONFIG_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG		0xB0
#define FAPI_NB_IOT_NPRACH_CONFIG_1_ENABLE_TAG									0xB1
#define FAPI_NB_IOT_NPRACH_CONFIG_1_SF_PERIODCITY_TAG							0xB2
#define FAPI_NB_IOT_NPRACH_CONFIG_1_START_TIME_TAG								0xB3
#define FAPI_NB_IOT_NPRACH_CONFIG_1_SUBCARRIER_OFFSET_TAG						0xB4
#define FAPI_NB_IOT_NPRACH_CONFIG_1_NUMBER_OF_SUBCARRIERS_TAG					0xB5
#define FAPI_NB_IOT_NPRACH_CONFIG_1_CP_LENGTH_TAG								0xB6
#define FAPI_NB_IOT_NPRACH_CONFIG_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG		0xB7
#define FAPI_NB_IOT_NPRACH_CONFIG_2_ENABLE_TAG									0xB8
#define FAPI_NB_IOT_NPRACH_CONFIG_2_SF_PERIODCITY_TAG							0xB9
#define FAPI_NB_IOT_NPRACH_CONFIG_2_START_TIME_TAG								0xBA
#define FAPI_NB_IOT_NPRACH_CONFIG_2_SUBCARRIER_OFFSET_TAG						0xBB
#define FAPI_NB_IOT_NPRACH_CONFIG_2_NUMBER_OF_SUBCARRIERS_TAG					0xBC
#define FAPI_NB_IOT_NPRACH_CONFIG_2_CP_LENGTH_TAG								0xBD
#define FAPI_NB_IOT_NPRACH_CONFIG_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG		0xBE

#define FAPI_NB_IOT_THREE_TONE_BASE_SEQUENCE_TAG			0xBF
#define FAPI_NB_IOT_SIX_TONE_BASE_SEQUENCE_TAG				0xC0
#define FAPI_NB_IOT_TWELVE_TONE_BASE_SEQUENCE_TAG			0xC1
#define FAPI_NB_IOT_THREE_TONE_CYCLIC_SHIFT_TAG				0xC2
#define FAPI_NB_IOT_SIX_TONE_CYCLIC_SHIFT_TAG				0xC3
#define FAPI_NB_IOT_DL_GAP_CONFIG_ENABLE_TAG				0xC4
#define FAPI_NB_IOT_DL_GAP_THRESHOLD_TAG					0xC5
#define FAPI_NB_IOT_DL_GAP_PERIODICITY_TAG					0xC6
#define FAPI_NB_IOT_DL_GAP_DURATION_COEFFICIENT_TAG			0xC7

//...
#define FAPI_PHY_CAPABILITIES_DL_BANDWIDTH_SUPPORT_TAG		0xC8
#define FAPI_PHY_CAPABILITIES_UL_BANDWIDTH_SUPPORT_TAG		0xC9
#define FAPI_PHY_CAPABILITIES_DL_MODULATION_SUPPORT_TAG		0xCA
#define FAPI_PHY_CAPABILITIES_UL_MODULATION_SUPPORT_TAG		0xCB
#define FAPI_PHY_CAPABILITIES_PHY_ANTENNA_CAPABILITY_TAG	0xCC
#define FAPI_PHY_CAPABILITIES_RELEASE_CAPABILITY_TAG		0xCD
#define FAPI_PHY_CAPABILITIES_MBSFN_CAPABILITY_TAG			0xCE
#define FAPI_PHY_CAPABILITIES_LAA_SUPPORT_TAG				0xD1
#define FAPI_PHY_CAPABILITIES_LAA_PD_SENSING_LBT_SUPPORT_TAG 0xD2
#define FAPI_PHY_CAPABILITIES_LAA_MULTI_CARRIER_LBT_SUPPORT_TAG	0xD3
#define FAPI_PHY_CAPABILITIES_LAA_PARTIAL_SF_SUPPORT_TAG	0xD4
#define FAPI_PHY_CAPABILITIES_NB_IOT_SUPPORT_TAG			0xD5
#define FAPI_PHY_CAPABILITIES_NB_IOT_OPERATING_MODE_CAPABILITY_TAG 0xD6

#define FAPI_PHY_CAPABILITIES_DATA_REPORT_MODE_TAG			0xF0
#define FAPI_PHY_SFN_SF_TAG									0xF1
#define FAPI_PHY_STATE_TAG									0xFA

#define FAPI_MSG_OK											0x0
#define FAPI_MSG_INVALID_STATE								0x1
#define FAPI_MSG_INVALID_CONFIG								0x2
#define FAPI_MSG_SFN_OUT_OF_SYNC							0x3
#define FAPI_MSG_SUBFRAME_ERR								0x4
#define FAPI_MSG_BCH_MISSING								0x5
#define FAPI_MSG_INVALID_SFN								0x6
#define FAPI_MSG_HI_ERR										0x7
#define FAPI_MSG_TX_ERR										0x8

#define FAPI_MSG_LBT_NO_PDU_IN_DL_REQ						0x9
#define FAPI_MSG_LBT_NO_VALID_CONFIG_REQ_RECIEVED			0xA
#define FAPI_MSG_FAPI_E_LBT_SF_SFN_PASSED_END_SF_SFN		0xB
#define FAPI_MSG_FAPI_E_LBT_OVERLAP							0xC
#define FAPI_MSG_BCH_PRESENT								0xD
#define FAPI_MSG_NBIOT_UNEXPECTED_REQ						0xE


// TODO : Work out what the correct maximums should be

// Number of UL/DL configurations, I, as defined by 36.212 section 5.3.3.1.4
// todo : work out what the max is
#define FAPI_MAX_UL_DL_CONFIGURATIONS						4
#define FAPI_MAX_NUM_PHYSICAL_ANTENNAS						4
#define FAPI_MAX_NUM_SCHEDULED_UES							8
#define FAPI_MAX_NUM_SUBBANDS								8
#define FAPI_MAX_ANTENNA_PORT_COUNT							2
	
	
typedef struct {
	uint8_t message_id;
	uint16_t length;
} fapi_header_t;

typedef struct {
	uint8_t tag;
	uint8_t length;
	uint16_t value;
} fapi_tlv_t;

typedef struct {
	fapi_header_t header;
} fapi_param_req_t;

typedef struct {
	fapi_header_t header;
	uint8_t error_code;
	uint8_t number_of_tlvs;
	fapi_tlv_t tlvs[255];
} fapi_param_resp_t;

typedef struct {
	fapi_header_t header;
	uint8_t number_of_tlvs;
	fapi_tlv_t tlvs[255];
} fapi_config_req_t;

typedef struct {
	fapi_header_t header;
	uint8_t error_code;
	uint8_t number_of_invalid_tlvs;
	uint8_t number_of_missing_tlvs;
	fapi_tlv_t tlvs[255];
} fapi_config_resp_t;

typedef struct {
	fapi_header_t header;
} fapi_start_req_t;

typedef struct {
	fapi_header_t header;
} fapi_stop_req_t;

typedef struct {
	fapi_header_t header;
} fapi_stop_ind_t;

typedef struct {
	uint16_t received_sfn_sf;
	uint16_t expected_sfn_sf;
} fapi_sfn_out_of_sync_error_ind_t;

typedef struct {
	uint16_t received_sfn_sf;
	uint16_t expected_sfn_sf;
} fapi_invalid_sfn_error_ind_t;

typedef struct {
	uint8_t sub_error_code;
	uint8_t direction;
	uint16_t rnti;
	uint8_t pdu_type;
} fapi_pdu_error_ind_t;

typedef struct {
	uint8_t sub_error_code;
	uint8_t phich_lowest_ul_rb_index;
} fapi_hi_error_ind_t;

typedef struct {
	uint8_t sub_error_code;
	uint8_t pdu_index;
} fapi_tx_error_ind_t;	

typedef struct {
	fapi_header_t header;
	uint8_t message_id;
	uint8_t error_code;
	union 
	{
		fapi_sfn_out_of_sync_error_ind_t	out_of_sync_err;
		fapi_invalid_sfn_error_ind_t		invalid_sfn_err;
		fapi_pdu_error_ind_t				pdu_err;
		fapi_hi_error_ind_t					hi_err;
		fapi_tx_error_ind_t					tx_err;	
	};
} fapi_error_ind_t;

typedef struct {
	uint32_t handle;
	uint16_t rnti;
} fapi_rx_ue_information_t;

typedef struct {
	uint8_t ul_cqi;
	uint8_t channel;
} fapi_ul_cqi_information_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
} fapi_subframe_ind_t;

typedef struct { 
	uint8_t value_0;
	uint8_t value_1;
} fapi_harq_ind_rel8_tdd_harq_data_bundling_t;

typedef struct { 
	uint8_t value_0;
	uint8_t value_1;
	uint8_t value_2;
	uint8_t value_3;
} fapi_harq_ind_rel8_tdd_harq_data_multiplexing_t;

typedef struct { 
	uint8_t value_0;
} fapi_harq_ind_rel8_tdd_harq_data_special_bundling_t;

typedef struct { 
	uint8_t mode;
	uint8_t number_of_ack_nack;
	union{
		fapi_harq_ind_rel8_tdd_harq_data_bundling_t			bundling;
		fapi_harq_ind_rel8_tdd_harq_data_multiplexing_t		multiplex;
		fapi_harq_ind_rel8_tdd_harq_data_special_bundling_t	special_bundling;
	} harq_data;
} fapi_harq_ind_rel8_tdd_pdu_t;

typedef struct { 
	uint8_t value_0;
} fapi_harq_ind_rel8_tdd_harq_data_t;


typedef struct {
	uint8_t mode;
	uint8_t number_of_ack_nack;
	union{
		fapi_harq_ind_rel8_tdd_harq_data_t	bundling;
		fapi_harq_ind_rel8_tdd_harq_data_t	multiplex;
		fapi_harq_ind_rel8_tdd_harq_data_special_bundling_t	special_bundling;
		fapi_harq_ind_rel8_tdd_harq_data_t	channel_selection;
		fapi_harq_ind_rel8_tdd_harq_data_t	format_3;
	} harq_data[21];
} fapi_harq_ind_rel9_tdd_pdu_t;

typedef struct {
	uint8_t mode;
	uint16_t number_of_ack_nack;
	union{
		fapi_harq_ind_rel8_tdd_harq_data_t					bundling;
		fapi_harq_ind_rel8_tdd_harq_data_t					multiplex;
		fapi_harq_ind_rel8_tdd_harq_data_special_bundling_t	special_bundling;
		fapi_harq_ind_rel8_tdd_harq_data_t					channel_selection;
		fapi_harq_ind_rel8_tdd_harq_data_t			format_3;
		fapi_harq_ind_rel8_tdd_harq_data_t			format_4;
		fapi_harq_ind_rel8_tdd_harq_data_t			format_5;
	} harq_data[22];
} fapi_harq_ind_rel13_tdd_pdu_t;

typedef struct {
	uint8_t harq_tb_1;
	uint8_t harq_tb_2;
} fapi_harq_ind_rel8_fdd_pdu_t;

typedef struct {
	uint8_t mode;
	uint8_t number_of_ack_nack;
	uint8_t harq_tb_n[10];
} fapi_harq_ind_rel9_fdd_pdu_t;

typedef struct {
	uint8_t mode;
	uint16_t number_of_ack_nack;
	uint8_t harq_tb_n[22];
} fapi_harq_ind_rel13_fdd_pdu_t;

typedef struct {
	fapi_rx_ue_information_t rx_ue_info;
	union {
		fapi_harq_ind_rel8_tdd_pdu_t	rel8_tdd_pdu;
		fapi_harq_ind_rel9_tdd_pdu_t	rel9_tdd_pdu;
		fapi_harq_ind_rel13_tdd_pdu_t	rel13_tdd_pdu;
		fapi_harq_ind_rel8_fdd_pdu_t	rel8_fdd_pdu;
		fapi_harq_ind_rel9_fdd_pdu_t	rel9_fdd_pdu;
		fapi_harq_ind_rel13_fdd_pdu_t	rel13_fdd_pdu;
	};
	fapi_ul_cqi_information_t ul_cqi_info;
} fapi_harq_ind_pdu_t;

typedef struct {
	uint16_t number_of_harqs;
	fapi_harq_ind_pdu_t pdus[32];
} fapi_harq_ind_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_harq_ind_body_t body;
} fapi_harq_ind_t;

typedef struct {
	uint8_t crc_flag;
} fapi_crc_ind_rel8_pdu_t;

typedef struct {
	fapi_rx_ue_information_t rx_ue_info;
	fapi_crc_ind_rel8_pdu_t rel8_pdu;
} fapi_crc_ind_pdu_t;

typedef struct {
	uint16_t number_of_crcs;
	fapi_crc_ind_pdu_t pdus[32];
} fapi_crc_ind_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_crc_ind_body_t body;
} fapi_crc_ind_t;

typedef struct {
	uint16_t length;
	uint16_t data_offset;
	uint8_t ul_cqi;
	uint16_t timing_advance;
} fapi_rx_ulsch_ind_rel8_pdu_t;

typedef struct {
	uint16_t timing_advance_r9;
} fapi_rx_ulsch_ind_rel9_pdu_t;

typedef struct {
	fapi_rx_ue_information_t rx_ue_info;
	fapi_rx_ulsch_ind_rel8_pdu_t rel8_pdu;
	fapi_rx_ulsch_ind_rel9_pdu_t rel9_pdu;
} fapi_rx_ulsch_ind_pdu_t;

typedef struct {
	uint16_t number_of_pdus;
	fapi_rx_ulsch_ind_pdu_t pdus[32];
	void* data[32];
} fapi_rx_ulsch_ind_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_rx_ulsch_ind_body_t body;
} fapi_rx_ulsch_ind_t;

typedef struct {
	uint16_t length;
	uint16_t data_offset;
	uint8_t ul_cqi;
	uint8_t ri;
	uint16_t timing_advance;
} fapi_rx_cqi_ind_rel8_pdu_t;

typedef struct {
	uint16_t length;
	uint16_t data_offset;
	uint8_t ul_cqi;
	uint8_t number_of_cc_reported;
	uint8_t ri[5];
	uint16_t timing_advance;
	uint16_t timing_advance_r9;
} fapi_rx_cqi_ind_rel9_pdu_t;

typedef struct {
	fapi_rx_ue_information_t rx_ue_info;
	fapi_rx_cqi_ind_rel8_pdu_t rel8_pdu;
	fapi_rx_cqi_ind_rel9_pdu_t rel9_pdu;
	fapi_ul_cqi_information_t ul_cqi_info;
} fapi_rx_cqi_ind_pdu_t;

typedef struct {
	uint16_t number_of_pdus;
	fapi_rx_cqi_ind_pdu_t pdus[32];
	uint8_t* data[32];
} fapi_rx_cqi_ind_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_rx_cqi_ind_body_t body;
} fapi_rx_cqi_ind_t;

typedef struct {
	fapi_rx_ue_information_t rx_ue_info;
	fapi_ul_cqi_information_t ul_cqi_info;
} fapi_rx_sr_ind_pdu_t;

typedef struct {
	uint16_t number_of_srs;
	fapi_rx_sr_ind_pdu_t pdus[32];
} fapi_rx_sr_ind_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_rx_sr_ind_body_t body;
} fapi_rx_sr_ind_t;

typedef struct {
	uint16_t rnti;
	uint8_t preamble;
	uint16_t timing_advance;
} fapi_rach_ind_pdu_rel8_t;

typedef struct {
	uint16_t timing_advance_r9;
} fapi_rach_ind_pdu_rel9_t;

typedef struct {
	uint8_t rach_resource_type;
} fapi_rach_ind_pdu_rel13_t;

typedef struct {
	fapi_rach_ind_pdu_rel8_t rel8_pdu;
	fapi_rach_ind_pdu_rel9_t rel9_pdu;
	fapi_rach_ind_pdu_rel13_t rel13_pdu;
} fapi_rach_ind_pdu_t;

typedef struct {
	uint8_t number_of_preambles;
	fapi_rach_ind_pdu_t pdus[32];
} fapi_rach_ind_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_rach_ind_body_t body;
} fapi_rach_ind_t;


typedef struct {
	uint16_t doppler_estimation;
	uint16_t timing_advance;
	uint8_t number_of_resource_blocks;
	uint8_t rb_start;
	uint8_t snr[100];
} fapi_srs_ind_rel8_pdu_t;

typedef struct { 
	uint16_t timing_advance_r9;
} fapi_srs_ind_rel9_pdu_t;

typedef struct { 
	uint8_t uppts_symbol;
} fapi_srs_ind_rel10_tdd_pdu_t;

typedef struct {
	uint16_t ul_rtoa;
} fapi_srs_ind_rel11_pdu_t;

typedef struct {
	uint8_t num_prb_per_subband;
	uint8_t number_of_subbands;
	uint8_t num_atennas;
	struct {
		uint8_t subband_index;
		uint16_t channel[FAPI_MAX_NUM_PHYSICAL_ANTENNAS];
	} subands[FAPI_MAX_NUM_SUBBANDS];
} fapi_ttd_channel_measurement_t;

typedef struct {
	fapi_rx_ue_information_t		rx_ue_info;
	fapi_srs_ind_rel8_pdu_t 		rel8_pdu;
	fapi_srs_ind_rel9_pdu_t 		rel9_pdu;
	fapi_srs_ind_rel10_tdd_pdu_t	rel10_tdd_pdu;
	fapi_srs_ind_rel11_pdu_t		rel11_pdu;
	fapi_ttd_channel_measurement_t tdd_channel_measurement;
} fapi_srs_ind_pdu_t;

typedef struct {
	uint8_t number_of_ues;
	fapi_srs_ind_pdu_t pdus[32];
} fapi_srs_ind_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_srs_ind_body_t body;
} fapi_srs_ind_t;


typedef struct {
	uint32_t handle;
	uint32_t result;
	uint32_t lte_txop_symbols;
	uint32_t initial_partial_sf;
} fapi_lbt_dl_pdsch_rsp_rel13_pdu_t;

typedef struct {
	fapi_lbt_dl_pdsch_rsp_rel13_pdu_t	rel13_pdu;
} fapi_lbt_dl_pdsch_rsp_pdu_t;

typedef struct {
	uint32_t handle;
	uint32_t result;
} fapi_lbt_dl_drs_rsp_rel13_pdu_t;

typedef struct {
	fapi_lbt_dl_drs_rsp_rel13_pdu_t 	rel13_pdu;
} fapi_lbt_dl_drs_rsp_pdu_t;

typedef struct {
	uint8_t pdu_type;
	uint8_t pdu_size;
	union {
		fapi_lbt_dl_pdsch_rsp_pdu_t		lbt_pdsch_rsp_pdu;
		fapi_lbt_dl_drs_rsp_pdu_t		lbt_drs_rsp_pdu;
	};
} fapi_lbt_dl_ind_pdu_t;

typedef struct {
	uint16_t number_of_pdus;
	fapi_lbt_dl_ind_pdu_t pdus[32];
} fapi_lbt_dl_ind_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_lbt_dl_ind_body_t body;
} fapi_lbt_dl_ind_t;

typedef struct {
	uint8_t harq_tb1;
} fapi_nb_harq_ind_rel13_pdu_t;

typedef struct {
	fapi_rx_ue_information_t rx_ue_info;
	union {
		fapi_nb_harq_ind_rel13_pdu_t	rel13_pdu;
	};
	fapi_ul_cqi_information_t ul_cqi_info;
} fapi_nb_harq_ind_pdu_t;

typedef struct {
	uint16_t number_of_harqs;
	fapi_nb_harq_ind_pdu_t pdus[32];
} fapi_nb_harq_ind_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_nb_harq_ind_body_t body;
} fapi_nb_harq_ind_t;

typedef struct {
	uint16_t rnti;
	uint8_t initial_sc;
	uint16_t timing_advance;
	uint8_t nrach_cs_level;
} fapi_nrach_ind_rel13_pdu_t;

typedef struct {
	fapi_rx_ue_information_t rx_ue_info;
	union {
		fapi_nrach_ind_rel13_pdu_t	rel13_pdu;
	};
	fapi_ul_cqi_information_t ul_cqi_info;
} fapi_nrach_ind_pdu_t;

typedef struct {
	uint16_t number_of_initial_scs_detected;
	fapi_nrach_ind_pdu_t pdus[32];
} fapi_nrach_ind_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_nrach_ind_body_t body;
} fapi_nrach_ind_t;

//------------------------------------------------------------------------------

typedef struct {
	uint8_t dci_format;
	uint8_t cce_index;
	uint8_t aggregation_level;
	uint16_t rnti;
	uint8_t resource_allocation_type;
	uint8_t virtual_resource_block_assignment_flag;
	uint32_t resource_block_coding;
	uint8_t mcs_1;
	uint8_t redundancy_version_1;
	uint8_t new_data_indicator_1;
	uint8_t transport_block_to_codeword_swap_flag;
	uint8_t mcs_2;
	uint8_t redundancy_version_2;
	uint8_t new_data_indictor_2;
	uint8_t harq_process;
	uint8_t tpmi;
	uint8_t pmi;
	uint8_t precoding_information;
	uint8_t tpc;
	uint8_t downlink_assignment_index;
	uint8_t n_gap;
	uint8_t transport_block_size_index;
	uint8_t downlink_power_offset;
	uint8_t allocate_prach_flag;
	uint8_t preamble_index;
	uint8_t prach_mask_index;
	uint8_t rnti_type;
	uint16_t transmission_power;
} fapi_dl_config_dci_rel8_pdu_t;

typedef struct {
	uint8_t mcch_flag;
	uint8_t mcch_change_notification;
	uint8_t scrambling_identity;
} fapi_dl_config_dci_rel9_pdu_t;

typedef struct {
	uint8_t cross_carrier_scheduling_flag;
	uint8_t carrier_indicator;
	uint8_t srs_flag;
	uint8_t srs_request;
	uint8_t antenna_ports_scrambling_and_layers;
	uint8_t total_dci_length_including_padding;
	uint8_t n_dl_rb;
} fapi_dl_config_dci_rel10_pdu_t;

typedef struct {
	uint8_t harq_ack_resource_offset;
	uint8_t pdsch_re_mapping_quasi_co_location_indicator;
} fapi_dl_config_dci_rel11_pdu_t;

typedef struct {
	uint8_t primary_cell_type;
	uint8_t ul_dl_configuration_flag;
	uint8_t number_ul_dl_configurations;
	uint8_t ul_dl_configuration_indication[FAPI_MAX_UL_DL_CONFIGURATIONS];
} fapi_dl_config_dci_rel12_pdu_t;


typedef struct {
	uint8_t subband_index;
	uint8_t scheduled_ues;
	uint16_t precoding_value[FAPI_MAX_NUM_PHYSICAL_ANTENNAS][FAPI_MAX_NUM_SCHEDULED_UES];
} fapi_dl_config_dci_dl_tpm_subband_info_t;

typedef struct {
	uint8_t num_prb_per_subband;
	uint8_t number_of_subbands;
	uint8_t num_antennas;
	fapi_dl_config_dci_dl_tpm_subband_info_t subband_info[FAPI_MAX_NUM_SUBBANDS];
} fapi_dl_config_dci_dl_tpm_t;

typedef struct {
	uint8_t laa_end_partial_sf_flag;
	uint8_t laa_end_partial_sf_configuration;
	uint8_t initial_lbt_sf;
	uint8_t codebook_size_determination;
	uint8_t drms_table_flag;
	uint8_t tpm_struct_flag;
	fapi_dl_config_dci_dl_tpm_t tpm;
} fapi_dl_config_dci_rel13_pdu_t;

typedef struct {
	fapi_dl_config_dci_rel8_pdu_t 	rel8_pdu;
	fapi_dl_config_dci_rel9_pdu_t 	rel9_pdu;
	fapi_dl_config_dci_rel10_pdu_t	rel10_pdu;
	fapi_dl_config_dci_rel11_pdu_t	rel11_pdu;
	fapi_dl_config_dci_rel12_pdu_t	rel12_pdu;
	fapi_dl_config_dci_rel13_pdu_t	rel13_pdu;
} fapi_dl_config_dci_pdu_t;

typedef struct {
	uint16_t length;
	uint16_t pdu_index;
	uint16_t transmission_power;
} fapi_dl_config_bch_rel8_pdu_t;

typedef struct {
	fapi_dl_config_bch_rel8_pdu_t rel8_pdu;
} fapi_dl_config_bch_pdu_t;

typedef struct {
	uint16_t length;
	uint16_t pdu_index;
	uint16_t rnti;
	uint8_t resource_allocation_type;
	uint32_t resource_block_coding;
	uint8_t modulation;
	uint16_t transmission_power;
	uint16_t mbsfn_area_id;
} fapi_dl_config_mch_rel8_pdu_t;

typedef struct {
	fapi_dl_config_mch_rel8_pdu_t rel8_pdu;
} fapi_dl_config_mch_pdu_t;

typedef struct {
	uint8_t subband_index;
	uint8_t num_atennas;
	uint16_t bf_value[8];
} fapi_dl_config_bf_vector_t;

typedef struct {
	uint16_t length;
	uint16_t pdu_index;
	uint16_t rnti;
	uint8_t resource_allocation_type;
	uint8_t virtual_resource_block_assignment_flag;
	uint32_t resource_block_coding;
	uint8_t modulation;
	uint8_t redundacy_version;
	uint8_t transport_blocks;
	uint8_t transport_block_codeword_swap_flag;
	uint8_t transmission_scheme;
	uint8_t number_of_layers;
	uint8_t nunber_of_subands;
	uint8_t codebook_index[13];
	uint8_t ue_category_capacity;
	uint8_t p_a;
	uint8_t delta_power_offset_index;
	uint8_t n_gap;
	uint8_t n_prb;
	uint8_t transmission_mode;
	uint8_t num_bf_prb_per_subband;
	uint8_t num_bf_vecitor;
	fapi_dl_config_bf_vector_t bf_vector[8];
} fapi_dl_config_dlsch_rel8_pdu_t;

typedef struct {
	uint8_t nscid;
} fapi_dl_config_dlsch_rel9_pdu_t;

typedef struct {
	uint8_t csi_rs_flag;
	uint8_t csi_rs_resource_config_r10;
	uint16_t csi_rs_zero_tx_power_resource_config_bitmap_r10;
	uint8_t csi_rs_number_nzp_configuration;
	uint8_t csi_rs_resource_config[4];
	uint8_t pdsch_start;
} fapi_dl_config_dlsch_rel10_pdu_t;

typedef struct {
	uint8_t drms_config_flag;
	uint16_t drms_scrambling;
	uint8_t csi_config_flag;
	uint16_t csi_scrambling;
	uint8_t pdsch_re_mapping_flag;
	uint8_t pdsch_re_mapping_atenna_ports;
	uint8_t pdsch_re_mapping_freq_shift;
} fapi_dl_config_dlsch_rel11_pdu_t;

typedef struct {
	uint8_t altcqi_table_r12;
	uint8_t maxlayers;
	uint8_t n_dl_harq;
} fapi_dl_config_dlsch_rel12_pdu_t;

typedef struct {
	uint8_t dwpts_symbols;
	uint8_t initial_lbt_sf;
	uint8_t ue_type;
	uint8_t pdsch_payload_type;
	uint16_t initial_transmission_sf_io;
	uint8_t drms_table_flag;
} fapi_dl_config_dlsch_rel13_pdu_t;

typedef struct {
	fapi_dl_config_dlsch_rel8_pdu_t rel8_pdu;
	fapi_dl_config_dlsch_rel9_pdu_t rel9_pdu;
	fapi_dl_config_dlsch_rel10_pdu_t rel10_pdu;
	fapi_dl_config_dlsch_rel11_pdu_t rel11_pdu;
	fapi_dl_config_dlsch_rel12_pdu_t rel12_pdu;
	fapi_dl_config_dlsch_rel13_pdu_t rel13_pdu;
} fapi_dl_config_dlsch_pdu_t;

typedef struct {
	uint16_t length;
	uint16_t pdu_index;
	uint16_t p_rnti;
	uint8_t resource_allocation_type;
	uint8_t virtual_resource_block_assignment_flag;
	uint32_t resource_block_coding;
	uint8_t mcs;
	uint8_t redudancy_version;
	uint8_t number_of_transport_blocks;
	uint8_t transport_block_to_codeword_swap_flag;
	uint8_t transmission_scheme;
	uint8_t number_of_layers;
	uint8_t codebook_index;
	uint8_t ue_category_capacity;
	uint8_t p_a;
	uint16_t transmission_power;
	uint8_t n_prb;
	uint8_t n_gap;
} fapi_dl_config_pch_rel8_pdu_t;

typedef struct {
	uint8_t ue_mode;
	uint16_t initial_transmission_sf_io;
} fapi_dl_config_pch_rel13_pdu_t;

typedef struct {
	fapi_dl_config_pch_rel8_pdu_t	rel8_pdu;
	fapi_dl_config_pch_rel13_pdu_t	rel13_pdu;
} fapi_dl_config_pch_pdu_t;

typedef struct {
	uint16_t transmission_power;
	uint8_t prs_bandwidth;
	uint8_t prs_cyclic_prefix_type;
	uint8_t prs_muting;
} fapi_dl_config_prs_rel9_pdu_t;

typedef struct {
	fapi_dl_config_prs_rel9_pdu_t	rel9_pdu;
} fapi_dl_config_prs_pdu_t;

typedef struct {
	uint8_t csi_rs_antenna_port_count_r10;
	uint8_t csi_rs_resource_config_r10;
	uint16_t transmission_power;
	uint16_t csi_rs_zero_tx_power_resource_config_bitmap_r10;
	uint8_t csi_rs_number_of_nzp_configuration;
	uint8_t csi_rs_resource_config[8];
} fapi_dl_config_csi_rs_rel10_pdu_t;

typedef struct {
	uint8_t csi_rs_class;
	uint8_t cdm_type;
	uint8_t num_bf_vector;
	struct {
		uint8_t csi_rs_resource_index;
		uint16_t bf_value[FAPI_MAX_ANTENNA_PORT_COUNT];
	} bf_vector[8];
}fapi_dl_config_csi_rs_rel13_pdu_t;

typedef struct {
	fapi_dl_config_csi_rs_rel10_pdu_t	rel10_pdu;
	fapi_dl_config_csi_rs_rel13_pdu_t	rel13_pdu;
} fapi_dl_config_csi_rs_pdu_t;

typedef struct {
	uint8_t edpcch_resource_assigenment_flag;
	uint16_t edpcch_id;
	uint8_t epdcch_start_symbol;
	uint8_t epdcch_num_prb;
	uint8_t epdcch_prb_index[8];
	fapi_dl_config_bf_vector_t bf_vector;
} fapi_dl_config_edpcch_rel11_params_t;

typedef struct {
	uint8_t dwpts_symbols;
	uint8_t initial_lbt_sf;
} fapi_dl_config_edpcch_rel13_params_t;

typedef struct {
	fapi_dl_config_dci_rel8_pdu_t			rel8_pdu;
	fapi_dl_config_dci_rel9_pdu_t			rel9_pdu;
	fapi_dl_config_dci_rel10_pdu_t			rel10_pdu;
	fapi_dl_config_dci_rel11_pdu_t			rel11_pdu;
	fapi_dl_config_dci_rel12_pdu_t			rel12_pdu;
	fapi_dl_config_dci_rel13_pdu_t			rel13_pdu;
	fapi_dl_config_edpcch_rel11_params_t	rel8_params;
	fapi_dl_config_edpcch_rel13_params_t	rel13_params;
} fapi_dl_config_edpcch_pdu_t;

typedef struct {
	uint8_t mpdcch_narrow_band;
	uint8_t number_of_prb_pairs;
	uint8_t resource_block_assignment;
	uint8_t mpdcch_tansmission_type;
	uint8_t start_symbol;
	uint8_t ecce_index;
	uint8_t aggregation_level;
	uint8_t rnti_type;
	uint16_t rnti;
	uint8_t ce_mode;
	uint16_t drms_scrambling_init;
	uint16_t initial_transmission_sf_io;
	uint16_t transmission_power;
	uint8_t dci_format;
	uint16_t resource_block_coding;
	uint8_t mcs;
	uint8_t pdsch_reptition_levels;
	uint8_t redundancy_version;
	uint8_t new_data_indicator;
	uint8_t harq_process;
	uint8_t tpmi_length;
	uint8_t tpmi;
	uint8_t pmi_flag;
	uint8_t pmi;
	uint8_t harq_resource_offset;
	uint8_t dci_subframe_repetition_number;
	uint8_t tpc;
	uint8_t downlink_assignment_index_length;
	uint8_t downlink_assignment_index;
	uint8_t allocate_prach_flag;
	uint8_t preamble_index;
	uint8_t prach_mask_index;
	uint8_t starting_ce_level;
	uint8_t srs_request;
	uint8_t antenna_ports_and_scrambling_identity_flag;
	uint8_t antenna_ports_and_scrambling_identity;
	uint8_t frequency_hopping_enabled_flag;
	uint8_t paging_direct_indication_differentiation_flag;
	uint8_t direct_indication;
	uint8_t total_dci_length_including_padding;
	uint8_t number_of_tx_antenna_ports;
	uint16_t precoding_value[8];
} fapi_dl_config_mdpcch_rel13_pdu_t;

typedef struct {
	fapi_dl_config_mdpcch_rel13_pdu_t	rel13_pdu;
} fapi_dl_config_mdpcch_pdu_t;

typedef struct {
	uint16_t length;
	uint16_t pdu_index;
	uint16_t transmission_power;
	uint16_t hyper_sfn_2_lsbs;
} fapi_dl_config_nbch_rel13_pdu_t;

typedef struct {
	fapi_dl_config_nbch_rel13_pdu_t		rel13_pdu;
} fapi_dl_config_nbch_pdu_t;

typedef struct {
	uint16_t length;
	uint16_t pdu_index;
	uint8_t ncce_index;
	uint8_t aggregation_level;
	uint8_t start_symbol;
	uint8_t rnti_type;
	uint16_t rnti;
	uint8_t scrambling_reinitialization_batch_index;
	uint8_t nrs_antenna_ports_assumed_by_the_ue;
	uint8_t dci_format;
	uint8_t scheduling_delay;
	uint8_t resource_assignment;
	uint8_t repetition_number;
	uint8_t mcs;
	uint8_t new_data_indicator;
	uint8_t harq_ack_resource;
	uint8_t npdcch_order_indication;
	uint8_t starting_number_of_nprach_repetitions;
	uint8_t subcarrier_indication_of_nprach;
	uint8_t paging_direct_indication_differentation_flag;
	uint8_t direct_indication;
	uint8_t dci_subframe_repetition_number;
	uint8_t total_dci_length_including_padding;
} fapi_dl_config_npdcch_rel13_pdu_t;

typedef struct {
	fapi_dl_config_npdcch_rel13_pdu_t	rel13_pdu;
} fapi_dl_config_npdcch_pdu_t;

typedef struct {
	uint16_t length;
	uint16_t pdu_index;
	uint8_t start_symbol;
	uint8_t rnti_type;
	uint16_t rnti;
	uint16_t resource_assignment;
	uint16_t repetition_number;
	uint8_t modulation;
	uint8_t number_of_subframes_for_resource_assignment;
	uint8_t scrambling_sequence_initialization_cinit;
	uint16_t sf_idx;
	uint8_t nrs_antenna_ports_assumed_by_the_ue;
} fapi_dl_config_ndlsch_rel13_pdu_t;

typedef struct {
	fapi_dl_config_ndlsch_rel13_pdu_t	rel13_pdu;
} fapi_dl_config_ndlsch_pdu_t;

typedef struct {
	uint8_t pdu_type;
	uint8_t pdu_size;
	union
	{
		fapi_dl_config_dci_pdu_t	dci_dl_pdu;
		fapi_dl_config_bch_pdu_t	bch_pdu;
		fapi_dl_config_mch_pdu_t	mch_pdu;
		fapi_dl_config_dlsch_pdu_t	dlsch_pdu;
		fapi_dl_config_pch_pdu_t	pch_pdu;
		fapi_dl_config_prs_pdu_t	prs_pdu;
		fapi_dl_config_csi_rs_pdu_t	csirs_pdu;
		fapi_dl_config_edpcch_pdu_t edpcch_pdu;
		fapi_dl_config_mdpcch_pdu_t mdpcch_pdu;
		fapi_dl_config_nbch_pdu_t   nbch_pdu;
		fapi_dl_config_npdcch_pdu_t npdcch_pdu;
		fapi_dl_config_ndlsch_pdu_t ndlsch_pdu;
	};
} fapi_dl_config_req_pdu_t;

typedef struct {
	uint8_t number_of_pdcch_ofdm_symbols;
	uint8_t number_of_dcis;
	uint16_t number_of_pdus;
	uint8_t number_of_pdsch_rntis;
	uint16_t transmission_power_for_pcfich;
	fapi_dl_config_req_pdu_t pdus[32];
} fapi_dl_config_req_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	uint16_t length;
	fapi_dl_config_req_body_t body;
} fapi_dl_config_req_t;

typedef struct {
	uint32_t handle;
	uint16_t size;
	uint16_t rnti;
	uint8_t resource_block_start;
	uint8_t number_of_resource_blocks;
	uint8_t modulation_type;
	uint8_t cyclic_shift_2_for_drms;
	uint8_t frequency_hopping_enabled_flag;
	uint8_t frequency_hopping_bits;
	uint8_t new_data_indication;
	uint8_t redundancy_version;
	uint8_t harq_process_number;
	uint8_t ul_tx_mode;
	uint8_t current_tx_nb;
	uint8_t n_srs;
} fapi_ul_config_req_ulsch_rel8_pdu_t;

typedef struct {
	uint8_t resource_allocation_type;
	uint32_t resource_block_coding;
	uint8_t transport_blocks;
	uint8_t transmission_scheme;
	uint8_t number_of_layers;
	uint8_t codebook_index;
	uint8_t disable_sequence_hopping_flag;
} fapi_ul_config_req_ulsch_rel10_pdu_t;

typedef struct {
	uint8_t virtual_cell_id_enabled_flag;
	uint16_t npusch_identity;
	uint8_t dmrs_config_flag;
	uint16_t ndmrs_csh_identity;
} fapi_ul_config_req_ulsch_rel11_pdu_t;

typedef struct {
	uint8_t  ue_type;
	uint16_t total_number_of_repetitions;
	uint16_t repetition_number;
	uint16_t initial_transmission_sf_io;
	uint8_t  empty_symbols_due_to_re_tunning;
} fapi_ul_config_req_ulsch_rel13_pdu_t;

typedef struct {
	fapi_ul_config_req_ulsch_rel8_pdu_t 	rel8_pdu;
	fapi_ul_config_req_ulsch_rel10_pdu_t	rel10_pdu;
	fapi_ul_config_req_ulsch_rel11_pdu_t	rel11_pdu;
	fapi_ul_config_req_ulsch_rel13_pdu_t	rel13_pdu;
} fapi_ul_config_req_ulsch_pdu_t;

typedef struct { 
	uint8_t dl_cqi_pmi_size_rank_1;
	uint8_t dl_cqi_pmi_size_rank_greater_1;
	uint8_t ri_size;
	uint8_t delta_offset_cqi;
	uint8_t delta_offset_ri;
} fapi_ul_config_cqi_ri_rel8_information_t;

typedef struct {
	uint8_t dl_cqi_pmi_ri_size;
	uint8_t control_type;
} fapi_ul_config_periodic_cqi_pmi_ri_report_t;

typedef struct {
	uint8_t number_of_cc;
	struct {
		uint8_t ri_size;
		uint8_t dl_cqi_pmi_size;
	} cc[5];
} fapi_ul_config_aperiodic_cqi_pmi_ri_report_t;

typedef struct {
	uint8_t report_type;
	uint8_t delta_offset_cqi;
	uint8_t delta_offset_ri;
	union {
		fapi_ul_config_periodic_cqi_pmi_ri_report_t periodic_cqi_pmi_ri_report;
		fapi_ul_config_aperiodic_cqi_pmi_ri_report_t aperiodic_cqi_pmi_ri_report;
	};
} fapi_ul_config_cqi_ri_rel9_information_t;

typedef struct {
	uint16_t dl_cqi_pmi_ri_size_2;
} fapi_ul_config_periodic_cqi_pmi_ri_report_re13_t;

typedef struct {
} fapi_ul_config_aperiodic_cqi_pmi_ri_report_re13_t;


typedef struct {
	union {
		fapi_ul_config_periodic_cqi_pmi_ri_report_re13_t periodic_cqi_pmi_ri_report;
		fapi_ul_config_aperiodic_cqi_pmi_ri_report_re13_t aperiodic_cqi_pmi_ri_report;
	};
} fapi_ul_config_cqi_ri_rel13_information_t;

typedef struct {
	fapi_ul_config_cqi_ri_rel8_information_t	rel8_info;
	fapi_ul_config_cqi_ri_rel9_information_t	rel9_cqi_ri_info;
	fapi_ul_config_cqi_ri_rel13_information_t	rel13_cqi_ri_info;
} fapi_ul_config_cqi_ri_information_t;

typedef struct { 
	uint8_t n_srs_initial;
	uint8_t initial_number_of_resource_blocks;
} fapi_ul_config_init_tx_rel8_params_t;

typedef struct {
	fapi_ul_config_init_tx_rel8_params_t rel8_params;
} fapi_ul_config_init_tx_params_t;

typedef struct { 
	uint8_t harq_size;
	uint8_t delta_offset_harq;
	uint8_t ack_nack_mode;
} fapi_ul_config_harq_rel10_information_t;

typedef struct {
	uint16_t harq_size_2;
	uint8_t delta_offset_harq_2;
} fapi_ul_config_harq_rel13_information_t;

typedef struct {
	fapi_ul_config_harq_rel10_information_t rel10_info;
	fapi_ul_config_harq_rel13_information_t rel13_info;
} fapi_ul_config_harq_information_t;

typedef struct { 
	uint32_t handle;
	uint16_t rnti;
} fapi_ul_config_req_ue_rel8_info_t;

typedef struct {
	uint8_t virtual_cell_id_enabled_flag;
	uint16_t npusch_identity;
} fapi_ul_config_req_ue_rel11_info_t;

typedef struct {
	uint8_t  ue_type;
	uint8_t  empty_symbols;
	uint16_t total_number_of_repetitions;
	uint16_t repetition_number;
} fapi_ul_config_req_ue_rel13_info_t;

typedef struct {
	fapi_ul_config_req_ue_rel8_info_t rel8_info;
	fapi_ul_config_req_ue_rel11_info_t rel11_info;
	fapi_ul_config_req_ue_rel13_info_t rel13_info;
} fapi_ul_config_req_ue_info_t;

typedef struct { 
	uint16_t pucch_index;
	uint8_t dl_cqi_pmi_size;
} fapi_ul_config_req_cqi_rel8_info_t;

typedef struct {
	uint8_t number_of_pucch_resource;
	uint16_t pucch_index_p1;
} fapi_ul_config_req_cqi_rel10_info_t;

typedef struct {
	uint8_t csi_mode;
	uint16_t dl_cqi_pmi_size_2;
	uint8_t starting_prb;
	uint8_t n_prb;
	uint8_t cdm_index;
	uint8_t n_srs;
} fapi_ul_config_req_cqi_rel13_info_t;

typedef struct {
	fapi_ul_config_req_cqi_rel8_info_t rel8_info;
	fapi_ul_config_req_cqi_rel10_info_t rel10_info;
	fapi_ul_config_req_cqi_rel13_info_t rel13_info;
} fapi_ul_config_req_cqi_info_t;

typedef struct { 
	uint16_t pucch_index;
} fapi_ul_config_req_sr_rel8_info_t;

typedef struct {
	uint8_t number_of_pucch_resources;
	uint16_t pucch_index_p1;
} fapi_ul_config_req_sr_rel10_info_t;

typedef struct {
	fapi_ul_config_req_sr_rel8_info_t rel8_info;
	fapi_ul_config_req_sr_rel10_info_t rel10_info;
} fapi_ul_config_req_sr_info_t;

typedef struct {
	uint8_t harq_size;
	uint8_t ack_nack_mode;
	uint8_t number_of_pucch_resources;
	uint16_t n_pucch_1_0;
	uint16_t n_pucch_1_1;
	uint16_t n_pucch_1_2;
	uint16_t n_pucch_1_3;
} fapi_ul_config_req_harq_rel10_tdd_info_t;

typedef struct {
	uint16_t n_pucch_1_0;
	uint8_t harq_size;
} fapi_ul_config_req_harq_rel8_fdd_info_t;

typedef struct {
	uint8_t harq_size;
	uint8_t ack_nack_mode;
	uint8_t number_of_pucch_resources;
	uint16_t n_pucch_1_0;
	uint16_t n_pucch_1_1;
	uint16_t n_pucch_1_2;
	uint16_t n_pucch_1_3;
} fapi_ul_config_req_harq_rel9_fdd_info_t;

typedef struct {
	uint8_t  num_ant_ports;
	uint16_t n_pucch_2_0;
	uint16_t n_pucch_2_1;
	uint16_t n_pucch_2_2;
	uint16_t n_pucch_2_3;	
} fapi_ul_config_req_harq_rel11_fdd_tdd_info_t;

typedef struct {
	uint16_t  harq_size_2;
	uint8_t starting_prb;
	uint8_t n_prb;
	uint8_t cdm_index;
	uint8_t n_srs;
} fapi_ul_config_req_harq_rel13_fdd_tdd_info_t;

typedef struct {
	fapi_ul_config_req_harq_rel10_tdd_info_t		rel10_tdd_info;
	fapi_ul_config_req_harq_rel8_fdd_info_t			rel8_fdd_info;
	fapi_ul_config_req_harq_rel9_fdd_info_t			rel9_fdd_info;
	fapi_ul_config_req_harq_rel11_fdd_tdd_info_t	rel11_fdd_tdd_info;
	fapi_ul_config_req_harq_rel13_fdd_tdd_info_t	rel13_fdd_tdd_info;
} fapi_ul_config_req_harq_info_t;

typedef struct {
	fapi_ul_config_req_ulsch_pdu_t		ulsch_pdu;
	fapi_ul_config_cqi_ri_information_t cqi_ri_info;
	fapi_ul_config_init_tx_params_t		init_tx_params;
} fapi_ul_config_req_ulsch_cqi_ri_pdu_t;

typedef struct {
	fapi_ul_config_req_ulsch_pdu_t		ulsch_pdu;
	fapi_ul_config_harq_information_t	harq_info;
	fapi_ul_config_init_tx_params_t		init_tx_params;
} fapi_ul_config_req_ulsch_harq_pdu_t;

typedef struct {
	fapi_ul_config_req_ulsch_pdu_t		ulsch_pdu;
	fapi_ul_config_cqi_ri_information_t cqi_ri_info;
	fapi_ul_config_harq_information_t	harq_info;
	fapi_ul_config_init_tx_params_t		init_tx_params;
} fapi_ul_config_req_ulsch_cqi_harq_pdu_t;

typedef struct {
	fapi_ul_config_req_ue_info_t		ue_info;
	fapi_ul_config_req_cqi_info_t		cqi_info;
} fapi_ul_config_req_uci_cqi_pdu_t;

typedef struct {
	fapi_ul_config_req_ue_info_t		ue_info;
	fapi_ul_config_req_sr_info_t		sr_info;
} fapi_ul_config_req_uci_sr_pdu_t;

typedef struct {
	fapi_ul_config_req_ue_info_t		ue_info;
	fapi_ul_config_req_harq_info_t		harq_info;
} fapi_ul_config_req_uci_harq_pdu_t;

typedef struct {
	fapi_ul_config_req_ue_info_t		ue_info;
	fapi_ul_config_req_sr_info_t		sr_info;
	fapi_ul_config_req_harq_info_t		harq_info;
} fapi_ul_config_req_uci_sr_harq_pdu_t;

typedef struct {
	fapi_ul_config_req_ue_info_t		ue_info;
	fapi_ul_config_req_cqi_info_t		cqi_info;
	fapi_ul_config_req_harq_info_t		harq_info;
} fapi_ul_config_req_uci_cqi_harq_pdu_t;

typedef struct {
	fapi_ul_config_req_ue_info_t		ue_info;
	fapi_ul_config_req_cqi_info_t		cqi_info;
	fapi_ul_config_req_sr_info_t		sr_info;
} fapi_ul_config_req_uci_cqi_sr_pdu_t;

typedef struct {
	fapi_ul_config_req_ue_info_t		ue_info;
	fapi_ul_config_req_cqi_info_t		cqi_info;
	fapi_ul_config_req_sr_info_t		sr_info;
	fapi_ul_config_req_harq_info_t		harq_info;
} fapi_ul_config_req_uci_cqi_sr_harq_pdu_t;

typedef struct {
	uint32_t handle;
	uint16_t size;
	uint16_t rnti;
	uint8_t srs_bandwidth;
	uint8_t frqeuency_domain_position;
	uint8_t srs_hopping_bandwidth;
	uint8_t transmission_comb;
	uint16_t srs_config_index;
	uint8_t sounding_reference_cyclic_shift;
} fapi_ul_config_req_srs_rel8_pdu_t;

typedef struct { 
	uint8_t antenna_port;
} fapi_ul_config_req_srs_rel10_pdu_t;

typedef struct {
	uint8_t number_of_combs;
} fapi_ul_config_req_srs_rel13_pdu_t;

typedef struct {
	fapi_ul_config_req_srs_rel8_pdu_t rel8_pdu;
	fapi_ul_config_req_srs_rel10_pdu_t rel10_pdu;
	fapi_ul_config_req_srs_rel13_pdu_t rel13_pdu;
} fapi_ul_config_req_srs_pdu_t;

typedef struct {
	fapi_ul_config_req_ue_info_t		ue_info;
} fapi_ul_config_req_harq_buffer_pdu_t;

typedef struct {
	fapi_ul_config_req_ulsch_pdu_t		ulsch_pdu;
	fapi_ul_config_req_cqi_info_t 	csi_information;
} fapi_ul_config_req_ulsch_uci_csi_pdu_t;

typedef struct {
	fapi_ul_config_req_ulsch_pdu_t		ulsch_pdu;
	fapi_ul_config_req_harq_info_t		harq_information;
} fapi_ul_config_req_ulsch_uci_harq_pdu_t;

typedef struct {
	fapi_ul_config_req_ulsch_pdu_t		ulsch_pdu;
	fapi_ul_config_req_cqi_info_t 		csi_information;
	fapi_ul_config_req_harq_info_t		harq_information;
} fapi_ul_config_req_ulsch_csi_uci_harq_pdu_t;

typedef struct {
	uint8_t harq_ack_resource;
} fapi_ul_config_req_nb_harq_rel13_info_t;

typedef struct {
	fapi_ul_config_req_nb_harq_rel13_info_t	nb_harq_pdu_rel13;
} fapi_ul_config_req_nb_harq_info_t;

typedef struct {
	uint8_t nulsch_format;
	uint32_t handle;
	uint16_t size;
	uint16_t rnti;
	uint8_t subcarrier_indication;
	uint8_t resource_assignment;
	uint8_t mcs;
	uint8_t redudancy_version;
	uint8_t repetition_number;
	uint8_t new_data_indication;
	uint8_t n_srs;
	uint16_t scrambling_sequence_initialization_cinit;
	uint16_t sf_idx;
	fapi_ul_config_req_ue_info_t ue_information;
	fapi_ul_config_req_nb_harq_info_t nb_harq_information;
} fapi_ul_config_req_nulsch_rel13_pdu_t;

typedef struct {
	fapi_ul_config_req_nulsch_rel13_pdu_t	nulsch_pdu_rel13;
} fapi_ul_config_req_nulsch_pdu_t;

typedef struct {
	uint8_t nprach_config_0;
	uint8_t nprach_config_1;
	uint8_t nprach_config_2;
} fapi_ul_config_req_nrach_rel13_pdu_t;

typedef struct {
	fapi_ul_config_req_nrach_rel13_pdu_t	nrach_pdu_rel13;
} fapi_ul_config_req_nrach_pdu_t;

typedef struct { 
	uint8_t pdu_type;
	uint8_t pdu_size;
	union
	{
		fapi_ul_config_req_ulsch_pdu_t				ulsch_pdu;
		fapi_ul_config_req_ulsch_cqi_ri_pdu_t		ulsch_cqi_ri_pdu;
		fapi_ul_config_req_ulsch_harq_pdu_t			ulsch_harq_pdu;
		fapi_ul_config_req_ulsch_cqi_harq_pdu_t		ulsch_cqi_harq_pdu;
		fapi_ul_config_req_uci_cqi_pdu_t			uci_cqi_pdu;
		fapi_ul_config_req_uci_sr_pdu_t				uci_sr_pdu;
		fapi_ul_config_req_uci_harq_pdu_t			uci_harq_pdu;
		fapi_ul_config_req_uci_sr_harq_pdu_t		uci_sr_harq_pdu;
		fapi_ul_config_req_uci_cqi_harq_pdu_t		uci_cqi_harq_pdu;
		fapi_ul_config_req_uci_cqi_sr_pdu_t			uci_cqi_sr_pdu;
		fapi_ul_config_req_uci_cqi_sr_harq_pdu_t	uci_cqi_sr_harq_pdu;
		fapi_ul_config_req_srs_pdu_t				srs_pdu;
		fapi_ul_config_req_harq_buffer_pdu_t		harq_buffer_pdu;
		fapi_ul_config_req_ulsch_uci_csi_pdu_t		ulsch_uci_csi_pdu;
		fapi_ul_config_req_ulsch_uci_harq_pdu_t		ulsch_uci_harq_pdu;
		fapi_ul_config_req_ulsch_csi_uci_harq_pdu_t	ulsch_csi_uci_harq_pdu;
		fapi_ul_config_req_nulsch_pdu_t				nulsch_pdu;
		fapi_ul_config_req_nrach_pdu_t				nrach_pdu;
	};
} fapi_ul_config_req_pdu_t;

typedef struct {
	uint8_t number_of_pdus;
	uint8_t rach_prach_frequency_resources;
	uint8_t srs_present;
	fapi_ul_config_req_pdu_t* pdus;
} fapi_ul_config_req_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	uint16_t length;
	fapi_ul_config_req_body_t body;
} fapi_ul_config_req_t;

typedef struct { 
	uint8_t dci_format;
	uint8_t cce_index;
	uint8_t aggregation_level;
	uint8_t rnti;
	uint8_t resource_block_start;
	uint8_t number_of_resource_blocks;
	uint8_t mcs_1;
	uint8_t cyclic_shift_2_for_drms;
	uint8_t frequency_hopping_enabled_flag;
	uint8_t frequency_hopping_flags;
	uint8_t new_data_indication_1;
	uint8_t ue_tx_antenna_selection;
	uint8_t tpc;
	uint8_t cqi_csi_request;
	uint8_t ul_index;
	uint8_t dl_assignment_index;
	uint32_t tpc_bitmap;
	uint16_t transmission_power;
} fapi_hi_dci0_req_dci0_rel8_pdu_t;

typedef struct {
	uint8_t cross_carrier_scheduling_flag;
	uint8_t carrier_indicator;
	uint8_t size_of_cqi_csi_feild;
	uint8_t srs_flag;
	uint8_t srs_request;
	uint8_t resource_allocation_flag;
	uint8_t resource_allocation_type;
	uint32_t resource_block_coding;
	uint8_t mcs_2;
	uint8_t new_data_indication_2;
	uint8_t number_of_antenna_ports;
	uint8_t tpmi;
	uint8_t total_dci_length_including_padding;
	uint8_t n_ul_rb;
} fapi_hi_dci0_req_dci0_rel10_pdu_t;

typedef struct {
	uint8_t pscch_resource;
	uint8_t time_resource_pattern;
} fapi_hi_dci0_req_dci0_rel12_pdu_t;

typedef struct {
	fapi_hi_dci0_req_dci0_rel8_pdu_t rel8_pdu;
	fapi_hi_dci0_req_dci0_rel10_pdu_t rel10_pdu;
	fapi_hi_dci0_req_dci0_rel12_pdu_t rel12_pdu;
} fapi_hi_dci0_req_dci0_pdu_t;

typedef struct {
	uint8_t resource_block_start;
	uint8_t cyclic_shift_2_for_drms;
	uint8_t hi_value;
	uint8_t i_phich;
	uint16_t transmission_power;
} fapi_hi_dci0_req_hi_rel8_pdu_t;

typedef struct {
	uint8_t flag_tb2;
	uint8_t hi_value_2;
} fapi_hi_dci0_req_hi_rel10_pdu_t;

typedef struct {
	fapi_hi_dci0_req_hi_rel8_pdu_t rel8_pdu;
	fapi_hi_dci0_req_hi_rel10_pdu_t rel10_pdu;
} fapi_hi_dci0_req_hi_pdu_t;

typedef struct {
	fapi_dl_config_dci_rel8_pdu_t			edpcch_dci_pdu_rel8;
	fapi_dl_config_dci_rel10_pdu_t			edpcch_dci_pdu_rel10;
	fapi_dl_config_edpcch_rel11_params_t	edpcch_parameters_rel11;
} fapi_hi_dci0_req_epdcch_dci_ul_pdu_t;

typedef struct {
	uint8_t mpdcch_narrowband;
	uint8_t number_of_prb_pairs;
	uint8_t resource_block_assignment;
	uint8_t mpdcch_transmission_type;
	uint8_t start_symbol;
	uint8_t ecce_index;
	uint8_t aggreagation_level;
	uint8_t rnti_type;
	uint16_t rnti;
	uint8_t ce_mode;
	uint16_t drms_scrambling_init;
	uint16_t initial_transmission_sf_io;
	uint16_t transmission_power;
	uint8_t dci_format;
	uint8_t resource_block_start;
	uint8_t number_of_resource_blocks;
	uint8_t mcs;
	uint8_t pusch_repetition_levels;
	uint8_t frequency_hopping_flag;
	uint8_t new_data_indication;
	uint8_t harq_process;
	uint8_t redudency_version;
	uint8_t tpc;
	uint8_t csi_request;
	uint8_t ul_inex;
	uint8_t dai_presence_flag;
	uint8_t dl_assignment_index;
	uint8_t srs_request;
	uint8_t dci_subframe_repetition_number;
	uint32_t tcp_bitmap;
	uint8_t total_dci_length_include_padding;
	uint8_t number_of_tx_antenna_ports;
	uint16_t precoding_value[FAPI_MAX_ANTENNA_PORT_COUNT];
} fapi_hi_dci0_req_mpdcch_dci_ul_rel13_pdu_t;

typedef struct {
	fapi_hi_dci0_req_mpdcch_dci_ul_rel13_pdu_t	mpdcch_dci_pdu_rel13;
} fapi_hi_dci0_req_mpdcch_dci_ul_pdu_t;

typedef struct {
	uint8_t ncce_index;
	uint8_t aggregation_level;
	uint8_t start_symbol;
	uint16_t rnti;
	uint8_t scrambling_reinitialization_batch_index;
	uint8_t nrs_antenna_ports_assumed_by_the_ue;
	uint8_t subcarrier_indication;
	uint8_t resource_assignment;
	uint8_t scheduling_delay;
	uint8_t mcs;
	uint8_t redudancy_version;
	uint8_t repetition_number;
	uint8_t new_data_indicator;
	uint8_t dci_subframe_repetition_number;
} fapi_hi_dci0_req_npdcch_dci_ul_rel13_pdu_t;

typedef struct {
	fapi_hi_dci0_req_npdcch_dci_ul_rel13_pdu_t	npdcch_dci_pdu_rel13;
} fapi_hi_dci0_req_npdcch_dci_ul_pdu_t;

typedef struct {
	uint8_t pdu_type;
	uint8_t pdu_size;
	union 
	{
		fapi_hi_dci0_req_hi_pdu_t				hi_pdu;
		fapi_hi_dci0_req_dci0_pdu_t				dci0_pdu;
		fapi_hi_dci0_req_epdcch_dci_ul_pdu_t	edpcch_dci_ul_pdu;
		fapi_hi_dci0_req_mpdcch_dci_ul_pdu_t	mdpcch_dci_ul_pdu;
		fapi_hi_dci0_req_npdcch_dci_ul_pdu_t	ndpcch_dci_ul_pdu;
	};
} fapi_hi_dci0_req_pdu_t;

typedef struct {
	uint16_t sfn_sf;
	uint8_t number_of_dci;
	uint8_t number_of_hi;
	fapi_hi_dci0_req_pdu_t* pdus;
} fapi_hi_dci0_req_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_hi_dci0_req_body_t body;
} fapi_hi_dci0_req_t;

typedef struct {
	uint16_t tag;
	uint16_t length;
	uint32_t* value;
} fapi_tx_req_pdu_tlv_t;

typedef struct {
	uint16_t pdu_length;
	uint16_t pdu_index;
	uint32_t num_tlv;
	fapi_tx_req_pdu_tlv_t tlvs[32];
} fapi_tx_req_pdu_t;

typedef struct {
	uint16_t number_of_pdus;
	fapi_tx_req_pdu_t* pdus;
} fapi_tx_req_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	fapi_tx_req_body_t body;
} fapi_tx_req_t;

typedef struct {
	uint32_t handle;
	uint32_t mp_cca;
	uint32_t n_cca;
	uint32_t offset;
	uint32_t lte_txop_sf;
	uint16_t txop_sfn_sf_end;
	uint32_t lbt_mode;
} fapi_lbt_pdsch_req_rel13_pdu_t;

typedef struct {
	fapi_lbt_pdsch_req_rel13_pdu_t	rel13_pdu;
} fapi_lbt_pdsch_req_pdu_t;

typedef struct {
	uint32_t handle;
	uint32_t offset;
	uint16_t sfn_sf_end;
	uint32_t lbt_mode;
} fapi_lbt_drs_req_rel13_pdu_t;

typedef struct {
	fapi_lbt_drs_req_rel13_pdu_t	rel13_pdu;
} fapi_lbt_drs_req_pdu_t;

typedef struct {
	uint8_t pdu_type;
	uint8_t pdu_size;
	union {
		fapi_lbt_pdsch_req_pdu_t	lbt_pdsch_req_pdu;
		fapi_lbt_drs_req_pdu_t		lbt_drs_req_pdu;
	};
} fapi_lbt_dl_config_req_pdu_t;

typedef struct {
	uint16_t number_of_pdus;
	fapi_lbt_dl_config_req_pdu_t pdus[32];
} fapi_lbt_dl_config_req_body_t;

typedef struct {
	fapi_header_t header;
	uint16_t sfn_sf;
	uint16_t length;
	fapi_lbt_dl_config_req_body_t body;
} fapi_lbt_dl_config_req_t;

//------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

#endif
