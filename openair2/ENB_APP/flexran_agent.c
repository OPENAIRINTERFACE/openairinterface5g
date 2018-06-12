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

/*! \file flexran_agent.h
 * \brief top level flexran agent receive thread and itti task
 * \author Xenofon Foukas and Navid Nikaein and shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 */

#include "flexran_agent.h"

#include <arpa/inet.h>

void *send_thread(void *args);
void *receive_thread(void *args);
pthread_t new_thread(void *(*f)(void *), void *b);
Protocol__FlexranMessage *flexran_agent_timeout(void* args);


int agent_task_created = 0;
/* 
 * enb agent task mainly wakes up the tx thread for periodic and oneshot messages to the controller 
 * and can interact with other itti tasks
*/
void *flexran_agent_task(void *args){

  //flexran_agent_info_t         *d = (flexran_agent_info_t *) args;
  Protocol__FlexranMessage *msg;
  void *data;
  int size;
  err_code_t err_code;
  int                   priority = 0;

  MessageDef                     *msg_p           = NULL;
  const char                     *msg_name        = NULL;
  int                             result;
  struct flexran_agent_timer_element_s * elem = NULL;

  itti_mark_task_ready(TASK_FLEXRAN_AGENT);

  do {
    // Wait for a message
    itti_receive_msg (TASK_FLEXRAN_AGENT, &msg_p);
    DevAssert(msg_p != NULL);
    msg_name = ITTI_MSG_NAME (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      LOG_W(FLEXRAN_AGENT, " *** Exiting FLEXRAN thread\n");
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(FLEXRAN_AGENT, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;
    
    case TIMER_HAS_EXPIRED:
      msg = flexran_agent_process_timeout(msg_p->ittiMsg.timer_has_expired.timer_id, msg_p->ittiMsg.timer_has_expired.arg);
      if (msg != NULL){
	data=flexran_agent_pack_message(msg,&size);
	elem = get_timer_entry(msg_p->ittiMsg.timer_has_expired.timer_id);
	if (flexran_agent_msg_send(elem->agent_id, FLEXRAN_AGENT_DEFAULT, data, size, priority)) {
	  err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
	  goto error;
	}

	LOG_D(FLEXRAN_AGENT,"sent message with size %d\n", size);
      }
      break;

    default:
      LOG_E(FLEXRAN_AGENT, "Received unexpected message %s\n", msg_name);
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    continue;
  error:
    LOG_E(FLEXRAN_AGENT,"flexran_agent_task: error %d occured\n",err_code);
  } while (1);

  return NULL;
}

void *receive_thread(void *args) {

  flexran_agent_info_t  *d = args;
  void                  *data;
  int                   size;
  int                   priority;
  err_code_t             err_code;

  Protocol__FlexranMessage *msg;
  
  while (1) {

    while (flexran_agent_msg_recv(d->mod_id, FLEXRAN_AGENT_DEFAULT, &data, &size, &priority) == 0) {
      
      LOG_D(FLEXRAN_AGENT,"received message with size %d\n", size);
  
      // Invoke the message handler
      msg=flexran_agent_handle_message(d->mod_id, data, size);

      free(data);
    
      // check if there is something to send back to the controller
      if (msg != NULL){
	data=flexran_agent_pack_message(msg,&size);

	if (flexran_agent_msg_send(d->mod_id, FLEXRAN_AGENT_DEFAULT, data, size, priority)) {
	  err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
	  goto error;
	}
      
	LOG_D(FLEXRAN_AGENT,"sent message with size %d\n", size);
      } 
    }
  }
    
  return NULL;

error:
  LOG_E(FLEXRAN_AGENT,"receive_thread: error %d occured\n",err_code);
  return NULL;
}


/* utility function to create a thread */
pthread_t new_thread(void *(*f)(void *), void *b) {
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att)){ 
    fprintf(stderr, "pthread_attr_init err\n"); 
    exit(1); 
  }

  struct sched_param sched_param_recv_thread;

  sched_param_recv_thread.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
  pthread_attr_setschedparam(&att, &sched_param_recv_thread);
  pthread_attr_setschedpolicy(&att, SCHED_FIFO);

  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED)) { 
    fprintf(stderr, "pthread_attr_setdetachstate err\n"); 
    exit(1); 
  }
  if (pthread_create(&t, &att, f, b)) { 
    fprintf(stderr, "pthread_create err\n"); 
    exit(1); 
  }
  if (pthread_attr_destroy(&att)) { 
    fprintf(stderr, "pthread_attr_destroy err\n"); 
    exit(1); 
  }

  return t;
}

