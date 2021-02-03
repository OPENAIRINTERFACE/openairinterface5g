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

/*! \file ngap_gNB.c
 * \brief NGAP gNB task
 * \author  Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \version 1.0
 * @ingroup _ngap
 */


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <crypt.h>

#include "tree.h"
#include "queue.h"

#include "intertask_interface.h"

#include "ngap_gNB_default_values.h"

#include "ngap_common.h"

#include "ngap_gNB_defs.h"
#include "ngap_gNB.h"
#include "ngap_gNB_encoder.h"
#include "ngap_gNB_handlers.h"
#include "ngap_gNB_nnsf.h"

#include "ngap_gNB_nas_procedures.h"
#include "ngap_gNB_management_procedures.h"
#include "ngap_gNB_context_management_procedures.h"

#include "ngap_gNB_itti_messaging.h"

#include "ngap_gNB_ue_context.h" // test, to be removed
#include "msc.h"

#include "assertions.h"
#include "conversions.h"
#if defined(TEST_S1C_AMF)
  #include "oaisim_amf_test_s1c.h"
#endif

ngap_gNB_config_t ngap_config;

static int ngap_gNB_generate_ng_setup_request(
  ngap_gNB_instance_t *instance_p, ngap_gNB_amf_data_t *ngap_amf_data_p);

void ngap_gNB_handle_register_gNB(instance_t instance, ngap_register_gnb_req_t *ngap_register_gNB);

void ngap_gNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp);

uint32_t ngap_generate_gNB_id(void) {
  char    *out;
  char     hostname[50];
  int      ret;
  uint32_t gNB_id;
  /* Retrieve the host name */
  ret = gethostname(hostname, sizeof(hostname));
  DevAssert(ret == 0);
  out = crypt(hostname, "eurecom");
  DevAssert(out != NULL);
  gNB_id = ((out[0] << 24) | (out[1] << 16) | (out[2] << 8) | out[3]);
  return gNB_id;
}

static void ngap_gNB_register_amf(ngap_gNB_instance_t *instance_p,
                                  net_ip_address_t    *amf_ip_address,
                                  net_ip_address_t    *local_ip_addr,
                                  uint16_t             in_streams,
                                  uint16_t             out_streams,
                                  uint8_t              broadcast_plmn_num,
                                  uint8_t              broadcast_plmn_index[PLMN_LIST_MAX_SIZE]) {
  MessageDef                 *message_p                   = NULL;
  sctp_new_association_req_t *sctp_new_association_req_p  = NULL;
  ngap_gNB_amf_data_t        *ngap_amf_data_p             = NULL;

  DevAssert(instance_p != NULL);
  DevAssert(amf_ip_address != NULL);
  message_p = itti_alloc_new_message(TASK_NGAP, 0, SCTP_NEW_ASSOCIATION_REQ);
  sctp_new_association_req_p = &message_p->ittiMsg.sctp_new_association_req;
  sctp_new_association_req_p->port = NGAP_PORT_NUMBER;
  sctp_new_association_req_p->ppid = NGAP_SCTP_PPID;
  sctp_new_association_req_p->in_streams  = in_streams;
  sctp_new_association_req_p->out_streams = out_streams;
  memcpy(&sctp_new_association_req_p->remote_address,
         amf_ip_address,
         sizeof(*amf_ip_address));
  memcpy(&sctp_new_association_req_p->local_address,
         local_ip_addr,
         sizeof(*local_ip_addr));
  NGAP_INFO("[gNB %ld] check the amf registration state\n",instance_p->instance);

  /* Create new AMF descriptor */
  ngap_amf_data_p = calloc(1, sizeof(*ngap_amf_data_p));
  DevAssert(ngap_amf_data_p != NULL);
  ngap_amf_data_p->cnx_id                = ngap_gNB_fetch_add_global_cnx_id();
  sctp_new_association_req_p->ulp_cnx_id = ngap_amf_data_p->cnx_id;
  ngap_amf_data_p->assoc_id          = -1;
  ngap_amf_data_p->broadcast_plmn_num = broadcast_plmn_num;
  memcpy(&ngap_amf_data_p->amf_s1_ip,
        amf_ip_address,
        sizeof(*amf_ip_address));
  for (int i = 0; i < broadcast_plmn_num; ++i)
    ngap_amf_data_p->broadcast_plmn_index[i] = broadcast_plmn_index[i];

  ngap_amf_data_p->ngap_gNB_instance = instance_p;
  STAILQ_INIT(&ngap_amf_data_p->served_guami);
  /* Insert the new descriptor in list of known AMF
   * but not yet associated.
   */
  RB_INSERT(ngap_amf_map, &instance_p->ngap_amf_head, ngap_amf_data_p);
  ngap_amf_data_p->state = NGAP_GNB_STATE_DISCONNECTED;
  instance_p->ngap_amf_nb ++;
  instance_p->ngap_amf_pending_nb ++;

  itti_send_msg_to_task(TASK_SCTP, instance_p->instance, message_p);
}

