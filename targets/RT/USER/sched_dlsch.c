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

/*! \file sched_dlsch.c
* \brief DLSCH decoding thread (RTAI)
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#include "rt_wrapper.h"

#include <sys/mman.h>

#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "SCHED/defs.h"

#include "UTIL/LOG/vcd_signal_dumper.h"

#define DEBUG_PHY

/// Mutex for instance count on dlsch scheduling
pthread_mutex_t dlsch_mutex[8];
/// Condition variable for dlsch thread
pthread_cond_t dlsch_cond[8];

pthread_t dlsch_threads[8];
pthread_attr_t attr_dlsch_threads;
unsigned char dlsch_thread_indices[8];

// activity indicators for harq_pid's
int dlsch_instance_cnt[8];
// process ids for cpu
int dlsch_cpuid[8];
// subframe number for each harq_pid (needed to store ack in right place for UL)
int dlsch_subframe[8];

extern int oai_exit;

/*
extern int dlsch_errors;
extern int dlsch_received;
extern int dlsch_errors_last;
extern int dlsch_received_last;
extern int dlsch_fer;
extern int current_dlsch_cqi;
*/

/** DLSCH Decoding Thread */
static void * dlsch_thread(void *param)
{

  //unsigned long cpuid;
  unsigned char dlsch_thread_index = *((unsigned char *)param);
  unsigned int ret=0;
  uint8_t harq_pid;


#ifdef RTAI
  RT_TASK *task;
  char task_name[8];
#endif

  int eNB_id = 0, UE_id = 0, CC_id=0;
  PHY_VARS_UE *phy_vars_ue = PHY_vars_UE_g[UE_id][CC_id];

  if ((dlsch_thread_index <0) || (dlsch_thread_index>7)) {
    LOG_E(PHY,"[SCHED][DLSCH] Illegal dlsch_thread_index %d (%p)!!!!\n",dlsch_thread_index,param);
    return 0;
  }

#ifdef RTAI
  sprintf(task_name,"DLSCH%d",dlsch_thread_index);
  task = rt_task_init_schmod(nam2num(task_name), 0, 0, 0, SCHED_FIFO, 0xF);

  if (task==NULL) {
    LOG_E(PHY,"[SCHED][DLSCH] Problem starting dlsch_thread_index %d (%s)!!!!\n",dlsch_thread_index,task_name);
    return 0;
  } else {
    LOG_I(PHY,"[SCHED][DLSCH] dlsch_thread for process %d started with id %p\n",
          dlsch_thread_index,
          task);
  }

#endif

  mlockall(MCL_CURRENT | MCL_FUTURE);

  //rt_set_runnable_on_cpuid(task,1);
  //cpuid = rtai_cpuid();

#ifdef HARD_RT
  rt_make_hard_real_time();
#endif

  //dlsch_cpuid[dlsch_thread_index] = cpuid;

  while (!oai_exit) {

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_THREAD0+dlsch_thread_index,0);

    if (pthread_mutex_lock(&dlsch_mutex[dlsch_thread_index]) != 0) {
      LOG_E(PHY,"[SCHED][DLSCH] error locking mutex.\n");
    } else {

      while (dlsch_instance_cnt[dlsch_thread_index] < 0) {
        pthread_cond_wait(&dlsch_cond[dlsch_thread_index],&dlsch_mutex[dlsch_thread_index]);
      }

      if (pthread_mutex_unlock(&dlsch_mutex[dlsch_thread_index]) != 0) {
        LOG_E(PHY,"[SCHED][DLSCH] error unlocking mutex.\n");
      }
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_THREAD0+dlsch_thread_index,1);

    if (oai_exit) break;

    LOG_I(PHY,"[SCHED][DLSCH] Frame %d: Calling dlsch_decoding with dlsch_thread_index = %d\n",phy_vars_ue->frame_rx,dlsch_thread_index);



    if (phy_vars_ue->frame_rx < phy_vars_ue->dlsch_errors[eNB_id]) {
      phy_vars_ue->dlsch_errors[eNB_id]=0;
      phy_vars_ue->dlsch_received[eNB_id] = 0;
    }

    harq_pid = dlsch_thread_index;

    if (phy_vars_ue->dlsch_ue[eNB_id][0]) {

      //      rt_printk("[SCHED][DLSCH] Frame %d, slot %d, start %llu, end %llu, proc time: %llu ns\n",phy_vars_ue->frame,last_slot,time0,time1,(time1-time0));

      dlsch_unscrambling(&phy_vars_ue->lte_frame_parms,
                         0,
                         phy_vars_ue->dlsch_ue[eNB_id][0],
                         phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->G,
                         phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->llr[0],
                         0,
                         dlsch_subframe[dlsch_thread_index]<<1);

      LOG_I(PHY,"[UE %d] PDSCH Calling dlsch_decoding for subframe %d, harq_pid %d, G%d\n", phy_vars_ue->Mod_id,dlsch_subframe[dlsch_thread_index], harq_pid,
            phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->G);

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_DECODING0+dlsch_thread_index,1);
      ret = dlsch_decoding(phy_vars_ue,
                           phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->llr[0],
                           &phy_vars_ue->lte_frame_parms,
                           phy_vars_ue->dlsch_ue[eNB_id][0],
                           phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid],
                           dlsch_subframe[dlsch_thread_index],
                           harq_pid,
                           1, // is_crnti
                           0);  // llr8_flag
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_DECODING0+dlsch_thread_index,0);

      LOG_D(PHY,"[UE  %d][PDSCH %x/%d] Frame %d subframe %d: PDSCH/DLSCH decoding iter %d (mcs %d, rv %d, TBS %d)\n",
            phy_vars_ue->Mod_id,
            phy_vars_ue->dlsch_ue[eNB_id][0]->rnti,harq_pid,
            phy_vars_ue->frame_rx,dlsch_subframe[dlsch_thread_index],ret,
            phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->mcs,
            phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->rvidx,
            phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->TBS);


      if (ret == (1+MAX_TURBO_ITERATIONS)) {
        phy_vars_ue->dlsch_errors[eNB_id]++;

#ifdef DEBUG_PHY
        LOG_I(PHY,"[UE  %d][PDSCH %x/%d] Frame %d subframe %d DLSCH in error (rv %d,mcs %d)\n",
              phy_vars_ue->Mod_id,phy_vars_ue->dlsch_ue[eNB_id][0]->rnti,
              harq_pid,phy_vars_ue->frame_rx,dlsch_subframe[dlsch_thread_index],
              phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->rvidx,
              phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->mcs);
#endif
      } else {
        LOG_I(PHY,"[UE  %d][PDSCH %x/%d] Frame %d subframe %d: Received DLSCH (rv %d,mcs %d)\n",
              phy_vars_ue->Mod_id,phy_vars_ue->dlsch_ue[eNB_id][0]->rnti,
              harq_pid,phy_vars_ue->frame_rx,dlsch_subframe[dlsch_thread_index],
              phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->rvidx,
              phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->mcs);

#ifdef OPENAIR2
        mac_xface->ue_send_sdu(phy_vars_ue->Mod_id,
                               0, // CC_id
                               phy_vars_ue->frame_rx,
                               phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->b,
                               phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->TBS>>3,
                               eNB_id);
#endif
        phy_vars_ue->total_TBS[eNB_id] =  phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->TBS + phy_vars_ue->total_TBS[eNB_id];
        phy_vars_ue->total_received_bits[eNB_id] = phy_vars_ue->total_received_bits[eNB_id] + phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->TBS;
      }
    }

    // this is done in main thread
    /*
    if (phy_vars_ue->frame % 100 == 0) {
      if ((phy_vars_ue->dlsch_received[eNB_id] - phy_vars_ue->dlsch_received_last[eNB_id]) != 0)
    phy_vars_ue->dlsch_fer[eNB_id] = (100*(phy_vars_ue->dlsch_errors[eNB_id] - phy_vars_ue->dlsch_errors_last[eNB_id]))/(phy_vars_ue->dlsch_received[eNB_id] - phy_vars_ue->dlsch_received_last[eNB_id]);
      phy_vars_ue->dlsch_errors_last[eNB_id] = phy_vars_ue->dlsch_errors[eNB_id];
      phy_vars_ue->dlsch_received_last[eNB_id] = phy_vars_ue->dlsch_received[eNB_id];
    }
    */


