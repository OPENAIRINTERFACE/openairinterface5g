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

/*! \file s1ap_eNB.c
 * \brief S1AP eNB task 
 * \author  S. Roux and Navid Nikaein 
 * \date 2010 - 2015
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _s1ap
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <crypt.h>

#include "tree.h"
#include "queue.h"

#include "intertask_interface.h"

#include "s1ap_eNB_default_values.h"

#include "s1ap_common.h"
#include "s1ap_ies_defs.h"

#include "s1ap_eNB_defs.h"
#include "s1ap_eNB.h"
#include "s1ap_eNB_encoder.h"
#include "s1ap_eNB_handlers.h"
#include "s1ap_eNB_nnsf.h"

#include "s1ap_eNB_nas_procedures.h"
#include "s1ap_eNB_management_procedures.h"
#include "s1ap_eNB_context_management_procedures.h"

#include "s1ap_eNB_itti_messaging.h"

#include "s1ap_eNB_ue_context.h" // test, to be removed
#include "msc.h"

#include "assertions.h"
#include "conversions.h"
#if defined(TEST_S1C_MME)
#include "oaisim_mme_test_s1c.h"
#endif

s1ap_eNB_config_t s1ap_config;

static int s1ap_eNB_generate_s1_setup_request(
  s1ap_eNB_instance_t *instance_p, s1ap_eNB_mme_data_t *s1ap_mme_data_p);


void s1ap_eNB_handle_register_eNB(instance_t instance, s1ap_register_enb_req_t *s1ap_register_eNB);

void s1ap_eNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp);

uint32_t s1ap_generate_eNB_id(void)
{
  char    *out;
  char     hostname[50];
  int      ret;
  uint32_t eNB_id;

  /* Retrieve the host name */
  ret = gethostname(hostname, sizeof(hostname));
  DevAssert(ret == 0);

  out = crypt(hostname, "eurecom");
  DevAssert(out != NULL);

  eNB_id = ((out[0] << 24) | (out[1] << 16) | (out[2] << 8) | out[3]);

  return eNB_id;
}

static void s1ap_eNB_register_mme(s1ap_eNB_instance_t *instance_p,
                                  net_ip_address_t    *mme_ip_address,
                                  net_ip_address_t    *local_ip_addr,
                                  uint16_t             in_streams,
                                  uint16_t             out_streams)
{
  MessageDef                 *message_p                   = NULL;
  sctp_new_association_req_t *sctp_new_association_req_p  = NULL;
  s1ap_eNB_mme_data_t        *s1ap_mme_data_p             = NULL;
  struct s1ap_eNB_mme_data_s *mme                         = NULL;
 
  DevAssert(instance_p != NULL);
  DevAssert(mme_ip_address != NULL);

  message_p = itti_alloc_new_message(TASK_S1AP, SCTP_NEW_ASSOCIATION_REQ);

  sctp_new_association_req_p = &message_p->ittiMsg.sctp_new_association_req;

  sctp_new_association_req_p->port = S1AP_PORT_NUMBER;
  sctp_new_association_req_p->ppid = S1AP_SCTP_PPID;

  sctp_new_association_req_p->in_streams  = in_streams;
  sctp_new_association_req_p->out_streams = out_streams;

  memcpy(&sctp_new_association_req_p->remote_address,
         mme_ip_address,
         sizeof(*mme_ip_address));

  memcpy(&sctp_new_association_req_p->local_address,
         local_ip_addr,
         sizeof(*local_ip_addr));
 
  S1AP_INFO("[eNB %d] check the mme registration state\n",instance_p->instance);
	      
  mme = s1ap_eNB_get_MME_from_instance(instance_p);

  if ( mme == NULL ) {
    
    /* Create new MME descriptor */
    s1ap_mme_data_p = calloc(1, sizeof(*s1ap_mme_data_p));
    DevAssert(s1ap_mme_data_p != NULL);
    
    s1ap_mme_data_p->cnx_id                = s1ap_eNB_fetch_add_global_cnx_id();
    sctp_new_association_req_p->ulp_cnx_id = s1ap_mme_data_p->cnx_id;
    
    s1ap_mme_data_p->assoc_id          = -1;
    s1ap_mme_data_p->s1ap_eNB_instance = instance_p;
    
    STAILQ_INIT(&s1ap_mme_data_p->served_gummei);
  
    /* Insert the new descriptor in list of known MME
     * but not yet associated.
     */
    RB_INSERT(s1ap_mme_map, &instance_p->s1ap_mme_head, s1ap_mme_data_p);
    s1ap_mme_data_p->state = S1AP_ENB_STATE_WAITING;
    instance_p->s1ap_mme_nb ++;
    instance_p->s1ap_mme_pending_nb ++;
  } else if (mme->state == S1AP_ENB_STATE_WAITING) {
    instance_p->s1ap_mme_pending_nb ++;
    sctp_new_association_req_p->ulp_cnx_id = mme->cnx_id;

    S1AP_INFO("[eNB %d] MME already registered, retrive the data (state %d, cnx %d, mme_nb %d, mme_pending_nb %d)\n",
	      instance_p->instance, 
	      mme->state, mme->cnx_id,
	      instance_p->s1ap_mme_nb, instance_p->s1ap_mme_pending_nb);

    /*s1ap_mme_data_p->cnx_id                = mme->cnx_id;
    sctp_new_association_req_p->ulp_cnx_id = mme->cnx_id;
    
    s1ap_mme_data_p->assoc_id          = -1;
    s1ap_mme_data_p->s1ap_eNB_instance = instance_p;
    */ 
  } else {
    
    S1AP_WARN("[eNB %d] MME already registered but not in the waiting state, retrive the data (state %d, cnx %d, mme_nb %d, mme_pending_nb %d)\n",
	      instance_p->instance, 
	      mme->state, mme->cnx_id, 
	      instance_p->s1ap_mme_nb, instance_p->s1ap_mme_pending_nb);
    
  }
    
  itti_send_msg_to_task(TASK_SCTP, instance_p->instance, message_p);
}


