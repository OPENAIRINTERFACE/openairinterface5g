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

/*! \file flexran_agent_mac.c
 * \brief FlexRAN agent Control Module RRC 
 * \author shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 */

#include "flexran_agent_rrc.h"
#include "flexran_agent_ran_api.h"

#include "liblfds700.h"

#include "common/utils/LOG/log.h"

/*Trigger boolean for RRC measurement*/
bool triggered_rrc = false;

/*Array containing the Agent-RRC interfaces*/
AGENT_RRC_xface *agent_rrc_xface[NUM_MAX_ENB];


void flexran_agent_ue_state_change(mid_t mod_id, uint32_t rnti, uint8_t state_change) {
  int size;
  Protocol__FlexranMessage *msg = NULL;
  Protocol__FlexHeader *header = NULL;
  void *data;
  int priority = 0;
  err_code_t err_code=0;

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
  switch (state_change) {
  case PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED:
    config->has_rnti = 1;
    config->rnti = rnti;
    break;
  case PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_UPDATED:
  case PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_ACTIVATED:
    flexran_agent_fill_rrc_ue_config(mod_id, rnti, config);
    /* we don't call into the MAC CM here; it will be called later through an
     * ue_config_request */
    break;
  case PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_MOVED:
  default:
    LOG_E(FLEXRAN_AGENT, "state change FLUESC_MOVED or unknown state occured for RNTI %x\n",
          rnti);
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
  if (err_code != 0)
     LOG_E(FLEXRAN_AGENT, "Could not send UE state message becasue of %d for RNTI %x\n",
           err_code, rnti);
}


int flexran_agent_destroy_ue_state_change(Protocol__FlexranMessage *msg) {
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_STATE_CHANGE_MSG)
    goto error;
  free(msg->ue_state_change_msg->header);
  if (msg->ue_state_change_msg->config->capabilities)
    free(msg->ue_state_change_msg->config->capabilities);
  free(msg->ue_state_change_msg->config);
  free(msg->ue_state_change_msg);
  free(msg);
  return 0;

 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

/* this is called by RRC as a part of rrc xface  . The controller previously requested  this*/ 
void flexran_trigger_rrc_measurements (mid_t mod_id, LTE_MeasResults_t*  measResults) {

  // int                   priority = 0; // Warning Preventing
  // void                  *data;
  // int                   size;
  // err_code_t             err_code = -100;
  triggered_rrc = true;

  /* TODO do we need this at the current state? meas_stats is never put into a
   * protobuf message?!
  int num = flexran_get_rrc_num_ues (mod_id);
  rnti_t rntis[num];
  flexran_get_rrc_rnti_list(mod_id, rntis, num);

  meas_stats = malloc(sizeof(rrc_meas_stats) * num); 

  for (int i = 0; i < num; i++){
    const rnti_t rnti = rntis[i];
    meas_stats[i].rnti = rnti;
    meas_stats[i].meas_id = flexran_get_rrc_pcell_measid(mod_id, rnti);
    meas_stats[i].rsrp =  flexran_get_rrc_pcell_rsrp(mod_id, rnti) - 140;
    // measResults->measResultPCell.rsrpResult - 140;
    meas_stats[i].rsrq =  flexran_get_rrc_pcell_rsrq(mod_id, rnti)/2 - 20;
  */
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


  // stats_reply_msg->cell_report = cell_report;
    
  // stats_reply_msg->ue_report = ue_report;
  
  // msg = malloc(sizeof(Protocol__FlexranMessage));
  // if(msg == NULL)
  //   goto error;
  // protocol__flexran_message__init(msg);
  // msg->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REPLY_MSG;
  // msg->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  // msg->stats_reply_msg = stats_reply_msg;
  
  // data = flexran_agent_pack_message(msg, &size);
  
  
  // if (flexran_agent_msg_send(mod_id, FLEXRAN_AGENT_DEFAULT, data, size, priority)) {
  
  //   err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
  //   goto error;
  // }
  
  //  LOG_I(FLEXRAN_AGENT,"RRC Trigger is done  \n");

  return;

  // error:

    // LOG_E(FLEXRAN_AGENT, "Could not send UE state message becasue of %d \n",err_code);
    /* Free the measurement report received from UE. */
    // ASN_STRUCT_FREE(asn_DEF_MeasResults, p->meas);
    /* Free the params. */
    // free(p);
    // return -1;
}


int flexran_agent_rrc_stats_reply(mid_t mod_id,       
          const report_config_t *report_config,
           Protocol__FlexUeStatsReport **ue_report,
           Protocol__FlexCellStatsReport **cell_report) {

  if (report_config->nr_ue > 0) {
    rnti_t rntis[report_config->nr_ue];
    flexran_get_rrc_rnti_list(mod_id, rntis, report_config->nr_ue);
    for (int i = 0; i < report_config->nr_ue; i++) {
      const rnti_t rnti = rntis[i];
      
      /* Check flag for creation of buffer status report */
      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_RRC_MEASUREMENTS) {
      	
        /*Source Cell*/
        Protocol__FlexRrcMeasurements *rrc_measurements;
      	rrc_measurements = malloc(sizeof(Protocol__FlexRrcMeasurements));
      	if (rrc_measurements == NULL)
      	  goto error;
      	protocol__flex_rrc_measurements__init(rrc_measurements);
      	
        rrc_measurements->measid = flexran_get_rrc_pcell_measid(mod_id, rnti);
      	rrc_measurements->has_measid = 1;
      	
        rrc_measurements->pcell_rsrp = flexran_get_rrc_pcell_rsrp(mod_id, rnti);
      	rrc_measurements->has_pcell_rsrp = 1;
      	
        rrc_measurements->pcell_rsrq = flexran_get_rrc_pcell_rsrq(mod_id, rnti);
      	rrc_measurements->has_pcell_rsrq = 1 ;
        
        /* Target Cell, Neghibouring*/
        Protocol__FlexNeighCellsMeasurements *neigh_meas;
        neigh_meas = malloc(sizeof(Protocol__FlexNeighCellsMeasurements));
        if (neigh_meas == NULL)
          goto error;
        protocol__flex_neigh_cells_measurements__init(neigh_meas);
         
        
        neigh_meas->n_eutra_meas = flexran_get_rrc_num_ncell(mod_id, rnti);

        Protocol__FlexEutraMeasurements **eutra_meas = NULL;

        if (neigh_meas->n_eutra_meas > 0){
          
          eutra_meas = malloc(sizeof(Protocol__FlexEutraMeasurements) * neigh_meas->n_eutra_meas);
          if (eutra_meas == NULL)
            goto error;
          
          for (int j = 0; j < neigh_meas->n_eutra_meas; j++ ){

              eutra_meas[j] = malloc(sizeof(Protocol__FlexEutraMeasurements));
              if (eutra_meas[j] == NULL)
                goto error;

              protocol__flex_eutra_measurements__init(eutra_meas[j]);

              eutra_meas[j]->phys_cell_id = flexran_get_rrc_neigh_phy_cell_id(mod_id, rnti, j);
              eutra_meas[j]->has_phys_cell_id = 1;


              /*TODO: Extend for CGI and PLMNID*/

              Protocol__FlexEutraRefSignalMeas *meas_result;
              meas_result = malloc(sizeof(Protocol__FlexEutraRefSignalMeas));

              protocol__flex_eutra_ref_signal_meas__init(meas_result);     

              meas_result->rsrp = flexran_get_rrc_neigh_rsrp(mod_id, rnti, eutra_meas[j]->phys_cell_id);
              meas_result->has_rsrp = 1;

              meas_result->rsrq = flexran_get_rrc_neigh_rsrq(mod_id, rnti, eutra_meas[j]->phys_cell_id);
              meas_result->has_rsrq = 1;

              eutra_meas[j]->meas_result = meas_result;
             
          }    

           neigh_meas->eutra_meas = eutra_meas;   

           rrc_measurements->neigh_meas = neigh_meas;
       
        }

      	 ue_report[i]->rrc_measurements = rrc_measurements;
         ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_RRC_MEASUREMENTS;
      	
      }

    } 

  }

  /* To be considered for RRC signaling of cell*/ 
  // if (report_config->nr_cc > 0) { 
    
            
  //           // Fill in the Cell reports
  //           for (i = 0; i < report_config->nr_cc; i++) {


  //                     /* Check flag for creation of noise and interference report */
  //                     if(report_config->cc_report_type[i].cc_report_flags & PROTOCOL__FLEX_CELL_STATS_TYPE__FLCST_NOISE_INTERFERENCE) {
  //                           // TODO: Fill in the actual noise and interference report for this cell
  //                           Protocol__FlexNoiseInterferenceReport *ni_report;
  //                           ni_report = malloc(sizeof(Protocol__FlexNoiseInterferenceReport));
  //                           if(ni_report == NULL)
  //                             goto error;
  //                           protocol__flex_noise_interference_report__init(ni_report);
  //                           // Current frame and subframe number
  //                           ni_report->sfn_sf = flexran_get_sfn_sf(enb_id);
  //                           ni_report->has_sfn_sf = 1;
  //                           //TODO:Received interference power in dbm
  //                           ni_report->rip = 0;
  //                           ni_report->has_rip = 1;
  //                           //TODO:Thermal noise power in dbm
  //                           ni_report->tnp = 0;
  //                           ni_report->has_tnp = 1;

  //                           ni_report->p0_nominal_pucch = flexran_get_p0_nominal_pucch(enb_id, 0);
  //                           ni_report->has_p0_nominal_pucch = 1;
  //                           cell_report[i]->noise_inter_report = ni_report;
  //                           cell_report[i]->flags |= PROTOCOL__FLEX_CELL_STATS_TYPE__FLCST_NOISE_INTERFERENCE;
  //                     }
  //           }
            

      
            
  // }

  return 0;

 error:

  for (int i = 0; i < report_config->nr_ue; i++){

      if (ue_report[i]->rrc_measurements && ue_report[i]->rrc_measurements->neigh_meas != NULL){
          for (int j = 0; j < ue_report[i]->rrc_measurements->neigh_meas->n_eutra_meas; j++){

             free(ue_report[i]->rrc_measurements->neigh_meas->eutra_meas[j]);
        }
        free(ue_report[i]->rrc_measurements->neigh_meas);
      }
  }

  if (cell_report != NULL)
        free(cell_report);
  if (ue_report != NULL)
        free(ue_report);

  return -1;
}

