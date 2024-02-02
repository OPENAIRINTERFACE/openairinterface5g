/* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
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

/*! \file RRC/NR/nr_rrc_defs.h
* \brief NR RRC struct definitions and function prototypes
* \author Navid Nikaein, Raymond Knopp, WEI-TAI CHEN
* \date 2010 - 2014, 2018
* \version 1.0
* \company Eurecom, NTSUT
* \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr, kroempa@gmail.com
*/

#ifndef __OPENAIR_RRC_DEFS_NR_H__
#define __OPENAIR_RRC_DEFS_NR_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "collection/tree.h"
#include "collection/linear_alloc.h"
#include "nr_rrc_common.h"
#include "ds/byte_array.h"

#include "common/ngran_types.h"
#include "common/platform_constants.h"
#include "common/platform_types.h"
#include "mac_rrc_dl.h"
#include "cucp_cuup_if.h"

#include "NR_SIB1.h"
#include "NR_RRCReconfigurationComplete.h"
#include "NR_RRCReconfiguration.h"
#include "NR_RRCReestablishmentRequest.h"
#include "NR_BCCH-BCH-Message.h"
#include "NR_BCCH-DL-SCH-Message.h"
#include "NR_BCCH-BCH-Message.h"
#include "NR_PLMN-IdentityInfo.h"
#include "NR_MCC-MNC-Digit.h"
#include "NR_NG-5G-S-TMSI.h"

#include "NR_UE-NR-Capability.h"
#include "NR_UE-MRDC-Capability.h"
#include "NR_MeasResults.h"
#include "NR_CellGroupConfig.h"
#include "NR_ServingCellConfigCommon.h"
#include "NR_EstablishmentCause.h"

//-------------------

#include "intertask_interface.h"

/* TODO: be sure this include is correct.
 * It solves a problem of compilation of the RRH GW,
 * issue #186.
 */
  #include "as_message.h"

  #include "commonDef.h"

#define PROTOCOL_NR_RRC_CTXT_UE_FMT                PROTOCOL_CTXT_FMT
#define PROTOCOL_NR_RRC_CTXT_UE_ARGS(CTXT_Pp)      PROTOCOL_NR_CTXT_ARGS(CTXT_Pp)

#define PROTOCOL_NR_RRC_CTXT_FMT                   PROTOCOL_CTXT_FMT
#define PROTOCOL_NR_RRC_CTXT_ARGS(CTXT_Pp)         PROTOCOL_NR_CTXT_ARGS(CTXT_Pp)

// 3GPP TS 38.331 Section 12 Table 12.1-1: UE performance requirements for RRC procedures for UEs
#define NR_RRC_SETUP_DELAY_MS           10
#define NR_RRC_RECONFIGURATION_DELAY_MS 10
#define NR_RRC_BWP_SWITCHING_DELAY_MS   6

// 3GPP TS 38.133 - Section 8 - Table 8.2.1.2.7-2: Parameters which cause interruption other than SCS
// This table was recently added to 3GPP. It shows that changing the parameters locationAndBandwidth, nrofSRS-Ports or
// maxMIMO-Layers-r16 causes an interruption. This parameter is not yet being used in code, but has been placed here
// for future reference.
#define NR_OF_SRS_PORTS_SWITCHING_DELAY_MS 30

#define NR_UE_MODULE_INVALID ((module_id_t) ~0) // FIXME attention! depends on type uint8_t!!!
#define NR_UE_INDEX_INVALID  ((module_id_t) ~0) // FIXME attention! depends on type uint8_t!!! used to be -1

typedef enum {
  NR_RRC_OK=0,
  NR_RRC_ConnSetup_failed,
  NR_RRC_PHY_RESYNCH,
  NR_RRC_Handover_failed,
  NR_RRC_HO_STARTED
} NR_RRC_status_t;

#define RRM_FREE(p)       if ( (p) != NULL) { free(p) ; p=NULL ; }
#define RRM_MALLOC(t,n)   (t *) malloc16( sizeof(t) * n )
#define RRM_CALLOC(t,n)   (t *) malloc16( sizeof(t) * n)
#define RRM_CALLOC2(t,s)  (t *) malloc16( s )

