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

static int s1ap_sctp_req(s1ap_eNB_instance_t *instance_p,
                         s1ap_eNB_mme_data_t *s1ap_mme_data_p);
void s1ap_eNB_timer_expired(instance_t                 instance,
                            timer_has_expired_t   *msg_p);

int s1ap_timer_setup(
  uint32_t      interval_sec,
  uint32_t      interval_us,
  task_id_t     task_id,
  int32_t       instance,
  uint32_t      timer_kind,
  timer_type_t  type,
  void         *timer_arg,
  long         *timer_id)
{
  uint32_t *timeoutArg=NULL;
  int ret=0;
  timeoutArg=malloc(sizeof(uint32_t));
  *timeoutArg=timer_kind;
  ret=timer_setup(interval_sec,
                interval_us,
                task_id,
                instance,
                type,
                (void*)timeoutArg,
                timer_id);
  return ret;
}

int s1ap_timer_remove(long timer_id)
{
  int ret;
  ret=timer_remove(timer_id);
  return ret;
}

uint32_t s1ap_generate_eNB_id(void) {
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
                                  uint16_t             mme_port,
                                  net_ip_address_t    *local_ip_addr,
                                  uint16_t             in_streams,
                                  uint16_t             out_streams,
                                  uint8_t              broadcast_plmn_num,
                                  uint8_t              broadcast_plmn_index[PLMN_LIST_MAX_SIZE]) {
  MessageDef                 *message_p                   = NULL;
  sctp_new_association_req_t *sctp_new_association_req_p  = NULL;
  s1ap_eNB_mme_data_t        *s1ap_mme_data_p             = NULL;
//  struct s1ap_eNB_mme_data_s *mme                         = NULL;
  DevAssert(instance_p != NULL);
  DevAssert(mme_ip_address != NULL);
  message_p = itti_alloc_new_message(TASK_S1AP, 0, SCTP_NEW_ASSOCIATION_REQ);
  sctp_new_association_req_p = &message_p->ittiMsg.sctp_new_association_req;
  sctp_new_association_req_p->port = mme_port;
  sctp_new_association_req_p->ppid = S1AP_SCTP_PPID;
  sctp_new_association_req_p->in_streams  = in_streams;
  sctp_new_association_req_p->out_streams = out_streams;
  memcpy(&sctp_new_association_req_p->remote_address,
         mme_ip_address,
         sizeof(*mme_ip_address));
  memcpy(&sctp_new_association_req_p->local_address,
         local_ip_addr,
         sizeof(*local_ip_addr));
  S1AP_INFO("[eNB %ld] check the mme registration state\n",instance_p->instance);
  /* Create new MME descriptor */
  s1ap_mme_data_p = calloc(1, sizeof(*s1ap_mme_data_p));
  DevAssert(s1ap_mme_data_p != NULL);
  s1ap_mme_data_p->cnx_id                = s1ap_eNB_fetch_add_global_cnx_id();
  sctp_new_association_req_p->ulp_cnx_id = s1ap_mme_data_p->cnx_id;
  s1ap_mme_data_p->assoc_id          = -1;
  s1ap_mme_data_p->broadcast_plmn_num = broadcast_plmn_num;
  memcpy(&s1ap_mme_data_p->mme_s1_ip,
         mme_ip_address,
         sizeof(*mme_ip_address));
  s1ap_mme_data_p->mme_port = mme_port;
  for (int i = 0; i < broadcast_plmn_num; ++i)
    s1ap_mme_data_p->broadcast_plmn_index[i] = broadcast_plmn_index[i];

  s1ap_mme_data_p->s1ap_eNB_instance = instance_p;
  STAILQ_INIT(&s1ap_mme_data_p->served_gummei);
  /* Insert the new descriptor in list of known MME
   * but not yet associated.
   */
  RB_INSERT(s1ap_mme_map, &instance_p->s1ap_mme_head, s1ap_mme_data_p);
  s1ap_mme_data_p->state = S1AP_ENB_STATE_DISCONNECTED;
  memcpy( &(s1ap_mme_data_p->mme_ip_address), mme_ip_address, sizeof(net_ip_address_t) );
  s1ap_mme_data_p->overload_state = S1AP_NO_OVERLOAD;
  s1ap_mme_data_p->sctp_req_cnt++;
  s1ap_mme_data_p->timer_id = S1AP_TIMERID_INIT;
  instance_p->s1ap_mme_pending_nb ++;

  itti_send_msg_to_task(TASK_SCTP, instance_p->instance, message_p);
}


