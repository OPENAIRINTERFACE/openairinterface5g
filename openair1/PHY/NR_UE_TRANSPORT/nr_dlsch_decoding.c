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

/*! \file PHY/NR_UE_TRANSPORT/nr_dlsch_decoding.c
* \brief Top-level routines for decoding  Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/

#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY/defs_nr_UE.h"
#include "SCHED_NR_UE/harq_nr.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "SCHED_NR_UE/defs.h"
#include "SIMULATION/TOOLS/sim.h"
#include "executables/nr-uesoftmodem.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "common/utils/nr/nr_common.h"

//#define ENABLE_PHY_PAYLOAD_DEBUG 1

//#define OAI_LDPC_MAX_NUM_LLR 27000//26112 // NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX

static uint64_t nb_total_decod =0;
static uint64_t nb_error_decod =0;

notifiedFIFO_t freeBlocks_dl;
notifiedFIFO_elt_t *msgToPush_dl;
int nbDlProcessing =0;


static  tpool_t pool_dl;

//extern double cpuf;
void init_dlsch_tpool(uint8_t num_dlsch_threads) {
  if( num_dlsch_threads==0)
    return;

  char *params=calloc(1,(num_dlsch_threads*3)+1);

  for (int i=0; i<num_dlsch_threads; i++) {
    memcpy(params+(i*3),"-1,",3);
  }

  initNamedTpool(params, &pool_dl, false,"dlsch");
  free(params);
}


void free_nr_ue_dlsch(NR_UE_DLSCH_t **dlschptr,uint8_t N_RB_DL) {
  int i,r;
  uint16_t a_segments = MAX_NUM_NR_DLSCH_SEGMENTS;  //number of segments to be allocated
  NR_UE_DLSCH_t *dlsch=*dlschptr;

  if (dlsch) {
    if (N_RB_DL != 273) {
      a_segments = a_segments*N_RB_DL;
      a_segments = a_segments/273 +1;
    }

    for (i=0; i<dlsch->Mdlharq; i++) {
      if (dlsch->harq_processes[i]) {
        if (dlsch->harq_processes[i]->b) {
          free16(dlsch->harq_processes[i]->b,a_segments*1056);
          dlsch->harq_processes[i]->b = NULL;
        }

        for (r=0; r<a_segments; r++) {
          free16(dlsch->harq_processes[i]->c[r],1056);
          dlsch->harq_processes[i]->c[r] = NULL;
        }

        for (r=0; r<a_segments; r++)
          if (dlsch->harq_processes[i]->d[r]) {
            free16(dlsch->harq_processes[i]->d[r],(5*8448)*sizeof(short));
            dlsch->harq_processes[i]->d[r] = NULL;
          }

        for (r=0; r<a_segments; r++)
          if (dlsch->harq_processes[i]->w[r]) {
            free16(dlsch->harq_processes[i]->w[r],(5*8448)*sizeof(short));
            dlsch->harq_processes[i]->w[r] = NULL;
          }

        for (r=0; r<a_segments; r++) {
          if (dlsch->harq_processes[i]->p_nrLDPC_procBuf[r]) {
            nrLDPC_free_mem(dlsch->harq_processes[i]->p_nrLDPC_procBuf[r]);
            dlsch->harq_processes[i]->p_nrLDPC_procBuf[r] = NULL;
          }
        }

        free16(dlsch->harq_processes[i],sizeof(NR_DL_UE_HARQ_t));
        dlsch->harq_processes[i] = NULL;
      }
    }

    free16(dlsch,sizeof(NR_UE_DLSCH_t));
    dlsch = NULL;
  }
}

NR_UE_DLSCH_t *new_nr_ue_dlsch(uint8_t Kmimo,uint8_t Mdlharq,uint32_t Nsoft,uint8_t max_ldpc_iterations,uint16_t N_RB_DL, uint8_t abstraction_flag) {
  NR_UE_DLSCH_t *dlsch;
  uint8_t exit_flag = 0,i,r;
  uint16_t a_segments = MAX_NUM_NR_DLSCH_SEGMENTS;  //number of segments to be allocated

  if (N_RB_DL != 273) {
    a_segments = a_segments*N_RB_DL;
    a_segments = (a_segments/273)+1;
  }

  uint16_t dlsch_bytes = a_segments*1056;  // allocated bytes per segment
  dlsch = (NR_UE_DLSCH_t *)malloc16(sizeof(NR_UE_DLSCH_t));

  if (dlsch) {
    memset(dlsch,0,sizeof(NR_UE_DLSCH_t));
    dlsch->Kmimo = Kmimo;
    dlsch->Mdlharq = Mdlharq;
    dlsch->number_harq_processes_for_pdsch = Mdlharq;
    dlsch->Nsoft = Nsoft;
    dlsch->Mlimit = 4;
    dlsch->max_ldpc_iterations = max_ldpc_iterations;

    for (i=0; i<Mdlharq; i++) {
      dlsch->harq_processes[i] = (NR_DL_UE_HARQ_t *)malloc16(sizeof(NR_DL_UE_HARQ_t));

      if (dlsch->harq_processes[i]) {
        memset(dlsch->harq_processes[i],0,sizeof(NR_DL_UE_HARQ_t));
        init_downlink_harq_status(dlsch->harq_processes[i]);
        dlsch->harq_processes[i]->first_rx=1;
        dlsch->harq_processes[i]->b = (uint8_t *)malloc16(dlsch_bytes);

        if (dlsch->harq_processes[i]->b)
          memset(dlsch->harq_processes[i]->b,0,dlsch_bytes);
        else
          exit_flag=3;

        if (abstraction_flag == 0) {
          for (r=0; r<a_segments; r++) {
            dlsch->harq_processes[i]->p_nrLDPC_procBuf[r] = nrLDPC_init_mem();
            dlsch->harq_processes[i]->c[r] = (uint8_t *)malloc16(1056);

            if (dlsch->harq_processes[i]->c[r])
              memset(dlsch->harq_processes[i]->c[r],0,1056);
            else
              exit_flag=2;

            dlsch->harq_processes[i]->d[r] = (short *)malloc16((5*8448)*sizeof(short));

            if (dlsch->harq_processes[i]->d[r])
              memset(dlsch->harq_processes[i]->d[r],0,(5*8448)*sizeof(short));
            else
              exit_flag=2;

            dlsch->harq_processes[i]->w[r] = (short *)malloc16((5*8448)*sizeof(short));

            if (dlsch->harq_processes[i]->w[r])
              memset(dlsch->harq_processes[i]->w[r],0,(5*8448)*sizeof(short));
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

  LOG_D(PHY,"new_ue_dlsch with size %zu: exit_flag = %u\n",sizeof(NR_DL_UE_HARQ_t), exit_flag);
  free_nr_ue_dlsch(&dlsch,N_RB_DL);
  return(NULL);
}

void nr_dlsch_unscrambling(int16_t *llr,
                           uint32_t size,
                           uint8_t q,
                           uint32_t Nid,
                           uint32_t n_RNTI) {
  uint8_t reset;
  uint32_t x1, x2, s=0;
  reset = 1;
  x2 = (n_RNTI<<15) + (q<<14) + Nid;

  for (int i=0; i<size; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
    }

    if (((s>>(i&0x1f))&1)==1)
      llr[i] = -llr[i];
  }
}

uint32_t nr_dlsch_decoding(PHY_VARS_NR_UE *phy_vars_ue,
                           UE_nr_rxtx_proc_t *proc,
                           int eNB_id,
                           short *dlsch_llr,
                           NR_DL_FRAME_PARMS *frame_parms,
                           NR_UE_DLSCH_t *dlsch,
                           NR_DL_UE_HARQ_t *harq_process,
                           uint32_t frame,
                           uint16_t nb_symb_sch,
                           uint8_t nr_slot_rx,
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
  int32_t no_iteration_ldpc, length_dec;
  uint32_t r,r_offset=0,Kr=8424,Kr_bytes,K_bits_F,err_flag=0;
  uint8_t crc_type;
  int8_t llrProcBuf[NR_LDPC_MAX_NUM_LLR] __attribute__ ((aligned(32)));
  t_nrLDPC_dec_params decParams;
  t_nrLDPC_dec_params *p_decParams = &decParams;
  t_nrLDPC_time_stats procTime = {0};
  t_nrLDPC_time_stats *p_procTime =&procTime ;

  if (!harq_process) {
    LOG_E(PHY,"dlsch_decoding.c: NULL harq_process pointer\n");
    return(dlsch->max_ldpc_iterations + 1);
  }

  t_nrLDPC_procBuf **p_nrLDPC_procBuf = harq_process->p_nrLDPC_procBuf;
  // HARQ stats
  phy_vars_ue->dl_stats[harq_process->round]++;
  int16_t z [68*384];
  int8_t l [68*384];
  //__m128i l;
  //int16_t inv_d [68*384];
  uint8_t kc;
  uint8_t Ilbrm = 1;
  uint32_t Tbslbrm;// = 950984;
  uint16_t nb_rb;// = 30;
  double Coderate;// = 0.0;
  uint8_t dmrs_Type = harq_process->dmrsConfigType;
  AssertFatal(dmrs_Type == 0 || dmrs_Type == 1, "Illegal dmrs_type %d\n", dmrs_Type);
  uint8_t nb_re_dmrs;

  if (dmrs_Type==NFAPI_NR_DMRS_TYPE1) {
    nb_re_dmrs = 6*harq_process->n_dmrs_cdm_groups;
  } else {
    nb_re_dmrs = 4*harq_process->n_dmrs_cdm_groups;
  }

  uint16_t dmrs_length = get_num_dmrs(harq_process->dlDmrsSymbPos);
  uint32_t i,j;
  __m128i *pv = (__m128i *)&z;
  __m128i *pl = (__m128i *)&l;
  vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_SEGMENTATION, VCD_FUNCTION_IN);

  //NR_DL_UE_HARQ_t *harq_process = dlsch->harq_processes[0];

  if (!dlsch_llr) {
    LOG_E(PHY,"dlsch_decoding.c: NULL dlsch_llr pointer\n");
    return(dlsch->max_ldpc_iterations + 1);
  }

  if (!frame_parms) {
    LOG_E(PHY,"dlsch_decoding.c: NULL frame_parms pointer\n");
    return(dlsch->max_ldpc_iterations + 1);
  }

  /*if (nr_slot_rx> (frame_parms->slots_per_frame-1)) {
    printf("dlsch_decoding.c: Illegal slot index %d\n",nr_slot_rx);
    return(dlsch->max_ldpc_iterations + 1);
  }*/
  /*if (harq_process->harq_ack.ack != 2) {
    LOG_D(PHY, "[UE %d] DLSCH @ SF%d : ACK bit is %d instead of DTX even before PDSCH is decoded!\n",
        phy_vars_ue->Mod_id, nr_slot_rx, harq_process->harq_ack.ack);
  }*/
  //  nb_rb = dlsch->nb_rb;
  /*
  if (nb_rb > frame_parms->N_RB_DL) {
    printf("dlsch_decoding.c: Illegal nb_rb %d\n",nb_rb);
    return(max_ldpc_iterations + 1);
    }*/
  /*harq_pid = dlsch->current_harq_pid[proc->thread_id];
  if (harq_pid >= 8) {
    printf("dlsch_decoding.c: Illegal harq_pid %d\n",harq_pid);
    return(max_ldpc_iterations + 1);
  }
  */
  nb_rb = harq_process->nb_rb;
  harq_process->trials[harq_process->round]++;
  uint16_t nb_rb_oh = 0; // it was not computed at UE side even before and set to 0 in nr_compute_tbs
  harq_process->TBS = nr_compute_tbs(harq_process->Qm,harq_process->R,nb_rb,nb_symb_sch,nb_re_dmrs*dmrs_length, nb_rb_oh, 0, harq_process->Nl);
  A = harq_process->TBS;
  ret = dlsch->max_ldpc_iterations + 1;
  dlsch->last_iteration_cnt = ret;
  harq_process->G = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, dmrs_length, harq_process->Qm,harq_process->Nl);
  G = harq_process->G;

  LOG_D(PHY,"%d.%d DLSCH Decoding, harq_pid %d TBS %d (%d) G %d nb_re_dmrs %d length dmrs %d mcs %d Nl %d nb_symb_sch %d nb_rb %d\n",
        frame,nr_slot_rx,harq_pid,A,A/8,G, nb_re_dmrs, dmrs_length, harq_process->mcs, harq_process->Nl, nb_symb_sch,nb_rb);

  if ((harq_process->R)<1024)
    Coderate = (float) (harq_process->R) /(float) 1024;
  else
    Coderate = (float) (harq_process->R) /(float) 2048;

  if ((A <=292) || ((A <= NR_MAX_PDSCH_TBS) && (Coderate <= 0.6667)) || Coderate <= 0.25) {
    p_decParams->BG = 2;
    kc = 52;

    if (Coderate < 0.3333) {
      p_decParams->R = 15;
    } else if (Coderate <0.6667) {
      p_decParams->R = 13;
    } else {
      p_decParams->R = 23;
    }
  } else {
    p_decParams->BG = 1;
    kc = 68;

    if (Coderate < 0.6667) {
      p_decParams->R = 13;
    } else if (Coderate <0.8889) {
      p_decParams->R = 23;
    } else {
      p_decParams->R = 89;
    }
  }

  if (harq_process->first_rx == 1) {
    // This is a new packet, so compute quantities regarding segmentation
    if (A > NR_MAX_PDSCH_TBS)
      harq_process->B = A+24;
    else
      harq_process->B = A+16;

    nr_segmentation(NULL,
                    NULL,
                    harq_process->B,
                    &harq_process->C,
                    &harq_process->K,
                    &harq_process->Z, // [hna] Z is Zc
                    &harq_process->F,
                    p_decParams->BG);

    if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD) && (!frame%100))
      LOG_I(PHY,"K %d C %d Z %d nl %d \n", harq_process->K, harq_process->C, p_decParams->Z, harq_process->Nl);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_SEGMENTATION, VCD_FUNCTION_OUT);
  p_decParams->Z = harq_process->Z;
  //printf("dlsch decoding nr segmentation Z %d\n", p_decParams->Z);
  //printf("coderate %f kc %d \n", Coderate, kc);
  p_decParams->numMaxIter = dlsch->max_ldpc_iterations;
  p_decParams->outMode= 0;
  err_flag = 0;
  r_offset = 0;
  uint16_t a_segments = MAX_NUM_NR_DLSCH_SEGMENTS;  //number of segments to be allocated

  if (nb_rb != 273) {
    a_segments = a_segments*nb_rb;
    a_segments = a_segments/273 +1;
  }

  if (harq_process->C > a_segments) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,a_segments);
    return((1+dlsch->max_ldpc_iterations));
  }

  if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
    LOG_I(PHY,"Segmentation: C %d, K %d\n",harq_process->C,harq_process->K);

  opp_enabled=1;
  Kr = harq_process->K; // [hna] overwrites this line "Kr = p_decParams->Z*kb"
  Kr_bytes = Kr>>3;
  K_bits_F = Kr-harq_process->F;

  for (r=0; r<harq_process->C; r++) {
    //printf("start rx segment %d\n",r);
    E = nr_get_E(G, harq_process->C, harq_process->Qm, harq_process->Nl, r);
#if UE_TIMING_TRACE
    start_meas(dlsch_deinterleaving_stats);
#endif
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_DEINTERLEAVING, VCD_FUNCTION_IN);
    nr_deinterleaving_ldpc(E,
                           harq_process->Qm,
                           harq_process->w[r], // [hna] w is e
                           dlsch_llr+r_offset);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_DEINTERLEAVING, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
    stop_meas(dlsch_deinterleaving_stats);
