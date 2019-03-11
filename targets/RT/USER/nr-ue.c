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
#include "nr-uesoftmodem.h"

#include "rt_wrapper.h"

#include "LAYER2/NR_MAC_UE/mac.h"
//#include "RRC/LTE/rrc_extern.h"
#include "PHY_INTERFACE/phy_interface_extern.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "fapi_nr_ue_l1.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"
#include "RRC/NR_UE/rrc_proto.h"

//#ifndef NO_RAT_NR
#include "SCHED_NR/phy_frame_config_nr.h"
//#endif
#include "SCHED_NR_UE/defs.h"

#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#include "T.h"

#ifdef XFORMS
  #include "PHY/TOOLS/nr_phy_scope.h"

  extern char do_forms;
#endif


extern double cpuf;
//static  nfapi_nr_config_request_t config_t;
//static  nfapi_nr_config_request_t* config =&config_t;

/*
 *  NR SLOT PROCESSING SEQUENCE
 *
 *  Processing occurs with following steps for connected mode:
 *
 *  - Rx samples for a slot are received,
 *  - PDCCH processing (including DCI extraction for downlink and uplink),
 *  - PDSCH processing (including transport blocks decoding),
 *  - PUCCH/PUSCH (transmission of acknowledgements, CSI, ... or data).
 *
 *  Time between reception of the slot and related transmission depends on UE processing performance.
 *  It is defined by the value NR_UE_CAPABILITY_SLOT_RX_TO_TX.
 *
 *  In NR, network gives the duration between Rx slot and Tx slot in the DCI:
 *  - for reception of a PDSCH and its associated acknowledgment slot (with a PUCCH or a PUSCH),
 *  - for reception of an uplink grant and its associated PUSCH slot.
 *
 *  So duration between reception and it associated transmission depends on its transmission slot given in the DCI.
 *  NR_UE_CAPABILITY_SLOT_RX_TO_TX means the minimum duration but higher duration can be given by the network because UE can support it.
 *
*                                                                                                    Slot k
*                                                                                  -------+------------+--------
*                Frame                                                                    | Tx samples |
*                Subframe                                                                 |   buffer   |
*                Slot n                                                            -------+------------+--------
*       ------ +------------+--------                                                     |
*              | Rx samples |                                                             |
*              |   buffer   |                                                             |
*       -------+------------+--------                                                     |
*                           |                                                             |
*                           V                                                             |
*                           +------------+                                                |
*                           |   PDCCH    |                                                |
*                           | processing |                                                |
*                           +------------+                                                |
*                           |            |                                                |
*                           |            v                                                |
*                           |            +------------+                                   |
*                           |            |   PDSCH    |                                   |
*                           |            | processing | decoding result                   |
*                           |            +------------+    -> ACK/NACK of PDSCH           |
*                           |                         |                                   |
*                           |                         v                                   |
*                           |                         +-------------+------------+        |
*                           |                         | PUCCH/PUSCH | Tx samples |        |
*                           |                         |  processing | transfer   |        |
*                           |                         +-------------+------------+        |
*                           |                                                             |
*                           |/___________________________________________________________\|
*                            \  duration between reception and associated transmission   /
*
* Remark: processing is done slot by slot, it can be distribute on different threads which are executed in parallel.
* This is an architecture optimization in order to cope with real time constraints.
* By example, for LTE, subframe processing is spread over 4 different threads.
*
 */

#ifndef NO_RAT_NR
  #define DURATION_RX_TO_TX           (NR_UE_CAPABILITY_SLOT_RX_TO_TX)  /* for NR this will certainly depends to such UE capability which is not yet defined */
#else
  #define DURATION_RX_TO_TX           (4)   /* For LTE, this duration is fixed to 4 and it is linked to LTE standard for both modes FDD/TDD */
#endif

#define FRAME_PERIOD    100000000ULL
#define DAQ_PERIOD      66667ULL
#define FIFO_PRIORITY   40

typedef enum {
  pss=0,
  pbch=1,
  si=2
} sync_mode_t;

void init_UE_threads(PHY_VARS_NR_UE *UE);
void *UE_thread(void *arg);
void init_UE(int nb_inst);

int32_t **rxdata;
int32_t **txdata;

#define KHz (1000UL)
#define MHz (1000*KHz)
#define SAIF_ENABLED

#ifdef SAIF_ENABLED
  uint64_t  g_ue_rx_thread_busy = 0;
#endif

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
  {1,  1920000, 1980000, 2110000, 2170000, FDD},
  {2,  1850000, 1910000, 1930000, 1990000, FDD},
  {3,  1710000, 1785000, 1805000, 1880000, FDD},
  {5,   824000,  849000,  869000,  894000, FDD},
  {7,  2500000, 2570000, 2620000, 2690000, FDD},
  {8,   880000,  915000,  925000,  960000, FDD},
  {12,  698000,  716000,  728000,  746000, FDD},
  {20,  832000,  862000,  791000,  821000, FDD},
  {25, 1850000, 1915000, 1930000, 1995000, FDD},
  {28,  703000,  758000,  758000,  813000, FDD},
  {34, 2010000, 2025000, 2010000, 2025000, TDD},
  {38, 2570000, 2620000, 2570000, 2630000, TDD},
  {39, 1880000, 1920000, 1880000, 1920000, TDD},
  {40, 2300000, 2400000, 2300000, 2400000, TDD},
  {41, 2496000, 2690000, 2496000, 2690000, TDD},
  {50, 1432000, 1517000, 1432000, 1517000, TDD},
  {51, 1427000, 1432000, 1427000, 1432000, TDD},
  {66, 1710000, 1780000, 2110000, 2200000, FDD},
  {70, 1695000, 1710000, 1995000, 2020000, FDD},
  {71,  663000,  698000,  617000,  652000, FDD},
  {74, 1427000, 1470000, 1475000, 1518000, FDD},
  {75,     000,     000, 1432000, 1517000, FDD},
  {76,     000,     000, 1427000, 1432000, FDD},
  {77, 3300000, 4200000, 3300000, 4200000, TDD},
  {78, 3300000, 3800000, 3300000, 3800000, TDD},
  {79, 4400000, 5000000, 4400000, 5000000, TDD},
  {80, 1710000, 1785000,     000,     000, FDD},
  {81,  860000,  915000,     000,     000, FDD},
  {82,  832000,  862000,     000,     000, FDD},
  {83,  703000,  748000,     000,     000, FDD},
  {84, 1920000, 1980000,     000,     000, FDD},
  {86, 1710000, 1785000,     000,     000, FDD}
};

