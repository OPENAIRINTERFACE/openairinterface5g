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
 
  _mm_empty();
  _m_empty();

}


void multadd_real_vector_complex_scalar(int16_t *x,
                                        int16_t *alpha,
                                        int16_t *y,
                                        uint32_t N)
{

  uint32_t i;

  // do 8 multiplications at a time
  simd_q15_t alpha_r_128, alpha_i_128, yr, yi, *x_128 = (simd_q15_t *)x, *y_128 = (simd_q15_t *)y;

  //  printf("alpha = %d,%d\n",alpha[0],alpha[1]);
  alpha_r_128 = set1_int16(alpha[0]);
  alpha_i_128 = set1_int16(alpha[1]);
  for (i=0; i<N>>3; i++) {

    yr     = mulhi_s1_int16(alpha_r_128,x_128[i]);
    yi     = mulhi_s1_int16(alpha_i_128,x_128[i]);
#if defined(__x86_64__) || defined(__i386__)
    const simd_q15_t tmp = _mm_loadu_si128(y_128);
    _mm_storeu_si128(y_128++, _mm_adds_epi16(tmp, _mm_unpacklo_epi16(yr, yi)));
    const simd_q15_t tmp2 = _mm_loadu_si128(y_128);
    _mm_storeu_si128(y_128++, _mm_adds_epi16(tmp2, _mm_unpackhi_epi16(yr, yi)));
#elif defined(__arm__) || defined (__aarch64__)
    int16x8x2_t yint;
    yint = vzipq_s16(yr,yi);
    *y_128 = adds_int16(*y_128, yint.val[0]);
    j++;
    *y_128 = adds_int16(*y_128, yint.val[1]);

    j++;
#endif
  }
}

