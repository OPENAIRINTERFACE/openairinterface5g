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

/*! \file PHY/NR_TRANSPORT/nr_ulsch_decoding.c
* \brief Top-level routines for decoding  LDPC (ULSCH) transport channels from 38.212, V15.4.0 2018-12
* \author Ahmed Hussein
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: ahmed.hussein@iis.fraunhofer.de
* \note
* \warning
*/


// [from gNB coding]
#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "SCHED_NR/sched_nr.h"
#include "defs.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/LOG/log.h"
#include <syscall.h>
//#define DEBUG_ULSCH_DECODING
//#define gNB_DEBUG_TRACE

#define OAI_UL_LDPC_MAX_NUM_LLR 27000//26112 // NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX = 68*384
//#define PRINT_CRC_CHECK

//extern double cpuf;

void free_gNB_ulsch(NR_gNB_ULSCH_t **ulschptr,uint8_t N_RB_UL)
{

  int i,r;
  uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS;  //number of segments to be allocated
  NR_gNB_ULSCH_t *ulsch = *ulschptr;

  if (ulsch) {
    if (N_RB_UL != 273) {
      a_segments = a_segments*N_RB_UL;
      a_segments = a_segments/273 +1;
    }  


    for (i=0; i<NR_MAX_ULSCH_HARQ_PROCESSES; i++) {

      if (ulsch->harq_processes[i]) {
        if (ulsch->harq_processes[i]->b) {
          free16(ulsch->harq_processes[i]->b,a_segments*1056);
          ulsch->harq_processes[i]->b = NULL;
        }
        for (r=0; r<a_segments; r++) {
          free16(ulsch->harq_processes[i]->c[r],(8448)*sizeof(uint8_t));
          ulsch->harq_processes[i]->c[r] = NULL;
        }
        for (r=0; r<a_segments; r++) {
          if (ulsch->harq_processes[i]->d[r]) {
            free16(ulsch->harq_processes[i]->d[r],(68*384)*sizeof(int16_t));
            ulsch->harq_processes[i]->d[r] = NULL;
          }
        }
        for (r=0; r<a_segments; r++) {
          if (ulsch->harq_processes[i]->w[r]) {
            free16(ulsch->harq_processes[i]->w[r],(3*(6144+64))*sizeof(int16_t));
            ulsch->harq_processes[i]->w[r] = NULL;
          }
        }
        for (r=0; r<a_segments; r++) {
          if (ulsch->harq_processes[i]->p_nrLDPC_procBuf[r]){
            nrLDPC_free_mem(ulsch->harq_processes[i]->p_nrLDPC_procBuf[r]);
            ulsch->harq_processes[i]->p_nrLDPC_procBuf[r] = NULL;
          }
        }
        free16(ulsch->harq_processes[i],sizeof(NR_UL_gNB_HARQ_t));
        ulsch->harq_processes[i] = NULL;
      }
    }
    free16(ulsch,sizeof(NR_gNB_ULSCH_t));
    *ulschptr = NULL;
  }
}


NR_gNB_ULSCH_t *new_gNB_ulsch(uint8_t max_ldpc_iterations,uint16_t N_RB_UL, uint8_t abstraction_flag)
{

  NR_gNB_ULSCH_t *ulsch;
  uint8_t i,r;
  uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS;  //number of segments to be allocated

  if (N_RB_UL != 273) {
    a_segments = a_segments*N_RB_UL;
    a_segments = a_segments/273 +1;
  }

  uint32_t ulsch_bytes = a_segments*1056;  // allocated bytes per segment
  ulsch = (NR_gNB_ULSCH_t *)malloc16_clear(sizeof(NR_gNB_ULSCH_t));
  
  ulsch->max_ldpc_iterations = max_ldpc_iterations;
  ulsch->Mlimit = 4;
  
  for (i=0; i<NR_MAX_ULSCH_HARQ_PROCESSES; i++) {
    
    ulsch->harq_processes[i] = (NR_UL_gNB_HARQ_t *)malloc16_clear(sizeof(NR_UL_gNB_HARQ_t));
    ulsch->harq_processes[i]->b = (uint8_t*)malloc16_clear(ulsch_bytes);
    if (abstraction_flag == 0) {
      for (r=0; r<a_segments; r++) {
	ulsch->harq_processes[i]->p_nrLDPC_procBuf[r] = nrLDPC_init_mem();
	ulsch->harq_processes[i]->c[r] = (uint8_t*)malloc16_clear(8448*sizeof(uint8_t));
	ulsch->harq_processes[i]->d[r] = (int16_t*)malloc16_clear((68*384)*sizeof(int16_t));
	ulsch->harq_processes[i]->w[r] = (int16_t*)malloc16_clear((3*(6144+64))*sizeof(int16_t));
      }
    }
  }
  
  return(ulsch);
}

