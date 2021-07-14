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
#include "lte-softmodem.h"

#include "rt_wrapper.h"
#include "system.h"

#include "LAYER2/MAC/mac.h"
#include "RRC/LTE/rrc_extern.h"
#include "PHY_INTERFACE/phy_stub_UE.h"
#include "PHY_INTERFACE/phy_interface_extern.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"

#undef MALLOC //there are two conflicting definitions, so we better make sure we don't use it at all
//#undef FRAME_LENGTH_COMPLEX_SAMPLES //there are two conflicting definitions, so we better make sure we don't use it at all

#include "PHY/phy_extern_ue.h"
#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/MAC/mac_proto.h"
#include "SCHED_UE/sched_UE.h"
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"

#include <inttypes.h>

#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "UTIL/OTG/otg_tx.h"
#include "UTIL/OTG/otg_externs.h"
#include "UTIL/MATH/oml.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "lte-softmodem.h"
#include "common/config/config_userapi.h"
#include "T.h"

extern double cpuf;


#define FRAME_PERIOD    100000000ULL
#define DAQ_PERIOD      66667ULL
#define FIFO_PRIORITY   40

typedef enum {
  pss=0,
  pbch=1,
  si=2
} sync_mode_t;

void init_UE_threads(int);
void init_UE_threads_stub(int);
void init_UE_single_thread_stub(int);
void *UE_thread(void *arg);
void init_UE(int nb_inst,int eMBMS_active, int uecap_xer_in, int timing_correction, int phy_test, int UE_scan, int UE_scan_carrier, runmode_t mode,int rxgain,int txpowermax,LTE_DL_FRAME_PARMS *fp);
void init_UE_stub(int nb_inst,int,int,char *);
void init_UE_stub_single_thread(int nb_inst,int,int,char *);
int init_timer_thread(void);
extern void oai_subframe_ind(uint16_t sfn, uint16_t sf);
extern void multicast_link_start(void (*rx_handlerP) (unsigned int, char *),
                                 unsigned char _multicast_group, char *multicast_ifname);
extern int oai_nfapi_crc_indication(nfapi_crc_indication_t *crc_ind);
extern int oai_nfapi_cqi_indication(nfapi_cqi_indication_t *cqi_ind);
extern int oai_nfapi_harq_indication(nfapi_harq_indication_t *harq_ind);
extern int oai_nfapi_sr_indication(nfapi_sr_indication_t *ind);
extern int oai_nfapi_rx_ind(nfapi_rx_indication_t *ind);
extern int multicast_link_write_sock(int groupP, char *dataP, uint32_t sizeP);


int	tx_req_num_elems;
extern uint16_t sf_ahead;
//extern int tx_req_UE_MAC1();

void ue_stub_rx_handler(unsigned int, char *);

int32_t **rxdata;
int32_t **txdata;

int timer_subframe = 0;
int timer_frame = 0;
SF_ticking *phy_stub_ticking = NULL;
int next_ra_frame = 0;
module_id_t next_Mod_id = 0;

#define KHz (1000UL)
#define MHz (1000*KHz)

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


threads_t threads= {-1,-1,-1,-1,-1,-1,-1,-1};

pthread_t                       main_ue_thread;
pthread_attr_t                  attr_UE_thread;
struct sched_param              sched_param_UE_thread;


void get_uethreads_params(void) {
  paramdef_t cmdline_threadsparams[] =CMDLINE_UETHREADSPARAMS_DESC;
  config_process_cmdline( cmdline_threadsparams,sizeof(cmdline_threadsparams)/sizeof(paramdef_t),NULL);
}


void phy_init_lte_ue_transport(PHY_VARS_UE *ue,int absraction_flag);

PHY_VARS_UE *init_ue_vars(LTE_DL_FRAME_PARMS *frame_parms,
                          uint8_t UE_id,
                          uint8_t abstraction_flag)

{
  PHY_VARS_UE *ue = (PHY_VARS_UE *)calloc(1,sizeof(PHY_VARS_UE));
  AssertFatal(ue,"");
  if (frame_parms!=(LTE_DL_FRAME_PARMS *)NULL) { // if we want to give initial frame parms, allocate the PHY_VARS_UE structure and put them in
    memcpy(&(ue->frame_parms), frame_parms, sizeof(LTE_DL_FRAME_PARMS));
  }

  ue->hw_timing_advance=get_softmodem_params()->hw_timing_advance;
  ue->Mod_id      = UE_id;
  ue->mac_enabled = 1;

  // In phy_stub_UE (MAC-to-MAC) mode these init functions don't need to get called. Is this correct?
  if (NFAPI_MODE!=NFAPI_UE_STUB_PNF) {
    // initialize all signal buffers
    init_lte_ue_signal(ue,1,abstraction_flag);
    // intialize transport
    init_lte_ue_transport(ue,abstraction_flag);
  }

  return(ue);
}


char uecap_xer[1024];



void init_thread(int sched_runtime,
                 int sched_deadline,
                 int sched_fifo,
                 cpu_set_t *cpuset,
                 char *name)
{
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
  int settingPriority = 1;

  if (checkIfFedoraDistribution())
    if (checkIfGenericKernelOnFedora())
      if (checkIfInsideContainer())
        settingPriority = 0;

  if (settingPriority) {
    if (CPU_COUNT(cpuset) > 0)
      AssertFatal( 0 == pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), cpuset), "");

    struct sched_param sp;
    sp.sched_priority = sched_fifo;
    AssertFatal(pthread_setschedparam(pthread_self(),SCHED_FIFO,&sp)==0,
                "Can't set thread priority, Are you root?\n");
  }

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
}

void init_UE(int nb_inst,
             int eMBMS_active,
             int uecap_xer_in,
             int timing_correction,
             int phy_test,
             int UE_scan,
             int UE_scan_carrier,
             runmode_t mode,
             int rxgain,
             int txpowermax,
             LTE_DL_FRAME_PARMS *fp0)
{
  PHY_VARS_UE *UE;
  int         inst;
  int         ret;
  LTE_DL_FRAME_PARMS *fp;
  LOG_I(PHY,"UE : Calling Layer 2 for initialization\n");
  l2_init_ue(eMBMS_active,(uecap_xer_in==1)?uecap_xer:NULL,
             0,// cba_group_active
             0); // HO flag

  if (PHY_vars_UE_g==NULL) PHY_vars_UE_g = (PHY_VARS_UE ***)calloc(1+nb_inst,sizeof(PHY_VARS_UE **));

  for (inst=0; inst<nb_inst; inst++) {
    if (PHY_vars_UE_g[inst]==NULL) PHY_vars_UE_g[inst] = (PHY_VARS_UE **)calloc(1+MAX_NUM_CCs,sizeof(PHY_VARS_UE *));

    LOG_I(PHY,"Allocating UE context %d\n",inst);

    PHY_vars_UE_g[inst][0] = init_ue_vars(fp0,inst,0);

    // turn off timing control loop in UE
    PHY_vars_UE_g[inst][0]->no_timing_correction = timing_correction;
    UE = PHY_vars_UE_g[inst][0];
    fp = &UE->frame_parms;
    printf("PHY_vars_UE_g[0][0] = %p\n",UE);

    if (phy_test==1)
      UE->mac_enabled = 0;
    else
      UE->mac_enabled = 1;

    if (UE->mac_enabled == 0) {  //set default UL parameters for testing mode
      for (int i=0; i<NUMBER_OF_CONNECTED_eNB_MAX; i++) {
        UE->pusch_config_dedicated[i].betaOffset_ACK_Index = 0;
        UE->pusch_config_dedicated[i].betaOffset_RI_Index  = 0;
        UE->pusch_config_dedicated[i].betaOffset_CQI_Index = 2;
        UE->scheduling_request_config[i].sr_PUCCH_ResourceIndex = 0;
        UE->scheduling_request_config[i].sr_ConfigIndex = 7+(0%3);
        UE->scheduling_request_config[i].dsr_TransMax = sr_n4;
      }
    }

    UE->UE_scan = UE_scan;
    UE->UE_scan_carrier = UE_scan_carrier;
    UE->mode    = mode;
    printf("UE->mode = %d\n",mode);

    if (UE->mac_enabled == 1) {
      UE->pdcch_vars[0][0]->crnti = 0x1234;
      UE->pdcch_vars[1][0]->crnti = 0x1234;
    } else {
      UE->pdcch_vars[0][0]->crnti = 0x1235;
      UE->pdcch_vars[1][0]->crnti = 0x1235;
    }

    UE->rx_total_gain_dB =  rxgain;
    UE->tx_power_max_dBm = txpowermax;
    UE->frame_parms.nb_antennas_tx = fp0->nb_antennas_tx;
    UE->frame_parms.nb_antennas_rx = fp0->nb_antennas_rx;

    if (fp->frame_type == TDD) {
      switch (fp->N_RB_DL) {
        case 100:
          if (fp->threequarter_fs) UE->N_TA_offset = (624*3)/4;
          else                              UE->N_TA_offset = 624;

          break;

        case 75:
          UE->N_TA_offset = (624*3)/4;
          break;

        case 50:
          UE->N_TA_offset = 624/2;
          break;

        case 25:
          UE->N_TA_offset = 624/4;
          break;

        case 15:
          UE->N_TA_offset = 624/8;
          break;

        case 6:
          UE->N_TA_offset = 624/16;
          break;

        default:
          AssertFatal(1==0,"illegal N_RB_DL %d\n",fp->N_RB_DL);
          break;
      }
    } else UE->N_TA_offset = 0;

    if( IS_SOFTMODEM_BASICSIM)
      /* this is required for the basic simulator in TDD mode
       * TODO: find a proper cleaner solution
       */
      UE->N_TA_offset = 0;

    LOG_I(PHY,"Intializing UE Threads for instance %d (%p,%p)...\n",inst,PHY_vars_UE_g[inst],PHY_vars_UE_g[inst][0]);
    init_UE_threads(inst);

    ret = openair0_device_load(&(UE->rfdevice), &openair0_cfg[0]);

    if (ret !=0) {
      exit_fun("Error loading device library");
    }

    UE->rfdevice.host_type = RAU_HOST;
    //    UE->rfdevice.type      = NONE_DEV;
    AssertFatal(0 == pthread_create(&UE->proc.pthread_ue,
                                    &UE->proc.attr_ue,
                                    UE_thread,
                                    (void *)UE), "");
  }

  printf("UE threads created by %ld\n", gettid());
}

// Initiating all UEs within a single set of threads for PHY_STUB. Future extensions -> multiple
// set of threads for multiple UEs.
void init_UE_stub_single_thread(int nb_inst,
                                int eMBMS_active,
                                int uecap_xer_in,
                                char *emul_iface)
{
  int         inst;
  LOG_I(PHY,"UE : Calling Layer 2 for initialization, nb_inst: %d \n", nb_inst);
  l2_init_ue(eMBMS_active,(uecap_xer_in==1)?uecap_xer:NULL,
             0,// cba_group_active
             0); // HO flag

  for (inst=0; inst<nb_inst; inst++) {
    LOG_I(PHY,"Initializing memory for UE instance %d (%p)\n",inst,PHY_vars_UE_g[inst]);
    // PHY_vars_UE_g[inst][0] = init_ue_vars(NULL,inst,0);
  }

  init_timer_thread();
  init_UE_single_thread_stub(nb_inst);
  printf("UE threads created \n");
  LOG_I(PHY,"Starting multicast link on %s\n",emul_iface);

  if(NFAPI_MODE!=NFAPI_UE_STUB_PNF)
    multicast_link_start(ue_stub_rx_handler,0,emul_iface);
}


