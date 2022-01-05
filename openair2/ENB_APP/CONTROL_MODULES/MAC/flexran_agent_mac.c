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

#include <dlfcn.h>

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

/*Array containing the Agent-MAC interfaces*/
AGENT_MAC_xface *agent_mac_xface[NUM_MAX_ENB];

/* Ringbuffer related structs used for maintaining the dl mac config messages */
//message_queue_t *dl_mac_config_queue;
struct lfds700_misc_prng_state ps[NUM_MAX_ENB];
struct lfds700_ringbuffer_element *dl_mac_config_array[NUM_MAX_ENB];
struct lfds700_ringbuffer_state ringbuffer_state[NUM_MAX_ENB];

/* Ringbuffer-related to slice configuration */
struct lfds700_ringbuffer_element *slice_config_array[NUM_MAX_ENB];
struct lfds700_ringbuffer_state slice_config_ringbuffer_state[NUM_MAX_ENB];

/* Ringbuffer related to slice configuration if we need to wait for additional
 * controller information */
struct lfds700_ringbuffer_element *store_slice_config_array[NUM_MAX_ENB];
struct lfds700_ringbuffer_state store_slice_config_ringbuffer_state[NUM_MAX_ENB];
/* stores the (increasing) xid for messages to prevent double use */
int xid = 50;

/* Ringbuffer-related to slice configuration */
struct lfds700_ringbuffer_element *ue_assoc_array[NUM_MAX_ENB];
struct lfds700_ringbuffer_state ue_assoc_ringbuffer_state[NUM_MAX_ENB];

/* a list of shared objects that are loaded into the user plane */
SLIST_HEAD(flexran_so_handle, flexran_agent_so_handle_s) flexran_handles[NUM_MAX_ENB];

