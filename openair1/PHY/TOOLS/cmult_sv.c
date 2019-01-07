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

#if defined(__x86_64__) || defined(__i386__)
#define simd_q15_t __m128i
#define simdshort_q15_t __m64
#define shiftright_int16(a,shift) _mm_srai_epi16(a,shift)
#define set1_int16(a) _mm_set1_epi16(a)
#define mulhi_int16(a,b) _mm_slli_epi16(_mm_mulhi_epi16(a,b),1)
#define mulhi_s1_int16(a,b) _mm_slli_epi16(_mm_mulhi_epi16(a,b),2)
#define adds_int16(a,b) _mm_adds_epi16(a,b)
#define mullo_int16(a,b) _mm_mullo_epi16(a,b)
#elif defined(__arm__)
#define simd_q15_t int16x8_t
#define simdshort_q15_t int16x4_t
#define shiftright_int16(a,shift) vshrq_n_s16(a,shift)
#define set1_int16(a) vdupq_n_s16(a)
#define mulhi_int16(a,b) vqdmulhq_s16(a,b)
#define mulhi_s1_int16(a,b) vshlq_n_s16(vqdmulhq_s16(a,b),1)
#define adds_int16(a,b) vqaddq_s16(a,b)
#define mullo_int16(a,b) vmulq_s16(a,b)
#define _mm_empty() 
#define _m_empty()
#endif


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
  simd_q15_t alpha_r_128,alpha_i_128,yr,yi,*x_128=(simd_q15_t*)x,*y_128=(simd_q15_t*)y;
  int j;

  //  printf("alpha = %d,%d\n",alpha[0],alpha[1]);
  alpha_r_128 = set1_int16(alpha[0]);
  alpha_i_128 = set1_int16(alpha[1]);

  j=0;

  for (i=0; i<N>>3; i++) {

    yr     = mulhi_s1_int16(alpha_r_128,x_128[i]);
    yi     = mulhi_s1_int16(alpha_i_128,x_128[i]);
#if defined(__x86_64__) || defined(__i386__)
    y_128[j]   = _mm_adds_epi16(y_128[j],_mm_unpacklo_epi16(yr,yi));
    j++;
    y_128[j]   = _mm_adds_epi16(y_128[j],_mm_unpackhi_epi16(yr,yi));
    j++;
#elif defined(__arm__)
    int16x8x2_t yint;
    yint = vzipq_s16(yr,yi);
    y_128[j]   = adds_int16(y_128[j],yint.val[0]);
    j++;
    y_128[j]   = adds_int16(y_128[j],yint.val[1]);
 
    j++;
#endif
  }

  _mm_empty();
  _m_empty();

}

