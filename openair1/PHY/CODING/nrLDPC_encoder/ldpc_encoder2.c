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
#include "defs.h"

#include "ldpc384_byte.c"
#include "ldpc352_byte.c"
#include "ldpc320_byte.c"
#include "ldpc288_byte.c"
#include "ldpc256_byte.c"
#include "ldpc240_byte.c"
#include "ldpc224_byte.c"
#include "ldpc208_byte.c"
#include "ldpc192_byte.c"
#include "ldpc176_byte.c"
#include "ldpc_BG2_Zc384_byte.c"
#include "ldpc_BG2_Zc352_byte.c"
#include "ldpc_BG2_Zc320_byte.c"
#include "ldpc_BG2_Zc288_byte.c"
#include "ldpc_BG2_Zc256_byte.c"
#include "ldpc_BG2_Zc240_byte.c"
#include "ldpc_BG2_Zc224_byte.c"
#include "ldpc_BG2_Zc208_byte.c"
#include "ldpc_BG2_Zc192_byte.c"
#include "ldpc_BG2_Zc176_byte.c"
#include "ldpc_BG2_Zc160_byte.c"
#include "ldpc_BG2_Zc144_byte.c"
#include "ldpc_BG2_Zc128_byte.c"
#include "ldpc_BG2_Zc120_byte.c"
#include "ldpc_BG2_Zc112_byte.c"
#include "ldpc_BG2_Zc104_byte.c"
#include "ldpc_BG2_Zc96_byte.c"
#include "ldpc_BG2_Zc88_byte.c"
#include "ldpc_BG2_Zc80_byte.c"
#include "ldpc_BG2_Zc72_byte.c"



void encode_parity_check_part_optim(uint8_t *c,uint8_t *d, short BG,short Zc,short Kb)
{

  if (BG==1)
  {
    switch (Zc)
    {
    case 2: break;
    case 3: break;
    case 4: break;
    case 5: break;
    case 6: break;
    case 7: break;
    case 8: break;
    case 9: break;
    case 10: break;
    case 11: break;
    case 12: break;
    case 13: break;
    case 14: break;
    case 15: break;
    case 16: break;
    case 18: break;
    case 20: break;
    case 22: break;
    case 24: break;      
    case 26: break;
    case 28: break;
    case 30: break;
    case 32: break;
    case 36: break;
    case 40: break;
    case 44: break;
    case 48: break;
    case 52: break;
    case 56: break;
    case 60: break;
    case 64: break;
    case 72: break;
    case 80: break;   
    case 88: break;   
    case 96: break;
    case 104: break;
    case 112: break;
    case 120: break;
    case 128: break;
    case 144: break;
    case 160: break;
    case 176: ldpc176_byte(c,d); break;
    case 192: ldpc192_byte(c,d); break;
    case 208: ldpc208_byte(c,d); break;
    case 224: ldpc224_byte(c,d); break;
    case 240: ldpc240_byte(c,d); break;
    case 256: ldpc256_byte(c,d); break;
    case 288: ldpc288_byte(c,d); break;
    case 320: ldpc320_byte(c,d); break;
    case 352: ldpc352_byte(c,d); break;
    case 384: ldpc384_byte(c,d); break;
    default: AssertFatal(0,"BG %d Zc %d is not supported yet\n",BG,Zc); break;
    }
  }
  else if (BG==2) {
    switch (Zc)
    {
    case 2: break;
    case 3: break;
    case 4: break;
    case 5: break;
    case 6: break;
    case 7: break;
    case 8: break;
    case 9: break;
    case 10: break;
    case 11: break;
    case 12: break;
    case 13: break;
    case 14: break;
    case 15: break;
    case 16: break;
    case 18: break;
    case 20: break;
    case 22: break;
    case 24: break;      
    case 26: break;
    case 28: break;
    case 30: break;
    case 32: break;
    case 36: break;
    case 40: break;
    case 44: break;
    case 48: break;
    case 52: break;
    case 56: break;
    case 60: break;
    case 64: break;
    case 72: ldpc_BG2_Zc72_byte(c,d); break;
    case 80: ldpc_BG2_Zc80_byte(c,d); break;   
    case 88: ldpc_BG2_Zc88_byte(c,d); break;   
    case 96: ldpc_BG2_Zc96_byte(c,d); break;
    case 104: ldpc_BG2_Zc104_byte(c,d); break;
    case 112: ldpc_BG2_Zc112_byte(c,d); break;
    case 120: ldpc_BG2_Zc120_byte(c,d); break;
    case 128: ldpc_BG2_Zc128_byte(c,d); break;
    case 144: ldpc_BG2_Zc144_byte(c,d); break;
    case 160: ldpc_BG2_Zc160_byte(c,d); break;
    case 176: ldpc_BG2_Zc176_byte(c,d); break;
    case 192: ldpc_BG2_Zc192_byte(c,d); break;
    case 208: ldpc_BG2_Zc208_byte(c,d); break;
    case 224: ldpc_BG2_Zc224_byte(c,d); break;
    case 240: ldpc_BG2_Zc240_byte(c,d); break;
    case 256: ldpc_BG2_Zc256_byte(c,d); break;
    case 288: ldpc_BG2_Zc288_byte(c,d); break;
    case 320: ldpc_BG2_Zc320_byte(c,d); break;
    case 352: ldpc_BG2_Zc352_byte(c,d); break;
    case 384: ldpc_BG2_Zc384_byte(c,d); break;
    default: AssertFatal(0,"BG %d Zc %d is not supported yet\n",BG,Zc); break;
    }
  }
  else {
    AssertFatal(0,"BG %d is not supported yet\n",BG);
  } 

}


