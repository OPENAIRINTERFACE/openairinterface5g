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

#ifndef _RRC_GNB_DRBS_H_
#define _RRC_GNB_DRBS_H_

#include "nr_rrc_defs.h"
#include "NR_SDAP-Config.h"
#include "NR_DRB-ToAddMod.h"
#include "NR_SRB-ToAddMod.h"

#define MAX_DRBS_PER_UE         (32)  /* Maximum number of Data Radio Bearers per UE */
#define MAX_PDUS_PER_UE         (8)   /* Maximum number of PDU Sessions per UE */
#define DRB_ACTIVE_NONGBR       (2)   /* DRB is used for Non-GBR Flows */
#define DRB_ACTIVE              (1)
#define DRB_INACTIVE            (0)
#define GBR_FLOW                (1)
#define NONGBR_FLOW             (0)

NR_SRB_ToAddMod_t *generateSRB2(void);
NR_SRB_ToAddModList_t **generateSRB2_confList(gNB_RRC_UE_t *ue, 
                                              NR_SRB_ToAddModList_t *SRB_configList, 
                                              uint8_t xid);
NR_DRB_ToAddMod_t *generateDRB(gNB_RRC_UE_t *rrc_ue,
                               uint8_t drb_id,
                               const pdu_session_param_t *pduSession,
                               bool enable_sdap,
                               int do_drb_integrity,
                               int do_drb_ciphering);

uint8_t next_available_drb(gNB_RRC_UE_t *ue, uint8_t pdusession_id, bool is_gbr);
bool drb_is_active(gNB_RRC_UE_t *ue, uint8_t drb_id);

#endif