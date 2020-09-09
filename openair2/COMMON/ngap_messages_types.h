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

#ifndef NGAP_MESSAGES_TYPES_H_
#define NGAP_MESSAGES_TYPES_H_

#include "LTE_asn_constant.h"
//-------------------------------------------------------------------------------------------//
// Defines to access message fields.

#define NGAP_REGISTER_GNB_REQ(mSGpTR)           (mSGpTR)->ittiMsg.ngap_register_gnb_req

#define NGAP_REGISTER_GNB_CNF(mSGpTR)           (mSGpTR)->ittiMsg.ngap_register_gnb_cnf
#define NGAP_DEREGISTERED_GNB_IND(mSGpTR)       (mSGpTR)->ittiMsg.ngap_deregistered_gnb_ind

#define NGAP_NAS_FIRST_REQ(mSGpTR)              (mSGpTR)->ittiMsg.ngap_nas_first_req
#define NGAP_UPLINK_NAS(mSGpTR)                 (mSGpTR)->ittiMsg.ngap_uplink_nas
#define NGAP_UE_CAPABILITIES_IND(mSGpTR)        (mSGpTR)->ittiMsg.ngap_ue_cap_info_ind
#define NGAP_INITIAL_CONTEXT_SETUP_RESP(mSGpTR) (mSGpTR)->ittiMsg.ngap_initial_context_setup_resp
#define NGAP_INITIAL_CONTEXT_SETUP_FAIL(mSGpTR) (mSGpTR)->ittiMsg.ngap_initial_context_setup_fail
#define NGAP_UE_CONTEXT_RELEASE_RESP(mSGpTR)    (mSGpTR)->ittiMsg.ngap_ue_release_resp
#define NGAP_NAS_NON_DELIVERY_IND(mSGpTR)       (mSGpTR)->ittiMsg.ngap_nas_non_delivery_ind
#define NGAP_UE_CTXT_MODIFICATION_RESP(mSGpTR)  (mSGpTR)->ittiMsg.ngap_ue_ctxt_modification_resp
#define NGAP_UE_CTXT_MODIFICATION_FAIL(mSGpTR)  (mSGpTR)->ittiMsg.ngap_ue_ctxt_modification_fail
#define NGAP_PDUSESSION_SETUP_RESP(mSGpTR)           (mSGpTR)->ittiMsg.ngap_pdusession_setup_resp
#define NGAP_PDUSESSION_SETUP_FAIL(mSGpTR)           (mSGpTR)->ittiMsg.ngap_pdusession_setup_req_fail
#define NGAP_PDUSESSION_MODIFY_RESP(mSGpTR)           (mSGpTR)->ittiMsg.ngap_pdusession_modify_resp
#define NGAP_PATH_SWITCH_REQ(mSGpTR)            (mSGpTR)->ittiMsg.ngap_path_switch_req
#define NGAP_PATH_SWITCH_REQ_ACK(mSGpTR)        (mSGpTR)->ittiMsg.ngap_path_switch_req_ack
#define NGAP_PDUSESSION_MODIFICATION_IND(mSGpTR)     (mSGpTR)->ittiMsg.ngap_pdusession_modification_ind

#define NGAP_DOWNLINK_NAS(mSGpTR)               (mSGpTR)->ittiMsg.ngap_downlink_nas
#define NGAP_INITIAL_CONTEXT_SETUP_REQ(mSGpTR)  (mSGpTR)->ittiMsg.ngap_initial_context_setup_req
#define NGAP_UE_CTXT_MODIFICATION_REQ(mSGpTR)   (mSGpTR)->ittiMsg.ngap_ue_ctxt_modification_req
#define NGAP_UE_CONTEXT_RELEASE_COMMAND(mSGpTR) (mSGpTR)->ittiMsg.ngap_ue_release_command
#define NGAP_UE_CONTEXT_RELEASE_COMPLETE(mSGpTR) (mSGpTR)->ittiMsg.ngap_ue_release_complete
#define NGAP_PDUSESSION_SETUP_REQ(mSGpTR)              (mSGpTR)->ittiMsg.ngap_pdusession_setup_req
#define NGAP_PDUSESSION_MODIFY_REQ(mSGpTR)              (mSGpTR)->ittiMsg.ngap_pdusession_modify_req
#define NGAP_PAGING_IND(mSGpTR)                 (mSGpTR)->ittiMsg.ngap_paging_ind

