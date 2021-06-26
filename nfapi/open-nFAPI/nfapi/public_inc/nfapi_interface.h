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


#ifndef _NFAPI_INTERFACE_H_
#define _NFAPI_INTERFACE_H_

#include "stddef.h"
#include <stdint.h>

// Constants - update based on implementation
#define NFAPI_MAX_PHY_RF_INSTANCES 2
#define NFAPI_PNF_PARAM_GENERAL_LOCATION_LENGTH 16
#define NFAPI_PNF_PARAM_GENERAL_OUI_LENGTH 3
#define NFAPI_MAX_NUM_RF_BANDS 16

#define NFAPI_MAX_PACKED_MESSAGE_SIZE 8192

// The following definition control the size of arrays used in the interface.
// These may be changed if desired. They are used in the encoder to make sure 
// that the user has not specified a 'count' larger than the max array, and also
// used by the decoder when decode an array. If the 'count' received is larger
// than the array it is to be stored in the decode fails.
#define NFAPI_MAX_NUM_ANTENNAS 8
#define NFAPI_MAX_NUM_SUBBANDS 13
#define NFAPI_MAX_BF_VECTORS 8
#define NFAPI_MAX_CC 1
#define NFAPI_MAX_NUM_PHYSICAL_ANTENNAS 8
#define NFAPI_MAX_RSSI 8
#define NFAPI_MAX_PSC_LIST 32
#define NFAPI_MAX_PCI_LIST 32
#define NFAPI_MAX_CARRIER_LIST 32
#define NFAPI_MAX_ARFCN_LIST 128
#define NFAPI_MAX_LTE_CELLS_FOUND 8
#define NFAPI_MAX_UTRAN_CELLS_FOUND 8
#define NFAPI_MAX_GSM_CELLS_FOUND 8
#define NFAPI_MAX_NB_IOT_CELLS_FOUND 8
#define NFAPI_MAX_SI_PERIODICITY 8
#define NFAPI_MAX_SI_INDEX 8
#define NFAPI_MAX_MIB_LENGTH 32
#define NFAPI_MAX_SIB_LENGTH 256
#define NFAPI_MAX_SI_LENGTH 256
#define NFAPI_MAX_OPAQUE_DATA 64
#define NFAPI_MAX_NUM_SCHEDULED_UES 8 // Used in the TPM structure
#define NFAPI_MAX_PNF_PHY 5
#define NFAPI_MAX_PNF_PHY_RF_CONFIG 5
#define NFAPI_MAX_PNF_RF  5
#define NFAPI_MAX_NMM_FREQUENCY_BANDS 32
#define NFAPI_MAX_RECEIVED_INTERFERENCE_POWER_RESULTS 100
#define NFAPI_MAX_UL_DL_CONFIGURATIONS 5
#define NFAPI_MAX_CSI_RS_RESOURCE_CONFIG 4
#define NFAPI_MAX_ANTENNA_PORT_COUNT 8
#define NFAPI_MAX_EPDCCH_PRB 8
#define NFAPI_MAX_TX_PHYSICAL_ANTENNA_PORTS 8
#define NFAPI_MAX_NUMBER_ACK_NACK_TDD 8
#define NFAPI_MAX_RO_DL 8

#define NFAPI_HEADER_LENGTH 8
#define NFAPI_P7_HEADER_LENGTH 16

#define NFAPI_VENDOR_EXTENSION_MIN_TAG_VALUE 0xF000
#define NFAPI_VENDOR_EXTENSION_MAX_TAG_VALUE 0xFFFF

#define NFAPI_VERSION_3_0_11	0x000
#define NFAPI_VERSION_3_0_12    0x001

#define NFAPI_HALF_FRAME_INDEX_FIRST_HALF 0
#define NFAPI_HALF_FRAME_INDEX_SECOND_HALF 1

// The IANA agreed port definition of the P5 SCTP VNF enpoint 
// http://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml?search=7701
#define NFAPI_P5_SCTP_PORT		7701

typedef unsigned int	uint32_t;
typedef unsigned short	uint16_t;
typedef unsigned char	uint8_t;
typedef signed int		int32_t;
typedef signed short	int16_t;
typedef signed char		int8_t;

typedef struct {
	uint16_t phy_id;
	uint16_t message_id;
	uint16_t message_length;
	uint16_t spare;
} nfapi_p4_p5_message_header_t;

typedef struct {
	uint16_t phy_id;
	uint16_t message_id;
	uint16_t message_length;
	uint16_t m_segment_sequence; /* This consists of 3 fields - namely, M, Segement & Sequence number*/
	uint32_t checksum;
	uint32_t transmit_timestamp;
} nfapi_p7_message_header_t;

#define NFAPI_PHY_ID_NA 0

//#define NFAPI_P7_GET_MORE(_mss) ( ((_mss) & 0x80) >> 7 )
//#define NFAPI_P7_GET_SEGMENT(_mss) ( ((_mss) & 0x70) >> 4 )
#define NFAPI_P7_GET_MORE(_mss) ( ((_mss) & 0x8000) >> 15 )
#define NFAPI_P7_GET_SEGMENT(_mss) ( ((_mss) & 0x7F00) >> 8 )
#define NFAPI_P7_GET_SEQUENCE(_mss) ( (_mss) & 0x00FF )
#define NFAPI_P7_SET_MSS(_more, _segm, _sequ) ( (((_more) & 0x1) << 7) | (((_segm) & 0x7) << 4) | ((_sequ) & 0xF) )

typedef struct {
	uint16_t tag;
	uint16_t length;
} nfapi_tl_t;
#define NFAPI_TAG_LENGTH_PACKED_LEN 4

// Convenience methods to convert between SFN/SLOT formats
#define NFAPI_SFNSLOT2DEC(_sfn,_slot) ( _sfn*20 + _slot  )  // total count of slots
#define NFAPI_SFNSLOTDEC2SFNSLOT(_sfnslot_dec) ((((_sfnslot_dec) / 20) << 6) | (((_sfnslot_dec) - (((_sfnslot_dec) / 20) * 20)) & 0x3F))

#define NFAPI_SFNSLOT2SFN(_sfnslot) ((_sfnslot) >> 6)
#define NFAPI_SFNSLOT2SLOT(_sfnslot) ((_sfnslot) & 0x3F)
#define NFAPI_SFNSLOTDEC2SFN(_sfnslot_dec) ((_sfnslot_dec) / 20)
#define NFAPI_SFNSLOTDEC2SLOT(_sfnslot_dec) ((_sfnslot_dec) % 20)
#define NFAPI_SFNSLOT2HEX(_sfn,_slot) ((_sfn << 6) | (_slot & 0x3F))

#define NFAPI_MAX_SFNSLOTDEC 1024*20 // 20 is for numerology 1

// Convenience methods to convert between SFN/SFN formats
#define NFAPI_SFNSF2DEC(_sfnsf) ((((_sfnsf) >> 4) * 10) + ((_sfnsf) & 0xF))
#define NFAPI_SFNSFDEC2SFNSF(_sfnsf_dec) ((((_sfnsf_dec) / 10) << 4) | (((_sfnsf_dec) - (((_sfnsf_dec) / 10) * 10)) & 0xF))

#define NFAPI_SFNSF2SFN(_sfnsf) ((_sfnsf) >> 4)
#define NFAPI_SFNSF2SF(_sfnsf) ((_sfnsf) & 0xF)

#define NFAPI_MAX_SFNSFDEC 10240

typedef nfapi_tl_t* nfapi_vendor_extension_tlv_t;


// nFAPI Message IDs
typedef enum {
	NFAPI_DL_CONFIG_REQUEST = 0x0080,
	NFAPI_UL_CONFIG_REQUEST,
	NFAPI_SUBFRAME_INDICATION,
	NFAPI_HI_DCI0_REQUEST,
	NFAPI_TX_REQUEST,
	NFAPI_HARQ_INDICATION,
	NFAPI_CRC_INDICATION,
	NFAPI_RX_ULSCH_INDICATION,
	NFAPI_RACH_INDICATION,
	NFAPI_SRS_INDICATION,
	NFAPI_RX_SR_INDICATION,
	NFAPI_RX_CQI_INDICATION,
	NFAPI_LBT_DL_CONFIG_REQUEST,
	NFAPI_LBT_DL_INDICATION,
	NFAPI_NB_HARQ_INDICATION,
	NFAPI_NRACH_INDICATION,
	NFAPI_UE_RELEASE_REQUEST,
	NFAPI_UE_RELEASE_RESPONSE,

	NFAPI_PNF_PARAM_REQUEST = 0x0100,
	NFAPI_PNF_PARAM_RESPONSE,
	NFAPI_PNF_CONFIG_REQUEST,
	NFAPI_PNF_CONFIG_RESPONSE,
	NFAPI_PNF_START_REQUEST,
	NFAPI_PNF_START_RESPONSE,
	NFAPI_PNF_STOP_REQUEST,
	NFAPI_PNF_STOP_RESPONSE,
	NFAPI_PARAM_REQUEST,
	NFAPI_PARAM_RESPONSE,
	NFAPI_CONFIG_REQUEST,
	NFAPI_CONFIG_RESPONSE,
	NFAPI_START_REQUEST,
	NFAPI_START_RESPONSE,
	NFAPI_STOP_REQUEST,
	NFAPI_STOP_RESPONSE,
	NFAPI_MEASUREMENT_REQUEST,
	NFAPI_MEASUREMENT_RESPONSE,

	NFAPI_UL_NODE_SYNC = 0x0180,
	NFAPI_DL_NODE_SYNC,
	NFAPI_TIMING_INFO,

	NFAPI_RSSI_REQUEST = 0x0200,
	NFAPI_RSSI_RESPONSE,
	NFAPI_RSSI_INDICATION,
	NFAPI_CELL_SEARCH_REQUEST,
	NFAPI_CELL_SEARCH_RESPONSE,
	NFAPI_CELL_SEARCH_INDICATION,
	NFAPI_BROADCAST_DETECT_REQUEST,
	NFAPI_BROADCAST_DETECT_RESPONSE,
	NFAPI_BROADCAST_DETECT_INDICATION,
	NFAPI_SYSTEM_INFORMATION_SCHEDULE_REQUEST,
	NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE,
	NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION,
	NFAPI_SYSTEM_INFORMATION_REQUEST,
	NFAPI_SYSTEM_INFORMATION_RESPONSE,
	NFAPI_SYSTEM_INFORMATION_INDICATION,
	NFAPI_NMM_STOP_REQUEST,
	NFAPI_NMM_STOP_RESPONSE,

	NFAPI_VENDOR_EXT_MSG_MIN = 0x0300,
	NFAPI_VENDOR_EXT_MSG_MAX = 0x03FF,


	NFAPI_MAX_MESSAGE_ID,
} nfapi_message_id_e;

// nFAPI Error Codes
typedef enum {
	NFAPI_MSG_OK = 0,
	NFAPI_MSG_INVALID_STATE,
	NFAPI_MSG_INVALID_CONFIG,
	NFAPI_SFN_OUT_OF_SYNC,
	NFAPI_MSG_SUBFRAME_ERR,
	NFAPI_MSG_BCH_MISSING,
	NFAPI_MSG_INVALID_SFN,
	NFAPI_MSG_HI_ERR,
	NFAPI_MSG_TX_ERR,
	
	NFAPI_LBT_NO_PDU_IN_DL_REQ,
	NFAPI_LBT_NO_VALID_CONFIG_REQ_RECEIVED,
	NFAPI_FAPI_E_LBT_SF_SFN_PASSED_END_SF_SFN,
	NFAPI_FAPI_E_LBT_OVERLAP,
	NFAPI_MSG_BCH_PRESENT,
	
	NFAPI_NBIOT_UNEXPECTED_REQ,

	// This is special return code that indicates that a response has
	// been send via P9
	NFAPI_MSG_P9_RESPONSE = 0xAA
} nfapi_error_code_e;


typedef enum {
	NFAPI_P4_MSG_OK = 100,
	NFAPI_P4_MSG_INVALID_STATE = 101,
	NFAPI_P4_MSG_INVALID_CONFIG = 102,
	NFAPI_P4_MSG_RAT_NOT_SUPPORTED = 103,
	NFAPI_P4_MSG_NMM_STOP_OK = 200,
	NFAPI_P4_MSG_NMM_STOP_IGNOREDED = 201,
	NFAPI_P4_MSG_NMM_STOP_INVALID_STATE = 202,
	NFAPI_P4_MSG_PROCEDURE_COMPLETE = 300,
	NFAPI_P4_MSG_PROCEDURE_STOPPED = 301,
	NFAPI_P4_MSG_PARTIAL_RESULTS = 302,
	NFAPI_P4_MSG_TIMEOUT = 303
} nfapi_p4_error_code_e;

// nFAPI enums
typedef enum {
	NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE = 0,
	NFAPI_DL_CONFIG_BCH_PDU_TYPE,
	NFAPI_DL_CONFIG_MCH_PDU_TYPE,
	NFAPI_DL_CONFIG_DLSCH_PDU_TYPE,
	NFAPI_DL_CONFIG_PCH_PDU_TYPE,
	NFAPI_DL_CONFIG_PRS_PDU_TYPE,
	NFAPI_DL_CONFIG_CSI_RS_PDU_TYPE,
	NFAPI_DL_CONFIG_EPDCCH_DL_PDU_TYPE,
	NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE,
	NFAPI_DL_CONFIG_NBCH_PDU_TYPE,
	NFAPI_DL_CONFIG_NPDCCH_PDU_TYPE,
	NFAPI_DL_CONFIG_NDLSCH_PDU_TYPE
} nfapi_dl_config_pdu_type_e;

typedef enum {
	NFAPI_DL_DCI_FORMAT_1 = 0,
	NFAPI_DL_DCI_FORMAT_1A,
	NFAPI_DL_DCI_FORMAT_1B,
	NFAPI_DL_DCI_FORMAT_1C,
	NFAPI_DL_DCI_FORMAT_1D,
	NFAPI_DL_DCI_FORMAT_2,
	NFAPI_DL_DCI_FORMAT_2A,
	NFAPI_DL_DCI_FORMAT_2B,
	NFAPI_DL_DCI_FORMAT_2C
} nfapi_dl_dci_format_e;

typedef enum {
	NFAPI_UL_DCI_FORMAT_0 = 0,
	NFAPI_UL_DCI_FORMAT_3,
	NFAPI_UL_DCI_FORMAT_3A,
	NFAPI_UL_DCI_FORMAT_4
} nfapi_ul_dci_format_e;

typedef enum {
	NFAPI_UL_CONFIG_ULSCH_PDU_TYPE = 0,
	NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE,
	NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE,
	NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE,
	NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE,
	NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE,
	NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE,
	NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE,
	NFAPI_UL_CONFIG_UCI_CQI_HARQ_PDU_TYPE,
	NFAPI_UL_CONFIG_UCI_CQI_SR_PDU_TYPE,
	NFAPI_UL_CONFIG_UCI_CQI_SR_HARQ_PDU_TYPE,
	NFAPI_UL_CONFIG_SRS_PDU_TYPE,
	NFAPI_UL_CONFIG_HARQ_BUFFER_PDU_TYPE,
	NFAPI_UL_CONFIG_ULSCH_UCI_CSI_PDU_TYPE,
	NFAPI_UL_CONFIG_ULSCH_UCI_HARQ_PDU_TYPE,
	NFAPI_UL_CONFIG_ULSCH_CSI_UCI_HARQ_PDU_TYPE,
	NFAPI_UL_CONFIG_NULSCH_PDU_TYPE,
	NFAPI_UL_CONFIG_NRACH_PDU_TYPE,
} nfapi_ul_config_pdu_type_e;

typedef enum {
	NFAPI_HI_DCI0_HI_PDU_TYPE = 0,
	NFAPI_HI_DCI0_DCI_PDU_TYPE,
	NFAPI_HI_DCI0_EPDCCH_DCI_PDU_TYPE,
	NFAPI_HI_DCI0_MPDCCH_DCI_PDU_TYPE,
	NFAPI_HI_DCI0_NPDCCH_DCI_PDU_TYPE,
} nfapi_hi_dci0_pdu_type_e;

typedef enum {
	NFAPI_HARQ_ACK = 1,
	NFAPI_HARQ_NACK,
	NFAPI_HARQ_ACK_OR_NACK,
	NFAPI_HARQ_DTX,
	NFAPI_HARQ_ACK_OR_DTX,
	NFAPI_HARQ_NACK_OR_DTX,
	NFAPI_HARQ_ACK_OR_NACK_OR_DTX
} nfapi_harq_type_e;

typedef enum {
	NFAPI_CSI_REPORT_TYPE_PERIODIC = 0,
	NFAPI_CSI_REPORT_TYPE_APERIODIC
} nfapi_csi_report_type_e;

typedef enum {
	NFAPI_DL_BW_SUPPORTED_6 = 1,
	NFAPI_DL_BW_SUPPORTED_15 = 2,
	NFAPI_DL_BW_SUPPORTED_25 = 4,
	NFAPI_DL_BW_SUPPORTED_50 = 8,
	NFAPI_DL_BW_SUPPORTED_75 = 16,
	NFAPI_DL_BW_SUPPORTED_100 = 32
} nfapi_dl_bandwith_supported_e;

typedef enum {
	NFAPI_UL_BW_SUPPORTED_6 = 1,
	NFAPI_UL_BW_SUPPORTED_15 = 2,
	NFAPI_UL_BW_SUPPORTED_25 = 4,
	NFAPI_UL_BW_SUPPORTED_50 = 8,
	NFAPI_UL_BW_SUPPORTED_75 = 16,
	NFAPI_UL_BW_SUPPORTED_100 = 32
} nfapi_ul_bandwith_supported_e;

typedef enum {
	NFAPI_3GPP_REL_SUPPORTED_8 = 0,
	NFAPI_3GPP_REL_SUPPORTED_9 = 1,
	NFAPI_3GPP_REL_SUPPORTED_10 = 2,
	NFAPI_3GPP_REL_SUPPORTED_11 = 4,
	NFAPI_3GPP_REL_SUPPORTED_12 = 8,
  NFAPI_3GPP_REL_SUPPORTED_15 = 64
} nfapi_3gpp_release_supported_e;


typedef enum {
	NFAPI_DUPLEXING_MODE_TDD = 0,
	NFAPI_DUPLEXING_MODE_FDD = 1,
	NFAPI_DUPLEXING_MODE_HD_FDD = 2,
} nfapi_duplexing_mode_e;

typedef enum {
	NFAPI_CP_NORMAL = 0,
	NFAPI_CP_EXTENDED = 1
} nfapi_cyclic_prefix_type_e;

typedef enum {
	NFAPI_RAT_TYPE_LTE = 0,
	NFAPI_RAT_TYPE_UTRAN = 1,
	NFAPI_RAT_TYPE_GERAN = 2,
	NFAPI_RAT_TYPE_NB_IOT = 3,
  NFAPI_RAT_TYPE_NR = 4
} nfapi_rat_type_e;

typedef enum {
	NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_BUNDLING,
	NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_MULIPLEXING,
	NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_SPECIAL_BUNDLING,
	NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_CHANNEL_SELECTION,
	NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_3,
	NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_4,
	NFAPI_HARQ_INDICATION_TDD_HARQ_ACK_NACK_FORMAT_FORMAT_5
} nfapi_harq_indication_tdd_ack_nackformat_e;


typedef enum {
	NFAPI_LBT_DL_CONFIG_REQUEST_PDSCH_PDU_TYPE = 0,
	NFAPI_LBT_DL_CONFIG_REQUEST_DRS_PDU_TYPE
} nfapi_lbt_dl_config_pdu_type_e;

