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

/*! \file lte-ue.c
 * \brief threads and support functions for real-time LTE UE target
 * \author R. Knopp, F. Kaltenberger, Navid Nikaein
 * \date 2015
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
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <linux/sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <syscall.h>
#include <sys/sysinfo.h>

#include "rt_wrapper.h"
#include "assertions.h"
#include "PHY/types.h"

#include "PHY/defs.h"
#ifdef OPENAIR2
#include "LAYER2/MAC/defs.h"
#include "RRC/LITE/extern.h"
#endif
#include "PHY_INTERFACE/extern.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#ifdef EXMIMO
#include "openair0_lib.h"
#else
#include "../../ARCH/COMMON/common_lib.h"
#endif

#include "PHY/extern.h"
#include "SCHED/extern.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/MAC/proto.h"

#include "UTIL/LOG/log_extern.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"

#define FRAME_PERIOD    100000000ULL
#define DAQ_PERIOD      66667ULL

typedef enum {
  pss=0,
  pbch=1,
  si=2
} sync_mode_t;

int init_dlsch_threads(void);
void cleanup_dlsch_threads(void);
int32_t init_rx_pdsch_thread(void);
void cleanup_rx_pdsch_thread(void);

extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;

extern openair0_config_t openair0_cfg[MAX_CARDS];
extern uint32_t          downlink_frequency[MAX_NUM_CCs][4];
extern int32_t           uplink_frequency_offset[MAX_NUM_CCs][4];
extern openair0_rf_map rf_map[MAX_NUM_CCs];

extern openair0_device openair0;
extern int oai_exit;

extern int32_t **rxdata;
extern int32_t **txdata;

//extern unsigned int tx_forward_nsamps;
//extern int tx_delay;

extern int rx_input_level_dBm;
extern uint8_t exit_missed_slots;
extern uint64_t num_missed_slots; // counter for the number of missed slots

extern void exit_fun(const char* s);

#ifdef EXMIMO

extern unsigned int             rxg_max[4];
extern unsigned int             rxg_med[4];
extern unsigned int             rxg_byp[4];
extern unsigned int             nf_max[4];
extern unsigned int             nf_med[4];
extern unsigned int             nf_byp[4];
extern rx_gain_t                rx_gain_mode[MAX_NUM_CCs][4];

extern double tx_gain[MAX_NUM_CCs][4];
extern double rx_gain[MAX_NUM_CCs][4];
#endif
#define KHz (1000UL)
#define MHz (1000 * KHz)

typedef struct eutra_band_s {
  int16_t band;
  uint32_t ul_min;
  uint32_t ul_max;
  uint32_t dl_min;
  uint32_t dl_max;
  lte_frame_type_t frame_type;
} eutra_band_t;

typedef struct band_info_s {
  int nbands;
  eutra_band_t band_info[100];
} band_info_t;

band_info_t bands_to_scan;

static const eutra_band_t eutra_bands[] = {
  { 1, 1920    * MHz, 1980    * MHz, 2110    * MHz, 2170    * MHz, FDD},
  { 2, 1850    * MHz, 1910    * MHz, 1930    * MHz, 1990    * MHz, FDD},
  { 3, 1710    * MHz, 1785    * MHz, 1805    * MHz, 1880    * MHz, FDD},
  { 4, 1710    * MHz, 1755    * MHz, 2110    * MHz, 2155    * MHz, FDD},
  { 5,  824    * MHz,  849    * MHz,  869    * MHz,  894    * MHz, FDD},
  { 6,  830    * MHz,  840    * MHz,  875    * MHz,  885    * MHz, FDD},
  { 7, 2500    * MHz, 2570    * MHz, 2620    * MHz, 2690    * MHz, FDD},
  { 8,  880    * MHz,  915    * MHz,  925    * MHz,  960    * MHz, FDD},
  { 9, 1749900 * KHz, 1784900 * KHz, 1844900 * KHz, 1879900 * KHz, FDD},
  {10, 1710    * MHz, 1770    * MHz, 2110    * MHz, 2170    * MHz, FDD},
  {11, 1427900 * KHz, 1452900 * KHz, 1475900 * KHz, 1500900 * KHz, FDD},
  {12,  698    * MHz,  716    * MHz,  728    * MHz,  746    * MHz, FDD},
  {13,  777    * MHz,  787    * MHz,  746    * MHz,  756    * MHz, FDD},
  {14,  788    * MHz,  798    * MHz,  758    * MHz,  768    * MHz, FDD},
  {17,  704    * MHz,  716    * MHz,  734    * MHz,  746    * MHz, FDD},
  {20,  832    * MHz,  862    * MHz,  791    * MHz,  821    * MHz, FDD},
  {22, 3510    * MHz, 3590    * MHz, 3410    * MHz, 3490    * MHz, FDD},
  {33, 1900    * MHz, 1920    * MHz, 1900    * MHz, 1920    * MHz, TDD},
  {34, 2010    * MHz, 2025    * MHz, 2010    * MHz, 2025    * MHz, TDD},
  {35, 1850    * MHz, 1910    * MHz, 1850    * MHz, 1910    * MHz, TDD},
  {36, 1930    * MHz, 1990    * MHz, 1930    * MHz, 1990    * MHz, TDD},
  {37, 1910    * MHz, 1930    * MHz, 1910    * MHz, 1930    * MHz, TDD},
  {38, 2570    * MHz, 2620    * MHz, 2570    * MHz, 2630    * MHz, TDD},
  {39, 1880    * MHz, 1920    * MHz, 1880    * MHz, 1920    * MHz, TDD},
  {40, 2300    * MHz, 2400    * MHz, 2300    * MHz, 2400    * MHz, TDD},
  {41, 2496    * MHz, 2690    * MHz, 2496    * MHz, 2690    * MHz, TDD},
  {42, 3400    * MHz, 3600    * MHz, 3400    * MHz, 3600    * MHz, TDD},
  {43, 3600    * MHz, 3800    * MHz, 3600    * MHz, 3800    * MHz, TDD},
  {44, 703    * MHz, 803    * MHz, 703    * MHz, 803    * MHz, TDD},
};

/*!
 * \brief This is the UE synchronize thread.
 * It performs band scanning and synchonization.
 * \param arg is a pointer to a \ref PHY_VARS_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void *UE_thread_synch(void *arg)
{
  static int UE_thread_synch_retval;
  int i, hw_slot_offset;
  PHY_VARS_UE *UE = (PHY_VARS_UE*) arg;
  int current_band = 0;
  int current_offset = 0;
  sync_mode_t sync_mode = pbch;
  int card;
  int ind;
  int found;
  int freq_offset=0;

  UE->is_synchronized = 0;
  printf("UE_thread_sync in with PHY_vars_UE %p\n",arg);
  printf("waiting for sync (UE_thread_synch) \n");

#ifndef DEADLINE_SCHEDULER
  int policy, s, j;
  struct sched_param sparam;
  char cpu_affinity[1024];
  cpu_set_t cpuset;

  /* Set affinity mask to include CPUs 1 to MAX_CPUS */
  /* CPU 0 is reserved for UHD threads */
  CPU_ZERO(&cpuset);

  #ifdef CPU_AFFINITY
  if (get_nprocs() >2)
  {
    for (j = 1; j < get_nprocs(); j++)
      CPU_SET(j, &cpuset);

    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
      perror( "pthread_setaffinity_np");
      exit_fun("Error setting processor affinity");
    }
  }
  #endif

  /* Check the actual affinity mask assigned to the thread */

  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0)
  {
    perror( "pthread_getaffinity_np");
    exit_fun("Error getting processor affinity ");
  }
  memset(cpu_affinity, 0 , sizeof(cpu_affinity));
  for (j = 0; j < CPU_SETSIZE; j++)
  if (CPU_ISSET(j, &cpuset))
  {  
     char temp[1024];
     sprintf(temp, " CPU_%d ", j);    
     strcat(cpu_affinity, temp);
  }

  memset(&sparam, 0 , sizeof (sparam));
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO)-1;
  policy = SCHED_FIFO ; 
  
  s = pthread_setschedparam(pthread_self(), policy, &sparam);
  if (s != 0)
     {
     perror("pthread_setschedparam : ");
     exit_fun("Error setting thread priority");
     }
  s = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (s != 0)
   {
     perror("pthread_getschedparam : ");
     exit_fun("Error getting thread priority");

   }

  LOG_I( HW, "[SCHED][UE] Started UE synch thread on CPU %d TID %ld , sched_policy = %s, priority = %d, CPU Affinity = %s \n", (int)sched_getcpu(), gettid(),
                   (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                   (policy == SCHED_RR)    ? "SCHED_RR" :
                   (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                   "???",
                   (int) sparam.sched_priority, cpu_affinity);

#endif


  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_sync_thread)\n");

  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);

  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex (UE_sync_thread)\n");

  printf("starting UE synch thread (IC %d)\n",UE->instance_cnt_synch);
  ind = 0;
  found = 0;


  if (UE->UE_scan == 0) {
    do  {
      current_band = eutra_bands[ind].band;
      printf( "Scanning band %d, dl_min %"PRIu32", ul_min %"PRIu32"\n", current_band, eutra_bands[ind].dl_min,eutra_bands[ind].ul_min);

      if ((eutra_bands[ind].dl_min <= downlink_frequency[0][0]) && (eutra_bands[ind].dl_max >= downlink_frequency[0][0])) {
        for (card=0; card<MAX_NUM_CCs; card++)
          for (i=0; i<4; i++)
            uplink_frequency_offset[card][i] = eutra_bands[ind].ul_min - eutra_bands[ind].dl_min;

        found = 1;
        break;
      }

      ind++;
    } while (ind < sizeof(eutra_bands) / sizeof(eutra_bands[0]));
  
    if (found == 0) {
      exit_fun("Can't find EUTRA band for frequency");
      return &UE_thread_synch_retval;
    }






    LOG_I( PHY, "[SCHED][UE] Check absolute frequency DL %"PRIu32", UL %"PRIu32" (oai_exit %d)\n", downlink_frequency[0][0], downlink_frequency[0][0]+uplink_frequency_offset[0][0],oai_exit );

    for (i=0;i<openair0_cfg[0].rx_num_channels;i++) {
      openair0_cfg[0].rx_freq[i] = downlink_frequency[0][i];
      openair0_cfg[0].tx_freq[i] = downlink_frequency[0][i]+uplink_frequency_offset[0][i];
      openair0_cfg[0].autocal[i] = 1;
    }

    sync_mode = pbch;

  } else if  (UE->UE_scan == 1) {
    current_band=0;

    for (card=0; card<MAX_CARDS; card++) {
      for (i=0; i<openair0_cfg[card].rx_num_channels; i++) {
        downlink_frequency[card][i] = bands_to_scan.band_info[0].dl_min;
        uplink_frequency_offset[card][i] = bands_to_scan.band_info[0].ul_min-bands_to_scan.band_info[0].dl_min;

        openair0_cfg[card].rx_freq[i] = downlink_frequency[card][i];
        openair0_cfg[card].tx_freq[i] = downlink_frequency[card][i]+uplink_frequency_offset[card][i];
#ifdef OAI_USRP
        openair0_cfg[card].rx_gain[i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;

#if 0 // UHD 3.8	
        switch(UE->lte_frame_parms.N_RB_DL) {
        case 6:
          openair0_cfg[card].rx_gain[i] -= 12;
          break;

        case 25:
          openair0_cfg[card].rx_gain[i] -= 6;
          break;

        case 50:
          openair0_cfg[card].rx_gain[i] -= 3;
          break;

        case 100:
          openair0_cfg[card].rx_gain[i] -= 0;
          break;

        default:
          printf( "Unknown number of RBs %d\n", UE->lte_frame_parms.N_RB_DL );
          break;
        }
#endif
        printf( "UE synch: setting RX gain (%d,%d) to %f\n", card, i, openair0_cfg[card].rx_gain[i] );
#endif
      }
    }

  }

  while (oai_exit==0) {

    if (pthread_mutex_lock(&UE->mutex_synch) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE initial synch thread\n" );
      exit_fun("noting to add");
      return &UE_thread_synch_retval;
    }
    

    while (UE->instance_cnt_synch < 0) {
      // the thread waits here most of the time
      pthread_cond_wait( &UE->cond_synch, &UE->mutex_synch );
    }

    if (pthread_mutex_unlock(&UE->mutex_synch) != 0) {
      LOG_E( PHY, "[SCHED][eNB] error unlocking mutex for UE Initial Synch thread\n" );
      exit_fun("nothing to add");
      return &UE_thread_synch_retval;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SYNCH, 1 );

    switch (sync_mode) {
    case pss:
      LOG_I(PHY,"[SCHED][UE] Scanning band %d (%d), freq %u\n",bands_to_scan.band_info[current_band].band, current_band,bands_to_scan.band_info[current_band].dl_min+current_offset);
      lte_sync_timefreq(UE,current_band,bands_to_scan.band_info[current_band].dl_min+current_offset);
      current_offset += 20000000; // increase by 20 MHz

      if (current_offset > bands_to_scan.band_info[current_band].dl_max-bands_to_scan.band_info[current_band].dl_min) {
        current_band++;
        current_offset=0;
      }

      if (current_band==bands_to_scan.nbands) {
        current_band=0;
        oai_exit=1;
      }

      for (card=0; card<MAX_CARDS; card++) {
        for (i=0; i<openair0_cfg[card].rx_num_channels; i++) {
          downlink_frequency[card][i] = bands_to_scan.band_info[current_band].dl_min+current_offset;
          uplink_frequency_offset[card][i] = bands_to_scan.band_info[current_band].ul_min-bands_to_scan.band_info[0].dl_min + current_offset;


          openair0_cfg[card].rx_freq[i] = downlink_frequency[card][i];
          openair0_cfg[card].tx_freq[i] = downlink_frequency[card][i]+uplink_frequency_offset[card][i];
#ifdef OAI_USRP
          openair0_cfg[card].rx_gain[i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;  // 65 calibrated for USRP B210 @ 2.6 GHz

#if 0 // UHD 3.8	  
          switch(UE->lte_frame_parms.N_RB_DL) {
          case 6:
            openair0_cfg[card].rx_gain[i] -= 12;
            break;

          case 25:
            openair0_cfg[card].rx_gain[i] -= 6;
            break;

          case 50:
            openair0_cfg[card].rx_gain[i] -= 3;
            break;

          case 100:
            openair0_cfg[card].rx_gain[i] -= 0;
            break;

          default:
            printf("Unknown number of RBs %d\n",UE->lte_frame_parms.N_RB_DL);
            break;
          }
#endif	  

          printf("UE synch: setting RX gain (%d,%d) to %f\n",card,i,openair0_cfg[card].rx_gain[i]);
#endif

        }

      }

      if (UE->UE_scan_carrier) {

	for (i=0;i<openair0_cfg[0].rx_num_channels;i++)
	  openair0_cfg[0].autocal[i] = 1;

      }


      break;
 
    case pbch:

      LOG_I(PHY,"[UE thread Synch] Running Initial Synch\n");
      if (initial_sync( UE, UE->mode ) == 0) {

        hw_slot_offset = (UE->rx_offset<<1) / UE->lte_frame_parms.samples_per_tti;
        LOG_I( HW, "Got synch: hw_slot_offset %d\n", hw_slot_offset );
	if (UE->UE_scan_carrier == 1) {

	  UE->UE_scan_carrier = 0;
	  // rerun with new cell parameters and frequency-offset
	  for (i=0;i<openair0_cfg[0].rx_num_channels;i++) {
	    openair0_cfg[0].rx_gain[i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;
	    openair0_cfg[0].rx_freq[i] -= UE->lte_ue_common_vars.freq_offset;
	    openair0_cfg[0].tx_freq[i] =  openair0_cfg[0].rx_freq[i]+uplink_frequency_offset[0][i];
	    downlink_frequency[0][i] = openair0_cfg[0].rx_freq[i];
	    freq_offset=0;	    
	  }

	  // reconfigure for potentially different bandwidth
	  switch(UE->lte_frame_parms.N_RB_DL) {
	  case 6:
	    openair0_cfg[0].sample_rate =1.92e6;
	    openair0_cfg[0].rx_bw          =.96e6;
	    openair0_cfg[0].tx_bw          =.96e6;
	    //            openair0_cfg[0].rx_gain[0] -= 12;
	    break;
	  case 25:
	    openair0_cfg[0].sample_rate =7.68e6;
	    openair0_cfg[0].rx_bw          =2.5e6;
	    openair0_cfg[0].tx_bw          =2.5e6;
	    //            openair0_cfg[0].rx_gain[0] -= 6;
	    break;
	  case 50:
	    openair0_cfg[0].sample_rate =15.36e6;
	    openair0_cfg[0].rx_bw          =5.0e6;
	    openair0_cfg[0].tx_bw          =5.0e6;
	    //            openair0_cfg[0].rx_gain[0] -= 3;
	    break;
	  case 100:
	    openair0_cfg[0].sample_rate=30.72e6;
	    openair0_cfg[0].rx_bw=10.0e6;
	    openair0_cfg[0].tx_bw=10.0e6;
	    //            openair0_cfg[0].rx_gain[0] -= 0;
	    break;
	  }
#ifndef EXMIMO
	  openair0.trx_set_freq_func(&openair0,&openair0_cfg[0],0);
	  //openair0.trx_set_gains_func(&openair0,&openair0_cfg[0]);
	  //openair0.trx_stop_func(0);	  
#else
	  openair0_set_frequencies(&openair0,&openair0_cfg[0],0);
	  openair0_set_gains(&openair0,&openair0_cfg[0]);
	  openair0_stop(0);
#endif
	  sleep(1);
	  init_frame_parms(&UE->lte_frame_parms,1);
	}
	else {
	  UE->is_synchronized = 1;

	 if( UE->mode == rx_dump_frame ){
	   FILE *fd;
	   if ((UE->frame_rx&1) == 0) {  // this guarantees SIB1 is present 
	     if ((fd = fopen("rxsig_frame0.dat","w")) != NULL) {
	       fwrite((void*)&UE->lte_ue_common_vars.rxdata[0][0],
		      sizeof(int32_t),
		      10*UE->lte_frame_parms.samples_per_tti,
		      fd);
	       LOG_I(PHY,"Dummping Frame ... bye bye \n");
	       fclose(fd);
	       exit(0);
	     }
	     else {
	       LOG_E(PHY,"Cannot open file for writing\n");
	       exit(0);
	     }
	   }
	   else {
	     UE->is_synchronized = 0;
	   }
	 }
	 

#ifndef EXMIMO
	  UE->slot_rx = 0;
	  UE->slot_tx = 4;
#else
	  UE->slot_rx = 18;
	  UE->slot_tx = 2;
#endif
	}
      } else {
        // initial sync failed
        // calculate new offset and try again
	if (UE->UE_scan_carrier == 1) {
	  if (freq_offset >= 0) {
	    freq_offset += 100;
	    freq_offset *= -1;
	  } else {
	    freq_offset *= -1;
	  }
	
	  if (abs(freq_offset) > 7500) {
	    LOG_I( PHY, "[initial_sync] No cell synchronization found, abandoning\n" );
	    FILE *fd;
	    if ((fd = fopen("rxsig_frame0.dat","w"))!=NULL) {
	      fwrite((void*)&UE->lte_ue_common_vars.rxdata[0][0],
		     sizeof(int32_t),
		     10*UE->lte_frame_parms.samples_per_tti,
		     fd);
	      LOG_I(PHY,"Dummping Frame ... bye bye \n");
	      fclose(fd);
	      exit(0);
	    }
	    mac_xface->macphy_exit("No cell synchronization found, abandoning");
	    return &UE_thread_synch_retval; // not reached
	  }
	}
	else {
	  
	}
        LOG_I( PHY, "[initial_sync] trying carrier off %d Hz, rxgain %d (DL %u, UL %u)\n", 
	       freq_offset,
               UE->rx_total_gain_dB,
               downlink_frequency[0][0]+freq_offset,
               downlink_frequency[0][0]+uplink_frequency_offset[0][0]+freq_offset );

        for (card=0; card<MAX_CARDS; card++) {
          for (i=0; i<openair0_cfg[card].rx_num_channels; i++) {
            openair0_cfg[card].rx_freq[i] = downlink_frequency[card][i]+freq_offset;
            openair0_cfg[card].tx_freq[i] = downlink_frequency[card][i]+uplink_frequency_offset[card][i]+freq_offset;
#ifndef EXMIMO
	    openair0.trx_set_freq_func(&openair0,&openair0_cfg[0],0);
	    
#else
	    openair0_set_frequencies(&openair0,&openair0_cfg[0],0);
	    
#endif

#if defined(OAI_USRP) || defined(OAI_BLADERF) || defined(OAI_LMSSDR)
            openair0_cfg[card].rx_gain[i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;
	    
	    
#if 0
            switch(UE->lte_frame_parms.N_RB_DL) {
            case 6:
              openair0_cfg[card].rx_gain[i] -= 12;
              break;

            case 25:
              openair0_cfg[card].rx_gain[i] -= 6;
              break;

            case 50:
              openair0_cfg[card].rx_gain[i] -= 0;//3;
              break;

            case 100:
              openair0_cfg[card].rx_gain[i] -= 0;
              break;

            default:
              printf("Unknown number of RBs %d\n",UE->lte_frame_parms.N_RB_DL);
              break;
            }
#endif	    
#endif
          }
        }
	if (UE->UE_scan_carrier==1) {
	  for (i=0;i<openair0_cfg[0].rx_num_channels;i++)
	    openair0_cfg[0].autocal[i] = 1;
	  
	}
      }// initial_sync=0

      break;

    case si:
    default:
      break;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SYNCH, 0 );



    if (pthread_mutex_lock(&UE->mutex_synch) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE synch\n" );
      exit_fun("noting to add");
      return &UE_thread_synch_retval;
    }

    // indicate readiness
    UE->instance_cnt_synch--;

    if (pthread_mutex_unlock(&UE->mutex_synch) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE synch\n" );
      exit_fun("noting to add");
      return &UE_thread_synch_retval;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SYNCH, 0 );
  }  // while !oai_exit

  return &UE_thread_synch_retval;
}

/*!
 * \brief This is the UE transmit thread.
 * This thread performs the phy_procedures_UE_TX() on every transmit slot.
 * \param arg is a pointer to a \ref PHY_VARS_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void *UE_thread_tx(void *arg)
{
  static int UE_thread_tx_retval;
  //int ret;

  PHY_VARS_UE *UE = (PHY_VARS_UE*)arg;

  UE->instance_cnt_tx=-1;

#ifdef RTAI
  RT_TASK *task = rt_task_init_schmod(nam2num("UE TX Thread"), 0, 0, 0, SCHED_FIFO, 0xF);

  if (task==NULL) {
    LOG_E(PHY,"[SCHED][UE] Problem starting UE TX thread!!!!\n");
    return 0;
  }

  LOG_D(HW,"Started UE TX thread (id %p)\n",task);
#else

#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;
  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  /* This creates a 1ms reservation every 10ms period*/
  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  = 900000;  // each tx thread requires .5ms to finish its job
  attr.sched_deadline = 1000000; // each tx thread will finish within 1ms
  attr.sched_period   = 1000000; // each tx thread has a period of 1ms from the starting point


  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] UE_thread_tx thread: sched_setattr failed\n");
    return &UE_thread_tx_retval;
  }

#else
  int policy, s, j;
  struct sched_param sparam;
  char cpu_affinity[1024];
  cpu_set_t cpuset;

  /* Set affinity mask to include CPUs 1 to MAX_CPUS */
  /* CPU 0 is reserved for UHD threads */
  CPU_ZERO(&cpuset);

  #ifdef CPU_AFFINITY
  if (get_nprocs() >2)
  {
    for (j = 1; j < get_nprocs(); j++)
      CPU_SET(j, &cpuset);

    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
      perror( "pthread_setaffinity_np");
      exit_fun("Error setting processor affinity");
    }
  }
  #endif

  /* Check the actual affinity mask assigned to the thread */

  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0)
  {
    perror( "pthread_getaffinity_np");
    exit_fun("Error getting processor affinity ");
  }
  memset(cpu_affinity, 0 , sizeof(cpu_affinity));
  for (j = 0; j < CPU_SETSIZE; j++)
  if (CPU_ISSET(j, &cpuset))
  {  
     char temp[1024];
     sprintf(temp, " CPU_%d ", j);    
     strcat(cpu_affinity, temp);
  }

  memset(&sparam, 0 , sizeof (sparam));
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO)-1;
  policy = SCHED_FIFO ; 
  
  s = pthread_setschedparam(pthread_self(), policy, &sparam);
  if (s != 0)
     {
     perror("pthread_setschedparam : ");
     exit_fun("Error setting thread priority");
     }
  s = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (s != 0)
   {
     perror("pthread_getschedparam : ");
     exit_fun("Error getting thread priority");

   }

  LOG_I( HW, "[SCHED][UE] Started UE thread TX on CPU %d TID %ld , sched_policy = %s, priority = %d, CPU Affinity = %s \n", (int)sched_getcpu(), gettid(),
                   (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                   (policy == SCHED_RR)    ? "SCHED_RR" :
                   (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                   "???",
                   (int) sparam.sched_priority, cpu_affinity);


#endif
#endif

  printf("waiting for sync (UE_thread_tx)\n");

  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_thread_tx)\n");

  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);

  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex, waiting (UE_thread_tx)\n");

  printf("Starting UE TX thread\n");

  // Lock memory from swapping. This is a process wide call (not constraint to this thread).
  mlockall(MCL_CURRENT | MCL_FUTURE);

  while (!oai_exit) {

    if (pthread_mutex_lock(&UE->mutex_tx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE TX\n" );
      exit_fun("nothing to add");
      return &UE_thread_tx_retval;
    }

    while (UE->instance_cnt_tx < 0) {
      // most of the time, the thread is waiting here
      pthread_cond_wait( &UE->cond_tx, &UE->mutex_tx );
    }

    if (pthread_mutex_unlock(&UE->mutex_tx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE TX\n" );
      exit_fun("nothing to add");
      return &UE_thread_tx_retval;
    }
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_THREAD_TX, 1 );

    if ((subframe_select( &UE->lte_frame_parms, UE->slot_tx>>1 ) == SF_UL) ||
        (UE->lte_frame_parms.frame_type == FDD)) {
      phy_procedures_UE_TX( UE, 0, 0, UE->mode, no_relay );
    }

    if ((subframe_select( &UE->lte_frame_parms, UE->slot_tx>>1 ) == SF_S) &&
        ((UE->slot_tx&1) == 1)) {
      phy_procedures_UE_S_TX( UE, 0, 0, no_relay );
    }

    UE->slot_tx += 2;

    if (UE->slot_tx >= 20) {
      UE->slot_tx -= 20;
      UE->frame_tx++;
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX_UE, UE->frame_tx );
    }

    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX_UE, UE->slot_tx>>1 );

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_THREAD_TX, 0 );

    if (pthread_mutex_lock(&UE->mutex_tx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE TX thread\n" );
      exit_fun("nothing to add");
      return &UE_thread_tx_retval;
    }

    UE->instance_cnt_tx--;
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE_INST_CNT_TX, UE->instance_cnt_tx);

    if (pthread_mutex_unlock(&UE->mutex_tx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE TX thread\n" );
      exit_fun("nothing to add");
      return &UE_thread_tx_retval;
    }

  }

  return &UE_thread_tx_retval;
}

