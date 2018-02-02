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

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "MAC_INTERFACE/extern.h"

#ifdef OPENAIR2
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log_if.h"
#include "RRC/LITE/extern.h"
#include "PHY_INTERFACE/extern.h"
#endif

#include "../USER/oaisim.h"

#include "channel_sim_proc.h"
#include "interface.h"
#include "Tsync.h"
#include "Process.h"
#define RF
//#define DEBUG_SIM


void init_rre(LTE_DL_FRAME_PARMS *frame_parms,double ***r_re0,double ***r_im0)
{

  int i;

  (*r_re0)  = malloc(2*sizeof(double*));
  (*r_im0)  = malloc(2*sizeof(double*));

  for (i=0; i<2; i++) {
    (*r_re0)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*r_re0)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    (*r_im0)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*r_im0)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
  }


}

void mmap_enb(int id,int **tx_data[3],int **rx_data[3],LTE_DL_FRAME_PARMS *frame_parms)
{


  int i;
  int fd[20];
  int result;
  unsigned int FILESIZE;

  FILESIZE=FRAME_LENGTH_COMPLEX_SAMPLES*(sizeof(s32));

  char buffer[100];
  int j=0,k=0;

  char sect=1;

  for(k=0; k<sect; k++) {
    (*(tx_data+k)) = malloc(2*sizeof(s32*));
    (*(rx_data+k)) = malloc(2*sizeof(s32*));
  }

  for (k=0; k<sect; k++) {
    for (i=0; i<2; i++) {
      sprintf(buffer,"/tmp/tx_data_%d_%d_%d.bin",id,k,i);
      fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
      result = lseek(fd[j], FILESIZE-1, SEEK_SET);
      result = write(fd[j], "", 1);
      (*(tx_data+k))[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);
      j++;

      sprintf(buffer,"/tmp/rx_data_%d_%d_%d.bin",id,k,i);
      fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
      result = lseek(fd[j], FILESIZE-1, SEEK_SET);
      result = write(fd[j], "", 1);
      (*(rx_data+k))[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);
      j++;
    }
  }


}

void mmap_ue(int id,int ***tx_data,int ***rx_data,LTE_DL_FRAME_PARMS *frame_parms)
{

  int i;
  int fd[20];
  int result;
  unsigned int FILESIZE;

  FILESIZE=FRAME_LENGTH_COMPLEX_SAMPLES*(sizeof(s32));

  char buffer[100];
  int j=0,k=0;

  (*tx_data) = malloc(2*sizeof(s32*));
  (*rx_data) = malloc(2*sizeof(s32*));

  for (i=0; i<2; i++) {
    sprintf(buffer,"/tmp/tx_data_%d_%d_%d.bin",id,k,i);
    fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    result = lseek(fd[j], FILESIZE-1, SEEK_SET);
    result = write(fd[j], "", 1);
    (*tx_data)[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);
    j++;

    sprintf(buffer,"/tmp/rx_data_%d_%d_%d.bin",id,k,i);
    fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    result = lseek(fd[j], FILESIZE-1, SEEK_SET);
    result = write(fd[j], "", 1);
    (*rx_data)[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);
    j++;
  }

}