void clean_gNB_ulsch(NR_gNB_ULSCH_t *ulsch)
{
  unsigned char i, j;

  if (ulsch) {
    ulsch->harq_mask = 0;
    ulsch->bundling = 0;
    ulsch->beta_offset_cqi_times8 = 0;
    ulsch->beta_offset_ri_times8 = 0;
    ulsch->beta_offset_harqack_times8 = 0;
    ulsch->Msg3_active = 0;
    ulsch->Msg3_flag = 0;
    ulsch->Msg3_subframe = 0;
    ulsch->Msg3_frame = 0;
    ulsch->rnti = 0;
    ulsch->rnti_type = 0;
    ulsch->cyclicShift = 0;
    ulsch->cooperation_flag = 0;
    ulsch->Mlimit = 0;
    ulsch->max_ldpc_iterations = 0;
    ulsch->last_iteration_cnt = 0;
    for (i=0;i<NR_MAX_SLOTS_PER_FRAME;i++) ulsch->harq_process_id[i] = 0;

    for (i=0; i<NR_MAX_ULSCH_HARQ_PROCESSES; i++) {
      if (ulsch->harq_processes[i]){
        /// Nfapi ULSCH PDU
        //nfapi_nr_ul_config_ulsch_pdu ulsch_pdu;
        ulsch->harq_processes[i]->frame=0;
        ulsch->harq_processes[i]->slot=0;
        ulsch->harq_processes[i]->round=0;
        ulsch->harq_processes[i]->TPC=0;
        ulsch->harq_processes[i]->mimo_mode=0;
        ulsch->harq_processes[i]->dci_alloc=0;
        ulsch->harq_processes[i]->rar_alloc=0;
        ulsch->harq_processes[i]->status=NR_SCH_IDLE;
        ulsch->harq_processes[i]->subframe_scheduling_flag=0;
        ulsch->harq_processes[i]->previous_first_rb=0;
        ulsch->harq_processes[i]->handled=0;
        ulsch->harq_processes[i]->delta_TF=0;

        ulsch->harq_processes[i]->TBS=0;
        /// Pointer to the payload (38.212 V15.4.0 section 5.1)
        //uint8_t *b;
        ulsch->harq_processes[i]->B=0;
        /// Pointers to code blocks after code block segmentation and CRC attachment (38.212 V15.4.0 section 5.2.2)
        //uint8_t *c[MAX_NUM_NR_ULSCH_SEGMENTS];
        ulsch->harq_processes[i]->K=0;
        ulsch->harq_processes[i]->F=0;
        ulsch->harq_processes[i]->C=0;
        /// Pointers to code blocks after LDPC coding (38.212 V15.4.0 section 5.3.2)
        //int16_t *d[MAX_NUM_NR_ULSCH_SEGMENTS];
        /// LDPC processing buffer
        //t_nrLDPC_procBuf* p_nrLDPC_procBuf[MAX_NUM_NR_ULSCH_SEGMENTS];
        ulsch->harq_processes[i]->Z=0;
        /// code blocks after bit selection in rate matching for LDPC code (38.212 V15.4.0 section 5.4.2.1)
        //int16_t e[MAX_NUM_NR_DLSCH_SEGMENTS][3*8448];
        ulsch->harq_processes[i]->E=0;


        ulsch->harq_processes[i]->n_DMRS=0;
        ulsch->harq_processes[i]->n_DMRS2=0;
        ulsch->harq_processes[i]->previous_n_DMRS=0;


        ulsch->harq_processes[i]->cqi_crc_status=0;
        for (j=0;j<MAX_CQI_BYTES;j++) ulsch->harq_processes[i]->o[j]=0;
        ulsch->harq_processes[i]->uci_format=0;
        ulsch->harq_processes[i]->Or1=0;
        ulsch->harq_processes[i]->Or2=0;
        ulsch->harq_processes[i]->o_RI[0]=0; ulsch->harq_processes[i]->o_RI[1]=0;
        ulsch->harq_processes[i]->O_RI=0;
        ulsch->harq_processes[i]->o_ACK[0]=0; ulsch->harq_processes[i]->o_ACK[1]=0;
        ulsch->harq_processes[i]->o_ACK[2]=0; ulsch->harq_processes[i]->o_ACK[3]=0;
        ulsch->harq_processes[i]->O_ACK=0;
        ulsch->harq_processes[i]->V_UL_DAI=0;
        /// "q" sequences for CQI/PMI (for definition see 36-212 V8.6 2009-03, p.27)
        //int8_t q[MAX_CQI_PAYLOAD];
        ulsch->harq_processes[i]->o_RCC=0;
        /// coded and interleaved CQI bits
        //int8_t o_w[(MAX_CQI_BITS+8)*3];
        /// coded CQI bits
        //int8_t o_d[96+((MAX_CQI_BITS+8)*3)];
        for (j=0;j<MAX_ACK_PAYLOAD;j++) ulsch->harq_processes[i]->q_ACK[j]=0;
        for (j=0;j<MAX_RI_PAYLOAD;j++) ulsch->harq_processes[i]->q_RI[j]=0;
        /// Temporary h sequence to flag PUSCH_x/PUSCH_y symbols which are not scrambled
        //uint8_t h[MAX_NUM_CHANNEL_BITS];
        /// soft bits for each received segment ("w"-sequence)(for definition see 36-212 V8.6 2009-03, p.15)
        //int16_t w[MAX_NUM_NR_ULSCH_SEGMENTS][3*(6144+64)];
      }
    }
  }
}

