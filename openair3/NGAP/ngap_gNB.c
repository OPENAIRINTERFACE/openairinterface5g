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
 * \author  S. Roux and Navid Nikaein
 * \date 2010 - 2015
 * \email: navid.nikaein@eurecom.fr
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
#if defined(TEST_S1C_MME)
  #include "oaisim_mme_test_s1c.h"
#endif

ngap_gNB_config_t ngap_config;

static int ngap_gNB_generate_s1_setup_request(
  ngap_gNB_instance_t *instance_p, ngap_gNB_mme_data_t *ngap_mme_data_p);

void ngap_gNB_handle_register_gNB(instance_t instance, ngap_register_enb_req_t *ngap_register_gNB);

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

static void ngap_gNB_register_mme(ngap_gNB_instance_t *instance_p,
                                  net_ip_address_t    *mme_ip_address,
                                  net_ip_address_t    *local_ip_addr,
                                  uint16_t             in_streams,
                                  uint16_t             out_streams,
                                  uint8_t              broadcast_plmn_num,
                                  uint8_t              broadcast_plmn_index[PLMN_LIST_MAX_SIZE]) {
  MessageDef                 *message_p                   = NULL;
  sctp_new_association_req_t *sctp_new_association_req_p  = NULL;
  ngap_gNB_mme_data_t        *ngap_mme_data_p             = NULL;
  struct ngap_gNB_mme_data_s *mme                         = NULL;
  DevAssert(instance_p != NULL);
  DevAssert(mme_ip_address != NULL);
  message_p = itti_alloc_new_message(TASK_NGAP, SCTP_NEW_ASSOCIATION_REQ);
  sctp_new_association_req_p = &message_p->ittiMsg.sctp_new_association_req;
  sctp_new_association_req_p->port = NGAP_PORT_NUMBER;
  sctp_new_association_req_p->ppid = NGAP_SCTP_PPID;
  sctp_new_association_req_p->in_streams  = in_streams;
  sctp_new_association_req_p->out_streams = out_streams;
  memcpy(&sctp_new_association_req_p->remote_address,
         mme_ip_address,
         sizeof(*mme_ip_address));
  memcpy(&sctp_new_association_req_p->local_address,
         local_ip_addr,
         sizeof(*local_ip_addr));
  NGAP_INFO("[gNB %d] check the mme registration state\n",instance_p->instance);
  mme = NULL;

  if ( mme == NULL ) {
    /* Create new MME descriptor */
    ngap_mme_data_p = calloc(1, sizeof(*ngap_mme_data_p));
    DevAssert(ngap_mme_data_p != NULL);
    ngap_mme_data_p->cnx_id                = ngap_gNB_fetch_add_global_cnx_id();
    sctp_new_association_req_p->ulp_cnx_id = ngap_mme_data_p->cnx_id;
    ngap_mme_data_p->assoc_id          = -1;
    ngap_mme_data_p->broadcast_plmn_num = broadcast_plmn_num;
    memcpy(&ngap_mme_data_p->mme_s1_ip,
    	   mme_ip_address,
    	   sizeof(*mme_ip_address));
    for (int i = 0; i < broadcast_plmn_num; ++i)
      ngap_mme_data_p->broadcast_plmn_index[i] = broadcast_plmn_index[i];

    ngap_mme_data_p->ngap_gNB_instance = instance_p;
    STAILQ_INIT(&ngap_mme_data_p->served_gummei);
    /* Insert the new descriptor in list of known MME
     * but not yet associated.
     */
    RB_INSERT(ngap_mme_map, &instance_p->ngap_mme_head, ngap_mme_data_p);
    ngap_mme_data_p->state = NGAP_GNB_STATE_WAITING;
    instance_p->ngap_mme_nb ++;
    instance_p->ngap_mme_pending_nb ++;
  } else if (mme->state == NGAP_GNB_STATE_WAITING) {
    instance_p->ngap_mme_pending_nb ++;
    sctp_new_association_req_p->ulp_cnx_id = mme->cnx_id;
    NGAP_INFO("[gNB %d] MME already registered, retrive the data (state %d, cnx %d, mme_nb %d, mme_pending_nb %d)\n",
              instance_p->instance,
              mme->state, mme->cnx_id,
              instance_p->ngap_mme_nb, instance_p->ngap_mme_pending_nb);
    /*ngap_mme_data_p->cnx_id                = mme->cnx_id;
    sctp_new_association_req_p->ulp_cnx_id = mme->cnx_id;

    ngap_mme_data_p->assoc_id          = -1;
    ngap_mme_data_p->ngap_gNB_instance = instance_p;
    */
  } else {
    NGAP_WARN("[gNB %d] MME already registered but not in the waiting state, retrive the data (state %d, cnx %d, mme_nb %d, mme_pending_nb %d)\n",
              instance_p->instance,
              mme->state, mme->cnx_id,
              instance_p->ngap_mme_nb, instance_p->ngap_mme_pending_nb);
  }

  itti_send_msg_to_task(TASK_SCTP, instance_p->instance, message_p);
}


