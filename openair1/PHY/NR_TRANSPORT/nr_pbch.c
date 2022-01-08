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

/*! \file PHY/NR_TRANSPORT/nr_pbch.c
* \brief Top-level routines for generating the PBCH/BCH physical/transport channel V15.1 03/2018
* \author Guy De Souza
* \thanks Special Thanks to Son Dang for helpful contributions and testing
* \date 2018
* \version 0.1
* \company Eurecom
* \email: desouza@eurecom.fr
* \note
* \warning
*/

#include "PHY/defs_gNB.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"
#include "PHY/sse_intrin.h"

//#define DEBUG_PBCH
//#define DEBUG_PBCH_ENCODING
//#define DEBUG_PBCH_DMRS

extern short nr_qpsk_mod_table[8];

const uint8_t nr_pbch_payload_interleaving_pattern[32] = {16, 23, 18, 17, 8, 30, 10, 6, 24, 7, 0, 5, 3, 2, 1, 4,
                                                    9, 11, 12, 13, 14, 15, 19, 20, 21, 22, 25, 26, 27, 28, 29, 31
                                                   };

int nr_generate_pbch_dmrs(uint32_t *gold_pbch_dmrs,
                          int32_t *txdataF,
                          int16_t amp,
                          uint8_t ssb_start_symbol,
                          nfapi_nr_config_request_scf_t *config,
                          NR_DL_FRAME_PARMS *frame_parms) {
  int k,l;
  //int16_t a;
  int16_t mod_dmrs[NR_PBCH_DMRS_LENGTH<<1];
  uint8_t idx=0;
  uint8_t nushift = config->cell_config.phy_cell_id.value &3;
  LOG_D(PHY, "PBCH DMRS mapping started at symbol %d shift %d\n", ssb_start_symbol+1, nushift);

  /// QPSK modulation
  for (int m=0; m<NR_PBCH_DMRS_LENGTH; m++) {
    idx = (((gold_pbch_dmrs[(m<<1)>>5])>>((m<<1)&0x1f))&3);
    mod_dmrs[m<<1] = nr_qpsk_mod_table[idx<<1];
    mod_dmrs[(m<<1)+1] = nr_qpsk_mod_table[(idx<<1) + 1];
#ifdef DEBUG_PBCH_DMRS
    printf("m %d idx %d gold seq %d b0-b1 %d-%d mod_dmrs %d %d\n", m, idx, gold_pbch_dmrs[(m<<1)>>5], (((gold_pbch_dmrs[(m<<1)>>5])>>((m<<1)&0x1f))&1),
           (((gold_pbch_dmrs[((m<<1)+1)>>5])>>(((m<<1)+1)&0x1f))&1), mod_dmrs[(m<<1)], mod_dmrs[(m<<1)+1]);
#endif
  }

  /// Resource mapping
  // PBCH DMRS are mapped  within the SSB block on every fourth subcarrier starting from nushift of symbols 1, 2, 3
  ///symbol 1  [0+nushift:4:236+nushift] -- 60 mod symbols
  k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + nushift;
  l = ssb_start_symbol + 1;

  for (int m = 0; m < 60; m++) {
#ifdef DEBUG_PBCH_DMRS
    printf("m %d at k %d of l %d\n", m, k, l);
#endif
    ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1]       = (amp * mod_dmrs[m<<1]) >> 15;
    ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1] = (amp * mod_dmrs[(m<<1) + 1]) >> 15;
#ifdef DEBUG_PBCH_DMRS
    printf("(%d,%d)\n",
           ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1],
           ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1)+1]);
#endif
    k+=4;

    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }

  ///symbol 2  [0+u:4:44+nushift ; 192+nu:4:236+nushift] -- 24 mod symbols
  k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + nushift;
  l++;

  for (int m = 60; m < 84; m++) {
#ifdef DEBUG_PBCH_DMRS
    printf("m %d at k %d of l %d\n", m, k, l);
#endif
    ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1]       = (amp * mod_dmrs[m<<1]) >> 15;
    ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1] = (amp * mod_dmrs[(m<<1) + 1]) >> 15;
#ifdef DEBUG_PBCH_DMRS
    printf("(%d,%d)\n",
           ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1],
           ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1)+1]);
#endif
    k+=(m==71)?148:4; // Jump from 44+nu to 192+nu

    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }

  ///symbol 3  [0+nushift:4:236+nushift] -- 60 mod symbols
  k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier + nushift;
  l++;

  for (int m = 84; m < NR_PBCH_DMRS_LENGTH; m++) {
#ifdef DEBUG_PBCH_DMRS
    printf("m %d at k %d of l %d\n", m, k, l);
#endif
    ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1]       = (amp * mod_dmrs[m<<1]) >> 15;
    ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1] = (amp * mod_dmrs[(m<<1) + 1]) >> 15;
