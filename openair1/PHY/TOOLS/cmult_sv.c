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

#include "PHY/sse_intrin.h"
#include "tools_defs.h"
#include <simde/simde-common.h>
#include <simde/x86/sse.h>

void multadd_complex_vector_real_scalar(int16_t *x,
                                        int16_t alpha,
                                        int16_t *y,
                                        uint8_t zero_flag,
                                        uint32_t N)
{

  simd_q15_t alpha_128,*x_128=(simd_q15_t *)x,*y_128=(simd_q15_t*)y;
  int n;

  alpha_128 = set1_int16(alpha);

  if (zero_flag == 1)
    for (n=0; n<N>>2; n++) {
     // print_shorts("x_128[n]=", &x_128[n]);
     // print_shorts("alpha_128", &alpha_128);
      y_128[n] = mulhi_int16(x_128[n],alpha_128);
     // print_shorts("y_128[n]=", &y_128[n]);
    }

  else
    for (n=0; n<N>>2; n++) {
      y_128[n] = adds_int16(y_128[n],mulhi_int16(x_128[n],alpha_128));
    }

  simde_mm_empty();
  simde_m_empty();
}

void multadd_real_vector_complex_scalar(const int16_t *x, const int16_t *alpha, int16_t *y, uint32_t N)
{

  uint32_t i;

  // do 8 multiplications at a time
  simd_q15_t *x_128 = (simd_q15_t *)x, *y_128 = (simd_q15_t *)y;

  //  printf("alpha = %d,%d\n",alpha[0],alpha[1]);
  const simd_q15_t alpha_r_128 = set1_int16(alpha[0]);
  const simd_q15_t alpha_i_128 = set1_int16(alpha[1]);
  for (i=0; i<N>>3; i++) {
    const simd_q15_t yr     = mulhi_s1_int16(alpha_r_128,x_128[i]);
    const simd_q15_t yi     = mulhi_s1_int16(alpha_i_128,x_128[i]);
    const simd_q15_t tmp = simde_mm_loadu_si128(y_128);
    simde_mm_storeu_si128(y_128++,simde_mm_adds_epi16(tmp, simde_mm_unpacklo_epi16(yr, yi)));
    const simd_q15_t tmp2 = simde_mm_loadu_si128(y_128);
    simde_mm_storeu_si128(y_128++, simde_mm_adds_epi16(tmp2, simde_mm_unpackhi_epi16(yr, yi)));
  }
}

