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
#include "PHY/defs_RU.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

RAN_CONTEXT_t RC;
extern PHY_VARS_UE ***PHY_vars_UE_g;


// put all of these in a common structure after
sim_t sim;


void init_ru_devices(void);

void init_RU(char *,int send_dmrssync);

void *rfsim_top(void *n_frames);

void wait_RUs(void) {
  int i;
  // wait for all RUs to be configured over fronthaul
  pthread_mutex_lock(&RC.ru_mutex);

  while (RC.ru_mask>0) {
    pthread_cond_wait(&RC.ru_cond,&RC.ru_mutex);
  }

  pthread_mutex_unlock(&RC.ru_mutex);

  // copy frame parameters from RU to UEs
  for (i=0; i<NB_UE_INST; i++) {
    sim.current_UE_rx_timestamp[i][0] = RC.ru[0]->frame_parms->samples_per_tti + RC.ru[0]->frame_parms->ofdm_symbol_size + RC.ru[0]->frame_parms->nb_prefix_samples0;
  }

  for (int ru_id=0; ru_id<RC.nb_RU; ru_id++) sim.current_ru_rx_timestamp[ru_id][0] = RC.ru[ru_id]->frame_parms->samples_per_tti;

  printf("RUs are ready, let's go\n");
}

void wait_eNBs(void) {
  return;
}


void RCConfig_sim(void) {
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};
  // Get num RU instances
  config_getlist( &RUParamList,NULL,0, NULL);
  RC.nb_RU     = RUParamList.numelt;
  AssertFatal(RC.nb_RU>0,"we need at least 1 RU for simulation\n");
  printf("returned with %d rus\n",RC.nb_RU);
  init_RU(NULL,0);
  printf("Waiting for RUs to get set up\n");
  wait_RUs();
  init_ru_devices();
  static int nframes = 100000;
  AssertFatal(0 == pthread_create(&sim.rfsim_thread,
                                  NULL,
                                  rfsim_top,
                                  (void *)&nframes), "");
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
  *ptimestamp = sim.last_ru_rx_timestamp[ru_id][CC_id];
  LOG_D(SIM,"RU_trx_read nsamps %d TS(%llu,%llu) => subframe %d\n",nsamps,
        (unsigned long long)sim.current_ru_rx_timestamp[ru_id][CC_id],
        (unsigned long long)sim.last_ru_rx_timestamp[ru_id][CC_id],
        (int)((*ptimestamp/RC.ru[ru_id]->frame_parms->samples_per_tti)%10));

  // if we're at a subframe boundary generate UL signals for this ru

  while (sample_count<nsamps) {
    while (sim.current_ru_rx_timestamp[ru_id][CC_id]<
           (nsamps+sim.last_ru_rx_timestamp[ru_id][CC_id])) {
      LOG_D(SIM,"RU: current TS %"PRIi64", last TS %"PRIi64", sleeping\n",sim.current_ru_rx_timestamp[ru_id][CC_id],sim.last_ru_rx_timestamp[ru_id][CC_id]);
      usleep(500);
    }

    subframe = (sim.last_ru_rx_timestamp[ru_id][CC_id]/RC.ru[ru_id]->frame_parms->samples_per_tti)%10;

    if (subframe_select(RC.ru[ru_id]->frame_parms,subframe) != SF_DL || RC.ru[ru_id]->frame_parms->frame_type == FDD) { 
      LOG_D(SIM,"RU_trx_read generating UL subframe %d (Ts %llu, current TS %llu)\n",
            subframe,(unsigned long long)*ptimestamp,
            (unsigned long long)sim.current_ru_rx_timestamp[ru_id][CC_id]);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SIM_DO_UL_SIGNAL,1);
      do_UL_sig(&sim,
                subframe,
                0,  // abstraction_flag
                RC.ru[ru_id]->frame_parms,
                0,  // frame is only used for abstraction
                ru_id,
                CC_id);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SIM_DO_UL_SIGNAL,0);
    }

    sim.last_ru_rx_timestamp[ru_id][CC_id] += RC.ru[ru_id]->frame_parms->samples_per_tti;
    sample_count += RC.ru[ru_id]->frame_parms->samples_per_tti;
  }

  return(nsamps);
}

