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
#include "PHY_INTERFACE/extern.h"
#endif

#include "../USER/oaisim.h"
#include "channel_sim_proc.h"
#include "interface.h"
#include "Tsync.h"
#include "Process.h"

#define FILENAMEMAX 255

//#define DEBUG_SIM
void Process_Func(int node_id,int port,double **r_re0,double **r_im0,double **r_re,double **r_im,double **s_re,double **s_im,
                  node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],u8 abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms)
{

  if(node_id<MAX_eNB)
    eNB_Inst(node_id,port,r_re0,r_im0,r_re,r_im,s_re,s_im,enb_data,abstraction_flag,frame_parms);
  else
    UE_Inst(node_id,port,r_re0,r_im0,r_re,r_im,s_re,s_im,ue_data,abstraction_flag,frame_parms);


}
void UE_Inst(int node_id,int port,double **r_re0,double **r_im0,double **r_re,double **r_im,double **s_re,double **s_im,
             node_desc_t *ue_data[NUMBER_OF_UE_MAX],u8 abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms)
{


  int next_slot,last_slot,slot=0,UE_id=0,eNB_id=0;
  lte_subframe_t direction;
  printf("UE [ %d ] Starts \n",node_id-MAX_eNB);
  //   init_mmap(node_id,frame_parms, &s_re, &s_im, &r_re, &r_im, &r_re0, &r_im0);
  init_channel_vars (frame_parms, &s_re, &s_im, &r_re, &r_im, &r_re0, &r_im0);
  mmap_ue(node_id,&PHY_vars_UE_g[0]->lte_ue_common_vars.txdata,&PHY_vars_UE_g[0]->lte_ue_common_vars.rxdata,frame_parms);

  // Interface_init(port,node_id);
  char p_input[FILENAMEMAX];
  int fd_pipe;
  int fd_channel;
  mkfifo("/tmp/channel", 0666);
  fd_channel=open("/tmp/channel",O_RDWR,0);
  sprintf(p_input,"/tmp/pipe_%d",node_id);
  mkfifo(p_input, 0666);
  fd_pipe=open(p_input,O_RDWR,0);
  // IntInitAll();
  mac_xface->frame=0;

  while(1) {
    // wait_4slot(&slot,&mac_xface->frame);
    read(fd_pipe,&slot,sizeof(slot));

    last_slot = (slot - 1)%20;

    if (last_slot <0)
      last_slot+=20;

    next_slot = (slot + 1)%20;

    direction = subframe_select(frame_parms,next_slot>>1);
#ifdef DEBUG_SIM
    printf("\n\n[SIM] EMU PHY procedures UE %d for frame %d, slot %d (subframe %d)\n",
           UE_id,mac_xface->frame, slot, (next_slot >> 1));
#endif

    if (PHY_vars_UE_g[UE_id]->UE_mode[0] != NOT_SYNCHED) {
      if ((mac_xface->frame)>0) {
        phy_procedures_UE_lte ((last_slot),(next_slot), PHY_vars_UE_g[UE_id], 0, abstraction_flag);
      }
    } else {
      if (((mac_xface->frame)>0) && ((last_slot) == (SLOTS_PER_FRAME-1))) {
        initial_sync(PHY_vars_UE_g[UE_id]);
      }
    }


    if( direction == SF_UL || (direction == SF_S && next_slot%2!=0)) {
      do_OFDM_mod(PHY_vars_UE_g[UE_id]->lte_ue_common_vars.txdataF,PHY_vars_UE_g[UE_id]->lte_ue_common_vars.txdata,next_slot,&PHY_vars_UE_g[UE_id]->lte_frame_parms);
    }

    write(fd_channel,&node_id,sizeof(node_id));

    //  send_exec_complete(CHANNEL_PORT);
    if(slot==19)
      (mac_xface->frame)++;

  }

}

