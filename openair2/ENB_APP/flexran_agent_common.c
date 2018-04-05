/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
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

/*! \file flexran_agent_common.c
 * \brief common primitives for all agents 
 * \author Xenofon Foukas, Mohamed Kassem and Navid Nikaein, shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 */

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#include "flexran_agent_common.h"
#include "flexran_agent_common_internal.h"
#include "flexran_agent_extern.h"
#include "flexran_agent_net_comm.h"
#include "flexran_agent_ran_api.h"
#include "PHY/extern.h"
#include "log.h"

#include "SCHED/defs.h"
#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "rrc_eNB_UE_context.h"

/*
 * message primitives
 */

int flexran_agent_serialize_message(Protocol__FlexranMessage *msg, void **buf, int *size) {

  *size = protocol__flexran_message__get_packed_size(msg);
  
  *buf = malloc(*size);
  if (buf == NULL)
    goto error;
  
  protocol__flexran_message__pack(msg, *buf);
  
  return 0;
  
 error:
  LOG_E(FLEXRAN_AGENT, "an error occured\n"); // change the com
  return -1;
}



/* We assume that the buffer size is equal to the message size.
   Should be chekced durint Tx/Rx */
int flexran_agent_deserialize_message(void *data, int size, Protocol__FlexranMessage **msg) {
  *msg = protocol__flexran_message__unpack(NULL, size, data);
  if (*msg == NULL)
    goto error;

  return 0;
  
 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}



int flexran_create_header(xid_t xid, Protocol__FlexType type,  Protocol__FlexHeader **header) {
  
  *header = malloc(sizeof(Protocol__FlexHeader));
  if(*header == NULL)
    goto error;
  
  protocol__flex_header__init(*header);
  (*header)->version = FLEXRAN_VERSION;
  (*header)->has_version = 1; 
  // check if the type is set
  (*header)->type = type;
  (*header)->has_type = 1;
  (*header)->xid = xid;
  (*header)->has_xid = 1;
  return 0;

 error:
  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_hello(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
 
  Protocol__FlexHeader *header = NULL;
  /*TODO: Need to set random xid or xid from received hello message*/
  xid_t xid = 1;

  Protocol__FlexHello *hello_msg;
  hello_msg = malloc(sizeof(Protocol__FlexHello));
  if(hello_msg == NULL)
    goto error;
  protocol__flex_hello__init(hello_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_HELLO, &header) != 0)
    goto error;

  hello_msg->header = header;

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_HELLO_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
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
  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_destroy_hello(Protocol__FlexranMessage *msg) {
  
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_HELLO_MSG)
    goto error;
  
  free(msg->hello_msg->header);
  free(msg->hello_msg);
  free(msg);
  return 0;

 error:
  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_echo_request(mid_t mod_id, const void* params, Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header = NULL;
  /*TODO: Need to set a random xid*/
  xid_t xid = 1;

  Protocol__FlexEchoRequest *echo_request_msg = NULL;
  echo_request_msg = malloc(sizeof(Protocol__FlexEchoRequest));
  if(echo_request_msg == NULL)
    goto error;
  protocol__flex_echo_request__init(echo_request_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_ECHO_REQUEST, &header) != 0)
    goto error;

  echo_request_msg->header = header;

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REQUEST_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
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


int flexran_agent_destroy_echo_request(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REQUEST_MSG)
    goto error;
  
  free(msg->echo_request_msg->header);
  free(msg->echo_request_msg);
  free(msg);
  return 0;
  
 error:
  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}



