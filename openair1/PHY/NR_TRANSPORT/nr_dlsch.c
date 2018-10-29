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

#include "nr_dlsch.h"
#include "nr_dci.h"
#include "nr_sch_dmrs.h"

#define DEBUG_DLSCH

uint8_t mod_order[5] = {1, 2, 4, 6, 8};
uint16_t mod_offset[5] = {1,3,7,23,87};

void nr_pdsch_codeword_scrambling(uint8_t *in,
                         uint8_t size,
                         uint8_t q,
                         uint32_t Nid,
                         uint32_t n_RNTI,
                         uint32_t* out) {

  uint8_t reset;
  uint32_t x1, x2, s=0;

  reset = 1;
  x2 = (n_RNTI<<15) + (q<<14) + Nid;

  for (int i=0; i<size; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
    }
    *out ^= ((in[i])&1) ^ ((s>>i)&1);
  }

}

void nr_modulation(uint32_t *in,
                   uint16_t length,
                   nr_mod_t modulation_type,
                   int16_t *out) {

  uint16_t offset;
	uint16_t order;
	order = mod_order[modulation_type];
	offset = mod_offset[modulation_type];
   
  for (int i=0; i<length/order; i++) {
    uint8_t idx = 0, b_idx;

    for (int j=0; j<order; j++) {
      b_idx = (i*order+j)&0x1f;
      if (i && (!b_idx))
        in++;
      idx ^= (((*in)>>b_idx)&1)<<(order-j-1);
    }

    out[i<<1] = nr_mod_table[(offset+idx)<<1];
    out[(i<<1)+1] = nr_mod_table[((offset+idx)<<1)+1];
  }
}

void nr_pdsch_codeword_modulation(uint32_t *in,
                         uint8_t  Qm,
                         uint32_t length,
                         int16_t *out) {

  uint16_t offset = (Qm==2)? NR_MOD_TABLE_QPSK_OFFSET : (Qm==4)? NR_MOD_TABLE_QAM16_OFFSET : \
                    (Qm==6)? NR_MOD_TABLE_QAM64_OFFSET: (Qm==8)? NR_MOD_TABLE_QAM256_OFFSET : 0;
  AssertFatal(offset, "Invalid modulation order %d\n", Qm);

  for (int i=0; i<length/Qm; i++) {
    uint8_t idx = 0, b_idx;

    for (int j=0; j<Qm; j++) {
      b_idx = (i*Qm+j)&0x1f;
      if (i && (!b_idx))
        in++;
      idx ^= (((*in)>>b_idx)&1)<<(Qm-j-1);
    }

    out[i<<1] = nr_mod_table[(offset+idx)<<1];
    out[(i<<1)+1] = nr_mod_table[((offset+idx)<<1)+1];
  }
}