#define NGAP_UE_CONTEXT_RELEASE_REQ(mSGpTR)     (mSGpTR)->ittiMsg.ngap_ue_release_req
#define NGAP_PDUSESSION_RELEASE_COMMAND(mSGpTR)      (mSGpTR)->ittiMsg.ngap_pdusession_release_command
#define NGAP_PDUSESSION_RELEASE_RESPONSE(mSGpTR)     (mSGpTR)->ittiMsg.ngap_pdusession_release_resp

//-------------------------------------------------------------------------------------------//
/* Maximum number of e-rabs to be setup/deleted in a single message.
 * Even if only one bearer will be modified by message.
 */
#define NGAP_MAX_PDUSESSION  (LTE_maxDRB + 3)

/* Length of the transport layer address string
 * 160 bits / 8 bits by char.
 */
#define NGAP_TRANSPORT_LAYER_ADDRESS_SIZE (160 / 8)

#define NGAP_MAX_NB_AMF_IP_ADDRESS 10
#define NGAP_IMSI_LENGTH           16

#define QOSFLOW_MAX_VALUE           64;

/* Security key length used within gNB
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

typedef struct net_ip_address_s {
  unsigned ipv4:1;
  unsigned ipv6:1;
  char ipv4_address[16];
  char ipv6_address[46];
} net_ip_address_t;

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

typedef struct nr_security_capabilities_s {
  uint16_t nRencryption_algorithms;
  uint16_t nRintegrity_algorithms;
  uint16_t eUTRAencryption_algorithms;
  uint16_t eUTRAintegrity_algorithms;
} nr_security_capabilities_t;

/* Provides the establishment cause for the RRC connection request as provided
 * by the upper layers. W.r.t. the cause value names: highPriorityAccess
 * concerns AC11..AC15, ‘mt Estands for ‘Mobile Terminating Eand ‘mo Efor
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

typedef struct ngap_guami_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_len;
  uint8_t  amf_region_id;
  uint16_t amf_set_id;
  uint8_t  amf_pointer;
} ngap_guami_t;

typedef struct ngap_allowed_NSSAI_s{
  uint8_t sST;
  uint8_t sD_flag;
  uint8_t sD[3];
}ngap_allowed_NSSAI_t;


typedef struct ngap_imsi_s {
  uint8_t  buffer[NGAP_IMSI_LENGTH];
  uint8_t  length;
} ngap_imsi_t;

typedef struct s_tmsi_s {
  uint8_t  amf_code;
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
    ngap_imsi_t  imsi;
    s_tmsi_t s_tmsi;
  } choice;
} ue_paging_identity_t;

typedef enum ue_identities_presenceMask_e {
  UE_IDENTITIES_NONE   = 0,
  UE_IDENTITIES_s_tmsi = 1 << 1,
  UE_IDENTITIES_guami = 1 << 2,
} ue_identities_presenceMask_t;

typedef struct nrue_identity_s {
  ue_identities_presenceMask_t presenceMask;
  s_tmsi_t s_tmsi;
  ngap_guami_t guami;
} nrue_identity_t;

typedef struct nas_pdu_s {
  /* Octet string data */
  uint8_t  *buffer;
  /* Length of the octet string */
  uint32_t  length;
} nas_pdu_t, ue_radio_cap_t;

typedef enum pdu_session_type_e {
  PDUSessionType_ipv4 = 0,
  PDUSessionType_ipv6 = 1,
  PDUSessionType_ipv4v6 = 2,
  PDUSessionType_ethernet = 3,
  PDUSessionType_unstructured = 4
}pdu_session_type_t;

typedef struct transport_layer_addr_s {
  /* Length of the transport layer address buffer in bits. NGAP layer received a
   * bit string<1..160> containing one of the following addresses: ipv4,
   * ipv6, or ipv4 and ipv6. The layer doesn't interpret the buffer but
   * silently forward it to NG-U.
   */
  uint8_t pdu_session_type;
  uint8_t length;
  uint8_t buffer[20]; // in network byte order
} transport_layer_addr_t;

#define TRANSPORT_LAYER_ADDR_COPY(dEST,sOURCE)        \
  do {                                                \
      AssertFatal(sOURCE.len <= 20);                  \
      memcpy(dEST.buffer, sOURCE.buffer, sOURCE.len); \
      dEST.length = sOURCE.length;                    \
  } while (0)

typedef struct pdusession_level_qos_parameter_s {
  uint8_t qci;

  allocation_retention_priority_t allocation_retention_priority;
} pdusession_level_qos_parameter_t;