#ifdef DEBUG_PHY

    if (phy_vars_ue->dlsch_ue[eNB_id][0]) {
      LOG_I(PHY,"[UE  %d][PDSCH %x/%d] Frame %d subframe %d: PDSCH/DLSCH decoding iter %d (mcs %d, rv %d, TBS %d)\n",
            phy_vars_ue->Mod_id,
            phy_vars_ue->dlsch_ue[eNB_id][0]->rnti,harq_pid,
            phy_vars_ue->frame_rx,dlsch_subframe[dlsch_thread_index],ret,
            phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->mcs,
            phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->rvidx,
            phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->TBS);

      if (phy_vars_ue->frame_rx%100==0) {
        LOG_D(PHY,"[UE  %d][PDSCH %x] Frame %d subframe %d dlsch_errors %d, dlsch_received %d, dlsch_fer %d, current_dlsch_cqi %d\n",
              phy_vars_ue->Mod_id,phy_vars_ue->dlsch_ue[eNB_id][0]->rnti,
              phy_vars_ue->frame_rx,dlsch_subframe[dlsch_thread_index],
              phy_vars_ue->dlsch_errors[eNB_id],
              phy_vars_ue->dlsch_received[eNB_id],
              phy_vars_ue->dlsch_fer[eNB_id],
              phy_vars_ue->PHY_measurements.wideband_cqi_tot[eNB_id]);
      }
    } else {
      LOG_I( PHY,"[UE %d][PDSCH ?/%d] Frame %d subframe %d: PDSCH/DLSCH decoding iter %d (phy_vars_ue->dlsch_ue[eNB_id][0] == 0)\n",
             phy_vars_ue->Mod_id,
             harq_pid,
             phy_vars_ue->frame_rx, dlsch_subframe[dlsch_thread_index], ret );
    }

