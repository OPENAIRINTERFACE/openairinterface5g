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
 * \author Xenofon Foukas, Mohamed Kassem and Navid Nikaein
 * \date 2016
 * \version 0.1
 */



#ifndef ENB_AGENT_COMMON_H_
#define ENB_AGENT_COMMON_H_

#include <time.h>

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"

# include "enb_agent_defs.h"
#include "enb_config.h"

#include "LAYER2/MAC/extern.h"
#include "LAYER2/RLC/rlc.h"

# include "tree.h"
# include "intertask_interface.h"
# include "timer.h"

#define FLEXRAN_VERSION 0

typedef int (*enb_agent_message_decoded_callback)(
	mid_t mod_id,
       	const void *params,
	Protocol__FlexranMessage **msg
);

typedef int (*enb_agent_message_destruction_callback)(
	Protocol__FlexranMessage *msg
);

/**********************************
 * FlexRAN protocol messages helper 
 * functions and generic handlers
 **********************************/

int enb_agent_serialize_message(Protocol__FlexranMessage *msg, void **buf, int *size);
int enb_agent_deserialize_message(void *data, int size, Protocol__FlexranMessage **msg);

void * enb_agent_pack_message(Protocol__FlexranMessage *msg, 
			      uint32_t * size);

err_code_t enb_agent_destroy_flexran_message(Protocol__FlexranMessage *msg);

int flex_create_header(xid_t xid, Protocol__FlexType type, Protocol__FlexHeader **header);

