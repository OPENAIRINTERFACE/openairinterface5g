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

/*! \file flexran_agent_mac.c
 * \brief FlexRAN agent Control Module RRC 
 * \author shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 */

#include "flexran_agent_rrc.h"


#include "liblfds700.h"

#include "log.h"


/*Flags showing if a rrc agent has already been registered*/
unsigned int rrc_agent_registered[NUM_MAX_ENB];

/*Array containing the Agent-RRC interfaces*/
AGENT_RRC_xface *agent_rrc_xface[NUM_MAX_ENB];

/* Ringbuffer related structs used for maintaining the dl rrc config messages */
//message_queue_t *rrc_dl_config_queue;
struct lfds700_misc_prng_state rrc_ps[NUM_MAX_ENB];
struct lfds700_ringbuffer_element *rrc_dl_config_array[NUM_MAX_ENB];
struct lfds700_ringbuffer_state rrc_ringbuffer_state[NUM_MAX_ENB];



void flexran_agent_init_rrc_agent(mid_t mod_id) {
  lfds700_misc_library_init_valid_on_current_logical_core();
  lfds700_misc_prng_init(&rrc_ps[mod_id]);
  int num_elements = RINGBUFFER_SIZE + 1;
  //Allow RINGBUFFER_SIZE messages to be stored in the ringbuffer at any time
  rrc_dl_config_array[mod_id] = malloc( sizeof(struct lfds700_ringbuffer_element) *  num_elements);
  lfds700_ringbuffer_init_valid_on_current_logical_core( &rrc_ringbuffer_state[mod_id], rrc_dl_config_array[mod_id], num_elements, &rrc_ps[mod_id], NULL );
}





