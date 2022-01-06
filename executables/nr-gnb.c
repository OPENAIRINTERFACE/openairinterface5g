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

/*! \file lte-enb.c
 * \brief Top-level threads for gNodeB
 * \author R. Knopp, F. Kaltenberger, Navid Nikaein
 * \date 2012
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
 * \note
 * \warning
 */

#define _GNU_SOURCE
#include <pthread.h>

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "assertions.h"
#include <common/utils/LOG/log.h>
#include <common/utils/system.h>

#include "PHY/types.h"

#include "PHY/INIT/phy_init.h"

#include "PHY/defs_gNB.h"
#include "SCHED/sched_eNB.h"
#include "SCHED_NR/sched_nr.h"
#include "SCHED_NR/fapi_nr_l1.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "../../ARCH/COMMON/common_lib.h"

//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

#include "PHY/phy_extern.h"

#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "RRC/LTE/rrc_extern.h"
#include "PHY_INTERFACE/phy_interface.h"
#include "common/utils/LOG/log_extern.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "enb_config.h"
#include "gnb_paramdef.h"


#include "s1ap_eNB.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"
#include <executables/softmodem-common.h>

#include "T.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "executables/softmodem-common.h"
#include <nfapi/oai_integration/nfapi_pnf.h>
#include <openair1/PHY/NR_TRANSPORT/nr_ulsch.h>
#include <openair1/PHY/NR_TRANSPORT/nr_dlsch.h>
#include <PHY/NR_ESTIMATION/nr_ul_estimation.h>

//#define USRP_DEBUG 1
// Fix per CC openair rf/if device update
// extern openair0_device openair0;


//pthread_t                       main_gNB_thread;

time_stats_t softmodem_stats_mt; // main thread
time_stats_t softmodem_stats_hw; //  hw acquisition
time_stats_t softmodem_stats_rxtx_sf; // total tx time
time_stats_t nfapi_meas; // total tx time
time_stats_t softmodem_stats_rx_sf; // total rx time


#include "executables/thread-common.h"


//#define TICK_TO_US(ts) (ts.diff)
#define TICK_TO_US(ts) (ts.trials==0?0:ts.diff/ts.trials)
#define L1STATSSTRLEN 16384


void tx_func(void *param) {

  processingData_L1tx_t *info = (processingData_L1tx_t *) param;
  int frame_tx = info->frame;
  int slot_tx = info->slot;

  phy_procedures_gNB_TX(info,
                        frame_tx,
                        slot_tx,
                        1);
}