#endif
#if UE_TIMING_TRACE
    start_meas(dlsch_rate_unmatching_stats);
#endif
    LOG_D(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,E %d, F %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
          harq_pid,r, G,E,harq_process->F,
          Kr*3,
          harq_process->TBS,
          harq_process->Qm,
          harq_process->nb_rb,
          harq_process->Nl,
          harq_process->rvidx,
          harq_process->round);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_RATE_MATCHING, VCD_FUNCTION_IN);

    if ((harq_process->Nl)<4)
      Tbslbrm = nr_compute_tbslbrm(harq_process->mcs_table,nb_rb,harq_process->Nl);
    else
      Tbslbrm = nr_compute_tbslbrm(harq_process->mcs_table,nb_rb,4);

    if (nr_rate_matching_ldpc_rx(Ilbrm,
                                 Tbslbrm,
                                 p_decParams->BG,
                                 p_decParams->Z,
                                 harq_process->d[r],
                                 harq_process->w[r],
                                 harq_process->C,
                                 harq_process->rvidx,
                                 (harq_process->first_rx==1)?1:0,
                                 E,
                                 harq_process->F,
                                 Kr-harq_process->F-2*(p_decParams->Z))==-1) {
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_RATE_MATCHING, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
      LOG_E(PHY,"dlsch_decoding.c: Problem in rate_matching\n");
      return(dlsch->max_ldpc_iterations + 1);
    } else {
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
    }

    r_offset += E;

    if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD)) {
      LOG_I(PHY,"decoder input(segment %u) :",r);

      for (int i=0; i<E; i++)
        LOG_D(PHY,"%d : %d\n",i,harq_process->d[r][i]);

      LOG_D(PHY,"\n");
    }

    memset(harq_process->c[r],0,Kr_bytes);

    if (harq_process->C == 1) {
      if (A > NR_MAX_PDSCH_TBS)
        crc_type = CRC24_A;
      else
        crc_type = CRC16;

      length_dec = harq_process->B;
    } else {
      crc_type = CRC24_B;
      length_dec = (harq_process->B+24*harq_process->C)/harq_process->C;
    }

    if (err_flag == 0) {
#if UE_TIMING_TRACE
      start_meas(dlsch_turbo_decoding_stats);
#endif
      //set first 2*Z_c bits to zeros
      memset(&z[0],0,2*harq_process->Z*sizeof(int16_t));
      //set Filler bits
      memset((&z[0]+K_bits_F),127,harq_process->F*sizeof(int16_t));
      //Move coded bits before filler bits
      memcpy((&z[0]+2*harq_process->Z),harq_process->d[r],(K_bits_F-2*harq_process->Z)*sizeof(int16_t));
      //skip filler bits
      memcpy((&z[0]+Kr),harq_process->d[r]+(Kr-2*harq_process->Z),(kc*harq_process->Z-Kr)*sizeof(int16_t));

      //Saturate coded bits before decoding into 8 bits values
      for (i=0, j=0; j < ((kc*harq_process->Z)>>4)+1;  i+=2, j++) {
        pl[j] = _mm_packs_epi16(pv[i],pv[i+1]);
      }

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_LDPC, VCD_FUNCTION_IN);
      no_iteration_ldpc = nrLDPC_decoder(p_decParams,
                                         (int8_t *)&pl[0],
                                         llrProcBuf,
                                         p_nrLDPC_procBuf[r],
                                         p_procTime);
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_LDPC, VCD_FUNCTION_OUT);

      // Fixme: correct type is unsigned, but nrLDPC_decoder and all called behind use signed int
      if (check_crc((uint8_t *)llrProcBuf,length_dec,harq_process->F,crc_type)) {
        LOG_D(PHY,"Segment %u CRC OK\n\033[0m",r);

        if (r==0) {
          for (int i=0; i<10; i++) LOG_D(PHY,"byte %d : %x\n",i,((uint8_t *)llrProcBuf)[i]);
        }

        //Temporary hack
        no_iteration_ldpc = dlsch->max_ldpc_iterations;
        ret = no_iteration_ldpc;
      } else {
        LOG_D(PHY,"CRC NOT OK\n\033[0m");
      }

      nb_total_decod++;

      if (no_iteration_ldpc > dlsch->max_ldpc_iterations) {
        nb_error_decod++;
      }

      for (int m=0; m < Kr>>3; m ++) {
        harq_process->c[r][m]= (uint8_t) llrProcBuf[m];
      }

      if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD)) {
        for (int k=0; k<A>>3; k++)
          LOG_D(PHY,"output decoder [%d] =  0x%02x \n", k, harq_process->c[r][k]);

        LOG_D(PHY,"no_iterations_ldpc %d (ret %u)\n",no_iteration_ldpc,ret);
      }

