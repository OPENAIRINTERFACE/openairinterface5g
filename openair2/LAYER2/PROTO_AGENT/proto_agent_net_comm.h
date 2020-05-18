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

/*! \file enb_agent_net_comm.h
 * \brief enb agent network interface abstraction 
 * \autho Xenofon Foukas
 * \date 2016
 * \version 0.1
 */
#ifndef PROTO_AGENT_NET_COMM_H_
#define PROTO_AGENT_NET_COMM_H_

#include "proto_agent_defs.h"

#include "tree.h"
#define ENB_AGENT_MAX 9

/* forward declaration */
struct proto_agent_async_channel_s;

/*Channel related information used for Tx/Rx of protocol messages*/
typedef struct proto_agent_channel_s {
  RB_ENTRY(proto_agent_channel_s) entry;
  int channel_id;
  struct proto_agent_async_channel_s *channel_info;
  /*Callbacks for channel message Tx and Rx*/
  int (*msg_send)(void *data, int size, int priority, void *channel_info);
  int (*msg_recv)(void **data, int *priority, void *channel_info);
  void (*release)(struct proto_agent_channel_s *channel);
} proto_agent_channel_t;

typedef struct proto_agent_channel_instance_s{
  RB_HEAD(proto_agent_channel_map, proto_agent_channel_s) proto_agent_head;
} proto_agent_channel_instance_t;


/*Register a channel to an agent. Use ENB_AGENT_MAX to register the
 *same channel to all agents*/
int proto_agent_register_channel(mod_id_t mod_id, proto_agent_channel_t *channel, proto_agent_id_t agent_id);

/*Unregister the current channel of an agent. Use ENB_AGENT_MAX to unregister all channels*/
void proto_agent_unregister_channel(mod_id_t mod_id, proto_agent_id_t agent_id);

/*Create a new channel. Returns the id of the new channel or negative number otherwise*/
int proto_agent_create_channel(void *channel_info,
			       int (*msg_send)(void *data, int size, int priority, void *channel_info),
			       int (*msg_recv)(void **data, int *priority, void *channel_info),
			     void (*release)(proto_agent_channel_t *channel));

/*Unregister a channel from all agents and destroy it. Returns 0 in case of success*/
int proto_agent_destroy_channel(int channel_id);

/*Return an agent communication channel based on its id*/
proto_agent_channel_t * proto_agent_get_channel(int channel_id);

/*Should be called before performing any channel operations*/
err_code_t proto_agent_init_channel_container(void);

int proto_agent_compare_channel(struct proto_agent_channel_s *a, struct proto_agent_channel_s *b);

/* RB_PROTOTYPE is for .h files */
RB_PROTOTYPE(proto_agent_channel_map, proto_agent_channel_s, entry, proto_agent_compare_channel);

#endif /*ENB_AGENT_COMM_H_*/
