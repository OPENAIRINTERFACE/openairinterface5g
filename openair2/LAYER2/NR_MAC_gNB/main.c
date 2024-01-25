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

/*! \file main.c
 * \brief top init of Layer 2
 * \author  Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
 * @ingroup _mac

 */

#include "NR_MAC_gNB/mac_proto.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "assertions.h"
#include "nr_pdcp/nr_pdcp_oai_api.h"

#include "common/utils/LOG/log.h"
#include "nr_rlc/nr_rlc_oai_api.h"
#include "openair2/F1AP/f1ap_ids.h"

#include "common/ran_context.h"
#include "executables/softmodem-common.h"

extern RAN_CONTEXT_t RC;


#define MACSTATSSTRLEN 16000

void *nrmac_stats_thread(void *arg) {

  gNB_MAC_INST *gNB = (gNB_MAC_INST *)arg;

  char output[MACSTATSSTRLEN] = {0};
  const char *end = output + MACSTATSSTRLEN;
  FILE *file = fopen("nrMAC_stats.log","w");
  AssertFatal(file!=NULL,"Cannot open nrMAC_stats.log, error %s\n",strerror(errno));

  while (oai_exit == 0) {
    char *p = output;
    NR_SCHED_LOCK(&gNB->sched_lock);
    p += dump_mac_stats(gNB, p, end - p, false);
    NR_SCHED_UNLOCK(&gNB->sched_lock);
    p += snprintf(p, end - p, "\n");
    p += print_meas_log(&gNB->eNB_scheduler, "DL & UL scheduling timing", NULL, NULL, p, end - p);
    p += print_meas_log(&gNB->schedule_dlsch, "dlsch scheduler", NULL, NULL, p, end - p);
    p += print_meas_log(&gNB->rlc_data_req, "rlc_data_req", NULL, NULL, p, end - p);
    p += print_meas_log(&gNB->rlc_status_ind, "rlc_status_ind", NULL, NULL, p, end - p);
    p += print_meas_log(&gNB->nr_srs_ri_computation_timer, "UL-RI computation time", NULL, NULL, p, end - p);
    p += print_meas_log(&gNB->nr_srs_tpmi_computation_timer, "UL-TPMI computation time", NULL, NULL, p, end - p);
    fwrite(output, p - output, 1, file);
    fflush(file);
    sleep(1);
    fseek(file,0,SEEK_SET);
  }
  fclose(file);
  return NULL;
}

void clear_mac_stats(gNB_MAC_INST *gNB) {
  UE_iterator(gNB->UE_info.list, UE) {
    memset(&UE->mac_stats,0,sizeof(UE->mac_stats));
  }
}

