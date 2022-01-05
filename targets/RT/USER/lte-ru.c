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

/*! \file lte-ru.c
 * \brief Top-level threads for RU entity
 * \author R. Knopp, F. Kaltenberger, Navid Nikaein
 * \date 2019
 * \version 0.1
 * \company Eurecom
 * \email: {knopp, florian.kaltenberger, navid.nikaein}@eurecom.fr
 * \note
 * \warning
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <sys/sysinfo.h>
#include "rt_wrapper.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all

#include "assertions.h"
#include "msc.h"
#include "PHY/defs_common.h"
#include "PHY/types.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "SCHED/sched_common.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "targets/ARCH/COMMON/common_lib.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"
//#include "PHY/TOOLS/time_meas.h"

/* these variables have to be defined before including ENB_APP/enb_paramdef.h */
static int DEFBANDS[] = {7};
static int DEFENBS[] = {0};
static int DEFBFW[] = {0x00007fff};

#include "ENB_APP/enb_paramdef.h"
#include "common/config/config_userapi.h"

#include "SIMULATION/ETH_TRANSPORT/proto.h"

#include "T.h"

#include "executables/softmodem-common.h"

#define MBMS_EXPERIMENTAL

extern volatile int oai_exit;
extern clock_source_t clock_source;
#include "executables/thread-common.h"
//extern PARALLEL_CONF_t get_thread_parallel_conf(void);
//extern WORKER_CONF_t   get_thread_worker_conf(void);
extern void phy_init_RU(RU_t *);

void prach_procedures(PHY_VARS_eNB *eNB,int br_flag);

void stop_RU(RU_t **rup,int nb_ru);

void do_ru_synch(RU_t *ru);

void configure_ru(int idx,
                  void *arg);

void configure_rru(int idx,
                   void *arg);

void reset_proc(RU_t *ru);
int connect_rau(RU_t *ru);

void wait_eNBs(void);

const char ru_states[6][9] = {"RU_IDLE","RU_CONFIG","RU_READY","RU_RUN","RU_ERROR","RU_SYNC"};

extern const char NB_functions[7][20];
extern const char NB_timing[2][20];

extern uint16_t sf_ahead;

extern const char ru_if_types[MAX_RU_IF_TYPES][20];


#if defined(PRE_SCD_THREAD)
#include "common/ran_context.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "openair2/LAYER2/MAC/mac_extern.h"
  extern uint8_t dlsch_ue_select_tbl_in_use;
  void init_ru_vnf(void);
  extern RAN_CONTEXT_t RC;
#endif

RU_t **RCconfig_RU(int nb_RU,int nb_L1_inst,PHY_VARS_eNB ***eNB,uint64_t *ru_mask,pthread_mutex_t *ru_mutex,pthread_cond_t *ru_cond);

/*************************************************************/
/* Functions to attach and configure RRU                     */


/*************************************************************/
/* Southbound Fronthaul functions, RCC/RAU                   */

// southbound IF5 fronthaul for 16-bit OAI format
static inline void fh_if5_south_out(RU_t *ru,int frame, int subframe, uint64_t timestamp) {
  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );

  ru->south_out_cnt++;

  send_IF5(ru, timestamp, subframe, &ru->seqno, IF5_RRH_GW_DL);
}


// southbound IF4p5 fronthaul
static inline void fh_if4p5_south_out(RU_t *ru,
                                      int frame,
                                      int subframe,
                                      uint64_t timestamp) {
  if (ru->idx == 0) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );

  LOG_D(PHY,"ENTERED fh_if4p5_south_out   Sending IF4p5 for frame %d subframe %d ru %d\n",ru->proc.frame_tx,ru->proc.tti_tx,ru->idx);

  if (subframe_select(ru->frame_parms, subframe)!=SF_UL) {
    send_IF4p5(ru, frame, subframe, IF4p5_PDLFFT);
    ru->south_out_cnt++;
    LOG_D(PHY,"south_out_cnt %d\n",ru->south_out_cnt);
  }

  /*if (ru->idx == 0 || ru->idx == ru) {
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU+ru->idx, ru->proc.frame_tx );
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_RU+ru->idx, ru->proc.subframe_tx );
    }*/
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_IF4P5_SOUTH_OUT_RU+ru->idx, ru->proc.frame_tx);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_IF4P5_SOUTH_OUT_RU+ru->idx, ru->proc.tti_tx);
}


/*************************************************************/
/* Input Fronthaul from south RCC/RAU                        */

// Synchronous if5 from south
void fh_if5_south_in(RU_t *ru,
                     int *frame,
                     int *subframe) {
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  RU_proc_t *proc = &ru->proc;
  recv_IF5(ru, &proc->timestamp_rx, *subframe, IF5_RRH_GW_UL);
  proc->frame_rx    = (proc->timestamp_rx / (fp->samples_per_tti*10))&1023;
  proc->tti_rx = (proc->timestamp_rx / fp->samples_per_tti)%10;
  
  if (proc->first_rx == 0) {
    if (proc->tti_rx != *subframe) {
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->tti_rx %d, subframe %d), resynching\n",proc->tti_rx,*subframe);
      *frame=proc->frame_rx;
      *subframe=proc->tti_rx;
    }

    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",proc->frame_rx,*frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->tti_rx;
  }

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );
}


// Synchronous if4p5 from south
void fh_if4p5_south_in(RU_t *ru,
                       int *frame,
                       int *subframe) {
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  RU_proc_t *proc = &ru->proc;
  int f,sf;
  uint16_t packet_type;
  uint32_t symbol_number=0;
  uint32_t symbol_mask_full;
  int pultick_received=0;

  if ((fp->frame_type == TDD) && (subframe_select(fp,*subframe)==SF_S))
    symbol_mask_full = (1<<fp->ul_symbols_in_S_subframe)-1;
  else
    symbol_mask_full = (1<<fp->symbols_per_tti)-1;

  LOG_D(PHY,"fh_if4p5_south_in: RU %d, frame %d, subframe %d, ru %d, mask %x\n",ru->idx,*frame,*subframe,ru->idx,proc->symbol_mask[*subframe]);
  //AssertFatal(proc->symbol_mask[*subframe]==0 || proc->symbol_mask[*subframe]>=symbol_mask_full,"rx_fh_if4p5: proc->symbol_mask[%d] = %x\n",*subframe,proc->symbol_mask[*subframe]); // >= because PULTICK for S-subframe could have been received during normal subframe

  if (proc->symbol_mask[*subframe]<symbol_mask_full) { // this is normal case, if not true then we received a PULTICK before the previous subframe was finished
    do {
      recv_IF4p5(ru, &f, &sf, &packet_type, &symbol_number);
      LOG_D(PHY,"fh_if4p5_south_in (%s/%d): RU %d, frame %d, subframe %d, f %d, sf %d, symbol %d\n",packet_type == IF4p5_PULFFT ? "PULFFT" : "PULTICK",packet_type,ru->idx,*frame,*subframe,f,sf,symbol_number);

      if (oai_exit == 1 || ru->cmd== STOP_RU) break;

      if (packet_type == IF4p5_PULFFT) proc->symbol_mask[sf] = proc->symbol_mask[sf] | (1<<symbol_number);
      else if (packet_type == IF4p5_PULTICK) {
        proc->symbol_mask[sf] = 0xffff;
        pultick_received++;
        /*
                 if ((proc->first_rx==0) && (f!=*frame)) LOG_E(PHY,"rx_fh_if4p5: PULTICK received frame %d != expected %d (RU %d) \n",f,*frame, ru->idx);
                 else if ((proc->first_rx==0) && (sf!=*subframe)) LOG_E(PHY,"rx_fh_if4p5: PULTICK received subframe %d != expected %d (first_rx %d)\n",sf,*subframe,proc->first_rx);
                 else break; */
        //if (f==*frame || sf==*subframe) break;
      } else if (packet_type == IF4p5_PRACH) {
        // nothing in RU for RAU
      }

      LOG_D(PHY,"rx_fh_if4p5 for RU %d: subframe %d, sf %d, symbol %d, symbol mask %x\n",ru->idx,*subframe,sf,symbol_number,proc->symbol_mask[sf]);
    } while(proc->symbol_mask[*subframe] < symbol_mask_full);
  } else {
    f = *frame;
    sf = *subframe;
  }

  //calculate timestamp_rx, timestamp_tx based on frame and subframe
  proc->tti_rx       = sf;
  proc->frame_rx     = f;
  proc->timestamp_rx = ((proc->frame_rx * 10)  + proc->tti_rx ) * fp->samples_per_tti ;

  //  proc->timestamp_tx = proc->timestamp_rx +  (4*fp->samples_per_tti);
  if (get_thread_parallel_conf() == PARALLEL_SINGLE_THREAD) {
    proc->tti_tx   = (sf+sf_ahead)%10;
    proc->frame_tx = (sf>(9-sf_ahead)) ? (f+1)&1023 : f;
  }

  LOG_D(PHY,"Setting proc for (%d,%d)\n",sf,f);

  if (proc->first_rx == 0) {
    if (proc->tti_rx != *subframe) {
      LOG_E(PHY,"Received Timestamp (IF4p5) doesn't correspond to the time we think it is (proc->tti_rx %d, subframe %d, symbol_mask %x)\n", proc->tti_rx, *subframe, proc->symbol_mask[*subframe]);
      *subframe=sf;
      //exit_fun("Exiting");
    }

    if (ru->cmd != WAIT_RESYNCH && proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp (IF4p5) doesn't correspond to the time we think it is (proc->frame_rx %d frame %d,symbol_mask %x\n",
            proc->frame_rx,*frame,proc->symbol_mask[*subframe]);
      //exit_fun("Exiting");
    } else if (ru->cmd == WAIT_RESYNCH && proc->frame_rx != *frame) {
      ru->cmd=EMPTY;
      *frame=proc->frame_rx;
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->tti_rx;
  }

  /*if (ru->idx == 0 || ru->idx == 1) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_RU+ru->idx, f );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX0_RU+ru->idx, sf );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_RU, f );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_RX0_RU, sf );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, proc->frame_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_TX0_RU, proc->tti_tx );
  }*/
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_IF4P5_SOUTH_IN_RU+ru->idx,f);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_IF4P5_SOUTH_IN_RU+ru->idx,sf);
  proc->symbol_mask[sf] = 0;
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff);
  LOG_D(PHY,"RU %d: fh_if4p5_south_in returning ...\n",ru->idx);
  //  usleep(100);
}


// Dummy FH from south for getting synchronization from master RU
void fh_slave_south_in(RU_t *ru,
                       int *frame,
                       int *subframe) {
  // This case is for synchronization to another thread
  // it just waits for an external event.  The actual rx_fh is handle by the asynchronous RX thread
  RU_proc_t *proc=&ru->proc;

  if (wait_on_condition(&proc->mutex_FH,&proc->cond_FH,&proc->instance_cnt_FH,"fh_slave_south_in") < 0)
    return;

  release_thread(&proc->mutex_FH,&proc->instance_cnt_FH,"rx_fh_slave_south_in");
}


// asynchronous inbound if4p5 fronthaul from south
void fh_if4p5_south_asynch_in(RU_t *ru,
                              int *frame,
                              int *subframe) {
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  RU_proc_t *proc        = &ru->proc;
  uint16_t packet_type;
  uint32_t symbol_number,symbol_mask,prach_rx;
  uint32_t got_prach_info=0;
  symbol_number = 0;
  symbol_mask   = (1<<fp->symbols_per_tti)-1;
  prach_rx      = 0;

  do {   // Blocking, we need a timeout on this !!!!!!!!!!!!!!!!!!!!!!!
    recv_IF4p5(ru, &proc->frame_rx, &proc->tti_rx, &packet_type, &symbol_number);

    if (ru->cmd == STOP_RU) break;

    // grab first prach information for this new subframe
    if (got_prach_info==0) {
      prach_rx       = is_prach_subframe(fp, proc->frame_rx, proc->tti_rx);
      got_prach_info = 1;
    }

    if (proc->first_rx != 0) {
      *frame = proc->frame_rx;
      *subframe = proc->tti_rx;
      proc->first_rx = 0;
    } else {
      if (proc->frame_rx != *frame) {
        LOG_E(PHY,"frame_rx %d is not what we expect %d\n",proc->frame_rx,*frame);
        exit_fun("Exiting");
      }

      if (proc->tti_rx != *subframe) {
        LOG_E(PHY,"tti_rx %d is not what we expect %d\n",proc->tti_rx,*subframe);
        exit_fun("Exiting");
      }
    }

    if      (packet_type == IF4p5_PULFFT)       symbol_mask &= (~(1<<symbol_number));
    else if (packet_type == IF4p5_PRACH)        prach_rx    &= (~0x1);
    else if (packet_type == IF4p5_PRACH_BR_CE0) prach_rx    &= (~0x2);
    else if (packet_type == IF4p5_PRACH_BR_CE1) prach_rx    &= (~0x4);
    else if (packet_type == IF4p5_PRACH_BR_CE2) prach_rx    &= (~0x8);
    else if (packet_type == IF4p5_PRACH_BR_CE3) prach_rx    &= (~0x10);
  } while( (symbol_mask > 0) || (prach_rx >0));   // haven't received all PUSCH symbols and PRACH information
}



/*************************************************************/
/* Input Fronthaul from North RRU                            */

// RRU IF4p5 TX fronthaul receiver. Assumes an if_device on input and if or rf device on output
// receives one subframe's worth of IF4p5 OFDM symbols and OFDM modulates
void fh_if4p5_north_in(RU_t *ru,
                       int *frame,
                       int *subframe) {
  uint32_t symbol_number=0;
  uint32_t symbol_mask, symbol_mask_full;
  uint16_t packet_type;
  /// **** incoming IF4p5 from remote RCC/RAU **** ///
  symbol_number = 0;
  symbol_mask = 0;
  symbol_mask_full = (1<<ru->frame_parms->symbols_per_tti)-1;
  LOG_D(PHY,"fh_if4p5_north_in: frame %d, subframe %d\n",*frame,*subframe);

  do {
    recv_IF4p5(ru, frame, subframe, &packet_type, &symbol_number);
    symbol_mask = symbol_mask | (1<<symbol_number);
  } while (symbol_mask != symbol_mask_full);

  ru->north_in_cnt++;

  // dump VCD output for first RU in list
  if (ru->idx == 0) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, *frame );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_TX0_RU, *subframe );
  }
}

void fh_if5_north_asynch_in(RU_t *ru,
                            int *frame,
                            int *subframe) {
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  RU_proc_t *proc        = &ru->proc;
  int tti_tx,frame_tx;
  openair0_timestamp timestamp_tx;
  recv_IF5(ru, &timestamp_tx, *subframe, IF5_RRH_GW_DL);
  //      LOG_I(PHY,"Received subframe %d (TS %llu) from RCC\n",tti_tx,timestamp_tx);
  tti_tx = (timestamp_tx/fp->samples_per_tti)%10;
  frame_tx    = (timestamp_tx/(fp->samples_per_tti*10))&1023;
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, proc->frame_tx );
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_TX0_RU, proc->tti_tx );

  if (proc->first_tx != 0) {
    *subframe = tti_tx;
    *frame    = frame_tx;
    proc->first_tx = 0;
  } else {
    AssertFatal(tti_tx == *subframe, "tti_tx %d is not what we expect %d\n",tti_tx,*subframe);
    AssertFatal(frame_tx == *frame, "frame_tx %d is not what we expect %d\n",frame_tx,*frame);
  }

  ru->north_in_cnt++;
}


void fh_if4p5_north_asynch_in(RU_t *ru,
                              int *frame,
                              int *subframe) {
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  RU_proc_t *proc        = &ru->proc;
  uint16_t packet_type;
  uint32_t symbol_number = 0, symbol_mask = 0, symbol_mask_full;
  int tti_tx, frame_tx, ret;
  LOG_D(PHY, "%s(ru:%p frame, subframe)\n", __FUNCTION__, ru);
  symbol_mask_full = ((subframe_select(fp,*subframe) == SF_S) ? (1<<fp->dl_symbols_in_S_subframe) : (1<<fp->symbols_per_tti))-1;
  LOG_D(PHY,"fh_if4p5_north_asynch_in: RU %d, frame %d, subframe %d\n", ru->idx, *frame, *subframe);

  do {
    recv_IF4p5(ru, &frame_tx, &tti_tx, &packet_type, &symbol_number);
    LOG_D(PHY,"income frame.subframe %d.%d, our frame.subframe.symbol_number %d.%d.%d (symbol mask %x)\n",frame_tx,tti_tx,*frame,*subframe,symbol_number,symbol_mask);

    if (ru->cmd == STOP_RU) {
      LOG_E(PHY,"Got STOP_RU\n");
      AssertFatal((ret=pthread_mutex_lock(&proc->mutex_ru))==0,"mutex_lock returns %d\n",ret);
      proc->instance_cnt_ru = -1;
      AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_ru))==0,"mutex_unlock returns %d\n",ret);
      ru->cmd=STOP_RU;
      return;
    }

    if ((subframe_select(fp,tti_tx) == SF_DL) && (symbol_number == 0)) start_meas(&ru->rx_fhaul);

    LOG_D(PHY,"subframe %d (%d): frame %d, subframe %d, symbol %d\n", *subframe, subframe_select(fp,*subframe), frame_tx, tti_tx, symbol_number);

    if (proc->first_tx != 0) {
      *frame    = frame_tx;
      *subframe = tti_tx;
      proc->first_tx = 0;
      symbol_mask_full = ((subframe_select(fp,*subframe) == SF_S) ? (1<<fp->dl_symbols_in_S_subframe) : (1<<fp->symbols_per_tti))-1;
    } else {
      /* AssertFatal(frame_tx == *frame, "frame_tx %d is not what we expect %d\n",frame_tx,*frame);
       AssertFatal(subframe_tx == *subframe, "In frame_tx %d : subframe_tx %d is not what we expect %d\n",frame_tx,subframe_tx,*subframe);
      */
      *frame    = frame_tx;
      *subframe = tti_tx;
    }

    if (packet_type == IF4p5_PDLFFT) {
      symbol_mask = symbol_mask | (1<<symbol_number);
    } else AssertFatal(1==0,"Illegal IF4p5 packet type (should only be IF4p5_PDLFFT got %d\n",packet_type);
  } while (symbol_mask != symbol_mask_full);

  if (subframe_select(fp,tti_tx) == SF_DL) stop_meas(&ru->rx_fhaul);

  ru->north_in_cnt++;
  proc->tti_tx   = tti_tx;
  proc->frame_tx = frame_tx;

  if ((frame_tx == 0)&&(tti_tx == 0)) proc->frame_tx_unwrap += 1024;

  proc->timestamp_tx = ((((uint64_t)frame_tx + (uint64_t)proc->frame_tx_unwrap) * 10) + (uint64_t)tti_tx) * (uint64_t)fp->samples_per_tti;
  LOG_D(PHY,"RU %d/%d TST %llu, frame %d, subframe %d\n",ru->idx,0,(long long unsigned int)proc->timestamp_tx,frame_tx,tti_tx);

  // dump VCD output for first RU in list
  if (ru->idx == 0) {
    /*VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, frame_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_TX0_RU, tti_tx );*/
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_IF4P5_NORTH_ASYNCH_IN, frame_tx);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_IF4P5_NORTH_ASYNCH_IN, tti_tx);
  }

  if (ru->feptx_ofdm) ru->feptx_ofdm(ru, frame_tx, tti_tx);

  if (ru->fh_south_out) ru->fh_south_out(ru, frame_tx, tti_tx, proc->timestamp_tx);
}


