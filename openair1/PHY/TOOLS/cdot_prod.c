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

/*! \brief Complex number dot_product
@param x input vector
@param y input vector
@param N size of vectors
@param output_shift normalization of int multiplications
*/

c32_t dot_product(const c16_t *x, const c16_t *y, const uint32_t N, const int output_shift)
{
  const int16_t reflip[32] __attribute__((aligned(32))) = {1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1};
  const int8_t imshuffle[64] __attribute__((aligned(32))) = {2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13, 18, 19, 16, 17, 22, 23, 20, 21, 26, 27, 24, 25, 30, 31, 28, 29};
  const c16_t *end = x + N;
  simde__m256i cumul_re = {0}, cumul_im = {0};
  while (x < end) {
    const simde__m256i in1 = simde_mm256_loadu_si256((simde__m256i *)x);
    const simde__m256i in2 = simde_mm256_loadu_si256((simde__m256i *)y);
    const simde__m256i tmpRe = simde_mm256_madd_epi16(in1, in2);
    cumul_re = simde_mm256_add_epi32(cumul_re, simde_mm256_srai_epi32(tmpRe, output_shift));
    const simde__m256i tmp1 = simde_mm256_shuffle_epi8(in2, *(simde__m256i *)imshuffle);
    const simde__m256i tmp2 = simde_mm256_sign_epi16(tmp1, *(simde__m256i *)reflip);
    const simde__m256i tmpIm = simde_mm256_madd_epi16(in1, tmp2);
    cumul_im = simde_mm256_add_epi32(cumul_im, simde_mm256_srai_epi32(tmpIm, output_shift));
    x += 8;
    y += 8;
  }

  // this gives Re Re Im Im Re Re Im Im
  const simde__m256i cumulTmp = simde_mm256_hadd_epi32(cumul_re, cumul_im);
  const simde__m256i cumul = simde_mm256_hadd_epi32(cumulTmp, cumulTmp);

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
//gcc -DMAIN openair1/PHY/TOOLS/cdot_prod.c -Iopenair1 -I. -Iopenair2/COMMON -march=native -lm
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

void main(void)
{
  const int multiply_reduction=15;
  const int arraySize=16;
  const int signalAmplitude=3000;

  c32_t result={0};
  cd_t resDouble={0};
  c16_t x[arraySize] __attribute__((aligned(16)));
  c16_t y[arraySize] __attribute__((aligned(16)));
  int fd=open("/dev/urandom",0);
  read(fd,x,sizeof(x));
  read(fd,y,sizeof(y));
  close(fd);
  for (int i=0; i<arraySize; i++) {
    x[i].r%=signalAmplitude;
    x[i].i%=signalAmplitude;
    y[i].r%=signalAmplitude;
    y[i].i%=signalAmplitude;
    resDouble.r+=x[i].r*(double)y[i].r+x[i].i*(double)y[i].i;
    resDouble.i+=x[i].r*(double)y[i].i-x[i].i*(double)y[i].r;
  }
  resDouble.r/=pow(2,multiply_reduction);
  resDouble.i/=pow(2,multiply_reduction);

  result = dot_product(x,y,8*2,15);

  printf("result = %d (double: %f), %d (double %f)\n", result.r, resDouble.r,  result.i, resDouble.i);
}
#endif
