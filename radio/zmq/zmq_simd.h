/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef ZMQ_SIMD_H
#define ZMQ_SIMD_H

#include "simde/x86/avx512.h"
#include "simde/x86/avx2.h"

#include "common/platform_types.h"
#include <limits>
#include <algorithm>
#include <cmath>

constexpr float c16_t_to_cf_t_factor = 32767.0f;

static inline void convert_samples_avx512_tx(float *msg_data, const int16_t *samples, size_t nsamps, float factor)
{
  float r_factor = 1.0f / factor;
  size_t total_elements = nsamps;
  size_t i = 0;
  float *output_ptr = msg_data;

#if defined(__AVX512F__)
  {
    simde__m512 v_factor = simde_mm512_set1_ps(r_factor);
    for (; i + 16 <= total_elements; i += 16) {
      simde__m256i v_in16 = simde_mm256_loadu_si256((const simde__m256i *)&samples[i]);
      simde__m512i v_in32 = simde_mm512_cvtepi16_epi32(v_in16);
      simde__m512 v_float = simde_mm512_cvtepi32_ps(v_in32);
      simde__m512 v_out = simde_mm512_mul_ps(v_float, v_factor);
      simde_mm512_storeu_ps(&output_ptr[i], v_out);
    }
  }
#endif

#if defined(__AVX2__)
  {
    simde__m256 v_factor = simde_mm256_set1_ps(r_factor);
    for (; i + 16 <= total_elements; i += 16) {
      simde__m256i v_in16 = simde_mm256_loadu_si256((const simde__m256i *)&samples[i]);
      simde__m128i v_in16_lo = simde_mm256_castsi256_si128(v_in16);
      simde__m128i v_in16_hi = simde_mm256_extractf128_si256(v_in16, 1);
      simde__m256i v_in32_lo = simde_mm256_cvtepi16_epi32(v_in16_lo);
      simde__m256i v_in32_hi = simde_mm256_cvtepi16_epi32(v_in16_hi);
      simde__m256 v_float_lo = simde_mm256_cvtepi32_ps(v_in32_lo);
      simde__m256 v_float_hi = simde_mm256_cvtepi32_ps(v_in32_hi);
      simde__m256 v_out_lo = simde_mm256_mul_ps(v_float_lo, v_factor);
      simde__m256 v_out_hi = simde_mm256_mul_ps(v_float_hi, v_factor);
      simde_mm256_storeu_ps(&output_ptr[i], v_out_lo);
      simde_mm256_storeu_ps(&output_ptr[i + 8], v_out_hi);
    }
  }
#endif

  // Cleanup loop for remaining elements
  for (; i < total_elements; i++) {
    output_ptr[i] = (float)samples[i] * r_factor;
  }
}

static inline void convert_samples_avx512_rx(const float *input_floats, int16_t *output_ints, size_t nsamps, float factor)
{
  size_t total_elements = nsamps;
  size_t i = 0;

#if defined(__AVX512F__)
  {
    simde__m512 v_factor = simde_mm512_set1_ps(factor);
    for (; i + 16 <= total_elements; i += 16) {
      simde__m512 v_float = simde_mm512_loadu_ps(&input_floats[i]);
      simde__m512 v_mul = simde_mm512_mul_ps(v_float, v_factor);
      simde__m512i v_int32 = simde_mm512_cvtps_epi32(v_mul);
      simde__m256i v_int16 = simde_mm512_cvtsepi32_epi16(v_int32);
      simde_mm256_storeu_si256((simde__m256i *)&output_ints[i], v_int16);
    }
  }
#endif

#if defined(__AVX2__)
  {
    simde__m256 v_factor = simde_mm256_set1_ps(factor);
    for (; i + 16 <= total_elements; i += 16) {
      simde__m256 v_float1 = simde_mm256_loadu_ps(&input_floats[i]);
      simde__m256 v_float2 = simde_mm256_loadu_ps(&input_floats[i + 8]);
      simde__m256 v_mul1 = simde_mm256_mul_ps(v_float1, v_factor);
      simde__m256 v_mul2 = simde_mm256_mul_ps(v_float2, v_factor);
      simde__m256i v_int32_1 = simde_mm256_cvtps_epi32(v_mul1);
      simde__m256i v_int32_2 = simde_mm256_cvtps_epi32(v_mul2);
      simde__m256i v_packed = simde_mm256_packs_epi32(v_int32_1, v_int32_2);
      simde__m256i v_permuted = simde_mm256_permute4x64_epi64(v_packed, 0xD8);
      simde_mm256_storeu_si256((simde__m256i *)&output_ints[i], v_permuted);
    }
  }
#endif

  // Cleanup loop for remaining elements: round to nearest and saturate, like the SIMD paths above
  for (; i < total_elements; i++) {
    const float scaled = std::clamp(input_floats[i] * factor,
                                    static_cast<float>(std::numeric_limits<int16_t>::min()),
                                    static_cast<float>(std::numeric_limits<int16_t>::max()));
    output_ints[i] = static_cast<int16_t>(std::lrintf(scaled));
  }
}

#endif