#ifdef DEBUG_PBCH_DMRS
    printf("(%d,%d)\n",
           ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1],
           ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1)+1]);
#endif
    k+=4;

    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }

#ifdef DEBUG_PBCH_DMRS
  write_output("txdataF_pbch_dmrs.m", "txdataF_pbch_dmrs", txdataF[0], frame_parms->samples_per_frame_wCP>>1, 1, 1);
#endif
  return 0;
}

static void nr_pbch_scrambling(NR_gNB_PBCH *pbch,
                        uint32_t Nid,
                        uint8_t nushift,
                        uint16_t M,
                        uint16_t length,
                        uint8_t encoded,
                        uint32_t unscrambling_mask) {
  uint8_t reset, offset;
  uint32_t x1, x2, s=0;
  uint32_t *pbch_e = pbch->pbch_e;
  reset = 1;
  // x1 is set in lte_gold_generic
  x2 = Nid;

  // The Gold sequence is shifted by nushift* M, so we skip (nushift*M /32) double words
  for (int i=0; i<(uint16_t)ceil(((float)nushift*M)/32); i++) {
    s = lte_gold_generic(&x1, &x2, reset);
    reset = 0;
  }

  // Scrambling is now done with offset (nushift*M)%32
  offset = (nushift*M)&0x1f;
#ifdef DEBUG_PBCH_ENCODING
  printf("Scrambling params: nushift %d M %d length %d encoded %d offset %d\n", nushift, M, length, encoded, offset);
#endif
#ifdef DEBUG_PBCH_ENCODING
  printf("s: %04x\t", s);
#endif
  int k = 0;

  if (!encoded) {
    /// 1st Scrambling
    for (int i = 0; i < length; ++i) {
      if ((unscrambling_mask>>i)&1)
        pbch->pbch_a_prime ^= ((pbch->pbch_a_interleaved>>i)&1)<<i;
      else {
        if (((k+offset)&0x1f)==0) {
          s = lte_gold_generic(&x1, &x2, reset);
          reset = 0;
        }

        pbch->pbch_a_prime ^= (((pbch->pbch_a_interleaved>>i)&1) ^ ((s>>((k+offset)&0x1f))&1))<<i;
        k++;                  /// k increase only when payload bit is not special bit
      }
    }
  } else {
    /// 2nd Scrambling
    for (int i = 0; i < length; ++i) {
      if (((i+offset)&0x1f)==0) {
        s = lte_gold_generic(&x1, &x2, reset);
        reset = 0;
      }

      pbch_e[i>>5] ^= (((s>>((i+offset)&0x1f))&1)<<(i&0x1f));
    }
  }
}


void nr_init_pbch_interleaver(uint8_t *interleaver) {
  uint8_t j_sfn=0, j_hrf=10, j_ssb=11, j_other=14;
  memset((void *)interleaver,0, NR_POLAR_PBCH_PAYLOAD_BITS);

  for (uint8_t i=0; i<NR_POLAR_PBCH_PAYLOAD_BITS; i++)
    if (!i) // choice bit:1
      *(interleaver+i) = *(nr_pbch_payload_interleaving_pattern+j_other++);
    else if (i<7) //Sfn bits:6
      *(interleaver+i) = *(nr_pbch_payload_interleaving_pattern+j_sfn++);
    else if (i<24) // other:17
      *(interleaver+i) = *(nr_pbch_payload_interleaving_pattern+j_other++);
    else if (i<28) // Sfn:4
      *(interleaver+i) = *(nr_pbch_payload_interleaving_pattern+j_sfn++);
    else if (i==28) // Hrf bit:1
      *(interleaver+i) = *(nr_pbch_payload_interleaving_pattern+j_hrf);
    else // Ssb bits:3
      *(interleaver+i) = *(nr_pbch_payload_interleaving_pattern+j_ssb++);
}