/*!
 * \brief This is the UE receive thread.
 * This thread performs the phy_procedures_UE_RX() on every received slot.
 * \param arg is a pointer to a \ref PHY_VARS_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */

/*
#ifdef OAI_USRP
void rescale(int16_t *input,int length)
{
#if defined(__x86_64__) || defined(__i386__)
  __m128i *input128 = (__m128i *)input;
#elif defined(__arm__)
  int16x8_t *input128 = (int16x8_t *)input;
#endif
  int i;

  for (i=0; i<length>>2; i++) {
#if defined(__x86_64__) || defined(__i386__)
    input128[i] = _mm_srai_epi16(input128[i],4);
#elif defined(__arm__)
    input128[i] = vshrq_n_s16(input128[i],4);
#endif
  }
}
#endif
*/

static void *UE_thread_rx(void *arg)
{
  static int UE_thread_rx_retval;
  PHY_VARS_UE *UE = (PHY_VARS_UE*)arg;
  int i;
  int ret;

  UE->instance_cnt_rx=-1;

#ifdef RTAI
  RT_TASK *task = rt_task_init_schmod(nam2num("UE RX Thread"), 0, 0, 0, SCHED_FIFO, 0xF);

  if (task==NULL) {
    LOG_E(PHY,"[SCHED][UE] Problem starting UE RX thread!!!!\n");
    return &UE_thread_rx_retval;
  }

  LOG_D(HW,"Started UE RX thread (id %p)\n",task);
#else

#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;
  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  // This creates a .5ms reservation every 1ms period
  attr.sched_policy   = SCHED_DEADLINE;
  attr.sched_runtime  = 900000;  // each rx thread requires 1ms to finish its job
  attr.sched_deadline = 1000000; // each rx thread will finish within 1ms
  attr.sched_period   = 1000000; // each rx thread has a period of 1ms from the starting point

  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] UE_thread_rx : sched_setattr failed\n");
    return &UE_thread_rx_retval;
  }

