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

#ifndef S1AP_MESSAGES_TYPES_H_
#define S1AP_MESSAGES_TYPES_H_

#include <netinet/in.h>
#include <netinet/sctp.h>
#include "common/platform_types.h"
#include "LTE_asn_constant.h"
//-------------------------------------------------------------------------------------------//
// Defines to access message fields.

#define S1AP_REGISTER_ENB_REQ(mSGpTR)           (mSGpTR)->ittiMsg.s1ap_register_enb_req

#define S1AP_REGISTER_ENB_CNF(mSGpTR)           (mSGpTR)->ittiMsg.s1ap_register_enb_cnf
#define S1AP_DEREGISTERED_ENB_IND(mSGpTR)       (mSGpTR)->ittiMsg.s1ap_deregistered_enb_ind

#define S1AP_NAS_FIRST_REQ(mSGpTR)              (mSGpTR)->ittiMsg.s1ap_nas_first_req
#define S1AP_UPLINK_NAS(mSGpTR)                 (mSGpTR)->ittiMsg.s1ap_uplink_nas
#define S1AP_UE_CAPABILITIES_IND(mSGpTR)        (mSGpTR)->ittiMsg.s1ap_ue_cap_info_ind
#define S1AP_INITIAL_CONTEXT_SETUP_RESP(mSGpTR) (mSGpTR)->ittiMsg.s1ap_initial_context_setup_resp
#define S1AP_INITIAL_CONTEXT_SETUP_FAIL(mSGpTR) (mSGpTR)->ittiMsg.s1ap_initial_context_setup_fail
#define S1AP_UE_CONTEXT_RELEASE_RESP(mSGpTR)    (mSGpTR)->ittiMsg.s1ap_ue_release_resp
#define S1AP_NAS_NON_DELIVERY_IND(mSGpTR)       (mSGpTR)->ittiMsg.s1ap_nas_non_delivery_ind
#define S1AP_UE_CTXT_MODIFICATION_RESP(mSGpTR)  (mSGpTR)->ittiMsg.s1ap_ue_ctxt_modification_resp
#define S1AP_UE_CTXT_MODIFICATION_FAIL(mSGpTR)  (mSGpTR)->ittiMsg.s1ap_ue_ctxt_modification_fail
#define S1AP_E_RAB_SETUP_RESP(mSGpTR)           (mSGpTR)->ittiMsg.s1ap_e_rab_setup_resp
#define S1AP_E_RAB_SETUP_FAIL(mSGpTR)           (mSGpTR)->ittiMsg.s1ap_e_rab_setup_req_fail
#define S1AP_E_RAB_MODIFY_RESP(mSGpTR)           (mSGpTR)->ittiMsg.s1ap_e_rab_modify_resp
#define S1AP_PATH_SWITCH_REQ(mSGpTR)            (mSGpTR)->ittiMsg.s1ap_path_switch_req
#define S1AP_PATH_SWITCH_REQ_ACK(mSGpTR)        (mSGpTR)->ittiMsg.s1ap_path_switch_req_ack
#define S1AP_E_RAB_MODIFICATION_IND(mSGpTR)     (mSGpTR)->ittiMsg.s1ap_e_rab_modification_ind

#define S1AP_DOWNLINK_NAS(mSGpTR)               (mSGpTR)->ittiMsg.s1ap_downlink_nas
#define S1AP_INITIAL_CONTEXT_SETUP_REQ(mSGpTR)  (mSGpTR)->ittiMsg.s1ap_initial_context_setup_req
#define S1AP_UE_CTXT_MODIFICATION_REQ(mSGpTR)   (mSGpTR)->ittiMsg.s1ap_ue_ctxt_modification_req
#define S1AP_UE_CONTEXT_RELEASE_COMMAND(mSGpTR) (mSGpTR)->ittiMsg.s1ap_ue_release_command
#define S1AP_UE_CONTEXT_RELEASE_COMPLETE(mSGpTR) (mSGpTR)->ittiMsg.s1ap_ue_release_complete
#define S1AP_E_RAB_SETUP_REQ(mSGpTR)              (mSGpTR)->ittiMsg.s1ap_e_rab_setup_req
#define S1AP_E_RAB_MODIFY_REQ(mSGpTR)              (mSGpTR)->ittiMsg.s1ap_e_rab_modify_req
#define S1AP_PAGING_IND(mSGpTR)                 (mSGpTR)->ittiMsg.s1ap_paging_ind