#endif

    if (pthread_mutex_lock(&dlsch_mutex[dlsch_thread_index]) != 0) {
      msg("[openair][SCHED][DLSCH] error locking mutex.\n");
    } else {
      dlsch_instance_cnt[dlsch_thread_index]--;

      if (pthread_mutex_unlock(&dlsch_mutex[dlsch_thread_index]) != 0) {
        msg("[openair][SCHED][DLSCH] error unlocking mutex.\n");
      }
    }

  }

#ifdef HARD_RT
  rt_make_soft_real_time();
#endif

  msg("[openair][SCHED][DLSCH] DLSCH thread %d exiting\n",dlsch_thread_index);

  return 0;
}

int init_dlsch_threads(void)
{

  int error_code;
  struct sched_param p;
  unsigned char dlsch_thread_index;

  pthread_attr_init (&attr_dlsch_threads);
  pthread_attr_setstacksize(&attr_dlsch_threads,OPENAIR_THREAD_STACK_SIZE);
  //attr_dlsch_threads.priority = 1;

  p.sched_priority = OPENAIR_THREAD_PRIORITY;
  pthread_attr_setschedparam  (&attr_dlsch_threads, &p);
#ifndef RTAI_ISNT_POSIX
  pthread_attr_setschedpolicy (&attr_dlsch_threads, SCHED_FIFO);
#endif

  for(dlsch_thread_index=0; dlsch_thread_index<8; dlsch_thread_index++) {

    pthread_mutex_init(&dlsch_mutex[dlsch_thread_index],NULL);
    pthread_cond_init(&dlsch_cond[dlsch_thread_index],NULL);

    dlsch_instance_cnt[dlsch_thread_index] = -1;
    dlsch_thread_indices[dlsch_thread_index] = dlsch_thread_index;
    rt_printk("[openair][SCHED][DLSCH][INIT] Allocating DLSCH thread for dlsch_thread_index %d (%p)\n",dlsch_thread_index,&dlsch_thread_indices[dlsch_thread_index]);
    error_code = pthread_create(&dlsch_threads[dlsch_thread_index],
                                &attr_dlsch_threads,
                                dlsch_thread,
                                (void *)&dlsch_thread_indices[dlsch_thread_index]);

    if (error_code!= 0) {
      rt_printk("[openair][SCHED][DLSCH][INIT] Could not allocate dlsch_thread %d, error %d\n",dlsch_thread_index,error_code);
      return(error_code);
    } else {
      rt_printk("[openair][SCHED][DLSCH][INIT] Allocate dlsch_thread %d successful\n",dlsch_thread_index);
    }
  }

  return(0);
}

void cleanup_dlsch_threads(void)
{

  unsigned char dlsch_thread_index;

  for(dlsch_thread_index=0; dlsch_thread_index<8; dlsch_thread_index++) {
    //  pthread_exit(&dlsch_threads[dlsch_thread_index]);
    rt_printk("[openair][SCHED][DLSCH] Scheduling dlsch_thread %d to exit\n",dlsch_thread_index);

    dlsch_instance_cnt[dlsch_thread_index] = 0;

    if (pthread_cond_signal(&dlsch_cond[dlsch_thread_index]) != 0)
      rt_printk("[openair][SCHED][DLSCH] ERROR pthread_cond_signal\n");
    else
      rt_printk("[openair][SCHED][DLSCH] Signalled dlsch_thread %d to exit\n",dlsch_thread_index);

    rt_printk("[openair][SCHED][DLSCH] Exiting ...\n");
    pthread_cond_destroy(&dlsch_cond[dlsch_thread_index]);
    pthread_mutex_destroy(&dlsch_mutex[dlsch_thread_index]);
  }
}
