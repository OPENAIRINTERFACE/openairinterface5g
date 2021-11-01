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

/*!\file ldpc_encoder2.c
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
#include <types.h>
#include "assertions.h"
#include "common/utils/LOG/log.h"
#include "PHY/TOOLS/time_meas.h"
#include "openair1/PHY/CODING/nrLDPC_defs.h"
//#define DEBUG_LDPC

#include "ldpc_encode_parity_check.c"
#include "ldpc_generate_coefficient.c"


int nrLDPC_encod(unsigned char **input,unsigned char **output,int Zc,int Kb,short block_length, short BG, encoder_implemparams_t *impp)
{
  //set_log(PHY, 4);
  

  int nrows=0,ncols=0;
  int rate=3;
  int no_punctured_columns,removed_bit;
  //Table of possible lifting sizes
  char temp;
  int simd_size;
  unsigned int macro_segment, macro_segment_end;

  
  macro_segment = 8*impp->macro_num;
  macro_segment_end = (impp->n_segments > 8*(impp->macro_num+1)) ? 8*(impp->macro_num+1) : impp->n_segments;
  ///printf("macro_segment: %d\n", macro_segment);
  ///printf("macro_segment_end: %d\n", macro_segment_end );

#ifdef __AVX2__
  __m256i shufmask = _mm256_set_epi64x(0x0303030303030303, 0x0202020202020202,0x0101010101010101, 0x0000000000000000);
  __m256i andmask  = _mm256_set1_epi64x(0x0102040810204080);  // every 8 bits -> 8 bytes, pattern repeats.
  __m256i zero256   = _mm256_setzero_si256();
  __m256i masks[8];
  register __m256i c256;
  masks[0] = _mm256_set1_epi8(0x1);
  masks[1] = _mm256_set1_epi8(0x2);
  masks[2] = _mm256_set1_epi8(0x4);
  masks[3] = _mm256_set1_epi8(0x8);
  masks[4] = _mm256_set1_epi8(0x10);
  masks[5] = _mm256_set1_epi8(0x20);
  masks[6] = _mm256_set1_epi8(0x40);
  masks[7] = _mm256_set1_epi8(0x80);
#endif



  //determine number of bits in codeword
  if (BG==1)
    {
      nrows=46; //parity check bits
      ncols=22; //info bits
      rate=3;
    }
    else if (BG==2)
    {
      nrows=42; //parity check bits
      ncols=10; // info bits
      rate=5;
    }

#ifdef DEBUG_LDPC
  LOG_D(PHY,"ldpc_encoder_optim_8seg: BG %d, Zc %d, Kb %d, block_length %d, segments %d\n",BG,Zc,Kb,block_length,impp->n_segments);
  LOG_D(PHY,"ldpc_encoder_optim_8seg: PDU (seg 0) %x %x %x %x\n",input[0][0],input[0][1],input[0][2],input[0][3]);
#endif

  AssertFatal(Zc>0,"no valid Zc found for block length %d\n",block_length);

  if ((Zc&31) > 0) simd_size = 16;
  else          simd_size = 32;

  unsigned char cc[22*Zc] __attribute__((aligned(32))); //padded input, unpacked, max size
  unsigned char dd[46*Zc] __attribute__((aligned(32))); //coded parity part output, unpacked, max size

  // calculate number of punctured bits
  no_punctured_columns=(int)((nrows-2)*Zc+block_length-block_length*rate)/Zc;
  removed_bit=(nrows-no_punctured_columns-2) * Zc+block_length-(int)(block_length*rate);
  //printf("%d\n",no_punctured_columns);
  //printf("%d\n",removed_bit);
  // unpack input
  memset(cc,0,sizeof(unsigned char) * ncols * Zc);
  memset(dd,0,sizeof(unsigned char) * nrows * Zc);

  if(impp->tinput != NULL) start_meas(impp->tinput);
#if 0
  for (i=0; i<block_length; i++) {
	//for (j=0; j<n_segments; j++) {
    for (j=macro_segment; j < macro_segment_end; j++) {

      temp = (input[j][i/8]&(1<<(i&7)))>>(i&7);
      //printf("c(%d,%d)=%d\n",j,i,temp);
      c[i] |= (temp << (j-macro_segment));
    }
  }
#else
#ifdef __AVX2__
  for (int i=0; i<block_length>>5; i++) {
    c256 = _mm256_and_si256(_mm256_cmpeq_epi8(_mm256_andnot_si256(_mm256_shuffle_epi8(_mm256_set1_epi32(((uint32_t*)input[macro_segment])[i]), shufmask),andmask),zero256),masks[0]);
    //for (j=1; j<n_segments; j++) {
    for (int j=macro_segment+1; j < macro_segment_end; j++) {    
      c256 = _mm256_or_si256(_mm256_and_si256(_mm256_cmpeq_epi8(_mm256_andnot_si256(_mm256_shuffle_epi8(_mm256_set1_epi32(((uint32_t*)input[j])[i]), shufmask),andmask),zero256),masks[j-macro_segment]),c256);
    }
    ((__m256i *)cc)[i] = c256;
  }

  for (int i=(block_length>>5)<<5;i<block_length;i++) {
    //for (j=0; j<n_segments; j++) {
	  for (int j=macro_segment; j < macro_segment_end; j++) {

	    temp = (input[j][i/8]&(128>>(i&7)))>>(7-(i&7));
      //printf("c(%d,%d)=%d\n",j,i,temp);
      cc[i] |= (temp << (j-macro_segment));
    }
  }
#else
  AssertFatal(1==0,"Need AVX2 for this\n");
#endif
#endif

  if(impp->tinput != NULL) stop_meas(impp->tinput);

  if ((BG==1 && Zc>176) || (BG==2 && Zc>64)) {
    // extend matrix
    if(impp->tprep != NULL) start_meas(impp->tprep);
    if(impp->tprep != NULL) stop_meas(impp->tprep);
    //parity check part
    if(impp->tparity != NULL) start_meas(impp->tparity);
    encode_parity_check_part_optim(cc, dd, BG, Zc, Kb, simd_size, ncols);
    if(impp->tparity != NULL) stop_meas(impp->tparity);
  }
  else {
    if (encode_parity_check_part_orig(cc, dd, BG, Zc, Kb, block_length)!=0) {
      printf("Problem with encoder\n");
      return(-1);
    }
  }
  if(impp->toutput != NULL) start_meas(impp->toutput);
  // information part and puncture columns
  /*
  memcpy(&output[0], &c[2*Zc], (block_length-2*Zc)*sizeof(unsigned char));
  memcpy(&output[block_length-2*Zc], &d[0], ((nrows-no_punctured_columns) * Zc-removed_bit)*sizeof(unsigned char));
  */
