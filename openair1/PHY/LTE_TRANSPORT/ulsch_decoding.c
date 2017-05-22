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

/*! \file PHY/LTE_TRANSPORT/ulsch_decoding.c
* \brief Top-level routines for decoding  the ULSCH transport channel from 36.212 V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/

//#include "defs.h"

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "PHY/CODING/extern.h"
#include "extern.h"
#include "SCHED/extern.h"
#ifdef OPENAIR2
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
#include "RRC/LITE/extern.h"
#include "PHY_INTERFACE/extern.h"
#endif

#ifdef PHY_ABSTRACTION
#include "UTIL/OCG/OCG.h"
#include "UTIL/OCG/OCG_extern.h"
#endif

#include "UTIL/LOG/vcd_signal_dumper.h"
//#define DEBUG_ULSCH_DECODING

void free_eNB_ulsch(LTE_eNB_ULSCH_t *ulsch)
{

  int i,r;

  if (ulsch) {
    for (i=0; i<8; i++) {
      if (ulsch->harq_processes[i]) {
        if (ulsch->harq_processes[i]->b) {
          free16(ulsch->harq_processes[i]->b,MAX_ULSCH_PAYLOAD_BYTES);
          ulsch->harq_processes[i]->b = NULL;
        }

        for (r=0; r<MAX_NUM_ULSCH_SEGMENTS; r++) {
          free16(ulsch->harq_processes[i]->c[r],((r==0)?8:0) + 768);
          ulsch->harq_processes[i]->c[r] = NULL;
        }

        for (r=0; r<MAX_NUM_ULSCH_SEGMENTS; r++)
          if (ulsch->harq_processes[i]->d[r]) {
            free16(ulsch->harq_processes[i]->d[r],((3*8*6144)+12+96)*sizeof(short));
            ulsch->harq_processes[i]->d[r] = NULL;
          }

        free16(ulsch->harq_processes[i],sizeof(LTE_UL_eNB_HARQ_t));
        ulsch->harq_processes[i] = NULL;
      }
    }

    free16(ulsch,sizeof(LTE_eNB_ULSCH_t));
    ulsch = NULL;
  }
}

LTE_eNB_ULSCH_t *new_eNB_ulsch(uint8_t max_turbo_iterations,uint8_t N_RB_UL, uint8_t abstraction_flag)
{

  LTE_eNB_ULSCH_t *ulsch;
  uint8_t exit_flag = 0,i,r;
  unsigned char bw_scaling =1;

  switch (N_RB_UL) {
  case 6:
    bw_scaling =16;
    break;

  case 25:
    bw_scaling =4;
    break;

  case 50:
    bw_scaling =2;
    break;

  default:
    bw_scaling =1;
    break;
  }

  ulsch = (LTE_eNB_ULSCH_t *)malloc16(sizeof(LTE_eNB_ULSCH_t));

  if (ulsch) {
    memset(ulsch,0,sizeof(LTE_eNB_ULSCH_t));
    ulsch->max_turbo_iterations = max_turbo_iterations;
    ulsch->Mlimit = 4;

    for (i=0; i<8; i++) {
      //      printf("new_ue_ulsch: Harq process %d\n",i);
      ulsch->harq_processes[i] = (LTE_UL_eNB_HARQ_t *)malloc16(sizeof(LTE_UL_eNB_HARQ_t));

      if (ulsch->harq_processes[i]) {
        memset(ulsch->harq_processes[i],0,sizeof(LTE_UL_eNB_HARQ_t));
        ulsch->harq_processes[i]->b = (uint8_t*)malloc16(MAX_ULSCH_PAYLOAD_BYTES/bw_scaling);

        if (ulsch->harq_processes[i]->b)
          memset(ulsch->harq_processes[i]->b,0,MAX_ULSCH_PAYLOAD_BYTES/bw_scaling);
        else
          exit_flag=3;

        if (abstraction_flag==0) {
          for (r=0; r<MAX_NUM_ULSCH_SEGMENTS/bw_scaling; r++) {
            ulsch->harq_processes[i]->c[r] = (uint8_t*)malloc16(((r==0)?8:0) + 3+768);
            if (ulsch->harq_processes[i]->c[r])
              memset(ulsch->harq_processes[i]->c[r],0,((r==0)?8:0) + 3+768);
            else
              exit_flag=2;

            ulsch->harq_processes[i]->d[r] = (short*)malloc16(((3*8*6144)+12+96)*sizeof(short));

            if (ulsch->harq_processes[i]->d[r])
              memset(ulsch->harq_processes[i]->d[r],0,((3*8*6144)+12+96)*sizeof(short));
            else
              exit_flag=2;
          }

          ulsch->harq_processes[i]->subframe_scheduling_flag = 0;
        }
      } else {
        exit_flag=1;
      }
    }

    if (exit_flag==0)
      return(ulsch);
  }

  LOG_E(PHY,"new_ue_ulsch: exit_flag = %d\n",exit_flag);
  free_eNB_ulsch(ulsch);

  return(NULL);
}

void clean_eNb_ulsch(LTE_eNB_ULSCH_t *ulsch)
{

  unsigned char i;

  //ulsch = (LTE_eNB_ULSCH_t *)malloc16(sizeof(LTE_eNB_ULSCH_t));
  if (ulsch) {
    ulsch->rnti = 0;

    for (i=0; i<8; i++) {
      if (ulsch->harq_processes[i]) {
        //    ulsch->harq_processes[i]->Ndi = 0;
        ulsch->harq_processes[i]->status = 0;
        ulsch->harq_processes[i]->subframe_scheduling_flag = 0;
        //ulsch->harq_processes[i]->phich_active = 0; //this will be done later after transmission of PHICH
        ulsch->harq_processes[i]->phich_ACK = 0;
        ulsch->harq_processes[i]->round = 0;
      }
    }

  }
}


uint8_t extract_cqi_crc(uint8_t *cqi,uint8_t CQI_LENGTH)
{

  uint8_t crc;

  crc = cqi[CQI_LENGTH>>3];
  //  printf("crc1: %x, shift %d\n",crc,CQI_LENGTH&0x7);
  crc = (crc<<(CQI_LENGTH&0x7));
  // clear crc bits
  //  ((char *)cqi)[CQI_LENGTH>>3] &= 0xff>>(8-(CQI_LENGTH&0x7));
  //  printf("crc2: %x, cqi0 %x\n",crc,cqi[1+(CQI_LENGTH>>3)]);
  crc |= (cqi[1+(CQI_LENGTH>>3)])>>(8-(CQI_LENGTH&0x7));
  // clear crc bits
  //(((char *)cqi)[1+(CQI_LENGTH>>3)]) = 0;

  //  printf("crc : %x\n",crc);
  return(crc);

}






int ulsch_decoding_data_2thread0(td_params* tdp) {

  PHY_VARS_eNB *eNB = tdp->eNB;
  int UE_id         = tdp->UE_id;
  int harq_pid      = tdp->harq_pid;
  int llr8_flag     = tdp->llr8_flag;

  unsigned int r,r_offset=0,Kr,Kr_bytes,iind;
  uint8_t crc_type;
  int offset = 0;
  int ret = 1;
  int16_t dummy_w[MAX_NUM_ULSCH_SEGMENTS][3*(6144+64)];
  LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[UE_id];
  LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];
  int Q_m = get_Qm_ul(ulsch_harq->mcs);
  int G = ulsch_harq->G;
  uint32_t E;
  uint32_t Gp,GpmodC,Nl=1;
  uint32_t C = ulsch_harq->C;

  uint8_t (*tc)(int16_t *y,
                uint8_t *,
                uint16_t,
                uint16_t,
                uint16_t,
                uint8_t,
                uint8_t,
                uint8_t,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *);

  if (llr8_flag == 0)
    tc = phy_threegpplte_turbo_decoder16;
  else
    tc = phy_threegpplte_turbo_decoder8;



  // go through first half of segments to get r_offset
  for (r=0; r<(ulsch_harq->C/2); r++) {

    // Get Turbo interleaver parameters
    if (r<ulsch_harq->Cminus)
      Kr = ulsch_harq->Kminus;
    else
      Kr = ulsch_harq->Kplus;

    Kr_bytes = Kr>>3;

    if (Kr_bytes<=64)
      iind = (Kr_bytes-5);
    else if (Kr_bytes <=128)
      iind = 59 + ((Kr_bytes-64)>>1);
    else if (Kr_bytes <= 256)
      iind = 91 + ((Kr_bytes-128)>>2);
    else if (Kr_bytes <= 768)
      iind = 123 + ((Kr_bytes-256)>>3);
    else {
      LOG_E(PHY,"ulsch_decoding: Illegal codeword size %d!!!\n",Kr_bytes);
      return(-1);
    }

    // This is stolen from rate-matching algorithm to get the value of E
    
    Gp = G/Nl/Q_m;
    GpmodC = Gp%C;
    
    if (r < (C-(GpmodC)))
      E = Nl*Q_m * (Gp/C);
    else
      E = Nl*Q_m * ((GpmodC==0?0:1) + (Gp/C));
    
    r_offset += E;

    if (r==0) {
      offset = Kr_bytes - (ulsch_harq->F>>3) - ((ulsch_harq->C>1)?3:0);
    } else {
      offset += (Kr_bytes- ((ulsch_harq->C>1)?3:0));
    }
  }

  // go through second half of segments
  for (; r<(ulsch_harq->C); r++) {


    //    printf("before subblock deinterleaving c[%d] = %p\n",r,ulsch_harq->c[r]);
    // Get Turbo interleaver parameters
    if (r<ulsch_harq->Cminus)
      Kr = ulsch_harq->Kminus;
    else
      Kr = ulsch_harq->Kplus;

    Kr_bytes = Kr>>3;

    if (Kr_bytes<=64)
      iind = (Kr_bytes-5);
    else if (Kr_bytes <=128)
      iind = 59 + ((Kr_bytes-64)>>1);
    else if (Kr_bytes <= 256)
      iind = 91 + ((Kr_bytes-128)>>2);
    else if (Kr_bytes <= 768)
      iind = 123 + ((Kr_bytes-256)>>3);
    else {
      LOG_E(PHY,"ulsch_decoding: Illegal codeword size %d!!!\n",Kr_bytes);
      return(-1);
    }

#ifdef DEBUG_ULSCH_DECODING
    printf("f1 %d, f2 %d, F %d\n",f1f2mat_old[2*iind],f1f2mat_old[1+(2*iind)],(r==0) ? ulsch_harq->F : 0);
#endif

    memset(&dummy_w[r][0],0,3*(6144+64)*sizeof(short));
    ulsch_harq->RTC[r] = generate_dummy_w(4+(Kr_bytes*8),
                                          (uint8_t*)&dummy_w[r][0],
                                          (r==0) ? ulsch_harq->F : 0);

#ifdef DEBUG_ULSCH_DECODING
    printf("Rate Matching Segment %d (coded bits (G) %d,unpunctured/repeated bits %d, Q_m %d, nb_rb %d, Nl %d)...\n",
        r, G,
        Kr*3,
        Q_m,
        nb_rb,
        ulsch_harq->Nl);
#endif


    if (lte_rate_matching_turbo_rx(ulsch_harq->RTC[r],
                                   G,
                                   ulsch_harq->w[r],
                                   (uint8_t*) &dummy_w[r][0],
                                   ulsch_harq->e+r_offset,
                                   ulsch_harq->C,
                                   NSOFT,
                                   0,   //Uplink
                                   1,
                                   ulsch_harq->rvidx,
                                   (ulsch_harq->round==0)?1:0,  // clear
                                   get_Qm_ul(ulsch_harq->mcs),
                                   1,
                                   r,
                                   &E)==-1) {
      LOG_E(PHY,"ulsch_decoding.c: Problem in rate matching\n");
      return(-1);
    }

    r_offset += E;

    sub_block_deinterleaving_turbo(4+Kr,
                                   &ulsch_harq->d[r][96],
                                   ulsch_harq->w[r]);

    if (ulsch_harq->C == 1)
      crc_type = CRC24_A;
    else
      crc_type = CRC24_B;
    
    
    ret = tc(&ulsch_harq->d[r][96],
	     ulsch_harq->c[r],
	     Kr,
	     f1f2mat_old[iind*2],
	     f1f2mat_old[(iind*2)+1],
	     ulsch->max_turbo_iterations,//MAX_TURBO_ITERATIONS,
	     crc_type,
	     (r==0) ? ulsch_harq->F : 0,
	     &eNB->ulsch_tc_init_stats,
	     &eNB->ulsch_tc_alpha_stats,
	     &eNB->ulsch_tc_beta_stats,
	     &eNB->ulsch_tc_gamma_stats,
	     &eNB->ulsch_tc_ext_stats,
	     &eNB->ulsch_tc_intl1_stats,
	     &eNB->ulsch_tc_intl2_stats);
    
    // Reassembly of Transport block here

    if (ret != (1+ulsch->max_turbo_iterations)) {
      if (r<ulsch_harq->Cminus)
	Kr = ulsch_harq->Kminus;
      else
	Kr = ulsch_harq->Kplus;
      
      Kr_bytes = Kr>>3;
      
      memcpy(ulsch_harq->b+offset,
	     ulsch_harq->c[r],
	     Kr_bytes - ((ulsch_harq->C>1)?3:0));
      offset += (Kr_bytes- ((ulsch_harq->C>1)?3:0));
      
      
    } else {
      break;
    }
    
  }

  return(ret);
}

extern int oai_exit;
void *td_thread(void *param) {
  pthread_setname_np( pthread_self(), "td processing");
  PHY_VARS_eNB *eNB = ((td_params*)param)->eNB;
  eNB_proc_t *proc  = &eNB->proc;

  while (!oai_exit) {

    if (wait_on_condition(&proc->mutex_td,&proc->cond_td,&proc->instance_cnt_td,"td thread")<0) break;  

    ((td_params*)param)->ret = ulsch_decoding_data_2thread0((td_params*)param);

    if (release_thread(&proc->mutex_td,&proc->instance_cnt_td,"td thread")<0) break;

    if (pthread_cond_signal(&proc->cond_td) != 0) {
      printf("[eNB] ERROR pthread_cond_signal for td thread exit\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return(NULL);
    }
  }

  return(NULL);
}

int ulsch_decoding_data_2thread(PHY_VARS_eNB *eNB,int UE_id,int harq_pid,int llr8_flag) {

  eNB_proc_t *proc = &eNB->proc;
  unsigned int r,r_offset=0,Kr,Kr_bytes,iind;
  uint8_t crc_type;
  int offset = 0;
  int ret = 1;
  int16_t dummy_w[MAX_NUM_ULSCH_SEGMENTS][3*(6144+64)];
  LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[UE_id];
  LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];
  //int Q_m = get_Qm_ul(ulsch_harq->mcs);
  int G = ulsch_harq->G;
  unsigned int E;
  int Cby2;

  uint8_t (*tc)(int16_t *y,
                uint8_t *,
                uint16_t,
                uint16_t,
                uint16_t,
                uint8_t,
                uint8_t,
                uint8_t,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *);

  struct timespec wait;

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;


  if (llr8_flag == 0)
    tc = phy_threegpplte_turbo_decoder16;
  else
    tc = phy_threegpplte_turbo_decoder8;

  if (ulsch_harq->C>1) { // wakeup worker if more than 1 segment
    if (pthread_mutex_timedlock(&proc->mutex_td,&wait) != 0) {
      printf("[eNB] ERROR pthread_mutex_lock for TD thread (IC %d)\n", proc->instance_cnt_td);
      exit_fun( "error locking mutex_fep" );
      return -1;
    }
    
    if (proc->instance_cnt_td==0) {
      printf("[eNB] TD thread busy\n");
      exit_fun("TD thread busy");
      pthread_mutex_unlock( &proc->mutex_td );
      return -1;
    }
    
    ++proc->instance_cnt_td;
    
    proc->tdp.eNB       = eNB;
    proc->tdp.UE_id     = UE_id;
    proc->tdp.harq_pid  = harq_pid;
    proc->tdp.llr8_flag = llr8_flag;
    
    
    // wakeup worker to do second half segments 
    if (pthread_cond_signal(&proc->cond_td) != 0) {
      printf("[eNB] ERROR pthread_cond_signal for td thread exit\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return (1+ulsch->max_turbo_iterations);
    }

    pthread_mutex_unlock( &proc->mutex_td );
    Cby2 = ulsch_harq->C/2;
  }
  else {
    Cby2 = 1;
  }

  // go through first half of segments in main thread
  for (r=0; r<Cby2; r++) {

    //    printf("before subblock deinterleaving c[%d] = %p\n",r,ulsch_harq->c[r]);
    // Get Turbo interleaver parameters
    if (r<ulsch_harq->Cminus)
      Kr = ulsch_harq->Kminus;
    else
      Kr = ulsch_harq->Kplus;

    Kr_bytes = Kr>>3;

    if (Kr_bytes<=64)
      iind = (Kr_bytes-5);
    else if (Kr_bytes <=128)
      iind = 59 + ((Kr_bytes-64)>>1);
    else if (Kr_bytes <= 256)
      iind = 91 + ((Kr_bytes-128)>>2);
    else if (Kr_bytes <= 768)
      iind = 123 + ((Kr_bytes-256)>>3);
    else {
      LOG_E(PHY,"ulsch_decoding: Illegal codeword size %d!!!\n",Kr_bytes);
      return(-1);
    }

#ifdef DEBUG_ULSCH_DECODING
    printf("f1 %d, f2 %d, F %d\n",f1f2mat_old[2*iind],f1f2mat_old[1+(2*iind)],(r==0) ? ulsch_harq->F : 0);
#endif

    memset(&dummy_w[r][0],0,3*(6144+64)*sizeof(short));
    ulsch_harq->RTC[r] = generate_dummy_w(4+(Kr_bytes*8),
                                          (uint8_t*)&dummy_w[r][0],
                                          (r==0) ? ulsch_harq->F : 0);

#ifdef DEBUG_ULSCH_DECODING
    printf("Rate Matching Segment %d (coded bits (G) %d,unpunctured/repeated bits %d, Q_m %d, nb_rb %d, Nl %d)...\n",
        r, G,
        Kr*3,
        Q_m,
        nb_rb,
        ulsch_harq->Nl);
#endif

    start_meas(&eNB->ulsch_rate_unmatching_stats);

    if (lte_rate_matching_turbo_rx(ulsch_harq->RTC[r],
                                   G,
                                   ulsch_harq->w[r],
                                   (uint8_t*) &dummy_w[r][0],
                                   ulsch_harq->e+r_offset,
                                   ulsch_harq->C,
                                   NSOFT,
                                   0,   //Uplink
                                   1,
                                   ulsch_harq->rvidx,
                                   (ulsch_harq->round==0)?1:0,  // clear
                                   get_Qm_ul(ulsch_harq->mcs),
                                   1,
                                   r,
                                   &E)==-1) {
      LOG_E(PHY,"ulsch_decoding.c: Problem in rate matching\n");
      return(-1);
    }

    stop_meas(&eNB->ulsch_rate_unmatching_stats);
    r_offset += E;

    start_meas(&eNB->ulsch_deinterleaving_stats);
    sub_block_deinterleaving_turbo(4+Kr,
                                   &ulsch_harq->d[r][96],
                                   ulsch_harq->w[r]);
    stop_meas(&eNB->ulsch_deinterleaving_stats);

    if (ulsch_harq->C == 1)
      crc_type = CRC24_A;
    else
      crc_type = CRC24_B;

    start_meas(&eNB->ulsch_turbo_decoding_stats);
    
    ret = tc(&ulsch_harq->d[r][96],
	     ulsch_harq->c[r],
	     Kr,
	     f1f2mat_old[iind*2],
	     f1f2mat_old[(iind*2)+1],
	     ulsch->max_turbo_iterations,//MAX_TURBO_ITERATIONS,
	     crc_type,
	     (r==0) ? ulsch_harq->F : 0,
	     &eNB->ulsch_tc_init_stats,
	     &eNB->ulsch_tc_alpha_stats,
	     &eNB->ulsch_tc_beta_stats,
	     &eNB->ulsch_tc_gamma_stats,
	     &eNB->ulsch_tc_ext_stats,
	     &eNB->ulsch_tc_intl1_stats,
	     &eNB->ulsch_tc_intl2_stats);

  // Reassembly of Transport block here

    if (ret != (1+ulsch->max_turbo_iterations)) {
      if (r<ulsch_harq->Cminus)
	Kr = ulsch_harq->Kminus;
      else
	Kr = ulsch_harq->Kplus;
      
      Kr_bytes = Kr>>3;
      
      if (r==0) {
	memcpy(ulsch_harq->b,
	       &ulsch_harq->c[0][(ulsch_harq->F>>3)],
	       Kr_bytes - (ulsch_harq->F>>3) - ((ulsch_harq->C>1)?3:0));
	offset = Kr_bytes - (ulsch_harq->F>>3) - ((ulsch_harq->C>1)?3:0);
      } else {
	memcpy(ulsch_harq->b+offset,
	       ulsch_harq->c[r],
	       Kr_bytes - ((ulsch_harq->C>1)?3:0));
	offset += (Kr_bytes- ((ulsch_harq->C>1)?3:0));
      }
      
    } else {
      break;
    }
    stop_meas(&eNB->ulsch_turbo_decoding_stats);    
  }

   // wait for worker to finish

  wait_on_busy_condition(&proc->mutex_td,&proc->cond_td,&proc->instance_cnt_td,"td thread");  

  return( (ret>proc->tdp.ret) ? ret : proc->tdp.ret );
}

int ulsch_decoding_data(PHY_VARS_eNB *eNB,int UE_id,int harq_pid,int llr8_flag) {

  unsigned int r,r_offset=0,Kr,Kr_bytes,iind;
  uint8_t crc_type;
  int offset = 0;
  int ret = 1;
  int16_t dummy_w[MAX_NUM_ULSCH_SEGMENTS][3*(6144+64)];
  LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[UE_id];
  LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];
  //int Q_m = get_Qm_ul(ulsch_harq->mcs);
  int G = ulsch_harq->G;
  unsigned int E;

  uint8_t (*tc)(int16_t *y,
                uint8_t *,
                uint16_t,
                uint16_t,
                uint16_t,
                uint8_t,
                uint8_t,
                uint8_t,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *);

  if (llr8_flag == 0)
    tc = phy_threegpplte_turbo_decoder16;
  else
    tc = phy_threegpplte_turbo_decoder8;


  for (r=0; r<ulsch_harq->C; r++) {

    //    printf("before subblock deinterleaving c[%d] = %p\n",r,ulsch_harq->c[r]);
    // Get Turbo interleaver parameters
    if (r<ulsch_harq->Cminus)
      Kr = ulsch_harq->Kminus;
    else
      Kr = ulsch_harq->Kplus;

    Kr_bytes = Kr>>3;

    if (Kr_bytes<=64)
      iind = (Kr_bytes-5);
    else if (Kr_bytes <=128)
      iind = 59 + ((Kr_bytes-64)>>1);
    else if (Kr_bytes <= 256)
      iind = 91 + ((Kr_bytes-128)>>2);
    else if (Kr_bytes <= 768)
      iind = 123 + ((Kr_bytes-256)>>3);
    else {
      LOG_E(PHY,"ulsch_decoding: Illegal codeword size %d!!!\n",Kr_bytes);
      return(-1);
    }

#ifdef DEBUG_ULSCH_DECODING
    printf("f1 %d, f2 %d, F %d\n",f1f2mat_old[2*iind],f1f2mat_old[1+(2*iind)],(r==0) ? ulsch_harq->F : 0);
#endif

    memset(&dummy_w[r][0],0,3*(6144+64)*sizeof(short));
    ulsch_harq->RTC[r] = generate_dummy_w(4+(Kr_bytes*8),
                                          (uint8_t*)&dummy_w[r][0],
                                          (r==0) ? ulsch_harq->F : 0);

#ifdef DEBUG_ULSCH_DECODING
    printf("Rate Matching Segment %d (coded bits (G) %d,unpunctured/repeated bits %d, Q_m %d, nb_rb %d, Nl %d)...\n",
        r, G,
        Kr*3,
        Q_m,
        nb_rb,
        ulsch_harq->Nl);
#endif

    start_meas(&eNB->ulsch_rate_unmatching_stats);

    if (lte_rate_matching_turbo_rx(ulsch_harq->RTC[r],
                                   G,
                                   ulsch_harq->w[r],
                                   (uint8_t*) &dummy_w[r][0],
                                   ulsch_harq->e+r_offset,
                                   ulsch_harq->C,
                                   NSOFT,
                                   0,   //Uplink
                                   1,
                                   ulsch_harq->rvidx,
                                   (ulsch_harq->round==0)?1:0,  // clear
                                   get_Qm_ul(ulsch_harq->mcs),
                                   1,
                                   r,
                                   &E)==-1) {
      LOG_E(PHY,"ulsch_decoding.c: Problem in rate matching\n");
      return(-1);
    }

    stop_meas(&eNB->ulsch_rate_unmatching_stats);
    r_offset += E;

    start_meas(&eNB->ulsch_deinterleaving_stats);
    sub_block_deinterleaving_turbo(4+Kr,
                                   &ulsch_harq->d[r][96],
                                   ulsch_harq->w[r]);
    stop_meas(&eNB->ulsch_deinterleaving_stats);

    if (ulsch_harq->C == 1)
      crc_type = CRC24_A;
    else
      crc_type = CRC24_B;
   
    start_meas(&eNB->ulsch_turbo_decoding_stats);
    
    ret = tc(&ulsch_harq->d[r][96],
	     ulsch_harq->c[r],
	     Kr,
	     f1f2mat_old[iind*2],
	     f1f2mat_old[(iind*2)+1],
	     ulsch->max_turbo_iterations,//MAX_TURBO_ITERATIONS,
	     crc_type,
	     (r==0) ? ulsch_harq->F : 0,
	     &eNB->ulsch_tc_init_stats,
	     &eNB->ulsch_tc_alpha_stats,
	     &eNB->ulsch_tc_beta_stats,
	     &eNB->ulsch_tc_gamma_stats,
	     &eNB->ulsch_tc_ext_stats,
	     &eNB->ulsch_tc_intl1_stats,
	     &eNB->ulsch_tc_intl2_stats);
    
    stop_meas(&eNB->ulsch_turbo_decoding_stats);
    
  // Reassembly of Transport block here

    if (ret != (1+ulsch->max_turbo_iterations)) {
      if (r<ulsch_harq->Cminus)
	Kr = ulsch_harq->Kminus;
      else
	Kr = ulsch_harq->Kplus;
      
      Kr_bytes = Kr>>3;
      
      if (r==0) {
	memcpy(ulsch_harq->b,
	       &ulsch_harq->c[0][(ulsch_harq->F>>3)],
	       Kr_bytes - (ulsch_harq->F>>3) - ((ulsch_harq->C>1)?3:0));
	offset = Kr_bytes - (ulsch_harq->F>>3) - ((ulsch_harq->C>1)?3:0);
      } else {
	memcpy(ulsch_harq->b+offset,
	       ulsch_harq->c[r],
	       Kr_bytes - ((ulsch_harq->C>1)?3:0));
	offset += (Kr_bytes- ((ulsch_harq->C>1)?3:0));
      }
      
    } else {
      break;
    }
    
  }

  return(ret);
}

static inline unsigned int lte_gold_unscram(unsigned int *x1, unsigned int *x2, unsigned char reset) __attribute__((always_inline));
static inline unsigned int lte_gold_unscram(unsigned int *x1, unsigned int *x2, unsigned char reset)
{
  int n;

  if (reset) {
    *x1 = 1+ (1<<31);
    *x2=*x2 ^ ((*x2 ^ (*x2>>1) ^ (*x2>>2) ^ (*x2>>3))<<31);

    // skip first 50 double words (1600 bits)
    //      printf("n=0 : x1 %x, x2 %x\n",x1,x2);
    for (n=1; n<50; n++) {
      *x1 = (*x1>>1) ^ (*x1>>4);
      *x1 = *x1 ^ (*x1<<31) ^ (*x1<<28);
      *x2 = (*x2>>1) ^ (*x2>>2) ^ (*x2>>3) ^ (*x2>>4);
      *x2 = *x2 ^ (*x2<<31) ^ (*x2<<30) ^ (*x2<<29) ^ (*x2<<28);
    }
  }

  *x1 = (*x1>>1) ^ (*x1>>4);
  *x1 = *x1 ^ (*x1<<31) ^ (*x1<<28);
  *x2 = (*x2>>1) ^ (*x2>>2) ^ (*x2>>3) ^ (*x2>>4);
  *x2 = *x2 ^ (*x2<<31) ^ (*x2<<30) ^ (*x2<<29) ^ (*x2<<28);
  return(*x1^*x2);
  //  printf("n=%d : c %x\n",n,x1^x2);

}
  
unsigned int  ulsch_decoding(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,
                             uint8_t UE_id,
                             uint8_t control_only_flag,
                             uint8_t Nbundled,
                             uint8_t llr8_flag)
{


  int16_t *ulsch_llr = eNB->pusch_vars[UE_id]->llr;
  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;
  LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[UE_id];
  uint8_t harq_pid;
  unsigned short nb_rb;
  unsigned int A;
  uint8_t Q_m;
  unsigned int i,i2,q,j,j2;
  int iprime;
  unsigned int ret=0;

  //  uint8_t dummy_channel_output[(3*8*block_length)+12];
  int r,Kr;

  uint8_t *columnset;
  unsigned int sumKr=0;
  unsigned int Qprime,L,G,Q_CQI,Q_RI,H,Hprime,Hpp,Cmux,Rmux_prime,O_RCC;
  unsigned int Qprime_ACK,Qprime_RI,len_ACK=0,len_RI=0;
  //  uint8_t q_ACK[MAX_ACK_PAYLOAD],q_RI[MAX_RI_PAYLOAD];
  int metric,metric_new;
  uint8_t o_flip[8];
  uint32_t x1, x2, s=0;
  int16_t ys,c;
  uint32_t wACK_idx;
  uint8_t dummy_w_cc[3*(MAX_CQI_BITS+8+32)];
  int16_t y[6*14*1200] __attribute__((aligned(32)));
  uint8_t ytag[14*1200];
  //  uint8_t ytag2[6*14*1200],*ytag2_ptr;
  int16_t cseq[6*14*1200];
  int off;

  int subframe = proc->subframe_rx;
  LTE_UL_eNB_HARQ_t *ulsch_harq;



  harq_pid = subframe2harq_pid(frame_parms,proc->frame_rx,subframe);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_ULSCH_DECODING0+harq_pid,1);

  // x1 is set in lte_gold_generic
  x2 = ((uint32_t)ulsch->rnti<<14) + ((uint32_t)subframe<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.3.1
  ulsch_harq = ulsch->harq_processes[harq_pid];

  if (harq_pid==255) {
    LOG_E(PHY, "FATAL ERROR: illegal harq_pid, returning\n");
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_ULSCH_DECODING0+harq_pid,0);
    return -1;
  }

  if (ulsch_harq->Nsymb_pusch == 0) {
      LOG_E(PHY, "FATAL ERROR: harq_pid %d, Nsymb 0!\n",harq_pid);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_ULSCH_DECODING0+harq_pid,0); 
      return 1+ulsch->max_turbo_iterations;
  }


  nb_rb = ulsch_harq->nb_rb;

  A = ulsch_harq->TBS;


  Q_m = get_Qm_ul(ulsch_harq->mcs);
  G = nb_rb * (12 * Q_m) * ulsch_harq->Nsymb_pusch;


#ifdef DEBUG_ULSCH_DECODING
  printf("ulsch_decoding (Nid_cell %d, rnti %x, x2 %x): round %d, RV %d, mcs %d, O_RI %d, O_ACK %d, G %d, subframe %d\n",
      frame_parms->Nid_cell,ulsch->rnti,x2,
      ulsch_harq->round,
      ulsch_harq->rvidx,
      ulsch_harq->mcs,
      ulsch_harq->O_RI,
      ulsch_harq->O_ACK,
      G,
      subframe);
#endif

  if (ulsch_harq->round == 0) {
    // This is a new packet, so compute quantities regarding segmentation
    ulsch_harq->B = A+24;
    lte_segmentation(NULL,
                     NULL,
                     ulsch_harq->B,
                     &ulsch_harq->C,
                     &ulsch_harq->Cplus,
                     &ulsch_harq->Cminus,
                     &ulsch_harq->Kplus,
                     &ulsch_harq->Kminus,
                     &ulsch_harq->F);
    //  CLEAR LLR's HERE for first packet in process
  }

  //  printf("after segmentation c[%d] = %p\n",0,ulsch_harq->c[0]);

  sumKr = 0;

  for (r=0; r<ulsch_harq->C; r++) {
    if (r<ulsch_harq->Cminus)
      Kr = ulsch_harq->Kminus;
    else
      Kr = ulsch_harq->Kplus;

    sumKr += Kr;
  }

  if (sumKr==0) {
    LOG_N(PHY,"[eNB %d] ulsch_decoding.c: FATAL sumKr is 0!\n",eNB->Mod_id);
    LOG_D(PHY,"ulsch_decoding (Nid_cell %d, rnti %x, x2 %x): harq_pid %d round %d, RV %d, mcs %d, O_RI %d, O_ACK %d, G %d, subframe %d\n",
          frame_parms->Nid_cell,ulsch->rnti,x2,
          harq_pid,
          ulsch_harq->round,
          ulsch_harq->rvidx,
          ulsch_harq->mcs,
          ulsch_harq->O_RI,
          ulsch_harq->O_ACK,
          G,
          subframe);
    mac_xface->macphy_exit("ulsch_decoding.c: FATAL sumKr is 0!");
    return(-1);
  }

  // Compute Q_ri
  Qprime = ulsch_harq->O_RI*ulsch_harq->Msc_initial*ulsch_harq->Nsymb_initial * ulsch->beta_offset_ri_times8;

  if (Qprime > 0 ) {
    if ((Qprime % (8*sumKr)) > 0)
      Qprime = 1+(Qprime/(8*sumKr));
    else
      Qprime = Qprime/(8*sumKr);

    if (Qprime > 4*nb_rb * 12)
      Qprime = 4*nb_rb * 12;
  }

  Q_RI = Q_m*Qprime;
  Qprime_RI = Qprime;


  // Compute Q_ack

  Qprime = ulsch_harq->O_ACK*ulsch_harq->Msc_initial*ulsch_harq->Nsymb_initial * ulsch->beta_offset_harqack_times8;

  if (Qprime > 0) {
    if ((Qprime % (8*sumKr)) > 0)
      Qprime = 1+(Qprime/(8*sumKr));
    else
      Qprime = Qprime/(8*sumKr);

    if (Qprime > (4*nb_rb * 12))
      Qprime = 4*nb_rb * 12;
  }

  //  Q_ACK = Qprime * Q_m;
  Qprime_ACK = Qprime;
#ifdef DEBUG_ULSCH_DECODING
  printf("ulsch_decoding.c: Qprime_ACK %d, Msc_initial %d, Nsymb_initial %d, sumKr %d\n",
      Qprime_ACK,ulsch_harq->Msc_initial,ulsch_harq->Nsymb_initial,sumKr);
#endif

  // Compute Q_cqi
  if (ulsch_harq->Or1 < 12)
    L=0;
  else
    L=8;

  // NOTE: we have to handle the case where we have a very small number of bits (condition on pg. 26 36.212)
  if (ulsch_harq->Or1 > 0)
    Qprime = (ulsch_harq->Or1 + L) * ulsch_harq->Msc_initial*ulsch_harq->Nsymb_initial * ulsch->beta_offset_cqi_times8;
  else
    Qprime=0;

  if (Qprime > 0) {  // check if ceiling is larger than floor in Q' expression
    if ((Qprime % (8*sumKr)) > 0)
      Qprime = 1+(Qprime/(8*sumKr));
    else
      Qprime = Qprime/(8*sumKr);
  }

  G = nb_rb * (12 * Q_m) * (ulsch_harq->Nsymb_pusch);



  Q_CQI = Q_m * Qprime;
#ifdef DEBUG_ULSCH_DECODING
  printf("ulsch_decoding: G %d, Q_RI %d, Q_CQI %d (L %d, Or1 %d) O_ACK %d\n",G,Q_RI,Q_CQI,L,ulsch_harq->Or1,ulsch_harq->O_ACK);
#endif

  G = G - Q_RI - Q_CQI;
  ulsch_harq->G = G;

  if ((int)G < 0) {
    LOG_E(PHY,"FATAL: ulsch_decoding.c G < 0 (%d) : Q_RI %d, Q_CQI %d\n",G,Q_RI,Q_CQI);
    return(-1);
  }

  H = G + Q_CQI;
  Hprime = H/Q_m;


  // Demultiplexing/Deinterleaving of PUSCH/ACK/RI/CQI
  start_meas(&eNB->ulsch_demultiplexing_stats);
  Hpp = Hprime + Qprime_RI;

  Cmux       = ulsch_harq->Nsymb_pusch;
  Rmux_prime = Hpp/Cmux;

  // Clear "tag" interleaving matrix to allow for CQI/DATA identification
  memset(ytag,0,Cmux*Rmux_prime);



  i=0;
  memset(y,LTE_NULL,Q_m*Hpp);

  // read in buffer and unscramble llrs for everything but placeholder bits
  // llrs stored per symbol correspond to columns of interleaving matrix


  s = lte_gold_unscram(&x1, &x2, 1);
  i2=0;

  for (i=0; i<((Hpp*Q_m)>>5); i++) {
    /*
    for (j=0; j<32; j++) {
      cseq[i2++] = (int16_t)((((s>>j)&1)<<1)-1);
    }
    */
