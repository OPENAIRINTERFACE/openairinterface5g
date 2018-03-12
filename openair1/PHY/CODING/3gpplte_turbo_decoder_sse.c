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

   Note: This routine currently requires SSE2,SSSE3 and SSE4.1 equipped computers.  IT WON'T RUN OTHERWISE!

   Changelog: 17.11.2009 FK SSE4.1 not required anymore
   Aug. 2012 new parallelization options for higher speed (8-way parallelization)
   Jan. 2013 8-bit LLR support with 16-way parallelization
   Feb. 2013 New interleaving and hard-decision optimizations (L. Thomas)
*/

///
///

#include "PHY/sse_intrin.h"

#ifndef TEST_DEBUG
#include "PHY/defs.h"
#include "PHY/CODING/defs.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "extern_3GPPinterleaver.h"
#else

#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

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


#ifdef LLR8

typedef int8_t llr_t; // internal decoder LLR data is 8-bit fixed
typedef int8_t channel_t;
#define MAX 64
#else

typedef int16_t llr_t; // internal decoder LLR data is 16-bit fixed
typedef int16_t channel_t;
#define MAX 256

#endif

void log_map (llr_t* systematic,channel_t* y_parity, llr_t* m11, llr_t* m10, llr_t *alpha, llr_t *beta, llr_t* ext,unsigned short frame_length,unsigned char term_flag,unsigned char F,int offset8_flag,
              time_stats_t *alpha_stats,time_stats_t *beta_stats,time_stats_t *gamma_stats,time_stats_t *ext_stats);
void compute_gamma(llr_t* m11,llr_t* m10,llr_t* systematic, channel_t* y_parity, unsigned short frame_length,unsigned char term_flag);
void compute_alpha(llr_t*alpha,llr_t *beta, llr_t* m11,llr_t* m10, unsigned short frame_length,unsigned char F);
void compute_beta(llr_t*alpha, llr_t* beta,llr_t* m11,llr_t* m10, unsigned short frame_length,unsigned char F,int offset8_flag);
void compute_ext(llr_t* alpha,llr_t* beta,llr_t* m11,llr_t* m10,llr_t* extrinsic, llr_t* ap, unsigned short frame_length);


void print_bytes(char *s, __m128i *x)
{

  int8_t *tempb = (int8_t *)x;

  printf("%s  : %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",s,
         tempb[0],tempb[1],tempb[2],tempb[3],tempb[4],tempb[5],tempb[6],tempb[7],
         tempb[8],tempb[9],tempb[10],tempb[11],tempb[12],tempb[13],tempb[14],tempb[15]);

}


void log_map(llr_t* systematic,
             channel_t* y_parity,
             llr_t* m11,
             llr_t* m10,
             llr_t *alpha,
             llr_t *beta,
             llr_t* ext,
             unsigned short frame_length,
             unsigned char term_flag,
             unsigned char F,
             int offset8_flag,
             time_stats_t *alpha_stats,
             time_stats_t *beta_stats,
             time_stats_t *gamma_stats,
             time_stats_t *ext_stats)
{

#ifdef DEBUG_LOGMAP
  msg("log_map, frame_length %d\n",frame_length);
#endif

  start_meas(gamma_stats) ;
  compute_gamma(m11,m10,systematic,y_parity,frame_length,term_flag) ;
  stop_meas(gamma_stats);
  start_meas(alpha_stats) ;
  compute_alpha(alpha,beta,m11,m10,frame_length,F)                  ;
  stop_meas(alpha_stats);
  start_meas(beta_stats)  ;
  compute_beta(alpha,beta,m11,m10,frame_length,F,offset8_flag)      ;
  stop_meas(beta_stats);
  start_meas(ext_stats)   ;
  compute_ext(alpha,beta,m11,m10,ext,systematic,frame_length)       ;
  stop_meas(ext_stats);


}

void compute_gamma(llr_t* m11,llr_t* m10,llr_t* systematic,channel_t* y_parity,
                   unsigned short frame_length,unsigned char term_flag)
{
  int k,K1;
  __m128i *systematic128 = (__m128i *)systematic;
  __m128i *y_parity128   = (__m128i *)y_parity;
  __m128i *m10_128        = (__m128i *)m10;
  __m128i *m11_128        = (__m128i *)m11;

#ifdef DEBUG_LOGMAP
  msg("compute_gamma, %p,%p,%p,%p,framelength %d\n",m11,m10,systematic,y_parity,frame_length);
#endif
#ifndef LLR8
  K1=frame_length>>3;

  for (k=0; k<K1; k++) {

    m11_128[k] = _mm_srai_epi16(_mm_adds_epi16(systematic128[k],y_parity128[k]),1);
    m10_128[k] = _mm_srai_epi16(_mm_subs_epi16(systematic128[k],y_parity128[k]),1);
    /*
    printf("gamma %d: s %d,%d,%d,%d,%d,%d,%d,%d\n",
     k,
     (int16_t)_mm_extract_epi16(systematic128[k],0),
     (int16_t)_mm_extract_epi16(systematic128[k],1),
     (int16_t)_mm_extract_epi16(systematic128[k],2),
     (int16_t)_mm_extract_epi16(systematic128[k],3),
     (int16_t)_mm_extract_epi16(systematic128[k],4),
     (int16_t)_mm_extract_epi16(systematic128[k],5),
     (int16_t)_mm_extract_epi16(systematic128[k],6),
     (int16_t)_mm_extract_epi16(systematic128[k],7));

    printf("gamma %d: yp %d,%d,%d,%d,%d,%d,%d,%d\n",
     k,
     (int16_t)_mm_extract_epi16(y_parity128[k],0),
     (int16_t)_mm_extract_epi16(y_parity128[k],1),
     (int16_t)_mm_extract_epi16(y_parity128[k],2),
     (int16_t)_mm_extract_epi16(y_parity128[k],3),
     (int16_t)_mm_extract_epi16(y_parity128[k],4),
     (int16_t)_mm_extract_epi16(y_parity128[k],5),
     (int16_t)_mm_extract_epi16(y_parity128[k],6),
     (int16_t)_mm_extract_epi16(y_parity128[k],7));

    printf("gamma %d: m11 %d,%d,%d,%d,%d,%d,%d,%d\n",
     k,
     (int16_t)_mm_extract_epi16(m11_128[k],0),
     (int16_t)_mm_extract_epi16(m11_128[k],1),
     (int16_t)_mm_extract_epi16(m11_128[k],2),
     (int16_t)_mm_extract_epi16(m11_128[k],3),
     (int16_t)_mm_extract_epi16(m11_128[k],4),
     (int16_t)_mm_extract_epi16(m11_128[k],5),
     (int16_t)_mm_extract_epi16(m11_128[k],6),
     (int16_t)_mm_extract_epi16(m11_128[k],7));
    printf("gamma %d: m10 %d,%d,%d,%d,%d,%d,%d,%d\n",
     k,
     (int16_t)_mm_extract_epi16(m10_128[k],0),
     (int16_t)_mm_extract_epi16(m10_128[k],1),
     (int16_t)_mm_extract_epi16(m10_128[k],2),
     (int16_t)_mm_extract_epi16(m10_128[k],3),
     (int16_t)_mm_extract_epi16(m10_128[k],4),
     (int16_t)_mm_extract_epi16(m10_128[k],5),
     (int16_t)_mm_extract_epi16(m10_128[k],6),
     (int16_t)_mm_extract_epi16(m10_128[k],7));
    */

  }

  // Termination
  m11_128[k] = _mm_srai_epi16(_mm_adds_epi16(systematic128[k+term_flag],y_parity128[k]),1);
  m10_128[k] = _mm_srai_epi16(_mm_subs_epi16(systematic128[k+term_flag],y_parity128[k]),1);

  //  printf("gamma (term): %d,%d, %d,%d, %d,%d\n",m11[k<<3],m10[k<<3],m11[1+(k<<3)],m10[1+(k<<3)],m11[2+(k<<3)],m10[2+(k<<3)]);
#else
  register __m128i sl,sh,ypl,yph; //K128=_mm_set1_epi8(-128);
  K1 = (frame_length>>4);

  for (k=0; k<K1; k++) {
    sl  = _mm_cvtepi8_epi16(systematic128[k]);
    sh = _mm_cvtepi8_epi16(_mm_srli_si128(systematic128[k],8));
    ypl = _mm_cvtepi8_epi16(y_parity128[k]);
    yph = _mm_cvtepi8_epi16(_mm_srli_si128(y_parity128[k],8));
    m11_128[k] = _mm_packs_epi16(_mm_srai_epi16(_mm_adds_epi16(sl,ypl),1),
                                 _mm_srai_epi16(_mm_adds_epi16(sh,yph),1));
    m10_128[k] = _mm_packs_epi16(_mm_srai_epi16(_mm_subs_epi16(sl,ypl),1),
                                 _mm_srai_epi16(_mm_subs_epi16(sh,yph),1));
    //    m11_128[k] = _mm_adds_epi8(systematic128[k],y_parity128[k]);
    //    m10_128[k] = _mm_subs_epi8(systematic128[k],y_parity128[k]);
    //    m11_128[k] = _mm_sub_epi8(_mm_avg_epu8(_mm_add_epi8(systematic128[k],K128),_mm_add_epi8(y_parity128[k],K128)),K128);
    //    m10_128[k] = _mm_sub_epi8(_mm_avg_epu8(_mm_add_epi8(systematic128[k],K128),_mm_add_epi8(_mm_sign_epi8(y_parity128[k],K128),K128)),K128);

    /*
    printf("gamma %d: s %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
     k,
     (int8_t)_mm_extract_epi8(systematic128[k],0),
     (int8_t)_mm_extract_epi8(systematic128[k],1),
     (int8_t)_mm_extract_epi8(systematic128[k],2),
     (int8_t)_mm_extract_epi8(systematic128[k],3),
     (int8_t)_mm_extract_epi8(systematic128[k],4),
     (int8_t)_mm_extract_epi8(systematic128[k],5),
     (int8_t)_mm_extract_epi8(systematic128[k],6),
     (int8_t)_mm_extract_epi8(systematic128[k],7),
     (int8_t)_mm_extract_epi8(systematic128[k],8),
     (int8_t)_mm_extract_epi8(systematic128[k],9),
     (int8_t)_mm_extract_epi8(systematic128[k],10),
     (int8_t)_mm_extract_epi8(systematic128[k],11),
     (int8_t)_mm_extract_epi8(systematic128[k],12),
     (int8_t)_mm_extract_epi8(systematic128[k],13),
     (int8_t)_mm_extract_epi8(systematic128[k],14),
     (int8_t)_mm_extract_epi8(systematic128[k],15));
    printf("gamma %d: yp %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
     k,
     (int8_t)_mm_extract_epi8(y_parity128[k],0),
     (int8_t)_mm_extract_epi8(y_parity128[k],1),
     (int8_t)_mm_extract_epi8(y_parity128[k],2),
     (int8_t)_mm_extract_epi8(y_parity128[k],3),
     (int8_t)_mm_extract_epi8(y_parity128[k],4),
     (int8_t)_mm_extract_epi8(y_parity128[k],5),
     (int8_t)_mm_extract_epi8(y_parity128[k],6),
     (int8_t)_mm_extract_epi8(y_parity128[k],7),
     (int8_t)_mm_extract_epi8(y_parity128[k],8),
     (int8_t)_mm_extract_epi8(y_parity128[k],9),
     (int8_t)_mm_extract_epi8(y_parity128[k],10),
     (int8_t)_mm_extract_epi8(y_parity128[k],11),
     (int8_t)_mm_extract_epi8(y_parity128[k],12),
     (int8_t)_mm_extract_epi8(y_parity128[k],13),
     (int8_t)_mm_extract_epi8(y_parity128[k],14),
     (int8_t)_mm_extract_epi8(y_parity128[k],15));
    printf("gamma %d: m11 %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
     k,
     (int8_t)_mm_extract_epi8(m11_128[k],0),
     (int8_t)_mm_extract_epi8(m11_128[k],1),
     (int8_t)_mm_extract_epi8(m11_128[k],2),
     (int8_t)_mm_extract_epi8(m11_128[k],3),
     (int8_t)_mm_extract_epi8(m11_128[k],4),
     (int8_t)_mm_extract_epi8(m11_128[k],5),
     (int8_t)_mm_extract_epi8(m11_128[k],6),
     (int8_t)_mm_extract_epi8(m11_128[k],7),
     (int8_t)_mm_extract_epi8(m11_128[k],8),
     (int8_t)_mm_extract_epi8(m11_128[k],9),
     (int8_t)_mm_extract_epi8(m11_128[k],10),
     (int8_t)_mm_extract_epi8(m11_128[k],11),
     (int8_t)_mm_extract_epi8(m11_128[k],12),
     (int8_t)_mm_extract_epi8(m11_128[k],13),
     (int8_t)_mm_extract_epi8(m11_128[k],14),
     (int8_t)_mm_extract_epi8(m11_128[k],15));
    printf("gamma %d: m10 %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
     k,
     (int8_t)_mm_extract_epi8(m10_128[k],0),
     (int8_t)_mm_extract_epi8(m10_128[k],1),
     (int8_t)_mm_extract_epi8(m10_128[k],2),
     (int8_t)_mm_extract_epi8(m10_128[k],3),
     (int8_t)_mm_extract_epi8(m10_128[k],4),
     (int8_t)_mm_extract_epi8(m10_128[k],5),
     (int8_t)_mm_extract_epi8(m10_128[k],6),
     (int8_t)_mm_extract_epi8(m10_128[k],7),
     (int8_t)_mm_extract_epi8(m10_128[k],8),
     (int8_t)_mm_extract_epi8(m10_128[k],9),
     (int8_t)_mm_extract_epi8(m10_128[k],10),
     (int8_t)_mm_extract_epi8(m10_128[k],11),
     (int8_t)_mm_extract_epi8(m10_128[k],12),
     (int8_t)_mm_extract_epi8(m10_128[k],13),
     (int8_t)_mm_extract_epi8(m10_128[k],14),
     (int8_t)_mm_extract_epi8(m10_128[k],15));
    */
  }

  // Termination

  sl  = _mm_cvtepi8_epi16(systematic128[k+term_flag]);
  sh = _mm_cvtepi8_epi16(_mm_srli_si128(systematic128[k],8));
  ypl = _mm_cvtepi8_epi16(y_parity128[k+term_flag]);
  yph = _mm_cvtepi8_epi16(_mm_srli_si128(y_parity128[k],8));
  m11_128[k] = _mm_packs_epi16(_mm_srai_epi16(_mm_adds_epi16(sl,ypl),1),
                               _mm_srai_epi16(_mm_adds_epi16(sh,yph),1));
  m10_128[k] = _mm_packs_epi16(_mm_srai_epi16(_mm_subs_epi16(sl,ypl),1),
                               _mm_srai_epi16(_mm_subs_epi16(sh,yph),1));

  //    m11_128[k] = _mm_adds_epi8(systematic128[k+term_flag],y_parity128[k]);
  //    m10_128[k] = _mm_subs_epi8(systematic128[k+term_flag],y_parity128[k]);
  //  m11_128[k] = _mm_sub_epi8(_mm_avg_epu8(_mm_add_epi8(systematic128[k+term_flag],K128),_mm_add_epi8(y_parity128[k],K128)),K128);
  //  m10_128[k] = _mm_sub_epi8(_mm_avg_epu8(_mm_add_epi8(systematic128[k+term_flag],K128),_mm_add_epi8(_mm_sign_epi8(y_parity128[k],K128),K128)),K128);
  /*
  printf("m11 = %p (K1 %d)\n",&m11_128[k],K1);
    printf("gamma %d: s %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
     k+term_flag,
     _mm_extract_epi8(systematic128[k+term_flag],0),
     _mm_extract_epi8(systematic128[k+term_flag],1),
     _mm_extract_epi8(systematic128[k+term_flag],2),
     _mm_extract_epi8(systematic128[k+term_flag],3),
     _mm_extract_epi8(systematic128[k+term_flag],4),
     _mm_extract_epi8(systematic128[k+term_flag],5),
     _mm_extract_epi8(systematic128[k+term_flag],6),
     _mm_extract_epi8(systematic128[k+term_flag],7),
     _mm_extract_epi8(systematic128[k+term_flag],8),
     _mm_extract_epi8(systematic128[k+term_flag],9),
     _mm_extract_epi8(systematic128[k+term_flag],10),
     _mm_extract_epi8(systematic128[k+term_flag],11),
     _mm_extract_epi8(systematic128[k+term_flag],12),
     _mm_extract_epi8(systematic128[k+term_flag],13),
     _mm_extract_epi8(systematic128[k+term_flag],14),
     _mm_extract_epi8(systematic128[k+term_flag],15));
    printf("gamma %d: yp %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
     k,
     _mm_extract_epi8(y_parity128[k],0),
     _mm_extract_epi8(y_parity128[k],1),
     _mm_extract_epi8(y_parity128[k],2),
     _mm_extract_epi8(y_parity128[k],3),
     _mm_extract_epi8(y_parity128[k],4),
     _mm_extract_epi8(y_parity128[k],5),
     _mm_extract_epi8(y_parity128[k],6),
     _mm_extract_epi8(y_parity128[k],7),
     _mm_extract_epi8(y_parity128[k],8),
     _mm_extract_epi8(y_parity128[k],9),
     _mm_extract_epi8(y_parity128[k],10),
     _mm_extract_epi8(y_parity128[k],11),
     _mm_extract_epi8(y_parity128[k],12),
     _mm_extract_epi8(y_parity128[k],13),
     _mm_extract_epi8(y_parity128[k],14),
     _mm_extract_epi8(y_parity128[k],15));
    printf("gamma %d: m11 %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
     k,
     _mm_extract_epi8(m11_128[k],0),
     _mm_extract_epi8(m11_128[k],1),
     _mm_extract_epi8(m11_128[k],2),
     _mm_extract_epi8(m11_128[k],3),
     _mm_extract_epi8(m11_128[k],4),
     _mm_extract_epi8(m11_128[k],5),
     _mm_extract_epi8(m11_128[k],6),
     _mm_extract_epi8(m11_128[k],7),
     _mm_extract_epi8(m11_128[k],8),
     _mm_extract_epi8(m11_128[k],9),
     _mm_extract_epi8(m11_128[k],10),
     _mm_extract_epi8(m11_128[k],11),
     _mm_extract_epi8(m11_128[k],12),
     _mm_extract_epi8(m11_128[k],13),
     _mm_extract_epi8(m11_128[k],14),
     _mm_extract_epi8(m11_128[k],15));
  */
#endif
  _mm_empty();
  _m_empty();

}

