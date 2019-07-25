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


#include <syscall.h>
#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "PHY/CODING/coding_extern.h"
#include "SCHED/sched_eNB.h"
#include "LAYER2/MAC/mac.h"
#include "RRC/LTE/rrc_extern.h"
#include "PHY_INTERFACE/phy_interface.h"

#include "common/utils/LOG/vcd_signal_dumper.h"
//#define DEBUG_ULSCH_DECODING
#include "targets/RT/USER/rt_wrapper.h"
#include "transport_proto.h"

extern WORKER_CONF_t get_thread_worker_conf(void);

void free_eNB_ulsch(LTE_eNB_ULSCH_t *ulsch) {
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
  }
}

LTE_eNB_ULSCH_t *new_eNB_ulsch(uint8_t max_turbo_iterations,uint8_t N_RB_UL, uint8_t abstraction_flag) {
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
        ulsch->harq_processes[i]->b = (uint8_t *)malloc16(MAX_ULSCH_PAYLOAD_BYTES/bw_scaling);

        if (ulsch->harq_processes[i]->b)
          memset(ulsch->harq_processes[i]->b,0,MAX_ULSCH_PAYLOAD_BYTES/bw_scaling);
        else
          exit_flag=3;

        if (abstraction_flag==0) {
          for (r=0; r<MAX_NUM_ULSCH_SEGMENTS/bw_scaling; r++) {
            ulsch->harq_processes[i]->c[r] = (uint8_t *)malloc16(((r==0)?8:0) + 3+768);

            if (ulsch->harq_processes[i]->c[r])
              memset(ulsch->harq_processes[i]->c[r],0,((r==0)?8:0) + 3+768);
            else
              exit_flag=2;

            ulsch->harq_processes[i]->d[r] = (short *)malloc16(((3*8*6144)+12+96)*sizeof(short));

            if (ulsch->harq_processes[i]->d[r])
              memset(ulsch->harq_processes[i]->d[r],0,((3*8*6144)+12+96)*sizeof(short));
            else
              exit_flag=2;
          }
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

void clean_eNb_ulsch(LTE_eNB_ULSCH_t *ulsch) {
  unsigned char i;

  //ulsch = (LTE_eNB_ULSCH_t *)malloc16(sizeof(LTE_eNB_ULSCH_t));
  if (ulsch) {
    ulsch->rnti = 0;
    ulsch->harq_mask = 0;

    for (i=0; i<8; i++) {
      if (ulsch->harq_processes[i]) {
        //    ulsch->harq_processes[i]->Ndi = 0;
        ulsch->harq_processes[i]->status = 0;
        //ulsch->harq_processes[i]->phich_active = 0; //this will be done later after transmission of PHICH
        ulsch->harq_processes[i]->phich_ACK = 0;
        ulsch->harq_processes[i]->round = 0;
        ulsch->harq_processes[i]->rar_alloc = 0;
        ulsch->harq_processes[i]->first_rb = 0;
        ulsch->harq_processes[i]->nb_rb = 0;
        ulsch->harq_processes[i]->TBS = 0;
        ulsch->harq_processes[i]->Or1 = 0;
        ulsch->harq_processes[i]->Or2 = 0;

        for ( int j = 0; j < 2; j++ ) {
          ulsch->harq_processes[i]->o_RI[j] = 0;
        }

        ulsch->harq_processes[i]->O_ACK = 0;
        ulsch->harq_processes[i]->srs_active = 0;
        ulsch->harq_processes[i]->rvidx = 0;
        ulsch->harq_processes[i]->Msc_initial = 0;
        ulsch->harq_processes[i]->Nsymb_initial = 0;
      }
    }

    ulsch->beta_offset_cqi_times8 = 0;
    ulsch->beta_offset_ri_times8 = 0;
    ulsch->beta_offset_harqack_times8 = 0;
    ulsch->Msg3_active = 0;
  }
}


uint8_t extract_cqi_crc(uint8_t *cqi,uint8_t CQI_LENGTH) {
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




int ulsch_decoding_data_2thread0(td_params *tdp) {
  PHY_VARS_eNB *eNB = tdp->eNB;
  int UE_id         = tdp->UE_id;
  int harq_pid      = tdp->harq_pid;
  int llr8_flag     = tdp->llr8_flag;
  unsigned int r,r_offset=0,Kr,Kr_bytes;
  uint8_t crc_type;
  int offset = 0;
  int ret = 1;
  int16_t dummy_w[MAX_NUM_ULSCH_SEGMENTS][3*(6144+64)];
  LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[UE_id];
  LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];
  int Q_m = ulsch_harq->Qm;
  int G = ulsch_harq->G;
  uint32_t E=0;
  uint32_t Gp,GpmodC,Nl=1;
  uint32_t C = ulsch_harq->C;
  decoder_if_t *tc;

  if (llr8_flag == 0)
    tc = decoder16;
  else
    tc = decoder8;

  // go through first half of segments to get r_offset
  for (r=0; r<(ulsch_harq->C/2); r++) {
    // Get Turbo interleaver parameters
    if (r<ulsch_harq->Cminus)
      Kr = ulsch_harq->Kminus;
    else
      Kr = ulsch_harq->Kplus;

    Kr_bytes = Kr>>3;
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
    memset(&dummy_w[r][0],0,3*(6144+64)*sizeof(short));
    ulsch_harq->RTC[r] = generate_dummy_w(4+(Kr_bytes*8),
                                          (uint8_t *)&dummy_w[r][0],
                                          (r==0) ? ulsch_harq->F : 0);
#ifdef DEBUG_ULSCH_DECODING
    printf("Rate Matching Segment %u (coded bits (G) %d,unpunctured/repeated bits %u, Q_m %d, nb_rb %d, Nl %d)...\n",
           r, G,
           Kr*3,
           Q_m,
           nb_rb,
           ulsch_harq->Nl);
#endif

    if (lte_rate_matching_turbo_rx(ulsch_harq->RTC[r],
                                   G,
                                   ulsch_harq->w[r],
                                   (uint8_t *) &dummy_w[r][0],
                                   ulsch_harq->e+r_offset,
                                   ulsch_harq->C,
                                   NSOFT,
                                   0,   //Uplink
                                   1,
                                   ulsch_harq->rvidx,
                                   (ulsch_harq->round==0)?1:0,  // clear
                                   ulsch_harq->Qm,
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
             NULL,
             ulsch_harq->c[r],
             NULL,
             Kr,
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
  PHY_VARS_eNB *eNB = ((td_params *)param)->eNB;
  L1_proc_t *proc  = &eNB->proc;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  thread_top_init("td_thread",1,200000,250000,500000);
  pthread_setname_np( pthread_self(),"td processing");
  LOG_I(PHY,"thread td created id=%ld\n", syscall(__NR_gettid));
  //wait_sync("td_thread");

  while (!oai_exit) {
    if (wait_on_condition(&proc->mutex_td,&proc->cond_td,&proc->instance_cnt_td,"td thread")<0) break;

    if(oai_exit) break;

    ((td_params *)param)->ret = ulsch_decoding_data_2thread0((td_params *)param);

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
  L1_proc_t *proc = &eNB->proc;
  unsigned int r,r_offset=0,Kr,Kr_bytes;
  uint8_t crc_type;
  int offset = 0;
  int ret = 1;
  int16_t dummy_w[MAX_NUM_ULSCH_SEGMENTS][3*(6144+64)];
  LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[UE_id];
  LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];
  int G = ulsch_harq->G;
  unsigned int E;
  int Cby2;
  decoder_if_t *tc;
  struct timespec wait;
  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  if (llr8_flag == 0)
    tc = decoder16;
  else
    tc = decoder8;

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
  } else {
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
    memset(&dummy_w[r][0],0,3*(6144+64)*sizeof(short));
    ulsch_harq->RTC[r] = generate_dummy_w(4+(Kr_bytes*8),
                                          (uint8_t *)&dummy_w[r][0],
                                          (r==0) ? ulsch_harq->F : 0);
#ifdef DEBUG_ULSCH_DECODING
    printf("Rate Matching Segment %u (coded bits (G) %d,unpunctured/repeated bits %u, Q_m %d, nb_rb %d, Nl %d)...\n",
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
                                   (uint8_t *) &dummy_w[r][0],
                                   ulsch_harq->e+r_offset,
                                   ulsch_harq->C,
                                   NSOFT,
                                   0,   //Uplink
                                   1,
                                   ulsch_harq->rvidx,
                                   (ulsch_harq->round==0)?1:0,  // clear
                                   ulsch_harq->Qm,
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
             NULL,
             ulsch_harq->c[r],
             NULL,
             Kr,
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
    //printf("/////////////////////////////////////////**************************loop for %d time in ulsch_decoding main\n",r);
  }

  // wait for worker to finish
  wait_on_busy_condition(&proc->mutex_td,&proc->cond_td,&proc->instance_cnt_td,"td thread");
  return( (ret>proc->tdp.ret) ? ret : proc->tdp.ret );
}

int ulsch_decoding_data(PHY_VARS_eNB *eNB,int UE_id,int harq_pid,int llr8_flag) {
  unsigned int r,r_offset=0,Kr,Kr_bytes;
  uint8_t crc_type;
  int offset = 0;
  int ret = 1;
  int16_t dummy_w[MAX_NUM_ULSCH_SEGMENTS][3*(6144+64)];
  LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[UE_id];
  LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];
  int G = ulsch_harq->G;
  unsigned int E;
  decoder_if_t *tc;

  if (llr8_flag == 0)
    tc = *decoder16;
  else
    tc = *decoder8;

  for (r=0; r<ulsch_harq->C; r++) {
    //    printf("before subblock deinterleaving c[%d] = %p\n",r,ulsch_harq->c[r]);
    // Get Turbo interleaver parameters
    if (r<ulsch_harq->Cminus)
      Kr = ulsch_harq->Kminus;
    else
      Kr = ulsch_harq->Kplus;

    Kr_bytes = Kr>>3;
    memset(&dummy_w[r][0],0,3*(6144+64)*sizeof(short));
    ulsch_harq->RTC[r] = generate_dummy_w(4+(Kr_bytes*8),
                                          (uint8_t *)&dummy_w[r][0],
                                          (r==0) ? ulsch_harq->F : 0);
#ifdef DEBUG_ULSCH_DECODING
    printf("Rate Matching Segment %u (coded bits (G) %d,unpunctured/repeated bits %u, Q_m %d, nb_rb %d, Nl %d)...\n",
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
                                   (uint8_t *) &dummy_w[r][0],
                                   ulsch_harq->e+r_offset,
                                   ulsch_harq->C,
                                   NSOFT,
                                   0,   //Uplink
                                   1,
                                   ulsch_harq->rvidx,
                                   (ulsch_harq->round==0)?1:0,  // clear
                                   ulsch_harq->Qm,
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
             NULL,
             ulsch_harq->c[r],
             NULL,
             Kr,
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

int ulsch_decoding_data_all(PHY_VARS_eNB *eNB,int UE_id,int harq_pid,int llr8_flag) {
  int ret = 0;
  /*if(get_thread_worker_conf() == WORKER_ENABLE)
  {
    ret = ulsch_decoding_data_2thread(eNB,UE_id,harq_pid,llr8_flag);
  }
  else*/
  {
    ret = ulsch_decoding_data(eNB,UE_id,harq_pid,llr8_flag);
  }
  return ret;
}

static inline unsigned int lte_gold_unscram(unsigned int *x1, unsigned int *x2, unsigned char reset) __attribute__((always_inline));
static inline unsigned int lte_gold_unscram(unsigned int *x1, unsigned int *x2, unsigned char reset) {
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

unsigned int  ulsch_decoding(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc,
                             uint8_t UE_id,
                             uint8_t control_only_flag,
                             uint8_t Nbundled,
                             uint8_t llr8_flag) {
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
  uint32_t x1, x2, s=0;
  int16_t ys,c;
  uint32_t wACK_idx;
  uint8_t dummy_w_cc[3*(MAX_CQI_BITS+8+32)];
  int16_t y[6*14*1200] __attribute__((aligned(32)));
  uint8_t ytag[14*1200];
  //  uint8_t ytag2[6*14*1200],*ytag2_ptr;
  int16_t cseq[6*14*1200] __attribute__((aligned(32)));
  int off;
  int frame = proc->frame_rx;
  int subframe = proc->subframe_rx;
  LTE_UL_eNB_HARQ_t *ulsch_harq;

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  LOG_D(PHY,"ue_type %d\n",ulsch->ue_type);
  if (ulsch->ue_type>0)     harq_pid = 0;
  else
#endif
    {
      harq_pid = subframe2harq_pid(frame_parms,proc->frame_rx,subframe);
    }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_ULSCH_DECODING0+harq_pid,1);

  // x1 is set in lte_gold_generic
  x2 = ((uint32_t)ulsch->rnti<<14) + ((uint32_t)subframe<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.3.1
  ulsch_harq = ulsch->harq_processes[harq_pid];
  AssertFatal(harq_pid!=255,
              "FATAL ERROR: illegal harq_pid, returning\n");
  AssertFatal(ulsch_harq->Nsymb_pusch != 0,
              "FATAL ERROR: harq_pid %d, Nsymb 0!\n",harq_pid);
  nb_rb = ulsch_harq->nb_rb;
  A = ulsch_harq->TBS;
  Q_m = ulsch_harq->Qm;
  G = nb_rb * (12 * Q_m) * ulsch_harq->Nsymb_pusch;
  //#ifdef DEBUG_ULSCH_DECODING
  LOG_D(PHY,"[PUSCH %d] Frame %d, Subframe %d: ulsch_decoding (Nid_cell %d, rnti %x, x2 %x): A %d, round %d, RV %d, O_r1 %d, O_RI %d, O_ACK %d, G %d, Q_m %d Nsymb_pusch %d nb_rb %d\n",
        harq_pid,
        proc->frame_rx,subframe,
        frame_parms->Nid_cell,ulsch->rnti,x2,
        A,
        ulsch_harq->round,
        ulsch_harq->rvidx,
        ulsch_harq->Or1,
        ulsch_harq->O_RI,
        ulsch_harq->O_ACK,
        G,
        ulsch_harq->Qm,
        ulsch_harq->Nsymb_pusch,
        nb_rb);
  //#endif
  //if (ulsch_harq->round == 0) { // delete for RB shortage pattern
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
  //}
  //  printf("after segmentation c[%d] = %p\n",0,ulsch_harq->c[0]);
  sumKr = 0;

  for (r=0; r<ulsch_harq->C; r++) {
    if (r<ulsch_harq->Cminus)
      Kr = ulsch_harq->Kminus;
    else
      Kr = ulsch_harq->Kplus;

    sumKr += Kr;
  }

  AssertFatal(sumKr>0,
              "[eNB] ulsch_decoding.c: FATAL sumKr is 0! (Nid_cell %d, rnti %x, x2 %x): harq_pid %d round %d, RV %d, O_RI %d, O_ACK %d, G %d, subframe %d\n",
              frame_parms->Nid_cell,ulsch->rnti,x2,
              harq_pid,
              ulsch_harq->round,
              ulsch_harq->rvidx,
              ulsch_harq->O_RI,
              ulsch_harq->O_ACK,
              G,
              subframe);
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
  printf("ulsch_decoding.c: Qprime_ACK %u, Msc_initial %d, Nsymb_initial %d, sumKr %u\n",
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
  printf("ulsch_decoding: G %u, Q_RI %u, Q_CQI %u (L %u, Or1 %d) O_ACK %d\n",G,Q_RI,Q_CQI,L,ulsch_harq->Or1,ulsch_harq->O_ACK);
#endif
  G = G - Q_RI - Q_CQI;
  ulsch_harq->G = G;
  AssertFatal((int)G > 0,
              "FATAL: ulsch_decoding.c G < 0 (%d) : Q_RI %d, Q_CQI %d\n",G,Q_RI,Q_CQI);
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
    ((__m128i *)cseq)[i2++] = ((__m128i *)unscrambling_lut)[(s&65535)<<1];
    ((__m128i *)cseq)[i2++] = ((__m128i *)unscrambling_lut)[1+((s&65535)<<1)];
    s>>=16;
    ((__m128i *)cseq)[i2++] = ((__m128i *)unscrambling_lut)[(s&65535)<<1];
    ((__m128i *)cseq)[i2++] = ((__m128i *)unscrambling_lut)[1+((s&65535)<<1)];
#else
    ((__m256i *)cseq)[i2++] = ((__m256i *)unscrambling_lut)[s&65535];
    ((__m256i *)cseq)[i2++] = ((__m256i *)unscrambling_lut)[(s>>16)&65535];
#endif
#elif defined(__arm__)
    ((int16x8_t *)cseq)[i2++] = ((int16x8_t *)unscrambling_lut)[(s&65535)<<1];
    ((int16x8_t *)cseq)[i2++] = ((int16x8_t *)unscrambling_lut)[1+((s&65535)<<1)];
    s>>=16;
    ((int16x8_t *)cseq)[i2++] = ((int16x8_t *)unscrambling_lut)[(s&65535)<<1];
    ((int16x8_t *)cseq)[i2++] = ((int16x8_t *)unscrambling_lut)[1+((s&65535)<<1)];
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
    printf("ulsch_decoding.c: ACK i %u, r %d, j %u, ColumnSet[j] %d\n",i,r,j,columnset[j]);
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
          *(__m64 *)&y[i2] = _mm_sign_pi16(*(__m64 *)&ulsch_llr[i],*(__m64 *)&cseq[i]);
          i+=4;
          i2+=(Cmux<<2);
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
    LOG_E(PHY,"ulsch_decoding: FATAL, ACK cannot be more than 2 bits yet O_ACK:%d SFN/SF:%04d%d UE_id:%d rnti:%x\n",ulsch_harq->O_ACK,proc->frame_rx,proc->subframe_rx,UE_id,ulsch->rnti);
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
    memset(ulsch_harq->o,0,(7+8+ulsch_harq->Or1) / 8);
    phy_viterbi_lte_sse2(ulsch_harq->o_d+96,ulsch_harq->o,8+ulsch_harq->Or1);

    if (extract_cqi_crc(ulsch_harq->o,ulsch_harq->Or1) == (crc8(ulsch_harq->o,ulsch_harq->Or1)>>24))
      ulsch_harq->cqi_crc_status = 1;

#ifdef DEBUG_ULSCH_DECODING
    printf("ulsch_decoding: Or1=%d\n",ulsch_harq->Or1);

    for (i=0; i<1+((8+ulsch_harq->Or1)/8); i++)
      printf("ulsch_decoding: O[%u] %d\n",i,ulsch_harq->o[i]);

    if (ulsch_harq->cqi_crc_status == 1)
      printf("RX CQI CRC OK (%x)\n",extract_cqi_crc(ulsch_harq->o,ulsch_harq->Or1));
    else
      printf("RX CQI CRC NOT OK (%x)\n",extract_cqi_crc(ulsch_harq->o,ulsch_harq->Or1));

#endif
  }

  LOG_D(PHY,"frame %d subframe %d O_ACK:%d o_ACK[]=%d:%d:%d:%d\n",frame,subframe,ulsch_harq->O_ACK,ulsch_harq->o_ACK[0],ulsch_harq->o_ACK[1],ulsch_harq->o_ACK[2],ulsch_harq->o_ACK[3]);
  // Do ULSCH Decoding for data portion
  ret = ulsch_decoding_data_all(eNB,UE_id,harq_pid,llr8_flag);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_ULSCH_DECODING0+harq_pid,0);
  return(ret);
}
