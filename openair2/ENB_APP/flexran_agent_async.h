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

/*! \file flexran_agent_async.h
 * \brief channel implementation for async interface
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#ifndef FLEXRAN_AGENT_ASYNC_H_
#define FLEXRAN_AGENT_ASYNC_H_

#include "flexran_agent_net_comm.h"

typedef struct {
  mid_t            enb_id;
  socket_link_t   *link;
  message_queue_t *send_queue;
  message_queue_t *receive_queue;
  link_manager_t  *manager;
} flexran_agent_async_channel_t;

/* Create a new channel for a given destination ip and destination port */
flexran_agent_async_channel_t * flexran_agent_async_channel_info(mid_t mod_id, char *dst_ip, uint16_t dst_port);

/* Send a message to the given channel */
int flexran_agent_async_msg_send(void *data, int size, int priority, void *channel_info);

/* Receive a message from a given channel */
int flexran_agent_async_msg_recv(void **data, int *priority, void *channel_info);

/* Release a channel */
void flexran_agent_async_release(flexran_agent_channel_t *channel);


#endif /*FLEXRAN_AGENT_ASYNC_H_*/