#ifdef PRINT_CRC_CHECK
  static uint32_t prnt_crc_cnt = 0;
#endif

void nr_processULSegment(void* arg) {
  ldpcDecode_t *rdata = (ldpcDecode_t*) arg;
  PHY_VARS_gNB *phy_vars_gNB = rdata->gNB;
  NR_UL_gNB_HARQ_t *ulsch_harq = rdata->ulsch_harq;
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
  int rv_index = rdata->rv_index;
  int r_offset = rdata->r_offset;
  uint8_t kc = rdata->Kc;
  uint32_t Tbslbrm = rdata->Tbslbrm;
  short* ulsch_llr = rdata->ulsch_llr;
  int max_ldpc_iterations = p_decoderParms->numMaxIter;
  int8_t llrProcBuf[OAI_UL_LDPC_MAX_NUM_LLR] __attribute__ ((aligned(32)));

  int16_t  z [68*384 + 16] __attribute__ ((aligned(16)));
  int8_t   l [68*384 + 16] __attribute__ ((aligned(16)));

  __m128i *pv = (__m128i*)&z;
  __m128i *pl = (__m128i*)&l;
  
  uint8_t  Ilbrm    = 0;

  Kr = ulsch_harq->K;
  Kr_bytes = Kr>>3;
  K_bits_F = Kr-ulsch_harq->F;

  t_nrLDPC_time_stats procTime = {0};
  t_nrLDPC_time_stats* p_procTime     = &procTime ;

  //start_meas(&phy_vars_gNB->ulsch_deinterleaving_stats);

  ////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////// nr_deinterleaving_ldpc ///////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////////// ulsch_llr =====> ulsch_harq->e //////////////////////////////

  nr_deinterleaving_ldpc(E,
                         Qm,
                         ulsch_harq->e[r],
                         ulsch_llr+r_offset);

  //for (int i =0; i<16; i++)
  //          printf("rx output deinterleaving w[%d]= %d r_offset %d\n", i,ulsch_harq->w[r][i], r_offset);

  stop_meas(&phy_vars_gNB->ulsch_deinterleaving_stats);


  /*LOG_D(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
        harq_pid,r, G,
        Kr*3,
        ulsch_harq->TBS,
        Qm,
        nb_rb,
        n_layers,
        pusch_pdu->pusch_data.rv_index,
        ulsch_harq->round);*/
  //////////////////////////////////////////////////////////////////////////////////////////


  //////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////// nr_rate_matching_ldpc_rx ////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////

  ///////////////////////// ulsch_harq->e =====> ulsch_harq->d /////////////////////////

  //start_meas(&phy_vars_gNB->ulsch_rate_unmatching_stats);

  if (nr_rate_matching_ldpc_rx(Ilbrm,
                               Tbslbrm,
                               p_decoderParms->BG,
                               p_decoderParms->Z,
                               ulsch_harq->d[r],
                               ulsch_harq->e[r],
                               ulsch_harq->C,
                               rv_index,
                               ulsch_harq->new_rx,
                               E,
       ulsch_harq->F,
       Kr-ulsch_harq->F-2*(p_decoderParms->Z))==-1) {

    stop_meas(&phy_vars_gNB->ulsch_rate_unmatching_stats);

    LOG_E(PHY,"ulsch_decoding.c: Problem in rate_matching\n");
    rdata->decodeIterations = max_ldpc_iterations + 1;
    return;
  } else {
    stop_meas(&phy_vars_gNB->ulsch_rate_unmatching_stats);
  }

  memset(ulsch_harq->c[r],0,Kr_bytes);

  if (ulsch_harq->C == 1) {
    if (A > 3824)
      crc_type = CRC24_A;
    else
      crc_type = CRC16;

    length_dec = ulsch_harq->B;
  }
  else {
    crc_type = CRC24_B;
    length_dec = (ulsch_harq->B+24*ulsch_harq->C)/ulsch_harq->C;
  }

  //start_meas(&phy_vars_gNB->ulsch_ldpc_decoding_stats);

  //set first 2*Z_c bits to zeros
  memset(&z[0],0,2*ulsch_harq->Z*sizeof(int16_t));
  //set Filler bits
  memset((&z[0]+K_bits_F),127,ulsch_harq->F*sizeof(int16_t));
  //Move coded bits before filler bits
  memcpy((&z[0]+2*ulsch_harq->Z),ulsch_harq->d[r],(K_bits_F-2*ulsch_harq->Z)*sizeof(int16_t));
  //skip filler bits
  memcpy((&z[0]+Kr),ulsch_harq->d[r]+(Kr-2*ulsch_harq->Z),(kc*ulsch_harq->Z-Kr)*sizeof(int16_t));
  //Saturate coded bits before decoding into 8 bits values
  for (i=0, j=0; j < ((kc*ulsch_harq->Z)>>4)+1;  i+=2, j++)
  {
    pl[j] = _mm_packs_epi16(pv[i],pv[i+1]);
  }
  //////////////////////////////////////////////////////////////////////////////////////////


  //////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////// nrLDPC_decoder /////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////// pl =====> llrProcBuf //////////////////////////////////

  no_iteration_ldpc = nrLDPC_decoder(p_decoderParms,
                                     (int8_t*)&pl[0],
                                     llrProcBuf,
                                     ulsch_harq->p_nrLDPC_procBuf[r],
                                     p_procTime);

  if (check_crc((uint8_t*)llrProcBuf,length_dec,ulsch_harq->F,crc_type)) {
#ifdef PRINT_CRC_CHECK
      LOG_I(PHY, "Segment %d CRC OK, iterations %d/%d\n",r,no_iteration_ldpc,max_ldpc_iterations);
#endif
    rdata->decodeIterations = no_iteration_ldpc;
    if (rdata->decodeIterations > p_decoderParms->numMaxIter) rdata->decodeIterations--;
  } else {
#ifdef PRINT_CRC_CHECK
      LOG_I(PHY, "CRC NOK\n");
#endif
    rdata->decodeIterations = max_ldpc_iterations + 1;
  }

  for (int m=0; m < Kr>>3; m ++) {
    ulsch_harq->c[r][m]= (uint8_t) llrProcBuf[m];
  }

  //stop_meas(&phy_vars_gNB->ulsch_ldpc_decoding_stats);
}

