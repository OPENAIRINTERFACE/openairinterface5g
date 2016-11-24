/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

#include "PHY/types.h"

#include "PHY/defs.h"
#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "../../ARCH/COMMON/common_lib.h"

//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "PHY/LTE_TRANSPORT/if5_tools.h"

#include "PHY/extern.h"
#include "SCHED/extern.h"
#include "LAYER2/MAC/extern.h"

#include "../../SIMU/USER/init_lte.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/proto.h"
#include "RRC/LITE/extern.h"
#include "PHY_INTERFACE/extern.h"

#ifdef SMBV
#include "PHY/TOOLS/smbv.h"
unsigned short config_frames[4] = {2,9,11,13};
#endif
#include "UTIL/LOG/log_extern.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "enb_config.h"
//#include "PHY/TOOLS/time_meas.h"

#ifndef OPENAIR2
#include "UTIL/OTG/otg_extern.h"
#endif

#if defined(ENABLE_ITTI)
# if defined(ENABLE_USE_MME)
#   include "s1ap_eNB.h"
#ifdef PDCP_USE_NETLINK
#   include "SIMULATION/ETH_TRANSPORT/proto.h"
#endif
# endif
#endif

#include "T.h"

//#define DEBUG_THREADS 1

//#define USRP_DEBUG 1
struct timing_info_t {
  //unsigned int frame, hw_slot, last_slot, next_slot;
  RTIME time_min, time_max, time_avg, time_last, time_now;
  //unsigned int mbox0, mbox1, mbox2, mbox_target;
  unsigned int n_samples;
} timing_info;

// Fix per CC openair rf/if device update
// extern openair0_device openair0;

#if defined(ENABLE_ITTI)
extern volatile int             start_eNB;
extern volatile int             start_UE;
#endif
extern volatile int                    oai_exit;

extern openair0_config_t openair0_cfg[MAX_CARDS];

extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;

//pthread_t                       main_eNB_thread;

time_stats_t softmodem_stats_mt; // main thread
time_stats_t softmodem_stats_hw; //  hw acquisition
time_stats_t softmodem_stats_rxtx_sf; // total tx time
time_stats_t softmodem_stats_rx_sf; // total rx time
int32_t **rxdata;
int32_t **txdata;

uint8_t seqno; //sequence number

static int                      time_offset[4] = {0,0,0,0};

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

void exit_fun(const char* s);

void init_eNB(eNB_func_t node_function[], eNB_timing_t node_timing[],int nb_inst,eth_params_t *,int);
void stop_eNB(int nb_inst);


static inline void thread_top_init(char *thread_name,
				   int affinity,
				   uint64_t runtime,
				   uint64_t deadline,
				   uint64_t period) {

  MSC_START_USE();

#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;

  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  = runtime;
  attr.sched_deadline = deadline;
  attr.sched_period   = period; 

  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] eNB tx thread: sched_setattr failed\n");
    exit(1);
  }

#else //LOW_LATENCY
  int policy, s, j;
  struct sched_param sparam;
  char cpu_affinity[1024];
  cpu_set_t cpuset;

  /* Set affinity mask to include CPUs 1 to MAX_CPUS */
  /* CPU 0 is reserved for UHD threads */
  /* CPU 1 is reserved for all RX_TX threads */
  /* Enable CPU Affinity only if number of CPUs >2 */
  CPU_ZERO(&cpuset);

#ifdef CPU_AFFINITY
  if (get_nprocs() > 2)
  {
    if (affinity == 0)
      CPU_SET(0,&cpuset);
    else
      for (j = 1; j < get_nprocs(); j++)
        CPU_SET(j, &cpuset);
    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
      perror( "pthread_setaffinity_np");
      exit_fun("Error setting processor affinity");
    }
  }
