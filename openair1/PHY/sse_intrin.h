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

/*! \file PHY/sse_intrin.h
 * \brief SSE includes and compatibility functions.
 *
 * This header collects all SSE compatibility functions. To use SSE inside a source file, include only sse_intrin.h.
 * The host CPU needs to have support for SSE2 at least. SSE3 and SSE4.1 functions are emulated if the CPU lacks support for them.
 * This will slow down the softmodem, but may be valuable if only offline signal processing is required.
 *
 * 
 * Has been changed in August 2022 to rely on SIMD Everywhere (SIMDE) from MIT
 * by bruno.mongazon-cazavet@nokia-bell-labs.com
 *
 * All AVX22 code is mapped to SIMDE which transparently relies on AVX2 HW (avx2-capable host) or SIMDE emulation
 * (non-avx2-capable host).
 * To force using SIMDE emulation on avx2-capable host use the --noavx2 flag. 
 * avx512 code is not mapped to SIMDE. It depends on --noavx512 flag.
 * If the --noavx512 is set the OAI AVX512 emulation using AVX2 is used.
 * If the --noavx512 is not set, AVX512 HW is used on avx512-capable host while OAI AVX512 emulation using AVX2
 * is used on non-avx512-capable host. 
 *
 * \author S. Held, Laurent THOMAS
 * \email sebastian.held@imst.de, laurent.thomas@open-cells.com	
 * \company IMST GmbH, Open Cells Project
 * \date 2019
 * \version 0.2
*/

#ifndef SSE_INTRIN_H
#define SSE_INTRIN_H


#if defined(__x86_64) || defined(__i386__)

/* x86 processors */

#include <simde/x86/mmx.h>
#include <simde/x86/sse.h>
#include <simde/x86/sse2.h>
#include <simde/x86/sse3.h>
#include <simde/x86/ssse3.h>
#include <simde/x86/sse4.1.h>
#include <simde/x86/sse4.2.h>
#include <simde/x86/avx2.h>
#include <simde/x86/fma.h>

#if defined(__AVX512BW__) || defined(__AVX512F__)
#include <immintrin.h>
#endif

#elif defined(__arm__) || defined(__aarch64__)

/* ARM processors */

#include <simde/arm/neon.h>

#endif // x86_64 || i386

/*
 * OAI specific
 */

#if defined(__x86_64__) || defined(__i386__)
  #define vect128 __m128i
#elif defined(__arm__) || defined(__aarch64__)
  #define vect128 int16x8_t
#endif

static const short minusConjug128[8]__attribute__((aligned(16))) = {-1,1,-1,1,-1,1,-1,1};
static inline vect128 mulByConjugate128(vect128 *a, vect128 *b, int8_t output_shift) {

#if defined(__x86_64__) || defined(__i386__)
  vect128 realPart = _mm_madd_epi16(*a,*b);
  realPart = _mm_srai_epi32(realPart,output_shift);
  vect128 imagPart = _mm_shufflelo_epi16(*b,_MM_SHUFFLE(2,3,0,1));
  imagPart = _mm_shufflehi_epi16(imagPart,_MM_SHUFFLE(2,3,0,1));
  imagPart = _mm_sign_epi16(imagPart,*(vect128 *)minusConjug128);
  imagPart = _mm_madd_epi16(imagPart,*a);
  imagPart = _mm_srai_epi32(imagPart,output_shift);
  vect128 lowPart = _mm_unpacklo_epi32(realPart,imagPart);
  vect128 highPart = _mm_unpackhi_epi32(realPart,imagPart);
  return ( _mm_packs_epi32(lowPart,highPart));
#elif defined(__arm__) || defined(__aarch64__)
  AssertFatal(false, "not developped\n");
#endif
}

#if defined(__x86_64__) || defined(__i386__)
#define displaySamples128(vect)  {\
    __m128i x=vect;                                       \
    printf("vector: %s = (%hd,%hd) (%hd,%hd) (%hd,%hd) (%hd,%hd)\n", #vect, \
           _mm_extract_epi16(x,0),                                  \
           _mm_extract_epi16(x,1),\
           _mm_extract_epi16(x,2),\
           _mm_extract_epi16(x,3),\
           _mm_extract_epi16(x,4),\
           _mm_extract_epi16(x,5),\
           _mm_extract_epi16(x,6),\
           _mm_extract_epi16(x,7));\
  }
#elif defined(__arm__) || defined(__aarch64__)
  displaySamples128(vect) {}
//TBD
#endif
#endif // SSE_INTRIN_H
