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


#ifndef OPENAIR2
  #include "UTIL/OTG/otg_extern.h"
#endif

#include "s1ap_eNB.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"


#include "T.h"
#include "nfapi/oai_integration/vendor_ext.h"
//#define DEBUG_THREADS 1

//#define USRP_DEBUG 1
// Fix per CC openair rf/if device update
// extern openair0_device openair0;


extern volatile int start_gNB;
extern volatile int start_UE;
extern volatile int oai_exit;

extern openair0_config_t openair0_cfg[MAX_CARDS];

extern int transmission_mode;

extern uint16_t sf_ahead;
extern uint16_t sl_ahead;

//pthread_t                       main_gNB_thread;

time_stats_t softmodem_stats_mt; // main thread
time_stats_t softmodem_stats_hw; //  hw acquisition
time_stats_t softmodem_stats_rxtx_sf; // total tx time
time_stats_t nfapi_meas; // total tx time
time_stats_t softmodem_stats_rx_sf; // total rx time

/* mutex, cond and variable to serialize phy proc TX calls
 * (this mechanism may be relaxed in the future for better
 * performances)
 */
static struct {
  pthread_mutex_t  mutex_phy_proc_tx;
  pthread_cond_t   cond_phy_proc_tx;
  volatile uint8_t phy_proc_CC_id;
} sync_phy_proc;

extern double cpuf;

void init_gNB(int,int);
void stop_gNB(int nb_inst);

int wakeup_txfh(PHY_VARS_gNB *gNB, gNB_L1_rxtx_proc_t *proc, int frame_tx, int slot_tx, uint64_t timestamp_tx);
int wakeup_tx(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx, int frame_tx, int slot_tx, uint64_t timestamp_tx);
#include "executables/thread-common.h"
//extern PARALLEL_CONF_t get_thread_parallel_conf(void);
//extern WORKER_CONF_t   get_thread_worker_conf(void);


void wakeup_prach_gNB(PHY_VARS_gNB *gNB, RU_t *ru, int frame, int subframe);

extern uint8_t nfapi_mode;
extern void oai_subframe_ind(uint16_t sfn, uint16_t sf);
extern void oai_slot_ind(uint16_t sfn, uint16_t slot);
extern void add_subframe(uint16_t *frameP, uint16_t *subframeP, int offset);

//#define TICK_TO_US(ts) (ts.diff)
#define TICK_TO_US(ts) (ts.trials==0?0:ts.diff/ts.trials)