void ngap_gNB_handle_register_gNB(instance_t instance, ngap_register_gnb_req_t *ngap_register_gNB) {
  ngap_gNB_instance_t *new_instance;
  uint8_t index;
  DevAssert(ngap_register_gNB != NULL);
  /* Look if the provided instance already exists */
  new_instance = ngap_gNB_get_instance(instance);

  if (new_instance != NULL) {
    /* Checks if it is a retry on the same gNB */
    DevCheck(new_instance->gNB_id == ngap_register_gNB->gNB_id, new_instance->gNB_id, ngap_register_gNB->gNB_id, 0);
    DevCheck(new_instance->cell_type == ngap_register_gNB->cell_type, new_instance->cell_type, ngap_register_gNB->cell_type, 0);
    DevCheck(new_instance->num_plmn == ngap_register_gNB->num_plmn, new_instance->num_plmn, ngap_register_gNB->num_plmn, 0);
    DevCheck(new_instance->tac == ngap_register_gNB->tac, new_instance->tac, ngap_register_gNB->tac, 0);

    for (int i = 0; i < new_instance->num_plmn; i++) {
      DevCheck(new_instance->mcc[i] == ngap_register_gNB->mcc[i], new_instance->mcc[i], ngap_register_gNB->mcc[i], 0);
      DevCheck(new_instance->mnc[i] == ngap_register_gNB->mnc[i], new_instance->mnc[i], ngap_register_gNB->mnc[i], 0);
      DevCheck(new_instance->mnc_digit_length[i] == ngap_register_gNB->mnc_digit_length[i], new_instance->mnc_digit_length[i], ngap_register_gNB->mnc_digit_length[i], 0);
    }

    DevCheck(new_instance->default_drx == ngap_register_gNB->default_drx, new_instance->default_drx, ngap_register_gNB->default_drx, 0);
  } else {
    new_instance = calloc(1, sizeof(ngap_gNB_instance_t));
    DevAssert(new_instance != NULL);
    RB_INIT(&new_instance->ngap_ue_head);
    RB_INIT(&new_instance->ngap_amf_head);
    /* Copy usefull parameters */
    new_instance->instance         = instance;
    new_instance->gNB_name         = ngap_register_gNB->gNB_name;
    new_instance->gNB_id           = ngap_register_gNB->gNB_id;
    new_instance->cell_type        = ngap_register_gNB->cell_type;
    new_instance->tac              = ngap_register_gNB->tac;
    
    memcpy(&new_instance->gNB_ng_ip,
       &ngap_register_gNB->gnb_ip_address,
       sizeof(ngap_register_gNB->gnb_ip_address));

    for (int i = 0; i < ngap_register_gNB->num_plmn; i++) {
      new_instance->mcc[i]              = ngap_register_gNB->mcc[i];
      new_instance->mnc[i]              = ngap_register_gNB->mnc[i];
      new_instance->mnc_digit_length[i] = ngap_register_gNB->mnc_digit_length[i];
      
      new_instance->num_nssai[i]        = ngap_register_gNB->num_nssai[i];
    }

    new_instance->num_plmn         = ngap_register_gNB->num_plmn;
    new_instance->default_drx      = ngap_register_gNB->default_drx;

    memcpy(new_instance->s_nssai, ngap_register_gNB->s_nssai, sizeof(ngap_register_gNB->s_nssai));

    /* Add the new instance to the list of gNB (meaningfull in virtual mode) */
    ngap_gNB_insert_new_instance(new_instance);
    NGAP_INFO("Registered new gNB[%ld] and %s gNB id %u\n",
              instance,
              ngap_register_gNB->cell_type == CELL_MACRO_GNB ? "macro" : "home",
              ngap_register_gNB->gNB_id);
  }

  DevCheck(ngap_register_gNB->nb_amf <= NGAP_MAX_NB_AMF_IP_ADDRESS,
           NGAP_MAX_NB_AMF_IP_ADDRESS, ngap_register_gNB->nb_amf, 0);

  /* Trying to connect to provided list of AMF ip address */
  for (index = 0; index < ngap_register_gNB->nb_amf; index++) {
    net_ip_address_t *amf_ip = &ngap_register_gNB->amf_ip_address[index];
    struct ngap_gNB_amf_data_s *amf = NULL;
    RB_FOREACH(amf, ngap_amf_map, &new_instance->ngap_amf_head) {
      /* Compare whether IPv4 and IPv6 information is already present, in which
       * wase we do not register again */
      if (amf->amf_s1_ip.ipv4 == amf_ip->ipv4 && (!amf_ip->ipv4
              || strncmp(amf->amf_s1_ip.ipv4_address, amf_ip->ipv4_address, 16) == 0)
          && amf->amf_s1_ip.ipv6 == amf_ip->ipv6 && (!amf_ip->ipv6
              || strncmp(amf->amf_s1_ip.ipv6_address, amf_ip->ipv6_address, 46) == 0))
        break;
    }
    if (amf)
      continue;
    ngap_gNB_register_amf(new_instance,
                          amf_ip,
                          &ngap_register_gNB->gnb_ip_address,
                          ngap_register_gNB->sctp_in_streams,
                          ngap_register_gNB->sctp_out_streams,
                          ngap_register_gNB->broadcast_plmn_num[index],
                          ngap_register_gNB->broadcast_plmn_index[index]);
  }
}

