/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#if defined(__x86_64__) || defined(__i386__)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stddef.h>
#include <complex.h>
#include <immintrin.h>
#include "../sse_intrin.h"
#include "assertions.h"
#define OAIDFTS_MAIN
#include "tools_defs.h"
#include "LOG/log.h"
#include <pthread.h>

static pthread_mutex_t sr_twiddle_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define Q15_INV_SQRT2 ((int16_t)23170)
#define Q15_INV_SQRT3 ((int16_t)18919)
#define Q15_HALF_SQRT3 ((int16_t)28378)

#define Q15_INV_2SQRT3 ((int16_t)9459) /* 1 / (2 * sqrt(3)) */

#define Q15_HALF ((int16_t)16384) /* 1 / 2 */

#define Q15_ONE ((int16_t)32767)

#define DFT32_Q15_ONE ((int16_t)32767)

#define DFT32_Q15_COS_PI_16 ((int16_t)32138)
#define DFT32_Q15_SIN_PI_16 ((int16_t)6393)

#define DFT32_Q15_COS_3PI_16 ((int16_t)27246)
#define DFT32_Q15_SIN_3PI_16 ((int16_t)18205)

#define Q15_COS_PI_8 ((int16_t)30274) /* cos(pi/8) */
#define Q15_SIN_PI_8 ((int16_t)12540) /* sin(pi/8) */

#define Q15_SQRT3_OVER_2 ((int16_t)28378) /* sqrt(3)/2 */

#define Q15_INV_SQRT5 ((int16_t)14654)

#define Q15_COS_2PI_5 ((int16_t)10126) /* cos(2pi/5)  */
#define Q15_COS_4PI_5 ((int16_t)-26510) /* cos(4pi/5)  */
#define Q15_SIN_2PI_5 ((int16_t)31163) /* sin(2pi/5)  */
#define Q15_SIN_4PI_5 ((int16_t)19260) /* sin(4pi/5)  */

#define SR_MAX_LOG2 25
#define ALIGNMENT 32

#define SPLIT_RADIX_STACK_MAX_C16 16384

#define STACK_MAX_N 1024

//============================================================================
// HELPERS
//============================================================================

static inline __m128i load4_complex_strided_c16(const c16_t *src, int stride, int base)
{
  const c16_t a = src[(base + 0) * stride];
  const c16_t b = src[(base + 1) * stride];
  const c16_t c = src[(base + 2) * stride];
  const c16_t d = src[(base + 3) * stride];

  return _mm_setr_epi16(a.r, a.i, b.r, b.i, c.r, c.i, d.r, d.i);
}

static inline __m128i pack1_complex_lane0_c16(const c16_t a)
{
  return _mm_setr_epi16(a.r, a.i, 0, 0, 0, 0, 0, 0);
}

static inline int16_t sat_i16(long v)
{
  if (v > INT16_MAX)
    return INT16_MAX;

  if (v < INT16_MIN)
    return INT16_MIN;

  return (int16_t)v;
}

static inline int log2_int(unsigned int N)
{
  return __builtin_ctz(N);
}
static void *aligned_malloc64(size_t size)
{
  void *ptr = NULL;

  if (posix_memalign(&ptr, 64, size) != 0) {
    return NULL;
  }

  return ptr;
}

static inline int is_power_of_two_int(int x)
{
  return x > 0 && ((x & (x - 1)) == 0);
}