static inline int rxtx(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx, int frame_tx, int slot_tx, char *thread_name) {

  sl_ahead = sf_ahead*gNB->frame_parms.slots_per_subframe;
  nfapi_nr_config_request_scf_t *cfg = &gNB->gNB_config;

  start_meas(&softmodem_stats_rxtx_sf);

  // *******************************************************************
  // NFAPI not yet supported for NR - this code has to be revised
  if (NFAPI_MODE == NFAPI_MODE_PNF) {
    // I am a PNF and I need to let nFAPI know that we have a (sub)frame tick
    //add_subframe(&frame, &subframe, 4);
    //oai_subframe_ind(proc->frame_tx, proc->subframe_tx);
    //LOG_D(PHY, "oai_subframe_ind(frame:%u, subframe:%d) - NOT CALLED ********\n", frame, subframe);
    start_meas(&nfapi_meas);
    // oai_subframe_ind(frame_rx, slot_rx);
    oai_slot_ind(frame_rx, slot_rx);
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
    LOG_W(PHY, "to remove rnti %d\n", rnti_to_remove[i]);
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
    for (j = 0; j < NUMBER_OF_NR_DLSCH_MAX; j++)
      if (gNB->dlsch[j][0]->rnti == rnti_to_remove[i]) {
        gNB->dlsch[j][0]->rnti = 0;
        gNB->dlsch[j][0]->harq_mask = 0;
        //clean_gNB_dlsch(gNB->dlsch[j][0]);
        down_removed++;
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
  if (rnti_to_remove_count) LOG_W(PHY, "to remove rnti_to_remove_count=%d, up_removed=%d down_removed=%d pucch_removed=%d\n", rnti_to_remove_count, up_removed, down_removed, pucch_removed);
  rnti_to_remove_count = 0;
  if (pthread_mutex_unlock(&rnti_to_remove_mutex)) exit(1);

  /*
  // if this is IF5 or 3GPP_gNB
  if (gNB && gNB->RU_list && gNB->RU_list[0] && gNB->RU_list[0]->function < NGFI_RAU_IF4p5) {
    wakeup_prach_gNB(gNB,NULL,proc->frame_rx,proc->slot_rx);
  }
  */
  // Call the scheduler

  pthread_mutex_lock(&gNB->UL_INFO_mutex);
  gNB->UL_INFO.frame     = frame_rx;
  gNB->UL_INFO.slot      = slot_rx;
  gNB->UL_INFO.module_id = gNB->Mod_id;
  gNB->UL_INFO.CC_id     = gNB->CC_id;
  gNB->if_inst->NR_UL_indication(&gNB->UL_INFO);
  pthread_mutex_unlock(&gNB->UL_INFO_mutex);
  
  // RX processing
  int tx_slot_type; int rx_slot_type;
  if(NFAPI_MODE != NFAPI_MONOLITHIC) { //slot selection routines not working properly in nfapi, so temporarily hardcoding
    if ((slot_tx==8) || (slot_tx==9) || (slot_tx==18) ||  (slot_tx==19)) { //tx slot config
      tx_slot_type         = NR_UPLINK_SLOT;
    }
    else if ((slot_tx==7) || (slot_tx==17)) {
      tx_slot_type         = NR_MIXED_SLOT;
    }
    else {
      tx_slot_type         = NR_DOWNLINK_SLOT;;
    }

    if ((slot_rx==8) || (slot_rx==9) || (slot_rx==18) ||  (slot_rx==19)) { // rx slot config
      rx_slot_type         = NR_UPLINK_SLOT; 
    }
    else if ((slot_rx==7) || (slot_rx==17)) {
      rx_slot_type         = NR_MIXED_SLOT;
    }
    else {
      rx_slot_type         = NR_DOWNLINK_SLOT;;
    }


  }
  else {
  tx_slot_type         = nr_slot_select(cfg,frame_tx,slot_tx);
  rx_slot_type         = nr_slot_select(cfg,frame_rx,slot_rx);
  }
  if (rx_slot_type == NR_UPLINK_SLOT || rx_slot_type == NR_MIXED_SLOT) {
    // UE-specific RX processing for subframe n
    // TODO: check if this is correct for PARALLEL_RU_L1_TRX_SPLIT

    // Do PRACH RU processing
    L1_nr_prach_procedures(gNB,frame_rx,slot_rx);

    //apply the rx signal rotation here
    apply_nr_rotation_ul(&gNB->frame_parms,
			 gNB->common_vars.rxdataF[0],
			 slot_rx,
			 0,
			 gNB->frame_parms.Ncp==EXTENDED?12:14,
			 gNB->frame_parms.ofdm_symbol_size);
    
    phy_procedures_gNB_uespec_RX(gNB, frame_rx, slot_rx);
  }

  if (oai_exit) return(-1);

  // *****************************************
  // TX processing for subframe n+sf_ahead
  // run PHY TX procedures the one after the other for all CCs to avoid race conditions
  // (may be relaxed in the future for performance reasons)
  // *****************************************

  if (tx_slot_type == NR_DOWNLINK_SLOT || tx_slot_type == NR_MIXED_SLOT) {

    if(get_thread_parallel_conf() != PARALLEL_RU_L1_TRX_SPLIT) {
      phy_procedures_gNB_TX(gNB, frame_tx,slot_tx, 1);
    }
  }

  stop_meas( &softmodem_stats_rxtx_sf );
  LOG_D(PHY,"%s() Exit proc[rx:%d%d tx:%d%d]\n", __FUNCTION__, frame_rx, slot_rx, frame_tx, slot_tx);
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
  return(0);
}

static void *gNB_L1_thread_tx(void *param) {
  PHY_VARS_gNB *gNB        = (PHY_VARS_gNB *)param;
  gNB_L1_proc_t *gNB_proc  = &gNB->proc;
  gNB_L1_rxtx_proc_t *L1_proc_tx = &gNB_proc->L1_proc_tx;
  //PHY_VARS_gNB *gNB = RC.gNB[0][proc->CC_id];
  char thread_name[100];

  sprintf(thread_name,"gNB_L1_thread_tx\n");

  while (!oai_exit) {
    if (wait_on_condition(&L1_proc_tx->mutex,&L1_proc_tx->cond,&L1_proc_tx->instance_cnt,thread_name)<0) break;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_PROC_RXTX1, 1 );

    if (oai_exit) break;

    // *****************************************
    // TX processing for subframe n+4
    // run PHY TX procedures the one after the other for all CCs to avoid race conditions
    // (may be relaxed in the future for performance reasons)
    // *****************************************
    int      frame_tx = L1_proc_tx->frame_tx;
    int      slot_tx = L1_proc_tx->slot_tx;
    uint64_t timestamp_tx = L1_proc_tx->timestamp_tx;
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SLOT_NUMBER_TX1_GNB,slot_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX1_GNB,frame_tx);
    phy_procedures_gNB_TX(gNB, frame_tx,slot_tx, 1);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_WAKEUP_TXFH, 1 );
    pthread_mutex_lock( &L1_proc_tx->mutex );
    L1_proc_tx->instance_cnt = -1;

    // the thread can now be woken up
    if (pthread_cond_signal(&L1_proc_tx->cond) != 0) {
      LOG_E( PHY, "[gNB] ERROR pthread_cond_signal for gNB TXnp4 thread\n");
      exit_fun( "ERROR pthread_cond_signal" );
    }
    pthread_mutex_unlock(&L1_proc_tx->mutex);

    wakeup_txfh(gNB,L1_proc_tx,frame_tx,slot_tx,timestamp_tx);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_WAKEUP_TXFH, 0 );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_PROC_RXTX1, 0 );
  }

  return 0;
}