#define L 40

void compute_alpha(llr_t* alpha,llr_t* beta,llr_t* m_11,llr_t* m_10,unsigned short frame_length,unsigned char F)
{
  int k,l,l2,K1,rerun_flag=0;
  __m128i *alpha128=(__m128i *)alpha,*alpha_ptr;
  __m128i a0,a1,a2,a3,a4,a5,a6,a7,*m11p,*m10p;
  __m128i m_b0,m_b1,m_b2,m_b3,m_b4,m_b5,m_b6,m_b7;
  __m128i new0,new1,new2,new3,new4,new5,new6,new7;
  __m128i alpha_max;

#ifndef LLR8
  l2 = L>>3;
  K1 = (frame_length>>3);
#else
  l2 = L>>4;
  K1 = (frame_length>>4);
#endif

  for (l=K1;; l=l2,rerun_flag=1) {
#ifndef LLR8
    alpha128 = (__m128i *)alpha;

    if (rerun_flag == 0) {
      alpha128[0] = _mm_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,0);
      alpha128[1] = _mm_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[2] = _mm_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[3] = _mm_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[4] = _mm_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[5] = _mm_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[6] = _mm_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[7] = _mm_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
    } else {
      //set initial alpha in columns 1-7 from final alpha from last run in columns 0-6
      alpha128[0] = _mm_slli_si128(alpha128[frame_length],2);
      alpha128[1] = _mm_slli_si128(alpha128[1+frame_length],2);
      alpha128[2] = _mm_slli_si128(alpha128[2+frame_length],2);
      alpha128[3] = _mm_slli_si128(alpha128[3+frame_length],2);
      alpha128[4] = _mm_slli_si128(alpha128[4+frame_length],2);
      alpha128[5] = _mm_slli_si128(alpha128[5+frame_length],2);
      alpha128[6] = _mm_slli_si128(alpha128[6+frame_length],2);
      alpha128[7] = _mm_slli_si128(alpha128[7+frame_length],2);
      // set initial alpha in column 0 to (0,-MAX/2,...,-MAX/2)
      alpha[8] = -MAX/2;
      alpha[16] = -MAX/2;
      alpha[24] = -MAX/2;
      alpha[32] = -MAX/2;
      alpha[40] = -MAX/2;
      alpha[48] = -MAX/2;
      alpha[56] = -MAX/2;
    }

    alpha_ptr = &alpha128[0];

    m11p = (__m128i*)m_11;
    m10p = (__m128i*)m_10;

    for (k=0;
         k<l;
         k++) {

      a1=_mm_load_si128(&alpha_ptr[1]);
      a3=_mm_load_si128(&alpha_ptr[3]);
      a5=_mm_load_si128(&alpha_ptr[5]);
      a7=_mm_load_si128(&alpha_ptr[7]);

      m_b0 = _mm_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi16(a7,*m11p);  // m11

      a0=_mm_load_si128(&alpha_ptr[0]);
      a2=_mm_load_si128(&alpha_ptr[2]);
      a4=_mm_load_si128(&alpha_ptr[4]);
      a6=_mm_load_si128(&alpha_ptr[6]);

      new0 = _mm_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm_adds_epi16(a0,*m11p);  // m11
      new1 = _mm_adds_epi16(a2,*m10p);  // m10
      new5 = _mm_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm_adds_epi16(a4,*m10p);  // m10
      new3 = _mm_adds_epi16(a6,*m11p);  // m11
      new7 = _mm_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm_max_epi16(m_b0,new0);
      a1 = _mm_max_epi16(m_b1,new1);
      a2 = _mm_max_epi16(m_b2,new2);
      a3 = _mm_max_epi16(m_b3,new3);
      a4 = _mm_max_epi16(m_b4,new4);
      a5 = _mm_max_epi16(m_b5,new5);
      a6 = _mm_max_epi16(m_b6,new6);
      a7 = _mm_max_epi16(m_b7,new7);

      alpha_max = _mm_max_epi16(a0,a1);
      alpha_max = _mm_max_epi16(alpha_max,a2);
      alpha_max = _mm_max_epi16(alpha_max,a3);
      alpha_max = _mm_max_epi16(alpha_max,a4);
      alpha_max = _mm_max_epi16(alpha_max,a5);
      alpha_max = _mm_max_epi16(alpha_max,a6);
      alpha_max = _mm_max_epi16(alpha_max,a7);

      alpha_ptr+=8;
      m11p++;
      m10p++;
      alpha_ptr[0] = _mm_subs_epi16(a0,alpha_max);
      alpha_ptr[1] = _mm_subs_epi16(a1,alpha_max);
      alpha_ptr[2] = _mm_subs_epi16(a2,alpha_max);
      alpha_ptr[3] = _mm_subs_epi16(a3,alpha_max);
      alpha_ptr[4] = _mm_subs_epi16(a4,alpha_max);
      alpha_ptr[5] = _mm_subs_epi16(a5,alpha_max);
      alpha_ptr[6] = _mm_subs_epi16(a6,alpha_max);
      alpha_ptr[7] = _mm_subs_epi16(a7,alpha_max);

    }

    /*
       // Try this on x86-64
    for (k=0;
    k<l;
    k+=8){


      //      m11_0=((__m128i*)m_11)[k];
      //      m10_0=((__m128i*)m_10)[k];

      // 0/7
      a1=_mm_load_si128(&alpha_ptr[1]);
      a3=_mm_load_si128(&alpha_ptr[3]);
      a5=_mm_load_si128(&alpha_ptr[5]);
      a7=_mm_load_si128(&alpha_ptr[7]);

      m_b0 = _mm_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi16(a7,*m11p);  // m11

      a0=_mm_load_si128(&alpha_ptr[0]);
      a2=_mm_load_si128(&alpha_ptr[2]);
      a4=_mm_load_si128(&alpha_ptr[4]);
      a6=_mm_load_si128(&alpha_ptr[6]);

      new0 = _mm_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm_adds_epi16(a0,*m11p);  // m11
      new1 = _mm_adds_epi16(a2,*m10p);  // m10
      new5 = _mm_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm_adds_epi16(a4,*m10p);  // m10
      new3 = _mm_adds_epi16(a6,*m11p);  // m11
      new7 = _mm_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm_max_epi16(m_b0,new0);
      a1 = _mm_max_epi16(m_b1,new1);
      a2 = _mm_max_epi16(m_b2,new2);
      a3 = _mm_max_epi16(m_b3,new3);
      a4 = _mm_max_epi16(m_b4,new4);
      a5 = _mm_max_epi16(m_b5,new5);
      a6 = _mm_max_epi16(m_b6,new6);
      a7 = _mm_max_epi16(m_b7,new7);

      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = a0;
      alpha_ptr[1] = a1;
      alpha_ptr[2] = a2;
      alpha_ptr[3] = a3;
      alpha_ptr[4] = a4;
      alpha_ptr[5] = a5;
      alpha_ptr[6] = a6;
      alpha_ptr[7] = a7;

      // 1/7
      a1=_mm_load_si128(&alpha_ptr[1]);
      a3=_mm_load_si128(&alpha_ptr[3]);
      a5=_mm_load_si128(&alpha_ptr[5]);
      a7=_mm_load_si128(&alpha_ptr[7]);

      m_b0 = _mm_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi16(a7,*m11p);  // m11

      a0=_mm_load_si128(&alpha_ptr[0]);
      a2=_mm_load_si128(&alpha_ptr[2]);
      a4=_mm_load_si128(&alpha_ptr[4]);
      a6=_mm_load_si128(&alpha_ptr[6]);

      new0 = _mm_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm_adds_epi16(a0,*m11p);  // m11
      new1 = _mm_adds_epi16(a2,*m10p);  // m10
      new5 = _mm_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm_adds_epi16(a4,*m10p);  // m10
      new3 = _mm_adds_epi16(a6,*m11p);  // m11
      new7 = _mm_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm_max_epi16(m_b0,new0);
      a1 = _mm_max_epi16(m_b1,new1);
      a2 = _mm_max_epi16(m_b2,new2);
      a3 = _mm_max_epi16(m_b3,new3);
      a4 = _mm_max_epi16(m_b4,new4);
      a5 = _mm_max_epi16(m_b5,new5);
      a6 = _mm_max_epi16(m_b6,new6);
      a7 = _mm_max_epi16(m_b7,new7);

      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = a0;
      alpha_ptr[1] = a1;
      alpha_ptr[2] = a2;
      alpha_ptr[3] = a3;
      alpha_ptr[4] = a4;
      alpha_ptr[5] = a5;
      alpha_ptr[6] = a6;
      alpha_ptr[7] = a7;

      // 2/7
      a1=_mm_load_si128(&alpha_ptr[1]);
      a3=_mm_load_si128(&alpha_ptr[3]);
      a5=_mm_load_si128(&alpha_ptr[5]);
      a7=_mm_load_si128(&alpha_ptr[7]);

      m_b0 = _mm_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi16(a7,*m11p);  // m11

      a0=_mm_load_si128(&alpha_ptr[0]);
      a2=_mm_load_si128(&alpha_ptr[2]);
      a4=_mm_load_si128(&alpha_ptr[4]);
      a6=_mm_load_si128(&alpha_ptr[6]);

      new0 = _mm_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm_adds_epi16(a0,*m11p);  // m11
      new1 = _mm_adds_epi16(a2,*m10p);  // m10
      new5 = _mm_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm_adds_epi16(a4,*m10p);  // m10
      new3 = _mm_adds_epi16(a6,*m11p);  // m11
      new7 = _mm_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm_max_epi16(m_b0,new0);
      a1 = _mm_max_epi16(m_b1,new1);
      a2 = _mm_max_epi16(m_b2,new2);
      a3 = _mm_max_epi16(m_b3,new3);
      a4 = _mm_max_epi16(m_b4,new4);
      a5 = _mm_max_epi16(m_b5,new5);
      a6 = _mm_max_epi16(m_b6,new6);
      a7 = _mm_max_epi16(m_b7,new7);

      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = a0;
      alpha_ptr[1] = a1;
      alpha_ptr[2] = a2;
      alpha_ptr[3] = a3;
      alpha_ptr[4] = a4;
      alpha_ptr[5] = a5;
      alpha_ptr[6] = a6;
      alpha_ptr[7] = a7;

      // 3/7
      a1=_mm_load_si128(&alpha_ptr[1]);
      a3=_mm_load_si128(&alpha_ptr[3]);
      a5=_mm_load_si128(&alpha_ptr[5]);
      a7=_mm_load_si128(&alpha_ptr[7]);

      m_b0 = _mm_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi16(a7,*m11p);  // m11

      a0=_mm_load_si128(&alpha_ptr[0]);
      a2=_mm_load_si128(&alpha_ptr[2]);
      a4=_mm_load_si128(&alpha_ptr[4]);
      a6=_mm_load_si128(&alpha_ptr[6]);

      new0 = _mm_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm_adds_epi16(a0,*m11p);  // m11
      new1 = _mm_adds_epi16(a2,*m10p);  // m10
      new5 = _mm_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm_adds_epi16(a4,*m10p);  // m10
      new3 = _mm_adds_epi16(a6,*m11p);  // m11
      new7 = _mm_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm_max_epi16(m_b0,new0);
      a1 = _mm_max_epi16(m_b1,new1);
      a2 = _mm_max_epi16(m_b2,new2);
      a3 = _mm_max_epi16(m_b3,new3);
      a4 = _mm_max_epi16(m_b4,new4);
      a5 = _mm_max_epi16(m_b5,new5);
      a6 = _mm_max_epi16(m_b6,new6);
      a7 = _mm_max_epi16(m_b7,new7);

      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = a0;
      alpha_ptr[1] = a1;
      alpha_ptr[2] = a2;
      alpha_ptr[3] = a3;
      alpha_ptr[4] = a4;
      alpha_ptr[5] = a5;
      alpha_ptr[6] = a6;
      alpha_ptr[7] = a7;

      // 4/7
      a1=_mm_load_si128(&alpha_ptr[1]);
      a3=_mm_load_si128(&alpha_ptr[3]);
      a5=_mm_load_si128(&alpha_ptr[5]);
      a7=_mm_load_si128(&alpha_ptr[7]);

      m_b0 = _mm_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi16(a7,*m11p);  // m11

      a0=_mm_load_si128(&alpha_ptr[0]);
      a2=_mm_load_si128(&alpha_ptr[2]);
      a4=_mm_load_si128(&alpha_ptr[4]);
      a6=_mm_load_si128(&alpha_ptr[6]);

      new0 = _mm_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm_adds_epi16(a0,*m11p);  // m11
      new1 = _mm_adds_epi16(a2,*m10p);  // m10
      new5 = _mm_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm_adds_epi16(a4,*m10p);  // m10
      new3 = _mm_adds_epi16(a6,*m11p);  // m11
      new7 = _mm_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm_max_epi16(m_b0,new0);
      a1 = _mm_max_epi16(m_b1,new1);
      a2 = _mm_max_epi16(m_b2,new2);
      a3 = _mm_max_epi16(m_b3,new3);
      a4 = _mm_max_epi16(m_b4,new4);
      a5 = _mm_max_epi16(m_b5,new5);
      a6 = _mm_max_epi16(m_b6,new6);
      a7 = _mm_max_epi16(m_b7,new7);

      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = a0;
      alpha_ptr[1] = a1;
      alpha_ptr[2] = a2;
      alpha_ptr[3] = a3;
      alpha_ptr[4] = a4;
      alpha_ptr[5] = a5;
      alpha_ptr[6] = a6;
      alpha_ptr[7] = a7;

      // 5/7
      a1=_mm_load_si128(&alpha_ptr[1]);
      a3=_mm_load_si128(&alpha_ptr[3]);
      a5=_mm_load_si128(&alpha_ptr[5]);
      a7=_mm_load_si128(&alpha_ptr[7]);

      m_b0 = _mm_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi16(a7,*m11p);  // m11

      a0=_mm_load_si128(&alpha_ptr[0]);
      a2=_mm_load_si128(&alpha_ptr[2]);
      a4=_mm_load_si128(&alpha_ptr[4]);
      a6=_mm_load_si128(&alpha_ptr[6]);

      new0 = _mm_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm_adds_epi16(a0,*m11p);  // m11
      new1 = _mm_adds_epi16(a2,*m10p);  // m10
      new5 = _mm_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm_adds_epi16(a4,*m10p);  // m10
      new3 = _mm_adds_epi16(a6,*m11p);  // m11
      new7 = _mm_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm_max_epi16(m_b0,new0);
      a1 = _mm_max_epi16(m_b1,new1);
      a2 = _mm_max_epi16(m_b2,new2);
      a3 = _mm_max_epi16(m_b3,new3);
      a4 = _mm_max_epi16(m_b4,new4);
      a5 = _mm_max_epi16(m_b5,new5);
      a6 = _mm_max_epi16(m_b6,new6);
      a7 = _mm_max_epi16(m_b7,new7);

      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = a0;
      alpha_ptr[1] = a1;
      alpha_ptr[2] = a2;
      alpha_ptr[3] = a3;
      alpha_ptr[4] = a4;
      alpha_ptr[5] = a5;
      alpha_ptr[6] = a6;
      alpha_ptr[7] = a7;

      // 6/7
      a1=_mm_load_si128(&alpha_ptr[1]);
      a3=_mm_load_si128(&alpha_ptr[3]);
      a5=_mm_load_si128(&alpha_ptr[5]);
      a7=_mm_load_si128(&alpha_ptr[7]);

      m_b0 = _mm_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi16(a7,*m11p);  // m11

      a0=_mm_load_si128(&alpha_ptr[0]);
      a2=_mm_load_si128(&alpha_ptr[2]);
      a4=_mm_load_si128(&alpha_ptr[4]);
      a6=_mm_load_si128(&alpha_ptr[6]);

      new0 = _mm_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm_adds_epi16(a0,*m11p);  // m11
      new1 = _mm_adds_epi16(a2,*m10p);  // m10
      new5 = _mm_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm_adds_epi16(a4,*m10p);  // m10
      new3 = _mm_adds_epi16(a6,*m11p);  // m11
      new7 = _mm_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm_max_epi16(m_b0,new0);
      a1 = _mm_max_epi16(m_b1,new1);
      a2 = _mm_max_epi16(m_b2,new2);
      a3 = _mm_max_epi16(m_b3,new3);
      a4 = _mm_max_epi16(m_b4,new4);
      a5 = _mm_max_epi16(m_b5,new5);
      a6 = _mm_max_epi16(m_b6,new6);
      a7 = _mm_max_epi16(m_b7,new7);

      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = a0;
      alpha_ptr[1] = a1;
      alpha_ptr[2] = a2;
      alpha_ptr[3] = a3;
      alpha_ptr[4] = a4;
      alpha_ptr[5] = a5;
      alpha_ptr[6] = a6;
      alpha_ptr[7] = a7;

    // 7/7
      a1=_mm_load_si128(&alpha_ptr[1]);
      a3=_mm_load_si128(&alpha_ptr[3]);
      a5=_mm_load_si128(&alpha_ptr[5]);
      a7=_mm_load_si128(&alpha_ptr[7]);

      m_b0 = _mm_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi16(a7,*m11p);  // m11

      a0=_mm_load_si128(&alpha_ptr[0]);
      a2=_mm_load_si128(&alpha_ptr[2]);
      a4=_mm_load_si128(&alpha_ptr[4]);
      a6=_mm_load_si128(&alpha_ptr[6]);

      new0 = _mm_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm_adds_epi16(a0,*m11p);  // m11
      new1 = _mm_adds_epi16(a2,*m10p);  // m10
      new5 = _mm_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm_adds_epi16(a4,*m10p);  // m10
      new3 = _mm_adds_epi16(a6,*m11p);  // m11
      new7 = _mm_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm_max_epi16(m_b0,new0);
      a1 = _mm_max_epi16(m_b1,new1);
      a2 = _mm_max_epi16(m_b2,new2);
      a3 = _mm_max_epi16(m_b3,new3);
      a4 = _mm_max_epi16(m_b4,new4);
      a5 = _mm_max_epi16(m_b5,new5);
      a6 = _mm_max_epi16(m_b6,new6);
      a7 = _mm_max_epi16(m_b7,new7);
    // compute and subtract maxima
      alpha_max = _mm_max_epi16(a0,a1);
      alpha_max = _mm_max_epi16(alpha_max,a2);
      alpha_max = _mm_max_epi16(alpha_max,a3);
      alpha_max = _mm_max_epi16(alpha_max,a4);
      alpha_max = _mm_max_epi16(alpha_max,a5);
      alpha_max = _mm_max_epi16(alpha_max,a6);
      alpha_max = _mm_max_epi16(alpha_max,a7);
      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = _mm_subs_epi16(a0,alpha_max);
      alpha_ptr[1] = _mm_subs_epi16(a1,alpha_max);
      alpha_ptr[2] = _mm_subs_epi16(a2,alpha_max);
      alpha_ptr[3] = _mm_subs_epi16(a3,alpha_max);
      alpha_ptr[4] = _mm_subs_epi16(a4,alpha_max);
      alpha_ptr[5] = _mm_subs_epi16(a5,alpha_max);
      alpha_ptr[6] = _mm_subs_epi16(a6,alpha_max);
      alpha_ptr[7] = _mm_subs_epi16(a7,alpha_max);
    }
    // fill in remainder
    for (k=0;
    k<(l-((l>>3)<<3));
    k++){


      a1=_mm_load_si128(&alpha_ptr[1]);
      a3=_mm_load_si128(&alpha_ptr[3]);
      a5=_mm_load_si128(&alpha_ptr[5]);
      a7=_mm_load_si128(&alpha_ptr[7]);

      m_b0 = _mm_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm_adds_epi16(a7,*m11p);  // m11

      a0=_mm_load_si128(&alpha_ptr[0]);
      a2=_mm_load_si128(&alpha_ptr[2]);
      a4=_mm_load_si128(&alpha_ptr[4]);
      a6=_mm_load_si128(&alpha_ptr[6]);

      new0 = _mm_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm_adds_epi16(a0,*m11p);  // m11
      new1 = _mm_adds_epi16(a2,*m10p);  // m10
      new5 = _mm_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm_adds_epi16(a4,*m10p);  // m10
      new3 = _mm_adds_epi16(a6,*m11p);  // m11
      new7 = _mm_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm_max_epi16(m_b0,new0);
      a1 = _mm_max_epi16(m_b1,new1);
      a2 = _mm_max_epi16(m_b2,new2);
      a3 = _mm_max_epi16(m_b3,new3);
      a4 = _mm_max_epi16(m_b4,new4);
      a5 = _mm_max_epi16(m_b5,new5);
      a6 = _mm_max_epi16(m_b6,new6);
      a7 = _mm_max_epi16(m_b7,new7);

      alpha_ptr += 8;
      m11p++;
      m10p++;
      alpha_ptr[0] = a0;
      alpha_ptr[1] = a1;
      alpha_ptr[2] = a2;
      alpha_ptr[3] = a3;
      alpha_ptr[4] = a4;
      alpha_ptr[5] = a5;
      alpha_ptr[6] = a6;
      alpha_ptr[7] = a7;
    }
    */
#else


    if (rerun_flag == 0) {

      alpha128[0] = _mm_set_epi8(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,0);
      alpha128[1] = _mm_set_epi8(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[2] = _mm_set_epi8(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[3] = _mm_set_epi8(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[4] = _mm_set_epi8(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[5] = _mm_set_epi8(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[6] = _mm_set_epi8(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[7] = _mm_set_epi8(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);


    } else {
      alpha128[0] = _mm_slli_si128(alpha128[(K1<<3)],1);
      alpha128[1] = _mm_slli_si128(alpha128[1+(K1<<3)],1);
      alpha128[2] = _mm_slli_si128(alpha128[2+(K1<<3)],1);
      alpha128[3] = _mm_slli_si128(alpha128[3+(K1<<3)],1);
      alpha128[4] = _mm_slli_si128(alpha128[4+(K1<<3)],1);
      alpha128[5] = _mm_slli_si128(alpha128[5+(K1<<3)],1);
      alpha128[6] = _mm_slli_si128(alpha128[6+(K1<<3)],1);
      alpha128[7] = _mm_slli_si128(alpha128[7+(K1<<3)],1);
      alpha[16] =  -MAX/2;
      alpha[32] = -MAX/2;
      alpha[48] = -MAX/2;
      alpha[64] = -MAX/2;
      alpha[80] = -MAX/2;
      alpha[96] = -MAX/2;
      alpha[112] = -MAX/2;
    }

    alpha_ptr = &alpha128[0];
    /*
    printf("alpha k %d (%d) (%p)\n",0,0,alpha_ptr);

    print_bytes("a0:",&alpha_ptr[0]);
    print_bytes("a1:",&alpha_ptr[1]);
    print_bytes("a2:",&alpha_ptr[2]);
    print_bytes("a3:",&alpha_ptr[3]);
    print_bytes("a4:",&alpha_ptr[4]);
    print_bytes("a5:",&alpha_ptr[5]);
    print_bytes("a6:",&alpha_ptr[6]);
    print_bytes("a7:",&alpha_ptr[7]);
    */

    m11p = (__m128i*)m_11;
    m10p = (__m128i*)m_10;

    for (k=0;
         k<l;
         k++) {


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
      /*
      printf("alpha k %d (%d) (%p)\n",k+1,(k+1)<<4,alpha_ptr);

      print_bytes("m11:",&m11_0);
      print_bytes("m10:",&m10_0);
      print_bytes("a0:",&alpha_ptr[0]);
      print_bytes("a1:",&alpha_ptr[1]);
      print_bytes("a2:",&alpha_ptr[2]);
      print_bytes("a3:",&alpha_ptr[3]);
      print_bytes("a4:",&alpha_ptr[4]);
      print_bytes("a5:",&alpha_ptr[5]);
      print_bytes("a6:",&alpha_ptr[6]);
      print_bytes("a7:",&alpha_ptr[7]);
      */
    }

#endif

    if (rerun_flag==1)
      break;
  }

  _mm_empty();
  _m_empty();
}


void compute_beta(llr_t* alpha,llr_t* beta,llr_t *m_11,llr_t* m_10,unsigned short frame_length,unsigned char F,int offset8_flag)
{

  int k,rerun_flag=0;
  __m128i m11_128,m10_128;
  __m128i m_b0,m_b1,m_b2,m_b3,m_b4,m_b5,m_b6,m_b7;
  __m128i new0,new1,new2,new3,new4,new5,new6,new7;

  __m128i *beta128,*alpha128,*beta_ptr;
  __m128i beta_max;
  int16_t m11,m10,beta0_16,beta1_16,beta2_16,beta3_16,beta4_16,beta5_16,beta6_16,beta7_16,beta0_2,beta1_2,beta2_2,beta3_2,beta_m;
  llr_t beta0,beta1;
#ifdef LLR8
  llr_t beta2,beta3,beta4,beta5,beta6,beta7;
  __m128i beta_16;

#endif

#ifdef DEBUG_LOGMAP
  msg("compute_beta, %p,%p,%p,%p,framelength %d,F %d\n",
      beta,m_11,m_10,alpha,frame_length,F);
#endif


  // termination for beta initialization

  //  printf("beta init: offset8 %d\n",offset8_flag);
  m11=(int16_t)m_11[2+frame_length];
  m10=(int16_t)m_10[2+frame_length];

  //  printf("m11,m10 %d,%d\n",m11,m10);

  beta0 = -m11;//M0T_TERM;
  beta1 = m11;//M1T_TERM;
  m11=(int16_t)m_11[1+frame_length];
  m10=(int16_t)m_10[1+frame_length];

  //  printf("m11,m10 %d,%d\n",m11,m10);

  beta0_2 = beta0-m11;//+M0T_TERM;
  beta1_2 = beta0+m11;//+M1T_TERM;
  beta2_2 = beta1+m10;//M2T_TERM;
  beta3_2 = beta1-m10;//+M3T_TERM;
  m11=(int16_t)m_11[frame_length];
  m10=(int16_t)m_10[frame_length];
  //  printf("m11,m10 %d,%d (%p)\n",m11,m10,m_11+frame_length);

  beta0_16 = beta0_2-m11;//+M0T_TERM;
  beta1_16 = beta0_2+m11;//+M1T_TERM;
  beta2_16 = beta1_2+m10;//+M2T_TERM;
  beta3_16 = beta1_2-m10;//+M3T_TERM;
  beta4_16 = beta2_2-m10;//+M4T_TERM;
  beta5_16 = beta2_2+m10;//+M5T_TERM;
  beta6_16 = beta3_2+m11;//+M6T_TERM;
  beta7_16 = beta3_2-m11;//+M7T_TERM;


  beta_m = (beta0_16>beta1_16) ? beta0_16 : beta1_16;
  beta_m = (beta_m>beta2_16) ? beta_m : beta2_16;
  beta_m = (beta_m>beta3_16) ? beta_m : beta3_16;
  beta_m = (beta_m>beta4_16) ? beta_m : beta4_16;
  beta_m = (beta_m>beta5_16) ? beta_m : beta5_16;
  beta_m = (beta_m>beta6_16) ? beta_m : beta6_16;
  beta_m = (beta_m>beta7_16) ? beta_m : beta7_16;


  beta0_16=beta0_16-beta_m;
  beta1_16=beta1_16-beta_m;
  beta2_16=beta2_16-beta_m;
  beta3_16=beta3_16-beta_m;
  beta4_16=beta4_16-beta_m;
  beta5_16=beta5_16-beta_m;
  beta6_16=beta6_16-beta_m;
  beta7_16=beta7_16-beta_m;

#ifdef LLR8
  beta_16 = _mm_set_epi16(beta7_16,beta6_16,beta5_16,beta4_16,beta3_16,beta2_16,beta1_16,beta0_16);
  beta_16 = _mm_packs_epi16(beta_16,beta_16);
  beta0 = _mm_extract_epi8(beta_16,0);
  beta1 = _mm_extract_epi8(beta_16,1);
  beta2 = _mm_extract_epi8(beta_16,2);
  beta3 = _mm_extract_epi8(beta_16,3);
  beta4 = _mm_extract_epi8(beta_16,4);
  beta5 = _mm_extract_epi8(beta_16,5);
  beta6 = _mm_extract_epi8(beta_16,6);
  beta7 = _mm_extract_epi8(beta_16,7);
  //  printf("beta (init)    : %d,%d,%d,%d,%d,%d,%d,%d\n",(uint8_t)beta0,(uint8_t)beta1,(uint8_t)beta2,(uint8_t)beta3,(uint8_t)beta4,(uint8_t)beta5,(uint8_t)beta6,(uint8_t)beta7);
#endif

  for (rerun_flag=0;; rerun_flag=1) {
    beta_ptr   = (__m128i*)&beta[frame_length<<3];
    alpha128   = (__m128i*)&alpha[0];

    if (rerun_flag == 0) {
#ifndef LLR8
      beta_ptr[0] = alpha128[(frame_length)];
      beta_ptr[1] = alpha128[1+(frame_length)];
      beta_ptr[2] = alpha128[2+(frame_length)];
      beta_ptr[3] = alpha128[3+(frame_length)];
      beta_ptr[4] = alpha128[4+(frame_length)];
      beta_ptr[5] = alpha128[5+(frame_length)];
      beta_ptr[6] = alpha128[6+(frame_length)];
      beta_ptr[7] = alpha128[7+(frame_length)];
#else
      beta_ptr[0] = alpha128[(frame_length>>1)];
      beta_ptr[1] = alpha128[1+(frame_length>>1)];
      beta_ptr[2] = alpha128[2+(frame_length>>1)];
      beta_ptr[3] = alpha128[3+(frame_length>>1)];
      beta_ptr[4] = alpha128[4+(frame_length>>1)];
      beta_ptr[5] = alpha128[5+(frame_length>>1)];
      beta_ptr[6] = alpha128[6+(frame_length>>1)];
      beta_ptr[7] = alpha128[7+(frame_length>>1)];
#endif
    } else {
      beta128 = (__m128i*)&beta[0];
#ifndef LLR8

      beta_ptr[0] = _mm_srli_si128(beta128[0],2);
      beta_ptr[1] = _mm_srli_si128(beta128[1],2);
      beta_ptr[2] = _mm_srli_si128(beta128[2],2);
      beta_ptr[3] = _mm_srli_si128(beta128[3],2);
      beta_ptr[4] = _mm_srli_si128(beta128[4],2);
      beta_ptr[5] = _mm_srli_si128(beta128[5],2);
      beta_ptr[6] = _mm_srli_si128(beta128[6],2);
      beta_ptr[7] = _mm_srli_si128(beta128[7],2);
#else
      beta_ptr[0] = _mm_srli_si128(beta128[0],1);
      beta_ptr[1] = _mm_srli_si128(beta128[1],1);
      beta_ptr[2] = _mm_srli_si128(beta128[2],1);
      beta_ptr[3] = _mm_srli_si128(beta128[3],1);
      beta_ptr[4] = _mm_srli_si128(beta128[4],1);
      beta_ptr[5] = _mm_srli_si128(beta128[5],1);
      beta_ptr[6] = _mm_srli_si128(beta128[6],1);
      beta_ptr[7] = _mm_srli_si128(beta128[7],1);
#endif
    }

#ifndef LLR8
    beta_ptr[0] = _mm_insert_epi16(beta_ptr[0],beta0_16,7);
    beta_ptr[1] = _mm_insert_epi16(beta_ptr[1],beta1_16,7);
    beta_ptr[2] = _mm_insert_epi16(beta_ptr[2],beta2_16,7);
    beta_ptr[3] = _mm_insert_epi16(beta_ptr[3],beta3_16,7);
    beta_ptr[4] = _mm_insert_epi16(beta_ptr[4],beta4_16,7);
    beta_ptr[5] = _mm_insert_epi16(beta_ptr[5],beta5_16,7);
    beta_ptr[6] = _mm_insert_epi16(beta_ptr[6],beta6_16,7);
    beta_ptr[7] = _mm_insert_epi16(beta_ptr[7],beta7_16,7);

    /*
      beta[7+(frame_length<<3)] = beta0_16;
      beta[15+(frame_length<<3)] = beta1_16;
      beta[23+(frame_length<<3)] = beta2_16;
      beta[31+(frame_length<<3)] = beta3_16;
      beta[39+(frame_length<<3)] = beta4_16;
      beta[47+(frame_length<<3)] = beta5_16;
      beta[55+(frame_length<<3)] = beta6_16;
      beta[63+(frame_length<<3)] = beta7_16;*/
#else

    if (offset8_flag==0) {
      beta_ptr[0] = _mm_insert_epi8(beta_ptr[0],beta0,15);
      beta_ptr[1] = _mm_insert_epi8(beta_ptr[1],beta1,15);
      beta_ptr[2] = _mm_insert_epi8(beta_ptr[2],beta2,15);
      beta_ptr[3] = _mm_insert_epi8(beta_ptr[3],beta3,15);
      beta_ptr[4] = _mm_insert_epi8(beta_ptr[4],beta4,15);
      beta_ptr[5] = _mm_insert_epi8(beta_ptr[5],beta5,15);
      beta_ptr[6] = _mm_insert_epi8(beta_ptr[6],beta6,15);
      beta_ptr[7] = _mm_insert_epi8(beta_ptr[7],beta7,15);
    } else {

    }

#endif

#ifndef LLR8
    int loopval=((rerun_flag==0)?0:((frame_length-L)>>3));

    for (k=(frame_length>>3)-1; k>=loopval; k--) {
      m11_128=((__m128i*)m_11)[k];
      m10_128=((__m128i*)m_10)[k];

      m_b0 = _mm_adds_epi16(beta_ptr[4],m11_128);  //m11
      m_b1 = _mm_subs_epi16(beta_ptr[4],m11_128);  //m00
      m_b2 = _mm_subs_epi16(beta_ptr[5],m10_128);  //m01
      m_b3 = _mm_adds_epi16(beta_ptr[5],m10_128);  //m10
      m_b4 = _mm_adds_epi16(beta_ptr[6],m10_128);  //m10
      m_b5 = _mm_subs_epi16(beta_ptr[6],m10_128);  //m01
      m_b6 = _mm_subs_epi16(beta_ptr[7],m11_128);  //m00
      m_b7 = _mm_adds_epi16(beta_ptr[7],m11_128);  //m11

      new0 = _mm_subs_epi16(beta_ptr[0],m11_128);  //m00
      new1 = _mm_adds_epi16(beta_ptr[0],m11_128);  //m11
      new2 = _mm_adds_epi16(beta_ptr[1],m10_128);  //m10
      new3 = _mm_subs_epi16(beta_ptr[1],m10_128);  //m01
      new4 = _mm_subs_epi16(beta_ptr[2],m10_128);  //m01
      new5 = _mm_adds_epi16(beta_ptr[2],m10_128);  //m10
      new6 = _mm_adds_epi16(beta_ptr[3],m11_128);  //m11
      new7 = _mm_subs_epi16(beta_ptr[3],m11_128);  //m00

      beta_ptr-=8;

      beta_ptr[0] = _mm_max_epi16(m_b0,new0);
      beta_ptr[1] = _mm_max_epi16(m_b1,new1);
      beta_ptr[2] = _mm_max_epi16(m_b2,new2);
      beta_ptr[3] = _mm_max_epi16(m_b3,new3);
      beta_ptr[4] = _mm_max_epi16(m_b4,new4);
      beta_ptr[5] = _mm_max_epi16(m_b5,new5);
      beta_ptr[6] = _mm_max_epi16(m_b6,new6);
      beta_ptr[7] = _mm_max_epi16(m_b7,new7);

      beta_max = _mm_max_epi16(beta_ptr[0],beta_ptr[1]);
      beta_max = _mm_max_epi16(beta_max   ,beta_ptr[2]);
      beta_max = _mm_max_epi16(beta_max   ,beta_ptr[3]);
      beta_max = _mm_max_epi16(beta_max   ,beta_ptr[4]);
      beta_max = _mm_max_epi16(beta_max   ,beta_ptr[5]);
      beta_max = _mm_max_epi16(beta_max   ,beta_ptr[6]);
      beta_max = _mm_max_epi16(beta_max   ,beta_ptr[7]);

      beta_ptr[0] = _mm_subs_epi16(beta_ptr[0],beta_max);
      beta_ptr[1] = _mm_subs_epi16(beta_ptr[1],beta_max);
      beta_ptr[2] = _mm_subs_epi16(beta_ptr[2],beta_max);
      beta_ptr[3] = _mm_subs_epi16(beta_ptr[3],beta_max);
      beta_ptr[4] = _mm_subs_epi16(beta_ptr[4],beta_max);
      beta_ptr[5] = _mm_subs_epi16(beta_ptr[5],beta_max);
      beta_ptr[6] = _mm_subs_epi16(beta_ptr[6],beta_max);
      beta_ptr[7] = _mm_subs_epi16(beta_ptr[7],beta_max);



    }

#else
#ifdef DEBUG_LOGMAP
    printf("beta0 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
           (frame_length>>4),
           _mm_extract_epi8(beta_ptr[0],0),
           _mm_extract_epi8(beta_ptr[0],1),
           _mm_extract_epi8(beta_ptr[0],2),
           _mm_extract_epi8(beta_ptr[0],3),
           _mm_extract_epi8(beta_ptr[0],4),
           _mm_extract_epi8(beta_ptr[0],5),
           _mm_extract_epi8(beta_ptr[0],6),
           _mm_extract_epi8(beta_ptr[0],7),
           _mm_extract_epi8(beta_ptr[0],8),
           _mm_extract_epi8(beta_ptr[0],9),
           _mm_extract_epi8(beta_ptr[0],10),
           _mm_extract_epi8(beta_ptr[0],11),
           _mm_extract_epi8(beta_ptr[0],12),
           _mm_extract_epi8(beta_ptr[0],13),
           _mm_extract_epi8(beta_ptr[0],14),
           _mm_extract_epi8(beta_ptr[0],15));
    printf("beta1 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
           (frame_length>>4),
           _mm_extract_epi8(beta_ptr[1],0),
           _mm_extract_epi8(beta_ptr[1],1),
           _mm_extract_epi8(beta_ptr[1],2),
           _mm_extract_epi8(beta_ptr[1],3),
           _mm_extract_epi8(beta_ptr[1],4),
           _mm_extract_epi8(beta_ptr[1],5),
           _mm_extract_epi8(beta_ptr[1],6),
           _mm_extract_epi8(beta_ptr[1],7),
           _mm_extract_epi8(beta_ptr[1],8),
           _mm_extract_epi8(beta_ptr[1],9),
           _mm_extract_epi8(beta_ptr[1],10),
           _mm_extract_epi8(beta_ptr[1],11),
           _mm_extract_epi8(beta_ptr[1],12),
           _mm_extract_epi8(beta_ptr[1],13),
           _mm_extract_epi8(beta_ptr[1],14),
           _mm_extract_epi8(beta_ptr[1],15));
    printf("beta2 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
           (frame_length>>4),
           _mm_extract_epi8(beta_ptr[2],0),
           _mm_extract_epi8(beta_ptr[2],1),
           _mm_extract_epi8(beta_ptr[2],2),
           _mm_extract_epi8(beta_ptr[2],3),
           _mm_extract_epi8(beta_ptr[2],4),
           _mm_extract_epi8(beta_ptr[2],5),
           _mm_extract_epi8(beta_ptr[2],6),
           _mm_extract_epi8(beta_ptr[2],7),
           _mm_extract_epi8(beta_ptr[2],8),
           _mm_extract_epi8(beta_ptr[2],9),
           _mm_extract_epi8(beta_ptr[2],10),
           _mm_extract_epi8(beta_ptr[2],11),
           _mm_extract_epi8(beta_ptr[2],12),
           _mm_extract_epi8(beta_ptr[2],13),
           _mm_extract_epi8(beta_ptr[2],14),
           _mm_extract_epi8(beta_ptr[2],15));
    printf("beta3 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
           (frame_length>>4),
           _mm_extract_epi8(beta_ptr[3],0),
           _mm_extract_epi8(beta_ptr[3],1),
           _mm_extract_epi8(beta_ptr[3],2),
           _mm_extract_epi8(beta_ptr[3],3),
           _mm_extract_epi8(beta_ptr[3],4),
           _mm_extract_epi8(beta_ptr[3],5),
           _mm_extract_epi8(beta_ptr[3],6),
           _mm_extract_epi8(beta_ptr[3],7),
           _mm_extract_epi8(beta_ptr[3],8),
           _mm_extract_epi8(beta_ptr[3],9),
           _mm_extract_epi8(beta_ptr[3],10),
           _mm_extract_epi8(beta_ptr[3],11),
           _mm_extract_epi8(beta_ptr[3],12),
           _mm_extract_epi8(beta_ptr[3],13),
           _mm_extract_epi8(beta_ptr[3],14),
           _mm_extract_epi8(beta_ptr[3],15));
    printf("beta4 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
           (frame_length>>4),
           _mm_extract_epi8(beta_ptr[4],0),
           _mm_extract_epi8(beta_ptr[4],1),
           _mm_extract_epi8(beta_ptr[4],2),
           _mm_extract_epi8(beta_ptr[4],3),
           _mm_extract_epi8(beta_ptr[4],4),
           _mm_extract_epi8(beta_ptr[4],5),
           _mm_extract_epi8(beta_ptr[4],6),
           _mm_extract_epi8(beta_ptr[4],7),
           _mm_extract_epi8(beta_ptr[4],8),
           _mm_extract_epi8(beta_ptr[4],9),
           _mm_extract_epi8(beta_ptr[4],10),
           _mm_extract_epi8(beta_ptr[4],11),
           _mm_extract_epi8(beta_ptr[4],12),
           _mm_extract_epi8(beta_ptr[4],13),
           _mm_extract_epi8(beta_ptr[4],14),
           _mm_extract_epi8(beta_ptr[4],15));
    printf("beta5 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
           (frame_length>>4),
           _mm_extract_epi8(beta_ptr[5],0),
           _mm_extract_epi8(beta_ptr[5],1),
           _mm_extract_epi8(beta_ptr[5],2),
           _mm_extract_epi8(beta_ptr[5],3),
           _mm_extract_epi8(beta_ptr[5],4),
           _mm_extract_epi8(beta_ptr[5],5),
           _mm_extract_epi8(beta_ptr[5],6),
           _mm_extract_epi8(beta_ptr[5],7),
           _mm_extract_epi8(beta_ptr[5],8),
           _mm_extract_epi8(beta_ptr[5],9),
           _mm_extract_epi8(beta_ptr[5],10),
           _mm_extract_epi8(beta_ptr[5],11),
           _mm_extract_epi8(beta_ptr[5],12),
           _mm_extract_epi8(beta_ptr[5],13),
           _mm_extract_epi8(beta_ptr[5],14),
           _mm_extract_epi8(beta_ptr[5],15));
    printf("beta6 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
           (frame_length>>4),
           _mm_extract_epi8(beta_ptr[6],0),
           _mm_extract_epi8(beta_ptr[6],1),
           _mm_extract_epi8(beta_ptr[6],2),
           _mm_extract_epi8(beta_ptr[6],3),
           _mm_extract_epi8(beta_ptr[6],4),
           _mm_extract_epi8(beta_ptr[6],5),
           _mm_extract_epi8(beta_ptr[6],6),
           _mm_extract_epi8(beta_ptr[6],7),
           _mm_extract_epi8(beta_ptr[6],8),
           _mm_extract_epi8(beta_ptr[6],9),
           _mm_extract_epi8(beta_ptr[6],10),
           _mm_extract_epi8(beta_ptr[6],11),
           _mm_extract_epi8(beta_ptr[6],12),
           _mm_extract_epi8(beta_ptr[6],13),
           _mm_extract_epi8(beta_ptr[6],14),
           _mm_extract_epi8(beta_ptr[6],15));
    printf("beta7 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
           (frame_length>>4),
           _mm_extract_epi8(beta_ptr[7],0),
           _mm_extract_epi8(beta_ptr[7],1),
           _mm_extract_epi8(beta_ptr[7],2),
           _mm_extract_epi8(beta_ptr[7],3),
           _mm_extract_epi8(beta_ptr[7],4),
           _mm_extract_epi8(beta_ptr[7],5),
           _mm_extract_epi8(beta_ptr[7],6),
           _mm_extract_epi8(beta_ptr[7],7),
           _mm_extract_epi8(beta_ptr[7],8),
           _mm_extract_epi8(beta_ptr[7],9),
           _mm_extract_epi8(beta_ptr[7],10),
           _mm_extract_epi8(beta_ptr[7],11),
           _mm_extract_epi8(beta_ptr[7],12),
           _mm_extract_epi8(beta_ptr[7],13),
           _mm_extract_epi8(beta_ptr[7],14),
           _mm_extract_epi8(beta_ptr[7],15));
#endif
    int loopval=(rerun_flag==0)?0:((frame_length-L)>>4);
    __m128i max_val=_mm_set1_epi8(32);
    __m128i zeros=_mm_set1_epi8(0);

    for (k=(frame_length>>4)-1; k>=loopval; k--) {

      m11_128=((__m128i*)m_11)[k];
      m10_128=((__m128i*)m_10)[k];
      /*
      if ((offset8_flag==1) && (k==((frame_length>>4)-9))) {
      beta_ptr[0] = _mm_insert_epi8(beta_ptr[0],beta0,15);
      beta_ptr[1] = _mm_insert_epi8(beta_ptr[1],beta1,15);
      beta_ptr[2] = _mm_insert_epi8(beta_ptr[2],beta2,15);
      beta_ptr[3] = _mm_insert_epi8(beta_ptr[3],beta3,15);
      beta_ptr[4] = _mm_insert_epi8(beta_ptr[4],beta4,15);
      beta_ptr[5] = _mm_insert_epi8(beta_ptr[5],beta5,15);
      beta_ptr[6] = _mm_insert_epi8(beta_ptr[6],beta6,15);
      beta_ptr[7] = _mm_insert_epi8(beta_ptr[7],beta7,15);
      }*/
      //      print_bytes("m11:",&m11_128);



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

      /*
      printf("beta0 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
      k,
      _mm_extract_epi8(beta_ptr[0],0),
      _mm_extract_epi8(beta_ptr[0],1),
      _mm_extract_epi8(beta_ptr[0],2),
      _mm_extract_epi8(beta_ptr[0],3),
      _mm_extract_epi8(beta_ptr[0],4),
      _mm_extract_epi8(beta_ptr[0],5),
      _mm_extract_epi8(beta_ptr[0],6),
      _mm_extract_epi8(beta_ptr[0],7),
      _mm_extract_epi8(beta_ptr[0],8),
      _mm_extract_epi8(beta_ptr[0],9),
      _mm_extract_epi8(beta_ptr[0],10),
      _mm_extract_epi8(beta_ptr[0],11),
      _mm_extract_epi8(beta_ptr[0],12),
      _mm_extract_epi8(beta_ptr[0],13),
      _mm_extract_epi8(beta_ptr[0],14),
      _mm_extract_epi8(beta_ptr[0],15));
      printf("beta1 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
      k,
      _mm_extract_epi8(beta_ptr[1],0),
      _mm_extract_epi8(beta_ptr[1],1),
      _mm_extract_epi8(beta_ptr[1],2),
      _mm_extract_epi8(beta_ptr[1],3),
      _mm_extract_epi8(beta_ptr[1],4),
      _mm_extract_epi8(beta_ptr[1],5),
      _mm_extract_epi8(beta_ptr[1],6),
      _mm_extract_epi8(beta_ptr[1],7),
      _mm_extract_epi8(beta_ptr[1],8),
      _mm_extract_epi8(beta_ptr[1],9),
      _mm_extract_epi8(beta_ptr[1],10),
      _mm_extract_epi8(beta_ptr[1],11),
      _mm_extract_epi8(beta_ptr[1],12),
      _mm_extract_epi8(beta_ptr[1],13),
      _mm_extract_epi8(beta_ptr[1],14),
      _mm_extract_epi8(beta_ptr[1],15));
      printf("beta2 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
      k,
      _mm_extract_epi8(beta_ptr[2],0),
      _mm_extract_epi8(beta_ptr[2],1),
      _mm_extract_epi8(beta_ptr[2],2),
      _mm_extract_epi8(beta_ptr[2],3),
      _mm_extract_epi8(beta_ptr[2],4),
      _mm_extract_epi8(beta_ptr[2],5),
      _mm_extract_epi8(beta_ptr[2],6),
      _mm_extract_epi8(beta_ptr[2],7),
      _mm_extract_epi8(beta_ptr[2],8),
      _mm_extract_epi8(beta_ptr[2],9),
      _mm_extract_epi8(beta_ptr[2],10),
      _mm_extract_epi8(beta_ptr[2],11),
      _mm_extract_epi8(beta_ptr[2],12),
      _mm_extract_epi8(beta_ptr[2],13),
      _mm_extract_epi8(beta_ptr[2],14),
      _mm_extract_epi8(beta_ptr[2],15));
      printf("beta3 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
      k,
      _mm_extract_epi8(beta_ptr[3],0),
      _mm_extract_epi8(beta_ptr[3],1),
      _mm_extract_epi8(beta_ptr[3],2),
      _mm_extract_epi8(beta_ptr[3],3),
      _mm_extract_epi8(beta_ptr[3],4),
      _mm_extract_epi8(beta_ptr[3],5),
      _mm_extract_epi8(beta_ptr[3],6),
      _mm_extract_epi8(beta_ptr[3],7),
      _mm_extract_epi8(beta_ptr[3],8),
      _mm_extract_epi8(beta_ptr[3],9),
      _mm_extract_epi8(beta_ptr[3],10),
      _mm_extract_epi8(beta_ptr[3],11),
      _mm_extract_epi8(beta_ptr[3],12),
      _mm_extract_epi8(beta_ptr[3],13),
      _mm_extract_epi8(beta_ptr[3],14),
      _mm_extract_epi8(beta_ptr[3],15));
      printf("beta4 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
      k,
      _mm_extract_epi8(beta_ptr[4],0),
      _mm_extract_epi8(beta_ptr[4],1),
      _mm_extract_epi8(beta_ptr[4],2),
      _mm_extract_epi8(beta_ptr[4],3),
      _mm_extract_epi8(beta_ptr[4],4),
      _mm_extract_epi8(beta_ptr[4],5),
      _mm_extract_epi8(beta_ptr[4],6),
      _mm_extract_epi8(beta_ptr[4],7),
      _mm_extract_epi8(beta_ptr[4],8),
      _mm_extract_epi8(beta_ptr[4],9),
      _mm_extract_epi8(beta_ptr[4],10),
      _mm_extract_epi8(beta_ptr[4],11),
      _mm_extract_epi8(beta_ptr[4],12),
      _mm_extract_epi8(beta_ptr[4],13),
      _mm_extract_epi8(beta_ptr[4],14),
      _mm_extract_epi8(beta_ptr[4],15));
      printf("beta5 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
      k,
      _mm_extract_epi8(beta_ptr[5],0),
      _mm_extract_epi8(beta_ptr[5],1),
      _mm_extract_epi8(beta_ptr[5],2),
      _mm_extract_epi8(beta_ptr[5],3),
      _mm_extract_epi8(beta_ptr[5],4),
      _mm_extract_epi8(beta_ptr[5],5),
      _mm_extract_epi8(beta_ptr[5],6),
      _mm_extract_epi8(beta_ptr[5],7),
      _mm_extract_epi8(beta_ptr[5],8),
      _mm_extract_epi8(beta_ptr[5],9),
      _mm_extract_epi8(beta_ptr[5],10),
      _mm_extract_epi8(beta_ptr[5],11),
      _mm_extract_epi8(beta_ptr[5],12),
      _mm_extract_epi8(beta_ptr[5],13),
      _mm_extract_epi8(beta_ptr[5],14),
      _mm_extract_epi8(beta_ptr[5],15));
      printf("beta6 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
      k,
      _mm_extract_epi8(beta_ptr[6],0),
      _mm_extract_epi8(beta_ptr[6],1),
      _mm_extract_epi8(beta_ptr[6],2),
      _mm_extract_epi8(beta_ptr[6],3),
      _mm_extract_epi8(beta_ptr[6],4),
      _mm_extract_epi8(beta_ptr[6],5),
      _mm_extract_epi8(beta_ptr[6],6),
      _mm_extract_epi8(beta_ptr[6],7),
      _mm_extract_epi8(beta_ptr[6],8),
      _mm_extract_epi8(beta_ptr[6],9),
      _mm_extract_epi8(beta_ptr[6],10),
      _mm_extract_epi8(beta_ptr[6],11),
      _mm_extract_epi8(beta_ptr[6],12),
      _mm_extract_epi8(beta_ptr[6],13),
      _mm_extract_epi8(beta_ptr[6],14),
      _mm_extract_epi8(beta_ptr[6],15));
      printf("beta7 %d:  %03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d,%03d\n",
      k,
      _mm_extract_epi8(beta_ptr[7],0),
      _mm_extract_epi8(beta_ptr[7],1),
      _mm_extract_epi8(beta_ptr[7],2),
      _mm_extract_epi8(beta_ptr[7],3),
      _mm_extract_epi8(beta_ptr[7],4),
      _mm_extract_epi8(beta_ptr[7],5),
      _mm_extract_epi8(beta_ptr[7],6),
      _mm_extract_epi8(beta_ptr[7],7),
      _mm_extract_epi8(beta_ptr[7],8),
      _mm_extract_epi8(beta_ptr[7],9),
      _mm_extract_epi8(beta_ptr[7],10),
      _mm_extract_epi8(beta_ptr[7],11),
      _mm_extract_epi8(beta_ptr[7],12),
      _mm_extract_epi8(beta_ptr[7],13),
      _mm_extract_epi8(beta_ptr[7],14),
      _mm_extract_epi8(beta_ptr[7],15));
      */

    }

#endif

    if (rerun_flag==1)
      break;
  }

  _mm_empty();
  _m_empty();
}

void compute_ext(llr_t* alpha,llr_t* beta,llr_t* m_11,llr_t* m_10,llr_t* ext, llr_t* systematic,unsigned short frame_length)
{
  __m128i *alpha128=(__m128i *)alpha;
  __m128i *beta128=(__m128i *)beta;
  __m128i *m11_128,*m10_128,*ext_128;
  __m128i *alpha_ptr,*beta_ptr;
  __m128i m00_1,m00_2,m00_3,m00_4;
  __m128i m01_1,m01_2,m01_3,m01_4;
  __m128i m10_1,m10_2,m10_3,m10_4;
  __m128i m11_1,m11_2,m11_3,m11_4;
  int k;

  //
  // LLR computation, 8 consequtive bits per loop
  //

#ifdef DEBUG_LOGMAP
  msg("compute_ext, %p, %p, %p, %p, %p, %p ,framelength %d\n",alpha,beta,m_11,m_10,ext,systematic,frame_length);
#endif

  alpha_ptr = alpha128;
  beta_ptr = &beta128[8];


#ifndef LLR8

  for (k=0; k<(frame_length>>3); k++) {

    m11_128        = (__m128i*)&m_11[k<<3];
    m10_128        = (__m128i*)&m_10[k<<3];
    ext_128        = (__m128i*)&ext[k<<3];
    /*
      printf("EXT %03d\n",k);
      print_shorts("a0:",&alpha_ptr[0]);
      print_shorts("a1:",&alpha_ptr[1]);
      print_shorts("a2:",&alpha_ptr[2]);
      print_shorts("a3:",&alpha_ptr[3]);
      print_shorts("a4:",&alpha_ptr[4]);
      print_shorts("a5:",&alpha_ptr[5]);
      print_shorts("a6:",&alpha_ptr[6]);
      print_shorts("a7:",&alpha_ptr[7]);
      print_shorts("b0:",&beta_ptr[0]);
      print_shorts("b1:",&beta_ptr[1]);
      print_shorts("b2:",&beta_ptr[2]);
      print_shorts("b3:",&beta_ptr[3]);
      print_shorts("b4:",&beta_ptr[4]);
      print_shorts("b5:",&beta_ptr[5]);
      print_shorts("b6:",&beta_ptr[6]);
      print_shorts("b7:",&beta_ptr[7]);
    */
    m00_4 = _mm_adds_epi16(alpha_ptr[7],beta_ptr[3]); //ALPHA_BETA_4m00;
    m11_4 = _mm_adds_epi16(alpha_ptr[7],beta_ptr[7]); //ALPHA_BETA_4m11;
    m00_3 = _mm_adds_epi16(alpha_ptr[6],beta_ptr[7]); //ALPHA_BETA_3m00;
    m11_3 = _mm_adds_epi16(alpha_ptr[6],beta_ptr[3]); //ALPHA_BETA_3m11;
    m00_2 = _mm_adds_epi16(alpha_ptr[1],beta_ptr[4]); //ALPHA_BETA_2m00;
    m11_2 = _mm_adds_epi16(alpha_ptr[1],beta_ptr[0]); //ALPHA_BETA_2m11;
    m11_1 = _mm_adds_epi16(alpha_ptr[0],beta_ptr[4]); //ALPHA_BETA_1m11;
    m00_1 = _mm_adds_epi16(alpha_ptr[0],beta_ptr[0]); //ALPHA_BETA_1m00;
    m01_4 = _mm_adds_epi16(alpha_ptr[5],beta_ptr[6]); //ALPHA_BETA_4m01;
    m10_4 = _mm_adds_epi16(alpha_ptr[5],beta_ptr[2]); //ALPHA_BETA_4m10;
    m01_3 = _mm_adds_epi16(alpha_ptr[4],beta_ptr[2]); //ALPHA_BETA_3m01;
    m10_3 = _mm_adds_epi16(alpha_ptr[4],beta_ptr[6]); //ALPHA_BETA_3m10;
    m01_2 = _mm_adds_epi16(alpha_ptr[3],beta_ptr[1]); //ALPHA_BETA_2m01;
    m10_2 = _mm_adds_epi16(alpha_ptr[3],beta_ptr[5]); //ALPHA_BETA_2m10;
    m10_1 = _mm_adds_epi16(alpha_ptr[2],beta_ptr[1]); //ALPHA_BETA_1m10;
    m01_1 = _mm_adds_epi16(alpha_ptr[2],beta_ptr[5]); //ALPHA_BETA_1m01;
    /*
      print_shorts("m11_1:",&m11_1);
      print_shorts("m11_2:",&m11_2);
      print_shorts("m11_3:",&m11_3);
      print_shorts("m11_4:",&m11_4);
      print_shorts("m00_1:",&m00_1);
      print_shorts("m00_2:",&m00_2);
      print_shorts("m00_3:",&m00_3);
      print_shorts("m00_4:",&m00_4);
      print_shorts("m10_1:",&m10_1);
      print_shorts("m10_2:",&m10_2);
      print_shorts("m10_3:",&m10_3);
      print_shorts("m10_4:",&m10_4);
      print_shorts("m01_1:",&m01_1);
      print_shorts("m01_2:",&m01_2);
      print_shorts("m01_3:",&m01_3);
      print_shorts("m01_4:",&m01_4);
    */
    m01_1 = _mm_max_epi16(m01_1,m01_2);
    m01_1 = _mm_max_epi16(m01_1,m01_3);
    m01_1 = _mm_max_epi16(m01_1,m01_4);
    m00_1 = _mm_max_epi16(m00_1,m00_2);
    m00_1 = _mm_max_epi16(m00_1,m00_3);
    m00_1 = _mm_max_epi16(m00_1,m00_4);
    m10_1 = _mm_max_epi16(m10_1,m10_2);
    m10_1 = _mm_max_epi16(m10_1,m10_3);
    m10_1 = _mm_max_epi16(m10_1,m10_4);
    m11_1 = _mm_max_epi16(m11_1,m11_2);
    m11_1 = _mm_max_epi16(m11_1,m11_3);
    m11_1 = _mm_max_epi16(m11_1,m11_4);

    //      print_shorts("m11_1:",&m11_1);

    m01_1 = _mm_subs_epi16(m01_1,*m10_128);
    m00_1 = _mm_subs_epi16(m00_1,*m11_128);
    m10_1 = _mm_adds_epi16(m10_1,*m10_128);
    m11_1 = _mm_adds_epi16(m11_1,*m11_128);

    //      print_shorts("m10_1:",&m10_1);
    //      print_shorts("m11_1:",&m11_1);
    m01_1 = _mm_max_epi16(m01_1,m00_1);
    m10_1 = _mm_max_epi16(m10_1,m11_1);
    //      print_shorts("m01_1:",&m01_1);
    //      print_shorts("m10_1:",&m10_1);

    *ext_128 = _mm_subs_epi16(m10_1,m01_1);

    /*
      print_shorts("ext:",ext_128);
      print_shorts("m11:",m11_128);
      print_shorts("m10:",m10_128);
      print_shorts("m10_1:",&m10_1);
      print_shorts("m01_1:",&m01_1);
      print_shorts("syst:",systematic_128);
    */

    alpha_ptr+=8;
    beta_ptr+=8;
  }

#else

  for (k=0; k<(frame_length>>4); k++) {

    m11_128        = (__m128i*)&m_11[k<<4];
    m10_128        = (__m128i*)&m_10[k<<4];
    ext_128        = (__m128i*)&ext[k<<4];

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
  }

#endif

  _mm_empty();
  _m_empty();

}



//int pi2[n],pi3[n+8],pi5[n+8],pi4[n+8],pi6[n+8],
int *pi2tab[188],*pi5tab[188],*pi4tab[188],*pi6tab[188];

void free_td()
{
  int ind;

  for (ind = 0; ind < 188; ind++) {
    free_and_zero(pi2tab[ind]);
    free_and_zero(pi5tab[ind]);
    free_and_zero(pi4tab[ind]);
    free_and_zero(pi6tab[ind]);
  }
}

void init_td()
{

  int ind,i,i2,i3,j,n,n2,pi,pi3;
  short * base_interleaver;

  for (ind=0; ind<188; ind++) {

    n = f1f2mat[ind].nb_bits;
    base_interleaver=il_tb+f1f2mat[ind].beg_index;
    pi2tab[ind] = malloc((n+8)*sizeof(int));
    pi5tab[ind] = malloc((n+8)*sizeof(int));
    pi4tab[ind] = malloc((n+8)*sizeof(int));
    pi6tab[ind] = malloc((n+8)*sizeof(int));

#ifdef LLR8

    if ((n&15)>0) {
      n2 = n+8;
    } else
      n2 = n;

    for (j=0,i=0; i<n2; i++,j+=16) {

      if (j>=n2)
        j-=(n2-1);

      pi2tab[ind][i] = j;
      //    printf("pi2[%d] = %d\n",i,j);
    }

#else
    n2 = n;

    for (i=i2=0; i2<8; i2++) {
      j=i2;

      for (i3=0; i3<(n>>3); i3++,i++,j+=8) {

        //    if (j>=n)
        //      j-=(n-1);

        pi2tab[ind][i]  = j;
        //    printf("pi2[%d] = %d\n",i,j);
      }
    }

#endif


    for (i=0; i<n2; i++) {
      pi = base_interleaver[i];//(unsigned int)threegpplte_interleaver(f1,f2,n);
      pi3 = pi2tab[ind][pi];
      pi4tab[ind][pi2tab[ind][i]] = pi3;
      pi5tab[ind][pi3] = pi2tab[ind][i];
      pi6tab[ind][pi] = pi2tab[ind][i];
    }

  }
}

unsigned char phy_threegpplte_turbo_decoder(short *y,
    unsigned char *decoded_bytes,
    unsigned short n,
    unsigned short f1,
    unsigned short f2,
    unsigned char max_iterations,
    unsigned char crc_type,
    unsigned char F,
    time_stats_t *init_stats,
    time_stats_t *alpha_stats,
    time_stats_t *beta_stats,
    time_stats_t *gamma_stats,
    time_stats_t *ext_stats,
    time_stats_t *intl1_stats,
    time_stats_t *intl2_stats)
{

  /*  y is a pointer to the input
      decoded_bytes is a pointer to the decoded output
      n is the size in bits of the coded block, with the tail */

  int n2;
#ifdef LLR8
  llr_t y8[3*(n+16)] __attribute__((aligned(16)));
#endif

  llr_t systematic0[n+16] __attribute__ ((aligned(16)));
  llr_t systematic1[n+16] __attribute__ ((aligned(16)));
  llr_t systematic2[n+16] __attribute__ ((aligned(16)));
  llr_t yparity1[n+16] __attribute__ ((aligned(16)));
  llr_t yparity2[n+16] __attribute__ ((aligned(16)));

  llr_t ext[n+128] __attribute__((aligned(16)));
  llr_t ext2[n+128] __attribute__((aligned(16)));

  llr_t alpha[(n+16)*8] __attribute__ ((aligned(16)));
  llr_t beta[(n+16)*8] __attribute__ ((aligned(16)));
  llr_t m11[n+16] __attribute__ ((aligned(16)));
  llr_t m10[n+16] __attribute__ ((aligned(16)));


  int *pi2_p,*pi4_p,*pi5_p,*pi6_p;
  llr_t *s,*s1,*s2,*yp1,*yp2,*yp;
  __m128i *yp128;
  unsigned int i,j,iind;//,pi;
  unsigned char iteration_cnt=0;
  unsigned int crc,oldcrc,crc_len;
  uint8_t temp;
  __m128i tmp128[(n+8)>>3];

  __m128i tmp, zeros=_mm_setzero_si128();
#ifdef LLR8
  __m128i MAX128=_mm_set1_epi16(MAX/2);
#endif

  register __m128i tmpe;
  int offset8_flag=0;

  if (crc_type > 3) {
    msg("Illegal crc length!\n");
    return 255;
  }


  start_meas(init_stats);

#ifdef LLR8

  if ((n&15)>0) {
    n2 = n+8;
    offset8_flag=1;
  } else
    n2 = n;

#else
  n2=n;
#endif

  for (iind=0; f1f2mat[iind].nb_bits!=n && iind <188; iind++);

  if ( iind == 188 ) {
    msg("Illegal frame length!\n");
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

#ifdef LLR8

  for (i=0,j=0; i<(3*(n2>>4))+1; i++,j+=2) {
    ((__m128i *)y8)[i] = _mm_packs_epi16(_mm_srai_epi16(((__m128i *)y)[j],1),_mm_srai_epi16(((__m128i *)y)[j+1],1));
    //((__m128i *)y8)[i] = _mm_packs_epi16(((__m128i *)y)[j],((__m128i *)y)[j+1]);
  }

  yp128 = (__m128i*)y8;
#else
  yp128 = (__m128i*)y;
#endif



  s = systematic0;
  s1 = systematic1;
  s2 = systematic2;
  yp1 = yparity1;
  yp2 = yparity2;


#ifndef LLR8

  for (i=0; i<n2; i+=8) {
    pi2_p = &pi2tab[iind][i];

    j=pi2_p[0];


    tmpe = _mm_load_si128(yp128);

    s[j]   = _mm_extract_epi16(tmpe,0);
    yp1[j] = _mm_extract_epi16(tmpe,1);
    yp2[j] = _mm_extract_epi16(tmpe,2);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[1];

    s[j]   = _mm_extract_epi16(tmpe,3);
    yp1[j] = _mm_extract_epi16(tmpe,4);
    yp2[j] = _mm_extract_epi16(tmpe,5);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[2];

    s[j]   = _mm_extract_epi16(tmpe,6);
    yp1[j] = _mm_extract_epi16(tmpe,7);
    tmpe = _mm_load_si128(&yp128[1]);
    yp2[j] = _mm_extract_epi16(tmpe,0);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[3];

    s[j]   = _mm_extract_epi16(tmpe,1);
    yp1[j] = _mm_extract_epi16(tmpe,2);
    yp2[j] = _mm_extract_epi16(tmpe,3);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[4];

    s[j]   = _mm_extract_epi16(tmpe,4);
    yp1[j] = _mm_extract_epi16(tmpe,5);
    yp2[j] = _mm_extract_epi16(tmpe,6);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[5];

    s[j]   = _mm_extract_epi16(tmpe,7);
    tmpe = _mm_load_si128(&yp128[2]);
    yp1[j] = _mm_extract_epi16(tmpe,0);
    yp2[j] = _mm_extract_epi16(tmpe,1);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[6];

    s[j]   = _mm_extract_epi16(tmpe,2);
    yp1[j] = _mm_extract_epi16(tmpe,3);
    yp2[j] = _mm_extract_epi16(tmpe,4);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[7];

    s[j]   = _mm_extract_epi16(tmpe,5);
    yp1[j] = _mm_extract_epi16(tmpe,6);
    yp2[j] = _mm_extract_epi16(tmpe,7);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    yp128+=3;

  }

#else

  for (i=0; i<n2; i+=16) {
    pi2_p = &pi2tab[iind][i];

    j=pi2_p[0];
    s[j]   = _mm_extract_epi8(yp128[0],0);
    yp1[j] = _mm_extract_epi8(yp128[0],1);
    yp2[j] = _mm_extract_epi8(yp128[0],2);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[1];
    s[j]   = _mm_extract_epi8(yp128[0],3);
    yp1[j] = _mm_extract_epi8(yp128[0],4);
    yp2[j] = _mm_extract_epi8(yp128[0],5);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[2];
    s[j]   = _mm_extract_epi8(yp128[0],6);
    yp1[j] = _mm_extract_epi8(yp128[0],7);
    yp2[j] = _mm_extract_epi8(yp128[0],8);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[3];
    s[j]   = _mm_extract_epi8(yp128[0],9);
    yp1[j] = _mm_extract_epi8(yp128[0],10);
    yp2[j] = _mm_extract_epi8(yp128[0],11);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[4];
    s[j]   = _mm_extract_epi8(yp128[0],12);
    yp1[j] = _mm_extract_epi8(yp128[0],13);
    yp2[j] = _mm_extract_epi8(yp128[0],14);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[5];
    s[j]   = _mm_extract_epi8(yp128[0],15);
    yp1[j] = _mm_extract_epi8(yp128[1],0);
    yp2[j] = _mm_extract_epi8(yp128[1],1);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[6];
    s[j]   = _mm_extract_epi8(yp128[1],2);
    yp1[j] = _mm_extract_epi8(yp128[1],3);
    yp2[j] = _mm_extract_epi8(yp128[1],4);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[7];
    s[j]   = _mm_extract_epi8(yp128[1],5);
    yp1[j] = _mm_extract_epi8(yp128[1],6);
    yp2[j] = _mm_extract_epi8(yp128[1],7);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[8];
    s[j]   = _mm_extract_epi8(yp128[1],8);
    yp1[j] = _mm_extract_epi8(yp128[1],9);
    yp2[j] = _mm_extract_epi8(yp128[1],10);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[9];
    s[j]   = _mm_extract_epi8(yp128[1],11);
    yp1[j] = _mm_extract_epi8(yp128[1],12);
    yp2[j] = _mm_extract_epi8(yp128[1],13);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[10];
    s[j]   = _mm_extract_epi8(yp128[1],14);
    yp1[j] = _mm_extract_epi8(yp128[1],15);
    yp2[j] = _mm_extract_epi8(yp128[2],0);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[11];
    s[j]   = _mm_extract_epi8(yp128[2],1);
    yp1[j] = _mm_extract_epi8(yp128[2],2);
    yp2[j] = _mm_extract_epi8(yp128[2],3);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[12];
    s[j]   = _mm_extract_epi8(yp128[2],4);
    yp1[j] = _mm_extract_epi8(yp128[2],5);
    yp2[j] = _mm_extract_epi8(yp128[2],6);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[13];
    s[j]   = _mm_extract_epi8(yp128[2],7);
    yp1[j] = _mm_extract_epi8(yp128[2],8);
    yp2[j] = _mm_extract_epi8(yp128[2],9);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[14];
    s[j]   = _mm_extract_epi8(yp128[2],10);
    yp1[j] = _mm_extract_epi8(yp128[2],11);
    yp2[j] = _mm_extract_epi8(yp128[2],12);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    j=pi2_p[15];
    s[j]   = _mm_extract_epi8(yp128[2],13);
    yp1[j] = _mm_extract_epi8(yp128[2],14);
    yp2[j] = _mm_extract_epi8(yp128[2],15);
    //    printf("init: j %d, s[j] %d yp1[j] %d yp2[j] %d\n",j,s[j],yp1[j],yp2[j]);

    yp128+=3;

  }

#endif


  yp=(llr_t*)yp128;
#ifdef LLR8

  if (n2>n) {
    /*
    s[n]=0;s[n+1]=0;s[n+2]=0;s[n+3]=0;
    s[n+4]=0;s[n+5]=0;s[n+6]=0;s[n+7]=0;
    s1[n]=0;s1[n+1]=0;s1[n+2]=0;s1[n+3]=0;
    s1[n+4]=0;s1[n+5]=0;s1[n+6]=0;s1[n+7]=0;
    s2[n]=0;s2[n+1]=0;s2[n+2]=0;s2[n+3]=0;
    s2[n+4]=0;s2[n+5]=0;s2[n+6]=0;s2[n+7]=0;*/
    yp=(llr_t*)(y8+n);
  }

#endif
  //  printf("n=%d,n2=%d\n",n,n2);

  // Termination
  for (i=n2; i<n2+3; i++) {
    s[i]= *yp;
    s1[i] = s[i] ;
    s2[i] = s[i];
    yp++;
    yp1[i] = *yp;
    yp++;
#ifdef DEBUG_LOGMAP
    msg("Term 1 (%d): %d %d\n",i,s[i],yp1[i]);
#endif //DEBUG_LOGMAP
  }

#ifdef LLR8

  for (i=n2+16; i<n2+19; i++) {
    s[i]= *yp;
    s1[i] = s[i] ;
    s2[i] = s[i];
    yp++;
    yp2[i-16] = *yp;
    yp++;
#ifdef DEBUG_LOGMAP
    msg("Term 2 (%d): %d %d\n",i-16,s[i],yp2[i-16]);
#endif //DEBUG_LOGMAP
  }

#else

  for (i=n2+8; i<n2+11; i++) {
    s[i]= *yp;
    s1[i] = s[i] ;
    s2[i] = s[i];
    yp++;
    yp2[i-8] = *yp;
    yp++;
#ifdef DEBUG_LOGMAP
    msg("Term 2 (%d): %d %d\n",i-3,s[i],yp2[i-8]);
#endif //DEBUG_LOGMAP
  }

#endif
#ifdef DEBUG_LOGMAP
  msg("\n");
#endif //DEBUG_LOGMAP

  stop_meas(init_stats);

  // do log_map from first parity bit

  log_map(systematic0,yparity1,m11,m10,alpha,beta,ext,n2,0,F,offset8_flag,alpha_stats,beta_stats,gamma_stats,ext_stats);

  while (iteration_cnt++ < max_iterations) {

#ifdef DEBUG_LOGMAP
    printf("\n*******************ITERATION %d (n %d), ext %p\n\n",iteration_cnt,n,ext);
#endif //DEBUG_LOGMAP

    start_meas(intl1_stats);
#ifndef LLR8

    pi4_p=pi4tab[iind];

    for (i=0; i<(n2>>3); i++) { // steady-state portion

      ((__m128i *)systematic2)[i]=_mm_insert_epi16(((__m128i *)systematic2)[i],((llr_t*)ext)[*pi4_p++],0);
      ((__m128i *)systematic2)[i]=_mm_insert_epi16(((__m128i *)systematic2)[i],((llr_t*)ext)[*pi4_p++],1);
      ((__m128i *)systematic2)[i]=_mm_insert_epi16(((__m128i *)systematic2)[i],((llr_t*)ext)[*pi4_p++],2);
      ((__m128i *)systematic2)[i]=_mm_insert_epi16(((__m128i *)systematic2)[i],((llr_t*)ext)[*pi4_p++],3);
      ((__m128i *)systematic2)[i]=_mm_insert_epi16(((__m128i *)systematic2)[i],((llr_t*)ext)[*pi4_p++],4);
      ((__m128i *)systematic2)[i]=_mm_insert_epi16(((__m128i *)systematic2)[i],((llr_t*)ext)[*pi4_p++],5);
      ((__m128i *)systematic2)[i]=_mm_insert_epi16(((__m128i *)systematic2)[i],((llr_t*)ext)[*pi4_p++],6);
      ((__m128i *)systematic2)[i]=_mm_insert_epi16(((__m128i *)systematic2)[i],((llr_t*)ext)[*pi4_p++],7);
    }

#else

    pi4_p=pi4tab[iind];

    for (i=0; i<(n2>>4); i++) { // steady-state portion
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],0);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],1);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],2);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],3);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],4);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],5);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],6);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],7);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],8);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],9);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],10);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],11);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],12);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],13);
      tmp=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],14);
      ((__m128i *)systematic2)[i]=_mm_insert_epi8(tmp,((llr_t*)ext)[*pi4_p++],15);
    }

