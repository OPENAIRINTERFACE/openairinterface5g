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

/* Defs */
#define MAX_NUM_BWP 2
#define MAX_NUM_CORESET 2
#define MAX_NUM_CCE 90
/*!\brief Maximum number of random access process */
#define NR_NB_RA_PROC_MAX 4

typedef enum {
  RA_IDLE = 0,
  Msg2 = 1,
  WAIT_Msg3 = 2,
  Msg4 = 3,
  WAIT_Msg4_ACK = 4
} RA_gNB_state_t;

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
  /// Msg3 pusch pdu
  nfapi_nr_pusch_pdu_t pusch_pdu;
  /// TBS used for Msg4
  int msg4_TBsize;
  /// MCS used for Msg4
  int msg4_mcs;
  /// RA search space
  NR_SearchSpace_t *ra_ss;
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
  /// Outgoing RAR pdu for PHY
  RAR_PDU RAR_pdu;
  /// Template for RA computations
  NR_RA_t ra[NR_NB_RA_PROC_MAX];
  /// VRB map for common channels
  uint8_t vrb_map[100];
  /// VRB map for common channels and retransmissions by PHICH
  uint8_t vrb_map_UL[100];
  /// number of subframe allocation pattern available for MBSFN sync area
  uint8_t num_sf_allocation_pattern;
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
  uint8_t dai_c;
  uint8_t timing_indicator;
  uint8_t resource_indicator;
} NR_sched_pucch;

typedef struct NR_UE_harq {
  uint8_t is_waiting;
  uint8_t ndi;
  uint8_t round;
  uint16_t feedback_slot;
} NR_UE_harq_t;

/*! \brief scheduling control information set through an API */
typedef struct {
  uint64_t dlsch_in_slot_bitmap;  // static bitmap signaling which slot in a tdd period contains dlsch
  uint64_t ulsch_in_slot_bitmap;  // static bitmap signaling which slot in a tdd period contains ulsch
  NR_sched_pucch *sched_pucch;
  uint16_t ta_timer;
  int16_t ta_update;
  uint8_t current_harq_pid;
  NR_UE_harq_t harq_processes[NR_MAX_NB_HARQ_PROCESSES];
  int dummy;
  NR_UE_mac_ce_ctrl_t UE_mac_ce_ctrl;// MAC CE related information
} NR_UE_sched_ctrl_t;

typedef struct NR_preamble_ue {
  uint8_t num_preambles;
  uint8_t *preamble_list;
} NR_preamble_ue;

/*! \brief UE list used by gNB to order UEs/CC for scheduling*/
typedef struct {
  DLSCH_PDU DLSCH_pdu[4][MAX_MOBILES_PER_GNB];
  /// scheduling control info
  NR_UE_sched_ctrl_t UE_sched_ctrl[MAX_MOBILES_PER_GNB];
  int next[MAX_MOBILES_PER_GNB];
  int head;
  int next_ul[MAX_MOBILES_PER_GNB];
  int head_ul;
  int avail;
  int num_UEs;
  boolean_t active[MAX_MOBILES_PER_GNB];
  boolean_t fiveG_connected[MAX_MOBILES_PER_GNB];
  rnti_t rnti[MAX_MOBILES_PER_GNB];
  rnti_t tc_rnti[MAX_MOBILES_PER_GNB];
  NR_preamble_ue preambles[MAX_MOBILES_PER_GNB];
  NR_CellGroupConfig_t *secondaryCellGroup[MAX_MOBILES_PER_GNB];
} NR_UE_list_t;

/*! \brief top level eNB MAC structure */
typedef struct gNB_MAC_INST_s {
  /// Ethernet parameters for northbound midhaul interface
  eth_params_t                    eth_params_n;
  /// Ethernet parameters for fronthaul interface
  eth_params_t                    eth_params_s;
  /// Module
  module_id_t                     Mod_id;
  /// frame counter
  frame_t                         frame;
  /// slot counter
  int                             slot;
  /// timing advance group
  NR_TAG_t                        *tag;
  /// Pointer to IF module instance for PHY
  NR_IF_Module_t                  *if_inst;
  /// TA command
  int                             ta_command;
  /// MAC CE flag indicating TA length
  int                             ta_len;
  /// Common cell resources
  NR_COMMON_channels_t common_channels[NFAPI_CC_MAX];
  /// current PDU index (BCH,DLSCH)
  uint16_t pdu_index[NFAPI_CC_MAX];

  /// NFAPI Config Request Structure
  nfapi_nr_config_request_scf_t     config[NFAPI_CC_MAX];
  /// NFAPI DL Config Request Structure
  nfapi_nr_dl_tti_request_t         DL_req[NFAPI_CC_MAX];
  /// NFAPI UL TTI Request Structure (this is from the new SCF specs)
  nfapi_nr_ul_tti_request_t         UL_tti_req[NFAPI_CC_MAX];
  /// NFAPI HI/DCI0 Config Request Structure
  nfapi_nr_ul_dci_request_t         UL_dci_req[NFAPI_CC_MAX];
  /// NFAPI DL PDU structure
  nfapi_nr_tx_data_request_t        TX_req[NFAPI_CC_MAX];

  NR_UE_list_t UE_list;

  /// UL handle
  uint32_t ul_handle;

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
} gNB_MAC_INST;

#endif /*__LAYER2_NR_MAC_GNB_H__ */
