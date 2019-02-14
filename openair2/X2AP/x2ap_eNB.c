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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "intertask_interface.h"

#include "x2ap_eNB.h"
#include "x2ap_eNB_defs.h"
#include "x2ap_eNB_management_procedures.h"
#include "x2ap_eNB_handler.h"
#include "x2ap_eNB_generate_messages.h"
#include "x2ap_common.h"

#include "queue.h"
#include "assertions.h"
#include "conversions.h"

struct x2ap_enb_map;
struct x2ap_eNB_data_s;

RB_PROTOTYPE(x2ap_enb_map, x2ap_eNB_data_s, entry, x2ap_eNB_compare_assoc_id);

static
void x2ap_eNB_handle_sctp_data_ind(instance_t instance, sctp_data_ind_t *sctp_data_ind);

static
void x2ap_eNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp);

static
void x2ap_eNB_handle_sctp_association_ind(instance_t instance, sctp_new_association_ind_t *sctp_new_association_ind);

static
void x2ap_eNB_handle_register_eNB(instance_t instance,
                                  x2ap_register_enb_req_t *x2ap_register_eNB);
static
void x2ap_eNB_register_eNB(x2ap_eNB_instance_t *instance_p,
                           net_ip_address_t    *target_eNB_ip_addr,
                           net_ip_address_t    *local_ip_addr,
                           uint16_t             in_streams,
                           uint16_t             out_streams,
                           uint32_t             enb_port_for_X2C,
                           int                  multi_sd);


static
void x2ap_eNB_handle_sctp_data_ind(instance_t instance, sctp_data_ind_t *sctp_data_ind) {
  int result;
  DevAssert(sctp_data_ind != NULL);
  x2ap_eNB_handle_message(instance, sctp_data_ind->assoc_id, sctp_data_ind->stream,
                          sctp_data_ind->buffer, sctp_data_ind->buffer_length);
  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
}

static
void x2ap_eNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp) {
  x2ap_eNB_instance_t *instance_p;
  x2ap_eNB_data_t *x2ap_enb_data_p;
  DevAssert(sctp_new_association_resp != NULL);
  printf("x2ap_eNB_handle_sctp_association_resp at 1\n");
  dump_trees();
  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  /* if the assoc_id is already known, it is certainly because an IND was received
   * before. In this case, just update streams and return
   */
  if (sctp_new_association_resp->assoc_id != -1) {
    x2ap_enb_data_p = x2ap_get_eNB(instance_p, sctp_new_association_resp->assoc_id,
                                   sctp_new_association_resp->ulp_cnx_id);

    if (x2ap_enb_data_p != NULL) {
      /* some sanity check - to be refined at some point */
      if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
        X2AP_ERROR("x2ap_enb_data_p not NULL and sctp state not SCTP_STATE_ESTABLISHED, what to do?\n");
        abort();
      }

      x2ap_enb_data_p->in_streams  = sctp_new_association_resp->in_streams;
      x2ap_enb_data_p->out_streams = sctp_new_association_resp->out_streams;
      return;
    }
  }

  x2ap_enb_data_p = x2ap_get_eNB(instance_p, -1,
                                 sctp_new_association_resp->ulp_cnx_id);
  DevAssert(x2ap_enb_data_p != NULL);
  printf("x2ap_eNB_handle_sctp_association_resp at 2\n");
  dump_trees();

  if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
    X2AP_WARN("Received unsuccessful result for SCTP association (%u), instance %d, cnx_id %u\n",
              sctp_new_association_resp->sctp_state,
              instance,
              sctp_new_association_resp->ulp_cnx_id);
    x2ap_handle_x2_setup_message(x2ap_enb_data_p,
                                 sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);
    return;
  }

  printf("x2ap_eNB_handle_sctp_association_resp at 3\n");
  dump_trees();
  /* Update parameters */
  x2ap_enb_data_p->assoc_id    = sctp_new_association_resp->assoc_id;
  x2ap_enb_data_p->in_streams  = sctp_new_association_resp->in_streams;
  x2ap_enb_data_p->out_streams = sctp_new_association_resp->out_streams;
  printf("x2ap_eNB_handle_sctp_association_resp at 4\n");
  dump_trees();
  /* Prepare new x2 Setup Request */
  x2ap_eNB_generate_x2_setup_request(instance_p, x2ap_enb_data_p);
}