void init_UE_stub(int nb_inst,
                  int eMBMS_active,
                  int uecap_xer_in,
                  char *emul_iface)
{
  int         inst;
  LOG_I(PHY,"UE : Calling Layer 2 for initialization\n");
  l2_init_ue(eMBMS_active,(uecap_xer_in==1)?uecap_xer:NULL,
             0,// cba_group_active
             0); // HO flag

  for (inst=0; inst<nb_inst; inst++) {
    LOG_I(PHY,"Initializing memory for UE instance %d (%p)\n",inst,PHY_vars_UE_g[inst]);
    PHY_vars_UE_g[inst][0] = init_ue_vars(NULL,inst,0);
  }

  init_timer_thread();

  for (inst=0; inst<nb_inst; inst++) {
    LOG_I(PHY,"Intializing UE Threads for instance %d (%p,%p)...\n",inst,PHY_vars_UE_g[inst],PHY_vars_UE_g[inst][0]);
    init_UE_threads_stub(inst);
  }

  printf("UE threads created \n");
  LOG_I(PHY,"Starting multicast link on %s\n",emul_iface);

  if(NFAPI_MODE!=NFAPI_UE_STUB_PNF)
    multicast_link_start(ue_stub_rx_handler,0,emul_iface);
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
  int i ;
  PHY_VARS_UE *UE = (PHY_VARS_UE *) arg;
  int current_band = 0;
  int current_offset = 0;
  sync_mode_t sync_mode = pbch;
  int CC_id = UE->CC_id;
  int ind;
  int found;
  int freq_offset=0;
  char threadname[128];
  printf("UE_thread_sync in with PHY_vars_UE %p\n",arg);
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  if ( threads.sync != -1 )
    CPU_SET(threads.sync, &cpuset);

  // this thread priority must be lower that the main acquisition thread
  sprintf(threadname, "sync UE %d\n", UE->Mod_id);
  init_thread(100000, 500000, FIFO_PRIORITY-1, &cpuset, threadname);
  printf("starting UE synch thread (IC %d)\n",UE->proc.instance_cnt_synch);
  ind = 0;
  found = 0;

  if (UE->UE_scan == 0) {
    do  {
      current_band = eutra_bands[ind].band;
      printf( "Scanning band %d, dl_min %"PRIu32", ul_min %"PRIu32"\n", current_band, eutra_bands[ind].dl_min,eutra_bands[ind].ul_min);

      if ((eutra_bands[ind].dl_min <= UE->frame_parms.dl_CarrierFreq) && (eutra_bands[ind].dl_max >= UE->frame_parms.dl_CarrierFreq)) {
        for (i=0; i<4; i++)
          uplink_frequency_offset[CC_id][i] = eutra_bands[ind].ul_min - eutra_bands[ind].dl_min;

        found = 1;
        break;
      }

      ind++;
    } while (ind < sizeof(eutra_bands) / sizeof(eutra_bands[0]));

    if (found == 0) {
      LOG_E(PHY,"Can't find EUTRA band for frequency %d",UE->frame_parms.dl_CarrierFreq);
      exit_fun("Can't find EUTRA band for frequency");
      return &UE_thread_synch_retval;
    }

    LOG_I( PHY, "[SCHED][UE] Check absolute frequency DL %"PRIu32", UL %"PRIu32" (oai_exit %d, rx_num_channels %d)\n", UE->frame_parms.dl_CarrierFreq, UE->frame_parms.ul_CarrierFreq,oai_exit,
           openair0_cfg[0].rx_num_channels);

    for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
      openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = UE->frame_parms.dl_CarrierFreq;
      openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] = UE->frame_parms.ul_CarrierFreq;
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
      uplink_frequency_offset[UE->rf_map.card][UE->rf_map.chain+i] =
        bands_to_scan.band_info[CC_id].ul_min-bands_to_scan.band_info[CC_id].dl_min;
      openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
      openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] =
        downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
      openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;
    }
  }

  /*
    while (sync_var<0)
      pthread_cond_wait(&sync_cond, &sync_mutex);
    pthread_mutex_unlock(&sync_mutex);
  */
  wait_sync("UE_thread_sync");
  printf("Started device, unlocked sync_mutex (UE_sync_thread)\n");

  while (oai_exit==0) {
    AssertFatal ( 0== pthread_mutex_lock(&UE->proc.mutex_synch), "");

    while (UE->proc.instance_cnt_synch < 0)
      // the thread waits here most of the time
      pthread_cond_wait( &UE->proc.cond_synch, &UE->proc.mutex_synch );

    AssertFatal ( 0== pthread_mutex_unlock(&UE->proc.mutex_synch), "");

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
        LOG_I(PHY, "[UE thread Synch] Running Initial Synch (mode %d)\n",UE->mode);

        if (initial_sync( UE, UE->mode ) == 0) {
          LOG_I( HW, "Got synch: hw_slot_offset %d, carrier off %d Hz, rxgain %d (DL %lu, UL %lu), UE_scan_carrier %d\n",
                 (UE->rx_offset<<1) / UE->frame_parms.samples_per_tti,
                 freq_offset,
                 UE->rx_total_gain_dB,
                 downlink_frequency[0][0]+freq_offset,
                 downlink_frequency[0][0]+uplink_frequency_offset[0][0]+freq_offset,
                 UE->UE_scan_carrier );

          // rerun with new cell parameters and frequency-offset
          for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
            openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;

            if (UE->UE_scan_carrier == 1) {
              if (freq_offset >= 0)
                openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] += abs(UE->common_vars.freq_offset);
              else
                openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] -= abs(UE->common_vars.freq_offset);

              openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] =
                openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i]+uplink_frequency_offset[CC_id][i];
              downlink_frequency[CC_id][i] = openair0_cfg[CC_id].rx_freq[i];
              freq_offset=0;
            }
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
          //UE->rfdevice.trx_stop_func(&UE->rfdevice);
          sleep(1);
          init_frame_parms(&UE->frame_parms,1);

          /*if (UE->rfdevice.trx_start_func(&UE->rfdevice) != 0 ) {
            LOG_E(HW,"Could not start the device\n");
            oai_exit=1;
            }*/

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
                         10*UE->frame_parms.samples_per_tti,
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
                       10*UE->frame_parms.samples_per_tti,
                       fd);
                LOG_I(PHY,"Dummping Frame ... bye bye \n");
                fclose(fd);
                exit(0);
              }

              AssertFatal(1==0,"No cell synchronization found, abandoning");
              return &UE_thread_synch_retval; // not reached
            }
          }

          LOG_I(PHY, "[initial_sync] trying carrier off %d Hz, rxgain %d (DL %lu, UL %lu)\n",
                freq_offset,
                UE->rx_total_gain_dB,
                downlink_frequency[0][0]+freq_offset,
                downlink_frequency[0][0]+uplink_frequency_offset[0][0]+freq_offset );

          for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
            openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+freq_offset;
            openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i]+freq_offset;
            openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;//-USRP_GAIN_OFFSET;

            if (UE->UE_scan_carrier==1)
              openair0_cfg[UE->rf_map.card].autocal[UE->rf_map.chain+i] = 1;
          }

          UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0],0);
        }// initial_sync=0

        break;

      case si:
      default:
        break;
    }

    AssertFatal ( 0== pthread_mutex_lock(&UE->proc.mutex_synch), "");
    // indicate readiness
    UE->proc.instance_cnt_synch--;
    AssertFatal ( 0== pthread_mutex_unlock(&UE->proc.mutex_synch), "");
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
const char *get_connectionloss_errstr(int errcode)
{
  switch (errcode) {
    case CONNECTION_LOST:
      return "RRC Connection lost, returning to PRACH";

    case PHY_RESYNCH:
      return "RRC Connection lost, trying to resynch";

    case RESYNCH:
      return "return to PRACH and perform a contention-free access";
  };

  return "UNKNOWN RETURN CODE";
}

static void *UE_thread_rxn_txnp4(void *arg)
{
  static __thread int UE_thread_rxtx_retval;
  struct rx_tx_thread_data *rtd = arg;
  UE_rxtx_proc_t *proc = rtd->proc;
  PHY_VARS_UE    *UE   = rtd->UE;
  proc->subframe_rx=proc->sub_frame_start;
  char threadname[256];
  sprintf(threadname,"UE_%d_proc_%d", UE->Mod_id, proc->sub_frame_start);
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);

  if ( (proc->sub_frame_start+1)%RX_NB_TH == 0 && threads.one != -1 )
    CPU_SET(threads.one, &cpuset);

  if ( RX_NB_TH > 1 && (proc->sub_frame_start+1)%RX_NB_TH == 1 && threads.two != -1 )
    CPU_SET(threads.two, &cpuset);

  if ( RX_NB_TH > 2 && (proc->sub_frame_start+1)%RX_NB_TH == 2 && threads.three != -1 )
    CPU_SET(threads.three, &cpuset);

  //CPU_SET(threads.three, &cpuset);
  init_thread(900000,1000000, FIFO_PRIORITY-1, &cpuset,
              threadname);

  while (!oai_exit) {
    if (pthread_mutex_lock(&proc->mutex_rxtx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error locking mutex for UE RXTX\n" );
      exit_fun("nothing to add");
    }

    while (proc->instance_cnt_rxtx < 0) {
      // most of the time, the thread is waiting here
      pthread_cond_wait( &proc->cond_rxtx, &proc->mutex_rxtx );
    }

    //printf("Processing sub frqme %d in %s\n", proc->subframe_rx, threadname);
    initRefTimes(t2);
    initRefTimes(t3);
    pickTime(current);
    updateTimes(proc->gotIQs, &t2, 10000, "Delay to wake up UE_Thread_Rx (case 2)");
    // Process Rx data for one sub-frame
    lte_subframe_t sf_type = subframe_select( &UE->frame_parms, proc->subframe_rx);

    if ((sf_type == SF_DL) ||
        (UE->frame_parms.frame_type == FDD) ||
        (sf_type == SF_S)) {
      if (UE->frame_parms.frame_type == TDD) {
        LOG_D(PHY, "%s,TDD%d,%s: calling UE_RX\n",
              threadname,
              UE->frame_parms.tdd_config,
              (sf_type==SF_DL? "SF_DL" :
               (sf_type==SF_UL? "SF_UL" :
                (sf_type==SF_S ? "SF_S"  : "UNKNOWN_SF_TYPE"))));
      } else {
        LOG_D(PHY, "%s,%s,%s: calling UE_RX\n",
              threadname,
              (UE->frame_parms.frame_type==FDD? "FDD":
               (UE->frame_parms.frame_type==TDD? "TDD":"UNKNOWN_DUPLEX_MODE")),
              (sf_type==SF_DL? "SF_DL" :
               (sf_type==SF_UL? "SF_UL" :
                (sf_type==SF_S ? "SF_S"  : "UNKNOWN_SF_TYPE"))));
      }

#ifdef UE_SLOT_PARALLELISATION
      phy_procedures_slot_parallelization_UE_RX( UE, proc, 0, 0, 1, UE->mode, no_relay, NULL );
#else
      phy_procedures_UE_RX(UE, proc, 0, 0, 1, UE->mode);
#endif
    }

#if UE_TIMING_TRACE
    start_meas(&UE->generic_stat);
#endif

    if (UE->mac_enabled==1) {
      int ret = ue_scheduler(UE->Mod_id,
                             proc->frame_rx,
                             proc->subframe_rx,
                             proc->frame_tx,
                             proc->subframe_tx,
                             subframe_select(&UE->frame_parms,proc->subframe_tx),
                             0,
                             0/*FIXME CC_id*/);

      if ( ret != CONNECTION_OK) {
        LOG_E( PHY, "[UE %"PRIu8"] Frame %"PRIu32", subframe %u %s\n",
               UE->Mod_id, proc->frame_rx, proc->subframe_tx,get_connectionloss_errstr(ret) );
      }
    }

#if UE_TIMING_TRACE
    stop_meas(&UE->generic_stat);
#endif

    // Prepare the future Tx data

    if ((subframe_select( &UE->frame_parms, proc->subframe_tx) == SF_UL) ||
        (UE->frame_parms.frame_type == FDD) )
      if (UE->mode != loop_through_memory)
        phy_procedures_UE_TX(UE,proc,0,0,UE->mode);

    if ((subframe_select( &UE->frame_parms, proc->subframe_tx) == SF_S) &&
        (UE->frame_parms.frame_type == TDD))
      if (UE->mode != loop_through_memory)
        phy_procedures_UE_S_TX(UE,0,0);

    updateTimes(current, &t3, 10000, "Delay to process sub-frame (case 3)");
    proc->instance_cnt_rxtx--;

    if ( IS_SOFTMODEM_BASICSIM || IS_SOFTMODEM_RFSIM ) {
      if (pthread_cond_signal(&proc->cond_rxtx) != 0) abort();
    }

    if (pthread_mutex_unlock(&proc->mutex_rxtx) != 0) {
      LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE RXTX\n" );
      exit_fun("noting to add");
    }
  }

  // thread finished
  free(arg);
  return &UE_thread_rxtx_retval;
}