typedef enum {
	NFAPI_LBT_DL_RSP_PDSCH_PDU_TYPE = 0,
	NFAPI_LBT_DL_RSP_DRS_PDU_TYPE
} nfapi_lbt_dl_rsp_pdu_type_e;

typedef struct {
	nfapi_tl_t tl;
	uint32_t length;
	uint8_t value[NFAPI_MAX_OPAQUE_DATA];
} nfapi_opaqaue_data_t;

// Utility functions to turn enums into char*
const char* nfapi_error_code_to_str(nfapi_error_code_e value);


// P5 Sub Structures
typedef struct {
	nfapi_tl_t tl;
	uint8_t nfapi_sync_mode;
	uint8_t location_mode;
	uint16_t location_coordinates_length;
	uint8_t location_coordinates[NFAPI_PNF_PARAM_GENERAL_LOCATION_LENGTH];
	uint32_t dl_config_timing;
	uint32_t tx_timing;
	uint32_t ul_config_timing;
	uint32_t hi_dci0_timing;
	uint16_t maximum_number_phys;
	uint16_t maximum_total_bandwidth;
	uint8_t maximum_total_number_dl_layers;
	uint8_t maximum_total_number_ul_layers;
	uint8_t shared_bands;
	uint8_t shared_pa;
	int16_t maximum_total_power;
	uint8_t oui[NFAPI_PNF_PARAM_GENERAL_OUI_LENGTH];
} nfapi_pnf_param_general_t;
#define NFAPI_PNF_PARAM_GENERAL_TAG 0x1000





typedef struct {
	uint16_t rf_config_index;
} nfapi_rf_config_info_t;

typedef struct {
	uint16_t phy_config_index;
	uint16_t number_of_rfs;
	nfapi_rf_config_info_t rf_config[NFAPI_MAX_PNF_PHY_RF_CONFIG];
	uint16_t number_of_rf_exclusions;
	nfapi_rf_config_info_t excluded_rf_config[NFAPI_MAX_PNF_PHY_RF_CONFIG];
	uint16_t downlink_channel_bandwidth_supported;
	uint16_t uplink_channel_bandwidth_supported;
	uint8_t number_of_dl_layers_supported;
	uint8_t number_of_ul_layers_supported;
	uint16_t maximum_3gpp_release_supported;
	uint8_t nmm_modes_supported;
} nfapi_pnf_phy_info_t;


typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_phys;
	nfapi_pnf_phy_info_t phy[NFAPI_MAX_PNF_PHY];
} nfapi_pnf_phy_t;
#define NFAPI_PNF_PHY_TAG 0x1001

typedef struct {
	uint16_t phy_config_index;
	uint16_t transmission_mode_7_supported;
	uint16_t transmission_mode_8_supported;
	uint16_t two_antenna_ports_for_pucch;
	uint16_t transmission_mode_9_supported;
	uint16_t simultaneous_pucch_pusch;
	uint16_t four_layer_tx_with_tm3_and_tm4;
} nfapi_pnf_phy_rel10_info_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_phys;
	nfapi_pnf_phy_rel10_info_t phy[NFAPI_MAX_PNF_PHY];
} nfapi_pnf_phy_rel10_t;
#define NFAPI_PNF_PHY_REL10_TAG 0x100A

typedef struct {
	uint16_t phy_config_index;
	uint16_t edpcch_supported;
	uint16_t multi_ack_csi_reporting;
	uint16_t pucch_tx_diversity;
	uint16_t ul_comp_supported;
	uint16_t transmission_mode_5_supported;
} nfapi_pnf_phy_rel11_info_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_phys;
	nfapi_pnf_phy_rel11_info_t phy[NFAPI_MAX_PNF_PHY];
} nfapi_pnf_phy_rel11_t;
#define NFAPI_PNF_PHY_REL11_TAG 0x100B


typedef struct {
	uint16_t phy_config_index;
	uint16_t csi_subframe_set;
	uint16_t enhanced_4tx_codebook;
	uint16_t drs_supported;
	uint16_t ul_64qam_supported;
	uint16_t transmission_mode_10_supported;
	uint16_t alternative_bts_indices;
} nfapi_pnf_phy_rel12_info_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_phys;
	nfapi_pnf_phy_rel12_info_t phy[NFAPI_MAX_PNF_PHY];
} nfapi_pnf_phy_rel12_t;
#define NFAPI_PNF_PHY_REL12_TAG 0x100C

typedef struct {
	uint16_t phy_config_index;
	uint16_t pucch_format4_supported;
	uint16_t pucch_format5_supported;
	uint16_t more_than_5_ca_support;
	uint16_t laa_supported;
	uint16_t laa_ending_in_dwpts_supported;
	uint16_t laa_starting_in_second_slot_supported;
	uint16_t beamforming_supported;
	uint16_t csi_rs_enhancement_supported;
	uint16_t drms_enhancement_supported;
	uint16_t srs_enhancement_supported;
} nfapi_pnf_phy_rel13_info_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_phys;
	nfapi_pnf_phy_rel13_info_t phy[NFAPI_MAX_PNF_PHY];
} nfapi_pnf_phy_rel13_t;
#define NFAPI_PNF_PHY_REL13_TAG 0x100D

typedef struct {
	uint16_t phy_config_index;
	uint16_t number_of_rfs;
	nfapi_rf_config_info_t rf_config[NFAPI_MAX_PNF_PHY_RF_CONFIG];
	uint16_t number_of_rf_exclusions;
	nfapi_rf_config_info_t excluded_rf_config[NFAPI_MAX_PNF_PHY_RF_CONFIG];
	uint8_t number_of_dl_layers_supported;
	uint8_t number_of_ul_layers_supported;
	uint16_t maximum_3gpp_release_supported;
	uint8_t nmm_modes_supported;
} nfapi_pnf_phy_rel13_nb_iot_info_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_phys;
	nfapi_pnf_phy_rel13_nb_iot_info_t phy[NFAPI_MAX_PNF_PHY];
} nfapi_pnf_phy_rel13_nb_iot_t;
#define NFAPI_PNF_PHY_REL13_NB_IOT_TAG 0x100E

typedef struct {
  uint16_t phy_config_index;
} nfapi_pnf_phy_rel15_info_t;

typedef struct {
  nfapi_tl_t tl;
  uint16_t number_of_phys;
  nfapi_pnf_phy_rel15_info_t phy[NFAPI_MAX_PNF_PHY];
} nfapi_pnf_phy_rel15_t;
#define NFAPI_PNF_PHY_REL15_TAG 0x100H


typedef struct {
	uint16_t rf_config_index;
	uint16_t band;
	int16_t maximum_transmit_power; 
	int16_t minimum_transmit_power;
	uint8_t number_of_antennas_suppported;
	uint32_t minimum_downlink_frequency;
	uint32_t maximum_downlink_frequency;
	uint32_t minimum_uplink_frequency;
	uint32_t maximum_uplink_frequency;
} nfapi_pnf_rf_info_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_rfs;
	nfapi_pnf_rf_info_t rf[NFAPI_MAX_PNF_RF];
} nfapi_pnf_rf_t;
#define NFAPI_PNF_RF_TAG 0x1002

typedef struct {
	uint16_t phy_id;
	uint16_t phy_config_index;
	uint16_t rf_config_index;
} nfapi_phy_rf_config_info_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_phy_rf_config_info;
	nfapi_phy_rf_config_info_t phy_rf_config[NFAPI_MAX_PHY_RF_INSTANCES];
} nfapi_pnf_phy_rf_config_t;
#define NFAPI_PNF_PHY_RF_TAG 0x1003

// Generic strucutre for single tlv value.
typedef struct {
	nfapi_tl_t tl;
	int32_t value;
} nfapi_int32_tlv_t;

typedef struct {
	nfapi_tl_t tl;
	uint32_t value;
} nfapi_uint32_tlv_t;

typedef struct {
	nfapi_tl_t tl;
	int64_t value;
} nfapi_int64_tlv_t;

typedef struct {
	nfapi_tl_t tl;
	uint64_t value;
} nfapi_uint64_tlv_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t value;
} nfapi_uint16_tlv_t;

typedef struct {
	nfapi_tl_t tl;
	int16_t value;
} nfapi_int16_tlv_t;

typedef struct {
	nfapi_tl_t tl;
	uint8_t value;
} nfapi_uint8_tlv_t;

typedef struct {
	nfapi_uint16_tlv_t phy_state;
} nfapi_l1_status;

#define NFAPI_L1_STATUS_PHY_STATE_TAG 0x00FA

typedef struct {
	nfapi_uint16_tlv_t dl_bandwidth_support;
	nfapi_uint16_tlv_t ul_bandwidth_support;
	nfapi_uint16_tlv_t dl_modulation_support;
	nfapi_uint16_tlv_t ul_modulation_support;
	nfapi_uint16_tlv_t phy_antenna_capability;
	nfapi_uint16_tlv_t release_capability;
	nfapi_uint16_tlv_t mbsfn_capability;
} nfapi_phy_capabilities_t;

#define NFAPI_PHY_CAPABILITIES_DL_BANDWIDTH_SUPPORT_TAG 0x00C8
#define NFAPI_PHY_CAPABILITIES_UL_BANDWIDTH_SUPPORT_TAG 0x00C9
#define NFAPI_PHY_CAPABILITIES_DL_MODULATION_SUPPORT_TAG 0x00CA
#define NFAPI_PHY_CAPABILITIES_UL_MODULATION_SUPPORT_TAG 0x00CB
#define NFAPI_PHY_CAPABILITIES_PHY_ANTENNA_CAPABILITY_TAG 0x00CC
#define NFAPI_PHY_CAPABILITIES_RELEASE_CAPABILITY_TAG 0x00CD
#define NFAPI_PHY_CAPABILITIES_MBSFN_CAPABILITY_TAG 0x00CE


typedef struct {
	nfapi_uint16_tlv_t data_report_mode;
	nfapi_uint16_tlv_t sfnsf;
} nfapi_l23_config_t;


#define NFAPI_L23_CONFIG_DATA_REPORT_MODE_TAG 0x00F0
#define NFAPI_L23_CONFIG_SFNSF_TAG 0x00F1

typedef struct {
  nfapi_uint16_tlv_t numerology_index_mu;
	nfapi_uint16_tlv_t duplex_mode;
	nfapi_uint16_tlv_t pcfich_power_offset;
	nfapi_uint16_tlv_t pb;
	nfapi_uint16_tlv_t dl_cyclic_prefix_type;
	nfapi_uint16_tlv_t ul_cyclic_prefix_type;
} nfapi_subframe_config_t;

#define NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG 0x0001
#define NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG 0x0002
#define NFAPI_SUBFRAME_CONFIG_PB_TAG 0x0003
#define NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG 0x0004
#define NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG 0x0005
#define NFAPI_SUBFRAME_CONFIG_NUMEROLOGY_INDEX_MU_TAG 0x0006

typedef struct {
	nfapi_uint16_tlv_t dl_channel_bandwidth;
	nfapi_uint16_tlv_t ul_channel_bandwidth;
	nfapi_uint16_tlv_t reference_signal_power;
	nfapi_uint16_tlv_t tx_antenna_ports;
	nfapi_uint16_tlv_t rx_antenna_ports;
} nfapi_rf_config_t;

#define NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG 0x000A
#define NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG 0x000B
#define NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG 0x000C
#define NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG 0x000D
#define NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG 0x000E

typedef struct {
	nfapi_uint16_tlv_t phich_resource;
	nfapi_uint16_tlv_t phich_duration;
	nfapi_uint16_tlv_t phich_power_offset;
} nfapi_phich_config_t;

#define NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG 0x0014
#define NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG 0x0015
#define NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG 0x0016

typedef struct {
	nfapi_uint16_tlv_t primary_synchronization_signal_epre_eprers;
	nfapi_uint16_tlv_t secondary_synchronization_signal_epre_eprers;
	nfapi_uint16_tlv_t physical_cell_id;
  nfapi_uint16_tlv_t half_frame_index;
  nfapi_uint16_tlv_t ssb_subcarrier_offset;
  nfapi_uint16_tlv_t ssb_position_in_burst;
  nfapi_uint16_tlv_t ssb_periodicity;
  nfapi_uint16_tlv_t ss_pbch_block_power;
  nfapi_uint16_tlv_t n_ssb_crb;
} nfapi_sch_config_t;

#define NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG 0x001E
#define NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG 0x001F
#define NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG 0x0020
#define NFAPI_SCH_CONFIG_HALF_FRAME_INDEX_TAG 0x0021
#define NFAPI_SCH_CONFIG_SSB_SUBCARRIER_OFFSET_TAG 0x0022
#define NFAPI_SCH_CONFIG_SSB_POSITION_IN_BURST 0x0023
#define NFAPI_SCH_CONFIG_SSB_PERIODICITY 0x0024
#define NFAPI_SCH_CONFIG_SS_PBCH_BLOCK_POWER 0x0025
#define NFAPI_SCH_CONFIG_N_SSB_CRB 0x0025

typedef struct {
	nfapi_uint16_tlv_t configuration_index;
	nfapi_uint16_tlv_t root_sequence_index;
	nfapi_uint16_tlv_t zero_correlation_zone_configuration;
	nfapi_uint16_tlv_t high_speed_flag;
	nfapi_uint16_tlv_t frequency_offset;
} nfapi_prach_config_t;

#define NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG 0x0028
#define NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG 0x0029
#define NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG 0x002A
#define NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG 0x002B
#define NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG 0x002C

typedef struct {
	nfapi_uint16_tlv_t hopping_mode;
	nfapi_uint16_tlv_t hopping_offset;
	nfapi_uint16_tlv_t number_of_subbands;
} nfapi_pusch_config_t;

#define NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG 0x0032
#define NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG 0x0033
#define NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG 0x0034

typedef struct {
	nfapi_uint16_tlv_t delta_pucch_shift;
	nfapi_uint16_tlv_t n_cqi_rb;
	nfapi_uint16_tlv_t n_an_cs;
	nfapi_uint16_tlv_t n1_pucch_an;
} nfapi_pucch_config_t;

#define NFAPI_PUCCH_CONFIG_DELTA_PUCCH_SHIFT_TAG 0x003C
#define NFAPI_PUCCH_CONFIG_N_CQI_RB_TAG 0x003D
#define NFAPI_PUCCH_CONFIG_N_AN_CS_TAG 0x003E
#define NFAPI_PUCCH_CONFIG_N1_PUCCH_AN_TAG 0x003F

typedef struct{
       nfapi_uint8_tlv_t mbsfn_area_idx;
       nfapi_uint16_tlv_t mbsfn_area_id_r9;
} nfapi_embms_sib13_config_t;
#define NFAPI_EMBMS_MBSFN_CONFIG_AREA_IDX_TAG 0x0039
#define NFAPI_EMBMS_MBSFN_CONFIG_AREA_IDR9_TAG 0x0040

typedef struct {
       nfapi_tl_t tl;
       uint16_t  num_mbsfn_config;
       uint16_t  radioframe_allocation_period[8];
       uint16_t  radioframe_allocation_offset[8];
       uint8_t  fourframes_flag[8];
       int32_t  mbsfn_subframeconfig[8];
} nfapi_embms_mbsfn_config_t;
#define NFAPI_EMBMS_MBSFN_CONFIG_TAG 0x0041

typedef struct {
       nfapi_uint8_tlv_t radioframe_allocation_period;
       nfapi_uint8_tlv_t radioframe_allocation_offset;
       nfapi_uint8_tlv_t non_mbsfn_config_flag;
       nfapi_uint16_tlv_t non_mbsfn_subframeconfig;
} nfapi_fembms_config_t;

#define NFAPI_FEMBMS_CONFIG_RADIOFRAME_ALLOCATION_PERIOD_TAG 0x0042
#define NFAPI_FEMBMS_CONFIG_RADIOFRAME_ALLOCATION_OFFSET_TAG 0x0043
#define NFAPI_FEMBMS_CONFIG_NON_MBSFN_FLAG_TAG 0x0044
#define NFAPI_FEMBMS_CONFIG_NON_MBSFN_SUBFRAMECONFIG_TAG 0x0045

typedef struct {
	nfapi_uint16_tlv_t bandwidth_configuration;
	nfapi_uint16_tlv_t max_up_pts;
	nfapi_uint16_tlv_t srs_subframe_configuration;
	nfapi_uint16_tlv_t srs_acknack_srs_simultaneous_transmission;
} nfapi_srs_config_t;

#define NFAPI_SRS_CONFIG_BANDWIDTH_CONFIGURATION_TAG 0x0046
#define NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG 0x0047
#define NFAPI_SRS_CONFIG_SRS_SUBFRAME_CONFIGURATION_TAG 0x0048
#define NFAPI_SRS_CONFIG_SRS_ACKNACK_SRS_SIMULTANEOUS_TRANSMISSION_TAG 0x0049

typedef struct {
	nfapi_uint16_tlv_t uplink_rs_hopping;
	nfapi_uint16_tlv_t group_assignment;
	nfapi_uint16_tlv_t cyclic_shift_1_for_drms;
} nfapi_uplink_reference_signal_config_t;

#define NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG 0x0050
#define NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG 0x0051
#define NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG 0x0052


typedef struct {
	nfapi_uint16_tlv_t ed_threshold_lbt_pdsch;
	nfapi_uint16_tlv_t ed_threshold_lbt_drs;
	nfapi_uint16_tlv_t pd_threshold;
	nfapi_uint16_tlv_t multi_carrier_type;
	nfapi_uint16_tlv_t multi_carrier_tx;
	nfapi_uint16_tlv_t multi_carrier_freeze;
	nfapi_uint16_tlv_t tx_antenna_ports_drs;
	nfapi_uint16_tlv_t tx_power_drs;
} nfapi_laa_config_t;

#define NFAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_PDSCH_TAG 0x0064
#define NFAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_DRS_TAG 0x0065
#define NFAPI_LAA_CONFIG_PD_THRESHOLD_TAG 0x0066
#define NFAPI_LAA_CONFIG_MULTI_CARRIER_TYPE_TAG 0x0067
#define NFAPI_LAA_CONFIG_MULTI_CARRIER_TX_TAG 0x0068
#define NFAPI_LAA_CONFIG_MULTI_CARRIER_FREEZE_TAG 0x0069
#define NFAPI_LAA_CONFIG_TX_ANTENNA_PORTS_FOR_DRS_TAG 0x006A
#define NFAPI_LAA_CONFIG_TRANSMISSION_POWER_FOR_DRS_TAG 0x006B

