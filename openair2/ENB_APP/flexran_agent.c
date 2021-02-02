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

#define _GNU_SOURCE
#include "flexran_agent.h"
#include <common/utils/system.h>

#include <pthread.h>
#include <arpa/inet.h>

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

int agent_task_created = 0;
/* 
 * enb agent task mainly wakes up the tx thread for periodic and oneshot messages to the controller 
 * and can interact with other itti tasks
*/
void *flexran_agent_task(void *args){
  MessageDef *msg_p = NULL;
  int         result;

  itti_mark_task_ready(TASK_FLEXRAN_AGENT);

  do {
    // Wait for a message
    itti_receive_msg (TASK_FLEXRAN_AGENT, &msg_p);
    DevAssert(msg_p != NULL);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      LOG_W(FLEXRAN_AGENT, " *** Exiting FLEXRAN thread\n");
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(FLEXRAN_AGENT, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;

    default:
      LOG_E(FLEXRAN_AGENT, "Received unexpected message %s\n", ITTI_MSG_NAME (msg_p));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);

  return NULL;
}

void *receive_thread(void *args) {

  flexran_agent_info_t  *d = args;
  void                  *data;
  int                   size;
  int                   priority;
  err_code_t             err_code=0;

  Protocol__FlexranMessage *msg;
  pthread_setname_np(pthread_self(), "flexran_rx_thr");
  
  while (1) {

    while ((size = flexran_agent_msg_recv(d->mod_id, FLEXRAN_AGENT_DEFAULT, &data, &priority)) > 0) {
      
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
  if (err_code != 0)
     LOG_E(FLEXRAN_AGENT,"receive_thread: error %d occured\n",err_code);
  return NULL;
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

  if (!channel_info) {
    LOG_E(FLEXRAN_AGENT, "could not create channel_info\n");
    exit(1);
  }

  /*Create a channel using the async channel info*/
  channel_id = flexran_agent_create_channel((void *) channel_info, 
					flexran_agent_async_msg_send, 
					flexran_agent_async_msg_recv,
					flexran_agent_async_release);

  
  if (channel_id <= 0) {
    LOG_E(FLEXRAN_AGENT, "could not create channel\n");
    goto error;
  }

  flexran_agent_channel_t *channel = get_channel(channel_id);
  
  if (channel == NULL) {
    LOG_E(FLEXRAN_AGENT, "could not get channel for channel_id %d\n", channel_id);
    goto error;
  }

  /*Register the channel for all underlying agents (use FLEXRAN_AGENT_MAX)*/
  flexran_agent_register_channel(mod_id, channel, FLEXRAN_AGENT_MAX);

  /*Example of registration for a specific agent(MAC):
   *flexran_agent_register_channel(mod_id, channel, FLEXRAN_AGENT_MAC);
   */

  pthread_t t; 
  threadCreate(&t, receive_thread, flexran, "flexran", -1, OAI_PRIORITY_RT);

  /* Register and initialize the control modules depending on capabilities.
   * After registering, calling flexran_agent_get_*_xface() tells whether a
   * control module is operational */
  uint32_t caps = flexran_get_capabilities_mask(mod_id);
  LOG_I(FLEXRAN_AGENT, "Agent handles BS ID %ld, capabilities=0x%x => handling%s%s%s%s%s%s%s%s%s\n",
        flexran_get_bs_id(mod_id), caps,
        FLEXRAN_CAP_LOPHY(caps) ? " LOPHY" : "",
        FLEXRAN_CAP_HIPHY(caps) ? " HIPHY" : "",
        FLEXRAN_CAP_LOMAC(caps) ? " LOMAC" : "",
        FLEXRAN_CAP_HIMAC(caps) ? " HIMAC" : "",
        FLEXRAN_CAP_RLC(caps)   ? " RLC"   : "",
        FLEXRAN_CAP_PDCP(caps)  ? " PDCP"  : "",
        FLEXRAN_CAP_SDAP(caps)  ? " SDAP"  : "",
        FLEXRAN_CAP_RRC(caps)   ? " RRC"   : "",
        FLEXRAN_CAP_S1AP(caps)  ? " S1AP"  : "");

  if (FLEXRAN_CAP_LOPHY(caps) || FLEXRAN_CAP_HIPHY(caps)) {
    flexran_agent_register_phy_xface(mod_id);
    LOG_I(FLEXRAN_AGENT, "registered PHY interface/CM for eNB %d\n", mod_id);
  }

  if (FLEXRAN_CAP_LOMAC(caps) || FLEXRAN_CAP_HIMAC(caps)) {
    flexran_agent_register_mac_xface(mod_id);
    flexran_agent_init_mac_agent(mod_id);
    LOG_I(FLEXRAN_AGENT, "registered MAC interface/CM for eNB %d\n", mod_id);
  }

  if (FLEXRAN_CAP_RRC(caps)) {
    flexran_agent_register_rrc_xface(mod_id);
    LOG_I(FLEXRAN_AGENT, "registered RRC interface/CM for eNB %d\n", mod_id);
  }

  if (FLEXRAN_CAP_PDCP(caps)) {
    flexran_agent_register_pdcp_xface(mod_id);
    LOG_I(FLEXRAN_AGENT, "registered PDCP interface/CM for eNB %d\n", mod_id);
  }

  if (FLEXRAN_CAP_S1AP(caps)) {
    flexran_agent_register_s1ap_xface(mod_id);
    LOG_I(FLEXRAN_AGENT, "registered S1AP interface/CM for eNB %d\n", mod_id);
  }

  /* 
   * initilize a timer 
   */ 
  
  flexran_agent_timer_init(mod_id);

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

  /* Init the app sublayer */
  flexran_agent_app_init(mod_id);

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
    MessageDef *msg_p = itti_alloc_new_message(TASK_ENB_APP, 0, RRC_CONFIGURATION_REQ);
    RRC_CONFIGURATION_REQ(msg_p) = RC.rrc[mod_id]->configuration;
    itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(mod_id), msg_p);
  }

  return 0;

error:
  LOG_E(FLEXRAN_AGENT, "%s(): there was an error\n", __func__);
  return 1;

}
