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

/*! \file PHY/NR_TRANSPORT/nr_dci.c
* \brief Implements DCI encoding and PDCCH TX procedures (38.212/38.213/38.214). V15.4.0 2019-01.
* \author Guy De Souza
* \date 2018
* \version 0.1
* \company Eurecom
* \email: desouza@eurecom.fr
* \note
* \warning
*/


#include "nr_dci.h"
#include "nr_dlsch.h"
#include "nr_sch_dmrs.h"
#include "PHY/MODULATION/nr_modulation.h"

//#define DEBUG_PDCCH_DMRS
//#define DEBUG_DCI
//#define DEBUG_CHANNEL_CODING

void nr_pdcch_scrambling(uint32_t *in,
                         uint32_t size,
                         uint32_t Nid,
                         uint32_t scrambling_RNTI,
                         uint32_t *out) {
  uint8_t reset;
  uint32_t x1, x2, s=0;
  reset = 1;
  x2 = (scrambling_RNTI<<16) + Nid;
  LOG_D(PHY,"PDCCH Scrambling x2 %x : scrambling_RNTI %x \n", x2, scrambling_RNTI);
  for (int i=0; i<size; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;

      if (i) {
        in++;
        out++;
      }
    }

    (*out) ^= ((((*in)>>(i&0x1f))&1) ^ ((s>>(i&0x1f))&1))<<(i&0x1f);
  }
}

