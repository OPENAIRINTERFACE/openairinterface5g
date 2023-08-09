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

/*! \file m2ap_MCE.c
 * \brief m2ap tasks for MCE
 * \author Javier Morgade  <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "intertask_interface.h"

#include "m2ap_MCE.h"
#include "m2ap_MCE_defs.h"
#include "m2ap_MCE_management_procedures.h"
#include "m2ap_MCE_handler.h"
#include "m2ap_MCE_generate_messages.h"
#include "m2ap_common.h"
#include "m2ap_MCE_interface_management.h"
#include "m2ap_ids.h"
#include "m2ap_timers.h"

#include "queue.h"
#include "assertions.h"
#include "conversions.h"

struct m2ap_mce_map;
struct m2ap_MCE_data_s;

m2ap_setup_req_t *m2ap_mce_data_from_enb;

RB_PROTOTYPE(m2ap_mce_map, m2ap_MCE_data_s, entry, m2ap_MCE_compare_assoc_id);

static
void m2ap_MCE_handle_sctp_data_ind(instance_t instance, sctp_data_ind_t *sctp_data_ind);

static
void m2ap_MCE_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp);

static
void m2ap_MCE_handle_sctp_association_ind(instance_t instance, sctp_new_association_ind_t *sctp_new_association_ind);

static
void m2ap_MCE_handle_register_MCE(instance_t instance,
                                  m2ap_register_mce_req_t *m2ap_register_MCE);
static
void m2ap_MCE_register_MCE(m2ap_MCE_instance_t *instance_p,
                           net_ip_address_t    *target_MCE_ip_addr,
                           net_ip_address_t    *local_ip_addr,
                           uint16_t             in_streams,
                           uint16_t             out_streams,
                           uint32_t             mce_port_for_M2C,
                           int                  multi_sd);

//static
//void m2ap_MCE_handle_handover_req(instance_t instance,
//                                  m2ap_handover_req_t *m2ap_handover_req);
//
//static
//void m2ap_MCE_handle_handover_req_ack(instance_t instance,
//                                      m2ap_handover_req_ack_t *m2ap_handover_req_ack);
//
//static
//void m2ap_MCE_ue_context_release(instance_t instance,
//                                 m2ap_ue_context_release_t *m2ap_ue_context_release);
//

static
void m2ap_MCE_handle_sctp_data_ind(instance_t instance, sctp_data_ind_t *sctp_data_ind) {
  int result;
  DevAssert(sctp_data_ind != NULL);
  m2ap_MCE_handle_message(instance, sctp_data_ind->assoc_id, sctp_data_ind->stream,
                          sctp_data_ind->buffer, sctp_data_ind->buffer_length);
  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
}

static
void m2ap_MCE_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp) {

   DevAssert(sctp_new_association_resp != NULL);

  if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
    LOG_W(M2AP, "Received unsuccessful result for SCTP association (%u), instance %ld, cnx_id %u\n",
              sctp_new_association_resp->sctp_state,
              instance,
              sctp_new_association_resp->ulp_cnx_id);

    if (sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN)
      return;
  }

   // go to an init func
  m2ap_mce_data_from_enb = (m2ap_setup_req_t *)calloc(1, sizeof(m2ap_setup_req_t));
  // save the assoc id
  m2ap_mce_data_from_enb->assoc_id         = sctp_new_association_resp->assoc_id;
  m2ap_mce_data_from_enb->sctp_in_streams  = sctp_new_association_resp->in_streams;
  m2ap_mce_data_from_enb->sctp_out_streams = sctp_new_association_resp->out_streams;



 // m2ap_MCE_instance_t *instance_p;
 // m2ap_MCE_data_t *m2ap_mce_data_p;
 // DevAssert(sctp_new_association_resp != NULL);
 // printf("m2ap_MCE_handle_sctp_association_resp at 1\n");
 // dump_mce_trees_m2();
 // instance_p = instance;//m2ap_MCE_get_instance(instance);
 // DevAssert(instance_p != NULL);

 // /* if the assoc_id is already known, it is certainly because an IND was received
 //  * before. In this case, just update streams and return
 //  */
 // if (sctp_new_association_resp->assoc_id != -1) {
 //   m2ap_mce_data_p = m2ap_get_MCE(instance_p, sctp_new_association_resp->assoc_id,
 //                                  sctp_new_association_resp->ulp_cnx_id);

 //   if (m2ap_mce_data_p != NULL) {
 //     /* some sanity check - to be refined at some point */
 //     if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
 //       M2AP_ERROR("m2ap_mce_data_p not NULL and sctp state not SCTP_STATE_ESTABLISHED, what to do?\n");
 //       abort();
 //     }

 //     m2ap_mce_data_p->in_streams  = sctp_new_association_resp->in_streams;
 //     m2ap_mce_data_p->out_streams = sctp_new_association_resp->out_streams;
 //     return;
 //   }
 // }

 // m2ap_mce_data_p = m2ap_get_MCE(instance_p, -1,
 //                                sctp_new_association_resp->ulp_cnx_id);
 // DevAssert(m2ap_mce_data_p != NULL);
 // printf("m2ap_MCE_handle_sctp_association_resp at 2\n");
 // dump_mce_trees_m2();

 // if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
 //   M2AP_WARN("Received unsuccessful result for SCTP association (%u), instance %d, cnx_id %u\n",
 //             sctp_new_association_resp->sctp_state,
 //             instance,
 //             sctp_new_association_resp->ulp_cnx_id);
 //   //m2ap_eNB_handle_m2_setup_message(instance_p, m2ap_mce_data_p,
 //                                //sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);
 //   return;
 // }

 // printf("m2ap_MCE_handle_sctp_association_resp at 3\n");
 // dump_mce_trees_m2();
 // /* Update parameters */
 // m2ap_mce_data_p->assoc_id    = sctp_new_association_resp->assoc_id;
 // m2ap_mce_data_p->in_streams  = sctp_new_association_resp->in_streams;
 // m2ap_mce_data_p->out_streams = sctp_new_association_resp->out_streams;
 // printf("m2ap_MCE_handle_sctp_association_resp at 4\n");
 // dump_mce_trees_m2();
 // /* Prepare new m2 Setup Request */
 // //m2ap_MCE_generate_m2_setup_request(instance_p, m2ap_mce_data_p);
}

