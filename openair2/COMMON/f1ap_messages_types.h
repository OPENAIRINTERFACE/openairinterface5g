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

#include <netinet/in.h>
#include <netinet/sctp.h>
#include "common/5g_platform_types.h"
#include "common/platform_constants.h"
#include "common/utils/ds/byte_array.h"

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.

#define F1AP_CU_SCTP_REQ(mSGpTR)                   (mSGpTR)->ittiMsg.f1ap_cu_sctp_req

#define F1AP_DU_REGISTER_REQ(mSGpTR)               (mSGpTR)->ittiMsg.f1ap_du_register_req

#define F1AP_RESET(mSGpTR)                         (mSGpTR)->ittiMsg.f1ap_reset
#define F1AP_RESET_ACK(mSGpTR)                         (mSGpTR)->ittiMsg.f1ap_reset_ack

#define F1AP_SETUP_REQ(mSGpTR)                     (mSGpTR)->ittiMsg.f1ap_setup_req
#define F1AP_SETUP_RESP(mSGpTR)                    (mSGpTR)->ittiMsg.f1ap_setup_resp
#define F1AP_GNB_CU_CONFIGURATION_UPDATE(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_gnb_cu_configuration_update
#define F1AP_GNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_gnb_cu_configuration_update_acknowledge
#define F1AP_GNB_CU_CONFIGURATION_UPDATE_FAILURE(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_gnb_cu_configuration_update_failure
#define F1AP_GNB_DU_CONFIGURATION_UPDATE(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_gnb_du_configuration_update
#define F1AP_GNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_gnb_du_configuration_update_acknowledge
#define F1AP_GNB_DU_CONFIGURATION_UPDATE_FAILURE(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_gnb_du_configuration_update_failure

#define F1AP_SETUP_FAILURE(mSGpTR)                 (mSGpTR)->ittiMsg.f1ap_setup_failure

#define F1AP_LOST_CONNECTION(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_lost_connection

#define F1AP_INITIAL_UL_RRC_MESSAGE(mSGpTR)        (mSGpTR)->ittiMsg.f1ap_initial_ul_rrc_message
#define F1AP_UL_RRC_MESSAGE(mSGpTR)                (mSGpTR)->ittiMsg.f1ap_ul_rrc_message
#define F1AP_UE_CONTEXT_SETUP_REQ(mSGpTR)          (mSGpTR)->ittiMsg.f1ap_ue_context_setup_req
#define F1AP_UE_CONTEXT_SETUP_RESP(mSGpTR)         (mSGpTR)->ittiMsg.f1ap_ue_context_setup_resp
#define F1AP_UE_CONTEXT_MODIFICATION_REQ(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_ue_context_mod_req
#define F1AP_UE_CONTEXT_MODIFICATION_RESP(mSGpTR)  (mSGpTR)->ittiMsg.f1ap_ue_context_modification_resp
#define F1AP_UE_CONTEXT_MODIFICATION_FAIL(mSGpTR)  (mSGpTR)->ittiMsg.f1ap_ue_context_modification_fail
#define F1AP_UE_CONTEXT_MODIFICATION_REQUIRED(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_ue_context_modification_required
#define F1AP_UE_CONTEXT_MODIFICATION_CONFIRM(mSGpTR)  (mSGpTR)->ittiMsg.f1ap_ue_context_modification_confirm
#define F1AP_UE_CONTEXT_MODIFICATION_REFUSE(mSGpTR)  (mSGpTR)->ittiMsg.f1ap_ue_context_modification_refuse

#define F1AP_DL_RRC_MESSAGE(mSGpTR)                (mSGpTR)->ittiMsg.f1ap_dl_rrc_message
#define F1AP_UE_CONTEXT_RELEASE_REQ(mSGpTR)        (mSGpTR)->ittiMsg.f1ap_ue_context_release_req
#define F1AP_UE_CONTEXT_RELEASE_CMD(mSGpTR)        (mSGpTR)->ittiMsg.f1ap_ue_context_release_cmd
#define F1AP_UE_CONTEXT_RELEASE_COMPLETE(mSGpTR)   (mSGpTR)->ittiMsg.f1ap_ue_context_release_complete

#define F1AP_PAGING_IND(mSGpTR)                    (mSGpTR)->ittiMsg.f1ap_paging_ind

/* Length of the transport layer address string
 * 160 bits / 8 bits by char.
 */
#define F1AP_TRANSPORT_LAYER_ADDRESS_SIZE (160 / 8)

#define F1AP_MAX_NB_CU_IP_ADDRESS 10

// Note this should be 512 from maxval in 38.473
#define F1AP_MAX_NB_CELLS 2

#define F1AP_MAX_NO_OF_TNL_ASSOCIATIONS 32
#define F1AP_MAX_NO_UE_ID 1024

#define F1AP_MAX_NO_OF_INDIVIDUAL_CONNECTIONS_TO_RESET 65536
/* 9.3.1.42 of 3GPP TS 38.473 - gNB-CU System Information */
#define F1AP_MAX_NO_SIB_TYPES 32
/* Maximum number of PLMNs that can be served by a cell */
#define F1AP_MAX_NB_PLMNS 6

typedef struct f1ap_net_config_t {
  char *CU_f1_ip_address;
  char *DU_f1c_ip_address;
  char *DU_f1u_ip_address;
  uint16_t CUport;
  uint16_t DUport;
} f1ap_net_config_t;

typedef struct f1ap_cp_tnl_s {
  in_addr_t tl_address; // currently only IPv4 supported
  uint16_t port;
} f1ap_cp_tnl_t;

typedef enum f1ap_mode_t { F1AP_MODE_TDD = 0, F1AP_MODE_FDD = 1 } f1ap_mode_t;

typedef struct f1ap_nr_frequency_info_t {
  uint32_t arfcn;
  int band;
} f1ap_nr_frequency_info_t;

typedef struct f1ap_transmission_bandwidth_t {
  uint8_t scs;
  uint16_t nrb;
} f1ap_transmission_bandwidth_t;

typedef struct f1ap_fdd_info_t {
  f1ap_nr_frequency_info_t ul_freqinfo;
  f1ap_nr_frequency_info_t dl_freqinfo;
  f1ap_transmission_bandwidth_t ul_tbw;
  f1ap_transmission_bandwidth_t dl_tbw;
} f1ap_fdd_info_t;

typedef struct f1ap_tdd_info_t {
  f1ap_nr_frequency_info_t freqinfo;
  f1ap_transmission_bandwidth_t tbw;
} f1ap_tdd_info_t;

/* PLMN information including its slice list
 * Per 38.473 §9.3.1.10, each PLMN can have its own set of slices */
typedef struct f1ap_served_plmn_info_t {
  plmn_id_t plmn;
  uint16_t num_nssai;
  nssai_t nssai[MAX_NUM_SLICES];
} f1ap_served_plmn_info_t;

typedef struct f1ap_served_cell_info_t {
  // NR CGI
  plmn_id_t plmn;
  uint64_t nr_cellid; // NR Global Cell Id

  // NR Physical Cell Ids
  uint16_t nr_pci;

  /* Tracking area code */
  uint32_t *tac;

  // PLMN list
  uint16_t num_plmn;
  f1ap_served_plmn_info_t served_plmn_list[F1AP_MAX_NB_PLMNS];

  f1ap_mode_t mode;
  union {
    f1ap_fdd_info_t fdd;
    f1ap_tdd_info_t tdd;
  };

  uint8_t *measurement_timing_config;
  int measurement_timing_config_len;
} f1ap_served_cell_info_t;

typedef struct f1ap_gnb_du_system_info_t {
  uint8_t *mib;
  int mib_length;
  uint8_t *sib1;
  int sib1_length;
} f1ap_gnb_du_system_info_t;

typedef struct f1ap_setup_req_s {
  /// ulong transaction id
  uint64_t transaction_id;

  // F1_Setup_Req payload
  uint64_t gNB_DU_id;
  char *gNB_DU_name;

  /// rrc version
  uint8_t rrc_ver[3];

  /// number of DU cells available
  uint16_t num_cells_available; //0< num_cells_available <= 512;
  struct {
    f1ap_served_cell_info_t info;
    f1ap_gnb_du_system_info_t *sys_info;
  } cell[F1AP_MAX_NB_CELLS];
} f1ap_setup_req_t;

typedef struct f1ap_du_register_req_t {
  f1ap_setup_req_t setup_req;
  f1ap_net_config_t net_config;
} f1ap_du_register_req_t;