void eNB_Inst(int node_id,int port,double **r_re0,double **r_im0,double **r_re,double **r_im,double **s_re,double **s_im,
              node_desc_t *enb_data[NUMBER_OF_eNB_MAX],u8 abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms)
{
  int next_slot,last_slot,slot=0,UE_id=0,eNB_id=0;
  lte_subframe_t direction;
  printf("eNB [ %d ] Starts \n",node_id);

  // init_mmap(node_id,frame_parms, &s_re, &s_im, &r_re, &r_im, &r_re0, &r_im0);
  init_channel_vars (frame_parms, &s_re, &s_im, &r_re, &r_im, &r_re0, &r_im0);
  mmap_enb(node_id,PHY_vars_eNB_g[0]->lte_eNB_common_vars.txdata,PHY_vars_eNB_g[0]->lte_eNB_common_vars.rxdata,frame_parms);

  // Interface_init(port,node_id);
  // IntInitAll();
  char p_input[FILENAMEMAX];
  int fd_pipe;
  int fd_channel;

  mkfifo("/tmp/channel", 0666);
  fd_channel=open("/tmp/channel",O_RDWR,0);
  sprintf(p_input,"/tmp/pipe_%d",node_id);
  mkfifo(p_input, 0666);
  fd_pipe=open(p_input, O_RDWR,0);
  mac_xface->frame=0;

  while(1) {
    // wait_4slot(&slot,&mac_xface->frame);
    read(fd_pipe,&slot,sizeof(slot));

    last_slot = (slot - 1)%20;

    if (last_slot <0)
      last_slot+=20;

    next_slot = (slot + 1)%20;
    direction = subframe_select(frame_parms,next_slot>>1);
#ifdef DEBUG_SIM
    printf
    ("\n\n [SIM]EMU PHY procedures eNB %d for frame %d, slot %d (subframe %d) (rxdataF_ext %p) Nid_cell %d\n",
     eNB_id, mac_xface->frame, slot, next_slot >> 1,
     PHY_vars_eNB_g[0]->lte_eNB_ulsch_vars[0]->rxdataF_ext, PHY_vars_eNB_g[eNB_id]->lte_frame_parms.Nid_cell);
#endif
    phy_procedures_eNB_lte (last_slot, next_slot, PHY_vars_eNB_g[eNB_id], abstraction_flag);

    if( direction == SF_DL || (direction == SF_S && next_slot%2==0)) {
      do_OFDM_mod(PHY_vars_eNB_g[eNB_id]->lte_eNB_common_vars.txdataF[0],
                  PHY_vars_eNB_g[eNB_id]->lte_eNB_common_vars.txdata[0],
                  next_slot,
                  &PHY_vars_eNB_g[eNB_id]->lte_frame_parms);
    }

    write(fd_channel,&node_id,sizeof(node_id));

    //  send_exec_complete(CHANNEL_PORT);
    if(slot==19)
      (mac_xface->frame)++;
  }

}