#endif

    stop_meas(intl1_stats);

    // do log_map from second parity bit

    log_map(systematic2,yparity2,m11,m10,alpha,beta,ext2,n2,1,F,offset8_flag,alpha_stats,beta_stats,gamma_stats,ext_stats);


#ifndef LLR8
    pi5_p=pi5tab[iind];

    for (i=0; i<(n>>3); i++) {
      tmp=_mm_insert_epi16(tmp,ext2[*pi5_p++],0);
      tmp=_mm_insert_epi16(tmp,ext2[*pi5_p++],1);
      tmp=_mm_insert_epi16(tmp,ext2[*pi5_p++],2);
      tmp=_mm_insert_epi16(tmp,ext2[*pi5_p++],3);
      tmp=_mm_insert_epi16(tmp,ext2[*pi5_p++],4);
      tmp=_mm_insert_epi16(tmp,ext2[*pi5_p++],5);
      tmp=_mm_insert_epi16(tmp,ext2[*pi5_p++],6);
      tmp=_mm_insert_epi16(tmp,ext2[*pi5_p++],7);
      ((__m128i *)systematic1)[i] = _mm_adds_epi16(_mm_subs_epi16(tmp,((__m128i*)ext)[i]),((__m128i *)systematic0)[i]);
    }

    if (iteration_cnt>1) {
      start_meas(intl2_stats);
      pi6_p=pi6tab[iind];

      for (i=0; i<(n2>>3); i++) {
        tmp=_mm_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p++],7);
        tmp=_mm_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p++],6);
        tmp=_mm_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p++],5);
        tmp=_mm_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p++],4);
        tmp=_mm_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p++],3);
        tmp=_mm_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p++],2);
        tmp=_mm_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p++],1);
        tmp=_mm_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p++],0);
        tmp=_mm_cmpgt_epi8(_mm_packs_epi16(tmp,zeros),zeros);
        decoded_bytes[i]=(unsigned char)_mm_movemask_epi8(tmp);

      }
    }