typedef struct {

	nfapi_uint16_tlv_t pbch_repetitions_enable_r13;
	nfapi_uint16_tlv_t prach_catm_root_sequence_index;
	nfapi_uint16_tlv_t prach_catm_zero_correlation_zone_configuration;
	nfapi_uint16_tlv_t prach_catm_high_speed_flag;
	nfapi_uint16_tlv_t prach_ce_level_0_enable;
	nfapi_uint16_tlv_t prach_ce_level_0_configuration_index;
	nfapi_uint16_tlv_t prach_ce_level_0_frequency_offset;
	nfapi_uint16_tlv_t prach_ce_level_0_number_of_repetitions_per_attempt;
	nfapi_uint16_tlv_t prach_ce_level_0_starting_subframe_periodicity;
	nfapi_uint16_tlv_t prach_ce_level_0_hopping_enable;
	nfapi_uint16_tlv_t prach_ce_level_0_hopping_offset;
	nfapi_uint16_tlv_t prach_ce_level_1_enable;
	nfapi_uint16_tlv_t prach_ce_level_1_configuration_index;
	nfapi_uint16_tlv_t prach_ce_level_1_frequency_offset;
	nfapi_uint16_tlv_t prach_ce_level_1_number_of_repetitions_per_attempt;
	nfapi_uint16_tlv_t prach_ce_level_1_starting_subframe_periodicity;
	nfapi_uint16_tlv_t prach_ce_level_1_hopping_enable;
	nfapi_uint16_tlv_t prach_ce_level_1_hopping_offset;
	nfapi_uint16_tlv_t prach_ce_level_2_enable;
	nfapi_uint16_tlv_t prach_ce_level_2_configuration_index;
	nfapi_uint16_tlv_t prach_ce_level_2_frequency_offset;
	nfapi_uint16_tlv_t prach_ce_level_2_number_of_repetitions_per_attempt;
	nfapi_uint16_tlv_t prach_ce_level_2_starting_subframe_periodicity;
	nfapi_uint16_tlv_t prach_ce_level_2_hopping_enable;
	nfapi_uint16_tlv_t prach_ce_level_2_hopping_offset;
	nfapi_uint16_tlv_t prach_ce_level_3_enable;
	nfapi_uint16_tlv_t prach_ce_level_3_configuration_index;
	nfapi_uint16_tlv_t prach_ce_level_3_frequency_offset;
	nfapi_uint16_tlv_t prach_ce_level_3_number_of_repetitions_per_attempt;
	nfapi_uint16_tlv_t prach_ce_level_3_starting_subframe_periodicity;
	nfapi_uint16_tlv_t prach_ce_level_3_hopping_enable;
	nfapi_uint16_tlv_t prach_ce_level_3_hopping_offset;
	nfapi_uint16_tlv_t pucch_interval_ulhoppingconfigcommonmodea;
	nfapi_uint16_tlv_t pucch_interval_ulhoppingconfigcommonmodeb;
} nfapi_emtc_config_t;

#define NFAPI_EMTC_CONFIG_PBCH_REPETITIONS_ENABLE_R13_TAG 0x0078
#define NFAPI_EMTC_CONFIG_PRACH_CATM_ROOT_SEQUENCE_INDEX_TAG 0x0079
#define NFAPI_EMTC_CONFIG_PRACH_CATM_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG 0x007A
#define NFAPI_EMTC_CONFIG_PRACH_CATM_HIGH_SPEED_FLAG 0x007B
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_ENABLE_TAG 0x007C
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_CONFIGURATION_INDEX_TAG 0x007D
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_FREQUENCY_OFFSET_TAG 0x007E
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG 0x007F
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_STARTING_SUBFRAME_PERIODICITY_TAG 0x0080
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_ENABLE_TAG 0x0081
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_OFFSET_TAG 0x0082
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_ENABLE_TAG 0x0083
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_CONFIGURATION_INDEX_TAG 0x0084
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_FREQUENCY_OFFSET_TAG 0x0085
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG 0x0086
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_STARTING_SUBFRAME_PERIODICITY_TAG 0x0087
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_ENABLE_TAG 0x0088
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_OFFSET_TAG 0x0089
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_ENABLE_TAG 0x008A
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_CONFIGURATION_INDEX_TAG 0x008B
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_FREQUENCY_OFFSET_TAG 0x008C
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG 0x008D
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_STARTING_SUBFRAME_PERIODICITY_TAG 0x008E
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_ENABLE_TAG 0x008F
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_OFFSET_TAG 0x0090
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_ENABLE_TAG 0x0091
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_CONFIGURATION_INDEX_TAG 0x0092
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_FREQUENCY_OFFSET_TAG 0x0093
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG 0x0094
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_STARTING_SUBFRAME_PERIODICITY_TAG 0x0095
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_ENABLE_TAG 0x0096
#define NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_OFFSET_TAG 0x0097
#define NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEA_TAG 0x0098
#define NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEB_TAG 0x0099

typedef struct {
	nfapi_uint16_tlv_t operating_mode;
	nfapi_uint16_tlv_t anchor;
	nfapi_uint16_tlv_t prb_index;
	nfapi_uint16_tlv_t control_region_size;
	nfapi_uint16_tlv_t assumed_crs_aps;
	nfapi_uint16_tlv_t nprach_config_0_enabled;
	nfapi_uint16_tlv_t nprach_config_0_sf_periodicity;
	nfapi_uint16_tlv_t nprach_config_0_start_time;
	nfapi_uint16_tlv_t nprach_config_0_subcarrier_offset;
	nfapi_uint16_tlv_t nprach_config_0_number_of_subcarriers;
	nfapi_uint16_tlv_t nprach_config_0_cp_length;
	nfapi_uint16_tlv_t nprach_config_0_number_of_repetitions_per_attempt;
	nfapi_uint16_tlv_t nprach_config_1_enabled;
	nfapi_uint16_tlv_t nprach_config_1_sf_periodicity;
	nfapi_uint16_tlv_t nprach_config_1_start_time;
	nfapi_uint16_tlv_t nprach_config_1_subcarrier_offset;
	nfapi_uint16_tlv_t nprach_config_1_number_of_subcarriers;
	nfapi_uint16_tlv_t nprach_config_1_cp_length;
	nfapi_uint16_tlv_t nprach_config_1_number_of_repetitions_per_attempt;
	nfapi_uint16_tlv_t nprach_config_2_enabled;
	nfapi_uint16_tlv_t nprach_config_2_sf_periodicity;
	nfapi_uint16_tlv_t nprach_config_2_start_time;
	nfapi_uint16_tlv_t nprach_config_2_subcarrier_offset;
	nfapi_uint16_tlv_t nprach_config_2_number_of_subcarriers;
	nfapi_uint16_tlv_t nprach_config_2_cp_length;
	nfapi_uint16_tlv_t nprach_config_2_number_of_repetitions_per_attempt;
	nfapi_uint16_tlv_t three_tone_base_sequence;
	nfapi_uint16_tlv_t six_tone_base_sequence;
	nfapi_uint16_tlv_t twelve_tone_base_sequence;
	nfapi_uint16_tlv_t three_tone_cyclic_shift;
	nfapi_uint16_tlv_t six_tone_cyclic_shift;
	nfapi_uint16_tlv_t dl_gap_config_enable;
	nfapi_uint16_tlv_t dl_gap_threshold;
	nfapi_uint16_tlv_t dl_gap_periodicity;
	nfapi_uint16_tlv_t dl_gap_duration_coefficient;
} nfapi_nb_iot_config_t;

#define NFAPI_NB_IOT_CONFIG_OPERATING_MODE_TAG 0x00A5
#define NFAPI_NB_IOT_CONFIG_ANCHOR_TAG 0x00A6
#define NFAPI_NB_IOT_CONFIG_PRB_INDEX_TAG 0x00A7
#define NFAPI_NB_IOT_CONFIG_CONTROL_REGION_SIZE_TAG 0x00A8
#define NFAPI_NB_IOT_CONFIG_ASSUMED_CRS_APS_TAG 0x00A9
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_ENABLED_TAG 0x00AA
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_SF_PERIODICITY_TAG 0x00AB
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_START_TIME_TAG 0x00AC
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_SUBCARRIER_OFFSET_TAG 0x00AD
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_NUMBER_OF_SUBCARRIERS_TAG 0x00AE
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_CP_LENGTH_TAG 0x00AF
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG 0x00B0
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_ENABLED_TAG 0x00B1
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_SF_PERIODICITY_TAG 0x00B2
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_START_TIME_TAG 0x00B3
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_SUBCARRIER_OFFSET_TAG 0x00B4
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_NUMBER_OF_SUBCARRIERS_TAG 0x00B5
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_CP_LENGTH_TAG 0x00B6
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG 0x00B7
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_ENABLED_TAG 0x00B8
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_SF_PERIODICITY_TAG 0x00B9
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_START_TIME_TAG 0x00BA
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_SUBCARRIER_OFFSET_TAG 0x00BB
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_NUMBER_OF_SUBCARRIERS_TAG 0x00BC
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_CP_LENGTH_TAG 0x00BD
#define NFAPI_NB_IOT_CONFIG_NPRACH_CONFIG_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG 0x00BE
#define NFAPI_NB_IOT_CONFIG_THREE_TONE_BASE_SEQUENCE_TAG 0x00BF
#define NFAPI_NB_IOT_CONFIG_SIX_TONE_BASE_SEQUENCE_TAG 0x00C0
#define NFAPI_NB_IOT_CONFIG_TWELVE_TONE_BASE_SEQUENCE_TAG 0x00C1
#define NFAPI_NB_IOT_CONFIG_THREE_TONE_CYCLIC_SHIFT_TAG 0x00C2
#define NFAPI_NB_IOT_CONFIG_SIX_TONE_CYCLIC_SHIFT_TAG 0x00C3
#define NFAPI_NB_IOT_CONFIG_DL_GAP_CONFIG_ENABLE_TAG 0x00C4
#define NFAPI_NB_IOT_CONFIG_DL_GAP_THRESHOLD_TAG 0x00C5
#define NFAPI_NB_IOT_CONFIG_DL_GAP_PERIODICITY_TAG 0x00C6
#define NFAPI_NB_IOT_CONFIG_DL_GAP_DURATION_COEFFICIENT_TAG 0x00C7

typedef struct {
	nfapi_uint16_tlv_t laa_support;
	nfapi_uint16_tlv_t pd_sensing_lbt_support;
	nfapi_uint16_tlv_t multi_carrier_lbt_support;
	nfapi_uint16_tlv_t partial_sf_support;
} nfapi_laa_capability_t;

#define NFAPI_LAA_CAPABILITY_LAA_SUPPORT_TAG 0x00D1
#define NFAPI_LAA_CAPABILITY_PD_SENSING_LBT_SUPPORT_TAG 0x00D2
#define NFAPI_LAA_CAPABILITY_MULTI_CARRIER_LBT_SUPPORT_TAG 0x00D3
#define NFAPI_LAA_CAPABILITY_PARTIAL_SF_SUPPORT_TAG 0x00D4

typedef struct {
	nfapi_uint16_tlv_t nb_iot_support;
	nfapi_uint16_tlv_t nb_iot_operating_mode_capability;
} nfapi_nb_iot_capability_t;

#define NFAPI_LAA_CAPABILITY_NB_IOT_SUPPORT_TAG 0x00D5
#define NFAPI_LAA_CAPABILITY_NB_IOT_OPERATING_MODE_CAPABILITY_TAG 0x00D6

typedef struct {
	nfapi_uint16_tlv_t subframe_assignment;
	nfapi_uint16_tlv_t special_subframe_patterns;
} nfapi_tdd_frame_structure_t;

#define NFAPI_TDD_FRAME_STRUCTURE_SUBFRAME_ASSIGNMENT_TAG 0x005A
#define NFAPI_TDD_FRAME_STRUCTURE_SPECIAL_SUBFRAME_PATTERNS_TAG 0x005B

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_rf_bands;
	uint16_t rf_band[NFAPI_MAX_NUM_RF_BANDS];
} nfapi_rf_bands_t;
#define NFAPI_PHY_RF_BANDS_TAG 0x0114

#define NFAPI_IPV4_ADDRESS_LENGTH 4
#define NFAPI_IPV6_ADDRESS_LENGTH 16

// Convience enum to allow the ip addres type to be distinguished
typedef enum {
	NFAPI_IP_ADDRESS_IPV4 = 0,
	NFAPI_IP_ADDRESS_IPV6
} nfapi_ip_address_type_e;

// The type could be infered from the length, but it is clearer in 
// code to have a type variable set
typedef struct {
	nfapi_tl_t tl;
	uint8_t type;
	union {
		uint8_t ipv4_address[NFAPI_IPV4_ADDRESS_LENGTH];
		uint8_t ipv6_address[NFAPI_IPV6_ADDRESS_LENGTH];
	} u;
} nfapi_ip_address_t;

typedef struct {
	nfapi_tl_t tl;
	uint8_t address[NFAPI_IPV4_ADDRESS_LENGTH];
} nfapi_ipv4_address_t;

typedef struct {
	nfapi_tl_t tl;
	uint8_t address[NFAPI_IPV6_ADDRESS_LENGTH];
} nfapi_ipv6_address_t;



typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_rf_bands;
	uint16_t bands[NFAPI_MAX_NMM_FREQUENCY_BANDS];
} nfapi_nmm_frequency_bands_t;

//These TLVs are used exclusively by nFAPI
typedef struct
{
	// These TLVs are used to setup the transport connection between VNF and PNF
	nfapi_ipv4_address_t p7_vnf_address_ipv4;
	nfapi_ipv6_address_t p7_vnf_address_ipv6;
	nfapi_uint16_tlv_t p7_vnf_port;

	nfapi_ipv4_address_t p7_pnf_address_ipv4;
	nfapi_ipv6_address_t p7_pnf_address_ipv6;
	nfapi_uint16_tlv_t p7_pnf_port;
	
	// These TLVs are used to setup the transport connection between VNF and PNF
	nfapi_uint8_tlv_t dl_ue_per_sf;
	nfapi_uint8_tlv_t ul_ue_per_sf;

	// These TLVs are used by PNF to report its RF capabilities to the VNF software
	nfapi_rf_bands_t rf_bands;

	// These TLVs are used by the VNF to configure the synchronization with the PNF.
	nfapi_uint8_tlv_t timing_window;
	nfapi_uint8_tlv_t timing_info_mode;
	nfapi_uint8_tlv_t timing_info_period;

	// These TLVs are used by the VNF to configure the RF in the PNF
	nfapi_uint16_tlv_t max_transmit_power;
	nfapi_uint16_tlv_t earfcn;

	nfapi_nmm_frequency_bands_t nmm_gsm_frequency_bands;
	nfapi_nmm_frequency_bands_t nmm_umts_frequency_bands;
	nfapi_nmm_frequency_bands_t nmm_lte_frequency_bands;
	nfapi_uint8_tlv_t nmm_uplink_rssi_supported;

} nfapi_nfapi_t;

#define NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG 0x0100
#define NFAPI_NFAPI_P7_VNF_ADDRESS_IPV6_TAG 0x0101
#define NFAPI_NFAPI_P7_VNF_PORT_TAG 0x0102
#define NFAPI_NFAPI_P7_PNF_ADDRESS_IPV4_TAG 0x0103
#define NFAPI_NFAPI_P7_PNF_ADDRESS_IPV6_TAG 0x0104
#define NFAPI_NFAPI_P7_PNF_PORT_TAG 0x0105

#define NFAPI_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG 0x010A
#define NFAPI_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG 0x010B
#define NFAPI_NFAPI_RF_BANDS_TAG 0x0114
#define NFAPI_NFAPI_TIMING_WINDOW_TAG 0x011E
#define NFAPI_NFAPI_TIMING_INFO_MODE_TAG 0x011F
#define NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG 0x0120
#define NFAPI_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG 0x0128
#define NFAPI_NFAPI_EARFCN_TAG 0x0129
#define NFAPI_NFAPI_NMM_GSM_FREQUENCY_BANDS_TAG 0x0130
#define NFAPI_NFAPI_NMM_UMTS_FREQUENCY_BANDS_TAG 0x0131
#define NFAPI_NFAPI_NMM_LTE_FREQUENCY_BANDS_TAG 0x0132
#define NFAPI_NFAPI_NMM_UPLINK_RSSI_SUPPORTED_TAG 0x0133


// P5 Message Structures
typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_pnf_param_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_pnf_param_general_t pnf_param_general;
	nfapi_pnf_phy_t pnf_phy;
	nfapi_pnf_rf_t pnf_rf;
	nfapi_pnf_phy_rel10_t pnf_phy_rel10;
	nfapi_pnf_phy_rel11_t pnf_phy_rel11;
	nfapi_pnf_phy_rel12_t pnf_phy_rel12;
	nfapi_pnf_phy_rel13_t pnf_phy_rel13;
	nfapi_pnf_phy_rel13_nb_iot_t pnf_phy_rel13_nb_iot;
  nfapi_pnf_phy_rel15_t pnf_phy_rel15;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_pnf_param_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint8_t num_tlvs;
	nfapi_pnf_phy_rf_config_t pnf_phy_rf_config;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_pnf_config_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_pnf_config_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_pnf_start_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_pnf_start_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_pnf_stop_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_pnf_stop_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_param_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint8_t error_code;
	uint8_t num_tlv;
	// fdd or tdd in idle or configured tlvs
	nfapi_l1_status l1_status;
	nfapi_phy_capabilities_t phy_capabilities;
	nfapi_laa_capability_t laa_capability;
	nfapi_nb_iot_capability_t nb_iot_capability;
	
	nfapi_subframe_config_t subframe_config;
	nfapi_rf_config_t rf_config;
	nfapi_phich_config_t phich_config;
	nfapi_sch_config_t sch_config;
	nfapi_prach_config_t prach_config;
	nfapi_pusch_config_t pusch_config;
	nfapi_pucch_config_t pucch_config;
        // addition nfpai tlvs for embms MBSFN config //TOBE REVIEWED
        nfapi_embms_sib13_config_t embms_sib13_config;
        nfapi_embms_mbsfn_config_t embms_mbsfn_config;
	nfapi_fembms_config_t fembms_config;
	nfapi_srs_config_t srs_config;
	nfapi_uplink_reference_signal_config_t uplink_reference_signal_config;
	nfapi_tdd_frame_structure_t tdd_frame_structure_config;
	nfapi_l23_config_t l23_config;
	nfapi_nb_iot_config_t nb_iot_config;

	// addition nfapi tlvs as per table 2-16 in idle or configure
	nfapi_nfapi_t nfapi_config;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_param_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint8_t num_tlv;
	nfapi_subframe_config_t subframe_config;
	nfapi_rf_config_t rf_config;
	nfapi_phich_config_t phich_config;
	nfapi_sch_config_t sch_config;
	nfapi_prach_config_t prach_config;
	nfapi_pusch_config_t pusch_config;
	nfapi_pucch_config_t pucch_config;
        // addition nfpai tlvs for embms MBSFN config //TOBE REVIEWED
        nfapi_embms_sib13_config_t embms_sib13_config;
        nfapi_embms_mbsfn_config_t embms_mbsfn_config;
        nfapi_fembms_config_t fembms_config;
	nfapi_srs_config_t srs_config;
	nfapi_uplink_reference_signal_config_t uplink_reference_signal_config;
	nfapi_laa_config_t laa_config;
	nfapi_emtc_config_t emtc_config;
	nfapi_tdd_frame_structure_t tdd_frame_structure_config;
	nfapi_l23_config_t l23_config;
	nfapi_nb_iot_config_t nb_iot_config;
	
	// addition nfapi tlvs as per table 2-16 in idle or configure
	nfapi_nfapi_t nfapi_config;

	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_config_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_config_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_start_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_start_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_stop_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_stop_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_uint16_tlv_t dl_rs_tx_power;
	nfapi_uint16_tlv_t received_interference_power;
	nfapi_uint16_tlv_t thermal_noise_power;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_measurement_request_t;

#define NFAPI_MEASUREMENT_REQUEST_DL_RS_XTX_POWER_TAG 0x1004
#define NFAPI_MEASUREMENT_REQUEST_RECEIVED_INTERFERENCE_POWER_TAG 0x1005
#define NFAPI_MEASUREMENT_REQUEST_THERMAL_NOISE_POWER_TAG 0x1006



typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_resource_blocks;
	int16_t received_interference_power[NFAPI_MAX_RECEIVED_INTERFERENCE_POWER_RESULTS];
} nfapi_received_interference_power_measurement_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_int16_tlv_t dl_rs_tx_power_measurement;
	nfapi_received_interference_power_measurement_t received_interference_power_measurement;
	nfapi_int16_tlv_t thermal_noise_power_measurement;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_measurement_response_t;