int ldpc_encoder_optim(unsigned char *test_input,unsigned char *channel_input,int Zc,int Kb,short block_length,short BG,time_stats_t *tinput,time_stats_t *tprep,time_stats_t *tparity,time_stats_t *toutput)
{

  short nrows=0,ncols=0;
  int i,i1,rate=3;
  int no_punctured_columns,removed_bit;

  int simd_size;

  //determine number of bits in codeword
   //if (block_length>3840)
   if (BG==1)
     {
       //BG=1;
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
  LOG_D(PHY,"ldpc_encoder_optim_8seg: BG %d, Zc %d, Kb %d, block_length %d\n",BG,Zc,Kb,block_length);
  LOG_D(PHY,"ldpc_encoder_optim_8seg: PDU %x %x %x %x\n",test_input[0],test_input[1],test_input[2],test_input[3]);
#endif

  if ((Zc&31) > 0) simd_size = 16;
  else simd_size = 32;

  unsigned char c[22*Zc] __attribute__((aligned(32))); //padded input, unpacked, max size
  unsigned char d[46*Zc] __attribute__((aligned(32))); //coded parity part output, unpacked, max size

  unsigned char c_extension[2*22*Zc*simd_size] __attribute__((aligned(32)));      //double size matrix of c

  // calculate number of punctured bits
  no_punctured_columns=(int)((nrows-2)*Zc+block_length-block_length*rate)/Zc;
  removed_bit=(nrows-no_punctured_columns-2) * Zc+block_length-(int)(block_length*rate);
  // printf("%d\n",no_punctured_columns);
  // printf("%d\n",removed_bit);
  // unpack input
  memset(c,0,sizeof(unsigned char) * ncols * Zc);
  memset(d,0,sizeof(unsigned char) * nrows * Zc);

  if(tinput != NULL) start_meas(tinput);
  for (i=0; i<block_length; i++) {
    c[i] = (test_input[i/8]&(128>>(i&7)))>>(7-(i&7));
      //printf("c(%d,%d)=%d\n",j,i,temp);
    }

  if(tinput != NULL) stop_meas(tinput);

  if ((BG==1 && Zc>176) || (BG==2 && Zc>64)) { 
    // extend matrix
    if(tprep != NULL) start_meas(tprep);
    for (i1=0; i1 < ncols; i1++)
      {
	memcpy(&c_extension[2*i1*Zc], &c[i1*Zc], Zc*sizeof(unsigned char));
	memcpy(&c_extension[(2*i1+1)*Zc], &c[i1*Zc], Zc*sizeof(unsigned char));
      }
    for (i1=1;i1<simd_size;i1++) {
      memcpy(&c_extension[(2*ncols*Zc*i1)], &c_extension[i1], (2*ncols*Zc*sizeof(unsigned char))-i1);
      //    memset(&c_extension[(2*ncols*Zc*i1)],0,i1);
      /*
	printf("shift %d: ",i1);
	for (int j=0;j<64;j++) printf("%d ",c_extension[(2*ncols*Zc*i1)+j]);
	printf("\n");
      */
    }
    if(tprep != NULL) stop_meas(tprep);
    //parity check part
    if(tparity != NULL) start_meas(tparity);
    encode_parity_check_part_optim(c_extension, d, BG, Zc, Kb);
    if(tparity != NULL) stop_meas(tparity);
  }
  else {
    if (encode_parity_check_part_orig(c, d, BG, Zc, Kb, block_length)!=0) {
      printf("Problem with encoder\n");
      return(-1);
    }
  }
  if(toutput != NULL) start_meas(toutput);
  // information part and puncture columns
  memcpy(&channel_input[0], &c[2*Zc], (block_length-2*Zc)*sizeof(unsigned char));
  memcpy(&channel_input[block_length-2*Zc], &d[0], ((nrows-no_punctured_columns) * Zc-removed_bit)*sizeof(unsigned char));

  if(toutput != NULL) stop_meas(toutput);
  return 0;
}

int ldpc_encoder_optim_8seg(unsigned char **test_input,unsigned char **channel_input,int Zc,int Kb,short block_length,short BG,int n_segments,time_stats_t *tinput,time_stats_t *tprep,time_stats_t *tparity,time_stats_t *toutput)
{

  short nrows=0,ncols=0;
  int i,i1,j,rate=3;
  int no_punctured_columns,removed_bit;
  char temp;
  int simd_size;

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

  AssertFatal(n_segments>0&&n_segments<=8,"0 < n_segments %d <= 8\n",n_segments);

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

  unsigned char c_extension[2*22*Zc*simd_size] __attribute__((aligned(32)));      //double size matrix of c

  // calculate number of punctured bits
  no_punctured_columns=(int)((nrows-2)*Zc+block_length-block_length*rate)/Zc;
  removed_bit=(nrows-no_punctured_columns-2) * Zc+block_length-(int)(block_length*rate);
  // printf("%d\n",no_punctured_columns);
  // printf("%d\n",removed_bit);
  // unpack input
  memset(c,0,sizeof(unsigned char) * ncols * Zc);
  memset(d,0,sizeof(unsigned char) * nrows * Zc);

  if(tinput != NULL) start_meas(tinput);
#if 0
  for (i=0; i<block_length; i++) {
    for (j=0; j<n_segments; j++) {

      temp = (test_input[j][i/8]&(128>>(i&7)))>>(7-(i&7));
      //printf("c(%d,%d)=%d\n",j,i,temp);
      c[i] |= (temp << j);
    }
  }
#else
#ifdef __AVX2__
  for (i=0; i<block_length>>5; i++) {
    c256 = _mm256_and_si256(_mm256_cmpeq_epi8(_mm256_andnot_si256(_mm256_shuffle_epi8(_mm256_set1_epi32(((uint32_t*)test_input[0])[i]), shufmask),andmask),zero256),masks[0]);
    for (j=1; j<n_segments; j++) {
      c256 = _mm256_or_si256(_mm256_and_si256(_mm256_cmpeq_epi8(_mm256_andnot_si256(_mm256_shuffle_epi8(_mm256_set1_epi32(((uint32_t*)test_input[j])[i]), shufmask),andmask),zero256),masks[j]),c256);
    }
    ((__m256i *)c)[i] = c256;
  }

  for (i=(block_length>>5)<<5;i<block_length;i++) {
    for (j=0; j<n_segments; j++) {

      temp = (test_input[j][i/8]&(128>>(i&7)))>>(7-(i&7));
      //printf("c(%d,%d)=%d\n",j,i,temp);
      c[i] |= (temp << j);
    }
  }
#else
  AssertFatal(1==0,"Need AVX2 for this\n");
#endif
#endif

  if(tinput != NULL) stop_meas(tinput);

  if ((BG==1 && Zc>176) || (BG==2 && Zc>64)) { 
    // extend matrix
    if(tprep != NULL) start_meas(tprep);
    for (i1=0; i1 < ncols; i1++)
      {
	memcpy(&c_extension[2*i1*Zc], &c[i1*Zc], Zc*sizeof(unsigned char));
	memcpy(&c_extension[(2*i1+1)*Zc], &c[i1*Zc], Zc*sizeof(unsigned char));
      }
    for (i1=1;i1<simd_size;i1++) {
      memcpy(&c_extension[(2*ncols*Zc*i1)], &c_extension[i1], (2*ncols*Zc*sizeof(unsigned char))-i1);
      //    memset(&c_extension[(2*ncols*Zc*i1)],0,i1);
      /*
	printf("shift %d: ",i1);
	for (int j=0;j<64;j++) printf("%d ",c_extension[(2*ncols*Zc*i1)+j]);
	printf("\n");
      */
    }
    if(tprep != NULL) stop_meas(tprep);
    //parity check part
    if(tparity != NULL) start_meas(tparity);
    encode_parity_check_part_optim(c_extension, d, BG, Zc, Kb);
    if(tparity != NULL) stop_meas(tparity);
  }
  else {
    if (encode_parity_check_part_orig(c, d, BG, Zc, Kb, block_length)!=0) {
      printf("Problem with encoder\n");
      return(-1);
    }
  }
  if(toutput != NULL) start_meas(toutput);
  // information part and puncture columns
  /*
  memcpy(&channel_input[0], &c[2*Zc], (block_length-2*Zc)*sizeof(unsigned char));
  memcpy(&channel_input[block_length-2*Zc], &d[0], ((nrows-no_punctured_columns) * Zc-removed_bit)*sizeof(unsigned char));
  */
#ifdef __AVX2__
  if ((((2*Zc)&31) == 0) && (((block_length-(2*Zc))&31) == 0)) {
    //AssertFatal(((2*Zc)&31) == 0,"2*Zc needs to be a multiple of 32 for now\n");
    //AssertFatal(((block_length-(2*Zc))&31) == 0,"block_length-(2*Zc) needs to be a multiple of 32 for now\n");
    uint32_t l1 = (block_length-(2*Zc))>>5;
    uint32_t l2 = ((nrows-no_punctured_columns) * Zc-removed_bit)>>5;
    __m256i *c256p = (__m256i *)&c[2*Zc];
    __m256i *d256p = (__m256i *)&d[0];
    //  if (((block_length-(2*Zc))&31)>0) l1++;
    
    for (i=0;i<l1;i++)
      for (j=0;j<n_segments;j++) ((__m256i *)channel_input[j])[i] = _mm256_and_si256(_mm256_srai_epi16(c256p[i],j),masks[0]);
    
    //  if ((((nrows-no_punctured_columns) * Zc-removed_bit)&31)>0) l2++;
    
    for (i1=0;i1<l2;i1++,i++)
      for (j=0;j<n_segments;j++) ((__m256i *)channel_input[j])[i] = _mm256_and_si256(_mm256_srai_epi16(d256p[i1],j),masks[0]);
  }
  else {
#ifdef DEBUG_LDPC
  LOG_W(PHY,"using non-optimized version\n");
#endif
    // do non-SIMD version
    for (i=0;i<(block_length-2*Zc);i++) 
      for (j=0; j<n_segments; j++)
	channel_input[j][i] = (c[2*Zc+i]>>j)&1;
    for (i=0;i<((nrows-no_punctured_columns) * Zc-removed_bit);i++)
      for (j=0; j<n_segments; j++)
	channel_input[j][block_length-2*Zc+i] = (d[i]>>j)&1;
    }

#else
    AssertFatal(1==0,"Need AVX2 for now\n");
#endif

  if(toutput != NULL) stop_meas(toutput);
  return 0;
}

int ldpc_encoder_optim_8seg_multi(unsigned char **test_input,unsigned char **channel_input,int Zc,int Kb,short block_length, short BG, int n_segments,unsigned int macro_num, time_stats_t *tinput,time_stats_t *tprep,time_stats_t *tparity,time_stats_t *toutput)
{

  short nrows=0,ncols=0;
  int i,i1,j,rate=3;
  int no_punctured_columns,removed_bit;
  //Table of possible lifting sizes
  char temp;
  int simd_size;
  unsigned int macro_segment, macro_segment_end;

  
  macro_segment = 8*macro_num;
  macro_segment_end = (n_segments > 8*(macro_num+1)) ? 8*(macro_num+1) : n_segments;
  //macro_segment_end = macro_segment + (n_segments > 8 ? 8 : n_segments);
  //printf("macro_segment: %d\n", macro_segment);
  //printf("macro_segment_end: %d\n", macro_segment_end );

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
  LOG_D(PHY,"ldpc_encoder_optim_8seg: BG %d, Zc %d, Kb %d, block_length %d, segments %d\n",BG,Zc,Kb,block_length,n_segments);
  LOG_D(PHY,"ldpc_encoder_optim_8seg: PDU (seg 0) %x %x %x %x\n",test_input[0][0],test_input[0][1],test_input[0][2],test_input[0][3]);
#endif

  AssertFatal(Zc>0,"no valid Zc found for block length %d\n",block_length);

  if ((Zc&31) > 0) simd_size = 16;
  else          simd_size = 32;

  unsigned char c[22*Zc] __attribute__((aligned(32))); //padded input, unpacked, max size
  unsigned char d[46*Zc] __attribute__((aligned(32))); //coded parity part output, unpacked, max size

  unsigned char c_extension[2*22*Zc*simd_size] __attribute__((aligned(32)));      //double size matrix of c

  // calculate number of punctured bits
  no_punctured_columns=(int)((nrows-2)*Zc+block_length-block_length*rate)/Zc;
  removed_bit=(nrows-no_punctured_columns-2) * Zc+block_length-(int)(block_length*rate);
  //printf("%d\n",no_punctured_columns);
  //printf("%d\n",removed_bit);
  // unpack input
  memset(c,0,sizeof(unsigned char) * ncols * Zc);
  memset(d,0,sizeof(unsigned char) * nrows * Zc);

  if(tinput != NULL) start_meas(tinput);
#if 0
  for (i=0; i<block_length; i++) {
	//for (j=0; j<n_segments; j++) {
    for (j=macro_segment; j < macro_segment_end; j++) {

      temp = (test_input[j][i/8]&(1<<(i&7)))>>(i&7);
      //printf("c(%d,%d)=%d\n",j,i,temp);
      c[i] |= (temp << (j-macro_segment));
    }
  }
#else
#ifdef __AVX2__
  for (i=0; i<block_length>>5; i++) {
    c256 = _mm256_and_si256(_mm256_cmpeq_epi8(_mm256_andnot_si256(_mm256_shuffle_epi8(_mm256_set1_epi32(((uint32_t*)test_input[macro_segment])[i]), shufmask),andmask),zero256),masks[0]);
    //for (j=1; j<n_segments; j++) {
    for (j=macro_segment+1; j < macro_segment_end; j++) {
      c256 = _mm256_or_si256(_mm256_and_si256(_mm256_cmpeq_epi8(_mm256_andnot_si256(_mm256_shuffle_epi8(_mm256_set1_epi32(((uint32_t*)test_input[j])[i]), shufmask),andmask),zero256),masks[j-macro_segment]),c256);
    }
    ((__m256i *)c)[i] = c256;
  }

  for (i=(block_length>>5)<<5;i<block_length;i++) {
    //for (j=0; j<n_segments; j++) {
	  for (j=macro_segment; j < macro_segment_end; j++) {

	    temp = (test_input[j][i/8]&(128>>(i&7)))>>(7-(i&7));
      //printf("c(%d,%d)=%d\n",j,i,temp);
      c[i] |= (temp << (j-macro_segment));
    }
  }
#else
  AssertFatal(1==0,"Need AVX2 for this\n");
#endif
#endif

  if(tinput != NULL) stop_meas(tinput);

  if ((BG==1 && Zc>176) || (BG==2 && Zc>64)) {
    // extend matrix
    if(tprep != NULL) start_meas(tprep);
    for (i1=0; i1 < ncols; i1++)
      {
	memcpy(&c_extension[2*i1*Zc], &c[i1*Zc], Zc*sizeof(unsigned char));
	memcpy(&c_extension[(2*i1+1)*Zc], &c[i1*Zc], Zc*sizeof(unsigned char));
      }
    for (i1=1;i1<simd_size;i1++) {
      memcpy(&c_extension[(2*ncols*Zc*i1)], &c_extension[i1], (2*ncols*Zc*sizeof(unsigned char))-i1);
      //    memset(&c_extension[(2*ncols*Zc*i1)],0,i1);
      /*
	printf("shift %d: ",i1);
	for (int j=0;j<64;j++) printf("%d ",c_extension[(2*ncols*Zc*i1)+j]);
	printf("\n");
      */
    }
    if(tprep != NULL) stop_meas(tprep);
    //parity check part
    if(tparity != NULL) start_meas(tparity);
    encode_parity_check_part_optim(c_extension, d, BG, Zc, Kb);
    if(tparity != NULL) stop_meas(tparity);
  }
  else {
    if (encode_parity_check_part_orig(c, d, BG, Zc, Kb, block_length)!=0) {
      printf("Problem with encoder\n");
      return(-1);
    }
  }
  if(toutput != NULL) start_meas(toutput);
  // information part and puncture columns
  /*
  memcpy(&channel_input[0], &c[2*Zc], (block_length-2*Zc)*sizeof(unsigned char));
  memcpy(&channel_input[block_length-2*Zc], &d[0], ((nrows-no_punctured_columns) * Zc-removed_bit)*sizeof(unsigned char));
  */
#ifdef __AVX2__
  if ((((2*Zc)&31) == 0) && (((block_length-(2*Zc))&31) == 0)) {
    //AssertFatal(((2*Zc)&31) == 0,"2*Zc needs to be a multiple of 32 for now\n");
    //AssertFatal(((block_length-(2*Zc))&31) == 0,"block_length-(2*Zc) needs to be a multiple of 32 for now\n");
    uint32_t l1 = (block_length-(2*Zc))>>5;
    uint32_t l2 = ((nrows-no_punctured_columns) * Zc-removed_bit)>>5;
    __m256i *c256p = (__m256i *)&c[2*Zc];
    __m256i *d256p = (__m256i *)&d[0];
    //  if (((block_length-(2*Zc))&31)>0) l1++;

    for (i=0;i<l1;i++)
      //for (j=0;j<n_segments;j++) ((__m256i *)channel_input[j])[i] = _mm256_and_si256(_mm256_srai_epi16(c256p[i],j),masks[0]);
    	for (j=macro_segment; j < macro_segment_end; j++) ((__m256i *)channel_input[j])[i] = _mm256_and_si256(_mm256_srai_epi16(c256p[i],j-macro_segment),masks[0]);


    //  if ((((nrows-no_punctured_columns) * Zc-removed_bit)&31)>0) l2++;

    for (i1=0;i1<l2;i1++,i++)
      //for (j=0;j<n_segments;j++) ((__m256i *)channel_input[j])[i] = _mm256_and_si256(_mm256_srai_epi16(d256p[i1],j),masks[0]);
    	for (j=macro_segment; j < macro_segment_end; j++)  ((__m256i *)channel_input[j])[i] = _mm256_and_si256(_mm256_srai_epi16(d256p[i1],j-macro_segment),masks[0]);
  }
  else {
#ifdef DEBUG_LDPC
  LOG_W(PHY,"using non-optimized version\n");
#endif
    // do non-SIMD version
    for (i=0;i<(block_length-2*Zc);i++)
      //for (j=0; j<n_segments; j++)
      for (j=macro_segment; j < macro_segment_end; j++)
	channel_input[j][i] = (c[2*Zc+i]>>(j-macro_segment))&1;
    for (i=0;i<((nrows-no_punctured_columns) * Zc-removed_bit);i++)
      //for (j=0; j<n_segments; j++)
    	  for (j=macro_segment; j < macro_segment_end; j++)
	channel_input[j][block_length-2*Zc+i] = (d[i]>>(j-macro_segment))&1;
    }

#else
    AssertFatal(1==0,"Need AVX2 for now\n");
#endif

  if(toutput != NULL) stop_meas(toutput);
  return 0;
}

