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

#ifndef M3AP_MESSAGES_TYPES_H_
#define M3AP_MESSAGES_TYPES_H_

#include "s1ap_messages_types.h"
#include "LTE_PhysCellId.h"

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.
#define M3AP_MME_SCTP_REQ(mSGpTR)		(mSGpTR)->ittiMsg.m3ap_mme_sctp_req

#define M3AP_REGISTER_MCE_REQ(mSGpTR)           (mSGpTR)->ittiMsg.m3ap_register_mce_req
//#define M3AP_HANDOVER_REQ(mSGpTR)               (mSGpTR)->ittiMsg.m3ap_handover_req
//#define M3AP_HANDOVER_REQ_ACK(mSGpTR)           (mSGpTR)->ittiMsg.m3ap_handover_req_ack
#define M3AP_REGISTER_MCE_CNF(mSGpTR)           (mSGpTR)->ittiMsg.m3ap_register_mce_cnf
#define M3AP_DEREGISTERED_MCE_IND(mSGpTR)       (mSGpTR)->ittiMsg.m3ap_deregistered_mce_ind
//#define M3AP_UE_CONTEXT_RELEASE(mSGpTR)         (mSGpTR)->ittiMsg.m3ap_ue_context_release
//#define M3AP_HANDOVER_CANCEL(mSGpTR)            (mSGpTR)->ittiMsg.m3ap_handover_cancel


#define M3AP_MBMS_SESSION_START_REQ(mSGpTR)            	(mSGpTR)->ittiMsg.m3ap_session_start_req
#define M3AP_MBMS_SESSION_START_RESP(mSGpTR)            (mSGpTR)->ittiMsg.m3ap_session_start_resp
#define M3AP_MBMS_SESSION_START_FAILURE(mSGpTR)         (mSGpTR)->ittiMsg.m3ap_session_start_failure
#define M3AP_MBMS_SESSION_STOP_REQ(mSGpTR)            	(mSGpTR)->ittiMsg.m3ap_session_stop_req
#define M3AP_MBMS_SESSION_STOP_RESP(mSGpTR)           	(mSGpTR)->ittiMsg.m3ap_session_stop_resp
#define M3AP_MBMS_SESSION_STOP_FAILURE(mSGpTR)          (mSGpTR)->ittiMsg.m3ap_session_stop_failure
#define M3AP_ERROR_INDICATION(mSGpTR)          		(mSGpTR)->ittiMsg.m3ap_error_indication
#define M3AP_RESET(mSGpTR)          			(mSGpTR)->ittiMsg.m3ap_reset
#define M3AP_RESET_ACK(mSGpTR)          		(mSGpTR)->ittiMsg.m3ap_reset_ack
#define M3AP_MBMS_SESSION_UPDATE_REQ(mSGpTR)            (mSGpTR)->ittiMsg.m3ap_mbms_session_update_req
#define M3AP_MBMS_SESSION_UPDATE_RESP(mSGpTR)           (mSGpTR)->ittiMsg.m3ap_mbms_session_update_resp
#define M3AP_MBMS_SESSION_UPDATE_FAILURE(mSGpTR)        (mSGpTR)->ittiMsg.m3ap_mbms_session_update_failure
#define M3AP_SETUP_REQ(mSGpTR)				(mSGpTR)->ittiMsg.m3ap_setup_req
#define M3AP_SETUP_RESP(mSGpTR)				(mSGpTR)->ittiMsg.m3ap_setup_resp
#define M3AP_SETUP_FAILURE(mSGpTR)			(mSGpTR)->ittiMsg.m3ap_setup_failure
#define M3AP_MCE_CONFIGURATION_UPDATE(mSGpTR)		(mSGpTR)->ittiMsg.m3ap_mce_configuration_update
#define M3AP_MCE_CONFIGURATION_UPDATE_ACK(mSGpTR)	(mSGpTR)->ittiMsg.m3ap_mce_configuration_update_ack
#define M3AP_MCE_CONFIGURATION_UPDATE_FAILURE(mSGpTR)	(mSGpTR)->ittiMsg.m3ap_mce_configuration_update_failure


#define M3AP_MAX_NB_MCE_IP_ADDRESS 2

#define M3AP_MAX_NB_MME_IP_ADDRESS 2


typedef struct m3ap_net_ip_address_s {
  unsigned ipv4:1;
  unsigned ipv6:1;
  char ipv4_address[16];
  char ipv6_address[46];
} m3ap_net_ip_address_t;

