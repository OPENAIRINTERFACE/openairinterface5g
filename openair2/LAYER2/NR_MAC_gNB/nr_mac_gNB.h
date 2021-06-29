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

/*! \file mac.h
* \brief MAC data structures, constant, and function prototype
* \author Navid Nikaein and Raymond Knopp, WIE-TAI CHEN
* \date 2011, 2018
* \version 0.5
* \company Eurecom, NTUST
* \email navid.nikaein@eurecom.fr, kroempa@gmail.com

*/
/** @defgroup _oai2  openair2 Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

/*@}*/

#ifndef __LAYER2_NR_MAC_GNB_H__
#define __LAYER2_NR_MAC_GNB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Commmon */
#include "targets/ARCH/COMMON/common_lib.h"
#include "COMMON/platform_constants.h"
#include "common/ran_context.h"

/* RRC */
#include "NR_BCCH-BCH-Message.h"
#include "NR_CellGroupConfig.h"
#include "NR_ServingCellConfigCommon.h"
#include "NR_MeasConfig.h"

/* PHY */
#include "PHY/defs_gNB.h"
#include "PHY/TOOLS/time_meas.h"

/* Interface */
#include "nfapi_nr_interface_scf.h"
#include "NR_PHY_INTERFACE/NR_IF_Module.h"

/* MAC */
#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "NR_TAG.h"

#include <openair3/UICC/usim_interface.h>


/* Defs */
#define MAX_NUM_BWP 2
#define MAX_NUM_CORESET 2
#define MAX_NUM_CCE 90
#define MAX_HARQ_ROUNDS 4
/*!\brief Maximum number of random access process */
#define NR_NB_RA_PROC_MAX 4
#define MAX_NUM_OF_SSB 64

/*! \brief NR_list_t is a "list" (of users, HARQ processes, slices, ...).
 * Especially useful in the scheduler and to keep "classes" of users. */
typedef struct {
  int head;
  int *next;
  int tail;
  int len;
} NR_list_t;

typedef enum {
  RA_IDLE = 0,
  Msg2 = 1,
  WAIT_Msg3 = 2,
  Msg4 = 3,
  WAIT_Msg4_ACK = 4
} RA_gNB_state_t;

typedef struct NR_preamble_ue {
  uint8_t num_preambles;
  uint8_t *preamble_list;
} NR_preamble_ue_t;

/*! \brief gNB template for the Random access information */
typedef struct {
  /// Flag to indicate this process is active
  RA_gNB_state_t state;
  /// BWP id of RA process
  int bwp_id;
  /// CORESET0 configured flag
  int coreset0_configured;
  /// Slot where preamble was received
  uint8_t preamble_slot;
  /// Subframe where Msg2 is to be sent
  uint8_t Msg2_slot;
  /// Frame where Msg2 is to be sent
  frame_t Msg2_frame;
  /// Subframe where Msg3 is to be sent
  sub_frame_t Msg3_slot;
  /// Frame where Msg3 is to be sent
  frame_t Msg3_frame;
  /// Msg3 time domain allocation index
  uint8_t Msg3_tda_id;
  /// Subframe where Msg4 is to be sent
  sub_frame_t Msg4_slot;
  /// Frame where Msg4 is to be sent
  frame_t Msg4_frame;
  /// harq_pid used for Msg4 transmission
  uint8_t harq_pid;
  /// UE RNTI allocated during RAR
  rnti_t rnti;
  /// RA RNTI allocated from received PRACH
  uint16_t RA_rnti;
  /// Received preamble_index
  uint8_t preamble_index;
  /// Received UE Contention Resolution Identifier
  uint8_t cont_res_id[6];
  /// Timing offset indicated by PHY
  int16_t timing_offset;
  /// Timeout for RRC connection
  int16_t RRC_timer;
  /// Msg3 first RB
  uint8_t msg3_first_rb;
  /// Msg3 number of RB
  uint8_t msg3_nb_rb;
  /// Msg3 TPC command
  uint8_t msg3_TPC;
  /// Msg3 ULdelay command
  uint8_t msg3_ULdelay;
  /// Msg3 cqireq command
  uint8_t msg3_cqireq;
  /// Round of Msg3 HARQ
  uint8_t msg3_round;
  /// TBS used for Msg4
  int msg4_TBsize;
  /// MCS used for Msg4
  int msg4_mcs;
  /// RA search space
  NR_SearchSpace_t *ra_ss;
  // Beam index
  uint8_t beam_id;
  /// secondaryCellGroup for UE in NSA that is to come
  NR_CellGroupConfig_t *secondaryCellGroup;
  /// Preambles for contention-free access
  NR_preamble_ue_t preambles;
  /// NSA: the UEs C-RNTI to use
  rnti_t crnti;
  /// CFRA flag
  bool cfra;
} NR_RA_t;

