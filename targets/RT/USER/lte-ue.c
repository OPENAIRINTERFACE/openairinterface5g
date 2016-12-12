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

#include "../../ARCH/COMMON/common_lib.h"

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

void init_UE_threads(int nb_inst);
void *UE_thread(void *arg);
void init_UE(int nb_inst);

extern pthread_cond_t sync_cond;
extern pthread_mutex_t sync_mutex;
extern int sync_var;


extern openair0_config_t openair0_cfg[MAX_CARDS];
extern uint32_t          downlink_frequency[MAX_NUM_CCs][4];
extern int32_t           uplink_frequency_offset[MAX_NUM_CCs][4];
extern int oai_exit;

int32_t **rxdata;
int32_t **txdata;

//extern unsigned int tx_forward_nsamps;
//extern int tx_delay;

extern int rx_input_level_dBm;
extern uint8_t exit_missed_slots;
extern uint64_t num_missed_slots; // counter for the number of missed slots

extern void exit_fun(const char* s);

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

pthread_t                       main_ue_thread;
pthread_attr_t                  attr_UE_thread;
struct sched_param              sched_param_UE_thread;

void init_UE(int nb_inst) {

  int error_code;
  int inst;
  PHY_VARS_UE *UE;
  int ret;

  for (inst=0;inst<nb_inst;inst++) {
    printf("Intializing UE Threads for instance %d ...\n",inst);
    init_UE_threads(inst);
    sleep(1);
    UE = PHY_vars_UE_g[inst][0];

    ret = openair0_device_load(&(UE->rfdevice), &openair0_cfg[0]);
    if (ret !=0){
       exit_fun("Error loading device library");
    }
    UE->rfdevice.host_type = BBU_HOST;
    //    UE->rfdevice.type      = NONE_DEV;
    error_code = pthread_create(&UE->proc.pthread_ue, &UE->proc.attr_ue, UE_thread, NULL);
    
    if (error_code!= 0) {
      LOG_D(HW,"[lte-softmodem.c] Could not allocate UE_thread, error %d\n",error_code);
      return;
    } else {
      LOG_D(HW, "[lte-softmodem.c] Allocate UE_thread successful\n" );
      pthread_setname_np( UE->proc.pthread_ue, "main UE" );
    }
  }

  printf("UE threads created\n");
#ifdef USE_MME
  
  while (start_UE == 0) {
    sleep(1);
  }
  
#endif
  
}

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
  int CC_id = UE->CC_id;
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




  printf("starting UE synch thread (IC %d)\n",UE->proc.instance_cnt_synch);
  ind = 0;
  found = 0;


  if (UE->UE_scan == 0) {
    do  {
      current_band = eutra_bands[ind].band;
      printf( "Scanning band %d, dl_min %"PRIu32", ul_min %"PRIu32"\n", current_band, eutra_bands[ind].dl_min,eutra_bands[ind].ul_min);

      if ((eutra_bands[ind].dl_min <= downlink_frequency[0][0]) && (eutra_bands[ind].dl_max >= downlink_frequency[0][0])) {
	for (i=0; i<4; i++)
	  uplink_frequency_offset[CC_id][i] = eutra_bands[ind].ul_min - eutra_bands[ind].dl_min;

        found = 1;
        break;
      }

      ind++;
    } while (ind < sizeof(eutra_bands) / sizeof(eutra_bands[0]));
  
    if (found == 0) {
      exit_fun("Can't find EUTRA band for frequency");
      return &UE_thread_synch_retval;
    }






    LOG_I( PHY, "[SCHED][UE] Check absolute frequency DL %"PRIu32", UL %"PRIu32" (oai_exit %d, rx_num_channels %d)\n", downlink_frequency[0][0], downlink_frequency[0][0]+uplink_frequency_offset[0][0],oai_exit, openair0_cfg[0].rx_num_channels);

    for (i=0;i<openair0_cfg[UE->rf_map.card].rx_num_channels;i++) {
      openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
      openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
      openair0_cfg[UE->rf_map.card].autocal[UE->rf_map.chain+i] = 1;
      if (uplink_frequency_offset[CC_id][i] != 0) // 
	openair0_cfg[UE->rf_map.card].duplex_mode = duplex_mode_FDD;
      else //FDD
	openair0_cfg[UE->rf_map.card].duplex_mode = duplex_mode_TDD;
    }

    sync_mode = pbch;

  } else if  (UE->UE_scan == 1) {
    current_band=0;

    for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
      downlink_frequency[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[CC_id].dl_min;
      uplink_frequency_offset[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[CC_id].ul_min-bands_to_scan.band_info[CC_id].dl_min;
      
      openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
      openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
      openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;
    }
  }


  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_sync_thread)\n");

  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);

  pthread_mutex_unlock(&sync_mutex);
  printf("Started device, unlocked sync_mutex (UE_sync_thread)\n");

  if (UE->rfdevice.trx_start_func(&UE->rfdevice) != 0 ) { 
    LOG_E(HW,"Could not start the device\n");
    oai_exit=1;
  }

  while (oai_exit==0) {

    if (pthread_mutex_lock(&UE->proc.mutex_synch) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE initial synch thread\n" );
      exit_fun("noting to add");
      return &UE_thread_synch_retval;
    }
    

    while (UE->proc.instance_cnt_synch < 0) {
      // the thread waits here most of the time
      pthread_cond_wait( &UE->proc.cond_synch, &UE->proc.mutex_synch );
    }

    if (pthread_mutex_unlock(&UE->proc.mutex_synch) != 0) {
      LOG_E( PHY, "[SCHED][eNB] error unlocking mutex for UE Initial Synch thread\n" );
      exit_fun("nothing to add");
      return &UE_thread_synch_retval;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_THREAD_SYNCH, 1 );

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

      for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
	downlink_frequency[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[current_band].dl_min+current_offset;
	uplink_frequency_offset[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[current_band].ul_min-bands_to_scan.band_info[0].dl_min + current_offset;

	openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
	openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
	openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;
	if (UE->UE_scan_carrier) {
	  openair0_cfg[UE->rf_map.card].autocal[UE->rf_map.chain+i] = 1;
	}
	
      }

      break;
 
    case pbch:

      LOG_I(PHY,"[UE thread Synch] Running Initial Synch (mode %d)\n",UE->mode);
      if (initial_sync( UE, UE->mode ) == 0) {

        hw_slot_offset = (UE->rx_offset<<1) / UE->frame_parms.samples_per_tti;
        LOG_I( HW, "Got synch: hw_slot_offset %d\n", hw_slot_offset );
	if (UE->UE_scan_carrier == 1) {

	  UE->UE_scan_carrier = 0;
	  // rerun with new cell parameters and frequency-offset
	  for (i=0;i<openair0_cfg[UE->rf_map.card].rx_num_channels;i++) {
	    openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;
	    openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] -= UE->common_vars.freq_offset;
	    openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] =  openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i]+uplink_frequency_offset[CC_id][i];
	    downlink_frequency[CC_id][i] = openair0_cfg[CC_id].rx_freq[i];
	    freq_offset=0;	    
	  }

	  // reconfigure for potentially different bandwidth
	  switch(UE->frame_parms.N_RB_DL) {
	  case 6:
	    openair0_cfg[UE->rf_map.card].sample_rate =1.92e6;
	    openair0_cfg[UE->rf_map.card].rx_bw          =.96e6;
	    openair0_cfg[UE->rf_map.card].tx_bw          =.96e6;
	    //            openair0_cfg[0].rx_gain[0] -= 12;
	    break;
	  case 25:
	    openair0_cfg[UE->rf_map.card].sample_rate =7.68e6;
	    openair0_cfg[UE->rf_map.card].rx_bw          =2.5e6;
	    openair0_cfg[UE->rf_map.card].tx_bw          =2.5e6;
	    //            openair0_cfg[0].rx_gain[0] -= 6;
	    break;
	  case 50:
	    openair0_cfg[UE->rf_map.card].sample_rate =15.36e6;
	    openair0_cfg[UE->rf_map.card].rx_bw          =5.0e6;
	    openair0_cfg[UE->rf_map.card].tx_bw          =5.0e6;
	    //            openair0_cfg[0].rx_gain[0] -= 3;
	    break;
	  case 100:
	    openair0_cfg[UE->rf_map.card].sample_rate=30.72e6;
	    openair0_cfg[UE->rf_map.card].rx_bw=10.0e6;
	    openair0_cfg[UE->rf_map.card].tx_bw=10.0e6;
	    //            openair0_cfg[0].rx_gain[0] -= 0;
	    break;
	  }
	
	  UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0],0);
	  //UE->rfdevice.trx_set_gains_func(&openair0,&openair0_cfg[0]);
	  UE->rfdevice.trx_stop_func(&UE->rfdevice);	  
	  sleep(1);
	  init_frame_parms(&UE->frame_parms,1);
	  if (UE->rfdevice.trx_start_func(&UE->rfdevice) != 0 ) { 
	    LOG_E(HW,"Could not start the device\n");
	    oai_exit=1;
	  }
	}
	else {
	  UE->is_synchronized = 1;

	  if( UE->mode == rx_dump_frame ){
	    FILE *fd;
	    if ((UE->proc.proc_rxtx[0].frame_rx&1) == 0) {  // this guarantees SIB1 is present 
	      if ((fd = fopen("rxsig_frame0.dat","w")) != NULL) {
		fwrite((void*)&UE->common_vars.rxdata[0][0],
		       sizeof(int32_t),
		       10*UE->frame_parms.samples_per_tti,
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
	      fwrite((void*)&UE->common_vars.rxdata[0][0],
		     sizeof(int32_t),
		     10*UE->frame_parms.samples_per_tti,
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

	for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
	  openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+freq_offset;
	  openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i]+freq_offset;
	  
	  openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;
	  
	  if (UE->UE_scan_carrier==1) {
	    openair0_cfg[UE->rf_map.card].autocal[UE->rf_map.chain+i] = 1;
	  }
	}

	UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0],0);
	    
      }// initial_sync=0

      break;

    case si:
    default:
      break;
    }


    if (pthread_mutex_lock(&UE->proc.mutex_synch) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE synch\n" );
      exit_fun("noting to add");
      return &UE_thread_synch_retval;
    }

    // indicate readiness
    UE->proc.instance_cnt_synch--;

    if (pthread_mutex_unlock(&UE->proc.mutex_synch) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE synch\n" );
      exit_fun("noting to add");
      return &UE_thread_synch_retval;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_THREAD_SYNCH, 0 );
  }  // while !oai_exit

  return &UE_thread_synch_retval;
}