PHY_VARS_NR_UE *init_nr_ue_vars(NR_DL_FRAME_PARMS *frame_parms,
                                uint8_t UE_id,
                                uint8_t abstraction_flag)

{
  PHY_VARS_NR_UE *ue;

  if (frame_parms!=(NR_DL_FRAME_PARMS *)NULL) { // if we want to give initial frame parms, allocate the PHY_VARS_UE structure and put them in
    ue = (PHY_VARS_NR_UE *)malloc(sizeof(PHY_VARS_NR_UE));
    memset(ue,0,sizeof(PHY_VARS_NR_UE));
    memcpy(&(ue->frame_parms), frame_parms, sizeof(NR_DL_FRAME_PARMS));
  } else ue = PHY_vars_UE_g[UE_id][0];

  ue->Mod_id      = UE_id;
  ue->mac_enabled = 1;
  // initialize all signal buffers
  init_nr_ue_signal(ue,1,abstraction_flag);
  // intialize transport
  init_nr_ue_transport(ue,abstraction_flag);
  return(ue);
}

void init_thread(int sched_runtime, int sched_deadline, int sched_fifo, cpu_set_t *cpuset, char *name) {
#ifdef DEADLINE_SCHEDULER

  if (sched_runtime!=0) {
    struct sched_attr attr= {0};
    attr.size = sizeof(attr);
    attr.sched_policy = SCHED_DEADLINE;
    attr.sched_runtime  = sched_runtime;
    attr.sched_deadline = sched_deadline;
    attr.sched_period   = 0;
    AssertFatal(sched_setattr(0, &attr, 0) == 0,
                "[SCHED] %s thread: sched_setattr failed %s \n", name, strerror(errno));
    LOG_I(HW,"[SCHED][eNB] %s deadline thread %lu started on CPU %d\n",
          name, (unsigned long)gettid(), sched_getcpu());
  }

#else

  if (CPU_COUNT(cpuset) > 0)
    AssertFatal( 0 == pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), cpuset), "");

  struct sched_param sp;
  sp.sched_priority = sched_fifo;
  AssertFatal(pthread_setschedparam(pthread_self(),SCHED_FIFO,&sp)==0,
              "Can't set thread priority, Are you root?\n");
  /* Check the actual affinity mask assigned to the thread */
  cpu_set_t *cset=CPU_ALLOC(CPU_SETSIZE);

  if (0 == pthread_getaffinity_np(pthread_self(), CPU_ALLOC_SIZE(CPU_SETSIZE), cset)) {
    char txt[512]= {0};

    for (int j = 0; j < CPU_SETSIZE; j++)
      if (CPU_ISSET(j, cset))
        sprintf(txt+strlen(txt), " %d ", j);

    printf("CPU Affinity of thread %s is %s\n", name, txt);
  }

  CPU_FREE(cset);
#endif
  // Lock memory from swapping. This is a process wide call (not constraint to this thread).
  mlockall(MCL_CURRENT | MCL_FUTURE);
  pthread_setname_np( pthread_self(), name );
  // LTS: this sync stuff should be wrong
  printf("waiting for sync (%s)\n",name);
  pthread_mutex_lock(&sync_mutex);
  printf("Locked sync_mutex, waiting (%s)\n",name);

  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);

  pthread_mutex_unlock(&sync_mutex);
  printf("started %s as PID: %ld\n",name, gettid());
}

void init_UE(int nb_inst) {
  int inst;
  NR_UE_MAC_INST_t *mac_inst;

  for (inst=0; inst < nb_inst; inst++) {
    //    UE->rfdevice.type      = NONE_DEV;
    //PHY_VARS_NR_UE *UE = PHY_vars_UE_g[inst][0];
    LOG_I(PHY,"Initializing memory for UE instance %d (%p)\n",inst,PHY_vars_UE_g[inst]);
    PHY_vars_UE_g[inst][0] = init_nr_ue_vars(NULL,inst,0);
    PHY_VARS_NR_UE *UE = PHY_vars_UE_g[inst][0];
    AssertFatal((UE->if_inst = nr_ue_if_module_init(inst)) != NULL, "can not initial IF module\n");
    nr_l3_init_ue();
    nr_l2_init_ue();
    mac_inst = get_mac_inst(0);
    mac_inst->if_module = UE->if_inst;
    UE->if_inst->scheduled_response = nr_ue_scheduled_response;
    UE->if_inst->phy_config_request = nr_ue_phy_config_request;
    LOG_I(PHY,"Intializing UE Threads for instance %d (%p,%p)...\n",inst,PHY_vars_UE_g[inst],PHY_vars_UE_g[inst][0]);
    //init_UE_threads(inst);
    //UE = PHY_vars_UE_g[inst][0];
    AssertFatal(0 == pthread_create(&UE->proc.pthread_ue,
                                    &UE->proc.attr_ue,
                                    UE_thread,
                                    (void *)UE), "");
  }

  printf("UE threads created by %ld\n", gettid());
#if 0
#if defined(ENABLE_USE_MME)
  extern volatile int start_UE;

  while (start_UE == 0) {
    sleep(1);
  }

#endif
#endif
}

/*!
 * \brief This is the UE synchronize thread.
 * It performs band scanning and synchonization.
 * \param arg is a pointer to a \ref PHY_VARS_NR_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */
static void *UE_thread_synch(void *arg) {

  static int __thread UE_thread_synch_retval;
  int i, hw_slot_offset;
  PHY_VARS_NR_UE *UE = (PHY_VARS_NR_UE *) arg;
  int current_band = 0;
  int current_offset = 0;
  lte_frame_type_t current_type;
  sync_mode_t sync_mode = pbch;
  int CC_id = UE->CC_id;
  int freq_offset=0;
  char threadname[128];
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);

  if ( threads.sync != -1 )
    CPU_SET(threads.sync, &cpuset);

  // this thread priority must be lower that the main acquisition thread
  sprintf(threadname, "sync UE %d", UE->Mod_id);
  init_thread(100000, 500000, FIFO_PRIORITY-1, &cpuset, threadname);
  UE->is_synchronized = 0;

  if (UE->UE_scan == 0) {
    int ind;
    int64_t dl_freq_khz = downlink_frequency[0][0]/1000;
    for ( ind=0;
          ind < sizeof(eutra_bands) / sizeof(eutra_bands[0]);
          ind++) {
      current_band = eutra_bands[ind].band;
      current_type = eutra_bands[ind].frame_type;
      LOG_D(PHY, "Scanning band %d, dl_min %"PRIu32", ul_min %"PRIu32"\n", current_band, eutra_bands[ind].dl_min,eutra_bands[ind].ul_min);

      if ( eutra_bands[ind].dl_min <= dl_freq_khz && eutra_bands[ind].dl_max >= dl_freq_khz ) {
        for (i=0; i<4; i++)
          uplink_frequency_offset[CC_id][i] = (eutra_bands[ind].ul_min - eutra_bands[ind].dl_min)*1000;

        break;
      }
    }


    AssertFatal( ind < sizeof(eutra_bands) / sizeof(eutra_bands[0]), "Can't find EUTRA band for frequency");

    UE->frame_parms.eutra_band = current_band;
    UE->frame_parms.frame_type = current_type;

    LOG_I( PHY, "[SCHED][UE] Check absolute frequency DL %"PRIu32", UL %"PRIu32" (oai_exit %d, rx_num_channels %d)\n",
           downlink_frequency[0][0], downlink_frequency[0][0]+uplink_frequency_offset[0][0],
           oai_exit, openair0_cfg[0].rx_num_channels);

    for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
      openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
      openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] =
        downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
      openair0_cfg[UE->rf_map.card].autocal[UE->rf_map.chain+i] = 1;

      if (uplink_frequency_offset[CC_id][i] != 0) //
        openair0_cfg[UE->rf_map.card].duplex_mode = duplex_mode_FDD;
      else //FDD
        openair0_cfg[UE->rf_map.card].duplex_mode = duplex_mode_TDD;
    }

    sync_mode = pbch;
  } else {
    current_band=0;

    for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
      downlink_frequency[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[CC_id].dl_min;
      uplink_frequency_offset[UE->rf_map.card][UE->rf_map.chain+i] =
        bands_to_scan.band_info[CC_id].ul_min-bands_to_scan.band_info[CC_id].dl_min;
      openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
      openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] =
        downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
      openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;
    }
  }

  //    AssertFatal(UE->rfdevice.trx_start_func(&UE->rfdevice) == 0, "Could not start the device\n");

  while (oai_exit==0) {
    AssertFatal ( 0== pthread_mutex_lock(&UE->proc.mutex_synch), "");

    while (UE->proc.instance_cnt_synch < 0)
      // the thread waits here most of the time
      pthread_cond_wait( &UE->proc.cond_synch, &UE->proc.mutex_synch );

    AssertFatal ( 0== pthread_mutex_unlock(&UE->proc.mutex_synch), "");

    switch (sync_mode) {
      case pss:
        LOG_I(PHY,"[SCHED][UE] Scanning band %d (%d), freq %u\n",bands_to_scan.band_info[current_band].band, current_band,bands_to_scan.band_info[current_band].dl_min+current_offset);
        //lte_sync_timefreq(UE,current_band,bands_to_scan.band_info[current_band].dl_min+current_offset);
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
#if DISABLE_LOG_X
        printf("[UE thread Synch] Running Initial Synch (mode %d)\n",UE->mode);
#else
        LOG_I(PHY, "[UE thread Synch] Running Initial Synch (mode %d)\n",UE->mode);
#endif

        if (nr_initial_sync( UE, UE->mode ) == 0) {
          //write_output("txdata_sym.m", "txdata_sym", UE->common_vars.rxdata[0], (10*UE->frame_parms.samples_per_slot), 1, 1);
          freq_offset = UE->common_vars.freq_offset; // frequency offset computed with pss in initial sync
          hw_slot_offset = (UE->rx_offset<<1) / UE->frame_parms.samples_per_slot;
          printf("Got synch: hw_slot_offset %d, carrier off %d Hz, rxgain %d (DL %u, UL %u), UE_scan_carrier %d\n",
                 hw_slot_offset,
                 freq_offset,
                 UE->rx_total_gain_dB,
                 downlink_frequency[0][0]+freq_offset,
                 downlink_frequency[0][0]+uplink_frequency_offset[0][0]+freq_offset,
                 UE->UE_scan_carrier );

          // rerun with new cell parameters and frequency-offset
          for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
            openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;

            if (freq_offset >= 0)
              openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] += abs(freq_offset);
            else
              openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] -= abs(freq_offset);

            openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] =
              openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i]+uplink_frequency_offset[CC_id][i];
            downlink_frequency[CC_id][i] = openair0_cfg[CC_id].rx_freq[i];
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

          if (UE->mode != loop_through_memory) {
            UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0],0);
            //UE->rfdevice.trx_set_gains_func(&openair0,&openair0_cfg[0]);
            //UE->rfdevice.trx_stop_func(&UE->rfdevice);
            // sleep(1);
            //nr_init_frame_parms_ue(&UE->frame_parms);
            /*if (UE->rfdevice.trx_start_func(&UE->rfdevice) != 0 ) {
              LOG_E(HW,"Could not start the device\n");
              oai_exit=1;
                        }*/
          }

          if (UE->UE_scan_carrier == 1) {
            UE->UE_scan_carrier = 0;
          } else {
            AssertFatal ( 0== pthread_mutex_lock(&UE->proc.mutex_synch), "");
            UE->is_synchronized = 1;
            AssertFatal ( 0== pthread_mutex_unlock(&UE->proc.mutex_synch), "");

            if( UE->mode == rx_dump_frame ) {
              FILE *fd;

              if ((UE->proc.proc_rxtx[0].frame_rx&1) == 0) {  // this guarantees SIB1 is present
                if ((fd = fopen("rxsig_frame0.dat","w")) != NULL) {
                  fwrite((void *)&UE->common_vars.rxdata[0][0],
                         sizeof(int32_t),
                         10*UE->frame_parms.samples_per_subframe,
                         fd);
                  LOG_I(PHY,"Dummping Frame ... bye bye \n");
                  fclose(fd);
                  exit(0);
                } else {
                  LOG_E(PHY,"Cannot open file for writing\n");
                  exit(0);
                }
              } else {
                AssertFatal ( 0== pthread_mutex_lock(&UE->proc.mutex_synch), "");
                UE->is_synchronized = 0;
                AssertFatal ( 0== pthread_mutex_unlock(&UE->proc.mutex_synch), "");
              }
            }
          }
        } else {
          // initial sync failed
          // calculate new offset and try again
          if (UE->UE_scan_carrier == 1) {
            if (freq_offset >= 0)
              freq_offset += 100;

            freq_offset *= -1;

            if (abs(freq_offset) > 7500) {
              LOG_I( PHY, "[initial_sync] No cell synchronization found, abandoning\n" );
              FILE *fd;

              if ((fd = fopen("rxsig_frame0.dat","w"))!=NULL) {
                fwrite((void *)&UE->common_vars.rxdata[0][0],
                       sizeof(int32_t),
                       10*UE->frame_parms.samples_per_subframe,
                       fd);
                LOG_I(PHY,"Dummping Frame ... bye bye \n");
                fclose(fd);
                exit(0);
              }

              //mac_xface->macphy_exit("No cell synchronization found, abandoning"); new mac
              return &UE_thread_synch_retval; // not reached
            }
          }

#if DISABLE_LOG_X
          printf("[initial_sync] trying carrier off %d Hz, rxgain %d (DL %u, UL %u)\n",
                 freq_offset,
                 UE->rx_total_gain_dB,
                 downlink_frequency[0][0]+freq_offset,
                 downlink_frequency[0][0]+uplink_frequency_offset[0][0]+freq_offset );
#else
          LOG_I(PHY, "[initial_sync] trying carrier off %d Hz, rxgain %d (DL %u, UL %u)\n",
                freq_offset,
                UE->rx_total_gain_dB,
                downlink_frequency[0][0]+freq_offset,
                downlink_frequency[0][0]+uplink_frequency_offset[0][0]+freq_offset );
#endif

          for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
            openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+freq_offset;
            openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i]+freq_offset;
            openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;

            if (UE->UE_scan_carrier==1)
              openair0_cfg[UE->rf_map.card].autocal[UE->rf_map.chain+i] = 1;
          }

          if (UE->mode != loop_through_memory)
            UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0],0);
        }// initial_sync=0

        break;

      case si:
      default:
        break;
    }

