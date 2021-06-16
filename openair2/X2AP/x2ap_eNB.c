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

/*! \file x2ap_eNB.c
 * \brief x2ap tasks for eNB
 * \author Konstantinos Alexandris <Konstantinos.Alexandris@eurecom.fr>, Cedric Roux <Cedric.Roux@eurecom.fr>, Navid Nikaein <Navid.Nikaein@eurecom.fr>
 * \date 2018
 * \version 1.0
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
#include "x2ap_ids.h"
#include "x2ap_timers.h"

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
void x2ap_eNB_handle_handover_req(instance_t instance,
                                  x2ap_handover_req_t *x2ap_handover_req);

static
void x2ap_eNB_handle_handover_req_ack(instance_t instance,
                                      x2ap_handover_req_ack_t *x2ap_handover_req_ack);

static
void x2ap_eNB_ue_context_release(instance_t instance,
                                 x2ap_ue_context_release_t *x2ap_ue_context_release);


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
        if (sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN){
          RB_REMOVE(x2ap_enb_map, &instance_p->x2ap_enb_head, x2ap_enb_data_p);
          return;
        }

        exit(1);
      }

      x2ap_enb_data_p->in_streams  = sctp_new_association_resp->in_streams;
      x2ap_enb_data_p->out_streams = sctp_new_association_resp->out_streams;
      return;
    }
  }

  x2ap_enb_data_p = x2ap_get_eNB(instance_p, -1,
                                 sctp_new_association_resp->ulp_cnx_id);
  DevAssert(x2ap_enb_data_p != NULL);
  dump_trees();

  /* gNB: exit if connection to eNB failed - to be modified if needed.
   * We may want to try to connect over and over again until we succeed
   * but the modifications to the code to get this behavior are complex.
   * Exit on error is a simple solution that can be caught by a script
   * for example.
   */
  if (instance_p->cell_type == CELL_MACRO_GNB
      && sctp_new_association_resp->sctp_state == SCTP_STATE_UNREACHABLE) {
    X2AP_ERROR("association with eNB failed, is it running? If no, run it first. If yes, check IP addresses in your configuration file.\n");
    exit(1);
  }

  if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
    X2AP_WARN("Received unsuccessful result for SCTP association (%u), instance %ld, cnx_id %u\n",
              sctp_new_association_resp->sctp_state,
              instance,
              sctp_new_association_resp->ulp_cnx_id);
    x2ap_handle_x2_setup_message(instance_p, x2ap_enb_data_p,
                                 sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);
    return;
  }

  dump_trees();
  /* Update parameters */
  x2ap_enb_data_p->assoc_id    = sctp_new_association_resp->assoc_id;
  x2ap_enb_data_p->in_streams  = sctp_new_association_resp->in_streams;
  x2ap_enb_data_p->out_streams = sctp_new_association_resp->out_streams;
  dump_trees();
  /* Prepare new x2 Setup Request */
  if(instance_p->cell_type == CELL_MACRO_GNB)
	  x2ap_gNB_generate_ENDC_x2_setup_request(instance_p, x2ap_enb_data_p);
  else
	  x2ap_eNB_generate_x2_setup_request(instance_p, x2ap_enb_data_p);
}