#define S1AP_UE_CONTEXT_RELEASE_REQ(mSGpTR)     (mSGpTR)->ittiMsg.s1ap_ue_release_req
#define S1AP_E_RAB_RELEASE_COMMAND(mSGpTR)      (mSGpTR)->ittiMsg.s1ap_e_rab_release_command
#define S1AP_E_RAB_RELEASE_RESPONSE(mSGpTR)     (mSGpTR)->ittiMsg.s1ap_e_rab_release_resp

//-------------------------------------------------------------------------------------------//
/* Maximum number of e-rabs to be setup/deleted in a single message.
 * Even if only one bearer will be modified by message.
 */
#define S1AP_MAX_E_RAB  (LTE_maxDRB + 3)

/* Length of the transport layer address string
 * 160 bits / 8 bits by char.
 */
#define S1AP_TRANSPORT_LAYER_ADDRESS_SIZE (160 / 8)

#define S1AP_MAX_NB_MME_IP_ADDRESS 10
#define S1AP_IMSI_LENGTH           16

/* Security key length used within eNB
 * Even if only 16 bytes will be effectively used,
 * the key length is 32 bytes (256 bits)
 */
#define SECURITY_KEY_LENGTH 32
typedef enum cell_type_e {
  CELL_MACRO_ENB,
  CELL_HOME_ENB,
  CELL_MACRO_GNB
} cell_type_t;

typedef enum paging_drx_e {
  PAGING_DRX_32  = 0x0,
  PAGING_DRX_64  = 0x1,
  PAGING_DRX_128 = 0x2,
  PAGING_DRX_256 = 0x3
} paging_drx_t;

/* Lower value codepoint
 * indicates higher priority.
 */
typedef enum paging_priority_s {
  PAGING_PRIO_LEVEL1  = 0,
  PAGING_PRIO_LEVEL2  = 1,
  PAGING_PRIO_LEVEL3  = 2,
  PAGING_PRIO_LEVEL4  = 3,
  PAGING_PRIO_LEVEL5  = 4,
  PAGING_PRIO_LEVEL6  = 5,
  PAGING_PRIO_LEVEL7  = 6,
  PAGING_PRIO_LEVEL8  = 7
} paging_priority_t;

typedef enum cn_domain_s {
  CN_DOMAIN_PS = 1,
  CN_DOMAIN_CS = 2
} cn_domain_t;

typedef uint64_t bitrate_t;

typedef struct ambr_s {
  bitrate_t br_ul;
  bitrate_t br_dl;
} ambr_t;

typedef enum priority_level_s {
  PRIORITY_LEVEL_SPARE       = 0,
  PRIORITY_LEVEL_HIGHEST     = 1,
  PRIORITY_LEVEL_LOWEST      = 14,
  PRIORITY_LEVEL_NO_PRIORITY = 15
} priority_level_t;

typedef enum pre_emp_capability_e {
  PRE_EMPTION_CAPABILITY_ENABLED  = 0,
  PRE_EMPTION_CAPABILITY_DISABLED = 1,
  PRE_EMPTION_CAPABILITY_MAX,
} pre_emp_capability_t;

typedef enum pre_emp_vulnerability_e {
  PRE_EMPTION_VULNERABILITY_ENABLED  = 0,
  PRE_EMPTION_VULNERABILITY_DISABLED = 1,
  PRE_EMPTION_VULNERABILITY_MAX,
} pre_emp_vulnerability_t;

typedef struct allocation_retention_priority_s {
  priority_level_t        priority_level;
  pre_emp_capability_t    pre_emp_capability;
  pre_emp_vulnerability_t pre_emp_vulnerability;
} allocation_retention_priority_t;

typedef struct security_capabilities_s {
  uint16_t encryption_algorithms;
  uint16_t integrity_algorithms;
} security_capabilities_t;

typedef struct nr_security_capabilities_s {
  uint16_t encryption_algorithms;
  uint16_t integrity_algorithms;
} nr_security_capabilities_t;

/* Provides the establishment cause for the RRC connection request as provided
 * by the upper layers. W.r.t. the cause value names: highPriorityAccess
 * concerns AC11..AC15, ‘mt’ stands for ‘Mobile Terminating’ and ‘mo’ for
 * 'Mobile Originating'. Defined in TS 36.331.
 */
