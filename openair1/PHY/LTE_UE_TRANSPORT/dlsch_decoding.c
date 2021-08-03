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

/*! \file PHY/LTE_TRANSPORT/dlsch_decoding.c
* \brief Top-level routines for decoding  Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/

//#include "defs.h"
#include "PHY/defs_UE.h"
#include "PHY/CODING/coding_extern.h"
#include "SCHED/sched_common_extern.h"
#include "SIMULATION/TOOLS/sim.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"
//#define DEBUG_DLSCH_DECODING
//#define UE_DEBUG_TRACE 1

void free_ue_dlsch(LTE_UE_DLSCH_t *dlsch) {
  int i,r;

  if (dlsch) {
    for (i=0; i<dlsch->Mdlharq; i++) {
      if (dlsch->harq_processes[i]) {
        if (dlsch->harq_processes[i]->b) {
          free16(dlsch->harq_processes[i]->b,MAX_DLSCH_PAYLOAD_BYTES);
          dlsch->harq_processes[i]->b = NULL;
        }

        for (r=0; r<MAX_NUM_DLSCH_SEGMENTS; r++) {
          free16(dlsch->harq_processes[i]->c[r],((r==0)?8:0) + 3+768);
          dlsch->harq_processes[i]->c[r] = NULL;
        }

        for (r=0; r<MAX_NUM_DLSCH_SEGMENTS; r++)
          if (dlsch->harq_processes[i]->d[r]) {
            free16(dlsch->harq_processes[i]->d[r],((3*8*6144)+12+96)*sizeof(short));
            dlsch->harq_processes[i]->d[r] = NULL;
          }

        free16(dlsch->harq_processes[i],sizeof(LTE_DL_UE_HARQ_t));
        dlsch->harq_processes[i] = NULL;
      }
    }

    free16(dlsch,sizeof(LTE_UE_DLSCH_t));
  }
}

LTE_UE_DLSCH_t *new_ue_dlsch(uint8_t Kmimo,uint8_t Mdlharq,uint32_t Nsoft,uint8_t max_turbo_iterations,uint8_t N_RB_DL, uint8_t abstraction_flag) {
  LTE_UE_DLSCH_t *dlsch;
  uint8_t exit_flag = 0,i,r;
  unsigned char bw_scaling =1;

  switch (N_RB_DL) {
    case 6:
      bw_scaling =4;
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

  dlsch = (LTE_UE_DLSCH_t *)malloc16(sizeof(LTE_UE_DLSCH_t));

  if (dlsch) {
    memset(dlsch,0,sizeof(LTE_UE_DLSCH_t));
    dlsch->Kmimo = Kmimo;
    dlsch->Mdlharq = Mdlharq;
    dlsch->Nsoft = Nsoft;
    dlsch->max_turbo_iterations = max_turbo_iterations;

    for (i=0; i<Mdlharq; i++) {
      //      printf("new_ue_dlsch: Harq process %d\n",i);
      dlsch->harq_processes[i] = (LTE_DL_UE_HARQ_t *)malloc16(sizeof(LTE_DL_UE_HARQ_t));

      if (dlsch->harq_processes[i]) {
        memset(dlsch->harq_processes[i],0,sizeof(LTE_DL_UE_HARQ_t));
        dlsch->harq_processes[i]->first_tx=1;
        dlsch->harq_processes[i]->b = (uint8_t *)malloc16(MAX_DLSCH_PAYLOAD_BYTES/bw_scaling);

        if (dlsch->harq_processes[i]->b)
          memset(dlsch->harq_processes[i]->b,0,MAX_DLSCH_PAYLOAD_BYTES/bw_scaling);
        else
          exit_flag=3;

        if (abstraction_flag == 0) {
          for (r=0; r<MAX_NUM_DLSCH_SEGMENTS/bw_scaling; r++) {
            dlsch->harq_processes[i]->c[r] = (uint8_t *)malloc16(((r==0)?8:0) + 3+ 768);

            if (dlsch->harq_processes[i]->c[r])
              memset(dlsch->harq_processes[i]->c[r],0,((r==0)?8:0) + 3+ 768);
            else
              exit_flag=2;

            dlsch->harq_processes[i]->d[r] = (short *)malloc16(((3*8*6144)+12+96)*sizeof(short));

            if (dlsch->harq_processes[i]->d[r])
              memset(dlsch->harq_processes[i]->d[r],0,((3*8*6144)+12+96)*sizeof(short));
            else
              exit_flag=2;
          }
        }
      } else {
        exit_flag=1;
      }
    }

    if (exit_flag==0)
      return(dlsch);
  }

  printf("new_ue_dlsch with size %zu: exit_flag = %u\n",sizeof(LTE_DL_UE_HARQ_t), exit_flag);
  free_ue_dlsch(dlsch);
  return(NULL);
}

uint32_t  dlsch_decoding(PHY_VARS_UE *phy_vars_ue,
                         short *dlsch_llr,
                         LTE_DL_FRAME_PARMS *frame_parms,
                         LTE_UE_DLSCH_t *dlsch,
                         LTE_DL_UE_HARQ_t *harq_process,
                         uint32_t frame,
                         uint8_t subframe,
                         uint8_t harq_pid,
                         uint8_t is_crnti,
                         uint8_t llr8_flag) {
#if UE_TIMING_TRACE
  time_stats_t *dlsch_rate_unmatching_stats=&phy_vars_ue->dlsch_rate_unmatching_stats;
  time_stats_t *dlsch_turbo_decoding_stats=&phy_vars_ue->dlsch_turbo_decoding_stats;
  time_stats_t *dlsch_deinterleaving_stats=&phy_vars_ue->dlsch_deinterleaving_stats;
#endif
  uint32_t A,E;
  uint32_t G;
  uint32_t ret,offset;
  //  uint8_t dummy_channel_output[(3*8*block_length)+12];
  short dummy_w[MAX_NUM_DLSCH_SEGMENTS][3*(6144+64)];
  uint32_t r,r_offset=0,Kr,Kr_bytes,err_flag=0;
  uint8_t crc_type;
#ifdef DEBUG_DLSCH_DECODING
  uint16_t i;
#endif
  //#ifdef __AVX2__
#if 0
  int Kr_last,skipped_last=0;
  uint8_t (*tc_2cw)(int16_t *y,
                    int16_t *y2,
                    uint8_t *,
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
#endif
  decoder_if_t *tc;

  if (!dlsch_llr) {
    printf("dlsch_decoding.c: NULL dlsch_llr pointer\n");
    return(dlsch->max_turbo_iterations);
  }

  if (!harq_process) {
    printf("dlsch_decoding.c: NULL harq_process pointer\n");
    return(dlsch->max_turbo_iterations);
  }

  if (!frame_parms) {
    printf("dlsch_decoding.c: NULL frame_parms pointer\n");
    return(dlsch->max_turbo_iterations);
  }

  if (subframe>9) {
    printf("dlsch_decoding.c: Illegal subframe index %d\n",subframe);
    return(dlsch->max_turbo_iterations);
  }

  if (dlsch->harq_ack[subframe].ack != 2) {
    LOG_D(PHY, "[UE %d] DLSCH @ SF%d : ACK bit is %d instead of DTX even before PDSCH is decoded!\n",
          phy_vars_ue->Mod_id, subframe, dlsch->harq_ack[subframe].ack);
  }

  if (llr8_flag == 0) {
    tc = decoder16;
  } else {
    AssertFatal (harq_process->TBS >= 256 , "Mismatch flag nbRB=%d TBS=%d mcs=%d Qm=%d RIV=%d round=%d \n",
                 harq_process->nb_rb, harq_process->TBS,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round);
    tc = decoder8;
  }

  //  nb_rb = dlsch->nb_rb;
  /*
  if (nb_rb > frame_parms->N_RB_DL) {
    printf("dlsch_decoding.c: Illegal nb_rb %d\n",nb_rb);
    return(max_turbo_iterations);
    }*/
  /*harq_pid = dlsch->current_harq_pid[phy_vars_ue->current_thread_id[subframe]];
  if (harq_pid >= 8) {
    printf("dlsch_decoding.c: Illegal harq_pid %d\n",harq_pid);
    return(max_turbo_iterations);
  }
  */
  harq_process->trials[harq_process->round]++;
  A = harq_process->TBS; //2072 for QPSK 1/3
  ret = dlsch->max_turbo_iterations;
  G = harq_process->G;
  //get_G(frame_parms,nb_rb,dlsch->rb_alloc,mod_order,num_pdcch_symbols,phy_vars_ue->frame,subframe);

  //  printf("DLSCH Decoding, harq_pid %d Ndi %d\n",harq_pid,harq_process->Ndi);

  if (harq_process->round == 0) {
    // This is a new packet, so compute quantities regarding segmentation
    harq_process->B = A+24;
    lte_segmentation(NULL,
                     NULL,
                     harq_process->B,
                     &harq_process->C,
                     &harq_process->Cplus,
                     &harq_process->Cminus,
                     &harq_process->Kplus,
                     &harq_process->Kminus,
                     &harq_process->F);
    //  CLEAR LLR's HERE for first packet in process
  }

  /*
  else {
    printf("dlsch_decoding.c: Ndi>0 not checked yet!!\n");
    return(max_turbo_iterations);
  }
  */
  err_flag = 0;
  r_offset = 0;
  unsigned char bw_scaling =1;

  switch (frame_parms->N_RB_DL) {
    case 6:
      bw_scaling =4;
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

  if (harq_process->C > MAX_NUM_DLSCH_SEGMENTS/bw_scaling) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,MAX_NUM_DLSCH_SEGMENTS/bw_scaling);
    return((1+dlsch->max_turbo_iterations));
  }

