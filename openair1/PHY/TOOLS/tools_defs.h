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

#ifndef __PHY_TOOLS_DEFS__H__
#define __PHY_TOOLS_DEFS__H__

/** @addtogroup _PHY_DSP_TOOLS_


* @{

*/

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "PHY/sse_intrin.h"
#include "common/utils/assertions.h"

#if defined(__x86_64__) || defined(__i386__)
#define simd_q15_t __m128i
#define simdshort_q15_t __m64
#define shiftright_int16(a,shift) _mm_srai_epi16(a,shift)
#define set1_int16(a) _mm_set1_epi16(a)
#define mulhi_int16(a,b) _mm_mulhrs_epi16 (a,b)
#define mulhi_s1_int16(a,b) _mm_slli_epi16(_mm_mulhi_epi16(a,b),2)
#define adds_int16(a,b) _mm_adds_epi16(a,b)
#define mullo_int16(a,b) _mm_mullo_epi16(a,b)
#elif defined(__arm__) || defined(__aarch64__)
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

#ifdef __cplusplus
extern "C" {
#endif

#define CEILIDIV(a,b) ((a+b-1)/b)
#define ROUNDIDIV(a,b) (((a<<1)+b)/(b<<1))

  typedef struct complexd {
    double r;
    double i;
  } cd_t;

  typedef struct complexf {
    float r;
    float i;
  } cf_t;

  typedef struct complex8 {
    int8_t r;
    int8_t i;
  } c8_t;

  typedef struct complex16 {
    int16_t r;
    int16_t i;
  } c16_t;

  typedef struct complex32 {
    int32_t r;
    int32_t i;
  } c32_t;

  typedef struct complex64 {
    int64_t r;
    int64_t i;
  } c64_t;

#define squaredMod(a) ((a).r*(a).r + (a).i*(a).i)
#define csum(res, i1, i2) (res).r = (i1).r + (i2).r ; (res).i = (i1).i + (i2).i

  __attribute__((always_inline)) inline c16_t c16Shift(const c16_t a, const int Shift) {
    return (c16_t) {
        .r = (int16_t)(a.r >> Shift),
        .i = (int16_t)(a.i >> Shift)
    };
  }

  __attribute__((always_inline)) inline c16_t c16addShift(const c16_t a, const c16_t b, const int Shift) {
    return (c16_t) {
        .r = (int16_t)((a.r + b.r) >> Shift),
        .i = (int16_t)((a.i + b.i) >> Shift)
    };
  }

  __attribute__((always_inline)) inline c16_t c16mulShift(const c16_t a, const c16_t b, const int Shift) {
    return (c16_t) {
      .r = (int16_t)((a.r * b.r - a.i * b.i) >> Shift),
      .i = (int16_t)((a.r * b.i + a.i * b.r) >> Shift)
    };
  }

  __attribute__((always_inline)) inline c16_t c16divShift(const c16_t a, const c16_t b, const int Shift) {
    return (c16_t) {
      .r = (int16_t)((a.r * b.r + a.i * b.i) >> Shift),
      .i = (int16_t)((a.r * b.i - a.i * b.r) >> Shift)
    };
  }

  __attribute__((always_inline)) inline c16_t c16maddShift(const c16_t a, const c16_t b, c16_t c, const int Shift) {
    return (c16_t) {
      .r = (int16_t)(((a.r * b.r - a.i * b.i ) >> Shift) + c.r),
      .i = (int16_t)(((a.r * b.i + a.i * b.r ) >> Shift) + c.i)
    };
  }

  __attribute__((always_inline)) inline c32_t c32x16mulShift(const c16_t a, const c16_t b, const int Shift) {
    return (c32_t) {
      .r = (a.r * b.r - a.i * b.i) >> Shift,
      .i = (a.r * b.i + a.i * b.r) >> Shift
    };
  }

  __attribute__((always_inline)) inline c32_t c32x16maddShift(const c16_t a, const c16_t b, const c32_t c, const int Shift) {
    return (c32_t) {
      .r = ((a.r * b.r - a.i * b.i) >> Shift) + c.r,
      .i = ((a.r * b.i + a.i * b.r) >> Shift) + c.i
    };
  }

  __attribute__((always_inline)) inline c16_t c16x32div(const c32_t a, const int div) {
    return (c16_t) {
      .r = (int16_t)(a.r / div),
      .i = (int16_t)(a.i / div)
    };
  }

  __attribute__((always_inline)) inline cd_t cdMul(const cd_t a, const cd_t b)
  {
    return (cd_t) {
        .r = a.r * b.r - a.i * b.i,
        .i = a.r * b.i + a.i * b.r
    };
  }

  // On N complex numbers
  //   y.r += (x * alpha.r) >> 14
  //   y.i += (x * alpha.i) >> 14
  // See regular C implementation at the end
  static __attribute__((always_inline)) inline void c16multaddVectRealComplex(const int16_t *x,
                                                                       const c16_t *alpha,
                                                                       c16_t *y,
                                                                       const int N) {
#if defined(__x86_64__) || defined(__i386__)
    // Default implementation for x86
    const int8_t makePairs[32] __attribute__((aligned(32)))={
      0,1,0+16,1+16,
      2,3,2+16,3+16,
      4,5,4+16,5+16,
      6,7,6+16,7+16,
      8,9,8+16,9+16,
      10,11,10+16,11+16,
      12,13,12+16,13+16,
      14,15,14+16,15+16};
    
    __m256i alpha256= simde_mm256_set1_epi32(*(int32_t *)alpha);
    __m128i *x128=(__m128i *)x;
    __m128i *y128=(__m128i *)y;
    AssertFatal(N%8==0,"Not implemented\n");
    for (int i=0; i<N/8; i++) {
      const __m256i xduplicate=simde_mm256_broadcastsi128_si256(*x128);
      const __m256i x_duplicate_ordered=simde_mm256_shuffle_epi8(xduplicate,*(__m256i*)makePairs);
      const __m256i x_mul_alpha_shift15 =simde_mm256_mulhrs_epi16(alpha256, x_duplicate_ordered);
      // Existing multiplication normalization is weird, constant table in alpha need to be doubled
      const __m256i x_mul_alpha_x2= simde_mm256_adds_epi16(x_mul_alpha_shift15,x_mul_alpha_shift15);
      *y128= _mm_adds_epi16(simde_mm256_extracti128_si256(x_mul_alpha_x2,0),*y128);
      y128++;
      *y128= _mm_adds_epi16(simde_mm256_extracti128_si256(x_mul_alpha_x2,1),*y128);
      y128++;
      x128++;
    } 
    
#elif defined(__arm__) || defined(__aarch64__)
    // Default implementation for ARM
    uint32_t i;

    // do 8 multiplications at a time
    simd_q15_t alpha_r_128,alpha_i_128,yr,yi,*x_128=(simd_q15_t*)x,*y_128=(simd_q15_t*)y;
    int j;

    //  printf("alpha = %d,%d\n",alpha[0],alpha[1]);
    alpha_r_128 = set1_int16(alpha->r);
    alpha_i_128 = set1_int16(alpha->i);

    j=0;

    for (i=0; i<N>>3; i++) {

      yr     = mulhi_s1_int16(alpha_r_128,x_128[i]);
      yi     = mulhi_s1_int16(alpha_i_128,x_128[i]);
      int16x8x2_t yint;
      yint = vzipq_s16(yr,yi);
      y_128[j]   = adds_int16(y_128[j],yint.val[0]);
      j++;
      y_128[j]   = adds_int16(y_128[j],yint.val[1]);

      j++;
    }
#else
    // Almost dead code (BMC)
    for (int i=0; i<N; i++) {
      int tmpr=y[i].r+((x[i]*alpha->r)>>14);
      if (tmpr>INT16_MAX)
        tmpr=INT16_MAX;
      if (tmpr<INT16_MIN)
        tmpr=INT16_MIN;
      int tmpi=y[i].i+((x[i]*alpha->i)>>14);
      if (tmpi>INT16_MAX)
        tmpi=INT16_MAX;
      if (tmpi<INT16_MIN)
        tmpi=INT16_MIN;
      y[i].r=(int16_t)tmpr;
      y[i].i=(int16_t)tmpi;
    }
#endif
  }
//cmult_sv.h

/*!\fn void multadd_real_vector_complex_scalar(int16_t *x,int16_t *alpha,int16_t *y,uint32_t N)
This function performs componentwise multiplication and accumulation of a complex scalar and a real vector.
@param x Vector input (Q1.15)
@param alpha Scalar input (Q1.15) in the format  |Re0 Im0|
@param y Output (Q1.15) in the format  |Re0  Im0 Re1 Im1|,......,|Re(N-1)  Im(N-1) Re(N-1) Im(N-1)|
@param N Length of x WARNING: N>=8

The function implemented is : \f$\mathbf{y} = y + \alpha\mathbf{x}\f$
*/
void multadd_real_vector_complex_scalar(int16_t *x,
                                        int16_t *alpha,
                                        int16_t *y,
                                        uint32_t N);

__attribute__((always_inline)) inline void multadd_real_four_symbols_vector_complex_scalar(int16_t *x,
                                                                                           c16_t *alpha,
                                                                                           c16_t *y)
{

  // do 8 multiplications at a time
  simd_q15_t alpha_r_128,alpha_i_128,yr,yi,*x_128=(simd_q15_t*)x;
  simd_q15_t y_128;
  y_128 = _mm_loadu_si128((simd_q15_t*)y);

  alpha_r_128 = set1_int16(alpha->r);
  alpha_i_128 = set1_int16(alpha->i);


  yr     = mulhi_s1_int16(alpha_r_128,x_128[0]);
  yi     = mulhi_s1_int16(alpha_i_128,x_128[0]);
  y_128   = _mm_adds_epi16(y_128,_mm_unpacklo_epi16(yr,yi));
  y_128   = _mm_adds_epi16(y_128,_mm_unpackhi_epi16(yr,yi));

  _mm_storeu_si128((simd_q15_t*)y, y_128);

}

/*!\fn void multadd_complex_vector_real_scalar(int16_t *x,int16_t alpha,int16_t *y,uint8_t zero_flag,uint32_t N)
This function performs componentwise multiplication and accumulation of a real scalar and a complex vector.
@param x Vector input (Q1.15) in the format |Re0 Im0|Re1 Im 1| ...
@param alpha Scalar input (Q1.15) in the format  |Re0|
@param y Output (Q1.15) in the format  |Re0  Im0 Re1 Im1|,......,|Re(N-1)  Im(N-1) Re(N-1) Im(N-1)|
@param zero_flag Set output (y) to zero prior to accumulation
@param N Length of x WARNING: N>=8

The function implemented is : \f$\mathbf{y} = y + \alpha\mathbf{x}\f$
*/
void multadd_complex_vector_real_scalar(int16_t *x,
                                        int16_t alpha,
                                        int16_t *y,
                                        uint8_t zero_flag,
                                        uint32_t N);

/*!\fn void init_fft(uint16_t size,uint8_t logsize,uint16_t *rev)
\brief Initialize the FFT engine for a given size
@param size Size of the FFT
@param logsize log2(size)
@param rev Pointer to bit-reversal permutation array
*/

//cmult_vv.c
/*!
  Multiply elementwise the complex conjugate of x1 with x2.
  @param x1       - input 1    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
              We assume x1 with a dinamic of 15 bit maximum
  @param x2       - input 2    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
              We assume x2 with a dinamic of 14 bit maximum
  @param y        - output     in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  @param N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
  @param output_shift  - shift to be applied to generate output
  @param madd - if not zero result is added to output
*/

int mult_cpx_conj_vector(int16_t *x1,
                         int16_t *x2,
                         int16_t *y,
                         uint32_t N,
                         int output_shift,
                         int madd);

/*!
  Element-wise multiplication and accumulation of two complex vectors x1 and x2.
  @param x1       - input 1    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
              We assume x1 with a dinamic of 15 bit maximum
  @param x2       - input 2    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
              We assume x2 with a dinamic of 14 bit maximum
  @param y        - output     in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  @param zero_flag Set output (y) to zero prior to accumulation
  @param N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
  @param output_shift  - shift to be applied to generate output
*/

int multadd_cpx_vector(int16_t *x1,
                       int16_t *x2,
                       int16_t *y,
                       uint8_t zero_flag,
                       uint32_t N,
                       int output_shift);

int mult_cpx_vector(int16_t *x1,
                    int16_t  *x2,
                    int16_t *y,
                    uint32_t N,
                    int output_shift);

// lte_dfts.c
void init_fft(uint16_t size,
              uint8_t logsize,
              uint16_t *rev);

/*!\fn void fft(int16_t *x,int16_t *y,int16_t *twiddle,uint16_t *rev,uint8_t log2size,uint8_t scale,uint8_t input_fmt)
This function performs optimized fixed-point radix-2 FFT/IFFT.
@param x Input
@param y Output in format: [Re0,Im0,Re0,Im0, Re1,Im1,Re1,Im1, ....., Re(N-1),Im(N-1),Re(N-1),Im(N-1)]
@param twiddle Twiddle factors
@param rev bit-reversed permutation
@param log2size Base-2 logarithm of FFT size
@param scale Total number of shifts (should be log2size/2 for normalized FFT)
@param input_fmt (0 - input is in complex Q1.15 format, 1 - input is in complex redundant Q1.15 format)
*/
/*void fft(int16_t *x,
         int16_t *y,
         int16_t *twiddle,
         uint16_t *rev,
         uint8_t log2size,
         uint8_t scale,
         uint8_t input_fmt
        );
*/

#define FOREACH_DFTSZ(SZ_DEF) \
  SZ_DEF(12) \
  SZ_DEF(24) \
  SZ_DEF(36) \
  SZ_DEF(48) \
  SZ_DEF(60) \
  SZ_DEF(64) \
  SZ_DEF(72) \
  SZ_DEF(96) \
  SZ_DEF(108) \
  SZ_DEF(120) \
  SZ_DEF(128) \
  SZ_DEF(144) \
  SZ_DEF(180) \
  SZ_DEF(192) \
  SZ_DEF(216) \
  SZ_DEF(240) \
  SZ_DEF(256) \
  SZ_DEF(288) \
  SZ_DEF(300) \
  SZ_DEF(324) \
  SZ_DEF(360) \
  SZ_DEF(384) \
  SZ_DEF(432) \
  SZ_DEF(480) \
  SZ_DEF(512) \
  SZ_DEF(540) \
  SZ_DEF(576) \
  SZ_DEF(600) \
  SZ_DEF(648) \
  SZ_DEF(720) \
  SZ_DEF(768) \
  SZ_DEF(864) \
  SZ_DEF(900) \
  SZ_DEF(960) \
  SZ_DEF(972) \
  SZ_DEF(1024) \
  SZ_DEF(1080) \
  SZ_DEF(1152) \
  SZ_DEF(1200) \
  SZ_DEF(1296) \
  SZ_DEF(1440) \
  SZ_DEF(1500) \
  SZ_DEF(1536) \
  SZ_DEF(1620) \
  SZ_DEF(1728) \
  SZ_DEF(1800) \
  SZ_DEF(1920) \
  SZ_DEF(1944) \
  SZ_DEF(2048) \
  SZ_DEF(2160) \
  SZ_DEF(2304) \
  SZ_DEF(2400) \
  SZ_DEF(2592) \
  SZ_DEF(2700) \
  SZ_DEF(2880) \
  SZ_DEF(2916) \
  SZ_DEF(3000) \
  SZ_DEF(3072) \
  SZ_DEF(3240) \
  SZ_DEF(4096) \
  SZ_DEF(6144) \
  SZ_DEF(8192) \
  SZ_DEF(9216) \
  SZ_DEF(12288) \
  SZ_DEF(18432) \
  SZ_DEF(24576) \
  SZ_DEF(36864) \
  SZ_DEF(49152) \
  SZ_DEF(73728) \
  SZ_DEF(98304)

#define FOREACH_IDFTSZ(SZ_DEF)\
  SZ_DEF(64) \
  SZ_DEF(128) \
  SZ_DEF(256) \
  SZ_DEF(512) \
  SZ_DEF(768) \
  SZ_DEF(1024) \
  SZ_DEF(1536) \
  SZ_DEF(2048) \
  SZ_DEF(3072) \
  SZ_DEF(4096) \
  SZ_DEF(6144) \
  SZ_DEF(8192) \
  SZ_DEF(9216) \
  SZ_DEF(12288) \
  SZ_DEF(16384) \
  SZ_DEF(18432) \
  SZ_DEF(24576) \
  SZ_DEF(32768) \
  SZ_DEF(36864) \
  SZ_DEF(49152) \
  SZ_DEF(65536) \
  SZ_DEF(73728) \
  SZ_DEF(98304)

#ifdef OAIDFTS_MAIN
typedef  void(*adftfunc_t)(int16_t *sigF,int16_t *sig,unsigned char scale_flag);
typedef  void(*aidftfunc_t)(int16_t *sigF,int16_t *sig,unsigned char scale_flag);

#define SZ_FUNC(Sz) void dft ## Sz(int16_t *x,int16_t *y,uint8_t scale_flag);

FOREACH_DFTSZ(SZ_FUNC)

#define SZ_iFUNC(Sz) void idft ## Sz(int16_t *x,int16_t *y,uint8_t scale_flag);

FOREACH_IDFTSZ(SZ_iFUNC)

#else
typedef  void(*dftfunc_t)(uint8_t sizeidx,int16_t *sigF,int16_t *sig,unsigned char scale_flag);
typedef  void(*idftfunc_t)(uint8_t sizeidx,int16_t *sigF,int16_t *sig,unsigned char scale_flag);
#  ifdef OAIDFTS_LOADER
dftfunc_t dft;
idftfunc_t idft;
#  else
extern dftfunc_t dft;
extern idftfunc_t idft;
extern int load_dftslib(void);
#  endif
#endif

#define SZ_ENUM(Sz) DFT_ ## Sz,

typedef enum dft_size_idx {
  FOREACH_DFTSZ(SZ_ENUM)
  DFT_SIZE_IDXTABLESIZE
}  dft_size_idx_t;

#define SZ_iENUM(Sz) IDFT_ ## Sz,

/*******************************************************************
*
* NAME :         get_dft
*
* PARAMETERS :   size of ofdm symbol
*
* RETURN :       function for discrete fourier transform
*
* DESCRIPTION :  get dft function depending of ofdm size
*
*********************************************************************/
static inline
dft_size_idx_t get_dft(int ofdm_symbol_size)
{
  switch (ofdm_symbol_size) {
    case 128:
      return DFT_128;
    case 256:
      return DFT_256;
    case 512:
      return DFT_512;
    case 768:
      return DFT_768;
    case 1024:
      return DFT_1024;
    case 1536:
      return DFT_1536;
    case 2048:
      return DFT_2048;
    case 3072:
      return DFT_3072;
    case 4096:
      return DFT_4096;
    case 6144:
      return DFT_6144;
    case 8192:
      return DFT_8192;
    case 9216:
      return DFT_9216;
    case 12288:
      return DFT_12288;
    case 18432:
      return DFT_18432;
    case 24576:
      return DFT_24576;
    case 36864:
      return DFT_36864;
    case 49152:
      return DFT_49152;
    case 73728:
      return DFT_73728;
    case 98304:
      return DFT_98304;
    default:
      printf("function get_dft : unsupported ofdm symbol size \n");
      assert(0);
      break;
  }
  return DFT_SIZE_IDXTABLESIZE; // never reached and will trigger assertion in idft function;
}

typedef enum idft_size_idx {
  FOREACH_IDFTSZ(SZ_iENUM)
  IDFT_SIZE_IDXTABLESIZE
}  idft_size_idx_t;

#ifdef OAIDFTS_MAIN

#define SZ_PTR(Sz) {dft ## Sz,Sz},
struct {
  adftfunc_t func;
  int size;
} dft_ftab[]= {
  FOREACH_DFTSZ(SZ_PTR)
};

#define SZ_iPTR(Sz)  {idft ## Sz,Sz},
struct {
  adftfunc_t func;
  int size;
} idft_ftab[]= {
  FOREACH_IDFTSZ(SZ_iPTR)
};

#endif

/*******************************************************************
*
* NAME :         get_idft
*
* PARAMETERS :   size of ofdm symbol
*
* RETURN :       index pointing to the dft func in the dft library
*
* DESCRIPTION :  get idft function depending of ofdm size
*
*********************************************************************/
static inline
idft_size_idx_t get_idft(int ofdm_symbol_size)
{
  switch (ofdm_symbol_size) {
    case 128:
      return IDFT_128;
    case 256:
      return IDFT_256;
    case 512:
      return IDFT_512;
    case 768:
      return IDFT_768;
    case 1024:
      return IDFT_1024;
    case 1536:
      return IDFT_1536;
    case 2048:
      return IDFT_2048;
    case 3072:
      return IDFT_3072;
    case 4096:
      return IDFT_4096;
    case 6144:
      return IDFT_6144;
    case 8192:
      return IDFT_8192;
    case 9216:
      return IDFT_9216;
    case 12288:
      return IDFT_12288;
    case 18432:
      return IDFT_18432;
    case 24576:
      return IDFT_24576;
    case 36864:
      return IDFT_36864;
    case 49152:
      return IDFT_49152;
    case 73728:
      return IDFT_73728;
    case 98304:
      return IDFT_98304;
    default:
      printf("function get_idft : unsupported ofdm symbol size \n");
      assert(0);
      break;
  }
  return IDFT_SIZE_IDXTABLESIZE; // never reached and will trigger assertion in idft function
}


/*!\fn int32_t rotate_cpx_vector(c16_t *x,c16_t *alpha,c16_t *y,uint32_t N,uint16_t output_shift)
This function performs componentwise multiplication of a vector with a complex scalar.
@param x Vector input (Q1.15)  in the format  |Re0  Im0|,......,|Re(N-1) Im(N-1)|
@param alpha Scalar input (Q1.15) in the format  |Re0 Im0|
@param y Output (Q1.15) in the format  |Re0  Im0|,......,|Re(N-1) Im(N-1)|
@param N Length of x WARNING: N>=4
@param output_shift Number of bits to shift output down to Q1.15 (should be 15 for Q1.15 inputs) WARNING: log2_amp>0 can cause overflow!!

The function implemented is : \f$\mathbf{y} = \alpha\mathbf{x}\f$
*/
void rotate_cpx_vector(c16_t *x,
                       c16_t *alpha,
                       c16_t *y,
                       uint32_t N,
                       uint16_t output_shift);


//cadd_sv.c

/*!\fn int32_t add_cpx_vector(int16_t *x,int16_t *alpha,int16_t *y,uint32_t N)
This function performs componentwise addition of a vector with a complex scalar.
@param x Vector input (Q1.15)  in the format  |Re0  Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
@param alpha Scalar input (Q1.15) in the format  |Re0 Im0|
@param y Output (Q1.15) in the format  |Re0  Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
@param N Length of x WARNING: N>=4

The function implemented is : \f$\mathbf{y} = \alpha + \mathbf{x}\f$
*/
int32_t add_cpx_vector(int16_t *x,
                       int16_t *alpha,
                       int16_t *y,
                       uint32_t N);

int32_t sub_cpx_vector16(int16_t *x,
                         int16_t *y,
                         int16_t *z,
                         uint32_t N);

/*!\fn int32_t signal_energy(int *,uint32_t);
\brief Computes the signal energy per subcarrier
*/
int32_t signal_energy(int32_t *,uint32_t);

/*!\fn int32_t signal_energy_fixed_p9(int *input, uint32_t length);
\brief Computes the signal energy per subcarrier
\ the input signal has a fixed point representation of AMP_SHIFT bits
\ the ouput energy has a fixed point representation of AMP_SHIFT bits
*/
int32_t signal_energy_amp_shift(int32_t *input, uint32_t length);

#ifdef LOCALIZATION
/*!\fn int32_t signal_energy(int *,uint32_t);
\brief Computes the signal energy per subcarrier
*/
int32_t subcarrier_energy(int32_t *,uint32_t, int32_t *subcarrier_energy, uint16_t rx_power_correction);
#endif

/*!\fn int32_t signal_energy_nodc(int32_t *,uint32_t);
\brief Computes the signal energy per subcarrier, without DC removal
*/
int32_t signal_energy_nodc(int32_t *,uint32_t);

int32_t signal_power(int32_t *,uint32_t);
int32_t interference_power(int32_t *,uint32_t);

/*!\fn double signal_energy_fp(double *s_re[2], double *s_im[2],uint32_t, uint32_t,uint32_t);
\brief Computes the signal energy per subcarrier
*/
double signal_energy_fp(double *s_re[2], double *s_im[2], uint32_t nb_antennas, uint32_t length,uint32_t offset);

/*!\fn double signal_energy_fp2(struct complex *, uint32_t);
\brief Computes the signal energy per subcarrier
*/
double signal_energy_fp2(struct complexd *s, uint32_t length);


int32_t iSqrt(int32_t value);
uint8_t log2_approx(uint32_t);
uint8_t log2_approx64(unsigned long long int x);
int16_t invSqrt(int16_t x);
uint32_t angle(struct complex16 perrror);

/// computes the number of factors 2 in x
unsigned char factor2(unsigned int x);

/*!\fn int32_t phy_phase_compensation_top (uint32_t pilot_type, uint32_t initial_pilot,
        uint32_t last_pilot, int32_t ignore_prefix);
Compensate the phase rotation of the RF. WARNING: This function is currently unused. It has not been tested!
@param pilot_type indicates whether it is a CHBCH (=0) or a SCH (=1) pilot
@param initial_pilot index of the first pilot (which serves as reference)
@param last_pilot index of the last pilot in the range of pilots to correct the phase
@param ignore_prefix set to 1 if cyclic prefix has not been removed (by the hardware)

*/


int8_t dB_fixed(uint32_t x);

uint8_t dB_fixed64(uint64_t x);

int8_t dB_fixed2(uint32_t x,uint32_t y);

int16_t dB_fixed_times10(uint32_t x);
int16_t dB_fixed_x10(uint32_t x);

int32_t phy_phase_compensation_top(uint32_t pilot_type,
                                   uint32_t initial_pilot,
                                   uint32_t last_pilot,
                                   int32_t ignore_prefix);

c32_t dot_product(const c16_t *x,
                  const c16_t *y,
                  const uint32_t N, // must be a multiple of 8
                  const int output_shift);

/** @} */


double interp(double x, double *xs, double *ys, int count);


#ifdef __cplusplus
}
#endif

#endif //__PHY_TOOLS_DEFS__H__