typedef enum rrc_establishment_cause_e {
  RRC_CAUSE_EMERGENCY             = 0x0,
  RRC_CAUSE_HIGH_PRIO_ACCESS      = 0x1,
  RRC_CAUSE_MT_ACCESS             = 0x2,
  RRC_CAUSE_MO_SIGNALLING         = 0x3,
  RRC_CAUSE_MO_DATA               = 0x4,
#if defined(UPDATE_RELEASE_10)
  RRC_CAUSE_DELAY_TOLERANT_ACCESS = 0x5,
#endif
  RRC_CAUSE_LAST
} rrc_establishment_cause_t;

typedef struct s1ap_gummei_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_len;
  uint8_t  mme_code;
  uint16_t mme_group_id;
} s1ap_gummei_t;

typedef struct s1ap_imsi_s {
  uint8_t  buffer[S1AP_IMSI_LENGTH];
  uint8_t  length;
} s1ap_imsi_t;

typedef struct s_tmsi_s {
  uint8_t  mme_code;
  uint32_t m_tmsi;
} s_tmsi_t;

typedef enum ue_paging_identity_presenceMask_e {
  UE_PAGING_IDENTITY_NONE   = 0,
  UE_PAGING_IDENTITY_imsi   = (1 << 1),
  UE_PAGING_IDENTITY_s_tmsi = (1 << 2),
} ue_paging_identity_presenceMask_t;

typedef struct ue_paging_identity_s {
  ue_paging_identity_presenceMask_t presenceMask;
  union {
    s1ap_imsi_t  imsi;
    s_tmsi_t s_tmsi;
  } choice;
} ue_paging_identity_t;

typedef enum ue_identities_presenceMask_e {
  UE_IDENTITIES_NONE   = 0,
  UE_IDENTITIES_s_tmsi = 1 << 1,
  UE_IDENTITIES_gummei = 1 << 2,
} ue_identities_presenceMask_t;

typedef struct ue_identity_s {
  ue_identities_presenceMask_t presenceMask;
  s_tmsi_t s_tmsi;
  s1ap_gummei_t gummei;
} ue_identity_t;

typedef struct nas_pdu_s {
  /* Octet string data */
  uint8_t  *buffer;
  /* Length of the octet string */
  uint32_t  length;
} nas_pdu_t, ue_radio_cap_t;

typedef struct transport_layer_addr_s {
  /* Length of the transport layer address buffer in bits. S1AP layer received a
   * bit string<1..160> containing one of the following addresses: ipv4,
   * ipv6, or ipv4 and ipv6. The layer doesn't interpret the buffer but
   * silently forward it to S1-U.
   */
  uint8_t length;
  uint8_t buffer[20]; // in network byte order
} transport_layer_addr_t;

#define TRANSPORT_LAYER_ADDR_COPY(dEST,sOURCE)        \
  do {                                                \
      AssertFatal(sOURCE.len <= 20);                  \
      memcpy(dEST.buffer, sOURCE.buffer, sOURCE.len); \
      dEST.length = sOURCE.length;                    \
  } while (0)

typedef struct e_rab_level_qos_parameter_s {
  uint8_t qci;

  allocation_retention_priority_t allocation_retention_priority;
} e_rab_level_qos_parameter_t;

typedef struct e_rab_s {
  /* Unique e_rab_id for the UE. */
  uint8_t                     e_rab_id;
  /* Quality of service for this e_rab */
  e_rab_level_qos_parameter_t qos;
  /* The NAS PDU should be forwarded by the RRC layer to the NAS layer */
  nas_pdu_t                   nas_pdu;
  /* The transport layer address for the IP packets */
  transport_layer_addr_t      sgw_addr;
  /* S-GW Tunnel endpoint identifier */
  uint32_t                    gtp_teid;
} e_rab_t;

typedef struct e_rab_setup_s {
  /* Unique e_rab_id for the UE. */
  uint8_t e_rab_id;

  /* The transport layer address for the IP packets */
  transport_layer_addr_t eNB_addr;

  /* S-GW Tunnel endpoint identifier */
  uint32_t gtp_teid;
} e_rab_setup_t;