#ifdef __AVX2__
  if ((((2*Zc)&31) == 0) && (((block_length-(2*Zc))&31) == 0)) {
    //AssertFatal(((2*Zc)&31) == 0,"2*Zc needs to be a multiple of 32 for now\n");
    //AssertFatal(((block_length-(2*Zc))&31) == 0,"block_length-(2*Zc) needs to be a multiple of 32 for now\n");
    uint32_t l1 = (block_length-(2*Zc))>>5;
    uint32_t l2 = ((nrows-no_punctured_columns) * Zc-removed_bit)>>5;
    __m256i *c256p = (__m256i *)&cc[2*Zc];
    __m256i *d256p = (__m256i *)&dd[0];
    //  if (((block_length-(2*Zc))&31)>0) l1++;

    for (int i=0;i<l1;i++)
      //for (j=0;j<n_segments;j++) ((__m256i *)output[j])[i] = _mm256_and_si256(_mm256_srai_epi16(c256p[i],j),masks[0]);
    	for (int j=macro_segment; j < macro_segment_end; j++) ((__m256i *)output[j])[i] = _mm256_and_si256(_mm256_srai_epi16(c256p[i],j-macro_segment),masks[0]);


    //  if ((((nrows-no_punctured_columns) * Zc-removed_bit)&31)>0) l2++;

    for (int i1=0, i=l1;i1<l2;i1++,i++)
      //for (j=0;j<n_segments;j++) ((__m256i *)output[j])[i] = _mm256_and_si256(_mm256_srai_epi16(d256p[i1],j),masks[0]);
    	for (int j=macro_segment; j < macro_segment_end; j++)  ((__m256i *)output[j])[i] = _mm256_and_si256(_mm256_srai_epi16(d256p[i1],j-macro_segment),masks[0]);
  }
  else {
#ifdef DEBUG_LDPC
  LOG_W(PHY,"using non-optimized version\n");
#endif
    // do non-SIMD version
    for (int i=0;i<(block_length-2*Zc);i++)
      //for (j=0; j<n_segments; j++)
      for (int j=macro_segment; j < macro_segment_end; j++)
	output[j][i] = (cc[2*Zc+i]>>(j-macro_segment))&1;
    for (int i=0;i<((nrows-no_punctured_columns) * Zc-removed_bit);i++)
      //for (j=0; j<n_segments; j++)
    	  for (int j=macro_segment; j < macro_segment_end; j++)
	output[j][block_length-2*Zc+i] = (dd[i]>>(j-macro_segment))&1;
    }

#else
    AssertFatal(1==0,"Need AVX2 for now\n");
#endif

  if(impp->toutput != NULL) stop_meas(impp->toutput);
  return 0;
}