typedef struct f1ap_sib_msg_t {
  /// RRC container with system information owned by gNB-CU
  uint8_t *SI_container;
  int SI_container_length;
  /// SIB block type, e.g. 2 for sibType2
  int SI_type;
} f1ap_sib_msg_t;

typedef struct served_cells_to_activate_s {
  plmn_id_t plmn;
  // NR Global Cell Id
  uint64_t nr_cellid;
  /// NRPCI [int 0..1007]
  uint16_t nrpci;
  /// num SI messages per DU cell
  uint8_t num_SI;
  /// gNB-CU System Information message (up to 32 messages per cell)
  f1ap_sib_msg_t SI_msg[F1AP_MAX_NO_SIB_TYPES];
} served_cells_to_activate_t;

typedef struct f1ap_setup_resp_s {
  /// ulong transaction id
  uint64_t transaction_id;
  /// string holding gNB_CU_name
  char     *gNB_CU_name;
  /// number of DU cells to activate
  uint16_t num_cells_to_activate; //0< num_cells_to_activate <= 512;
  served_cells_to_activate_t cells_to_activate[F1AP_MAX_NB_CELLS];

  /// rrc version
  uint8_t rrc_ver[3];

} f1ap_setup_resp_t;

typedef struct f1ap_gnb_cu_configuration_update_s {
  /// Transaction ID
  uint64_t transaction_id;
  /// number of DU cells to activate
  uint16_t num_cells_to_activate; //0< num_cells_to_activate/mod <= 512;
  served_cells_to_activate_t cells_to_activate[F1AP_MAX_NB_CELLS];
} f1ap_gnb_cu_configuration_update_t;

typedef struct f1ap_setup_failure_s {
  uint16_t cause;
  uint16_t time_to_wait;
  uint16_t criticality_diagnostics;
  /// Transaction ID (M)
  uint64_t transaction_id;
} f1ap_setup_failure_t;

typedef struct f1ap_gnb_cu_configuration_update_acknowledge_s {
  uint64_t transaction_id;
  // Cells Failed to be Activated List
  uint16_t num_cells_failed_to_be_activated;
  struct {
    // NR CGI
    plmn_id_t plmn;
    uint64_t nr_cellid;
    uint16_t cause;
  } cells_failed_to_be_activated[F1AP_MAX_NB_CELLS];
  int have_criticality;
  uint16_t criticality_diagnostics;
  // gNB-CU TNL Association Setup List
  uint16_t noofTNLAssociations_to_setup;
  f1ap_cp_tnl_t tnlAssociations_to_setup[F1AP_MAX_NO_OF_TNL_ASSOCIATIONS];
  // gNB-CU TNL Association Failed to Setup List
  uint16_t noofTNLAssociations_failed;
  f1ap_cp_tnl_t tnlAssociations_failed[F1AP_MAX_NO_OF_TNL_ASSOCIATIONS];
  // Dedicated SI Delivery Needed UE List
  uint16_t noofDedicatedSIDeliveryNeededUEs;
  struct {
    uint32_t gNB_CU_ue_id;
    // NR CGI
    plmn_id_t ue_plmn;
    uint64_t ue_nr_cellid;
  } dedicatedSIDeliveryNeededUEs[F1AP_MAX_NO_UE_ID];
} f1ap_gnb_cu_configuration_update_acknowledge_t;

typedef struct f1ap_gnb_cu_configuration_update_failure_s {
  uint16_t cause;
  uint16_t time_to_wait;
  uint16_t criticality_diagnostics; 
} f1ap_gnb_cu_configuration_update_failure_t;

typedef struct f1ap_cell_status_t {
  // NR CGI
  plmn_id_t plmn;
  uint64_t nr_cellid; // NR Global Cell Id
  enum { F1AP_STATE_IN_SERVICE, F1AP_STATE_OUT_OF_SERVICE} service_state;
} f1ap_cell_status_t;

/*DU configuration messages*/
typedef struct f1ap_gnb_du_configuration_update_s {
  /*TODO UPDATE TO SUPPORT DU CONFIG*/

  /* Transaction ID */
  uint64_t transaction_id;
  /// int cells_to_add
  uint16_t num_cells_to_add;
  struct {
    f1ap_served_cell_info_t info;
    f1ap_gnb_du_system_info_t *sys_info;
  } cell_to_add[F1AP_MAX_NB_CELLS];

  /// int cells_to_modify
  uint16_t num_cells_to_modify;
  struct {
    plmn_id_t old_plmn;
    uint64_t old_nr_cellid; // NR Global Cell Id
    f1ap_served_cell_info_t info;
    f1ap_gnb_du_system_info_t *sys_info;
  } cell_to_modify[F1AP_MAX_NB_CELLS];

  /// int cells_to_delete
  uint16_t num_cells_to_delete;
  struct {
    // NR CGI
    plmn_id_t plmn;
    uint64_t nr_cellid; // NR Global Cell Id
  } cell_to_delete[F1AP_MAX_NB_CELLS];

  f1ap_cell_status_t status[F1AP_MAX_NB_CELLS];
  int num_status;

  /// gNB-DU unique ID, at least within a gNB-CU (0 .. 2^36 - 1)
  uint64_t *gNB_DU_ID;
} f1ap_gnb_du_configuration_update_t;

typedef struct f1ap_gnb_du_configuration_update_acknowledge_s {
  /// ulong transaction id
  uint64_t transaction_id;
  /// number of DU cells to activate
  uint16_t num_cells_to_activate; // 0< num_cells_to_activate <= 512;
  served_cells_to_activate_t cells_to_activate[F1AP_MAX_NB_CELLS];
} f1ap_gnb_du_configuration_update_acknowledge_t;

typedef struct f1ap_gnb_du_configuration_update_failure_s {
  /*TODO UPDATE TO SUPPORT DU CONFIG*/
  uint16_t cause;
  uint16_t time_to_wait;
  uint16_t criticality_diagnostics;
} f1ap_gnb_du_configuration_update_failure_t;

typedef struct f1ap_dl_rrc_message_s {

  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;
  uint32_t *old_gNB_DU_ue_id;
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
  plmn_id_t plmn;
  /// nr cell id
  uint64_t nr_cellid;
  /// crnti
  uint16_t crnti;
  uint8_t *rrc_container;
  int      rrc_container_length;
  uint8_t *du2cu_rrc_container;
  int      du2cu_rrc_container_length;
  uint8_t transaction_id;
} f1ap_initial_ul_rrc_message_t;

typedef struct f1ap_ul_rrc_message_s {
  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;
  uint8_t  srb_id;
  uint8_t *rrc_container;
  int      rrc_container_length;
} f1ap_ul_rrc_message_t;

typedef struct f1ap_up_tnl_s {
  in_addr_t tl_address; // currently only IPv4 supported
  uint32_t teid;
} f1ap_up_tnl_t;

typedef enum preemption_capability_e {
  SHALL_NOT_TRIGGER_PREEMPTION,
  MAY_TRIGGER_PREEMPTION,
} preemption_capability_t;

typedef enum preemption_vulnerability_e {
  NOT_PREEMPTABLE,
  PREEMPTABLE,
} preemption_vulnerability_t;

typedef enum f1ap_rlc_mode_t { F1AP_RLC_MODE_AM, F1AP_RLC_MODE_UM_BIDIR, F1AP_RLC_UM_UNI_UL, F1AP_RLC_UM_UNI_DL } f1ap_rlc_mode_t;

typedef struct f1ap_cu_to_du_rrc_info_s {
  byte_array_t *cg_configinfo;
  byte_array_t *ue_cap;
  byte_array_t *meas_config;
  byte_array_t *meas_timing_config;
  byte_array_t *ho_prep_info;
} f1ap_cu_to_du_rrc_info_t;

typedef struct f1ap_du_to_cu_rrc_info_t {
  byte_array_t cell_group_config;
  byte_array_t *meas_gap_config;
} f1ap_du_to_cu_rrc_info_t;

typedef struct du_to_cu_rrc_information_s {
  uint8_t * cellGroupConfig;
  uint32_t  cellGroupConfig_length;
  uint8_t * measGapConfig;
  uint32_t  measGapConfig_length;
  uint8_t * requestedP_MaxFR1;
  uint32_t  requestedP_MaxFR1_length;
}du_to_cu_rrc_information_t;

