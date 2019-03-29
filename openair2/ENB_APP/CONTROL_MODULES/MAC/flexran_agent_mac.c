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
 * \brief FlexRAN agent message handler for MAC layer
 * \author Xenofon Foukas, Mohamed Kassem and Navid Nikaein
 * \date 2016
 * \version 0.1
 */

#include "flexran_agent_mac.h"
#include "flexran_agent_extern.h"
#include "flexran_agent_common.h"
#include "flexran_agent_mac_internal.h"
#include "flexran_agent_net_comm.h"
#include "flexran_agent_timer.h"
#include "flexran_agent_ran_api.h"

#include "LAYER2/MAC/mac_proto.h"

#include "liblfds700.h"

#include "common/utils/LOG/log.h"

/*Array containing the Agent-MAC interfaces*/
AGENT_MAC_xface *agent_mac_xface[NUM_MAX_ENB];

/* Ringbuffer related structs used for maintaining the dl mac config messages */
//message_queue_t *dl_mac_config_queue;
struct lfds700_misc_prng_state ps[NUM_MAX_ENB];
struct lfds700_ringbuffer_element *dl_mac_config_array[NUM_MAX_ENB];
struct lfds700_ringbuffer_state ringbuffer_state[NUM_MAX_ENB];

/* the slice config as kept in the underlying system */
Protocol__FlexSliceConfig *slice_config[MAX_NUM_SLICES];
/* a structure that keeps updates which will be reflected in slice_config later */
Protocol__FlexSliceConfig *sc_update[MAX_NUM_SLICES];
/* indicates whether sc_update contains new data */
int perform_slice_config_update_count = 1;
/* queue of incoming new UE<>slice association commands */
Protocol__FlexUeConfig *ue_slice_assoc_update[MAX_NUM_SLICES];
int n_ue_slice_assoc_updates = 0;
/* mutex for sc_update: do not receive new config and write it at the same time */
pthread_mutex_t sc_update_mtx = PTHREAD_MUTEX_INITIALIZER;


