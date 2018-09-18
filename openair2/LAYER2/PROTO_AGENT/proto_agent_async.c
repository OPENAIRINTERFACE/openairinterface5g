/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/


#include "proto_agent_async.h"
#include "proto_agent_defs.h"


#include "common/utils/LOG/log.h"

proto_agent_async_channel_t *
proto_server_async_channel_info(mod_id_t mod_id, const char *ip, uint16_t port)
{
  LOG_E(PROTO_AGENT, "does not bind to specific address at the moment, ignoring %s\n", ip);
  proto_agent_async_channel_t *channel;
  channel = malloc(sizeof(proto_agent_channel_t));
  
  if (channel == NULL)
    goto error;

  channel->port = port;
  channel->peer_addr = NULL;

  channel->enb_id = mod_id;
  channel->link = new_link_udp_server(port);
  
  if (channel->link == NULL) goto error;
  
  channel->send_queue = new_message_queue();
  if (channel->send_queue == NULL) goto error;
  channel->receive_queue = new_message_queue();
  if (channel->receive_queue == NULL) goto error;

  channel->manager = create_link_manager(channel->send_queue,
                                         channel->receive_queue,
                                         channel->link,
                                         CHANNEL_UDP,
                                         channel->peer_addr,
                                         channel->port);
  if (channel->manager == NULL) goto error;
  
  return channel;

 error:
  LOG_E(PROTO_AGENT,"there was an error\n");
  return NULL;
}


proto_agent_async_channel_t *
proto_agent_async_channel_info(mod_id_t mod_id, const char *dst_ip, uint16_t dst_port)
{
  proto_agent_async_channel_t *channel;
  channel = (proto_agent_async_channel_t *) malloc(sizeof(proto_agent_channel_t));

  if (channel == NULL)
    goto error;

  channel->port = dst_port;
  channel->peer_addr = dst_ip;

  channel->enb_id = mod_id;
  channel->link = new_link_udp_client(channel->peer_addr, channel->port);
 
  if (channel->link == NULL) goto error;
  
  channel->send_queue = new_message_queue();
  if (channel->send_queue == NULL) goto error;
  channel->receive_queue = new_message_queue();
  if (channel->receive_queue == NULL) goto error;
  
  channel->manager = create_link_manager(channel->send_queue,
                                         channel->receive_queue,
                                         channel->link,
                                         CHANNEL_UDP,
                                         channel->peer_addr,
                                         channel->port);
  if (channel->manager == NULL) goto error;
  
  return channel;

 error:
  LOG_E(PROTO_AGENT,"there was an error\n");
  return NULL;
}

int proto_agent_async_msg_send(void *data, int size, int priority, void *channel_info) {
  proto_agent_async_channel_t *channel = channel_info;

  return message_put(channel->send_queue, data, size, priority);
}

int proto_agent_async_msg_recv(void **data, int *size, int *priority, void *channel_info)
{
  proto_agent_async_channel_t *channel;
  channel = (proto_agent_async_channel_t *)channel_info;

  return message_get(channel->receive_queue, data, size, priority);
}

void proto_agent_async_release(proto_agent_channel_t *channel) {
  proto_agent_async_channel_t *channel_info;
  channel_info = (proto_agent_async_channel_t *) channel->channel_info;

  destroy_link_manager(channel_info->manager);
  
  destroy_message_queue(channel_info->send_queue);
  destroy_message_queue(channel_info->receive_queue);
  
  close_link(channel_info->link);
  free(channel_info);
}
