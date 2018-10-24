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

#include "COMMON/platform_constants.h"

#include "NR_BCCH-BCH-Message.h"
#include "NR_ServingCellConfigCommon.h"

#include "nfapi_nr_interface.h"

#include "NR_PHY_INTERFACE/NR_IF_Module.h"

#include "PHY/TOOLS/time_meas.h"

#include "PHY/defs_gNB.h"

#include "targets/ARCH/COMMON/common_lib.h"

#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/ran_context.h"

/*! \brief gNB common channels */
typedef struct {
    int physCellId;
    int p_gNB;
    int Ncp;
    int eutra_band;
    uint32_t dl_CarrierFreq;
    NR_BCCH_BCH_Message_t *mib;
    TDD_Config_t *tdd_Config;
    ARFCN_ValueEUTRA_t ul_CarrierFreq;
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
  /// subframe counter
  sub_frame_t                     subframe;  
  /// Pointer to IF module instance for PHY
  NR_IF_Module_t                  *if_inst;
    /// Common cell resources
  NR_COMMON_channels_t common_channels[NFAPI_CC_MAX];
  /// current PDU index (BCH,DLSCH)
  uint16_t pdu_index[NFAPI_CC_MAX];

  /// NFAPI Config Request Structure
  nfapi_nr_config_request_t         config[NFAPI_CC_MAX];
  /// NFAPI DL Config Request Structure
  nfapi_nr_dl_config_request_t      DL_req[NFAPI_CC_MAX];
  /// NFAPI UL Config Request Structure, send to L1 4 subframes before processing takes place
  nfapi_ul_config_request_t         UL_req[NFAPI_CC_MAX];
  /// Preallocated DL pdu list
  nfapi_nr_dl_config_request_pdu_t  dl_config_pdu_list[NFAPI_CC_MAX][MAX_NUM_DL_PDU];
  /// Preallocated UL pdu list
  nfapi_ul_config_request_pdu_t     ul_config_pdu_list[NFAPI_CC_MAX][MAX_NUM_UL_PDU];
  /// Preallocated UL pdu list for ULSCH (n+k delay)
  nfapi_ul_config_request_pdu_t     ul_config_pdu_list_tmp[NFAPI_CC_MAX][10][MAX_NUM_UL_PDU];
  /// NFAPI "Temporary" UL Config Request Structure, holds future UL_config requests
  nfapi_ul_config_request_t         UL_req_tmp[NFAPI_CC_MAX][10];
  /// Preallocated HI_DCI0 pdu list
  nfapi_hi_dci0_request_pdu_t       hi_dci0_pdu_list[NFAPI_CC_MAX][MAX_NUM_HI_DCI0_PDU];
  /// NFAPI HI/DCI0 Config Request Structure
  nfapi_hi_dci0_request_t           HI_DCI0_req[NFAPI_CC_MAX];
  /// Prealocated TX pdu list
  nfapi_tx_request_pdu_t            tx_request_pdu[NFAPI_CC_MAX][MAX_NUM_TX_REQUEST_PDU];
  /// NFAPI DL PDU structure
  nfapi_tx_request_t                TX_req[NFAPI_CC_MAX];

  UE_list_t UE_list;

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
} gNB_MAC_INST;

#endif /*__LAYER2_NR_MAC_GNB_H__ */
