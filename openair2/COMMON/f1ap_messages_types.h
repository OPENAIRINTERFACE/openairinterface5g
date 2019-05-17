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

#ifndef F1AP_MESSAGES_TYPES_H_
#define F1AP_MESSAGES_TYPES_H_

#include "rlc.h"

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.

#define F1AP_CU_SCTP_REQ(mSGpTR)                   (mSGpTR)->ittiMsg.f1ap_cu_sctp_req

#define F1AP_SETUP_REQ(mSGpTR)                     (mSGpTR)->ittiMsg.f1ap_setup_req
#define F1AP_SETUP_RESP(mSGpTR)                    (mSGpTR)->ittiMsg.f1ap_setup_resp
#define F1AP_SETUP_FAILURE(mSGpTR)                 (mSGpTR)->ittiMsg.f1ap_setup_failure

#define F1AP_INITIAL_UL_RRC_MESSAGE(mSGpTR)        (mSGpTR)->ittiMsg.f1ap_initial_ul_rrc_message
#define F1AP_UL_RRC_MESSAGE(mSGpTR)                (mSGpTR)->ittiMsg.f1ap_ul_rrc_message
#define F1AP_UE_CONTEXT_SETUP_REQ(mSGpTR)          (mSGpTR)->ittiMsg.f1ap_ue_context_setup_req
#define F1AP_UE_CONTEXT_RELEASE_RESP(mSGpTR)       (mSGpTR)->ittiMsg.f1ap_ue_context_release_resp
#define F1AP_UE_CONTEXT_MODIFICATION_RESP(mSGpTR)  (mSGpTR)->ittiMsg.f1ap_ue_context_modification_resp
#define F1AP_UE_CONTEXT_MODIFICATION_FAIL(mSGpTR)  (mSGpTR)->ittiMsg.f1ap_ue_context_modification_fail

#define F1AP_DL_RRC_MESSAGE(mSGpTR)                (mSGpTR)->ittiMsg.f1ap_dl_rrc_message
#define F1AP_UE_CONTEXT_RELEASE_REQ(mSGpTR)        (mSGpTR)->ittiMsg.f1ap_ue_context_release_req
#define F1AP_UE_CONTEXT_RELEASE_CMD(mSGpTR)        (mSGpTR)->ittiMsg.f1ap_ue_context_release_req
#define F1AP_UE_CONTEXT_MODIFICATION_REQ(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_ue_context_modification_req

/* Length of the transport layer address string
 * 160 bits / 8 bits by char.
 */
#define F1AP_TRANSPORT_LAYER_ADDRESS_SIZE (160 / 8)

#define F1AP_MAX_NB_CU_IP_ADDRESS 10

// Note this should be 512 from maxval in 38.473
#define F1AP_MAX_NB_CELLS 2

typedef struct f1ap_net_ip_address_s {
  unsigned ipv4:1;
  unsigned ipv6:1;
  char ipv4_address[16];
  char ipv6_address[46];
} f1ap_net_ip_address_t;

typedef struct f1ap_cu_setup_req_s {
   //
} f1ap_cu_setup_req_t;

