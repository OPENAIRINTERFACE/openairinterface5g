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

/*! \file flexran_agent_net_comm.c
 * \brief FlexRAN agent network interface abstraction 
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#include "flexran_agent_net_comm.h"
#include "log.h"

flexran_agent_channel_t *agent_channel[NUM_MAX_ENB][FLEXRAN_AGENT_MAX];
flexran_agent_channel_instance_t channel_instance;
int flexran_agent_channel_id = 0;

int flexran_agent_msg_send(mid_t mod_id, agent_id_t agent_id, void *data, int size, int priority) {
  /*Check if agent id is valid*/
  if (agent_id >= FLEXRAN_AGENT_MAX || agent_id < 0) {
    goto error;
  }
  flexran_agent_channel_t *channel;
  channel = agent_channel[mod_id][agent_id];
  
  /*Check if agent has a channel registered*/
  if (channel == NULL) {
    goto error;
  }

  return channel->msg_send(data, size, priority, channel->channel_info);
  
 error:
  LOG_E(FLEXRAN_AGENT, "No channel registered for agent with id %d\n", agent_id);
  return -1;
}

int flexran_agent_msg_recv(mid_t mod_id, agent_id_t agent_id, void **data, int *size, int *priority) {
  /*Check if agent id is valid*/
  if (agent_id >= FLEXRAN_AGENT_MAX || agent_id < 0) {
    goto error;
  }
  flexran_agent_channel_t *channel;
  channel = agent_channel[mod_id][agent_id];
  
  /*Check if agent has a channel registered*/
  if (channel == NULL) {
    goto error;
  }
  
  return channel->msg_recv(data, size, priority, channel->channel_info);
  
 error:
  LOG_E(FLEXRAN_AGENT, "No channel registered for agent with id %d\n", agent_id);
  return -1;
}

int flexran_agent_register_channel(mid_t mod_id, flexran_agent_channel_t *channel, agent_id_t agent_id) {
  int i;

  if (channel == NULL) {
    return -1;
  }

  if (agent_id == FLEXRAN_AGENT_MAX) {
    for (i = 0; i < FLEXRAN_AGENT_MAX; i++) {
      agent_channel[mod_id][i] = channel;
    }
  } else {
    agent_channel[mod_id][agent_id] = channel;
  }
  return 0;
}

void flexran_agent_unregister_channel(mid_t mod_id, agent_id_t agent_id) {
  int i;

  if (agent_id == FLEXRAN_AGENT_MAX) {
    for (i = 0; i < FLEXRAN_AGENT_MAX; i++) {
      agent_channel[mod_id][i] = NULL;
    }
  } else {
    agent_channel[mod_id][agent_id] = NULL;
  }
}

int flexran_agent_create_channel(void *channel_info,
				 int (*msg_send)(void *data, int size, int priority, void *channel_info),
				 int (*msg_recv)(void **data, int *size, int *priority, void *channel_info),
				 void (*release)(flexran_agent_channel_t *channel)) {
  
  int channel_id = ++flexran_agent_channel_id;
  flexran_agent_channel_t *channel = (flexran_agent_channel_t *) malloc(sizeof(flexran_agent_channel_t));
  channel->channel_id = channel_id;
  channel->channel_info = channel_info;
  channel->msg_send = msg_send;
  channel->msg_recv = msg_recv;
  channel->release = release;
  
  /*element should be a real pointer*/
  RB_INSERT(flexran_agent_channel_map, &channel_instance.flexran_agent_head, channel); 
  
  LOG_I(FLEXRAN_AGENT,"Created a new channel with id %d \n", channel->channel_id);
 
  return channel_id; 
}

int flexran_agent_destroy_channel(int channel_id) {
  int i, j;

  /*Check to see if channel exists*/
  struct flexran_agent_channel_s *e = NULL;
  struct flexran_agent_channel_s search;
  memset(&search, 0, sizeof(struct flexran_agent_channel_s));

  e = RB_FIND(flexran_agent_channel_map, &channel_instance.flexran_agent_head, &search);

  if (e == NULL) {
    return -1;
  }

  /*Unregister the channel from all agents*/
  for (i = 0; i < NUM_MAX_ENB; i++) {
    for (j = 0; j < FLEXRAN_AGENT_MAX; j++) {
      if (agent_channel[i][j] != NULL) {
        if (agent_channel[i][j]->channel_id == e->channel_id) {
            free(agent_channel[i][j]);
        }
      }
    }
  }

  /*Remove the channel from the tree and free memory*/
  RB_REMOVE(flexran_agent_channel_map, &channel_instance.flexran_agent_head, e);
  e->release(e);
  free(e);

  return 0;
}

err_code_t flexran_agent_init_channel_container(void) {
  int i, j;
  LOG_I(FLEXRAN_AGENT, "init RB tree for channel container\n");

  RB_INIT(&channel_instance.flexran_agent_head);
  
  for (i = 0; i < NUM_MAX_ENB; i++) {
    for (j = 0; j < FLEXRAN_AGENT_MAX; j++) {
      agent_channel[i][j] = malloc(sizeof(flexran_agent_channel_t));
      if (!agent_channel[i][j])
        return -1;
    }
  }

  return 0;
}

RB_GENERATE(flexran_agent_channel_map, flexran_agent_channel_s, entry, flexran_agent_compare_channel);

int flexran_agent_compare_channel(struct flexran_agent_channel_s *a, struct flexran_agent_channel_s *b) {
  if (a->channel_id < b->channel_id) return -1;
  if (a->channel_id > b->channel_id) return 1;

  // equal timers
  return 0;
}

flexran_agent_channel_t * get_channel(int channel_id) {
  
  struct flexran_agent_channel_s search;
  memset(&search, 0, sizeof(struct flexran_agent_channel_s));
  search.channel_id = channel_id;
  
  return  RB_FIND(flexran_agent_channel_map, &channel_instance.flexran_agent_head, &search);
  
}
