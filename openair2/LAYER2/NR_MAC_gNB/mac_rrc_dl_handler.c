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

#include "mac_rrc_dl_handler.h"

#include "mac_proto.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"

#include "NR_RRCSetup.h"
#include "NR_DL-CCCH-Message.h"
#include "NR_CellGroupConfig.h"

int dl_rrc_message_rrcSetup(module_id_t module_id, const f1ap_dl_rrc_message_t *dl_rrc, NR_RRCSetup_t *rrcSetup);

int dl_rrc_message(module_id_t module_id, const f1ap_dl_rrc_message_t *dl_rrc)
{
  /* dispatch message to dl_rrc_message_rrcSetup() and others, similar to as is
   * done in the DU (should be the same here) */

  if (dl_rrc->srb_id == 0) {

    NR_DL_CCCH_Message_t *dl_ccch_msg = NULL;
    asn_dec_rval_t dec_rval = uper_decode(NULL,
                                          &asn_DEF_NR_DL_CCCH_Message,
                                          (void **) &dl_ccch_msg,
                                          dl_rrc->rrc_container,
                                          dl_rrc->rrc_container_length,
                                          0,
                                          0);
    AssertFatal(dec_rval.code == RC_OK, "could not decode F1AP message\n");

    switch (dl_ccch_msg->message.choice.c1->present) {
    case NR_DL_CCCH_MessageType__c1_PR_NOTHING:
      LOG_W(NR_MAC, "Received NOTHING on DL-CCCH-Message\n");
      break;
    case NR_DL_CCCH_MessageType__c1_PR_rrcReject:
      LOG_D(NR_MAC, "DL-CCCH/SRB0, received rrcReject for RNTI %04x\n", dl_rrc->rnti);
      AssertFatal(0, "rrcReject not implemented yet\n");
      break;
    case NR_DL_CCCH_MessageType__c1_PR_rrcSetup:
      LOG_I(NR_MAC, "DL-CCCH/SRB0, received rrcSetup for RNTI %04x\n", dl_rrc->rnti);
      return dl_rrc_message_rrcSetup(module_id, dl_rrc, dl_ccch_msg->message.choice.c1->choice.rrcSetup);
      break;
    case NR_DL_CCCH_MessageType__c1_PR_spare2:
      LOG_W(NR_MAC, "DL-CCCH/SRB0, received spare2\n");
      break;
    case NR_DL_CCCH_MessageType__c1_PR_spare1:
      LOG_W(NR_MAC, "DL-CCCH/SRB0, received spare1\n");
      break;
    default:
      AssertFatal(0 == 1, "Unknown DL-CCCH/SRB0 message %d\n", dl_ccch_msg->message.choice.c1->present);
      break;
    }
    return 0;
  }

  return -1; /* not handled yet */
}

int dl_rrc_message_rrcSetup(module_id_t module_id, const f1ap_dl_rrc_message_t *dl_rrc, NR_RRCSetup_t *rrcSetup)
{
  DevAssert(rrcSetup != NULL);

  NR_RRCSetup_IEs_t *rrcSetup_ies = rrcSetup->criticalExtensions.choice.rrcSetup;
  AssertFatal(rrcSetup_ies->masterCellGroup.buf != NULL,"masterCellGroup is NULL\n");
  NR_CellGroupConfig_t *cellGroup = NULL;
  asn_dec_rval_t dec_rval = uper_decode(NULL,
                                        &asn_DEF_NR_CellGroupConfig,
                                        (void **)&cellGroup,
                                        rrcSetup_ies->masterCellGroup.buf,
                                        rrcSetup_ies->masterCellGroup.size,
                                        0,
                                        0);
  AssertFatal(dec_rval.code == RC_OK, "could not decode masterCellGroup\n");

  /* there might be a memory leak for the cell group if we call this multiple
   * times. Also, the first parameters of rrc_mac_config_req_gNB() are only
   * relevant when setting the scc, which we don't do (and cannot do) here. */
  rrc_pdsch_AntennaPorts_t pap = {0};
  rrc_mac_config_req_gNB(module_id,
                         pap, /* only when scc != NULL */
                         0, /* only when scc != NULL */
                         0, /* only when scc != NULL */
                         0, /* only when scc != NULL */
                         NULL, /* scc */
                         NULL, /* mib */
                         NULL, /* sib1 */
                         0, /* add_ue */
                         dl_rrc->rnti,
                         cellGroup);

  /* TODO: drop the RRC context */
  gNB_RRC_INST *rrc = RC.nrrrc[module_id];
  struct rrc_gNB_ue_context_s *ue_context_p = rrc_gNB_get_ue_context(rrc, dl_rrc->rnti);
  gNB_RRC_UE_t *ue_p = &ue_context_p->ue_context;
  ue_context_p->ue_context.SRB_configList = rrcSetup_ies->radioBearerConfig.srb_ToAddModList;
  ue_context_p->ue_context.masterCellGroup = cellGroup;

  /* TODO: this should pass through RLC and NOT the RRC with a shared buffer */
  AssertFatal(ue_p->Srb0.Active == 1,"SRB0 is not active\n");
  memcpy(ue_p->Srb0.Tx_buffer.Payload, dl_rrc->rrc_container, dl_rrc->rrc_container_length);
  ue_p->Srb0.Tx_buffer.payload_size = dl_rrc->rrc_container_length;

  protocol_ctxt_t ctxt = { .module_id = module_id, .rnti = dl_rrc->rnti };
  nr_rrc_rlc_config_asn1_req(&ctxt,
                             ue_context_p->ue_context.SRB_configList,
                             NULL,
                             NULL,
                             NULL,
                             cellGroup->rlc_BearerToAddModList);

  return 0;
}