void flexran_agent_ue_state_change(mid_t mod_id, uint32_t rnti, uint8_t state_change) {
  int size;
  Protocol__FlexranMessage *msg;
  Protocol__FlexHeader *header;
  void *data;
  int priority = 0;
  err_code_t err_code;

  int xid = 0;

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_UE_STATE_CHANGE, &header) != 0)
    goto error;

  Protocol__FlexUeStateChange *ue_state_change_msg;
  ue_state_change_msg = malloc(sizeof(Protocol__FlexUeStateChange));
  if(ue_state_change_msg == NULL) {
    goto error;
  }
  protocol__flex_ue_state_change__init(ue_state_change_msg);
  ue_state_change_msg->has_type = 1;
  ue_state_change_msg->type = state_change;

  Protocol__FlexUeConfig *config;
  config = malloc(sizeof(Protocol__FlexUeConfig));
  if (config == NULL) {
    goto error;
  }
  protocol__flex_ue_config__init(config);
  if (state_change == PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED) {
    // Simply set the rnti of the UE
    config->has_rnti = 1;
    config->rnti = rnti;
  } else if (state_change == PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_UPDATED
       || state_change == PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_ACTIVATED) {
        int i = find_UE_id(mod_id, rnti);
      config->has_rnti = 1;
      config->rnti = rnti;
        if(flexran_get_time_alignment_timer(mod_id,i) != -1) {
          config->time_alignment_timer = flexran_get_time_alignment_timer(mod_id,i);
          config->has_time_alignment_timer = 1;
        }
        if(flexran_get_meas_gap_config(mod_id,i) != -1){
          config->meas_gap_config_pattern = flexran_get_meas_gap_config(mod_id,i);
            config->has_meas_gap_config_pattern = 1;
        }
        if(config->has_meas_gap_config_pattern == 1 &&
         config->meas_gap_config_pattern != PROTOCOL__FLEX_MEAS_GAP_CONFIG_PATTERN__FLMGCP_OFF) {
        config->meas_gap_config_sf_offset = flexran_get_meas_gap_config_offset(mod_id,i);
        config->has_meas_gap_config_sf_offset = 1;
        }
        //TODO: Set the SPS configuration (Optional)
        //Not supported for now, so we do not set it

        //TODO: Set the SR configuration (Optional)
        //We do not set it for now

        //TODO: Set the CQI configuration (Optional)
        //We do not set it for now
      
      if(flexran_get_ue_transmission_mode(mod_id,i) != -1) {
          config->transmission_mode = flexran_get_ue_transmission_mode(mod_id,i);
          config->has_transmission_mode = 1;
        }

      config->ue_aggregated_max_bitrate_ul = flexran_get_ue_aggregated_max_bitrate_ul(mod_id,i);
        config->has_ue_aggregated_max_bitrate_ul = 1;

      config->ue_aggregated_max_bitrate_dl = flexran_get_ue_aggregated_max_bitrate_dl(mod_id,i);
        config->has_ue_aggregated_max_bitrate_dl = 1;

        //TODO: Set the UE capabilities
        Protocol__FlexUeCapabilities *c_capabilities;
        c_capabilities = malloc(sizeof(Protocol__FlexUeCapabilities));
        protocol__flex_ue_capabilities__init(c_capabilities);
        //TODO: Set half duplex (FDD operation)
        c_capabilities->has_half_duplex = 0;
        c_capabilities->half_duplex = 1;//flexran_get_half_duplex(i);
        //TODO: Set intra-frame hopping flag
        c_capabilities->has_intra_sf_hopping = 0;
        c_capabilities->intra_sf_hopping = 1;//flexran_get_intra_sf_hopping(i);
        //TODO: Set support for type 2 hopping with n_sb > 1
        c_capabilities->has_type2_sb_1 = 0;
        c_capabilities->type2_sb_1 = 1;//flexran_get_type2_sb_1(i);
        //TODO: Set ue category
        c_capabilities->has_ue_category = 0;
        c_capabilities->ue_category = 1;//flexran_get_ue_category(i);
        //TODO: Set UE support for resource allocation type 1
        c_capabilities->has_res_alloc_type1 = 0;
        c_capabilities->res_alloc_type1 = 1;//flexran_get_res_alloc_type1(i);
        //Set the capabilites to the message
        config->capabilities = c_capabilities;
      
        if(flexran_get_ue_transmission_antenna(mod_id,i) != -1) {
        config->has_ue_transmission_antenna = 1;
        config->ue_transmission_antenna = flexran_get_ue_transmission_antenna(mod_id,i);
        }

        if(flexran_get_tti_bundling(mod_id,i) != -1) {
        config->has_tti_bundling = 1;
        config->tti_bundling = flexran_get_tti_bundling(mod_id,i);
        }

        if(flexran_get_maxHARQ_TX(mod_id,i) != -1){
        config->has_max_harq_tx = 1;
        config->max_harq_tx = flexran_get_maxHARQ_TX(mod_id,i);
        }

        if(flexran_get_beta_offset_ack_index(mod_id,i) != -1) {
        config->has_beta_offset_ack_index = 1;
        config->beta_offset_ack_index = flexran_get_beta_offset_ack_index(mod_id,i);
        }

        if(flexran_get_beta_offset_ri_index(mod_id,i) != -1) {
        config->has_beta_offset_ri_index = 1;
        config->beta_offset_ri_index = flexran_get_beta_offset_ri_index(mod_id,i);
        }

        if(flexran_get_beta_offset_cqi_index(mod_id,i) != -1) {
        config->has_beta_offset_cqi_index = 1;
        config->beta_offset_cqi_index = flexran_get_beta_offset_cqi_index(mod_id,i);
        }

        if(flexran_get_ack_nack_simultaneous_trans(mod_id,i) != -1) {
        config->has_ack_nack_simultaneous_trans = 1;
        config->ack_nack_simultaneous_trans = flexran_get_ack_nack_simultaneous_trans(mod_id,i);
        }

        if(flexran_get_simultaneous_ack_nack_cqi(mod_id,i) != -1) {
        config->has_simultaneous_ack_nack_cqi = 1;
        config->simultaneous_ack_nack_cqi = flexran_get_simultaneous_ack_nack_cqi(mod_id,i);
        }

        if(flexran_get_aperiodic_cqi_rep_mode(mod_id,i) != -1) {
        config->has_aperiodic_cqi_rep_mode = 1;
        int mode = flexran_get_aperiodic_cqi_rep_mode(mod_id,i);
        if (mode > 4) {
          config->aperiodic_cqi_rep_mode = PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_NONE;
        } else {
          config->aperiodic_cqi_rep_mode = mode;
        }
        }

        if(flexran_get_tdd_ack_nack_feedback(mod_id, i) != -1) {
        config->has_tdd_ack_nack_feedback = 1;
        config->tdd_ack_nack_feedback = flexran_get_tdd_ack_nack_feedback(mod_id,i);
        }

        if(flexran_get_ack_nack_repetition_factor(mod_id, i) != -1) {
        config->has_ack_nack_repetition_factor = 1;
        config->ack_nack_repetition_factor = flexran_get_ack_nack_repetition_factor(mod_id,i);
        }

        if(flexran_get_extended_bsr_size(mod_id, i) != -1) {
        config->has_extended_bsr_size = 1;
        config->extended_bsr_size = flexran_get_extended_bsr_size(mod_id,i);
        }

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
        config->has_scell_deactivation_timer = 0;
        config->scell_deactivation_timer = 0;
        }
  } else if (state_change == PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_MOVED) {
    // TODO: Not supported for now. Leave blank
  }

  ue_state_change_msg->config = config;
  msg = malloc(sizeof(Protocol__FlexranMessage));
  if (msg == NULL) {
    goto error;
  }
  protocol__flexran_message__init(msg);
  msg->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_STATE_CHANGE_MSG;
  msg->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  msg->ue_state_change_msg = ue_state_change_msg;

  data = flexran_agent_pack_message(msg, &size);
  /*Send sr info using the MAC channel of the eNB*/
  if (flexran_agent_msg_send(mod_id, FLEXRAN_AGENT_DEFAULT, data, size, priority)) {
    err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
    goto error;
  }

  LOG_D(FLEXRAN_AGENT,"sent message with size %d\n", size);
  return;
 error:
  LOG_E(FLEXRAN_AGENT, "Could not send UE state message becasue of %d \n",err_code);
}