static
void x2ap_eNB_handle_sctp_association_ind(instance_t instance, sctp_new_association_ind_t *sctp_new_association_ind) {
  x2ap_eNB_instance_t *instance_p;
  x2ap_eNB_data_t *x2ap_enb_data_p;
  printf("x2ap_eNB_handle_sctp_association_ind at 1 (called for instance %d)\n", instance);
  dump_trees();
  DevAssert(sctp_new_association_ind != NULL);
  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);
  x2ap_enb_data_p = x2ap_get_eNB(instance_p, sctp_new_association_ind->assoc_id, -1);

  if (x2ap_enb_data_p != NULL) abort();

  //  DevAssert(x2ap_enb_data_p != NULL);
  if (x2ap_enb_data_p == NULL) {
    /* Create new eNB descriptor */
    x2ap_enb_data_p = calloc(1, sizeof(*x2ap_enb_data_p));
    DevAssert(x2ap_enb_data_p != NULL);
    x2ap_enb_data_p->cnx_id                = x2ap_eNB_fetch_add_global_cnx_id();
    x2ap_enb_data_p->x2ap_eNB_instance = instance_p;
    /* Insert the new descriptor in list of known eNB
     * but not yet associated.
     */
    RB_INSERT(x2ap_enb_map, &instance_p->x2ap_enb_head, x2ap_enb_data_p);
    x2ap_enb_data_p->state = X2AP_ENB_STATE_CONNECTED;
    instance_p->x2_target_enb_nb++;

    if (instance_p->x2_target_enb_pending_nb > 0) {
      instance_p->x2_target_enb_pending_nb--;
    }
  } else {
    X2AP_WARN("x2ap_enb_data_p already exists\n");
  }

  printf("x2ap_eNB_handle_sctp_association_ind at 2\n");
  dump_trees();
  /* Update parameters */
  x2ap_enb_data_p->assoc_id    = sctp_new_association_ind->assoc_id;
  x2ap_enb_data_p->in_streams  = sctp_new_association_ind->in_streams;
  x2ap_enb_data_p->out_streams = sctp_new_association_ind->out_streams;
  printf("x2ap_eNB_handle_sctp_association_ind at 3\n");
  dump_trees();
}

int x2ap_eNB_init_sctp (x2ap_eNB_instance_t *instance_p,
                        net_ip_address_t    *local_ip_addr,
                        uint32_t enb_port_for_X2C) {
  // Create and alloc new message
  MessageDef                             *message;
  sctp_init_t                            *sctp_init  = NULL;
  DevAssert(instance_p != NULL);
  DevAssert(local_ip_addr != NULL);
  message = itti_alloc_new_message (TASK_X2AP, SCTP_INIT_MSG_MULTI_REQ);
  sctp_init = &message->ittiMsg.sctp_init_multi;
  sctp_init->port = enb_port_for_X2C;
  sctp_init->ppid = X2AP_SCTP_PPID;
  sctp_init->ipv4 = 1;
  sctp_init->ipv6 = 0;
  sctp_init->nb_ipv4_addr = 1;
#if 0
  memcpy(&sctp_init->ipv4_address,
         local_ip_addr,
         sizeof(*local_ip_addr));
#endif
  sctp_init->ipv4_address[0] = inet_addr(local_ip_addr->ipv4_address);
  /*
   * SR WARNING: ipv6 multi-homing fails sometimes for localhost.
   * * * * Disable it for now.
   */
  sctp_init->nb_ipv6_addr = 0;
  sctp_init->ipv6_address[0] = "0:0:0:0:0:0:0:1";
  return itti_send_msg_to_task (TASK_SCTP, instance_p->instance, message);
}