static inline simde__m256i c16_mul_q15_simd256(simde__m256i x, simde__m256i w_re_negim, simde__m256i w_im_re)
{
  simde__m256i round = simde_mm256_set1_epi32(1 << 14);

  simde__m256i re32 = simde_mm256_madd_epi16(x, w_re_negim);
  simde__m256i im32 = simde_mm256_madd_epi16(x, w_im_re);

  re32 = simde_mm256_srai_epi32(simde_mm256_add_epi32(re32, round), 15);
  im32 = simde_mm256_srai_epi32(simde_mm256_add_epi32(im32, round), 15);

  simde__m256i packed = simde_mm256_packs_epi32(re32, im32);

  const simde__m256i mask = simde_mm256_set_epi8(15,
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
                                                 0,
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

  return simde_mm256_shuffle_epi8(packed, mask);
}

static inline int16_t q15_from_float(float x)
{
  return sat_i16((int32_t)lrintf(32767.0f * x));
}

static inline __m128i swap_complex_pairs_i16_128(__m128i a)
{
  const __m128i shuf = _mm_set_epi8(13, 12, 15, 14, 9, 8, 11, 10, 5, 4, 7, 6, 1, 0, 3, 2);

  return _mm_shuffle_epi8(a, shuf);
}

static inline __m128i complex_mul4_prepack_q15_128(__m128i a, __m128i w_re_re, __m128i w_im_signed)
{
  const __m128i a_swapped = swap_complex_pairs_i16_128(a);

  const __m128i prod_re = _mm_mulhrs_epi16(a, w_re_re);
  const __m128i prod_im = _mm_mulhrs_epi16(a_swapped, w_im_signed);

  return _mm_adds_epi16(prod_re, prod_im);
}

static inline __m128i complex_mul4_bcast_q15_128(__m128i a, int16_t wr, int16_t wi)
{
  const __m128i w_re_re = _mm_set1_epi16(wr);

  const __m128i w_im_signed = _mm_setr_epi16(-wi, wi, -wi, wi, -wi, wi, -wi, wi);

  return complex_mul4_prepack_q15_128(a, w_re_re, w_im_signed);
}

static inline __m128i mul_q15_128(__m128i z)
{
  /*
   * j * (r + ji) = -i + jr
   *
   * [r i] -> [-i r]
   */
  const __m128i swapped = swap_complex_pairs_i16_128(z);

  const __m128i sign = _mm_setr_epi16(-1, 1, -1, 1, -1, 1, -1, 1);

  return _mm_sign_epi16(swapped, sign);
}

static inline __m128i mul_minus_q15_128(__m128i z)
{
  /*
   * -j * (r + ji) = i - jr
   *
   * [r i] -> [i -r]
   */
  const __m128i swapped = swap_complex_pairs_i16_128(z);

  const __m128i sign = _mm_setr_epi16(1, -1, 1, -1, 1, -1, 1, -1);

  return _mm_sign_epi16(swapped, sign);
}

static inline __m128i q15_mul_i16_128(__m128i x, int16_t q15)
{
  return _mm_mulhrs_epi16(x, _mm_set1_epi16(q15));
}

typedef enum { DFT_DIR_FORWARD = -1, DFT_DIR_INVERSE = 1 } dft_dir_t;

static inline __m128i mul_minus_j_dir_i16_128(__m128i z, dft_dir_t dir)
{
  return (dir == DFT_DIR_FORWARD) ? mul_minus_q15_128(z) : mul_q15_128(z);
}

static inline __m128i mul_plus_j_dir_i16_128(__m128i z, dft_dir_t dir)
{
  return (dir == DFT_DIR_FORWARD) ? mul_q15_128(z) : mul_minus_q15_128(z);
}

static inline __m128i twiddle_im_dir_128(__m128i w_im_signed, dft_dir_t dir)
{
  return (dir == DFT_DIR_FORWARD) ? w_im_signed : _mm_sub_epi16(_mm_setzero_si128(), w_im_signed);
}

static inline int16_t twiddle_im_scalar_dir_i16(int16_t wi_forward, dft_dir_t dir)
{
  return (dir == DFT_DIR_FORWARD) ? wi_forward : sat_i16(-(long)wi_forward);
}

static void *aligned_malloc(size_t size)
{
  void *ptr = NULL;

  if (posix_memalign(&ptr, ALIGNMENT, size) != 0)
    return NULL;

  return ptr;
}

static inline __m256i swap_complex_pairs_i16_256(__m256i a)
{
  const __m256i shuf = _mm256_setr_epi8(2,
                                        3,
                                        0,
                                        1,
                                        6,
                                        7,
                                        4,
                                        5,
                                        10,
                                        11,
                                        8,
                                        9,
                                        14,
                                        15,
                                        12,
                                        13,

                                        2,
                                        3,
                                        0,
                                        1,
                                        6,
                                        7,
                                        4,
                                        5,
                                        10,
                                        11,
                                        8,
                                        9,
                                        14,
                                        15,
                                        12,
                                        13);

  return _mm256_shuffle_epi8(a, shuf);
}

static inline __m256i mul_j_i16_256(__m256i z)
{
  const __m256i swapped = swap_complex_pairs_i16_256(z);

  const __m256i sign = _mm256_setr_epi16(-1, +1, -1, +1, -1, +1, -1, +1, -1, +1, -1, +1, -1, +1, -1, +1);

  return _mm256_sign_epi16(swapped, sign);
}

static inline __m256i mul_minus_j_i16_256(__m256i z)
{
  const __m256i swapped = swap_complex_pairs_i16_256(z);

  const __m256i sign = _mm256_setr_epi16(+1, -1, +1, -1, +1, -1, +1, -1, +1, -1, +1, -1, +1, -1, +1, -1);

  return _mm256_sign_epi16(swapped, sign);
}

static inline __m256i mul_minus_j_dir_i16_256(__m256i z, dft_dir_t dir)
{
  return (dir == DFT_DIR_FORWARD) ? mul_minus_j_i16_256(z) : mul_j_i16_256(z);
}

static inline __m256i mul_plus_j_dir_i16_256(__m256i z, dft_dir_t dir)
{
  return (dir == DFT_DIR_FORWARD) ? mul_j_i16_256(z) : mul_minus_j_i16_256(z);
}

static inline void
transpose8_complex_i16_256(__m256i *r0, __m256i *r1, __m256i *r2, __m256i *r3, __m256i *r4, __m256i *r5, __m256i *r6, __m256i *r7)
{
  const __m256i a = *r0;
  const __m256i b = *r1;
  const __m256i c = *r2;
  const __m256i d = *r3;
  const __m256i e = *r4;
  const __m256i f = *r5;
  const __m256i g = *r6;
  const __m256i h = *r7;

  const __m256i t0 = _mm256_unpacklo_epi32(a, b);
  const __m256i t1 = _mm256_unpackhi_epi32(a, b);
  const __m256i t2 = _mm256_unpacklo_epi32(c, d);
  const __m256i t3 = _mm256_unpackhi_epi32(c, d);
  const __m256i t4 = _mm256_unpacklo_epi32(e, f);
  const __m256i t5 = _mm256_unpackhi_epi32(e, f);
  const __m256i t6 = _mm256_unpacklo_epi32(g, h);
  const __m256i t7 = _mm256_unpackhi_epi32(g, h);

  const __m256i s0 = _mm256_unpacklo_epi64(t0, t2);
  const __m256i s1 = _mm256_unpackhi_epi64(t0, t2);
  const __m256i s2 = _mm256_unpacklo_epi64(t1, t3);
  const __m256i s3 = _mm256_unpackhi_epi64(t1, t3);

  const __m256i s4 = _mm256_unpacklo_epi64(t4, t6);
  const __m256i s5 = _mm256_unpackhi_epi64(t4, t6);
  const __m256i s6 = _mm256_unpacklo_epi64(t5, t7);
  const __m256i s7 = _mm256_unpackhi_epi64(t5, t7);

  *r0 = _mm256_permute2x128_si256(s0, s4, 0x20);
  *r1 = _mm256_permute2x128_si256(s1, s5, 0x20);
  *r2 = _mm256_permute2x128_si256(s2, s6, 0x20);
  *r3 = _mm256_permute2x128_si256(s3, s7, 0x20);

  *r4 = _mm256_permute2x128_si256(s0, s4, 0x31);
  *r5 = _mm256_permute2x128_si256(s1, s5, 0x31);
  *r6 = _mm256_permute2x128_si256(s2, s6, 0x31);
  *r7 = _mm256_permute2x128_si256(s3, s7, 0x31);
}

static inline __m256i complex_mul8_prepack_q15_256(__m256i a, __m256i w_re_re, __m256i w_im_signed)
{
  const __m256i a_swapped = swap_complex_pairs_i16_256(a);

  const __m256i prod_re = _mm256_mulhrs_epi16(a, w_re_re);
  const __m256i prod_im = _mm256_mulhrs_epi16(a_swapped, w_im_signed);

  return _mm256_adds_epi16(prod_re, prod_im);
}

static void dft_mixed_radix_c16_scaled_strided(const c16_t *src, int stride, c16_t *dst, int N, dft_dir_t dir);
static void dft_mixed_radix_c16_scaled(const c16_t *src, c16_t *dst, int N, dft_dir_t dir);
//=====================================================================================
// TWIDDLES START
//=====================================================================================

typedef struct {
  int N;
  int initialized;

  float complex *forward;
  float complex *inverse;

  int r3_q15_blocks;
  __m128i *r3_q15_w1_re;
  __m128i *r3_q15_w1_im;
  __m128i *r3_q15_w2_re;
  __m128i *r3_q15_w2_im;
  __m128i *r3_q15_w1_re_inv;
  __m128i *r3_q15_w1_im_inv;
  __m128i *r3_q15_w2_re_inv;
  __m128i *r3_q15_w2_im_inv;

  int r5_q15_blocks;
  __m128i *r5_q15_w1_re;
  __m128i *r5_q15_w1_im;
  __m128i *r5_q15_w2_re;
  __m128i *r5_q15_w2_im;
  __m128i *r5_q15_w3_re;
  __m128i *r5_q15_w3_im;
  __m128i *r5_q15_w4_re;
  __m128i *r5_q15_w4_im;
  __m128i *r5_q15_w1_re_inv;
  __m128i *r5_q15_w1_im_inv;
  __m128i *r5_q15_w2_re_inv;
  __m128i *r5_q15_w2_im_inv;
  __m128i *r5_q15_w3_re_inv;
  __m128i *r5_q15_w3_im_inv;
  __m128i *r5_q15_w4_re_inv;
  __m128i *r5_q15_w4_im_inv;

  __m256i C64_RE_RE_q15_256[8] __attribute__((aligned(64)));
  __m256i C64_IM_SIGNED_q15_256[8] __attribute__((aligned(64)));

  __m256i W128_RE_RE_q15_256[8] __attribute__((aligned(64)));
  __m256i W128_IM_SIGNED_q15_256[8] __attribute__((aligned(64)));

  __m256i C64_RE_RE_q15_256_inverse[8] __attribute__((aligned(64)));
  __m256i C64_IM_SIGNED_q15_256_inverse[8] __attribute__((aligned(64)));

  __m256i W128_RE_RE_q15_256_inverse[8] __attribute__((aligned(64)));
  __m256i W128_IM_SIGNED_q15_256_inverse[8] __attribute__((aligned(64)));

} TwiddleTable;

static TwiddleTable g_tables[DFT_SIZE_IDXTABLESIZE];

static inline __m128i pack4_twiddle_q15_re_re_scaled(const float complex *W, int k0, int mul, int N, float scale)
{
  int16_t v[8];

  for (int j = 0; j < 4; j++) {
    const int k = (mul * (k0 + j)) % N;
    const int16_t wr = q15_from_float(crealf(W[k]) * scale);

    v[2 * j + 0] = wr;
    v[2 * j + 1] = wr;
  }

  return _mm_loadu_si128((const __m128i *)v);
}

static inline __m128i pack4_twiddle_q15_im_signed_scaled(const float complex *W, int k0, int mul, int N, float scale)
{
  int16_t v[8];

  for (int j = 0; j < 4; j++) {
    const int k = (mul * (k0 + j)) % N;
    const int16_t wi = q15_from_float(cimagf(W[k]) * scale);

    v[2 * j + 0] = -wi;
    v[2 * j + 1] = wi;
  }

  return _mm_loadu_si128((const __m128i *)v);
}

static inline __m256i pack8_twiddle_q15_re_re256i(const float complex *W, int k0, int mul, int N)
{
  int16_t v[16] __attribute__((aligned(32)));

  for (int j = 0; j < 8; j++) {
    const int k = (mul * (k0 + j)) % N;
    const int16_t wr = (N == 64)                  ? q15_from_float(crealf(W[k])) / 8
                       : (N % 4 == 0 && N != 128) ? q15_from_float(crealf(W[k])) / 2
                                                  : q15_from_float(crealf(W[k])) / sqrtf(2.0f);

    v[2 * j + 0] = wr;
    v[2 * j + 1] = wr;
  }

  return _mm256_load_si256((const __m256i *)v);
}

static inline __m256i pack8_twiddle_q15_im_signed256i(const float complex *W, int k0, int mul, int N)
{
  int16_t v[16] __attribute__((aligned(32)));

  for (int j = 0; j < 8; j++) {
    const int k = (mul * (k0 + j)) % N;
    const int16_t wi = (N == 64)                  ? q15_from_float(cimagf(W[k])) / 8
                       : (N % 4 == 0 && N != 128) ? q15_from_float(cimagf(W[k])) / 2
                                                  : q15_from_float(cimagf(W[k])) / sqrtf(2.0f);

    v[2 * j + 0] = -wi;
    v[2 * j + 1] = wi;
  }

  return _mm256_load_si256((const __m256i *)v);
}

static int twiddle_table_64_128_create_q15_simd(TwiddleTable *table)
{
  const int N = table->N;

  for (int m = 0; m < 8; m++) {
    if (N == 64) {
      table->C64_RE_RE_q15_256[m] = pack8_twiddle_q15_re_re256i(table->forward, 0, m, N);

      table->C64_IM_SIGNED_q15_256[m] = pack8_twiddle_q15_im_signed256i(table->forward, 0, m, N);

      table->C64_RE_RE_q15_256_inverse[m] = pack8_twiddle_q15_re_re256i(table->inverse, 0, m, N);

      table->C64_IM_SIGNED_q15_256_inverse[m] = pack8_twiddle_q15_im_signed256i(table->inverse, 0, m, N);
    } else {
      table->W128_RE_RE_q15_256[m] = pack8_twiddle_q15_re_re256i(table->forward, 8 * m, 1, N);

      table->W128_IM_SIGNED_q15_256[m] = pack8_twiddle_q15_im_signed256i(table->forward, 8 * m, 1, N);

      table->W128_RE_RE_q15_256_inverse[m] = pack8_twiddle_q15_re_re256i(table->inverse, 8 * m, 1, N);

      table->W128_IM_SIGNED_q15_256_inverse[m] = pack8_twiddle_q15_im_signed256i(table->inverse, 8 * m, 1, N);
    }
  }
  return 1;
}
static int twiddle_table_create_radix3_q15_simd(TwiddleTable *table)
{
  const int N = table->N;

  if (N <= 0) {
    return 0;
  }

  if (N % 3 != 0) {
    return 1;
  }

  const int size = N / 3;
  const int blocks = (size + 3) / 4;

  table->r3_q15_blocks = blocks;

  table->r3_q15_w1_re = aligned_malloc(sizeof(__m128i) * blocks);
  table->r3_q15_w1_im = aligned_malloc(sizeof(__m128i) * blocks);
  table->r3_q15_w2_re = aligned_malloc(sizeof(__m128i) * blocks);
  table->r3_q15_w2_im = aligned_malloc(sizeof(__m128i) * blocks);

  table->r3_q15_w1_re_inv = aligned_malloc(sizeof(__m128i) * blocks);
  table->r3_q15_w1_im_inv = aligned_malloc(sizeof(__m128i) * blocks);
  table->r3_q15_w2_re_inv = aligned_malloc(sizeof(__m128i) * blocks);
  table->r3_q15_w2_im_inv = aligned_malloc(sizeof(__m128i) * blocks);

  if (!table->r3_q15_w1_re || !table->r3_q15_w1_im || !table->r3_q15_w2_re || !table->r3_q15_w2_im || !table->r3_q15_w1_re_inv
      || !table->r3_q15_w1_im_inv || !table->r3_q15_w2_re_inv || !table->r3_q15_w2_im_inv) {
    return 0;
  }

  for (int b = 0; b < blocks; b++) {
    const int k0 = 4 * b;
    const float scale3 = 1.0f / sqrtf(3.0f);

    table->r3_q15_w1_re[b] = pack4_twiddle_q15_re_re_scaled(table->forward, k0, 1, N, scale3);
    table->r3_q15_w1_im[b] = pack4_twiddle_q15_im_signed_scaled(table->forward, k0, 1, N, scale3);

    table->r3_q15_w2_re[b] = pack4_twiddle_q15_re_re_scaled(table->forward, k0, 2, N, scale3);
    table->r3_q15_w2_im[b] = pack4_twiddle_q15_im_signed_scaled(table->forward, k0, 2, N, scale3);

    table->r3_q15_w1_re_inv[b] = pack4_twiddle_q15_re_re_scaled(table->inverse, k0, 1, N, scale3);
    table->r3_q15_w1_im_inv[b] = pack4_twiddle_q15_im_signed_scaled(table->inverse, k0, 1, N, scale3);

    table->r3_q15_w2_re_inv[b] = pack4_twiddle_q15_re_re_scaled(table->inverse, k0, 2, N, scale3);
    table->r3_q15_w2_im_inv[b] = pack4_twiddle_q15_im_signed_scaled(table->inverse, k0, 2, N, scale3);
  }

  return 1;
}

static int twiddle_table_create_radix5_q15_simd(TwiddleTable *table)
{
  const int N = table->N;

  if (N <= 0)
    return 0;

  if (N % 5 != 0)
    return 1;

  const int size = N / 5;
  const int blocks = (size + 3) / 4;

  table->r5_q15_blocks = blocks;

  table->r5_q15_w1_re = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w1_im = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w2_re = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w2_im = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w3_re = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w3_im = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w4_re = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w4_im = aligned_malloc64(blocks * sizeof(__m128i));

  table->r5_q15_w1_re_inv = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w1_im_inv = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w2_re_inv = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w2_im_inv = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w3_re_inv = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w3_im_inv = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w4_re_inv = aligned_malloc64(blocks * sizeof(__m128i));
  table->r5_q15_w4_im_inv = aligned_malloc64(blocks * sizeof(__m128i));

  if (!table->r5_q15_w1_re || !table->r5_q15_w1_im || !table->r5_q15_w2_re || !table->r5_q15_w2_im || !table->r5_q15_w3_re
      || !table->r5_q15_w3_im || !table->r5_q15_w4_re || !table->r5_q15_w4_im || !table->r5_q15_w1_re_inv
      || !table->r5_q15_w1_im_inv || !table->r5_q15_w2_re_inv || !table->r5_q15_w2_im_inv || !table->r5_q15_w3_re_inv
      || !table->r5_q15_w3_im_inv || !table->r5_q15_w4_re_inv || !table->r5_q15_w4_im_inv) {
    return 0;
  }

  for (int b = 0; b < blocks; b++) {
    const int k0 = 4 * b;
    const float scale5 = 1.0f / sqrtf(5.0f);

    table->r5_q15_w1_re[b] = pack4_twiddle_q15_re_re_scaled(table->forward, k0, 1, N, scale5);
    table->r5_q15_w1_im[b] = pack4_twiddle_q15_im_signed_scaled(table->forward, k0, 1, N, scale5);

    table->r5_q15_w2_re[b] = pack4_twiddle_q15_re_re_scaled(table->forward, k0, 2, N, scale5);
    table->r5_q15_w2_im[b] = pack4_twiddle_q15_im_signed_scaled(table->forward, k0, 2, N, scale5);

    table->r5_q15_w3_re[b] = pack4_twiddle_q15_re_re_scaled(table->forward, k0, 3, N, scale5);
    table->r5_q15_w3_im[b] = pack4_twiddle_q15_im_signed_scaled(table->forward, k0, 3, N, scale5);

    table->r5_q15_w4_re[b] = pack4_twiddle_q15_re_re_scaled(table->forward, k0, 4, N, scale5);
    table->r5_q15_w4_im[b] = pack4_twiddle_q15_im_signed_scaled(table->forward, k0, 4, N, scale5);

    table->r5_q15_w1_re_inv[b] = pack4_twiddle_q15_re_re_scaled(table->inverse, k0, 1, N, scale5);
    table->r5_q15_w1_im_inv[b] = pack4_twiddle_q15_im_signed_scaled(table->inverse, k0, 1, N, scale5);

    table->r5_q15_w2_re_inv[b] = pack4_twiddle_q15_re_re_scaled(table->inverse, k0, 2, N, scale5);
    table->r5_q15_w2_im_inv[b] = pack4_twiddle_q15_im_signed_scaled(table->inverse, k0, 2, N, scale5);

    table->r5_q15_w3_re_inv[b] = pack4_twiddle_q15_re_re_scaled(table->inverse, k0, 3, N, scale5);
    table->r5_q15_w3_im_inv[b] = pack4_twiddle_q15_im_signed_scaled(table->inverse, k0, 3, N, scale5);

    table->r5_q15_w4_re_inv[b] = pack4_twiddle_q15_re_re_scaled(table->inverse, k0, 4, N, scale5);
    table->r5_q15_w4_im_inv[b] = pack4_twiddle_q15_im_signed_scaled(table->inverse, k0, 4, N, scale5);
  }

  return 1;
}

static void twiddle_table_release_float_tables(TwiddleTable *table)
{
  free(table->forward);
  free(table->inverse);
  table->forward = NULL;
  table->inverse = NULL;
}

static void twiddle_table_release_dynamic_tables(TwiddleTable *table)
{
  twiddle_table_release_float_tables(table);

  free(table->r3_q15_w1_re);
  free(table->r3_q15_w1_im);
  free(table->r3_q15_w2_re);
  free(table->r3_q15_w2_im);
  free(table->r3_q15_w1_re_inv);
  free(table->r3_q15_w1_im_inv);
  free(table->r3_q15_w2_re_inv);
  free(table->r3_q15_w2_im_inv);

  free(table->r5_q15_w1_re);
  free(table->r5_q15_w1_im);
  free(table->r5_q15_w2_re);
  free(table->r5_q15_w2_im);
  free(table->r5_q15_w3_re);
  free(table->r5_q15_w3_im);
  free(table->r5_q15_w4_re);
  free(table->r5_q15_w4_im);
  free(table->r5_q15_w1_re_inv);
  free(table->r5_q15_w1_im_inv);
  free(table->r5_q15_w2_re_inv);
  free(table->r5_q15_w2_im_inv);
  free(table->r5_q15_w3_re_inv);
  free(table->r5_q15_w3_im_inv);
  free(table->r5_q15_w4_re_inv);
  free(table->r5_q15_w4_im_inv);

  memset(table, 0, sizeof(*table));
}

static TwiddleTable *twiddle_table_create(dft_size_idx_t sizeidx, int N)
{
  TwiddleTable *table = &g_tables[sizeidx];

  memset(table, 0, sizeof(*table));
  table->N = N;

  table->forward = aligned_malloc((size_t)N * sizeof(*table->forward));
  table->inverse = aligned_malloc((size_t)N * sizeof(*table->inverse));

  if (!table->forward || !table->inverse) {
    fprintf(stderr, "twiddle_table_create: allocation failed for N=%d\n", N);
    twiddle_table_release_dynamic_tables(table);
    return NULL;
  }

  for (int k = 0; k < N; k++) {
    const float theta = 2.0f * (float)M_PI * (float)k / (float)N;
    const float c = cosf(theta);
    const float s = sinf(theta);

    table->forward[k] = c - I * s;
    table->inverse[k] = c + I * s;
  }

  if ((N == 64 || N == 128) && !twiddle_table_64_128_create_q15_simd(table)) {
    twiddle_table_release_dynamic_tables(table);
    return NULL;
  }

  if (N % 5 == 0) {
    if (!twiddle_table_create_radix5_q15_simd(table)) {
      twiddle_table_release_dynamic_tables(table);
      return NULL;
    }
  } else if (N % 3 == 0) {
    if (!twiddle_table_create_radix3_q15_simd(table)) {
      twiddle_table_release_dynamic_tables(table);
      return NULL;
    }
  }

  twiddle_table_release_float_tables(table);
  table->initialized = 1;
  return table;
}
static pthread_mutex_t twiddle_table_mutex =
    PTHREAD_MUTEX_INITIALIZER;

const TwiddleTable *twiddle_table_get(int N)
{
  const dft_size_idx_t sizeidx = get_dft(N);

  if (sizeidx == DFT_SIZE_IDXTABLESIZE) {
    fprintf(stderr, "twiddle_table_get: unsupported DFT size N=%d\n", N);
    abort();
  }

  TwiddleTable *table = &g_tables[sizeidx];

  pthread_mutex_lock(&twiddle_table_mutex);

  if (!table->initialized) {
    if (!twiddle_table_create(sizeidx, N)) {
      pthread_mutex_unlock(&twiddle_table_mutex);
      return NULL;
    }
  }

  pthread_mutex_unlock(&twiddle_table_mutex);

  return table;
}

typedef struct {
  int N;
  int blocks;
  int initialized;
  simde__m256i *W1_RE_NEGIM;
  simde__m256i *W1_IM_RE;
  simde__m256i *W3_RE_NEGIM;
  simde__m256i *W3_IM_RE;
} sr_twiddle_simd_t;

static sr_twiddle_simd_t sr_twiddles_fwd[SR_MAX_LOG2 + 1];
static sr_twiddle_simd_t sr_twiddles_bwd[SR_MAX_LOG2 + 1];

static int init_sr_twiddle_simd(sr_twiddle_simd_t *tw, int N, dft_dir_t dir)
{
  int quarter = N / 4;
  int blocks = quarter / 8;

  tw->N = N;
  tw->blocks = blocks;

  tw->W1_RE_NEGIM = aligned_alloc(32, sizeof(simde__m256i) * blocks);
  tw->W1_IM_RE = aligned_alloc(32, sizeof(simde__m256i) * blocks);
  tw->W3_RE_NEGIM = aligned_alloc(32, sizeof(simde__m256i) * blocks);
  tw->W3_IM_RE = aligned_alloc(32, sizeof(simde__m256i) * blocks);

  if (!tw->W1_RE_NEGIM || !tw->W1_IM_RE || !tw->W3_RE_NEGIM || !tw->W3_IM_RE) {
    free(tw->W1_RE_NEGIM);
    free(tw->W1_IM_RE);
    free(tw->W3_RE_NEGIM);
    free(tw->W3_IM_RE);
    tw->W1_RE_NEGIM = NULL;
    tw->W1_IM_RE = NULL;
    tw->W3_RE_NEGIM = NULL;
    tw->W3_IM_RE = NULL;
    return 0;
  }

  for (int b = 0; b < blocks; b++) {
    int16_t w1_re_negim[16] __attribute__((aligned(64)));
    int16_t w1_im_re[16] __attribute__((aligned(64)));
    int16_t w3_re_negim[16] __attribute__((aligned(64)));
    int16_t w3_im_re[16] __attribute__((aligned(64)));

    for (int j = 0; j < 8; j++) {
      int k = 8 * b + j;

      float theta1 = (float)dir * 2.0f * (float)M_PI * k / (float)N;
      float theta3 = (float)dir * 6.0f * (float)M_PI * k / (float)N;

      int16_t w1r = sat_i16(lrintf(32767.0f * cosf(theta1))) / 2;
      int16_t w1i = sat_i16(lrintf(32767.0f * sinf(theta1))) / 2;

      int16_t w3r = sat_i16(lrintf(32767.0f * cosf(theta3))) / 2;
      int16_t w3i = sat_i16(lrintf(32767.0f * sinf(theta3))) / 2;

      w1_re_negim[2 * j] = w1r;
      w1_re_negim[2 * j + 1] = sat_i16(-(long)w1i);
      w1_im_re[2 * j] = w1i;
      w1_im_re[2 * j + 1] = w1r;

      w3_re_negim[2 * j] = w3r;
      w3_re_negim[2 * j + 1] = sat_i16(-(long)w3i);
      w3_im_re[2 * j] = w3i;
      w3_im_re[2 * j + 1] = w3r;
    }

    tw->W1_RE_NEGIM[b] = simde_mm256_load_si256((simde__m256i *)w1_re_negim);
    tw->W1_IM_RE[b] = simde_mm256_load_si256((simde__m256i *)w1_im_re);
    tw->W3_RE_NEGIM[b] = simde_mm256_load_si256((simde__m256i *)w3_re_negim);
    tw->W3_IM_RE[b] = simde_mm256_load_si256((simde__m256i *)w3_im_re);
  }
  return 1;
}

static sr_twiddle_simd_t *sr_twiddle_table_create(int N, dft_dir_t dir)
{
  const int idx = log2_int((unsigned int)N);

  sr_twiddle_simd_t *tw = (dir == DFT_DIR_FORWARD) ? &sr_twiddles_fwd[idx] : &sr_twiddles_bwd[idx];

  memset(tw, 0, sizeof(*tw));

  if (!init_sr_twiddle_simd(tw, N, dir)) {
    return NULL;
  }

  tw->initialized = 1;
  return tw;
}

const sr_twiddle_simd_t *sr_twiddle_table_get(int N, dft_dir_t dir)
{
  if (N <= 0 || !is_power_of_two_int(N)) {
    fprintf(stderr, "sr_twiddle_table_get: invalid N=%d; expected a power of two\n", N);
    return NULL;
  }

  const int idx = log2_int((unsigned int)N);

  if (idx > SR_MAX_LOG2) {
    fprintf(stderr,
            "sr_twiddle_table_get: log2(N)=%d exceeds "
            "SR_MAX_LOG2=%d\n",
            idx,
            SR_MAX_LOG2);
    abort();
  }

  sr_twiddle_simd_t *tw = (dir == DFT_DIR_FORWARD) ? &sr_twiddles_fwd[idx] : &sr_twiddles_bwd[idx];

  pthread_mutex_lock(&sr_twiddle_mutex);

  if (!tw->initialized) {
    if (!sr_twiddle_table_create(N, dir)) {
      pthread_mutex_unlock(&sr_twiddle_mutex);
      return NULL;
    }
  }

  pthread_mutex_unlock(&sr_twiddle_mutex);
  return tw;
}

#define DFT_C16_SR_MAX_N 65536

static int ensure_sr_twiddles_for_size(int N, dft_dir_t dir)
{
  for (int size = N; size > DFT_C16_SR_MAX_N; size >>= 1) {
    if (!sr_twiddle_table_get(size, dir))
      return 0;
  }

  return 1;
}

static void dft_c16_init_impl(void)
{
  AssertFatal(twiddle_table_get(64) != NULL, "Failed to initialize DFT64 twiddles\n");

  AssertFatal(twiddle_table_get(128) != NULL, "Failed to initialize DFT128 twiddles\n");

  for (int N = 256; N <= DFT_C16_SR_MAX_N; N <<= 1) {
    AssertFatal(sr_twiddle_table_get(N, DFT_DIR_FORWARD) != NULL, "Failed to initialize forward SR twiddles N=%d\n", N);

    AssertFatal(sr_twiddle_table_get(N, DFT_DIR_INVERSE) != NULL, "Failed to initialize inverse SR twiddles N=%d\n", N);
  }
}

/*
 * Called automatically when load_dftslib() loads this shared library.
 */
__attribute__((constructor)) static void dft_c16_library_init(void)
{
  dft_c16_init_impl();
}

//=====================================================================================
// TWIDDLES END
//=====================================================================================

//===================================================================
// DFT64 8x8 int
//===================================================================

static inline __m128i dft64_dc_from_h0(__m256i h0)
{
  const __m256i real_mask = _mm256_setr_epi16(1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);

  const __m256i imag_mask = _mm256_setr_epi16(0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1);

  __m256i real32 = _mm256_madd_epi16(h0, real_mask);
  __m256i imag32 = _mm256_madd_epi16(h0, imag_mask);

  /*
   * After the two horizontal adds, each 128-bit half contains
   * [sum_real, sum_imag, ...].
   */
  __m256i sum = _mm256_hadd_epi32(real32, imag32);
  sum = _mm256_hadd_epi32(sum, sum);

  __m128i dc32 = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));

  /*
   * Rounded division by 8:
   * positive values: +4
   * negative values: +3
   */
  const __m128i sign = _mm_srai_epi32(dc32, 31);

  dc32 = _mm_add_epi32(dc32, _mm_add_epi32(_mm_set1_epi32(4), sign));

  dc32 = _mm_srai_epi32(dc32, 3);

  return _mm_packs_epi32(dc32, _mm_setzero_si128());
}

