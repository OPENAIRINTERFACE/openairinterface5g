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


proto_agent_instance_t proto_agent[MAX_DU];
proto_agent_instance_t proto_server[MAX_DU];

char in_ip[40];
static uint16_t in_port;
char local_cache[40];

void *send_thread(void *args);
void *receive_thread(void *args);
pthread_t new_thread(void *(*f)(void *), void *b);
pthread_t cu_thread[MAX_DU], du_thread;
Protocol__FlexsplitMessage *proto_agent_timeout_fsp(void* args);

mid_t client_mod[MAX_DU], server_mod;
proto_agent_instance_t *client_channel[MAX_DU], *server_channel;
proto_recv_t client_info[MAX_DU];

#define TEST_MOD 0

uint8_t tcp = 0;
uint8_t udp = 0;
uint8_t sctp = 0;
char *link_type = NULL;


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

    LOG_D(PROTO_AGENT, "Received message with size %d and priority %d, calling message handle\n", size, priority);

    msg=proto_agent_handle_message(d->enb_id, data, size);

    if (msg == NULL)
    {
        LOG_D(PROTO_AGENT, "msg to send back is NULL\n");
    }
    
    if (msg != NULL){
      if (proto_agent_msg_send(d->enb_id, PROTO_AGENT_DEFAULT, msg, size, priority)) {
	err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
	goto error;
      }
      LOG_D(PROTO_AGENT, "sent message with size %d\n", size);
    }
  }

  return NULL;

error:
  LOG_E(PROTO_AGENT, "receive_thread: error %d occured\n",err_code);
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
   //printf( "Initializing server thread for listening connections\n");
   mid_t mod_id = (mid_t) 0;
   cudu_params_t* cudu = NULL;
   cudu = get_cudu_config();
   proto_server_start(mod_id, (const cudu_params_t*) cudu);
   return NULL;
}

/*  Server side function; upon a new connection 
    reception, sends the hello packets
*/
int proto_server_start(mid_t mod_id, const cudu_params_t* cudu){
  
  int channel_id;
  char *peer_address;
  
  proto_server[mod_id].enb_id = mod_id;
  
  server_mod = mod_id;

  if (cudu->local_du.du_ipv4_address != NULL)
  {
    //LOG_D(PROTO_AGENT, "DU ADDRESS IS %s\n",cudu->local_du.du_ipv4_address);
    peer_address = strdup(cudu->local_du.du_ipv4_address);
    strcpy(in_ip, cudu->local_du.du_ipv4_address);
  }
  else
  {
    strcpy(in_ip, DEFAULT_PROTO_AGENT_IPv4_ADDRESS);
    //LOG_D(PROTO_AGENT, "Cannot read DU address from conf file, setting the default (%s)\n", DEFAULT_PROTO_AGENT_IPv4_ADDRESS);
  }
  if (cudu->local_du.du_port != 0)
    in_port = cudu->local_du.du_port;
  else
  {
    in_port = DEFAULT_PROTO_AGENT_PORT;
    //LOG_D(PROTO_AGENT, "Cannot read DU port from conf file, setting the default (%d)\n", DEFAULT_PROTO_AGENT_PORT);
  }

  if(cudu->local_du.tcp == 1)
  {
      tcp = 1;
      link_type = strdup("TCP");
      LOG_D(PROTO_AGENT, "Starting PROTO agent SERVER for module id %d on ipv4 %s, port %d over TCP\n",
	proto_server[mod_id].enb_id,
	in_ip,
	in_port);
  }
  else if(cudu->local_du.udp  == 1)
  {
      udp = 1;      
      link_type = strdup("UDP");
      LOG_D(PROTO_AGENT, "Starting PROTO agent SERVER for module id %d on ipv4 %s, port %d over UDP\n",
	proto_server[mod_id].enb_id,
	in_ip,
	in_port);
  }
  else if(cudu->local_du.sctp  == 1)
  {
      sctp = 1;
      link_type = strdup("SCTP");
      LOG_D(PROTO_AGENT, "Starting PROTO agent SERVER for module id %d on ipv4 %s, port %d over SCTP\n",
	proto_server[mod_id].enb_id,
	in_ip,
	in_port);
  }
  else
  {
      tcp = 1;
      link_type = strdup("TCP");
      LOG_D(PROTO_AGENT, "Starting PROTO agent SERVER for module id %d on ipv4 %s, port %d over TCP\n",
	proto_server[mod_id].enb_id,
	in_ip,
	in_port);
    
  }  
  
  /*
   * Initialize the channel container
   */
  proto_agent_init_channel_container();

  /*Create the async channel info*/
  proto_agent_instance_t *channel_info = proto_server_async_channel_info(mod_id, in_ip, in_port, link_type, peer_address);
  
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
  
    
  if (tcp == 1) channel->type = 0;
  else if (udp == 1) channel->type = 1;
  else if (sctp == 1) channel->type = 2;
  else channel->type = 0;
  
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

  if (udp == 0)
  {
    // If the comm is not UDP, allow the server to send the first packet over the channel
    //printf( "Proto agent Server: Calling the echo_request packet constructor\n");
    msg_flag = proto_agent_echo_request(mod_id, NULL, &init_msg);
    if (msg_flag != 0)
    goto error;
  
    int msgsize = 0;
    int err_code;
    if (init_msg != NULL)
      msg = proto_agent_pack_message(init_msg, &msgsize);

    if (msg!= NULL)
    {
      LOG_D(PROTO_AGENT, "Server sending the message over the async channel\n");
      proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, (void *) channel_info);
    }
    /* After sending the message, wait for any replies; 
      the server thread blocks until it reads any data 
      over the channel
    */

  }

  
  du_thread=new_thread(proto_server_receive, &proto_server[mod_id]);
  LOG_D(PROTO_AGENT, "server ends with thread_id %u\n",du_thread);
  return 0;

