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

/*! \file s1ap_eNB_overload.c
 * \brief s1ap procedures for overload messages within eNB
 * \author Sebastien ROUX <sebastien.roux@eurecom.fr>
 * \date 2012
 * \version 0.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "intertask_interface.h"

#include "s1ap_common.h"
#include "s1ap_eNB_defs.h"

#include "s1ap_eNB.h"
#include "s1ap_eNB_ue_context.h"
#include "s1ap_eNB_encoder.h"
#include "s1ap_eNB_overload.h"
#include "s1ap_eNB_management_procedures.h"

#include "assertions.h"

int s1ap_eNB_handle_overload_start(uint32_t         assoc_id,
                                   uint32_t         stream,
                                   S1AP_S1AP_PDU_t *pdu)
{
    s1ap_eNB_mme_data_t     *mme_desc_p;
    S1AP_OverloadStart_t    *container;
    S1AP_OverloadStartIEs_t *ie;

    DevAssert(pdu != NULL);

    container = &pdu->choice.initiatingMessage.value.choice.OverloadStart;

    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_OverloadStartIEs_t, ie, container,
                               S1AP_ProtocolIE_ID_id_OverloadResponse, true);
    if (ie != NULL) {
        DevCheck(ie->value.choice.OverloadResponse.present ==
                 S1AP_OverloadResponse_PR_overloadAction,
                 S1AP_OverloadResponse_PR_overloadAction, 0, 0);
    }
    /* Non UE-associated signalling -> stream 0 */
    DevCheck(stream == 0, stream, 0, 0);

    if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
        /* No MME context associated */
        return -1;
    }

    /* Mark the MME as overloaded and set the overload state according to
     * the value received.
     */
    mme_desc_p->state = S1AP_ENB_OVERLOAD;
    mme_desc_p->overload_state =
        ie->value.choice.OverloadResponse.choice.overloadAction;

    return 0;
}

int s1ap_eNB_handle_overload_stop(uint32_t         assoc_id,
                                  uint32_t         stream,
                                  S1AP_S1AP_PDU_t *pdu)
{
    /* We received Overload stop message, meaning that the MME is no more
     * overloaded. This is an empty message, with only message header and no
     * Information Element.
     */
    DevAssert(pdu != NULL);

    s1ap_eNB_mme_data_t *mme_desc_p;

    /* Non UE-associated signalling -> stream 0 */
    DevCheck(stream == 0, stream, 0, 0);

    if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
        /* No MME context associated */
        return -1;
    }

    mme_desc_p->state = S1AP_ENB_STATE_CONNECTED;
    mme_desc_p->overload_state = S1AP_NO_OVERLOAD;
    return 0;
}