/*
int rotate_cpx_vector(int16_t *x,
                      int16_t *alpha,
                      int16_t *y,
                      uint32_t N,
                      uint16_t output_shift,
                      uint8_t format)
{
  // Multiply elementwise two complex vectors of N elements
  // x        - input 1    in the format  |Re0  Im0 Re0 Im0|,......,|Re(N-1)  Im(N-1) Re(N-1) Im(N-1)|
  //            We assume x1 with a dynamic of 15 bit maximum
  //
  // alpha      - input 2    in the format  |Re0 Im0|
  //            We assume x2 with a dynamic of 15 bit maximum
  //
  // y        - output     in the format  |Re0  Im0 Re0 Im0|,......,|Re(N-1)  Im(N-1) Re(N-1) Im(N-1)|
  //
  // N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
  //
  // output_shift - shift at output to return in Q1.15
  // format - 0 means alpha is in shuffled format, 1 means x is in shuffled format

  uint32_t i;                 // loop counter

  register __m128i m0,m1;



  __m128i *x_128;
  __m128i *y_128;


  shift = _mm_cvtsi32_si128(output_shift);
  x_128 = (__m128i *)&x[0];

  if (format==0) {  // alpha is in shuffled format for complex multiply
    ((int16_t *)&alpha_128)[0] = alpha[0];
    ((int16_t *)&alpha_128)[1] = -alpha[1];
    ((int16_t *)&alpha_128)[2] = alpha[1];
    ((int16_t *)&alpha_128)[3] = alpha[0];
    ((int16_t *)&alpha_128)[4] = alpha[0];
    ((int16_t *)&alpha_128)[5] = -alpha[1];
    ((int16_t *)&alpha_128)[6] = alpha[1];
    ((int16_t *)&alpha_128)[7] = alpha[0];
  } else { // input is in shuffled format for complex multiply
    ((int16_t *)&alpha_128)[0] = alpha[0];
    ((int16_t *)&alpha_128)[1] = alpha[1];
    ((int16_t *)&alpha_128)[2] = alpha[0];
    ((int16_t *)&alpha_128)[3] = alpha[1];
    ((int16_t *)&alpha_128)[4] = alpha[0];
    ((int16_t *)&alpha_128)[5] = alpha[1];
    ((int16_t *)&alpha_128)[6] = alpha[0];
    ((int16_t *)&alpha_128)[7] = alpha[1];
  }

  y_128 = (__m128i *)&y[0];

  //  _mm_empty();
  //  return(0);

  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>3); i++) {

    m0 = _mm_madd_epi16(x_128[0],alpha_128); //pmaddwd_r2r(mm1,mm0);         // 1- compute x1[0]*x2[0]
    m0 = _mm_sra_epi32(m0,shift);        // 1- shift right by shift in order to  compensate for the input amplitude
    m1=m0;
    m0 = _mm_packs_epi32(m1,m0);        // 1- pack in a 128 bit register [re im re im]
    y_128[0] = _mm_unpacklo_epi32(m0,m0);        // 1- pack in a 128 bit register [re im re im]
    m0 = _mm_madd_epi16(x_128[1],alpha_128); //pmaddwd_r2r(mm1,mm0);         // 1- compute x1[0]*x2[0]
    m0 = _mm_sra_epi32(m0,shift);        // 1- shift right by shift in order to  compensate for the input amplitude
    m1 = m0;
    m1 = _mm_packs_epi32(m1,m0);        // 1- pack in a 128 bit register [re im re im]
    y_128[1] = _mm_unpacklo_epi32(m1,m1);        // 1- pack in a 128 bit register [re im re im]
    m0 = _mm_madd_epi16(x_128[2],alpha_128); //pmaddwd_r2r(mm1,mm0);         // 1- compute x1[0]*x2[0]
    m0 = _mm_sra_epi32(m0,shift);        // 1- shift right by shift in order to  compensate for the input amplitude
    m1 = m0;
    m1 = _mm_packs_epi32(m1,m0);        // 1- pack in a 128 bit register [re im re im]
    y_128[2] = _mm_unpacklo_epi32(m1,m1);        // 1- pack in a 128 bit register [re im re im]
    m0 = _mm_madd_epi16(x_128[3],alpha_128); //pmaddwd_r2r(mm1,mm0);         // 1- compute x1[0]*x2[0]
    m0 = _mm_sra_epi32(m0,shift);        // 1- shift right by shift in order to  compensate for the input amplitude
    m1 = m0;
    m1 = _mm_packs_epi32(m1,m0);        // 1- pack in a 128 bit register [re im re im]
    y_128[3] = _mm_unpacklo_epi32(m1,m1);        // 1- pack in a 128 bit register [re im re im]
    if (format==1) {  // Put output in proper format (Re,-Im,Im,Re), shuffle = (0,1,3,2) = 0x1e

      y_128[0] = _mm_shufflelo_epi16(y_128[0],0x1e);
      y_128[0] = _mm_shufflehi_epi16(y_128[0],0x1e);
      ((int16_t*)&y_128[0])[1] = -((int16_t*)&y_128[0])[1];
      ((int16_t*)&y_128[0])[5] = -((int16_t*)&y_128[0])[5];
      y_128[1] = _mm_shufflelo_epi16(y_128[1],0x1e);
      y_128[1] = _mm_shufflehi_epi16(y_128[1],0x1e);
      ((int16_t*)&y_128[1])[1] = -((int16_t*)&y_128[1])[1];
      ((int16_t*)&y_128[1])[5] = -((int16_t*)&y_128[1])[5];
      y_128[2] = _mm_shufflelo_epi16(y_128[2],0x1e);
      y_128[2] = _mm_shufflehi_epi16(y_128[2],0x1e);
      ((int16_t*)&y_128[2])[1] = -((int16_t*)&y_128[2])[1];
      ((int16_t*)&y_128[2])[5] = -((int16_t*)&y_128[2])[5];
      y_128[3] = _mm_shufflelo_epi16(y_128[3],0x1e);
      y_128[3] = _mm_shufflehi_epi16(y_128[3],0x1e);
      ((int16_t*)&y_128[3])[1] = -((int16_t*)&y_128[3])[1];
      ((int16_t*)&y_128[3])[5] = -((int16_t*)&y_128[3])[5];
    }


    x_128+=4;
    y_128 +=4;
  }


  _mm_empty();
  _m_empty();

  return(0);
}

int rotate_cpx_vector2(int16_t *x,
                       int16_t *alpha,
                       int16_t *y,
                       uint32_t N,
                       uint16_t output_shift,
                       uint8_t format)
{
  // Multiply elementwise two complex vectors of N elements
  // x        - input 1    in the format  |Re0  Im0 Re0 Im0|,......,|Re(N-1)  Im(N-1) Re(N-1) Im(N-1)|
  //            We assume x1 with a dynamic of 15 bit maximum
  //
  // alpha      - input 2    in the format  |Re0 Im0|
  //            We assume x2 with a dynamic of 15 bit maximum
  //
  // y        - output     in the format  |Re0  Im0 Re0 Im0|,......,|Re(N-1)  Im(N-1) Re(N-1) Im(N-1)|
  //
  // N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
  //
  // log2_amp - increase the output amplitude by a factor 2^log2_amp (default is 0)
  //            WARNING: log2_amp>0 can cause overflow!!

  uint32_t i;                 // loop counter

  register __m128i m0,m1;


  __m128i *x_128;
  __m128i *y_128;


  shift = _mm_cvtsi32_si128(output_shift);
  x_128 = (__m128i *)&x[0];

  if (format==0) {  // alpha is in shuffled format for complex multiply
    ((int16_t *)&alpha_128)[0] = alpha[0];
    ((int16_t *)&alpha_128)[1] = -alpha[1];
    ((int16_t *)&alpha_128)[2] = alpha[1];
    ((int16_t *)&alpha_128)[3] = alpha[0];
    ((int16_t *)&alpha_128)[4] = alpha[0];
    ((int16_t *)&alpha_128)[5] = -alpha[1];
    ((int16_t *)&alpha_128)[6] = alpha[1];
    ((int16_t *)&alpha_128)[7] = alpha[0];
  } else { // input is in shuffled format for complex multiply
    ((int16_t *)&alpha_128)[0] = alpha[0];
    ((int16_t *)&alpha_128)[1] = alpha[1];
    ((int16_t *)&alpha_128)[2] = alpha[0];
    ((int16_t *)&alpha_128)[3] = alpha[1];
    ((int16_t *)&alpha_128)[4] = alpha[0];
    ((int16_t *)&alpha_128)[5] = alpha[1];
    ((int16_t *)&alpha_128)[6] = alpha[0];
    ((int16_t *)&alpha_128)[7] = alpha[1];
  }

  y_128 = (__m128i *)&y[0];

  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>1); i++) {


    m0 = _mm_madd_epi16(x_128[i],alpha_128); //pmaddwd_r2r(mm1,mm0);         // 1- compute x1[0]*x2[0]
    m0 = _mm_sra_epi32(m0,shift);        // 1- shift right by shift in order to  compensate for the input amplitude
    m1=m0;
    m1 = _mm_packs_epi32(m1,m0);        // 1- pack in a 128 bit register [re im re im]
    y_128[i] = _mm_unpacklo_epi32(m1,m1);        // 1- pack in a 128 bit register [re im re im]
    if (format==1) {  // Put output in proper format (Re,-Im,Im,Re), shuffle = (0,1,3,2) = 0x1e

      y_128[i] = _mm_shufflelo_epi16(y_128[i],0x1e);
      y_128[i] = _mm_shufflehi_epi16(y_128[i],0x1e);
      ((int16_t*)&y_128[i])[1] = -((int16_t*)&y_128[i])[1];
      ((int16_t*)&y_128[i])[5] = -((int16_t*)&y_128[i])[5];
    }
  }


  _mm_empty();
  _m_empty();


  return(0);
}
*/

