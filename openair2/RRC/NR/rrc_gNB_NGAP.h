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

/*! \file rrc_gNB_NGAP.h
 * \brief rrc NGAP procedures for gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \version 0.1
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 *         (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com) 
 */

#ifndef RRC_GNB_NGAP_H_
#define RRC_GNB_NGAP_H_

#include "rrc_gNB_UE_context.h"
#include "nr_rrc_defs.h"
#include "nr_rrc_extern.h"

#include "NR_RRCSetupComplete-IEs.h"
#include "NR_RegisteredAMF.h"

typedef struct rrc_ue_ngap_ids_s {
  /* Tree related data */
  RB_ENTRY(rrc_ue_ngap_ids_s) entries;

  // keys
  uint16_t ue_initial_id;
  uint32_t gNB_ue_ngap_id;

  // value
  rnti_t   ue_rnti;
} rrc_ue_ngap_ids_t;

void
rrc_gNB_send_NGAP_NAS_FIRST_REQ(
    const protocol_ctxt_t     *const ctxt_pP,
    rrc_gNB_ue_context_t      *ue_context_pP,
    NR_RRCSetupComplete_IEs_t *rrcSetupComplete
);

int
rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ(
    MessageDef *msg_p,
    const char *msg_name,
    instance_t instance
);

void
rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP(
    const protocol_ctxt_t *const ctxt_pP,
    rrc_gNB_ue_context_t          *const ue_context_pP
);

int
rrc_gNB_process_security(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t *const ue_context_pP,
  ngap_security_capabilities_t *security_capabilities_pP
);

#endif
