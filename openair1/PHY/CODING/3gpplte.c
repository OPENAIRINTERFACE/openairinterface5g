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

/* file: 3gpplte.c
   purpose: Encoding routines for implementing Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03
   author: raymond.knopp@eurecom.fr
   date: 10.2009
*/
#ifndef TC_MAIN
//#include "defs.h"
#endif
#include <stdint.h>
#include <stdio.h>
#include "PHY/CODING/coding_defs.h"

//#define DEBUG_TURBO_ENCODER 1

uint32_t threegpplte_interleaver_output;
uint32_t threegpplte_interleaver_tmp;

inline void threegpplte_interleaver_reset(void)
{
  threegpplte_interleaver_output = 0;
  threegpplte_interleaver_tmp    = 0;
}
// pi_i = i*f1 + i*i*f2
// pi_i+1 = i*f1 + f1 + i*i*f2 + 2*i*f2 + f2
//        = pi_i + f1 + (2*i + 1)*f2
inline uint16_t threegpplte_interleaver(uint16_t f1,
                                        uint16_t f2,
                                        uint16_t K)
{

  threegpplte_interleaver_tmp = (threegpplte_interleaver_tmp+(f2<<1));
  /*
  if (threegpplte_interleaver_tmp>=K)
    threegpplte_interleaver_tmp-=K;
  */

  threegpplte_interleaver_output = (threegpplte_interleaver_output + threegpplte_interleaver_tmp + f1 - f2)%K;
  /*
  threegpplte_interleaver_output = (threegpplte_interleaver_output + threegpplte_interleaver_tmp + f1 - f2);
  if (threegpplte_interleaver_output>=K)
    threegpplte_interleaver_output-=K;
  */
#ifdef DEBUG_TURBO_ENCODER
  printf("pi(i) %u : 2*f2 * i = %u, f1 %u f2 %u, K %u, pi(i-1) %u \n",threegpplte_interleaver_output,threegpplte_interleaver_tmp,f1,f2,K,threegpplte_interleaver_output);
#endif
  return(threegpplte_interleaver_output);
}

inline uint8_t threegpplte_rsc(uint8_t input,uint8_t *state)
{

  uint8_t output;

  output = (input ^ (*state>>2) ^ (*state>>1))&1;
  *state = (((input<<2)^(*state>>1))^((*state>>1)<<2)^((*state)<<2))&7;
  return(output);

}

uint8_t output_lut[16],state_lut[16];

inline uint8_t threegpplte_rsc_lut(uint8_t input,uint8_t *state)
{


  uint8_t off;

  off = (*state<<1)|input;
  *state = state_lut[off];
  return(output_lut[off]);

}

inline void threegpplte_rsc_termination(uint8_t *x,uint8_t *z,uint8_t *state)
{


  *z     = ((*state>>2) ^ (*state))   &1;
  *x     = ((*state)    ^ (*state>>1))   &1;
  *state = (*state)>>1;


}

int turbo_encoder_init = 0;
uint32_t bit_byte_lut[2048];

