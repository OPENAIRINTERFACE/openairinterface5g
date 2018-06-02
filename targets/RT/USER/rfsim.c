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

/*! \file rfsim.c
* \brief function for simulated RF device
* \author R. Knopp
* \date 2018
* \version 1.0
* \company Eurecom
* \email: openair_tech@eurecom.fr
* \note
* \warning
*/


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#include <time.h>
#include <mcheck.h>
#include <sys/timerfd.h>

#include "assertions.h"
#include "rfsim.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
#include "enb_config.h"
#include "enb_paramdef.h"
#include "platform_constants.h"
#include "common/config/config_paramdesc.h"
#include "common/config/config_userapi.h"
#include "common/ran_context.h"
#include "PHY/defs_UE.h"
#include "PHY/defs_eNB.h"

RAN_CONTEXT_t RC;
extern PHY_VARS_UE ***PHY_vars_UE_g;


// put all of these in a common structure after
channel_desc_t *RU2UE[NUMBER_OF_RU_MAX][NUMBER_OF_UE_MAX][MAX_NUM_CCs];
channel_desc_t *UE2RU[NUMBER_OF_UE_MAX][NUMBER_OF_RU_MAX][MAX_NUM_CCs];
double r_re_DL[NUMBER_OF_UE_MAX][2][30720];
double r_im_DL[NUMBER_OF_UE_MAX][2][30720];
double r_re_UL[NUMBER_OF_eNB_MAX][2][30720];
double r_im_UL[NUMBER_OF_eNB_MAX][2][30720];
int RU_output_mask[NUMBER_OF_UE_MAX];
int UE_output_mask[NUMBER_OF_RU_MAX];
pthread_mutex_t RU_output_mutex[NUMBER_OF_UE_MAX];
pthread_mutex_t UE_output_mutex[NUMBER_OF_RU_MAX];
pthread_mutex_t subframe_mutex;
int subframe_ru_mask=0,subframe_UE_mask=0;
openair0_timestamp current_ru_rx_timestamp[NUMBER_OF_RU_MAX][MAX_NUM_CCs];
openair0_timestamp current_UE_rx_timestamp[MAX_MOBILES_PER_ENB][MAX_NUM_CCs];
openair0_timestamp last_ru_rx_timestamp[NUMBER_OF_RU_MAX][MAX_NUM_CCs];
openair0_timestamp last_UE_rx_timestamp[MAX_MOBILES_PER_ENB][MAX_NUM_CCs];
double ru_amp[NUMBER_OF_RU_MAX];
pthread_t rfsim_thread;


void init_ru_devices(void);
void init_RU(const char*);
void rfsim_top(void *n_frames);

void wait_RUs(void)
{
  int i;
  
  // wait for all RUs to be configured over fronthaul
  pthread_mutex_lock(&RC.ru_mutex);
  
  
  
  while (RC.ru_mask>0) {
    pthread_cond_wait(&RC.ru_cond,&RC.ru_mutex);
  }
  
  // copy frame parameters from RU to UEs
  for (i=0;i<NB_UE_INST;i++) {
/*
    PHY_vars_UE_g[i][0]->frame_parms.N_RB_DL              = RC.ru[0]->frame_parms.N_RB_DL;
    PHY_vars_UE_g[i][0]->frame_parms.N_RB_UL              = RC.ru[0]->frame_parms.N_RB_UL;
    PHY_vars_UE_g[i][0]->frame_parms.nb_antennas_tx       = 1;
    PHY_vars_UE_g[i][0]->frame_parms.nb_antennas_rx       = 1;
    // set initially to 2, it will be revised after initial synchronization
    PHY_vars_UE_g[i][0]->frame_parms.nb_antenna_ports_eNB = 2;
    PHY_vars_UE_g[i][0]->frame_parms.tdd_config = 1;
    PHY_vars_UE_g[i][0]->frame_parms.dl_CarrierFreq       = RC.ru[0]->frame_parms.dl_CarrierFreq;
    PHY_vars_UE_g[i][0]->frame_parms.ul_CarrierFreq       = RC.ru[0]->frame_parms.ul_CarrierFreq;
    PHY_vars_UE_g[i][0]->frame_parms.eutra_band           = RC.ru[0]->frame_parms.eutra_band;
    LOG_I(PHY,"Initializing UE %d frame parameters from RU information: N_RB_DL %d, p %d, dl_Carrierfreq %u, ul_CarrierFreq %u, eutra_band %d\n",
          i,
          PHY_vars_UE_g[i][0]->frame_parms.N_RB_DL,
          PHY_vars_UE_g[i][0]->frame_parms.nb_antenna_ports_eNB,
          PHY_vars_UE_g[i][0]->frame_parms.dl_CarrierFreq,
          PHY_vars_UE_g[i][0]->frame_parms.ul_CarrierFreq,
          PHY_vars_UE_g[i][0]->frame_parms.eutra_band);

*/

    current_UE_rx_timestamp[i][0] = RC.ru[0]->frame_parms.samples_per_tti + RC.ru[0]->frame_parms.ofdm_symbol_size + RC.ru[0]->frame_parms.nb_prefix_samples0;

  }

  for (int ru_id=0;ru_id<RC.nb_RU;ru_id++) current_ru_rx_timestamp[ru_id][0] = RC.ru[ru_id]->frame_parms.samples_per_tti;

  printf("RUs are ready, let's go\n");
}