unsigned int emulator_absSF;

void ue_stub_rx_handler(unsigned int num_bytes,
                        char *rx_buffer)
{
  PHY_VARS_UE *UE;
  UE = PHY_vars_UE_g[0][0];
  UE_tport_t *pdu = (UE_tport_t *)rx_buffer;
  SLSCH_t *slsch = (SLSCH_t *)&pdu->slsch;
  SLDCH_t *sldch = (SLDCH_t *)&pdu->sldch;

  switch (((UE_tport_header_t *)rx_buffer)->packet_type) {
    case TTI_SYNC:
      emulator_absSF = ((UE_tport_header_t *)rx_buffer)->absSF;
      wakeup_thread(&UE->timer_mutex,&UE->timer_cond,&UE->instance_cnt_timer,"timer_thread",100,1);
      break;

    case SLSCH:
      LOG_I(PHY,"Emulator SFN.SF %d.%d, Got SLSCH packet\n",emulator_absSF/10,emulator_absSF%10);
      LOG_I(PHY,"Received %d bytes on UE-UE link for SFN.SF %d.%d, sending SLSCH payload (%d bytes) to MAC\n",num_bytes,
            pdu->header.absSF/10,pdu->header.absSF%10,
            slsch->payload_length);
      printf("SLSCH:");

      for (int i=0; i<sizeof(SLSCH_t); i++) printf("%x ",((uint8_t *)slsch)[i]);

      printf("\n");
      ue_send_sl_sdu(0,
                     0,
                     pdu->header.absSF/10,
                     pdu->header.absSF%10,
                     pdu->payload,
                     slsch->payload_length,
                     0,
                     SL_DISCOVERY_FLAG_NO);
      break;

    case SLDCH:
      LOG_I(PHY,"Emulator SFN.SF %d.%d, Got SLDCH packet\n",emulator_absSF/10,emulator_absSF%10);
      LOG_I(PHY,"Received %d bytes on UE-UE link for SFN.SF %d.%d, sending SLDCH payload (%d bytes) to MAC\n",num_bytes,
            pdu->header.absSF/10,pdu->header.absSF%10,
            sldch->payload_length);
      printf("SLDCH:");

      for (int i=0; i<sizeof(SLDCH_t); i++) printf("%x ",((uint8_t *)sldch)[i]);

      printf("\n");
      ue_send_sl_sdu(0,
                     0,
                     pdu->header.absSF/10,
                     pdu->header.absSF%10,
                     sldch->payload,
                     sldch->payload_length,
                     0,
                     SL_DISCOVERY_FLAG_YES);
      break;
  }
}


/*!
 * \brief This is the UE thread for RX subframe n and TX subframe n+4.
 * This thread performs the phy_procedures_UE_RX() on every received slot.
 * then, if TX is enabled it performs TX for n+4.
 * \param arg is a pointer to a \ref PHY_VARS_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */

static void *UE_phy_stub_single_thread_rxn_txnp4(void *arg)
{
  thread_top_init("UE_phy_stub_thread_rxn_txnp4",1,870000L,1000000L,1000000L);
  // for multipule UE's L2-emulator
  //module_id_t Mod_id = 0;
  //int init_ra_UE = -1; // This counter is used to initiate the RA of each UE in different SFrames
  static __thread int UE_thread_rxtx_retval;
  struct rx_tx_thread_data *rtd = arg;

  if (rtd == NULL) {
    LOG_E( MAC, "[SCHED][UE] rx_tx_thread_data *rtd: NULL pointer\n" );
    exit_fun("nothing to add");
  }

  UE_rxtx_proc_t *proc = rtd->proc;
  // settings for nfapi-L2-emulator mode
  module_id_t ue_thread_id = rtd->ue_thread_id;
  uint16_t     ue_index = 0;
  uint16_t     ue_num = NB_UE_INST/NB_THREAD_INST+((NB_UE_INST%NB_THREAD_INST > ue_thread_id) ? 1 :0);
  module_id_t ue_Mod_id;
  PHY_VARS_UE    *UE = NULL;
  int ret;
  uint8_t   end_flag;
  proc = &PHY_vars_UE_g[0][0]->proc.proc_rxtx[0];
  phy_stub_ticking->num_single_thread[ue_thread_id] = -1;
  UE = rtd->UE;

  UL_INFO = (UL_IND_t *)malloc(sizeof(UL_IND_t));
  UL_INFO->rx_ind.rx_indication_body.rx_pdu_list = calloc(NB_UE_INST, sizeof(nfapi_rx_indication_pdu_t));
  UL_INFO->rx_ind.rx_indication_body.number_of_pdus = 0;
  UL_INFO->crc_ind.crc_indication_body.crc_pdu_list = calloc(NB_UE_INST, sizeof(nfapi_crc_indication_pdu_t));
  UL_INFO->crc_ind.crc_indication_body.number_of_crcs = 0;
  UL_INFO->harq_ind.harq_indication_body.harq_pdu_list = calloc(NB_UE_INST, sizeof(nfapi_harq_indication_pdu_t));
  UL_INFO->harq_ind.harq_indication_body.number_of_harqs = 0;
  UL_INFO->sr_ind.sr_indication_body.sr_pdu_list = calloc(NB_UE_INST, sizeof(nfapi_sr_indication_pdu_t));
  UL_INFO->sr_ind.sr_indication_body.number_of_srs = 0;
  UL_INFO->cqi_ind.cqi_indication_body.cqi_pdu_list =  calloc(NB_UE_INST, sizeof(nfapi_cqi_indication_pdu_t));
  UL_INFO->cqi_ind.cqi_indication_body.cqi_raw_pdu_list = calloc(NB_UE_INST, sizeof(nfapi_cqi_indication_raw_pdu_t));
  UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis = 0;

  if(ue_thread_id == 0) {
    phy_stub_ticking->ticking_var = -1;
    proc->subframe_rx=proc->sub_frame_start;
    // Initializations for nfapi-L2-emulator mode
    dl_config_req = NULL;
    ul_config_req = NULL;
    hi_dci0_req        = NULL;
    tx_request_pdu_list = NULL;

    // waiting for all UE's threads set phy_stub_ticking->num_single_thread[ue_thread_id] = -1.
    do {
      end_flag = 1;

      for(uint16_t i = 0; i< NB_THREAD_INST; i++) {
        if(phy_stub_ticking->num_single_thread[i] == 0) {
          end_flag = 0;
        }
      }
    } while(end_flag == 0);

    sync_var=0;
  }

  //PANOS: CAREFUL HERE!
  wait_sync("UE_phy_stub_single_thread_rxn_txnp4");

  while (!oai_exit) {
    if(ue_thread_id == 0) {
      if (pthread_mutex_lock(&phy_stub_ticking->mutex_ticking) != 0) {
        LOG_E( MAC, "[SCHED][UE] error locking mutex for UE RXTX\n" );
        exit_fun("nothing to add");
      }

      while (phy_stub_ticking->ticking_var < 0) {
        // most of the time, the thread is waiting here
        //pthread_cond_wait( &proc->cond_rxtx, &proc->mutex_rxtx )
        LOG_D(MAC,"Waiting for ticking_var\n");
        pthread_cond_wait( &phy_stub_ticking->cond_ticking, &phy_stub_ticking->mutex_ticking);
      }

      phy_stub_ticking->ticking_var--;

      if (pthread_mutex_unlock(&phy_stub_ticking->mutex_ticking) != 0) {
        LOG_E( MAC, "[SCHED][UE] error unlocking mutex for UE RXn_TXnp4\n" );
        exit_fun("nothing to add");
      }

      proc->subframe_rx=timer_subframe;
      proc->frame_rx = timer_frame;
      // FDD and TDD tx timing settings.
      // XXX:It is the result of timing adjustment in debug.
      // It is necessary to investigate why this will work in the future.
      proc->subframe_tx=(timer_subframe+sf_ahead)%10;
      proc->frame_tx = proc->frame_rx + (proc->subframe_rx>(9-sf_ahead)?1:0);
      //oai_subframe_ind(proc->frame_rx, proc->subframe_rx);

      if (UE != NULL) {
        if (UE->frame_parms.frame_type == FDD) {
          oai_subframe_ind(proc->frame_rx, proc->subframe_rx);
        } else {
          oai_subframe_ind(proc->frame_tx, proc->subframe_tx);
        }
      } else {
        // Default will be FDD
        oai_subframe_ind(proc->frame_rx, proc->subframe_rx);
      }

      //Guessing that the next 4 lines are not needed for the phy_stub mode.
      /*initRefTimes(t2);
        initRefTimes(t3);
        pickTime(current);
        updateTimes(proc->gotIQs, &t2, 10000, "Delay to wake up UE_Thread_Rx (case 2)");*/

      if (pthread_mutex_lock(&phy_stub_ticking->mutex_single_thread) != 0) {
        LOG_E( MAC, "[SCHED][UE] error locking mutex for ue_thread_id %d (mutex_single_thread)\n",ue_thread_id);
        exit_fun("nothing to add");
      }

      memset(&phy_stub_ticking->num_single_thread[0],0,sizeof(int)*NB_THREAD_INST);
      pthread_cond_broadcast(&phy_stub_ticking->cond_single_thread);

      if (pthread_mutex_unlock(&phy_stub_ticking->mutex_single_thread) != 0) {
        LOG_E( MAC, "[SCHED][UE] error unlocking mutex for ue_thread_id %d (mutex_single_thread)\n",ue_thread_id);
        exit_fun("nothing to add");
      }
    } else {
      if (pthread_mutex_lock(&phy_stub_ticking->mutex_single_thread) != 0) {
        LOG_E( MAC, "[SCHED][UE] error locking mutex for ue_thread_id %d (mutex_single_thread)\n",ue_thread_id);
        exit_fun("nothing to add");
      }

      while (phy_stub_ticking->num_single_thread[ue_thread_id] < 0) {
        // most of the time, the thread is waiting here
        LOG_D(MAC,"Waiting for single_thread (ue_thread_id %d)\n",ue_thread_id);
        pthread_cond_wait( &phy_stub_ticking->cond_single_thread, &phy_stub_ticking->mutex_single_thread);
      }

      if (pthread_mutex_unlock(&phy_stub_ticking->mutex_single_thread) != 0) {
        LOG_E( MAC, "[SCHED][UE] error unlocking mutex for ue_thread_id %d (mutex_single_thread)\n",ue_thread_id);
        exit_fun("nothing to add");
      }
    }

    if (dl_config_req && tx_request_pdu_list) {
      nfapi_dl_config_request_body_t* dl_config_req_body = &dl_config_req->dl_config_request_body;
      for (int i = 0; i < dl_config_req_body->number_pdu; ++i) {
        nfapi_dl_config_request_pdu_t* pdu = &dl_config_req_body->dl_config_pdu_list[i];
        if (pdu->pdu_type ==  NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE) {
          i += 1;
          AssertFatal(i < dl_config_req->dl_config_request_body.number_pdu,
                      "Need PDU following DCI at index %d, but not found\n",
                      i);
          nfapi_dl_config_request_pdu_t *dlsch = &dl_config_req_body->dl_config_pdu_list[i];
          if (dlsch->pdu_type != NFAPI_DL_CONFIG_DLSCH_PDU_TYPE) {
            LOG_E(MAC, "expected DLSCH PDU at index %d\n", i);
            continue;
          }
          dl_config_req_UE_MAC_dci(NFAPI_SFNSF2SFN(dl_config_req->sfn_sf),
                                   NFAPI_SFNSF2SF(dl_config_req->sfn_sf),
                                   pdu,
                                   dlsch,
                                   ue_num);
        } else if (pdu->pdu_type == NFAPI_DL_CONFIG_BCH_PDU_TYPE) {
          dl_config_req_UE_MAC_bch(NFAPI_SFNSF2SFN(dl_config_req->sfn_sf),
                                   NFAPI_SFNSF2SF(dl_config_req->sfn_sf),
                                   pdu,
                                   ue_num);
        } else if (pdu->pdu_type == NFAPI_DL_CONFIG_MCH_PDU_TYPE) {
          dl_config_req_UE_MAC_mch(NFAPI_SFNSF2SFN(dl_config_req->sfn_sf),
                                   NFAPI_SFNSF2SF(dl_config_req->sfn_sf),
                                   pdu,
                                   ue_num);
        }
      }
    }

    if (hi_dci0_req) {
      nfapi_hi_dci0_request_body_t *hi_dci0_body = &hi_dci0_req->hi_dci0_request_body;
      for (int i = 0; i < hi_dci0_body->number_of_dci + hi_dci0_body->number_of_hi; i++) {
        nfapi_hi_dci0_request_pdu_t* pdu = &hi_dci0_body->hi_dci0_pdu_list[i];
        hi_dci0_req_UE_MAC(NFAPI_SFNSF2SFN(hi_dci0_req->sfn_sf),
                           NFAPI_SFNSF2SF(hi_dci0_req->sfn_sf),
                           pdu,
                           ue_num);
      }
    }

    //for (Mod_id=0; Mod_id<NB_UE_INST; Mod_id++) {
    for (ue_index=0; ue_index < ue_num; ue_index++) {
      ue_Mod_id = ue_thread_id + NB_THREAD_INST*ue_index;
      UE = PHY_vars_UE_g[ue_Mod_id][0];
      //LOG_D(MAC, "UE_phy_stub_single_thread_rxn_txnp4, NB_UE_INST:%d, Mod_id:%d \n", NB_UE_INST, Mod_id);
      //UE = PHY_vars_UE_g[Mod_id][0];
      lte_subframe_t sf_type = subframe_select( &UE->frame_parms, proc->subframe_rx);

      if ((sf_type == SF_DL) ||
          (UE->frame_parms.frame_type == FDD) ||
          (sf_type == SF_S)) {
        if (UE->frame_parms.frame_type == TDD) {
          LOG_D(PHY, "TDD%d,%s: calling UE_RX\n",
                UE->frame_parms.tdd_config,
                (sf_type==SF_DL? "SF_DL" :
                 (sf_type==SF_UL? "SF_UL" :
                  (sf_type==SF_S ? "SF_S"  : "UNKNOWN_SF_TYPE"))));
        } else {
          LOG_D(PHY, "%s,%s: calling UE_RX\n",
                (UE->frame_parms.frame_type==FDD? "FDD":
                 (UE->frame_parms.frame_type==TDD? "TDD":"UNKNOWN_DUPLEX_MODE")),
                (sf_type==SF_DL? "SF_DL" :
                 (sf_type==SF_UL? "SF_UL" :
                  (sf_type==SF_S ? "SF_S"  : "UNKNOWN_SF_TYPE"))));
        }

        phy_procedures_UE_SL_RX(UE,proc);

        if(NFAPI_MODE!=NFAPI_UE_STUB_PNF)
          phy_procedures_UE_SL_TX(UE,proc);
      }

#if UE_TIMING_TRACE
      start_meas(&UE->generic_stat);
#endif

      if (UE->mac_enabled==1) {
        ret = ue_scheduler(ue_Mod_id,
                           proc->frame_rx,
                           proc->subframe_rx,
                           proc->frame_tx,
                           proc->subframe_tx,
                           subframe_select(&UE->frame_parms,proc->subframe_tx),
                           0,
                           0/*FIXME CC_id*/);

        if ( ret != CONNECTION_OK) {
          LOG_E( PHY, "[UE %"PRIu8"] Frame %"PRIu32", subframe %u %s\n",
                 UE->Mod_id, proc->frame_rx, proc->subframe_tx,get_connectionloss_errstr(ret) );
        }
      }

#if UE_TIMING_TRACE
      stop_meas(&UE->generic_stat);
#endif

      // Prepare the future Tx data

      if ((subframe_select( &UE->frame_parms, proc->subframe_tx) == SF_UL) ||
          (UE->frame_parms.frame_type == FDD) )
        if (UE->mode != loop_through_memory) {
          // We make the start of RA between consecutive UEs differ by 20 frames
          //if ((UE_mac_inst[Mod_id].UE_mode[0] == PRACH  && Mod_id == 0) || (UE_mac_inst[Mod_id].UE_mode[0] == PRACH && Mod_id>0 && proc->frame_rx >= UE_mac_inst[Mod_id-1].ra_frame + 20) ) {
          if (UE_mac_inst[ue_Mod_id].UE_mode[0] == PRACH  && ue_Mod_id == next_Mod_id) {
            next_ra_frame++;

            if(next_ra_frame > 500) {
              // check if we have PRACH opportunity
              if (is_prach_subframe(&UE->frame_parms,proc->frame_tx, proc->subframe_tx) &&  UE_mac_inst[ue_Mod_id].SI_Decoded == 1) {
                // The one working strangely...
                //if (is_prach_subframe(&UE->frame_parms,proc->frame_tx, proc->subframe_tx && Mod_id == (module_id_t) init_ra_UE) ) {
                PRACH_RESOURCES_t *prach_resources = ue_get_rach(ue_Mod_id, 0, proc->frame_tx, 0, proc->subframe_tx);

                if(prach_resources!=NULL ) {
                  UE_mac_inst[ue_Mod_id].ra_frame = proc->frame_rx;
                  LOG_D(MAC, "UE_phy_stub_thread_rxn_txnp4 before RACH, Mod_id: %d frame %d subframe %d\n", ue_Mod_id,proc->frame_tx, proc->subframe_tx);
                  fill_rach_indication_UE_MAC(ue_Mod_id, proc->frame_tx,proc->subframe_tx, UL_INFO, prach_resources->ra_PreambleIndex, prach_resources->ra_RNTI);
                  Msg1_transmitted(ue_Mod_id, 0, proc->frame_tx, 0);
                  UE_mac_inst[ue_Mod_id].UE_mode[0] = RA_RESPONSE;
                  next_Mod_id = ue_Mod_id + 1;
                  //next_ra_frame = (proc->frame_rx + 20)%1000;
                  next_ra_frame = 0;
                }

                //ue_prach_procedures(ue,proc,eNB_id,abstraction_flag,mode);
              }
            }
          } // mode is PRACH

          // Substitute call to phy_procedures Tx with call to phy_stub functions in order to trigger
          // UE Tx procedures directly at the MAC layer, based on the received ul_config requests from the vnf (eNB).
          // Generate UL_indications which correspond to UL traffic.
          if(ul_config_req!=NULL) { //&& UE_mac_inst[Mod_id].ul_config_req->ul_config_request_body.ul_config_pdu_list != NULL){
            ul_config_req_UE_MAC(ul_config_req, timer_frame, timer_subframe, ue_Mod_id);
          }
        }

      phy_procedures_UE_SL_RX(UE,proc);
    } //for (Mod_id=0; Mod_id<NB_UE_INST; Mod_id++)

    phy_stub_ticking->num_single_thread[ue_thread_id] = -1;

    // waiting for all UE's threads set phy_stub_ticking->num_single_thread[ue_thread_id] = -1.
    if(ue_thread_id == 0) {
      do {
        end_flag = 1;

        for(uint16_t i = 0; i< NB_THREAD_INST; i++) {
          if(phy_stub_ticking->num_single_thread[i] == 0) {
            end_flag = 0;
          }
        }
      } while(end_flag == 0);

      if (UL_INFO->crc_ind.crc_indication_body.number_of_crcs>0) {
        //LOG_D(PHY,"UL_info->crc_ind.crc_indication_body.number_of_crcs:%d CRC_IND:SFN/SF:%d\n", UL_info->crc_ind.crc_indication_body.number_of_crcs, NFAPI_SFNSF2DEC(UL_info->crc_ind.sfn_sf));
        //LOG_I(MAC, "ul_config_req_UE_MAC 2.2, SFN/SF of PNF counter:%d.%d, number_of_crcs: %d \n", timer_frame, timer_subframe, UL_INFO->crc_ind.crc_indication_body.number_of_crcs);
        oai_nfapi_crc_indication(&UL_INFO->crc_ind);
        //LOG_I(MAC, "ul_config_req_UE_MAC 2.21 \n");
        UL_INFO->crc_ind.crc_indication_body.number_of_crcs = 0;
      }

      if (UL_INFO->rx_ind.rx_indication_body.number_of_pdus>0) {
        //LOG_D(PHY,"UL_info->rx_ind.number_of_pdus:%d RX_IND:SFN/SF:%d\n", UL_info->rx_ind.rx_indication_body.number_of_pdus, NFAPI_SFNSF2DEC(UL_info->rx_ind.sfn_sf));
        //LOG_I(MAC, "ul_config_req_UE_MAC 2.3, SFN/SF of PNF counter:%d.%d, number_of_pdus: %d \n", timer_frame, timer_subframe, UL_INFO->rx_ind.rx_indication_body.number_of_pdus);
        oai_nfapi_rx_ind(&UL_INFO->rx_ind);

        for(uint8_t num_pdu = 0; num_pdu < UL_INFO->rx_ind.rx_indication_body.number_of_pdus; num_pdu++) {
          free(UL_INFO->rx_ind.rx_indication_body.rx_pdu_list[num_pdu].data);
        }

        //LOG_I(MAC, "ul_config_req_UE_MAC 2.31 \n");
        UL_INFO->rx_ind.rx_indication_body.number_of_pdus = 0;
      }

      if (UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis > 0) {
        oai_nfapi_cqi_indication(&UL_INFO->cqi_ind);
        UL_INFO->cqi_ind.cqi_indication_body.number_of_cqis = 0;
      }

      if(UL_INFO->harq_ind.harq_indication_body.number_of_harqs>0) {
        //LOG_D(MAC, "ul_config_req_UE_MAC 2.4, SFN/SF of PNF counter:%d.%d, number_of_harqs: %d \n", timer_frame, timer_subframe, UL_INFO->harq_ind.harq_indication_body.number_of_harqs);
        oai_nfapi_harq_indication(&UL_INFO->harq_ind);
        //LOG_I(MAC, "ul_config_req_UE_MAC 2.41 \n");
        UL_INFO->harq_ind.harq_indication_body.number_of_harqs =0;
      }

      if(UL_INFO->sr_ind.sr_indication_body.number_of_srs>0) {
        //LOG_I(MAC, "ul_config_req_UE_MAC 2.5, SFN/SF of PNF counter:%d.%d, number_of_srs: %d \n", timer_frame, timer_subframe, UL_INFO->sr_ind.sr_indication_body.number_of_srs);
        oai_nfapi_sr_indication(&UL_INFO->sr_ind);
        //LOG_I(MAC, "ul_config_req_UE_MAC 2.51 \n");
        UL_INFO->sr_ind.sr_indication_body.number_of_srs = 0;
      }

      // De-allocate memory of nfapi requests copies before next subframe round
      if(dl_config_req!=NULL) {
        if(dl_config_req->vendor_extension!=NULL) {
          free(dl_config_req->vendor_extension);
          dl_config_req->vendor_extension = NULL;
        }

        if(dl_config_req->dl_config_request_body.dl_config_pdu_list!=NULL) {
          free(dl_config_req->dl_config_request_body.dl_config_pdu_list);
          dl_config_req->dl_config_request_body.dl_config_pdu_list = NULL;
        }

        free(dl_config_req);
        dl_config_req = NULL;
      }

      if(tx_request_pdu_list!=NULL) {
        for (int i = 0; i < tx_req_num_elems; i++) {
          for (int j = 0; j < tx_request_pdu_list[i].num_segments; j++) {
            free(tx_request_pdu_list[i].segments[j].segment_data);
            tx_request_pdu_list[i].segments[j].segment_data = NULL;
          }
        }
        tx_req_num_elems = 0;
        free(tx_request_pdu_list);
        tx_request_pdu_list = NULL;
      }

      if(ul_config_req!=NULL) {
        if(ul_config_req->ul_config_request_body.ul_config_pdu_list != NULL) {
          free(ul_config_req->ul_config_request_body.ul_config_pdu_list);
          ul_config_req->ul_config_request_body.ul_config_pdu_list = NULL;
        }

        free(ul_config_req);
        ul_config_req = NULL;
      }

      if(hi_dci0_req!=NULL) {
        if(hi_dci0_req->hi_dci0_request_body.hi_dci0_pdu_list!=NULL) {
          free(hi_dci0_req->hi_dci0_request_body.hi_dci0_pdu_list);
          hi_dci0_req->hi_dci0_request_body.hi_dci0_pdu_list = NULL;
        }

        free(hi_dci0_req);
        hi_dci0_req = NULL;
      }
    }
  }

  // Free UL_INFO messages
  free(UL_INFO->cqi_ind.cqi_indication_body.cqi_raw_pdu_list);
  UL_INFO->cqi_ind.cqi_indication_body.cqi_raw_pdu_list = NULL;
  free(UL_INFO->cqi_ind.cqi_indication_body.cqi_pdu_list);
  UL_INFO->cqi_ind.cqi_indication_body.cqi_pdu_list = NULL;
  free(UL_INFO->sr_ind.sr_indication_body.sr_pdu_list);
  UL_INFO->sr_ind.sr_indication_body.sr_pdu_list = NULL;
  free(UL_INFO->harq_ind.harq_indication_body.harq_pdu_list);
  UL_INFO->harq_ind.harq_indication_body.harq_pdu_list = NULL;
  free(UL_INFO->crc_ind.crc_indication_body.crc_pdu_list);
  UL_INFO->crc_ind.crc_indication_body.crc_pdu_list = NULL;
  free(UL_INFO->rx_ind.rx_indication_body.rx_pdu_list);
  UL_INFO->rx_ind.rx_indication_body.rx_pdu_list = NULL;
  free(UL_INFO);
  UL_INFO = NULL;

  // thread finished
  free(arg);
  return &UE_thread_rxtx_retval;
}


