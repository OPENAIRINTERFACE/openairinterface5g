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
 * \author S. Held
 * \email sebastian.held@imst.de
 * \company IMST GmbH
 * \date 2015
 * \version 0.1
*/

#ifndef SSE_INTRIN_H
#define SSE_INTRIN_H


#if defined(__x86_64) || defined(__i386__)

#ifndef __SSE2__
#  error SSE2 processor intrinsics disabled
#endif

#include <emmintrin.h>
#include <xmmintrin.h>

#ifdef __SSE3__
#  include <pmmintrin.h>
#  include <tmmintrin.h>
#endif

#ifdef __SSE4_1__
#  include <smmintrin.h>
#endif

#ifdef __AVX2__
#  include <immintrin.h>
#endif

// ------------------------------------------------
// compatibility functions if SSE3 or SSE4 is not available
// ------------------------------------------------
#if !defined(__SSE3__) || !defined(__SSE4_1__)
/*! \brief SSE vector type.
 *
 * this source code fragment is Copyright (c) 2006-2008 Advanced Micro Devices, Inc.
 * It is licensed under Apache License 2.0 (compatible to our GPL3).
 * see http://sourceforge.net/projects/sseplus
 * \author Advanced Micro Devices, Inc.
 * \date 2006-2008
 * \copyright Apache License 2.0
 */
typedef union {
  __m128  f;
  __m128d d;
  __m128i i;
  __m64       m64[ 2];
  signed char  s8[16];
} ssp_m128;
#endif


// ------------------------------------------------
// compatibility functions if SSE3 is not available
// ------------------------------------------------
#ifndef __SSE3__
#warning SSE3 instruction set not preset
#define _mm_abs_epi16(xmmx) _mm_xor_si128((xmmx),_mm_cmpgt_epi16(_mm_setzero_si128(),(xmmx)))
#define _mm_sign_epi16(xmmx,xmmy) _mm_xor_si128((xmmx),_mm_cmpgt_epi16(_mm_setzero_si128(),(xmmy)))
#define _mm_hadd_epi32(xmmx,xmmy) _mm_unpacklo_epi64(_mm_add_epi32(_mm_shuffle_epi32((xmmx),_MM_SHUFFLE(0,2,0,2)),_mm_shuffle_epi32((xmmx),_MM_SHUFFLE(1,3,1,3))),_mm_add_epi32(_mm_shuffle_epi32((xmmy),_MM_SHUFFLE(0,2,0,2)),_mm_shuffle_epi32((xmmy),_MM_SHUFFLE(1,3,1,3))))

// variant from lte_ul_channel_estimation.c and dlsch_demodulation.c and pmch.c
//#define _mm_abs_epi16(xmmx) _mm_add_epi16(_mm_xor_si128((xmmx),_mm_cmpgt_epi16(_mm_setzero_si128(),(xmmx))),_mm_srli_epi16(_mm_cmpgt_epi16(_mm_setzero_si128(),(xmmx)),15))

// variant from cdot_prod.c
//#define _mm_abs_epi16(xmmx) xmmx=_mm_xor_si128((xmmx),_mm_cmpgt_epi16(_mm_setzero_si128(),(xmmx)))

#define _mm_shuffle_epi8(xmmx,xmmy) ssp_shuffle_epi8_SSE2(xmmx,xmmy)

/*! \brief Helper function.
 *
 * this source code fragment is Copyright (c) 2006-2008 Advanced Micro Devices, Inc.
 * It is licensed under Apache License 2.0 (compatible to our GPL3).
 * see http://sourceforge.net/projects/sseplus
 * \author Advanced Micro Devices, Inc.
 * \date 2006-2008
 * \copyright Apache License 2.0
 */
static inline __m128i ssp_comge_epi8_SSE2(__m128i a, __m128i b)
{
  __m128i c;
  c = _mm_cmpgt_epi8( a, b );
  a = _mm_cmpeq_epi8( a, b );
  a = _mm_or_si128  ( a, c );
  return a;
}

/*! \brief SSE2 emulation of SSE3 _mm_shuffle_epi8().
 *
 * this source code fragment is Copyright (c) 2006-2008 Advanced Micro Devices, Inc.
 * It is licensed under Apache License 2.0 (compatible to our GPL3).
 * see http://sourceforge.net/projects/sseplus
 * \author Advanced Micro Devices, Inc.
 * \date 2006-2008
 * \copyright Apache License 2.0
 */
