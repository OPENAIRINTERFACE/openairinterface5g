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
#include "s1ap_eNB_defs.h"

#include "s1ap_eNB.h"
#include "s1ap_eNB_ue_context.h"
#include "s1ap_eNB_encoder.h"
#include "s1ap_eNB_trace.h"
#include "s1ap_eNB_itti_messaging.h"
#include "s1ap_eNB_management_procedures.h"

static
void s1ap_eNB_generate_trace_failure(struct s1ap_eNB_ue_context_s *ue_desc_p,
                                     S1AP_E_UTRAN_Trace_ID_t      *trace_id,
                                     S1AP_Cause_t                 *cause_p)
{
    S1AP_S1AP_PDU_t                     pdu;
    S1AP_TraceFailureIndication_t      *out;
    S1AP_TraceFailureIndicationIEs_t   *ie;
    uint8_t                            *buffer = NULL;
    uint32_t                            length;

    DevAssert(ue_desc_p != NULL);
    DevAssert(trace_id  != NULL);
    DevAssert(cause_p   != NULL);

    /* Prepare the S1AP message to encode */
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_TraceFailureIndication;
    pdu.choice.initiatingMessage.criticality = S1AP_Criticality_ignore;
    pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_TraceFailureIndication;
    out = &pdu.choice.initiatingMessage.value.choice.TraceFailureIndication;

    /* mandatory */
    ie = (S1AP_TraceFailureIndicationIEs_t *)calloc(1, sizeof(S1AP_TraceFailureIndicationIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_TraceFailureIndicationIEs__value_PR_MME_UE_S1AP_ID;
    ie->value.choice.MME_UE_S1AP_ID = ue_desc_p->mme_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* mandatory */
    ie = (S1AP_TraceFailureIndicationIEs_t *)calloc(1, sizeof(S1AP_TraceFailureIndicationIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_TraceFailureIndicationIEs__value_PR_ENB_UE_S1AP_ID;
    ie->value.choice.ENB_UE_S1AP_ID = ue_desc_p->eNB_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* mandatory */
    ie = (S1AP_TraceFailureIndicationIEs_t *)calloc(1, sizeof(S1AP_TraceFailureIndicationIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_E_UTRAN_Trace_ID;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_TraceFailureIndicationIEs__value_PR_E_UTRAN_Trace_ID;
    memcpy(&ie->value.choice.E_UTRAN_Trace_ID, trace_id, sizeof(S1AP_E_UTRAN_Trace_ID_t));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* mandatory */
    ie = (S1AP_TraceFailureIndicationIEs_t *)calloc(1, sizeof(S1AP_TraceFailureIndicationIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_Cause;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_TraceFailureIndicationIEs__value_PR_Cause;
    memcpy(&ie->value.choice.Cause, cause_p, sizeof(S1AP_Cause_t));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    if (s1ap_eNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        return;
    }

    s1ap_eNB_itti_send_sctp_data_req(ue_desc_p->mme_ref->s1ap_eNB_instance->instance,
                                     ue_desc_p->mme_ref->assoc_id, buffer,
                                     length, ue_desc_p->tx_stream);
}

int s1ap_eNB_handle_trace_start(uint32_t         assoc_id,
                                uint32_t         stream,
                                S1AP_S1AP_PDU_t *pdu)
{
    S1AP_TraceStart_t            *container;
    S1AP_TraceStartIEs_t         *ie;
    struct s1ap_eNB_ue_context_s *ue_desc_p = NULL;
    struct s1ap_eNB_mme_data_s   *mme_ref_p;

    DevAssert(pdu != NULL);

    container = &pdu->choice.initiatingMessage.value.choice.TraceStart;

    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_TraceStartIEs_t, ie, container,
                               S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
    mme_ref_p = s1ap_eNB_get_MME(NULL, assoc_id, 0);
    DevAssert(mme_ref_p != NULL);
  if (ie != NULL) {
    ue_desc_p = s1ap_eNB_get_ue_context(mme_ref_p->s1ap_eNB_instance,
                                        ie->value.choice.ENB_UE_S1AP_ID);
  }
  else
  {
    return -1;
  }
    if (ue_desc_p == NULL) {
        /* Could not find context associated with this eNB_ue_s1ap_id -> generate
         * trace failure indication.
         */
        S1AP_E_UTRAN_Trace_ID_t trace_id;
        S1AP_Cause_t cause;
        memset(&trace_id, 0, sizeof(S1AP_E_UTRAN_Trace_ID_t));
        memset(&cause, 0, sizeof(S1AP_Cause_t));
        cause.present = S1AP_Cause_PR_radioNetwork;
        cause.choice.radioNetwork = S1AP_CauseRadioNetwork_unknown_pair_ue_s1ap_id;
        s1ap_eNB_generate_trace_failure(NULL, &trace_id, &cause);
    }

    return 0;
}

int s1ap_eNB_handle_deactivate_trace(uint32_t         assoc_id,
                                     uint32_t         stream,
                                     S1AP_S1AP_PDU_t *message_p)
{
    //     S1AP_DeactivateTraceIEs_t *deactivate_trace_p;
    //
    //     deactivate_trace_p = &message_p->msg.deactivateTraceIEs;

    return 0;
}
