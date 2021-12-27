/*******************************************************************************
 Copyright (c) 2009-2018, Intel Corporation

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/**
 * Header file for module with CRC computation methods
 *
 * PCLMULQDQ implementation is based on work by:
 *               Erdinc Ozturk
 *               Vinodh Gopal
 *               James Guilford
 *
 * "Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction"
 * URL: http://download.intel.com/design/intarch/papers/323102.pdf
 */

#ifndef __CRC_H__
#define __CRC_H__

#include <x86intrin.h>

#include "crcext.h"
#include "types.h"
#include <immintrin.h>
#include <wmmintrin.h>

/**
 * PCLMULQDQ CRC computation context structure
 */
struct crc_pclmulqdq_ctx {
        /**
         * K1 = reminder X^128 / P(X) : 0-63
         * K2 = reminder X^192 / P(X) : 64-127
         */
        uint64_t k1;
        uint64_t k2;

        /**
         * K3 = reminder X^64 / P(X) : 0-63
         * q  = quotient X^64 / P(X) : 64-127
         */
        uint64_t k3;
        uint64_t q;

        /**
         * p   = polynomial / P(X) : 0-63
         * res = reserved : 64-127
         */
        uint64_t p;
        uint64_t res;
};

/**
 * Functions and prototypes
 */

/**
 * @brief Initializes look-up-table (LUT) for given 8 bit polynomial
 *
 * @param poly CRC polynomial
 * @param lut pointer to look-up-table to be initialized
 */
void crc8_init_lut(const uint8_t poly, uint8_t *lut);

/**
 * @brief Calculates 8 bit CRC using LUT method.
 *
 * @param crc CRC initial value
 * @param data pointer to data block to calculate CRC for
 * @param data_len size of data block
 * @param lut 256x8bits look-up-table pointer
 *
 * @return New CRC value
 */
__forceinline
uint8_t crc8_calc_lut(const uint8_t *data,
                      uint32_t data_len,
                      uint8_t crc,
                      const uint8_t *lut)
{
        if (unlikely(data == NULL || lut == NULL))
                return crc;

        while (data_len--)
                crc = lut[*data++ ^ crc];

        return crc;
}

/**
 * @brief Initializes look-up-table (LUT) for given 16 bit polynomial
 *
 * @param poly CRC polynomial
 * @param lut pointer to 256x16bits look-up-table to be initialized
 */
void crc16_init_lut(const uint16_t poly, uint16_t *lut);

/**
 * @brief Calculates 16 bit CRC using LUT method.
 *
 * @param crc CRC initial value
 * @param data pointer to data block to calculate CRC for
 * @param data_len size of data block
 * @param lut 256x16bits look-up-table pointer
 *
 * @return New CRC value
 */
__forceinline
uint16_t crc16_calc_lut(const uint8_t *data,
                        uint32_t data_len,
                        uint16_t crc,
                        const uint16_t *lut)
{
        if (unlikely(data == NULL || lut == NULL))
                return crc;

        while (data_len--)
                crc = lut[(crc >> 8) ^ *data++] ^ (crc << 8);

        return crc;
}

/**
 * @brief Initializes look-up-table (LUT) for given 32 bit polynomial
 *
 * @param poly CRC polynomial
 * @param lut pointer to 256x32bits look-up-table to be initialized
 */
void crc32_init_lut(const uint32_t poly, uint32_t *lut);

/**
 * @brief Calculates 32 bit CRC using LUT method.
 *
 * @param crc CRC initial value
 * @param data pointer to data block to calculate CRC for
 * @param data_len size of data block
 * @param lut 256x32bits look-up-table pointer
 *
 * @return New CRC value
 */
__forceinline
uint32_t crc32_calc_lut(const uint8_t *data,
                        uint32_t data_len,
                        uint32_t crc,
                        const uint32_t *lut)
{
        if (unlikely(data == NULL || lut == NULL))
                return crc;

        while (data_len--)
                crc = lut[(crc >> 24) ^ *data++] ^ (crc << 8);

        return crc;
}