static inline void dft8x8_q15_256_dir(const __m256i x0,
                                      const __m256i x1,
                                      const __m256i x2,
                                      const __m256i x3,
                                      const __m256i x4,
                                      const __m256i x5,
                                      const __m256i x6,
                                      const __m256i x7,
                                      __m256i *Y0,
                                      __m256i *Y1,
                                      __m256i *Y2,
                                      __m256i *Y3,
                                      __m256i *Y4,
                                      __m256i *Y5,
                                      __m256i *Y6,
                                      __m256i *Y7,
                                      dft_dir_t dir)
{
  const __m256i c = _mm256_set1_epi16(Q15_INV_SQRT2);

  const __m256i s04 = _mm256_adds_epi16(x0, x4);
  const __m256i d04 = _mm256_subs_epi16(x0, x4);

  const __m256i s15 = _mm256_adds_epi16(x1, x5);
  const __m256i d15 = _mm256_subs_epi16(x1, x5);

  const __m256i s26 = _mm256_adds_epi16(x2, x6);
  const __m256i d26 = _mm256_subs_epi16(x2, x6);

  const __m256i s37 = _mm256_adds_epi16(x3, x7);
  const __m256i d37 = _mm256_subs_epi16(x3, x7);

  const __m256i s02 = _mm256_adds_epi16(s04, s26);
  const __m256i d02 = _mm256_subs_epi16(s04, s26);

  const __m256i s13 = _mm256_adds_epi16(s15, s37);
  const __m256i d13 = _mm256_subs_epi16(s15, s37);

  *Y0 = _mm256_adds_epi16(s02, s13);
  *Y4 = _mm256_subs_epi16(s02, s13);

  *Y2 = _mm256_adds_epi16(d02, mul_minus_j_dir_i16_256(d13, dir));
  *Y6 = _mm256_adds_epi16(d02, mul_plus_j_dir_i16_256(d13, dir));

  const __m256i p = _mm256_adds_epi16(d15, d37);
  const __m256i q = _mm256_subs_epi16(d15, d37);

  const __m256i d26_mj = mul_minus_j_dir_i16_256(d26, dir);
  const __m256i d26_pj = mul_plus_j_dir_i16_256(d26, dir);

  const __m256i base_mj = _mm256_adds_epi16(d04, d26_mj);
  const __m256i base_pj = _mm256_adds_epi16(d04, d26_pj);

  const __m256i t1_arg = _mm256_adds_epi16(q, mul_minus_j_dir_i16_256(p, dir));
  const __m256i t3_arg = _mm256_adds_epi16(q, mul_plus_j_dir_i16_256(p, dir));

  const __m256i t1 = _mm256_mulhrs_epi16(c, t1_arg);
  const __m256i t3 = _mm256_mulhrs_epi16(c, t3_arg);

  *Y1 = _mm256_adds_epi16(base_mj, t1);
  *Y5 = _mm256_subs_epi16(base_mj, t1);

  *Y7 = _mm256_adds_epi16(base_pj, t3);
  *Y3 = _mm256_subs_epi16(base_pj, t3);
}

static inline void dft64_avx(const c16_t *src, c16_t *dst, dft_dir_t dir)
{
  const __m256i x0 = _mm256_loadu_si256((const __m256i *)(src + 0));
  const __m256i x1 = _mm256_loadu_si256((const __m256i *)(src + 8));
  const __m256i x2 = _mm256_loadu_si256((const __m256i *)(src + 16));
  const __m256i x3 = _mm256_loadu_si256((const __m256i *)(src + 24));
  const __m256i x4 = _mm256_loadu_si256((const __m256i *)(src + 32));
  const __m256i x5 = _mm256_loadu_si256((const __m256i *)(src + 40));
  const __m256i x6 = _mm256_loadu_si256((const __m256i *)(src + 48));
  const __m256i x7 = _mm256_loadu_si256((const __m256i *)(src + 56));

  __m256i H0, H1, H2, H3;
  __m256i H4, H5, H6, H7;

  dft8x8_q15_256_dir(x0, x1, x2, x3, x4, x5, x6, x7, &H0, &H1, &H2, &H3, &H4, &H5, &H6, &H7, dir);
  const TwiddleTable *tw = &g_tables[DFT_64];
  const __m256i *C64_RE = (dir == DFT_DIR_FORWARD) ? tw->C64_RE_RE_q15_256 : tw->C64_RE_RE_q15_256_inverse;

  const __m256i *C64_IM = (dir == DFT_DIR_FORWARD) ? tw->C64_IM_SIGNED_q15_256 : tw->C64_IM_SIGNED_q15_256_inverse;
  const __m128i dc = dft64_dc_from_h0(H0);
  H0 = _mm256_srai_epi16(H0, 3);

  H1 = complex_mul8_prepack_q15_256(H1, C64_RE[1], C64_IM[1]);
  H2 = complex_mul8_prepack_q15_256(H2, C64_RE[2], C64_IM[2]);
  H3 = complex_mul8_prepack_q15_256(H3, C64_RE[3], C64_IM[3]);
  H4 = complex_mul8_prepack_q15_256(H4, C64_RE[4], C64_IM[4]);
  H5 = complex_mul8_prepack_q15_256(H5, C64_RE[5], C64_IM[5]);
  H6 = complex_mul8_prepack_q15_256(H6, C64_RE[6], C64_IM[6]);
  H7 = complex_mul8_prepack_q15_256(H7, C64_RE[7], C64_IM[7]);

  /*
   * Transpose complex 8x8.
   */
  transpose8_complex_i16_256(&H0, &H1, &H2, &H3, &H4, &H5, &H6, &H7);

  __m256i Y0, Y1, Y2, Y3;
  __m256i Y4, Y5, Y6, Y7;

  /*
   * Second stage.
   */
  dft8x8_q15_256_dir(H0, H1, H2, H3, H4, H5, H6, H7, &Y0, &Y1, &Y2, &Y3, &Y4, &Y5, &Y6, &Y7, dir);
  Y0 = _mm256_blend_epi32(Y0, _mm256_castsi128_si256(dc), 0x01);
  _mm256_storeu_si256((__m256i *)(dst + 0), Y0);
  _mm256_storeu_si256((__m256i *)(dst + 8), Y1);
  _mm256_storeu_si256((__m256i *)(dst + 16), Y2);
  _mm256_storeu_si256((__m256i *)(dst + 24), Y3);
  _mm256_storeu_si256((__m256i *)(dst + 32), Y4);
  _mm256_storeu_si256((__m256i *)(dst + 40), Y5);
  _mm256_storeu_si256((__m256i *)(dst + 48), Y6);
  _mm256_storeu_si256((__m256i *)(dst + 56), Y7);
}

static inline void dft64_q15_128_strided(const c16_t *src, int stride, c16_t *dst, dft_dir_t dir)
{
  if (stride == 1) {
    dft64_avx((c16_t *)src, dst, dir);
    return;
  }
  c16_t tmp[64] __attribute__((aligned(64)));
  for (int i = 0; i < 64; i++) {
    tmp[i] = src[i * stride];
  }

  dft64_avx(tmp, dst, dir);
}
//===================================================================
// DFT128 int
//===================================================================

static inline void dft128_stage0_blk_q15_256_dir(const c16_t *src,
                                                 c16_t *a,
                                                 c16_t *b,
                                                 int blk,
                                                 dft_dir_t dir,
                                                 const __m256i *tw_RE,
                                                 const __m256i *tw_IM)
{
  const __m256i x0 = _mm256_loadu_si256((const __m256i *)(src + 8 * blk));

  const __m256i x1 = _mm256_loadu_si256((const __m256i *)(src + 64 + 8 * blk));

  __m256i sum = _mm256_adds_epi16(x0, x1);
  __m256i diff = _mm256_subs_epi16(x0, x1);

  const __m256i s = _mm256_set1_epi16(Q15_INV_SQRT2);
  sum = _mm256_mulhrs_epi16(sum, s);

  diff = complex_mul8_prepack_q15_256(diff, tw_RE[blk], tw_IM[blk]);

  _mm256_store_si256((__m256i *)(a + 8 * blk), sum);

  _mm256_store_si256((__m256i *)(b + 8 * blk), diff);
}

static inline void interleave64_complex_q15_256(const c16_t *A, const c16_t *B, c16_t *dst)
{
  for (int blk = 0; blk < 8; blk++) {
    const __m256i va = _mm256_load_si256((const __m256i *)(A + 8 * blk));

    const __m256i vb = _mm256_load_si256((const __m256i *)(B + 8 * blk));

    /*
     * va = [A0 A1 A2 A3 | A4 A5 A6 A7]
     * vb = [B0 B1 B2 B3 | B4 B5 B6 B7]
     *
     * Each A0/B0 is one c16_t = 32 bits.
     */
    const __m256i lo = _mm256_unpacklo_epi32(va, vb);
    const __m256i hi = _mm256_unpackhi_epi32(va, vb);

    /*
     * out0 = [A0 B0 A1 B1 A2 B2 A3 B3]
     * out1 = [A4 B4 A5 B5 A6 B6 A7 B7]
     */
    const __m256i out0 = _mm256_permute2x128_si256(lo, hi, 0x20);
    const __m256i out1 = _mm256_permute2x128_si256(lo, hi, 0x31);

    _mm256_storeu_si256((__m256i *)(dst + 16 * blk), out0);

    _mm256_storeu_si256((__m256i *)(dst + 16 * blk + 8), out1);
  }
}

static inline void dft128_dir(const c16_t *src, c16_t *dst, dft_dir_t dir)
{
  c16_t a[64] __attribute__((aligned(32)));
  c16_t b[64] __attribute__((aligned(32)));

  c16_t A[64] __attribute__((aligned(32)));
  c16_t B[64] __attribute__((aligned(32)));
  const TwiddleTable *tw = &g_tables[DFT_128];

  const __m256i *W128_RE = dir == DFT_DIR_FORWARD ? tw->W128_RE_RE_q15_256 : tw->W128_RE_RE_q15_256_inverse;

  const __m256i *W128_IM = dir == DFT_DIR_FORWARD ? tw->W128_IM_SIGNED_q15_256 : tw->W128_IM_SIGNED_q15_256_inverse;

  for (int blk = 0; blk < 8; blk++) {
    dft128_stage0_blk_q15_256_dir(src, a, b, blk, dir, W128_RE, W128_IM);
  }

  dft64_avx(a, A, dir);
  dft64_avx(b, B, dir);

  interleave64_complex_q15_256(A, B, dst);
}

static inline void dft128_q15_128_strided(const c16_t *src, int stride, c16_t *dst, dft_dir_t dir)
{
  if (stride == 1) {
    dft128_dir((c16_t *)src, dst, dir);
    return;
  }

  c16_t tmp[128] __attribute__((aligned(64)));

  for (int i = 0; i < 128; i++) {
    tmp[i] = src[i * stride];
  }

  dft128_dir(tmp, dst, dir);
}

//===================================================================
// DFT SPLIT
//===================================================================

static inline size_t split_radix_work_len_c16(int N)
{
  size_t need = 0;

  while (N > 128) {
    need += 2u * (size_t)N;
    N >>= 1;
  }

  return need;
}

static inline size_t split_radix_work_len_strided_c16(int N)
{
  size_t need = 0;

  while (N > 128) {
    need += (size_t)N;
    N >>= 1;
  }

  return need;
}

static inline void sr_combine_simd(c16_t *E, c16_t *O1, c16_t *O3, c16_t *y, int N, const sr_twiddle_simd_t *tw, dft_dir_t dir)
{
  int half = N / 2;
  int quarter = N / 4;

  const simde__m256i swap_mask = simde_mm256_setr_epi8(2,
                                                       3,
                                                       0,
                                                       1,
                                                       6,
                                                       7,
                                                       4,
                                                       5,
                                                       10,
                                                       11,
                                                       8,
                                                       9,
                                                       14,
                                                       15,
                                                       12,
                                                       13,
                                                       2,
                                                       3,
                                                       0,
                                                       1,
                                                       6,
                                                       7,
                                                       4,
                                                       5,
                                                       10,
                                                       11,
                                                       8,
                                                       9,
                                                       14,
                                                       15,
                                                       12,
                                                       13);

  simde__m256i sign_mask;

  if (dir == DFT_DIR_FORWARD) {
    sign_mask = simde_mm256_setr_epi16(1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1);
  } else {
    sign_mask = simde_mm256_setr_epi16(-1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 1);
  }

  const simde__m256i sqrt2_inv = simde_mm256_set1_epi16(Q15_INV_SQRT2);

  for (int b = 0; b < tw->blocks; b++) {
    int k = 8 * b;

    simde__m256i O1v = simde_mm256_load_si256((simde__m256i *)&O1[k]);
    simde__m256i O3v = simde_mm256_load_si256((simde__m256i *)&O3[k]);

    simde__m256i t1 = c16_mul_q15_simd256(O1v, tw->W1_RE_NEGIM[b], tw->W1_IM_RE[b]);

    simde__m256i t2 = c16_mul_q15_simd256(O3v, tw->W3_RE_NEGIM[b], tw->W3_IM_RE[b]);

    simde__m256i a = simde_mm256_adds_epi16(t1, t2);
    simde__m256i d = simde_mm256_subs_epi16(t1, t2);

    simde__m256i bval = simde_mm256_shuffle_epi8(d, swap_mask);
    bval = simde_mm256_sign_epi16(bval, sign_mask);

    simde__m256i E0 = simde_mm256_load_si256((simde__m256i *)&E[k]);
    simde__m256i E1 = simde_mm256_load_si256((simde__m256i *)&E[k + quarter]);

    E0 = simde_mm256_mulhrs_epi16(E0, sqrt2_inv);
    E1 = simde_mm256_mulhrs_epi16(E1, sqrt2_inv);

    simde__m256i Y0 = simde_mm256_adds_epi16(E0, a);
    simde__m256i Y2 = simde_mm256_subs_epi16(E0, a);
    simde__m256i Y1 = simde_mm256_adds_epi16(E1, bval);
    simde__m256i Y3 = simde_mm256_subs_epi16(E1, bval);

    simde_mm256_store_si256((simde__m256i *)&y[k], Y0);
    simde_mm256_store_si256((simde__m256i *)&y[k + quarter], Y1);
    simde_mm256_store_si256((simde__m256i *)&y[k + half], Y2);
    simde_mm256_store_si256((simde__m256i *)&y[k + 3 * quarter], Y3);
  }
}