void fh_if5_north_out(RU_t *ru) {
  RU_proc_t *proc=&ru->proc;
  uint8_t seqno=0;
  /// **** send_IF5 of rxdata to BBU **** ///
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF5, 1 );
  send_IF5(ru, proc->timestamp_rx, proc->tti_rx, &seqno, IF5_RRH_GW_UL);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_SEND_IF5, 0 );
}


// RRU IF4p5 northbound interface (RX)
void fh_if4p5_north_out(RU_t *ru) {
  RU_proc_t *proc        = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  const int subframe     = proc->tti_rx;

  if (ru->idx==0) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_RX0_RU, proc->tti_rx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_IF4P5_NORTH_OUT, proc->tti_rx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_IF4P5_NORTH_OUT, proc->frame_rx );
  }

  LOG_D(PHY,"fh_if4p5_north_out: Sending IF4p5_PULFFT SFN.SF %d.%d\n", proc->frame_rx, proc->tti_rx);

  if ((fp->frame_type == TDD) && (subframe_select(fp,subframe)!=SF_UL)) {
    /// **** in TDD during DL send_IF4 of ULTICK to RCC **** ///
    send_IF4p5(ru, proc->frame_rx, proc->tti_rx, IF4p5_PULTICK);
    ru->north_out_cnt++;
    return;
  }

  start_meas(&ru->tx_fhaul);
  send_IF4p5(ru, proc->frame_rx, proc->tti_rx, IF4p5_PULFFT);
  ru->north_out_cnt++;
  stop_meas(&ru->tx_fhaul);
}

/* add fail safe for late command */
typedef enum {
  STATE_BURST_NORMAL    = 0,
  STATE_BURST_TERMINATE = 1,
  STATE_BURST_STOP_1    = 2,
  STATE_BURST_STOP_2    = 3,
  STATE_BURST_RESTART   = 4,
} late_control_e;

volatile late_control_e late_control=STATE_BURST_NORMAL;

/* add fail safe for late command end */

static void *emulatedRF_thread(void *param) {
  RU_proc_t *proc = (RU_proc_t *) param;
  int microsec = 500; // length of time to sleep, in miliseconds
  int numerology = 0 ;
  struct timespec req = {0};
  req.tv_sec = 0;
  req.tv_nsec = (numerology>0)? ((microsec * 1000L)/numerology):(microsec * 1000L)*2;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(1,&cpuset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  int policy;
  int ret;
  struct sched_param sparam;
  memset(&sparam, 0, sizeof(sparam));
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
  policy = SCHED_FIFO ;
  pthread_setschedparam(pthread_self(), policy, &sparam);
  wait_sync("emulatedRF_thread");

  while(!oai_exit) {
    nanosleep(&req, (struct timespec *)NULL);

    if(proc->emulate_rf_busy ) {
      LOG_E(PHY,"rf being delayed in emulated RF\n");
    }

    proc->emulate_rf_busy = 1;
    AssertFatal((ret=pthread_mutex_lock(&proc->mutex_emulateRF))==0,"mutex_lock returns %d\n",ret);
    ++proc->instance_cnt_emulateRF;
    pthread_cond_signal(&proc->cond_emulateRF);
    AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_emulateRF))==0,"mutex_unlock returns %d\n",ret);
  }

  return 0;
}


void rx_rf(RU_t *ru,
           int *frame,
           int *subframe) {
  RU_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  void *rxp[ru->nb_rx];
  unsigned int rxs;
  int i;
  int resynch=0;
  openair0_timestamp ts=0,old_ts=0;

  for (i=0; i<ru->nb_rx; i++)
    rxp[i] = (void *)&ru->common.rxdata[i][*subframe*fp->samples_per_tti];

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );
  old_ts = proc->timestamp_rx;

  if(ru->emulate_rf) {
    wait_on_condition(&proc->mutex_emulateRF,&proc->cond_emulateRF,&proc->instance_cnt_emulateRF,"emulatedRF_thread");
    release_thread(&proc->mutex_emulateRF,&proc->instance_cnt_emulateRF,"emulatedRF_thread");
    rxs = fp->samples_per_tti;
  } else {
    rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
                                     &ts,
                                     rxp,
                                     fp->samples_per_tti,
                                     ru->nb_rx);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );
  ru->south_in_cnt++;
  LOG_D(PHY,"south_in_cnt %d\n",ru->south_in_cnt);

  if (ru->cmd==RU_FRAME_RESYNCH) {
    LOG_I(PHY,"Applying frame resynch %d => %d\n",*frame,ru->cmdval);

    if (proc->frame_rx>ru->cmdval) ru->ts_offset += (proc->frame_rx - ru->cmdval)*fp->samples_per_tti*10;
    else ru->ts_offset -= (-proc->frame_rx + ru->cmdval)*fp->samples_per_tti*10;

    *frame = ru->cmdval;
    ru->cmd=EMPTY;
    resynch=1;
  }

  if(get_softmodem_params()->emulate_rf) {
    proc->timestamp_rx = old_ts + fp->samples_per_tti;
  } else {
    proc->timestamp_rx = ts-ru->ts_offset;
  }

  //  AssertFatal(rxs == fp->samples_per_tti,
  //        "rx_rf: Asked for %d samples, got %d from SDR\n",fp->samples_per_tti,rxs);
  if(rxs != fp->samples_per_tti) {
    LOG_E(PHY,"rx_rf: Asked for %d samples, got %d from SDR\n",fp->samples_per_tti,rxs);
    late_control=STATE_BURST_TERMINATE;
  }

  if (proc->first_rx == 1) {
    ru->ts_offset = proc->timestamp_rx;
    proc->timestamp_rx = 0;
  } else if (resynch==0 && (proc->timestamp_rx - old_ts != fp->samples_per_tti)) {
    LOG_D(PHY,"rx_rf: rfdevice timing drift of %"PRId64" samples (ts_off %"PRId64")\n",proc->timestamp_rx - old_ts - fp->samples_per_tti,ru->ts_offset);
    ru->ts_offset += (proc->timestamp_rx - old_ts - fp->samples_per_tti);
    proc->timestamp_rx = ts-ru->ts_offset;
  }

  proc->frame_rx     = (proc->timestamp_rx / (fp->samples_per_tti*10))&1023;
  proc->tti_rx  = (proc->timestamp_rx / fp->samples_per_tti)%10;
  // synchronize first reception to frame 0 subframe 0

  if (get_thread_parallel_conf() == PARALLEL_SINGLE_THREAD && ru->fh_north_asynch_in == NULL) {
#ifdef PHY_TX_THREAD
    proc->timestamp_phy_tx = proc->timestamp_rx+((sf_ahead-1)*fp->samples_per_tti);
    proc->subframe_phy_tx  = (proc->tti_rx+(sf_ahead-1))%10;
    proc->frame_phy_tx     = (proc->tti_rx>(9-(sf_ahead-1))) ? (proc->frame_rx+1)&1023 : proc->frame_rx;
#else
    proc->timestamp_tx = proc->timestamp_rx+(sf_ahead*fp->samples_per_tti);
    proc->tti_tx       = (proc->tti_rx+sf_ahead)%10;
    proc->frame_tx     = (proc->tti_rx>(9-sf_ahead)) ? (proc->frame_rx+1)&1023 : proc->frame_rx;
#endif
    //proc->timestamp_tx = proc->timestamp_rx+(sf_ahead*fp->samples_per_tti);
    //proc->subframe_tx  = (proc->tti_rx+sf_ahead)%10;
    //proc->frame_tx     = (proc->tti_rx>(9-sf_ahead)) ? (proc->frame_rx+1)&1023 : proc->frame_rx;
    LOG_D(PHY,"RU %d/%d TS %llu (off %d), frame %d, subframe %d\n",
          ru->idx,
          0,
          (unsigned long long int)proc->timestamp_rx,
          (int)ru->ts_offset,
          proc->frame_rx,
          proc->tti_rx);
    LOG_D(PHY,"south_in/rx_rf: RU %d/%d TS %llu (off %d), frame %d, subframe %d\n",
          ru->idx,
          0,
          (unsigned long long int)proc->timestamp_rx,
          (int)ru->ts_offset,
          proc->frame_rx,
          proc->tti_rx);

  }

  // dump VCD output for first RU in list
  if (ru->idx == 0) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_RU, proc->frame_rx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_RX0_RU, proc->tti_rx );
  }

  if (proc->first_rx == 0) {
    if (proc->tti_rx != *subframe) {
      LOG_E(PHY,"Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->tti_rx %d, subframe %d)\n",(long long unsigned int)proc->timestamp_rx,proc->tti_rx,*subframe);
      exit_fun("Exiting");
    }

    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",(long long unsigned int)proc->timestamp_rx,proc->frame_rx,*frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->tti_rx;
  }

  //LOG_I(PHY,"timestamp_rx %lu, frame %d(%d), subframe %d(%d)\n",ru->timestamp_rx,proc->frame_rx,frame,proc->tti_rx,subframe);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );

  if (rxs != fp->samples_per_tti) {
#if defined(USRP_REC_PLAY)
    exit_fun("Exiting IQ record/playback");
#else
    //exit_fun( "problem receiving samples" );
    LOG_E(PHY, "problem receiving samples");
#endif
  }
}


void tx_rf(RU_t *ru,
           int frame,
           int subframe,
           uint64_t timestamp) {
  RU_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  void *txp[ru->nb_tx];
  unsigned int txs;
  int i;
  T(T_ENB_PHY_OUTPUT_SIGNAL, T_INT(0), T_INT(0), T_INT(frame), T_INT(subframe),
    T_INT(0), T_BUFFER(&ru->common.txdata[0][subframe * fp->samples_per_tti], fp->samples_per_tti * 4));
  lte_subframe_t SF_type     = subframe_select(fp,subframe%10);
  lte_subframe_t prevSF_type = subframe_select(fp,(subframe+9)%10);
  //lte_subframe_t nextSF_type = subframe_select(fp,(subframe+1)%10);
  int sf_extension = 0;

  if ((SF_type == SF_DL) ||
      (SF_type == SF_S)) {
    int siglen=fp->samples_per_tti,flags=1;

    if (SF_type == SF_S) {
      int txsymb = fp->dl_symbols_in_S_subframe+(ru->is_slave==0 ? 1 : 0);
      AssertFatal(txsymb>0,"illegal txsymb %d\n",txsymb);
      /* end_of_burst_delay is used to stop TX only "after a while".
       * If we stop right after effective signal, with USRP B210 and
       * B200mini, we observe a high EVM on the S subframe (on the
       * PSS).
       * A value of 400 (for 30.72MHz) solves this issue. This is
       * the default.
       */
      siglen = (fp->ofdm_symbol_size + fp->nb_prefix_samples0)
               + (txsymb - 1) * (fp->ofdm_symbol_size + fp->nb_prefix_samples)
               + ru->end_of_burst_delay;
      flags=3; // end of burst
    }

    if (fp->frame_type == TDD &&
        SF_type == SF_DL &&
        prevSF_type == SF_UL) {
      flags = 2; // start of burst
      sf_extension = ru->sf_extension;
    }

#if defined(__x86_64) || defined(__i386__)
#ifdef __AVX2__
    sf_extension = (sf_extension)&0xfffffff8;
#else
    sf_extension = (sf_extension)&0xfffffffc;
#endif
#elif defined(__arm__)
    sf_extension = (sf_extension)&0xfffffffc;
#endif

    for (i=0; i<ru->nb_tx; i++)
      txp[i] = (void *)&ru->common.txdata[i][(subframe*fp->samples_per_tti)-sf_extension];

    /* add fail safe for late command */
    if(late_control!=STATE_BURST_NORMAL) { //stop burst
      LOG_E(PHY,"%d.%d late_control : %d\n",frame,subframe,late_control);
      switch (late_control) {
        case STATE_BURST_TERMINATE:
          flags=10; // end of burst and no time spec
          late_control=STATE_BURST_STOP_1;
          break;

        case STATE_BURST_STOP_1:
          flags=0; // no send
          late_control=STATE_BURST_STOP_2;
          return;//no send
          break;

        case STATE_BURST_STOP_2:
          flags=0; // no send
          late_control=STATE_BURST_RESTART;
          return;//no send
          break;

        case STATE_BURST_RESTART:
          flags=2; // start burst
          late_control=STATE_BURST_NORMAL;
          break;

        default:
          LOG_D(PHY,"[TXPATH] RU %d late_control %d not implemented\n",ru->idx, late_control);
          break;
      }
    }
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_RU, frame);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TTI_NUMBER_TX0_RU, subframe);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, (timestamp-ru->openair0_cfg.tx_sample_advance)&0xffffffff );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_WRITE_FLAGS,flags);
 
    /* add fail safe for late command end */
   VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
    // prepare tx buffer pointers
    txs = ru->rfdevice.trx_write_func(&ru->rfdevice,
                                      timestamp+ru->ts_offset-ru->openair0_cfg.tx_sample_advance-sf_extension,
                                      txp,
                                      siglen+sf_extension,
                                      ru->nb_tx,
                                      flags);
    ru->south_out_cnt++;
    LOG_D(PHY,"south_out_cnt %d\n",ru->south_out_cnt);
    int se = dB_fixed(signal_energy(txp[0],siglen+sf_extension));

    if (SF_type == SF_S) LOG_D(PHY,"[TXPATH] RU %d tx_rf (en %d,len %d), writing to TS %llu, frame %d, unwrapped_frame %d, subframe %d\n",ru->idx, se,
                                 siglen+sf_extension, (long long unsigned int)timestamp, frame, proc->frame_tx_unwrap, subframe);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );

    //    AssertFatal(txs ==  siglen+sf_extension,"TX : Timeout (sent %d/%d)\n",txs, siglen);
    if( usrp_tx_thread == 0 && (txs !=  siglen+sf_extension) && (late_control==STATE_BURST_NORMAL) ) { /* add fail safe for late command */
      late_control=STATE_BURST_TERMINATE;
      LOG_E(PHY,"TX : Timeout (sent %d/%d) state =%d\n",txs, siglen,late_control);
    }
  }
}