#if defined(__x86_64__) || defined(__i386__)
#ifndef __AVX2__
    ((__m128i*)cseq)[i2++] = ((__m128i*)unscrambling_lut)[(s&65535)<<1];
    ((__m128i*)cseq)[i2++] = ((__m128i*)unscrambling_lut)[1+((s&65535)<<1)];
    s>>=16;
    ((__m128i*)cseq)[i2++] = ((__m128i*)unscrambling_lut)[(s&65535)<<1];
    ((__m128i*)cseq)[i2++] = ((__m128i*)unscrambling_lut)[1+((s&65535)<<1)];
#else
    ((__m256i*)cseq)[i2++] = ((__m256i*)unscrambling_lut)[s&65535];
    ((__m256i*)cseq)[i2++] = ((__m256i*)unscrambling_lut)[(s>>16)&65535];
#endif
#elif defined(__arm__)
    ((int16x8_t*)cseq)[i2++] = ((int16x8_t*)unscrambling_lut)[(s&65535)<<1];
    ((int16x8_t*)cseq)[i2++] = ((int16x8_t*)unscrambling_lut)[1+((s&65535)<<1)];
    s>>=16;
    ((int16x8_t*)cseq)[i2++] = ((int16x8_t*)unscrambling_lut)[(s&65535)<<1];
    ((int16x8_t*)cseq)[i2++] = ((int16x8_t*)unscrambling_lut)[1+((s&65535)<<1)];
