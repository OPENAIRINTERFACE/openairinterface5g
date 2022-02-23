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

/* Common data types, macros and functions used across the CRC library */

#ifndef CRC_TYPES_H_
#define CRC_TYPES_H_

#ifdef __KERNEL__
#include <asm/i387.h>
#include <linux/types.h>
typedef uint32_t uint_fast32_t; 
typedef uint16_t uint_fast16_t; 
typedef uint8_t uint_fast8_t; 
#else
#include <stdint.h>
#include <stddef.h>
#endif

/* Declare variable at address aligned to a boundary */
#define DECLARE_ALIGNED(_declaration, _boundary)         \
        _declaration __attribute__((aligned(_boundary)))

/* Macro to make function to be always inlined */
#ifndef DEBUG
#define __forceinline                                   \
        static inline __attribute__((always_inline))
#else
#define __forceinline                                   \
        static
#endif

/* Likely & unliekly hints for the compiler */
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* Macro to get dimension of an array */
#ifndef DIM
#define DIM(x) (sizeof(x)/sizeof(x[0]))
#endif

/**
 * Common functions
 */

/**
 * @brief Swaps bytes in 16 bit word
 *
 * @param val 16 bit data value
 *
 * @return byte swapped value
 */
__forceinline uint16_t bswap2(const uint16_t val)
{
        return (uint16_t) ((val >> 8) | (val << 8));
}

/**
 * @brief Swaps bytes in 32 bit word
 * ABCD -> DCBA
 *
 * @param val 32 bit data value
 *
 * @return byte swapped value
 */
__forceinline uint32_t bswap4(const uint32_t val)
{
        return ((val >> 24) |             /**< A*/
                ((val & 0xff0000) >> 8) | /**< B*/
                ((val & 0xff00) << 8) |   /**< C*/
                (val << 24));             /**< D*/
}

#endif /* CRC_TYPES_H_ */