void do_DL_sig_channel_T(void *param)
{

  ch_thread* cthread = (ch_thread*)param;
  int count;
  u8 eNB_i,UE_i;
  int thread_id;
  double **r_re0,**r_re,**r_im,**r_im0,**s_re,**s_im;
  channel_desc_t *eNB2UE[3][8];
  node_desc_t *enb_data[3];
  node_desc_t *ue_data[8];
  int *next_slot,abstraction_flag;
  LTE_DL_FRAME_PARMS *frame_parms;
  int **tx_data[3],**rx_data[3];
  node_desc_t *enb_data_2[3];
  eNB_i=(cthread->eNB_id);
  UE_i=(cthread->UE_id);
  r_re0=cthread->r_re0;
  r_re=cthread->r_re;
  r_im=cthread->r_im;
  r_im0=cthread->r_im0;
  s_im=cthread->s_im;
  s_re=cthread->s_re;
  eNB2UE[eNB_i][UE_i]=cthread->eNB2UE;
  enb_data[0]=(cthread->enb_data);
  enb_data_2[eNB_i]=cthread->enb_data;

  ue_data[UE_i]=cthread->ue_data;
  frame_parms=cthread->frame_parms;
  next_slot=(cthread->next_slot);
  abstraction_flag=*(cthread->abstraction_flag);
  thread_id=(cthread->thread_id);

  for (int i=0;i<3;i++)
  {
      tx_data[i]=cthread->tx_data[i];
      rx_data[i]=cthread->rx_data[i];
  }

  s32 att_eNB_id=-1;
  s32 **txdata,**rxdata;

  double tx_pwr, rx_pwr;
  s32 rx_pwr2;
  u32 i,aa;
  u32 slot_offset;

  while(1) {

    if(pthread_cond_wait(&downlink_cond[eNB_i][UE_i], &downlink_mutex[eNB_i][UE_i])) exit(1);

    slot_offset = (nslot)*(frame_parms->samples_per_tti>>1);
    //   pthread_mutex_lock(&exclusive);

    multipath_channel(eNB2UE[eNB_i][UE_i],s_re,s_im,r_re0,r_im0,
                      frame_parms->samples_per_tti>>1,0);


    rx_pwr = signal_energy_fp2(eNB2UE[eNB_i][UE_i]->ch[0],eNB2UE[eNB_i][UE_i]->channel_length)*eNB2UE[eNB_i][UE_i]->channel_length;

    printf("[SIM][DL] Channel eNB %d => UE %d : tx_power %f dBm, path_loss %f dB\n",
           eNB_i,UE_i,
           enb_data_2[eNB_i]->tx_power_dBm,
           eNB2UE[eNB_i][UE_i]->path_loss_dB);
    printf("[SIM][DL] Channel eNB %d => UE %d : Channel gain %f dB (%f)\n",eNB_i,UE_i,10*log10(rx_pwr),rx_pwr);

    rx_pwr = signal_energy_fp(r_re0,r_im0,frame_parms->nb_antennas_rx,frame_parms->samples_per_tti>>1,0);

    printf("[SIM][DL] UE %d : rx_pwr %f dB for slot %d (subframe %d)\n",UE_i,10*log10(rx_pwr),nslot,nslot>>1);

    if (eNB2UE[eNB_i][UE_i]->first_run == 1)
      eNB2UE[eNB_i][UE_i]->first_run = 0;

    rf_rx(r_re0,
          r_im0,
          NULL,
          NULL,
          0,
          frame_parms->nb_antennas_rx,
          frame_parms->samples_per_tti>>1,
          1e3/eNB2UE[eNB_i][UE_i]->BW,  // sampling time (ns)
          0.0,               // freq offset (Hz) (-20kHz..20kHz)
          0.0,               // drift (Hz) NOT YET IMPLEMENTED
          ue_data[UE_i]->rx_noise_level,                // noise_figure NOT YET IMPLEMENTED
          110.00 - 66.227,   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)
          200,               // IP3_dBm (dBm)
          &eNB2UE[eNB_i][UE_i]->ip,               // initial phase
          30.0e3,            // pn_cutoff (kHz)
          -500.0,            // pn_amp (dBc) default: 50
          0.0,               // IQ imbalance (dB),
          0.0);              // IQ phase imbalance (rad)

    rx_pwr = signal_energy_fp(r_re0,r_im0,frame_parms->nb_antennas_rx,frame_parms->samples_per_tti>>1,0);

    printf("[SIM][DL] UE %d : ADC in (eNB %d) %f dB for slot %d (subframe %d)\n",UE_i,eNB_i,10*log10(rx_pwr),nslot,nslot>>1);
    //count = __sync_add_and_fetch(&_COT, 1);
    //COMPILER_BARRIER();

    if (pthread_mutex_lock(&downlink_mutex_channel)) exit(1);

    _COT++;

    if(_COT==NUM_THREAD_DOWNLINK) {
      //  if(count==NUM_THREAD_DOWNLINK){
      if(pthread_cond_signal(&downlink_cond_channel)) exit(1);
    }

    if(pthread_mutex_unlock(&downlink_mutex_channel)) exit(1);

    //  pthread_mutex_unlock(&exclusive);
  }


}

