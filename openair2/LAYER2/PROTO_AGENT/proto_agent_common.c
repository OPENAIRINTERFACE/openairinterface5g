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

/*! \file proto_agent_common.c
 * \brief common primitives for all agents 
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#include<stdio.h>
#include <dlfcn.h>
#include <time.h>

#include "proto_agent_common.h"
#include "PHY/extern.h"
#include "log.h"

#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "rrc_eNB_UE_context.h"

void * enb[NUM_MAX_ENB];
void * enb_ue[NUM_MAX_ENB];
void * enb_rrc[NUM_MAX_ENB];
/*
 * message primitives
 */

int proto_agent_serialize_message(Protocol__FlexsplitMessage *msg, void **buf, int *size) {

  *size = protocol__flexsplit_message__get_packed_size(msg);
  
  *buf = malloc(*size);
  if (buf == NULL)
    goto error;
  
  protocol__flexsplit_message__pack(msg, *buf);
  
  return 0;
  
 error:
  LOG_E(PROTO_AGENT, "an error occured\n"); // change the com
  return -1;
}



/* We assume that the buffer size is equal to the message size.
   Should be chekced durint Tx/Rx */
int proto_agent_deserialize_message(void *data, int size, Protocol__FlexsplitMessage **msg) {
  *msg = protocol__flexsplit_message__unpack(NULL, size, data);
  if (*msg == NULL)
    goto error;

  return 0;
  
 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}



