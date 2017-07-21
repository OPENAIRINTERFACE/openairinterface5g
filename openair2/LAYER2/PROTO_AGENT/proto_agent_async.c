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

/*! \file enb_agent_async.c
 * \brief channel implementation for async interface
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#include "proto_agent_async.h"
#include "proto_agent_defs.h"


#include "log.h"

uint16_t proto_udp = 0;
uint16_t proto_tcp = 0;
uint16_t proto_sctp = 0;

proto_agent_async_channel_t * proto_server_async_channel_info(mid_t mod_id, char *dst_ip, uint16_t dst_port, const char* type, const char *peer_addr) {

  proto_agent_async_channel_t *channel;
  channel = (proto_agent_async_channel_t *) malloc(sizeof(proto_agent_channel_t));

  channel->port = dst_port;
  channel->peer_addr = NULL;
  
  if (channel == NULL)
    goto error;

  channel->enb_id = mod_id;
  /*Create a socket*/
  if (strcmp(type, "TCP") == 0)
  {
    proto_tcp = 1;
    channel->link = new_link_server(dst_port);
    channel->type = 0;
  }
  else if (strcmp(type, "UDP") == 0)
  {
    proto_udp = 1;
    //channel->link = new_udp_link_server(dst_port);
    channel->link = new_link_udp_server(dst_port);
    channel->type = 1;
    channel->peer_addr = peer_addr;
  }
  else if (strcmp(type, "SCTP") == 0)
  {
    proto_sctp = 1;
    //channel->link = new_sctp_link_server(dst_port);
    channel->link = new_link_sctp_server(dst_port);
    channel->type = 2;
  }
  
  if (channel->link == NULL) goto error;
  
   /* 
   * create a message queue
   */ 
  
  channel->send_queue = new_message_queue();
  if (channel->send_queue == NULL) goto error;
  channel->receive_queue = new_message_queue();
  if (channel->receive_queue == NULL) goto error;

   /* 
   * create a link manager 
   */  
  channel->manager = create_link_manager(channel->send_queue, channel->receive_queue, channel->link, channel->type, channel->peer_addr, channel->port);
  if (channel->manager == NULL) goto error;
  
  return channel;

 error:
  LOG_E(PROTO_AGENT,"there was an error\n");
  return 1;
}


proto_agent_async_channel_t * proto_agent_async_channel_info(mid_t mod_id, char *dst_ip, uint16_t dst_port, const char* type, const char *peer_addr) {

  proto_agent_async_channel_t *channel;
  channel = (proto_agent_async_channel_t *) malloc(sizeof(proto_agent_channel_t));
  
  channel->port = dst_port;
  channel->peer_addr = NULL;
  
  if (channel == NULL)
    goto error;

  channel->enb_id = mod_id;
  /*Create a socket*/
  if (strcmp(type, "TCP") == 0)
  {
    proto_tcp = 1;
    channel->link = new_link_client(dst_ip, dst_port);
    channel->type = 0;
  }
  else if (strcmp(type, "UDP") == 0)
  {
    proto_udp = 1;
    channel->link = new_link_udp_client(dst_ip, dst_port);
    channel->type = 1;
    channel->peer_addr = peer_addr;
  }
  else if (strcmp(type, "SCTP") == 0)
  {
    proto_sctp = 1;
    channel->link = new_link_sctp_client(dst_ip, dst_port);;
    channel->type = 2;
  }
  
 
  if (channel->link == NULL) goto error;
  
   /* 
   * create a message queue
   */ 
  
  channel->send_queue = new_message_queue();
  if (channel->send_queue == NULL) goto error;
  channel->receive_queue = new_message_queue();
  if (channel->receive_queue == NULL) goto error;
  
   /* 
   * create a link manager 
   */  
  channel->manager = create_link_manager(channel->send_queue, channel->receive_queue, channel->link, channel->type, channel->peer_addr, channel->port);
  if (channel->manager == NULL) goto error;
  
  return channel;

 error:
  LOG_E(PROTO_AGENT,"there was an error\n");
  return 1;
}

int proto_agent_async_msg_send(void *data, int size, int priority, void *channel_info) {
  proto_agent_async_channel_t *channel;
  channel = (proto_agent_channel_t *)channel_info;

  return message_put(channel->send_queue, data, size, priority);
}

int proto_agent_async_msg_recv(void **data, int *size, int *priority, void *channel_info) {
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