void wait_eNBs(void)
{
  return;
}


void RCConfig_sim(void) {

  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};

  //  AssertFatal(RC.config_file_name!=NULL,"configuration file is undefined\n");

  // Get num RU instances
  config_getlist( &RUParamList,NULL,0, NULL);
  RC.nb_RU     = RUParamList.numelt;
  
  printf("returned with %d rus\n",RC.nb_RU);
  
  init_RU(NULL);

  printf("Waiting for RUs to get set up\n"); 
  wait_RUs();

  init_ru_devices();

  static int nframes = 100000;
 
  AssertFatal(0 == pthread_create(&rfsim_thread,
                                  NULL,
                                  rfsim_top,
                                  (void*)&nframes), "");

}



int ru_trx_start(openair0_device *device) {
  return(0);
}

void ru_trx_end(openair0_device *device) {
  return;
}

int ru_trx_stop(openair0_device *device) {
  return(0);
}
int UE_trx_start(openair0_device *device) {
  return(0);
}
void UE_trx_end(openair0_device *device) {
  return;
}
int UE_trx_stop(openair0_device *device) {
  return(0);
}
int ru_trx_set_freq(openair0_device *device, openair0_config_t *openair0_cfg, int dummy) {
  return(0);
}
int ru_trx_set_gains(openair0_device *device, openair0_config_t *openair0_cfg) {
  return(0);
}
int UE_trx_set_freq(openair0_device *device, openair0_config_t *openair0_cfg, int dummy) {
  return(0);
}
int UE_trx_set_gains(openair0_device *device, openair0_config_t *openair0_cfg) {
  return(0);
}

extern pthread_mutex_t subframe_mutex;
extern int subframe_ru_mask,subframe_UE_mask;


int ru_trx_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc) {

  int ru_id  = device->Mod_id;
  int CC_id  = device->CC_id;

  int subframe;
  int sample_count=0;

  *ptimestamp = last_ru_rx_timestamp[ru_id][CC_id];


  LOG_D(SIM,"RU_trx_read nsamps %d TS(%llu,%llu) => subframe %d\n",nsamps,
        (unsigned long long)current_ru_rx_timestamp[ru_id][CC_id],
        (unsigned long long)last_ru_rx_timestamp[ru_id][CC_id],
	(int)((*ptimestamp/RC.ru[ru_id]->frame_parms.samples_per_tti)%10));
  // if we're at a subframe boundary generate UL signals for this ru

  while (sample_count<nsamps) {
    while (current_ru_rx_timestamp[ru_id][CC_id]<
	   (nsamps+last_ru_rx_timestamp[ru_id][CC_id])) {
      LOG_D(SIM,"RU: current TS %"PRIi64", last TS %"PRIi64", sleeping\n",current_ru_rx_timestamp[ru_id][CC_id],last_ru_rx_timestamp[ru_id][CC_id]);
      usleep(500);
    }

   
 
    subframe = (last_ru_rx_timestamp[ru_id][CC_id]/RC.ru[ru_id]->frame_parms.samples_per_tti)%10;
    if (subframe_select(&RC.ru[ru_id]->frame_parms,subframe) != SF_DL || RC.ru[ru_id]->frame_parms.frame_type == FDD) { 
      LOG_D(SIM,"RU_trx_read generating UL subframe %d (Ts %llu, current TS %llu)\n",
	    subframe,(unsigned long long)*ptimestamp,
	    (unsigned long long)current_ru_rx_timestamp[ru_id][CC_id]);
      
      do_UL_sig(UE2RU,
		subframe,
		0,  // abstraction_flag
		&RC.ru[ru_id]->frame_parms,
		0,  // frame is only used for abstraction
		ru_id,
		CC_id);
    }
    last_ru_rx_timestamp[ru_id][CC_id] += RC.ru[ru_id]->frame_parms.samples_per_tti;
    sample_count += RC.ru[ru_id]->frame_parms.samples_per_tti;
  }
  

  return(nsamps);
}