int flexran_agent_echo_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  
  xid_t xid;
  Protocol__FlexHeader *header = NULL;
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexEchoRequest *echo_req = input->echo_request_msg;
  xid = (echo_req->header)->xid;

  Protocol__FlexEchoReply *echo_reply_msg = NULL;
  echo_reply_msg = malloc(sizeof(Protocol__FlexEchoReply));
  if(echo_reply_msg == NULL)
    goto error;
  protocol__flex_echo_reply__init(echo_reply_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_ECHO_REPLY, &header) != 0)
    goto error;

  echo_reply_msg->header = header;

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
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
  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_destroy_echo_reply(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REPLY_MSG)
    goto error;
  
  free(msg->echo_reply_msg->header);
  free(msg->echo_reply_msg);
  free(msg);
  return 0;
  
 error:
  LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_destroy_enb_config_reply(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REPLY_MSG)
    goto error;
  free(msg->enb_config_reply_msg->header);
  int i, j;
  Protocol__FlexEnbConfigReply *reply = msg->enb_config_reply_msg;
  
  for(i = 0; i < reply->n_cell_config;i++) {
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
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_destroy_ue_config_reply(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_CONFIG_REPLY_MSG)
    goto error;
  free(msg->ue_config_reply_msg->header);
  int i;
  Protocol__FlexUeConfigReply *reply = msg->ue_config_reply_msg;
  
  for(i = 0; i < reply->n_ue_config;i++){
    free(reply->ue_config[i]->capabilities);
    free(reply->ue_config[i]);
  }
  free(reply->ue_config);
  free(reply);
  free(msg);

  return 0;
 error:
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_destroy_lc_config_reply(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_LC_CONFIG_REPLY_MSG)
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
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_destroy_enb_config_request(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REQUEST_MSG)
    goto error;
  free(msg->enb_config_request_msg->header);
  free(msg->enb_config_request_msg);
  free(msg);
  return 0;
  
 error:
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_destroy_ue_config_request(Protocol__FlexranMessage *msg) {
  /* TODO: Deallocate memory for a dynamically allocated UE config message */
  return 0;
}

int flexran_agent_destroy_lc_config_request(Protocol__FlexranMessage *msg) {
  /* TODO: Deallocate memory for a dynamically allocated LC config message */
  return 0;
}

// call this function to start a nanosecond-resolution timer
struct timespec timer_start(void) {
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

int flexran_agent_control_delegation(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {

  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexControlDelegation *control_delegation_msg = input->control_delegation_msg;

  //  struct timespec vartime = timer_start();
  //Write the payload lib into a file in the cache and load the lib
  char lib_name[120];
  char target[512];
  snprintf(lib_name, sizeof(lib_name), "/%s.so", control_delegation_msg->name);
  strcpy(target, RC.flexran[mod_id]->cache_name);
  strcat(target, lib_name);
  
  FILE *f;
  f = fopen(target, "wb");
  fwrite(control_delegation_msg->payload.data, control_delegation_msg->payload.len, 1, f);
  fclose(f);

  //  long time_elapsed_nanos = timer_end(vartime);
  *msg = NULL;
  return 0;

}

int flexran_agent_destroy_control_delegation(Protocol__FlexranMessage *msg) {
  /*TODO: Dealocate memory for a dynamically allocated control delegation message*/
  return 0;
}

int flexran_agent_reconfiguration(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexAgentReconfiguration *agent_reconfiguration_msg = input->agent_reconfiguration_msg;

  apply_reconfiguration_policy(mod_id, agent_reconfiguration_msg->policy, strlen(agent_reconfiguration_msg->policy));

  *msg = NULL;
  return 0;
}

int flexran_agent_destroy_agent_reconfiguration(Protocol__FlexranMessage *msg) {
  /*TODO: Dealocate memory for a dynamically allocated agent reconfiguration message*/
  return 0;
}


int flexran_agent_lc_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {

  xid_t xid;
  Protocol__FlexHeader *header = NULL;
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexLcConfigRequest *lc_config_request_msg = input->lc_config_request_msg;
  xid = (lc_config_request_msg->header)->xid;

  int i, j;

  Protocol__FlexLcConfigReply *lc_config_reply_msg;
  lc_config_reply_msg = malloc(sizeof(Protocol__FlexLcConfigReply));
  if(lc_config_reply_msg == NULL)
    goto error;
  protocol__flex_lc_config_reply__init(lc_config_reply_msg);

  if(flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_GET_LC_CONFIG_REPLY, &header) != 0)
    goto error;

  lc_config_reply_msg->header = header;

  lc_config_reply_msg->n_lc_ue_config = flexran_get_num_ues(mod_id);

  Protocol__FlexLcUeConfig **lc_ue_config;
  if (lc_config_reply_msg->n_lc_ue_config > 0) {
    lc_ue_config = malloc(sizeof(Protocol__FlexLcUeConfig *) * lc_config_reply_msg->n_lc_ue_config);
    if (lc_ue_config == NULL) {
      goto error;
    }
    // Fill the config for each UE
    for (i = 0; i < lc_config_reply_msg->n_lc_ue_config; i++) {
      lc_ue_config[i] = malloc(sizeof(Protocol__FlexLcUeConfig));
      protocol__flex_lc_ue_config__init(lc_ue_config[i]);

      lc_ue_config[i]->has_rnti = 1;
      lc_ue_config[i]->rnti = flexran_get_ue_crnti(mod_id,i);

      //TODO: Set the number of LC configurations that will be reported for this UE
      //Set this according to the current state of the UE. This is only a temporary fix
      int status = 0;
      status = mac_eNB_get_rrc_status(mod_id, flexran_get_ue_crnti(mod_id, i));
      /* TODO needs to be revised and appropriate API to be implemented */
      if (status < RRC_CONNECTED) {
	lc_ue_config[i]->n_lc_config = 0;
      } else if (status == RRC_CONNECTED) {
	lc_ue_config[i]->n_lc_config = 1;
      } else {
	lc_ue_config[i]->n_lc_config = 3;
      }

      Protocol__FlexLcConfig **lc_config;
      if (lc_ue_config[i]->n_lc_config > 0) {
	lc_config = malloc(sizeof(Protocol__FlexLcConfig *) * lc_ue_config[i]->n_lc_config);
	if (lc_config == NULL) {
	  goto error;
	}
	for (j = 0; j < lc_ue_config[i]->n_lc_config; j++) {
	  lc_config[j] = malloc(sizeof(Protocol__FlexLcConfig));
	  protocol__flex_lc_config__init(lc_config[j]);
	 
	  lc_config[j]->has_lcid = 1;
	  lc_config[j]->lcid = j+1;
	 
	  int lcg = flexran_get_lcg(mod_id, i, j+1);
	  if (lcg >= 0 && lcg <= 3) {
	    lc_config[j]->has_lcg = 1;
	    lc_config[j]->lcg = flexran_get_lcg(mod_id, i,j+1);
	  }
	 
	  lc_config[j]->has_direction = 1;
	  lc_config[j]->direction = flexran_get_direction(i,j+1);
	  //TODO: Bearer type. One of FLQBT_* values. Currently only default bearer supported
	  lc_config[j]->has_qos_bearer_type = 1;
	  lc_config[j]->qos_bearer_type = PROTOCOL__FLEX_QOS_BEARER_TYPE__FLQBT_NON_GBR;

	  //TODO: Set the QCI defined in TS 23.203, coded as defined in TS 36.413
	  // One less than the actual QCI value. Needs to be generalized
	  lc_config[j]->has_qci = 1;
	  lc_config[j]->qci = 1;
	  if (lc_config[j]->direction == PROTOCOL__FLEX_QOS_BEARER_TYPE__FLQBT_GBR) {
            /* TODO all of the need to be taken from API */
	    //TODO: Set the max bitrate (UL)
	    lc_config[j]->has_e_rab_max_bitrate_ul = 0;
	    lc_config[j]->e_rab_max_bitrate_ul = 0;
	    //TODO: Set the max bitrate (DL)
	    lc_config[j]->has_e_rab_max_bitrate_dl = 0;
	    lc_config[j]->e_rab_max_bitrate_dl = 0;
	    //TODO: Set the guaranteed bitrate (UL)
	    lc_config[j]->has_e_rab_guaranteed_bitrate_ul = 0;
	    lc_config[j]->e_rab_guaranteed_bitrate_ul = 0;
	    //TODO: Set the guaranteed bitrate (DL)
	    lc_config[j]->has_e_rab_guaranteed_bitrate_dl = 0;
	    lc_config[j]->e_rab_guaranteed_bitrate_dl = 0;
	  }
	}
	lc_ue_config[i]->lc_config = lc_config;
      }
    } // end for UE
    lc_config_reply_msg->lc_ue_config = lc_ue_config;
  } // lc_config_reply_msg->n_lc_ue_config > 0
  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if (*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_LC_CONFIG_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
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
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

/*
 * ************************************
 * UE Configuration Reply
 * ************************************
 */

int flexran_agent_ue_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {

  xid_t xid;
  Protocol__FlexHeader *header = NULL;
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexUeConfigRequest *ue_config_request_msg = input->ue_config_request_msg;
  xid = (ue_config_request_msg->header)->xid;

  int i;

  Protocol__FlexUeConfigReply *ue_config_reply_msg;
  ue_config_reply_msg = malloc(sizeof(Protocol__FlexUeConfigReply));
  if(ue_config_reply_msg == NULL)
    goto error;
  protocol__flex_ue_config_reply__init(ue_config_reply_msg);

  if(flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_GET_UE_CONFIG_REPLY, &header) != 0)
    goto error;

  ue_config_reply_msg->header = header;

  ue_config_reply_msg->n_ue_config = flexran_get_num_ues(mod_id);

  Protocol__FlexUeConfig **ue_config;
  if (ue_config_reply_msg->n_ue_config > 0) {
    ue_config = malloc(sizeof(Protocol__FlexUeConfig *) * ue_config_reply_msg->n_ue_config);
    if (ue_config == NULL) {
      goto error;
    }
    for (i = 0; i < ue_config_reply_msg->n_ue_config; i++) {
      ue_config[i] = malloc(sizeof(Protocol__FlexUeConfig));
      protocol__flex_ue_config__init(ue_config[i]);

      ue_config[i]->rnti = flexran_get_ue_crnti(mod_id,i);
      ue_config[i]->has_rnti = 1;
      ue_config[i]->imsi = flexran_get_ue_imsi(mod_id, i);
      ue_config[i]->has_imsi = 1;
      //TODO: Set the DRX configuration (optional)
      //Not supported for now, so we do not set it

      if (flexran_get_time_alignment_timer(mod_id,i) != -1) {
    	  ue_config[i]->time_alignment_timer = flexran_get_time_alignment_timer(mod_id,i);
    	  ue_config[i]->has_time_alignment_timer = 1;
      }

      if (flexran_get_meas_gap_config(mod_id,i) != -1) {
    	  ue_config[i]->meas_gap_config_pattern = flexran_get_meas_gap_config(mod_id,i);
    	  ue_config[i]->has_meas_gap_config_pattern = 1;
      }
 
      if (ue_config[i]->has_meas_gap_config_pattern == 1 &&
	 ue_config[i]->meas_gap_config_pattern != PROTOCOL__FLEX_MEAS_GAP_CONFIG_PATTERN__FLMGCP_OFF) {
	ue_config[i]->meas_gap_config_sf_offset = flexran_get_meas_gap_config_offset(mod_id,i);
	ue_config[i]->has_meas_gap_config_sf_offset = 1;
      }
      //TODO: Set the SPS configuration (Optional)
      //Not supported for noe, so we do not set it

      //TODO: Set the SR configuration (Optional)
      //We do not set it for now

      //TODO: Set the CQI configuration (Optional)
      //We do not set it for now

      if (flexran_get_ue_transmission_mode(mod_id,i) != -1) {
	ue_config[i]->transmission_mode = flexran_get_ue_transmission_mode(mod_id,i);
	ue_config[i]->has_transmission_mode = 1;
      }

      ue_config[i]->ue_aggregated_max_bitrate_ul = flexran_get_ue_aggregated_max_bitrate_ul(mod_id,i);
      ue_config[i]->has_ue_aggregated_max_bitrate_ul = 1;

      ue_config[i]->ue_aggregated_max_bitrate_dl = flexran_get_ue_aggregated_max_bitrate_dl(mod_id,i);
      ue_config[i]->has_ue_aggregated_max_bitrate_dl = 1;

      Protocol__FlexUeCapabilities *capabilities;
      capabilities = malloc(sizeof(Protocol__FlexUeCapabilities));
      protocol__flex_ue_capabilities__init(capabilities);
      capabilities->has_half_duplex = 1;
      capabilities->half_duplex = flexran_get_half_duplex(mod_id, i);
      capabilities->has_intra_sf_hopping = 1;
      capabilities->intra_sf_hopping = flexran_get_intra_sf_hopping(mod_id, i);
      capabilities->has_type2_sb_1 = 1;
      capabilities->type2_sb_1 = flexran_get_type2_sb_1(mod_id, i);
      capabilities->has_ue_category = 1;
      capabilities->ue_category = flexran_get_ue_category(mod_id, i);
      capabilities->has_res_alloc_type1 = 1;
      capabilities->res_alloc_type1 = flexran_get_res_alloc_type1(mod_id, i);
      //Set the capabilites to the message
      ue_config[i]->capabilities = capabilities;

      if (flexran_get_ue_transmission_antenna(mod_id,i) != -1) {
	ue_config[i]->has_ue_transmission_antenna = 1;
	ue_config[i]->ue_transmission_antenna = flexran_get_ue_transmission_antenna(mod_id,i);
      }

      if (flexran_get_tti_bundling(mod_id,i) != -1) {
	ue_config[i]->has_tti_bundling = 1;
	ue_config[i]->tti_bundling = flexran_get_tti_bundling(mod_id,i);
      }

      if (flexran_get_maxHARQ_TX(mod_id,i) != -1) {
	ue_config[i]->has_max_harq_tx = 1;
	ue_config[i]->max_harq_tx = flexran_get_maxHARQ_TX(mod_id,i);
      }

      if (flexran_get_beta_offset_ack_index(mod_id,i) != -1) {
	ue_config[i]->has_beta_offset_ack_index = 1;
	ue_config[i]->beta_offset_ack_index = flexran_get_beta_offset_ack_index(mod_id,i);
      }

      if (flexran_get_beta_offset_ri_index(mod_id,i) != -1) {
	ue_config[i]->has_beta_offset_ri_index = 1;
    	  ue_config[i]->beta_offset_ri_index = flexran_get_beta_offset_ri_index(mod_id,i);
      }

      if (flexran_get_beta_offset_cqi_index(mod_id,i) != -1) {
	ue_config[i]->has_beta_offset_cqi_index = 1;
	ue_config[i]->beta_offset_cqi_index = flexran_get_beta_offset_cqi_index(mod_id,i);
      }
      
      /* assume primary carrier */
      if (flexran_get_ack_nack_simultaneous_trans(mod_id, i, 0) != -1) {
	ue_config[i]->has_ack_nack_simultaneous_trans = 1;
	ue_config[i]->ack_nack_simultaneous_trans = flexran_get_ack_nack_simultaneous_trans(mod_id, i, 0);
      }
      
      if (flexran_get_simultaneous_ack_nack_cqi(mod_id,i) != -1) {
	ue_config[i]->has_simultaneous_ack_nack_cqi = 1;
	ue_config[i]->simultaneous_ack_nack_cqi = flexran_get_simultaneous_ack_nack_cqi(mod_id,i);
      }
      
      if (flexran_get_aperiodic_cqi_rep_mode(mod_id,i) != -1) {
	ue_config[i]->has_aperiodic_cqi_rep_mode = 1;
	int mode = flexran_get_aperiodic_cqi_rep_mode(mod_id,i);
	if (mode > 4) {
	  ue_config[i]->aperiodic_cqi_rep_mode = PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_NONE;
	} else {
	  ue_config[i]->aperiodic_cqi_rep_mode = mode;
	}
      }
      
      if (flexran_get_tdd_ack_nack_feedback_mode(mod_id, i) != -1) {
	ue_config[i]->has_tdd_ack_nack_feedback = 1;
	ue_config[i]->tdd_ack_nack_feedback = flexran_get_tdd_ack_nack_feedback_mode(mod_id,i);
      }
      
      if(flexran_get_ack_nack_repetition_factor(mod_id, i) != -1) {
	ue_config[i]->has_ack_nack_repetition_factor = 1;
	ue_config[i]->ack_nack_repetition_factor = flexran_get_ack_nack_repetition_factor(mod_id,i);
      }
      
      if (flexran_get_extended_bsr_size(mod_id, i) != -1) {
	ue_config[i]->has_extended_bsr_size = 1;
	ue_config[i]->extended_bsr_size = flexran_get_extended_bsr_size(mod_id,i);
      }
      //TODO: Set carrier aggregation support (boolean)
      ue_config[i]->has_ca_support = 0;
      ue_config[i]->ca_support = 0;

      ue_config[i]->has_pcell_carrier_index = 1;
      ue_config[i]->pcell_carrier_index = UE_PCCID(mod_id, i);
      if(ue_config[i]->has_ca_support){
	//TODO: Set cross carrier scheduling support (boolean)
	ue_config[i]->has_cross_carrier_sched_support = 0;
	ue_config[i]->cross_carrier_sched_support = 0;
	//TODO: Set secondary cells configuration
	// We do not set it for now. No carrier aggregation support
	//TODO: Set deactivation timer for secondary cell
	ue_config[i]->has_scell_deactivation_timer = 0;
	ue_config[i]->scell_deactivation_timer = 0;
      }
    }
    ue_config_reply_msg->ue_config = ue_config;
  }
  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if (*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_CONFIG_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
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
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


/*
 * ************************************
 * eNB Configuration Request and Reply
 * ************************************
 */

int flexran_agent_enb_config_request(mid_t mod_id, const void* params, Protocol__FlexranMessage **msg) {

	Protocol__FlexHeader *header = NULL;
	xid_t xid = 1;

	Protocol__FlexEnbConfigRequest *enb_config_request_msg;
	enb_config_request_msg = malloc(sizeof(Protocol__FlexEnbConfigRequest));
	if(enb_config_request_msg == NULL)
	  goto error;
	protocol__flex_enb_config_request__init(enb_config_request_msg);
	
	if(flexran_create_header(xid,PROTOCOL__FLEX_TYPE__FLPT_GET_ENB_CONFIG_REQUEST, &header) != 0)
	  goto error;

	enb_config_request_msg->header = header;

	*msg = malloc(sizeof(Protocol__FlexranMessage));
	if(*msg == NULL)
	  goto error;

	protocol__flexran_message__init(*msg);
	(*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REQUEST_MSG;
	(*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
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

int flexran_agent_enb_config_reply(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {

  xid_t xid;
  Protocol__FlexHeader *header = NULL;
  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexEnbConfigRequest *enb_config_req_msg = input->enb_config_request_msg;
  xid = (enb_config_req_msg->header)->xid;
  
  int i, j;
  
  Protocol__FlexEnbConfigReply *enb_config_reply_msg;
  enb_config_reply_msg = malloc(sizeof(Protocol__FlexEnbConfigReply));
  if(enb_config_reply_msg == NULL)
    goto error;
  protocol__flex_enb_config_reply__init(enb_config_reply_msg);

  if(flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_GET_ENB_CONFIG_REPLY, &header) != 0)
    goto error;
  
  enb_config_reply_msg->header = header;

  enb_config_reply_msg->enb_id = RC.flexran[mod_id]->agent_id;
  enb_config_reply_msg->has_enb_id = 1;

  enb_config_reply_msg->n_cell_config = MAX_NUM_CCs;
  
  Protocol__FlexCellConfig **cell_conf;
  if(enb_config_reply_msg->n_cell_config > 0){
    cell_conf = malloc(sizeof(Protocol__FlexCellConfig *) * enb_config_reply_msg->n_cell_config);
    if(cell_conf == NULL)
      goto error;
    for(i = 0; i < enb_config_reply_msg->n_cell_config; i++){
      cell_conf[i] = malloc(sizeof(Protocol__FlexCellConfig));
      protocol__flex_cell_config__init(cell_conf[i]);

      cell_conf[i]->phy_cell_id = 1;
      cell_conf[i]->has_phy_cell_id = flexran_get_cell_id(mod_id,i);

      cell_conf[i]->cell_id = i;
      cell_conf[i]->has_cell_id = 1;

      cell_conf[i]->pusch_hopping_offset = flexran_get_hopping_offset(mod_id,i);
      cell_conf[i]->has_pusch_hopping_offset = 1;

      if (flexran_get_hopping_mode(mod_id,i) == 0) {
	cell_conf[i]->hopping_mode = PROTOCOL__FLEX_HOPPING_MODE__FLHM_INTER;
      } else if(flexran_get_hopping_mode(mod_id,i) == 1) {
	cell_conf[i]->hopping_mode = PROTOCOL__FLEX_HOPPING_MODE__FLHM_INTERINTRA;
      }
      cell_conf[i]->has_hopping_mode = 1;

      cell_conf[i]->n_sb = flexran_get_n_SB(mod_id,i);
      cell_conf[i]->has_n_sb = 1;

      if (flexran_get_phich_resource(mod_id,i) == 0) {
	cell_conf[i]->phich_resource = PROTOCOL__FLEX_PHICH_RESOURCE__FLPR_ONE_SIXTH; //0
      } else if (flexran_get_phich_resource(mod_id,i) == 1) {
	cell_conf[i]->phich_resource = PROTOCOL__FLEX_PHICH_RESOURCE__FLPR_HALF; //1
      } else if (flexran_get_phich_resource(mod_id,i) == 2) {
	cell_conf[i]->phich_resource = PROTOCOL__FLEX_PHICH_RESOURCE__FLPR_ONE; // 2
      } else if (flexran_get_phich_resource(mod_id,i) == 3) {
	cell_conf[i]->phich_resource = PROTOCOL__FLEX_PHICH_RESOURCE__FLPR_TWO;//3
      }
      cell_conf[i]->has_phich_resource = 1;

      if (flexran_get_phich_duration(mod_id,i) == 0) {
    	cell_conf[i]->phich_duration = PROTOCOL__FLEX_PHICH_DURATION__FLPD_NORMAL;
      } else if(flexran_get_phich_duration(mod_id,i) == 1) {
	cell_conf[i]->phich_duration = PROTOCOL__FLEX_PHICH_DURATION__FLPD_EXTENDED;
      }
      cell_conf[i]->has_phich_duration = 1;
      cell_conf[i]->init_nr_pdcch_ofdm_sym = flexran_get_num_pdcch_symb(mod_id,i);
      cell_conf[i]->has_init_nr_pdcch_ofdm_sym = 1;
      Protocol__FlexSiConfig *si_config;
      si_config = malloc(sizeof(Protocol__FlexSiConfig));
      if(si_config == NULL)
        goto error;
      protocol__flex_si_config__init(si_config);

      si_config->sfn = flexran_get_current_system_frame_num(mod_id);
      si_config->has_sfn = 1;

      si_config->sib1_length = flexran_get_sib1_length(mod_id,i);
      si_config->has_sib1_length = 1;

      si_config->si_window_length = (uint32_t) flexran_get_si_window_length(mod_id, i);
      si_config->has_si_window_length = 1;

      si_config->n_si_message = 0;

      /* Protocol__FlexSiMessage **si_message; */
      /* si_message = malloc(sizeof(Protocol__FlexSiMessage *) * si_config->n_si_message); */
      /* if(si_message == NULL) */
      /* 	goto error; */
      /* for(j = 0; j < si_config->n_si_message; j++){ */
      /* 	si_message[j] = malloc(sizeof(Protocol__FlexSiMessage)); */
      /* 	if(si_message[j] == NULL) */
      /* 	  goto error; */
      /* 	protocol__flex_si_message__init(si_message[j]); */
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

      cell_conf[i]->si_config = si_config;

      cell_conf[i]->dl_bandwidth = flexran_get_N_RB_DL(mod_id,i);
      cell_conf[i]->has_dl_bandwidth = 1;

      cell_conf[i]->ul_bandwidth = flexran_get_N_RB_UL(mod_id,i);
      cell_conf[i]->has_ul_bandwidth = 1;

      if (flexran_get_ul_cyclic_prefix_length(mod_id, i) == 0) {
	cell_conf[i]->ul_cyclic_prefix_length = PROTOCOL__FLEX_UL_CYCLIC_PREFIX_LENGTH__FLUCPL_NORMAL;
      } else if(flexran_get_ul_cyclic_prefix_length(mod_id, i) == 1) {
	cell_conf[i]->ul_cyclic_prefix_length = PROTOCOL__FLEX_UL_CYCLIC_PREFIX_LENGTH__FLUCPL_EXTENDED;
      }
      cell_conf[i]->has_ul_cyclic_prefix_length = 1;

      if (flexran_get_ul_cyclic_prefix_length(mod_id,i) == 0) {
	cell_conf[i]->ul_cyclic_prefix_length = PROTOCOL__FLEX_DL_CYCLIC_PREFIX_LENGTH__FLDCPL_NORMAL;
      } else if (flexran_get_ul_cyclic_prefix_length(mod_id,i) == 1) {
	cell_conf[i]->ul_cyclic_prefix_length = PROTOCOL__FLEX_DL_CYCLIC_PREFIX_LENGTH__FLDCPL_EXTENDED;
      }

      cell_conf[i]->has_dl_cyclic_prefix_length = 1;
      cell_conf[i]->antenna_ports_count = flexran_get_antenna_ports(mod_id, i);
      cell_conf[i]->has_antenna_ports_count = 1;

      if (flexran_get_duplex_mode(mod_id,i) == 1) {
	cell_conf[i]->duplex_mode = PROTOCOL__FLEX_DUPLEX_MODE__FLDM_FDD;
      } else if(flexran_get_duplex_mode(mod_id,i) == 0) {
	cell_conf[i]->duplex_mode = PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD;
      }
      cell_conf[i]->has_duplex_mode = 1;
      cell_conf[i]->subframe_assignment = flexran_get_subframe_assignment(mod_id, i);
      cell_conf[i]->has_subframe_assignment = 1;
      cell_conf[i]->special_subframe_patterns = flexran_get_special_subframe_assignment(mod_id,i);
      cell_conf[i]->has_special_subframe_patterns = 1;
      //TODO: Fill in with actual value, The MBSFN radio frame period
      cell_conf[i]->n_mbsfn_subframe_config_rfperiod = 0;
      uint32_t *elem_rfperiod;
      elem_rfperiod = (uint32_t *) malloc(sizeof(uint32_t) *cell_conf[i]->n_mbsfn_subframe_config_rfperiod);
      if(elem_rfperiod == NULL)
	goto error;
      for(j = 0; j < cell_conf[i]->n_mbsfn_subframe_config_rfperiod; j++){
	elem_rfperiod[j] = 1;
      }
      cell_conf[i]->mbsfn_subframe_config_rfperiod = elem_rfperiod;

      //TODO: Fill in with actual value, The MBSFN radio frame offset
      cell_conf[i]->n_mbsfn_subframe_config_rfoffset = 0;
      uint32_t *elem_rfoffset;
      elem_rfoffset = (uint32_t *) malloc(sizeof(uint32_t) *cell_conf[i]->n_mbsfn_subframe_config_rfoffset);
      if(elem_rfoffset == NULL)
	goto error;
      for(j = 0; j < cell_conf[i]->n_mbsfn_subframe_config_rfoffset; j++){
	elem_rfoffset[j] = 1;
      }
      cell_conf[i]->mbsfn_subframe_config_rfoffset = elem_rfoffset;

      //TODO: Fill in with actual value, Bitmap indicating the MBSFN subframes
      cell_conf[i]->n_mbsfn_subframe_config_sfalloc = 0;
      uint32_t *elem_sfalloc;
      elem_sfalloc = (uint32_t *) malloc(sizeof(uint32_t) *cell_conf[i]->n_mbsfn_subframe_config_sfalloc);
      if(elem_sfalloc == NULL)
	goto error;
      for(j = 0; j < cell_conf[i]->n_mbsfn_subframe_config_sfalloc; j++){
	elem_sfalloc[j] = 1;
      }
      cell_conf[i]->mbsfn_subframe_config_sfalloc = elem_sfalloc;

      cell_conf[i]->prach_config_index = flexran_get_prach_ConfigIndex(mod_id,i);
      cell_conf[i]->has_prach_config_index = 1;

      cell_conf[i]->prach_freq_offset = flexran_get_prach_FreqOffset(mod_id,i);
      cell_conf[i]->has_prach_freq_offset = 1;

      cell_conf[i]->ra_response_window_size = flexran_get_ra_ResponseWindowSize(mod_id,i);
      cell_conf[i]->has_ra_response_window_size = 1;

      cell_conf[i]->mac_contention_resolution_timer = flexran_get_mac_ContentionResolutionTimer(mod_id,i);
      cell_conf[i]->has_mac_contention_resolution_timer = 1;

      cell_conf[i]->max_harq_msg3tx = flexran_get_maxHARQ_Msg3Tx(mod_id,i);
      cell_conf[i]->has_max_harq_msg3tx = 1;

      cell_conf[i]->n1pucch_an = flexran_get_n1pucch_an(mod_id,i);
      cell_conf[i]->has_n1pucch_an = 1;

      cell_conf[i]->deltapucch_shift = flexran_get_deltaPUCCH_Shift(mod_id,i);
      cell_conf[i]->has_deltapucch_shift = 1;

      cell_conf[i]->nrb_cqi = flexran_get_nRB_CQI(mod_id,i);
      cell_conf[i]->has_nrb_cqi = 1;

      cell_conf[i]->srs_subframe_config = flexran_get_srs_SubframeConfig(mod_id,i);
      cell_conf[i]->has_srs_subframe_config = 1;

      cell_conf[i]->srs_bw_config = flexran_get_srs_BandwidthConfig(mod_id,i);
      cell_conf[i]->has_srs_bw_config = 1;

      cell_conf[i]->srs_mac_up_pts = flexran_get_srs_MaxUpPts(mod_id,i);
      cell_conf[i]->has_srs_mac_up_pts = 1;

      cell_conf[i]->dl_freq = flexran_agent_get_operating_dl_freq (mod_id,i);
      cell_conf[i]->has_dl_freq = 1;

      cell_conf[i]->ul_freq = flexran_agent_get_operating_ul_freq (mod_id, i);
      cell_conf[i]->has_ul_freq = 1;

      cell_conf[i]->eutra_band = flexran_agent_get_operating_eutra_band (mod_id,i);
      cell_conf[i]->has_eutra_band = 1;

      cell_conf[i]->dl_pdsch_power = flexran_agent_get_operating_pdsch_refpower(mod_id, i);
      cell_conf[i]->has_dl_pdsch_power = 1;

      cell_conf[i]->ul_pusch_power = flexran_agent_get_operating_pusch_p0 (mod_id,i);
      cell_conf[i]->has_ul_pusch_power = 1;

      if (flexran_get_enable64QAM(mod_id,i) == 0) {
	cell_conf[i]->enable_64qam = PROTOCOL__FLEX_QAM__FLEQ_MOD_16QAM;
      } else if(flexran_get_enable64QAM(mod_id,i) == 1) {
	cell_conf[i]->enable_64qam = PROTOCOL__FLEX_QAM__FLEQ_MOD_64QAM;
      }
      cell_conf[i]->has_enable_64qam = 1;

      cell_conf[i]->carrier_index = i;
      cell_conf[i]->has_carrier_index = 1;
    }
    enb_config_reply_msg->cell_config=cell_conf;
  }
  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
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
  //LOG_E(FLEXRAN_AGENT, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int flexran_agent_rrc_measurement(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {

  protocol_ctxt_t  ctxt;

  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;
  Protocol__FlexRrcTriggering *triggering = input->rrc_triggering;

  agent_reconf_rrc *reconf_param = malloc(sizeof(agent_reconf_rrc));
  

  reconf_param->trigger_policy = triggering->rrc_trigger;

  struct rrc_eNB_ue_context_s   *ue_context_p = NULL;

  RB_FOREACH(ue_context_p, rrc_ue_tree_s, &(RC.rrc[mod_id]->rrc_ue_head)){


  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, mod_id, ENB_FLAG_YES, ue_context_p->ue_context.rnti, flexran_get_current_frame(mod_id), flexran_get_current_subframe (mod_id), mod_id);
  
  flexran_rrc_eNB_generate_defaultRRCConnectionReconfiguration(&ctxt, ue_context_p, 0, reconf_param);  

  }
  
  
  *msg = NULL;
  return 0;
}


int flexran_agent_destroy_rrc_measurement(Protocol__FlexranMessage *msg){
  // TODO
  return 0;
}



