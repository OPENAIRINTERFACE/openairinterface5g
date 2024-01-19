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
#include "executables/softmodem-common.h"

// #define DEBUG_DLSCH_PRECODING_PRINT_WITH_TRIVIAL // TODO: For debug, to be removed if want to merge to develop

//Table 6.3.1.5-1 Precoding Matrix W 1 layer 2 antenna ports 'n' = -1 and 'o' = -j
const char nr_W_1l_2p[6][2][1] = {
    {{'1'}, {'0'}}, // pmi 0
    {{'0'}, {'1'}},
    {{'1'}, {'1'}},
    {{'1'}, {'n'}},
    {{'1'}, {'j'}},
    {{'1'}, {'o'}} // pmi 5
};

//Table 6.3.1.5-3 Precoding Matrix W 1 layer 4 antenna ports 'n' = -1 and 'o' = -j
const char nr_W_1l_4p[28][4][1] = {
    {{'1'}, {'0'}, {'0'}, {'0'}}, // pmi 0
    {{'0'}, {'1'}, {'0'}, {'0'}},
    {{'0'}, {'0'}, {'1'}, {'0'}},
    {{'0'}, {'0'}, {'0'}, {'1'}},
    {{'1'}, {'0'}, {'1'}, {'0'}},
    {{'1'}, {'0'}, {'n'}, {'0'}},
    {{'1'}, {'0'}, {'j'}, {'0'}},
    {{'1'}, {'0'}, {'o'}, {'0'}}, // pmi 7
    {{'0'}, {'1'}, {'0'}, {'1'}}, // pmi 8
    {{'0'}, {'1'}, {'0'}, {'n'}},
    {{'0'}, {'1'}, {'0'}, {'j'}},
    {{'0'}, {'1'}, {'0'}, {'o'}},
    {{'1'}, {'1'}, {'1'}, {'1'}},
    {{'1'}, {'1'}, {'j'}, {'j'}},
    {{'1'}, {'1'}, {'n'}, {'n'}},
    {{'1'}, {'1'}, {'o'}, {'o'}},
    {{'1'}, {'j'}, {'1'}, {'j'}}, // pmi
                                  // 16
    {{'1'}, {'j'}, {'j'}, {'n'}},
    {{'1'}, {'j'}, {'n'}, {'o'}},
    {{'1'}, {'j'}, {'o'}, {'1'}},
    {{'1'}, {'n'}, {'1'}, {'n'}},
    {{'1'}, {'n'}, {'j'}, {'o'}},
    {{'1'}, {'n'}, {'n'}, {'1'}},
    {{'1'}, {'n'}, {'o'}, {'j'}}, // pmi 23
    {{'1'}, {'o'}, {'1'}, {'o'}}, // pmi 24
    {{'1'}, {'o'}, {'j'}, {'1'}},
    {{'1'}, {'o'}, {'n'}, {'j'}},
    {{'1'}, {'o'}, {'o'}, {'n'}} // pmi 27
};

//Table 6.3.1.5-4 Precoding Matrix W 2 antenna ports layers 2  'n' = -1 and 'o' = -j
const char nr_W_2l_2p[3][2][2] = {
    {{'1', '0'}, {'0', '1'}}, // pmi 0
    {{'1', '1'}, {'1', 'n'}},
    {{'1', '1'}, {'j', 'o'}} // pmi 2
};