int flexran_agent_mac_stats_reply_ue(mid_t mod_id,
                                    Protocol__FlexUeStatsReport **ue_report,
                                    int      n_ue,
                                    uint32_t ue_flags) {
  const int cc_id = 0; // TODO: make for multiple CCs

  for (int i = 0; i < n_ue; i++) {
    const rnti_t rnti = ue_report[i]->rnti;
    const int UE_id = flexran_get_mac_ue_id_rnti(mod_id, rnti);

    /* buffer status report */
    if (ue_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_BSR) {
      ue_report[i]->n_bsr = 4; //TODO should be automated
      uint32_t *elem = (uint32_t *) malloc(sizeof(uint32_t)*ue_report[i]->n_bsr);
      if (elem == NULL)
        goto error;
      for (int j = 0; j < ue_report[i]->n_bsr; j++) {
        elem[j] = flexran_get_ue_bsr_ul_buffer_info (mod_id, i, j);
      }
      ue_report[i]->bsr = elem;
      ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_BSR;
    }

    /* power headroom report */
    if (ue_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PHR) {
      ue_report[i]->phr = flexran_get_ue_phr (mod_id, UE_id);
      ue_report[i]->has_phr = 1;
      ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PHR;
    }

    /* Check flag for creation of RLC buffer status report */
    if (ue_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_RLC_BS) {
      ue_report[i]->n_rlc_report = 3; // Set this to the number of LCs for this UE. This needs to be generalized for for LCs
      Protocol__FlexRlcBsr ** rlc_reports;
      rlc_reports = malloc(sizeof(Protocol__FlexRlcBsr *) * ue_report[i]->n_rlc_report);
      if (rlc_reports == NULL)
        goto error;

      for (int j = 0; j < ue_report[i]->n_rlc_report; j++) {
        rlc_reports[j] = malloc(sizeof(Protocol__FlexRlcBsr));
        if (rlc_reports[j] == NULL) {
          for (int k = 0; k < j; k++) {
            free(rlc_reports[k]);
          }
          free(rlc_reports);
          goto error;
        }
        protocol__flex_rlc_bsr__init(rlc_reports[j]);
        rlc_reports[j]->lc_id = j+1;
        rlc_reports[j]->has_lc_id = 1;
        rlc_reports[j]->tx_queue_size = flexran_get_tx_queue_size(mod_id, UE_id, j + 1);
        rlc_reports[j]->has_tx_queue_size = 1;

        rlc_reports[j]->tx_queue_hol_delay = flexran_get_hol_delay(mod_id, UE_id, j + 1);
        rlc_reports[j]->has_tx_queue_hol_delay = 1;
        //TODO:Set retransmission queue size in bytes
        rlc_reports[j]->retransmission_queue_size = 10;
        rlc_reports[j]->has_retransmission_queue_size = 0;
        //TODO:Set retransmission queue head of line delay in ms
        rlc_reports[j]->retransmission_queue_hol_delay = 100;
        rlc_reports[j]->has_retransmission_queue_hol_delay = 0;
        rlc_reports[j]->status_pdu_size = flexran_get_num_pdus_buffer(mod_id, UE_id, j + 1);
        rlc_reports[j]->has_status_pdu_size = 1;

      }
      if (ue_report[i]->n_rlc_report > 0)
        ue_report[i]->rlc_report = rlc_reports;
      ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_RLC_BS;
    }

    /* MAC CE buffer status report */
    if (ue_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_MAC_CE_BS) {
      // TODO: Fill in the actual MAC CE buffer status report
      ue_report[i]->pending_mac_ces = (flexran_get_MAC_CE_bitmap_TA(mod_id, UE_id, 0) | (0 << 1) | (0 << 2) | (0 << 3)) & 15;
      // Use as bitmap. Set one or more of the;
      // PROTOCOL__FLEX_CE_TYPE__FLPCET_ values
      // found in stats_common.pb-c.h. See
      // flex_ce_type in FlexRAN specification
      ue_report[i]->has_pending_mac_ces = 1;
      ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_MAC_CE_BS;
    }

    /* DL CQI report */
    if (ue_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_DL_CQI) {
      // TODO: Fill in the actual DL CQI report for the UE based on its configuration
      Protocol__FlexDlCqiReport * dl_report;
      dl_report = malloc(sizeof(Protocol__FlexDlCqiReport));
      if (dl_report == NULL)
        goto error;
      protocol__flex_dl_cqi_report__init(dl_report);

      dl_report->sfn_sn = flexran_get_sfn_sf(mod_id);
      dl_report->has_sfn_sn = 1;
      dl_report->n_csi_report = flexran_get_active_CC(mod_id, UE_id);
      dl_report->n_csi_report = 1 ;
      //Create the actual CSI reports.
      Protocol__FlexDlCsi **csi_reports;
      csi_reports = malloc(sizeof(Protocol__FlexDlCsi *)*dl_report->n_csi_report);
      if (csi_reports == NULL) {
        free(dl_report);
        goto error;
      }
      for (int j = 0; j < dl_report->n_csi_report; j++) {
        csi_reports[j] = malloc(sizeof(Protocol__FlexDlCsi));
        if (csi_reports[j] == NULL) {
          for (int k = 0; k < j; k++) {
            free(csi_reports[k]);
          }
          free(csi_reports);
          csi_reports = NULL;
          goto error;
        }
        protocol__flex_dl_csi__init(csi_reports[j]);
        csi_reports[j]->serv_cell_index = j;
        csi_reports[j]->has_serv_cell_index = 1;
        csi_reports[j]->ri = flexran_get_current_RI(mod_id, UE_id, j);
        csi_reports[j]->has_ri = 1;
        //TODO: the type of CSI report based on the configuration of the UE
        //For now we only support type P10, which only needs a wideband value
        csi_reports[j]->type =  PROTOCOL__FLEX_CSI_TYPE__FLCSIT_P10;
        csi_reports[j]->has_type = 1;
        csi_reports[j]->report_case = PROTOCOL__FLEX_DL_CSI__REPORT_P10CSI;

        if (csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P10CSI) {
          Protocol__FlexCsiP10 *csi10;
          csi10 = malloc(sizeof(Protocol__FlexCsiP10));
          if (csi10 == NULL) {
            for (int k = 0; k <= j; k++) {
              free(csi_reports[k]);
            }
            free(csi_reports);
            csi_reports = NULL;
            goto error;
          }
          protocol__flex_csi_p10__init(csi10);
          csi10->wb_cqi = flexran_get_ue_wcqi(mod_id, UE_id);
          csi10->has_wb_cqi = 1;
          csi_reports[j]->p10csi = csi10;
        }

        /*
        else if (csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P11CSI) {


          Protocol__FlexCsiP11 *csi11;
          csi11 = malloc(sizeof(Protocol__FlexCsiP11));
          if (csi11 == NULL) {
            for (int k = 0; k <= j; k++) {
              free(csi_reports[k]);
            }
            free(csi_reports);
            csi_reports = NULL;
            goto error;
          }
          protocol__flex_csi_p11__init(csi11);

          csi11->wb_cqi = malloc(sizeof(csi11->wb_cqi));
          csi11->n_wb_cqi = 1;
          csi11->wb_cqi[0] = flexran_get_ue_wcqi (mod_id, UE_id);
          // According To spec 36.213

          if (flexran_get_antenna_ports(mod_id, j) == 2 && csi_reports[j]->ri == 1) {
            // TODO PMI
            csi11->wb_pmi = flexran_get_ue_wpmi(mod_id, UE_id, 0);
            csi11->has_wb_pmi = 1;

          }

          else if (flexran_get_antenna_ports(mod_id, j) == 2 && csi_reports[j]->ri == 2) {
            // TODO PMI
            csi11->wb_pmi = flexran_get_ue_wpmi(mod_id, UE_id, 0);
            csi11->has_wb_pmi = 1;

          }

          else if (flexran_get_antenna_ports(mod_id, j) == 4 && csi_reports[j]->ri == 2) {
            // TODO PMI
            csi11->wb_pmi = flexran_get_ue_wpmi(mod_id, UE_id, 0);
            csi11->has_wb_pmi = 1;


          }

          csi11->has_wb_pmi = 0;
          csi_reports[j]->p11csi = csi11;
        }
        else if (csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P20CSI) {

          Protocol__FlexCsiP20 *csi20;
          csi20 = malloc(sizeof(Protocol__FlexCsiP20));
          if (csi20 == NULL) {
            for (int k = 0; k <= j; k++) {
              free(csi_reports[k]);
            }
            free(csi_reports);
            csi_reports = NULL;
            goto error;
          }
          protocol__flex_csi_p20__init(csi20);

          csi20->wb_cqi = flexran_get_ue_wcqi (mod_id, UE_id);
          csi20->has_wb_cqi = 1;
          csi20->bandwidth_part_index = 1 ; //TODO
          csi20->has_bandwidth_part_index = 0;
          csi20->sb_index = 1; //TODO
          csi20->has_sb_index = 1;
          csi_reports[j]->p20csi = csi20;
        }
        else if (csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_P21CSI) {

          // Protocol__FlexCsiP21 *csi21;
          // csi21 = malloc(sizeof(Protocol__FlexCsiP21));
          // if (csi21 == NULL)
          // goto error;
          // protocol__flex_csi_p21__init(csi21);

          // csi21->wb_cqi = flexran_get_ue_wcqi (mod_id, UE_id);


          // csi21->wb_pmi = flexran_get_ue_pmi(mod_id); //TDO inside
          // csi21->has_wb_pmi = 1;

          // csi21->sb_cqi = 1; // TODO

          // csi21->bandwidth_part_index = 1 ; //TDO inside
          // csi21->has_bandwidth_part_index = 1 ;

          // csi21->sb_index = 1 ;//TODO
          // csi21->has_sb_index = 1 ;


          // csi_reports[j]->p20csi = csi21;

        }

        else if (csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A12CSI) {


          // Protocol__FlexCsiA12 *csi12;
          // csi12 = malloc(sizeof(Protocol__FlexCsiA12));
          // if (csi12 == NULL)
          // goto error;
          // protocol__flex_csi_a12__init(csi12);

          // csi12->wb_cqi = flexran_get_ue_wcqi (mod_id, UE_id);

          // csi12->sb_pmi = 1 ; //TODO inside

          // TODO continou
        }

        else if (csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A22CSI) {

          // Protocol__FlexCsiA22 *csi22;
          // csi22 = malloc(sizeof(Protocol__FlexCsiA22));
          // if (csi22 == NULL)
          // goto error;
          // protocol__flex_csi_a22__init(csi22);

          // csi22->wb_cqi = flexran_get_ue_wcqi (mod_id, UE_id);

          // csi22->sb_cqi = 1 ; //TODO inside

          // csi22->wb_pmi = flexran_get_ue_wcqi (mod_id, UE_id);
          // csi22->has_wb_pmi = 1;

          // csi22->sb_pmi = 1 ; //TODO inside
          // csi22->has_wb_pmi = 1;

          // csi22->sb_list = flexran_get_ue_wcqi (mod_id, UE_id);


        }

        else if (csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A20CSI) {

          // Protocol__FlexCsiA20 *csi20;
          // csi20 = malloc(sizeof(Protocol__FlexCsiA20));
          // if (csi20 == NULL)
          // goto error;
          // protocol__flex_csi_a20__init(csi20);

          // csi20->wb_cqi = flexran_get_ue_wcqi (mod_id, UE_id);
          // csi20->has_wb_cqi = 1;

          // csi20>sb_cqi = 1 ; //TODO inside
          // csi20>has_sb_cqi = 1 ;

          // csi20->sb_list = 1; // TODO inside


        }

        else if (csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A30CSI) {

        }

        else if (csi_reports[j]->report_case == PROTOCOL__FLEX_DL_CSI__REPORT_A31CSI) {

        }
        */

      }
      dl_report->csi_report = csi_reports;
      ue_report[i]->dl_cqi_report = dl_report;
      ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_DL_CQI;
    }

    /* paging buffer status report */
    if (ue_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PBS) {
      //TODO: Fill in the actual paging buffer status report. For this field to be valid, the RNTI
      //set in the report must be a P-RNTI
      Protocol__FlexPagingBufferReport *paging_report;
      paging_report = malloc(sizeof(Protocol__FlexPagingBufferReport));
      if (paging_report == NULL)
        goto error;
      protocol__flex_paging_buffer_report__init(paging_report);
      paging_report->n_paging_info = 1;
      Protocol__FlexPagingInfo **p_info;
      p_info = malloc(sizeof(Protocol__FlexPagingInfo *) * paging_report->n_paging_info);
      if (p_info == NULL) {
        free(paging_report);
        goto error;
      }

      for (int j = 0; j < paging_report->n_paging_info; j++) {

        p_info[j] = malloc(sizeof(Protocol__FlexPagingInfo));
        if (p_info[j] == NULL) {
          for (int k = 0; k < j; k++) {
            free(p_info[k]);
          }
          free(p_info);
          p_info = NULL;
          free(paging_report);
          goto error;
        }
        protocol__flex_paging_info__init(p_info[j]);
        //TODO: Set paging index. This index is the same that will be used for the scheduling of the
        //paging message by the controller
        p_info[j]->paging_index = 10;
        p_info[j]->has_paging_index = 0;
        //TODO:Set the paging message size
        p_info[j]->paging_message_size = 100;
        p_info[j]->has_paging_message_size = 0;
        //TODO: Set the paging subframe
        p_info[j]->paging_subframe = 10;
        p_info[j]->has_paging_subframe = 0;
        //TODO: Set the carrier index for the pending paging message
        p_info[j]->carrier_index = 0;
        p_info[j]->has_carrier_index = 0;

      }
      //Add all paging info to the paging buffer rerport
      paging_report->paging_info = p_info;
      //Add the paging report to the UE report
      ue_report[i]->pbr = paging_report;
      ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_PBS;
    }

    /* Check flag for creation of UL CQI report */
    if (ue_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_UL_CQI) {

      //Fill in the full UL CQI report of the UE
      Protocol__FlexUlCqiReport *full_ul_report;
      full_ul_report = malloc(sizeof(Protocol__FlexUlCqiReport));
      if (full_ul_report == NULL)
        goto error;
      protocol__flex_ul_cqi_report__init(full_ul_report);
      full_ul_report->sfn_sn = flexran_get_sfn_sf(mod_id);
      full_ul_report->has_sfn_sn = 1;
      //TODO:Set the number of UL measurement reports based on the types of measurements
      //configured for this UE and on the servCellIndex
      full_ul_report->n_cqi_meas = 1;
      Protocol__FlexUlCqi **ul_report;
      ul_report = malloc(sizeof(Protocol__FlexUlCqi *) * full_ul_report->n_cqi_meas);
      if (ul_report == NULL) {
        free(full_ul_report);
        goto error;
      }
      //Fill each UL report of the UE for each of the configured report types
      for (int j = 0; j < full_ul_report->n_cqi_meas; j++) {

        ul_report[j] = malloc(sizeof(Protocol__FlexUlCqi));
        if (ul_report[j] == NULL) {
          for (int k = 0; k < j; k++) {
            free(ul_report[k]);
          }
          free(ul_report);
          free(full_ul_report);
          goto error;
        }
        protocol__flex_ul_cqi__init(ul_report[j]);
        //TODO: Set the type of the UL report. As an example set it to SRS UL report
        ul_report[j]->type = PROTOCOL__FLEX_UL_CQI_TYPE__FLUCT_SRS;
        ul_report[j]->has_type = 1;
        //TODO:Set the number of SINR measurements based on the report type
        ul_report[j]->n_sinr = 0;
        uint32_t *sinr_meas;
        sinr_meas = (uint32_t *) malloc(sizeof(uint32_t) * ul_report[j]->n_sinr);
        if (sinr_meas == NULL) {
          for (int k = 0; k < j; k++) {
            free(ul_report[k]);
          }
          free(ul_report);
          free(full_ul_report);
          goto error;
        }
        //TODO:Set the SINR measurements for the specified type
        for (int k = 0; k < ul_report[j]->n_sinr; k++) {
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

        for (int j = 0; j < MAX_NUM_CCs; j++) {

          full_ul_report->pucch_dbm[j] = malloc(sizeof(Protocol__FlexPucchDbm));
          protocol__flex_pucch_dbm__init(full_ul_report->pucch_dbm[j]);
          full_ul_report->pucch_dbm[j]->has_serv_cell_index = 1;
          full_ul_report->pucch_dbm[j]->serv_cell_index = j;

          if (flexran_get_p0_pucch_dbm(mod_id, UE_id, j) != -1) {
            full_ul_report->pucch_dbm[j]->p0_pucch_dbm = flexran_get_p0_pucch_dbm(mod_id, UE_id, j);
            full_ul_report->pucch_dbm[j]->has_p0_pucch_dbm = 1;
          }
        }


      }
      //  Add full UL CQI report to the UE report
      ue_report[i]->ul_cqi_report = full_ul_report;
      ue_report[i]->flags |= PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_UL_CQI;

    }
    if (ue_flags & PROTOCOL__FLEX_UE_STATS_TYPE__FLUST_MAC_STATS) {

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
      if (mac_sdus == NULL) {
        free(macstats);
        goto error;
      }

      macstats->n_mac_sdus_dl = flexran_get_num_mac_sdu_tx(mod_id, UE_id, cc_id);
      for (int j = 0; j < macstats->n_mac_sdus_dl; j++) {
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
  return 0;

error:
  if (ue_report != NULL) {
    if (n_ue > 0) {
      for (int i = 0; i < n_ue; i++) {
        if (ue_report[i]->bsr != NULL) {
          free(ue_report[i]->bsr);
          ue_report[i]->bsr = NULL;
        }
        if (ue_report[i]->rlc_report != NULL) {
          for (int j = 0; j < ue_report[i]->n_rlc_report; j++) {
            if (ue_report[i]->rlc_report[j] != NULL) {
              free(ue_report[i]->rlc_report[j]);
              ue_report[i]->rlc_report[j] = NULL;
            }
          }
          free(ue_report[i]->rlc_report);
          ue_report[i]->rlc_report = NULL;
        }
        if (ue_report[i]->dl_cqi_report != NULL) {
          if (ue_report[i]->dl_cqi_report->csi_report != NULL) {
            for (int j = 0; j < ue_report[i]->dl_cqi_report->n_csi_report; j++) {
              if (ue_report[i]->dl_cqi_report->csi_report[j] != NULL) {
                if (ue_report[i]->dl_cqi_report->csi_report[j]->p10csi != NULL) {
                  free(ue_report[i]->dl_cqi_report->csi_report[j]->p10csi);
                  ue_report[i]->dl_cqi_report->csi_report[j]->p10csi = NULL;
                }
                if (ue_report[i]->dl_cqi_report->csi_report[j]->p11csi != NULL) {
                  if (ue_report[i]->dl_cqi_report->csi_report[j]->p11csi->wb_cqi != NULL) {
                    free(ue_report[i]->dl_cqi_report->csi_report[j]->p11csi->wb_cqi);
                    ue_report[i]->dl_cqi_report->csi_report[j]->p11csi->wb_cqi = NULL;
                  }
                  free(ue_report[i]->dl_cqi_report->csi_report[j]->p11csi);
                  ue_report[i]->dl_cqi_report->csi_report[j]->p11csi = NULL;
                }
                if (ue_report[i]->dl_cqi_report->csi_report[j]->p20csi != NULL) {
                  free(ue_report[i]->dl_cqi_report->csi_report[j]->p20csi);
                  ue_report[i]->dl_cqi_report->csi_report[j]->p20csi = NULL;
                }
                free(ue_report[i]->dl_cqi_report->csi_report[j]);
                ue_report[i]->dl_cqi_report->csi_report[j] = NULL;
              }
            }
            free(ue_report[i]->dl_cqi_report->csi_report);
            ue_report[i]->dl_cqi_report->csi_report = NULL;
          }
          free(ue_report[i]->dl_cqi_report);
          ue_report[i]->dl_cqi_report = NULL;
        }
        if (ue_report[i]->pbr != NULL) {
          if (ue_report[i]->pbr->paging_info != NULL) {
            for (int j = 0; j < ue_report[i]->pbr->n_paging_info; j++) {
              free(ue_report[i]->pbr->paging_info[j]);
              ue_report[i]->pbr->paging_info[j] = NULL;
            }
            free(ue_report[i]->pbr->paging_info);
            ue_report[i]->pbr->paging_info = NULL;
          }
          free(ue_report[i]->pbr);
          ue_report[i]->pbr = NULL;
        }
        if (ue_report[i]->ul_cqi_report != NULL) {
          if (ue_report[i]->ul_cqi_report->cqi_meas != NULL) {
            for (int j = 0; j < ue_report[i]->ul_cqi_report->n_cqi_meas; j++) {
              if (ue_report[i]->ul_cqi_report->cqi_meas[j] != NULL) {
                if (ue_report[i]->ul_cqi_report->cqi_meas[j]->sinr != NULL) {
                  free(ue_report[i]->ul_cqi_report->cqi_meas[j]->sinr);
                  ue_report[i]->ul_cqi_report->cqi_meas[j]->sinr = NULL;
                }
                free(ue_report[i]->ul_cqi_report->cqi_meas[j]);
                ue_report[i]->ul_cqi_report->cqi_meas[j] = NULL;
              }
            }
            free(ue_report[i]->ul_cqi_report->cqi_meas);
            ue_report[i]->ul_cqi_report->cqi_meas = NULL;
          }
          if (ue_report[i]->ul_cqi_report->pucch_dbm != NULL) {
            for (int j = 0; j < MAX_NUM_CCs; j++) {
              if (ue_report[i]->ul_cqi_report->pucch_dbm[j] != NULL) {
                free(ue_report[i]->ul_cqi_report->pucch_dbm[j]);
                ue_report[i]->ul_cqi_report->pucch_dbm[j] = NULL;
              }
            }
            free(ue_report[i]->ul_cqi_report->pucch_dbm);
            ue_report[i]->ul_cqi_report->pucch_dbm = NULL;
          }
          free(ue_report[i]->ul_cqi_report);
          ue_report[i]->ul_cqi_report = NULL;
        }
        if (ue_report[i]->mac_stats != NULL) {
          if (ue_report[i]->mac_stats->mac_sdus_dl != NULL) {
            for (int j = 0; j < ue_report[i]->mac_stats->n_mac_sdus_dl; j++) {
              if (ue_report[i]->mac_stats->mac_sdus_dl[j] != NULL) {
                free(ue_report[i]->mac_stats->mac_sdus_dl[j]);
                ue_report[i]->mac_stats->mac_sdus_dl[j] = NULL;
              }
            }
            free(ue_report[i]->mac_stats->mac_sdus_dl);
            ue_report[i]->mac_stats->mac_sdus_dl = NULL;
          }
          free(ue_report[i]->mac_stats);
          ue_report[i]->mac_stats = NULL;
        }
      }
    }
    free(ue_report);
  }
  return -1;
}

int flexran_agent_mac_stats_reply_cell(mid_t mod_id,
                                       Protocol__FlexCellStatsReport **cell_report,
                                       int      n_cc,
                                       uint32_t cc_flags) {
  for (int i = 0; i < n_cc; i++) {
    /* noise and interference report */
    if(cc_flags & PROTOCOL__FLEX_CELL_STATS_TYPE__FLCST_NOISE_INTERFERENCE) {
      // TODO: Fill in the actual noise and interference report for this cell
      Protocol__FlexNoiseInterferenceReport *ni_report;
      ni_report = malloc(sizeof(Protocol__FlexNoiseInterferenceReport));
      AssertFatal(ni_report, "cannot malloc() ni_report\n");
      protocol__flex_noise_interference_report__init(ni_report);
      ni_report->sfn_sf = flexran_get_sfn_sf(mod_id);
      ni_report->has_sfn_sf = 1;
      ni_report->rip = 0; //TODO: Received interference power in dbm
      ni_report->has_rip = 0;
      ni_report->tnp = 0; //TODO: Thermal noise power in dbm
      ni_report->has_tnp = 0;
      ni_report->p0_nominal_pucch = flexran_get_p0_nominal_pucch(mod_id, 0);
      ni_report->has_p0_nominal_pucch = 1;
      cell_report[i]->noise_inter_report = ni_report;
      cell_report[i]->flags |= PROTOCOL__FLEX_CELL_STATS_TYPE__FLCST_NOISE_INTERFERENCE;
    }
  }
  return 0;
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
      if (RC.mac && RC.mac[mod_id] && RC.mac[mod_id]->UE_info.eNB_UE_stats[UE_PCCID(mod_id,i)][i].harq_pid == 1) {
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
        RC.mac[mod_id]->UE_info.eNB_UE_stats[UE_PCCID(mod_id, UE_id)][UE_id].harq_pid = 0;
      dl_info[i]->has_harq_process_id = 1;
      /* Fill in the status of the HARQ process (2 TBs)*/
      dl_info[i]->n_harq_status = 2;
      dl_info[i]->harq_status = malloc(sizeof(uint32_t) * dl_info[i]->n_harq_status);
      for (j = 0; j < dl_info[i]->n_harq_status; j++) {
        dl_info[i]->harq_status[j] = RC.mac[mod_id]->UE_info.UE_sched_ctrl[UE_id].round[UE_PCCID(mod_id, UE_id)][j];
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
    if (sf_trigger_msg->dl_info != NULL) {
      for (i = 0; i < sf_trigger_msg->n_dl_info; i++) {
        if (sf_trigger_msg->dl_info[i] != NULL) {
          if (sf_trigger_msg->dl_info[i]->harq_status != NULL) {
            free(sf_trigger_msg->dl_info[i]->harq_status);
          }
          free(sf_trigger_msg->dl_info[i]);
        }
      }
      free(sf_trigger_msg->dl_info);
    }
    if (sf_trigger_msg->ul_info != NULL) {
      for (i = 0; i < sf_trigger_msg->n_ul_info; i++) {
        if (sf_trigger_msg->ul_info[i] != NULL) {
          if (sf_trigger_msg->ul_info[i]->ul_reception != NULL) {
            free(sf_trigger_msg->ul_info[i]->ul_reception);
          }
          free(sf_trigger_msg->ul_info[i]);
        }
      }
      free(sf_trigger_msg->ul_info);
    }
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
    free(header);
    goto error;
  }
  protocol__flex_dl_mac_config__init(dl_mac_config_msg);

  dl_mac_config_msg->header = header;
  dl_mac_config_msg->has_sfn_sf = 1;
  dl_mac_config_msg->sfn_sf = flexran_get_sfn_sf(mod_id);

  *msg = malloc(sizeof(Protocol__FlexranMessage));
  if(*msg == NULL) {
    free(dl_mac_config_msg);
    free(header);
    goto error;
  }
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
  if(*msg == NULL) {
    free(ul_mac_config_msg);
    goto error;
  }
  protocol__flexran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_UL_MAC_CONFIG_MSG;
  (*msg)->msg_dir =  PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->ul_mac_config_msg = ul_mac_config_msg;

  return 0;

 error:
  if(header){
      free(header);
      header = NULL;
  }
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

  struct lfds700_misc_prng_state scrng;
  i = posix_memalign((void **) &slice_config_array[mod_id],
                     LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES,
                     sizeof(struct lfds700_ringbuffer_element) * num_elements);
  AssertFatal(i == 0,
              "posix_memalign(): could not allocate aligned memory\n");
  lfds700_ringbuffer_init_valid_on_current_logical_core(
      &slice_config_ringbuffer_state[mod_id],
      slice_config_array[mod_id],
      num_elements,
      &scrng,
      NULL);

  struct lfds700_misc_prng_state sscrng;
  i = posix_memalign((void **) &store_slice_config_array[mod_id],
                     LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES,
                     sizeof(struct lfds700_ringbuffer_element) * num_elements);
  AssertFatal(i == 0,
              "posix_memalign(): could not allocate aligned memory\n");
  lfds700_ringbuffer_init_valid_on_current_logical_core(
      &store_slice_config_ringbuffer_state[mod_id],
      store_slice_config_array[mod_id],
      num_elements,
      &sscrng,
      NULL);

  struct lfds700_misc_prng_state uarng;
  i = posix_memalign((void **) &ue_assoc_array[mod_id],
                     LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES,
                     sizeof(struct lfds700_ringbuffer_element) * num_elements);
  AssertFatal(i == 0,
              "posix_memalign(): could not allocate aligned memory\n");
  lfds700_ringbuffer_init_valid_on_current_logical_core(
      &ue_assoc_ringbuffer_state[mod_id],
      ue_assoc_array[mod_id],
      num_elements,
      &uarng,
      NULL);

  for (i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    for (j = 0; j < 8; j++) {
      if (RC.mac && RC.mac[mod_id])
        RC.mac[mod_id]->UE_info.eNB_UE_stats[UE_PCCID(mod_id,i)][i].harq_pid = 0;
    }
  }

  SLIST_INIT(&flexran_handles[mod_id]);
}

/***********************************************
 * FlexRAN agent - technology mac API implementation
 ***********************************************/

void flexran_agent_send_sr_info(mid_t mod_id) {
  int size;
  Protocol__FlexranMessage *msg = NULL;
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
  Protocol__FlexranMessage *msg = NULL;
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

  xface->flexran_agent_send_sr_info = flexran_agent_send_sr_info;
  xface->flexran_agent_send_sf_trigger = flexran_agent_send_sf_trigger;
  xface->flexran_agent_get_pending_dl_mac_config = flexran_agent_get_pending_dl_mac_config;
  xface->flexran_agent_notify_tick = flexran_agent_timer_signal;

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

  conf->slice_config = malloc(sizeof(Protocol__FlexSliceConfig));
  if (conf->slice_config) {
    protocol__flex_slice_config__init(conf->slice_config);
    Protocol__FlexSliceConfig *sc = conf->slice_config;

    sc->dl = malloc(sizeof(Protocol__FlexSliceDlUlConfig));
    DevAssert(sc->dl);
    protocol__flex_slice_dl_ul_config__init(sc->dl);
    sc->dl->algorithm = flexran_get_dl_slice_algo(mod_id);
    sc->dl->has_algorithm = 1;
    sc->dl->n_slices = flexran_get_num_dl_slices(mod_id);
    if (sc->dl->n_slices > 0) {
      sc->dl->slices = calloc(sc->dl->n_slices, sizeof(Protocol__FlexSlice *));
      DevAssert(sc->dl->slices);
      for (int i = 0; i < sc->dl->n_slices; ++i) {
        sc->dl->slices[i] = malloc(sizeof(Protocol__FlexSlice));
        DevAssert(sc->dl->slices[i]);
        protocol__flex_slice__init(sc->dl->slices[i]);
        flexran_get_dl_slice(mod_id, i, sc->dl->slices[i], sc->dl->algorithm);
      }
    } else {
      sc->dl->scheduler = flexran_get_dl_scheduler_name(mod_id);
    }

    sc->ul = malloc(sizeof(Protocol__FlexSliceDlUlConfig));
    DevAssert(sc->ul);
    protocol__flex_slice_dl_ul_config__init(sc->ul);
    sc->ul->algorithm = flexran_get_ul_slice_algo(mod_id);
    sc->ul->has_algorithm = 1;
    sc->ul->n_slices = flexran_get_num_ul_slices(mod_id);
    if (sc->ul->n_slices > 0) {
      sc->ul->slices = calloc(sc->ul->n_slices, sizeof(Protocol__FlexSlice *));
      DevAssert(sc->ul->slices);
      for (int i = 0; i < sc->ul->n_slices; ++i) {
        sc->ul->slices[i] = malloc(sizeof(Protocol__FlexSlice));
        DevAssert(sc->ul->slices[i]);
        protocol__flex_slice__init(sc->ul->slices[i]);
        flexran_get_ul_slice(mod_id, i, sc->ul->slices[i], sc->ul->algorithm);
      }
    } else {
      sc->ul->scheduler = flexran_get_ul_scheduler_name(mod_id);
    }
  }
}

void flexran_agent_destroy_mac_slice_config(Protocol__FlexCellConfig *conf) {
  Protocol__FlexSliceConfig *sc = conf->slice_config;
  for (int i = 0; i < sc->dl->n_slices; ++i) {
    free(sc->dl->slices[i]);
    sc->dl->slices[i] = NULL;
    /* scheduler names are not freed: we assume we read them directly from the
     * underlying memory and do not dispose it */
  }
  free(sc->dl->slices);
  /* scheduler name is not freed */
  free(sc->dl);
  sc->dl = NULL;
  for (int i = 0; i < sc->ul->n_slices; ++i) {
    free(sc->ul->slices[i]);
    sc->ul->slices[i] = NULL;
    /* scheduler names are not freed */
  }
  free(sc->ul->slices);
  /* scheduler name is not freed */
  free(sc->ul);
  sc->ul = NULL;
  free(conf->slice_config);
  conf->slice_config = NULL;
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

  int dl_id = flexran_get_ue_dl_slice_id(mod_id, ue_id);
  ue_conf->dl_slice_id = dl_id;
  ue_conf->has_dl_slice_id = dl_id >= 0;
  int ul_id = flexran_get_ue_ul_slice_id(mod_id, ue_id);
  ue_conf->ul_slice_id = ul_id;
  ue_conf->has_ul_slice_id = ul_id >= 0;

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

  lfds700_ringbuffer_cleanup(&ringbuffer_state[mod_id], NULL );
  free(dl_mac_config_array[mod_id]);
  lfds700_ringbuffer_cleanup(&slice_config_ringbuffer_state[mod_id], NULL );
  free(slice_config_array[mod_id]);
  lfds700_ringbuffer_cleanup(&store_slice_config_ringbuffer_state[mod_id], NULL );
  free(store_slice_config_array[mod_id]);
  lfds700_ringbuffer_cleanup(&ue_assoc_ringbuffer_state[mod_id], NULL );
  free(ue_assoc_array[mod_id]);
  lfds700_misc_library_cleanup();

  AGENT_MAC_xface *xface = agent_mac_xface[mod_id];
  xface->flexran_agent_send_sr_info = NULL;
  xface->flexran_agent_send_sf_trigger = NULL;
  xface->flexran_agent_get_pending_dl_mac_config = NULL;
  xface->flexran_agent_notify_tick = NULL;

  xface->dl_scheduler_loaded_lib = NULL;
  xface->ul_scheduler_loaded_lib = NULL;
  free(xface);
  agent_mac_xface[mod_id] = NULL;

  return 0;
}

void helper_destroy_mac_slice_config(Protocol__FlexSliceConfig *slice_config) {
  if (slice_config->dl) {
    if (slice_config->dl->scheduler)
      free(slice_config->dl->scheduler);
    for (int i = 0; i < slice_config->dl->n_slices; i++)
      if (slice_config->dl->slices[i]->scheduler)
        free(slice_config->dl->slices[i]->scheduler);
  }
  if (slice_config->ul) {
    if (slice_config->ul->scheduler)
      free(slice_config->ul->scheduler);
    for (int i = 0; i < slice_config->ul->n_slices; i++)
      if (slice_config->ul->slices[i]->scheduler)
        free(slice_config->ul->slices[i]->scheduler);
  }

  Protocol__FlexCellConfig helper;
  helper.slice_config = slice_config;
  flexran_agent_destroy_mac_slice_config(&helper);
}

int check_scheduler(mid_t mod_id, char *s) {
  if (!s)
    return 1;
  if (dlsym(NULL, s))
    return 1;
  flexran_agent_so_handle_t *so = NULL;
  SLIST_FOREACH(so, &flexran_handles[mod_id], entries) {
    if (strcmp(so->name, s) == 0)
      return 1;
  }
  return 0;
}

void request_scheduler(mid_t mod_id, char *s, int xid) {
  LOG_W(FLEXRAN_AGENT,
        "unknown scheduler %s, requesting controller to send it\n",
        s);
  Protocol__FlexControlDelegationRequest *req = malloc(sizeof(Protocol__FlexControlDelegationRequest));
  DevAssert(req);
  protocol__flex_control_delegation_request__init(req);
  Protocol__FlexHeader *header = NULL;
  int rc = flexran_create_header(
      xid, PROTOCOL__FLEX_TYPE__FLPT_DELEGATE_REQUEST, &header);
  AssertFatal(rc == 0, "%s(): cannot create header\n", __func__);
  req->header = header;
  req->delegation_type = PROTOCOL__FLEX_CONTROL_DELEGATION_TYPE__FLCDT_MAC_DL_UE_SCHEDULER;
  req->name = strdup(s);

  Protocol__FlexranMessage *msg = malloc(sizeof(Protocol__FlexranMessage));
  DevAssert(msg);
  protocol__flexran_message__init(msg);
  msg->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_CONTROL_DEL_REQ_MSG;
  msg->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
  msg->control_del_req_msg = req;

  int size = 0;
  void *data = flexran_agent_pack_message(msg, &size);
  if (flexran_agent_msg_send(mod_id, FLEXRAN_AGENT_DEFAULT, data, size, 0) < 0)
    LOG_E(FLEXRAN_AGENT, "%s(): error while sending message\n", __func__);
}

Protocol__FlexranMessage *request_scheduler_timeout(
    mid_t mod_id,
    const Protocol__FlexranMessage *msg) {
  const Protocol__FlexEnbConfigReply *ecr = msg->enb_config_reply_msg;

  struct lfds700_misc_prng_state ls;
  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);

  Protocol__FlexranMessage *smsg = NULL;
  struct lfds700_ringbuffer_state *state = &store_slice_config_ringbuffer_state[mod_id];
  if (lfds700_ringbuffer_read(state, NULL, (void **) &smsg, &ls)) {
    AssertFatal(msg == smsg,
                "expected and returned enb_config_reply are not the same\n");
    AssertFatal(ecr->header->xid == smsg->enb_config_reply_msg->header->xid,
                "expected and returned enb_config_reply are not the same\n");
  } else {
    LOG_E(FLEXRAN_AGENT, "%s(): could not read config from ringbuffer\n", __func__);
  }
  /* call the helper so that scheduler names are correctly freed */
  helper_destroy_mac_slice_config(ecr->cell_config[0]->slice_config);
  ecr->cell_config[0]->slice_config = NULL;
  /* we should not call flexran_agent_destroy_enb_config_reply(smsg); since
   * upon timer removal, this is automatically done */
  LOG_W(FLEXRAN_AGENT,
        "remove stored slice config (xid %d) after timeout\n",
        ecr->header->xid);
  flexran_agent_destroy_timer(mod_id, ecr->header->xid);
  return NULL;
}

void prepare_update_slice_config(mid_t mod_id,
                                 Protocol__FlexSliceConfig **slice_config,
                                 int request_objects) {
  if (!*slice_config) return;
  /* just use the memory and set to NULL in original */
  Protocol__FlexSliceConfig *sc = *slice_config;
  *slice_config = NULL;

  struct lfds700_misc_prng_state ls;
  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);
  enum lfds700_misc_flag overwrite_occurred_flag;

  int put_on_hold = 0;
  if (request_objects
      && sc->dl->scheduler
      && !check_scheduler(mod_id, sc->dl->scheduler)) {
    request_scheduler(mod_id, sc->dl->scheduler, xid++);
    put_on_hold = 1;
  }
  for (int i = 0; request_objects && i < sc->dl->n_slices; ++i) {
    Protocol__FlexSlice *sl = sc->dl->slices[i];
    if (sl->scheduler && !check_scheduler(mod_id, sl->scheduler)) {
      request_scheduler(mod_id, sl->scheduler, xid++);
      put_on_hold = 1;
    }
  }
  if (put_on_hold) {
    Protocol__FlexEnbConfigReply *reply = malloc(sizeof(Protocol__FlexEnbConfigReply));
    DevAssert(reply);
    protocol__flex_enb_config_reply__init(reply);
    /* use the last used xid so we can correlate answer and waiting config */
    int rc = flexran_create_header(
        xid - 1, PROTOCOL__FLEX_TYPE__FLPT_GET_ENB_CONFIG_REPLY, &reply->header);
    AssertFatal(rc == 0, "%s(): cannot create header\n", __func__);
    reply->n_cell_config = 1;
    reply->cell_config = malloc(sizeof(Protocol__FlexCellConfig *));
    DevAssert(reply->cell_config);
    reply->cell_config[0] = malloc(sizeof(Protocol__FlexCellConfig));
    DevAssert(reply->cell_config[0]);
    protocol__flex_cell_config__init(reply->cell_config[0]);
    reply->cell_config[0]->slice_config = sc;

    Protocol__FlexranMessage *msg = malloc(sizeof(Protocol__FlexranMessage));
    DevAssert(msg);
    protocol__flexran_message__init(msg);
    msg->msg_case = PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REPLY_MSG;
    msg->msg_dir = PROTOCOL__FLEXRAN_DIRECTION__INITIATING_MESSAGE;
    msg->enb_config_reply_msg = reply;

    Protocol__FlexEnbConfigReply *o;
    lfds700_ringbuffer_write(&store_slice_config_ringbuffer_state[mod_id],
                             NULL,
                             (void *)msg,
                             &overwrite_occurred_flag,
                             NULL,
                             (void **) &o,
                             &ls);
    AssertFatal(overwrite_occurred_flag == LFDS700_MISC_FLAG_LOWERED,
                "unhandled: stored slice config for controller has been overwritten\n");

    flexran_agent_create_timer(mod_id,
                               3000,
                               FLEXRAN_AGENT_TIMER_TYPE_PERIODIC,
                               xid - 1,
                               request_scheduler_timeout,
                               msg); /* need reply for xid */
    return;
  }

  Protocol__FlexSliceConfig *overwritten_sc;
  lfds700_ringbuffer_write(&slice_config_ringbuffer_state[mod_id],
                           NULL,
                           (void *)sc,
                           &overwrite_occurred_flag,
                           NULL,
                           (void **)&overwritten_sc,
                           &ls);

  if (overwrite_occurred_flag == LFDS700_MISC_FLAG_RAISED) {
    helper_destroy_mac_slice_config(overwritten_sc);
    LOG_E(FLEXRAN_AGENT, "lost slice config: too many reconfigurations at once\n");
  }
}

void prepare_ue_slice_assoc_update(mid_t mod_id, Protocol__FlexUeConfig **ue_config) {
  struct lfds700_misc_prng_state ls;
  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);
  enum lfds700_misc_flag overwrite_occurred_flag;

  Protocol__FlexUeConfig *overwritten_ue_config;
  lfds700_ringbuffer_write(&ue_assoc_ringbuffer_state[mod_id],
                           NULL,
                           (void *) *ue_config,
                           &overwrite_occurred_flag,
                           NULL,
                           (void **)&overwritten_ue_config,
                           &ls);

  if (overwrite_occurred_flag == LFDS700_MISC_FLAG_RAISED) {
    free(overwritten_ue_config);
    LOG_E(FLEXRAN_AGENT, "lost UE-slice association: too many UE-slice associations at once\n");
  }
}

void flexran_agent_slice_update(mid_t mod_id) {
  struct lfds700_misc_prng_state ls;
  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);

  Protocol__FlexSliceConfig *sc = NULL;
  struct lfds700_ringbuffer_state *state = &slice_config_ringbuffer_state[mod_id];
  if (lfds700_ringbuffer_read(state, NULL, (void **) &sc, &ls) != 0) {
    apply_update_dl_slice_config(mod_id, sc->dl);
    apply_update_ul_slice_config(mod_id, sc->ul);
    helper_destroy_mac_slice_config(sc);

    flexran_agent_so_handle_t *so = NULL;
    flexran_agent_so_handle_t *prev = NULL; // the previous to current element, if we delete in order to go back
    //SLIST_FOREACH(so, &flexran_handles[mod_id], entries)
    for(so = flexran_handles[mod_id].slh_first; so;) {
      char *name = so->name;
      int in_use = strcmp(flexran_get_dl_scheduler_name(mod_id), name) == 0
                   || strcmp(flexran_get_ul_scheduler_name(mod_id), name) == 0;
      Protocol__FlexSliceAlgorithm dl_algo = flexran_get_dl_slice_algo(mod_id);
      if (!in_use && dl_algo != PROTOCOL__FLEX_SLICE_ALGORITHM__None) {
        int n = flexran_get_num_dl_slices(mod_id);
        for (int i = 0; i < n; ++i) {
          Protocol__FlexSlice s;
          /* NONE so it won't do any allocations */
          flexran_get_dl_slice(mod_id, i, &s, PROTOCOL__FLEX_SLICE_ALGORITHM__None);
          if (strcmp(s.scheduler, name) == 0) {
            in_use = 1;
            break;
          }
        }
      }
      Protocol__FlexSliceAlgorithm ul_algo = flexran_get_ul_slice_algo(mod_id);
      if (!in_use && ul_algo != PROTOCOL__FLEX_SLICE_ALGORITHM__None) {
        int n = flexran_get_num_ul_slices(mod_id);
        for (int i = 0; i < n; ++i) {
          Protocol__FlexSlice s;
          /* NONE so it won't do any allocations */
          flexran_get_ul_slice(mod_id, i, &s, PROTOCOL__FLEX_SLICE_ALGORITHM__None);
          if (strcmp(s.scheduler, name) == 0) {
            in_use = 1;
            break;
          }
        }
      }
      if (!in_use) {
        char s[512];
        int rc = flexran_agent_map_name_to_delegated_object(mod_id, so->name, s, 512);
        LOG_W(FLEXRAN_AGENT,
              "removing %s (library handle %p) since it is not used in the user "
              "plane anymore\n",
              s,
              so->dl_handle);
        dlclose(so->dl_handle);
        if (rc < 0) {
          LOG_E(FLEXRAN_AGENT, "cannot map name %s\n", so->name);
        } else {
          int rc = remove(s);
          if (rc < 0)
            LOG_E(FLEXRAN_AGENT, "cannot remove file %s: %s\n", s, strerror(errno));
        }
        free (so->name);
        if (!prev) { //it's the head, start over
          SLIST_REMOVE_HEAD(&flexran_handles[mod_id], entries);
          so = flexran_handles[mod_id].slh_first;
        } else {
          SLIST_REMOVE(&flexran_handles[mod_id], so, flexran_agent_so_handle_s, entries);
          so = prev->entries.sle_next;
        }
      } else {
        prev = so;
        so = so->entries.sle_next;
      }
    }
  }

  Protocol__FlexUeConfig *ue_config = NULL;
  state = &ue_assoc_ringbuffer_state[mod_id];
  if (lfds700_ringbuffer_read(state, NULL, (void **) &ue_config, &ls) != 0) {
    apply_ue_slice_assoc_update(mod_id, ue_config);
    free(ue_config);
  }
}

void flexran_agent_mac_inform_delegation(mid_t mod_id,
                                         Protocol__FlexControlDelegation *cdm) {
  LOG_W(FLEXRAN_AGENT,
        "received FlexControlDelegation message for object '%s' xid %d\n",
        cdm->name,
        cdm->header->xid);
  if (cdm->header->xid < xid - 1) {
    LOG_I(FLEXRAN_AGENT,
          "waiting for %d more messages (up to xid %d)\n",
          xid - 1 - cdm->header->xid,
          xid - 1);
    return;
  }
  /* should receive up to xid - 1, otherwise it means there was no request */
  AssertFatal(xid > cdm->header->xid,
              "received control delegation with xid %d that we never requested "
              "(last request %d)\n",
              cdm->header->xid,
              xid - 1);

  /* Load the library so the user plane can search it */
  char s[512];
  int rc = flexran_agent_map_name_to_delegated_object(mod_id, cdm->name, s, 512);
  if (rc < 0) {
    LOG_E(FLEXRAN_AGENT, "cannot map name %s\n", cdm->name);
    return;
  }
  /* TODO where to unload/save handle?? */
  void *h = dlopen(s, RTLD_NOW);
  if (!h) {
    LOG_E(FLEXRAN_AGENT, "dlopen(): %s\n", dlerror());
    return;
  }
  LOG_I(FLEXRAN_AGENT, "library handle %p\n", h);

  struct lfds700_misc_prng_state ls;
  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);

  Protocol__FlexranMessage *msg = NULL;
  struct lfds700_ringbuffer_state *state = &store_slice_config_ringbuffer_state[mod_id];
  if (lfds700_ringbuffer_read(state, NULL, (void **) &msg, &ls) == 0) {
    LOG_E(FLEXRAN_AGENT, "%s(): could not read config from ringbuffer\n", __func__);
    return;
  }
  AssertFatal(cdm->header->xid == msg->enb_config_reply_msg->header->xid,
              "expected and retrieved xid of stored slice configuration does "
              "not match (expected %d, retrieved %d)\n",
              cdm->header->xid,
              msg->enb_config_reply_msg->header->xid);

  /* Since we recover, stop the timeout */
  rc = flexran_agent_destroy_timer(mod_id, cdm->header->xid);

  prepare_update_slice_config(
      mod_id,
      &msg->enb_config_reply_msg->cell_config[0]->slice_config,
      0 /* don't do a request */);
  /* we should not call flexran_agent_destroy_enb_config_reply(smsg); since
   * upon timer removal, this is automatically done.
   * prepare_update_slice_config() takes ownership of the slice config, it is
   * freed inside */

  flexran_agent_so_handle_t *so = malloc(sizeof(flexran_agent_so_handle_t));
  DevAssert(so);
  so->name = strdup(cdm->name);
  DevAssert(so->name);
  so->dl_handle = h;
  SLIST_INSERT_HEAD(&flexran_handles[mod_id], so, entries);
}

void flexran_agent_mac_fill_loaded_mac_objects(
    mid_t mod_id,
    Protocol__FlexEnbConfigReply *reply) {
  int n = 0;
  flexran_agent_so_handle_t *so = NULL;
  SLIST_FOREACH(so, &flexran_handles[mod_id], entries)
    ++n;
  reply->n_loadedmacobjects = n;
  reply->loadedmacobjects = calloc(n, sizeof(char *));
  if (!reply->loadedmacobjects) {
    reply->n_loadedmacobjects = 0;
    return;
  }
  n = 0;
  SLIST_FOREACH(so, &flexran_handles[mod_id], entries)
    reply->loadedmacobjects[n++] = so->name;
}
