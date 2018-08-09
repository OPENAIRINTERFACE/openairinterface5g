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

/* file: 3gpplte_turbo_decoder_sse_16bit.c
   purpose: Routines for implementing max-logmap decoding of Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03
   authors: raymond.knopp@eurecom.fr, Laurent Thomas (Alcatel-Lucent)
   date: 21.10.2009

   Note: This version of the routine currently requires SSE2,SSSE3 and SSE4.1 equipped computers.  It uses 16-bit inputs for
         LLRS and uses 16-bit arithmetic for the internal computations!

   Changelog: 17.11.2009 FK SSE4.1 not required anymore
   Aug. 2012 new parallelization options for higher speed (8-way parallelization)
   Jan. 2013 8-bit LLR support with 16-way parallelization
   Feb. 2013 New interleaving and hard-decision optimizations (L. Thomas)
   May 2013 Extracted 16bit code
*/

///
///





#ifndef TEST_DEBUG
#include "PHY/impl_defs_top.h"
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

#ifdef __AVX2__
#include "PHY/sse_intrin.h"

//#define DEBUG_LOGMAP

#ifdef DEBUG_LOGMAP
#define print_shorts(s,x) fprintf(fdavx2,"%s %d,%d,%d,%d,%d,%d,%d,%d\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7]);fprintf(fdavx2b,"%s %d,%d,%d,%d,%d,%d,%d,%d\n",s,(x)[8],(x)[9],(x)[10],(x)[11],(x)[12],(x)[13],(x)[14],(x)[15])
FILE *fdavx2,*fdavx2b;
#else

#endif


#define print_bytes(s,x) printf("%s %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7],(x)[8],(x)[9],(x)[10],(x)[11],(x)[12],(x)[13],(x)[14],(x)[15],(x)[16],(x)[17],(x)[18],(x)[19],(x)[20],(x)[21],(x)[22],(x)[23],(x)[24],(x)[25],(x)[26],(x)[27],(x)[28],(x)[29],(x)[30],(x)[31])




typedef int16_t llr_t; // internal decoder LLR data is 16-bit fixed
typedef int16_t channel_t;
#define MAX 256

void log_map16avx2(llr_t* systematic,channel_t* y_parity, llr_t* m11, llr_t* m10, llr_t *alpha, llr_t *beta, llr_t* ext,uint16_t frame_length,unsigned char term_flag,unsigned char F,int offset8_flag,time_stats_t *alpha_stats,time_stats_t *beta_stats,time_stats_t *gamma_stats,time_stats_t *ext_stats);
void compute_gamma16avx2(llr_t* m11,llr_t* m10,llr_t* systematic, channel_t* y_parity, uint16_t frame_length,unsigned char term_flag);
void compute_alpha16avx2(llr_t*alpha,llr_t *beta, llr_t* m11,llr_t* m10, uint16_t frame_length,unsigned char F);
void compute_beta16avx2(llr_t*alpha, llr_t* beta,llr_t* m11,llr_t* m10, uint16_t frame_length,unsigned char F,int offset8_flag);
void compute_ext16avx2(llr_t* alpha,llr_t* beta,llr_t* m11,llr_t* m10,llr_t* extrinsic, llr_t* ap, uint16_t frame_length);


void log_map16avx2(llr_t* systematic,
		   channel_t* y_parity,
		   llr_t* m11,
		   llr_t* m10,
		   llr_t *alpha,
		   llr_t *beta,
		   llr_t* ext,
		   uint16_t frame_length,
		   unsigned char term_flag,
		   unsigned char F,
		   int offset8_flag,
		   time_stats_t *alpha_stats,
		   time_stats_t *beta_stats,
		   time_stats_t *gamma_stats,
		   time_stats_t *ext_stats)
{

#ifdef DEBUG_LOGMAP
  fprintf(fdavx2,"log_map (avx2_16bit), frame_length %d\n",frame_length);
  fprintf(fdavx2b,"log_map (avx2_16bit), frame_length %d\n",frame_length);
#endif

  start_meas(gamma_stats) ;
  compute_gamma16avx2(m11,m10,systematic,y_parity,frame_length,term_flag) ;
  stop_meas(gamma_stats);
  start_meas(alpha_stats) ;
  compute_alpha16avx2(alpha,beta,m11,m10,frame_length,F)                  ;
  stop_meas(alpha_stats);
  start_meas(beta_stats)  ;
  compute_beta16avx2(alpha,beta,m11,m10,frame_length,F,offset8_flag)      ;
  stop_meas(beta_stats);
  start_meas(ext_stats)   ;
  compute_ext16avx2(alpha,beta,m11,m10,ext,systematic,frame_length)       ;
  stop_meas(ext_stats);


}

void compute_gamma16avx2(llr_t* m11,llr_t* m10,llr_t* systematic,channel_t* y_parity,
                     uint16_t frame_length,unsigned char term_flag)
{
  int k,K1;

  __m256i *systematic128 = (__m256i *)systematic;
  __m256i *y_parity128   = (__m256i *)y_parity;
  __m256i *m10_128        = (__m256i *)m10;
  __m256i *m11_128        = (__m256i *)m11;

#ifdef DEBUG_LOGMAP
  fprintf(fdavx2,"compute_gamma (avx2_16bit), %p,%p,%p,%p,framelength %d\n",m11,m10,systematic,y_parity,frame_length);
  fprintf(fdavx2b,"compute_gamma (avx2_16bit), %p,%p,%p,%p,framelength %d\n",m11,m10,systematic,y_parity,frame_length);
#endif

  K1=frame_length>>3;

  for (k=0; k<K1; k++) {
    m11_128[k] = _mm256_srai_epi16(_mm256_adds_epi16(systematic128[k],y_parity128[k]),1);
    m10_128[k] = _mm256_srai_epi16(_mm256_subs_epi16(systematic128[k],y_parity128[k]),1);
#ifdef DEBUG_LOGMAP
    fprintf(fdavx2,"Loop index k %d\n",k);
    fprintf(fdavx2b,"Loop index k %d\n",k);
    print_shorts("sys",(int16_t*)&systematic128[k]);
    print_shorts("yp",(int16_t*)&y_parity128[k]);
    print_shorts("m11",(int16_t*)&m11_128[k]);
    print_shorts("m10",(int16_t*)&m10_128[k]);
#endif
  }

  // Termination
  m11_128[k] = _mm256_srai_epi16(_mm256_adds_epi16(systematic128[k+term_flag],y_parity128[k]),1);
  m10_128[k] = _mm256_srai_epi16(_mm256_subs_epi16(systematic128[k+term_flag],y_parity128[k]),1);

#ifdef DEBUG_LOGMAP
  fprintf(fdavx2,"Loop index k %d (term flag %d)\n",k,term_flag);
  fprintf(fdavx2b,"Loop index k %d (term flag %d)\n",k,term_flag);
  print_shorts("sys",(int16_t*)&systematic128[k+term_flag]);
  print_shorts("yp",(int16_t*)&y_parity128[k]);
  print_shorts("m11",(int16_t*)&m11_128[k]);
  print_shorts("m10",(int16_t*)&m10_128[k]);
#endif
}

#define L 40