typedef struct f1ap_setup_req_s {

  // Midhaul networking parameters

  /* Connexion id used between SCTP/F1AP */
  uint16_t cnx_id;

  /* SCTP association id */
  int32_t  assoc_id;

  /* The eNB IP address to bind */
  f1ap_net_ip_address_t CU_f1_ip_address;
  f1ap_net_ip_address_t DU_f1_ip_address;

  /* Number of SCTP streams used for a mme association */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;

  uint16_t default_sctp_stream_id;

  // F1_Setup_Req payload
  uint64_t gNB_DU_id;
  char *gNB_DU_name;

  /* The type of the cell */
  enum cell_type_e cell_type;
  
  /// number of DU cells available
  uint16_t num_cells_available; //0< num_cells_available <= 512;

  // Served Cell Information
  /* Tracking area code */
  uint16_t tac[F1AP_MAX_NB_CELLS];

  /* Mobile Country Codes
   * Mobile Network Codes
   */
  uint16_t mcc[F1AP_MAX_NB_CELLS];//[6];
  uint16_t mnc[F1AP_MAX_NB_CELLS];//[6];
  uint8_t  mnc_digit_length[F1AP_MAX_NB_CELLS];//[6];

  // NR Global Cell Id
  uint64_t nr_cellid[F1AP_MAX_NB_CELLS];
  // NR Physical Cell Ids
  uint16_t nr_pci[F1AP_MAX_NB_CELLS];
  // Number of slide support items (max 16, could be increased to as much as 1024)
  uint16_t num_ssi[F1AP_MAX_NB_CELLS];//[6];
  uint8_t sst[F1AP_MAX_NB_CELLS];//[16][6];
  uint8_t sd[F1AP_MAX_NB_CELLS];//[16][6];
  // fdd_flag = 1 means FDD, 0 means TDD
  int  fdd_flag;

  union {
    struct {
      uint32_t ul_nr_arfcn;
      uint8_t ul_scs;
      uint8_t ul_nrb;

      uint32_t dl_nr_arfcn;
      uint8_t dl_scs;
      uint8_t dl_nrb;

      uint32_t sul_active;
      uint32_t sul_nr_arfcn;
      uint8_t sul_scs;
      uint8_t sul_nrb;

      uint8_t ul_num_frequency_bands;
      uint16_t ul_nr_band[32];
      uint8_t ul_num_sul_frequency_bands;
      uint16_t ul_nr_sul_band[32];

      uint8_t dl_num_frequency_bands;
      uint16_t dl_nr_band[32];
      uint8_t dl_num_sul_frequency_bands;
      uint16_t dl_nr_sul_band[32];
    } fdd;
    struct {

      uint32_t nr_arfcn;
      uint8_t scs;
      uint8_t nrb;

      uint32_t sul_active;
      uint32_t sul_nr_arfcn;
      uint8_t sul_scs;
      uint8_t sul_nrb;

      uint8_t num_frequency_bands;
      uint16_t nr_band[32];
      uint8_t num_sul_frequency_bands;
      uint16_t nr_sul_band[32];

    } tdd;
  } nr_mode_info[F1AP_MAX_NB_CELLS];

  char *measurement_timing_information[F1AP_MAX_NB_CELLS];
  uint8_t ranac[F1AP_MAX_NB_CELLS];

  // System Information
  uint8_t *mib[F1AP_MAX_NB_CELLS];
  int     mib_length[F1AP_MAX_NB_CELLS];
  uint8_t *sib1[F1AP_MAX_NB_CELLS];
  int     sib1_length[F1AP_MAX_NB_CELLS];


} f1ap_setup_req_t;

typedef struct f1ap_setup_resp_s {
  /* Connexion id used between SCTP/F1AP */
  uint16_t cnx_id;

  /* SCTP association id */
  int32_t  assoc_id;

  /* Number of SCTP streams used for a mme association */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;

  /// string holding gNB_CU_name
  char     *gNB_CU_name;
  /// number of DU cells to activate
  uint16_t num_cells_to_activate; //0< num_cells_to_activate <= 512;
  /// mcc of DU cells
  uint16_t mcc[F1AP_MAX_NB_CELLS];
  /// mnc of DU cells
  uint16_t mnc[F1AP_MAX_NB_CELLS];
  /// mnc digit length of DU cells
  uint8_t mnc_digit_length[F1AP_MAX_NB_CELLS];
  // NR Global Cell Id
  uint64_t nr_cellid[F1AP_MAX_NB_CELLS];
  /// NRPCI
  uint16_t nrpci[F1AP_MAX_NB_CELLS];
  /// num SI messages per DU cell
  uint8_t num_SI[F1AP_MAX_NB_CELLS];
  /// SI message containers (up to 21 messages per cell)
  uint8_t *SI_container[F1AP_MAX_NB_CELLS][21];
  int      SI_container_length[F1AP_MAX_NB_CELLS][21];
} f1ap_setup_resp_t;