void threegpplte_turbo_encoder(uint8_t *input,
                               uint16_t input_length_bytes,
                               uint8_t *output,
                               uint8_t F)
{

  int i,k=0;
  int dummy;
  uint8_t *x;
  uint8_t b,z,zprime,xprime;
  uint8_t state0=0,state1=0;
  uint16_t input_length_bits = input_length_bytes<<3, pi=0,pi_pos,pi_bitpos;
  uint32_t *bit_byte_lutp;
  short * base_interleaver;

  if (turbo_encoder_init==0) {
    turbo_encoder_init=1;

    for (state0=0; state0<8; state0++) {
      state1 = state0;
      output_lut[state0<<1] = threegpplte_rsc(0,&state1) ;
      state_lut[state0<<1] = state1;

      state1 = state0;
      output_lut[1+(state0<<1)] = threegpplte_rsc(1,&state1) ;
      state_lut[1+(state0<<1)]  = state1;

    }

    for (dummy=0; dummy<2048; dummy++) {
      b = dummy&7;
      bit_byte_lut[dummy] = ((dummy>>3)&(1<<(7-b)))>>(7-b);
    }
  }

  // look for f1 and f2 precomputed interleaver values
  for (i=0; f1f2mat[i].nb_bits!= input_length_bits && i <188; i++);

  if ( i == 188 ) {
    printf("Illegal frame length!\n");
    return;
  } else {
    base_interleaver=il_tb+f1f2mat[i].beg_index;
  }

  x = output;
  //  threegpplte_interleaver_reset();
  pi = 0;

  for (i=0; i<input_length_bytes; i++) {

#ifdef DEBUG_TURBO_ENCODER
    printf("\n****input %d  : %x\n",i,input[i]);
#endif //DEBUG_TURBO_ENCODER
    bit_byte_lutp=&bit_byte_lut[input[i]<<3];

    for (b=0; b<8; b++) {
      //      *x = (input[i]&(1<<(7-b)))>>(7-b);
      *x = bit_byte_lutp[b];
#ifdef DEBUG_TURBO_ENCODER
      printf("bit %d/%d: %d\n",b,b+(i<<3),*x);
#endif //DEBUG_TURBO_ENCODER

#ifdef DEBUG_TURBO_ENCODER
      printf("state0: %d\n",state0);
#endif //DEBUG_TURBO_ENCODER
      z               = threegpplte_rsc_lut(*x,&state0) ;

#ifdef DEBUG_TURBO_ENCODER
      printf("(x,z): (%d,%d),state0 %d\n",*x,z,state0);
#endif //DEBUG_TURBO_ENCODER

      // Filler bits get punctured
      if (k<F) {
        *x = LTE_NULL;
        z  = LTE_NULL;
      }

      pi_pos          = pi>>3;
      pi_bitpos       = pi&7;
      //      xprime          = (input[pi_pos]&(1<<(7-pi_bitpos)))>>(7-pi_bitpos);
      xprime          = bit_byte_lut[(input[pi_pos]<<3)+pi_bitpos];
      zprime          = threegpplte_rsc_lut(xprime,&state1);
#ifdef DEBUG_TURBO_ENCODER
      printf("pi %d, pi_pos %d, pi_bitpos %d, x %d, z %d, xprime %d, zprime %d, state0 %d state1 %d\n",pi,pi_pos,pi_bitpos,*x,z,xprime,zprime,state0,state1);
#endif //DEBUG_TURBO_ENCODER
      x[1]            = z;
      x[2]            = zprime;


      x+=3;

      pi              = *(++base_interleaver);//threegpplte_interleaver(interleaver_f1,interleaver_f2,input_length_bits);
      k++;
    }
  }

  // Trellis termination
  threegpplte_rsc_termination(&x[0],&x[1],&state0);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state0 %d\n",x[0],x[1],state0);
#endif //DEBUG_TURBO_ENCODER

  threegpplte_rsc_termination(&x[2],&x[3],&state0);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state0 %d\n",x[2],x[3],state0);
#endif //DEBUG_TURBO_ENCODER

  threegpplte_rsc_termination(&x[4],&x[5],&state0);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state0 %d\n",x[4],x[5],state0);
#endif //DEBUG_TURBO_ENCODER

  threegpplte_rsc_termination(&x[6],&x[7],&state1);

#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state1 %d\n",x[6],x[7],state1);
#endif //DEBUG_TURBO_ENCODER
  threegpplte_rsc_termination(&x[8],&x[9],&state1);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state1 %d\n",x[8],x[9],state1);
#endif //DEBUG_TURBO_ENCODER
  threegpplte_rsc_termination(&x[10],&x[11],&state1);

#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state1 %d\n",x[10],x[11],state1);
#endif //DEBUG_TURBO_ENCODER

}

inline short threegpp_interleaver_parameters(uint16_t bytes_per_codeword)
{
  if (bytes_per_codeword<=64)
    return (bytes_per_codeword-5);
  else if (bytes_per_codeword <=128)
    return (59 + ((bytes_per_codeword-64)>>1));
  else if (bytes_per_codeword <= 256)
    return (91 + ((bytes_per_codeword-128)>>2));
  else if (bytes_per_codeword <= 768)
    return (123 + ((bytes_per_codeword-256)>>3));
  else {
#ifdef DEBUG_TURBO_ENCODER
    printf("Illegal codeword size !!!\n");
#endif
    return(-1);
  }
}
 
