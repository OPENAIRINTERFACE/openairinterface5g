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

#include "proto_agent_async.h"
#include "proto_agent_defs.h"


#include "common/utils/LOG/log.h"

proto_agent_async_channel_t *
proto_agent_async_channel_info(mod_id_t mod_id, const char *bind_ip, uint16_t bind_port,
                               const char* peer_ip, uint16_t peer_port)
{
  proto_agent_async_channel_t *channel;
  channel = malloc(sizeof(proto_agent_async_channel_t));
  
  if (channel == NULL)
    goto error;

  channel->enb_id = mod_id;
  channel->link = new_link_udp_server(bind_ip, bind_port);
  
  if (channel->link == NULL) goto error;
  
  channel->send_queue = new_message_queue();
  if (channel->send_queue == NULL) goto error;
  channel->receive_queue = new_message_queue();
  if (channel->receive_queue == NULL) goto error;

  channel->manager = create_link_manager(channel->send_queue,
                                         channel->receive_queue,
                                         channel->link);
  /* manually set remote IP&port for UDP server remote end */
  channel->manager->peer_port = peer_port;
  channel->manager->peer_addr = peer_ip;

  if (channel->manager == NULL) goto error;
  
  return channel;

 error:
  if (channel)
    free(channel);
  LOG_E(PROTO_AGENT, "error creating proto_agent_async_channel_t\n");
  return NULL;
}

int proto_agent_async_msg_send(void *data, int size, int priority, void *channel_info)
{
  proto_agent_async_channel_t *channel = channel_info;
  return message_put(channel->send_queue, data, size, priority);
}

int proto_agent_async_msg_recv(void **data, int *priority, void *channel_info)
{
  proto_agent_async_channel_t *channel = channel_info;
  return message_get(channel->receive_queue, data, priority);
}

void proto_agent_async_msg_recv_unlock(proto_agent_async_channel_t *channel) {
  message_get_unlock(channel->receive_queue);
}

void proto_agent_async_release(proto_agent_channel_t *channel)
{
  proto_agent_async_channel_t *channel_info = channel->channel_info;

  destroy_link_manager(channel_info->manager);
  
  destroy_message_queue(channel_info->send_queue);
  destroy_message_queue(channel_info->receive_queue);
  
  close_link(channel_info->link);
  free(channel_info);
}
