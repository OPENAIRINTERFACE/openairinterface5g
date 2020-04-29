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

/*! \file enb_agent.h
 * \brief top level enb agent receive thread and itti task
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */
#define _GNU_SOURCE
#include "proto_agent_common.h"
#include "common/utils/LOG/log.h"
#include "proto_agent.h"
#include "assertions.h"
#include "proto_agent_net_comm.h"
#include "proto_agent_async.h"

#include <pthread.h>

#define  ENB_AGENT_MAX 9

proto_agent_instance_t proto_agent[MAX_DU];

pthread_t new_thread(void *(*f)(void *), void *b);

Protocol__FlexsplitMessage *proto_agent_timeout_fsp(void *args);

#define TEST_MOD 0

#define ECHO

/*  Server side function; upon a new connection
    reception, sends the hello packets
*/
int proto_agent_start(mod_id_t mod_id, const cudu_params_t *p) {
  int channel_id;
  // RS: CUDU does not work!
  //DevAssert(p->local_interface);
  //DevAssert(p->local_ipv4_address);
  //DevAssert(p->local_port > 1024); // "unprivileged" port
  //DevAssert(p->remote_ipv4_address);
  //DevAssert(p->remote_port > 1024); // "unprivileged" port
  proto_agent[mod_id].mod_id = mod_id;
  proto_agent[mod_id].exit = 0;
  /* Initialize the channel container */
  /* TODO only initialize the first time */
  proto_agent_init_channel_container();
  /*Create the async channel info*/
  proto_agent_async_channel_t *channel_info;
  channel_info = proto_agent_async_channel_info(mod_id, p->local_ipv4_address, p->local_port,
                 p->remote_ipv4_address, p->remote_port);

  if (!channel_info) goto error;

  /* Create a channel using the async channel info */
  channel_id = proto_agent_create_channel((void *) channel_info,
                                          proto_agent_async_msg_send,
                                          proto_agent_async_msg_recv,
                                          proto_agent_async_release);

  if (channel_id <= 0) goto error;

  proto_agent_channel_t *channel = proto_agent_get_channel(channel_id);

  if (!channel) goto error;

  proto_agent[mod_id].channel = channel;
  /* Register the channel for all underlying agents (use ENB_AGENT_MAX) */
  proto_agent_register_channel(mod_id, channel, ENB_AGENT_MAX);
  // Code for sending the HELLO/ECHO_REQ message once a connection is established
  //uint8_t *msg = NULL;
  //Protocol__FlexsplitMessage *init_msg=NULL;
  //if (udp == 0)
  //{
  //  // If the comm is not UDP, allow the server to send the first packet over the channel
  //  //printf( "Proto agent Server: Calling the echo_request packet constructor\n");
  //  msg_flag = proto_agent_echo_request(mod_id, NULL, &init_msg);
  //  if (msg_flag != 0)
  //  goto error;
  //
  //  int msgsize = 0;
  //  if (init_msg != NULL)
  //    msg = proto_agent_pack_message(init_msg, &msgsize);
  //  if (msg!= NULL)
  //  {
  //    LOG_D(PROTO_AGENT, "Server sending the message over the async channel\n");
  //    proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, (void *) channel_info);
  //  }
  //  /* After sending the message, wait for any replies;
  //    the server thread blocks until it reads any data
  //    over the channel
  //  */
  //}
  proto_agent[mod_id].recv_thread = new_thread(proto_agent_receive, &proto_agent[mod_id]);
  fprintf(stderr, "[PROTO_AGENT] server started at port %s:%d\n", p->local_ipv4_address, p->local_port);
  return 0;
error:
  LOG_E(PROTO_AGENT, "there was an error\n");
  return 1;
}

void proto_agent_stop(mod_id_t mod_id) {
  if (!proto_agent[mod_id].channel) return;

  /* unlock the independent read thread proto_agent_receive() */
  proto_agent[mod_id].exit = 1;
  proto_agent_async_msg_recv_unlock(proto_agent[mod_id].channel->channel_info);
  proto_agent_async_release(proto_agent[mod_id].channel);
  proto_agent_destroy_channel(proto_agent[mod_id].channel->channel_id);
  free(proto_agent[mod_id].channel);
  proto_agent[mod_id].channel = NULL;
  LOG_W(PROTO_AGENT, "server stopped\n");
}

//void
//proto_agent_send_hello(void)
//{
//  uint8_t *msg = NULL;
//  Protocol__FlexsplitMessage *init_msg=NULL;
//  int msg_flag = 0;
//
//
//  //printf( "PDCP agent: Calling the HELLO packet constructor\n");
//  msg_flag = proto_agent_hello(proto_agent[TEST_MOD].mod_id, NULL, &init_msg);
//
//  int msgsize = 0;
//  if (msg_flag == 0)
//  {
//    proto_agent_serialize_message(init_msg, &msg, &msgsize);
//  }
//
//  LOG_D(PROTO_AGENT, "Agent sending the message over the async channel\n");
//  proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, (void *) client_channel[TEST_MOD]);
//}