int flexran_agent_mac_stats_reply(mid_t mod_id,
          const report_config_t *report_config,
           Protocol__FlexUeStatsReport **ue_report,
           Protocol__FlexCellStatsReport **cell_report) {


  // Protocol__FlexHeader *header;
  int i, j, k;
  int UE_id;
  int cc_id = 0;
  int enb_id = mod_id;

  /* Allocate memory for list of UE reports */
  if (report_config->nr_ue > 0) {


          for (i = 0; i < report_config->nr_ue; i++) {

                UE_id = flexran_get_mac_ue_id(mod_id, i);

                ue_report[i]->rnti = report_config->ue_report_type[i].ue_rnti;
                ue_report[i]->has_rnti = 1;

                /* Check flag for creation of buffer status report */
                if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_BSR) {
                      //TODO should be automated
                        ue_report[i]->n_bsr = 4;
                        uint32_t *elem;
                        elem = (uint32_t *) malloc(sizeof(uint32_t)*ue_report[i]->n_bsr);
                        if (elem == NULL)
                               goto error;
                        for (j = 0; j < ue_report[i]->n_bsr; j++) {
                                // NN: we need to know the cc_id here, consider the first one
                                elem[j] = flexran_get_ue_bsr_ul_buffer_info (enb_id, i, j);
                        }

                        ue_report[i]->bsr = elem;
                        ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_BSR;
                }

                /* Check flag for creation of PHR report */
                if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PHR) {
                        ue_report[i]->phr = flexran_get_ue_phr (enb_id, UE_id); // eNB_UE_list->UE_template[UE_PCCID(enb_id,UE_id)][UE_id].phr_info;
                        ue_report[i]->has_phr = 1;
                        ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PHR;

                }

                /* Check flag for creation of RLC buffer status report */
                if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_RLC_BS) {
                        ue_report[i]->n_rlc_report = 3; // Set this to the number of LCs for this UE. This needs to be generalized for for LCs
                        Protocol__FlexRlcBsr ** rlc_reports;
                        rlc_reports = malloc(sizeof(Protocol__FlexRlcBsr *) * ue_report[i]->n_rlc_report);
                        if (rlc_reports == NULL)
                              goto error;

                        // NN: see LAYER2/openair2_proc.c for rlc status
                        for (j = 0; j < ue_report[i]->n_rlc_report; j++) {

                              rlc_reports[j] = malloc(sizeof(Protocol__FlexRlcBsr));
                              if (rlc_reports[j] == NULL)
                                 goto error;
                              protocol__flex_rlc_bsr__init(rlc_reports[j]);
                              rlc_reports[j]->lc_id = j+1;
                              rlc_reports[j]->has_lc_id = 1;
                              rlc_reports[j]->tx_queue_size = flexran_get_tx_queue_size(enb_id, UE_id, j + 1);
                              rlc_reports[j]->has_tx_queue_size = 1;

                              //TODO:Set tx queue head of line delay in ms
                              rlc_reports[j]->tx_queue_hol_delay = flexran_get_hol_delay(enb_id, UE_id, j + 1);
                              rlc_reports[j]->has_tx_queue_hol_delay = 1;
                              //TODO:Set retransmission queue size in bytes
                              rlc_reports[j]->retransmission_queue_size = 10;
                              rlc_reports[j]->has_retransmission_queue_size = 0;
                              //TODO:Set retransmission queue head of line delay in ms
                              rlc_reports[j]->retransmission_queue_hol_delay = 100;
                              rlc_reports[j]->has_retransmission_queue_hol_delay = 0;
                              //TODO DONE:Set current size of the pending message in bytes
                              rlc_reports[j]->status_pdu_size = flexran_get_num_pdus_buffer(enb_id, UE_id, j + 1);
                              rlc_reports[j]->has_status_pdu_size = 1;

                        }
                        // Add RLC buffer status reports to the full report
                        if (ue_report[i]->n_rlc_report > 0)
                            ue_report[i]->rlc_report = rlc_reports;
                        ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_RLC_BS;

                }

                /* Check flag for creation of MAC CE buffer status report */
                if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_MAC_CE_BS) {
                        // TODO: Fill in the actual MAC CE buffer status report
                        ue_report[i]->pending_mac_ces = (flexran_get_MAC_CE_bitmap_TA(enb_id, UE_id, 0) | (0 << 1) | (0 << 2) | (0 << 3)) & 15;
                                      // Use as bitmap. Set one or more of the; /* Use as bitmap. Set one or more of the
                                       // PROTOCOL__FLEX_CE_TYPE__FLPCET_ values
                                       // found in stats_common.pb-c.h. See
                                       // flex_ce_type in FlexRAN specification
                        ue_report[i]->has_pending_mac_ces = 1;
                        ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_MAC_CE_BS;
                }

                /* Check flag for creation of DL CQI report */
                if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_DL_CQI) {
                        // TODO: Fill in the actual DL CQI report for the UE based on its configuration
                        Protocol__FlexDlCqiReport * dl_report;
                        dl_report = malloc(sizeof(Protocol__FlexDlCqiReport));
                        if (dl_report == NULL)
                          goto error;
                        protocol__flex_dl_cqi_report__init(dl_report);

                        dl_report->sfn_sn = flexran_get_sfn_sf(enb_id);
                        dl_report->has_sfn_sn = 1;
                        //Set the number of DL CQI reports for this UE. One for each CC
                        dl_report->n_csi_report = flexran_get_active_CC(enb_id, UE_id);
                        dl_report->n_csi_report = 1 ;
                        //Create the actual CSI reports.
                        Protocol__FlexDlCsi **csi_reports;
                        csi_reports = malloc(sizeof(Protocol__FlexDlCsi *)*dl_report->n_csi_report);
                        if (csi_reports == NULL)
                          goto error;
                        for (j = 0; j < dl_report->n_csi_report; j++) {

                              csi_reports[j] = malloc(sizeof(Protocol__FlexDlCsi));
                              if (csi_reports[j] == NULL)
                                goto error;
                              protocol__flex_dl_csi__init(csi_reports[j]);
                              //The servCellIndex for this report
                              csi_reports[j]->serv_cell_index = j;
                              csi_reports[j]->has_serv_cell_index = 1;
                              //The rank indicator value for this cc
                              csi_reports[j]->ri = flexran_get_current_RI(enb_id, UE_id, j);
                              csi_reports[j]->has_ri = 1;
                              //TODO: the type of CSI report based on the configuration of the UE
                              //For now we only support type P10, which only needs a wideband value
                              //The full set of types can be found in stats_common.pb-c.h and
                              //in the FlexRAN specifications
                              csi_reports[j]->type =  PROTOCOL__FLEX_CSI_TYPE__FLCSIT_P10;
                              csi_reports[j]->has_type = 1;
                              csi_reports[j]->report_case = PROTOCOL__FLEX_DL_CSI__REPORT_P10CSI;

                              if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P10CSI){

                                    Protocol__FlexCsiP10 *csi10;
                                    csi10 = malloc(sizeof(Protocol__FlexCsiP10));
                                    if (csi10 == NULL)
                                    goto error;
                                    protocol__flex_csi_p10__init(csi10);
                                    //TODO: set the wideband value
                                    // NN: this is also depends on cc_id
                                    csi10->wb_cqi = flexran_get_ue_wcqi (enb_id, UE_id); //eNB_UE_list->eNB_UE_stats[UE_PCCID(enb_id,UE_id)][UE_id].dl_cqi;
                                    csi10->has_wb_cqi = 1;
                                    //Add the type of measurements to the csi report in the proper union type
                                    csi_reports[j]->p10csi = csi10;
                              }

                              else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P11CSI){


                                    Protocol__FlexCsiP11 *csi11;
                                    csi11 = malloc(sizeof(Protocol__FlexCsiP11));
                                    if (csi11 == NULL)
                                    goto error;
                                    protocol__flex_csi_p11__init(csi11);

                                    csi11->wb_cqi = malloc(sizeof(csi11->wb_cqi));
				    csi11->n_wb_cqi = 1;
				    csi11->wb_cqi[0] = flexran_get_ue_wcqi (enb_id, UE_id);
                                    // According To spec 36.213

                                    if (flexran_get_antenna_ports(enb_id, j) == 2 && csi_reports[j]->ri == 1) {
                                        // TODO PMI
                                        csi11->wb_pmi = flexran_get_ue_wpmi(enb_id, UE_id, 0);
                                        csi11->has_wb_pmi = 1;

                                       }

                                      else if (flexran_get_antenna_ports(enb_id, j) == 2 && csi_reports[j]->ri == 2){
                                        // TODO PMI
                                        csi11->wb_pmi = flexran_get_ue_wpmi(enb_id, UE_id, 0);
                                        csi11->has_wb_pmi = 1;

                                      }

                                      else if (flexran_get_antenna_ports(enb_id, j) == 4 && csi_reports[j]->ri == 2){
                                        // TODO PMI
                                        csi11->wb_pmi = flexran_get_ue_wpmi(enb_id, UE_id, 0);
                                        csi11->has_wb_pmi = 1;


                                      }

                                      csi11->has_wb_pmi = 0;

                                      csi_reports[j]->p11csi = csi11;

                               }





                              else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P20CSI){

                                    Protocol__FlexCsiP20 *csi20;
                                    csi20 = malloc(sizeof(Protocol__FlexCsiP20));
                                    if (csi20 == NULL)
                                    goto error;
                                    protocol__flex_csi_p20__init(csi20);

                                    csi20->wb_cqi = flexran_get_ue_wcqi (enb_id, UE_id);
                                    csi20->has_wb_cqi = 1;


                                    csi20->bandwidth_part_index = 1 ;//TODO
                                    csi20->has_bandwidth_part_index = 1;

                                    csi20->sb_index = 1 ;//TODO
                                    csi20->has_sb_index = 1 ;


                                    csi_reports[j]->p20csi = csi20;


                              }

                              else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P21CSI){

                                  // Protocol__FlexCsiP21 *csi21;
                                  // csi21 = malloc(sizeof(Protocol__FlexCsiP21));
                                  // if (csi21 == NULL)
                                  // goto error;
                                  // protocol__flex_csi_p21__init(csi21);

                                  // csi21->wb_cqi = flexran_get_ue_wcqi (enb_id, UE_id);


                                  // csi21->wb_pmi = flexran_get_ue_pmi(enb_id); //TDO inside
                                  // csi21->has_wb_pmi = 1;

                                  // csi21->sb_cqi = 1; // TODO

                                  // csi21->bandwidth_part_index = 1 ; //TDO inside
                                  // csi21->has_bandwidth_part_index = 1 ;

                                  // csi21->sb_index = 1 ;//TODO
                                  // csi21->has_sb_index = 1 ;


                                  // csi_reports[j]->p20csi = csi21;

                              }

                              else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A12CSI){


                                  // Protocol__FlexCsiA12 *csi12;
                                  // csi12 = malloc(sizeof(Protocol__FlexCsiA12));
                                  // if (csi12 == NULL)
                                  // goto error;
                                  // protocol__flex_csi_a12__init(csi12);

                                  // csi12->wb_cqi = flexran_get_ue_wcqi (enb_id, UE_id);

                                  // csi12->sb_pmi = 1 ; //TODO inside

                                  // TODO continou
                              }

                              else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A22CSI){

                                    // Protocol__FlexCsiA22 *csi22;
                                    // csi22 = malloc(sizeof(Protocol__FlexCsiA22));
                                    // if (csi22 == NULL)
                                    // goto error;
                                    // protocol__flex_csi_a22__init(csi22);

                                    // csi22->wb_cqi = flexran_get_ue_wcqi (enb_id, UE_id);

                                    // csi22->sb_cqi = 1 ; //TODO inside

                                    // csi22->wb_pmi = flexran_get_ue_wcqi (enb_id, UE_id);
                                    // csi22->has_wb_pmi = 1;

                                    // csi22->sb_pmi = 1 ; //TODO inside
                                    // csi22->has_wb_pmi = 1;

                                    // csi22->sb_list = flexran_get_ue_wcqi (enb_id, UE_id);


                                }

                                else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A20CSI){

                                    // Protocol__FlexCsiA20 *csi20;
                                    // csi20 = malloc(sizeof(Protocol__FlexCsiA20));
                                    // if (csi20 == NULL)
                                    // goto error;
                                    // protocol__flex_csi_a20__init(csi20);

                                    // csi20->wb_cqi = flexran_get_ue_wcqi (enb_id, UE_id);
                                    // csi20->has_wb_cqi = 1;

                                    // csi20>sb_cqi = 1 ; //TODO inside
                                    // csi20>has_sb_cqi = 1 ;

                                    // csi20->sb_list = 1; // TODO inside


                                }

                                else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A30CSI){

                                }

                                else if(csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A31CSI){

                                }

                        }
                     //Add the csi reports to the full DL CQI report
                    dl_report->csi_report = csi_reports;
                    //Add the DL CQI report to the stats report
                     ue_report[i]->dl_cqi_report = dl_report;
                    ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_DL_CQI;
                }

                /* Check flag for creation of paging buffer status report */
                if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PBS) {
                            //TODO: Fill in the actual paging buffer status report. For this field to be valid, the RNTI
                            //set in the report must be a P-RNTI
                            Protocol__FlexPagingBufferReport *paging_report;
                            paging_report = malloc(sizeof(Protocol__FlexPagingBufferReport));
                            if (paging_report == NULL)
                              goto error;
                            protocol__flex_paging_buffer_report__init(paging_report);
                            //Set the number of pending paging messages
                            paging_report->n_paging_info = 1;
                            //Provide a report for each pending paging message
                            Protocol__FlexPagingInfo **p_info;
                            p_info = malloc(sizeof(Protocol__FlexPagingInfo *) * paging_report->n_paging_info);
                            if (p_info == NULL)
                              goto error;

                            for (j = 0; j < paging_report->n_paging_info; j++) {

                                    p_info[j] = malloc(sizeof(Protocol__FlexPagingInfo));
                                    if(p_info[j] == NULL)
                                      goto error;
                                    protocol__flex_paging_info__init(p_info[j]);
                                    //TODO: Set paging index. This index is the same that will be used for the scheduling of the
                                    //paging message by the controller
                                    p_info[j]->paging_index = 10;
                                    p_info[j]->has_paging_index = 1;
                                    //TODO:Set the paging message size
                                    p_info[j]->paging_message_size = 100;
                                    p_info[j]->has_paging_message_size = 1;
                                    //TODO: Set the paging subframe
                                    p_info[j]->paging_subframe = 10;
                                    p_info[j]->has_paging_subframe = 1;
                                    //TODO: Set the carrier index for the pending paging message
                                    p_info[j]->carrier_index = 0;
                                    p_info[j]->has_carrier_index = 1;

                            }
                            //Add all paging info to the paging buffer rerport
                            paging_report->paging_info = p_info;
                            //Add the paging report to the UE report
                            ue_report[i]->pbr = paging_report;
                            ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PBS;
                }

                  /* Check flag for creation of UL CQI report */
                if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_UL_CQI) {

                      //Fill in the full UL CQI report of the UE
                      Protocol__FlexUlCqiReport *full_ul_report;
                      full_ul_report = malloc(sizeof(Protocol__FlexUlCqiReport));
                      if(full_ul_report == NULL)
                        goto error;
                      protocol__flex_ul_cqi_report__init(full_ul_report);
                      //TODO:Set the SFN and SF of the generated report
                      full_ul_report->sfn_sn = flexran_get_sfn_sf(enb_id);
                      full_ul_report->has_sfn_sn = 1;
                      //TODO:Set the number of UL measurement reports based on the types of measurements
                      //configured for this UE and on the servCellIndex
                      full_ul_report->n_cqi_meas = 1;
                      Protocol__FlexUlCqi **ul_report;
                      ul_report = malloc(sizeof(Protocol__FlexUlCqi *) * full_ul_report->n_cqi_meas);
                      if(ul_report == NULL)
                        goto error;
                      //Fill each UL report of the UE for each of the configured report types
                      for(j = 0; j < full_ul_report->n_cqi_meas; j++) {

                              ul_report[j] = malloc(sizeof(Protocol__FlexUlCqi));
                              if(ul_report[j] == NULL)
                              goto error;
                              protocol__flex_ul_cqi__init(ul_report[j]);
                              //TODO: Set the type of the UL report. As an example set it to SRS UL report
                              // See enum flex_ul_cqi_type in FlexRAN specification for more details
                              ul_report[j]->type = PROTOCOL__FLEX_UL_CQI_TYPE__FLUCT_SRS;
                              ul_report[j]->has_type = 1;
                              //TODO:Set the number of SINR measurements based on the report type
                              //See struct flex_ul_cqi in FlexRAN specification for more details
                              ul_report[j]->n_sinr = 0;
                              uint32_t *sinr_meas;
                              sinr_meas = (uint32_t *) malloc(sizeof(uint32_t) * ul_report[j]->n_sinr);
                              if (sinr_meas == NULL)
                                goto error;
                              //TODO:Set the SINR measurements for the specified type
                              for (k = 0; k < ul_report[j]->n_sinr; k++) {
                                      sinr_meas[k] = 10;
                              }
                              ul_report[j]->sinr = sinr_meas;
                              //TODO: Set the servCellIndex for this report
                              ul_report[j]->serv_cell_index = 0;
                              ul_report[j]->has_serv_cell_index = 1;

                              //Set the list of UL reports of this UE to the full UL report
                              full_ul_report->cqi_meas = ul_report;

                              full_ul_report->n_pucch_dbm = MAX_NUM_CCs;
                              full_ul_report->pucch_dbm = malloc(sizeof(Protocol__FlexPucchDbm *) * full_ul_report->n_pucch_dbm);

                              for (j = 0; j < MAX_NUM_CCs; j++) {

                                      full_ul_report->pucch_dbm[j] = malloc(sizeof(Protocol__FlexPucchDbm));
                                      protocol__flex_pucch_dbm__init(full_ul_report->pucch_dbm[j]);
                                      full_ul_report->pucch_dbm[j]->has_serv_cell_index = 1;
                                      full_ul_report->pucch_dbm[j]->serv_cell_index = j;

                                      if(flexran_get_p0_pucch_dbm(enb_id, UE_id, j) != -1){
                                        full_ul_report->pucch_dbm[j]->p0_pucch_dbm = flexran_get_p0_pucch_dbm(enb_id, UE_id, j);
                                        full_ul_report->pucch_dbm[j]->has_p0_pucch_dbm = 1;
                                      }
                              }


                          }
                        //  Add full UL CQI report to the UE report
                        ue_report[i]->ul_cqi_report = full_ul_report;
                        ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_UL_CQI;

                     }
                      if (report_config->ue_report_type[i].ue_report_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_MAC_STATS) {

                            Protocol__FlexMacStats *macstats;
                            macstats = malloc(sizeof(Protocol__FlexMacStats));
                            if (macstats == NULL)
                              goto error;
                            protocol__flex_mac_stats__init(macstats);


                            macstats->total_bytes_sdus_dl = flexran_get_total_size_dl_mac_sdus(mod_id, UE_id, cc_id);
                            macstats->has_total_bytes_sdus_dl = 1;

                            macstats->total_bytes_sdus_ul = flexran_get_total_size_ul_mac_sdus(mod_id, UE_id, cc_id);
                            macstats->has_total_bytes_sdus_ul = 1;

                            macstats->tbs_dl = flexran_get_TBS_dl(mod_id, UE_id, cc_id);
                            macstats->has_tbs_dl = 1;

                            macstats->tbs_ul = flexran_get_TBS_ul(mod_id, UE_id, cc_id);
                            macstats->has_tbs_ul = 1;

                            macstats->prb_retx_dl = flexran_get_num_prb_retx_dl_per_ue(mod_id, UE_id, cc_id);
                            macstats->has_prb_retx_dl = 1;

                            macstats->prb_retx_ul = flexran_get_num_prb_retx_ul_per_ue(mod_id, UE_id, cc_id);
                            macstats->has_prb_retx_ul = 1;

                            macstats->prb_dl = flexran_get_num_prb_dl_tx_per_ue(mod_id, UE_id, cc_id);
                            macstats->has_prb_dl = 1;

                            macstats->prb_ul = flexran_get_num_prb_ul_rx_per_ue(mod_id, UE_id, cc_id);
                            macstats->has_prb_ul = 1;

                            macstats->mcs1_dl = flexran_get_mcs1_dl(mod_id, UE_id, cc_id);
                            macstats->has_mcs1_dl = 1;

                            macstats->mcs2_dl = flexran_get_mcs2_dl(mod_id, UE_id, cc_id);
                            macstats->has_mcs2_dl = 1;

                            macstats->mcs1_ul = flexran_get_mcs1_ul(mod_id, UE_id, cc_id);
                            macstats->has_mcs1_ul = 1;

                            macstats->mcs2_ul = flexran_get_mcs2_ul(mod_id, UE_id, cc_id);
                            macstats->has_mcs2_ul = 1;

                            macstats->total_prb_dl = flexran_get_total_prb_dl_tx_per_ue(mod_id, UE_id, cc_id);
                            macstats->has_total_prb_dl = 1;

                            macstats->total_prb_ul = flexran_get_total_prb_ul_rx_per_ue(mod_id, UE_id, cc_id);
                            macstats->has_total_prb_ul = 1;

                            macstats->total_pdu_dl = flexran_get_total_num_pdu_dl(mod_id, UE_id, cc_id);
                            macstats->has_total_pdu_dl = 1;

                            macstats->total_pdu_ul = flexran_get_total_num_pdu_ul(mod_id, UE_id, cc_id);
                            macstats->has_total_pdu_ul = 1;

                            macstats->total_tbs_dl = flexran_get_total_TBS_dl(mod_id, UE_id, cc_id);
                            macstats->has_total_tbs_dl = 1;

                            macstats->total_tbs_ul = flexran_get_total_TBS_ul(mod_id, UE_id, cc_id);
                            macstats->has_total_tbs_ul = 1;

                            macstats->harq_round = flexran_get_harq_round(mod_id, cc_id, UE_id);
                            macstats->has_harq_round = 1;

                            Protocol__FlexMacSdusDl ** mac_sdus;
                            mac_sdus = malloc(sizeof(Protocol__FlexMacSdusDl) * flexran_get_num_mac_sdu_tx(mod_id, UE_id, cc_id));
                            if (mac_sdus == NULL)
                                goto error;

                            macstats->n_mac_sdus_dl = flexran_get_num_mac_sdu_tx(mod_id, UE_id, cc_id);

                            for (j = 0; j < macstats->n_mac_sdus_dl; j++){


                                mac_sdus[j] = malloc(sizeof(Protocol__FlexMacSdusDl));
                                protocol__flex_mac_sdus_dl__init(mac_sdus[j]);

                                mac_sdus[j]->lcid = flexran_get_mac_sdu_lcid_index(mod_id, UE_id, cc_id, j);
                                mac_sdus[j]->has_lcid = 1;

                                mac_sdus[j]->sdu_length = flexran_get_mac_sdu_size(mod_id, UE_id, cc_id, mac_sdus[j]->lcid);
                                mac_sdus[j]->has_sdu_length = 1;


                            }


                            macstats->mac_sdus_dl = mac_sdus;


                        ue_report[i]->mac_stats = macstats;
                        ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_MAC_STATS;
               }




             }




     }

  /* Allocate memory for list of cell reports */
  if (report_config->nr_cc > 0) {


            // Fill in the Cell reports
            for (i = 0; i < report_config->nr_cc; i++) {


                      /* Check flag for creation of noise and interference report */
                      if(report_config->cc_report_type[i].cc_report_flags & PROTOCOL__FLEX_CELL_STATS_TYPE__FLCST_NOISE_INTERFERENCE) {
                            // TODO: Fill in the actual noise and interference report for this cell
                            Protocol__FlexNoiseInterferenceReport *ni_report;
                            ni_report = malloc(sizeof(Protocol__FlexNoiseInterferenceReport));
                            if(ni_report == NULL)
                              goto error;
                            protocol__flex_noise_interference_report__init(ni_report);
                            // Current frame and subframe number
                            ni_report->sfn_sf = flexran_get_sfn_sf(enb_id);
                            ni_report->has_sfn_sf = 1;
                            //TODO:Received interference power in dbm
                            ni_report->rip = 0;
                            ni_report->has_rip = 1;
                            //TODO:Thermal noise power in dbm
                            ni_report->tnp = 0;
                            ni_report->has_tnp = 1;

                            ni_report->p0_nominal_pucch = flexran_get_p0_nominal_pucch(enb_id, 0);
                            ni_report->has_p0_nominal_pucch = 1;
                            cell_report[i]->noise_inter_report = ni_report;
                            cell_report[i]->flags |= PROTOCOL__FLEX_CELL_STATS_TYPE__FLCST_NOISE_INTERFERENCE;
                      }
            }




  }

  return 0;

 error:

  if (cell_report != NULL)
        free(cell_report);
  if (ue_report != NULL)
        free(ue_report);

  return -1;
}

