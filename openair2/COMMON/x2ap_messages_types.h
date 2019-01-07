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

#include "LTE_PhysCellId.h"

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.

#define X2AP_REGISTER_ENB_REQ(mSGpTR)           (mSGpTR)->ittiMsg.x2ap_register_enb_req
#define X2AP_HANDOVER_REQ(mSGpTR)               (mSGpTR)->ittiMsg.x2ap_handover_req
#define X2AP_HANDOVER_REQ_ACK(mSGpTR)           (mSGpTR)->ittiMsg.x2ap_handover_req_ack
#define X2AP_REGISTER_ENB_CNF(mSGpTR)           (mSGpTR)->ittiMsg.x2ap_register_enb_cnf
#define X2AP_DEREGISTERED_ENB_IND(mSGpTR)       (mSGpTR)->ittiMsg.x2ap_deregistered_enb_ind


#define X2AP_MAX_NB_ENB_IP_ADDRESS 2

// eNB application layer -> X2AP messages
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
} x2ap_register_enb_req_t;

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

//used for src
typedef struct x2ap_handover_req_s {
  int source_rnti;                       /* TODO: to be fixed/remove */
  int source_x2id;                       /* TODO: to be fixed/remove */

  unsigned  old_eNB_ue_s1ap_id:24;

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

  /* ue_context_pP->ue_context.e_rab[i].param.sgw_addr; */

  x2ap_lastvisitedcell_info_t lastvisitedcell_info;

  /* TODO: this parameter has to be removed */
  int target_mod_id;
} x2ap_handover_req_t;

typedef struct x2ap_handover_req_ack_s {
  int source_rnti;                       /* TODO: to be fixed/remove */
  int source_x2id;                       /* TODO: to be fixed/remove */
  /* TODO: this parameter has to be removed */
  int target_mod_id;
  uint8_t rrc_buffer[1024 /* arbitrary, big enough */];
  int rrc_buffer_size;

  uint32_t mme_ue_s1ap_id;
} x2ap_handover_req_ack_t;

#endif /* X2AP_MESSAGES_TYPES_H_ */