void rotate_cpx_vector(const c16_t *const x, const c16_t *const alpha, c16_t *y, uint32_t N, uint16_t output_shift)
{
  // multiply a complex vector with a complex value (alpha)
  // stores result in y
  // N is the number of complex numbers
  // output_shift reduces the result of the multiplication by this number of bits
#if defined(__x86_64__) || defined (__i386__)
  if ( __builtin_cpu_supports("avx2")) {
    // output is 32 bytes aligned, but not the input

    const c16_t for_re = {alpha->r, -alpha->i};
    const simde__m256i alpha_for_real = simde_mm256_set1_epi32(*(uint32_t *)&for_re);
    const c16_t for_im = {alpha->i, alpha->r};
    const simde__m256i alpha_for_im = simde_mm256_set1_epi32(*(uint32_t *)&for_im);
    const simde__m256i perm_mask = simde_mm256_set_epi8(31,
                                                        30,
                                                        23,
                                                        22,
                                                        29,
                                                        28,
                                                        21,
                                                        20,
                                                        27,
                                                        26,
                                                        19,
                                                        18,
                                                        25,
                                                        24,
                                                        17,
                                                        16,
                                                        15,
                                                        14,
                                                        7,
                                                        6,
                                                        13,
                                                        12,
                                                        5,
                                                        4,
                                                        11,
                                                        10,
                                                        3,
                                                        2,
                                                        9,
                                                        8,
                                                        1,
                                                        0);
    simde__m256i *xd = (simde__m256i *)x;
    const simde__m256i *end = xd + N / 8;
    for (simde__m256i *yd = (simde__m256i *)y; xd < end; yd++, xd++) {
      const simde__m256i y256 = simde_mm256_lddqu_si256(xd);
      const simde__m256i xre = simde_mm256_srai_epi32(simde_mm256_madd_epi16(y256, alpha_for_real), output_shift);
      const simde__m256i xim = simde_mm256_srai_epi32(simde_mm256_madd_epi16(y256, alpha_for_im), output_shift);
      // a bit faster than unpacklo+unpackhi+packs
      const simde__m256i tmp = simde_mm256_packs_epi32(xre, xim);
      simde_mm256_storeu_si256(yd, simde_mm256_shuffle_epi8(tmp, perm_mask));
    }
    c16_t* alpha16=(c16_t*) alpha, *yLast;
    yLast=((c16_t*)y)+(N/8)*8;
    for (c16_t* xTail=(c16_t*) end;
         xTail<((c16_t*) x)+N;
         xTail++, yLast++) {
      *yLast=c16mulShift(*xTail,*alpha16,output_shift);
    }
  } else {
#endif
    // Multiply elementwise two complex vectors of N elements
    // x        - input 1    in the format  |Re0  Im0 |,......,|Re(N-1) Im(N-1)|
    //            We assume x1 with a dynamic of 15 bit maximum
    //
    // alpha      - input 2    in the format  |Re0 Im0|
    //            We assume x2 with a dynamic of 15 bit maximum
    //
    // y        - output     in the format  |Re0  Im0|,......,|Re(N-1) Im(N-1)|
    //
    // N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
    //
    // log2_amp - increase the output amplitude by a factor 2^log2_amp (default is 0)
    //            WARNING: log2_amp>0 can cause overflow!!

    uint32_t i;                 // loop counter


    simd_q15_t *y_128,alpha_128;
    int32_t *xd=(int32_t *)x; 

    simde__m128i shift = simde_mm_cvtsi32_si128(output_shift);
    register simd_q15_t m0,m1,m2,m3;

    ((int16_t *)&alpha_128)[0] = alpha->r;
    ((int16_t *)&alpha_128)[1] = -alpha->i;
    ((int16_t *)&alpha_128)[2] = alpha->i;
    ((int16_t *)&alpha_128)[3] = alpha->r;
    ((int16_t *)&alpha_128)[4] = alpha->r;
    ((int16_t *)&alpha_128)[5] = -alpha->i;
    ((int16_t *)&alpha_128)[6] = alpha->i;
    ((int16_t *)&alpha_128)[7] = alpha->r;
    y_128 = (simd_q15_t *) y;


    for(i=0; i<N>>2; i++) {
      m0 = simde_mm_setr_epi32(xd[0],xd[0],xd[1],xd[1]);
      m1 = simde_mm_setr_epi32(xd[2],xd[2],xd[3],xd[3]);
      m2 = simde_mm_madd_epi16(m0,alpha_128); //complex multiply. result is 32bit [Re Im Re Im]
      m3 = simde_mm_madd_epi16(m1,alpha_128); //complex multiply. result is 32bit [Re Im Re Im]
      m2 = simde_mm_sra_epi32(m2,shift);        // shift right by shift in order to  compensate for the input amplitude
      m3 = simde_mm_sra_epi32(m3,shift);        // shift right by shift in order to  compensate for the input amplitude

      y_128[0] = simde_mm_packs_epi32(m2,m3);        // pack in 16bit integers with saturation [re im re im re im re im]
      //print_ints("y_128[0]=", &y_128[0]);
      xd+=4;
      y_128+=1;
    }
#if defined(__x86__) || defined(__x86_64__)
  }
#endif
}

#ifdef MAIN
#define L 8

main ()
{

  int16_t input[256] __attribute__((aligned(16)));
  int16_t input2[256] __attribute__((aligned(16)));
  int16_t output[256] __attribute__((aligned(16)));
  c16_t alpha;

  int i;

  input[0] = 100;
  input[1] = 200;
  input[2] = -200;
  input[3] = 100;
  input[4] = 1000;
  input[5] = 2000;
  input[6] = -2000;
  input[7] = 1000;
  input[8] = 100;
  input[9] = 200;
  input[10] = -200;
  input[11] = 100;
  input[12] = 1000;
  input[13] = 2000;
  input[14] = -2000;
  input[15] = 1000;

  input2[0] = 1;
  input2[1] = 2;
  input2[2] = 1;
  input2[3] = 2;
  input2[4] = 10;
  input2[5] = 20;
  input2[6] = 10;
  input2[7] = 20;
  input2[8] = 1;
  input2[9] = 2;
  input2[10] = 1;
  input2[11] = 2;
  input2[12] = 1000;
  input2[13] = 2000;
  input2[14] = 1000;
  input2[15] = 2000;

  alpha->r=32767;
  alpha->i=0;

  //mult_cpx_vector(input,input2,output,L,0);
  rotate_cpx_vector_norep(input,alpha,input,L,15);

  for(i=0; i<L<<1; i++)
    printf("output[i]=%d\n",input[i]);

}

#endif //MAIN