int flexran_agent_rrc_destroy_stats_reply(Protocol__FlexStatsReply *reply)
{
  for (int i = 0; i < reply->n_ue_report; i++){
    if (reply->ue_report[i]->rrc_measurements && reply->ue_report[i]->rrc_measurements->neigh_meas){
      for (int j = 0; j < reply->ue_report[i]->rrc_measurements->neigh_meas->n_eutra_meas; j++){
        free(reply->ue_report[i]->rrc_measurements->neigh_meas->eutra_meas[j]->meas_result);
        free(reply->ue_report[i]->rrc_measurements->neigh_meas->eutra_meas[j]);
      }
      free(reply->ue_report[i]->rrc_measurements->neigh_meas->eutra_meas);
      free(reply->ue_report[i]->rrc_measurements->neigh_meas);
      free(reply->ue_report[i]->rrc_measurements);
    }
  }
  return 0;
}

void flexran_agent_fill_rrc_ue_config(mid_t mod_id, rnti_t rnti,
    Protocol__FlexUeConfig *ue_conf)
{
  if (ue_conf->has_rnti && ue_conf->rnti != rnti) {
    LOG_E(FLEXRAN_AGENT, "ue_config existing RNTI %x does not match RRC RNTI %x\n",
          ue_conf->rnti, rnti);
    return;
  }
  ue_conf->has_rnti = 1;
  ue_conf->rnti = rnti;
  ue_conf->imsi = flexran_get_ue_imsi(mod_id, rnti);
  ue_conf->has_imsi = 1;

  //TODO: Set the DRX configuration (optional)
  //Not supported for now, so we do not set it

  ue_conf->time_alignment_timer = flexran_get_time_alignment_timer(mod_id, rnti);
  ue_conf->has_time_alignment_timer = 1;

  ue_conf->meas_gap_config_pattern = flexran_get_meas_gap_config(mod_id, rnti);
  ue_conf->has_meas_gap_config_pattern = 1;

  if(ue_conf->meas_gap_config_pattern != PROTOCOL__FLEX_MEAS_GAP_CONFIG_PATTERN__FLMGCP_OFF) {
    ue_conf->meas_gap_config_sf_offset = flexran_get_meas_gap_config_offset(mod_id, rnti);
    ue_conf->has_meas_gap_config_sf_offset = 1;
  }

  //TODO: Set the SPS configuration (Optional)
  //Not supported for now, so we do not set it

  //TODO: Set the SR configuration (Optional)
  //We do not set it for now

  //TODO: Set the CQI configuration (Optional)
  //We do not set it for now

  ue_conf->transmission_mode = flexran_get_ue_transmission_mode(mod_id, rnti);
  ue_conf->has_transmission_mode = 1;

  Protocol__FlexUeCapabilities *c_capabilities;
  c_capabilities = malloc(sizeof(Protocol__FlexUeCapabilities));
  if (c_capabilities) {
    protocol__flex_ue_capabilities__init(c_capabilities);

    c_capabilities->has_half_duplex = 1;
    c_capabilities->half_duplex = flexran_get_half_duplex(mod_id, rnti);

    c_capabilities->has_intra_sf_hopping = 1;
    c_capabilities->intra_sf_hopping = flexran_get_intra_sf_hopping(mod_id, rnti);

    c_capabilities->has_type2_sb_1 = 1;
    c_capabilities->type2_sb_1 = flexran_get_type2_sb_1(mod_id, rnti);

    c_capabilities->has_ue_category = 1;
    c_capabilities->ue_category = flexran_get_ue_category(mod_id, rnti);

    c_capabilities->has_res_alloc_type1 = 1;
    c_capabilities->res_alloc_type1 = flexran_get_res_alloc_type1(mod_id, rnti);

    ue_conf->capabilities = c_capabilities;
  }

  ue_conf->has_ue_transmission_antenna = 1;
  ue_conf->ue_transmission_antenna = flexran_get_ue_transmission_antenna(mod_id, rnti);

  ue_conf->has_tti_bundling = 1;
  ue_conf->tti_bundling = flexran_get_tti_bundling(mod_id, rnti);

  ue_conf->has_max_harq_tx = 1;
  ue_conf->max_harq_tx = flexran_get_maxHARQ_TX(mod_id, rnti);

  ue_conf->has_beta_offset_ack_index = 1;
  ue_conf->beta_offset_ack_index = flexran_get_beta_offset_ack_index(mod_id, rnti);

  ue_conf->has_beta_offset_ri_index = 1;
  ue_conf->beta_offset_ri_index = flexran_get_beta_offset_ri_index(mod_id, rnti);

  ue_conf->has_beta_offset_cqi_index = 1;
  ue_conf->beta_offset_cqi_index = flexran_get_beta_offset_cqi_index(mod_id, rnti);

  /* assume primary carrier */
  ue_conf->has_ack_nack_simultaneous_trans = 1;
  ue_conf->ack_nack_simultaneous_trans = flexran_get_ack_nack_simultaneous_trans(mod_id,0);

  ue_conf->has_simultaneous_ack_nack_cqi = 1;
  ue_conf->simultaneous_ack_nack_cqi = flexran_get_simultaneous_ack_nack_cqi(mod_id, rnti);

  ue_conf->has_aperiodic_cqi_rep_mode = 1;
  ue_conf->aperiodic_cqi_rep_mode = flexran_get_aperiodic_cqi_rep_mode(mod_id, rnti);

  ue_conf->has_tdd_ack_nack_feedback = 1;
  ue_conf->tdd_ack_nack_feedback = flexran_get_tdd_ack_nack_feedback_mode(mod_id, rnti);

  ue_conf->has_ack_nack_repetition_factor = 1;
  ue_conf->ack_nack_repetition_factor = flexran_get_ack_nack_repetition_factor(mod_id, rnti);

  ue_conf->has_extended_bsr_size = 1;
  ue_conf->extended_bsr_size = flexran_get_extended_bsr_size(mod_id, rnti);
}