/*!
 * \brief This is the UE thread for RX subframe n and TX subframe n+4.
 * This thread performs the phy_procedures_UE_RX() on every received slot.
 * then, if TX is enabled it performs TX for n+4. 
 * \param arg is a pointer to a \ref PHY_VARS_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */

static void *UE_thread_rxn_txnp4(void *arg)
{
  static int UE_thread_rxtx_retval;
  UE_rxtx_proc_t *proc = (UE_rxtx_proc_t *)arg;
  int ret;
  PHY_VARS_UE *UE=PHY_vars_UE_g[0][proc->CC_id];
  proc->instance_cnt_rxtx=-1;


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
    perror("[SCHED] UE_thread_rxtx : sched_setattr failed\n");
    return &UE_thread_rxtx_retval;
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

  // Lock memory from swapping. This is a process wide call (not constraint to this thread).
  mlockall(MCL_CURRENT | MCL_FUTURE);

  printf("waiting for sync (UE_thread_rxn_txnp4)\n");

  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (UE_thread_rxn_txnp4)\n");

  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);

  pthread_mutex_unlock(&sync_mutex);
  printf("unlocked sync_mutex, waiting (UE_thread_rxtx)\n");

  printf("Starting UE RXN_TXNP4 thread\n");

  while (!oai_exit) {
    if (pthread_mutex_lock(&proc->mutex_rxtx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE RXTX\n" );
      exit_fun("nothing to add");
      return &UE_thread_rxtx_retval;
    }

    while (proc->instance_cnt_rxtx < 0) {
      // most of the time, the thread is waiting here
      pthread_cond_wait( &proc->cond_rxtx, &proc->mutex_rxtx );
    }

    if (pthread_mutex_unlock(&proc->mutex_rxtx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE RXn_TXnp4\n" );
      exit_fun("nothing to add");
      return &UE_thread_rxtx_retval;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_THREAD_RXTX0+(proc->subframe_rx&1), 1 );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_RX0_UE+(proc->subframe_rx&1), proc->subframe_rx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_SUBFRAME_NUMBER_TX0_UE+(proc->subframe_tx&1), proc->subframe_tx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_RX0_UE+(proc->subframe_rx&1), proc->frame_rx );
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME( VCD_SIGNAL_DUMPER_VARIABLES_FRAME_NUMBER_TX0_UE+(proc->subframe_tx&1), proc->frame_tx );

    if ((subframe_select( &UE->frame_parms, proc->subframe_rx) == SF_DL) ||
	(UE->frame_parms.frame_type == FDD) ||
	(subframe_select( &UE->frame_parms, proc->subframe_rx ) == SF_S)) {
    
      phy_procedures_UE_RX( UE, proc, 0, 0, UE->mode, no_relay, NULL );
    }
    
    if (UE->mac_enabled==1) {

      ret = mac_xface->ue_scheduler(UE->Mod_id,
				    proc->frame_tx,
				    proc->subframe_rx,
				    subframe_select(&UE->frame_parms,proc->subframe_tx),
				    0,
				    0/*FIXME CC_id*/);
      
      if (ret == CONNECTION_LOST) {
	LOG_E( PHY, "[UE %"PRIu8"] Frame %"PRIu32", subframe %u RRC Connection lost, returning to PRACH\n",
	       UE->Mod_id, proc->frame_rx, proc->subframe_tx );
	UE->UE_mode[0] = PRACH;
      } else if (ret == PHY_RESYNCH) {
	LOG_E( PHY, "[UE %"PRIu8"] Frame %"PRIu32", subframe %u RRC Connection lost, trying to resynch\n",
	       UE->Mod_id, proc->frame_rx, proc->subframe_tx );
	UE->UE_mode[0] = RESYNCH;
      } else if (ret == PHY_HO_PRACH) {
	LOG_I( PHY, "[UE %"PRIu8"] Frame %"PRIu32", subframe %u, return to PRACH and perform a contention-free access\n",
	       UE->Mod_id, proc->frame_rx, proc->subframe_tx );
	UE->UE_mode[0] = PRACH;
      }
    }

    if ((subframe_select( &UE->frame_parms, proc->subframe_tx) == SF_UL) ||
	(UE->frame_parms.frame_type == FDD) ||
	(subframe_select( &UE->frame_parms, proc->subframe_tx ) == SF_S)) {

      if (UE->mode != loop_through_memory) {
	phy_procedures_UE_TX(UE,proc,0,0,UE->mode,no_relay);
      }
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_THREAD_RXTX0+(proc->subframe_rx&1), 0 );

    
    if (pthread_mutex_lock(&proc->mutex_rxtx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE RXTX\n" );
      exit_fun("noting to add");
      return &UE_thread_rxtx_retval;
    }
    
    proc->instance_cnt_rxtx--;
    VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE_INST_CNT_RX, proc->instance_cnt_rxtx);
    
    if (pthread_mutex_unlock(&proc->mutex_rxtx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE RXTX\n" );
      exit_fun("noting to add");
      return &UE_thread_rxtx_retval;
    }
  }
  
  // thread finished
  return &UE_thread_rxtx_retval;
}





#define RX_OFF_MAX 10
#define RX_OFF_MIN 5
#define RX_OFF_MID ((RX_OFF_MAX+RX_OFF_MIN)/2)

/*!
 * \brief This is the main UE thread.
 * This thread controls the other three UE threads:
 * - UE_thread_rxn_txnp4 (even subframes)
 * - UE_thread_rxn_txnp4 (odd subframes)
 * - UE_thread_synch
 * \param arg unused
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */

void *UE_thread(void *arg) {

  static int UE_thread_retval;
  PHY_VARS_UE *UE = PHY_vars_UE_g[0][0];
  //  int tx_enabled = 0;
  unsigned int rxs,txs;
  int dummy_rx[UE->frame_parms.nb_antennas_rx][UE->frame_parms.samples_per_tti] __attribute__((aligned(32)));
  openair0_timestamp timestamp,timestamp1;
  void* rxp[2], *txp[2];

#ifdef NAS_UE
  MessageDef *message_p;
#endif

  int start_rx_stream = 0;
  int rx_off_diff = 0;
  int rx_correction_timer = 0;
  int i;

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

  while (!oai_exit) {
    
    if (UE->is_synchronized == 0) {
      
      if (pthread_mutex_lock(&UE->proc.mutex_synch) != 0) {
	LOG_E( PHY, "[SCHED][UE] verror locking mutex for UE initial synch thread\n" );
	exit_fun("nothing to add");
	return &UE_thread_retval;
      }
      
      int instance_cnt_synch = UE->proc.instance_cnt_synch;
      
      if (pthread_mutex_unlock(&UE->proc.mutex_synch) != 0) {
	LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE initial synch thread\n" );
	exit_fun("nothing to add");
	return &UE_thread_retval;
      }
      
      if (instance_cnt_synch < 0) {  // we can invoke the synch
	// grab 10 ms of signal and wakeup synch thread
	for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
	  rxp[i] = (void*)&rxdata[i][0];
      
	if (UE->mode != loop_through_memory) {
	  rxs = UE->rfdevice.trx_read_func(&UE->rfdevice,
					   &timestamp,
					   rxp,
					   UE->frame_parms.samples_per_tti*10,
					   UE->frame_parms.nb_antennas_rx);

	  
	  if (rxs!=UE->frame_parms.samples_per_tti*10) {
	    exit_fun("problem in rx");
	    return &UE_thread_retval;
	  }
	}

	instance_cnt_synch = ++UE->proc.instance_cnt_synch;
	if (instance_cnt_synch == 0) {
	  if (pthread_cond_signal(&UE->proc.cond_synch) != 0) {
	    LOG_E( PHY, "[SCHED][UE] ERROR pthread_cond_signal for UE sync thread\n" );
	    exit_fun("nothing to add");
	    return &UE_thread_retval;
	  }
	} else {
	  LOG_E( PHY, "[SCHED][UE] UE sync thread busy!!\n" );
	  exit_fun("nothing to add");
	  return &UE_thread_retval;
	}
      } // 
      else {
	// grab 10 ms of signal into dummy buffer

	if (UE->mode != loop_through_memory) {
	  for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
	    rxp[i] = (void*)&dummy_rx[i][0];
	  for (int sf=0;sf<10;sf++) {
	    //	    printf("Reading dummy sf %d\n",sf);
	    rxs = UE->rfdevice.trx_read_func(&UE->rfdevice,
					     &timestamp,
					     rxp,
					     UE->frame_parms.samples_per_tti,
					     UE->frame_parms.nb_antennas_rx);

	    if (rxs!=UE->frame_parms.samples_per_tti){
	      exit_fun("problem in rx");
	      return &UE_thread_retval;
	    }

	  }
	}
      }
      
    } // UE->is_synchronized==0
    else {
      if (start_rx_stream==0) {
	start_rx_stream=1;
	if (UE->mode != loop_through_memory) {

	  if (UE->no_timing_correction==0) {
	    LOG_I(PHY,"Resynchronizing RX by %d samples (mode = %d)\n",UE->rx_offset,UE->mode);
	    rxs = UE->rfdevice.trx_read_func(&UE->rfdevice,
					     &timestamp,
					     (void**)rxdata,
					     UE->rx_offset,
					     UE->frame_parms.nb_antennas_rx);
	    if (rxs != UE->rx_offset) {
	      exit_fun("problem in rx");
	      return &UE_thread_retval;
	    }
	  }
	  UE->rx_offset=0;
	  UE->proc.proc_rxtx[0].frame_rx++;
	  UE->proc.proc_rxtx[1].frame_rx++;

	  // read in first symbol
	  rxs = UE->rfdevice.trx_read_func(&UE->rfdevice,
					   &timestamp,
					   (void**)rxdata,
					   UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0,
					   UE->frame_parms.nb_antennas_rx);
	  slot_fep(UE,
		   0,
		   0,
		   0,
		   0,
		   0);
	  if (rxs != UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0) {
	    exit_fun("problem in rx");
	    return &UE_thread_retval;
	  }
	} //UE->mode != loop_through_memory
	else
	  rt_sleep_ns(1000000);

      }// start_rx_stream==0
      else {
	//UE->proc.proc_rxtx[0].frame_rx++;
	//UE->proc.proc_rxtx[1].frame_rx++;
	
	for (int sf=0;sf<10;sf++) {
	  for (i=0; i<UE->frame_parms.nb_antennas_rx; i++) 
	    rxp[i] = (void*)&rxdata[i][UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0+(sf*UE->frame_parms.samples_per_tti)];
	  // grab signal for subframe
	  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );
	  if (UE->mode != loop_through_memory) {
	    if (sf<9) {
	      rxs = UE->rfdevice.trx_read_func(&UE->rfdevice,
					       &timestamp,
					       rxp,
					       UE->frame_parms.samples_per_tti,
					       UE->frame_parms.nb_antennas_rx);
	      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
	      // prepare tx buffer pointers
	      
	      for (i=0; i<UE->frame_parms.nb_antennas_tx; i++)
		txp[i] = (void*)&UE->common_vars.txdata[i][((sf+2)%10)*UE->frame_parms.samples_per_tti];
	      
	      txs = UE->rfdevice.trx_write_func(&UE->rfdevice,
						timestamp+
						(2*UE->frame_parms.samples_per_tti) -
						UE->frame_parms.ofdm_symbol_size-UE->frame_parms.nb_prefix_samples0 -
						openair0_cfg[0].tx_sample_advance,
						txp,
						UE->frame_parms.samples_per_tti,
						UE->frame_parms.nb_antennas_tx,
						1);
              if (txs !=  UE->frame_parms.samples_per_tti) {
                 LOG_E(PHY,"TX : Timeout (sent %d/%d)\n",txs, UE->frame_parms.samples_per_tti);
                 exit_fun( "problem transmitting samples" );
              }

	      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );

	    }
	    
	    else {
	      rxs = UE->rfdevice.trx_read_func(&UE->rfdevice,
					       &timestamp,
					       rxp,
					       UE->frame_parms.samples_per_tti-UE->frame_parms.ofdm_symbol_size-UE->frame_parms.nb_prefix_samples0,
					       UE->frame_parms.nb_antennas_rx);
	      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 1 );
	      // prepare tx buffer pointers
	      
	      for (i=0; i<UE->frame_parms.nb_antennas_tx; i++)
		txp[i] = (void*)&UE->common_vars.txdata[i][((sf+2)%10)*UE->frame_parms.samples_per_tti];
	      
	      txs = UE->rfdevice.trx_write_func(&UE->rfdevice,
						timestamp+
						(2*UE->frame_parms.samples_per_tti) -
						UE->frame_parms.ofdm_symbol_size-UE->frame_parms.nb_prefix_samples0 -
						openair0_cfg[0].tx_sample_advance,
						txp,
						UE->frame_parms.samples_per_tti - rx_off_diff,
						UE->frame_parms.nb_antennas_tx,
						1);
              if (txs !=  UE->frame_parms.samples_per_tti - rx_off_diff) {
                 LOG_E(PHY,"TX : Timeout (sent %d/%d)\n",txs, UE->frame_parms.samples_per_tti-rx_off_diff);
                 exit_fun( "problem transmitting samples" );
              }
	      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_WRITE, 0 );

	      // read in first symbol of next frame and adjust for timing drift
	      rxs = UE->rfdevice.trx_read_func(&UE->rfdevice,
					       &timestamp1,
					       (void**)rxdata,
					       UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0 - rx_off_diff,
					       UE->frame_parms.nb_antennas_rx);
	      rx_off_diff = 0;
	    }
	  }
	  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 0 );
	  // operate on thread sf mod 2
	  UE_rxtx_proc_t *proc = &UE->proc.proc_rxtx[sf&1];

	  // lock mutex
	  if (pthread_mutex_lock(&proc->mutex_rxtx) != 0) {
	    LOG_E( PHY, "[SCHED][UE] error locking mutex for UE RX\n" );
	    exit_fun("nothing to add");
	    return &UE_thread_retval;
	  }
	  // increment instance count and change proc subframe/frame variables
	  int instance_cnt_rxtx = ++proc->instance_cnt_rxtx;
	  if(sf == 0)
	  {
	     UE->proc.proc_rxtx[0].frame_rx++;
	     UE->proc.proc_rxtx[1].frame_rx++;
	  }
	  proc->subframe_rx=sf;
	  proc->subframe_tx=(sf+4)%10;
	  proc->frame_tx = proc->frame_rx + ((proc->subframe_rx>5)?1:0);
	  proc->timestamp_tx = timestamp+(4*UE->frame_parms.samples_per_tti)-UE->frame_parms.ofdm_symbol_size-UE->frame_parms.nb_prefix_samples0;

	  /*
	  if (sf != (timestamp/UE->frame_parms.samples_per_tti)%10) {
	    LOG_E(PHY,"steady-state UE_thread error : frame_rx %d, subframe_rx %d, frame_tx %d, subframe_tx %d, rx subframe %d\n",proc->frame_rx,proc->subframe_rx,proc->frame_tx,proc->subframe_tx,(timestamp/UE->frame_parms.samples_per_tti)%10);
	    exit(-1);
	  }
	  */
	  if (pthread_mutex_unlock(&proc->mutex_rxtx) != 0) {
	    LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE RX\n" );
	    exit_fun("nothing to add");
	    return &UE_thread_retval;
	  }


	  if (instance_cnt_rxtx == 0) {
	    if (pthread_cond_signal(&proc->cond_rxtx) != 0) {
	      LOG_E( PHY, "[SCHED][UE] ERROR pthread_cond_signal for UE RX thread\n" );
	      exit_fun("nothing to add");
	      return &UE_thread_retval;
	    }
	  } else {
	    LOG_E( PHY, "[SCHED][UE] UE RX thread busy (IC %d)!!\n", instance_cnt_rxtx);
	    if (instance_cnt_rxtx > 2) {
	      sleep(1);
	      exit_fun("instance_cnt_rxtx > 2");
	      return &UE_thread_retval;
	    }
	  }
	  if (UE->mode == loop_through_memory) {
	    printf("Processing subframe %d",proc->subframe_rx);
	    getchar();
	  }
	}// for sf=0..10
	if ((UE->rx_offset<(5*UE->frame_parms.samples_per_tti)) &&
	    (UE->rx_offset > RX_OFF_MIN) && 
	    (rx_correction_timer == 0)) {
	  rx_off_diff = -UE->rx_offset + RX_OFF_MIN;
	  LOG_D(PHY,"UE->rx_offset %d > %d, diff %d\n",UE->rx_offset,RX_OFF_MIN,rx_off_diff);
	  rx_correction_timer = 5;
	} else if ((UE->rx_offset>(5*UE->frame_parms.samples_per_tti)) && 
		   (UE->rx_offset < ((10*UE->frame_parms.samples_per_tti)-RX_OFF_MIN)) &&
		   (rx_correction_timer == 0)) {   // moving to the left so drop rx_off_diff samples
	  rx_off_diff = 10*UE->frame_parms.samples_per_tti - RX_OFF_MIN - UE->rx_offset;
	  LOG_D(PHY,"UE->rx_offset %d < %d, diff %d\n",UE->rx_offset,10*UE->frame_parms.samples_per_tti-RX_OFF_MIN,rx_off_diff);
	  
	  rx_correction_timer = 5;
	}
	
	if (rx_correction_timer>0)
	  rx_correction_timer--;
      } // start_rx_stream==1
    } // UE->is_synchronized==1
      
  } // while !oai_exit
 return NULL;
} // UE_thread