#endif
    s = lte_gold_unscram(&x1, &x2, 0);
  }


  //  printf("after unscrambling c[%d] = %p\n",0,ulsch_harq->c[0]);

  if (frame_parms->Ncp == 0)
    columnset = cs_ri_normal;
  else
    columnset = cs_ri_extended;

  j=0;

  for (i=0; i<Qprime_RI; i++) {
    r = Rmux_prime - 1 - (i>>2);
    /*
    for (q=0;q<Q_m;q++)
      ytag2[q+(Q_m*((r*Cmux) + columnset[j]))]  = q_RI[(q+(Q_m*i))%len_RI];
    */
    off =((Rmux_prime*Q_m*columnset[j])+(r*Q_m));
    cseq[off+1] = cseq[off];  // PUSCH_y

    for (q=2; q<Q_m; q++)
      cseq[off+q] = -1;    // PUSCH_x

    j=(j+3)&3;

  }

  //  printf("after RI c[%d] = %p\n",0,ulsch_harq->c[0]);

  // HARQ-ACK Bits (Note these overwrite some bits)
  if (frame_parms->Ncp == 0)
    columnset = cs_ack_normal;
  else
    columnset = cs_ack_extended;

  j=0;

  for (i=0; i<Qprime_ACK; i++) {
    r = Rmux_prime - 1 - (i>>2);
    off =((Rmux_prime*Q_m*columnset[j])+(r*Q_m));

    if (ulsch_harq->O_ACK == 1) {
      if (ulsch->bundling==0)
        cseq[off+1] = cseq[off];  // PUSCH_y

      for (q=2; q<Q_m; q++)
        cseq[off+q] = -1;    // PUSCH_x
    } else if (ulsch_harq->O_ACK == 2) {
      for (q=2; q<Q_m; q++)
        cseq[off+q] = -1;    // PUSCH_x
    }

#ifdef DEBUG_ULSCH_DECODING
    printf("ulsch_decoding.c: ACK i %d, r %d, j %d, ColumnSet[j] %d\n",i,r,j,columnset[j]);
#endif
    j=(j+3)&3;
  }



  i=0;

  switch (Q_m) {
  case 2:
    for (j=0; j<Cmux; j++) {
      i2=j<<1;

      for (r=0; r<Rmux_prime; r++) {
        c = cseq[i];
        //  printf("ulsch %d: %d * ",i,c);
        y[i2++] = c*ulsch_llr[i++];
        //  printf("%d\n",ulsch_llr[i-1]);
        c = cseq[i];
        //  printf("ulsch %d: %d * ",i,c);
        y[i2] = c*ulsch_llr[i++];
        //  printf("%d\n",ulsch_llr[i-1]);
        i2=(i2+(Cmux<<1)-1);
      }
    }

    break;

  case 4:
    for (j=0; j<Cmux; j++) {
      i2=j<<2;

      for (r=0; r<Rmux_prime; r++) {
	/*
        c = cseq[i];
        y[i2++] = c*ulsch_llr[i++];
        c = cseq[i];
        y[i2++] = c*ulsch_llr[i++];
        c = cseq[i];
        y[i2++] = c*ulsch_llr[i++];
        c = cseq[i];
        y[i2] = c*ulsch_llr[i++];
        i2=(i2+(Cmux<<2)-3);
	*/
	// slightly more optimized version (equivalent to above) for 16QAM to improve computational performance
	*(__m64 *)&y[i2] = _mm_sign_pi16(*(__m64*)&ulsch_llr[i],*(__m64*)&cseq[i]);i+=4;i2+=(Cmux<<2);


      }
    }

    break;

  case 6:
    for (j=0; j<Cmux; j++) {
      i2=j*6;

      for (r=0; r<Rmux_prime; r++) {
        c = cseq[i];
        y[i2++] = c*ulsch_llr[i++];
        c = cseq[i];
        y[i2++] = c*ulsch_llr[i++];
        c = cseq[i];
        y[i2++] = c*ulsch_llr[i++];
        c = cseq[i];
        y[i2++] = c*ulsch_llr[i++];
        c = cseq[i];
        y[i2++] = c*ulsch_llr[i++];
        c = cseq[i];
        y[i2] = c*ulsch_llr[i++];
        i2=(i2+(Cmux*6)-5);
      }
    }

    break;
  }




  if (i!=(H+Q_RI))
    LOG_D(PHY,"ulsch_decoding.c: Error in input buffer length (j %d, H+Q_RI %d)\n",i,H+Q_RI);

  // HARQ-ACK Bits (LLRs are nulled in overwritten bits after copying HARQ-ACK LLR)

  if (frame_parms->Ncp == 0)
    columnset = cs_ack_normal;
  else
    columnset = cs_ack_extended;

  j=0;

  if (ulsch_harq->O_ACK == 1) {
    switch (Q_m) {
    case 2:
      len_ACK = 2;
      break;

    case 4:
      len_ACK = 4;
      break;

    case 6:
      len_ACK = 6;
      break;
    }
  }

  if (ulsch_harq->O_ACK == 2) {
    switch (Q_m) {
    case 2:
      len_ACK = 6;
      break;

    case 4:
      len_ACK = 12;
      break;

    case 6:
      len_ACK = 18;
      break;
    }
  }

  if (ulsch_harq->O_ACK > 2) {
    LOG_E(PHY,"ulsch_decoding: FATAL, ACK cannot be more than 2 bits yet\n");
    return(-1);
  }

  for (i=0; i<len_ACK; i++)
    ulsch_harq->q_ACK[i] = 0;


  for (i=0; i<Qprime_ACK; i++) {
    r = Rmux_prime -1 - (i>>2);

    for (q=0; q<Q_m; q++) {
      if (y[q+(Q_m*((r*Cmux) + columnset[j]))]!=0)
        ulsch_harq->q_ACK[(q+(Q_m*i))%len_ACK] += y[q+(Q_m*((r*Cmux) + columnset[j]))];
      y[q+(Q_m*((r*Cmux) + columnset[j]))]=0;  // NULL LLRs in ACK positions
    }

    j=(j+3)&3;
  }

  //  printf("after ACKNAK c[%d] = %p\n",0,ulsch_harq->c[0]);

  // RI BITS

  if (ulsch_harq->O_RI == 1) {
    switch (Q_m) {
    case 2:
      len_RI=2;
      break;

    case 4:
      len_RI=4;
      break;

    case 6:
      len_RI=6;
      break;
    }
  }

  if (ulsch_harq->O_RI > 1) {
    LOG_E(PHY,"ulsch_decoding: FATAL, RI cannot be more than 1 bit yet\n");
    return(-1);
  }

  for (i=0; i<len_RI; i++)
    ulsch_harq->q_RI[i] = 0;

  if (frame_parms->Ncp == 0)
    columnset = cs_ri_normal;
  else
    columnset = cs_ri_extended;

  j=0;

  for (i=0; i<Qprime_RI; i++) {
    r = Rmux_prime -1 - (i>>2);

    for (q=0; q<Q_m; q++)
      ulsch_harq->q_RI[(q+(Q_m*i))%len_RI] += y[q+(Q_m*((r*Cmux) + columnset[j]))];

    ytag[(r*Cmux) + columnset[j]] = LTE_NULL;
    j=(j+3)&3;
  }

  //  printf("after RI2 c[%d] = %p\n",0,ulsch_harq->c[0]);

  // CQI and Data bits
  j=0;
  j2=0;

  //  r=0;
  if (Q_RI>0) {
    for (i=0; i<(Q_CQI/Q_m); i++) {
      
      while (ytag[j]==LTE_NULL) {
	j++;
	j2+=Q_m;
      }
      
      for (q=0; q<Q_m; q++) {
	//      ys = y[q+(Q_m*((r*Cmux)+j))];
	ys = y[q+j2];
	
	if (ys>127)
	  ulsch_harq->q[q+(Q_m*i)] = 127;
	else if (ys<-128)
	  ulsch_harq->q[q+(Q_m*i)] = -128;
	else
	  ulsch_harq->q[q+(Q_m*i)] = ys;
      }
      
      j2+=Q_m;
    }
    
        
    switch (Q_m) {
    case 2:
      for (iprime=0; iprime<G;) {
	while (ytag[j]==LTE_NULL) {
	  j++;
	  j2+=2;
	}
	
	ulsch_harq->e[iprime++] = y[j2++];
	ulsch_harq->e[iprime++] = y[j2++];
	
      }
      
      
      break;
      
    case 4:
      for (iprime=0; iprime<G;) {
	while (ytag[j]==LTE_NULL) {
	  j++;
	  j2+=4;
	}
	
	ulsch_harq->e[iprime++] = y[j2++];
	ulsch_harq->e[iprime++] = y[j2++];
	ulsch_harq->e[iprime++] = y[j2++];
	ulsch_harq->e[iprime++] = y[j2++];
	
      }
      
      break;
      
    case 6:
      for (iprime=0; iprime<G;) {
	while (ytag[j]==LTE_NULL) {
	  j++;
	  j2+=6;
	}
	
	ulsch_harq->e[iprime++] = y[j2++];
	ulsch_harq->e[iprime++] = y[j2++];
	ulsch_harq->e[iprime++] = y[j2++];
	ulsch_harq->e[iprime++] = y[j2++];
	ulsch_harq->e[iprime++] = y[j2++];
	ulsch_harq->e[iprime++] = y[j2++];
	
      }
      
      break;

    }
    

  } // Q_RI>0
  else {

    for (i=0; i<(Q_CQI/Q_m); i++) {
      
      for (q=0; q<Q_m; q++) {
	ys = y[q+j2];
	if (ys>127)
	  ulsch_harq->q[q+(Q_m*i)] = 127;
	else if (ys<-128)
	  ulsch_harq->q[q+(Q_m*i)] = -128;
	else
	  ulsch_harq->q[q+(Q_m*i)] = ys;
      }
      
      j2+=Q_m;
    }
    /* To be improved according to alignment of j2
#if defined(__x86_64__)||defined(__i386__)
#ifndef __AVX2__
    for (iprime=0; iprime<G;iprime+=8,j2+=8)
      *((__m128i *)&ulsch_harq->e[iprime]) = *((__m128i *)&y[j2]);
#else
    for (iprime=0; iprime<G;iprime+=16,j2+=16)
      *((__m256i *)&ulsch_harq->e[iprime]) = *((__m256i *)&y[j2]);
#endif
#elif defined(__arm__)
    for (iprime=0; iprime<G;iprime+=8,j2+=8)
      *((int16x8_t *)&ulsch_harq->e[iprime]) = *((int16x8_t *)&y[j2]);
#endif
    */
    int16_t *yp,*ep;
    for (iprime=0,yp=&y[j2],ep=&ulsch_harq->e[0]; 
	 iprime<G;
	 iprime+=8,j2+=8,ep+=8,yp+=8) {
      ep[0] = yp[0];
      ep[1] = yp[1];
      ep[2] = yp[2];
      ep[3] = yp[3];
      ep[4] = yp[4];
      ep[5] = yp[5];
      ep[6] = yp[6];
      ep[7] = yp[7];
    }
  }
    
   
  stop_meas(&eNB->ulsch_demultiplexing_stats);

  //  printf("after ACKNAK2 c[%d] = %p (iprime %d, G %d)\n",0,ulsch_harq->c[0],iprime,G);

  // Do CQI/RI/HARQ-ACK Decoding first and pass to MAC

  // HARQ-ACK
  wACK_idx = (ulsch->bundling==0) ? 4 : ((Nbundled-1)&3);

  if (ulsch_harq->O_ACK == 1) {
    ulsch_harq->q_ACK[0] *= wACK_RX[wACK_idx][0];
    ulsch_harq->q_ACK[0] += (ulsch->bundling==0) ? ulsch_harq->q_ACK[1]*wACK_RX[wACK_idx][0] : ulsch_harq->q_ACK[1]*wACK_RX[wACK_idx][1];

    if (ulsch_harq->q_ACK[0] < 0)
      ulsch_harq->o_ACK[0] = 0;
    else
      ulsch_harq->o_ACK[0] = 1;
  }

  if (ulsch_harq->O_ACK == 2) {
    switch (Q_m) {

    case 2:
      ulsch_harq->q_ACK[0] = ulsch_harq->q_ACK[0]*wACK_RX[wACK_idx][0] + ulsch_harq->q_ACK[3]*wACK_RX[wACK_idx][1];
      ulsch_harq->q_ACK[1] = ulsch_harq->q_ACK[1]*wACK_RX[wACK_idx][0] + ulsch_harq->q_ACK[4]*wACK_RX[wACK_idx][1];
      ulsch_harq->q_ACK[2] = ulsch_harq->q_ACK[2]*wACK_RX[wACK_idx][0] + ulsch_harq->q_ACK[5]*wACK_RX[wACK_idx][1];
      break;
    case 4:
      ulsch_harq->q_ACK[0] = ulsch_harq->q_ACK[0]*wACK_RX[wACK_idx][0] + ulsch_harq->q_ACK[5]*wACK_RX[wACK_idx][1];
      ulsch_harq->q_ACK[1] = ulsch_harq->q_ACK[1]*wACK_RX[wACK_idx][0] + ulsch_harq->q_ACK[8]*wACK_RX[wACK_idx][1];
      ulsch_harq->q_ACK[2] = ulsch_harq->q_ACK[4]*wACK_RX[wACK_idx][0] + ulsch_harq->q_ACK[9]*wACK_RX[wACK_idx][1];
      break;
    case 6:
      ulsch_harq->q_ACK[0] =  ulsch_harq->q_ACK[0]*wACK_RX[wACK_idx][0] + ulsch_harq->q_ACK[7]*wACK_RX[wACK_idx][1];
      ulsch_harq->q_ACK[1] =  ulsch_harq->q_ACK[1]*wACK_RX[wACK_idx][0] + ulsch_harq->q_ACK[12]*wACK_RX[wACK_idx][1];
      ulsch_harq->q_ACK[2] =  ulsch_harq->q_ACK[6]*wACK_RX[wACK_idx][0] + ulsch_harq->q_ACK[13]*wACK_RX[wACK_idx][1];
      break;
    }

    ulsch_harq->o_ACK[0] = 1;
    ulsch_harq->o_ACK[1] = 1;
    metric     = ulsch_harq->q_ACK[0]+ulsch_harq->q_ACK[1]-ulsch_harq->q_ACK[2];
    metric_new = -ulsch_harq->q_ACK[0]+ulsch_harq->q_ACK[1]+ulsch_harq->q_ACK[2];

    if (metric_new > metric) {
      ulsch_harq->o_ACK[0]=0;
      ulsch_harq->o_ACK[1]=1;
      metric = metric_new;
    }

    metric_new = ulsch_harq->q_ACK[0]-ulsch_harq->q_ACK[1]+ulsch_harq->q_ACK[2];


    if (metric_new > metric) {
      ulsch_harq->o_ACK[0] = 1;
      ulsch_harq->o_ACK[1] = 0;
      metric = metric_new;
    }

    metric_new = -ulsch_harq->q_ACK[0]-ulsch_harq->q_ACK[1]-ulsch_harq->q_ACK[2];

    if (metric_new > metric) {
      ulsch_harq->o_ACK[0] = 0;
      ulsch_harq->o_ACK[1] = 0;
      metric = metric_new;
    }
  }

  // RI

  // rank 1
  if ((ulsch_harq->O_RI == 1) && (Qprime_RI > 0)) {
    ulsch_harq->o_RI[0] = ((ulsch_harq->q_RI[0] + ulsch_harq->q_RI[Q_m/2]) > 0) ? 0 : 1;
  }

  // CQI

  //  printf("before cqi c[%d] = %p\n",0,ulsch_harq->c[0]);
  ulsch_harq->cqi_crc_status = 0;
  if (Q_CQI>0) {
    memset((void *)&dummy_w_cc[0],0,3*(ulsch_harq->Or1+8+32));

    O_RCC = generate_dummy_w_cc(ulsch_harq->Or1+8,
                                &dummy_w_cc[0]);


    lte_rate_matching_cc_rx(O_RCC,
                            Q_CQI,
                            ulsch_harq->o_w,
                            dummy_w_cc,
                            ulsch_harq->q);

    sub_block_deinterleaving_cc((unsigned int)(ulsch_harq->Or1+8),
                                &ulsch_harq->o_d[96],
                                &ulsch_harq->o_w[0]);

    memset(o_flip,0,1+((8+ulsch_harq->Or1)/8));
    phy_viterbi_lte_sse2(ulsch_harq->o_d+96,o_flip,8+ulsch_harq->Or1);

    if (extract_cqi_crc(o_flip,ulsch_harq->Or1) == (crc8(o_flip,ulsch_harq->Or1)>>24))
      ulsch_harq->cqi_crc_status = 1;

    if (ulsch->harq_processes[harq_pid]->Or1<=32) {
      ulsch_harq->o[3] = o_flip[0] ;
      ulsch_harq->o[2] = o_flip[1] ;
      ulsch_harq->o[1] = o_flip[2] ;
      ulsch_harq->o[0] = o_flip[3] ;
    } else {
      ulsch_harq->o[7] = o_flip[0] ;
      ulsch_harq->o[6] = o_flip[1] ;
      ulsch_harq->o[5] = o_flip[2] ;
      ulsch_harq->o[4] = o_flip[3] ;
      ulsch_harq->o[3] = o_flip[4] ;
      ulsch_harq->o[2] = o_flip[5] ;
      ulsch_harq->o[1] = o_flip[6] ;
      ulsch_harq->o[0] = o_flip[7] ;

    }

#ifdef DEBUG_ULSCH_DECODING
    printf("ulsch_decoding: Or1=%d\n",ulsch_harq->Or1);

    for (i=0; i<1+((8+ulsch_harq->Or1)/8); i++)
      printf("ulsch_decoding: O[%d] %d\n",i,ulsch_harq->o[i]);

    if (ulsch_harq->cqi_crc_status == 1)
      printf("RX CQI CRC OK (%x)\n",extract_cqi_crc(o_flip,ulsch_harq->Or1));
    else
      printf("RX CQI CRC NOT OK (%x)\n",extract_cqi_crc(o_flip,ulsch_harq->Or1));

#endif
  }


  // Do ULSCH Decoding for data portion

  ret = eNB->td(eNB,UE_id,harq_pid,llr8_flag);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_ULSCH_DECODING0+harq_pid,0);

  return(ret);
}