static inline void pack_split_radix_input_avx2_fused(const c16_t *__restrict x, c16_t *__restrict sub_in, int N)
{
  _Static_assert(sizeof(c16_t) == 4, "c16_t must be 32-bit");

  const int half = N >> 1;
  const int quarter = N >> 2;

  c16_t *__restrict E_in = sub_in;
  c16_t *__restrict O1_in = sub_in + half;
  c16_t *__restrict O3_in = sub_in + half + quarter;

  const simde__m256i idx = simde_mm256_setr_epi32(0, 2, 4, 6, 1, 5, 3, 7);

  int in = 0;
  int e = 0;
  int o = 0;

  for (; in + 32 <= N; in += 32, e += 16, o += 8) {
    simde__m256i v0 = simde_mm256_loadu_si256((const simde__m256i *)&x[in + 0]);

    simde__m256i v1 = simde_mm256_loadu_si256((const simde__m256i *)&x[in + 8]);

    simde__m256i v2 = simde_mm256_loadu_si256((const simde__m256i *)&x[in + 16]);

    simde__m256i v3 = simde_mm256_loadu_si256((const simde__m256i *)&x[in + 24]);

    /*
     * p0 = [x0,  x2,  x4,  x6,  x1,  x5,  x3,  x7]
     * p1 = [x8,  x10, x12, x14, x9,  x13, x11, x15]
     * p2 = [x16, x18, x20, x22, x17, x21, x19, x23]
     * p3 = [x24, x26, x28, x30, x25, x29, x27, x31]
     */
    simde__m256i p0 = simde_mm256_permutevar8x32_epi32(v0, idx);
    simde__m256i p1 = simde_mm256_permutevar8x32_epi32(v1, idx);
    simde__m256i p2 = simde_mm256_permutevar8x32_epi32(v2, idx);
    simde__m256i p3 = simde_mm256_permutevar8x32_epi32(v3, idx);

    /*
     * E output:
     *
     * low128(p0) = x0,  x2,  x4,  x6
     * low128(p1) = x8,  x10, x12, x14
     * low128(p2) = x16, x18, x20, x22
     * low128(p3) = x24, x26, x28, x30
     */
    simde_mm_store_si128((simde__m128i *)&E_in[e + 0], simde_mm256_castsi256_si128(p0));

    simde_mm_store_si128((simde__m128i *)&E_in[e + 4], simde_mm256_castsi256_si128(p1));

    simde_mm_store_si128((simde__m128i *)&E_in[e + 8], simde_mm256_castsi256_si128(p2));

    simde_mm_store_si128((simde__m128i *)&E_in[e + 12], simde_mm256_castsi256_si128(p3));

    /*
     * high128(p0) = x1,  x5,  x3,  x7
     * high128(p1) = x9,  x13, x11, x15
     * high128(p2) = x17, x21, x19, x23
     * high128(p3) = x25, x29, x27, x31
     */
    simde__m128i h0 = simde_mm256_extracti128_si256(p0, 1);
    simde__m128i h1 = simde_mm256_extracti128_si256(p1, 1);
    simde__m128i h2 = simde_mm256_extracti128_si256(p2, 1);
    simde__m128i h3 = simde_mm256_extracti128_si256(p3, 1);

    /*
     * O1:
     * unpacklo_epi64(h0,h1) = x1,  x5,  x9,  x13
     * unpacklo_epi64(h2,h3) = x17, x21, x25, x29
     *
     * O3:
     * unpackhi_epi64(h0,h1) = x3,  x7,  x11, x15
     * unpackhi_epi64(h2,h3) = x19, x23, x27, x31
     */
    simde__m128i o1_0 = simde_mm_unpacklo_epi64(h0, h1);
    simde__m128i o3_0 = simde_mm_unpackhi_epi64(h0, h1);

    simde__m128i o1_1 = simde_mm_unpacklo_epi64(h2, h3);
    simde__m128i o3_1 = simde_mm_unpackhi_epi64(h2, h3);

    simde_mm_store_si128((simde__m128i *)&O1_in[o + 0], o1_0);
    simde_mm_store_si128((simde__m128i *)&O1_in[o + 4], o1_1);

    simde_mm_store_si128((simde__m128i *)&O3_in[o + 0], o3_0);
    simde_mm_store_si128((simde__m128i *)&O3_in[o + 4], o3_1);
  }
}

static void dft_split_radix_pure_simd_core(c16_t *__restrict x, c16_t *__restrict y, c16_t *__restrict work, int N, dft_dir_t dir)
{
  if (N == 64) {
    dft64_avx(x, y, dir);
    return;
  }

  if (N == 128) {
    dft128_dir(x, y, dir);
    return;
  }

  const int half = N >> 1;
  const int quarter = N >> 2;

  c16_t *sub_in = work;
  c16_t *sub_out = work + N;

  c16_t *E = sub_out;
  c16_t *O1 = sub_out + half;
  c16_t *O3 = sub_out + half + quarter;

  pack_split_radix_input_avx2_fused(x, sub_in, N);

  dft_split_radix_pure_simd_core(sub_in, E, work + 2 * N, half, dir);

  dft_split_radix_pure_simd_core(sub_in + half, O1, work + 2 * N, quarter, dir);

  dft_split_radix_pure_simd_core(sub_in + half + quarter, O3, work + 2 * N, quarter, dir);
  const int idx = log2_int((unsigned int)N);
  const sr_twiddle_simd_t *table = (dir == DFT_DIR_FORWARD) ? &sr_twiddles_fwd[idx] : &sr_twiddles_bwd[idx];

  AssertFatal(table->initialized, "Missing split-radix twiddles for N=%d\n", N);
  sr_combine_simd(E, O1, O3, y, N, table, dir);
}

static void dft_split_radix_pure_simd(c16_t *x, c16_t *y, int N, dft_dir_t dir)
{
  AssertFatal(ensure_sr_twiddles_for_size(N, dir), "Failed to initialize split-radix twiddles for N=%d\n", N);
  const size_t work_len = split_radix_work_len_c16(N);

  if (work_len == 0) {
    dft_split_radix_pure_simd_core(x, y, NULL, N, dir);
    return;
  }

  if (work_len <= SPLIT_RADIX_STACK_MAX_C16) {
    c16_t work_stack[SPLIT_RADIX_STACK_MAX_C16] __attribute__((aligned(64)));

    dft_split_radix_pure_simd_core(x, y, work_stack, N, dir);
    return;
  }
  c16_t *work = (c16_t *)aligned_malloc64(sizeof(c16_t) * work_len);

  if (!work) {
    LOG_E(PHY, "dft_split_radix_pure_simd: allocation failed N=%d work_len=%zu\n", N, work_len);
    return;
  }

  dft_split_radix_pure_simd_core(x, y, work, N, dir);

  free(work);
}

static void dft_split_radix_pure_simd_core_strided(const c16_t *__restrict x,
                                                   int stride,
                                                   c16_t *__restrict y,
                                                   c16_t *__restrict work,
                                                   int N,
                                                   dft_dir_t dir)
{
  if (N == 64) {
    dft64_q15_128_strided(x, stride, y, dir);
    return;
  }
  if (N == 128) {
    dft128_q15_128_strided(x, stride, y, dir);
    return;
  }
  const int half = N >> 1;
  const int quarter = N >> 2;
  c16_t *E = work;
  c16_t *O1 = work + half;
  c16_t *O3 = work + half + quarter;
  c16_t *child_work = work + N;

  dft_split_radix_pure_simd_core_strided(x + 0 * stride, stride * 2, E, child_work, half, dir);

  dft_split_radix_pure_simd_core_strided(x + 1 * stride, stride * 4, O1, child_work, quarter, dir);

  dft_split_radix_pure_simd_core_strided(x + 3 * stride, stride * 4, O3, child_work, quarter, dir);

  const int idx = log2_int((unsigned int)N);
  const sr_twiddle_simd_t *table = (dir == DFT_DIR_FORWARD) ? &sr_twiddles_fwd[idx] : &sr_twiddles_bwd[idx];

  AssertFatal(table->initialized, "Missing split-radix twiddles for N=%d\n", N);
  sr_combine_simd(E, O1, O3, y, N, table, dir);
}

static void dft_split_radix_pure_simd_strided(const c16_t *x, int stride, c16_t *y, int N, dft_dir_t dir)
{
  AssertFatal(ensure_sr_twiddles_for_size(N, dir), "Failed to initialize split-radix twiddles for N=%d\n", N);
  const size_t work_len = split_radix_work_len_strided_c16(N);

  if (work_len == 0) {
    dft_split_radix_pure_simd_core_strided(x, stride, y, NULL, N, dir);
    return;
  }

  if (work_len <= SPLIT_RADIX_STACK_MAX_C16) {
    c16_t work_stack[SPLIT_RADIX_STACK_MAX_C16] __attribute__((aligned(64)));

    dft_split_radix_pure_simd_core_strided(x, stride, y, work_stack, N, dir);
    return;
  }
  c16_t *work = (c16_t *)aligned_malloc64(sizeof(c16_t) * work_len);

  if (!work) {
    LOG_E(PHY, "dft_split_radix_pure_simd_strided: allocation failed N=%d work_len=%zu\n", N, work_len);
    return;
  }

  dft_split_radix_pure_simd_core_strided(x, stride, y, work, N, dir);

  free(work);
}

//===================================================================
// DFT4
//===================================================================
static inline __m128i dft4_avx(__m128i x, dft_dir_t dir)
{
  /*
   * lo = [x0 x1 x0 x1]
   * hi = [x2 x3 x2 x3]
   */
  x = _mm_srai_epi16(x, 1);
  const __m128i lo = _mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 1, 0));
  const __m128i hi = _mm_shuffle_epi32(x, _MM_SHUFFLE(3, 2, 3, 2));

  const __m128i s = _mm_adds_epi16(lo, hi);
  const __m128i d = _mm_subs_epi16(lo, hi);

  const __m128i s_sw = _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1));
  const __m128i d_sw = _mm_shuffle_epi32(d, _MM_SHUFFLE(2, 3, 0, 1));

  const __m128i y0v = _mm_adds_epi16(s, s_sw);
  const __m128i y2v = _mm_subs_epi16(s, s_sw);

  const __m128i y1v = _mm_adds_epi16(d, mul_minus_j_dir_i16_128(d_sw, dir));
  const __m128i y3v = _mm_adds_epi16(d, mul_plus_j_dir_i16_128(d_sw, dir));

  /*
   * y0v lane0 = Y0
   * y1v lane0 = Y1
   * y2v lane0 = Y2
   * y3v lane0 = Y3
   */
  const __m128i y01 = _mm_unpacklo_epi32(y0v, y1v); // [Y0 Y1 ... ...]
  const __m128i y23 = _mm_unpacklo_epi32(y2v, y3v); // [Y2 Y3 ... ...]

  return _mm_unpacklo_epi64(y01, y23); // [Y0 Y1 Y2 Y3]
}

static inline void dft4_void(const c16_t *src, c16_t *dst, dft_dir_t dir)
{
  const __m128i x = _mm_loadu_si128((const __m128i *)src);

  const __m128i y = dft4_avx(x, dir);

  _mm_storeu_si128((__m128i *)dst, y);
}

static inline void dft4_strided_q15_128(const c16_t *src, int stride, c16_t *dst, dft_dir_t dir)
{
  const __m128i x = load4_complex_strided_c16(src, stride, 0);
  _mm_storeu_si128((__m128i *)dst, dft4_avx(x, dir));
}

static inline void dft8_avx(const c16_t *src, c16_t *dst, dft_dir_t dir)
{
  /*
   * v0 = [x0 x1 x2 x3]
   * v1 = [x4 x5 x6 x7]
   */
  const __m128i v0 = _mm_loadu_si128((const __m128i *)(src + 0));

  const __m128i v1 = _mm_loadu_si128((const __m128i *)(src + 4));

  /*
   * E_in = [x0 x2 x4 x6]
   * O_in = [x1 x3 x5 x7]
   */
  const __m128i v0_even = _mm_shuffle_epi32(v0, _MM_SHUFFLE(2, 0, 2, 0));

  const __m128i v1_even = _mm_shuffle_epi32(v1, _MM_SHUFFLE(2, 0, 2, 0));

  const __m128i v0_odd = _mm_shuffle_epi32(v0, _MM_SHUFFLE(3, 1, 3, 1));

  const __m128i v1_odd = _mm_shuffle_epi32(v1, _MM_SHUFFLE(3, 1, 3, 1));

  const __m128i E_in = _mm_unpacklo_epi64(v0_even, v1_even); // [x0 x2 x4 x6]

  const __m128i O_in = _mm_unpacklo_epi64(v0_odd, v1_odd); // [x1 x3 x5 x7]

  /*
   * E = DFT4(x0, x2, x4, x6)
   * O = DFT4(x1, x3, x5, x7)
   */
  const __m128i E = q15_mul_i16_128(dft4_avx(E_in, dir), Q15_INV_SQRT2);
  const __m128i O = dft4_avx(O_in, dir);

  /*
   * Twiddles W8 forward :
   *
   * W8^0 =  1 + j0
   * W8^1 =  c - jc
   * W8^2 =  0 - j1
   * W8^3 = -c - jc
   *
   * c = 1/sqrt(2) = 23170
   *
   * w_re_re     = [wr0 wr0 wr1 wr1 wr2 wr2 wr3 wr3]
   * w_im_signed = [-wi0 wi0 -wi1 wi1 -wi2 wi2 -wi3 wi3]
   */
  const __m128i W8_RE_RE = _mm_setr_epi16(23170, 23170, Q15_HALF, Q15_HALF, 0, 0, -Q15_HALF, -Q15_HALF);

  const __m128i W8_IM_SIGNED_FWD = _mm_setr_epi16(0, 0, Q15_HALF, -Q15_HALF, 23170, -23170, Q15_HALF, -Q15_HALF);

  const __m128i W8_IM_SIGNED = twiddle_im_dir_128(W8_IM_SIGNED_FWD, dir);

  /*
   * T[k] = W8^k * O[k], k = 0..3
   */
  const __m128i T = complex_mul4_prepack_q15_128(O, W8_RE_RE, W8_IM_SIGNED);

  /*
   * Combine radix-2 :
   *
   * Y[k]     = E[k] + T[k]
   * Y[k + 4] = E[k] - T[k]
   */
  const __m128i Y03 = _mm_adds_epi16(E, T); // [Y0 Y1 Y2 Y3]
  const __m128i Y47 = _mm_subs_epi16(E, T); // [Y4 Y5 Y6 Y7]

  _mm_storeu_si128((__m128i *)(dst + 0), Y03);
  _mm_storeu_si128((__m128i *)(dst + 4), Y47);
}

static inline void dft8_strided_q15_128(const c16_t *src, int stride, c16_t *dst, dft_dir_t dir)
{
  c16_t tmp[8] __attribute__((aligned(16)));

  for (int i = 0; i < 8; i++) {
    tmp[i] = src[i * stride];
  }

  dft8_avx(tmp, dst, dir);
}

/*
 * Twiddles for DFT16 radix-4 combine.
 *
 * Format:
 *   RE_RE     = [ re0, re0, re1, re1, re2, re2, re3, re3]
 *   IM_SIGNED = [-im0, im0,-im1, im1,-im2, im2,-im3, im3]
 *
 * Forward:
 *   W16^k = cos(2*pi*k/16) - j sin(2*pi*k/16)
 */

/* W16^(1*j), j = 0..3 */
static const int16_t W16_1_RE_RE[8] __attribute__((
    aligned(16))) = {Q15_ONE, Q15_ONE, Q15_COS_PI_8, Q15_COS_PI_8, Q15_INV_SQRT2, Q15_INV_SQRT2, Q15_SIN_PI_8, Q15_SIN_PI_8};

static const int16_t W16_1_IM_SIGNED[8]
    __attribute__((aligned(16))) = {0, 0, Q15_SIN_PI_8, -Q15_SIN_PI_8, Q15_INV_SQRT2, -Q15_INV_SQRT2, Q15_COS_PI_8, -Q15_COS_PI_8};

/* W16^(2*j), j = 0..3 */
static const int16_t W16_2_RE_RE[8]
    __attribute__((aligned(16))) = {Q15_ONE, Q15_ONE, Q15_INV_SQRT2, Q15_INV_SQRT2, 0, 0, -Q15_INV_SQRT2, -Q15_INV_SQRT2};

static const int16_t W16_2_IM_SIGNED[8]
    __attribute__((aligned(16))) = {0, 0, Q15_INV_SQRT2, -Q15_INV_SQRT2, Q15_ONE, -Q15_ONE, Q15_INV_SQRT2, -Q15_INV_SQRT2};

/* W16^(3*j), j = 0..3 */
static const int16_t W16_3_RE_RE[8] __attribute__((
    aligned(16))) = {Q15_ONE, Q15_ONE, Q15_SIN_PI_8, Q15_SIN_PI_8, -Q15_INV_SQRT2, -Q15_INV_SQRT2, -Q15_COS_PI_8, -Q15_COS_PI_8};

static const int16_t W16_3_IM_SIGNED[8]
    __attribute__((aligned(16))) = {0, 0, Q15_COS_PI_8, -Q15_COS_PI_8, Q15_INV_SQRT2, -Q15_INV_SQRT2, -Q15_SIN_PI_8, Q15_SIN_PI_8};