int UE_trx_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc)
{
  int UE_id = device->Mod_id;
  int CC_id  = device->CC_id;

  int subframe;
  int sample_count=0;
  int read_size;
  int sptti = PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti;

  *ptimestamp = last_UE_rx_timestamp[UE_id][CC_id];

  LOG_D(SIM,"UE %d DL simulation 0: UE_trx_read nsamps %d TS %llu (%llu, offset %d) antenna %d\n",
        UE_id,
        nsamps,
        (unsigned long long)current_UE_rx_timestamp[UE_id][CC_id],
        (unsigned long long)last_UE_rx_timestamp[UE_id][CC_id],
        (int)(last_UE_rx_timestamp[UE_id][CC_id]%sptti),
	cc);


  if (nsamps < sptti)
    read_size = nsamps;
  else
    read_size = sptti;

  while (sample_count<nsamps) {
    LOG_D(SIM,"UE %d: DL simulation 1: UE_trx_read : current TS now %"PRIi64", last TS %"PRIi64"\n",UE_id,current_UE_rx_timestamp[UE_id][CC_id],last_UE_rx_timestamp[UE_id][CC_id]);
    while (current_UE_rx_timestamp[UE_id][CC_id] < 
	   (last_UE_rx_timestamp[UE_id][CC_id]+read_size)) {
      LOG_D(SIM,"UE %d: DL simulation 2: UE_trx_read : current TS %"PRIi64", last TS %"PRIi64", sleeping\n",UE_id,current_UE_rx_timestamp[UE_id][CC_id],last_UE_rx_timestamp[UE_id][CC_id]);
      usleep(500);
    }
    LOG_D(SIM,"UE %d: DL simulation 3: UE_trx_read : current TS now %"PRIi64", last TS %"PRIi64"\n",UE_id,current_UE_rx_timestamp[UE_id][CC_id],last_UE_rx_timestamp[UE_id][CC_id]);

    // if we cross a subframe-boundary
    subframe = (last_UE_rx_timestamp[UE_id][CC_id]/sptti)%10;

    // tell top-level we are busy 
    pthread_mutex_lock(&subframe_mutex);
    subframe_UE_mask|=(1<<UE_id);
    LOG_D(SIM,"Setting UE_id %d mask to busy (%d)\n",UE_id,subframe_UE_mask);
    pthread_mutex_unlock(&subframe_mutex);
    
    

    LOG_D(PHY,"UE %d: DL simulation 4: UE_trx_read generating DL subframe %d (Ts %llu, current TS %llu,nsamps %d)\n",
	  UE_id,subframe,(unsigned long long)*ptimestamp,
	  (unsigned long long)current_UE_rx_timestamp[UE_id][CC_id],
	  nsamps);

    LOG_D(SIM,"UE %d: DL simulation 5: Doing DL simulation for %d samples starting in subframe %d at offset %d\n",
	  UE_id,nsamps,subframe,
	  (int)(last_UE_rx_timestamp[UE_id][CC_id]%sptti));

    do_DL_sig(RU2UE,
	      subframe,
	      last_UE_rx_timestamp[UE_id][CC_id]%sptti,
	      sptti,
	      0, //abstraction_flag,
	      &PHY_vars_UE_g[UE_id][CC_id]->frame_parms,
	      UE_id,
	      CC_id);
    LOG_D(SIM,"UE %d: DL simulation 6: UE_trx_read @ TS %"PRIi64" (%"PRIi64")=> frame %d, subframe %d\n",
	  UE_id, current_UE_rx_timestamp[UE_id][CC_id],
	  last_UE_rx_timestamp[UE_id][CC_id],
	  (int)((last_UE_rx_timestamp[UE_id][CC_id]/(sptti*10))&1023),
	  subframe);

    last_UE_rx_timestamp[UE_id][CC_id] += read_size;
    sample_count += read_size;
 



  }


  return(nsamps);
}