#endif //CPU_AFFINITY

  /* Check the actual affinity mask assigned to the thread */
  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0) {
    perror( "pthread_getaffinity_np");
    exit_fun("Error getting processor affinity ");
  }
  memset(cpu_affinity,0,sizeof(cpu_affinity));
  for (j = 0; j < CPU_SETSIZE; j++)
    if (CPU_ISSET(j, &cpuset)) {  
      char temp[1024];
      sprintf (temp, " CPU_%d", j);
      strcat(cpu_affinity, temp);
    }

  memset(&sparam, 0, sizeof(sparam));
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
  policy = SCHED_FIFO ; 
  
  s = pthread_setschedparam(pthread_self(), policy, &sparam);
  if (s != 0) {
    perror("pthread_setschedparam : ");
    exit_fun("Error setting thread priority");
  }
  
  s = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (s != 0) {
    perror("pthread_getschedparam : ");
    exit_fun("Error getting thread priority");
  }

  LOG_I(HW, "[SCHED][eNB] %s started on CPU %d TID %ld, sched_policy = %s , priority = %d, CPU Affinity=%s \n",thread_name,sched_getcpu(),gettid(),
                   (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                   (policy == SCHED_RR)    ? "SCHED_RR" :
                   (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                   "???",
                   sparam.sched_priority, cpu_affinity );

#endif //LOW_LATENCY

  mlockall(MCL_CURRENT | MCL_FUTURE);

}

static inline void wait_sync(char *thread_name) {

  printf( "waiting for sync (%s)\n",thread_name);
  pthread_mutex_lock( &sync_mutex );
  
  while (sync_var<0)
    pthread_cond_wait( &sync_cond, &sync_mutex );
  
  pthread_mutex_unlock(&sync_mutex);
  
  printf( "got sync (%s)\n", thread_name);

}

void do_OFDM_mod_rt(int subframe,PHY_VARS_eNB *phy_vars_eNB) {
     
  unsigned int aa,slot_offset, slot_offset_F;
  int dummy_tx_b[7680*4] __attribute__((aligned(32)));
  int i,j, tx_offset;
  int slot_sizeF = (phy_vars_eNB->frame_parms.ofdm_symbol_size)*
                   ((phy_vars_eNB->frame_parms.Ncp==1) ? 6 : 7);
  int len,len2;
  int16_t *txdata;
//  int CC_id = phy_vars_eNB->proc.CC_id;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_SFGEN , 1 );

  slot_offset_F = (subframe<<1)*slot_sizeF;

  slot_offset = subframe*phy_vars_eNB->frame_parms.samples_per_tti;

  if ((subframe_select(&phy_vars_eNB->frame_parms,subframe)==SF_DL)||
      ((subframe_select(&phy_vars_eNB->frame_parms,subframe)==SF_S))) {
    //    LOG_D(HW,"Frame %d: Generating slot %d\n",frame,next_slot);

    for (aa=0; aa<phy_vars_eNB->frame_parms.nb_antennas_tx; aa++) {
      if (phy_vars_eNB->frame_parms.Ncp == EXTENDED) {
        PHY_ofdm_mod(&phy_vars_eNB->common_vars.txdataF[0][aa][slot_offset_F],
                     dummy_tx_b,
                     phy_vars_eNB->frame_parms.ofdm_symbol_size,
                     6,
                     phy_vars_eNB->frame_parms.nb_prefix_samples,
                     CYCLIC_PREFIX);
        PHY_ofdm_mod(&phy_vars_eNB->common_vars.txdataF[0][aa][slot_offset_F+slot_sizeF],
                     dummy_tx_b+(phy_vars_eNB->frame_parms.samples_per_tti>>1),
                     phy_vars_eNB->frame_parms.ofdm_symbol_size,
                     6,
                     phy_vars_eNB->frame_parms.nb_prefix_samples,
                     CYCLIC_PREFIX);
      } else {
        normal_prefix_mod(&phy_vars_eNB->common_vars.txdataF[0][aa][slot_offset_F],
                          dummy_tx_b,
                          7,
                          &(phy_vars_eNB->frame_parms));
	// if S-subframe generate first slot only
	if (subframe_select(&phy_vars_eNB->frame_parms,subframe) == SF_DL) 
	  normal_prefix_mod(&phy_vars_eNB->common_vars.txdataF[0][aa][slot_offset_F+slot_sizeF],
			    dummy_tx_b+(phy_vars_eNB->frame_parms.samples_per_tti>>1),
			    7,
			    &(phy_vars_eNB->frame_parms));
      }

      // if S-subframe generate first slot only
      if (subframe_select(&phy_vars_eNB->frame_parms,subframe) == SF_S)
	len = phy_vars_eNB->frame_parms.samples_per_tti>>1;
      else
	len = phy_vars_eNB->frame_parms.samples_per_tti;
      /*
      for (i=0;i<len;i+=4) {
	dummy_tx_b[i] = 0x100;
	dummy_tx_b[i+1] = 0x01000000;
	dummy_tx_b[i+2] = 0xff00;
	dummy_tx_b[i+3] = 0xff000000;
	}*/
      
      if (slot_offset+time_offset[aa]<0) {
	txdata = (int16_t*)&phy_vars_eNB->common_vars.txdata[0][aa][(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->frame_parms.samples_per_tti)+tx_offset];
        len2 = -(slot_offset+time_offset[aa]);
	len2 = (len2>len) ? len : len2;
	for (i=0; i<(len2<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
	if (len2<len) {
	  txdata = (int16_t*)&phy_vars_eNB->common_vars.txdata[0][aa][0];
	  for (j=0; i<(len<<1); i++,j++) {
	    txdata[j++] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	  }
	}
      }  
      else if ((slot_offset+time_offset[aa]+len)>(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->frame_parms.samples_per_tti)) {
	tx_offset = (int)slot_offset+time_offset[aa];
	txdata = (int16_t*)&phy_vars_eNB->common_vars.txdata[0][aa][tx_offset];
	len2 = -tx_offset+LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->frame_parms.samples_per_tti;
	for (i=0; i<(len2<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
	txdata = (int16_t*)&phy_vars_eNB->common_vars.txdata[0][aa][0];
	for (j=0; i<(len<<1); i++,j++) {
	  txdata[j++] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
      }
      else {
	tx_offset = (int)slot_offset+time_offset[aa];
	txdata = (int16_t*)&phy_vars_eNB->common_vars.txdata[0][aa][tx_offset];

	for (i=0; i<(len<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
      }
      
     // if S-subframe switch to RX in second subframe
      /*
     if (subframe_select(&phy_vars_eNB->frame_parms,subframe) == SF_S) {
       for (i=0; i<len; i++) {
	 phy_vars_eNB->common_vars.txdata[0][aa][tx_offset++] = 0x00010001;
       }
     }
      */
     if ((((phy_vars_eNB->frame_parms.tdd_config==0) ||
	   (phy_vars_eNB->frame_parms.tdd_config==1) ||
	   (phy_vars_eNB->frame_parms.tdd_config==2) ||
	   (phy_vars_eNB->frame_parms.tdd_config==6)) && 
	   (subframe==0)) || (subframe==5)) {
       // turn on tx switch N_TA_offset before
       //LOG_D(HW,"subframe %d, time to switch to tx (N_TA_offset %d, slot_offset %d) \n",subframe,phy_vars_eNB->N_TA_offset,slot_offset);
       for (i=0; i<phy_vars_eNB->N_TA_offset; i++) {
         tx_offset = (int)slot_offset+time_offset[aa]+i-phy_vars_eNB->N_TA_offset/2;
         if (tx_offset<0)
           tx_offset += LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->frame_parms.samples_per_tti;
	 
         if (tx_offset>=(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->frame_parms.samples_per_tti))
           tx_offset -= LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*phy_vars_eNB->frame_parms.samples_per_tti;
	 
         phy_vars_eNB->common_vars.txdata[0][aa][tx_offset] = 0x00000000;
       }
     }
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_SFGEN , 0 );
}

void tx_fh_if5(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc) {
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, proc->timestamp_tx&0xffffffff );
  send_IF5(eNB, proc->timestamp_tx, proc->subframe_tx, &seqno, IF5_RRH_GW_DL);
}

void tx_fh_if5_mobipass(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc) {
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, proc->timestamp_tx&0xffffffff );
  send_IF5(eNB, proc->timestamp_tx, proc->subframe_tx, &seqno, IF5_MOBIPASS); 
}

void tx_fh_if4p5(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc) {    
  send_IF4p5(eNB,proc->frame_tx, proc->subframe_tx, IF4p5_PDLFFT, 0);
}

void proc_tx_high0(PHY_VARS_eNB *eNB,
		   eNB_rxtx_proc_t *proc,
		   relaying_type_t r_type,
		   PHY_VARS_RN *rn) {

  int offset = proc == &eNB->proc.proc_rxtx[0] ? 0 : 1;

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_ENB+offset, proc->frame_tx );
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_ENB+offset, proc->subframe_tx );

  phy_procedures_eNB_TX(eNB,proc,r_type,rn,1);

  /* we're done, let the next one proceed */
  if (pthread_mutex_lock(&sync_phy_proc.mutex_phy_proc_tx) != 0) {
    LOG_E(PHY, "[SCHED][eNB] error locking PHY proc mutex for eNB TX proc\n");
    exit_fun("nothing to add");
  }	
  sync_phy_proc.phy_proc_CC_id++;
  sync_phy_proc.phy_proc_CC_id %= MAX_NUM_CCs;
  pthread_cond_broadcast(&sync_phy_proc.cond_phy_proc_tx);
  if (pthread_mutex_unlock(&sync_phy_proc.mutex_phy_proc_tx) != 0) {
    LOG_E(PHY, "[SCHED][eNB] error unlocking PHY proc mutex for eNB TX proc\n");
    exit_fun("nothing to add");
  }

}

void proc_tx_high(PHY_VARS_eNB *eNB,
		  eNB_rxtx_proc_t *proc,
		  relaying_type_t r_type,
		  PHY_VARS_RN *rn) {


  // do PHY high
  proc_tx_high0(eNB,proc,r_type,rn);

  // if TX fronthaul go ahead 
  if (eNB->tx_fh) eNB->tx_fh(eNB,proc);

}

void proc_tx_full(PHY_VARS_eNB *eNB,
		  eNB_rxtx_proc_t *proc,
		  relaying_type_t r_type,
		  PHY_VARS_RN *rn) {


  // do PHY high
  proc_tx_high0(eNB,proc,r_type,rn);
  // do OFDM modulation
  do_OFDM_mod_rt(proc->subframe_tx,eNB);
  // if TX fronthaul go ahead 
  if (eNB->tx_fh) eNB->tx_fh(eNB,proc);



}

void proc_tx_rru_if4p5(PHY_VARS_eNB *eNB,
		       eNB_rxtx_proc_t *proc,
		       relaying_type_t r_type,
		       PHY_VARS_RN *rn) {

  uint32_t symbol_number=0;
  uint32_t symbol_mask, symbol_mask_full;
  uint16_t packet_type;

  int offset = proc == &eNB->proc.proc_rxtx[0] ? 0 : 1;

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_ENB+offset, proc->frame_tx );
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_ENB+offset, proc->subframe_tx );

  /// **** recv_IF4 of txdataF from RCC **** ///             
  symbol_number = 0;
  symbol_mask = 0;
  symbol_mask_full = (1<<eNB->frame_parms.symbols_per_tti)-1;
  

  do { 
    recv_IF4p5(eNB, &proc->frame_tx, &proc->subframe_tx, &packet_type, &symbol_number);
    symbol_mask = symbol_mask | (1<<symbol_number);
  } while (symbol_mask != symbol_mask_full); 

  do_OFDM_mod_rt(proc->subframe_tx, eNB);
}

void proc_tx_rru_if5(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc) {
  int offset = proc == &eNB->proc.proc_rxtx[0] ? 0 : 1;

  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_ENB+offset, proc->frame_tx );
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_ENB+offset, proc->subframe_tx );
  /// **** recv_IF5 of txdata from BBU **** ///       
  recv_IF5(eNB, &proc->timestamp_tx, proc->subframe_tx, IF5_RRH_GW_DL);
}

int wait_CCs(eNB_rxtx_proc_t *proc) {

  struct timespec wait;

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  if (pthread_mutex_timedlock(&sync_phy_proc.mutex_phy_proc_tx,&wait) != 0) {
    LOG_E(PHY, "[SCHED][eNB] error locking PHY proc mutex for eNB TX\n");
    exit_fun("nothing to add");
    return(-1);
  }
  
  // wait for our turn or oai_exit
  while (sync_phy_proc.phy_proc_CC_id != proc->CC_id && !oai_exit) {
    pthread_cond_wait(&sync_phy_proc.cond_phy_proc_tx,
		      &sync_phy_proc.mutex_phy_proc_tx);
  }
  
  if (pthread_mutex_unlock(&sync_phy_proc.mutex_phy_proc_tx) != 0) {
    LOG_E(PHY, "[SCHED][eNB] error unlocking PHY proc mutex for eNB TX\n");
    exit_fun("nothing to add");
    return(-1);
  }
  return(0);
}

static inline int rxtx(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc, char *thread_name) {

  start_meas(&softmodem_stats_rxtx_sf);
  // ****************************************
  // Common RX procedures subframe n
  phy_procedures_eNB_common_RX(eNB);
  
  // UE-specific RX processing for subframe n
  if (eNB->proc_uespec_rx) eNB->proc_uespec_rx(eNB, proc, no_relay );
  
  // *****************************************
  // TX processing for subframe n+4
  // run PHY TX procedures the one after the other for all CCs to avoid race conditions
  // (may be relaxed in the future for performance reasons)
  // *****************************************
  //if (wait_CCs(proc)<0) return(-1);
  
  if (oai_exit) return(-1);
  
  if (eNB->proc_tx)	eNB->proc_tx(eNB, proc, no_relay, NULL );
  
  if (release_thread(&proc->mutex_rxtx,&proc->instance_cnt_rxtx,thread_name)<0) return(-1);

  stop_meas( &softmodem_stats_rxtx_sf );
  
  return(0);
}

/*!
 * \brief The RX UE-specific and TX thread of eNB.
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* eNB_thread_rxtx( void* param ) {

  static int eNB_thread_rxtx_status;

  eNB_rxtx_proc_t *proc = (eNB_rxtx_proc_t*)param;
  PHY_VARS_eNB *eNB = PHY_vars_eNB_g[0][proc->CC_id];

  char thread_name[100];


  // set default return value
  eNB_thread_rxtx_status = 0;


  sprintf(thread_name,"RXn_TXnp4_%d\n",&eNB->proc.proc_rxtx[0] == proc ? 0 : 1);
  thread_top_init(thread_name,1,850000L,1000000L,2000000L);

  while (!oai_exit) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RXTX0+(proc->subframe_rx&1), 0 );

    if (wait_on_condition(&proc->mutex_rxtx,&proc->cond_rxtx,&proc->instance_cnt_rxtx,thread_name)<0) break;

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RXTX0+(proc->subframe_rx&1), 1 );

    
  
    if (oai_exit) break;

    if (rxtx(eNB,proc,thread_name) < 0) break;

  } // while !oai_exit

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_eNB_PROC_RXTX0+(proc->subframe_rx&1), 0 );

  printf( "Exiting eNB thread RXn_TXnp4\n");

  eNB_thread_rxtx_status = 0;
  return &eNB_thread_rxtx_status;
}

#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)
/* Wait for eNB application initialization to be complete (eNB registration to MME) */
static void wait_system_ready (char *message, volatile int *start_flag) {
  
  static char *indicator[] = {".    ", "..   ", "...  ", ".... ", ".....",
			      " ....", "  ...", "   ..", "    .", "     "};
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


// asynchronous UL with IF4p5 (RCC,RAU,eNodeB_BBU)
void fh_if5_asynch_UL(PHY_VARS_eNB *eNB,int *frame,int *subframe) {

  eNB_proc_t *proc       = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;

  recv_IF5(eNB, &proc->timestamp_rx, *subframe, IF5_RRH_GW_UL); 

  proc->subframe_rx = (proc->timestamp_rx/fp->samples_per_tti)%10;
  proc->frame_rx    = (proc->timestamp_rx/(10*fp->samples_per_tti))&1023;

  if (proc->first_rx != 0) {
    proc->first_rx = 0;
    *subframe = proc->subframe_rx;
    *frame    = proc->frame_rx; 
  }
  else {
    if (proc->subframe_rx != *subframe) {
      LOG_E(PHY,"subframe_rx %d is not what we expect %d\n",proc->subframe_rx,*subframe);
      exit_fun("Exiting");
    }
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"subframe_rx %d is not what we expect %d\n",proc->frame_rx,*frame);  
      exit_fun("Exiting");
    }
  }
} // eNodeB_3GPP_BBU 