void rx_func(void *param) {
  processingData_L1_t *info = (processingData_L1_t *) param;
  PHY_VARS_gNB *gNB = info->gNB;
  int frame_rx = info->frame_rx;
  int slot_rx = info->slot_rx;
  int frame_tx = info->frame_tx;
  int slot_tx = info->slot_tx;
  sl_ahead = sf_ahead*gNB->frame_parms.slots_per_subframe;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;

  start_meas(&softmodem_stats_rxtx_sf);

  // *******************************************************************

  if (NFAPI_MODE == NFAPI_MODE_PNF) {
    // I am a PNF and I need to let nFAPI know that we have a (sub)frame tick
    //LOG_D(PHY, "oai_nfapi_slot_ind(frame:%u, slot:%d) ********\n", frame_rx, slot_rx);
    start_meas(&nfapi_meas);
    handle_nr_slot_ind(frame_rx, slot_rx);
    stop_meas(&nfapi_meas);

    /*if (gNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus||
        gNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs ||
        gNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs ||
        gNB->UL_INFO.rach_ind.number_of_pdus ||
        gNB->UL_INFO.cqi_ind.number_of_cqis
       ) {
      LOG_D(PHY, "UL_info[rx_ind:%05d:%d harqs:%05d:%d crcs:%05d:%d rach_pdus:%0d.%d:%d cqis:%d] RX:%04d%d TX:%04d%d \n",
            NFAPI_SFNSF2DEC(gNB->UL_INFO.rx_ind.sfn_sf),   gNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus,
            NFAPI_SFNSF2DEC(gNB->UL_INFO.harq_ind.sfn_sf), gNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs,
            NFAPI_SFNSF2DEC(gNB->UL_INFO.crc_ind.sfn_sf),  gNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs,
            gNB->UL_INFO.rach_ind.sfn, gNB->UL_INFO.rach_ind.slot,gNB->UL_INFO.rach_ind.number_of_pdus,
            gNB->UL_INFO.cqi_ind.number_of_cqis,
            frame_rx, slot_rx,
            frame_tx, slot_tx);
    }*/
  }
  // ****************************************

  T(T_GNB_PHY_DL_TICK, T_INT(gNB->Mod_id), T_INT(frame_tx), T_INT(slot_tx));

  /* hack to remove UEs */
  extern int rnti_to_remove[10];
  extern volatile int rnti_to_remove_count;
  extern pthread_mutex_t rnti_to_remove_mutex;
  if (pthread_mutex_lock(&rnti_to_remove_mutex)) exit(1);
  int up_removed = 0;
  int down_removed = 0;
  int pucch_removed = 0;
  for (int i = 0; i < rnti_to_remove_count; i++) {
    LOG_W(NR_PHY, "to remove rnti 0x%04x\n", rnti_to_remove[i]);
    void clean_gNB_ulsch(NR_gNB_ULSCH_t *ulsch);
    void clean_gNB_dlsch(NR_gNB_DLSCH_t *dlsch);
    int j;
    for (j = 0; j < NUMBER_OF_NR_ULSCH_MAX; j++)
      if (gNB->ulsch[j][0]->rnti == rnti_to_remove[i]) {
        gNB->ulsch[j][0]->rnti = 0;
        gNB->ulsch[j][0]->harq_mask = 0;
        //clean_gNB_ulsch(gNB->ulsch[j][0]);
        int h;
        for (h = 0; h < NR_MAX_ULSCH_HARQ_PROCESSES; h++) {
          gNB->ulsch[j][0]->harq_processes[h]->status = SCH_IDLE;
          gNB->ulsch[j][0]->harq_processes[h]->round  = 0;
          gNB->ulsch[j][0]->harq_processes[h]->handled = 0;
        }
        up_removed++;
      }
    for (j = 0; j < NUMBER_OF_NR_PUCCH_MAX; j++)
      if (gNB->pucch[j]->active > 0 &&
          gNB->pucch[j]->pucch_pdu.rnti == rnti_to_remove[i]) {
        gNB->pucch[j]->active = 0;
        gNB->pucch[j]->pucch_pdu.rnti = 0;
        pucch_removed++;
      }
#if 0
    for (j = 0; j < NUMBER_OF_NR_PDCCH_MAX; j++)
      gNB->pdcch_pdu[j].frame = -1;
    for (j = 0; j < NUMBER_OF_NR_PDCCH_MAX; j++)
      gNB->ul_pdcch_pdu[j].frame = -1;
    for (j = 0; j < NUMBER_OF_NR_PRACH_MAX; j++)
      gNB->prach_vars.list[j].frame = -1;
#endif
  }
  if (rnti_to_remove_count) LOG_W(NR_PHY, "to remove rnti_to_remove_count=%d, up_removed=%d down_removed=%d pucch_removed=%d\n", rnti_to_remove_count, up_removed, down_removed, pucch_removed);
  rnti_to_remove_count = 0;
  if (pthread_mutex_unlock(&rnti_to_remove_mutex)) exit(1);

  // RX processing
  int tx_slot_type         = nr_slot_select(cfg,frame_tx,slot_tx);
  int rx_slot_type         = nr_slot_select(cfg,frame_rx,slot_rx);

  if (rx_slot_type == NR_UPLINK_SLOT || rx_slot_type == NR_MIXED_SLOT) {
    // UE-specific RX processing for subframe n
    // TODO: check if this is correct for PARALLEL_RU_L1_TRX_SPLIT

    // Do PRACH RU processing
    L1_nr_prach_procedures(gNB,frame_rx,slot_rx);

    //apply the rx signal rotation here
    for (int aa = 0; aa < gNB->frame_parms.nb_antennas_rx; aa++) {
      apply_nr_rotation_ul(&gNB->frame_parms,
                           gNB->common_vars.rxdataF[aa],
                           slot_rx,
                           0,
                           gNB->frame_parms.Ncp==EXTENDED?12:14,
                           gNB->frame_parms.ofdm_symbol_size);
    }
    phy_procedures_gNB_uespec_RX(gNB, frame_rx, slot_rx);
  }

  stop_meas( &softmodem_stats_rxtx_sf );
  LOG_D(PHY,"%s() Exit proc[rx:%d%d tx:%d%d]\n", __FUNCTION__, frame_rx, slot_rx, frame_tx, slot_tx);

  // Call the scheduler
  start_meas(&gNB->ul_indication_stats);
  pthread_mutex_lock(&gNB->UL_INFO_mutex);
  gNB->UL_INFO.frame     = frame_rx;
  gNB->UL_INFO.slot      = slot_rx;
  gNB->UL_INFO.module_id = gNB->Mod_id;
  gNB->UL_INFO.CC_id     = gNB->CC_id;
  gNB->if_inst->NR_UL_indication(&gNB->UL_INFO);
  pthread_mutex_unlock(&gNB->UL_INFO_mutex);
  stop_meas(&gNB->ul_indication_stats);
  
  if (tx_slot_type == NR_DOWNLINK_SLOT || tx_slot_type == NR_MIXED_SLOT) {
    notifiedFIFO_elt_t *res;
    processingData_L1tx_t *syncMsg;
    // Its a FIFO so it maitains the order in which the MAC fills the messages
    // so no need for checking for right slot
    res = pullTpool(gNB->L1_tx_filled, gNB->threadPool);
    syncMsg = (processingData_L1tx_t *)NotifiedFifoData(res);
    syncMsg->gNB = gNB;
    syncMsg->timestamp_tx = info->timestamp_tx;
    res->key = slot_tx;
    pushTpool(gNB->threadPool, res);
  }
    
#if 0
  LOG_D(PHY, "rxtx:%lld nfapi:%lld phy:%lld tx:%lld rx:%lld prach:%lld ofdm:%lld ",
        softmodem_stats_rxtx_sf.diff_now, nfapi_meas.diff_now,
        TICK_TO_US(gNB->phy_proc),
        TICK_TO_US(gNB->phy_proc_tx),
        TICK_TO_US(gNB->phy_proc_rx),
        TICK_TO_US(gNB->rx_prach),
        TICK_TO_US(gNB->ofdm_mod_stats),
        softmodem_stats_rxtx_sf.diff_now, nfapi_meas.diff_now);
  LOG_D(PHY,
        "dlsch[enc:%lld mod:%lld scr:%lld rm:%lld t:%lld i:%lld] rx_dft:%lld ",
        TICK_TO_US(gNB->dlsch_encoding_stats),
        TICK_TO_US(gNB->dlsch_modulation_stats),
        TICK_TO_US(gNB->dlsch_scrambling_stats),
        TICK_TO_US(gNB->dlsch_rate_matching_stats),
        TICK_TO_US(gNB->dlsch_turbo_encoding_stats),
        TICK_TO_US(gNB->dlsch_interleaving_stats),
        TICK_TO_US(gNB->rx_dft_stats));
  LOG_D(PHY," ulsch[ch:%lld freq:%lld dec:%lld demod:%lld ru:%lld ",
        TICK_TO_US(gNB->ulsch_channel_estimation_stats),
        TICK_TO_US(gNB->ulsch_freq_offset_estimation_stats),
        TICK_TO_US(gNB->ulsch_decoding_stats),
        TICK_TO_US(gNB->ulsch_demodulation_stats),
        TICK_TO_US(gNB->ulsch_rate_unmatching_stats));
  LOG_D(PHY, "td:%lld dei:%lld dem:%lld llr:%lld tci:%lld ",
        TICK_TO_US(gNB->ulsch_turbo_decoding_stats),
        TICK_TO_US(gNB->ulsch_deinterleaving_stats),
        TICK_TO_US(gNB->ulsch_demultiplexing_stats),
        TICK_TO_US(gNB->ulsch_llr_stats),
        TICK_TO_US(gNB->ulsch_tc_init_stats));
  LOG_D(PHY, "tca:%lld tcb:%lld tcg:%lld tce:%lld l1:%lld l2:%lld]\n\n",
        TICK_TO_US(gNB->ulsch_tc_alpha_stats),
        TICK_TO_US(gNB->ulsch_tc_beta_stats),
        TICK_TO_US(gNB->ulsch_tc_gamma_stats),
        TICK_TO_US(gNB->ulsch_tc_ext_stats),
        TICK_TO_US(gNB->ulsch_tc_intl1_stats),
        TICK_TO_US(gNB->ulsch_tc_intl2_stats)
       );
#endif
}
static void dump_L1_meas_stats(PHY_VARS_gNB *gNB, RU_t *ru, char *output) {
  int stroff = 0;
  stroff += print_meas_log(gNB->phy_proc_tx[0], "L1 Tx processing thread 0", NULL, NULL, output);
  //stroff += print_meas_log(gNB->phy_proc_tx[1], "L1 Tx processing thread 1", NULL, NULL, output+stroff);
  stroff += print_meas_log(&gNB->dlsch_encoding_stats, "DLSCH encoding", NULL, NULL, output+stroff);
  stroff += print_meas_log(&gNB->phy_proc_rx, "L1 Rx processing", NULL, NULL, output+stroff);
  stroff += print_meas_log(&gNB->ul_indication_stats, "UL Indication", NULL, NULL, output+stroff);
  stroff += print_meas_log(&gNB->rx_pusch_stats, "PUSCH inner-receiver", NULL, NULL, output+stroff);
  stroff += print_meas_log(&gNB->ulsch_decoding_stats, "PUSCH decoding", NULL, NULL, output+stroff);
  if (ru->feprx) stroff += print_meas_log(&ru->ofdm_demod_stats,"feprx",NULL,NULL, output+stroff);

  if (ru->feptx_ofdm) {
    stroff += print_meas_log(&ru->precoding_stats,"feptx_prec",NULL,NULL, output+stroff);
    stroff += print_meas_log(&ru->txdataF_copy_stats,"txdataF_copy",NULL,NULL, output+stroff);
    stroff += print_meas_log(&ru->ofdm_mod_stats,"feptx_ofdm",NULL,NULL, output+stroff);
    stroff += print_meas_log(&ru->ofdm_total_stats,"feptx_total",NULL,NULL, output+stroff);
  }

  if (ru->fh_north_asynch_in) stroff += print_meas_log(&ru->rx_fhaul,"rx_fhaul",NULL,NULL, output+stroff);

  stroff += print_meas_log(&ru->tx_fhaul,"tx_fhaul",NULL,NULL, output+stroff);

  if (ru->fh_north_out) {
    stroff += print_meas_log(&ru->compression,"compression",NULL,NULL, output+stroff);
    stroff += print_meas_log(&ru->transport,"transport",NULL,NULL, output+stroff);
  }
}

