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
 * \brief Top-level threads for eNodeB
 * \author R. Knopp, F. Kaltenberger, Navid Nikaein
 * \date 2012
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr, navid.nikaein@eurecom.fr
 * \note
 * \warning
 */

/*! \function wakeup_txfh
 * \brief Implementation of creating multiple RU threads for beamforming emulation
 * \author TH Wang(Judy), TY Hsu, SY Yeh(fdragon)
 * \date 2018
 * \version 0.1
 * \company Eurecom and ISIP@NCTU
 * \email: Tsu-Han.Wang@eurecom.fr,tyhsu@cs.nctu.edu.tw,fdragon.cs96g@g2.nctu.edu.tw
 * \note
 * \warning
 */


#define _GNU_SOURCE
#include <pthread.h>


#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "assertions.h"


#include "PHY/types.h"

#include "PHY/INIT/phy_init.h"

#include "PHY/defs_eNB.h"
#include "SCHED/sched_eNB.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "nfapi/oai_integration/vendor_ext.h"
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "targets/ARCH/COMMON/common_lib.h"

//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"

#include "PHY/phy_extern.h"

#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/MAC/mac_proto.h"
#include "RRC/LTE/rrc_extern.h"
#include "PHY_INTERFACE/phy_interface.h"
#include "common/utils/LOG/log.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "enb_config.h"
#include "targets/RT/USER/lte-softmodem.h"

#include "s1ap_eNB.h"
#include "SIMULATION/ETH_TRANSPORT/proto.h"

#include "T.h"

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

//#define DEBUG_THREADS 1

//#define USRP_DEBUG 1
struct timing_info_t {
  //unsigned int frame, hw_slot, last_slot, next_slot;
  //unsigned int mbox0, mbox1, mbox2, mbox_target;
  unsigned int n_samples;
} timing_info;

// Fix per CC openair rf/if device update
// extern openair0_device openair0;

extern int oai_exit;

extern int transmission_mode;

extern int oaisim_flag;

#include "executables/thread-common.h"
//extern PARALLEL_CONF_t get_thread_parallel_conf(void);
//extern WORKER_CONF_t   get_thread_worker_conf(void);

//pthread_t                       main_eNB_thread;

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


void init_eNB(int,int);
void stop_eNB(int nb_inst);

int wakeup_tx(PHY_VARS_eNB *eNB, int frame_rx, int subframe_rx, int frame_tx, int subframe_tx, uint64_t timestamp_tx);
int wakeup_txfh(PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, int frame_tx, int subframe_tx, uint64_t timestamp_tx);
void wakeup_prach_eNB(PHY_VARS_eNB *eNB,RU_t *ru, int frame, int subframe);
void wakeup_prach_eNB_br(PHY_VARS_eNB *eNB,RU_t *ru, int frame, int subframe);


extern void oai_subframe_ind(uint16_t sfn, uint16_t sf);
extern void add_subframe(uint16_t *frameP, uint16_t *subframeP, int offset);

//#define TICK_TO_US(ts) (ts.diff)
#define TICK_TO_US(ts) (ts.trials==0?0:ts.diff/ts.trials)


static inline int rxtx(PHY_VARS_eNB *eNB,
                       L1_rxtx_proc_t *proc,
                       char *thread_name) {
  int ret;
  start_meas(&softmodem_stats_rxtx_sf);
  //L1_rxtx_proc_t *L1_proc_tx = &eNB->proc.L1_proc_tx;
  // *******************************************************************
#if defined(PRE_SCD_THREAD)
  RU_t *ru = RC.ru[0];
#endif

  if (eNB ==NULL) {
    LOG_D(PHY,"%s:%d: rxtx invalid argument, eNB pointer is NULL",__FILE__,__LINE__);
    return -1;
  }

  if (NFAPI_MODE==NFAPI_MODE_PNF) {
    // I am a PNF and I need to let nFAPI know that we have a (sub)frame tick
    uint16_t frame = proc->frame_rx;
    uint16_t subframe = proc->subframe_rx;
    //add_subframe(&frame, &subframe, 4);
    //oai_subframe_ind(proc->frame_tx, proc->subframe_tx);
    //LOG_D(PHY, "oai_subframe_ind(frame:%u, subframe:%d) - NOT CALLED ********\n", frame, subframe);
    start_meas(&nfapi_meas);
    oai_subframe_ind(frame, subframe);
    stop_meas(&nfapi_meas);

    if (eNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus||
        eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs ||
        eNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs ||
        eNB->UL_INFO.rach_ind.rach_indication_body.number_of_preambles ||
        eNB->UL_INFO.cqi_ind.cqi_indication_body.number_of_cqis
       ) {
      LOG_D(PHY, "UL_info[rx_ind:%05d:%d harqs:%05d:%d crcs:%05d:%d preambles:%05d:%d cqis:%d] RX:%04d%d TX:%04d%d num_pdcch_symbols:%d\n",
            NFAPI_SFNSF2DEC(eNB->UL_INFO.rx_ind.sfn_sf),   eNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus,
            NFAPI_SFNSF2DEC(eNB->UL_INFO.harq_ind.sfn_sf), eNB->UL_INFO.harq_ind.harq_indication_body.number_of_harqs,
            NFAPI_SFNSF2DEC(eNB->UL_INFO.crc_ind.sfn_sf),  eNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs,
            NFAPI_SFNSF2DEC(eNB->UL_INFO.rach_ind.sfn_sf), eNB->UL_INFO.rach_ind.rach_indication_body.number_of_preambles,
            eNB->UL_INFO.cqi_ind.cqi_indication_body.number_of_cqis,
            proc->frame_rx, proc->subframe_rx,
            proc->frame_tx, proc->subframe_tx, eNB->pdcch_vars[proc->subframe_tx&1].num_pdcch_symbols);
    }
  }

  if (NFAPI_MODE==NFAPI_MODE_PNF && eNB->pdcch_vars[proc->subframe_tx&1].num_pdcch_symbols == 0) {
    LOG_E(PHY, "eNB->pdcch_vars[proc->subframe_tx&1].num_pdcch_symbols == 0");
    return 0;
  }

  // ****************************************
  // Common RX procedures subframe n
  T(T_ENB_PHY_DL_TICK, T_INT(eNB->Mod_id), T_INT(proc->frame_tx), T_INT(proc->subframe_tx));

  // if this is IF5 or 3GPP_eNB
  if (eNB->RU_list && eNB->RU_list[0] && eNB->RU_list[0]->function < NGFI_RAU_IF4p5) {
    wakeup_prach_eNB(eNB,NULL,proc->frame_rx,proc->subframe_rx);
    wakeup_prach_eNB_br(eNB,NULL,proc->frame_rx,proc->subframe_rx);
  }

  if (NFAPI_MODE!=NFAPI_MODE_PNF) {
    release_UE_in_freeList(eNB->Mod_id);
  } else {
    release_rnti_of_phy(eNB->Mod_id);
  }

  // UE-specific RX processing for subframe n
  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    phy_procedures_eNB_uespec_RX(eNB, proc);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER, 1 );
#if defined(PRE_SCD_THREAD)

  if (NFAPI_MODE==NFAPI_MODE_VNF) {
    new_dlsch_ue_select_tbl_in_use = dlsch_ue_select_tbl_in_use;
    dlsch_ue_select_tbl_in_use = !dlsch_ue_select_tbl_in_use;
    // L2-emulator can work only one eNB.
    //      memcpy(&pre_scd_eNB_UE_stats,&RC.mac[ru->eNB_list[0]->Mod_id]->UE_info.eNB_UE_stats, sizeof(eNB_UE_STATS)*MAX_NUM_CCs*NUMBER_OF_UE_MAX);
    //      memcpy(&pre_scd_activeUE, &RC.mac[ru->eNB_list[0]->Mod_id]->UE_info.active, sizeof(bool)*NUMBER_OF_UE_MAX);
    memcpy(&pre_scd_eNB_UE_stats,&RC.mac[0]->UE_info.eNB_UE_stats, sizeof(eNB_UE_STATS)*MAX_NUM_CCs*NUMBER_OF_UE_MAX);
    memcpy(&pre_scd_activeUE, &RC.mac[0]->UE_info.active, sizeof(bool)*NUMBER_OF_UE_MAX);
    AssertFatal((ret= pthread_mutex_lock(&ru->proc.mutex_pre_scd))==0,"[eNB] error locking proc mutex for eNB pre scd, return %d\n",ret);
    ru->proc.instance_pre_scd++;

    if (ru->proc.instance_pre_scd == 0) {
      if (pthread_cond_signal(&ru->proc.cond_pre_scd) != 0) {
        LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB pre scd\n" );
        exit_fun( "ERROR pthread_cond_signal cond_pre_scd" );
      }
    } else {
      LOG_E( PHY, "[eNB] frame %d subframe %d rxtx busy instance_pre_scd %d\n",
             proc->frame_rx,proc->subframe_rx,ru->proc.instance_pre_scd );
    }

    AssertFatal((ret= pthread_mutex_unlock(&ru->proc.mutex_pre_scd))==0,"[eNB] error unlocking proc mutex for eNB pre scd, return %d\n",ret);
  }