static void x2ap_eNB_register_eNB(x2ap_eNB_instance_t *instance_p,
                                  net_ip_address_t    *target_eNB_ip_address,
                                  net_ip_address_t    *local_ip_addr,
                                  uint16_t             in_streams,
                                  uint16_t             out_streams,
                                  uint32_t         enb_port_for_X2C,
                                  int                  multi_sd) {
  MessageDef                       *message                   = NULL;
  sctp_new_association_req_multi_t *sctp_new_association_req  = NULL;
  x2ap_eNB_data_t                  *x2ap_enb_data             = NULL;
  DevAssert(instance_p != NULL);
  DevAssert(target_eNB_ip_address != NULL);
  message = itti_alloc_new_message(TASK_X2AP, SCTP_NEW_ASSOCIATION_REQ_MULTI);
  sctp_new_association_req = &message->ittiMsg.sctp_new_association_req_multi;
  sctp_new_association_req->port = enb_port_for_X2C;
  sctp_new_association_req->ppid = X2AP_SCTP_PPID;
  sctp_new_association_req->in_streams  = in_streams;
  sctp_new_association_req->out_streams = out_streams;
  sctp_new_association_req->multi_sd = multi_sd;
  memcpy(&sctp_new_association_req->remote_address,
         target_eNB_ip_address,
         sizeof(*target_eNB_ip_address));
  memcpy(&sctp_new_association_req->local_address,
         local_ip_addr,
         sizeof(*local_ip_addr));
  /* Create new eNB descriptor */
  x2ap_enb_data = calloc(1, sizeof(*x2ap_enb_data));
  DevAssert(x2ap_enb_data != NULL);
  x2ap_enb_data->cnx_id                = x2ap_eNB_fetch_add_global_cnx_id();
  sctp_new_association_req->ulp_cnx_id = x2ap_enb_data->cnx_id;
  x2ap_enb_data->assoc_id          = -1;
  x2ap_enb_data->x2ap_eNB_instance = instance_p;
  /* Insert the new descriptor in list of known eNB
   * but not yet associated.
   */
  RB_INSERT(x2ap_enb_map, &instance_p->x2ap_enb_head, x2ap_enb_data);
  x2ap_enb_data->state = X2AP_ENB_STATE_WAITING;
  instance_p->x2_target_enb_nb ++;
  instance_p->x2_target_enb_pending_nb ++;
  itti_send_msg_to_task(TASK_SCTP, instance_p->instance, message);
}

static
void x2ap_eNB_handle_register_eNB(instance_t instance,
                                  x2ap_register_enb_req_t *x2ap_register_eNB) {
  x2ap_eNB_instance_t *new_instance;
  DevAssert(x2ap_register_eNB != NULL);
  /* Look if the provided instance already exists */
  new_instance = x2ap_eNB_get_instance(instance);

  if (new_instance != NULL) {
    /* Checks if it is a retry on the same eNB */
    DevCheck(new_instance->eNB_id == x2ap_register_eNB->eNB_id, new_instance->eNB_id, x2ap_register_eNB->eNB_id, 0);
    DevCheck(new_instance->cell_type == x2ap_register_eNB->cell_type, new_instance->cell_type, x2ap_register_eNB->cell_type, 0);
    DevCheck(new_instance->tac == x2ap_register_eNB->tac, new_instance->tac, x2ap_register_eNB->tac, 0);
    DevCheck(new_instance->mcc == x2ap_register_eNB->mcc, new_instance->mcc, x2ap_register_eNB->mcc, 0);
    DevCheck(new_instance->mnc == x2ap_register_eNB->mnc, new_instance->mnc, x2ap_register_eNB->mnc, 0);
    X2AP_WARN("eNB[%d] already registered\n", instance);
  } else {
    new_instance = calloc(1, sizeof(x2ap_eNB_instance_t));
    DevAssert(new_instance != NULL);
    RB_INIT(&new_instance->x2ap_enb_head);
    /* Copy usefull parameters */
    new_instance->instance         = instance;
    new_instance->eNB_name         = x2ap_register_eNB->eNB_name;
    new_instance->eNB_id           = x2ap_register_eNB->eNB_id;
    new_instance->cell_type        = x2ap_register_eNB->cell_type;
    new_instance->tac              = x2ap_register_eNB->tac;
    new_instance->mcc              = x2ap_register_eNB->mcc;
    new_instance->mnc              = x2ap_register_eNB->mnc;
    new_instance->mnc_digit_length = x2ap_register_eNB->mnc_digit_length;
    new_instance->num_cc           = x2ap_register_eNB->num_cc;

    for (int i = 0; i< x2ap_register_eNB->num_cc; i++) {
      new_instance->eutra_band[i]              = x2ap_register_eNB->eutra_band[i];
      new_instance->downlink_frequency[i]      = x2ap_register_eNB->downlink_frequency[i];
      new_instance->uplink_frequency_offset[i] = x2ap_register_eNB->uplink_frequency_offset[i];
      new_instance->Nid_cell[i]                = x2ap_register_eNB->Nid_cell[i];
      new_instance->N_RB_DL[i]                 = x2ap_register_eNB->N_RB_DL[i];
      new_instance->frame_type[i]              = x2ap_register_eNB->frame_type[i];
      new_instance->fdd_earfcn_DL[i]           = x2ap_register_eNB->fdd_earfcn_DL[i];
      new_instance->fdd_earfcn_UL[i]           = x2ap_register_eNB->fdd_earfcn_UL[i];
    }

    DevCheck(x2ap_register_eNB->nb_x2 <= X2AP_MAX_NB_ENB_IP_ADDRESS,
             X2AP_MAX_NB_ENB_IP_ADDRESS, x2ap_register_eNB->nb_x2, 0);
    memcpy(new_instance->target_enb_x2_ip_address,
           x2ap_register_eNB->target_enb_x2_ip_address,
           x2ap_register_eNB->nb_x2 * sizeof(net_ip_address_t));
    new_instance->nb_x2             = x2ap_register_eNB->nb_x2;
    new_instance->enb_x2_ip_address = x2ap_register_eNB->enb_x2_ip_address;
    new_instance->sctp_in_streams   = x2ap_register_eNB->sctp_in_streams;
    new_instance->sctp_out_streams  = x2ap_register_eNB->sctp_out_streams;
    new_instance->enb_port_for_X2C  = x2ap_register_eNB->enb_port_for_X2C;
    /* Add the new instance to the list of eNB (meaningfull in virtual mode) */
    x2ap_eNB_insert_new_instance(new_instance);
    X2AP_INFO("Registered new eNB[%d] and %s eNB id %u\n",
              instance,
              x2ap_register_eNB->cell_type == CELL_MACRO_ENB ? "macro" : "home",
              x2ap_register_eNB->eNB_id);

    /* initiate the SCTP listener */
    if (x2ap_eNB_init_sctp(new_instance,&x2ap_register_eNB->enb_x2_ip_address,x2ap_register_eNB->enb_port_for_X2C) <  0 ) {
      X2AP_ERROR ("Error while sending SCTP_INIT_MSG to SCTP \n");
      return;
    }

    X2AP_INFO("eNB[%d] eNB id %u acting as a listner (server)\n",
              instance, x2ap_register_eNB->eNB_id);
  }
}

