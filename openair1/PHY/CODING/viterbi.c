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

/* file: viterbi.c
   purpose: SIMD Optimized 802.11/802.16 Viterbi Decoder
   author: raymond.knopp@eurecom.fr
   date: 10.2004
*/



#include "PHY/sse_intrin.h"

extern unsigned char ccodedot11_table[128],ccodedot11_table_rev[128];




static unsigned char inputs[64][2048];
static unsigned short survivors[64][2048];
static short partial_metrics[64],partial_metrics_new[64];

void phy_viterbi_dot11(char *y,unsigned char *decoded_bytes,unsigned short n)
{

  /*  y is a pointer to the input
      decoded_bytes is a pointer to the decoded output
      n is the size in bits of the coded block, with the tail */


  char *in = y;
  short m0,m1,w[4],max_metric;
  short position;
  unsigned short prev_state0,prev_state1,state;

  partial_metrics[0] = 0;

  for (state=1; state<64; state++)
    partial_metrics[state] = -127;

  for (position=0; position<n; position++) {

    //    printf("Channel Output %d = (%d,%d)\n",position,*in,*(in+1));

    //        printf("%d %d\n",in[0],in[1]);

    w[3] = in[0] + in[1];  // 1,1
    w[0] = -w[3];          // -1,-1
    w[1] = in[0] - in[1];  // -1, 1
    w[2] = -w[1];          // 1 ,-1

    max_metric = -127;

    //    printf("w: %d %d %d %d\n",w[0],w[1],w[2],w[3]);
    for (state=0; state<64 ; state++) {

      // input 0
      prev_state0 = (state<<1);
      m0 = partial_metrics[prev_state0%64] + w[ccodedot11_table[prev_state0]];
      /*
      if (position < 8)
      printf("%d,%d : prev_state0 = %d,m0 = %d,w=%d (%d)\n",position,state,prev_state0%64,m0,w[ccodedot11_table[prev_state0]],partial_metrics[prev_state0%64]);
      */
      // input 1
      prev_state1 = (1+ (state<<1));
      m1 = partial_metrics[prev_state1%64] + w[ccodedot11_table[prev_state1]];

      /*
      if (position <8)
      printf("%d,%d : prev_state1 = %d,m1 = %d,w=%d (%d)\n",position,state,prev_state1%64,m1,w[ccodedot11_table[prev_state1]],partial_metrics[prev_state0%64]);
      */
      if (m0>m1) {
        partial_metrics_new[state] = m0;
        survivors[state][position] = prev_state0%64;
        inputs[state][position] = (state>31) ? 1 : 0;

        if (m0>max_metric)
          max_metric = m0;
      } else {
        partial_metrics_new[state] = m1;
        survivors[state][position] = prev_state1%64;
        inputs[state][position] = (state>31) ? 1 : 0;

        if (m1>max_metric)
          max_metric = m1;
      }

    }

    for (state=0 ; state<64; state++) {

      partial_metrics[state] = partial_metrics_new[state]- max_metric;
      //      printf("%d partial_metrics[%d] = %d\n",position,state,partial_metrics[state]);
    }

    in+=2;
  }


  // Traceback
  prev_state0 = 0;

  for (position = n-1 ; position>-1; position--) {

    decoded_bytes[(position)>>3] += (inputs[prev_state0][position]<<(position%8));

    //    if (position%8==0)
    //      printf("%d\n",decoded_bytes[(position)>>3]);


    prev_state0 = survivors[prev_state0][position];

  }


}



static char m0_table[64*256] __attribute__ ((aligned(16)));
static char m1_table[64*256] __attribute__ ((aligned(16)));


// Set up Viterbi tables for SSE2 implementation
void phy_generate_viterbi_tables(void)
{

  char w[4],in0,in1;
  unsigned char state,index0,index1;

  for (in0 = -8; in0 <8 ; in0++) {   // use 4-bit quantization
    for (in1 = -8; in1 <8 ; in1++) {

      w[3] = 16+ in0 + in1;  // 1,1
      w[0] = 16 - in0 - in1;          // -1,-1
      w[1] = 16+ in0 - in1;  // -1, 1
      w[2] = 16 - in0 + in1;          // 1 ,-1

      //    printf("w: %d %d %d %d\n",w[0],w[1],w[2],w[3]);
      for (state=0; state<64 ; state++) {

        // input 0
        index0 = (state<<1);
        m0_table[(in0+8 + (16*(in1+8)))*64 +state]  = w[ccodedot11_table_rev[index0]];


        //    if (position < 8)
        //    printf("%d,%d : prev_state0 = %d,m0 = %d,w=%d (%d)\n",position,state,prev_state0%64,m0,w[ccodedot11_table[prev_state0]],partial_metrics[prev_state0%64]);

        // input 1
        index1 = (1+ (state<<1));
        m1_table[(in0+8 + (16*(in1+8)))*64 +state] = w[ccodedot11_table_rev[index1]];

      }
    }
  }
}



#define INIT0 0x00000080