#endif
  AssertFatal((ret= pthread_mutex_lock(&eNB->UL_INFO_mutex))==0,"error locking UL_INFO_mutex, return %d\n",ret);
  eNB->UL_INFO.frame     = proc->frame_rx;
  eNB->UL_INFO.subframe  = proc->subframe_rx;
  eNB->UL_INFO.module_id = eNB->Mod_id;
  eNB->UL_INFO.CC_id     = eNB->CC_id;
  eNB->if_inst->UL_indication(&eNB->UL_INFO, (void*)proc);
  AssertFatal((ret= pthread_mutex_unlock(&eNB->UL_INFO_mutex))==0,"error unlocking UL_INFO_mutex, return %d\n",ret);
  /* this conflict resolution may be totally wrong, to be tested */
  /* CONFLICT RESOLUTION: BEGIN */
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER, 0 );

  if(oai_exit) return(-1);

  if(get_thread_parallel_conf() == PARALLEL_SINGLE_THREAD) {
#ifndef PHY_TX_THREAD
    phy_procedures_eNB_TX(eNB, proc, 1);
#endif
  }

  /* CONFLICT RESOLUTION: what about this release_thread call, has it to be done? if yes, where? */
  //if (release_thread(&proc->mutex_rxtx,&proc->instance_cnt_rxtx,thread_name)<0) return(-1);
  /* CONFLICT RESOLUTION: END */
  stop_meas( &softmodem_stats_rxtx_sf );
  LOG_D(PHY,"%s() Exit proc[rx:%d%d tx:%d%d]\n", __FUNCTION__, proc->frame_rx, proc->subframe_rx, proc->frame_tx, proc->subframe_tx);
  LOG_D(PHY, "rxtx:%lld nfapi:%lld tx:%lld rx:%lld prach:%lld ofdm:%lld ",
        softmodem_stats_rxtx_sf.p_time, nfapi_meas.p_time,
        TICK_TO_US(eNB->phy_proc_tx),
        TICK_TO_US(eNB->phy_proc_rx),
        TICK_TO_US(eNB->rx_prach),
        TICK_TO_US(eNB->ofdm_mod_stats)
       );
  LOG_D(PHY,
        "dlsch[enc:%lld mod:%lld scr:%lld rm:%lld t:%lld i:%lld] rx_dft:%lld ",
        TICK_TO_US(eNB->dlsch_encoding_stats),
        TICK_TO_US(eNB->dlsch_modulation_stats),
        TICK_TO_US(eNB->dlsch_scrambling_stats),
        TICK_TO_US(eNB->dlsch_rate_matching_stats),
        TICK_TO_US(eNB->dlsch_turbo_encoding_stats),
        TICK_TO_US(eNB->dlsch_interleaving_stats),
        TICK_TO_US(eNB->rx_dft_stats));
  LOG_D(PHY," ulsch[ch:%lld freq:%lld dec:%lld demod:%lld ru:%lld ",
        TICK_TO_US(eNB->ulsch_channel_estimation_stats),
        TICK_TO_US(eNB->ulsch_freq_offset_estimation_stats),
        TICK_TO_US(eNB->ulsch_decoding_stats),
        TICK_TO_US(eNB->ulsch_demodulation_stats),
        TICK_TO_US(eNB->ulsch_rate_unmatching_stats));
  LOG_D(PHY, "td:%lld dei:%lld dem:%lld llr:%lld tci:%lld ",
        TICK_TO_US(eNB->ulsch_turbo_decoding_stats),
        TICK_TO_US(eNB->ulsch_deinterleaving_stats),
        TICK_TO_US(eNB->ulsch_demultiplexing_stats),
        TICK_TO_US(eNB->ulsch_llr_stats),
        TICK_TO_US(eNB->ulsch_tc_init_stats));
  LOG_D(PHY, "tca:%lld tcb:%lld tcg:%lld tce:%lld l1:%lld l2:%lld]\n\n",
        TICK_TO_US(eNB->ulsch_tc_alpha_stats),
        TICK_TO_US(eNB->ulsch_tc_beta_stats),
        TICK_TO_US(eNB->ulsch_tc_gamma_stats),
        TICK_TO_US(eNB->ulsch_tc_ext_stats),
        TICK_TO_US(eNB->ulsch_tc_intl1_stats),
        TICK_TO_US(eNB->ulsch_tc_intl2_stats)
       );
  return(0);
}


static void *L1_thread_tx(void *param) {
  L1_proc_t *eNB_proc  = (L1_proc_t *)param;
  L1_rxtx_proc_t *proc = &eNB_proc->L1_proc_tx;
  PHY_VARS_eNB *eNB = eNB_proc->eNB;
  char thread_name[100];
  sprintf(thread_name,"TXnp4_%d\n",&eNB->proc.L1_proc == proc ? 0 : 1);
  thread_top_init(thread_name,1,470000,500000,500000);
  int ret;

  //wait_sync("tx_thread");

  proc->respEncode = eNB->proc.L1_proc.respEncode;
  while (!oai_exit) {
    LOG_D(PHY,"Waiting for TX (IC %d)\n",proc->instance_cnt);

    if (wait_on_condition(&proc->mutex,&proc->cond,&proc->instance_cnt,thread_name)<0) break;

    if (oai_exit) break;

    LOG_D(PHY,"L1_thread_tx: Running for %d.%d\n",proc->frame_tx,proc->subframe_tx);
    // *****************************************
    // TX processing for subframe n+4
    // run PHY TX procedures the one after the other for all CCs to avoid race conditions
    // (may be relaxed in the future for performance reasons)
    // *****************************************
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX1_ENB,proc->subframe_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX1_ENB,proc->subframe_rx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX1_ENB,proc->frame_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX1_ENB,proc->frame_rx);
    LOG_D(PHY,"L1 TX processing %d.%d\n",proc->frame_tx,proc->subframe_tx);
    phy_procedures_eNB_TX(eNB, proc, 1);
    AssertFatal((ret= pthread_mutex_lock( &proc->mutex ))==0,"error locking L1_proc_tx mutex, return %d\n",ret);
    int subframe_tx = proc->subframe_tx;
    int frame_tx    = proc->frame_tx;
    uint64_t timestamp_tx = proc->timestamp_tx;
    proc->instance_cnt = -1;
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_L1_PROC_TX_IC,proc->instance_cnt);
    LOG_D(PHY,"L1 TX signaling done for %d.%d\n",proc->frame_tx,proc->subframe_tx);
    // the thread can now be woken up
    LOG_D(PHY,"L1_thread_tx: signaling completion in %d.%d\n",proc->frame_tx,proc->subframe_tx);

    AssertFatal((ret=pthread_cond_signal(&proc->cond))== 0, "ERROR pthread_cond_signal for eNB TXnp4 thread ret %d\n",ret);
    AssertFatal((ret= pthread_mutex_unlock( &proc->mutex ))==0,"error unlocking L1_proc_tx mutex, return %d\n",ret);


    wakeup_txfh(eNB,proc,frame_tx,subframe_tx,timestamp_tx);
  }

  return 0;
}