//Table 6.3.1.5-5 Precoding Matrix W 2 layers 4 antenna ports 'n' = -1 and 'o' = -j
const char nr_W_2l_4p[22][4][2] = {
    {{'1', '0'}, {'0', '1'}, {'0', '0'}, {'0', '0'}}, // pmi 0
    {{'1', '0'}, {'0', '0'}, {'0', '1'}, {'0', '0'}}, {{'1', '0'}, {'0', '0'}, {'0', '0'}, {'0', '1'}},
    {{'0', '0'}, {'1', '0'}, {'0', '1'}, {'0', '0'}}, // pmi 3
    {{'0', '0'}, {'1', '0'}, {'0', '0'}, {'0', '1'}}, // pmi 4
    {{'0', '0'}, {'0', '0'}, {'1', '0'}, {'0', '1'}}, {{'1', '0'}, {'0', '1'}, {'1', '0'}, {'0', 'o'}},
    {{'1', '0'}, {'0', '1'}, {'1', '0'}, {'0', 'j'}}, {{'1', '0'}, {'0', '1'}, {'o', '0'}, {'0', '1'}}, // pmi 8
    {{'1', '0'}, {'0', '1'}, {'o', '0'}, {'0', 'n'}}, {{'1', '0'}, {'0', '1'}, {'n', '0'}, {'0', 'o'}},
    {{'1', '0'}, {'0', '1'}, {'n', '0'}, {'0', 'j'}}, // pmi 11
    {{'1', '0'}, {'0', '1'}, {'j', '0'}, {'0', '1'}}, // pmi 12
    {{'1', '0'}, {'0', '1'}, {'j', '0'}, {'0', 'n'}}, {{'1', '1'}, {'1', '1'}, {'1', 'n'}, {'1', 'n'}},
    {{'1', '1'}, {'1', '1'}, {'j', 'o'}, {'j', 'o'}}, // pmi 15
    {{'1', '1'}, {'j', 'j'}, {'1', 'n'}, {'j', 'o'}}, // pmi 16
    {{'1', '1'}, {'j', 'j'}, {'j', 'o'}, {'n', '1'}}, {{'1', '1'}, {'n', 'n'}, {'1', 'n'}, {'n', '1'}},
    {{'1', '1'}, {'n', 'n'}, {'j', 'o'}, {'o', 'j'}}, // pmi 19
    {{'1', '1'}, {'o', 'o'}, {'1', 'n'}, {'o', 'j'}}, {{'1', '1'}, {'o', 'o'}, {'j', 'o'}, {'1', 'n'}} // pmi 21
};

//Table 6.3.1.5-6 Precoding Matrix W 3 layers 4 antenna ports 'n' = -1 and 'o' = -j
const char nr_W_3l_4p[7][4][3] = {{{'1', '0', '0'}, {'0', '1', '0'}, {'0', '0', '1'}, {'0', '0', '0'}}, // pmi 0
                                  {{'1', '0', '0'}, {'0', '1', '0'}, {'1', '0', '0'}, {'0', '0', '1'}},
                                  {{'1', '0', '0'}, {'0', '1', '0'}, {'n', '0', '0'}, {'0', '0', '1'}},
                                  {{'1', '1', '1'}, {'1', 'n', '1'}, {'1', '1', 'n'}, {'1', 'n', 'n'}}, // pmi 3
                                  {{'1', '1', '1'}, {'1', 'n', '1'}, {'j', 'j', 'o'}, {'j', 'o', 'o'}}, // pmi 4
                                  {{'1', '1', '1'}, {'n', '1', 'n'}, {'1', '1', 'n'}, {'n', '1', '1'}},
                                  {{'1', '1', '1'}, {'n', '1', 'n'}, {'j', 'j', 'o'}, {'o', 'j', 'j'}}};

