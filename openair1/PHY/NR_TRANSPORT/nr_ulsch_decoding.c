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
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "SCHED_NR/sched_nr.h"
#include "SCHED_NR/fapi_nr_l1.h"
#include "defs.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/LOG/log.h"
#include <syscall.h>
//#define DEBUG_ULSCH_DECODING
//#define gNB_DEBUG_TRACE

#define OAI_UL_LDPC_MAX_NUM_LLR 27000//26112 // NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX = 68*384
//#define DEBUG_CRC
#ifdef DEBUG_CRC
#define PRINT_CRC_CHECK(a) a
#else
#define PRINT_CRC_CHECK(a)
#endif

//extern double cpuf;

void free_gNB_ulsch(NR_gNB_ULSCH_t *ulsch, uint16_t N_RB_UL)
{

  uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER*NR_MAX_NB_LAYERS;  //number of segments to be allocated

  if (N_RB_UL != 273) {
    a_segments = a_segments*N_RB_UL;
    a_segments = a_segments/273 +1;
  }

  if (ulsch->harq_process) {
    if (ulsch->harq_process->b) {
      free_and_zero(ulsch->harq_process->b);
      ulsch->harq_process->b = NULL;
    }
    for (int r = 0; r < a_segments; r++) {
      free_and_zero(ulsch->harq_process->c[r]);
      free_and_zero(ulsch->harq_process->d[r]);
    }
    free_and_zero(ulsch->harq_process->c);
    free_and_zero(ulsch->harq_process->d);
    free_and_zero(ulsch->harq_process->d_to_be_cleared);
    free_and_zero(ulsch->harq_process);
    ulsch->harq_process = NULL;
  }
}

NR_gNB_ULSCH_t new_gNB_ulsch(uint8_t max_ldpc_iterations, uint16_t N_RB_UL)
{

  uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER*NR_MAX_NB_LAYERS;  //number of segments to be allocated

  if (N_RB_UL != 273) {
    a_segments = a_segments*N_RB_UL;
    a_segments = a_segments/273 +1;
  }

  uint32_t ulsch_bytes = a_segments * 1056; // allocated bytes per segment
  NR_gNB_ULSCH_t ulsch = {0};

  ulsch.max_ldpc_iterations = max_ldpc_iterations;
  ulsch.harq_pid = -1;
  ulsch.active = false;

  NR_UL_gNB_HARQ_t *harq = malloc16_clear(sizeof(*harq));
  init_abort(&harq->abort_decode);
  ulsch.harq_process = harq;
  harq->b = malloc16_clear(ulsch_bytes * sizeof(*harq->b));
  harq->c = malloc16_clear(a_segments * sizeof(*harq->c));
  harq->d = malloc16_clear(a_segments * sizeof(*harq->d));
  for (int r = 0; r < a_segments; r++) {
    harq->c[r] = malloc16_clear(8448 * sizeof(*harq->c[r]));
    harq->d[r] = malloc16_clear(68 * 384 * sizeof(*harq->d[r]));
  }
  harq->d_to_be_cleared = calloc(a_segments, sizeof(bool));
  AssertFatal(harq->d_to_be_cleared != NULL, "out of memory\n");
  return(ulsch);
}