typedef struct e_rab_tobe_added_s {
  /* Unique e_rab_id for the UE. */
  uint8_t e_rab_id;

  /* Unique drb_ID for the UE. */
  uint8_t drb_ID;

  /* The transport layer address for the IP packets */
  transport_layer_addr_t sgw_addr;

  /* S-GW Tunnel endpoint identifier */
  uint32_t gtp_teid;
} e_rab_tobe_added_t;

typedef struct e_rab_admitted_tobe_added_s {
  /* Unique e_rab_id for the UE. */
  uint8_t e_rab_id;

  /* Unique drb_ID for the UE. */
  uint8_t drb_ID;

  /* The transport layer address for the IP packets */
  transport_layer_addr_t gnb_addr;

  /* S-GW Tunnel endpoint identifier */
  uint32_t gtp_teid;
} e_rab_admitted_tobe_added_t;



typedef struct e_rab_tobeswitched_s {
  /* Unique e_rab_id for the UE. */
  uint8_t e_rab_id;

  /* The transport layer address for the IP packets */
  transport_layer_addr_t sgw_addr;

  /* S-GW Tunnel endpoint identifier */
  uint32_t gtp_teid;
} e_rab_tobeswitched_t;

typedef struct e_rab_modify_s {
  /* Unique e_rab_id for the UE. */
  uint8_t e_rab_id;
} e_rab_modify_t;

typedef enum S1ap_Cause_e {
  S1AP_CAUSE_NOTHING,  /* No components present */
  S1AP_CAUSE_RADIO_NETWORK,
  S1AP_CAUSE_TRANSPORT,
  S1AP_CAUSE_NAS,
  S1AP_CAUSE_PROTOCOL,
  S1AP_CAUSE_MISC,
  /* Extensions may appear below */

} s1ap_Cause_t;

typedef struct e_rab_failed_s {
  /* Unique e_rab_id for the UE. */
  uint8_t e_rab_id;
  /* Cause of the failure */
  //     cause_t cause;
  s1ap_Cause_t cause;
  uint8_t cause_value;
} e_rab_failed_t;

typedef enum s1ap_ue_ctxt_modification_present_s {
  S1AP_UE_CONTEXT_MODIFICATION_SECURITY_KEY = (1 << 0),
  S1AP_UE_CONTEXT_MODIFICATION_UE_AMBR      = (1 << 1),
  S1AP_UE_CONTEXT_MODIFICATION_UE_SECU_CAP  = (1 << 2),
} s1ap_ue_ctxt_modification_present_t;

typedef enum s1ap_paging_ind_present_s {
  S1AP_PAGING_IND_PAGING_DRX      = (1 << 0),
  S1AP_PAGING_IND_PAGING_PRIORITY = (1 << 1),
} s1ap_paging_ind_present_t;

//-------------------------------------------------------------------------------------------//
// eNB application layer -> S1AP messages
typedef struct s1ap_register_enb_req_s {
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

#define PLMN_LIST_MAX_SIZE 6
  /* Mobile Country Code
   * Mobile Network Code
   */
  uint16_t mcc[PLMN_LIST_MAX_SIZE];
  uint16_t mnc[PLMN_LIST_MAX_SIZE];
  uint8_t  mnc_digit_length[PLMN_LIST_MAX_SIZE];
  uint8_t  num_plmn;

  /* Default Paging DRX of the eNB as defined in TS 36.304 */
  paging_drx_t default_drx;

  /* The eNB IP address to bind */
  net_ip_address_t enb_ip_address;

  /* Nb of MME to connect to */
  uint8_t          nb_mme;
  /* List of MME to connect to */
  net_ip_address_t mme_ip_address[S1AP_MAX_NB_MME_IP_ADDRESS];
  uint16_t         mme_port[S1AP_MAX_NB_MME_IP_ADDRESS];
  uint8_t          broadcast_plmn_num[S1AP_MAX_NB_MME_IP_ADDRESS];
  uint8_t          broadcast_plmn_index[S1AP_MAX_NB_MME_IP_ADDRESS][PLMN_LIST_MAX_SIZE];

  /* Number of SCTP streams used for a mme association */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;
  uint16_t s1_setuprsp_wait_timer;
  uint16_t s1_setupreq_wait_timer;
  uint16_t s1_setupreq_count;
  uint16_t sctp_req_timer;
  uint16_t sctp_req_count;
} s1ap_register_enb_req_t;

//-------------------------------------------------------------------------------------------//
// S1AP -> eNB application layer messages
typedef struct s1ap_register_enb_cnf_s {
  /* Nb of MME connected */
  uint8_t          nb_mme;
} s1ap_register_enb_cnf_t;