void compute_alpha16avx2(llr_t* alpha,llr_t* beta,llr_t* m_11,llr_t* m_10,uint16_t frame_length,unsigned char F)
{
  int k,l,l2,K1,rerun_flag=0;

  __m256i *alpha128=(__m256i *)alpha,*alpha_ptr;
  __m256i a0,a1,a2,a3,a4,a5,a6,a7,*m11p,*m10p;
  __m256i m_b0,m_b1,m_b2,m_b3,m_b4,m_b5,m_b6,m_b7;
  __m256i new0,new1,new2,new3,new4,new5,new6,new7;
  __m256i alpha_max;

  unsigned long long timein,timeout;

  l2 = L>>3;
  K1 = (frame_length>>3);
#ifdef DEBUG_LOGMAP
  fprintf(fdavx2,"Compute alpha (avx2_16bit)\n");
  fprintf(fdavx2b,"Compute alpha (avx2_16bit)\n");
#endif
  timein = rdtsc_oai();

  for (l=K1;; l=l2,rerun_flag=1) {
    alpha128 = (__m256i *)alpha;

    if (rerun_flag == 0) {

      alpha128[0] = _mm256_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,0,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,0);
      alpha128[1] = _mm256_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[2] = _mm256_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[3] = _mm256_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[4] = _mm256_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[5] = _mm256_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[6] = _mm256_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
      alpha128[7] = _mm256_set_epi16(-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2,-MAX/2);
#ifdef DEBUG_LOGMAP
      fprintf(fdavx2,"Initial alpha\n");
      fprintf(fdavx2b,"Initial alpha\n");
      print_shorts("a0",(int16_t*)&alpha128[0]);
      print_shorts("a1",(int16_t*)&alpha128[1]);
      print_shorts("a2",(int16_t*)&alpha128[2]);
      print_shorts("a3",(int16_t*)&alpha128[3]);
      print_shorts("a4",(int16_t*)&alpha128[4]);
      print_shorts("a5",(int16_t*)&alpha128[5]);
      print_shorts("a6",(int16_t*)&alpha128[6]);
      print_shorts("a7",(int16_t*)&alpha128[7]);
#endif
    } else {
      //set initial alpha in columns 1-7 from final alpha from last run in columns 0-6
      alpha128[0] = _mm256_slli_si256(alpha128[frame_length],2);
      alpha128[1] = _mm256_slli_si256(alpha128[1+frame_length],2);
      alpha128[2] = _mm256_slli_si256(alpha128[2+frame_length],2);
      alpha128[3] = _mm256_slli_si256(alpha128[3+frame_length],2);
      alpha128[4] = _mm256_slli_si256(alpha128[4+frame_length],2);
      alpha128[5] = _mm256_slli_si256(alpha128[5+frame_length],2);
      alpha128[6] = _mm256_slli_si256(alpha128[6+frame_length],2);
      alpha128[7] = _mm256_slli_si256(alpha128[7+frame_length],2);
      // set initial alpha in column 0 to (0,-MAX/2,...,-MAX/2)
      alpha[16] = -MAX/2;
      alpha[32] = -MAX/2;
      alpha[48] = -MAX/2;
      alpha[64] = -MAX/2;
      alpha[80] = -MAX/2;
      alpha[96] = -MAX/2;
      alpha[112] = -MAX/2;

      alpha[24] = -MAX/2;
      alpha[40] = -MAX/2;
      alpha[56] = -MAX/2;
      alpha[72] = -MAX/2;
      alpha[88] = -MAX/2;
      alpha[104] = -MAX/2;
      alpha[120] = -MAX/2;
#ifdef DEBUG_LOGMAP
      fprintf(fdavx2,"Second run\n");
      fprintf(fdavx2b,"Second run\n");
      print_shorts("a0",(int16_t*)&alpha128[0]);
      print_shorts("a1",(int16_t*)&alpha128[1]);
      print_shorts("a2",(int16_t*)&alpha128[2]);
      print_shorts("a3",(int16_t*)&alpha128[3]);
      print_shorts("a4",(int16_t*)&alpha128[4]);
      print_shorts("a5",(int16_t*)&alpha128[5]);
      print_shorts("a6",(int16_t*)&alpha128[6]);
      print_shorts("a7",(int16_t*)&alpha128[7]);
#endif

    }

    alpha_ptr = &alpha128[0];
    m11p = (__m256i*)m_11;
    m10p = (__m256i*)m_10;

    for (k=0;
         k<l;
         k++) {


      a1=_mm256_load_si256(&alpha_ptr[1]);
      a3=_mm256_load_si256(&alpha_ptr[3]);
      a5=_mm256_load_si256(&alpha_ptr[5]);
      a7=_mm256_load_si256(&alpha_ptr[7]);

      m_b0 = _mm256_adds_epi16(a1,*m11p);  // m11
      m_b4 = _mm256_subs_epi16(a1,*m11p);  // m00=-m11
      m_b1 = _mm256_subs_epi16(a3,*m10p);  // m01=-m10
      m_b5 = _mm256_adds_epi16(a3,*m10p);  // m10
      m_b2 = _mm256_adds_epi16(a5,*m10p);  // m10
      m_b6 = _mm256_subs_epi16(a5,*m10p);  // m01=-m10
      m_b3 = _mm256_subs_epi16(a7,*m11p);  // m00=-m11
      m_b7 = _mm256_adds_epi16(a7,*m11p);  // m11

      a0=_mm256_load_si256(&alpha_ptr[0]);
      a2=_mm256_load_si256(&alpha_ptr[2]);
      a4=_mm256_load_si256(&alpha_ptr[4]);
      a6=_mm256_load_si256(&alpha_ptr[6]);

      new0 = _mm256_subs_epi16(a0,*m11p);  // m00=-m11
      new4 = _mm256_adds_epi16(a0,*m11p);  // m11
      new1 = _mm256_adds_epi16(a2,*m10p);  // m10
      new5 = _mm256_subs_epi16(a2,*m10p);  // m01=-m10
      new2 = _mm256_subs_epi16(a4,*m10p);  // m01=-m10
      new6 = _mm256_adds_epi16(a4,*m10p);  // m10
      new3 = _mm256_adds_epi16(a6,*m11p);  // m11
      new7 = _mm256_subs_epi16(a6,*m11p);  // m00=-m11

      a0 = _mm256_max_epi16(m_b0,new0);
      a1 = _mm256_max_epi16(m_b1,new1);
      a2 = _mm256_max_epi16(m_b2,new2);
      a3 = _mm256_max_epi16(m_b3,new3);
      a4 = _mm256_max_epi16(m_b4,new4);
      a5 = _mm256_max_epi16(m_b5,new5);
      a6 = _mm256_max_epi16(m_b6,new6);
      a7 = _mm256_max_epi16(m_b7,new7);

      alpha_max = _mm256_max_epi16(a0,a1);
      alpha_max = _mm256_max_epi16(alpha_max,a2);
      alpha_max = _mm256_max_epi16(alpha_max,a3);
      alpha_max = _mm256_max_epi16(alpha_max,a4);
      alpha_max = _mm256_max_epi16(alpha_max,a5);
      alpha_max = _mm256_max_epi16(alpha_max,a6);
      alpha_max = _mm256_max_epi16(alpha_max,a7);

      alpha_ptr+=8;
      m11p++;
      m10p++;

      alpha_ptr[0] = _mm256_subs_epi16(a0,alpha_max);
      alpha_ptr[1] = _mm256_subs_epi16(a1,alpha_max);
      alpha_ptr[2] = _mm256_subs_epi16(a2,alpha_max);
      alpha_ptr[3] = _mm256_subs_epi16(a3,alpha_max);
      alpha_ptr[4] = _mm256_subs_epi16(a4,alpha_max);
      alpha_ptr[5] = _mm256_subs_epi16(a5,alpha_max);
      alpha_ptr[6] = _mm256_subs_epi16(a6,alpha_max);
      alpha_ptr[7] = _mm256_subs_epi16(a7,alpha_max);

#ifdef DEBUG_LOGMAP
      fprintf(fdavx2,"Loop index %d\n",k);
      fprintf(fdavx2b,"Loop index %d\n",k);
      print_shorts("mb0",(int16_t*)&m_b0);
      print_shorts("mb1",(int16_t*)&m_b1);
      print_shorts("mb2",(int16_t*)&m_b2);
      print_shorts("mb3",(int16_t*)&m_b3);
      print_shorts("mb4",(int16_t*)&m_b4);
      print_shorts("mb5",(int16_t*)&m_b5);
      print_shorts("mb6",(int16_t*)&m_b6);
      print_shorts("mb7",(int16_t*)&m_b7);

      fprintf(fdavx2,"Loop index %d, new\n",k);
      fprintf(fdavx2b,"Loop index %d, new\n",k);
      print_shorts("new0",(int16_t*)&new0);
      print_shorts("new1",(int16_t*)&new1);
      print_shorts("new2",(int16_t*)&new2);
      print_shorts("new3",(int16_t*)&new3);
      print_shorts("new4",(int16_t*)&new4);
      print_shorts("new5",(int16_t*)&new5);
      print_shorts("new6",(int16_t*)&new6);
      print_shorts("new7",(int16_t*)&new7);

      fprintf(fdavx2,"Loop index %d, after max\n",k);
      fprintf(fdavx2b,"Loop index %d, after max\n",k);
      print_shorts("a0",(int16_t*)&a0);
      print_shorts("a1",(int16_t*)&a1);
      print_shorts("a2",(int16_t*)&a2);
      print_shorts("a3",(int16_t*)&a3);
      print_shorts("a4",(int16_t*)&a4);
      print_shorts("a5",(int16_t*)&a5);
      print_shorts("a6",(int16_t*)&a6);
      print_shorts("a7",(int16_t*)&a7);

      fprintf(fdavx2,"Loop index %d\n",k);
      fprintf(fdavx2b,"Loop index %d\n",k);
      print_shorts("a0",(int16_t*)&alpha_ptr[0]);
      print_shorts("a1",(int16_t*)&alpha_ptr[1]);
      print_shorts("a2",(int16_t*)&alpha_ptr[2]);
      print_shorts("a3",(int16_t*)&alpha_ptr[3]);
      print_shorts("a4",(int16_t*)&alpha_ptr[4]);
      print_shorts("a5",(int16_t*)&alpha_ptr[5]);
      print_shorts("a6",(int16_t*)&alpha_ptr[6]);
      print_shorts("a7",(int16_t*)&alpha_ptr[7]);


#endif

    }

    if (rerun_flag==1)
      break;
  }
  timeout = rdtsc_oai();
  printf("alpha: inner loop time %llu\n",timeout-timein);

}


