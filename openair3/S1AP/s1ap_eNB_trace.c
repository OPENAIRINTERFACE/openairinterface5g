/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

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
