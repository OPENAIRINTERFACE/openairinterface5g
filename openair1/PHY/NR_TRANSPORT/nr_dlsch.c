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

/*! \file PHY/NR_TRANSPORT/dlsch_decoding.c
* \brief Top-level routines for transmission of the PDSCH 38211 v 15.2.0
* \author Guy De Souza
* \date 2018
* \version 0.1
* \company Eurecom
* \email: desouza@eurecom.fr
* \note
* \warning
*/

#include "nr_dlsch.h"
#include "nr_dci.h"
#include "nr_sch_dmrs.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/nr/nr_common.h"

//#define DEBUG_DLSCH
//#define DEBUG_DLSCH_MAPPING

void nr_pdsch_codeword_scrambling(uint8_t *in,
                                  uint32_t size,
                                  uint8_t q,
                                  uint32_t Nid,
                                  uint32_t n_RNTI,
                                  uint32_t* out) {

  uint8_t reset, b_idx;
  uint32_t x1, x2, s=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_PDSCH_CODEWORD_SCRAMBLING, 1);
  reset = 1;
  x2 = (n_RNTI<<15) + (q<<14) + Nid;

  for (int i=0; i<size; i++) {
    b_idx = i&0x1f;
    if (b_idx==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
      if (i)
        out++;
    }
    *out ^= (((in[i])&1) ^ ((s>>b_idx)&1))<<b_idx;
    //printf("i %d b_idx %d in %d s 0x%08x out 0x%08x\n", i, b_idx, in[i], s, *out);
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_PDSCH_CODEWORD_SCRAMBLING, 0);
}

void nr_pdsch_codeword_scrambling_optim(uint8_t *in,
					uint32_t size,
					uint8_t q,
					uint32_t Nid,
					uint32_t n_RNTI,
					uint32_t* out) {
  
  uint32_t x1, x2, s=0,in32;

  x2 = (n_RNTI<<15) + (q<<14) + Nid;

  s=lte_gold_generic(&x1, &x2, 1);

#if defined(__AVX2__)
  for (int i=0; i<((size>>5)+((size&0x1f) > 0 ? 1 : 0)); i++) {
    in32=_mm256_movemask_epi8(_mm256_slli_epi16(((__m256i*)in)[i],7));
    out[i]=(in32^s);
    //printf("in[%d] %x => %x\n",i,in32,out[i]);
    s=lte_gold_generic(&x1, &x2, 0);
  }
#elif defined(__SSE4__)
  _m128i *in128;
  for (int i=0; i<((size>>5)+((size&0x1f) > 0 ? 1 : 0)); i++) {
    in128=&((__m128i*)in)[i<<1];
    ((uint16_t*)&in32)[0] = _mm128_movemask_epi8(_mm256_slli_epi16(in128[0],7));
    ((uint16_t*)&in32)[1] = _mm128_movemask_epi8(_mm256_slli_epi16(in128[1],7));
    out[i]=(in32^s);
    s=lte_gold_generic(&x1, &x2, 0);
  }
  //#elsif defined(__arm__) || defined(__aarch64)
  
#else 
  nr_pdsch_codeword_scrambling(in,
			       size,
			       q,
			       Nid,
			       n_RNTI,
			       out);
#endif
}