void do_UL_sig_channel_T(void *param)
{

  ch_thread* cthread = (ch_thread*)param;
  int count=0;
  u8 eNB_i,UE_i;
  double **r_re0,**r_re,**r_im,**r_im0,**s_re,**s_im;
  channel_desc_t *UE2eNB[8][3];
  node_desc_t *enb_data2[3];
  node_desc_t *ue_data[8];
  u16 next_slot,abstraction_flag;
  LTE_DL_FRAME_PARMS *frame_parms;
  int thread_id;

  eNB_i=(cthread->eNB_id);
  UE_i=(cthread->UE_id);
  r_re0=cthread->r_re0;
  r_re=cthread->r_re;
  r_im=cthread->r_im;
  r_im0=cthread->r_im0;
  s_im=cthread->s_im;
  s_re=cthread->s_re;
  UE2eNB[UE_i][eNB_i]=cthread->UE2eNB;
  enb_data2[eNB_i]=cthread->enb_data;
  ue_data[UE_i]=cthread->ue_data;
  next_slot=*(cthread->next_slot);
  abstraction_flag=*(cthread->abstraction_flag);
  frame_parms=cthread->frame_parms;
  thread_id=(cthread->thread_id);

  s32 **txdata,**rxdata;

  u8 UE_id=0,eNB_id=0,aa;
  double tx_pwr, rx_pwr;
  s32 rx_pwr2;
  u32 i;
  u32 slot_offset;
  double nf = 0; //currently unused


  while(1) {

    slot_offset = (nslot)*(frame_parms->samples_per_tti>>1);

    if(pthread_cond_wait(&uplink_cond[UE_i][eNB_i], &uplink_mutex[UE_i][eNB_i])) exit(1);//[UE_i][eNB_i]);

    //  pthread_mutex_lock(&exclusive);

    multipath_channel(UE2eNB[UE_i][eNB_i],s_re,s_im,r_re0,r_im0,
                      frame_parms->samples_per_tti>>1,0);

    printf("[SIM][UL] Channel UE %d => eNB %d : %f dB\n",UE_i,eNB_i,10*log10(rx_pwr));

    rx_pwr = signal_energy_fp(r_re0,r_im0,frame_parms->nb_antennas_rx,frame_parms->samples_per_tti>>1,0);

    printf("[SIM][UL] eNB %d : eNB out %f dB for slot %d (subframe %d), sptti %d\n",eNB_i,10*log10(rx_pwr),nslot,nslot>>1,frame_parms->samples_per_tti);

    if (UE2eNB[UE_i][eNB_i]->first_run == 1)
      UE2eNB[UE_i][eNB_i]->first_run = 0;


    // RF model
    rf_rx(r_re0,
          r_im0,
          NULL,
          NULL,
          0,
          frame_parms->nb_antennas_rx,
          frame_parms->samples_per_tti>>1,
          (UE_id==0) ? (1.0/7.68e6 * 1e9) : 1e9,  // sampling time (ns) + set noise bandwidth to 0 for UE>0 (i.e. no noise except for first UE)
          0.0,               // freq offset (Hz) (-20kHz..20kHz)
          0.0,               // drift (Hz) NOT YET IMPLEMENTED
          nf,                // noise_figure NOT YET IMPLEMENTED
          150.00 - 66.227,   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)
          200,               // IP3_dBm (dBm)
          &UE2eNB[UE_i][eNB_i]->ip,               // initial phase
          30.0e3,            // pn_cutoff (kHz)
          -500.0,            // pn_amp (dBc) default: 50
          0.0,               // IQ imbalance (dB),
          0.0);              // IQ phase imbalance (rad)


    rx_pwr = signal_energy_fp(r_re,r_im,frame_parms->nb_antennas_rx,frame_parms->samples_per_tti>>1,0);

    printf("[SIM][UL] rx_pwr (ADC in) %f dB for slot %d (subframe %d)\n",10*log10(rx_pwr),nslot,nslot>>1);

    //  count = __sync_add_and_fetch(&_COT_U, 1);
    //   COMPILER_BARRIER();

    if (pthread_mutex_lock(&uplink_mutex_channel)) exit(1);

    _COT_U++;

    if(_COT_U==NUM_THREAD_UPLINK) {
      //if(count==NUM_THREAD_UPLINK){
      if (pthread_cond_signal(&uplink_cond_channel)) exit(1);
    }

    if(pthread_mutex_unlock(&uplink_mutex_channel)) exit(1);

    //    pthread_mutex_unlock(&exclusive);

  }

}