int ru_trx_write(openair0_device *device,openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {

  int ru_id = device->Mod_id;

  LTE_DL_FRAME_PARMS *frame_parms = &RC.ru[ru_id]->frame_parms;

  pthread_mutex_lock(&subframe_mutex);
  LOG_D(SIM,"[TXPATH] ru_trx_write: RU %d mask %d\n",ru_id,subframe_ru_mask);
  pthread_mutex_unlock(&subframe_mutex); 

  // compute amplitude of TX signal from first symbol in subframe
  // note: assumes that the packet is an entire subframe 

  ru_amp[ru_id] = 0;
  for (int aa=0; aa<RC.ru[ru_id]->nb_tx; aa++) {
    ru_amp[ru_id] += (double)signal_energy((int32_t*)buff[aa],frame_parms->ofdm_symbol_size)/(12*frame_parms->N_RB_DL);
  }
  ru_amp[ru_id] = sqrt(ru_amp[ru_id]);

  LOG_I(PHY,"Setting amp for RU %d to %f (%d)\n",ru_id,ru_amp[ru_id], dB_fixed((double)signal_energy((int32_t*)buff[0],frame_parms->ofdm_symbol_size)));
  // tell top-level we are done
  pthread_mutex_lock(&subframe_mutex);
  subframe_ru_mask|=(1<<ru_id);
  LOG_D(SIM,"Setting RU %d to busy\n",ru_id);
  pthread_mutex_unlock(&subframe_mutex);

  return(nsamps);
}

int UE_trx_write(openair0_device *device,openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {

  return(nsamps);
}

void init_ru_devices(){


  module_id_t ru_id;
  RU_t *ru;

  // allocate memory for RU if not already done
  if (RC.ru==NULL) RC.ru = (RU_t**)malloc(RC.nb_RU*sizeof(RU_t*));

  for (ru_id=0;ru_id<RC.nb_RU;ru_id++) {
    LOG_D(SIM,"Initiaizing rfdevice for RU %d\n",ru_id);
    if (RC.ru[ru_id]==NULL) RC.ru[ru_id] = (RU_t*)malloc(sizeof(RU_t));
    ru               = RC.ru[ru_id];
    ru->rfdevice.Mod_id             = ru_id;
    ru->rfdevice.CC_id              = 0;
    ru->rfdevice.trx_start_func     = ru_trx_start;
    ru->rfdevice.trx_read_func      = ru_trx_read;
    ru->rfdevice.trx_write_func     = ru_trx_write;
    ru->rfdevice.trx_end_func       = ru_trx_end;
    ru->rfdevice.trx_stop_func      = ru_trx_stop;
    ru->rfdevice.trx_set_freq_func  = ru_trx_set_freq;
    ru->rfdevice.trx_set_gains_func = ru_trx_set_gains;
    last_ru_rx_timestamp[ru_id][0] = 0;

  }
}

init_ue_devices() {

  AssertFatal(PHY_vars_UE_g!=NULL,"Top-level structure for UE is null\n");
  for (int UE_id=0;UE_id<NB_UE_INST;UE_id++) {
    AssertFatal(PHY_vars_UE_g[UE_id]!=NULL,"UE %d context is not allocated\n");
    printf("Initializing UE %d\n",UE_id);
    for (int CC_id=0;CC_id<MAX_NUM_CCs;CC_id++) {
        PHY_vars_UE_g[UE_id][CC_id]->rfdevice.Mod_id               = UE_id;
        PHY_vars_UE_g[UE_id][CC_id]->rfdevice.CC_id                = CC_id;
        PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_start_func       = UE_trx_start;
        PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_read_func        = UE_trx_read;
        PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_write_func       = UE_trx_write;
        PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_end_func         = UE_trx_end;
        PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_stop_func        = UE_trx_stop;
        PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_set_freq_func    = UE_trx_set_freq;
        PHY_vars_UE_g[UE_id][CC_id]->rfdevice.trx_set_gains_func   = UE_trx_set_gains;
        last_UE_rx_timestamp[UE_id][CC_id] = 0;
    }
  }
}

void init_ocm(double snr_dB,double sinr_dB)
{
  module_id_t UE_id, ru_id;
  int CC_id;

  randominit(0);
  set_taus_seed(0);


  init_channel_vars ();//fp, &s_re, &s_im, &r_re, &r_im, &r_re0, &r_im0);

  // initialize channel descriptors
  LOG_I(PHY,"Initializing channel descriptors (nb_RU %d, nb_UE %d)\n",RC.nb_RU,NB_UE_INST);
  for (ru_id = 0; ru_id < RC.nb_RU; ru_id++) {
    for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

	LOG_I(PHY,"Initializing channel descriptors (RU %d, UE %d) for N_RB_DL %d\n",ru_id,UE_id,
	      RC.ru[ru_id]->frame_parms.N_RB_DL);
        RU2UE[ru_id][UE_id][CC_id] = 
	  new_channel_desc_scm(RC.ru[ru_id]->nb_tx,
			       PHY_vars_UE_g[UE_id][CC_id]->frame_parms.nb_antennas_rx,
			       AWGN,
			       N_RB2sampling_rate(RC.ru[ru_id]->frame_parms.N_RB_DL),
			       N_RB2channel_bandwidth(RC.ru[ru_id]->frame_parms.N_RB_DL),
			       0.0,
			       0,
			       0);
        random_channel(RU2UE[ru_id][UE_id][CC_id],0);
        LOG_D(OCM,"[SIM] Initializing channel (%s) from UE %d to ru %d\n", "AWGN",UE_id, ru_id);


        UE2RU[UE_id][ru_id][CC_id] = 
	  new_channel_desc_scm(PHY_vars_UE_g[UE_id][CC_id]->frame_parms.nb_antennas_tx,
			       RC.ru[ru_id]->nb_rx,
			       AWGN,
			       N_RB2sampling_rate(RC.ru[ru_id]->frame_parms.N_RB_UL),
			       N_RB2channel_bandwidth(RC.ru[ru_id]->frame_parms.N_RB_UL),
			       0.0,
			       0,
			       0);

        random_channel(UE2RU[UE_id][ru_id][CC_id],0);

        // to make channel reciprocal uncomment following line instead of previous. However this only works for SISO at the moment. For MIMO the channel would need to be transposed.
        //UE2RU[UE_id][ru_id] = RU2UE[ru_id][UE_id];

	AssertFatal(RU2UE[ru_id][UE_id][CC_id]!=NULL,"RU2UE[%d][%d][%d] is null\n",ru_id,UE_id,CC_id);
	AssertFatal(UE2RU[UE_id][ru_id][CC_id]!=NULL,"UE2RU[%d][%d][%d] is null\n",UE_id,ru_id,CC_id);
	//pathloss: -132.24 dBm/15kHz RE + target SNR - eNB TX power per RE
	if (ru_id == (UE_id % RC.nb_RU)) {
	  RU2UE[ru_id][UE_id][CC_id]->path_loss_dB = -132.24 + snr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower;
	  UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = -132.24 + snr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower; 
	} else {
	  RU2UE[ru_id][UE_id][CC_id]->path_loss_dB = -132.24 + sinr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower;
	  UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = -132.24 + sinr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower;
	}
	
	LOG_D(OCM,"Path loss from eNB %d to UE %d (CCid %d)=> %f dB (eNB TX %d, SNR %f)\n",ru_id,UE_id,CC_id,
	      RU2UE[ru_id][UE_id][CC_id]->path_loss_dB,
	      RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower,snr_dB);
	

      }
    }
  }
}