typedef enum ReconfigurationCompl_e {
  RRCreconf_info_not_present = 0,
  RRCreconf_failure          = 1,
  RRCreconf_success          = 2,
} ReconfigurationCompl_t;

typedef enum TransmActionInd_e {
  TransmActionInd_STOP,
  TransmActionInd_RESTART,
} TransmActionInd_t;

typedef enum lower_layer_status_e {
  LOWER_LAYERS_SUSPEND,
  LOWER_LAYERS_RESUME,
} lower_layer_status_t;

typedef struct f1ap_srb_to_setup_t {
  int id;
} f1ap_srb_to_setup_t;

typedef struct f1ap_srb_setup_t {
  int id;
  int lcid;
} f1ap_srb_setup_t;

/// 9.3.1.52 Packet Error Rate
typedef struct f1ap_per_t {
  uint8_t scalar;
  uint8_t exponent;
} f1ap_per_t;

// 9.3.1.49 Non-Dynamic 5QI Descriptor
typedef struct f1ap_nondynamic_5qi_t {
  int fiveQI;
} f1ap_nondynamic_5qi_t;

// 9.3.1.47 Dynamic 5QI Descriptor
typedef struct f1ap_dynamic_5qi_t {
  int prio; /// QoS Priority Level
  int pdb; /// Packet Delay Budget
  f1ap_per_t per; /// Packet Error Rate
  bool *delay_critical; /// Delay Critical
  int *avg_win; /// Averaging Window
} f1ap_dynamic_5qi_t;

typedef struct f1ap_arp_t {
  uint16_t prio;
  preemption_capability_t preempt_cap;
  preemption_vulnerability_t preempt_vuln;
} f1ap_arp_t;

// 9.3.1.45 QoS Flow Level QoS Parameters
typedef struct f1ap_qos_flow_param_t {
  fiveQI_t qos_type;
  union {
    f1ap_nondynamic_5qi_t nondyn;
    f1ap_dynamic_5qi_t dyn;
  };
  f1ap_arp_t arp;
} f1ap_qos_flow_param_t;

// in 9.2.2.1 Flows Mapped to DRB Item
typedef struct f1ap_drb_flows_mapped_t {
  int qfi;
  f1ap_qos_flow_param_t param;
} f1ap_drb_flows_mapped_t;

/// in 9.2.2.1 DRB Info in UE Context Setup Request
typedef struct f1ap_drb_info_nr_t {
  f1ap_qos_flow_param_t drb_qos;
  nssai_t nssai;
  int flows_len;
  f1ap_drb_flows_mapped_t *flows;
} f1ap_drb_info_nr_t;

typedef enum { F1AP_PDCP_SN_12B, F1AP_PDCP_SN_18B } f1ap_pdcp_sn_len_t;
/// in 9.2.2.1 DRB to Be Setup Item IEs
typedef struct f1ap_drb_to_setup_t {
  int id;
  enum { F1AP_QOS_CHOICE_EUTRAN, F1AP_QOS_CHOICE_NR } qos_choice;
  union {
    // eutran not implemented
    f1ap_drb_info_nr_t nr;
  };
  int up_ul_tnl_len;
  f1ap_up_tnl_t up_ul_tnl[2];
  f1ap_rlc_mode_t rlc_mode;
  f1ap_pdcp_sn_len_t *dl_pdcp_sn_len; // in setup mandatory, in setupmod optional
  f1ap_pdcp_sn_len_t *ul_pdcp_sn_len;
} f1ap_drb_to_setup_t;

typedef struct f1ap_drb_setup_t {
  int id;
  int *lcid;
  int up_dl_tnl_len;
  f1ap_up_tnl_t up_dl_tnl[2];
} f1ap_drb_setup_t;

typedef struct f1ap_drb_to_release_t {
  int id;
} f1ap_drb_to_release_t;

typedef struct f1ap_ue_context_setup_req_s {
  uint32_t gNB_CU_ue_id;
  uint32_t *gNB_DU_ue_id;

  plmn_id_t plmn;
  uint64_t nr_cellid;
  uint8_t servCellIndex;

  f1ap_cu_to_du_rrc_info_t cu_to_du_rrc_info;

  int srbs_len;
  f1ap_srb_to_setup_t *srbs;

  int drbs_len;
  f1ap_drb_to_setup_t *drbs;

  byte_array_t *rrc_container;

  uint64_t *gnb_du_ue_agg_mbr_ul; // C-ifDRBSetup
} f1ap_ue_context_setup_req_t;

typedef struct f1ap_ue_context_setup_resp_s {
  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;

  f1ap_du_to_cu_rrc_info_t du_to_cu_rrc_info;

  uint16_t *crnti;

  int drbs_len;
  f1ap_drb_setup_t *drbs;

  int srbs_len;
  f1ap_srb_setup_t *srbs;
} f1ap_ue_context_setup_resp_t;

typedef struct f1ap_ue_context_mod_req_t {
  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;

  plmn_id_t *plmn;
  uint64_t *nr_cellid;
  uint8_t *servCellIndex;

  f1ap_cu_to_du_rrc_info_t *cu_to_du_rrc_info;

  TransmActionInd_t *transm_action_ind;
  ReconfigurationCompl_t *reconfig_compl;

  byte_array_t *rrc_container;

  int srbs_len;
  f1ap_srb_to_setup_t *srbs; // reuse the same type as for setup; there is a
                             // list for SRB modification

  int drbs_len;
  f1ap_drb_to_setup_t *drbs; // as for SRBs

  int drbs_rel_len;
  f1ap_drb_to_release_t *drbs_rel;

  lower_layer_status_t *status;
} f1ap_ue_context_mod_req_t;

typedef struct f1ap_ue_context_mod_resp {
  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;

  f1ap_du_to_cu_rrc_info_t *du_to_cu_rrc_info;

  int drbs_len;
  f1ap_drb_setup_t *drbs;

  int srbs_len;
  f1ap_srb_setup_t *srbs;
} f1ap_ue_context_mod_resp_t;

typedef enum F1ap_Cause_e {
  F1AP_CAUSE_NOTHING,  /* No components present */
  F1AP_CAUSE_RADIO_NETWORK,
  F1AP_CAUSE_TRANSPORT,
  F1AP_CAUSE_PROTOCOL,
  F1AP_CAUSE_MISC,
} f1ap_Cause_t;

typedef struct f1ap_ue_context_modif_required_t {
  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;
  du_to_cu_rrc_information_t *du_to_cu_rrc_information;
  f1ap_Cause_t cause;
  long cause_value;
} f1ap_ue_context_modif_required_t;

typedef struct f1ap_ue_context_modif_confirm_t {
  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;
  uint8_t *rrc_container;
  int      rrc_container_length;
} f1ap_ue_context_modif_confirm_t;

typedef struct f1ap_ue_context_modif_refuse_t {
  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;
  f1ap_Cause_t cause;
  long cause_value;
} f1ap_ue_context_modif_refuse_t;

typedef struct f1ap_ue_context_rel_req_t {
  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;
  f1ap_Cause_t cause;
  long cause_value;
} f1ap_ue_context_rel_req_t;

typedef struct f1ap_ue_context_rel_cmd_t {
  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;
  f1ap_Cause_t cause;
  long cause_value;

  byte_array_t *rrc_container;
  int *srb_id; // C-ifRRCContainer

  uint32_t *old_gNB_DU_ue_id; // if after reestablishment request
} f1ap_ue_context_rel_cmd_t;

typedef struct f1ap_ue_context_rel_cplt_t {
  uint32_t gNB_CU_ue_id;
  uint32_t gNB_DU_ue_id;
} f1ap_ue_context_rel_cplt_t;

typedef struct f1ap_paging_ind_s {
  uint16_t ueidentityindexvalue;
  uint64_t fiveg_s_tmsi;
  uint8_t  fiveg_s_tmsi_length;
  plmn_id_t plmn;
  uint64_t nr_cellid;
  uint8_t  paging_drx;
} f1ap_paging_ind_t;

typedef struct f1ap_lost_connection_t {
  int dummy;
} f1ap_lost_connection_t;

typedef enum F1AP_ResetType_e {
  F1AP_RESET_ALL,
  F1AP_RESET_PART_OF_F1_INTERFACE
} f1ap_ResetType_t;

typedef struct f1ap_ue_to_reset_t {
  uint32_t *gNB_CU_ue_id;
  uint32_t *gNB_DU_ue_id;
} f1ap_ue_to_reset_t;

