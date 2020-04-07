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

#include "NR_BCCH-BCH-Message.h"
#include "NR_CellGroupConfig.h"
#include "NR_ServingCellConfigCommon.h"
#include "NR_MeasConfig.h"

#include "nfapi_nr_interface_scf.h"
#include "NR_PHY_INTERFACE/NR_IF_Module.h"

#include "COMMON/platform_constants.h"
#include "common/ran_context.h"
#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "PHY/defs_gNB.h"
#include "PHY/TOOLS/time_meas.h"
#include "targets/ARCH/COMMON/common_lib.h"

#include "nr_mac_common.h"

#define MAX_NUM_BWP 2
#define MAX_NUM_CORESET 2
#define MAX_NUM_CCE 90

#include "NR_TAG.h"

/*! \brief gNB common channels */
typedef struct {
  int physCellId;
  int p_gNB;
  int Ncp;
  int nr_band;
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
  RA_t ra[NB_RA_PROC_MAX];
  /// VRB map for common channels
  uint8_t vrb_map[100];
  /// VRB map for common channels and retransmissions by PHICH
  uint8_t vrb_map_UL[100];
  /// number of subframe allocation pattern available for MBSFN sync area
  uint8_t num_sf_allocation_pattern;
} NR_COMMON_channels_t;

typedef struct NR_sched_pucch {
  int frame;
  int ul_slot;
  uint8_t dai_c;
  uint8_t timing_indicator;
  uint8_t resource_indicator;
  struct NR_sched_pucch *next_sched_pucch;
} NR_sched_pucch;

/*! \brief scheduling control information set through an API */
typedef struct {
  uint64_t dlsch_in_slot_bitmap;  // static bitmap signaling which slot in a tdd period contains dlsch
  uint64_t ulsch_in_slot_bitmap;  // static bitmap signaling which slot in a tdd period contains ulsch
  NR_sched_pucch *sched_pucch;
  uint16_t ta_timer;
  int16_t ta_update;
} NR_UE_sched_ctrl_t;

/*! \brief UE list used by eNB to order UEs/CC for scheduling*/
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
  rnti_t rnti[MAX_MOBILES_PER_GNB];
  NR_CellGroupConfig_t *secondaryCellGroup[MAX_MOBILES_PER_GNB];
} NR_UE_list_t;

/*! \brief top level gNB MAC structure */
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