void nr_pdsch_layer_mapping(int16_t **mod_symbs,
                         uint8_t n_layers,
                         uint16_t n_symbs,
                         int16_t **tx_layers) {

  switch (n_layers) {

    case 1:
      memcpy((void*)tx_layers[0], (void*)mod_symbs[0], (n_symbs<<1)*sizeof(int16_t));
    break;

    case 2:
    case 3:
    case 4:
      for (int i=0; i<n_symbs/n_layers; i++)
        for (int l=0; l<n_layers; l++) {
          tx_layers[l][i<<1] = mod_symbs[0][(n_layers*i+l)<<1];
          tx_layers[l][(i<<1)+1] = mod_symbs[0][((n_layers*i+l)<<1)+1];
        }
    break;

    case 5:
      for (int i=0; i<n_symbs>>1; i++)
        for (int l=0; l<2; l++) {
          tx_layers[l][i<<1] = mod_symbs[0][((i<<1)+l)<<1];
          tx_layers[l][(i<<1)+1] = mod_symbs[0][(((i<<1)+l)<<1)+1];
        }
      for (int i=0; i<n_symbs/3; i++)
        for (int l=2; l<5; l++) {
          tx_layers[l][i<<1] = mod_symbs[1][(3*i+l)<<1];
          tx_layers[l][(i<<1)+1] = mod_symbs[1][((3*i+l)<<1)+1];
      }
    break;

    case 6:
      for (int q=0; q<2; q++)
        for (int i=0; i<n_symbs/3; i++)
          for (int l=0; l<3; l++) {
            tx_layers[l][i<<1] = mod_symbs[q][(3*i+l)<<1];
            tx_layers[l][(i<<1)+1] = mod_symbs[q][((3*i+l)<<1)+1];
          }
    break;

    case 7:
      for (int i=0; i<n_symbs/3; i++)
        for (int l=0; l<3; l++) {
          tx_layers[l][i<<1] = mod_symbs[1][(3*i+l)<<1];
          tx_layers[l][(i<<1)+1] = mod_symbs[1][((3*i+l)<<1)+1];
        }
      for (int i=0; i<n_symbs/4; i++)
        for (int l=3; l<7; l++) {
          tx_layers[l][i<<1] = mod_symbs[0][((i<<2)+l)<<1];
          tx_layers[l][(i<<1)+1] = mod_symbs[0][(((i<<2)+l)<<1)+1];
        }
    break;

    case 8:
      for (int q=0; q<2; q++)
        for (int i=0; i<n_symbs>>2; i++)
          for (int l=0; l<3; l++) {
            tx_layers[l][i<<1] = mod_symbs[q][((i<<2)+l)<<1];
            tx_layers[l][(i<<1)+1] = mod_symbs[q][(((i<<2)+l)<<1)+1];
          }
    break;

  default:
  AssertFatal(0, "Invalid number of layers %d\n", n_layers);
  }
}