typedef struct f1ap_reset_t {
  uint64_t          transaction_id;
  f1ap_Cause_t      cause;
  long              cause_value;
  f1ap_ResetType_t  reset_type;
  int num_ue_to_reset;
  f1ap_ue_to_reset_t *ue_to_reset; // array of num_ue_to_reset elements
} f1ap_reset_t;

typedef struct f1ap_reset_ack_t {
  uint64_t transaction_id;
  int num_ue_to_reset;
  f1ap_ue_to_reset_t *ue_to_reset; // array of num_ue_to_reset elements
  //uint16_t criticality_diagnostics; // not implemented as of now
} f1ap_reset_ack_t;

/* Structure of Position Information Transfer related NRPPA messages */
/* IE structures for Positioning related messages as per TS 38.473 V16.3.1*/

typedef enum f1ap_subcarrier_spacing_e {
  F1AP_SUBCARRIER_SPACING_15KHZ,
  F1AP_SUBCARRIER_SPACING_30KHZ,
  F1AP_SUBCARRIER_SPACING_60KHZ,
  F1AP_SUBCARRIER_SPACING_120KHZ
} f1ap_subcarrier_spacing_pr;

typedef struct f1ap_scs_specific_carrier_s {
  uint32_t offset_to_carrier;
  f1ap_subcarrier_spacing_pr subcarrier_spacing;
  uint16_t carrier_bandwidth;
} f1ap_scs_specific_carrier_t;

typedef struct f1ap_uplink_channel_bw_per_scs_list_s {
  f1ap_scs_specific_carrier_t *scs_specific_carrier;
  uint32_t scs_specific_carrier_list_length;
} f1ap_uplink_channel_bw_per_scs_list_t;

typedef enum f1ap_transmission_comb_e {
  F1AP_TRANSMISSION_COMB_PR_NOTHING,
  F1AP_TRANSMISSION_COMB_PR_N2,
  F1AP_TRANSMISSION_COMB_PR_N4
} f1ap_transmission_comb_pr;

typedef struct f1ap_transmission_comb_n2_s {
  uint8_t comb_offset_n2;
  uint8_t cyclic_shift_n2;
} f1ap_transmission_comb_n2_t, f1ap_transmission_comb_pos_n2_t;

typedef struct f1ap_transmission_comb_n4_s {
  uint8_t comb_offset_n4;
  uint8_t cyclic_shift_n4;
} f1ap_transmission_comb_n4_t, f1ap_transmission_comb_pos_n4_t;

typedef union f1ap_transmission_comb_c {
  f1ap_transmission_comb_n2_t n2;
  f1ap_transmission_comb_n4_t n4;
} f1ap_transmission_comb_u;

typedef struct f1ap_transmission_comb_s {
  f1ap_transmission_comb_pr present;
  f1ap_transmission_comb_u choice;
} f1ap_transmission_comb_t;

typedef enum f1ap_srs_resource_type_periodicity_e {
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT1 = 0,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT2,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT4,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT5,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT8,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT10,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT16,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT20,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT32,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT40,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT64,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT80,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT160,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT320,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT640,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT1280,
  F1AP_SRS_RESOURCE_TYPE_PERIODICITY_SLOT2560
} f1ap_srs_resource_type_periodicity_pr;

typedef struct f1ap_resource_type_periodic_s {
  f1ap_srs_resource_type_periodicity_pr periodicity;
  uint16_t offset;
} f1ap_resource_type_periodic_t, f1ap_resource_type_semi_persistent_t;

typedef union f1ap_resource_type_c {
  f1ap_resource_type_periodic_t periodic;
  f1ap_resource_type_semi_persistent_t semi_persistent;
  bool aperiodic;
} f1ap_resource_type_u;

typedef enum f1ap_resource_type_e {
  F1AP_RESOURCE_TYPE_PR_NOTHING,
  F1AP_RESOURCE_TYPE_PR_PERIODIC,
  F1AP_RESOURCE_TYPE_PR_SEMI_PERSISTENT,
  F1AP_RESOURCE_TYPE_PR_APERIODIC
} f1ap_resource_type_pr;

typedef struct f1ap_resource_type_s {
  f1ap_resource_type_pr present;
  f1ap_resource_type_u choice;
} f1ap_resource_type_t;

typedef struct f1ap_transmission_comb_n8_s {
  uint8_t comb_offset_n8;
  uint8_t cyclic_shift_n8;
} f1ap_transmission_comb_pos_n8_t;

typedef union f1ap_transmission_comb_pos_c {
  f1ap_transmission_comb_pos_n2_t n2;
  f1ap_transmission_comb_pos_n4_t n4;
  f1ap_transmission_comb_pos_n8_t n8;
} f1ap_transmission_comb_pos_u;

typedef enum f1ap_transmission_comb_pos_e {
  F1AP_TRANSMISSION_COMB_POS_PR_NOTHING,
  F1AP_TRANSMISSION_COMB_POS_PR_N2,
  F1AP_TRANSMISSION_COMB_POS_PR_N4,
  F1AP_TRANSMISSION_COMB_POS_PR_N8
} f1ap_transmission_comb_pos_pr;

typedef struct f1ap_transmission_comb_pos_s {
  f1ap_transmission_comb_pos_pr present;
  f1ap_transmission_comb_pos_u choice;
} f1ap_transmission_comb_pos_t;

typedef enum f1ap_srs_resource_type_pos_periodicity_e {
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT1 = 0,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT2,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT4,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT8,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT10,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT16,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT20,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT32,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT40,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT64,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT80,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT160,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT320,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT640,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT1280,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT2560,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT5120,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT10240,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT20480,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT40960,
  F1AP_SRS_RESOURCE_TYPE_POS_PERIODICITY_SLOT81920
} f1ap_srs_resource_type_pos_periodicity_pr;

typedef struct f1ap_resource_type_periodic_pos_s {
  f1ap_srs_resource_type_pos_periodicity_pr periodicity;
  uint16_t offset;
} f1ap_resource_type_periodic_pos_t, f1ap_resource_type_semi_persistent_pos_t;

typedef struct f1ap_resource_type_aperiodic_pos_s {
  uint8_t slot_offset;
} f1ap_resource_type_aperiodic_pos_t;

typedef union f1ap_resource_type_pos_c {
  f1ap_resource_type_periodic_pos_t periodic;
  f1ap_resource_type_semi_persistent_pos_t semi_persistent;
  f1ap_resource_type_aperiodic_pos_t aperiodic;
} f1ap_resource_type_pos_u;

typedef enum f1ap_resource_type_pos_e {
  F1AP_RESOURCE_TYPE_POS_PR_NOTHING,
  F1AP_RESOURCE_TYPE_POS_PR_PERIODIC,
  F1AP_RESOURCE_TYPE_POS_PR_SEMI_PERSISTENT,
  F1AP_RESOURCE_TYPE_POS_PR_APERIODIC
} f1ap_resource_type_pos_pr;

typedef struct f1ap_resource_type_pos_s {
  f1ap_resource_type_pos_pr present;
  f1ap_resource_type_pos_u choice;
} f1ap_resource_type_pos_t;

typedef struct f1ap_srs_resource_id_list_s {
  long *srs_resource_id;
  uint8_t srs_resource_id_list_length;
} f1ap_srs_resource_id_list_t;

typedef struct f1ap_resource_set_type_aperiodic_s {
  uint8_t srs_resource_trigger;
  long slot_offset;
} f1ap_resource_set_type_aperiodic_t;

typedef union f1ap_resource_set_type_c {
  bool periodic;
  bool semi_persistent;
  f1ap_resource_set_type_aperiodic_t aperiodic;
} f1ap_resource_set_type_u;

typedef enum f1ap_resource_set_type_e {
  F1AP_RESOURCE_SET_TYPE_PR_NOTHING,
  F1AP_RESOURCE_SET_TYPE_PR_PERIODIC,
  F1AP_RESOURCE_SET_TYPE_PR_SEMI_PERSISTENT,
  F1AP_RESOURCE_SET_TYPE_PR_APERIODIC
} f1ap_resource_set_type_pr;

typedef struct f1ap_resource_set_type_s {
  f1ap_resource_set_type_pr present;
  f1ap_resource_set_type_u choice;
} f1ap_resource_set_type_t;

typedef struct f1ap_pos_srs_resource_id_list_s {
  long *srs_pos_resource_id;
  uint32_t pos_srs_resource_id_list_length;
} f1ap_pos_srs_resource_id_list_t;

