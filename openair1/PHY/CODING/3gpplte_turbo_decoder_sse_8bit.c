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

/* file: 3gpplte_turbo_decoder_sse.c
   purpose: Routines for implementing max-logmap decoding of Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03
   authors: raymond.knopp@eurecom.fr, Laurent Thomas (Alcatel-Lucent)
   date: 21.10.2009

   Note: This routine currently requires SSE2,SSSE3 and SSE4.1 equipped computers.  It uses 16-bit inputs for LLRs and 8-bit arithmetic for internal computations!

   Changelog: 17.11.2009 FK SSE4.1 not required anymore
   Aug. 2012 new parallelization options for higher speed (8-way parallelization)
   Jan. 2013 8-bit LLR support with 16-way parallelization
   Feb. 2013 New interleaving and hard-decision optimizations (L. Thomas)
   May 2013 Extracted 8-bit code
*/

///
///

#include "PHY/sse_intrin.h"

#ifndef TEST_DEBUG
  #include "PHY/defs_common.h"
  #include "PHY/CODING/coding_defs.h"
  #include "PHY/CODING/lte_interleaver_inline.h"
#else

  #include "defs.h"
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
#endif

#ifdef MEX
  #include "mex.h"
#endif

#include "common/ran_context.h"

#define SHUFFLE16(a,b,c,d,e,f,g,h) _mm_set_epi8(h==-1?-1:h*2+1, \
    h==-1?-1:h*2, \
    g==-1?-1:g*2+1, \
    g==-1?-1:g*2, \
    f==-1?-1:f*2+1, \
    f==-1?-1:f*2, \
    e==-1?-1:e*2+1, \
    e==-1?-1:e*2, \
    d==-1?-1:d*2+1, \
    d==-1?-1:d*2, \
    c==-1?-1:c*2+1, \
    c==-1?-1:c*2, \
    b==-1?-1:b*2+1, \
    b==-1?-1:b*2, \
    a==-1?-1:a*2+1, \
    a==-1?-1:a*2);





//#define DEBUG_LOGMAP



typedef int8_t llr_t; // internal decoder LLR data is 8-bit fixed
typedef int8_t channel_t;
#define MAX8 127


void log_map8(llr_t *systematic,channel_t *y_parity, llr_t *m11, llr_t *m10, llr_t *alpha, llr_t *beta, llr_t *ext,unsigned short frame_length,unsigned char term_flag,unsigned char F,int offset8_flag,
              time_stats_t *alpha_stats,time_stats_t *beta_stats,time_stats_t *gamma_stats,time_stats_t *ext_stats);
void compute_gamma8(llr_t *m11,llr_t *m10,llr_t *systematic, channel_t *y_parity, unsigned short frame_length,unsigned char term_flag);
void compute_alpha8(llr_t *alpha,llr_t *beta, llr_t *m11,llr_t *m10, unsigned short frame_length,unsigned char F);
void compute_beta8(llr_t *alpha, llr_t *beta,llr_t *m11,llr_t *m10, unsigned short frame_length,unsigned char F,int offset8_flag);
void compute_ext8(llr_t *alpha,llr_t *beta,llr_t *m11,llr_t *m10,llr_t *extrinsic, llr_t *ap, unsigned short frame_length);


void print_bytes(char *s, int8_t *x) {
  printf("%s  : %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",s,
         x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7],
         x[8],x[9],x[10],x[11],x[12],x[13],x[14],x[15]);
}


void log_map8(llr_t *systematic,
              channel_t *y_parity,
              llr_t *m11,
              llr_t *m10,
              llr_t *alpha,
              llr_t *beta,
              llr_t *ext,
              unsigned short frame_length,
              unsigned char term_flag,
              unsigned char F,
              int offset8_flag,
              time_stats_t *alpha_stats,
              time_stats_t *beta_stats,
              time_stats_t *gamma_stats,
              time_stats_t *ext_stats) {
#ifdef DEBUG_LOGMAP
  printf("log_map, frame_length %d\n",frame_length);
#endif

  if (gamma_stats) start_meas(gamma_stats) ;

  compute_gamma8(m11,m10,systematic,y_parity,frame_length,term_flag) ;

  if (gamma_stats) stop_meas(gamma_stats);

  if (alpha_stats) start_meas(alpha_stats) ;

  compute_alpha8(alpha,beta,m11,m10,frame_length,F)                  ;

  if (alpha_stats) stop_meas(alpha_stats);

  if (beta_stats) start_meas(beta_stats)  ;

  compute_beta8(alpha,beta,m11,m10,frame_length,F,offset8_flag)      ;

  if (beta_stats) stop_meas(beta_stats);

  if (ext_stats) start_meas(ext_stats)   ;

  compute_ext8(alpha,beta,m11,m10,ext,systematic,frame_length)       ;

  if (ext_stats) stop_meas(ext_stats);
}

void compute_gamma8(llr_t *m11,llr_t *m10,llr_t *systematic,channel_t *y_parity,
                    unsigned short frame_length,unsigned char term_flag) {
  int k,K1;
#if defined(__x86_64__)||defined(__i386__)
  __m128i *systematic128 = (__m128i *)systematic;
  __m128i *y_parity128   = (__m128i *)y_parity;
  __m128i *m10_128        = (__m128i *)m10;
  __m128i *m11_128        = (__m128i *)m11;
#elif defined(__arm__)
  int8x16_t *systematic128  = (int8x16_t *)systematic;
  int8x16_t *y_parity128    = (int8x16_t *)y_parity;
  int8x16_t *m10_128        = (int8x16_t *)m10;
  int8x16_t *m11_128        = (int8x16_t *)m11;
#endif
#ifdef DEBUG_LOGMAP
  printf("compute_gamma, %p,%p,%p,%p,framelength %d\n",m11,m10,systematic,y_parity,frame_length);
#endif
#if defined(__x86_64__) || defined(__i386__)
  register __m128i sl,sh,ypl,yph; //K128=_mm_set1_epi8(-128);
#endif
  K1 = (frame_length>>4);

  for (k=0; k<K1; k++) {
#if defined(__x86_64__) || defined(__i386__)
    sl  = _mm_cvtepi8_epi16(systematic128[k]);
    sh  = _mm_cvtepi8_epi16(_mm_srli_si128(systematic128[k],8));
    ypl = _mm_cvtepi8_epi16(y_parity128[k]);
    yph = _mm_cvtepi8_epi16(_mm_srli_si128(y_parity128[k],8));
    m11_128[k] = _mm_packs_epi16(_mm_srai_epi16(_mm_adds_epi16(sl,ypl),1),
                                 _mm_srai_epi16(_mm_adds_epi16(sh,yph),1));
    m10_128[k] = _mm_packs_epi16(_mm_srai_epi16(_mm_subs_epi16(sl,ypl),1),
                                 _mm_srai_epi16(_mm_subs_epi16(sh,yph),1));
#elif defined(__arm__)
    m11_128[k] = vhaddq_s8(systematic128[k],y_parity128[k]);
    m10_128[k] = vhsubq_s8(systematic128[k],y_parity128[k]);
#endif
  }

  // Termination
#if defined(__x86_64__) || defined(__i386__)
  sl  = _mm_cvtepi8_epi16(systematic128[k+term_flag]);
  sh = _mm_cvtepi8_epi16(_mm_srli_si128(systematic128[k],8));
  ypl = _mm_cvtepi8_epi16(y_parity128[k+term_flag]);
  yph = _mm_cvtepi8_epi16(_mm_srli_si128(y_parity128[k],8));
  m11_128[k] = _mm_packs_epi16(_mm_srai_epi16(_mm_adds_epi16(sl,ypl),1),
                               _mm_srai_epi16(_mm_adds_epi16(sh,yph),1));
  m10_128[k] = _mm_packs_epi16(_mm_srai_epi16(_mm_subs_epi16(sl,ypl),1),
                               _mm_srai_epi16(_mm_subs_epi16(sh,yph),1));
#elif defined(__arm__)
  m11_128[k] = vhaddq_s8(systematic128[k+term_flag],y_parity128[k]);
  m10_128[k] = vhsubq_s8(systematic128[k+term_flag],y_parity128[k]);
#endif
}

#define L 16

