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

void nr_pdsch_codeword_scrambling(uint32_t *in,
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
    *out ^= (((*in)>>i)&1) ^ ((s>>i)&1);
  }

}

void nr_pdsch_codeword_modulation(uint32_t *in,
                         uint8_t  Qm,
                         uint32_t length,
                         uint16_t *out) {

  uint16_t offset = (Qm==2)? NR_MOD_TABLE_QPSK_OFFSET : (Qm==4)? NR_MOD_TABLE_QAM16_OFFSET : \
                    (Qm==6)? NR_MOD_TABLE_QAM64_OFFSET: (Qm==8)? NR_MOD_TABLE_QAM256_OFFSET : 0;
  AssertFatal(offset, "Invalid modulation order %d\n", Qm);

  for (int i=0; i<length/Qm; i++) {
    uint8_t idx = 0, b_idx;

    for (int j=0; j<Qm; j++) {
      b_idx = (i*Qm+j)&0x1f;
      if (i && (!b_idx))
        in++;
      idx ^= (((*in)>>b_idx)&1)<<(Qm-j);
    }

    out[i<<1] = nr_mod_table[(offset+idx)<<1];
    out[(i<<1)+1] = nr_mod_table[((offset+idx)<<1)+1];
  }
}

void nr_pdsch_layer_mapping(uint16_t **mod_symbs,
                         uint8_t n_codewords,
                         uint8_t n_layers,
                         uint16_t *n_symbs,
                         uint16_t **tx_layers) {

  switch (n_layers) {

    case 1:
      memcpy((void*)tx_layers[0], (void*)mod_symbs[0], (n_symbs[0]<<1)*sizeof(uint16_t));
    break;

    case 2:
    case 3:
    case 4:
      for (int i=0; i<n_symbs[0]/n_layers; i++)
        for (int l=0; l<n_layers; l++) {
          tx_layers[l][i<<1] = mod_symbs[0][(n_layers*i+l)<<1];
          tx_layers[l][(i<<1)+1] = mod_symbs[0][((n_layers*i+l)<<1)+1];
        }
    break;

    case 5:
      for (int i=0; i<n_symbs[0]>>1; i++)
        for (int l=0; l<2; l++) {
          tx_layers[l][i<<1] = mod_symbs[0][((i<<1)+l)<<1];
          tx_layers[l][(i<<1)+1] = mod_symbs[0][(((i<<1)+l)<<1)+1];
        }
      for (int i=0; i<n_symbs[1]/3; i++)
        for (int l=2; l<5; l++) {
          tx_layers[l][i<<1] = mod_symbs[1][(3*i+l)<<1];
          tx_layers[l][(i<<1)+1] = mod_symbs[1][((3*i+l)<<1)+1];
      }
    break;

    case 6:
      for (int q=0; q<2; q++)
        for (int i=0; i<n_symbs[q]/3; i++)
          for (int l=0; l<3; l++) {
            tx_layers[l][i<<1] = mod_symbs[q][(3*i+l)<<1];
            tx_layers[l][(i<<1)+1] = mod_symbs[q][((3*i+l)<<1)+1];
          }
    break;

    case 7:
      for (int i=0; i<n_symbs[0]/3; i++)
        for (int l=0; l<3; l++) {
          tx_layers[l][i<<1] = mod_symbs[1][(3*i+l)<<1];
          tx_layers[l][(i<<1)+1] = mod_symbs[1][((3*i+l)<<1)+1];
        }
      for (int i=0; i<n_symbs[1]/4; i++)
        for (int l=3; l<7; l++) {
          tx_layers[l][i<<1] = mod_symbs[0][((i<<2)+l)<<1];
          tx_layers[l][(i<<1)+1] = mod_symbs[0][(((i<<2)+l)<<1)+1];
        }
    break;

    case 8:
      for (int q=0; q<2; q++)
        for (int i=0; i<n_symbs[q]>>2; i++)
          for (int l=0; l<3; l++) {
            tx_layers[l][i<<1] = mod_symbs[q][((i<<2)+l)<<1];
            tx_layers[l][(i<<1)+1] = mod_symbs[q][(((i<<2)+l)<<1)+1];
          }
    break;

  default:
  AsserFatal(0, "Invalid number of layers %d\n", n_layers);
  }
}

uint8_t nr_generate_pdsch(NR_gNB_DLSCH_t dlsch,
                          NR_gNB_DCI_ALLOC_t dci_alloc,
                          int32_t** txdataF,
                          int16_t amp,
                          NR_DL_FRAME_PARMS frame_parms,
                          nfapi_nr_config_request_t config) {

  NR_DL_gNB_HARQ_t *harq = dlsch.harq_processes[0];
  uint32_t scrambled_output[NR_MAX_NB_CODEWORDS][NR_MAX_PDSCH_ENCODED_LENGTH]={0};
  uint16_t mod_symbs[NR_MAX_NB_CODEWORDS][NR_MAX_PDSCH_ENCODED_LENGTH>>1] = {0};
  uint16_t tx_layers[NR_MAX_NB_LAYERS][NR_MAX_PDSCH_ENCODED_LENGTH>>1];

  /// CRC, coding, interleaving and rate matching

  /// scrambling
  uint16_t n_RNTI = (pdcch_params.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC)? ((pdcch_params.scrambling_id)?pdcch_params.rnti:0) : 0;
  uint16_t Nid = (pdcch_params.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC)? pdcch_params.scrambling_id : config.sch_config.physical_cell_id.value;
  for (int q=0; q<harq->n_codewords; q++)
    nr_pdsch_codeword_scrambling(harq->f,
                         harq->TBS,
                         q,
                         Nid,
                         n_RNTI,
                         scrambled_output[q]);
 
  /// Modulation
  for (int q=0; q<harq->n_codewords; q++)
    nr_pdsch_codeword_modulation(scrambled_output[q],
                         harq->Qm,
                         harq->TBS,
                         mod_symbs[q]);

  /// Layer mapping
  nr_pdsch_layer_mapping(mod_symbs,
                         harq->n_codewords,
                         harq->Nl,
                         n_symbs,
                         tx_layers);

  /// Antenna port mapping -- Not yet necessary

  /// Resource mapping
  
  
  return 0;
}