static inline __m128i ssp_shuffle_epi8_SSE2 (__m128i a, __m128i mask)
{
  ssp_m128 A,B, MASK, maskZero;
  A.i        = a;
  maskZero.i = ssp_comge_epi8_SSE2( mask, _mm_setzero_si128()        );
  MASK.i     = _mm_and_si128      ( mask, _mm_set1_epi8( (char)0x0F) );

  B.s8[ 0] = A.s8[ (MASK.s8[ 0]) ];
  B.s8[ 1] = A.s8[ (MASK.s8[ 1]) ];
  B.s8[ 2] = A.s8[ (MASK.s8[ 2]) ];
  B.s8[ 3] = A.s8[ (MASK.s8[ 3]) ];
  B.s8[ 4] = A.s8[ (MASK.s8[ 4]) ];
  B.s8[ 5] = A.s8[ (MASK.s8[ 5]) ];
  B.s8[ 6] = A.s8[ (MASK.s8[ 6]) ];
  B.s8[ 7] = A.s8[ (MASK.s8[ 7]) ];
  B.s8[ 8] = A.s8[ (MASK.s8[ 8]) ];
  B.s8[ 9] = A.s8[ (MASK.s8[ 9]) ];
  B.s8[10] = A.s8[ (MASK.s8[10]) ];
  B.s8[11] = A.s8[ (MASK.s8[11]) ];
  B.s8[12] = A.s8[ (MASK.s8[12]) ];
  B.s8[13] = A.s8[ (MASK.s8[13]) ];
  B.s8[14] = A.s8[ (MASK.s8[14]) ];
  B.s8[15] = A.s8[ (MASK.s8[15]) ];

  B.i = _mm_and_si128( B.i, maskZero.i );
  return B.i;
}
#endif // __SSE3__





// ------------------------------------------------
// compatibility functions if SSE4 is not available
// ------------------------------------------------
#ifndef __SSE4_1__
#warning SSE4_1 instruction set not preset

// https://gitorious.org/vc/vc/commit/ee49857ffe5b74c74bc57d501b05519443fc609a license LGPL3
#define _mm_extract_epi32(xmmx,index) _mm_cvtsi128_si32(_mm_srli_si128(xmmx, (index) * 4))

#define _mm_insert_epi8(x,y,z) ssp_insert_epi8_SSE2(x,y,z)
#define _mm_cvtepi8_epi16(x) ssp_cvtepi8_epi16_SSE2(x)
#define _mm_max_epi8(x,y) ssp_max_epi8_SSE2(x,y)
#define _mm_cvtepi16_epi32(x) ssp_cvtepi16_epi32_SSE2(x)

/*! \brief SSE2 emulation of SSE4 _mm_insert_epi8().
 *
 * this source code fragment is Copyright (c) 2006-2008 Advanced Micro Devices, Inc.
 * It is licensed under Apache License 2.0 (compatible to our GPL3).
 * see http://sourceforge.net/projects/sseplus
 * \author Advanced Micro Devices, Inc.
 * \date 2006-2008
 * \copyright Apache License 2.0
 */
static inline __m128i ssp_insert_epi8_SSE2( __m128i a, int b, const int ndx )
{
  ssp_m128 Ahi, Alo;
  b = b & 0xFF;                                           /* Convert to 8-bit integer */
  Ahi.i = _mm_unpackhi_epi8( a, _mm_setzero_si128() );    /* Ahi = a_8[8:15]  Simulate 8bit integers as 16-bit integers */
  Alo.i = _mm_unpacklo_epi8( a, _mm_setzero_si128() );    /* Alo = a_8[0:7]   Simulate 8bit integers as 16-bit integers */

  /* Insert b as a 16-bit integer to upper or lower half of a */
  switch( ndx & 0xF ) {
  case 0:
    Alo.i = _mm_insert_epi16( Alo.i, b, 0 );
    break;

  case 1:
    Alo.i = _mm_insert_epi16( Alo.i, b, 1 );
    break;

  case 2:
    Alo.i = _mm_insert_epi16( Alo.i, b, 2 );
    break;

  case 3:
    Alo.i = _mm_insert_epi16( Alo.i, b, 3 );
    break;

  case 4:
    Alo.i = _mm_insert_epi16( Alo.i, b, 4 );
    break;

  case 5:
    Alo.i = _mm_insert_epi16( Alo.i, b, 5 );
    break;

  case 6:
    Alo.i = _mm_insert_epi16( Alo.i, b, 6 );
    break;

  case 7:
    Alo.i = _mm_insert_epi16( Alo.i, b, 7 );
    break;

  case 8:
    Ahi.i = _mm_insert_epi16( Ahi.i, b, 0 );
    break;

  case 9:
    Ahi.i = _mm_insert_epi16( Ahi.i, b, 1 );
    break;

  case 10:
    Ahi.i = _mm_insert_epi16( Ahi.i, b, 2 );
    break;

  case 11:
    Ahi.i = _mm_insert_epi16( Ahi.i, b, 3 );
    break;

  case 12:
    Ahi.i = _mm_insert_epi16( Ahi.i, b, 4 );
    break;

  case 13:
    Ahi.i = _mm_insert_epi16( Ahi.i, b, 5 );
    break;

  case 14:
    Ahi.i = _mm_insert_epi16( Ahi.i, b, 6 );
    break;

  default:
    Ahi.i = _mm_insert_epi16( Ahi.i, b, 7 );
  }

  return _mm_packus_epi16( Alo.i, Ahi.i ); // Pack the 16-bit integers to 8bit again.

  ///* Another implementation, but slower: */
  //ssp_m128 A, B, mask;
  //mask.i = _mm_setzero_si128();
  //mask.s8[ ndx & 0x0F ] = (ssp_s8)0xFF;
  //B.i    = _mm_set1_epi8( (ssp_s8)b );
  //A.i    = _mm_andnot_si128( mask.i, a );
  //mask.i = _mm_and_si128( mask.i, B.i );
  //A.i = _mm_or_si128( A.i, mask.i );
  //return A.i;
}