int flexran_agent_register_rrc_xface(mid_t mod_id)
{
  if (agent_rrc_xface[mod_id]) {
    LOG_E(FLEXRAN_AGENT, "RRC agent for eNB %d is already registered\n", mod_id);
    return -1;
  }
  AGENT_RRC_xface *xface = malloc(sizeof(AGENT_RRC_xface));
  if (!xface) {
    LOG_E(FLEXRAN_AGENT, "could not allocate memory for RRC agent xface %d\n", mod_id);
    return -1;
  }

//  xface->flexran_agent_send_update_rrc_stats = flexran_agent_send_update_rrc_stats;
  
  xface->flexran_agent_notify_ue_state_change = flexran_agent_ue_state_change;
  xface->flexran_trigger_rrc_measurements = flexran_trigger_rrc_measurements;

  agent_rrc_xface[mod_id] = xface;

  return 0;
}

void flexran_agent_fill_rrc_cell_config(mid_t mod_id, uint8_t cc_id,
    Protocol__FlexCellConfig *conf) {

  if (!conf->si_config) {
    conf->si_config = malloc(sizeof(Protocol__FlexSiConfig));
    if (conf->si_config)
      protocol__flex_si_config__init(conf->si_config);
  }

  if (conf->si_config) {
    // TODO THIS IS DU RRC
    conf->si_config->sib1_length = flexran_get_sib1_length(mod_id, cc_id);
    conf->si_config->has_sib1_length = 1;

    conf->si_config->si_window_length = (uint32_t) flexran_get_si_window_length(mod_id,  cc_id);
    conf->si_config->has_si_window_length = 1;

    conf->si_config->n_si_message = 0;

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
  }

  conf->ra_response_window_size = flexran_get_ra_ResponseWindowSize(mod_id, cc_id);
  conf->has_ra_response_window_size = 1;

  // belongs to MAC but is read in RRC
  conf->mac_contention_resolution_timer = flexran_get_mac_ContentionResolutionTimer(mod_id, cc_id);
  conf->has_mac_contention_resolution_timer = 1;

  conf->ul_pusch_power = flexran_agent_get_operating_pusch_p0 (mod_id, cc_id);
  conf->has_ul_pusch_power = 1;

  conf->n_plmn_id = flexran_get_rrc_num_plmn_ids(mod_id);
  conf->plmn_id = calloc(conf->n_plmn_id, sizeof(Protocol__FlexPlmn *));
  if (conf->plmn_id) {
    for (int i = 0; i < conf->n_plmn_id; i++) {
      conf->plmn_id[i] = malloc(sizeof(Protocol__FlexPlmn));
      if (!conf->plmn_id[i]) continue;
      protocol__flex_plmn__init(conf->plmn_id[i]);
      conf->plmn_id[i]->mcc = flexran_get_rrc_mcc(mod_id, i);
      conf->plmn_id[i]->has_mcc = 1;
      conf->plmn_id[i]->mnc = flexran_get_rrc_mnc(mod_id, i);
      conf->plmn_id[i]->has_mnc = 1;
      conf->plmn_id[i]->mnc_length = flexran_get_rrc_mnc_digit_length(mod_id, i);
      conf->plmn_id[i]->has_mnc_length = 1;
    }
  } else {
    conf->n_plmn_id = 0;
  }
}

int flexran_agent_unregister_rrc_xface(mid_t mod_id)
{
  if (!agent_rrc_xface[mod_id]) {
    LOG_E(FLEXRAN_AGENT, "RRC agent for eNB %d is not registered\n", mod_id);
    return -1;
  }
  //xface->agent_ctxt = NULL;
//  xface->flexran_agent_send_update_rrc_stats = NULL;

  agent_rrc_xface[mod_id]->flexran_agent_notify_ue_state_change = NULL;
  agent_rrc_xface[mod_id]->flexran_trigger_rrc_measurements = NULL;
  free(agent_rrc_xface[mod_id]);
  agent_rrc_xface[mod_id] = NULL;

  return 0;
}

AGENT_RRC_xface *flexran_agent_get_rrc_xface(mid_t mod_id)
{
  return agent_rrc_xface[mod_id];
}
