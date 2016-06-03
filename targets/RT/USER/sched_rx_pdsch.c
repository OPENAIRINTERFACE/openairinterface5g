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
#include "SCHED/extern.h"

#include "UTIL/LOG/vcd_signal_dumper.h"

RTIME time0,time1;

#define DEBUG_PHY

/// Mutex for instance count on rx_pdsch scheduling
pthread_mutex_t rx_pdsch_mutex;
/// Condition variable for rx_pdsch thread
pthread_cond_t rx_pdsch_cond;

pthread_t rx_pdsch_thread_var;
pthread_attr_t attr_rx_pdsch_thread;

// activity indicators for harq_pid's
int rx_pdsch_instance_cnt;
// process ids for cpu
int rx_pdsch_cpuid;
// subframe number for each harq_pid (needed to store ack in right place for UL)
int rx_pdsch_slot;

extern int oai_exit;
extern pthread_mutex_t dlsch_mutex[8];
extern int dlsch_instance_cnt[8];
extern int dlsch_subframe[8];
extern pthread_cond_t dlsch_cond[8];

/** RX_PDSCH Decoding Thread */
static void * rx_pdsch_thread(void *param)
{

  //unsigned long cpuid;
  uint8_t dlsch_thread_index = 0;
  uint8_t pilot2,harq_pid,subframe;
  //  uint8_t last_slot;

  uint8_t dual_stream_UE = 0;
  uint8_t i_mod = 0;


#ifdef RTAI
  RT_TASK *task;
#endif

  int m,eNB_id = 0;
  int eNB_id_i = 1;
  PHY_VARS_UE *UE = PHY_vars_UE_g[0][0];

#ifdef RTAI
  task = rt_task_init_schmod(nam2num("RX_PDSCH_THREAD"), 0, 0, 0, SCHED_FIFO, 0xF);

  if (task==NULL) {
    LOG_E(PHY,"[SCHED][RX_PDSCH] Problem starting rx_pdsch thread!!!!\n");
    return 0;
  } else {
    LOG_I(PHY,"[SCHED][RX_PDSCH] rx_pdsch_thread started for with id %p\n",task);
  }

#endif

  mlockall(MCL_CURRENT | MCL_FUTURE);

  //rt_set_runnable_on_cpuid(task,1);
  //cpuid = rtai_cpuid();

#ifdef HARD_RT
  rt_make_hard_real_time();
#endif

  if (UE->lte_frame_parms.Ncp == NORMAL) {  // normal prefix
    pilot2 = 7;
  } else { // extended prefix
    pilot2 = 6;
  }


  while (!oai_exit) {
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_THREAD, 0);

    if (pthread_mutex_lock(&rx_pdsch_mutex) != 0) {
      LOG_E(PHY,"[SCHED][RX_PDSCH] error locking mutex.\n");
    } else {
      while (rx_pdsch_instance_cnt < 0) {
        pthread_cond_wait(&rx_pdsch_cond,&rx_pdsch_mutex);
      }

      if (pthread_mutex_unlock(&rx_pdsch_mutex) != 0) {
        LOG_E(PHY,"[SCHED][RX_PDSCH] error unlocking mutex.\n");
      }
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PDSCH_THREAD, 1);

    //      last_slot = rx_pdsch_slot;
    subframe = UE->slot_rx>>1;
    // Important! assumption that PDCCH procedure of next SF is not called yet
    harq_pid = UE->dlsch_ue[eNB_id][0]->current_harq_pid;
    UE->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->G = get_G(&UE->lte_frame_parms,
        UE->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->nb_rb,
        UE->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->rb_alloc_even,
        get_Qm(UE->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->mcs),
        UE->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->Nl,
        UE->lte_ue_pdcch_vars[eNB_id]->num_pdcch_symbols,
        UE->frame_rx,subframe);

    if ((UE->transmission_mode[eNB_id] == 5) &&
        (UE->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->dl_power_off==0) &&
        (openair_daq_vars.use_ia_receiver > 0)) {
      dual_stream_UE = 1;
      eNB_id_i = UE->n_connected_eNB;

      if (openair_daq_vars.use_ia_receiver == 2) {
        i_mod =  get_Qm(((UE->frame_rx%1024)/3)%28);
      } else {
        i_mod =  get_Qm(UE->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->mcs);
      }
    } else {
      dual_stream_UE = 0;
      eNB_id_i = eNB_id+1;
      i_mod = 0;
    }

    if (oai_exit) break;

    LOG_D(PHY,"[SCHED][RX_PDSCH] Frame %d, slot %d: Calling rx_pdsch_decoding with harq_pid %d\n",UE->frame_rx,UE->slot_rx,harq_pid);


    // Check if we are in even or odd slot
    if (UE->slot_rx%2) { // odd slots

      // measure time
      //time0 = rt_get_time_ns();
      //        rt_printk("[SCHED][RX_PDSCH][before rx_pdsch] Frame %d, slot %d, time %llu\n",UE->frame,last_slot,rt_get_time_ns());
      for (m=pilot2; m<UE->lte_frame_parms.symbols_per_tti; m++) {

        rx_pdsch(UE,
                 PDSCH,
                 eNB_id,
                 eNB_id_i,
                 subframe,
                 m,
                 0,
                 dual_stream_UE,
                 i_mod,
                 harq_pid);

      }

      //        time1 = rt_get_time_ns();
      //        rt_printk("[SCHED][RX_PDSCH] Frame %d, slot %d, start %llu, end %llu, proc time: %llu ns\n",UE->frame_rx,last_slot,time0,time1,(time1-time0));

      dlsch_thread_index = harq_pid;

      if (pthread_mutex_lock (&dlsch_mutex[dlsch_thread_index]) != 0) {               // Signal MAC_PHY Scheduler
        LOG_E(PHY,"[UE  %d] ERROR pthread_mutex_lock\n",UE->Mod_id);     // lock before accessing shared resource
        //  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_OUT);
        //return(-1);
      }

      dlsch_instance_cnt[dlsch_thread_index]++;
      dlsch_subframe[dlsch_thread_index] = subframe;
      pthread_mutex_unlock (&dlsch_mutex[dlsch_thread_index]);

      if (dlsch_instance_cnt[dlsch_thread_index] == 0) {
        if (pthread_cond_signal(&dlsch_cond[dlsch_thread_index]) != 0) {
          LOG_E(PHY,"[UE  %d] ERROR pthread_cond_signal for dlsch_cond[%d]\n",UE->Mod_id,dlsch_thread_index);
          //    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_OUT);
          //return(-1);
        }
      } else {
        LOG_W(PHY,"[UE  %d] DLSCH thread for dlsch_thread_index %d busy!!!\n",UE->Mod_id,dlsch_thread_index);
        //  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_PROCEDURES_UE_RX, VCD_FUNCTION_OUT);
        //return(-1);
      }

    } else { // even slots

      for (m=UE->lte_ue_pdcch_vars[eNB_id]->num_pdcch_symbols; m<pilot2; m++) {

        rx_pdsch(UE,
                 PDSCH,
                 eNB_id,
                 eNB_id_i,
                 subframe,
                 m,
                 (m==UE->lte_ue_pdcch_vars[eNB_id]->num_pdcch_symbols)?1:0,   // first_symbol_flag
                 dual_stream_UE,
                 i_mod,
                 harq_pid);
      }
    }


    if (pthread_mutex_lock(&rx_pdsch_mutex) != 0) {
      msg("[openair][SCHED][RX_PDSCH] error locking mutex.\n");
    } else {
      rx_pdsch_instance_cnt--;

      if (pthread_mutex_unlock(&rx_pdsch_mutex) != 0) {
        msg("[openair][SCHED][RX_PDSCH] error unlocking mutex.\n");
      }
    }

  }

