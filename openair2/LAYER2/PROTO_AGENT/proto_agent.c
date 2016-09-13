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

/*! \file enb_agent.h
 * \brief top level enb agent receive thread and itti task
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#include "proto_agent_common.h"
#include "log.h"
#include "proto_agent.h"
#include "assertions.h"
#include "proto_agent_net_comm.h"
#include "proto_agent_async.h" 


proto_agent_instance_t proto_agent[NUM_MAX_ENB];
proto_agent_instance_t proto_server[NUM_MAX_ENB];

char in_ip[40];
static uint16_t in_port;
char local_cache[40];

void *send_thread(void *args);
void *receive_thread(void *args);
pthread_t new_thread(void *(*f)(void *), void *b);
Protocol__FlexsplitMessage *proto_agent_timeout_fsp(void* args);

mid_t client_mod, server_mod;
proto_agent_instance_t *client_channel, *server_channel;

#define ECHO

/* Thread continuously listening for incomming packets */

void *receive_thread(void *args) {

  proto_agent_instance_t         *d = args;
  void                  *data;
  int                   size;
  int                   priority;
  err_code_t             err_code;

  Protocol__FlexsplitMessage *msg;

  while (1) {
    if (proto_agent_msg_recv(d->enb_id, PROTO_AGENT_DEFAULT, &data, &size, &priority)) {
      err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(PROTO_AGENT,"Received message with size %d and priority %d, calling message handle\n", size, priority);

    msg=proto_agent_handle_message(d->enb_id, data, size);

    if (msg == NULL)
    {
        LOG_E(PROTO_AGENT,"msg to send back is NULL\n");
    }

    free(data);

    if (msg != NULL){
      if (proto_agent_msg_send(d->enb_id, PROTO_AGENT_DEFAULT, msg, size, priority)) {
	err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
	goto error;
      }
      LOG_D(PROTO_AGENT,"sent message with size %d\n", size);
    }
  }

  return NULL;

error:
  LOG_E(PROTO_AGENT,"receive_thread: error %d occured\n",err_code);
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

/* Function to be called as a thread: 
   it is calling the proto_agent_server with 
   the appropriate arguments 
*/

void * proto_server_init(void *args)
{
   LOG_D(PROTO_AGENT, "Initializing server thread for listening connections\n");
   mid_t mod_id = (mid_t) 1;
   Enb_properties_array_t* enb_properties = NULL;
   enb_properties = enb_config_get();
   proto_server_start(mod_id, (const Enb_properties_array_t*) enb_properties);
   return NULL;
}

/*  Server side function; upon a new connection 
    reception, sends the hello packets
*/
int proto_server_start(mid_t mod_id, const Enb_properties_array_t* enb_properties){
  
  int channel_id;

  //  Maybe change the RAN_LTE_OAI to protocol?? (e.g. PDCP)
  set_enb_vars(mod_id, RAN_LTE_OAI);
  proto_server[mod_id].enb_id = mod_id;
  
  server_mod = mod_id;
  /* 
   * check the configuration - Getting all the values from the config file
   * TODO: get the configuration optionally from the conf file
   */ 
  strcpy(local_cache, DEFAULT_PROTO_AGENT_CACHE);
  strcpy(in_ip, DEFAULT_PROTO_AGENT_IPv4_ADDRESS ); 
  in_port = DEFAULT_PROTO_AGENT_PORT ;

  LOG_I(PROTO_AGENT,"Starting PROTO agent SERVER for module id %d on ipv4 %s, port %d\n",
	proto_server[mod_id].enb_id,
	in_ip,
	in_port);

  /*
   * Initialize the channel container
   */
  proto_agent_init_channel_container();

  /*Create the async channel info*/
  proto_agent_instance_t *channel_info = proto_server_async_channel_info(mod_id, in_ip, in_port);
  
  server_channel = channel_info;

  /*Create a channel using the async channel info*/
  channel_id = proto_agent_create_channel((void *) channel_info,
					proto_agent_async_msg_send,
					proto_agent_async_msg_recv,
					proto_agent_async_release);

  if (channel_id <= 0) {
    goto error;
  }

  proto_agent_channel_t *channel = get_channel(channel_id);
  
  if (channel == NULL) {
    goto error;
  }

  /*Register the channel for all underlying agents (use ENB_AGENT_MAX)*/
  proto_agent_register_channel(mod_id, channel, ENB_AGENT_MAX);

  // Code for sending the HELLO/ECHO_REQ message once a connection is established
  Protocol__FlexsplitMessage *msg = NULL;
  Protocol__FlexsplitMessage *init_msg=NULL;
  Protocol__FlexsplitMessage *rep_msg=NULL;
  Protocol__FlexsplitMessage *ser_msg=NULL;
  int msg_flag = 0;
  int priority;
  int size;

#ifdef ECHO
  LOG_I(PROTO_AGENT, "Proto agent Server: Calling the echo_request packet constructor\n");
  msg_flag = proto_agent_echo_request(mod_id, NULL, &init_msg);
#else
  LOG_D(PROTO_AGENT, "Proto agent Server: Calling the hello packet constructor\n");
  msg_flag = proto_agent_hello(mod_id, NULL, &init_msg);
#endif

  int msgsize = 0;
  int err_code;
  msg = proto_agent_pack_message(init_msg, &msgsize);
  //proto_agent_serialize_message(init_msg, &msg, &msgsize);

  LOG_I(PROTO_AGENT,"Server sending the message over the async channel\n");
  proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, (void *) channel_info);

  /* After sending the message, wait for any replies; 
     the server thread blocks until it reads any data 
     over the channel
  */

  LOG_I(PROTO_AGENT, "Server reading any message over the async channel.\n");
  
  while (1) {
    LOG_I(PROTO_AGENT, "Server reading any message over the async channel.\n");
    if (proto_agent_async_msg_recv(&rep_msg, &size, &priority, channel_info)){
      //(mod_id, PROTO_AGENT_DEFAULT, &rep_msg, &size, &priority)) {
      err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
      goto error;
    }
 
    LOG_I(PROTO_AGENT,"Server received reply message with size %d and priority %d, calling message handler\n", size, priority);
  
    msg=proto_agent_handle_message(mod_id, rep_msg, size);
    
    if (msg == NULL)
    {
        LOG_I(PROTO_AGENT,"Server msg to send back is NULL\n");
    }
    else
    { //(msg != NULL){
      ser_msg = proto_agent_pack_message(msg, &size);
      //proto_agent_serialize_message(msg, &ser_msg, &size);      

//      if (proto_agent_msg_send(mod_id, PROTO_AGENT_DEFAULT, msg, size, priority)) {
      if (proto_agent_async_msg_send((void *)ser_msg, (int) size, 1, (void *) channel_info)) {

	err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
	goto error;
      }
      LOG_I(PROTO_AGENT,"sent message with size %d\n", size);
    }
  }
    LOG_I(PROTO_AGENT,"server ends\n");
  return 0;

error:
  LOG_I(PROTO_AGENT,"there was an error\n");
  return 1;

}

int proto_agent_start(mid_t mod_id, const Enb_properties_array_t* enb_properties){
  
  int channel_id;
  
  set_enb_vars(mod_id, RAN_LTE_OAI);
  proto_agent[mod_id].enb_id = mod_id;
  client_mod = mod_id;
  
  /* 
   * check the configuration - Getting all the values from the config file
   */ 

  strcpy(local_cache, DEFAULT_PROTO_AGENT_CACHE);
  strcpy(in_ip, DEFAULT_PROTO_AGENT_IPv4_ADDRESS );
  in_port = DEFAULT_PROTO_AGENT_PORT;

  LOG_I(PROTO_AGENT,"starting PROTO agent client for module id %d on ipv4 %s, port %d\n",  
	proto_agent[mod_id].enb_id,
	in_ip,
	in_port);

  /*
   * Initialize the channel container
   */
  proto_agent_init_channel_container();

  /*Create the async channel info*/
  proto_agent_instance_t *channel_info = proto_agent_async_channel_info(mod_id, in_ip, in_port);
  client_channel = channel_info;
  /*Create a channel using the async channel info*/
  channel_id = proto_agent_create_channel((void *) channel_info, 
					proto_agent_async_msg_send, 
					proto_agent_async_msg_recv,
					proto_agent_async_release);
  if (channel_id <= 0) {
    goto error;
  }

  proto_agent_channel_t *channel = get_channel(channel_id);

  if (channel == NULL) {
    goto error;
  }

  /*Register the channel for all underlying agents (use ENB_AGENT_MAX)*/
  proto_agent_register_channel(mod_id, channel, ENB_AGENT_MAX);

  /*Example of registration for a specific agent(MAC):
   *enb_agent_register_channel(mod_id, channel, ENB_AGENT_MAC);
   */

  // Do not call a new thread, but do the msg exchange without threads
  //new_thread(receive_thread, &proto_agent[mod_id]);
  
  void                  *data;
  int                   size;
  int                   priority;
  err_code_t             err_code;
  Protocol__FlexsplitMessage *msg;
  Protocol__FlexsplitMessage *ser_msg;
  
  if (proto_agent_async_msg_recv(&data, &size, &priority, channel_info)) {
      err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
      goto error;
  }

  LOG_I(PROTO_AGENT,"Received message with size %d and priority %d, calling message handle\n", size, priority);

  msg=proto_agent_handle_message(proto_agent[mod_id].enb_id, data, size);

  if (msg == NULL)
  {
        LOG_I(PROTO_AGENT," CLIENT msg to send back is NULL\n");
  }

    //free(data);

    if (msg != NULL){
      ser_msg = proto_agent_pack_message(msg, &size);
      //proto_agent_serialize_message(msg, &ser_msg, &size);
      if(  proto_agent_async_msg_send((void *)ser_msg, (int) size, 1, (void *) channel_info)){
	  err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
	goto error;
      }
      LOG_I(PROTO_AGENT,"CLIENT sent message with size %d\n", size);
    }
  
 
  //LOG_D(PROTO_AGENT, "Client launched the the receive thread for mod_id %d\n", proto_agent[mod_id]);

  return 0;

error:
  LOG_E(PROTO_AGENT,"there was an error %u\n", err_code);
  return 1;

}


void 
proto_agent_send_hello(void)
{
  Protocol__FlexsplitMessage *msg = NULL;
  Protocol__FlexsplitMessage *init_msg=NULL;
  //Protocol__FlexsplitMessage *rep_msg=NULL;
  int msg_flag = 0;
  //int priority;
  //int size;
  
  LOG_D(PROTO_AGENT, "PDCP agent Server: Calling the hello packet constructor\n");
  msg_flag = proto_agent_hello(proto_agent[client_mod].enb_id, NULL, &init_msg);
  
  int msgsize = 0;
  //int err_code;
  if (msg_flag > 0)
  {
    proto_agent_serialize_message(init_msg, &msg, &msgsize);
  }
  
  LOG_D(PROTO_AGENT,"Server sending the message over the async channel\n");
  proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, (void *) client_channel);
}