#define NFAPI_MEASUREMENT_RESPONSE_DL_RS_POWER_MEASUREMENT_TAG 0x1007
#define NFAPI_MEASUREMENT_RESPONSE_RECEIVED_INTERFERENCE_POWER_MEASUREMENT_TAG 0x1008
#define NFAPI_MEASUREMENT_RESPONSE_THERMAL_NOISE_MEASUREMENT_TAG 0x1009

// P7 Sub Structures
typedef struct {
	nfapi_tl_t tl;
	uint8_t dci_format;
	uint8_t cce_idx;
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
	uint8_t new_data_indicator_2;
	uint8_t harq_process;
	uint8_t tpmi;
	uint8_t pmi;
	uint8_t precoding_information;
	uint8_t tpc;
	uint8_t downlink_assignment_index;
	uint8_t ngap;
	uint8_t transport_block_size_index;
	uint8_t downlink_power_offset;
	uint8_t allocate_prach_flag;
	uint8_t preamble_index;
	uint8_t prach_mask_index;
	uint8_t rnti_type;
	uint16_t transmission_power;
} nfapi_dl_config_dci_dl_pdu_rel8_t;
#define NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG 0x2001

typedef struct {
	nfapi_tl_t tl;
	uint8_t mcch_flag;
	uint8_t mcch_change_notification;
	uint8_t scrambling_identity;
} nfapi_dl_config_dci_dl_pdu_rel9_t;
#define NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL9_TAG 0x2002

typedef struct {
	nfapi_tl_t tl;
	uint8_t cross_carrier_scheduling_flag;
	uint8_t carrier_indicator;
	uint8_t srs_flag;
	uint8_t srs_request;
	uint8_t antenna_ports_scrambling_and_layers;
	uint8_t total_dci_length_including_padding;
	uint8_t n_dl_rb;
} nfapi_dl_config_dci_dl_pdu_rel10_t;
#define NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL10_TAG 0x2003


typedef struct {
	nfapi_tl_t tl;
	uint8_t harq_ack_resource_offset;
	uint8_t pdsch_re_mapping_quasi_co_location_indicator;
} nfapi_dl_config_dci_dl_pdu_rel11_t;

#define NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL11_TAG 0x2039



typedef struct {
	nfapi_tl_t tl;
	uint8_t primary_cell_type;
	uint8_t ul_dl_configuration_flag;
	uint8_t number_ul_dl_configurations;
	uint8_t ul_dl_configuration_indication[NFAPI_MAX_UL_DL_CONFIGURATIONS];
} nfapi_dl_config_dci_dl_pdu_rel12_t;

#define NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL12_TAG 0x203a



typedef struct {
	uint8_t subband_index;
	uint8_t scheduled_ues;
	uint16_t precoding_value[NFAPI_MAX_NUM_PHYSICAL_ANTENNAS][NFAPI_MAX_NUM_SCHEDULED_UES];
} nfapi_dl_config_dci_dl_tpm_subband_info_t;

typedef struct {
	uint8_t num_prb_per_subband;
	uint8_t number_of_subbands;
	uint8_t num_antennas;
	nfapi_dl_config_dci_dl_tpm_subband_info_t subband_info[NFAPI_MAX_NUM_SUBBANDS];
} nfapi_dl_config_dci_dl_tpm_t;

typedef struct {
	nfapi_tl_t tl;
	uint8_t laa_end_partial_sf_flag;
	uint8_t laa_end_partial_sf_configuration;
	uint8_t initial_lbt_sf;
	uint8_t codebook_size_determination;
	uint8_t drms_table_flag;
	uint8_t tpm_struct_flag;
	nfapi_dl_config_dci_dl_tpm_t tpm;
} nfapi_dl_config_dci_dl_pdu_rel13_t;

#define NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL13_TAG 0x203b

typedef struct {
	nfapi_dl_config_dci_dl_pdu_rel8_t dci_dl_pdu_rel8;
	nfapi_dl_config_dci_dl_pdu_rel9_t dci_dl_pdu_rel9;
	nfapi_dl_config_dci_dl_pdu_rel10_t dci_dl_pdu_rel10;
	nfapi_dl_config_dci_dl_pdu_rel11_t dci_dl_pdu_rel11;
	nfapi_dl_config_dci_dl_pdu_rel12_t dci_dl_pdu_rel12;
	nfapi_dl_config_dci_dl_pdu_rel13_t dci_dl_pdu_rel13;
} nfapi_dl_config_dci_dl_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint16_t length;
	int16_t pdu_index;
	uint16_t transmission_power;
} nfapi_dl_config_bch_pdu_rel8_t;
#define NFAPI_DL_CONFIG_REQUEST_BCH_PDU_REL8_TAG 0x2004

typedef struct {
	nfapi_dl_config_bch_pdu_rel8_t bch_pdu_rel8;
} nfapi_dl_config_bch_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint16_t length;
	int16_t pdu_index;
	uint16_t rnti;
	uint8_t resource_allocation_type;
	uint32_t resource_block_coding;
	uint8_t modulation;
	uint16_t transmission_power;
	uint16_t mbsfn_area_id;
} nfapi_dl_config_mch_pdu_rel8_t;
#define NFAPI_DL_CONFIG_REQUEST_MCH_PDU_REL8_TAG 0x2005

typedef struct {
	nfapi_dl_config_mch_pdu_rel8_t mch_pdu_rel8;
} nfapi_dl_config_mch_pdu;


typedef struct {
	uint8_t subband_index;
	uint8_t num_antennas;
	uint16_t bf_value[NFAPI_MAX_NUM_ANTENNAS];
} nfapi_bf_vector_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t length;
	int16_t pdu_index;
	uint16_t rnti;
	uint8_t resource_allocation_type;
	uint8_t virtual_resource_block_assignment_flag;
	uint32_t resource_block_coding;
	uint8_t modulation;
	uint8_t redundancy_version;
	uint8_t transport_blocks;
	uint8_t transport_block_to_codeword_swap_flag;
	uint8_t transmission_scheme;
	uint8_t number_of_layers;
	uint8_t number_of_subbands;
	uint8_t codebook_index[NFAPI_MAX_NUM_SUBBANDS];
	uint8_t ue_category_capacity;
	uint8_t pa;
	uint8_t delta_power_offset_index;
	uint8_t ngap;
	uint8_t nprb;
	uint8_t transmission_mode;
	uint8_t num_bf_prb_per_subband;
	uint8_t num_bf_vector;
	nfapi_bf_vector_t bf_vector[NFAPI_MAX_BF_VECTORS];
} nfapi_dl_config_dlsch_pdu_rel8_t;
#define NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL8_TAG 0x2006

typedef struct {
	nfapi_tl_t tl;
	uint8_t nscid;
} nfapi_dl_config_dlsch_pdu_rel9_t;
#define NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL9_TAG 0x2007

typedef struct {
	nfapi_tl_t tl;
	uint8_t csi_rs_flag;
	uint8_t csi_rs_resource_config_r10;
	uint16_t csi_rs_zero_tx_power_resource_config_bitmap_r10;
	uint8_t csi_rs_number_nzp_configuration;
	uint8_t csi_rs_resource_config[NFAPI_MAX_CSI_RS_RESOURCE_CONFIG];
	uint8_t pdsch_start;
} nfapi_dl_config_dlsch_pdu_rel10_t;
#define NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL10_TAG 0x2008

typedef struct {
	nfapi_tl_t tl;
	uint8_t drms_config_flag;
	uint16_t drms_scrambling;
	uint8_t csi_config_flag;
	uint16_t csi_scrambling;
	uint8_t pdsch_re_mapping_flag;
	uint8_t pdsch_re_mapping_atenna_ports;
	uint8_t pdsch_re_mapping_freq_shift;
} nfapi_dl_config_dlsch_pdu_rel11_t;
#define NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL11_TAG 0x203C

typedef struct {
	nfapi_tl_t tl;
	uint8_t altcqi_table_r12;
	uint8_t maxlayers;
	uint8_t n_dl_harq;
} nfapi_dl_config_dlsch_pdu_rel12_t;
#define NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL12_TAG 0x203D

typedef struct {
	nfapi_tl_t tl;
	uint8_t dwpts_symbols;
	uint8_t initial_lbt_sf;
	uint8_t ue_type;
	uint8_t pdsch_payload_type;
	uint16_t initial_transmission_sf_io;
	uint8_t drms_table_flag;
} nfapi_dl_config_dlsch_pdu_rel13_t;
#define NFAPI_DL_CONFIG_REQUEST_DLSCH_PDU_REL13_TAG 0x203E

typedef struct {
	nfapi_dl_config_dlsch_pdu_rel8_t dlsch_pdu_rel8;
	nfapi_dl_config_dlsch_pdu_rel9_t dlsch_pdu_rel9;
	nfapi_dl_config_dlsch_pdu_rel10_t dlsch_pdu_rel10;
	nfapi_dl_config_dlsch_pdu_rel11_t dlsch_pdu_rel11;
	nfapi_dl_config_dlsch_pdu_rel12_t dlsch_pdu_rel12;
	nfapi_dl_config_dlsch_pdu_rel13_t dlsch_pdu_rel13;
} nfapi_dl_config_dlsch_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint16_t length;
	int16_t pdu_index;
	uint16_t p_rnti;
	uint8_t resource_allocation_type;
	uint8_t virtual_resource_block_assignment_flag;
	uint32_t resource_block_coding;
	uint8_t mcs;
	uint8_t redundancy_version;
	uint8_t number_of_transport_blocks;
	uint8_t transport_block_to_codeword_swap_flag;
	uint8_t transmission_scheme;
	uint8_t number_of_layers;
	uint8_t codebook_index;
	uint8_t ue_category_capacity;
	uint8_t pa;
	uint16_t transmission_power;
	uint8_t nprb;
	uint8_t ngap;
} nfapi_dl_config_pch_pdu_rel8_t;
#define NFAPI_DL_CONFIG_REQUEST_PCH_PDU_REL8_TAG 0x2009

typedef struct {
	nfapi_tl_t tl;
	uint8_t ue_mode;
	uint16_t initial_transmission_sf_io;
} nfapi_dl_config_pch_pdu_rel13_t;
#define NFAPI_DL_CONFIG_REQUEST_PCH_PDU_REL13_TAG 0x203F

typedef struct {
	nfapi_dl_config_pch_pdu_rel8_t pch_pdu_rel8;
	nfapi_dl_config_pch_pdu_rel13_t pch_pdu_rel13;
} nfapi_dl_config_pch_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint16_t transmission_power;
	uint8_t prs_bandwidth;
	uint8_t prs_cyclic_prefix_type;
	uint8_t prs_muting;
} nfapi_dl_config_prs_pdu_rel9_t;
#define NFAPI_DL_CONFIG_REQUEST_PRS_PDU_REL9_TAG 0x200A

typedef struct {
	nfapi_dl_config_prs_pdu_rel9_t prs_pdu_rel9;
} nfapi_dl_config_prs_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint8_t csi_rs_antenna_port_count_r10;
	uint8_t csi_rs_resource_config_r10;
	uint16_t transmission_power;
	uint16_t csi_rs_zero_tx_power_resource_config_bitmap_r10;
	uint8_t csi_rs_number_of_nzp_configuration;
	uint8_t csi_rs_resource_config[NFAPI_MAX_CSI_RS_RESOURCE_CONFIG];
} nfapi_dl_config_csi_rs_pdu_rel10_t;
#define NFAPI_DL_CONFIG_REQUEST_CSI_RS_PDU_REL10_TAG 0x200B

typedef struct {
	nfapi_tl_t tl;
	uint8_t csi_rs_class;
	uint8_t cdm_type;
	uint8_t num_bf_vector;
	struct {
		uint8_t csi_rs_resource_index;
		uint16_t bf_value[NFAPI_MAX_ANTENNA_PORT_COUNT];
	} bf_vector[NFAPI_MAX_BF_VECTORS];

}nfapi_dl_config_csi_rs_pdu_rel13_t;
#define NFAPI_DL_CONFIG_REQUEST_CSI_RS_PDU_REL13_TAG 0x2040

typedef struct {
	nfapi_dl_config_csi_rs_pdu_rel10_t csi_rs_pdu_rel10;
	nfapi_dl_config_csi_rs_pdu_rel13_t csi_rs_pdu_rel13;
} nfapi_dl_config_csi_rs_pdu;

#define NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL8_TAG 0x2001
#define NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL9_TAG 0x2002
#define NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL10_TAG 0x2003
#define NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL11_TAG 0x2039
#define NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL12_TAG 0x203a
#define NFAPI_DL_CONFIG_REQUEST_EPDCCH_PDU_REL13_TAG 0x203b

typedef struct {
	nfapi_tl_t tl;
	uint8_t epdcch_resource_assignment_flag;
	uint16_t epdcch_id;
	uint8_t epdcch_start_symbol;
	uint8_t epdcch_num_prb;
	uint8_t epdcch_prb_index[NFAPI_MAX_EPDCCH_PRB];
	nfapi_bf_vector_t bf_vector;
} nfapi_dl_config_epdcch_parameters_rel11_t;
#define NFAPI_DL_CONFIG_REQUEST_EPDCCH_PARAM_REL11_TAG 0x2041

typedef struct {
	nfapi_tl_t tl;
	uint8_t dwpts_symbols;
	uint8_t initial_lbt_sf;
} nfapi_dl_config_epdcch_parameters_rel13_t;
#define NFAPI_DL_CONFIG_REQUEST_EPDCCH_PARAM_REL13_TAG 0x2042

typedef struct {
	nfapi_dl_config_dci_dl_pdu_rel8_t			epdcch_pdu_rel8;
	nfapi_dl_config_dci_dl_pdu_rel9_t			epdcch_pdu_rel9;
	nfapi_dl_config_dci_dl_pdu_rel10_t			epdcch_pdu_rel10;
	nfapi_dl_config_dci_dl_pdu_rel11_t			epdcch_pdu_rel11;
	nfapi_dl_config_dci_dl_pdu_rel12_t			epdcch_pdu_rel12;
	nfapi_dl_config_dci_dl_pdu_rel13_t			epdcch_pdu_rel13;
	nfapi_dl_config_epdcch_parameters_rel11_t	epdcch_params_rel11;
	nfapi_dl_config_epdcch_parameters_rel13_t	epdcch_params_rel13;
} nfapi_dl_config_epdcch_pdu;


typedef struct {
	nfapi_tl_t tl;
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
	uint16_t precoding_value[NFAPI_MAX_TX_PHYSICAL_ANTENNA_PORTS];
} nfapi_dl_config_mpdcch_pdu_rel13_t;
#define NFAPI_DL_CONFIG_REQUEST_MPDCCH_PDU_REL13_TAG 0x205B


typedef struct {
	nfapi_dl_config_mpdcch_pdu_rel13_t mpdcch_pdu_rel13;
} nfapi_dl_config_mpdcch_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint16_t length;
	int16_t pdu_index;
	uint16_t transmission_power;
	uint16_t hyper_sfn_2_lsbs;
} nfapi_dl_config_nbch_pdu_rel13_t;

#define NFAPI_DL_CONFIG_REQUEST_NBCH_PDU_REL13_TAG 0x205C

typedef struct {
	nfapi_dl_config_nbch_pdu_rel13_t nbch_pdu_rel13;
} nfapi_dl_config_nbch_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint16_t length;
	int16_t pdu_index;
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
} nfapi_dl_config_npdcch_pdu_rel13_t;

#define NFAPI_DL_CONFIG_REQUEST_NPDCCH_PDU_REL13_TAG 0x205D

typedef struct {
	nfapi_dl_config_npdcch_pdu_rel13_t npdcch_pdu_rel13;
} nfapi_dl_config_npdcch_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint16_t length;
	int16_t pdu_index;
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
} nfapi_dl_config_ndlsch_pdu_rel13_t;

#define NFAPI_DL_CONFIG_REQUEST_NDLSCH_PDU_REL13_TAG 0x205E

typedef struct {
	nfapi_dl_config_ndlsch_pdu_rel13_t ndlsch_pdu_rel13;
} nfapi_dl_config_ndlsch_pdu;


typedef struct {
	uint8_t pdu_type;
	uint8_t pdu_size;
	union {
		nfapi_dl_config_dci_dl_pdu	dci_dl_pdu;
		nfapi_dl_config_bch_pdu		bch_pdu;
		nfapi_dl_config_mch_pdu		mch_pdu;
		nfapi_dl_config_dlsch_pdu	dlsch_pdu;
		nfapi_dl_config_pch_pdu		pch_pdu;
		nfapi_dl_config_prs_pdu		prs_pdu;
		nfapi_dl_config_csi_rs_pdu	csi_rs_pdu;
		nfapi_dl_config_epdcch_pdu	epdcch_pdu;
		nfapi_dl_config_mpdcch_pdu	mpdcch_pdu;
		nfapi_dl_config_nbch_pdu	nbch_pdu;
		nfapi_dl_config_npdcch_pdu	npdcch_pdu;
		nfapi_dl_config_ndlsch_pdu	ndlsch_pdu;
	};
} nfapi_dl_config_request_pdu_t;

#define NFAPI_DL_CONFIG_MAX_PDU 100
typedef struct {
	nfapi_tl_t tl;
	uint8_t number_pdcch_ofdm_symbols;
	uint8_t number_dci;
	uint16_t number_pdu;
	uint8_t number_pdsch_rnti;
	uint16_t transmission_power_pcfich;
	nfapi_dl_config_request_pdu_t* dl_config_pdu_list;
} nfapi_dl_config_request_body_t;
#define NFAPI_DL_CONFIG_REQUEST_BODY_TAG 0x2000

typedef struct {
	nfapi_tl_t tl;
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
} nfapi_ul_config_ulsch_pdu_rel8_t;
#define NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL8_TAG 0x200D

typedef struct {
	nfapi_tl_t tl;
	uint8_t resource_allocation_type;
	uint32_t resource_block_coding;
	uint8_t transport_blocks;
	uint8_t transmission_scheme;
	uint8_t number_of_layers;
	uint8_t codebook_index;
	uint8_t disable_sequence_hopping_flag;
} nfapi_ul_config_ulsch_pdu_rel10_t;
#define NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL10_TAG 0x200E


typedef struct {
	nfapi_tl_t tl;
	uint8_t virtual_cell_id_enabled_flag;
	uint16_t npusch_identity;
	uint8_t dmrs_config_flag;
	uint16_t ndmrs_csh_identity;
} nfapi_ul_config_ulsch_pdu_rel11_t;

#define NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL11_TAG 0x2043

typedef struct {
	nfapi_tl_t tl;
	uint8_t  ue_type;
	uint16_t total_number_of_repetitions;
	uint16_t repetition_number;
	uint16_t initial_transmission_sf_io;
	uint8_t  empty_symbols_due_to_re_tunning;
} nfapi_ul_config_ulsch_pdu_rel13_t;
#define NFAPI_UL_CONFIG_REQUEST_ULSCH_PDU_REL13_TAG 0x2044

typedef struct {
	nfapi_ul_config_ulsch_pdu_rel8_t ulsch_pdu_rel8;
	nfapi_ul_config_ulsch_pdu_rel10_t ulsch_pdu_rel10;
	nfapi_ul_config_ulsch_pdu_rel11_t ulsch_pdu_rel11;
	nfapi_ul_config_ulsch_pdu_rel13_t ulsch_pdu_rel13;
} nfapi_ul_config_ulsch_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint8_t dl_cqi_pmi_size_rank_1;
	uint8_t dl_cqi_pmi_size_rank_greater_1;
	uint8_t ri_size;
	uint8_t delta_offset_cqi;
	uint8_t delta_offset_ri;
} nfapi_ul_config_cqi_ri_information_rel8_t;
#define NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL8_TAG 0x2010

typedef struct {
	uint8_t dl_cqi_pmi_ri_size;
	uint8_t control_type;
} nfapi_ul_config_periodic_cqi_pmi_ri_report_t;

