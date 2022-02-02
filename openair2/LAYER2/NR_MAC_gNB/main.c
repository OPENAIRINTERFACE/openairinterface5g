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


#define MACSTATSSTRLEN 16384

void *nrmac_stats_thread(void *arg) {

  gNB_MAC_INST *gNB = (gNB_MAC_INST *)arg;

  char output[MACSTATSSTRLEN];
  memset(output,0,MACSTATSSTRLEN);
  FILE *fd=fopen("nrMAC_stats.log","w");
  AssertFatal(fd!=NULL,"Cannot open nrMAC_stats.log, error %s\n",strerror(errno));

  while (oai_exit == 0) {
     dump_mac_stats(gNB,output,MACSTATSSTRLEN,false);
     fprintf(fd,"%s\n",output);
     fflush(fd);
     usleep(200000);
     fseek(fd,0,SEEK_SET);
  }
  fclose(fd);
  return NULL;
}

void clear_mac_stats(gNB_MAC_INST *gNB) {
  memset((void*)gNB->UE_info.mac_stats,0,MAX_MOBILES_PER_GNB*sizeof(NR_mac_stats_t));
}

void dump_mac_stats(gNB_MAC_INST *gNB, char *output, int strlen, bool reset_rsrp)
{
  NR_UE_info_t *UE_info = &gNB->UE_info;
  int num = 1;
 
  int stroff=0;
  if (UE_info->num_UEs == 0) return;

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {

    const NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    NR_mac_stats_t *stats = &UE_info->mac_stats[UE_id];
    const int avg_rsrp = stats->num_rsrp_meas > 0 ? stats->cumul_rsrp / stats->num_rsrp_meas : 0;
    stroff+=sprintf(output+stroff,"UE ID %d RNTI %04x (%d/%d) PH %d dB PCMAX %d dBm, average RSRP %d (%d meas)\n",
      UE_id,
      UE_info->rnti[UE_id],
      num++,
      UE_info->num_UEs,
      sched_ctrl->ph,
      sched_ctrl->pcmax,
      avg_rsrp,
      stats->num_rsrp_meas);

    stroff+=sprintf(output+stroff,"UE %d: dlsch_rounds %"PRIu64"/%"PRIu64"/%"PRIu64"/%"PRIu64", dlsch_errors %"PRIu64", pucch0_DTX %d, BLER %.5f MCS %d\n",
                    UE_id,
                    stats->dlsch_rounds[0], stats->dlsch_rounds[1],
                    stats->dlsch_rounds[2], stats->dlsch_rounds[3],
                    stats->dlsch_errors,
                    stats->pucch0_DTX,
                    sched_ctrl->dl_bler_stats.bler,
                    sched_ctrl->dl_bler_stats.mcs);
    if (reset_rsrp) {
      stats->num_rsrp_meas = 0;
      stats->cumul_rsrp = 0;
    }
    stroff+=sprintf(output+stroff,"UE %d: dlsch_total_bytes %"PRIu64"\n", UE_id, stats->dlsch_total_bytes);
    stroff+=sprintf(output+stroff,"UE %d: ulsch_rounds %"PRIu64"/%"PRIu64"/%"PRIu64"/%"PRIu64", ulsch_DTX %d, ulsch_errors %"PRIu64"\n",
                    UE_id,
                    stats->ulsch_rounds[0], stats->ulsch_rounds[1],
                    stats->ulsch_rounds[2], stats->ulsch_rounds[3],
                    stats->ulsch_DTX,
                    stats->ulsch_errors);
    stroff+=sprintf(output+stroff,
                    "UE %d: ulsch_total_bytes_scheduled %"PRIu64", ulsch_total_bytes_received %"PRIu64"\n",
                    UE_id,
                    stats->ulsch_total_bytes_scheduled, stats->ulsch_total_bytes_rx);
    for (int lc_id = 0; lc_id < 63; lc_id++) {
      if (stats->lc_bytes_tx[lc_id] > 0) {
        stroff+=sprintf(output+stroff, "UE %d: LCID %d: %"PRIu64" bytes TX\n", UE_id, lc_id, stats->lc_bytes_tx[lc_id]);
	LOG_D(NR_MAC, "UE %d: LCID %d: %"PRIu64" bytes TX\n", UE_id, lc_id, stats->lc_bytes_tx[lc_id]);
      }
      if (stats->lc_bytes_rx[lc_id] > 0) {
        stroff+=sprintf(output+stroff, "UE %d: LCID %d: %"PRIu64" bytes RX\n", UE_id, lc_id, stats->lc_bytes_rx[lc_id]);
	LOG_D(NR_MAC, "UE %d: LCID %d: %"PRIu64" bytes RX\n", UE_id, lc_id, stats->lc_bytes_rx[lc_id]);
      }
    }
  }
  print_meas_log(&gNB->eNB_scheduler, "DL & UL scheduling timing stats", NULL, NULL, output+stroff);
}


void mac_top_init_gNB(void)
{
  module_id_t     i;
  int             list_el;
  NR_UE_info_t    *UE_info;
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

      if (get_softmodem_params()->phy_test) {
        RC.nrmac[i]->pre_processor_dl = nr_preprocessor_phytest;
        RC.nrmac[i]->pre_processor_ul = nr_ul_preprocessor_phytest;
      } else {
        RC.nrmac[i]->pre_processor_dl = nr_init_fr1_dlsch_preprocessor(i, 0);
        RC.nrmac[i]->pre_processor_ul = nr_init_fr1_ulsch_preprocessor(i, 0);
      }
      pthread_create(&RC.nrmac[i]->stats_thread,NULL,nrmac_stats_thread,(void*)RC.nrmac[i]);

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
    
    UE_info = &nrmac->UE_info;
    UE_info->num_UEs = 0;
    create_nr_list(&UE_info->list, MAX_MOBILES_PER_GNB);
    for (list_el = 0; list_el < MAX_MOBILES_PER_GNB; list_el++) {
      UE_info->active[list_el] = false;
    }
  }

  srand48(0);
}