void compute_alpha8(llr_t *alpha,llr_t *beta,llr_t *m_11,llr_t *m_10,unsigned short frame_length,unsigned char F) {
  int k,loopval,rerun_flag;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *alpha128=(__m128i *)alpha,*alpha_ptr;
  __m128i *m11p,*m10p;
  __m128i m_b0,m_b1,m_b2,m_b3,m_b4,m_b5,m_b6,m_b7;
  __m128i new0,new1,new2,new3,new4,new5,new6,new7;
  __m128i alpha_max;
#elif defined(__arm__)
  int8x16_t *alpha128=(int8x16_t *)alpha,*alpha_ptr;
  int8x16_t *m11p,*m10p;
  int8x16_t m_b0,m_b1,m_b2,m_b3,m_b4,m_b5,m_b6,m_b7;
  int8x16_t new0,new1,new2,new3,new4,new5,new6,new7;
  int8x16_t alpha_max;
#endif
  // Set initial state: first colum is known
  // the other columns are unknown, so all states are set to same value
#if defined(__x86_64__) || defined(__i386__)
  alpha128[0] = _mm_set_epi8(-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,0);
  alpha128[1] = _mm_set_epi8(-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2);
  alpha128[2] = _mm_set_epi8(-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2);
  alpha128[3] = _mm_set_epi8(-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2);
  alpha128[4] = _mm_set_epi8(-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2);
  alpha128[5] = _mm_set_epi8(-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2);
  alpha128[6] = _mm_set_epi8(-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2);
  alpha128[7] = _mm_set_epi8(-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2,-MAX8/2);

  for (loopval=frame_length>>4, rerun_flag=0; rerun_flag<2; loopval=L, rerun_flag++) {
    alpha_ptr = &alpha128[0];
    m11p = (__m128i *)m_11;
    m10p = (__m128i *)m_10;

    for (k=0;  k<loopval;  k++) {
      m_b0 = _mm_adds_epi8(alpha_ptr[1],*m11p);  // m11
      m_b4 = _mm_subs_epi8(alpha_ptr[1],*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi8(alpha_ptr[3],*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi8(alpha_ptr[3],*m10p);  // m10
      m_b2 = _mm_adds_epi8(alpha_ptr[5],*m10p);  // m10
      m_b6 = _mm_subs_epi8(alpha_ptr[5],*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi8(alpha_ptr[7],*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi8(alpha_ptr[7],*m11p);  // m11
      new0 = _mm_subs_epi8(alpha_ptr[0],*m11p);  // m00=-m11
      new4 = _mm_adds_epi8(alpha_ptr[0],*m11p);  // m11
      new1 = _mm_adds_epi8(alpha_ptr[2],*m10p);  // m10
      new5 = _mm_subs_epi8(alpha_ptr[2],*m10p);  // m01=-m10
      new2 = _mm_subs_epi8(alpha_ptr[4],*m10p);  // m01=-m10
      new6 = _mm_adds_epi8(alpha_ptr[4],*m10p);  // m10
      new3 = _mm_adds_epi8(alpha_ptr[6],*m11p);  // m11
      new7 = _mm_subs_epi8(alpha_ptr[6],*m11p);  // m00=-m11
      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = _mm_max_epi8(m_b0,new0);
      alpha_ptr[1] = _mm_max_epi8(m_b1,new1);
      alpha_ptr[2] = _mm_max_epi8(m_b2,new2);
      alpha_ptr[3] = _mm_max_epi8(m_b3,new3);
      alpha_ptr[4] = _mm_max_epi8(m_b4,new4);
      alpha_ptr[5] = _mm_max_epi8(m_b5,new5);
      alpha_ptr[6] = _mm_max_epi8(m_b6,new6);
      alpha_ptr[7] = _mm_max_epi8(m_b7,new7);
      // compute and subtract maxima
      alpha_max = _mm_max_epi8(alpha_ptr[0],alpha_ptr[1]);
      alpha_max = _mm_max_epi8(alpha_max,alpha_ptr[2]);
      alpha_max = _mm_max_epi8(alpha_max,alpha_ptr[3]);
      alpha_max = _mm_max_epi8(alpha_max,alpha_ptr[4]);
      alpha_max = _mm_max_epi8(alpha_max,alpha_ptr[5]);
      alpha_max = _mm_max_epi8(alpha_max,alpha_ptr[6]);
      alpha_max = _mm_max_epi8(alpha_max,alpha_ptr[7]);
      alpha_ptr[0] = _mm_subs_epi8(alpha_ptr[0],alpha_max);
      alpha_ptr[1] = _mm_subs_epi8(alpha_ptr[1],alpha_max);
      alpha_ptr[2] = _mm_subs_epi8(alpha_ptr[2],alpha_max);
      alpha_ptr[3] = _mm_subs_epi8(alpha_ptr[3],alpha_max);
      alpha_ptr[4] = _mm_subs_epi8(alpha_ptr[4],alpha_max);
      alpha_ptr[5] = _mm_subs_epi8(alpha_ptr[5],alpha_max);
      alpha_ptr[6] = _mm_subs_epi8(alpha_ptr[6],alpha_max);
      alpha_ptr[7] = _mm_subs_epi8(alpha_ptr[7],alpha_max);
    }

    // Set intial state for next iteration from the last state
    // as acolum end states are the first states of the next column
    int K1= frame_length>>1;
    alpha128[0] = _mm_slli_si128(alpha128[K1],1);
    alpha128[1] = _mm_slli_si128(alpha128[1+K1],1);
    alpha128[2] = _mm_slli_si128(alpha128[2+K1],1);
    alpha128[3] = _mm_slli_si128(alpha128[3+K1],1);
    alpha128[4] = _mm_slli_si128(alpha128[4+K1],1);
    alpha128[5] = _mm_slli_si128(alpha128[5+K1],1);
    alpha128[6] = _mm_slli_si128(alpha128[6+K1],1);
    alpha128[7] = _mm_slli_si128(alpha128[7+K1],1);
    alpha[16] =  -MAX8/2;
    alpha[32] = -MAX8/2;
    alpha[48] = -MAX8/2;
    alpha[64] = -MAX8/2;
    alpha[80] = -MAX8/2;
    alpha[96] = -MAX8/2;
    alpha[112] = -MAX8/2;
  }

#elif defined(__arm__)
  alpha128[0] = vdupq_n_s8(-MAX8/2);
  alpha128[0] = vsetq_lane_s8(0,alpha128[0],0);
  alpha128[1] = vdupq_n_s8(-MAX8/2);
  alpha128[2] = vdupq_n_s8(-MAX8/2);
  alpha128[3] = vdupq_n_s8(-MAX8/2);
  alpha128[4] = vdupq_n_s8(-MAX8/2);
  alpha128[5] = vdupq_n_s8(-MAX8/2);
  alpha128[6] = vdupq_n_s8(-MAX8/2);
  alpha128[7] = vdupq_n_s8(-MAX8/2);

  for (loopval=frame_length>>4, rerun_flag=0; rerun_flag<2; loopval=L, rerun_flag++) {
    alpha_ptr = &alpha128[0];
    m11p = (int8x16_t *)m_11;
    m10p = (int8x16_t *)m_10;

    for (k=0;  k<loopval;  k++) {
      m_b0 = vqaddq_s8(alpha_ptr[1],*m11p);  // m11
      m_b4 = vqsubq_s8(alpha_ptr[1],*m11p);  // m00=-m11
      m_b1 = vqsubq_s8(alpha_ptr[3],*m10p);  // m01=-m10
      m_b5 = vqaddq_s8(alpha_ptr[3],*m10p);  // m10
      m_b2 = vqaddq_s8(alpha_ptr[5],*m10p);  // m10
      m_b6 = vqsubq_s8(alpha_ptr[5],*m10p);  // m01=-m10
      m_b3 = vqsubq_s8(alpha_ptr[7],*m11p);  // m00=-m11
      m_b7 = vqaddq_s8(alpha_ptr[7],*m11p);  // m11
      new0 = vqsubq_s8(alpha_ptr[0],*m11p);  // m00=-m11
      new4 = vqaddq_s8(alpha_ptr[0],*m11p);  // m11
      new1 = vqaddq_s8(alpha_ptr[2],*m10p);  // m10
      new5 = vqsubq_s8(alpha_ptr[2],*m10p);  // m01=-m10
      new2 = vqsubq_s8(alpha_ptr[4],*m10p);  // m01=-m10
      new6 = vqaddq_s8(alpha_ptr[4],*m10p);  // m10
      new3 = vqaddq_s8(alpha_ptr[6],*m11p);  // m11
      new7 = vqsubq_s8(alpha_ptr[6],*m11p);  // m00=-m11
      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = vmaxq_s8(m_b0,new0);
      alpha_ptr[1] = vmaxq_s8(m_b1,new1);
      alpha_ptr[2] = vmaxq_s8(m_b2,new2);
      alpha_ptr[3] = vmaxq_s8(m_b3,new3);
      alpha_ptr[4] = vmaxq_s8(m_b4,new4);
      alpha_ptr[5] = vmaxq_s8(m_b5,new5);
      alpha_ptr[6] = vmaxq_s8(m_b6,new6);
      alpha_ptr[7] = vmaxq_s8(m_b7,new7);
      // compute and subtract maxima
      alpha_max = vmaxq_s8(alpha_ptr[0],alpha_ptr[1]);
      alpha_max = vmaxq_s8(alpha_max,alpha_ptr[2]);
      alpha_max = vmaxq_s8(alpha_max,alpha_ptr[3]);
      alpha_max = vmaxq_s8(alpha_max,alpha_ptr[4]);
      alpha_max = vmaxq_s8(alpha_max,alpha_ptr[5]);
      alpha_max = vmaxq_s8(alpha_max,alpha_ptr[6]);
      alpha_max = vmaxq_s8(alpha_max,alpha_ptr[7]);
      alpha_ptr[0] = vqsubq_s8(alpha_ptr[0],alpha_max);
      alpha_ptr[1] = vqsubq_s8(alpha_ptr[1],alpha_max);
      alpha_ptr[2] = vqsubq_s8(alpha_ptr[2],alpha_max);
      alpha_ptr[3] = vqsubq_s8(alpha_ptr[3],alpha_max);
      alpha_ptr[4] = vqsubq_s8(alpha_ptr[4],alpha_max);
      alpha_ptr[5] = vqsubq_s8(alpha_ptr[5],alpha_max);
      alpha_ptr[6] = vqsubq_s8(alpha_ptr[6],alpha_max);
      alpha_ptr[7] = vqsubq_s8(alpha_ptr[7],alpha_max);
    }

    // Set intial state for next iteration from the last state
    // as a column end states are the first states of the next column
    int K1= frame_length>>1;
    alpha128[0] = (int8x16_t)vshlq_n_s64((int64x2_t)alpha128[K1],8);
    alpha128[0] = vsetq_lane_s8(alpha[8],alpha128[0],7);
    alpha128[1] = (int8x16_t)vshlq_n_s64((int64x2_t)alpha128[1+K1],8);
    alpha128[1] = vsetq_lane_s8(alpha[24],alpha128[0],7);
    alpha128[2] = (int8x16_t)vshlq_n_s64((int64x2_t)alpha128[2+K1],8);
    alpha128[2] = vsetq_lane_s8(alpha[40],alpha128[0],7);
    alpha128[3] = (int8x16_t)vshlq_n_s64((int64x2_t)alpha128[3+K1],8);
    alpha128[3] = vsetq_lane_s8(alpha[56],alpha128[0],7);
    alpha128[4] = (int8x16_t)vshlq_n_s64((int64x2_t)alpha128[4+K1],8);
    alpha128[4] = vsetq_lane_s8(alpha[72],alpha128[0],7);
    alpha128[5] = (int8x16_t)vshlq_n_s64((int64x2_t)alpha128[5+K1],8);
    alpha128[5] = vsetq_lane_s8(alpha[88],alpha128[0],7);
    alpha128[6] = (int8x16_t)vshlq_n_s64((int64x2_t)alpha128[6+K1],8);
    alpha128[6] = vsetq_lane_s8(alpha[104],alpha128[0],7);
    alpha128[7] = (int8x16_t)vshlq_n_s64((int64x2_t)alpha128[7+K1],8);
    alpha128[7] = vsetq_lane_s8(alpha[120],alpha128[0],7);
    alpha[16] =  -MAX8/2;
    alpha[32] = -MAX8/2;
    alpha[48] = -MAX8/2;
    alpha[64] = -MAX8/2;
    alpha[80] = -MAX8/2;
    alpha[96] = -MAX8/2;
    alpha[112] = -MAX8/2;
  }

#endif
}


void compute_beta8(llr_t *alpha,llr_t *beta,llr_t *m_11,llr_t *m_10,unsigned short frame_length,unsigned char F,int offset8_flag) {
  int k,rerun_flag, loopval;
#if defined(__x86_64__) || defined(__i386__)
  __m128i m11_128,m10_128;
  __m128i m_b0,m_b1,m_b2,m_b3,m_b4,m_b5,m_b6,m_b7;
  __m128i new0,new1,new2,new3,new4,new5,new6,new7;
  __m128i *beta128,*alpha128,*beta_ptr;
  __m128i beta_max;
#elif defined(__arm__)
  int8x16_t m11_128,m10_128;
  int8x16_t m_b0,m_b1,m_b2,m_b3,m_b4,m_b5,m_b6,m_b7;
  int8x16_t new0,new1,new2,new3,new4,new5,new6,new7;
  int8x16_t *beta128,*alpha128,*beta_ptr;
  int8x16_t beta_max;
#endif
  llr_t beta0,beta1;
  llr_t beta2,beta3,beta4,beta5,beta6,beta7;

  if (frame_length > 6144) {
    LOG_E(PHY,"compute_beta: frame_length %d\n",frame_length);
    return;
  }

  // we are supposed to run compute_alpha just before compute_beta
  // so the initial states of backward computation can be set from last value of alpha states (forward computation)
#if defined(__x86_64__) || defined(__i386__)
  beta_ptr   = (__m128i *)&beta[frame_length<<3];
  alpha128   = (__m128i *)&alpha[0];
#elif defined(__arm__)
  beta_ptr   = (int8x16_t *)&beta[frame_length<<3];
  alpha128   = (int8x16_t *)&alpha[0];
#endif
  beta_ptr[0] = alpha128[(frame_length>>1)];
  beta_ptr[1] = alpha128[1+(frame_length>>1)];
  beta_ptr[2] = alpha128[2+(frame_length>>1)];
  beta_ptr[3] = alpha128[3+(frame_length>>1)];
  beta_ptr[4] = alpha128[4+(frame_length>>1)];
  beta_ptr[5] = alpha128[5+(frame_length>>1)];
  beta_ptr[6] = alpha128[6+(frame_length>>1)];
  beta_ptr[7] = alpha128[7+(frame_length>>1)];
  int overlap = (frame_length>>4)> L ? (frame_length>>4)-L : 0 ;

  for (rerun_flag=0, loopval=0;
       rerun_flag<2 ;
       loopval=overlap,rerun_flag++) {
    if (offset8_flag==0) {
      // FIXME! beta0-beta7 are used uninitialized. FIXME!
      // workaround: init with 0
      beta0 = beta1 = beta2 = beta3 = beta4 = beta5 = beta6 = beta7 = 0;
#if defined(__x86_64__) || defined(__i386__)
      beta_ptr[0] = _mm_insert_epi8(beta_ptr[0],beta0,15);
      beta_ptr[1] = _mm_insert_epi8(beta_ptr[1],beta1,15);
      beta_ptr[2] = _mm_insert_epi8(beta_ptr[2],beta2,15);
      beta_ptr[3] = _mm_insert_epi8(beta_ptr[3],beta3,15);
      beta_ptr[4] = _mm_insert_epi8(beta_ptr[4],beta4,15);
      beta_ptr[5] = _mm_insert_epi8(beta_ptr[5],beta5,15);
      beta_ptr[6] = _mm_insert_epi8(beta_ptr[6],beta6,15);
      beta_ptr[7] = _mm_insert_epi8(beta_ptr[7],beta7,15);
#elif defined(__arm__)
      beta_ptr[0] = vsetq_lane_s8(beta0,beta_ptr[0],15);
      beta_ptr[1] = vsetq_lane_s8(beta1,beta_ptr[1],15);
      beta_ptr[2] = vsetq_lane_s8(beta2,beta_ptr[2],15);
      beta_ptr[3] = vsetq_lane_s8(beta3,beta_ptr[3],15);
      beta_ptr[4] = vsetq_lane_s8(beta4,beta_ptr[4],15);
      beta_ptr[5] = vsetq_lane_s8(beta5,beta_ptr[5],15);
      beta_ptr[6] = vsetq_lane_s8(beta6,beta_ptr[6],15);
      beta_ptr[7] = vsetq_lane_s8(beta7,beta_ptr[7],15);
#endif
    }

#if defined(__x86_64__) || defined(__i386__)
    beta_ptr = (__m128i *)&beta[frame_length<<3];
#elif defined(__arm__)
    beta_ptr = (int8x16_t *)&beta[frame_length<<3];
#endif

    for (k=(frame_length>>4)-1;
         k>=loopval;
         k--) {
#if defined(__x86_64__) || defined(__i386__)
      m11_128=((__m128i *)m_11)[k];
      m10_128=((__m128i *)m_10)[k];
      m_b0 = _mm_adds_epi8(beta_ptr[4],m11_128);  //m11
      m_b1 = _mm_subs_epi8(beta_ptr[4],m11_128);  //m00
      m_b2 = _mm_subs_epi8(beta_ptr[5],m10_128);  //m01
      m_b3 = _mm_adds_epi8(beta_ptr[5],m10_128);  //m10
      m_b4 = _mm_adds_epi8(beta_ptr[6],m10_128);  //m10
      m_b5 = _mm_subs_epi8(beta_ptr[6],m10_128);  //m01
      m_b6 = _mm_subs_epi8(beta_ptr[7],m11_128);  //m00
      m_b7 = _mm_adds_epi8(beta_ptr[7],m11_128);  //m11
      new0 = _mm_subs_epi8(beta_ptr[0],m11_128);  //m00
      new1 = _mm_adds_epi8(beta_ptr[0],m11_128);  //m11
      new2 = _mm_adds_epi8(beta_ptr[1],m10_128);  //m10
      new3 = _mm_subs_epi8(beta_ptr[1],m10_128);  //m01
      new4 = _mm_subs_epi8(beta_ptr[2],m10_128);  //m01
      new5 = _mm_adds_epi8(beta_ptr[2],m10_128);  //m10
      new6 = _mm_adds_epi8(beta_ptr[3],m11_128);  //m11
      new7 = _mm_subs_epi8(beta_ptr[3],m11_128);  //m00
      beta_ptr-=8;
      beta_ptr[0] = _mm_max_epi8(m_b0,new0);
      beta_ptr[1] = _mm_max_epi8(m_b1,new1);
      beta_ptr[2] = _mm_max_epi8(m_b2,new2);
      beta_ptr[3] = _mm_max_epi8(m_b3,new3);
      beta_ptr[4] = _mm_max_epi8(m_b4,new4);
      beta_ptr[5] = _mm_max_epi8(m_b5,new5);
      beta_ptr[6] = _mm_max_epi8(m_b6,new6);
      beta_ptr[7] = _mm_max_epi8(m_b7,new7);
      beta_max = _mm_max_epi8(beta_ptr[0],beta_ptr[1]);
      beta_max = _mm_max_epi8(beta_max   ,beta_ptr[2]);
      beta_max = _mm_max_epi8(beta_max   ,beta_ptr[3]);
      beta_max = _mm_max_epi8(beta_max   ,beta_ptr[4]);
      beta_max = _mm_max_epi8(beta_max   ,beta_ptr[5]);
      beta_max = _mm_max_epi8(beta_max   ,beta_ptr[6]);
      beta_max = _mm_max_epi8(beta_max   ,beta_ptr[7]);
      beta_ptr[0] = _mm_subs_epi8(beta_ptr[0],beta_max);
      beta_ptr[1] = _mm_subs_epi8(beta_ptr[1],beta_max);
      beta_ptr[2] = _mm_subs_epi8(beta_ptr[2],beta_max);
      beta_ptr[3] = _mm_subs_epi8(beta_ptr[3],beta_max);
      beta_ptr[4] = _mm_subs_epi8(beta_ptr[4],beta_max);
      beta_ptr[5] = _mm_subs_epi8(beta_ptr[5],beta_max);
      beta_ptr[6] = _mm_subs_epi8(beta_ptr[6],beta_max);
      beta_ptr[7] = _mm_subs_epi8(beta_ptr[7],beta_max);
#elif defined(__arm__)
      m11_128=((int8x16_t *)m_11)[k];
      m10_128=((int8x16_t *)m_10)[k];
      m_b0 = vqaddq_s8(beta_ptr[4],m11_128);  //m11
      m_b1 = vqsubq_s8(beta_ptr[4],m11_128);  //m00
      m_b2 = vqsubq_s8(beta_ptr[5],m10_128);  //m01
      m_b3 = vqaddq_s8(beta_ptr[5],m10_128);  //m10
      m_b4 = vqaddq_s8(beta_ptr[6],m10_128);  //m10
      m_b5 = vqsubq_s8(beta_ptr[6],m10_128);  //m01
      m_b6 = vqsubq_s8(beta_ptr[7],m11_128);  //m00
      m_b7 = vqaddq_s8(beta_ptr[7],m11_128);  //m11
      new0 = vqsubq_s8(beta_ptr[0],m11_128);  //m00
      new1 = vqaddq_s8(beta_ptr[0],m11_128);  //m11
      new2 = vqaddq_s8(beta_ptr[1],m10_128);  //m10
      new3 = vqsubq_s8(beta_ptr[1],m10_128);  //m01
      new4 = vqsubq_s8(beta_ptr[2],m10_128);  //m01
      new5 = vqaddq_s8(beta_ptr[2],m10_128);  //m10
      new6 = vqaddq_s8(beta_ptr[3],m11_128);  //m11
      new7 = vqsubq_s8(beta_ptr[3],m11_128);  //m00
      beta_ptr-=8;
      beta_ptr[0] = vmaxq_s8(m_b0,new0);
      beta_ptr[1] = vmaxq_s8(m_b1,new1);
      beta_ptr[2] = vmaxq_s8(m_b2,new2);
      beta_ptr[3] = vmaxq_s8(m_b3,new3);
      beta_ptr[4] = vmaxq_s8(m_b4,new4);
      beta_ptr[5] = vmaxq_s8(m_b5,new5);
      beta_ptr[6] = vmaxq_s8(m_b6,new6);
      beta_ptr[7] = vmaxq_s8(m_b7,new7);
      beta_max = vmaxq_s8(beta_ptr[0],beta_ptr[1]);
      beta_max = vmaxq_s8(beta_max   ,beta_ptr[2]);
      beta_max = vmaxq_s8(beta_max   ,beta_ptr[3]);
      beta_max = vmaxq_s8(beta_max   ,beta_ptr[4]);
      beta_max = vmaxq_s8(beta_max   ,beta_ptr[5]);
      beta_max = vmaxq_s8(beta_max   ,beta_ptr[6]);
      beta_max = vmaxq_s8(beta_max   ,beta_ptr[7]);
      beta_ptr[0] = vqsubq_s8(beta_ptr[0],beta_max);
      beta_ptr[1] = vqsubq_s8(beta_ptr[1],beta_max);
      beta_ptr[2] = vqsubq_s8(beta_ptr[2],beta_max);
      beta_ptr[3] = vqsubq_s8(beta_ptr[3],beta_max);
      beta_ptr[4] = vqsubq_s8(beta_ptr[4],beta_max);
      beta_ptr[5] = vqsubq_s8(beta_ptr[5],beta_max);
      beta_ptr[6] = vqsubq_s8(beta_ptr[6],beta_max);
      beta_ptr[7] = vqsubq_s8(beta_ptr[7],beta_max);
#endif
    }

    // Set intial state for next iteration from the last state
    // as column last states are the first states of the next column
    // The initial state of column 0 is coming from tail bits (to be computed)
#if defined(__x86_64__) || defined(__i386__)
    beta128 = (__m128i *)&beta[0];
    beta_ptr   = (__m128i *)&beta[frame_length<<3];
    beta_ptr[0] = _mm_srli_si128(beta128[0],1);
    beta_ptr[1] = _mm_srli_si128(beta128[1],1);
    beta_ptr[2] = _mm_srli_si128(beta128[2],1);
    beta_ptr[3] = _mm_srli_si128(beta128[3],1);
    beta_ptr[4] = _mm_srli_si128(beta128[4],1);
    beta_ptr[5] = _mm_srli_si128(beta128[5],1);
    beta_ptr[6] = _mm_srli_si128(beta128[6],1);
    beta_ptr[7] = _mm_srli_si128(beta128[7],1);
#elif defined(__arm__)
    beta128 = (int8x16_t *)&beta[0];
    beta_ptr   = (int8x16_t *)&beta[frame_length<<3];
    beta_ptr[0] = (int8x16_t)vshrq_n_s64((int64x2_t)beta128[0],8);
    beta_ptr[0] = vsetq_lane_s8(beta[7],beta_ptr[0],8);
    beta_ptr[1] = (int8x16_t)vshrq_n_s64((int64x2_t)beta128[1],8);
    beta_ptr[1] = vsetq_lane_s8(beta[23],beta_ptr[1],8);
    beta_ptr[2] = (int8x16_t)vshrq_n_s64((int64x2_t)beta128[2],8);
    beta_ptr[2] = vsetq_lane_s8(beta[39],beta_ptr[2],8);
    beta_ptr[3] = (int8x16_t)vshrq_n_s64((int64x2_t)beta128[3],8);
    beta_ptr[3] = vsetq_lane_s8(beta[55],beta_ptr[3],8);
    beta_ptr[4] = (int8x16_t)vshrq_n_s64((int64x2_t)beta128[4],8);
    beta_ptr[4] = vsetq_lane_s8(beta[71],beta_ptr[4],8);
    beta_ptr[5] = (int8x16_t)vshrq_n_s64((int64x2_t)beta128[5],8);
    beta_ptr[5] = vsetq_lane_s8(beta[87],beta_ptr[5],8);
    beta_ptr[6] = (int8x16_t)vshrq_n_s64((int64x2_t)beta128[6],8);
    beta_ptr[6] = vsetq_lane_s8(beta[103],beta_ptr[6],8);
    beta_ptr[7] = (int8x16_t)vshrq_n_s64((int64x2_t)beta128[7],8);
    beta_ptr[7] = vsetq_lane_s8(beta[119],beta_ptr[7],8);
#endif
  }
}

void compute_ext8(llr_t *alpha,llr_t *beta,llr_t *m_11,llr_t *m_10,llr_t *ext, llr_t *systematic,unsigned short frame_length) {
#if defined(__x86_64__) || defined(__i386__)
  __m128i *alpha128=(__m128i *)alpha;
  __m128i *beta128=(__m128i *)beta;
  __m128i *m11_128,*m10_128,*ext_128;
  __m128i *alpha_ptr,*beta_ptr;
  __m128i m00_1,m00_2,m00_3,m00_4;
  __m128i m01_1,m01_2,m01_3,m01_4;
  __m128i m10_1,m10_2,m10_3,m10_4;
  __m128i m11_1,m11_2,m11_3,m11_4;
#elif defined(__arm__)
  int8x16_t *alpha128=(int8x16_t *)alpha;
  int8x16_t *beta128=(int8x16_t *)beta;
  int8x16_t *m11_128,*m10_128,*ext_128;
  int8x16_t *alpha_ptr,*beta_ptr;
  int8x16_t m00_1,m00_2,m00_3,m00_4;
  int8x16_t m01_1,m01_2,m01_3,m01_4;
  int8x16_t m10_1,m10_2,m10_3,m10_4;
  int8x16_t m11_1,m11_2,m11_3,m11_4;
#endif
  int k;
  //
  // LLR computation, 8 consequtive bits per loop
  //
#ifdef DEBUG_LOGMAP
  printf("compute_ext, %p, %p, %p, %p, %p, %p ,framelength %d\n",alpha,beta,m_11,m_10,ext,systematic,frame_length);
#endif
  alpha_ptr = alpha128;
  beta_ptr = &beta128[8];

  for (k=0; k<(frame_length>>4); k++) {
#if defined(__x86_64__) || defined(__i386__)
    m11_128        = (__m128i *)&m_11[k<<4];
    m10_128        = (__m128i *)&m_10[k<<4];
    ext_128        = (__m128i *)&ext[k<<4];
    m00_4 = _mm_adds_epi8(alpha_ptr[7],beta_ptr[3]); //ALPHA_BETA_4m00;
    m11_4 = _mm_adds_epi8(alpha_ptr[7],beta_ptr[7]); //ALPHA_BETA_4m11;
    m00_3 = _mm_adds_epi8(alpha_ptr[6],beta_ptr[7]); //ALPHA_BETA_3m00;
    m11_3 = _mm_adds_epi8(alpha_ptr[6],beta_ptr[3]); //ALPHA_BETA_3m11;
    m00_2 = _mm_adds_epi8(alpha_ptr[1],beta_ptr[4]); //ALPHA_BETA_2m00;
    m11_2 = _mm_adds_epi8(alpha_ptr[1],beta_ptr[0]); //ALPHA_BETA_2m11;
    m11_1 = _mm_adds_epi8(alpha_ptr[0],beta_ptr[4]); //ALPHA_BETA_1m11;
    m00_1 = _mm_adds_epi8(alpha_ptr[0],beta_ptr[0]); //ALPHA_BETA_1m00;
    m01_4 = _mm_adds_epi8(alpha_ptr[5],beta_ptr[6]); //ALPHA_BETA_4m01;
    m10_4 = _mm_adds_epi8(alpha_ptr[5],beta_ptr[2]); //ALPHA_BETA_4m10;
    m01_3 = _mm_adds_epi8(alpha_ptr[4],beta_ptr[2]); //ALPHA_BETA_3m01;
    m10_3 = _mm_adds_epi8(alpha_ptr[4],beta_ptr[6]); //ALPHA_BETA_3m10;
    m01_2 = _mm_adds_epi8(alpha_ptr[3],beta_ptr[1]); //ALPHA_BETA_2m01;
    m10_2 = _mm_adds_epi8(alpha_ptr[3],beta_ptr[5]); //ALPHA_BETA_2m10;
    m10_1 = _mm_adds_epi8(alpha_ptr[2],beta_ptr[1]); //ALPHA_BETA_1m10;
    m01_1 = _mm_adds_epi8(alpha_ptr[2],beta_ptr[5]); //ALPHA_BETA_1m01;
    m01_1 = _mm_max_epi8(m01_1,m01_2);
    m01_1 = _mm_max_epi8(m01_1,m01_3);
    m01_1 = _mm_max_epi8(m01_1,m01_4);
    m00_1 = _mm_max_epi8(m00_1,m00_2);
    m00_1 = _mm_max_epi8(m00_1,m00_3);
    m00_1 = _mm_max_epi8(m00_1,m00_4);
    m10_1 = _mm_max_epi8(m10_1,m10_2);
    m10_1 = _mm_max_epi8(m10_1,m10_3);
    m10_1 = _mm_max_epi8(m10_1,m10_4);
    m11_1 = _mm_max_epi8(m11_1,m11_2);
    m11_1 = _mm_max_epi8(m11_1,m11_3);
    m11_1 = _mm_max_epi8(m11_1,m11_4);
    m01_1 = _mm_subs_epi8(m01_1,*m10_128);
    m00_1 = _mm_subs_epi8(m00_1,*m11_128);
    m10_1 = _mm_adds_epi8(m10_1,*m10_128);
    m11_1 = _mm_adds_epi8(m11_1,*m11_128);
    m01_1 = _mm_max_epi8(m01_1,m00_1);
    m10_1 = _mm_max_epi8(m10_1,m11_1);
    *ext_128 = _mm_subs_epi8(m10_1,m01_1);
    alpha_ptr+=8;
    beta_ptr+=8;
#elif defined(__arm__)
    m11_128        = (int8x16_t *)&m_11[k<<4];
    m10_128        = (int8x16_t *)&m_10[k<<4];
    ext_128        = (int8x16_t *)&ext[k<<4];
    m00_4 = vqaddq_s8(alpha_ptr[7],beta_ptr[3]); //ALPHA_BETA_4m00;
    m11_4 = vqaddq_s8(alpha_ptr[7],beta_ptr[7]); //ALPHA_BETA_4m11;
    m00_3 = vqaddq_s8(alpha_ptr[6],beta_ptr[7]); //ALPHA_BETA_3m00;
    m11_3 = vqaddq_s8(alpha_ptr[6],beta_ptr[3]); //ALPHA_BETA_3m11;
    m00_2 = vqaddq_s8(alpha_ptr[1],beta_ptr[4]); //ALPHA_BETA_2m00;
    m11_2 = vqaddq_s8(alpha_ptr[1],beta_ptr[0]); //ALPHA_BETA_2m11;
    m11_1 = vqaddq_s8(alpha_ptr[0],beta_ptr[4]); //ALPHA_BETA_1m11;
    m00_1 = vqaddq_s8(alpha_ptr[0],beta_ptr[0]); //ALPHA_BETA_1m00;
    m01_4 = vqaddq_s8(alpha_ptr[5],beta_ptr[6]); //ALPHA_BETA_4m01;
    m10_4 = vqaddq_s8(alpha_ptr[5],beta_ptr[2]); //ALPHA_BETA_4m10;
    m01_3 = vqaddq_s8(alpha_ptr[4],beta_ptr[2]); //ALPHA_BETA_3m01;
    m10_3 = vqaddq_s8(alpha_ptr[4],beta_ptr[6]); //ALPHA_BETA_3m10;
    m01_2 = vqaddq_s8(alpha_ptr[3],beta_ptr[1]); //ALPHA_BETA_2m01;
    m10_2 = vqaddq_s8(alpha_ptr[3],beta_ptr[5]); //ALPHA_BETA_2m10;
    m10_1 = vqaddq_s8(alpha_ptr[2],beta_ptr[1]); //ALPHA_BETA_1m10;
    m01_1 = vqaddq_s8(alpha_ptr[2],beta_ptr[5]); //ALPHA_BETA_1m01;
    m01_1 = vmaxq_s8(m01_1,m01_2);
    m01_1 = vmaxq_s8(m01_1,m01_3);
    m01_1 = vmaxq_s8(m01_1,m01_4);
    m00_1 = vmaxq_s8(m00_1,m00_2);
    m00_1 = vmaxq_s8(m00_1,m00_3);
    m00_1 = vmaxq_s8(m00_1,m00_4);
    m10_1 = vmaxq_s8(m10_1,m10_2);
    m10_1 = vmaxq_s8(m10_1,m10_3);
    m10_1 = vmaxq_s8(m10_1,m10_4);
    m11_1 = vmaxq_s8(m11_1,m11_2);
    m11_1 = vmaxq_s8(m11_1,m11_3);
    m11_1 = vmaxq_s8(m11_1,m11_4);
    m01_1 = vqsubq_s8(m01_1,*m10_128);
    m00_1 = vqsubq_s8(m00_1,*m11_128);
    m10_1 = vqaddq_s8(m10_1,*m10_128);
    m11_1 = vqaddq_s8(m11_1,*m11_128);
    m01_1 = vmaxq_s8(m01_1,m00_1);
    m10_1 = vmaxq_s8(m10_1,m11_1);
    *ext_128 = vqsubq_s8(m10_1,m01_1);
    alpha_ptr+=8;
    beta_ptr+=8;
#endif
  }
}



//int pi2[n],pi3[n+8],pi5[n+8],pi4[n+8],pi6[n+8],
int *pi2tab8[188],*pi5tab8[188],*pi4tab8[188],*pi6tab8[188];

void free_td8(void) {
  int ind;

  for (ind=0; ind<188; ind++) {
    free_and_zero(pi2tab8[ind]);
    free_and_zero(pi5tab8[ind]);
    free_and_zero(pi4tab8[ind]);
    free_and_zero(pi6tab8[ind]);
  }
}



extern RAN_CONTEXT_t RC;

void init_td8(void) {
  int ind,i,j,n,n2,pi,pi3;
  short *base_interleaver;

  for (ind=0; ind<188; ind++) {
    n = f1f2mat[ind].nb_bits;
    base_interleaver=il_tb+f1f2mat[ind].beg_index;
#ifdef MEX
    // This is needed for the Mex implementation to make the memory persistent
    pi2tab8[ind] = mxMalloc((n+8)*sizeof(int));
    pi5tab8[ind] = mxMalloc((n+8)*sizeof(int));
    pi4tab8[ind] = mxMalloc((n+8)*sizeof(int));
    pi6tab8[ind] = mxMalloc((n+8)*sizeof(int));
#else
    pi2tab8[ind] = malloc((n+8)*sizeof(int));
    pi5tab8[ind] = malloc((n+8)*sizeof(int));
    pi4tab8[ind] = malloc((n+8)*sizeof(int));
    pi6tab8[ind] = malloc((n+8)*sizeof(int));
#endif

    if ((n&15)>0) {
      n2 = n+8;
    } else
      n2 = n;

    for (j=0,i=0; i<n2; i++,j+=16) {
      if (j>=n2)
        j-=(n2-1);

      pi2tab8[ind][i] = j;
      //    printf("pi2[%d] = %d\n",i,j);
    }

    for (i=0; i<n2; i++) {
      pi = base_interleaver[i];//(unsigned int)threegpplte_interleaver(f1,f2,n);
      pi3 = pi2tab8[ind][pi];
      pi4tab8[ind][pi2tab8[ind][i]] = pi3;
      pi5tab8[ind][pi3] = pi2tab8[ind][i];
      pi6tab8[ind][pi] = pi2tab8[ind][i];
    }
  }
}

uint8_t phy_threegpplte_turbo_decoder8(int16_t *y,
                                       int16_t *y2,
                                       uint8_t *decoded_bytes,
                                       uint8_t *decoded_bytes2,
                                       uint16_t n,
                                       uint8_t max_iterations,
                                       uint8_t crc_type,
                                       uint8_t F,
                                       time_stats_t *init_stats,
                                       time_stats_t *alpha_stats,
                                       time_stats_t *beta_stats,
                                       time_stats_t *gamma_stats,
                                       time_stats_t *ext_stats,
                                       time_stats_t *intl1_stats,
                                       time_stats_t *intl2_stats) {
  /*  y is a pointer to the input
      decoded_bytes is a pointer to the decoded output
      n is the size in bits of the coded block, with the tail */
  int n2;
  llr_t y8[3*(n+16)] __attribute__((aligned(16)));
  llr_t systematic0[n+32] __attribute__ ((aligned(16)));
  llr_t systematic1[n+32] __attribute__ ((aligned(16)));
  llr_t systematic2[n+32] __attribute__ ((aligned(16)));
  llr_t yparity1[n+32] __attribute__ ((aligned(16)));
  llr_t yparity2[n+32] __attribute__ ((aligned(16)));
  llr_t ext[n+128] __attribute__((aligned(16)));
  llr_t ext2[n+128] __attribute__((aligned(16)));
  llr_t alpha[(n+16)*8] __attribute__ ((aligned(16)));
  llr_t beta[(n+16)*8] __attribute__ ((aligned(16)));
  llr_t m11[n+16] __attribute__ ((aligned(16)));
  llr_t m10[n+16] __attribute__ ((aligned(16)));
  //  int *pi2_p,*pi4_p,*pi5_p,*pi6_p;
  int *pi4_p,*pi5_p,*pi6_p;
  llr_t *s1,*s2,*yp1,*yp2,*yp;
  unsigned int i,j,iind;//,pi;
  unsigned char iteration_cnt=0;
  unsigned int crc,oldcrc,crc_len;
  uint8_t temp;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *yp128;
  __m128i tmp128[(n+8)>>3];
  __m128i tmp, zeros=_mm_setzero_si128();
#elif defined(__arm__)
  int8x16_t *yp128;
  int8x16_t tmp128[(n+8)>>3];
  int8x16_t tmp, zeros=vdupq_n_s8(0);
  const uint8_t __attribute__ ((aligned (16))) _Powers[16]=
  { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 };
  // Set the powers of 2 (do it once for all, if applicable)
  uint8x16_t Powers= vld1q_u8(_Powers);
#endif
  int offset8_flag=0;

  if (crc_type > 3) {
    printf("Illegal crc length!\n");
    return 255;
  }

  if (init_stats) start_meas(init_stats);

  if ((n&15)>0) {
    n2 = n+8;
    offset8_flag=1;
  } else
    n2 = n;

  for (iind=0; iind < 188 && f1f2mat[iind].nb_bits != n; iind++);

  if ( iind == 188 ) {
    printf("Illegal frame length!\n");
    return 255;
  }

  switch (crc_type) {
    case CRC24_A:
    case CRC24_B:
      crc_len=3;
      break;

    case CRC16:
      crc_len=2;
      break;

    case CRC8:
      crc_len=1;
      break;

    default:
      crc_len=3;
  }

#if defined(__x86_64__) || defined(__i386__)
  // note: this makes valgrind freak
  __m128i avg=_mm_set1_epi32(0);

  for (i=0; i<(3*(n>>4))+1; i++) {
    __m128i tmp=_mm_abs_epi16(_mm_unpackhi_epi16(((__m128i *)y)[i],((__m128i *)y)[i]));
    avg=_mm_add_epi32(_mm_cvtepi16_epi32(_mm_abs_epi16(((__m128i *)y)[i])),avg);
    avg=_mm_add_epi32(_mm_cvtepi16_epi32(tmp),avg);
  }

  int32_t round_avg=(_mm_extract_epi32(avg,0)+_mm_extract_epi32(avg,1)+_mm_extract_epi32(avg,2)+_mm_extract_epi32(avg,3))/(n*3);

  //printf("avg input turbo: %d sum %d taille bloc %d\n",round_avg,round_sum,n);

  if (round_avg < 16 )
    for (i=0,j=0; i<(3*(n2>>4))+1; i++,j+=2)
      ((__m128i *)y8)[i] = _mm_packs_epi16(((__m128i *)y)[j],((__m128i *)y)[j+1]);
  else if (round_avg < 32)
    for (i=0,j=0; i<(3*(n2>>4))+1; i++,j+=2)
      ((__m128i *)y8)[i] = _mm_packs_epi16(_mm_srai_epi16(((__m128i *)y)[j],1),_mm_srai_epi16(((__m128i *)y)[j+1],1));
  else if (round_avg < 64 )
    for (i=0,j=0; i<(3*(n2>>4))+1; i++,j+=2)
      ((__m128i *)y8)[i] = _mm_packs_epi16(_mm_srai_epi16(((__m128i *)y)[j],2),_mm_srai_epi16(((__m128i *)y)[j+1],2));
  else if (round_avg < 128)
    for (i=0,j=0; i<(3*(n2>>4))+1; i++,j+=2)
      ((__m128i *)y8)[i] = _mm_packs_epi16(_mm_srai_epi16(((__m128i *)y)[j],3),_mm_srai_epi16(((__m128i *)y)[j+1],3));
  else
    for (i=0,j=0; i<(3*(n2>>4))+1; i++,j+=2)
      ((__m128i *)y8)[i] = _mm_packs_epi16(_mm_srai_epi16(((__m128i *)y)[j],3),_mm_srai_epi16(((__m128i *)y)[j+1],4));

  yp128 = (__m128i *)y8;
#elif defined(__arm__)
  int32x4_t avg=vdupq_n_s32(0);

  for (i=0; i<(3*(n>>4))+1; i++) {
    int16x8_t tmp=vabsq_s16(((int16x8_t *)y)[i]);
    avg = vqaddq_s32(avg,vaddl_s16(((int16x4_t *)&tmp)[0],((int16x4_t *)&tmp)[1]));
  }

  int32_t round_avg=(vgetq_lane_s32(avg,0)+vgetq_lane_s32(avg,1)+vgetq_lane_s32(avg,2)+vgetq_lane_s32(avg,3))/(n*3);

  //printf("avg input turbo: %d sum %d taille bloc %d\n",round_avg,round_sum,n);

  if (round_avg < 16 )
    for (i=0,j=0; i<(3*(n2>>3))+1; i++,j+=2)
      ((int8x8_t *)y8)[i] = vqmovn_s16(((int16x8_t *)y)[j]);
  else if (round_avg < 32)
    for (i=0,j=0; i<(3*(n2>>3))+1; i++,j+=2)
      ((int8x8_t *)y8)[i] = vqmovn_s16(vshrq_n_s16(((int16x8_t *)y)[j],1));
  else if (round_avg < 64 )
    for (i=0,j=0; i<(3*(n2>>3))+1; i++,j+=2)
      ((int8x8_t *)y8)[i] = vqmovn_s16(vshrq_n_s16(((int16x8_t *)y)[j],2));
  else
    for (i=0,j=0; i<(3*(n2>>3))+1; i++,j+=2)
      ((int8x8_t *)y8)[i] = vqmovn_s16(vshrq_n_s16(((int16x8_t *)y)[j],3));

  yp128 = (int8x16_t *)y8;
#endif
  s1 = systematic1;
  s2 = systematic2;
  yp1 = yparity1;
  yp2 = yparity2;
  yp=y8;
#if 1

  for (i=0; i<16 ; i++ )
    for (j=0; j<n2; j+=16) {
      int k=i+j;
      systematic0[k]=*yp++;
      yp1[k]=*yp++;
      yp2[k]=*yp++;
    }

#endif
  yp=(llr_t *)yp128;

  if (n2>n) {
    /*
    systematic0[n]=0;systematic0[n+1]=0;systematic0[n+2]=0;systematic0[n+3]=0;
    systematic0[n+4]=0;s[n+5]=0;s[n+6]=0;s[n+7]=0;
    s1[n]=0;s1[n+1]=0;s1[n+2]=0;s1[n+3]=0;
    s1[n+4]=0;s1[n+5]=0;s1[n+6]=0;s1[n+7]=0;
    s2[n]=0;s2[n+1]=0;s2[n+2]=0;s2[n+3]=0;
    s2[n+4]=0;s2[n+5]=0;s2[n+6]=0;s2[n+7]=0;*/
    yp=(llr_t *)(y8+n);
  }

  //  printf("n=%d,n2=%d\n",n,n2);

  // Termination
  for (i=n2; i<n2+3; i++) {
    systematic0[i]= *yp;
    s1[i] = systematic0[i] ;
    s2[i] = systematic0[i];
    yp++;
    yp1[i] = *yp;
    yp++;
#ifdef DEBUG_LOGMAP
    printf("Term 1 (%u): %d %d\n",i,systematic0[i],yp1[i]);
#endif //DEBUG_LOGMAP
  }

  for (i=n2+16; i<n2+19; i++) {
    systematic0[i]= *yp;
    s1[i] = systematic0[i] ;
    s2[i] = systematic0[i];
    yp++;
    yp2[i-16] = *yp;
    yp++;
#ifdef DEBUG_LOGMAP
    printf("Term 2 (%u): %d %d\n",i-16,systematic0[i],yp2[i-16]);
#endif //DEBUG_LOGMAP
  }

#ifdef DEBUG_LOGMAP
  printf("\n");
#endif //DEBUG_LOGMAP

  if (init_stats) stop_meas(init_stats);

  // do log_map from first parity bit
  log_map8(systematic0,yparity1,m11,m10,alpha,beta,ext,n2,0,F,offset8_flag,alpha_stats,beta_stats,gamma_stats,ext_stats);

  while (iteration_cnt++ < max_iterations) {
#ifdef DEBUG_LOGMAP
    printf("\n*******************ITERATION %d (n %d, n2 %d), ext %p\n\n",iteration_cnt,n,n2,ext);
#endif //DEBUG_LOGMAP

    if (intl1_stats) start_meas(intl1_stats);

    pi4_p=pi4tab8[iind];

    for (i=0; i<(n2>>4); i++) { // steady-state portion
#if defined(__x86_64__) || defined(__i386__)
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],0);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],1);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],2);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],3);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],4);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],5);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],6);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],7);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],8);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],9);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],10);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],11);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],12);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],13);
      tmp=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],14);
      ((__m128i *)systematic2)[i]=_mm_insert_epi8(tmp,((llr_t *)ext)[*pi4_p++],15);