#else
  int policy, s, j;
  struct sched_param sparam;
  char cpu_affinity[1024];
  cpu_set_t cpuset;

  /* Set affinity mask to include CPUs 1 to MAX_CPUS */
  /* CPU 0 is reserved for UHD threads */
  CPU_ZERO(&cpuset);

  #ifdef CPU_AFFINITY
  if (get_nprocs() >2)
  {
    for (j = 1; j < get_nprocs(); j++)
      CPU_SET(j, &cpuset);

    s = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (s != 0)
    {
      perror( "pthread_setaffinity_np");
      exit_fun("Error setting processor affinity");
    }
  }
  #endif

  /* Check the actual affinity mask assigned to the thread */

  s = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  if (s != 0)
  {
    perror( "pthread_getaffinity_np");
    exit_fun("Error getting processor affinity ");
  }
  memset(cpu_affinity, 0 , sizeof(cpu_affinity));
  for (j = 0; j < CPU_SETSIZE; j++)
  if (CPU_ISSET(j, &cpuset))
  {  
     char temp[1024];
     sprintf(temp, " CPU_%d ", j);    
     strcat(cpu_affinity, temp);
  }

  memset(&sparam, 0 , sizeof (sparam));
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO)-1;
  policy = SCHED_FIFO ; 
  
  s = pthread_setschedparam(pthread_self(), policy, &sparam);
  if (s != 0)
     {
     perror("pthread_setschedparam : ");
     exit_fun("Error setting thread priority");
     }
  s = pthread_getschedparam(pthread_self(), &policy, &sparam);
  if (s != 0)
   {
     perror("pthread_getschedparam : ");
     exit_fun("Error getting thread priority");

   }

  LOG_I( HW, "[SCHED][UE] Started UE RX thread on CPU %d TID %ld , sched_policy = %s, priority = %d, CPU Affinity = %s \n", (int)sched_getcpu(), gettid(),
                   (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
                   (policy == SCHED_RR)    ? "SCHED_RR" :
                   (policy == SCHED_OTHER) ? "SCHED_OTHER" :
                   "???",
                   (int) sparam.sched_priority, cpu_affinity);


#endif
#endif

  // Lock memory from swapping. This is a process wide call (not constraint to this thread).
  mlockall(MCL_CURRENT | MCL_FUTURE);

  printf("waiting for sync (UE_thread_rx)\n");

  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_thread_rx)\n");

  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);

  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex, waiting (UE_thread_rx)\n");

  printf("Starting UE RX thread\n");

  while (!oai_exit) {
    if (pthread_mutex_lock(&UE->mutex_rx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE RX\n" );
      exit_fun("nothing to add");
      return &UE_thread_rx_retval;
    }

    while (UE->instance_cnt_rx < 0) {
      // most of the time, the thread is waiting here
      pthread_cond_wait( &UE->cond_rx, &UE->mutex_rx );
    }

    if (pthread_mutex_unlock(&UE->mutex_rx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE RX\n" );
      exit_fun("nothing to add");
      return &UE_thread_rx_retval;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_THREAD_RX, 1 );
    for (i=0; i<2; i++) {
      if ((subframe_select( &UE->lte_frame_parms, UE->slot_rx>>1 ) == SF_DL) ||
          (UE->lte_frame_parms.frame_type == FDD)) {
	/*
#ifdef OAI_USRP
	// this does the adjustments of RX signal amplitude to bring into least 12 significant bits
	int slot_length = UE->lte_frame_parms.samples_per_tti>>1;
	int rx_offset = (UE->slot_rx)*slot_length + UE->rx_offset;
	int frame_length = UE->lte_frame_parms.samples_per_tti*10;
	int aa;
	if (rx_offset > frame_length)
	  rx_offset-=frame_length;


	if (rx_offset >= 0) {
	  if (rx_offset + slot_length < frame_length)
	    for (aa=0;aa<UE->lte_frame_parms.nb_antennas_rx;aa++)
	      rescale((int16_t*)&UE->lte_ue_common_vars.rxdata[aa][rx_offset&(~0x3)],
		      slot_length);
	  else {
	    int diff = rx_offset + slot_length - frame_length;
	    for (aa=0;aa<UE->lte_frame_parms.nb_antennas_rx;aa++){
	      rescale((int16_t*)&UE->lte_ue_common_vars.rxdata[aa][rx_offset&(~0x3)],
		      slot_length-diff);
	      rescale((int16_t*)&UE->lte_ue_common_vars.rxdata[aa][0],
		      diff);
	    }
	  }
	}
	else {
	    for (aa=0;aa<UE->lte_frame_parms.nb_antennas_rx;aa++){
	      rescale((int16_t*)&UE->lte_ue_common_vars.rxdata[aa][(frame_length+rx_offset)&(~0x3)],
		      -rx_offset);
	      rescale((int16_t*)&UE->lte_ue_common_vars.rxdata[aa][0],
		      slot_length+rx_offset);
	    }
	}
#endif
	*/
        phy_procedures_UE_RX( UE, 0, 0, UE->mode, no_relay, NULL );
      }

      if ((subframe_select( &UE->lte_frame_parms, UE->slot_rx>>1 ) == SF_S) &&
          ((UE->slot_rx&1) == 0)) {
	/*
#ifdef OAI_USRP
	// this does the adjustments of RX signal amplitude to bring into least 12 significant bits
	int slot_length = UE->lte_frame_parms.samples_per_tti>>1;
	int rx_offset = (UE->slot_rx)*slot_length + UE->rx_offset;
	int frame_length = UE->lte_frame_parms.samples_per_tti*10;
	if (rx_offset > frame_length)
	  rx_offset-=frame_length;
	int aa;

	if (rx_offset >= 0) {
	  if (rx_offset + slot_length < frame_length)
	    for (aa=0;aa<UE->lte_frame_parms.nb_antennas_rx;aa++)
	      rescale((int16_t*)&UE->lte_ue_common_vars.rxdata[aa][rx_offset&(~0x3)],
		      slot_length);
	  else {
	    int diff = rx_offset + slot_length - frame_length;
	    for (aa=0;aa<UE->lte_frame_parms.nb_antennas_rx;aa++){
	      rescale((int16_t*)&UE->lte_ue_common_vars.rxdata[aa][rx_offset&(~0x3)],
		      slot_length-diff);
	      rescale((int16_t*)&UE->lte_ue_common_vars.rxdata[aa][0],
		      diff);
	    }
	  }
	}
	else {
	  for (aa=0;aa<UE->lte_frame_parms.nb_antennas_rx;aa++){
	    rescale((int16_t*)&UE->lte_ue_common_vars.rxdata[aa][(frame_length+rx_offset)&(~0x3)],
		    -rx_offset);
	    rescale((int16_t*)&UE->lte_ue_common_vars.rxdata[aa][0],
		    slot_length+rx_offset);
	  }
	}
#endif
	*/
        phy_procedures_UE_RX( UE, 0, 0, UE->mode, no_relay, NULL );
      }

      if ((UE->mac_enabled==1) && (i==0)) {
        ret = mac_xface->ue_scheduler(UE->Mod_id,
                                      UE->frame_tx,
                                      UE->slot_rx>>1,
                                      subframe_select(&UE->lte_frame_parms,UE->slot_tx>>1),
                                      0,
                                      0/*FIXME CC_id*/);

        if (ret == CONNECTION_LOST) {
          LOG_E( PHY, "[UE %"PRIu8"] Frame %"PRIu32", subframe %u RRC Connection lost, returning to PRACH\n",
                 UE->Mod_id, UE->frame_rx, UE->slot_tx>>1 );
          UE->UE_mode[0] = PRACH;
        } else if (ret == PHY_RESYNCH) {
          LOG_E( PHY, "[UE %"PRIu8"] Frame %"PRIu32", subframe %u RRC Connection lost, trying to resynch\n",
                 UE->Mod_id, UE->frame_rx, UE->slot_tx>>1 );
          UE->UE_mode[0] = RESYNCH;
        } else if (ret == PHY_HO_PRACH) {
          LOG_I( PHY, "[UE %"PRIu8"] Frame %"PRIu32", subframe %u, return to PRACH and perform a contention-free access\n",
                 UE->Mod_id, UE->frame_rx, UE->slot_tx>>1 );
          UE->UE_mode[0] = PRACH;
        }
      }

      UE->slot_rx++;

      if (UE->slot_rx == 20) {
        UE->slot_rx = 0;
        UE->frame_rx++;
        VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX_UE, UE->frame_rx );
      }

      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX_UE, UE->slot_rx>>1 );
    }
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_THREAD_RX, 0 );

    if (pthread_mutex_lock(&UE->mutex_rx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE RX\n" );
      exit_fun("noting to add");
      return &UE_thread_rx_retval;
    }

    UE->instance_cnt_rx--;
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE_INST_CNT_RX, UE->instance_cnt_rx);

    if (pthread_mutex_unlock(&UE->mutex_rx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE RX\n" );
      exit_fun("noting to add");
      return &UE_thread_rx_retval;
    }
  }

  // thread finished
  return &UE_thread_rx_retval;
}