void *nrL1_stats_thread(void *param) {
  PHY_VARS_gNB     *gNB      = (PHY_VARS_gNB *)param;
  RU_t *ru = RC.ru[0];
  char output[L1STATSSTRLEN];
  memset(output,0,L1STATSSTRLEN);
  wait_sync("L1_stats_thread");
  FILE *fd;
  fd=fopen("nrL1_stats.log","w");
  AssertFatal(fd!=NULL,"Cannot open nrL1_stats.log\n");

  reset_meas(gNB->phy_proc_tx[0]);
  //reset_meas(gNB->phy_proc_tx[1]);
  reset_meas(&gNB->dlsch_encoding_stats);
  reset_meas(&gNB->phy_proc_rx);
  reset_meas(&gNB->ul_indication_stats);
  reset_meas(&gNB->rx_pusch_stats);
  reset_meas(&gNB->ulsch_decoding_stats);

  while (!oai_exit) {
    sleep(1);
    dump_nr_I0_stats(fd,gNB);
    dump_pdsch_stats(fd,gNB);
    dump_pusch_stats(fd,gNB);
    dump_L1_meas_stats(gNB, ru, output);
    fprintf(fd,"%s\n",output);
    fflush(fd);
    fseek(fd,0,SEEK_SET);
  }
  fclose(fd);
  return(NULL);
}