#elif defined(__arm__)
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,0);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,1);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,2);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,3);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,4);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,5);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,6);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,7);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,8);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,9);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,10);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,11);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,12);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,13);
      tmp=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,14);
      ((int8x16_t *)systematic2)[i]=vsetq_lane_s8(((llr_t *)ext)[*pi4_p++],tmp,15);
#endif
    }

    if (intl1_stats) stop_meas(intl1_stats);

    // do log_map from second parity bit
    log_map8(systematic2,yparity2,m11,m10,alpha,beta,ext2,n2,1,F,offset8_flag,alpha_stats,beta_stats,gamma_stats,ext_stats);
    pi5_p=pi5tab8[iind];
    uint16_t decoded_bytes_interl[6144/16] __attribute__((aligned(16)));

    if ((n2&0x7f) == 0) {  // n2 is a multiple of 128 bits
      for (i=0; i<(n2>>4); i++) {
#if defined(__x86_64__) || defined(__i386__)
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],0);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],1);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],2);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],3);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],4);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],5);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],6);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],7);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],8);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],9);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],10);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],11);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],12);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],13);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],14);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],15);
        decoded_bytes_interl[i]=(uint16_t) _mm_movemask_epi8(_mm_cmpgt_epi8(tmp,zeros));
        ((__m128i *)systematic1)[i] = _mm_adds_epi8(_mm_subs_epi8(tmp,((__m128i *)ext)[i]),((__m128i *)systematic0)[i]);