void s1ap_eNB_handle_register_eNB(instance_t instance, s1ap_register_enb_req_t *s1ap_register_eNB) {
  s1ap_eNB_instance_t *new_instance;
  uint8_t index;
  DevAssert(s1ap_register_eNB != NULL);
  /* Look if the provided instance already exists */
  new_instance = s1ap_eNB_get_instance(instance);

  if (new_instance != NULL) {
    /* Checks if it is a retry on the same eNB */
    DevCheck(new_instance->eNB_id == s1ap_register_eNB->eNB_id, new_instance->eNB_id, s1ap_register_eNB->eNB_id, 0);
    DevCheck(new_instance->cell_type == s1ap_register_eNB->cell_type, new_instance->cell_type, s1ap_register_eNB->cell_type, 0);
    DevCheck(new_instance->num_plmn == s1ap_register_eNB->num_plmn, new_instance->num_plmn, s1ap_register_eNB->num_plmn, 0);
    DevCheck(new_instance->tac == s1ap_register_eNB->tac, new_instance->tac, s1ap_register_eNB->tac, 0);

    for (int i = 0; i < new_instance->num_plmn; i++) {
      DevCheck(new_instance->mcc[i] == s1ap_register_eNB->mcc[i], new_instance->mcc[i], s1ap_register_eNB->mcc[i], 0);
      DevCheck(new_instance->mnc[i] == s1ap_register_eNB->mnc[i], new_instance->mnc[i], s1ap_register_eNB->mnc[i], 0);
      DevCheck(new_instance->mnc_digit_length[i] == s1ap_register_eNB->mnc_digit_length[i], new_instance->mnc_digit_length[i], s1ap_register_eNB->mnc_digit_length[i], 0);
    }

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

    memcpy(&new_instance->eNB_s1_ip,
       &s1ap_register_eNB->enb_ip_address,
       sizeof(s1ap_register_eNB->enb_ip_address));

    for (int i = 0; i < s1ap_register_eNB->num_plmn; i++) {
      new_instance->mcc[i]              = s1ap_register_eNB->mcc[i];
      new_instance->mnc[i]              = s1ap_register_eNB->mnc[i];
      new_instance->mnc_digit_length[i] = s1ap_register_eNB->mnc_digit_length[i];
    }

    memcpy( &(new_instance->enb_ip_address), &(s1ap_register_eNB->enb_ip_address), sizeof(net_ip_address_t) );
    new_instance->s1_setuprsp_wait_timer = s1ap_register_eNB->s1_setuprsp_wait_timer;
    new_instance->s1_setupreq_wait_timer = s1ap_register_eNB->s1_setupreq_wait_timer;
    new_instance->s1_setupreq_count = s1ap_register_eNB->s1_setupreq_count;
    new_instance->sctp_req_timer = s1ap_register_eNB->sctp_req_timer;
    new_instance->sctp_req_count = s1ap_register_eNB->sctp_req_count;
    new_instance->sctp_in_streams = s1ap_register_eNB->sctp_in_streams;
    new_instance->sctp_out_streams = s1ap_register_eNB->sctp_out_streams;

    new_instance->num_plmn         = s1ap_register_eNB->num_plmn;
    new_instance->default_drx      = s1ap_register_eNB->default_drx;
    /* Add the new instance to the list of eNB (meaningfull in virtual mode) */
    s1ap_eNB_insert_new_instance(new_instance);
    S1AP_INFO("Registered new eNB[%ld] and %s eNB id %u\n",
              instance,
              s1ap_register_eNB->cell_type == CELL_MACRO_ENB ? "macro" : "home",
              s1ap_register_eNB->eNB_id);
  }

  if( s1ap_register_eNB->nb_mme > S1AP_MAX_NB_MME_IP_ADDRESS )
  {
    S1AP_ERROR("Invalid MME number = %d\n ", s1ap_register_eNB->nb_mme);
    s1ap_register_eNB->nb_mme = S1AP_MAX_NB_MME_IP_ADDRESS;
  }
  new_instance->s1ap_mme_nb = s1ap_register_eNB->nb_mme;

  /* Trying to connect to provided list of MME ip address */
  for (index = 0; index < s1ap_register_eNB->nb_mme; index++) {
    net_ip_address_t *mme_ip = &s1ap_register_eNB->mme_ip_address[index];
    uint16_t mme_port = s1ap_register_eNB->mme_port[index];
    struct s1ap_eNB_mme_data_s *mme = NULL;
    RB_FOREACH(mme, s1ap_mme_map, &new_instance->s1ap_mme_head) {
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
    s1ap_eNB_register_mme(new_instance,
                          mme_ip,
                          mme_port,
                          &s1ap_register_eNB->enb_ip_address,
                          s1ap_register_eNB->sctp_in_streams,
                          s1ap_register_eNB->sctp_out_streams,
                          s1ap_register_eNB->broadcast_plmn_num[index],
                          s1ap_register_eNB->broadcast_plmn_index[index]);
  }
}

void s1ap_eNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp) {
  s1ap_eNB_instance_t *instance_p;
  s1ap_eNB_mme_data_t *s1ap_mme_data_p;
  s1ap_eNB_ue_context_t        *ue_p = NULL;
  MessageDef                   *message_p = NULL;
  uint32_t                     timer_kind = 0;
  struct plmn_identity_s*      plmnInfo;
  struct served_group_id_s*    groupInfo;
  struct served_gummei_s*      gummeiInfo;
  struct mme_code_s*           mmeCode;
  int8_t                       cnt = 0;
  unsigned                     enb_s1ap_id[NUMBER_OF_UE_MAX];
  DevAssert(sctp_new_association_resp != NULL);
  instance_p = s1ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);
  s1ap_mme_data_p = s1ap_eNB_get_MME(instance_p, -1,
                                     sctp_new_association_resp->ulp_cnx_id);
  DevAssert(s1ap_mme_data_p != NULL);
  memset(enb_s1ap_id, 0, sizeof(enb_s1ap_id) );
  if( s1ap_mme_data_p->timer_id != S1AP_TIMERID_INIT ) {
    s1ap_timer_remove( s1ap_mme_data_p->timer_id );
    s1ap_mme_data_p->timer_id = S1AP_TIMERID_INIT;
  }
  
  if( sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED ) {
    RB_FOREACH(ue_p, s1ap_ue_map, &instance_p->s1ap_ue_head) {
      if( ue_p->mme_ref == s1ap_mme_data_p ) {
        if(cnt < NUMBER_OF_UE_MAX) {
          enb_s1ap_id[cnt] = ue_p->eNB_ue_s1ap_id;
          cnt++;
          
          message_p = NULL;
          message_p = itti_alloc_new_message(TASK_S1AP, 0, S1AP_UE_CONTEXT_RELEASE_COMMAND);
          
          if( message_p != NULL ) {
            S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).eNB_ue_s1ap_id = ue_p->eNB_ue_s1ap_id;
            
            if( itti_send_msg_to_task(TASK_RRC_ENB, ue_p->eNB_instance->instance, message_p) < 0 ) {
              S1AP_ERROR("UE Context Release Command Transmission Failure: eNB_ue_s1ap_id=%u\n", ue_p->eNB_ue_s1ap_id);
            }
          } else {
            S1AP_ERROR("Invalid message_p : eNB_ue_s1ap_id=%u\n", ue_p->eNB_ue_s1ap_id);
          }
        }else{
          S1AP_ERROR("s1ap_eNB_handle_sctp_association_resp: cnt %d > max\n", cnt);
        }
      }
    }
    for( ; cnt > 0 ; ) {
      cnt--;
      struct s1ap_eNB_ue_context_s *ue_context_p = NULL;
      struct s1ap_eNB_ue_context_s *s1ap_ue_context_p = NULL;
      ue_context_p = s1ap_eNB_get_ue_context(instance_p, (uint32_t)enb_s1ap_id[cnt]);
      if (ue_context_p != NULL) {
        s1ap_ue_context_p = RB_REMOVE(s1ap_ue_map, &instance_p->s1ap_ue_head, ue_context_p);
        s1ap_eNB_free_ue_context(s1ap_ue_context_p);
      }
    }
    s1ap_mme_data_p->mme_name = 0;
    s1ap_mme_data_p->overload_state = S1AP_NO_OVERLOAD;
    s1ap_mme_data_p->nextstream = 0;
    s1ap_mme_data_p->in_streams = 0;
    s1ap_mme_data_p->out_streams = 0;
    s1ap_mme_data_p->assoc_id = -1;
    
    while (!STAILQ_EMPTY(&s1ap_mme_data_p->served_gummei)) {
      gummeiInfo = STAILQ_FIRST(&s1ap_mme_data_p->served_gummei);
      STAILQ_REMOVE_HEAD(&s1ap_mme_data_p->served_gummei, next);

      while (!STAILQ_EMPTY(&gummeiInfo->served_plmns)) {
        plmnInfo = STAILQ_FIRST(&gummeiInfo->served_plmns);
        STAILQ_REMOVE_HEAD(&gummeiInfo->served_plmns, next);
        free(plmnInfo);
      }
      while (!STAILQ_EMPTY(&gummeiInfo->served_group_ids)) {
        groupInfo = STAILQ_FIRST(&gummeiInfo->served_group_ids);
        STAILQ_REMOVE_HEAD(&gummeiInfo->served_group_ids, next);
        free(groupInfo);
      }
      while (!STAILQ_EMPTY(&gummeiInfo->mme_codes)) {
        mmeCode = STAILQ_FIRST(&gummeiInfo->mme_codes);
        STAILQ_REMOVE_HEAD(&gummeiInfo->mme_codes, next);
        free(mmeCode);
      }
      free(gummeiInfo);
    }
    
    STAILQ_INIT(&s1ap_mme_data_p->served_gummei);
    
    if( s1ap_mme_data_p->state == S1AP_ENB_STATE_DISCONNECTED ) {
      if( (s1ap_mme_data_p->sctp_req_cnt <= instance_p->sctp_req_count) ||
        (instance_p->sctp_req_count == 0xffff) ) {
        timer_kind = s1ap_mme_data_p->cnx_id;
        timer_kind = timer_kind | S1AP_MMEIND;
        timer_kind = timer_kind | SCTP_REQ_WAIT;
        
        if( s1ap_timer_setup( instance_p->sctp_req_timer, 0, TASK_S1AP, instance_p->instance,
          timer_kind, S1AP_TIMER_ONE_SHOT, NULL, &s1ap_mme_data_p->timer_id) < 0 ) {
          S1AP_ERROR("Timer Start NG(SCTP retransmission wait timer) : MME=%d\n",s1ap_mme_data_p->cnx_id);
          s1ap_sctp_req( instance_p, s1ap_mme_data_p );
        }
      } else {
        S1AP_ERROR("Retransmission count exceeded of SCTP : MME=%d\n",s1ap_mme_data_p->cnx_id);
      }
    } else {
      S1AP_ERROR("SCTP disconnection reception : MME = %d\n",s1ap_mme_data_p->cnx_id);

      if( (s1ap_mme_data_p->sctp_req_cnt <= instance_p->sctp_req_count) ||
          (instance_p->sctp_req_count == 0xffff) ) {
        s1ap_sctp_req( instance_p, s1ap_mme_data_p );
      } else {
        S1AP_ERROR("Retransmission count exceeded of SCTP : MME=%d\n",s1ap_mme_data_p->cnx_id);
      }
      s1ap_mme_data_p->state = S1AP_ENB_STATE_DISCONNECTED;
    }
  } else {
    /* Update parameters */
    s1ap_mme_data_p->assoc_id    = sctp_new_association_resp->assoc_id;
    s1ap_mme_data_p->in_streams  = sctp_new_association_resp->in_streams;
    s1ap_mme_data_p->out_streams = sctp_new_association_resp->out_streams;
    /* Prepare new S1 Setup Request */
    s1ap_mme_data_p->s1_setupreq_cnt = 0;
    s1ap_mme_data_p->sctp_req_cnt = 0;
    if (s1ap_eNB_generate_s1_setup_request(instance_p, s1ap_mme_data_p) == -1) {
      S1AP_ERROR("s1ap eNB generate s1 setup request failed\n");
      return;
    }
  }
}