#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
    }

    if ((err_flag == 0) && (ret>=(1+dlsch->max_ldpc_iterations))) {// a Code segment is in error so break;
      LOG_D(PHY,"AbsSubframe %d.%d CRC failed, segment %d/%d \n",frame%1024,nr_slot_rx,r,harq_process->C-1);
      err_flag = 1;
    }
  }

  if (err_flag == 1) {
    LOG_D(PHY,"[UE %d] DLSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d) Kr %d r %d harq_process->round %d\n",
          phy_vars_ue->Mod_id, frame, nr_slot_rx, harq_pid,harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs,Kr,r,harq_process->round);
    harq_process->ack = 0;
    harq_process->errors[harq_process->round]++;

    if (harq_process->round >= dlsch->Mlimit) {
      harq_process->status = SCH_IDLE;
      harq_process->round  = 0;
      phy_vars_ue->dl_stats[4]++;
    }

    if(is_crnti) {
      LOG_D(PHY,"[UE %d] DLSCH: Setting NACK for nr_slot_rx %d (pid %d, pid status %d, round %d/Max %d, TBS %d)\n",
            phy_vars_ue->Mod_id,nr_slot_rx,harq_pid,harq_process->status,harq_process->round,dlsch->Mdlharq,harq_process->TBS);
    }

    return((1 + dlsch->max_ldpc_iterations));
  } else {
    LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for nr_slot_rx %d TBS %d mcs %d nb_rb %d harq_process->round %d\n",
          phy_vars_ue->Mod_id,nr_slot_rx,harq_process->TBS,harq_process->mcs,harq_process->nb_rb, harq_process->round);
    harq_process->status = SCH_IDLE;
    harq_process->round  = 0;
    harq_process->ack = 1;

    //LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d)\n",
    //  phy_vars_ue->Mod_id, frame, subframe, harq_pid, harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs);

    if(is_crnti) {
      LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for nr_slot_rx %d (pid %d, round %d, TBS %d)\n",phy_vars_ue->Mod_id,nr_slot_rx,harq_pid,harq_process->round,harq_process->TBS);
    }

    //LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);
  }

  // Reassembly of Transport block here
  offset = 0;
  Kr = harq_process->K;
  Kr_bytes = Kr>>3;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_COMBINE_SEG, VCD_FUNCTION_IN);

  for (r=0; r<harq_process->C; r++) {
    memcpy(harq_process->b+offset,
           harq_process->c[r],
           Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0));
    offset += (Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0));

    if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD)) {
      LOG_D(PHY,"Segment %u : Kr= %u bytes\n",r,Kr_bytes);
      LOG_D(PHY,"copied %d bytes to b sequence (harq_pid %d)\n",
            (Kr_bytes - (harq_process->F>>3)-((harq_process->C>1)?3:0)),harq_pid);
      LOG_D(PHY,"b[0] = %p,c[%d] = %p\n",
            (void *)(uint64_t)(harq_process->b[offset]),
            harq_process->F>>3,
            (void *)(uint64_t)(harq_process->c[r]) );

      if (frame%100 == 0) {
        LOG_D (PHY, "Printing 60 first payload bytes at frame: %d ", frame);

        for (int i = 0; i <60 ; i++) { //Kr_bytes
          LOG_D(PHY, "[%d] : %x ", i, harq_process->b[i]);
        }
      }
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_COMBINE_SEG, VCD_FUNCTION_OUT);
  dlsch->last_iteration_cnt = ret;
  return(ret);
}