typedef struct {
	uint8_t number_of_cc;
	struct {
		uint8_t ri_size;
		uint8_t dl_cqi_pmi_size[8];
	} cc[NFAPI_MAX_CC];
} nfapi_ul_config_aperiodic_cqi_pmi_ri_report_t;

typedef struct {
	nfapi_tl_t tl;
	uint8_t report_type;
	uint8_t delta_offset_cqi;
	uint8_t delta_offset_ri;
	union {
		nfapi_ul_config_periodic_cqi_pmi_ri_report_t periodic_cqi_pmi_ri_report;
		nfapi_ul_config_aperiodic_cqi_pmi_ri_report_t aperiodic_cqi_pmi_ri_report;
	};
} nfapi_ul_config_cqi_ri_information_rel9_t;
#define NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL9_TAG 0x2011

typedef struct {
	uint16_t dl_cqi_pmi_ri_size_2;
} nfapi_ul_config_periodic_cqi_pmi_ri_report_re13_t;

typedef struct {
} nfapi_ul_config_aperiodic_cqi_pmi_ri_report_re13_t;

typedef struct {
	nfapi_tl_t tl;
	uint8_t report_type; // Convience parameter, not sent on the wire
	union {
		nfapi_ul_config_periodic_cqi_pmi_ri_report_re13_t periodic_cqi_pmi_ri_report;
		nfapi_ul_config_aperiodic_cqi_pmi_ri_report_re13_t aperiodic_cqi_pmi_ri_report;
	};
} nfapi_ul_config_cqi_ri_information_rel13_t;
#define NFAPI_UL_CONFIG_REQUEST_CQI_RI_INFORMATION_REL13_TAG 0x2045

typedef struct {
	nfapi_ul_config_cqi_ri_information_rel8_t cqi_ri_information_rel8;
	nfapi_ul_config_cqi_ri_information_rel9_t cqi_ri_information_rel9;
	nfapi_ul_config_cqi_ri_information_rel13_t cqi_ri_information_rel13;
} nfapi_ul_config_cqi_ri_information;

typedef struct {
	nfapi_tl_t tl;
	uint8_t harq_size;
	uint8_t delta_offset_harq;
	uint8_t ack_nack_mode;
} nfapi_ul_config_ulsch_harq_information_rel10_t;
#define NFAPI_UL_CONFIG_REQUEST_ULSCH_HARQ_INFORMATION_REL10_TAG 0x2012

typedef struct {
	nfapi_tl_t tl;
	uint16_t harq_size_2;
	uint8_t delta_offset_harq_2;
} nfapi_ul_config_ulsch_harq_information_rel13_t;
#define NFAPI_UL_CONFIG_REQUEST_ULSCH_HARQ_INFORMATION_REL13_TAG 0x2046

typedef struct {
	nfapi_ul_config_ulsch_harq_information_rel10_t harq_information_rel10;
	nfapi_ul_config_ulsch_harq_information_rel13_t harq_information_rel13;
} nfapi_ul_config_ulsch_harq_information;

typedef struct {
	nfapi_tl_t tl;
	uint8_t n_srs_initial;
	uint8_t initial_number_of_resource_blocks;
} nfapi_ul_config_initial_transmission_parameters_rel8_t;
#define NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG 0x200F

typedef struct {
	nfapi_ul_config_initial_transmission_parameters_rel8_t initial_transmission_parameters_rel8;
} nfapi_ul_config_initial_transmission_parameters;

typedef struct {
	nfapi_ul_config_ulsch_pdu ulsch_pdu;
	nfapi_ul_config_cqi_ri_information cqi_ri_information;
	nfapi_ul_config_initial_transmission_parameters initial_transmission_parameters;
} nfapi_ul_config_ulsch_cqi_ri_pdu;

typedef struct {
	nfapi_ul_config_ulsch_pdu ulsch_pdu;
	nfapi_ul_config_ulsch_harq_information harq_information;
	nfapi_ul_config_initial_transmission_parameters initial_transmission_parameters;
} nfapi_ul_config_ulsch_harq_pdu;

typedef struct {
	nfapi_ul_config_ulsch_pdu ulsch_pdu;
	nfapi_ul_config_cqi_ri_information cqi_ri_information;
	nfapi_ul_config_ulsch_harq_information harq_information;
	nfapi_ul_config_initial_transmission_parameters initial_transmission_parameters;
} nfapi_ul_config_ulsch_cqi_harq_ri_pdu;


typedef struct {
	nfapi_tl_t tl;
	uint32_t handle;
	uint16_t rnti;
} nfapi_ul_config_ue_information_rel8_t;
#define NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG 0x2013

typedef struct {
	nfapi_tl_t tl;
	uint8_t virtual_cell_id_enabled_flag;
	uint16_t npusch_identity;
} nfapi_ul_config_ue_information_rel11_t;

#define NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL11_TAG 0x2047

typedef struct {
	nfapi_tl_t tl;
	uint8_t  ue_type;
	uint8_t  empty_symbols;
	uint16_t total_number_of_repetitions;
	uint16_t repetition_number;
} nfapi_ul_config_ue_information_rel13_t;

#define NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL13_TAG 0x2048

typedef struct {
	nfapi_ul_config_ue_information_rel8_t ue_information_rel8;
	nfapi_ul_config_ue_information_rel11_t ue_information_rel11;
	nfapi_ul_config_ue_information_rel13_t ue_information_rel13;
} nfapi_ul_config_ue_information;

typedef struct {
	nfapi_tl_t tl;
	uint16_t pucch_index;
	uint8_t dl_cqi_pmi_size;
} nfapi_ul_config_cqi_information_rel8_t;
#define NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL8_TAG 0x2014

typedef struct {
	nfapi_tl_t tl;
	uint8_t number_of_pucch_resource;
	uint16_t pucch_index_p1;
} nfapi_ul_config_cqi_information_rel10_t;
#define NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL10_TAG 0x2015

typedef struct {
	nfapi_tl_t tl;
	uint8_t csi_mode;
	uint16_t dl_cqi_pmi_size_2;
	uint8_t starting_prb;
	uint8_t n_prb;
	uint8_t cdm_index;
	uint8_t n_srs;
} nfapi_ul_config_cqi_information_rel13_t;
#define NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL13_TAG 0x2049

typedef struct {
	nfapi_ul_config_cqi_information_rel8_t cqi_information_rel8;
	nfapi_ul_config_cqi_information_rel10_t cqi_information_rel10;
	nfapi_ul_config_cqi_information_rel13_t cqi_information_rel13;
} nfapi_ul_config_cqi_information;

typedef struct {
	nfapi_tl_t tl;
	uint16_t pucch_index;
} nfapi_ul_config_sr_information_rel8_t;
#define NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL8_TAG 0x2016

typedef struct {
	nfapi_tl_t tl;
	uint8_t number_of_pucch_resources;
	uint16_t pucch_index_p1;
} nfapi_ul_config_sr_information_rel10_t;
#define NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL10_TAG 0x2017

typedef struct { 
	nfapi_ul_config_sr_information_rel8_t sr_information_rel8;
	nfapi_ul_config_sr_information_rel10_t sr_information_rel10;
} nfapi_ul_config_sr_information;

typedef struct { 
	nfapi_tl_t tl;
	uint8_t harq_size;
	uint8_t ack_nack_mode;
	uint8_t number_of_pucch_resources;
	uint16_t n_pucch_1_0;
	uint16_t n_pucch_1_1;
	uint16_t n_pucch_1_2;
	uint16_t n_pucch_1_3;
} nfapi_ul_config_harq_information_rel10_tdd_t;
#define NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL10_TDD_TAG 0x2018


typedef struct { 
	nfapi_tl_t tl;
	uint16_t n_pucch_1_0;
	uint8_t harq_size;
} nfapi_ul_config_harq_information_rel8_fdd_t;
#define NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL8_FDD_TAG 0x2019

typedef struct { 
	nfapi_tl_t tl;
	uint8_t harq_size;
	uint8_t ack_nack_mode;
	uint8_t number_of_pucch_resources;
	uint16_t n_pucch_1_0;
	uint16_t n_pucch_1_1;
	uint16_t n_pucch_1_2;
	uint16_t n_pucch_1_3;
} nfapi_ul_config_harq_information_rel9_fdd_t;
#define NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL9_FDD_TAG 0x201a

typedef struct { 
	nfapi_tl_t tl;
	uint8_t  num_ant_ports;
	uint16_t n_pucch_2_0;
	uint16_t n_pucch_2_1;
	uint16_t n_pucch_2_2;
	uint16_t n_pucch_2_3;	
} nfapi_ul_config_harq_information_rel11_t;
#define NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL11_TAG 0x204A

typedef struct { 
	nfapi_tl_t tl;
	uint16_t  harq_size_2;
	uint8_t starting_prb;
	uint8_t n_prb;
	uint8_t cdm_index;
	uint8_t n_srs;
} nfapi_ul_config_harq_information_rel13_t;
#define NFAPI_UL_CONFIG_REQUEST_HARQ_INFORMATION_REL13_TAG 0x204B

typedef struct {
	nfapi_ul_config_harq_information_rel10_tdd_t harq_information_rel10_tdd;
	nfapi_ul_config_harq_information_rel8_fdd_t harq_information_rel8_fdd;
	nfapi_ul_config_harq_information_rel9_fdd_t harq_information_rel9_fdd;
	nfapi_ul_config_harq_information_rel11_t harq_information_rel11;
	nfapi_ul_config_harq_information_rel13_t harq_information_rel13;
} nfapi_ul_config_harq_information;

typedef struct { 
	nfapi_tl_t tl;
	uint32_t handle;
	uint16_t size;
	uint16_t rnti;
	uint8_t srs_bandwidth;
	uint8_t frequency_domain_position;
	uint8_t srs_hopping_bandwidth;
	uint8_t transmission_comb;
	uint16_t i_srs;
	uint8_t sounding_reference_cyclic_shift;
} nfapi_ul_config_srs_pdu_rel8_t;
#define NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL8_TAG 0x201b

typedef struct { 
	nfapi_tl_t tl;
	uint8_t antenna_port;
} nfapi_ul_config_srs_pdu_rel10_t;
#define NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL10_TAG 0x201c

typedef struct { 
	nfapi_tl_t tl;
	uint8_t number_of_combs;
} nfapi_ul_config_srs_pdu_rel13_t;
#define NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL13_TAG 0x204c

typedef struct {
	nfapi_ul_config_srs_pdu_rel8_t srs_pdu_rel8;
	nfapi_ul_config_srs_pdu_rel10_t srs_pdu_rel10;
	nfapi_ul_config_srs_pdu_rel13_t srs_pdu_rel13;
} nfapi_ul_config_srs_pdu;

typedef struct {
	nfapi_ul_config_ue_information ue_information;
	nfapi_ul_config_cqi_information cqi_information;
} nfapi_ul_config_uci_cqi_pdu;

typedef struct {
	nfapi_ul_config_ue_information ue_information;
	nfapi_ul_config_sr_information sr_information;
} nfapi_ul_config_uci_sr_pdu;

typedef struct {
	nfapi_ul_config_ue_information ue_information;
	nfapi_ul_config_harq_information harq_information;
} nfapi_ul_config_uci_harq_pdu;

typedef struct {
	nfapi_ul_config_ue_information ue_information;
	nfapi_ul_config_sr_information sr_information;
	nfapi_ul_config_harq_information harq_information;
} nfapi_ul_config_uci_sr_harq_pdu;

typedef struct {
	nfapi_ul_config_ue_information ue_information;
	nfapi_ul_config_cqi_information cqi_information;
	nfapi_ul_config_harq_information harq_information;
} nfapi_ul_config_uci_cqi_harq_pdu;

typedef struct {
	nfapi_ul_config_ue_information ue_information;
	nfapi_ul_config_cqi_information cqi_information;
	nfapi_ul_config_sr_information sr_information;
} nfapi_ul_config_uci_cqi_sr_pdu;

typedef struct {
	nfapi_ul_config_ue_information ue_information;
	nfapi_ul_config_cqi_information cqi_information;
	nfapi_ul_config_sr_information sr_information;
	nfapi_ul_config_harq_information harq_information;
} nfapi_ul_config_uci_cqi_sr_harq_pdu;

typedef struct {
	nfapi_ul_config_ue_information ue_information;
} nfapi_ul_config_harq_buffer_pdu;

typedef struct {
	nfapi_ul_config_ulsch_pdu ulsch_pdu;
	nfapi_ul_config_cqi_information csi_information;
} nfapi_ul_config_ulsch_uci_csi_pdu;

typedef struct {
	nfapi_ul_config_ulsch_pdu ulsch_pdu;
	nfapi_ul_config_harq_information harq_information;
} nfapi_ul_config_ulsch_uci_harq_pdu;

typedef struct {
	nfapi_ul_config_ulsch_pdu ulsch_pdu;
	nfapi_ul_config_cqi_information csi_information;
	nfapi_ul_config_harq_information harq_information;
} nfapi_ul_config_ulsch_csi_uci_harq_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint8_t harq_ack_resource;
} nfapi_ul_config_nb_harq_information_rel13_fdd_t;
#define NFAPI_UL_CONFIG_REQUEST_NB_HARQ_INFORMATION_REL13_FDD_TAG 0x2061

typedef struct {
	nfapi_ul_config_nb_harq_information_rel13_fdd_t nb_harq_information_rel13_fdd;
} nfapi_ul_config_nb_harq_information;

typedef struct {
	nfapi_tl_t tl;	
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
	nfapi_ul_config_ue_information ue_information;
	nfapi_ul_config_nb_harq_information nb_harq_information;
} nfapi_ul_config_nulsch_pdu_rel13_t;
#define NFAPI_UL_CONFIG_REQUEST_NULSCH_PDU_REL13_TAG 0x205F

typedef struct {
	nfapi_ul_config_nulsch_pdu_rel13_t nulsch_pdu_rel13;
} nfapi_ul_config_nulsch_pdu;


typedef struct {
	nfapi_tl_t tl;
	uint8_t nprach_config_0;
	uint8_t nprach_config_1;
	uint8_t nprach_config_2;
} nfapi_ul_config_nrach_pdu_rel13_t;
#define NFAPI_UL_CONFIG_REQUEST_NRACH_PDU_REL13_TAG 0x2067

typedef struct {
	nfapi_ul_config_nrach_pdu_rel13_t nrach_pdu_rel13;
} nfapi_ul_config_nrach_pdu;

typedef struct {
	uint8_t pdu_type;
	uint8_t pdu_size;
	union {
		nfapi_ul_config_ulsch_pdu				ulsch_pdu;
		nfapi_ul_config_ulsch_cqi_ri_pdu		ulsch_cqi_ri_pdu;
		nfapi_ul_config_ulsch_harq_pdu			ulsch_harq_pdu;
		nfapi_ul_config_ulsch_cqi_harq_ri_pdu	ulsch_cqi_harq_ri_pdu;
		nfapi_ul_config_uci_cqi_pdu				uci_cqi_pdu;
		nfapi_ul_config_uci_sr_pdu				uci_sr_pdu;
		nfapi_ul_config_uci_harq_pdu			uci_harq_pdu;
		nfapi_ul_config_uci_sr_harq_pdu			uci_sr_harq_pdu;
		nfapi_ul_config_uci_cqi_harq_pdu		uci_cqi_harq_pdu;
		nfapi_ul_config_uci_cqi_sr_pdu			uci_cqi_sr_pdu;
		nfapi_ul_config_uci_cqi_sr_harq_pdu		uci_cqi_sr_harq_pdu;
		nfapi_ul_config_srs_pdu					srs_pdu;
		nfapi_ul_config_harq_buffer_pdu			harq_buffer_pdu;
		nfapi_ul_config_ulsch_uci_csi_pdu		ulsch_uci_csi_pdu;
		nfapi_ul_config_ulsch_uci_harq_pdu		ulsch_uci_harq_pdu;
		nfapi_ul_config_ulsch_csi_uci_harq_pdu	ulsch_csi_uci_harq_pdu;
		nfapi_ul_config_nulsch_pdu				nulsch_pdu;
		nfapi_ul_config_nrach_pdu				nrach_pdu;
	};
} nfapi_ul_config_request_pdu_t;

#define NFAPI_UL_CONFIG_MAX_PDU 100
typedef struct {
	nfapi_tl_t tl;
	uint8_t number_of_pdus;
	uint8_t rach_prach_frequency_resources;
	uint8_t srs_present;
	nfapi_ul_config_request_pdu_t* ul_config_pdu_list;
} nfapi_ul_config_request_body_t;
#define NFAPI_UL_CONFIG_REQUEST_BODY_TAG 0x200C

typedef struct {
	nfapi_tl_t tl;
	uint8_t resource_block_start;
	uint8_t cyclic_shift_2_for_drms;
	uint8_t hi_value;
	uint8_t i_phich;
	uint16_t transmission_power;
} nfapi_hi_dci0_hi_pdu_rel8_t;
#define NFAPI_HI_DCI0_REQUEST_HI_PDU_REL8_TAG 0x201e

typedef struct {
	nfapi_tl_t tl;
	uint8_t flag_tb2;
	uint8_t hi_value_2;
} nfapi_hi_dci0_hi_pdu_rel10_t;
#define NFAPI_HI_DCI0_REQUEST_HI_PDU_REL10_TAG 0x201f

typedef struct {
	nfapi_hi_dci0_hi_pdu_rel8_t		hi_pdu_rel8;
	nfapi_hi_dci0_hi_pdu_rel10_t	hi_pdu_rel10;
} nfapi_hi_dci0_hi_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint8_t dci_format;
	uint8_t cce_index;
	uint8_t aggregation_level;
	uint16_t rnti;
	uint8_t resource_block_start;
	uint8_t number_of_resource_block;
	uint8_t mcs_1;
	uint8_t cyclic_shift_2_for_drms;
	uint8_t frequency_hopping_enabled_flag;
	uint8_t frequency_hopping_bits;
	uint8_t new_data_indication_1;
	uint8_t ue_tx_antenna_seleciton;
	uint8_t tpc;
	uint8_t cqi_csi_request;
	uint8_t ul_index;
	uint8_t dl_assignment_index;
	uint32_t tpc_bitmap;
	uint16_t transmission_power;
	uint8_t harq_pid;
} nfapi_hi_dci0_dci_pdu_rel8_t;
#define NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG 0x2020

typedef struct {
	nfapi_tl_t tl;
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
} nfapi_hi_dci0_dci_pdu_rel10_t;
#define NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL10_TAG 0x2021

typedef struct {
	nfapi_tl_t tl;
	uint8_t pscch_resource;
	uint8_t time_resource_pattern;
} nfapi_hi_dci0_dci_pdu_rel12_t;

#define NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL12_TAG 0x204D

typedef struct {
	nfapi_hi_dci0_dci_pdu_rel8_t	dci_pdu_rel8;
	nfapi_hi_dci0_dci_pdu_rel10_t	dci_pdu_rel10;
	nfapi_hi_dci0_dci_pdu_rel12_t	dci_pdu_rel12;
} nfapi_hi_dci0_dci_pdu;

typedef nfapi_hi_dci0_dci_pdu_rel8_t nfapi_hi_dci0_epdcch_dci_pdu_rel8_t;
#define NFAPI_HI_DCI0_REQUEST_EPDCCH_DCI_PDU_REL8_TAG 0x2020

typedef nfapi_hi_dci0_dci_pdu_rel10_t nfapi_hi_dci0_epdcch_dci_pdu_rel10_t;
#define NFAPI_HI_DCI0_REQUEST_EPDCCH_DCI_PDU_REL10_TAG 0x2021