void s1ap_eNB_handle_register_eNB(instance_t instance, s1ap_register_enb_req_t *s1ap_register_eNB)
{
  s1ap_eNB_instance_t *new_instance;
  uint8_t index;
  
  DevAssert(s1ap_register_eNB != NULL);

  /* Look if the provided instance already exists */
  new_instance = s1ap_eNB_get_instance(instance);
  
 
  if (new_instance != NULL) { 
    /* Checks if it is a retry on the same eNB */
    DevCheck(new_instance->eNB_id == s1ap_register_eNB->eNB_id, new_instance->eNB_id, s1ap_register_eNB->eNB_id, 0);
    DevCheck(new_instance->cell_type == s1ap_register_eNB->cell_type, new_instance->cell_type, s1ap_register_eNB->cell_type, 0);
    DevCheck(new_instance->tac == s1ap_register_eNB->tac, new_instance->tac, s1ap_register_eNB->tac, 0);
    DevCheck(new_instance->mcc == s1ap_register_eNB->mcc, new_instance->mcc, s1ap_register_eNB->mcc, 0);
    DevCheck(new_instance->mnc == s1ap_register_eNB->mnc, new_instance->mnc, s1ap_register_eNB->mnc, 0);
    DevCheck(new_instance->mnc_digit_length == s1ap_register_eNB->mnc_digit_length, new_instance->mnc_digit_length, s1ap_register_eNB->mnc_digit_length, 0);
    DevCheck(new_instance->default_drx == s1ap_register_eNB->default_drx, new_instance->default_drx, s1ap_register_eNB->default_drx, 0);
  } else {
    new_instance = calloc(1, sizeof(s1ap_eNB_instance_t));
    DevAssert(new_instance != NULL);

    RB_INIT(&new_instance->s1ap_ue_head);
    RB_INIT(&new_instance->s1ap_mme_head);

    /* Copy usefull parameters */
    new_instance->instance         = instance;
    new_instance->eNB_name         = s1ap_register_eNB->eNB_name;
    new_instance->eNB_id           = s1ap_register_eNB->eNB_id;
    new_instance->cell_type        = s1ap_register_eNB->cell_type;
    new_instance->tac              = s1ap_register_eNB->tac;
    new_instance->mcc              = s1ap_register_eNB->mcc;
    new_instance->mnc              = s1ap_register_eNB->mnc;
    new_instance->mnc_digit_length = s1ap_register_eNB->mnc_digit_length;
    new_instance->default_drx      = s1ap_register_eNB->default_drx;

    /* Add the new instance to the list of eNB (meaningfull in virtual mode) */
    s1ap_eNB_insert_new_instance(new_instance);

    S1AP_INFO("Registered new eNB[%d] and %s eNB id %u\n",
               instance,
               s1ap_register_eNB->cell_type == CELL_MACRO_ENB ? "macro" : "home",
               s1ap_register_eNB->eNB_id);
  }

  DevCheck(s1ap_register_eNB->nb_mme <= S1AP_MAX_NB_MME_IP_ADDRESS,
           S1AP_MAX_NB_MME_IP_ADDRESS, s1ap_register_eNB->nb_mme, 0);

  /* Trying to connect to provided list of MME ip address */
  for (index = 0; index < s1ap_register_eNB->nb_mme; index++) {
    s1ap_eNB_register_mme(new_instance,
    		          &s1ap_register_eNB->mme_ip_address[index],
                          &s1ap_register_eNB->enb_ip_address,
                          s1ap_register_eNB->sctp_in_streams,
                          s1ap_register_eNB->sctp_out_streams);
  }
}