/*!
 * \brief The RX UE-specific and TX thread of gNB.
 * \param param is a \ref gNB_L1_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */

static void *gNB_L1_thread( void *param ) {
  static int gNB_thread_rxtx_status;
  PHY_VARS_gNB *gNB        = (PHY_VARS_gNB *)param;
  gNB_L1_proc_t *gNB_proc  = &gNB->proc;
  gNB_L1_rxtx_proc_t *L1_proc = &gNB_proc->L1_proc;
  //PHY_VARS_gNB *gNB = RC.gNB[0][proc->CC_id];
  char thread_name[100];
  // set default return value
  // set default return value
  gNB_thread_rxtx_status = 0;

  sprintf(thread_name,"gNB_L1_thread");


  while (!oai_exit) {

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_PROC_RXTX0, 0 );
    if (wait_on_condition(&L1_proc->mutex,&L1_proc->cond,&L1_proc->instance_cnt,thread_name)<0) break;
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_PROC_RXTX0, 1 );

    int frame_rx          = L1_proc->frame_rx;
    int slot_rx;
    if (NFAPI_MODE == NFAPI_MODE_PNF)
      slot_rx = (L1_proc->slot_rx) + 1;
    else 
      slot_rx = L1_proc->slot_rx;
    int frame_tx          = L1_proc->frame_tx;
    int slot_tx           = L1_proc->slot_tx;
    uint64_t timestamp_tx = L1_proc->timestamp_tx;
    if(NFAPI_MODE==NFAPI_MODE_VNF)
      if (gNB->CC_id==0) {
        int next_slot;
        next_slot = (slot_rx + 1) % 20;
        int next_frame_rx = (frame_rx + ((slot_rx + 1) % 20 == 0)) % 1024;
        int next_frame_tx = (frame_tx + ((slot_rx + 1) % 20 == 0)) % 1024;
        if (rxtx(gNB, next_frame_rx, next_slot, next_frame_tx, next_slot, thread_name) < 0) break;
      }
    if (wait_on_condition(&L1_proc->mutex,&L1_proc->cond,&L1_proc->instance_cnt,thread_name)<0) break;
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_PROC_RXTX0, 1 );


    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SLOT_NUMBER_TX0_GNB,slot_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SLOT_NUMBER_RX0_GNB,slot_rx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_GNB,frame_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_GNB,frame_rx);

    if (oai_exit) break;

    if (gNB->CC_id==0) {
      if (rxtx(gNB,frame_rx,slot_rx,frame_tx,slot_tx,thread_name) < 0) break;
    }

    if (release_thread(&L1_proc->mutex,&L1_proc->instance_cnt,thread_name)<0) break;

    if(get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT)  wakeup_tx(gNB,frame_rx,slot_rx,frame_tx,slot_tx,timestamp_tx);
    else if(get_thread_parallel_conf() == PARALLEL_RU_L1_SPLIT) wakeup_txfh(gNB,L1_proc,frame_tx,slot_tx,timestamp_tx);
  } // while !oai_exit

  LOG_D(PHY, " *** Exiting gNB thread RXn_TXnp4\n");
  gNB_thread_rxtx_status = 0;
  return &gNB_thread_rxtx_status;
}


#if 0 
// Wait for gNB application initialization to be complete (gNB registration to MME)
static void wait_system_ready (char *message, volatile int *start_flag) {
  static char *indicator[] = {".    ", "..   ", "...  ", ".... ", ".....",
                              " ....", "  ...", "   ..", "    .", "     "
                             };
  int i = 0;

  while ((!oai_exit) && (*start_flag == 0)) {
    LOG_N(EMU, message, indicator[i]);
    fflush(stdout);
    i = (i + 1) % (sizeof(indicator) / sizeof(indicator[0]));
    usleep(200000);
  }

  LOG_D(EMU,"\n");
}
#endif





void gNB_top(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx, char *string, struct RU_t_s *ru) {
  gNB_L1_proc_t *proc           = &gNB->proc;
  gNB_L1_rxtx_proc_t *L1_proc = &proc->L1_proc;
  NR_DL_FRAME_PARMS *fp = ru->nr_frame_parms;
  RU_proc_t *ru_proc=&ru->proc;
  proc->frame_rx    = frame_rx;
  proc->slot_rx = slot_rx;
  sl_ahead = sf_ahead*fp->slots_per_subframe;

  if (!oai_exit) {
    T(T_ENB_MASTER_TICK, T_INT(0), T_INT(proc->frame_rx), T_INT(proc->slot_rx));
    L1_proc->timestamp_tx = ru_proc->timestamp_rx + (sf_ahead*fp->samples_per_subframe);
    L1_proc->frame_rx     = ru_proc->frame_rx;
    L1_proc->slot_rx      = ru_proc->tti_rx;
    L1_proc->frame_tx     = (L1_proc->slot_rx > (fp->slots_per_frame-1-(fp->slots_per_subframe*sf_ahead))) ? (L1_proc->frame_rx+1)&1023 : L1_proc->frame_rx;
    L1_proc->slot_tx      = (L1_proc->slot_rx + (fp->slots_per_subframe*sf_ahead))%fp->slots_per_frame;

    if (rxtx(gNB,L1_proc->frame_rx,L1_proc->slot_rx,L1_proc->frame_tx,L1_proc->slot_tx,string) < 0) LOG_E(PHY,"gNB %d CC_id %d failed during execution\n",gNB->Mod_id,gNB->CC_id);

    ru_proc->timestamp_tx = L1_proc->timestamp_tx;
    ru_proc->tti_tx       = L1_proc->slot_tx;
    ru_proc->frame_tx     = L1_proc->frame_tx;
  }
}

