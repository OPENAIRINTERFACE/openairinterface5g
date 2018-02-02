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

#include "defs.h"
#include <stdio.h>

#if defined(__x86_64__) || defined(__i386__)
int16_t conjug[8]__attribute__((aligned(16))) = {-1,1,-1,1,-1,1,-1,1} ;
int16_t conjug2[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1} ;

#define simd_q15_t __m128i
#define simdshort_q15_t __m64
#define set1_int16(a) _mm_set1_epi16(a)
#define setr_int16(a0, a1, a2, a3, a4, a5, a6, a7) _mm_setr_epi16(a0, a1, a2, a3, a4, a5, a6, a7 )
#elif defined(__arm__)
int16_t conjug[4]__attribute__((aligned(16))) = {-1,1,-1,1} ;
#define simd_q15_t int16x8_t
#define simdshort_q15_t int16x4_t
#define _mm_empty()
#define _m_empty()
#endif

int mult_cpx_conj_vector(int16_t *x1,
                         int16_t *x2,
                         int16_t *y,
                         uint32_t N,
                         int output_shift,
			 int madd)
{
  // Multiply elementwise the complex conjugate of x1 with x2.
  // x1       - input 1    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  //            We assume x1 with a dinamic of 15 bit maximum
  //
  // x2       - input 2    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  //            We assume x2 with a dinamic of 14 bit maximum
  ///
  // y        - output     in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  //
  // N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
  //
  // output_shift  - shift to be applied to generate output
  //
  // madd - add the output to y

  uint32_t i;                 // loop counter

  simd_q15_t *x1_128;
  simd_q15_t *x2_128;
  simd_q15_t *y_128;
#if defined(__x86_64__) || defined(__i386__)
  simd_q15_t tmp_re,tmp_im;
  simd_q15_t tmpy0,tmpy1;

#elif defined(__arm__)
  int32x4_t tmp_re,tmp_im;
  int32x4_t tmp_re1,tmp_im1;
  int16x4x2_t tmpy;
  int32x4_t shift = vdupq_n_s32(-output_shift);
#endif

  x1_128 = (simd_q15_t *)&x1[0];
  x2_128 = (simd_q15_t *)&x2[0];
  y_128  = (simd_q15_t *)&y[0];


  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>2); i++) {
#if defined(__x86_64__) || defined(__i386__)
    tmp_re = _mm_madd_epi16(*x1_128,*x2_128);
    tmp_im = _mm_shufflelo_epi16(*x1_128,_MM_SHUFFLE(2,3,0,1));
    tmp_im = _mm_shufflehi_epi16(tmp_im,_MM_SHUFFLE(2,3,0,1));
    tmp_im = _mm_sign_epi16(tmp_im,*(__m128i*)&conjug[0]);
    tmp_im = _mm_madd_epi16(tmp_im,*x2_128);
    tmp_re = _mm_srai_epi32(tmp_re,output_shift);
    tmp_im = _mm_srai_epi32(tmp_im,output_shift);
    tmpy0  = _mm_unpacklo_epi32(tmp_re,tmp_im);
    tmpy1  = _mm_unpackhi_epi32(tmp_re,tmp_im);
    if (madd==0)
      *y_128 = _mm_packs_epi32(tmpy0,tmpy1);
    else
      *y_128 += _mm_packs_epi32(tmpy0,tmpy1);

#elif defined(__arm__)

    tmp_re  = vmull_s16(((simdshort_q15_t *)x1_128)[0], ((simdshort_q15_t*)x2_128)[0]);
    //tmp_re = [Re(x1[0])Re(x2[0]) Im(x1[0])Im(x2[0]) Re(x1[1])Re(x2[1]) Im(x1[1])Im(x2[1])]
    tmp_re1 = vmull_s16(((simdshort_q15_t *)x1_128)[1], ((simdshort_q15_t*)x2_128)[1]);
    //tmp_re1 = [Re(x1[1])Re(x2[1]) Im(x1[1])Im(x2[1]) Re(x1[1])Re(x2[2]) Im(x1[1])Im(x2[2])]
    tmp_re  = vcombine_s32(vpadd_s32(vget_low_s32(tmp_re),vget_high_s32(tmp_re)),
                           vpadd_s32(vget_low_s32(tmp_re1),vget_high_s32(tmp_re1)));
    //tmp_re = [Re(ch[0])Re(rx[0])+Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1])+Im(ch[1])Im(ch[1]) Re(ch[2])Re(rx[2])+Im(ch[2]) Im(ch[2]) Re(ch[3])Re(rx[3])+Im(ch[3])Im(ch[3])]

    tmp_im  = vmull_s16(vrev32_s16(vmul_s16(((simdshort_q15_t*)x2_128)[0],*(simdshort_q15_t*)conjug)), ((simdshort_q15_t*)x1_128)[0]);
    //tmp_im = [-Im(ch[0])Re(rx[0]) Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1]) Re(ch[1])Im(rx[1])]
    tmp_im1 = vmull_s16(vrev32_s16(vmul_s16(((simdshort_q15_t*)x2_128)[1],*(simdshort_q15_t*)conjug)), ((simdshort_q15_t*)x1_128)[1]);
    //tmp_im1 = [-Im(ch[2])Re(rx[2]) Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3]) Re(ch[3])Im(rx[3])]
    tmp_im  = vcombine_s32(vpadd_s32(vget_low_s32(tmp_im),vget_high_s32(tmp_im)),
                           vpadd_s32(vget_low_s32(tmp_im1),vget_high_s32(tmp_im1)));
    //tmp_im = [-Im(ch[0])Re(rx[0])+Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1])+Re(ch[1])Im(rx[1]) -Im(ch[2])Re(rx[2])+Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3])+Re(ch[3])Im(rx[3])]

    tmp_re = vqshlq_s32(tmp_re,shift);
    tmp_im = vqshlq_s32(tmp_im,shift);
    tmpy   = vzip_s16(vmovn_s32(tmp_re),vmovn_s32(tmp_im));
    if (madd==0)
      *y_128 = vcombine_s16(tmpy.val[0],tmpy.val[1]);
    else
      *y_128 += vcombine_s16(tmpy.val[0],tmpy.val[1]);
#endif
    x1_128++;
    x2_128++;
    y_128++;
  }


  _mm_empty();
  _m_empty();

  return(0);
}