#ifdef DEBUG_DLSCH_DECODING
  printf("Segmentation: C %d, Cminus %d, Kminus %d, Kplus %d\n",harq_process->C,harq_process->Cminus,harq_process->Kminus,harq_process->Kplus);
#endif
  opp_enabled=1;

  for (r=0; r<harq_process->C; r++) {
    // Get Turbo interleaver parameters
    if (r<harq_process->Cminus)
      Kr = harq_process->Kminus;
    else
      Kr = harq_process->Kplus;

    Kr_bytes = Kr>>3;
#if UE_TIMING_TRACE
    start_meas(dlsch_rate_unmatching_stats);
#endif
    memset(&dummy_w[r][0],0,3*(6144+64)*sizeof(short));
    harq_process->RTC[r] = generate_dummy_w(4+(Kr_bytes*8),
                                            (uint8_t *) &dummy_w[r][0],
                                            (r==0) ? harq_process->F : 0);
#ifdef DEBUG_DLSCH_DECODING
    LOG_D(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
          harq_pid,r, G,
          Kr*3,
          harq_process->TBS,
          harq_process->Qm,
          harq_process->nb_rb,
          harq_process->Nl,
          harq_process->rvidx,
          harq_process->round);
#endif
#ifdef DEBUG_DLSCH_DECODING
    printf(" in decoding dlsch->harq_processes[harq_pid]->rvidx = %d\n", dlsch->harq_processes[harq_pid]->rvidx);
#endif

    if (lte_rate_matching_turbo_rx(harq_process->RTC[r],
                                   G,
                                   harq_process->w[r],
                                   (uint8_t *)&dummy_w[r][0],
                                   dlsch_llr+r_offset,
                                   harq_process->C,
                                   dlsch->Nsoft,
                                   dlsch->Mdlharq,
                                   dlsch->Kmimo,
                                   harq_process->rvidx,
                                   (harq_process->round==0)?1:0,
                                   harq_process->Qm,
                                   harq_process->Nl,
                                   r,
                                   &E)==-1) {
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
      LOG_E(PHY,"dlsch_decoding.c: Problem in rate_matching\n");
      return(dlsch->max_turbo_iterations);
    } else {
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
    }

    r_offset += E;
    /*
    printf("Subblock deinterleaving, d %p w %p\n",
     harq_process->d[r],
     harq_process->w);
    */
#if UE_TIMING_TRACE
    start_meas(dlsch_deinterleaving_stats);
#endif
    sub_block_deinterleaving_turbo(4+Kr,
                                   &harq_process->d[r][96],
                                   harq_process->w[r]);
#if UE_TIMING_TRACE
    stop_meas(dlsch_deinterleaving_stats);
#endif
#ifdef DEBUG_DLSCH_DECODING
    /*
    if (r==0) {
              LOG_M("decoder_llr.m","decllr",dlsch_llr,G,1,0);
              LOG_M("decoder_in.m","dec",&harq_process->d[0][96],(3*8*Kr_bytes)+12,1,0);
    }

    printf("decoder input(segment %d) :",r);
    int i; for (i=0;i<(3*8*Kr_bytes)+12;i++)
      printf("%d : %d\n",i,harq_process->d[r][96+i]);
      printf("\n");*/
#endif
    //    printf("Clearing c, %p\n",harq_process->c[r]);
    memset(harq_process->c[r],0,Kr_bytes);

    //    printf("done\n");
    if (harq_process->C == 1)
      crc_type = CRC24_A;
    else
      crc_type = CRC24_B;

    /*
    printf("decoder input(segment %d)\n",r);
    for (i=0;i<(3*8*Kr_bytes)+12;i++)
      if ((harq_process->d[r][96+i]>7) ||
    (harq_process->d[r][96+i] < -8))
    printf("%d : %d\n",i,harq_process->d[r][96+i]);
    printf("\n");
    */
    //#ifndef __AVX2__
#if 1

    if (err_flag == 0) {
      /*
              LOG_I(PHY, "turbo algo Kr=%d cb_cnt=%d C=%d nbRB=%d crc_type %d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d maxIter %d\n",
                                  Kr,r,harq_process->C,harq_process->nb_rb,crc_type,A,harq_process->TBS,
                                  harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round,dlsch->max_turbo_iterations);
      */
      if (llr8_flag) {
        AssertFatal (Kr >= 256, "turbo algo issue Kr=%d cb_cnt=%d C=%d nbRB=%d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d\n",
                     Kr,r,harq_process->C,harq_process->nb_rb,A,harq_process->TBS,harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round);
      }

#if UE_TIMING_TRACE
      start_meas(dlsch_turbo_decoding_stats);
#endif
      LOG_D(PHY,"AbsSubframe %d.%d Start turbo segment %d/%d \n",frame%1024,subframe,r,harq_process->C-1);
      ret = tc
            (&harq_process->d[r][96],
             NULL,
             harq_process->c[r],
             NULL,
             Kr,
             dlsch->max_turbo_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
    }

#else

    if ((harq_process->C == 1) ||
        ((r==harq_process->C-1) && (skipped_last==0))) { // last segment with odd number of segments
#if UE_TIMING_TRACE
      start_meas(dlsch_turbo_decoding_stats);
#endif
      ret = tc
            (&harq_process->d[r][96],
             harq_process->c[r],
             Kr,
             dlsch->max_turbo_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
      //      printf("single decode, exit\n");
      //      exit(-1);
    } else {
      // we can merge code segments
      if ((skipped_last == 0) && (r<harq_process->C-1)) {
        skipped_last = 1;
        Kr_last = Kr;
      } else {
        skipped_last=0;

        if (Kr_last == Kr) { // decode 2 code segments with AVX2 version
#ifdef DEBUG_DLSCH_DECODING
          printf("single decoding segment %d (%p)\n",r-1,&harq_process->d[r-1][96]);
#endif
#if UE_TIMING_TRACE
          start_meas(dlsch_turbo_decoding_stats);
#endif
#ifdef DEBUG_DLSCH_DECODING
          printf("double decoding segments %d,%d (%p,%p)\n",r-1,r,&harq_process->d[r-1][96],&harq_process->d[r][96]);
#endif
          ret = tc_2cw
                (&harq_process->d[r-1][96],
                 &harq_process->d[r][96],
                 harq_process->c[r-1],
                 harq_process->c[r],
                 Kr,
                 dlsch->max_turbo_iterations,
                 crc_type,
                 (r==0) ? harq_process->F : 0,
                 &phy_vars_ue->dlsch_tc_init_stats,
                 &phy_vars_ue->dlsch_tc_alpha_stats,
                 &phy_vars_ue->dlsch_tc_beta_stats,
                 &phy_vars_ue->dlsch_tc_gamma_stats,
                 &phy_vars_ue->dlsch_tc_ext_stats,
                 &phy_vars_ue->dlsch_tc_intl1_stats,
                 &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
          /*
          ret = tc
                  (&harq_process->d[r-1][96],
                   harq_process->c[r-1],
                   Kr_last,
                   dlsch->max_turbo_iterations,
                   crc_type,
                   (r==0) ? harq_process->F : 0,
                   &phy_vars_ue->dlsch_tc_init_stats,
                   &phy_vars_ue->dlsch_tc_alpha_stats,
                   &phy_vars_ue->dlsch_tc_beta_stats,
                   &phy_vars_ue->dlsch_tc_gamma_stats,
                   &phy_vars_ue->dlsch_tc_ext_stats,
                   &phy_vars_ue->dlsch_tc_intl1_stats,
                   &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);

             exit(-1);*/
#if UE_TIMING_TRACE
          stop_meas(dlsch_turbo_decoding_stats);
#endif
        } else { // Kr_last != Kr
#if UE_TIMING_TRACE
          start_meas(dlsch_turbo_decoding_stats);
#endif
          ret = tc
                (&harq_process->d[r-1][96],
                 harq_process->c[r-1],
                 Kr_last,
                 dlsch->max_turbo_iterations,
                 crc_type,
                 (r==0) ? harq_process->F : 0,
                 &phy_vars_ue->dlsch_tc_init_stats,
                 &phy_vars_ue->dlsch_tc_alpha_stats,
                 &phy_vars_ue->dlsch_tc_beta_stats,
                 &phy_vars_ue->dlsch_tc_gamma_stats,
                 &phy_vars_ue->dlsch_tc_ext_stats,
                 &phy_vars_ue->dlsch_tc_intl1_stats,
                 &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
#if UE_TIMING_TRACE
          stop_meas(dlsch_turbo_decoding_stats);
          start_meas(dlsch_turbo_decoding_stats);
#endif
          ret = tc
                (&harq_process->d[r][96],
                 harq_process->c[r],
                 Kr,
                 dlsch->max_turbo_iterations,
                 crc_type,
                 (r==0) ? harq_process->F : 0,
                 &phy_vars_ue->dlsch_tc_init_stats,
                 &phy_vars_ue->dlsch_tc_alpha_stats,
                 &phy_vars_ue->dlsch_tc_beta_stats,
                 &phy_vars_ue->dlsch_tc_gamma_stats,
                 &phy_vars_ue->dlsch_tc_ext_stats,
                 &phy_vars_ue->dlsch_tc_intl1_stats,
                 &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
#if UE_TIMING_TRACE
          stop_meas(dlsch_turbo_decoding_stats);
          /*printf("Segmentation: C %d r %d, dlsch_rate_unmatching_stats %5.3f dlsch_deinterleaving_stats %5.3f  dlsch_turbo_decoding_stats %5.3f \n",
                    harq_process->C,
                    r,
                    dlsch_rate_unmatching_stats->p_time/(cpuf*1000.0),
                    dlsch_deinterleaving_stats->p_time/(cpuf*1000.0),
                    dlsch_turbo_decoding_stats->p_time/(cpuf*1000.0));*/
#endif
        }
      }
    }

#endif

    if ((err_flag == 0) && (ret>=(1+dlsch->max_turbo_iterations))) {// a Code segment is in error so break;
      LOG_D(PHY,"AbsSubframe %d.%d CRC failed, segment %d/%d \n",frame%1024,subframe,r,harq_process->C-1);
      err_flag = 1;
    }
  }

  int32_t frame_rx_prev = frame;
  int32_t subframe_rx_prev = subframe - 1;

  if (subframe_rx_prev < 0) {
    frame_rx_prev--;
    subframe_rx_prev += 10;
  }

  frame_rx_prev = frame_rx_prev%1024;

  if (err_flag == 1) {
#if UE_DEBUG_TRACE
    LOG_D(PHY,"[UE %d] DLSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d) Kr %d r %d harq_process->round %d\n",
          phy_vars_ue->Mod_id, frame, subframe, harq_pid,harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs,Kr,r,harq_process->round);
#endif
    dlsch->harq_ack[subframe].ack = 0;
    dlsch->harq_ack[subframe].harq_id = harq_pid;
    dlsch->harq_ack[subframe].send_harq_status = 1;
    harq_process->errors[harq_process->round]++;
    harq_process->round++;

    //    printf("Rate: [UE %d] DLSCH: Setting NACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);
    if (harq_process->round >= dlsch->Mdlharq) {
      harq_process->status = SCH_IDLE;
      harq_process->round  = 0;
    }

    if(is_crnti) {
      LOG_D(PHY,"[UE %d] DLSCH: Setting NACK for subframe %d (pid %d, pid status %d, round %d/Max %d, TBS %d)\n",
            phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->status,harq_process->round,dlsch->Mdlharq,harq_process->TBS);
    }

    return((1+dlsch->max_turbo_iterations));
  } else {
#if UE_DEBUG_TRACE
    LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for subframe %d TBS %d mcs %d nb_rb %d\n",
          phy_vars_ue->Mod_id,subframe,harq_process->TBS,harq_process->mcs,harq_process->nb_rb);
#endif
    harq_process->status = SCH_IDLE;
    harq_process->round  = 0;
    dlsch->harq_ack[subframe].ack = 1;
    dlsch->harq_ack[subframe].harq_id = harq_pid;
    dlsch->harq_ack[subframe].send_harq_status = 1;
    //LOG_I(PHY,"[UE %d] DLSCH: Setting ACK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d)\n",
    //  phy_vars_ue->Mod_id, frame, subframe, harq_pid, harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs);

    if(is_crnti) {
      LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for subframe %d (pid %d, round %d, TBS %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round,harq_process->TBS);
    }

    //LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);
  }

  // Reassembly of Transport block here
  offset = 0;

  /*
  printf("harq_pid %d\n",harq_pid);
  printf("F %d, Fbytes %d\n",harq_process->F,harq_process->F>>3);
  printf("C %d\n",harq_process->C);
  */
  for (r=0; r<harq_process->C; r++) {
    if (r<harq_process->Cminus)
      Kr = harq_process->Kminus;
    else
      Kr = harq_process->Kplus;

    Kr_bytes = Kr>>3;

    //    printf("Segment %d : Kr= %d bytes\n",r,Kr_bytes);
    if (r==0) {
      memcpy(harq_process->b,
             &harq_process->c[0][(harq_process->F>>3)],
             Kr_bytes - (harq_process->F>>3)- ((harq_process->C>1)?3:0));
      offset = Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0);
      //            printf("copied %d bytes to b sequence (harq_pid %d)\n",
      //          Kr_bytes - (harq_process->F>>3),harq_pid);
      //          printf("b[0] = %x,c[%d] = %x\n",
      //      harq_process->b[0],
      //      harq_process->F>>3,
      //      harq_process->c[0][(harq_process->F>>3)]);
    } else {
      memcpy(harq_process->b+offset,
             harq_process->c[r],
             Kr_bytes- ((harq_process->C>1)?3:0));
      offset += (Kr_bytes - ((harq_process->C>1)?3:0));
    }
  }

  dlsch->last_iteration_cnt = ret;
  return(ret);
}

int dlsch_encoding_SIC(PHY_VARS_UE *ue,
                       unsigned char *a,
                       uint8_t num_pdcch_symbols,
                       LTE_eNB_DLSCH_t *dlsch,
                       int frame,
                       uint8_t subframe,
                       time_stats_t *rm_stats,
                       time_stats_t *te_stats,
                       time_stats_t *i_stats) {
  unsigned int G;
  unsigned int crc=1;
  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  unsigned char harq_pid = ue->dlsch[subframe&2][0][0]->rnti;
  unsigned short nb_rb = dlsch->harq_processes[harq_pid]->nb_rb;
  unsigned int A;
  unsigned char mod_order;
  unsigned int Kr=0,Kr_bytes,r,r_offset=0;
  //  unsigned short m=dlsch->harq_processes[harq_pid]->mcs;
  uint8_t beamforming_mode=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ENCODING, VCD_FUNCTION_IN);
  A = dlsch->harq_processes[harq_pid]->TBS; //6228
  // printf("Encoder: A: %d\n",A);
  mod_order = dlsch->harq_processes[harq_pid]->Qm;

  if(dlsch->harq_processes[harq_pid]->mimo_mode == TM7)
    beamforming_mode = 7;
  else if(dlsch->harq_processes[harq_pid]->mimo_mode == TM8)
    beamforming_mode = 8;
  else if(dlsch->harq_processes[harq_pid]->mimo_mode == TM9_10)
    beamforming_mode = 9;

  G = get_G(frame_parms,nb_rb,dlsch->harq_processes[harq_pid]->rb_alloc,mod_order,dlsch->harq_processes[harq_pid]->Nl,num_pdcch_symbols,frame,subframe,beamforming_mode);

  //  if (dlsch->harq_processes[harq_pid]->Ndi == 1) {  // this is a new packet
  if (dlsch->harq_processes[harq_pid]->round == 0) {  // this is a new packet
#ifdef DEBUG_DLSCH_CODING
    printf("SIC encoding thinks this is a new packet \n");
#endif
    /*
    int i;
    printf("dlsch (tx): \n");
    for (i=0;i<(A>>3);i++)
      printf("%02x.",a[i]);
    printf("\n");
    */
    // Add 24-bit crc (polynomial A) to payload
    crc = crc24a(a,
                 A)>>8;
    a[A>>3] = ((uint8_t *)&crc)[2];
    a[1+(A>>3)] = ((uint8_t *)&crc)[1];
    a[2+(A>>3)] = ((uint8_t *)&crc)[0];
    //    printf("CRC %x (A %d)\n",crc,A);
    dlsch->harq_processes[harq_pid]->B = A+24;
    //    dlsch->harq_processes[harq_pid]->b = a;
    memcpy(dlsch->harq_processes[harq_pid]->b,a,(A/8)+4);

    if (lte_segmentation(dlsch->harq_processes[harq_pid]->b,
                         dlsch->harq_processes[harq_pid]->c,
                         dlsch->harq_processes[harq_pid]->B,
                         &dlsch->harq_processes[harq_pid]->C,
                         &dlsch->harq_processes[harq_pid]->Cplus,
                         &dlsch->harq_processes[harq_pid]->Cminus,
                         &dlsch->harq_processes[harq_pid]->Kplus,
                         &dlsch->harq_processes[harq_pid]->Kminus,
                         &dlsch->harq_processes[harq_pid]->F)<0)
      return(-1);

    for (r=0; r<dlsch->harq_processes[harq_pid]->C; r++) {
      if (r<dlsch->harq_processes[harq_pid]->Cminus)
        Kr = dlsch->harq_processes[harq_pid]->Kminus;
      else
        Kr = dlsch->harq_processes[harq_pid]->Kplus;

      Kr_bytes = Kr>>3;
#ifdef DEBUG_DLSCH_CODING
      printf("Generating Code Segment %u (%u bits)\n",r,Kr);
      // generate codewords
      printf("bits_per_codeword (Kr)= %u, A %u\n",Kr,A);
      printf("N_RB = %d\n",nb_rb);
      printf("Ncp %d\n",frame_parms->Ncp);
      printf("mod_order %d\n",mod_order);
#endif
      start_meas(te_stats);
      encoder(dlsch->harq_processes[harq_pid]->c[r],
              Kr>>3,
              &dlsch->harq_processes[harq_pid]->d[r][96],
              (r==0) ? dlsch->harq_processes[harq_pid]->F : 0
             );
      stop_meas(te_stats);
#ifdef DEBUG_DLSCH_CODING

      if (r==0)
        LOG_M("enc_output0.m","enc0",&dlsch->harq_processes[harq_pid]->d[r][96],(3*8*Kr_bytes)+12,1,4);

#endif
      start_meas(i_stats);
      dlsch->harq_processes[harq_pid]->RTC[r] =
        sub_block_interleaving_turbo(4+(Kr_bytes*8),
                                     &dlsch->harq_processes[harq_pid]->d[r][96],
                                     dlsch->harq_processes[harq_pid]->w[r]);
      stop_meas(i_stats);
    }
  }

  // Fill in the "e"-sequence from 36-212, V8.6 2009-03, p. 16-17 (for each "e") and concatenate the
  // outputs for each code segment, see Section 5.1.5 p.20

  for (r=0; r<dlsch->harq_processes[harq_pid]->C; r++) {
#ifdef DEBUG_DLSCH_CODING
    // printf("Rate Matching, Code segment %u (coded bits (G) %u,unpunctured/repeated bits per code segment %u,mod_order %d, nb_rb %d)...\n",
    //        r,
    //        G,
    //        Kr*3,
    //        mod_order,nb_rb);
#endif
    start_meas(rm_stats);
#ifdef DEBUG_DLSCH_CODING
    printf("rvidx in SIC encoding = %d\n", dlsch->harq_processes[harq_pid]->rvidx);
#endif
    r_offset += lte_rate_matching_turbo(dlsch->harq_processes[harq_pid]->RTC[r],
                                        G,  //G
                                        dlsch->harq_processes[harq_pid]->w[r],
                                        dlsch->harq_processes[harq_pid]->eDL+r_offset,
                                        dlsch->harq_processes[harq_pid]->C, // C
                                        dlsch->Nsoft,                    // Nsoft,
                                        dlsch->Mdlharq,
                                        dlsch->Kmimo,
                                        dlsch->harq_processes[harq_pid]->rvidx,
                                        dlsch->harq_processes[harq_pid]->Qm,
                                        dlsch->harq_processes[harq_pid]->Nl,
                                        r,
                                        nb_rb);
    //                                        m);                       // r
    stop_meas(rm_stats);
#ifdef DEBUG_DLSCH_CODING

    if (r==dlsch->harq_processes[harq_pid]->C-1)
      LOG_M("enc_output.m","enc",dlsch->harq_processes[harq_pid]->e,r_offset,1,4);

#endif
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ENCODING, VCD_FUNCTION_OUT);
  return(0);
}
