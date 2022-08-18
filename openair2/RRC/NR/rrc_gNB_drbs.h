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

#define MAX_DRBS_PER_UE         (32)  /* Maximum number of Data Radio Bearers per UE */
#define MAX_PDUS_PER_UE         (8)   /* Maximum number of PDU Sessions per UE */
#define DRB_ACTIVE              (1)
#define DRB_INACTIVE            (0)

typedef struct nr_pdus_s {
  uint8_t pdu_drbs[MAX_DRBS_PER_PDUSESSION];  /* Data Radio Bearers of PDU Session */
  uint8_t pdusession_id;
  uint8_t drbs_established;                   /* Max value -> MAX_DRBS_PER_PDUSESSION*/
} nr_pdus_t;

typedef struct nr_ue_s {
  nr_pdus_t pdus[MAX_PDUS_PER_UE];      /* PDU Sessions */
  uint8_t   used_drbs[MAX_DRBS_PER_UE]; /* Data Radio Bearers of UE, the value is not the drb_id but the pdusession_id */
  rnti_t    ue_id;

  struct nr_ue_s *next_ue;
} nr_ue_t;

NR_DRB_ToAddMod_t *generateDRB(gNB_RRC_UE_t *rrc_ue,
                               const pdu_session_param_t *pduSession,
                               bool enable_sdap,
                               int do_drb_integrity,
                               int do_drb_ciphering);

nr_ue_t *nr_ue_get(rnti_t rnti);
nr_ue_t *nr_ue_new(rnti_t rnti);
uint8_t next_available_drb(gNB_RRC_UE_t *ue, uint8_t pdusession_id);

#endif