#elif defined(__arm__)
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,0);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,1);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,2);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,3);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,4);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,5);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,6);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,7);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,8);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,9);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,10);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,11);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,12);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,13);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,14);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,15);
        uint64x2_t Mask= vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vandq_u8(vcgtq_s8(tmp,zeros), Powers))));
        vst1q_lane_u8(&((uint8_t *)&decoded_bytes[i])[0], (uint8x16_t)Mask, 0);
        vst1q_lane_u8(&((uint8_t *)&decoded_bytes[i])[1], (uint8x16_t)Mask, 8);
        ((int8x16_t *)systematic1)[i] = vqaddq_s8(vqsubq_s8(tmp,((int8x16_t *)ext)[i]),((int8x16_t *)systematic0)[i]);
#endif
      }
    } else {
      for (i=0; i<(n2>>4); i++) {
#if defined(__x86_64__) || defined(__i386__)
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],0);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],1);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],2);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],3);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],4);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],5);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],6);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],7);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],8);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],9);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],10);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],11);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],12);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],13);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],14);
        tmp=_mm_insert_epi8(tmp,ext2[*pi5_p++],15);
        tmp128[i] = _mm_adds_epi8(((__m128i *)ext2)[i],((__m128i *)systematic2)[i]);
        ((__m128i *)systematic1)[i] = _mm_adds_epi8(_mm_subs_epi8(tmp,((__m128i *)ext)[i]),((__m128i *)systematic0)[i]);