void ngap_gNB_handle_register_gNB(instance_t instance, ngap_register_enb_req_t *ngap_register_gNB) {
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
    RB_INIT(&new_instance->ngap_mme_head);
    /* Copy usefull parameters */
    new_instance->instance         = instance;
    new_instance->gNB_name         = ngap_register_gNB->gNB_name;
    new_instance->gNB_id           = ngap_register_gNB->gNB_id;
    new_instance->cell_type        = ngap_register_gNB->cell_type;
    new_instance->tac              = ngap_register_gNB->tac;
    
    memcpy(&new_instance->gNB_s1_ip,
	   &ngap_register_gNB->enb_ip_address,
	   sizeof(ngap_register_gNB->enb_ip_address));

    for (int i = 0; i < ngap_register_gNB->num_plmn; i++) {
      new_instance->mcc[i]              = ngap_register_gNB->mcc[i];
      new_instance->mnc[i]              = ngap_register_gNB->mnc[i];
      new_instance->mnc_digit_length[i] = ngap_register_gNB->mnc_digit_length[i];
    }

    new_instance->num_plmn         = ngap_register_gNB->num_plmn;
    new_instance->default_drx      = ngap_register_gNB->default_drx;
    /* Add the new instance to the list of gNB (meaningfull in virtual mode) */
    ngap_gNB_insert_new_instance(new_instance);
    NGAP_INFO("Registered new gNB[%d] and %s gNB id %u\n",
              instance,
              ngap_register_gNB->cell_type == CELL_MACRO_GNB ? "macro" : "home",
              ngap_register_gNB->gNB_id);
  }

  DevCheck(ngap_register_gNB->nb_mme <= NGAP_MAX_NB_MME_IP_ADDRESS,
           NGAP_MAX_NB_MME_IP_ADDRESS, ngap_register_gNB->nb_mme, 0);

  /* Trying to connect to provided list of MME ip address */
  for (index = 0; index < ngap_register_gNB->nb_mme; index++) {
    net_ip_address_t *mme_ip = &ngap_register_gNB->mme_ip_address[index];
    struct ngap_gNB_mme_data_s *mme = NULL;
    RB_FOREACH(mme, ngap_mme_map, &new_instance->ngap_mme_head) {
      /* Compare whether IPv4 and IPv6 information is already present, in which
       * wase we do not register again */
      if (mme->mme_s1_ip.ipv4 == mme_ip->ipv4 && (!mme_ip->ipv4
              || strncmp(mme->mme_s1_ip.ipv4_address, mme_ip->ipv4_address, 16) == 0)
          && mme->mme_s1_ip.ipv6 == mme_ip->ipv6 && (!mme_ip->ipv6
              || strncmp(mme->mme_s1_ip.ipv6_address, mme_ip->ipv6_address, 46) == 0))
        break;
    }
    if (mme)
      continue;
    ngap_gNB_register_mme(new_instance,
                          mme_ip,
                          &ngap_register_gNB->enb_ip_address,
                          ngap_register_gNB->sctp_in_streams,
                          ngap_register_gNB->sctp_out_streams,
                          ngap_register_gNB->broadcast_plmn_num[index],
                          ngap_register_gNB->broadcast_plmn_index[index]);
  }
}

void ngap_gNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp) {
  ngap_gNB_instance_t *instance_p;
  ngap_gNB_mme_data_t *ngap_mme_data_p;
  DevAssert(sctp_new_association_resp != NULL);
  instance_p = ngap_gNB_get_instance(instance);
  DevAssert(instance_p != NULL);
  ngap_mme_data_p = ngap_gNB_get_MME(instance_p, -1,
                                     sctp_new_association_resp->ulp_cnx_id);
  DevAssert(ngap_mme_data_p != NULL);

  if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
    NGAP_WARN("Received unsuccessful result for SCTP association (%u), instance %d, cnx_id %u\n",
              sctp_new_association_resp->sctp_state,
              instance,
              sctp_new_association_resp->ulp_cnx_id);
    ngap_handle_s1_setup_message(ngap_mme_data_p, sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);
    return;
  }

  /* Update parameters */
  ngap_mme_data_p->assoc_id    = sctp_new_association_resp->assoc_id;
  ngap_mme_data_p->in_streams  = sctp_new_association_resp->in_streams;
  ngap_mme_data_p->out_streams = sctp_new_association_resp->out_streams;
  /* Prepare new S1 Setup Request */
  ngap_gNB_generate_s1_setup_request(instance_p, ngap_mme_data_p);
}