int enb_agent_hello(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int enb_agent_destroy_hello(Protocol__FlexranMessage *msg);

int enb_agent_echo_request(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int enb_agent_destroy_echo_request(Protocol__FlexranMessage *msg);

int enb_agent_echo_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int enb_agent_destroy_echo_reply(Protocol__FlexranMessage *msg);

int enb_agent_enb_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int enb_agent_destroy_enb_config_reply(Protocol__FlexranMessage *msg);

int enb_agent_ue_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int enb_agent_destroy_ue_config_reply(Protocol__FlexranMessage *msg);

int enb_agent_lc_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int enb_agent_destroy_lc_config_reply(Protocol__FlexranMessage *msg);

int enb_agent_enb_config_request(mid_t mod_id, const void* params, Protocol__FlexranMessage **msg);
int enb_agent_destroy_enb_config_request(Protocol__FlexranMessage *msg);

int enb_agent_destroy_ue_config_request(Protocol__FlexranMessage *msg);

int enb_agent_destroy_lc_config_request(Protocol__FlexranMessage *msg);

int enb_agent_ue_state_change(mid_t mod_id, uint32_t rnti, uint8_t state_change);
int enb_agent_destroy_ue_state_change(Protocol__FlexranMessage *msg);

int enb_agent_control_delegation(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int enb_agent_destroy_control_delegation(Protocol__FlexranMessage *msg);

int enb_agent_reconfiguration(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int enb_agent_destroy_agent_reconfiguration(Protocol__FlexranMessage *msg);

Protocol__FlexranMessage* enb_agent_handle_message (mid_t mod_id, 
						    uint8_t *data, 
						    uint32_t size);

Protocol__FlexranMessage *enb_agent_handle_timed_task(void *args);




/****************************
 * get generic info from RAN
 ****************************/

void set_enb_vars(mid_t mod_id, ran_name_t ran);

int get_current_time_ms (mid_t mod_id, int subframe_flag);

/*Return the current frame number
 *Could be using implementation specific numbering of frames
 */
unsigned int get_current_frame(mid_t mod_id);

/*Return the current SFN (0-1023)*/ 
unsigned int get_current_system_frame_num(mid_t mod_id);

unsigned int get_current_subframe(mid_t mod_id);

/*Return the frame and subframe number in compact 16-bit format.
  Bits 0-3 subframe, rest for frame. Required by FlexRAN protocol*/
uint16_t get_sfn_sf (mid_t mod_id);

uint16_t get_future_sfn_sf(mid_t mod_id, int ahead_of_time);

int get_num_ues(mid_t mod_id);

int get_ue_crnti (mid_t mod_id, mid_t ue_id);

int get_ue_bsr (mid_t mod_id, mid_t ue_id, lcid_t lcid);

int get_ue_phr (mid_t mod_id, mid_t ue_id);

int get_ue_wcqi (mid_t mod_id, mid_t ue_id);

int get_tx_queue_size(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id);

int get_MAC_CE_bitmap_TA(mid_t mod_id, mid_t ue_id, int CC_id);

int get_active_CC(mid_t mod_id, mid_t ue_id);

int get_current_RI(mid_t mod_id, mid_t ue_id, int CC_id);

int get_n1pucch_an(mid_t mod_id, int CC_id);

int get_nRB_CQI(mid_t mod_id, int CC_id);

int get_deltaPUCCH_Shift(mid_t mod_id, int CC_id);

int get_prach_ConfigIndex(mid_t mod_id, int CC_id);

int get_prach_FreqOffset(mid_t mod_id, int CC_id);

int get_maxHARQ_Msg3Tx(mid_t mod_id, int CC_id);

int get_ul_cyclic_prefix_length(mid_t mod_id, int CC_id);

int get_dl_cyclic_prefix_length(mid_t mod_id, int CC_id);

int get_cell_id(mid_t mod_id, int CC_id);

int get_srs_BandwidthConfig(mid_t mod_id, int CC_id);

int get_srs_SubframeConfig(mid_t mod_id, int CC_id);

int get_srs_MaxUpPts(mid_t mod_id, int CC_id);

int get_N_RB_DL(mid_t mod_id, int CC_id);

int get_N_RB_UL(mid_t mod_id, int CC_id);

int get_subframe_assignment(mid_t mod_id, int CC_id);

int get_special_subframe_assignment(mid_t mod_id, int CC_id);

int get_ra_ResponseWindowSize(mid_t mod_id, int CC_id);

int get_mac_ContentionResolutionTimer(mid_t mod_id, int CC_id);

int get_duplex_mode(mid_t mod_id, int CC_id);

long get_si_window_length(mid_t mod_id, int CC_id);

int get_num_pdcch_symb(mid_t mod_id, int CC_id);

int get_tpc(mid_t mod_id, mid_t ue_id);

int get_harq(const mid_t mod_id, const uint8_t CC_id, const mid_t ue_id,
	     const int frame, const uint8_t subframe, int *id, int *status);

int get_p0_pucch_dbm(mid_t mod_id, mid_t ue_id, int CC_id);

int get_p0_nominal_pucch(mid_t mod_id, int CC_id);

/*
 * ************************************
 * Get Messages for UE Configuration Reply
 * ************************************
 */

int get_time_alignment_timer(mid_t mod_id, mid_t ue_id);

int get_meas_gap_config(mid_t mod_id, mid_t ue_id);

int get_meas_gap_config_offset(mid_t mod_id, mid_t ue_id);

int get_ue_aggregated_max_bitrate_dl (mid_t mod_id, mid_t ue_id);

int get_ue_aggregated_max_bitrate_ul (mid_t mod_id, mid_t ue_id);

int get_half_duplex(mid_t ue_id);

int get_intra_sf_hopping(mid_t ue_id);

int get_type2_sb_1(mid_t ue_id);

int get_ue_category(mid_t ue_id);

int get_res_alloc_type1(mid_t ue_id);

int get_ue_transmission_mode(mid_t mod_id, mid_t ue_id);

int get_tti_bundling(mid_t mod_id, mid_t ue_id);

int get_maxHARQ_TX(mid_t mod_id, mid_t ue_id);

int get_beta_offset_ack_index(mid_t mod_id, mid_t ue_id);

int get_beta_offset_ri_index(mid_t mod_id, mid_t ue_id);

int get_beta_offset_cqi_index(mid_t mod_id, mid_t ue_id);

int get_simultaneous_ack_nack_cqi(mid_t mod_id, mid_t ue_id);

int get_ack_nack_simultaneous_trans(mid_t mod_id,mid_t ue_id);

int get_aperiodic_cqi_rep_mode(mid_t mod_id,mid_t ue_id);

int get_tdd_ack_nack_feedback(mid_t mod_id, mid_t ue_id);

int get_ack_nack_repetition_factor(mid_t mod_id, mid_t ue_id);

int get_extended_bsr_size(mid_t mod_id, mid_t ue_id);

int get_ue_transmission_antenna(mid_t mod_id, mid_t ue_id);

int get_lcg(mid_t ue_id, mid_t lc_id);

int get_direction(mid_t ue_id, mid_t lc_id);

/*******************
 * timer primitves
 *******************/

#define TIMER_NULL                 -1 
#define TIMER_TYPE_INVALIDE        -2
#define	TIMER_SETUP_FAILED         -3
#define	TIMER_REMOVED_FAILED       -4
#define	TIMER_ELEMENT_NOT_FOUND    -5


/* Type of the callback executed when the timer expired */
typedef Protocol__FlexranMessage *(*enb_agent_timer_callback_t)(void*);

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
  Protocol__FlexranMessage *msg;
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
  xid_t xid; /*The id of the task as received by the controller
	       message*/
  
  enb_agent_timer_callback_t cb;
  enb_agent_timer_args_t *timer_args;
  
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
				  xid_t xid,
				  enb_agent_timer_callback_t cb,
				  void*    timer_args,
				  long *timer_id);

err_code_t enb_agent_destroy_timers(void);
err_code_t enb_agent_destroy_timer(long timer_id);
err_code_t enb_agent_destroy_timer_by_task_id(xid_t xid);

err_code_t enb_agent_stop_timer(long timer_id);

err_code_t enb_agent_restart_timer(long *timer_id);

struct enb_agent_timer_element_s * get_timer_entry(long timer_id);

Protocol__FlexranMessage * enb_agent_process_timeout(long timer_id, void* timer_args);

int enb_agent_compare_timer(struct enb_agent_timer_element_s *a, struct enb_agent_timer_element_s *b);

/*Specify a delay in nanoseconds to timespec and sleep until then*/
void enb_agent_sleep_until(struct timespec *ts, int delay);

/* RB_PROTOTYPE is for .h files */
RB_PROTOTYPE(enb_agent_map, enb_agent_timer_element_s, entry, enb_agent_compare_timer);


#endif