#if 0 //defined XFORMS

    if (do_forms) {
      extern FD_lte_phy_scope_ue  *form_ue[NUMBER_OF_UE_MAX];
      phy_scope_UE(form_ue[0],
                   PHY_vars_UE_g[0][0],
                   0,0,1);
    }

#endif
    AssertFatal ( 0== pthread_mutex_lock(&UE->proc.mutex_synch), "");
    // indicate readiness
    UE->proc.instance_cnt_synch--;
    AssertFatal ( 0== pthread_mutex_unlock(&UE->proc.mutex_synch), "");
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME( VCD_SIGNAL_DUMPER_FUNCTIONS_UE_THREAD_SYNCH, 0 );
  }  // while !oai_exit

  return &UE_thread_synch_retval;
}

void processSlotRX( PHY_VARS_NR_UE *UE, UE_nr_rxtx_proc_t *proc) {
  // Process Rx data for one sub-frame
  if (slot_select_nr(&UE->frame_parms, proc->frame_tx, proc->nr_tti_tx) & NR_DOWNLINK_SLOT) {
    //clean previous FAPI MESSAGE
    UE->rx_ind.number_pdus = 0;
    UE->dci_ind.number_of_dcis = 0;
    //clean previous FAPI MESSAGE
    // call L2 for DL_CONFIG (DCI)
    UE->dcireq.module_id = UE->Mod_id;
    UE->dcireq.gNB_index = 0;
    UE->dcireq.cc_id     = 0;
    UE->dcireq.frame     = proc->frame_rx;
    UE->dcireq.slot      = proc->nr_tti_rx;
    nr_ue_dcireq(&UE->dcireq); //to be replaced with function pointer later
    NR_UE_MAC_INST_t *UE_mac = get_mac_inst(0);
    UE_mac->scheduled_response.dl_config = &UE->dcireq.dl_config_req;
    UE_mac->scheduled_response.slot = proc->nr_tti_rx;
    nr_ue_scheduled_response(&UE_mac->scheduled_response);
    //write_output("uerxdata_frame.m", "uerxdata_frame", UE->common_vars.rxdata[0], UE->frame_parms.samples_per_frame, 1, 1);
    printf("Processing slot %d\n",proc->nr_tti_rx);
#ifdef UE_SLOT_PARALLELISATION
    phy_procedures_slot_parallelization_nrUE_RX( UE, proc, 0, 0, 1, UE->mode, no_relay, NULL );
#else
    phy_procedures_nrUE_RX( UE, proc, 0, 1, UE->mode, UE_mac->phy_config.config_req.pbch_config);
    //            printf(">>> nr_ue_pdcch_procedures ended\n");
#endif
  }

  if (UE->mac_enabled==1) {
    //  trigger L2 to run ue_scheduler thru IF module
    //  [TODO] mapping right after NR initial sync
    if(UE->if_inst != NULL && UE->if_inst->ul_indication != NULL) {
      UE->ul_indication.module_id = 0;
      UE->ul_indication.gNB_index = 0;
      UE->ul_indication.cc_id = 0;
      UE->ul_indication.frame = proc->frame_rx;
      UE->ul_indication.slot = proc->nr_tti_rx;
      UE->if_inst->ul_indication(&UE->ul_indication);
    }
  }
}

/*!
 * \brief This is the UE thread for RX subframe n and TX subframe n+4.
 * This thread performs the phy_procedures_UE_RX() on every received slot.
 * then, if TX is enabled it performs TX for n+4.
 * \param arg is a pointer to a \ref PHY_VARS_NR_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */

static void *UE_thread_rxn_txnp4(void *arg) {
  struct nr_rxtx_thread_data *rtd = arg;
  UE_nr_rxtx_proc_t *proc = rtd->proc;
  PHY_VARS_NR_UE    *UE   = rtd->UE;
  //proc->counter_decoder = 0;
  proc->instance_cnt_rxtx=-1;
  proc->subframe_rx=proc->sub_frame_start;
  proc->dci_err_cnt=0;
  char threadname[256];
  sprintf(threadname,"UE_%d_proc_%d", UE->Mod_id, proc->sub_frame_start);
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  char timing_proc_name[256];
  sprintf(timing_proc_name,"Delay to process sub-frame proc %d",proc->sub_frame_start);

  if ( (proc->sub_frame_start+1)%RX_NB_TH == 0 && threads.one != -1 )
    CPU_SET(threads.one, &cpuset);

  if ( (proc->sub_frame_start+1)%RX_NB_TH == 1 && threads.two != -1 )
    CPU_SET(threads.two, &cpuset);

  if ( (proc->sub_frame_start+1)%RX_NB_TH == 2 && threads.three != -1 )
    CPU_SET(threads.three, &cpuset);

  //CPU_SET(threads.three, &cpuset);
  init_thread(900000,1000000, FIFO_PRIORITY-1, &cpuset,
              threadname);

  while (!oai_exit) {
    AssertFatal( 0 == pthread_mutex_lock(&proc->mutex_rxtx), "[SCHED][UE] error locking mutex for UE RXTX\n" );

    while (proc->instance_cnt_rxtx < 0) {
      // most of the time, the thread is waiting here
      pthread_cond_wait( &proc->cond_rxtx, &proc->mutex_rxtx );
    }

    AssertFatal ( 0== pthread_mutex_unlock(&proc->mutex_rxtx), "[SCHED][UE] error unlocking mutex for UE RXn_TXnp4\n" );
    processSlotRX(UE, proc);
    //printf(">>> mac ended\n");
    // Prepare the future Tx data
#if 0
#ifndef NO_RAT_NR

    if (slot_select_nr(&UE->frame_parms, proc->frame_tx, proc->nr_tti_tx) & NR_UPLINK_SLOT)
#else
    if ((subframe_select( &UE->frame_parms, proc->subframe_tx) == SF_UL) ||
        (UE->frame_parms.frame_type == FDD) )
#endif
      if (UE->mode != loop_through_memory)
        phy_procedures_nrUE_TX(UE,proc,0,0,UE->mode,no_relay);

    //phy_procedures_UE_TX(UE,proc,0,0,UE->mode,no_relay);
#endif
#if 0

    if ((subframe_select( &UE->frame_parms, proc->subframe_tx) == SF_S) &&
        (UE->frame_parms.frame_type == TDD))
      if (UE->mode != loop_through_memory)
        //phy_procedures_UE_S_TX(UE,0,0,no_relay);
        updateTimes(current, &t3, 10000, timing_proc_name);

#endif
    AssertFatal( 0 == pthread_mutex_lock(&proc->mutex_rxtx), "[SCHED][UE] error locking mutex for UE RXTX\n" );
    proc->instance_cnt_rxtx--;
#if BASIC_SIMULATOR

    if (pthread_cond_signal(&proc->cond_rxtx) != 0) abort();

#endif
    AssertFatal (0 == pthread_mutex_unlock(&proc->mutex_rxtx), "[SCHED][UE] error unlocking mutex for UE RXTX\n" );
  }

  // thread finished
  free(arg);
  return NULL;
}

void readFrame(PHY_VARS_NR_UE *UE,  openair0_timestamp *timestamp) {
  void *rxp[NB_ANTENNAS_RX];
  void *dummy_tx[UE->frame_parms.nb_antennas_tx];

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    dummy_tx[i]=malloc16_clear(UE->frame_parms.samples_per_subframe*4);

  for(int x=0; x<10; x++) {
    for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
      rxp[i] = ((void *)&UE->common_vars.rxdata[i][0]) + 4*x*UE->frame_parms.samples_per_subframe;

    AssertFatal( UE->frame_parms.samples_per_subframe ==
                 UE->rfdevice.trx_read_func(&UE->rfdevice,
                                            timestamp,
                                            rxp,
                                            UE->frame_parms.samples_per_subframe,
                                            UE->frame_parms.nb_antennas_rx), "");
  }

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    free(dummy_tx[i]);
}

void trashFrame(PHY_VARS_NR_UE *UE, openair0_timestamp *timestamp) {
  void *dummy_tx[UE->frame_parms.nb_antennas_tx];

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    dummy_tx[i]=malloc16_clear(UE->frame_parms.samples_per_subframe*4);

  void *dummy_rx[UE->frame_parms.nb_antennas_rx];

  for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
    dummy_rx[i]=malloc16(UE->frame_parms.samples_per_subframe*4);

  for (int sf=0; sf<NR_NUMBER_OF_SUBFRAMES_PER_FRAME; sf++) {
    //      printf("Reading dummy sf %d\n",sf);
    UE->rfdevice.trx_read_func(&UE->rfdevice,
                               timestamp,
                               dummy_rx,
                               UE->frame_parms.samples_per_subframe,
                               UE->frame_parms.nb_antennas_rx);
    usleep(500); // this sleep improves in the case of simulated RF and doesn't harm with true radio
  }

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    free(dummy_tx[i]);

  for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
    free(dummy_rx[i]);
}

void syncInFrame(PHY_VARS_NR_UE *UE, openair0_timestamp *timestamp) {
  if (UE->no_timing_correction==0) {
    LOG_I(PHY,"Resynchronizing RX by %d samples (mode = %d)\n",UE->rx_offset,UE->mode);
    void *dummy_tx[UE->frame_parms.nb_antennas_tx];

    for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
      dummy_tx[i]=malloc16_clear(UE->frame_parms.samples_per_subframe*4);

    for ( int size=UE->rx_offset ; size > 0 ; size -= UE->frame_parms.samples_per_subframe ) {
      int unitTransfer=size>UE->frame_parms.samples_per_subframe ? UE->frame_parms.samples_per_subframe : size ;
      AssertFatal(unitTransfer ==
                  UE->rfdevice.trx_read_func(&UE->rfdevice,
                                             timestamp,
                                             (void **)UE->common_vars.rxdata,
                                             unitTransfer,
                                             UE->frame_parms.nb_antennas_rx),"");
    }

    for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
      free(dummy_tx[i]);
  }
}