void nr_generate_pdsch(processingData_L1tx_t *msgTx,
                       int frame,
                       int slot) {

  PHY_VARS_gNB *gNB = msgTx->gNB;
  NR_gNB_DLSCH_t *dlsch;
  uint32_t ***pdsch_dmrs = gNB->nr_gold_pdsch_dmrs[slot];
  int32_t** txdataF = gNB->common_vars.txdataF;
  int16_t amp = AMP;
  int xOverhead = 0;
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  time_stats_t *dlsch_encoding_stats=&gNB->dlsch_encoding_stats;
  time_stats_t *dlsch_scrambling_stats=&gNB->dlsch_scrambling_stats;
  time_stats_t *dlsch_modulation_stats=&gNB->dlsch_modulation_stats;
  time_stats_t *tinput=&gNB->tinput;
  time_stats_t *tprep=&gNB->tprep;
  time_stats_t *tparity=&gNB->tparity;
  time_stats_t *toutput=&gNB->toutput;
  time_stats_t *dlsch_rate_matching_stats=&gNB->dlsch_rate_matching_stats;
  time_stats_t *dlsch_interleaving_stats=&gNB->dlsch_interleaving_stats;
  time_stats_t *dlsch_segmentation_stats=&gNB->dlsch_segmentation_stats;

  for (int dlsch_id=0; dlsch_id<msgTx->num_pdsch_slot; dlsch_id++) {
    dlsch = msgTx->dlsch[dlsch_id][0];

    NR_DL_gNB_HARQ_t *harq = &dlsch->harq_process;
    nfapi_nr_dl_tti_pdsch_pdu_rel15_t *rel15 = &harq->pdsch_pdu.pdsch_pdu_rel15;
    uint32_t scrambled_output[NR_MAX_NB_CODEWORDS][NR_MAX_PDSCH_ENCODED_LENGTH>>5];
    int16_t **mod_symbs = (int16_t**)dlsch->mod_symbs;
    int16_t **tx_layers = (int16_t**)dlsch->txdataF;
    int16_t **txdataF_precoding = (int16_t**)dlsch->txdataF_precoding;
    int8_t Wf[2], Wt[2], l0, l_prime, l_overline, delta;
    uint8_t dmrs_Type = rel15->dmrsConfigType;
    int nb_re_dmrs;
    uint16_t n_dmrs;
    if (rel15->dmrsConfigType==NFAPI_NR_DMRS_TYPE1) {
      nb_re_dmrs = 6*rel15->numDmrsCdmGrpsNoData;
    }
    else {
      nb_re_dmrs = 4*rel15->numDmrsCdmGrpsNoData;
    }
    n_dmrs = (rel15->BWPStart+rel15->rbStart+rel15->rbSize)*nb_re_dmrs;

    uint16_t dmrs_symbol_map = rel15->dlDmrsSymbPos;//single DMRS: 010000100 Double DMRS 110001100
    uint8_t dmrs_len = get_num_dmrs(rel15->dlDmrsSymbPos);
    uint16_t nb_re = ((12*rel15->NrOfSymbols)-nb_re_dmrs*dmrs_len-xOverhead)*rel15->rbSize*rel15->nrOfLayers;
    uint8_t Qm = rel15->qamModOrder[0];
    uint32_t encoded_length = nb_re*Qm;
    int16_t mod_dmrs[n_dmrs<<1] __attribute__ ((aligned(16)));

    /* PTRS */
    uint16_t beta_ptrs = 1;
    uint8_t ptrs_symbol = 0;
    uint16_t dlPtrsSymPos = 0;
    uint16_t n_ptrs = 0;
    uint16_t ptrs_idx = 0;
    uint8_t is_ptrs_re = 0;
    if(rel15->pduBitmap & 0x1) {
      set_ptrs_symb_idx(&dlPtrsSymPos,
                          rel15->NrOfSymbols,
                          rel15->StartSymbolIndex,
                          1<<rel15->PTRSTimeDensity,
                          rel15->dlDmrsSymbPos);
      n_ptrs = (rel15->rbSize + rel15->PTRSFreqDensity - 1)/rel15->PTRSFreqDensity;
    }
    int16_t mod_ptrs[n_ptrs<<1] __attribute__ ((aligned(16)));

    /// CRC, coding, interleaving and rate matching
    AssertFatal(harq->pdu!=NULL,"harq->pdu is null\n");
    unsigned char output[rel15->rbSize * NR_SYMBOLS_PER_SLOT * NR_NB_SC_PER_RB * 8 * NR_MAX_NB_LAYERS] __attribute__((aligned(32)));
    bzero(output,rel15->rbSize * NR_SYMBOLS_PER_SLOT * NR_NB_SC_PER_RB * 8 * NR_MAX_NB_LAYERS);
    start_meas(dlsch_encoding_stats);
    nr_dlsch_encoding(gNB,
		      harq->pdu, frame, slot, dlsch, frame_parms, output,
		      tinput,tprep,tparity,toutput,
		      dlsch_rate_matching_stats,
		      dlsch_interleaving_stats,
		      dlsch_segmentation_stats);
    stop_meas(dlsch_encoding_stats);
#ifdef DEBUG_DLSCH
    printf("PDSCH encoding:\nPayload:\n");
    for (int i=0; i<harq->B>>7; i++) {
      for (int j=0; j<16; j++)
	printf("0x%02x\t", harq->pdu[(i<<4)+j]);
      printf("\n");
    }
    printf("\nEncoded payload:\n");
    for (int i=0; i<encoded_length>>3; i++) {
      for (int j=0; j<8; j++)
	printf("%d", output[(i<<3)+j]);
      printf("\t");
    }
    printf("\n");
#endif
    
    
    
    /// scrambling
    start_meas(dlsch_scrambling_stats);
    for (int q=0; q<rel15->NrOfCodewords; q++)
      memset((void*)scrambled_output[q], 0, (encoded_length>>5)*sizeof(uint32_t));
    for (int q=0; q<rel15->NrOfCodewords; q++)
      nr_pdsch_codeword_scrambling_optim(output,
					 encoded_length,
					 q,
					 rel15->dataScramblingId,
					 rel15->rnti,
					 scrambled_output[q]);
    
    stop_meas(dlsch_scrambling_stats);
#ifdef DEBUG_DLSCH
    printf("PDSCH scrambling:\n");
    for (int i=0; i<encoded_length>>8; i++) {
      for (int j=0; j<8; j++)
	printf("0x%08x\t", scrambled_output[0][(i<<3)+j]);
      printf("\n");
    }
#endif
    
    /// Modulation
    start_meas(dlsch_modulation_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_PDSCH_MODULATION, 1);
    for (int q=0; q<rel15->NrOfCodewords; q++)
      nr_modulation(scrambled_output[q],
		    encoded_length,
		    Qm,
		    mod_symbs[q]);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_PDSCH_MODULATION, 0);
    stop_meas(dlsch_modulation_stats);
#ifdef DEBUG_DLSCH
    printf("PDSCH Modulation: Qm %d(%d)\n", Qm, nb_re);
    for (int i=0; i<nb_re>>3; i++) {
      for (int j=0; j<8; j++) {
	printf("%d %d\t", mod_symbs[0][((i<<3)+j)<<1], mod_symbs[0][(((i<<3)+j)<<1)+1]);
      }
      printf("\n");
    }
#endif
    
    start_meas(&gNB->dlsch_layer_mapping_stats); 
    /// Layer mapping
    nr_layer_mapping(mod_symbs,
		     rel15->nrOfLayers,
		     nb_re,
		     tx_layers);
#ifdef DEBUG_DLSCH
    printf("Layer mapping (%d layers):\n", rel15->nrOfLayers);
    for (int l=0; l<rel15->nrOfLayers; l++)
      for (int i=0; i<(nb_re/rel15->nrOfLayers)>>3; i++) {
	printf("layer %d, Re %d..%d : ",l,i<<3,(i<<3)+7);
	for (int j=0; j<8; j++) {
	  printf("l%d %d\t", tx_layers[l][((i<<3)+j)<<1], tx_layers[l][(((i<<3)+j)<<1)+1]);
	}
	printf("\n");
      }
#endif

    stop_meas(&gNB->dlsch_layer_mapping_stats); 
    /// Resource mapping
    
    // Non interleaved VRB to PRB mapping
    uint16_t start_sc = frame_parms->first_carrier_offset + (rel15->rbStart+rel15->BWPStart)*NR_NB_SC_PER_RB;
    if (start_sc >= frame_parms->ofdm_symbol_size)
      start_sc -= frame_parms->ofdm_symbol_size;

    int txdataF_offset = (slot%2)*frame_parms->samples_per_slot_wCP;

#ifdef DEBUG_DLSCH_MAPPING
    printf("PDSCH resource mapping started (start SC %d\tstart symbol %d\tN_PRB %d\tnb_re %d,nb_layers %d)\n",
	   start_sc, rel15->StartSymbolIndex, rel15->rbSize, nb_re,rel15->nrOfLayers);
#endif

    start_meas(&gNB->dlsch_resource_mapping_stats);
    for (int nl=0; nl<rel15->nrOfLayers; nl++) {

      int dmrs_port = get_dmrs_port(nl,rel15->dmrsPorts);
      // DMRS params for this dmrs port
      get_Wt(Wt, dmrs_port, dmrs_Type);
      get_Wf(Wf, dmrs_port, dmrs_Type);
      delta = get_delta(dmrs_port, dmrs_Type);
      l_prime = 0; // single symbol nl 0
      l0 = get_l0(rel15->dlDmrsSymbPos);
      l_overline = l0;

#ifdef DEBUG_DLSCH_MAPPING
      uint8_t dmrs_symbol = l0+l_prime;
      printf("DMRS Type %d params for nl %d: Wt %d %d \t Wf %d %d \t delta %d \t l_prime %d \t l0 %d\tDMRS symbol %d\n",
	     1+dmrs_Type,nl, Wt[0], Wt[1], Wf[0], Wf[1], delta, l_prime, l0, dmrs_symbol);
#endif

      uint16_t m=0, dmrs_idx=0;

      // Loop Over OFDM symbols:
      for (int l=rel15->StartSymbolIndex; l<rel15->StartSymbolIndex+rel15->NrOfSymbols; l++) {
        /// DMRS QPSK modulation
        uint8_t k_prime=0;
        uint16_t n=0;

        if ((dmrs_symbol_map & (1 << l))){ // DMRS time occasion
          // The reference point for is subcarrier 0 of the lowest-numbered resource block in CORESET 0 if the corresponding
          // PDCCH is associated with CORESET 0 and Type0-PDCCH common search space and is addressed to SI-RNTI
          // 3GPP TS 38.211 V15.8.0 Section 7.4.1.1.2 Mapping to physical resources
          if (rel15->rnti==SI_RNTI) {
            if (dmrs_Type==NFAPI_NR_DMRS_TYPE1) {
              dmrs_idx = rel15->rbStart*6;
            } else {
              dmrs_idx = rel15->rbStart*4;
            }
          } else {
            if (dmrs_Type == NFAPI_NR_DMRS_TYPE1) {
              dmrs_idx = (rel15->rbStart+rel15->BWPStart)*6;
            } else {
              dmrs_idx = (rel15->rbStart+rel15->BWPStart)*4;
            }
          }
          if (l==(l_overline+1)) //take into account the double DMRS symbols
            l_prime = 1;
          else if (l>(l_overline+1)) {//new DMRS pair
            l_overline = l;
            l_prime = 0;
          }
          /// DMRS QPSK modulation
          nr_modulation(pdsch_dmrs[l][0], n_dmrs*2, DMRS_MOD_ORDER, mod_dmrs); // currently only codeword 0 is modulated. Qm = 2 as DMRS is QPSK modulated

#ifdef DEBUG_DLSCH
          printf("DMRS modulation (symbol %d, %d symbols, type %d):\n", l, n_dmrs, dmrs_Type);
          for (int i=0; i<n_dmrs>>4; i++) {
            for (int j=0; j<8; j++) {
              printf("%d %d\t", mod_dmrs[((i<<3)+j)<<1], mod_dmrs[(((i<<3)+j)<<1)+1]);
            }
            printf("\n");
          }
#endif
        }

        /* calculate if current symbol is PTRS symbols */
        ptrs_idx = 0;

        if(rel15->pduBitmap & 0x1) {
          ptrs_symbol = is_ptrs_symbol(l,dlPtrsSymPos);
          if(ptrs_symbol) {
            /* PTRS QPSK Modulation for each OFDM symbol in a slot */
            printf("Doing ptrs modulation for symbol %d, n_ptrs %d\n",l,n_ptrs);
            nr_modulation(pdsch_dmrs[l][0], (n_ptrs<<1), DMRS_MOD_ORDER, mod_ptrs);
          }
        }
        uint16_t k = start_sc;
        if (ptrs_symbol || dmrs_symbol_map & (1 << l)) {

          // Loop Over SCs:
          for (int i=0; i<rel15->rbSize*NR_NB_SC_PER_RB; i++) {
            /* check if cuurent RE is PTRS RE*/
            is_ptrs_re = 0;
            if (ptrs_symbol)
              is_ptrs_re = is_ptrs_subcarrier(k,
                                              rel15->rnti,
                                              nl,
                                              rel15->dmrsConfigType,
                                              rel15->PTRSFreqDensity,
                                              rel15->rbSize,
                                              rel15->PTRSReOffset,
                                              start_sc,
                                              frame_parms->ofdm_symbol_size);
            /* Map DMRS Symbol */
            if ( (dmrs_symbol_map & (1 << l)) &&
                 (k == ((start_sc+get_dmrs_freq_idx(n, k_prime, delta, dmrs_Type))%(frame_parms->ofdm_symbol_size)))) {
              txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) +     (2*txdataF_offset)] = (Wt[l_prime]*Wf[k_prime]*amp*mod_dmrs[dmrs_idx<<1]) >> 15;
              txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) + 1 + (2*txdataF_offset)] = (Wt[l_prime]*Wf[k_prime]*amp*mod_dmrs[(dmrs_idx<<1) + 1]) >> 15;
#ifdef DEBUG_DLSCH_MAPPING
              printf("dmrs_idx %d\t l %d \t k %d \t k_prime %d \t n %d \t txdataF: %d %d\n",
                     dmrs_idx, l, k, k_prime, n, txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) + (2*txdataF_offset)],
                     txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) + 1 + (2*txdataF_offset)]);
