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

/*! \file ngap_gNB_context_management_procedures.c
 * \brief NGAP context management procedures
 * \author  Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \version 1.0
 * @ingroup _ngap
 */
 

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "assertions.h"
#include "conversions.h"

#include "intertask_interface.h"

#include "ngap_common.h"
#include "ngap_gNB_defs.h"

#include "ngap_gNB_itti_messaging.h"

#include "ngap_gNB_encoder.h"
#include "ngap_gNB_nnsf.h"
#include "ngap_gNB_ue_context.h"
#include "ngap_gNB_nas_procedures.h"
#include "ngap_gNB_management_procedures.h"
#include "ngap_gNB_context_management_procedures.h"
#include "NGAP_PDUSessionResourceItemCxtRelReq.h"
#include "msc.h"


int ngap_ue_context_release_complete(instance_t instance,
                                     ngap_ue_release_complete_t *ue_release_complete_p)
{

  ngap_gNB_instance_t                 *ngap_gNB_instance_p = NULL;
  struct ngap_gNB_ue_context_s        *ue_context_p        = NULL;
  NGAP_NGAP_PDU_t                      pdu;
  NGAP_UEContextReleaseComplete_t     *out;
  NGAP_UEContextReleaseComplete_IEs_t *ie;
  uint8_t  *buffer;
  uint32_t length;

  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);

  DevAssert(ue_release_complete_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  /*RB_FOREACH(ue_context_p, ngap_ue_map, &ngap_gNB_instance_p->ngap_ue_head) {
    NGAP_WARN("in ngap_ue_map: UE context gNB_ue_ngap_id %u amf_ue_ngap_id %u state %u\n",
        ue_context_p->gNB_ue_ngap_id, ue_context_p->amf_ue_ngap_id,
        ue_context_p->ue_state);
  }*/
  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                      ue_release_complete_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: %u\n",
              ue_release_complete_p->gNB_ue_ngap_id);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = CALLOC(1, sizeof(struct NGAP_SuccessfulOutcome));
  pdu.choice.successfulOutcome->procedureCode = NGAP_ProcedureCode_id_UEContextRelease;
  pdu.choice.successfulOutcome->criticality = NGAP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = NGAP_SuccessfulOutcome__value_PR_UEContextReleaseComplete;
  out = &pdu.choice.successfulOutcome->value.choice.UEContextReleaseComplete;

  /* mandatory */
  ie = (NGAP_UEContextReleaseComplete_IEs_t *)calloc(1, sizeof(NGAP_UEContextReleaseComplete_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_UEContextReleaseComplete_IEs__value_PR_AMF_UE_NGAP_ID;
  asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (NGAP_UEContextReleaseComplete_IEs_t *)calloc(1, sizeof(NGAP_UEContextReleaseComplete_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_UEContextReleaseComplete_IEs__value_PR_RAN_UE_NGAP_ID;
  ie->value.choice.RAN_UE_NGAP_ID = ue_release_complete_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    /* Encode procedure has failed... */
    NGAP_ERROR("Failed to encode UE context release complete\n");
    return -1;
  }

  MSC_LOG_TX_MESSAGE(
    MSC_NGAP_GNB,
    MSC_NGAP_AMF,
    buffer,
    length,
    MSC_AS_TIME_FMT" UEContextRelease successfulOutcome gNB_ue_ngap_id %u amf_ue_ngap_id %lu",
    0,0, //MSC_AS_TIME_ARGS(ctxt_pP),
    ue_release_complete_p->gNB_ue_ngap_id,
    ue_context_p->amf_ue_ngap_id);

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  //LG ngap_gNB_itti_send_sctp_close_association(ngap_gNB_instance_p->instance,
  //                                             ue_context_p->amf_ref->assoc_id);
  // release UE context
  struct ngap_gNB_ue_context_s *ue_context2_p = NULL;

  if ((ue_context2_p = RB_REMOVE(ngap_ue_map, &ngap_gNB_instance_p->ngap_ue_head, ue_context_p))
      != NULL) {
    NGAP_WARN("Removed UE context gNB_ue_ngap_id %u\n",
              ue_context2_p->gNB_ue_ngap_id);
    ngap_gNB_free_ue_context(ue_context2_p);
  } else {
    NGAP_WARN("Removing UE context gNB_ue_ngap_id %u: did not find context\n",
              ue_context_p->gNB_ue_ngap_id);
  }
  /*RB_FOREACH(ue_context_p, ngap_ue_map, &ngap_gNB_instance_p->ngap_ue_head) {
    NGAP_WARN("in ngap_ue_map: UE context gNB_ue_ngap_id %u amf_ue_ngap_id %u state %u\n",
        ue_context_p->gNB_ue_ngap_id, ue_context_p->amf_ue_ngap_id,
        ue_context_p->ue_state);
  }*/

  return 0;
}


int ngap_ue_context_release_req(instance_t instance,
                                ngap_ue_release_req_t *ue_release_req_p)
{
  ngap_gNB_instance_t                *ngap_gNB_instance_p           = NULL;
  struct ngap_gNB_ue_context_s       *ue_context_p                  = NULL;
  NGAP_NGAP_PDU_t                     pdu;
  NGAP_UEContextReleaseRequest_t     *out;
  NGAP_UEContextReleaseRequest_IEs_t *ie;
  uint8_t                            *buffer                        = NULL;
  uint32_t                            length;
  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);

  DevAssert(ue_release_req_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                      ue_release_req_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: %u\n",
              ue_release_req_p->gNB_ue_ngap_id);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = CALLOC(1, sizeof(struct NGAP_InitiatingMessage));
  pdu.choice.initiatingMessage->procedureCode = NGAP_ProcedureCode_id_UEContextReleaseRequest;
  pdu.choice.initiatingMessage->criticality = NGAP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = NGAP_InitiatingMessage__value_PR_UEContextReleaseRequest;
  out = &pdu.choice.initiatingMessage->value.choice.UEContextReleaseRequest;

  /* mandatory */
  ie = (NGAP_UEContextReleaseRequest_IEs_t *)calloc(1, sizeof(NGAP_UEContextReleaseRequest_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_UEContextReleaseRequest_IEs__value_PR_AMF_UE_NGAP_ID;
  asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (NGAP_UEContextReleaseRequest_IEs_t *)calloc(1, sizeof(NGAP_UEContextReleaseRequest_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_UEContextReleaseRequest_IEs__value_PR_RAN_UE_NGAP_ID;
  ie->value.choice.RAN_UE_NGAP_ID = ue_release_req_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  if (ue_release_req_p->nb_of_pdusessions > 0) {
    ie = (NGAP_UEContextReleaseRequest_IEs_t *)calloc(1, sizeof(NGAP_UEContextReleaseRequest_IEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceListCxtRelReq;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_UEContextReleaseRequest_IEs__value_PR_PDUSessionResourceListCxtRelReq;
    for (int i = 0; i < ue_release_req_p->nb_of_pdusessions; i++) {
      NGAP_PDUSessionResourceItemCxtRelReq_t     *item;
      item = (NGAP_PDUSessionResourceItemCxtRelReq_t *)calloc(1,sizeof(NGAP_PDUSessionResourceItemCxtRelReq_t));
      item->pDUSessionID = ue_release_req_p->pdusessions[i].pdusession_id;
      ASN_SEQUENCE_ADD(&ie->value.choice.PDUSessionResourceListCxtRelReq.list, item);
    }
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  ie = (NGAP_UEContextReleaseRequest_IEs_t *)calloc(1, sizeof(NGAP_UEContextReleaseRequest_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_Cause;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_UEContextReleaseRequest_IEs__value_PR_Cause;

  switch (ue_release_req_p->cause) {
    case NGAP_Cause_PR_radioNetwork:
      ie->value.choice.Cause.present = NGAP_Cause_PR_radioNetwork;
      ie->value.choice.Cause.choice.radioNetwork = ue_release_req_p->cause_value;
      break;

    case NGAP_Cause_PR_transport:
      ie->value.choice.Cause.present = NGAP_Cause_PR_transport;
      ie->value.choice.Cause.choice.transport = ue_release_req_p->cause_value;
      break;

    case NGAP_Cause_PR_nas:
      ie->value.choice.Cause.present = NGAP_Cause_PR_nas;
      ie->value.choice.Cause.choice.nas = ue_release_req_p->cause_value;
      break;

    case NGAP_Cause_PR_protocol:
      ie->value.choice.Cause.present = NGAP_Cause_PR_protocol;
      ie->value.choice.Cause.choice.protocol = ue_release_req_p->cause_value;
      break;

    case NGAP_Cause_PR_misc:
      ie->value.choice.Cause.present = NGAP_Cause_PR_misc;
      ie->value.choice.Cause.choice.misc = ue_release_req_p->cause_value;
      break;

    case NGAP_Cause_PR_NOTHING:
    default:
      ie->value.choice.Cause.present = NGAP_Cause_PR_NOTHING;
      break;
  }

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);



  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    /* Encode procedure has failed... */
    NGAP_ERROR("Failed to encode UE context release complete\n");
    return -1;
  }

  MSC_LOG_TX_MESSAGE(
    MSC_NGAP_GNB,
    MSC_NGAP_AMF,
    buffer,
    length,
    MSC_AS_TIME_FMT" UEContextReleaseRequest initiatingMessage gNB_ue_ngap_id %u amf_ue_ngap_id %lu",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    ue_release_req_p->gNB_ue_ngap_id,
    ue_context_p->amf_ue_ngap_id);

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return 0;
}

