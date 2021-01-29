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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <syscall.h>


#include "tree.h"
#include "queue.h"
#include "intertask_interface.h"
#include "s1ap_eNB_default_values.h"
#include "s1ap_common.h"
#include "s1ap_eNB_defs.h"
#include "s1ap_eNB_management_procedures.h"
#include "assertions.h"

#include "platform_types.h"
#include "oaisim_mme_test_s1c.h"


void s1ap_eNB_handle_sctp_data_ind(sctp_data_ind_t *sctp_data_ind)
{
  int result;
  DevAssert(sctp_data_ind != NULL);
  mme_test_s1_notify_sctp_data_ind(sctp_data_ind->assoc_id, sctp_data_ind->stream,
                                   sctp_data_ind->buffer, sctp_data_ind->buffer_length);
  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
}


void s1ap_eNB_itti_send_sctp_data_req(instance_t instance, int32_t assoc_id, uint8_t *buffer,
                                      uint32_t buffer_length, uint16_t stream)
{
  MessageDef      *message_p;
  sctp_data_req_t *sctp_data_req;
  message_p = itti_alloc_new_message(TASK_S1AP, SCTP_DATA_REQ);
  sctp_data_req = &message_p->ittiMsg.sctp_data_req;
  sctp_data_req->assoc_id      = assoc_id;
  sctp_data_req->buffer        = buffer;
  sctp_data_req->buffer_length = buffer_length;
  sctp_data_req->stream        = stream;
  itti_send_msg_to_task(TASK_SCTP, instance, message_p);
}

void s1ap_handle_s1_setup_message(s1ap_eNB_mme_data_t *mme_desc_p, int sctp_shutdown)
{
  if (sctp_shutdown) {
    /* A previously connected MME has been shutdown */

    /* TODO check if it was used by some eNB and send a message to inform these eNB if there is no more associated MME */
    if (mme_desc_p->state == S1AP_ENB_STATE_CONNECTED) {
      mme_desc_p->state = S1AP_ENB_STATE_DISCONNECTED;

      if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb > 0) {
        /* Decrease associated MME number */
        mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb --;
      }

      /* If there are no more associated MME, inform eNB app */
      if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb == 0) {
        MessageDef                 *message_p;
        message_p = itti_alloc_new_message(TASK_S1AP, S1AP_DEREGISTERED_ENB_IND);
        S1AP_DEREGISTERED_ENB_IND(message_p).nb_mme = 0;
        itti_send_msg_to_task(TASK_ENB_APP, mme_desc_p->s1ap_eNB_instance->instance, message_p);
      }
    }
  } else {
    /* Check that at least one setup message is pending */
    DevCheck(mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb > 0, mme_desc_p->s1ap_eNB_instance->instance,
             mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb, 0);

    if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb > 0) {
      /* Decrease pending messages number */
      mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb --;
    }

    /* If there are no more pending messages, inform eNB app */
    if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb == 0) {
      MessageDef                 *message_p;
      message_p = itti_alloc_new_message(TASK_S1AP, S1AP_REGISTER_ENB_CNF);
      S1AP_REGISTER_ENB_CNF(message_p).nb_mme = mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb;
      itti_send_msg_to_task(TASK_ENB_APP, mme_desc_p->s1ap_eNB_instance->instance, message_p);
    }
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
  mme_test_s1_start_test(instance_p, s1ap_mme_data_p);
}

void s1ap_eNB_register_mme(s1ap_eNB_instance_t *instance_p,
                           net_ip_address_t    *mme_ip_address,
                           net_ip_address_t    *local_ip_addr)
{
  MessageDef                 *message_p                   = NULL;
  sctp_new_association_req_t *sctp_new_association_req_p  = NULL;
  s1ap_eNB_mme_data_t        *s1ap_mme_data_p             = NULL;
  DevAssert(instance_p != NULL);
  DevAssert(mme_ip_address != NULL);
  message_p = itti_alloc_new_message(TASK_S1AP, SCTP_NEW_ASSOCIATION_REQ);
  sctp_new_association_req_p = &message_p->ittiMsg.sctp_new_association_req;
  sctp_new_association_req_p->port = S1AP_PORT_NUMBER;
  sctp_new_association_req_p->ppid = S1AP_SCTP_PPID;
  memcpy(&sctp_new_association_req_p->remote_address,
         mme_ip_address,
         sizeof(*mme_ip_address));
  memcpy(&sctp_new_association_req_p->local_address,
         local_ip_addr,
         sizeof(*local_ip_addr));
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
    S1AP_DEBUG("Registered new eNB[%d] and %s eNB id %u\n",
               instance,
               s1ap_register_eNB->cell_type == CELL_MACRO_ENB ? "macro" : "home",
               s1ap_register_eNB->eNB_id);
  }

  DevCheck(s1ap_register_eNB->nb_mme <= S1AP_MAX_NB_MME_IP_ADDRESS,
           S1AP_MAX_NB_MME_IP_ADDRESS, s1ap_register_eNB->nb_mme, 0);

  /* Trying to connect to provided list of MME ip address */
  for (index = 0; index < s1ap_register_eNB->nb_mme; index++) {
    s1ap_eNB_register_mme(new_instance, &s1ap_register_eNB->mme_ip_address[index],
                          &s1ap_register_eNB->enb_ip_address);
  }
}




void *s1ap_eNB_task(void *arg)
{
  MessageDef *received_msg = NULL;
  int         result;
  S1AP_DEBUG("Starting S1AP layer\n");
  s1ap_eNB_prepare_internal_data();
  itti_mark_task_ready(TASK_S1AP);

  while (1) {
    itti_receive_msg(TASK_S1AP, &received_msg);

    switch (ITTI_MSG_ID(received_msg)) {
      case TERMINATE_MESSAGE:
        itti_exit_task();
        break;

      case S1AP_REGISTER_ENB_REQ: {
        /* Register a new eNB.
         * in Virtual mode eNBs will be distinguished using the mod_id/
         * Each eNB has to send an S1AP_REGISTER_ENB message with its
         * own parameters.
         */
        s1ap_eNB_handle_register_eNB(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                     &S1AP_REGISTER_ENB_REQ(received_msg));
      }
      break;

      case SCTP_NEW_ASSOCIATION_RESP: {
        s1ap_eNB_handle_sctp_association_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                              &received_msg->ittiMsg.sctp_new_association_resp);
      }
      break;

      case SCTP_DATA_IND: {
        s1ap_eNB_handle_sctp_data_ind(&received_msg->ittiMsg.sctp_data_ind);
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