// eNB application layer -> M3AP messages

/* M3AP UE CONTEXT RELEASE */
//typedef struct m3ap_ue_context_release_s {
//  /* used for M3AP->RRC in source and RRC->M3AP in target */
//  int rnti;
//
//  int source_assoc_id;
//} m3ap_ue_context_release_t;

//typedef enum {
//  M3AP_T_RELOC_PREP_TIMEOUT,
//  M3AP_TX2_RELOC_OVERALL_TIMEOUT
//} m3ap_handover_cancel_cause_t;

//typedef struct m3ap_handover_cancel_s {
//  int rnti;
//  m3ap_handover_cancel_cause_t cause;
//} m3ap_handover_cancel_t;

typedef struct m3ap_register_mce_req_s {
  /* Unique eNB_id to identify the eNB within EPC.
   * For macro eNB ids this field should be 20 bits long.
   * For home eNB ids this field should be 28 bits long.
   */
  uint32_t MCE_id;
  /* The type of the cell */
  enum cell_type_e cell_type;

  /* Optional name for the cell
   * NOTE: the name can be NULL (i.e no name) and will be cropped to 150
   * characters.
   */
  char *MCE_name;

  /* Tracking area code */
  uint16_t tac;

  /* Mobile Country Code
   * Mobile Network Code
   */
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_digit_length;

  /*
   * CC Params
   */
  int16_t                 eutra_band[MAX_NUM_CCs];
  uint32_t                downlink_frequency[MAX_NUM_CCs];
  int32_t                 uplink_frequency_offset[MAX_NUM_CCs];
  uint32_t                Nid_cell[MAX_NUM_CCs];
  int16_t                 N_RB_DL[MAX_NUM_CCs];
  lte_frame_type_t        frame_type[MAX_NUM_CCs];
  uint32_t                fdd_earfcn_DL[MAX_NUM_CCs];
  uint32_t                fdd_earfcn_UL[MAX_NUM_CCs];
  int                     num_cc;

  /* To be considered for TDD */
  //uint16_t tdd_EARFCN;
  //uint16_t tdd_Transmission_Bandwidth;

  /* The local eNB IP address to bind */
  net_ip_address_t mme_m3_ip_address;

  /* Nb of MME to connect to */
  uint8_t          nb_m3;

  /* List of target eNB to connect to for M3*/
  net_ip_address_t target_mme_m3_ip_address[M3AP_MAX_NB_MCE_IP_ADDRESS];

  /* Number of SCTP streams used for associations */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;

  /* eNB port for M3C*/
  uint32_t mme_port_for_M3C;

  /* timers (unit: millisecond) */
  int t_reloc_prep;
  int tm3_reloc_overall;

  /* Nb of MME to connect to */
  uint8_t          nb_mme;
  /* List of MME to connect to */
  net_ip_address_t mme_ip_address[M3AP_MAX_NB_MME_IP_ADDRESS];
} m3ap_register_mce_req_t;

typedef struct m3ap_subframe_process_s {
  /* nothing, we simply use the module ID in the header */
} m3ap_subframe_process_t;

//-------------------------------------------------------------------------------------------//
// M3AP -> eNB application layer messages
typedef struct m3ap_register_mce_cnf_s {
  /* Nb of connected eNBs*/
  uint8_t          nb_mme;
} m3ap_register_mce_cnf_t;

typedef struct m3ap_deregistered_mce_ind_s {
  /* Nb of connected eNBs */
  uint8_t          nb_mme;
} m3ap_deregistered_mce_ind_t;

