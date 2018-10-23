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

/*! \file mac_proto.h
 * \brief MAC functions prototypes for gNB
 * \author Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \email navid.nikaein@eurecom.fr, kroempa@gmail.com
 * \version 1.0
 * \company Eurecom, NTUST
 */

#ifndef __LAYER2_NR_MAC_PROTO_H__
#define __LAYER2_NR_MAC_PROTO_H__

#include "nr_mac_gNB.h"
#include "PHY/defs_gNB.h"

void mac_top_init_gNB(void);

int rrc_mac_config_req_gNB(module_id_t Mod_idP, 
                           int CC_id,
                           int p_gNB,
                           int eutra_bandP,
                           int dl_CarrierFreqP,
                           int dl_BandwidthP,
                           NR_BCCH_BCH_Message_t *mib,
                           NR_ServingCellConfigCommon_t *servingcellconfigcommon
                           );

int  is_nr_UL_sf(NR_COMMON_channels_t * ccP, sub_frame_t subframeP);

void clear_nr_nfapi_information(gNB_MAC_INST * gNB, 
                                int CC_idP,
                                frame_t frameP, 
                                sub_frame_t subframeP);

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP, 
                               frame_t frameP,
                               sub_frame_t subframeP);

void schedule_nr_mib(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP);

#endif /*__LAYER2_NR_MAC_PROTO_H__*/