#ifdef PHY_ABSTRACTION

#ifdef PHY_ABSTRACTION_UL
int ulsch_abstraction(double* sinr_dB, uint8_t TM, uint8_t mcs,uint16_t nrb, uint16_t frb)
{

  int index,ii;
  double sinr_eff = 0;
  int rb_count = 0;
  int offset;
  double bler = 0;
  TM = TM-1;
  sinr_eff = sinr_dB[frb]; //the single sinr_eff value we calculated with MMSE FDE formula in init_snr_up function


  sinr_eff *= 10;
  sinr_eff = floor(sinr_eff);
  sinr_eff /= 10;

  LOG_D(PHY,"[ABSTRACTION] sinr_eff after rounding = %f\n",sinr_eff);

  for (index = 0; index < 16; index++) {
    if(index == 0) {
      if (sinr_eff < sinr_bler_map_up[mcs][0][index]) {
        bler = 1;
        break;
      }
    }

    if (sinr_eff == sinr_bler_map_up[mcs][0][index]) {
      bler = sinr_bler_map_up[mcs][1][index];
    }
  }

#ifdef USER_MODE // need to be adapted for the emulation in the kernel space 

  if (uniformrandom() < bler) {
    LOG_I(OCM,"abstraction_decoding failed (mcs=%d, sinr_eff=%f, bler=%f)\n",mcs,sinr_eff,bler);
    return(0);
  } else {
    LOG_I(OCM,"abstraction_decoding successful (mcs=%d, sinr_eff=%f, bler=%f)\n",mcs,sinr_eff,bler);
    return(1);
  }

#endif
}