static void nr_processULSegment(void *arg)
{
  ldpcDecode_t *rdata = (ldpcDecode_t *)arg;
  PHY_VARS_gNB *phy_vars_gNB = rdata->gNB;
  NR_UL_gNB_HARQ_t *ulsch_harq = rdata->ulsch_harq;
  t_nrLDPC_dec_params *p_decoderParms = &rdata->decoderParms;
  int length_dec;
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
  short *ulsch_llr = rdata->ulsch_llr;
  int max_ldpc_iterations = p_decoderParms->numMaxIter;
  int8_t llrProcBuf[OAI_UL_LDPC_MAX_NUM_LLR] __attribute__((aligned(32)));

  int16_t z[68 * 384 + 16] __attribute__((aligned(16)));
  int8_t l[68 * 384 + 16] __attribute__((aligned(16)));

  __m128i *pv = (__m128i *)&z;
  __m128i *pl = (__m128i *)&l;

  Kr = ulsch_harq->K;
  Kr_bytes = Kr >> 3;
  K_bits_F = Kr - ulsch_harq->F;

  t_nrLDPC_time_stats procTime = {0};
  t_nrLDPC_time_stats *p_procTime = &procTime;

  // start_meas(&phy_vars_gNB->ulsch_deinterleaving_stats);

  ////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////// nr_deinterleaving_ldpc ///////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////////// ulsch_llr =====> ulsch_harq->e //////////////////////////////

  /// code blocks after bit selection in rate matching for LDPC code (38.212 V15.4.0 section 5.4.2.1)
  int16_t harq_e[E];

  nr_deinterleaving_ldpc(E, Qm, harq_e, ulsch_llr + r_offset);

  // for (int i =0; i<16; i++)
  //          printf("rx output deinterleaving w[%d]= %d r_offset %d\n", i,ulsch_harq->w[r][i], r_offset);

  stop_meas(&phy_vars_gNB->ulsch_deinterleaving_stats);

  //////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////// nr_rate_matching_ldpc_rx ////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////

  ///////////////////////// ulsch_harq->e =====> ulsch_harq->d /////////////////////////

  // start_meas(&phy_vars_gNB->ulsch_rate_unmatching_stats);

  if (nr_rate_matching_ldpc_rx(rdata->tbslbrm,
                               p_decoderParms->BG,
                               p_decoderParms->Z,
                               ulsch_harq->d[r],
                               harq_e,
                               ulsch_harq->C,
                               rv_index,
                               ulsch_harq->d_to_be_cleared[r],
                               E,
                               ulsch_harq->F,
                               Kr - ulsch_harq->F - 2 * (p_decoderParms->Z))
      == -1) {
    stop_meas(&phy_vars_gNB->ulsch_rate_unmatching_stats);

    LOG_E(PHY, "ulsch_decoding.c: Problem in rate_matching\n");
    rdata->decodeIterations = max_ldpc_iterations + 1;
    return;
  } else {
    stop_meas(&phy_vars_gNB->ulsch_rate_unmatching_stats);
  }

  ulsch_harq->d_to_be_cleared[r] = false;

  memset(ulsch_harq->c[r], 0, Kr_bytes);

  if (ulsch_harq->C == 1) {
    if (A > 3824)
      crc_type = CRC24_A;
    else
      crc_type = CRC16;

    length_dec = ulsch_harq->B;
  } else {
    crc_type = CRC24_B;
    length_dec = (ulsch_harq->B + 24 * ulsch_harq->C) / ulsch_harq->C;
  }

  // start_meas(&phy_vars_gNB->ulsch_ldpc_decoding_stats);

  // set first 2*Z_c bits to zeros
  memset(&z[0], 0, 2 * ulsch_harq->Z * sizeof(int16_t));
  // set Filler bits
  memset((&z[0] + K_bits_F), 127, ulsch_harq->F * sizeof(int16_t));
  // Move coded bits before filler bits
  memcpy((&z[0] + 2 * ulsch_harq->Z), ulsch_harq->d[r], (K_bits_F - 2 * ulsch_harq->Z) * sizeof(int16_t));
  // skip filler bits
  memcpy((&z[0] + Kr), ulsch_harq->d[r] + (Kr - 2 * ulsch_harq->Z), (kc * ulsch_harq->Z - Kr) * sizeof(int16_t));
  // Saturate coded bits before decoding into 8 bits values
  for (i = 0, j = 0; j < ((kc * ulsch_harq->Z) >> 4) + 1; i += 2, j++) {
    pl[j] = _mm_packs_epi16(pv[i], pv[i + 1]);
  }
  //////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////// nrLDPC_decoder /////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////// pl =====> llrProcBuf //////////////////////////////////
  p_decoderParms->block_length = length_dec;
  p_decoderParms->crc_type = crc_type;
  rdata->decodeIterations = nrLDPC_decoder(p_decoderParms, (int8_t *)pl, llrProcBuf, p_procTime, &ulsch_harq->abort_decode);

  if (rdata->decodeIterations <= p_decoderParms->numMaxIter)
    memcpy(ulsch_harq->c[r],llrProcBuf,  Kr>>3);
  //stop_meas(&phy_vars_gNB->ulsch_ldpc_decoding_stats);
}