void compute_beta16avx2(llr_t* alpha,llr_t* beta,llr_t *m_11,llr_t* m_10,uint16_t frame_length,unsigned char F,int offset8_flag)
{

  int k,rerun_flag=0;

  __m256i *m11p,*m10p;
  register __m256i b0,b1,b2,b3,b4,b5,b6,b7;
  register __m256i m_b0,m_b1,m_b2,m_b3,m_b4,m_b5,m_b6,m_b7;
  register __m256i new0,new1,new2,new3,new4,new5,new6,new7;

  __m256i *beta128,*alpha128,*beta_ptr;
  __m256i beta_max;

  llr_t m11,m10,beta0_16,beta1_16,beta2_16,beta3_16,beta4_16,beta5_16,beta6_16,beta7_16,beta0_2,beta1_2,beta2_2,beta3_2,beta_m;
  llr_t m11_cw2,m10_cw2,beta0_cw2_16,beta1_cw2_16,beta2_cw2_16,beta3_cw2_16,beta4_cw2_16,beta5_cw2_16,beta6_cw2_16,beta7_cw2_16,beta0_2_cw2,beta1_2_cw2,beta2_2_cw2,beta3_2_cw2,beta_m_cw2;
  llr_t beta0,beta1;
  llr_t beta0_cw2,beta1_cw2;

  unsigned long long timein,timeout;

#ifdef DEBUG_LOGMAP
  fprintf(fdavx2,"compute_beta (avx2_16bit), %p,%p,%p,%p,framelength %d,F %d\n",
      beta,m_11,m_10,alpha,frame_length,F);
  fprintf(fdavx2b,"compute_beta (avx2_16bit), %p,%p,%p,%p,framelength %d,F %d\n",
      beta,m_11,m_10,alpha,frame_length,F);
#endif


  // termination for beta initialization

  //  fprintf(fdavx2,"beta init: offset8 %d\n",offset8_flag);
  m11=(int16_t)m_11[(frame_length<<1)+2];
  m10=(int16_t)m_10[(frame_length<<1)+2];

  m11_cw2=(int16_t)m_11[(frame_length<<1)+8+2];
  m10_cw2=(int16_t)m_10[(frame_length<<1)+8+2];

#ifdef DEBUG_LOGMAP
  fprintf(fdavx2,"m11,m10 %d,%d\n",m11,m10);
  fprintf(fdavx2b,"m11,m10 %d,%d\n",m11_cw2,m10_cw2);
#endif

  beta0 = -m11;//M0T_TERM;
  beta1 = m11;//M1T_TERM;
  beta0_cw2 = -m11_cw2;//M0T_TERM;
  beta1_cw2 = m11_cw2;//M1T_TERM;

  m11=(int16_t)m_11[(frame_length<<1)+1];
  m10=(int16_t)m_10[(frame_length<<1)+1];
  m11_cw2=(int16_t)m_11[(frame_length<<1)+1+8];
  m10_cw2=(int16_t)m_10[(frame_length<<1)+1+8];

#ifdef DEBUG_LOGMAP
  fprintf(fdavx2,"m11,m10 %d,%d\n",m11,m10);
  fprintf(fdavx2b,"m11,m10 %d,%d\n",m11_cw2,m10_cw2);
#endif
  beta0_2 = beta0-m11;//+M0T_TERM;
  beta1_2 = beta0+m11;//+M1T_TERM;
  beta2_2 = beta1+m10;//M2T_TERM;
  beta3_2 = beta1-m10;//+M3T_TERM;
  beta0_2_cw2 = beta0_cw2-m11_cw2;//+M0T_TERM;
  beta1_2_cw2 = beta0_cw2+m11_cw2;//+M1T_TERM;
  beta2_2_cw2 = beta1_cw2+m10_cw2;//M2T_TERM;
  beta3_2_cw2 = beta1_cw2-m10_cw2;//+M3T_TERM;

  m11=(int16_t)m_11[frame_length<<1];
  m10=(int16_t)m_10[frame_length<<1];
  m11_cw2=(int16_t)m_11[(frame_length<<1)+8];
  m10_cw2=(int16_t)m_10[(frame_length<<1)+8];
#ifdef DEBUG_LOGMAP
  fprintf(fdavx2,"m11,m10 %d,%d\n",m11,m10);
  fprintf(fdavx2b,"m11,m10 %d,%d\n",m11_cw2,m10_cw2);
#endif
  beta0_16 = beta0_2-m11;//+M0T_TERM;
  beta1_16 = beta0_2+m11;//+M1T_TERM;
  beta2_16 = beta1_2+m10;//+M2T_TERM;
  beta3_16 = beta1_2-m10;//+M3T_TERM;
  beta4_16 = beta2_2-m10;//+M4T_TERM;
  beta5_16 = beta2_2+m10;//+M5T_TERM;
  beta6_16 = beta3_2+m11;//+M6T_TERM;
  beta7_16 = beta3_2-m11;//+M7T_TERM;

  beta0_cw2_16 = beta0_2_cw2-m11_cw2;//+M0T_TERM;
  beta1_cw2_16 = beta0_2_cw2+m11_cw2;//+M1T_TERM;
  beta2_cw2_16 = beta1_2_cw2+m10_cw2;//+M2T_TERM;
  beta3_cw2_16 = beta1_2_cw2-m10_cw2;//+M3T_TERM;
  beta4_cw2_16 = beta2_2_cw2-m10_cw2;//+M4T_TERM;
  beta5_cw2_16 = beta2_2_cw2+m10_cw2;//+M5T_TERM;
  beta6_cw2_16 = beta3_2_cw2+m11_cw2;//+M6T_TERM;
  beta7_cw2_16 = beta3_2_cw2-m11_cw2;//+M7T_TERM;


  beta_m = (beta0_16>beta1_16) ? beta0_16 : beta1_16;
  beta_m = (beta_m>beta2_16) ? beta_m : beta2_16;
  beta_m = (beta_m>beta3_16) ? beta_m : beta3_16;
  beta_m = (beta_m>beta4_16) ? beta_m : beta4_16;
  beta_m = (beta_m>beta5_16) ? beta_m : beta5_16;
  beta_m = (beta_m>beta6_16) ? beta_m : beta6_16;
  beta_m = (beta_m>beta7_16) ? beta_m : beta7_16;

  beta_m_cw2 = (beta0_cw2_16>beta1_cw2_16) ? beta0_cw2_16 : beta1_cw2_16;
  beta_m_cw2 = (beta_m_cw2>beta2_cw2_16) ? beta_m_cw2 : beta2_cw2_16;
  beta_m_cw2 = (beta_m_cw2>beta3_cw2_16) ? beta_m_cw2 : beta3_cw2_16;
  beta_m_cw2 = (beta_m_cw2>beta4_cw2_16) ? beta_m_cw2 : beta4_cw2_16;
  beta_m_cw2 = (beta_m_cw2>beta5_cw2_16) ? beta_m_cw2 : beta5_cw2_16;
  beta_m_cw2 = (beta_m_cw2>beta6_cw2_16) ? beta_m_cw2 : beta6_cw2_16;
  beta_m_cw2 = (beta_m_cw2>beta7_cw2_16) ? beta_m_cw2 : beta7_cw2_16;


  beta0_16=beta0_16-beta_m;
  beta1_16=beta1_16-beta_m;
  beta2_16=beta2_16-beta_m;
  beta3_16=beta3_16-beta_m;
  beta4_16=beta4_16-beta_m;
  beta5_16=beta5_16-beta_m;
  beta6_16=beta6_16-beta_m;
  beta7_16=beta7_16-beta_m;

  beta0_cw2_16=beta0_cw2_16-beta_m_cw2;
  beta1_cw2_16=beta1_cw2_16-beta_m_cw2;
  beta2_cw2_16=beta2_cw2_16-beta_m_cw2;
  beta3_cw2_16=beta3_cw2_16-beta_m_cw2;
  beta4_cw2_16=beta4_cw2_16-beta_m_cw2;
  beta5_cw2_16=beta5_cw2_16-beta_m_cw2;
  beta6_cw2_16=beta6_cw2_16-beta_m_cw2;
  beta7_cw2_16=beta7_cw2_16-beta_m_cw2;

  for (rerun_flag=0;; rerun_flag=1) {

    beta_ptr   = (__m256i*)&beta[frame_length<<4];
    alpha128   = (__m256i*)&alpha[0];

    if (rerun_flag == 0) {
      beta_ptr[0] = alpha128[(frame_length)];
      beta_ptr[1] = alpha128[1+(frame_length)];
      beta_ptr[2] = alpha128[2+(frame_length)];
      beta_ptr[3] = alpha128[3+(frame_length)];
      beta_ptr[4] = alpha128[4+(frame_length)];
      beta_ptr[5] = alpha128[5+(frame_length)];
      beta_ptr[6] = alpha128[6+(frame_length)];
      beta_ptr[7] = alpha128[7+(frame_length)];

#ifdef DEBUG_LOGMAP
      fprintf(fdavx2,"beta init \n");
      fprintf(fdavx2b,"beta init \n");
      print_shorts("b0",(int16_t*)&beta_ptr[0]);
      print_shorts("b1",(int16_t*)&beta_ptr[1]);
      print_shorts("b2",(int16_t*)&beta_ptr[2]);
      print_shorts("b3",(int16_t*)&beta_ptr[3]);
      print_shorts("b4",(int16_t*)&beta_ptr[4]);
      print_shorts("b5",(int16_t*)&beta_ptr[5]);
      print_shorts("b6",(int16_t*)&beta_ptr[6]);
      print_shorts("b7",(int16_t*)&beta_ptr[7]);
#endif
    } else {

      beta128 = (__m256i*)&beta[0];
      beta_ptr[0] = _mm256_srli_si256(beta128[0],2);
      beta_ptr[1] = _mm256_srli_si256(beta128[1],2);
      beta_ptr[2] = _mm256_srli_si256(beta128[2],2);
      beta_ptr[3] = _mm256_srli_si256(beta128[3],2);
      beta_ptr[4] = _mm256_srli_si256(beta128[4],2);
      beta_ptr[5] = _mm256_srli_si256(beta128[5],2);
      beta_ptr[6] = _mm256_srli_si256(beta128[6],2);
      beta_ptr[7] = _mm256_srli_si256(beta128[7],2);
#ifdef DEBUG_LOGMAP
      fprintf(fdavx2,"beta init (second run)\n");
      fprintf(fdavx2b,"beta init (second run)\n");
      print_shorts("b0",(int16_t*)&beta_ptr[0]);
      print_shorts("b1",(int16_t*)&beta_ptr[1]);
      print_shorts("b2",(int16_t*)&beta_ptr[2]);
      print_shorts("b3",(int16_t*)&beta_ptr[3]);
      print_shorts("b4",(int16_t*)&beta_ptr[4]);
      print_shorts("b5",(int16_t*)&beta_ptr[5]);
      print_shorts("b6",(int16_t*)&beta_ptr[6]);
      print_shorts("b7",(int16_t*)&beta_ptr[7]);
#endif
    }


    beta_ptr[0] = _mm256_insert_epi16(beta_ptr[0],beta0_16,7);
    beta_ptr[1] = _mm256_insert_epi16(beta_ptr[1],beta1_16,7);
    beta_ptr[2] = _mm256_insert_epi16(beta_ptr[2],beta2_16,7);
    beta_ptr[3] = _mm256_insert_epi16(beta_ptr[3],beta3_16,7);
    beta_ptr[4] = _mm256_insert_epi16(beta_ptr[4],beta4_16,7);
    beta_ptr[5] = _mm256_insert_epi16(beta_ptr[5],beta5_16,7);
    beta_ptr[6] = _mm256_insert_epi16(beta_ptr[6],beta6_16,7);
    beta_ptr[7] = _mm256_insert_epi16(beta_ptr[7],beta7_16,7);

    beta_ptr[0] = _mm256_insert_epi16(beta_ptr[0],beta0_cw2_16,15);
    beta_ptr[1] = _mm256_insert_epi16(beta_ptr[1],beta1_cw2_16,15);
    beta_ptr[2] = _mm256_insert_epi16(beta_ptr[2],beta2_cw2_16,15);
    beta_ptr[3] = _mm256_insert_epi16(beta_ptr[3],beta3_cw2_16,15);
    beta_ptr[4] = _mm256_insert_epi16(beta_ptr[4],beta4_cw2_16,15);
    beta_ptr[5] = _mm256_insert_epi16(beta_ptr[5],beta5_cw2_16,15);
    beta_ptr[6] = _mm256_insert_epi16(beta_ptr[6],beta6_cw2_16,15);
    beta_ptr[7] = _mm256_insert_epi16(beta_ptr[7],beta7_cw2_16,15);

#ifdef DEBUG_LOGMAP
      fprintf(fdavx2,"beta init (after insert) \n");
      fprintf(fdavx2b,"beta init (after insert) \n");
      print_shorts("b0",(int16_t*)&beta_ptr[0]);
      print_shorts("b1",(int16_t*)&beta_ptr[1]);
      print_shorts("b2",(int16_t*)&beta_ptr[2]);
      print_shorts("b3",(int16_t*)&beta_ptr[3]);
      print_shorts("b4",(int16_t*)&beta_ptr[4]);
      print_shorts("b5",(int16_t*)&beta_ptr[5]);
      print_shorts("b6",(int16_t*)&beta_ptr[6]);
      print_shorts("b7",(int16_t*)&beta_ptr[7]);
#endif
    int loopval=((rerun_flag==0)?0:((frame_length-L)>>3));

    printf("beta: rerun %d => loopval %d\n",rerun_flag,loopval);

    timein = rdtsc_oai();

    m11p = (frame_length>>3)-1+(__m256i*)m_11;
    m10p = (frame_length>>3)-1+(__m256i*)m_10;

    for (k=(frame_length>>3)-1; k>=loopval; k--) {

      
      b4 = _mm256_load_si256(&beta_ptr[4]);
      b5 = _mm256_load_si256(&beta_ptr[5]);
      b6 = _mm256_load_si256(&beta_ptr[6]);
      b7 = _mm256_load_si256(&beta_ptr[7]);

      m_b0 = _mm256_adds_epi16(b4,*m11p);  //m11
      m_b1 = _mm256_subs_epi16(b4,*m11p);  //m00
      m_b2 = _mm256_subs_epi16(b5,*m10p);  //m01
      m_b3 = _mm256_adds_epi16(b5,*m10p);  //m10
      m_b4 = _mm256_adds_epi16(b6,*m10p);  //m10
      m_b5 = _mm256_subs_epi16(b6,*m10p);  //m01
      m_b6 = _mm256_subs_epi16(b7,*m11p);  //m00
      m_b7 = _mm256_adds_epi16(b7,*m11p);  //m11

      b0 = _mm256_load_si256(&beta_ptr[0]);
      b1 = _mm256_load_si256(&beta_ptr[1]);
      b2 = _mm256_load_si256(&beta_ptr[2]);
      b3 = _mm256_load_si256(&beta_ptr[3]);

      new0 = _mm256_subs_epi16(b0,*m11p);  //m00
      new1 = _mm256_adds_epi16(b0,*m11p);  //m11
      new2 = _mm256_adds_epi16(b1,*m10p);  //m10
      new3 = _mm256_subs_epi16(b1,*m10p);  //m01
      new4 = _mm256_subs_epi16(b2,*m10p);  //m01
      new5 = _mm256_adds_epi16(b2,*m10p);  //m10
      new6 = _mm256_adds_epi16(b3,*m11p);  //m11
      new7 = _mm256_subs_epi16(b3,*m11p);  //m00


      b0 = _mm256_max_epi16(m_b0,new0);
      b1 = _mm256_max_epi16(m_b1,new1);
      b2 = _mm256_max_epi16(m_b2,new2);
      b3 = _mm256_max_epi16(m_b3,new3);
      b4 = _mm256_max_epi16(m_b4,new4);
      b5 = _mm256_max_epi16(m_b5,new5);
      b6 = _mm256_max_epi16(m_b6,new6);
      b7 = _mm256_max_epi16(m_b7,new7);

      beta_max = _mm256_max_epi16(b0,b1);
      beta_max = _mm256_max_epi16(beta_max   ,b2);
      beta_max = _mm256_max_epi16(beta_max   ,b3);
      beta_max = _mm256_max_epi16(beta_max   ,b4);
      beta_max = _mm256_max_epi16(beta_max   ,b5);
      beta_max = _mm256_max_epi16(beta_max   ,b6);
      beta_max = _mm256_max_epi16(beta_max   ,b7);

      beta_ptr-=8;
      m11p--;
      m10p--;

      beta_ptr[0] = _mm256_subs_epi16(b0,beta_max);
      beta_ptr[1] = _mm256_subs_epi16(b1,beta_max);
      beta_ptr[2] = _mm256_subs_epi16(b2,beta_max);
      beta_ptr[3] = _mm256_subs_epi16(b3,beta_max);
      beta_ptr[4] = _mm256_subs_epi16(b4,beta_max);
      beta_ptr[5] = _mm256_subs_epi16(b5,beta_max);
      beta_ptr[6] = _mm256_subs_epi16(b6,beta_max);
      beta_ptr[7] = _mm256_subs_epi16(b7,beta_max);

#ifdef DEBUG_LOGMAP
      fprintf(fdavx2,"Loop index %d, mb\n",k);
      fprintf(fdavx2,"beta init (after max)\n");
      fprintf(fdavx2b,"Loop index %d, mb\n",k);
      fprintf(fdavx2b,"beta init (after max)\n");

      print_shorts("b0",(int16_t*)&beta_ptr[0]);
      print_shorts("b1",(int16_t*)&beta_ptr[1]);
      print_shorts("b2",(int16_t*)&beta_ptr[2]);
      print_shorts("b3",(int16_t*)&beta_ptr[3]);
      print_shorts("b4",(int16_t*)&beta_ptr[4]);
      print_shorts("b5",(int16_t*)&beta_ptr[5]);
      print_shorts("b6",(int16_t*)&beta_ptr[6]);
      print_shorts("b7",(int16_t*)&beta_ptr[7]);

#endif
    }
    timeout = rdtsc_oai();
    printf("beta: inner loop time %llu\n",timeout-timein);

    if (rerun_flag==1)
      break;
  }
}