/*! \brief SSE2 emulation of SSE4 _mm_cvtepi8_epi16().
 *
 * this source code fragment is Copyright (c) 2006-2008 Advanced Micro Devices, Inc.
 * It is licensed under Apache License 2.0 (compatible to our GPL3).
 * see http://sourceforge.net/projects/sseplus
 * \author Advanced Micro Devices, Inc.
 * \date 2006-2008
 * \copyright Apache License 2.0
 */
static inline __m128i ssp_cvtepi8_epi16_SSE2 ( __m128i a)
{
  __m128i b = _mm_setzero_si128 ();
  __m128i c = _mm_unpacklo_epi8(a, b);
  __m128i d = _mm_set1_epi16 (128);

  b = _mm_and_si128(d, c);
  d = _mm_set1_epi16(0x1FE);
  b = _mm_mullo_epi16(b, d);

  return _mm_add_epi16(c, b);
}

/*! \brief Helper function.
 *
 * this source code fragment is Copyright (c) 2006-2008 Advanced Micro Devices, Inc.
 * It is licensed under Apache License 2.0 (compatible to our GPL3).
 * see http://sourceforge.net/projects/sseplus
 * \author Advanced Micro Devices, Inc.
 * \date 2006-2008
 * \copyright Apache License 2.0
 */
static inline __m128i ssp_logical_bitwise_select_SSE2( __m128i a, __m128i b, __m128i mask )   // Bitwise (mask ? a : b)
{
  a = _mm_and_si128   ( a,    mask );                                 // clear a where mask = 0
  b = _mm_andnot_si128( mask, b    );                                 // clear b where mask = 1
  a = _mm_or_si128    ( a,    b    );                                 // a = a OR b
  return a;
}

/*! \brief SSE2 emulation of SSE4 _mm_max_epi8().
 *
 * this source code fragment is Copyright (c) 2006-2008 Advanced Micro Devices, Inc.
 * It is licensed under Apache License 2.0 (compatible to our GPL3).
 * see http://sourceforge.net/projects/sseplus
 * \author Advanced Micro Devices, Inc.
 * \date 2006-2008
 * \copyright Apache License 2.0
 */
static inline __m128i ssp_max_epi8_SSE2( __m128i a, __m128i b )
{
  __m128i mask  = _mm_cmpgt_epi8( a, b );                             // FFFFFFFF where a > b
  a = ssp_logical_bitwise_select_SSE2( a, b, mask );
  return a;
}

/*! \brief SSE2 emulation of SSE4 _mm_cvtepi16_epi32().
 *
 * this source code fragment is Copyright (c) 2006-2008 Advanced Micro Devices, Inc.
 * It is licensed under Apache License 2.0 (compatible to our GPL3).
 * see http://sourceforge.net/projects/sseplus
 * \author Advanced Micro Devices, Inc.
 * \date 2006-2008
 * \copyright Apache License 2.0
 */
static inline __m128i ssp_cvtepi16_epi32_SSE2 ( __m128i a)
{
  __m128i b = _mm_set1_epi32 (-1);         //0xFFFFFFFF
  __m128i c = _mm_unpacklo_epi16(a, b);    //FFFFa0**FFFFa1**....
  __m128i d = _mm_set1_epi32 (0x8000);     //0x8000

  b = _mm_andnot_si128(c, d);              // 0x80 for positive, 0x00 for negative
  d = _mm_slli_epi32(b, 1);                // 0x100 for positive, 0x000 for negative

  return _mm_add_epi32(c, d);
}
#endif // __SSE4_1__

#elif defined(__arm__)
#include <arm_neon.h>

#endif // x86_64 || i386

#endif // SSE_INTRIN_H