int mult_cpx_vector(int16_t *x1, //Q15
                    int16_t *x2,//Q13
                    int16_t *y,
                    uint32_t N,
                    int output_shift)
{
  // Multiply elementwise x1 with x2.
  // x1       - input 1    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  //            We assume x1 with a dinamic of 15 bit maximum
  //
  // x2       - input 2    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  //            We assume x2 with a dinamic of 14 bit maximum
  ///
  // y        - output     in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  //
  // N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
  //
  // output_shift  - shift to be applied to generate output
  uint32_t i;                 // loop counter
  simd_q15_t *x1_128;
  simd_q15_t *x2_128;
  simd_q15_t *y_128;
  simd_q15_t tmp_re,tmp_im;
  simd_q15_t tmpy0,tmpy1;
  x1_128 = (simd_q15_t *)&x1[0];
  x2_128 = (simd_q15_t *)&x2[0];
  y_128  = (simd_q15_t *)&y[0];
  //print_shorts("x1_128:",&x1_128[0]);
 // print_shorts("x2_128:",&x2_128[0]);

  //right shift by 13 while p_a * x0 and 15 while
  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>2); i++) {
    tmp_re = _mm_sign_epi16(*x1_128,*(__m128i*)&conjug2[0]);// Q15
    //print_shorts("tmp_re1:",&tmp_re[i]);
    tmp_re = _mm_madd_epi16(tmp_re,*x2_128); //Q28
    //print_ints("tmp_re2:",&tmp_re[i]);
    tmp_im = _mm_shufflelo_epi16(*x1_128,_MM_SHUFFLE(2,3,0,1)); //Q15
    //print_shorts("tmp_im1:",&tmp_im[i]);
    tmp_im = _mm_shufflehi_epi16(tmp_im,_MM_SHUFFLE(2,3,0,1)); //Q15
    //print_shorts("tmp_im2:",&tmp_im[i]);
    tmp_im = _mm_madd_epi16(tmp_im, *x2_128); //Q28
    //print_ints("tmp_im3:",&tmp_im[i]);
    tmp_re = _mm_srai_epi32(tmp_re,output_shift);//Q(28-shift)
    //print_ints("tmp_re shifted:",&tmp_re[i]);
    tmp_im = _mm_srai_epi32(tmp_im,output_shift); //Q(28-shift)
    //print_ints("tmp_im shifted:",&tmp_im[i]);
    tmpy0  = _mm_unpacklo_epi32(tmp_re,tmp_im); //Q(28-shift)
    //print_ints("unpack lo :",&tmpy0[i]);
    tmpy1  = _mm_unpackhi_epi32(tmp_re,tmp_im); //Q(28-shift)
    //print_ints("mrc rho0:",&tmpy1[i]);
    *y_128 = _mm_packs_epi32(tmpy0,tmpy1); //must be Q15
    //print_shorts("*y_128:",&y_128[i]);
    x1_128++;
    x2_128++;
    y_128++;
  }
  _mm_empty();
  _m_empty();
  return(0);
}