#ifdef HARD_RT
  rt_make_soft_real_time();
#endif

  LOG_D(PHY,"[openair][SCHED][RX_PDSCH] RX_PDSCH thread exiting\n");

  return 0;
}

int init_rx_pdsch_thread(void)
{

  int error_code;
  struct sched_param p;

  pthread_mutex_init(&rx_pdsch_mutex,NULL);

  pthread_cond_init(&rx_pdsch_cond,NULL);

  pthread_attr_init (&attr_rx_pdsch_thread);
  pthread_attr_setstacksize(&attr_rx_pdsch_thread,OPENAIR_THREAD_STACK_SIZE);

  //attr_rx_pdsch_thread.priority = 1;

  p.sched_priority = OPENAIR_THREAD_PRIORITY;
  pthread_attr_setschedparam  (&attr_rx_pdsch_thread, &p);
#ifndef RTAI_ISNT_POSIX
  pthread_attr_setschedpolicy (&attr_rx_pdsch_thread, SCHED_FIFO);
#endif

  rx_pdsch_instance_cnt = -1;
  rt_printk("[openair][SCHED][RX_PDSCH][INIT] Allocating RX_PDSCH thread\n");
  error_code = pthread_create(&rx_pdsch_thread_var,
                              &attr_rx_pdsch_thread,
                              rx_pdsch_thread,
                              0);

  if (error_code!= 0) {
    rt_printk("[openair][SCHED][RX_PDSCH][INIT] Could not allocate rx_pdsch_thread, error %d\n",error_code);
    return(error_code);
  } else {
    rt_printk("[openair][SCHED][RX_PDSCH][INIT] Allocate rx_pdsch_thread successful\n");
    return(0);
  }

}

void cleanup_rx_pdsch_thread(void)
{

  rt_printk("[openair][SCHED][RX_PDSCH] Scheduling rx_pdsch_thread to exit\n");

  rx_pdsch_instance_cnt = 0;

  if (pthread_cond_signal(&rx_pdsch_cond) != 0)
    rt_printk("[openair][SCHED][RX_PDSCH] ERROR pthread_cond_signal\n");
  else
    rt_printk("[openair][SCHED][RX_PDSCH] Signalled rx_pdsch_thread to exit\n");

  rt_printk("[openair][SCHED][RX_PDSCH] Exiting ...\n");
  pthread_cond_destroy(&rx_pdsch_cond);
  pthread_mutex_destroy(&rx_pdsch_mutex);
}
