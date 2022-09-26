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
 * Author and copyright: Laurent Thomas, open-cells.com
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

#include "e1ap.h"

int asn1_encoder_xer_print = 1;

int e1ap_assoc_id(bool isCu, instance_t instance) {
  return 0;
}

int e1ap_encode_send(bool isCu, instance_t instance, E1AP_E1AP_PDU_t *pdu, uint16_t stream, const char *func) {
  DevAssert(pdu != NULL);

  if (asn1_encoder_xer_print) {
    LOG_E(E1AP, "----------------- ASN1 ENCODER PRINT START ----------------- \n");
    xer_fprint(stdout, &asn_DEF_E1AP_E1AP_PDU, pdu);
    LOG_E(E1AP, "----------------- ASN1 ENCODER PRINT END----------------- \n");
  }

  char errbuf[2048]; /* Buffer for error message */
  size_t errlen = sizeof(errbuf); /* Size of the buffer */
  int ret = asn_check_constraints(&asn_DEF_E1AP_E1AP_PDU, pdu, errbuf, &errlen);

  if(ret) {
    fprintf(stderr, "%s: Constraint validation failed: %s\n", func, errbuf);
  }

  void *buffer = NULL;
  ssize_t encoded = aper_encode_to_new_buffer(&asn_DEF_E1AP_E1AP_PDU, 0, pdu, buffer);

  if (encoded < 0) {
    LOG_E(E1AP, "%s: Failed to encode E1AP message\n", func);
    return -1;
  } else {
    MessageDef *message = itti_alloc_new_message(isCu?TASK_CUCP_E1:TASK_CUUP_E1, 0, SCTP_DATA_REQ);
    sctp_data_req_t *s = &message->ittiMsg.sctp_data_req;
    s->assoc_id      = e1ap_assoc_id(isCu,instance);
    s->buffer        = buffer;
    s->buffer_length = encoded;
    s->stream        = stream;
    LOG_I(E1AP, "%s: Sending ITTI message to SCTP Task\n", func);
    itti_send_msg_to_task(TASK_SCTP, instance, message);
  }

  return encoded;
}

void e1ap_itti_send_sctp_close_association(bool isCu, instance_t instance) {
  MessageDef *message = itti_alloc_new_message(TASK_S1AP, 0, SCTP_CLOSE_ASSOCIATION);
  sctp_close_association_t *sctp_close_association = &message->ittiMsg.sctp_close_association;
  sctp_close_association->assoc_id      = e1ap_assoc_id(isCu,instance);
  itti_send_msg_to_task(TASK_SCTP, instance, message);
}

int e1ap_send_RESET(bool isCu, instance_t instance, E1AP_Reset_t *Reset) {
  AssertFatal(false,"Not implemented yet\n");
  E1AP_E1AP_PDU_t pdu= {0};
  return e1ap_encode_send(isCu, instance, &pdu,0, __func__);
}