int flexran_agent_mac_destroy_stats_reply(Protocol__FlexStatsReply *reply) {
  int i, j, k;

  Protocol__FlexDlCqiReport *dl_report;
  Protocol__FlexUlCqiReport *ul_report;
  Protocol__FlexPagingBufferReport *paging_report;

  // Free the memory for the UE reports
  for (i = 0; i < reply->n_ue_report; i++) {
    free(reply->ue_report[i]->bsr);
    for (j = 0; j < reply->ue_report[i]->n_rlc_report; j++) {
      free(reply->ue_report[i]->rlc_report[j]);
    }
    free(reply->ue_report[i]->rlc_report);
    // If DL CQI report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_DL_CQI) {
      dl_report = reply->ue_report[i]->dl_cqi_report;
      // Delete all CSI reports
      for (j = 0; j < dl_report->n_csi_report; j++) {
	//Must free memory based on the type of report
	switch(dl_report->csi_report[j]->report_case) {
	case PROTOCOL__FLEX_DL_CSI__REPORT_P10CSI:
	  free(dl_report->csi_report[j]->p10csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_P11CSI:
	  free(dl_report->csi_report[j]->p11csi->wb_cqi);
	  free(dl_report->csi_report[j]->p11csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_P20CSI:
	  free(dl_report->csi_report[j]->p20csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_P21CSI:
	  free(dl_report->csi_report[j]->p21csi->wb_cqi);
	  free(dl_report->csi_report[j]->p21csi->sb_cqi);
	  free(dl_report->csi_report[j]->p21csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_A12CSI:
	  free(dl_report->csi_report[j]->a12csi->wb_cqi);
	  free(dl_report->csi_report[j]->a12csi->sb_pmi);
	  free(dl_report->csi_report[j]->a12csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_A22CSI:
	  free(dl_report->csi_report[j]->a22csi->wb_cqi);
	  free(dl_report->csi_report[j]->a22csi->sb_cqi);
	  free(dl_report->csi_report[j]->a22csi->sb_list);
	  free(dl_report->csi_report[j]->a22csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_A20CSI:
	  free(dl_report->csi_report[j]->a20csi->sb_list);
	  free(dl_report->csi_report[j]->a20csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_A30CSI:
	  free(dl_report->csi_report[j]->a30csi->sb_cqi);
	  free(dl_report->csi_report[j]->a30csi);
	  break;
	case PROTOCOL__FLEX_DL_CSI__REPORT_A31CSI:
	  free(dl_report->csi_report[j]->a31csi->wb_cqi);
	  for (k = 0; k < dl_report->csi_report[j]->a31csi->n_sb_cqi; k++) {
	    free(dl_report->csi_report[j]->a31csi->sb_cqi[k]);
	  }
	  free(dl_report->csi_report[j]->a31csi->sb_cqi);
	  break;
	default:
	  break;
	}

	free(dl_report->csi_report[j]);
      }
      free(dl_report->csi_report);
      free(dl_report);
    }
    // If Paging buffer report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PBS) {
      paging_report = reply->ue_report[i]->pbr;
      // Delete all paging buffer reports
      for (j = 0; j < paging_report->n_paging_info; j++) {
	free(paging_report->paging_info[j]);
      }
      free(paging_report->paging_info);
      free(paging_report);
    }
    // If UL CQI report flag was set
    if (reply->ue_report[i]->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_UL_CQI) {
      ul_report = reply->ue_report[i]->ul_cqi_report;
      for (j = 0; j < ul_report->n_cqi_meas; j++) {
	free(ul_report->cqi_meas[j]->sinr);
	free(ul_report->cqi_meas[j]);
      }
      free(ul_report->cqi_meas);
      for (j = 0; j < ul_report->n_pucch_dbm; j++) {
	free(ul_report->pucch_dbm[j]);
      }
      free(ul_report->pucch_dbm);
      free(ul_report);
    }
    if (reply->ue_report[i]->flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_MAC_STATS) {
      for (j = 0; j < reply->ue_report[i]->mac_stats->n_mac_sdus_dl; j++)
        free(reply->ue_report[i]->mac_stats->mac_sdus_dl[j]);
      free(reply->ue_report[i]->mac_stats->mac_sdus_dl);
      free(reply->ue_report[i]->mac_stats);
    }
  }

  // Free memory for all Cell reports
  for (i = 0; i < reply->n_cell_report; i++) {
    if (reply->cell_report[i]->flags & PROTOCOL__FLEX_CELL_STATS_TYPE__FLCST_NOISE_INTERFERENCE) {
      free(reply->cell_report[i]->noise_inter_report);
    }
  }

  return 0;
}

int flexran_agent_mac_sr_info(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header = NULL;
  int i;
  const int xid = *((int *)params);

  Protocol__FlexUlSrInfo *ul_sr_info_msg;
  ul_sr_info_msg = malloc(sizeof(Protocol__FlexUlSrInfo));
  if (ul_sr_info_msg == NULL) {
    goto error;
  }
  protocol__flex_ul_sr_info__init(ul_sr_info_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_UL_SR_INFO, &header) != 0)
    goto error;

  ul_sr_info_msg->header = header;
  ul_sr_info_msg->has_sfn_sf = 1;
  ul_sr_info_msg->sfn_sf = flexran_get_sfn_sf(mod_id);
  /*TODO: Set the number of UEs that sent an SR */
  ul_sr_info_msg->n_rnti = 1;
  ul_sr_info_msg->rnti = (uint32_t *) malloc(ul_sr_info_msg->n_rnti * sizeof(uint32_t));

  if(ul_sr_info_msg->rnti == NULL) {
    goto error;
  }
  /*TODO:Set the rnti of the UEs that sent an SR */
  for (i = 0; i < ul_sr_info_msg->n_rnti; i++) {
    ul_sr_info_msg->rnti[i] = 1;
  }

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_UL_SR_INFO_MSG;
  (*msg)->msg_dir =  PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->ul_sr_info_msg = ul_sr_info_msg;
  return 0;

 error:
  // TODO: Need to make proper error handling
  if (header != NULL)
    free(header);
  if (ul_sr_info_msg != NULL) {
    free(ul_sr_info_msg->rnti);
    free(ul_sr_info_msg);
  }
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_mac_destroy_sr_info(Protocol__FlexranMessage *msg) {
   if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_UL_SR_INFO_MSG)
     goto error;

   free(msg->ul_sr_info_msg->header);
   free(msg->ul_sr_info_msg->rnti);
   free(msg->ul_sr_info_msg);
   free(msg);
   return 0;

 error:
   //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
   return -1;
}

int flexran_agent_mac_sf_trigger(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {
  Protocol__FlexHeader *header = NULL;
  int i, j, UE_id;

  int available_harq[MAX_MOBILES_PER_ENB];

  const int xid = *((int *)params);


  Protocol__FlexSfTrigger *sf_trigger_msg;
  sf_trigger_msg = malloc(sizeof(Protocol__FlexSfTrigger));
  if (sf_trigger_msg == NULL) {
    goto error;
  }
  protocol__flex_sf_trigger__init(sf_trigger_msg);

  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_SF_TRIGGER, &header) != 0)
    goto error;

  frame_t frame;
  sub_frame_t subframe;

  for (i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    available_harq[i] = -1;
  }

  int ahead_of_time = 0;

  frame = (frame_t) flexran_get_current_system_frame_num(mod_id);
  subframe = (sub_frame_t) flexran_get_current_subframe(mod_id);

  subframe = ((subframe + ahead_of_time) % 10);

  if (subframe < flexran_get_current_subframe(mod_id)) {
    frame = (frame + 1) % 1024;
  }

  int additional_frames = ahead_of_time / 10;
  frame = (frame + additional_frames) % 1024;

  sf_trigger_msg->header = header;
  sf_trigger_msg->has_sfn_sf = 1;
  sf_trigger_msg->sfn_sf = flexran_get_future_sfn_sf(mod_id, 3);

  sf_trigger_msg->n_dl_info = 0;

  for (i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    for (j = 0; j < 8; j++) {
      if (RC.mac && RC.mac[mod_id] && RC.mac[mod_id]->UE_list.eNB_UE_stats[UE_PCCID(mod_id,i)][i].harq_pid == 1) {
	available_harq[i] = j;
	break;
      }
    }
  }


  //  LOG_I(FLEXRAN_AGENT, "Sending subframe trigger for frame %d and subframe %d\n", flexran_get_current_frame(mod_id), (flexran_get_current_subframe(mod_id) + 1) % 10);

  sf_trigger_msg->n_dl_info = flexran_get_mac_num_ues(mod_id);

  Protocol__FlexDlInfo **dl_info = NULL;

  if (sf_trigger_msg->n_dl_info > 0) {
    dl_info = malloc(sizeof(Protocol__FlexDlInfo *) * sf_trigger_msg->n_dl_info);
    if(dl_info == NULL)
      goto error;
    i = -1;
    //Fill the status of the current HARQ process for each UE
    for(i = 0; i < sf_trigger_msg->n_dl_info; i++) {
      if (available_harq[i] < 0)
	continue;
      dl_info[i] = malloc(sizeof(Protocol__FlexDlInfo));
      if(dl_info[i] == NULL)
	goto error;
      UE_id = flexran_get_mac_ue_id(mod_id, i);
      protocol__flex_dl_info__init(dl_info[i]);
      dl_info[i]->rnti = flexran_get_mac_ue_crnti(mod_id, UE_id);
      dl_info[i]->has_rnti = 1;
      /*Fill in the right id of this round's HARQ process for this UE*/
      //      uint8_t harq_id;
      //uint8_t harq_status;
      //      flexran_get_harq(mod_id, UE_PCCID(mod_id, UE_id), i, frame, subframe, &harq_id, &harq_status);


      dl_info[i]->harq_process_id = available_harq[UE_id];
      if (RC.mac && RC.mac[mod_id])
        RC.mac[mod_id]->UE_list.eNB_UE_stats[UE_PCCID(mod_id, UE_id)][UE_id].harq_pid = 0;
      dl_info[i]->has_harq_process_id = 1;
      /* Fill in the status of the HARQ process (2 TBs)*/
      dl_info[i]->n_harq_status = 2;
      dl_info[i]->harq_status = malloc(sizeof(uint32_t) * dl_info[i]->n_harq_status);
      for (j = 0; j < dl_info[i]->n_harq_status; j++) {
        dl_info[i]->harq_status[j] = RC.mac[mod_id]->UE_list.UE_sched_ctrl[UE_id].round[UE_PCCID(mod_id, UE_id)][j];
        // TODO: This should be different per TB
      }
      //      LOG_I(FLEXRAN_AGENT, "Sending subframe trigger for frame %d and subframe %d and harq %d (round %d)\n", flexran_get_current_frame(mod_id), (flexran_get_current_subframe(mod_id) + 1) % 10, dl_info[i]->harq_process_id, dl_info[i]->harq_status[0]);
      if(dl_info[i]->harq_status[0] > 0) {
	//	LOG_I(FLEXRAN_AGENT, "[Frame %d][Subframe %d]Need to make a retransmission for harq %d (round %d)\n", flexran_get_current_frame(mod_id), flexran_get_current_subframe(mod_id), dl_info[i]->harq_process_id, dl_info[i]->harq_status[0]);
      }
      /*Fill in the serving cell index for this UE */
      dl_info[i]->serv_cell_index = UE_PCCID(mod_id, UE_id);
      dl_info[i]->has_serv_cell_index = 1;
    }
  }

  sf_trigger_msg->dl_info = dl_info;

  /* Fill in the number of UL reception status related info, based on the number of currently
   * transmitting UEs
   */
  sf_trigger_msg->n_ul_info = flexran_get_mac_num_ues(mod_id);

  Protocol__FlexUlInfo **ul_info = NULL;

  if (sf_trigger_msg->n_ul_info > 0) {
    ul_info = malloc(sizeof(Protocol__FlexUlInfo *) * sf_trigger_msg->n_ul_info);
    if(ul_info == NULL)
      goto error;
    //Fill the reception info for each transmitting UE
    for(i = 0; i < sf_trigger_msg->n_ul_info; i++) {
      ul_info[i] = malloc(sizeof(Protocol__FlexUlInfo));
      if(ul_info[i] == NULL)
	goto error;
      protocol__flex_ul_info__init(ul_info[i]);

      UE_id = flexran_get_mac_ue_id(mod_id, i);

      ul_info[i]->rnti = flexran_get_mac_ue_crnti(mod_id, UE_id);
      ul_info[i]->has_rnti = 1;
      /* Fill in the Tx power control command for this UE (if available),
       * primary carrier */
      if(flexran_get_tpc(mod_id, UE_id, 0) != 1){
          /* assume primary carrier */
          ul_info[i]->tpc = flexran_get_tpc(mod_id, UE_id, 0);
          ul_info[i]->has_tpc = 1;
      }
      else{
          /* assume primary carrier */
          ul_info[i]->tpc = flexran_get_tpc(mod_id, UE_id, 0);
    	  ul_info[i]->has_tpc = 0;
      }
      /*TODO: fill in the amount of data in bytes in the MAC SDU received in this subframe for the
	given logical channel*/
      ul_info[i]->n_ul_reception = 0;
      ul_info[i]->ul_reception = malloc(sizeof(uint32_t) * ul_info[i]->n_ul_reception);
      for (j = 0; j < ul_info[i]->n_ul_reception; j++) {
	ul_info[i]->ul_reception[j] = 100;
      }
      /*TODO: Fill in the reception status for each UEs data*/
      ul_info[i]->reception_status = PROTOCOL__FLEX_RECEPTION_STATUS__FLRS_OK;
      ul_info[i]->has_reception_status = 1;
      /*Fill in the serving cell index for this UE */
      ul_info[i]->serv_cell_index = UE_PCCID(mod_id, UE_id);
      ul_info[i]->has_serv_cell_index = 1;
    }
  }

  sf_trigger_msg->ul_info = ul_info;

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_SF_TRIGGER_MSG;
  (*msg)->msg_dir =  PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->sf_trigger_msg = sf_trigger_msg;
  return 0;

 error:
  if (header != NULL)
    free(header);
  if (sf_trigger_msg != NULL) {
    for (i = 0; i < sf_trigger_msg->n_dl_info; i++) {
      free(sf_trigger_msg->dl_info[i]->harq_status);
    }
    free(sf_trigger_msg->dl_info);
    free(sf_trigger_msg->ul_info);
    free(sf_trigger_msg);
  }
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_mac_destroy_sf_trigger(Protocol__FlexranMessage *msg) {
  int i;
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_SF_TRIGGER_MSG)
    goto error;

  free(msg->sf_trigger_msg->header);
  for (i = 0; i < msg->sf_trigger_msg->n_dl_info; i++) {
    free(msg->sf_trigger_msg->dl_info[i]->harq_status);
    free(msg->sf_trigger_msg->dl_info[i]);
  }
  free(msg->sf_trigger_msg->dl_info);
  for (i = 0; i < msg->sf_trigger_msg->n_ul_info; i++) {
    free(msg->sf_trigger_msg->ul_info[i]->ul_reception);
    free(msg->sf_trigger_msg->ul_info[i]);
  }
  free(msg->sf_trigger_msg->ul_info);
  free(msg->sf_trigger_msg);
  free(msg);

  return 0;

 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int flexran_agent_mac_create_empty_dl_config(mid_t mod_id, Protocol__FlexranMessage **msg) {

  int xid = 0;
  Protocol__FlexHeader *header = NULL;
  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_DL_MAC_CONFIG, &header) != 0)
    goto error;

  Protocol__FlexDlMacConfig *dl_mac_config_msg;
  dl_mac_config_msg = malloc(sizeof(Protocol__FlexDlMacConfig));
  if (dl_mac_config_msg == NULL) {
    goto error;
  }
  protocol__flex_dl_mac_config__init(dl_mac_config_msg);

  dl_mac_config_msg->header = header;
  dl_mac_config_msg->has_sfn_sf = 1;
  dl_mac_config_msg->sfn_sf = flexran_get_sfn_sf(mod_id);

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_DL_MAC_CONFIG_MSG;
  (*msg)->msg_dir =  PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->dl_mac_config_msg = dl_mac_config_msg;

  return 0;

 error:
  return -1;
}

int flexran_agent_mac_destroy_dl_config(Protocol__FlexranMessage *msg) {
  int i,j, k;
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_DL_MAC_CONFIG_MSG)
    goto error;

  Protocol__FlexDlDci *dl_dci;

  free(msg->dl_mac_config_msg->header);
  for (i = 0; i < msg->dl_mac_config_msg->n_dl_ue_data; i++) {
    free(msg->dl_mac_config_msg->dl_ue_data[i]->ce_bitmap);
    for (j = 0; j < msg->dl_mac_config_msg->dl_ue_data[i]->n_rlc_pdu; j++) {
      for (k = 0; k <  msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]->n_rlc_pdu_tb; k++) {
	free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]->rlc_pdu_tb[k]);
      }
      free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]->rlc_pdu_tb);
      free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]);
    }
    free(msg->dl_mac_config_msg->dl_ue_data[i]->rlc_pdu);
    dl_dci = msg->dl_mac_config_msg->dl_ue_data[i]->dl_dci;
    free(dl_dci->tbs_size);
    free(dl_dci->mcs);
    free(dl_dci->ndi);
    free(dl_dci->rv);
    free(dl_dci);
    free(msg->dl_mac_config_msg->dl_ue_data[i]);
  }
  free(msg->dl_mac_config_msg->dl_ue_data);

  for (i = 0; i <  msg->dl_mac_config_msg->n_dl_rar; i++) {
    dl_dci = msg->dl_mac_config_msg->dl_rar[i]->rar_dci;
    free(dl_dci->tbs_size);
    free(dl_dci->mcs);
    free(dl_dci->ndi);
    free(dl_dci->rv);
    free(dl_dci);
    free(msg->dl_mac_config_msg->dl_rar[i]);
  }
  free(msg->dl_mac_config_msg->dl_rar);

  for (i = 0; i < msg->dl_mac_config_msg->n_dl_broadcast; i++) {
    dl_dci = msg->dl_mac_config_msg->dl_broadcast[i]->broad_dci;
    free(dl_dci->tbs_size);
    free(dl_dci->mcs);
    free(dl_dci->ndi);
    free(dl_dci->rv);
    free(dl_dci);
    free(msg->dl_mac_config_msg->dl_broadcast[i]);
  }
  free(msg->dl_mac_config_msg->dl_broadcast);

    for ( i = 0; i < msg->dl_mac_config_msg->n_ofdm_sym; i++) {
    free(msg->dl_mac_config_msg->ofdm_sym[i]);
  }
  free(msg->dl_mac_config_msg->ofdm_sym);
  free(msg->dl_mac_config_msg);
  free(msg);

  return 0;

 error:
  return -1;
}

