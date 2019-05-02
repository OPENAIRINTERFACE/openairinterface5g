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

/*! \file PHY/NR_UE_TRANSPORT/nr_ulsch.c
* \brief Top-level routines for transmission of the PUSCH TS 38.211 v 15.4.0
* \author Khalid Ahmed
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: khalid.ahmed@iis.fraunhofer.de
* \note
* \warning
*/
#include <stdint.h>
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "common/utils/assertions.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/defs_nr_common.h"
#include "PHY/TOOLS/tools_defs.h"

//#define DEBUG_SCFDMA

#ifdef DEBUG_SCFDMA

  FILE *debug_scfdma;

#endif

int generate_ue_ulsch_params(PHY_VARS_NR_UE *UE,
                             temp_nfapi_nr_ul_config_pdcch_parameters_rel15_t *nr_ul_pdcch_params,
                             uint8_t thread_id,
                             int gNB_id,
                             unsigned char harq_pid,
                             unsigned char *test_input){

  int N_PRB_oh, N_RE_prime, cwd_idx, length_dmrs, Nid_cell;
  uint16_t n_rnti;

  NR_UE_ULSCH_t *ulsch_ue;
  NR_UL_UE_HARQ_t *harq_process_ul_ue;

  length_dmrs = 1;
  n_rnti = 0x1234;
  Nid_cell = 0;

  for (cwd_idx = 0; cwd_idx < nr_ul_pdcch_params->nb_codewords; cwd_idx++) {

    ulsch_ue = UE->ulsch[thread_id][gNB_id][cwd_idx];
    harq_process_ul_ue = ulsch_ue->harq_processes[harq_pid];

    ulsch_ue->length_dmrs = length_dmrs;
    ulsch_ue->rnti        = n_rnti;
    ulsch_ue->Nid_cell    = Nid_cell;
    ulsch_ue->Nsc_pusch   = nr_ul_pdcch_params->nb_rb*NR_NB_SC_PER_RB;
    ulsch_ue->Nsymb_pusch = nr_ul_pdcch_params->n_symb;
    ulsch_ue->nb_re_dmrs  = UE->dmrs_UplinkConfig.pusch_maxLength*(UE->dmrs_UplinkConfig.pusch_dmrs_type == pusch_dmrs_type1)?6:4;


    N_PRB_oh   = 0; // higher layer (RRC) parameter xOverhead in PUSCH-ServingCellConfig
    N_RE_prime = NR_NB_SC_PER_RB*nr_ul_pdcch_params->n_symb - ulsch_ue->nb_re_dmrs - N_PRB_oh;

    if (harq_process_ul_ue) {

      harq_process_ul_ue->mcs                = nr_ul_pdcch_params->Imcs;
      harq_process_ul_ue->Nl                 = nr_ul_pdcch_params->Nl;
      harq_process_ul_ue->nb_rb              = nr_ul_pdcch_params->nb_rb;
      harq_process_ul_ue->first_rb           = nr_ul_pdcch_params->first_rb;
      harq_process_ul_ue->number_of_symbols  = nr_ul_pdcch_params->n_symb;
      harq_process_ul_ue->num_of_mod_symbols = N_RE_prime*nr_ul_pdcch_params->nb_rb*nr_ul_pdcch_params->nb_codewords;
      harq_process_ul_ue->rvidx              = nr_ul_pdcch_params->rvidx;
      harq_process_ul_ue->a                  = test_input;
      harq_process_ul_ue->TBS                = nr_compute_tbs(nr_ul_pdcch_params->Imcs,
                                                              nr_ul_pdcch_params->nb_rb,
                                                              nr_ul_pdcch_params->n_symb,
                                                              ulsch_ue->nb_re_dmrs,
                                                              length_dmrs,
                                                              nr_ul_pdcch_params->Nl);

    }

  }

  return 0;
}



