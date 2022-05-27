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
#include "PHY/NR_REFSIG/nr_mod_table.h"

//Table 6.3.1.5-1 Precoding Matrix W 1 layer 2 antenna ports 'n' = -1 and 'o' = -j
char nr_W_1l_2p[6][2][1] = {
  {{'1'}, {'0'}},//pmi 0
  {{'0'}, {'1'}},
  {{'1'}, {'1'}},
  {{'1'}, {'n'}},
  {{'1'}, {'j'}},
  {{'1'}, {'o'}}//pmi 5
};

//Table 6.3.1.5-3 Precoding Matrix W 1 layer 4 antenna ports 'n' = -1 and 'o' = -j
char nr_W_1l_4p[28][4][1] = {
  {{'1'}, {'0'}, {'0'}, {'0'}},//pmi 0
  {{'0'}, {'1'}, {'0'}, {'0'}},
  {{'0'}, {'0'}, {'1'}, {'0'}},
  {{'0'}, {'0'}, {'0'}, {'1'}},
  {{'1'}, {'0'}, {'1'}, {'0'}},
  {{'1'}, {'0'}, {'n'}, {'0'}},
  {{'1'}, {'0'}, {'j'}, {'0'}},
  {{'1'}, {'0'}, {'o'}, {'0'}},//pmi 7
  {{'0'}, {'1'}, {'0'}, {'1'}},//pmi 8
  {{'0'}, {'1'}, {'0'}, {'n'}},
  {{'0'}, {'1'}, {'0'}, {'j'}},
  {{'0'}, {'1'}, {'0'}, {'o'}},
  {{'1'}, {'1'}, {'1'}, {'1'}},
  {{'1'}, {'1'}, {'j'}, {'j'}},
  {{'1'}, {'1'}, {'n'}, {'n'}},
  {{'1'}, {'1'}, {'o'}, {'o'}},
  {{'1'}, {'j'}, {'1'}, {'j'}},//pmi 16
  {{'1'}, {'j'}, {'j'}, {'n'}},
  {{'1'}, {'j'}, {'n'}, {'o'}},
  {{'1'}, {'j'}, {'o'}, {'1'}},
  {{'1'}, {'n'}, {'1'}, {'n'}},
  {{'1'}, {'n'}, {'j'}, {'o'}},
  {{'1'}, {'n'}, {'n'}, {'1'}},
  {{'1'}, {'n'}, {'o'}, {'j'}},//pmi 23
  {{'1'}, {'o'}, {'1'}, {'o'}},//pmi 24
  {{'1'}, {'o'}, {'j'}, {'1'}},
  {{'1'}, {'o'}, {'n'}, {'j'}},
  {{'1'}, {'o'}, {'o'}, {'n'}}//pmi 27
};

//Table 6.3.1.5-4 Precoding Matrix W 2 antenna ports layers 2  'n' = -1 and 'o' = -j
char nr_W_2l_2p[3][2][2] = {
  {{'1', '0'}, {'0', '1'}},//pmi 0
  {{'1', '1'}, {'1', 'n'}},
  {{'1', '1'}, {'j', 'o'}} //pmi 2
};