// This thread reads the finished L1 tx jobs from threaPool
// and pushes RU tx thread in the right order. It works only
// two parallel L1 tx threads.
void *tx_reorder_thread(void* param) {
  PHY_VARS_gNB *gNB = (PHY_VARS_gNB *)param;
    notifiedFIFO_elt_t *resL1Reserve = NULL;
  

  resL1Reserve=pullTpool(gNB->L1_tx_out, gNB->threadPool);
  int next_tx_slot=((processingData_L1tx_t *)NotifiedFifoData(resL1Reserve))->slot;
  
  while (!oai_exit) {
    notifiedFIFO_elt_t *resL1;
    if (resL1Reserve) {
       resL1=resL1Reserve;
       if (((processingData_L1tx_t *)NotifiedFifoData(resL1))->slot != next_tx_slot) {
         LOG_E(PHY,"order mistake");
	 resL1Reserve=NULL;
	 resL1 = pullTpool(gNB->L1_tx_out, gNB->threadPool);
       }
     } else { 
       resL1 = pullTpool(gNB->L1_tx_out, gNB->threadPool);
       if (((processingData_L1tx_t *)NotifiedFifoData(resL1))->slot != next_tx_slot) {
          if (resL1Reserve)
              LOG_E(PHY,"error, have a stored packet, then a second one\n");
	  resL1Reserve=resL1;
          resL1 = pullTpool(gNB->L1_tx_out, gNB->threadPool);
          if (((processingData_L1tx_t *)NotifiedFifoData(resL1))->slot != next_tx_slot)
	    LOG_E(PHY,"error, pull two msg, none is good\n");
       }
    }
    processingData_L1tx_t *syncMsgL1= (processingData_L1tx_t *)NotifiedFifoData(resL1);
    processingData_RU_t syncMsgRU;
    syncMsgRU.frame_tx = syncMsgL1->frame;
    syncMsgRU.slot_tx = syncMsgL1->slot;
    syncMsgRU.timestamp_tx = syncMsgL1->timestamp_tx;
    syncMsgRU.ru = gNB->RU_list[0];
    next_tx_slot = get_next_downlink_slot(gNB, &gNB->gNB_config, syncMsgRU.frame_tx, syncMsgRU.slot_tx);
    pushNotifiedFIFO(gNB->L1_tx_free, resL1);
    if (resL1==resL1Reserve)
       resL1Reserve=NULL;
    ru_tx_func((void*)&syncMsgRU);
  }
  return(NULL);
}