int UE_trx_read(openair0_device *device, openair0_timestamp *ptimestamp, void **buff, int nsamps, int cc) {
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SIM_UE_TRX_READ,1);
  int UE_id = device->Mod_id;
  int CC_id  = device->CC_id;
  int subframe;
  int sample_count=0;
  int read_size;
  int sptti = PHY_vars_UE_g[UE_id][CC_id]->frame_parms.samples_per_tti;
  *ptimestamp = sim.last_UE_rx_timestamp[UE_id][CC_id];
  LOG_D(PHY,"UE %d DL simulation 0: UE_trx_read nsamps %d TS %llu (%llu, offset %d) antenna %d\n",
        UE_id,
        nsamps,
        (unsigned long long)sim.current_UE_rx_timestamp[UE_id][CC_id],
        (unsigned long long)sim.last_UE_rx_timestamp[UE_id][CC_id],
        (int)(sim.last_UE_rx_timestamp[UE_id][CC_id]%sptti),
        cc);

  if (nsamps < sptti)
    read_size = nsamps;
  else
    read_size = sptti;

  while (sample_count<nsamps) {
    LOG_D(SIM,"UE %d: DL simulation 1: UE_trx_read : current TS now %"PRIi64", last TS %"PRIi64"\n",UE_id,sim.current_UE_rx_timestamp[UE_id][CC_id],sim.last_UE_rx_timestamp[UE_id][CC_id]);

    while (sim.current_UE_rx_timestamp[UE_id][CC_id] <
           (sim.last_UE_rx_timestamp[UE_id][CC_id]+read_size)) {
      LOG_D(SIM,"UE %d: DL simulation 2: UE_trx_read : current TS %"PRIi64", last TS %"PRIi64", sleeping\n",UE_id,sim.current_UE_rx_timestamp[UE_id][CC_id],sim.last_UE_rx_timestamp[UE_id][CC_id]);
      usleep(500);
    }

    LOG_D(SIM,"UE %d: DL simulation 3: UE_trx_read : current TS now %"PRIi64", last TS %"PRIi64"\n",UE_id,sim.current_UE_rx_timestamp[UE_id][CC_id],sim.last_UE_rx_timestamp[UE_id][CC_id]);
    // if we cross a subframe-boundary
    subframe = (sim.last_UE_rx_timestamp[UE_id][CC_id]/sptti)%10;
    // tell top-level we are busy
    pthread_mutex_lock(&sim.subframe_mutex);
    sim.subframe_UE_mask|=(1<<UE_id);
    LOG_D(SIM,"Setting UE_id %d mask to busy (%d)\n",UE_id,sim.subframe_UE_mask);
    pthread_mutex_unlock(&sim.subframe_mutex);
    LOG_D(PHY,"UE %d: DL simulation 4: UE_trx_read generating DL subframe %d (Ts %llu, current TS %llu,nsamps %d)\n",
          UE_id,subframe,(unsigned long long)*ptimestamp,
          (unsigned long long)sim.current_UE_rx_timestamp[UE_id][CC_id],
          nsamps);
    LOG_D(SIM,"UE %d: DL simulation 5: Doing DL simulation for %d samples starting in subframe %d at offset %d\n",
          UE_id,nsamps,subframe,
          (int)(sim.last_UE_rx_timestamp[UE_id][CC_id]%sptti));
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SIM_DO_DL_SIGNAL,1);
    do_DL_sig(&sim,
              subframe,
              sim.last_UE_rx_timestamp[UE_id][CC_id]%sptti,
              sptti,
              0, //abstraction_flag,
              &PHY_vars_UE_g[UE_id][CC_id]->frame_parms,
              UE_id,
              CC_id);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SIM_DO_DL_SIGNAL,0);
    LOG_D(PHY,"UE %d: DL simulation 6: UE_trx_read @ TS %"PRIi64" (%"PRIi64")=> frame %d, subframe %d\n",
          UE_id, sim.current_UE_rx_timestamp[UE_id][CC_id],
          sim.last_UE_rx_timestamp[UE_id][CC_id],
          (int)((sim.last_UE_rx_timestamp[UE_id][CC_id]/(sptti*10))&1023),
          subframe);
    sim.last_UE_rx_timestamp[UE_id][CC_id] += read_size;
    sample_count += read_size;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SIM_UE_TRX_READ,0);
  return(nsamps);
}