// asynchronous UL with IF4p5 (RCC,RAU,eNodeB_BBU)
void fh_if4p5_asynch_UL(PHY_VARS_eNB *eNB,int *frame,int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  eNB_proc_t *proc       = &eNB->proc;

  uint16_t packet_type;
  uint32_t symbol_number,symbol_mask,symbol_mask_full,prach_rx;


  symbol_number = 0;
  symbol_mask = 0;
  symbol_mask_full = (1<<fp->symbols_per_tti)-1;
  prach_rx = 0;

  do {   // Blocking, we need a timeout on this !!!!!!!!!!!!!!!!!!!!!!!
    recv_IF4p5(eNB, &proc->frame_rx, &proc->subframe_rx, &packet_type, &symbol_number);
    if (proc->first_rx != 0) {
      *frame = proc->frame_rx;
      *subframe = proc->subframe_rx;
      proc->first_rx = 0;
    }
    else {
      if (proc->frame_rx != *frame) {
	LOG_E(PHY,"frame_rx %d is not what we expect %d\n",proc->frame_rx,*frame);
	exit_fun("Exiting");
      }
      if (proc->subframe_rx != *subframe) {
	LOG_E(PHY,"subframe_rx %d is not what we expect %d\n",proc->subframe_rx,*subframe);
	exit_fun("Exiting");
      }
    }
    if (packet_type == IF4p5_PULFFT) {
      symbol_mask = symbol_mask | (1<<symbol_number);
      prach_rx = (is_prach_subframe(fp, proc->frame_rx, proc->subframe_rx)>0) ? 1 : 0;                            
    } else if (packet_type == IF4p5_PRACH) {
      prach_rx = 0;
    }
  } while( (symbol_mask != symbol_mask_full) || (prach_rx == 1));    
  

} 


void fh_if5_asynch_DL(PHY_VARS_eNB *eNB,int *frame,int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  eNB_proc_t *proc       = &eNB->proc;
  int subframe_tx,frame_tx;
  openair0_timestamp timestamp_tx;

  recv_IF5(eNB, &timestamp_tx, *subframe, IF5_RRH_GW_DL); 
      //      printf("Received subframe %d (TS %llu) from RCC\n",subframe_tx,timestamp_tx);

  subframe_tx = (timestamp_tx/fp->samples_per_tti)%10;
  frame_tx    = (timestamp_tx/(fp->samples_per_tti*10))&1023;

  if (proc->first_tx != 0) {
    *subframe = subframe_tx;
    *frame    = frame_tx;
    proc->first_tx = 0;
  }
  else {
    if (subframe_tx != *subframe) {
      LOG_E(PHY,"subframe_tx %d is not what we expect %d\n",subframe_tx,*subframe);
      exit_fun("Exiting");
    }
    if (frame_tx != *frame) { 
      LOG_E(PHY,"frame_tx %d is not what we expect %d\n",frame_tx,*frame);
      exit_fun("Exiting");
    }
  }
}

void fh_if4p5_asynch_DL(PHY_VARS_eNB *eNB,int *frame,int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  eNB_proc_t *proc       = &eNB->proc;

  uint16_t packet_type;
  uint32_t symbol_number,symbol_mask,symbol_mask_full;
  int subframe_tx,frame_tx;

  symbol_number = 0;
  symbol_mask = 0;
  symbol_mask_full = (1<<fp->symbols_per_tti)-1;

  do {   // Blocking, we need a timeout on this !!!!!!!!!!!!!!!!!!!!!!!
    recv_IF4p5(eNB, &frame_tx, &subframe_tx, &packet_type, &symbol_number);
    if (proc->first_tx != 0) {
      *frame    = frame_tx;
      *subframe = subframe_tx;
      proc->first_tx = 0;
    }
    else {
      if (frame_tx != *frame) {
	LOG_E(PHY,"frame_tx %d is not what we expect %d\n",frame_tx,*frame);
	exit_fun("Exiting");
      }
      if (subframe_tx != *subframe) {
	LOG_E(PHY,"subframe_tx %d is not what we expect %d\n",subframe_tx,*subframe);
	exit_fun("Exiting");
      }
    }
    if (packet_type == IF4p5_PDLFFT) {
      symbol_mask = symbol_mask | (1<<symbol_number);
    }
    else {
      LOG_E(PHY,"Illegal IF4p5 packet type (should only be IF4p5_PDLFFT%d\n",packet_type);
      exit_fun("Exiting");
    }
  } while (symbol_mask != symbol_mask_full);    
  
  do_OFDM_mod_rt(subframe_tx, eNB);
} 