uint32_t  nr_dlsch_decoding_mthread(PHY_VARS_NR_UE *phy_vars_ue,
                                    UE_nr_rxtx_proc_t *proc,
                                    int eNB_id,
                                    short *dlsch_llr,
                                    NR_DL_FRAME_PARMS *frame_parms,
                                    NR_UE_DLSCH_t *dlsch,
                                    NR_DL_UE_HARQ_t *harq_process,
                                    uint32_t frame,
                                    uint16_t nb_symb_sch,
                                    uint8_t nr_slot_rx,
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
  uint32_t r,r_offset=0,Kr=8424,Kr_bytes,err_flag=0,K_bits_F;
  uint8_t crc_type;
  //UE_rxtx_proc_t *proc = &phy_vars_ue->proc;
  int32_t no_iteration_ldpc,length_dec;
  /*uint8_t C;
  uint8_t Qm;
  uint8_t r_thread;
  uint32_t Er, Gp,GpmodC;*/
  t_nrLDPC_dec_params decParams;
  t_nrLDPC_dec_params *p_decParams = &decParams;
  t_nrLDPC_time_stats procTime;
  t_nrLDPC_time_stats *p_procTime =&procTime ;
  int8_t llrProcBuf[NR_LDPC_MAX_NUM_LLR] __attribute__ ((aligned(32)));

  if (!harq_process) {
    LOG_E(PHY,"dlsch_decoding.c: NULL harq_process pointer\n");
    return(dlsch->max_ldpc_iterations);
  }

  t_nrLDPC_procBuf *p_nrLDPC_procBuf = harq_process->p_nrLDPC_procBuf[0];
  uint8_t Nl=4;
  int16_t z [68*384];
  int8_t l [68*384];
  uint8_t kc;
  uint8_t Ilbrm = 1;
  uint32_t Tbslbrm = 950984;
  uint16_t nb_rb = 30;
  double Coderate = 0.0;
  uint8_t dmrs_type = harq_process->dmrsConfigType;
  uint8_t nb_re_dmrs;

  if (dmrs_type == NFAPI_NR_DMRS_TYPE1)
    nb_re_dmrs = 6*harq_process->n_dmrs_cdm_groups;
  else
    nb_re_dmrs = 4*harq_process->n_dmrs_cdm_groups;

  uint16_t length_dmrs = get_num_dmrs(harq_process->dlDmrsSymbPos);
  uint32_t i,j;
  __m128i *pv = (__m128i *)&z;
  __m128i *pl = (__m128i *)&l;
  notifiedFIFO_t nf;
  initNotifiedFIFO(&nf);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_SEGMENTATION, VCD_FUNCTION_IN);

  if (!dlsch_llr) {
    LOG_E(PHY,"dlsch_decoding.c: NULL dlsch_llr pointer\n");
    return(dlsch->max_ldpc_iterations);
  }

  if (!frame_parms) {
    LOG_E(PHY,"dlsch_decoding.c: NULL frame_parms pointer\n");
    return(dlsch->max_ldpc_iterations);
  }

  /* if (nr_slot_rx> (frame_parms->slots_per_frame-1)) {
     printf("dlsch_decoding.c: Illegal slot index %d\n",nr_slot_rx);
     return(dlsch->max_ldpc_iterations);
   }

   if (dlsch->harq_ack[nr_slot_rx].ack != 2) {
     LOG_D(PHY, "[UE %d] DLSCH @ SF%d : ACK bit is %d instead of DTX even before PDSCH is decoded!\n",
         phy_vars_ue->Mod_id, nr_slot_rx, dlsch->harq_ack[nr_slot_rx].ack);
   }*/
  /*
  if (nb_rb > frame_parms->N_RB_DL) {
    printf("dlsch_decoding.c: Illegal nb_rb %d\n",nb_rb);
    return(max_ldpc_iterations);
    }*/
  /*harq_pid = dlsch->current_harq_pid[proc->thread_id];
  if (harq_pid >= 8) {
    printf("dlsch_decoding.c: Illegal harq_pid %d\n",harq_pid);
    return(max_ldpc_iterations);
  }
  */
  nb_rb = harq_process->nb_rb;
  harq_process->trials[harq_process->round]++;
  // HARQ stats
  phy_vars_ue->dl_stats[harq_process->round]++;
  uint16_t nb_rb_oh = 0; // it was not computed at UE side even before and set to 0 in nr_compute_tbs
  harq_process->TBS = nr_compute_tbs(harq_process->Qm,harq_process->R,nb_rb,nb_symb_sch,nb_re_dmrs*length_dmrs, nb_rb_oh, 0, harq_process->Nl);
  A = harq_process->TBS;
  ret = dlsch->max_ldpc_iterations + 1;
  dlsch->last_iteration_cnt = ret;
  harq_process->G = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, harq_process->Qm,harq_process->Nl);
  G = harq_process->G;
  LOG_D(PHY,"DLSCH Decoding main, harq_pid %d TBS %d G %d, nb_re_dmrs %d, length_dmrs %d  mcs %d Nl %d nb_symb_sch %d nb_rb %d\n",harq_pid,A,G, nb_re_dmrs, length_dmrs, harq_process->mcs,
        harq_process->Nl, nb_symb_sch,nb_rb);
  proc->decoder_main_available = 1;
  proc->decoder_thread_available = 0;
  proc->decoder_thread_available1 = 0;

  if ((harq_process->R)<1024)
    Coderate = (float) (harq_process->R) /(float) 1024;
  else
    Coderate = (float) (harq_process->R) /(float) 2048;

  if ((A <= 292) || ((A <= NR_MAX_PDSCH_TBS) && (Coderate <= 0.6667)) || Coderate <= 0.25) {
    p_decParams->BG = 2;
    kc = 52;

    if (Coderate < 0.3333) {
      p_decParams->R = 15;
    } else if (Coderate <0.6667) {
      p_decParams->R = 13;
    } else {
      p_decParams->R = 23;
    }
  } else {
    p_decParams->BG = 1;
    kc = 68;

    if (Coderate < 0.6667) {
      p_decParams->R = 13;
    } else if (Coderate <0.8889) {
      p_decParams->R = 23;
    } else {
      p_decParams->R = 89;
    }
  }

  if (harq_process->first_rx == 1) {
    // This is a new packet, so compute quantities regarding segmentation
    if (A > NR_MAX_PDSCH_TBS)
      harq_process->B = A+24;
    else
      harq_process->B = A+16;

    nr_segmentation(NULL,
                    NULL,
                    harq_process->B,
                    &harq_process->C,
                    &harq_process->K,
                    &harq_process->Z,
                    &harq_process->F,
                    p_decParams->BG);
  }

  p_decParams->Z = harq_process->Z;
  p_decParams->numMaxIter = dlsch->max_ldpc_iterations;
  p_decParams->outMode= 0;
  err_flag = 0;
  r_offset = 0;
  uint16_t a_segments = MAX_NUM_NR_DLSCH_SEGMENTS;  //number of segments to be allocated

  if (nb_rb != 273) {
    a_segments = a_segments*nb_rb;
    a_segments = a_segments/273 +1;
  }

  if (harq_process->C > a_segments) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,a_segments);
    return((1+dlsch->max_ldpc_iterations));
  }

  if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
    LOG_D(PHY,"Segmentation: C %d, K %d\n",harq_process->C,harq_process->K);

  notifiedFIFO_elt_t *res_dl;
  opp_enabled=1;

  if (harq_process->C>1) {
    for (int nb_seg =1 ; nb_seg<harq_process->C; nb_seg++) {
      if ( (res_dl=tryPullTpool(&nf, &pool_dl)) != NULL ) {
        pushNotifiedFIFO_nothreadSafe(&freeBlocks_dl,res_dl);
      }

      AssertFatal((msgToPush_dl=pullNotifiedFIFO_nothreadSafe(&freeBlocks_dl)) != NULL,"chained list failure");
      nr_rxtx_thread_data_t *curMsg=(nr_rxtx_thread_data_t *)NotifiedFifoData(msgToPush_dl);
      curMsg->UE=phy_vars_ue;
      nbDlProcessing++;
      memset(&curMsg->proc, 0, sizeof(curMsg->proc));
      curMsg->proc.frame_rx   = proc->frame_rx;
      curMsg->proc.nr_slot_rx = proc->nr_slot_rx;
      curMsg->proc.thread_id  = proc->thread_id;
      curMsg->proc.num_seg    = nb_seg;
      curMsg->proc.eNB_id= eNB_id;
      curMsg->proc.harq_pid=harq_pid;
      curMsg->proc.llr8_flag = llr8_flag;
      msgToPush_dl->key= (nr_slot_rx%2) ? (nb_seg+30): nb_seg;
      pushTpool(&pool_dl, msgToPush_dl);
      /*Qm= harq_process->Qm;
        Nl=harq_process->Nl;
        r_thread = harq_process->C/2-1;
        C= harq_process->C;

        Gp = G/Nl/Qm;
        GpmodC = Gp%C;


        if (r_thread < (C-(GpmodC)))
          Er = Nl*Qm * (Gp/C);
        else
          Er = Nl*Qm * ((GpmodC==0?0:1) + (Gp/C));
        printf("mthread Er %d\n", Er);

        printf("mthread instance_cnt_dlsch_td %d\n",  proc->instance_cnt_dlsch_td);*/
    }

    //proc->decoder_main_available = 1;
  }

  r = 0;

  if (r==0) r_offset =0;

  Kr = harq_process->K;
  Kr_bytes = Kr>>3;
  K_bits_F = Kr-harq_process->F;
  E = nr_get_E(G, harq_process->C, harq_process->Qm, harq_process->Nl, r);
  /*
  printf("Subblock deinterleaving, dlsch_llr %p, w %p\n",
   dlsch_llr+r_offset,
   &harq_process->w[r]);
  */