int wakeup_txfh(PHY_VARS_gNB *gNB,gNB_L1_rxtx_proc_t *proc,int frame_tx,int slot_tx,uint64_t timestamp_tx) {

  RU_t *ru;
  RU_proc_t *ru_proc;

  int waitret = 0, ret = 0, time_ns = 1000*1000;
  struct timespec now, abstime;
  // note this should depend on the numerology used by the TX L1 thread, set here for 500us slot time
  // note this should depend on the numerology used by the TX L1 thread, set here for 500us slot time
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GAIN_CONTROL,1);
  time_ns = time_ns/gNB->frame_parms.slots_per_subframe;
  AssertFatal((ret = pthread_mutex_lock(&proc->mutex_RUs_tx))==0,"mutex_lock returns %d\n",ret);
  while (proc->instance_cnt_RUs < 0) {
    clock_gettime(CLOCK_REALTIME, &now);
    abstime.tv_sec = now.tv_sec;
    abstime.tv_nsec = now.tv_nsec + time_ns;

    if (abstime.tv_nsec >= 1000*1000*1000) {
      abstime.tv_nsec -= 1000*1000*1000;
      abstime.tv_sec  += 1;
    }
    if((waitret = pthread_cond_timedwait(&proc->cond_RUs,&proc->mutex_RUs_tx,&abstime)) == 0) break; // this unlocks mutex_rxtx while waiting and then locks it again
  }
  proc->instance_cnt_RUs = -1;
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_UE,proc->instance_cnt_RUs);
  AssertFatal((ret = pthread_mutex_unlock(&proc->mutex_RUs_tx))==0,"mutex_unlock returns %d\n",ret);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GAIN_CONTROL,0);

  if (waitret == ETIMEDOUT) {
    LOG_W(PHY,"Dropping TX slot (%d.%d) because FH is blocked more than 1 slot times (500us)\n",frame_tx,slot_tx);

    AssertFatal((ret=pthread_mutex_lock(&gNB->proc.mutex_RU_tx))==0,"mutex_lock returns %d\n",ret);
    gNB->proc.RU_mask_tx = 0;
    AssertFatal((ret=pthread_mutex_unlock(&gNB->proc.mutex_RU_tx))==0,"mutex_unlock returns %d\n",ret);
    AssertFatal((ret=pthread_mutex_lock(&proc->mutex_RUs_tx))==0,"mutex_lock returns %d\n",ret);
    proc->instance_cnt_RUs = 0;
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_UE,proc->instance_cnt_RUs);
    AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_RUs_tx))==0,"mutex_unlock returns %d\n",ret);

    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_UE,1);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_UE,0);

    return(-1);
  }

  for(int i=0; i<gNB->num_RU; i++)
  {
    ru      = gNB->RU_list[i];
    ru_proc = &ru->proc;


    if (ru_proc->instance_cnt_gNBs == 0) {
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST_UE, 1);
      LOG_E(PHY,"Frame %d, subframe %d: TX FH thread busy, dropping Frame %d, subframe %d\n", ru_proc->frame_tx, ru_proc->tti_tx, proc->frame_rx, proc->slot_rx);
      AssertFatal((ret=pthread_mutex_lock(&gNB->proc.mutex_RU_tx))==0,"mutex_lock returns %d\n",ret);
      gNB->proc.RU_mask_tx = 0;
      AssertFatal((ret=pthread_mutex_unlock(&gNB->proc.mutex_RU_tx))==0,"mutex_unlock returns %d\n",ret);

      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST_UE, 0);
      return(-1);
    }
    AssertFatal((ret = pthread_mutex_lock(&ru_proc->mutex_gNBs))==0,"ERROR pthread_mutex_lock failed on mutex_gNBs L1_thread_tx with ret=%d\n",ret);

    ru_proc->instance_cnt_gNBs = 0;
    ru_proc->timestamp_tx = timestamp_tx;
    ru_proc->tti_tx       = slot_tx;
    ru_proc->frame_tx     = frame_tx;

    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX1_UE, ru_proc->instance_cnt_gNBs);

    LOG_D(PHY,"Signaling tx_thread_fh for %d.%d\n",frame_tx,slot_tx);
    // the thread can now be woken up
    AssertFatal(pthread_cond_signal(&ru_proc->cond_gNBs) == 0,
                "[gNB] ERROR pthread_cond_signal for gNB TXnp4 thread\n");
    AssertFatal((ret=pthread_mutex_unlock(&ru_proc->mutex_gNBs))==0,"mutex_unlock returned %d\n",ret);

  }
  return(0);
}