static
void ngap_gNB_handle_sctp_data_ind(sctp_data_ind_t *sctp_data_ind) {
  int result;
  DevAssert(sctp_data_ind != NULL);
#if defined(TEST_S1C_MME)
  mme_test_s1_notify_sctp_data_ind(sctp_data_ind->assoc_id, sctp_data_ind->stream,
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
      ngap_gNB_handle_register_gNB(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                   &NGAP_REGISTER_GNB_REQ(received_msg));
    }
    break;

    case SCTP_NEW_ASSOCIATION_RESP: {
      ngap_gNB_handle_sctp_association_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                            &received_msg->ittiMsg.sctp_new_association_resp);
    }
    break;

    case SCTP_DATA_IND: {
      ngap_gNB_handle_sctp_data_ind(&received_msg->ittiMsg.sctp_data_ind);
    }
    break;

    case NGAP_NAS_FIRST_REQ: {
      ngap_gNB_handle_nas_first_req(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                    &NGAP_NAS_FIRST_REQ(received_msg));
    }
    break;

    case NGAP_UPLINK_NAS: {
      ngap_gNB_nas_uplink(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                          &NGAP_UPLINK_NAS(received_msg));
    }
    break;

    case NGAP_UE_CAPABILITIES_IND: {
      ngap_gNB_ue_capabilities(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                               &NGAP_UE_CAPABILITIES_IND(received_msg));
    }
    break;

    case NGAP_INITIAL_CONTEXT_SETUP_RESP: {
      ngap_gNB_initial_ctxt_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                 &NGAP_INITIAL_CONTEXT_SETUP_RESP(received_msg));
    }
    break;

    case NGAP_E_RAB_SETUP_RESP: {
      ngap_gNB_e_rab_setup_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                &NGAP_E_RAB_SETUP_RESP(received_msg));
    }
    break;

    case NGAP_E_RAB_MODIFY_RESP: {
      ngap_gNB_e_rab_modify_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                 &NGAP_E_RAB_MODIFY_RESP(received_msg));
    }
    break;

    case NGAP_NAS_NON_DELIVERY_IND: {
      ngap_gNB_nas_non_delivery_ind(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                    &NGAP_NAS_NON_DELIVERY_IND(received_msg));
    }
    break;

    case NGAP_PATH_SWITCH_REQ: {
      ngap_gNB_path_switch_req(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                               &NGAP_PATH_SWITCH_REQ(received_msg));
    }
    break;

    case NGAP_E_RAB_MODIFICATION_IND: {
    	ngap_gNB_generate_E_RAB_Modification_Indication(ITTI_MESSAGE_GET_INSTANCE(received_msg),
    	                               &NGAP_E_RAB_MODIFICATION_IND(received_msg));
    }
    break;

    case NGAP_UE_CONTEXT_RELEASE_COMPLETE: {
      ngap_ue_context_release_complete(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                       &NGAP_UE_CONTEXT_RELEASE_COMPLETE(received_msg));
    }
    break;

    case NGAP_UE_CONTEXT_RELEASE_REQ: {
      ngap_gNB_instance_t               *ngap_gNB_instance_p           = NULL; // test
      struct ngap_gNB_ue_context_s      *ue_context_p                  = NULL; // test
      ngap_ue_context_release_req(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                  &NGAP_UE_CONTEXT_RELEASE_REQ(received_msg));
      ngap_gNB_instance_p = ngap_gNB_get_instance(ITTI_MESSAGE_GET_INSTANCE(received_msg)); // test
      DevAssert(ngap_gNB_instance_p != NULL); // test

      if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                          NGAP_UE_CONTEXT_RELEASE_REQ(received_msg).gNB_ue_ngap_id)) == NULL) { // test
        /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
        NGAP_ERROR("Failed to find ue context associated with gNB ue ngap id: %u\n",
                   NGAP_UE_CONTEXT_RELEASE_REQ(received_msg).gNB_ue_ngap_id); // test
      }  // test
    }
    break;

    case NGAP_E_RAB_RELEASE_RESPONSE: {
      ngap_gNB_e_rab_release_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                  &NGAP_E_RAB_RELEASE_RESPONSE(received_msg));
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
* gNB generate a S1 setup request towards MME
*/
static int ngap_gNB_generate_s1_setup_request(
  ngap_gNB_instance_t *instance_p,
  ngap_gNB_mme_data_t *ngap_mme_data_p)