static inline void dft4x4_q15_128(const __m128i x0,
                                  const __m128i x1,
                                  const __m128i x2,
                                  const __m128i x3,
                                  __m128i *Y0,
                                  __m128i *Y1,
                                  __m128i *Y2,
                                  __m128i *Y3,
                                  dft_dir_t dir)
{
  const __m128i x0s = _mm_srai_epi16(x0, 1);
  const __m128i x1s = _mm_srai_epi16(x1, 1);
  const __m128i x2s = _mm_srai_epi16(x2, 1);
  const __m128i x3s = _mm_srai_epi16(x3, 1);

  const __m128i s02 = _mm_adds_epi16(x0s, x2s);
  const __m128i d02 = _mm_subs_epi16(x0s, x2s);

  const __m128i s13 = _mm_adds_epi16(x1s, x3s);
  const __m128i d13 = _mm_subs_epi16(x1s, x3s);

  *Y0 = _mm_adds_epi16(s02, s13);
  *Y2 = _mm_subs_epi16(s02, s13);

  /*
   * Forward DFT4:
   * Y1 = d02 - j*d13
   * Y3 = d02 + j*d13
   */
  *Y1 = _mm_adds_epi16(d02, mul_minus_j_dir_i16_128(d13, dir));
  *Y3 = _mm_adds_epi16(d02, mul_plus_j_dir_i16_128(d13, dir));
}
static inline void transpose4_complex_i16_128(__m128i *Y0, __m128i *Y1, __m128i *Y2, __m128i *Y3)
{
  const __m128i a = *Y0; // [a0 a1 a2 a3]
  const __m128i b = *Y1; // [b0 b1 b2 b3]
  const __m128i c = *Y2; // [c0 c1 c2 c3]
  const __m128i d = *Y3; // [d0 d1 d2 d3]

  const __m128i ab_lo = _mm_unpacklo_epi32(a, b); // [a0 b0 a1 b1]
  const __m128i ab_hi = _mm_unpackhi_epi32(a, b); // [a2 b2 a3 b3]

  const __m128i cd_lo = _mm_unpacklo_epi32(c, d); // [c0 d0 c1 d1]
  const __m128i cd_hi = _mm_unpackhi_epi32(c, d); // [c2 d2 c3 d3]

  *Y0 = _mm_unpacklo_epi64(ab_lo, cd_lo); // [a0 b0 c0 d0]
  *Y1 = _mm_unpackhi_epi64(ab_lo, cd_lo); // [a1 b1 c1 d1]
  *Y2 = _mm_unpacklo_epi64(ab_hi, cd_hi); // [a2 b2 c2 d2]
  *Y3 = _mm_unpackhi_epi64(ab_hi, cd_hi); // [a3 b3 c3 d3]
}

static inline void combine16_q15_128(const __m128i H[4], c16_t *dst, dft_dir_t dir)
{
  /*
   * H[0] = [H0[0] H0[1] H0[2] H0[3]]
   * H[1] = [H1[0] H1[1] H1[2] H1[3]]
   * H[2] = [H2[0] H2[1] H2[2] H2[3]]
   * H[3] = [H3[0] H3[1] H3[2] H3[3]]
   */
  const __m128i W1_RE_RE = _mm_load_si128((const __m128i *)W16_1_RE_RE);

  const __m128i W1_IM_SIGNED = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W16_1_IM_SIGNED), dir);

  const __m128i W2_RE_RE = _mm_load_si128((const __m128i *)W16_2_RE_RE);

  const __m128i W2_IM_SIGNED = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W16_2_IM_SIGNED), dir);

  const __m128i W3_RE_RE = _mm_load_si128((const __m128i *)W16_3_RE_RE);

  const __m128i W3_IM_SIGNED = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W16_3_IM_SIGNED), dir);
  const __m128i A0 = H[0];

  const __m128i A1 = complex_mul4_prepack_q15_128(H[1], W1_RE_RE, W1_IM_SIGNED);

  const __m128i A2 = complex_mul4_prepack_q15_128(H[2], W2_RE_RE, W2_IM_SIGNED);

  const __m128i A3 = complex_mul4_prepack_q15_128(H[3], W3_RE_RE, W3_IM_SIGNED);

  __m128i t0 = A0;
  __m128i t1 = A1;
  __m128i t2 = A2;
  __m128i t3 = A3;

  transpose4_complex_i16_128(&t0, &t1, &t2, &t3);

  __m128i Y0, Y1, Y2, Y3;

  dft4x4_q15_128(t0, t1, t2, t3, &Y0, &Y1, &Y2, &Y3, dir);

  _mm_storeu_si128((__m128i *)(dst + 0), Y0);
  _mm_storeu_si128((__m128i *)(dst + 4), Y1);
  _mm_storeu_si128((__m128i *)(dst + 8), Y2);
  _mm_storeu_si128((__m128i *)(dst + 12), Y3);
}

static inline void dft16_q15_128(const c16_t *src, c16_t *dst, dft_dir_t dir)
{
  __m128i H[4] __attribute__((aligned(16)));

  const __m128i x0 = _mm_loadu_si128((const __m128i *)(src + 0));

  const __m128i x1 = _mm_loadu_si128((const __m128i *)(src + 4));

  const __m128i x2 = _mm_loadu_si128((const __m128i *)(src + 8));

  const __m128i x3 = _mm_loadu_si128((const __m128i *)(src + 12));

  dft4x4_q15_128(x0, x1, x2, x3, &H[0], &H[1], &H[2], &H[3], dir);

  combine16_q15_128(H, dst, dir);
}

/*
 * Format:
 *   RE_RE     = [ re0, re0, re1, re1, re2, re2, re3, re3]
 *   IM_SIGNED = [-im0, im0,-im1, im1,-im2, im2,-im3, im3]
 *
 * Forward:
 *   W12^k = cos(2*pi*k/12) - j sin(2*pi*k/12)
 *
 */

/* W12^k, k = 0..3 */
static const int16_t W12_R3_W1_RE_RE[8] __attribute__((aligned(16))) = {Q15_INV_SQRT3,
                                                                        Q15_INV_SQRT3, /* 1 / sqrt3 */
                                                                        Q15_HALF,
                                                                        Q15_HALF, /* cos(pi/6) / sqrt3 = 1/2 */
                                                                        Q15_INV_2SQRT3,
                                                                        Q15_INV_2SQRT3, /* cos(pi/3) / sqrt3 */
                                                                        0,
                                                                        0};

static const int16_t W12_R3_W1_IM_SIGNED[8]
    __attribute__((aligned(16))) = {0, 0, Q15_INV_2SQRT3, -Q15_INV_2SQRT3, Q15_HALF, -Q15_HALF, Q15_INV_SQRT3, -Q15_INV_SQRT3};

/* W12^(2k), k = 0..3 */
static const int16_t W12_R3_W2_RE_RE[8] __attribute__((aligned(16))) = {Q15_INV_SQRT3,
                                                                        Q15_INV_SQRT3,
                                                                        Q15_INV_2SQRT3,
                                                                        Q15_INV_2SQRT3,
                                                                        -Q15_INV_2SQRT3,
                                                                        -Q15_INV_2SQRT3,
                                                                        -Q15_INV_SQRT3,
                                                                        -Q15_INV_SQRT3};

static const int16_t W12_R3_W2_IM_SIGNED[8] __attribute__((aligned(16))) = {0, 0, Q15_HALF, -Q15_HALF, Q15_HALF, -Q15_HALF, 0, 0};

static inline __m128i pack3_complex_plus_zero_c16(const c16_t a, const c16_t b, const c16_t c)
{
  return _mm_setr_epi16(a.r, a.i, b.r, b.i, c.r, c.i, 0, 0);
}

static inline void dft12_q15_128(const c16_t *src, c16_t *dst, dft_dir_t dir)
{
  /*
   * lane 0 : src[0], src[3], src[6],  src[9]
   * lane 1 : src[1], src[4], src[7],  src[10]
   * lane 2 : src[2], src[5], src[8],  src[11]
   * lane 3 : dummy
   */
  const __m128i x0 = pack3_complex_plus_zero_c16(src[0], src[1], src[2]);
  const __m128i x1 = pack3_complex_plus_zero_c16(src[3], src[4], src[5]);
  const __m128i x2 = pack3_complex_plus_zero_c16(src[6], src[7], src[8]);
  const __m128i x3 = pack3_complex_plus_zero_c16(src[9], src[10], src[11]);

  __m128i H0, H1, H2, H3;

  /*
   * dft4x4_q15_128 applies the required /2 scaling.
   *
   * H0 = [F0[0], F1[0], F2[0], dummy]
   * H1 = [F0[1], F1[1], F2[1], dummy]
   * H2 = [F0[2], F1[2], F2[2], dummy]
   * H3 = [F0[3], F1[3], F2[3], dummy]
   */
  dft4x4_q15_128(x0, x1, x2, x3, &H0, &H1, &H2, &H3, dir);

  /*
   * After the transpose:
   *
   * H0 = A  = [F0[0], F0[1], F0[2], F0[3]]
   * H1 = X1 = [F1[0], F1[1], F1[2], F1[3]]
   * H2 = X2 = [F2[0], F2[1], F2[2], F2[3]]
   * H3 = dummy
   */
  transpose4_complex_i16_128(&H0, &H1, &H2, &H3);

  const __m128i A = H0;
  const __m128i X1 = H1;
  const __m128i X2 = H2;

  const __m128i W1_RE = _mm_load_si128((const __m128i *)W12_R3_W1_RE_RE);

  const __m128i W1_IM = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W12_R3_W1_IM_SIGNED), dir);

  const __m128i W2_RE = _mm_load_si128((const __m128i *)W12_R3_W2_RE_RE);

  const __m128i W2_IM = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W12_R3_W2_IM_SIGNED), dir);

  /*
   * A is also scaled by 1/sqrt(3). Since the first dft4x4 stage
   * already applies /2, the total scale is:
   *
   *   /2 * /sqrt(3) = 1/sqrt(12)
   */
  const __m128i As = q15_mul_i16_128(A, Q15_INV_SQRT3);

  /*
   * B[k] = W12^k    * X1[k] / sqrt(3)
   * C[k] = W12^(2k) * X2[k] / sqrt(3)
   */
  const __m128i B = complex_mul4_prepack_q15_128(X1, W1_RE, W1_IM);

  const __m128i C = complex_mul4_prepack_q15_128(X2, W2_RE, W2_IM);

  const __m128i S = _mm_adds_epi16(B, C);
  const __m128i D = _mm_subs_epi16(B, C);

  /*
   * Y0 = A + B + C
   */
  const __m128i Y0 = _mm_adds_epi16(As, S);

  /*
   * base = A - 1/2 * (B + C)
   */
  const __m128i halfS = q15_mul_i16_128(S, Q15_HALF);

  const __m128i base = _mm_subs_epi16(As, halfS);

  /*
   * c3D = sqrt(3)/2 * (B - C)
   */
  const __m128i c3D = q15_mul_i16_128(D, Q15_SQRT3_OVER_2);

  /*
   * Forward radix-3:
   *
   * Y1 = base - j*c3D
   * Y2 = base + j*c3D
   */
  const __m128i Y1 = _mm_adds_epi16(base, mul_minus_j_dir_i16_128(c3D, dir));

  const __m128i Y2 = _mm_adds_epi16(base, mul_plus_j_dir_i16_128(c3D, dir));

  /*
   * dst[0..3]   = Y0
   * dst[4..7]   = Y1
   * dst[8..11]  = Y2
   */
  _mm_storeu_si128((__m128i *)(dst + 0), Y0);
  _mm_storeu_si128((__m128i *)(dst + 4), Y1);
  _mm_storeu_si128((__m128i *)(dst + 8), Y2);
}

static inline void dft12_q15_128_strided(const c16_t *src, int stride, c16_t *dst, dft_dir_t dir)
{
  const __m128i x0 = pack3_complex_plus_zero_c16(src[0 * stride], src[1 * stride], src[2 * stride]);

  const __m128i x1 = pack3_complex_plus_zero_c16(src[3 * stride], src[4 * stride], src[5 * stride]);

  const __m128i x2 = pack3_complex_plus_zero_c16(src[6 * stride], src[7 * stride], src[8 * stride]);

  const __m128i x3 = pack3_complex_plus_zero_c16(src[9 * stride], src[10 * stride], src[11 * stride]);

  __m128i H0, H1, H2, H3;

  dft4x4_q15_128(x0, x1, x2, x3, &H0, &H1, &H2, &H3, dir);

  transpose4_complex_i16_128(&H0, &H1, &H2, &H3);

  const __m128i A = H0;
  const __m128i X1 = H1;
  const __m128i X2 = H2;

  const __m128i W1_RE = _mm_load_si128((const __m128i *)W12_R3_W1_RE_RE);

  const __m128i W1_IM = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W12_R3_W1_IM_SIGNED), dir);

  const __m128i W2_RE = _mm_load_si128((const __m128i *)W12_R3_W2_RE_RE);

  const __m128i W2_IM = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W12_R3_W2_IM_SIGNED), dir);

  const __m128i As = q15_mul_i16_128(A, Q15_INV_SQRT3);

  const __m128i B = complex_mul4_prepack_q15_128(X1, W1_RE, W1_IM);

  const __m128i C = complex_mul4_prepack_q15_128(X2, W2_RE, W2_IM);

  const __m128i S = _mm_adds_epi16(B, C);
  const __m128i D = _mm_subs_epi16(B, C);

  const __m128i Y0 = _mm_adds_epi16(As, S);

  const __m128i halfS = q15_mul_i16_128(S, Q15_HALF);

  const __m128i base = _mm_subs_epi16(As, halfS);

  const __m128i c3D = q15_mul_i16_128(D, Q15_SQRT3_OVER_2);

  const __m128i Y1 = _mm_adds_epi16(base, mul_minus_j_dir_i16_128(c3D, dir));

  const __m128i Y2 = _mm_adds_epi16(base, mul_plus_j_dir_i16_128(c3D, dir));

  _mm_storeu_si128((__m128i *)(dst + 0), Y0);
  _mm_storeu_si128((__m128i *)(dst + 4), Y1);
  _mm_storeu_si128((__m128i *)(dst + 8), Y2);
}

/* =========================================================
 * W32^(1*j), j = 0..7
 * ========================================================= */

static const int16_t W32_1_RE_RE_LO[8] __attribute__((aligned(16))) = {DFT32_Q15_ONE,
                                                                       DFT32_Q15_ONE,
                                                                       DFT32_Q15_COS_PI_16,
                                                                       DFT32_Q15_COS_PI_16,
                                                                       Q15_COS_PI_8,
                                                                       Q15_COS_PI_8,
                                                                       DFT32_Q15_COS_3PI_16,
                                                                       DFT32_Q15_COS_3PI_16};

static const int16_t W32_1_IM_SIGNED_LO[8] __attribute__((aligned(16))) =
    {0, 0, DFT32_Q15_SIN_PI_16, -DFT32_Q15_SIN_PI_16, Q15_SIN_PI_8, -Q15_SIN_PI_8, DFT32_Q15_SIN_3PI_16, -DFT32_Q15_SIN_3PI_16};

static const int16_t W32_1_RE_RE_HI[8] __attribute__((aligned(16))) = {Q15_INV_SQRT2,
                                                                       Q15_INV_SQRT2,
                                                                       DFT32_Q15_SIN_3PI_16,
                                                                       DFT32_Q15_SIN_3PI_16,
                                                                       Q15_SIN_PI_8,
                                                                       Q15_SIN_PI_8,
                                                                       DFT32_Q15_SIN_PI_16,
                                                                       DFT32_Q15_SIN_PI_16};

static const int16_t W32_1_IM_SIGNED_HI[8] __attribute__((aligned(16))) = {Q15_INV_SQRT2,
                                                                           -Q15_INV_SQRT2,
                                                                           DFT32_Q15_COS_3PI_16,
                                                                           -DFT32_Q15_COS_3PI_16,
                                                                           Q15_COS_PI_8,
                                                                           -Q15_COS_PI_8,
                                                                           DFT32_Q15_COS_PI_16,
                                                                           -DFT32_Q15_COS_PI_16};

/* =========================================================
 * W32^(2*j), j = 0..7
 * ========================================================= */

static const int16_t W32_2_RE_RE_LO[8] __attribute__((aligned(
    16))) = {DFT32_Q15_ONE, DFT32_Q15_ONE, Q15_COS_PI_8, Q15_COS_PI_8, Q15_INV_SQRT2, Q15_INV_SQRT2, Q15_SIN_PI_8, Q15_SIN_PI_8};

static const int16_t W32_2_IM_SIGNED_LO[8]
    __attribute__((aligned(16))) = {0, 0, Q15_SIN_PI_8, -Q15_SIN_PI_8, Q15_INV_SQRT2, -Q15_INV_SQRT2, Q15_COS_PI_8, -Q15_COS_PI_8};

static const int16_t W32_2_RE_RE_HI[8] __attribute__((
    aligned(16))) = {0, 0, -Q15_SIN_PI_8, -Q15_SIN_PI_8, -Q15_INV_SQRT2, -Q15_INV_SQRT2, -Q15_COS_PI_8, -Q15_COS_PI_8};

static const int16_t W32_2_IM_SIGNED_HI[8] __attribute__((aligned(16))) =
    {DFT32_Q15_ONE, -DFT32_Q15_ONE, Q15_COS_PI_8, -Q15_COS_PI_8, Q15_INV_SQRT2, -Q15_INV_SQRT2, Q15_SIN_PI_8, -Q15_SIN_PI_8};