/*!
 * \brief The RX UE-specific and TX thread of eNB.
 * \param param is a \ref L1_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void *L1_thread( void *param ) {
  static int eNB_thread_rxtx_status;
  L1_rxtx_proc_t *proc;

  // Working
  if(NFAPI_MODE==NFAPI_MODE_VNF) {
    proc = (L1_rxtx_proc_t *)param;
  } else {
    L1_proc_t *eNB_proc  = (L1_proc_t *)param;
    proc = &eNB_proc->L1_proc;
  }

  PHY_VARS_eNB *eNB = RC.eNB[0][proc->CC_id];


  char thread_name[100];
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  // set default return value
  eNB_thread_rxtx_status = 0;
  sprintf(thread_name,"RXn_TXnp4_%d\n",&eNB->proc.L1_proc == proc ? 0 : 1);
#if 1
  {
    struct sched_param sparam =
    {
      .sched_priority = 79,
    };
    if (pthread_setschedparam(pthread_self(), SCHED_RR, &sparam) != 0)
    {
      LOG_E(PHY,"pthread_setschedparam: %s\n", strerror(errno));
    }
  }
#else
  thread_top_init(thread_name,1,470000,500000,500000);
#endif
  LOG_I(PHY,"thread rxtx created id=%ld\n", syscall(__NR_gettid));

  while (!oai_exit) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RXTX0, 0 );
    T(T_ENB_MASTER_TICK, T_INT(0), T_INT(proc->frame_rx), T_INT(proc->subframe_rx));
    LOG_D(PHY,"L1RX waiting for RU RX\n");

    if (wait_on_condition(&proc->mutex,&proc->cond,&proc->instance_cnt,thread_name)<0) break;

    LOG_D(PHY,"L1RX starting in %d.%d\n",proc->frame_rx,proc->subframe_rx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_CPUID_ENB_THREAD_RXTX,sched_getcpu());
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RXTX0, 1 );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_ENB,proc->subframe_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX0_ENB,proc->subframe_rx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_ENB,proc->frame_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_ENB,proc->frame_rx);

    if (oai_exit) break;

    if (eNB->CC_id==0) {
      if (rxtx(eNB,proc,thread_name) < 0) break;
    }

    LOG_D(PHY,"L1 RX %d.%d done\n",proc->frame_rx,proc->subframe_rx);

    if (release_thread(&proc->mutex,&proc->instance_cnt,thread_name)<0) break;
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_L1_PROC_IC, proc->instance_cnt);
    if (NFAPI_MODE!=NFAPI_MODE_VNF) {
      if(get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT)     wakeup_tx(eNB,proc->frame_rx,proc->subframe_rx,proc->frame_tx,proc->subframe_tx,proc->timestamp_tx);
      else if(get_thread_parallel_conf() == PARALLEL_RU_L1_SPLIT) {
        phy_procedures_eNB_TX(eNB, proc, 1);
        wakeup_txfh(eNB,proc,proc->frame_tx,proc->subframe_tx,proc->timestamp_tx);
      }
    }

  } // while !oai_exit
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RXTX0+(proc->subframe_rx&1), 0 );
  LOG_D(PHY, " *** Exiting eNB thread RXn_TXnp4\n");
  eNB_thread_rxtx_status = 0;
  return &eNB_thread_rxtx_status;
}


void eNB_top(PHY_VARS_eNB *eNB,
             int frame_rx,
             int subframe_rx,
             char *string,
             RU_t *ru) {
  L1_proc_t *proc        = &eNB->proc;
  L1_rxtx_proc_t *L1_proc= &proc->L1_proc;
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  RU_proc_t *ru_proc     = &ru->proc;
  proc->frame_rx         = frame_rx;
  proc->subframe_rx      = subframe_rx;

  if (!oai_exit) {
    T(T_ENB_MASTER_TICK, T_INT(0), T_INT(ru_proc->frame_rx), T_INT(ru_proc->tti_rx));
    L1_proc->timestamp_tx = ru_proc->timestamp_rx + (ru->sf_ahead*fp->samples_per_tti);
    L1_proc->frame_rx     = ru_proc->frame_rx;
    L1_proc->subframe_rx  = ru_proc->tti_rx;
    L1_proc->frame_tx     = (L1_proc->subframe_rx > (9-ru->sf_ahead)) ? (L1_proc->frame_rx+1)&1023 : L1_proc->frame_rx;
    L1_proc->subframe_tx  = (L1_proc->subframe_rx + ru->sf_ahead)%10;
    
    if (rxtx(eNB,L1_proc,string) < 0)
      LOG_E(PHY,"eNB %d CC_id %d failed during execution\n",eNB->Mod_id,eNB->CC_id);  
    ru_proc->timestamp_tx = L1_proc->timestamp_tx;
    ru_proc->tti_tx  = L1_proc->subframe_tx;
    ru_proc->frame_tx     = L1_proc->frame_tx;
  }
}


int wakeup_txfh(PHY_VARS_eNB *eNB,
                L1_rxtx_proc_t *proc,
                int frame_tx,
                int subframe_tx,
                uint64_t timestamp_tx) {
  RU_t *ru;
  RU_proc_t *ru_proc;
  LTE_DL_FRAME_PARMS *fp;
  int waitret,ret;
  LOG_D(PHY,"L1 TX Waking up TX FH %d.%d (IC RU TX %d)\n",frame_tx,subframe_tx,proc->instance_cnt_RUs);
  // grab the information for the RU due to the following wait
  waitret=timedwait_on_condition(&proc->mutex_RUs,&proc->cond_RUs,&proc->instance_cnt_RUs,"wakeup_txfh",1000000);
  AssertFatal(release_thread(&proc->mutex_RUs,&proc->instance_cnt_RUs,"wakeup_txfh")==0, "error releaseing eNB lock on RUs\n");

  if (waitret == ETIMEDOUT) {
    LOG_W(PHY,"Dropping TX slot (%d.%d) because FH is blocked more than 1 subframe times (1ms)\n",frame_tx,subframe_tx);
    AssertFatal((ret=pthread_mutex_lock(&eNB->proc.mutex_RU_tx))==0,"mutex_lock returns %d\n",ret);
    eNB->proc.RU_mask_tx = 0;
    AssertFatal((ret=pthread_mutex_unlock(&eNB->proc.mutex_RU_tx))==0,"mutex_unlock returns %d\n",ret);
    AssertFatal((ret=pthread_mutex_lock(&proc->mutex_RUs))==0,"mutex_lock returns %d\n",ret);
    proc->instance_cnt_RUs = 0;
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_UE,proc->instance_cnt_RUs);
    AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_RUs))==0,"mutex_unlock returns %d\n",ret);
    return(-1);
  }

  for(int ru_id=0; ru_id<eNB->num_RU; ru_id++) {
    ru      = eNB->RU_list[ru_id];
    ru_proc = &ru->proc;
    fp      = ru->frame_parms;

    if (((fp->frame_type == TDD) && (subframe_select(fp,proc->subframe_tx)==SF_UL))||
        (eNB->RU_list[ru_id]->state == RU_SYNC)||
        (eNB->RU_list[ru_id]->wait_cnt>0)) {
      AssertFatal((ret=pthread_mutex_lock(&proc->mutex_RUs))==0, "mutex_lock returns %d\n",ret);
      proc->instance_cnt_RUs = 0;
      AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_RUs))==0, "mutex_unlock returns %d\n",ret);
      continue;//hacking only works when all RU_tx works on the same subframe #TODO: adding mask stuff
    }

    if (ru_proc->instance_cnt_eNBs == 0) {
      LOG_E(PHY,"Frame %d, subframe %d: TX FH thread busy, dropping Frame %d, subframe %d\n", ru_proc->frame_tx, ru_proc->tti_tx, proc->frame_rx, proc->subframe_rx);
      AssertFatal((ret=pthread_mutex_lock(&eNB->proc.mutex_RU_tx))==0,"mutex_lock returns %d\n",ret);
      eNB->proc.RU_mask_tx = 0;
      AssertFatal((ret=pthread_mutex_unlock(&eNB->proc.mutex_RU_tx))==0,"mutex_unlock returns %d\n",ret);
      return(-1);
    }

    AssertFatal((ret = pthread_mutex_lock(&ru_proc->mutex_eNBs))==0,"ERROR pthread_mutex_lock failed on mutex_eNBs L1_thread_tx with ret=%d\n",ret);

    ru_proc->instance_cnt_eNBs = 0;
    ru_proc->timestamp_tx = timestamp_tx;
    ru_proc->tti_tx       = subframe_tx;
    ru_proc->frame_tx     = frame_tx;
    LOG_D(PHY,"L1 TX Waking up TX FH (2) %d.%d\n",frame_tx,subframe_tx);
    // the thread can now be woken up
    AssertFatal(pthread_cond_signal(&ru_proc->cond_eNBs) == 0,
                "[eNB] ERROR pthread_cond_signal for eNB TXnp4 thread\n");
    AssertFatal((ret=pthread_mutex_unlock(&ru_proc->mutex_eNBs))==0,"mutex_unlock returned %d\n",ret);

  }

  return(0);
}


int wakeup_tx(PHY_VARS_eNB *eNB,
              int frame_rx,
              int subframe_rx,
              int frame_tx,
              int subframe_tx,
              uint64_t timestamp_tx) {
  L1_rxtx_proc_t *L1_proc_tx = &eNB->proc.L1_proc_tx;
  int ret;
  LOG_D(PHY,"ENTERED wakeup_tx (IC %d)\n",L1_proc_tx->instance_cnt);
  // check if subframe is a has TX else return
  if (subframe_select(&eNB->frame_parms,subframe_tx) == SF_UL) return 0;  
  AssertFatal((ret = pthread_mutex_lock(&L1_proc_tx->mutex))==0,"mutex_lock returns %d\n",ret);
  LOG_D(PHY,"L1 RX %d.%d Waiting to wake up L1 TX %d.%d (IC L1TX %d)\n",frame_rx,subframe_rx,frame_tx,subframe_tx,L1_proc_tx->instance_cnt);

  while(L1_proc_tx->instance_cnt == 0) {
    pthread_cond_wait(&L1_proc_tx->cond,&L1_proc_tx->mutex);
  }

  LOG_D(PHY,"L1 RX Got signal that TX %d.%d is done\n",L1_proc_tx->frame_tx,L1_proc_tx->subframe_tx);
  L1_proc_tx->instance_cnt = 0;
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_L1_PROC_TX_IC,L1_proc_tx->instance_cnt);
  L1_proc_tx->subframe_rx   = subframe_rx;
  L1_proc_tx->frame_rx      = frame_rx;
  L1_proc_tx->subframe_tx   = subframe_tx;
  L1_proc_tx->frame_tx      = frame_tx;
  L1_proc_tx->timestamp_tx  = timestamp_tx;
  // the thread can now be woken up
  LOG_D(PHY,"L1 RX Waking up L1 TX %d.%d\n",frame_tx,subframe_tx);

  AssertFatal(pthread_cond_signal(&L1_proc_tx->cond) == 0, "ERROR pthread_cond_signal for eNB L1 thread tx\n");
  AssertFatal((ret=pthread_mutex_unlock(&L1_proc_tx->mutex))==0,"mutex_unlock returns %d\n",ret);

  return(0);
}


int wakeup_rxtx(PHY_VARS_eNB *eNB,
                RU_t *ru) {
  L1_proc_t *proc   =&eNB->proc;
  RU_proc_t *ru_proc=&ru->proc;
  L1_rxtx_proc_t *L1_proc=&proc->L1_proc;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  int ret;
  LOG_D(PHY,"ENTERED wakeup_rxtx, %d.%d\n",ru_proc->frame_rx,ru_proc->tti_rx);
  // wake up TX for subframe n+sl_ahead
  // lock the TX mutex and make sure the thread is ready
  AssertFatal((ret=pthread_mutex_lock(&L1_proc->mutex)) == 0,"mutex_lock returns %d\n", ret);

  if (L1_proc->instance_cnt == 0) { // L1_thread is busy so abort the subframe
    AssertFatal((ret=pthread_mutex_unlock( &L1_proc->mutex))==0,"mutex_unlock return %d\n",ret);
    LOG_W(PHY,"L1_thread isn't ready in %d.%d, aborting RX processing\n",ru_proc->frame_rx,ru_proc->tti_rx);
/*    AssertFatal(1==0,"L1_thread isn't ready in %d.%d (L1RX %d.%d), aborting RX, exiting\n",
      ru_proc->frame_rx,ru_proc->tti_rx,L1_proc->frame_rx,L1_proc->subframe_rx);
*/
    return(0);
   
  }

  ++L1_proc->instance_cnt;
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_L1_PROC_IC, L1_proc->instance_cnt);

  // We have just received and processed the common part of a subframe, say n.
  // TS_rx is the last received timestamp (start of 1st slot), TS_tx is the desired
  // transmitted timestamp of the next TX slot (first).
  // The last (TS_rx mod samples_per_frame) was n*samples_per_tti,
  // we want to generate subframe (n+sf_ahead), so TS_tx = TX_rx+sf_ahead*samples_per_tti,
  // and proc->subframe_tx = proc->subframe_rx+sf_ahead
  L1_proc->timestamp_tx = ru_proc->timestamp_rx + (ru->sf_ahead*fp->samples_per_tti);
  L1_proc->frame_rx     = ru_proc->frame_rx;
  L1_proc->subframe_rx  = ru_proc->tti_rx;
  L1_proc->frame_tx     = (L1_proc->subframe_rx > (9-ru->sf_ahead)) ? (L1_proc->frame_rx+1)&1023 : L1_proc->frame_rx;
  L1_proc->subframe_tx  = (L1_proc->subframe_rx + ru->sf_ahead)%10;
  LOG_D(PHY,"wakeup_rxtx: L1_proc->subframe_rx %d, L1_proc->subframe_tx %d, RU %d\n",L1_proc->subframe_rx,L1_proc->subframe_tx,ru->idx);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_WAKEUP_RXTX_RX_RU+ru->idx, L1_proc->frame_rx);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_WAKEUP_RXTX_RX_RU+ru->idx, L1_proc->subframe_rx);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_WAKEUP_RXTX_TX_RU+ru->idx, L1_proc->frame_tx);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_WAKEUP_RXTX_TX_RU+ru->idx, L1_proc->subframe_tx);


  // the thread can now be woken up
  AssertFatal((ret=pthread_cond_signal(&L1_proc->cond))== 0, "ERROR pthread_cond_signal for eNB RXn-TXnp4 thread, ret %d\n",ret);
  AssertFatal((ret=pthread_mutex_unlock( &L1_proc->mutex))==0,"mutex_unlock return %d\n",ret);


  return(0);
}