void update_ocm(double snr_dB,double sinr_dB)
{
  module_id_t UE_id, ru_id;
  int CC_id;


   for (ru_id = 0; ru_id < RC.nb_RU; ru_id++) {
     for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
       for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {

         AssertFatal(RU2UE[ru_id][UE_id][CC_id]!=NULL,"RU2UE[%d][%d][%d] is null\n",ru_id,UE_id,CC_id);
	 AssertFatal(UE2RU[UE_id][ru_id][CC_id]!=NULL,"UE2RU[%d][%d][%d] is null\n",UE_id,ru_id,CC_id);
	 //pathloss: -132.24 dBm/15kHz RE + target SNR - eNB TX power per RE
	 if (ru_id == (UE_id % RC.nb_RU)) {
	   RU2UE[ru_id][UE_id][CC_id]->path_loss_dB = -132.24 + snr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower;
	   UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = -132.24 + snr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower; 
	 } else {
	   RU2UE[ru_id][UE_id][CC_id]->path_loss_dB = -132.24 + sinr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower;
	   UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = -132.24 + sinr_dB - RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower;
	 }
	    
	 LOG_D(OCM,"Path loss from eNB %d to UE %d (CCid %d)=> %f dB (eNB TX %d, SNR %f)\n",ru_id,UE_id,CC_id,
	      RU2UE[ru_id][UE_id][CC_id]->path_loss_dB,
	      RC.ru[ru_id]->frame_parms.pdsch_config_common.referenceSignalPower,snr_dB);
	    
       }
     }
   }
}