error:
  LOG_E(PROTO_AGENT, "there was an error\n");
  return 1;

}

int proto_agent_start(uint8_t enb_id, mid_t cu_id, uint8_t type_id, cudu_params_t *cudu){
  
  int channel_id;
  char *peer_address = NULL;
 // cudu_params_t *cudu = get_cudu_config();
  
  proto_agent[cu_id].enb_id = cu_id;
  client_mod[cu_id] = cu_id; // FIXME: Allow for multiple types, now it will allow for DUs of different type per mod_id
  
  client_info[cu_id].type_id = type_id;
  client_info[cu_id].mod_id = cu_id;
  
  /* 
   * check the configuration - Getting all the values from the config file
   */ 
  
  if (cudu->cu[cu_id].cu_ipv4_address != NULL)
  {
    strcpy(in_ip, cudu->cu[cu_id].cu_ipv4_address); 
    peer_address = strdup(cudu->cu[cu_id].cu_ipv4_address);
  }
  else
  {
    strcpy(in_ip, DEFAULT_PROTO_AGENT_IPv4_ADDRESS);
    LOG_D(PROTO_AGENT, "Cannot read DU address from conf file, setting the default (%s)\n", DEFAULT_PROTO_AGENT_IPv4_ADDRESS);
  }
  if (cudu->cu[cu_id].cu_port != 0)
    in_port = cudu->cu[cu_id].cu_port;
  else
  {
    in_port = DEFAULT_PROTO_AGENT_PORT;
    LOG_D(PROTO_AGENT, "Cannot read DU port from conf file, setting the default (%d)\n", DEFAULT_PROTO_AGENT_PORT);
  }

  if(cudu->cu[cu_id].tcp == 1)
  {
      tcp = 1;
      link_type = strdup("TCP");
      LOG_D(PROTO_AGENT, "Starting PROTO agent client for module id %d on ipv4 %s, port %d over TCP\n",
	proto_server[cu_id].enb_id,
	in_ip,
	in_port);
  }
  else if(cudu->cu[cu_id].udp == 1)
  {
      udp = 1;      
      link_type = strdup("UDP");
      LOG_D(PROTO_AGENT, "Starting PROTO agent client for module id %d on ipv4 %s, port %d over UDP\n",
	proto_server[cu_id].enb_id,
	in_ip,
	in_port);
  }
  else if(cudu->cu[cu_id].sctp == 1)
  {
      sctp = 1;
      link_type = strdup("SCTP");
      LOG_D(PROTO_AGENT, "Starting PROTO agent client for module id %d on ipv4 %s, port %d over SCTP\n",
	proto_server[cu_id].enb_id,
	in_ip,
	in_port);
  }
  else
  {
      tcp = 1;
      link_type = strdup("TCP");
      LOG_D(PROTO_AGENT, "Starting PROTO agent client for module id %d on ipv4 %s, port %d over TCP\n",
	proto_server[cu_id].enb_id,
	in_ip,
	in_port);
    
  }  

  /*
   * Initialize the channel container
   */
  proto_agent_init_channel_container();

  /*Create the async channel info*/
  
  proto_agent_instance_t *channel_info = proto_agent_async_channel_info(cu_id, in_ip, in_port, link_type, peer_address);
  client_channel[cu_id] = channel_info;
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
  
  if (tcp == 1) channel->type = 0;
  else if (udp == 1) channel->type = 1;
  else if (sctp == 1) channel->type = 2;
  else channel->type = 0;

  /*Register the channel for all underlying agents (use ENB_AGENT_MAX)*/
  proto_agent_register_channel(cu_id, channel, ENB_AGENT_MAX);

  void                  *data;
  int                   size;
  int                   priority;
   err_code_t             err_code;
  Protocol__FlexsplitMessage *msg = NULL;
  Protocol__FlexsplitMessage *init_msg=NULL;
  Protocol__FlexsplitMessage *rep_msg=NULL;
  Protocol__FlexsplitMessage *ser_msg=NULL;
  int msg_flag;
  
  // In the case of UDP comm, start the echo request from the client side; the server thread should be blocked until it reads the SRC port of the 1st packet
  if (udp == 1)
  {
    msg_flag = proto_agent_echo_request(cu_id, NULL, &init_msg);  



    if (msg_flag != 0)
      goto error;
    
    int msgsize = 0;
    int err_code;
    if (init_msg != NULL)
      msg = proto_agent_pack_message(init_msg, &msgsize);

    if (msg!= NULL)
    {
      LOG_D(PROTO_AGENT, "Client sending the ECHO_REQUEST message over the async channel\n");
      proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, (void *) channel_info);
    }
  }
  /* After sending the message, wait for any replies; 
     the server thread blocks until it reads any data 
     over the channel
  */
  cu_thread[cu_id]=new_thread(proto_client_receive, (void *) &client_info[cu_id]);
  return 0;

