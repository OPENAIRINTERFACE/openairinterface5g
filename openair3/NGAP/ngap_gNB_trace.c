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

#include "ngap_gNB_default_values.h"

#include "ngap_common.h"
#include "ngap_gNB_defs.h"

#include "ngap_gNB.h"
#include "ngap_gNB_ue_context.h"
#include "ngap_gNB_encoder.h"
#include "ngap_gNB_trace.h"
#include "ngap_gNB_itti_messaging.h"
#include "ngap_gNB_management_procedures.h"

static
void ngap_gNB_generate_trace_failure(struct ngap_gNB_ue_context_s *ue_desc_p,
                                     NGAP_E_UTRAN_Trace_ID_t      *trace_id,
                                     NGAP_Cause_t                 *cause_p)
{
    NGAP_NGAP_PDU_t                     pdu;
    NGAP_TraceFailureIndication_t      *out;
    NGAP_TraceFailureIndicationIEs_t   *ie;
    uint8_t                            *buffer = NULL;
    uint32_t                            length;

    DevAssert(ue_desc_p != NULL);
    DevAssert(trace_id  != NULL);
    DevAssert(cause_p   != NULL);

    /* Prepare the NGAP message to encode */
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage.procedureCode = NGAP_ProcedureCode_id_TraceFailureIndication;
    pdu.choice.initiatingMessage.criticality = NGAP_Criticality_ignore;
    pdu.choice.initiatingMessage.value.present = NGAP_InitiatingMessage__value_PR_TraceFailureIndication;
    out = &pdu.choice.initiatingMessage.value.choice.TraceFailureIndication;

    /* mandatory */
    ie = (NGAP_TraceFailureIndicationIEs_t *)calloc(1, sizeof(NGAP_TraceFailureIndicationIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_TraceFailureIndicationIEs__value_PR_MME_UE_NGAP_ID;
    ie->value.choice.MME_UE_NGAP_ID = ue_desc_p->mme_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* mandatory */
    ie = (NGAP_TraceFailureIndicationIEs_t *)calloc(1, sizeof(NGAP_TraceFailureIndicationIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_TraceFailureIndicationIEs__value_PR_GNB_UE_NGAP_ID;
    ie->value.choice.GNB_UE_NGAP_ID = ue_desc_p->gNB_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* mandatory */
    ie = (NGAP_TraceFailureIndicationIEs_t *)calloc(1, sizeof(NGAP_TraceFailureIndicationIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_E_UTRAN_Trace_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_TraceFailureIndicationIEs__value_PR_E_UTRAN_Trace_ID;
    memcpy(&ie->value.choice.E_UTRAN_Trace_ID, trace_id, sizeof(NGAP_E_UTRAN_Trace_ID_t));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* mandatory */
    ie = (NGAP_TraceFailureIndicationIEs_t *)calloc(1, sizeof(NGAP_TraceFailureIndicationIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_Cause;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_TraceFailureIndicationIEs__value_PR_Cause;
    memcpy(&ie->value.choice.Cause, cause_p, sizeof(NGAP_Cause_t));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        return;
    }

    ngap_gNB_itti_send_sctp_data_req(ue_desc_p->mme_ref->ngap_gNB_instance->instance,
                                     ue_desc_p->mme_ref->assoc_id, buffer,
                                     length, ue_desc_p->tx_stream);
}

int ngap_gNB_handle_trace_start(uint32_t         assoc_id,
                                uint32_t         stream,
                                NGAP_NGAP_PDU_t *pdu)
{
    NGAP_TraceStart_t            *container;
    NGAP_TraceStartIEs_t         *ie;
    struct ngap_gNB_ue_context_s *ue_desc_p = NULL;
    struct ngap_gNB_mme_data_s   *mme_ref_p;

    DevAssert(pdu != NULL);

    container = &pdu->choice.initiatingMessage.value.choice.TraceStart;

    NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_TraceStartIEs_t, ie, container,
                               NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID, TRUE);
    mme_ref_p = ngap_gNB_get_MME(NULL, assoc_id, 0);
    DevAssert(mme_ref_p != NULL);
  if (ie != NULL) {
    ue_desc_p = ngap_gNB_get_ue_context(mme_ref_p->ngap_gNB_instance,
                                        ie->value.choice.GNB_UE_NGAP_ID);
  }
    if (ue_desc_p == NULL) {
        /* Could not find context associated with this gNB_ue_ngap_id -> generate
         * trace failure indication.
         */
        NGAP_E_UTRAN_Trace_ID_t trace_id;
        NGAP_Cause_t cause;
        memset(&trace_id, 0, sizeof(NGAP_E_UTRAN_Trace_ID_t));
        memset(&cause, 0, sizeof(NGAP_Cause_t));
        cause.present = NGAP_Cause_PR_radioNetwork;
        cause.choice.radioNetwork = NGAP_CauseRadioNetwork_unknown_pair_ue_ngap_id;
        ngap_gNB_generate_trace_failure(NULL, &trace_id, &cause);
    }

    return 0;
}

int ngap_gNB_handle_deactivate_trace(uint32_t         assoc_id,
                                     uint32_t         stream,
                                     NGAP_NGAP_PDU_t *message_p)
{
    //     NGAP_DeactivateTraceIEs_t *deactivate_trace_p;
    //
    //     deactivate_trace_p = &message_p->msg.deactivateTraceIEs;

    return 0;
}