int flexran_agent_mac_create_empty_ul_config(mid_t mod_id, Protocol__FlexranMessage **msg) {

  int xid = 0;
  Protocol__FlexHeader *header = NULL;
  if (flexran_create_header(xid, PROTOCOL__FLEX_TYPE__FLPT_UL_MAC_CONFIG, &header) != 0)
    goto error;

  Protocol__FlexUlMacConfig *ul_mac_config_msg;
  ul_mac_config_msg = malloc(sizeof(Protocol__FlexUlMacConfig));
  if (ul_mac_config_msg == NULL) {
    goto error;
  }
  protocol__flex_ul_mac_config__init(ul_mac_config_msg);

  ul_mac_config_msg->header = header;
  ul_mac_config_msg->has_sfn_sf = 1;
  ul_mac_config_msg->sfn_sf = flexran_get_sfn_sf(mod_id);

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL)
    goto error;
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_UL_MAC_CONFIG_MSG;
  (*msg)->msg_dir =  PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->ul_mac_config_msg = ul_mac_config_msg;

  return 0;

 error:
  return -1;
}


int flexran_agent_mac_destroy_ul_config(Protocol__FlexranMessage *msg) {
  int i; //,j, k;
  if(msg->msg_case != PROTOCOL__FLEXRAN_MESSAGE__MSG_UL_MAC_CONFIG_MSG)
    goto error;

  // Protocol__FlexUlDci *ul_dci;

  free(msg->ul_mac_config_msg->header);
  for (i = 0; i < msg->ul_mac_config_msg->n_ul_ue_data; i++) {
    // TODO  uplink rlc ...
    // free(msg->ul_mac_config_msg->dl_ue_data[i]->ce_bitmap);
  //   for (j = 0; j < msg->ul_mac_config_msg->ul_ue_data[i]->n_rlc_pdu; j++) {
  //     for (k = 0; k <  msg->ul_mac_config_msg->ul_ue_data[i]->rlc_pdu[j]->n_rlc_pdu_tb; k++) {
  // free(msg->ul_mac_config_msg->dl_ue_data[i]->rlc_pdu[j]->rlc_pdu_tb[k]);
  //     }
  //     free(msg->ul_mac_config_msg->ul_ue_data[i]->rlc_pdu[j]->rlc_pdu_tb);
  //     free(msg->ul_mac_config_msg->ul_ue_data[i]->rlc_pdu[j]);
  //   }
    // free(msg->ul_mac_config_msg->ul_ue_data[i]->rlc_pdu);
    // ul_dci = msg->ul_mac_config_msg->ul_ue_data[i]->ul_dci;
    // free(dl_dci->tbs_size);
    // free(ul_dci->mcs);
    // free(ul_dci->ndi);
    // free(ul_dci->rv);
    // free(ul_dci);
    // free(msg->ul_mac_config_msg->ul_ue_data[i]);
  }
  free(msg->ul_mac_config_msg->ul_ue_data);

  free(msg->ul_mac_config_msg);
  free(msg);

  return 0;

 error:
  return -1;
}