#endif
              dmrs_idx++;
              k_prime++;
              k_prime&=1;
              n+=(k_prime)?0:1;
            }
            else if(is_ptrs_re){
              txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) +     (2*txdataF_offset)] = (beta_ptrs*amp*mod_ptrs[ptrs_idx<<1]) >> 15;
              txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) + 1 + (2*txdataF_offset)] = (beta_ptrs*amp*mod_ptrs[(ptrs_idx<<1) + 1])>> 15;
#ifdef DEBUG_DLSCH_MAPPING
              printf("ptrs_idx %d\t l %d \t k %d \t k_prime %d \t n %d \t txdataF: %d %d, mod_ptrs: %d %d\n",
                     ptrs_idx, l, k, k_prime, n, ((int16_t*)txdataF_precoding[nl])[((l*frame_parms->ofdm_symbol_size + k)<<1) + (2*txdataF_offset)],
                     ((int16_t*)txdataF_precoding[nl])[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1 + (2*txdataF_offset)],mod_ptrs[ptrs_idx<<1],mod_ptrs[(ptrs_idx<<1)+1]);
#endif
              ptrs_idx++;
            }
          /* Map DATA Symbol */
            else if( ptrs_symbol || allowed_xlsch_re_in_dmrs_symbol(k,start_sc,frame_parms->ofdm_symbol_size,rel15->numDmrsCdmGrpsNoData,dmrs_Type)) {
              txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) +     (2*txdataF_offset)] = (amp * tx_layers[nl][m<<1]) >> 15;
              txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) + 1 + (2*txdataF_offset)] = (amp * tx_layers[nl][(m<<1) + 1]) >> 15;
