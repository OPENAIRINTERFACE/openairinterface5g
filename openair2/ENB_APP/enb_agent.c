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

#include "enb_agent_common.h"
#include "log.h"
#include "enb_agent.h"

#include "assertions.h"

//#define TEST_TIMER

enb_agent_instance_t enb_agent[NUM_MAX_ENB_AGENT];
msg_context_t shared_ctxt[NUM_MAX_ENB_AGENT];
/* this could also go into enb_agent struct*/ 
enb_agent_info_t  enb_agent_info;

char in_ip[40];
static uint16_t in_port;

void *send_thread(void *args);
void *receive_thread(void *args);
pthread_t new_thread(void *(*f)(void *), void *b);
err_code_t enb_agent_timeout(void* args);

/* 
 * enb agent task mainly wakes up the tx thread for periodic and oneshot messages to the controller 
 * and can interact with other itti tasks
*/
void *enb_agent_task(void *args){

  MessageDef                     *msg_p           = NULL;
  const char                     *msg_name        = NULL;
  instance_t                      instance;
  int                             result;


  itti_mark_task_ready(TASK_ENB_AGENT);

  do {
    // Wait for a message
    itti_receive_msg (TASK_ENB_AGENT, &msg_p);
    DevAssert(msg_p != NULL);
    msg_name = ITTI_MSG_NAME (msg_p);
    instance = ITTI_MSG_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(ENB_AGENT, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;
    
    case TIMER_HAS_EXPIRED:
      enb_agent_process_timeout(msg_p->ittiMsg.timer_has_expired.timer_id, &msg_p->ittiMsg.timer_has_expired.arg);
      break;

    default:
      LOG_E(ENB_AGENT, "Received unexpected message %s\n", msg_name);
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);

  return NULL;
}

void *send_thread(void *args) {

#ifdef TEST_TIMER

  msg_context_t         *d = args;
  void                  *data;
  int                   size;
  int                   priority;

  struct timeval t1, t2;
  long long t;
  struct timespec ts;
  unsigned int delay = 250*1000;
  while(1) {
    gettimeofday(&t1, NULL);
    enb_agent_sleep_until(&ts, delay);
    gettimeofday(&t2, NULL);
    t = ((t2.tv_sec * 1000000) + t2.tv_usec) - ((t1.tv_sec * 1000000) + t1.tv_usec);
    LOG_I(ENB_AGENT, "Call to sleep_until(%d) took %lld us\n", delay, t);
    sleep(1);
  }

#endif
  /* while (1) {
    // need logic for the timer, and 
    usleep(10);
    if (message_put(d->tx_mq, data, size, priority)) goto error;
    }*/

  return NULL;

error:
  printf("receive_thread: there was an error\n");
  return NULL;
}

void *receive_thread(void *args) {

  msg_context_t         *d = args;
  void                  *data;
  int                   size;
  int                   priority;
  err_code_t             err_code;

  Protocol__ProgranMessage *msg;
  
  while (1) {
    if (message_get(d->rx_mq, &data, &size, &priority)){
      err_code = PROTOCOL__PROGRAN_ERR__MSG_DEQUEUING;
      goto error;
    }
    LOG_D(ENB_AGENT,"received message with size %d\n", size);
  
    
    msg=enb_agent_handle_message(d->mod_id, data, size);

    free(data);
    
    d->rx_xid = ((d->rx_xid)+1)%4;
    d->tx_xid = d->rx_xid;
  
    // check if there is something to send back to the controller
    if (msg != NULL){
      data=enb_agent_send_message(msg,&size);
     
      if (message_put(d->tx_mq, data, size, priority)){
	err_code = PROTOCOL__PROGRAN_ERR__MSG_ENQUEUING;
	goto error;
      }
      LOG_D(ENB_AGENT,"sent message with size %d\n", size);
    }
    
  }

  return NULL;

error:
  LOG_E(ENB_AGENT,"receive_thread: error %d occured\n",err_code);
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

int enb_agent_start(mid_t mod_id, const Enb_properties_array_t* enb_properties){

  // 
  set_enb_vars(mod_id, RAN_LTE_OAI);
  enb_agent[mod_id].mod_id = mod_id;
  enb_agent_info.nb_modules+=1;
  
  /* 
   * check the configuration
   */ 
  if (enb_properties->properties[mod_id]->enb_agent_ipv4_address != NULL) {
    strncpy(in_ip, enb_properties->properties[mod_id]->enb_agent_ipv4_address, sizeof(in_ip) );
    in_ip[sizeof(in_ip) - 1] = 0; // terminate string
  } else {
    strcpy(in_ip, DEFAULT_ENB_AGENT_IPv4_ADDRESS ); 
  }
  
  if (enb_properties->properties[mod_id]->enb_agent_port != 0 ) {
    in_port = enb_properties->properties[mod_id]->enb_agent_port;
  } else {
    in_port = DEFAULT_ENB_AGENT_PORT ;
  }
  LOG_I(ENB_AGENT,"starting enb agent client for module id %d on ipv4 %s, port %d\n",  
	enb_agent[mod_id].mod_id,
	in_ip,
	in_port);

  //#define TEST_TIMER 0    
#if !defined TEST_TIMER

  /* 
   * create a socket 
   */ 
  enb_agent[mod_id].link = new_link_client(in_ip, in_port);
  if (enb_agent[mod_id].link == NULL) goto error;
  
  LOG_I(ENB_AGENT,"starting enb agent client for module id %d on ipv4 %s, port %d\n",  
	enb_agent[mod_id].mod_id,
	in_ip,
	in_port);
  /* 
   * create a message queue
   */ 
  
  enb_agent[mod_id].send_queue = new_message_queue();
  if (enb_agent[mod_id].send_queue == NULL) goto error;
  enb_agent[mod_id].receive_queue = new_message_queue();
  if (enb_agent[mod_id].receive_queue == NULL) goto error;
  
  /* 
   * create a link manager 
   */ 
  
  enb_agent[mod_id].manager = create_link_manager(enb_agent[mod_id].send_queue, enb_agent[mod_id].receive_queue, enb_agent[mod_id].link);
  if (enb_agent[mod_id].manager == NULL) goto error;

  memset(&shared_ctxt, 0, sizeof(msg_context_t));
  
  shared_ctxt[mod_id].mod_id = mod_id;
  shared_ctxt[mod_id].tx_mq =  enb_agent[mod_id].send_queue;
  shared_ctxt[mod_id].rx_mq =  enb_agent[mod_id].receive_queue;

  /* 
   * start the enb agent rx thread 
   */ 
  
  new_thread(receive_thread, &shared_ctxt[mod_id]);

#endif 
  
  /* 
   * initilize a timer 
   */ 
  
  enb_agent_init_timer();
  
  
  
  /* 
   * start the enb agent task for tx and interaction with the underlying network function
   */ 
  
  if (itti_create_task (TASK_ENB_AGENT, enb_agent_task, NULL) < 0) {
    LOG_E(ENB_AGENT, "Create task for eNB Agent failed\n");
    return -1;
  }


#ifdef TEST_TIMER
  long timer_id=0;
  enb_agent_timer_args_t timer_args;
  memset (&timer_args, 0, sizeof(enb_agent_timer_args_t));
  timer_args.mod_id = mod_id;
  timer_args.cc_actions= ENB_AGENT_ACTION_APPLY;
  timer_args.cc_report_flags = PROTOCOL__PRP_CELL_STATS_TYPE__PRCST_NOISE_INTERFERENCE;
  timer_args.ue_actions =  ENB_AGENT_ACTION_SEND;
  timer_args.ue_report_flags = PROTOCOL__PRP_UE_STATS_TYPE__PRUST_BSR | PROTOCOL__PRP_UE_STATS_TYPE__PRUST_DL_CQI;
  enb_agent_create_timer(1, 0, ENB_AGENT_DEFAULT, mod_id, ENB_AGENT_TIMER_TYPE_PERIODIC, enb_agent_timeout,(void*)&timer_args, &timer_id);
#endif 

  new_thread(send_thread, &shared_ctxt);

  //while (1) pause();
 

  LOG_I(ENB_AGENT,"client ends\n");
  return 0;

error:
  LOG_I(ENB_AGENT,"there was an error\n");
  return 1;

}



int enb_agent_stop(mid_t mod_id){
  
  int i=0;

  enb_agent_destroy_timers();
  for ( i =0; i < enb_agent_info.nb_modules; i++) {
  
    destroy_link_manager(enb_agent[i].manager);
  
    destroy_message_queue(enb_agent[i].send_queue);
    destroy_message_queue(enb_agent[i].receive_queue);
  
    close_link(enb_agent[i].link);
  }
}



err_code_t enb_agent_timeout(void* args){

  //  enb_agent_timer_args_t *timer_args = calloc(1, sizeof(*timer_args));
  //memcpy (timer_args, args, sizeof(*timer_args));
  enb_agent_timer_args_t *timer_args = (enb_agent_timer_args_t *) args;
  
  LOG_I(ENB_AGENT, "enb_agent %d timeout\n", timer_args->mod_id);
  LOG_I(ENB_AGENT, "eNB action %d ENB flags %d \n", timer_args->cc_actions,timer_args->cc_report_flags);
  LOG_I(ENB_AGENT, "UE action %d UE flags %d \n", timer_args->ue_actions,timer_args->ue_report_flags);
  
  return 0;
}