typedef struct f1ap_setup_failure_s {
  uint16_t cause;
  uint16_t time_to_wait;
  uint16_t criticality_diagnostics; 
} f1ap_setup_failure_t;

typedef struct f1ap_dl_rrc_message_s {

  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;
  uint32_t old_gNB_DU_ue_id;
  uint16_t rnti; 
  uint8_t  srb_id;
  uint8_t  execute_duplication;
  uint8_t *rrc_container;
  int      rrc_container_length;
  union {
    // both map 0..255 => 1..256
    uint8_t en_dc;
    uint8_t ngran;
  } RAT_frequency_priority_information;
} f1ap_dl_rrc_message_t;

typedef struct f1ap_initial_ul_rrc_message_s {
  uint32_t gNB_DU_ue_id;
  /// mcc of DU cell
  uint16_t mcc;
  /// mnc of DU cell
  uint16_t mnc;
  /// mnc digit length of DU cells
  uint8_t mnc_digit_length;
  /// nr cell id
  uint64_t nr_cellid;
  /// crnti
  uint16_t crnti;
  uint8_t *rrc_container;
  int      rrc_container_length;
  uint8_t *du2cu_rrc_container;
  int      du2cu_rrc_container_length;
} f1ap_initial_ul_rrc_message_t;

typedef struct f1ap_ul_rrc_message_s {
  uint16_t rnti;
  uint8_t  srb_id;
  uint8_t *rrc_container;
  int      rrc_container_length;
} f1ap_ul_rrc_message_t;

typedef struct f1ap_up_tnl_s {
  in_addr_t tl_address; // currently only IPv4 supported
  uint32_t  gtp_teid;
} f1ap_up_tnl_t;

typedef struct f1ap_drb_to_be_setup_s {
  uint8_t        drb_id;
  f1ap_up_tnl_t  up_ul_tnl[2];
  uint8_t        up_ul_tnl_length;
  rlc_mode_t     rlc_mode;
} f1ap_drb_to_be_setup_t;

typedef struct f1ap_ue_context_setup_req_s {
  uint32_t gNB_CU_ue_id;    // BK: need to replace by use from rnti
  uint32_t *gNB_DU_ue_id;
  uint16_t rnti; 
  // SpCell Info
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_digit_length;
  uint64_t nr_cellid;
  uint8_t servCellIndex;
  uint8_t *cellULConfigured;
  uint32_t servCellId;
  uint8_t *cu_to_du_rrc_information;
  uint8_t  cu_to_du_rrc_information_length;
  f1ap_drb_to_be_setup_t *drbs_to_be_setup; // BK: need to replace by s1ap_initial_context_setup_req
  uint8_t  drbs_to_be_setup_length;       // BK: need to replace by s1ap_initial_context_setup_req
  s1ap_initial_context_setup_req_t *s1ap_initial_context_setup_req;
   // coniatner for the rrc_eNB_generate_SecurityModeCommand message
  uint8_t *rrc_container;
  int      rrc_container_length;
} f1ap_ue_context_setup_req_t;

typedef enum F1ap_Cause_e {
  F1AP_CAUSE_NOTHING,  /* No components present */
  F1AP_CAUSE_RADIO_NETWORK,
  F1AP_CAUSE_TRANSPORT,
  F1AP_CAUSE_PROTOCOL,
  F1AP_CAUSE_MISC,
} f1ap_Cause_t;

typedef struct f1ap_ue_context_release_s {
  uint16_t      rnti;
  f1ap_Cause_t  cause;
  long          cause_value;
  uint8_t      *rrc_container;
  int           rrc_container_length;
} f1ap_ue_context_release_req_t, f1ap_ue_context_release_cmd_t,
  f1ap_ue_context_release_cplt_t;

#endif /* F1AP_MESSAGES_TYPES_H_ */