//Table 6.3.1.5-5 Precoding Matrix W 2 layers 4 antenna ports 'n' = -1 and 'o' = -j
char nr_W_2l_4p[22][4][2] = {
  {{'1', '0'}, {'0', '1'}, {'0', '0'}, {'0', '0'}},//pmi 0
  {{'1', '0'}, {'0', '0'}, {'0', '1'}, {'0', '0'}},
  {{'1', '0'}, {'0', '0'}, {'0', '0'}, {'0', '1'}},
  {{'0', '0'}, {'1', '0'}, {'0', '1'}, {'0', '0'}},//pmi 3
  {{'0', '0'}, {'1', '0'}, {'0', '0'}, {'0', '1'}},//pmi 4
  {{'0', '0'}, {'0', '0'}, {'1', '0'}, {'0', '1'}},
  {{'1', '0'}, {'0', '1'}, {'1', '0'}, {'0', 'o'}},
  {{'1', '0'}, {'0', '1'}, {'1', '0'}, {'0', 'j'}},
  {{'1', '0'}, {'0', '1'}, {'o', '0'}, {'0', '1'}},//pmi 8
  {{'1', '0'}, {'0', '1'}, {'o', '0'}, {'0', 'n'}},
  {{'1', '0'}, {'0', '1'}, {'n', '0'}, {'0', 'o'}},
  {{'1', '0'}, {'0', '1'}, {'n', '0'}, {'0', 'j'}},//pmi 11
  {{'1', '0'}, {'0', '1'}, {'j', '0'}, {'0', '1'}},//pmi 12
  {{'1', '0'}, {'0', '1'}, {'j', '0'}, {'0', 'n'}},
  {{'1', '1'}, {'1', '1'}, {'1', 'n'}, {'1', 'n'}},
  {{'1', '1'}, {'1', '1'}, {'j', 'o'}, {'j', 'o'}},//pmi 15
  {{'1', '1'}, {'j', 'j'}, {'1', 'n'}, {'j', 'o'}},//pmi 16
  {{'1', '1'}, {'j', 'j'}, {'j', 'o'}, {'n', '1'}},
  {{'1', '1'}, {'n', 'n'}, {'1', 'n'}, {'n', '1'}},
  {{'1', '1'}, {'n', 'n'}, {'j', 'o'}, {'o', 'j'}},//pmi 19
  {{'1', '1'}, {'o', 'o'}, {'1', 'n'}, {'o', 'j'}},
  {{'1', '1'}, {'o', 'o'}, {'j', 'o'}, {'1', 'n'}}//pmi 21
};

//Table 6.3.1.5-6 Precoding Matrix W 3 layers 4 antenna ports 'n' = -1 and 'o' = -j
char nr_W_3l_4p[7][4][3] = {
  {{'1', '0', '0'}, {'0', '1', '0'}, {'0', '0', '1'},{'0', '0', '0'}},//pmi 0
  {{'1', '0', '0'}, {'0', '1', '0'}, {'1', '0', '0'},{'0', '0', '1'}},
  {{'1', '0', '0'}, {'0', '1', '0'}, {'n', '0', '0'},{'0', '0', '1'}},
  {{'1', '1', '1'}, {'1', 'n', '1'}, {'1', '1', 'n'},{'1', 'n', 'n'}},//pmi 3
  {{'1', '1', '1'}, {'1', 'n', '1'}, {'j', 'j', 'o'},{'j', 'o', 'o'}},//pmi 4
  {{'1', '1', '1'}, {'n', '1', 'n'}, {'1', '1', 'n'},{'n', '1', '1'}},
  {{'1', '1', '1'}, {'n', '1', 'n'}, {'j', 'j', 'o'},{'o', 'j', 'j'}}
};

//Table 6.3.1.5-7 Precoding Matrix W 4 layers 4 antenna ports 'n' = -1 and 'o' = -j
char nr_W_4l_4p[5][4][4] = {
  {{'1', '0', '0', '0'}, {'0', '1', '0', '0'}, {'0', '0', '1', '0'}, {'0', '0', '0', '1'}},//pmi 0
  {{'1', '1', '0', '0'}, {'0', '0', '1', '1'}, {'1', 'n', '0', '0'}, {'0', '0', '1', 'n'}},
  {{'1', '1', '0', '0'}, {'0', '0', '1', '1'}, {'j', 'o', '0', '0'}, {'0', '0', 'j', 'o'}},
  {{'1', '1', '1', '1'}, {'1', 'n', '1', 'n'}, {'1', '1', 'n', 'n'}, {'1', 'n', 'n', '1'}},//pmi 3
  {{'1', '1', '1', '1'}, {'1', 'n', '1', 'n'}, {'j', 'j', 'o', 'o'}, {'j', 'o', 'o', 'j'}}//pmi 4
};

