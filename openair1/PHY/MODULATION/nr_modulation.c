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

#include "nr_modulation.h"

extern short nr_mod_table[NR_MOD_TABLE_SIZE_SHORT];

void nr_modulation(uint32_t *in,
                   uint16_t length,
                   uint16_t mod_order,
                   int16_t *out) {

  uint16_t offset;
	uint16_t order;
	int i,j;
  uint8_t idx, b_idx;

  offset = (mod_order==2)? NR_MOD_TABLE_QPSK_OFFSET : (mod_order==4)? NR_MOD_TABLE_QAM16_OFFSET : \
                    (mod_order==6)? NR_MOD_TABLE_QAM64_OFFSET: (mod_order==8)? NR_MOD_TABLE_QAM256_OFFSET : 0;

  for (i=0; i<length/mod_order; i++) {
    idx = 0;

    for (j=0; j<mod_order; j++) {
      b_idx = (i*mod_order+j)&0x1f;
      if (i && (!b_idx))
        in++;
      idx ^= (((*in)>>b_idx)&1)<<(mod_order-j-1);
    }

    out[i<<1] = nr_mod_table[(offset+idx)<<1];
    out[(i<<1)+1] = nr_mod_table[((offset+idx)<<1)+1];
  }
}

void nr_layer_mapping(int16_t **mod_symbs,
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