static
void s1ap_eNB_handle_sctp_data_ind(sctp_data_ind_t *sctp_data_ind) {
  int result;
  DevAssert(sctp_data_ind != NULL);
#if defined(TEST_S1C_MME)
  mme_test_s1_notify_sctp_data_ind(sctp_data_ind->assoc_id, sctp_data_ind->stream,
                                   sctp_data_ind->buffer, sctp_data_ind->buffer_length);
#else
  if (s1ap_eNB_handle_message(sctp_data_ind->assoc_id, sctp_data_ind->stream,
                          sctp_data_ind->buffer, sctp_data_ind->buffer_length) == -1) {
    S1AP_ERROR("Failed to handle s1ap eNB message\n");
  }
#endif
  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
}

void s1ap_eNB_timer_expired(
  instance_t                instance,
  timer_has_expired_t   *msg_p)
{
  uint32_t                  timer_kind = 0;
  int16_t                   line_ind = 0;
  s1ap_eNB_mme_data_t       *mme_desc_p = NULL;
  uint32_t                  timer_ind = 0;
  s1ap_eNB_instance_t       *instance_p = NULL;
  long                      timer_id = S1AP_TIMERID_INIT;
  
  instance_p = s1ap_eNB_get_instance(instance);
  if(msg_p->arg!=NULL){
    timer_kind = *((uint32_t*)msg_p->arg);
    free(msg_p->arg);
  }else{
    S1AP_ERROR("s1 timer timer_kind is NULL\n");
    return;
  }
  line_ind = (int16_t)(timer_kind & S1AP_LINEIND);
  timer_id = msg_p->timer_id;
  
  if( (timer_kind & S1AP_MMEIND) == S1AP_MMEIND ) {
    mme_desc_p = s1ap_eNB_get_MME(instance_p, -1, line_ind);
    if(mme_desc_p != NULL) {
      if( timer_id == mme_desc_p->timer_id ) {
        mme_desc_p->timer_id = S1AP_TIMERID_INIT;
        timer_ind = timer_kind & S1AP_TIMERIND;
        
        switch(timer_ind)
        {
          case S1_SETRSP_WAIT:
          {
            if( (instance_p->s1_setupreq_count >= mme_desc_p->s1_setupreq_cnt) ||
                (instance_p->s1_setupreq_count == 0xffff) ) {
              s1ap_eNB_generate_s1_setup_request( instance_p, mme_desc_p );
            } else {
              S1AP_ERROR("Retransmission count exceeded of S1 SETUP REQUEST : MME=%d\n",line_ind);
            }
            break;
          }
          case S1_SETREQ_WAIT:
          {
            if( (instance_p->s1_setupreq_count >= mme_desc_p->s1_setupreq_cnt) ||
                (instance_p->s1_setupreq_count == 0xffff) ) {
              s1ap_eNB_generate_s1_setup_request( instance_p, mme_desc_p );
            } else {
              S1AP_ERROR("Retransmission count exceeded of S1 SETUP REQUEST : MME=%d\n",line_ind);
            }
            break;
          }
          case SCTP_REQ_WAIT:
          {
            if( (instance_p->sctp_req_count >= mme_desc_p->sctp_req_cnt) ||
                (instance_p->sctp_req_count == 0xffff) ) {
              s1ap_sctp_req( instance_p, mme_desc_p );
            } else {
              S1AP_ERROR("Retransmission count exceeded of SCTP : MME=%d\n",line_ind);
            }
            break;
          }
          default:
          {
            S1AP_WARN("Invalid Timer indication\n");
            break;
          }
        }
      } else {
        S1AP_DEBUG("Unmatch timer id\n");
        return;
      }
    } else {
      S1AP_WARN("Not applicable MME detected : connection id = %d\n", line_ind);
      return;
    }
  }
  return;
}

