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

/*! \file enb_agent_common.h
 * \brief common message primitves and utilities 
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */



#ifndef ENB_AGENT_COMMON_H_
#define ENB_AGENT_COMMON_H_


#include "header.pb-c.h"
#include "progran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"

# include "enb_agent_defs.h"

#include "LAYER2/MAC/extern.h"
#include "LAYER2/RLC/rlc.h"

# include "tree.h"
# include "intertask_interface.h"
# include "timer.h"

#define PROGRAN_VERSION 0

typedef int (*enb_agent_message_decoded_callback)(
	mid_t mod_id,
        xid_t xid,
	const void *params,
	Protocol__ProgranMessage **msg
);

typedef int (*enb_agent_message_destruction_callback)(
	Protocol__ProgranMessage *msg
);

int enb_agent_serialize_message(Protocol__ProgranMessage *msg, void **buf, int *size);
int enb_agent_deserialize_message(void *data, int size, Protocol__ProgranMessage **msg);

int prp_create_header(xid_t xid, Protocol__PrpType type, Protocol__PrpHeader **header);

int enb_agent_hello(mid_t mod_id, xid_t xid, const void *params, Protocol__ProgranMessage **msg);
int enb_agent_destroy_hello(Protocol__ProgranMessage *msg);

int enb_agent_echo_request(mid_t mod_id, xid_t xid, const void *params, Protocol__ProgranMessage **msg);
int enb_agent_destroy_echo_request(Protocol__ProgranMessage *msg);

int enb_agent_echo_reply(mid_t mod_id, xid_t xid, const void *params, Protocol__ProgranMessage **msg);
int enb_agent_destroy_echo_reply(Protocol__ProgranMessage *msg);


Protocol__ProgranMessage* enb_agent_handle_message (mid_t mod_id, 
						    xid_t xid, 
						    uint8_t *data, 
						    uint32_t size);

void * enb_agent_send_message(xid_t xid, 
			      Protocol__ProgranMessage *msg, 
			      uint32_t * size);






/****************************
 * get generic info from RAN
 ****************************/

void set_enb_vars(mid_t mod_id, ran_name_t ran);

int get_current_time_ms (mid_t mod_id, int subframe_flag);

int get_current_frame(mid_t mod_id);

int get_current_subframe(mid_t mod_id);

int get_num_ues(mid_t mod_id);

int get_ue_crnti (mid_t mod_id, mid_t ue_id);

int get_ue_bsr (mid_t mod_id, mid_t ue_id, lcid_t lcid);

int get_ue_phr (mid_t mod_id, mid_t ue_id);

int get_ue_wcqi (mid_t mod_id, mid_t ue_id);






/*******************
 * timer primitves
 *******************/

#define TIMER_NULL                 -1 
#define TIMER_TYPE_INVALIDE        -2
#define	TIMER_SETUP_FAILED         -3
#define	TIMER_REMOVED_FAILED       -4
#define	TIMER_ELEMENT_NOT_FOUND    -5


/* Type of the callback executed when the timer expired */
typedef err_code_t (*enb_agent_timer_callback_t)(void*);

typedef enum {
  /* oneshot timer:  */
  ENB_AGENT_TIMER_TYPE_ONESHOT = 0x0,

  /* periodic timer  */
  ENB_AGENT_TIMER_TYPE_PERIODIC = 0x1,

  /* Inactive state: initial state for any timer. */
  ENB_AGENT_TIMER_TYPE_EVENT_DRIVEN = 0x2,
  
  /* Max number of states available */
  ENB_AGENT_TIMER_TYPE_MAX,
} eNB_agent_timer_type_t;

typedef enum {
  /* Inactive state: initial state for any timer. */
  ENB_AGENT_TIMER_STATE_INACTIVE = 0x0,

  /* Inactive state: initial state for any timer. */
  ENB_AGENT_TIMER_STATE_ACTIVE = 0x1,

  /* Inactive state: initial state for any timer. */
  ENB_AGENT_TIMER_STATE_STOPPED = 0x2,
  
  /* Max number of states available */
  ENB_AGENT_TIMER_STATE_MAX,
} eNB_agent_timer_state_t;

typedef struct enb_agent_timer_args_s{
  mid_t            mod_id;

  agent_action_t   cc_actions;
  uint32_t         cc_report_flags;

  agent_action_t   ue_actions;
  uint32_t         ue_report_flags;

} enb_agent_timer_args_t;



typedef struct enb_agent_timer_element_s{
  RB_ENTRY(enb_agent_timer_element_s) entry;

  agent_id_t             agent_id;
  instance_t       instance;
  
  eNB_agent_timer_type_t  type;
  eNB_agent_timer_state_t state;

  uint32_t interval_sec;
  uint32_t interval_usec;

  long timer_id;  /* Timer id returned by the timer API*/
  
  enb_agent_timer_callback_t cb;
  // void* timer_args;
  
} enb_agent_timer_element_t;

typedef struct enb_agent_timer_instance_s{
  RB_HEAD(enb_agent_map, enb_agent_timer_element_s) enb_agent_head;
}enb_agent_timer_instance_t;

err_code_t enb_agent_init_timer(void);

err_code_t enb_agent_create_timer(uint32_t interval_sec,
				  uint32_t interval_usec,
				  agent_id_t     agent_id,
				  instance_t     instance,
				  uint32_t timer_type,
				  enb_agent_timer_callback_t cb,
				  void*    timer_args,
				  long *timer_id);

err_code_t enb_agent_destroy_timers(void);
err_code_t enb_agent_destroy_timer(long timer_id);

err_code_t enb_agent_stop_timer(long timer_id);

err_code_t enb_agent_restart_timer(long *timer_id);

struct enb_agent_timer_element_s * get_timer_entry(long timer_id);



err_code_t enb_agent_process_timeout(long timer_id, void* timer_args);

int enb_agent_compare_timer(struct enb_agent_timer_element_s *a, struct enb_agent_timer_element_s *b);

/* RB_PROTOTYPE is for .h files */
RB_PROTOTYPE(enb_agent_map, enb_agent_timer_element_s, entry, enb_agent_compare_timer);


#endif







