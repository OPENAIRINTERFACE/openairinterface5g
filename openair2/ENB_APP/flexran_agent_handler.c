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

/*! \file flexran_agent_handler.c
 * \brief FlexRAN agent tx and rx message handler 
 * \author Xenofon Foukas and Navid Nikaein and shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 */

#include "flexran_agent_defs.h"
#include "flexran_agent_common.h"
#include "flexran_agent_mac.h"
#include "flexran_agent_rrc.h"
#include "flexran_agent_pdcp.h"
#include "flexran_agent_timer.h"
#include "flexran_agent_ran_api.h"
#include "common/utils/LOG/log.h"

#include "assertions.h"

flexran_agent_message_decoded_callback agent_messages_callback[][3] = {
  {flexran_agent_hello, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_HELLO_MSG*/
  {flexran_agent_echo_reply, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REQUEST_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REPLY_MSG*/ //Must add handler when receiving echo reply
  {flexran_agent_handle_stats, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REQUEST_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REPLY_MSG*/
  {0, 0, 0}, /*PROTOCOK__FLEXRAN_MESSAGE__MSG_SF_TRIGGER_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_UL_SR_INFO_MSG*/
  {flexran_agent_enb_config_reply, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REQUEST_MSG*/
  {flexran_agent_handle_enb_config_reply, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REPLY_MSG*/
  {flexran_agent_ue_config_reply, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_CONFIG_REQUEST_MSG*/
  {flexran_agent_handle_ue_config_reply, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_CONFIG_REPLY_MSG*/
  {flexran_agent_lc_config_reply, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_LC_CONFIG_REQUEST_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_LC_CONFIG_REPLY_MSG*/
  {flexran_agent_mac_handle_dl_mac_config, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_DL_MAC_CONFIG_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_STATE_CHANGE_MSG*/
  {flexran_agent_control_delegation, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_CONTROL_DELEGATION_MSG*/
  {flexran_agent_reconfiguration, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_AGENT_RECONFIGURATION_MSG*/
  {flexran_agent_rrc_reconfiguration, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_RRC_TRIGGERING_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_UL_MAC_CONFIG_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_DISCONNECT_MSG*/
  {flexran_agent_rrc_trigger_handover, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_HO_COMMAND_MSG*/
};

flexran_agent_message_destruction_callback message_destruction_callback[] = {
  flexran_agent_destroy_hello,
  flexran_agent_destroy_echo_request,
  flexran_agent_destroy_echo_reply,
  flexran_agent_destroy_stats_request,
  flexran_agent_destroy_stats_reply,
  flexran_agent_mac_destroy_sf_trigger,
  flexran_agent_mac_destroy_sr_info,
  flexran_agent_destroy_enb_config_request,
  flexran_agent_destroy_enb_config_reply,
  flexran_agent_destroy_ue_config_request,
  flexran_agent_destroy_ue_config_reply,
  flexran_agent_destroy_lc_config_request,
  flexran_agent_destroy_lc_config_reply,
  flexran_agent_mac_destroy_dl_config,
  flexran_agent_destroy_ue_state_change,
  flexran_agent_destroy_control_delegation,
  flexran_agent_destroy_agent_reconfiguration,
};

/* static const char *flexran_agent_direction2String[] = { */
/*   "", /\* not_set  *\/ */
/*   "originating message", /\* originating message *\/ */
/*   "successfull outcome", /\* successfull outcome *\/ */
/*   "unsuccessfull outcome", /\* unsuccessfull outcome *\/ */
/* }; */


Protocol__FlexranMessage* flexran_agent_handle_message (mid_t mod_id,
							uint8_t *data, 
							uint32_t size){
  
  Protocol__FlexranMessage *decoded_message = NULL;
  Protocol__FlexranMessage *reply_message = NULL;
  err_code_t err_code;
  DevAssert(data != NULL);

  if (flexran_agent_deserialize_message(data, size, &decoded_message) < 0) {
    err_code= PROTOCOL__FLEXRAN_ERR__MSG_DECODING;
    goto error; 
  }
  if ((decoded_message->msg_case > sizeof(agent_messages_callback) / (3 * sizeof(flexran_agent_message_decoded_callback))) || 
      (decoded_message->msg_dir > PROTOCOL__FLEXRAN_DIRECTION__UNSUCCESSFUL_OUTCOME)){
    err_code= PROTOCOL__FLEXRAN_ERR__MSG_NOT_HANDLED;
    goto error;
  }
    
  if (agent_messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1] == NULL) {
    err_code= PROTOCOL__FLEXRAN_ERR__MSG_NOT_SUPPORTED;
    goto error;

  }

  err_code = ((*agent_messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1])(mod_id, (void *) decoded_message, &reply_message));
  if ( err_code < 0 ){
    goto error;
  } else if (err_code == 0) { //If err_code > 1, we do not want to dispose the message yet
    protocol__flexran_message__free_unpacked(decoded_message, NULL);
  }
  return reply_message;
  
error:
  LOG_E(FLEXRAN_AGENT,"errno %d occured\n",err_code);
  return NULL;

}



void * flexran_agent_pack_message(Protocol__FlexranMessage *msg, 
				  int * size){

  void * buffer;
  
  if (flexran_agent_serialize_message(msg, &buffer, size) < 0 ) {
    LOG_E(FLEXRAN_AGENT,"errno %d occured\n",PROTOCOL__FLEXRAN_ERR__MSG_ENCODING);
    goto error;
  }
  
  // free the msg --> later keep this in the data struct and just update the values
  //TODO call proper destroy function
  ((*message_destruction_callback[msg->msg_case-1])(msg));
  
  DevAssert(buffer !=NULL);
  
  LOG_D(FLEXRAN_AGENT,"Serilized the eNB-UE stats reply (size %d)\n", *size);
  
  return buffer;
  
 error : 
  return NULL;   
}

Protocol__FlexranMessage *flexran_agent_handle_timed_task(void *args) {
  err_code_t err_code;
  flexran_agent_timer_args_t *timer_args = (flexran_agent_timer_args_t *) args;

  Protocol__FlexranMessage *timed_task, *reply_message;
  timed_task = timer_args->msg;
  err_code = ((*agent_messages_callback[timed_task->msg_case-1][timed_task->msg_dir-1])(timer_args->mod_id, (void *) timed_task, &reply_message));
  if ( err_code < 0 ){
    goto error;
  }

  return reply_message;
  
 error:
  LOG_E(FLEXRAN_AGENT,"errno %d occured\n",err_code);
  return NULL;
}

Protocol__FlexranMessage* flexran_agent_process_timeout(long timer_id, void* timer_args){
    
  struct flexran_agent_timer_element_s *found = get_timer_entry(timer_id);
 
  if (found == NULL ) goto error;
  LOG_D(FLEXRAN_AGENT, "Found the entry (%p): timer_id is 0x%lx  0x%lx\n", found, timer_id, found->timer_id);
  
  if (timer_args == NULL)
    LOG_W(FLEXRAN_AGENT,"null timer args\n");
  
  return found->cb(timer_args);

 error:
  LOG_E(FLEXRAN_AGENT, "can't get the timer element\n");
  return NULL;
}

err_code_t flexran_agent_destroy_flexran_message(Protocol__FlexranMessage *msg) {
  return ((*message_destruction_callback[msg->msg_case-1])(msg));
}


/* 
  Top Level Statistics Report

 */



int flexran_agent_handle_stats(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg){

  // TODO: Must deal with sanitization of input
  // TODO: Must check if RNTIs and cell ids of the request actually exist
  // TODO: Must resolve conflicts among stats requests

  int i;
  err_code_t err_code = 0;
  xid_t xid;
  uint32_t usec_interval, sec_interval;

  //TODO: We do not deal with multiple CCs at the moment and eNB id is 0
  int enb_id = mod_id;

  //eNB_MAC_INST *eNB = &eNB_mac_inst[enb_id];
  //UE_list_t *eNB_UE_list=  &eNB->UE_list;

  report_config_t report_config;

  uint32_t ue_flags = 0;
  uint32_t c_flags = 0;

  Protocol__FlexranMessage *input = (Protocol__FlexranMessage *)params;

  Protocol__FlexStatsRequest *stats_req = input->stats_request_msg;
  xid = (stats_req->header)->xid;

  // Check the type of request that is made
  switch(stats_req->body_case) {
  case PROTOCOL__FLEX_STATS_REQUEST__BODY_COMPLETE_STATS_REQUEST: ;
    Protocol__FlexCompleteStatsRequest *comp_req = stats_req->complete_stats_request;
    if (comp_req->report_frequency == PROTOCOL__FLEX_STATS_REPORT_FREQ__FLSRF_OFF) {
      /*Disable both periodic and continuous updates*/
      // flexran_agent_disable_cont_stats_update(mod_id);
      flexran_agent_destroy_timer_by_task_id(xid);
      *msg = NULL;
      return 0;
    } else { //One-off, periodical or continuous reporting
      //Set the proper flags
      ue_flags = comp_req->ue_report_flags;
      c_flags = comp_req->cell_report_flags;
      //Create a list of all eNB RNTIs and cells

      //Set the number of UEs and create list with their RNTIs stats configs
      report_config.nr_ue = 0;
      if (flexran_agent_get_rrc_xface(mod_id))
        report_config.nr_ue = flexran_get_rrc_num_ues(mod_id);
      else if (flexran_agent_get_mac_xface(mod_id))
        report_config.nr_ue = flexran_get_mac_num_ues(mod_id);

      if (flexran_agent_get_rrc_xface(mod_id) && flexran_agent_get_mac_xface(mod_id)
          && flexran_get_rrc_num_ues(mod_id) != flexran_get_mac_num_ues(mod_id)) {
        const int nrrc = flexran_get_rrc_num_ues(mod_id);
        const int nmac = flexran_get_mac_num_ues(mod_id);
        report_config.nr_ue = nrrc < nmac ? nrrc : nmac;
        LOG_E(FLEXRAN_AGENT, "%s(): different numbers of UEs in RRC (%d) and MAC (%d), reporting for %d UEs\n",
              __func__, nrrc, nmac, report_config.nr_ue);
      }
      report_config.ue_report_type = malloc(sizeof(ue_report_type_t) * report_config.nr_ue);
      if (report_config.ue_report_type == NULL) {
        // TODO: Add appropriate error code
        err_code = -100;
        goto error;
      }
      if (flexran_agent_get_rrc_xface(mod_id)) {
        rnti_t rntis[report_config.nr_ue];
        flexran_get_rrc_rnti_list(mod_id, rntis, report_config.nr_ue);
        for (i = 0; i < report_config.nr_ue; i++) {
          report_config.ue_report_type[i].ue_rnti = rntis[i];
          report_config.ue_report_type[i].ue_report_flags = ue_flags;
        }
      }
      if (flexran_agent_get_mac_xface(mod_id) && !flexran_agent_get_rrc_xface(mod_id)) {
        for (i = 0; i < report_config.nr_ue; i++) {
          const int UE_id = flexran_get_mac_ue_id(mod_id, i);
          report_config.ue_report_type[i].ue_rnti = flexran_get_mac_ue_crnti(enb_id, UE_id);
          report_config.ue_report_type[i].ue_report_flags = ue_flags;
        }
      }
      //Set the number of CCs and create a list with the cell stats configs
      report_config.nr_cc = MAX_NUM_CCs;
      report_config.cc_report_type = malloc(sizeof(cc_report_type_t) * report_config.nr_cc);
      if (report_config.cc_report_type == NULL) {
        // TODO: Add appropriate error code
        err_code = -100;
        goto error;
      }
      for (i = 0; i < report_config.nr_cc; i++) {
        //TODO: Must fill in the proper cell ids
        report_config.cc_report_type[i].cc_id = i;
        report_config.cc_report_type[i].cc_report_flags = c_flags;
      }
      /* Check if request was periodical */
      if (comp_req->report_frequency == PROTOCOL__FLEX_STATS_REPORT_FREQ__FLSRF_PERIODICAL) {
        /* Create a one off flexran message as an argument for the periodical task */
        Protocol__FlexranMessage *timer_msg = NULL;
        stats_request_config_t request_config;
        request_config.report_type = PROTOCOL__FLEX_STATS_TYPE__FLST_COMPLETE_STATS;
        request_config.report_frequency = PROTOCOL__FLEX_STATS_REPORT_FREQ__FLSRF_ONCE;
        request_config.period = 0;
        /* Need to make sure that the ue flags are saved (Bug) */
        if (report_config.nr_ue == 0) {
          report_config.nr_ue = 1;
          report_config.ue_report_type = malloc(sizeof(ue_report_type_t));
           if (report_config.ue_report_type == NULL) {
             // TODO: Add appropriate error code
             err_code = -100;
             goto error;
           }
           report_config.ue_report_type[0].ue_rnti = 0; // Dummy value
           report_config.ue_report_type[0].ue_report_flags = ue_flags;
        }
        request_config.config = &report_config;
        if (flexran_agent_stats_request(enb_id, xid, &request_config, &timer_msg) == -1) {
          err_code = -100;
          goto error;
        }
        /* Create a timer */
        long timer_id = 0;
        flexran_agent_timer_args_t *timer_args = malloc(sizeof(flexran_agent_timer_args_t));
        memset (timer_args, 0, sizeof(flexran_agent_timer_args_t));
        timer_args->mod_id = enb_id;
        timer_args->msg = timer_msg;
        /*Convert subframes to usec time*/
        usec_interval = 1000*comp_req->sf;
        sec_interval = 0;
        /*add seconds if required*/
        if (usec_interval >= 1000*1000) {
          sec_interval = usec_interval/(1000*1000);
          usec_interval = usec_interval%(1000*1000);
        }
        flexran_agent_create_timer(sec_interval, usec_interval, FLEXRAN_AGENT_DEFAULT,
            enb_id, FLEXRAN_AGENT_TIMER_TYPE_PERIODIC, xid,
            flexran_agent_handle_timed_task,(void*) timer_args, &timer_id);
      } else if (comp_req->report_frequency == PROTOCOL__FLEX_STATS_REPORT_FREQ__FLSRF_CONTINUOUS) {
        /*If request was for continuous updates, disable the previous configuration and
          set up a new one*/
        flexran_agent_disable_cont_stats_update(mod_id);
        stats_request_config_t request_config;
        request_config.report_type = PROTOCOL__FLEX_STATS_TYPE__FLST_COMPLETE_STATS;
        request_config.report_frequency = PROTOCOL__FLEX_STATS_REPORT_FREQ__FLSRF_ONCE;
        request_config.period = 0;
        /* Need to make sure that the ue flags are saved (Bug) */
        if (report_config.nr_ue == 0) {
          report_config.nr_ue = 1;
          report_config.ue_report_type = malloc(sizeof(ue_report_type_t));
          if (report_config.ue_report_type == NULL) {
            // TODO: Add appropriate error code
            err_code = -100;
            goto error;
          }
          report_config.ue_report_type[0].ue_rnti = 0; // Dummy value
          report_config.ue_report_type[0].ue_report_flags = ue_flags;
        }
        request_config.config = &report_config;
        flexran_agent_enable_cont_stats_update(enb_id, xid, &request_config);
      }
    }
    break;
  case PROTOCOL__FLEX_STATS_REQUEST__BODY_CELL_STATS_REQUEST:;
    Protocol__FlexCellStatsRequest *cell_req = stats_req->cell_stats_request;
    // UE report config will be blank
    report_config.nr_ue = 0;
    report_config.ue_report_type = NULL;
    report_config.nr_cc = cell_req->n_cell;
    report_config.cc_report_type = malloc(sizeof(cc_report_type_t) * report_config.nr_cc);
    if (report_config.cc_report_type == NULL) {
      // TODO: Add appropriate error code
      err_code = -100;
      goto error;
    }
    for (i = 0; i < report_config.nr_cc; i++) {
      //TODO: Must fill in the proper cell ids
      report_config.cc_report_type[i].cc_id = cell_req->cell[i];
      report_config.cc_report_type[i].cc_report_flags = cell_req->flags;
    }
    break;
  case PROTOCOL__FLEX_STATS_REQUEST__BODY_UE_STATS_REQUEST:;
    Protocol__FlexUeStatsRequest *ue_req = stats_req->ue_stats_request;
    // Cell report config will be blank
    report_config.nr_cc = 0;
    report_config.cc_report_type = NULL;
    report_config.nr_ue = ue_req->n_rnti;
    report_config.ue_report_type = malloc(sizeof(ue_report_type_t) * report_config.nr_ue);
    if (report_config.ue_report_type == NULL) {
      // TODO: Add appropriate error code
      err_code = -100;
      goto error;
    }
    for (i = 0; i < report_config.nr_ue; i++) {
      const int UE_id = flexran_get_mac_ue_id(mod_id, i);
      report_config.ue_report_type[i].ue_rnti = ue_req->rnti[UE_id];
      report_config.ue_report_type[i].ue_report_flags = ue_req->flags;
    }
    break;
  default:
    //TODO: Add appropriate error code
    err_code = -100;
    goto error;
  }

  if (flexran_agent_stats_reply(enb_id, xid, &report_config, msg )) {
    err_code = PROTOCOL__FLEXRAN_ERR__MSG_BUILD;
    goto error;
  }

  if (report_config.ue_report_type)
    free(report_config.ue_report_type);
  if (report_config.cc_report_type)
    free(report_config.cc_report_type);

  return 0;

 error :
  LOG_E(FLEXRAN_AGENT, "%s(): errno %d occured\n", __func__, err_code);
  return err_code;
}

/*
  Top level reply 
 */

int flexran_agent_stats_reply(mid_t enb_id, xid_t xid, const report_config_t *report_config, Protocol__FlexranMessage **msg){

  Protocol__FlexHeader *header = NULL;
  Protocol__FlexUeStatsReport **ue_report = NULL;
  Protocol__FlexCellStatsReport **cell_report = NULL;
  Protocol__FlexStatsReply *stats_reply_msg = NULL;
  err_code_t err_code = PROTOCOL__FLEXRAN_ERR__UNEXPECTED;
  int i,j;

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_STATS_REPLY, &header) != 0) {
    goto error;
  }

  stats_reply_msg = malloc(sizeof(Protocol__FlexStatsReply));

  if (stats_reply_msg == NULL) {
    goto error;
  }

  protocol__flex_stats_reply__init(stats_reply_msg);
  stats_reply_msg->header = header;

  stats_reply_msg->n_ue_report = report_config->nr_ue;
  stats_reply_msg->n_cell_report = report_config->nr_cc;

  // UE report

  ue_report = malloc(sizeof(Protocol__FlexUeStatsReport *) * report_config->nr_ue);

  if (ue_report == NULL) {
    goto error;
  }
  
  for (i = 0; i < report_config->nr_ue; i++) {

      ue_report[i] = malloc(sizeof(Protocol__FlexUeStatsReport));
      if (ue_report[i] == NULL) {
        goto error;
      }
      protocol__flex_ue_stats_report__init(ue_report[i]);
      ue_report[i]->rnti = report_config->ue_report_type[i].ue_rnti;
      ue_report[i]->has_rnti = 1;
      ue_report[i]->has_flags = 1; /* actual flags are filled in the CMs below */
  
  }

  // cell rpoert 
  
  cell_report = malloc(sizeof(Protocol__FlexCellStatsReport *) * report_config->nr_cc);
  if (cell_report == NULL) {
    goto error;
  }
  
  for (i = 0; i < report_config->nr_cc; i++) {

      cell_report[i] = malloc(sizeof(Protocol__FlexCellStatsReport));
      if(cell_report[i] == NULL) {
          goto error;
      }

      protocol__flex_cell_stats_report__init(cell_report[i]);
      cell_report[i]->carrier_index = report_config->cc_report_type[i].cc_id;
      cell_report[i]->has_carrier_index = 1;
      cell_report[i]->has_flags = 1; /* actual flags are filled in the CMs below */

  }

  /* MAC reply split */
  if (flexran_agent_get_mac_xface(enb_id)
      && flexran_agent_mac_stats_reply(enb_id, report_config,  ue_report, cell_report) < 0) {
    err_code = PROTOCOL__FLEXRAN_ERR__MSG_BUILD;
    goto error;
  }

  /* RRC reply split */
  if (flexran_agent_get_rrc_xface(enb_id)
      && flexran_agent_rrc_stats_reply(enb_id, report_config,  ue_report, cell_report) < 0) {
    err_code = PROTOCOL__FLEXRAN_ERR__MSG_BUILD;
    goto error;
  }

  /* PDCP reply split */
  if (flexran_agent_get_pdcp_xface(enb_id)
      && flexran_agent_pdcp_stats_reply(enb_id, report_config,  ue_report, cell_report) < 0) {
    err_code = PROTOCOL__FLEXRAN_ERR__MSG_BUILD;
    goto error;
  }

  /* GTP reply split, currently performed through RRC module */
  if (flexran_agent_get_rrc_xface(enb_id)
      && flexran_agent_rrc_gtp_stats_reply(enb_id, report_config, ue_report, cell_report) < 0) {
    err_code = PROTOCOL__FLEXRAN_ERR__MSG_BUILD;
    goto error;
  }

  stats_reply_msg->cell_report = cell_report;
  stats_reply_msg->ue_report = ue_report;

 *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL) {
    goto error;
  }
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REPLY_MSG;
  (*msg)->msg_dir =  PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->stats_reply_msg = stats_reply_msg;

  return 0;

error :
  LOG_E(FLEXRAN_AGENT, "errno %d occured\n", err_code);

  if (header != NULL) {
    free(header);
    header = NULL;
  }

  if (stats_reply_msg != NULL) {
    free(stats_reply_msg);
    stats_reply_msg = NULL;
  }

  if (ue_report != NULL) {
    for (j = 0; j < report_config->nr_ue; j++) {
      if (ue_report[j] != NULL) {
        free(ue_report[j]);
      }
    }
    free(ue_report);
    ue_report = NULL;
  }

  if (cell_report != NULL) {
    for (j = 0; j < report_config->nr_cc; j++) {
      if (cell_report[j] != NULL) {
        free(cell_report[j]);
      }
    }
    free(cell_report);
    cell_report = NULL;
  }

  return err_code;
}

/*
  Top Level Request 
 */

int flexran_agent_stats_request(mid_t mod_id,
            xid_t xid,
            const stats_request_config_t *report_config,
            Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header = NULL;
  int i;

  Protocol__FlexStatsRequest *stats_request_msg;
  stats_request_msg = malloc(sizeof(Protocol__FlexStatsRequest));
  if(stats_request_msg == NULL)
    goto error;
  protocol__flex_stats_request__init(stats_request_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_STATS_REQUEST, &header) != 0)
    goto error;

  stats_request_msg->header = header;

  stats_request_msg->type = report_config->report_type;
  stats_request_msg->has_type = 1;

  switch (report_config->report_type) {
  case PROTOCOL__FLEX_STATS_TYPE__FLST_COMPLETE_STATS:
    stats_request_msg->body_case =  PROTOCOL__FLEX_STATS_REQUEST__BODY_COMPLETE_STATS_REQUEST;
    Protocol__FlexCompleteStatsRequest *complete_stats;
    complete_stats = malloc(sizeof(Protocol__FlexCompleteStatsRequest));
    if(complete_stats == NULL)
      goto error;
    protocol__flex_complete_stats_request__init(complete_stats);
    complete_stats->report_frequency = report_config->report_frequency;
    complete_stats->has_report_frequency = 1;
    complete_stats->sf = report_config->period;
    complete_stats->has_sf = 1;
    complete_stats->has_cell_report_flags = 1;
    complete_stats->has_ue_report_flags = 1;
    if (report_config->config->nr_cc > 0) {
      complete_stats->cell_report_flags = report_config->config->cc_report_type[0].cc_report_flags;
    }
    if (report_config->config->nr_ue > 0) {
      complete_stats->ue_report_flags = report_config->config->ue_report_type[0].ue_report_flags;
    }
    stats_request_msg->complete_stats_request = complete_stats;
    break;
  case  PROTOCOL__FLEX_STATS_TYPE__FLST_CELL_STATS:
    stats_request_msg->body_case = PROTOCOL__FLEX_STATS_REQUEST__BODY_CELL_STATS_REQUEST;
     Protocol__FlexCellStatsRequest *cell_stats;
     cell_stats = malloc(sizeof(Protocol__FlexCellStatsRequest));
    if(cell_stats == NULL)
      goto error;
    protocol__flex_cell_stats_request__init(cell_stats);
    cell_stats->n_cell = report_config->config->nr_cc;
    cell_stats->has_flags = 1;
    if (cell_stats->n_cell > 0) {
      uint32_t *cells;
      cells = (uint32_t *) malloc(sizeof(uint32_t)*cell_stats->n_cell);
      for (i = 0; i < cell_stats->n_cell; i++) {
  cells[i] = report_config->config->cc_report_type[i].cc_id;
      }
      cell_stats->cell = cells;
      cell_stats->flags = report_config->config->cc_report_type[i].cc_report_flags;
    }
    stats_request_msg->cell_stats_request = cell_stats;
    break;
  case PROTOCOL__FLEX_STATS_TYPE__FLST_UE_STATS:
    stats_request_msg->body_case = PROTOCOL__FLEX_STATS_REQUEST__BODY_UE_STATS_REQUEST;
     Protocol__FlexUeStatsRequest *ue_stats;
     ue_stats = malloc(sizeof(Protocol__FlexUeStatsRequest));
    if(ue_stats == NULL)
      goto error;
    protocol__flex_ue_stats_request__init(ue_stats);
    ue_stats->n_rnti = report_config->config->nr_ue;
    ue_stats->has_flags = 1;
    if (ue_stats->n_rnti > 0) {
      uint32_t *ues;
      ues = (uint32_t *) malloc(sizeof(uint32_t)*ue_stats->n_rnti);
      for (i = 0; i < ue_stats->n_rnti; i++) {
  ues[i] = report_config->config->ue_report_type[i].ue_rnti;
      }
      ue_stats->rnti = ues;
      ue_stats->flags = report_config->config->ue_report_type[i].ue_report_flags;
    }
    stats_request_msg->ue_stats_request = ue_stats;
    break;
  default:
    goto error;
  }
  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REQUEST_MSG;
  (*msg)->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->stats_request_msg = stats_request_msg;
  return 0;

 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (stats_request_msg != NULL)
    free(stats_request_msg);
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_destroy_stats_request(Protocol__FlexranMessage *msg) {
   if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REQUEST_MSG)
    goto error;
  free(msg->stats_request_msg->header);
  if (msg->stats_request_msg->body_case == PROTOCOL__FLEX_STATS_REQUEST__BODY_CELL_STATS_REQUEST) {
    free(msg->stats_request_msg->cell_stats_request->cell);
  }
  if (msg->stats_request_msg->body_case == PROTOCOL__FLEX_STATS_REQUEST__BODY_UE_STATS_REQUEST) {
    free(msg->stats_request_msg->ue_stats_request->rnti);
  }
  free(msg->stats_request_msg);
  free(msg);
  return 0;

 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_destroy_stats_reply(Protocol__FlexranMessage *msg)
{
  if (msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REPLY_MSG) {
    LOG_E(FLEXRAN_AGENT, "%s(): message is not a msg_stats_reply\n", __func__);
    return -1;
  }

  flexran_agent_mac_destroy_stats_reply(msg->stats_reply_msg);
  flexran_agent_rrc_destroy_stats_reply(msg->stats_reply_msg);
  flexran_agent_pdcp_destroy_stats_reply(msg->stats_reply_msg);
  for (int i = 0; i < msg->stats_reply_msg->n_cell_report; ++i)
    free(msg->stats_reply_msg->cell_report[i]);
  for (int i = 0; i < msg->stats_reply_msg->n_ue_report; ++i)
    free(msg->stats_reply_msg->ue_report[i]);
  free(msg->stats_reply_msg->cell_report);
  free(msg->stats_reply_msg->ue_report);
  free(msg->stats_reply_msg->header);
  free(msg->stats_reply_msg);
  free(msg);
  return 0;
}

err_code_t flexran_agent_disable_cont_stats_update(mid_t mod_id) {
  /*Disable the continuous updates for the MAC*/
  if (pthread_mutex_lock(stats_context[mod_id].mutex)) {
    goto error;
  }
  stats_context[mod_id].cont_update = 0;
  stats_context[mod_id].xid = 0;
  if (stats_context[mod_id].stats_req != NULL) {
    flexran_agent_destroy_flexran_message(stats_context[mod_id].stats_req);
  }
  if (stats_context[mod_id].prev_stats_reply != NULL) {
    flexran_agent_destroy_flexran_message(stats_context[mod_id].prev_stats_reply);
  }
  if (pthread_mutex_unlock(stats_context[mod_id].mutex)) {
    goto error;
  }
  return 0;

 error:
  LOG_E(FLEXRAN_AGENT, "stats_context for eNB %d is not initialized\n", mod_id);
  return -1;

}

err_code_t flexran_agent_enable_cont_stats_update(mid_t mod_id,
                  xid_t xid, stats_request_config_t *stats_req) {
  
  if (pthread_mutex_lock(stats_context[mod_id].mutex)) {
    goto error;
  }

  Protocol__FlexranMessage *req_msg = NULL;

  flexran_agent_stats_request(mod_id, xid, stats_req, &req_msg);

  if (req_msg != NULL) {
    stats_context[mod_id].stats_req = req_msg;
    stats_context[mod_id].prev_stats_reply = NULL;

    stats_context[mod_id].cont_update = 1;
    stats_context[mod_id].xid = xid;
  }

  if (pthread_mutex_unlock(stats_context[mod_id].mutex)) {
    goto error;
  }
  return 0;

 error:
  LOG_E(FLEXRAN_AGENT, "stats_context for eNB %d is not initialized\n", mod_id);
  return -1;
}


err_code_t flexran_agent_init_cont_stats_update(mid_t mod_id) {

  
  /*Initially the continuous update is set to false*/
  stats_context[mod_id].cont_update = 0;
  stats_context[mod_id].is_initialized = 1;
  stats_context[mod_id].stats_req = NULL;
  stats_context[mod_id].prev_stats_reply = NULL;
  stats_context[mod_id].mutex = calloc(1, sizeof(pthread_mutex_t));
  if (stats_context[mod_id].mutex == NULL)
    goto error;
  if (pthread_mutex_init(stats_context[mod_id].mutex, NULL) != 0)
    goto error;

  return 0;

 error:
  return -1;
}

err_code_t flexran_agent_destroy_cont_stats_update(mid_t mod_id) {
  
  stats_context[mod_id].cont_update = 0;
  stats_context[mod_id].is_initialized = 0;
  flexran_agent_destroy_flexran_message(stats_context[mod_id].stats_req);
  flexran_agent_destroy_flexran_message(stats_context[mod_id].prev_stats_reply);
  free(stats_context[mod_id].mutex);

  return 1;
}
