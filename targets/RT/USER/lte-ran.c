/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

   Contact Information
   OpenAirInterface Admin: openair_admin@eurecom.fr
   OpenAirInterface Tech : openair_tech@eurecom.fr
   OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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

void exit_fun(const char* s);

void init_eNB(eNB_func_t node_function[], eNB_timing_t node_timing[],int nb_inst,eth_params_t *,int);
void stop_eNB(int nb_inst);
void init_RU(RAN_CONTEXT *rc, eNB_func_t node_function, RU_if_in_t ru_if_in[], RU_if_timing_t ru_if_timing[], eth_params_t *eth_params);
void stop_RU();

// Generic thread initialisation function
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
    exit_fun("Error setting deadline scheduler");
  }

  LOG_I( HW, "[SCHED] eNB %s deadline thread started on CPU %d\n", thread_name,sched_getcpu() );

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

// RU OFDM Modulator, used in IF4p5 RRU, RCC/RAU with IF5, eNodeB
 
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



void proc_tx_high0(RU_t *ru,
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

/*
void proc_tx_high(RU_t *ru,
		  eNB_rxtx_proc_t *proc,
		  relaying_type_t r_type,
		  PHY_VARS_RN *rn) {


  // do PHY high
  proc_tx_high0(eNB,proc,r_type,rn);

  // if TX fronthaul go ahead 
  if (eNB->tx_fh) eNB->tx_fh(eNB,proc);

}

void proc_tx_full(RU_t *ru,
		  eNB_rxtx_proc_t *proc,
		  relaying_type_t r_type,
		  PHY_VARS_RN *rn) {


  // do PHY high
  proc_tx_high0(eNB,proc,r_type,rn);




}
*/

// RU IF5 TX fronthaul for 16-bit OAI format
static inline void tx_rcc_if5(PHY_vars_eNB_t *ru,ru_proc_t *proc) {
  if (ru == RC.ru[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, eNB->timestamp_tx&0xffffffff );
  send_IF5(ru, proc->timestamp_txp proc->subframe_tx, &seqno, IF5_RRH_GW_DL);
}

// RCC IF5 TX fronthaul for Mobipass packet format
static inline void tx_rcc_if5_mobipass(PHY_VARS_eNB_t *eNB,ru_proc_t *proc) {
  if (eNB == RC.eNB[0][p]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, eNB->timestamp_tx&0xffffffff );
  send_IF5(ru, proc->timestamp_tx, proc->subframe_tx, &seqno, IF5_MOBIPASS); 
}

// RCC IF4p5 TX fronthaul
static inline void tx_rcc_if4p5(PHY_VARS_eNB_t *eNB,eNB_rxtx_proc_t *proc) {    
  send_IF4p5(eNB,proc->frame_tx, proc->subframe_tx, IF4p5_PDLFFT, 0);
}

// RAU IF5 TX fronthaul for 16-bit OAI format
static inline void tx_ru_if5(RU_t *ru) {
  if (ru == RC.ru_list[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );
  send_IF5(eNB, ru->proc.timestamp_txp ru->proc.subframe_tx, &seqno, IF5_RRH_GW_DL);
}

// RAU IF5 TX fronthaul for Mobipass packet format
static inline void tx_ru_if5_mobipass(RU_t *ru) {
  if (ru == RC.ru_list[0]) VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, ru->proc.timestamp_tx&0xffffffff );
  send_IF5(eNB, ru->proc.timestamp_tx, ru->proc.subframe_tx, &seqno, IF5_MOBIPASS); 
}

// RAU IF4p5 TX fronthaul
static inline void tx_fh_if4p5(RU_t *ru) {    
  send_IF4p5(eNB,proc->frame_tx, proc->subframe_tx, IF4p5_PDLFFT, 0);
}


// RRU/RAU IF4p5 TX fronthaul receiver. Assumes an if_device on input and if or rf device on output 
// receives one subframe's worth of IF4p5 OFDM symbols and precodes (if required for RAU function) modulates via IDFT + prefix insertion (if required for RRU function)
void proc_tx_ru_if4p5(RU_t *ru) {

  uint32_t symbol_number=0;
  uint32_t symbol_mask, symbol_mask_full;
  uint16_t packet_type;
  ru_proc_t = &ru->proc;

  // dump VCD output for first RU in list
  if (ru == RC.ru_list[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_ENB, proc->frame_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_ENB, proc->subframe_tx );
  }
  /// **** incoming IF4p5 from remote RCC/RAU **** ///             
  symbol_number = 0;
  symbol_mask = 0;
  symbol_mask_full = (1<<ru->frame_parms.symbols_per_tti)-1;
  
  for (PHY_VARS_eNB *eNB_tx = ru->eNB_list[0],i=0; eNB_tx[i] != NULL ; i++) {
    do { 
      recv_IF4p5(ru, &proc->frame_tx, &proc->subframe_tx, &packet_type, &symbol_number);
      symbol_mask = symbol_mask | (1<<symbol_number);
    } while (symbol_mask != symbol_mask_full); 
    if (ru->do_precoding) ru->do_precoding(i,ru);	
  }
  // do OFDM modulation if needed
  if (ru->do_OFDM_mod) ru->do_OFDM_mod(ru);

  // do outgoing TX fronthaul if needed 
  if (ru->tx_fh) ru->tx_fh(ru);
}

void proc_tx_ru_if5(RU_t *ru) {
  

  if (ru == RC.ru_list[0]) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_ENB, ru->proc.frame_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_ENB, ru->proc.subframe_tx );
  }

  /// **** recv_IF5 of txdata from BBU **** ///       
  recv_IF5(eNB, &ru->timestamp_tx, proc->subframe_tx, IF5_RRH_GW_DL);

  // do OFDM modulation if needed
  if (ru->do_OFDM_mod) ru->do_OFDM_mod(ru);

  // do outgoing TX fronthaul if needed 
  if (ru->tx_fh) ru->tx_fh(ru);
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

static inline int rxtx(PHY_VARS_eNB eNB_t *eNB,eNB_rxtx_proc_t *proc, char *thread_name) {

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
void fh_if5_asynch_UL(RU_t *ru,int *frame,int *subframe) {

  eNB_proc_t *proc       = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;

  recv_IF5(eNB, &ru->timestamp_rx, *subframe, IF5_RRH_GW_UL); 

  proc->subframe_rx = (ru->timestamp_rx/fp->samples_per_tti)%10;
  proc->frame_rx    = (ru->timestamp_rx/(10*fp->samples_per_tti))&1023;

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
void fh_if4p5_asynch_UL(RU_t *ru,int *frame,int *subframe) {

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


void fh_if5_asynch_DL(RU_t *ru,int *frame,int *subframe) {

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

void fh_if4p5_asynch_DL(RU_t *ru,int *frame,int *subframe) {

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





void rx_rf(RU_t *ru,int *frame,int *subframe) {

  eNB_proc_t *proc = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  void *rxp[fp->nb_antennas_rx],*txp[fp->nb_antennas_tx]; 
  unsigned int rxs,txs;
  int i;
  int tx_sfoffset = 3;//(eNB->single_thread_flag == 1) ? 3 : 3;
  if (proc->first_rx==0) {
    
    // Transmit TX buffer based on timestamp from RX
    //    printf("trx_write -> USRP TS %llu (sf %d)\n", (ru->timestamp_rx+(3*fp->samples_per_tti)),(proc->subframe_rx+2)%10);
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TST, (ru->timestamp_rx+(tx_sfoffset*fp->samples_per_tti)-openair0_cfg[0].tx_sample_advance)&0xffffffff );
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
    // prepare tx buffer pointers
	
    for (i=0; i<fp->nb_antennas_tx; i++)
      txp[i] = (void*)&eNB->common_vars.txdata[0][i][((proc->subframe_rx+tx_sfoffset)%10)*fp->samples_per_tti];
    
    txs = eNB->rfdevice.trx_write_func(&eNB->rfdevice,
				       ru->timestamp_rx+(tx_sfoffset*fp->samples_per_tti)-openair0_cfg[0].tx_sample_advance,
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
				    &(ru->timestamp_rx),
				    rxp,
				    fp->samples_per_tti,
				    fp->nb_antennas_rx);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );
 
  if (proc->first_rx == 1)
    eNB->ts_offset = ru->timestamp_rx;
 
  proc->frame_rx    = ((ru->timestamp_rx-eNB->ts_offset) / (fp->samples_per_tti*10))&1023;
  proc->subframe_rx = ((ru->timestamp_rx-eNB->ts_offset) / fp->samples_per_tti)%10;
  // synchronize first reception to frame 0 subframe 0

  ru->timestamp_tx = ru->timestamp_rx+(4*fp->samples_per_tti);
  //printf("trx_read <- USRP TS %llu (sf %d, f %d, first_rx %d)\n", ru->timestamp_rx,proc->subframe_rx,proc->frame_rx,proc->first_rx);  
  
  if (proc->first_rx == 0) {
    if (proc->subframe_rx != *subframe){
      LOG_E(PHY,"Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d)\n",ru->timestamp_rx,proc->subframe_rx,*subframe);
      exit_fun("Exiting");
    }
    
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->frame_rx %d frame %d)\n",ru->timestamp_rx,proc->frame_rx,*frame);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->subframe_rx;        
  }
  
  //printf("timestamp_rx %lu, frame %d(%d), subframe %d(%d)\n",ru->timestamp_rx,proc->frame_rx,frame,proc->subframe_rx,subframe);
  
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, ru->timestamp_rx&0xffffffff );
  
  if (rxs != fp->samples_per_tti)
    exit_fun( "problem receiving samples" );
  

  
}

void rx_fh_if5(RU_t *ru,int *frame, int *subframe) {

  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  eNB_proc_t *proc = &eNB->proc;

  recv_IF5(eNB, &ru->timestamp_rx, *subframe, IF5_RRH_GW_UL); 

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


void rx_fh_if4p5(RU_t *ru,int *frame,int *subframe) {

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
      LOG_E(PHY,"Received Timestamp (IF4p5) doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d,CCid %d)\n",proc->subframe_rx,*subframe,eNB->CC_id);
      exit_fun("Exiting");
    }
    if (proc->frame_rx != *frame) {
      LOG_E(PHY,"Received Timestamp (IF4p5) doesn't correspond to the time we think it is (proc->frame_rx %d frame %d,CCid %d)\n",proc->frame_rx,*frame,eNB->CC_id);
      exit_fun("Exiting");
    }
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->subframe_rx;        
  }
  
  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_TRX_TS, proc->timestamp_rx&0xffffffff );
  
}

void rx_fh_slave(RU_t *ru,int *frame,int *subframe) {
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

void wakeup_slaves(ru_proc_t *proc) {

  int i;
  struct timespec wait;
  
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;
  
  for (i=0;i<proc->num_slaves;i++) {
    ru_proc_t *slave_proc = proc->slave_proc[i];
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
      LOG_W( PHY,"[RU] Frame %d, slave CC_id %d thread busy!! (cnt_FH %i)\n",slave_proc->frame_rx,slave_proc->CC_id, cnt_slave);
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

static void* ru_thread( void* param ) {

  static int ru_thread_status;

  RU_t *ru=(RU_t*)param;
  ru_proc_t *proc=ru->proc;
  int subframe=0, frame=0; 

  // set default return value
  ru_thread_status = 0;

  thread_top_init("ru_thread",0,870000,1000000,1000000);

  wait_sync("ru_thread");

  /*
#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)
  if (eNB->node_function < NGFI_RRU_IF5)
    wait_system_ready ("Waiting for eNB application to be ready %s\r", &start_eNB);
#endif 
  */

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

    LOG_D(PHY,"RU thread %p (proc %p), frame %d (%p), subframe %d (%p)\n",
	  pthread_self(), proc, frame,&frame,subframe,&subframe);
 
    // synchronization on FH interface, acquire signals/data and block
    if (ru->rx_fh) ru->rx_fh(ru,&frame,&subframe);
    else AssertFatal(1==0, "No fronthaul interface : eNB->node_function %d",eNB->node_function);

    T(T_ENB_MASTER_TICK, T_INT(0), T_INT(proc->frame_rx), T_INT(proc->subframe_rx));
    /*
      // wakeup correct eNB processes
      proc_rxtx->subframe_rx = proc->subframe_rx;
      proc_rxtx->frame_rx    = proc->frame_rx;
      proc_rxtx->subframe_tx = (proc->subframe_rx+4)%10;
      proc_rxtx->frame_tx    = (proc->subframe_rx < 6) ? proc->frame_rx : (proc->frame_rx+1)&1023; 
      proc_rxtx->timestamp_tx = proc->timestamp_tx;
    */
    // At this point, all information for subframe has been received on FH interface
    // If this proc is to provide synchronization, do so
    wakeup_slaves(proc);

    // wait until eNBs are finished subframe RX n and TX n+4
 
    //    if (rxtx(eNB,proc_rxtx,"eNB_thread_single") < 0) break;
  }
  

  printf( "Exiting ru_thread \n");
 
  ru_thread_status = 0;
  return &ru_thread_status;

}

extern void init_fep_thread(PHY_VARS_eNB *, pthread_attr_t *);_
extern void init_td_thread(PHY_VARS_eNB *, pthread_attr_t *);
extern void init_te_thread(PHY_VARS_eNB *, pthread_attr_t *);

void init_eNB_proc(int inst) {
  
  int i;
  int CC_id;
  PHY_VARS_eNB *eNB;
  eNB_proc_t *proc;
  eNB_rxtx_proc_t *proc_rxtx;
  pthread_attr_t *attr0=NULL,*attr1=NULL,*attr_prach=NULL,*attr_asynch=NULL,*attr_single=NULL,*attr_fep=NULL,*attr_td=NULL,*attr_te;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    eNB = PHY_vars_eNB_g[inst][CC_id];
    LOG_I(PHY,"Initializing eNB %d CC_id %d (%s,%s),\n",inst,CC_id,eNB_functions[eNB->node_function],eNB_timing[eNB->node_timing]);
    proc = &eNB->proc;

    proc_rxtx = proc->proc_rxtx;
    proc_rxtx[0].instance_cnt_rxtx = -1;
    proc_rxtx[1].instance_cnt_rxtx = -1;
    proc->instance_cnt_prach       = -1;
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
    pthread_cond_init( &proc->cond_asynch_rxtx, NULL);

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
    }
    else {
      snprintf( name, sizeof(name), " %d", i );
      pthread_setname_np( proc->pthread_single, name );
    }
  }

  //for multiple CCs: setup master and slaves
  /*
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
    }*/


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
    pthread_cond_signal( &proc_rxtx[0].cond_rxtx );    
    pthread_cond_signal( &proc_rxtx[1].cond_rxtx );
    pthread_cond_signal( &proc->cond_prach );
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

void init_RU(RAN_CONTEXT *rc, eNB_func_t node_function, RU_if_in_t ru_if_in[], RU_if_timing_t ru_if_timing[], eth_params_t *eth_params) {
  
  int ru_id;

  for (ru_id=0;ru_id<rc->nb_RU;ru_id++) {
    ru = &rc.ru_desc[ru_id];
    ru->RU_if_in[ru_id]     = ru_if_in[ru_id];
    ru->RU_if_timing        = ru_if_timing[ru_id];
    LOG_I(PHY,"Initializing RRU descriptor %d : (%s,%s)\n",ru_id,ru_if_types[ru_if_in[ru_id]],eNB_timing[ru_timing[ru_id]]);
    
    switch (ru->RU_if_in[ru_id]) {
    case LOCAL_RF:   // this is an RRU or eNB with integrated RF
      if (node_function ==  NGFI_RRU_IF5) {
	ru->do_prach              = NULL;                   // no prach processing
	ru->fep_rx                = eNB_fep_rru_if5;        // need only to do send_IF5
	ru->fep_tx                = NULL;                   // nothing (this is a time-domain signal)
	ru->fh_asynch             = fh_if5_asynch_DL;       // TX packets come asynchronously 
	ru->start_if              = start_if;               // need to start the if interface for if5
	ru->ifdevice.host_type    = RRH_HOST;
	ru->rfdevice.host_type    = RRH_HOST;
      }
      else if (node_function == NGFI_RRU_IF4p5) {
	ru->do_prach              = do_prach;               // IF4p5 needs to do part of prach processing in RRU
	ru->fep_rx                = ru_fep_full;           // this is DFTs + send_IF4p5
	ru->fep_tx                = ru_fep_idft;            // this is fep with idft only (no precoding in RRU)
	ru->fh_asynch             = fh_if4p5_asynch_DL;     // TX packets come asynchronously
	ru->start_if              = start_if;               // need to start the if interface for if4p5
	ru->ifdevice.host_type    = RRH_HOST;
	ru->rfdevice.host_type    = RRH_HOST;
      }
      else if (node_function == eNodeB_3GPP) {              
	ru->do_prach             = NULL;                    // prach is done completely in eNB processing
	ru->fep_rx               = eNB_fep_full;            // this is DFTs only
	ru->fep_tx               = pc_fep_idft_prec;        // this is fep with idft and precoding
	ru->fh_asynch            = NULL;                    // no incoming fronthaul
	ru->start_if             = NULL;                    // no if interface
	ru->rfdevice.host_type   = BBU_HOST;
      }
      ru->rx_fh                 = rx_rf;                               // local synchronous RF RX
      ru->tx_fh                 = NULL;                                // nothing connected directly to radio
      ru->start_rf              = start_rf;                            // need to start the local RF interface

      ret = openair0_device_load(&ru->rfdevice, &openair0_cfg[ru_id]);
      if (setup_RU_buffers(rc,ru_id,&openair0_cfg[ru_id])!=0) {
	printf("Exiting, cannot initialize eNodeB Buffers\n");
	exit(-1);
      }
      break;

    case REMOTE_IF5: // the remote unit is IF5 RRU
      ru->do_prach              = NULL;                // no prach processing in RU
      ru->fep_rx                = eNB_fep_full;        // this is DFTs
      ru->fep_tx                = pc_fep_tx_rru_if5;   // need to do transmit Precoding + FEP + IF5 fronthaul
      if (ru->RU_if_timing == synch_to_other) {
	ru->rx_fh               = rx_fh_slave;         // synchronize to master
	ru->tx_fh               = tx_fh_if5_mobipass;  // use send_IF5 for mobipass
	ru->fh_asynch           = fh_if5_asynch_UL;    // UL is asynchronous
      }
      else {
	ru->tx_fh               = tx_fh_if5;           // synchronous IF5 transmission
	ru->rx_fh               = rx_fh_if5;           // synchronous IF5 reception
	ru->fh_asynch           = NULL;                // no asynchronous UL
      }
      ru->start_rf             = NULL;                // no local RF
      ru->start_if             = start_if;            // need to start if interface for IF5 
      ru->fh_asynch            = fh_if5_asynch_DL;
      ru->ifdevice.host_type   = BBU_HOST;

      ret = openair0_transport_load(&ru->ifdevice, &openair0_cfg[ru_id], (eth_params+ru_id));
      printf("openair0_transport_init returns %d for ru_id %d\n",ret,ru_id);
      if (ret<0) {
	printf("Exiting, cannot initialize transport protocol\n");
	exit(-1);
      }
      break;

    case REMOTE_IF4p5:
      ru->do_prach              = NULL;                // no prach processing in RU
      ru->fep_rx                = eNB_fep_full;        // this is DFTs
      ru->fep_tx                = proc_tx_high;        // need to do transmit Precoding + IF4p5 fronthaul (no IDFTs)
      ru->tx_fh                 = tx_fh_if4p5;         // synchronous IF5 transmission
      ru->rx_fh                 = rx_fh_if4p5;         // synchronous IF5 reception
      ru->fh_asynch             = (ru->RU_if_timing == synch_to_other) ? fh_if4p5_asynch_UL : NULL;                // asynchronous UL if synch_to_other
      
      ru->start_rf              = NULL;                // no local RF
      ru->start_if              = start_if;            // need to start if interface for IF4p5 
      ru->fh_asynch             = fh_if5_asynch_DL;
      ru->ifdevice.host_type    = BBU_HOST;

      ret = openair0_transport_load(&ru->ifdevice, &openair0_cfg[ru_id], (eth_params+ru_id));
      printf("openair0_transport_init returns %d for ru_id %d\n",ret,ru_id);
      if (ret<0) {
	printf("Exiting, cannot initialize transport protocol\n");
	exit(-1);
      }
      
      malloc_IF4p5_buffer(eNB);
      
      break;

    case REMOTE_IF1pp:
      LOG_E(PHY,"RU with IF1pp not supported yet\n");
      break;

    } // switch on interface type 

  } // for ru_id

  sleep(1);
  LOG_D(HW,"[lte-softmodem.c] eNB threads created\n");
  

}

void init_RAN(RAN_CONTEXT *rc,eNB_func_t node_function[], eNB_timing_t node_timing[],eth_params_t *eth_params,int single_thread_flag) {
  
  int CC_id;
  int inst;
  PHY_VARS_eNB *eNB;
  int ret;

  for (inst=0;inst<rc->nb_inst;inst++) {
    for (CC_id=0;CC_id<rc->nb_CC;CC_id++) {
      eNB = rc->eNB[inst][CC_id]; 
      if (eNB) {
	eNB->node_function      = node_function[CC_id];
	eNB->node_timing        = node_timing[CC_id];
	eNB->abstraction_flag   = 0;
	eNB->single_thread_flag = single_thread_flag;
	eNB->ts_offset          = 0;
	LOG_I(PHY,"Initializing eNB %d CC_id %d : (%s,%s)\n",inst,CC_id,eNB_functions[node_function[CC_id]],eNB_timing[node_timing[CC_id]]);

	switch (node_function[CC_id]) {
	case NGFI_RRU_IF5:
	  eNB->td                   = NULL;
	  eNB->te                   = NULL;
	  eNB->proc_uespec_rx       = NULL;
	  eNB->proc_tx              = NULL;
	  break;
	case NGFI_RRU_IF4p5:
	  eNB->td                   = NULL;
	  eNB->te                   = NULL;
	  eNB->proc_uespec_rx       = NULL;
	  eNB->proc_tx              = NULL;//proc_tx_rru_if4p5;
	  break;
	case eNodeB_3GPP:
	  eNB->do_prach             = do_prach;
	  eNB->td                   = ulsch_decoding_data;//(single_thread_flag==1) ? ulsch_decoding_data_2thread : ulsch_decoding_data;
	  eNB->te                   = dlsch_encoding;//(single_thread_flag==1) ? dlsch_encoding_2threads : dlsch_encoding;
	  eNB->proc_uespec_rx       = phy_procedures_eNB_uespec_RX;
	  eNB->proc_tx              = proc_tx_full;
	  break;
	case eNodeB_3GPP_BBU:
	  eNB->do_prach       = do_prach;
	  eNB->td             = ulsch_decoding_data;//(single_thread_flag==1) ? ulsch_decoding_data_2thread : ulsch_decoding_data;
	  eNB->te             = dlsch_encoding;//(single_thread_flag==1) ? dlsch_encoding_2threads : dlsch_encoding;
	  eNB->proc_uespec_rx = phy_procedures_eNB_uespec_RX;
	  eNB->proc_tx        = proc_tx_full;
	  break;
	case NGFI_RCC_IF4p5:
	  eNB->do_prach             = do_prach;
	  eNB->td                   = ulsch_decoding_data;//(single_thread_flag==1) ? ulsch_decoding_data_2thread : ulsch_decoding_data;
	  eNB->te                   = dlsch_encoding;//(single_thread_flag==1) ? dlsch_encoding_2threads : dlsch_encoding;
	  eNB->proc_uespec_rx       = phy_procedures_eNB_uespec_RX;
	  eNB->proc_tx              = proc_tx_high;
	  break;
	case NGFI_RAU_IF4p5:
	  eNB->do_prach       = do_prach;
	  eNB->td             = ulsch_decoding_data;//(single_thread_flag==1) ? ulsch_decoding_data_2thread : ulsch_decoding_data;
	  eNB->te             = dlsch_encoding;//(single_thread_flag==1) ? dlsch_encoding_2threads : dlsch_encoding;
	  eNB->proc_uespec_rx = phy_procedures_eNB_uespec_RX;
	  eNB->proc_tx        = proc_tx_high;
	  break;	
	}
	// initialize eNB procedure threads if needed
	init_eNB_proc(rc,inst);
      }
    }
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