void nr_pusch_codeword_scrambling(uint8_t *in,
                         uint16_t size,
                         uint32_t Nid,
                         uint32_t n_RNTI,
                         uint32_t* out) {

  uint8_t reset, b_idx;
  uint32_t x1, x2, s=0, temp_out;

  reset = 1;
  x2 = (n_RNTI<<15) + Nid;

  for (int i=0; i<size; i++) {
    b_idx = i&0x1f;
    if (b_idx==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
      if (i)
        out++;
    }
    if (in[i]==NR_PUSCH_x)
      *out ^= 1<<b_idx;
    else if (in[i]==NR_PUSCH_y){
      if (b_idx!=0)
        *out ^= (*out & (1<<(b_idx-1)))<<1;
      else{

        temp_out = *(out-1);
        *out ^= temp_out>>31;

      }
    }
    else
      *out ^= (((in[i])&1) ^ ((s>>b_idx)&1))<<b_idx;
    //printf("i %d b_idx %d in %d s 0x%08x out 0x%08x\n", i, b_idx, in[i], s, *out);
  }

}


void pusch_transform_precoding(NR_UE_ULSCH_t *ulsch, NR_DL_FRAME_PARMS *frame_parms, int harq_pid){

  NR_UL_UE_HARQ_t *harq_process;
  int x[8192]__attribute__ ((aligned (32))); // 8192 is the maximum number of fft bins
  uint32_t *dmod;
  int sc, pusch_symb, pusch_sc;
  int symb, k, l, num_mod_symb;

  harq_process = ulsch->harq_processes[harq_pid];

#ifdef DEBUG_SCFDMA
  debug_scfdma = fopen("debug_scfdma.txt","w");
#endif

  dmod = ulsch->d_mod;
  pusch_symb = ulsch->Nsymb_pusch;
  pusch_sc = ulsch->Nsc_pusch;
  num_mod_symb = harq_process->num_of_mod_symbols;

  void (*dft)(int16_t *,int16_t *, int);

  switch (frame_parms->ofdm_symbol_size) {
  case 128:
    dft = dft128;
    break;

  case 256:
    dft = dft256;
    break;

  case 512:
    dft = dft512;
    break;

  case 1024:
    dft = dft1024;
    break;

  case 1536:
    dft = dft1536;
    break;

  case 2048:
    dft = dft2048;
    break;

  case 4096:
    dft = dft4096;
    break;

  case 8192:
    dft = dft8192;
    break;

  default:
    dft = dft512;
    break;
  }

  k = 0;
  symb = 0;

  for(l = 0; l < pusch_symb; l++){

    for (sc = 0; sc < pusch_sc; sc++){

      ((int16_t *)x)[sc*2] = (symb<num_mod_symb)?(AMP*((int16_t *)dmod)[symb*2])>>15:0;
      ((int16_t *)x)[sc*2 + 1] = (symb<num_mod_symb)?(AMP*((int16_t *)dmod)[symb*2 + 1])>>15:0;

  #ifdef DEBUG_SCFDMA
      fprintf(debug_scfdma, "x[%d] = %d\n", symb*2, ((int16_t *)x)[sc*2] );
      fprintf(debug_scfdma, "x[%d] = %d\n", symb*2 + 1, ((int16_t *)x)[sc*2 + 1] );
  #endif

      symb++;

    }


    dft((int16_t *)x, (int16_t *)&ulsch->y[l*pusch_sc], 1);

  }

#ifdef DEBUG_SCFDMA

  for (symb = 0; symb < num_mod_symb; symb++)
  {
    fprintf(debug_scfdma, "ulsch->y[%d] = %d\n", symb*2, ((int16_t *)ulsch->y)[symb*2] );
    fprintf(debug_scfdma, "ulsch->y[%d] = %d\n", symb*2 + 1, ((int16_t *)ulsch->y)[symb*2 + 1] );
  }

  fclose(debug_scfdma);
#endif

}