void init_gNB_Tpool(int inst) {
  PHY_VARS_gNB *gNB;
  gNB = RC.gNB[inst];
  gNB_L1_proc_t *proc = &gNB->proc;

  // ULSCH decoding threadpool
  gNB->threadPool = (tpool_t*)malloc(sizeof(tpool_t));
  int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
  LOG_I(PHY,"Number of threads requested in config file: %d, Number of threads available on this machine: %d\n",gNB->pusch_proc_threads,numCPU);
  int threadCnt = min(numCPU, gNB->pusch_proc_threads);
  if (threadCnt < 2) LOG_E(PHY,"Number of threads for gNB should be more than 1. Allocated only %d\n",threadCnt);
  char pool[80];
  sprintf(pool,"-1");
  int s_offset = 0;
  for (int icpu=1; icpu<threadCnt; icpu++) {
    sprintf(pool+2+s_offset,",-1");
    s_offset += 3;
  }
  if (getenv("noThreads")) strcpy(pool, "n");
  initTpool(pool, gNB->threadPool, false);
  // ULSCH decoder result FIFO
  gNB->respDecode = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  initNotifiedFIFO(gNB->respDecode);

  // L1 RX result FIFO 
  gNB->resp_L1 = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  initNotifiedFIFO(gNB->resp_L1);
  notifiedFIFO_elt_t *msg = newNotifiedFIFO_elt(sizeof(processingData_L1_t),0,gNB->resp_L1,rx_func);
  pushNotifiedFIFO(gNB->resp_L1,msg); // to unblock the process in the beginning

  // L1 TX result FIFO 
  gNB->L1_tx_free = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  gNB->L1_tx_filled = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  gNB->L1_tx_out = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  initNotifiedFIFO(gNB->L1_tx_free);
  initNotifiedFIFO(gNB->L1_tx_filled);
  initNotifiedFIFO(gNB->L1_tx_out);
  
  // we create 2 threads for L1 tx processing
  for (int i=0; i < 1; i++) {
    notifiedFIFO_elt_t *msgL1Tx = newNotifiedFIFO_elt(sizeof(processingData_L1tx_t),0,gNB->L1_tx_out,tx_func);
    processingData_L1tx_t *msgDataTx = (processingData_L1tx_t *)NotifiedFifoData(msgL1Tx);
    init_DLSCH_struct(gNB, msgDataTx);
    memset(msgDataTx->ssb, 0, 64*sizeof(NR_gNB_SSB_t));
    reset_meas(&msgDataTx->phy_proc_tx);
    gNB->phy_proc_tx[i] = &msgDataTx->phy_proc_tx;
    pushNotifiedFIFO(gNB->L1_tx_free,msgL1Tx); // to unblock the process in the beginning
  }

  threadCreate(&proc->L1_stats_thread,nrL1_stats_thread,(void*)gNB,"L1_stats",-1,OAI_PRIORITY_RT_LOW);
  threadCreate(&proc->pthread_tx_reorder, tx_reorder_thread, (void *)gNB, "thread_tx_reorder", -1, OAI_PRIORITY_RT_MAX);
}


