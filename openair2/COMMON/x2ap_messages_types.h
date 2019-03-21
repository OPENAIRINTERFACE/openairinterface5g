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

#ifndef X2AP_MESSAGES_TYPES_H_
#define X2AP_MESSAGES_TYPES_H_

#include "s1ap_messages_types.h"
#include "LTE_PhysCellId.h"

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.

#define X2AP_REGISTER_ENB_REQ(mSGpTR)           (mSGpTR)->ittiMsg.x2ap_register_enb_req
#define X2AP_HANDOVER_REQ(mSGpTR)               (mSGpTR)->ittiMsg.x2ap_handover_req
#define X2AP_HANDOVER_REQ_ACK(mSGpTR)           (mSGpTR)->ittiMsg.x2ap_handover_req_ack
#define X2AP_REGISTER_ENB_CNF(mSGpTR)           (mSGpTR)->ittiMsg.x2ap_register_enb_cnf
#define X2AP_DEREGISTERED_ENB_IND(mSGpTR)       (mSGpTR)->ittiMsg.x2ap_deregistered_enb_ind
#define X2AP_UE_CONTEXT_RELEASE(mSGpTR)         (mSGpTR)->ittiMsg.x2ap_ue_context_release
#define X2AP_HANDOVER_CANCEL(mSGpTR)            (mSGpTR)->ittiMsg.x2ap_handover_cancel


#define X2AP_MAX_NB_ENB_IP_ADDRESS 2

// eNB application layer -> X2AP messages

/* X2AP UE CONTEXT RELEASE */
typedef struct x2ap_ue_context_release_s {
  /* used for X2AP->RRC in source and RRC->X2AP in target */
  int rnti;

  int source_assoc_id;
} x2ap_ue_context_release_t;

typedef enum {
  X2AP_T_RELOC_PREP_TIMEOUT,
  X2AP_TX2_RELOC_OVERALL_TIMEOUT
} x2ap_handover_cancel_cause_t;

typedef struct x2ap_handover_cancel_s {
  int rnti;
  x2ap_handover_cancel_cause_t cause;
} x2ap_handover_cancel_t;

typedef struct x2ap_register_enb_req_s {
  /* Unique eNB_id to identify the eNB within EPC.
   * For macro eNB ids this field should be 20 bits long.
   * For home eNB ids this field should be 28 bits long.
   */
  uint32_t eNB_id;
  /* The type of the cell */
  enum cell_type_e cell_type;

  /* Optional name for the cell
   * NOTE: the name can be NULL (i.e no name) and will be cropped to 150
   * characters.
   */
  char *eNB_name;

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
  net_ip_address_t enb_x2_ip_address;

  /* Nb of MME to connect to */
  uint8_t          nb_x2;

  /* List of target eNB to connect to for X2*/
  net_ip_address_t target_enb_x2_ip_address[X2AP_MAX_NB_ENB_IP_ADDRESS];

  /* Number of SCTP streams used for associations */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;

  /* eNB port for X2C*/
  uint32_t enb_port_for_X2C;

  /* timers (unit: millisecond) */
  int t_reloc_prep;
  int tx2_reloc_overall;
} x2ap_register_enb_req_t;

typedef struct x2ap_subframe_process_s {
  /* nothing, we simply use the module ID in the header */
} x2ap_subframe_process_t;

//-------------------------------------------------------------------------------------------//
// X2AP -> eNB application layer messages
typedef struct x2ap_register_enb_cnf_s {
  /* Nb of connected eNBs*/
  uint8_t          nb_x2;
} x2ap_register_enb_cnf_t;

typedef struct x2ap_deregistered_enb_ind_s {
  /* Nb of connected eNBs */
  uint8_t          nb_x2;
} x2ap_deregistered_enb_ind_t;

//-------------------------------------------------------------------------------------------//
// X2AP <-> RRC
typedef struct x2ap_gummei_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_len;
  uint8_t  mme_code;
  uint16_t mme_group_id;
} x2ap_gummei_t;

typedef struct x2ap_lastvisitedcell_info_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_len;
  LTE_PhysCellId_t target_physCellId;
  cell_type_t cell_type;
  uint64_t time_UE_StayedInCell;
}x2ap_lastvisitedcell_info_t;

typedef struct x2ap_handover_req_s {
  /* used for RRC->X2AP in source eNB */
  int rnti;

  /* used for X2AP->RRC in target eNB */
  int x2_id;

  LTE_PhysCellId_t target_physCellId;

  x2ap_gummei_t ue_gummei;

  /*UE-ContextInformation */

  /* MME UE id  */
  uint32_t mme_ue_s1ap_id;

  security_capabilities_t security_capabilities;

  uint8_t      kenb[32]; // keNB or keNB*

  /*next_hop_chaining_coun */
  long int     kenb_ncc;

  /* UE aggregate maximum bitrate */
  ambr_t ue_ambr;

  uint8_t nb_e_rabs_tobesetup;

 /* list of e_rab setup-ed by RRC layers */
  e_rab_setup_t e_rabs_tobesetup[S1AP_MAX_E_RAB];

  /* list of e_rab to be setup by RRC layers */
  e_rab_t  e_rab_param[S1AP_MAX_E_RAB];

  x2ap_lastvisitedcell_info_t lastvisitedcell_info;

  uint8_t rrc_buffer[1024 /* arbitrary, big enough */];
  int rrc_buffer_size;

  int target_assoc_id;
} x2ap_handover_req_t;

typedef struct x2ap_handover_req_ack_s {
  /* used for RRC->X2AP in target and X2AP->RRC in source */
  int rnti;

  /* used for RRC->X2AP in target */
  int x2_id_target;

  int source_assoc_id;

  uint8_t nb_e_rabs_tobesetup;

 /* list of e_rab setup-ed by RRC layers */
  e_rab_setup_t e_rabs_tobesetup[S1AP_MAX_E_RAB];

  /* list of e_rab to be setup by RRC layers */
  e_rab_t  e_rab_param[S1AP_MAX_E_RAB];

  uint8_t rrc_buffer[1024 /* arbitrary, big enough */];
  int rrc_buffer_size;

  uint32_t mme_ue_s1ap_id;
} x2ap_handover_req_ack_t;

#endif /* X2AP_MESSAGES_TYPES_H_ */