/**
 * @brief Initializes look up tables for slice-By-2 method.
 *
 * @param poly CRC polynomial
 * @param slice1 slice-by-2 look-up-table 1
 * @param slice2 slice-by-2 look-up-table 2
 *
 * @return New CRC value
 */
void crc16_init_slice2(const uint16_t poly,
                       uint16_t *slice1,
                       uint16_t *slice2);

/**
 * @brief Calculates 16 bit CRC using Slice-By-2 method.
 *
 * @param crc CRC initial value
 * @param data pointer to data block to calculate CRC for
 * @param data_len size of data block
 * @param slice1 256x16bits slice look-up-table 1
 * @param slice2 256x16bits slice look-up-table 2
 *
 * @return New CRC value
 */
__forceinline
uint16_t crc16_calc_slice2(const uint8_t *data,
                           uint32_t data_len,
                           uint16_t crc,
                           const uint16_t *slice1,
                           const uint16_t *slice2)
{
        uint_fast32_t i;

        if (unlikely(data == NULL))
                return crc;

        if (unlikely(slice1 == NULL || slice2 == NULL))
                return crc;

        crc = bswap2(crc);
        for (i = (data_len & (~1)), data += (data_len & (~1)); i != 0;
             i -= sizeof(uint16_t)) {
                crc ^= (*((const uint16_t *)(data - i)));
                crc = slice2[(uint8_t)crc] ^ slice1[(uint8_t)(crc >> 8)];
        }
        crc = bswap2(crc);
        if (data_len & 1)
                crc = (crc << 8) ^ bswap2(slice1[(crc >> 8) ^ *data]);

        return crc;
}

/**
 * @brief Initializes look up tables for slice-By-4 method.
 *
 * @param poly CRC polynomial
 * @param slice1 256x32bits slice look-up-table 1
 * @param slice2 256x32bits slice look-up-table 2
 * @param slice3 256x32bits slice look-up-table 3
 * @param slice4 256x32bits slice look-up-table 4
 *
 * @return New CRC value
 */
void crc32_init_slice4(const uint32_t poly,
                       uint32_t *slice1, uint32_t *slice2,
                       uint32_t *slice3, uint32_t *slice4);

/**
 * @brief Calculates 32 bit CRC using Slice-By-4 method.
 *
 * @param data pointer to data block to calculate CRC for
 * @param data_len size of data block
 * @param crc CRC initial value
 * @param slice1 256x32bits slice look-up-table 1
 * @param slice2 256x32bits slice look-up-table 2
 * @param slice3 256x32bits slice look-up-table 3
 * @param slice4 256x32bits slice look-up-table 4
 *
 * @return New CRC value
 */
__forceinline
uint32_t crc32_calc_slice4(const uint8_t *data,
                           uint32_t data_len, uint32_t crc,
                           const uint32_t *slice1, const uint32_t *slice2,
                           const uint32_t *slice3, const uint32_t *slice4)
{
        uint_fast32_t i;

        if (unlikely(data == NULL))
                return crc;

        if (unlikely(slice1 == NULL || slice2 == NULL ||
                     slice3 == NULL || slice4 == NULL))
                return crc;

        crc = bswap4(crc);
        for (i = data_len & (~3), data += (data_len & (~3)); i != 0;
             i -= sizeof(uint32_t)) {
                crc ^= (*((const uint32_t *)(data - i)));
                crc = slice4[(uint8_t)(crc)] ^
                        slice3[(uint8_t)(crc >> 8)] ^
                        slice2[(uint8_t)(crc >> 16)] ^
                        slice1[(uint8_t)(crc >> 24)];
        }
        crc = bswap4(crc);
        for (i = data_len & 3, data += (data_len & 3); i != 0; i--)
                crc = (crc << 8) ^
                        bswap4(slice1[(crc >> 24) ^ *(data - i)]);

        return crc;
}