/*! \brief gNB common channels */
typedef struct {
  int physCellId;
  int p_gNB;
  int Ncp;
  int nr_band;
  lte_frame_type_t frame_type;
  uint64_t dl_CarrierFreq;
  NR_BCCH_BCH_Message_t *mib;
  NR_ServingCellConfigCommon_t *ServingCellConfigCommon;
  NR_ARFCN_ValueEUTRA_t ul_CarrierFreq;
  long ul_Bandwidth;
  /// Outgoing MIB PDU for PHY
  MIB_PDU MIB_pdu;
  /// Outgoing BCCH pdu for PHY
  BCCH_PDU BCCH_pdu;
  /// Outgoing BCCH DCI allocation
  uint32_t BCCH_alloc_pdu;
  /// Outgoing CCCH pdu for PHY
  CCCH_PDU CCCH_pdu;
  /// Outgoing PCCH DCI allocation
  uint32_t PCCH_alloc_pdu;
  /// Outgoing PCCH pdu for PHY
  PCCH_PDU PCCH_pdu;
  /// Template for RA computations
  NR_RA_t ra[NR_NB_RA_PROC_MAX];
  /// VRB map for common channels
  uint16_t vrb_map[275];
  /// VRB map for common channels and PUSCH, dynamically allocated because
  /// length depends on number of slots and RBs
  uint16_t *vrb_map_UL;
  /// number of subframe allocation pattern available for MBSFN sync area
  uint8_t num_sf_allocation_pattern;
  ///Number of active SSBs
  uint8_t num_active_ssb;
  //Total available prach occasions per configuration period
  uint32_t total_prach_occasions_per_config_period;
  //Total available prach occasions
  uint32_t total_prach_occasions;
  //Max Association period
  uint8_t max_association_period;
  //SSB index
  uint8_t ssb_index[MAX_NUM_OF_SSB];
  //CB preambles for each SSB
  uint8_t cb_preambles_per_ssb;
} NR_COMMON_channels_t;


// SP ZP CSI-RS Resource Set Activation/Deactivation MAC CE
typedef struct sp_zp_csirs {
  bool is_scheduled;     //ZP CSI-RS ACT/Deact MAC CE is scheduled
  bool act_deact;        //Activation/Deactivation indication
  uint8_t serv_cell_id;  //Identity of Serving cell for which MAC CE applies
  uint8_t bwpid;         //Downlink BWP id
  uint8_t rsc_id;        //SP ZP CSI-RS resource set
} sp_zp_csirs_t;

//SP CSI-RS / CSI-IM Resource Set Activation/Deactivation MAC CE
#define MAX_CSI_RESOURCE_SET 64
typedef struct csi_rs_im {
  bool is_scheduled;
  bool act_deact;
  uint8_t serv_cellid;
  uint8_t bwp_id;
  bool im;
  uint8_t csi_im_rsc_id;
  uint8_t nzp_csi_rsc_id;
  uint8_t nb_tci_resource_set_id;
  uint8_t tci_state_id [ MAX_CSI_RESOURCE_SET ];
} csi_rs_im_t;

typedef struct pdcchStateInd {
  bool is_scheduled;
  uint8_t servingCellId;
  uint8_t coresetId;
  uint8_t tciStateId;
} pdcchStateInd_t;

typedef struct SPCSIReportingpucch {
  bool is_scheduled;
  uint8_t servingCellId;
  uint8_t bwpId;
  bool s0tos3_actDeact[4];
} SPCSIReportingpucch_t;

#define MAX_APERIODIC_TRIGGER_STATES 128 //38.331                               
typedef struct aperiodicCSI_triggerStateSelection {
  bool is_scheduled;
  uint8_t servingCellId;
  uint8_t bwpId;
  uint8_t highestTriggerStateSelected;
  bool triggerStateSelection[MAX_APERIODIC_TRIGGER_STATES];
} aperiodicCSI_triggerStateSelection_t;

#define MAX_TCI_STATES 128 //38.331                                             
typedef struct pdschTciStatesActDeact {
  bool is_scheduled;
  uint8_t servingCellId;
  uint8_t bwpId;
  uint8_t highestTciStateActivated;
  bool tciStateActDeact[MAX_TCI_STATES];
} pdschTciStatesActDeact_t;