void s1ap_eNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp)
{
  s1ap_eNB_instance_t *instance_p;
  s1ap_eNB_mme_data_t *s1ap_mme_data_p;

  DevAssert(sctp_new_association_resp != NULL);

  instance_p = s1ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  s1ap_mme_data_p = s1ap_eNB_get_MME(instance_p, -1,
                                     sctp_new_association_resp->ulp_cnx_id);
  DevAssert(s1ap_mme_data_p != NULL);

  if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
    S1AP_WARN("Received unsuccessful result for SCTP association (%u), instance %d, cnx_id %u\n",
              sctp_new_association_resp->sctp_state,
              instance,
              sctp_new_association_resp->ulp_cnx_id);

    s1ap_handle_s1_setup_message(s1ap_mme_data_p, sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);

    return;
  }

  /* Update parameters */
  s1ap_mme_data_p->assoc_id    = sctp_new_association_resp->assoc_id;
  s1ap_mme_data_p->in_streams  = sctp_new_association_resp->in_streams;
  s1ap_mme_data_p->out_streams = sctp_new_association_resp->out_streams;

  /* Prepare new S1 Setup Request */
  s1ap_eNB_generate_s1_setup_request(instance_p, s1ap_mme_data_p);
}

static
void s1ap_eNB_handle_sctp_data_ind(sctp_data_ind_t *sctp_data_ind)
{
  int result;

  DevAssert(sctp_data_ind != NULL);
#if defined(TEST_S1C_MME)
  mme_test_s1_notify_sctp_data_ind(sctp_data_ind->assoc_id, sctp_data_ind->stream,
          sctp_data_ind->buffer, sctp_data_ind->buffer_length);
#else
  s1ap_eNB_handle_message(sctp_data_ind->assoc_id, sctp_data_ind->stream,
                          sctp_data_ind->buffer, sctp_data_ind->buffer_length);
#endif
  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
}

void *s1ap_eNB_task(void *arg)
{
  MessageDef *received_msg = NULL;
  int         result;

  S1AP_DEBUG("Starting S1AP layer\n");

  s1ap_eNB_prepare_internal_data();

  itti_mark_task_ready(TASK_S1AP);
  MSC_START_USE();

  while (1) {
    itti_receive_msg(TASK_S1AP, &received_msg);

    switch (ITTI_MSG_ID(received_msg)) {
    case TERMINATE_MESSAGE:
      S1AP_WARN(" *** Exiting S1AP thread\n");
      itti_exit_task();
      break;

    case S1AP_REGISTER_ENB_REQ: {
      /* Register a new eNB.
       * in Virtual mode eNBs will be distinguished using the mod_id/
       * Each eNB has to send an S1AP_REGISTER_ENB message with its
       * own parameters.
       */
      s1ap_eNB_handle_register_eNB(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                   &S1AP_REGISTER_ENB_REQ(received_msg));
    }
    break;

    case SCTP_NEW_ASSOCIATION_RESP: {
      s1ap_eNB_handle_sctp_association_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                            &received_msg->ittiMsg.sctp_new_association_resp);
    }
    break;

    case SCTP_DATA_IND: {
      s1ap_eNB_handle_sctp_data_ind(&received_msg->ittiMsg.sctp_data_ind);
    }
    break;

    case S1AP_NAS_FIRST_REQ: {
      s1ap_eNB_handle_nas_first_req(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                    &S1AP_NAS_FIRST_REQ(received_msg));
    }
    break;

    case S1AP_UPLINK_NAS: {
      s1ap_eNB_nas_uplink(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                          &S1AP_UPLINK_NAS(received_msg));
    }
    break;

    case S1AP_UE_CAPABILITIES_IND: {
      s1ap_eNB_ue_capabilities(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                               &S1AP_UE_CAPABILITIES_IND(received_msg));
    }
    break;

    case S1AP_INITIAL_CONTEXT_SETUP_RESP: {
      s1ap_eNB_initial_ctxt_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                 &S1AP_INITIAL_CONTEXT_SETUP_RESP(received_msg));
    }
    break;

    case S1AP_E_RAB_SETUP_RESP: {
      s1ap_eNB_e_rab_setup_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
				&S1AP_E_RAB_SETUP_RESP(received_msg));
    }
    break;

    case S1AP_E_RAB_MODIFY_RESP: {
      s1ap_eNB_e_rab_modify_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
        &S1AP_E_RAB_MODIFY_RESP(received_msg));
    }
    break;
      
    case S1AP_NAS_NON_DELIVERY_IND: {
      s1ap_eNB_nas_non_delivery_ind(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                    &S1AP_NAS_NON_DELIVERY_IND(received_msg));
    }
    break;

    case S1AP_UE_CONTEXT_RELEASE_COMPLETE: {
      s1ap_ue_context_release_complete(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                       &S1AP_UE_CONTEXT_RELEASE_COMPLETE(received_msg));
    }
    break;

    case S1AP_UE_CONTEXT_RELEASE_REQ: {
      s1ap_eNB_instance_t               *s1ap_eNB_instance_p           = NULL; // test
      struct s1ap_eNB_ue_context_s      *ue_context_p                  = NULL; // test

      s1ap_ue_context_release_req(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                  &S1AP_UE_CONTEXT_RELEASE_REQ(received_msg));


      s1ap_eNB_instance_p = s1ap_eNB_get_instance(ITTI_MESSAGE_GET_INSTANCE(received_msg)); // test
      DevAssert(s1ap_eNB_instance_p != NULL); // test

      if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p,
                          S1AP_UE_CONTEXT_RELEASE_REQ(received_msg).eNB_ue_s1ap_id)) == NULL) { // test
        /* The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs */
        S1AP_ERROR("Failed to find ue context associated with eNB ue s1ap id: %u\n",
                   S1AP_UE_CONTEXT_RELEASE_REQ(received_msg).eNB_ue_s1ap_id); // test
      }  // test
    }
    break;

   case S1AP_E_RAB_RELEASE_RESPONSE: {
        s1ap_eNB_e_rab_release_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                    &S1AP_E_RAB_RELEASE_RESPONSE(received_msg));
    }
    break;

    default:
      S1AP_ERROR("Received unhandled message: %d:%s\n",
                 ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);

    received_msg = NULL;
  }

  return NULL;
}