#if UE_TIMING_TRACE
  start_meas(dlsch_deinterleaving_stats);
#endif
  nr_deinterleaving_ldpc(E,
                         harq_process->Qm,
                         harq_process->w[r],
                         dlsch_llr+r_offset);

  if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
    for (int i =0; i<16; i++)
      LOG_D(PHY,"rx output deinterleaving w[%d]= %d r_offset %u\n", i,harq_process->w[r][i], r_offset);

#if UE_TIMING_TRACE
  stop_meas(dlsch_deinterleaving_stats);
#endif
#if UE_TIMING_TRACE
  start_meas(dlsch_rate_unmatching_stats);
#endif

  if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
    LOG_I(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
          harq_pid,r, G,
          Kr*3,
          harq_process->TBS,
          harq_process->Qm,
          harq_process->nb_rb,
          harq_process->Nl,
          harq_process->rvidx,
          harq_process->round);

  // for tbslbrm calculation according to 5.4.2.1 of 38.212
  if (harq_process->Nl < Nl)
    Nl = harq_process->Nl;

  Tbslbrm = nr_compute_tbslbrm(harq_process->mcs_table,nb_rb,harq_process->Nl);

  if (nr_rate_matching_ldpc_rx(Ilbrm,
                               Tbslbrm,
                               p_decParams->BG,
                               p_decParams->Z,
                               harq_process->d[r],
                               harq_process->w[r],
                               harq_process->C,
                               harq_process->rvidx,
                               (harq_process->first_rx==1)?1:0,
                               E,
                               harq_process->F,
                               Kr-harq_process->F-2*(p_decParams->Z))==-1) {
#if UE_TIMING_TRACE
    stop_meas(dlsch_rate_unmatching_stats);
#endif
    LOG_E(PHY,"dlsch_decoding.c: Problem in rate_matching\n");
    return(dlsch->max_ldpc_iterations);
  } else {
#if UE_TIMING_TRACE
    stop_meas(dlsch_rate_unmatching_stats);
#endif
  }

  if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
    for (int i =0; i<16; i++)
      LOG_I(PHY,"rx output ratematching d[%d]= %d r_offset %u\n", i,harq_process->d[r][i], r_offset);

  if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD)) {
    if (r==0) {
      LOG_M("decoder_llr.m","decllr",dlsch_llr,G,1,0);
      LOG_M("decoder_in.m","dec",&harq_process->d[0][96],(3*8*Kr_bytes)+12,1,0);
    }

    LOG_D(PHY,"decoder input(segment %u) :",r);

    for (int i=0; i<(3*8*Kr_bytes); i++)
      LOG_D(PHY,"%d : %d\n",i,harq_process->d[r][i]);

    LOG_D(PHY,"\n");
  }

  memset(harq_process->c[r],0,Kr_bytes);

  if (harq_process->C == 1) {
    if (A > NR_MAX_PDSCH_TBS)
      crc_type = CRC24_A;
    else
      crc_type = CRC16;

    length_dec = harq_process->B;
  } else {
    crc_type = CRC24_B;
    length_dec = (harq_process->B+24*harq_process->C)/harq_process->C;
  }

  //#ifndef __AVX2__

  if (err_flag == 0) {
    /*
            LOG_D(PHY, "LDPC algo Kr=%d cb_cnt=%d C=%d nbRB=%d crc_type %d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d maxIter %d\n",
                                Kr,r,harq_process->C,harq_process->nb_rb,crc_type,A,harq_process->TBS,
                                harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round,dlsch->max_ldpc_iterations);
    */
#if UE_TIMING_TRACE
    start_meas(dlsch_turbo_decoding_stats);
#endif
    LOG_D(PHY,"mthread AbsSubframe %d.%d Start LDPC segment %d/%d \n",frame%1024,nr_slot_rx,r,harq_process->C-1);
    /*for (int cnt =0; cnt < (kc-2)*p_decParams->Z; cnt++){
      inv_d[cnt] = (1)*harq_process->d[r][cnt];
    }*/
    //set first 2*Z_c bits to zeros
    memset(&z[0],0,2*harq_process->Z*sizeof(int16_t));
    //set Filler bits
    memset((&z[0]+K_bits_F),127,harq_process->F*sizeof(int16_t));
    //Move coded bits before filler bits
    memcpy((&z[0]+2*harq_process->Z),harq_process->d[r],(K_bits_F-2*harq_process->Z)*sizeof(int16_t));
    //skip filler bits
    memcpy((&z[0]+Kr),harq_process->d[r]+(Kr-2*harq_process->Z),(kc*harq_process->Z-Kr)*sizeof(int16_t));

    //Saturate coded bits before decoding into 8 bits values
    for (i=0, j=0; j < ((kc*harq_process->Z)>>4)+1;  i+=2, j++) {
      pl[j] = _mm_packs_epi16(pv[i],pv[i+1]);
    }

    no_iteration_ldpc = nrLDPC_decoder(p_decParams,
                                       (int8_t *)&pl[0],
                                       llrProcBuf,
                                       p_nrLDPC_procBuf,
                                       p_procTime);
    nb_total_decod++;

    if (no_iteration_ldpc > 10) {
      nb_error_decod++;
      ret = 1+dlsch->max_ldpc_iterations;
    } else {
      ret=2;
    }

    if (check_crc((uint8_t *)llrProcBuf,length_dec,harq_process->F,crc_type)) {
      LOG_D(PHY,"Segment %u CRC OK\n",r);
      ret = 2;
    } else {
      ret = 1+dlsch->max_ldpc_iterations;
    }

    if (!nb_total_decod%10000) {
      printf("Error number of iteration LPDC %d %ld/%ld \n", no_iteration_ldpc, nb_error_decod,nb_total_decod);
      fflush(stdout);
    }

    for (int m=0; m < Kr>>3; m ++) {
      harq_process->c[r][m]= (uint8_t) llrProcBuf[m];
    }

    /*for (int u=0; u < Kr>>3; u ++)
      {
        ullrProcBuf[u]= (uint8_t) llrProcBuf[u];
      }


      printf("output unsigned ullrProcBuf \n");

      for (int j=0; j < Kr>>3; j ++)
      {
        printf(" %d \n", ullrProcBuf[j]);
      }
      printf(" \n");*/
    //printf("output channel decoder %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1], harq_process->c[r][2],harq_process->c[r][3], harq_process->c[r][4]);

    //printf("output decoder %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1], harq_process->c[r][2],harq_process->c[r][3], harq_process->c[r][4]);
    if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
      for (int k=0; k<32; k++)
        LOG_D(PHY,"output decoder [%d] =  0x%02x \n", k, harq_process->c[r][k]);

#if UE_TIMING_TRACE
    stop_meas(dlsch_turbo_decoding_stats);
#endif
  }

  if ((err_flag == 0) && (ret>=(1+dlsch->max_ldpc_iterations))) {// a Code segment is in error so break;
    LOG_D(PHY,"AbsSubframe %d.%d CRC failed, segment %d/%d \n",frame%1024,nr_slot_rx,r,harq_process->C-1);
    err_flag = 1;
  }

  //} //loop r

  if (err_flag == 1) {
    if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
      LOG_I(PHY,"[UE %d] DLSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d) Kr %d r %d harq_process->round %d\n",
            phy_vars_ue->Mod_id, frame, nr_slot_rx, harq_pid,harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs,Kr,r,harq_process->round);
    harq_process->ack = 0;
    harq_process->errors[harq_process->round]++;
    harq_process->round++;

    if (harq_process->round >= dlsch->Mlimit) {
      harq_process->status = SCH_IDLE;
      harq_process->round  = 0;
    }

    if(is_crnti) {
      LOG_D(PHY,"[UE %d] DLSCH: Setting NACK for nr_slot_rx %d (pid %d, pid status %d, round %d/Max %d, TBS %d)\n",
            phy_vars_ue->Mod_id,nr_slot_rx,harq_pid,harq_process->status,harq_process->round,dlsch->Mlimit,harq_process->TBS);
    }

    return((1+dlsch->max_ldpc_iterations));
  } else {
    if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
      LOG_I(PHY,"[UE %d] DLSCH: Setting ACK for nr_slot_rx %d TBS %d mcs %d nb_rb %d\n",
            phy_vars_ue->Mod_id,nr_slot_rx,harq_process->TBS,harq_process->mcs,harq_process->nb_rb);

    harq_process->status = SCH_IDLE;
    harq_process->round  = 0;
    harq_process->ack = 1;
    //LOG_I(PHY,"[UE %d] DLSCH: Setting ACK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d)\n",
    //  phy_vars_ue->Mod_id, frame, subframe, harq_pid, harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs);

    if(is_crnti) {
      LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for nr_slot_rx %d (pid %d, round %d, TBS %d)\n",phy_vars_ue->Mod_id,nr_slot_rx,harq_pid,harq_process->round,harq_process->TBS);
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
  //uint32_t wait = 0;
  /* while((proc->decoder_thread_available == 0) )
  {
          usleep(1);
  }
  proc->decoder_thread_available == 0;*/
  /*notifiedFIFO_elt_t *res1=tryPullTpool(&nf, Tpool);
  if (!res1) {
    printf("mthread trypull null\n");
    usleep(1);
    wait++;
  }*/
  //usleep(50);
  proc->decoder_main_available = 0;
  Kr = harq_process->K; //to check if same K in all segments
  Kr_bytes = Kr>>3;

  for (r=0; r<harq_process->C; r++) {
    memcpy(harq_process->b+offset,
           harq_process->c[r],
           Kr_bytes- - (harq_process->F>>3) -((harq_process->C>1)?3:0));
    offset += (Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0));

    if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD)) {
      LOG_I(PHY,"Segment %u : Kr= %u bytes\n",r,Kr_bytes);
      LOG_I(PHY,"copied %d bytes to b sequence (harq_pid %d)\n",
            (Kr_bytes - (harq_process->F>>3)-((harq_process->C>1)?3:0)),harq_pid);
      LOG_I(PHY,"b[0] = %p,c[%d] = %p\n",
            (void *)(uint64_t)(harq_process->b[offset]),
            harq_process->F>>3,
            (void *)(uint64_t)(harq_process->c[r]));
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_SEGMENTATION, VCD_FUNCTION_OUT);
  dlsch->last_iteration_cnt = ret;
  //proc->decoder_thread_available = 0;
  //proc->decoder_main_available = 0;
  return(ret);
}