void init_channel_vars(void)
{

  int i;

  memset(RU_output_mask,0,sizeof(int)*NUMBER_OF_UE_MAX);
  for (i=0;i<NB_UE_INST;i++)
    pthread_mutex_init(&RU_output_mutex[i],NULL);

  memset(UE_output_mask,0,sizeof(int)*NUMBER_OF_RU_MAX);
  for (i=0;i<RC.nb_RU;i++)
    pthread_mutex_init(&UE_output_mutex[i],NULL);

}



void rfsim_top(void *n_frames) {


  wait_sync("rfsim_top");


  printf("Running rfsim with %d frames\n",*(int*)n_frames);
  for (int frame = 0;
       frame < *(int*)n_frames;
       frame++) {

    for (int sf = 0; sf < 10; sf++) {
      int CC_id=0;
      int all_done=0;
      while (all_done==0) {
	
	pthread_mutex_lock(&subframe_mutex);

	int subframe_ru_mask_local  = (subframe_select(&RC.ru[0]->frame_parms,(sf+4)%10)!=SF_UL) ? subframe_ru_mask : ((1<<RC.nb_RU)-1);
	int subframe_UE_mask_local  = (RC.ru[0]->frame_parms.frame_type == FDD || subframe_select(&RC.ru[0]->frame_parms,(sf+4)%10)!=SF_DL) ? subframe_UE_mask : ((1<<NB_UE_INST)-1);
	pthread_mutex_unlock(&subframe_mutex);
	LOG_D(SIM,"Frame %d, Subframe %d, NB_RU %d, NB_UE %d: Checking masks %x,%x\n",frame,sf,RC.nb_RU,NB_UE_INST,subframe_ru_mask_local,subframe_UE_mask_local);
	if ((subframe_ru_mask_local == ((1<<RC.nb_RU)-1)) &&
	    (subframe_UE_mask_local == ((1<<NB_UE_INST)-1))) all_done=1;
	else usleep(1500);
      }
      
      
      //clear subframe masks for next round
      pthread_mutex_lock(&subframe_mutex);
      subframe_ru_mask=0;
      subframe_UE_mask=0;
      pthread_mutex_unlock(&subframe_mutex);
      
      // increment timestamps
      for (int ru_id=0;ru_id<RC.nb_RU;ru_id++) {
	current_ru_rx_timestamp[ru_id][CC_id] += RC.ru[ru_id]->frame_parms.samples_per_tti;
	LOG_D(SIM,"RU %d/%d: TS %"PRIi64"\n",ru_id,CC_id,current_ru_rx_timestamp[ru_id][CC_id]);
      }
      for (int UE_inst = 0; UE_inst<NB_UE_INST;UE_inst++) {
	current_UE_rx_timestamp[UE_inst][CC_id] += PHY_vars_UE_g[UE_inst][CC_id]->frame_parms.samples_per_tti;
	LOG_D(SIM,"UE %d/%d: TS %"PRIi64"\n",UE_inst,CC_id,current_UE_rx_timestamp[UE_inst][CC_id]);
      }
      if (oai_exit == 1) return;
    }
  }

}