#ifndef EXMIMO
#define RX_OFF_MAX 10
#define RX_OFF_MIN 5
#define RX_OFF_MID ((RX_OFF_MAX+RX_OFF_MIN)/2)

/*!
 * \brief This is the main UE thread.
 * This thread controls the other three UE threads:
 * - UE_thread_rx
 * - UE_thread_tx
 * - UE_thread_synch
 * \param arg unused
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
void *UE_thread(void *arg)
{
  UNUSED(arg)
  static int UE_thread_retval;
  PHY_VARS_UE *UE = PHY_vars_UE_g[0][0];
  int spp = openair0_cfg[0].samples_per_packet;
  int slot=1, frame=0, hw_subframe=0, rxpos=0, txpos=openair0_cfg[0].tx_scheduling_advance;
#ifdef __AVX2__
  int dummy[2][spp] __attribute__((aligned(32)));
#else
  int dummy[2][spp] __attribute__((aligned(16)));
#endif
  int dummy_dump = 0;
  int tx_enabled = 0;
  int start_rx_stream = 0;
  int rx_off_diff = 0;
  int rx_correction_timer = 0;
  int first_rx = 0;
  RTIME T0;
  unsigned int rxs;

  openair0_timestamp timestamp;

#ifdef NAS_UE
  MessageDef *message_p;
#endif

#ifdef RTAI
  RT_TASK *task = rt_task_init_schmod(nam2num("UE thread"), 0, 0, 0, SCHED_FIFO, 0xF);

  if (task==NULL) {
    LOG_E(PHY,"[SCHED][UE] Problem starting UE thread!!!!\n");
    return 0;
  }

#else

#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;
  unsigned int flags = 0;

  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;//sched_get_priority_max(SCHED_DEADLINE);

  // This creates a .5 ms  reservation
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime  = 100000;
  attr.sched_deadline = 500000;
  attr.sched_period   = 500000;

  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] main eNB thread: sched_setattr failed\n");
    exit_fun("Nothing to add");
    return &UE_thread_retval;
  }
  LOG_I(HW,"[SCHED][eNB] eNB main deadline thread %lu started on CPU %d\n",
        (unsigned long)gettid(), sched_getcpu());

#else
  struct sched_param sp;
  sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
  pthread_setschedparam(pthread_self(),SCHED_FIFO,&sp);
#endif
#endif

  // Lock memory from swapping. This is a process wide call (not constraint to this thread).
  mlockall(MCL_CURRENT | MCL_FUTURE);

  printf("waiting for sync (UE_thread)\n");
  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_thread)\n");

  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);

  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex, waiting (UE_thread)\n");

  printf("starting UE thread\n");

#ifdef NAS_UE
  message_p = itti_alloc_new_message(TASK_NAS_UE, INITIALIZE_MESSAGE);
  itti_send_msg_to_task (TASK_NAS_UE, INSTANCE_DEFAULT, message_p);
#endif

  T0 = rt_get_time_ns();
  first_rx = 1;
  rxpos=0;

  while (!oai_exit) {
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_HW_SUBFRAME, hw_subframe );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_HW_FRAME, frame );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_DUMMY_DUMP, dummy_dump );


    while (rxpos < (1+hw_subframe)*UE->lte_frame_parms.samples_per_tti) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );

#ifndef USRP_DEBUG

      DevAssert( UE->lte_frame_parms.nb_antennas_rx <= 2 );
      void* rxp[2];

      for (int i=0; i<UE->lte_frame_parms.nb_antennas_rx; i++)
        rxp[i] = (dummy_dump==0) ? (void*)&rxdata[i][rxpos] : (void*)dummy[i];
      
      /*      if (dummy_dump == 0)
	      printf("writing %d samples to %d (first_rx %d)\n",spp - ((first_rx==1) ? rx_off_diff : 0),rxpos,first_rx);*/
      
      if (UE->mode != loop_through_memory) {
	rxs = openair0.trx_read_func(&openair0,
				     &timestamp,
				     rxp,
				     spp - ((first_rx==1) ? rx_off_diff : 0),
				     UE->lte_frame_parms.nb_antennas_rx);

	if (rxs != (spp- ((first_rx==1) ? rx_off_diff : 0))) {
	  printf("rx error: asked %d got %d ",spp - ((first_rx==1) ? rx_off_diff : 0),rxs);
	  if (UE->is_synchronized == 1) {
	    exit_fun("problem in rx");
	    return &UE_thread_retval;
	  }
	}
      }

      if (rx_off_diff !=0)
	LOG_D(PHY,"frame %d, rx_offset %d, rx_off_diff %d\n",UE->frame_rx,UE->rx_offset,rx_off_diff);

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );

      // Transmit TX buffer based on timestamp from RX
      if ((tx_enabled==1) && (UE->mode!=loop_through_memory)) {
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );

        DevAssert( UE->lte_frame_parms.nb_antennas_tx <= 2 );
        void* txp[2];

        for (int i=0; i<UE->lte_frame_parms.nb_antennas_tx; i++)
          txp[i] = (void*)&txdata[i][txpos];

        openair0.trx_write_func(&openair0,
                                (timestamp+openair0_cfg[0].tx_scheduling_advance-openair0_cfg[0].tx_sample_advance),
                                txp,
				spp - ((first_rx==1) ? rx_off_diff : 0),
                                UE->lte_frame_parms.nb_antennas_tx,
                                1);

        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );
      }
      else if (UE->mode == loop_through_memory)
	rt_sleep_ns(1000000);