/* =========================================================
 * W32^(3*j), j = 0..7
 * ========================================================= */

static const int16_t W32_3_RE_RE_LO[8] __attribute__((aligned(16))) = {DFT32_Q15_ONE,
                                                                       DFT32_Q15_ONE,
                                                                       DFT32_Q15_COS_3PI_16,
                                                                       DFT32_Q15_COS_3PI_16,
                                                                       Q15_SIN_PI_8,
                                                                       Q15_SIN_PI_8,
                                                                       -DFT32_Q15_SIN_PI_16,
                                                                       -DFT32_Q15_SIN_PI_16};

static const int16_t W32_3_IM_SIGNED_LO[8] __attribute__((aligned(16))) =
    {0, 0, DFT32_Q15_SIN_3PI_16, -DFT32_Q15_SIN_3PI_16, Q15_COS_PI_8, -Q15_COS_PI_8, DFT32_Q15_COS_PI_16, -DFT32_Q15_COS_PI_16};

static const int16_t W32_3_RE_RE_HI[8] __attribute__((aligned(16))) = {-Q15_INV_SQRT2,
                                                                       -Q15_INV_SQRT2,
                                                                       -DFT32_Q15_COS_PI_16,
                                                                       -DFT32_Q15_COS_PI_16,
                                                                       -Q15_COS_PI_8,
                                                                       -Q15_COS_PI_8,
                                                                       -DFT32_Q15_SIN_3PI_16,
                                                                       -DFT32_Q15_SIN_3PI_16};

static const int16_t W32_3_IM_SIGNED_HI[8] __attribute__((aligned(16))) = {Q15_INV_SQRT2,
                                                                           -Q15_INV_SQRT2,
                                                                           DFT32_Q15_SIN_PI_16,
                                                                           -DFT32_Q15_SIN_PI_16,
                                                                           -Q15_SIN_PI_8,
                                                                           Q15_SIN_PI_8,
                                                                           -DFT32_Q15_COS_3PI_16,
                                                                           DFT32_Q15_COS_3PI_16};

static inline void dft8x4_q15_128(const __m128i x0,
                                  const __m128i x1,
                                  const __m128i x2,
                                  const __m128i x3,
                                  const __m128i x4,
                                  const __m128i x5,
                                  const __m128i x6,
                                  const __m128i x7,
                                  __m128i *Y0,
                                  __m128i *Y1,
                                  __m128i *Y2,
                                  __m128i *Y3,
                                  __m128i *Y4,
                                  __m128i *Y5,
                                  __m128i *Y6,
                                  __m128i *Y7,
                                  dft_dir_t dir)
{
  __m128i E0, E1, E2, E3;
  __m128i O0, O1, O2, O3;

  dft4x4_q15_128(x0, x2, x4, x6, &E0, &E1, &E2, &E3, dir);
  dft4x4_q15_128(x1, x3, x5, x7, &O0, &O1, &O2, &O3, dir);

  const __m128i E0s = q15_mul_i16_128(E0, Q15_INV_SQRT2);
  const __m128i E1s = q15_mul_i16_128(E1, Q15_INV_SQRT2);
  const __m128i E2s = q15_mul_i16_128(E2, Q15_INV_SQRT2);
  const __m128i E3s = q15_mul_i16_128(E3, Q15_INV_SQRT2);

  /*
   * W8 scaled by 1/sqrt(2):
   *
   * W8^0 / sqrt(2) =  1/sqrt(2)
   * W8^1 / sqrt(2) =  1/2 - j1/2
   * W8^2 / sqrt(2) =  0 - j1/sqrt(2)
   * W8^3 / sqrt(2) = -1/2 - j1/2
   */
  const __m128i T0 = q15_mul_i16_128(O0, Q15_INV_SQRT2);

  const __m128i T1 = complex_mul4_bcast_q15_128(O1, Q15_HALF, twiddle_im_scalar_dir_i16(-Q15_HALF, dir));

  const __m128i T2 = complex_mul4_bcast_q15_128(O2, 0, twiddle_im_scalar_dir_i16(-Q15_INV_SQRT2, dir));

  const __m128i T3 = complex_mul4_bcast_q15_128(O3, -Q15_HALF, twiddle_im_scalar_dir_i16(-Q15_HALF, dir));

  *Y0 = _mm_adds_epi16(E0s, T0);
  *Y4 = _mm_subs_epi16(E0s, T0);

  *Y1 = _mm_adds_epi16(E1s, T1);
  *Y5 = _mm_subs_epi16(E1s, T1);

  *Y2 = _mm_adds_epi16(E2s, T2);
  *Y6 = _mm_subs_epi16(E2s, T2);

  *Y3 = _mm_adds_epi16(E3s, T3);
  *Y7 = _mm_subs_epi16(E3s, T3);
}

static inline void combine32_q15_128(const __m128i H_lo[4], const __m128i H_hi[4], c16_t *dst, dft_dir_t dir)
{
  const __m128i W1_RE_LO = _mm_load_si128((const __m128i *)W32_1_RE_RE_LO);
  const __m128i W1_IM_LO = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W32_1_IM_SIGNED_LO), dir);

  const __m128i W1_RE_HI = _mm_load_si128((const __m128i *)W32_1_RE_RE_HI);
  const __m128i W1_IM_HI = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W32_1_IM_SIGNED_HI), dir);

  const __m128i W2_RE_LO = _mm_load_si128((const __m128i *)W32_2_RE_RE_LO);
  const __m128i W2_IM_LO = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W32_2_IM_SIGNED_LO), dir);

  const __m128i W2_RE_HI = _mm_load_si128((const __m128i *)W32_2_RE_RE_HI);
  const __m128i W2_IM_HI = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W32_2_IM_SIGNED_HI), dir);

  const __m128i W3_RE_LO = _mm_load_si128((const __m128i *)W32_3_RE_RE_LO);
  const __m128i W3_IM_LO = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W32_3_IM_SIGNED_LO), dir);

  const __m128i W3_RE_HI = _mm_load_si128((const __m128i *)W32_3_RE_RE_HI);
  const __m128i W3_IM_HI = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W32_3_IM_SIGNED_HI), dir);

  const __m128i A0_lo = H_lo[0];
  const __m128i A0_hi = H_hi[0];

  const __m128i A1_lo = complex_mul4_prepack_q15_128(H_lo[1], W1_RE_LO, W1_IM_LO);
  const __m128i A1_hi = complex_mul4_prepack_q15_128(H_hi[1], W1_RE_HI, W1_IM_HI);

  const __m128i A2_lo = complex_mul4_prepack_q15_128(H_lo[2], W2_RE_LO, W2_IM_LO);
  const __m128i A2_hi = complex_mul4_prepack_q15_128(H_hi[2], W2_RE_HI, W2_IM_HI);

  const __m128i A3_lo = complex_mul4_prepack_q15_128(H_lo[3], W3_RE_LO, W3_IM_LO);
  const __m128i A3_hi = complex_mul4_prepack_q15_128(H_hi[3], W3_RE_HI, W3_IM_HI);

  __m128i lo0 = A0_lo;
  __m128i lo1 = A1_lo;
  __m128i lo2 = A2_lo;
  __m128i lo3 = A3_lo;

  __m128i hi0 = A0_hi;
  __m128i hi1 = A1_hi;
  __m128i hi2 = A2_hi;
  __m128i hi3 = A3_hi;

  transpose4_complex_i16_128(&lo0, &lo1, &lo2, &lo3);
  transpose4_complex_i16_128(&hi0, &hi1, &hi2, &hi3);

  __m128i Y0, Y1, Y2, Y3;
  __m128i Y4, Y5, Y6, Y7;

  dft8x4_q15_128(lo0, lo1, lo2, lo3, hi0, hi1, hi2, hi3, &Y0, &Y1, &Y2, &Y3, &Y4, &Y5, &Y6, &Y7, dir);

  _mm_storeu_si128((__m128i *)(dst + 0), Y0);
  _mm_storeu_si128((__m128i *)(dst + 4), Y1);
  _mm_storeu_si128((__m128i *)(dst + 8), Y2);
  _mm_storeu_si128((__m128i *)(dst + 12), Y3);

  _mm_storeu_si128((__m128i *)(dst + 16), Y4);
  _mm_storeu_si128((__m128i *)(dst + 20), Y5);
  _mm_storeu_si128((__m128i *)(dst + 24), Y6);
  _mm_storeu_si128((__m128i *)(dst + 28), Y7);
}

static inline void dft32_q15_128(const c16_t *src, c16_t *dst, dft_dir_t dir)
{
  __m128i H_lo[4] __attribute__((aligned(16)));
  __m128i H_hi[4] __attribute__((aligned(16)));

  const __m128i x0_lo = _mm_loadu_si128((const __m128i *)(src + 0));
  const __m128i x0_hi = _mm_loadu_si128((const __m128i *)(src + 4));

  const __m128i x1_lo = _mm_loadu_si128((const __m128i *)(src + 8));
  const __m128i x1_hi = _mm_loadu_si128((const __m128i *)(src + 12));

  const __m128i x2_lo = _mm_loadu_si128((const __m128i *)(src + 16));
  const __m128i x2_hi = _mm_loadu_si128((const __m128i *)(src + 20));

  const __m128i x3_lo = _mm_loadu_si128((const __m128i *)(src + 24));
  const __m128i x3_hi = _mm_loadu_si128((const __m128i *)(src + 28));

  dft4x4_q15_128(x0_lo, x1_lo, x2_lo, x3_lo, &H_lo[0], &H_lo[1], &H_lo[2], &H_lo[3], dir);

  dft4x4_q15_128(x0_hi, x1_hi, x2_hi, x3_hi, &H_hi[0], &H_hi[1], &H_hi[2], &H_hi[3], dir);

  combine32_q15_128(H_lo, H_hi, dst, dir);
}

static inline void dft32_q15_128_strided(const c16_t *src, int stride, c16_t *dst, dft_dir_t dir)
{
  __m128i H_lo[4] __attribute__((aligned(16)));
  __m128i H_hi[4] __attribute__((aligned(16)));

  const __m128i x0_lo = load4_complex_strided_c16(src, stride, 0);
  const __m128i x0_hi = load4_complex_strided_c16(src, stride, 4);

  const __m128i x1_lo = load4_complex_strided_c16(src, stride, 8);
  const __m128i x1_hi = load4_complex_strided_c16(src, stride, 12);

  const __m128i x2_lo = load4_complex_strided_c16(src, stride, 16);
  const __m128i x2_hi = load4_complex_strided_c16(src, stride, 20);

  const __m128i x3_lo = load4_complex_strided_c16(src, stride, 24);
  const __m128i x3_hi = load4_complex_strided_c16(src, stride, 28);

  dft4x4_q15_128(x0_lo, x1_lo, x2_lo, x3_lo, &H_lo[0], &H_lo[1], &H_lo[2], &H_lo[3], dir);

  dft4x4_q15_128(x0_hi, x1_hi, x2_hi, x3_hi, &H_hi[0], &H_hi[1], &H_hi[2], &H_hi[3], dir);

  combine32_q15_128(H_lo, H_hi, dst, dir);
}

/*
 * W24^k, k = 0..3, scaled by 1/sqrt(3)
 */
static const int16_t W24_R3_W1_RE_RE_LO[8] __attribute__((aligned(16))) = {
    18919,
    18919, /* cos(0)      / sqrt3 */
    18274,
    18274, /* cos(pi/12)  / sqrt3 */
    16384,
    16384, /* cos(pi/6)   / sqrt3 */
    13377,
    13377 /* cos(pi/4)   / sqrt3 */
};

static const int16_t W24_R3_W1_IM_SIGNED_LO[8] __attribute__((aligned(16))) = {0, 0, 4896, -4896, 9459, -9459, 13377, -13377};

/*
 * W24^k, k = 4..7, scaled by 1/sqrt(3)
 */
static const int16_t W24_R3_W1_RE_RE_HI[8] __attribute__((aligned(16))) = {
    9459,
    9459, /* cos(pi/3)    / sqrt3 */
    4896,
    4896, /* cos(5pi/12)  / sqrt3 */
    0,
    0, /* cos(pi/2)    / sqrt3 */
    -4896,
    -4896 /* cos(7pi/12)  / sqrt3 */
};

static const int16_t W24_R3_W1_IM_SIGNED_HI[8]
    __attribute__((aligned(16))) = {16384, -16384, 18274, -18274, 18919, -18919, 18274, -18274};

/*
 * W24^(2k), k = 0..3, scaled by 1/sqrt(3)
 */
static const int16_t W24_R3_W2_RE_RE_LO[8] __attribute__((aligned(16))) = {
    18919,
    18919, /* k=0 : W24^0 */
    16384,
    16384, /* k=1 : W24^2  */
    9459,
    9459, /* k=2 : W24^4  */
    0,
    0 /* k=3 : W24^6  */
};

static const int16_t W24_R3_W2_IM_SIGNED_LO[8] __attribute__((aligned(16))) = {0, 0, 9459, -9459, 16384, -16384, 18919, -18919};

/*
 * W24^(2k), k = 4..7, scaled by 1/sqrt(3)
 */
static const int16_t W24_R3_W2_RE_RE_HI[8] __attribute__((aligned(16))) = {
    -9459,
    -9459, /* k=4 : W24^8  */
    -16384,
    -16384, /* k=5 : W24^10 */
    -18919,
    -18919, /* k=6 : W24^12 */
    -16384,
    -16384 /* k=7 : W24^14 */
};

static const int16_t W24_R3_W2_IM_SIGNED_HI[8] __attribute__((aligned(16))) = {16384, -16384, 9459, -9459, 0, 0, -9459, 9459};

static inline void radix3_combine4_q15_128_scaled(__m128i A,
                                                  __m128i X1,
                                                  __m128i X2,
                                                  __m128i w1_re,
                                                  __m128i w1_im,
                                                  __m128i w2_re,
                                                  __m128i w2_im,
                                                  __m128i *Y0,
                                                  __m128i *Y1,
                                                  __m128i *Y2,
                                                  dft_dir_t dir)
{
  const __m128i As = q15_mul_i16_128(A, Q15_INV_SQRT3);

  /*
   * B[k] = W24^k    * X1[k] / sqrt(3)
   * C[k] = W24^(2k) * X2[k] / sqrt(3)
   */
  const __m128i B = complex_mul4_prepack_q15_128(X1, w1_re, w1_im);

  const __m128i C = complex_mul4_prepack_q15_128(X2, w2_re, w2_im);

  const __m128i S = _mm_adds_epi16(B, C);
  const __m128i D = _mm_subs_epi16(B, C);

  /*
   * Y0 = A + B + C
   */
  *Y0 = _mm_adds_epi16(As, S);

  /*
   * base = A - 1/2 * (B + C)
   */
  const __m128i halfS = q15_mul_i16_128(S, Q15_HALF);

  const __m128i base = _mm_subs_epi16(As, halfS);

  /*
   * c3D = sqrt(3)/2 * (B - C)
   */
  const __m128i c3D = q15_mul_i16_128(D, Q15_SQRT3_OVER_2);

  /*
   * Forward radix-3:
   *
   * Y1 = base - j*c3D
   * Y2 = base + j*c3D
   */
  *Y1 = _mm_adds_epi16(base, mul_minus_j_dir_i16_128(c3D, dir));
  *Y2 = _mm_adds_epi16(base, mul_plus_j_dir_i16_128(c3D, dir));
}