typedef struct UE_info {
  sp_zp_csirs_t sp_zp_csi_rs;
  csi_rs_im_t csi_im;
  pdcchStateInd_t pdcch_state_ind;
  SPCSIReportingpucch_t SP_CSI_reporting_pucch;
  aperiodicCSI_triggerStateSelection_t aperi_CSI_trigger;
  pdschTciStatesActDeact_t pdsch_TCI_States_ActDeact;
} NR_UE_mac_ce_ctrl_t;


typedef struct NR_sched_pucch {
  int frame;
  int ul_slot;
  bool sr_flag;
  int csi_bits;
  bool simultaneous_harqcsi;
  uint8_t dai_c;
  uint8_t timing_indicator;
  uint8_t resource_indicator;
} NR_sched_pucch_t;

/* this struct is a helper: as long as the TDA and DCI format remain the same
 * over the same uBWP and search space, there is no need to recalculate all
 * S/L, MCS table, or DMRS-related parameters over and over again. Hence, we
 * store them in this struct for easy reference. */
typedef struct NR_sched_pusch_save {
  int dci_format;
  int time_domain_allocation;
  uint8_t num_dmrs_cdm_grps_no_data;

  int startSymbolIndex;
  int nrOfSymbols;

  NR_PUSCH_Config_t *pusch_Config;
  uint8_t transform_precoding;
  uint8_t mcs_table;

  long mapping_type;
  NR_DMRS_UplinkConfig_t *NR_DMRS_UplinkConfig;
  uint16_t dmrs_config_type;
  uint16_t ul_dmrs_symb_pos;
  uint8_t num_dmrs_symb;
  uint8_t N_PRB_DMRS;
} NR_sched_pusch_save_t;

typedef struct NR_sched_pusch {
  int frame;
  int slot;

  /// RB allocation within active uBWP
  uint16_t rbSize;
  uint16_t rbStart;

  // time-domain allocation for scheduled RBs
  int time_domain_allocation;

  /// MCS
  uint8_t mcs;

  /// TBS-related info
  uint16_t R;
  uint8_t Qm;
  uint32_t tb_size;

  /// UL HARQ PID to use for this UE, or -1 for "any new"
  int8_t ul_harq_pid;
} NR_sched_pusch_t;

typedef struct NR_UE_harq {
  bool is_waiting;
  uint8_t ndi;
  uint8_t round;
  uint16_t feedback_slot;

  /* Transport block to be sent using this HARQ process */
  uint32_t tb[16384];
  uint32_t tb_size;
} NR_UE_harq_t;

typedef struct NR_UE_old_sched {
  uint16_t rbSize;
  int time_domain_allocation;
  uint8_t mcsTableIdx;
  uint8_t mcs;
  uint8_t numDmrsCdmGrpsNoData;
} NR_UE_ret_info_t;

typedef enum {
  INACTIVE = 0,
  ACTIVE_NOT_SCHED,
  ACTIVE_SCHED
} NR_UL_harq_states_t;

typedef struct NR_UE_ul_harq {
  bool is_waiting;
  uint8_t ndi;
  uint8_t round;
  uint16_t feedback_slot;

  /// sched_pusch keeps information on MCS etc used for the initial transmission
  NR_sched_pusch_t sched_pusch;
} NR_UE_ul_harq_t;


typedef struct {
  uint8_t nb_ssbri_cri;
  uint8_t cri_ssbri_bitlen;
  uint8_t rsrp_bitlen;
  uint8_t diff_rsrp_bitlen;
}CRI_SSBRI_RSRP_bitlen_t;


#define MAX_CSI_RESOURCE_SET_IN_CSI_RESOURCE_CONFIG 16
typedef struct nr_csi_report {
  NR_CSI_ReportConfig__reportQuantity_PR reportQuantity_type;
  NR_CSI_ResourceConfig__csi_RS_ResourceSetList_PR CSI_Resource_type;
  uint8_t nb_of_nzp_csi_report;
  uint8_t nb_of_csi_ssb_report;
  CRI_SSBRI_RSRP_bitlen_t CSI_report_bitlen[MAX_CSI_RESOURCE_SET_IN_CSI_RESOURCE_CONFIG];
} nr_csi_report_t;