int ulsch_abstraction_MIESM(double* sinr_dB,uint8_t TM, uint8_t mcs,uint16_t nrb, uint16_t frb)
{
  int index;
  double sinr_eff = 0;
  double sinr_db1 = 0;
  double sinr_db2 = 0;
  double SI=0;
  double RBIR=0;
  int rb_count = 0;
  int offset, M=0;
  double bler = 0;
  int start,middle,end;
  TM = TM-1;

  for (offset = frb; offset <= (frb + nrb -1); offset++) {

    rb_count++;

    //we need to do the table lookups here for the mutual information corresponding to the certain sinr_dB.

    sinr_db1 = sinr_dB[offset*2];
    sinr_db2 = sinr_dB[offset*2+1];

    printf("sinr_db1=%f\n,sinr_db2=%f\n",sinr_db1,sinr_db2);

    //rounding up for the table lookup
    sinr_db1 *= 10;
    sinr_db2 *= 10;

    sinr_db1 = floor(sinr_db1);
    sinr_db2 = floor(sinr_db2);

    if ((int)sinr_db1%2) {
      sinr_db1 += 1;
    }

    if ((int)sinr_db2%2) {
      sinr_db2 += 1;
    }

    sinr_db1 /= 10;
    sinr_db2 /= 10;

    if(mcs<10) {
      //for sinr_db1
      for (index = 0; index < 162; index++) {
        if (sinr_db1 < MI_map_4qam[0][0]) {
          SI += (MI_map_4qam[1][0]/beta1_dlsch_MI[TM][mcs]);
          M +=2;
          break;
        }

        if (sinr_db1 > MI_map_4qam[0][161]) {
          SI += (MI_map_4qam[1][161]/beta1_dlsch_MI[TM][mcs]);
          M +=2;
          break;
        }

        if (sinr_db1 == MI_map_4qam[0][index]) {
          SI += (MI_map_4qam[1][index]/beta1_dlsch_MI[TM][mcs]);
          M +=2;
          break;
        }
      }

      //for sinr_db2
      for (index = 0; index < 162; index++) {
        if (sinr_db2 < MI_map_4qam[0][0]) {
          SI += (MI_map_4qam[1][0]/beta1_dlsch_MI[TM][mcs]);
          M +=2;
          break;
        }

        if (sinr_db2 > MI_map_4qam[0][161]) {
          SI += (MI_map_4qam[1][161]/beta1_dlsch_MI[TM][mcs]);
          M +=2;
          break;
        }

        if (sinr_db2 == MI_map_4qam[0][index]) {
          SI += (MI_map_4qam[1][index]/beta1_dlsch_MI[TM][mcs]);
          M +=2;
          break;
        }
      }

    } else if(mcs>9 && mcs<17) {
      //for sinr_db1
      for (index = 0; index < 197; index++) {
        if (sinr_db1 < MI_map_16qam[0][0]) {
          SI += (MI_map_16qam[1][0]/beta1_dlsch_MI[TM][mcs]);
          M +=4;
          break;
        }

        if (sinr_db1 > MI_map_16qam[0][196]) {
          SI += (MI_map_16qam[1][196]/beta1_dlsch_MI[TM][mcs]);
          M +=4;
          break;
        }

        if (sinr_db1 == MI_map_16qam[0][index]) {
          SI += (MI_map_16qam[1][index]/beta1_dlsch_MI[TM][mcs]);
          M +=4;
          break;
        }
      }

      //for sinr_db2
      for (index = 0; index < 197; index++) {
        if (sinr_db2 < MI_map_16qam[0][0]) {
          SI += (MI_map_16qam[1][0]/beta1_dlsch_MI[TM][mcs]);
          M +=4;
          break;
        }

        if (sinr_db2 > MI_map_16qam[0][196]) {
          SI += (MI_map_16qam[1][196]/beta1_dlsch_MI[TM][mcs]);
          M +=4;
          break;
        }

        if (sinr_db2 == MI_map_16qam[0][index]) {
          SI += (MI_map_16qam[1][index]/beta1_dlsch_MI[TM][mcs]);
          M +=4;
          break;
        }
      }

    } else if(mcs>16 && mcs<22) {
      //for sinr_db1
      for (index = 0; index < 227; index++) {
        if (sinr_db1 < MI_map_64qam[0][0]) {
          SI += (MI_map_64qam[1][0]/beta1_dlsch_MI[TM][mcs]);
          M +=6;
          break;
        }

        if (sinr_db1 > MI_map_64qam[0][226]) {
          SI += (MI_map_64qam[1][226]/beta1_dlsch_MI[TM][mcs]);
          M +=6;
          break;
        }

        if (sinr_db1 == MI_map_64qam[0][index]) {
          SI += (MI_map_64qam[1][index]/beta1_dlsch_MI[TM][mcs]);
          M +=6;
          break;
        }
      }

      //for sinr_db2
      for (index = 0; index < 227; index++) {
        if (sinr_db2 < MI_map_64qam[0][0]) {
          SI += (MI_map_64qam[1][0]/beta1_dlsch_MI[TM][mcs]);
          M +=6;
          break;
        }

        if (sinr_db2 > MI_map_64qam[0][226]) {
          SI += (MI_map_64qam[1][226]/beta1_dlsch_MI[TM][mcs]);
          M +=6;
          break;
        }

        if (sinr_db2 == MI_map_64qam[0][index]) {
          SI += (MI_map_64qam[1][index]/beta1_dlsch_MI[TM][mcs]);
          M +=6;
          break;
        }
      }
    }
  }

  // }

  RBIR = SI/M;

  //Now RBIR->SINR_effective Mapping
  //binary search method is performed here
  if(mcs<10) {
    start = 0;
    end = 161;
    middle = end/2;

    if (RBIR <= MI_map_4qam[2][start]) {
      sinr_eff =  MI_map_4qam[0][start];
    } else {
      if (RBIR >= MI_map_4qam[2][end])
        sinr_eff =  MI_map_4qam[0][end];
      else {
        //while((end-start > 1) && (RBIR >= MI_map_4qam[2]))
        if (RBIR < MI_map_4qam[2][middle]) {
          end = middle;
          middle = end/2;
        } else {
          start = middle;
          middle = (end-middle)/2;
        }
      }

      for (; end>start; end--) {
        if ((RBIR < MI_map_4qam[2][end]) && (RBIR >  MI_map_4qam[2][end-2])) {
          sinr_eff = MI_map_4qam[0][end-1];
          break;
        }
      }
    }

    sinr_eff = sinr_eff * beta2_dlsch_MI[TM][mcs];
  }



  else if (mcs>9 && mcs<17) {

    start = 0;
    end = 196;
    middle = end/2;

    if (RBIR <= MI_map_16qam[2][start]) {
      sinr_eff =  MI_map_16qam[0][start];
    } else {
      if (RBIR >= MI_map_16qam[2][end])
        sinr_eff =  MI_map_16qam[0][end];
      else {
        //while((end-start > 1) && (RBIR >= MI_map_4qam[2]))
        if (RBIR < MI_map_16qam[2][middle]) {
          end = middle;
          middle = end/2;
        } else {
          start = middle;
          middle = (end-middle)/2;
        }
      }

      for (; end>start; end--) {
        if ((RBIR < MI_map_16qam[2][end]) && (RBIR >  MI_map_16qam[2][end-2])) {
          sinr_eff = MI_map_16qam[0][end-1];
          break;
        }
      }
    }

    sinr_eff = sinr_eff * beta2_dlsch_MI[TM][mcs];
  } else if (mcs>16) {
    start = 0;
    end = 226;
    middle = end/2;

    if (RBIR <= MI_map_64qam[2][start]) {
      sinr_eff =  MI_map_64qam[0][start];
    } else {
      if (RBIR >= MI_map_64qam[2][end])
        sinr_eff =  MI_map_64qam[0][end];
      else {
        //while((end-start > 1) && (RBIR >= MI_map_4qam[2]))
        if (RBIR < MI_map_64qam[2][middle]) {
          end = middle;
          middle = end/2;
        } else {
          start = middle;
          middle = (end-middle)/2;
        }
      }

      for (; end>start; end--) {
        if ((RBIR < MI_map_64qam[2][end]) && (RBIR >  MI_map_64qam[2][end-2])) {
          sinr_eff = MI_map_64qam[0][end-1];
          break;
        }
      }
    }

    sinr_eff = sinr_eff * beta2_dlsch_MI[TM][mcs];
  }

  printf("SINR_Eff = %e\n",sinr_eff);

  sinr_eff *= 10;
  sinr_eff = floor(sinr_eff);
  // if ((int)sinr_eff%2) {
  //   sinr_eff += 1;
  // }
  sinr_eff /= 10;
  printf("sinr_eff after rounding = %f\n",sinr_eff);

  for (index = 0; index < 16; index++) {
    if(index == 0) {
      if (sinr_eff < sinr_bler_map_up[mcs][0][index]) {
        bler = 1;
        break;
      }
    }

    if (sinr_eff == sinr_bler_map_up[mcs][0][index]) {
      bler = sinr_bler_map_up[mcs][1][index];
    }
  }

#ifdef USER_MODE // need to be adapted for the emulation in the kernel space 

  if (uniformrandom() < bler) {
    printf("abstraction_decoding failed (mcs=%d, sinr_eff=%f, bler=%f)\n",mcs,sinr_eff,bler);
    return(0);
  } else {
    printf("abstraction_decoding successful (mcs=%d, sinr_eff=%f, bler=%f)\n",mcs,sinr_eff,bler);
    return(1);
  }

#endif

}