void wakeup_prach_eNB(PHY_VARS_eNB *eNB,
                      RU_t *ru,
                      int frame,
                      int subframe) {
  L1_proc_t *proc = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int i,ret;

  if (ru!=NULL) {
    AssertFatal((ret=pthread_mutex_lock(&proc->mutex_RU_PRACH))==0,"mutex_lock return %d\n",ret);

    for (i=0; i<eNB->num_RU; i++) {
      if (ru == eNB->RU_list[i] && eNB->RU_list[i]->wait_cnt == 0) {
        LOG_D(PHY,"frame %d, subframe %d: RU %d for eNB %d signals PRACH (mask %x, num_RU %d)\n",frame,subframe,i,eNB->Mod_id,proc->RU_mask_prach,eNB->num_RU);
        proc->RU_mask_prach |= (1<<i);
      } else if (eNB->RU_list[i]->state == RU_SYNC || eNB->RU_list[i]->wait_cnt > 0) {
        proc->RU_mask_prach |= (1<<i);
      }
    }

    if (proc->RU_mask_prach != (1<<eNB->num_RU)-1) {  // not all RUs have provided their information so return
      AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_RU_PRACH))==0,"mutex_unlock return %d\n",ret);
      return;
    } else { // all RUs have provided their information so continue on and wakeup eNB processing
      proc->RU_mask_prach = 0;
      AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_RU_PRACH))==0,"mutex_unlock return %d\n",ret);
    }
  }

  // check if we have to detect PRACH first
  if (is_prach_subframe(fp,frame,subframe)>0) {
    LOG_D(PHY,"Triggering prach processing, frame %d, subframe %d\n",frame,subframe);

    if (proc->instance_cnt_prach == 0) {
      LOG_W(PHY,"[eNB] Frame %d Subframe %d, dropping PRACH\n", frame,subframe);
      return;
    }

    // wake up thread for PRACH RX
    AssertFatal((ret=pthread_mutex_lock(&proc->mutex_prach))==0,"[eNB] ERROR pthread_mutex_lock for eNB PRACH thread %d (IC %d)\n", proc->thread_index, proc->instance_cnt_prach);
    ++proc->instance_cnt_prach;
    // set timing for prach thread
    proc->frame_prach = frame;
    proc->subframe_prach = subframe;

    // the thread can now be woken up
    if (pthread_cond_signal(&proc->cond_prach) != 0) {
      LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB PRACH thread %d\n", proc->thread_index);
      exit_fun( "ERROR pthread_cond_signal" );
      return;
    }

    AssertFatal((ret=pthread_mutex_unlock( &proc->mutex_prach))==0,"mutex_unlock return %d\n",ret);
  }
}