#else
    pi5_p=pi5tab[iind];
    uint16_t decoded_bytes_interl[6144/16];

    for (i=0; i<(n2>>4); i++) {
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
      //decoded_bytes_interl[i]=(uint16_t) _mm_movemask_epi8(_mm_cmpgt_epi8(tmp,zeros));
      tmp128[i] = _mm_adds_epi8(((__m128i *)ext2)[i],((__m128i *)systematic2)[i]);

      ((__m128i *)systematic1)[i] = _mm_adds_epi8(_mm_subs_epi8(tmp,((__m128i*)ext)[i]),((__m128i *)systematic0)[i]);
    }

    /* LT modification, something wrong here
    if (iteration_cnt>1) {
      start_meas(intl2_stats);
      pi6_p=pi6tab[iind];
      __m128i* dbytes=(__m128i*)decoded_bytes_interl;
      __m128i shuffle=SHUFFLE16(7,6,5,4,3,2,1,0);
      __m128i mask  __attribute__((aligned(16)));
      int n_128=n2>>7;
      for (i=0;i<n_128;i++) {
        mask=_mm_set1_epi16(1);
        __m128i tmp __attribute__((aligned(16)));
        tmp=_mm_shuffle_epi8(dbytes[i],shuffle);
        __m128i tmp2 __attribute__((aligned(16))) ;

        tmp2=_mm_and_si128(tmp,mask);
        tmp2=_mm_cmpeq_epi16(tmp2,mask);
        decoded_bytes[n_128*0+i]=(uint8_t) _mm_movemask_epi8(_mm_packs_epi16(tmp2,zeros));

        int j;
        for (j=1; j<16; j++) {
    mask=_mm_slli_epi16(mask,1);
    tmp2=_mm_and_si128(tmp,mask);
    tmp2=_mm_cmpeq_epi16(tmp2,mask);
    decoded_bytes[n_128*j +i]=(uint8_t) _mm_movemask_epi8(_mm_packs_epi16(tmp2,zeros));
        }
      }
    }
    */
    // Previous version
    if (iteration_cnt>1) {
      start_meas(intl2_stats);
      pi6_p=pi6tab[iind];

      for (i=0; i<(n2>>4); i++) {
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
      }
    }