void nr_dlsch_decoding_process(void *arg) {
  nr_rxtx_thread_data_t *rxtxD= (nr_rxtx_thread_data_t *)arg;
  UE_nr_rxtx_proc_t *proc = &rxtxD->proc;
  PHY_VARS_NR_UE    *phy_vars_ue   = rxtxD->UE;
  int llr8_flag1;
  int32_t no_iteration_ldpc,length_dec;
  t_nrLDPC_dec_params decParams;
  t_nrLDPC_dec_params *p_decParams = &decParams;
  t_nrLDPC_time_stats procTime;
  t_nrLDPC_time_stats *p_procTime =&procTime ;
  int8_t llrProcBuf[NR_LDPC_MAX_NUM_LLR] __attribute__ ((aligned(32)));
  t_nrLDPC_procBuf *p_nrLDPC_procBuf;
  int16_t z [68*384];
  int8_t l [68*384];
  //__m128i l;
  //int16_t inv_d [68*384];
  //int16_t *p_invd =&inv_d;
  uint8_t  kc;
  uint8_t Ilbrm = 1;
  uint32_t Tbslbrm = 950984;
  uint16_t nb_rb = 30; //to update
  double Coderate = 0.0;
  uint16_t nb_symb_sch = 12;
  uint8_t nb_re_dmrs = 6;
  uint16_t length_dmrs = 1;
  uint32_t i,j;
  __m128i *pv = (__m128i *)&z;
  __m128i *pl = (__m128i *)&l;
  proc->instance_cnt_dlsch_td=-1;
  //proc->nr_slot_rx = proc->sub_frame_start * frame_parms->slots_per_subframe;
  proc->decoder_thread_available = 1;
#if UE_TIMING_TRACE
  time_stats_t *dlsch_rate_unmatching_stats=&phy_vars_ue->dlsch_rate_unmatching_stats;
  time_stats_t *dlsch_turbo_decoding_stats=&phy_vars_ue->dlsch_turbo_decoding_stats;
  time_stats_t *dlsch_deinterleaving_stats=&phy_vars_ue->dlsch_deinterleaving_stats;
#endif
  uint32_t A,E;
  uint32_t G;
  uint32_t ret;
  uint32_t r,r_offset=0,Kr,Kr_bytes,err_flag=0,K_bits_F;
  uint8_t crc_type;
  uint8_t C,Cprime;
  uint8_t Qm;
  uint8_t Nl;
  //uint32_t Er;
  int eNB_id                = proc->eNB_id;
  int harq_pid              = proc->harq_pid;
  llr8_flag1                = proc->llr8_flag;
  int frame                 = proc->frame_rx;
  r                     = proc->num_seg;
  NR_UE_DLSCH_t *dlsch      = phy_vars_ue->dlsch[proc->thread_id][eNB_id][0];
  NR_DL_UE_HARQ_t *harq_process  = dlsch->harq_processes[harq_pid];
  short *dlsch_llr        = phy_vars_ue->pdsch_vars[proc->thread_id][eNB_id]->llr[0];
  p_nrLDPC_procBuf = harq_process->p_nrLDPC_procBuf[r];
  nb_symb_sch = harq_process->nb_symbols;
  LOG_D(PHY,"dlsch decoding process frame %d slot %d segment %d r %u nb symb %d \n", frame, proc->nr_slot_rx, proc->num_seg, r, harq_process->nb_symbols);
  nb_rb = harq_process->nb_rb;
  harq_process->trials[harq_process->round]++;
  uint16_t nb_rb_oh = 0; // it was not computed at UE side even before and set to 0 in nr_compute_tbs
  harq_process->TBS = nr_compute_tbs(harq_process->Qm,harq_process->R,nb_rb,nb_symb_sch,nb_re_dmrs*length_dmrs, nb_rb_oh, 0, harq_process->Nl);
  A = harq_process->TBS; //2072 for QPSK 1/3
  ret = dlsch->max_ldpc_iterations;
  harq_process->G = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, harq_process->Qm,harq_process->Nl);
  G = harq_process->G;

  LOG_D(PHY,"DLSCH Decoding process, harq_pid %d TBS %d G %d mcs %d Nl %d nb_symb_sch %d nb_rb %d, Coderate %d\n",harq_pid,A,G, harq_process->mcs, harq_process->Nl, nb_symb_sch,nb_rb,harq_process->R);

  if ((harq_process->R)<1024)
    Coderate = (float) (harq_process->R) /(float) 1024;
  else
    Coderate = (float) (harq_process->R) /(float) 2048;

  if ((A <= 292) || ((A <= NR_MAX_PDSCH_TBS) && (Coderate <= 0.6667)) || Coderate <= 0.25) {
    p_decParams->BG = 2;
    kc = 52;

    if (Coderate < 0.3333) {
      p_decParams->R = 15;
    } else if (Coderate <0.6667) {
      p_decParams->R = 13;
    } else {
      p_decParams->R = 23;
    }
  } else {
    p_decParams->BG = 1;
    kc = 68;

    if (Coderate < 0.6667) {
      p_decParams->R = 13;
    } else if (Coderate <0.8889) {
      p_decParams->R = 23;
    } else {
      p_decParams->R = 89;
    }
  }

  if (harq_process->first_rx == 1) {
    // This is a new packet, so compute quantities regarding segmentation
    if (A > NR_MAX_PDSCH_TBS)
      harq_process->B = A+24;
    else
      harq_process->B = A+16;

    nr_segmentation(NULL,
                    NULL,
                    harq_process->B,
                    &harq_process->C,
                    &harq_process->K,
                    &harq_process->Z,
                    &harq_process->F,
                    p_decParams->BG);
    p_decParams->Z = harq_process->Z;
  }
  LOG_D(PHY,"round %d Z %d K %d BG %d\n", harq_process->round, p_decParams->Z, harq_process->K, p_decParams->BG);
  p_decParams->numMaxIter = dlsch->max_ldpc_iterations;
  p_decParams->outMode= 0;
  err_flag = 0;
  opp_enabled=1;
  Qm= harq_process->Qm;
  Nl=harq_process->Nl;
  //r_thread = harq_process->C/2-1;
  C= harq_process->C;
  Cprime = C; //assume CBGTI not present

  if (r <= Cprime - ((G/(Nl*Qm))%Cprime) - 1)
    r_offset = Nl*Qm*(G/(Nl*Qm*Cprime));
  else
    r_offset = Nl*Qm*((G/(Nl*Qm*Cprime))+1);

  //for (r=(harq_process->C/2); r<harq_process->C; r++) {
  //    r=1; //(harq_process->C/2);
  r_offset = r*r_offset;
  Kr = harq_process->K;
  Kr_bytes = Kr>>3;
  K_bits_F = Kr-harq_process->F;
  E = nr_get_E(G, harq_process->C, harq_process->Qm, harq_process->Nl, r);