/**
 * @brief Performs one folding round
 *
 * Logically function operates as follows:
 *     DATA = READ_NEXT_16BYTES();
 *     F1 = LSB8(FOLD)
 *     F2 = MSB8(FOLD)
 *     T1 = CLMUL( F1, K1 )
 *     T2 = CLMUL( F2, K2 )
 *     FOLD = XOR( T1, T2, DATA )
 *
 * @param data_block 16 byte data block
 * @param k1_k2 k1 and k2 constanst enclosed in XMM register
 * @param fold running 16 byte folded data
 *
 * @return New 16 byte folded data
 */
__forceinline
__m128i crc32_folding_round(const __m128i data_block,
                            const __m128i k1_k2,
                            const __m128i fold)
{
        __m128i tmp = _mm_clmulepi64_si128(fold, k1_k2, 0x11);

        return _mm_xor_si128(_mm_clmulepi64_si128(fold, k1_k2, 0x00),
                             _mm_xor_si128(data_block, tmp));
}

/**
 * @brief Performs Barret's reduction from 128 bits to 64 bits
 *
 * @param data128 128 bits data to be reduced
 * @param k3_q k3 and Q constants enclosed in XMM register
 *
 * @return data reduced to 64 bits
 */
__forceinline
__m128i crc32_reduce_128_to_64(__m128i data128, const __m128i k3_q)
{
        __m128i tmp;

        tmp = _mm_xor_si128(_mm_clmulepi64_si128(data128, k3_q, 0x01 /* k3 */),
                            data128);

        data128 = _mm_xor_si128(_mm_clmulepi64_si128(tmp, k3_q, 0x01 /* k3 */),
                                data128);

        return _mm_srli_si128(_mm_slli_si128(data128, 8), 8);
}

/**
 * @brief Performs Barret's reduction from 64 bits to 32 bits
 *
 * @param data64 64 bits data to be reduced
 * @param k3_q k3 and Q constants enclosed in XMM register
 * @param p_res P constant enclosed in XMM register
 *
 * @return data reduced to 32 bits
 */
__forceinline
uint32_t
crc32_reduce_64_to_32(__m128i fold, const __m128i k3_q, const __m128i p_res)
{
        __m128i temp;

        temp = _mm_clmulepi64_si128(_mm_srli_si128(fold, 4),
                                    k3_q, 0x10 /* Q */);
        temp = _mm_srli_si128(_mm_xor_si128(temp, fold), 4);
        temp = _mm_clmulepi64_si128(temp, p_res, 0 /* P */);
        return _mm_extract_epi32(_mm_xor_si128(temp, fold), 0);
}

/**
 * @brief Calculates 32 bit CRC for given \a data block by applying folding and
 *        reduction methods.
 *
 * Algorithm operates on 32 bit CRCs so polynomials and initial values may
 * need to be promoted to 32 bits where required.
 *
 * @param crc initial CRC value (32 bit value)
 * @param data pointer to data block
 * @param data_len length of \a data block in bytes
 * @param params pointer to PCLMULQDQ CRC calculation context
 *
 * @return CRC for given \a data block (32 bits wide).
 */