void compute_ext16avx2(llr_t* alpha,llr_t* beta,llr_t* m_11,llr_t* m_10,llr_t* ext, llr_t* systematic,uint16_t frame_length)
{

  __m256i *alpha128=(__m256i *)alpha;
  __m256i *beta128=(__m256i *)beta;
  __m256i *m11_128,*m10_128,*ext_128;
  __m256i *alpha_ptr,*beta_ptr;
  __m256i m00_1,m00_2,m00_3,m00_4;
  __m256i m01_1,m01_2,m01_3,m01_4;
  __m256i m10_1,m10_2,m10_3,m10_4;
  __m256i m11_1,m11_2,m11_3,m11_4;

  int k;

  //
  // LLR computation, 8 consequtive bits per loop
  //

#ifdef DEBUG_LOGMAP
  fprintf(fdavx2,"compute_ext (avx2_16bit), %p, %p, %p, %p, %p, %p ,framelength %d\n",alpha,beta,m_11,m_10,ext,systematic,frame_length);
  fprintf(fdavx2b,"compute_ext (avx2_16bit), %p, %p, %p, %p, %p, %p ,framelength %d\n",alpha,beta,m_11,m_10,ext,systematic,frame_length);
#endif

  alpha_ptr = alpha128;
  beta_ptr = &beta128[8];


  for (k=0; k<(frame_length>>3); k++) {


    m11_128        = (__m256i*)&m_11[k<<4];
    m10_128        = (__m256i*)&m_10[k<<4];
    ext_128        = (__m256i*)&ext[k<<4];

    /*
      fprintf(fdavx2,"EXT %03d\n",k);
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
    m00_4 = _mm256_adds_epi16(alpha_ptr[7],beta_ptr[3]); //ALPHA_BETA_4m00;
    m11_4 = _mm256_adds_epi16(alpha_ptr[7],beta_ptr[7]); //ALPHA_BETA_4m11;
    m00_3 = _mm256_adds_epi16(alpha_ptr[6],beta_ptr[7]); //ALPHA_BETA_3m00;
    m11_3 = _mm256_adds_epi16(alpha_ptr[6],beta_ptr[3]); //ALPHA_BETA_3m11;
    m00_2 = _mm256_adds_epi16(alpha_ptr[1],beta_ptr[4]); //ALPHA_BETA_2m00;
    m11_2 = _mm256_adds_epi16(alpha_ptr[1],beta_ptr[0]); //ALPHA_BETA_2m11;
    m11_1 = _mm256_adds_epi16(alpha_ptr[0],beta_ptr[4]); //ALPHA_BETA_1m11;
    m00_1 = _mm256_adds_epi16(alpha_ptr[0],beta_ptr[0]); //ALPHA_BETA_1m00;
    m01_4 = _mm256_adds_epi16(alpha_ptr[5],beta_ptr[6]); //ALPHA_BETA_4m01;
    m10_4 = _mm256_adds_epi16(alpha_ptr[5],beta_ptr[2]); //ALPHA_BETA_4m10;
    m01_3 = _mm256_adds_epi16(alpha_ptr[4],beta_ptr[2]); //ALPHA_BETA_3m01;
    m10_3 = _mm256_adds_epi16(alpha_ptr[4],beta_ptr[6]); //ALPHA_BETA_3m10;
    m01_2 = _mm256_adds_epi16(alpha_ptr[3],beta_ptr[1]); //ALPHA_BETA_2m01;
    m10_2 = _mm256_adds_epi16(alpha_ptr[3],beta_ptr[5]); //ALPHA_BETA_2m10;
    m10_1 = _mm256_adds_epi16(alpha_ptr[2],beta_ptr[1]); //ALPHA_BETA_1m10;
    m01_1 = _mm256_adds_epi16(alpha_ptr[2],beta_ptr[5]); //ALPHA_BETA_1m01;
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
    m01_1 = _mm256_max_epi16(m01_1,m01_2);
    m01_1 = _mm256_max_epi16(m01_1,m01_3);
    m01_1 = _mm256_max_epi16(m01_1,m01_4);
    m00_1 = _mm256_max_epi16(m00_1,m00_2);
    m00_1 = _mm256_max_epi16(m00_1,m00_3);
    m00_1 = _mm256_max_epi16(m00_1,m00_4);
    m10_1 = _mm256_max_epi16(m10_1,m10_2);
    m10_1 = _mm256_max_epi16(m10_1,m10_3);
    m10_1 = _mm256_max_epi16(m10_1,m10_4);
    m11_1 = _mm256_max_epi16(m11_1,m11_2);
    m11_1 = _mm256_max_epi16(m11_1,m11_3);
    m11_1 = _mm256_max_epi16(m11_1,m11_4);

    //      print_shorts("m11_1:",&m11_1);

    m01_1 = _mm256_subs_epi16(m01_1,*m10_128);
    m00_1 = _mm256_subs_epi16(m00_1,*m11_128);
    m10_1 = _mm256_adds_epi16(m10_1,*m10_128);
    m11_1 = _mm256_adds_epi16(m11_1,*m11_128);

    //      print_shorts("m10_1:",&m10_1);
    //      print_shorts("m11_1:",&m11_1);
    m01_1 = _mm256_max_epi16(m01_1,m00_1);
    m10_1 = _mm256_max_epi16(m10_1,m11_1);
    //      print_shorts("m01_1:",&m01_1);
    //      print_shorts("m10_1:",&m10_1);

    *ext_128 = _mm256_subs_epi16(m10_1,m01_1);

#ifdef DEBUG_LOGMAP
    fprintf(fdavx2,"ext %p\n",ext_128);
    fprintf(fdavx2b,"ext %p\n",ext_128);
    print_shorts("ext:",(int16_t*)ext_128);
    print_shorts("m11:",(int16_t*)m11_128);
    print_shorts("m10:",(int16_t*)m10_128);
    print_shorts("m10_1:",(int16_t*)&m10_1);
    print_shorts("m01_1:",(int16_t*)&m01_1);


#endif    

    alpha_ptr+=8;
    beta_ptr+=8;
    }
}



//int pi2[n],pi3[n+8],pi5[n+8],pi4[n+8],pi6[n+8],
int *pi2tab16avx2[188],*pi5tab16avx2[188],*pi4tab16avx2[188],*pi6tab16avx2[188];

void free_td16avx2(void)
{
  int ind;

  for (ind=0; ind<188; ind++) {
    free_and_zero(pi2tab16avx2[ind]);
    free_and_zero(pi5tab16avx2[ind]);
    free_and_zero(pi4tab16avx2[ind]);
    free_and_zero(pi6tab16avx2[ind]);
  }
}

void init_td16avx2(void)
{

  int ind,i,i2,i3,j,n,pi,pi2_i,pi2_pi;
  short * base_interleaver;

  for (ind=0; ind<188; ind++) {
    n = f1f2mat[ind].nb_bits;
    base_interleaver=il_tb+f1f2mat[ind].beg_index;
#ifdef MEX
    // This is needed for the Mex implementation to make the memory persistent
    pi2tab16[ind] = mxMalloc((n+8)*sizeof(int));
    pi5tab16[ind] = mxMalloc((n+8)*sizeof(int));
    pi4tab16[ind] = mxMalloc((n+8)*sizeof(int));
    pi6tab16[ind] = mxMalloc((n+8)*sizeof(int));
#else
    pi2tab16avx2[ind] = malloc((n+8)*sizeof(int));
    pi5tab16avx2[ind] = malloc((n+8)*sizeof(int));
    pi4tab16avx2[ind] = malloc((n+8)*sizeof(int));
    pi6tab16avx2[ind] = malloc((n+8)*sizeof(int));
#endif

    //    fprintf(fdavx2,"Interleaver index %d\n",ind);
    for (i=i2=0; i2<8; i2++) {
      j=i2;

      for (i3=0; i3<(n>>3); i3++,i++,j+=8) {

        //    if (j>=n)
        //      j-=(n-1);

        pi2tab16avx2[ind][i]  = ((j>>3)<<4) + (j&7); // 16*floor(j/8) + j mod8, which allows the second codeword to be in pi[i] + 8 
	//	fprintf(fdavx2,"pi2[%d] = %d(%d)\n",i, pi2tab16avx2[ind][i],j);
      }
    }

    for (i=0; i<n; i++) {
      pi = base_interleaver[i];//(uint32_t)threegpplte_interleaver(f1,f2,n);
      pi2_i  = ((pi2tab16avx2[ind][i]>>4)<<3)+(pi2tab16avx2[ind][i]&7);
      pi2_pi = ((pi2tab16avx2[ind][pi]>>4)<<3)+(pi2tab16avx2[ind][pi]&7);
      pi4tab16avx2[ind][pi2_i]  = pi2tab16avx2[ind][pi];
      pi5tab16avx2[ind][pi2_pi] = pi2tab16avx2[ind][i];
      pi6tab16avx2[ind][pi]     = pi2tab16avx2[ind][i];
    }

  }
}

unsigned char phy_threegpplte_turbo_decoder16avx2(int16_t *y,
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
						  time_stats_t *intl2_stats)
{

  /*  y is a pointer to the input
      decoded_bytes is a pointer to the decoded output
      n is the size in bits of the coded block, with the tail */


  llr_t systematic0[2*(n+16)] __attribute__ ((aligned(32)));
  llr_t systematic1[2*(n+16)] __attribute__ ((aligned(32)));
  llr_t systematic2[2*(n+16)] __attribute__ ((aligned(32)));
  llr_t yparity1[2*(n+16)] __attribute__ ((aligned(32)));
  llr_t yparity2[2*(n+16)] __attribute__ ((aligned(32)));

  llr_t ext[2*(n+128)] __attribute__((aligned(32)));
  llr_t ext2[2*(n+128)] __attribute__((aligned(32)));

  llr_t alpha[(n+16)*16] __attribute__ ((aligned(32)));
  llr_t beta[(n+16)*16] __attribute__ ((aligned(32)));
  llr_t m11[2*(n+16)] __attribute__ ((aligned(32)));
  llr_t m10[2*(n+16)] __attribute__ ((aligned(32)));


  int *pi2_p,*pi4_p,*pi5_p,*pi6_p;
  llr_t *s,*s1,*s2,*yp1,*yp2,*yp,*yp_cw2;
  uint32_t i,j,iind;//,pi;
  uint8_t iteration_cnt=0;
  uint32_t crc,oldcrc,crc_cw2,oldcrc_cw2,crc_len;
  uint8_t temp;
  uint32_t db;


  __m256i tmp, zeros=_mm256_setzero_si256();


  int offset8_flag=0;

#ifdef DEBUG_LOGMAP
  fdavx2 = fopen("dump_avx2.txt","w");
  fdavx2b = fopen("dump_avx2b.txt","w");

  printf("tc avx2_16 (y,y2) %p,%p\n",y,y2);
#endif
  if (crc_type > 3) {
    printf("Illegal crc length!\n");
    return 255;
  }


  start_meas(init_stats);



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


  s = systematic0;
  s1 = systematic1;
  s2 = systematic2;
  yp1 = yparity1;
  yp2 = yparity2;

  pi2_p    = &pi2tab16avx2[iind][0];
  for (i=0,j=0; i<n; i++) {
    s[*pi2_p]     = y[j];
    s[*pi2_p+8]   = y2[j++];
    yp1[*pi2_p]   = y[j];
    yp1[*pi2_p+8] = y2[j++];
    yp2[*pi2_p]   = y[j];
    yp2[(*pi2_p++)+8] = y2[j++];
  }    
  yp=(llr_t*)&y[j];
  yp_cw2=(llr_t*)&y2[j];

  // Termination
  for (i=0; i<3; i++) {
    s[(n<<1)+i]    = *yp;
    s1[(n<<1)+i]   = *yp;
    s2[(n<<1)+i]   = *yp;
    yp++;
    yp1[(n<<1)+i]  = *yp;
    yp++;

    s[(n<<1)+i+8]   = *yp_cw2;
    s1[(n<<1)+i+8]  = *yp_cw2;
    s2[(n<<1)+i+8]  = *yp_cw2;
    yp_cw2++;
    yp1[(n<<1)+i+8] = *yp_cw2;
    yp_cw2++;
#ifdef DEBUG_LOGMAP
    fprintf(fdavx2,"Term 1 (%d): %d %d\n",n+i,s[(n<<1)+i],yp1[(n<<1)+i]);
    fprintf(fdavx2b,"Term 1 (%d): %d %d\n",n+i,s[(n<<1)+i+8],yp1[(n<<1)+i+8]);
#endif //DEBUG_LOGMAP
  }

  for (i=16; i<19; i++) {
    s[(n<<1)+i]  = *yp;
    s1[(n<<1)+i] = *yp;
    s2[(n<<1)+i] = *yp;
    yp++;
    yp2[(n<<1)+(i-16)] = *yp;
    yp++;

    s[(n<<1)+i+8]= *yp_cw2;
    s1[(n<<1)+i+8] = *yp_cw2 ;
    s2[(n<<1)+i+8] = *yp_cw2;
    yp_cw2++;
    yp2[(n<<1)+i-16+8] = *yp_cw2;
    yp_cw2++;
#ifdef DEBUG_LOGMAP
    fprintf(fdavx2,"Term 2 (%d): %d %d\n",n+i-3-8,s[(n<<1)+i],yp2[(n<<1)+i-16]);
    fprintf(fdavx2b,"Term 2 (%d): %d %d\n",n+i-3-8,s[(n<<1)+i+8],yp2[(n<<1)+i-16+8]);
#endif //DEBUG_LOGMAP
  }

#ifdef DEBUG_LOGMAP
  fprintf(fdavx2,"\n");
  fprintf(fdavx2b,"\n");
#endif //DEBUG_LOGMAP

  stop_meas(init_stats);

  // do log_map from first parity bit

  log_map16avx2(systematic0,yparity1,m11,m10,alpha,beta,ext,n,0,F,offset8_flag,alpha_stats,beta_stats,gamma_stats,ext_stats);

  while (iteration_cnt++ < max_iterations) {

#ifdef DEBUG_LOGMAP
    fprintf(fdavx2,"\n*******************ITERATION %d (n %d), ext %p\n\n",iteration_cnt,n,ext);
    fprintf(fdavx2b,"\n*******************ITERATION %d (n %d), ext %p\n\n",iteration_cnt,n,ext);
#endif //DEBUG_LOGMAP
 
    start_meas(intl1_stats);

    pi4_p=pi4tab16avx2[iind];

    for (i=0; i<(n>>3); i++) { // steady-state portion

      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[*pi4_p],0);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[8+*pi4_p++],8);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[*pi4_p],1);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[8+*pi4_p++],9);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[*pi4_p],2);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[8+*pi4_p++],10);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[*pi4_p],3);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[8+*pi4_p++],11);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[*pi4_p],4);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[8+*pi4_p++],12);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[*pi4_p],5);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[8+*pi4_p++],13);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[*pi4_p],6);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[8+*pi4_p++],14);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[*pi4_p],7);
      ((__m256i *)systematic2)[i]=_mm256_insert_epi16(((__m256i *)systematic2)[i],ext[8+*pi4_p++],15);