//-------------------------------------------------------------------------------------------//
// M3AP <-> RRC
//typedef struct m3ap_gummei_s {
//  uint16_t mcc;
//  uint16_t mnc;
//  uint8_t  mnc_len;
//  uint8_t  mme_code;
//  uint16_t mme_group_id;
//} m3ap_gummei_t;
//
//typedef struct m3ap_lastvisitedcell_info_s {
//  uint16_t mcc;
//  uint16_t mnc;
//  uint8_t  mnc_len;
//  LTE_PhysCellId_t target_physCellId;
//  cell_type_t cell_type;
//  uint64_t time_UE_StayedInCell;
//}m3ap_lastvisitedcell_info_t;
//
//typedef struct m3ap_handover_req_s {
//  /* used for RRC->M3AP in source eNB */
//  int rnti;
//
//  /* used for M3AP->RRC in target eNB */
//  int x2_id;
//
//  LTE_PhysCellId_t target_physCellId;
//
//  m3ap_gummei_t ue_gummei;
//
//  /*UE-ContextInformation */
//
//  /* MME UE id  */
//  uint32_t mme_ue_s1ap_id;
//
//  security_capabilities_t security_capabilities;
//
//  uint8_t      kenb[32]; // keNB or keNB*
//
//  /*next_hop_chaining_coun */
//  long int     kenb_ncc;
//
//  /* UE aggregate maximum bitrate */
//  ambr_t ue_ambr;
//
//  uint8_t nb_e_rabs_tobesetup;
//
// /* list of e_rab setup-ed by RRC layers */
//  e_rab_setup_t e_rabs_tobesetup[S1AP_MAX_E_RAB];
//
//  /* list of e_rab to be setup by RRC layers */
//  e_rab_t  e_rab_param[S1AP_MAX_E_RAB];
//
//  m3ap_lastvisitedcell_info_t lastvisitedcell_info;
//
//  uint8_t rrc_buffer[1024 /* arbitrary, big enough */];
//  int rrc_buffer_size;
//
//  int target_assoc_id;
//} m3ap_handover_req_t;
//
//typedef struct m3ap_handover_req_ack_s {
//  /* used for RRC->M3AP in target and M3AP->RRC in source */
//  int rnti;
//
//  /* used for RRC->M3AP in target */
//  int x2_id_target;
//
//  int source_assoc_id;
//
//  uint8_t nb_e_rabs_tobesetup;
//
// /* list of e_rab setup-ed by RRC layers */
//  e_rab_setup_t e_rabs_tobesetup[S1AP_MAX_E_RAB];
//
//  /* list of e_rab to be setup by RRC layers */
//  e_rab_t  e_rab_param[S1AP_MAX_E_RAB];
//
//  uint8_t rrc_buffer[1024 /* arbitrary, big enough */];
//  int rrc_buffer_size;
//
//  uint32_t mme_ue_s1ap_id;
//} m3ap_handover_req_ack_t;
//

typedef struct m3ap_mme_sctp_req_s {
  	/* the local mce ip address to bind */
	net_ip_address_t mme_m3_ip_address;

  	/* enb port for m2c*/
  	uint32_t mme_port_for_M3C;
}m3ap_mme_sctp_req_t;



typedef struct m3ap_session_start_req_s{
}m3ap_session_start_req_t;
typedef struct m3ap_session_start_resp_s{
}m3ap_session_start_resp_t;
typedef struct m3ap_session_start_failure_s{
}m3ap_session_start_failure_t;
typedef struct m3ap_session_stop_req_s{
}m3ap_session_stop_req_t;
typedef struct m3ap_session_stop_resp_s{
}m3ap_session_stop_resp_t;
typedef struct m3ap_session_stop_failure_s{
}m3ap_session_stop_failure_t;
typedef struct m3ap_error_indication_s{
}m3ap_error_indication_t;
typedef struct m3ap_reset_s{
}m3ap_reset_t;
typedef struct m3ap_reset_ack_s{
}m3ap_reset_ack_t;
typedef struct m3ap_mbms_session_update_req_s{
}m3ap_mbms_session_update_req_t;
typedef struct m3ap_mbms_session_update_resp_s{
}m3ap_mbms_session_update_resp_t;
typedef struct m3ap_mbms_session_update_failure_s{
}m3ap_mbms_session_update_failure_t;
typedef struct m3ap_setup_req_s{
  /* Connexion id used between SCTP/M3AP */
  uint16_t cnx_id;
  /* SCTP association id */
  int32_t  assoc_id;

  /* Number of SCTP streams used for a mme association */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;

  uint16_t default_sctp_stream_id;
}m3ap_setup_req_t;
typedef struct m3ap_setup_resp_s{
}m3ap_setup_resp_t;
typedef struct m3ap_setup_failure_s{
}m3ap_setup_failure_t;
typedef struct m3ap_mce_configuration_update_s{
}m3ap_mce_configuration_update_t;
typedef struct m3ap_mce_configuration_update_ack_s{
}m3ap_mce_configuration_update_ack_t;
typedef struct m3ap_mce_configuration_update_failure_s{
}m3ap_mce_configuration_update_failure_t;












#endif /* M3AP_MESSAGES_TYPES_H_ */