/*!
 * \brief The Asynchronous RX/TX FH thread of RAU/RCC/eNB/RRU.
 * This handles the RX FH for an asynchronous RRU/UE
 * \param param is a \ref L1_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void *ru_thread_asynch_rxtx( void *param ) {
  static int ru_thread_asynch_rxtx_status;
  RU_t *ru         = (RU_t *)param;
  RU_proc_t *proc  = &ru->proc;
  int subframe=0, frame=0;
  thread_top_init("ru_thread_asynch_rxtx",1,870000,1000000,1000000);
  // wait for top-level synchronization and do one acquisition to get timestamp for setting frame/subframe
  wait_sync("ru_thread_asynch_rxtx");
  // wait for top-level synchronization and do one acquisition to get timestamp for setting frame/subframe
  LOG_I(PHY, "waiting for devices (ru_thread_asynch_rxtx)\n");
  wait_on_condition(&proc->mutex_asynch_rxtx,&proc->cond_asynch_rxtx,&proc->instance_cnt_asynch_rxtx,"thread_asynch");
  LOG_I(PHY, "devices ok (ru_thread_asynch_rxtx)\n");

  while (!oai_exit) {

    if (ru->state != RU_RUN) {
      subframe=0;
      frame=0;
      usleep(1000);
    } else {
      if (subframe==9) {
        subframe=0;
        frame++;
        frame&=1023;
      } else {
        subframe++;
      }

      LOG_D(PHY,"ru_thread_asynch_rxtx: Waiting on incoming fronthaul\n");

      // asynchronous receive from north (RRU IF4/IF5)
      if (ru->fh_north_asynch_in) {
        if (subframe_select(ru->frame_parms,subframe)!=SF_UL)
          ru->fh_north_asynch_in(ru, &frame, &subframe);
      } else
        AssertFatal(1==0,"Unknown function in ru_thread_asynch_rxtx\n");
    }
  }

  ru_thread_asynch_rxtx_status=0;
  return(&ru_thread_asynch_rxtx_status);
}


void wakeup_slaves(RU_proc_t *proc) {
  int ret;
  struct timespec wait;
  int time_ns = 5000000L;

  for (int i=0; i<proc->num_slaves; i++) {
    RU_proc_t *slave_proc = proc->slave_proc[i];
    // wake up slave FH thread
    // lock the FH mutex and make sure the thread is ready
    clock_gettime(CLOCK_REALTIME,&wait);
    wait.tv_nsec += time_ns;

    if(wait.tv_nsec >= 1000*1000*1000) {
      wait.tv_nsec -= 1000*1000*1000;
      wait.tv_sec  += 1;
    }

    AssertFatal((ret=pthread_mutex_timedlock(&slave_proc->mutex_FH,&wait))==0,"ERROR pthread_mutex_lock for RU %d slave %d (IC %d)\n",proc->ru->idx,slave_proc->ru->idx,slave_proc->instance_cnt_FH);
    int cnt_slave            = ++slave_proc->instance_cnt_FH;
    slave_proc->frame_rx     = proc->frame_rx;
    slave_proc->tti_rx  = proc->tti_rx;
    slave_proc->timestamp_rx = proc->timestamp_rx;
    slave_proc->timestamp_tx = proc->timestamp_tx;
    AssertFatal((ret=pthread_mutex_unlock( &slave_proc->mutex_FH ))==0,"mutex_unlock returns %d\n",ret);

    if (cnt_slave == 0) {
      // the thread was presumably waiting where it should and can now be woken up
      if (pthread_cond_signal(&slave_proc->cond_FH) != 0) {
        LOG_E( PHY, "ERROR pthread_cond_signal for RU %d, slave RU %d\n",proc->ru->idx,slave_proc->ru->idx);
        exit_fun( "ERROR pthread_cond_signal" );
        break;
      }
    } else {
      LOG_W( PHY,"[RU] Frame %d, slave %d thread busy!! (cnt_FH %i)\n",slave_proc->frame_rx,slave_proc->ru->idx, cnt_slave);
      exit_fun( "FH thread busy" );
      break;
    }
  }
}


/*!
 * \brief The prach receive thread of RU.
 * \param param is a \ref RU_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
void *ru_thread_prach( void *param ) {
  static int ru_thread_prach_status;
  RU_t *ru        = (RU_t *)param;
  RU_proc_t *proc = (RU_proc_t *)&ru->proc;
  // set default return value
  ru_thread_prach_status = 0;
  thread_top_init("ru_thread_prach",1,500000,1000000,20000000);
  //wait_sync("ru_thread_prach");

  while (*ru->ru_mask>0 && ru->function!=eNodeB_3GPP) {
    usleep(1e6);
    LOG_D(PHY,"%s() RACH waiting for RU to be configured\n", __FUNCTION__);
  }

  LOG_I(PHY,"%s() RU configured - RACH processing thread running\n", __FUNCTION__);

  while (!oai_exit) {
    if (wait_on_condition(&proc->mutex_prach,&proc->cond_prach,&proc->instance_cnt_prach,"ru_prach_thread") < 0) break;

    if (oai_exit) break;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_RU_PRACH_RX, 1 );

    if (ru->eNB_list[0]) {
      prach_procedures(
        ru->eNB_list[0],0
      );
    } else {
      rx_prach(NULL,
               ru,
               NULL,
               NULL,
               NULL,
               NULL,
               proc->frame_prach,
               0,0
              );
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_RU_PRACH_RX, 0 );

    if (release_thread(&proc->mutex_prach,&proc->instance_cnt_prach,"ru_prach_thread") < 0) break;
  }

  LOG_I(PHY, "Exiting RU thread PRACH\n");
  ru_thread_prach_status = 0;
  return &ru_thread_prach_status;
}


void *ru_thread_prach_br( void *param ) {
  static int ru_thread_prach_status;
  RU_t *ru        = (RU_t *)param;
  RU_proc_t *proc = (RU_proc_t *)&ru->proc;
  // set default return value
  ru_thread_prach_status = 0;
  thread_top_init("ru_thread_prach_br",1,500000,1000000,20000000);
  //wait_sync("ru_thread_prach_br");

  while (!oai_exit) {
    if (wait_on_condition(&proc->mutex_prach_br,&proc->cond_prach_br,&proc->instance_cnt_prach_br,"ru_prach_thread_br") < 0) break;

    if (oai_exit) break;

    rx_prach(NULL,
             ru,
             NULL,
             NULL,
             NULL,
             NULL,
             proc->frame_prach_br,
             0,
             1);

    if (release_thread(&proc->mutex_prach_br,&proc->instance_cnt_prach_br,"ru_prach_thread_br") < 0) break;
  }

  LOG_I(PHY, "Exiting RU thread PRACH BR\n");
  ru_thread_prach_status = 0;
  return &ru_thread_prach_status;
}


int wakeup_synch(RU_t *ru) {
  int ret;
  struct timespec wait;
  int time_ns = 5000000L;
  // wake up synch thread
  // lock the synch mutex and make sure the thread is readif (pthread_mutex_timedlock(&ru->proc.mutex_synch,&wait) != 0) {
  clock_gettime(CLOCK_REALTIME,&wait);
  wait.tv_nsec += time_ns;

  if(wait.tv_nsec >= 1000*1000*1000) {
    wait.tv_nsec -= 1000*1000*1000;
    wait.tv_sec  += 1;
  }

  AssertFatal((ret=pthread_mutex_timedlock(&ru->proc.mutex_synch,&wait)) == 0,"[RU] ERROR pthread_mutex_lock for RU synch thread (IC %d)\n", ru->proc.instance_cnt_synch );
  ++ru->proc.instance_cnt_synch;

  // the thread can now be woken up
  if (pthread_cond_signal(&ru->proc.cond_synch) != 0) {
    LOG_E( PHY, "[RU] ERROR pthread_cond_signal for RU synch thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }

  AssertFatal((ret=pthread_mutex_unlock( &ru->proc.mutex_synch ))==0,"mutex_unlock returns %d\n",ret);
  return(0);
}


void do_ru_synch(RU_t *ru) {
  LTE_DL_FRAME_PARMS *fp  = ru->frame_parms;
  RU_proc_t *proc         = &ru->proc;
  int rxs, ic, ret, i;
  void *rxp[2],*rxp2[2];
  int32_t dummy_rx[ru->nb_rx][fp->samples_per_tti] __attribute__((aligned(32)));

  // initialize the synchronization buffer to the common_vars.rxdata
  for (int i=0; i<ru->nb_rx; i++)
    rxp[i] = &ru->common.rxdata[i][0];

  double temp_freq1 = ru->rfdevice.openair0_cfg->rx_freq[0];

  for (i=0; i<4; i++) {
    ru->rfdevice.openair0_cfg->rx_freq[i] = ru->rfdevice.openair0_cfg->tx_freq[i];
    ru->rfdevice.openair0_cfg->tx_freq[i] = temp_freq1;
  }

  ru->rfdevice.trx_set_freq_func(&ru->rfdevice,ru->rfdevice.openair0_cfg,0);

  while ((ru->in_synch ==0)&&(!oai_exit)) {
    // read in frame
    rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
                                     &(proc->timestamp_rx),
                                     rxp,
                                     fp->samples_per_tti*10,
                                     ru->nb_rx);

    if (rxs != fp->samples_per_tti*10) LOG_E(PHY,"requested %d samples, got %d\n",fp->samples_per_tti*10,rxs);

    // wakeup synchronization processing thread
    wakeup_synch(ru);
    ic=0;

    while ((ic>=0)&&(!oai_exit)) {
      // continuously read in frames, 1ms at a time,
      // until we are done with the synchronization procedure
      for (i=0; i<ru->nb_rx; i++)
        rxp2[i] = (void *)&dummy_rx[i][0];

      for (i=0; i<10; i++)
        rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
                                         &(proc->timestamp_rx),
                                         rxp2,
                                         fp->samples_per_tti,
                                         ru->nb_rx);

      AssertFatal((ret=pthread_mutex_lock(&ru->proc.mutex_synch))==0,"mutex_lock returns %d\n",ret);
      ic = ru->proc.instance_cnt_synch;
      AssertFatal((ret=pthread_mutex_unlock(&ru->proc.mutex_synch))==0,"mutex_unlock returns %d\n",ret);
    } // ic>=0
  } // in_synch==0

  // read in rx_offset samples
  LOG_I(PHY,"Resynchronizing by %d samples\n",ru->rx_offset);
  rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
                                   &(proc->timestamp_rx),
                                   rxp,
                                   ru->rx_offset,
                                   ru->nb_rx);
  // Verification of synchronization procedure
  ru->state = RU_CHECK_SYNC;
  LOG_I(PHY,"Exiting synch routine\n");
}


int check_sync(RU_t *ru, RU_t *ru_master, int subframe) {
  if (fabs(ru_master->proc.t[subframe].tv_nsec - ru->proc.t[subframe].tv_nsec) > 500000)
    return 0;

  return 1;
}


void wakeup_L1s(RU_t *ru) {
  PHY_VARS_eNB **eNB_list = ru->eNB_list;
  PHY_VARS_eNB *eNB       = eNB_list[0];
  L1_proc_t *proc         = &eNB->proc;
  struct timespec t;
  LOG_D(PHY, "wakeup_L1s (num %d) for RU %d (%d.%d) ru->eNB_top:%p\n", ru->num_eNB, ru->idx, ru->proc.frame_rx, ru->proc.tti_rx, ru->eNB_top);
  char string[20];
  sprintf(string, "Incoming RU %d", ru->idx);

  // call eNB function directly
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_WAKEUP_L1S_RU+ru->idx, ru->proc.frame_rx);
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_WAKEUP_L1S_RU+ru->idx, ru->proc.tti_rx);
  AssertFatal(0==pthread_mutex_lock(&proc->mutex_RU),"");
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LOCK_MUTEX_RU+ru->idx, 1);
  //LOG_I(PHY,"wakeup_L1s: Frame %d, Subframe %d: RU %d done (wait_cnt %d),RU_mask[%d] %x\n",
  //          ru->proc.frame_rx,ru->proc.tti_rx,ru->idx,ru->wait_cnt,ru->proc.tti_rx,proc->RU_mask[ru->proc.tti_rx]);
  //VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_WAKEUP_L1S_RU+ru->idx, ru->proc.frame_rx);
  //VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_WAKEUP_L1S_RU+ru->idx, ru->proc.tti_rx);
  clock_gettime(CLOCK_MONOTONIC, &ru->proc.t[ru->proc.tti_rx]);

  if (proc->RU_mask[ru->proc.tti_rx] == 0) {
    //clock_gettime(CLOCK_MONOTONIC,&proc->t[ru->proc.tti_rx]);
    proc->t[ru->proc.tti_rx] = ru->proc.t[ru->proc.tti_rx];
    //start_meas(&proc->ru_arrival_time);
    LOG_D(PHY,"RU %d starting timer for frame %d subframe %d\n", ru->idx, ru->proc.frame_rx, ru->proc.tti_rx);
  }

  for (int i=0; i<eNB->num_RU; i++) {
    if (eNB->RU_list[i]->wait_cnt==1 && ru->proc.tti_rx!=9) eNB->RU_list[i]->wait_cnt=0;

    LOG_D(PHY,"RU %d has frame %d and subframe %d, state %s\n",
          eNB->RU_list[i]->idx, eNB->RU_list[i]->proc.frame_rx, eNB->RU_list[i]->proc.tti_rx, ru_states[eNB->RU_list[i]->state]);

    if (ru == eNB->RU_list[i] && eNB->RU_list[i]->wait_cnt == 0) {
      //AssertFatal((proc->RU_mask&(1<<i)) == 0, "eNB %d frame %d, subframe %d : previous information from RU %d (num_RU %d,mask %x) has not been served yet!\n", eNB->Mod_id,ru->proc.frame_rx,ru->proc.tti_rx,ru->idx,eNB->num_RU,proc->RU_mask);
      proc->RU_mask[ru->proc.tti_rx] |= (1<<i);
    } else if (eNB->RU_list[i]->state == RU_SYNC ||(eNB->RU_list[i]->is_slave==1 && eNB->RU_list[i]->wait_cnt>0 && ru!=eNB->RU_list[i] /*&& ru->is_slave==0*/) ) {
      proc->RU_mask[ru->proc.tti_rx] |= (1<<i);
    }

    //LOG_I(PHY,"RU %d, RU_mask[%d] %d, i %d, frame %d, slave %d, ru->cnt %d, i->cnt %d\n",ru->idx,ru->proc.tti_rx,proc->RU_mask[ru->proc.tti_rx],i,ru->proc.frame_rx,ru->is_slave,ru->wait_cnt,eNB->RU_list[i]->wait_cnt);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_MASK_RU, proc->RU_mask[ru->proc.tti_rx]);

    if (ru->is_slave == 0 && ( (proc->RU_mask[ru->proc.tti_rx]&(1<<i)) == 1 ) && eNB->RU_list[i]->state == RU_RUN) { //This is master & the RRU has already been received
      if (check_sync(eNB->RU_list[i],eNB->RU_list[0],ru->proc.tti_rx) == 0)
        LOG_E(PHY,"RU %d is not SYNC, subframe %d, time  %f this is master\n",
              eNB->RU_list[i]->idx, ru->proc.tti_rx, fabs(eNB->RU_list[i]->proc.t[ru->proc.tti_rx].tv_nsec - eNB->RU_list[0]->proc.t[ru->proc.tti_rx].tv_nsec));
    } else if (ru->is_slave == 1 && ru->state == RU_RUN && ( (proc->RU_mask[ru->proc.tti_rx]&(1<<0)) == 1)) { // master already received. TODO: we assume that RU0 is master.
      if (check_sync(ru,eNB->RU_list[0],ru->proc.tti_rx) == 0)
        LOG_E(PHY,"RU %d is not SYNC time, subframe %d, time  %f\n",
              ru->idx, ru->proc.tti_rx, fabs(ru->proc.t[ru->proc.tti_rx].tv_nsec - eNB->RU_list[0]->proc.t[ru->proc.tti_rx].tv_nsec));
    }
  }

  //clock_gettime(CLOCK_MONOTONIC,&t);
  //LOG_I(PHY,"RU mask is now %x, time is %lu\n",proc->RU_mask[ru->proc.tti_rx], t.tv_nsec - proc->t[ru->proc.tti_rx].tv_nsec);

  if (proc->RU_mask[ru->proc.tti_rx] == (1<<eNB->num_RU)-1) { // all RUs have provided their information so continue on and wakeup eNB top
    LOG_D(PHY,"ru_mask is %d \n ", proc->RU_mask[ru->proc.tti_rx]);
    LOG_D(PHY,"the number of RU is %d, the current ru is RU %d \n ", (1<<eNB->num_RU)-1, ru->idx);
    LOG_D(PHY,"ru->proc.tti_rx is %d \n", ru->proc.tti_rx);
    LOG_D(PHY,"Resetting mask frame %d, subframe %d, this is RU %d\n", ru->proc.frame_rx, ru->proc.tti_rx, ru->idx);
    proc->RU_mask[ru->proc.tti_rx] = 0;
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_MASK_RU, proc->RU_mask[ru->proc.tti_rx]);
    clock_gettime(CLOCK_MONOTONIC,&t);
    //stop_meas(&proc->ru_arrival_time);
    /*AssertFatal(t.tv_nsec < proc->t[ru->proc.tti_rx].tv_nsec+5000000, "Time difference for subframe %d (Frame %d) => %lu > 5ms, this is RU %d\n",
                  ru->proc.tti_rx, ru->proc.frame_rx, t.tv_nsec - proc->t[ru->proc.tti_rx].tv_nsec, ru->idx);*/
    //VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_WAKEUP_L1S_RU+ru->idx, ru->proc.frame_rx);
    //VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_WAKEUP_L1S_RU+ru->idx, ru->proc.tti_rx);
    AssertFatal(0==pthread_mutex_unlock(&proc->mutex_RU),"");
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_LOCK_MUTEX_RU+ru->idx, 0 );
    // unlock RUs that are waiting for eNB processing to be completed
    LOG_D(PHY,"RU %d wakeup eNB top for subframe %d\n", ru->idx, ru->proc.tti_rx);

    if (ru->wait_cnt == 0) {
      if (ru->num_eNB==1 && ru->eNB_top!=0 && get_thread_parallel_conf() == PARALLEL_SINGLE_THREAD) {
        LOG_D(PHY,"RU %d Call eNB_top\n", ru->idx);
        ru->eNB_top(eNB_list[0], proc->frame_rx, proc->subframe_rx, string, ru);
      } else {
        for (int i=0; i<ru->num_eNB; i++) {
          LOG_D(PHY,"ru->wakeup_rxtx:%p\n", ru->wakeup_rxtx);
          eNB_list[i]->proc.ru_proc = &ru->proc;

          if (ru->wakeup_rxtx!=0 && ru->wakeup_rxtx(eNB_list[i],ru) < 0) LOG_E(PHY,"could not wakeup eNB rxtx process for subframe %d\n", ru->proc.tti_rx);
        }
      }
    }

    /*
      AssertFatal(0==pthread_mutex_lock(&ruproc->mutex_eNBs),"");
      LOG_D(PHY,"RU %d sending signal to unlock waiting ru_threads\n", ru->idx);
      AssertFatal(0==pthread_cond_broadcast(&ruproc->cond_eNBs),"");
      if (ruproc->instance_cnt_eNBs==-1) ruproc->instance_cnt_eNBs++;
      AssertFatal(0==pthread_mutex_unlock(&ruproc->mutex_eNBs),"");
    */
  } else { // not all RUs have provided their information
    AssertFatal(0==pthread_mutex_unlock(&proc->mutex_RU),"");
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_LOCK_MUTEX_RU+ru->idx, 0 );
  }

  //      pthread_mutex_unlock(&proc->mutex_RU);
  //      LOG_D(PHY,"wakeup eNB top for for subframe %d\n", ru->proc.tti_rx);
  //      ru->eNB_top(eNB_list[0],ru->proc.frame_rx,ru->proc.tti_rx,string);
  ru->proc.emulate_rf_busy = 0;
}


void wakeup_prach_ru(RU_t *ru) {
  struct timespec wait;
  int time_ns = 5000000L, ret;
  clock_gettime(CLOCK_REALTIME,&wait);
  wait.tv_nsec += time_ns;

  if(wait.tv_nsec >= 1000*1000*1000) {
    wait.tv_nsec -= 1000*1000*1000;
    wait.tv_sec  += 1;
  }

  AssertFatal((ret=pthread_mutex_timedlock(&ru->proc.mutex_prach,&wait)) == 0,"[RU] ERROR pthread_mutex_lock for RU prach thread (IC %d)\n", ru->proc.instance_cnt_prach);

  if (ru->proc.instance_cnt_prach==-1) {
    ++ru->proc.instance_cnt_prach;
    ru->proc.frame_prach    = ru->proc.frame_rx;
    ru->proc.subframe_prach = ru->proc.tti_rx;

    // DJP - think prach_procedures() is looking at eNB frame_prach
    if (ru->eNB_list[0]) {
      ru->eNB_list[0]->proc.frame_prach = ru->proc.frame_rx;
      ru->eNB_list[0]->proc.subframe_prach = ru->proc.tti_rx;
    }

    LOG_D(PHY,"RU %d: waking up PRACH thread\n",ru->idx);
    // the thread can now be woken up
    AssertFatal(pthread_cond_signal(&ru->proc.cond_prach) == 0, "ERROR pthread_cond_signal for RU prach thread\n");
  } else LOG_W(PHY,"RU prach thread busy, skipping\n");

  AssertFatal((ret=pthread_mutex_unlock( &ru->proc.mutex_prach ))==0,"mutex_unlock returns %d\n",ret);
}


