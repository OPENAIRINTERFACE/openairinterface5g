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
#include "PHY/CODING/defs.h"
#include "extern_3GPPinterleaver.h"

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
  printf("pi(i) %d : 2*f2 * i = %d, f1 %d f2 %d, K %d, pi(i-1) %d \n",threegpplte_interleaver_output,threegpplte_interleaver_tmp,f1,f2,K,threegpplte_interleaver_output);
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
                               uint8_t F,
                               uint16_t interleaver_f1,
                               uint16_t interleaver_f2)
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
                            &output[0],
                            F1,
                            F2);
  return(0);
}

#endif // MAIN