void Channel_Inst(int node_id,int port,double **s_re[MAX_eNB+MAX_UE],double **s_im[MAX_eNB+MAX_UE],double **r_re[MAX_eNB+MAX_UE],double **r_im[MAX_eNB+MAX_UE],double **r_re0,double **r_im0,
                  double **r_re0_d[MAX_UE][MAX_eNB],double **r_im0_d[MAX_UE][MAX_eNB],double **r_re0_u[MAX_eNB][MAX_UE],double **r_im0_u[MAX_eNB][MAX_UE],channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX],
                  channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX],node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],u8 abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms)
{

  // Interface_init(port,node_id);
  // IntInitAll();
  mkfifo("/tmp/channel", 0666);
  fd_channel=open("/tmp/channel", O_RDWR ,0);

  int next_slot,last_slot,slot=0,UE_id=0,eNB_id=0;
  lte_subframe_t direction;
  char in_buffer[100];

  int ci,ji=0;

  for(ci=0; ci<NB_eNB_INST; ci++) {
    mmap_enb(ji,tx[ci],rx[ci],frame_parms);
    init_channel_vars (frame_parms, &(s_re[ci]), &(s_im[ci]), &(r_re[ci]), &(r_im[ci]), &(r_re0), &(r_im0));
    // init_mmap_channel(ji,frame_parms, &(s_re[ci]), &(s_im[ci]), &(r_re[ci]), &(r_im[ci]), &(r_re0), &(r_im0));
    sprintf(in_buffer, "/tmp/pipe_%d",ji);
    mkfifo(in_buffer, 0666);
    fd_NB[ci]=open(in_buffer, O_RDWR ,0);
    ji++;
  }

  ji=0;

  for(ci=NB_eNB_INST; ci<(NB_eNB_INST+NB_UE_INST); ci++) {
    mmap_enb(MAX_eNB+ji,tx[ci],rx[ci],frame_parms);
    init_channel_vars (frame_parms, &(s_re[ci]), &(s_im[ci]), &(r_re[ci]), &(r_im[ci]), &(r_re0), &(r_im0));
    //  init_mmap_channel(MAX_eNB+ji,frame_parms, &(s_re[ci]), &(s_im[ci]), &(r_re[ci]), &(r_im[ci]), &(r_re0), &(r_im0));
    sprintf(in_buffer, "/tmp/pipe_%d",MAX_eNB+ji);
    mkfifo(in_buffer, 0666);

    fd_NB[ci]=open(in_buffer, O_RDWR ,0);
    ji++;
  }

  for (eNB_id = 0; eNB_id < NB_eNB_INST; eNB_id++) {
    for (UE_id = 0; UE_id < NB_UE_INST; UE_id++) {
      init_rre(frame_parms,&(r_re0_u[eNB_id][UE_id]),&(r_im0_u[eNB_id][UE_id]));
      init_rre(frame_parms,&(r_re0_d[UE_id][eNB_id]),&(r_im0_d[UE_id][eNB_id]));
    }
  }

  for(eNB_id=0; eNB_id<NB_eNB_INST; eNB_id++) {
    for(UE_id=0; UE_id<NB_UE_INST; UE_id++) {
      e2u_t[eNB_id][UE_id]=(ch_thread*)calloc(1,sizeof(ch_thread));
    }
  }

  for(UE_id=0; UE_id<NB_UE_INST; UE_id++) {
    for(eNB_id=0; eNB_id<NB_eNB_INST; eNB_id++) {
      u2e_t[UE_id][eNB_id]=(ch_thread*)calloc(1,sizeof(ch_thread));
    }
  }

  pthread_t cthr_u[NB_eNB_INST][NB_UE_INST];
  pthread_t cthr_d[NB_UE_INST][NB_eNB_INST];


  pthread_mutex_init(&downlink_mutex_channel,NULL);
  pthread_mutex_init(&uplink_mutex_channel,NULL);
  pthread_mutex_init(&exclusive,NULL);

  if (pthread_cond_init (&downlink_cond_channel, NULL)) exit(1);

  if (pthread_cond_init (&uplink_cond_channel, NULL)) exit(1);

  if (pthread_mutex_lock(&downlink_mutex_channel)) exit(1);

  if (pthread_mutex_lock(&uplink_mutex_channel)) exit(1);

  NUM_THREAD_DOWNLINK=0;

  for (UE_id = 0; UE_id < (NB_UE_INST); UE_id++) {
    for (eNB_id=0; eNB_id<(NB_eNB_INST); eNB_id++) {
      u2e_t[UE_id][eNB_id]->thread_id=NUM_THREAD_DOWNLINK;
      u2e_t[UE_id][eNB_id]->eNB_id=eNB_id;
      u2e_t[UE_id][eNB_id]->UE_id=UE_id;
      u2e_t[UE_id][eNB_id]->r_re0=r_re0_d[UE_id][eNB_id];
      u2e_t[UE_id][eNB_id]->r_im0=r_im0_d[UE_id][eNB_id];
      u2e_t[UE_id][eNB_id]->r_re=r_re[NB_eNB_INST+UE_id];
      u2e_t[UE_id][eNB_id]->r_im=r_im[NB_eNB_INST+UE_id];
      u2e_t[UE_id][eNB_id]->s_im=s_im[eNB_id];
      u2e_t[UE_id][eNB_id]->s_re=s_re[eNB_id];
      u2e_t[UE_id][eNB_id]->eNB2UE=eNB2UE[eNB_id][UE_id];
      u2e_t[UE_id][eNB_id]->UE2eNB=UE2eNB[UE_id][eNB_id];
      u2e_t[UE_id][eNB_id]->enb_data=enb_data[eNB_id];
      u2e_t[UE_id][eNB_id]->ue_data=ue_data[UE_id];
      u2e_t[UE_id][eNB_id]->next_slot=&next_slot;
      u2e_t[UE_id][eNB_id]->abstraction_flag=&abstraction_flag;
      u2e_t[UE_id][eNB_id]->frame_parms=frame_parms;

      for (int i=0;i<3;i++)
      {
          u2e_t[UE_id][eNB_id]->tx_data[i]=tx[eNB_id][i];
          u2e_t[UE_id][eNB_id]->rx_data[i]=rx[NB_eNB_INST+UE_id][i];
      }

      if(pthread_cond_init (&downlink_cond[eNB_id][UE_id], NULL)) exit(1);

      if(pthread_mutex_lock(&downlink_mutex[eNB_id][UE_id])) exit(1);

      pthread_create (&cthr_d[UE_id][eNB_id], NULL, do_DL_sig_channel_T,(void*)(u2e_t[UE_id][eNB_id]));
      NUM_THREAD_DOWNLINK++;
    }
  }

  NUM_THREAD_UPLINK=0;

  for (eNB_id=0; eNB_id<(NB_eNB_INST); eNB_id++) {
    for (UE_id = 0; UE_id < (NB_UE_INST); UE_id++) {
      e2u_t[eNB_id][UE_id]->thread_id=NUM_THREAD_UPLINK;
      e2u_t[eNB_id][UE_id]->eNB_id=eNB_id;
      e2u_t[eNB_id][UE_id]->UE_id=UE_id;
      e2u_t[eNB_id][UE_id]->r_re=r_re[eNB_id];
      e2u_t[eNB_id][UE_id]->r_im=r_im[eNB_id];
      e2u_t[eNB_id][UE_id]->r_re0=r_re0_u[eNB_id][UE_id];
      e2u_t[eNB_id][UE_id]->r_im0=r_im0_u[eNB_id][UE_id];
      e2u_t[eNB_id][UE_id]->s_im=s_im[NB_eNB_INST+UE_id];
      e2u_t[eNB_id][UE_id]->s_re=s_re[NB_eNB_INST+UE_id];
      e2u_t[eNB_id][UE_id]->eNB2UE=eNB2UE[eNB_id][UE_id];
      e2u_t[eNB_id][UE_id]->UE2eNB=UE2eNB[UE_id][eNB_id];
      e2u_t[eNB_id][UE_id]->enb_data=enb_data[eNB_id];
      e2u_t[eNB_id][UE_id]->ue_data=ue_data[UE_id];
      e2u_t[eNB_id][UE_id]->next_slot=&next_slot;
      e2u_t[eNB_id][UE_id]->abstraction_flag=&abstraction_flag;
      e2u_t[eNB_id][UE_id]->frame_parms=frame_parms;
      for (int i=0;i<3;i++)
      {
          e2u_t[eNB_id][UE_id]->tx_data[i]=tx[NB_eNB_INST+UE_id][i];
          e2u_t[eNB_id][UE_id]->rx_data[i]=rx[eNB_id][i];
      }

      if(pthread_cond_init (&uplink_cond[UE_id][eNB_id], NULL)) exit(1);

      if(pthread_mutex_lock(&uplink_mutex[UE_id][eNB_id])) exit(1);

      pthread_create (&cthr_u[eNB_id][UE_id], NULL, do_UL_sig_channel_T,(void*)e2u_t[eNB_id][UE_id]);
      NUM_THREAD_UPLINK++;
    }
  }
}