typedef union f1ap_pos_resource_set_type_c {
  bool periodic;
  bool semi_persistent;
  uint8_t srs_resource;
} f1ap_pos_resource_set_type_u;

typedef enum f1ap_pos_resource_set_type_e {
  F1AP_POS_RESOURCE_SET_TYPE_PR_NOTHING,
  F1AP_POS_RESOURCE_SET_TYPE_PR_PERIODIC,
  F1AP_POS_RESOURCE_SET_TYPE_PR_SEMI_PERSISTENT,
  F1AP_POS_RESOURCE_SET_TYPE_PR_APERIODIC
} f1ap_pos_resource_set_type_pr;

typedef struct f1ap_pos_resource_set_type_s {
  f1ap_pos_resource_set_type_pr present;
  f1ap_pos_resource_set_type_u choice;
} f1ap_pos_resource_set_type_t;

typedef enum f1ap_srs_resource_number_of_ports_e {
  F1AP_SRS_NUMBER_OF_PORTS_N1,
  F1AP_SRS_NUMBER_OF_PORTS_N2,
  F1AP_SRS_NUMBER_OF_PORTS_N4
} f1ap_srs_resource_number_of_ports_pr;

typedef enum f1ap_srs_resource_number_of_symbols_e {
  F1AP_SRS_NUMBER_OF_SYMBOLS_N1,
  F1AP_SRS_NUMBER_OF_SYMBOLS_N2,
  F1AP_SRS_NUMBER_OF_SYMBOLS_N4
} f1ap_srs_resource_number_of_symbols_pr;

typedef enum f1ap_srs_repetition_factor_e {
  F1AP_SRS_REPETITION_FACTOR_RF1,
  F1AP_SRS_REPETITION_FACTOR_RF2,
  F1AP_SRS_REPETITION_FACTOR_RF4
} f1ap_srs_repetition_factor_pr;

typedef enum f1ap_srs_group_or_sequencehopping_e {
  F1AP_GROUPORSEQUENCEHOPPING_NOTHING,
  F1AP_GROUPORSEQUENCEHOPPING_GROUPHOPPING,
  F1AP_GROUPORSEQUENCEHOPPING_SEQUENCEHOPPING
} f1ap_srs_group_or_sequencehopping_pr;

typedef struct f1ap_srs_resource_s {
  uint32_t srs_resource_id;
  f1ap_srs_resource_number_of_ports_pr nr_of_srs_ports;
  f1ap_transmission_comb_t transmission_comb;
  uint8_t start_position;
  f1ap_srs_resource_number_of_symbols_pr nr_of_symbols;
  f1ap_srs_repetition_factor_pr repetition_factor;
  uint8_t freq_domain_position;
  uint16_t freq_domain_shift;
  uint8_t c_srs;
  uint8_t b_srs;
  uint8_t b_hop;
  f1ap_srs_group_or_sequencehopping_pr group_or_sequence_hopping;
  f1ap_resource_type_t resource_type;
  uint16_t sequence_id;
} f1ap_srs_resource_t;

typedef enum f1ap_srs_resource_item_number_of_symbols_e {
  F1AP_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N1,
  F1AP_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N2,
  F1AP_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N4,
  F1AP_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N8,
  F1AP_SRS_RESOURCE_ITEM_NUMBER_OF_SYMBOLS_N12
} f1ap_srs_resource_item_number_of_symbols_pr;

typedef struct f1ap_pos_srs_resource_item_s {
  uint32_t srs_pos_resource_id;
  f1ap_transmission_comb_pos_t transmission_comb_pos;
  uint8_t start_position;
  f1ap_srs_resource_item_number_of_symbols_pr nr_of_symbols;
  uint16_t freq_domain_shift;
  uint8_t c_srs;
  f1ap_srs_group_or_sequencehopping_pr group_or_sequence_hopping;
  f1ap_resource_type_pos_t resource_type_pos;
  uint32_t sequence_id;
} f1ap_pos_srs_resource_item_t;

typedef struct f1ap_srs_resource_set_s {
  uint8_t srs_resource_set_id;
  f1ap_srs_resource_id_list_t srs_resource_id_list;
  f1ap_resource_set_type_t resource_set_type;
} f1ap_srs_resource_set_t;

typedef struct f1ap_pos_srs_resource_set_item_s {
  uint8_t pos_srs_resource_set_id;
  f1ap_pos_srs_resource_id_list_t pos_srs_resource_id_list;
  f1ap_pos_resource_set_type_t pos_resource_set_type;
} f1ap_pos_srs_resource_set_item_t;

typedef struct f1ap_srs_resource_list_s {
  f1ap_srs_resource_t *srs_resource;
  uint32_t srs_resource_list_length;
} f1ap_srs_resource_list_t;

typedef struct f1ap_pos_srs_resource_list_s {
  f1ap_pos_srs_resource_item_t *pos_srs_resource_item;
  uint32_t pos_srs_resource_list_length;
} f1ap_pos_srs_resource_list_t;

typedef struct f1ap_srs_resource_set_list_s {
  f1ap_srs_resource_set_t *srs_resource_set;
  uint32_t srs_resource_set_list_length;
} f1ap_srs_resource_set_list_t;

typedef struct f1ap_pos_srs_resource_set_list_s {
  f1ap_pos_srs_resource_set_item_t *pos_srs_resource_set_item;
  uint32_t pos_srs_resource_set_list_length;
} f1ap_pos_srs_resource_set_list_t;

typedef struct f1ap_srs_config_s {
  f1ap_srs_resource_list_t *srs_resource_list;
  f1ap_pos_srs_resource_list_t *pos_srs_resource_list;
  f1ap_srs_resource_set_list_t *srs_resource_set_list;
  f1ap_pos_srs_resource_set_list_t *pos_srs_resource_set_list;
} f1ap_srs_config_t;

typedef enum f1ap_cp_type_e { F1AP_CP_TYPE_NORMAL, F1AP_CP_TYPE_EXTENDED } f1ap_cp_type_pr;

typedef struct f1ap_active_ul_bwp_s {
  uint32_t location_and_bandwidth;
  f1ap_subcarrier_spacing_pr subcarrier_spacing;
  f1ap_cp_type_pr cyclic_prefix;
  uint32_t tx_direct_current_location;
  uint8_t shift_7_dot_5kHz;
  f1ap_srs_config_t srs_config;
} f1ap_active_ul_bwp_t;

typedef struct f1ap_srs_carrier_list_item_s {
  uint32_t pointA;
  f1ap_uplink_channel_bw_per_scs_list_t uplink_channel_bw_per_scs_list;
  f1ap_active_ul_bwp_t active_ul_bwp;
  uint16_t pci;
} f1ap_srs_carrier_list_item_t;

typedef struct f1ap_srs_carrier_list_s {
  f1ap_srs_carrier_list_item_t *srs_carrier_list_item;
  uint32_t srs_carrier_list_length;
} f1ap_srs_carrier_list_t;

// optional: IE 9.3.1.192 (TS 38.473 V16.21.0)
typedef struct f1ap_srs_configuration_s {
  f1ap_srs_carrier_list_t srs_carrier_list;
} f1ap_srs_configuration_t;

typedef union f1ap_srs_type_c {
  uint8_t *srs_resource_set_id;
  bool *aperiodic;
} f1ap_srs_type_u;

typedef enum f1ap_srs_type_e {
  F1AP_SRS_TYPE_PR_NOTHING,
  F1AP_SRS_TYPE_PR_SEMIPERSISTENTSRS,
  F1AP_SRS_TYPE_PR_APERIODICSRS
} f1ap_srs_type_pr;

typedef struct f1ap_srs_type_s {
  f1ap_srs_type_pr present;
  f1ap_srs_type_u choice;
} f1ap_srs_type_t;

typedef union f1ap_abort_transmission_c {
  uint8_t srs_resource_set_id;
  bool release_all;
} f1ap_abort_transmission_u;

typedef enum f1ap_abort_transmission_e {
  F1AP_ABORT_TRANSMISSION_PR_NOTHING,
  F1AP_ABORT_TRANSMISSION_PR_SRSRESOURCESETID,
  F1AP_ABORT_TRANSMISSION_PR_RELEASEALL
} f1ap_abort_transmission_pr;

typedef struct f1ap_abort_transmission_s {
  f1ap_abort_transmission_pr present;
  f1ap_abort_transmission_u choice;
} f1ap_abort_transmission_t;

typedef struct f1ap_trp_list_item_s {
  uint32_t trp_id;
} f1ap_trp_list_item_t;