#else
      // define USRP_DEBUG is active
      rt_sleep_ns(1000000);
#endif

      rx_off_diff = 0;
      first_rx = 0;

      rxpos += spp;
      txpos += spp;

      if (txpos >= 10*PHY_vars_UE_g[0][0]->lte_frame_parms.samples_per_tti)
        txpos -= 10*PHY_vars_UE_g[0][0]->lte_frame_parms.samples_per_tti;
    }

    if (rxpos >= 10*PHY_vars_UE_g[0][0]->lte_frame_parms.samples_per_tti)
      rxpos -= 10*PHY_vars_UE_g[0][0]->lte_frame_parms.samples_per_tti;

    if (UE->is_synchronized == 1)  {
      LOG_D( HW, "UE_thread: hw_frame %d, hw_subframe %d (time %lli)\n", frame, hw_subframe, rt_get_time_ns()-T0 );

      if (start_rx_stream == 1) {
	LOG_D(PHY,"Locking mutex_rx (IC %d)\n",UE->instance_cnt_rx);
        if (pthread_mutex_lock(&UE->mutex_rx) != 0) {
          LOG_E( PHY, "[SCHED][UE] error locking mutex for UE RX thread\n" );
          exit_fun("nothing to add");
          return &UE_thread_retval;
        }

        int instance_cnt_rx = ++UE->instance_cnt_rx;

	LOG_D(PHY,"Unlocking mutex_rx (IC %d)\n",instance_cnt_rx);
        if (pthread_mutex_unlock(&UE->mutex_rx) != 0) {
          LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE RX thread\n" );
          exit_fun("nothing to add");
          return &UE_thread_retval;
        }

	VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE_INST_CNT_RX, instance_cnt_rx);


        if (instance_cnt_rx == 0) {
	  LOG_D(HW,"signalling rx thread to wake up, hw_frame %d, hw_subframe %d (time %lli)\n", frame, hw_subframe, rt_get_time_ns()-T0 );
          if (pthread_cond_signal(&UE->cond_rx) != 0) {
            LOG_E( PHY, "[SCHED][UE] ERROR pthread_cond_signal for UE RX thread\n" );
            exit_fun("nothing to add");
            return &UE_thread_retval;
          }
	  
	  LOG_D(HW,"signalled rx thread to wake up, hw_frame %d, hw_subframe %d (time %lli)\n", frame, hw_subframe, rt_get_time_ns()-T0 );
	  if (UE->mode == loop_through_memory) {
	    printf("Processing subframe %d",UE->slot_rx>>1);
	    getchar();
	  }

          if (UE->mode == rx_calib_ue) {
            if (frame == 10) {
              LOG_D(PHY,
                    "[SCHED][UE] Found cell with N_RB_DL %"PRIu8", PHICH CONFIG (%d,%d), Nid_cell %"PRIu16", NB_ANTENNAS_TX %"PRIu8", frequency offset "PRIi32" Hz, RSSI (digital) %hu dB, measured Gain %d dB, total_rx_gain %"PRIu32" dB, USRP rx gain %f dB\n",
                    UE->lte_frame_parms.N_RB_DL,
                    UE->lte_frame_parms.phich_config_common.phich_duration,
                    UE->lte_frame_parms.phich_config_common.phich_resource,
                    UE->lte_frame_parms.Nid_cell,
                    UE->lte_frame_parms.nb_antennas_tx_eNB,
                    UE->lte_ue_common_vars.freq_offset,
                    UE->PHY_measurements.rx_power_avg_dB[0],
                    UE->PHY_measurements.rx_power_avg_dB[0] - rx_input_level_dBm,
                    UE->rx_total_gain_dB,
                    openair0_cfg[0].rx_gain[0]
                   );
              exit_fun("[HW][UE] UE in RX calibration mode, exiting");
              return &UE_thread_retval;
            }
          }
        } else {
          LOG_E( PHY, "[SCHED][UE] UE RX thread busy (IC %d)!!\n", instance_cnt_rx);
	  if (instance_cnt_rx > 2) {
	    exit_fun("instance_cnt_rx > 1");
	    return &UE_thread_retval;
	  }
        }

       
        if ((tx_enabled==1)&&(UE->mode != loop_through_memory)) {

	  if (pthread_mutex_lock(&UE->mutex_tx) != 0) {
	    LOG_E( PHY, "[SCHED][UE] error locking mutex for UE TX thread\n" );
	    exit_fun("nothing to add");
	    return &UE_thread_retval;
	  }


          int instance_cnt_tx = ++UE->instance_cnt_tx;

          if (pthread_mutex_unlock(&UE->mutex_tx) != 0) {
            LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE TX thread\n" );
            exit_fun("nothing to add");
            return &UE_thread_retval;
          }
	  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE_INST_CNT_TX, instance_cnt_tx);


          if (instance_cnt_tx == 0) {
            if (pthread_cond_signal(&UE->cond_tx) != 0) {
              LOG_E( PHY, "[SCHED][UE] ERROR pthread_cond_signal for UE TX thread\n" );
              exit_fun("nothing to add");
              return &UE_thread_retval;
            }
	    LOG_D(HW,"signalled tx thread to wake up, hw_frame %d, hw_subframe %d (time %lli)\n", frame, hw_subframe, rt_get_time_ns()-T0 );

          } else {
            LOG_E( PHY, "[SCHED][UE] UE TX thread busy (IC %d)!!\n" );
	    if (instance_cnt_tx>2) {
	      exit_fun("instance_cnt_tx > 1");
	      return &UE_thread_retval;
	    }
          }
        }

      }
    } else {
      // we are not yet synchronized
      if ((hw_subframe == 9) && (dummy_dump == 0)) {
        // Wake up initial synch thread
        if (pthread_mutex_lock(&UE->mutex_synch) != 0) {
          LOG_E( PHY, "[SCHED][UE] error locking mutex for UE initial synch thread\n" );
          exit_fun("nothing to add");
          return &UE_thread_retval;
        }

        int instance_cnt_synch = ++UE->instance_cnt_synch;

        if (pthread_mutex_unlock(&UE->mutex_synch) != 0) {
          LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE initial synch thread\n" );
          exit_fun("nothing to add");
          return &UE_thread_retval;
        }

        dummy_dump = 1;

        if (instance_cnt_synch == 0) {
          if (pthread_cond_signal(&UE->cond_synch) != 0) {
            LOG_E( PHY, "[SCHED][UE] ERROR pthread_cond_signal for UE sync thread\n" );
            exit_fun("nothing to add");
            return &UE_thread_retval;
          }
        } else {
          LOG_E( PHY, "[SCHED][UE] UE sync thread busy!!\n" );
          exit_fun("nothing to add");
          return &UE_thread_retval;
        }
      }
    }

    hw_subframe++;
    slot+=2;

    if (hw_subframe==10) {
      hw_subframe = 0;
      first_rx = 1;
      frame++;
      slot = 1;

      int fail = pthread_mutex_lock(&UE->mutex_synch);
      int instance_cnt_synch = UE->instance_cnt_synch;
      fail = fail || pthread_mutex_unlock(&UE->mutex_synch);

      if (fail) {
        LOG_E( PHY, "[SCHED][UE] error (un-)locking mutex for UE synch\n" );
        exit_fun("noting to add");
        return &UE_thread_retval;
      }

      if (instance_cnt_synch < 0) {
        // the UE_thread_synch is ready
        if (UE->is_synchronized == 1) {
          rx_off_diff = 0;
          LTE_DL_FRAME_PARMS *frame_parms = &UE->lte_frame_parms; // for macro FRAME_LENGTH_COMPLEX_SAMPLES

	  //	  LOG_I(PHY,"UE->rx_offset %d\n",UE->rx_offset);
          if ((UE->rx_offset > RX_OFF_MAX) && (start_rx_stream == 0)) {
            start_rx_stream=1;
            frame=0;
            // dump ahead in time to start of frame

#ifndef USRP_DEBUG
	    if (UE->mode != loop_through_memory) {
	      LOG_I(PHY,"Resynchronizing RX by %d samples\n",UE->rx_offset);
	      rxs = openair0.trx_read_func(&openair0,
					   &timestamp,
					   (void**)rxdata,
					   UE->rx_offset,
					   UE->lte_frame_parms.nb_antennas_rx);
	      if (rxs != UE->rx_offset) {
		exit_fun("problem in rx");
		return &UE_thread_retval;
	      }
	      UE->rx_offset=0;
	      tx_enabled = 1;
	    }
	    else
	      rt_sleep_ns(1000000);
#else
            rt_sleep_ns(10000000);
#endif

          } else if ((UE->rx_offset<(FRAME_LENGTH_COMPLEX_SAMPLES/2)) &&
		     (UE->rx_offset > RX_OFF_MIN) && 
		     (start_rx_stream==1) && 
		     (rx_correction_timer == 0)) {
            rx_off_diff = -UE->rx_offset + RX_OFF_MIN;
	    LOG_D(PHY,"UE->rx_offset %d > %d, diff %d\n",UE->rx_offset,RX_OFF_MIN,rx_off_diff);
            rx_correction_timer = 5;
          } else if ((UE->rx_offset>(FRAME_LENGTH_COMPLEX_SAMPLES/2)) && 
		     (UE->rx_offset < (FRAME_LENGTH_COMPLEX_SAMPLES-RX_OFF_MIN)) &&
		     (start_rx_stream==1) && 
		     (rx_correction_timer == 0)) {   // moving to the left so drop rx_off_diff samples
            rx_off_diff = FRAME_LENGTH_COMPLEX_SAMPLES - RX_OFF_MIN - UE->rx_offset;
	    LOG_D(PHY,"UE->rx_offset %d < %d, diff %d\n",UE->rx_offset,FRAME_LENGTH_COMPLEX_SAMPLES-RX_OFF_MIN,rx_off_diff);

            rx_correction_timer = 5;
          }

          if (rx_correction_timer>0)
            rx_correction_timer--;
        }

        dummy_dump=0;
      }
    }