int ru_trx_write(openair0_device *device,openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {
  int ru_id = device->Mod_id;
  LTE_DL_FRAME_PARMS *frame_parms = RC.ru[ru_id]->frame_parms;
  pthread_mutex_lock(&sim.subframe_mutex);
  LOG_D(SIM,"[TXPATH] ru_trx_write: RU %d mask %d\n",ru_id,sim.subframe_ru_mask);
  pthread_mutex_unlock(&sim.subframe_mutex);
  // compute amplitude of TX signal from first symbol in subframe
  // note: assumes that the packet is an entire subframe
  sim.ru_amp[ru_id] = 0;

  for (int aa=0; aa<RC.ru[ru_id]->nb_tx; aa++) {
    sim.ru_amp[ru_id] += (double)signal_energy((int32_t *)buff[aa],frame_parms->ofdm_symbol_size)/(12*frame_parms->N_RB_DL);
  }

  sim.ru_amp[ru_id] = sqrt(sim.ru_amp[ru_id]);
  LOG_D(PHY,"Setting amp for RU %d to %f (%d)\n",ru_id,sim.ru_amp[ru_id], dB_fixed((double)signal_energy((int32_t *)buff[0],frame_parms->ofdm_symbol_size)));
  // tell top-level we are done
  pthread_mutex_lock(&sim.subframe_mutex);
  sim.subframe_ru_mask|=(1<<ru_id);
  LOG_D(SIM,"Setting RU %d to busy\n",ru_id);
  pthread_mutex_unlock(&sim.subframe_mutex);
  return(nsamps);
}

int UE_trx_write(openair0_device *device,openair0_timestamp timestamp, void **buff, int nsamps, int cc, int flags) {
  return(nsamps);
}

void init_ru_devices() {
  module_id_t ru_id;
  RU_t *ru;

  // allocate memory for RU if not already done
  if (RC.ru==NULL) RC.ru = (RU_t **)malloc(RC.nb_RU*sizeof(RU_t *));

  for (ru_id=0; ru_id<RC.nb_RU; ru_id++) {
    LOG_D(SIM,"Initiaizing rfdevice for RU %d\n",ru_id);

    if (RC.ru[ru_id]==NULL) RC.ru[ru_id] = (RU_t *)malloc(sizeof(RU_t));

    ru                                 = RC.ru[ru_id];
    ru->rfdevice.Mod_id                = ru_id;
    ru->rfdevice.CC_id                 = 0;
    ru->rfdevice.trx_start_func        = ru_trx_start;
    ru->rfdevice.trx_read_func         = ru_trx_read;
    ru->rfdevice.trx_write_func        = ru_trx_write;
    ru->rfdevice.trx_end_func          = ru_trx_end;
    ru->rfdevice.trx_stop_func         = ru_trx_stop;
    ru->rfdevice.trx_set_freq_func     = ru_trx_set_freq;
    ru->rfdevice.trx_set_gains_func    = ru_trx_set_gains;
    sim.last_ru_rx_timestamp[ru_id][0] = 0;
  }
}

void init_ue_devices(PHY_VARS_UE *UE) {
  AssertFatal(UE!=NULL,"UE context is not allocated\n");
  printf("Initializing UE %d.%d\n",UE->Mod_id,UE->CC_id);
  UE->rfdevice.Mod_id               = UE->Mod_id;
  UE->rfdevice.CC_id                = UE->CC_id;
  UE->rfdevice.trx_start_func       = UE_trx_start;
  UE->rfdevice.trx_read_func        = UE_trx_read;
  UE->rfdevice.trx_write_func       = UE_trx_write;
  UE->rfdevice.trx_end_func         = UE_trx_end;
  UE->rfdevice.trx_stop_func        = UE_trx_stop;
  UE->rfdevice.trx_set_freq_func    = UE_trx_set_freq;
  UE->rfdevice.trx_set_gains_func   = UE_trx_set_gains;
  sim.last_UE_rx_timestamp[UE->Mod_id][UE->CC_id] = 0;
}

void init_ocm(void) {
  module_id_t UE_id, ru_id;
  int CC_id;
  double DS_TDL = .03;
  randominit(0);
  set_taus_seed(0);
  init_channelmod();
  double snr_dB  = channelmod_get_snr_dB();
  double sinr_dB = channelmod_get_sinr_dB();
  init_channel_vars ();//fp, &s_re, &s_im, &r_re, &r_im, &r_re0, &r_im0);
  // initialize channel descriptors
  LOG_I(PHY,"Initializing channel descriptors (nb_RU %d, nb_UE %d)\n",RC.nb_RU,NB_UE_INST);

  for (ru_id = 0; ru_id < RC.nb_RU; ru_id++) {
    for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        LOG_I(PHY,"Initializing channel descriptors (RU %d, UE %d) for N_RB_DL %d\n",ru_id,UE_id,
              RC.ru[ru_id]->frame_parms->N_RB_DL);
        sim.RU2UE[ru_id][UE_id][CC_id] = 
          new_channel_desc_scm(RC.ru[ru_id]->nb_tx,
                               PHY_vars_UE_g[UE_id][CC_id]->frame_parms.nb_antennas_rx,
                               AWGN,
                               N_RB2sampling_rate(RC.ru[ru_id]->frame_parms->N_RB_DL),
                               N_RB2channel_bandwidth(RC.ru[ru_id]->frame_parms->N_RB_DL),
                               DS_TDL,
                               0.0,
                               0,
                               0,
                               0);
        random_channel(sim.RU2UE[ru_id][UE_id][CC_id],0);
        LOG_D(OCM,"[SIM] Initializing channel (%s) from UE %d to ru %d\n", "AWGN", UE_id, ru_id);

        sim.UE2RU[UE_id][ru_id][CC_id] =
          new_channel_desc_scm(PHY_vars_UE_g[UE_id][CC_id]->frame_parms.nb_antennas_tx,
                               RC.ru[ru_id]->nb_rx,
                               AWGN,
                               N_RB2sampling_rate(RC.ru[ru_id]->frame_parms->N_RB_UL),
                               N_RB2channel_bandwidth(RC.ru[ru_id]->frame_parms->N_RB_UL),
                               DS_TDL,
                               0.0,
                               0,
                               0,
                               0);
        random_channel(sim.UE2RU[UE_id][ru_id][CC_id],0);
        // to make channel reciprocal uncomment following line instead of previous. However this only works for SISO at the moment. For MIMO the channel would need to be transposed.
        //UE2RU[UE_id][ru_id] = RU2UE[ru_id][UE_id];
        AssertFatal(sim.RU2UE[ru_id][UE_id][CC_id]!=NULL,"RU2UE[%d][%d][%d] is null\n",ru_id,UE_id,CC_id);
        AssertFatal(sim.UE2RU[UE_id][ru_id][CC_id]!=NULL,"UE2RU[%d][%d][%d] is null\n",UE_id,ru_id,CC_id);

        //pathloss: -132.24 dBm/15kHz RE + target SNR - eNB TX power per RE
        if (ru_id == (UE_id % RC.nb_RU)) {
          sim.RU2UE[ru_id][UE_id][CC_id]->path_loss_dB = -132.24 + snr_dB - RC.ru[ru_id]->frame_parms->pdsch_config_common.referenceSignalPower;
          sim.UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = -132.24 + snr_dB - RC.ru[ru_id]->frame_parms->pdsch_config_common.referenceSignalPower;
        } else {
          sim.RU2UE[ru_id][UE_id][CC_id]->path_loss_dB = -132.24 + sinr_dB - RC.ru[ru_id]->frame_parms->pdsch_config_common.referenceSignalPower;
          sim.UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = -132.24 + sinr_dB - RC.ru[ru_id]->frame_parms->pdsch_config_common.referenceSignalPower;
        }

        LOG_I(OCM,"Path loss from eNB %d to UE %d (CCid %d)=> %f dB (eNB TX %d, SNR %f)\n",ru_id,UE_id,CC_id,
              sim.RU2UE[ru_id][UE_id][CC_id]->path_loss_dB,
              RC.ru[ru_id]->frame_parms->pdsch_config_common.referenceSignalPower,
              snr_dB);
      }
    }
  }
}


