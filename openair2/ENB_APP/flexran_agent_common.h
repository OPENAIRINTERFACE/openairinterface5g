/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file flexran_agent_common.h
 * \brief common message primitves and utilities 
 * \author Xenofon Foukas, Mohamed Kassem and Navid Nikaein
 * \date 2016
 * \version 0.1
 */



#ifndef FLEXRAN_AGENT_COMMON_H_
#define FLEXRAN_AGENT_COMMON_H_

#include <time.h>

#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"

#include "flexran_agent_defs.h"
#include "enb_config.h"

#include "LAYER2/MAC/extern.h"
#include "LAYER2/RLC/rlc.h"

# include "tree.h"
# include "intertask_interface.h"
# include "timer.h"

#define FLEXRAN_VERSION 0

typedef int (*flexran_agent_message_decoded_callback)(
	mid_t mod_id,
       	const void *params,
	Protocol__FlexranMessage **msg
);

typedef int (*flexran_agent_message_destruction_callback)(
	Protocol__FlexranMessage *msg
);

/**********************************
 * FlexRAN protocol messages helper 
 * functions and generic handlers
 **********************************/

/* Helper functions for message (de)serialization */
int flexran_agent_serialize_message(Protocol__FlexranMessage *msg, void **buf, int *size);
int flexran_agent_deserialize_message(void *data, int size, Protocol__FlexranMessage **msg);

/* Serialize message and then destroy the input flexran msg. Should be called when protocol
   message is created dynamically */
void * flexran_agent_pack_message(Protocol__FlexranMessage *msg, 
			      int * size);

/* Calls destructor of the given message */
err_code_t flexran_agent_destroy_flexran_message(Protocol__FlexranMessage *msg);

/* Function to create the header for any FlexRAN protocol message */
int flexran_create_header(xid_t xid, Protocol__FlexType type, Protocol__FlexHeader **header);