error:
  LOG_E(PROTO_AGENT, "there was an error %u\n", err_code);
  return 1;

}

void 
proto_agent_send_hello(void)
{
  Protocol__FlexsplitMessage *msg = NULL;
  Protocol__FlexsplitMessage *init_msg=NULL;
  int msg_flag = 0;

  
  //printf( "PDCP agent: Calling the HELLO packet constructor\n");
  msg_flag = proto_agent_hello(proto_agent[TEST_MOD].enb_id, NULL, &init_msg);
  
  int msgsize = 0;
  if (msg_flag == 0)
  {
    proto_agent_serialize_message(init_msg, &msg, &msgsize);
  }
  
  LOG_D(PROTO_AGENT, "Agent sending the message over the async channel\n");
  proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, (void *) client_channel[TEST_MOD]);
}


void
proto_agent_send_rlc_data_req(uint8_t mod_id, uint8_t type_id, const protocol_ctxt_t* const ctxt_pP, const srb_flag_t srb_flagP, const MBMS_flag_t MBMS_flagP, const rb_id_t rb_idP, const mui_t muiP, 
			      confirm_t confirmP, sdu_size_t sdu_sizeP, mem_block_t *sdu_pP)
{
  
  //LOG_D(PROTO_AGENT, "PROTOPDCP: sending the data req over the async channel\n");
  
  Protocol__FlexsplitMessage *msg = NULL;
  Protocol__FlexsplitMessage *init_msg=NULL;
  Protocol__FlexsplitMessage *rep = NULL;
  Protocol__FlexsplitMessage *srep = NULL;

  int msg_flag = 0;
  void *data=NULL;
  int priority;
  int size;
  int ret;
  int err_code;
  
  //printf( "PDCP agent: Calling the PDCP DATA REQ constructor\n");
 
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

  msg_flag = proto_agent_pdcp_data_req(type_id, (void *) args, &init_msg);
  if (msg_flag != 0)
    goto error;
  
  int msgsize = 0;
  if (init_msg != NULL)
  {
    
    msg = proto_agent_pack_message(init_msg, &msgsize);
  
    
    LOG_D(PROTO_AGENT, "Sending the pdcp data_req message over the async channel\n");
  
    if (msg!=NULL)
      proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, (void *) client_channel[mod_id]);
  
  }
  else
  {
    goto error; 
  }
  
  return;
