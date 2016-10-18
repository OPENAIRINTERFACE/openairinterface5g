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

#include<stdio.h>
#include <time.h>

#include "enb_agent_common.h"
#include "enb_agent_common_internal.h"
#include "enb_agent_extern.h"
#include "PHY/extern.h"
#include "log.h"

#include "SCHED/defs.h"
#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "rrc_eNB_UE_context.h"

void * enb[NUM_MAX_ENB];
void * enb_ue[NUM_MAX_ENB];
void * enb_rrc[NUM_MAX_ENB];
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


int enb_agent_hello(mid_t mod_id, const void *params, Protocol__ProgranMessage **msg) {
 
  Protocol__PrpHeader *header;
  /*TODO: Need to set random xid or xid from received hello message*/
  xid_t xid = 1;
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
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__SUCCESSFUL_OUTCOME;
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


int enb_agent_echo_request(mid_t mod_id, const void* params, Protocol__ProgranMessage **msg) {
  Protocol__PrpHeader *header;
  /*TODO: Need to set a random xid*/
  xid_t xid = 1;
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



int enb_agent_echo_reply(mid_t mod_id, const void *params, Protocol__ProgranMessage **msg) {
  
  xid_t xid;
  Protocol__ProgranMessage *input = (Protocol__ProgranMessage *)params;
  Protocol__PrpEchoRequest *echo_req = input->echo_request_msg;
  xid = (echo_req->header)->xid;

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


int enb_agent_destroy_enb_config_reply(Protocol__ProgranMessage *msg) {
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_ENB_CONFIG_REPLY_MSG)
    goto error;
  free(msg->enb_config_reply_msg->header);
  int i, j;
  Protocol__PrpEnbConfigReply *reply = msg->enb_config_reply_msg;
  
  for(i = 0; i < reply->n_cell_config;i++){
    free(reply->cell_config[i]->mbsfn_subframe_config_rfoffset);
    free(reply->cell_config[i]->mbsfn_subframe_config_rfperiod);
    free(reply->cell_config[i]->mbsfn_subframe_config_sfalloc);
    if (reply->cell_config[i]->si_config != NULL) {
      for(j = 0; j < reply->cell_config[i]->si_config->n_si_message;j++){
	free(reply->cell_config[i]->si_config->si_message[j]);
      }
      free(reply->cell_config[i]->si_config->si_message);
      free(reply->cell_config[i]->si_config);
    }
    free(reply->cell_config[i]);
  }
  free(reply->cell_config);
  free(reply);
  free(msg);
  
  return 0;
 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_destroy_ue_config_reply(Protocol__ProgranMessage *msg) {
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_UE_CONFIG_REPLY_MSG)
    goto error;
  free(msg->ue_config_reply_msg->header);
  int i, j;
  Protocol__PrpUeConfigReply *reply = msg->ue_config_reply_msg;
  
  for(i = 0; i < reply->n_ue_config;i++){
    free(reply->ue_config[i]->capabilities);
    free(reply->ue_config[i]);
  }
  free(reply->ue_config);
  free(reply);
  free(msg);

  return 0;
 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_destroy_lc_config_reply(Protocol__ProgranMessage *msg) {
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_LC_CONFIG_REPLY_MSG)
    goto error;

  int i, j;
  free(msg->lc_config_reply_msg->header);
  for (i = 0; i < msg->lc_config_reply_msg->n_lc_ue_config; i++) {
    for (j = 0; j < msg->lc_config_reply_msg->lc_ue_config[i]->n_lc_config; j++) {
      free(msg->lc_config_reply_msg->lc_ue_config[i]->lc_config[j]);
    }
    free(msg->lc_config_reply_msg->lc_ue_config[i]->lc_config);
    free(msg->lc_config_reply_msg->lc_ue_config[i]);
  }
  free(msg->lc_config_reply_msg->lc_ue_config);
  free(msg->lc_config_reply_msg);
  free(msg);
  return 0;
 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_destroy_ue_state_change(Protocol__ProgranMessage *msg) {
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_UE_STATE_CHANGE_MSG)
    goto error;
  free(msg->ue_state_change_msg->header);
  //TODO: Free the contents of the UE config structure
  free(msg->ue_state_change_msg);
  free(msg);
  return 0;

 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_destroy_enb_config_request(Protocol__ProgranMessage *msg) {
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_ENB_CONFIG_REQUEST_MSG)
    goto error;
  free(msg->enb_config_request_msg->header);
  free(msg->enb_config_request_msg);
  free(msg);
  return 0;
  
 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int enb_agent_destroy_ue_config_request(Protocol__ProgranMessage *msg) {
  /* TODO: Deallocate memory for a dynamically allocated UE config message */
  return 0;
}

int enb_agent_destroy_lc_config_request(Protocol__ProgranMessage *msg) {
  /* TODO: Deallocate memory for a dynamically allocated LC config message */
  return 0;
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

int enb_agent_control_delegation(mid_t mod_id, const void *params, Protocol__ProgranMessage **msg) {

  Protocol__ProgranMessage *input = (Protocol__ProgranMessage *)params;
  Protocol__PrpControlDelegation *control_delegation_msg = input->control_delegation_msg;

  uint32_t delegation_type = control_delegation_msg->delegation_type;

  int i;

  struct timespec vartime = timer_start();
  
  //Write the payload lib into a file in the cache and load the lib
  char lib_name[120];
  char target[512];
  snprintf(lib_name, sizeof(lib_name), "/%s.so", control_delegation_msg->name);
  strcpy(target, local_cache);
  strcat(target, lib_name);

  FILE *f;
  f = fopen(target, "wb");
  fwrite(control_delegation_msg->payload.data, control_delegation_msg->payload.len, 1, f);
  fclose(f);

  long time_elapsed_nanos = timer_end(vartime);
  *msg = NULL;
  return 0;

 error:
  return -1;
}

int enb_agent_destroy_control_delegation(Protocol__ProgranMessage *msg) {
  /*TODO: Dealocate memory for a dynamically allocated control delegation message*/
}

int enb_agent_reconfiguration(mid_t mod_id, const void *params, Protocol__ProgranMessage **msg) {
  Protocol__ProgranMessage *input = (Protocol__ProgranMessage *)params;
  Protocol__PrpAgentReconfiguration *agent_reconfiguration_msg = input->agent_reconfiguration_msg;

  apply_reconfiguration_policy(mod_id, agent_reconfiguration_msg->policy, strlen(agent_reconfiguration_msg->policy));

  *msg = NULL;
  return 0;
}

int enb_agent_destroy_agent_reconfiguration(Protocol__ProgranMessage *msg) {
  /*TODO: Dealocate memory for a dynamically allocated agent reconfiguration message*/
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
  LOG_E(ENB_AGENT, "unknown RAN name %d\n", ran);
}

int get_current_time_ms (mid_t mod_id, int subframe_flag){

  if (subframe_flag == 1){
    return ((eNB_MAC_INST *)enb[mod_id])->frame*10 + ((eNB_MAC_INST *)enb[mod_id])->subframe;
  }else {
    return ((eNB_MAC_INST *)enb[mod_id])->frame*10;
  }
   
}

unsigned int get_current_frame (mid_t mod_id) {

  #warning "SFN will not be in [0-1023] when oaisim is used"
  return ((eNB_MAC_INST *)enb[mod_id])->frame;
  
}

unsigned int get_current_system_frame_num(mid_t mod_id) {
  return (get_current_frame(mod_id) %1024);
}

unsigned int get_current_subframe (mid_t mod_id) {

  return ((eNB_MAC_INST *)enb[mod_id])->subframe;
  
}

uint16_t get_sfn_sf (mid_t mod_id) {
  
  frame_t frame;
  sub_frame_t subframe;
  uint16_t sfn_sf, frame_mask, sf_mask;
  
  frame = (frame_t) get_current_system_frame_num(mod_id);
  subframe = (sub_frame_t) get_current_subframe(mod_id);
  frame_mask = ((1<<12) - 1);
  sf_mask = ((1<<4) - 1);
  sfn_sf = (subframe & sf_mask) | ((frame & frame_mask) << 4);
  
  return sfn_sf;
}

uint16_t get_future_sfn_sf (mid_t mod_id, int ahead_of_time) {
  
  frame_t frame;
  sub_frame_t subframe;
  uint16_t sfn_sf, frame_mask, sf_mask;
  
  frame = (frame_t) get_current_system_frame_num(mod_id);
  subframe = (sub_frame_t) get_current_subframe(mod_id);

  subframe = ((subframe + ahead_of_time) % 10);

  int full_frames_ahead = ((ahead_of_time / 10) % 10);
  
  frame = frame + full_frames_ahead;

  if (subframe < get_current_subframe(mod_id)) {
    frame++;
  }

  frame_mask = ((1<<12) - 1);
  sf_mask = ((1<<4) - 1);
  sfn_sf = (subframe & sf_mask) | ((frame & frame_mask) << 4);
  
  return sfn_sf;
}

int get_num_ues (mid_t mod_id){

  return  ((UE_list_t *)enb_ue[mod_id])->num_UEs;
}

int get_ue_crnti (mid_t mod_id, mid_t ue_id){

  return  UE_RNTI(mod_id, ue_id);
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
int get_tx_queue_size(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id)
{
	rnti_t rnti = get_ue_crnti(mod_id,ue_id);
	uint16_t frame = (uint16_t) get_current_frame(mod_id);
	mac_rlc_status_resp_t rlc_status = mac_rlc_status_ind(mod_id,rnti, mod_id,frame,ENB_FLAG_YES,MBMS_FLAG_NO,channel_id,0);
	return rlc_status.bytes_in_buffer;
}

int get_MAC_CE_bitmap_TA(mid_t mod_id, mid_t ue_id,int CC_id)
{
	rnti_t rnti = get_ue_crnti(mod_id,ue_id);
	LTE_eNB_UE_stats *eNB_UE_stats = mac_xface->get_eNB_UE_stats(mod_id,CC_id,rnti);
	
	if (eNB_UE_stats == NULL) {
	  return 0;
	}

	int32_t time_advance;
	switch (PHY_vars_eNB_g[mod_id][CC_id]->frame_parms.N_RB_DL) {
	case 6:
	  time_advance = eNB_UE_stats->timing_advance_update;
	  break;

	case 15:
	  time_advance = eNB_UE_stats->timing_advance_update/2;
	  break;

	case 25:
	  time_advance = eNB_UE_stats->timing_advance_update/4;
	  break;

	case 50:
	  time_advance = eNB_UE_stats->timing_advance_update/8;
	  break;

	case 75:
	  time_advance = eNB_UE_stats->timing_advance_update/12;
	  break;

	case 100:
	  time_advance = eNB_UE_stats->timing_advance_update/16;
	  break;
	}

	if(time_advance > 0)
		return 1;
	else
		return 0;
}
int get_active_CC(mid_t mod_id, mid_t ue_id)
{
	return ((UE_list_t *)enb_ue[mod_id])->numactiveCCs[ue_id];
}

int get_current_RI(mid_t mod_id, mid_t ue_id, int CC_id)
{
	LTE_eNB_UE_stats *eNB_UE_stats = NULL;

	int pCCid = UE_PCCID(mod_id,ue_id);
	rnti_t rnti = get_ue_crnti(mod_id,ue_id);

	eNB_UE_stats = mac_xface->get_eNB_UE_stats(mod_id,CC_id,rnti);
	
	if (eNB_UE_stats == NULL) {
	  return 0;
	}

	return eNB_UE_stats[CC_id].rank;
}

int get_tpc(mid_t mod_id, mid_t ue_id)
{
	LTE_eNB_UE_stats *eNB_UE_stats = NULL;
	int32_t normalized_rx_power, target_rx_power;
	int tpc = 1;

	int pCCid = UE_PCCID(mod_id,ue_id);
	rnti_t rnti = get_ue_crnti(mod_id,ue_id);

	eNB_UE_stats =  mac_xface->get_eNB_UE_stats(mod_id, pCCid, rnti);

	target_rx_power = mac_xface->get_target_pusch_rx_power(mod_id,pCCid);

	if (eNB_UE_stats == NULL) {
	  normalized_rx_power = target_rx_power;
	} else if (eNB_UE_stats->UL_rssi != NULL) {
	  normalized_rx_power = eNB_UE_stats->UL_rssi[0];
	} else {
	  normalized_rx_power = target_rx_power;
	}

	if (normalized_rx_power>(target_rx_power+1)) {
		tpc = 0; //-1
	} else if (normalized_rx_power<(target_rx_power-1)) {
		tpc = 2; //+1
	} else {
		tpc = 1; //0
	}
	return tpc;
}

int get_harq(const mid_t mod_id, const uint8_t CC_id, const mid_t ue_id, const int frame, const uint8_t subframe, int *id, int *status)	//flag_id_status = 0 then id, else status
{
	/*TODO: Add int TB in function parameters to get the status of the second TB. This can be done to by editing in
	 * get_ue_active_harq_pid function in line 272 file: phy_procedures_lte_eNB.c to add
	 * DLSCH_ptr = PHY_vars_eNB_g[Mod_id][CC_id]->dlsch_eNB[(uint32_t)UE_id][1];*/

  uint8_t harq_pid;
  uint8_t round;
  

  uint16_t rnti = get_ue_crnti(mod_id,ue_id);

  mac_xface->get_ue_active_harq_pid(mod_id,CC_id,rnti,frame,subframe,&harq_pid,&round,openair_harq_DL);

  *id = harq_pid;
  if (round > 0) {
    *status = 1;
  } else {
    *status = 0;
  }

  return 0;
}

int get_p0_pucch_dbm(mid_t mod_id, mid_t ue_id, int CC_id)
{
	LTE_eNB_UE_stats *eNB_UE_stats = NULL;
	uint32_t rnti = get_ue_crnti(mod_id,ue_id);

	eNB_UE_stats =  mac_xface->get_eNB_UE_stats(mod_id, CC_id, rnti);
	
	if (eNB_UE_stats == NULL) {
	  return -1;
	}
	
	if(eNB_UE_stats->Po_PUCCH_update == 1)
	{
		return eNB_UE_stats->Po_PUCCH_dBm;
	}
	else
		return -1;
}

int get_p0_nominal_pucch(mid_t mod_id, int CC_id)
{
	int32_t pucch_rx_received = mac_xface->get_target_pucch_rx_power(mod_id, CC_id);
	return pucch_rx_received;
}


/*
 * ************************************
 * Get Messages for eNB Configuration Reply
 * ************************************
 */

int get_hopping_offset(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->pusch_config_common.pusch_HoppingOffset;
}
int get_hopping_mode(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->pusch_config_common.hoppingMode;
}
int get_n_SB(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->pusch_config_common.n_SB;
}
int get_enable64QAM(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->pusch_config_common.enable64QAM;
}
int get_phich_duration(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->phich_config_common.phich_duration;
}
int get_phich_resource(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	if(frame_parms->phich_config_common.phich_resource == oneSixth)
		return 0;
	else if(frame_parms->phich_config_common.phich_resource == half)
		return 1;
	else if(frame_parms->phich_config_common.phich_resource == one)
		return 2;
	else if(frame_parms->phich_config_common.phich_resource == two)
		return 3;

	return -1;
}
int get_n1pucch_an(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->pucch_config_common.n1PUCCH_AN;
}
int get_nRB_CQI(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->pucch_config_common.nRB_CQI;
}
int get_deltaPUCCH_Shift(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->pucch_config_common.deltaPUCCH_Shift;
}
int get_prach_ConfigIndex(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
}
int get_prach_FreqOffset(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset;
}
int get_maxHARQ_Msg3Tx(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->maxHARQ_Msg3Tx;
}
int get_ul_cyclic_prefix_length(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->Ncp_UL;
}
int get_dl_cyclic_prefix_length(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->Ncp;
}
int get_cell_id(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->Nid_cell;
}
int get_srs_BandwidthConfig(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->soundingrs_ul_config_common.srs_BandwidthConfig;
}
int get_srs_SubframeConfig(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->soundingrs_ul_config_common.srs_SubframeConfig;
}
int get_srs_MaxUpPts(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->soundingrs_ul_config_common.srs_MaxUpPts;
}
int get_N_RB_DL(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->N_RB_DL;
}
int get_N_RB_UL(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->N_RB_UL;
}
int get_subframe_assignment(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->tdd_config;
}
int get_special_subframe_assignment(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	return frame_parms->tdd_config_S;
}
int get_ra_ResponseWindowSize(mid_t mod_id, int CC_id)
{
	Enb_properties_array_t *enb_properties;
	enb_properties = enb_config_get();
	return enb_properties->properties[mod_id]->rach_raResponseWindowSize[CC_id];
}
int get_mac_ContentionResolutionTimer(mid_t mod_id, int CC_id)
{
	Enb_properties_array_t *enb_properties;
	enb_properties = enb_config_get();
	return enb_properties->properties[mod_id]->rach_macContentionResolutionTimer[CC_id];
}
int get_duplex_mode(mid_t mod_id, int CC_id)
{
	LTE_DL_FRAME_PARMS   *frame_parms;

	frame_parms = mac_xface->get_lte_frame_parms(mod_id, CC_id);
	if(frame_parms->frame_type == 0)
		return 1;
	else if (frame_parms->frame_type == 1)
		return 0;

	return -1;
}
long get_si_window_length(mid_t mod_id, int CC_id)
{
	return  ((eNB_RRC_INST *)enb_rrc[mod_id])->carrier[CC_id].sib1->si_WindowLength;
}
int get_sib1_length(mid_t mod_id, int CC_id)
{
	return  ((eNB_RRC_INST *)enb_rrc[mod_id])->carrier[CC_id].sizeof_SIB1;
}
int get_num_pdcch_symb(mid_t mod_id, int CC_id)
{
  /* TODO: This should return the number of PDCCH symbols initially used by the cell CC_id */
  return 0;
  //(PHY_vars_UE_g[mod_id][CC_id]->lte_ue_pdcch_vars[mod_id]->num_pdcch_symbols);
}

/*
 * ************************************
 * Get Messages for UE Configuration Reply
 * ************************************
 */


int get_time_alignment_timer(mid_t mod_id, mid_t ue_id)
{
	struct rrc_eNB_ue_context_s* ue_context_p = NULL;
	uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

	ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
	if(ue_context_p != NULL)
	{
		if(ue_context_p->ue_context.mac_MainConfig != NULL)
			return ue_context_p->ue_context.mac_MainConfig->timeAlignmentTimerDedicated;
	}
	else
		return -1;
}

int get_meas_gap_config(mid_t mod_id, mid_t ue_id) {
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

  ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
  if(ue_context_p != NULL) {
    if(ue_context_p->ue_context.measGapConfig != NULL){
      if(ue_context_p->ue_context.measGapConfig->present == MeasGapConfig_PR_setup) {
	if (ue_context_p->ue_context.measGapConfig->choice.setup.gapOffset.present == MeasGapConfig__setup__gapOffset_PR_gp0) {
	  return PROTOCOL__PRP_MEAS_GAP_CONFIG_PATTERN__PRMGCP_GP1;
	} else if (ue_context_p->ue_context.measGapConfig->choice.setup.gapOffset.present == MeasGapConfig__setup__gapOffset_PR_gp1) {
	  return PROTOCOL__PRP_MEAS_GAP_CONFIG_PATTERN__PRMGCP_GP2;
	} else {
	  return PROTOCOL__PRP_MEAS_GAP_CONFIG_PATTERN__PRMGCP_OFF;
	}
      }
    }
  }
  return -1;
}


int get_meas_gap_config_offset(mid_t mod_id, mid_t ue_id) {
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  uint32_t rntiP = get_ue_crnti(mod_id,ue_id);
  
  ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
  
  if(ue_context_p != NULL) {
    if(ue_context_p->ue_context.measGapConfig != NULL){
      if(ue_context_p->ue_context.measGapConfig->present == MeasGapConfig_PR_setup) {
	if (ue_context_p->ue_context.measGapConfig->choice.setup.gapOffset.present == MeasGapConfig__setup__gapOffset_PR_gp0) {
	  return ue_context_p->ue_context.measGapConfig->choice.setup.gapOffset.choice.gp0;
	} else if (ue_context_p->ue_context.measGapConfig->choice.setup.gapOffset.present == MeasGapConfig__setup__gapOffset_PR_gp1) {
	  return ue_context_p->ue_context.measGapConfig->choice.setup.gapOffset.choice.gp0;
	} 
      }
    }
  }
  return -1;
}

int get_ue_aggregated_max_bitrate_dl (mid_t mod_id, mid_t ue_id)
{
	return ((UE_list_t *)enb_ue[mod_id])->UE_sched_ctrl[ue_id].ue_AggregatedMaximumBitrateDL;
}

int get_ue_aggregated_max_bitrate_ul (mid_t mod_id, mid_t ue_id)
{
	return ((UE_list_t *)enb_ue[mod_id])->UE_sched_ctrl[ue_id].ue_AggregatedMaximumBitrateUL;
}

int get_half_duplex(mid_t ue_id)
{
	//int halfduplex = 0;
	//int bands_to_scan = ((UE_RRC_INST *)enb_ue_rrc[ue_id])->UECap->UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list.count;
	//for (int i =0; i < bands_to_scan; i++){
		//if(((UE_RRC_INST *)enb_ue_rrc[ue_id])->UECap->UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list.array[i]->halfDuplex > 0)
		//	halfduplex = 1;
	//}
	//return halfduplex;
}

int get_intra_sf_hopping(mid_t ue_id)
{
	//TODO:Get proper value
	//temp = (((UE_RRC_INST *)enb_ue_rrc[ue_id])->UECap->UE_EUTRA_Capability->featureGroupIndicators->buf);
	//return (0 & ( 1 << (31)));
}

int get_type2_sb_1(mid_t ue_id)
{
	//TODO:Get proper value
	//uint8_t temp = 0;
	//temp = (((UE_RRC_INST *)enb_ue_rrc[ue_id])->UECap->UE_EUTRA_Capability->featureGroupIndicators->buf);
	//return (temp & ( 1 << (11)));
}

int get_ue_category(mid_t ue_id)
{
	//TODO:Get proper value
	//return (((UE_RRC_INST *)enb_ue_rrc[ue_id])->UECap->UE_EUTRA_Capability->ue_Category);
}

int get_res_alloc_type1(mid_t ue_id)
{
	//TODO:Get proper value
	//uint8_t temp = 0;
	//temp = (((UE_RRC_INST *)enb_ue_rrc[ue_id])->UECap->UE_EUTRA_Capability->featureGroupIndicators->buf);
	//return (temp & ( 1 << (30)));
}

int get_ue_transmission_mode(mid_t mod_id, mid_t ue_id)
{
	struct rrc_eNB_ue_context_s* ue_context_p = NULL;
	uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

	ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);

	if(ue_context_p != NULL)
	{
		if(ue_context_p->ue_context.physicalConfigDedicated != NULL){
			return ue_context_p->ue_context.physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode;
		}
	}
	else
		return -1;
}

int get_tti_bundling(mid_t mod_id, mid_t ue_id)
{
	struct rrc_eNB_ue_context_s* ue_context_p = NULL;
	uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

	ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
	if(ue_context_p != NULL)
	{
		if(ue_context_p->ue_context.mac_MainConfig != NULL){
			return ue_context_p->ue_context.mac_MainConfig->ul_SCH_Config->ttiBundling;
		}
	}
	else
		return -1;
}

int get_maxHARQ_TX(mid_t mod_id, mid_t ue_id) {
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  uint32_t rntiP = get_ue_crnti(mod_id,ue_id);
  
  ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
  if(ue_context_p != NULL) {
    if(ue_context_p->ue_context.mac_MainConfig != NULL){
      return *ue_context_p->ue_context.mac_MainConfig->ul_SCH_Config->maxHARQ_Tx;
    }
  }
  return -1;
}

int get_beta_offset_ack_index(mid_t mod_id, mid_t ue_id)
{
	struct rrc_eNB_ue_context_s* ue_context_p = NULL;
	uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

	ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
	if(ue_context_p != NULL)
	{
		if(ue_context_p->ue_context.physicalConfigDedicated != NULL){
			return ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_ACK_Index;
		}
	}
	else
		return -1;
}

int get_beta_offset_ri_index(mid_t mod_id, mid_t ue_id)
{
	struct rrc_eNB_ue_context_s* ue_context_p = NULL;
	uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

	ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
	if(ue_context_p != NULL)
	{
		if(ue_context_p->ue_context.physicalConfigDedicated != NULL){
			return ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_RI_Index;
		}
	}
	else
		return -1;
}

int get_beta_offset_cqi_index(mid_t mod_id, mid_t ue_id)
{
	struct rrc_eNB_ue_context_s* ue_context_p = NULL;
	uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

	ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
	if(ue_context_p != NULL)
	{
		if(ue_context_p->ue_context.physicalConfigDedicated != NULL){
			return ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_CQI_Index;
		}
	}
	else
		return -1;
}

int get_simultaneous_ack_nack_cqi(mid_t mod_id, mid_t ue_id)
{
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  uint32_t rntiP = get_ue_crnti(mod_id,ue_id);
  
  ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
  if(ue_context_p != NULL) {
    if(ue_context_p->ue_context.physicalConfigDedicated != NULL){
      if (ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic != NULL) {
	return ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.simultaneousAckNackAndCQI;
      }
    }
  }
  return -1;
}

int get_ack_nack_simultaneous_trans(mid_t mod_id,mid_t ue_id)
{
	return (&eNB_rrc_inst[mod_id])->carrier[0].sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission;
}

int get_aperiodic_cqi_rep_mode(mid_t mod_id,mid_t ue_id) {
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  uint32_t rntiP = get_ue_crnti(mod_id,ue_id);
  
  ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
  
  if(ue_context_p != NULL) {
    if(ue_context_p->ue_context.physicalConfigDedicated != NULL){
      return *ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig->cqi_ReportModeAperiodic;
    }
  }
  return -1;
}

int get_tdd_ack_nack_feedback(mid_t mod_id, mid_t ue_id)
{
	struct rrc_eNB_ue_context_s* ue_context_p = NULL;
	uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

	ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);

	if(ue_context_p != NULL)
	{
		if(ue_context_p->ue_context.physicalConfigDedicated != NULL){
			return ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode;
		}
	}
	else
		return -1;
}

int get_ack_nack_repetition_factor(mid_t mod_id, mid_t ue_id)
{
	struct rrc_eNB_ue_context_s* ue_context_p = NULL;
	uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

	ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
	if(ue_context_p != NULL)
	{
		if(ue_context_p->ue_context.physicalConfigDedicated != NULL){
			return ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated->ackNackRepetition.choice.setup.repetitionFactor;
		}
	}
	else
		return -1;
}

int get_extended_bsr_size(mid_t mod_id, mid_t ue_id) {
  //TODO: need to double check
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

  ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);
  if(ue_context_p != NULL) {
    if(ue_context_p->ue_context.mac_MainConfig != NULL){
      if(ue_context_p->ue_context.mac_MainConfig->ext2 != NULL){
	long val = (*(ue_context_p->ue_context.mac_MainConfig->ext2->mac_MainConfig_v1020->extendedBSR_Sizes_r10));
	if (val > 0) {
	  return 1;
	}
      }
    }
  }
  return -1;
}

int get_ue_transmission_antenna(mid_t mod_id, mid_t ue_id)
{
	struct rrc_eNB_ue_context_s* ue_context_p = NULL;
	uint32_t rntiP = get_ue_crnti(mod_id,ue_id);

	ue_context_p = rrc_eNB_get_ue_context(&eNB_rrc_inst[mod_id],rntiP);

	if(ue_context_p != NULL)
	{
		if(ue_context_p->ue_context.physicalConfigDedicated != NULL){
			if(ue_context_p->ue_context.physicalConfigDedicated->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.choice.setup == AntennaInfoDedicated__ue_TransmitAntennaSelection__setup_closedLoop)
				return 2;
			else if(ue_context_p->ue_context.physicalConfigDedicated->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.choice.setup == AntennaInfoDedicated__ue_TransmitAntennaSelection__setup_openLoop)
				return 1;
			else
				return 0;
		}
	}
	else
		return -1;
}

int get_lcg(mid_t ue_id, mid_t lc_id)
{
  if (UE_mac_inst == NULL) {
    return -1;
  }
  if(UE_mac_inst[ue_id].logicalChannelConfig[lc_id] != NULL) {
    return *UE_mac_inst[ue_id].logicalChannelConfig[lc_id]->ul_SpecificParameters->logicalChannelGroup;
  } else {
    return -1;
  }
}

int get_direction(mid_t ue_id, mid_t lc_id)
{
	/*TODO: fill with the value for the rest of LCID*/
	if(lc_id == DCCH | lc_id == DCCH1)
		return 2;
	else if(lc_id == DTCH)
		return 1;
}

int enb_agent_ue_state_change(mid_t mod_id, uint32_t rnti, uint8_t state_change) {
  int size;
  Protocol__ProgranMessage *msg;
  Protocol__PrpHeader *header;
  void *data;
  int priority;
  err_code_t err_code;

  int xid = 0;

  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_UE_STATE_CHANGE, &header) != 0)
    goto error;

  Protocol__PrpUeStateChange *ue_state_change_msg;
  ue_state_change_msg = malloc(sizeof(Protocol__PrpUeStateChange));
  if(ue_state_change_msg == NULL) {
    goto error;
  }
  protocol__prp_ue_state_change__init(ue_state_change_msg);
  ue_state_change_msg->has_type = 1;
  ue_state_change_msg->type = state_change;

  Protocol__PrpUeConfig *config;
  config = malloc(sizeof(Protocol__PrpUeConfig));
  if (config == NULL) {
    goto error;
  }
  protocol__prp_ue_config__init(config);
  if (state_change == PROTOCOL__PRP_UE_STATE_CHANGE_TYPE__PRUESC_DEACTIVATED) {
    // Simply set the rnti of the UE
    config->has_rnti = 1;
    config->rnti = rnti;
  } else if (state_change == PROTOCOL__PRP_UE_STATE_CHANGE_TYPE__PRUESC_UPDATED
	     || state_change == PROTOCOL__PRP_UE_STATE_CHANGE_TYPE__PRUESC_ACTIVATED) {
	  	  int i =find_UE_id(mod_id,rnti);
     	  config->has_rnti = 1;
          config->rnti = rnti;
	  		//TODO: Set the time_alignment_timer
	  	  if(get_time_alignment_timer(mod_id,i) != -1){
	  		  config->time_alignment_timer = get_time_alignment_timer(mod_id,i);
	  		  config->has_time_alignment_timer = 1;
	  	  }
	  	  //TODO: Set the measurement gap configuration pattern
	  	  if(get_meas_gap_config(mod_id,i) != -1){
	  		  config->meas_gap_config_pattern = get_meas_gap_config(mod_id,i);
	  	  	  config->has_meas_gap_config_pattern = 1;
	  	  }
	  	  //TODO: Set the measurement gap offset if applicable
	  	  if(config->has_meas_gap_config_pattern == 1 &&
		     config->meas_gap_config_pattern != PROTOCOL__PRP_MEAS_GAP_CONFIG_PATTERN__PRMGCP_OFF) {
		    config->meas_gap_config_sf_offset = get_meas_gap_config_offset(mod_id,i);
		    config->has_meas_gap_config_sf_offset = 1;
	  	  }
	  	  //TODO: Set the SPS configuration (Optional)
	  	  //Not supported for noe, so we do not set it

	  	  //TODO: Set the SR configuration (Optional)
	  	  //We do not set it for now

	  	  //TODO: Set the CQI configuration (Optional)
	  	  //We do not set it for now

	  	  //TODO: Set the transmission mode
	  	  if(get_ue_transmission_mode(mod_id,i) != -1){
	  		  config->transmission_mode = get_ue_transmission_mode(mod_id,i);
	  		  config->has_transmission_mode = 1;
	  	  }

	  	  //TODO: Set the aggregated bit-rate of the non-gbr bearer (UL)
	  	  config->ue_aggregated_max_bitrate_ul = get_ue_aggregated_max_bitrate_ul(mod_id,i);
	  	  config->has_ue_aggregated_max_bitrate_ul = 1;

	  	  //TODO: Set the aggregated bit-rate of the non-gbr bearer (DL)
	  	  config->ue_aggregated_max_bitrate_dl = get_ue_aggregated_max_bitrate_dl(mod_id,i);
	  	  config->has_ue_aggregated_max_bitrate_dl = 1;

	  	  //TODO: Set the UE capabilities
	  	  Protocol__PrpUeCapabilities *c_capabilities;
	  	  c_capabilities = malloc(sizeof(Protocol__PrpUeCapabilities));
	  	  protocol__prp_ue_capabilities__init(c_capabilities);
	  	  //TODO: Set half duplex (FDD operation)
	  	  c_capabilities->has_half_duplex = 1;
	  	  c_capabilities->half_duplex = 1;//get_half_duplex(i);
	  	  //TODO: Set intra-frame hopping flag
	  	  c_capabilities->has_intra_sf_hopping = 1;
	  	  c_capabilities->intra_sf_hopping = 1;//get_intra_sf_hopping(i);
	  	  //TODO: Set support for type 2 hopping with n_sb > 1
	  	  c_capabilities->has_type2_sb_1 = 1;
	  	  c_capabilities->type2_sb_1 = 1;//get_type2_sb_1(i);
	  	  //TODO: Set ue category
	  	  c_capabilities->has_ue_category = 1;
	  	  c_capabilities->ue_category = 1;//get_ue_category(i);
	  	  //TODO: Set UE support for resource allocation type 1
	  	  c_capabilities->has_res_alloc_type1 = 1;
	  	  c_capabilities->res_alloc_type1 = 1;//get_res_alloc_type1(i);
	  	  //Set the capabilites to the message
	  	  config->capabilities = c_capabilities;
	  	  //TODO: Set UE transmission antenna. One of the PRUTA_* values
	  	  if(get_ue_transmission_antenna(mod_id,i) != -1){
	  		  config->has_ue_transmission_antenna = 1;
	  		  config->ue_transmission_antenna = get_ue_transmission_antenna(mod_id,i);
	  	  }
	  	  //TODO: Set tti bundling flag (See ts 36.321)
	  	  if(get_tti_bundling(mod_id,i) != -1){
	  		  config->has_tti_bundling = 1;
	  		  config->tti_bundling = get_tti_bundling(mod_id,i);
	  	  }
	  	  //TODO: Set the max HARQ retransmission for the UL
	  	  if(get_maxHARQ_TX(mod_id,i) != -1){
	  		  config->has_max_harq_tx = 1;
	  		  config->max_harq_tx = get_maxHARQ_TX(mod_id,i);
	  	  }
	  	  //TODO: Fill beta_offset_ack_index (TS 36.213)
	  	  if(get_beta_offset_ack_index(mod_id,i) != -1){
	  		  config->has_beta_offset_ack_index = 1;
	  		  config->beta_offset_ack_index = get_beta_offset_ack_index(mod_id,i);
	  	  }
	  	  //TODO: Fill beta_offset_ri_index (TS 36.213)
	  	  if(get_beta_offset_ri_index(mod_id,i) != -1){
	  		  config->has_beta_offset_ri_index = 1;
	  		  config->beta_offset_ri_index = get_beta_offset_ri_index(mod_id,i);
	  	  }
	  	  //TODO: Fill beta_offset_cqi_index (TS 36.213)
	  	  if(get_beta_offset_cqi_index(mod_id,i) != -1){
	  		  config->has_beta_offset_cqi_index = 1;
	  		  config->beta_offset_cqi_index = get_beta_offset_cqi_index(mod_id,i);
	  	  }
	  	  //TODO: Fill ack_nack_simultaneous_trans (TS 36.213)
	  	  if(get_ack_nack_simultaneous_trans(mod_id,i) != -1){
	  		  config->has_ack_nack_simultaneous_trans = 1;
	  		  config->ack_nack_simultaneous_trans = get_ack_nack_simultaneous_trans(mod_id,i);
	  	  }
	  	  //TODO: Fill simultaneous_ack_nack_cqi (TS 36.213)
	  	  if(get_simultaneous_ack_nack_cqi(mod_id,i) != -1){
	  		  config->has_simultaneous_ack_nack_cqi = 1;
	  		  config->simultaneous_ack_nack_cqi = get_simultaneous_ack_nack_cqi(mod_id,i);
	  	  }
	  	  //TODO: Set PRACRM_* value regarding aperiodic cqi report mode
	  	  if(get_aperiodic_cqi_rep_mode(mod_id,i) != -1){
	  		  config->has_aperiodic_cqi_rep_mode = 1;
			  int mode = get_aperiodic_cqi_rep_mode(mod_id,i);
			  if (mode > 4) {
			    config->aperiodic_cqi_rep_mode = PROTOCOL__PRP_APERIODIC_CQI_REPORT_MODE__PRACRM_NONE;
			      } else {
			    config->aperiodic_cqi_rep_mode = mode;
			  }
	  	  }
	  	  //TODO: Set tdd_ack_nack_feedback
	  	  if(get_tdd_ack_nack_feedback(mod_id, i) != -1){
	  		  config->has_tdd_ack_nack_feedback = 1;
	  		  config->tdd_ack_nack_feedback = get_tdd_ack_nack_feedback(mod_id,i);
	  	  }
	  	  //TODO: Set ack_nack_repetition factor
	  	  if(get_ack_nack_repetition_factor(mod_id, i) != -1){
	  		  config->has_ack_nack_repetition_factor = 1;
	  		  config->ack_nack_repetition_factor = get_ack_nack_repetition_factor(mod_id,i);
	  	  }
	  	  //TODO: Set extended BSR size
	  	  if(get_extended_bsr_size(mod_id, i) != -1){
	  		  config->has_extended_bsr_size = 1;
	  		  config->extended_bsr_size = get_extended_bsr_size(mod_id,i);
	  	  }
		  //TODO: Set index of primary cell
		  config->has_pcell_carrier_index = 1;
		  config->pcell_carrier_index = UE_PCCID(mod_id, i);
	  	  //TODO: Set carrier aggregation support (boolean)
	  	  config->has_ca_support = 0;
	  	  config->ca_support = 0;
	  	  if(config->has_ca_support){
	  		  //TODO: Set cross carrier scheduling support (boolean)
	  		  config->has_cross_carrier_sched_support = 1;
	  		  config->cross_carrier_sched_support = 0;
	  		  //TODO: Set secondary cells configuration
	  		  // We do not set it for now. No carrier aggregation support

	  		  //TODO: Set deactivation timer for secondary cell
	  		  config->has_scell_deactivation_timer = 1;
	  		  config->scell_deactivation_timer = 1;
	  	  }
  } else if (state_change == PROTOCOL__PRP_UE_STATE_CHANGE_TYPE__PRUESC_MOVED) {
    // TODO: Not supported for now. Leave blank
  }

  ue_state_change_msg->config = config;
  msg = malloc(sizeof(Protocol__ProgranMessage));
  if (msg == NULL) {
    goto error;
  }
  protocol__progran_message__init(msg);
  msg->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_UE_STATE_CHANGE_MSG;
  msg->msg_dir = PROTOCOL__PROGRAN_DIRECTION__INITIATING_MESSAGE;
  msg->ue_state_change_msg = ue_state_change_msg;

  data = enb_agent_pack_message(msg, &size);
  /*Send sr info using the MAC channel of the eNB*/
  if (enb_agent_msg_send(mod_id, ENB_AGENT_DEFAULT, data, size, priority)) {
    err_code = PROTOCOL__PROGRAN_ERR__MSG_ENQUEUING;
    goto error;
  }

  LOG_D(ENB_AGENT,"sent message with size %d\n", size);
  return;
 error:
  LOG_D(ENB_AGENT, "Could not send UE state message\n");
}