/*!
 * \brief This is the UE thread for RX subframe n and TX subframe n+4.
 * This thread performs the phy_procedures_UE_RX() on every received slot.
 * then, if TX is enabled it performs TX for n+4.
 * \param arg is a pointer to a \ref PHY_VARS_UE structure.
 * \returns a pointer to an int. The storage is not on the heap and must not be freed.
 */

static void *UE_phy_stub_thread_rxn_txnp4(void *arg)
{
  thread_top_init("UE_phy_stub_thread_rxn_txnp4",1,870000L,1000000L,1000000L);
  module_id_t Mod_id = 0;
  static __thread int UE_thread_rxtx_retval;
  struct rx_tx_thread_data *rtd = arg;
  UE_rxtx_proc_t *proc = rtd->proc;
  PHY_VARS_UE    *UE   = rtd->UE;
  phy_stub_ticking->ticking_var = -1;
  proc->subframe_rx=proc->sub_frame_start;
  // CAREFUL HERE!
  wait_sync("UE_phy_stub_thread_rxn_txnp4");

  while (!oai_exit) {
    if (pthread_mutex_lock(&phy_stub_ticking->mutex_ticking) != 0) {
      LOG_E( MAC, "[SCHED][UE] error locking mutex for UE RXTX\n" );
      exit_fun("nothing to add");
    }

    while (phy_stub_ticking->ticking_var < 0) {
      // most of the time, the thread is waiting here
      //pthread_cond_wait( &proc->cond_rxtx, &proc->mutex_rxtx )
      LOG_D(MAC,"Waiting for ticking_var\n");
      pthread_cond_wait( &phy_stub_ticking->cond_ticking, &phy_stub_ticking->mutex_ticking);
    }

    phy_stub_ticking->ticking_var--;

    if (pthread_mutex_unlock(&phy_stub_ticking->mutex_ticking) != 0) {
      LOG_E( MAC, "[SCHED][UE] error unlocking mutex for UE RXn_TXnp4\n" );
      exit_fun("nothing to add");
    }

    proc->subframe_rx=timer_subframe;
    proc->frame_rx = timer_frame;
    proc->subframe_tx=(timer_subframe+4)%10;
    proc->frame_tx = proc->frame_rx + (proc->subframe_rx>5?1:0);
    // Process Rx data for one sub-frame
    lte_subframe_t sf_type = subframe_select( &UE->frame_parms, proc->subframe_rx);

    if ((sf_type == SF_DL) ||
        (UE->frame_parms.frame_type == FDD) ||
        (sf_type == SF_S)) {
      if (UE->frame_parms.frame_type == TDD) {
        LOG_D(PHY, "TDD%d,%s: calling UE_RX\n",
              UE->frame_parms.tdd_config,
              (sf_type==SF_DL? "SF_DL" :
               (sf_type==SF_UL? "SF_UL" :
                (sf_type==SF_S ? "SF_S"  : "UNKNOWN_SF_TYPE"))));
      } else {
        LOG_D(PHY, "%s,%s: calling UE_RX\n",
              (UE->frame_parms.frame_type==FDD? "FDD":
               (UE->frame_parms.frame_type==TDD? "TDD":"UNKNOWN_DUPLEX_MODE")),
              (sf_type==SF_DL? "SF_DL" :
               (sf_type==SF_UL? "SF_UL" :
                (sf_type==SF_S ? "SF_S"  : "UNKNOWN_SF_TYPE"))));
      }

      phy_procedures_UE_SL_RX(UE,proc);
      oai_subframe_ind(timer_frame, timer_subframe);

      if(dl_config_req!= NULL) {
        AssertFatal(0, "dl_config_req_UE_MAC() not handled\n");
        //dl_config_req_UE_MAC(dl_config_req, Mod_id);
      }

      //if(UE_mac_inst[Mod_id].hi_dci0_req!= NULL){
      if (hi_dci0_req!=NULL && hi_dci0_req->hi_dci0_request_body.hi_dci0_pdu_list!=NULL) {
        AssertFatal(0, "hi_dci0_req_UE_MAC() not handled\n");
        //hi_dci0_req_UE_MAC(hi_dci0_req, Mod_id);
        //if(UE_mac_inst[Mod_id].hi_dci0_req->hi_dci0_request_body.hi_dci0_pdu_list!=NULL){
        free(hi_dci0_req->hi_dci0_request_body.hi_dci0_pdu_list);
        hi_dci0_req->hi_dci0_request_body.hi_dci0_pdu_list = NULL;
        //}
        free(hi_dci0_req);
        hi_dci0_req = NULL;
      } else if(hi_dci0_req!=NULL) {
        free(hi_dci0_req);
        hi_dci0_req = NULL;
      }

      if (NFAPI_MODE!=NFAPI_UE_STUB_PNF)
        phy_procedures_UE_SL_TX(UE,proc);
    }

#if UE_TIMING_TRACE
    start_meas(&UE->generic_stat);
#endif

    if (UE->mac_enabled==1) {
      int ret = ue_scheduler(UE->Mod_id,
                             proc->frame_rx,
                             proc->subframe_rx,
                             proc->frame_tx,
                             proc->subframe_tx,
                             subframe_select(&UE->frame_parms,proc->subframe_tx),
                             0,
                             0);

      if (ret != CONNECTION_OK)
        LOG_E( PHY, "[UE %"PRIu8"] Frame %"PRIu32", subframe %u %s\n",
               UE->Mod_id, proc->frame_rx, proc->subframe_tx,get_connectionloss_errstr(ret) );
    }

#if UE_TIMING_TRACE
    stop_meas(&UE->generic_stat);
#endif

    // Prepare the future Tx data

    if ((subframe_select( &UE->frame_parms, proc->subframe_tx) == SF_UL) ||
        (UE->frame_parms.frame_type == FDD) )
      if (UE->mode != loop_through_memory) {
        if ((UE_mac_inst[Mod_id].UE_mode[0] == PRACH) ) {
          // check if we have PRACH opportunity
          if (is_prach_subframe(&UE->frame_parms,proc->frame_tx, proc->subframe_tx)) {
            PRACH_RESOURCES_t *prach_resources = ue_get_rach(Mod_id, 0, proc->frame_tx, 0, proc->subframe_tx);

            if(prach_resources!=NULL) {
              fill_rach_indication_UE_MAC(Mod_id, proc->frame_tx,proc->subframe_tx, UL_INFO, prach_resources->ra_PreambleIndex, prach_resources->ra_RNTI);
              Msg1_transmitted(Mod_id, 0, proc->frame_tx, 0);
              UE_mac_inst[Mod_id].UE_mode[0] = RA_RESPONSE;
            }

            //ue_prach_procedures(ue,proc,eNB_id,abstraction_flag,mode);
          }
        } // mode is PRACH

        // Substitute call to phy_procedures Tx with call to phy_stub functions in order to trigger
        // UE Tx procedures directly at the MAC layer, based on the received ul_config requests from the vnf (eNB).
        // Generate UL_indications which correspond to UL traffic.
        if(ul_config_req!= NULL && ul_config_req->ul_config_request_body.ul_config_pdu_list != NULL) {
          //LOG_I(MAC, "UE_phy_stub_thread_rxn_txnp4 ul_config_req is not NULL \n");
          ul_config_req_UE_MAC(ul_config_req, timer_frame, timer_subframe, Mod_id);

          if(ul_config_req->ul_config_request_body.ul_config_pdu_list != NULL) {
            free(ul_config_req->ul_config_request_body.ul_config_pdu_list);
            ul_config_req->ul_config_request_body.ul_config_pdu_list = NULL;
          }

          free(ul_config_req);
          ul_config_req = NULL;
        } else if(ul_config_req!=NULL) {
          free(ul_config_req);
          ul_config_req = NULL;
        }
      }

    phy_procedures_UE_SL_RX(UE,proc);
  }

  // thread finished
  free(arg);
  return &UE_thread_rxtx_retval;
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
void write_dummy(PHY_VARS_UE *UE,  openair0_timestamp timestamp) {
  // we have to write to tell explicitly to the eNB, else it will wait for us forever
  // we write the next subframe (always write in future of what we received)
  //
  struct complex16 v= {0};
  void *samplesVoid[UE->frame_parms.nb_antennas_tx];
  
  for ( int i=0; i < UE->frame_parms.nb_antennas_tx; i++)
    samplesVoid[i]=(void *)&v;
  
  AssertFatal( 1 == UE->rfdevice.trx_write_func(&UE->rfdevice,
						timestamp+2*UE->frame_parms.samples_per_tti,
						samplesVoid, 
						1,
						UE->frame_parms.nb_antennas_tx,
						1),"");
}

void *UE_thread(void *arg)
{
  PHY_VARS_UE *UE = (PHY_VARS_UE *) arg;
  //  int tx_enabled = 0;
  int dummy_rx[UE->frame_parms.nb_antennas_rx][UE->frame_parms.samples_per_tti] __attribute__((aligned(32)));
  openair0_timestamp timestamp,timestamp1;
  void *rxp[NB_ANTENNAS_RX], *txp[NB_ANTENNAS_TX];
  int start_rx_stream = 0;
  int i;
  int th_id;
  static uint8_t thread_idx = 0;
  int ret;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  if ( threads.main != -1 )
    CPU_SET(threads.main, &cpuset);
  init_thread(100000, 500000, FIFO_PRIORITY, &cpuset, "UHD Threads");

  /*
  while (sync_var<0)
    pthread_cond_wait(&sync_cond, &sync_mutex);
  pthread_mutex_unlock(&sync_mutex);
  */
  wait_sync("UE thread");
#ifdef NAS_UE
  MessageDef *message_p;
  message_p = itti_alloc_new_message(TASK_NAS_UE, 0, INITIALIZE_MESSAGE);
  itti_send_msg_to_task (TASK_NAS_UE, UE->Mod_id + NB_eNB_INST, message_p);
#endif
  int sub_frame=-1;
  //int cumulated_shift=0;

  if (UE->rfdevice.trx_start_func(&UE->rfdevice) != 0 ) {
    LOG_E(HW,"Could not start the device\n");
    oai_exit=1;
  }

  while (!oai_exit) {
    if (IS_SOFTMODEM_BASICSIM)
      while (!(UE->proc.instance_cnt_synch < 0)) {
        printf("ue sync not ready\n");
        usleep(500*1000);
      }

    AssertFatal ( 0== pthread_mutex_lock(&UE->proc.mutex_synch), "");
    int instance_cnt_synch = UE->proc.instance_cnt_synch;
    int is_synchronized    = UE->is_synchronized;
    AssertFatal ( 0== pthread_mutex_unlock(&UE->proc.mutex_synch), "");
    
    if (is_synchronized == 0) {
      if (instance_cnt_synch < 0) {  // we can invoke the synch
        // grab 10 ms of signal and wakeup synch thread

        if (UE->mode != loop_through_memory) {
          if (IS_SOFTMODEM_RFSIM ) {
	    for(int sf=0; sf<10; sf++) {
	      for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
		rxp[i] = (void *)&UE->common_vars.rxdata[i][UE->frame_parms.samples_per_tti*sf];
	      
              AssertFatal(UE->frame_parms.samples_per_tti == UE->rfdevice.trx_read_func(&UE->rfdevice,
						      &timestamp,
						      rxp,
						      UE->frame_parms.samples_per_tti,
						      UE->frame_parms.nb_antennas_rx), "");
	      write_dummy(UE, timestamp);
	    }
	  } else {
	    for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
	      rxp[i] = (void *)&UE->common_vars.rxdata[i][0];
	    
	    AssertFatal( UE->frame_parms.samples_per_tti*10 ==
                         UE->rfdevice.trx_read_func(&UE->rfdevice,
                                                    &timestamp,
                                                    rxp,
                                                    UE->frame_parms.samples_per_tti*10,
                                                    UE->frame_parms.nb_antennas_rx), "");
	      }
	}

        AssertFatal ( 0== pthread_mutex_lock(&UE->proc.mutex_synch), "");
        instance_cnt_synch = ++UE->proc.instance_cnt_synch;

        if (instance_cnt_synch == 0) {
          AssertFatal( 0 == pthread_cond_signal(&UE->proc.cond_synch), "");
        } else {
          LOG_E( PHY, "[SCHED][UE] UE sync thread busy!!\n" );
          exit_fun("nothing to add");
        }

        AssertFatal ( 0== pthread_mutex_unlock(&UE->proc.mutex_synch), "");
      } else {
#if OAISIM
        (void)dummy_rx; /* avoid gcc warnings */
        usleep(500);
#else
        // grab 10 ms of signal into dummy buffer
        if (UE->mode != loop_through_memory) {
          for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
            rxp[i] = (void *)&dummy_rx[i][0];

          for (int sf=0; sf<10; sf++) {
            //      printf("Reading dummy sf %d\n",sf);
            UE->rfdevice.trx_read_func(&UE->rfdevice,
                                       &timestamp,
                                       rxp,
                                       UE->frame_parms.samples_per_tti,
                                       UE->frame_parms.nb_antennas_rx);
	    if (IS_SOFTMODEM_RFSIM )
	      write_dummy(UE, timestamp);
	  }
	  }
#endif
      }
    } // UE->is_synchronized==0
    else {
      if (start_rx_stream==0) {
        start_rx_stream=1;

        if (UE->mode != loop_through_memory) {
          if (UE->no_timing_correction==0) {
            LOG_I(PHY,"Resynchronizing RX by %d samples (mode = %d)\n",UE->rx_offset,UE->mode);
	    while ( UE->rx_offset ) {
	      size_t s=min(UE->rx_offset,UE->frame_parms.samples_per_tti);
              AssertFatal(s == UE->rfdevice.trx_read_func(&UE->rfdevice,
						     &timestamp,
						     (void **)UE->common_vars.rxdata,
						     s,
						     UE->frame_parms.nb_antennas_rx),"");
	      if (IS_SOFTMODEM_RFSIM )
		write_dummy(UE, timestamp);
	      UE->rx_offset-=s;
	    }
          }

          UE->rx_offset=0;
          UE->time_sync_cell=0;

          //UE->proc.proc_rxtx[0].frame_rx++;
          //UE->proc.proc_rxtx[1].frame_rx++;
          for (th_id=0; th_id < RX_NB_TH; th_id++) {
            UE->proc.proc_rxtx[th_id].frame_rx++;
          }

          // read in first symbol
          AssertFatal (UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0 ==
                       UE->rfdevice.trx_read_func(&UE->rfdevice,
                                                  &timestamp,
                                                  (void **)UE->common_vars.rxdata,
                                                  UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0,
                                                  UE->frame_parms.nb_antennas_rx),"");
          slot_fep(UE,0, 0, 0, 0, 0);
        } //UE->mode != loop_through_memory
        else
          rt_sleep_ns(1000*1000);
      } else {
        sub_frame++;
        sub_frame%=10;
        UE_rxtx_proc_t *proc = &UE->proc.proc_rxtx[thread_idx];
        // update thread index for received subframe
        UE->current_thread_id[sub_frame] = thread_idx;

        if (IS_SOFTMODEM_BASICSIM || IS_SOFTMODEM_RFSIM ) {
          int t;

          for (t = 0; t < 2; t++) {
            UE_rxtx_proc_t *proc = &UE->proc.proc_rxtx[t];
            pthread_mutex_lock(&proc->mutex_rxtx);

            while (proc->instance_cnt_rxtx >= 0) pthread_cond_wait( &proc->cond_rxtx, &proc->mutex_rxtx );

            pthread_mutex_unlock(&proc->mutex_rxtx);
          }
	  usleep(300);
        }

        LOG_D(PHY,"Process Subframe %d thread Idx %d \n", sub_frame, UE->current_thread_id[sub_frame]);
        thread_idx++;

        if(thread_idx>=RX_NB_TH)
          thread_idx = 0;

        if (UE->mode != loop_through_memory) {
          for (i=0; i<UE->frame_parms.nb_antennas_rx; i++)
            rxp[i] = (void *)&UE->common_vars.rxdata[i][UE->frame_parms.ofdm_symbol_size+
                     UE->frame_parms.nb_prefix_samples0+
                     sub_frame*UE->frame_parms.samples_per_tti];

          for (i=0; i<UE->frame_parms.nb_antennas_tx; i++)
            txp[i] = (void *)&UE->common_vars.txdata[i][((sub_frame+2)%10)*UE->frame_parms.samples_per_tti];

          int readBlockSize, writeBlockSize;

          if (sub_frame<9) {
            readBlockSize=UE->frame_parms.samples_per_tti;
            writeBlockSize=UE->frame_parms.samples_per_tti;
          } else {
            // set TO compensation to zero
            UE->rx_offset_diff = 0;

            // compute TO compensation that should be applied for this frame

            if (UE->no_timing_correction == 0) {
              if ( UE->rx_offset < 5*UE->frame_parms.samples_per_tti  &&
                   UE->rx_offset > 0 )
                UE->rx_offset_diff = -1 ;

              if ( UE->rx_offset > 5*UE->frame_parms.samples_per_tti &&
                   UE->rx_offset < 10*UE->frame_parms.samples_per_tti )
                UE->rx_offset_diff = 1;
            }

            LOG_D(PHY,"AbsSubframe %d.%d SET rx_off_diff to %d rx_offset %d \n",proc->frame_rx,sub_frame,UE->rx_offset_diff,UE->rx_offset);
            readBlockSize=UE->frame_parms.samples_per_tti -
                          UE->frame_parms.ofdm_symbol_size -
                          UE->frame_parms.nb_prefix_samples0 -
                          UE->rx_offset_diff;
            writeBlockSize=UE->frame_parms.samples_per_tti -
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
                           (2*UE->frame_parms.samples_per_tti) -
                           UE->frame_parms.ofdm_symbol_size-UE->frame_parms.nb_prefix_samples0 -
                           openair0_cfg[0].tx_sample_advance,
                           txp,
                           writeBlockSize,
                           UE->frame_parms.nb_antennas_tx,
                           1),"");

          if( sub_frame==9) {
            // read in first symbol of next frame and adjust for timing drift
            int first_symbols=writeBlockSize-readBlockSize;

            if ( first_symbols > 0 )
              AssertFatal(first_symbols ==
                          UE->rfdevice.trx_read_func(&UE->rfdevice,
                                                     &timestamp1,
                                                     (void **)UE->common_vars.rxdata,
                                                     first_symbols,
                                                     UE->frame_parms.nb_antennas_rx),"");

            if ( first_symbols <0 )
              LOG_E(PHY,"can't compensate: diff =%d\n", first_symbols);
          }

          pickTime(gotIQs);

          /* no timeout in IS_SOFTMODEM_BASICSIM or IS_SOFTMODEM_RFSIM mode */
          if (IS_SOFTMODEM_BASICSIM || IS_SOFTMODEM_RFSIM) {
            ret = pthread_mutex_lock(&proc->mutex_rxtx);
          } else {
            struct timespec tv;
            if (clock_gettime(CLOCK_REALTIME, &tv) != 0) {
              perror("clock_gettime");
              exit(1);
            }
            tv.tv_nsec += 10*1000;
            if (tv.tv_nsec >= 1000 * 1000 * 1000) {
              tv.tv_sec++;
              tv.tv_nsec -= 1000 * 1000 * 1000;
            }
            ret = pthread_mutex_timedlock(&proc->mutex_rxtx, &tv);
          }

          // operate on thread sf mod 2
          if (ret != 0) {
            if (ret == ETIMEDOUT) {
              LOG_E(PHY,"Missed real time\n");
              continue;
            } else {
              LOG_E(PHY,"System error %s (%d)\n",strerror(errno),errno);
              abort();
            }
          }

          //          usleep(3000);
          if(sub_frame == 0) {
            //UE->proc.proc_rxtx[0].frame_rx++;
            //UE->proc.proc_rxtx[1].frame_rx++;
            for (th_id=0; th_id < RX_NB_TH; th_id++) {
              UE->proc.proc_rxtx[th_id].frame_rx++;
            }
          }

          //UE->proc.proc_rxtx[0].gotIQs=readTime(gotIQs);
          //UE->proc.proc_rxtx[1].gotIQs=readTime(gotIQs);
          for (th_id=0; th_id < RX_NB_TH; th_id++) {
            UE->proc.proc_rxtx[th_id].gotIQs=readTime(gotIQs);
          }

          proc->subframe_rx=sub_frame;
          proc->subframe_tx=(sub_frame+4)%10;
          proc->frame_tx = proc->frame_rx + (proc->subframe_rx>5?1:0);
          proc->timestamp_tx = timestamp+
                               (4*UE->frame_parms.samples_per_tti)-
                               UE->frame_parms.ofdm_symbol_size-UE->frame_parms.nb_prefix_samples0;
          proc->instance_cnt_rxtx++;
          LOG_D( PHY, "[SCHED][UE %d] UE RX instance_cnt_rxtx %d subframe %d !!\n", UE->Mod_id, proc->instance_cnt_rxtx,proc->subframe_rx);
          T(T_UE_MASTER_TICK, T_INT(0), T_INT(proc->frame_rx%1024), T_INT(proc->subframe_rx));
          AssertFatal (pthread_cond_signal(&proc->cond_rxtx) ==0,"");
          AssertFatal(pthread_mutex_unlock(&proc->mutex_rxtx) ==0,"");
          initRefTimes(t1);
          initStaticTime(lastTime);
          updateTimes(lastTime, &t1, 20000, "Delay between two IQ acquisitions (case 1)");
          pickStaticTime(lastTime);
        } else {
          printf("Processing subframe %d",proc->subframe_rx);
          getchar();
        }
      } // start_rx_stream==1
    } // UE->is_synchronized==1
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
void init_UE_threads(int inst)
{
  struct rx_tx_thread_data *rtd;
  PHY_VARS_UE *UE;
  AssertFatal(PHY_vars_UE_g!=NULL,"PHY_vars_UE_g is NULL\n");
  AssertFatal(PHY_vars_UE_g[inst]!=NULL,"PHY_vars_UE_g[inst] is NULL\n");
  AssertFatal(PHY_vars_UE_g[inst][0]!=NULL,"PHY_vars_UE_g[inst][0] is NULL\n");
  UE = PHY_vars_UE_g[inst][0];
  pthread_attr_init (&UE->proc.attr_ue);
  pthread_attr_setstacksize(&UE->proc.attr_ue,8192);//5*PTHREAD_STACK_MIN);
  pthread_mutex_init(&UE->proc.mutex_synch,NULL);
  pthread_cond_init(&UE->proc.cond_synch,NULL);
  UE->proc.instance_cnt_synch = -1;
  UE->is_synchronized = 0;
  // the threads are not yet active, therefore access is allowed without locking
  int nb_threads=RX_NB_TH;

  for (int i=0; i<nb_threads; i++) {
    rtd = calloc(1, sizeof(struct rx_tx_thread_data));

    if (rtd == NULL) abort();

    rtd->UE = UE;
    rtd->proc = &UE->proc.proc_rxtx[i];
    pthread_mutex_init(&UE->proc.proc_rxtx[i].mutex_rxtx,NULL);
    pthread_cond_init(&UE->proc.proc_rxtx[i].cond_rxtx,NULL);
    UE->proc.proc_rxtx[i].instance_cnt_rxtx = -1;
    UE->proc.proc_rxtx[i].sub_frame_start=i;
    UE->proc.proc_rxtx[i].sub_frame_step=nb_threads;
    printf("Init_UE_threads rtd %d proc %d nb_threads %d i %d\n",rtd->proc->sub_frame_start, UE->proc.proc_rxtx[i].sub_frame_start,nb_threads, i);
    pthread_create(&UE->proc.proc_rxtx[i].pthread_rxtx, NULL, UE_thread_rxn_txnp4, rtd);
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


/*!
 * \brief Initialize the UE theads.
 * Creates the UE threads:
 * - UE_thread_rxtx0
 * - UE_thread_synch
 * - UE_thread_fep_slot0
 * - UE_thread_fep_slot1
 * - UE_thread_dlsch_proc_slot0
 * - UE_thread_dlsch_proc_slot1
 * and the locking between them.
 */
void init_UE_single_thread_stub(int nb_inst)
{
  struct rx_tx_thread_data *rtd;
  PHY_VARS_UE *UE;

  for (int i=0; i<nb_inst; i++) {
    AssertFatal(PHY_vars_UE_g!=NULL,"PHY_vars_UE_g is NULL\n");
    AssertFatal(PHY_vars_UE_g[i]!=NULL,"PHY_vars_UE_g[inst] is NULL\n");
    AssertFatal(PHY_vars_UE_g[i][0]!=NULL,"PHY_vars_UE_g[inst][0] is NULL\n");

    if(NFAPI_MODE==NFAPI_UE_STUB_PNF) {
#ifdef NAS_UE
      MessageDef *message_p;
      message_p = itti_alloc_new_message(TASK_NAS_UE, 0, INITIALIZE_MESSAGE);
      itti_send_msg_to_task (TASK_NAS_UE, i + NB_eNB_INST, message_p);
#endif
    }
  }

  UE = PHY_vars_UE_g[0][0];
  pthread_attr_init (&UE->proc.attr_ue);
  pthread_attr_setstacksize(&UE->proc.attr_ue,8192);//5*PTHREAD_STACK_MIN);
  // Don't need synch for phy_stub mode
  //pthread_mutex_init(&UE->proc.mutex_synch,NULL);
  //pthread_cond_init(&UE->proc.cond_synch,NULL);
  // the threads are not yet active, therefore access is allowed without locking
  // In phy_stub_UE mode due to less heavy processing operations we don't need two threads
  //int nb_threads=RX_NB_TH;
  int nb_threads=1;

  for(uint16_t ue_thread_id = 0; ue_thread_id < NB_THREAD_INST; ue_thread_id++) {
    UE = PHY_vars_UE_g[ue_thread_id][0];

    for (int i=0; i<nb_threads; i++) {
      rtd = calloc(1, sizeof(struct rx_tx_thread_data));

      if (rtd == NULL) abort();

      rtd->UE = UE;
      rtd->proc = &UE->proc.proc_rxtx[i];
      rtd->ue_thread_id = ue_thread_id;
      pthread_mutex_init(&UE->proc.proc_rxtx[i].mutex_rxtx,NULL);
      pthread_cond_init(&UE->proc.proc_rxtx[i].cond_rxtx,NULL);
      UE->proc.proc_rxtx[i].sub_frame_start=i;
      UE->proc.proc_rxtx[i].sub_frame_step=nb_threads;
      printf("Init_UE_threads rtd %d proc %d nb_threads %d i %d\n",rtd->proc->sub_frame_start, UE->proc.proc_rxtx[i].sub_frame_start,nb_threads, i);
      pthread_create(&UE->proc.proc_rxtx[i].pthread_rxtx, NULL, UE_phy_stub_single_thread_rxn_txnp4, rtd);
    }
  }

  // Remove thread for UE_sync in phy_stub_UE mode.
  //pthread_create(&UE->proc.pthread_synch,NULL,UE_thread_synch,(void*)UE);
}


/*!
 * \brief Initialize the UE theads.
 * Creates the UE threads:
 * - UE_thread_rxtx0
 * - UE_thread_synch
 * - UE_thread_fep_slot0
 * - UE_thread_fep_slot1
 * - UE_thread_dlsch_proc_slot0
 * - UE_thread_dlsch_proc_slot1
 * and the locking between them.
 */
void init_UE_threads_stub(int inst)
{
  struct rx_tx_thread_data *rtd;
  PHY_VARS_UE *UE;
  AssertFatal(PHY_vars_UE_g!=NULL,"PHY_vars_UE_g is NULL\n");
  AssertFatal(PHY_vars_UE_g[inst]!=NULL,"PHY_vars_UE_g[inst] is NULL\n");
  AssertFatal(PHY_vars_UE_g[inst][0]!=NULL,"PHY_vars_UE_g[inst][0] is NULL\n");
  UE = PHY_vars_UE_g[inst][0];
  pthread_attr_init (&UE->proc.attr_ue);
  pthread_attr_setstacksize(&UE->proc.attr_ue,8192);//5*PTHREAD_STACK_MIN);
  // Don't need synch for phy_stub mode
  //pthread_mutex_init(&UE->proc.mutex_synch,NULL);
  //pthread_cond_init(&UE->proc.cond_synch,NULL);
  // the threads are not yet active, therefore access is allowed without locking
  // In phy_stub_UE mode due to less heavy processing operations we don't need two threads
  //int nb_threads=RX_NB_TH;
  int nb_threads=1;

  for (int i=0; i<nb_threads; i++) {
    rtd = calloc(1, sizeof(struct rx_tx_thread_data));

    if (rtd == NULL) abort();

    rtd->UE = UE;
    rtd->proc = &UE->proc.proc_rxtx[i];
    pthread_mutex_init(&UE->proc.proc_rxtx[i].mutex_rxtx,NULL);
    pthread_cond_init(&UE->proc.proc_rxtx[i].cond_rxtx,NULL);
    UE->proc.proc_rxtx[i].sub_frame_start=i;
    UE->proc.proc_rxtx[i].sub_frame_step=nb_threads;
    printf("Init_UE_threads rtd %d proc %d nb_threads %d i %d\n",rtd->proc->sub_frame_start, UE->proc.proc_rxtx[i].sub_frame_start,nb_threads, i);
    pthread_create(&UE->proc.proc_rxtx[i].pthread_rxtx, NULL, UE_phy_stub_thread_rxn_txnp4, rtd);
  }

  // Remove thread for UE_sync in phy_stub_UE mode.
  //pthread_create(&UE->proc.pthread_synch,NULL,UE_thread_synch,(void*)UE);
}


#ifdef OPENAIR2
void fill_ue_band_info(void)
{
  LTE_UE_EUTRA_Capability_t *UE_EUTRA_Capability = UE_rrc_inst[0].UECap->UE_EUTRA_Capability;
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

int setup_ue_buffers(PHY_VARS_UE **phy_vars_ue,
                     openair0_config_t *openair0_cfg)
{
  int i, CC_id;
  LTE_DL_FRAME_PARMS *frame_parms;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    AssertFatal( phy_vars_ue[CC_id] !=0, "");
    frame_parms = &(phy_vars_ue[CC_id]->frame_parms);
    // replace RX signal buffers with mmaped HW versions
    rxdata = (int32_t **)malloc16( frame_parms->nb_antennas_rx*sizeof(int32_t *) );
    txdata = (int32_t **)malloc16( frame_parms->nb_antennas_tx*sizeof(int32_t *) );

    for (i=0; i<frame_parms->nb_antennas_rx; i++) {
      LOG_I(PHY, "Mapping UE CC_id %d, rx_ant %d, freq %lu on card %d, chain %d\n",
            CC_id, i, downlink_frequency[CC_id][i], phy_vars_ue[CC_id]->rf_map.card, (phy_vars_ue[CC_id]->rf_map.chain)+i );
      free( phy_vars_ue[CC_id]->common_vars.rxdata[i] );
      rxdata[i] = (int32_t *)malloc16_clear( 307200*sizeof(int32_t) );
      phy_vars_ue[CC_id]->common_vars.rxdata[i] = rxdata[i]; // what about the "-N_TA_offset" ? // N_TA offset for TDD
    }

    for (i=0; i<frame_parms->nb_antennas_tx; i++) {
      LOG_I(PHY, "Mapping UE CC_id %d, tx_ant %d, freq %lu on card %d, chain %d\n",
            CC_id, i, downlink_frequency[CC_id][i], phy_vars_ue[CC_id]->rf_map.card, (phy_vars_ue[CC_id]->rf_map.chain)+i );
      free( phy_vars_ue[CC_id]->common_vars.txdata[i] );
      txdata[i] = (int32_t *)malloc16_clear( 307200*sizeof(int32_t) );
      phy_vars_ue[CC_id]->common_vars.txdata[i] = txdata[i];
    }

    // rxdata[x] points now to the same memory region as phy_vars_ue[CC_id]->common_vars.rxdata[x]
    // txdata[x] points now to the same memory region as phy_vars_ue[CC_id]->common_vars.txdata[x]
    // be careful when releasing memory!
    // because no "release_ue_buffers"-function is available, at least rxdata and txdata memory will leak (only some bytes)
  }

  return 0;
}


// Panos: This timer thread is used only in the phy_stub mode as an independent timer
// which will be ticking and provide the SFN/SF values that will be used from the UE threads
// playing the role of nfapi-pnf.

//02/02/2018
static void *timer_thread( void *param )
{
  thread_top_init("timer_thread",1,870000L,1000000L,1000000L);
  timer_subframe =9;
  timer_frame    =1023;
  //phy_stub_ticking = (SF_ticking*)malloc(sizeof(SF_ticking));
  phy_stub_ticking->ticking_var = -1;
  PHY_VARS_UE *UE;
  UE = PHY_vars_UE_g[0][0];
  //double t_diff;
  int external_timer = 0;
  wait_sync("timer_thread");
  opp_enabled = 1;

  // first check if we are receiving timing indications
  if(NFAPI_MODE==NFAPI_UE_STUB_OFFNET) {
    usleep(10000);

    if (UE->instance_cnt_timer > 0) {
      external_timer = 1;
      int absSFm1 = ((emulator_absSF+10239)%10240);
      timer_frame = absSFm1/10;
      timer_subframe = absSFm1%10;
      pthread_mutex_lock(&UE->timer_mutex);
      UE->instance_cnt_timer = -1;
      pthread_mutex_unlock(&UE->timer_mutex);
      LOG_I(PHY,"Running with external timer\n");
    } else LOG_I(PHY,"Running with internal timer\n");
  }

  struct timespec t_start;

  struct timespec t_now;

  struct timespec t_sleep;

  uint64_t T_0;

  uint64_t T_now;

  uint64_t T_next_SF;

  uint64_t T_sleep;

  uint64_t sf_cnt = 0; //Total Subframe counter

  clock_gettime(CLOCK_MONOTONIC, &t_start);

  T_0 = (uint64_t) t_start.tv_sec*1000000000 + t_start.tv_nsec;

  LOG_D(MAC, "timer_thread(), T_0 value: %" PRId64 "\n", T_0);

  while (!oai_exit) {
    // these are local subframe/frame counters to check that we are in synch with the fronthaul timing.
    // They are set on the first rx/tx in the underly FH routines.
    if (timer_subframe==9) {
      timer_subframe=0;
      timer_frame++;
      timer_frame&=1023;
    } else {
      timer_subframe++;
    }

    //AssertFatal( 0 == pthread_cond_signal(&phy_stub_ticking->cond_ticking), "");
    AssertFatal(pthread_mutex_lock(&phy_stub_ticking->mutex_ticking) ==0,"");
    phy_stub_ticking->ticking_var++;

    // This should probably be a call to pthread_cond_broadcast when we introduce support for multiple UEs (threads)
    if(phy_stub_ticking->ticking_var == 0) {
      //AssertFatal(phy_stub_ticking->ticking_var == 0,"phy_stub_ticking->ticking_var = %d",
      //phy_stub_ticking->ticking_var);
      if (pthread_cond_signal(&phy_stub_ticking->cond_ticking) != 0) {
        //LOG_E( PHY, "[SCHED][UE %d] ERROR pthread_cond_signal for UE RX thread\n", UE->Mod_id);
        LOG_E( PHY, "timer_thread ERROR pthread_cond_signal for UE_thread\n");
        exit_fun("nothing to add");
      }
    } else
      LOG_D(MAC, "timer_thread() Timing problem! ticking_var value:%d \n \n \n", phy_stub_ticking->ticking_var);

    AssertFatal(pthread_mutex_unlock(&phy_stub_ticking->mutex_ticking) ==0,"");
    start_meas(&UE->timer_stats);

    //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); // get initial time-stamp
    if (external_timer == 0) {
      clock_gettime(CLOCK_MONOTONIC, &t_now);
      sf_cnt++;
      T_next_SF = T_0 + sf_cnt*1000000;
      T_now =(uint64_t) t_now.tv_sec*1000000000 + t_now.tv_nsec;

      if(T_now > T_next_SF) {
        t_sleep.tv_sec =0;
        t_sleep.tv_nsec =0;
      } else {
        T_sleep = T_next_SF - T_now;
        t_sleep.tv_sec =0;
        t_sleep.tv_nsec = (__syscall_slong_t) T_sleep;
      }

      nanosleep(&t_sleep, (struct timespec *)NULL);
      UE_tport_t pdu;
      pdu.header.packet_type = TTI_SYNC;
      pdu.header.absSF = (timer_frame*10)+timer_subframe;

      if (NFAPI_MODE != NFAPI_UE_STUB_PNF) {
        multicast_link_write_sock(0,
                                  (char *)&pdu,
                                  sizeof(UE_tport_header_t));
      }
    } else {
      wait_on_condition(&UE->timer_mutex,&UE->timer_cond,&UE->instance_cnt_timer,"timer_thread");
      release_thread(&UE->timer_mutex,&UE->instance_cnt_timer,"timer_thread");
    }

    /*stop_meas(&UE->timer_stats);
    t_diff = get_time_meas_us(&UE->timer_stats);

    stop_meas(&UE->timer_stats);
    t_diff = get_time_meas_us(&UE->timer_stats);*/
  }

  free(phy_stub_ticking);
  pthread_cond_destroy(&phy_stub_ticking->cond_ticking);
  pthread_mutex_destroy(&phy_stub_ticking->mutex_ticking);
  return 0;
}


int init_timer_thread(void)
{
  //PHY_VARS_UE *UE=PHY_vars_UE_g[0];
  PHY_VARS_UE *UE=PHY_vars_UE_g[0][0];
  phy_stub_ticking = (SF_ticking *)malloc(sizeof(SF_ticking));
  pthread_mutex_init(&UE->timer_mutex,NULL);
  pthread_cond_init(&UE->timer_cond,NULL);
  UE->instance_cnt_timer = -1;
  memset(&phy_stub_ticking->num_single_thread[0],0,sizeof(int)*NB_THREAD_INST);
  pthread_mutex_init(&phy_stub_ticking->mutex_ticking,NULL);
  pthread_cond_init(&phy_stub_ticking->cond_ticking,NULL);
  pthread_mutex_init(&phy_stub_ticking->mutex_single_thread,NULL);
  pthread_cond_init(&phy_stub_ticking->cond_single_thread,NULL);
  pthread_create(&phy_stub_ticking->pthread_timer, NULL, &timer_thread, NULL);
  return 0;
}


/* HACK: this function is needed to compile the UE
 * fix it somehow
 */
int8_t find_dlsch(uint16_t rnti,
                  PHY_VARS_eNB *eNB,
                  find_type_t type)
{
  printf("you cannot read this\n");
  abort();
}