typedef struct f1ap_trp_list_s {
  f1ap_trp_list_item_t *trp_list_item;
  uint32_t trp_list_length;
} f1ap_trp_list_t;

typedef enum f1ap_trp_information_type_item_e {
  F1AP_TRP_INFORMATION_TYPE_ITEM_NR_PCI,
  F1AP_TRP_INFORMATION_TYPE_ITEM_NG_RAN_CGI,
  F1AP_TRP_INFORMATION_TYPE_ITEM_NR_ARFCN,
  F1AP_TRP_INFORMATION_TYPE_ITEM_PRS_CONFIG,
  F1AP_TRP_INFORMATION_TYPE_ITEM_SSB_CONFIG,
  F1AP_TRP_INFORMATION_TYPE_ITEM_SFN_INIT_TIME,
  F1AP_TRP_INFORMATION_TYPE_ITEM_SPATIAL_DIRECTION_INFO,
  F1AP_TRP_INFORMATION_TYPE_ITEM_GEO_COORDINATES
} f1ap_trp_information_type_item_pr;

typedef struct f1ap_trp_information_type_list_s {
  f1ap_trp_information_type_item_pr *trp_information_type_item;
  uint8_t trp_information_type_list_length;
} f1ap_trp_information_type_list_t;

typedef struct f1ap_access_point_position_s {
  long latitude_sign;
  long latitude;
  long longitude;
  long direction_of_altitude;
  long altitude;
  long uncertainty_semi_major;
  long uncertainty_semi_minor;
  long orientation_of_major_axis;
  long uncertainty_altitude;
  long confidence;
} f1ap_access_point_position_t;

typedef struct f1ap_ngran_high_accuracy_access_point_position_s {
  long latitude;
  long longitude;
  long altitude;
  long uncertainty_semi_major;
  long uncertainty_semi_minor;
  long orientation_of_major_axis;
  long horizontal_confidence;
  long uncertainty_altitude;
  long vertical_confidence;
} f1ap_ngran_high_accuracy_access_point_position_t;

typedef union f1ap_trp_position_direct_accuracy_c {
  f1ap_access_point_position_t trp_position;
  f1ap_ngran_high_accuracy_access_point_position_t trp_HAposition;
} f1ap_trp_position_direct_accuracy_u;

typedef enum f1ap_trp_position_direct_accuracy_e {
  F1AP_TRP_POSITION_DIRECT_ACCURACY_PR_NOTHING,
  F1AP_TRP_POSITION_DIRECT_ACCURACY_PR_TRPPOSITION,
  F1AP_TRP_POSITION_DIRECT_ACCURACY_PR_TRPHAPOSITION
} f1ap_trp_position_direct_accuracy_pr;

typedef struct f1ap_trp_position_direct_accuracy_s {
  f1ap_trp_position_direct_accuracy_pr present;
  f1ap_trp_position_direct_accuracy_u choice;
} f1ap_trp_position_direct_accuracy_t;

typedef struct f1ap_trp_position_direct_s {
  f1ap_trp_position_direct_accuracy_t accuracy;
} f1ap_trp_position_direct_t;

typedef enum f1ap_reference_point_e {
  F1AP_REFERENCE_POINT_PR_NOTHING,
  F1AP_REFERENCE_POINT_PR_COORDINATEID,
  F1AP_REFERENCE_POINT_PR_REFERENCEPOINTCOORDINATE,
  F1AP_REFERENCE_POINT_PR_REFERENCEPOINTCOORDINATEHA
} f1ap_reference_point_pr;

typedef union f1ap_reference_point_c {
  long coordinate_id;
  f1ap_access_point_position_t reference_point_coordinate;
  f1ap_ngran_high_accuracy_access_point_position_t reference_point_coordinateHA;
} f1ap_reference_point_u;

typedef struct f1ap_reference_point_s {
  f1ap_reference_point_pr present;
  f1ap_reference_point_u choice;
} f1ap_reference_point_t;

typedef struct f1ap_location_uncertainty_s {
  long horizontal_uncertainty;
  long horizontal_confidence;
  long vertical_uncertainty;
  long vertical_confidence;
} f1ap_location_uncertainty_t;

typedef struct f1ap_relative_geodetic_location_s {
  long milli_arc_second_units;
  long height_units;
  long delta_latitude;
  long delta_longitude;
  long delta_height;
  f1ap_location_uncertainty_t location_uncertainty;
} f1ap_relative_geodetic_location_t;

typedef struct f1ap_relative_cartesian_location_s {
  long xyz_unit;
  long xvalue;
  long yvalue;
  long zvalue;
  f1ap_location_uncertainty_t location_uncertainty;
} f1ap_relative_cartesian_location_t;

typedef union f1ap_trp_reference_point_type_c {
  f1ap_relative_geodetic_location_t trp_position_relative_geodetic;
  f1ap_relative_cartesian_location_t trp_position_relative_cartesian;
} f1ap_trp_reference_point_type_u;

typedef enum f1ap_trp_reference_point_type_e {
  F1AP_TRP_REFERENCE_POINT_TYPE_PR_NOTHING,
  F1AP_TRP_REFERENCE_POINT_TYPE_PR_TRPPOSITION_RELATIVE_GEODETIC,
  F1AP_TRP_REFERENCE_POINT_TYPE_PR_TRPPOSITION_RELATIVE_CARTESIAN
} f1ap_trp_reference_point_type_pr;

typedef struct f1ap_trp_reference_point_type_t {
  f1ap_trp_reference_point_type_pr present;
  f1ap_trp_reference_point_type_u choice;
} f1ap_trp_reference_point_type_t;

typedef struct f1ap_trp_position_referenced_t {
  f1ap_reference_point_t reference_point;
  f1ap_trp_reference_point_type_t reference_point_type;
} f1ap_trp_position_referenced_t;

typedef union f1ap_trp_position_definition_type_c {
  f1ap_trp_position_direct_t direct;
  f1ap_trp_position_referenced_t referenced;
} f1ap_trp_position_definition_type_u;

typedef enum f1ap_trp_position_definition_type_e {
  F1AP_TRP_POSITION_DEFINITION_TYPE_PR_NOTHING,
  F1AP_TRP_POSITION_DEFINITION_TYPE_PR_DIRECT,
  F1AP_TRP_POSITION_DEFINITION_TYPE_PR_REFERENCED
} f1ap_trp_position_definition_type_pr;

typedef struct f1ap_trp_position_definition_type_s {
  f1ap_trp_position_definition_type_u choice;
  f1ap_trp_position_definition_type_pr present;
} f1ap_trp_position_definition_type_t;

typedef struct f1ap_geographical_coordinates_s {
  f1ap_trp_position_definition_type_t trp_position_definition_type;
} f1ap_geographical_coordinates_t;

typedef struct f1ap_ng_ran_cgi_s {
  plmn_id_t plmn;
  uint64_t nr_cellid;
} f1ap_ng_ran_cgi_t;

typedef union f1ap_trp_information_type_response_item_c {
  uint16_t pci_nr;
  f1ap_ng_ran_cgi_t ng_ran_cgi;
  uint32_t nr_arfcn;
  // f1ap_prs_configuration_t pRSConfiguration;
  // f1ap_ssb_information_t sSBinformation;
  // bit_string_t sFNInitialisationTime;
  // f1ap_spatial_direction_information_t spatialDirectionInformation;
  f1ap_geographical_coordinates_t geographical_coordinates;
} f1ap_trp_information_type_response_item_u;

typedef enum f1ap_trp_information_type_response_item_e {
  F1AP_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_NOTHING,
  F1AP_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_PCI_NR,
  F1AP_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_NG_RAN_CGI,
  F1AP_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_NRARFCN,
  F1AP_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_PRSCONFIGURATION,
  F1AP_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_SSBINFORMATION,
  F1AP_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_SFNINITIALISATIONTIME,
  F1AP_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_SPATIALDIRECTIONINFORMATION,
  F1AP_TRP_INFORMATION_TYPE_RESPONSE_ITEM_PR_GEOGRAPHICALCOORDINATES
} f1ap_trp_information_type_response_item_pr;

typedef struct f1ap_trp_information_type_response_item_s {
  f1ap_trp_information_type_response_item_pr present;
  f1ap_trp_information_type_response_item_u choice;
} f1ap_trp_information_type_response_item_t;