/* Hello protocol message constructor and destructor */
int flexran_agent_hello(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_hello(Protocol__FlexranMessage *msg);

/* Echo request protocol message constructor and destructor */
int flexran_agent_echo_request(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_echo_request(Protocol__FlexranMessage *msg);

/* Echo reply protocol message constructor and destructor */
int flexran_agent_echo_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_echo_reply(Protocol__FlexranMessage *msg);

/* eNodeB configuration reply message constructor and destructor */
int flexran_agent_enb_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_enb_config_reply(Protocol__FlexranMessage *msg);

/* UE configuration reply message constructor and destructor */
int flexran_agent_ue_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_ue_config_reply(Protocol__FlexranMessage *msg);

/* Logical channel reply configuration message constructor and destructor */
int flexran_agent_lc_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_lc_config_reply(Protocol__FlexranMessage *msg);

/* eNodeB configuration request message constructor and destructor */
int flexran_agent_enb_config_request(mid_t mod_id, const void* params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_enb_config_request(Protocol__FlexranMessage *msg);

/* UE configuration request message constructor */
/* TODO: Need to define and implement destructor */
int flexran_agent_destroy_ue_config_request(Protocol__FlexranMessage *msg);

/* Logical channel configuration request message constructor */
/* TODO: Need to define and implement destructor */
int flexran_agent_destroy_lc_config_request(Protocol__FlexranMessage *msg);

/* UE state change message constructor and destructor */
int flexran_agent_ue_state_change(mid_t mod_id, uint32_t rnti, uint8_t state_change);
int flexran_agent_destroy_ue_state_change(Protocol__FlexranMessage *msg);

/* Control delegation message constructor and destructor */
int flexran_agent_control_delegation(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_control_delegation(Protocol__FlexranMessage *msg);

/* Policy reconfiguration message constructor and destructor */
int flexran_agent_reconfiguration(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg);
int flexran_agent_destroy_agent_reconfiguration(Protocol__FlexranMessage *msg);

/* FlexRAN protocol message dispatcher function */
Protocol__FlexranMessage* flexran_agent_handle_message (mid_t mod_id, 
						    uint8_t *data, 
						    uint32_t size);

/* Function to be used to send a message to a dispatcher once the appropriate event is triggered. */
Protocol__FlexranMessage *flexran_agent_handle_timed_task(void *args);




/****************************
 * get generic info from RAN
 ****************************/

void flexran_set_enb_vars(mid_t mod_id, ran_name_t ran);

int flexran_get_current_time_ms (mid_t mod_id, int subframe_flag);

/*Return the current frame number
 *Could be using implementation specific numbering of frames
 */
unsigned int flexran_get_current_frame(mid_t mod_id);

/*Return the current SFN (0-1023)*/ 
unsigned int flexran_get_current_system_frame_num(mid_t mod_id);

unsigned int flexran_get_current_subframe(mid_t mod_id);

/*Return the frame and subframe number in compact 16-bit format.
  Bits 0-3 subframe, rest for frame. Required by FlexRAN protocol*/
uint16_t flexran_get_sfn_sf (mid_t mod_id);

/* Return a future frame and subframe number that is ahead_of_time
   subframes later in compact 16-bit format. Bits 0-3 subframe,
   rest for frame */
uint16_t flexran_get_future_sfn_sf(mid_t mod_id, int ahead_of_time);

/* Return the number of attached UEs */
int flexran_get_num_ues(mid_t mod_id);

/* Get the rnti of a UE with id ue_id */
int flexran_get_ue_crnti (mid_t mod_id, mid_t ue_id);

/* Get the RLC buffer status report of a ue for a designated
   logical channel id */
int flexran_get_ue_bsr (mid_t mod_id, mid_t ue_id, lcid_t lcid);

/* Get power headroom of UE with id ue_id */
int flexran_get_ue_phr (mid_t mod_id, mid_t ue_id);

/* Get the UE wideband CQI */
int flexran_get_ue_wcqi (mid_t mod_id, mid_t ue_id);

/* Get the transmission queue size for a UE with a channel_id logical channel id */
int flexran_get_tx_queue_size(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id);

/* Get the head of line delay for a UE with a channel_id logical channel id */
int flexran_get_hol_delay(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id);

/* Check the status of the timing advance for a UE */
short flexran_get_TA(mid_t mod_id, mid_t ue_id, int CC_id);

/* Update the timing advance status (find out whether a timing advance command is required) */
void flexran_update_TA(mid_t mod_id, mid_t ue_id, int CC_id);

/* Return timing advance MAC control element for a designated cell and UE */
int flexran_get_MAC_CE_bitmap_TA(mid_t mod_id, mid_t ue_id, int CC_id);

/* Get the number of active component carriers for a specific UE */
int flexran_get_active_CC(mid_t mod_id, mid_t ue_id);

/* Get the rank indicator for a designated cell and UE */
int flexran_get_current_RI(mid_t mod_id, mid_t ue_id, int CC_id);

/* See TS 36.213, section 10.1 */
int flexran_get_n1pucch_an(mid_t mod_id, int CC_id);

/* See TS 36.211, section 5.4 */
int flexran_get_nRB_CQI(mid_t mod_id, int CC_id);

/* See TS 36.211, section 5.4 */
int flexran_get_deltaPUCCH_Shift(mid_t mod_id, int CC_id);

/* See TS 36.211, section 5.7.1 */
int flexran_get_prach_ConfigIndex(mid_t mod_id, int CC_id);

/* See TS 36.211, section 5.7.1 */
int flexran_get_prach_FreqOffset(mid_t mod_id, int CC_id);

/* See TS 36.321 */
int flexran_get_maxHARQ_Msg3Tx(mid_t mod_id, int CC_id);

/* Get the length of the UL cyclic prefix */
int flexran_get_ul_cyclic_prefix_length(mid_t mod_id, int CC_id);

/* Get the length of the DL cyclic prefix */
int flexran_get_dl_cyclic_prefix_length(mid_t mod_id, int CC_id);

/* Get the physical cell id of a cell */
int flexran_get_cell_id(mid_t mod_id, int CC_id);

/* See TS 36.211, section 5.5.3.2 */
int flexran_get_srs_BandwidthConfig(mid_t mod_id, int CC_id);

/* See TS 36.211, table 5.5.3.3-1 and 2 */
int flexran_get_srs_SubframeConfig(mid_t mod_id, int CC_id);

/* Boolean value. See TS 36.211,
   section 5.5.3.2. TDD only */
int flexran_get_srs_MaxUpPts(mid_t mod_id, int CC_id);

/* Get number of DL resource blocks */
int flexran_get_N_RB_DL(mid_t mod_id, int CC_id);

/* Get number of UL resource blocks */
int flexran_get_N_RB_UL(mid_t mod_id, int CC_id);

/* Get number of resource block groups */
int flexran_get_N_RBG(mid_t mod_id, int CC_id);

/* Get DL/UL subframe assignment. TDD only */
int flexran_get_subframe_assignment(mid_t mod_id, int CC_id);

/* TDD only. See TS 36.211, table 4.2.1 */
int flexran_get_special_subframe_assignment(mid_t mod_id, int CC_id);

/* Get the duration of the random access response window in subframes */
int flexran_get_ra_ResponseWindowSize(mid_t mod_id, int CC_id);

/* Get timer used for random access */
int flexran_get_mac_ContentionResolutionTimer(mid_t mod_id, int CC_id);

/* Get type of duplex mode (FDD/TDD) */
int flexran_get_duplex_mode(mid_t mod_id, int CC_id);

/* Get the SI window length */
long flexran_get_si_window_length(mid_t mod_id, int CC_id);

/* Get the number of PDCCH symbols configured for the cell */
int flexran_get_num_pdcch_symb(mid_t mod_id, int CC_id);

/* See TS 36.213, sec 5.1.1.1 */
int flexran_get_tpc(mid_t mod_id, mid_t ue_id);

/* Get the first available HARQ process for a specific cell and UE during 
   a designated frame and subframe. Returns 0 for success. The id and the 
   status of the HARQ process are stored in id and status respectively */
int flexran_get_harq(const mid_t mod_id, const uint8_t CC_id, const mid_t ue_id,
		     const int frame, const uint8_t subframe, unsigned char *id, unsigned char *round);

/* Uplink power control management*/
int flexran_get_p0_pucch_dbm(mid_t mod_id, mid_t ue_id, int CC_id);

int flexran_get_p0_nominal_pucch(mid_t mod_id, int CC_id);

int flexran_get_p0_pucch_status(mid_t mod_id, mid_t ue_id, int CC_id);

int flexran_update_p0_pucch(mid_t mod_id, mid_t ue_id, int CC_id);


/*
 * ************************************
 * Get Messages for UE Configuration Reply
 * ************************************
 */

/* Get timer in subframes. Controls the synchronization
   status of the UE, not the actual timing 
   advance procedure. See TS 36.321 */
int flexran_get_time_alignment_timer(mid_t mod_id, mid_t ue_id);

/* Get measurement gap configuration. See TS 36.133 */
int flexran_get_meas_gap_config(mid_t mod_id, mid_t ue_id);

/* Get measurement gap configuration offset if applicable */
int flexran_get_meas_gap_config_offset(mid_t mod_id, mid_t ue_id);

/* DL aggregated bit-rate of non-gbr bearer
   per UE. See TS 36.413 */
int flexran_get_ue_aggregated_max_bitrate_dl (mid_t mod_id, mid_t ue_id);

/* UL aggregated bit-rate of non-gbr bearer
   per UE. See TS 36.413 */
int flexran_get_ue_aggregated_max_bitrate_ul (mid_t mod_id, mid_t ue_id);

/* Only half-duplex support. FDD
   operation. Boolean value */
int flexran_get_half_duplex(mid_t ue_id);

/* Support of intra-subframe hopping.
   Boolean value */
int flexran_get_intra_sf_hopping(mid_t ue_id);

/* UE support for type 2 hopping with
   n_sb>1 */
int flexran_get_type2_sb_1(mid_t ue_id);

/* Get the UE category */
int flexran_get_ue_category(mid_t ue_id);

/* UE support for resource allocation
   type 1 */
int flexran_get_res_alloc_type1(mid_t ue_id);

/* Get UE transmission mode */
int flexran_get_ue_transmission_mode(mid_t mod_id, mid_t ue_id);

/* Boolean value. See TS 36.321 */
int flexran_get_tti_bundling(mid_t mod_id, mid_t ue_id);

/* The max HARQ retransmission for UL.
   See TS 36.321 */
int flexran_get_maxHARQ_TX(mid_t mod_id, mid_t ue_id);

/* See TS 36.213 */
int flexran_get_beta_offset_ack_index(mid_t mod_id, mid_t ue_id);

/* See TS 36.213 */
int flexran_get_beta_offset_ri_index(mid_t mod_id, mid_t ue_id);

/* See TS 36.213 */
int flexran_get_beta_offset_cqi_index(mid_t mod_id, mid_t ue_id);

/* Boolean. See TS36.213, Section 10.1 */
int flexran_get_simultaneous_ack_nack_cqi(mid_t mod_id, mid_t ue_id);

/* Boolean. See TS 36.213, Section 8.2 */
int flexran_get_ack_nack_simultaneous_trans(mid_t mod_id,mid_t ue_id);

/* Get aperiodic CQI report mode */
int flexran_get_aperiodic_cqi_rep_mode(mid_t mod_id,mid_t ue_id);

/* Get ACK/NACK feedback mode. TDD only */
int flexran_get_tdd_ack_nack_feedback(mid_t mod_id, mid_t ue_id);

/* See TS36.213, section 10.1 */
int flexran_get_ack_nack_repetition_factor(mid_t mod_id, mid_t ue_id);

/* Boolean. Extended buffer status report size */
int flexran_get_extended_bsr_size(mid_t mod_id, mid_t ue_id);

/* Get number of UE transmission antennas */
int flexran_get_ue_transmission_antenna(mid_t mod_id, mid_t ue_id);

/* Get logical channel group of a channel with id lc_id */
int flexran_get_lcg(mid_t ue_id, mid_t lc_id);

/* Get direction of logical channel with id lc_id */
int flexran_get_direction(mid_t ue_id, mid_t lc_id);

/*******************
 * timer primitves
 *******************/

#define TIMER_NULL                 -1 
#define TIMER_TYPE_INVALIDE        -2
#define	TIMER_SETUP_FAILED         -3
#define	TIMER_REMOVED_FAILED       -4
#define	TIMER_ELEMENT_NOT_FOUND    -5


/* Type of the callback executed when the timer expired */
typedef Protocol__FlexranMessage *(*flexran_agent_timer_callback_t)(void*);

typedef enum {
  /* oneshot timer:  */
  FLEXRAN_AGENT_TIMER_TYPE_ONESHOT = 0x0,

  /* periodic timer  */
  FLEXRAN_AGENT_TIMER_TYPE_PERIODIC = 0x1,

  /* Inactive state: initial state for any timer. */
  FLEXRAN_AGENT_TIMER_TYPE_EVENT_DRIVEN = 0x2,
  
  /* Max number of states available */
  FLEXRAN_AGENT_TIMER_TYPE_MAX,
} flexran_agent_timer_type_t;

typedef enum {
  /* Inactive state: initial state for any timer. */
  FLEXRAN_AGENT_TIMER_STATE_INACTIVE = 0x0,

  /* Inactive state: initial state for any timer. */
  FLEXRAN_AGENT_TIMER_STATE_ACTIVE = 0x1,

  /* Inactive state: initial state for any timer. */
  FLEXRAN_AGENT_TIMER_STATE_STOPPED = 0x2,
  
  /* Max number of states available */
  FLEXRAN_AGENT_TIMER_STATE_MAX,
} flexran_agent_timer_state_t;

typedef struct flexran_agent_timer_args_s{
  mid_t            mod_id;
  Protocol__FlexranMessage *msg;
} flexran_agent_timer_args_t;



typedef struct flexran_agent_timer_element_s{
  RB_ENTRY(flexran_agent_timer_element_s) entry;

  agent_id_t             agent_id;
  instance_t       instance;
  
  flexran_agent_timer_type_t  type;
  flexran_agent_timer_state_t state;

  uint32_t interval_sec;
  uint32_t interval_usec;

  long timer_id;  /* Timer id returned by the timer API*/
  xid_t xid; /*The id of the task as received by the controller
	       message*/
  
  flexran_agent_timer_callback_t cb;
  flexran_agent_timer_args_t *timer_args;
  
} flexran_agent_timer_element_t;

typedef struct flexran_agent_timer_instance_s{
  RB_HEAD(flexran_agent_map, flexran_agent_timer_element_s) flexran_agent_head;
}flexran_agent_timer_instance_t;


err_code_t flexran_agent_init_timer(void);

/* Create a timer for some agent related event with id xid. Will store the id 
   of the generated timer in timer_id */
err_code_t flexran_agent_create_timer(uint32_t interval_sec,
				  uint32_t interval_usec,
				  agent_id_t     agent_id,
				  instance_t     instance,
				  uint32_t timer_type,
				  xid_t xid,
				  flexran_agent_timer_callback_t cb,
				  void*    timer_args,
				  long *timer_id);

/* Destroy all existing timers */
err_code_t flexran_agent_destroy_timers(void);

/* Destroy the timer with the given timer_id */
err_code_t flexran_agent_destroy_timer(long timer_id);

/* Destroy the timer for task with id xid */
err_code_t flexran_agent_destroy_timer_by_task_id(xid_t xid);

/* Stop a timer */
err_code_t flexran_agent_stop_timer(long timer_id);

/* Restart the given timer */
err_code_t flexran_agent_restart_timer(long *timer_id);

/* Find the timer with the given timer_id */
struct flexran_agent_timer_element_s * get_timer_entry(long timer_id);

/* Obtain the protocol message stored in the given expired timer */
Protocol__FlexranMessage * flexran_agent_process_timeout(long timer_id, void* timer_args);

/* Comparator function comparing two timers. Decides the ordering of the timers */
int flexran_agent_compare_timer(struct flexran_agent_timer_element_s *a, struct flexran_agent_timer_element_s *b);

/*Specify a delay in nanoseconds to timespec and sleep until then*/
void flexran_agent_sleep_until(struct timespec *ts, int delay);

/* RB_PROTOTYPE is for .h files */
RB_PROTOTYPE(flexran_agent_map, flexran_agent_timer_element_s, entry, flexran_agent_compare_timer);


#endif