void wakeup_prach_eNB_br(PHY_VARS_eNB *eNB,
                         RU_t *ru,
                         int frame,
                         int subframe) {
  L1_proc_t *proc = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int i,ret;

  if (ru!=NULL) {
    AssertFatal((ret=pthread_mutex_lock(&proc->mutex_RU_PRACH_br))==0,"mutex_lock return %d\n",ret);

    for (i=0; i<eNB->num_RU; i++) {
      if (ru == eNB->RU_list[i]) {
        LOG_D(PHY,"frame %d, subframe %d: RU %d for eNB %d signals PRACH BR (mask %x, num_RU %d)\n",frame,subframe,i,eNB->Mod_id,proc->RU_mask_prach_br,eNB->num_RU);

        if ((proc->RU_mask_prach_br&(1<<i)) > 0)
          LOG_E(PHY,"eNB %d frame %d, subframe %d : previous information (PRACH BR) from RU %d (num_RU %d, mask %x) has not been served yet!\n",
                eNB->Mod_id,frame,subframe,ru->idx,eNB->num_RU,proc->RU_mask_prach_br);

        proc->RU_mask_prach_br |= (1<<i);
      }
    }

    if (proc->RU_mask_prach_br != (1<<eNB->num_RU)-1) {  // not all RUs have provided their information so return
      AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_RU_PRACH_br))==0,"mutex_unlock return %d\n",ret);
      return;
    } else { // all RUs have provided their information so continue on and wakeup eNB processing
      proc->RU_mask_prach_br = 0;
      AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_RU_PRACH_br))==0,"mutex_unlock return %d\n",ret);
    }
  }

  // check if we have to detect PRACH first
  if (is_prach_subframe(fp,frame,subframe)>0) {
    LOG_D(PHY,"Triggering prach br processing, frame %d, subframe %d\n",frame,subframe);

    if (proc->instance_cnt_prach_br == 0) {
      LOG_W(PHY,"[eNB] Frame %d Subframe %d, dropping PRACH BR\n", frame,subframe);
      return;
    }

    // wake up thread for PRACH RX
    AssertFatal((ret=pthread_mutex_lock(&proc->mutex_prach_br))==0,"[eNB] ERROR pthread_mutex_lock for eNB PRACH thread %d (IC %d)\n", proc->thread_index, proc->instance_cnt_prach_br);
    ++proc->instance_cnt_prach_br;
    // set timing for prach thread
    proc->frame_prach_br = frame;
    proc->subframe_prach_br = subframe;

    // the thread can now be woken up
    if (pthread_cond_signal(&proc->cond_prach_br) != 0) {
      LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB PRACH BR thread %d\n", proc->thread_index);
      exit_fun( "ERROR pthread_cond_signal" );
      return;
    }

    AssertFatal((ret=pthread_mutex_unlock( &proc->mutex_prach_br))==0,"mutex_unlock return %d\n",ret);
  }
}