typedef struct f1ap_trp_information_type_response_list_s {
  f1ap_trp_information_type_response_item_t *trp_information_type_response_item;
  uint8_t trp_information_type_response_item_length;
} f1ap_trp_information_type_response_list_t;

typedef struct f1ap_trp_information_s {
  uint32_t trp_id;
  f1ap_trp_information_type_response_list_t trp_information_type_response_list;
} f1ap_trp_information_t;

// IE 9.3.1.176 (TS 38.473 V16.21.0)
typedef struct f1ap_trp_information_list_s {
  f1ap_trp_information_t *trp_information_item;
  uint32_t trp_information_item_length;
} f1ap_trp_information_list_t;

typedef struct f1ap_trp_measurement_request_item_s {
  uint32_t tRPID;
} f1ap_trp_measurement_request_item_t;

typedef struct f1ap_trp_measurement_request_list_s {
  f1ap_trp_measurement_request_item_t *trp_measurement_request_item;
  uint32_t trp_measurement_request_list_length;
} f1ap_trp_measurement_request_list_t;

typedef enum f1ap_PosMeasurementType_e {
  F1AP_POSMEASUREMENTTYPE_GNB_RX_TX = 0,
  F1AP_POSMEASUREMENTTYPE_UL_SRS_RSRP = 1,
  F1AP_POSMEASUREMENTTYPE_UL_AOA = 2,
  F1AP_POSMEASUREMENTTYPE_UL_RTOA = 3
} f1ap_PosMeasurementType_e;

typedef struct f1ap_pos_measurement_quantities_item_s {
  f1ap_PosMeasurementType_e pos_measurement_type;
} f1ap_pos_measurement_quantities_item_t;

typedef struct f1ap_pos_measurement_quantities_s {
  f1ap_pos_measurement_quantities_item_t *pos_measurement_quantities_item;
  uint32_t pos_measurement_quantities_length;
} f1ap_pos_measurement_quantities_t;

typedef enum f1ap_pos_measurement_periodicity_e {
  F1AP_POSMEASUREMENTPERIODICITY_MS120 = 0,
  F1AP_POSMEASUREMENTPERIODICITY_MS240 = 1,
  F1AP_POSMEASUREMENTPERIODICITY_MS480 = 2,
  F1AP_POSMEASUREMENTPERIODICITY_MS640 = 3,
  F1AP_POSMEASUREMENTPERIODICITY_MS1024 = 4,
  F1AP_POSMEASUREMENTPERIODICITY_MS2048 = 5,
  F1AP_POSMEASUREMENTPERIODICITY_MS5120 = 6,
  F1AP_POSMEASUREMENTPERIODICITY_MS10240 = 7,
  F1AP_POSMEASUREMENTPERIODICITY_MIN1 = 8,
  F1AP_POSMEASUREMENTPERIODICITY_MIN6 = 9,
  F1AP_POSMEASUREMENTPERIODICITY_MIN12 = 10,
  F1AP_POSMEASUREMENTPERIODICITY_MIN30 = 11,
  F1AP_POSMEASUREMENTPERIODICITY_MIN60 = 12
} f1ap_pos_measurement_periodicity_pr;

typedef enum f1ap_pos_report_characteristics_e {
  F1AP_POSREPORTCHARACTERISTICS_ONDEMAND = 0,
  F1AP_POSREPORTCHARACTERISTICS_PERIODIC = 1
} f1ap_pos_report_characteristics_pr;

typedef struct f1ap_lcs_to_gcs_translationaoa_c {
  uint16_t alpha;
  uint16_t beta;
  uint16_t gamma;
} f1ap_lcs_to_gcs_translationaoa_t;

typedef struct f1ap_ul_aoa_s {
  uint16_t azimuth_aoa;
  uint16_t *zenith_aoa;
  f1ap_lcs_to_gcs_translationaoa_t *lcs_to_gcs_translation_aoa;
} f1ap_ul_aoa_t;

typedef union f1ap_relative_path_delay_c {
  uint32_t k0;
  uint32_t k1;
  uint32_t k2;
  uint32_t k3;
  uint32_t k4;
  uint32_t k5;
} f1ap_relative_path_delay_u, f1ap_ul_rtoa_measurement_item_u, f1ap_gnb_rx_tx_time_diff_meas_u;

typedef enum f1ap_gnb_rx_tx_time_diff_meas_e {
  F1AP_GNBRXTXTIMEDIFFMEAS_PR_NOTHING,
  F1AP_GNBRXTXTIMEDIFFMEAS_PR_K0,
  F1AP_GNBRXTXTIMEDIFFMEAS_PR_K1,
  F1AP_GNBRXTXTIMEDIFFMEAS_PR_K2,
  F1AP_GNBRXTXTIMEDIFFMEAS_PR_K3,
  F1AP_GNBRXTXTIMEDIFFMEAS_PR_K4,
  F1AP_GNBRXTXTIMEDIFFMEAS_PR_K5
} f1ap_gnb_rx_tx_time_diff_meas_pr;

typedef struct f1ap_gnb_rx_tx_time_diff_meas_s {
  f1ap_gnb_rx_tx_time_diff_meas_pr present;
  f1ap_gnb_rx_tx_time_diff_meas_u choice;
} f1ap_gnb_rx_tx_time_diff_meas_t;

typedef enum f1ap_ul_rtoa_measurement_item_e {
  F1AP_ULRTOAMEAS_PR_NOTHING,
  F1AP_ULRTOAMEAS_PR_K0,
  F1AP_ULRTOAMEAS_PR_K1,
  F1AP_ULRTOAMEAS_PR_K2,
  F1AP_ULRTOAMEAS_PR_K3,
  F1AP_ULRTOAMEAS_PR_K4,
  F1AP_ULRTOAMEAS_PR_K5
} f1ap_ul_rtoa_measurement_item_pr;

typedef struct f1ap_ul_rtoa_measurement_item_s {
  f1ap_ul_rtoa_measurement_item_pr present;
  f1ap_ul_rtoa_measurement_item_u choice;
} f1ap_ul_rtoa_measurement_item_t;

typedef union f1ap_ul_rtoa_measurement_s {
  f1ap_ul_rtoa_measurement_item_t ul_rtoa_measurement_item;
} f1ap_ul_rtoa_measurement_t;

typedef struct f1ap_gnb_rx_tx_time_diff_s {
  f1ap_gnb_rx_tx_time_diff_meas_t rx_tx_time_diff;
} f1ap_gnb_rx_tx_time_diff_t;

typedef union f1ap_measured_results_value_c {
  f1ap_ul_aoa_t ul_angle_of_arrival;
  uint8_t ul_srs_rsrp;
  f1ap_ul_rtoa_measurement_t ul_rtoa;
  f1ap_gnb_rx_tx_time_diff_t gnb_rx_tx_time_diff;
} f1ap_measured_results_value_u;

typedef enum f1ap_measured_results_value_e {
  F1AP_MEASURED_RESULTS_VALUE_PR_NOTHING,
  F1AP_MEASURED_RESULTS_VALUE_PR_UL_ANGLEOFARRIVAL,
  F1AP_MEASURED_RESULTS_VALUE_PR_UL_SRS_RSRP,
  F1AP_MEASURED_RESULTS_VALUE_PR_UL_RTOA,
  F1AP_MEASURED_RESULTS_VALUE_PR_GNB_RXTXTIMEDIFF
} f1ap_measured_results_value_pr;

typedef struct f1ap_measured_results_value_s {
  f1ap_measured_results_value_pr present;
  f1ap_measured_results_value_u choice;
} f1ap_measured_results_value_t;

typedef union f1ap_time_stamp_slot_index_c {
  uint8_t scs_15;
  uint8_t scs_30;
  uint8_t scs_60;
  uint8_t scs_120;
} f1ap_time_stamp_slot_index_u;

typedef enum f1ap_time_stamp_slot_index_e {
  F1AP_TIME_STAMP_SLOT_INDEX_PR_NOTHING,
  F1AP_TIME_STAMP_SLOT_INDEX_PR_SCS_15,
  F1AP_TIME_STAMP_SLOT_INDEX_PR_SCS_30,
  F1AP_TIME_STAMP_SLOT_INDEX_PR_SCS_60,
  F1AP_TIME_STAMP_SLOT_INDEX_PR_SCS_120
} f1ap_time_stamp_slot_index_pr;

typedef struct f1ap_time_stamp_slot_index_s {
  f1ap_time_stamp_slot_index_pr present;
  f1ap_time_stamp_slot_index_u choice;
} f1ap_time_stamp_slot_index_t;