typedef struct pdusession_s {
  /* Unique pdusession_id for the UE. */
  uint8_t                     pdusession_id;

  uint8_t                     nb_qos;
  /* Quality of service for this pdusession */
  pdusession_level_qos_parameter_t qos[QOSFLOW_MAX_VALUE];
  /* The NAS PDU should be forwarded by the RRC layer to the NAS layer */
  nas_pdu_t                   nas_pdu;
  /* The transport layer address for the IP packets */
  transport_layer_addr_t      upf_addr;
  /* S-GW Tunnel endpoint identifier */
  uint32_t                    gtp_teid;
} pdusession_t;

typedef struct pdusession_setup_s {
  /* Unique pdusession_id for the UE. */
  uint8_t pdusession_id;

  /* The transport layer address for the IP packets */
  transport_layer_addr_t gNB_addr;

  /* S-GW Tunnel endpoint identifier */
  uint32_t gtp_teid;
} pdusession_setup_t;

typedef struct pdusession_tobe_added_s {
  /* Unique pdusession_id for the UE. */
  uint8_t pdusession_id;

  /* Unique drb_ID for the UE. */
  uint8_t drb_ID;

  /* The transport layer address for the IP packets */
  transport_layer_addr_t upf_addr;

  /* S-GW Tunnel endpoint identifier */
  uint32_t gtp_teid;
} pdusession_tobe_added_t;

typedef struct pdusession_admitted_tobe_added_s {
  /* Unique pdusession_id for the UE. */
  uint8_t pdusession_id;

  /* Unique drb_ID for the UE. */
  uint8_t drb_ID;

  /* The transport layer address for the IP packets */
  transport_layer_addr_t gnb_addr;

  /* S-GW Tunnel endpoint identifier */
  uint32_t gtp_teid;
} pdusession_admitted_tobe_added_t;



typedef struct pdusession_tobeswitched_s {
  /* Unique pdusession_id for the UE. */
  uint8_t pdusession_id;

  /* The transport layer address for the IP packets */
  transport_layer_addr_t upf_addr;

  /* S-GW Tunnel endpoint identifier */
  uint32_t gtp_teid;
} pdusession_tobeswitched_t;

typedef struct pdusession_modify_s {
  /* Unique pdusession_id for the UE. */
  uint8_t pdusession_id;
} pdusession_modify_t;

typedef enum S1ap_Cause_e {
  NGAP_CAUSE_NOTHING,  /* No components present */
  NGAP_CAUSE_RADIO_NETWORK,
  NGAP_CAUSE_TRANSPORT,
  NGAP_CAUSE_NAS,
  NGAP_CAUSE_PROTOCOL,
  NGAP_CAUSE_MISC,
  /* Extensions may appear below */

} ngap_Cause_t;

typedef struct pdusession_failed_s {
  /* Unique pdusession_id for the UE. */
  uint8_t pdusession_id;
  /* Cause of the failure */
  //     cause_t cause;
  ngap_Cause_t cause;
  uint8_t cause_value;
} pdusession_failed_t;

typedef enum ngap_ue_ctxt_modification_present_s {
  NGAP_UE_CONTEXT_MODIFICATION_SECURITY_KEY = (1 << 0),
  NGAP_UE_CONTEXT_MODIFICATION_UE_AMBR      = (1 << 1),
  NGAP_UE_CONTEXT_MODIFICATION_UE_SECU_CAP  = (1 << 2),
} ngap_ue_ctxt_modification_present_t;

typedef enum ngap_paging_ind_present_s {
  NGAP_PAGING_IND_PAGING_DRX      = (1 << 0),
  NGAP_PAGING_IND_PAGING_PRIORITY = (1 << 1),
} ngap_paging_ind_present_t;