void wakeup_prach_ru_br(RU_t *ru) {
  struct timespec wait;
  int time_ns = 5000000L, ret;
  clock_gettime(CLOCK_REALTIME,&wait);
  wait.tv_nsec += time_ns;

  if(wait.tv_nsec >= 1000*1000*1000) {
    wait.tv_nsec -= 1000*1000*1000;
    wait.tv_sec  += 1;
  }

  AssertFatal((ret=pthread_mutex_timedlock(&ru->proc.mutex_prach_br,&wait))==0,"[RU] ERROR pthread_mutex_lock for RU prach thread BR (IC %d)\n", ru->proc.instance_cnt_prach_br);

  if (ru->proc.instance_cnt_prach_br==-1) {
    ++ru->proc.instance_cnt_prach_br;
    ru->proc.frame_prach_br    = ru->proc.frame_rx;
    ru->proc.subframe_prach_br = ru->proc.tti_rx;
    LOG_D(PHY,"RU %d: waking up PRACH thread\n",ru->idx);
    // the thread can now be woken up
    AssertFatal(pthread_cond_signal(&ru->proc.cond_prach_br) == 0, "ERROR pthread_cond_signal for RU prach thread BR\n");
  } else LOG_W(PHY,"RU prach thread busy, skipping\n");

  AssertFatal((ret=pthread_mutex_unlock( &ru->proc.mutex_prach_br ))==0,"mutex_unlock returns %d\n",ret);
}


// this is for RU with local RF unit
void fill_rf_config(RU_t *ru,
                    char *rf_config_file) {
  LTE_DL_FRAME_PARMS *fp   = ru->frame_parms;
  openair0_config_t *cfg   = &ru->openair0_cfg;
  //LOG_I(PHY,"////////////////numerology in config = %d\n",numerology);
  int numerology = get_softmodem_params()->numerology;

  if(fp->N_RB_DL == 100) {
    if(ru->numerology == 0) {
      if (fp->threequarter_fs) {
        cfg->sample_rate=23.04e6;
        cfg->samples_per_frame = 230400;
        cfg->tx_bw = 20e6;
        cfg->rx_bw = 20e6;
      } else {
        cfg->sample_rate=30.72e6;
        cfg->samples_per_frame = 307200;
        cfg->tx_bw = 20e6;
        cfg->rx_bw = 20e6;
      }
    } else if(ru->numerology == 1) {
      cfg->sample_rate=61.44e6;
      cfg->samples_per_frame = 307200;
      cfg->tx_bw = 20e6;
      cfg->rx_bw = 20e6;
    } else if(ru->numerology == 2) {
      cfg->sample_rate=122.88e6;
      cfg->samples_per_frame = 307200;
      cfg->tx_bw = 40e6;
      cfg->rx_bw = 40e6;
    } else {
      LOG_I(PHY,"Wrong input for numerology %d\n setting to 20MHz normal CP configuration",numerology);
      cfg->sample_rate=30.72e6;
      cfg->samples_per_frame = 307200;
      cfg->tx_bw = 10e6;
      cfg->rx_bw = 10e6;
    }
  } else if(fp->N_RB_DL == 50) {
    cfg->sample_rate=15.36e6;
    cfg->samples_per_frame = 153600;
    cfg->tx_bw = 10e6;
    cfg->rx_bw = 10e6;
  } else if (fp->N_RB_DL == 25) {
    cfg->sample_rate=7.68e6;
    cfg->samples_per_frame = 76800;
    cfg->tx_bw = 5e6;
    cfg->rx_bw = 5e6;
  } else if (fp->N_RB_DL == 6) {
    cfg->sample_rate=1.92e6;
    cfg->samples_per_frame = 19200;
    cfg->tx_bw = 1.5e6;
    cfg->rx_bw = 1.5e6;
  } else AssertFatal(1==0,"Unknown N_RB_DL %d\n",fp->N_RB_DL);

  if (fp->frame_type==TDD)
    cfg->duplex_mode = duplex_mode_TDD;
  else //FDD
    cfg->duplex_mode = duplex_mode_FDD;

  cfg->Mod_id = 0;
  cfg->num_rb_dl=fp->N_RB_DL;
  cfg->tx_num_channels=ru->nb_tx;
  cfg->rx_num_channels=ru->nb_rx;

  for (int i=0; i<ru->nb_tx; i++) {
    cfg->tx_freq[i] = (double)fp->dl_CarrierFreq;
    cfg->rx_freq[i] = (double)fp->ul_CarrierFreq;
    cfg->tx_gain[i] = (double)ru->att_tx;
    cfg->rx_gain[i] = ru->max_rxgain-(double)ru->att_rx;
    cfg->configFilename = rf_config_file;
    LOG_I(PHY,"channel %d, Setting tx_gain offset %f, rx_gain offset %f, tx_freq %f, rx_freq %f\n",
           i, cfg->tx_gain[i],
           cfg->rx_gain[i],
           cfg->tx_freq[i],
           cfg->rx_freq[i]);
  }
}


/* this function maps the RU tx and rx buffers to the available rf chains.
   Each rf chain is is addressed by the card number and the chain on the card. The
   rf_map specifies for each antenna port, on which rf chain the mapping should start. Multiple
   antennas are mapped to successive RF chains on the same card. */
int setup_RU_buffers(RU_t *ru) {
  int i,j;
  int card,ant;
  //uint16_t N_TA_offset = 0;
  LTE_DL_FRAME_PARMS *frame_parms;

  if (ru) {
    frame_parms = ru->frame_parms;
    LOG_I(PHY,"setup_RU_buffers: frame_parms = %p\n",frame_parms);
  } else {
    LOG_I(PHY,"RU not initialized (NULL pointer)\n");
    return(-1);
  }

  if (frame_parms->frame_type == TDD) {
    if      (frame_parms->N_RB_DL == 100) ru->N_TA_offset = 624;
    else if (frame_parms->N_RB_DL == 50)  ru->N_TA_offset = 624/2;
    else if (frame_parms->N_RB_DL == 25)  ru->N_TA_offset = 624/4;

    if(IS_SOFTMODEM_BASICSIM)
      /* this is required for the basic simulator in TDD mode
       * TODO: find a proper cleaner solution
       */
      ru->N_TA_offset = 0;

    if      (frame_parms->N_RB_DL == 100) /* no scaling to do */;
    else if (frame_parms->N_RB_DL == 50) {
      ru->sf_extension       /= 2;
      ru->end_of_burst_delay /= 2;
    } else if (frame_parms->N_RB_DL == 25) {
      ru->sf_extension       /= 4;
      ru->end_of_burst_delay /= 4;
    } else {
      LOG_I(PHY,"not handled, todo\n");
      exit(1);
    }
  } else {
    ru->N_TA_offset = 0;
    ru->sf_extension = 0;
    ru->end_of_burst_delay = 0;
  }

  if (ru->openair0_cfg.mmapped_dma == 1) {
    // replace RX signal buffers with mmaped HW versions
    for (i=0; i<ru->nb_rx; i++) {
      card = i/4;
      ant = i%4;
      LOG_I(PHY,"Mapping RU id %d, rx_ant %d, on card %d, chain %d\n",ru->idx,i,ru->rf_map.card+card, ru->rf_map.chain+ant);
      free(ru->common.rxdata[i]);
      ru->common.rxdata[i] = ru->openair0_cfg.rxbase[ru->rf_map.chain+ant];
      LOG_I(PHY,"rxdata[%d] @ %p\n",i,ru->common.rxdata[i]);

      for (j=0; j<16; j++) {
        LOG_I(PHY,"rxbuffer %d: %x\n",j,ru->common.rxdata[i][j]);
        ru->common.rxdata[i][j] = 16-j;
      }
    }

    for (i=0; i<ru->nb_tx; i++) {
      card = i/4;
      ant = i%4;
      LOG_I(PHY,"Mapping RU id %d, tx_ant %d, on card %d, chain %d\n",ru->idx,i,ru->rf_map.card+card, ru->rf_map.chain+ant);
      free(ru->common.txdata[i]);
      ru->common.txdata[i] = ru->openair0_cfg.txbase[ru->rf_map.chain+ant];
      LOG_I(PHY,"txdata[%d] @ %p\n",i,ru->common.txdata[i]);

      for (j=0; j<16; j++) {
        LOG_I(PHY,"txbuffer %d: %x\n",j,ru->common.txdata[i][j]);
        ru->common.txdata[i][j] = 16-j;
      }
    }
  } else { // not memory-mapped DMA
    //nothing to do, everything already allocated in lte_init
  }

  return(0);
}


static void *ru_stats_thread(void *param) {
  RU_t *ru = (RU_t *)param;
  wait_sync("ru_stats_thread");

  while (!oai_exit) {
    sleep(1);

    if (opp_enabled) {
      if (ru->feprx) print_meas(&ru->ofdm_demod_stats,"feprx",NULL,NULL);

      if (ru->feptx_ofdm) print_meas(&ru->ofdm_mod_stats,"feptx_ofdm",NULL,NULL);

      if (ru->fh_north_asynch_in) print_meas(&ru->rx_fhaul,"rx_fhaul",NULL,NULL);

      if (ru->fh_north_out) {
        print_meas(&ru->tx_fhaul,"tx_fhaul",NULL,NULL);
        print_meas(&ru->compression,"compression",NULL,NULL);
        print_meas(&ru->transport,"transport",NULL,NULL);
        LOG_I(PHY,"ru->north_out_cnt = %d\n",ru->north_out_cnt);
      }

      if (ru->fh_south_out) LOG_I(PHY,"ru->south_out_cnt = %d\n",ru->south_out_cnt);

      if (ru->fh_north_asynch_in) LOG_I(PHY,"ru->north_in_cnt = %d\n",ru->north_in_cnt);
    }
  }

  return(NULL);
}

#ifdef PHY_TX_THREAD
  int first_phy_tx = 1;
  volatile int16_t phy_tx_txdataF_end;
  volatile int16_t phy_tx_end;
#endif


static void *ru_thread_tx( void *param ) {
  RU_t *ru         = (RU_t *)param;
  RU_proc_t *proc  = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  PHY_VARS_eNB *eNB;
  L1_proc_t *eNB_proc;
  L1_rxtx_proc_t *L1_proc;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  char filename[256];
  thread_top_init("ru_thread_tx",1,400000,500000,500000);
  //CPU_SET(5, &cpuset);
  //pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  //wait_sync("ru_thread_tx");
  wait_on_condition(&proc->mutex_FH1,&proc->cond_FH1,&proc->instance_cnt_FH1,"ru_thread_tx");
  int ret;

  while (!oai_exit) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_CPUID_RU_THREAD_TX,sched_getcpu());

    if (oai_exit) break;

    LOG_D(PHY,"ru_thread_tx (ru %d): Waiting for TX processing\n",ru->idx);
    // wait until eNBs are finished subframe RX n and TX n+4
    wait_on_condition(&proc->mutex_eNBs,&proc->cond_eNBs,&proc->instance_cnt_eNBs,"ru_thread_tx");
    ret = pthread_mutex_lock(&proc->mutex_eNBs);
    AssertFatal(ret == 0,"mutex_lock return %d\n",ret);
    int frame_tx=proc->frame_tx;
    int tti_tx  =proc->tti_tx;
    uint64_t timestamp_tx = proc->timestamp_tx;
    ret = pthread_mutex_unlock(&proc->mutex_eNBs);
    AssertFatal(ret == 0,"mutex_lock returns %d\n",ret);


    if (oai_exit) break;

    // do TX front-end processing if needed (precoding and/or IDFTs)
    if (ru->feptx_prec) ru->feptx_prec(ru,frame_tx,tti_tx);

    // do OFDM if needed
    if ((ru->fh_north_asynch_in == NULL) && (ru->feptx_ofdm)) ru->feptx_ofdm(ru,frame_tx,tti_tx);

    if(!(ru->emulate_rf)) { //if(!emulate_rf){
      // do outgoing fronthaul (south) if needed
      if ((ru->fh_north_asynch_in == NULL) && (ru->fh_south_out)) ru->fh_south_out(ru,frame_tx,tti_tx,timestamp_tx);

      if (ru->fh_north_out) ru->fh_north_out(ru);
    } else {
      for (int i=0; i<ru->nb_tx; i++) {
        if(frame_tx == 2) {
          sprintf(filename,"txdataF%d_frame%d_sf%d.m",i,frame_tx,tti_tx);
          LOG_M(filename,"txdataF_frame",ru->common.txdataF_BF[i],fp->symbols_per_tti*fp->ofdm_symbol_size, 1, 1);
        }

        if(frame_tx == 2 && tti_tx==0) {
          sprintf(filename,"txdata%d_frame%d.m",i,frame_tx);
          LOG_M(filename,"txdata_frame",ru->common.txdata[i],fp->samples_per_tti*10, 1, 1);
        }
      }
    }

    LOG_D(PHY,"ru_thread_tx: releasing RU TX in %d.%d\n", frame_tx, tti_tx);
    release_thread(&proc->mutex_eNBs,&proc->instance_cnt_eNBs,"ru_thread_tx");

    for(int i = 0; i<ru->num_eNB; i++) {
      eNB       = ru->eNB_list[i];
      eNB_proc  = &eNB->proc;
      L1_proc   = (get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT)? &eNB_proc->L1_proc_tx : &eNB_proc->L1_proc;
      AssertFatal((ret=pthread_mutex_lock(&eNB_proc->mutex_RU_tx))==0,"mutex_lock returns %d\n",ret);

      for (int j=0; j<eNB->num_RU; j++) {
        if (ru == eNB->RU_list[j]) {
          if ((eNB_proc->RU_mask_tx&(1<<j)) > 0)
            LOG_E(PHY,"eNB %d frame %d, subframe %d : previous information from RU tx %d (num_RU %d,mask %x) has not been served yet!\n",
                  eNB->Mod_id,eNB_proc->frame_rx,eNB_proc->subframe_rx,ru->idx,eNB->num_RU,eNB_proc->RU_mask_tx);

          eNB_proc->RU_mask_tx |= (1<<j);
        } else if (eNB->RU_list[j]->state==RU_SYNC ||(eNB->RU_list[j]->is_slave==1 && eNB->RU_list[j]->wait_cnt>0 && ru!=eNB->RU_list[j])) {
          eNB_proc->RU_mask_tx |= (1<<j);
        }

        VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_MASK_TX_RU, eNB_proc->RU_mask_tx);
      }

      if (eNB_proc->RU_mask_tx != (1<<eNB->num_RU)-1) {  // not all RUs have provided their information so return
        //LOG_I(PHY,"Not all RUs have provided their info (mask = %d), RU %d, num_RUs %d\n", eNB_proc->RU_mask_tx,ru->idx,eNB->num_RU);
        AssertFatal((ret=pthread_mutex_unlock(&eNB_proc->mutex_RU_tx))==0,"mutex_unlock returns %d\n",ret);
      } else { // all RUs TX are finished so send the ready signal to eNB processing
        eNB_proc->RU_mask_tx = 0;
        VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_MASK_TX_RU, eNB_proc->RU_mask_tx);
        AssertFatal((ret=pthread_mutex_unlock(&eNB_proc->mutex_RU_tx))==0,"mutex_unlock returns %d\n",ret);
        AssertFatal((ret=pthread_mutex_lock( &L1_proc->mutex_RUs))==0,"mutex_lock returns %d\n",ret);
        L1_proc->instance_cnt_RUs = 0;
        LOG_D(PHY,"ru_thread_tx: Signaling RU TX done in %d.%d\n", frame_tx, tti_tx);
        // the thread can now be woken up
        LOG_D(PHY,"ru_thread_tx: clearing mask and Waking up L1 thread\n");

        if (pthread_cond_signal(&L1_proc->cond_RUs) != 0) {
          LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB TXnp4 thread\n");
          exit_fun( "ERROR pthread_cond_signal" );
        }

        AssertFatal((ret=pthread_mutex_unlock( &L1_proc->mutex_RUs))==0,"mutex_unlock returns %d\n",ret);
      }
    }

    //LOG_I(PHY,"ru_thread_tx: Frame %d, Subframe %d: RU %d done (wait_cnt %d),RU_mask_tx %d\n",
    //eNB_proc->frame_rx,eNB_proc->subframe_rx,ru->idx,ru->wait_cnt,eNB_proc->RU_mask_tx);
  }

  release_thread(&proc->mutex_FH1,&proc->instance_cnt_FH1,"ru_thread_tx");
  return 0;
}