uint8_t nr_ue_ulsch_procedures(PHY_VARS_NR_UE *UE,
                               unsigned char harq_pid,
                               uint8_t slot,
                               uint8_t thread_id,
                               int eNB_id) {

  unsigned int available_bits;
  uint8_t mod_order, cwd_index, num_of_codewords;
  uint32_t scrambled_output[NR_MAX_NB_CODEWORDS][NR_MAX_PDSCH_ENCODED_LENGTH>>5];
  int32_t *mod_symbols[MAX_NUM_NR_RE];
  uint32_t ***pusch_dmrs;
  int16_t **tx_layers;
  int32_t **txdataF;
  uint16_t start_sc, start_rb;
  int8_t Wf[2], Wt[2], l0, l_prime[2], delta;
  uint16_t n_dmrs;
  uint8_t dmrs_type;
  uint8_t mapping_type;
  int ap, start_symbol, i;
  int sample_offsetF;

  NR_UE_ULSCH_t *ulsch_ue;
  NR_UL_UE_HARQ_t *harq_process_ul_ue;
  NR_DL_FRAME_PARMS *frame_parms = &UE->frame_parms;
  NR_UE_PUSCH *pusch_ue = UE->pusch_vars[thread_id][eNB_id];

  num_of_codewords = 1; // tmp assumption

  for (cwd_index = 0;cwd_index < num_of_codewords; cwd_index++) {

    ulsch_ue = UE->ulsch[thread_id][eNB_id][cwd_index];
    harq_process_ul_ue = ulsch_ue->harq_processes[harq_pid];

    /////////////////////////ULSCH coding/////////////////////////
    ///////////

    nr_ulsch_encoding(ulsch_ue, frame_parms, harq_pid);

    ///////////
    ////////////////////////////////////////////////////////////////////

    /////////////////////////ULSCH scrambling/////////////////////////
    ///////////

    mod_order      = nr_get_Qm(harq_process_ul_ue->mcs, 1);

    available_bits = nr_get_G(harq_process_ul_ue->nb_rb,
                              ulsch_ue->Nsymb_pusch,
                              ulsch_ue->nb_re_dmrs,
                              ulsch_ue->length_dmrs,
                              mod_order,
                              1);

    memset(scrambled_output[cwd_index], 0, ((available_bits>>5)+1)*sizeof(uint32_t));

    nr_pusch_codeword_scrambling(ulsch_ue->g,
                                 available_bits,
                                 ulsch_ue->Nid_cell,
                                 ulsch_ue->rnti,
                                 scrambled_output[cwd_index]); // assume one codeword for the moment


    /////////////
    //////////////////////////////////////////////////////////////////////////

    /////////////////////////ULSCH modulation/////////////////////////
    ///////////

    nr_modulation(scrambled_output[cwd_index], // assume one codeword for the moment
                  available_bits,
                  mod_order,
                  (int16_t *)ulsch_ue->d_mod);

    pusch_transform_precoding(ulsch_ue, frame_parms, harq_pid);

    ///////////
    ////////////////////////////////////////////////////////////////////////

    mod_symbols[cwd_index] = (int32_t *)malloc16((NR_MAX_PUSCH_ENCODED_LENGTH)*sizeof(int32_t*));

    memcpy(mod_symbols[cwd_index],ulsch_ue->d_mod,(available_bits/mod_order)*sizeof(int32_t));

  }

  start_symbol = 14 - ulsch_ue->Nsymb_pusch;

  /////////////////////////DMRS Modulation/////////////////////////
  ///////////

  pusch_dmrs = UE->nr_gold_pusch_dmrs[slot];
  n_dmrs = (harq_process_ul_ue->nb_rb*ulsch_ue->nb_re_dmrs);
  int16_t mod_dmrs[n_dmrs<<1];
  dmrs_type = UE->dmrs_UplinkConfig.pusch_dmrs_type;
  mapping_type = UE->pusch_config.pusch_TimeDomainResourceAllocation[0]->mappingType;

  l0 = get_l0_ul(mapping_type, 2);
  nr_modulation(pusch_dmrs[l0][0], n_dmrs*2, DMRS_MOD_ORDER, mod_dmrs); // currently only codeword 0 is modulated. Qm = 2 as DMRS is QPSK modulated


  ///////////
  ////////////////////////////////////////////////////////////////////////

  /////////////////////////ULSCH layer mapping/////////////////////////
  ///////////

  tx_layers = (int16_t **)pusch_ue->txdataF_layers;

  nr_layer_mapping((int16_t **)mod_symbols,
                   harq_process_ul_ue->Nl,
                   available_bits/mod_order,
                   tx_layers);


  ///////////
  ////////////////////////////////////////////////////////////////////////

  /////////////////////////ULSCH RE mapping/////////////////////////
  ///////////

  txdataF = UE->common_vars.txdataF;

  start_rb = harq_process_ul_ue->first_rb;
  start_sc = frame_parms->first_carrier_offset + start_rb*NR_NB_SC_PER_RB;

  if (start_sc >= frame_parms->ofdm_symbol_size)
    start_sc -= frame_parms->ofdm_symbol_size;

  for (ap=0; ap<harq_process_ul_ue->Nl; ap++) {

    // DMRS params for this ap
    get_Wt(Wt, ap, dmrs_type);
    get_Wf(Wf, ap, dmrs_type);
    delta = get_delta(ap, dmrs_type);
    l_prime[0] = 0; // single symbol ap 0
    uint8_t dmrs_symbol = l0+l_prime[0]; // Assuming dmrs-AdditionalPosition = 0

    uint8_t k_prime=0, l;
    uint16_t m=0, n=0, dmrs_idx=0, k=0;

    for (l=start_symbol; l<start_symbol+ulsch_ue->Nsymb_pusch; l++) {

      k = start_sc;

      for (i=0; i<harq_process_ul_ue->nb_rb*NR_NB_SC_PER_RB; i++) {

        sample_offsetF = l*frame_parms->ofdm_symbol_size + k;

        if ((l == dmrs_symbol) && (k == ((start_sc+get_dmrs_freq_idx_ul(n, k_prime, delta, dmrs_type))%(frame_parms->ofdm_symbol_size)))) {

          ((int16_t*)txdataF[ap])[(sample_offsetF)<<1] = (Wt[l_prime[0]]*Wf[k_prime]*AMP*mod_dmrs[dmrs_idx<<1]) >> 15;
          ((int16_t*)txdataF[ap])[((sample_offsetF)<<1) + 1] = (Wt[l_prime[0]]*Wf[k_prime]*AMP*mod_dmrs[(dmrs_idx<<1) + 1]) >> 15;

          #ifdef DEBUG_PUSCH_MAPPING
            printf("dmrs_idx %d\t l %d \t k %d \t k_prime %d \t n %d \t txdataF: %d %d\n",
            dmrs_idx, l, k, k_prime, n, ((int16_t*)txdataF[ap])[(sample_offsetF)<<1],
            ((int16_t*)txdataF[ap])[((sample_offsetF)<<1) + 1]);
          #endif

          dmrs_idx++;
          k_prime++;
          k_prime&=1;
          n+=(k_prime)?0:1;
        }

        else {

          ((int16_t*)txdataF[ap])[(sample_offsetF)<<1] = (AMP * tx_layers[ap][m<<1]) >> 15;
          ((int16_t*)txdataF[ap])[((sample_offsetF)<<1) + 1] = (AMP * tx_layers[ap][(m<<1) + 1]) >> 15;

          #ifdef DEBUG_PUSCH_MAPPING
            printf("m %d\t l %d \t k %d \t txdataF: %d %d\n",
            m, l, k, ((int16_t*)txdataF[ap])[(sample_offsetF)<<1],
            ((int16_t*)txdataF[ap])[((sample_offsetF)<<1) + 1]);
          #endif

          m++;
        }

        if (++k >= frame_parms->ofdm_symbol_size)
          k -= frame_parms->ofdm_symbol_size;
      }
    }
  }

  ///////////
  ////////////////////////////////////////////////////////////////////////

  return 0;
}


uint8_t nr_ue_pusch_common_procedures(PHY_VARS_NR_UE *UE,
                                      uint8_t slot,
                                      uint8_t Nl,
                                      NR_DL_FRAME_PARMS *frame_parms) {

  int tx_offset, ap;
  int32_t **txdata;
  int32_t **txdataF;

  /////////////////////////IFFT///////////////////////
  ///////////

  tx_offset = slot*frame_parms->samples_per_slot;
  txdata = UE->common_vars.txdata;
  txdataF = UE->common_vars.txdataF;

  for (ap = 0; ap < Nl; ap++) {
      if (frame_parms->Ncp == 1) { // extended cyclic prefix
  PHY_ofdm_mod(txdataF[ap],
         &txdata[ap][tx_offset],
         frame_parms->ofdm_symbol_size,
         12,
         frame_parms->nb_prefix_samples,
         CYCLIC_PREFIX);
      } else { // normal cyclic prefix
  nr_normal_prefix_mod(txdataF[ap],
           &txdata[ap][tx_offset],
           14,
           frame_parms);
      }
    }

  ///////////
  ////////////////////////////////////////////////////
  return 0;
}