int wakeup_tx(PHY_VARS_gNB *gNB,int frame_rx,int slot_rx,int frame_tx,int slot_tx,uint64_t timestamp_tx) {


  gNB_L1_rxtx_proc_t *L1_proc_tx = &gNB->proc.L1_proc_tx;

  int ret;
 
  
  AssertFatal((ret = pthread_mutex_lock(&L1_proc_tx->mutex))==0,"mutex_lock returns %d\n",ret);


  while(L1_proc_tx->instance_cnt == 0) {
    pthread_cond_wait(&L1_proc_tx->cond,&L1_proc_tx->mutex);
  }

  L1_proc_tx->instance_cnt = 0;

  
  L1_proc_tx->slot_rx       = slot_rx;
  L1_proc_tx->frame_rx      = frame_rx;
  L1_proc_tx->slot_tx       = slot_tx;
  L1_proc_tx->frame_tx      = frame_tx;
  L1_proc_tx->timestamp_tx  = timestamp_tx;

 
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX1_UE,1);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX1_UE,0);
  // the thread can now be woken up
  // the thread can now be woken up
  AssertFatal(pthread_cond_signal(&L1_proc_tx->cond) == 0, "ERROR pthread_cond_signal for gNB L1 thread\n");
  
  AssertFatal((ret=pthread_mutex_unlock(&L1_proc_tx->mutex))==0,"mutex_unlock returns %d\n",ret);

  return(0);
}