/*
void *UE_thread_old(void *arg)
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
  void* rxp[2];

  openair0_timestamp timestamp;

#ifdef NAS_UE
  MessageDef *message_p;
#endif

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


    while (rxpos < (1+hw_subframe)*UE->frame_parms.samples_per_tti) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_TRX_READ, 1 );

#ifndef USRP_DEBUG

      DevAssert( UE->frame_parms.nb_antennas_rx <= 2 );
      void* rxp[2];

      for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
        rxp[i] = (dummy_dump==0) ? (void*)&rxdata[i][rxpos] : (void*)dummy[i];
      
    
      if (UE->mode != loop_through_memory) {
	rxs = UE->rfdevice.trx_read_func(&UE->rfdevice,
				     &timestamp,
				     rxp,
				     spp - ((first_rx==1) ? rx_off_diff : 0),
				     UE->frame_parms.nb_antennas_rx);

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

        DevAssert( UE->frame_parms.nb_antennas_tx <= 2 );
        void* txp[2];

        for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
          txp[i] = (void*)&txdata[i][txpos];

        UE->rfdevice.trx_write_func(&openair0,
                                (timestamp+openair0_cfg[0].tx_scheduling_advance-openair0_cfg[0].tx_sample_advance),
                                txp,
				spp - ((first_rx==1) ? rx_off_diff : 0),
                                UE->frame_parms.nb_antennas_tx,
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

      if (txpos >= 10*PHY_vars_UE_g[0][0]->frame_parms.samples_per_tti)
        txpos -= 10*PHY_vars_UE_g[0][0]->frame_parms.samples_per_tti;
    }

    if (rxpos >= 10*PHY_vars_UE_g[0][0]->frame_parms.samples_per_tti)
      rxpos -= 10*PHY_vars_UE_g[0][0]->frame_parms.samples_per_tti;

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
          if (pthread_cond_signal(&UE->proc.cond_rx) != 0) {
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
                    UE->frame_parms.N_RB_DL,
                    UE->frame_parms.phich_config_common.phich_duration,
                    UE->frame_parms.phich_config_common.phich_resource,
                    UE->frame_parms.Nid_cell,
                    UE->frame_parms.nb_antennas_tx_eNB,
                    UE->common_vars.freq_offset,
                    UE->measurements.rx_power_avg_dB[0],
                    UE->measurements.rx_power_avg_dB[0] - rx_input_level_dBm,
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
          LTE_DL_FRAME_PARMS *frame_parms = &UE->frame_parms; // for macro FRAME_LENGTH_COMPLEX_SAMPLES

	  //	  LOG_I(PHY,"UE->rx_offset %d\n",UE->rx_offset);
          if ((UE->rx_offset > RX_OFF_MAX) && (start_rx_stream == 0)) {
            start_rx_stream=1;
            frame=0;
            // dump ahead in time to start of frame

#ifndef USRP_DEBUG
	    if (UE->mode != loop_through_memory) {
	      LOG_I(PHY,"Resynchronizing RX by %d samples\n",UE->rx_offset);
	      rxs = UE->rfdevice.trx_read_func(&UE->rfdevice,
					   &timestamp,
					   (void**)rxdata,
					   UE->rx_offset,
					   UE->frame_parms.nb_antennas_rx);
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

*/