int fsp_create_header(xid_t xid, Protocol__FspType type,  Protocol__FspHeader **header) {
  
  *header = malloc(sizeof(Protocol__FspHeader));
  if(*header == NULL)
    goto error;
  
  protocol__fsp_header__init(*header);
  (*header)->version = FLEXSPLIT_VERSION;
  (*header)->has_version = 1; 
  // check if the type is set
  (*header)->type = type;
  (*header)->has_type = 1;
  (*header)->xid = xid;
  (*header)->has_xid = 1;
  return 0;

 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int proto_agent_hello(mid_t mod_id, const void *params, Protocol__FlexsplitMessage **msg) {
 
  Protocol__FspHeader *header;
  /*TODO: Need to set random xid or xid from received hello message*/
  xid_t xid = 1;
  if (fsp_create_header(xid, PROTOCOL__FSP_TYPE__FSPT_HELLO, &header) != 0)
    goto error;

  Protocol__FspHello *hello_msg;
  hello_msg = malloc(sizeof(Protocol__FspHello));
  if(hello_msg == NULL)
    goto error;
  protocol__fsp_hello__init(hello_msg);
  hello_msg->header = header;

  *msg = malloc(sizeof(Protocol__FlexsplitMessage));
  if(*msg == NULL)
    goto error;
  
  protocol__flexsplit_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXSPLIT_MESSAGE__MSG_HELLO_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXSPLIT_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->has_msg_dir = 1;
  (*msg)->hello_msg = hello_msg;
  return 0;
  
 error:
  if(header != NULL)
    free(header);
  if(hello_msg != NULL)
    free(hello_msg);
  if(*msg != NULL)
    free(*msg);
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int proto_agent_destroy_hello(Protocol__FlexsplitMessage *msg) {
  
  if(msg->msg_case != PROTOCOL__FLEXSPLIT_MESSAGE__MSG_HELLO_MSG)
    goto error;
  
  free(msg->hello_msg->header);
  free(msg->hello_msg);
  free(msg);
  return 0;

 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

// call this function to start a nanosecond-resolution timer
struct timespec timer_start(){
    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    return start_time;
}

// call this function to end a timer, returning nanoseconds elapsed as a long
long timer_end(struct timespec start_time){
    struct timespec end_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
    long diffInNanos = end_time.tv_nsec - start_time.tv_nsec;
    return diffInNanos;
}


/*
 * get generic info from RAN
 */

void set_enb_vars(mid_t mod_id, ran_name_t ran){

  switch (ran){
  case RAN_LTE_OAI :
    enb[mod_id] =  (void *)&eNB_mac_inst[mod_id];
    enb_ue[mod_id] = (void *)&eNB_mac_inst[mod_id].UE_list;
    enb_rrc[mod_id] = (void *)&eNB_rrc_inst[mod_id];
    break;
  default :
    goto error;
  }

  return; 

 error:
  LOG_E(PROTO_AGENT, "unknown RAN name %d\n", ran);
}


//struct proto_agent_map agent_map;
proto_agent_timer_instance_t timer_instance;
err_code_t proto_agent_init_timer(void){
  
  LOG_I(PROTO_AGENT, "init RB tree\n");

  RB_INIT(&timer_instance.proto_agent_head);
 
  /*
    struct proto_agent_timer_element_s e;
  memset(&e, 0, sizeof(proto_agent_timer_element_t));
  RB_INSERT(proto_agent_map, &agent_map, &e); 
  */
 return PROTOCOL__FLEXSPLIT_ERR__NO_ERR;
}

RB_GENERATE(proto_agent_map,proto_agent_timer_element_s, entry, proto_agent_compare_timer);

/* The timer_id might not be the best choice for the comparison */
int proto_agent_compare_timer(struct proto_agent_timer_element_s *a, struct proto_agent_timer_element_s *b){

  if (a->timer_id < b->timer_id) return -1;
  if (a->timer_id > b->timer_id) return 1;

  // equal timers
  return 0;
}

err_code_t proto_agent_create_timer(uint32_t interval_sec,
				  uint32_t interval_usec,
				  agent_id_t     agent_id,
				  instance_t     instance,
				  uint32_t timer_type,
				  xid_t xid,
				  proto_agent_timer_callback_t cb,
				  void*    timer_args,
				  long *timer_id){
  
  struct proto_agent_timer_element_s *e = calloc(1, sizeof(*e));
  DevAssert(e != NULL);
  
//uint32_t timer_id;
  int ret=-1;
  
  if ((interval_sec == 0) && (interval_usec == 0 ))
    return TIMER_NULL;
  
  if (timer_type >= PROTO_AGENT_TIMER_TYPE_MAX)
    return TIMER_TYPE_INVALIDE;
  
  if (timer_type  ==   PROTO_AGENT_TIMER_TYPE_ONESHOT){ 
    ret = timer_setup(interval_sec, 
		      interval_usec, 
		      TASK_PROTO_AGENT, 
		      instance, 
		      TIMER_ONE_SHOT,
		      timer_args,
		      timer_id);
    
    e->type = TIMER_ONE_SHOT;
  }
  else if (timer_type  ==   PROTO_AGENT_TIMER_TYPE_PERIODIC ){
    ret = timer_setup(interval_sec, 
		      interval_usec, 
		      TASK_PROTO_AGENT, 
		      instance, 
		      TIMER_PERIODIC,
		      timer_args,
		      timer_id);
    
    e->type = TIMER_PERIODIC;
  }
  
  if (ret < 0 ) {
    return TIMER_SETUP_FAILED; 
  }

  e->agent_id = agent_id;
  e->instance = instance;
  e->state = PROTO_AGENT_TIMER_STATE_ACTIVE;
  e->timer_id = *timer_id;
  e->xid = xid;
  e->timer_args = timer_args; 
  e->cb = cb;
  /*element should be a real pointer*/
  RB_INSERT(proto_agent_map, &timer_instance.proto_agent_head, e); 
  
  LOG_I(PROTO_AGENT,"Created a new timer with id 0x%lx for agent %d, instance %d \n",
	e->timer_id, e->agent_id, e->instance);
 
  return 0; 
}

err_code_t proto_agent_destroy_timer(long timer_id){
  
  struct proto_agent_timer_element_s *e = get_timer_entry(timer_id);

  if (e != NULL ) {
    RB_REMOVE(proto_agent_map, &timer_instance.proto_agent_head, e);
    proto_agent_destroy_flexsplit_message(e->timer_args->msg);
    free(e);
  }
  
  if (timer_remove(timer_id) < 0 ) 
    goto error;
  
  return 0;

 error:
  LOG_E(PROTO_AGENT, "timer can't be removed\n");
  return TIMER_REMOVED_FAILED ;
}

err_code_t proto_agent_destroy_timer_by_task_id(xid_t xid) {
  struct proto_agent_timer_element_s *e = NULL;
  long timer_id;
  RB_FOREACH(e, proto_agent_map, &timer_instance.proto_agent_head) {
    if (e->xid == xid) {
      timer_id = e->timer_id;
      RB_REMOVE(proto_agent_map, &timer_instance.proto_agent_head, e);
      proto_agent_destroy_flexsplit_message(e->timer_args->msg);
      free(e);
      if (timer_remove(timer_id) < 0 ) { 
	goto error;
      }
    }
  }
  return 0;

 error:
  LOG_E(PROTO_AGENT, "timer can't be removed\n");
  return TIMER_REMOVED_FAILED ;
}

err_code_t proto_agent_destroy_timers(void){
  
  struct proto_agent_timer_element_s *e = NULL;
  
  RB_FOREACH(e, proto_agent_map, &timer_instance.proto_agent_head) {
    RB_REMOVE(proto_agent_map, &timer_instance.proto_agent_head, e);
    timer_remove(e->timer_id);
    proto_agent_destroy_flexsplit_message(e->timer_args->msg);
    free(e);
  }  

  return 0;

}

void proto_agent_sleep_until(struct timespec *ts, int delay) {
  ts->tv_nsec += delay;
  if(ts->tv_nsec >= 1000*1000*1000){
    ts->tv_nsec -= 1000*1000*1000;
    ts->tv_sec++;
  }
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ts,  NULL);
}

/*
 int i =0;
  RB_FOREACH(e, proto_agent_map, &proto_agent_head) {
    printf("%d: %p\n", i, e); i++;
    }
*/


err_code_t proto_agent_stop_timer(long timer_id){
  
  struct proto_agent_timer_element_s *e=NULL;
  struct proto_agent_timer_element_s search;
  memset(&search, 0, sizeof(struct proto_agent_timer_element_s));
  search.timer_id = timer_id;

  e = RB_FIND(proto_agent_map, &timer_instance.proto_agent_head, &search);

  if (e != NULL ) {
    e->state =  PROTO_AGENT_TIMER_STATE_STOPPED;
  }
  
  timer_remove(timer_id);
  
  return 0;
}

struct proto_agent_timer_element_s * get_timer_entry(long timer_id) {
  
  struct proto_agent_timer_element_s search;
  memset(&search, 0, sizeof(struct proto_agent_timer_element_s));
  search.timer_id = timer_id;

  return  RB_FIND(proto_agent_map, &timer_instance.proto_agent_head, &search); 
}

/*
// this will change the timer_id
err_code_t enb_agent_restart_timer(uint32_t *timer_id){
  
  struct enb_agent_timer_element_s *e=NULL;
    
  RB_FOREACH(e, enb_agent_map, &enb_agent_head) {
    if (e->timer_id == timer_id)
      break;
  }

  if (e != NULL ) {
    e->state =  ENB_AGENT_TIMER_STATE_ACTIVE;
  }
  
  ret = timer_setup(e->interval_sec, 
		    e->interval_usec, 
		    e->agent_id, 
		    e->instance, 
		    e->type,
		    e->timer_args,
		    &timer_id);
    
  }

  if (ret < 0 ) {
    return PROTOCOL__PROGRAN_ERR__TIMER_SETUP_FAILED; 
  }
  
  return 0;

}

*/

