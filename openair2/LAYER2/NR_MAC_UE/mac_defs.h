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

/* \file mac_defs.h
 * \brief MAC data structures, constant, and function prototype
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#ifndef __LAYER2_NR_MAC_DEFS_H__
#define __LAYER2_NR_MAC_DEFS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform_types.h"
#include "NR_DRX-Config.h"
#include "NR_SchedulingRequestConfig.h"
#include "NR_BSR-Config.h"
#include "NR_TAG-Config.h"
#include "NR_PHR-Config.h"
#include "NR_RNTI-Value.h"
#include "NR_MIB.h"
#include "NR_MAC-CellGroupConfig.h"
#include "NR_PhysicalCellGroupConfig.h"
#include "NR_SpCellConfig.h"
#include "NR_ServingCellConfig.h"
#include "fapi_nr_ue_interface.h"
#include "NR_IF_Module.h"
#include "PHY/defs_nr_common.h"
#include "openair2/LAYER2/NR_MAC_COMMON/nr_mac.h"

#define NB_NR_UE_MAC_INST 1
/*!\brief Maximum number of logical channl group IDs */


/*!\brief value for indicating BSR Timer is not running */
#define NR_MAC_UE_BSR_TIMER_NOT_RUNNING   (0xFFFF)

typedef enum {
    SFN_C_MOD_2_EQ_0, 
    SFN_C_MOD_2_EQ_1,
    SFN_C_IMPOSSIBLE
} SFN_C_TYPE;

// LTE structure, might need to be adapted for NR
typedef struct {
  /// buffer status for each lcgid
  uint8_t  BSR[NR_MAX_NUM_LCGID]; // should be more for mesh topology
  /// keep the number of bytes in rlc buffer for each lcgid
  int32_t  BSR_bytes[NR_MAX_NUM_LCGID];
  /// after multiplexing buffer remain for each lcid
  int32_t  LCID_buffer_remain[NR_MAX_NUM_LCID];
  /// sum of all lcid buffer size
  uint16_t  All_lcid_buffer_size_lastTTI;
  /// buffer status for each lcid
  uint8_t  LCID_status[NR_MAX_NUM_LCID];
  /// SR pending as defined in 36.321
  uint8_t  SR_pending;
  /// SR_COUNTER as defined in 36.321
  uint16_t SR_COUNTER;
  /// logical channel group ide for each LCID
  uint8_t  LCGID[NR_MAX_NUM_LCID];
  /// retxBSR-Timer, default value is sf2560
  uint16_t retxBSR_Timer;
  /// retxBSR_SF, number of subframe before triggering a regular BSR
  uint16_t retxBSR_SF;
  /// periodicBSR-Timer, default to infinity
  uint16_t periodicBSR_Timer;
  /// periodicBSR_SF, number of subframe before triggering a periodic BSR
  uint16_t periodicBSR_SF;
  /// default value is 0: not configured
  uint16_t sr_ProhibitTimer;
  /// sr ProhibitTime running
  uint8_t sr_ProhibitTimer_Running;
  ///  default value to n5
  uint16_t maxHARQ_Tx;
  /// default value is false
  uint16_t ttiBundling;
  /// default value is release
  struct DRX_Config *drx_config;
  /// default value is release
  struct MAC_MainConfig__phr_Config *phr_config;
  ///timer before triggering a periodic PHR
  uint16_t periodicPHR_Timer;
  ///timer before triggering a prohibit PHR
  uint16_t prohibitPHR_Timer;
  ///DL Pathloss change value
  uint16_t PathlossChange;
  ///number of subframe before triggering a periodic PHR
  int16_t periodicPHR_SF;
  ///number of subframe before triggering a prohibit PHR
  int16_t prohibitPHR_SF;
  ///DL Pathloss Change in db
  uint16_t PathlossChange_db;

  /// default value is false
  uint16_t extendedBSR_Sizes_r10;
  /// default value is false
  uint16_t extendedPHR_r10;

  //Bj bucket usage per  lcid
  int16_t Bj[NR_MAX_NUM_LCID];
  // Bucket size per lcid
  int16_t bucket_size[NR_MAX_NUM_LCID];
} NR_UE_SCHEDULING_INFO;


/*!\brief Top level UE MAC structure */
typedef struct {
  
  ////  MAC config
  NR_DRX_Config_t    	       *drx_Config;
  NR_SchedulingRequestConfig_t *schedulingRequestConfig;
  NR_BSR_Config_t    	       *bsr_Config;
  NR_TAG_Config_t              *tag_Config;
  NR_PHR_Config_t              *phr_Config;
  NR_RNTI_Value_t              *cs_RNTI;
  NR_MIB_t                     *mib;
  
  ///     Type0-PDCCH seach space
  fapi_nr_dl_config_dci_dl_pdu_rel15_t type0_pdcch_dci_config;
  uint32_t type0_pdcch_ss_mux_pattern;
  SFN_C_TYPE type0_pdcch_ss_sfn_c;
  uint32_t type0_pdcch_ss_n_c;
  uint32_t type0_pdcch_consecutive_slots;
  
  ///     Random access parameter
  uint16_t ra_rnti;
  uint16_t crnti;

   //BWP params
  NR_BWP_PARMS initial_bwp_dl;
  NR_BWP_PARMS initial_bwp_ul;

  ////	FAPI-like interface message
  fapi_nr_tx_request_t tx_request;
  fapi_nr_ul_config_request_t ul_config_request;
  fapi_nr_dl_config_request_t dl_config_request;

  ///     Interface module instances
  nr_ue_if_module_t       *if_module;
  nr_scheduled_response_t scheduled_response;
  nr_phy_config_t         phy_config;

  /// BSR report flag management
  uint8_t BSR_reporting_active;
  NR_UE_SCHEDULING_INFO   scheduling_info;

  /// PHR
  uint8_t PHR_reporting_active;
} NR_UE_MAC_INST_t;

typedef enum seach_space_mask_e {
    type0_pdcch  = 0x1, 
    type0a_pdcch = 0x2,
    type1_pdcch  = 0x4, 
    type2_pdcch  = 0x8,
    type3_pdcch  = 0x10
} search_space_mask_t;

typedef enum subcarrier_spacing_e {
    scs_15kHz  = 0x1,
    scs_30kHz  = 0x2,
    scs_60kHz  = 0x4,
    scs_120kHz = 0x8,
    scs_240kHz = 0x16
} subcarrier_spacing_t;

typedef enum channel_bandwidth_e {
    bw_5MHz   = 0x1,
    bw_10MHz  = 0x2,
    bw_20MHz  = 0x4,
    bw_40MHz  = 0x8,
    bw_80MHz  = 0x16,
    bw_100MHz = 0x32
} channel_bandwidth_t;

typedef enum frequency_range_e {
    FR1 = 0, 
    FR2
} frequency_range_t;

#define NUM_SLOT_FRAME 10

/*@}*/
#endif /*__LAYER2_MAC_DEFS_H__ */