#if UE_TIMING_TRACE
  start_meas(dlsch_deinterleaving_stats);
#endif
  nr_deinterleaving_ldpc(E,
                         harq_process->Qm,
                         harq_process->w[r],
                         dlsch_llr+r_offset);

  if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
    for (int i =0; i<16; i++)
      LOG_D(PHY,"rx output thread 0 deinterleaving w[%d]= %d r_offset %u\n", i,harq_process->w[r][i], r_offset);

#if UE_TIMING_TRACE
  stop_meas(dlsch_deinterleaving_stats);
#endif
#if UE_TIMING_TRACE
  start_meas(dlsch_rate_unmatching_stats);
#endif

  if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
    LOG_I(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
          harq_pid,r, G,
          Kr*3,
          harq_process->TBS,
          harq_process->Qm,
          harq_process->nb_rb,
          harq_process->Nl,
          harq_process->rvidx,
          harq_process->round);

  if (Nl<4)
    Tbslbrm = nr_compute_tbslbrm(harq_process->mcs_table,nb_rb,Nl);
  else
    Tbslbrm = nr_compute_tbslbrm(harq_process->mcs_table,nb_rb,4);

  if (nr_rate_matching_ldpc_rx(Ilbrm,
                               Tbslbrm,
                               p_decParams->BG,
                               p_decParams->Z,
                               harq_process->d[r],
                               harq_process->w[r],
                               harq_process->C,
                               harq_process->rvidx,
                               (harq_process->first_rx==1)?1:0,
                               E,
                               harq_process->F,
                               Kr-harq_process->F-2*(p_decParams->Z))==-1) {
#if UE_TIMING_TRACE
    stop_meas(dlsch_rate_unmatching_stats);
#endif
    LOG_E(PHY,"dlsch_decoding.c: Problem in rate_matching\n");
    //return(dlsch->max_ldpc_iterations);
  } else {
#if UE_TIMING_TRACE
    stop_meas(dlsch_rate_unmatching_stats);
#endif
  }

  if (LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD)) {
    LOG_D(PHY,"decoder input(segment %u) :",r);

    for (int i=0; i<(3*8*Kr_bytes)+12; i++)
      LOG_D(PHY,"%d : %d\n",i,harq_process->d[r][i]);

    LOG_D(PHY,"\n");
  }

  memset(harq_process->c[r],0,Kr_bytes);

  if (harq_process->C == 1) {
    if (A > NR_MAX_PDSCH_TBS)
      crc_type = CRC24_A;
    else
      crc_type = CRC16;

    length_dec = harq_process->B;
  } else {
    crc_type = CRC24_B;
    length_dec = (harq_process->B+24*harq_process->C)/harq_process->C;
  }

  if (err_flag == 0) {
    /*
            LOG_D(PHY, "LDPC algo Kr=%d cb_cnt=%d C=%d nbRB=%d crc_type %d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d maxIter %d\n",
                                Kr,r,harq_process->C,harq_process->nb_rb,crc_type,A,harq_process->TBS,
                                harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round,dlsch->max_ldpc_iterations);
    */
    if (llr8_flag1) {
      AssertFatal (Kr >= 256, "LDPC algo issue Kr=%d cb_cnt=%d C=%d nbRB=%d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d\n",
                   Kr,r,harq_process->C,harq_process->nb_rb,A,harq_process->TBS,harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round);
    }

#if UE_TIMING_TRACE
    start_meas(dlsch_turbo_decoding_stats);
#endif
    //      LOG_D(PHY,"AbsSubframe %d.%d Start LDPC segment %d/%d \n",frame%1024,subframe,r,harq_process->C-1);
    /*
            for (int cnt =0; cnt < (kc-2)*p_decParams->Z; cnt++){
                  inv_d[cnt] = (1)*harq_process->d[r][cnt];
                  }
    */
    //set first 2*Z_c bits to zeros
    memset(&z[0],0,2*harq_process->Z*sizeof(int16_t));
    //set Filler bits
    memset((&z[0]+K_bits_F),127,harq_process->F*sizeof(int16_t));
    //Move coded bits before filler bits
    memcpy((&z[0]+2*harq_process->Z),harq_process->d[r],(K_bits_F-2*harq_process->Z)*sizeof(int16_t));
    //skip filler bits
    memcpy((&z[0]+Kr),harq_process->d[r]+(Kr-2*harq_process->Z),(kc*harq_process->Z-Kr)*sizeof(int16_t));

    //Saturate coded bits before decoding into 8 bits values
    for (i=0, j=0; j < ((kc*harq_process->Z)>>4)+1;  i+=2, j++) {
      pl[j] = _mm_packs_epi16(pv[i],pv[i+1]);
    }

    no_iteration_ldpc = nrLDPC_decoder(p_decParams,
                                       (int8_t *)&pl[0],
                                       llrProcBuf,
                                       p_nrLDPC_procBuf,
                                       p_procTime);

    // Fixme: correct type is unsigned, but nrLDPC_decoder and all called behind use signed int
    if (check_crc((uint8_t *)llrProcBuf,length_dec,harq_process->F,crc_type)) {
      LOG_D(PHY,"Segment %u CRC OK\n",r);
      ret = 2;
    } else {
      LOG_D(PHY,"Segment %u CRC NOK\n",r);
      ret = 1+dlsch->max_ldpc_iterations;
    }

    if (no_iteration_ldpc > 10)
      LOG_D(PHY,"Error number of iteration LPDC %d\n", no_iteration_ldpc);

    for (int m=0; m < Kr>>3; m ++) {
      harq_process->c[r][m]= (uint8_t) llrProcBuf[m];
    }

    if ( LOG_DEBUGFLAG(DEBUG_DLSCH_DECOD))
      for (int k=0; k<2; k++)
        LOG_D(PHY,"segment 1 output decoder [%d] =  0x%02x \n", k, harq_process->c[r][k]);

#if UE_TIMING_TRACE
    stop_meas(dlsch_turbo_decoding_stats);
#endif
  }

  if ((err_flag == 0) && (ret>=(1+dlsch->max_ldpc_iterations))) {// a Code segment is in error so break;
    //      LOG_D(PHY,"AbsSubframe %d.%d CRC failed, segment %d/%d \n",frame%1024,subframe,r,harq_process->C-1);
    err_flag = 1;
  }

  //}
  proc->decoder_thread_available = 1;
  //proc->decoder_main_available = 0;
}