void nr_generate_dci(PHY_VARS_gNB *gNB,
                        nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15,
                        uint32_t **gold_pdcch_dmrs,
                        int32_t *txdataF,
                        int16_t amp,
                        NR_DL_FRAME_PARMS *frame_parms) {

  int16_t mod_dmrs[NR_MAX_CSET_DURATION][NR_MAX_PDCCH_DMRS_LENGTH>>1] __attribute__((aligned(16))); // 3 for the max coreset duration
  uint16_t cset_start_sc;
  uint8_t cset_start_symb, cset_nsymb;
  int k,l,k_prime,dci_idx, dmrs_idx;

  // find coreset descriptor
    
  int rb_offset;
  int n_rb;
  
  // compute rb_offset and n_prb based on frequency allocation
  nr_fill_cce_list(gNB,0,pdcch_pdu_rel15);
  get_coreset_rballoc(pdcch_pdu_rel15->FreqDomainResource,&n_rb,&rb_offset);
  cset_start_sc = frame_parms->first_carrier_offset + (pdcch_pdu_rel15->BWPStart + rb_offset) * NR_NB_SC_PER_RB;

  for (int d=0;d<pdcch_pdu_rel15->numDlDci;d++) {
    /*The coreset is initialised
     * in frequency: the first subcarrier is obtained by adding the first CRB overlapping the SSB and the rb_offset for coreset 0
     * or the rb_offset for other coresets
     * in time: by its first slot and its first symbol*/
    const nfapi_nr_dl_dci_pdu_t *dci_pdu = &pdcch_pdu_rel15->dci_pdu[d];

    cset_start_symb = pdcch_pdu_rel15->StartSymbolIndex;
    cset_nsymb = pdcch_pdu_rel15->DurationSymbols;
    dci_idx = 0;
    LOG_D(PHY, "Coreset rb_offset %d, nb_rb %d BWP Start %d\n",rb_offset,n_rb,pdcch_pdu_rel15->BWPStart);
    LOG_D(PHY, "Coreset starting subcarrier %d on symbol %d (%d symbols)\n", cset_start_sc, cset_start_symb, cset_nsymb);
    // DMRS length is per OFDM symbol
    uint32_t dmrs_length = n_rb*6; //2(QPSK)*3(per RB)*6(REG per CCE)
    uint32_t encoded_length = dci_pdu->AggregationLevel*108; //2(QPSK)*9(per RB)*6(REG per CCE)
    LOG_D(PHY, "DL_DCI : rb_offset %d, nb_rb %d, DMRS length per symbol %d\t DCI encoded length %d (precoder_granularity %d,reg_mapping %d),Scrambling_Id %d,ScramblingRNTI %x,PayloadSizeBits %d\n",
    rb_offset, n_rb,dmrs_length, encoded_length,pdcch_pdu_rel15->precoderGranularity,pdcch_pdu_rel15->CceRegMappingType,
    dci_pdu->ScramblingId,dci_pdu->ScramblingRNTI,dci_pdu->PayloadSizeBits);
    dmrs_length += rb_offset*6; // To accommodate more DMRS symbols in case of rb offset
      
    /// DMRS QPSK modulation
    for (int symb=cset_start_symb; symb<cset_start_symb + pdcch_pdu_rel15->DurationSymbols; symb++) {
      
      nr_modulation(gold_pdcch_dmrs[symb], dmrs_length, DMRS_MOD_ORDER, mod_dmrs[symb]); //Qm = 2 as DMRS is QPSK modulated
      
#ifdef DEBUG_PDCCH_DMRS
       if(dci_pdu->RNTI!=0xFFFF) {      
      for (int i=0; i<dmrs_length>>1; i++)
	printf("symb %d i %d %p gold seq 0x%08x mod_dmrs %d %d\n", symb, i,
	       &gold_pdcch_dmrs[symb][i>>5],gold_pdcch_dmrs[symb][i>>5], mod_dmrs[symb][i<<1], mod_dmrs[symb][(i<<1)+1] );
    }  
#endif
    }
    
    /// DCI payload processing
    // CRC attachment + Scrambling + Channel coding + Rate matching
    uint32_t encoder_output[NR_MAX_DCI_SIZE_DWORD];

    uint16_t n_RNTI = dci_pdu->RNTI;
    uint16_t Nid    = dci_pdu->ScramblingId;
    uint16_t scrambling_RNTI = dci_pdu->ScramblingRNTI;

    t_nrPolar_params *currentPtr = nr_polar_params(NR_POLAR_DCI_MESSAGE_TYPE, 
						   dci_pdu->PayloadSizeBits,
						   dci_pdu->AggregationLevel,
						   0,NULL);
    polar_encoder_fast((uint64_t*)dci_pdu->Payload, (void*)encoder_output, n_RNTI,1,currentPtr);
#ifdef DEBUG_CHANNEL_CODING
    printf("polar rnti %x,length %d, L %d\n",n_RNTI, dci_pdu->PayloadSizeBits,pdcch_pdu_rel15->dci_pdu->AggregationLevel);
    printf("DCI PDU: [0]->0x%lx \t [1]->0x%lx\n",
	   ((uint64_t*)dci_pdu->Payload)[0], ((uint64_t*)dci_pdu->Payload)[1]);
    printf("Encoded Payload (length:%d dwords):\n", encoded_length>>5);
    
    for (int i=0; i<encoded_length>>5; i++)
      printf("[%d]->0x%08x \t", i,encoder_output[i]);    

    printf("\n");
#endif
    /// Scrambling
    uint32_t scrambled_output[NR_MAX_DCI_SIZE_DWORD]= {0};
    nr_pdcch_scrambling(encoder_output, encoded_length, Nid, scrambling_RNTI, scrambled_output);
#ifdef DEBUG_CHANNEL_CODING
    printf("scrambled output: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\t [4]->0x%08x\t [5]->0x%08x\t \
[6]->0x%08x \t [7]->0x%08x \t [8]->0x%08x \t [9]->0x%08x\t [10]->0x%08x\t [11]->0x%08x\n",
	   scrambled_output[0], scrambled_output[1], scrambled_output[2], scrambled_output[3], scrambled_output[4],scrambled_output[5],
	   scrambled_output[6], scrambled_output[7], scrambled_output[8], scrambled_output[9], scrambled_output[10],scrambled_output[11] );
#endif
    /// QPSK modulation
    int16_t mod_dci[NR_MAX_DCI_SIZE>>1] __attribute__((aligned(16)));
    nr_modulation(scrambled_output, encoded_length, DMRS_MOD_ORDER, mod_dci); //Qm = 2 as DMRS is QPSK modulated
#ifdef DEBUG_DCI
    
    for (int i=0; i<encoded_length>>1; i++)
      printf("i %d mod_dci %d %d\n", i, mod_dci[i<<1], mod_dci[(i<<1)+1] );
    
#endif

    /// Resource mapping

    if (cset_start_sc >= frame_parms->ofdm_symbol_size)
      cset_start_sc -= frame_parms->ofdm_symbol_size;

    // Get cce_list indices by reg_idx in ascending order
    int reg_list_index = 0;
    int reg_list_order[NR_MAX_PDCCH_AGG_LEVEL] = {};
    for (int p = 0; p < NR_MAX_PDCCH_AGG_LEVEL; p++) {
      for(int p2 = 0; p2 < dci_pdu->AggregationLevel; p2++) {
        if(gNB->cce_list[d][p2].reg_list[0].reg_idx == p * NR_NB_REG_PER_CCE) {
          reg_list_order[reg_list_index] = p2;
          reg_list_index++;
          break;
        }
      }
    }

    /*Mapping the encoded DCI along with the DMRS */
    for(int symbol_idx = 0; symbol_idx < pdcch_pdu_rel15->DurationSymbols; symbol_idx++) {
      for (int cce_count = 0; cce_count < dci_pdu->AggregationLevel; cce_count+=pdcch_pdu_rel15->DurationSymbols) {

        int8_t cce_idx = reg_list_order[cce_count];

        for (int reg_in_cce_idx = 0; reg_in_cce_idx < NR_NB_REG_PER_CCE; reg_in_cce_idx++) {

          k = cset_start_sc + gNB->cce_list[d][cce_idx].reg_list[reg_in_cce_idx].start_sc_idx;

          if (k >= frame_parms->ofdm_symbol_size)
            k -= frame_parms->ofdm_symbol_size;

          l = cset_start_symb + symbol_idx;

          // dmrs index depends on reference point for k according to 38.211 7.4.1.3.2
          if (pdcch_pdu_rel15->CoreSetType == NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG)
            dmrs_idx = (gNB->cce_list[d][cce_idx].reg_list[reg_in_cce_idx].reg_idx) * 3;
          else
            dmrs_idx = (gNB->cce_list[d][cce_idx].reg_list[reg_in_cce_idx].reg_idx + rb_offset) * 3;

          k_prime = 0;

          for (int m = 0; m < NR_NB_SC_PER_RB; m++) {
            if (m == (k_prime << 2) + 1) { // DMRS if not already mapped
              ((int16_t *) txdataF)[(l * frame_parms->ofdm_symbol_size + k) << 1] =
                  (amp * mod_dmrs[l][dmrs_idx << 1]) >> 15;
              ((int16_t *) txdataF)[((l * frame_parms->ofdm_symbol_size + k) << 1) + 1] =
                  (amp * mod_dmrs[l][(dmrs_idx << 1) + 1]) >> 15;

#ifdef DEBUG_PDCCH_DMRS
              LOG_D(PHY,"PDCCH DMRS %d: l %d position %d => (%d,%d)\n",dmrs_idx,l,k,((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1],
               ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1)+1]);
#endif

              dmrs_idx++;
              k_prime++;

            } else { // DCI payload
              ((int16_t *) txdataF)[(l * frame_parms->ofdm_symbol_size + k) << 1] = (amp * mod_dci[dci_idx << 1]) >> 15;
              ((int16_t *) txdataF)[((l * frame_parms->ofdm_symbol_size + k) << 1) + 1] =
                  (amp * mod_dci[(dci_idx << 1) + 1]) >> 15;
#ifdef DEBUG_DCI
              LOG_D(PHY,"PDCCH: l %d position %d => (%d,%d)\n",l,k,((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1],
               ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1)+1]);
#endif

              dci_idx++;
            }

            k++;

            if (k >= frame_parms->ofdm_symbol_size)
              k -= frame_parms->ofdm_symbol_size;

          } // m
        } // reg_in_cce_idx
      } // cce_count
    } // symbol_idx

    LOG_D(PHY,
          "DCI: payloadSize = %d | payload = %llx\n",
          dci_pdu->PayloadSizeBits,
          *(unsigned long long *)dci_pdu->Payload);
  } // for (int d=0;d<pdcch_pdu_rel15->numDlDci;d++)
}

void nr_generate_dci_top(PHY_VARS_gNB *gNB,
			    nfapi_nr_dl_tti_pdcch_pdu *pdcch_pdu,
			    nfapi_nr_dl_tti_pdcch_pdu *ul_dci_pdu,
                            uint32_t **gold_pdcch_dmrs,
                            int32_t *txdataF,
                            int16_t amp,
                            NR_DL_FRAME_PARMS *frame_parms) {

  AssertFatal(pdcch_pdu!=NULL || ul_dci_pdu!=NULL,"At least one pointer has to be !NULL\n");

  if (pdcch_pdu) {
    nr_generate_dci(gNB,&pdcch_pdu->pdcch_pdu_rel15,gold_pdcch_dmrs,txdataF,amp,frame_parms);
  }
  if (ul_dci_pdu) {
    nr_generate_dci(gNB,&ul_dci_pdu->pdcch_pdu_rel15,gold_pdcch_dmrs,txdataF,amp,frame_parms);
  }
}