/*!
 * \brief The Asynchronous RX/TX FH thread of RAU/RCC/eNB/RRU.
 * This handles the RX FH for an asynchronous RRU/UE
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* eNB_thread_asynch_rxtx( void* param ) {

  static int eNB_thread_asynch_rxtx_status;

  eNB_proc_t *proc = (eNB_proc_t*)param;
  PHY_VARS_eNB *eNB = PHY_vars_eNB_g[0][proc->CC_id];



  int subframe=0, frame=0; 

  thread_top_init("thread_asynch",1,870000L,1000000L,1000000L);

  // wait for top-level synchronization and do one acquisition to get timestamp for setting frame/subframe

  wait_sync("thread_asynch");

  // wait for top-level synchronization and do one acquisition to get timestamp for setting frame/subframe
  printf( "waiting for devices (eNB_thread_asynch_rx)\n");

  wait_on_condition(&proc->mutex_asynch_rxtx,&proc->cond_asynch_rxtx,&proc->instance_cnt_asynch_rxtx,"thread_asynch");

  printf( "devices ok (eNB_thread_asynch_rx)\n");


  while (!oai_exit) { 
   
    if (oai_exit) break;   

    if (subframe==9) { 
      subframe=0;
      frame++;
      frame&=1023;
    } else {
      subframe++;
    }      

    if (eNB->fh_asynch) eNB->fh_asynch(eNB,&frame,&subframe);
    else AssertFatal(1==0, "Unknown eNB->node_function %d",eNB->node_function);
    
  }

  eNB_thread_asynch_rxtx_status=0;
  return(&eNB_thread_asynch_rxtx_status);
}





void rx_rf(PHY_VARS_eNB *eNB,int *frame,int *subframe) {

  eNB_proc_t *proc = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  void *rxp[fp->nb_antennas_rx],*txp[fp->nb_antennas_tx]; 
  unsigned int rxs,txs;
  int i;
  int tx_sfoffset = 3;//(eNB->single_thread_flag == 1) ? 3 : 3;
  if (proc->first_rx==0) {
    
    // Transmit TX buffer based on timestamp from RX
    //    printf("trx_write -> USRP TS %llu (sf %d)\n", (proc->timestamp_rx+(3*fp->samples_per_tti)),(proc->subframe_rx+2)%10);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, (proc->timestamp_rx+(tx_sfoffset*fp->samples_per_tti)-openair0_cfg[0].tx_sample_advance)&0xffffffff );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
    // prepare tx buffer pointers
	
    for (i=0; i<fp->nb_antennas_tx; i++)
      txp[i] = (void*)&eNB->common_vars.txdata[0][i][((proc->subframe_rx+tx_sfoffset)%10)*fp->samples_per_tti];
    
    txs = eNB->rfdevice.trx_write_func(&eNB->rfdevice,
				       proc->timestamp_rx+(tx_sfoffset*fp->samples_per_tti)-openair0_cfg[0].tx_sample_advance,
				       txp,
				       fp->samples_per_tti,
				       fp->nb_antennas_tx,
				       1);
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );
    
    
    
    if (txs !=  fp->samples_per_tti) {
      LOG_E(PHY,"TX : Timeout (sent %d/%d)\n",txs, fp->samples_per_tti);
      exit_fun( "problem transmitting samples" );
    }	
  }
  
  for (i=0; i<fp->nb_antennas_rx; i++)
    rxp[i] = (void*)&eNB->common_vars.rxdata[0][i][*subframe*fp->samples_per_tti];
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );

  rxs = eNB->rfdevice.trx_read_func(&eNB->rfdevice,
				    &(proc->timestamp_rx),
				    rxp,
				    fp->samples_per_tti,
				    fp->nb_antennas_rx);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );
  
  proc->frame_rx    = (proc->timestamp_rx / (fp->samples_per_tti*10))&1023;
  proc->subframe_rx = (proc->timestamp_rx / fp->samples_per_tti)%10;
  proc->timestamp_tx = proc->timestamp_rx+(4*fp->samples_per_tti);
  //  printf("trx_read <- USRP TS %llu (sf %d, first_rx %d)\n", proc->timestamp_rx,proc->subframe_rx,proc->first_rx);  
  
  if (proc->first_rx == 0) {
    if (proc->subframe_rx != *subframe){
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d)\n",proc->subframe_rx,*subframe);
      exit_fun("Exiting");
    }
    
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",proc->frame_rx,*frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->subframe_rx;        
  }
  
  //printf("timestamp_rx %lu, frame %d(%d), subframe %d(%d)\n",proc->timestamp_rx,proc->frame_rx,frame,proc->subframe_rx,subframe);
  
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );
  
  if (rxs != fp->samples_per_tti)
    exit_fun( "problem receiving samples" );
  

  
}

void rx_fh_if5(PHY_VARS_eNB *eNB,int *frame, int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  eNB_proc_t *proc = &eNB->proc;

  recv_IF5(eNB, &proc->timestamp_rx, *subframe, IF5_RRH_GW_UL); 

  proc->frame_rx    = (proc->timestamp_rx / (fp->samples_per_tti*10))&1023;
  proc->subframe_rx = (proc->timestamp_rx / fp->samples_per_tti)%10;
  
  if (proc->first_rx == 0) {
    if (proc->subframe_rx != *subframe){
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d)\n",proc->subframe_rx,subframe);
      exit_fun("Exiting");
    }
    
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",proc->frame_rx,frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->subframe_rx;        
  }      
  
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );

}


void rx_fh_if4p5(PHY_VARS_eNB *eNB,int *frame,int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  eNB_proc_t *proc = &eNB->proc;

  int prach_rx;

  uint16_t packet_type;
  uint32_t symbol_number=0;
  uint32_t symbol_mask, symbol_mask_full;

  symbol_mask = 0;
  symbol_mask_full = (1<<fp->symbols_per_tti)-1;
  prach_rx = 0;

  do {   // Blocking, we need a timeout on this !!!!!!!!!!!!!!!!!!!!!!!
    recv_IF4p5(eNB, &proc->frame_rx, &proc->subframe_rx, &packet_type, &symbol_number);

    if (packet_type == IF4p5_PULFFT) {
      symbol_mask = symbol_mask | (1<<symbol_number);
      prach_rx = (is_prach_subframe(fp, proc->frame_rx, proc->subframe_rx)>0) ? 1 : 0;                            
    } else if (packet_type == IF4p5_PRACH) {
      prach_rx = 0;
    }

  } while( (symbol_mask != symbol_mask_full) || (prach_rx == 1));    

  //caculate timestamp_rx, timestamp_tx based on frame and subframe
   proc->timestamp_rx = ((proc->frame_rx * 10)  + proc->subframe_rx ) * fp->samples_per_tti ;
   proc->timestamp_tx = proc->timestamp_rx +  (4*fp->samples_per_tti);
 
 
  if (proc->first_rx == 0) {
    if (proc->subframe_rx != *subframe){
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d)\n",proc->subframe_rx,*subframe);
      exit_fun("Exiting");
    }
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",proc->frame_rx,*frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->subframe_rx;        
  }
  
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );
  
}

void rx_fh_slave(PHY_VARS_eNB *eNB,int *frame,int *subframe) {
  // This case is for synchronization to another thread
  // it just waits for an external event.  The actual rx_fh is handle by the asynchronous RX thread
  eNB_proc_t *proc=&eNB->proc;

  if (wait_on_condition(&proc->mutex_FH,&proc->cond_FH,&proc->instance_cnt_FH,"rx_fh_slave") < 0)
    return;

  release_thread(&proc->mutex_FH,&proc->instance_cnt_FH,"rx_fh_slave");

  
}


int wakeup_rxtx(eNB_proc_t *proc,eNB_rxtx_proc_t *proc_rxtx,LTE_DL_FRAME_PARMS *fp) {

  int i;
  struct timespec wait;
  
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  /* accept some delay in processing - up to 5ms */
  for (i = 0; i < 10 && proc_rxtx->instance_cnt_rxtx == 0; i++) {
    LOG_W( PHY,"[eNB] Frame %d, eNB RXn-TXnp4 thread busy!! (cnt_rxtx %i)\n", proc_rxtx->frame_tx, proc_rxtx->instance_cnt_rxtx);
    usleep(500);
  }
  if (proc_rxtx->instance_cnt_rxtx == 0) {
    exit_fun( "TX thread busy" );
    return(-1);
  }

  // wake up TX for subframe n+4
  // lock the TX mutex and make sure the thread is ready
  if (pthread_mutex_timedlock(&proc_rxtx->mutex_rxtx,&wait) != 0) {
    LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB RXTX thread %d (IC %d)\n", proc_rxtx->subframe_rx&1,proc_rxtx->instance_cnt_rxtx );
    exit_fun( "error locking mutex_rxtx" );
    return(-1);
  }
  
  ++proc_rxtx->instance_cnt_rxtx;
  
  // We have just received and processed the common part of a subframe, say n. 
  // TS_rx is the last received timestamp (start of 1st slot), TS_tx is the desired 
  // transmitted timestamp of the next TX slot (first).
  // The last (TS_rx mod samples_per_frame) was n*samples_per_tti, 
  // we want to generate subframe (n+4), so TS_tx = TX_rx+4*samples_per_tti,
  // and proc->subframe_tx = proc->subframe_rx+4
  proc_rxtx->timestamp_tx = proc->timestamp_rx + (4*fp->samples_per_tti);
  proc_rxtx->frame_rx     = proc->frame_rx;
  proc_rxtx->subframe_rx  = proc->subframe_rx;
  proc_rxtx->frame_tx     = (proc_rxtx->subframe_rx > 5) ? (proc_rxtx->frame_rx+1)&1023 : proc_rxtx->frame_rx;
  proc_rxtx->subframe_tx  = (proc_rxtx->subframe_rx + 4)%10;
  
  // the thread can now be woken up
  if (pthread_cond_signal(&proc_rxtx->cond_rxtx) != 0) {
    LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB RXn-TXnp4 thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return(-1);
  }
  
  pthread_mutex_unlock( &proc_rxtx->mutex_rxtx );

  return(0);
}

