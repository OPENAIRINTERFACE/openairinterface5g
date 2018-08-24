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

#define NB_NR_UE_MAC_INST 1

typedef enum {
    SFN_C_MOD_2_EQ_0, 
    SFN_C_MOD_2_EQ_1
} SFN_C_TYPE;

/*!\brief Top level UE MAC structure */
typedef struct {
    
    ////  MAC config
    NR_DRX_Config_t    	            *drx_Config;
    NR_SchedulingRequestConfig_t    *schedulingRequestConfig;
    NR_BSR_Config_t    	            *bsr_Config;
    NR_TAG_Config_t		            *tag_Config;
    NR_PHR_Config_t		            *phr_Config;
    NR_RNTI_Value_t 	            *cs_RNTI;
	NR_MIB_t 			            *mib;

    ///     Type0-PDCCH seach space
    fapi_nr_dl_config_dci_dl_pdu_rel15_t type0_pdcch_dci_config;
    uint32_t type0_pdcch_ss_mux_pattern;
    SFN_C_TYPE type0_pdcch_ss_sfn_c;
    uint32_t type0_pdcch_ss_n_c;
    uint32_t type0_pdcch_consecutive_slots;

    ///     Random access parameter
    uint16_t ra_rnti;


	////	FAPI-like interface message
	fapi_nr_tx_request_t tx_request;
	fapi_nr_ul_config_request_t ul_config_request;
	fapi_nr_dl_config_request_t dl_config_request;



	///     Interface module instances
	nr_ue_if_module_t *if_module;
	nr_scheduled_response_t	scheduled_response;
	nr_phy_config_t phy_config;




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