#if defined(ENABLE_ITTI)
    itti_update_lte_time(frame, slot);
#endif
  }

  return &UE_thread_retval;
}
#endif



#ifdef EXMIMO
/* This is the main UE thread. Initially it is doing a periodic get_frame. One synchronized it gets woken up by the kernel driver using the RTAI message mechanism (rt_send and rt_receive). */
void *UE_thread(void *arg)
{
  PHY_VARS_UE *UE=PHY_vars_UE_g[0][0];
#ifdef RTAI
  RT_TASK *task;
#endif
  // RTIME in, out, diff;
  int slot=0,frame=0,hw_slot;
  // unsigned int aa;
  int delay_cnt;
  RTIME time_in;
  int /* hw_slot_offset=0, */ rx_offset_mbox=0,mbox_target=0,mbox_current=0;
  int diff2;
  int /* i, */ ret;
  int /* CC_id, */ card;
  volatile unsigned int *DAQ_MBOX = openair0_daq_cnt();

  int wait_sync_cnt = 0;
  int first_synch = 1;
#ifdef DEADLINE_SCHEDULER
  struct sched_attr attr;
  unsigned int flags = 0;
  //  unsigned long mask = 1; // processor 0
#endif
  int freq_offset;


#ifdef RTAI
  task = rt_task_init_schmod(nam2num("UE thread"), 0, 0, 0, SCHED_FIFO, 0xF);

  if (task==NULL) {
    LOG_E(PHY,"[SCHED][UE] Problem starting UE thread!!!!\n");
    return 0;
  }

#endif


#ifdef HARD_RT
  rt_make_hard_real_time();
#endif


#ifdef DEADLINE_SCHEDULER
  attr.size = sizeof(attr);
  attr.sched_flags = 0;
  attr.sched_nice = 0;
  attr.sched_priority = 0;

  // This creates a .25 ms  reservation
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime  = (0.1  * 100) * 10000;
  attr.sched_deadline = (0.25 * 100) * 10000;
  attr.sched_period   = (0.5  * 100) * 10000;

  // pin the UE main thread to CPU0
  // if (pthread_setaffinity_np(pthread_self(), sizeof(mask),&mask) <0) {
  //   perror("[MAIN_ENB_THREAD] pthread_setaffinity_np failed\n");
  //   }

  if (sched_setattr(0, &attr, flags) < 0 ) {
    perror("[SCHED] main UE thread: sched_setattr failed\n");
    exit_fun("Nothing to add");
  } else {
    LOG_I(HW,"[SCHED][eNB] eNB main deadline thread %ld started on CPU %d\n",
          gettid(),sched_getcpu());
  }

#endif


  mlockall(MCL_CURRENT | MCL_FUTURE);

  printf("waiting for sync (UE_thread)\n");

  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_thread)\n");

  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);

  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex, waiting (UE_thread)\n");

  printf("starting UE thread\n");

  freq_offset = 0; //-7500;

  first_synch = 1;

  while (!oai_exit)  {

    hw_slot = (((((volatile unsigned int *)DAQ_MBOX)[0]+1)%150)<<1)/15; //the slot the hw is about to store


    if (UE->is_synchronized) {

      if (first_synch == 1) {
        first_synch = 0;

        for (card=0; card<openair0_num_detected_cards; card++)
          openair0_start_rt_acquisition(card);

        rt_sleep_ns(FRAME_PERIOD/10);
      }

      //this is the mbox counter that indicates the start of the frame
      rx_offset_mbox = (UE->rx_offset * 150) / (10*UE->lte_frame_parms.samples_per_tti);
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE_RX_OFFSET, UE->rx_offset);
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE_OFFSET_MBOX, rx_offset_mbox);
      //this is the mbox counter where we should be
      mbox_target = (((((slot+1)%20)*15+1)>>1) + rx_offset_mbox + 1)%150;
      // round up to the next multiple of two (mbox counter from express MIMO gives only even numbers)
      mbox_target = ((mbox_target+1)-((mbox_target-1)%2))%150;
      //this is the mbox counter where we are
      mbox_current = ((volatile unsigned int *)DAQ_MBOX)[0];

      //this is the time we need to sleep in order to synchronize with the hw (in multiples of DAQ_PERIOD)
      if ((mbox_current>=120) && (mbox_target<30)) //handle the frame wrap-arround
        diff2 = 150-mbox_current+mbox_target;
      else if ((mbox_current<30) && (mbox_target>=120))
        diff2 = -150+mbox_target-mbox_current;
      else
        diff2 = mbox_target - mbox_current;

      if (diff2 <(-7)) {
        LOG_D(HW,"UE Frame %d: missed slot, proceeding with next one (slot %d, hw_slot %d, diff %d)\n",frame, slot, hw_slot, diff2);

        if (frame>0) {
          if (exit_missed_slots==1)
            exit_fun("[HW][UE] missed slot");
          else {
            num_missed_slots++;
            LOG_W(HW,"[UE] just missed slot (total missed slots %ld)\n", num_missed_slots);
          }
        }

        slot++;

        if (slot==20) {
          slot=0;
          frame++;
        }

        // update thread slot/frame counters because of skipped slot
        UE->slot_rx++;
        UE->slot_tx++;

        if (UE->slot_rx == 20) {
          UE->slot_rx = 0;
          UE->frame_rx++;
        }

        if (UE->slot_tx == 20) {
          UE->slot_tx = 0;
          UE->frame_tx++;
        }

        continue;
      }

      if (diff2>8)
        LOG_D(HW,"UE Frame %d: skipped slot, waiting for hw to catch up (slot %d, hw_slot %d, mbox_current %d, mbox_target %d, diff %d)\n",frame, slot, hw_slot, mbox_current, mbox_target, diff2);


      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *DAQ_MBOX);
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DIFF, diff2);



      // This loop implements the delay of 1 slot to allow for processing
      delay_cnt = 0;

      while ((diff2>0) && (!oai_exit) )  {
        time_in = rt_get_time_ns();
        //LOG_D(HW,"eNB Frame %d delaycnt %d : hw_slot %d (%d), slot %d (%d), diff %d, time %llu\n",frame,delay_cnt,hw_slot,((volatile unsigned int *)DAQ_MBOX)[0],slot,mbox_target,diff2,time_in);
        VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *DAQ_MBOX);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RT_SLEEP,1);
        ret = rt_sleep_ns(diff2*DAQ_PERIOD);
        VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_RT_SLEEP,0);
        VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *DAQ_MBOX);

        if (ret)
          LOG_D(HW,"eNB Frame %d, time %llu: rt_sleep_ns returned %d\n",frame, time_in);

        hw_slot = (((((volatile unsigned int *)DAQ_MBOX)[0]+1)%150)<<1)/15;
        //LOG_D(HW,"eNB Frame %d : hw_slot %d, time %llu\n",frame,hw_slot,rt_get_time_ns());
        delay_cnt++;

        if (delay_cnt == 30)  {
          LOG_D(HW,"UE frame %d: HW stopped ... \n",frame);
          exit_fun("[HW][UE] HW stopped");
        }

        mbox_current = ((volatile unsigned int *)DAQ_MBOX)[0];

        if ((mbox_current>=135) && (mbox_target<15)) //handle the frame wrap-arround
          diff2 = 150-mbox_current+mbox_target;
        else if ((mbox_current<15) && (mbox_target>=135))
          diff2 = -150+mbox_target-mbox_current;
        else
          diff2 = mbox_target - mbox_current;

        VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DAQ_MBOX, *DAQ_MBOX);
        VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DIFF, diff2);
      }

      // on even slots, schedule processing of entire subframe
      if ((slot&1) == 0)  {

        if (pthread_mutex_lock(&UE->mutex_rx) != 0) {
          LOG_E(PHY,"[SCHED][UE] error locking mutex for UE RX thread\n");
          exit_fun("nothing to add");
        } else {

	  int instance_cnt_rx = ++UE->instance_cnt_rx;

          pthread_mutex_unlock(&UE->mutex_rx);
	  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE_INST_CNT_RX, instance_cnt_rx);


          if (instance_cnt_rx == 0) {
            LOG_D(HW,"Scheduling UE RX for frame %d (hw frame %d), subframe %d (%d), mode %d\n",UE->frame_rx,frame,slot>>1,UE->slot_rx>>1,UE->mode);

            if (pthread_cond_signal(&UE->cond_rx) != 0) {
              LOG_E(PHY,"[SCHED][UE] ERROR pthread_cond_signal for UE RX thread\n");
              exit_fun("nothing to add");
            } else {
              //        printf("UE_thread: cond_signal for RX ok (%p) @ %llu\n",(void*)&UE->cond_rx,rt_get_time_ns()-T0);
            }

            if (UE->mode == rx_calib_ue) {
              if (frame == 10) {
                LOG_D(PHY,
                      "[SCHED][UE] Found cell with N_RB_DL %d, PHICH CONFIG (%d,%d), Nid_cell %d, NB_ANTENNAS_TX %d, initial frequency offset %d Hz, frequency offset %d Hz, RSSI (digital) %d dB, measured Gain %d dB, total_rx_gain %d dB, USRP rx gain %f dB\n",
                      UE->lte_frame_parms.N_RB_DL,
                      UE->lte_frame_parms.phich_config_common.phich_duration,
                      UE->lte_frame_parms.phich_config_common.phich_resource,
                      UE->lte_frame_parms.Nid_cell,
                      UE->lte_frame_parms.nb_antennas_tx_eNB,
                      freq_offset,
                      UE->lte_ue_common_vars.freq_offset,
                      UE->PHY_measurements.rx_power_avg_dB[0],
                      UE->PHY_measurements.rx_power_avg_dB[0] - rx_input_level_dBm,
                      UE->rx_total_gain_dB,
                      openair0_cfg[0].rx_gain[0]
                     );
                exit_fun("[HW][UE] UE in RX calibration mode, exiting");
              }
            }
          } else {
            LOG_E(PHY,"[SCHED][UE] UE RX thread busy!!\n");
            exit_fun("nothing to add");
          }
        }

        if (pthread_mutex_lock(&UE->mutex_tx) != 0) {
          LOG_E(PHY,"[SCHED][UE] error locking mutex for UE TX thread\n");
          exit_fun("nothing to add");
        } else {

          int instance_cnt_tx = ++UE->instance_cnt_tx;

          pthread_mutex_unlock(&UE->mutex_tx);
	  VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE_INST_CNT_TX, instance_cnt_tx);

          if (instance_cnt_tx == 0) {
            LOG_D(HW,"Scheduling UE TX for frame %d (hw frame %d), subframe %d (%d), mode %d\n",UE->frame_tx,frame,slot>>1,UE->slot_tx>>1,UE->mode);

            if (pthread_cond_signal(&UE->cond_tx) != 0) {
              LOG_E(PHY,"[SCHED][UE] ERROR pthread_cond_signal for UE TX thread\n");
              exit_fun("nothing to add");
            } else {
              //        printf("UE_thread: cond_signal for RX ok (%p) @ %llu\n",(void*)&UE->cond_rx,rt_get_time_ns()-T0);
            }
          } else {
            LOG_E(PHY,"[SCHED][UE] UE TX thread busy!!\n");
            exit_fun("nothing to add");
          }
        }
      }

      /*
      if ((slot%2000)<10)
      LOG_D(HW,"fun0: doing very hard work\n");
      */
      // now increment slot and frame counters
      slot++;

      if (slot==20) {
        slot=0;
        frame++;
      }
    } else if (UE->is_synchronized == 0) { // we are not yet synchronized
      //hw_slot_offset = 0;
      first_synch = 1;
      slot = 0;


      // wait until we can lock mutex_synch
      //printf("Locking mutex_synch (UE_thread)\n");
      if (pthread_mutex_lock(&UE->mutex_synch) != 0) {
        LOG_E(PHY,"[SCHED][UE] error locking mutex for UE initial synch thread\n");
        exit_fun("noting to add");
      } else {

        if (UE->instance_cnt_synch < 0) {

          wait_sync_cnt=0;
	  openair0_config(&openair0_cfg[0],1);
      //  openair0_set_gains(&openair0,&openair0_cfg[0]);

	  printf("Getting frame\n");
          openair0_get_frame(0);
          rt_sleep_ns(FRAME_PERIOD);
          // increment instance count for sync thread
          UE->instance_cnt_synch++;
          pthread_mutex_unlock(&UE->mutex_synch);

          if (pthread_cond_signal(&UE->cond_synch) != 0) {
            LOG_E(PHY,"[SCHED][UE] ERROR pthread_cond_signal for UE sync thread\n");
            exit_fun("nothing to add");
          }
        } else {
          wait_sync_cnt++;
          pthread_mutex_unlock(&UE->mutex_synch);

          if (wait_sync_cnt>1000)
            exit_fun("waiting to long for synch thread");
          else
            rt_sleep_ns(FRAME_PERIOD);
        }
      }


      /*
        if (initial_sync(UE,mode)==0) {

        if (mode == rx_calib_ue) {
        exit_fun("[HW][UE] UE in RX calibration mode");
        }
        else {
        is_synchronized = 1;
        //start the streaming DMA transfers
        for (card=0;card<openair0_num_detected_cards;card++)
        openair0_start_rt_acquisition(card);

        hw_slot_offset = (UE->rx_offset<<1) / UE->lte_frame_parms.samples_per_tti;
        }
        }
        else {
        if (freq_offset >= 0) {
        freq_offset += 100;
        freq_offset *= -1;
        }
        else {
        freq_offset *= -1;
        }
        if (abs(freq_offset) > 7500) {
        LOG_I(PHY,"[initial_sync] No cell synchronization found, abondoning\n");
        mac_xface->macphy_exit("No cell synchronization found, abondoning");
        }
        else {
        //    LOG_I(PHY,"[initial_sync] trying carrier off %d Hz\n",freq_offset);
        #ifndef USRP
        for (CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
        for (i=0; i<openair0_cfg[rf_map[CC_id].card].rx_num_channels; i++)
        openair0_cfg[rf_map[CC_id].card].rx_freq[rf_map[CC_id].chain+i] = downlink_frequency[CC_id][i]+freq_offset;
        for (i=0; i<openair0_cfg[rf_map[CC_id].card].tx_num_channels; i++)
        openair0_cfg[rf_map[CC_id].card].tx_freq[rf_map[CC_id].chain+i] = downlink_frequency[CC_id][i]+freq_offset;
        }
        openair0_config(&openair0_cfg[0],UE_flag);
        #endif
        rt_sleep_ns(FRAME_PERIOD);
        }
        }
      */
    }
  }

  LOG_D(HW,"UE_thread: finished, ran %d times.\n",frame);

