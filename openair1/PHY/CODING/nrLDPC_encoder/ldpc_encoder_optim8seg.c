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

/*! \file ldpc_encoder_optim8seg.c
 * \brief Defines the optimized LDPC encoder
 * \author Florian Kaltenberger, Raymond Knopp, Kien le Trung (Eurecom)
 * \email openair_tech@eurecom.fr
 * \date 27-03-2018
 * \version 1.0
 * \note
 * \warning
 */

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "assertions.h"
#include "common/utils/LOG/log.h"
#include "time_meas.h"
#include "openair1/PHY/CODING/nrLDPC_defs.h"
#include "openair1/PHY/CODING/nrLDPC_extern.h"
#include "ldpc_encode_parity_check.c" 
#include "ldpc_generate_coefficient.c"
#include "PHY/sse_intrin.h"

int LDPCencoder(uint8_t **test_input, uint8_t **channel_input, encoder_implemparams_t *impp)
{
  int Zc = impp->Zc;
  int Kb = impp->Kb;
  short block_length = impp->K;
  short BG = impp->BG;

  short nrows=0,ncols=0;
  int i,i1,j,rate=3;
  int no_punctured_columns,removed_bit;
  char temp;
  int simd_size;

  simde__m256i shufmask = simde_mm256_set_epi64x(0x0303030303030303, 0x0202020202020202,0x0101010101010101, 0x0000000000000000);
  simde__m256i andmask  = simde_mm256_set1_epi64x(0x0102040810204080);  // every 8 bits -> 8 bytes, pattern repeats.
  simde__m256i zero256   = simde_mm256_setzero_si256();
  simde__m256i masks[8];
  register simde__m256i c256;
  masks[0] = simde_mm256_set1_epi8(0x1);
  masks[1] = simde_mm256_set1_epi8(0x2);
  masks[2] = simde_mm256_set1_epi8(0x4);
  masks[3] = simde_mm256_set1_epi8(0x8);
  masks[4] = simde_mm256_set1_epi8(0x10);
  masks[5] = simde_mm256_set1_epi8(0x20);
  masks[6] = simde_mm256_set1_epi8(0x40);
  masks[7] = simde_mm256_set1_epi8(0x80);

  AssertFatal((impp->n_segments>0&&impp->n_segments<=8),"0 < n_segments %d <= 8\n",impp->n_segments);

  //determine number of bits in codeword
  //if (block_length>3840)
  if (BG==1)
    {
      nrows=46; //parity check bits
      ncols=22; //info bits
      rate=3;
    }
    //else if (block_length<=3840)
   else if	(BG==2)
    {
      //BG=2;
      nrows=42; //parity check bits
      ncols=10; // info bits
      rate=5;

      }

#ifdef DEBUG_LDPC
  LOG_D(PHY,"ldpc_encoder_optim_8seg: BG %d, Zc %d, Kb %d, block_length %d, segments %d\n",BG,Zc,Kb,block_length,n_segments);
  LOG_D(PHY,"ldpc_encoder_optim_8seg: PDU (seg 0) %x %x %x %x\n",test_input[0][0],test_input[0][1],test_input[0][2],test_input[0][3]);
#endif

  AssertFatal(Zc>0,"no valid Zc found for block length %d\n",block_length);

  if ((Zc&31) > 0) simd_size = 16;
  else          simd_size = 32;

  unsigned char c[22*Zc] __attribute__((aligned(32))); //padded input, unpacked, max size
  unsigned char d[46*Zc] __attribute__((aligned(32))); //coded parity part output, unpacked, max size


  // calculate number of punctured bits
  no_punctured_columns=(int)((nrows-2)*Zc+block_length-block_length*rate)/Zc;
  removed_bit=(nrows-no_punctured_columns-2) * Zc+block_length-(int)(block_length*rate);
  // printf("%d\n",no_punctured_columns);
  // printf("%d\n",removed_bit);
  // unpack input
  memset(c,0,sizeof(unsigned char) * ncols * Zc);
  memset(d,0,sizeof(unsigned char) * nrows * Zc);

  if(impp->tinput != NULL) start_meas(impp->tinput);
#if 0
  for (i=0; i<block_length; i++) {
    for (j=0; j<n_segments; j++) {

      temp = (test_input[j][i/8]&(128>>(i&7)))>>(7-(i&7));
      //printf("c(%d,%d)=%d\n",j,i,temp);
      c[i] |= (temp << j);
    }
  }
#else
  for (i=0; i<block_length>>5; i++) {
    c256 = simde_mm256_and_si256(simde_mm256_cmpeq_epi8(simde_mm256_andnot_si256(simde_mm256_shuffle_epi8(simde_mm256_set1_epi32(((uint32_t*)test_input[0])[i]), shufmask),andmask),zero256),masks[0]);
    for (j=1; j<impp->n_segments; j++) {
      c256 = simde_mm256_or_si256(simde_mm256_and_si256(simde_mm256_cmpeq_epi8(simde_mm256_andnot_si256(simde_mm256_shuffle_epi8(simde_mm256_set1_epi32(((uint32_t*)test_input[j])[i]), shufmask),andmask),zero256),masks[j]),c256);
    }
    ((simde__m256i *)c)[i] = c256;
  }

  for (i=(block_length>>5)<<5;i<block_length;i++) {
    for (j=0; j<impp->n_segments; j++) {

      temp = (test_input[j][i/8]&(128>>(i&7)))>>(7-(i&7));
      //printf("c(%d,%d)=%d\n",j,i,temp);
      c[i] |= (temp << j);
    }
  }
#endif

  if(impp->tinput != NULL) stop_meas(impp->tinput);

  if ((BG==1 && Zc>=176) || (BG==2 && Zc>=64)) { 
    // extend matrix
    if(impp->tprep != NULL) start_meas(impp->tprep);
    if(impp->tprep != NULL) stop_meas(impp->tprep);
    //parity check part
    if(impp->tparity != NULL) start_meas(impp->tparity);
    encode_parity_check_part_optim(c, d, BG, Zc, Kb, simd_size, ncols);
    if(impp->tparity != NULL) stop_meas(impp->tparity);
  }
  else {
    if (encode_parity_check_part_orig(c, d, BG, Zc, Kb, block_length)!=0) {
      printf("Problem with encoder\n");
      return(-1);
    }
  }
  if(impp->toutput != NULL) start_meas(impp->toutput);
  // information part and puncture columns
  /*
  memcpy(&channel_input[0], &c[2*Zc], (block_length-2*Zc)*sizeof(unsigned char));
  memcpy(&channel_input[block_length-2*Zc], &d[0], ((nrows-no_punctured_columns) * Zc-removed_bit)*sizeof(unsigned char));
  */
  if ((((2*Zc)&31) == 0) && (((block_length-(2*Zc))&31) == 0)) {
    //AssertFatal(((2*Zc)&31) == 0,"2*Zc needs to be a multiple of 32 for now\n");
    //AssertFatal(((block_length-(2*Zc))&31) == 0,"block_length-(2*Zc) needs to be a multiple of 32 for now\n");
    uint32_t l1 = (block_length-(2*Zc))>>5;
    uint32_t l2 = ((nrows-no_punctured_columns) * Zc-removed_bit)>>5;
    simde__m256i *c256p = (simde__m256i *)&c[2*Zc];
    simde__m256i *d256p = (simde__m256i *)&d[0];
    //  if (((block_length-(2*Zc))&31)>0) l1++;
    
    for (i=0;i<l1;i++)
      for (j=0;j<impp->n_segments;j++) ((simde__m256i *)channel_input[j])[i] = simde_mm256_and_si256(simde_mm256_srai_epi16(c256p[i],j),masks[0]);
    
    //  if ((((nrows-no_punctured_columns) * Zc-removed_bit)&31)>0) l2++;
    
    for (i1=0;i1<l2;i1++,i++)
      for (j=0;j<impp->n_segments;j++) ((simde__m256i *)channel_input[j])[i] = simde_mm256_and_si256(simde_mm256_srai_epi16(d256p[i1],j),masks[0]);
  }
  else {
#ifdef DEBUG_LDPC
  LOG_W(PHY,"using non-optimized version\n");
#endif
    // do non-SIMD version
    for (i=0;i<(block_length-2*Zc);i++) 
      for (j=0; j<impp->n_segments; j++)
	channel_input[j][i] = (c[2*Zc+i]>>j)&1;
    for (i=0;i<((nrows-no_punctured_columns) * Zc-removed_bit);i++)
      for (j=0; j<impp->n_segments; j++)
	channel_input[j][block_length-2*Zc+i] = (d[i]>>j)&1;
    }

  if(impp->toutput != NULL) stop_meas(impp->toutput);
  return 0;
}


