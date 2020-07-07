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
                   uint32_t length,
                   uint16_t mod_order,
                   int16_t *out)
{
  uint16_t offset;
  uint8_t idx, b_idx;

  offset = (mod_order==2)? NR_MOD_TABLE_QPSK_OFFSET : (mod_order==4)? NR_MOD_TABLE_QAM16_OFFSET : \
                    (mod_order==6)? NR_MOD_TABLE_QAM64_OFFSET: (mod_order==8)? NR_MOD_TABLE_QAM256_OFFSET : 0;

  for (int i=0; i<length/mod_order; i++)
  {
    idx = 0;
    for (int j=0; j<mod_order; j++)
    {
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
  LOG_D(PHY,"Doing layer mapping for %d layers, %d symbols\n",n_layers,n_symbs);

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

void nr_ue_layer_mapping(NR_UE_ULSCH_t **ulsch_ue,
                      uint8_t n_layers,
                      uint16_t n_symbs,
                      int16_t **tx_layers) {

  int16_t *mod_symbs;

  switch (n_layers) {

    case 1:
      mod_symbs = (int16_t *)ulsch_ue[0]->d_mod;
      for (int i=0; i<n_symbs; i++){
        tx_layers[0][i<<1] = (mod_symbs[i<<1]*AMP)>>15;
        tx_layers[0][(i<<1)+1] = (mod_symbs[(i<<1)+1]*AMP)>>15;
      }
    break;

    case 2:
    case 3:
    case 4:
      mod_symbs = (int16_t *)ulsch_ue[0]->d_mod;

      for (int i=0; i<n_symbs/n_layers; i++){
        for (int l=0; l<n_layers; l++) {
          tx_layers[l][i<<1] = (mod_symbs[(n_layers*i+l)<<1]*AMP)>>15;
          tx_layers[l][(i<<1)+1] = (mod_symbs[((n_layers*i+l)<<1)+1]*AMP)>>15;
        }
      }
    break;

    case 5:
      mod_symbs = (int16_t *)ulsch_ue[0]->d_mod;

      for (int i=0; i<n_symbs>>1; i++)
        for (int l=0; l<2; l++) {
          tx_layers[l][i<<1] = (mod_symbs[((i<<1)+l)<<1]*AMP)>>15;
          tx_layers[l][(i<<1)+1] = (mod_symbs[(((i<<1)+l)<<1)+1]*AMP)>>15;
        }

      mod_symbs = (int16_t *)ulsch_ue[1]->d_mod;

      for (int i=0; i<n_symbs/3; i++)
        for (int l=2; l<5; l++) {
          tx_layers[l][i<<1] = (mod_symbs[(3*i+l)<<1]*AMP)>>15;
          tx_layers[l][(i<<1)+1] = (mod_symbs[((3*i+l)<<1)+1]*AMP)>>15;
      }
    break;

    case 6:
      for (int q=0; q<2; q++){

        mod_symbs = (int16_t *)ulsch_ue[q]->d_mod;

        for (int i=0; i<n_symbs/3; i++)
          for (int l=0; l<3; l++) {
            tx_layers[l][i<<1] = (mod_symbs[(3*i+l)<<1]*AMP)>>15;
            tx_layers[l][(i<<1)+1] = (mod_symbs[((3*i+l)<<1)+1]*AMP)>>15;
          }
      }
    break;

    case 7:
      mod_symbs = (int16_t *)ulsch_ue[1]->d_mod;

      for (int i=0; i<n_symbs/3; i++)
        for (int l=0; l<3; l++) {
          tx_layers[l][i<<1] = (mod_symbs[(3*i+l)<<1]*AMP)>>15;
          tx_layers[l][(i<<1)+1] = (mod_symbs[((3*i+l)<<1)+1]*AMP)>>15;
        }

      mod_symbs = (int16_t *)ulsch_ue[0]->d_mod;

      for (int i=0; i<n_symbs/4; i++)
        for (int l=3; l<7; l++) {
          tx_layers[l][i<<1] = (mod_symbs[((i<<2)+l)<<1]*AMP)>>15;
          tx_layers[l][(i<<1)+1] = (mod_symbs[(((i<<2)+l)<<1)+1]*AMP)>>15;
        }
    break;

    case 8:
      for (int q=0; q<2; q++){

        mod_symbs = (int16_t *)ulsch_ue[q]->d_mod;

        for (int i=0; i<n_symbs>>2; i++)
          for (int l=0; l<3; l++) {
            tx_layers[l][i<<1] = (mod_symbs[((i<<2)+l)<<1]*AMP)>>15;
            tx_layers[l][(i<<1)+1] = (mod_symbs[(((i<<2)+l)<<1)+1]*AMP)>>15;
          }
      }
    break;

  default:
  AssertFatal(0, "Invalid number of layers %d\n", n_layers);
  }
}


void nr_dft(int32_t *z, int32_t *d, uint32_t Msc_PUSCH)
{
#if defined(__x86_64__) || defined(__i386__)
  __m128i dft_in128[1][1200], dft_out128[1][1200];
#elif defined(__arm__)
  int16x8_t dft_in128[1][1200], dft_out128[1][1200];
#endif
  uint32_t *dft_in0 = (uint32_t*)dft_in128[0], *dft_out0 = (uint32_t*)dft_out128[0];

  uint32_t i, ip;

#if defined(__x86_64__) || defined(__i386__)
  __m128i norm128;
#elif defined(__arm__)
  int16x8_t norm128;
#endif

  for (i = 0, ip = 0; i < Msc_PUSCH; i++, ip+=4) {
    dft_in0[ip] = d[i];
  }

  switch (Msc_PUSCH) {
    case 12:
      dft(DFT_12,(int16_t *)dft_in0, (int16_t *)dft_out0,0);

#if defined(__x86_64__) || defined(__i386__)
      norm128 = _mm_set1_epi16(9459);
#elif defined(__arm__)
      norm128 = vdupq_n_s16(9459);
#endif
      for (i=0; i<12; i++) {
#if defined(__x86_64__) || defined(__i386__)
        ((__m128i*)dft_out0)[i] = _mm_slli_epi16(_mm_mulhi_epi16(((__m128i*)dft_out0)[i], norm128), 1);
#elif defined(__arm__)
        ((int16x8_t*)dft_out0)[i] = vqdmulhq_s16(((int16x8_t*)dft_out0)[i], norm128);
#endif
      }

      break;

    case 24:
      dft(DFT_24,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 36:
      dft(DFT_36,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 48:
      dft(DFT_48,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 60:
      dft(DFT_60,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 72:
      dft(DFT_72,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 96:
      dft(DFT_96,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 108:
      dft(DFT_108,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 120:
      dft(DFT_120,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 144:
      dft(DFT_144,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 180:
      dft(DFT_180,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 192:
      dft(DFT_192,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 216:
      dft(DFT_216,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 240:
      dft(DFT_240,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 288:
      dft(DFT_288,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 300:
      dft(DFT_300,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 324:
      dft(DFT_324,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 360:
      dft(DFT_360,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 384:
      dft(DFT_384,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 432:
      dft(DFT_432,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 480:
      dft(DFT_480,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 540:
      dft(DFT_540,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 576:
      dft(DFT_576,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 600:
      dft(DFT_600,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 648:
      dft(DFT_648,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 720:
      dft(DFT_720,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 768:
      dft(DFT_768,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 864:
      dft(DFT_864,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 900:
      dft(DFT_900,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 960:
      dft(DFT_960,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 972:
      dft(DFT_960,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 1080:
      dft(DFT_1080,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 1152:
      dft(DFT_1152,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 1200:
      dft(DFT_1200,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;
  }

  for (i = 0, ip = 0; i < Msc_PUSCH; i++, ip+=4) {
    z[i] = dft_out0[ip];
  }
}
