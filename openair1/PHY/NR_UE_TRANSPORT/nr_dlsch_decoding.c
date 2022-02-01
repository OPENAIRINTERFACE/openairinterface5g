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

#define OAI_UL_LDPC_MAX_NUM_LLR 27000//26112 // NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX = 68*384
//#define OAI_LDPC_MAX_NUM_LLR 27000//26112 // NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX

static uint64_t nb_total_decod =0;
static uint64_t nb_error_decod =0;

notifiedFIFO_t freeBlocks_dl;
notifiedFIFO_elt_t *msgToPush_dl;
int nbDlProcessing =0;


static  tpool_t pool_dl;
//extern double cpuf;

void init_dlsch_tpool(uint8_t num_dlsch_threads) {
  char *params = NULL;

  if( num_dlsch_threads==0) {
    params = calloc(1,2);
    memcpy(params,"N",1);
  }
  else {
    params = calloc(1,(num_dlsch_threads*3)+1);
    for (int i=0; i<num_dlsch_threads; i++) {
      memcpy(params+(i*3),"-1,",3);
    }
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

bool nr_ue_postDecode(PHY_VARS_NR_UE *phy_vars_ue, notifiedFIFO_elt_t *req, bool last, notifiedFIFO_t *nf_p) {
  ldpcDecode_ue_t *rdata = (ldpcDecode_ue_t*) NotifiedFifoData(req);
  NR_DL_UE_HARQ_t *harq_process = rdata->harq_process;
  NR_UE_DLSCH_t *dlsch = (NR_UE_DLSCH_t *) rdata->dlsch;
  int r = rdata->segment_r;

  bool decodeSuccess = (rdata->decodeIterations < (1+dlsch->max_ldpc_iterations));

  if (decodeSuccess) {
    memcpy(harq_process->b+rdata->offset,
           harq_process->c[r],
           rdata->Kr_bytes - (harq_process->F>>3) -((harq_process->C>1)?3:0));

  } else {
    if ( !last ) {
      int nb=abortTpool(&(pool_dl), req->key);
      nb+=abortNotifiedFIFO(nf_p, req->key);
      LOG_D(PHY,"downlink segment error %d/%d, aborted %d segments\n",rdata->segment_r,rdata->nbSegments, nb);
      LOG_D(PHY, "DLSCH %d in error\n",rdata->dlsch_id);
      last = true;
    }
  }

  // if all segments are done
  if (last) {
    if (decodeSuccess) {
      //LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for nr_slot_rx %d TBS %d mcs %d nb_rb %d harq_process->round %d\n",
      //      phy_vars_ue->Mod_id,nr_slot_rx,harq_process->TBS,harq_process->mcs,harq_process->nb_rb, harq_process->round);
      harq_process->status = SCH_IDLE;
      harq_process->round  = 0;
      harq_process->ack = 1;

      //LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d)\n",
      //  phy_vars_ue->Mod_id, frame, subframe, harq_pid, harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs);

      //if(is_crnti) {
      //  LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for nr_slot_rx %d (pid %d, round %d, TBS %d)\n",phy_vars_ue->Mod_id,nr_slot_rx,harq_pid,harq_process->round,harq_process->TBS);
      //}
      dlsch->last_iteration_cnt = rdata->decodeIterations;
      LOG_D(PHY, "DLSCH received ok \n");
    } else {
      //LOG_D(PHY,"[UE %d] DLSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d) Kr %d r %d harq_process->round %d\n",
      //      phy_vars_ue->Mod_id, frame, nr_slot_rx, harq_pid,harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs,Kr,r,harq_process->round);
      harq_process->ack = 0;
      harq_process->round++;
      if (harq_process->round >= dlsch->Mlimit) {
        harq_process->status = SCH_IDLE;
        harq_process->round  = 0;
        phy_vars_ue->dl_stats[4]++;
      }

      //if(is_crnti) {
      //  LOG_D(PHY,"[UE %d] DLSCH: Setting NACK for nr_slot_rx %d (pid %d, pid status %d, round %d/Max %d, TBS %d)\n",
      //        phy_vars_ue->Mod_id,nr_slot_rx,harq_pid,harq_process->status,harq_process->round,dlsch->Mdlharq,harq_process->TBS);
      //}
      dlsch->last_iteration_cnt = dlsch->max_ldpc_iterations + 1;
      LOG_D(PHY, "DLSCH received nok \n");
    }
    return true; //stop
  }
  else
  {
	return false; //not last one
  }
}

void nr_processDLSegment(void* arg) {
  ldpcDecode_ue_t *rdata = (ldpcDecode_ue_t*) arg;
  NR_UE_DLSCH_t *dlsch = rdata->dlsch;
#if UE_TIMING_TRACE //TBD
  PHY_VARS_NR_UE *phy_vars_ue = rdata->phy_vars_ue;
  time_stats_t *dlsch_rate_unmatching_stats=&phy_vars_ue->dlsch_rate_unmatching_stats;
  time_stats_t *dlsch_turbo_decoding_stats=&phy_vars_ue->dlsch_turbo_decoding_stats;
  time_stats_t *dlsch_deinterleaving_stats=&phy_vars_ue->dlsch_deinterleaving_stats;
#endif
  NR_DL_UE_HARQ_t *harq_process= rdata->harq_process;
  t_nrLDPC_dec_params *p_decoderParms = &rdata->decoderParms;
  int length_dec;
  int no_iteration_ldpc;
  int Kr;
  int Kr_bytes;
  int K_bits_F;
  uint8_t crc_type;
  int i;
  int j;
  int r = rdata->segment_r;
  int A = rdata->A;
  int E = rdata->E;
  int Qm = rdata->Qm;
  //int rv_index = rdata->rv_index;
  int r_offset = rdata->r_offset;
  uint8_t kc = rdata->Kc;
  uint32_t Tbslbrm = rdata->Tbslbrm;
  short* dlsch_llr = rdata->dlsch_llr;
  rdata->decodeIterations = dlsch->max_ldpc_iterations + 1;
  int8_t llrProcBuf[OAI_UL_LDPC_MAX_NUM_LLR] __attribute__ ((aligned(32)));

  int16_t  z [68*384 + 16] __attribute__ ((aligned(16)));
  int8_t   l [68*384 + 16] __attribute__ ((aligned(16)));

  __m128i *pv = (__m128i*)&z;
  __m128i *pl = (__m128i*)&l;

  uint8_t  Ilbrm    = 0;

  Kr = harq_process->K; // [hna] overwrites this line "Kr = p_decParams->Z*kb"
  Kr_bytes = Kr>>3;
  K_bits_F = Kr-harq_process->F;

  t_nrLDPC_time_stats procTime = {0};
  t_nrLDPC_time_stats* p_procTime     = &procTime ;

  t_nrLDPC_procBuf **p_nrLDPC_procBuf = harq_process->p_nrLDPC_procBuf;

#if UE_TIMING_TRACE
    start_meas(dlsch_deinterleaving_stats);
#endif
    //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_DEINTERLEAVING, VCD_FUNCTION_IN);
    nr_deinterleaving_ldpc(E,
                           Qm,
                           harq_process->w[r], // [hna] w is e
                           dlsch_llr+r_offset);
    //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_DEINTERLEAVING, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
    stop_meas(dlsch_deinterleaving_stats);
#endif
#if UE_TIMING_TRACE
    start_meas(dlsch_rate_unmatching_stats);
#endif
    /* LOG_D(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,E %d, F %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
          harq_pid,r, G,E,harq_process->F,
          Kr*3,
          harq_process->TBS,
          Qm,
          harq_process->nb_rb,
          harq_process->Nl,
          harq_process->rvidx,
          harq_process->round); */
    //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_RATE_MATCHING, VCD_FUNCTION_IN);

    if (nr_rate_matching_ldpc_rx(Ilbrm,
                                 Tbslbrm,
                                 p_decoderParms->BG,
                                 p_decoderParms->Z,
                                 harq_process->d[r],
                                 harq_process->w[r],
                                 harq_process->C,
                                 harq_process->rvidx,
                                 (harq_process->first_rx==1)?1:0,
                                 E,
                                 harq_process->F,
                                 Kr-harq_process->F-2*(p_decoderParms->Z))==-1) {
      //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_RATE_MATCHING, VCD_FUNCTION_OUT);
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
      LOG_E(PHY,"dlsch_decoding.c: Problem in rate_matching\n");
      rdata->decodeIterations = dlsch->max_ldpc_iterations + 1;
	  return;
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

    {
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

      //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_LDPC, VCD_FUNCTION_IN);
      p_decoderParms->block_length=length_dec;
      nrLDPC_initcall(p_decoderParms, (int8_t*)&pl[0], llrProcBuf);
      no_iteration_ldpc = nrLDPC_decoder(p_decoderParms,
                                         (int8_t *)&pl[0],
                                         llrProcBuf,
                                         p_nrLDPC_procBuf[r],
                                         p_procTime);
      //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_LDPC, VCD_FUNCTION_OUT);

      // Fixme: correct type is unsigned, but nrLDPC_decoder and all called behind use signed int
      if (check_crc((uint8_t *)llrProcBuf,length_dec,harq_process->F,crc_type)) {
        LOG_D(PHY,"Segment %u CRC OK\n\033[0m",r);

        if (r==0) {
          for (int i=0; i<10; i++) LOG_D(PHY,"byte %d : %x\n",i,((uint8_t *)llrProcBuf)[i]);
        }

        //Temporary hack
        no_iteration_ldpc = dlsch->max_ldpc_iterations;
        rdata->decodeIterations = no_iteration_ldpc;
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

#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
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
  uint32_t A,E;
  uint32_t G;
  uint32_t ret,offset;
  uint32_t r,r_offset=0,Kr=8424,Kr_bytes;
  t_nrLDPC_dec_params decParams;
  t_nrLDPC_dec_params *p_decParams = &decParams;

  if (!harq_process) {
    LOG_E(PHY,"dlsch_decoding.c: NULL harq_process pointer\n");
    return(dlsch->max_ldpc_iterations + 1);
  }

  // HARQ stats
  phy_vars_ue->dl_stats[harq_process->round]++;
  uint8_t kc;
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
  vcd_signal_dumper_dump_function_by_name(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_SEGMENTATION, VCD_FUNCTION_IN);

  //NR_DL_UE_HARQ_t *harq_process = dlsch->harq_processes[0];
  
  int nbDecode = 0;
  
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
  if ((harq_process->Nl)<4)
    Tbslbrm = nr_compute_tbslbrm(harq_process->mcs_table,nb_rb,harq_process->Nl);
  else
    Tbslbrm = nr_compute_tbslbrm(harq_process->mcs_table,nb_rb,4);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_SEGMENTATION, VCD_FUNCTION_OUT);
  p_decParams->Z = harq_process->Z;
  //printf("dlsch decoding nr segmentation Z %d\n", p_decParams->Z);
  //printf("coderate %f kc %d \n", Coderate, kc);
  p_decParams->numMaxIter = dlsch->max_ldpc_iterations;
  p_decParams->outMode= 0;
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

  Kr = harq_process->K; // [hna] overwrites this line "Kr = p_decParams->Z*kb"
  Kr_bytes = Kr>>3;
  offset = 0;
  void (*nr_processDLSegment_ptr)(void*) = &nr_processDLSegment;
  notifiedFIFO_t nf;
  initNotifiedFIFO(&nf);
  for (r=0; r<harq_process->C; r++) {
    //printf("start rx segment %d\n",r);
    E = nr_get_E(G, harq_process->C, harq_process->Qm, harq_process->Nl, r);
    union ldpcReqUnion id = {.s={dlsch->rnti,frame,nr_slot_rx,0,0}};
    notifiedFIFO_elt_t *req=newNotifiedFIFO_elt(sizeof(ldpcDecode_ue_t), id.p, &nf, nr_processDLSegment_ptr);
    ldpcDecode_ue_t * rdata=(ldpcDecode_ue_t *) NotifiedFifoData(req);

    rdata->phy_vars_ue = phy_vars_ue;
    rdata->harq_process = harq_process;
    rdata->decoderParms = decParams;
    rdata->dlsch_llr = dlsch_llr;
    rdata->Kc = kc;
    rdata->harq_pid = harq_pid;
    rdata->segment_r = r;
    rdata->nbSegments = harq_process->C;
    rdata->E = E;
    rdata->A = A;
    rdata->Qm = harq_process->Qm;
    rdata->r_offset = r_offset;
    rdata->Kr_bytes = Kr_bytes;
    rdata->rv_index = harq_process->rvidx;
    rdata->Tbslbrm = Tbslbrm;
    rdata->offset = offset;
    rdata->dlsch = dlsch;
    rdata->dlsch_id = 0;
    pushTpool(&(pool_dl),req);
    nbDecode++;
    LOG_D(PHY,"Added a block to decode, in pipe: %d\n",nbDecode);
    r_offset += E;
    offset += (Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0));
    //////////////////////////////////////////////////////////////////////////////////////////
  }
  for (r=0; r<nbDecode; r++) {
    notifiedFIFO_elt_t *req=pullTpool(&nf, &(pool_dl));
    bool last = false;
    if (r == nbDecode - 1)
      last = true;
    bool stop = nr_ue_postDecode(phy_vars_ue, req, last, &nf);
    delNotifiedFIFO_elt(req);
    if (stop)
      break;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_COMBINE_SEG, VCD_FUNCTION_OUT);
  ret = dlsch->last_iteration_cnt;
  return(ret);
}