/*! \brief scheduling control information set through an API */
typedef struct {
  /// total amount of data awaiting for this UE
  uint32_t num_total_bytes;
  /// per-LC status data
  mac_rlc_status_resp_t rlc_status[MAX_NUM_LCID];

  /// the currently active BWP in DL
  NR_BWP_Downlink_t *active_bwp;
  /// the currently active BWP in UL
  NR_BWP_Uplink_t *active_ubwp;

  /// PUCCH scheduling information. Array of three, we assume for the moment:
  /// HARQ in the first field, SR in second, CSI in third (as fixed by RRC
  /// conf., i.e. if actually present).  The order is important for
  /// nr_acknack_scheduling()!
  NR_sched_pucch_t sched_pucch[3];

  NR_sched_pusch_save_t pusch_save;
  NR_sched_pusch_t sched_pusch;

  /// CCE index and aggregation, should be coherent with cce_list
  NR_SearchSpace_t *search_space;
  NR_ControlResourceSet_t *coreset;
  int cce_index;
  uint8_t aggregation_level;

  /// RB allocation within active BWP
  uint16_t rbSize;
  uint16_t rbStart;

  /// uplink bytes that are currently scheduled
  int sched_ul_bytes;
  /// estimation of the UL buffer size
  int estimated_ul_buffer;

  // time-domain allocation for scheduled RBs
  int time_domain_allocation;

  /// MCS-related infos
  uint8_t mcsTableIdx;
  uint8_t mcs;
  uint8_t numDmrsCdmGrpsNoData;

  /// Retransmission-related information
  NR_UE_ret_info_t retInfo[NR_MAX_NB_HARQ_PROCESSES];
  /// DL HARQ PID to use for this UE, or -1 for "any new"
  int8_t dl_harq_pid;

  uint16_t ta_frame;
  int16_t ta_update;
  bool ta_apply;
  uint8_t tpc0;
  uint8_t tpc1;
  uint16_t ul_rssi;
  /// information about every HARQ process
  NR_UE_harq_t harq_processes[NR_MAX_NB_HARQ_PROCESSES];
  /// HARQ processes that are free
  NR_list_t available_dl_harq;
  /// HARQ processes that await feedback
  NR_list_t feedback_dl_harq;
  /// HARQ processes that await retransmission
  NR_list_t retrans_dl_harq;
  /// information about every UL HARQ process
  NR_UE_ul_harq_t ul_harq_processes[NR_MAX_NB_HARQ_PROCESSES];
  /// UL HARQ processes that are free
  NR_list_t available_ul_harq;
  /// UL HARQ processes that await feedback
  NR_list_t feedback_ul_harq;
  /// UL HARQ processes that await retransmission
  NR_list_t retrans_ul_harq;
  int dummy;
  NR_UE_mac_ce_ctrl_t UE_mac_ce_ctrl;// MAC CE related information
} NR_UE_sched_ctrl_t;

typedef struct {
  boolean_t fiveG_connected;
  uicc_t *uicc;
} NRUEcontext_t;

typedef struct {
  int lc_bytes_tx[64];
  int lc_bytes_rx[64];
  int dlsch_rounds[8];
  int dlsch_errors;
  int dlsch_total_bytes;
  int dlsch_current_bytes;
  int ulsch_rounds[8];
  int ulsch_errors;
  int ulsch_total_bytes_scheduled;
  int ulsch_total_bytes_rx;
  int ulsch_current_bytes;
} NR_mac_stats_t;


/*! \brief UE list used by gNB to order UEs/CC for scheduling*/
#define MAX_CSI_REPORTCONFIG 48
typedef struct {
  /// scheduling control info
  nr_csi_report_t csi_report_template[MAX_MOBILES_PER_GNB][MAX_CSI_REPORTCONFIG];
  NR_UE_sched_ctrl_t UE_sched_ctrl[MAX_MOBILES_PER_GNB];
  NR_mac_stats_t mac_stats[MAX_MOBILES_PER_GNB];
  NR_list_t list;
  int num_UEs;

  bool active[MAX_MOBILES_PER_GNB];
  rnti_t rnti[MAX_MOBILES_PER_GNB];
  NR_CellGroupConfig_t *secondaryCellGroup[MAX_MOBILES_PER_GNB];
  /// CCE indexing
  int Y[MAX_MOBILES_PER_GNB][3][160];
  int m[MAX_MOBILES_PER_GNB];
  int num_pdcch_cand[MAX_MOBILES_PER_GNB][MAX_NUM_CORESET];
  // UE selected beam index
  uint8_t UE_beam_index[MAX_MOBILES_PER_GNB];
} NR_UE_info_t;

typedef void (*nr_pp_impl_dl)(module_id_t mod_id,
                              frame_t frame,
                              sub_frame_t slot);
