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

/*! \file enb_agent_common.c
 * \brief common primitives for all agents 
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */


#include "enb_agent_common.h"
#include "log.h"


void * enb[NUM_MAX_ENB_AGENT];
void * enb_ue[NUM_MAX_ENB_AGENT];
/*
 * message primitives
 */

int enb_agent_serialize_message(Protocol__ProgranMessage *msg, void **buf, int *size) {

  *size = protocol__progran_message__get_packed_size(msg);
  
  *buf = malloc(*size);
  if (buf == NULL)
    goto error;
  
  protocol__progran_message__pack(msg, *buf);
  
  return 0;
  
 error:
  LOG_E(ENB_AGENT, "an error occured\n"); // change the com
  return -1;
}



/* We assume that the buffer size is equal to the message size.
   Should be chekced durint Tx/Rx */
int enb_agent_deserialize_message(void *data, int size, Protocol__ProgranMessage **msg) {
  *msg = protocol__progran_message__unpack(NULL, size, data);
  if (*msg == NULL)
    goto error;

  return 0;
  
 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}



int prp_create_header(xid_t xid, Protocol__PrpType type,  Protocol__PrpHeader **header) {
  
  *header = malloc(sizeof(Protocol__PrpHeader));
  if(*header == NULL)
    goto error;
  
  protocol__prp_header__init(*header);
  (*header)->version = PROGRAN_VERSION;
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


int enb_agent_hello(mid_t mod_id, xid_t xid, const void *params, Protocol__ProgranMessage **msg) {
 
  Protocol__PrpHeader *header;
  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_HELLO, &header) != 0)
    goto error;

  Protocol__PrpHello *hello_msg;
  hello_msg = malloc(sizeof(Protocol__PrpHello));
  if(hello_msg == NULL)
    goto error;
  protocol__prp_hello__init(hello_msg);
  hello_msg->header = header;

  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_HELLO_MSG;
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__INITIATING_MESSAGE;
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


int enb_agent_destroy_hello(Protocol__ProgranMessage *msg) {
  
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_HELLO_MSG)
    goto error;
  
  free(msg->hello_msg->header);
  free(msg->hello_msg);
  free(msg);
  return 0;

 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int enb_agent_echo_request(mid_t mod_id, xid_t xid, const void* params, Protocol__ProgranMessage **msg) {
  Protocol__PrpHeader *header;
  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_ECHO_REQUEST, &header) != 0)
    goto error;

  Protocol__PrpEchoRequest *echo_request_msg;
  echo_request_msg = malloc(sizeof(Protocol__PrpEchoRequest));
  if(echo_request_msg == NULL)
    goto error;
  protocol__prp_echo_request__init(echo_request_msg);
  echo_request_msg->header = header;

  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REQUEST_MSG;
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->echo_request_msg = echo_request_msg;
  return 0;

 error:
  if(header != NULL)
    free(header);
  if(echo_request_msg != NULL)
    free(echo_request_msg);
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int enb_agent_destroy_echo_request(Protocol__ProgranMessage *msg) {
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REQUEST_MSG)
    goto error;
  
  free(msg->echo_request_msg->header);
  free(msg->echo_request_msg);
  free(msg);
  return 0;
  
 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}



int enb_agent_echo_reply(mid_t mod_id, xid_t xid, const void *params, Protocol__ProgranMessage **msg) {
  Protocol__PrpHeader *header;
  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_ECHO_REPLY, &header) != 0)
    goto error;

  Protocol__PrpEchoReply *echo_reply_msg;
  echo_reply_msg = malloc(sizeof(Protocol__PrpEchoReply));
  if(echo_reply_msg == NULL)
    goto error;
  protocol__prp_echo_reply__init(echo_reply_msg);
  echo_reply_msg->header = header;

  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->has_msg_dir = 1;
  (*msg)->echo_reply_msg = echo_reply_msg;
  return 0;

 error:
  if(header != NULL)
    free(header);
  if(echo_reply_msg != NULL)
    free(echo_reply_msg);
  if(*msg != NULL)
    free(*msg);
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int enb_agent_destroy_echo_reply(Protocol__ProgranMessage *msg) {
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REPLY_MSG)
    goto error;
  
  free(msg->echo_reply_msg->header);
  free(msg->echo_reply_msg);
  free(msg);
  return 0;
  
 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

/*
 * get generic info from RAN
 */

void set_enb_vars(mid_t mod_id, ran_name_t ran){

  switch (ran){
  case RAN_LTE_OAI :
    enb[mod_id] =  (void *)&eNB_mac_inst[mod_id];
    enb_ue[mod_id] = (void *)&eNB_mac_inst[mod_id].UE_list;
    break;
  default :
    goto error;
  }

  return; 

 error:
  LOG_E(ENB_AGENT, "unknown RAN name %d\n", ran);
}

int get_current_time_ms (mid_t mod_id, int subframe_flag){

  if (subframe_flag == 1){
    return ((eNB_MAC_INST *)enb[mod_id])->frame*10 + ((eNB_MAC_INST *)enb[mod_id])->subframe;
  }else {
    return ((eNB_MAC_INST *)enb[mod_id])->frame*10;
  }
   
}

int get_num_ues (mid_t mod_id){

  return  ((UE_list_t *)enb_ue[mod_id])->num_UEs;
}

int get_ue_crnti (mid_t mod_id, mid_t ue_id){

  return  ((UE_list_t *)enb_ue[mod_id])->eNB_UE_stats[UE_PCCID(mod_id,ue_id)][ue_id].crnti;
}