#ifdef HARD_RT
  rt_make_soft_real_time();
#endif

  // clean task
#ifdef RTAI
  rt_task_delete(task);
#endif
  LOG_D(HW,"Task deleted. returning\n");
  return 0;
}

#else  // This is for USRP or ETHERNET targets

#endif


/*!
 * \brief Initialize the UE theads.
 * Creates the UE threads:
 * - UE_thread_tx
 * - UE_thread_rx
 * - UE_thread_synch
 * and the locking between them.
 */
void init_UE_threads(void)
{
  PHY_VARS_UE *UE = PHY_vars_UE_g[0][0];

  // the threads are not yet active, therefore access is allowed without locking
  UE->instance_cnt_tx = -1;
  UE->instance_cnt_rx = -1;
  UE->instance_cnt_synch = -1;
  pthread_mutex_init(&UE->mutex_tx,NULL);
  pthread_mutex_init(&UE->mutex_rx,NULL);
  pthread_mutex_init(&UE->mutex_synch,NULL);
  pthread_cond_init(&UE->cond_tx,NULL);
  pthread_cond_init(&UE->cond_rx,NULL);
  pthread_cond_init(&UE->cond_synch,NULL);
  pthread_create(&UE->thread_tx,NULL,UE_thread_tx,(void*)UE);
  pthread_setname_np( UE->thread_tx, "UE_thread_tx" );
  pthread_create(&UE->thread_rx,NULL,UE_thread_rx,(void*)UE);
  pthread_setname_np( UE->thread_rx, "UE_thread_rx" );
  pthread_create(&UE->thread_synch,NULL,UE_thread_synch,(void*)UE);
  pthread_setname_np( UE->thread_synch, "UE_thread_synch" );
  UE->frame_tx = 0;
  UE->frame_rx = 0;
}