void update_ocm(double snr_dB,double sinr_dB) {
  module_id_t UE_id, ru_id;
  int CC_id;

  for (ru_id = 0; ru_id < RC.nb_RU; ru_id++) {
    for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
      for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
        AssertFatal(sim.RU2UE[ru_id][UE_id][CC_id]!=NULL,"RU2UE[%d][%d][%d] is null\n",ru_id,UE_id,CC_id);
        AssertFatal(sim.UE2RU[UE_id][ru_id][CC_id]!=NULL,"UE2RU[%d][%d][%d] is null\n",UE_id,ru_id,CC_id);

        //pathloss: -132.24 dBm/15kHz RE + target SNR - eNB TX power per RE
        if (ru_id == (UE_id % RC.nb_RU)) {
          sim.RU2UE[ru_id][UE_id][CC_id]->path_loss_dB = -132.24 + snr_dB - RC.ru[ru_id]->frame_parms->pdsch_config_common.referenceSignalPower;
          sim.UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = -132.24 + snr_dB - RC.ru[ru_id]->frame_parms->pdsch_config_common.referenceSignalPower;
        } else {
          sim.RU2UE[ru_id][UE_id][CC_id]->path_loss_dB = -132.24 + sinr_dB - RC.ru[ru_id]->frame_parms->pdsch_config_common.referenceSignalPower;
          sim.UE2RU[UE_id][ru_id][CC_id]->path_loss_dB = -132.24 + sinr_dB - RC.ru[ru_id]->frame_parms->pdsch_config_common.referenceSignalPower;
        }
	    
        LOG_D(OCM,"Path loss from eNB %d to UE %d (CCid %d)=> %f dB (eNB TX %d, SNR %f)\n",ru_id,UE_id,CC_id,
              sim.RU2UE[ru_id][UE_id][CC_id]->path_loss_dB,
              RC.ru[ru_id]->frame_parms->pdsch_config_common.referenceSignalPower,
              snr_dB);
      }
    }
  }
}