void nr_modulation(uint32_t *in,
                   uint32_t length,
                   uint16_t mod_order,
                   int16_t *out)
{
  uint16_t mask = ((1<<mod_order)-1);
  int32_t* nr_mod_table32;
  int32_t* out32 = (int32_t*) out;
  uint8_t* in_bytes = (uint8_t*) in;
  uint64_t* in64 = (uint64_t*) in;
  int64_t* out64 = (int64_t*) out;
  uint8_t idx;
  uint32_t i,j;
  uint32_t bit_cnt;
  uint64_t x,x1,x2;

#if defined(__SSE2__)
  __m128i *nr_mod_table128;
  __m128i *out128;
#endif

  LOG_D(PHY,"nr_modulation: length %d, mod_order %d\n",length,mod_order);

  switch (mod_order) {

#if defined(__SSE2__)
  case 2:
    nr_mod_table128 = (__m128i*) nr_qpsk_byte_mod_table;
    out128 = (__m128i*) out;
    for (i=0; i<length/8; i++)
      out128[i] = nr_mod_table128[in_bytes[i]];
    // the bits that are left out
    i = i*8/2;
    nr_mod_table32 = (int32_t*) nr_qpsk_mod_table;
    while (i<length/2) {
      idx = ((in_bytes[(i*2)/8]>>((i*2)&0x7)) & mask);
      out32[i] = nr_mod_table32[idx];
      i++;
    }
    return;
#else
  case 2:
    nr_mod_table32 = (int32_t*) nr_qpsk_mod_table;
    for (i=0; i<length/mod_order; i++) {
      idx = ((in[i*2/32]>>((i*2)&0x1f)) & mask);
      out32[i] = nr_mod_table32[idx];
    }
    return;
#endif

  case 4:
    out64 = (int64_t*) out;
    for (i=0; i<length/8; i++)
      out64[i] = nr_16qam_byte_mod_table[in_bytes[i]];
    // the bits that are left out
    i = i*8/4;
    while (i<length/4) {
      idx = ((in_bytes[(i*4)/8]>>((i*4)&0x7)) & mask);
      out32[i] = nr_16qam_mod_table[idx];
      i++;
    }
    return;

  case 6:
    j = 0;
    for (i=0; i<length/192; i++) {
      x = in64[i*3];
      x1 = x&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x>>12)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x>>24)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x>>36)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x>>48)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x2 = (x>>60);
      x = in64[i*3+1];
      x2 |= x<<4;
      x1 = x2&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x2>>12)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x2>>24)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x2>>36)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x2>>48)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x2 = ((x>>56)&0xf0) | (x2>>60);
      x = in64[i*3+2];
      x2 |= x<<8;
      x1 = x2&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x2>>12)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x2>>24)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x2>>36)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x2>>48)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x2 = ((x>>52)&0xff0) | (x2>>60);
      out64[j++] = nr_64qam_mod_table[x2];
    }
    i *= 24;
    bit_cnt = i * 8;
    while (bit_cnt < length) {
      x = *((uint32_t*)(in_bytes+i));
      x1 = x&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      x1 = (x>>12)&4095;
      out64[j++] = nr_64qam_mod_table[x1];
      i += 3;
      bit_cnt += 24;
    }
    return;

  case 8:
    nr_mod_table32 = (int32_t*) nr_256qam_mod_table;
    for (i=0; i<length/8; i++)
      out32[i] = nr_mod_table32[in_bytes[i]];
    return;

  default:
    break;
  }
  AssertFatal(false,"Invalid or unsupported modulation order %d\n",mod_order);
}