/*!
 * \brief The prach receive thread of eNB.
 * \param param is a \ref L1_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void *eNB_thread_prach( void *param ) {
  static int eNB_thread_prach_status;
  PHY_VARS_eNB *eNB= (PHY_VARS_eNB *)param;
  L1_proc_t *proc = &eNB->proc;
  // set default return value
  eNB_thread_prach_status = 0;
  thread_top_init("eNB_thread_prach",1,500000,1000000,20000000);

  //wait_sync("eNB_thread_prach");

  while (!oai_exit) {
    if (wait_on_condition(&proc->mutex_prach,&proc->cond_prach,&proc->instance_cnt_prach,"eNB_prach_thread") < 0) break;

    if (oai_exit) break;

    LOG_D(PHY,"Running eNB prach procedures\n");
    prach_procedures(eNB,0 );

    if (release_thread(&proc->mutex_prach,&proc->instance_cnt_prach,"eNB_prach_thread") < 0) break;
  }

  LOG_I(PHY, "Exiting eNB thread PRACH\n");
  eNB_thread_prach_status = 0;
  return &eNB_thread_prach_status;
}



/*!
 * \brief The prach receive thread of eNB for BL/CE UEs.
 * \param param is a \ref L1_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void *eNB_thread_prach_br( void *param ) {
  static int eNB_thread_prach_status;
  PHY_VARS_eNB *eNB= (PHY_VARS_eNB *)param;
  L1_proc_t *proc = &eNB->proc;
  // set default return value
  eNB_thread_prach_status = 0;
  thread_top_init("eNB_thread_prach_br",1,500000,1000000,20000000);

  while (!oai_exit) {
    if (wait_on_condition(&proc->mutex_prach_br,&proc->cond_prach_br,&proc->instance_cnt_prach_br,"eNB_prach_thread_br") < 0) break;

    if (oai_exit) break;

    LOG_D(PHY,"Running eNB prach procedures for BL/CE UEs\n");
    prach_procedures(eNB,1);

    if (release_thread(&proc->mutex_prach_br,&proc->instance_cnt_prach_br,"eNB_prach_thread_br") < 0) break;
  }

  LOG_I(PHY, "Exiting eNB thread PRACH BR\n");
  eNB_thread_prach_status = 0;
  return &eNB_thread_prach_status;
}


extern void init_td_thread(PHY_VARS_eNB *);
extern void init_te_thread(PHY_VARS_eNB *);
extern void kill_td_thread(PHY_VARS_eNB *);
extern void kill_te_thread(PHY_VARS_eNB *);


static void *process_stats_thread(void *param) {
  PHY_VARS_eNB     *eNB      = (PHY_VARS_eNB *)param;
  wait_sync("process_stats_thread");

  while (!oai_exit) {
    sleep(1);

    if (opp_enabled == 1) {
      if ( eNB->ulsch_decoding_stats.trials>0)
        print_meas(&eNB->ulsch_decoding_stats,"ulsch_decoding",NULL,NULL);

      if (eNB->dlsch_encoding_stats.trials >0) {
        print_meas(&eNB->dlsch_turbo_encoding_preperation_stats,"dlsch_coding_crc",NULL,NULL);
        print_meas(&eNB->dlsch_turbo_encoding_segmentation_stats,"dlsch_segmentation",NULL,NULL);
        print_meas(&eNB->dlsch_encoding_stats,"dlsch_encoding",NULL,NULL);
        print_meas(&eNB->dlsch_turbo_encoding_signal_stats,"coding_signal",NULL,NULL);
        print_meas(&eNB->dlsch_turbo_encoding_main_stats,"coding_main",NULL,NULL);
        print_meas(&eNB->dlsch_turbo_encoding_stats,"turbo_encoding",NULL,NULL);
        print_meas(&eNB->dlsch_interleaving_stats,"turbo_interleaving",NULL,NULL);
        print_meas(&eNB->dlsch_rate_matching_stats,"turbo_rate_matching",NULL,NULL);
        print_meas(&eNB->dlsch_turbo_encoding_waiting_stats,"coding_wait",NULL,NULL);
        print_meas(&eNB->dlsch_turbo_encoding_wakeup_stats0,"coding_worker_0",NULL,NULL);
        print_meas(&eNB->dlsch_turbo_encoding_wakeup_stats1,"coding_worker_1",NULL,NULL);
      }

      print_meas(&eNB->dlsch_modulation_stats,"dlsch_modulation",NULL,NULL);
    }
  }

  return(NULL);
}

void *L1_stats_thread(void *param) {
  PHY_VARS_eNB     *eNB      = (PHY_VARS_eNB *)param;
  wait_sync("L1_stats_thread");
  FILE *fd;
  while (!oai_exit) {
    sleep(1);
    fd=fopen("L1_stats.log","w");
    AssertFatal(fd!=NULL,"Cannot open L1_stats.log\n");
    dump_I0_stats(fd,eNB);
    dump_ulsch_stats(fd,eNB,eNB->proc.L1_proc_tx.frame_tx);
    dump_uci_stats(fd,eNB,eNB->proc.L1_proc_tx.frame_tx);
    fclose(fd);
  }
  return(NULL);
}

void init_eNB_proc(int inst) {
  /*int i=0;*/
  int CC_id;
  PHY_VARS_eNB *eNB;
  L1_proc_t *proc;
  L1_rxtx_proc_t *L1_proc, *L1_proc_tx;
  pthread_attr_t *attr0=NULL,*attr1=NULL,*attr_prach=NULL;
  pthread_attr_t *attr_prach_br=NULL;
  LOG_I(PHY,"%s(inst:%d) RC.nb_CC[inst]:%d \n",__FUNCTION__,inst,RC.nb_CC[inst]);

  for (CC_id=0; CC_id<RC.nb_CC[inst]; CC_id++) {
    eNB = RC.eNB[inst][CC_id];
    LOG_I(PHY,"Initializing eNB processes instance:%d CC_id %d \n",inst,CC_id);
    proc = &eNB->proc;
    L1_proc                        = &proc->L1_proc;
    L1_proc_tx                     = &proc->L1_proc_tx;
    L1_proc->instance_cnt          = -1;
    L1_proc_tx->instance_cnt       = -1;
    L1_proc->instance_cnt_RUs      = 0;
    L1_proc_tx->instance_cnt_RUs   = 0;
    proc->eNB                      = eNB;
    proc->instance_cnt_prach       = -1;
    proc->instance_cnt_asynch_rxtx = -1;
    proc->instance_cnt_synch       = -1;
    proc->CC_id                    = CC_id;
    proc->first_rx                 =1;
    proc->first_tx                 =1;
    proc->RU_mask_tx               = (1<<eNB->num_RU)-1;
    memset((void *)proc->RU_mask,0,10*sizeof(proc->RU_mask[0]));
    proc->RU_mask_prach            =0;
    pthread_mutex_init( &eNB->UL_INFO_mutex, NULL);
    pthread_mutex_init( &L1_proc->mutex, NULL);
    pthread_mutex_init( &L1_proc_tx->mutex, NULL);
    pthread_cond_init( &L1_proc->cond, NULL);
    pthread_cond_init( &L1_proc_tx->cond, NULL);
    pthread_mutex_init( &L1_proc->mutex_RUs, NULL);
    pthread_mutex_init( &L1_proc_tx->mutex_RUs, NULL);
    pthread_cond_init( &L1_proc->cond_RUs, NULL);
    pthread_cond_init( &L1_proc_tx->cond_RUs, NULL);
    pthread_mutex_init( &proc->mutex_prach, NULL);
    pthread_mutex_init( &proc->mutex_asynch_rxtx, NULL);
    pthread_mutex_init( &proc->mutex_RU,NULL);
    pthread_mutex_init( &proc->mutex_RU_tx,NULL);
    pthread_mutex_init( &proc->mutex_RU_PRACH,NULL);
    pthread_cond_init( &proc->cond_prach, NULL);
    pthread_cond_init( &proc->cond_asynch_rxtx, NULL);
    pthread_attr_init( &proc->attr_prach);
    pthread_attr_init( &proc->attr_asynch_rxtx);
    pthread_attr_init( &L1_proc->attr);
    pthread_attr_init( &L1_proc_tx->attr);
    proc->instance_cnt_prach_br    = -1;
    proc->RU_mask_prach_br=0;
    pthread_mutex_init( &proc->mutex_prach_br, NULL);
    pthread_mutex_init( &proc->mutex_RU_PRACH_br,NULL);
    pthread_cond_init( &proc->cond_prach_br, NULL);
    pthread_attr_init( &proc->attr_prach_br);
#ifndef DEADLINE_SCHEDULER
    attr0       = &L1_proc->attr;
    attr1       = &L1_proc_tx->attr;
    attr_prach  = &proc->attr_prach;
    attr_prach_br  = &proc->attr_prach_br;
    //    attr_td     = &proc->attr_td;
    //    attr_te     = &proc->attr_te;
#endif

    if(get_thread_worker_conf() == WORKER_ENABLE) {
      init_te_thread(eNB);
      init_td_thread(eNB);
    }

    LOG_I(PHY,"eNB->single_thread_flag:%d\n", eNB->single_thread_flag);

    if ((get_thread_parallel_conf() == PARALLEL_RU_L1_SPLIT) && NFAPI_MODE!=NFAPI_MODE_VNF) {
      pthread_create( &L1_proc->pthread, attr0, L1_thread, proc );
    } else if ((get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT) && NFAPI_MODE!=NFAPI_MODE_VNF) {
      pthread_create( &L1_proc->pthread, attr0, L1_thread, proc );
      pthread_create( &L1_proc_tx->pthread, attr1, L1_thread_tx, proc);
    } else if (NFAPI_MODE==NFAPI_MODE_VNF) { // this is neccesary in VNF or L2 FAPI simulator.
      // Original Code from Fujitsu w/ old structure/field name
      //pthread_create( &proc_rxtx[0].pthread_rxtx, attr0, eNB_thread_rxtx, &proc_rxtx[0] );
      //pthread_create( &proc_rxtx[1].pthread_rxtx, attr1, eNB_thread_rxtx, &proc_rxtx[1] );
      pthread_create( &L1_proc->pthread, attr0, L1_thread, L1_proc );
      if (pthread_setname_np(L1_proc->pthread, "oai:enb-L1-rx") != 0)
      {
          LOG_E(PHY, "pthread_setname_np: %s\n", strerror(errno));
      }

      pthread_create( &L1_proc_tx->pthread, attr1, L1_thread, L1_proc_tx);
      if (pthread_setname_np(L1_proc_tx->pthread, "oai:enb-L1-tx") != 0)
      {
          LOG_E(PHY, "pthread_setname_np: %s\n", strerror(errno));
      }
    }

    if (NFAPI_MODE!=NFAPI_MODE_VNF) {
      pthread_create( &proc->pthread_prach, attr_prach, eNB_thread_prach, eNB );
      pthread_create( &proc->pthread_prach_br, attr_prach_br, eNB_thread_prach_br, eNB );
    }

    AssertFatal(proc->instance_cnt_prach == -1,"instance_cnt_prach = %d\n",proc->instance_cnt_prach);

    if (opp_enabled == 1) pthread_create(&proc->process_stats_thread,NULL,process_stats_thread,(void *)eNB);
    pthread_create(&proc->L1_stats_thread,NULL,L1_stats_thread,(void*)eNB);
  }

  //for multiple CCs: setup master and slaves
  /*
     for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
     eNB = PHY_vars_eNB_g[inst][CC_id];

     if (eNB->node_timing == synch_to_ext_device) { //master
     eNB->proc.num_slaves = MAX_NUM_CCs-1;
     eNB->proc.slave_proc = (L1_proc_t**)malloc(eNB->proc.num_slaves*sizeof(L1_proc_t*));

     for (i=0; i< eNB->proc.num_slaves; i++) {
     if (i < CC_id)  eNB->proc.slave_proc[i] = &(PHY_vars_eNB_g[inst][i]->proc);
     if (i >= CC_id)  eNB->proc.slave_proc[i] = &(PHY_vars_eNB_g[inst][i+1]->proc);
     }
     }
     }
  */
  /* setup PHY proc TX sync mechanism */
  pthread_mutex_init(&sync_phy_proc.mutex_phy_proc_tx, NULL);
  pthread_cond_init(&sync_phy_proc.cond_phy_proc_tx, NULL);
  sync_phy_proc.phy_proc_CC_id = 0;
}


/*!
 * \brief Terminate eNB TX and RX threads.
 */
