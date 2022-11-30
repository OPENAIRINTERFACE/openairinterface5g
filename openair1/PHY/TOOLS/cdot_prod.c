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

#include "tools_defs.h"
#include "PHY/sse_intrin.h"

// returns the complex dot product of x and y

#ifdef MAIN
void print_ints(char *s,__m128i *x);
void print_shorts(char *s,__m128i *x);
void print_bytes(char *s,__m128i *x);
#endif
/*! \brief Complex number dot_product
  WARNING: the OAI historical name is dot_product but it is not: it is sum(x*y) not, sum(x*conjugate(y))
*/

c32_t dot_product(const c16_t *x,//! input vector
                  const c16_t *y,//! input vector
                  const uint32_t N,//! size of the vectors
                  const int output_shift //! normalization afer int16 multiplication
                  )
{
  const int16_t reflip[32] __attribute__((aligned(32))) = {1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1};
  const int8_t imshuffle[64] __attribute__((aligned(32))) = {2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13, 18, 19, 16, 17, 22, 23, 20, 21, 26, 27, 24, 25, 30, 31, 28, 29};
  const c16_t *end = x + N;
  __m256i cumul_re = {0}, cumul_im = {0};
  while (x < end) {
    const __m256i in1 = simde_mm256_loadu_si256((__m256i *)x);
    const __m256i in2 = simde_mm256_loadu_si256((__m256i *)y);
    const __m256i tmpRe = simde_mm256_madd_epi16(in1, in2);
    cumul_re = simde_mm256_add_epi32(cumul_re, simde_mm256_srai_epi32(tmpRe, output_shift));
    const __m256i tmp1 = simde_mm256_shuffle_epi8(in2, *(__m256i *)imshuffle);
    const __m256i tmp2 = simde_mm256_sign_epi16(tmp1, *(__m256i *)reflip);
    const __m256i tmpIm = simde_mm256_madd_epi16(in1, tmp2);
    cumul_im = simde_mm256_add_epi32(cumul_im, simde_mm256_srai_epi32(tmpIm, output_shift));
    x += 8;
    y += 8;
  }

  // this gives Re Re Im Im Re Re Im Im
  const __m256i cumulTmp = simde_mm256_hadd_epi32(cumul_re, cumul_im);
  const __m256i cumul = simde_mm256_hadd_epi32(cumulTmp, cumulTmp);

  c32_t ret;
  ret.r = simde_mm256_extract_epi32(cumul, 0) + simde_mm256_extract_epi32(cumul, 4);
  ret.i = simde_mm256_extract_epi32(cumul, 1) + simde_mm256_extract_epi32(cumul, 5);
  if (x!=end) {
    x-=8;
    y-=8;
    for ( ; x <end; x++,y++ ) {
      ret.r += ((x->r*y->r)>>output_shift) + ((x->i*y->i)>>output_shift);
      ret.i += ((x->r*y->i)>>output_shift) - ((x->i*y->r)>>output_shift);
    }
  }
  return ret;
}

#ifdef MAIN
void print_bytes(char *s,__m128i *x)
{

  char *tempb = (char *)x;

  printf("%s  : %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",s,
         tempb[0],tempb[1],tempb[2],tempb[3],tempb[4],tempb[5],tempb[6],tempb[7],
         tempb[8],tempb[9],tempb[10],tempb[11],tempb[12],tempb[13],tempb[14],tempb[15]
        );

}

void print_shorts(char *s,__m128i *x)
{

  int16_t *tempb = (int16_t *)x;

  printf("%s  : %d,%d,%d,%d,%d,%d,%d,%d\n",s,
         tempb[0],tempb[1],tempb[2],tempb[3],tempb[4],tempb[5],tempb[6],tempb[7]
        );

}

void print_ints(char *s,__m128i *x)
{

  int32_t *tempb = (int32_t *)x;

  printf("%s  : %d,%d,%d,%d\n",s,
         tempb[0],tempb[1],tempb[2],tempb[3]
        );

}

void main(void)
{

  int32_t result;

  int16_t x[16*2] __attribute__((aligned(16))) = {1<<0,1<<1,1<<2,1<<3,1<<4,1<<5,1<<6,1<<7,1<<8,1<<9,1<<10,1<<11,1<<12,1<<13,1<<12,1<<13,1<<0,1<<1,1<<2,1<<3,1<<4,1<<5,1<<6,1<<7,1<<8,1<<9,1<<10,1<<11,1<<12,1<<13,1<<12,1<<13};
  int16_t y[16*2] __attribute__((aligned(16))) = {1<<0,1<<1,1<<2,1<<3,1<<4,1<<5,1<<6,1<<7,1<<8,1<<9,1<<10,1<<11,1<<12,1<<13,1<<12,1<<13,1<<0,1<<1,1<<2,1<<3,1<<4,1<<5,1<<6,1<<7,1<<8,1<<9,1<<10,1<<11,1<<12,1<<13,1<<12,1<<13};
  //  int16_t y[16*2] __attribute__((aligned(16))) = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

  result = dot_product(x,y,8*2,15);

  printf("result = %d, %d\n", ((int16_t*) &result)[0],  ((int16_t*) &result)[1] );
}
#endif