void wakeup_slaves(eNB_proc_t *proc) {

  int i;
  struct timespec wait;
  
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;
  
  for (i=0;i<proc->num_slaves;i++) {
    eNB_proc_t *slave_proc = proc->slave_proc[i];
    // wake up slave FH thread
    // lock the FH mutex and make sure the thread is ready
    if (pthread_mutex_timedlock(&slave_proc->mutex_FH,&wait) != 0) {
      LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB CCid %d slave CCid %d (IC %d)\n",proc->CC_id,slave_proc->CC_id);
      exit_fun( "error locking mutex_rxtx" );
      break;
    }
    
    int cnt_slave            = ++slave_proc->instance_cnt_FH;
    slave_proc->frame_rx     = proc->frame_rx;
    slave_proc->subframe_rx  = proc->subframe_rx;
    slave_proc->timestamp_rx = proc->timestamp_rx;
    slave_proc->timestamp_tx = proc->timestamp_tx; 

    pthread_mutex_unlock( &slave_proc->mutex_FH );
    
    if (cnt_slave == 0) {
      // the thread was presumably waiting where it should and can now be woken up
      if (pthread_cond_signal(&slave_proc->cond_FH) != 0) {
	LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB CCid %d, slave CCid %d\n",proc->CC_id,slave_proc->CC_id);
          exit_fun( "ERROR pthread_cond_signal" );
	  break;
      }
    } else {
      LOG_W( PHY,"[eNB] Frame %d, slave CC_id %d thread busy!! (cnt_FH %i)\n",slave_proc->frame_rx,slave_proc->CC_id, cnt_slave);
      exit_fun( "FH thread busy" );
      break;
    }             
  }
}

/*!
 * \brief The Fronthaul thread of RRU/RAU/RCC/eNB
 * In the case of RRU/eNB, handles interface with external RF
 * In the case of RAU/RCC, handles fronthaul interface with RRU/RAU
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */

static void* eNB_thread_FH( void* param ) {
  
  static int eNB_thread_FH_status;

  eNB_proc_t *proc = (eNB_proc_t*)param;
  PHY_VARS_eNB *eNB = PHY_vars_eNB_g[0][proc->CC_id];
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;

  int subframe=0, frame=0; 

  // set default return value
  eNB_thread_FH_status = 0;

  thread_top_init("eNB_thread_FH",0,870000,1000000,1000000);

  wait_sync("eNB_thread_FH");

#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)
  if (eNB->node_function < NGFI_RRU_IF5)
    wait_system_ready ("Waiting for eNB application to be ready %s\r", &start_eNB);
#endif 

  // Start IF device if any
  if (eNB->start_if) 
    if (eNB->start_if(eNB) != 0)
      LOG_E(HW,"Could not start the IF device\n");

  // Start RF device if any
  if (eNB->start_rf)
    if (eNB->start_rf(eNB) != 0)
      LOG_E(HW,"Could not start the RF device\n");

  // wakeup asnych_rxtx thread because the devices are ready at this point
  pthread_mutex_lock(&proc->mutex_asynch_rxtx);
  proc->instance_cnt_asynch_rxtx=0;
  pthread_mutex_unlock(&proc->mutex_asynch_rxtx);
  pthread_cond_signal(&proc->cond_asynch_rxtx);

  // This is a forever while loop, it loops over subframes which are scheduled by incoming samples from HW devices
  while (!oai_exit) {

    // these are local subframe/frame counters to check that we are in synch with the fronthaul timing.
    // They are set on the first rx/tx in the underly FH routines.
    if (subframe==9) { 
      subframe=0;
      frame++;
      frame&=1023;
    } else {
      subframe++;
    }      

 
    // synchronization on FH interface, acquire signals/data and block
    if (eNB->rx_fh) eNB->rx_fh(eNB,&frame,&subframe);
    else AssertFatal(1==0, "No fronthaul interface : eNB->node_function %d",eNB->node_function);

    T(T_ENB_MASTER_TICK, T_INT(0), T_INT(proc->frame_rx), T_INT(proc->subframe_rx));

    // At this point, all information for subframe has been received on FH interface
    // If this proc is to provide synchronization, do so
    wakeup_slaves(proc);
      
    // wake up RXn_TXnp4 thread for the subframe
    // choose even or odd thread for RXn-TXnp4 processing 
    if (wakeup_rxtx(proc,&proc->proc_rxtx[proc->subframe_rx&1],fp) < 0)
      break;

    // artifical sleep for very slow fronthaul
    if (eNB->frame_parms.N_RB_DL==6)
      rt_sleep_ns(800000LL);
  }
    
  printf( "Exiting FH thread \n");
 
  eNB_thread_FH_status = 0;
  return &eNB_thread_FH_status;
}