static void *ru_thread( void *param ) {
  RU_t *ru         = (RU_t *)param;
  RU_proc_t *proc  = &ru->proc;
  int subframe = 9;
  int frame = 1023;
  int resynch_done = 0;
  int ret;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  char filename[256];
  // set default return value
#if defined(PRE_SCD_THREAD)
  dlsch_ue_select_tbl_in_use = 1;
#endif
  // set default return value
  thread_top_init("ru_thread",1,400000,500000,500000);
  //CPU_SET(1, &cpuset);
  //pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  pthread_setname_np( pthread_self(),"ru thread");
  LOG_I(PHY,"thread ru created id=%ld\n", syscall(__NR_gettid));
  LOG_I(PHY,"Starting RU %d (%s,%s),\n", ru->idx, NB_functions[ru->function], NB_timing[ru->if_timing]);

  if(get_softmodem_params()->emulate_rf) {
    phy_init_RU(ru);

    if (setup_RU_buffers(ru)!=0) {
      LOG_I(PHY,"Exiting, cannot initialize RU Buffers\n");
      exit(-1);
    }

    LOG_I(PHY, "Signaling main thread that RU %d is ready\n",ru->idx);
    AssertFatal((ret=pthread_mutex_lock(ru->ru_mutex))==0,"mutex_lock returns %d\n",ret);
    *ru->ru_mask &= ~(1<<ru->idx);
    pthread_cond_signal(ru->ru_cond);
    AssertFatal((ret=pthread_mutex_unlock(ru->ru_mutex))==0,"mutex_unlock returns %d\n",ret);
    ru->state = RU_RUN;
  } else if (ru->has_ctrl_prt == 0) {
    // There is no control port: start everything here
    LOG_I(PHY, "RU %d has no OAI ctrl port\n",ru->idx);

    fill_rf_config(ru,ru->rf_config_file);
    init_frame_parms(ru->frame_parms,1);
    ru->frame_parms->nb_antennas_rx = ru->nb_rx;

    if (ru->if_south == LOCAL_RF)       openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);

    phy_init_RU(ru);
      
      
    if (setup_RU_buffers(ru)!=0) {
        LOG_I(PHY,"Exiting, cannot initialize RU Buffers\n");
	      exit(-1);
    }

    AssertFatal((ret=pthread_mutex_lock(ru->ru_mutex))==0,"mutex_lock returns %d\n",ret);
    *ru->ru_mask &= ~(1<<ru->idx);
    pthread_cond_signal(ru->ru_cond);
    AssertFatal((ret=pthread_mutex_unlock(ru->ru_mutex))==0,"mutex_unlock returns %d\n",ret);
    ru->state = RU_RUN;
  }

  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_FH1))==0,"mutex_lock returns %d\n",ret);
  proc->instance_cnt_FH1 = 0;
  pthread_cond_signal(&proc->cond_FH1);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_FH1))==0,"mutex_unlock returns %d\n",ret);

  if(usrp_tx_thread == 1){
     if (ru->start_write_thread){
        if(ru->start_write_thread(ru) != 0){
            LOG_E(HW,"Could not start tx write thread\n");
        }
        else{
            LOG_I(PHY,"tx write thread ready\n");
        }
     }
  }

  while (!oai_exit) {
    if (ru->if_south != LOCAL_RF && ru->is_slave==1) {
      ru->wait_cnt = 100;
    } else {
      ru->wait_cnt = 0;
      ru->wait_check = 0;
    }

    // wait to be woken up
    if (ru->function!=eNodeB_3GPP && ru->has_ctrl_prt == 1) {
      LOG_D(PHY,"RU %d: Waiting for control thread to say go\n",ru->idx);
      if (wait_on_condition(&ru->proc.mutex_ru,&ru->proc.cond_ru_thread,&ru->proc.instance_cnt_ru,"ru_thread")<0) break;
    } else wait_sync("ru_thread"); 
    LOG_D(PHY,"RU %d: Got start from control thread\n",ru->idx);

    if(!(ru->emulate_rf)) {
      if (ru->is_slave == 0) AssertFatal(ru->state == RU_RUN,"ru-%d state = %s != RU_RUN\n",ru->idx,ru_states[ru->state]);
      else if (ru->is_slave == 1) AssertFatal(ru->state == RU_SYNC || ru->state == RU_RUN ||
                                                ru->state == RU_CHECK_SYNC,"ru %d state = %s != RU_SYNC or RU_RUN or RU_CHECK_SYNC\n",ru->idx,ru_states[ru->state]);

      // Start RF device if any
      if (ru->start_rf) {
	if (ru->start_rf(ru) != 0)
	  AssertFatal(1==0,"Could not start the RF device\n");
	else LOG_I(PHY,"RU %d rf device ready\n",ru->idx);
      } else LOG_D(PHY,"RU %d no rf device\n",ru->idx);
    }

    // if an asnych_rxtx thread exists
    // wakeup the thread because the devices are ready at this point

    if ((ru->fh_south_asynch_in)||(ru->fh_north_asynch_in)) {
      AssertFatal((ret=pthread_mutex_lock(&proc->mutex_asynch_rxtx))==0,"mutex_lock returns %d\n",ret);
      proc->instance_cnt_asynch_rxtx=0;
      pthread_cond_signal(&proc->cond_asynch_rxtx);
      AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_asynch_rxtx))==0,"mutex_unlock returns %d\n",ret);
    } else LOG_D(PHY,"RU %d no asynch_south interface\n",ru->idx);

    // if this is a slave RRU, try to synchronize on the DL frequency
    if ((ru->is_slave == 1) && (ru->if_south == LOCAL_RF)) do_ru_synch(ru);

    if (ru->state == RU_RUN || ru->state == RU_CHECK_SYNC) LOG_I(PHY,"RU %d Starting steady-state operation\n",ru->idx);

    // This is a forever while loop, it loops over subframes which are scheduled by incoming samples from HW devices
    while (ru->state == RU_RUN || ru->state == RU_CHECK_SYNC) {
      // these are local subframe/frame counters to check that we are in synch with the fronthaul timing.
      // They are set on the first rx/tx in the underly FH routines.
      if (subframe==9) {
        subframe=0;
        frame++;
        frame&=1023;
      } else {
        subframe++;
      }

      // synchronization on input FH interface, acquire signals/data and block
      if (ru->fh_south_in) ru->fh_south_in(ru,&frame,&subframe);
      else AssertFatal(1==0, "No fronthaul interface at south port");
#ifdef PHY_TX_THREAD

      if(first_phy_tx == 0) {
        phy_tx_end = 0;
        phy_tx_txdataF_end = 0;
        AssertFatal((ret=pthread_mutex_lock(&ru->proc.mutex_phy_tx))==0,"[RU] ERROR pthread_mutex_lock for phy tx thread (IC %d)\n", ru->proc.instance_cnt_phy_tx);

        if (ru->proc.instance_cnt_phy_tx==-1) {
          ++ru->proc.instance_cnt_phy_tx;
          // the thread can now be woken up
          AssertFatal(pthread_cond_signal(&ru->proc.cond_phy_tx) == 0, "ERROR pthread_cond_signal for phy_tx thread\n");
        } else {
          LOG_E(PHY,"phy tx thread busy, skipping\n");
          ++ru->proc.instance_cnt_phy_tx;
        }

        AssertFatal((ret=pthread_mutex_unlock( &ru->proc.mutex_phy_tx ))==0,"mutex_unlock returns %d\n",ret);
      } else {
        phy_tx_end = 1;
        phy_tx_txdataF_end = 1;
      }

      first_phy_tx = 0;
#endif

      if (ru->stop_rf && ru->cmd == STOP_RU) {
        ru->stop_rf(ru);
        ru->state = RU_IDLE;
        ru->cmd   = EMPTY;
        LOG_I(PHY,"RU %d rf device stopped\n",ru->idx);
        break;
      } else if (ru->cmd == STOP_RU) {
        ru->state = RU_IDLE;
        ru->cmd   = EMPTY;
        LOG_I(PHY,"RU %d stopped\n",ru->idx);
        break;
      }

      if (oai_exit == 1) break;

      if (ru->wait_cnt > 0) {
        ru->wait_cnt--;
        LOG_D(PHY,"RU thread %d, frame %d, subframe %d, wait_cnt %d \n",ru->idx, frame, subframe, ru->wait_cnt);

        if (ru->if_south!=LOCAL_RF && ru->wait_cnt <=20 && subframe == 5 && frame != ru->ru0->proc.frame_rx && resynch_done == 0) {
          // Send RRU_frame adjust
          RRU_CONFIG_msg_t rru_config_msg;
          rru_config_msg.type = RRU_frame_resynch;
          rru_config_msg.len  = sizeof(RRU_CONFIG_msg_t); // TODO: set to correct msg len
          ((uint16_t *)&rru_config_msg.msg[0])[0] = ru->ru0->proc.frame_rx;
          ru->cmd=WAIT_RESYNCH;
          LOG_I(PHY,"Sending Frame Resynch %d to RRU %d\n", ru->ru0->proc.frame_rx,ru->idx);
          AssertFatal((ru->ifdevice.trx_ctlsend_func(&ru->ifdevice,&rru_config_msg,rru_config_msg.len)!=-1),"Failed to send msg to RAU\n");
          resynch_done=1;
        }

        wakeup_L1s(ru);
      } else {
        LOG_D(PHY,"RU thread %d, frame %d, subframe %d (do_prach %d, is_prach_subframe %d)\n",
              ru->idx, frame, subframe, ru->do_prach, is_prach_subframe(ru->frame_parms, proc->frame_rx, proc->tti_rx));

        if ((ru->do_prach>0) && (is_prach_subframe(ru->frame_parms, proc->frame_rx, proc->tti_rx)==1)) {
          LOG_D(PHY,"Waking up prach for %d.%d\n", proc->frame_rx, proc->tti_rx);
          wakeup_prach_ru(ru);
        } else if ((ru->do_prach>0) && (is_prach_subframe(ru->frame_parms, proc->frame_rx, proc->tti_rx)>1)) {
          wakeup_prach_ru_br(ru);
        }

        // adjust for timing offset between RU
        if (ru->idx!=0) proc->frame_tx = (proc->frame_tx+proc->frame_offset)&1023;

        // At this point, all information for subframe has been received on FH interface
        // If this proc is to provide synchronization, do so
        wakeup_slaves(proc);

        // do RX front-end processing (frequency-shift, dft) if needed
        if (ru->feprx) ru->feprx(ru, proc->tti_rx);

        // wakeup all eNB processes waiting for this RU
        AssertFatal((ret=pthread_mutex_lock(&proc->mutex_eNBs))==0,"mutex_lock returns %d\n",ret);

        if (proc->instance_cnt_eNBs==0) proc->instance_cnt_eNBs--;

        AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_eNBs))==0,"mutex_unlock returns %d\n",ret);
#if defined(PRE_SCD_THREAD)
        new_dlsch_ue_select_tbl_in_use = dlsch_ue_select_tbl_in_use;
        dlsch_ue_select_tbl_in_use = !dlsch_ue_select_tbl_in_use;
        memcpy(&pre_scd_eNB_UE_stats,&RC.mac[ru->eNB_list[0]->Mod_id]->UE_info.eNB_UE_stats, sizeof(eNB_UE_STATS)*MAX_NUM_CCs*NUMBER_OF_UE_MAX);
        memcpy(&pre_scd_activeUE, &RC.mac[ru->eNB_list[0]->Mod_id]->UE_info.active, sizeof(boolean_t)*NUMBER_OF_UE_MAX);
        AssertFatal((ret=pthread_mutex_lock(&ru->proc.mutex_pre_scd))==0,"[eNB] error locking proc mutex for eNB pre scd\n");
        ru->proc.instance_pre_scd++;

        if (ru->proc.instance_pre_scd == 0) {
          if (pthread_cond_signal(&ru->proc.cond_pre_scd) != 0) {
            LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB pre scd\n" );
            exit_fun( "ERROR pthread_cond_signal cond_pre_scd" );
          }
        } else {
          LOG_E( PHY, "[eNB] frame %d subframe %d rxtx busy instance_pre_scd %d\n",
                 frame,subframe,ru->proc.instance_pre_scd );
        }

        AssertFatal((ret=pthread_mutex_unlock(&ru->proc.mutex_pre_scd))==0,"[eNB] error unlocking mutex_pre_scd mutex for eNB pre scd\n");
#endif
	// wakeup all eNB processes waiting for this RU
	if (ru->num_eNB>0) wakeup_L1s(ru);

#ifdef MBMS_EXPERIMENTAL
	//Workaround ... this must be properly handled
	if(ru->if_south==LOCAL_RF && ru->function==eNodeB_3GPP && ru->eNB_list[0]!=NULL){
		if(ru->frame_parms->num_MBSFN_config!=ru->eNB_list[0]->frame_parms.num_MBSFN_config){
			ru->frame_parms = &ru->eNB_list[0]->frame_parms;//->frame_parms;
			LOG_W(PHY,"RU MBSFN SF PARAMS Updated\n");
		}
	}
#endif
	
#ifndef PHY_TX_THREAD

        if(get_thread_parallel_conf() == PARALLEL_SINGLE_THREAD || ru->num_eNB==0) {
          // do TX front-end processing if needed (precoding and/or IDFTs)
          if (ru->feptx_prec) ru->feptx_prec(ru, proc->frame_tx, proc->tti_tx);

          // do OFDM if needed
          if ((ru->fh_north_asynch_in == NULL) && (ru->feptx_ofdm)) ru->feptx_ofdm(ru, proc->frame_tx, proc->tti_tx);

          if(!(ru->emulate_rf)) { //if(!emulate_rf){
            // do outgoing fronthaul (south) if needed
            if ((ru->fh_north_asynch_in == NULL) && (ru->fh_south_out)) ru->fh_south_out(ru, proc->frame_tx, proc->tti_tx, proc->timestamp_tx);

            if ((ru->fh_north_out) && (ru->state!=RU_CHECK_SYNC)) ru->fh_north_out(ru);
          } else {
            for (int i=0; i<ru->nb_tx; i++) {
              if(proc->frame_tx == 2) {
                sprintf(filename,"txdataF%d_frame%d_sf%d.m",i,proc->frame_tx,proc->tti_tx);
                LOG_M(filename,"txdataF_frame",ru->common.txdataF_BF[i],ru->frame_parms->symbols_per_tti*ru->frame_parms->ofdm_symbol_size, 1, 1);
              }

              if(proc->frame_tx == 2 && proc->tti_tx==0) {
                sprintf(filename,"txdata%d_frame%d.m",i,proc->frame_tx);
                LOG_M(filename,"txdata_frame",ru->common.txdata[i],ru->frame_parms->samples_per_tti*10, 1, 1);
              }
            }
          }

          proc->emulate_rf_busy = 0;
        }

#else
        struct timespec time_req, time_rem;
        time_req.tv_sec = 0;
        time_req.tv_nsec = 10000;

        while((!oai_exit)&&(phy_tx_end == 0)) {
          nanosleep(&time_req,&time_rem);
          continue;
        }

#endif
      } // else wait_cnt == 0
    } // ru->state = RU_RUN || RU_CHECK_SYNC
  } // while !oai_exit

  LOG_I(PHY, "Exiting ru_thread \n");

  if (!(ru->emulate_rf)) {
    if (ru->stop_rf != NULL) {
      if (ru->stop_rf(ru) != 0)
        LOG_E(HW,"Could not stop the RF device\n");
      else LOG_I(PHY,"RU %d rf device stopped\n",ru->idx);
    }
  }

  return NULL;
}


// This thread run the initial synchronization like a UE
void *ru_thread_synch(void *arg) {
  RU_t *ru = (RU_t *)arg;
  LTE_DL_FRAME_PARMS *fp;
  int64_t peak_val, avg;
  static int ru_thread_synch_status = 0;
  int cnt=0;
  thread_top_init("ru_thread_synch",0,5000000,10000000,10000000);
  wait_sync("ru_thread_synch");
  // initialize variables for PSS detection
  ru_sync_time_init(ru); //lte_sync_time_init(ru->frame_parms);

  fp = ru->frame_parms;
  int last_rxoff=0;
  while (!oai_exit) {
    // wait to be woken up
    if (wait_on_condition(&ru->proc.mutex_synch,&ru->proc.cond_synch,&ru->proc.instance_cnt_synch,"ru_thread_synch")<0) break;

    // if we're not in synch, then run initial synch
    if (ru->in_synch == 0) {
      // run intial synch like UE
      LOG_I(PHY,"Running initial synchronization\n");
      ru->rx_offset = ru_sync_time(ru,
                                   &peak_val,
                                   &avg);
      LOG_I(PHY,"RU synch cnt %d: %d, val %llu (%d dB,%d dB)\n",cnt,ru->rx_offset,(unsigned long long)peak_val,dB_fixed64(peak_val),dB_fixed64(avg));
      cnt++;
      int abs_diff= ru->rx_offset - last_rxoff;
      if (abs_diff<0) abs_diff=-abs_diff;
      if (ru->rx_offset >= 0 && abs_diff<6 && avg>0 && dB_fixed(peak_val/avg)>=15 && cnt>10) {
        LOG_I(PHY,"Estimated peak_val %d dB, avg %d => timing offset %llu\n",dB_fixed(peak_val),dB_fixed(avg),(unsigned long long int)ru->rx_offset);
        ru->in_synch = 1;
       /* 
                LOG_M("ru_sync_rx.m","rurx",&ru->common.rxdata[0][0],LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti,1,1);
                LOG_M("ru_sync_corr.m","sync_corr",ru->dmrs_corr,LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti,1,6);
                LOG_M("ru_dmrs.m","rudmrs",&ru->dmrssync[0],fp->ofdm_symbol_size,1,1);
          
        exit(-1);
       */
      } // sync_pos > 0
      else { //AssertFatal(cnt<1000,"Cannot find synch reference\n");
        if (cnt>200) {
          LOG_M("ru_sync_rx.m","rurx",&ru->common.rxdata[0][0],LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti,1,1);
          LOG_M("ru_sync_corr.m","sync_corr",ru->dmrs_corr,LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti,1,6);
          LOG_M("ru_dmrs.m","rudmrs",&ru->dmrssync[0],fp->ofdm_symbol_size,1,1);
          exit(-1);
        }
      }
      last_rxoff=ru->rx_offset;
    } // ru->in_synch==0

    if (release_thread(&ru->proc.mutex_synch,&ru->proc.instance_cnt_synch,"ru_synch_thread") < 0) break;
  } // oai_exit

  ru_sync_time_free(ru);
  ru_thread_synch_status = 0;
  return &ru_thread_synch_status;
}


