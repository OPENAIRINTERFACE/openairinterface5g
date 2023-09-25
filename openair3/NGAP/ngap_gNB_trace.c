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

int ngap_gNB_handle_trace_start(sctp_assoc_t assoc_id, uint32_t stream, NGAP_NGAP_PDU_t *pdu)
{
    //TODO
    return 0;
}

int ngap_gNB_handle_deactivate_trace(sctp_assoc_t assoc_id, uint32_t stream, NGAP_NGAP_PDU_t *message_p)
{
    //     NGAP_DeactivateTraceIEs_t *deactivate_trace_p;
    //
    //     deactivate_trace_p = &message_p->msg.deactivateTraceIEs;

    return 0;
}
