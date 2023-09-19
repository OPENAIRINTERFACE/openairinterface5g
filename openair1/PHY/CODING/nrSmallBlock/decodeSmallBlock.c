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

/*!\file PHY/CODING/nrSmallBlock/decodeSmallBlock.c
 * \brief
 * \author Turker Yilmaz
 * \date 2019
 * \version 0.1
 * \company EURECOM
 * \email turker.yilmaz@eurecom.fr
 * \note
 * \warning
*/

#include "PHY/CODING/nrSmallBlock/nr_small_block_defs.h"
#include "assertions.h"
#include "PHY/sse_intrin.h"
#if defined(__AVX512F__)
#include <simde/x86/avx512.h>
// simde current version missed this instruction
#define simde_mm512_reduce_add_epi32 _mm512_reduce_add_epi32
#define simde_mm512_cvtepi8_epi32 _mm512_cvtepi8_epi32
#endif

//#define DEBUG_DECODESMALLBLOCK

//input = [d̂_0] [d̂_1] [d̂_2] ... [d̂_31]
//output = [? ... ? ĉ_K-1 ... ĉ_2 ĉ_1 ĉ_0]

uint16_t decodeSmallBlock(int8_t *in, uint8_t len){
	uint16_t out = 0;

	AssertFatal(len >= 3 && len <= 11, "[decodeSmallBlock] Message Length = %d (Small Block Coding is only defined for input lengths 3 to 11)", len);

	if(len<7) {
		int16_t Rhat[NR_SMALL_BLOCK_CODED_BITS] = {0}, Rhatabs[NR_SMALL_BLOCK_CODED_BITS] = {0};
		uint16_t maxVal;
		uint8_t maxInd = 0;
		uint8_t jmax = (1<<(len-1));
		for (int j = 0; j < jmax; ++j)
			for (int k = 0; k < NR_SMALL_BLOCK_CODED_BITS; ++k)
				Rhat[j] += in[k] * hadamard32InterleavedTransposed[j][k];

		for (int i = 0; i < NR_SMALL_BLOCK_CODED_BITS; i += 16) {
			simde__m256i a15_a0 = simde_mm256_loadu_si256((simde__m256i*)&Rhat[i]);
			a15_a0 = simde_mm256_abs_epi16(a15_a0);
			simde_mm256_storeu_si256((simde__m256i*)(&Rhatabs[i]), a15_a0);
		}
		maxVal = Rhatabs[0];
		for (int k = 1; k < jmax; ++k){
			if (Rhatabs[k] > maxVal){
				maxVal = Rhatabs[k];
				maxInd = k;
			}
		}

		out = properOrderedBasis[maxInd] | ( (Rhat[maxInd] > 0) ? (uint16_t)0 : (uint16_t)1 );

#ifdef DEBUG_DECODESMALLBLOCK
		for (int k = 0; k < jmax; ++k)
			printf("[decodeSmallBlock]Rhat[%d]=%d %d %d %d\n",k, Rhat[k], maxVal, maxInd, ((uint32_t)out>>k)&1);
		printf("[decodeSmallBlock]0x%x 0x%x\n", out, properOrderedBasis[maxInd]);
#endif

	} else {
		uint8_t maxRow = 0, maxCol = 0;

        int16_t maxVal = 0;
		int DmatrixElementVal = 0;
#if !defined(__AVX512F__)
		int8_t DmatrixElement[NR_SMALL_BLOCK_CODED_BITS] = {0};
#endif		
		simde__m256i _in_256 = simde_mm256_loadu_si256 ((simde__m256i*)&in[0]);
		simde__m256i _maskD_256, _Dmatrixj_256, _maskH_256, _DmatrixElement_256;
		for (int j = 0; j < ( 1<<(len-6) ); ++j) {
			_maskD_256 = simde_mm256_loadu_si256 ((simde__m256i*)(&maskD[j][0]));
			_Dmatrixj_256 = simde_mm256_sign_epi8 (_in_256, _maskD_256);
			for (int k = 0; k < NR_SMALL_BLOCK_CODED_BITS; ++k) {
				_maskH_256 = simde_mm256_loadu_si256 ((simde__m256i*)(&hadamard32InterleavedTransposed[k][0]));
				_DmatrixElement_256 = simde_mm256_sign_epi8 (_Dmatrixj_256, _maskH_256);
#if defined(__AVX512F__)
        DmatrixElementVal = simde_mm512_reduce_add_epi32(
            simde_mm512_add_epi32(simde_mm512_cvtepi8_epi32(simde_mm256_extracti128_si256(_DmatrixElement_256, 1)),
                                  simde_mm512_cvtepi8_epi32(simde_mm256_castsi256_si128(_DmatrixElement_256))));
#else
				simde_mm256_storeu_si256((simde__m256i*)(&DmatrixElement[0]), _DmatrixElement_256);
				for (int i = 0; i < NR_SMALL_BLOCK_CODED_BITS; ++i)
					DmatrixElementVal += DmatrixElement[i];
#endif
				if (abs(DmatrixElementVal) > abs(maxVal)){
					maxVal = DmatrixElementVal;
					maxRow = j;
					maxCol = k;
				}
				DmatrixElementVal=0;
			}
		}
		out = properOrderedBasisExtended[maxRow] | properOrderedBasis[maxCol] | ( (maxVal > 0) ? (uint16_t)0 : (uint16_t)1 );
#ifdef DEBUG_DECODESMALLBLOCK
		for (int k = 0; k < NR_SMALL_BLOCK_CODED_BITS; ++k)
					printf("[decodeSmallBlock]maxRow = %d maxCol = %d out[%d]=%d\n", maxRow, maxCol, k, ((uint32_t)out>>k)&1);
#endif

	}

	return out;
}