#if defined(PRE_SCD_THREAD)
void *pre_scd_thread( void *param ) {
  void rlc_tick(int, int);
  static int              eNB_pre_scd_status;
  protocol_ctxt_t         ctxt;
  int                     frame;
  int                     subframe;
  int                     min_rb_unit[MAX_NUM_CCs];
  int                     CC_id;
  int                     Mod_id;
  RU_t               *ru      = (RU_t *)param;
  int                     ret;

  // L2-emulator can work only one eNB
  if( NFAPI_MODE==NFAPI_MODE_VNF)
    Mod_id = 0;
  else
    Mod_id = ru->eNB_list[0]->Mod_id;

  frame = 0;
  subframe = 4;
  thread_top_init("pre_scd_thread",0,870000,1000000,1000000);

  while (!oai_exit) {
    if(oai_exit) {
      break;
    }

    AssertFatal((ret=pthread_mutex_lock(&ru->proc.mutex_pre_scd ))==0,"mutex_lock returns %d\n",ret);

    if (ru->proc.instance_pre_scd < 0) {
      pthread_cond_wait(&ru->proc.cond_pre_scd, &ru->proc.mutex_pre_scd);
    }

    AssertFatal((ret=pthread_mutex_unlock(&ru->proc.mutex_pre_scd))==0,"mutex_unlock returns %d\n",ret);
    PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, Mod_id, ENB_FLAG_YES,
                                   NOT_A_RNTI, frame, subframe,Mod_id);
    rlc_tick(frame, subframe);
    pdcp_run(&ctxt);

    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
      rrc_rx_tx(&ctxt, CC_id);
      min_rb_unit[CC_id] = get_min_rb_unit(Mod_id, CC_id);
    }

    pre_scd_nb_rbs_required(Mod_id, frame, subframe,min_rb_unit,pre_nb_rbs_required[new_dlsch_ue_select_tbl_in_use]);

    if (subframe==9) {
      subframe=0;
      frame++;
      frame&=1023;
    } else {
      subframe++;
    }

    AssertFatal((ret=pthread_mutex_lock(&ru->proc.mutex_pre_scd ))==0,"mutex_lock returns %d\n",ret);
    ru->proc.instance_pre_scd--;
    AssertFatal((ret=pthread_mutex_unlock(&ru->proc.mutex_pre_scd))==0,"mutex_unlock returns %d\n",ret);
  }

  eNB_pre_scd_status = 0;
  return &eNB_pre_scd_status;
}
#endif


#ifdef PHY_TX_THREAD
/*!
 * \brief The phy tx thread of eNB.
 * \param param is a \ref L1_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void *eNB_thread_phy_tx( void *param ) {
  static int eNB_thread_phy_tx_status;
  RU_t *ru        = (RU_t *)param;
  RU_proc_t *proc = &ru->proc;
  PHY_VARS_eNB **eNB_list = ru->eNB_list;
  L1_rxtx_proc_t L1_proc;
  // set default return value
  eNB_thread_phy_tx_status = 0;
  int ret;
  thread_top_init("eNB_thread_phy_tx",1,500000L,1000000L,20000000L);

  while (!oai_exit) {
    if (oai_exit) break;

    if (wait_on_condition(&proc->mutex_phy_tx,&proc->cond_phy_tx,&proc->instance_cnt_phy_tx,"eNB_phy_tx_thread") < 0) break;

    LOG_D(PHY,"Running eNB phy tx procedures\n");
    AssertFatal(ru->num_eNB == 1, "Handle multiple L1 case\n");

    if(ru->num_eNB == 1) {
      L1_proc.subframe_tx = proc->subframe_phy_tx;
      L1_proc.frame_tx = proc->frame_phy_tx;
      phy_procedures_eNB_TX(eNB_list[0], &L1_proc, 1);
      phy_tx_txdataF_end = 1;

      if(pthread_mutex_lock(&ru->proc.mutex_rf_tx) != 0) {
        LOG_E(PHY, "[RU] ERROR pthread_mutex_lock for rf tx thread (IC %d)\n", ru->proc.instance_cnt_rf_tx);
        exit_fun("error locking mutex_rf_tx");
      }

      if (ru->proc.instance_cnt_rf_tx==-1) {
        ++ru->proc.instance_cnt_rf_tx;
        ru->proc.frame_tx     = proc->frame_phy_tx;
        ru->proc.tti_tx       = proc->subframe_phy_tx;
        ru->proc.timestamp_tx = proc->timestamp_phy_tx;
        // the thread can now be woken up
        AssertFatal(pthread_cond_signal(&ru->proc.cond_rf_tx) == 0, "ERROR pthread_cond_signal for rf_tx thread\n");
      } else {
        LOG_E(PHY,"rf tx thread busy, skipping\n");
        late_control=STATE_BURST_TERMINATE;
      }

      pthread_mutex_unlock( &ru->proc.mutex_rf_tx );
    }

    AssertFatal((ret=pthread_mutex_unlock( &ru->proc.mutex_rf_tx ))==0,"mutex_unlock returns %d\n",ret);

    if (release_thread(&proc->mutex_phy_tx,&proc->instance_cnt_phy_tx,"eNB_thread_phy_tx") < 0) break;

    phy_tx_end = 1;
  }

  LOG_I(PHY, "Exiting eNB thread PHY TX\n");
  eNB_thread_phy_tx_status = 0;
  return &eNB_thread_phy_tx_status;
}


static void *rf_tx( void *param ) {
  static int rf_tx_status;
  RU_t *ru      = (RU_t *)param;
  RU_proc_t *proc = &ru->proc;
  // set default return value
  rf_tx_status = 0;
  thread_top_init("rf_tx",1,500000L,1000000L,20000000L);

  while (!oai_exit) {
    if (oai_exit) break;

    if (wait_on_condition(&proc->mutex_rf_tx,&proc->cond_rf_tx,&proc->instance_cnt_rf_tx,"rf_tx_thread") < 0) break;

    LOG_D(PHY,"Running eNB rf tx procedures\n");

    if(ru->num_eNB == 1) {
      // do TX front-end processing if needed (precoding and/or IDFTs)
      if (ru->feptx_prec) ru->feptx_prec(ru);

      // do OFDM if needed
      if ((ru->fh_north_asynch_in == NULL) && (ru->feptx_ofdm)) ru->feptx_ofdm(ru,proc->frame_tx,proc->tti_tx);

      if(!ru->emulate_rf) {
        // do outgoing fronthaul (south) if needed
        if ((ru->fh_north_asynch_in == NULL) && (ru->fh_south_out)) ru->fh_south_out(ru,proc->frame_tx,proc->tti_tx,proc->timestamp_tx);

        if (ru->fh_north_out) ru->fh_north_out(ru);
      }
    }

    if (release_thread(&proc->mutex_rf_tx,&proc->instance_cnt_rf_tx,"rf_tx") < 0) break;

    if(proc->instance_cnt_rf_tx >= 0) {
      late_control=STATE_BURST_TERMINATE;
      LOG_E(PHY,"detect rf tx busy change mode TX failsafe\n");
    }
  }

  LOG_I(PHY, "Exiting rf TX\n");
  rf_tx_status = 0;
  return &rf_tx_status;
}
#endif



int start_streaming(RU_t *ru) {
  LOG_I(PHY,"Starting streaming on third-party RRU\n");
  return(ru->ifdevice.thirdparty_startstreaming(&ru->ifdevice));
}

int start_if(struct RU_t_s *ru,struct PHY_VARS_eNB_s *eNB) {
  return(ru->ifdevice.trx_start_func(&ru->ifdevice));
}

int start_rf(RU_t *ru) {
  return(ru->rfdevice.trx_start_func(&ru->rfdevice));
}

int stop_rf(RU_t *ru) {
    if(ru->rfdevice.trx_end_func != NULL) {
      ru->rfdevice.trx_end_func(&ru->rfdevice);
    }
  return 0;
}


extern void configure_ru(int idx, void *arg);
extern void fep_full(RU_t *ru, int subframe);
extern void feptx_ofdm(RU_t *ru, int frame_tx, int tti_tx);
extern void feptx_ofdm_2thread(RU_t *ru, int frame_tx, int tti_tx);
extern void feptx_prec(RU_t *ru, int frame_tx, int tti_tx);
extern void init_fep_thread(RU_t *ru, pthread_attr_t *attr_fep);
extern void init_feptx_thread(RU_t *ru, pthread_attr_t *attr_feptx);
extern void kill_fep_thread(RU_t *ru);
extern void kill_feptx_thread(RU_t *ru);
extern void ru_fep_full_2thread(RU_t *ru, int subframe);
extern void *ru_thread_control( void *param );


void reset_proc(RU_t *ru) {
  int i=0;
  RU_proc_t *proc;
  AssertFatal(ru != NULL, "ru is null\n");
  proc = &ru->proc;
  proc->ru = ru;
  proc->first_rx                 = 1;
  proc->first_tx                 = 1;
  proc->frame_offset             = 0;
  proc->frame_tx_unwrap          = 0;

  for (i=0; i<10; i++) proc->symbol_mask[i]=0;
}

int start_write_thread(RU_t *ru) {
    return(ru->rfdevice.trx_write_init(&ru->rfdevice));
}

void init_RU_proc(RU_t *ru) {
  int i=0;
  RU_proc_t *proc;
  pthread_attr_t *attr_FH=NULL, *attr_FH1=NULL, *attr_prach=NULL, *attr_asynch=NULL, *attr_synch=NULL, *attr_emulateRF=NULL, *attr_ctrl=NULL, *attr_prach_br=NULL;
  //pthread_attr_t *attr_fep=NULL;
  LOG_I(PHY,"Initializing RU proc %d (%s,%s),\n",ru->idx,NB_functions[ru->function],NB_timing[ru->if_timing]);
  proc = &ru->proc;
  memset((void *)proc,0,sizeof(RU_proc_t));
  proc->ru = ru;
  proc->instance_cnt_prach       = -1;
  proc->instance_cnt_synch       = -1;
  proc->instance_cnt_FH          = -1;
  proc->instance_cnt_FH1         = -1;
  proc->instance_cnt_emulateRF   = -1;
  proc->instance_cnt_asynch_rxtx = -1;
  proc->instance_cnt_ru          = -1;
  proc->instance_cnt_eNBs        = -1;
  proc->first_rx                 = 1;
  proc->first_tx                 = 1;
  proc->frame_offset             = 0;
  proc->num_slaves               = 0;
  proc->frame_tx_unwrap          = 0;

  for (i=0; i<10; i++) proc->symbol_mask[i]=0;

  pthread_mutex_init( &proc->mutex_prach, NULL);
  pthread_mutex_init( &proc->mutex_asynch_rxtx, NULL);
  pthread_mutex_init( &proc->mutex_synch,NULL);
  pthread_mutex_init( &proc->mutex_FH,NULL);
  pthread_mutex_init( &proc->mutex_FH1,NULL);
  pthread_mutex_init( &proc->mutex_emulateRF,NULL);
  pthread_mutex_init( &proc->mutex_eNBs, NULL);
  pthread_mutex_init( &proc->mutex_ru,NULL);
  pthread_cond_init( &proc->cond_prach, NULL);
  pthread_cond_init( &proc->cond_FH, NULL);
  pthread_cond_init( &proc->cond_FH1, NULL);
  pthread_cond_init( &proc->cond_emulateRF, NULL);
  pthread_cond_init( &proc->cond_asynch_rxtx, NULL);
  pthread_cond_init( &proc->cond_synch,NULL);
  pthread_cond_init( &proc->cond_eNBs, NULL);
  pthread_cond_init( &proc->cond_ru_thread,NULL);
  pthread_attr_init( &proc->attr_FH);
  pthread_attr_init( &proc->attr_FH1);
  pthread_attr_init( &proc->attr_emulateRF);
  pthread_attr_init( &proc->attr_prach);
  pthread_attr_init( &proc->attr_synch);
  pthread_attr_init( &proc->attr_asynch_rxtx);
  pthread_attr_init( &proc->attr_fep);
  proc->instance_cnt_prach_br = -1;
  pthread_mutex_init( &proc->mutex_prach_br, NULL);
  pthread_cond_init( &proc->cond_prach_br, NULL);
  pthread_attr_init( &proc->attr_prach_br);
#ifdef PHY_TX_THREAD
  proc->instance_cnt_phy_tx = -1;
  pthread_mutex_init( &proc->mutex_phy_tx, NULL);
  pthread_cond_init( &proc->cond_phy_tx, NULL);
  proc->instance_cnt_rf_tx = -1;
  pthread_mutex_init( &proc->mutex_rf_tx, NULL);
  pthread_cond_init( &proc->cond_rf_tx, NULL);
#endif
#ifndef DEADLINE_SCHEDULER
  attr_FH        = &proc->attr_FH;
  attr_FH1       = &proc->attr_FH1;
  attr_prach     = &proc->attr_prach;
  attr_synch     = &proc->attr_synch;
  attr_asynch    = &proc->attr_asynch_rxtx;
  attr_emulateRF = &proc->attr_emulateRF;
  attr_prach_br  = &proc->attr_prach_br;
#endif

  if (ru->has_ctrl_prt == 1) pthread_create( &proc->pthread_ctrl, attr_ctrl, ru_thread_control, (void*)ru );
  else {
    if (ru->start_if) {
      LOG_I(PHY,"Starting IF interface for RU %d\n",ru->idx);
      AssertFatal(
                  ru->start_if(ru,NULL)   == 0, "Could not start the IF device\n");

      if (ru->if_south != LOCAL_RF) wait_eNBs();
    }
  }


  pthread_create( &proc->pthread_FH, attr_FH, ru_thread, (void *)ru );
#if defined(PRE_SCD_THREAD)
  proc->instance_pre_scd = -1;
  pthread_mutex_init( &proc->mutex_pre_scd, NULL);
  pthread_cond_init( &proc->cond_pre_scd, NULL);
  pthread_create(&proc->pthread_pre_scd, NULL, pre_scd_thread, (void *)ru);
  pthread_setname_np(proc->pthread_pre_scd, "pre_scd_thread");
#endif
#ifdef PHY_TX_THREAD
  pthread_create( &proc->pthread_phy_tx, NULL, eNB_thread_phy_tx, (void *)ru );
  pthread_setname_np( proc->pthread_phy_tx, "phy_tx_thread" );
  pthread_create( &proc->pthread_rf_tx, NULL, rf_tx, (void *)ru );
#endif

  if (ru->emulate_rf)
    pthread_create( &proc->pthread_emulateRF, attr_emulateRF, emulatedRF_thread, (void *)proc );

  if (get_thread_parallel_conf() == PARALLEL_RU_L1_SPLIT || get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT)
    pthread_create( &proc->pthread_FH1, attr_FH1, ru_thread_tx, (void *)ru );

  if (ru->function == NGFI_RRU_IF4p5) {
    pthread_create( &proc->pthread_prach, attr_prach, ru_thread_prach, (void *)ru );
    pthread_create( &proc->pthread_prach_br, attr_prach_br, ru_thread_prach_br, (void *)ru );

    if (ru->is_slave == 1) pthread_create( &proc->pthread_synch, attr_synch, ru_thread_synch, (void *)ru);

    if ((ru->if_timing == synch_to_other) || (ru->function == NGFI_RRU_IF5) || (ru->function == NGFI_RRU_IF4p5)) {
      LOG_I(PHY,"Starting ru_thread_asynch_rxtx, ru->is_slave %d, ru->generate_dmrs_sync %d\n",
            ru->is_slave,ru->generate_dmrs_sync);
      //generate_ul_ref_sigs();
      //ru->dmrssync = (int16_t*)malloc16_clear(ru->frame_parms.ofdm_symbol_size*2*sizeof(int16_t));
      pthread_create( &proc->pthread_asynch_rxtx, attr_asynch, ru_thread_asynch_rxtx, (void *)ru );
    }
  } else if (ru->function == eNodeB_3GPP && ru->if_south == LOCAL_RF) { // DJP - need something else to distinguish between monolithic and PNF
    LOG_I(PHY,"%s() DJP - added creation of pthread_prach\n", __FUNCTION__);
    pthread_create( &proc->pthread_prach, attr_prach, ru_thread_prach, (void *)ru );
    ru->state=RU_RUN;
  }

  if (get_thread_worker_conf() == WORKER_ENABLE) {
    init_fep_thread(ru, NULL);
    init_feptx_thread(ru, NULL);
  }

  if (opp_enabled == 1) pthread_create(&ru->ru_stats_thread,NULL,ru_stats_thread,(void *)ru);
/*
  if (ru->function == eNodeB_3GPP) {
    usleep(10000);
    LOG_I(PHY, "Signaling main thread that RU %d (is_slave %d,send_dmrs %d) is ready in state %s\n",ru->idx,ru->is_slave,ru->generate_dmrs_sync,ru_states[ru->state]);
    AssertFatal((ret=pthread_mutex_lock(ru->ru_mutex))==0,"mutex_lock returns %d\n",ret);
    *ru->ru_mask &= ~(1<<ru->idx);
    pthread_cond_signal(ru->ru_cond);
    AssertFatal((ret=pthread_mutex_unlock(ru->ru_mutex))==0,"mutex_unlock returns %d\n",ret);
  }
  */
}


void kill_RU_proc(RU_t *ru) {
  int ret;
  RU_proc_t *proc = &ru->proc;
#if defined(PRE_SCD_THREAD)
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_pre_scd))==0,"mutex_lock returns %d\n",ret);
  ru->proc.instance_pre_scd = 0;
  pthread_cond_signal(&proc->cond_pre_scd);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_pre_scd))==0,"mutex_unlock returns %d\n",ret);
  pthread_join(proc->pthread_pre_scd, NULL);
  pthread_mutex_destroy(&proc->mutex_pre_scd);
  pthread_cond_destroy(&proc->cond_pre_scd);
#endif
#ifdef PHY_TX_THREAD
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_phy_tx))==0,"mutex_lock returns %d\n",ret);
  proc->instance_cnt_phy_tx = 0;
  pthread_cond_signal(&proc->cond_phy_tx);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_phy_tx))==0,"mutex_unlock returns %d\n",ret);
  pthread_join(ru->proc.pthread_phy_tx, NULL);
  pthread_mutex_destroy( &proc->mutex_phy_tx);
  pthread_cond_destroy( &proc->cond_phy_tx);
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_rf_tx))==0,"mutex_lock returns %d\n",ret);
  proc->instance_cnt_rf_tx = 0;
  pthread_cond_signal(&proc->cond_rf_tx);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_rf_tx))==0,"mutex_unlock returns %d\n",ret);
  pthread_join(proc->pthread_rf_tx, NULL);
  pthread_mutex_destroy( &proc->mutex_rf_tx);
  pthread_cond_destroy( &proc->cond_rf_tx);