//Table 6.3.1.5-7 Precoding Matrix W 4 layers 4 antenna ports 'n' = -1 and 'o' = -j
const char nr_W_4l_4p[5][4][4] = {
    {{'1', '0', '0', '0'}, {'0', '1', '0', '0'}, {'0', '0', '1', '0'}, {'0', '0', '0', '1'}}, // pmi 0
    {{'1', '1', '0', '0'}, {'0', '0', '1', '1'}, {'1', 'n', '0', '0'}, {'0', '0', '1', 'n'}},
    {{'1', '1', '0', '0'}, {'0', '0', '1', '1'}, {'j', 'o', '0', '0'}, {'0', '0', 'j', 'o'}},
    {{'1', '1', '1', '1'}, {'1', 'n', '1', 'n'}, {'1', '1', 'n', 'n'}, {'1', 'n', 'n', '1'}}, // pmi 3
    {{'1', '1', '1', '1'}, {'1', 'n', '1', 'n'}, {'j', 'j', 'o', 'o'}, {'j', 'o', 'o', 'j'}} // pmi 4
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
  uint32_t i;

#if defined(__SSE2__)
  simde__m128i *nr_mod_table128;
  simde__m128i *out128;
#endif

  LOG_D(PHY,"nr_modulation: length %d, mod_order %d\n",length,mod_order);

  switch (mod_order) {

#if defined(__SSE2__)
  case 2:
    nr_mod_table128 = (simde__m128i *)nr_qpsk_byte_mod_table;
    out128 = (simde__m128i *)out;
    for (i=0; i<length/8; i++)
      out128[i] = nr_mod_table128[in_bytes[i]];
    // the bits that are left out
    i = i*8/2;
    nr_mod_table32 = (int32_t*) nr_qpsk_mod_table;
    while (i<length/2) {
      const int idx = ((in_bytes[(i * 2) / 8] >> ((i * 2) & 0x7)) & mask);
      out32[i] = nr_mod_table32[idx];
      i++;
    }
    return;
#else
  case 2:
    nr_mod_table32 = (int32_t*) nr_qpsk_mod_table;
    for (i=0; i<length/mod_order; i++) {
      const int idx = ((in[i * 2 / 32] >> ((i * 2) & 0x1f)) & mask);
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
      const int idx = ((in_bytes[(i * 4) / 8] >> ((i * 4) & 0x7)) & mask);
      out32[i] = nr_16qam_mod_table[idx];
      i++;
    }
    return;

  case 6:
    for (i = 0; i < length - 3 * 64; i += 3 * 64) {
      uint64_t x = *in64++;
      uint64_t x1 = x & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x >> 12) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x >> 24) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x >> 36) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x >> 48) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      uint64_t x2 = (x >> 60);
      x = *in64++;
      x2 |= x<<4;
      x1 = x2 & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x2 >> 12) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x2 >> 24) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x2 >> 36) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x2 >> 48) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x2 = ((x>>56)&0xf0) | (x2>>60);
      x = *in64++;
      x2 |= x<<8;
      x1 = x2 & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x2 >> 12) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x2 >> 24) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x2 >> 36) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (x2 >> 48) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x2 = ((x>>52)&0xff0) | (x2>>60);
      *out64++ = nr_64qam_mod_table[x2];
    }
    while (i + 24 <= length) {
      uint32_t xx = 0;
      memcpy(&xx, in_bytes + i / 8, 3);
      uint64_t x1 = xx & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      x1 = (xx >> 12) & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
      i += 24;
    }
    if (i != length) {
      uint32_t xx = 0;
      memcpy(&xx, in_bytes + i / 8, 2);
      uint64_t x1 = xx & 0xfff;
      *out64++ = nr_64qam_mod_table[x1];
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

void nr_layer_mapping(int nbCodes,
                      int encoded_len,
                      c16_t mod_symbs[nbCodes][encoded_len],
                      uint8_t n_layers,
                      int layerSz,
                      uint32_t n_symbs,
                      c16_t tx_layers[n_layers][layerSz])
{
  LOG_D(PHY,"Doing layer mapping for %d layers, %d symbols\n",n_layers,n_symbs);

  switch (n_layers) {

    case 1:
    memcpy(tx_layers[0], mod_symbs[0], n_symbs * sizeof(**mod_symbs));
    break;

    case 2:
    case 3:
    case 4:
    for (int i = 0; i < n_symbs / n_layers; i++) {
      const c16_t *base = mod_symbs[0] + n_layers * i;
      for (int l = 0; l < n_layers; l++)
        tx_layers[l][i] = base[l];
    }
      break;

    case 5:
      for (int i = 0; i < n_symbs; i += 2) {
      const int txIdx = i / 2;
      for (int l = 0; l < 2; l++)
        tx_layers[l][txIdx] = mod_symbs[0][i + l];
      }
      for (int i = 0; i < n_symbs; i += 3) {
      const int txIdx = i / 3;
      for (int l = 2; l < 5; l++)
        tx_layers[l][txIdx] = mod_symbs[1][i + l];
      }
      break;

    case 6:
      for (int q=0; q<2; q++)
      for (int i = 0; i < n_symbs; i += 3) {
        const int txIdx = i / 3;
        for (int l = 0; l < 3; l++)
          tx_layers[l][txIdx] = mod_symbs[q][i + l];
      }
      break;

    case 7:
      for (int i = 0; i < n_symbs; i += 3) {
      const int txIdx = i / 3;
      for (int l = 0; l < 3; l++)
        tx_layers[l][txIdx] = mod_symbs[1][i + l];
      }
      for (int i = 0; i < n_symbs; i += 4) {
      const int txIdx = i / 4;
      for (int l = 3; l < 7; l++)
        tx_layers[l][txIdx] = mod_symbs[0][i + l];
      }
      break;

    case 8:
      for (int q=0; q<2; q++)
      for (int i = 0; i < n_symbs; i += 4) {
        const int txIdx = i / 4;
        for (int l = 0; l < 3; l++)
          tx_layers[l][txIdx] = mod_symbs[q][i + l];
      }
      break;

    default:
      AssertFatal(0, "Invalid number of layers %d\n", n_layers);
  }
}

void nr_ue_layer_mapping(int16_t *mod_symbs,
                         uint8_t n_layers,
                         uint32_t n_symbs,
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
  simde__m128i dft_in128[1][3240], dft_out128[1][3240];
  uint32_t *dft_in0 = (uint32_t*)dft_in128[0], *dft_out0 = (uint32_t*)dft_out128[0];

  uint32_t i, ip;

  simde__m128i norm128;

  if ((Msc_PUSCH % 1536) > 0) {
    for (i = 0, ip = 0; i < Msc_PUSCH; i++, ip+=4) {
      dft_in0[ip] = d[i];
    }
  }

  switch (Msc_PUSCH) {
    case 12:
      dft(DFT_12,(int16_t *)dft_in0, (int16_t *)dft_out0,0);

      norm128 = simde_mm_set1_epi16(9459);
      for (i=0; i<12; i++) {
        ((simde__m128i*)dft_out0)[i] = simde_mm_slli_epi16(simde_mm_mulhi_epi16(((simde__m128i*)dft_out0)[i], norm128), 1);
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
  uint64_t sl_CarrierFreq = fp->sl_CarrierFreq;
  double f[2] = {(double)dl_CarrierFreq, (double)ul_CarrierFreq};

  const int nsymb = fp->symbols_per_slot * fp->slots_per_frame/10;
  const double Tc=(1/480e3/4096);
  const double Nu=2048*64*(1/(float)(1<<fp->numerology_index));
  const double Ncp0=16*64 + (144*64*(1/(float)(1<<fp->numerology_index)));
  const double Ncp1=(144*64*(1/(float)(1<<fp->numerology_index)));

  for (uint8_t ll = 0; ll < 2; ll++){

    double f0 = f[ll];
    LOG_D(PHY, "Doing symbol rotation calculation for gNB TX/RX, f0 %f Hz, Nsymb %d\n", f0, nsymb);
    c16_t *symbol_rotation = fp->symbol_rotation[ll];
    if (get_softmodem_params()->sl_mode == 2) {
      f0 = (double)sl_CarrierFreq;
      symbol_rotation = fp->symbol_rotation[link_type_sl];
    }

    double tl = 0.0;
    double poff = 0.0;
    double exp_re = 0.0;
    double exp_im = 0.0;

    for (int l = 0; l < nsymb; l++) {

      double Ncp;
      if (l == 0 || l == (7 * (1 << fp->numerology_index))) {
        Ncp = Ncp0;
      } else {
        Ncp = Ncp1;
      }

      poff = 2 * M_PI * (tl + (Ncp * Tc)) * f0;
      exp_re = cos(poff);
      exp_im = sin(-poff);
      symbol_rotation[l].r = (int16_t)floor(exp_re * 32767);
      symbol_rotation[l].i = (int16_t)floor(exp_im * 32767);

      LOG_D(PHY, "Symbol rotation %d/%d => tl %f (%d,%d) (%f)\n",
        l,
        nsymb,
        tl,
        symbol_rotation[l].r,
        symbol_rotation[l].i,
        (poff / 2 / M_PI) - floor(poff / 2 / M_PI));

      tl += (Nu + Ncp) * Tc;

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
    fp->timeshift_symbol_rotation[i].r = (int16_t)round(exp_re * 32767);
    fp->timeshift_symbol_rotation[i].i = (int16_t)round(exp_im * 32767);

    if (i < 10)
      LOG_D(PHY,"Timeshift symbol rotation %d => (%d,%d) %f\n",i,
            fp->timeshift_symbol_rotation[i].r,
            fp->timeshift_symbol_rotation[i].i,
            poff);
  }
}

int nr_layer_precoder(int16_t **datatx_F_precoding, const char *prec_matrix, uint8_t n_layers, int32_t re_offset)
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

c16_t nr_layer_precoder_cm(int n_layers,
                           int n_symbols,
                           int symSz,
                           c16_t datatx_F_precoding[n_layers][n_symbols][symSz],
                           int ap,
                           nfapi_nr_pm_pdu_t *pmi_pdu,
                           int symbol,
                           int offset)
{
  c16_t precodatatx_F = {0};
  for (int al = 0; al < n_layers; al++) {
    nfapi_nr_pm_weights_t *w = &pmi_pdu->weights[al][ap];
    c16_t prec_weight = {.r = w->precoder_weight_Re, .i = w->precoder_weight_Im};
    precodatatx_F = c16maddShift(datatx_F_precoding[al][symbol][offset], prec_weight, precodatatx_F, 15);
  }
  return precodatatx_F;
}

void nr_layer_precoder_simd(const int n_layers,
                           const int n_symbols,
                           const int symSz,
                           const c16_t txdataF_res_mapped[n_layers][n_symbols][symSz],
                           const int ant,
                           const nfapi_nr_pm_pdu_t *pmi_pdu,
                           const int symbol,
                           const int sc_offset,
                           const int re_cnt,
                           c16_t *txdataF_precoded)
{
  uint32_t sc = sc_offset;
  c16_t prec_weight = {0};
  // For x86, use 256 SIMD for every 8 RE and 128 SIMD for last 4 RE
  // For aarch64, use 128 SIMD for every 4 RE

  // 256 SIMD: Do 8 RE in one iteration, 3 iterations for 2 RB
#ifdef __AVX2__
  const uint32_t re_cnt_align8 = re_cnt & ~7;
  for(; sc < sc_offset + (re_cnt_align8); sc += sizeof(simde__m256i) / sizeof(prec_weight)) {
    // Matrix multiplication for 4 elements of the result (sizeof(simde__m256i) / sizeof(*prec_matrix) = 8)
    simde__m256i y = simde_mm256_set1_epi16(0); // Y = W[0]*X[0] + W[1]*X[1] + ... + W[nrOfLayers-1]*X[nrOfLayers-1]
    for(int nl = 0; nl < n_layers; nl++) {
      prec_weight.r = pmi_pdu->weights[nl][ant].precoder_weight_Re;
      prec_weight.i = pmi_pdu->weights[nl][ant].precoder_weight_Im;

      const simde__m256i x = simde_mm256_loadu_epi32(&txdataF_res_mapped[nl][symbol][sc]);

      // Rearrange precoding matrix weight to match complex multiplication and broadcast it to match SIMD size
      const simde__m256i w_c   = simde_mm256_set1_epi32(c16toI32(c16conj(prec_weight)));   // broadcast conjugate of w
      const simde__m256i w_s   = simde_mm256_set1_epi32(c16toI32(c16swap(prec_weight)));   // broadcast swapped real and img of w

      // Multiplication and shift
      const simde__m256i reals = simde_mm256_srai_epi32(simde_mm256_madd_epi16(x, w_c), 15); // (int32_t) .r = (x.r * w.r - x.i * w.i) >> 15
      const simde__m256i imags = simde_mm256_slli_epi32(simde_mm256_madd_epi16(x, w_s), 1);  // (int32_t) .i = (x.r * w.i + x.i * w.r) << 1, since higher 16 bit of each 32 bit is taken by blend_epi16

      // Re-arrange to match c16_t format
      const simde__m256i produ = simde_mm256_blend_epi16(reals, imags, 0xAA);

      // Accumulate the product
      y = simde_mm256_adds_epi16(y, produ);
    }
    // Store the result to txdataF
    simde_mm256_storeu_si256(&txdataF_precoded[sc], y);
  }
#endif

  // 128 SIMD: Do 4 RE in one iteration, 3 iterations for 1 RB
  const uint32_t re_cnt_align4 = re_cnt & ~3;
  for(; sc < sc_offset+re_cnt_align4; sc += sizeof(simde__m128i) / sizeof(prec_weight)) {
    #ifdef DEBUG_DLSCH_PRECODING_PRINT_WITH_TRIVIAL // Get result with trivial solution, TODO: To be removed
      c16_t y_triv[4];
      for(int i = 0; i < 4; i++)
        y_triv[i] = nr_layer_precoder_cm(n_layers,
                                         NR_SYMBOLS_PER_SLOT,
                                         symSz,
                                         txdataF_res_mapped,
                                         ant,
                                         pmi_pdu,
                                         symbol,
                                         sc + i);
      memcpy(&txdataF_precoded[sc], y_triv, sizeof(y_triv));
    #endif

    // Matrix multiplication for 4 elements of the result (sizeof(simde__m128i) / sizeof(c16_t) = 4)
    simde__m128i y = simde_mm_set1_epi16(0); // Y = W[0]*X[0] + W[1]*X[1] + ... + W[nrOfLayers-1]*X[nrOfLayers-1]
    for(int nl = 0; nl < n_layers; nl++) {
      prec_weight.r = pmi_pdu->weights[nl][ant].precoder_weight_Re;
      prec_weight.i = pmi_pdu->weights[nl][ant].precoder_weight_Im;

      const simde__m128i x = simde_mm_loadu_epi32(&txdataF_res_mapped[nl][symbol][sc]);

      // Rearrange precoding matrix weight to match complex multiplication and broadcast it to match SIMD size
      const simde__m128i w_c   = simde_mm_set1_epi32(c16toI32(c16conj(prec_weight)));   // broadcast conjugate of w
      const simde__m128i w_s   = simde_mm_set1_epi32(c16toI32(c16swap(prec_weight)));   // broadcast swapped real and img of w

      // Multiplication and shift
      const simde__m128i reals = simde_mm_srai_epi32(simde_mm_madd_epi16(x, w_c), 15); // (int32_t) .r = (x.r * w.r - x.i * w.i) >> 15
      const simde__m128i imags = simde_mm_slli_epi32(simde_mm_madd_epi16(x, w_s), 1);  // (int32_t) .i = (x.r * w.i + x.i * w.r) << 1, since higher 16 bit of each 32 bit is taken by blend_epi16

      /* Re-arrange to match c16_t format
        bit index: 0            | 16              | 32           | 48              | 64           | 80              | 96           | 112
        reals =   {R0.r[15..30] | R0.r[31] (0)*15 | R1.r[15..30] | R1.r[31] (0)*15 | R2.r[15..30] | R2.r[31] (0)*15 | R3.r[15..30] | R3.r[31] (0)*15}
        imags =   {0 R0.i[0..14]| R0.i[15..30]    | 0 R1.i[0..14]| R1.i[15..30]    | 0 R2.i[0..14]| R2.i[15..30]    | 0 R3.i[0..14]| R3.i[15..30]   }
        16b from  {reals        | imags           | reals        | imags           | reals        | imags           | reals        | imags          }
        produ =   {R0.r[15..30] | R0.i[15..30]    | R1.r[15..30] | R1.i[15..30]    | R2.r[15..30] | R2.i[15..30]    | R3.r[15..30] | R3.i[15..30]   }
      */
      const simde__m128i produ = simde_mm_blend_epi16(reals, imags, 0xAA);

      // Accumulate the product
      y = simde_mm_adds_epi16(y, produ);
    }
    // Store the result to txdataF
    simde_mm_storeu_si128(&txdataF_precoded[sc], y);

    #ifdef DEBUG_DLSCH_PRECODING_PRINT_WITH_TRIVIAL // Print simd and trivial result, TODO: To be removed
      c16_t *y_simd = (c16_t*) &y;
      printf("debug_to_be_removed re_cnt=%d, sc=%d, y_simd=(%+4d,%+4d), (%+4d,%+4d), (%+4d,%+4d), (%+4d,%+4d)\n", re_cnt, sc, y_simd[0].r, y_simd[0].i, y_simd[1].r, y_simd[1].i, y_simd[2].r, y_simd[2].i, y_simd[3].r, y_simd[3].i);
      printf("debug_to_be_removed re_cnt=%d, sc=%d, y_triv=(%+4d,%+4d), (%+4d,%+4d), (%+4d,%+4d), (%+4d,%+4d)\n", re_cnt, sc, y_triv[0].r, y_triv[0].i, y_triv[1].r, y_triv[1].i, y_triv[2].r, y_triv[2].i, y_triv[3].r, y_triv[3].i);
    #endif
  }
}