uint32_t nr_ulsch_decoding(PHY_VARS_gNB *phy_vars_gNB,
                           uint8_t ULSCH_id,
                           short *ulsch_llr,
                           NR_DL_FRAME_PARMS *frame_parms,
                           nfapi_nr_pusch_pdu_t *pusch_pdu,
                           uint32_t frame,
                           uint8_t nr_tti_rx,
                           uint8_t harq_pid,
                           uint32_t G) {

  uint32_t A;
  uint32_t r;
  uint32_t r_offset;
  uint32_t offset;
  int kc;
  int Tbslbrm;
  int E;

#ifdef PRINT_CRC_CHECK
  prnt_crc_cnt++;
#endif
  

  NR_gNB_ULSCH_t                       *ulsch                 = phy_vars_gNB->ulsch[ULSCH_id][0];
  NR_gNB_PUSCH                         *pusch                 = phy_vars_gNB->pusch_vars[ULSCH_id];
  NR_UL_gNB_HARQ_t                     *harq_process          = ulsch->harq_processes[harq_pid];

  if (!harq_process) {
    LOG_E(PHY,"ulsch_decoding.c: NULL harq_process pointer\n");
    return 1;
  }

  t_nrLDPC_dec_params decParams;
  t_nrLDPC_dec_params* p_decParams    = &decParams;

  int Kr;
  int Kr_bytes;
    
  phy_vars_gNB->nbDecode = 0;
  harq_process->processedSegments = 0;

  double   Coderate = 0.0;
  
  // ------------------------------------------------------------------
  uint16_t nb_rb          = pusch_pdu->rb_size;
  uint8_t Qm              = pusch_pdu->qam_mod_order;
  uint16_t R              = pusch_pdu->target_code_rate;
  uint8_t mcs             = pusch_pdu->mcs_index;
  uint8_t n_layers        = pusch_pdu->nrOfLayers;
  // ------------------------------------------------------------------

   if (!ulsch_llr) {
    LOG_E(PHY,"ulsch_decoding.c: NULL ulsch_llr pointer\n");
    return 1;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_gNB_ULSCH_DECODING,1);
  harq_process->TBS = pusch_pdu->pusch_data.tb_size;
  harq_process->round = nr_rv_to_round(pusch_pdu->pusch_data.rv_index);

  harq_process->new_rx = false; // flag to indicate if this is a new reception for this harq (initialized to false)
  if (harq_process->round == 0) {
    harq_process->new_rx = true;
    harq_process->ndi = pusch_pdu->pusch_data.new_data_indicator;
  }

  // this happens if there was a DTX in round 0
  if (harq_process->ndi != pusch_pdu->pusch_data.new_data_indicator) {
    harq_process->new_rx = true;
    harq_process->ndi = pusch_pdu->pusch_data.new_data_indicator;
    LOG_E(PHY,"Missed ULSCH detection. NDI toggled but rv %d does not correspond to first reception\n",pusch_pdu->pusch_data.rv_index);
  }

  A   = (harq_process->TBS)<<3;

  LOG_D(NR_PHY, "ULSCH Decoding, harq_pid %d TBS %d G %d mcs %d Nl %d nb_rb %d, Qm %d, n_layers %d, Coderate %d\n", harq_pid, A, G, mcs, n_layers, nb_rb, Qm, n_layers, R);

  if (R<1024)
    Coderate = (float) R /(float) 1024;
  else
    Coderate = (float) R /(float) 2048;
  
  if ((A <=292) || ((A<=3824) && (Coderate <= 0.6667)) || Coderate <= 0.25){
    p_decParams->BG = 2;
    kc = 52;
    if (Coderate < 0.3333) {
      p_decParams->R = 15;
    }
    else if (Coderate <0.6667) {
      p_decParams->R = 13;
    }
    else {
      p_decParams->R = 23;
    }
  } else {
    p_decParams->BG = 1;
    kc = 68;
    if (Coderate < 0.6667) {
      p_decParams->R = 13;
    }
    else if (Coderate <0.8889) {
      p_decParams->R = 23;
    }
    else {
      p_decParams->R = 89;
    }
  }
  
  NR_gNB_SCH_STATS_t *stats=NULL;
  int first_free=-1;
  for (int i=0;i<NUMBER_OF_NR_SCH_STATS_MAX;i++) {
    if (phy_vars_gNB->ulsch_stats[i].rnti == 0 && first_free == -1) {
      first_free = i;
      stats=&phy_vars_gNB->ulsch_stats[i];
    }
    if (phy_vars_gNB->ulsch_stats[i].rnti == ulsch->rnti) {
      stats=&phy_vars_gNB->ulsch_stats[i];
      break;
    }
  }
  if (stats) {
    stats->frame = frame;
    stats->rnti = ulsch->rnti;
    stats->round_trials[harq_process->round]++;
    for (int aarx=0;aarx<frame_parms->nb_antennas_rx;aarx++) {
       stats->power[aarx]=dB_fixed_x10(pusch->ulsch_power[aarx]);
       stats->noise_power[aarx]=dB_fixed_x10(pusch->ulsch_noise_power[aarx]);
    }
    if (harq_process->new_rx == 0) {
      stats->current_Qm = Qm;
      stats->current_RI = n_layers;
      stats->total_bytes_tx += harq_process->TBS;
    }
  }
  if (A > 3824)
    harq_process->B = A+24;
  else
    harq_process->B = A+16;

// [hna] Perform nr_segmenation with input and output set to NULL to calculate only (B, C, K, Z, F)
  nr_segmentation(NULL,
                  NULL,
                  harq_process->B,
                  &harq_process->C,
                  &harq_process->K,
                  &harq_process->Z, // [hna] Z is Zc
                  &harq_process->F,
                  p_decParams->BG);

#ifdef DEBUG_ULSCH_DECODING
  printf("ulsch decoding nr segmentation Z %d\n", harq_process->Z);
  if (!frame%100)
    printf("K %d C %d Z %d \n", harq_process->K, harq_process->C, harq_process->Z);
#endif
  Tbslbrm = nr_compute_tbslbrm(0,nb_rb,n_layers);

  p_decParams->Z = harq_process->Z;


  p_decParams->numMaxIter = ulsch->max_ldpc_iterations;
  p_decParams->outMode= 0;

  r_offset = 0;

  uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS;  //number of segments to be allocated

  if (nb_rb != 273) {
    a_segments = a_segments*nb_rb;
    a_segments = a_segments/273 +1;
  }

  if (harq_process->C > a_segments) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,a_segments);
    return 1;
  }