#endif

  if (get_thread_worker_conf() == WORKER_ENABLE) {
    LOG_D(PHY, "killing FEP thread\n");
    kill_fep_thread(ru);
    LOG_D(PHY, "killing FEP TX thread\n");
    kill_feptx_thread(ru);
  }

  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_FH))==0,"mutex_lock returns %d\n",ret);
  proc->instance_cnt_FH = 0;
  pthread_cond_signal(&proc->cond_FH);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_FH))==0,"mutex_unlock returns %d\n",ret);
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_FH1))==0,"mutex_lock returns %d\n",ret);
  proc->instance_cnt_FH1 = 0;
  pthread_cond_signal(&proc->cond_FH1);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_FH1))==0,"mutex_unlock returns %d\n",ret);
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_prach))==0,"mutex_lock returns %d\n",ret);
  proc->instance_cnt_prach = 0;
  pthread_cond_signal(&proc->cond_prach);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_prach))==0,"mutex_unlock returns %d\n",ret);
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_prach_br))==0,"mutex_lock returns %d\n",ret);
  proc->instance_cnt_prach_br = 0;
  pthread_cond_signal(&proc->cond_prach_br);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_prach_br))==0,"mutex_unlock returns %d\n",ret);
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_synch))==0,"mutex_lock returns %d\n",ret);
  proc->instance_cnt_synch = 0;
  pthread_cond_signal(&proc->cond_synch);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_synch))==0,"mutex_unlock returns %d\n",ret);
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_eNBs))==0,"mutex_lock returns %d\n",ret);
  proc->instance_cnt_eNBs = 1;
  // cond_eNBs is used by both ru_thread and ru_thread_tx, so we need to send
  // a broadcast to wake up both threads
  pthread_cond_broadcast(&proc->cond_eNBs);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_eNBs))==0,"mutex_unlock returns %d\n",ret);
  AssertFatal((ret=pthread_mutex_lock(&proc->mutex_asynch_rxtx))==0,"mutex_lock returns %d\n",ret);
  proc->instance_cnt_asynch_rxtx = 0;
  pthread_cond_signal(&proc->cond_asynch_rxtx);
  AssertFatal((ret=pthread_mutex_unlock(&proc->mutex_asynch_rxtx))==0,"mutex_unlock returns %d\n",ret);
  LOG_D(PHY, "Joining pthread_FH\n");
  pthread_join(proc->pthread_FH, NULL);

  if (get_thread_parallel_conf() == PARALLEL_RU_L1_SPLIT || get_thread_parallel_conf() == PARALLEL_RU_L1_TRX_SPLIT) {
    LOG_D(PHY, "Joining pthread_FHTX\n");
    pthread_join(proc->pthread_FH1, NULL);
  }

  if (ru->function == NGFI_RRU_IF4p5) {
    LOG_D(PHY, "Joining pthread_prach\n");
    pthread_join(proc->pthread_prach, NULL);
    LOG_D(PHY, "Joining pthread_prach_br\n");
    pthread_join(proc->pthread_prach_br, NULL);

    if (ru->is_slave) {
      LOG_D(PHY, "Joining pthread_\n");
      pthread_join(proc->pthread_synch, NULL);
    }

    if ((ru->if_timing == synch_to_other) ||
        (ru->function == NGFI_RRU_IF5) ||
        (ru->function == NGFI_RRU_IF4p5)) {
      LOG_D(PHY, "Joining pthread_asynch_rxtx\n");
      pthread_join(proc->pthread_asynch_rxtx, NULL);
    }
  }

  if (opp_enabled) {
    LOG_D(PHY, "Joining ru_stats_thread\n");
    pthread_join(ru->ru_stats_thread, NULL);
  }

  pthread_mutex_destroy(&proc->mutex_prach);
  pthread_mutex_destroy(&proc->mutex_asynch_rxtx);
  pthread_mutex_destroy(&proc->mutex_synch);
  pthread_mutex_destroy(&proc->mutex_FH);
  pthread_mutex_destroy(&proc->mutex_FH1);
  pthread_mutex_destroy(&proc->mutex_eNBs);
  pthread_cond_destroy(&proc->cond_prach);
  pthread_cond_destroy(&proc->cond_FH);
  pthread_cond_destroy(&proc->cond_FH1);
  pthread_cond_destroy(&proc->cond_asynch_rxtx);
  pthread_cond_destroy(&proc->cond_synch);
  pthread_cond_destroy(&proc->cond_eNBs);
  pthread_attr_destroy(&proc->attr_FH);
  pthread_attr_destroy(&proc->attr_FH1);
  pthread_attr_destroy(&proc->attr_prach);
  pthread_attr_destroy(&proc->attr_synch);
  pthread_attr_destroy(&proc->attr_asynch_rxtx);
  pthread_attr_destroy(&proc->attr_fep);
  pthread_mutex_destroy(&proc->mutex_prach_br);
  pthread_cond_destroy(&proc->cond_prach_br);
  pthread_attr_destroy(&proc->attr_prach_br);
}


void init_precoding_weights(RU_t **rup,int nb_RU,PHY_VARS_eNB *eNB) {
  int layer,ru_id,aa,re,tb;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  RU_t *ru;
  LTE_eNB_DLSCH_t *dlsch;

  // init precoding weigths
  for (int dlsch_id=0; dlsch_id<NUMBER_OF_DLSCH_MAX; dlsch_id++) {
    for (tb=0; tb<2; tb++) {
      dlsch = eNB->dlsch[dlsch_id][tb];

      for (layer=0; layer<4; layer++) {
        int nb_tx=0;

        for (ru_id=0; ru_id<nb_RU; ru_id++) {
          ru = rup[ru_id];
          nb_tx+=ru->nb_tx;
        }

        dlsch->ue_spec_bf_weights[layer] = (int32_t **)malloc16(nb_tx*sizeof(int32_t *));

        for (aa=0; aa<nb_tx; aa++) {
          dlsch->ue_spec_bf_weights[layer][aa] = (int32_t *)malloc16(fp->ofdm_symbol_size*sizeof(int32_t));

          for (re=0; re<fp->ofdm_symbol_size; re++) {
            dlsch->ue_spec_bf_weights[layer][aa][re] = 0x00007fff;
          }
        }
      }
    }
  }
}


void set_function_spec_param(RU_t *ru) {
  int ret;


  switch (ru->if_south) {
    case LOCAL_RF:   // this is an RU with integrated RF (RRU, eNB)
      if (ru->function ==  NGFI_RRU_IF5) {                 // IF5 RRU
        ru->do_prach              = 0;                      // no prach processing in RU
        ru->fh_north_in           = NULL;                   // no shynchronous incoming fronthaul from north
        ru->fh_north_out          = fh_if5_north_out;       // need only to do send_IF5  reception
        ru->fh_south_out          = tx_rf;                  // send output to RF
        ru->fh_north_asynch_in    = fh_if5_north_asynch_in; // TX packets come asynchronously
        ru->feprx                 = NULL;                   // nothing (this is a time-domain signal)
        ru->feptx_ofdm            = NULL;                   // nothing (this is a time-domain signal)
        ru->feptx_prec            = NULL;                   // nothing (this is a time-domain signal)
        ru->start_if              = start_if;               // need to start the if interface for if5
        ru->ifdevice.host_type    = RRU_HOST;
        ru->rfdevice.host_type    = RRU_HOST;
        ru->ifdevice.eth_params   = &ru->eth_params;
        reset_meas(&ru->rx_fhaul);
        reset_meas(&ru->tx_fhaul);
        reset_meas(&ru->compression);
        reset_meas(&ru->transport);
        ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
        LOG_I(PHY,"NGFI_RRU_IF5: openair0_transport_init returns %d for ru_id %d\n", ret, ru->idx);

        if (ret<0) {
          LOG_I(PHY,"Exiting, cannot initialize transport protocol\n");
          exit(-1);
        }
      } else if (ru->function == NGFI_RRU_IF4p5) {
        ru->do_prach              = 1;                        // do part of prach processing in RU
        ru->fh_north_in           = NULL;                     // no synchronous incoming fronthaul from north
        ru->fh_north_out          = fh_if4p5_north_out;       // send_IF4p5 on reception
        ru->fh_south_out          = tx_rf;                    // send output to RF
        ru->fh_north_asynch_in    = fh_if4p5_north_asynch_in; // TX packets come asynchronously
        ru->feprx                 = (get_thread_worker_conf() == WORKER_DISABLE) ? fep_full :ru_fep_full_2thread;                 // RX DFTs
        ru->feptx_ofdm            = (get_thread_worker_conf() == WORKER_DISABLE) ? feptx_ofdm : feptx_ofdm_2thread;               // this is fep with idft only (no precoding in RRU)
        ru->feptx_prec            = NULL;
        ru->start_if              = start_if;                 // need to start the if interface for if4p5
        ru->ifdevice.host_type    = RRU_HOST;
        ru->rfdevice.host_type    = RRU_HOST;
        ru->ifdevice.eth_params   = &ru->eth_params;
        reset_meas(&ru->rx_fhaul);
        reset_meas(&ru->tx_fhaul);
        reset_meas(&ru->compression);
        reset_meas(&ru->transport);
        ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
        LOG_I(PHY,"NGFI_RRU_if4p5 : openair0_transport_init returns %d for ru_id %d\n", ret, ru->idx);

        if (ret<0) {
          LOG_I(PHY,"Exiting, cannot initialize transport protocol\n");
          exit(-1);
        }

        malloc_IF4p5_buffer(ru);
      } else if (ru->function == eNodeB_3GPP) {
        ru->do_prach             = 0;                       // no prach processing in RU
        ru->feprx                = (get_thread_worker_conf() == WORKER_DISABLE) ? fep_full : ru_fep_full_2thread;                // RX DFTs
        ru->feptx_ofdm           = (get_thread_worker_conf() == WORKER_DISABLE) ? feptx_ofdm : feptx_ofdm_2thread;              // this is fep with idft and precoding
        ru->feptx_prec           = feptx_prec;              // this is fep with idft and precoding
        ru->fh_north_in          = NULL;                    // no incoming fronthaul from north
        ru->fh_north_out         = NULL;                    // no outgoing fronthaul to north
        ru->start_if             = NULL;                    // no if interface
        ru->rfdevice.host_type   = RAU_HOST;
        ru->start_write_thread     = start_write_thread;
      }

      ru->fh_south_in            = rx_rf;                               // local synchronous RF RX
      ru->fh_south_out           = tx_rf;                               // local synchronous RF TX
      ru->start_rf               = start_rf;                            // need to start the local RF interface
      ru->stop_rf                = stop_rf;
      LOG_I(PHY,"NFGI_RRU_IF4p5: configuring ru_id %d (start_rf %p)\n", ru->idx, start_rf);
      /*
        if (ru->function == eNodeB_3GPP) { // configure RF parameters only for 3GPP eNodeB, we need to get them from RAU otherwise
        fill_rf_config(ru,rf_config_file);
        init_frame_parms(&ru->frame_parms,1);
        phy_init_RU(ru);
        }

        ret = openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
        if (setup_RU_buffers(ru)!=0) {
        LOG_I(PHY,"Exiting, cannot initialize RU Buffers\n");
        exit(-1);
        }*/
      break;

    case REMOTE_IF5: // the remote unit is IF5 RRU
      ru->do_prach               = 0;
      ru->feprx                  = (get_thread_worker_conf() == WORKER_DISABLE) ? fep_full : fep_full;             // this is frequency-shift + DFTs
      ru->feptx_prec             = feptx_prec;                                                                     // need to do transmit Precoding + IDFTs
      ru->feptx_ofdm             = (get_thread_worker_conf() == WORKER_DISABLE) ? feptx_ofdm : feptx_ofdm_2thread; // need to do transmit Precoding + IDFTs

      if (ru->if_timing == synch_to_other) {
        ru->fh_south_in          = fh_slave_south_in;                  // synchronize to master
      } else {
        ru->fh_south_in          = fh_if5_south_in;     // synchronous IF5 reception
        ru->fh_south_out         = fh_if5_south_out;    // synchronous IF5 transmission
        ru->fh_south_asynch_in   = NULL;                // no asynchronous UL
      }
      ru->start_rf               = ru->eth_params.transp_preference == ETH_UDP_IF5_ECPRI_MODE ? start_streaming : NULL;
      ru->stop_rf                = NULL;
      ru->start_if               = start_if;             // need to start if interface for IF5
      ru->ifdevice.host_type     = RAU_HOST;
      ru->ifdevice.eth_params    = &ru->eth_params;
      ru->ifdevice.configure_rru = configure_ru;
      ret = openair0_transport_load(&ru->ifdevice,&ru->openair0_cfg,&ru->eth_params);
      LOG_I(PHY,"REMOTE_IF5: openair0_transport_init returns %d for ru_id %d\n", ret, ru->idx);

      if (ret<0) {
        LOG_I(PHY,"Exiting, cannot initialize transport protocol\n");
        exit(-1);
      }

      break;

    case REMOTE_IF4p5:
      ru->do_prach               = 0;
      ru->feprx                  = NULL;                // DFTs
      ru->feptx_prec             = feptx_prec;          // Precoding operation
      ru->feptx_ofdm             = NULL;                // no OFDM mod
      ru->fh_south_in            = fh_if4p5_south_in;   // synchronous IF4p5 reception
      ru->fh_south_out           = fh_if4p5_south_out;  // synchronous IF4p5 transmission
      ru->fh_south_asynch_in     = (ru->if_timing == synch_to_other) ? fh_if4p5_south_in : NULL;                // asynchronous UL if synch_to_other
      ru->fh_north_out           = NULL;
      ru->fh_north_asynch_in     = NULL;
      ru->start_rf               = NULL;                // no local RF
      ru->stop_rf                = NULL;
      ru->start_if               = start_if;            // need to start if interface for IF4p5
      ru->ifdevice.host_type     = RAU_HOST;
      ru->ifdevice.eth_params    = &ru->eth_params;
      ru->ifdevice.configure_rru = configure_ru;
      ret = openair0_transport_load(&ru->ifdevice, &ru->openair0_cfg, &ru->eth_params);
      LOG_I(PHY,"REMOTE IF4p5: openair0_transport_init returns %d for ru_id %d\n", ret, ru->idx);

      if (ret<0) {
        LOG_I(PHY,"Exiting, cannot initialize transport protocol\n");
        exit(-1);
      }

      if (ru->ifdevice.get_internal_parameter != NULL) {
        void *t = ru->ifdevice.get_internal_parameter("fh_if4p5_south_in");
        if (t != NULL)
          ru->fh_south_in = t;
        t = ru->ifdevice.get_internal_parameter("fh_if4p5_south_out");
        if (t != NULL)
          ru->fh_south_out = t;
      }

      malloc_IF4p5_buffer(ru);

      break;

    default:
      LOG_E(PHY,"RU with invalid or unknown southbound interface type %d\n",ru->if_south);
      break;
  } // switch on interface type

}

void init_RU0(RU_t *ru,int ru_id,char *rf_config_file, int send_dmrssync) {

  ru->rf_config_file = rf_config_file;
  ru->idx            = ru_id;
  ru->ts_offset      = 0;
  
  if (ru->is_slave == 1) {
    ru->in_synch = 0;
    ru->generate_dmrs_sync = 0;
  } else {
    ru->in_synch = 1;
    ru->generate_dmrs_sync = send_dmrssync;
  }
  
  ru->cmd = EMPTY;
  ru->south_out_cnt = 0;
  // use eNB_list[0] as a reference for RU frame parameters
  // NOTE: multiple CC_id are not handled here yet!
  
  //ru->generate_dmrs_sync = (ru->is_slave == 0) ? 1 : 0;
  if ((ru->is_slave == 0) && (ru->ota_sync_enable == 1))
    ru->generate_dmrs_sync = 1;
  else
    ru->generate_dmrs_sync = 0;
  
  ru->wakeup_L1_sleeptime = 2000;
  ru->wakeup_L1_sleep_cnt_max  = 3;
  

}

// This part if on eNB side 
void init_RU(RU_t **rup,int nb_RU,PHY_VARS_eNB ***eNBp,int nb_L1,int *nb_CC,char *rf_config_file, int send_dmrssync) {
  int ru_id, i, CC_id;
  RU_t *ru;
  PHY_VARS_eNB *eNB0     = (PHY_VARS_eNB *)NULL;
  LTE_DL_FRAME_PARMS *fp = (LTE_DL_FRAME_PARMS *)NULL;
  // create status mask
  if (nb_CC != NULL)
    for (i=0; i<nb_L1; i++)
      for (CC_id=0; CC_id<nb_CC[i]; CC_id++) eNBp[i][CC_id]->num_RU=0;

  LOG_D(PHY,"Process RUs RC.nb_RU:%d\n",nb_RU);

  for (ru_id=0; ru_id<nb_RU; ru_id++) {
    LOG_D(PHY,"Process ru[%d]\n",ru_id);
    ru                 = rup[ru_id];
    init_RU0(ru,ru_id,rf_config_file,send_dmrssync);
    ru->ru0 = rup[0];
    if (ru->num_eNB > 0) {
      LOG_D(PHY, "%s() ru[%d].num_eNB:%d ru->eNB_list[0]:%p eNB[0][0]:%p rf_config_file:%s\n", __FUNCTION__, ru_id, ru->num_eNB, ru->eNB_list[0], eNBp[0][0], ru->rf_config_file);
      
      if (ru->eNB_list[0] == 0) {
	LOG_E(PHY,"%s() DJP - ru->eNB_list ru->num_eNB are not initialized - so do it manually\n", __FUNCTION__);
	ru->eNB_list[0] = eNBp[0][0];
        ru->num_eNB=1;
        //
        // DJP - feptx_prec() / feptx_ofdm() parses the eNB_list (based on num_eNB) and copies the txdata_F to txdata in RU
        //
      } else {
        LOG_E(PHY,"DJP - delete code above this %s:%d\n", __FILE__, __LINE__);
      }
    }
    
    eNB0 = ru->eNB_list[0];
    fp   = ru->frame_parms;
    LOG_D(PHY, "RU FUnction:%d ru->if_south:%d\n", ru->function, ru->if_south);
    LOG_D(PHY, "eNB0:%p   fp:%p\n", eNB0, fp);

    if (eNB0) {
      if ((ru->function != NGFI_RRU_IF5) && (ru->function != NGFI_RRU_IF4p5))
        AssertFatal(eNB0!=NULL,"eNB0 is null!\n");

      if (eNB0) {
        LOG_I(PHY,"Copying frame parms from eNB %d to ru %d\n",eNB0->Mod_id,ru->idx);
        ru->frame_parms = &eNB0->frame_parms;
        // attach all RU to all eNBs in its list/
        LOG_D(PHY,"ru->num_eNB:%d eNB0->num_RU:%d\n", ru->num_eNB, eNB0->num_RU);

        for (i=0; i<ru->num_eNB; i++) {
          eNB0 = ru->eNB_list[i];
          eNB0->RU_list[eNB0->num_RU++] = ru;
        }
      }
    }

    LOG_I(PHY, "Initializing RRU descriptor %d : (%s,%s,%d)\n", ru_id, ru_if_types[ru->if_south], NB_timing[ru->if_timing], ru->function);
    set_function_spec_param(ru);
    if (ru->function != NGFI_RRU_IF4p5 && ru->function != NGFI_RRU_IF5) {
       fill_rf_config(ru,ru->rf_config_file);
       init_frame_parms(ru->frame_parms,1);
    }

    LOG_I(PHY, "Starting ru_thread %d, is_slave %d, send_dmrs %d\n", ru_id, ru->is_slave, ru->generate_dmrs_sync);
    init_RU_proc(ru);
  } // for ru_id

  //  sleep(1);
  LOG_D(HW, "[lte-softmodem.c] RU threads created\n");
}