/*!
 * \brief Initialize the UE theads.
 * Creates the UE threads:
 * - UE_thread_rxtx0
 * - UE_thread_rxtx1
 * - UE_thread_synch
 * and the locking between them.
 */
void init_UE_threads(int inst)
{
  PHY_VARS_UE *UE;
 
  UE = PHY_vars_UE_g[inst][0];

  pthread_attr_init (&UE->proc.attr_ue);
  pthread_attr_setstacksize(&UE->proc.attr_ue,8192);//5*PTHREAD_STACK_MIN);
  
#ifndef LOWLATENCY
  UE->proc.sched_param_ue.sched_priority = sched_get_priority_max(SCHED_FIFO);
  pthread_attr_setschedparam(&UE->proc.attr_ue,&sched_param_UE_thread);
#endif

  // the threads are not yet active, therefore access is allowed without locking
  UE->proc.proc_rxtx[0].instance_cnt_rxtx = -1;
  UE->proc.proc_rxtx[1].instance_cnt_rxtx = -1;
  UE->proc.instance_cnt_synch = -1;
  pthread_mutex_init(&UE->proc.proc_rxtx[0].mutex_rxtx,NULL);
  pthread_mutex_init(&UE->proc.proc_rxtx[1].mutex_rxtx,NULL);
  pthread_mutex_init(&UE->proc.mutex_synch,NULL);
  pthread_cond_init(&UE->proc.proc_rxtx[0].cond_rxtx,NULL);
  pthread_cond_init(&UE->proc.proc_rxtx[1].cond_rxtx,NULL);
  pthread_cond_init(&UE->proc.cond_synch,NULL);
  pthread_create(&UE->proc.proc_rxtx[0].pthread_rxtx,NULL,UE_thread_rxn_txnp4,(void*)&UE->proc.proc_rxtx[0]);
  pthread_setname_np( UE->proc.proc_rxtx[0].pthread_rxtx, "UE_thread_rxn_txnp4_even" );
  pthread_create(&UE->proc.proc_rxtx[1].pthread_rxtx,NULL,UE_thread_rxn_txnp4,(void*)&UE->proc.proc_rxtx[1]);
  pthread_setname_np( UE->proc.proc_rxtx[1].pthread_rxtx, "UE_thread_rxn_txnp4_odd" );
  pthread_create(&UE->proc.pthread_synch,NULL,UE_thread_synch,(void*)UE);
  pthread_setname_np( UE->proc.pthread_synch, "UE_thread_synch" );
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

int setup_ue_buffers(PHY_VARS_UE **phy_vars_ue, openair0_config_t *openair0_cfg)
{

  int i, CC_id;
  LTE_DL_FRAME_PARMS *frame_parms;
  openair0_rf_map *rf_map;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    rf_map = &phy_vars_ue[CC_id]->rf_map;
    
    if (phy_vars_ue[CC_id]) {
      frame_parms = &(phy_vars_ue[CC_id]->frame_parms);
    } else {
      printf("phy_vars_UE[%d] not initialized\n", CC_id);
      return(-1);
    }
    
    /*
    if (frame_parms->frame_type == TDD) {
      if (frame_parms->N_RB_DL == 100)
        N_TA_offset = 624;
      else if (frame_parms->N_RB_DL == 50)
        N_TA_offset = 624/2;
      else if (frame_parms->N_RB_DL == 25)
        N_TA_offset = 624/4;
      }
    */
    
    // replace RX signal buffers with mmaped HW versions
    rxdata = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
    txdata = (int32_t**)malloc16( frame_parms->nb_antennas_tx*sizeof(int32_t*) );
    
    for (i=0; i<frame_parms->nb_antennas_rx; i++) {
      printf( "Mapping UE CC_id %d, rx_ant %d, freq %u on card %d, chain %d\n", CC_id, i, downlink_frequency[CC_id][i], rf_map->card, rf_map->chain+i );
      free( phy_vars_ue[CC_id]->common_vars.rxdata[i] );
      rxdata[i] = (int32_t*)malloc16_clear( 307200*sizeof(int32_t) );
      phy_vars_ue[CC_id]->common_vars.rxdata[i] = rxdata[i]; // what about the "-N_TA_offset" ? // N_TA offset for TDD
      printf("rxdata[%d] : %p\n",i,rxdata[i]);
    }
    
    for (i=0; i<frame_parms->nb_antennas_tx; i++) {
      printf( "Mapping UE CC_id %d, tx_ant %d, freq %u on card %d, chain %d\n", CC_id, i, downlink_frequency[CC_id][i], rf_map->card, rf_map->chain+i );
      free( phy_vars_ue[CC_id]->common_vars.txdata[i] );
      txdata[i] = (int32_t*)malloc16_clear( 307200*sizeof(int32_t) );
      phy_vars_ue[CC_id]->common_vars.txdata[i] = txdata[i];
      printf("txdata[%d] : %p\n",i,txdata[i]);
    }
    
    // rxdata[x] points now to the same memory region as phy_vars_ue[CC_id]->common_vars.rxdata[x]
    // txdata[x] points now to the same memory region as phy_vars_ue[CC_id]->common_vars.txdata[x]
    // be careful when releasing memory!
    // because no "release_ue_buffers"-function is available, at least rxdata and txdata memory will leak (only some bytes)
    
  }

  return 0;
}
