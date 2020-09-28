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

/*! \file ngap_gNB_trace.c
 * \brief ngap trace procedures for gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \version 0.1
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
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


int ngap_gNB_handle_trace_start(uint32_t         assoc_id,
                                uint32_t         stream,
                                NGAP_NGAP_PDU_t *pdu)
{
#if 0
    NGAP_TraceStart_t            *container;
    NGAP_TraceStartIEs_t         *ie;
    struct ngap_gNB_ue_context_s *ue_desc_p = NULL;
    struct ngap_gNB_amf_data_s   *amf_ref_p;

    DevAssert(pdu != NULL);

    container = &pdu->choice.initiatingMessage.value.choice.TraceStart;

    NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_TraceStartIEs_t, ie, container,
                               NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID, TRUE);
    amf_ref_p = ngap_gNB_get_AMF(NULL, assoc_id, 0);
    DevAssert(amf_ref_p != NULL);
  if (ie != NULL) {
    ue_desc_p = ngap_gNB_get_ue_context(amf_ref_p->ngap_gNB_instance,
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

#endif
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
