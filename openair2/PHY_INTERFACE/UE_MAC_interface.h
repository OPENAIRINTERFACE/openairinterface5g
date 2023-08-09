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

/* This is the interface module between PHY
 * Provided the FAPI style interface structures for P7.
 */

/*! \file openair2/PHY_INTERFACE/IF_Module.h
* \brief data structures for PHY/MAC interface modules
* \author EURECOM/NTUST
* \date 2017
* \version 0.1
* \company Eurecom
* \email: raymond.knopp@eurecom.fr
* \note
* \warning
*/
#ifndef __UE_MAC_INTERFACE__H__
#define __UE_MAC_INTERFACE__H__

#include "nfapi_interface.h"
#include "openair1/PHY/defs_RU.h"
#include "common/openairinterface5g_limits.h"

#define MAX_NUM_DL_PDU 100
#define MAX_NUM_UL_PDU 100
#define MAX_NUM_HI_DCI0_PDU 100
#define MAX_NUM_TX_REQUEST_PDU 100

#define MAX_NUM_HARQ_IND 100
#define MAX_NUM_CRC_IND 100
#define MAX_NUM_SR_IND 100
#define MAX_NUM_CQI_IND 100
#define MAX_NUM_RACH_IND 100
#define MAX_NUM_SRS_IND 100


// UE_MAC enums
typedef enum {
	UE_MAC_DL_IND_PDSCH_PDU_TYPE =0,
	UE_MAC_DL_IND_SI_PDSCH_PDU_TYPE,
	UE_MAC_DL_IND_P_PDSCH_PDU_TYPE,
	UE_MAC_DL_IND_DLSCH_RAR_PDU_TYPE
} UE_MAC_dl_ind_pdu_type_e;

// UE_MAC enums
typedef enum {
	UE_MAC_Tx_IND_Msg1_TYPE =0,
	UE_MAC_Tx_IND_Msg3_TYPE
} UE_MAC_Tx_ind_type_e;


// *** UE_Tx.request related structures

typedef struct {
	uint16_t pdu_length;
	uint16_t pdu_index;
	uint8_t num_segments;
	struct {
		uint32_t segment_length;
		uint8_t* segment_data;
	} segments[NFAPI_TX_MAX_SEGMENTS];
} UE_MAC_tx_request_pdu_t;


typedef struct {
	nfapi_tl_t tl;
	uint16_t number_of_pdus;
	UE_MAC_tx_request_pdu_t* ue_tx_pdu_list;
} UE_MAC_tx_request_body_t;


typedef struct {
	//nfapi_p7_message_header_t header;
	uint16_t sfn_sf;
	UE_MAC_tx_request_body_t ue_tx_request_body;
} UE_MAC_tx_request_t;


typedef struct{


}UE_MAC_sl_config_request_Tx_t;

typedef struct{


}UE_MAC_sl_config_request_Rx_t;


typedef struct{


}UE_MAC_sl_tx_request_t;






// *** UE_DL.indication related structures

typedef struct{
	unsigned char eNB_index;
	uint8_t       first_sync; //boolean 0 or 1
	uint8_t       sync; // boolean 0 or 1 to indicate whether rrc_out_of_sync_ind() or dl_phy_sync_success()
						// should be called from the handler function of the interface respectively.
}UE_MAC_bch_indication_pdu_t;


typedef struct{
	nfapi_tl_t tl;
	UE_MAC_bch_indication_pdu_t* bch_ind_list;
}UE_MAC_BCH_indication_body_t;


// Corresponding to inputs of MAC functions: ue_send_sdu(), ue_decode_si(), ue_decode_p().
typedef struct{
	uint8_t* 	data;
	uint16_t 	data_len;
}UE_MAC_dlsch_pdu;


// Corresponding to inputs of MAC function: process_rar().
typedef struct{
	rnti_t 		ra_rnti;
	uint8_t* 	rar_input_buffer; // Originating from PHY
	rnti_t* 	t_crnti;
	uint8_t 	preamble_index;
	uint8_t* 	rar_output_buffer; //should be returned to PHY: dlsch0->harq_processes[0]->b
}UE_MAC_dlsch_rar_pdu;


typedef struct{
	uint8_t pdu_type;
	uint8_t eNB_index;
	union{
		UE_MAC_dlsch_pdu 	 dlsch_pdu_ind;
		UE_MAC_dlsch_rar_pdu dlsch_rar_pdu_ind;
	};
}UE_MAC_dlsch_indication_pdu_t;


typedef struct{
	nfapi_tl_t tl;
	uint16_t number_of_pdus;
	UE_MAC_dlsch_indication_pdu_t* dlsch_ind_list;
}UE_MAC_DLSCH_indication_body_t;






// *** UE_SL.indication related structures

typedef struct{

}ue_sci_indication_body_t;


typedef struct{

}ue_SLSCH_indication_body_t;


typedef struct{

}ue_SLDCH_indication_body_t;

typedef struct{

}ue_SLBCH_indication_body_t;


// *** UE_Config_common.request related structures

typedef struct {
	uint8_t subframeAssignment;
	uint8_t specialSubframePatterns;
}UE_PHY_tdd_frame_structure_t;


typedef struct {
	uint16_t rootSequenceIndex;
	uint8_t prach_Config_enabled;
	uint8_t prach_ConfigIndex;
	uint8_t highSpeedFlag;
	uint8_t zeroCorrelationZoneConfig;
	uint8_t prach_FreqOffset;
}UE_PHY_prach_config_t;

typedef struct {
	uint8_t deltaPUCCH_Shift;
	uint8_t nRB_CQI;
	uint8_t nCS_AN;
	uint16_t n1PUCCH_AN;
}UE_PHY_pucch_config_t;