#ifdef DEBUG_DLSCH_MAPPING
              printf("m %d\t l %d \t k %d \t txdataF: %d %d\n",
                     m, l, k, txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) + (2*txdataF_offset)],
                     txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) + 1 + (2*txdataF_offset)]);
#endif
              m++;
            }
            /* mute RE */
            else {
              txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) +     (2*txdataF_offset)] = 0;
              txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) + 1 + (2*txdataF_offset)] = 0;
            }
            if (++k >= frame_parms->ofdm_symbol_size)
              k -= frame_parms->ofdm_symbol_size;
          } //RE loop
        }
        else { // no PTRS or DMRS in this symbol
          // Loop Over SCs:
          int upper_limit=rel15->rbSize*NR_NB_SC_PER_RB;
          int remaining_re = 0;
          if (start_sc + upper_limit > frame_parms->ofdm_symbol_size) {
            remaining_re = upper_limit + start_sc - frame_parms->ofdm_symbol_size;
            upper_limit = frame_parms->ofdm_symbol_size - start_sc;
          }
          // fix the alignment issues later, use 64-bit SIMD below instead of 128.
          if (0/*(frame_parms->N_RB_DL&1)==0*/) {
            __m128i *txF=(__m128i*)&txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size+start_sc+txdataF_offset)<<1)];

            __m128i *txl = (__m128i*)&tx_layers[nl][m<<1];
            __m128i amp128=_mm_set1_epi16(amp);
            for (int i=0; i<(upper_limit>>2); i++) {
              txF[i] = _mm_mulhrs_epi16(amp128,txl[i]);
            } //RE loop, first part
            m+=upper_limit;
            if (remaining_re > 0) {
               txF = (__m128i*)&txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size+txdataF_offset)<<1)];
               txl = (__m128i*)&tx_layers[nl][m<<1];
               for (int i=0; i<(remaining_re>>2); i++) {
                 txF[i] = _mm_mulhrs_epi16(amp128,txl[i]);
               }
            }
          }
          else {
            __m64 *txF=(__m64*)&txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size+start_sc+txdataF_offset)<<1)];

            __m64 *txl = (__m64*)&tx_layers[nl][m<<1];
            __m64 amp64=_mm_set1_pi16(amp);
            for (int i=0; i<(upper_limit>>1); i++) {

              txF[i] = _mm_mulhrs_pi16(amp64,txl[i]);
#ifdef DEBUG_DLSCH_MAPPING
              if ((i&1) > 0)
                  printf("m %d\t l %d \t k %d \t txdataF: %d %d\n",
                       m, l, start_sc+(i>>1), txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + start_sc+(i>>1))<<1) + (2*txdataF_offset)],
                  txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + start_sc+(i>>1))<<1) + 1 + (2*txdataF_offset)]);