int wakeup_rxtx(PHY_VARS_gNB *gNB,RU_t *ru) {
  gNB_L1_proc_t *proc=&gNB->proc;
  gNB_L1_rxtx_proc_t *L1_proc=&proc->L1_proc;
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  RU_proc_t *ru_proc=&ru->proc;
  int ret;
  int i;
  struct timespec abstime;
  int time_ns = 50000;
  int wait_timer = 0;
  bool do_last_check = 1;
  
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_RU))==0,"mutex_lock returns %d\n",ret);
  for (i=0; i<gNB->num_RU; i++) {
    if (ru == gNB->RU_list[i]) {
      if ((proc->RU_mask&(1<<i)) > 0)
        LOG_E(PHY,"gNB %d frame %d, subframe %d : previous information from RU %d (num_RU %d,mask %x) has not been served yet!\n",
              gNB->Mod_id,proc->frame_rx,proc->slot_rx,ru->idx,gNB->num_RU,proc->RU_mask);
      proc->RU_mask |= (1<<i);
    }
  }
  if (proc->RU_mask != (1<<gNB->num_RU)-1) {  // not all RUs have provided their information so return
    LOG_E(PHY,"Not all RUs have provided their info\n");
    AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_RU))==0,"mutex_unlock returns %d\n",ret);
    return(0);
  }
  else { // all RUs have provided their information so continue on and wakeup gNB processing
    proc->RU_mask = 0;
    AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_RU))==0,"muex_unlock returns %d\n",ret);
  }

  // wake up TX for subframe n+sf_ahead
  // lock the TX mutex and make sure the thread is ready
  while (wait_timer < 200) {
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_nsec = abstime.tv_nsec + time_ns;

    if (abstime.tv_nsec >= 1000*1000*1000) {
      abstime.tv_nsec -= 1000*1000*1000;
      abstime.tv_sec  += 1;
    }

    AssertFatal((ret=pthread_mutex_timedlock(&L1_proc->mutex, &abstime)) == 0,"mutex_lock returns %d\n", ret);

    if (L1_proc->instance_cnt == 0) { // L1_thread is busy so wait for a bit
      AssertFatal((ret=pthread_mutex_unlock( &L1_proc->mutex))==0,"muex_unlock return %d\n",ret);
      wait_timer += 50;
      usleep(50);
    }
    else {
      do_last_check = 0;
      break;
    }
  }
  if (do_last_check) {
    AssertFatal((ret=pthread_mutex_timedlock(&L1_proc->mutex, &abstime)) == 0,"mutex_lock returns %d\n", ret);
    if (L1_proc->instance_cnt == 0) { // L1_thread is busy so abort the subframe
      AssertFatal((ret=pthread_mutex_unlock( &L1_proc->mutex))==0,"muex_unlock return %d\n",ret);
      LOG_W(PHY,"L1_thread isn't ready in %d.%d, aborting RX processing\n",ru_proc->frame_rx,ru_proc->tti_rx);
      return (-1);
    }
  }

  ++L1_proc->instance_cnt;
  // We have just received and processed the common part of a subframe, say n.
  // TS_rx is the last received timestamp (start of 1st slot), TS_tx is the desired
  // transmitted timestamp of the next TX slot (first).

  // The last (TS_rx mod samples_per_frame) was n*samples_per_tti, 
  // we want to generate subframe (n+sf_ahead), so TS_tx = TX_rx+sf_ahead*samples_per_tti,
  // and proc->slot_tx = proc->slot_rx+sf_ahead
  L1_proc->timestamp_tx = ru_proc->timestamp_rx + (sf_ahead*fp->samples_per_subframe);
  L1_proc->frame_rx     = ru_proc->frame_rx;
  L1_proc->slot_rx  = ru_proc->tti_rx;
  L1_proc->frame_tx = (L1_proc->slot_rx > (fp->slots_per_frame-1-(fp->slots_per_subframe*sf_ahead))) ? (L1_proc->frame_rx+1)&1023 : L1_proc->frame_rx;
  L1_proc->slot_tx  = (L1_proc->slot_rx + (fp->slots_per_subframe*sf_ahead))%fp->slots_per_frame;

  LOG_D(PHY,"wakeupL1: passing parameter IC = %d, RX: %d.%d, TX: %d.%d to L1 sf_ahead = %d\n", L1_proc->instance_cnt, L1_proc->frame_rx, L1_proc->slot_rx, L1_proc->frame_tx, L1_proc->slot_tx, sf_ahead);

  pthread_mutex_unlock( &L1_proc->mutex );

  // the thread can now be woken up
  if (pthread_cond_signal(&L1_proc->cond) != 0) {
    LOG_E( PHY, "[gNB] ERROR pthread_cond_signal for gNB RXn-TXnp4 thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }

  return(0);
}
/*
void wakeup_prach_gNB(PHY_VARS_gNB *gNB,RU_t *ru,int frame,int subframe) {

  gNB_L1_proc_t *proc = &gNB->proc;
  LTE_DL_FRAME_PARMS *fp=&gNB->frame_parms;
  int i;

  if (ru!=NULL) {
    pthread_mutex_lock(&proc->mutex_RU_PRACH);
    for (i=0;i<gNB->num_RU;i++) {
      if (ru == gNB->RU_list[i]) {
  LOG_D(PHY,"frame %d, subframe %d: RU %d for gNB %d signals PRACH (mask %x, num_RU %d)\n",frame,subframe,i,gNB->Mod_id,proc->RU_mask_prach,gNB->num_RU);
  if ((proc->RU_mask_prach&(1<<i)) > 0)
    LOG_E(PHY,"gNB %d frame %d, subframe %d : previous information (PRACH) from RU %d (num_RU %d, mask %x) has not been served yet!\n",
    gNB->Mod_id,frame,subframe,ru->idx,gNB->num_RU,proc->RU_mask_prach);
  proc->RU_mask_prach |= (1<<i);
      }
    }
    if (proc->RU_mask_prach != (1<<gNB->num_RU)-1) {  // not all RUs have provided their information so return
      pthread_mutex_unlock(&proc->mutex_RU_PRACH);
      return;
    }
    else { // all RUs have provided their information so continue on and wakeup gNB processing
      proc->RU_mask_prach = 0;
      pthread_mutex_unlock(&proc->mutex_RU_PRACH);
    }
  }

  // check if we have to detect PRACH first
  if (is_prach_subframe(fp,frame,subframe)>0) {
    LOG_D(PHY,"Triggering prach processing, frame %d, subframe %d\n",frame,subframe);
    if (proc->instance_cnt_prach == 0) {
      LOG_W(PHY,"[gNB] Frame %d Subframe %d, dropping PRACH\n", frame,subframe);
      return;
    }

    // wake up thread for PRACH RX
    if (pthread_mutex_lock(&proc->mutex_prach) != 0) {
      LOG_E( PHY, "[gNB] ERROR pthread_mutex_lock for gNB PRACH thread %d (IC %d)\n", proc->thread_index, proc->instance_cnt_prach);
      exit_fun( "error locking mutex_prach" );
      return;
    }

    ++proc->instance_cnt_prach;
    // set timing for prach thread
    proc->frame_prach = frame;
    proc->subframe_prach = subframe;

    // the thread can now be woken up
    if (pthread_cond_signal(&proc->cond_prach) != 0) {
      LOG_E( PHY, "[gNB] ERROR pthread_cond_signal for gNB PRACH thread %d\n", proc->thread_index);
      exit_fun( "ERROR pthread_cond_signal" );
      return;
    }

    pthread_mutex_unlock( &proc->mutex_prach );
  }

}*/

/*!
 * \brief The prach receive thread of gNB.
 * \param param is a \ref gNB_L1_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
/*
static void* gNB_thread_prach( void* param ) {
 static int gNB_thread_prach_status;


 PHY_VARS_gNB *gNB= (PHY_VARS_gNB *)param;
 gNB_L1_proc_t *proc = &gNB->proc;

 // set default return value
 gNB_thread_prach_status = 0;



 while (!oai_exit) {

   if (oai_exit) break;


   if (wait_on_condition(&proc->mutex_prach,&proc->cond_prach,&proc->instance_cnt_prach,"gNB_prach_thread") < 0) break;

   LOG_D(PHY,"Running gNB prach procedures\n");
   prach_procedures(gNB ,0);

   if (release_thread(&proc->mutex_prach,&proc->instance_cnt_prach,"gNB_prach_thread") < 0) break;
 }

 LOG_I(PHY, "Exiting gNB thread PRACH\n");

 gNB_thread_prach_status = 0;
 return &gNB_thread_prach_status;
}
*/

extern void init_td_thread(PHY_VARS_gNB *);
extern void init_te_thread(PHY_VARS_gNB *);

static void *process_stats_thread(void *param) {

  PHY_VARS_gNB *gNB  = (PHY_VARS_gNB *)param;

  reset_meas(&gNB->dlsch_encoding_stats);
  reset_meas(&gNB->dlsch_scrambling_stats);
  reset_meas(&gNB->dlsch_modulation_stats);

  wait_sync("process_stats_thread");

  while(!oai_exit)
  {
    sleep(1);
    print_meas(&gNB->dlsch_encoding_stats, "pdsch_encoding", NULL, NULL);
    print_meas(&gNB->dlsch_scrambling_stats, "pdsch_scrambling", NULL, NULL);
    print_meas(&gNB->dlsch_modulation_stats, "pdsch_modulation", NULL, NULL);
  }
  return(NULL);
}


void init_gNB_proc(int inst) {
  int i=0;
  int CC_id = 0;
  PHY_VARS_gNB *gNB;
  gNB_L1_proc_t *proc;
  gNB_L1_rxtx_proc_t *L1_proc,*L1_proc_tx;
//  LOG_I(PHY,"%s(inst:%d) RC.nb_nr_CC[inst]:%d \n",__FUNCTION__,inst,RC.nb_nr_CC[inst]);
  gNB = RC.gNB[inst];
  LOG_I(PHY,"Initializing gNB processes instance:%d CC_id %d \n",inst,CC_id);
  
  proc = &gNB->proc;
  L1_proc                        = &proc->L1_proc;
  L1_proc_tx                     = &proc->L1_proc_tx;
  L1_proc->instance_cnt          = -1;
  L1_proc_tx->instance_cnt       = -1;
  L1_proc->instance_cnt_RUs      = 0;
  L1_proc_tx->instance_cnt_RUs   = 0;
  proc->instance_cnt_prach       = -1;
  proc->instance_cnt_asynch_rxtx = -1;
  proc->CC_id                    = CC_id;
  proc->first_rx                 =1;
  proc->first_tx                 =1;
  proc->RU_mask                  =0;
  proc->RU_mask_tx               = (1<<gNB->num_RU)-1;
  proc->RU_mask_prach            =0;
  pthread_mutex_init( &gNB->UL_INFO_mutex, NULL);
  pthread_mutex_init( &L1_proc->mutex, NULL);
  pthread_mutex_init( &L1_proc_tx->mutex, NULL);
  pthread_cond_init( &L1_proc->cond, NULL);
  pthread_cond_init( &L1_proc_tx->cond, NULL);
  pthread_mutex_init( &proc->mutex_prach, NULL);
  pthread_mutex_init( &proc->mutex_asynch_rxtx, NULL);
  pthread_mutex_init( &proc->mutex_RU,NULL);
  pthread_mutex_init( &proc->mutex_RU_tx,NULL);
  pthread_mutex_init( &proc->mutex_RU_PRACH,NULL);
  pthread_cond_init( &proc->cond_prach, NULL);
  pthread_cond_init( &proc->cond_asynch_rxtx, NULL);
  LOG_I(PHY,"gNB->single_thread_flag:%d\n", gNB->single_thread_flag);
  
  if (get_thread_parallel_conf() == PARALLEL_RU_L1_SPLIT || get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT) {
    threadCreate( &L1_proc->pthread, gNB_L1_thread, gNB, "L1_proc", -1, OAI_PRIORITY_RT );
    threadCreate( &L1_proc_tx->pthread, gNB_L1_thread_tx, gNB,"L1_proc_tx", -1, OAI_PRIORITY_RT);
  }
  
  if(opp_enabled == 1) threadCreate(&proc->L1_stats_thread, process_stats_thread,(void *)gNB, "time_meas", -1, OAI_PRIORITY_RT_LOW);
  //pthread_create( &proc->pthread_prach, attr_prach, gNB_thread_prach, gNB );
  char name[16];
  
  if (gNB->single_thread_flag==0) {
    snprintf( name, sizeof(name), "L1 %d", i );
    pthread_setname_np( L1_proc->pthread, name );
    snprintf( name, sizeof(name), "L1TX %d", i );
    pthread_setname_np( L1_proc_tx->pthread, name );
  }
  
  AssertFatal(proc->instance_cnt_prach == -1,"instance_cnt_prach = %d\n",proc->instance_cnt_prach);

  /* setup PHY proc TX sync mechanism */
  pthread_mutex_init(&sync_phy_proc.mutex_phy_proc_tx, NULL);
  pthread_cond_init(&sync_phy_proc.cond_phy_proc_tx, NULL);
  sync_phy_proc.phy_proc_CC_id = 0;

  gNB->threadPool = (tpool_t*)malloc(sizeof(tpool_t));
  gNB->respDecode = (notifiedFIFO_t*) malloc(sizeof(notifiedFIFO_t));
  int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
  int threadCnt = min(numCPU, gNB->pusch_proc_threads);
  char ul_pool[80];
  sprintf(ul_pool,"-1");
  int s_offset = 0;
  for (int icpu=1; icpu<threadCnt; icpu++) {
    sprintf(ul_pool+2+s_offset,",-1");
    s_offset += 3;
  }
  initTpool(ul_pool, gNB->threadPool, false);
  initNotifiedFIFO(gNB->respDecode);
}



/*!
 * \brief Terminate gNB TX and RX threads.
 */
void kill_gNB_proc(int inst) {
  int *status;
  PHY_VARS_gNB *gNB;
  gNB_L1_proc_t *proc;
  gNB_L1_rxtx_proc_t *L1_proc, *L1_proc_tx;

  gNB=RC.gNB[inst];
  proc = &gNB->proc;
  L1_proc     = &proc->L1_proc;
  L1_proc_tx  = &proc->L1_proc_tx;
  LOG_I(PHY, "Killing TX inst %d\n",inst );
  
  if (get_thread_parallel_conf() == PARALLEL_RU_L1_SPLIT || get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT) {
    pthread_mutex_lock(&L1_proc->mutex);
    L1_proc->instance_cnt = 0;
    pthread_cond_signal(&L1_proc->cond);
    pthread_mutex_unlock(&L1_proc->mutex);
    pthread_mutex_lock(&L1_proc_tx->mutex);
    L1_proc_tx->instance_cnt = 0;
    pthread_cond_signal(&L1_proc_tx->cond);
    pthread_mutex_unlock(&L1_proc_tx->mutex);
  }
  
  proc->instance_cnt_prach = 0;
  pthread_cond_signal( &proc->cond_prach );
  pthread_cond_signal( &proc->cond_asynch_rxtx );
  pthread_cond_broadcast(&sync_phy_proc.cond_phy_proc_tx);
  //    LOG_D(PHY, "joining pthread_prach\n");
  //    pthread_join( proc->pthread_prach, (void**)&status );
  LOG_I(PHY, "Destroying prach mutex/cond\n");
  pthread_mutex_destroy( &proc->mutex_prach );
  pthread_cond_destroy( &proc->cond_prach );
  LOG_I(PHY, "Destroying UL_INFO mutex\n");
  pthread_mutex_destroy(&gNB->UL_INFO_mutex);
  
  if (get_thread_parallel_conf() == PARALLEL_RU_L1_SPLIT || get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT) {
    LOG_I(PHY, "Joining L1_proc mutex/cond\n");
    pthread_join( L1_proc->pthread, (void **)&status );
    LOG_I(PHY, "Joining L1_proc_tx mutex/cond\n");
    pthread_join( L1_proc_tx->pthread, (void **)&status );
  }
  
  LOG_I(PHY, "Destroying L1_proc mutex/cond\n");
  pthread_mutex_destroy( &L1_proc->mutex );
  pthread_cond_destroy( &L1_proc->cond );
  LOG_I(PHY, "Destroying L1_proc_tx mutex/cond\n");
  pthread_mutex_destroy( &L1_proc_tx->mutex );
  pthread_cond_destroy( &L1_proc_tx->cond );
  pthread_mutex_destroy( &proc->mutex_RU );
  pthread_mutex_destroy( &proc->mutex_RU_tx );
  
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
    init_gNB_proc(inst);
  }

  for (ru_id=0; ru_id<RC.nb_RU; ru_id++) {
    AssertFatal(RC.ru[ru_id]!=NULL,"ru_id %d is null\n",ru_id);
    RC.ru[ru_id]->nr_wakeup_rxtx         = wakeup_rxtx;
    //    RC.ru[ru_id]->wakeup_prach_eNB    = wakeup_prach_gNB;
    RC.ru[ru_id]->gNB_top             = gNB_top;
  }
}