size_t dump_mac_stats(gNB_MAC_INST *gNB, char *output, size_t strlen, bool reset_rsrp)
{
  const char *begin = output;
  const char *end = output + strlen;

  /* this function is called from gNB_dlsch_ulsch_scheduler(), so assumes the
   * scheduler to be locked*/
  NR_SCHED_ENSURE_LOCKED(&gNB->sched_lock);

  NR_SCHED_LOCK(&gNB->UE_info.mutex);
  UE_iterator(gNB->UE_info.list, UE) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
    NR_mac_stats_t *stats = &UE->mac_stats;
    const int avg_rsrp = stats->num_rsrp_meas > 0 ? stats->cumul_rsrp / stats->num_rsrp_meas : 0;

    output += snprintf(output, end - output, "UE RNTI %04x CU-UE-ID ", UE->rnti);
    if (du_exists_f1_ue_data(UE->rnti)) {
      f1_ue_data_t ued = du_get_f1_ue_data(UE->rnti);
      output += snprintf(output, end - output, "%d", ued.secondary_ue);
    } else {
      output += snprintf(output, end-output, "(none)");
    }

    bool in_sync = !sched_ctrl->ul_failure;
    output += snprintf(output,
                       end - output,
                       " %s PH %d dB PCMAX %d dBm, average RSRP %d (%d meas)\n",
                       in_sync ? "in-sync" : "out-of-sync",
                       sched_ctrl->ph,
                       sched_ctrl->pcmax,
                       avg_rsrp,
                       stats->num_rsrp_meas);

    if(sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.print_report)
      output += snprintf(output,
                         end - output,
                         "UE %04x: CQI %d, RI %d, PMI (%d,%d)\n",
                         UE->rnti,
                         sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.wb_cqi_1tb,
                         sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.ri+1,
                         sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.pmi_x1,
                         sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.pmi_x2);

    if (stats->srs_stats[0] != '\0') {
      output += snprintf(output, end - output, "UE %04x: %s\n", UE->rnti, stats->srs_stats);
    }

    output += snprintf(output,
                       end - output,
                       "UE %04x: dlsch_rounds ", UE->rnti);
    output += snprintf(output, end - output, "%"PRIu64, stats->dl.rounds[0]);
    for (int i = 1; i < gNB->dl_bler.harq_round_max; i++)
      output += snprintf(output, end - output, "/%"PRIu64, stats->dl.rounds[i]);

    output += snprintf(output,
                       end - output,
                       ", dlsch_errors %"PRIu64", pucch0_DTX %d, BLER %.5f MCS (%d) %d\n",
                       stats->dl.errors,
                       stats->pucch0_DTX,
                       sched_ctrl->dl_bler_stats.bler,
                       UE->current_DL_BWP.mcsTableIdx,
                       sched_ctrl->dl_bler_stats.mcs);
    if (reset_rsrp) {
      stats->num_rsrp_meas = 0;
      stats->cumul_rsrp = 0;
    }
    output += snprintf(output,
                       end - output,
                       "UE %04x: ulsch_rounds ", UE->rnti);
    output += snprintf(output, end - output, "%"PRIu64, stats->ul.rounds[0]);
    for (int i = 1; i < gNB->ul_bler.harq_round_max; i++)
      output += snprintf(output, end - output, "/%"PRIu64, stats->ul.rounds[i]);

    output += snprintf(output,
                       end - output,
                       ", ulsch_errors %"PRIu64", ulsch_DTX %d, BLER %.5f MCS (%d) %d\n",
                       stats->ul.errors,
                       stats->ulsch_DTX,
                       sched_ctrl->ul_bler_stats.bler,
                       UE->current_UL_BWP.mcs_table,
                       sched_ctrl->ul_bler_stats.mcs);

    output += snprintf(output,
                       end - output,
                       "UE %04x: MAC:    TX %14"PRIu64" RX %14"PRIu64" bytes\n",
                       UE->rnti, stats->dl.total_bytes, stats->ul.total_bytes);

    for (int i = 0; i < sched_ctrl->dl_lc_num; i++) {
      int lc_id = sched_ctrl->dl_lc_ids[i];
      output += snprintf(output,
                         end - output,
                         "UE %04x: LCID %d: TX %14"PRIu64" RX %14"PRIu64" bytes\n",
                         UE->rnti,
                         lc_id,
                         stats->dl.lc_bytes[lc_id],
                         stats->ul.lc_bytes[lc_id]);
    }
  }
  NR_SCHED_UNLOCK(&gNB->UE_info.mutex);
  return output - begin;
}

static void mac_rrc_init(gNB_MAC_INST *mac, ngran_node_t node_type)
{
  switch (node_type) {
    case ngran_gNB_CU:
      AssertFatal(1 == 0, "nothing to do for CU\n");
      break;
    case ngran_gNB_DU:
      mac_rrc_ul_f1ap_init(&mac->mac_rrc);
      break;
    case ngran_gNB:
      mac_rrc_ul_direct_init(&mac->mac_rrc);
      break;
    default:
      AssertFatal(0 == 1, "Unknown node type %d\n", node_type);
      break;
  }
}