void flexran_agent_get_pending_dl_mac_config(mid_t mod_id, Protocol__FlexranMessage **msg) {

  struct lfds700_misc_prng_state ls;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);

  if (lfds700_ringbuffer_read(&ringbuffer_state[mod_id], NULL, (void **) msg, &ls) == 0) {
    *msg = NULL;
  }
}

int flexran_agent_mac_handle_dl_mac_config(mid_t mod_id, const void *params, Protocol__FlexranMessage **msg) {

  struct lfds700_misc_prng_state ls;
  enum lfds700_misc_flag overwrite_occurred_flag;
  Protocol__FlexranMessage *overwritten_dl_config;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);

  lfds700_ringbuffer_write( &ringbuffer_state[mod_id],
			    NULL,
			    (void *) params,
			    &overwrite_occurred_flag,
			    NULL,
			    (void **)&overwritten_dl_config,
			    &ls);

  if (overwrite_occurred_flag == LFDS700_MISC_FLAG_RAISED) {
    // Delete unmanaged dl_config
    flexran_agent_mac_destroy_dl_config(overwritten_dl_config);
  }
  *msg = NULL;
  return 2;

  // error:
  //*msg = NULL;
  //return -1;
}

void flexran_agent_init_mac_agent(mid_t mod_id) {
  int i, j;
  lfds700_misc_library_init_valid_on_current_logical_core();
  lfds700_misc_prng_init(&ps[mod_id]);
  int num_elements = RINGBUFFER_SIZE + 1;
  //Allow RINGBUFFER_SIZE messages to be stored in the ringbuffer at any time
  /* lfds700_ringbuffer_init_valid_on_current_logical_core()'s second argument
   * must be aligned to LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES. From the
   * documentation: "Heap allocated variables however will by no means be
   * correctly aligned and an aligned malloc must be used." Therefore, we use
   * posix_memalign */
  i = posix_memalign((void **)&dl_mac_config_array[mod_id],
                    LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES,
                    sizeof(struct lfds700_ringbuffer_element) *  num_elements);
  AssertFatal(i == 0, "posix_memalign(): could not allocate aligned memory for lfds library\n");
  lfds700_ringbuffer_init_valid_on_current_logical_core(&ringbuffer_state[mod_id],
      dl_mac_config_array[mod_id], num_elements, &ps[mod_id], NULL);
  for (i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    for (j = 0; j < 8; j++) {
      if (RC.mac && RC.mac[mod_id])
        RC.mac[mod_id]->UE_list.eNB_UE_stats[UE_PCCID(mod_id,i)][i].harq_pid = 0;
    }
  }
}