void rotate_cpx_vector(c16_t *x,
                       c16_t *alpha,
                       c16_t *y,
                       uint32_t N,
                       uint16_t output_shift)
{
  // multiply a complex vector with a complex value (alpha)
  // stores result in y
  // N is the number of complex numbers
  // output_shift reduces the result of the multiplication by this number of bits
  //AssertFatal(N%8==0, "To be developped");
  if ( (intptr_t)x%32 == 0  && !(intptr_t)y%32 == 0 && __builtin_cpu_supports("avx2")) {
    // output is 32 bytes aligned, but not the input
    
    const c16_t for_re={alpha->r, -alpha->i};
    __m256i const alpha_for_real =  simde_mm256_set1_epi32(*(uint32_t*)&for_re);
    const c16_t for_im={alpha->i, alpha->r};
    __m256i const alpha_for_im= simde_mm256_set1_epi32(*(uint32_t*)&for_im);
    __m256i const perm_mask =
      simde_mm256_set_epi8(31,30,23,22,29,28,21,20,27,26,19,18,25,24,17,16,
			   15,14,7,6,13,12,5,4,11,10,3,2,9,8,1,0);
    __m256i* xd= (__m256i*)x;
    const __m256i *end=xd+N/8;
    for( __m256i* yd = (__m256i *)y; xd<end ; yd++, xd++) {
      const __m256i xre = simde_mm256_srai_epi32(simde_mm256_madd_epi16(*xd,alpha_for_real),
						 output_shift);
      const __m256i xim = simde_mm256_srai_epi32(simde_mm256_madd_epi16(*xd,alpha_for_im),
						 output_shift);
      // a bit faster than unpacklo+unpackhi+packs
      const __m256i tmp=simde_mm256_packs_epi32(xre,xim);
      *yd=simde_mm256_shuffle_epi8(tmp,perm_mask);
    }
    c16_t* alpha16=(c16_t*) alpha, *yLast;
    yLast=((c16_t*)y)+(N/8)*8;
    for (c16_t* xTail=(c16_t*) end;
         xTail<((c16_t*) x)+N;
         xTail++, yLast++) {
      *yLast=c16mulShift(*xTail,*alpha16,output_shift);
    }
  } else {
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

#if defined(__x86_64__) || defined(__i386__)
    __m128i shift = _mm_cvtsi32_si128(output_shift);
    register simd_q15_t m0,m1,m2,m3;

    ((int16_t *)&alpha_128)[0] = alpha->r;
    ((int16_t *)&alpha_128)[1] = -alpha->i;
    ((int16_t *)&alpha_128)[2] = alpha->i;
    ((int16_t *)&alpha_128)[3] = alpha->r;
    ((int16_t *)&alpha_128)[4] = alpha->r;
    ((int16_t *)&alpha_128)[5] = -alpha->i;
    ((int16_t *)&alpha_128)[6] = alpha->i;
    ((int16_t *)&alpha_128)[7] = alpha->r;
#elif defined(__arm__) || defined(__aarch64__)
    int32x4_t shift;
    int32x4_t ab_re0,ab_re1,ab_im0,ab_im1,re32,im32;
    int16_t reflip[8]  __attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1};
    int32x4x2_t xtmp;

    ((int16_t *)&alpha_128)[0] = alpha->r;
    ((int16_t *)&alpha_128)[1] = alpha->i;
    ((int16_t *)&alpha_128)[2] = alpha->r;
    ((int16_t *)&alpha_128)[3] = alpha->i;
    ((int16_t *)&alpha_128)[4] = alpha->r;
    ((int16_t *)&alpha_128)[5] = alpha->i;
    ((int16_t *)&alpha_128)[6] = alpha->r;
    ((int16_t *)&alpha_128)[7] = alpha->i;
    int16x8_t bflip = vrev32q_s16(alpha_128);
    int16x8_t bconj = vmulq_s16(alpha_128,*(int16x8_t *)reflip);
    shift = vdupq_n_s32(-output_shift);
#endif
    y_128 = (simd_q15_t *) y;


    for(i=0; i<N>>2; i++) {
#if defined(__x86_64__) || defined(__i386__)
      m0 = _mm_setr_epi32(xd[0],xd[0],xd[1],xd[1]);
      m1 = _mm_setr_epi32(xd[2],xd[2],xd[3],xd[3]);
      m2 = _mm_madd_epi16(m0,alpha_128); //complex multiply. result is 32bit [Re Im Re Im]
      m3 = _mm_madd_epi16(m1,alpha_128); //complex multiply. result is 32bit [Re Im Re Im]
      m2 = _mm_sra_epi32(m2,shift);        // shift right by shift in order to  compensate for the input amplitude
      m3 = _mm_sra_epi32(m3,shift);        // shift right by shift in order to  compensate for the input amplitude

      y_128[0] = _mm_packs_epi32(m2,m3);        // pack in 16bit integers with saturation [re im re im re im re im]
      //print_ints("y_128[0]=", &y_128[0]);
#elif defined(__arm__) || defined(__aarch64__)

      ab_re0 = vmull_s16(((int16x4_t*)xd)[0],((int16x4_t*)&bconj)[0]);
      ab_re1 = vmull_s16(((int16x4_t*)xd)[1],((int16x4_t*)&bconj)[1]);
      ab_im0 = vmull_s16(((int16x4_t*)xd)[0],((int16x4_t*)&bflip)[0]);
      ab_im1 = vmull_s16(((int16x4_t*)xd)[1],((int16x4_t*)&bflip)[1]);
      re32 = vshlq_s32(vcombine_s32(vpadd_s32(((int32x2_t*)&ab_re0)[0],((int32x2_t*)&ab_re0)[1]),
                                    vpadd_s32(((int32x2_t*)&ab_re1)[0],((int32x2_t*)&ab_re1)[1])),
                       shift);
      im32 = vshlq_s32(vcombine_s32(vpadd_s32(((int32x2_t*)&ab_im0)[0],((int32x2_t*)&ab_im0)[1]),
                                    vpadd_s32(((int32x2_t*)&ab_im1)[0],((int32x2_t*)&ab_im1)[1])),
                       shift);

      xtmp = vzipq_s32(re32,im32);
  
      y_128[0] = vcombine_s16(vmovn_s32(xtmp.val[0]),vmovn_s32(xtmp.val[1]));

#endif
      xd+=4;
      y_128+=1;
    }
  }
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