#define MAX_MEAS_OBJ                                  7
#define MAX_MEAS_CONFIG                               7
#define MAX_MEAS_ID                                   7

#define PAYLOAD_SIZE_MAX                              1024
#define RRC_BUF_SIZE                                  1024
#define UNDEF_SECURITY_MODE                           0xff
#define NO_SECURITY_MODE                              0x20

/* TS 36.331: RRC-TransactionIdentifier ::= INTEGER (0..3) */
#define NR_RRC_TRANSACTION_IDENTIFIER_NUMBER 4

typedef struct UE_S_TMSI_NR_s {
  bool                                                presence;
  uint16_t                                            amf_set_id;
  uint8_t                                             amf_pointer;
  uint32_t                                            fiveg_tmsi;
} __attribute__ ((__packed__)) NR_UE_S_TMSI;


typedef enum nr_e_rab_satus_e {
  NR_E_RAB_STATUS_NEW,
  NR_E_RAB_STATUS_DONE,           // from the gNB perspective
  NR_E_RAB_STATUS_ESTABLISHED,    // get the reconfigurationcomplete form UE
  NR_E_RAB_STATUS_FAILED,
} nr_e_rab_status_t;

typedef struct nr_e_rab_param_s {
  e_rab_t param;
  uint8_t status;
  uint8_t xid; // transaction_id
} __attribute__ ((__packed__)) nr_e_rab_param_t;


typedef struct HANDOVER_INFO_NR_s {
  uint8_t                                             ho_prepare;
  uint8_t                                             ho_complete;
  uint8_t                                             modid_s;            //module_idP of serving cell
  uint8_t                                             modid_t;            //module_idP of target cell
  uint8_t                                             ueid_s;             //UE index in serving cell
  uint8_t                                             ueid_t;             //UE index in target cell

  // NR not define at this moment
  //AS_Config_t                                       as_config;          /* these two parameters are taken from 36.331 section 10.2.2: HandoverPreparationInformation-r8-IEs */
  //AS_Context_t                                      as_context;         /* They are mandatory for HO */

  uint8_t                                             buf[RRC_BUF_SIZE];  /* ASN.1 encoded handoverCommandMessage */
  int                                                 size;               /* size of above message in bytes */
} NR_HANDOVER_INFO;

#define NR_RRC_BUFFER_SIZE                            sizeof(RRC_BUFFER_NR)

typedef struct nr_rrc_guami_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_len;
  uint8_t  amf_region_id;
  uint16_t amf_set_id;
  uint8_t  amf_pointer;
} nr_rrc_guami_t;

typedef enum pdu_session_satus_e {
  PDU_SESSION_STATUS_NEW,
  PDU_SESSION_STATUS_DONE,
  PDU_SESSION_STATUS_ESTABLISHED,
  PDU_SESSION_STATUS_REESTABLISHED, // after HO
  PDU_SESSION_STATUS_TOMODIFY, // ENDC NSA
  PDU_SESSION_STATUS_FAILED,
  PDU_SESSION_STATUS_TORELEASE, // to release DRB between eNB and UE
  PDU_SESSION_STATUS_RELEASED
} pdu_session_status_t;

typedef struct pdu_session_param_s {
  pdusession_t param;
  pdu_session_status_t status;
  uint8_t xid; // transaction_id
  ngap_Cause_t cause;
  uint8_t cause_value;
} rrc_pdu_session_param_t;

typedef struct drb_s {
  int status;
  int defaultDRBid;
  int drb_id;
  int reestablishPDCP;
  int recoverPDCP;
  int daps_Config_r16;
  struct cnAssociation_s {
    int present;
    int eps_BearerIdentity;
    struct sdap_config_s {
      bool defaultDRB;
      int pdusession_id;
      int sdap_HeaderDL;
      int sdap_HeaderUL;
      int mappedQoS_FlowsToAdd[QOSFLOW_MAX_VALUE];
    } sdap_config;
  } cnAssociation;
  struct pdcp_config_s {
    int discardTimer;
    int pdcp_SN_SizeUL;
    int pdcp_SN_SizeDL;
    int t_Reordering;
    int integrityProtection;
    struct headerCompression_s {
      int NotUsed;
      int present;
    } headerCompression;
    struct ext1_s {
      int cipheringDisabled;
    } ext1;
  } pdcp_config;
} drb_t;