/***********************************************
 * FlexRAN agent - technology mac API implementation
 ***********************************************/

void flexran_agent_send_sr_info(mid_t mod_id) {
  int size;
  Protocol__FlexranMessage *msg;
  void *data;
  int priority = 0;
  err_code_t err_code;

  int xid = 0;

  /*TODO: Must use a proper xid*/
  err_code = flexran_agent_mac_sr_info(mod_id, (void *) &xid, &msg);
  if (err_code < 0) {
    goto error;
  }

  if (msg != NULL){
    data=flexran_agent_pack_message(msg, &size);
    /*Send sr info using the MAC channel of the eNB*/
    if (flexran_agent_msg_send(mod_id, FLEXRAN_AGENT_MAC, data, size, priority)) {
      err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(FLEXRAN_AGENT,"sent message with size %d\n", size);
    return;
  }
 error:
  LOG_D(FLEXRAN_AGENT, "Could not send sr message\n");
}

void flexran_agent_send_sf_trigger(mid_t mod_id) {
  int size;
  Protocol__FlexranMessage *msg;
  void *data;
  int priority = 0;
  err_code_t err_code;

  int xid = 0;

  /*TODO: Must use a proper xid*/
  err_code = flexran_agent_mac_sf_trigger(mod_id, (void *) &xid, &msg);
  if (err_code < 0) {
    goto error;
  }

  if (msg != NULL){
    data=flexran_agent_pack_message(msg, &size);
    /*Send sr info using the MAC channel of the eNB*/
    if (flexran_agent_msg_send(mod_id, FLEXRAN_AGENT_MAC, data, size, priority)) {
      err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENQUEUING;
      goto error;
    }

    LOG_D(FLEXRAN_AGENT,"sent message with size %d\n", size);
    return;
  }
 error:
  LOG_D(FLEXRAN_AGENT, "Could not send sf trigger message\n");
}



int flexran_agent_register_mac_xface(mid_t mod_id)
{
  if (agent_mac_xface[mod_id]) {
    LOG_E(MAC, "MAC agent for eNB %d is already registered\n", mod_id);
    return -1;
  }
  AGENT_MAC_xface *xface = malloc(sizeof(AGENT_MAC_xface));
  if (!xface) {
    LOG_E(FLEXRAN_AGENT, "could not allocate memory for MAC agent xface %d\n", mod_id);
    return -1;
  }

  //xface->agent_ctxt = &shared_ctxt[mod_id];
  xface->flexran_agent_send_sr_info = flexran_agent_send_sr_info;
  xface->flexran_agent_send_sf_trigger = flexran_agent_send_sf_trigger;
  //xface->flexran_agent_send_update_mac_stats = flexran_agent_send_update_mac_stats;
  xface->flexran_agent_get_pending_dl_mac_config = flexran_agent_get_pending_dl_mac_config;

  xface->dl_scheduler_loaded_lib = NULL;
  xface->ul_scheduler_loaded_lib = NULL;
  agent_mac_xface[mod_id] = xface;

  return 0;
}

void flexran_agent_fill_mac_cell_config(mid_t mod_id, uint8_t cc_id,
    Protocol__FlexCellConfig *conf) {
  if (!conf->si_config) {
    conf->si_config = malloc(sizeof(Protocol__FlexSiConfig));
    if (conf->si_config)
      protocol__flex_si_config__init(conf->si_config);
  }

  if (conf->si_config) {
    conf->si_config->sfn = flexran_get_current_system_frame_num(mod_id);
    conf->si_config->has_sfn = 1;
  }

  /* get a pointer to the config which is maintained in the agent throughout
  * its lifetime */
  conf->slice_config = flexran_agent_get_slice_config(mod_id);
}

void flexran_agent_fill_mac_ue_config(mid_t mod_id, mid_t ue_id,
    Protocol__FlexUeConfig *ue_conf)
{
  if (ue_conf->has_rnti && ue_conf->rnti != flexran_get_mac_ue_crnti(mod_id, ue_id)) {
    LOG_E(FLEXRAN_AGENT, "ue_config existing RNTI %x does not match MAC RNTI %x\n",
          ue_conf->rnti, flexran_get_mac_ue_crnti(mod_id, ue_id));
    return;
  }
  ue_conf->rnti = flexran_get_mac_ue_crnti(mod_id, ue_id);
  ue_conf->has_rnti = 1;

  ue_conf->dl_slice_id = flexran_get_ue_dl_slice_id(mod_id, ue_id);
  ue_conf->has_dl_slice_id = 1;
  ue_conf->ul_slice_id = flexran_get_ue_ul_slice_id(mod_id, ue_id);
  ue_conf->has_ul_slice_id = 1;

  ue_conf->ue_aggregated_max_bitrate_ul = flexran_get_ue_aggregated_max_bitrate_ul(mod_id, ue_id);
  ue_conf->has_ue_aggregated_max_bitrate_ul = 1;

  ue_conf->ue_aggregated_max_bitrate_dl = flexran_get_ue_aggregated_max_bitrate_dl(mod_id, ue_id);
  ue_conf->has_ue_aggregated_max_bitrate_dl = 1;

  /* TODO update through RAN API */
  //config->has_pcell_carrier_index = 1;
  //config->pcell_carrier_index = UE_PCCID(mod_id, i);

  //TODO: Set carrier aggregation support (boolean)
}

void flexran_agent_fill_mac_lc_ue_config(mid_t mod_id, mid_t ue_id,
    Protocol__FlexLcUeConfig *lc_ue_conf)
{
  lc_ue_conf->rnti = flexran_get_mac_ue_crnti(mod_id, ue_id);
  lc_ue_conf->has_rnti = 1;

  lc_ue_conf->n_lc_config = flexran_get_num_ue_lcs(mod_id, ue_id);
  if (lc_ue_conf->n_lc_config == 0)
    return;

  Protocol__FlexLcConfig **lc_config =
    calloc(lc_ue_conf->n_lc_config, sizeof(Protocol__FlexLcConfig *));
  if (!lc_config) {
    LOG_E(FLEXRAN_AGENT, "could not allocate memory for lc_config of UE %x\n", lc_ue_conf->rnti);
    lc_ue_conf->n_lc_config = 0;
    return; // can not allocate memory, skip rest
  }
  for (int j = 0; j < lc_ue_conf->n_lc_config; j++) {
    lc_config[j] = malloc(sizeof(Protocol__FlexLcConfig));
    if (!lc_config[j]) continue; // go over this error, try entry
    protocol__flex_lc_config__init(lc_config[j]);

    lc_config[j]->has_lcid = 1;
    lc_config[j]->lcid = j+1;

    const int lcg = flexran_get_lcg(mod_id, ue_id, j+1);
    if (lcg >= 0 && lcg <= 3) {
      lc_config[j]->has_lcg = 1;
      lc_config[j]->lcg = flexran_get_lcg(mod_id, ue_id, j+1);
    }

    lc_config[j]->has_direction = 1;
    lc_config[j]->direction = flexran_get_direction(ue_id, j+1);
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
  lc_ue_conf->lc_config = lc_config;
}

int flexran_agent_unregister_mac_xface(mid_t mod_id)
{
  if (!agent_mac_xface[mod_id]) {
    LOG_E(FLEXRAN_AGENT, "MAC agent CM for eNB %d is not registered\n", mod_id);
    return -1;
  }
  AGENT_MAC_xface *xface = agent_mac_xface[mod_id];
  //xface->agent_ctxt = NULL;
  xface->flexran_agent_send_sr_info = NULL;
  xface->flexran_agent_send_sf_trigger = NULL;
  //xface->flexran_agent_send_update_mac_stats = NULL;
  xface->flexran_agent_get_pending_dl_mac_config = NULL;

  xface->dl_scheduler_loaded_lib = NULL;
  xface->ul_scheduler_loaded_lib = NULL;
  free(xface);
  agent_mac_xface[mod_id] = NULL;

  return 0;
}

AGENT_MAC_xface *flexran_agent_get_mac_xface(mid_t mod_id)
{
  return agent_mac_xface[mod_id];
}

void flexran_create_config_structures(mid_t mod_id)
{
  int i;
  int n_dl = flexran_get_num_dl_slices(mod_id);
  int m_ul = flexran_get_num_ul_slices(mod_id);
  slice_config[mod_id] = flexran_agent_create_slice_config(n_dl, m_ul);
  sc_update[mod_id] = flexran_agent_create_slice_config(n_dl, m_ul);
  if (!slice_config[mod_id] || !sc_update[mod_id]) return;

  flexran_agent_read_slice_config(mod_id, slice_config[mod_id]);
  flexran_agent_read_slice_config(mod_id, sc_update[mod_id]);
  for (i = 0; i < n_dl; i++) {
    flexran_agent_read_slice_dl_config(mod_id, i, slice_config[mod_id]->dl[i]);
    flexran_agent_read_slice_dl_config(mod_id, i, sc_update[mod_id]->dl[i]);
  }
  for (i = 0; i < m_ul; i++) {
    flexran_agent_read_slice_ul_config(mod_id, i, slice_config[mod_id]->ul[i]);
    flexran_agent_read_slice_ul_config(mod_id, i, sc_update[mod_id]->ul[i]);
  }
}

void flexran_check_and_remove_slices(mid_t mod_id)
{
  Protocol__FlexDlSlice **dl = sc_update[mod_id]->dl;
  Protocol__FlexDlSlice **dlreal = slice_config[mod_id]->dl;
  int i = 0;
  while (i < sc_update[mod_id]->n_dl) {
    /* remove slices whose percentage is zero */
    if (dl[i]->percentage > 0) {
      ++i;
      continue;
    }
    if (flexran_remove_dl_slice(mod_id, i) < 1) {
      LOG_W(FLEXRAN_AGENT, "[%d] can not remove slice index %d ID %d\n",
            mod_id, i, dl[i]->id);
      ++i;
      continue;
    }
    LOG_I(FLEXRAN_AGENT, "[%d] removed slice index %d ID %d\n",
          mod_id, i, dl[i]->id);
    if (dl[i]->n_sorting > 0) free(dl[i]->sorting);
    free(dl[i]->scheduler_name);
    if (dlreal[i]->n_sorting > 0) {
      dlreal[i]->n_sorting = 0;
      free(dlreal[i]->sorting);
    }
    free(dlreal[i]->scheduler_name);
    --sc_update[mod_id]->n_dl;
    --slice_config[mod_id]->n_dl;
    const size_t last = sc_update[mod_id]->n_dl;
    /* we need to memcpy the higher slice to the position we just deleted */
    memcpy(dl[i], dl[last], sizeof(*dl[last]));
    memset(dl[last], 0, sizeof(*dl[last]));
    memcpy(dlreal[i], dlreal[last], sizeof(*dlreal[last]));
    memset(dlreal[last], 0, sizeof(*dlreal[last]));
    /* dont increase i but recheck the slice which has been copied to here */
  }
  Protocol__FlexUlSlice **ul = sc_update[mod_id]->ul;
  Protocol__FlexUlSlice **ulreal = slice_config[mod_id]->ul;
  i = 0;
  while (i < sc_update[mod_id]->n_ul) {
    if (ul[i]->percentage > 0) {
      ++i;
      continue;
    }
    if (flexran_remove_ul_slice(mod_id, i) < 1) {
      LOG_W(FLEXRAN_AGENT, "[%d] can not remove slice index %d ID %d\n",
            mod_id, i, ul[i]->id);
      ++i;
      continue;
    }
    LOG_I(FLEXRAN_AGENT, "[%d] removed slice index %d ID %d\n",
          mod_id, i, ul[i]->id);
    free(ul[i]->scheduler_name);
    free(ulreal[i]->scheduler_name);
    --sc_update[mod_id]->n_ul;
    --slice_config[mod_id]->n_ul;
    const size_t last = sc_update[mod_id]->n_ul;
    /* see DL remarks */
    memcpy(ul[i], ul[last], sizeof(*ul[last]));
    memset(ul[last], 0, sizeof(*ul[last]));
    memcpy(ulreal[i], ulreal[last], sizeof(*ulreal[last]));
    memset(ulreal[last], 0, sizeof(*ulreal[last]));
    /* dont increase i but recheck the slice which has been copied to here */
  }
}

void flexran_agent_slice_update(mid_t mod_id)
{
  int i;
  int changes = 0;

  if (perform_slice_config_update_count <= 0) return;
  perform_slice_config_update_count--;

  pthread_mutex_lock(&sc_update_mtx);

  if (!slice_config[mod_id]) {
    /* if the configuration does not exist for agent, create from eNB structure
     * and exit */
    flexran_create_config_structures(mod_id);
    pthread_mutex_unlock(&sc_update_mtx);
    return;
  }

  /********* read existing config *********/
  /* simply update slice_config all the time and write new config
   * (apply_new_slice_dl_config() only updates if changes are necessary) */
  slice_config[mod_id]->n_dl = flexran_get_num_dl_slices(mod_id);
  slice_config[mod_id]->n_ul = flexran_get_num_ul_slices(mod_id);
  for (i = 0; i < slice_config[mod_id]->n_dl; i++) {
    flexran_agent_read_slice_dl_config(mod_id, i, slice_config[mod_id]->dl[i]);
  }
  for (i = 0; i < slice_config[mod_id]->n_ul; i++) {
    flexran_agent_read_slice_ul_config(mod_id, i, slice_config[mod_id]->ul[i]);
  }

  /********* write new config *********/
  /* check for removal (sc_update[X]->dl[Y].percentage == 0)
   * and update sc_update & slice_config accordingly */
  flexran_check_and_remove_slices(mod_id);

  /* create new DL and UL slices if necessary */
  for (i = slice_config[mod_id]->n_dl; i < sc_update[mod_id]->n_dl; i++) {
    flexran_create_dl_slice(mod_id, sc_update[mod_id]->dl[i]->id);
  }
  for (i = slice_config[mod_id]->n_ul; i < sc_update[mod_id]->n_ul; i++) {
    flexran_create_ul_slice(mod_id, sc_update[mod_id]->ul[i]->id);
  }
  slice_config[mod_id]->n_dl = flexran_get_num_dl_slices(mod_id);
  slice_config[mod_id]->n_ul = flexran_get_num_ul_slices(mod_id);
  changes += apply_new_slice_config(mod_id, slice_config[mod_id], sc_update[mod_id]);
  for (i = 0; i < slice_config[mod_id]->n_dl; i++) {
    changes += apply_new_slice_dl_config(mod_id,
                                         slice_config[mod_id]->dl[i],
                                         sc_update[mod_id]->dl[i]);
    flexran_agent_read_slice_dl_config(mod_id, i, slice_config[mod_id]->dl[i]);
  }
  for (i = 0; i < slice_config[mod_id]->n_ul; i++) {
    changes += apply_new_slice_ul_config(mod_id,
                                         slice_config[mod_id]->ul[i],
                                         sc_update[mod_id]->ul[i]);
    flexran_agent_read_slice_ul_config(mod_id, i, slice_config[mod_id]->ul[i]);
  }
  flexran_agent_read_slice_config(mod_id, slice_config[mod_id]);
  if (n_ue_slice_assoc_updates > 0) {
    changes += apply_ue_slice_assoc_update(mod_id);
  }
  if (changes > 0)
    LOG_I(FLEXRAN_AGENT, "[%d] slice configuration: applied %d changes\n", mod_id, changes);

  pthread_mutex_unlock(&sc_update_mtx);
}

Protocol__FlexSliceConfig *flexran_agent_get_slice_config(mid_t mod_id)
{
  if (!slice_config[mod_id]) return NULL;
  Protocol__FlexSliceConfig *config = NULL;

  pthread_mutex_lock(&sc_update_mtx);
  config = flexran_agent_create_slice_config(slice_config[mod_id]->n_dl,
                                             slice_config[mod_id]->n_ul);
  if (!config) {
    pthread_mutex_unlock(&sc_update_mtx);
    return NULL;
  }
  config->has_intraslice_share_active = 1;
  config->intraslice_share_active = slice_config[mod_id]->intraslice_share_active;
  config->has_interslice_share_active = 1;
  config->interslice_share_active = slice_config[mod_id]->interslice_share_active;
  for (int i = 0; i < slice_config[mod_id]->n_dl; ++i) {
    if (!config->dl[i]) continue;
    config->dl[i]->has_id         = 1;
    config->dl[i]->id             = slice_config[mod_id]->dl[i]->id;
    config->dl[i]->has_label      = 1;
    config->dl[i]->label          = slice_config[mod_id]->dl[i]->label;
    config->dl[i]->has_percentage = 1;
    config->dl[i]->percentage     = slice_config[mod_id]->dl[i]->percentage;
    config->dl[i]->has_isolation  = 1;
    config->dl[i]->isolation      = slice_config[mod_id]->dl[i]->isolation;
    config->dl[i]->has_priority   = 1;
    config->dl[i]->priority       = slice_config[mod_id]->dl[i]->priority;
    config->dl[i]->has_position_low  = 1;
    config->dl[i]->position_low   = slice_config[mod_id]->dl[i]->position_low;
    config->dl[i]->has_position_high = 1;
    config->dl[i]->position_high  = slice_config[mod_id]->dl[i]->position_high;
    config->dl[i]->has_maxmcs     = 1;
    config->dl[i]->maxmcs         = slice_config[mod_id]->dl[i]->maxmcs;
    config->dl[i]->n_sorting      = slice_config[mod_id]->dl[i]->n_sorting;
    config->dl[i]->sorting        = calloc(config->dl[i]->n_sorting, sizeof(Protocol__FlexDlSorting));
    if (!config->dl[i]->sorting) config->dl[i]->n_sorting = 0;
    for (int j = 0; j < config->dl[i]->n_sorting; ++j)
      config->dl[i]->sorting[j]   = slice_config[mod_id]->dl[i]->sorting[j];
    config->dl[i]->has_accounting = 1;
    config->dl[i]->accounting     = slice_config[mod_id]->dl[i]->accounting;
    config->dl[i]->scheduler_name = strdup(slice_config[mod_id]->dl[i]->scheduler_name);
  }
  for (int i = 0; i < slice_config[mod_id]->n_ul; ++i) {
    if (!config->ul[i]) continue;
    config->ul[i]->has_id         = 1;
    config->ul[i]->id             = slice_config[mod_id]->ul[i]->id;
    config->ul[i]->has_label      = 1;
    config->ul[i]->label          = slice_config[mod_id]->ul[i]->label;
    config->ul[i]->has_percentage = 1;
    config->ul[i]->percentage     = slice_config[mod_id]->ul[i]->percentage;
    config->ul[i]->has_isolation  = 1;
    config->ul[i]->isolation      = slice_config[mod_id]->ul[i]->isolation;
    config->ul[i]->has_priority   = 1;
    config->ul[i]->priority       = slice_config[mod_id]->ul[i]->priority;
    config->ul[i]->has_first_rb   = 1;
    config->ul[i]->first_rb       = slice_config[mod_id]->ul[i]->first_rb;
    config->ul[i]->has_maxmcs     = 1;
    config->ul[i]->maxmcs         = slice_config[mod_id]->ul[i]->maxmcs;
    config->ul[i]->n_sorting      = slice_config[mod_id]->ul[i]->n_sorting;
    config->ul[i]->sorting        = calloc(config->ul[i]->n_sorting, sizeof(Protocol__FlexUlSorting));
    if (!config->ul[i]->sorting) config->ul[i]->n_sorting = 0;
    for (int j = 0; j < config->ul[i]->n_sorting; ++j)
      config->ul[i]->sorting[j]   = slice_config[mod_id]->ul[i]->sorting[j];
    config->ul[i]->has_accounting = 1;
    config->ul[i]->accounting     = slice_config[mod_id]->ul[i]->accounting;
    config->ul[i]->scheduler_name = strdup(slice_config[mod_id]->ul[i]->scheduler_name);
  }

  pthread_mutex_unlock(&sc_update_mtx);
  return config;
}