void ngap_gNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp) {
  ngap_gNB_instance_t *instance_p;
  ngap_gNB_amf_data_t *ngap_amf_data_p;
  DevAssert(sctp_new_association_resp != NULL);
  instance_p = ngap_gNB_get_instance(instance);
  DevAssert(instance_p != NULL);
  ngap_amf_data_p = ngap_gNB_get_AMF(instance_p, -1,
                                     sctp_new_association_resp->ulp_cnx_id);
  DevAssert(ngap_amf_data_p != NULL);

  if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
    NGAP_WARN("Received unsuccessful result for SCTP association (%u), instance %ld, cnx_id %u\n",
              sctp_new_association_resp->sctp_state,
              instance,
              sctp_new_association_resp->ulp_cnx_id);
    ngap_handle_ng_setup_message(ngap_amf_data_p, sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);
    return;
  }

  /* Update parameters */
  ngap_amf_data_p->assoc_id    = sctp_new_association_resp->assoc_id;
  ngap_amf_data_p->in_streams  = sctp_new_association_resp->in_streams;
  ngap_amf_data_p->out_streams = sctp_new_association_resp->out_streams;
  /* Prepare new NG Setup Request */
  ngap_gNB_generate_ng_setup_request(instance_p, ngap_amf_data_p);
}

static
void ngap_gNB_handle_sctp_data_ind(sctp_data_ind_t *sctp_data_ind) {
  int result;
  DevAssert(sctp_data_ind != NULL);
#if defined(TEST_S1C_AMF)
  amf_test_s1_notify_sctp_data_ind(sctp_data_ind->assoc_id, sctp_data_ind->stream,
                                   sctp_data_ind->buffer, sctp_data_ind->buffer_length);
#else
  ngap_gNB_handle_message(sctp_data_ind->assoc_id, sctp_data_ind->stream,
                          sctp_data_ind->buffer, sctp_data_ind->buffer_length);
#endif
  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
}