static
void m2ap_MCE_handle_sctp_association_ind(instance_t instance, sctp_new_association_ind_t *sctp_new_association_ind) {
  //m2ap_MCE_instance_t *instance_p;
  //m2ap_MCE_data_t *m2ap_mce_data_p;
  //printf("m2ap_MCE_handle_sctp_association_ind at 1 (called for instance %d)\n", instance);
  ///dump_mce_trees_m2();
  ///DevAssert(sctp_new_association_ind != NULL);
  ///instance_p = instance;//m2ap_MCE_get_instance(instance);
  ///DevAssert(instance_p != NULL);
  ///m2ap_mce_data_p = m2ap_get_MCE(instance_p, sctp_new_association_ind->assoc_id, -1);

  ///if (m2ap_mce_data_p != NULL) abort();

  /////  DevAssert(m2ap_enb_data_p != NULL);
  ///if (m2ap_mce_data_p == NULL) {
  ///  /* Create new MCE descriptor */
  ///  m2ap_mce_data_p = calloc(1, sizeof(*m2ap_mce_data_p));
  ///  DevAssert(m2ap_mce_data_p != NULL);
  ///  m2ap_mce_data_p->cnx_id                = m2ap_MCE_fetch_add_global_cnx_id();
  ///  m2ap_mce_data_p->m2ap_MCE_instance = instance_p;
  ///  /* Insert the new descriptor in list of known MCE
  ///   * but not yet associated.
  ///   */
  ///  RB_INSERT(m2ap_mce_map, &instance_p->m2ap_mce_head, m2ap_mce_data_p);
  ///  m2ap_mce_data_p->state = M2AP_MCE_STATE_CONNECTED;
  ///  instance_p->m2_target_mce_nb++;

  ///  if (instance_p->m2_target_mce_pending_nb > 0) {
  ///    instance_p->m2_target_mce_pending_nb--;
  ///  }
  ///} else {
  ///  M2AP_WARN("m2ap_mce_data_p already exists\n");
  ///}

  ///printf("m2ap_MCE_handle_sctp_association_ind at 2\n");
  ///dump_mce_trees_m2();
  ////* Update parameters */
  ///m2ap_mce_data_p->assoc_id    = sctp_new_association_ind->assoc_id;
  ///m2ap_mce_data_p->in_streams  = sctp_new_association_ind->in_streams;
  ///m2ap_mce_data_p->out_streams = sctp_new_association_ind->out_streams;
  ///printf("m2ap_MCE_handle_sctp_association_ind at 3\n");
  ///dump_mce_trees_m2();
}

