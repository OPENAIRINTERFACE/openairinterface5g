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

/*!\file nrLDPC_cnProc.h
 * \brief Defines the functions for check node processing
 * Version AVX512 
*/

#ifndef __NR_LDPC_CNPROC__H__
#define __NR_LDPC_CNPROC__H__
#include <immintrin.h>
#include <avx512fintrin.h>
//#include "include/immintrin.h"


/**
   \brief Performs CN processing for BG2 on the CN processing buffer and stores the results in the CN processing results buffer.
   \param p_lut Pointer to decoder LUTs
   \param p_procBuf Pointer to processing buffers
   \param Z Lifting size
*/

    //intrinsic sign avec AVX512  car __m512i _mm512_sign_epi16 (__m512i a, __m512i b) n'exite pas en AVX, c'est pour quoi je la rédefini ici
    /* Emulate _mm512_sign_epi16() with instructions  that exist in the AVX-512 instruction set      */
    __m512i mm512_sign_epi16(__m512i a, __m512i b){
        b = _mm512_min_epi16(b, _mm512_set1_epi16(1));  
        b = _mm512_max_epi16(b, _mm512_set1_epi16(-1)); 
        a = _mm512_mullo_epi16(a, b);                 
        return a;
    } 


/* comment
static inline void nrLDPC_cnProc_BG2(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint64_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;
    
    __m512i* p_cnProcBuf;
    __m512i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint64_t M;
    uint64_t i;
    uint64_t j;
    uint64_t k;
    // Offset to each bit within a group in terms of 64 bytes
    uint64_t bitOffsetInGroup;

    __m512i zmm0, min, sgn;
    __m512i* p_cnProcBufResBit;

    const __m512i* p_ones   = (__m512i*) ones512_epi8;
    const __m512i* p_maxLLR = (__m512i*) maxLLR512_epi8;




    // LUT with offsets for bits that need to be processed
    // 1. bit proc requires LLRs of 2. and 3. bit, 2.bits of 1. and 3. etc.
    // Offsets are in units of bitOffsetInGroup
    const uint8_t lut_idxCnProcG3[3][2] = {{36,72}, {0,72}, {0,36}};

    // =====================================================================
    // Process group with 3 BNs

    if (lut_numCnInCnGroups[0] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[0]*Z + 63)>>5;
        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[0]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 3
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[0]];

        // Loop over every BN
        for (j=0; j<3; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

          //  __m512i *pj0 = &p_cnProcBuf[lut_idxCnProcG3[j][0]];
         //   __m512i *pj1 = &p_cnProcBuf[lut_idxCnProcG3[j][1]];

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
	            zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // 64 CNS of second BN
		        zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][1] + i];
		
                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                sgn  = mm512_sign_epi16(sgn, zmm0);

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
	
            }
        }
    }

    // =====================================================================
    // Process group with 4 BNs

    // Offset is 20*384/64 = 120
    const uint16_t lut_idxCnProcG4[4][3] = {{120,240,360}, {0,240,360}, {0,120,360}, {0,120,240}};

    if (lut_numCnInCnGroups[1] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[1]*Z + 63)>>5;
        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[1]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 4
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

        // Loop over every BN
        for (j=0; j<4; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG4[j][0] + i];
                sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<3; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG4[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 5 BNs

    // Offset is 9*384/64 = 54
    const uint16_t lut_idxCnProcG5[5][4] = {{54,108,162,216}, {0,108,162,216},
                                            {0,54,162,216}, {0,54,108,216}, {0,54,108,162}};

    if (lut_numCnInCnGroups[2] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[2]*Z + 63)>>5;
        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[2]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 5
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[2]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[2]];

        // Loop over every BN
        for (j=0; j<5; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG5[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<4; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG5[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 6 BNs

    // Offset is 3*384/64 = 18
    const uint16_t lut_idxCnProcG6[6][5] = {{18,36,54,72,90}, {0,36,54,72,90},
                                            {0,18,54,72,90}, {0,18,36,72,90},
                                            {0,18,36,54,90}, {0,18,36,54,72}};

    if (lut_numCnInCnGroups[3] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[3]*Z + 63)>>5;
        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[3]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 6
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[3]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[3]];

        // Loop over every BN
        for (j=0; j<6; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG6[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<5; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG6[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 8 BNs

    // Offset is 2*384/64 = 12
    const uint8_t lut_idxCnProcG8[8][7] = {{12,24,36,48,60,72,84}, {0,24,36,48,60,72,84},
                                           {0,12,36,48,60,72,84}, {0,12,24,48,60,72,84},
                                           {0,12,24,36,60,72,84}, {0,12,24,36,48,72,84},
                                           {0,12,24,36,48,60,84}, {0,12,24,36,48,60,72}};

    if (lut_numCnInCnGroups[4] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[4]*Z + 63)>>5;
        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[4]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 8
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[4]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[4]];

        // Loop over every BN
        for (j=0; j<8; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG8[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<7; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG8[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 10 BNs

    // Offset is 2*384/64 = 12
    const uint8_t lut_idxCnProcG10[10][9] = {{12,24,36,48,60,72,84,96,108}, {0,24,36,48,60,72,84,96,108},
                                             {0,12,36,48,60,72,84,96,108}, {0,12,24,48,60,72,84,96,108},
                                             {0,12,24,36,60,72,84,96,108}, {0,12,24,36,48,72,84,96,108},
                                             {0,12,24,36,48,60,84,96,108}, {0,12,24,36,48,60,72,96,108},
                                             {0,12,24,36,48,60,72,84,108}, {0,12,24,36,48,60,144,84,96}};

    if (lut_numCnInCnGroups[5] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[5]*Z + 63)>>5;
        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[5]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 10
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[5]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[5]];

        // Loop over every BN
        for (j=0; j<10; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG10[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<9; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG10[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

}
*/
/**
   \brief Performs CN processing for BG1 on the CN processing buffer and stores the results in the CN processing results buffer.
   \param p_lut Pointer to decoder LUTs
   \param Z Lifting size
*/
static inline void nrLDPC_cnProc_BG1(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint64_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;
    
    __m512i* p_cnProcBuf;
    __m512i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint64_t M;
    uint64_t i;
    uint64_t j;
    uint64_t k;
    // Offset to each bit within a group in terms of 64 bytes
    uint64_t bitOffsetInGroup;

    __m512i zmm0, min, sgn;
    __m512i* p_cnProcBufResBit;

    const __m512i* p_ones   = (__m512i*) ones512_epi8;
    const __m512i* p_maxLLR = (__m512i*) maxLLR512_epi8;

    // LUT with offsets for bits that need to be processed
    // 1. bit proc requires LLRs of 2. and 3. bit, 2.bits of 1. and 3. etc.
    // Offsets are in units of bitOffsetInGroup (1*384/64)=6

    const uint8_t lut_idxCnProcG3[3][2] = {{6,12}, {0,12}, {0,6}};

    // =====================================================================
    // Process group with 3 BNs

    if (lut_numCnInCnGroups[0] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[0]*Z + 63)>>5;

        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[0]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 3
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[0]];

        // Loop over every BN
        for (j=0; j<3; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);  
                min  = _mm512_abs_epi8(zmm0);

                // 64 CNS of second BN
                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][1] + i];
                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                sgn  = mm512_sign_epi16(sgn, zmm0);

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 4 BNs

    // Offset is 5*384/64 = 30
    const uint8_t lut_idxCnProcG4[4][3] = {{30,60,90}, {0,60,90}, {0,30,90}, {0,30,60}};

    if (lut_numCnInCnGroups[1] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[1]*Z + 63)>>5;

        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[1]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 4
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

        // Loop over every BN
        for (j=0; j<4; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG4[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<3; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG4[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 5 BNs

    // Offset is 18*384/64 = 108
    const uint16_t lut_idxCnProcG5[5][4] = {{108,216,324,432}, {0,216,324,432},
                                            {0,108,324,432}, {0,108,216,432}, {0,108,216,324}};

    if (lut_numCnInCnGroups[2] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[2]*Z + 63)>>5;

        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[2]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 5
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[2]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[2]];

        // Loop over every BN
        for (j=0; j<5; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG5[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<4; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG5[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 6 BNs

    // Offset is 8*384/64 = 48
    const uint16_t lut_idxCnProcG6[6][5] = {{48,96,144,192,240}, {0,96,144,192,240},
                                            {0,48,144,192,240}, {0,48,96,192,240},
                                            {0,48,96,144,240}, {0,48,96,144,192}};

    if (lut_numCnInCnGroups[3] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[3]*Z + 63)>>5;

        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[3]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 6
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[3]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[3]];

        // Loop over every BN
        for (j=0; j<6; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG6[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<5; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG6[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 7 BNs

    // Offset is 5*384/64 = 30
    const uint16_t lut_idxCnProcG7[7][6] = {{30,60,90,120,150,180}, {0,60,90,120,150,180},
                                            {0,30,90,120,150,180},   {0,30,60,120,150,180},
                                            {0,30,60,90,150,180},   {0,30,60,90,120,180},
                                            {0,30,60,90,120,150}};

    if (lut_numCnInCnGroups[4] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[4]*Z + 63)>>5;

        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[4]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 7
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[4]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[4]];

        // Loop over every BN
        for (j=0; j<7; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG7[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<6; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG7[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 8 BNs

    // Offset is 2*384/64 = 12
    const uint8_t lut_idxCnProcG8[8][7] = {{12,24,36,48,56,72,84}, {0,24,36,48,56,72,84},
                                           {0,12,36,48,56,72,84}, {0,12,24,48,56,72,84},
                                           {0,12,24,36,56,72,84}, {0,12,24,36,48,72,84},
                                           {0,12,24,36,48,56,84}, {0,12,24,36,48,120,72}};

    if (lut_numCnInCnGroups[5] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[5]*Z + 63)>>5;

        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[5]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 8
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[5]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[5]];

        // Loop over every BN
        for (j=0; j<8; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG8[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<7; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG8[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 9 BNs

    // Offset is 2*384/64 = 12
    const uint8_t lut_idxCnProcG9[9][8] = {{12,24,36,48,60,72,84,96}, {0,24,36,48,60,72,84,96},
                                           {0,12,36,48,60,72,84,96}, {0,12,24,48,60,72,84,96},
                                           {0,12,24,36,60,72,84,96}, {0,12,24,36,48,72,84,96},
                                           {0,12,24,36,48,60,84,96}, {0,12,24,36,48,60,72,96},
                                           {0,12,24,36,48,60,72,84}};

    if (lut_numCnInCnGroups[6] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[6]*Z + 63)>>5;

        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[6]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 9
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[6]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[6]];

        // Loop over every BN
        for (j=0; j<9; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG9[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<8; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG9[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 10 BNs

    // Offset is 1*384/64 = 6
    const uint8_t lut_idxCnProcG10[10][9] = {{6,12,18,24,30,36,42,48,54}, {0,12,18,24,30,36,42,48,54},
                                             {0,6,18,24,30,36,42,48,54}, {0,6,12,24,30,36,42,48,54},
                                             {0,6,12,18,30,36,42,48,54}, {0,6,12,18,24,36,42,48,54},
                                             {0,6,12,18,24,30,42,48,54}, {0,6,12,18,24,30,36,48,54},
                                             {0,6,12,18,24,30,36,42,54}, {0,6,12,36,24,30,36,42,48}};

    if (lut_numCnInCnGroups[7] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[7]*Z + 63)>>5;

        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[7]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 10
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[7]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[7]];

        // Loop over every BN
        for (j=0; j<10; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG10[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<9; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG10[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 19 BNs

    // Offset is 4*384/64 = 24
    const uint16_t lut_idxCnProcG19[19][18] = {{24,48,72,96,120,144,168,192,216,240,264,288,312,336,360,384,408,432}, {0,48,72,96,120,144,168,192,216,240,264,288,312,336,360,384,408,432},
                                               {0,24,72,96,120,144,168,192,216,240,264,288,312,336,360,384,408,432}, {0,24,48,96,120,144,168,192,216,240,264,288,312,336,360,384,408,432},
                                               {0,24,48,72,120,144,168,192,216,240,264,288,312,336,360,384,408,432}, {0,24,48,72,96,144,168,192,216,240,264,288,312,336,360,384,408,432},
                                               {0,24,48,72,96,120,168,192,216,240,264,288,312,336,360,384,408,432}, {0,24,48,72,96,120,144,192,216,240,264,288,312,336,360,384,408,432},
                                               {0,24,48,72,96,120,144,168,216,240,264,288,312,336,360,384,408,432}, {0,24,48,72,96,120,144,168,192,240,264,288,312,336,360,384,408,432},
                                               {0,24,48,72,96,120,144,168,192,216,264,288,312,336,360,384,408,432}, {0,24,48,72,96,120,144,168,192,216,240,288,312,336,360,384,408,432},
                                               {0,24,48,72,96,120,144,168,192,216,240,264,312,336,360,384,408,432}, {0,24,48,72,96,120,144,168,192,216,240,264,288,336,360,384,408,432},
                                               {0,24,48,72,96,120,144,168,192,216,240,264,288,312,360,384,408,432}, {0,24,48,72,96,120,144,168,192,216,240,264,288,312,336,384,408,432},
                                               {0,24,48,72,96,120,144,168,192,216,240,264,288,312,336,360,408,432}, {0,24,48,72,96,120,144,168,192,216,240,264,288,312,336,360,384,432},
                                               {0,24,48,72,96,120,144,168,192,216,240,264,288,312,336,360,384,408}};
    if (lut_numCnInCnGroups[8] > 0)
    {
        // Number of groups of 64 CNS for parallel processing
        // Ceil for values not divisible 64
        M = (lut_numCnInCnGroups[8]*Z + 63)>>5;

        // Set the offset to each bit within a group in terms of 64 bytes
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[8]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 19
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[8]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[8]];

        // Loop over every BN
        for (j=0; j<19; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 64 CNS (first BN)
                zmm0 = p_cnProcBuf[lut_idxCnProcG19[j][0] + i];
               sgn  = mm512_sign_epi16(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<18; k++)
                {
                    zmm0 = p_cnProcBuf[lut_idxCnProcG19[j][k] + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                    sgn  = mm512_sign_epi16(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = mm512_sign_epi16(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

}
/**
   \brief Performs parity check for BG1 on the CN processing buffer. Stops as soon as error is detected.
   \param p_lut Pointer to decoder LUTs
   \param Z Lifting size
   \return 32-bit parity check indicator
*/
static inline uint64_t nrLDPC_cnProcPc_BG1(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint64_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;
    
    __m512i* p_cnProcBuf;
    __m512i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint64_t M;
    uint64_t i;
    uint64_t j;
    uint64_t pcRes = 0;
    uint64_t pcResSum = 0;
    uint64_t Mrem;
    uint64_t M64;

    __m512i zmm0, zmm1;

    // =====================================================================
    // Process group with 3 BNs

    if (lut_numCnInCnGroups[0] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[0]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 3
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[0]];

        // Loop over CNs
        for (i=0; i<(M64-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<3; j++)
            {
                // BN offset is units of (1*384/64) = 6
                zmm0 = p_cnProcBuf   [j*6 + i];
                zmm1 = p_cnProcBufRes[j*6 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<3; j++)
        {
            // BN offset is units of (1*384/64) = 6
            zmm0 = p_cnProcBuf   [j*6 + i];
            zmm1 = p_cnProcBufRes[j*6 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 4 BNs

    if (lut_numCnInCnGroups[1] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[1]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 4
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

        // Loop over CNs
        for (i=0; i<(M64-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<4; j++)
            {
                // BN offset is units of 5*384/64 = 30
                zmm0 = p_cnProcBuf   [j*30 + i];
                zmm1 = p_cnProcBufRes[j*30 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<4; j++)
        {
            // BN offset is units of 5*384/64 = 30
            zmm0 = p_cnProcBuf   [j*30 + i];
            zmm1 = p_cnProcBufRes[j*30 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 5 BNs

    if (lut_numCnInCnGroups[2] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[2]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 5
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[2]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[2]];

        // Loop over CNs
        for (i=0; i<(M64-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<5; j++)
            {
                // BN offset is units of 18*384/64 = 108
                zmm0 = p_cnProcBuf   [j*108 + i];
                zmm1 = p_cnProcBufRes[j*108 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;

        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<5; j++)
        {
            // BN offset is units of 18*384/64 = 108
            zmm0 = p_cnProcBuf   [j*108 + i];
            zmm1 = p_cnProcBufRes[j*108 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 6 BNs

    if (lut_numCnInCnGroups[3] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[3]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 6
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[3]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[3]];

        // Loop over CNs
        for (i=0; i<(M64-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<6; j++)
            {
                // BN offset is units of 8*384/64 = 48
                zmm0 = p_cnProcBuf   [j*48 + i];
                zmm1 = p_cnProcBufRes[j*48 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<6; j++)
        {
            // BN offset is units of 8*384/64 = 48
            zmm0 = p_cnProcBuf   [j*48 + i];
            zmm1 = p_cnProcBufRes[j*48 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 7 BNs

    if (lut_numCnInCnGroups[4] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[4]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 7
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[4]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[4]];

        // Loop over CNs
        for (i=0; i<(M64-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<7; j++)
            {
                // BN offset is units of 5*384/64 = 30
                zmm0 = p_cnProcBuf   [j*30 + i];
                zmm1 = p_cnProcBufRes[j*30 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<7; j++)
        {
            // BN offset is units of 5*384/64 = 30
            zmm0 = p_cnProcBuf   [j*30 + i];
            zmm1 = p_cnProcBufRes[j*30 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 8 BNs

    if (lut_numCnInCnGroups[5] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[5]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 8
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[5]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[5]];

        // Loop over CNs
        for (i=0; i<(M64-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<8; j++)
            {
                // BN offset is units of 2*384/64 = 12
                zmm0 = p_cnProcBuf   [j*12 + i];
                zmm1 = p_cnProcBufRes[j*12 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<8; j++)
        {
            // BN offset is units of 2*384/64 = 12
            zmm0 = p_cnProcBuf   [j*12 + i];
            zmm1 = p_cnProcBufRes[j*12 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 9 BNs

    if (lut_numCnInCnGroups[6] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[6]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 9
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[6]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[6]];

        // Loop over CNs
        for (i=0; i<(M64-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<9; j++)
            {
                // BN offset is units of 2*384/64 = 12
                zmm0 = p_cnProcBuf   [j*12 + i];
                zmm1 = p_cnProcBufRes[j*12 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<9; j++)
        {
            // BN offset is units of 2*384/64 = 12
            zmm0 = p_cnProcBuf   [j*12 + i];
            zmm1 = p_cnProcBufRes[j*12 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 10 BNs

    if (lut_numCnInCnGroups[7] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[7]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 10
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[7]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[7]];

        // Loop over CNs
        for (i=0; i<(M64-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<10; j++)
            {
                // BN offset is units of 1*384/64 = 6
                zmm0 = p_cnProcBuf   [j*6 + i];
                zmm1 = p_cnProcBufRes[j*6 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<10; j++)
        {
            // BN offset is units of 1*384/64 = 6
            zmm0 = p_cnProcBuf   [j*6 + i];
            zmm1 = p_cnProcBufRes[j*6 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 19 BNs

    if (lut_numCnInCnGroups[8] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[8]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 19
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[8]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[8]];

        // Loop over CNs
        for (i=0; i<(M64-1); i++)
        {
            pcRes = 0;
            // Loop over every BN (Last BN is connected to multiple CNs)
            // Compute PC for 64 CNs at once
            for (j=0; j<19; j++)
            {
                // BN offset is units of 4*384/64 = 24
                zmm0 = p_cnProcBuf   [j*24 + i];
                zmm1 = p_cnProcBufRes[j*24 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN (Last BN is connected to multiple CNs)
        // Compute PC for 64 CNs at once
        for (j=0; j<19; j++)
        {
            // BN offset is units of 4*384/64 = 24
            zmm0 = p_cnProcBuf   [j*24 + i];
            zmm1 = p_cnProcBufRes[j*24 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    return pcResSum;
}





/**
   \brief Performs parity check for BG2 on the CN processing buffer. Stops as soon as error is detected.
   \param p_lut Pointer to decoder LUTs
   \param Z Lifting size
   \return 32-bit parity check indicator
*/

/*** comment
static inline uint64_t nrLDPC_cnProcPc_BG2(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint64_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;
    
    __m512i* p_cnProcBuf;
    __m512i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint64_t M;
    uint64_t i;
    uint64_t j;
    uint64_t pcRes = 0;
    uint64_t pcResSum = 0;
    uint64_t Mrem;
    uint64_ ;

    __m512i zmm0, zmm1;

    // =====================================================================
    // Process group with 3 BNs

    if (lut_numCnInCnGroups[0] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[0]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 3
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[0]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<3; j++)
            {
                // BN offset is units of (6*384/32) = 72
                zmm0 = p_cnProcBuf   [j*72 + i];
                zmm1 = p_cnProcBufRes[j*72 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<3; j++)
        {
            // BN offset is units of (6*384/32) = 72
            zmm0 = p_cnProcBuf   [j*72 + i];
            zmm1 = p_cnProcBufRes[j*72 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 4 BNs

    if (lut_numCnInCnGroups[1] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[1]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 4
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<4; j++)
            {
                // BN offset is units of 20*384/32 = 240
                zmm0 = p_cnProcBuf   [j*240 + i];
                zmm1 = p_cnProcBufRes[j*240 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<4; j++)
        {
            // BN offset is units of 20*384/32 = 240
            zmm0 = p_cnProcBuf   [j*240 + i];
            zmm1 = p_cnProcBufRes[j*240 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 5 BNs

    if (lut_numCnInCnGroups[2] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[2]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 5
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[2]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[2]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<5; j++)
            {
                // BN offset is units of 9*384/32 = 108
                zmm0 = p_cnProcBuf   [j*108 + i];
                zmm1 = p_cnProcBufRes[j*108 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<5; j++)
        {
            // BN offset is units of 9*384/32 = 108
            zmm0 = p_cnProcBuf   [j*108 + i];
            zmm1 = p_cnProcBufRes[j*108 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 6 BNs

    if (lut_numCnInCnGroups[3] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[3]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 6
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[3]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[3]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<6; j++)
            {
                // BN offset is units of 3*384/32 = 36
                zmm0 = p_cnProcBuf   [j*36 + i];
                zmm1 = p_cnProcBufRes[j*36 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<6; j++)
        {
            // BN offset is units of 3*384/32 = 36
            zmm0 = p_cnProcBuf   [j*36 + i];
            zmm1 = p_cnProcBufRes[j*36 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 8 BNs

    if (lut_numCnInCnGroups[4] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[4]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 8
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[4]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[4]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<8; j++)
            {
                // BN offset is units of 2*384/32 = 24
                zmm0 = p_cnProcBuf   [j*24 + i];
                zmm1 = p_cnProcBufRes[j*24 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<8; j++)
        {
            // BN offset is units of 2*384/32 = 24
            zmm0 = p_cnProcBuf   [j*24 + i];
            zmm1 = p_cnProcBufRes[j*24 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    // =====================================================================
    // Process group with 10 BNs

    if (lut_numCnInCnGroups[5] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[5]*Z;
        // Remainder modulo 32
        Mrem = M&63;
        // Number of groups of 64 CNs for parallel processing
        // Ceil for values not divisible by 32
       M64 = (M + 63)>>5;

        // Set pointers to start of group 10
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[5]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[5]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 64 CNs at once
            for (j=0; j<10; j++)
            {
                // BN offset is units of 2*384/32 = 24
                zmm0 = p_cnProcBuf   [j*24 + i];
                zmm1 = p_cnProcBufRes[j*24 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 64 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 64 CNs at once
        for (j=0; j<10; j++)
        {
            // BN offset is units of 2*384/32 = 24
            zmm0 = p_cnProcBuf   [j*24 + i];
            zmm1 = p_cnProcBufRes[j*24 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm512_movemask_epi8(_mm512_adds_epi8(zmm0,zmm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(64 - Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    return pcResSum;
}
*/
#endif