void Clean_Param(double **r_re,double **r_im,LTE_DL_FRAME_PARMS *frame_parms)
{
  u32 i,aa;

  for (i=0; i<(frame_parms->samples_per_tti>>1); i++) {
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
      r_re[aa][i]=0.0;
      r_im[aa][i]=0.0;
    }
  }
}

void Channel_Out(lte_subframe_t direction,int eNB_id,int UE_id,double **r_re,double **r_im,double **r_re0,double **r_im0,LTE_DL_FRAME_PARMS *frame_parms)
{
  u32 i,aa;
  double alpha=0.0;
  u32 slot_offset;
  frame_parms = &PHY_vars_UE_g[0]->lte_frame_parms;
  slot_offset = (nslot)*(frame_parms->samples_per_tti>>1);
  s32 rx_pwr2;
  s32 **rxdata;

  if(eNB_id==(UE_id % NB_eNB_INST))
    alpha=1;

  for (i=0; i<(frame_parms->samples_per_tti>>1); i++) {
    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
      r_re[aa][i]+=(r_re0[aa][i]*alpha);
      r_im[aa][i]+=(r_im0[aa][i]*alpha);

    }
  }

  rxdata=rx[eNB_id][0];

  if (direction  == SF_DL ) {
    frame_parms = &PHY_vars_eNB_g[0]->lte_frame_parms;
    slot_offset = (nslot)*(frame_parms->samples_per_tti>>1);
    adc(r_re,
        r_im,
        0,
        slot_offset,
        rx[NB_eNB_INST+UE_id][0],
        frame_parms->nb_antennas_rx,
        frame_parms->samples_per_tti>>1,
        12);
  } else {
    frame_parms = &PHY_vars_UE_g[0]->lte_frame_parms;
    slot_offset = (nslot)*(frame_parms->samples_per_tti>>1);
    adc(r_re,
        r_im,
        0,
        slot_offset,
        rx[eNB_id][0],
        frame_parms->nb_antennas_rx,
        frame_parms->samples_per_tti>>1,
        12);
  }


  rx_pwr2 = signal_energy(rxdata[0]+slot_offset,frame_parms->samples_per_tti>>1);
  //  printf("[SIM][DL/UL] UE/eNB %d : rx_pwr (ADC out) %f dB (%d) for slot %d (subframe %d), writing to %p\n",UE_id, 10*log10((double)rx_pwr2),rx_pwr2,nslot,nslot>>1,rxdata);
}

/*
void do_UL_sig_channel(u8 eNB_i,u8 UE_i,double **r_re0,double **r_im0,double **r_re,double **r_im,double **s_re,double **s_im,channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX],u16 next_slot,u8 abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms) {

  s32 **txdata,**rxdata;

  u8 UE_id=0,eNB_id=0,aa;
  double tx_pwr, rx_pwr;
  s32 rx_pwr2;
  u32 i;
  u32 slot_offset;
  double nf = 0; //currently unused

  slot_offset = (next_slot)*(frame_parms->samples_per_tti>>1);

  multipath_channel(UE2eNB[UE_i][eNB_i],s_re,s_im,r_re0,r_im0,
        frame_parms->samples_per_tti>>1,0);


  rx_pwr = signal_energy_fp(r_re0,r_im0,frame_parms->nb_antennas_rx,frame_parms->samples_per_tti>>1,0);

  if (UE2eNB[UE_i][eNB_i]->first_run == 1)
    UE2eNB[UE_i][eNB_i]->first_run = 0;


  // RF model
  rf_rx(r_re0,
        r_im0,
        NULL,
        NULL,
        0,
        frame_parms->nb_antennas_rx,
        frame_parms->samples_per_tti>>1,
        (UE_id==0) ? (1.0/7.68e6 * 1e9) : 1e9,  // sampling time (ns) + set noise bandwidth to 0 for UE>0 (i.e. no noise except for first UE)
        0.0,               // freq offset (Hz) (-20kHz..20kHz)
        0.0,               // drift (Hz) NOT YET IMPLEMENTED
        nf,                // noise_figure NOT YET IMPLEMENTED
        150.00 - 66.227,   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)
        200,               // IP3_dBm (dBm)
        &UE2eNB[UE_i][eNB_i]->ip,               // initial phase
        30.0e3,            // pn_cutoff (kHz)
        -500.0,            // pn_amp (dBc) default: 50
        0.0,               // IQ imbalance (dB),
        0.0);              // IQ phase imbalance (rad)


      rx_pwr = signal_energy_fp(r_re,r_im,frame_parms->nb_antennas_rx,frame_parms->samples_per_tti>>1,0);
        printf("[SIM][UL] rx_pwr (ADC in) %f dB for slot %d (subframe %d)\n",10*log10(rx_pwr),next_slot,next_slot>>1);

}
*/