typedef struct {
	int8_t referenceSignalPower;
	uint8_t p_b;
}UE_PHY_pdsch_config_t;


typedef struct {
	uint8_t n_SB;
	PUSCH_HOPPING_t hoppingMode;
	uint8_t pusch_HoppingOffset;
	uint8_t enable64QAM;
	uint8_t groupHoppingEnabled;
	uint8_t groupAssignmentPUSCH;
	uint8_t sequenceHoppingEnabled;
	uint8_t cyclicShift;
}UE_PHY_pusch_config_t;


typedef struct{
	uint8_t enabled_flag;
	uint8_t srs_BandwidthConfig;
	uint8_t srs_SubframeConfig;
	uint8_t ackNackSRS_SimultaneousTransmission;
	uint8_t srs_MaxUpPts;
}UE_PHY_SRS_config_t;

typedef struct{
	int8_t p0_NominalPUSCH;
	PUSCH_alpha_t alpha;
	int8_t p0_NominalPUCCH;
	int8_t deltaPreambleMsg3;
	long deltaF_PUCCH_Format1;
	long deltaF_PUCCH_Format1b;
	long deltaF_PUCCH_Format2;
	long deltaF_PUCCH_Format2a;
	long deltaF_PUCCH_Format2b;
}UE_PHY_UL_power_control_config_t;


typedef struct{
	uint8_t maxHARQ_Msg3Tx;
}UE_PHY_HARQ_Msg3_config_t;

typedef struct{
	uint8_t nb_antennas_tx;
}UE_PHY_antenna_config_t;

typedef struct{
	PHICH_RESOURCE_t phich_resource;
	PHICH_DURATION_t phich_duration;
}UE_PHY_phich_config_t;


typedef struct {
	UE_PHY_tdd_frame_structure_t ue_tdd_frame_structure_config;
	UE_PHY_prach_config_t ue_prach_config;
	UE_PHY_pucch_config_t ue_pucch_config;
	UE_PHY_pdsch_config_t ue_pdsch_config;
	UE_PHY_pusch_config_t ue_pusch_config;
	UE_PHY_SRS_config_t   ue_srs_config;
	UE_PHY_UL_power_control_config_t ue_ul_pow_cntl_config;
	UE_PHY_HARQ_Msg3_config_t ue_harq_msg3_config;
	/* Where can we find the types and values of the configuration for the PCH?
	UE_MAC_pusch_config_t ue_pch_config
	radioResourceConfigCommon->pcch_Config.defaultPagingCycle, radioResourceConfigCommon->pcch_Config.nB*/
	UE_PHY_antenna_config_t ue_ant_config;
	UE_PHY_phich_config_t ue_phich_config;
	/* MBSFN?*/
}UE_PHY_config_common_request_t;







// *** UE_Config_dedicated. request related structures

typedef struct{
	PA_t p_a;
}UE_PHY_pdsch_config_dedicated_t;


typedef struct{
	uint8_t ackNackRepetition;
	ANFBmode_t tdd_AckNackFeedbackMode;
	//ACKNAKREP_t repetitionFactor;
	//uint16_t n1PUCCH_AN_Rep;
}UE_PHY_pucch_config_dedicated_t;


typedef struct{
	uint16_t betaOffset_ACK_Index;
	uint16_t betaOffset_RI_Index;
	uint16_t betaOffset_CQI_Index;
}UE_PHY_pusch_config_dedicated_t;


typedef struct{
	int8_t p0_UE_PUSCH;
	uint8_t deltaMCS_Enabled;
	uint8_t accumulationEnabled;
	int8_t p0_UE_PUCCH;
	int8_t pSRS_Offset;
	uint8_t filterCoefficient;
}UE_PHY_ul_power_control_config_dedicated_t;


typedef struct{
	uint16_t sr_PUCCH_ResourceIndex;
	uint8_t sr_ConfigIndex;
	DSR_TRANSMAX_t dsr_TransMax;
}UE_PHY_SR_config_dedicated_t;


typedef struct{
	uint8_t srsConfigDedicatedSetup;
	uint8_t duration;
	uint8_t cyclicShift;
	uint8_t freqDomainPosition;
	uint8_t srs_Bandwidth;
	uint16_t srs_ConfigIndex;
	uint8_t srs_HoppingBandwidth;
	uint8_t transmissionComb;
	//uint8_t srsCellSubframe;
	//uint8_t srsUeSubframe;
}UE_PHY_srs_ul_config_dedicated_t;


typedef struct{
	CQI_REPORTMODEAPERIODIC cqi_ReportModeAperiodic;
	CQI_REPORTPERIODIC CQI_ReportPeriodic;
	//int8_t nomPDSCH_RS_EPRE_Offset;
}UE_PHY_cqi_report_config_dedicated_t;



typedef struct{
	uint8_t transmission_mode [NUMBER_OF_CONNECTED_eNB_MAX];
	UE_PHY_pdsch_config_dedicated_t ue_pdsch_config;
	UE_PHY_pucch_config_dedicated_t ue_pucch_config;
	UE_PHY_pusch_config_dedicated_t ue_pusch_config;
	UE_PHY_ul_power_control_config_dedicated_t ue_ul_pow_cntrl_config;
	UE_PHY_SR_config_dedicated_t	ue_SR_config;
	UE_PHY_srs_ul_config_dedicated_t ue_srs_ul_config;
	UE_PHY_cqi_report_config_dedicated_t ue_cqi_report_config;
}UE_PHY_config_dedicated_request_t;

#endif



















/*
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
} nfapi_hi_dci0_dci_pdu_rel8_t;

*/