const interleaver_TS_36_212_t f1f2[188] = {
  { 40, 3, 10 },
  { 48, 7, 12 },
  { 56, 19, 42 },
  { 64, 7, 16 },
  { 72, 7, 18 },
  { 80, 11, 20 },
  { 88, 5, 22 },
  { 96, 11, 24 },
  { 104, 7, 26 },
  { 112, 41, 84 },
  { 120, 103, 90 },
  { 128, 15, 32 },
  { 136, 9, 34 },
  { 144, 17, 108 },
  { 152, 9, 38 },
  { 160, 21, 120 },
  { 168, 101, 84 },
  { 176, 21, 44 },
  { 184, 57, 46 },
  { 192, 23, 48 },
  { 200, 13, 50 },
  { 208, 27, 52 },
  { 216, 11, 36 },
  { 224, 27, 56 },
  { 232, 85, 58 },
  { 240, 29, 60 },
  { 248, 33, 62 },
  { 256, 15, 32 },
  { 264, 17, 198 },
  { 272, 33, 68 },
  { 280, 103, 210 },
  { 288, 19, 36 },
  { 296, 19, 74 },
  { 304, 37, 76 },
  { 312, 19, 78 },
  { 320, 21, 120 },
  { 328, 21, 82 },
  { 336, 115, 84 },
  { 344, 193, 86 },
  { 352, 21, 44 },
  { 360, 133, 90 },
  { 368, 81, 46 },
  { 376, 45, 94 },
  { 384, 23, 48 },
  { 392, 243, 98 },
  { 400, 151, 40 },
  { 408, 155, 102 },
  { 416, 25, 52 },
  { 424, 51, 106 },
  { 432, 47, 72 },
  { 440, 91, 110 },
  { 448, 29, 168 },
  { 456, 29, 114 },
  { 464, 247, 58 },
  { 472, 29, 118 },
  { 480, 89, 180 },
  { 488, 91, 122 },
  { 496, 157, 62 },
  { 504, 55, 84 },
  { 512, 31, 64 },
  { 528, 17, 66 },
  { 544, 35, 68 },
  { 560, 227, 420 },
  { 576, 65, 96 },
  { 592, 19, 74 },
  { 608, 37, 76 },
  { 624, 41, 234 },
  { 640, 39, 80 },
  { 656, 185, 82 },
  { 672, 43, 252 },
  { 688, 21, 86 },
  { 704, 155, 44 },
  { 720, 79, 120 },
  { 736, 139, 92 },
  { 752, 23, 94 },
  { 768, 217, 48 },
  { 784, 25, 98 },
  { 800, 17, 80 },
  { 816, 127, 102 },
  { 832, 25, 52 },
  { 848, 239, 106 },
  { 864, 17, 48 },
  { 880, 137, 110 },
  { 896, 215, 112 },
  { 912, 29, 114 },
  { 928, 15, 58 },
  { 944, 147, 118 },
  { 960, 29, 60 },
  { 976, 59, 122 },
  { 992, 65, 124 },
  { 1008, 55, 84 },
  { 1024, 31, 64 },
  { 1056, 17, 66 },
  { 1088, 171, 204 },
  { 1120, 67, 140 },
  { 1152, 35, 72 },
  { 1184, 19, 74 },
  { 1216, 39, 76 },
  { 1248, 19, 78 },
  { 1280, 199, 240 },
  { 1312, 21, 82 },
  { 1344, 211, 252 },
  { 1376, 21, 86 },
  { 1408, 43, 88 },
  { 1440, 149, 60 },
  { 1472, 45, 92 },
  { 1504, 49, 846 },
  { 1536, 71, 48 },
  { 1568, 13, 28 },
  { 1600, 17, 80 },
  { 1632, 25, 102 },
  { 1664, 183, 104 },
  { 1696, 55, 954 },
  { 1728, 127, 96 },
  { 1760, 27, 110 },
  { 1792, 29, 112 },
  { 1824, 29, 114 },
  { 1856, 57, 116 },
  { 1888, 45, 354 },
  { 1920, 31, 120 },
  { 1952, 59, 610 },
  { 1984, 185, 124 },
  { 2016, 113, 420 },
  { 2048, 31, 64 },
  { 2112, 17, 66 },
  { 2176, 171, 136 },
  { 2240, 209, 420 },
  { 2304, 253, 216 },
  { 2368, 367, 444 },
  { 2432, 265, 456 },
  { 2496, 181, 468 },
  { 2560, 39, 80 },
  { 2624, 27, 164 },
  { 2688, 127, 504 },
  { 2752, 143, 172 },
  { 2816, 43, 88 },
  { 2880, 29, 300 },
  { 2944, 45, 92 },
  { 3008, 157, 188 },
  { 3072, 47, 96 },
  { 3136, 13, 28 },
  { 3200, 111, 240 },
  { 3264, 443, 204 },
  { 3328, 51, 104 },
  { 3392, 51, 212 },
  { 3456, 451, 192 },
  { 3520, 257, 220 },
  { 3584, 57, 336 },
  { 3648, 313, 228 },
  { 3712, 271, 232 },
  { 3776, 179, 236 },
  { 3840, 331, 120 },
  { 3904, 363, 244 },
  { 3968, 375, 248 },
  { 4032, 127, 168 },
  { 4096, 31, 64 },
  { 4160, 33, 130 },
  { 4224, 43, 264 },
  { 4288, 33, 134 },
  { 4352, 477, 408 },
  { 4416, 35, 138 },
  { 4480, 233, 280 },
  { 4544, 357, 142 },
  { 4608, 337, 480 },
  { 4672, 37, 146 },
  { 4736, 71, 444 },
  { 4800, 71, 120 },
  { 4864, 37, 152 },
  { 4928, 39, 462 },
  { 4992, 127, 234 },
  { 5056, 39, 158 },
  { 5120, 39, 80 },
  { 5184, 31, 96 },
  { 5248, 113, 902 },
  { 5312, 41, 166 },
  { 5376, 251, 336 },
  { 5440, 43, 170 },
  { 5504, 21, 86 },
  { 5568, 43, 174 },
  { 5632, 45, 176 },
  { 5696, 45, 178 },
  { 5760, 161, 120 },
  { 5824, 89, 182 },
  { 5888, 323, 184 },
  { 5952, 47, 186 },
  { 6016, 23, 94 },
  { 6080, 47, 190 },
  { 6144, 263, 480 },
};