void ngap_gNB_init(void) {
  NGAP_DEBUG("Starting NGAP layer\n");
  ngap_gNB_prepare_internal_data();
  itti_mark_task_ready(TASK_NGAP);
  MSC_START_USE();
}

void *ngap_gNB_process_itti_msg(void *notUsed) {
  MessageDef *received_msg = NULL;
  int         result;
  itti_receive_msg(TASK_NGAP, &received_msg);

  switch (ITTI_MSG_ID(received_msg)) {
    case TERMINATE_MESSAGE:
      NGAP_WARN(" *** Exiting NGAP thread\n");
      itti_exit_task();
      break;

    case NGAP_REGISTER_GNB_REQ: {
      /* Register a new gNB.
       * in Virtual mode gNBs will be distinguished using the mod_id/
       * Each gNB has to send an NGAP_REGISTER_GNB message with its
       * own parameters.
       */
      ngap_gNB_handle_register_gNB(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                   &NGAP_REGISTER_GNB_REQ(received_msg));
    }
    break;

    case SCTP_NEW_ASSOCIATION_RESP: {
      ngap_gNB_handle_sctp_association_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                            &received_msg->ittiMsg.sctp_new_association_resp);
    }
    break;

    case SCTP_DATA_IND: {
      ngap_gNB_handle_sctp_data_ind(&received_msg->ittiMsg.sctp_data_ind);
    }
    break;

    case NGAP_NAS_FIRST_REQ: {
      ngap_gNB_handle_nas_first_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                    &NGAP_NAS_FIRST_REQ(received_msg));
    }
    break;

    case NGAP_UPLINK_NAS: {
      ngap_gNB_nas_uplink(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                          &NGAP_UPLINK_NAS(received_msg));
    }
    break;

    case NGAP_UE_CAPABILITIES_IND: {
      ngap_gNB_ue_capabilities(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                               &NGAP_UE_CAPABILITIES_IND(received_msg));
    }
    break;

    case NGAP_INITIAL_CONTEXT_SETUP_RESP: {
      ngap_gNB_initial_ctxt_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                 &NGAP_INITIAL_CONTEXT_SETUP_RESP(received_msg));
    }
    break;

    case NGAP_PDUSESSION_SETUP_RESP: {
      ngap_gNB_pdusession_setup_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                &NGAP_PDUSESSION_SETUP_RESP(received_msg));
    }
    break;

    case NGAP_PDUSESSION_MODIFY_RESP: {
      ngap_gNB_pdusession_modify_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                 &NGAP_PDUSESSION_MODIFY_RESP(received_msg));
    }
    break;

    case NGAP_NAS_NON_DELIVERY_IND: {
      ngap_gNB_nas_non_delivery_ind(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                    &NGAP_NAS_NON_DELIVERY_IND(received_msg));
    }
    break;

    case NGAP_PATH_SWITCH_REQ: {
      ngap_gNB_path_switch_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                               &NGAP_PATH_SWITCH_REQ(received_msg));
    }
    break;

    case NGAP_PDUSESSION_MODIFICATION_IND: {
    	ngap_gNB_generate_PDUSESSION_Modification_Indication(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
    	                               &NGAP_PDUSESSION_MODIFICATION_IND(received_msg));
    }
    break;

    case NGAP_UE_CONTEXT_RELEASE_COMPLETE: {
      ngap_ue_context_release_complete(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                       &NGAP_UE_CONTEXT_RELEASE_COMPLETE(received_msg));
    }
    break;

    case NGAP_UE_CONTEXT_RELEASE_REQ: {
      ngap_gNB_instance_t               *ngap_gNB_instance_p           = NULL; // test
      struct ngap_gNB_ue_context_s      *ue_context_p                  = NULL; // test
      ngap_ue_context_release_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                  &NGAP_UE_CONTEXT_RELEASE_REQ(received_msg));
      ngap_gNB_instance_p = ngap_gNB_get_instance(ITTI_MSG_DESTINATION_INSTANCE(received_msg)); // test
      DevAssert(ngap_gNB_instance_p != NULL); // test

      if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                          NGAP_UE_CONTEXT_RELEASE_REQ(received_msg).gNB_ue_ngap_id)) == NULL) { // test
        /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
        NGAP_ERROR("Failed to find ue context associated with gNB ue ngap id: %u\n",
                   NGAP_UE_CONTEXT_RELEASE_REQ(received_msg).gNB_ue_ngap_id); // test
      }  // test
    }
    break;

    case NGAP_PDUSESSION_RELEASE_RESPONSE: {
      ngap_gNB_pdusession_release_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                  &NGAP_PDUSESSION_RELEASE_RESPONSE(received_msg));
    }
    break;

    default:
      NGAP_ERROR("Received unhandled message: %d:%s\n",
                 ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
      break;
  }

  result = itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  received_msg = NULL;
  return NULL;
}