int rotate_cpx_vector(int16_t *x,
                      int16_t *alpha,
                      int16_t *y,
                      uint32_t N,
                      uint16_t output_shift)
{
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

  ((int16_t *)&alpha_128)[0] = alpha[0];
  ((int16_t *)&alpha_128)[1] = -alpha[1];
  ((int16_t *)&alpha_128)[2] = alpha[1];
  ((int16_t *)&alpha_128)[3] = alpha[0];
  ((int16_t *)&alpha_128)[4] = alpha[0];
  ((int16_t *)&alpha_128)[5] = -alpha[1];
  ((int16_t *)&alpha_128)[6] = alpha[1];
  ((int16_t *)&alpha_128)[7] = alpha[0];
#elif defined(__arm__)
  int32x4_t shift;
  int32x4_t ab_re0,ab_re1,ab_im0,ab_im1,re32,im32;
  int16_t reflip[8]  __attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1};
  int32x4x2_t xtmp;

  ((int16_t *)&alpha_128)[0] = alpha[0];
  ((int16_t *)&alpha_128)[1] = alpha[1];
  ((int16_t *)&alpha_128)[2] = alpha[0];
  ((int16_t *)&alpha_128)[3] = alpha[1];
  ((int16_t *)&alpha_128)[4] = alpha[0];
  ((int16_t *)&alpha_128)[5] = alpha[1];
  ((int16_t *)&alpha_128)[6] = alpha[0];
  ((int16_t *)&alpha_128)[7] = alpha[1];
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
#elif defined(__arm__)

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


  _mm_empty();
  _m_empty();

  return(0);
}