t_interleaver_codebook *f1f2mat;
short *il_tb;
__attribute__((constructor)) static void init_interleaver(void) {
  int totSize=0;
  f1f2mat=(t_interleaver_codebook*) malloc(sizeof(*f1f2mat) * sizeof(f1f2) / sizeof(*f1f2));
  for (int i=0; i < sizeof(f1f2)/sizeof(*f1f2) ; i++) {
    f1f2mat[i].nb_bits=f1f2[i].nb_bits;
    f1f2mat[i].f1=f1f2[i].f1;
    f1f2mat[i].f2=f1f2[i].f2;
    f1f2mat[i].beg_index=totSize;
    totSize+=f1f2mat[i].nb_bits;
  }
  il_tb=(short*)malloc(sizeof(*il_tb)*totSize);
  int idx=0;
  for (int i=0; i < sizeof(f1f2)/sizeof(*f1f2) ; i++)
    for (uint64_t j=0; j<f1f2[i].nb_bits; j++)
    il_tb[idx++]=(j*f1f2[i].f1+j*j*f1f2[i].f2)%f1f2[i].nb_bits;
}


#ifdef MAIN

#define INPUT_LENGTH 5
#define F1 3
#define F2 10

int main(int argc,char **argv)
{

  uint8_t input[INPUT_LENGTH],state,state2;
  uint8_t output[12+(3*(INPUT_LENGTH<<3))],x,z;
  int i;
  uint8_t out;

  for (state=0; state<8; state++) {
    for (i=0; i<2; i++) {
      state2=state;
      out = threegpplte_rsc(i,&state2);
      printf("State (%d->%d) : (%d,%d)\n",state,state2,i,out);
    }
  }

  printf("\n");

  for (state=0; state<8; state++) {

    state2=state;
    threegpplte_rsc_termination(&x,&z,&state2);
    printf("Termination: (%d->%d) : (%d,%d)\n",state,state2,x,z);
  }

  for (i=0; i<5; i++) {
    input[i] = i*219;
    printf("Input %d : %x\n",i,input[i]);
  }

  threegpplte_turbo_encoder(&input[0],
                            5,
                            &output[0]);
  return(0);
}

#endif // MAIN