void s1ap_eNB_init(void) {
  S1AP_DEBUG("Starting S1AP layer\n");
  s1ap_eNB_prepare_internal_data();
  itti_mark_task_ready(TASK_S1AP);
  MSC_START_USE();
}

void *s1ap_eNB_process_itti_msg(void *notUsed) {
  MessageDef *received_msg = NULL;
  int         result;
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

    case S1AP_NAS_FIRST_REQ: {
      s1ap_eNB_handle_nas_first_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                    &S1AP_NAS_FIRST_REQ(received_msg));
    }
    break;

    case S1AP_UPLINK_NAS: {
      s1ap_eNB_nas_uplink(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                          &S1AP_UPLINK_NAS(received_msg));
    }
    break;

    case S1AP_UE_CAPABILITIES_IND: {
      s1ap_eNB_ue_capabilities(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                               &S1AP_UE_CAPABILITIES_IND(received_msg));
    }
    break;

    case S1AP_INITIAL_CONTEXT_SETUP_RESP: {
      s1ap_eNB_initial_ctxt_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                 &S1AP_INITIAL_CONTEXT_SETUP_RESP(received_msg));
    }
    break;

    case S1AP_E_RAB_SETUP_RESP: {
      s1ap_eNB_e_rab_setup_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                &S1AP_E_RAB_SETUP_RESP(received_msg));
    }
    break;

    case S1AP_E_RAB_MODIFY_RESP: {
      s1ap_eNB_e_rab_modify_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                 &S1AP_E_RAB_MODIFY_RESP(received_msg));
    }
    break;

    case S1AP_NAS_NON_DELIVERY_IND: {
      s1ap_eNB_nas_non_delivery_ind(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                    &S1AP_NAS_NON_DELIVERY_IND(received_msg));
    }
    break;

    case S1AP_PATH_SWITCH_REQ: {
      s1ap_eNB_path_switch_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                               &S1AP_PATH_SWITCH_REQ(received_msg));
    }
    break;

    case S1AP_E_RAB_MODIFICATION_IND: {
    	s1ap_eNB_generate_E_RAB_Modification_Indication(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
    	                               &S1AP_E_RAB_MODIFICATION_IND(received_msg));
    }
    break;

    case S1AP_UE_CONTEXT_RELEASE_COMPLETE: {
      s1ap_ue_context_release_complete(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                       &S1AP_UE_CONTEXT_RELEASE_COMPLETE(received_msg));
    }
    break;

    case S1AP_UE_CONTEXT_RELEASE_REQ: {
      s1ap_eNB_instance_t               *s1ap_eNB_instance_p           = NULL; // test
      struct s1ap_eNB_ue_context_s      *ue_context_p                  = NULL; // test
      s1ap_ue_context_release_req(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                  &S1AP_UE_CONTEXT_RELEASE_REQ(received_msg));
      s1ap_eNB_instance_p = s1ap_eNB_get_instance(ITTI_MSG_DESTINATION_INSTANCE(received_msg)); // test
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
      s1ap_eNB_e_rab_release_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                  &S1AP_E_RAB_RELEASE_RESPONSE(received_msg));
    }
    break;

    case TIMER_HAS_EXPIRED:
    {
      s1ap_eNB_timer_expired(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                             &received_msg->ittiMsg.timer_has_expired);
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
  return NULL;
}