typedef enum {
  RRC_ACTION_NONE, /* no transaction ongoing */
  RRC_SETUP,
  RRC_SETUP_FOR_REESTABLISHMENT,
  RRC_REESTABLISH,
  RRC_REESTABLISH_COMPLETE,
  RRC_DEFAULT_RECONF,
  RRC_DEDICATED_RECONF,
  RRC_PDUSESSION_ESTABLISH,
  RRC_PDUSESSION_MODIFY,
  RRC_PDUSESSION_RELEASE,
  RRC_UECAPABILITY_ENQUIRY,
} rrc_action_t;

typedef struct gNB_RRC_UE_s {
  drb_t                              established_drbs[MAX_DRBS_PER_UE];
  NR_DRB_ToReleaseList_t            *DRB_ReleaseList;

  NR_SRB_INFO_TABLE_ENTRY Srb[maxSRBs]; // 3gpp max is 3 SRBs, number 1..3, we waste the entry 0 for code simplicity
  NR_MeasConfig_t                   *measConfig;
  NR_HANDOVER_INFO                  *handover_info;
  NR_MeasResults_t                  *measResults;

  bool as_security_active;

  byte_array_t ue_cap_buffer;
  NR_UE_NR_Capability_t*             UE_Capability_nr;
  int                                UE_Capability_size;
  NR_UE_MRDC_Capability_t*           UE_Capability_MRDC;
  int                                UE_MRDC_Capability_size;

  NR_CellGroupConfig_t               *secondaryCellGroup;
  NR_CellGroupConfig_t               *masterCellGroup;
  NR_RRCReconfiguration_t            *reconfig;
  NR_RadioBearerConfig_t             *rb_config;

  ImsiMobileIdentity_t               imsi;

  /* KgNB as derived from KASME received from EPC */
  uint8_t kgnb[32];
  int8_t  kgnb_ncc;
  uint8_t nh[32];
  int8_t  nh_ncc;

  /* Used integrity/ciphering algorithms */
  NR_CipheringAlgorithm_t            ciphering_algorithm;
  e_NR_IntegrityProtAlgorithm        integrity_algorithm;

  NR_UE_STATE_t                      StatusRrc;
  rnti_t                             rnti;
  uint64_t                           random_ue_identity;

  /* Information from UE RRC Setup Request */
  NR_UE_S_TMSI                       Initialue_identity_5g_s_TMSI;
  uint64_t                           ng_5G_S_TMSI_Part1;
  NR_EstablishmentCause_t            establishment_cause;

  uint32_t                           rrc_ue_id;
  uint64_t amf_ue_ngap_id;
  nr_rrc_guami_t                     ue_guami;

  ngap_security_capabilities_t       security_capabilities;
  //NSA block
  /* Number of NSA e_rab */
  uint8_t                            nb_of_e_rabs;
  /* list of pdu session to be setup by RRC layers */
  nr_e_rab_param_t                   e_rab[NB_RB_MAX];//[S1AP_MAX_E_RAB];
  uint32_t                           nsa_gtp_teid[S1AP_MAX_E_RAB];
  transport_layer_addr_t             nsa_gtp_addrs[S1AP_MAX_E_RAB];
  rb_id_t                            nsa_gtp_ebi[S1AP_MAX_E_RAB];
  rb_id_t                            nsa_gtp_psi[S1AP_MAX_E_RAB];

  //SA block
  int nb_of_pdusessions;
  rrc_pdu_session_param_t pduSession[NGAP_MAX_PDU_SESSION];
  rrc_action_t xids[NR_RRC_TRANSACTION_IDENTIFIER_NUMBER];
  uint8_t e_rab_release_command_flag;
  uint32_t ue_rrc_inactivity_timer;
  uint32_t                           ue_reestablishment_counter;
  uint32_t                           ue_reconfiguration_counter;
  struct NR_SpCellConfig                                *spCellConfig;

  /* Nas Pdu */
  ngap_pdu_t nas_pdu;

  /* hack, see rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ() for more info */
  int max_delays_pdu_session;

} gNB_RRC_UE_t;