#ifdef OPENAIR2
void fill_ue_band_info(void)
{

  UE_EUTRA_Capability_t *UE_EUTRA_Capability = UE_rrc_inst[0].UECap->UE_EUTRA_Capability;
  int i,j;

  bands_to_scan.nbands = UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list.count;

  for (i=0; i<bands_to_scan.nbands; i++) {

    for (j=0; j<sizeof (eutra_bands) / sizeof (eutra_bands[0]); j++)
      if (eutra_bands[j].band == UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list.array[i]->bandEUTRA) {
        memcpy(&bands_to_scan.band_info[i],
               &eutra_bands[j],
               sizeof(eutra_band_t));

        printf("Band %d (%lu) : DL %u..%u Hz, UL %u..%u Hz, Duplex %s \n",
               bands_to_scan.band_info[i].band,
               UE_EUTRA_Capability->rf_Parameters.supportedBandListEUTRA.list.array[i]->bandEUTRA,
               bands_to_scan.band_info[i].dl_min,
               bands_to_scan.band_info[i].dl_max,
               bands_to_scan.band_info[i].ul_min,
               bands_to_scan.band_info[i].ul_max,
               (bands_to_scan.band_info[i].frame_type==FDD) ? "FDD" : "TDD");
        break;
      }
  }
}
#endif

int setup_ue_buffers(PHY_VARS_UE **phy_vars_ue, openair0_config_t *openair0_cfg, openair0_rf_map rf_map[MAX_NUM_CCs])
{

  //#ifndef EXMIMO
  //  uint16_t N_TA_offset = 0;
  //#endif

  int i, CC_id;
  LTE_DL_FRAME_PARMS *frame_parms;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    if (phy_vars_ue[CC_id]) {
      frame_parms = &(phy_vars_ue[CC_id]->lte_frame_parms);
    } else {
      printf("phy_vars_UE[%d] not initialized\n", CC_id);
      return(-1);
    }


    //#ifndef EXMIMO
    //    if (frame_parms->frame_type == TDD) {
    //      if (frame_parms->N_RB_DL == 100)
    //  N_TA_offset = 624;
    //      else if (frame_parms->N_RB_DL == 50)
    //  N_TA_offset = 624/2;
    //      else if (frame_parms->N_RB_DL == 25)
    //  N_TA_offset = 624/4;
    //    }
    //#endif

#ifdef EXMIMO
    openair0_cfg[CC_id].tx_num_channels = 0;
    openair0_cfg[CC_id].rx_num_channels = 0;

    // replace RX signal buffers with mmaped HW versions
    for (i=0; i<frame_parms->nb_antennas_rx; i++) {
      printf("Mapping UE CC_id %d, rx_ant %d, freq %u on card %d, chain %d\n",CC_id,i,downlink_frequency[CC_id][i],rf_map[CC_id].card,rf_map[CC_id].chain+i);
      free(phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[i]);
      phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[i] = (int32_t*) openair0_exmimo_pci[rf_map[CC_id].card].adc_head[rf_map[CC_id].chain+i];

      if (openair0_cfg[rf_map[CC_id].card].rx_freq[rf_map[CC_id].chain+i]) {
        printf("Error with rf_map! A channel has already been allocated!\n");
        return(-1);
      } else {
        openair0_cfg[rf_map[CC_id].card].rx_freq[rf_map[CC_id].chain+i] = downlink_frequency[CC_id][i];
        openair0_cfg[rf_map[CC_id].card].rx_gain[rf_map[CC_id].chain+i] = rx_gain[CC_id][i];
        openair0_cfg[rf_map[CC_id].card].rxg_mode[rf_map[CC_id].chain+i] = rx_gain_mode[CC_id][i];
        openair0_cfg[rf_map[CC_id].card].rx_num_channels++;
      }

      printf("rxdata[%d] @ %p\n",i,phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[i]);
    }

    for (i=0; i<frame_parms->nb_antennas_tx; i++) {
      printf("Mapping UE CC_id %d, tx_ant %d, freq %u on card %d, chain %d\n",CC_id,i,downlink_frequency[CC_id][i],rf_map[CC_id].card,rf_map[CC_id].chain+i);
      free(phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[i]);
      phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[i] = (int32_t*) openair0_exmimo_pci[rf_map[CC_id].card].dac_head[rf_map[CC_id].chain+i];

      if (openair0_cfg[rf_map[CC_id].card].tx_freq[rf_map[CC_id].chain+i]) {
        printf("Error with rf_map! A channel has already been allocated!\n");
        return(-1);
      } else {
        openair0_cfg[rf_map[CC_id].card].tx_freq[rf_map[CC_id].chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
        openair0_cfg[rf_map[CC_id].card].tx_gain[rf_map[CC_id].chain+i] = tx_gain[CC_id][i];
        openair0_cfg[rf_map[CC_id].card].tx_num_channels++;
      }

      printf("txdata[%d] @ %p\n",i,phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[i]);
    }

#else
    // replace RX signal buffers with mmaped HW versions
    rxdata = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
    txdata = (int32_t**)malloc16( frame_parms->nb_antennas_tx*sizeof(int32_t*) );

    for (i=0; i<frame_parms->nb_antennas_rx; i++) {
      printf( "Mapping UE CC_id %d, rx_ant %d, freq %u on card %d, chain %d\n", CC_id, i, downlink_frequency[CC_id][i], rf_map[CC_id].card, rf_map[CC_id].chain+i );
      free( phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[i] );
      rxdata[i] = (int32_t*)malloc16_clear( 307200*sizeof(int32_t) );
      phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[i] = rxdata[i]; // what about the "-N_TA_offset" ? // N_TA offset for TDD
    }

    for (i=0; i<frame_parms->nb_antennas_tx; i++) {
      printf( "Mapping UE CC_id %d, tx_ant %d, freq %u on card %d, chain %d\n", CC_id, i, downlink_frequency[CC_id][i], rf_map[CC_id].card, rf_map[CC_id].chain+i );
      free( phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[i] );
      txdata[i] = (int32_t*)malloc16_clear( 307200*sizeof(int32_t) );
      phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[i] = txdata[i];
    }

    // rxdata[x] points now to the same memory region as phy_vars_ue[CC_id]->lte_ue_common_vars.rxdata[x]
    // txdata[x] points now to the same memory region as phy_vars_ue[CC_id]->lte_ue_common_vars.txdata[x]
    // be careful when releasing memory!
    // because no "release_ue_buffers"-function is available, at least rxdata and txdata memory will leak (only some bytes)
#endif

  }

  return 0;
}