#elif defined(__arm__)
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,0);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,1);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,2);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,3);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,4);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,5);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,6);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,7);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,8);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,9);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,10);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,11);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,12);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,13);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,14);
        tmp=vsetq_lane_s8(ext2[*pi5_p++],tmp,15);
        tmp128[i] = vqaddq_s8(((int8x16_t *)ext2)[i],((int8x16_t *)systematic2)[i]);
        ((int8x16_t *)systematic1)[i] = vqaddq_s8(vqsubq_s8(tmp,((int8x16_t *)ext)[i]),((int8x16_t *)systematic0)[i]);
#endif
      }
    }

    // Check if we decoded the block
    if (iteration_cnt>1) {
      if (intl2_stats) start_meas(intl2_stats);

      if ((n2&0x7f) == 0) {  // n2 is a multiple of 128 bits
        // re-order the decoded bits in theregular order
        // as it is presently ordered as 16 sequential columns
#if defined(__x86_64__) || defined(__i386__)
        __m128i *dbytes=(__m128i *)decoded_bytes_interl;
        __m128i shuffle=SHUFFLE16(7,6,5,4,3,2,1,0);
        __m128i mask  __attribute__((aligned(16)));
        int n_128=n2>>7;

        for (i=0; i<n_128; i++) {
          mask=_mm_set1_epi16(1);
          __m128i tmp __attribute__((aligned(16)));
          tmp=_mm_shuffle_epi8(dbytes[i],shuffle);
          __m128i tmp2 __attribute__((aligned(16))) ;
          tmp2=_mm_and_si128(tmp,mask);
          tmp2=_mm_cmpeq_epi16(tmp2,mask);
          //    printf("decoded_bytes %p\n",decoded_bytes);
          decoded_bytes[n_128*0+i]=(uint8_t) _mm_movemask_epi8(_mm_packs_epi16(tmp2,zeros));
          int j;

          for (j=1; j<16; j++) {
            mask=_mm_slli_epi16(mask,1);
            tmp2=_mm_and_si128(tmp,mask);
            tmp2=_mm_cmpeq_epi16(tmp2,mask);
            decoded_bytes[n_128*j +i]=(uint8_t) _mm_movemask_epi8(_mm_packs_epi16(tmp2,zeros));
          }
        }

#elif defined(__arm__)
        uint8x16_t *dbytes=(uint8x16_t *)decoded_bytes_interl;
        uint16x8_t mask  __attribute__((aligned(16)));
        int n_128=n2>>7;

        for (i=0; i<n_128; i++) {
          mask=vdupq_n_u16(1);
          uint8x16_t tmp __attribute__((aligned(16)));
          tmp=vcombine_u8(vrev64_u8(((uint8x8_t *)&dbytes[i])[1]),vrev64_u8(((uint8x8_t *)&dbytes[i])[0]));
          vst1q_lane_u8(&decoded_bytes[n_128*0+i],(uint8x16_t)vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vandq_u8(tmp, Powers)))),0);
          int j;

          for (j=1; j<16; j++) {
            mask=vshlq_n_u16(mask,1);
            vst1q_lane_u8(&decoded_bytes[n_128*0+i],(uint8x16_t)vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vandq_u8(tmp, Powers)))),0);
          }
        }

