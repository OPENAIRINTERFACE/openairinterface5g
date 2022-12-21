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

#include "PHY/defs_common.h"
#include "tools_defs.h"
#include <stdio.h>

static const int16_t conjug[8]__attribute__((aligned(16))) = {-1,1,-1,1,-1,1,-1,1} ;
static const int16_t conjug2[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1} ;

#define simd_q15_t simde__m128i
#define simdshort_q15_t simde__m64
#define set1_int16(a) simde_mm_set1_epi16(a)
#define setr_int16(a0, a1, a2, a3, a4, a5, a6, a7) simde_mm_setr_epi16(a0, a1, a2, a3, a4, a5, a6, a7 )

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
  simd_q15_t tmp_re,tmp_im;
  simd_q15_t tmpy0,tmpy1;

  x1_128 = (simd_q15_t *)&x1[0];
  x2_128 = (simd_q15_t *)&x2[0];
  y_128  = (simd_q15_t *)&y[0];


  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>2); i++) {
    tmp_re = simde_mm_madd_epi16(*x1_128,*x2_128);
    tmp_im = simde_mm_shufflelo_epi16(*x1_128, SIMDE_MM_SHUFFLE(2,3,0,1));
    tmp_im = simde_mm_shufflehi_epi16(tmp_im, SIMDE_MM_SHUFFLE(2,3,0,1));
    tmp_im = simde_mm_sign_epi16(tmp_im,*(simde__m128i*)&conjug[0]);
    tmp_im = simde_mm_madd_epi16(tmp_im,*x2_128);
    tmp_re = simde_mm_srai_epi32(tmp_re,output_shift);
    tmp_im = simde_mm_srai_epi32(tmp_im,output_shift);
    tmpy0  = simde_mm_unpacklo_epi32(tmp_re,tmp_im);
    tmpy1  = simde_mm_unpackhi_epi32(tmp_re,tmp_im);
    if (madd==0)
      *y_128 = simde_mm_packs_epi32(tmpy0,tmpy1);
    else
      *y_128 += simde_mm_packs_epi32(tmpy0,tmpy1);
    x1_128++;
    x2_128++;
    y_128++;
  }


  simde_mm_empty();
  simde_m_empty();

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
    tmp_re = simde_mm_sign_epi16(*x1_128,*(simde__m128i*)&conjug2[0]);// Q15
    //print_shorts("tmp_re1:",&tmp_re[i]);
    tmp_re = simde_mm_madd_epi16(tmp_re,*x2_128); //Q28
    //print_ints("tmp_re2:",&tmp_re[i]);
    tmp_im = simde_mm_shufflelo_epi16(*x1_128, SIMDE_MM_SHUFFLE(2,3,0,1)); //Q15
    //print_shorts("tmp_im1:",&tmp_im[i]);
    tmp_im = simde_mm_shufflehi_epi16(tmp_im, SIMDE_MM_SHUFFLE(2,3,0,1)); //Q15
    //print_shorts("tmp_im2:",&tmp_im[i]);
    tmp_im = simde_mm_madd_epi16(tmp_im, *x2_128); //Q28
    //print_ints("tmp_im3:",&tmp_im[i]);
    tmp_re = simde_mm_srai_epi32(tmp_re,output_shift);//Q(28-shift)
    //print_ints("tmp_re shifted:",&tmp_re[i]);
    tmp_im = simde_mm_srai_epi32(tmp_im,output_shift); //Q(28-shift)
    //print_ints("tmp_im shifted:",&tmp_im[i]);
    tmpy0  = simde_mm_unpacklo_epi32(tmp_re,tmp_im); //Q(28-shift)
    //print_ints("unpack lo :",&tmpy0[i]);
    tmpy1  = simde_mm_unpackhi_epi32(tmp_re,tmp_im); //Q(28-shift)
    //print_ints("mrc rho0:",&tmpy1[i]);
    *y_128 = simde_mm_packs_epi32(tmpy0,tmpy1); //must be Q15
    //print_shorts("*y_128:",&y_128[i]);
    x1_128++;
    x2_128++;
    y_128++;
  }
  simde_mm_empty();
  simde_m_empty();
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
  simd_q15_t tmp_re,tmp_im;
  simd_q15_t tmpy0,tmpy1;
  x1_128 = (simd_q15_t *)&x1[0];
  x2_128 = (simd_q15_t *)&x2[0];
  y_128  = (simd_q15_t *)&y[0];
  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>2); i++) {
    tmp_re = simde_mm_sign_epi16(*x1_128,*(simde__m128i*)&conjug2[0]);
    tmp_re = simde_mm_madd_epi16(tmp_re,*x2_128);
    tmp_im = simde_mm_shufflelo_epi16(*x1_128, SIMDE_MM_SHUFFLE(2,3,0,1));
    tmp_im = simde_mm_shufflehi_epi16(tmp_im, SIMDE_MM_SHUFFLE(2,3,0,1));
    tmp_im = simde_mm_madd_epi16(tmp_im,*x2_128);
    tmp_re = simde_mm_srai_epi32(tmp_re,output_shift);
    tmp_im = simde_mm_srai_epi32(tmp_im,output_shift);
    tmpy0  = simde_mm_unpacklo_epi32(tmp_re,tmp_im);
    //print_ints("unpack lo:",&tmpy0[i]);
    tmpy1  = simde_mm_unpackhi_epi32(tmp_re,tmp_im);
    //print_ints("unpack hi:",&tmpy1[i]);
    if (zero_flag == 1)
      *y_128 = simde_mm_packs_epi32(tmpy0,tmpy1);
    else
      *y_128 = simde_mm_adds_epi16(*y_128,simde_mm_packs_epi32(tmpy0,tmpy1));
    //print_shorts("*y_128:",&y_128[i]);
    x1_128++;
    x2_128++;
    y_128++;
  }
  simde_mm_empty();
  simde_m_empty();
  return(0);
}