void kill_eNB_proc(int inst) {
  int *status;
  PHY_VARS_eNB *eNB;
  L1_proc_t *proc;
  L1_rxtx_proc_t *L1_proc, *L1_proc_tx;
  int ret;

  for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    eNB=RC.eNB[inst][CC_id];
    proc        = &eNB->proc;
    L1_proc     = &proc->L1_proc;
    L1_proc_tx  = &proc->L1_proc_tx;

    if(get_thread_worker_conf() == WORKER_ENABLE) {
      kill_td_thread(eNB);
      kill_te_thread(eNB);
    }

    LOG_I(PHY, "Killing TX CC_id %d inst %d\n", CC_id, inst );

    if ((get_thread_parallel_conf() == PARALLEL_RU_L1_SPLIT || get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT) && NFAPI_MODE!=NFAPI_MODE_VNF) {
      AssertFatal((ret=pthread_mutex_lock(&L1_proc->mutex))==0,"mutex_lock return %d\n",ret);
      L1_proc->instance_cnt = 0;
      pthread_cond_signal(&L1_proc->cond);
      AssertFatal((ret=pthread_mutex_unlock(&L1_proc->mutex))==0,"mutex_unlock return %d\n",ret);
      AssertFatal((ret=pthread_mutex_lock(&L1_proc_tx->mutex))==0,"mutex_lock return %d\n",ret);
      L1_proc_tx->instance_cnt = 0;
      pthread_cond_signal(&L1_proc_tx->cond);
      AssertFatal((ret=pthread_mutex_unlock(&L1_proc_tx->mutex))==0,"muex_unlock return %d\n",ret);
    }

    AssertFatal((ret=pthread_mutex_lock(&proc->mutex_prach))==0,"mutex_lock return %d\n",ret);
    proc->instance_cnt_prach = 0;
    pthread_cond_signal( &proc->cond_prach );
    AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_prach))==0,"mutex_unlock return %d\n",ret);
    pthread_cond_signal( &proc->cond_asynch_rxtx );
    pthread_cond_broadcast(&sync_phy_proc.cond_phy_proc_tx);
    LOG_D(PHY, "joining pthread_prach\n");
    pthread_join( proc->pthread_prach, (void **)&status );
    LOG_I(PHY, "Destroying prach mutex/cond\n");
    pthread_mutex_destroy( &proc->mutex_prach);
    pthread_cond_destroy( &proc->cond_prach );
    proc->instance_cnt_prach_br = 0;
    pthread_cond_signal( &proc->cond_prach_br );
    pthread_join( proc->pthread_prach_br, (void **)&status );
    pthread_mutex_destroy( &proc->mutex_prach_br);
    pthread_cond_destroy( &proc->cond_prach_br );
    LOG_I(PHY, "Destroying UL_INFO mutex\n");
    pthread_mutex_destroy(&eNB->UL_INFO_mutex);

    if ((get_thread_parallel_conf() == PARALLEL_RU_L1_SPLIT || get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT) && NFAPI_MODE!=NFAPI_MODE_VNF) {
      LOG_I(PHY, "Joining L1_proc mutex/cond\n");
      pthread_join( L1_proc->pthread, (void **)&status );
      LOG_I(PHY, "Joining L1_proc_tx mutex/cond\n");
      pthread_join( L1_proc_tx->pthread, (void **)&status );
    }

    LOG_I(PHY, "Destroying L1_proc mutex/cond\n");
    pthread_mutex_destroy( &L1_proc->mutex );
    pthread_cond_destroy( &L1_proc->cond );
    pthread_mutex_destroy( &L1_proc->mutex_RUs );
    pthread_cond_destroy( &L1_proc->cond_RUs );
    LOG_I(PHY, "Destroying L1_proc_tx mutex/cond\n");
    pthread_mutex_destroy( &L1_proc_tx->mutex );
    pthread_cond_destroy( &L1_proc_tx->cond );
    pthread_mutex_destroy( &L1_proc_tx->mutex_RUs );
    pthread_cond_destroy( &L1_proc_tx->cond_RUs );
    pthread_attr_destroy(&proc->attr_prach);
    pthread_attr_destroy(&proc->attr_asynch_rxtx);
    pthread_attr_destroy(&L1_proc->attr);
    pthread_attr_destroy(&L1_proc_tx->attr);
    pthread_mutex_destroy(&proc->mutex_RU_PRACH_br);
    pthread_attr_destroy(&proc->attr_prach_br);
  }
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
  print_meas(&softmodem_stats_mt, "Main ENB Thread", NULL, NULL);
  print_meas(&softmodem_stats_hw, "HW Acquisation", NULL, NULL);

  for (sfn=0; sfn < 10; sfn++) {
    print_meas(&softmodem_stats_rxtx_sf,"[eNB][total_phy_proc_rxtx]",NULL, NULL);
    print_meas(&softmodem_stats_rx_sf,"[eNB][total_phy_proc_rx]",NULL,NULL);
  }
}


void free_transport(PHY_VARS_eNB *eNB) {
  for (int i=0; i<NUMBER_OF_DLSCH_MAX; i++) {
    LOG_D(PHY, "Freeing Transport Channel Buffers for DLSCH %d\n",i);

    for (int j=0; j<2; j++) free_eNB_dlsch(eNB->dlsch[i][j]);
  }
  for (int i=0;i<NUMBER_OF_ULSCH_MAX;i++) {
    LOG_D(PHY, "Freeing Transport Channel Buffer for ULSCH %d\n",i);
    free_eNB_ulsch(eNB->ulsch[i]);
  }
}