//-------------------------------------------------------------------------------------------//
// gNB application layer -> NGAP messages
typedef struct ngap_register_gnb_req_s {
  /* Unique gNB_id to identify the gNB within EPC.
   * For macro gNB ids this field should be 20 bits long.
   * For home gNB ids this field should be 28 bits long.
   */
  uint32_t gNB_id;
  /* The type of the cell */
  enum cell_type_e cell_type;

  /* Optional name for the cell
   * NOTE: the name can be NULL (i.e no name) and will be cropped to 150
   * characters.
   */
  char *gNB_name;

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

  /* Default Paging DRX of the gNB as defined in TS 36.304 */
  paging_drx_t default_drx;

  /* The gNB IP address to bind */
  net_ip_address_t gnb_ip_address;

  /* Nb of AMF to connect to */
  uint8_t          nb_amf;
  /* List of AMF to connect to */
  net_ip_address_t amf_ip_address[NGAP_MAX_NB_AMF_IP_ADDRESS];
  uint8_t          broadcast_plmn_num[NGAP_MAX_NB_AMF_IP_ADDRESS];
  uint8_t          broadcast_plmn_index[NGAP_MAX_NB_AMF_IP_ADDRESS][PLMN_LIST_MAX_SIZE];

  /* Number of SCTP streams used for a amf association */
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;
} ngap_register_gnb_req_t;

//-------------------------------------------------------------------------------------------//
// NGAP -> gNB application layer messages
typedef struct ngap_register_gnb_cnf_s {
  /* Nb of AMF connected */
  uint8_t          nb_amf;
} ngap_register_gnb_cnf_t;

typedef struct ngap_deregistered_gnb_ind_s {
  /* Nb of AMF connected */
  uint8_t          nb_amf;
} ngap_deregistered_gnb_ind_t;

//-------------------------------------------------------------------------------------------//
// RRC -> NGAP messages

/* The NAS First Req is the first message exchanged between RRC and NGAP
 * for an UE.
 * The rnti uniquely identifies an UE within a cell. Later the gnb_ue_ngap_id
 * will be the unique identifier used between RRC and NGAP.
 */
typedef struct ngap_nas_first_req_s {
  /* UE id for initial connection to NGAP */
  uint16_t ue_initial_id;

  /* the chosen PLMN identity as index, see TS 36.331 6.2.2 RRC Connection
   * Setup Complete. This index here is zero-based, unlike the standard! */
  int selected_plmn_identity;

  /* Establishment cause as sent by UE */
  rrc_establishment_cause_t establishment_cause;

  /* NAS PDU */
  nas_pdu_t nas_pdu;

  /* If this flag is set NGAP layer is expecting the GUAMI. If = 0,
   * the temporary s-tmsi is used.
   */
  nrue_identity_t ue_identity;
} ngap_nas_first_req_t;

typedef struct ngap_uplink_nas_s {
  /* Unique UE identifier within an gNB */
  uint32_t  gNB_ue_ngap_id;

  /* NAS pdu */
  nas_pdu_t nas_pdu;
} ngap_uplink_nas_t;

typedef struct ngap_ue_cap_info_ind_s {
  uint32_t  gNB_ue_ngap_id;
  ue_radio_cap_t ue_radio_cap;
} ngap_ue_cap_info_ind_t;

typedef struct ngap_initial_context_setup_resp_s {
  uint32_t  gNB_ue_ngap_id;

  /* Number of pdusession setup-ed in the list */
  uint8_t       nb_of_pdusessions;
  /* list of pdusession setup-ed by RRC layers */
  pdusession_setup_t pdusessions[NGAP_MAX_PDUSESSION];

  /* Number of pdusession failed to be setup in list */
  uint8_t        nb_of_pdusessions_failed;
  /* list of pdusessions that failed to be setup */
  pdusession_failed_t pdusessions_failed[NGAP_MAX_PDUSESSION];
} ngap_initial_context_setup_resp_t;

typedef struct ngap_initial_context_setup_fail_s {
  uint32_t  gNB_ue_ngap_id;

  /* TODO add cause */
} ngap_initial_context_setup_fail_t, ngap_ue_ctxt_modification_fail_t, ngap_pdusession_setup_req_fail_t;

typedef struct ngap_nas_non_delivery_ind_s {
  uint32_t  gNB_ue_ngap_id;
  nas_pdu_t nas_pdu;
  /* TODO: add cause */
} ngap_nas_non_delivery_ind_t;

typedef struct ngap_ue_ctxt_modification_req_s {
  uint32_t  gNB_ue_ngap_id;

  /* Bit-mask of possible present parameters */
  ngap_ue_ctxt_modification_present_t present;

  /* Following fields are optionnaly present */

  /* Security key */
  uint8_t security_key[SECURITY_KEY_LENGTH];

  /* UE aggregate maximum bitrate */
  ambr_t ue_ambr;

  /* NR Security capabilities */
  nr_security_capabilities_t security_capabilities;
} ngap_ue_ctxt_modification_req_t;

