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
 * Author and copyright: Laurent Thomas, open-cells.com
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

#ifndef E1AP_MESSAGES_TYPES_H
#define E1AP_MESSAGES_TYPES_H

#include <netinet/in.h>
#include <netinet/sctp.h>
#include "common/ngran_types.h"
#include "f1ap_messages_types.h"
#include "ngap_messages_types.h"

#define E1AP_MAX_NUM_TRANSAC_IDS 4
#define E1AP_MAX_NUM_PLMNS 4
#define E1AP_MAX_NUM_SLICES 1024
#define E1AP_MAX_NUM_CELL_GROUPS 4
#define E1AP_MAX_NUM_QOS_FLOWS 4
#define E1AP_MAX_NUM_NGRAN_DRB 4
#define E1AP_MAX_NUM_PDU_SESSIONS 4
#define E1AP_MAX_NUM_DRBS 4
#define E1AP_MAX_NUM_DRBS 4
#define E1AP_MAX_NUM_UP_PARAM 4

#define E1AP_REGISTER_REQ(mSGpTR)                         (mSGpTR)->ittiMsg.e1ap_register_req
#define E1AP_SETUP_REQ(mSGpTR)                            (mSGpTR)->ittiMsg.e1ap_setup_req
#define E1AP_SETUP_RESP(mSGpTR)                           (mSGpTR)->ittiMsg.e1ap_setup_resp
#define E1AP_SETUP_FAIL(mSGpTR)                           (mSGpTR)->ittiMsg.e1ap_setup_fail
#define E1AP_BEARER_CONTEXT_SETUP_REQ(mSGpTR)             (mSGpTR)->ittiMsg.e1ap_bearer_setup_req
#define E1AP_BEARER_CONTEXT_SETUP_RESP(mSGpTR)            (mSGpTR)->ittiMsg.e1ap_bearer_setup_resp
#define E1AP_BEARER_CONTEXT_MODIFICATION_REQ(mSGpTR)      (mSGpTR)->ittiMsg.e1ap_bearer_setup_req
#define E1AP_BEARER_CONTEXT_MODIFICATION_RESP(mSGpTR)     (mSGpTR)->ittiMsg.e1ap_bearer_modif_resp
#define E1AP_BEARER_CONTEXT_RELEASE_CMD(mSGpTR)           (mSGpTR)->ittiMsg.e1ap_bearer_release_cmd
#define E1AP_BEARER_CONTEXT_RELEASE_CPLT(mSGpTR)          (mSGpTR)->ittiMsg.e1ap_bearer_release_cplt
#define E1AP_LOST_CONNECTION(mSGpTR)                      (mSGpTR)->ittiMsg.e1ap_lost_connection

typedef net_ip_address_t e1ap_net_ip_address_t;

typedef struct PLMN_ID_s {
  int mcc;
  int mnc;
  int mnc_digit_length;
} PLMN_ID_t;

typedef nssai_t e1ap_nssai_t;

typedef struct e1ap_net_config_t {
  net_ip_address_t CUUP_e1_ip_address;
  net_ip_address_t CUCP_e1_ip_address;
  uint16_t remotePortF1U;
  char* localAddressF1U;
  uint16_t localPortF1U;
  char* localAddressN3;
  uint16_t localPortN3;
  uint16_t remotePortN3;
} e1ap_net_config_t;

typedef struct e1ap_setup_req_s {
  uint64_t              gNB_cu_up_id;
  char *                gNB_cu_up_name;
  uint64_t              transac_id;
  int                   supported_plmns;
  struct {
    PLMN_ID_t id;
    int supported_slices;
    e1ap_nssai_t *slice;
  } plmn[E1AP_MAX_NUM_PLMNS];
} e1ap_setup_req_t;

typedef struct e1ap_register_req_t {
  e1ap_setup_req_t setup_req;
  e1ap_net_config_t net_config;
  uint32_t gnb_id; // unused in CU-UP, but might be necessary for some functionality, e.g., E2 agent
} e1ap_register_req_t;

typedef struct e1ap_setup_resp_s {
  long transac_id;
} e1ap_setup_resp_t;

/* E1AP Setup Failure */
typedef struct e1ap_setup_fail_s {
  long transac_id;
} e1ap_setup_fail_t;

typedef struct cell_group_s {
  long id;
} cell_group_t;

typedef struct up_params_s {
  in_addr_t tlAddress;
  long teId;
  int cell_group_id;
} up_params_t;

typedef struct drb_to_setup_s {
  long drbId;
  long pDCP_SN_Size_UL;
  long pDCP_SN_Size_DL;
  long rLC_Mode;
  long qci;
  long qosPriorityLevel;
  long pre_emptionCapability;
  long pre_emptionVulnerability;
  in_addr_t tlAddress;
  long teId;
  int numCellGroups;
  cell_group_t cellGroupList[E1AP_MAX_NUM_CELL_GROUPS];
} drb_to_setup_t;

typedef struct qos_characteristics_s {
  union {
    struct {
      long fiveqi;
      long qos_priority_level;
    } non_dynamic;
    struct {
      long fiveqi; // -1 -> optional
      long qos_priority_level;
      long packet_delay_budget;
      struct {
        long per_scalar;
        long per_exponent;
      } packet_error_rate;
    } dynamic;
  };
  fiveQI_type_t qos_type;
} qos_characteristics_t;

typedef struct ngran_allocation_retention_priority_s {
  uint16_t priority_level;
  long preemption_capability;
  long preemption_vulnerability;
} ngran_allocation_retention_priority_t;