int nr_ulsch_decoding(PHY_VARS_gNB *phy_vars_gNB,
                        uint8_t ULSCH_id,
                        short *ulsch_llr,
                        NR_DL_FRAME_PARMS *frame_parms,
                        nfapi_nr_pusch_pdu_t *pusch_pdu,
                        uint32_t frame,
                        uint8_t nr_tti_rx,
                        uint8_t harq_pid,
                        uint32_t G)
  {
    if (!ulsch_llr) {
      LOG_E(PHY, "ulsch_decoding.c: NULL ulsch_llr pointer\n");
      return -1;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_gNB_ULSCH_DECODING, 1);

    NR_gNB_ULSCH_t *ulsch = &phy_vars_gNB->ulsch[ULSCH_id];
    NR_gNB_PUSCH *pusch = &phy_vars_gNB->pusch_vars[ULSCH_id];
    NR_UL_gNB_HARQ_t *harq_process = ulsch->harq_process;

    if (!harq_process) {
      LOG_E(PHY, "ulsch_decoding.c: NULL harq_process pointer\n");
      return -1;
  }
  uint8_t dtx_det = 0;

  int Kr;
  int Kr_bytes;

  harq_process->processedSegments = 0;
  
  // ------------------------------------------------------------------
  uint16_t nb_rb          = pusch_pdu->rb_size;
  uint8_t Qm              = pusch_pdu->qam_mod_order;
  uint8_t mcs             = pusch_pdu->mcs_index;
  uint8_t n_layers        = pusch_pdu->nrOfLayers;
  // ------------------------------------------------------------------

  harq_process->TBS = pusch_pdu->pusch_data.tb_size;

  dtx_det = 0;

  uint32_t A = (harq_process->TBS) << 3;

  // target_code_rate is in 0.1 units
  float Coderate = (float) pusch_pdu->target_code_rate / 10240.0f;

  LOG_D(PHY,"ULSCH Decoding, harq_pid %d rnti %x TBS %d G %d mcs %d Nl %d nb_rb %d, Qm %d, Coderate %f RV %d round %d new RX %d\n",
        harq_pid, ulsch->rnti, A, G, mcs, n_layers, nb_rb, Qm, Coderate, pusch_pdu->pusch_data.rv_index, harq_process->round, harq_process->harq_to_be_cleared);
  t_nrLDPC_dec_params decParams = {0};
  decParams.BG = pusch_pdu->maintenance_parms_v3.ldpcBaseGraph;
  int kc;
  if (decParams.BG == 2) {
    kc = 52;
  } else {
    kc = 68;
  }

  NR_gNB_PHY_STATS_t *stats = get_phy_stats(phy_vars_gNB, ulsch->rnti);
  if (stats) {
    stats->frame = frame;
    stats->ulsch_stats.round_trials[harq_process->round]++;
    for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      stats->ulsch_stats.power[aarx] = dB_fixed_x10(pusch->ulsch_power[aarx]);
      stats->ulsch_stats.noise_power[aarx] = dB_fixed_x10(pusch->ulsch_noise_power[aarx]);
    }
    if (!harq_process->harq_to_be_cleared) {
      stats->ulsch_stats.current_Qm = Qm;
      stats->ulsch_stats.current_RI = n_layers;
      stats->ulsch_stats.total_bytes_tx += harq_process->TBS;
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
                  decParams.BG);

  if (harq_process->C>MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER*n_layers) {
    LOG_E(PHY,"nr_segmentation.c: too many segments %d, B %d\n",harq_process->C,harq_process->B);
    return(-1);
  }


#ifdef DEBUG_ULSCH_DECODING
  printf("ulsch decoding nr segmentation Z %d\n", harq_process->Z);
  if (!frame%100)
    printf("K %d C %d Z %d \n", harq_process->K, harq_process->C, harq_process->Z);
#endif
  decParams.Z = harq_process->Z;

  decParams.numMaxIter = ulsch->max_ldpc_iterations;
  decParams.outMode = 0;

  uint32_t r_offset = 0;

  uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER*n_layers;  //number of segments to be allocated

  if (nb_rb != 273) {
    a_segments = a_segments*nb_rb;
    a_segments = a_segments/273 +1;
  }

  if (harq_process->C > a_segments) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,a_segments);
    return -1;
  }