static inline void dft24_q15_128(const c16_t *src, c16_t *dst, dft_dir_t dir)
{
  /*
   * Radix-3 split.
   *
   * Lane 0 : src[0], src[3], src[6],  ..., src[21]
   * Lane 1 : src[1], src[4], src[7],  ..., src[22]
   * Lane 2 : src[2], src[5], src[8],  ..., src[23]
   * Lane 3 : dummy
   */

  const __m128i x0 = pack3_complex_plus_zero_c16(src[0], src[1], src[2]);
  const __m128i x1 = pack3_complex_plus_zero_c16(src[3], src[4], src[5]);
  const __m128i x2 = pack3_complex_plus_zero_c16(src[6], src[7], src[8]);
  const __m128i x3 = pack3_complex_plus_zero_c16(src[9], src[10], src[11]);

  const __m128i x4 = pack3_complex_plus_zero_c16(src[12], src[13], src[14]);
  const __m128i x5 = pack3_complex_plus_zero_c16(src[15], src[16], src[17]);
  const __m128i x6 = pack3_complex_plus_zero_c16(src[18], src[19], src[20]);
  const __m128i x7 = pack3_complex_plus_zero_c16(src[21], src[22], src[23]);

  __m128i H0, H1, H2, H3;
  __m128i H4, H5, H6, H7;

  /*
   * H0 = [F0[0], F1[0], F2[0], dummy]
   * H1 = [F0[1], F1[1], F2[1], dummy]
   * ...
   * H7 = [F0[7], F1[7], F2[7], dummy]

   */
  dft8x4_q15_128(x0, x1, x2, x3, x4, x5, x6, x7, &H0, &H1, &H2, &H3, &H4, &H5, &H6, &H7, dir);

  /*
   * Transpose k=0..3 :
   *
   * A_lo  = [F0[0], F0[1], F0[2], F0[3]]
   * X1_lo = [F1[0], F1[1], F1[2], F1[3]]
   * X2_lo = [F2[0], F2[1], F2[2], F2[3]]
   */
  transpose4_complex_i16_128(&H0, &H1, &H2, &H3);

  const __m128i A_lo = H0;
  const __m128i X1_lo = H1;
  const __m128i X2_lo = H2;

  /*
   * Transpose k=4..7 :
   *
   * A_hi  = [F0[4], F0[5], F0[6], F0[7]]
   * X1_hi = [F1[4], F1[5], F1[6], F1[7]]
   * X2_hi = [F2[4], F2[5], F2[6], F2[7]]
   */
  transpose4_complex_i16_128(&H4, &H5, &H6, &H7);

  const __m128i A_hi = H4;
  const __m128i X1_hi = H5;
  const __m128i X2_hi = H6;

  const __m128i W1_RE_LO = _mm_load_si128((const __m128i *)W24_R3_W1_RE_RE_LO);
  const __m128i W1_IM_LO = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W24_R3_W1_IM_SIGNED_LO), dir);

  const __m128i W2_RE_LO = _mm_load_si128((const __m128i *)W24_R3_W2_RE_RE_LO);
  const __m128i W2_IM_LO = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W24_R3_W2_IM_SIGNED_LO), dir);

  const __m128i W1_RE_HI = _mm_load_si128((const __m128i *)W24_R3_W1_RE_RE_HI);
  const __m128i W1_IM_HI = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W24_R3_W1_IM_SIGNED_HI), dir);

  const __m128i W2_RE_HI = _mm_load_si128((const __m128i *)W24_R3_W2_RE_RE_HI);
  const __m128i W2_IM_HI = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W24_R3_W2_IM_SIGNED_HI), dir);

  __m128i Y0_lo, Y1_lo, Y2_lo;
  __m128i Y0_hi, Y1_hi, Y2_hi;

  radix3_combine4_q15_128_scaled(A_lo, X1_lo, X2_lo, W1_RE_LO, W1_IM_LO, W2_RE_LO, W2_IM_LO, &Y0_lo, &Y1_lo, &Y2_lo, dir);

  radix3_combine4_q15_128_scaled(A_hi, X1_hi, X2_hi, W1_RE_HI, W1_IM_HI, W2_RE_HI, W2_IM_HI, &Y0_hi, &Y1_hi, &Y2_hi, dir);

  /*
   * dst[0..7]    = Y0
   * dst[8..15]   = Y1
   * dst[16..23]  = Y2
   */
  _mm_storeu_si128((__m128i *)(dst + 0), Y0_lo);
  _mm_storeu_si128((__m128i *)(dst + 4), Y0_hi);

  _mm_storeu_si128((__m128i *)(dst + 8), Y1_lo);
  _mm_storeu_si128((__m128i *)(dst + 12), Y1_hi);

  _mm_storeu_si128((__m128i *)(dst + 16), Y2_lo);
  _mm_storeu_si128((__m128i *)(dst + 20), Y2_hi);
}

static inline void dft24_q15_128_strided(const c16_t *src, int stride, c16_t *dst, dft_dir_t dir)
{
  c16_t tmp[24] __attribute__((aligned(64)));

  for (int i = 0; i < 24; i++) {
    tmp[i] = src[i * stride];
  }

  dft24_q15_128(tmp, dst, dir);
}

/*
 * W20^(1*k), k=0..3, scaled by 1/sqrt(5)
 */
static const int16_t W20_1_RE_RE[8] __attribute__((aligned(16))) = {14654, 14654, 13937, 13937, 11855, 11855, 8613, 8613};

static const int16_t W20_1_IM_SIGNED[8] __attribute__((aligned(16))) = {0, 0, 4528, -4528, 8613, -8613, 11855, -11855};

/*
 * W20^(2*k), k=0..3, scaled by 1/sqrt(5)
 */
static const int16_t W20_2_RE_RE[8] __attribute__((aligned(16))) = {14654, 14654, 11855, 11855, 4528, 4528, -4528, -4528};

static const int16_t W20_2_IM_SIGNED[8] __attribute__((aligned(16))) = {0, 0, 8613, -8613, 13937, -13937, 13937, -13937};

/*
 * W20^(3*k), k=0..3, scaled by 1/sqrt(5)
 */
static const int16_t W20_3_RE_RE[8] __attribute__((aligned(16))) = {14654, 14654, 8613, 8613, -4528, -4528, -13937, -13937};

static const int16_t W20_3_IM_SIGNED[8] __attribute__((aligned(16))) = {0, 0, 11855, -11855, 13937, -13937, 4528, -4528};

/*
 * W20^(4*k), k=0..3, scaled by 1/sqrt(5)
 */
static const int16_t W20_4_RE_RE[8] __attribute__((aligned(16))) = {14654, 14654, 4528, 4528, -11855, -11855, -11855, -11855};

static const int16_t W20_4_IM_SIGNED[8] __attribute__((aligned(16))) = {0, 0, 13937, -13937, 8613, -8613, -8613, 8613};

static inline void radix5_combine4_q15_128_dft20(__m128i A,
                                                 __m128i X1,
                                                 __m128i X2,
                                                 __m128i X3,
                                                 __m128i X4,
                                                 __m128i w1_re,
                                                 __m128i w1_im,
                                                 __m128i w2_re,
                                                 __m128i w2_im,
                                                 __m128i w3_re,
                                                 __m128i w3_im,
                                                 __m128i w4_re,
                                                 __m128i w4_im,
                                                 __m128i *Y0,
                                                 __m128i *Y1,
                                                 __m128i *Y2,
                                                 __m128i *Y3,
                                                 __m128i *Y4,
                                                 dft_dir_t dir)
{
  /*
   * Twiddles W20 are already scaled by 1/sqrt(5).
   */
  const __m128i B = complex_mul4_prepack_q15_128(X1, w1_re, w1_im);

  const __m128i C = complex_mul4_prepack_q15_128(X2, w2_re, w2_im);

  const __m128i D = complex_mul4_prepack_q15_128(X3, w3_re, w3_im);

  const __m128i E = complex_mul4_prepack_q15_128(X4, w4_re, w4_im);

  const __m128i BE = _mm_adds_epi16(B, E);
  const __m128i BEminus = _mm_subs_epi16(B, E);

  const __m128i CD = _mm_adds_epi16(C, D);
  const __m128i CDminus = _mm_subs_epi16(C, D);

  /*
   * A also needs radix-5 scaling.
   */
  const __m128i As = q15_mul_i16_128(A, Q15_INV_SQRT5);

  /*
   * Y0 = A + B + C + D + E
   */
  *Y0 = _mm_adds_epi16(As, _mm_adds_epi16(BE, CD));

  /*
   * Y1 / Y4
   */
  const __m128i base1 = _mm_adds_epi16(As, _mm_adds_epi16(q15_mul_i16_128(BE, Q15_COS_2PI_5), q15_mul_i16_128(CD, Q15_COS_4PI_5)));

  const __m128i imag1 = _mm_adds_epi16(q15_mul_i16_128(BEminus, Q15_SIN_2PI_5), q15_mul_i16_128(CDminus, Q15_SIN_4PI_5));

  *Y1 = _mm_adds_epi16(base1, mul_minus_j_dir_i16_128(imag1, dir));
  *Y4 = _mm_adds_epi16(base1, mul_plus_j_dir_i16_128(imag1, dir));

  /*
   * Y2 / Y3
   */
  const __m128i base2 = _mm_adds_epi16(As, _mm_adds_epi16(q15_mul_i16_128(BE, Q15_COS_4PI_5), q15_mul_i16_128(CD, Q15_COS_2PI_5)));

  const __m128i imag2 = _mm_subs_epi16(q15_mul_i16_128(BEminus, Q15_SIN_4PI_5), q15_mul_i16_128(CDminus, Q15_SIN_2PI_5));

  *Y2 = _mm_adds_epi16(base2, mul_minus_j_dir_i16_128(imag2, dir));
  *Y3 = _mm_adds_epi16(base2, mul_plus_j_dir_i16_128(imag2, dir));
}
static inline void dft20_q15_128(const c16_t *src, c16_t *dst, dft_dir_t dir)
{
  /*
   * Branches:
   *
   * r=0 : src[0],  src[5],  src[10], src[15]
   * r=1 : src[1],  src[6],  src[11], src[16]
   * r=2 : src[2],  src[7],  src[12], src[17]
   * r=3 : src[3],  src[8],  src[13], src[18]
   * r=4 : src[4],  src[9],  src[14], src[19]
   *
   * First dft4x4 computes r=0..3 in parallel.
   * Second dft4x4 computes r=4 in lane0 only.
   */

  const __m128i x0 = load4_complex_strided_c16(src, 1, 0);

  const __m128i x1 = load4_complex_strided_c16(src, 1, 5);

  const __m128i x2 = load4_complex_strided_c16(src, 1, 10);

  const __m128i x3 = load4_complex_strided_c16(src, 1, 15);

  __m128i H0, H1, H2, H3;

  /*
   * H0 = [F0[0], F1[0], F2[0], F3[0]]
   * H1 = [F0[1], F1[1], F2[1], F3[1]]
   * H2 = [F0[2], F1[2], F2[2], F3[2]]
   * H3 = [F0[3], F1[3], F2[3], F3[3]]
   */
  dft4x4_q15_128(x0, x1, x2, x3, &H0, &H1, &H2, &H3, dir);

  /*
   * Transpose:
   *
   * H0 = A  = [F0[0], F0[1], F0[2], F0[3]]
   * H1 = X1 = [F1[0], F1[1], F1[2], F1[3]]
   * H2 = X2 = [F2[0], F2[1], F2[2], F2[3]]
   * H3 = X3 = [F3[0], F3[1], F3[2], F3[3]]
   */
  transpose4_complex_i16_128(&H0, &H1, &H2, &H3);

  const __m128i A = H0;
  const __m128i X1 = H1;
  const __m128i X2 = H2;
  const __m128i X3 = H3;

  /*
   * Branch r=4.
   * Only lane0 is useful.
   */
  const __m128i z0 = pack1_complex_lane0_c16(src[4]);

  const __m128i z1 = pack1_complex_lane0_c16(src[9]);

  const __m128i z2 = pack1_complex_lane0_c16(src[14]);

  const __m128i z3 = pack1_complex_lane0_c16(src[19]);

  __m128i G0, G1, G2, G3;

  dft4x4_q15_128(z0, z1, z2, z3, &G0, &G1, &G2, &G3, dir);

  /*
   * X4 = [F4[0], F4[1], F4[2], F4[3]]
   */
  const __m128i g01 = _mm_unpacklo_epi32(G0, G1);
  const __m128i g23 = _mm_unpacklo_epi32(G2, G3);
  const __m128i X4 = _mm_unpacklo_epi64(g01, g23);

  const __m128i W1_RE = _mm_load_si128((const __m128i *)W20_1_RE_RE);
  const __m128i W1_IM = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W20_1_IM_SIGNED), dir);

  const __m128i W2_RE = _mm_load_si128((const __m128i *)W20_2_RE_RE);
  const __m128i W2_IM = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W20_2_IM_SIGNED), dir);

  const __m128i W3_RE = _mm_load_si128((const __m128i *)W20_3_RE_RE);
  const __m128i W3_IM = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W20_3_IM_SIGNED), dir);

  const __m128i W4_RE = _mm_load_si128((const __m128i *)W20_4_RE_RE);
  const __m128i W4_IM = twiddle_im_dir_128(_mm_load_si128((const __m128i *)W20_4_IM_SIGNED), dir);

  __m128i Y0, Y1, Y2, Y3, Y4;

  radix5_combine4_q15_128_dft20(A,
                                X1,
                                X2,
                                X3,
                                X4,
                                W1_RE,
                                W1_IM,
                                W2_RE,
                                W2_IM,
                                W3_RE,
                                W3_IM,
                                W4_RE,
                                W4_IM,
                                &Y0,
                                &Y1,
                                &Y2,
                                &Y3,
                                &Y4,
                                dir);

  /*
   * dst[0..3]    = Y0
   * dst[4..7]    = Y1
   * dst[8..11]   = Y2
   * dst[12..15]  = Y3
   * dst[16..19]  = Y4
   */
  _mm_storeu_si128((__m128i *)(dst + 0), Y0);
  _mm_storeu_si128((__m128i *)(dst + 4), Y1);
  _mm_storeu_si128((__m128i *)(dst + 8), Y2);
  _mm_storeu_si128((__m128i *)(dst + 12), Y3);
  _mm_storeu_si128((__m128i *)(dst + 16), Y4);
}

static inline void dft20_q15_128_strided(const c16_t *src, int stride, c16_t *dst, dft_dir_t dir)
{
  c16_t tmp[20] __attribute__((aligned(64)));

  for (int i = 0; i < 20; i++) {
    tmp[i] = src[i * stride];
  }

  dft20_q15_128(tmp, dst, dir);
}

static inline void dft16_q15_128_from_regs(const __m128i x0,
                                           const __m128i x1,
                                           const __m128i x2,
                                           const __m128i x3,
                                           c16_t *dst,
                                           dft_dir_t dir)
{
  __m128i H[4] __attribute__((aligned(16)));

  dft4x4_q15_128(x0, x1, x2, x3, &H[0], &H[1], &H[2], &H[3], dir);

  combine16_q15_128(H, dst, dir);
}

static inline void dft16_q15_128_strided(const c16_t *src, int stride, c16_t *dst, dft_dir_t dir)
{
  const __m128i x0 = load4_complex_strided_c16(src, stride, 0);

  const __m128i x1 = load4_complex_strided_c16(src, stride, 4);

  const __m128i x2 = load4_complex_strided_c16(src, stride, 8);

  const __m128i x3 = load4_complex_strided_c16(src, stride, 12);

  dft16_q15_128_from_regs(x0, x1, x2, x3, dst, dir);
}

//===================================================================
// RADIX_3
//===================================================================
static inline void radix3_combine4_q15_128_fast(__m128i A,
                                                __m128i X1,
                                                __m128i X2,
                                                __m128i w1_re_re,
                                                __m128i w1_im_im,
                                                __m128i w2_re_re,
                                                __m128i w2_im_im,
                                                __m128i *Y0,
                                                __m128i *Y1,
                                                __m128i *Y2,
                                                dft_dir_t dir)
{
  /*
   * B = W1 * X1
   * C = W2 * X2
   */
  const __m128i Bs = complex_mul4_prepack_q15_128(X1, w1_re_re, w1_im_im);
  const __m128i Cs = complex_mul4_prepack_q15_128(X2, w2_re_re, w2_im_im);

  /*
   * Scaling for N = 3 * size when sub-FFTs are already scaled:
   *
   * final scale = 1 / sqrt(3)
   *
   * Y0 = (A + B + C) / sqrt(3)
   */
  const __m128i As = q15_mul_i16_128(A, Q15_INV_SQRT3);
  const __m128i S = _mm_adds_epi16(Bs, Cs);
  const __m128i D = _mm_subs_epi16(Bs, Cs);
  *Y0 = _mm_adds_epi16(As, S);

  /*
   * base = A/sqrt(3) - B/(2*sqrt(3)) - C/(2*sqrt(3))
   */
  const __m128i Sh = _mm_srai_epi16(S, 1);

  const __m128i base = _mm_subs_epi16(As, Sh);

  /*
   * B and C already include the 1/sqrt(3) twiddle scaling.
   * Apply sqrt(3)/2 to their difference for the radix-3 imaginary term.
   */
  const __m128i Z = q15_mul_i16_128(D, Q15_HALF_SQRT3);

  /*
   * Y1 = base - j*Z
   * Y2 = base + j*Z
   */
  *Y1 = _mm_adds_epi16(base, (dir == DFT_DIR_FORWARD) ? mul_minus_q15_128(Z) : mul_q15_128(Z));
  *Y2 = _mm_adds_epi16(base, (dir == DFT_DIR_FORWARD) ? mul_q15_128(Z) : mul_minus_q15_128(Z));
}