typedef struct f1ap_time_stamp_s {
  uint16_t system_frame_number;
  f1ap_time_stamp_slot_index_t slot_index;
} f1ap_time_stamp_t;

typedef struct f1ap_pos_measurement_result_item_s {
  f1ap_measured_results_value_t measured_results_value;
  f1ap_time_stamp_t time_stamp;
} f1ap_pos_measurement_result_item_t;

typedef struct f1ap_pos_measurement_result_s {
  f1ap_pos_measurement_result_item_t *pos_measurement_result_item;
  uint32_t pos_measurement_result_item_length;
} f1ap_pos_measurement_result_t;

typedef struct f1ap_pos_measurement_result_list_item_s {
  f1ap_pos_measurement_result_t pos_measurement_result;
  uint32_t trp_id;
} f1ap_pos_measurement_result_list_item_t;

typedef struct f1ap_pos_measurement_result_list_s {
  f1ap_pos_measurement_result_list_item_t *pos_measurement_result_list_item;
  uint32_t pos_measurement_result_list_length;
} f1ap_pos_measurement_result_list_t;

typedef struct f1ap_positioning_information_req_s {
  // IE 9.3.1.4 (mandatory)
  uint32_t gNB_CU_ue_id;
  // IE 9.3.1.5 (mandatory)
  uint32_t gNB_DU_ue_id;
} f1ap_positioning_information_req_t;

typedef struct f1ap_positioning_information_resp_s {
  // IE 9.3.1.4 (mandatory)
  uint32_t gNB_CU_ue_id;
  // IE 9.3.1.5 (mandatory)
  uint32_t gNB_DU_ue_id;
  // IE 9.3.1.192 (optional)
  f1ap_srs_configuration_t *srs_configuration;
} f1ap_positioning_information_resp_t;

typedef struct f1ap_positioning_information_failure_s {
  // IE 9.3.1.4 (mandatory)
  uint32_t gNB_CU_ue_id;
  // IE 9.3.1.5 (mandatory)
  uint32_t gNB_DU_ue_id;
  // IE 9.3.1.2 (mandatory)
  f1ap_Cause_t cause;
  // IE 9.3.1.2 (mandatory)
  long cause_value;
} f1ap_positioning_information_failure_t;

typedef struct f1ap_positioning_activation_req_s {
  // IE 9.3.1.4 (mandatory)
  uint32_t gNB_CU_ue_id;
  // IE 9.3.1.5 (mandatory)
  uint32_t gNB_DU_ue_id;
  // (mandatory)
  f1ap_srs_type_t srs_type;
} f1ap_positioning_activation_req_t;

typedef struct f1ap_positioning_activation_resp_s {
  // IE 9.3.1.4 (mandatory)
  uint32_t gNB_CU_ue_id;
  // IE 9.3.1.5 (mandatory)
  uint32_t gNB_DU_ue_id;
} f1ap_positioning_activation_resp_t;

typedef struct f1ap_positioning_activation_failure_s {
  // IE 9.3.1.4 (mandatory)
  uint32_t gNB_CU_ue_id;
  // IE 9.3.1.5 (mandatory)
  uint32_t gNB_DU_ue_id;
  // IE 9.3.1.2 (mandatory)
  f1ap_Cause_t cause;
  // IE 9.3.1.2 (mandatory)
  long cause_value;
} f1ap_positioning_activation_failure_t;

typedef struct f1ap_positioning_deactivation_s {
  // IE 9.3.1.4 (mandatory)
  uint32_t gNB_CU_ue_id;
  // IE 9.3.1.5 (mandatory)
  uint32_t gNB_DU_ue_id;
  // (mandatory)
  f1ap_abort_transmission_t abort_transmission;
} f1ap_positioning_deactivation_t;

typedef struct f1ap_positioning_information_update_s {
  // IE 9.3.1.4 (mandatory)
  uint32_t gNB_CU_ue_id;
  // IE 9.3.1.5 (mandatory)
  uint32_t gNB_DU_ue_id;
  // IE 9.3.1.192 (optional)
  f1ap_srs_configuration_t *srs_configuration;
} f1ap_positioning_information_update_t;

typedef struct f1ap_trp_information_req_s {
  // IE 9.3.1.23 (mandatory)
  uint8_t transaction_id;
  bool has_trp_list;
  // mandatory
  f1ap_trp_list_t trp_list;
  // mandatory
  f1ap_trp_information_type_list_t trp_information_type_list;
} f1ap_trp_information_req_t;

typedef struct f1ap_trp_information_resp_s {
  // IE 9.3.1.23 (mandatory)
  uint8_t transaction_id;
  // mandatory
  f1ap_trp_information_list_t trp_information_list;
} f1ap_trp_information_resp_t;

typedef struct f1ap_trp_information_failure_s {
  // IE 9.3.1.23 (mandatory)
  uint8_t transaction_id;
  // IE 9.3.1.2 (mandatory)
  f1ap_Cause_t cause;
  // IE 9.3.1.2 (mandatory)
  long cause_value;
} f1ap_trp_information_failure_t;

typedef struct f1ap_measurement_req_s {
  // IE 9.3.1.23 (mandatory)
  uint8_t transaction_id;
  // (mandatory)
  uint16_t lmf_measurement_id;
  // (mandatory)
  uint16_t ran_measurement_id;
  // (mandatory)
  f1ap_trp_measurement_request_list_t trp_measurement_request_list;
  // (mandatory) ondemand = 0, periodic = 1
  f1ap_pos_report_characteristics_pr pos_report_characteristics;
  // if report characteristics periodic
  f1ap_pos_measurement_periodicity_pr measurement_periodicity;
  // (mandatory)
  f1ap_pos_measurement_quantities_t pos_measurement_quantities;
  // IE 9.3.1.192 (optional)
  f1ap_srs_configuration_t *srs_configuration;
} f1ap_positioning_measurement_req_t;

typedef struct f1ap_positioning_measurement_resp_s {
  // IE 9.3.1.23 (mandatory)
  uint8_t transaction_id;
  // (mandatory)
  uint16_t lmf_measurement_id;
  // (mandatory)
  uint16_t ran_measurement_id;
  // (mandatory)
  f1ap_pos_measurement_result_list_t *pos_measurement_result_list;
} f1ap_positioning_measurement_resp_t;

typedef struct f1ap_positioning_measurement_failure_s {
  // IE 9.3.1.23 (mandatory)
  uint8_t transaction_id;
  // (mandatory)
  uint16_t lmf_measurement_id;
  // (mandatory)
  uint16_t ran_measurement_id;
  // IE 9.3.1.2 (mandatory)
  f1ap_Cause_t cause;
  // IE 9.3.1.2 (mandatory)
  long cause_value;
} f1ap_positioning_measurement_failure_t;

typedef struct f1ap_positioning_measurement_report_s {
  // IE 9.3.1.23 (mandatory)
  uint8_t transaction_id;
  // (mandatory)
  uint16_t lmf_measurement_id;
  // (mandatory)
  uint16_t ran_measurement_id;
  // (mandatory)
  f1ap_pos_measurement_result_list_t *pos_measurement_result_list;
} f1ap_positioning_measurement_report_t;

typedef struct f1ap_positioning_measurement_abort_s {
  // IE 9.3.1.23 (mandatory)
  uint8_t transaction_id;
  // (mandatory)
  uint16_t lmf_measurement_id;
  // (mandatory)
  uint16_t ran_measurement_id;
} f1ap_positioning_measurement_abort_t;

typedef struct f1ap_positioning_measurement_failure_indication_s {
  // IE 9.3.1.23 (mandatory)
  uint8_t transaction_id;
  // (mandatory)
  uint16_t lmf_measurement_id;
  // (mandatory)
  uint16_t ran_measurement_id;
  // IE 9.3.1.2 (mandatory)
  f1ap_Cause_t cause;
  // IE 9.3.1.2 (mandatory)
  long cause_value;
} f1ap_positioning_measurement_failure_indication_t;

typedef struct f1ap_positioning_measurement_update_s {
  // IE 9.3.1.23 (mandatory)
  uint8_t transaction_id;
  // (mandatory)
  uint16_t lmf_measurement_id;
  // (mandatory)
  uint16_t ran_measurement_id;
  // IE 9.3.1.192 (optional)
  f1ap_srs_configuration_t *srs_configuration;
} f1ap_positioning_measurement_update_t;

#endif /* F1AP_MESSAGES_TYPES_H_ */