typedef nfapi_dl_config_epdcch_parameters_rel11_t nfapi_hi_dci0_epdcch_parameters_rel11_t;
#define NFAPI_HI_DCI0_REQUEST_EPDCCH_PARAMETERS_REL11_TAG 0x2041

typedef struct {
	nfapi_hi_dci0_epdcch_dci_pdu_rel8_t		epdcch_dci_pdu_rel8;
	nfapi_hi_dci0_epdcch_dci_pdu_rel10_t	epdcch_dci_pdu_rel10;
	nfapi_hi_dci0_epdcch_parameters_rel11_t	epdcch_parameters_rel11;
} nfapi_hi_dci0_epdcch_dci_pdu;


typedef struct {
	nfapi_tl_t tl;
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
	uint16_t precoding_value[NFAPI_MAX_ANTENNA_PORT_COUNT];
} nfapi_hi_dci0_mpdcch_dci_pdu_rel13_t;
#define NFAPI_HI_DCI0_REQUEST_MPDCCH_DCI_PDU_REL13_TAG 0x204E

typedef struct {
	nfapi_hi_dci0_mpdcch_dci_pdu_rel13_t	mpdcch_dci_pdu_rel13;
} nfapi_hi_dci0_mpdcch_dci_pdu;

typedef struct {
	nfapi_tl_t tl;
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
} nfapi_hi_dci0_npdcch_dci_pdu_rel13_t;

#define NFAPI_HI_DCI0_REQUEST_NPDCCH_DCI_PDU_REL13_TAG 0x2062

typedef struct {
	nfapi_hi_dci0_npdcch_dci_pdu_rel13_t	npdcch_dci_pdu_rel13;
} nfapi_hi_dci0_npdcch_dci_pdu;

typedef struct {
	uint8_t pdu_type;
	uint8_t pdu_size;
	union {
		nfapi_hi_dci0_hi_pdu			hi_pdu;
		nfapi_hi_dci0_dci_pdu			dci_pdu;
		nfapi_hi_dci0_epdcch_dci_pdu	epdcch_dci_pdu;
		nfapi_hi_dci0_mpdcch_dci_pdu	mpdcch_dci_pdu;
		nfapi_hi_dci0_npdcch_dci_pdu	npdcch_dci_pdu;
	};
} nfapi_hi_dci0_request_pdu_t;

#define NFAPI_HI_DCI0_MAX_PDU 100
typedef struct {
	nfapi_tl_t tl;
	uint16_t sfnsf;
	uint8_t number_of_dci;
	uint8_t number_of_hi;
	nfapi_hi_dci0_request_pdu_t* hi_dci0_pdu_list;
} nfapi_hi_dci0_request_body_t;
#define NFAPI_HI_DCI0_REQUEST_BODY_TAG 0x201D

#define NFAPI_TX_MAX_SEGMENTS 32
typedef struct {
	uint16_t pdu_length;
	int16_t pdu_index;
	uint8_t num_segments;
	struct {
		uint32_t segment_length;
		uint8_t* segment_data;
	} segments[NFAPI_TX_MAX_SEGMENTS];
} nfapi_tx_request_pdu_t;

#define NFAPI_TX_MAX_PDU 100
typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_pdus;
	nfapi_tx_request_pdu_t* tx_pdu_list;
} nfapi_tx_request_body_t;
#define NFAPI_TX_REQUEST_BODY_TAG 0x2022

#define  NFAPI_RELEASE_MAX_RNTI  256
typedef struct {
    uint32_t handle;
    uint16_t rnti;
} nfapi_ue_release_request_TLVs_t;

typedef struct {
    nfapi_tl_t tl;
    uint16_t number_of_TLVs;
    nfapi_ue_release_request_TLVs_t ue_release_request_TLVs_list[NFAPI_RELEASE_MAX_RNTI];
} nfapi_ue_release_request_body_t;
#define NFAPI_UE_RELEASE_BODY_TAG 0x2068

// P7 Message Structures
typedef struct {
	nfapi_p7_message_header_t header;
	uint32_t t1;
	int32_t delta_sfn_sf;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_dl_node_sync_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint32_t t1;
	uint32_t t2;
	uint32_t t3;	
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_ul_node_sync_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint32_t last_sfn_sf;

	uint32_t time_since_last_timing_info;
	uint32_t dl_config_jitter;
	uint32_t tx_request_jitter;
	uint32_t ul_config_jitter;
	uint32_t hi_dci0_jitter;

	int32_t dl_config_latest_delay;
	int32_t tx_request_latest_delay;
	int32_t ul_config_latest_delay;
	int32_t hi_dci0_latest_delay;

	int32_t dl_config_earliest_arrival;
	int32_t tx_request_earliest_arrival;
	int32_t ul_config_earliest_arrival;
	int32_t hi_dci0_earliest_arrival;
	
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_timing_info_t;

typedef struct {
	nfapi_p7_message_header_t header;
	
	uint32_t last_sfn;
	uint32_t last_slot;
	uint32_t time_since_last_timing_info;
	
	uint32_t dl_tti_jitter;
	uint32_t tx_data_request_jitter;
	uint32_t ul_tti_jitter;
	uint32_t ul_dci_jitter;

	int32_t dl_tti_latest_delay;
	int32_t tx_data_request_latest_delay;
	int32_t ul_tti_latest_delay;
	int32_t ul_dci_latest_delay;
	
	int32_t dl_tti_earliest_arrival;
	int32_t tx_data_request_earliest_arrival;
	int32_t ul_tti_earliest_arrival;
	int32_t ul_dci_earliest_arrival;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_timing_info_t;

typedef struct {
	nfapi_tl_t tl;
	uint32_t handle;
	uint16_t rnti;
} nfapi_rx_ue_information;
#define NFAPI_RX_UE_INFORMATION_TAG 0x2038

typedef struct { 
	uint8_t value_0;
	uint8_t value_1;
} nfapi_harq_indication_tdd_harq_data_bundling_t;

typedef struct { 
	uint8_t value_0;
	uint8_t value_1;
	uint8_t value_2;
	uint8_t value_3;
} nfapi_harq_indication_tdd_harq_data_multiplexing_t;

typedef struct { 
	uint8_t value_0;
} nfapi_harq_indication_tdd_harq_data_special_bundling_t;

typedef struct { 
	uint8_t value_0;
} nfapi_harq_indication_tdd_harq_data_t;

typedef struct { 
	nfapi_tl_t tl;
	uint8_t mode;
	uint8_t number_of_ack_nack;
	union{
		nfapi_harq_indication_tdd_harq_data_bundling_t			bundling;
		nfapi_harq_indication_tdd_harq_data_multiplexing_t		multiplex;
		nfapi_harq_indication_tdd_harq_data_special_bundling_t	special_bundling;
	} harq_data;
} nfapi_harq_indication_tdd_rel8_t;
#define NFAPI_HARQ_INDICATION_TDD_REL8_TAG 0x2027

typedef struct {
	nfapi_tl_t tl;
	uint8_t mode;
	uint8_t number_of_ack_nack;
	union{
		nfapi_harq_indication_tdd_harq_data_t	bundling;
		nfapi_harq_indication_tdd_harq_data_t	multiplex;
		nfapi_harq_indication_tdd_harq_data_special_bundling_t	special_bundling;
		nfapi_harq_indication_tdd_harq_data_t	channel_selection;
		nfapi_harq_indication_tdd_harq_data_t	format_3;
	} harq_data[NFAPI_MAX_NUMBER_ACK_NACK_TDD];
} nfapi_harq_indication_tdd_rel9_t;
#define NFAPI_HARQ_INDICATION_TDD_REL9_TAG 0x2028

typedef struct {
	nfapi_tl_t tl;
	uint8_t mode;
	uint16_t number_of_ack_nack;
	union{
		nfapi_harq_indication_tdd_harq_data_t					bundling;
		nfapi_harq_indication_tdd_harq_data_t					multiplex;
		nfapi_harq_indication_tdd_harq_data_special_bundling_t	special_bundling;
		nfapi_harq_indication_tdd_harq_data_t					channel_selection;
		nfapi_harq_indication_tdd_harq_data_t			format_3;
		nfapi_harq_indication_tdd_harq_data_t			format_4;
		nfapi_harq_indication_tdd_harq_data_t			format_5;
	} harq_data[NFAPI_MAX_NUMBER_ACK_NACK_TDD];
} nfapi_harq_indication_tdd_rel13_t;
#define NFAPI_HARQ_INDICATION_TDD_REL13_TAG 0x204F

typedef struct { 
	nfapi_tl_t tl;
	uint8_t harq_tb1;
	uint8_t harq_tb2;
} nfapi_harq_indication_fdd_rel8_t;
#define NFAPI_HARQ_INDICATION_FDD_REL8_TAG 0x2029

#define NFAPI_HARQ_ACK_NACK_REL9_MAX 10
typedef struct {
	nfapi_tl_t tl;
	uint8_t mode;
	uint8_t number_of_ack_nack;
	uint8_t harq_tb_n[NFAPI_HARQ_ACK_NACK_REL9_MAX];
} nfapi_harq_indication_fdd_rel9_t;
#define NFAPI_HARQ_INDICATION_FDD_REL9_TAG 0x202a

#define NFAPI_HARQ_ACK_NACK_REL13_MAX 22 // Need to check this max?
typedef struct {
	nfapi_tl_t tl;
	uint8_t mode;
	uint16_t number_of_ack_nack;
	uint8_t harq_tb_n[NFAPI_HARQ_ACK_NACK_REL13_MAX];
} nfapi_harq_indication_fdd_rel13_t;
#define NFAPI_HARQ_INDICATION_FDD_REL13_TAG 0x2050

typedef struct {
	nfapi_tl_t tl;
	uint8_t ul_cqi;
	uint8_t channel;
} nfapi_ul_cqi_information_t;
#define NFAPI_UL_CQI_INFORMATION_TAG 0x2052

// Only expect 1 harq_indication TLV.tag to be set
// Would this be a better a an union, but not clear which combinations
// are valid
typedef struct {
	uint16_t							instance_length;
	nfapi_rx_ue_information				rx_ue_information;
	nfapi_harq_indication_tdd_rel8_t	harq_indication_tdd_rel8;
	nfapi_harq_indication_tdd_rel9_t	harq_indication_tdd_rel9;
	nfapi_harq_indication_tdd_rel13_t	harq_indication_tdd_rel13;
	nfapi_harq_indication_fdd_rel8_t	harq_indication_fdd_rel8;
	nfapi_harq_indication_fdd_rel9_t	harq_indication_fdd_rel9;
	nfapi_harq_indication_fdd_rel13_t	harq_indication_fdd_rel13;
	nfapi_ul_cqi_information_t			ul_cqi_information;
} nfapi_harq_indication_pdu_t;

#define NFAPI_HARQ_IND_MAX_PDU 100
typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_harqs;
	nfapi_harq_indication_pdu_t* harq_pdu_list;
} nfapi_harq_indication_body_t;
#define NFAPI_HARQ_INDICATION_BODY_TAG 0x2026

typedef struct {
	nfapi_tl_t tl;
	uint8_t crc_flag;
} nfapi_crc_indication_rel8_t;
#define NFAPI_CRC_INDICATION_REL8_TAG 0x202c

typedef struct {
	uint16_t					instance_length;
	nfapi_rx_ue_information		rx_ue_information;
	nfapi_crc_indication_rel8_t	crc_indication_rel8;
} nfapi_crc_indication_pdu_t;

#define NFAPI_CRC_IND_MAX_PDU 100
typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_crcs;
	nfapi_crc_indication_pdu_t* crc_pdu_list;
} nfapi_crc_indication_body_t;
#define NFAPI_CRC_INDICATION_BODY_TAG 0x202b

typedef struct {
	uint16_t					instance_length;
	nfapi_rx_ue_information		rx_ue_information;
	nfapi_ul_cqi_information_t	ul_cqi_information;
} nfapi_sr_indication_pdu_t;

#define NFAPI_SR_IND_MAX_PDU 100
typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_srs;				// Question : should this be srs
	nfapi_sr_indication_pdu_t* sr_pdu_list;
} nfapi_sr_indication_body_t;
#define NFAPI_SR_INDICATION_BODY_TAG 0x202d

// The data offset should be set to 0 or 1 before encoding
// If it is set to 1 the nfapi library will detemine the correct offset

typedef struct {
	nfapi_tl_t tl;
	uint16_t length;
	uint16_t data_offset;
	uint8_t ul_cqi;
	uint8_t ri;
	uint16_t timing_advance;
} nfapi_cqi_indication_rel8_t;
#define NFAPI_CQI_INDICATION_REL8_TAG 0x202f

#define NFAPI_CC_MAX 4
typedef struct {
	nfapi_tl_t tl;
	uint16_t length;
	uint16_t data_offset;
	uint8_t ul_cqi;
	uint8_t number_of_cc_reported;
	uint8_t ri[NFAPI_CC_MAX];
	uint16_t timing_advance;
	uint16_t timing_advance_r9;
} nfapi_cqi_indication_rel9_t;
#define NFAPI_CQI_INDICATION_REL9_TAG 0x2030

typedef struct {
	uint16_t					instance_length;
	nfapi_rx_ue_information		rx_ue_information;
	nfapi_cqi_indication_rel8_t cqi_indication_rel8;
	nfapi_cqi_indication_rel9_t cqi_indication_rel9;
	nfapi_ul_cqi_information_t	ul_cqi_information;
} nfapi_cqi_indication_pdu_t;

#define NFAPI_CQI_RAW_MAX_LEN 12
typedef struct {
	uint8_t pdu[NFAPI_CQI_RAW_MAX_LEN];
} nfapi_cqi_indication_raw_pdu_t;

#define NFAPI_CQI_IND_MAX_PDU 100
typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_cqis;
	nfapi_cqi_indication_pdu_t*			cqi_pdu_list;
	nfapi_cqi_indication_raw_pdu_t*		cqi_raw_pdu_list;
} nfapi_cqi_indication_body_t;
#define NFAPI_CQI_INDICATION_BODY_TAG 0x202e

typedef struct { 
	nfapi_tl_t tl;
	uint16_t rnti;
	uint8_t preamble;
	uint16_t timing_advance;
} nfapi_preamble_pdu_rel8_t;
#define NFAPI_PREAMBLE_REL8_TAG 0x2032

typedef struct {
	nfapi_tl_t tl;
	uint16_t timing_advance_r9;
} nfapi_preamble_pdu_rel9_t;
#define NFAPI_PREAMBLE_REL9_TAG 0x2033

typedef struct {
	nfapi_tl_t tl;
	uint8_t rach_resource_type;
} nfapi_preamble_pdu_rel13_t;
#define NFAPI_PREAMBLE_REL13_TAG 0x2051

typedef struct { 
	uint16_t					instance_length;
	nfapi_preamble_pdu_rel8_t	preamble_rel8;
	nfapi_preamble_pdu_rel9_t	preamble_rel9;
	nfapi_preamble_pdu_rel13_t	preamble_rel13;
} nfapi_preamble_pdu_t;

#define NFAPI_PREAMBLE_MAX_PDU 100
typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_preambles;
	nfapi_preamble_pdu_t*			preamble_list;
} nfapi_rach_indication_body_t;
#define NFAPI_RACH_INDICATION_BODY_TAG 0x2031

#define NFAPI_NUM_RB_MAX 1000
typedef struct {
	nfapi_tl_t tl;
	uint16_t doppler_estimation;
	uint16_t timing_advance;
	uint8_t number_of_resource_blocks;
	uint8_t rb_start;
	uint8_t snr[NFAPI_NUM_RB_MAX];
} nfapi_srs_indication_fdd_rel8_t;
#define NFAPI_SRS_INDICATION_FDD_REL8_TAG 0x2035

typedef struct { 
	nfapi_tl_t tl;
	uint16_t timing_advance_r9;
} nfapi_srs_indication_fdd_rel9_t;
#define NFAPI_SRS_INDICATION_FDD_REL9_TAG 0x2036

typedef struct { 
	nfapi_tl_t tl;
	uint8_t uppts_symbol;
} nfapi_srs_indication_ttd_rel10_t;
#define NFAPI_SRS_INDICATION_TDD_REL10_TAG 0x2037

typedef struct { 
	nfapi_tl_t tl;
	uint16_t ul_rtoa;
} nfapi_srs_indication_fdd_rel11_t;
#define NFAPI_SRS_INDICATION_FDD_REL11_TAG 0x2053


typedef struct {
	nfapi_tl_t tl;
	uint8_t num_prb_per_subband;
	uint8_t number_of_subbands;
	uint8_t num_atennas;
	struct {
		uint8_t subband_index;
		uint16_t channel[NFAPI_MAX_NUM_PHYSICAL_ANTENNAS];
	} subands[NFAPI_MAX_NUM_SUBBANDS];
} nfapi_tdd_channel_measurement_t;
#define NFAPI_TDD_CHANNEL_MEASUREMENT_TAG 0x2054

typedef struct {
	uint16_t							instance_length;
	nfapi_rx_ue_information				rx_ue_information;
	nfapi_srs_indication_fdd_rel8_t		srs_indication_fdd_rel8;
	nfapi_srs_indication_fdd_rel9_t		srs_indication_fdd_rel9;
	nfapi_srs_indication_ttd_rel10_t	srs_indication_tdd_rel10;
	nfapi_srs_indication_fdd_rel11_t	srs_indication_fdd_rel11;
	nfapi_tdd_channel_measurement_t		tdd_channel_measurement;
} nfapi_srs_indication_pdu_t;

#define NFAPI_SRS_IND_MAX_PDU 16
typedef struct {
	nfapi_tl_t tl;
	uint8_t number_of_ues;
	nfapi_srs_indication_pdu_t* srs_pdu_list;
} nfapi_srs_indication_body_t;
#define NFAPI_SRS_INDICATION_BODY_TAG 0x2034

typedef struct {
	nfapi_tl_t tl;
	uint16_t length;
	uint16_t offset;
	uint8_t ul_cqi;
	uint16_t timing_advance;
} nfapi_rx_indication_rel8_t;
#define NFAPI_RX_INDICATION_REL8_TAG 0x2024

typedef struct {
	nfapi_tl_t tl;
	uint16_t timing_advance_r9;
 } nfapi_rx_indication_rel9_t;
#define NFAPI_RX_INDICATION_REL9_TAG 0x2025

#define NFAPI_RX_IND_DATA_MAX 8192
typedef struct {
	nfapi_rx_ue_information rx_ue_information;
	nfapi_rx_indication_rel8_t rx_indication_rel8;
	nfapi_rx_indication_rel9_t rx_indication_rel9;
	uint8_t rx_ind_data[NFAPI_RX_IND_DATA_MAX];
} nfapi_rx_indication_pdu_t;

#define NFAPI_RX_IND_MAX_PDU 100
typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_pdus;
	nfapi_rx_indication_pdu_t* rx_pdu_list;
} nfapi_rx_indication_body_t;
#define NFAPI_RX_INDICATION_BODY_TAG 0x2023

typedef struct {
	nfapi_tl_t tl;	
	uint8_t harq_tb1;
} nfapi_nb_harq_indication_fdd_rel13_t;
#define NFAPI_NB_HARQ_INDICATION_FDD_REL13_TAG 0x2064

typedef struct {
	uint16_t								instance_length;
	nfapi_rx_ue_information					rx_ue_information;
	nfapi_nb_harq_indication_fdd_rel13_t	nb_harq_indication_fdd_rel13;
	nfapi_ul_cqi_information_t				ul_cqi_information;
} nfapi_nb_harq_indication_pdu_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_harqs;
	nfapi_nb_harq_indication_pdu_t* nb_harq_pdu_list;
} nfapi_nb_harq_indication_body_t;
#define NFAPI_NB_HARQ_INDICATION_BODY_TAG 0x2063