uint8_t nr_generate_pdsch(NR_gNB_DLSCH_t dlsch,
                          NR_gNB_DCI_ALLOC_t dci_alloc,
                          uint32_t **pdsch_dmrs,
                          int32_t** txdataF,
                          int16_t amp,
                          uint8_t subframe,
                          NR_DL_FRAME_PARMS frame_parms,
                          nfapi_nr_config_request_t config) {

  NR_DL_gNB_HARQ_t *harq = dlsch.harq_processes[dci_alloc.harq_pid];
  nfapi_nr_dl_config_dlsch_pdu_rel15_t *rel15 = &harq->dlsch_pdu.dlsch_pdu_rel15;
  nfapi_nr_dl_config_pdcch_parameters_rel15_t pdcch_params = dci_alloc.pdcch_params;
  uint32_t scrambled_output[NR_MAX_NB_CODEWORDS][NR_MAX_PDSCH_ENCODED_LENGTH];
  int16_t mod_symbs[NR_MAX_NB_CODEWORDS][NR_MAX_PDSCH_ENCODED_LENGTH>>1];
  int16_t tx_layers[NR_MAX_NB_LAYERS][NR_MAX_PDSCH_ENCODED_LENGTH>>1];
  uint16_t n_symbs;
  int8_t Wf[2], Wt[2], l0, delta;
  uint16_t TBS = rel15->transport_block_size;
  uint8_t Qm = rel15->modulation_order;

  /// CRC, coding, interleaving and rate matching
  nr_dlsch_encoding(harq->pdu, subframe, &dlsch, &frame_parms);

  /// scrambling
  uint16_t n_RNTI = (pdcch_params.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC)? \
  ((pdcch_params.scrambling_id)?pdcch_params.rnti:0) : 0;
  uint16_t Nid = (pdcch_params.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC)? \
  pdcch_params.scrambling_id : config.sch_config.physical_cell_id.value;
  for (int q=0; q<rel15->nb_codewords; q++)
    nr_pdsch_codeword_scrambling(harq->f,
                         TBS,
                         q,
                         Nid,
                         n_RNTI,
                         scrambled_output[q]);
#ifdef DEBUG_DLSCH
//printf("PDSCH Scrambling(TBS %d): before  \t after \n", TBS);
//for (int i=0; i<TBS; i++) {
  //printf("%d\t%d\n", harq->f[i], scrambled_output[0][i]);
//}
#endif

 
  /// Modulation
  n_symbs = TBS/Qm;
  for (int q=0; q<rel15->nb_codewords; q++)
    nr_pdsch_codeword_modulation(scrambled_output[q],
                         Qm,
                         TBS,
                         mod_symbs[q]);
#ifdef DEBUG_DLSCH
printf("PDSCH Modulation: Qm %d()\n", Qm, n_symbs);
for (int i=0; i<n_symbs; i++) {
  for (int j=0; j<Qm; j++) {
    printf("%d %d\t", mod_symbs[0][(i*Qm+j)<<1], mod_symbs[0][((i*Qm+j)<<1)+1]);
  }
  printf("\n");
}
#endif


  /// Layer mapping
  nr_pdsch_layer_mapping(mod_symbs,
                         rel15->nb_layers,
                         n_symbs,
                         tx_layers);

  /// Antenna port mapping
    //to be moved to init phase potentially, for now tx_layers 1-8 are mapped on antenna ports 1000-1007

  /// DMRS QPSK modulation
  uint16_t n_dmrs = rel15->n_prb*rel15->nb_re_dmrs;
  int16_t mod_dmrs[n_dmrs<<1];
  uint8_t dmrs_type = config.pdsch_config.dmrs_type.value;
  l0 = get_l0(dmrs_type, 2);//config.pdsch_config.dmrs_typeA_position.value);
  nr_modulation(pdsch_dmrs[l0], n_dmrs, MOD_QPSK, mod_dmrs);

  /// Resource mapping
  AssertFatal(rel15->nb_layers<=config.rf_config.tx_antenna_ports.value, "Not enough Tx antennas (%d) for %d layers\n",\
   config.rf_config.tx_antenna_ports.value, rel15->nb_layers);

    // Non interleaved VRB to PRB mapping
  uint8_t start_sc = frame_parms.first_carrier_offset + rel15->start_prb*NR_NB_SC_PER_RB +\
  ((pdcch_params.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_COMMON) && (pdcch_params.dci_format == NFAPI_NR_DL_DCI_FORMAT_1_0))?\
       (((int)floor(frame_parms.ssb_start_subcarrier/NR_NB_SC_PER_RB) + pdcch_params.rb_offset)*NR_NB_SC_PER_RB) : 0;

  for (int ap=0; ap<rel15->nb_layers; ap++) {

    // DMRS params for this ap
    get_Wt(Wt, ap, dmrs_type);
    get_Wf(Wf, ap, dmrs_type);
    delta = get_delta(ap, dmrs_type);
    uint8_t k_prime=0, n=0, dmrs_idx=0;
    uint16_t m = 0;

    for (int l=rel15->start_symbol; l<rel15->nb_symbols; l++)
      for (int k=start_sc; k<rel15->n_prb*NR_NB_SC_PER_RB; k++) {
        if (k >= frame_parms.ofdm_symbol_size)
          k -= frame_parms.ofdm_symbol_size;

        if ((l==l0) && (k == ((dmrs_type)? (6*n+k_prime+delta):((n<<2)+(k_prime<<1)+delta)))) {
          ((int16_t*)txdataF[ap])[(l*frame_parms.ofdm_symbol_size + k)<<1] = (Wt[k_prime]*Wf[k_prime]*amp/2*mod_dmrs[dmrs_idx<<1]) >> 15;
          ((int16_t*)txdataF[ap])[((l*frame_parms.ofdm_symbol_size + k)<<1) + 1] = (Wt[k_prime]*Wf[k_prime]*amp/2*mod_dmrs[(dmrs_idx<<1) + 1]) >> 15;
          dmrs_idx++;
          n++;
          k_prime++;
          k_prime&=1;
        }

        ((int16_t*)txdataF[ap])[(l*frame_parms.ofdm_symbol_size + k)<<1] = (amp * tx_layers[ap][m<<1]) >> 15;
        ((int16_t*)txdataF[ap])[((l*frame_parms.ofdm_symbol_size + k)<<1) + 1] = (amp * tx_layers[ap][(m<<1) + 1]) >> 15;
        m++;
      }
  }

#ifdef DEBUG_DLSCH
  write_output("txdataF_dlsch.m", "txdataF_dlsch", txdataF[0], frame_parms.samples_per_subframe_wCP>>1, 1, 1);
#endif

  return 0;
}