typedef struct s1ap_deregistered_enb_ind_s {
  /* Nb of MME connected */
  uint8_t          nb_mme;
} s1ap_deregistered_enb_ind_t;

//-------------------------------------------------------------------------------------------//
// RRC -> S1AP messages

/* The NAS First Req is the first message exchanged between RRC and S1AP
 * for an UE.
 * The rnti uniquely identifies an UE within a cell. Later the enb_ue_s1ap_id
 * will be the unique identifier used between RRC and S1AP.
 */
typedef struct s1ap_nas_first_req_s {
  /* UE id for initial connection to S1AP */
  uint16_t ue_initial_id;

  /* the chosen PLMN identity as index, see TS 36.331 6.2.2 RRC Connection
   * Setup Complete. This index here is zero-based, unlike the standard! */
  int selected_plmn_identity;

  /* Establishment cause as sent by UE */
  rrc_establishment_cause_t establishment_cause;

  /* NAS PDU */
  nas_pdu_t nas_pdu;

  /* If this flag is set S1AP layer is expecting the GUMMEI. If = 0,
   * the temporary s-tmsi is used.
   */
  ue_identity_t ue_identity;
} s1ap_nas_first_req_t;

typedef struct s1ap_uplink_nas_s {
  /* Unique UE identifier within an eNB */
  unsigned eNB_ue_s1ap_id:24;

  /* NAS pdu */
  nas_pdu_t nas_pdu;
} s1ap_uplink_nas_t;

typedef struct s1ap_ue_cap_info_ind_s {
  unsigned  eNB_ue_s1ap_id:24;
  ue_radio_cap_t ue_radio_cap;
} s1ap_ue_cap_info_ind_t;

typedef struct s1ap_initial_context_setup_resp_s {
  unsigned  eNB_ue_s1ap_id:24;

  /* Number of e_rab setup-ed in the list */
  uint8_t       nb_of_e_rabs;
  /* list of e_rab setup-ed by RRC layers */
  e_rab_setup_t e_rabs[S1AP_MAX_E_RAB];

  /* Number of e_rab failed to be setup in list */
  uint8_t        nb_of_e_rabs_failed;
  /* list of e_rabs that failed to be setup */
  e_rab_failed_t e_rabs_failed[S1AP_MAX_E_RAB];
} s1ap_initial_context_setup_resp_t;

typedef struct s1ap_initial_context_setup_fail_s {
  unsigned  eNB_ue_s1ap_id:24;

  /* TODO add cause */
} s1ap_initial_context_setup_fail_t, s1ap_ue_ctxt_modification_fail_t, s1ap_e_rab_setup_req_fail_t;

typedef struct s1ap_nas_non_delivery_ind_s {
  unsigned  eNB_ue_s1ap_id:24;
  nas_pdu_t nas_pdu;
  /* TODO: add cause */
} s1ap_nas_non_delivery_ind_t;

typedef struct s1ap_ue_ctxt_modification_req_s {
  unsigned  eNB_ue_s1ap_id:24;

  /* Bit-mask of possible present parameters */
  s1ap_ue_ctxt_modification_present_t present;

  /* Following fields are optionnaly present */

  /* Security key */
  uint8_t security_key[SECURITY_KEY_LENGTH];

  /* UE aggregate maximum bitrate */
  ambr_t ue_ambr;

  /* Security capabilities */
  security_capabilities_t security_capabilities;
} s1ap_ue_ctxt_modification_req_t;

typedef struct s1ap_ue_ctxt_modification_resp_s {
  unsigned  eNB_ue_s1ap_id:24;
} s1ap_ue_ctxt_modification_resp_t;

typedef struct s1ap_ue_release_complete_s {

  unsigned eNB_ue_s1ap_id:24;

} s1ap_ue_release_complete_t;

//-------------------------------------------------------------------------------------------//
// S1AP -> RRC messages
typedef struct s1ap_downlink_nas_s {
  /* UE id for initial connection to S1AP */
  uint16_t ue_initial_id;

  /* Unique UE identifier within an eNB */
  unsigned eNB_ue_s1ap_id:24;

  /* NAS pdu */
  nas_pdu_t nas_pdu;
} s1ap_downlink_nas_t;