#ifdef DEBUG_ULSCH_DECODING
  printf("Segmentation: C %d, K %d\n",harq_process->C,harq_process->K);
#endif

  if (harq_process->harq_to_be_cleared) {
    for (int r = 0; r < harq_process->C; r++)
      harq_process->d_to_be_cleared[r] = true;
    harq_process->harq_to_be_cleared = false;
  }

  Kr = harq_process->K;
  Kr_bytes = Kr >> 3;

  uint32_t offset = 0;
  if (phy_vars_gNB->ldpc_offload_flag && mcs > 9) {
    int8_t llrProcBuf[22 * 384];
    //  if (dtx_det==0) {
    int16_t z_ol[68 * 384];
    int8_t l_ol[68 * 384];
    int crc_type;
    int length_dec;

    if (harq_process->C == 1) {
      if (A > 3824)
        crc_type = CRC24_A;
      else
        crc_type = CRC16;

      length_dec = harq_process->B;
    } else {
      crc_type = CRC24_B;
      length_dec = (harq_process->B + 24 * harq_process->C) / harq_process->C;
    }
    int decodeIterations = 2;
    for (int r = 0; r < harq_process->C; r++) {
      int E = nr_get_E(G, harq_process->C, Qm, n_layers, r);
      memset(harq_process->c[r], 0, Kr_bytes);

      decParams.R = nr_get_R_ldpc_decoder(pusch_pdu->pusch_data.rv_index, E, decParams.BG, decParams.Z, &harq_process->llrLen, harq_process->round);

      if ((dtx_det == 0) && (pusch_pdu->pusch_data.rv_index == 0)) {
        // if (dtx_det==0){
          memcpy((&z_ol[0]), ulsch_llr + r_offset, E * sizeof(short));
          __m128i *pv_ol128 = (__m128i *)&z_ol;
          __m128i *pl_ol128 = (__m128i *)&l_ol;
          for (int i = 0, j = 0; j < ((kc * harq_process->Z) >> 4) + 1; i += 2, j++) {
            pl_ol128[j] = _mm_packs_epi16(pv_ol128[i], pv_ol128[i + 1]);
          }

          int ret = nrLDPC_decoder_offload(&decParams, harq_pid, ULSCH_id, r, pusch_pdu->pusch_data.rv_index, harq_process->F, E, Qm, (int8_t *)&pl_ol128[0], llrProcBuf, 1);
          if (ret < 0) {
            LOG_E(PHY, "ulsch_decoding.c: Problem in LDPC decoder offload\n");

            decodeIterations = ulsch->max_ldpc_iterations + 1;
            return -1;
          }
        for (int m = 0; m < Kr >> 3; m++) {
          harq_process->c[r][m] = (uint8_t)llrProcBuf[m];
        }

        if (check_crc((uint8_t *)llrProcBuf, length_dec, crc_type)) {
          PRINT_CRC_CHECK(LOG_I(PHY, "Segment %d CRC OK\n", r));
          decodeIterations = 2;
        } else {
          PRINT_CRC_CHECK(LOG_I(PHY, "segment %d CRC NOK\n", r));
          decodeIterations = ulsch->max_ldpc_iterations + 1;
        }
        //}

        r_offset += E;

        /*for (int k=0;k<8;k++)
          {
          printf("output decoder [%d] =  0x%02x \n", k, harq_process->c[r][k]);
          printf("llrprocbuf [%d] =  %x adr %p\n", k, llrProcBuf[k], llrProcBuf+k);
          }
        */
      } else {
        dtx_det = 0;
        decodeIterations = ulsch->max_ldpc_iterations + 1;
      }
      bool decodeSuccess = (decodeIterations <= ulsch->max_ldpc_iterations);
      if (decodeSuccess) {
        memcpy(harq_process->b + offset, harq_process->c[r], Kr_bytes - (harq_process->F >> 3) - ((harq_process->C > 1) ? 3 : 0));
        offset += (Kr_bytes - (harq_process->F >> 3) - ((harq_process->C > 1) ? 3 : 0));
        harq_process->processedSegments++;
      } else {
        LOG_D(PHY, "uplink segment error %d/%d\n", r, harq_process->C);
        LOG_D(PHY, "ULSCH %d in error\n", ULSCH_id);
        break; // don't even attempt to decode other segments
      }
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_gNB_ULSCH_DECODING, 0);

    if (harq_process->processedSegments == harq_process->C) {
      LOG_D(PHY, "[gNB %d] ULSCH: Setting ACK for slot %d TBS %d\n", phy_vars_gNB->Mod_id, ulsch->slot, harq_process->TBS);
      ulsch->active = false;
      harq_process->round = 0;

      LOG_D(PHY, "ULSCH received ok \n");
      nr_fill_indication(phy_vars_gNB, ulsch->frame, ulsch->slot, ULSCH_id, harq_pid, 0, 0);

    } else {
      LOG_D(PHY,
            "[gNB %d] ULSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d)\n",
            phy_vars_gNB->Mod_id,
            ulsch->frame,
            ulsch->slot,
            harq_pid,
            ulsch->active,
            harq_process->round,
            harq_process->TBS);
      ulsch->handled = 1;
      decodeIterations = ulsch->max_ldpc_iterations + 1;
      LOG_D(PHY, "ULSCH %d in error\n", ULSCH_id);
      nr_fill_indication(phy_vars_gNB, ulsch->frame, ulsch->slot, ULSCH_id, harq_pid, 1, 0);
    }
    ulsch->last_iteration_cnt = decodeIterations;
  }

  else {
    dtx_det = 0;
    set_abort(&harq_process->abort_decode, false);
    for (int r = 0; r < harq_process->C; r++) {
      int E = nr_get_E(G, harq_process->C, Qm, n_layers, r);
      union ldpcReqUnion id = {.s = {ulsch->rnti, frame, nr_tti_rx, 0, 0}};
      notifiedFIFO_elt_t *req = newNotifiedFIFO_elt(sizeof(ldpcDecode_t), id.p, &phy_vars_gNB->respDecode, &nr_processULSegment);
      ldpcDecode_t *rdata = (ldpcDecode_t *)NotifiedFifoData(req);
      decParams.R = nr_get_R_ldpc_decoder(pusch_pdu->pusch_data.rv_index, E, decParams.BG, decParams.Z, &harq_process->llrLen, harq_process->round);
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
      rdata->offset = offset;
      rdata->ulsch = ulsch;
      rdata->ulsch_id = ULSCH_id;
      rdata->tbslbrm = pusch_pdu->maintenance_parms_v3.tbSizeLbrmBytes;
      pushTpool(&phy_vars_gNB->threadPool, req);
      LOG_D(PHY, "Added a block to decode, in pipe: %d\n", r);
      r_offset += E;
      offset += (Kr_bytes - (harq_process->F >> 3) - ((harq_process->C > 1) ? 3 : 0));
      //////////////////////////////////////////////////////////////////////////////////////////
    }
  }
  return harq_process->C;
}