#endif

    // check status on output
    if (iteration_cnt>1) {
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

      stop_meas(intl2_stats);

      if (crc == oldcrc) {
        return(iteration_cnt);
      }
    }

    // do log_map from first parity bit
    if (iteration_cnt < max_iterations) {
      log_map(systematic1,yparity1,m11,m10,alpha,beta,ext,n2,0,F,offset8_flag,alpha_stats,beta_stats,gamma_stats,ext_stats);
      __m128i* ext_128=(__m128i*) ext;
      __m128i* s1_128=(__m128i*) systematic1;
      __m128i* s0_128=(__m128i*) systematic0;
#ifndef LLR8
      int myloop=n2>>3;

      for (i=0; i<myloop; i++) {
        *ext_128=_mm_adds_epi16(_mm_subs_epi16(*ext_128,*s1_128++),*s0_128++);
        ext_128++;
      }

#else
      int myloop=n2>>4;

      for (i=0; i<myloop; i++) {
        *ext_128=_mm_adds_epi8(_mm_subs_epi8(*ext_128,*s1_128++),*s0_128++);
        ext_128++;
      }

#endif
    }
  }

  return(iteration_cnt);
}

#ifdef TEST_DEBUG

int test_logmap8()
{
  unsigned char test[8];
  //_declspec(align(16))  char channel_output[512];
  //_declspec(align(16))  unsigned char output[512],decoded_output[16], *inPtr, *outPtr;

  short channel_output[512];
  unsigned char output[512],decoded_output[16];
  unsigned int i,crc,ret;

  test[0] = 7;
  test[1] = 0xa5;
  test[2] = 0x11;
  test[3] = 0x92;
  test[4] = 0xfe;

  crcTableInit();

  crc = crc24a(test,
               40)>>8;

  *(unsigned int*)(&test[5]) = crc;

  printf("crc24 = %x\n",crc);
  threegpplte_turbo_encoder(test,   //input
                            8,      //input length bytes
                            output, //output
                            0,      //filler bits
                            7,      //interleaver f1
                            16);    //interleaver f2

  for (i = 0; i < 204; i++) {
    channel_output[i] = 15*(2*output[i] - 1);
    //    msg("Position %d : %d\n",i,channel_output[i]);
  }

  memset(decoded_output,0,16);
  ret = phy_threegpplte_turbo_decoder(channel_output,
                                      decoded_output,
                                      64,       // length bits
                                      7,        // interleaver f1
                                      16,       // interleaver f2
                                      6,        // max iterations
                                      CRC24_A,  // CRC type (CRC24_A,CRC24_B)
                                      0,        // filler bits
                                      0);       // decoder instance


  for (i=0; i<8; i++)
    printf("output %d => %x (input %x)\n",i,decoded_output[i],test[i]);
}




int main()
{


  test_logmap8();

  return(0);
}

#endif // TEST_DEBUG