typedef struct s1ap_initial_context_setup_req_s {
  /* UE id for initial connection to S1AP */
  uint16_t ue_initial_id;

  /* eNB ue s1ap id as initialized by S1AP layer */
  unsigned eNB_ue_s1ap_id:24;

  uint32_t mme_ue_s1ap_id;

  /* UE aggregate maximum bitrate */
  ambr_t ue_ambr;

  /* Security algorithms */
  security_capabilities_t security_capabilities;

  /* Security key */
  uint8_t security_key[SECURITY_KEY_LENGTH];

  /* Number of e_rab to be setup in the list */
  uint8_t  nb_of_e_rabs;
  /* list of e_rab to be setup by RRC layers */
  e_rab_t  e_rab_param[S1AP_MAX_E_RAB];

  /* NR Security algorithms (if any, set to 0 if not present) */
  nr_security_capabilities_t nr_security_capabilities;
} s1ap_initial_context_setup_req_t;

typedef struct tai_plmn_identity_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_digit_length;
} plmn_identity_t;

typedef struct s1ap_paging_ind_s {
  /* UE identity index value.
   * Specified in 3GPP TS 36.304
   */
  unsigned ue_index_value:10;

  /* UE paging identity */
  ue_paging_identity_t ue_paging_identity;

  /* Indicates origin of paging */
  cn_domain_t cn_domain;

  /* PLMN_identity in TAI of Paging*/
  plmn_identity_t plmn_identity[256];

  /* TAC in TAIList of Paging*/
  int16_t tac[256];

  /* size of TAIList*/
  int16_t tai_size;

  /* Optional fields */
  paging_drx_t paging_drx;

  paging_priority_t paging_priority;
} s1ap_paging_ind_t;

typedef struct s1ap_e_rab_setup_req_s {
  /* UE id for initial connection to S1AP */
  uint16_t ue_initial_id;

  /* MME UE id  */
  uint32_t mme_ue_s1ap_id;

  /* eNB ue s1ap id as initialized by S1AP layer */
  unsigned eNB_ue_s1ap_id:24;

  /* Number of e_rab to be setup in the list */
  uint8_t nb_e_rabs_tosetup;

  /* E RAB setup request */
  e_rab_t e_rab_setup_params[S1AP_MAX_E_RAB];

} s1ap_e_rab_setup_req_t;

typedef struct s1ap_e_rab_setup_resp_s {
  unsigned  eNB_ue_s1ap_id:24;

  /* Number of e_rab setup-ed in the list */
  uint8_t       nb_of_e_rabs;
  /* list of e_rab setup-ed by RRC layers */
  e_rab_setup_t e_rabs[S1AP_MAX_E_RAB];

  /* Number of e_rab failed to be setup in list */
  uint8_t        nb_of_e_rabs_failed;
  /* list of e_rabs that failed to be setup */
  e_rab_failed_t e_rabs_failed[S1AP_MAX_E_RAB];
} s1ap_e_rab_setup_resp_t;

typedef struct s1ap_path_switch_req_s {

  unsigned  eNB_ue_s1ap_id:24;

  /* Number of e_rab setup-ed in the list */
  uint8_t       nb_of_e_rabs;

  /* list of e_rab setup-ed by RRC layers */
  e_rab_setup_t e_rabs_tobeswitched[S1AP_MAX_E_RAB];

  /* MME UE id  */
  uint32_t mme_ue_s1ap_id;

  s1ap_gummei_t ue_gummei;

  uint16_t ue_initial_id;

   /* Security algorithms */
  security_capabilities_t security_capabilities;

} s1ap_path_switch_req_t;

typedef struct s1ap_path_switch_req_ack_s {

  /* UE id for initial connection to S1AP */
  uint16_t ue_initial_id;

  unsigned  eNB_ue_s1ap_id:24;

  /* MME UE id  */
  uint32_t mme_ue_s1ap_id;

  /* UE aggregate maximum bitrate */
  ambr_t ue_ambr;

  /* Number of e_rab setup-ed in the list */
  uint8_t       nb_e_rabs_tobeswitched;

  /* list of e_rab to be switched by RRC layers */
  e_rab_tobeswitched_t e_rabs_tobeswitched[S1AP_MAX_E_RAB];

  /* Number of e_rabs to be released by RRC */
  uint8_t        nb_e_rabs_tobereleased;

  /* list of e_rabs to be released */
  e_rab_failed_t e_rabs_tobereleased[S1AP_MAX_E_RAB];

  /* Security key */
  int     next_hop_chain_count;
  uint8_t next_security_key[SECURITY_KEY_LENGTH];

} s1ap_path_switch_req_ack_t;