#endif
              /* handle this, mute RE */
              /*else {
                txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) +     (2*txdataF_offset)] = 0;
                txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) + 1 + (2*txdataF_offset)] = 0;
              }*/
            } //RE loop, first part
            m+=upper_limit;
            if (remaining_re > 0) {
               txF = (__m64*)&txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size+txdataF_offset)<<1)];
               txl = (__m64*)&tx_layers[nl][m<<1];
               for (int i=0; i<(remaining_re>>1); i++) {
                 txF[i] = _mm_mulhrs_pi16(amp64,txl[i]);
#ifdef DEBUG_DLSCH_MAPPING
                 if ((i&1) > 0)
                   printf("m %d\t l %d \t k %d \t txdataF: %d %d\n",
                          m, l, i>>1, txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + (i>>1))<<1) + (2*txdataF_offset)],
                          txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + (i>>1))<<1) + 1 + (2*txdataF_offset)]);
#endif
                 /* handle this, mute RE */
                 /*else {
                   txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) +     (2*txdataF_offset)] = 0;
                   txdataF_precoding[nl][((l*frame_parms->ofdm_symbol_size + k)<<1) + 1 + (2*txdataF_offset)] = 0;
                 }*/
               } //RE loop, second part
            } // 
            m+=remaining_re;
          } // N_RB_DL even
        } // no DMRS/PTRS in symbol  
      } // symbol loop
    }// layer loop
    stop_meas(&gNB->dlsch_resource_mapping_stats);

    ///Layer Precoding and Antenna port mapping
    // tx_layers 1-8 are mapped on antenna ports 1000-1007
    // The precoding info is supported by nfapi such as num_prgs, prg_size, prgs_list and pm_idx
    // The same precoding matrix is applied on prg_size RBs, Thus
    //        pmi = prgs_list[rbidx/prg_size].pm_idx, rbidx =0,...,rbSize-1
    // The Precoding matrix:
    // The Codebook Type I and Type II are not supported yet.
    // We`adopt the precoding matrices of PUSCH for 4 layers.
    start_meas(&gNB->dlsch_precoding_stats);

    for (int ap=0; ap<frame_parms->nb_antennas_tx; ap++) {

      for (int l=rel15->StartSymbolIndex; l<rel15->StartSymbolIndex+rel15->NrOfSymbols; l++) {
        uint16_t k = start_sc;

        for (int rb=0; rb<rel15->rbSize; rb++) {
          //get pmi info
          uint8_t pmi;
          if (rel15->precodingAndBeamforming.prg_size > 0)
            pmi = rel15->precodingAndBeamforming.prgs_list[(int)rb/rel15->precodingAndBeamforming.prg_size].pm_idx;
          else
            pmi = 0;//no precoding

          if (pmi == 0) {//unitary Precoding
            if(ap<rel15->nrOfLayers)
              memcpy((void*)&txdataF[ap][l*frame_parms->ofdm_symbol_size + txdataF_offset + k],
                     (void*)&txdataF_precoding[ap][2*(l*frame_parms->ofdm_symbol_size + txdataF_offset+ k)],
                     NR_NB_SC_PER_RB*sizeof(int32_t));
            else
              memset((void*)&txdataF[ap][l*frame_parms->ofdm_symbol_size + txdataF_offset + k],
                     0,
                     NR_NB_SC_PER_RB*sizeof(int32_t));
            k += NR_NB_SC_PER_RB;
            if (k >= frame_parms->ofdm_symbol_size) {
              k -= frame_parms->ofdm_symbol_size;
            }
          }
          else {
            //get the precoding matrix weights:
            char *W_prec;
            switch (frame_parms->nb_antennas_tx) {
              case 1://1 antenna port
                W_prec = nr_W_1l_2p[pmi][ap];
                break;
              case 2://2 antenna ports
                if (rel15->nrOfLayers == 1)//1 layer
                  W_prec = nr_W_1l_2p[pmi][ap];
                else//2 layers
                  W_prec = nr_W_2l_2p[pmi][ap];
                break;
              case 4://4 antenna ports
                if (rel15->nrOfLayers == 1)//1 layer
                  W_prec = nr_W_1l_4p[pmi][ap];
                else if (rel15->nrOfLayers == 2)//2 layers
                  W_prec = nr_W_2l_4p[pmi][ap];
                else if (rel15->nrOfLayers == 3)//3 layers
                  W_prec = nr_W_3l_4p[pmi][ap];
                else//4 layers
                  W_prec = nr_W_4l_4p[pmi][ap];
                break;
              default:
                LOG_D(PHY,"Precoding 1,2, or 4 antenna ports are currently supported\n");
                W_prec = nr_W_1l_2p[pmi][ap];
                break;
            }
            for (int i=0; i<NR_NB_SC_PER_RB; i++) {
              int32_t re_offset = l*frame_parms->ofdm_symbol_size + k;
              int32_t precodatatx_F = nr_layer_precoder(txdataF_precoding, W_prec, rel15->nrOfLayers, re_offset+txdataF_offset);
              ((int16_t*)txdataF[ap])[(re_offset<<1) + (2*txdataF_offset)] = ((int16_t *) &precodatatx_F)[0];
              ((int16_t*)txdataF[ap])[(re_offset<<1) + 1 + (2*txdataF_offset)] = ((int16_t *) &precodatatx_F)[1];
#ifdef DEBUG_DLSCH_MAPPING
              printf("antenna %d\t l %d \t k %d \t txdataF: %d %d\n",
                     ap, l, k, ((int16_t*)txdataF[ap])[(re_offset<<1) + (2*txdataF_offset)],
                     ((int16_t*)txdataF[ap])[(re_offset<<1) + 1 + (2*txdataF_offset)]);
#endif
              if (++k >= frame_parms->ofdm_symbol_size) {
                k -= frame_parms->ofdm_symbol_size;
              }
            }
          }
        } //RB loop
      } // symbol loop
    }// port loop

    stop_meas(&gNB->dlsch_precoding_stats);
    dlsch->slot_tx[slot]=0;

    // TODO: handle precoding
    // this maps the layers onto antenna ports
    
    // handle beamforming ID
    // each antenna port is assigned a beam_index
    // since PHY can only handle BF on slot basis we set the whole slot

    // first check if this slot has not already been allocated to another beam
    if (gNB->common_vars.beam_id[0][slot*frame_parms->symbols_per_slot]==255) {
      for (int j=0;j<frame_parms->symbols_per_slot;j++) 
	gNB->common_vars.beam_id[0][slot*frame_parms->symbols_per_slot+j] = rel15->precodingAndBeamforming.prgs_list[0].dig_bf_interface_list[0].beam_idx;
    }
    else {
      LOG_D(PHY,"beam index for PDSCH allocation already taken\n");
    }
  }// dlsch loop
}

void dump_pdsch_stats(FILE *fd,PHY_VARS_gNB *gNB) {

  for (int i=0;i<NUMBER_OF_NR_SCH_STATS_MAX;i++)
    if (gNB->dlsch_stats[i].rnti > 0 && gNB->dlsch_stats[i].frame != gNB->dlsch_stats[i].dump_frame) {
      gNB->dlsch_stats[i].dump_frame = gNB->dlsch_stats[i].frame;
      fprintf(fd,"DLSCH RNTI %x: current_Qm %d, current_RI %d, total_bytes TX %d\n",
	    gNB->dlsch_stats[i].rnti,
	    gNB->dlsch_stats[i].current_Qm,
	    gNB->dlsch_stats[i].current_RI,
	    gNB->dlsch_stats[i].total_bytes_tx);
    }

}

void clear_pdsch_stats(PHY_VARS_gNB *gNB) {

  for (int i=0;i<gNB->number_of_nr_dlsch_max;i++)
    memset((void*)&gNB->dlsch_stats[i],0,sizeof(gNB->dlsch_stats[i]));
}
