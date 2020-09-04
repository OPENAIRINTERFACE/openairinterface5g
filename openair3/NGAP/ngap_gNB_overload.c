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

/*! \file ngap_gNB_overload.c
 * \brief ngap procedures for overload messages within gNB
 * \author Sebastien ROUX <sebastien.roux@eurecom.fr>
 * \date 2012
 * \version 0.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "intertask_interface.h"

#include "ngap_common.h"
#include "ngap_gNB_defs.h"

#include "ngap_gNB.h"
#include "ngap_gNB_ue_context.h"
#include "ngap_gNB_encoder.h"
#include "ngap_gNB_overload.h"
#include "ngap_gNB_management_procedures.h"

#include "assertions.h"

int ngap_gNB_handle_overload_start(uint32_t         assoc_id,
                                   uint32_t         stream,
                                   NGAP_NGAP_PDU_t *pdu)
{
    ngap_gNB_mme_data_t     *mme_desc_p;
    NGAP_OverloadStart_t    *container;
    NGAP_OverloadStartIEs_t *ie;

    DevAssert(pdu != NULL);

    container = &pdu->choice.initiatingMessage.value.choice.OverloadStart;

    NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_OverloadStartIEs_t, ie, container,
                               NGAP_ProtocolIE_ID_id_OverloadResponse, true);
    if (ie != NULL) {
        DevCheck(ie->value.choice.OverloadResponse.present ==
                 NGAP_OverloadResponse_PR_overloadAction,
                 NGAP_OverloadResponse_PR_overloadAction, 0, 0);
    }
    /* Non UE-associated signalling -> stream 0 */
    DevCheck(stream == 0, stream, 0, 0);

    if ((mme_desc_p = ngap_gNB_get_MME(NULL, assoc_id, 0)) == NULL) {
        /* No MME context associated */
        return -1;
    }

    /* Mark the MME as overloaded and set the overload state according to
     * the value received.
     */
    mme_desc_p->state = NGAP_GNB_OVERLOAD;
    mme_desc_p->overload_state =
        ie->value.choice.OverloadResponse.choice.overloadAction;

    return 0;
}

int ngap_gNB_handle_overload_stop(uint32_t         assoc_id,
                                  uint32_t         stream,
                                  NGAP_NGAP_PDU_t *pdu)
{
    /* We received Overload stop message, meaning that the MME is no more
     * overloaded. This is an empty message, with only message header and no
     * Information Element.
     */
    DevAssert(pdu != NULL);

    ngap_gNB_mme_data_t *mme_desc_p;

    /* Non UE-associated signalling -> stream 0 */
    DevCheck(stream == 0, stream, 0, 0);

    if ((mme_desc_p = ngap_gNB_get_MME(NULL, assoc_id, 0)) == NULL) {
        /* No MME context associated */
        return -1;
    }

    mme_desc_p->state = NGAP_GNB_STATE_CONNECTED;
    mme_desc_p->overload_state = NGAP_NO_OVERLOAD;
    return 0;
}