#ifdef DEBUG_ULSCH_DECODING
  printf("Segmentation: C %d, K %d\n",harq_process->C,harq_process->K);
#endif
  Kr = harq_process->K;
  Kr_bytes = Kr>>3;
  offset = 0;
  void (*nr_processULSegment_ptr)(void*) = &nr_processULSegment;

  for (r=0; r<harq_process->C; r++) {

    E = nr_get_E(G, harq_process->C, Qm, n_layers, r);

    union ldpcReqUnion id = {.s={ulsch->rnti,frame,nr_tti_rx,0,0}};
    notifiedFIFO_elt_t *req=newNotifiedFIFO_elt(sizeof(ldpcDecode_t), id.p, phy_vars_gNB->respDecode, nr_processULSegment_ptr);
    ldpcDecode_t * rdata=(ldpcDecode_t *) NotifiedFifoData(req);

    rdata->gNB = phy_vars_gNB;
    rdata->ulsch_harq = harq_process;
    rdata->decoderParms = decParams;
    rdata->ulsch_llr = ulsch_llr;
    rdata->Kc = kc;
    rdata->harq_pid = harq_pid;
    rdata->segment_r = r;
    rdata->nbSegments = harq_process->C;
    rdata->E = E;
    rdata->A = A;
    rdata->Qm = Qm;
    rdata->r_offset = r_offset;
    rdata->Kr_bytes = Kr_bytes;
    rdata->rv_index = pusch_pdu->pusch_data.rv_index;
    rdata->Tbslbrm = Tbslbrm;
    rdata->offset = offset;
    rdata->ulsch = ulsch;
    rdata->ulsch_id = ULSCH_id;
    pushTpool(phy_vars_gNB->threadPool,req);
    phy_vars_gNB->nbDecode++;
    LOG_D(PHY,"Added a block to decode, in pipe: %d\n",phy_vars_gNB->nbDecode);
    r_offset += E;
    offset += (Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0));
    //////////////////////////////////////////////////////////////////////////////////////////
  }
  return 1;
}