int computeSamplesShift(PHY_VARS_NR_UE *UE) {
  if ( getenv("RFSIMULATOR") != 0) {
    LOG_E(PHY,"SET rx_offset %d \n",UE->rx_offset);
    //UE->rx_offset_diff=0;
    return 0;
  }

  // compute TO compensation that should be applied for this frame
  if ( UE->rx_offset < 5*UE->frame_parms.samples_per_slot  &&
       UE->rx_offset > 0 )
    return -1 ;

  if ( UE->rx_offset > 5*UE->frame_parms.samples_per_slot &&
       UE->rx_offset < 10*UE->frame_parms.samples_per_slot )
    return 1;

  return 0;
}

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
  PHY_VARS_NR_UE *UE = (PHY_VARS_NR_UE *) arg;
  //  int tx_enabled = 0;
  openair0_timestamp timestamp;
  void *rxp[NB_ANTENNAS_RX], *txp[NB_ANTENNAS_TX];
  int start_rx_stream = 0;
  int i;
  char threadname[128];
  int th_id;
  const uint16_t table_sf_slot[20] = {0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9};

  for (int i=0; i<  RX_NB_TH_MAX; i++ )
    UE->proc.proc_rxtx[i].counter_decoder = 0;

  static uint8_t thread_idx = 0;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);

  if ( threads.main != -1 )
    CPU_SET(threads.main, &cpuset);

  sprintf(threadname, "Main UE %d", UE->Mod_id);
  init_thread(100000, 500000, FIFO_PRIORITY, &cpuset,threadname);

  if ((oaisim_flag == 0) && (UE->mode !=loop_through_memory))
    AssertFatal(0== openair0_device_load(&(UE->rfdevice), &openair0_cfg[0]), "");

  UE->rfdevice.host_type = RAU_HOST;
  init_UE_threads(UE);
#ifdef NAS_UE
  //MessageDef *message_p;
  //message_p = itti_alloc_new_message(TASK_NAS_UE, INITIALIZE_MESSAGE);
  //itti_send_msg_to_task (TASK_NAS_UE, UE->Mod_id + NB_eNB_INST, message_p);