int flexran_agent_destroy_ue_state_change(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_STATE_CHANGE_MSG)
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

/* this is called by RRC as a part of rrc xface  . The controller previously requested  this*/ 
void flexran_trigger_rrc_measurements (mid_t mod_id, MeasResults_t*  measResults) {

  // int i, m, k;
  int                   priority = 0; // Warning Preventing
  void                  *data;
  int                   size;
  err_code_t             err_code;
  Protocol__FlexUeStatsReport **ue_report;
  Protocol__FlexCellStatsReport **cell_report;
  Protocol__FlexStatsReply *stats_reply_msg;
  Protocol__FlexranMessage *msg;

  Protocol__FlexHeader *header;
  int xid = 0;
  int i;

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_STATS_REPLY, &header) != 0)
    goto error;



  stats_reply_msg = malloc(sizeof(Protocol__FlexStatsReply));

  if (stats_reply_msg == NULL)
    goto error;

  protocol__flex_stats_reply__init(stats_reply_msg);
  stats_reply_msg->header = header;
  stats_reply_msg->n_ue_report = 1;
  stats_reply_msg->n_cell_report = 1;

/****** LOCK ******************************************************************/
  // pthread_spin_lock(&rrc_meas_t_lock);

  // struct rrc_meas_trigg *ctxt;
  // ctxt = rrc_meas_get_trigg(p->rnti, p->meas->measId);

  // pthread_spin_unlock(&rrc_meas_t_lock);