/*!
 * \brief The prach receive thread of eNB.
 * \param param is a \ref eNB_proc_t structure which contains the info what to process.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void* eNB_thread_prach( void* param ) {
  static int eNB_thread_prach_status;

  eNB_proc_t *proc = (eNB_proc_t*)param;
  PHY_VARS_eNB *eNB= PHY_vars_eNB_g[0][proc->CC_id];

  // set default return value
  eNB_thread_prach_status = 0;

  thread_top_init("eNB_thread_prach",1,500000L,1000000L,20000000L);

  while (!oai_exit) {
    
    if (oai_exit) break;

    if (wait_on_condition(&proc->mutex_prach,&proc->cond_prach,&proc->instance_cnt_prach,"eNB_prach_thread") < 0) break;
    
    prach_procedures(eNB);
    
    if (release_thread(&proc->mutex_prach,&proc->instance_cnt_prach,"eNB_prach_thread") < 0) break;
  }

  printf( "Exiting eNB thread PRACH\n");

  eNB_thread_prach_status = 0;
  return &eNB_thread_prach_status;
}

static void* eNB_thread_single( void* param ) {

  static int eNB_thread_single_status;

  eNB_proc_t *proc = (eNB_proc_t*)param;
  eNB_rxtx_proc_t *proc_rxtx = &proc->proc_rxtx[0];
  PHY_VARS_eNB *eNB = PHY_vars_eNB_g[0][proc->CC_id];

  int subframe=0, frame=0; 

  // set default return value
  eNB_thread_single_status = 0;

  thread_top_init("eNB_thread_single",0,870000,1000000,1000000);

  wait_sync("eNB_thread_single");

#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)
  if (eNB->node_function < NGFI_RRU_IF5)
    wait_system_ready ("Waiting for eNB application to be ready %s\r", &start_eNB);
#endif 

  // Start IF device if any
  if (eNB->start_if) 
    if (eNB->start_if(eNB) != 0)
      LOG_E(HW,"Could not start the IF device\n");

  // Start RF device if any
  if (eNB->start_rf)
    if (eNB->start_rf(eNB) != 0)
      LOG_E(HW,"Could not start the RF device\n");

  // wakeup asnych_rxtx thread because the devices are ready at this point
  pthread_mutex_lock(&proc->mutex_asynch_rxtx);
  proc->instance_cnt_asynch_rxtx=0;
  pthread_mutex_unlock(&proc->mutex_asynch_rxtx);
  pthread_cond_signal(&proc->cond_asynch_rxtx);

  // This is a forever while loop, it loops over subframes which are scheduled by incoming samples from HW devices
  while (!oai_exit) {

    // these are local subframe/frame counters to check that we are in synch with the fronthaul timing.
    // They are set on the first rx/tx in the underly FH routines.
    if (subframe==9) { 
      subframe=0;
      frame++;
      frame&=1023;
    } else {
      subframe++;
    }      

    LOG_D(PHY,"eNB Fronthaul thread, frame %d, subframe %d\n",frame,subframe);
 
    // synchronization on FH interface, acquire signals/data and block
    if (eNB->rx_fh) eNB->rx_fh(eNB,&frame,&subframe);
    else AssertFatal(1==0, "No fronthaul interface : eNB->node_function %d",eNB->node_function);

    T(T_ENB_MASTER_TICK, T_INT(0), T_INT(proc->frame_rx), T_INT(proc->subframe_rx));

    proc_rxtx->subframe_rx = proc->subframe_rx;
    proc_rxtx->frame_rx    = proc->frame_rx;
    proc_rxtx->subframe_tx = (proc->subframe_rx+4)%10;
    proc_rxtx->frame_tx    = (proc->subframe_rx < 6) ? proc->frame_rx : (proc->frame_rx+1)&1023; 
    proc_rxtx->timestamp_tx = proc->timestamp_tx;

    // At this point, all information for subframe has been received on FH interface
    // If this proc is to provide synchronization, do so
    wakeup_slaves(proc);

    if (rxtx(eNB,proc_rxtx,"eNB_thread_single") < 0) break;
  }
  

  printf( "Exiting eNB_single thread \n");
 
  eNB_thread_single_status = 0;
  return &eNB_thread_single_status;

}

extern void init_fep_thread(PHY_VARS_eNB *, pthread_attr_t *);
extern void init_td_thread(PHY_VARS_eNB *, pthread_attr_t *);
extern void init_te_thread(PHY_VARS_eNB *, pthread_attr_t *);

void init_eNB_proc(int inst) {
  
  int i=0;
  int CC_id;
  PHY_VARS_eNB *eNB;
  eNB_proc_t *proc;
  eNB_rxtx_proc_t *proc_rxtx;
  pthread_attr_t *attr0=NULL,*attr1=NULL,*attr_FH=NULL,*attr_prach=NULL,*attr_asynch=NULL,*attr_single=NULL,*attr_fep=NULL,*attr_td=NULL,*attr_te;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    eNB = PHY_vars_eNB_g[inst][CC_id];
#ifndef OCP_FRAMEWORK
    LOG_I(PHY,"Initializing eNB %d CC_id %d (%s,%s),\n",inst,CC_id,eNB_functions[eNB->node_function],eNB_timing[eNB->node_timing]);
#endif
    proc = &eNB->proc;

    proc_rxtx = proc->proc_rxtx;
    proc_rxtx[0].instance_cnt_rxtx = -1;
    proc_rxtx[1].instance_cnt_rxtx = -1;
    proc->instance_cnt_prach       = -1;
    proc->instance_cnt_FH          = -1;
    proc->instance_cnt_asynch_rxtx = -1;
    proc->CC_id = CC_id;    
    
    proc->first_rx=1;
    proc->first_tx=1;

    pthread_mutex_init( &proc_rxtx[0].mutex_rxtx, NULL);
    pthread_mutex_init( &proc_rxtx[1].mutex_rxtx, NULL);
    pthread_cond_init( &proc_rxtx[0].cond_rxtx, NULL);
    pthread_cond_init( &proc_rxtx[1].cond_rxtx, NULL);

    pthread_mutex_init( &proc->mutex_prach, NULL);
    pthread_mutex_init( &proc->mutex_asynch_rxtx, NULL);

    pthread_cond_init( &proc->cond_prach, NULL);
    pthread_cond_init( &proc->cond_FH, NULL);
    pthread_cond_init( &proc->cond_asynch_rxtx, NULL);

    pthread_attr_init( &proc->attr_FH);
    pthread_attr_init( &proc->attr_prach);
    pthread_attr_init( &proc->attr_asynch_rxtx);
    pthread_attr_init( &proc->attr_single);
    pthread_attr_init( &proc->attr_fep);
    pthread_attr_init( &proc->attr_td);
    pthread_attr_init( &proc->attr_te);
    pthread_attr_init( &proc_rxtx[0].attr_rxtx);
    pthread_attr_init( &proc_rxtx[1].attr_rxtx);
#ifndef DEADLINE_SCHEDULER
    attr0       = &proc_rxtx[0].attr_rxtx;
    attr1       = &proc_rxtx[1].attr_rxtx;
    attr_FH     = &proc->attr_FH;
    attr_prach  = &proc->attr_prach;
    attr_asynch = &proc->attr_asynch_rxtx;
    attr_single = &proc->attr_single;
    attr_fep    = &proc->attr_fep;
    attr_td     = &proc->attr_td;
    attr_te     = &proc->attr_te; 
#endif

    if (eNB->single_thread_flag==0) {
      pthread_create( &proc_rxtx[0].pthread_rxtx, attr0, eNB_thread_rxtx, &proc_rxtx[0] );
      pthread_create( &proc_rxtx[1].pthread_rxtx, attr1, eNB_thread_rxtx, &proc_rxtx[1] );
      pthread_create( &proc->pthread_FH, attr_FH, eNB_thread_FH, &eNB->proc );
    }
    else {
      pthread_create(&proc->pthread_single, attr_single, eNB_thread_single, &eNB->proc);
      init_fep_thread(eNB,attr_fep);
      init_td_thread(eNB,attr_td);
      init_te_thread(eNB,attr_te);
    }
    pthread_create( &proc->pthread_prach, attr_prach, eNB_thread_prach, &eNB->proc );
    if ((eNB->node_timing == synch_to_other) ||
	(eNB->node_function == NGFI_RRU_IF5) ||
	(eNB->node_function == NGFI_RRU_IF4p5))


      pthread_create( &proc->pthread_asynch_rxtx, attr_asynch, eNB_thread_asynch_rxtx, &eNB->proc );

    char name[16];
    if (eNB->single_thread_flag == 0) {
      snprintf( name, sizeof(name), "RXTX0 %d", i );
      pthread_setname_np( proc_rxtx[0].pthread_rxtx, name );
      snprintf( name, sizeof(name), "RXTX1 %d", i );
      pthread_setname_np( proc_rxtx[1].pthread_rxtx, name );
      snprintf( name, sizeof(name), "FH %d", i );
      pthread_setname_np( proc->pthread_FH, name );
    }
    else {
      snprintf( name, sizeof(name), " %d", i );
      pthread_setname_np( proc->pthread_single, name );
    }
  }

  //for multiple CCs: setup master and slaves
  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    eNB = PHY_vars_eNB_g[inst][CC_id];

    if (eNB->node_timing == synch_to_ext_device) { //master
      eNB->proc.num_slaves = MAX_NUM_CCs-1;
      eNB->proc.slave_proc = (eNB_proc_t**)malloc(eNB->proc.num_slaves*sizeof(eNB_proc_t*));

      for (i=0; i< eNB->proc.num_slaves; i++) {
        if (i < CC_id)  eNB->proc.slave_proc[i] = &(PHY_vars_eNB_g[inst][i]->proc);
        if (i >= CC_id)  eNB->proc.slave_proc[i] = &(PHY_vars_eNB_g[inst][i+1]->proc);
      }
    }
  }


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
  eNB_proc_t *proc;
  eNB_rxtx_proc_t *proc_rxtx;
  for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    eNB=PHY_vars_eNB_g[inst][CC_id];
    
    proc = &eNB->proc;
    proc_rxtx = &proc->proc_rxtx[0];
    
#ifdef DEBUG_THREADS
    printf( "Killing TX CC_id %d thread %d\n", CC_id, i );
#endif
    
    proc_rxtx[0].instance_cnt_rxtx = 0; // FIXME data race!
    proc_rxtx[1].instance_cnt_rxtx = 0; // FIXME data race!
    proc->instance_cnt_prach = 0;
    proc->instance_cnt_FH = 0;
    pthread_cond_signal( &proc_rxtx[0].cond_rxtx );    
    pthread_cond_signal( &proc_rxtx[1].cond_rxtx );
    pthread_cond_signal( &proc->cond_prach );
    pthread_cond_signal( &proc->cond_FH );
    pthread_cond_broadcast(&sync_phy_proc.cond_phy_proc_tx);

    pthread_join( proc->pthread_FH, (void**)&status ); 
    pthread_mutex_destroy( &proc->mutex_FH );
    pthread_cond_destroy( &proc->cond_FH );
            
    pthread_join( proc->pthread_prach, (void**)&status );    
    pthread_mutex_destroy( &proc->mutex_prach );
    pthread_cond_destroy( &proc->cond_prach );         

    int i;
    for (i=0;i<2;i++) {
      pthread_join( proc_rxtx[i].pthread_rxtx, (void**)&status );
      pthread_mutex_destroy( &proc_rxtx[i].mutex_rxtx );
      pthread_cond_destroy( &proc_rxtx[i].cond_rxtx );
    }
  }
}


/* this function maps the phy_vars_eNB tx and rx buffers to the available rf chains.
   Each rf chain is is addressed by the card number and the chain on the card. The
   rf_map specifies for each CC, on which rf chain the mapping should start. Multiple
   antennas are mapped to successive RF chains on the same card. */