#endif
  int nb_slot_frame = 10*UE->frame_parms.slots_per_subframe;
  int slot_nr=-1;

  //int cumulated_shift=0;
  if ((oaisim_flag == 0) && (UE->mode != loop_through_memory))
    AssertFatal(UE->rfdevice.trx_start_func(&UE->rfdevice) == 0, "Could not start the device\n");

  while (!oai_exit) {
    AssertFatal ( 0== pthread_mutex_lock(&UE->proc.mutex_synch), "");
    int instance_cnt_synch = UE->proc.instance_cnt_synch;
    int is_synchronized    = UE->is_synchronized;
    AssertFatal ( 0== pthread_mutex_unlock(&UE->proc.mutex_synch), "");

    if (is_synchronized == 0) {
#if BASIC_SIMULATOR

      while (!((instance_cnt_synch = UE->proc.instance_cnt_synch) < 0)) {
        printf("ue sync not ready\n");
        usleep(500*1000);
      }

#endif

      if (UE->mode != loop_through_memory) {
        if (instance_cnt_synch < 0) {  // we can invoke the synch
          // grab 10 ms of signal and wakeup synch thread
          readFrame(UE, &timestamp);
          AssertFatal( 0 == pthread_mutex_lock(&UE->proc.mutex_synch), "");
          AssertFatal( 0 == ++UE->proc.instance_cnt_synch, "[SCHED][UE] UE sync thread busy!!\n" );
          AssertFatal( 0 == pthread_cond_signal(&UE->proc.cond_synch), "");
          AssertFatal( 0 == pthread_mutex_unlock(&UE->proc.mutex_synch), "");
        } else {
          // grab 10 ms of signal into dummy buffer to wait result of sync detection
          trashFrame(UE, &timestamp);
        }
      }

      continue;
    }

    if (start_rx_stream==0) {
      start_rx_stream=1;

      if (UE->mode != loop_through_memory) {
        syncInFrame(UE, &timestamp);
        UE->rx_offset=0;
        UE->time_sync_cell=0;
        UE->proc.proc_rxtx[0].frame_rx++;

        //UE->proc.proc_rxtx[1].frame_rx++;
        for (th_id=1; th_id < RX_NB_TH; th_id++) {
          UE->proc.proc_rxtx[th_id].frame_rx = UE->proc.proc_rxtx[0].frame_rx;
        }

        //printf("first stream frame rx %d\n",UE->proc.proc_rxtx[0].frame_rx);
        // read in first symbol
        AssertFatal (UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0 ==
                     UE->rfdevice.trx_read_func(&UE->rfdevice,
                                                &timestamp,
                                                (void **)UE->common_vars.rxdata,
                                                UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0,
                                                UE->frame_parms.nb_antennas_rx),"");
        //write_output("txdata_sym.m", "txdata_sym", UE->common_vars.rxdata[0], (UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0), 1, 1);
        //nr_slot_fep(UE,0, 0, 0, 1, NR_PDCCH_EST);
      } //UE->mode != loop_through_memory
      else
        rt_sleep_ns(1000*1000);

      continue;
    }

    thread_idx++;
    thread_idx%=RX_NB_TH;
    //printf("slot_nr %d nb slot frame %d\n",slot_nr, nb_slot_frame);
    slot_nr++;
    slot_nr %= nb_slot_frame;
    UE_nr_rxtx_proc_t *proc = &UE->proc.proc_rxtx[thread_idx];
    // update thread index for received subframe
    UE->current_thread_id[slot_nr] = thread_idx;
#if BASIC_SIMULATOR

    for (int t = 0; t < RX_NB_TH; t++) {
      UE_rxtx_proc_t *proc = &UE->proc.proc_rxtx[t];
      pthread_mutex_lock(&proc->mutex_rxtx);

      while (proc->instance_cnt_rxtx >= 0) pthread_cond_wait( &proc->cond_rxtx, &proc->mutex_rxtx );

      pthread_mutex_unlock(&proc->mutex_rxtx);
    }

#endif
    LOG_D(PHY,"Process slot %d thread Idx %d \n", slot_nr, UE->current_thread_id[slot_nr]);
    proc->nr_tti_rx=slot_nr;
    proc->subframe_rx=table_sf_slot[slot_nr];
    proc->frame_tx = proc->frame_rx;
    proc->nr_tti_tx= slot_nr + DURATION_RX_TO_TX;

    if (proc->nr_tti_tx > nb_slot_frame) {
      proc->frame_tx = (proc->frame_tx + 1)%MAX_FRAME_NUMBER;
      proc->nr_tti_tx %= nb_slot_frame;
    }

    if(slot_nr == 0) {
      UE->proc.proc_rxtx[0].frame_rx++;

      //UE->proc.proc_rxtx[1].frame_rx++;
      for (th_id=1; th_id < RX_NB_TH; th_id++) {
        UE->proc.proc_rxtx[th_id].frame_rx = UE->proc.proc_rxtx[0].frame_rx;
      }
    }

    if (UE->mode != loop_through_memory) {
      for (i=0; i<UE->frame_parms.nb_antennas_rx; i++)
        rxp[i] = (void *)&UE->common_vars.rxdata[i][UE->frame_parms.ofdm_symbol_size+
                 UE->frame_parms.nb_prefix_samples0+
                 slot_nr*UE->frame_parms.samples_per_slot];

      for (i=0; i<UE->frame_parms.nb_antennas_tx; i++)
        txp[i] = (void *)&UE->common_vars.txdata[i][((slot_nr+2)%NR_NUMBER_OF_SUBFRAMES_PER_FRAME)*UE->frame_parms.samples_per_slot];

      int readBlockSize, writeBlockSize;

      if (slot_nr<(nb_slot_frame - 1)) {
        readBlockSize=UE->frame_parms.samples_per_slot;
        writeBlockSize=UE->frame_parms.samples_per_slot;
      } else {
        UE->rx_offset_diff = computeSamplesShift(UE);
        readBlockSize=UE->frame_parms.samples_per_slot -
                      UE->frame_parms.ofdm_symbol_size -
                      UE->frame_parms.nb_prefix_samples0 -
                      UE->rx_offset_diff;
        writeBlockSize=UE->frame_parms.samples_per_slot -
                       UE->rx_offset_diff;
      }

      AssertFatal(readBlockSize ==
                  UE->rfdevice.trx_read_func(&UE->rfdevice,
                                             &timestamp,
                                             rxp,
                                             readBlockSize,
                                             UE->frame_parms.nb_antennas_rx),"");
      AssertFatal( writeBlockSize ==
                   UE->rfdevice.trx_write_func(&UE->rfdevice,
                       timestamp+
                       (2*UE->frame_parms.samples_per_slot) -
                       UE->frame_parms.ofdm_symbol_size-UE->frame_parms.nb_prefix_samples0 -
                       openair0_cfg[0].tx_sample_advance,
                       txp,
                       writeBlockSize,
                       UE->frame_parms.nb_antennas_tx,
                       1),"");

      if( slot_nr==(nb_slot_frame-1)) {
        // read in first symbol of next frame and adjust for timing drift
        int first_symbols=writeBlockSize-readBlockSize;

        if ( first_symbols > 0 )
          AssertFatal(first_symbols ==
                      UE->rfdevice.trx_read_func(&UE->rfdevice,
                                                 &timestamp,
                                                 (void **)UE->common_vars.rxdata,
                                                 first_symbols,
                                                 UE->frame_parms.nb_antennas_rx),"");
        else
          LOG_E(PHY,"can't compensate: diff =%d\n", first_symbols);
      }

      pickTime(gotIQs);
      // operate on thread sf mod 2
      AssertFatal(pthread_mutex_lock(&proc->mutex_rxtx) ==0,"");
#ifdef SAIF_ENABLED

      if (!(proc->frame_rx%4000)) {
        printf("frame_rx=%d rx_thread_busy=%ld - rate %8.3f\n",
               proc->frame_rx, g_ue_rx_thread_busy,
               (float)g_ue_rx_thread_busy/(proc->frame_rx*10+1)*100.0);
        fflush(stdout);
      }

#endif

      //UE->proc.proc_rxtx[0].gotIQs=readTime(gotIQs);
      //UE->proc.proc_rxtx[1].gotIQs=readTime(gotIQs);
      for (th_id=0; th_id < RX_NB_TH; th_id++) {
        UE->proc.proc_rxtx[th_id].gotIQs=readTime(gotIQs);
      }

      proc->subframe_tx=proc->nr_tti_rx;
      proc->timestamp_tx = timestamp+
                           (DURATION_RX_TO_TX*UE->frame_parms.samples_per_slot)-
                           UE->frame_parms.ofdm_symbol_size-UE->frame_parms.nb_prefix_samples0;
      proc->instance_cnt_rxtx++;
      LOG_D( PHY, "[SCHED][UE %d] UE RX instance_cnt_rxtx %d subframe %d !!\n", UE->Mod_id, proc->instance_cnt_rxtx,proc->subframe_rx);

      if (proc->instance_cnt_rxtx != 0) {
#ifdef SAIF_ENABLED
        g_ue_rx_thread_busy++;
#endif

        if ( getenv("RFSIMULATOR") != NULL ) {
          do {
            AssertFatal (pthread_mutex_unlock(&proc->mutex_rxtx) == 0, "");
            usleep(100);
            AssertFatal (pthread_mutex_lock(&proc->mutex_rxtx) == 0, "");
          } while ( proc->instance_cnt_rxtx >= 0);
        } else
          LOG_E( PHY, "[SCHED][UE %d] !! UE RX thread busy (IC %d)!!\n", UE->Mod_id, proc->instance_cnt_rxtx);

        AssertFatal( proc->instance_cnt_rxtx <= 4, "[SCHED][UE %d] !!! UE instance_cnt_rxtx > 2 (IC %d) (Proc %d)!!",
                     UE->Mod_id, proc->instance_cnt_rxtx,
                     UE->current_thread_id[slot_nr]);
      }

      AssertFatal (pthread_cond_signal(&proc->cond_rxtx) ==0,"");
      AssertFatal (pthread_mutex_unlock(&proc->mutex_rxtx) ==0,"");
      //                    initRefTimes(t1);
      //                    initStaticTime(lastTime);
      //                    updateTimes(lastTime, &t1, 20000, "Delay between two IQ acquisitions (case 1)");
      //                    pickStaticTime(lastTime);
    } //UE->mode != loop_through_memory
    else {
      processSlotRX(UE,proc);
      getchar();
    } // else loop_through_memory
  } // while !oai_exit

  return NULL;
}