#endif
      } else {
        pi6_p=pi6tab8[iind];

        for (i=0; i<(n2>>4); i++) {
#if defined(__x86_64__) || defined(__i386__)
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],7);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],6);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],5);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],4);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],3);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],2);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],1);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],0);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],15);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],14);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],13);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],12);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],11);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],10);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],9);
          tmp=_mm_insert_epi8(tmp, ((llr_t *)tmp128)[*pi6_p++],8);
          tmp=_mm_cmpgt_epi8(tmp,zeros);
          ((uint16_t *)decoded_bytes)[i]=(uint16_t)_mm_movemask_epi8(tmp);
#elif defined(__arm__)
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,7);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,6);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,5);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,4);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,3);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,2);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,1);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,0);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,15);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,14);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,13);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,12);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,11);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,10);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,9);
          tmp=vsetq_lane_s8(((llr_t *)tmp128)[*pi6_p++],tmp,8);
          uint64x2_t Mask= vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vandq_u8(vcgtq_s8(tmp,zeros), Powers))));
          vst1q_lane_u8(&((uint8_t *)&decoded_bytes[i])[0], (uint8x16_t)Mask, 0);
          vst1q_lane_u8(&((uint8_t *)&decoded_bytes[i])[1], (uint8x16_t)Mask, 8);