/*
void do_DL_sig_channel(u8 eNB_i,u8 UE_i,double **r_re0,double **r_im0,double **r_re,double **r_im,double **s_re,double **s_im,channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX],
node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX], u16 next_slot,u8 abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms) {
  s32 att_eNB_id=-1;
  s32 **txdata,**rxdata;

  u8 eNB_id=0,UE_id=0;
  double tx_pwr, rx_pwr;
  s32 rx_pwr2;
  u32 i,aa;
  u32 slot_offset;


  multipath_channel(eNB2UE[eNB_i][UE_i],s_re,s_im,r_re0,r_im0,
        frame_parms->samples_per_tti>>1,0);

  rx_pwr = signal_energy_fp2(eNB2UE[eNB_i][UE_i]->ch[0],eNB2UE[eNB_i][UE_i]->channel_length)*eNB2UE[eNB_i][UE_i]->channel_length;


  printf("[SIM][DL] Channel eNB %d => UE %d : tx_power %f dBm, path_loss %f dB\n",
         eNB_i,UE_i,
         enb_data[eNB_id]->tx_power_dBm,
         eNB2UE[eNB_id][UE_id]->path_loss_dB);
  printf("[SIM][DL] Channel eNB %d => UE %d : Channel gain %f dB (%f)\n",eNB_id,UE_id,10*log10(rx_pwr),rx_pwr);

  rx_pwr = signal_energy_fp(r_re0,r_im0,frame_parms->nb_antennas_rx,frame_parms->samples_per_tti>>1,0);
  printf("[SIM][DL] UE %d : rx_pwr %f dB for slot %d (subframe %d)\n",UE_id,10*log10(rx_pwr),next_slot,next_slot>>1);

  if (eNB2UE[eNB_id][UE_id]->first_run == 1)
    eNB2UE[eNB_id][UE_id]->first_run = 0;

  rf_rx(r_re0,
        r_im0,
        NULL,
        NULL,
        0,
        frame_parms->nb_antennas_rx,
        frame_parms->samples_per_tti>>1,
        1e3/eNB2UE[eNB_i][UE_i]->BW,  // sampling time (ns)
        0.0,               // freq offset (Hz) (-20kHz..20kHz)
        0.0,               // drift (Hz) NOT YET IMPLEMENTED
        ue_data[UE_id]->rx_noise_level,                // noise_figure NOT YET IMPLEMENTED
        110.00 - 66.227,   // rx_gain (dB) (66.227 = 20*log10(pow2(11)) = gain from the adc that will be applied later)
        200,               // IP3_dBm (dBm)
        &eNB2UE[eNB_i][UE_i]->ip,               // initial phase
        30.0e3,            // pn_cutoff (kHz)
        -500.0,            // pn_amp (dBc) default: 50
        0.0,               // IQ imbalance (dB),
        0.0);              // IQ phase imbalance (rad)

  rx_pwr = signal_energy_fp(r_re0,r_im0,frame_parms->nb_antennas_rx,frame_parms->samples_per_tti>>1,0);
  printf("[SIM][DL] UE %d : ADC in (eNB %d) %f dB for slot %d (subframe %d)\n",UE_id,eNB_id,10*log10(rx_pwr),next_slot,next_slot>>1);

}

*/