static
void x2ap_eNB_handle_sctp_init_msg_multi_cnf(
  instance_t instance_id,
  sctp_init_msg_multi_cnf_t *m) {
  x2ap_eNB_instance_t *instance;
  int index;
  DevAssert(m != NULL);
  instance = x2ap_eNB_get_instance(instance_id);
  DevAssert(instance != NULL);
  instance->multi_sd = m->multi_sd;

  /* Exit if CNF message reports failure.
   * Failure means multi_sd < 0.
   */
  if (instance->multi_sd < 0) {
    X2AP_ERROR("Error: be sure to properly configure X2 in your configuration file.\n");
    DevAssert(instance->multi_sd >= 0);
  }

  /* Trying to connect to the provided list of eNB ip address */

  for (index = 0; index < instance->nb_x2; index++) {
    X2AP_INFO("eNB[%d] eNB id %u acting as an initiator (client)\n",
              instance_id, instance->eNB_id);
    x2ap_eNB_register_eNB(instance,
                          &instance->target_enb_x2_ip_address[index],
                          &instance->enb_x2_ip_address,
                          instance->sctp_in_streams,
                          instance->sctp_out_streams,
                          instance->enb_port_for_X2C,
                          instance->multi_sd);
  }
}

void *x2ap_task(void *arg) {
  MessageDef *received_msg = NULL;
  int         result;
  X2AP_DEBUG("Starting X2AP layer\n");
  x2ap_eNB_prepare_internal_data();
  itti_mark_task_ready(TASK_X2AP);

  while (1) {
    itti_receive_msg(TASK_X2AP, &received_msg);

    switch (ITTI_MSG_ID(received_msg)) {
      case TERMINATE_MESSAGE:
        X2AP_WARN(" *** Exiting X2AP thread\n");
        itti_exit_task();
        break;

      case X2AP_REGISTER_ENB_REQ:
        x2ap_eNB_handle_register_eNB(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                     &X2AP_REGISTER_ENB_REQ(received_msg));
        break;

      case SCTP_INIT_MSG_MULTI_CNF:
        x2ap_eNB_handle_sctp_init_msg_multi_cnf(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                                &received_msg->ittiMsg.sctp_init_msg_multi_cnf);
        break;

      case SCTP_NEW_ASSOCIATION_RESP:
        x2ap_eNB_handle_sctp_association_resp(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                              &received_msg->ittiMsg.sctp_new_association_resp);
        break;

      case SCTP_NEW_ASSOCIATION_IND:
        x2ap_eNB_handle_sctp_association_ind(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                             &received_msg->ittiMsg.sctp_new_association_ind);
        break;

      case SCTP_DATA_IND:
        x2ap_eNB_handle_sctp_data_ind(ITTI_MESSAGE_GET_INSTANCE(received_msg),
                                      &received_msg->ittiMsg.sctp_data_ind);
        break;

      default:
        X2AP_ERROR("Received unhandled message: %d:%s\n",
                   ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
        break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    received_msg = NULL;
  }

  return NULL;
}