void stop_ru(RU_t *ru) {
#if defined(PRE_SCD_THREAD) || defined(PHY_TX_THREAD)
  int *status;
#endif
  LOG_I(PHY,"Stopping RU %p processing threads\n",(void *)ru);
#if defined(PRE_SCD_THREAD)

  if(ru) {
    ru->proc.instance_pre_scd = 0;
    pthread_cond_signal( &ru->proc.cond_pre_scd );
    pthread_join(ru->proc.pthread_pre_scd, (void **)&status );
    pthread_mutex_destroy(&ru->proc.mutex_pre_scd );
    pthread_cond_destroy(&ru->proc.cond_pre_scd );
  }

#endif
#ifdef PHY_TX_THREAD

  if(ru) {
    ru->proc.instance_cnt_phy_tx = 0;
    pthread_cond_signal(&ru->proc.cond_phy_tx);
    pthread_join( ru->proc.pthread_phy_tx, (void **)&status );
    pthread_mutex_destroy( &ru->proc.mutex_phy_tx );
    pthread_cond_destroy( &ru->proc.cond_phy_tx );
    ru->proc.instance_cnt_rf_tx = 0;
    pthread_cond_signal(&ru->proc.cond_rf_tx);
    pthread_join( ru->proc.pthread_rf_tx, (void **)&status );
    pthread_mutex_destroy( &ru->proc.mutex_rf_tx );
    pthread_cond_destroy( &ru->proc.cond_rf_tx );
  }

#endif
}


void stop_RU(RU_t **rup,int nb_ru) {
  for (int inst = 0; inst < nb_ru; inst++) {
    LOG_I(PHY, "Stopping RU %d processing threads\n", inst);
    kill_RU_proc(rup[inst]);
  }
}

//Some of the member of ru pointer is used in pre_scd.
//This funtion is for initializing ru pointer for L2 FAPI simulator.
#if defined(PRE_SCD_THREAD)
void init_ru_vnf(void) {
  int ru_id;
  RU_t *ru;
  RU_proc_t *proc;
  //  PHY_VARS_eNB *eNB0= (PHY_VARS_eNB *)NULL;
  int i;
  int CC_id;
  dlsch_ue_select_tbl_in_use = 1;
  // create status mask
  RC.ru_mask = 0;
  pthread_mutex_init(&RC.ru_mutex,NULL);
  pthread_cond_init(&RC.ru_cond,NULL);
  // read in configuration file)
  LOG_I(PHY,"configuring RU from file\n");
  RC.ru = RCconfig_RU(RC.nb_RU,RC.nb_L1_inst,RC.eNB,&RC.ru_mask,&RC.ru_mutex,&RC.ru_cond);
  LOG_I(PHY,"number of L1 instances %d, number of RU %d, number of CPU cores %d\n",RC.nb_L1_inst,RC.nb_RU,get_nprocs());

  if (RC.nb_CC != 0)
    for (i=0; i<RC.nb_L1_inst; i++)
      for (CC_id=0; CC_id<RC.nb_CC[i]; CC_id++) RC.eNB[i][CC_id]->num_RU=0;

  LOG_D(PHY,"Process RUs RC.nb_RU:%d\n",RC.nb_RU);

  for (ru_id=0; ru_id<RC.nb_RU; ru_id++) {
    LOG_D(PHY,"Process RC.ru[%d]\n",ru_id);
    ru               = RC.ru[ru_id];
    //    ru->rf_config_file = rf_config_file;
    ru->idx          = ru_id;
    ru->ts_offset    = 0;
    // use eNB_list[0] as a reference for RU frame parameters
    // NOTE: multiple CC_id are not handled here yet!

    if (ru->num_eNB > 0) {
      //      LOG_D(PHY, "%s() RC.ru[%d].num_eNB:%d ru->eNB_list[0]:%p RC.eNB[0][0]:%p rf_config_file:%s\n", __FUNCTION__, ru_id, ru->num_eNB, ru->eNB_list[0], RC.eNB[0][0], ru->rf_config_file);
      if (ru->eNB_list[0] == 0) {
        LOG_E(PHY,"%s() DJP - ru->eNB_list ru->num_eNB are not initialized - so do it manually\n", __FUNCTION__);
        ru->eNB_list[0] = RC.eNB[0][0];
        ru->num_eNB=1;
        //
        // DJP - feptx_prec() / feptx_ofdm() parses the eNB_list (based on num_eNB) and copies the txdata_F to txdata in RU
        //
      } else {
        LOG_E(PHY,"DJP - delete code above this %s:%d\n", __FILE__, __LINE__);
      }
    }

    // frame_parms is not used in L2 FAPI simulator
    /*
        eNB0             = ru->eNB_list[0];
        LOG_D(PHY, "RU FUnction:%d ru->if_south:%d\n", ru->function, ru->if_south);
        LOG_D(PHY, "eNB0:%p\n", eNB0);
        if (eNB0)
        {
          if ((ru->function != NGFI_RRU_IF5) && (ru->function != NGFI_RRU_IF4p5))
            AssertFatal(eNB0!=NULL,"eNB0 is null!\n");

          if (eNB0) {
            LOG_I(PHY,"Copying frame parms from eNB %d to ru %d\n",eNB0->Mod_id,ru->idx);
            memcpy((void*)&ru->frame_parms,(void*)&eNB0->frame_parms,sizeof(LTE_DL_FRAME_PARMS));

            // attach all RU to all eNBs in its list/
            LOG_D(PHY,"ru->num_eNB:%d eNB0->num_RU:%d\n", ru->num_eNB, eNB0->num_RU);
            for (i=0;i<ru->num_eNB;i++) {
              eNB0 = ru->eNB_list[i];
              eNB0->RU_list[eNB0->num_RU++] = ru;
            }
          }
        }
    */
    LOG_I(PHY,"Initializing RRU descriptor %d : (%s,%s,%d)\n",ru_id,ru_if_types[ru->if_south],NB_timing[ru->if_timing],ru->function);
    //    set_function_spec_param(ru);
    LOG_I(PHY,"Starting ru_thread %d\n",ru_id);
    //    init_RU_proc(ru);
    proc = &ru->proc;
    memset((void *)proc,0,sizeof(RU_proc_t));
    proc->instance_pre_scd = -1;
    pthread_mutex_init( &proc->mutex_pre_scd, NULL);
    pthread_cond_init( &proc->cond_pre_scd, NULL);
    pthread_create(&proc->pthread_pre_scd, NULL, pre_scd_thread, (void *)ru);
    pthread_setname_np(proc->pthread_pre_scd, "pre_scd_thread");
  } // for ru_id

  //  sleep(1);
  LOG_D(HW,"[lte-softmodem.c] RU threads created\n");
}
#endif


/* --------------------------------------------------------*/
/* from here function to use configuration module          */
RU_t **RCconfig_RU(int nb_RU,int nb_L1_inst,PHY_VARS_eNB ***eNB,uint64_t *ru_mask,pthread_mutex_t *ru_mutex,pthread_cond_t *ru_cond) {
  int i = 0;
  paramdef_t RUParams[] = RUPARAMS_DESC;
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};
  config_getlist( &RUParamList,RUParams,sizeof(RUParams)/sizeof(paramdef_t), NULL);
  RU_t **ru=NULL;
  if ( RUParamList.numelt > 0) {
    ru = (RU_t **)malloc(nb_RU*sizeof(RU_t *));

    for (int j = 0; j < nb_RU; j++) {
      ru[j]                                    = (RU_t *)malloc(sizeof(RU_t));
      memset((void *)ru[j],0,sizeof(RU_t));
      ru[j]->idx                                 = j;
      LOG_I(PHY,"Creating ru[%d]:%p\n", j, ru[j]);
      ru[j]->if_timing                           = synch_to_ext_device;

      ru[j]->ru_mask = ru_mask;
      ru[j]->ru_mutex = ru_mutex;
      ru[j]->ru_cond = ru_cond;
      if (nb_L1_inst >0)
        ru[j]->num_eNB                           = RUParamList.paramarray[j][RU_ENB_LIST_IDX].numelt;
      else
        ru[j]->num_eNB                           = 0;

      for (i=0; i<ru[j]->num_eNB; i++) ru[j]->eNB_list[i] = eNB[RUParamList.paramarray[j][RU_ENB_LIST_IDX].iptr[i]][0];

      ru[j]->has_ctrl_prt                        = 0;

      if (config_isparamset(RUParamList.paramarray[j], RU_SDR_ADDRS)) {
        ru[j]->openair0_cfg.sdr_addrs = strdup(*(RUParamList.paramarray[j][RU_SDR_ADDRS].strptr));
      }

      if (config_isparamset(RUParamList.paramarray[j], RU_SDR_CLK_SRC)) {
        if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "internal") == 0) {
          ru[j]->openair0_cfg.clock_source = internal;
          LOG_D(PHY, "RU clock source set as internal\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "external") == 0) {
          ru[j]->openair0_cfg.clock_source = external;
          LOG_D(PHY, "RU clock source set as external\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "gpsdo") == 0) {
          ru[j]->openair0_cfg.clock_source = gpsdo;
          LOG_D(PHY, "RU clock source set as gpsdo\n");
        } else {
          LOG_E(PHY, "Erroneous RU clock source in the provided configuration file: '%s'\n", *(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr));
        }
      }
      else {
	ru[j]->openair0_cfg.clock_source = unset;
      }

      if (config_isparamset(RUParamList.paramarray[j], RU_SDR_TME_SRC)) {
        if (strcmp(*(RUParamList.paramarray[j][RU_SDR_TME_SRC].strptr), "internal") == 0) {
          ru[j]->openair0_cfg.time_source = internal;
          LOG_D(PHY, "RU time source set as internal\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_TME_SRC].strptr), "external") == 0) {
          ru[j]->openair0_cfg.time_source = external;
          LOG_D(PHY, "RU time source set as external\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_TME_SRC].strptr), "gpsdo") == 0) {
          ru[j]->openair0_cfg.time_source = gpsdo;
          LOG_D(PHY, "RU time source set as gpsdo\n");
        } else {
          LOG_E(PHY, "Erroneous RU time source in the provided configuration file: '%s'\n", *(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr));
        }
      }
      else {
	ru[j]->openair0_cfg.time_source = unset;
      }      

      LOG_I(PHY,"RU %d is_slave=%s\n",j,*(RUParamList.paramarray[j][RU_IS_SLAVE_IDX].strptr));

      if (strcmp(*(RUParamList.paramarray[j][RU_IS_SLAVE_IDX].strptr), "yes") == 0) ru[j]->is_slave=1;
      else ru[j]->is_slave=0;

      LOG_I(PHY,"RU %d ota_sync_enabled=%s\n",j,*(RUParamList.paramarray[j][RU_OTA_SYNC_ENABLE_IDX].strptr));

      if (strcmp(*(RUParamList.paramarray[j][RU_OTA_SYNC_ENABLE_IDX].strptr), "yes") == 0) ru[j]->ota_sync_enable=1;
      else ru[j]->ota_sync_enable=0;

      if (strcmp(*(RUParamList.paramarray[j][RU_LOCAL_RF_IDX].strptr), "yes") == 0) {
        if ( !(config_isparamset(RUParamList.paramarray[j],RU_LOCAL_IF_NAME_IDX)) ) {
          ru[j]->if_south                        = LOCAL_RF;
          ru[j]->function                        = eNodeB_3GPP;
          ru[j]->state                           = RU_RUN;
          printf("Setting function for RU %d to eNodeB_3GPP\n",j);
        } else {
          ru[j]->eth_params.local_if_name            = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));
          ru[j]->eth_params.my_addr                  = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr));
          ru[j]->eth_params.remote_addr              = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
          ru[j]->eth_params.my_portd                 = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
          ru[j]->eth_params.remote_portd             = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);

          // Check if control port set
          if  (!(config_isparamset(RUParamList.paramarray[j],RU_REMOTE_PORTC_IDX)) ) {
            LOG_I(PHY,"Removing control port for RU %d\n",j);
            ru[j]->has_ctrl_prt            = 0;
          } else {
            ru[j]->eth_params.my_portc                 = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
            ru[j]->eth_params.remote_portc             = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
            LOG_I(PHY," Control port %u \n",ru[j]->eth_params.my_portc);
          }

          if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp") == 0) {
            ru[j]->if_south                        = LOCAL_RF;
            ru[j]->function                        = NGFI_RRU_IF5;
            ru[j]->eth_params.transp_preference    = ETH_UDP_MODE;
            LOG_I(PHY,"Setting function for RU %d to NGFI_RRU_IF5 (udp)\n",j);
          } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw") == 0) {
            ru[j]->if_south                        = LOCAL_RF;
            ru[j]->function                        = NGFI_RRU_IF5;
            ru[j]->eth_params.transp_preference    = ETH_RAW_MODE;
            LOG_I(PHY,"Setting function for RU %d to NGFI_RRU_IF5 (raw)\n",j);
          } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_if4p5") == 0) {
            ru[j]->if_south                        = LOCAL_RF;
            ru[j]->function                        = NGFI_RRU_IF4p5;
            ru[j]->eth_params.transp_preference    = ETH_UDP_IF4p5_MODE;
            ru[j]->has_ctrl_prt                   =1;
            LOG_I(PHY,"Setting function for RU %d to NGFI_RRU_IF4p5 (udp)\n",j);
          } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw_if4p5") == 0) {
            ru[j]->if_south                        = LOCAL_RF;
            ru[j]->function                        = NGFI_RRU_IF4p5;
            ru[j]->eth_params.transp_preference    = ETH_RAW_IF4p5_MODE;
            ru[j]->has_ctrl_prt                   =1;
            LOG_I(PHY,"Setting function for RU %d to NGFI_RRU_IF4p5 (raw)\n",j);
          }
        }

        ru[j]->max_pdschReferenceSignalPower     = *(RUParamList.paramarray[j][RU_MAX_RS_EPRE_IDX].uptr);;
        ru[j]->max_rxgain                        = *(RUParamList.paramarray[j][RU_MAX_RXGAIN_IDX].uptr);
        ru[j]->num_bands                         = RUParamList.paramarray[j][RU_BAND_LIST_IDX].numelt;
        /* sf_extension is in unit of samples for 30.72MHz here, has to be scaled later */
        ru[j]->sf_extension                      = *(RUParamList.paramarray[j][RU_SF_EXTENSION_IDX].uptr);

        for (i=0; i<ru[j]->num_bands; i++) ru[j]->band[i] = RUParamList.paramarray[j][RU_BAND_LIST_IDX].iptr[i];
      } //strcmp(local_rf, "yes") == 0
      else {
        LOG_I(PHY,"RU %d: Transport %s\n",j,*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr));
        ru[j]->eth_params.local_if_name      = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));
        ru[j]->eth_params.my_addr            = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr));
        ru[j]->eth_params.remote_addr        = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
        ru[j]->eth_params.my_portc           = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
        ru[j]->eth_params.remote_portc       = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
        ru[j]->eth_params.my_portd           = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
        ru[j]->eth_params.remote_portd       = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);
      
        if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp") == 0) {
	  ru[j]->if_south                     = REMOTE_IF5;
	  ru[j]->function                     = NGFI_RAU_IF5;
	  ru[j]->eth_params.transp_preference = ETH_UDP_MODE;
        } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_ecpri_if5") == 0) {
	  ru[j]->if_south                     = REMOTE_IF5;
	  ru[j]->function                     = NGFI_RAU_IF5;
	  ru[j]->eth_params.transp_preference = ETH_UDP_IF5_ECPRI_MODE;
        } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw") == 0) {
	  ru[j]->if_south                     = REMOTE_IF5;
	  ru[j]->function                     = NGFI_RAU_IF5;
	  ru[j]->eth_params.transp_preference = ETH_RAW_MODE;
        } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "udp_if4p5") == 0) {
	  ru[j]->if_south                     = REMOTE_IF4p5;
	  ru[j]->function                     = NGFI_RAU_IF4p5;
  	  ru[j]->eth_params.transp_preference = ETH_UDP_IF4p5_MODE;
	  ru[j]->has_ctrl_prt                 = 1;
        } else if (strcmp(*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr), "raw_if4p5") == 0) {
	  ru[j]->if_south                     = REMOTE_IF4p5;
	  ru[j]->function                     = NGFI_RAU_IF4p5;
	  ru[j]->eth_params.transp_preference = ETH_RAW_IF4p5_MODE;
	  ru[j]->has_ctrl_prt                 = 1;
	
          if (strcmp(*(RUParamList.paramarray[j][RU_IS_SLAVE_IDX].strptr), "yes") == 0) ru[j]->is_slave=1;
          else ru[j]->is_slave=0;
        }
      }  /* strcmp(local_rf, "yes") != 0 */
      
      ru[j]->nb_tx                             = *(RUParamList.paramarray[j][RU_NB_TX_IDX].uptr);
      ru[j]->nb_rx                             = *(RUParamList.paramarray[j][RU_NB_RX_IDX].uptr);
      ru[j]->att_tx                            = *(RUParamList.paramarray[j][RU_ATT_TX_IDX].uptr);
      ru[j]->att_rx                            = *(RUParamList.paramarray[j][RU_ATT_RX_IDX].uptr);
      *ru_mask= (*ru_mask)|(1<<j);
    }// j=0..num_rus
  } 
    
  return ru;
}