/*
void init_mmap_channel(int id,LTE_DL_FRAME_PARMS *frame_parms, double ***s_re,double ***s_im,double ***r_re,double ***r_im,double ***r_re0,double ***r_im0) {

    int i;
    int fd[20];
    int result;
    double *map;
    unsigned int FILESIZE;

       FILESIZE=FRAME_LENGTH_COMPLEX_SAMPLES*(sizeof(double));

  (*s_re) = malloc(2*sizeof(double*));
  (*s_im)  = malloc(2*sizeof(double*));
  (*r_re)  = malloc(2*sizeof(double*));
  (*r_im)  = malloc(2*sizeof(double*));
  (*r_re0)  = malloc(2*sizeof(double*));
  (*r_im0)  = malloc(2*sizeof(double*));

  char buffer[100];
  int j=0;
  for (i=0;i<2;i++) {

    sprintf(buffer,"/tmp/s_re_%d_%d.bin",id,i);
     fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
     result = lseek(fd[j], FILESIZE-1, SEEK_SET);
     result = write(fd[j], "", 1);
     (*s_re)[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);

     j++;

    sprintf(buffer,"/tmp/s_im_%d_%d.bin",id,i);
       fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
       result = lseek(fd[j], FILESIZE-1, SEEK_SET);
       result = write(fd[j], "", 1);
       (*s_im)[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);

       j++;

       sprintf(buffer,"/tmp/r_re_%d_%d.bin",id,i);
       fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
      result = lseek(fd[j], FILESIZE-1, SEEK_SET);
      result = write(fd[j], "", 1);
      (*r_re)[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);

        j++;


       sprintf(buffer,"/tmp/r_im_%d_%d.bin",id,i);
       fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
       result = lseek(fd[j], FILESIZE-1, SEEK_SET);
       result = write(fd[j], "", 1);
       (*r_im)[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);

       j++;
  }

  for (i=0;i<2;i++) {
    (*r_re0)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*r_re0)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    (*r_im0)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*r_im0)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
  }

}

void init_mmap(int id,LTE_DL_FRAME_PARMS *frame_parms, double ***s_re,double ***s_im,double ***r_re,double ***r_im,double ***r_re0,double ***r_im0) {

    int i;
    int fd[20];
    int result;
    double *map;
    unsigned int FILESIZE;

       FILESIZE=FRAME_LENGTH_COMPLEX_SAMPLES*(sizeof(double));

  *s_re = malloc(2*sizeof(double*));
  *s_im = malloc(2*sizeof(double*));
  *r_re = malloc(2*sizeof(double*));
  *r_im = malloc(2*sizeof(double*));
  *r_re0 = malloc(2*sizeof(double*));
  *r_im0 = malloc(2*sizeof(double*));
  char buffer[100];
  int j=0;
  for (i=0;i<2;i++) {

    sprintf(buffer,"/tmp/s_re_%d_%d.bin",id,i);
     fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
     result = lseek(fd[j], FILESIZE-1, SEEK_SET);
     result = write(fd[j], "", 1);
     (*s_re)[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);

     j++;

    sprintf(buffer,"/tmp/s_im_%d_%d.bin",id,i);
       fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
       result = lseek(fd[j], FILESIZE-1, SEEK_SET);
       result = write(fd[j], "", 1);
       (*s_im)[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);

       j++;

       sprintf(buffer,"/tmp/r_re_%d_%d.bin",id,i);
       fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
      result = lseek(fd[j], FILESIZE-1, SEEK_SET);
      result = write(fd[j], "", 1);
      (*r_re)[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);

        j++;

       sprintf(buffer,"/tmp/r_im_%d_%d.bin",id,i);
       fd[j] = open(buffer, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
       result = lseek(fd[j], FILESIZE-1, SEEK_SET);
       result = write(fd[j], "", 1);
       (*r_im)[i] = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd[j], 0);

       j++;
  }

  for (i=0;i<2;i++) {

    (*r_re0)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*r_re0)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    (*r_im0)[i] = malloc(FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
    bzero((*r_im0)[i],FRAME_LENGTH_COMPLEX_SAMPLES*sizeof(double));
  }


}
*/