static int s1ap_eNB_generate_s1_setup_request(
  s1ap_eNB_instance_t *instance_p, s1ap_eNB_mme_data_t *s1ap_mme_data_p)
{
  s1ap_message               message;

  S1ap_S1SetupRequestIEs_t *s1SetupRequest_p;
  S1ap_PLMNidentity_t       plmnIdentity;
  S1ap_SupportedTAs_Item_t  ta;

  uint8_t  *buffer;
  uint32_t  len;
  int       ret = 0;

  DevAssert(instance_p != NULL);
  DevAssert(s1ap_mme_data_p != NULL);

  memset(&message, 0, sizeof(s1ap_message));

  message.direction     = S1AP_PDU_PR_initiatingMessage;
  message.procedureCode = S1ap_ProcedureCode_id_S1Setup;
  message.criticality   = S1ap_Criticality_reject;

  s1SetupRequest_p = &message.msg.s1ap_S1SetupRequestIEs;
  memset((void *)&plmnIdentity, 0, sizeof(S1ap_PLMNidentity_t));

  memset((void *)&ta, 0, sizeof(S1ap_SupportedTAs_Item_t));

  s1ap_mme_data_p->state = S1AP_ENB_STATE_WAITING;

  s1SetupRequest_p->global_ENB_ID.eNB_ID.present = S1ap_ENB_ID_PR_macroENB_ID;
  MACRO_ENB_ID_TO_BIT_STRING(instance_p->eNB_id,
                             &s1SetupRequest_p->global_ENB_ID.eNB_ID.choice.macroENB_ID);
  MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                    &s1SetupRequest_p->global_ENB_ID.pLMNidentity);

  S1AP_INFO("%d -> %02x%02x%02x\n", instance_p->eNB_id, s1SetupRequest_p->global_ENB_ID.eNB_ID.choice.macroENB_ID.buf[0], s1SetupRequest_p->global_ENB_ID.eNB_ID.choice.macroENB_ID.buf[1],
            s1SetupRequest_p->global_ENB_ID.eNB_ID.choice.macroENB_ID.buf[2]);

  INT16_TO_OCTET_STRING(instance_p->tac, &ta.tAC);
  MCC_MNC_TO_TBCD(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length, &plmnIdentity);

  ASN_SEQUENCE_ADD(&ta.broadcastPLMNs.list, &plmnIdentity);
  ASN_SEQUENCE_ADD(&s1SetupRequest_p->supportedTAs.list, &ta);

  s1SetupRequest_p->defaultPagingDRX = instance_p->default_drx;

  if (instance_p->eNB_name != NULL) {
    s1SetupRequest_p->presenceMask |= S1AP_S1SETUPREQUESTIES_ENBNAME_PRESENT;
    OCTET_STRING_fromBuf(&s1SetupRequest_p->eNBname, instance_p->eNB_name,
                         strlen(instance_p->eNB_name));
  }

  if (s1ap_eNB_encode_pdu(&message, &buffer, &len) < 0) {
    S1AP_ERROR("Failed to encode S1 setup request\n");
    return -1;
  }

  /* Non UE-Associated signalling -> stream = 0 */
  s1ap_eNB_itti_send_sctp_data_req(instance_p->instance, s1ap_mme_data_p->assoc_id, buffer, len, 0);

  return ret;
}
