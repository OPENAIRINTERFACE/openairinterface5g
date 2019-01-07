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

/* file: viterbit_lte.c
   purpose: SIMD optimized LTE Viterbi Decoder for rate 1/3 Tail-biting convolutional code.  Performs two iterations
            of code.  First pass does Viterbi with all initial partial metrics set to zero.  Second pass does Viterbi
            with initial partial metrics set to values from final state values after first pass. Max is selected at
      end to do trace-back.
   author: raymond.knopp@eurecom.fr
   date: 21.10.2009
*/

#ifndef TEST_DEBUG
#include "PHY/defs_common.h"
#include "PHY/phy_extern.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define msg printf
#endif


#include "PHY/sse_intrin.h"

extern uint8_t ccodelte_table[128],ccodelte_table_rev[128];




static int8_t m0_table[64*16*16*16] __attribute__ ((aligned(16)));
static int8_t m1_table[64*16*16*16] __attribute__ ((aligned(16)));


// Set up Viterbi tables for SSE2 implementation
void phy_generate_viterbi_tables_lte()
{

  int8_t w[8],in0,in1,in2;
  uint8_t state,index0,index1;

  for (in0 = -8; in0 <8 ; in0++) {   // use 4-bit quantization
    for (in1 = -8; in1 <8 ; in1++) {
      for (in2 = -8; in2 <8 ; in2++) {


        w[0] = 24 - in0 - in1 - in2;           // -1,-1,-1
        w[1] = 24 + in0 - in1 - in2;           // -1, 1,-1
        w[2] = 24 - in0 + in1 - in2;           //  1,-1,-1
        w[3] = 24 + in0 + in1 - in2;           //  1, 1,-1
        w[4] = 24 - in0 - in1 + in2;           // -1,-1, 1
        w[5] = 24 + in0 - in1 + in2;           // -1, 1, 1
        w[6] = 24 - in0 + in1 + in2;           //  1,-1, 1
        w[7] = 24 + in0 + in1 + in2;           //  1, 1, 1

        //    printf("w: %d %d %d %d\n",w[0],w[1],w[2],w[3]);
        for (state=0; state<64 ; state++) {

          // input 0
          index0 = (state<<1);

          m0_table[(in0+8 + (16*(in1+8)) + (256*(in2+8)))*64 +state] = w[ccodelte_table_rev[index0]];


          //    if (position < 8)
          //    printf("%d,%d : prev_state0 = %d,m0 = %d,w=%d (%d)\n",position,state,prev_state0%64,m0,w[ccodelte_table[prev_state0]],partial_metrics[prev_state0%64]);

          // input 1
          index1 = (1+ (state<<1));
          m1_table[(in0+8 + (16*(in1+8)) + (256*(in2+8)))*64 +state] = w[ccodelte_table_rev[index1]];

        }
      }
    }
  }
}


#define INIT0 0x00000080
#define RESCALE 0x00000040

//#define DEBUG_VITERBI

#ifdef DEBUG_VITERBI
void print_bytes(char *s,__m128i *x)
{

  uint8_t *tempb = (uint8_t *)x;

  printf("%s  : %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",s,
         tempb[0],tempb[1],tempb[2],tempb[3],tempb[4],tempb[5],tempb[6],tempb[7],
         tempb[8],tempb[9],tempb[10],tempb[11],tempb[12],tempb[13],tempb[14],tempb[15]);

}

/*
void print_shorts(__m128i x,char *s) {

  int16_t *tempb = (int16_t *)&x;

  printf("%s  : %d,%d,%d,%d,%d,%d,%d,%d\n",s,
   tempb[0],tempb[1],tempb[2],tempb[3],tempb[4],tempb[5],tempb[6],tempb[7]
   );

}
*/
#endif // DEBUG_VITERBI