void
proto_agent_send_rlc_data_req(const protocol_ctxt_t* const ctxt_pP, const srb_flag_t srb_flagP, const MBMS_flag_t MBMS_flagP, const rb_id_t rb_idP, const mui_t muiP, 
			      confirm_t confirmP, sdu_size_t sdu_sizeP, mem_block_t *sdu_pP)
{
  
  printf("PROTOPDCP: sending the data req over the async channel\n");
  
  Protocol__FlexsplitMessage *msg = NULL;
  Protocol__FlexsplitMessage *init_msg=NULL;
  Protocol__FlexsplitMessage *rep = NULL;
  Protocol__FlexsplitMessage *srep = NULL;

  //Protocol__FlexsplitMessage *rep_msg=NULL;
  int msg_flag = 0;
  void *data=NULL;
  int priority;
  int size;
  int ret;
  int err_code;
  
  LOG_D(PROTO_AGENT, "PDCP agent Server: Calling the hello packet constructor\n");
 
  //EDW
  printf("instance is %u, %u\n", ctxt_pP->instance, ctxt_pP->frame);
  data_req_args *args = malloc(sizeof(data_req_args));
  
  args->ctxt = malloc(sizeof(protocol_ctxt_t));
  memcpy(args->ctxt, ctxt_pP, sizeof(protocol_ctxt_t));
  args->srb_flag = srb_flagP;
  args->MBMS_flag = MBMS_flagP;
  args->rb_id = rb_idP;
  args->mui = muiP;
  args->confirm = confirmP;
  args->sdu_size = sdu_sizeP;
  args->sdu_p = malloc(sdu_sizeP);
  memcpy(args->sdu_p, sdu_pP->data, sdu_sizeP);

  msg_flag = proto_agent_pdcp_data_req(proto_agent[client_mod].enb_id, (void *) args, &init_msg);
  
  printf("PROTO_AGENT msg flag is %d", msg_flag);
  int msgsize = 0;
  //int err_code;
  msg = proto_agent_pack_message(init_msg, &msgsize);
  
  free(init_msg);
  
  LOG_I(PROTO_AGENT,"Server sending the pdcp data_req message over the async channel\n");
  
  proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, (void *) client_channel);
  
  // Block until you receive the ACK with the op code
 // while (1) {
  /*
    msg = NULL;
    rep = NULL;*/
    //data = NULL;
    msgsize = 0;
    priority = 0;
    msg = NULL;
    
    while (rep == NULL)
    {
     if (proto_agent_async_msg_recv(&rep, &msgsize, &priority, client_channel)) {
       err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
       goto error;
     }
    }
    LOG_D(PROTO_AGENT,"Received message with size %d and priority %d, calling message handle\n", size, priority);

    msg = proto_agent_handle_message(proto_agent[client_mod].enb_id, rep, msgsize);

    if (msg == NULL)
    {
        LOG_E(PROTO_AGENT,"msg to send back is NULL\n");
    }
    else
    {
    srep = proto_agent_pack_message(rep, &size);

    //free(data);

    
      if (proto_agent_async_msg_send((void *)msg, (int) size, 1, (void *) client_channel)){
      //if (proto_agent_msg_send(proto_agent[client_mod].enb_id, PROTO_AGENT_DEFAULT, msg, size, priority)) {
	err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
	goto error;
      }
      LOG_D(PROTO_AGENT,"sent message with size %d\n", size);
    }
    
  //}

error:
  LOG_E(PROTO_AGENT,"there was an error\n");
  return;
  
}

  


  
void *
proto_server_receive(void)
{
  proto_agent_instance_t         *d = &proto_server[server_mod];
  void                  *data;
  int                   size;
  int                   priority;
  err_code_t             err_code;

  Protocol__FlexsplitMessage *msg;

  //while (1) {
    if (proto_agent_msg_recv(d->enb_id, PROTO_AGENT_DEFAULT, &data, &size, &priority)) {
      err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_I(PROTO_AGENT,"Server Received message with size %d and priority %d, calling message handle\n", size, priority);

    msg=proto_agent_handle_message(d->enb_id, data, size);

    if (msg == NULL)
    {
        LOG_E(PROTO_AGENT,"msg to send back is NULL\n");
    }

    //free(data);

    if (msg != NULL){
      if (proto_agent_msg_send(d->enb_id, PROTO_AGENT_DEFAULT, msg, size, priority)) {
	err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
	goto error;
      }
      LOG_D(PROTO_AGENT,"sent message with size %d\n", size);
    }
  //}

  return NULL;

error:
  LOG_E(PROTO_AGENT,"receive_thread: error %d occured\n",err_code);
  return NULL;
  
}