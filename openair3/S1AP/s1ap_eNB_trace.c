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

#include <stdint.h>

#include "assertions.h"

#include "intertask_interface.h"

#include "s1ap_eNB_default_values.h"

#include "s1ap_common.h"
#include "s1ap_ies_defs.h"
#include "s1ap_eNB_defs.h"

#include "s1ap_eNB.h"
#include "s1ap_eNB_ue_context.h"
#include "s1ap_eNB_encoder.h"
#include "s1ap_eNB_trace.h"
#include "s1ap_eNB_itti_messaging.h"
#include "s1ap_eNB_management_procedures.h"

static
void s1ap_eNB_generate_trace_failure(struct s1ap_eNB_ue_context_s *ue_desc_p,
                                     S1ap_E_UTRAN_Trace_ID_t      *trace_id,
                                     S1ap_Cause_t                 *cause_p)
{
  s1ap_message message;
  S1ap_TraceFailureIndicationIEs_t *trace_failure_p;
  uint8_t  *buffer;
  uint32_t  length;

  DevAssert(ue_desc_p != NULL);
  DevAssert(trace_id  != NULL);
  DevAssert(cause_p   != NULL);

  memset(&message, 0, sizeof(s1ap_message));

  trace_failure_p = &message.msg.s1ap_TraceFailureIndicationIEs;

  trace_failure_p->mme_ue_s1ap_id = ue_desc_p->mme_ue_s1ap_id;
  trace_failure_p->eNB_UE_S1AP_ID = ue_desc_p->eNB_ue_s1ap_id;

  memcpy(&trace_failure_p->e_UTRAN_Trace_ID, trace_id, sizeof(S1ap_E_UTRAN_Trace_ID_t));
  memcpy(&trace_failure_p->cause, cause_p, sizeof(S1ap_Cause_t));

  if (s1ap_eNB_encode_pdu(&message, &buffer, &length) < 0) {
    return;
  }

  s1ap_eNB_itti_send_sctp_data_req(ue_desc_p->mme_ref->s1ap_eNB_instance->instance,
                                   ue_desc_p->mme_ref->assoc_id, buffer,
                                   length, ue_desc_p->tx_stream);
}

int s1ap_eNB_handle_trace_start(uint32_t               assoc_id,
                                uint32_t               stream,
                                struct s1ap_message_s *message_p)
{
  S1ap_TraceStartIEs_t         *trace_start_p;
  struct s1ap_eNB_ue_context_s *ue_desc_p;
  struct s1ap_eNB_mme_data_s   *mme_ref_p;

  DevAssert(message_p != NULL);

  trace_start_p = &message_p->msg.s1ap_TraceStartIEs;

  mme_ref_p = s1ap_eNB_get_MME(NULL, assoc_id, 0);
  DevAssert(mme_ref_p != NULL);

  if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_ref_p->s1ap_eNB_instance,
                   trace_start_p->eNB_UE_S1AP_ID)) == NULL) {
    /* Could not find context associated with this eNB_ue_s1ap_id -> generate
     * trace failure indication.
     */
    S1ap_E_UTRAN_Trace_ID_t trace_id;
    S1ap_Cause_t cause;

    memset(&trace_id, 0, sizeof(S1ap_E_UTRAN_Trace_ID_t));
    memset(&cause, 0, sizeof(S1ap_Cause_t));

    cause.present = S1ap_Cause_PR_radioNetwork;
    cause.choice.radioNetwork = S1ap_CauseRadioNetwork_unknown_pair_ue_s1ap_id;

    s1ap_eNB_generate_trace_failure(ue_desc_p, &trace_id, &cause);
  }

  return 0;
}

int s1ap_eNB_handle_deactivate_trace(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     struct s1ap_message_s *message_p)
{
  //     S1ap_DeactivateTraceIEs_t *deactivate_trace_p;
  //
  //     deactivate_trace_p = &message_p->msg.deactivateTraceIEs;

  return 0;
}