int m2ap_MCE_init_sctp (m2ap_MCE_instance_t *instance_p,
                        net_ip_address_t    *local_ip_addr,
                        uint32_t mce_port_for_M2C) {
  // Create and alloc new message
  MessageDef                             *message;
  sctp_init_t                            *sctp_init  = NULL;
  DevAssert(instance_p != NULL);
  DevAssert(local_ip_addr != NULL);
  message = itti_alloc_new_message (TASK_M2AP_MCE, 0, SCTP_INIT_MSG_MULTI_REQ);
  sctp_init = &message->ittiMsg.sctp_init_multi;
  sctp_init->port = mce_port_for_M2C;
  sctp_init->ppid = M2AP_SCTP_PPID;
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

static void m2ap_MCE_register_MCE(m2ap_MCE_instance_t *instance_p,
                                  net_ip_address_t    *target_MCE_ip_address,
                                  net_ip_address_t    *local_ip_addr,
                                  uint16_t             in_streams,
                                  uint16_t             out_streams,
                                  uint32_t         mce_port_for_M2C,
                                  int                  multi_sd) {
  MessageDef                       *message                   = NULL;
  sctp_new_association_req_multi_t *sctp_new_association_req  = NULL;
  m2ap_MCE_data_t                  *m2ap_mce_data             = NULL;
  DevAssert(instance_p != NULL);
  DevAssert(target_MCE_ip_address != NULL);
  message = itti_alloc_new_message(TASK_M2AP_MCE, 0, SCTP_NEW_ASSOCIATION_REQ_MULTI);
  sctp_new_association_req = &message->ittiMsg.sctp_new_association_req_multi;
  sctp_new_association_req->port = mce_port_for_M2C;
  sctp_new_association_req->ppid = M2AP_SCTP_PPID;
  sctp_new_association_req->in_streams  = in_streams;
  sctp_new_association_req->out_streams = out_streams;
  sctp_new_association_req->multi_sd = multi_sd;
  memcpy(&sctp_new_association_req->remote_address,
         target_MCE_ip_address,
         sizeof(*target_MCE_ip_address));
  memcpy(&sctp_new_association_req->local_address,
         local_ip_addr,
         sizeof(*local_ip_addr));
  /* Create new MCE descriptor */
  m2ap_mce_data = calloc(1, sizeof(*m2ap_mce_data));
  DevAssert(m2ap_mce_data != NULL);
  m2ap_mce_data->cnx_id                = m2ap_MCE_fetch_add_global_cnx_id();
  sctp_new_association_req->ulp_cnx_id = m2ap_mce_data->cnx_id;
  m2ap_mce_data->assoc_id          = -1;
  m2ap_mce_data->m2ap_MCE_instance = instance_p;
  /* Insert the new descriptor in list of known MCE
   * but not yet associated.
   */
  RB_INSERT(m2ap_mce_map, &instance_p->m2ap_mce_head, m2ap_mce_data);
  m2ap_mce_data->state = M2AP_MCE_STATE_WAITING;
  instance_p->m2_target_mce_nb ++;
  instance_p->m2_target_mce_pending_nb ++;
  itti_send_msg_to_task(TASK_SCTP, instance_p->instance, message);
}

static
void m2ap_MCE_handle_register_MCE(instance_t instance,
                                  m2ap_register_mce_req_t *m2ap_register_MCE) {
  m2ap_MCE_instance_t *new_instance;
  DevAssert(m2ap_register_MCE != NULL);
  /* Look if the provided instance already exists */
  new_instance = m2ap_MCE_get_instance(instance);

  if (new_instance != NULL) {
    /* Checks if it is a retry on the same MCE */
    DevCheck(new_instance->MCE_id == m2ap_register_MCE->MCE_id, new_instance->MCE_id, m2ap_register_MCE->MCE_id, 0);
    DevCheck(new_instance->cell_type == m2ap_register_MCE->cell_type, new_instance->cell_type, m2ap_register_MCE->cell_type, 0);
    DevCheck(new_instance->tac == m2ap_register_MCE->tac, new_instance->tac, m2ap_register_MCE->tac, 0);
    DevCheck(new_instance->mcc == m2ap_register_MCE->mcc, new_instance->mcc, m2ap_register_MCE->mcc, 0);
    DevCheck(new_instance->mnc == m2ap_register_MCE->mnc, new_instance->mnc, m2ap_register_MCE->mnc, 0);
    M2AP_WARN("MCE[%ld] already registered\n", instance);
  } else {
    new_instance = calloc(1, sizeof(m2ap_MCE_instance_t));
    DevAssert(new_instance != NULL);
    RB_INIT(&new_instance->m2ap_mce_head);
    /* Copy usefull parameters */
    new_instance->instance         = instance;
    new_instance->MCE_name         = m2ap_register_MCE->MCE_name;
    new_instance->MCE_id           = m2ap_register_MCE->MCE_id;
    new_instance->cell_type        = m2ap_register_MCE->cell_type;
    new_instance->tac              = m2ap_register_MCE->tac;
    new_instance->mcc              = m2ap_register_MCE->mcc;
    new_instance->mnc              = m2ap_register_MCE->mnc;
    new_instance->mnc_digit_length = m2ap_register_MCE->mnc_digit_length;
    new_instance->num_cc           = m2ap_register_MCE->num_cc;

    m2ap_id_manager_init(&new_instance->id_manager);
    m2ap_timers_init(&new_instance->timers,
                     m2ap_register_MCE->t_reloc_prep,
                     m2ap_register_MCE->tm2_reloc_overall);

    for (int i = 0; i< m2ap_register_MCE->num_cc; i++) {
      new_instance->eutra_band[i]              = m2ap_register_MCE->eutra_band[i];
      new_instance->downlink_frequency[i]      = m2ap_register_MCE->downlink_frequency[i];
      new_instance->uplink_frequency_offset[i] = m2ap_register_MCE->uplink_frequency_offset[i];
      new_instance->Nid_cell[i]                = m2ap_register_MCE->Nid_cell[i];
      new_instance->N_RB_DL[i]                 = m2ap_register_MCE->N_RB_DL[i];
      new_instance->frame_type[i]              = m2ap_register_MCE->frame_type[i];
      new_instance->fdd_earfcn_DL[i]           = m2ap_register_MCE->fdd_earfcn_DL[i];
      new_instance->fdd_earfcn_UL[i]           = m2ap_register_MCE->fdd_earfcn_UL[i];
    }

    DevCheck(m2ap_register_MCE->nb_m2 <= M2AP_MAX_NB_MCE_IP_ADDRESS,
             M2AP_MAX_NB_MCE_IP_ADDRESS, m2ap_register_MCE->nb_m2, 0);
    memcpy(new_instance->target_mce_m2_ip_address,
           m2ap_register_MCE->target_mce_m2_ip_address,
           m2ap_register_MCE->nb_m2 * sizeof(net_ip_address_t));
    new_instance->nb_m2             = m2ap_register_MCE->nb_m2;
    new_instance->mce_m2_ip_address = m2ap_register_MCE->mce_m2_ip_address;
    new_instance->sctp_in_streams   = m2ap_register_MCE->sctp_in_streams;
    new_instance->sctp_out_streams  = m2ap_register_MCE->sctp_out_streams;
    new_instance->mce_port_for_M2C  = m2ap_register_MCE->mce_port_for_M2C;
    /* Add the new instance to the list of MCE (meaningfull in virtual mode) */
    m2ap_MCE_insert_new_instance(new_instance);
    M2AP_INFO("Registered new MCE[%ld] and %s MCE id %u\n",
              instance,
              m2ap_register_MCE->cell_type == CELL_MACRO_ENB ? "macro" : "home",
              m2ap_register_MCE->MCE_id);

    /* initiate the SCTP listener */
    if (m2ap_MCE_init_sctp(new_instance,&m2ap_register_MCE->mce_m2_ip_address,m2ap_register_MCE->mce_port_for_M2C) <  0 ) {
      M2AP_ERROR ("Error while sending SCTP_INIT_MSG to SCTP \n");
      return;
    }

    M2AP_INFO("MCE[%ld] MCE id %u acting as a listner (server)\n",
              instance, m2ap_register_MCE->MCE_id);
  }
}

static
void m2ap_MCE_handle_sctp_init_msg_multi_cnf(
  instance_t instance_id,
  sctp_init_msg_multi_cnf_t *m) {
  m2ap_MCE_instance_t *instance;
  int index;
  DevAssert(m != NULL);
  instance = m2ap_MCE_get_instance(instance_id);
  DevAssert(instance != NULL);
  instance->multi_sd = m->multi_sd;

  /* Exit if CNF message reports failure.
   * Failure means multi_sd < 0.
   */
  if (instance->multi_sd < 0) {
    M2AP_ERROR("Error: be sure to properly configure M2 in your configuration file.\n");
    DevAssert(instance->multi_sd >= 0);
  }

  /* Trying to connect to the provided list of MCE ip address */

  for (index = 0; index < instance->nb_m2; index++) {
    M2AP_INFO("MCE[%ld] MCE id %u acting as an initiator (client)\n",
              instance_id, instance->MCE_id);
    m2ap_MCE_register_MCE(instance,
                          &instance->target_mce_m2_ip_address[index],
                          &instance->mce_m2_ip_address,
                          instance->sctp_in_streams,
                          instance->sctp_out_streams,
                          instance->mce_port_for_M2C,
                          instance->multi_sd);
  }
}

//static
//void m2ap_MCE_handle_handover_req(instance_t instance,
//                                  m2ap_handover_req_t *m2ap_handover_req)
//{
//  m2ap_MCE_instance_t *instance_p;
//  m2ap_MCE_data_t     *target;
//  m2ap_id_manager     *id_manager;
//  int                 ue_id;
//
//  int target_pci = m2ap_handover_req->target_physCellId;
//
//  instance_p = m2ap_MCE_get_instance(instance);
//  DevAssert(instance_p != NULL);
//
//  target = m2ap_is_MCE_pci_in_list(target_pci);
//  DevAssert(target != NULL);
//
//  /* allocate m2ap ID */
//  id_manager = &instance_p->id_manager;
//  ue_id = m2ap_allocate_new_id(id_manager);
//  if (ue_id == -1) {
//    M2AP_ERROR("could not allocate a new M2AP UE ID\n");
//    /* TODO: cancel handover: send (to be defined) message to RRC */
//    exit(1);
//  }
//  /* id_source is ue_id, id_target is unknown yet */
//  m2ap_set_ids(id_manager, ue_id, m2ap_handover_req->rnti, ue_id, -1);
//  m2ap_id_set_state(id_manager, ue_id, M2ID_STATE_SOURCE_PREPARE);
//  m2ap_set_reloc_prep_timer(id_manager, ue_id,
//                            m2ap_timer_get_tti(&instance_p->timers));
//  m2ap_id_set_target(id_manager, ue_id, target);
//
//  m2ap_MCE_generate_m2_handover_request(instance_p, target, m2ap_handover_req, ue_id);
//}

//static
//void m2ap_MCE_handle_handover_req_ack(instance_t instance,
//                                      m2ap_handover_req_ack_t *m2ap_handover_req_ack)
//{
//  /* TODO: remove this hack (the goal is to find the correct
//   * eNodeB structure for the other end) - we need a proper way for RRC
//   * and M2AP to identify eNodeBs
//   * RRC knows about mod_id and M2AP knows about MCE_id (MCE_ID in
//   * the configuration file)
//   * as far as I understand.. CROUX
//   */
//  m2ap_MCE_instance_t *instance_p;
//  m2ap_MCE_data_t     *target;
//  int source_assoc_id = m2ap_handover_req_ack->source_assoc_id;
//  int                 ue_id;
//  int                 id_source;
//  int                 id_target;
//
//  instance_p = m2ap_MCE_get_instance(instance);
//  DevAssert(instance_p != NULL);
//
//  target = m2ap_get_MCE(NULL, source_assoc_id, 0);
//  DevAssert(target != NULL);
//
//  /* rnti is a new information, save it */
//  ue_id     = m2ap_handover_req_ack->m2_id_target;
//  id_source = m2ap_id_get_id_source(&instance_p->id_manager, ue_id);
//  id_target = ue_id;
//  m2ap_set_ids(&instance_p->id_manager, ue_id, m2ap_handover_req_ack->rnti, id_source, id_target);
//
//  m2ap_MCE_generate_m2_handover_request_ack(instance_p, target, m2ap_handover_req_ack);
//}
//
//static
//void m2ap_MCE_ue_context_release(instance_t instance,
//                                 m2ap_ue_context_release_t *m2ap_ue_context_release)
//{
//  m2ap_MCE_instance_t *instance_p;
//  m2ap_MCE_data_t     *target;
//  int source_assoc_id = m2ap_ue_context_release->source_assoc_id;
//  int ue_id;
//  instance_p = m2ap_MCE_get_instance(instance);
//  DevAssert(instance_p != NULL);
//
//  target = m2ap_get_MCE(NULL, source_assoc_id, 0);
//  DevAssert(target != NULL);
//
//  m2ap_MCE_generate_m2_ue_context_release(instance_p, target, m2ap_ue_context_release);
//
//  /* free the M2AP UE ID */
//  ue_id = m2ap_find_id_from_rnti(&instance_p->id_manager, m2ap_ue_context_release->rnti);
//  if (ue_id == -1) {
//    M2AP_ERROR("could not find UE %x\n", m2ap_ue_context_release->rnti);
//    exit(1);
//  }
//  m2ap_release_id(&instance_p->id_manager, ue_id);
//}

void MCE_task_send_sctp_init_req(instance_t enb_id, m2ap_mce_sctp_req_t * m2ap_mce_sctp_req) {
  // 1. get the itti msg, and retrive the enb_id from the message
  // 2. use RC.rrc[enb_id] to fill the sctp_init_t with the ip, port
  // 3. creat an itti message to init

  LOG_I(M2AP, "M2AP_SCTP_REQ(create socket)\n");
  MessageDef  *message_p = NULL;

  message_p = itti_alloc_new_message (TASK_M2AP_MCE, 0, SCTP_INIT_MSG);

 // if( m2ap_mce_sctp_req == NULL ){
 //         message_p->ittiMsg.sctp_init.port = M2AP_PORT_NUMBER;
 //         message_p->ittiMsg.sctp_init.ppid = M2AP_SCTP_PPID;
 //         message_p->ittiMsg.sctp_init.ipv4 = 1;
 //         message_p->ittiMsg.sctp_init.ipv6 = 0;
 //         message_p->ittiMsg.sctp_init.nb_ipv4_addr = 1;
 //         //message_p->ittiMsg.sctp_init.ipv4_address[0] = inet_addr(RC.rrc[enb_id]->eth_params_s.my_addr);
 //         message_p->ittiMsg.sctp_init.ipv4_address[0] = inet_addr("127.0.0.7");
 //         /*
 //          * SR WARNING: ipv6 multi-homing fails sometimes for localhost.
 //          * * * * Disable it for now.
 //          */
 //         message_p->ittiMsg.sctp_init.nb_ipv6_addr = 0;
 //         message_p->ittiMsg.sctp_init.ipv6_address[0] = "0:0:0:0:0:0:0:1";
 // }else{
	  message_p->ittiMsg.sctp_init.port = m2ap_mce_sctp_req->mce_port_for_M2C;
	  message_p->ittiMsg.sctp_init.ppid = M2AP_SCTP_PPID;
	  message_p->ittiMsg.sctp_init.ipv4 = 1;
	  message_p->ittiMsg.sctp_init.ipv6 = 0;
	  message_p->ittiMsg.sctp_init.nb_ipv4_addr = 1;
	  //message_p->ittiMsg.sctp_init.ipv4_address[0] = inet_addr(RC.rrc[enb_id]->eth_params_s.my_addr);
	  message_p->ittiMsg.sctp_init.ipv4_address[0] = inet_addr(m2ap_mce_sctp_req->mce_m2_ip_address.ipv4_address);
	  /*
	   * SR WARNING: ipv6 multi-homing fails sometimes for localhost.
	   * * * * Disable it for now.
	   */
	  message_p->ittiMsg.sctp_init.nb_ipv6_addr = 0;
	  message_p->ittiMsg.sctp_init.ipv6_address[0] = "0:0:0:0:0:0:0:1";
 // }

  itti_send_msg_to_task(TASK_SCTP, enb_id, message_p);
}

void *m2ap_MCE_task(void *arg) {
  MessageDef *received_msg = NULL;
  int         result;
  M2AP_DEBUG("Starting M2AP layer\n");
  m2ap_MCE_prepare_internal_data();
  itti_mark_task_ready(TASK_M2AP_MCE);

  //MCE_task_send_sctp_init_req(0,NULL);


  while (1) {
    itti_receive_msg(TASK_M2AP_MCE, &received_msg);

    switch (ITTI_MSG_ID(received_msg)) {
      case MESSAGE_TEST:
	LOG_W(M2AP,"MCE Received MESSAGE_TEST Message\n");
	//MessageDef * message_p = itti_alloc_new_message(TASK_M2AP_MCE, 0, MESSAGE_TEST);
        //itti_send_msg_to_task(TASK_M3AP, 1/*ctxt_pP->module_id*/, message_p);
	break;
      case TERMINATE_MESSAGE:
        M2AP_WARN(" *** Exiting M2AP thread\n");
        itti_exit_task();
        break;

      case M2AP_MCE_SCTP_REQ:
	MCE_task_send_sctp_init_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
				     &M2AP_MCE_SCTP_REQ(received_msg));
	break;

      case M2AP_SUBFRAME_PROCESS:
        m2ap_check_timers(ITTI_MSG_DESTINATION_INSTANCE(received_msg));
        break;

      case M2AP_REGISTER_MCE_REQ:
	LOG_I(M2AP,"MCE Received M2AP_REGISTER_MCE_REQ Message\n");
        m2ap_MCE_handle_register_MCE(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                     &M2AP_REGISTER_MCE_REQ(received_msg));
        break;

      case M2AP_SETUP_RESP:
	LOG_I(M2AP,"MCE Received M2AP_SETUP_RESP Message\n");
	MCE_send_M2_SETUP_RESPONSE(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
				    &M2AP_SETUP_RESP(received_msg));
	break;

      case M2AP_SETUP_FAILURE:
	LOG_I(M2AP,"MCE Received M2AP_SETUP_FAILURE Message\n");
	MCE_send_M2_SETUP_FAILURE(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
				    &M2AP_SETUP_FAILURE(received_msg));
	break;

      case M2AP_MBMS_SCHEDULING_INFORMATION:
	LOG_I(M2AP,"MCE Received M2AP_MBMS_SCHEDULING_INFORMATION Message\n");
        MCE_send_MBMS_SCHEDULING_INFORMATION(0,
						&M2AP_MBMS_SCHEDULING_INFORMATION(received_msg));
	break;

       case M2AP_MBMS_SESSION_START_REQ:
	LOG_I(M2AP,"MCE Received M2AP_MBMS_SESSION_START_REQ Message\n");
        MCE_send_MBMS_SESSION_START_REQUEST(0,
						&M2AP_MBMS_SESSION_START_REQ(received_msg));
	break;

       case M2AP_MBMS_SESSION_STOP_REQ:
	LOG_I(M2AP,"MCE Received M2AP_MBMS_SESSION_STOP_REQ Message\n");
        MCE_send_MBMS_SESSION_STOP_REQUEST(0,
						&M2AP_MBMS_SESSION_STOP_REQ(received_msg));
	break;
	
       case M2AP_MBMS_SESSION_UPDATE_REQ:
	LOG_I(M2AP,"MCE Received M2AP_MBMS_SESSION_UPDATE_REQ Message\n");
        MCE_send_MBMS_SESSION_UPDATE_REQUEST(0,
						&M2AP_MBMS_SESSION_UPDATE_REQ(received_msg));
	break;

       case M2AP_RESET:
	LOG_I(M2AP,"MCE Received M2AP_RESET Message\n");
        MCE_send_RESET(0,
						&M2AP_RESET(received_msg));
	break;

       case M2AP_ENB_CONFIGURATION_UPDATE_ACK:
	LOG_I(M2AP,"MCE Received M2AP_ENB_CONFIGURATION_UPDATE_ACK Message\n");
        MCE_send_ENB_CONFIGURATION_UPDATE_ACKNOWLEDGE(0,
						&M2AP_ENB_CONFIGURATION_UPDATE_ACK(received_msg));
	break;

       case M2AP_ENB_CONFIGURATION_UPDATE_FAILURE:
	LOG_I(M2AP,"MCE Received M2AP_ENB_CONFIGURATION_UPDATE_FAILURE Message\n");
        MCE_send_ENB_CONFIGURATION_UPDATE_FAILURE(0,
						&M2AP_ENB_CONFIGURATION_UPDATE_FAILURE(received_msg));
	break;


       case M2AP_MCE_CONFIGURATION_UPDATE:
	LOG_I(M2AP,"MCE Received M2AP_MCE_CONFIGURATION_UPDATE Message\n");
        //MCE_send_MCE_CONFIGURATION_UPDATE(0,
						//&M2AP_MCE_CONFIGURATION_UPDATE(received_msg));
	break;


//      case M2AP_HANDOVER_REQ:
//        m2ap_MCE_handle_handover_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
//                                     &M2AP_HANDOVER_REQ(received_msg));
//        break;
//
//      case M2AP_HANDOVER_REQ_ACK:
//        m2ap_MCE_handle_handover_req_ack(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
//                                         &M2AP_HANDOVER_REQ_ACK(received_msg));
//        break;
//
//      case M2AP_UE_CONTEXT_RELEASE:
//        m2ap_MCE_ue_context_release(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
//                                                &M2AP_UE_CONTEXT_RELEASE(received_msg));
//        break;
//
      case SCTP_INIT_MSG_MULTI_CNF:
        LOG_D(M2AP,"MCE Received SCTP_INIT_MSG_MULTI_CNF Message\n");
        m2ap_MCE_handle_sctp_init_msg_multi_cnf(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                                &received_msg->ittiMsg.sctp_init_msg_multi_cnf);
        break;

      case SCTP_NEW_ASSOCIATION_RESP:
        LOG_D(M2AP,"MCE Received SCTP_NEW_ASSOCIATION_RESP Message\n");
        m2ap_MCE_handle_sctp_association_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                              &received_msg->ittiMsg.sctp_new_association_resp);
        break;

      case SCTP_NEW_ASSOCIATION_IND:
        LOG_D(M2AP,"MCE Received SCTP_NEW_ASSOCIATION Message\n");
        m2ap_MCE_handle_sctp_association_ind(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                             &received_msg->ittiMsg.sctp_new_association_ind);
        break;

      case SCTP_DATA_IND:
        LOG_D(M2AP,"MCE Received SCTP_DATA_IND Message\n");
        m2ap_MCE_handle_sctp_data_ind(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                      &received_msg->ittiMsg.sctp_data_ind);
        break;

      default:
        M2AP_ERROR("MCE Received unhandled message: %d:%s\n",
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

int is_m2ap_MCE_enabled(void)
{
  static volatile int config_loaded = 0;
  static volatile int enabled = 0;
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  if (pthread_mutex_lock(&mutex)) goto mutex_error;

  if (config_loaded) {
    if (pthread_mutex_unlock(&mutex)) goto mutex_error;
    return enabled;
  }

  char *enable_m2 = NULL;
  paramdef_t p[] = {
   { "enable_mce_m2", "yes/no", 0, .strptr=&enable_m2, .defstrval="", TYPE_STRING, 0 }
  };

  /* TODO: do it per module - we check only first MCE */
  config_get(config_get_if(), p, sizeofArray(p), "MCEs.[0]");
  if (enable_m2 != NULL && strcmp(enable_m2, "yes") == 0)
    enabled = 1;

  config_loaded = 1;

  if (pthread_mutex_unlock(&mutex)) goto mutex_error;

  return enabled;

mutex_error:
  LOG_E(M2AP, "mutex error\n");
  exit(1);
}