int channel_container_init = 0;
int flexran_agent_start(mid_t mod_id)
{
  flexran_agent_info_t *flexran = RC.flexran[mod_id];
  int channel_id;
  char *in_ip = flexran->remote_ipv4_addr;
  uint16_t in_port = flexran->remote_port;

  /* if this agent is disabled, return and don't do anything */
  if (!flexran->enabled) {
    LOG_I(FLEXRAN_AGENT, "FlexRAN Agent for eNB %d is DISABLED\n", mod_id);
    return 100;
  }

  /*
   * Initialize the channel container
   */
  if (!channel_container_init) {
    flexran_agent_init_channel_container();
    channel_container_init = 1;
  }
  /*Create the async channel info*/
  flexran_agent_async_channel_t *channel_info = flexran_agent_async_channel_info(mod_id, in_ip, in_port);

  /*Create a channel using the async channel info*/
  channel_id = flexran_agent_create_channel((void *) channel_info, 
					flexran_agent_async_msg_send, 
					flexran_agent_async_msg_recv,
					flexran_agent_async_release);

  
  if (channel_id <= 0) {
    goto error;
  }

  flexran_agent_channel_t *channel = get_channel(channel_id);
  
  if (channel == NULL) {
    goto error;
  }

  /*Register the channel for all underlying agents (use FLEXRAN_AGENT_MAX)*/
  flexran_agent_register_channel(mod_id, channel, FLEXRAN_AGENT_MAX);

  /*Example of registration for a specific agent(MAC):
   *flexran_agent_register_channel(mod_id, channel, FLEXRAN_AGENT_MAC);
   */

  /*Initialize the continuous stats update mechanism*/
  flexran_agent_init_cont_stats_update(mod_id);
  
  new_thread(receive_thread, flexran);

  /*Initialize and register the mac xface. Must be modified later
   *for more flexibility in agent management */

  AGENT_MAC_xface *mac_agent_xface = (AGENT_MAC_xface *) malloc(sizeof(AGENT_MAC_xface));
  flexran_agent_register_mac_xface(mod_id, mac_agent_xface);
  
  AGENT_RRC_xface *rrc_agent_xface = (AGENT_RRC_xface *) malloc(sizeof(AGENT_RRC_xface));
  flexran_agent_register_rrc_xface(mod_id, rrc_agent_xface);

  AGENT_PDCP_xface *pdcp_agent_xface = (AGENT_PDCP_xface *) malloc(sizeof(AGENT_PDCP_xface));
  flexran_agent_register_pdcp_xface(mod_id, pdcp_agent_xface);

  /* 
   * initilize a timer 
   */ 
  
  flexran_agent_init_timer();

  /*
   * Initialize the mac agent
   */
  flexran_agent_init_mac_agent(mod_id);
  
  /* 
   * start the enb agent task for tx and interaction with the underlying network function
   */ 
  if (!agent_task_created) {
    if (itti_create_task (TASK_FLEXRAN_AGENT, flexran_agent_task, flexran) < 0) {
      LOG_E(FLEXRAN_AGENT, "Create task for FlexRAN Agent failed\n");
      return -1;
    }
    agent_task_created = 1;
  }

  pthread_mutex_init(&flexran->mutex_node_ctrl, NULL);
  pthread_cond_init(&flexran->cond_node_ctrl, NULL);

  if (flexran->node_ctrl_state == ENB_WAIT) {
    /* wait three seconds before showing message and waiting "for real".
     * This way, the message is (hopefully...) the last one and the user knows
     * what is happening. If the controller sends a reconfiguration message in
     * the meantime, the softmodem will never wait */
    sleep(3);
    LOG_I(ENB_APP, " * eNB %d: Waiting for FlexRAN RTController command *\n", mod_id);
    pthread_mutex_lock(&flexran->mutex_node_ctrl);
    while (ENB_NORMAL_OPERATION != flexran->node_ctrl_state)
      pthread_cond_wait(&flexran->cond_node_ctrl, &flexran->mutex_node_ctrl);
    pthread_mutex_unlock(&flexran->mutex_node_ctrl);

    /* reconfigure RRC again, the agent might have changed the configuration */
    MessageDef *msg_p = itti_alloc_new_message(TASK_ENB_APP, RRC_CONFIGURATION_REQ);
    RRC_CONFIGURATION_REQ(msg_p) = RC.rrc[mod_id]->configuration;
    itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(mod_id), msg_p);
  }

  return 0;

error:
  LOG_I(FLEXRAN_AGENT,"there was an error\n");
  return 1;

}

Protocol__FlexranMessage *flexran_agent_timeout(void* args){

  //  flexran_agent_timer_args_t *timer_args = calloc(1, sizeof(*timer_args));
  //memcpy (timer_args, args, sizeof(*timer_args));
  flexran_agent_timer_args_t *timer_args = (flexran_agent_timer_args_t *) args;
  
  LOG_I(FLEXRAN_AGENT, "flexran_agent %d timeout\n", timer_args->mod_id);
  //LOG_I(FLEXRAN_AGENT, "eNB action %d ENB flags %d \n", timer_args->cc_actions,timer_args->cc_report_flags);
  //LOG_I(FLEXRAN_AGENT, "UE action %d UE flags %d \n", timer_args->ue_actions,timer_args->ue_report_flags);
  
  return NULL;
}