void Channel_DL(double **s_re[MAX_eNB+MAX_UE],double **s_im[MAX_eNB+MAX_UE],double **r_re[MAX_eNB+MAX_UE],double **r_im[MAX_eNB+MAX_UE],double **r_re0,double **r_im0,
                double **r_re0_d[MAX_UE][MAX_eNB],double **r_im0_d[MAX_UE][MAX_eNB],double **r_re0_u[MAX_eNB][MAX_UE],double **r_im0_u[MAX_eNB][MAX_UE],channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX],
                channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX],node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],u8 abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms,
                int slot)
{

  int count=0;
  int next_slot,last_slot,UE_id=0,eNB_id=0;
  lte_subframe_t direction;
  s32 **txdata,**rxdata;
  double tx_pwr, rx_pwr;
  s32 rx_pwr2;
  u32 i;
  u32 slot_offset;

  for (eNB_id=0; eNB_id<(NB_eNB_INST); eNB_id++) {
    write(fd_NB[eNB_id],&slot,sizeof(slot));
    count++;
    //send_exec_msg(mac_xface->frame,slot,eNB_PORT+eNB_id);
  }


  while(count--) {
    int dummy=1;
    read(fd_channel,&dummy,sizeof(dummy));
    //wait_4Msg();
  }




  for (eNB_id=0; eNB_id<(NB_eNB_INST); eNB_id++) {
    frame_parms = &PHY_vars_eNB_g[eNB_id]->lte_frame_parms;
    slot_offset = (nslot)*(frame_parms->samples_per_tti>>1);
    txdata =tx[eNB_id][0];
    tx_pwr = dac_fixed_gain(s_re[eNB_id],
                            s_im[eNB_id],
                            txdata,
                            slot_offset,
                            frame_parms->nb_antennas_tx,
                            frame_parms->samples_per_tti>>1,
                            14,
                            40);
    printf("[SIM][DL] eNB %d: tx_pwr %f dB for slot %d (subframe %d)\n",eNB_id,10*log10(tx_pwr),nslot,nslot>>1);
  }

  _COT=0;

  for (UE_id = 0; UE_id < (NB_UE_INST); UE_id++) {
    for (eNB_id=0; eNB_id<(NB_eNB_INST); eNB_id++) {
      if(pthread_cond_signal(&downlink_cond[eNB_id][UE_id])) exit(1);

      if(pthread_mutex_unlock(&downlink_mutex[eNB_id][UE_id])) exit(1);
    }
  }

  if(pthread_cond_wait(&downlink_cond_channel, &downlink_mutex_channel)) exit(1);

  for (UE_id = 0; UE_id < (NB_UE_INST); UE_id++) {
    Clean_Param(r_re[NB_eNB_INST+UE_id],r_im[NB_eNB_INST+UE_id],frame_parms);

    for (eNB_id=0; eNB_id<(NB_eNB_INST); eNB_id++) {
      Channel_Out(SF_DL,eNB_id,UE_id,r_re[NB_eNB_INST+UE_id],r_im[NB_eNB_INST+UE_id],r_re0_d[UE_id][eNB_id],r_im0_d[UE_id][eNB_id],frame_parms);
      //  adc_channel(eNB_id,UE_id,r_re[NB_eNB_INST+UE_id],r_im[NB_eNB_INST+UE_id]);

    }
  }

  count=0;

  for (UE_id = 0; UE_id < (NB_UE_INST); UE_id++)
    if (mac_xface->frame >= (UE_id * 10)) {
      write(fd_NB[NB_eNB_INST+UE_id],&slot,sizeof(slot));
      //send_exec_msg(mac_xface->frame,slot,UE_PORT+UE_id);
      count++;
    }

  while(count--) {
    int dummy=1;
    read(fd_channel,&dummy,sizeof(dummy));
    //wait_4Msg();
  }

}