/*
int mult_vector32_scalar(int16_t *x1,
                         int x2,
                         int16_t *y,
                         uint32_t N)

{
  // Multiply elementwise two real vectors of N elements
  // x1       - input 1    in the format  |Re(0)  xxx  Re(1) xxx|,......,|Re(N-2) xxx Re(N-1) xxx|
  //            We assume x1 with a dinamic of 31 bit maximum
  //
  // x1       - input 2
  //
  // y        - output     in the format  |Re0 (64bit) |,......,|Re(N-1) (64bit)|
  //
  // N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
  //

  uint32_t i;                 // loop counter

  __m128i *x1_128;
  __m128i x2_128;
  __m128i *y_128;


  x1_128 = (__m128i *)&x1[0];
  x2_128 = _mm_setr_epi32(x2,0,x2,0);
  y_128 = (__m128i *)&y[0];


  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>3); i++) {
    y_128[0] = _mm_mul_epu32(x1_128[0],x2_128);
    y_128[1] = _mm_mul_epu32(x1_128[1],x2_128);
    y_128[2] = _mm_mul_epu32(x1_128[2],x2_128);
    y_128[3] = _mm_mul_epu32(x1_128[3],x2_128);


    x1_128+=4;
    y_128 +=4;
  }


  _mm_empty();
  _m_empty();

  return(0);
}
*/

int complex_conjugate(int16_t *x1,
                      int16_t *y,
                      uint32_t N)

{
  uint32_t i;                 // loop counter

  simd_q15_t *x1_128;
  simd_q15_t *y_128;
  int16_t x2[8] __attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1}; 
  simd_q15_t *x2_128 = (simd_q15_t*)&x2[0];
  x1_128 = (simd_q15_t *)&x1[0];
  y_128 = (simd_q15_t *)&y[0];


  // we compute 4 cpx multiply for each loop
  for(i=0; i<(N>>3); i++) {
    y_128[0] = mullo_int16(x1_128[0],*x2_128);
    y_128[1] = mullo_int16(x1_128[1],*x2_128);
    y_128[2] = mullo_int16(x1_128[2],*x2_128);
    y_128[3] = mullo_int16(x1_128[3],*x2_128);


    x1_128+=4;
    y_128 +=4;
  }


  _mm_empty();
  _m_empty();

  return(0);
}


#ifdef MAIN
#define L 8

main ()
{

  int16_t input[256] __attribute__((aligned(16)));
  int16_t input2[256] __attribute__((aligned(16)));
  int16_t output[256] __attribute__((aligned(16)));
  int16_t alpha[2];

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

  alpha[0]=32767;
  alpha[1]=0;

  //mult_cpx_vector(input,input2,output,L,0);
  rotate_cpx_vector_norep(input,alpha,input,L,15);

  for(i=0; i<L<<1; i++)
    printf("output[i]=%d\n",input[i]);

}

#endif //MAIN