int e1ap_send_RESET_ACKNOWLEDGE(instance_t instance, E1AP_Reset_t *Reset) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1ap_handle_RESET(instance_t instance,
                      uint32_t assoc_id,
                      uint32_t stream,
                      E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1ap_handle_RESET_ACKNOWLEDGE(instance_t instance,
                                  uint32_t assoc_id,
                                  uint32_t stream,
                                  E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

/*
    Error Indication
*/
int e1ap_handle_ERROR_INDICATION(instance_t instance,
                                 uint32_t assoc_id,
                                 uint32_t stream,
                                 E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1ap_send_ERROR_INDICATION(instance_t instance, E1AP_ErrorIndication_t *ErrorIndication) {
  AssertFatal(false,"Not implemented yet\n");
}


/*
    E1 Setup: can be sent on both ways, to be refined
*/

int e1apCUUP_send_SETUP_REQUEST(instance_t instance, E1AP_Reset_t *Reset) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_send_SETUP_RESPONSE(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_send_SETUP_FAILURE() {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_SETUP_REQUEST(instance_t instance,
                                  uint32_t assoc_id,
                                  uint32_t stream,
                                  E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_handle_SETUP_RESPONSE(instance_t instance,
                                   uint32_t assoc_id,
                                   uint32_t stream,
                                   E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_handle_SETUP_FAILURE(instance_t instance,
                                  uint32_t assoc_id,
                                  uint32_t stream,
                                  E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

/*
  E1 configuration update: can be sent in both ways, to be refined
*/

int e1apCUUP_send_gNB_DU_CONFIGURATION_UPDATE(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_send_gNB_DU_CONFIGURATION_FAILURE(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_send_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_gNB_DU_CONFIGURATION_UPDATE(instance_t instance,
    uint32_t assoc_id,
    uint32_t stream,
    E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_handle_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
    uint32_t assoc_id,
    uint32_t stream,
    E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_handle_gNB_DU_CONFIGURATION_FAILURE(instance_t instance,
    uint32_t assoc_id,
    uint32_t stream,
    E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

/*
  E1 release
*/

int e1ap_send_RELEASE_REQUEST(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1ap_send_RELEASE_ACKNOWLEDGE(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1ap_handle_RELEASE_REQUEST(instance_t instance,
                                uint32_t assoc_id,
                                uint32_t stream,
                                E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1ap_handle_RELEASE_ACKNOWLEDGE(instance_t instance,
                                    uint32_t assoc_id,
                                    uint32_t stream,
                                    E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

/*
  BEARER CONTEXT SETUP REQUEST
*/

int e1apCUCP_send_BEARER_CONTEXT_SETUP_REQUEST(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_send_BEARER_CONTEXT_SETUP_RESPONSE(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_send_BEARER_CONTEXT_SETUP_FAILURE(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_handle_BEARER_CONTEXT_SETUP_REQUEST(instance_t instance,
                                                 uint32_t assoc_id,
                                                 uint32_t stream,
                                                 E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_BEARER_CONTEXT_SETUP_RESPONSE(instance_t instance,
                                                  uint32_t assoc_id,
                                                  uint32_t stream,
                                                  E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_BEARER_CONTEXT_SETUP_FAILURE(instance_t instance,
                                                 uint32_t assoc_id,
                                                 uint32_t stream,
                                                 E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

/*
  BEARER CONTEXT MODIFICATION REQUEST
*/

int e1apCUCP_send_BEARER_CONTEXT_MODIFICATION_REQUEST(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_send_BEARER_CONTEXT_MODIFICATION_RESPONSE(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_send_BEARER_CONTEXT_MODIFICATION_FAILURE(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_handle_BEARER_CONTEXT_MODIFICATION_REQUEST(instance_t instance,
                                                        uint32_t assoc_id,
                                                        uint32_t stream,
                                                        E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_RESPONSE(instance_t instance,
                                                         uint32_t assoc_id,
                                                         uint32_t stream,
                                                         E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_FAILURE(instance_t instance,
                                                        uint32_t assoc_id,
                                                        uint32_t stream,
                                                        E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_send_BEARER_CONTEXT_MODIFICATION_REQUIRED(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_send_BEARER_CONTEXT_MODIFICATION_CONFIRM(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_REQUIRED(instance_t instance,
                                                         uint32_t assoc_id,
                                                         uint32_t stream,
                                                         E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_handle_BEARER_CONTEXT_MODIFICATION_CONFIRM(instance_t instance,
                                                        uint32_t assoc_id,
                                                        uint32_t stream,
                                                        E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}
/*
  BEARER CONTEXT RELEASE
*/

int e1apCUCP_send_BEARER_CONTEXT_RELEASE_COMMAND(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_send_BEARER_CONTEXT_RELEASE_COMPLETE(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_send_BEARER_CONTEXT_RELEASE_REQUEST(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_handle_BEARER_CONTEXT_RELEASE_COMMAND(instance_t instance,
                                                   uint32_t assoc_id,
                                                   uint32_t stream,
                                                   E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_BEARER_CONTEXT_RELEASE_COMPLETE(instance_t instance,
                                                    uint32_t assoc_id,
                                                    uint32_t stream,
                                                    E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_BEARER_CONTEXT_RELEASE_REQUEST(instance_t instance,
                                                   uint32_t assoc_id,
                                                   uint32_t stream,
                                                   E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

/*
BEARER CONTEXT INACTIVITY NOTIFICATION
 */

int e1apCUUP_send_BEARER_CONTEXT_INACTIVITY_NOTIFICATION(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_BEARER_CONTEXT_INACTIVITY_NOTIFICATION(instance_t instance,
                                                           uint32_t assoc_id,
                                                           uint32_t stream,
                                                           E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}
/*
  DL DATA
*/

int e1apCUUP_send_DL_DATA_NOTIFICATION(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUUP_send_DATA_USAGE_REPORT(instance_t instance) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_DL_DATA_NOTIFICATION(instance_t instance,
                                         uint32_t assoc_id,
                                         uint32_t stream,
                                         E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}

int e1apCUCP_handle_send_DATA_USAGE_REPORT(instance_t instance,
                                           uint32_t assoc_id,
                                           uint32_t stream,
                                           E1AP_E1AP_PDU_t *pdu) {
  AssertFatal(false,"Not implemented yet\n");
}