/****** UNLOCK ****************************************************************/

  // if (ctxt == NULL) {

  //   flexran_RRC_meas_reconf(p->rnti, -1, p->meas->measId, NULL, NULL);
  //    Free the measurement report received from UE. 
  //   ASN_STRUCT_FREE(asn_DEF_MeasResults, p->meas);
  //   /* Free the params. */
  //   free(p);
  //   return 0;
  // }


  /* Check here whether trigger is registered in agent and then proceed.
  */
  // if (em_has_trigger(mod_id, ctxt->t_id, RRC_MEAS_TRIGGER) == 0) {

  //   flexran_RRC_meas_reconf(p->rnti, -1, p->meas->measId, NULL, NULL);
  //   /* Trigger does not exist in agent so remove from wrapper as well. */
  //   if (rrc_meas_rem_trigg(ctxt) < 0) {
  //     goto error;
  //   }
  // }

  


  /* Set the RNTI of the UE. */
  // repl->rnti = p->rnti;
  /* Set the request status. */
  // if (p->reconfig_success == 0) {
  //   repl->status = STATS_REQ_STATUS__SREQS_FAILURE;
  //   goto error;
  // }
  /* Successful outcome. */
  // repl->status = STATS_REQ_STATUS__SREQS_SUCCESS;
  /* Set the measurement ID of measurement. */
  // repl->has_measid = 1;
  // repl->measid = measResults->measId;
  /* Fill the Primary Cell RSRP and RSRQ. */
  // repl->has_pcell_rsrp = 1;
  // repl->has_pcell_rsrq = 1;
  // #ifdef Rel10
    // repl->has_pcell_rsrp = 1;
    // repl->has_pcell_rsrq = 1;
    // repl->pcell_rsrp = measResults2->measResultPCell.rsrpResult - 140;
    // repl->pcell_rsrq = (measResults2->measResultPCell.rsrqResult)/2 - 20;

    ue_report = malloc(sizeof(Protocol__FlexUeStatsReport *) * 1);
          if (ue_report == NULL)
            goto error;

    for (i = 0; i < 1; i++) {

      ue_report[i] = malloc(sizeof(Protocol__FlexUeStatsReport));
       if(ue_report[i] == NULL)
          goto error;
      protocol__flex_ue_stats_report__init(ue_report[i]);
      ue_report[i]->rnti = flexran_get_ue_crnti(mod_id, 0);
      ue_report[i]->has_rnti = 1;
       ue_report[i]->flags = 65536;
       ue_report[i]->has_flags = 1;
  
    }

    cell_report = malloc(sizeof(Protocol__FlexCellStatsReport *) * 1);
    if (cell_report == NULL)
       goto error;
  
     for (i = 0; i < 1; i++) {

      cell_report[i] = malloc(sizeof(Protocol__FlexCellStatsReport));
      if(cell_report[i] == NULL)
          goto error;

      protocol__flex_cell_stats_report__init(cell_report[i]);
      cell_report[i]->carrier_index = 0; //report_config->cc_report_type[i].cc_id;
      cell_report[i]->has_carrier_index = 1;
      cell_report[i]->flags = 0; // report_config->cc_report_type[i].cc_report_flags;
      cell_report[i]->has_flags = 1;
     }
 

    Protocol__FlexRrcMeasurements *rrc_measurements;
    rrc_measurements = malloc(sizeof(Protocol__FlexRrcMeasurements));
    if (rrc_measurements == NULL)
        goto error;
    protocol__flex_rrc_measurements__init(rrc_measurements);
                            
    rrc_measurements->measid = measResults->measId;
    rrc_measurements->has_measid = 1;

    rrc_measurements->pcell_rsrp = measResults->measResultPCell.rsrpResult - 140;
    rrc_measurements->has_pcell_rsrp = 1;

    rrc_measurements->pcell_rsrq = (measResults->measResultPCell.rsrqResult)/2 - 20;                          
    rrc_measurements->has_pcell_rsrq = 1 ;

    ue_report[0]->rrc_measurements = rrc_measurements;



   

  // #else
  //   repl->has_pcell_rsrp = 1;
  //   repl->has_pcell_rsrq = 1;
  //   repl->pcell_rsrp = RSRP_meas_mapping[meas->
  //                       measResultServCell.rsrpResult];
  //   repl->pcell_rsrq = RSRQ_meas_mapping[meas->
  //                       measResultServCell.rsrqResult];
  // #endif

  // repl->neigh_meas = NULL;

  // if (meas->measResultNeighCells != NULL) {
  //   /*
  //   * Neighboring cells measurements performed by UE.
  //   */
  //   NeighCellsMeasurements *neigh_meas;
  //   neigh_meas = malloc(sizeof(NeighCellsMeasurements));
  //   if (neigh_meas == NULL)
  //     goto error;
  //   neigh_cells_measurements__init(neigh_meas);

  //   /* EUTRAN RRC Measurements. */
  //   if (meas->measResultNeighCells->present ==
  //         MeasResults__measResultNeighCells_PR_measResultListEUTRA) {

  //     MeasResultListEUTRA_t meas_list = meas->measResultNeighCells->
  //                         choice.measResultListEUTRA;
  //     /* Set the number of EUTRAN measurements present in report. */
  //     neigh_meas->n_eutra_meas = meas_list.list.count;
  //     if (neigh_meas->n_eutra_meas > 0) {
  //       /* Initialize EUTRAN measurements. */
  //       EUTRAMeasurements **eutra_meas;
  //       eutra_meas = malloc(sizeof(EUTRAMeasurements *) *
  //                         neigh_meas->n_eutra_meas);
  //       for (i = 0; i < neigh_meas->n_eutra_meas; i++) {
  //         eutra_meas[i] = malloc(sizeof(EUTRAMeasurements));
  //         eutra_measurements__init(eutra_meas[i]);
  //         /* Fill in the physical cell identifier. */
  //         eutra_meas[i]->has_phys_cell_id = 1;
  //         eutra_meas[i]->phys_cell_id = meas_list.list.array[i]->
  //                                 physCellId;
  //         // log_i(agent,"PCI of Target %d", eutra_meas[i]->phys_cell_id);
  //         /* Check for Reference signal measurements. */
  //         if (&(meas_list.list.array[i]->measResult)) {
  //           /* Initialize Ref. signal measurements. */
  //           EUTRARefSignalMeas *meas_result;
  //           meas_result = malloc(sizeof(EUTRARefSignalMeas));
  //           eutra_ref_signal_meas__init(meas_result);

  //           if (meas_list.list.array[i]->measResult.rsrpResult) {
  //             meas_result->has_rsrp = 1;
  //             meas_result->rsrp = RSRP_meas_mapping[*(meas_list.
  //                   list.array[i]->measResult.rsrpResult)];
  //             // log_i(agent,"RSRP of Target %d", meas_result->rsrp);
  //           }

  //           if (meas_list.list.array[i]->measResult.rsrqResult) {
  //             meas_result->has_rsrq = 1;
  //             meas_result->rsrq = RSRQ_meas_mapping[*(meas_list.
  //                   list.array[i]->measResult.rsrqResult)];
  //             // log_i(agent,"RSRQ of Target %d", meas_result->rsrq);
  //           }
  //           eutra_meas[i]->meas_result = meas_result;
  //         }
  //         /* Check for CGI measurements. */
  //         if (meas_list.list.array[i]->cgi_Info) {
  //           /* Initialize CGI measurements. */
  //           EUTRACgiMeasurements *cgi_meas;
  //           cgi_meas = malloc(sizeof(EUTRACgiMeasurements));
  //           eutra_cgi_measurements__init(cgi_meas);

  //           /* EUTRA Cell Global Identity (CGI). */
  //           CellGlobalIdEUTRA *cgi;
  //           cgi = malloc(sizeof(CellGlobalIdEUTRA));
  //           cell_global_id__eutra__init(cgi);

  //           cgi->has_cell_id = 1;
  //           CellIdentity_t cId = meas_list.list.array[i]->
  //                     cgi_Info->cellGlobalId.cellIdentity;
  //           cgi->cell_id = (cId.buf[0] << 20) + (cId.buf[1] << 12) +
  //                   (cId.buf[2] << 4) + (cId.buf[3] >> 4);

  //           /* Public land mobile network identifier of neighbor
  //            * cell.
  //            */
  //           PlmnIdentity *plmn_id;
  //           plmn_id = malloc(sizeof(PlmnIdentity));
  //           plmn_identity__init(plmn_id);

  //           MNC_t mnc = meas_list.list.array[i]->
  //                 cgi_Info->cellGlobalId.plmn_Identity.mnc;

  //           plmn_id->has_mnc = 1;
  //           plmn_id->mnc = 0;
  //           for (m = 0; m < mnc.list.count; m++) {
  //             plmn_id->mnc += *mnc.list.array[m] *
  //               ((uint32_t) pow(10, mnc.list.count - m - 1));
  //           }

  //           MCC_t *mcc = meas_list.list.array[i]->
  //                 cgi_Info->cellGlobalId.plmn_Identity.mcc;

  //           plmn_id->has_mcc = 1;
  //           plmn_id->mcc = 0;
  //           for (m = 0; m < mcc->list.count; m++) {
  //             plmn_id->mcc += *mcc->list.array[m] *
  //               ((uint32_t) pow(10, mcc->list.count - m - 1));
  //           }

  //           TrackingAreaCode_t tac = meas_list.list.array[i]->
  //                         cgi_Info->trackingAreaCode;

  //           cgi_meas->has_tracking_area_code = 1;
  //           cgi_meas->tracking_area_code = (tac.buf[0] << 8) +
  //                               (tac.buf[1]);

  //           PLMN_IdentityList2_t *plmn_l = meas_list.list.array[i]->
  //                         cgi_Info->plmn_IdentityList;

  //           cgi_meas->n_plmn_id = plmn_l->list.count;
  //           /* Set the PLMN ID list in CGI measurements. */
  //           PlmnIdentity **plmn_id_l;
  //           plmn_id_l = malloc(sizeof(PlmnIdentity *) *
  //                           cgi_meas->n_plmn_id);

  //           MNC_t mnc2;
  //           MCC_t *mcc2;
  //           for (m = 0; m < cgi_meas->n_plmn_id; m++) {
  //             plmn_id_l[m] = malloc(sizeof(PlmnIdentity));
  //             plmn_identity__init(plmn_id_l[m]);

  //             mnc2 = plmn_l->list.array[m]->mnc;
  //             plmn_id_l[m]->has_mnc = 1;
  //             plmn_id_l[m]->mnc = 0;
  //             for (k = 0; k < mnc2.list.count; k++) {
  //               plmn_id_l[m]->mnc += *mnc2.list.array[k] *
  //               ((uint32_t) pow(10, mnc2.list.count - k - 1));
  //             }

  //             mcc2 = plmn_l->list.array[m]->mcc;
  //             plmn_id_l[m]->has_mcc = 1;
  //             plmn_id_l[m]->mcc = 0;
  //             for (k = 0; k < mcc2->list.count; k++) {
  //               plmn_id_l[m]->mcc += *mcc2->list.array[k] *
  //               ((uint32_t) pow(10, mcc2->list.count - k - 1));
  //             }
  //           }
  //           cgi_meas->plmn_id = plmn_id_l;
  //           eutra_meas[i]->cgi_meas = cgi_meas;
  //         }
  //       }
  //       neigh_meas->eutra_meas = eutra_meas;
  //     }
  //   }
  //   repl->neigh_meas = neigh_meas;
  // }
  /* Attach the RRC measurement reply message to RRC measurements message. */
  // mrrc_meas->repl = repl;
  /* Attach RRC measurement message to triggered event message. */
  // te->mrrc_meas = mrrc_meas;
  // te->has_action = 0;
  /* Attach the triggered event message to main message. */
  // reply->te = te;

  /* Send the report to controller. */
  // if (flexran_agent_msg_send(b_id, reply) < 0) {
  //   goto error;
  // }

  /* Free the measurement report received from UE. */
  // ASN_STRUCT_FREE(asn_DEF_MeasResults, p->meas);
  /* Free the params. */
  // free(p);


  stats_reply_msg->cell_report = cell_report;
    
  stats_reply_msg->ue_report = ue_report;
  
  msg = malloc(sizeof(Protocol__FlexranMessage));
  if(msg == NULL)
    goto error;
  protocol__flexran_message__init(msg);
  msg->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REPLY_MSG;
  msg->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  msg->stats_reply_msg = stats_reply_msg;
  
  data = flexran_agent_pack_message(msg, &size);
  
  
  if (flexran_agent_msg_send(mod_id, FLEXRAN_AGENT_DEFAULT, data, size, priority)) {
  
    err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
    goto error;
  }
  
   LOG_I(FLEXRAN_AGENT,"RRC Trigger is done  \n");

  return;

  error:

    LOG_E(FLEXRAN_AGENT, "Could not send UE state message becasue of %d \n",err_code);
    /* Free the measurement report received from UE. */
    // ASN_STRUCT_FREE(asn_DEF_MeasResults, p->meas);
    /* Free the params. */
    // free(p);
    // return -1;
}


int flexran_agent_register_rrc_xface(mid_t mod_id, AGENT_RRC_xface *xface) {
  if (rrc_agent_registered[mod_id]) {
    LOG_E(RRC, "RRC agent for eNB %d is already registered\n", mod_id);
    return -1;
  }

//  xface->flexran_agent_send_update_rrc_stats = flexran_agent_send_update_rrc_stats;
  
  xface->flexran_agent_notify_ue_state_change = flexran_agent_ue_state_change;
  xface->flexran_trigger_rrc_measurements = flexran_trigger_rrc_measurements;

  rrc_agent_registered[mod_id] = 1;
  agent_rrc_xface[mod_id] = xface;

  return 0;
}

int flexran_agent_unregister_rrc_xface(mid_t mod_id, AGENT_RRC_xface *xface) {

  //xface->agent_ctxt = NULL;
//  xface->flexran_agent_send_update_rrc_stats = NULL;

  xface->flexran_agent_notify_ue_state_change = NULL;
  xface->flexran_trigger_rrc_measurements = NULL;
  rrc_agent_registered[mod_id] = 0;
  agent_rrc_xface[mod_id] = NULL;

  return 0;
}
