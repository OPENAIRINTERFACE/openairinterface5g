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

/*! \file flexran_agent_async.c
 * \brief channel implementation for async interface
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#include "flexran_agent_async.h"
#include "flexran_agent_defs.h"

#include "common/utils/LOG/log.h"

flexran_agent_async_channel_t * flexran_agent_async_channel_info(mid_t mod_id, char *dst_ip, uint16_t dst_port) {

  flexran_agent_async_channel_t *channel;
  channel = (flexran_agent_async_channel_t *) malloc(sizeof(flexran_agent_channel_t));
  
  if (channel == NULL) {
    LOG_E(FLEXRAN_AGENT, "could not allocate memory for flexran_agent_async_channel_t\n");
    return NULL;
  }

  channel->enb_id = mod_id;
  /*Create a socket*/
  channel->link = new_link_client(dst_ip, dst_port);
  if (channel->link == NULL) {
    LOG_E(FLEXRAN_AGENT, "could not create new link client\n");
    goto error;
  }
  
  LOG_I(FLEXRAN_AGENT,"starting enb agent client for module id %d on ipv4 %s, port %d\n",  
	channel->enb_id,
	dst_ip,
	dst_port);
  
   /* 
   * create a message queue
   */ 
  // Set size of queues statically for now
//  channel->send_queue = new_message_queue(500);
  // not using the circular buffer: affects the PDCP split
  channel->send_queue = new_message_queue();
  if (channel->send_queue == NULL) {
    LOG_E(FLEXRAN_AGENT, "could not create send_queue\n");
    goto error;
  }
  // not using the circular buffer: affects the PDCP split
  //channel->receive_queue = new_message_queue(500);
  channel->receive_queue = new_message_queue();
  if (channel->receive_queue == NULL) {
    LOG_E(FLEXRAN_AGENT, "could not create send_queue\n");
    goto error;
  }
  
  /* create a link manager */
  channel->manager = create_link_manager(channel->send_queue, channel->receive_queue, channel->link);
  if (channel->manager == NULL) {
    LOG_E(FLEXRAN_AGENT, "could not create link_manager\n");
    goto error;
  }
  
  return channel;

 error:
  LOG_I(FLEXRAN_AGENT, "%s(): there was an error\n", __func__);
  return NULL;
}

int flexran_agent_async_msg_send(void *data, int size, int priority, void *channel_info) {
  flexran_agent_async_channel_t *channel;
  channel = (flexran_agent_async_channel_t *)channel_info;

  return message_put(channel->send_queue, data, size, priority);
}

int flexran_agent_async_msg_recv(void **data, int *priority, void *channel_info) {
  flexran_agent_async_channel_t *channel;
  channel = (flexran_agent_async_channel_t *)channel_info;

  return message_get(channel->receive_queue, data, priority);
}

void flexran_agent_async_release(flexran_agent_channel_t *channel) {
  flexran_agent_async_channel_t *channel_info;
  channel_info = (flexran_agent_async_channel_t *) channel->channel_info;

  destroy_link_manager(channel_info->manager);
  
  destroy_message_queue(channel_info->send_queue);
  destroy_message_queue(channel_info->receive_queue);
  
  close_link(channel_info->link);
  free(channel_info);
}