typedef struct ngap_ue_ctxt_modification_resp_s {
  uint32_t  gNB_ue_ngap_id;
} ngap_ue_ctxt_modification_resp_t;

typedef struct ngap_ue_release_complete_s {

  uint32_t gNB_ue_ngap_id;

} ngap_ue_release_complete_t;

//-------------------------------------------------------------------------------------------//
// NGAP -> RRC messages
typedef struct ngap_downlink_nas_s {
  /* UE id for initial connection to NGAP */
  uint16_t ue_initial_id;

  /* Unique UE identifier within an gNB */
  uint32_t gNB_ue_ngap_id;

  /* NAS pdu */
  nas_pdu_t nas_pdu;
} ngap_downlink_nas_t;


typedef struct ngap_initial_context_setup_req_s {
  /* UE id for initial connection to NGAP */
  uint16_t ue_initial_id;

  /* gNB ue ngap id as initialized by NGAP layer */
  uint32_t gNB_ue_ngap_id;

  unsigned long long amf_ue_ngap_id:40;

  /* UE aggregate maximum bitrate */
  ambr_t ue_ambr;

  /* guami */
  ngap_guami_t guami;

  /* allowed nssai */
  uint8_t nb_allowed_nssais;
  ngap_allowed_NSSAI_t allowed_nssai[8];

  /* Security algorithms */
  nr_security_capabilities_t security_capabilities;

  /* Security key */
  uint8_t security_key[SECURITY_KEY_LENGTH];

  /* Number of pdusession to be setup in the list */
  uint8_t  nb_of_pdusessions;
  /* list of pdusession to be setup by RRC layers */
  pdusession_t  pdusession_param[NGAP_MAX_PDUSESSION];
} ngap_initial_context_setup_req_t;

typedef struct tai_plmn_identity_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_digit_length;
} plmn_identity_t;

typedef struct ngap_paging_ind_s {
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
} ngap_paging_ind_t;

typedef struct ngap_pdusession_setup_req_s {
  /* UE id for initial connection to NGAP */
  uint16_t ue_initial_id;

  /* AMF UE id  */
  unsigned long long amf_ue_ngap_id:40;

  /* gNB ue ngap id as initialized by NGAP layer */
  uint32_t  gNB_ue_ngap_id;

  /* Number of pdusession to be setup in the list */
  uint8_t nb_pdusessions_tosetup;

  /* E RAB setup request */
  pdusession_t pdusession_setup_params[NGAP_MAX_PDUSESSION];

} ngap_pdusession_setup_req_t;

typedef struct ngap_pdusession_setup_resp_s {
  unsigned  gNB_ue_ngap_id:24;

  /* Number of pdusession setup-ed in the list */
  uint8_t       nb_of_pdusessions;
  /* list of pdusession setup-ed by RRC layers */
  pdusession_setup_t pdusessions[NGAP_MAX_PDUSESSION];

  /* Number of pdusession failed to be setup in list */
  uint8_t        nb_of_pdusessions_failed;
  /* list of pdusessions that failed to be setup */
  pdusession_failed_t pdusessions_failed[NGAP_MAX_PDUSESSION];
} ngap_pdusession_setup_resp_t;

typedef struct ngap_path_switch_req_s {

  uint32_t  gNB_ue_ngap_id;

  /* Number of pdusession setup-ed in the list */
  uint8_t       nb_of_pdusessions;

  /* list of pdusession setup-ed by RRC layers */
  pdusession_setup_t pdusessions_tobeswitched[NGAP_MAX_PDUSESSION];

  /* AMF UE id  */
  unsigned long long amf_ue_ngap_id:40;

  ngap_guami_t ue_guami;

  uint16_t ue_initial_id;

   /* Security algorithms */
  nr_security_capabilities_t security_capabilities;

} ngap_path_switch_req_t;

typedef struct ngap_path_switch_req_ack_s {

  /* UE id for initial connection to NGAP */
  uint16_t ue_initial_id;

  uint32_t  gNB_ue_ngap_id;

  /* AMF UE id  */
  unsigned long long amf_ue_ngap_id:40;

  /* UE aggregate maximum bitrate */
  ambr_t ue_ambr;

  /* Number of pdusession setup-ed in the list */
  uint8_t       nb_pdusessions_tobeswitched;

  /* list of pdusession to be switched by RRC layers */
  pdusession_tobeswitched_t pdusessions_tobeswitched[NGAP_MAX_PDUSESSION];

  /* Number of pdusessions to be released by RRC */
  uint8_t        nb_pdusessions_tobereleased;

  /* list of pdusessions to be released */
  pdusession_failed_t pdusessions_tobereleased[NGAP_MAX_PDUSESSION];

  /* Security key */
  int     next_hop_chain_count;
  uint8_t next_security_key[SECURITY_KEY_LENGTH];

} ngap_path_switch_req_ack_t;