typedef struct {
	nfapi_tl_t tl;
	uint16_t rnti;
	uint8_t initial_sc;
	uint16_t timing_advance;
	uint8_t nrach_ce_level;
} nfapi_nrach_indication_pdu_rel13_t;
#define NFAPI_NRACH_INDICATION_REL13_TAG 0x2066

typedef struct {
	nfapi_nrach_indication_pdu_rel13_t		nrach_indication_rel13;
} nfapi_nrach_indication_pdu_t;

typedef struct {
	nfapi_tl_t tl;
	uint8_t number_of_initial_scs_detected;
	nfapi_nrach_indication_pdu_t* nrach_pdu_list;
} nfapi_nrach_indication_body_t;
#define NFAPI_NRACH_INDICATION_BODY_TAG 0x2065

typedef struct {
	nfapi_tl_t tl;
	uint32_t handle;
	uint32_t mp_cca;
	uint32_t n_cca;
	uint32_t offset;
	uint32_t lte_txop_sf;
	uint16_t txop_sfn_sf_end;
	uint32_t lbt_mode;
} nfapi_lbt_pdsch_req_pdu_rel13_t;
#define NFAPI_LBT_PDSCH_REQ_PDU_REL13_TAG 0x2056

typedef struct {
	nfapi_lbt_pdsch_req_pdu_rel13_t lbt_pdsch_req_pdu_rel13;
} nfapi_lbt_pdsch_req_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint32_t handle;
	uint32_t offset;
	uint16_t sfn_sf_end;
	uint32_t lbt_mode;
} nfapi_lbt_drs_req_pdu_rel13_t;
#define NFAPI_LBT_DRS_REQ_PDU_REL13_TAG 0x2057

typedef struct {
	nfapi_lbt_drs_req_pdu_rel13_t lbt_drs_req_pdu_rel13;
} nfapi_lbt_drs_req_pdu;

typedef struct {
	uint8_t pdu_type;
	uint8_t pdu_size;
	union {
		nfapi_lbt_pdsch_req_pdu		lbt_pdsch_req_pdu;
		nfapi_lbt_drs_req_pdu		lbt_drs_req_pdu;
	};
} nfapi_lbt_dl_config_request_pdu_t;

#define NFAPI_LBT_DL_CONFIG_REQ_MAX_PDU 16
typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_pdus;
	nfapi_lbt_dl_config_request_pdu_t*		lbt_dl_config_req_pdu_list;
} nfapi_lbt_dl_config_request_body_t;
#define NFAPI_LBT_DL_CONFIG_REQUEST_BODY_TAG 0x2055


typedef struct {
	nfapi_tl_t tl;
	uint32_t handle;
	uint32_t result;
	uint32_t lte_txop_symbols;
	uint32_t initial_partial_sf;
} nfapi_lbt_pdsch_rsp_pdu_rel13_t;
#define NFAPI_LBT_PDSCH_RSP_PDU_REL13_TAG 0x2059

typedef struct {
	nfapi_lbt_pdsch_rsp_pdu_rel13_t lbt_pdsch_rsp_pdu_rel13;
} nfapi_lbt_pdsch_rsp_pdu;

typedef struct {
	nfapi_tl_t tl;
	uint32_t handle;
	uint32_t result;
} nfapi_lbt_drs_rsp_pdu_rel13_t;
#define NFAPI_LBT_DRS_RSP_PDU_REL13_TAG 0x205A

typedef struct {
	nfapi_lbt_drs_rsp_pdu_rel13_t lbt_drs_rsp_pdu_rel13;
} nfapi_lbt_drs_rsp_pdu;


typedef struct {
	uint8_t pdu_type;
	uint8_t pdu_size;
	union {
		nfapi_lbt_pdsch_rsp_pdu		lbt_pdsch_rsp_pdu;
		nfapi_lbt_drs_rsp_pdu		lbt_drs_rsp_pdu;
	};
} nfapi_lbt_dl_indication_pdu_t;

#define NFAPI_LBT_IND_MAX_PDU 16
typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_pdus;
	nfapi_lbt_dl_indication_pdu_t* lbt_indication_pdu_list;
} nfapi_lbt_dl_indication_body_t;
#define NFAPI_LBT_DL_INDICATION_BODY_TAG 0x2058

typedef struct {
} nfapi_error_indication_msg_invalid_state;

typedef struct {
} nfapi_error_indication_msg_bch_missing;

typedef struct {
	uint16_t recieved_sfn_sf;
	uint16_t expected_sfn_sf;
} nfapi_error_indication_sfn_out_of_sync;

typedef struct {
	uint8_t sub_error_code;
	uint8_t direction;
	uint16_t rnti;
	uint8_t pdu_type;
} nfapi_error_indication_msg_pdu_err;

typedef struct {
	uint16_t recieved_sfn_sf;
	uint16_t expected_sfn_sf;
} nfapi_error_indication_msg_invalid_sfn;

typedef struct {
	uint8_t sub_error_code;
	uint8_t phich_lowest_ul_rb_index;
} nfapi_error_indication_msg_hi_err;

typedef struct {
	uint8_t sub_error_code;
	int16_t pdu_index;
} nfapi_error_indication_msg_tx_err;

// 
// P4 Message Structures
//

typedef struct {
	nfapi_tl_t tl;
	uint8_t frequency_band_indicator;
	uint16_t measurement_period;
	uint8_t bandwidth;
	uint32_t timeout;
	uint8_t number_of_earfcns;
	uint16_t earfcn[NFAPI_MAX_CARRIER_LIST];
} nfapi_lte_rssi_request_t;

#define NFAPI_LTE_RSSI_REQUEST_TAG 0x3000

#define NFAPI_P4_START_TAG NFAPI_LTE_RSSI_REQUEST_TAG

typedef struct {
	nfapi_tl_t tl;
	uint8_t frequency_band_indicator;
	uint16_t measurement_period;
	uint32_t timeout;
	uint8_t number_of_uarfcns;
	uint16_t uarfcn[NFAPI_MAX_CARRIER_LIST];
} nfapi_utran_rssi_request_t;

#define NFAPI_UTRAN_RSSI_REQUEST_TAG 0x3001

typedef struct {
	uint16_t arfcn;
	uint8_t direction;
} nfapi_arfcn_t;

typedef struct {
	nfapi_tl_t tl;
	uint8_t frequency_band_indicator;
	uint16_t measurement_period;
	uint32_t timeout;
	uint8_t number_of_arfcns;
	nfapi_arfcn_t arfcn[NFAPI_MAX_CARRIER_LIST];
} nfapi_geran_rssi_request_t;

#define NFAPI_GERAN_RSSI_REQUEST_TAG 0x3002



typedef struct {
	uint16_t earfcn;
	uint8_t number_of_ro_dl;
	uint8_t ro_dl[NFAPI_MAX_RO_DL];
} nfapi_earfcn_t;

typedef struct {
	nfapi_tl_t tl;
	uint8_t frequency_band_indicator;
	uint16_t measurement_period;
	uint32_t timeout;
	uint8_t number_of_earfcns;
	nfapi_earfcn_t earfcn[NFAPI_MAX_CARRIER_LIST];
} nfapi_nb_iot_rssi_request_t;

#define NFAPI_NB_IOT_RSSI_REQUEST_TAG 0x3020

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_rssi;
	int16_t rssi[NFAPI_MAX_RSSI];
} nfapi_rssi_indication_body_t;

#define NFAPI_RSSI_INDICATION_TAG 0x3003

typedef struct {
	nfapi_tl_t tl;
	uint16_t earfcn;
	uint8_t measurement_bandwidth;
	uint8_t exhaustive_search;
	uint32_t timeout;
	uint8_t number_of_pci;
	uint16_t pci[NFAPI_MAX_PCI_LIST];
} nfapi_lte_cell_search_request_t;

#define NFAPI_LTE_CELL_SEARCH_REQUEST_TAG 0x3004

typedef struct {
	nfapi_tl_t tl;
	uint16_t uarfcn;
	uint8_t exhaustive_search;
	uint32_t timeout;
	uint8_t number_of_psc;
	uint16_t psc[NFAPI_MAX_PSC_LIST];
} nfapi_utran_cell_search_request_t;

#define NFAPI_UTRAN_CELL_SEARCH_REQUEST_TAG 0x3005

typedef struct {
	nfapi_tl_t tl;
	uint32_t timeout;
	uint8_t number_of_arfcn;
	uint16_t arfcn[NFAPI_MAX_ARFCN_LIST];
} nfapi_geran_cell_search_request_t;

#define NFAPI_GERAN_CELL_SEARCH_REQUEST_TAG 0x3006

typedef struct {
	nfapi_tl_t tl;
	uint16_t earfcn;
	uint8_t ro_dl;
	uint8_t exhaustive_search;
	uint32_t timeout;
	uint8_t number_of_pci;
	uint16_t pci[NFAPI_MAX_PCI_LIST];
} nfapi_nb_iot_cell_search_request_t;

#define NFAPI_NB_IOT_CELL_SEARCH_REQUEST_TAG 0x3021

typedef struct {
	uint16_t pci;
	uint8_t rsrp;
	uint8_t rsrq;
	int16_t frequency_offset;
} nfapi_lte_found_cell_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_lte_cells_found;
	nfapi_lte_found_cell_t lte_found_cells[NFAPI_MAX_LTE_CELLS_FOUND];
} nfapi_lte_cell_search_indication_t;

#define NFAPI_LTE_CELL_SEARCH_INDICATION_TAG 0x3007

typedef struct {
	uint16_t psc;
	uint8_t rscp;
	uint8_t ecno;
	int16_t frequency_offset;
} nfapi_utran_found_cell_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_utran_cells_found;
	nfapi_utran_found_cell_t utran_found_cells[NFAPI_MAX_UTRAN_CELLS_FOUND];
} nfapi_utran_cell_search_indication_t;

#define NFAPI_UTRAN_CELL_SEARCH_INDICATION_TAG 0x3008

typedef struct {
	uint16_t arfcn;
	uint8_t bsic;
	uint8_t rxlev;
	uint8_t rxqual;
	int16_t frequency_offset;
	uint32_t sfn_offset;
} nfapi_gsm_found_cell_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_gsm_cells_found;
	nfapi_gsm_found_cell_t gsm_found_cells[NFAPI_MAX_GSM_CELLS_FOUND];
} nfapi_geran_cell_search_indication_t;

#define NFAPI_GERAN_CELL_SEARCH_INDICATION_TAG 0x3009

typedef struct {
	uint16_t pci;
	uint8_t rsrp;
	uint8_t rsrq;
	int16_t frequency_offset;
} nfapi_nb_iot_found_cell_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_nb_iot_cells_found;
	nfapi_nb_iot_found_cell_t nb_iot_found_cells[NFAPI_MAX_NB_IOT_CELLS_FOUND];
} nfapi_nb_iot_cell_search_indication_t;

#define NFAPI_NB_IOT_CELL_SEARCH_INDICATION_TAG 0x3022

typedef nfapi_opaqaue_data_t nfapi_pnf_cell_search_state_t;

#define NFAPI_PNF_CELL_SEARCH_STATE_TAG 0x300A

typedef struct {
	nfapi_tl_t tl;
	uint16_t earfcn;
	uint16_t pci;
	uint32_t timeout;
} nfapi_lte_broadcast_detect_request_t;

#define NFAPI_LTE_BROADCAST_DETECT_REQUEST_TAG 0x300B

typedef struct {
	nfapi_tl_t tl;
	uint16_t uarfcn;
	uint16_t psc;
	uint32_t timeout;
} nfapi_utran_broadcast_detect_request_t;

#define NFAPI_UTRAN_BROADCAST_DETECT_REQUEST_TAG 0x300C

typedef struct {
	nfapi_tl_t tl;
	uint16_t earfcn;
	uint8_t ro_dl;
	uint16_t pci;
	uint32_t timeout;
} nfapi_nb_iot_broadcast_detect_request_t;

#define NFAPI_NB_IOT_BROADCAST_DETECT_REQUEST_TAG 0x3023

typedef struct {
	nfapi_tl_t tl;
	uint8_t number_of_tx_antenna;
	uint16_t mib_length;
	uint8_t mib[NFAPI_MAX_MIB_LENGTH];
	uint32_t sfn_offset;
} nfapi_lte_broadcast_detect_indication_t;

#define NFAPI_LTE_BROADCAST_DETECT_INDICATION_TAG 0x300E

typedef struct {
	nfapi_tl_t tl;
	uint16_t mib_length;
	uint8_t mib[NFAPI_MAX_MIB_LENGTH];
	uint32_t sfn_offset;
} nfapi_utran_broadcast_detect_indication_t;

#define NFAPI_UTRAN_BROADCAST_DETECT_INDICATION_TAG 0x300F


typedef struct {
	nfapi_tl_t tl;
	uint8_t number_of_tx_antenna;
	uint16_t mib_length;
	uint8_t mib[NFAPI_MAX_MIB_LENGTH];
	uint32_t sfn_offset;
} nfapi_nb_iot_broadcast_detect_indication_t;

#define NFAPI_NB_IOT_BROADCAST_DETECT_INDICATION_TAG 0x3024

#define NFAPI_PNF_CELL_BROADCAST_STATE_TAG 0x3010

typedef struct {
	nfapi_tl_t tl;
	uint16_t earfcn;
	uint16_t pci;
	uint16_t downlink_channel_bandwidth;
	uint8_t phich_configuration;
	uint8_t number_of_tx_antenna;
	uint8_t retry_count;
	uint32_t timeout;
} nfapi_lte_system_information_schedule_request_t;

#define NFAPI_LTE_SYSTEM_INFORMATION_SCHEDULE_REQUEST_TAG 0x3011


typedef struct {
	nfapi_tl_t tl;
	uint16_t earfcn;
	uint8_t ro_dl;
	uint16_t pci;
	uint8_t scheduling_info_sib1_nb;
	uint32_t timeout;
} nfapi_nb_iot_system_information_schedule_request_t;

#define NFAPI_NB_IOT_SYSTEM_INFORMATION_SCHEDULE_REQUEST_TAG 0x3025

typedef nfapi_opaqaue_data_t nfapi_pnf_cell_broadcast_state_t;

typedef struct {
	uint8_t si_periodicity;
	uint8_t si_index;
} nfapi_lte_system_information_si_periodicity_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t earfcn;
	uint16_t pci;
	uint16_t downlink_channel_bandwidth;
	uint8_t phich_configuration;
	uint8_t number_of_tx_antenna;
	uint8_t number_of_si_periodicity;
	nfapi_lte_system_information_si_periodicity_t si_periodicity[NFAPI_MAX_SI_PERIODICITY];
	uint8_t si_window_length;
	uint32_t timeout;
} nfapi_lte_system_information_request_t;

#define NFAPI_LTE_SYSTEM_INFORMATION_REQUEST_TAG 0x3014

typedef struct {
	nfapi_tl_t tl;
	uint16_t uarfcn;
	uint16_t psc;
	uint32_t timeout;
} nfapi_utran_system_information_request_t;

#define NFAPI_UTRAN_SYSTEM_INFORMATION_REQUEST_TAG 0x3015

typedef struct {
	nfapi_tl_t tl;
	uint16_t arfcn;
	uint8_t bsic;
	uint32_t timeout;
} nfapi_geran_system_information_request_t;

#define NFAPI_GERAN_SYSTEM_INFORMATION_REQUEST_TAG 0x3016

typedef struct {
	uint8_t si_periodicity;
	uint8_t si_repetition_pattern;
	uint8_t si_tb_size;
	uint8_t number_of_si_index;
	uint8_t si_index[NFAPI_MAX_SI_INDEX];
} nfapi_nb_iot_system_information_si_periodicity_t;

typedef struct {
	nfapi_tl_t tl;
	uint16_t earfcn;
	uint8_t ro_dl;
	uint16_t pci;
	uint8_t number_of_si_periodicity;
	nfapi_nb_iot_system_information_si_periodicity_t si_periodicity[NFAPI_MAX_SI_PERIODICITY];
	uint8_t si_window_length;
	uint32_t timeout;
} nfapi_nb_iot_system_information_request_t;

#define NFAPI_NB_IOT_SYSTEM_INFORMATION_REQUEST_TAG 0x3027

typedef struct {
	nfapi_tl_t tl;
	uint8_t sib_type;
	uint16_t sib_length;
	uint8_t sib[NFAPI_MAX_SIB_LENGTH];
} nfapi_lte_system_information_indication_t;

#define NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG 0x3018

typedef struct {
	nfapi_tl_t tl;
	uint16_t sib_length;
	uint8_t sib[NFAPI_MAX_SIB_LENGTH];
} nfapi_utran_system_information_indication_t;

#define NFAPI_UTRAN_SYSTEM_INFORMATION_INDICATION_TAG 0x3019

typedef struct {
	nfapi_tl_t tl;
	uint16_t si_length;
	uint8_t si[NFAPI_MAX_SI_LENGTH];
} nfapi_geran_system_information_indication_t;

#define NFAPI_GERAN_SYSTEM_INFORMATION_INDICATION_TAG 0x301a

typedef struct {
	nfapi_tl_t tl;
	uint8_t sib_type;
	uint16_t sib_length;
	uint8_t sib[NFAPI_MAX_SIB_LENGTH];
} nfapi_nb_iot_system_information_indication_t;

#define NFAPI_NB_IOT_SYSTEM_INFORMATION_INDICATION_TAG 0x3026


//
// Top level NFAP messages
//

//
// P7
//

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_dl_config_request_body_t dl_config_request_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_dl_config_request_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_ul_config_request_body_t ul_config_request_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_ul_config_request_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_hi_dci0_request_body_t hi_dci0_request_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_hi_dci0_request_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_tx_request_body_t tx_request_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_tx_request_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
} nfapi_subframe_indication_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_harq_indication_body_t harq_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_harq_indication_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_crc_indication_body_t crc_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_crc_indication_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_sr_indication_body_t sr_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_sr_indication_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_cqi_indication_body_t cqi_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_cqi_indication_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_rach_indication_body_t rach_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_rach_indication_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_srs_indication_body_t srs_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_srs_indication_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_rx_indication_body_t rx_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_rx_indication_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_nb_harq_indication_body_t nb_harq_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nb_harq_indication_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_nrach_indication_body_t nrach_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nrach_indication_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_lbt_dl_config_request_body_t lbt_dl_config_request_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_lbt_dl_config_request_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	nfapi_lbt_dl_indication_body_t lbt_dl_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_lbt_dl_indication_t;