__forceinline
uint32_t
crc32_calc_pclmulqdq(const uint8_t *data,
                     uint32_t data_len, uint32_t crc,
                     const struct crc_pclmulqdq_ctx *params)
{
        __m128i temp, fold, k, swap;
        uint32_t n;

        if (unlikely(data == NULL || data_len == 0 || params == NULL))
                return crc;

#ifdef __KERNEL__
        /**
         * Preserve FPU context
         */
        kernel_fpu_begin();
#endif

        /**
         * Add 4 bytes to data block size
         * This is to secure the following:
         *     CRC32 = M(X)^32 mod P(X)
         * M(X) - message to compute CRC on
         * P(X) - CRC polynomial
         */
        data_len += 4;

        /**
         * Load first 16 data bytes in \a fold and
         * set \a swap BE<->LE 16 byte conversion variable
         */
        fold = _mm_loadu_si128((__m128i *)data);
        swap = crc_xmm_be_le_swap128;

        /**
         * -------------------------------------------------
         * Folding all data into single 16 byte data block
         * Assumes: \a fold holds first 16 bytes of data
         */

        if (unlikely(data_len <= 16)) {
                /**
                 * Data block fits into 16 byte block
                 * - adjust data block
                 * - 4 least significant bytes need to be zero
                 */
                fold = _mm_shuffle_epi8(fold, swap);
                fold = _mm_slli_si128(xmm_shift_right(fold, 20 - data_len), 4);

                /**
                 * Apply CRC init value
                 */
                temp = _mm_insert_epi32(_mm_setzero_si128(), bswap4(crc), 0);
                temp = xmm_shift_left(temp, data_len - 4);
                fold = _mm_xor_si128(fold, temp);
        } else {
                /**
                 * There are 2x16 data blocks or more
                 */
                __m128i next_data;

                /**
                 * n = number of bytes required to align \a data_len
                 *     to multiple of 16
                 */
                n = ((~data_len) + 1) & 15;

                /**
                 * Apply CRC initial value and
                 * get \a fold to BE format
                 */
                fold = _mm_xor_si128(fold,
                                     _mm_insert_epi32(_mm_setzero_si128(),
                                                      crc, 0));
                fold = _mm_shuffle_epi8(fold, swap);

                /**
                 * Load next 16 bytes of data and
                 * adjust \a fold & \a next_data as follows:
                 *
                 * CONCAT(fold,next_data) >> (n*8)
                 */
                next_data = _mm_loadu_si128((__m128i *)&data[16]);
                next_data = _mm_shuffle_epi8(next_data, swap);
                next_data = _mm_or_si128(xmm_shift_right(next_data, n),
                                         xmm_shift_left(fold, 16 - n));
                fold = xmm_shift_right(fold, n);

                if (unlikely(data_len <= 32))
                        /**
                         * In such unlikely case clear 4 least significant bytes
                         */
                        next_data =
                                _mm_slli_si128(_mm_srli_si128(next_data, 4), 4);

                /**
                 * Do the initial folding round on 2 first 16 byte chunks
                 */
                k = _mm_load_si128((__m128i *)(&params->k1));
                fold = crc32_folding_round(next_data, k, fold);

                if (likely(data_len > 32)) {
                        /**
                         * \a data_block needs to be at least 48 bytes long
                         * in order to get here
                         */
                        __m128i new_data;

                        /**
                         * Main folding loop
                         * - n is adjusted to point to next 16 data block
                         *   to read
                         *   (16+16) = 2x16; represents 2 first data blocks
                         *                   processed above
                         *   (- n) is the number of zero bytes padded to
                         *   the message in order to align it to 16 bytes
                         * - the last 16 bytes is processed separately
                         */
                        for (n = 16 + 16 - n; n < (data_len - 16); n += 16) {
                                new_data = _mm_loadu_si128((__m128i *)&data[n]);
                                new_data = _mm_shuffle_epi8(new_data, swap);
                                fold = crc32_folding_round(new_data, k, fold);
                        }

                        /**
                         * The last folding round works always on 12 bytes
                         * (12 bytes of data and 4 zero bytes)
                         * Read from offset -4 is to avoid one
                         * shift right operation.
                         */
                        new_data = _mm_loadu_si128((__m128i *)&data[n - 4]);
                        new_data = _mm_shuffle_epi8(new_data, swap);
                        new_data = _mm_slli_si128(new_data, 4);
                        fold = crc32_folding_round(new_data, k, fold);
                } /* if (data_len > 32) */
        }

        /**
         * -------------------------------------------------
         * Reduction 128 -> 32
         * Assumes: \a fold holds 128bit folded data
         */

        /**
         * REDUCTION 128 -> 64
         */
        k = _mm_load_si128((__m128i *)(&params->k3));
        fold = crc32_reduce_128_to_64(fold, k);

        /**
         * REDUCTION 64 -> 32
         */
        n = crc32_reduce_64_to_32(fold, k,
                                  _mm_load_si128((__m128i *)(&params->p)));

#ifdef __KERNEL__
        /**
         * - restore FPU context
         */
        kernel_fpu_end();
#endif

        return n;
}

#endif /* __CRC_H__ */