//-----------------------------------------------------------------------------
{
  NGAP_NGAP_PDU_t            pdu;
  NGAP_S1SetupRequest_t     *out = NULL;
  NGAP_S1SetupRequestIEs_t   *ie = NULL;
  NGAP_SupportedTAs_Item_t   *ta = NULL;
  NGAP_PLMNidentity_t      *plmn = NULL;
  uint8_t  *buffer = NULL;
  uint32_t  len = 0;
  int       ret = 0;
  DevAssert(instance_p != NULL);
  DevAssert(ngap_mme_data_p != NULL);
  ngap_mme_data_p->state = NGAP_GNB_STATE_WAITING;
  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = NGAP_ProcedureCode_id_S1Setup;
  pdu.choice.initiatingMessage.criticality = NGAP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = NGAP_InitiatingMessage__value_PR_S1SetupRequest;
  out = &pdu.choice.initiatingMessage.value.choice.S1SetupRequest;
  /* mandatory */
  ie = (NGAP_S1SetupRequestIEs_t *)calloc(1, sizeof(NGAP_S1SetupRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_Global_GNB_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_S1SetupRequestIEs__value_PR_Global_GNB_ID;
  MCC_MNC_TO_PLMNID(instance_p->mcc[ngap_mme_data_p->broadcast_plmn_index[0]],
                    instance_p->mnc[ngap_mme_data_p->broadcast_plmn_index[0]],
                    instance_p->mnc_digit_length[ngap_mme_data_p->broadcast_plmn_index[0]],
                    &ie->value.choice.Global_GNB_ID.pLMNidentity);
  ie->value.choice.Global_GNB_ID.gNB_ID.present = NGAP_GNB_ID_PR_macroGNB_ID;
  MACRO_GNB_ID_TO_BIT_STRING(instance_p->gNB_id,
                             &ie->value.choice.Global_GNB_ID.gNB_ID.choice.macroGNB_ID);
  NGAP_INFO("%d -> %02x%02x%02x\n", instance_p->gNB_id,
            ie->value.choice.Global_GNB_ID.gNB_ID.choice.macroGNB_ID.buf[0],
            ie->value.choice.Global_GNB_ID.gNB_ID.choice.macroGNB_ID.buf[1],
            ie->value.choice.Global_GNB_ID.gNB_ID.choice.macroGNB_ID.buf[2]);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  if (instance_p->gNB_name) {
    ie = (NGAP_S1SetupRequestIEs_t *)calloc(1, sizeof(NGAP_S1SetupRequestIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_gNBname;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_S1SetupRequestIEs__value_PR_GNBname;
    OCTET_STRING_fromBuf(&ie->value.choice.GNBname, instance_p->gNB_name,
                         strlen(instance_p->gNB_name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  ie = (NGAP_S1SetupRequestIEs_t *)calloc(1, sizeof(NGAP_S1SetupRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_SupportedTAs;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_S1SetupRequestIEs__value_PR_SupportedTAs;
  {
    ta = (NGAP_SupportedTAs_Item_t *)calloc(1, sizeof(NGAP_SupportedTAs_Item_t));
    INT16_TO_OCTET_STRING(instance_p->tac, &ta->tAC);
    {
      for (int i = 0; i < ngap_mme_data_p->broadcast_plmn_num; ++i) {
        plmn = (NGAP_PLMNidentity_t *)calloc(1, sizeof(NGAP_PLMNidentity_t));
        MCC_MNC_TO_TBCD(instance_p->mcc[ngap_mme_data_p->broadcast_plmn_index[i]],
                        instance_p->mnc[ngap_mme_data_p->broadcast_plmn_index[i]],
                        instance_p->mnc_digit_length[ngap_mme_data_p->broadcast_plmn_index[i]],
                        plmn);
        ASN_SEQUENCE_ADD(&ta->broadcastPLMNs.list, plmn);
      }
    }
    ASN_SEQUENCE_ADD(&ie->value.choice.SupportedTAs.list, ta);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_S1SetupRequestIEs_t *)calloc(1, sizeof(NGAP_S1SetupRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_DefaultPagingDRX;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_S1SetupRequestIEs__value_PR_PagingDRX;
  ie->value.choice.PagingDRX = instance_p->default_drx;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (ngap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    NGAP_ERROR("Failed to encode S1 setup request\n");
    return -1;
  }

  /* Non UE-Associated signalling -> stream = 0 */
  ngap_gNB_itti_send_sctp_data_req(instance_p->instance, ngap_mme_data_p->assoc_id, buffer, len, 0);
  return ret;
}




