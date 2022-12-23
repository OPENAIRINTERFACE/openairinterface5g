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
#include "NR_UL-DCCH-Message.h"
#include "NGAP_CauseRadioNetwork.h"

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

int
rrc_gNB_process_NGAP_DOWNLINK_NAS(
  MessageDef *msg_p,
  const char *msg_name,
  instance_t  instance,
  mui_t      *rrc_gNB_mui
);

void
rrc_gNB_send_NGAP_UPLINK_NAS(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_gNB_ue_context_t     *const ue_context_pP,
  NR_UL_DCCH_Message_t     *const ul_dcch_msg
);

void
rrc_gNB_send_NGAP_PDUSESSION_SETUP_RESP(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_gNB_ue_context_t     *const ue_context_pP,
  uint8_t                   xid
);

int
rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ(
    MessageDef *msg_p,
    const char *msg_name,
    instance_t instance
);

int
rrc_gNB_process_NGAP_PDUSESSION_MODIFY_REQ(
  MessageDef *msg_p,
  const char *msg_name,
  instance_t instance
);

int
rrc_gNB_send_NGAP_PDUSESSION_MODIFY_RESP(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_gNB_ue_context_t     *const ue_context_pP,
  uint8_t                   xid
);

void
rrc_gNB_modify_dedicatedRRCReconfiguration(
  const protocol_ctxt_t     *const ctxt_pP,
  rrc_gNB_ue_context_t      *ue_context_pP
);


void
rrc_gNB_send_NGAP_UE_CONTEXT_RELEASE_REQ(
  const module_id_t gnb_mod_idP,
  const rrc_gNB_ue_context_t *const ue_context_pP,
  const ngap_Cause_t causeP,
  const long cause_valueP
);

int 
rrc_gNB_process_NGAP_UE_CONTEXT_RELEASE_REQ (
  MessageDef *msg_p, 
  const char *msg_name, 
  instance_t instance
);

int
rrc_gNB_process_NGAP_UE_CONTEXT_RELEASE_COMMAND(
  MessageDef *msg_p,
  const char *msg_name,
  instance_t instance
);

void rrc_gNB_send_NGAP_UE_CONTEXT_RELEASE_COMPLETE(
  instance_t instance,
  uint32_t   gNB_ue_ngap_id);

void
rrc_gNB_NGAP_remove_ue_ids(
  gNB_RRC_INST *const rrc_instance_pP,
  struct rrc_ue_ngap_ids_s *const ue_ids_pP
);

void
rrc_gNB_send_NGAP_UE_CAPABILITIES_IND(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_gNB_ue_context_t     *const ue_context_pP,
  NR_UL_DCCH_Message_t     *const ul_dcch_msg
);

int
rrc_gNB_process_NGAP_PDUSESSION_RELEASE_COMMAND(
  MessageDef *msg_p,
  const char *msg_name,
  instance_t instance
);

void
rrc_gNB_send_NGAP_PDUSESSION_RELEASE_RESPONSE(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_gNB_ue_context_t     *const ue_context_pP,
  uint8_t                   xid
);

void
nr_rrc_pdcp_config_security(
    const protocol_ctxt_t  *const ctxt_pP,
    rrc_gNB_ue_context_t   *const ue_context_pP,
    const uint8_t          send_security_mode_command
);

int
rrc_gNB_process_PAGING_IND(
    MessageDef *msg_p,
    const char *msg_name,
    instance_t instance);

#endif