typedef struct s1ap_e_rab_modification_ind_s {

  unsigned  eNB_ue_s1ap_id:24;

  /* MME UE id  */
    uint32_t mme_ue_s1ap_id;

  /* Number of e_rab setup-ed in the list */
  uint8_t       nb_of_e_rabs_tobemodified;

  uint8_t       nb_of_e_rabs_nottobemodified;

  /* list of e_rab setup-ed by RRC layers */
  e_rab_setup_t e_rabs_tobemodified[S1AP_MAX_E_RAB];

  e_rab_setup_t e_rabs_nottobemodified[S1AP_MAX_E_RAB];

  uint16_t ue_initial_id;

} s1ap_e_rab_modification_ind_t;

// S1AP --> RRC messages
typedef struct s1ap_ue_release_command_s {

  unsigned eNB_ue_s1ap_id:24;

} s1ap_ue_release_command_t;


//-------------------------------------------------------------------------------------------//
// S1AP <-- RRC messages
typedef struct s1ap_ue_release_req_s {
  unsigned      eNB_ue_s1ap_id:24;
  s1ap_Cause_t  cause;
  long          cause_value;
} s1ap_ue_release_req_t, s1ap_ue_release_resp_t;

typedef struct s1ap_e_rab_modify_req_s {
  /* UE id for initial connection to S1AP */
  uint16_t ue_initial_id;

  /* MME UE id  */
  uint32_t mme_ue_s1ap_id;

  /* eNB ue s1ap id as initialized by S1AP layer */
  unsigned eNB_ue_s1ap_id:24;

  /* Number of e_rab to be modify in the list */
  uint8_t nb_e_rabs_tomodify;

  /* E RAB modify request */
  e_rab_t e_rab_modify_params[S1AP_MAX_E_RAB];
} s1ap_e_rab_modify_req_t;

typedef struct s1ap_e_rab_modify_resp_s {
  unsigned  eNB_ue_s1ap_id:24;

  /* Number of e_rab modify-ed in the list */
  uint8_t       nb_of_e_rabs;
  /* list of e_rab modify-ed by RRC layers */
  e_rab_modify_t e_rabs[S1AP_MAX_E_RAB];

  /* Number of e_rab failed to be modify in list */
  uint8_t        nb_of_e_rabs_failed;
  /* list of e_rabs that failed to be modify */
  e_rab_failed_t e_rabs_failed[S1AP_MAX_E_RAB];
} s1ap_e_rab_modify_resp_t;

typedef struct e_rab_release_s {
  /* Unique e_rab_id for the UE. */
  uint8_t                     e_rab_id;
} e_rab_release_t;

typedef struct s1ap_e_rab_release_command_s {
  /* MME UE id  */
  uint32_t mme_ue_s1ap_id;

  /* eNB ue s1ap id as initialized by S1AP layer */
  unsigned eNB_ue_s1ap_id:24;

  /* The NAS PDU should be forwarded by the RRC layer to the NAS layer */
  nas_pdu_t                   nas_pdu;

  /* Number of e_rab to be released in the list */
  uint8_t nb_e_rabs_torelease;

  /* E RAB release command */
  e_rab_release_t e_rab_release_params[S1AP_MAX_E_RAB];

} s1ap_e_rab_release_command_t;

typedef struct s1ap_e_rab_release_resp_s {
  /* MME UE id  */
  uint32_t mme_ue_s1ap_id;

  /* eNB ue s1ap id as initialized by S1AP layer */
  unsigned eNB_ue_s1ap_id:24;

  /* Number of e_rab released in the list */
  uint8_t       nb_of_e_rabs_released;

  /* list of e_rabs released */
  e_rab_release_t e_rab_release[S1AP_MAX_E_RAB];

  /* Number of e_rab failed to be released in list */
  uint8_t        nb_of_e_rabs_failed;
  /* list of e_rabs that failed to be released */
  e_rab_failed_t e_rabs_failed[S1AP_MAX_E_RAB];

} s1ap_e_rab_release_resp_t;

#endif /* S1AP_MESSAGES_TYPES_H_ */