#endif

uint32_t ulsch_decoding_emul(PHY_VARS_eNB *eNB, eNB_rxtx_proc_t *proc,
                             uint8_t UE_index,
                             uint16_t *crnti)
{

  uint8_t UE_id;
  uint16_t rnti;
  int subframe = proc->subframe_rx;
  uint8_t harq_pid;
  uint8_t CC_id = eNB->CC_id;

  harq_pid = subframe2harq_pid(&eNB->frame_parms,proc->frame_rx,subframe);

  rnti = eNB->ulsch[UE_index]->rnti;
#ifdef DEBUG_PHY
  LOG_D(PHY,"[eNB %d] ulsch_decoding_emul : subframe %d UE_index %d harq_pid %d rnti %x\n",eNB->Mod_id,subframe,UE_index,harq_pid,rnti);
#endif

  for (UE_id=0; UE_id<NB_UE_INST; UE_id++) {
    if (rnti == PHY_vars_UE_g[UE_id][CC_id]->pdcch_vars[subframe & 0x1][0]->crnti)
      break;

  }

  if (UE_id==NB_UE_INST) {
    LOG_W(PHY,"[eNB %d] ulsch_decoding_emul: FATAL, didn't find UE with rnti %x (UE index %d)\n",
          eNB->Mod_id, rnti, UE_index);
    return(1+eNB->ulsch[UE_id]->max_turbo_iterations);
  } else {
    LOG_D(PHY,"[eNB %d] Found UE with rnti %x => UE_id %d\n",eNB->Mod_id, rnti, UE_id);
  }

  if (PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->harq_processes[harq_pid]->status == CBA_ACTIVE) {
    *crnti = rnti;
    PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->harq_processes[harq_pid]->status=IDLE;
  } else
    *crnti = 0x0;

  // Do abstraction here to determine if packet it in error
  /* if (ulsch_abstraction_MIESM(eNB->sinr_dB_eNB,1, eNB->ulsch[UE_id]->harq_processes[harq_pid]->mcs,eNB->ulsch[UE_id]->harq_processes[harq_pid]->nb_rb, eNB->ulsch[UE_id]->harq_processes[harq_pid]->first_rb) == 1)
   flag = 1;
   else flag = 0;*/


  /*
  //SINRdbPost = eNB->sinr_dB_eNB;
  mcsPost = eNB->ulsch[UE_id]->harq_processes[harq_pid]->mcs,
  nrbPost = eNB->ulsch[UE_id]->harq_processes[harq_pid]->nb_rb;
  frbPost = eNB->ulsch[UE_id]->harq_processes[harq_pid]->first_rb;


  if(nrbPost > 0)
  {
  SINRdbPost = eNB->sinr_dB_eNB;
  ULflag1 = 1;
  }
  else
  {
   SINRdbPost = NULL  ;
   ULflag1 = 0 ;
  }*/

  //
  // write_output("postprocSINR.m","SINReNB",eNB->sinr_dB,301,1,7);


  //Yazdir buraya her frame icin 300 eNb
  // fprintf(SINRrx,"%e,%e,%e,%e;\n",SINRdbPost);
  //fprintf(SINRrx,"%e\n",SINRdbPost);

  // fprintf(csv_fd,"%e+i*(%e),",channelx,channely);

  // if (ulsch_abstraction(eNB->sinr_dB,1, eNB->ulsch[UE_id]->harq_processes[harq_pid]->mcs,eNB->ulsch[UE_id]->harq_processes[harq_pid]->nb_rb, eNB->ulsch[UE_id]->harq_processes[harq_pid]->first_rb) == 1) {
  if (1) {
    LOG_D(PHY,"ulsch_decoding_emul abstraction successful\n");

    memcpy(eNB->ulsch[UE_index]->harq_processes[harq_pid]->b,
           PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->harq_processes[harq_pid]->b,
           eNB->ulsch[UE_index]->harq_processes[harq_pid]->TBS>>3);

    // get local ue's ack
    if ((UE_index >= oai_emulation.info.first_ue_local) ||(UE_index <(oai_emulation.info.first_ue_local+oai_emulation.info.nb_ue_local))) {
      get_ack(&eNB->frame_parms,
              PHY_vars_UE_g[UE_id][CC_id]->dlsch[0][0][0]->harq_ack,
              proc->subframe_tx,
              proc->subframe_rx,
              eNB->ulsch[UE_index]->harq_processes[harq_pid]->o_ACK,0);
    } else { // get remote UEs' ack
      eNB->ulsch[UE_index]->harq_processes[harq_pid]->o_ACK[0] = PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->o_ACK[0];
      eNB->ulsch[UE_index]->harq_processes[harq_pid]->o_ACK[1] = PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->o_ACK[1];
    }

    // Do abstraction of PUSCH feedback
#ifdef DEBUG_PHY
    LOG_D(PHY,"[eNB %d][EMUL] ue index %d UE_id %d: subframe %d : o_ACK (%d %d), cqi (val %d, len %d)\n",
          eNB->Mod_id,UE_index, UE_id, subframe,eNB->ulsch[UE_index]->harq_processes[harq_pid]->o_ACK[0],
          eNB->ulsch[UE_index]->harq_processes[harq_pid]->o_ACK[1],
          ((HLC_subband_cqi_rank1_2A_5MHz *)PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->o)->cqi1,
          PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->O);
#endif

    eNB->ulsch[UE_index]->harq_processes[harq_pid]->Or1 = PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->O;
    eNB->ulsch[UE_index]->harq_processes[harq_pid]->Or2 = PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->O;

    eNB->ulsch[UE_index]->harq_processes[harq_pid]->uci_format = PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->uci_format;
    memcpy(eNB->ulsch[UE_index]->harq_processes[harq_pid]->o,PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->o,MAX_CQI_BYTES);
    memcpy(eNB->ulsch[UE_index]->harq_processes[harq_pid]->o_RI,PHY_vars_UE_g[UE_id][CC_id]->ulsch[0]->o_RI,2);

    eNB->ulsch[UE_index]->harq_processes[harq_pid]->cqi_crc_status = 1;

    return(1);
  } else {
    LOG_W(PHY,"[eNB %d] ulsch_decoding_emul abstraction failed for UE %d\n",eNB->Mod_id,UE_index);

    eNB->ulsch[UE_index]->harq_processes[harq_pid]->cqi_crc_status = 0;

    // retransmission
    return(1+eNB->ulsch[UE_index]->max_turbo_iterations);
  }

}
#endif