error:
  LOG_E(PROTO_AGENT, "PROTO_AGENT there was an error\n");
  return;
  
}

  
void
proto_agent_send_pdcp_data_ind(const protocol_ctxt_t* const ctxt_pP, const srb_flag_t srb_flagP,
			       const MBMS_flag_t MBMS_flagP, const rb_id_t rb_idP, sdu_size_t sdu_sizeP, mem_block_t *sdu_pP)
{
  //LOG_D(PROTO_AGENT, "PROTOPDCP: Sending Data Indication over the async channel\n");
  
  Protocol__FlexsplitMessage *msg = NULL;
  Protocol__FlexsplitMessage *init_msg = NULL;
  Protocol__FlexsplitMessage *rep = NULL;
  Protocol__FlexsplitMessage *srep = NULL;

  
  int msg_flag = 0;
  void *data=NULL;
  int priority;
  int size;
  int ret;
  int err_code;
  
  //printf( "PDCP agent: Calling the PDCP_DATA_IND constructor\n");
 
  data_req_args *args = malloc(sizeof(data_req_args));
  
  args->ctxt = malloc(sizeof(protocol_ctxt_t));
  memcpy(args->ctxt, ctxt_pP, sizeof(protocol_ctxt_t));
  args->srb_flag = srb_flagP;
  args->MBMS_flag = MBMS_flagP;
  args->rb_id = rb_idP;
  args->sdu_size = sdu_sizeP;
  args->sdu_p = malloc(sdu_sizeP);
  memcpy(args->sdu_p, sdu_pP->data, sdu_sizeP);

  msg_flag = proto_agent_pdcp_data_ind(proto_server[server_mod].enb_id, (void *) args, &init_msg);
  if (msg_flag != 0)
    goto error;

  int msgsize = 0;
  
  if (init_msg != NULL)
  {
    msg = proto_agent_pack_message(init_msg, &msgsize);
    
    if (msg!=NULL)
    {
      LOG_D(PROTO_AGENT, "Sending the pdcp data_ind message over the async channel\n");
      proto_agent_async_msg_send((void *)msg, (int) msgsize, 1, (void *) server_channel);
    }
  }
  else
  {
    goto error; 
  }
  return;

error:
  LOG_E(PROTO_AGENT, "there was an error\n");
  return;
  
}

  


void *
proto_server_receive(void)
{
  proto_agent_instance_t         *d = &proto_server[server_mod];
  void                  *data = NULL;
  int                   size;
  int                   priority;
  err_code_t             err_code;

  Protocol__FlexsplitMessage *msg;
  Protocol__FlexsplitMessage *ser_msg;
  
  while (1) {
   
    msg = NULL;
    ser_msg = NULL;
    
    if (proto_agent_async_msg_recv(&data, &size, &priority, server_channel)){
      err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(PROTO_AGENT, "Server side Received message with size %d and priority %d, calling message handle\n", size, priority);

    msg=proto_agent_handle_message(d->enb_id, data, size);

    if (msg == NULL)
    {
        LOG_D(PROTO_AGENT, "msg to send back is NULL\n");
    }
    else
    {
        ser_msg = proto_agent_pack_message(msg, &size);
    }
  
    LOG_D(PROTO_AGENT, "Server sending the reply message over the async channel\n");
    if (ser_msg != NULL){
      if (proto_agent_async_msg_send((void *)ser_msg, (int) size, 1, (void *) server_channel)){
	err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
	goto error;
      }
      LOG_D(PROTO_AGENT, "sent message with size %d\n", size);
    }

  }
  
  return NULL;

error:
  LOG_E(PROTO_AGENT, "server_receive_thread: error %d occured\n",err_code);
  return NULL;
  
}
  
void *
proto_client_receive(void *args)
{
  
  proto_recv_t*         recv = args;
  mid_t 	      recv_mod = recv->mod_id;
  uint8_t 	      type = recv->type_id;

  LOG_D(PROTO_AGENT, "\n\nrecv mod is %u\n\n",recv_mod);  
  //proto_agent_instance_t         *d = &proto_agent[TEST_MOD];
  void                  *data = NULL;
  int                   size;
  int                   priority;
  err_code_t             err_code;

  Protocol__FlexsplitMessage *msg;
  Protocol__FlexsplitMessage *ser_msg;


  while (1) {
      
    msg = NULL;
    ser_msg = NULL;

    while(client_channel[recv_mod] == NULL)
    {
	//just wait
    }
    LOG_D(PROTO_AGENT, "Will receive packets\n");
    if (proto_agent_async_msg_recv(&data, &size, &priority, client_channel[recv_mod])){
      err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(PROTO_AGENT, "Client Received message with size %d and priority %d, calling message handle with mod_id %u\n", size, priority, recv_mod);

    msg=proto_agent_handle_message(recv_mod, data, size);

    if (msg == NULL)
    {
        LOG_D(PROTO_AGENT, "msg to send back is NULL\n");
    }
    else
    {
      ser_msg = proto_agent_pack_message(msg, &size);
    }
    LOG_D(PROTO_AGENT, "Server sending the reply message over the async channel\n");

    if (ser_msg != NULL){
      if (proto_agent_async_msg_send((void *)ser_msg, (int) size, 1, (void *) client_channel[recv_mod])){
	err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
	goto error;
      }
      LOG_D(PROTO_AGENT, "sent message with size %d\n", size);
    }
   
  }
  
  return NULL;

error:
  LOG_E(PROTO_AGENT, " client_receive_thread: error %d occured\n",err_code);
  return NULL;
  
}


uint8_t select_du(uint8_t max_dus)
{
    static uint8_t selected = 0;
    if (selected < max_dus -1 )
    {
      selected++;
    }
    else
    {
      selected = 0;
    }
    return selected;
}