void *dlsch_thread(void *arg) {
  //this thread should be over the processing thread to keep in real time
  notifiedFIFO_t nf;
  initNotifiedFIFO(&nf);
  notifiedFIFO_elt_t *res_dl;
  initNotifiedFIFO_nothreadSafe(&freeBlocks_dl);

  for (int i=0; i<tpool_nbthreads(pool_dl)+1; i++) {
    pushNotifiedFIFO_nothreadSafe(&freeBlocks_dl,
                                  newNotifiedFIFO_elt(sizeof(nr_rxtx_thread_data_t), 0,&nf,nr_dlsch_decoding_process));
  }

  while (!oai_exit) {
    notifiedFIFO_elt_t *res;

    while (nbDlProcessing >= tpool_nbthreads(pool_dl)) {
      if ( (res=tryPullTpool(&nf, &pool_dl)) != NULL ) {
        //nbDlProcessing--;
        pushNotifiedFIFO_nothreadSafe(&freeBlocks_dl,res);
      }

      usleep(200);
    }

    res_dl=pullTpool(&nf, &pool_dl);
    nbDlProcessing--;
    pushNotifiedFIFO_nothreadSafe(&freeBlocks_dl,res_dl);
    //msgToPush->key=0;
    //pushTpool(Tpool, msgToPush);
  } // while !oai_exit

  return NULL;
}