/*!
 * \brief Terminate gNB TX and RX threads.
 */
void kill_gNB_proc(int inst) {
  PHY_VARS_gNB *gNB;

  gNB=RC.gNB[inst];
  
  LOG_I(PHY, "Destroying UL_INFO mutex\n");
  pthread_mutex_destroy(&gNB->UL_INFO_mutex);
  
}

void reset_opp_meas(void) {
  int sfn;
  reset_meas(&softmodem_stats_mt);
  reset_meas(&softmodem_stats_hw);

  for (sfn=0; sfn < 10; sfn++) {
    reset_meas(&softmodem_stats_rxtx_sf);
    reset_meas(&softmodem_stats_rx_sf);
  }
}


void print_opp_meas(void) {
  int sfn=0;
  print_meas(&softmodem_stats_mt, "Main gNB Thread", NULL, NULL);
  print_meas(&softmodem_stats_hw, "HW Acquisation", NULL, NULL);

  for (sfn=0; sfn < 10; sfn++) {
    print_meas(&softmodem_stats_rxtx_sf,"[gNB][total_phy_proc_rxtx]",NULL, NULL);
    print_meas(&softmodem_stats_rx_sf,"[gNB][total_phy_proc_rx]",NULL,NULL);
  }
}


/// eNB kept in function name for nffapi calls, TO FIX
void init_eNB_afterRU(void) {
  int inst,ru_id,i,aa;
  PHY_VARS_gNB *gNB;
  LOG_I(PHY,"%s() RC.nb_nr_inst:%d\n", __FUNCTION__, RC.nb_nr_inst);

  if(NFAPI_MODE == NFAPI_MODE_PNF)
    RC.nb_nr_inst = 1;
  for (inst=0; inst<RC.nb_nr_inst; inst++) {
    LOG_I(PHY,"RC.nb_nr_CC[inst:%d]:%p\n", inst, RC.gNB[inst]);
    gNB                                  =  RC.gNB[inst];
    phy_init_nr_gNB(gNB,0,0);

    // map antennas and PRACH signals to gNB RX
    if (0) AssertFatal(gNB->num_RU>0,"Number of RU attached to gNB %d is zero\n",gNB->Mod_id);

    LOG_I(PHY,"Mapping RX ports from %d RUs to gNB %d\n",gNB->num_RU,gNB->Mod_id);
    LOG_I(PHY,"gNB->num_RU:%d\n", gNB->num_RU);

    for (ru_id=0,aa=0; ru_id<gNB->num_RU; ru_id++) {
      AssertFatal(gNB->RU_list[ru_id]->common.rxdataF!=NULL,
		  "RU %d : common.rxdataF is NULL\n",
		  gNB->RU_list[ru_id]->idx);
      AssertFatal(gNB->RU_list[ru_id]->prach_rxsigF!=NULL,
		  "RU %d : prach_rxsigF is NULL\n",
		  gNB->RU_list[ru_id]->idx);
      
      for (i=0; i<gNB->RU_list[ru_id]->nb_rx; aa++,i++) {
	LOG_I(PHY,"Attaching RU %d antenna %d to gNB antenna %d\n",gNB->RU_list[ru_id]->idx,i,aa);
	gNB->prach_vars.rxsigF[aa]    =  gNB->RU_list[ru_id]->prach_rxsigF[0][i];
#if 0
printf("before %p\n", gNB->common_vars.rxdataF[aa]);
#endif
	gNB->common_vars.rxdataF[aa]     =  gNB->RU_list[ru_id]->common.rxdataF[i];
#if 0
printf("after %p\n", gNB->common_vars.rxdataF[aa]);
#endif
      }
    }

    /* TODO: review this code, there is something wrong.
     * In monolithic mode, we come here with nb_antennas_rx == 0
     * (not tested in other modes).
     */
    //init_precoding_weights(RC.gNB[inst]);
    init_gNB_Tpool(inst);
  }

}

