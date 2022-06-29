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

#include "LAYER2/nr_pdcp/nr_pdcp_entity.h"
#include "RRC/NR/nr_rrc_defs.h"
#include "common/utils/LOG/log.h"
//#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

#include "common/ran_context.h"
#include "executables/softmodem-common.h"

extern RAN_CONTEXT_t RC;


#define MACSTATSSTRLEN 65536

void *nrmac_stats_thread(void *arg) {

  gNB_MAC_INST *gNB = (gNB_MAC_INST *)arg;

  char output[MACSTATSSTRLEN] = {0};
  const char *end = output + MACSTATSSTRLEN;
  FILE *file = fopen("nrMAC_stats.log","w");
  AssertFatal(file!=NULL,"Cannot open nrMAC_stats.log, error %s\n",strerror(errno));

  while (oai_exit == 0) {
    char *p = output;
    p += dump_mac_stats(gNB, p, end - p, false);
    p += snprintf(p, end - p, "\n");
    p += print_meas_log(&gNB->eNB_scheduler, "DL & UL scheduling timing", NULL, NULL, p, end - p);
    p += print_meas_log(&gNB->schedule_dlsch, "dlsch scheduler", NULL, NULL, p, end - p);
    p += print_meas_log(&gNB->rlc_data_req, "rlc_data_req", NULL, NULL, p, end - p);
    p += print_meas_log(&gNB->rlc_status_ind, "rlc_status_ind", NULL, NULL, p, end - p);
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
  int num = 1;
  const char *begin = output;
  const char *end = output + strlen;
 
  pthread_mutex_lock(&gNB->UE_info.mutex);
  UE_iterator(gNB->UE_info.list, UE) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
    NR_mac_stats_t *stats = &UE->mac_stats;
    const int avg_rsrp = stats->num_rsrp_meas > 0 ? stats->cumul_rsrp / stats->num_rsrp_meas : 0;


    output += snprintf(output,
                       end - output,
                       "UE RNTI %04x (%d) PH %d dB PCMAX %d dBm, average RSRP %d (%d meas)\n",
                       UE->rnti,
                       num++,
                       sched_ctrl->ph,
                       sched_ctrl->pcmax,
                       avg_rsrp,
                       stats->num_rsrp_meas);
    output += snprintf(output,
                       end - output,
                       "UE %04x: CQI %d, RI %d, PMI (%d,%d)\n",
                       UE->rnti,
                       UE->UE_sched_ctrl.CSI_report.cri_ri_li_pmi_cqi_report.wb_cqi_1tb,
                       UE->UE_sched_ctrl.CSI_report.cri_ri_li_pmi_cqi_report.ri+1,
                       UE->UE_sched_ctrl.CSI_report.cri_ri_li_pmi_cqi_report.pmi_x1,
                       UE->UE_sched_ctrl.CSI_report.cri_ri_li_pmi_cqi_report.pmi_x2);

    output += snprintf(output,
                       end - output,
                       "UE %04x: dlsch_rounds %"PRIu64"/%"PRIu64"/%"PRIu64"/%"PRIu64", dlsch_errors %"PRIu64", pucch0_DTX %d, BLER %.5f MCS %d\n",
                       UE->rnti,
                       stats->dl.rounds[0], stats->dl.rounds[1],
                       stats->dl.rounds[2], stats->dl.rounds[3],
                       stats->dl.errors,
                       stats->pucch0_DTX,
                       sched_ctrl->dl_bler_stats.bler,
                       sched_ctrl->dl_bler_stats.mcs);
    if (reset_rsrp) {
      stats->num_rsrp_meas = 0;
      stats->cumul_rsrp = 0;
    }
    output += snprintf(output,
                       end - output,
                       "UE %04x: dlsch_total_bytes %"PRIu64"\n",
                       UE->rnti,
                       stats->dl.total_bytes);
    output += snprintf(output,
                       end - output,
                       "UE %04x: ulsch_rounds %"PRIu64"/%"PRIu64"/%"PRIu64"/%"PRIu64", ulsch_DTX %d, ulsch_errors %"PRIu64", BLER %.5f MCS %d\n",
                       UE->rnti,
                       stats->ul.rounds[0], stats->ul.rounds[1],
                       stats->ul.rounds[2], stats->ul.rounds[3],
                       stats->ulsch_DTX,
                       stats->ul.errors,
                       sched_ctrl->ul_bler_stats.bler,
                       sched_ctrl->ul_bler_stats.mcs);
    output += snprintf(output,
                       end - output,
                      "UE %04x: ulsch_total_bytes_scheduled %"PRIu64", ulsch_total_bytes_received %"PRIu64"\n",
                      UE->rnti,
                      stats->ulsch_total_bytes_scheduled, stats->ul.total_bytes);
    for (int lc_id = 0; lc_id < 63; lc_id++) {
      if (stats->dl.lc_bytes[lc_id] > 0)
        output += snprintf(output,
                           end - output,
                           "UE %04x: LCID %d: %"PRIu64" bytes TX\n",
                           UE->rnti,
                           lc_id,
                           stats->dl.lc_bytes[lc_id]);
      if (stats->ul.lc_bytes[lc_id] > 0)
        output += snprintf(output,
                           end - output,
                           "UE %04x: LCID %d: %"PRIu64" bytes RX\n",
                           UE->rnti,
                           lc_id,
                           stats->ul.lc_bytes[lc_id]);
    }
  }
  pthread_mutex_unlock(&gNB->UE_info.mutex);
  return output - begin;
}


void mac_top_init_gNB(void)
{
  module_id_t     i;
  gNB_MAC_INST    *nrmac;

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

      RC.nrmac[i]->first_MIB = true;

      pthread_mutex_init(&RC.nrmac[i]->UE_info.mutex, NULL);

      if (get_softmodem_params()->phy_test) {
        RC.nrmac[i]->pre_processor_dl = nr_preprocessor_phytest;
        RC.nrmac[i]->pre_processor_ul = nr_ul_preprocessor_phytest;
      } else {
        RC.nrmac[i]->pre_processor_dl = nr_init_fr1_dlsch_preprocessor(i, 0);
        RC.nrmac[i]->pre_processor_ul = nr_init_fr1_ulsch_preprocessor(i, 0);
      }
      if (!IS_SOFTMODEM_NOSTATS_BIT)
        pthread_create(&RC.nrmac[i]->stats_thread, NULL, nrmac_stats_thread, (void*)RC.nrmac[i]);
    }//END for (i = 0; i < RC.nb_nr_macrlc_inst; i++)

    AssertFatal(rlc_module_init(1) == 0,"Could not initialize RLC layer\n");

    // These should be out of here later
    pdcp_layer_init();

    if(IS_SOFTMODEM_NOS1 && get_softmodem_params()->phy_test)
      nr_DRB_preconfiguration(0x1234);

    rrc_init_nr_global_param();


  } else {
    RC.nrmac = NULL;
  }

  // Initialize Linked-List for Active UEs
  for (i = 0; i < RC.nb_nr_macrlc_inst; i++) {

    nrmac = RC.nrmac[i];
    nrmac->if_inst = NR_IF_Module_init(i);
    memset(&nrmac->UE_info, 0, sizeof(nrmac->UE_info));
  }

  srand48(0);
}