int multadd_cpx_vector(int16_t *x1,
                    int16_t *x2,
                    int16_t *y,
                    uint8_t zero_flag,
                    uint32_t N,
                    int output_shift)
{
  // Multiply elementwise the complex conjugate of x1 with x2.
  // x1       - input 1    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  //            We assume x1 with a dinamic of 15 bit maximum
  //
  // x2       - input 2    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  //            We assume x2 with a dinamic of 14 bit maximum
  ///
  // y        - output     in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  //
  // zero_flag - Set output (y) to zero prior to disable accumulation
  //
  // N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
  //
  // output_shift  - shift to be applied to generate output
  uint32_t i;                 // loop counter
  simd_q15_t *x1_128;
  simd_q15_t *x2_128;
  simd_q15_t *y_128;
#if defined(__x86_64__) || defined(__i386__)
  simd_q15_t tmp_re,tmp_im;
  simd_q15_t tmpy0,tmpy1;
#elif defined(__arm__)
  int32x4_t tmp_re,tmp_im;
  int32x4_t tmp_re1,tmp_im1;
  int16x4x2_t tmpy;
  int32x4_t shift = vdupq_n_s32(-output_shift);
#endif
  x1_128 = (simd_q15_t *)&x1[0];
  x2_128 = (simd_q15_t *)&x2[0];
  y_128  = (simd_q15_t *)&y[0];
  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>2); i++) {
#if defined(__x86_64__) || defined(__i386__)
    tmp_re = _mm_sign_epi16(*x1_128,*(__m128i*)&conjug2[0]);
    tmp_re = _mm_madd_epi16(tmp_re,*x2_128);
    tmp_im = _mm_shufflelo_epi16(*x1_128,_MM_SHUFFLE(2,3,0,1));
    tmp_im = _mm_shufflehi_epi16(tmp_im,_MM_SHUFFLE(2,3,0,1));
    tmp_im = _mm_madd_epi16(tmp_im,*x2_128);
    tmp_re = _mm_srai_epi32(tmp_re,output_shift);
    tmp_im = _mm_srai_epi32(tmp_im,output_shift);
    tmpy0  = _mm_unpacklo_epi32(tmp_re,tmp_im);
    //print_ints("unpack lo:",&tmpy0[i]);
    tmpy1  = _mm_unpackhi_epi32(tmp_re,tmp_im);
    //print_ints("unpack hi:",&tmpy1[i]);
    if (zero_flag == 1)
      *y_128 = _mm_packs_epi32(tmpy0,tmpy1);
    else
      *y_128 = _mm_adds_epi16(*y_128,_mm_packs_epi32(tmpy0,tmpy1));
    //print_shorts("*y_128:",&y_128[i]);
#elif defined(__arm__)
    msg("mult_cpx_vector not implemented for __arm__");
#endif
    x1_128++;
    x2_128++;
    y_128++;
  }
  _mm_empty();
  _m_empty();
  return(0);
}