void init_gNB(int single_thread_flag,int wait_for_sync) {

  int inst;
  PHY_VARS_gNB *gNB;

  if (RC.gNB == NULL) {
    RC.gNB = (PHY_VARS_gNB **) calloc(1+RC.nb_nr_L1_inst, sizeof(PHY_VARS_gNB *));
    LOG_I(PHY,"gNB L1 structure RC.gNB allocated @ %p\n",RC.gNB);
  }

  for (inst=0; inst<RC.nb_nr_L1_inst; inst++) {

    if (RC.gNB[inst] == NULL) {
      RC.gNB[inst] = (PHY_VARS_gNB *) calloc(1, sizeof(PHY_VARS_gNB));
      LOG_I(PHY,"[nr-gnb.c] gNB structure RC.gNB[%d] allocated @ %p\n",inst,RC.gNB[inst]);
    }
    gNB                     = RC.gNB[inst];
    gNB->abstraction_flag   = 0;
    gNB->single_thread_flag = single_thread_flag;
    /*nr_polar_init(&gNB->nrPolar_params,
      NR_POLAR_PBCH_MESSAGE_TYPE,
      NR_POLAR_PBCH_PAYLOAD_BITS,
      NR_POLAR_PBCH_AGGREGATION_LEVEL);*/
    LOG_I(PHY,"Initializing gNB %d single_thread_flag:%d\n",inst,gNB->single_thread_flag);
    LOG_I(PHY,"Initializing gNB %d\n",inst);

    LOG_I(PHY,"Registering with MAC interface module (before %p)\n",gNB->if_inst);
    AssertFatal((gNB->if_inst         = NR_IF_Module_init(inst))!=NULL,"Cannot register interface");
    LOG_I(PHY,"Registering with MAC interface module (after %p)\n",gNB->if_inst);
    gNB->if_inst->NR_Schedule_response   = nr_schedule_response;
    gNB->if_inst->NR_PHY_config_req      = nr_phy_config_request;
    memset((void *)&gNB->UL_INFO,0,sizeof(gNB->UL_INFO));
    LOG_I(PHY,"Setting indication lists\n");

    gNB->UL_INFO.rx_ind.pdu_list = gNB->rx_pdu_list;
    gNB->UL_INFO.crc_ind.crc_list = gNB->crc_pdu_list;
    /*gNB->UL_INFO.sr_ind.sr_indication_body.sr_pdu_list = gNB->sr_pdu_list;
    gNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list = gNB->harq_pdu_list;
    gNB->UL_INFO.cqi_ind.cqi_pdu_list = gNB->cqi_pdu_list;
    gNB->UL_INFO.cqi_ind.cqi_raw_pdu_list = gNB->cqi_raw_pdu_list;*/

    gNB->prach_energy_counter = 0;
    gNB->prb_interpolation = get_softmodem_params()->prb_interpolation;
  }
  

  LOG_I(PHY,"[nr-gnb.c] gNB structure allocated\n");
}


void stop_gNB(int nb_inst) {
  for (int inst=0; inst<nb_inst; inst++) {
    LOG_I(PHY,"Killing gNB %d processing threads\n",inst);
    kill_gNB_proc(inst);
  }
}