void init_channel_vars(void) {
  int i;
  memset(sim.RU_output_mask,0,sizeof(int)*NUMBER_OF_UE_MAX);

  for (i=0; i<NB_UE_INST; i++)
    pthread_mutex_init(&sim.RU_output_mutex[i],NULL);

  memset(sim.UE_output_mask,0,sizeof(int)*NUMBER_OF_RU_MAX);

  for (i=0; i<RC.nb_RU; i++)
    pthread_mutex_init(&sim.UE_output_mutex[i],NULL);
}


void *rfsim_top(void *n_frames) {
  wait_sync("rfsim_top");
  printf("Running rfsim with %d frames\n",*(int *)n_frames);

  for (int frame = 0; frame < *(int *)n_frames; frame++) {
    for (int sf = 0; sf < 10; sf++) {
      int CC_id=0;
      int all_done=0;

      while (all_done==0) {
        pthread_mutex_lock(&sim.subframe_mutex);
        int subframe_ru_mask_local  = (subframe_select(RC.ru[0]->frame_parms,(sf+4)%10)!=SF_UL) ? sim.subframe_ru_mask : ((1<<RC.nb_RU)-1);
        int subframe_UE_mask_local  = (RC.ru[0]->frame_parms->frame_type == FDD || subframe_select(RC.ru[0]->frame_parms,(sf+4)%10)!=SF_DL) ? sim.subframe_UE_mask : ((1<<NB_UE_INST)-1);
        pthread_mutex_unlock(&sim.subframe_mutex);
        LOG_D(SIM,"Frame %d, Subframe %d, NB_RU %d, NB_UE %d: Checking masks %x,%x\n",frame,sf,RC.nb_RU,NB_UE_INST,subframe_ru_mask_local,subframe_UE_mask_local);

        if ((subframe_ru_mask_local == ((1<<RC.nb_RU)-1)) &&
            (subframe_UE_mask_local == ((1<<NB_UE_INST)-1))) all_done=1;
        else usleep(1500);
      }

      //clear subframe masks for next round
      pthread_mutex_lock(&sim.subframe_mutex);
      sim.subframe_ru_mask=0;
      sim.subframe_UE_mask=0;
      pthread_mutex_unlock(&sim.subframe_mutex);

      // increment timestamps

      for (int ru_id=0; ru_id<RC.nb_RU; ru_id++) {
        sim.current_ru_rx_timestamp[ru_id][CC_id] += RC.ru[ru_id]->frame_parms->samples_per_tti;
        LOG_D(SIM,"RU %d/%d: TS %"PRIi64"\n",ru_id,CC_id,sim.current_ru_rx_timestamp[ru_id][CC_id]);
      }

      for (int UE_inst = 0; UE_inst<NB_UE_INST; UE_inst++) {
        sim.current_UE_rx_timestamp[UE_inst][CC_id] += PHY_vars_UE_g[UE_inst][CC_id]->frame_parms.samples_per_tti;
        LOG_D(SIM,"UE %d/%d: TS %"PRIi64"\n",UE_inst,CC_id,sim.current_UE_rx_timestamp[UE_inst][CC_id]);
      }

      if (oai_exit == 1) return((void *)NULL);
    }
  }

  return((void *)NULL);
}