#endif
        }
      }

      // check the CRC
      oldcrc= *((unsigned int *)(&decoded_bytes[(n>>3)-crc_len]));

      switch (crc_type) {
        case CRC24_A:
          oldcrc&=0x00ffffff;
          crc = crc24a(&decoded_bytes[F>>3],
                       n-24-F)>>8;
          temp=((uint8_t *)&crc)[2];
          ((uint8_t *)&crc)[2] = ((uint8_t *)&crc)[0];
          ((uint8_t *)&crc)[0] = temp;
          break;

        case CRC24_B:
          oldcrc&=0x00ffffff;
          crc = crc24b(decoded_bytes,
                       n-24)>>8;
          temp=((uint8_t *)&crc)[2];
          ((uint8_t *)&crc)[2] = ((uint8_t *)&crc)[0];
          ((uint8_t *)&crc)[0] = temp;
          break;

        case CRC16:
          oldcrc&=0x0000ffff;
          crc = crc16(decoded_bytes,
                      n-16)>>16;
          break;

        case CRC8:
          oldcrc&=0x000000ff;
          crc = crc8(decoded_bytes,
                     n-8)>>24;
          break;

        default:
          printf("FATAL: 3gpplte_turbo_decoder_sse.c: Unknown CRC\n");
          return(255);
          break;
      }

      if (intl2_stats) stop_meas(intl2_stats);

      if (crc == oldcrc) {
        return(iteration_cnt);
      }
    }

    // do a new iteration if it is not yet decoded
    if (iteration_cnt < max_iterations) {
      log_map8(systematic1,yparity1,m11,m10,alpha,beta,ext,n2,0,F,offset8_flag,alpha_stats,beta_stats,gamma_stats,ext_stats);
#if defined(__x86_64__) || defined(__i386__)
      __m128i *ext_128=(__m128i *) ext;
      __m128i *s1_128=(__m128i *) systematic1;
      __m128i *s0_128=(__m128i *) systematic0;
#elif defined(__arm__)
      int8x16_t *ext_128=(int8x16_t *) ext;
      int8x16_t *s1_128=(int8x16_t *) systematic1;
      int8x16_t *s0_128=(int8x16_t *) systematic0;
#endif
      int myloop=n2>>4;

      for (i=0; i<myloop; i++) {
#if defined(__x86_64__) || defined(__i386__)
        *ext_128=_mm_adds_epi8(_mm_subs_epi8(*ext_128,*s1_128++),*s0_128++);
#elif defined(__arm__)
        *ext_128=vqaddq_s8(vqsubq_s8(*ext_128,*s1_128++),*s0_128++);
#endif
        ext_128++;
      }
    }
  }

  return(iteration_cnt);
}