void init_gNB(int single_thread_flag,int wait_for_sync) {

  int inst;
  PHY_VARS_gNB *gNB;

  if (RC.gNB == NULL) {
    RC.gNB = (PHY_VARS_gNB **) malloc((1+RC.nb_nr_L1_inst)*sizeof(PHY_VARS_gNB *));
    for (inst=0; inst<RC.nb_nr_L1_inst; inst++) {
      RC.gNB[inst] = (PHY_VARS_gNB *) malloc(sizeof(PHY_VARS_gNB));
      memset((void*)RC.gNB[inst],0,sizeof(PHY_VARS_gNB));
    }
  }

  LOG_I(PHY,"gNB L1 structure RC.gNB allocated @ %p\n",RC.gNB);

  for (inst=0; inst<RC.nb_nr_L1_inst; inst++) {

    LOG_I(PHY,"[lte-softmodem.c] gNB structure RC.gNB[%d] allocated @ %p\n",inst,RC.gNB[inst]);
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
  }
  

  LOG_I(PHY,"[nr-softmodem.c] gNB structure allocated\n");
}


void stop_gNB(int nb_inst) {
  for (int inst=0; inst<nb_inst; inst++) {
    LOG_I(PHY,"Killing gNB %d processing threads\n",inst);
    kill_gNB_proc(inst);
  }
}