#ifdef DEBUG_LOGMAP
      print_shorts("syst2",(int16_t*)&((__m256i *)systematic2)[i]);
#endif
    }

    stop_meas(intl1_stats);

    // do log_map from second parity bit

    log_map16avx2(systematic2,yparity2,m11,m10,alpha,beta,ext2,n,1,F,offset8_flag,alpha_stats,beta_stats,gamma_stats,ext_stats);



    pi5_p=pi5tab16avx2[iind];

    for (i=0; i<(n>>3); i++) {

      tmp=_mm256_insert_epi16(tmp,ext2[*pi5_p],0);
      tmp=_mm256_insert_epi16(tmp,ext2[8+*pi5_p++],8);
      tmp=_mm256_insert_epi16(tmp,ext2[*pi5_p],1);
      tmp=_mm256_insert_epi16(tmp,ext2[8+*pi5_p++],9);
      tmp=_mm256_insert_epi16(tmp,ext2[*pi5_p],2);
      tmp=_mm256_insert_epi16(tmp,ext2[8+*pi5_p++],10);
      tmp=_mm256_insert_epi16(tmp,ext2[*pi5_p],3);
      tmp=_mm256_insert_epi16(tmp,ext2[8+*pi5_p++],11);
      tmp=_mm256_insert_epi16(tmp,ext2[*pi5_p],4);
      tmp=_mm256_insert_epi16(tmp,ext2[8+*pi5_p++],12);
      tmp=_mm256_insert_epi16(tmp,ext2[*pi5_p],5);
      tmp=_mm256_insert_epi16(tmp,ext2[8+*pi5_p++],13);
      tmp=_mm256_insert_epi16(tmp,ext2[*pi5_p],6);
      tmp=_mm256_insert_epi16(tmp,ext2[8+*pi5_p++],14);
      tmp=_mm256_insert_epi16(tmp,ext2[*pi5_p],7);
      tmp=_mm256_insert_epi16(tmp,ext2[8+*pi5_p++],15);
      ((__m256i *)systematic1)[i] = _mm256_adds_epi16(_mm256_subs_epi16(tmp,((__m256i*)ext)[i]),((__m256i *)systematic0)[i]);
#ifdef DEBUG_LOGMAP
      print_shorts("syst1",(int16_t*)&((__m256i *)systematic1)[i]);
#endif
    }

    if (iteration_cnt>1) {
      start_meas(intl2_stats);
      pi6_p=pi6tab16avx2[iind];

      for (i=0; i<(n>>3); i++) {

        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p],7);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[8+*pi6_p++],15);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p],6);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[8+*pi6_p++],14);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p],5);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[8+*pi6_p++],13);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p],4);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[8+*pi6_p++],12);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p],3);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[8+*pi6_p++],11);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p],2);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[8+*pi6_p++],10);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p],1);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[8+*pi6_p++],9);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[*pi6_p],0);
        tmp=_mm256_insert_epi16(tmp, ((llr_t*)ext2)[8+*pi6_p++],8);