int get_ue_bsr (mid_t mod_id, mid_t ue_id, lcid_t lcid) {

  return ((UE_list_t *)enb_ue[mod_id])->UE_template[UE_PCCID(mod_id,ue_id)][ue_id].bsr_info[lcid];
}

int get_ue_phr (mid_t mod_id, mid_t ue_id) {

  return ((UE_list_t *)enb_ue[mod_id])->UE_template[UE_PCCID(mod_id,ue_id)][ue_id].phr_info;
}

int get_ue_wcqi (mid_t mod_id, mid_t ue_id) {

  return ((UE_list_t *)enb_ue[mod_id])->eNB_UE_stats[UE_PCCID(mod_id,ue_id)][ue_id].dl_cqi;
}
/*
 * timer primitives
 */


//struct enb_agent_map agent_map;
enb_agent_timer_instance_t timer_instance;
err_code_t enb_agent_init_timer(void){
  
  LOG_I(ENB_AGENT, "init RB tree\n");

  RB_INIT(&timer_instance.enb_agent_head);
 
  /*
    struct enb_agent_timer_element_s e;
  memset(&e, 0, sizeof(enb_agent_timer_element_t));
  RB_INSERT(enb_agent_map, &agent_map, &e); 
  */
 return PROTOCOL__PROGRAN_ERR__NO_ERR;
}

RB_GENERATE(enb_agent_map,enb_agent_timer_element_s, entry, enb_agent_compare_timer);

/* The timer_id might not be the best choice for the comparison */
int enb_agent_compare_timer(struct enb_agent_timer_element_s *a, struct enb_agent_timer_element_s *b){

  if (a->timer_id < b->timer_id) return -1;
  if (a->timer_id > b->timer_id) return 1;

  // equal timers
  return 0;
}

err_code_t enb_agent_create_timer(uint32_t interval_sec,
				  uint32_t interval_usec,
				  agent_id_t     agent_id,
				  instance_t     instance,
				  uint32_t timer_type,
				  enb_agent_timer_callback_t cb,
				  void*    timer_args,
				  long *timer_id){
  
  struct enb_agent_timer_element_s *e = calloc(1, sizeof(*e));
  DevAssert(e != NULL);
  
//uint32_t timer_id;
  int ret=-1;
  
  if ((interval_sec == 0) && (interval_usec == 0 ))
    return TIMER_NULL;
  
  if (timer_type >= ENB_AGENT_TIMER_TYPE_MAX)
    return TIMER_TYPE_INVALIDE;
  
  if (timer_type  ==   ENB_AGENT_TIMER_TYPE_ONESHOT){ 
    ret = timer_setup(interval_sec, 
		      interval_usec, 
		      TASK_ENB_AGENT, 
		      instance, 
		      TIMER_ONE_SHOT,
		      timer_args,
		      timer_id);
    
    e->type = TIMER_ONE_SHOT;
  }
  else if (timer_type  ==   ENB_AGENT_TIMER_TYPE_PERIODIC ){
    ret = timer_setup(interval_sec, 
		      interval_usec, 
		      TASK_ENB_AGENT, 
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
  e->state = ENB_AGENT_TIMER_STATE_ACTIVE;
  e->timer_id = *timer_id;
  //  e->timer_args = timer_args; 
  e->cb = cb;
  /*element should be a real pointer*/
  RB_INSERT(enb_agent_map, &timer_instance.enb_agent_head, e); 
  
  LOG_I(ENB_AGENT,"Created a new timer with id 0x%lx for agent %d, instance %d \n",
	e->timer_id, e->agent_id, e->instance);
 
  return 0; 
}

err_code_t enb_agent_destroy_timer(long timer_id){
  
  struct enb_agent_timer_element_s *e = get_timer_entry(timer_id);

  if (e != NULL ) {
    RB_REMOVE(enb_agent_map, &timer_instance.enb_agent_head, &e);
    free(e);
  }
  
  if (timer_remove(timer_id) < 0 ) 
    goto error;
  
  return 0;

 error:
  LOG_E(ENB_AGENT, "timer can't be removed\n");
  return TIMER_REMOVED_FAILED ;
}

err_code_t enb_agent_destroy_timers(void){
  
  struct enb_agent_timer_element_s *e = NULL;
  
  RB_FOREACH(e, enb_agent_map, &timer_instance.enb_agent_head) {
    RB_REMOVE(enb_agent_map, &timer_instance.enb_agent_head, e);
    timer_remove(e->timer_id); 
    free(e);
  }  

  return 0;

}

struct enb_agent_timer_element_s * get_timer_entry(long timer_id) {
  
  struct enb_agent_timer_element_s *search= calloc(1,sizeof(*search));
  search->timer_id = timer_id;

  return  RB_FIND(enb_agent_map, &timer_instance.enb_agent_head, search); 
}

/*
 int i =0;
  RB_FOREACH(e, enb_agent_map, &enb_agent_head) {
    printf("%d: %p\n", i, e); i++;
    }
*/

/*
err_code_t enb_agent_stop_timer(uint32_t timer_id){
  
  struct enb_agent_timer_element_s *e=NULL;
  
  RB_FOREACH(e, enb_agent_map, &enb_agent_head) {
    if (e->timer_id == timer_id)
      break;
  }

  if (e != NULL ) {
    e->state =  ENB_AGENT_TIMER_STATE_STOPPED;
  }
  
  timer_remove(timer_id);
  
  return 0;

}

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