int nr_generate_pbch(nfapi_nr_dl_tti_ssb_pdu *ssb_pdu,
                     uint8_t *interleaver,
                     int32_t *txdataF,
                     int16_t amp,
                     uint8_t ssb_start_symbol,
                     uint8_t n_hf,
                     int sfn,
                     nfapi_nr_config_request_scf_t *config,
                     NR_DL_FRAME_PARMS *frame_parms) {
  int k,l,m;
  //int16_t a;
  int16_t mod_pbch_e[NR_POLAR_PBCH_E];
  uint8_t idx=0;
  uint16_t M;
  uint8_t nushift;
  uint32_t unscrambling_mask;
  uint64_t a_reversed=0;
  LOG_D(PHY, "PBCH generation started\n");
  ///Payload generation
  NR_gNB_PBCH m_pbch;
  NR_gNB_PBCH *pbch = &m_pbch;
  memset((void *)pbch, 0, sizeof(NR_gNB_PBCH));
  pbch->pbch_a=0;
  uint8_t ssb_index = ssb_pdu->ssb_pdu_rel15.SsbBlockIndex;
  uint8_t *pbch_pdu = (uint8_t*)&ssb_pdu->ssb_pdu_rel15.bchPayload;
  uint8_t Lmax = frame_parms->Lmax;
  for (int i=0; i<NR_PBCH_PDU_BITS; i++)
    pbch->pbch_a |= ((pbch_pdu[i>>3]>>(7-(i&7)))&1)<<i;

  #ifdef DEBUG_PBCH_ENCODING
  for (int i=0; i<3; i++)
    printf("pbch_pdu[%d]: 0x%02x\n", i, pbch_pdu[i]);

  printf("PBCH payload = 0x%08x\n",pbch->pbch_a);
  #endif

  // Extra byte generation
  for (int i=0; i<4; i++)
    pbch->pbch_a |= ((sfn>>(3-i))&1)<<(24+i); // resp. 4th, 3rd, 2nd ans 1st lsb of sfn

  pbch->pbch_a |= n_hf<<28; // half frame index bit

  if (Lmax == 64)
    for (int i=0; i<3; i++)
      pbch->pbch_a |= ((ssb_index>>(5-i))&1)<<(29+i); // resp. 6th, 5th and 4th bits of ssb_index
  else
    pbch->pbch_a |= ((config->ssb_table.ssb_subcarrier_offset.value>>4)&1)<<29; //MSB of k_SSB (bit index 4)

  LOG_D(PHY,"After extra byte: pbch_a = 0x%08x\n",pbch->pbch_a);

  // Payload interleaving

  for (int i=0; i<NR_POLAR_PBCH_PAYLOAD_BITS; i++) {
    pbch->pbch_a_interleaved |= ((pbch->pbch_a>>i)&1)<<(*(interleaver+i));
#ifdef DEBUG_PBCH_ENCODING
    printf("i %d out 0x%08x ilv %d (in>>i)&1) %d\n", i, pbch->pbch_a_interleaved, *(interleaver+i), (pbch->pbch_a>>i)&1);
#endif
  }

#ifdef DEBUG_PBCH_ENCODING
  printf("Interleaving:\n");
  printf("pbch_a_interleaved: 0x%08x\n", pbch->pbch_a_interleaved);
#endif
  // Scrambling
  unscrambling_mask = (Lmax ==64)? 0x100006D:0x1000041;
  M = (Lmax == 64)? (NR_POLAR_PBCH_PAYLOAD_BITS - 6) : (NR_POLAR_PBCH_PAYLOAD_BITS - 3);
  nushift = (((sfn>>2)&1)<<1) ^ ((sfn>>1)&1);
  pbch->pbch_a_prime = 0;
  nr_pbch_scrambling(pbch, (uint32_t)config->cell_config.phy_cell_id.value, nushift, M, NR_POLAR_PBCH_PAYLOAD_BITS, 0, unscrambling_mask);
#ifdef DEBUG_PBCH_ENCODING
  printf("Payload scrambling: nushift %d M %d sfn3 %d sfn2 %d\n", nushift, M, (sfn>>2)&1, (sfn>>1)&1);
  printf("pbch_a_prime: 0x%08x\n", pbch->pbch_a_prime);
#endif

  // Encoder reversal
  for (int i=0; i<NR_POLAR_PBCH_PAYLOAD_BITS; i++)
    a_reversed |= (((uint64_t)pbch->pbch_a_prime>>i)&1)<<(31-i);

  /// CRC, coding and rate matching
  polar_encoder_fast (&a_reversed, (void*)pbch->pbch_e, 0, 0,
                      nr_polar_params( NR_POLAR_PBCH_MESSAGE_TYPE, NR_POLAR_PBCH_PAYLOAD_BITS, NR_POLAR_PBCH_AGGREGATION_LEVEL,0,NULL)
                     );
#ifdef DEBUG_PBCH_ENCODING
  printf("Channel coding:\n");

  for (int i=0; i<NR_POLAR_PBCH_E_DWORD; i++)
    printf("pbch_e[%d]: 0x%08x\t", i, pbch->pbch_e[i]);

  printf("\n");
#endif
  /// Scrambling
  M =  NR_POLAR_PBCH_E;
  nushift = (Lmax==4)? ssb_index&3 : ssb_index&7;
  nr_pbch_scrambling(pbch, (uint32_t)config->cell_config.phy_cell_id.value, nushift, M, NR_POLAR_PBCH_E, 1, 0);
#ifdef DEBUG_PBCH_ENCODING
  printf("Scrambling:\n");

  for (int i=0; i<NR_POLAR_PBCH_E_DWORD; i++)
    printf("pbch_e[%d]: 0x%08x\t", i, pbch->pbch_e[i]);

  printf("\n");
#endif

  /// QPSK modulation
  for (int i=0; i<NR_POLAR_PBCH_E>>1; i++) {
    idx = ((pbch->pbch_e[(i<<1)>>5]>>((i<<1)&0x1f))&3);
    mod_pbch_e[i<<1] = nr_qpsk_mod_table[idx<<1];
    mod_pbch_e[(i<<1)+1] = nr_qpsk_mod_table[(idx<<1)+1];
#ifdef DEBUG_PBCH
    printf("i %d idx %d  mod_pbch %d %d\n", i, idx, mod_pbch_e[2*i], mod_pbch_e[2*i+1]);
#endif
  }

  /// Resource mapping
  nushift = config->cell_config.phy_cell_id.value &3;
  // PBCH modulated symbols are mapped  within the SSB block on symbols 1, 2, 3 excluding the subcarriers used for the PBCH DMRS
  ///symbol 1  [0:239] -- 180 mod symbols
  k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier;
  l = ssb_start_symbol + 1;
  m = 0;

  for (int ssb_sc_idx = 0; ssb_sc_idx < 240; ssb_sc_idx++) {
    if ((ssb_sc_idx&3) == nushift) {  //skip DMRS
      k++;
      continue;
    } else {
#ifdef DEBUG_PBCH
      printf("m %d ssb_sc_idx %d at k %d of l %d\n", m, ssb_sc_idx, k, l);
#endif
      ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1]       = (amp * mod_pbch_e[m<<1]) >> 15;
      ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1] = (amp * mod_pbch_e[(m<<1) + 1]) >> 15;
      k++;
      m++;
    }

    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }

  ///symbol 2  [0:47 ; 192:239] -- 72 mod symbols
  k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier;
  l++;
  m=180;

  for (int ssb_sc_idx = 0; ssb_sc_idx < 48; ssb_sc_idx++) {
    if ((ssb_sc_idx&3) == nushift) {
      k++;
      continue;
    } else {
#ifdef DEBUG_PBCH
      printf("m %d ssb_sc_idx %d at k %d of l %d\n", m, ssb_sc_idx, k, l);
#endif
      ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1]       = (amp * mod_pbch_e[m<<1]) >> 15;
      ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1] = (amp * mod_pbch_e[(m<<1) + 1]) >> 15;
      k++;
      m++;
    }

    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }

  k += 144;

  if (k >= frame_parms->ofdm_symbol_size)
    k-=frame_parms->ofdm_symbol_size;

  m=216;

  for (int ssb_sc_idx = 192; ssb_sc_idx < 240; ssb_sc_idx++) {
    if ((ssb_sc_idx&3) == nushift) {
      k++;
      continue;
    } else {
#ifdef DEBUG_PBCH
      printf("m %d ssb_sc_idx %d at k %d of l %d\n", m, ssb_sc_idx, k, l);
#endif
      ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1]       = (amp * mod_pbch_e[m<<1]) >> 15;
      ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1] = (amp * mod_pbch_e[(m<<1) + 1]) >> 15;
      k++;
      m++;
    }

    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }

  ///symbol 3  [0:239] -- 180 mod symbols
  k = frame_parms->first_carrier_offset + frame_parms->ssb_start_subcarrier;
  l++;
  m=252;

  for (int ssb_sc_idx = 0; ssb_sc_idx < 240; ssb_sc_idx++) {
    if ((ssb_sc_idx&3) == nushift) {
      k++;
      continue;
    } else {
#ifdef DEBUG_PBCH
      printf("m %d ssb_sc_idx %d at k %d of l %d\n", m, ssb_sc_idx, k, l);
#endif
      ((int16_t *)txdataF)[(l*frame_parms->ofdm_symbol_size + k)<<1]       = (amp * mod_pbch_e[m<<1]) >> 15;
      ((int16_t *)txdataF)[((l*frame_parms->ofdm_symbol_size + k)<<1) + 1] = (amp * mod_pbch_e[(m<<1) + 1]) >> 15;
      k++;
      m++;
    }


    if (k >= frame_parms->ofdm_symbol_size)
      k-=frame_parms->ofdm_symbol_size;
  }


  return 0;
}