static
void x2ap_eNB_handle_sctp_association_ind(instance_t instance, sctp_new_association_ind_t *sctp_new_association_ind) {
  x2ap_eNB_instance_t *instance_p;
  x2ap_eNB_data_t *x2ap_enb_data_p;
  printf("x2ap_eNB_handle_sctp_association_ind at 1 (called for instance %ld)\n", instance);
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
  message = itti_alloc_new_message (TASK_X2AP, 0, SCTP_INIT_MSG_MULTI_REQ);
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
  message = itti_alloc_new_message(TASK_X2AP, 0, SCTP_NEW_ASSOCIATION_REQ_MULTI);
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
    X2AP_WARN("eNB[%ld] already registered\n", instance);
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

    x2ap_id_manager_init(&new_instance->id_manager);
    x2ap_timers_init(&new_instance->timers,
                     x2ap_register_eNB->t_reloc_prep,
                     x2ap_register_eNB->tx2_reloc_overall,
                     x2ap_register_eNB->t_dc_prep,
                     x2ap_register_eNB->t_dc_overall);

    for (int i = 0; i< x2ap_register_eNB->num_cc; i++) {
      if(new_instance->cell_type == CELL_MACRO_GNB){
        new_instance->nr_band[i]              = x2ap_register_eNB->nr_band[i];
        new_instance->tdd_nRARFCN[i]             = x2ap_register_eNB->nrARFCN[i];
      }
      else{
        new_instance->eutra_band[i]              = x2ap_register_eNB->eutra_band[i];
        new_instance->downlink_frequency[i]      = x2ap_register_eNB->downlink_frequency[i];
        new_instance->fdd_earfcn_DL[i]           = x2ap_register_eNB->fdd_earfcn_DL[i];
        new_instance->fdd_earfcn_UL[i]           = x2ap_register_eNB->fdd_earfcn_UL[i];
      }

      new_instance->uplink_frequency_offset[i] = x2ap_register_eNB->uplink_frequency_offset[i];
      new_instance->Nid_cell[i]                = x2ap_register_eNB->Nid_cell[i];
      new_instance->N_RB_DL[i]                 = x2ap_register_eNB->N_RB_DL[i];
      new_instance->frame_type[i]              = x2ap_register_eNB->frame_type[i];
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
    X2AP_INFO("Registered new eNB[%ld] and %s eNB id %u\n",
              instance,
              x2ap_register_eNB->cell_type == CELL_MACRO_ENB ? "macro" : "home",
              x2ap_register_eNB->eNB_id);

    /* initiate the SCTP listener */
    if (x2ap_eNB_init_sctp(new_instance,&x2ap_register_eNB->enb_x2_ip_address,x2ap_register_eNB->enb_port_for_X2C) <  0 ) {
      X2AP_ERROR ("Error while sending SCTP_INIT_MSG to SCTP \n");
      return;
    }

    X2AP_INFO("eNB[%ld] eNB id %u acting as a listner (server)\n",
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
    X2AP_INFO("eNB[%ld] eNB id %u acting as an initiator (client)\n",
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

static
void x2ap_eNB_handle_handover_req(instance_t instance,
                                  x2ap_handover_req_t *x2ap_handover_req)
{
  x2ap_eNB_instance_t *instance_p;
  x2ap_eNB_data_t     *target;
  x2ap_id_manager     *id_manager;
  int                 ue_id;

  int target_pci = x2ap_handover_req->target_physCellId;

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  target = x2ap_is_eNB_pci_in_list(target_pci);
  DevAssert(target != NULL);

  /* allocate x2ap ID */
  id_manager = &instance_p->id_manager;
  ue_id = x2ap_allocate_new_id(id_manager);
  if (ue_id == -1) {
    X2AP_ERROR("could not allocate a new X2AP UE ID\n");
    /* TODO: cancel handover: send (to be defined) message to RRC */
    exit(1);
  }
  /* id_source is ue_id, id_target is unknown yet */
  x2ap_set_ids(id_manager, ue_id, x2ap_handover_req->rnti, ue_id, -1);
  x2ap_id_set_state(id_manager, ue_id, X2ID_STATE_SOURCE_PREPARE);
  x2ap_set_reloc_prep_timer(id_manager, ue_id,
                            x2ap_timer_get_tti(&instance_p->timers));
  x2ap_id_set_target(id_manager, ue_id, target);

  x2ap_eNB_generate_x2_handover_request(instance_p, target, x2ap_handover_req, ue_id);
}

static
void x2ap_eNB_handle_handover_req_ack(instance_t instance,
                                      x2ap_handover_req_ack_t *x2ap_handover_req_ack)
{
  /* TODO: remove this hack (the goal is to find the correct
   * eNodeB structure for the other end) - we need a proper way for RRC
   * and X2AP to identify eNodeBs
   * RRC knows about mod_id and X2AP knows about eNB_id (eNB_ID in
   * the configuration file)
   * as far as I understand.. CROUX
   */
  x2ap_eNB_instance_t *instance_p;
  x2ap_eNB_data_t     *target;
  int source_assoc_id = x2ap_handover_req_ack->source_assoc_id;
  int                 ue_id;
  int                 id_source;
  int                 id_target;

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  target = x2ap_get_eNB(NULL, source_assoc_id, 0);
  DevAssert(target != NULL);

  /* rnti is a new information, save it */
  ue_id     = x2ap_handover_req_ack->x2_id_target;
  id_source = x2ap_id_get_id_source(&instance_p->id_manager, ue_id);
  id_target = ue_id;
  x2ap_set_ids(&instance_p->id_manager, ue_id, x2ap_handover_req_ack->rnti, id_source, id_target);

  x2ap_eNB_generate_x2_handover_request_ack(instance_p, target, x2ap_handover_req_ack);
}

static
void x2ap_eNB_handle_sgNB_add_req(instance_t instance,
                                  x2ap_ENDC_sgnb_addition_req_t *x2ap_ENDC_sgnb_addition_req)
{
  x2ap_id_manager     *id_manager;
  x2ap_eNB_instance_t *instance_p;
  x2ap_eNB_data_t     *x2ap_eNB_data;
  int                 ue_id;
  LTE_PhysCellId_t target_pci;
  target_pci = x2ap_ENDC_sgnb_addition_req->target_physCellId;
  x2ap_eNB_data = x2ap_is_eNB_pci_in_list(target_pci);
  DevAssert(x2ap_eNB_data != NULL);
  DevAssert(x2ap_ENDC_sgnb_addition_req != NULL);


  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  /* allocate x2ap ID */
  id_manager = &instance_p->id_manager;
  ue_id = x2ap_allocate_new_id(id_manager);
  if (ue_id == -1) {
    X2AP_ERROR("could not allocate a new X2AP UE ID\n");
    /* TODO: cancel NSA: send (to be defined) message to RRC */
    exit(1);
  }
  /* id_source is ue_id, id_target is unknown yet */
  x2ap_set_ids(id_manager, ue_id, x2ap_ENDC_sgnb_addition_req->rnti, ue_id, -1);
  x2ap_id_set_state(id_manager, ue_id, X2ID_STATE_NSA_ENB_PREPARE);
  x2ap_set_dc_prep_timer(id_manager, ue_id,
                         x2ap_timer_get_tti(&instance_p->timers));
  x2ap_id_set_target(id_manager, ue_id, x2ap_eNB_data);

  x2ap_eNB_generate_ENDC_x2_SgNB_addition_request(instance_p, x2ap_ENDC_sgnb_addition_req,
      x2ap_eNB_data, ue_id);
}

static
void x2ap_gNB_trigger_sgNB_add_req_ack(instance_t instance,
		x2ap_ENDC_sgnb_addition_req_ACK_t *x2ap_ENDC_sgnb_addition_req_ACK)
{
  /* TODO: remove this hack (the goal is to find the correct
   * eNodeB structure for the other end) - we need a proper way for RRC
   * and X2AP to identify eNodeBs
   * RRC knows about mod_id and X2AP knows about eNB_id (eNB_ID in
   * the configuration file)
   * as far as I understand.. CROUX
   */

  x2ap_id_manager     *id_manager;
  x2ap_eNB_instance_t *instance_p;
  x2ap_eNB_data_t     *target;
  int                 ue_id;

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);
  target = x2ap_get_eNB(NULL,x2ap_ENDC_sgnb_addition_req_ACK->target_assoc_id, 0);
  DevAssert(target != NULL);
	
  /* allocate x2ap ID */
  id_manager = &instance_p->id_manager;
  ue_id = x2ap_allocate_new_id(id_manager);
  if (ue_id == -1) {
    X2AP_ERROR("could not allocate a new X2AP UE ID\n");
    exit(1);
  }
  /* id_Source is MeNB_ue_x2_id, id_target is rnti (rnti is SgNB_ue_x2_id) */
  x2ap_set_ids(id_manager, ue_id,
      x2ap_ENDC_sgnb_addition_req_ACK->SgNB_ue_x2_id,
      x2ap_ENDC_sgnb_addition_req_ACK->MeNB_ue_x2_id,
      x2ap_ENDC_sgnb_addition_req_ACK->SgNB_ue_x2_id);
  x2ap_id_set_state(id_manager, ue_id, X2ID_STATE_NSA_GNB_OVERALL);
  x2ap_set_dc_overall_timer(id_manager, ue_id,
                            x2ap_timer_get_tti(&instance_p->timers));
  x2ap_id_set_target(id_manager, ue_id, target);

  x2ap_gNB_generate_ENDC_x2_SgNB_addition_request_ACK(instance_p, target,
      x2ap_ENDC_sgnb_addition_req_ACK, ue_id);
}

/**
 * @fn	: Function triggers sgnb reconfiguration complete
 * @param	: IN instance, IN x2ap_reconf_complete
**/ 
static
void x2ap_eNB_trigger_sgnb_reconfiguration_complete(instance_t instance,
    x2ap_ENDC_reconf_complete_t *x2ap_reconf_complete)
{
  x2ap_eNB_instance_t *instance_p;
  x2ap_eNB_data_t     *target;
  int                 id_source;
  int                 id_target;

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);
  DevAssert(x2ap_reconf_complete != NULL);

  target = x2ap_get_eNB(NULL,x2ap_reconf_complete->gnb_x2_assoc_id, 0);
  DevAssert(target != NULL);

  id_source = x2ap_reconf_complete->MeNB_ue_x2_id;
  id_target = x2ap_reconf_complete->SgNB_ue_x2_id;
  x2ap_eNB_generate_ENDC_x2_SgNB_reconfiguration_complete(instance_p, target, id_source, id_target);
}


static
void x2ap_eNB_ue_context_release(instance_t instance,
                                 x2ap_ue_context_release_t *x2ap_ue_context_release)
{
  x2ap_eNB_instance_t *instance_p;
  x2ap_eNB_data_t     *target;
  int source_assoc_id = x2ap_ue_context_release->source_assoc_id;
  int ue_id;
  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  target = x2ap_get_eNB(NULL, source_assoc_id, 0);
  DevAssert(target != NULL);

  x2ap_eNB_generate_x2_ue_context_release(instance_p, target, x2ap_ue_context_release);

  /* free the X2AP UE ID */
  ue_id = x2ap_find_id_from_rnti(&instance_p->id_manager, x2ap_ue_context_release->rnti);
  if (ue_id == -1) {
    X2AP_ERROR("could not find UE %x\n", x2ap_ue_context_release->rnti);
    exit(1);
  }
  x2ap_release_id(&instance_p->id_manager, ue_id);
}

static
void x2ap_eNB_handle_sgNB_release_request(instance_t instance,
    x2ap_ENDC_sgnb_release_request_t *x2ap_release_req)
{
  x2ap_eNB_instance_t *instance_p;
  x2ap_eNB_data_t     *target;

  instance_p = x2ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);
  DevAssert(x2ap_release_req != NULL);

  if (x2ap_release_req->rnti == -1 ||
      x2ap_release_req->assoc_id == -1) {
    X2AP_WARN("x2ap_eNB_handle_sgNB_release_request: bad rnti or assoc_id, do not send release request to gNB\n");
    return;
  }

  target = x2ap_get_eNB(NULL, x2ap_release_req->assoc_id, 0);
  DevAssert(target != NULL);

  /* id_source is not used by oai's gNB so it's not big deal. For
   * interoperability with other gNBs things may need to be refined.
   */
  x2ap_eNB_generate_ENDC_x2_SgNB_release_request(instance_p, target,
                                                 0, x2ap_release_req->rnti,
                                                 x2ap_release_req->cause);
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

      case X2AP_SUBFRAME_PROCESS:
        x2ap_check_timers(ITTI_MSG_DESTINATION_INSTANCE(received_msg));
        break;

      case X2AP_REGISTER_ENB_REQ:
        x2ap_eNB_handle_register_eNB(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                     &X2AP_REGISTER_ENB_REQ(received_msg));
        break;

      case X2AP_HANDOVER_REQ:
        x2ap_eNB_handle_handover_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                     &X2AP_HANDOVER_REQ(received_msg));
        break;

      case X2AP_HANDOVER_REQ_ACK:
        x2ap_eNB_handle_handover_req_ack(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                         &X2AP_HANDOVER_REQ_ACK(received_msg));
        break;

      case X2AP_UE_CONTEXT_RELEASE:
        x2ap_eNB_ue_context_release(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                                &X2AP_UE_CONTEXT_RELEASE(received_msg));
        break;

      case X2AP_ENDC_SGNB_ADDITION_REQ:
        x2ap_eNB_handle_sgNB_add_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                     &X2AP_ENDC_SGNB_ADDITION_REQ(received_msg));
        break;

      case X2AP_ENDC_SGNB_ADDITION_REQ_ACK:
    	  x2ap_gNB_trigger_sgNB_add_req_ack(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
    			  &X2AP_ENDC_SGNB_ADDITION_REQ_ACK(received_msg));
    	LOG_I(X2AP, "Received elements for X2AP_ENDC_SGNB_ADDITION_REQ_ACK \n");
    	break;

      case X2AP_ENDC_SGNB_RECONF_COMPLETE:
        x2ap_eNB_trigger_sgnb_reconfiguration_complete(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                          &X2AP_ENDC_SGNB_RECONF_COMPLETE(received_msg));
        break;

      case X2AP_ENDC_SGNB_RELEASE_REQUEST:
        x2ap_eNB_handle_sgNB_release_request(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                          &X2AP_ENDC_SGNB_RELEASE_REQUEST(received_msg));
        break;

      case SCTP_INIT_MSG_MULTI_CNF:
        x2ap_eNB_handle_sctp_init_msg_multi_cnf(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                                &received_msg->ittiMsg.sctp_init_msg_multi_cnf);
        break;

      case SCTP_NEW_ASSOCIATION_RESP:
        x2ap_eNB_handle_sctp_association_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                              &received_msg->ittiMsg.sctp_new_association_resp);
        break;

      case SCTP_NEW_ASSOCIATION_IND:
        x2ap_eNB_handle_sctp_association_ind(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                             &received_msg->ittiMsg.sctp_new_association_ind);
        break;

      case SCTP_DATA_IND:
        x2ap_eNB_handle_sctp_data_ind(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
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

#include "common/config/config_userapi.h"

int is_x2ap_enabled(void)
{
  static volatile int config_loaded = 0;
  static volatile int enabled = 0;
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  if (pthread_mutex_lock(&mutex)) goto mutex_error;
  if (config_loaded) {
    if (pthread_mutex_unlock(&mutex)) goto mutex_error;
    return enabled;
  }

  char *enable_x2 = NULL;
  paramdef_t p[] = {
   { "enable_x2", "yes/no", 0, strptr:&enable_x2, defstrval:"", TYPE_STRING, 0 }
  };

  /* TODO: do it per module - we check only first eNB */
  config_get(p, sizeof(p)/sizeof(paramdef_t), "eNBs.[0]");
  if (enable_x2 != NULL && strcmp(enable_x2, "yes") == 0){
	  enabled = 1;
  }

  /*Consider also the case of enabling X2AP for a gNB by parsing a gNB configuration file*/

  config_get(p, sizeof(p)/sizeof(paramdef_t), "gNBs.[0]");
    if (enable_x2 != NULL && strcmp(enable_x2, "yes") == 0){
  	  enabled = 1;
    }

  config_loaded = 1;

  if (pthread_mutex_unlock(&mutex)) goto mutex_error;
  return enabled;

mutex_error:
  LOG_E(X2AP, "mutex error\n");
  exit(1);
}