void mac_top_init_gNB(ngran_node_t node_type,
                      NR_ServingCellConfigCommon_t *scc,
                      NR_ServingCellConfig_t *scd,
                      const nr_mac_config_t *config)
{
  module_id_t     i;
  gNB_MAC_INST    *nrmac;

  AssertFatal(RC.nb_nr_macrlc_inst == 1, "what is the point of calling %s() if you don't need exactly one MAC?\n", __func__);

  LOG_I(MAC, "[MAIN] Init function start:nb_nr_macrlc_inst=%d\n",RC.nb_nr_macrlc_inst);

  if (RC.nb_nr_macrlc_inst > 0) {

    RC.nrmac = (gNB_MAC_INST **) malloc16(RC.nb_nr_macrlc_inst *sizeof(gNB_MAC_INST *));
    
    AssertFatal(RC.nrmac != NULL,"can't ALLOCATE %zu Bytes for %d gNB_MAC_INST with size %zu \n",
                RC.nb_nr_macrlc_inst * sizeof(gNB_MAC_INST *),
                RC.nb_nr_macrlc_inst, sizeof(gNB_MAC_INST));

    for (i = 0; i < RC.nb_nr_macrlc_inst; i++) {

      RC.nrmac[i] = (gNB_MAC_INST *) malloc16(sizeof(gNB_MAC_INST));
      
      AssertFatal(RC.nrmac != NULL,"can't ALLOCATE %zu Bytes for %d gNB_MAC_INST with size %zu \n",
                  RC.nb_nr_macrlc_inst * sizeof(gNB_MAC_INST *),
                  RC.nb_nr_macrlc_inst, sizeof(gNB_MAC_INST));
      
      LOG_D(MAC,"[MAIN] ALLOCATE %zu Bytes for %d gNB_MAC_INST @ %p\n",sizeof(gNB_MAC_INST), RC.nb_nr_macrlc_inst, RC.mac);
      
      bzero(RC.nrmac[i], sizeof(gNB_MAC_INST));
      
      RC.nrmac[i]->Mod_id = i;

      RC.nrmac[i]->tag = (NR_TAG_t*)malloc(sizeof(NR_TAG_t));
      memset((void*)RC.nrmac[i]->tag,0,sizeof(NR_TAG_t));
        
      RC.nrmac[i]->ul_handle = 0;

      RC.nrmac[i]->common_channels[0].ServingCellConfigCommon = scc;
      RC.nrmac[i]->radio_config = *config;

      RC.nrmac[i]->common_channels[0].pre_ServingCellConfig = scd;

      RC.nrmac[i]->first_MIB = true;
      RC.nrmac[i]->common_channels[0].mib = get_new_MIB_NR(scc);

      RC.nrmac[i]->cset0_bwp_start = 0;
      RC.nrmac[i]->cset0_bwp_size = 0;

      pthread_mutex_init(&RC.nrmac[i]->sched_lock, NULL);

      pthread_mutex_init(&RC.nrmac[i]->UE_info.mutex, NULL);
      uid_linear_allocator_init(&RC.nrmac[i]->UE_info.uid_allocator);

      if (get_softmodem_params()->phy_test) {
        RC.nrmac[i]->pre_processor_dl = nr_preprocessor_phytest;
        RC.nrmac[i]->pre_processor_ul = nr_ul_preprocessor_phytest;
      } else {
        RC.nrmac[i]->pre_processor_dl = nr_init_fr1_dlsch_preprocessor(0);
        RC.nrmac[i]->pre_processor_ul = nr_init_fr1_ulsch_preprocessor(0);
      }
      if (!IS_SOFTMODEM_NOSTATS_BIT)
         threadCreate(&RC.nrmac[i]->stats_thread, nrmac_stats_thread, (void*)RC.nrmac[i], "MAC_STATS", -1,     sched_get_priority_min(SCHED_OAI)+1 );
      mac_rrc_init(RC.nrmac[i], node_type);
    }//END for (i = 0; i < RC.nb_nr_macrlc_inst; i++)

    AssertFatal(rlc_module_init(1) == 0,"Could not initialize RLC layer\n");

    // These should be out of here later
    if (get_softmodem_params()->usim_test == 0 ) nr_pdcp_layer_init(false);

    if(IS_SOFTMODEM_NOS1 && get_softmodem_params()->phy_test) {
      // get default noS1 configuration
      NR_RadioBearerConfig_t *rbconfig = NULL;
      NR_RLC_BearerConfig_t *rlc_rbconfig = NULL;
      fill_nr_noS1_bearer_config(&rbconfig, &rlc_rbconfig);

      /* Note! previously, in nr_DRB_preconfiguration(), we passed ENB_FLAG_NO
       * if ENB_NAS_USE_TUN was *not* set. It seems to me that we could not set
       * this flag anywhere in the code, hence we would always configure PDCP
       * with ENB_FLAG_NO in nr_DRB_preconfiguration(). This makes sense for
       * noS1, because the result of passing ENB_FLAG_NO to PDCP is that PDCP
       * will output the packets at a local interface, which is in line with
       * the noS1 mode.  Hence, below, we simply hardcode ENB_FLAG_NO */
      // setup PDCP, RLC
      nr_pdcp_add_drbs(ENB_FLAG_NO, 0x1234, rbconfig->drb_ToAddModList, 0, NULL, NULL);
      nr_rlc_add_drb(0x1234, rbconfig->drb_ToAddModList->list.array[0]->drb_Identity, rlc_rbconfig);

      // free memory
      free_nr_noS1_bearer_config(&rbconfig, &rlc_rbconfig);
    }

  } else {
    RC.nrmac = NULL;
  }

  // Initialize Linked-List for Active UEs
  for (i = 0; i < RC.nb_nr_macrlc_inst; i++) {
    nrmac = RC.nrmac[i];
    nrmac->if_inst = NR_IF_Module_init(i);
    memset(&nrmac->UE_info, 0, sizeof(nrmac->UE_info));
  }

  du_init_f1_ue_data();

  srand48(0);

  // triggers also PYH initialization in case we have L1 via FAPI
  nr_mac_config_scc(RC.nrmac[0], scc, config);
}

void nr_mac_send_f1_setup_req(void)
{
  gNB_MAC_INST *mac = RC.nrmac[0];
  DevAssert(mac);
  mac->mac_rrc.f1_setup_request(mac->f1_config.setup_req);
}