/*!
 * \brief Initialize the UE theads.
 * Creates the UE threads:
 * - UE_thread_rxtx0
 * - UE_thread_rxtx1
 * - UE_thread_synch
 * - UE_thread_fep_slot0
 * - UE_thread_fep_slot1
 * - UE_thread_dlsch_proc_slot0
 * - UE_thread_dlsch_proc_slot1
 * and the locking between them.
 */
void init_UE_threads(PHY_VARS_NR_UE *UE) {
  struct nr_rxtx_thread_data *rtd;
  pthread_attr_init (&UE->proc.attr_ue);
  pthread_attr_setstacksize(&UE->proc.attr_ue,8192);//5*PTHREAD_STACK_MIN);
  pthread_mutex_init(&UE->proc.mutex_synch,NULL);
  pthread_cond_init(&UE->proc.cond_synch,NULL);
  UE->proc.instance_cnt_synch = -1;

  // the threads are not yet active, therefore access is allowed without locking
  for (int i=0; i<RX_NB_TH; i++) {
    rtd = calloc(1, sizeof(struct nr_rxtx_thread_data));

    if (rtd == NULL) abort();

    rtd->UE = UE;
    rtd->proc = &UE->proc.proc_rxtx[i];
    pthread_mutex_init(&UE->proc.proc_rxtx[i].mutex_rxtx,NULL);
    pthread_cond_init(&UE->proc.proc_rxtx[i].cond_rxtx,NULL);
    UE->proc.proc_rxtx[i].sub_frame_start=i;
    UE->proc.proc_rxtx[i].sub_frame_step=RX_NB_TH;
    printf("Init_UE_threads rtd %d proc %d nb_threads %d i %d\n",rtd->proc->sub_frame_start, UE->proc.proc_rxtx[i].sub_frame_start,RX_NB_TH, i);
    pthread_create(&UE->proc.proc_rxtx[i].pthread_rxtx, NULL, UE_thread_rxn_txnp4, rtd);
#ifdef UE_DLSCH_PARALLELISATION
    pthread_mutex_init(&UE->proc.proc_rxtx[i].mutex_dlsch_td,NULL);
    pthread_cond_init(&UE->proc.proc_rxtx[i].cond_dlsch_td,NULL);
    pthread_create(&UE->proc.proc_rxtx[i].pthread_dlsch_td,NULL,nr_dlsch_decoding_2thread0, rtd);
    //thread 2
    pthread_mutex_init(&UE->proc.proc_rxtx[i].mutex_dlsch_td1,NULL);
    pthread_cond_init(&UE->proc.proc_rxtx[i].cond_dlsch_td1,NULL);
    pthread_create(&UE->proc.proc_rxtx[i].pthread_dlsch_td1,NULL,nr_dlsch_decoding_2thread1, rtd);
#endif
#ifdef UE_SLOT_PARALLELISATION
    //pthread_mutex_init(&UE->proc.proc_rxtx[i].mutex_slot0_dl_processing,NULL);
    //pthread_cond_init(&UE->proc.proc_rxtx[i].cond_slot0_dl_processing,NULL);
    //pthread_create(&UE->proc.proc_rxtx[i].pthread_slot0_dl_processing,NULL,UE_thread_slot0_dl_processing, rtd);
    pthread_mutex_init(&UE->proc.proc_rxtx[i].mutex_slot1_dl_processing,NULL);
    pthread_cond_init(&UE->proc.proc_rxtx[i].cond_slot1_dl_processing,NULL);
    pthread_create(&UE->proc.proc_rxtx[i].pthread_slot1_dl_processing,NULL,UE_thread_slot1_dl_processing, rtd);
#endif
  }

  pthread_create(&UE->proc.pthread_synch,NULL,UE_thread_synch,(void *)UE);
}


#ifdef OPENAIR2
  /*
  void fill_ue_band_info(void) {

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
  }*/
#endif

/*
int setup_ue_buffers(PHY_VARS_NR_UE **phy_vars_ue, openair0_config_t *openair0_cfg) {

    int i, CC_id;
    NR_DL_FRAME_PARMS *frame_parms;
    openair0_rf_map *rf_map;

    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        rf_map = &phy_vars_ue[CC_id]->rf_map;

        AssertFatal( phy_vars_ue[CC_id] !=0, "");
        frame_parms = &(phy_vars_ue[CC_id]->frame_parms);

        // replace RX signal buffers with mmaped HW versions
        rxdata = (int32_t**)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t*) );
        txdata = (int32_t**)malloc16( frame_parms->nb_antennas_tx*sizeof(int32_t*) );

        for (i=0; i<frame_parms->nb_antennas_rx; i++) {
            LOG_I(PHY, "Mapping UE CC_id %d, rx_ant %d, freq %u on card %d, chain %d\n",
                  CC_id, i, downlink_frequency[CC_id][i], rf_map->card, rf_map->chain+i );
            free( phy_vars_ue[CC_id]->common_vars.rxdata[i] );
            rxdata[i] = (int32_t*)malloc16_clear( 307200*sizeof(int32_t) );
            phy_vars_ue[CC_id]->common_vars.rxdata[i] = rxdata[i]; // what about the "-N_TA_offset" ? // N_TA offset for TDD
        }

        for (i=0; i<frame_parms->nb_antennas_tx; i++) {
            LOG_I(PHY, "Mapping UE CC_id %d, tx_ant %d, freq %u on card %d, chain %d\n",
                  CC_id, i, downlink_frequency[CC_id][i], rf_map->card, rf_map->chain+i );
            free( phy_vars_ue[CC_id]->common_vars.txdata[i] );
            txdata[i] = (int32_t*)malloc16_clear( 307200*sizeof(int32_t) );
            phy_vars_ue[CC_id]->common_vars.txdata[i] = txdata[i];
        }

        // rxdata[x] points now to the same memory region as phy_vars_ue[CC_id]->common_vars.rxdata[x]
        // txdata[x] points now to the same memory region as phy_vars_ue[CC_id]->common_vars.txdata[x]
        // be careful when releasing memory!
        // because no "release_ue_buffers"-function is available, at least rxdata and txdata memory will leak (only some bytes)
    }
    return 0;
}
*/
