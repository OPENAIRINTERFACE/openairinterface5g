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
//#include "enb_agent_mac_defs.h" // TODO: Check if they are needed

//#include "enb_agent_extern.h" // TODO: Check if they are needed for this implementation

#include "assertions.h"

#include "proto_agent_net_comm.h" // TODO: Check if they are needed
#include "proto_agent_async.h" 

//#define TEST_TIMER

proto_agent_instance_t proto_agent[NUM_MAX_ENB];
proto_agent_instance_t proto_server[NUM_MAX_ENB];

// Use the same configuration file for the moment

char in_ip[40];
static uint16_t in_port;
char local_cache[40];

void *send_thread(void *args);
void *client_receive_thread(void *args);
void *server_receive_thread(void *args);
//void *server_send_thread(void *args);
pthread_t new_thread(void *(*f)(void *), void *b);
//Protocol__ProgranMessage *enb_agent_timeout_prp(void* args);
Protocol__FlexsplitMessage *proto_agent_timeout_fsp(void* args);


/* 
 * enb agent task mainly wakes up the tx thread for periodic and oneshot messages to the controller 
 * and can interact with other itti tasks
*/
void *proto_agent_task(void *args){

  proto_agent_instance_t         *d = (proto_agent_instance_t *) args;
  Protocol__FlexsplitMessage *msg;
  void *data;
  int size;
  err_code_t err_code;
  int                   priority;

  MessageDef                     *msg_p           = NULL;
  const char                     *msg_name        = NULL;
  instance_t                      instance;
  int                             result;


  itti_mark_task_ready(TASK_PROTO_AGENT);

  do {
    // Wait for a message
    itti_receive_msg (TASK_PROTO_AGENT, &msg_p);
    DevAssert(msg_p != NULL);
    msg_name = ITTI_MSG_NAME (msg_p);
    instance = ITTI_MSG_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(PROTO_AGENT, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;
    
    case TIMER_HAS_EXPIRED:
      msg = proto_agent_process_timeout(msg_p->ittiMsg.timer_has_expired.timer_id, msg_p->ittiMsg.timer_has_expired.arg);
      if (msg != NULL){
	data=proto_agent_pack_message(msg,&size);
	if (proto_agent_msg_send(d->enb_id, PROTO_AGENT_DEFAULT, data, size, priority)) {
	  err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
	  goto error;
	}

	LOG_D(PROTO_AGENT,"sent message with size %d\n", size);
      }
      break;

    default:
      LOG_E(PROTO_AGENT, "Received unexpected message %s\n", msg_name);
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    continue;
  error:
    LOG_E(PROTO_AGENT,"proto_agent_task: error %d occured\n",err_code);
  } while (1);

  return NULL;
}

void *send_thread(void *args) {

  proto_agent_instance_t         *d = args;
  void                  *data;
  int                   size;
  int                   priority;
  err_code_t             err_code;

  Protocol__FlexsplitMessage *msg;
  
  void **init_msg;
  int msg_flag = 0;
  msg_flag = proto_agent_hello(d->enb_id, args, init_msg);

  if (msg_flag == -1)
  {
	LOG_E( PROTO_AGENT, "Server did not create the message\n");
	goto error;
  }
  if (proto_agent_msg_send(d->enb_id, PROTO_AGENT_DEFAULT, init_msg, sizeof(Protocol__FlexsplitMessage), 0)) {
    err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENQUEUING;
    goto error;
  }

    LOG_D(PROTO_AGENT,"sent message with size %d\n", size);
  
    
//    msg=proto_agent_handle_message(d->enb_id, data, size);

    free(data);

    free(*init_msg);
    // check if there is something to send back to the controller
    if (msg != NULL){
      data=proto_agent_pack_message(msg,&size);

      if (proto_agent_msg_send(d->enb_id, PROTO_AGENT_DEFAULT, data, size, priority)) {
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

    LOG_D(PROTO_AGENT,"received message with size %d\n", size);
  
    
    msg=proto_agent_handle_message(d->enb_id, data, size);

    free(data);
    
    // check if there is something to send back to the controller
    if (msg != NULL){
      data=proto_agent_pack_message(msg,&size);

      if (proto_agent_msg_send(d->enb_id, PROTO_AGENT_DEFAULT, data, size, priority)) {
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


// This is the only function that we will call for the moment from this file
int proto_server_start(mid_t mod_id, const Enb_properties_array_t* enb_properties){
  
  int channel_id;

  printf("INSIDE PROTO_SERVER_START\n"); 
  //  Maybe change the RAN_LTE_OAI to protocol?? (e.g. PDCP)
  set_enb_vars(mod_id, RAN_LTE_OAI);
  proto_server[mod_id].enb_id = mod_id;
  
  /* 
   * check the configuration - Getting all the values from the config file
   */ 
  if (enb_properties->properties[mod_id]->proto_agent_cache != NULL) {
    strncpy(local_cache, enb_properties->properties[mod_id]->proto_agent_cache, sizeof(local_cache));
    local_cache[sizeof(local_cache) - 1] = 0;
  } else {
    strcpy(local_cache, DEFAULT_PROTO_AGENT_CACHE);
  }
  
  if (enb_properties->properties[mod_id]->proto_agent_ipv4_address != NULL) {
    strncpy(in_ip, enb_properties->properties[mod_id]->proto_agent_ipv4_address, sizeof(in_ip) );
    in_ip[sizeof(in_ip) - 1] = 0; // terminate string
  } else {
    strcpy(in_ip, DEFAULT_PROTO_AGENT_IPv4_ADDRESS ); 
  }
  
  if (enb_properties->properties[mod_id]->proto_agent_port != 0 ) {
    in_port = enb_properties->properties[mod_id]->proto_agent_port;
  } else {
    in_port = DEFAULT_PROTO_AGENT_PORT ;
  }
  LOG_I(ENB_AGENT,"starting PROTO agent SERVER for module id %d on ipv4 %s, port %d\n",  
	proto_server[mod_id].enb_id,
	in_ip,
	in_port);

  /*
   * Initialize the channel container
   */
  proto_agent_init_channel_container();

  /*Create the async channel info*/
  printf("Before creating the async server channe;\n");
  proto_agent_instance_t *channel_info = proto_server_async_channel_info(mod_id, in_ip, in_port);
  printf("After creating the async server channe;\n");

  /*Create a channel using the async channel info*/
  printf("Before creating the receive channel\n");

  channel_id = proto_agent_create_channel((void *) channel_info, 
					proto_agent_async_msg_send, 
					proto_agent_async_msg_recv,
					proto_agent_async_release);

  printf("After creating the receive channel\n");
  
  if (channel_id <= 0) {
    goto error;
  }
  printf("Before getting ing the receive thread\n");

  proto_agent_channel_t *channel = get_channel(channel_id);
  printf("After getting ing the receive thread\n");
  
  if (channel == NULL) {
    goto error;
  }

  /*Register the channel for all underlying agents (use ENB_AGENT_MAX)*/
  printf("Before registering the receive channel \n");

  proto_agent_register_channel(mod_id, channel, ENB_AGENT_MAX);
  printf("After registering the receive channel\n");

  /*Example of registration for a specific agent(MAC):
   *enb_agent_register_channel(mod_id, channel, ENB_AGENT_MAC);
   */

  /*Initialize the continuous MAC stats update mechanism*/
  // For the moment we do not need this
  // enb_agent_init_cont_mac_stats_update(mod_id);
  printf("Before initializing the receive thread\n");
  //new_thread(send_thread, &proto_server[mod_id]);
  new_thread(receive_thread, &proto_server[mod_id]);
  printf("After initializing the receive thread\n");

  /*
   * initilize a timer 
   */ 
  printf("Before initializing the receive timer\n");
  
  proto_agent_init_timer();

  printf("After initializing the receive thread\n");

  /* 
   * start the enb agent task for tx and interaction with the underlying network function
   */ 
  
  if (itti_create_task (TASK_PROTO_AGENT, proto_agent_task, (void *) &proto_server[mod_id]) < 0) {
    LOG_E(PROTO_AGENT, "Create task for eNB Agent failed\n");
    return -1;
  } 

  new_thread(send_thread, &proto_server[mod_id]);


  LOG_I(PROTO_AGENT,"server ends\n");
  return 0;

error:
  LOG_I(PROTO_AGENT,"there was an error\n");
  return 1;

}

int proto_agent_start(mid_t mod_id, const Enb_properties_array_t* enb_properties){
  
  int channel_id;
  
  printf("Starting client\n");

  //  Maybe change the RAN_LTE_OAI to protocol?? (e.g. PDCP)
  set_enb_vars(mod_id, RAN_LTE_OAI);
  proto_agent[mod_id].enb_id = mod_id;
  
  /* 
   * check the configuration - Getting all the values from the config file
   */ 
  if (enb_properties->properties[mod_id]->proto_agent_cache != NULL) {
    strncpy(local_cache, enb_properties->properties[mod_id]->proto_agent_cache, sizeof(local_cache));
    local_cache[sizeof(local_cache) - 1] = 0;
  } else {
    strcpy(local_cache, DEFAULT_PROTO_AGENT_CACHE);
  }
  
  if (enb_properties->properties[mod_id]->proto_agent_ipv4_address != NULL) {
    strncpy(in_ip, enb_properties->properties[mod_id]->proto_agent_ipv4_address, sizeof(in_ip) );
    in_ip[sizeof(in_ip) - 1] = 0; // terminate string
  } else {
    strcpy(in_ip, DEFAULT_PROTO_AGENT_IPv4_ADDRESS ); 
  }
  
  if (enb_properties->properties[mod_id]->proto_agent_port != 0 ) {
    in_port = enb_properties->properties[mod_id]->proto_agent_port;
  } else {
    in_port = DEFAULT_PROTO_AGENT_PORT ;
  }
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

  /*Initialize the continuous MAC stats update mechanism*/
  // For the moment we do not need this
  // enb_agent_init_cont_mac_stats_update(mod_id);
  
  new_thread(receive_thread, &proto_agent[mod_id]);

  /*Initialize and register the mac xface. Must be modified later
   *for more flexibility in agent management */
   // We do not need this 
  // AGENT_MAC_xface *mac_agent_xface = (AGENT_MAC_xface *) malloc(sizeof(AGENT_MAC_xface));
  // enb_agent_register_mac_xface(mod_id, mac_agent_xface);
  
  /* 
   * initilize a timer 
   */ 
  
  proto_agent_init_timer();

  /*
   * Initialize the mac agent
   */
  //enb_agent_init_mac_agent(mod_id);
  
  /* 
   * start the enb agent task for tx and interaction with the underlying network function
   */ 
  
  if (itti_create_task (TASK_PROTO_AGENT, proto_agent_task, (void *) &proto_agent[mod_id]) < 0) {
    LOG_E(PROTO_AGENT, "Create task for eNB Agent failed\n");
    return -1;
  } 

  LOG_I(PROTO_AGENT,"client ends\n");
  return 0;

error:
  LOG_I(PROTO_AGENT,"there was an error\n");
  return 1;

}




Protocol__FlexsplitMessage *proto_agent_timeout(void* args){

  //  enb_agent_timer_args_t *timer_args = calloc(1, sizeof(*timer_args));
  //memcpy (timer_args, args, sizeof(*timer_args));
  proto_agent_timer_args_t *timer_args = (proto_agent_timer_args_t *) args;
  
  LOG_I(PROTO_AGENT, "proto_agent %d timeout\n", timer_args->mod_id);
  //LOG_I(ENB_AGENT, "eNB action %d ENB flags %d \n", timer_args->cc_actions,timer_args->cc_report_flags);
  //LOG_I(ENB_AGENT, "UE action %d UE flags %d \n", timer_args->ue_actions,timer_args->ue_report_flags);
  
  return NULL;
}