void init_transport(PHY_VARS_eNB *eNB) {
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  LOG_I(PHY, "Initialise transport\n");

  if (NFAPI_MODE!=NFAPI_MODE_VNF) {
    for (int i=0;i<NUMBER_OF_DLSCH_MAX; i++) {
      LOG_I(PHY,"Allocating Transport Channel Buffers for DLSCH %d/%d/%d\n",i,NUMBER_OF_DLSCH_MAX,(i-NUMBER_OF_DLSCH_MAX)<0);

      for (int j=0; j<2; j++) {
        eNB->dlsch[i][j] = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL,0,fp);
        LOG_I(PHY,"eNB->dlsch[%d][%d] %p\n",i,j,eNB->dlsch[i][j]);
        if (!eNB->dlsch[i][j]) {
          LOG_E(PHY,"Can't get eNB dlsch structures for DLSCH %d \n", i);
          exit(-1);
        } else {
          eNB->dlsch[i][j]->rnti=0;
          LOG_D(PHY,"dlsch[%d][%d] => %p rnti:%d\n",i,j,eNB->dlsch[i][j], eNB->dlsch[i][j]->rnti);
        }
      }
    }
    for (int i=0;i<NUMBER_OF_ULSCH_MAX; i++) {

      LOG_I(PHY,"Allocating Transport Channel Buffers for ULSCH %d/%d\n",i,NUMBER_OF_ULSCH_MAX);
      eNB->ulsch[i] = new_eNB_ulsch(MAX_TURBO_ITERATIONS,fp->N_RB_UL, 0);

      if (!eNB->ulsch[i]) {
        LOG_E(PHY,"Can't get eNB ulsch structures\n");
        exit(-1);
      }
    }

    eNB->dlsch_SI  = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
    LOG_D(PHY,"eNB %d.%d : SI %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_SI);
    eNB->dlsch_ra  = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
    LOG_D(PHY,"eNB %d.%d : RA %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_ra);
    eNB->dlsch_MCH = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
    LOG_D(PHY,"eNB %d.%d : MCH %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_MCH);
  }

  eNB->rx_total_gain_dB=130;

  for(int i=0; i<NUMBER_OF_UE_MAX; i++)
    eNB->mu_mimo_mode[i].dl_pow_off = 2;

  eNB->check_for_total_transmissions = 0;
  eNB->check_for_MUMIMO_transmissions = 0;
  eNB->FULL_MUMIMO_transmissions = 0;
  eNB->check_for_SUMIMO_transmissions = 0;
  fp->pucch_config_common.deltaPUCCH_Shift = 1;
  if (eNB->use_DTX == 0) fill_subframe_mask(eNB);
  
}


void init_eNB_afterRU(void) {
  int inst,CC_id,i;
  PHY_VARS_eNB *eNB;
  LOG_I(PHY,"%s() RC.nb_inst:%d\n", __FUNCTION__, RC.nb_inst);

  for (inst=0; inst<RC.nb_inst; inst++) {
    LOG_I(PHY,"RC.nb_CC[inst]:%d\n", RC.nb_CC[inst]);

    for (CC_id=0; CC_id<RC.nb_CC[inst]; CC_id++) {
      LOG_I(PHY,"RC.nb_CC[inst:%d][CC_id:%d]:%p\n", inst, CC_id, RC.eNB[inst][CC_id]);
      eNB                                  =  RC.eNB[inst][CC_id];
     // map antennas and PRACH signals to eNB RX
      LOG_I(PHY,"Mapping RX ports from %d RUs to eNB %d\n",eNB->num_RU,eNB->Mod_id);
      eNB->frame_parms.nb_antennas_rx       = 0;
      LOG_I(PHY,"eNB->num_RU:%d\n", eNB->num_RU);     
      if (NFAPI_MODE==NFAPI_MODE_PNF) AssertFatal(eNB->num_RU>0,"Number of RU attached to eNB %d is      zero\n",eNB->Mod_id);
      for (int ru_id=0; ru_id<eNB->num_RU; ru_id++) 
         eNB->frame_parms.nb_antennas_rx    += eNB->RU_list[ru_id]->nb_rx;
      phy_init_lte_eNB(eNB,0,0);
      LOG_I(PHY,"Overwriting eNB->prach_vars.rxsigF[0]:%p\n", eNB->prach_vars.rxsigF[0]);
      eNB->prach_vars.rxsigF[0] = (int16_t **)malloc16(64*sizeof(int16_t *));

      for (int ce_level=0; ce_level<4; ce_level++) {
        LOG_I(PHY,"Overwriting eNB->prach_vars_br.rxsigF.rxsigF[0]:%p\n", eNB->prach_vars_br.rxsigF[ce_level]);
        eNB->prach_vars_br.rxsigF[ce_level] = (int16_t **)malloc16(64*sizeof(int16_t *));
      }


      for (int ru_id=0,aa=0; ru_id<eNB->num_RU; ru_id++) {

        AssertFatal(eNB->RU_list[ru_id]->common.rxdataF!=NULL,
                    "RU %d : common.rxdataF is NULL\n",
                    eNB->RU_list[ru_id]->idx);
        AssertFatal(eNB->RU_list[ru_id]->prach_rxsigF!=NULL,
                    "RU %d : prach_rxsigF is NULL\n",
                    eNB->RU_list[ru_id]->idx);

        for (i=0; i<eNB->RU_list[ru_id]->nb_rx; aa++,i++) {
          LOG_I(PHY,"Attaching RU %d antenna %d to eNB antenna %d\n",eNB->RU_list[ru_id]->idx,i,aa);
          eNB->prach_vars.rxsigF[0][aa]    =  eNB->RU_list[ru_id]->prach_rxsigF[0][i];

          for (int ce_level=0; ce_level<4; ce_level++)
            eNB->prach_vars_br.rxsigF[ce_level][aa] = eNB->RU_list[ru_id]->prach_rxsigF_br[ce_level][i];

          eNB->common_vars.rxdataF[aa]     =  eNB->RU_list[ru_id]->common.rxdataF[i];
        }
      }

      /* TODO: review this code, there is something wrong.
       * In monolithic mode, we come here with nb_antennas_rx == 0
       * (not tested in other modes).
       */
      if (eNB->frame_parms.nb_antennas_rx < 1) {
        LOG_I(PHY, "%s() ************* DJP ***** eNB->frame_parms.nb_antennas_rx:%d - GOING TO HARD CODE TO 1", __FUNCTION__, eNB->frame_parms.nb_antennas_rx);
        eNB->frame_parms.nb_antennas_rx = 1;
      } else {
        //LOG_I(PHY," Delete code\n");
      }

      if (eNB->frame_parms.nb_antennas_tx < 1) {
        LOG_I(PHY, "%s() ************* DJP ***** eNB->frame_parms.nb_antennas_tx:%d - GOING TO HARD CODE TO 1", __FUNCTION__, eNB->frame_parms.nb_antennas_tx);
        eNB->frame_parms.nb_antennas_tx = 1;
      } else {
        //LOG_I(PHY," Delete code\n");
      }

      AssertFatal(eNB->frame_parms.nb_antennas_rx >0,
                  "inst %d, CC_id %d : nb_antennas_rx %d\n",inst,CC_id,eNB->frame_parms.nb_antennas_rx);
      LOG_I(PHY,"inst %d, CC_id %d : nb_antennas_rx %d\n",inst,CC_id,eNB->frame_parms.nb_antennas_rx);
      init_transport(eNB);
      //init_precoding_weights(RC.eNB[inst][CC_id]);
    }

    init_eNB_proc(inst);
  }

  for (int ru_id=0; ru_id<RC.nb_RU; ru_id++) {
    AssertFatal(RC.ru[ru_id]!=NULL,"ru_id %d is null\n",ru_id);
    RC.ru[ru_id]->wakeup_rxtx         = wakeup_rxtx;
    RC.ru[ru_id]->wakeup_prach_eNB    = wakeup_prach_eNB;
    RC.ru[ru_id]->wakeup_prach_eNB_br = wakeup_prach_eNB_br;
    RC.ru[ru_id]->eNB_top             = eNB_top;
  }
}


void init_eNB(int single_thread_flag,
              int wait_for_sync) {
  int CC_id;
  int inst;
  PHY_VARS_eNB *eNB;
  LOG_I(PHY,"[lte-softmodem.c] eNB structure about to allocated RC.nb_L1_inst:%d RC.nb_L1_CC[0]:%d\n",RC.nb_L1_inst,RC.nb_L1_CC[0]);

  if (RC.eNB == NULL) RC.eNB = (PHY_VARS_eNB ** *) malloc(RC.nb_L1_inst*sizeof(PHY_VARS_eNB **));

  LOG_I(PHY,"[lte-softmodem.c] eNB structure RC.eNB allocated\n");

  for (inst=0; inst<RC.nb_L1_inst; inst++) {
    if (RC.eNB[inst] == NULL) RC.eNB[inst] = (PHY_VARS_eNB **) malloc(RC.nb_CC[inst]*sizeof(PHY_VARS_eNB *));

    for (CC_id=0; CC_id<RC.nb_L1_CC[inst]; CC_id++) {
      if (RC.eNB[inst][CC_id] == NULL) 
         RC.eNB[inst][CC_id] = (PHY_VARS_eNB *) calloc(1,sizeof(PHY_VARS_eNB));

      eNB                     = RC.eNB[inst][CC_id];
      eNB->abstraction_flag   = 0;
      eNB->single_thread_flag = single_thread_flag;
      LOG_I(PHY,"Initializing eNB %d CC_id %d single_thread_flag:%d\n",inst,CC_id,single_thread_flag);
      LOG_I(PHY,"Initializing eNB %d CC_id %d\n",inst,CC_id);
      LOG_I(PHY,"Registering with MAC interface module\n");
      AssertFatal((eNB->if_inst         = IF_Module_init(inst))!=NULL,"Cannot register interface");
      eNB->if_inst->schedule_response   = schedule_response;
      eNB->if_inst->PHY_config_req      = phy_config_request;
      eNB->if_inst->PHY_config_update_sib2_req      = phy_config_update_sib2_request;
      eNB->if_inst->PHY_config_update_sib13_req      = phy_config_update_sib13_request;
      memset((void *)&eNB->UL_INFO,0,sizeof(eNB->UL_INFO));
      memset((void *)&eNB->Sched_INFO,0,sizeof(eNB->Sched_INFO));
      LOG_I(PHY,"Setting indication lists\n");
      eNB->UL_INFO.rx_ind.rx_indication_body.rx_pdu_list   = eNB->rx_pdu_list;
      eNB->UL_INFO.crc_ind.crc_indication_body.crc_pdu_list = eNB->crc_pdu_list;
      eNB->UL_INFO.sr_ind.sr_indication_body.sr_pdu_list = eNB->sr_pdu_list;
      eNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list = eNB->harq_pdu_list;
      eNB->UL_INFO.cqi_ind.cqi_indication_body.cqi_pdu_list = eNB->cqi_pdu_list;
      eNB->UL_INFO.cqi_ind.cqi_indication_body.cqi_raw_pdu_list = eNB->cqi_raw_pdu_list;
      eNB->prach_energy_counter = 0;
    }
  }

  LOG_I(PHY,"[lte-softmodem.c] eNB structure allocated\n");
}


void stop_eNB(int nb_inst) {
  for (int inst=0; inst<nb_inst; inst++) {
    LOG_I(PHY,"Killing eNB %d processing threads\n",inst);
    kill_eNB_proc(inst);
  }
}