void phy_viterbi_lte_sse2(int8_t *y,uint8_t *decoded_bytes,uint16_t n)
{


#if defined(__x86_64__) || defined(__i386__)
  __m128i  TB[4*8192];
  __m128i *m0_ptr,*m1_ptr,*TB_ptr = &TB[0];
  
  __m128i metrics0_15,metrics16_31,metrics32_47,metrics48_63,even0_30a,even0_30b,even32_62a,even32_62b,odd1_31a,odd1_31b,odd33_63a,odd33_63b,TBeven0_30,TBeven32_62,TBodd1_31,
    TBodd33_63;
  
  __m128i min_state,min_state2;

#elif defined(__arm__)
  uint8x16x2_t TB[2*8192];  // 2 int8x16_t per input bit, 8 bits / byte, 8192 is largest packet size in bits

  uint8x16_t even0_30a,even0_30b,even32_62a,even32_62b,odd1_31a,odd1_31b,odd33_63a,odd33_63b,TBeven0_30,TBeven32_62,TBodd1_31,TBodd33_63;
  uint8x16x2_t metrics0_31,metrics32_63;

  uint8x16_t min_state;

  uint8x16_t *m0_ptr,*m1_ptr;
  uint8x16x2_t *TB_ptr = &TB[0];


#endif
  int8_t *in = y;
  uint8_t prev_state0,maxm,s;
  static uint8_t *TB_ptr2;
  uint32_t table_offset;
  uint8_t iter;
  int16_t position;

  // set initial metrics
  //debug_msg("Doing viterbi\n");

#if defined(__x86_64__) || defined(__i386__)

  metrics0_15  = _mm_setzero_si128();
  metrics16_31 = _mm_setzero_si128();
  metrics32_47 = _mm_setzero_si128();
  metrics48_63 = _mm_setzero_si128();
#elif defined(__arm__)
    metrics0_31.val[0]  = vdupq_n_u8(0); 
    metrics0_31.val[1]  = vdupq_n_u8(0);
    metrics32_63.val[0] = vdupq_n_u8(0);
    metrics32_63.val[1] = vdupq_n_u8(0);
#endif

  for (iter=0; iter<2; iter++) {
    in = y;
    TB_ptr=&TB[0];

    for (position=0; position<n; position++) {


      // get branch metric offsets for the 64 states
      table_offset = (in[0]+8 + ((in[1]+8)<<4) + ((in[2]+8)<<8))<<6;

#if defined(__x86_64__) || defined(__i386__)
      m0_ptr = (__m128i *)&m0_table[table_offset];
      m1_ptr = (__m128i *)&m1_table[table_offset];

      // even states
      even0_30a  = _mm_adds_epu8(metrics0_15,m0_ptr[0]);
      even32_62a = _mm_adds_epu8(metrics16_31,m0_ptr[1]);
      even0_30b  = _mm_adds_epu8(metrics32_47,m0_ptr[2]);
      even32_62b = _mm_adds_epu8(metrics48_63,m0_ptr[3]);


      // odd states
      odd1_31a   = _mm_adds_epu8(metrics0_15,m1_ptr[0]);
      odd33_63a  = _mm_adds_epu8(metrics16_31,m1_ptr[1]);
      odd1_31b   = _mm_adds_epu8(metrics32_47,m1_ptr[2]);
      odd33_63b  = _mm_adds_epu8(metrics48_63,m1_ptr[3]);

      // select maxima

      even0_30a  = _mm_max_epu8(even0_30a,even0_30b);
      even32_62a = _mm_max_epu8(even32_62a,even32_62b);
      odd1_31a   = _mm_max_epu8(odd1_31a,odd1_31b);
      odd33_63a  = _mm_max_epu8(odd33_63a,odd33_63b);

      // Traceback information

      TBeven0_30  = _mm_cmpeq_epi8(even0_30a,even0_30b);
      TBeven32_62 = _mm_cmpeq_epi8(even32_62a,even32_62b);
      TBodd1_31   = _mm_cmpeq_epi8(odd1_31a,odd1_31b);
      TBodd33_63  = _mm_cmpeq_epi8(odd33_63a,odd33_63b);

      metrics0_15        = _mm_unpacklo_epi8(even0_30a ,odd1_31a);
      metrics16_31       = _mm_unpackhi_epi8(even0_30a ,odd1_31a);
      metrics32_47       = _mm_unpacklo_epi8(even32_62a,odd33_63a);
      metrics48_63       = _mm_unpackhi_epi8(even32_62a,odd33_63a);

      TB_ptr[0]  = _mm_unpacklo_epi8(TBeven0_30,TBodd1_31);
      TB_ptr[1] = _mm_unpackhi_epi8(TBeven0_30,TBodd1_31);
      TB_ptr[2] = _mm_unpacklo_epi8(TBeven32_62,TBodd33_63);
      TB_ptr[3] = _mm_unpackhi_epi8(TBeven32_62,TBodd33_63);


      in+=3;
      TB_ptr += 4;

      // rescale by subtracting minimum
      /****************************************************
      USE SSSE instruction phminpos!!!!!!!
      ****************************************************/
      min_state =_mm_min_epu8(metrics0_15,metrics16_31);
      min_state =_mm_min_epu8(min_state,metrics32_47);
      min_state =_mm_min_epu8(min_state,metrics48_63);

      min_state2 = min_state;
      min_state  = _mm_unpacklo_epi8(min_state,min_state);
      min_state2 = _mm_unpackhi_epi8(min_state2,min_state2);
      min_state  = _mm_min_epu8(min_state,min_state2);

      min_state2 = min_state;
      min_state  = _mm_unpacklo_epi8(min_state,min_state);
      min_state2 = _mm_unpackhi_epi8(min_state2,min_state2);
      min_state  = _mm_min_epu8(min_state,min_state2);

      min_state2 = min_state;
      min_state  = _mm_unpacklo_epi8(min_state,min_state);
      min_state2 = _mm_unpackhi_epi8(min_state2,min_state2);
      min_state  = _mm_min_epu8(min_state,min_state2);

      min_state2 = min_state;
      min_state  = _mm_unpacklo_epi8(min_state,min_state);
      min_state2 = _mm_unpackhi_epi8(min_state2,min_state2);
      min_state  = _mm_min_epu8(min_state,min_state2);

      metrics0_15  = _mm_subs_epu8(metrics0_15,min_state);
      metrics16_31 = _mm_subs_epu8(metrics16_31,min_state);
      metrics32_47 = _mm_subs_epu8(metrics32_47,min_state);
      metrics48_63 = _mm_subs_epu8(metrics48_63,min_state);
#elif defined(__arm__)
    m0_ptr = (uint8x16_t *)&m0_table[table_offset];
    m1_ptr = (uint8x16_t *)&m1_table[table_offset];


    // even states
    even0_30a  = vqaddq_u8(metrics0_31.val[0],m0_ptr[0]);
    even32_62a = vqaddq_u8(metrics0_31.val[1],m0_ptr[1]);
    even0_30b  = vqaddq_u8(metrics32_63.val[0],m0_ptr[2]);
    even32_62b = vqaddq_u8(metrics32_63.val[1],m0_ptr[3]);

    // odd states
    odd1_31a   = vqaddq_u8(metrics0_31.val[0],m1_ptr[0]);
    odd33_63a  = vqaddq_u8(metrics0_31.val[1],m1_ptr[1]);
    odd1_31b   = vqaddq_u8(metrics32_63.val[0],m1_ptr[2]);
    odd33_63b  = vqaddq_u8(metrics32_63.val[1],m1_ptr[3]);
    // select maxima
    even0_30a  = vmaxq_u8(even0_30a,even0_30b);
    even32_62a = vmaxq_u8(even32_62a,even32_62b);
    odd1_31a   = vmaxq_u8(odd1_31a,odd1_31b);
    odd33_63a  = vmaxq_u8(odd33_63a,odd33_63b);

    // Traceback information
    TBeven0_30  = vceqq_u8(even0_30a,even0_30b);
    TBeven32_62 = vceqq_u8(even32_62a,even32_62b);
    TBodd1_31   = vceqq_u8(odd1_31a,odd1_31b);
    TBodd33_63  = vceqq_u8(odd33_63a,odd33_63b);

    metrics0_31  = vzipq_u8(even0_30a,odd1_31a);
    metrics32_63 = vzipq_u8(even32_62a,odd33_63a);

    TB_ptr[0] = vzipq_u8(TBeven0_30,TBodd1_31);
    TB_ptr[1] = vzipq_u8(TBeven32_62,TBodd33_63);

    in+=2;
    TB_ptr += 2;

    // rescale by subtracting minimum
    /****************************************************
    USE SSSE instruction phminpos!!!!!!!
    ****************************************************/
    min_state =vminq_u8(metrics0_31.val[0],metrics0_31.val[1]);
    min_state =vminq_u8(min_state,metrics32_63.val[0]);
    min_state =vminq_u8(min_state,metrics32_63.val[1]);
    // here we have 16 maximum metrics from the 64 states
    uint8x8_t min_state2 = vpmin_u8(((uint8x8_t*)&min_state)[0],((uint8x8_t*)&min_state)[0]);
    // now the 8 maximum in min_state2
    min_state2 = vpmin_u8(min_state2,min_state2);
    // now the 4 maximum in min_state2, repeated twice
    min_state2 = vpmin_u8(min_state2,min_state2);
    // now the 2 maximum in min_state2, repeated 4 times
    min_state2 = vpmin_u8(min_state2,min_state2);
    // now the 1 maximum in min_state2, repeated 8 times
    min_state  = vcombine_u8(min_state2,min_state2);
    // now the 1 maximum in min_state, repeated 16 times
    metrics0_31.val[0]  = vqsubq_u8(metrics0_31.val[0],min_state);
    metrics0_31.val[1]  = vqsubq_u8(metrics0_31.val[1],min_state);
    metrics32_63.val[0] = vqsubq_u8(metrics32_63.val[0],min_state);
    metrics32_63.val[1] = vqsubq_u8(metrics32_63.val[1],min_state);
#endif
    }

  } // iteration

  // Traceback
  prev_state0 = 0;
  maxm = 0;

#if defined(__x86_64__) || defined(__i386__)
  for (s=0; s<16; s++)
    if (((uint8_t *)&metrics0_15)[s] > maxm) {
      maxm = ((uint8_t *)&metrics0_15)[s];
      prev_state0 = s;
    }

  for (s=0; s<16; s++)
    if (((uint8_t *)&metrics16_31)[s] > maxm) {
      maxm = ((uint8_t *)&metrics16_31)[s];
      prev_state0 = s+16;
    }

  for (s=0; s<16; s++)
    if (((uint8_t *)&metrics32_47)[s] > maxm) {
      maxm = ((uint8_t *)&metrics32_47)[s];
      prev_state0 = s+32;
    }

  for (s=0; s<16; s++)
    if (((uint8_t *)&metrics48_63)[s] > maxm) {
      maxm = ((uint8_t *)&metrics48_63)[s];
      prev_state0 = s+48;
    }


#elif defined(__arm__)
  for (s=0; s<16; s++)
    if (((uint8_t *)&metrics0_31.val[0])[s] > maxm) {
      maxm = ((uint8_t *)&metrics0_31.val[0])[s];
      prev_state0 = s;
    }

  for (s=0; s<16; s++)
    if (((uint8_t *)&metrics0_31.val[1])[s] > maxm) {
      maxm = ((uint8_t *)&metrics0_31.val[1])[s];
      prev_state0 = s+16;
    }

  for (s=0; s<16; s++)
    if (((uint8_t *)&metrics32_63.val[0])[s] > maxm) {
      maxm = ((uint8_t *)&metrics32_63.val[0])[s];
      prev_state0 = s+32;
    }

  for (s=0; s<16; s++)
    if (((uint8_t *)&metrics32_63.val[1])[s] > maxm) {
      maxm = ((uint8_t *)&metrics32_63.val[1])[s];
      prev_state0 = s+48;
    }
#endif

  TB_ptr2 = (uint8_t *)&TB[(n-1)*4];

  for (position = n-1 ; position>-1; position--) {

    decoded_bytes[(position)>>3] += (prev_state0 & 0x1)<<(7-(position & 0x7));


    if (TB_ptr2[prev_state0] == 0)
      prev_state0 = (prev_state0 >> 1);
    else
      prev_state0 = 32 + (prev_state0>>1);

    TB_ptr2-=64;
  }


#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}

#ifdef TEST_DEBUG
int test_viterbi(uint8_t dabflag)
{
  uint8_t test[8];
  //_declspec(align(16))  int8_t channel_output[512];
  //_declspec(align(16))  uint8_t output[512],decoded_output[16], *inPtr, *outPtr;

  int8_t channel_output[512];
  uint8_t output[512],decoded_output[16], *inPtr, *outPtr;
  uint32_t i;


  test[0] = 7;
  test[1] = 0xa5;
  test[2] = 0;
  test[3] = 0xfe;
  test[4] = 0x1a;
  test[5] = 0x33;
  test[6] = 0x99;
  test[7] = 0x14;

  if (dabflag==0) {
    ccodelte_init();
    ccodelte_init_inv();
  } else {
    ccodedab_init();
    ccodedab_init_inv();
    printf("Running with DAB polynomials\n");
  }

  inPtr = test;
  outPtr = output;
  phy_generate_viterbi_tables_lte();
  ccodelte_encode(64,     //input length in bits
                  0,      // add 16-bit crc with rnti scrambling
                  inPtr,  // input pointer
                  outPtr, // output pointer
                  0);     // rnti (optional)

  for (i = 0; i < 64*3; i++) {
    channel_output[i] = 7*(2*output[i] - 1);
  }

  memset(decoded_output,0,16);
  phy_viterbi_lte_sse2(channel_output,decoded_output,64);
  printf("Optimized Viterbi :");

  for (i =0 ; i<8 ; i++)
    printf("input %d : %x => %x\n",i,inPtr[i],decoded_output[i]);
}




int main(int argc, char **argv)
{

  char c;
  uint8_t dabflag=0;

  while ((c = getopt (argc, argv, "d")) != -1) {
    if (c=='d')
      dabflag=1;
  }

  test_viterbi(dabflag);
  return(0);
}

#endif // TEST_DEBUG