int setup_eNB_buffers(PHY_VARS_eNB **phy_vars_eNB, openair0_config_t *openair0_cfg) {

  int i, CC_id;
  int j;

  uint16_t N_TA_offset = 0;

  LTE_DL_FRAME_PARMS *frame_parms;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    if (phy_vars_eNB[CC_id]) {
      frame_parms = &(phy_vars_eNB[CC_id]->frame_parms);
      printf("setup_eNB_buffers: frame_parms = %p\n",frame_parms);
    } else {
      printf("phy_vars_eNB[%d] not initialized\n", CC_id);
      return(-1);
    }

    if (frame_parms->frame_type == TDD) {
      if (frame_parms->N_RB_DL == 100)
        N_TA_offset = 624;
      else if (frame_parms->N_RB_DL == 50)
        N_TA_offset = 624/2;
      else if (frame_parms->N_RB_DL == 25)
        N_TA_offset = 624/4;
    }

 

    if (openair0_cfg[CC_id].mmapped_dma == 1) {
    // replace RX signal buffers with mmaped HW versions
      
      for (i=0; i<frame_parms->nb_antennas_rx; i++) {
	printf("Mapping eNB CC_id %d, rx_ant %d\n",CC_id,i);
	free(phy_vars_eNB[CC_id]->common_vars.rxdata[0][i]);
	phy_vars_eNB[CC_id]->common_vars.rxdata[0][i] = openair0_cfg[CC_id].rxbase[i];
	
	
	
	printf("rxdata[%d] @ %p\n",i,phy_vars_eNB[CC_id]->common_vars.rxdata[0][i]);
	
	for (j=0; j<16; j++) {
	  printf("rxbuffer %d: %x\n",j,phy_vars_eNB[CC_id]->common_vars.rxdata[0][i][j]);
	  phy_vars_eNB[CC_id]->common_vars.rxdata[0][i][j] = 16-j;
	}
      }
      
      for (i=0; i<frame_parms->nb_antennas_tx; i++) {
	printf("Mapping eNB CC_id %d, tx_ant %d\n",CC_id,i);
	free(phy_vars_eNB[CC_id]->common_vars.txdata[0][i]);
	phy_vars_eNB[CC_id]->common_vars.txdata[0][i] = openair0_cfg[CC_id].txbase[i];//(int32_t*) openair0_exmimo_pci[rf_map[CC_id].card].dac_head[rf_map[CC_id].chain+i];
	
	printf("txdata[%d] @ %p\n",i,phy_vars_eNB[CC_id]->common_vars.txdata[0][i]);
	
	for (j=0; j<16; j++) {
	  printf("txbuffer %d: %x\n",j,phy_vars_eNB[CC_id]->common_vars.txdata[0][i][j]);
	  phy_vars_eNB[CC_id]->common_vars.txdata[0][i][j] = 16-j;
	}
      }
    }
    else {  // not memory-mapped DMA 
    

      rxdata = (int32_t**)malloc16(frame_parms->nb_antennas_rx*sizeof(int32_t*));
      txdata = (int32_t**)malloc16(frame_parms->nb_antennas_tx*sizeof(int32_t*));
      
      for (i=0; i<frame_parms->nb_antennas_rx; i++) {
	free(phy_vars_eNB[CC_id]->common_vars.rxdata[0][i]);
	rxdata[i] = (int32_t*)(32 + malloc16(32+frame_parms->samples_per_tti*10*sizeof(int32_t))); // FIXME broken memory allocation
	phy_vars_eNB[CC_id]->common_vars.rxdata[0][i] = rxdata[i]-N_TA_offset; // N_TA offset for TDD         FIXME! N_TA_offset > 16 => access of unallocated memory
	memset(rxdata[i], 0, frame_parms->samples_per_tti*10*sizeof(int32_t));
	printf("rxdata[%d] @ %p (%p) (N_TA_OFFSET %d)\n", i, phy_vars_eNB[CC_id]->common_vars.rxdata[0][i],rxdata[i],N_TA_offset);      
      }
      
      for (i=0; i<frame_parms->nb_antennas_tx; i++) {
	free(phy_vars_eNB[CC_id]->common_vars.txdata[0][i]);
	txdata[i] = (int32_t*)(32 + malloc16(32 + frame_parms->samples_per_tti*10*sizeof(int32_t))); // FIXME broken memory allocation
	phy_vars_eNB[CC_id]->common_vars.txdata[0][i] = txdata[i];
	memset(txdata[i],0, frame_parms->samples_per_tti*10*sizeof(int32_t));
	printf("txdata[%d] @ %p\n", i, phy_vars_eNB[CC_id]->common_vars.txdata[0][i]);
      }
    }
  }

  return(0);
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
 
int start_if(PHY_VARS_eNB *eNB) {
  return(eNB->ifdevice.trx_start_func(&eNB->ifdevice));
}

int start_rf(PHY_VARS_eNB *eNB) {
  return(eNB->rfdevice.trx_start_func(&eNB->rfdevice));
}

extern void eNB_fep_rru_if5(PHY_VARS_eNB *eNB);
extern void eNB_fep_full(PHY_VARS_eNB *eNB);
extern void eNB_fep_full_2thread(PHY_VARS_eNB *eNB);
extern void do_prach(PHY_VARS_eNB *eNB);