typedef struct {
	nfapi_p7_message_header_t header;
	uint8_t message_id;
	uint8_t error_code;
	union {
		nfapi_error_indication_msg_invalid_state	msg_invalid_state;
		nfapi_error_indication_msg_bch_missing		msg_bch_missing;
		nfapi_error_indication_sfn_out_of_sync		sfn_out_of_sync;
		nfapi_error_indication_msg_pdu_err			msg_pdu_err;
		nfapi_error_indication_msg_invalid_sfn		msg_invalid_sfn;
		nfapi_error_indication_msg_hi_err			msg_hi_err;
		nfapi_error_indication_msg_tx_err			msg_tx_err;
	};
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_error_indication_t;

typedef struct {
    nfapi_p7_message_header_t header;
    uint16_t sfn_sf;
    nfapi_ue_release_request_body_t ue_release_request_body;
    nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_ue_release_request_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_ue_release_response_t;

// 
// P4 Messages
// 

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint8_t rat_type;
	union {
		nfapi_lte_rssi_request_t					lte_rssi_request;
		nfapi_utran_rssi_request_t					utran_rssi_request;
		nfapi_geran_rssi_request_t					geran_rssi_request;
		nfapi_nb_iot_rssi_request_t					nb_iot_rssi_request;
	};
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_rssi_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_rssi_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_rssi_indication_body_t rssi_indication_body;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_rssi_indication_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint8_t rat_type;
	union {
		nfapi_lte_cell_search_request_t				lte_cell_search_request;
		nfapi_utran_cell_search_request_t			utran_cell_search_request;
		nfapi_geran_cell_search_request_t			geran_cell_search_request;
		nfapi_nb_iot_cell_search_request_t			nb_iot_cell_search_request;
	};
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_cell_search_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_cell_search_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_lte_cell_search_indication_t lte_cell_search_indication;
	nfapi_utran_cell_search_indication_t utran_cell_search_indication;
	nfapi_geran_cell_search_indication_t geran_cell_search_indication;
	nfapi_pnf_cell_search_state_t pnf_cell_search_state;
	nfapi_nb_iot_cell_search_indication_t nb_iot_cell_search_indication;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_cell_search_indication_t;


typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint8_t rat_type;
	union {
		nfapi_lte_broadcast_detect_request_t		lte_broadcast_detect_request;
		nfapi_utran_broadcast_detect_request_t		utran_broadcast_detect_request;
		nfapi_nb_iot_broadcast_detect_request_t		nb_iot_broadcast_detect_request;
	};
	nfapi_pnf_cell_search_state_t pnf_cell_search_state;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_broadcast_detect_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_broadcast_detect_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_lte_broadcast_detect_indication_t lte_broadcast_detect_indication;
	nfapi_utran_broadcast_detect_indication_t utran_broadcast_detect_indication;
	nfapi_nb_iot_broadcast_detect_indication_t nb_iot_broadcast_detect_indication;
	nfapi_pnf_cell_broadcast_state_t pnf_cell_broadcast_state;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_broadcast_detect_indication_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint8_t rat_type;
	union {
		nfapi_lte_system_information_schedule_request_t lte_system_information_schedule_request;
		nfapi_nb_iot_system_information_schedule_request_t nb_iot_system_information_schedule_request;
	};
	nfapi_pnf_cell_broadcast_state_t pnf_cell_broadcast_state;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_system_information_schedule_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_system_information_schedule_response_t;

typedef struct { 
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_lte_system_information_indication_t lte_system_information_indication;
	nfapi_nb_iot_system_information_indication_t nb_iot_system_information_indication;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_system_information_schedule_indication_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint8_t rat_type;
	union {
		nfapi_lte_system_information_request_t lte_system_information_request;
		nfapi_utran_system_information_request_t utran_system_information_request;
		nfapi_geran_system_information_request_t geran_system_information_request;
		nfapi_nb_iot_system_information_request_t nb_iot_system_information_request;
	};
	nfapi_pnf_cell_broadcast_state_t pnf_cell_broadcast_state;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_system_information_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_system_information_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_lte_system_information_indication_t lte_system_information_indication;
	nfapi_utran_system_information_indication_t utran_system_information_indication;
	nfapi_geran_system_information_indication_t geran_system_information_indication;
	nfapi_nb_iot_system_information_indication_t nb_iot_system_information_indication;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_system_information_indication_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nmm_stop_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nmm_stop_response_t;

typedef struct
{
  // TODO: see if this needs to be uncommented

  // These TLVs are used to setup the transport connection between VNF and PNF
  nfapi_ipv4_address_t p7_vnf_address_ipv4;
  nfapi_ipv6_address_t p7_vnf_address_ipv6;
  nfapi_uint16_tlv_t p7_vnf_port;

  nfapi_ipv4_address_t p7_pnf_address_ipv4;
  nfapi_ipv6_address_t p7_pnf_address_ipv6;
  nfapi_uint16_tlv_t p7_pnf_port;

  nfapi_uint8_tlv_t timing_window; //Value: 0 → 30,000 microseconds 
  nfapi_uint8_tlv_t timing_info_mode;
  nfapi_uint8_tlv_t timing_info_period;

  nfapi_uint32_tlv_t dl_tti_timing_offset;
  nfapi_uint32_tlv_t ul_tti_timing_offset;
  nfapi_uint32_tlv_t ul_dci_timing_offset;
  nfapi_uint32_tlv_t tx_data_timing_offset;
  
   // These TLVs are used to setup the transport connection between VNF and PNF
  /*
  nfapi_uint8_tlv_t dl_ue_per_sf;
  nfapi_uint8_tlv_t ul_ue_per_sf;

  // These TLVs are used by PNF to report its RF capabilities to the VNF software
  nfapi_rf_bands_t rf_bands;
*/
  // These TLVs are used by the VNF to configure the synchronization with the PNF.
 

  // These TLVs are used by the VNF to configure the RF in the PNF
  //nfapi_uint16_tlv_t max_transmit_power;
  //nfapi_uint32_tlv_t nrarfcn;

  // nfapi_nmm_frequency_bands_t nmm_gsm_frequency_bands;
  // nfapi_nmm_frequency_bands_t nmm_umts_frequency_bands;
  // nfapi_nmm_frequency_bands_t nmm_lte_frequency_bands;
  // nfapi_uint8_tlv_t nmm_uplink_rssi_supported;

} nfapi_nr_nfapi_t;

//
// Configuration options for the encode decode functions
//

/*! Configuration options for the p7 pack unpack functions
 *
 */
typedef struct nfapi_p7_codec_config {

	/*! Optional call back to allow the user to define the memory allocator. 
	 *  \param size The size of the memory to allocate
	 *  \return a pointer to a valid memory block or 0 if it has failed.
	 *
	 * If not set the nfapi unpack functions will use malloc
	 */
	void* (*allocate)(size_t size);

	/*! Optional call back to allow the user to define the memory deallocator. 
	 *  \param ptr A poiner to a memory block allocated by the allocate callback
	 * 
	 *	If not set the client should use free
	 */
	void (*deallocate)(void* ptr);

	/*! Optional call back function to handle unpacking vendor extension tlv.
	 *  \param tl A pointer to a decoded tag length structure
	 *  \param ppReadPackedMsg A handle to the read buffer. 
	 *  \param end The end of the read buffer
	 *  \param ve A handle to a vendor extention structure that the call back should allocate if the structure can be decoded
	 *  \param config A pointer to the p7 codec configuration
	 *  \return return 0 if packed successfully, -1 if failed.
	 *
	 *  If not set the tlv will be skipped
	 *
	 *  Client should use the help methods in nfapi.h to decode the vendor extention.
	 * 
	 *  \todo Add code example
	 */
	int (*unpack_vendor_extension_tlv)(nfapi_tl_t* tl, uint8_t **ppReadPackedMsg, uint8_t *end, void** ve, struct nfapi_p7_codec_config* config);

	/*! Optional call back function to handle packing vendor extension tlv. 
	 *  \param ve A pointer to a vendor extention structure.
	 *  \param ppWritePackedMsg A handle to the write buffer
	 *  \param end The end of the write buffer. The callee should make sure not to write beyond the end
	 *  \param config A pointer to the p7 codec configuration
	 *  \return return 0 if packed successfully, -1 if failed.
	 * 
	 *  If not set the the tlv will be skipped
	 * 
	 *  Client should use the help methods in nfapi.h to encode the vendor extention
	 * 
	 *  \todo Add code example
	 */
	int (*pack_vendor_extension_tlv)(void* ve, uint8_t **ppWritePackedMsg, uint8_t *end, struct nfapi_p7_codec_config* config);

	/*! Optional call back function to handle unpacking vendor extension messages. 
	 *  \param header A pointer to a decode P7 message header for the vendor extention message
	 *  \param ppReadPackedMsg A handle to the encoded data buffer
	 *  \param end A pointer to the end of the encoded data buffer
	 *  \param config  A pointer to the p7 codec configuration
	 *  \return 0 if unpacked successfully, -1 if failed
	 *
	 *  If not set the message will be ignored
	 *
	 *  If the message if is unknown the function should return -1
	 */
	int (*unpack_p7_vendor_extension)(nfapi_p7_message_header_t* header, uint8_t **ppReadPackedMsg, uint8_t *end, struct nfapi_p7_codec_config* config);

	/*! Optional call back function to handle packing vendor extension messages. 
	 *  \param header A poiner to a P7 message structure for the venfor extention message
	 *  \param ppWritePackedmsg A handle to the buffer to write the encoded message into
	 *  \param end A pointer to the end of the buffer
	 *  \param cofig A pointer to the p7 codec configuration
	 *  \return 0 if packed successfully, -1 if failed
	 * 
	 * If not set the the message will be ingored
	 *	 
	 *  If the message if is unknown the function should return -1
	 */
	int (*pack_p7_vendor_extension)(nfapi_p7_message_header_t* header, uint8_t **ppWritePackedmsg, uint8_t *end, struct nfapi_p7_codec_config* config);

	/*! Optional user data that will be passed back with callbacks
	 */
	void* user_data;

} nfapi_p7_codec_config_t;

/*! Configuration options for the p4 & p5 pack unpack functions
 *
 */
typedef struct nfapi_p4_p5_codec_config {

	/*! Optional call back to allow the user to define the memory allocator.
     *  \param size The size of the memory to allocate
	 *  \return a pointer to a valid memory block or 0 if it has failed.
	 *
	 *  If not set the nfapi unpack functions will use malloc
	 */
	void* (*allocate)(size_t size);

	/*! Optional call back to allow the user to define the memory deallocator. 
	 *  \param ptr A poiner to a memory block allocated by the allocate callback
	 *
	 *  If not set free will be used
	 */
	void (*deallocate)(void* ptr);

	/*! Optional call back function to handle unpacking vendor extension tlv.
	 *  \param tl A pointer to a decoded tag length structure
	 *  \param ppReadPackedMsg A handle to the data buffer to decode
	 *  \param end A pointer to the end of the buffer
	 *  \param ve A handle to a vendor extention structure that will be allocated by this callback
	 *  \param config A pointer to the P4/P5 codec configuration
	 *  \return 0 if unpacked successfully, -1 if failed
	 *  
	 *  If not set the tlv will be skipped
	 */
	int (*unpack_vendor_extension_tlv)(nfapi_tl_t* tl, uint8_t **ppReadPackedMsg, uint8_t *end, void** ve, struct nfapi_p4_p5_codec_config* config);

	/*! Optional call back function to handle packing vendor extension tlv. 
	 *  \param ve
	 *  \param ppWritePackedMsg A handle to the data buffer pack the tlv into
	 *  \param end A pointer to the end of the buffer
	 *  \param config A pointer to the P4/P5 codec configuration
	 *  \return 0 if packed successfully, -1 if failed
	 *
	 *  If not set the the tlv will be skipped
	 */
	int (*pack_vendor_extension_tlv)(void* ve, uint8_t **ppWritePackedMsg, uint8_t *end, struct nfapi_p4_p5_codec_config* config);

	/*! Optional call back function to handle unpacking vendor extension messages. 
	 *  \param header A pointer to a decode P4/P5 message header
	 *  \param ppReadPackgedMsg A handle to the data buffer to decode
	 *  \param end A pointer to the end of the buffer
	 *  \param config A pointer to the P4/P5 codec configuration
	 *  \return 0 if packed successfully, -1 if failed
	 *
	 * If not set the message will be ignored
	 */
	int (*unpack_p4_p5_vendor_extension)(nfapi_p4_p5_message_header_t* header, uint8_t **ppReadPackedMsg, uint8_t *end, struct nfapi_p4_p5_codec_config* config);

	/*! Optional call back function to handle packing vendor extension messages.
	 *  \param header A pointer to the P4/P5 message header to be encoded
	 *  \param ppWritePackedMsg A handle to the data buffer pack the message into
	 *  \param end A pointer to the end of the buffer
	 *  \param config A pointer to the P4/P5 codec configuration
	 *  \return 0 if packed successfully, -1 if failed
	 *  
	 *  If not set the the message will be ingored
	 */
	int (*pack_p4_p5_vendor_extension)(nfapi_p4_p5_message_header_t* header, uint8_t **ppwritepackedmsg, uint8_t *end, struct nfapi_p4_p5_codec_config* config);

	/*! Optional user data that will be passed back with callbacks
	 */
	void* user_data;

} nfapi_p4_p5_codec_config_t;

//
// Functions
// 

/*! \brief Encodes an NFAPI P4 message to a buffer
 *  \param pMessageBuf A pointer to a nfapi p4 message structure
 *  \param messageBufLen The size of the p4 message structure
 *  \param pPackedBuf A pointer to the buffer that the p4 message will be packed into
 *  \param packedBufLen The size of the buffer 
 *  \param config A pointer to the nfapi configuration structure
 *  \return 0 means success, -1 means failure.
 *
 * The function will encode a nFAPI P4 message structure pointed to be pMessageBuf into a byte stream pointed to by pPackedBuf.
 * 
 */
int nfapi_p4_message_pack(void *pMessageBuf, uint32_t messageBufLen, void *pPackedBuf, uint32_t packedBufLen, nfapi_p4_p5_codec_config_t* config);

/*! \brief Decodes a NFAPI P4 message header
 *  \param pMessageBuf A pointer to an encoded P4 message header
 *  \param messageBufLen The size of the encoded P4 message header
 *  \param pUnpackedBuf A pointer to the nfapi_message_header
 *  \param unpackedBufLen The size of nfapi_message_header structure.
 *  \param config A pointer to the nfapi configuration structure
 *  \return 0 means success, -1 means failure.
 *
 * The function will decode a byte stream pointed to by pMessageBuf into a nfapi_p4_p5_message_header structure pointer to by pUnpackedBuf
 */
int nfapi_p4_message_header_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p4_p5_codec_config_t* config);

/*! \brief Decodes a NFAPI P4 message
 *  \param pMessageBuf A pointer to an encoded P4 message
 *  \param messageBufLen The size of the encoded P4 message
 *  \param pUnpackedBuf A pointer to the nfapi_message_header
 *  \param unpackedBufLen The size of nfapi_message_header structure.
 *  \param config A pointer to the nfapi configuration structure
 *  \return 0 means success, -1 means failure.
 *
 * The function will decode a byte stream pointed to by pMessageBuf into a nfapi p4 message structure pointer to by pUnpackedBuf 
 */
int nfapi_p4_message_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p4_p5_codec_config_t* config);

/*! \brief Encodes an NFAPI P5 message to a buffer
 *  \param pMessageBuf A pointer to a nfapi p5 message structure
 *  \param messageBufLen The size of the p5 message structure
 *  \param pPackedBuf A pointer to the buffer that the p5 message will be packed into
 *  \param packedBufLen The size of the buffer 
 *  \param config A pointer to the nfapi configuration structure
 *  \return 0 means success, -1 means failure.
 *
 * The function will encode a nFAPI P5 message structure pointed to be pMessageBuf into a byte stream pointed to by pPackedBuf.
 * 
 */
int nfapi_p5_message_pack(void *pMessageBuf, uint32_t messageBufLen, void *pPackedBuf, uint32_t packedBufLen, nfapi_p4_p5_codec_config_t* config);
int nfapi_nr_p5_message_pack(void *pMessageBuf, uint32_t messageBufLen, void *pPackedBuf, uint32_t packedBufLen, nfapi_p4_p5_codec_config_t* config);

/*! \brief Decodes an NFAPI P5 message header
 *  \param pMessageBuf A pointer to an encoded P5 message header
 *  \param messageBufLen The size of the encoded P5 message header
 *  \param pUnpackedBuf A pointer to the nfapi_message_header
 *  \param unpackedBufLen The size of nfapi_message_header structure.
 *  \param config A pointer to the nfapi configuration structure
 *  \return 0 means success, -1 means failure.
 *
 * The function will decode a byte stream pointed to by pMessageBuf into a nfapi_p4_p5_message_header structure pointer to by pUnpackedBuf
 */
int nfapi_p5_message_header_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p4_p5_codec_config_t* config);

/*! \brief Decodes a NFAPI P5 message
 *  \param pMessageBuf A pointer to an encoded P5 message
 *  \param messageBufLen The size of the encoded P5 message
 *  \param pUnpackedBuf A pointer to the nfapi_message_header
 *  \param unpackedBufLen The size of nfapi_message_header structure.
 *  \param config A pointer to the nfapi configuration structure
 *  \return 0 means success, -1 means failure.
 *
 * The function will decode a byte stream pointed to by pMessageBuf into a nfapi p5 message structure pointer to by pUnpackedBuf 
 */
int nfapi_nr_p5_message_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p4_p5_codec_config_t* config);
int nfapi_p5_message_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p4_p5_codec_config_t* config);

/*! \brief Encodes an NFAPI P7 message to a buffer
 *  \param pMessageBuf A pointer to a nfapi p7 message structure
 *  \param pPackedBuf A pointer to the buffer that the p7 message will be packed into
 *  \param packedBufLen The size of the buffer 
 *  \param config A pointer to the nfapi configuration structure
 *  \return 0 means success, -1 means failure.
 *
 * The function will encode a nFAPI P7 message structure pointed to be pMessageBuf into a byte stream pointed to by pPackedBuf.
 * 
 */
int nfapi_p7_message_pack(void *pMessageBuf, void *pPackedBuf, uint32_t packedBufLen, nfapi_p7_codec_config_t* config);
int nfapi_nr_p7_message_pack(void *pMessageBuf, void *pPackedBuf, uint32_t packedBufLen, nfapi_p7_codec_config_t* config);

/*! \brief Decodes an NFAPI P7 message header
 *  \param pMessageBuf A pointer to an encoded P7 message header
 *  \param messageBufLen The size of the encoded P7 message header
 *  \param pUnpackedBuf A pointer to the nfapi_message_header
 *  \param unpackedBufLen The size of nfapi_message_header structure.
 *  \param config A pointer to the nfapi configuration structure
 *  \return 0 means success, -1 means failure.
 *
 * The function will decode a byte stream pointed to by pMessageBuf into a nfapi_p7_message_header structure pointer to by pUnpackedBuf

 */
int nfapi_p7_message_header_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p7_codec_config_t* config);

/*! \brief Decodes a NFAPI P7 message
 *  \param pMessageBuf A pointer to an encoded P7 message
 *  \param messageBufLen The size of the encoded P7 message
 *  \param pUnpackedBuf A pointer to the nfapi_message_header
 *  \param unpackedBufLen The size of nfapi_message_header structure.
 *  \param config A pointer to the nfapi configuration structure
 *  \return 0 means success, -1 means failure.
 *
 * The function will decode a byte stream pointed to by pMessageBuf into a nfapi p7 message structure pointer to by pUnpackedBuf 
 */
int nfapi_p7_message_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p7_codec_config_t* config);
int nfapi_nr_p7_message_unpack(void *pMessageBuf, uint32_t messageBufLen, void *pUnpackedBuf, uint32_t unpackedBufLen, nfapi_p7_codec_config_t* config);

/*! \brief Calculates the checksum of a  message
 *
 *  \param buffer Pointer to the packed message
 *  \param len The length of the message
 *  \return The checksum. If there is an error the function with return -1
 */
uint32_t nfapi_p7_calculate_checksum(uint8_t* buffer, uint32_t len);

/*! \brief Calculates & updates the checksum in the message
 *
 *  \param buffer Pointer to the packed message
 *  \param len The length of the message
  *  \return 0 means success, -1 means failure.
 */
int nfapi_p7_update_checksum(uint8_t* buffer, uint32_t len);

/*! \brief Updates the transmition time stamp in the p7 message header
 *
 *  \param buffer Pointer to the packed message
 *  \param timestamp The time stamp value
  *  \return 0 means success, -1 means failure.
 */
int nfapi_p7_update_transmit_timestamp(uint8_t* buffer, uint32_t timestamp);

#endif /* _NFAPI_INTERFACE_H_ */