/*
 * timer primitives
 */

int enb_agent_lc_config_reply(mid_t mod_id, const void *params, Protocol__ProgranMessage **msg) {

  xid_t xid;
  Protocol__ProgranMessage *input = (Protocol__ProgranMessage *)params;
  Protocol__PrpLcConfigRequest *lc_config_request_msg = input->lc_config_request_msg;
  xid = (lc_config_request_msg->header)->xid;

  int i, j;
  Protocol__PrpHeader *header;
  if(prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_GET_LC_CONFIG_REPLY, &header) != 0)
    goto error;

  Protocol__PrpLcConfigReply *lc_config_reply_msg;
  lc_config_reply_msg = malloc(sizeof(Protocol__PrpLcConfigReply));
  if(lc_config_reply_msg == NULL)
    goto error;
  protocol__prp_lc_config_reply__init(lc_config_reply_msg);
  lc_config_reply_msg->header = header;

  //TODO: Fill in the actual number of UEs that we are going to report LC configs about
  lc_config_reply_msg->n_lc_ue_config = get_num_ues(mod_id);

  Protocol__PrpLcUeConfig **lc_ue_config;
  if (lc_config_reply_msg->n_lc_ue_config > 0) {
    lc_ue_config = malloc(sizeof(Protocol__PrpLcUeConfig *) * lc_config_reply_msg->n_lc_ue_config);
    if (lc_ue_config == NULL) {
      goto error;
    }
    // Fill the config for each UE
    for (i = 0; i < lc_config_reply_msg->n_lc_ue_config; i++) {
      lc_ue_config[i] = malloc(sizeof(Protocol__PrpLcUeConfig));
      protocol__prp_lc_ue_config__init(lc_ue_config[i]);
      //TODO: Set the RNTI of the UE
      lc_ue_config[i]->has_rnti = 1;
      lc_ue_config[i]->rnti = get_ue_crnti(mod_id,i);
      //TODO: Set the number of LC configurations that will be reported for this UE
      //Set this according to the current state of the UE. This is only a temporary fix
      int status = 0;
      status = mac_eNB_get_rrc_status(mod_id, get_ue_crnti(mod_id, i));
      if (status < RRC_CONNECTED) {
	lc_ue_config[i]->n_lc_config = 0;
      } else if (status == RRC_CONNECTED) {
	lc_ue_config[i]->n_lc_config = 1;
      } else {
	lc_ue_config[i]->n_lc_config = 3;
      }
      Protocol__PrpLcConfig **lc_config;
      if (lc_ue_config[i]->n_lc_config > 0) {
	lc_config = malloc(sizeof(Protocol__PrpLcConfig *) * lc_ue_config[i]->n_lc_config);
	if (lc_config == NULL) {
	  goto error;
	}
	for (j = 0; j < lc_ue_config[i]->n_lc_config; j++) {
	  lc_config[j] = malloc(sizeof(Protocol__PrpLcConfig));
	  protocol__prp_lc_config__init(lc_config[j]);
	  //TODO: Set the LC id
	  lc_config[j]->has_lcid = 1;
	  lc_config[j]->lcid = j+1;
	  //TODO: Set the LCG of the channel
	  int lcg = get_lcg(i, j+1);
	  if (lcg >= 0 && lcg <= 3) {
	    lc_config[j]->has_lcg = 1;
	    lc_config[j]->lcg = get_lcg(i,j+1);
	  }
	  //TODO: Set the LC direction
	  lc_config[j]->has_direction = 1;
	  lc_config[j]->direction = get_direction(i,j+1);
	  //TODO: Bearer type. One of PRQBT_* values
	  lc_config[j]->has_qos_bearer_type = 1;
	  lc_config[j]->qos_bearer_type = PROTOCOL__PRP_QOS_BEARER_TYPE__PRQBT_NON_GBR;
	  //TODO: Set the QCI defined in TS 23.203, coded as defined in TS 36.413
	  // One less than the actual QCI value
	  lc_config[j]->has_qci = 1;
	  lc_config[j]->qci = 1;
	  if (lc_config[j]->direction == PROTOCOL__PRP_QOS_BEARER_TYPE__PRQBT_GBR) {
	    //TODO: Set the max bitrate (UL)
	    lc_config[j]->has_e_rab_max_bitrate_ul = 1;
	    lc_config[j]->e_rab_max_bitrate_ul = 1;
	    //TODO: Set the max bitrate (DL)
	    lc_config[j]->has_e_rab_max_bitrate_dl = 1;
	    lc_config[j]->e_rab_max_bitrate_dl = 1;
	    //TODO: Set the guaranteed bitrate (UL)
	    lc_config[j]->has_e_rab_guaranteed_bitrate_ul = 1;
	    lc_config[j]->e_rab_guaranteed_bitrate_ul = 1;
	    //TODO: Set the guaranteed bitrate (DL)
	    lc_config[j]->has_e_rab_guaranteed_bitrate_dl = 1;
	    lc_config[j]->e_rab_guaranteed_bitrate_dl = 1;
	  }
	}
	lc_ue_config[i]->lc_config = lc_config;
      }
    } // end for UE
    lc_config_reply_msg->lc_ue_config = lc_ue_config;
  } // lc_config_reply_msg->n_lc_ue_config > 0
  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if (*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_LC_CONFIG_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->lc_config_reply_msg = lc_config_reply_msg;

  return 0;

 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (lc_config_reply_msg != NULL)
    free(lc_config_reply_msg);
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

/*
 * ************************************
 * UE Configuration Reply
 * ************************************
 */

int enb_agent_ue_config_reply(mid_t mod_id, const void *params, Protocol__ProgranMessage **msg) {

  xid_t xid;
  Protocol__ProgranMessage *input = (Protocol__ProgranMessage *)params;
  Protocol__PrpUeConfigRequest *ue_config_request_msg = input->ue_config_request_msg;
  xid = (ue_config_request_msg->header)->xid;

  int i;

  Protocol__PrpHeader *header;
  if(prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_GET_UE_CONFIG_REPLY, &header) != 0)
    goto error;

  Protocol__PrpUeConfigReply *ue_config_reply_msg;
  ue_config_reply_msg = malloc(sizeof(Protocol__PrpUeConfigReply));
  if(ue_config_reply_msg == NULL)
    goto error;
  protocol__prp_ue_config_reply__init(ue_config_reply_msg);
  ue_config_reply_msg->header = header;

  //TODO: Fill in the actual number of UEs that are currently connected
  ue_config_reply_msg->n_ue_config = get_num_ues(mod_id);

  Protocol__PrpUeConfig **ue_config;
  if (ue_config_reply_msg->n_ue_config > 0) {
    ue_config = malloc(sizeof(Protocol__PrpUeConfig *) * ue_config_reply_msg->n_ue_config);
    if (ue_config == NULL) {
      goto error;
    }
    for (i = 0; i < ue_config_reply_msg->n_ue_config; i++) {
      ue_config[i] = malloc(sizeof(Protocol__PrpUeConfig));
      protocol__prp_ue_config__init(ue_config[i]);
      //TODO: Set the RNTI of the ue with id i
      ue_config[i]->rnti = get_ue_crnti(mod_id,i);
      ue_config[i]->has_rnti = 1;
      //TODO: Set the DRX configuration (optional)
      //Not supported for now, so we do not set it

      //TODO: Set the time_alignment_timer
      if(get_time_alignment_timer(mod_id,i) != -1){
    	  ue_config[i]->time_alignment_timer = get_time_alignment_timer(mod_id,i);
    	  ue_config[i]->has_time_alignment_timer = 1;
	  }
      //TODO: Set the measurement gap configuration pattern
      if(get_meas_gap_config(mod_id,i) != -1){
    	  ue_config[i]->meas_gap_config_pattern = get_meas_gap_config(mod_id,i);
    	  ue_config[i]->has_meas_gap_config_pattern = 1;
	  }
      //TODO: Set the measurement gap offset if applicable
      if(ue_config[i]->has_meas_gap_config_pattern == 1 &&
	 ue_config[i]->meas_gap_config_pattern != PROTOCOL__PRP_MEAS_GAP_CONFIG_PATTERN__PRMGCP_OFF) {
	ue_config[i]->meas_gap_config_sf_offset = get_meas_gap_config_offset(mod_id,i);
	ue_config[i]->has_meas_gap_config_sf_offset = 1;
      }
      //TODO: Set the SPS configuration (Optional)
      //Not supported for noe, so we do not set it

      //TODO: Set the SR configuration (Optional)
      //We do not set it for now

      //TODO: Set the CQI configuration (Optional)
      //We do not set it for now

      //TODO: Set the transmission mode
      if(get_ue_transmission_mode(mod_id,i) != -1){
    	  ue_config[i]->transmission_mode = get_ue_transmission_mode(mod_id,i);
    	  ue_config[i]->has_transmission_mode = 1;
	  }

      //TODO: Set the aggregated bit-rate of the non-gbr bearer (UL)
      ue_config[i]->ue_aggregated_max_bitrate_ul = get_ue_aggregated_max_bitrate_ul(mod_id,i);
      ue_config[i]->has_ue_aggregated_max_bitrate_ul = 1;

      //TODO: Set the aggregated bit-rate of the non-gbr bearer (DL)
      ue_config[i]->ue_aggregated_max_bitrate_dl = get_ue_aggregated_max_bitrate_dl(mod_id,i);
      ue_config[i]->has_ue_aggregated_max_bitrate_dl = 1;

      //TODO: Set the UE capabilities
      Protocol__PrpUeCapabilities *capabilities;
      capabilities = malloc(sizeof(Protocol__PrpUeCapabilities));
      protocol__prp_ue_capabilities__init(capabilities);
      //TODO: Set half duplex (FDD operation)
      capabilities->has_half_duplex = 1;
      capabilities->half_duplex = 1;//get_half_duplex(i);
      //TODO: Set intra-frame hopping flag
      capabilities->has_intra_sf_hopping = 1;
      capabilities->intra_sf_hopping = 1;//get_intra_sf_hopping(i);
      //TODO: Set support for type 2 hopping with n_sb > 1
      capabilities->has_type2_sb_1 = 1;
      capabilities->type2_sb_1 = 1;//get_type2_sb_1(i);
      //TODO: Set ue category
      capabilities->has_ue_category = 1;
      capabilities->ue_category = 1;//get_ue_category(i);
      //TODO: Set UE support for resource allocation type 1
      capabilities->has_res_alloc_type1 = 1;
      capabilities->res_alloc_type1 = 1;//get_res_alloc_type1(i);
      //Set the capabilites to the message
      ue_config[i]->capabilities = capabilities;
      //TODO: Set UE transmission antenna. One of the PRUTA_* values
      if(get_ue_transmission_antenna(mod_id,i) != -1){
    	  ue_config[i]->has_ue_transmission_antenna = 1;
    	  ue_config[i]->ue_transmission_antenna = get_ue_transmission_antenna(mod_id,i);
	  }
      //TODO: Set tti bundling flag (See ts 36.321)
      if(get_tti_bundling(mod_id,i) != -1){
    	  ue_config[i]->has_tti_bundling = 1;
		  ue_config[i]->tti_bundling = get_tti_bundling(mod_id,i);
	  }
      //TODO: Set the max HARQ retransmission for the UL
      if(get_maxHARQ_TX(mod_id,i) != -1){
    	  ue_config[i]->has_max_harq_tx = 1;
    	  ue_config[i]->max_harq_tx = get_maxHARQ_TX(mod_id,i);
	  }
      //TODO: Fill beta_offset_ack_index (TS 36.213)
      if(get_beta_offset_ack_index(mod_id,i) != -1){
    	  ue_config[i]->has_beta_offset_ack_index = 1;
    	  ue_config[i]->beta_offset_ack_index = get_beta_offset_ack_index(mod_id,i);
	  }
      //TODO: Fill beta_offset_ri_index (TS 36.213)
      if(get_beta_offset_ri_index(mod_id,i) != -1){
    	  ue_config[i]->has_beta_offset_ri_index = 1;
    	  ue_config[i]->beta_offset_ri_index = get_beta_offset_ri_index(mod_id,i);
	  }
      //TODO: Fill beta_offset_cqi_index (TS 36.213)
      if(get_beta_offset_cqi_index(mod_id,i) != -1){
    	  ue_config[i]->has_beta_offset_cqi_index = 1;
    	  ue_config[i]->beta_offset_cqi_index = get_beta_offset_cqi_index(mod_id,i);
	  }
      //TODO: Fill ack_nack_simultaneous_trans (TS 36.213)
      if(get_ack_nack_simultaneous_trans(mod_id,i) != -1){
    	  ue_config[i]->has_ack_nack_simultaneous_trans = 1;
    	  ue_config[i]->ack_nack_simultaneous_trans = get_ack_nack_simultaneous_trans(mod_id,i);
	  }
      //TODO: Fill simultaneous_ack_nack_cqi (TS 36.213)
      if(get_simultaneous_ack_nack_cqi(mod_id,i) != -1){
    	  ue_config[i]->has_simultaneous_ack_nack_cqi = 1;
    	  ue_config[i]->simultaneous_ack_nack_cqi = get_simultaneous_ack_nack_cqi(mod_id,i);
	  }
      //TODO: Set PRACRM_* value regarding aperiodic cqi report mode
      if(get_aperiodic_cqi_rep_mode(mod_id,i) != -1){
    	  ue_config[i]->has_aperiodic_cqi_rep_mode = 1;
	  int mode = get_aperiodic_cqi_rep_mode(mod_id,i);
	  if (mode > 4) {
	    ue_config[i]->aperiodic_cqi_rep_mode = PROTOCOL__PRP_APERIODIC_CQI_REPORT_MODE__PRACRM_NONE;
	  } else {
	    ue_config[i]->aperiodic_cqi_rep_mode = mode;
	  }
      }
      //TODO: Set tdd_ack_nack_feedback
      if(get_tdd_ack_nack_feedback(mod_id, i) != -1){
	ue_config[i]->has_tdd_ack_nack_feedback = 1;
	ue_config[i]->tdd_ack_nack_feedback = get_tdd_ack_nack_feedback(mod_id,i);
      }
      //TODO: Set ack_nack_repetition factor
      if(get_ack_nack_repetition_factor(mod_id, i) != -1){
	ue_config[i]->has_ack_nack_repetition_factor = 1;
	ue_config[i]->ack_nack_repetition_factor = get_ack_nack_repetition_factor(mod_id,i);
      }
      //TODO: Set extended BSR size
      if(get_extended_bsr_size(mod_id, i) != -1){
	ue_config[i]->has_extended_bsr_size = 1;
	ue_config[i]->extended_bsr_size = get_extended_bsr_size(mod_id,i);
      }
      //TODO: Set carrier aggregation support (boolean)
      ue_config[i]->has_ca_support = 0;
      ue_config[i]->ca_support = 0;
      //TODO: Set index of primary cell
      ue_config[i]->has_pcell_carrier_index = 1;
      ue_config[i]->pcell_carrier_index = UE_PCCID(mod_id, i);
      if(ue_config[i]->has_ca_support){
	//TODO: Set cross carrier scheduling support (boolean)
	ue_config[i]->has_cross_carrier_sched_support = 1;
	ue_config[i]->cross_carrier_sched_support = 0;
	//TODO: Set secondary cells configuration
	// We do not set it for now. No carrier aggregation support
	//TODO: Set deactivation timer for secondary cell
	ue_config[i]->has_scell_deactivation_timer = 1;
	ue_config[i]->scell_deactivation_timer = 1;
      }
    }
    ue_config_reply_msg->ue_config = ue_config;
  }
  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if (*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_UE_CONFIG_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->ue_config_reply_msg = ue_config_reply_msg;
  return 0;

 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (ue_config_reply_msg != NULL)
    free(ue_config_reply_msg);
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


/*
 * ************************************
 * eNB Configuration Request and Reply
 * ************************************
 */

int enb_agent_enb_config_request(mid_t mod_id, const void* params, Protocol__ProgranMessage **msg) {

	Protocol__PrpHeader *header;
	xid_t xid = 1;
	if(prp_create_header(xid,PROTOCOL__PRP_TYPE__PRPT_GET_ENB_CONFIG_REQUEST, &header) != 0)
		goto error;

	Protocol__PrpEnbConfigRequest *enb_config_request_msg;
	enb_config_request_msg = malloc(sizeof(Protocol__PrpEnbConfigRequest));

	if(enb_config_request_msg == NULL)
		goto error;

	protocol__prp_enb_config_request__init(enb_config_request_msg);
	enb_config_request_msg->header = header;

	*msg = malloc(sizeof(Protocol__ProgranMessage));
	if(*msg == NULL)
		goto error;

	protocol__progran_message__init(*msg);
	(*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_ENB_CONFIG_REQUEST_MSG;
	(*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__INITIATING_MESSAGE;
	(*msg)->enb_config_request_msg = enb_config_request_msg;
	return 0;

	error:
	  // TODO: Need to make proper error handling
	  if (header != NULL)
	    free(header);
	  if (enb_config_request_msg != NULL)
	    free(enb_config_request_msg);
	  if(*msg != NULL)
	    free(*msg);
	  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
	  return -1;
}

int enb_agent_enb_config_reply(mid_t mod_id, const void *params, Protocol__ProgranMessage **msg) {

  xid_t xid;
  Protocol__ProgranMessage *input = (Protocol__ProgranMessage *)params;
  Protocol__PrpEnbConfigRequest *enb_config_req_msg = input->enb_config_request_msg;
  xid = (enb_config_req_msg->header)->xid;
  
  int i, j, k;
  int cc_id = 0;
  int enb_id = mod_id;
  
  Protocol__PrpHeader *header;
  if(prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_GET_ENB_CONFIG_REPLY, &header) != 0)
    goto error;
  
  Protocol__PrpEnbConfigReply *enb_config_reply_msg;
  enb_config_reply_msg = malloc(sizeof(Protocol__PrpEnbConfigReply));
  if(enb_config_reply_msg == NULL)
    goto error;
  protocol__prp_enb_config_reply__init(enb_config_reply_msg);
  enb_config_reply_msg->header = header;
  
  enb_config_reply_msg->enb_id = mod_id;
  
  
  enb_config_reply_msg->n_cell_config = MAX_NUM_CCs;
  
  Protocol__PrpCellConfig **cell_conf;
  if(enb_config_reply_msg->n_cell_config > 0){
    cell_conf = malloc(sizeof(Protocol__PrpCellConfig *) * enb_config_reply_msg->n_cell_config);
    if(cell_conf == NULL)
      goto error;
    for(i = 0; i < enb_config_reply_msg->n_cell_config; i++){
      cell_conf[i] = malloc(sizeof(Protocol__PrpCellConfig));
      protocol__prp_cell_config__init(cell_conf[i]);
      //TODO: Fill in with actual value, the PCI of this cell
      cell_conf[i]->phy_cell_id = 1;
      cell_conf[i]->has_phy_cell_id = get_cell_id(enb_id,i);
      //TODO: Fill in with actual value, the PLMN cell id of this cell
      cell_conf[i]->cell_id = i;
      cell_conf[i]->has_cell_id = 1;
      //TODO: Fill in with actual value, PUSCH resources in RBs for hopping
      cell_conf[i]->pusch_hopping_offset = get_hopping_offset(enb_id,i);
      cell_conf[i]->has_pusch_hopping_offset = 1;
      //TODO: Fill in with actual value
      if(get_hopping_mode(enb_id,i) == 0){
	 cell_conf[i]->hopping_mode = PROTOCOL__PRP_HOPPING_MODE__PRHM_INTER;
      }else if(get_hopping_mode(enb_id,i) == 1){
	cell_conf[i]->hopping_mode = PROTOCOL__PRP_HOPPING_MODE__PRHM_INTERINTRA;
      }
      cell_conf[i]->has_hopping_mode = 1;
      //TODO: Fill in with actual value, the number of subbands
      cell_conf[i]->n_sb = get_n_SB(enb_id,i);
      cell_conf[i]->has_n_sb = 1;
      //TODO: Fill in with actual value, The number of resource element groups used for PHICH. One of PRPR_
      if(get_phich_resource(enb_id,i) == 0){
	cell_conf[i]->phich_resource = PROTOCOL__PRP_PHICH_RESOURCE__PRPR_ONE_SIXTH; //0
      }else if (get_phich_resource(enb_id,i) == 1){
	cell_conf[i]->phich_resource = PROTOCOL__PRP_PHICH_RESOURCE__PRPR_HALF; //1
      }else if (get_phich_resource(enb_id,i) == 2){
	cell_conf[i]->phich_resource = PROTOCOL__PRP_PHICH_RESOURCE__PRPR_ONE; // 2
      }else if (get_phich_resource(enb_id,i) == 3){
	cell_conf[i]->phich_resource = PROTOCOL__PRP_PHICH_RESOURCE__PRPR_TWO;//3
      }
      cell_conf[i]->has_phich_resource = 1;
      //TODO: Fill in with actual value, one of the PRPD_ values
      if(get_phich_duration(enb_id,i) == 0){
    	cell_conf[i]->phich_duration = PROTOCOL__PRP_PHICH_DURATION__PRPD_NORMAL;
      }else if(get_phich_duration(enb_id,i) == 1){
	cell_conf[i]->phich_duration = PROTOCOL__PRP_PHICH_DURATION__PRPD_EXTENDED;
      }
      cell_conf[i]->has_phich_duration = 1;
      //TODO: Fill in with actual value, See TS 36.211, section 6.9
      cell_conf[i]->init_nr_pdcch_ofdm_sym = get_num_pdcch_symb(enb_id,i);
      cell_conf[i]->has_init_nr_pdcch_ofdm_sym = 1;
      //TODO: Fill in with actual value
      /* Protocol__PrpSiConfig *si_config; */
      /* si_config = malloc(sizeof(Protocol__PrpSiConfig)); */
      /* if(si_config == NULL) */
      /* 	goto error; */
      /* protocol__prp_si_config__init(si_config); */
      /* //TODO: Fill in with actual value, Frame number to apply the SI configuration */
      /* si_config->sfn = 1; */
      /* si_config->has_sfn = 1; */
      /* //TODO: Fill in with actual value, the length of SIB1 in bytes */
      /* si_config->sib1_length = get_sib1_length(enb_id,i); */
      /* si_config->has_sib1_length = 1; */
      /* //TODO: Fill in with actual value, Scheduling window for all SIs in SF */
      /* si_config->si_window_length = (uint32_t) get_si_window_length(enb_id,i); */
      /* si_config->has_si_window_length = 1; */
      /* //TODO: Fill in with actual value, the number of SI messages */
      /* si_config->n_si_message=1; */
      /* Protocol__PrpSiMessage **si_message; */
      /* si_message = malloc(sizeof(Protocol__PrpSiMessage *) * si_config->n_si_message); */
      /* if(si_message == NULL) */
      /* 	goto error; */
      /* for(j = 0; j < si_config->n_si_message; j++){ */
      /* 	si_message[j] = malloc(sizeof(Protocol__PrpSiMessage)); */
      /* 	if(si_message[j] == NULL) */
      /* 	  goto error; */
      /* 	protocol__prp_si_message__init(si_message[j]); */
      /* 	//TODO: Fill in with actual value, Periodicity of SI msg in radio frames */
      /* 	si_message[j]->periodicity = 1;				//SIPeriod */
      /* 	si_message[j]->has_periodicity = 1; */
      /* 	//TODO: Fill in with actual value, rhe length of the SI message in bytes */
      /* 	si_message[j]->length = 10; */
      /* 	si_message[j]->has_length = 1; */
      /* } */
      /* if(si_config->n_si_message > 0){ */
      /* 	si_config->si_message = si_message; */
      /* } */
      /* cell_conf[i]->si_config = si_config; */
      
      //TODO: Fill in with actual value, the DL transmission bandwidth in RBs
      cell_conf[i]->dl_bandwidth = get_N_RB_DL(enb_id,i);
      cell_conf[i]->has_dl_bandwidth = 1;
      //TODO: Fill in with actual value, the UL transmission bandwidth in RBs
      cell_conf[i]->ul_bandwidth = get_N_RB_UL(enb_id,i);
      cell_conf[i]->has_ul_bandwidth = 1;
      //TODO: Fill in with actual value, one of PRUCPL values
      if(get_ul_cyclic_prefix_length(enb_id,i) == 0){
    	  cell_conf[i]->ul_cyclic_prefix_length = PROTOCOL__PRP_UL_CYCLIC_PREFIX_LENGTH__PRUCPL_NORMAL;
      }else if(get_ul_cyclic_prefix_length(enb_id,i) == 1){
	cell_conf[i]->ul_cyclic_prefix_length = PROTOCOL__PRP_UL_CYCLIC_PREFIX_LENGTH__PRUCPL_EXTENDED;      
      }
      cell_conf[i]->has_ul_cyclic_prefix_length = 1;
      //TODO: Fill in with actual value, one of PRUCPL values
      if(get_ul_cyclic_prefix_length(enb_id,i) == 0){
    	  cell_conf[i]->ul_cyclic_prefix_length = PROTOCOL__PRP_DL_CYCLIC_PREFIX_LENGTH__PRDCPL_NORMAL;
      }else if(get_ul_cyclic_prefix_length(enb_id,i) == 1){
	  cell_conf[i]->ul_cyclic_prefix_length = PROTOCOL__PRP_DL_CYCLIC_PREFIX_LENGTH__PRDCPL_EXTENDED;
      }

      cell_conf[i]->has_dl_cyclic_prefix_length = 1;
      //TODO: Fill in with actual value, number of cell specific antenna ports
      cell_conf[i]->antenna_ports_count = 1;
      cell_conf[i]->has_antenna_ports_count = 1;
      //TODO: Fill in with actual value, one of PRDM values
      if(get_duplex_mode(enb_id,i) == 1){
	cell_conf[i]->duplex_mode = PROTOCOL__PRP_DUPLEX_MODE__PRDM_FDD;
      }else if(get_duplex_mode(enb_id,i) == 0){
	cell_conf[i]->duplex_mode = PROTOCOL__PRP_DUPLEX_MODE__PRDM_TDD;
      }
      cell_conf[i]->has_duplex_mode = 1;
      //TODO: Fill in with actual value, DL/UL subframe assignment. TDD only
      cell_conf[i]->subframe_assignment = get_subframe_assignment(enb_id,i);
      cell_conf[i]->has_subframe_assignment = 1;
      //TODO: Fill in with actual value, TDD only. See TS 36.211, table 4.2.1
      cell_conf[i]->special_subframe_patterns = get_special_subframe_assignment(enb_id,i);
      cell_conf[i]->has_special_subframe_patterns = 1;
      //TODO: Fill in with actual value, The MBSFN radio frame period
      cell_conf[i]->n_mbsfn_subframe_config_rfperiod = 5;
      uint32_t *elem_rfperiod;
      elem_rfperiod = (uint32_t *) malloc(sizeof(uint32_t) *cell_conf[i]->n_mbsfn_subframe_config_rfperiod);
      if(elem_rfperiod == NULL)
	goto error;
      for(j = 0; j < cell_conf[i]->n_mbsfn_subframe_config_rfperiod; j++){
	elem_rfperiod[j] = 1;
      }
      cell_conf[i]->mbsfn_subframe_config_rfperiod = elem_rfperiod;
      
      //TODO: Fill in with actual value, The MBSFN radio frame offset
      cell_conf[i]->n_mbsfn_subframe_config_rfoffset = 5;
      uint32_t *elem_rfoffset;
      elem_rfoffset = (uint32_t *) malloc(sizeof(uint32_t) *cell_conf[i]->n_mbsfn_subframe_config_rfoffset);
      if(elem_rfoffset == NULL)
	goto error;
      for(j = 0; j < cell_conf[i]->n_mbsfn_subframe_config_rfoffset; j++){
	elem_rfoffset[j] = 1;
      }
      cell_conf[i]->mbsfn_subframe_config_rfoffset = elem_rfoffset;
      
      //TODO: Fill in with actual value, Bitmap indicating the MBSFN subframes
      cell_conf[i]->n_mbsfn_subframe_config_sfalloc = 5;
      uint32_t *elem_sfalloc;
      elem_sfalloc = (uint32_t *) malloc(sizeof(uint32_t) *cell_conf[i]->n_mbsfn_subframe_config_sfalloc);
      if(elem_sfalloc == NULL)
	goto error;
      for(j = 0; j < cell_conf[i]->n_mbsfn_subframe_config_sfalloc; j++){
	elem_sfalloc[j] = 1;
      }
      cell_conf[i]->mbsfn_subframe_config_sfalloc = elem_sfalloc;
      
      //TODO: Fill in with actual value, See TS 36.211, section 5.7.1
      cell_conf[i]->prach_config_index = get_prach_ConfigIndex(enb_id,i);
      cell_conf[i]->has_prach_config_index = 1;
      //TODO: Fill in with actual value, See TS 36.211, section 5.7.1
      cell_conf[i]->prach_freq_offset = get_prach_FreqOffset(enb_id,i);
      cell_conf[i]->has_prach_freq_offset = 1;
      //TODO: Fill in with actual value, Duration of RA response window in SF
      cell_conf[i]->ra_response_window_size = get_ra_ResponseWindowSize(enb_id,i);
      cell_conf[i]->has_ra_response_window_size = 1;
      //TODO: Fill in with actual value, Timer used for RA
      cell_conf[i]->mac_contention_resolution_timer = get_mac_ContentionResolutionTimer(enb_id,i);
      cell_conf[i]->has_mac_contention_resolution_timer = 1;
      //TODO: Fill in with actual value, See TS 36.321
      cell_conf[i]->max_harq_msg3tx = get_maxHARQ_Msg3Tx(enb_id,i);
      cell_conf[i]->has_max_harq_msg3tx = 1;
      //TODO: Fill in with actual value, See TS 36.213, section 10.1
      cell_conf[i]->n1pucch_an = get_n1pucch_an(enb_id,i);
      cell_conf[i]->has_n1pucch_an = 1;
      //TODO: Fill in with actual value, See TS 36.211, section 5.4
      cell_conf[i]->deltapucch_shift = get_deltaPUCCH_Shift(enb_id,i);
      cell_conf[i]->has_deltapucch_shift = 1;
      //TODO: Fill in with actual value, See TS 36.211, section 5.4
      cell_conf[i]->nrb_cqi = get_nRB_CQI(enb_id,i);
      cell_conf[i]->has_nrb_cqi = 1;
      //TODO: Fill in with actual value, See TS 36.211, table 5.5.3.3-1 and 2
      cell_conf[i]->srs_subframe_config = get_srs_SubframeConfig(enb_id,i);
      cell_conf[i]->has_srs_subframe_config = 1;
      //TODO: Fill in with actual value, See TS 36.211, section 5.5.3.2
      cell_conf[i]->srs_bw_config = get_srs_BandwidthConfig(enb_id,i);
      cell_conf[i]->has_srs_bw_config = 1;
      //TODO: Fill in with actual value, Boolean value. See TS 36.211, section 5.5.3.2. TDD only
      cell_conf[i]->srs_mac_up_pts = get_srs_MaxUpPts(enb_id,i);
      cell_conf[i]->has_srs_mac_up_pts = 1;
      //TODO: Fill in with actual value, One of the PREQ_ values
      if(get_enable64QAM(enb_id,i) == 0){
	cell_conf[i]->enable_64qam = PROTOCOL__PRP_QAM__PREQ_MOD_16QAM;
      }else if(get_enable64QAM(enb_id,i) == 1){
	cell_conf[i]->enable_64qam = PROTOCOL__PRP_QAM__PREQ_MOD_64QAM;
      }
      cell_conf[i]->has_enable_64qam = 1;
      //TODO: Fill in with actual value, Carrier component index
      cell_conf[i]->carrier_index = i;
      cell_conf[i]->has_carrier_index = 1;
    }
    enb_config_reply_msg->cell_config=cell_conf;
  }
  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_ENB_CONFIG_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->enb_config_reply_msg = enb_config_reply_msg;
  
  return 0;
  
 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (enb_config_reply_msg != NULL)
    free(enb_config_reply_msg);
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


//struct enb_agent_map agent_map;
enb_agent_timer_instance_t timer_instance;
int agent_timer_init = 0;
err_code_t enb_agent_init_timer(void){
  
  LOG_I(ENB_AGENT, "init RB tree\n");
  if (!agent_timer_init) {
    RB_INIT(&timer_instance.enb_agent_head);
    agent_timer_init = 1;
  }
 
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
				  xid_t xid,
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
  e->xid = xid;
  e->timer_args = timer_args; 
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
    RB_REMOVE(enb_agent_map, &timer_instance.enb_agent_head, e);
    enb_agent_destroy_progran_message(e->timer_args->msg);
    free(e);
  }
  
  if (timer_remove(timer_id) < 0 ) 
    goto error;
  
  return 0;

 error:
  LOG_E(ENB_AGENT, "timer can't be removed\n");
  return TIMER_REMOVED_FAILED ;
}

err_code_t enb_agent_destroy_timer_by_task_id(xid_t xid) {
  struct enb_agent_timer_element_s *e = NULL;
  long timer_id;
  RB_FOREACH(e, enb_agent_map, &timer_instance.enb_agent_head) {
    if (e->xid == xid) {
      timer_id = e->timer_id;
      RB_REMOVE(enb_agent_map, &timer_instance.enb_agent_head, e);
      enb_agent_destroy_progran_message(e->timer_args->msg);
      free(e);
      if (timer_remove(timer_id) < 0 ) { 
	goto error;
      }
    }
  }
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
    enb_agent_destroy_progran_message(e->timer_args->msg);
    free(e);
  }  

  return 0;

}

void enb_agent_sleep_until(struct timespec *ts, int delay) {
  ts->tv_nsec += delay;
  if(ts->tv_nsec >= 1000*1000*1000){
    ts->tv_nsec -= 1000*1000*1000;
    ts->tv_sec++;
  }
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ts,  NULL);
}

/*
 int i =0;
  RB_FOREACH(e, enb_agent_map, &enb_agent_head) {
    printf("%d: %p\n", i, e); i++;
    }
*/


err_code_t enb_agent_stop_timer(long timer_id){
  
  struct enb_agent_timer_element_s *e=NULL;
  struct enb_agent_timer_element_s search;
  memset(&search, 0, sizeof(struct enb_agent_timer_element_s));
  search.timer_id = timer_id;

  e = RB_FIND(enb_agent_map, &timer_instance.enb_agent_head, &search);

  if (e != NULL ) {
    e->state =  ENB_AGENT_TIMER_STATE_STOPPED;
  }
  
  timer_remove(timer_id);
  
  return 0;
}

struct enb_agent_timer_element_s * get_timer_entry(long timer_id) {
  
  struct enb_agent_timer_element_s search;
  memset(&search, 0, sizeof(struct enb_agent_timer_element_s));
  search.timer_id = timer_id;

  return  RB_FIND(enb_agent_map, &timer_instance.enb_agent_head, &search); 
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