typedef bool (*nr_pp_impl_ul)(module_id_t mod_id,
                              frame_t frame,
                              sub_frame_t slot,
                              int num_slots_per_tdd,
                              uint64_t ulsch_in_slot_bitmap);

/*! \brief top level eNB MAC structure */
typedef struct gNB_MAC_INST_s {
  /// Ethernet parameters for northbound midhaul interface
  eth_params_t                    eth_params_n;
  /// Ethernet parameters for fronthaul interface
  eth_params_t                    eth_params_s;
  /// Module
  module_id_t                     Mod_id;
  /// timing advance group
  NR_TAG_t                        *tag;
  /// Pointer to IF module instance for PHY
  NR_IF_Module_t                  *if_inst;
  /// Pusch target SNR
  int                             pusch_target_snrx10;
  /// Pucch target SNR
  int                             pucch_target_snrx10;
  /// Common cell resources
  NR_COMMON_channels_t common_channels[NFAPI_CC_MAX];
  /// current PDU index (BCH,DLSCH)
  uint16_t pdu_index[NFAPI_CC_MAX];

  /// NFAPI Config Request Structure
  nfapi_nr_config_request_scf_t     config[NFAPI_CC_MAX];
  /// NFAPI DL Config Request Structure
  nfapi_nr_dl_tti_request_t         DL_req[NFAPI_CC_MAX];
  /// a PDCCH PDU groups DCIs per BWP and CORESET. The following structure
  /// keeps pointers to PDCCH PDUs within DL_req so that we can easily track
  /// PDCCH PDUs per CC/BWP/CORESET
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_idx[NFAPI_CC_MAX][MAX_NUM_BWP][MAX_NUM_CORESET];
  /// NFAPI UL TTI Request Structure, simple pointer into structure
  /// UL_tti_req_ahead for current frame/slot
  nfapi_nr_ul_tti_request_t        *UL_tti_req[NFAPI_CC_MAX];
  /// NFAPI UL TTI Request Structure for future TTIs, dynamically allocated
  /// because length depends on number of slots
  nfapi_nr_ul_tti_request_t        *UL_tti_req_ahead[NFAPI_CC_MAX];
  /// NFAPI HI/DCI0 Config Request Structure
  nfapi_nr_ul_dci_request_t         UL_dci_req[NFAPI_CC_MAX];
  /// NFAPI DL PDU structure
  nfapi_nr_tx_data_request_t        TX_req[NFAPI_CC_MAX];

  NR_UE_info_t UE_info;

  /// UL handle
  uint32_t ul_handle;
  //UE_info_t UE_info;

  // MAC function execution peformance profiler
  /// processing time of eNB scheduler
  time_stats_t eNB_scheduler;
  /// processing time of eNB scheduler for SI
  time_stats_t schedule_si;
  /// processing time of eNB scheduler for Random access
  time_stats_t schedule_ra;
  /// processing time of eNB ULSCH scheduler
  time_stats_t schedule_ulsch;
  /// processing time of eNB DCI generation
  time_stats_t fill_DLSCH_dci;
  /// processing time of eNB MAC preprocessor
  time_stats_t schedule_dlsch_preprocessor;
  /// processing time of eNB DLSCH scheduler
  time_stats_t schedule_dlsch;  // include rlc_data_req + MAC header + preprocessor
  /// processing time of eNB MCH scheduler
  time_stats_t schedule_mch;
  /// processing time of eNB ULSCH reception
  time_stats_t rx_ulsch_sdu;  // include rlc_data_ind
  /// processing time of eNB PCH scheduler
  time_stats_t schedule_pch;
  /// CCE lists
  int cce_list[MAX_NUM_BWP][MAX_NUM_CORESET][MAX_NUM_CCE];
  /// PUCCH: keep track of the resources has already been used by saving the
  /// highest index not yet been used in a given slot. Dynamically allocated
  /// so we can have it for every slot as a function of the numerology
  int *pucch_index_used[MAX_NUM_BWP];

  /// DL preprocessor for differentiated scheduling
  nr_pp_impl_dl pre_processor_dl;
  /// UL preprocessor for differentiated scheduling
  nr_pp_impl_ul pre_processor_ul;

  NR_UE_sched_ctrl_t *sched_ctrlCommon;
  NR_CellGroupConfig_t *secondaryCellGroupCommon;
  NR_Type0_PDCCH_CSS_config_t type0_PDCCH_CSS_config;

  frame_t handled_frame;
  sub_frame_t handled_slot;

} gNB_MAC_INST;

#endif /*__LAYER2_NR_MAC_GNB_H__ */