typedef struct rrc_gNB_ue_context_s {
  /* Tree related data */
  RB_ENTRY(rrc_gNB_ue_context_s) entries;
  /* UE id for initial connection to NGAP */
  struct gNB_RRC_UE_s   ue_context;
} rrc_gNB_ue_context_t;

typedef struct {

  uint8_t                                   *SIB23;
  uint8_t                                   sizeof_SIB23;

} rrc_gNB_carrier_data_t;
//---------------------------------------------------


typedef struct {
  /* nea0 = 0, nea1 = 1, ... */
  int ciphering_algorithms[4];
  int ciphering_algorithms_count;

  /* nia0 = 0, nia1 = 1, ... */
  int integrity_algorithms[4];
  int integrity_algorithms_count;

  /* flags to enable/disable ciphering and integrity for DRBs */
  int do_drb_ciphering;
  int do_drb_integrity;
} nr_security_configuration_t;

typedef struct nr_mac_rrc_dl_if_s {
  f1_setup_response_func_t f1_setup_response;
  f1_setup_failure_func_t f1_setup_failure;
  ue_context_setup_request_func_t ue_context_setup_request;
  ue_context_modification_request_func_t ue_context_modification_request;
  ue_context_modification_confirm_func_t ue_context_modification_confirm;
  ue_context_modification_refuse_func_t ue_context_modification_refuse;
  ue_context_release_command_func_t ue_context_release_command;
  dl_rrc_message_transfer_func_t dl_rrc_message_transfer;
} nr_mac_rrc_dl_if_t;

typedef struct cucp_cuup_if_s {
  cucp_cuup_bearer_context_setup_func_t bearer_context_setup;
  cucp_cuup_bearer_context_setup_func_t bearer_context_mod;
  cucp_cuup_bearer_context_release_func_t bearer_context_release;
} cucp_cuup_if_t;

typedef struct nr_rrc_du_container_t {
  /* Tree-related data */
  RB_ENTRY(nr_rrc_du_container_t) entries;

  sctp_assoc_t assoc_id;
  f1ap_setup_req_t *setup_req;
  NR_MIB_t *mib;
  NR_SIB1_t *sib1;
} nr_rrc_du_container_t;

typedef struct nr_rrc_cuup_container_t {
  /* Tree-related data */
  RB_ENTRY(nr_rrc_cuup_container_t) entries;

  e1ap_setup_req_t *setup_req;
  sctp_assoc_t assoc_id;
} nr_rrc_cuup_container_t;

//---NR---(completely change)---------------------
typedef struct gNB_RRC_INST_s {

  ngran_node_t                                        node_type;
  uint32_t                                            node_id;
  char                                               *node_name;
  int                                                 module_id;
  eth_params_t                                        eth_params_s;
  rrc_gNB_carrier_data_t                              carrier;
  uid_allocator_t                                     uid_allocator;
  RB_HEAD(rrc_nr_ue_tree_s, rrc_gNB_ue_context_s) rrc_ue_head; // ue_context tree key search by rnti
  /// NR cell id
  uint64_t nr_cellid;

  // RRC configuration
  gNB_RrcConfigurationReq configuration;

  // gNB N3 GTPU instance
  instance_t e1_inst;

  char *uecap_file;

  // security configuration (preferred algorithms)
  nr_security_configuration_t security;

  nr_mac_rrc_dl_if_t mac_rrc;
  cucp_cuup_if_t cucp_cuup;

  RB_HEAD(rrc_du_tree, nr_rrc_du_container_t) dus; // DUs, indexed by assoc_id
  size_t num_dus;

  RB_HEAD(rrc_cuup_tree, nr_rrc_cuup_container_t) cuups; // CU-UPs, indexed by assoc_id
  size_t num_cuups;

} gNB_RRC_INST;

#include "nr_rrc_proto.h" //should be put here otherwise compilation error

#endif