void phy_viterbi_dot11_sse2(char *y,unsigned char *decoded_bytes,unsigned short n,int offset, int traceback )
{

#if defined(__x86_64__) || defined(__i386__)
  __m128i  TB[4*4095*8]; // 4 __m128i per input bit (64 states, 8-bits per state = 16-way), 4095 is largest packet size in bytes, 8 bits/byte

  __m128i metrics0_15,metrics16_31,metrics32_47,metrics48_63,even0_30a,even0_30b,even32_62a,even32_62b,odd1_31a,odd1_31b,odd33_63a,odd33_63b,TBeven0_30,TBeven32_62,TBodd1_31,TBodd33_63;

  __m128i min_state,min_state2;


  __m128i *m0_ptr,*m1_ptr,*TB_ptr = &TB[offset<<2];

#elif defined(__arm__)
  uint8x16x2_t TB[2*4095*8];  // 2 int8x16_t per input bit, 8 bits / byte, 4095 is largest packet size in bytes

  uint8x16_t even0_30a,even0_30b,even32_62a,even32_62b,odd1_31a,odd1_31b,odd33_63a,odd33_63b,TBeven0_30,TBeven32_62,TBodd1_31,TBodd33_63;
  uint8x16x2_t metrics0_31,metrics32_63;

  uint8x16_t min_state;

  uint8x16_t *m0_ptr,*m1_ptr;
  uint8x16x2_t *TB_ptr = &TB[offset<<1];

#endif

  char *in = y;
  unsigned char prev_state0;
  unsigned char *TB_ptr2;
  unsigned short table_offset;

  short position;

  //  printf("offset %d, TB_ptr %p\n",offset,TB_ptr);
#if defined(__x86_64__) || defined(__i386__)
  if (offset == 0) {
    // set initial metrics

    metrics0_15 = _mm_cvtsi32_si128(INIT0);
    metrics16_31 = _mm_setzero_si128();
    metrics32_47 = _mm_setzero_si128();
    metrics48_63 = _mm_setzero_si128();
  }

#elif defined(__arm__)
  if (offset == 0) {
    // set initial metrics

    metrics0_31.val[0]  = vdupq_n_u8(0); metrics0_31.val[0] = vsetq_lane_u8(INIT0,metrics0_31.val[0],0);
    metrics0_31.val[1]  = vdupq_n_u8(0);
    metrics32_63.val[0] = vdupq_n_u8(0);
    metrics32_63.val[1] = vdupq_n_u8(0);
  }


#endif

  for (position=offset; position<(offset+n); position++) {

    //printf("%d : (%d,%d)\n",position,in[0],in[1]);

    // get branch metric offsets for the 64 states
    table_offset = (in[0]+8 + ((in[1]+8)<<4))<<6;

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

    TB_ptr[0] = _mm_unpacklo_epi8(TBeven0_30,TBodd1_31);
    TB_ptr[1] = _mm_unpackhi_epi8(TBeven0_30,TBodd1_31);
    TB_ptr[2] = _mm_unpacklo_epi8(TBeven32_62,TBodd33_63);
    TB_ptr[3] = _mm_unpackhi_epi8(TBeven32_62,TBodd33_63);

    in+=2;
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

  // Traceback
  if (traceback == 1) {
    prev_state0 = 0;
    TB_ptr2 = (unsigned char *)&TB[(offset+n-1)<<2];

    for (position = offset+n-1 ; position>-1; position--) {
      //   printf("pos %d: decoded %x\n",position>>3,decoded_bytes[position>>3]);
      decoded_bytes[(position)>>3] += (prev_state0 & 0x1)<<(position & 0x7);

      /*
      if ((position%8) == 0)
      printf("%d: %x\n",(position>>3),decoded_bytes[(position>>3)]);

      printf("pos %d : ps = %d -> %d\n",position,prev_state0,TB_ptr2[prev_state0]);
      */
      if (TB_ptr2[prev_state0] == 0)
        prev_state0 = (prev_state0 >> 1);
      else
        prev_state0 = 32 + (prev_state0>>1);

      TB_ptr2-=64;
    }
  }

#if defined(__x86_64) || defined(__i386__)
  _mm_empty();
#endif
}

#ifdef TEST_DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int test_viterbi()
{
  unsigned char test[8];
  //_declspec(align(16))  char channel_output[512];
  //_declspec(align(16))  unsigned char output[512],decoded_output[16], *inPtr, *outPtr;

  char channel_output[512];
  unsigned char output[512],decoded_output[16], *inPtr, *outPtr;
  unsigned int i;


  test[0] = 7;
  test[1] = 0xa5;
  test[2] = 0;
  test[3] = 0xfe;
  test[4] = 0x1a;
  test[5] = 0x33;
  test[6] = 0x99;
  test[7] = 0;


  ccodedot11_init();
  ccodedot11_init_inv();

  inPtr = test;
  outPtr = output;
  phy_generate_viterbi_tables();
  ccodedot11_encode(8, inPtr, outPtr,0);

  for (i = 0; i < 128; i++) {
    channel_output[i] = 7*(2*output[i] - 1);
  }

  memset(decoded_output,0,16);
  phy_viterbi_dot11(channel_output,decoded_output,64);

  printf("Input               :");

  for (i =0 ; i<8 ; i++)
    printf("%x ",inPtr[i]);

  printf("\n");

  printf("Unoptimized Viterbi :");

  for (i =0 ; i<8 ; i++)
    printf("%x ",decoded_output[i]);

  printf("\n");

  memset(decoded_output,0,16);
  phy_viterbi_dot11_sse2(channel_output,decoded_output,64);
  printf("Optimized Viterbi   :");

  for (i =0 ; i<8 ; i++)
    printf("%x ",decoded_output[i]);

  printf("\n");


  printf("\n");
}




int main()
{


  test_viterbi();
  return(0);
}

#endif // TEST_DEBUG