rlc_op_status_t  proto_agent_send_rlc_data_req(const protocol_ctxt_t *const ctxt_pP,
    const srb_flag_t srb_flagP, const MBMS_flag_t MBMS_flagP,
    const rb_id_t rb_idP, const mui_t muiP,
    confirm_t confirmP, sdu_size_t sdu_sizeP, mem_block_t *sdu_pP) {
  uint8_t *msg = NULL;
  Protocol__FlexsplitMessage *init_msg=NULL;
  int msg_flag = 0;
  int msgsize = 0;
  mod_id_t mod_id = ctxt_pP->module_id;
  data_req_args args;
  DevAssert(proto_agent[mod_id].channel);
  DevAssert(proto_agent[mod_id].channel->channel_info);
  args.ctxt = ctxt_pP;
  args.srb_flag = srb_flagP;
  args.MBMS_flag = MBMS_flagP;
  args.rb_id = rb_idP;
  args.mui = muiP;
  args.confirm = confirmP;
  args.sdu_size = sdu_sizeP;
  args.sdu_p = sdu_pP;
  msg_flag = proto_agent_pdcp_data_req(mod_id, (void *) &args, &init_msg);

  if (msg_flag != 0 || !init_msg) goto error;

  msg = proto_agent_pack_message(init_msg, &msgsize);

  if (!msg) goto error;

  proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, proto_agent[mod_id].channel->channel_info);
  free_mem_block(sdu_pP, __func__);
  return RLC_OP_STATUS_OK;
error:
  LOG_E(PROTO_AGENT, "PROTO_AGENT there was an error\n");
  return RLC_OP_STATUS_INTERNAL_ERROR;
}


boolean_t
proto_agent_send_pdcp_data_ind(const protocol_ctxt_t *const ctxt_pP, const srb_flag_t srb_flagP,
                               const MBMS_flag_t MBMS_flagP, const rb_id_t rb_idP, sdu_size_t sdu_sizeP, mem_block_t *sdu_pP) {
  uint8_t *msg = NULL;
  Protocol__FlexsplitMessage *init_msg = NULL;
  int msg_flag = 0;
  int msgsize = 0;
  mod_id_t mod_id = ctxt_pP->module_id;
  data_req_args args;
  DevAssert(proto_agent[mod_id].channel);
  DevAssert(proto_agent[mod_id].channel->channel_info);
  args.ctxt = ctxt_pP;
  args.srb_flag = srb_flagP;
  args.MBMS_flag = MBMS_flagP;
  args.rb_id = rb_idP;
  args.sdu_size = sdu_sizeP;
  args.sdu_p = sdu_pP;
  msg_flag = proto_agent_pdcp_data_ind(mod_id, (void *) &args, &init_msg);

  if (msg_flag != 0 || !init_msg) goto error;

  msg = proto_agent_pack_message(init_msg, &msgsize);

  if (!msg) goto error;

  proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, proto_agent[mod_id].channel->channel_info);
  free_mem_block(sdu_pP, __func__);
  return TRUE;
error:
  LOG_E(PROTO_AGENT, "there was an error in %s\n", __func__);
  return FALSE;
}

void *
proto_agent_receive(void *args) {
  proto_agent_instance_t *inst = args;
  void                  *data = NULL;
  int                   size;
  int                   priority;
  err_code_t             err_code;
  pthread_setname_np(pthread_self(), "proto_rx");
  Protocol__FlexsplitMessage *msg;
  uint8_t *ser_msg;

  while (1) {
    msg = NULL;
    ser_msg = NULL;

    if ((size = proto_agent_async_msg_recv(&data, &priority, inst->channel->channel_info)) < 0) {
      err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
      goto error;
    }

    if (inst->exit) break;

    LOG_D(PROTO_AGENT, "Server side Received message with size %d and priority %d, calling message handle\n", size, priority);
    msg = proto_agent_handle_message(inst->mod_id, data, size);

    if (!msg) {
      LOG_D(PROTO_AGENT, "msg to send back is NULL\n");
      continue;
    }

    ser_msg = proto_agent_pack_message(msg, &size);

    if (!ser_msg) {
      continue;
    }

    LOG_D(PROTO_AGENT, "Server sending the reply message over the async channel\n");

    if (proto_agent_async_msg_send((void *)ser_msg, (int) size, 1, inst->channel->channel_info)) {
      err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(PROTO_AGENT, "sent message with size %d\n", size);
  }

  return NULL;
error:
  LOG_E(PROTO_AGENT, "proto_agent_receive(): error %d occured\n",err_code);
  return NULL;
}