static void radix_3_fft_c16_scaled_strided(const c16_t *src, int stride, c16_t *dst, int N, dft_dir_t dir)
{
  if ((N % 3) != 0) {
    LOG_E(PHY, "radix_3_fft_c16_scaled_strided: invalid N=%d\n", N);
    return;
  }

  const int size = N / 3;

  if ((size & 3) != 0) {
    LOG_E(PHY, "radix_3_fft_c16_scaled_strided: scalar tail not implemented, size=%d\n", size);
    return;
  }

  const TwiddleTable *tw = twiddle_table_get(N);

  if (!tw || !tw->r3_q15_w1_re || !tw->r3_q15_w1_im || !tw->r3_q15_w2_re || !tw->r3_q15_w2_im || !tw->r3_q15_w1_re_inv
      || !tw->r3_q15_w1_im_inv || !tw->r3_q15_w2_re_inv || !tw->r3_q15_w2_im_inv) {
    LOG_E(PHY, "radix_3_fft_c16_scaled_strided: missing radix-3 twiddles\n");
    return;
  }

  c16_t tmp_stack[STACK_MAX_N] __attribute__((aligned(64)));
  c16_t *tmp_heap = NULL;
  c16_t *tmp = NULL;

  if (N <= STACK_MAX_N) {
    tmp = tmp_stack;
  } else {
    tmp_heap = aligned_malloc64(sizeof(c16_t) * (size_t)N);
    if (!tmp_heap) {
      LOG_E(PHY, "radix_3_fft_c16_scaled_strided: allocation failed\n");
      return;
    }
    tmp = tmp_heap;
  }

  dft_mixed_radix_c16_scaled_strided(src, stride * 3, tmp + 0 * size, size, dir);

  dft_mixed_radix_c16_scaled_strided(src + 1 * stride, stride * 3, tmp + 1 * size, size, dir);

  dft_mixed_radix_c16_scaled_strided(src + 2 * stride, stride * 3, tmp + 2 * size, size, dir);

  const __m128i *w1_re_tbl = (dir == DFT_DIR_FORWARD) ? tw->r3_q15_w1_re : tw->r3_q15_w1_re_inv;

  const __m128i *w1_im_tbl = (dir == DFT_DIR_FORWARD) ? tw->r3_q15_w1_im : tw->r3_q15_w1_im_inv;

  const __m128i *w2_re_tbl = (dir == DFT_DIR_FORWARD) ? tw->r3_q15_w2_re : tw->r3_q15_w2_re_inv;

  const __m128i *w2_im_tbl = (dir == DFT_DIR_FORWARD) ? tw->r3_q15_w2_im : tw->r3_q15_w2_im_inv;

  for (int k = 0; k < size; k += 4) {
    const int b = k >> 2;

    const __m128i A = _mm_load_si128((const __m128i *)(tmp + 0 * size + k));

    const __m128i X1 = _mm_load_si128((const __m128i *)(tmp + 1 * size + k));

    const __m128i X2 = _mm_load_si128((const __m128i *)(tmp + 2 * size + k));

    __m128i Y0, Y1, Y2;

    radix3_combine4_q15_128_fast(A, X1, X2, w1_re_tbl[b], w1_im_tbl[b], w2_re_tbl[b], w2_im_tbl[b], &Y0, &Y1, &Y2, dir);

    _mm_storeu_si128((__m128i *)(dst + 0 * size + k), Y0);
    _mm_storeu_si128((__m128i *)(dst + 1 * size + k), Y1);
    _mm_storeu_si128((__m128i *)(dst + 2 * size + k), Y2);
  }

  free(tmp_heap);
}

//===================================================================
// DFT RADIX 5
//===================================================================

static inline void radix5_combine4_q15_128_fast(__m128i A,
                                                __m128i X1,
                                                __m128i X2,
                                                __m128i X3,
                                                __m128i X4,
                                                __m128i w1_re,
                                                __m128i w1_im,
                                                __m128i w2_re,
                                                __m128i w2_im,
                                                __m128i w3_re,
                                                __m128i w3_im,
                                                __m128i w4_re,
                                                __m128i w4_im,
                                                __m128i *Y0,
                                                __m128i *Y1,
                                                __m128i *Y2,
                                                __m128i *Y3,
                                                __m128i *Y4,
                                                dft_dir_t dir)
{
  const __m128i B = complex_mul4_prepack_q15_128(X1, w1_re, w1_im);
  const __m128i C = complex_mul4_prepack_q15_128(X2, w2_re, w2_im);
  const __m128i D = complex_mul4_prepack_q15_128(X3, w3_re, w3_im);
  const __m128i E = complex_mul4_prepack_q15_128(X4, w4_re, w4_im);

  const __m128i BE = _mm_adds_epi16(B, E);
  const __m128i BEminus = _mm_subs_epi16(B, E);
  const __m128i CD = _mm_adds_epi16(C, D);
  const __m128i CDminus = _mm_subs_epi16(C, D);

  /*
   * B, C, D and E already include the 1/sqrt(5) twiddle scaling.
   */
  const __m128i As = q15_mul_i16_128(A, Q15_INV_SQRT5);
  *Y0 = _mm_adds_epi16(_mm_adds_epi16(BE, CD), As);

  /*
   * base1 = As + c1*(B+E) + c2*(C+D)
   */

  const __m128i base1 = _mm_adds_epi16(_mm_adds_epi16(q15_mul_i16_128(BE, Q15_COS_2PI_5), q15_mul_i16_128(CD, Q15_COS_4PI_5)), As);

  /*
   * imag1 = s1*(B-E) + s2*(C-D)
   */
  const __m128i imag1 = _mm_adds_epi16(q15_mul_i16_128(BEminus, Q15_SIN_2PI_5), q15_mul_i16_128(CDminus, Q15_SIN_4PI_5));

  *Y1 = _mm_adds_epi16(base1, mul_minus_j_dir_i16_128(imag1, dir));
  *Y4 = _mm_adds_epi16(base1, mul_plus_j_dir_i16_128(imag1, dir));

  /*
   * base2 = As + c2*(B+E) + c1*(C+D)
   */
  const __m128i base2 = _mm_adds_epi16(_mm_adds_epi16(q15_mul_i16_128(BE, Q15_COS_4PI_5), q15_mul_i16_128(CD, Q15_COS_2PI_5)), As);

  /*
   * imag2 = s2*(B-E) - s1*(C-D)
   */

  const __m128i imag2 = _mm_subs_epi16(q15_mul_i16_128(BEminus, Q15_SIN_4PI_5), q15_mul_i16_128(CDminus, Q15_SIN_2PI_5));

  *Y2 = _mm_adds_epi16(base2, mul_minus_j_dir_i16_128(imag2, dir));
  *Y3 = _mm_adds_epi16(base2, mul_plus_j_dir_i16_128(imag2, dir));
}

#define RADIX5_STACK_MAX_N 1024

static void radix_5_fft_c16_scaled_strided(const c16_t *src, int stride, c16_t *dst, int N, dft_dir_t dir)
{
  if ((N % 5) != 0) {
    LOG_E(PHY, "radix_5_fft_c16_scaled_strided: invalid N=%d\n", N);
    return;
  }

  const int size = N / 5;

  if ((size & 3) != 0) {
    LOG_E(PHY, "radix_5_fft_c16_scaled_strided: scalar tail not implemented, size=%d\n", size);
    return;
  }

  const TwiddleTable *tw = twiddle_table_get(N);

  if (!tw || !tw->r5_q15_w1_re || !tw->r5_q15_w1_im || !tw->r5_q15_w2_re || !tw->r5_q15_w2_im || !tw->r5_q15_w3_re
      || !tw->r5_q15_w3_im || !tw->r5_q15_w4_re || !tw->r5_q15_w4_im || !tw->r5_q15_w1_re_inv || !tw->r5_q15_w1_im_inv
      || !tw->r5_q15_w2_re_inv || !tw->r5_q15_w2_im_inv || !tw->r5_q15_w3_re_inv || !tw->r5_q15_w3_im_inv || !tw->r5_q15_w4_re_inv
      || !tw->r5_q15_w4_im_inv) {
    LOG_E(PHY, "radix_5_fft_c16_scaled_strided: missing radix-5 twiddles\n");
    return;
  }

  c16_t tmp_stack[RADIX5_STACK_MAX_N] __attribute__((aligned(64)));
  c16_t *tmp_heap = NULL;
  c16_t *tmp = NULL;

  if (N <= RADIX5_STACK_MAX_N) {
    tmp = tmp_stack;
  } else {
    tmp_heap = aligned_malloc64(sizeof(c16_t) * (size_t)N);
    if (!tmp_heap) {
      LOG_E(PHY, "radix_5_fft_c16_scaled_strided: allocation failed\n");
      return;
    }
    tmp = tmp_heap;
  }

  dft_mixed_radix_c16_scaled_strided(src + 0 * stride, stride * 5, tmp + 0 * size, size, dir);

  dft_mixed_radix_c16_scaled_strided(src + 1 * stride, stride * 5, tmp + 1 * size, size, dir);

  dft_mixed_radix_c16_scaled_strided(src + 2 * stride, stride * 5, tmp + 2 * size, size, dir);

  dft_mixed_radix_c16_scaled_strided(src + 3 * stride, stride * 5, tmp + 3 * size, size, dir);

  dft_mixed_radix_c16_scaled_strided(src + 4 * stride, stride * 5, tmp + 4 * size, size, dir);

  const __m128i *w1_re_tbl = (dir == DFT_DIR_FORWARD) ? tw->r5_q15_w1_re : tw->r5_q15_w1_re_inv;
  const __m128i *w1_im_tbl = (dir == DFT_DIR_FORWARD) ? tw->r5_q15_w1_im : tw->r5_q15_w1_im_inv;
  const __m128i *w2_re_tbl = (dir == DFT_DIR_FORWARD) ? tw->r5_q15_w2_re : tw->r5_q15_w2_re_inv;
  const __m128i *w2_im_tbl = (dir == DFT_DIR_FORWARD) ? tw->r5_q15_w2_im : tw->r5_q15_w2_im_inv;
  const __m128i *w3_re_tbl = (dir == DFT_DIR_FORWARD) ? tw->r5_q15_w3_re : tw->r5_q15_w3_re_inv;
  const __m128i *w3_im_tbl = (dir == DFT_DIR_FORWARD) ? tw->r5_q15_w3_im : tw->r5_q15_w3_im_inv;
  const __m128i *w4_re_tbl = (dir == DFT_DIR_FORWARD) ? tw->r5_q15_w4_re : tw->r5_q15_w4_re_inv;
  const __m128i *w4_im_tbl = (dir == DFT_DIR_FORWARD) ? tw->r5_q15_w4_im : tw->r5_q15_w4_im_inv;

  for (int k = 0; k < size; k += 4) {
    const int b = k >> 2;

    const __m128i A = _mm_load_si128((const __m128i *)(tmp + 0 * size + k));

    const __m128i X1 = _mm_load_si128((const __m128i *)(tmp + 1 * size + k));

    const __m128i X2 = _mm_load_si128((const __m128i *)(tmp + 2 * size + k));

    const __m128i X3 = _mm_load_si128((const __m128i *)(tmp + 3 * size + k));

    const __m128i X4 = _mm_load_si128((const __m128i *)(tmp + 4 * size + k));

    __m128i Y0, Y1, Y2, Y3, Y4;

    radix5_combine4_q15_128_fast(A,
                                 X1,
                                 X2,
                                 X3,
                                 X4,
                                 w1_re_tbl[b],
                                 w1_im_tbl[b],
                                 w2_re_tbl[b],
                                 w2_im_tbl[b],
                                 w3_re_tbl[b],
                                 w3_im_tbl[b],
                                 w4_re_tbl[b],
                                 w4_im_tbl[b],
                                 &Y0,
                                 &Y1,
                                 &Y2,
                                 &Y3,
                                 &Y4,
                                 dir);

    _mm_storeu_si128((__m128i *)(dst + 0 * size + k), Y0);
    _mm_storeu_si128((__m128i *)(dst + 1 * size + k), Y1);
    _mm_storeu_si128((__m128i *)(dst + 2 * size + k), Y2);
    _mm_storeu_si128((__m128i *)(dst + 3 * size + k), Y3);
    _mm_storeu_si128((__m128i *)(dst + 4 * size + k), Y4);
  }

  free(tmp_heap);
}

//===================================================================
// DFT MIXED RADIX
//===================================================================
static void dft_mixed_radix_c16_scaled_strided(const c16_t *src, int stride, c16_t *dst, int N, dft_dir_t dir)
{
  if (N == 1) {
    dst[0] = src[0];
    return;
  }

  if (N == 4) {
    if (stride == 1)
      dft4_void(src, dst, dir);
    else
      dft4_strided_q15_128(src, stride, dst, dir);
    return;
  }

  if (N == 8) {
    dft8_strided_q15_128(src, stride, dst, dir);
    return;
  }

  if (N == 12) {
    dft12_q15_128_strided(src, stride, dst, dir);
    return;
  }

  if (N == 16) {
    dft16_q15_128_strided(src, stride, dst, dir);
    return;
  }

  if (N == 20) {
    dft20_q15_128_strided(src, stride, dst, dir);
    return;
  }

  if (N == 24) {
    dft24_q15_128_strided(src, stride, dst, dir);
    return;
  }

  if (N == 32) {
    dft32_q15_128_strided(src, stride, dst, dir);
    return;
  }

  if (N == 64) {
    dft64_q15_128_strided(src, stride, dst, dir);
    return;
  }

  if (N == 128) {
    dft128_q15_128_strided(src, stride, dst, dir);
    return;
  }

  if (N % 5 == 0) {
    radix_5_fft_c16_scaled_strided(src, stride, dst, N, dir);
    return;
  }

  if (N % 3 == 0) {
    radix_3_fft_c16_scaled_strided(src, stride, dst, N, dir);
    return;
  }

  if (is_power_of_two_int(N) && N >= 256) {
    if (stride == 1) {
      dft_split_radix_pure_simd((c16_t *)src, dst, N, dir);
    } else {
      dft_split_radix_pure_simd_strided(src, stride, dst, N, dir);
    }
    return;
  }

  AssertFatal(false, "Unsupported mixed-radix DFT factorization for N=%d\n", N);
}

static void dft_mixed_radix_c16_scaled(const c16_t *src, c16_t *dst, int N, dft_dir_t dir)
{
  if (is_power_of_two_int(N) && N >= 256) {
    dft_split_radix_pure_simd((c16_t *)src, dst, N, dir);
    return;
  }
  dft_mixed_radix_c16_scaled_strided(src, 1, dst, N, dir);
}
#define DEFINE_MIXED_TRANSFORM(NAME, N, DIR)                        \
  void NAME##N(int16_t *input, int16_t *output, uint8_t scale_flag) \
  {                                                                 \
    (void)scale_flag;                                               \
    const c16_t *src = (const c16_t *)(const void *)input;          \
    c16_t *dst = (c16_t *)(void *)output;                           \
    if ((N) == 12)                                                  \
      dft12_q15_128(src, dst, (DIR));                               \
    else if ((N) == 16)                                             \
      dft16_q15_128(src, dst, (DIR));                               \
    else if ((N) == 24)                                             \
      dft24_q15_128(src, dst, (DIR));                               \
    else if ((N) == 32)                                             \
      dft32_q15_128(src, dst, (DIR));                               \
    else if ((N) == 64)                                             \
      dft64_avx(src, dst, (DIR));                                   \
    else if ((N) == 128)                                            \
      dft128_dir(src, dst, (DIR));                                  \
    else                                                            \
      dft_mixed_radix_c16_scaled(src, dst, (N), (DIR));             \
  }

#define DEFINE_MIXED_DFT(N) DEFINE_MIXED_TRANSFORM(dft, N, DFT_DIR_FORWARD)
#define DEFINE_MIXED_IDFT(N) DEFINE_MIXED_TRANSFORM(idft, N, DFT_DIR_INVERSE)

/* Export exactly the ABI symbol set used by tools_defs.h function tables. */
FOREACH_DFTSZ(DEFINE_MIXED_DFT)
FOREACH_IDFTSZ(DEFINE_MIXED_IDFT)

void dft_implementation(uint8_t sizeidx, int16_t *input, int16_t *output, unsigned char scale_flag)
{
  AssertFatal(sizeidx < DFT_SIZE_IDXTABLESIZE, "Invalid dft size index %i\n", sizeidx);
  const int algn = (dft_ftab[sizeidx].size % 3) ? 0x1F : 0xF;
  AssertFatal(((intptr_t)output & algn) == 0, "Output buffer should be %d-byte aligned: %p\n", algn + 1, output);

  if (((intptr_t)input) & algn) {
    const size_t bytes = (size_t)dft_ftab[sizeidx].size * sizeof(c16_t);
    c16_t *tmp = aligned_malloc64(bytes);
    AssertFatal(tmp != NULL, "Failed to allocate aligned DFT input copy (%zu bytes)\n", bytes);
    memcpy(tmp, input, bytes);
    dft_ftab[sizeidx].func((int16_t *)(void *)tmp, output, scale_flag);
    free(tmp);
  } else {
    dft_ftab[sizeidx].func(input, output, scale_flag);
  }
}

void idft_implementation(uint8_t sizeidx, int16_t *input, int16_t *output, unsigned char scale_flag)
{
  AssertFatal(sizeidx < IDFT_SIZE_IDXTABLESIZE, "Invalid idft size index %i\n", sizeidx);
  const int algn = 0x1F;
  AssertFatal(((intptr_t)output & algn) == 0, "Output buffer should be %d-byte aligned: %p\n", algn + 1, output);

  if (((intptr_t)input) & algn) {
    const size_t bytes = (size_t)idft_ftab[sizeidx].size * sizeof(c16_t);
    c16_t *tmp = aligned_malloc64(bytes);
    AssertFatal(tmp != NULL, "Failed to allocate aligned IDFT input copy (%zu bytes)\n", bytes);
    memcpy(tmp, input, bytes);
    idft_ftab[sizeidx].func((int16_t *)(void *)tmp, output, scale_flag);
    free(tmp);
  } else {
    idft_ftab[sizeidx].func(input, output, scale_flag);
  }
}

#endif