void *s1ap_eNB_task(void *arg) {
  s1ap_eNB_init();

  while (1) {
    (void) s1ap_eNB_process_itti_msg(NULL);
  }

  return NULL;
}

//-----------------------------------------------------------------------------
/*
* eNB generate a S1 setup request towards MME
*/
static int s1ap_eNB_generate_s1_setup_request(
  s1ap_eNB_instance_t *instance_p,
  s1ap_eNB_mme_data_t *s1ap_mme_data_p)
//-----------------------------------------------------------------------------
{
  S1AP_S1AP_PDU_t            pdu;
  S1AP_S1SetupRequest_t     *out = NULL;
  S1AP_S1SetupRequestIEs_t   *ie = NULL;
  S1AP_SupportedTAs_Item_t   *ta = NULL;
  S1AP_PLMNidentity_t      *plmn = NULL;
  uint8_t  *buffer = NULL;
  uint32_t  len = 0;
  int       ret = 0;
  uint32_t  timer_kind = 0;
  DevAssert(instance_p != NULL);
  DevAssert(s1ap_mme_data_p != NULL);
  s1ap_mme_data_p->state = S1AP_ENB_STATE_WAITING;
  /* Prepare the S1AP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_S1Setup;
  pdu.choice.initiatingMessage.criticality = S1AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_S1SetupRequest;
  out = &pdu.choice.initiatingMessage.value.choice.S1SetupRequest;
  /* mandatory */
  ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_Global_ENB_ID;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_S1SetupRequestIEs__value_PR_Global_ENB_ID;
  MCC_MNC_TO_PLMNID(instance_p->mcc[s1ap_mme_data_p->broadcast_plmn_index[0]],
                    instance_p->mnc[s1ap_mme_data_p->broadcast_plmn_index[0]],
                    instance_p->mnc_digit_length[s1ap_mme_data_p->broadcast_plmn_index[0]],
                    &ie->value.choice.Global_ENB_ID.pLMNidentity);
  ie->value.choice.Global_ENB_ID.eNB_ID.present = S1AP_ENB_ID_PR_macroENB_ID;
  MACRO_ENB_ID_TO_BIT_STRING(instance_p->eNB_id,
                             &ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID);
  S1AP_INFO("%d -> %02x%02x%02x\n", instance_p->eNB_id,
            ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.buf[0],
            ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.buf[1],
            ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.buf[2]);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  if (instance_p->eNB_name) {
    ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNBname;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_S1SetupRequestIEs__value_PR_ENBname;
    OCTET_STRING_fromBuf(&ie->value.choice.ENBname, instance_p->eNB_name,
                         strlen(instance_p->eNB_name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_SupportedTAs;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_S1SetupRequestIEs__value_PR_SupportedTAs;
  {
    ta = (S1AP_SupportedTAs_Item_t *)calloc(1, sizeof(S1AP_SupportedTAs_Item_t));
    INT16_TO_OCTET_STRING(instance_p->tac, &ta->tAC);
    {
      for (int i = 0; i < s1ap_mme_data_p->broadcast_plmn_num; ++i) {
        plmn = (S1AP_PLMNidentity_t *)calloc(1, sizeof(S1AP_PLMNidentity_t));
        MCC_MNC_TO_TBCD(instance_p->mcc[s1ap_mme_data_p->broadcast_plmn_index[i]],
                        instance_p->mnc[s1ap_mme_data_p->broadcast_plmn_index[i]],
                        instance_p->mnc_digit_length[s1ap_mme_data_p->broadcast_plmn_index[i]],
                        plmn);
        ASN_SEQUENCE_ADD(&ta->broadcastPLMNs.list, plmn);
      }
    }
    ASN_SEQUENCE_ADD(&ie->value.choice.SupportedTAs.list, ta);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_DefaultPagingDRX;
  ie->criticality = S1AP_Criticality_ignore;
  ie->value.present = S1AP_S1SetupRequestIEs__value_PR_PagingDRX;
  ie->value.choice.PagingDRX = instance_p->default_drx;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (s1ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    S1AP_ERROR("Failed to encode S1 setup request\n");
    return -1;
  }

  timer_kind = s1ap_mme_data_p->cnx_id;
  timer_kind = timer_kind | S1AP_MMEIND;
  timer_kind = timer_kind | S1_SETRSP_WAIT;
  
  if( s1ap_timer_setup(instance_p->s1_setuprsp_wait_timer, 0, TASK_S1AP, instance_p->instance, timer_kind, S1AP_TIMER_ONE_SHOT,
    NULL, &s1ap_mme_data_p->timer_id) < 0 )
  {
    S1AP_ERROR("Timer Start NG(S1 Setup Response) : MME=%d\n",s1ap_mme_data_p->cnx_id);
  }
  s1ap_mme_data_p->s1_setupreq_cnt++;

  /* Non UE-Associated signalling -> stream = 0 */
  s1ap_eNB_itti_send_sctp_data_req(instance_p->instance, s1ap_mme_data_p->assoc_id, buffer, len, 0);
  return ret;
}




static int s1ap_sctp_req(s1ap_eNB_instance_t *instance_p,
                         s1ap_eNB_mme_data_t *s1ap_mme_data_p)
{
  MessageDef                 *message_p                   = NULL;
  sctp_new_association_req_t *sctp_new_association_req_p  = NULL;
  
  if( instance_p == NULL )
  {
      S1AP_ERROR("Invalid instance_p\n");
      return -1;
  }
  
  message_p = itti_alloc_new_message(TASK_S1AP, 0, SCTP_NEW_ASSOCIATION_REQ);
  sctp_new_association_req_p = &message_p->ittiMsg.sctp_new_association_req;
  sctp_new_association_req_p->port = S1AP_PORT_NUMBER;
  sctp_new_association_req_p->ppid = S1AP_SCTP_PPID;
  sctp_new_association_req_p->in_streams  = instance_p->sctp_in_streams;
  sctp_new_association_req_p->out_streams = instance_p->sctp_out_streams;
  sctp_new_association_req_p->ulp_cnx_id = s1ap_mme_data_p->cnx_id;
  
  if( s1ap_mme_data_p->mme_ip_address.ipv4 != 0 ) {
    memcpy(&sctp_new_association_req_p->remote_address,
        &s1ap_mme_data_p->mme_ip_address,
        sizeof(net_ip_address_t));
    if( instance_p->enb_ip_address.ipv4 != 0 ) {
      memcpy(&sctp_new_association_req_p->local_address,
          &instance_p->enb_ip_address,
          sizeof(net_ip_address_t));
    } else {
      S1AP_ERROR("Invalid IP Address Format V4(MME):V6\n");
      return -1;
    }
  } else {
    memcpy(&sctp_new_association_req_p->remote_address,
        &s1ap_mme_data_p->mme_ip_address,
        sizeof(net_ip_address_t));
    if( instance_p->enb_ip_address.ipv6 != 0 ) {
      memcpy(&sctp_new_association_req_p->local_address,
          &instance_p->enb_ip_address,
          sizeof(net_ip_address_t));
    } else {
      S1AP_ERROR("Invalid IP Address Format V6(MME):V4\n");
      return -1;
    }
  }
  
  itti_send_msg_to_task(TASK_SCTP, instance_p->instance, message_p);
  
  s1ap_mme_data_p->sctp_req_cnt++;
  
  return 0;
}