void Channel_UL(double **s_re[MAX_eNB+MAX_UE],double **s_im[MAX_eNB+MAX_UE],double **r_re[MAX_eNB+MAX_UE],double **r_im[MAX_eNB+MAX_UE],double **r_re0,double **r_im0,
                double **r_re0_d[MAX_UE][MAX_eNB],double **r_im0_d[MAX_UE][MAX_eNB],double **r_re0_u[MAX_eNB][MAX_UE],double **r_im0_u[MAX_eNB][MAX_UE],channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX],
                channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX],node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],u8 abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms,
                int slot)
{
  int count=0;
  int next_slot,last_slot,UE_id=0,eNB_id=0;
  lte_subframe_t direction;
  s32 **txdata,**rxdata;
  double tx_pwr, rx_pwr;
  s32 rx_pwr2;
  u32 i;
  u32 slot_offset;

  for (UE_id = 0; UE_id < (NB_UE_INST); UE_id++)
    if (mac_xface->frame >= (UE_id * 10)) {
      write(fd_NB[NB_eNB_INST+UE_id],&slot,sizeof(slot));
      //send_exec_msg(mac_xface->frame,slot,UE_PORT+UE_id);
      count++;
    }

  while(count--) {
    int dummy=1;
    read(fd_channel,&dummy,sizeof(dummy));
    //wait_4Msg();
  }



  for (UE_id = 0; UE_id < (NB_UE_INST); UE_id++) {
    frame_parms = &PHY_vars_UE_g[UE_id]->lte_frame_parms;
    slot_offset = (nslot)*(frame_parms->samples_per_tti>>1);
    txdata =tx[NB_eNB_INST+UE_id][0]; //PHY_vars_UE_g[UE_id]->lte_ue_common_vars.txdata;
    tx_pwr = dac_fixed_gain(s_re[NB_eNB_INST+UE_id],
                            s_im[NB_eNB_INST+UE_id],
                            txdata,
                            slot_offset,
                            frame_parms->nb_antennas_tx,
                            frame_parms->samples_per_tti>>1,
                            14,
                            18);
    printf("[SIM][UL] UE %d tx_pwr %f dB for slot %d (subframe %d)\n",UE_id,10*log10(tx_pwr),nslot,nslot>>1);
    rx_pwr = signal_energy_fp(s_re[NB_eNB_INST+UE_id],s_im[NB_eNB_INST+UE_id],frame_parms->nb_antennas_rx,frame_parms->samples_per_tti>>1,0);
    printf("[SIM][UL] UE %d tx_pwr %f dB for slot %d (subframe %d)\n",UE_id,10*log10(rx_pwr),nslot,nslot>>1);
  }

  _COT_U=0;

  for (eNB_id=0; eNB_id<(NB_eNB_INST); eNB_id++) {
    for (UE_id = 0; UE_id < (NB_UE_INST); UE_id++) {
      if ( pthread_cond_signal(&uplink_cond[UE_id][eNB_id])) exit(1);

      if ( pthread_mutex_unlock(&uplink_mutex[UE_id][eNB_id])) exit(1);
    }
  }

  if ( pthread_cond_wait(&uplink_cond_channel, &uplink_mutex_channel)) exit(1);

  for (eNB_id=0; eNB_id<(NB_eNB_INST); eNB_id++) {
    Clean_Param(r_re[eNB_id],r_im[eNB_id],frame_parms);

    for (UE_id = 0; UE_id < (NB_UE_INST); UE_id++) {
      Channel_Out(SF_UL,eNB_id,UE_id,r_re[eNB_id],r_im[eNB_id],r_re0_u[eNB_id][UE_id],r_im0_u[eNB_id][UE_id],frame_parms);
      printf("[SIM][DL] eNB %d: tx_pwr %f dB for slot %d (subframe %d)\n",eNB_id,10*log10(tx_pwr),next_slot,next_slot>>1);
    }
  }


  count=0;

  for (eNB_id=0; eNB_id<(NB_eNB_INST); eNB_id++) {
    write(fd_NB[eNB_id],&slot,sizeof(slot));
    //  send_exec_msg(mac_xface->frame,slot,eNB_PORT+eNB_id);
    count++;
  }

  while(count--) {
    int dummy=1;
    read(fd_channel,&dummy,sizeof(dummy));
    //  wait_4Msg();
  }

}