void *ngap_gNB_task(void *arg) {
  ngap_gNB_init();

  while (1) {
    (void) ngap_gNB_process_itti_msg(NULL);
  }

  return NULL;
}

//-----------------------------------------------------------------------------
/*
* gNB generate a NG setup request towards AMF
*/
static int ngap_gNB_generate_ng_setup_request(
  ngap_gNB_instance_t *instance_p,
  ngap_gNB_amf_data_t *ngap_amf_data_p)
//-----------------------------------------------------------------------------
{
  NGAP_NGAP_PDU_t            pdu;
  NGAP_NGSetupRequest_t      *out = NULL;
  NGAP_NGSetupRequestIEs_t   *ie = NULL;
  NGAP_SupportedTAItem_t     *ta = NULL;
  NGAP_BroadcastPLMNItem_t   *plmn = NULL;
  NGAP_SliceSupportItem_t    *ssi = NULL;
  uint8_t  *buffer = NULL;
  uint32_t  len = 0;
  int       ret = 0;
  DevAssert(instance_p != NULL);
  DevAssert(ngap_amf_data_p != NULL);
  ngap_amf_data_p->state = NGAP_GNB_STATE_WAITING;
  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = CALLOC(1, sizeof(struct NGAP_InitiatingMessage));
  pdu.choice.initiatingMessage->procedureCode = NGAP_ProcedureCode_id_NGSetup;
  pdu.choice.initiatingMessage->criticality = NGAP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = NGAP_InitiatingMessage__value_PR_NGSetupRequest;
  out = &pdu.choice.initiatingMessage->value.choice.NGSetupRequest;
  /* mandatory */
  ie = (NGAP_NGSetupRequestIEs_t *)calloc(1, sizeof(NGAP_NGSetupRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_GlobalRANNodeID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_NGSetupRequestIEs__value_PR_GlobalRANNodeID;
  ie->value.choice.GlobalRANNodeID.present = NGAP_GlobalRANNodeID_PR_globalGNB_ID;
  ie->value.choice.GlobalRANNodeID.choice.globalGNB_ID = CALLOC(1, sizeof(struct NGAP_GlobalGNB_ID));
  MCC_MNC_TO_PLMNID(instance_p->mcc[ngap_amf_data_p->broadcast_plmn_index[0]],
                    instance_p->mnc[ngap_amf_data_p->broadcast_plmn_index[0]],
                    instance_p->mnc_digit_length[ngap_amf_data_p->broadcast_plmn_index[0]],
                    &(ie->value.choice.GlobalRANNodeID.choice.globalGNB_ID->pLMNIdentity));
  ie->value.choice.GlobalRANNodeID.choice.globalGNB_ID->gNB_ID.present = NGAP_GNB_ID_PR_gNB_ID;
  MACRO_GNB_ID_TO_BIT_STRING(instance_p->gNB_id,
                             &ie->value.choice.GlobalRANNodeID.choice.globalGNB_ID->gNB_ID.choice.gNB_ID);
  NGAP_INFO("%u -> %02x%02x%02x%02x\n", instance_p->gNB_id,
            ie->value.choice.GlobalRANNodeID.choice.globalGNB_ID->gNB_ID.choice.gNB_ID.buf[0],
            ie->value.choice.GlobalRANNodeID.choice.globalGNB_ID->gNB_ID.choice.gNB_ID.buf[1],
            ie->value.choice.GlobalRANNodeID.choice.globalGNB_ID->gNB_ID.choice.gNB_ID.buf[2],
            ie->value.choice.GlobalRANNodeID.choice.globalGNB_ID->gNB_ID.choice.gNB_ID.buf[3]);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  if (instance_p->gNB_name) {
    ie = (NGAP_NGSetupRequestIEs_t *)calloc(1, sizeof(NGAP_NGSetupRequestIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_RANNodeName;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_NGSetupRequestIEs__value_PR_RANNodeName;
    OCTET_STRING_fromBuf(&ie->value.choice.RANNodeName, instance_p->gNB_name,
                         strlen(instance_p->gNB_name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  ie = (NGAP_NGSetupRequestIEs_t *)calloc(1, sizeof(NGAP_NGSetupRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_SupportedTAList;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_NGSetupRequestIEs__value_PR_SupportedTAList;
  {
    ta = (NGAP_SupportedTAItem_t *)calloc(1, sizeof(NGAP_SupportedTAItem_t));
    INT24_TO_OCTET_STRING(instance_p->tac, &ta->tAC);
    {
      for (int i = 0; i < ngap_amf_data_p->broadcast_plmn_num; ++i) {
        plmn = (NGAP_BroadcastPLMNItem_t *)calloc(1, sizeof(NGAP_BroadcastPLMNItem_t));
        MCC_MNC_TO_TBCD(instance_p->mcc[ngap_amf_data_p->broadcast_plmn_index[i]],
                        instance_p->mnc[ngap_amf_data_p->broadcast_plmn_index[i]],
                        instance_p->mnc_digit_length[ngap_amf_data_p->broadcast_plmn_index[i]],
                        &plmn->pLMNIdentity);

        for(int si = 0; si < instance_p->num_nssai[i]; si++) {
          ssi = (NGAP_SliceSupportItem_t *)calloc(1, sizeof(NGAP_SliceSupportItem_t));
          INT8_TO_OCTET_STRING(instance_p->s_nssai[i][si].sST, &ssi->s_NSSAI.sST);

          if(instance_p->s_nssai[i]->sD_flag) {
            ssi->s_NSSAI.sD = calloc(1, sizeof(NGAP_SD_t));
            ssi->s_NSSAI.sD->buf = calloc(3, sizeof(uint8_t));
            ssi->s_NSSAI.sD->size = 3;
            ssi->s_NSSAI.sD->buf[0] = instance_p->s_nssai[i][si].sD[0];
            ssi->s_NSSAI.sD->buf[1] = instance_p->s_nssai[i][si].sD[1];
            ssi->s_NSSAI.sD->buf[2] = instance_p->s_nssai[i][si].sD[2];
          }
          

          ASN_SEQUENCE_ADD(&plmn->tAISliceSupportList.list, ssi);
        }
        
        ASN_SEQUENCE_ADD(&ta->broadcastPLMNList.list, plmn);
      }
    }
    ASN_SEQUENCE_ADD(&ie->value.choice.SupportedTAList.list, ta);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  
  /* mandatory */
  ie = (NGAP_NGSetupRequestIEs_t *)calloc(1, sizeof(NGAP_NGSetupRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_DefaultPagingDRX;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_NGSetupRequestIEs__value_PR_PagingDRX;
  ie->value.choice.PagingDRX = instance_p->default_drx;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (ngap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    NGAP_ERROR("Failed to encode NG setup request\n");
    return -1;
  }

  /* Non UE-Associated signalling -> stream = 0 */
  ngap_gNB_itti_send_sctp_data_req(instance_p->instance, ngap_amf_data_p->assoc_id, buffer, len, 0);
  return ret;
}