void nr_layer_mapping(int16_t **mod_symbs,
                      uint8_t n_layers,
                      uint16_t n_symbs,
                      int16_t **tx_layers)
{
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

void nr_ue_layer_mapping(int16_t *mod_symbs,
                         uint8_t n_layers,
                         uint16_t n_symbs,
                         int16_t **tx_layers) {

  for (int i=0; i<n_symbs/n_layers; i++) {
    for (int l=0; l<n_layers; l++) {
      tx_layers[l][i<<1] = (mod_symbs[(n_layers*i+l)<<1]*AMP)>>15;
      tx_layers[l][(i<<1)+1] = (mod_symbs[((n_layers*i+l)<<1)+1]*AMP)>>15;
    }
  }
}


void nr_dft(int32_t *z, int32_t *d, uint32_t Msc_PUSCH)
{
#if defined(__x86_64__) || +defined(__i386__)
  __m128i dft_in128[1][3240], dft_out128[1][3240];
#elif defined(__arm__)
  int16x8_t dft_in128[1][3240], dft_out128[1][3240];
#endif
  uint32_t *dft_in0 = (uint32_t*)dft_in128[0], *dft_out0 = (uint32_t*)dft_out128[0];

  uint32_t i, ip;

#if defined(__x86_64__) || defined(__i386__)
  __m128i norm128;
#elif defined(__arm__)
  int16x8_t norm128;
#endif

  if ((Msc_PUSCH % 1536) > 0) {
    for (i = 0, ip = 0; i < Msc_PUSCH; i++, ip+=4) {
      dft_in0[ip] = d[i];
    }
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
      dft(DFT_972,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
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

    case 1296:
      dft(DFT_1296,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 1440:
      dft(DFT_1440,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 1500:
      dft(DFT_1500,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 1536:
      //dft(DFT_1536,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      dft(DFT_1536,(int16_t*)d, (int16_t*)z, 1);
      break;

    case 1620:
      dft(DFT_1620,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 1728:
      dft(DFT_1728,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 1800:
      dft(DFT_1800,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 1920:
      dft(DFT_1920,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 1944:
      dft(DFT_1944,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 2160:
      dft(DFT_2160,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 2304:
      dft(DFT_2304,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 2400:
      dft(DFT_2400,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 2592:
      dft(DFT_2592,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 2700:
      dft(DFT_2700,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 2880:
      dft(DFT_2880,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 2916:
      dft(DFT_2916,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 3000:
      dft(DFT_3000,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    case 3072:
      //dft(DFT_3072,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      dft(DFT_3072,(int16_t*)d, (int16_t*)z, 1);
      break;

    case 3240:
      dft(DFT_3240,(int16_t*)dft_in0, (int16_t*)dft_out0, 1);
      break;

    default:
      // should not be reached
      LOG_E( PHY, "Unsupported Msc_PUSCH value of %"PRIu16"\n", Msc_PUSCH );
      return;

  }


  if ((Msc_PUSCH % 1536) > 0) {
    for (i = 0, ip = 0; i < Msc_PUSCH; i++, ip+=4)
      z[i] = dft_out0[ip];
  }

}


void init_symbol_rotation(NR_DL_FRAME_PARMS *fp) {

  uint64_t dl_CarrierFreq = fp->dl_CarrierFreq;
  uint64_t ul_CarrierFreq = fp->ul_CarrierFreq;
  double f[2] = {(double)dl_CarrierFreq, (double)ul_CarrierFreq};

  const int nsymb = fp->symbols_per_slot * fp->slots_per_frame/10;
  const double Tc=(1/480e3/4096);
  const double Nu=2048*64*(1/(float)(1<<fp->numerology_index));
  const double Ncp0=16*64 + (144*64*(1/(float)(1<<fp->numerology_index)));
  const double Ncp1=(144*64*(1/(float)(1<<fp->numerology_index)));

  for (uint8_t ll = 0; ll < 2; ll++){

    double f0 = f[ll];
    double Ncpm1 = Ncp0;
    int16_t *symbol_rotation = fp->symbol_rotation[ll];

    double tl = 0;
    double poff = 2 * M_PI * ((Ncp0 * Tc)) * f0;
    double exp_re = cos(poff);
    double exp_im = sin(-poff);
    symbol_rotation[0] = (int16_t)floor(exp_re * 32767);
    symbol_rotation[1] = (int16_t)floor(exp_im * 32767);
    LOG_I(PHY, "Doing symbol rotation calculation for gNB TX/RX, f0 %f Hz, Nsymb %d\n", f0, nsymb);
    LOG_I(PHY, "Symbol rotation %d/%d => (%d,%d)\n",
      0,
      nsymb,
      symbol_rotation[0],
      symbol_rotation[1]);

    for (int l = 1; l < nsymb; l++) {

      double Ncp;
      if (l == (7 * (1 << fp->numerology_index))) {
        Ncp = Ncp0;
      } else {
        Ncp = Ncp1;
      }

      tl += (Nu + Ncpm1) * Tc;
      poff = 2 * M_PI * (tl + (Ncp * Tc)) * f0;
      exp_re = cos(poff);
      exp_im = sin(-poff);
      symbol_rotation[l<<1] = (int16_t)floor(exp_re * 32767);
      symbol_rotation[1 + (l<<1)] = (int16_t)floor(exp_im * 32767);

      LOG_I(PHY, "Symbol rotation %d/%d => tl %f (%d,%d) (%f)\n",
        l,
        nsymb,
        tl,
        symbol_rotation[l<<1],
        symbol_rotation[1 + (l<<1)],
        (poff / 2 / M_PI) - floor(poff / 2 / M_PI));

      Ncpm1 = Ncp;

    }
  }
}

void init_timeshift_rotation(NR_DL_FRAME_PARMS *fp)
{
  const int sample_offset = fp->nb_prefix_samples / fp->ofdm_offset_divisor;
  for (int i = 0; i < fp->ofdm_symbol_size; i++) {
    double poff = -i * 2.0 * M_PI * sample_offset / fp->ofdm_symbol_size;
    double exp_re = cos(poff);
    double exp_im = sin(-poff);
    fp->timeshift_symbol_rotation[i*2] = (int16_t)round(exp_re * 32767);
    fp->timeshift_symbol_rotation[i*2+1] = (int16_t)round(exp_im * 32767);

    if (i < 10)
      LOG_I(PHY,"Timeshift symbol rotation %d => (%d,%d) %f\n",i,
            fp->timeshift_symbol_rotation[i*2],
            fp->timeshift_symbol_rotation[i*2+1],
            poff);
  }
}

int nr_layer_precoder(int16_t **datatx_F_precoding, char *prec_matrix, uint8_t n_layers, int32_t re_offset)
{
  int32_t precodatatx_F = 0;

  for (int al = 0; al<n_layers; al++) {
    int16_t antenna_re = datatx_F_precoding[al][re_offset<<1];
    int16_t antenna_im = datatx_F_precoding[al][(re_offset<<1) +1];

    switch (prec_matrix[al]) {
      case '0': //multiply by zero
        break;

      case '1': //multiply by 1
        ((int16_t *) &precodatatx_F)[0] += antenna_re;
        ((int16_t *) &precodatatx_F)[1] += antenna_im;
        break;

      case 'n': // multiply by -1
        ((int16_t *) &precodatatx_F)[0] -= antenna_re;
        ((int16_t *) &precodatatx_F)[1] -= antenna_im;
        break;

      case 'j': //
        ((int16_t *) &precodatatx_F)[0] -= antenna_im;
        ((int16_t *) &precodatatx_F)[1] += antenna_re;
        break;

      case 'o': // -j
        ((int16_t *) &precodatatx_F)[0] += antenna_im;
        ((int16_t *) &precodatatx_F)[1] -= antenna_re;
        break;
    }
  }

  return precodatatx_F;
  // normalize
  /*  ((int16_t *)precodatatx_F)[0] = (int16_t)((((int16_t *)precodatatx_F)[0]*ONE_OVER_SQRT2_Q15)>>15);
      ((int16_t *)precodatatx_F)[1] = (int16_t)((((int16_t *)precodatatx_F)[1]*ONE_OVER_SQRT2_Q15)>>15);*/
}

int nr_layer_precoder_cm(int16_t **datatx_F_precoding, int *prec_matrix, uint8_t n_layers, int32_t re_offset)
{
  int32_t precodatatx_F = 0;
  for (int al = 0; al<n_layers; al++) {
    int16_t antenna_re = datatx_F_precoding[al][re_offset<<1];
    int16_t antenna_im = datatx_F_precoding[al][(re_offset<<1) +1];
    //printf("antenna precoding: %d %d\n",((int16_t *)&prec_matrix[al])[0],((int16_t *)&prec_matrix[al])[1]);
    ((int16_t *) &precodatatx_F)[0] += (int16_t)(((int32_t)(antenna_re*(((int16_t *)&prec_matrix[al])[0])) - (int32_t)(antenna_im* (((int16_t *)&prec_matrix[al])[1])))>>15);
    ((int16_t *) &precodatatx_F)[1] += (int16_t)(((int32_t)(antenna_re*(((int16_t *)&prec_matrix[al])[1])) + (int32_t)(antenna_im* (((int16_t *)&prec_matrix[al])[0])))>>15);
  }

  return precodatatx_F;
}