#ifdef DEBUG_LOGMAP
	print_shorts("tmp",(int16_t*)&tmp);
#endif
        tmp=_mm256_cmpgt_epi8(_mm256_packs_epi16(tmp,zeros),zeros);
        db=(uint32_t)_mm256_movemask_epi8(tmp);
	decoded_bytes[i]=db&0xff;
	decoded_bytes2[i]=(uint8_t)(db>>16)&0xff;
#ifdef DEBUG_LOGMAP
	print_shorts("tmp",(int16_t*)&tmp);
	fprintf(fdavx2,"decoded_bytes[%d] %x (%x)\n",i,decoded_bytes[i],db);
	fprintf(fdavx2b,"decoded_bytes[%d] %x (%x)\n",i,decoded_bytes2[i],db);
#endif
      }
    }

    // check status on output
    if (iteration_cnt>1) {
      oldcrc= *((uint32_t *)(&decoded_bytes[(n>>3)-crc_len]));

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

      // second CW
      oldcrc_cw2= *((uint32_t *)(&decoded_bytes2[(n>>3)-crc_len]));

      switch (crc_type) {

      case CRC24_A:
        oldcrc_cw2&=0x00ffffff;
        crc_cw2 = crc24a(&decoded_bytes2[F>>3],
			 n-24-F)>>8;
        temp=((uint8_t *)&crc_cw2)[2];
        ((uint8_t *)&crc_cw2)[2] = ((uint8_t *)&crc_cw2)[0];
        ((uint8_t *)&crc_cw2)[0] = temp;
        break;

      case CRC24_B:
        oldcrc_cw2&=0x00ffffff;
        crc_cw2 = crc24b(decoded_bytes2,
			 n-24)>>8;
        temp=((uint8_t *)&crc_cw2)[2];
        ((uint8_t *)&crc_cw2)[2] = ((uint8_t *)&crc_cw2)[0];
        ((uint8_t *)&crc_cw2)[0] = temp;
        break;

      case CRC16:
        oldcrc_cw2&=0x0000ffff;
        crc_cw2 = crc16(decoded_bytes2,
			n-16)>>16;
        break;

      case CRC8:
        oldcrc_cw2&=0x000000ff;
        crc_cw2 = crc8(decoded_bytes2,
		       n-8)>>24;
        break;

      default:
        printf("FATAL: 3gpplte_turbo_decoder_sse.c: Unknown CRC\n");
        return(255);
        break;
      }

      stop_meas(intl2_stats);

#ifdef DEBUG_LOGMAP
      fprintf(fdavx2,"oldcrc %x, crc %x, oldcrc_cw2 %x, crc_cw2 %x\n",oldcrc,crc,oldcrc_cw2,crc_cw2);
      fprintf(fdavx2b,"oldcrc %x, crc %x, oldcrc_cw2 %x, crc_cw2 %x\n",oldcrc,crc,oldcrc_cw2,crc_cw2);
#endif

      if (crc == oldcrc && crc_cw2 == oldcrc_cw2) {
        return(iteration_cnt);
      }
    }

    // do log_map from first parity bit
    if (iteration_cnt < max_iterations) {
      log_map16avx2(systematic1,yparity1,m11,m10,alpha,beta,ext,n,0,F,offset8_flag,alpha_stats,beta_stats,gamma_stats,ext_stats);

      __m256i* ext_128=(__m256i*) ext;
      __m256i* s1_128=(__m256i*) systematic1;
      __m256i* s0_128=(__m256i*) systematic0;
      int myloop=n>>3;

      for (i=0; i<myloop; i++) {

        *ext_128=_mm256_adds_epi16(_mm256_subs_epi16(*ext_128,*s1_128++),*s0_128++);
        ext_128++;
      }
    }
  }

  //  fprintf(fdavx2,"crc %x, oldcrc %x\n",crc,oldcrc);


  _mm_empty();
  _m_empty();

#ifdef DEBUG_LOGMAP
  fclose(fdavx2);
#endif
  return(iteration_cnt);
}
#else  //__AVX2__
unsigned char phy_threegpplte_turbo_decoder16avx2(int16_t *y,
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
						  time_stats_t *intl2_stats)
{
   return 0;
}
void free_td16avx2(void)
{

}

void init_td16avx2(void)
{
    
}

#endif //__AVX2__