void Channel_Func(double **s_re[MAX_eNB+MAX_UE],double **s_im[MAX_eNB+MAX_UE],double **r_re[MAX_eNB+MAX_UE],double **r_im[MAX_eNB+MAX_UE],double **r_re0,double **r_im0,
                  double **r_re0_d[MAX_UE][MAX_eNB],double **r_im0_d[MAX_UE][MAX_eNB],double **r_re0_u[MAX_eNB][MAX_UE],double **r_im0_u[MAX_eNB][MAX_UE],channel_desc_t *eNB2UE[NUMBER_OF_eNB_MAX][NUMBER_OF_UE_MAX],
                  channel_desc_t *UE2eNB[NUMBER_OF_UE_MAX][NUMBER_OF_eNB_MAX],node_desc_t *enb_data[NUMBER_OF_eNB_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],u8 abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms,
                  int slot)
{
  int last_slot;
  lte_subframe_t direction;

  last_slot = (slot - 1)%20;

  if (last_slot <0)
    last_slot+=20;

  nslot = (slot + 1)%20;

  direction = subframe_select(frame_parms,nslot>>1);

  if( direction == SF_DL || (direction == SF_S && nslot%2==0)) {
    Channel_DL(s_re,s_im,r_re,r_im,r_re0,r_im0,r_re0_d,r_im0_d,r_re0_u,r_im0_u,eNB2UE,UE2eNB,enb_data,ue_data,abstraction_flag,frame_parms,slot);
  } else {
    Channel_UL(s_re,s_im,r_re,r_im,r_re0,r_im0,r_re0_d,r_im0_d,r_re0_u,r_im0_u,eNB2UE,UE2eNB,enb_data,ue_data,abstraction_flag,frame_parms,slot);
  }
}