typedef struct qos_flow_level_qos_parameters_s {
  qos_characteristics_t qos_characteristics;
  ngran_allocation_retention_priority_t alloc_reten_priority; // additional members should be added!!
} qos_flow_level_qos_parameters_t;

typedef struct qos_flow_setup_e {
  long qfi; // qos flow identifier
  qos_flow_level_qos_parameters_t qos_params;
} qos_flow_to_setup_t;

typedef struct DRB_nGRAN_to_setup_s {
  long id;
  long defaultDRB;
  long sDAP_Header_UL;
  long sDAP_Header_DL;
  long pDCP_SN_Size_UL;
  long pDCP_SN_Size_DL;
  long discardTimer;
  long reorderingTimer;
  long rLC_Mode;
  in_addr_t tlAddress;
  int teId;
  int numDlUpParam;
  up_params_t DlUpParamList[E1AP_MAX_NUM_UP_PARAM];
  int numCellGroups;
  cell_group_t cellGroupList[E1AP_MAX_NUM_CELL_GROUPS];
  int numQosFlow2Setup;
  qos_flow_to_setup_t qosFlows[E1AP_MAX_NUM_QOS_FLOWS];
} DRB_nGRAN_to_setup_t;

typedef struct pdu_session_to_setup_s {
  long sessionId;
  long sessionType;
  e1ap_nssai_t nssai;
  long integrityProtectionIndication;
  long confidentialityProtectionIndication;
  in_addr_t tlAddress;
  in_addr_t tlAddress_dl;
  int32_t teId;
  int32_t teId_dl;
  int tl_port;
  int tl_port_dl;
  long numDRB2Setup;
  DRB_nGRAN_to_setup_t DRBnGRanList[E1AP_MAX_NUM_NGRAN_DRB];
  long numDRB2Modify;
  DRB_nGRAN_to_setup_t DRBnGRanModList[E1AP_MAX_NUM_NGRAN_DRB];
} pdu_session_to_setup_t;

typedef struct e1ap_bearer_setup_req_s {
  uint32_t gNB_cu_cp_ue_id;
  uint32_t gNB_cu_up_ue_id;
  uint64_t cipheringAlgorithm;
  uint64_t integrityProtectionAlgorithm;
  char     encryptionKey[128];
  char     integrityProtectionKey[128];
  long     ueDlAggMaxBitRate;
  PLMN_ID_t servingPLMNid;
  long activityNotificationLevel;
  int numPDUSessions;
  pdu_session_to_setup_t pduSession[E1AP_MAX_NUM_PDU_SESSIONS];
  int numPDUSessionsMod;
  pdu_session_to_setup_t pduSessionMod[E1AP_MAX_NUM_PDU_SESSIONS];
} e1ap_bearer_setup_req_t;

typedef struct e1ap_bearer_release_cmd_s {
  uint32_t gNB_cu_cp_ue_id;
  uint32_t gNB_cu_up_ue_id;
  long cause_type;
  long cause;
} e1ap_bearer_release_cmd_t;

typedef struct e1ap_bearer_release_cplt_s {
  uint32_t gNB_cu_cp_ue_id;
  uint32_t gNB_cu_up_ue_id;
} e1ap_bearer_release_cplt_t;

typedef struct qos_flow_setup_s {
  long qfi;
} qos_flow_setup_t;

typedef struct DRB_nGRAN_setup_s {
  long id;
  int numUpParam;
  up_params_t UpParamList[E1AP_MAX_NUM_UP_PARAM];
  int numQosFlowSetup;
  qos_flow_setup_t qosFlows[E1AP_MAX_NUM_QOS_FLOWS];
} DRB_nGRAN_setup_t;

typedef struct DRB_nGRAN_modified_s {
  long id;
} DRB_nGRAN_modified_t;

typedef struct DRB_nGRAN_failed_s {
  long id;
  long cause_type;
  long cause;
} DRB_nGRAN_failed_t;

typedef struct pdu_session_setup_s {
  long id;
  in_addr_t tlAddress;
  long teId;
  int numDRBSetup;
  DRB_nGRAN_setup_t DRBnGRanList[E1AP_MAX_NUM_NGRAN_DRB];
  int numDRBFailed;
  DRB_nGRAN_failed_t DRBnGRanFailedList[E1AP_MAX_NUM_NGRAN_DRB];
} pdu_session_setup_t;

typedef struct pdu_session_modif_s {
  long id;
  // setup as part of PDU session modification not supported yet
  int numDRBModified;
  DRB_nGRAN_modified_t DRBnGRanModList[E1AP_MAX_NUM_NGRAN_DRB];
} pdu_session_modif_t;

typedef struct e1ap_bearer_setup_resp_s {
  uint32_t gNB_cu_cp_ue_id;
  uint32_t gNB_cu_up_ue_id;
  int numPDUSessions;
  pdu_session_setup_t pduSession[E1AP_MAX_NUM_PDU_SESSIONS];
} e1ap_bearer_setup_resp_t;

typedef struct e1ap_bearer_modif_resp_s {
  uint32_t gNB_cu_cp_ue_id;
  uint32_t gNB_cu_up_ue_id;
  int numPDUSessionsMod;
  pdu_session_modif_t pduSessionMod[E1AP_MAX_NUM_PDU_SESSIONS];
} e1ap_bearer_modif_resp_t;

/* E1AP Connection Loss indication */
typedef struct e1ap_lost_connection_t {
  int dummy;
} e1ap_lost_connection_t;


#endif /* E1AP_MESSAGES_TYPES_H */