void init_eNB(eNB_func_t node_function[], eNB_timing_t node_timing[],int nb_inst,eth_params_t *eth_params,int single_thread_flag) {
  
  int CC_id;
  int inst;
  PHY_VARS_eNB *eNB;
  int ret;

  for (inst=0;inst<nb_inst;inst++) {
    for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
      eNB = PHY_vars_eNB_g[inst][CC_id]; 
      eNB->node_function      = node_function[CC_id];
      eNB->node_timing        = node_timing[CC_id];
      eNB->abstraction_flag   = 0;
      eNB->single_thread_flag = single_thread_flag;
#ifndef OCP_FRAMEWORK
      LOG_I(PHY,"Initializing eNB %d CC_id %d : (%s,%s)\n",inst,CC_id,eNB_functions[node_function[CC_id]],eNB_timing[node_timing[CC_id]]);
#endif

      switch (node_function[CC_id]) {
      case NGFI_RRU_IF5:
	eNB->do_prach             = NULL;
	eNB->fep                  = eNB_fep_rru_if5;
	eNB->td                   = NULL;
	eNB->te                   = NULL;
	eNB->proc_uespec_rx       = NULL;
	eNB->proc_tx              = NULL;
	eNB->tx_fh                = NULL;
	eNB->rx_fh                = rx_rf;
	eNB->start_rf             = start_rf;
	eNB->start_if             = start_if;
	eNB->fh_asynch            = fh_if5_asynch_DL;
	ret = openair0_device_load(&eNB->rfdevice, &openair0_cfg[CC_id]);
        if (ret<0) {
          printf("Exiting, cannot initialize rf device\n");
          exit(-1);
        }
	eNB->rfdevice.host_type   = RRH_HOST;
	eNB->ifdevice.host_type   = RRH_HOST;
        ret = openair0_transport_load(&eNB->ifdevice, &openair0_cfg[CC_id], (eth_params+CC_id));
	printf("openair0_transport_init returns %d for CC_id %d\n",ret,CC_id);
        if (ret<0) {
          printf("Exiting, cannot initialize transport protocol\n");
          exit(-1);
        }
	break;
      case NGFI_RRU_IF4p5:
	eNB->do_prach             = do_prach;
	eNB->fep                  = eNB_fep_full;//(single_thread_flag==1) ? eNB_fep_full_2thread : eNB_fep_full;
	eNB->td                   = NULL;
	eNB->te                   = NULL;
	eNB->proc_uespec_rx       = NULL;
	eNB->proc_tx              = NULL;//proc_tx_rru_if4p5;
	eNB->tx_fh                = NULL;
	eNB->rx_fh                = rx_rf;
	eNB->fh_asynch            = fh_if4p5_asynch_DL;
	eNB->start_rf             = start_rf;
	eNB->start_if             = start_if;
	ret = openair0_device_load(&eNB->rfdevice, &openair0_cfg[CC_id]);
        if (ret<0) {
          printf("Exiting, cannot initialize rf device\n");
          exit(-1);
        }
	eNB->rfdevice.host_type   = RRH_HOST;
	eNB->ifdevice.host_type   = RRH_HOST;
        ret = openair0_transport_load(&eNB->ifdevice, &openair0_cfg[CC_id], (eth_params+CC_id));
	printf("openair0_transport_init returns %d for CC_id %d\n",ret,CC_id);
        if (ret<0) {
          printf("Exiting, cannot initialize transport protocol\n");
          exit(-1);
        }

	malloc_IF4p5_buffer(eNB);

	break;
      case eNodeB_3GPP:
	eNB->do_prach             = do_prach;
	eNB->fep                  = eNB_fep_full;//(single_thread_flag==1) ? eNB_fep_full_2thread : eNB_fep_full;
	eNB->td                   = ulsch_decoding_data;//(single_thread_flag==1) ? ulsch_decoding_data_2thread : ulsch_decoding_data;
	eNB->te                   = dlsch_encoding;//(single_thread_flag==1) ? dlsch_encoding_2threads : dlsch_encoding;
	eNB->proc_uespec_rx       = phy_procedures_eNB_uespec_RX;
	eNB->proc_tx              = proc_tx_full;
	eNB->tx_fh                = NULL;
	eNB->rx_fh                = rx_rf;
	eNB->start_rf             = start_rf;
	eNB->start_if             = NULL;
        eNB->fh_asynch            = NULL;
	ret = openair0_device_load(&eNB->rfdevice, &openair0_cfg[CC_id]);
        if (ret<0) {
          printf("Exiting, cannot initialize rf device\n");
          exit(-1);
        }
	eNB->rfdevice.host_type   = BBU_HOST;
	eNB->ifdevice.host_type   = BBU_HOST;
	break;
      case eNodeB_3GPP_BBU:
	eNB->do_prach       = do_prach;
	eNB->fep            = eNB_fep_full;//(single_thread_flag==1) ? eNB_fep_full_2thread : eNB_fep_full;
	eNB->td             = ulsch_decoding_data;//(single_thread_flag==1) ? ulsch_decoding_data_2thread : ulsch_decoding_data;
	eNB->te             = dlsch_encoding;//(single_thread_flag==1) ? dlsch_encoding_2threads : dlsch_encoding;
	eNB->proc_uespec_rx = phy_procedures_eNB_uespec_RX;
	eNB->proc_tx        = proc_tx_full;
        if (eNB->node_timing == synch_to_other) {
           eNB->tx_fh          = tx_fh_if5_mobipass;
           eNB->rx_fh          = rx_fh_slave;
           eNB->fh_asynch      = fh_if5_asynch_UL;

        }
        else {
           eNB->tx_fh          = tx_fh_if5;
           eNB->rx_fh          = rx_fh_if5;
           eNB->fh_asynch      = NULL;
        }

	eNB->start_rf       = NULL;
	eNB->start_if       = start_if;
	eNB->rfdevice.host_type   = BBU_HOST;

	eNB->ifdevice.host_type   = BBU_HOST;

        ret = openair0_transport_load(&eNB->ifdevice, &openair0_cfg[CC_id], (eth_params+CC_id));
        printf("openair0_transport_init returns %d for CC_id %d\n",ret,CC_id);
        if (ret<0) {
          printf("Exiting, cannot initialize transport protocol\n");
          exit(-1);
        }
	break;
      case NGFI_RCC_IF4p5:
	eNB->do_prach             = do_prach;
	eNB->fep                  = NULL;
	eNB->td                   = ulsch_decoding_data;//(single_thread_flag==1) ? ulsch_decoding_data_2thread : ulsch_decoding_data;
	eNB->te                   = dlsch_encoding;//(single_thread_flag==1) ? dlsch_encoding_2threads : dlsch_encoding;
	eNB->proc_uespec_rx       = phy_procedures_eNB_uespec_RX;
	eNB->proc_tx              = proc_tx_high;
	eNB->tx_fh                = tx_fh_if4p5;
	eNB->rx_fh                = rx_fh_if4p5;
	eNB->start_rf             = NULL;
	eNB->start_if             = start_if;
        eNB->fh_asynch            = (eNB->node_timing == synch_to_other) ? fh_if4p5_asynch_UL : NULL;
	eNB->rfdevice.host_type   = BBU_HOST;
	eNB->ifdevice.host_type   = BBU_HOST;
        ret = openair0_transport_load(&eNB->ifdevice, &openair0_cfg[CC_id], (eth_params+CC_id));
        printf("openair0_transport_init returns %d for CC_id %d\n",ret,CC_id);
        if (ret<0) {
          printf("Exiting, cannot initialize transport protocol\n");
          exit(-1);
        }
	malloc_IF4p5_buffer(eNB);

	break;
      case NGFI_RAU_IF4p5:
	eNB->do_prach       = do_prach;
	eNB->fep            = NULL;

	eNB->td             = ulsch_decoding_data;//(single_thread_flag==1) ? ulsch_decoding_data_2thread : ulsch_decoding_data;
	eNB->te             = dlsch_encoding;//(single_thread_flag==1) ? dlsch_encoding_2threads : dlsch_encoding;
	eNB->proc_uespec_rx = phy_procedures_eNB_uespec_RX;
	eNB->proc_tx        = proc_tx_high;
	eNB->tx_fh          = tx_fh_if4p5; 
	eNB->rx_fh          = rx_fh_if4p5; 
        eNB->fh_asynch      = (eNB->node_timing == synch_to_other) ? fh_if4p5_asynch_UL : NULL;
	eNB->start_rf       = NULL;
	eNB->start_if       = start_if;

	eNB->rfdevice.host_type   = BBU_HOST;
	eNB->ifdevice.host_type   = BBU_HOST;
        ret = openair0_transport_load(&eNB->ifdevice, &openair0_cfg[CC_id], (eth_params+CC_id));
        printf("openair0_transport_init returns %d for CC_id %d\n",ret,CC_id);
        if (ret<0) {
          printf("Exiting, cannot initialize transport protocol\n");
          exit(-1);
        }
	break;	
	malloc_IF4p5_buffer(eNB);

      }
    }

    if (setup_eNB_buffers(PHY_vars_eNB_g[inst],&openair0_cfg[CC_id])!=0) {
      printf("Exiting, cannot initialize eNodeB Buffers\n");
      exit(-1);
    }

    init_eNB_proc(inst);
  }

  sleep(1);
  LOG_D(HW,"[lte-softmodem.c] eNB threads created\n");
  

}


void stop_eNB(int nb_inst) {

  for (int inst=0;inst<nb_inst;inst++) {
    printf("Killing eNB %d processing threads\n",inst);
    kill_eNB_proc(inst);
  }
}