typedef struct ngap_pdusession_modification_ind_s {

  uint32_t  gNB_ue_ngap_id;

  /* AMF UE id  */
  unsigned long long amf_ue_ngap_id:40;

  /* Number of pdusession setup-ed in the list */
  uint8_t       nb_of_pdusessions_tobemodified;

  uint8_t       nb_of_pdusessions_nottobemodified;

  /* list of pdusession setup-ed by RRC layers */
  pdusession_setup_t pdusessions_tobemodified[NGAP_MAX_PDUSESSION];

  pdusession_setup_t pdusessions_nottobemodified[NGAP_MAX_PDUSESSION];

  uint16_t ue_initial_id;

} ngap_pdusession_modification_ind_t;

// NGAP --> RRC messages
typedef struct ngap_ue_release_command_s {

  uint32_t  gNB_ue_ngap_id;

} ngap_ue_release_command_t;


//-------------------------------------------------------------------------------------------//
// NGAP <-- RRC messages
typedef struct ngap_ue_release_req_s {
  uint32_t  gNB_ue_ngap_id;
  ngap_Cause_t  cause;
  long          cause_value;
} ngap_ue_release_req_t, ngap_ue_release_resp_t;

typedef struct ngap_pdusession_modify_req_s {
  /* UE id for initial connection to NGAP */
  uint16_t ue_initial_id;

  /* AMF UE id  */
  unsigned long long amf_ue_ngap_id:40;

  /* gNB ue ngap id as initialized by NGAP layer */
  uint32_t  gNB_ue_ngap_id;

  /* Number of pdusession to be modify in the list */
  uint8_t nb_pdusessions_tomodify;

  /* E RAB modify request */
  pdusession_t pdusession_modify_params[NGAP_MAX_PDUSESSION];
} ngap_pdusession_modify_req_t;

typedef struct ngap_pdusession_modify_resp_s {
  uint32_t  gNB_ue_ngap_id;

  /* Number of pdusession modify-ed in the list */
  uint8_t       nb_of_pdusessions;
  /* list of pdusession modify-ed by RRC layers */
  pdusession_modify_t pdusessions[NGAP_MAX_PDUSESSION];

  /* Number of pdusession failed to be modify in list */
  uint8_t        nb_of_pdusessions_failed;
  /* list of pdusessions that failed to be modify */
  pdusession_failed_t pdusessions_failed[NGAP_MAX_PDUSESSION];
} ngap_pdusession_modify_resp_t;

typedef struct pdusession_release_s {
  /* Unique pdusession_id for the UE. */
  uint8_t                     pdusession_id;
} pdusession_release_t;

typedef struct ngap_pdusession_release_command_s {
  /* AMF UE id  */
  unsigned long long amf_ue_ngap_id:40;

  /* gNB ue ngap id as initialized by NGAP layer */
  uint32_t  gNB_ue_ngap_id;

  /* The NAS PDU should be forwarded by the RRC layer to the NAS layer */
  nas_pdu_t                   nas_pdu;

  /* Number of pdusession to be released in the list */
  uint8_t nb_pdusessions_torelease;

  /* E RAB release command */
  pdusession_release_t pdusession_release_params[NGAP_MAX_PDUSESSION];

} ngap_pdusession_release_command_t;

typedef struct ngap_pdusession_release_resp_s {
  /* AMF UE id  */
  unsigned long long amf_ue_ngap_id:40;

  /* gNB ue ngap id as initialized by NGAP layer */
  uint32_t  gNB_ue_ngap_id;

  /* Number of pdusession released in the list */
  uint8_t       nb_of_pdusessions_released;

  /* list of pdusessions released */
  pdusession_release_t pdusession_release[NGAP_MAX_PDUSESSION];

  /* Number of pdusession failed to be released in list */
  uint8_t        nb_of_pdusessions_failed;
  /* list of pdusessions that failed to be released */
  pdusession_failed_t pdusessions_failed[NGAP_MAX_PDUSESSION];

} ngap_pdusession_release_resp_t;

#endif /* NGAP_MESSAGES_TYPES_H_ */
