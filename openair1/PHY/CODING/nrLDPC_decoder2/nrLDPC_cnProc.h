
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
 * \author Sebastian Wagner (TCL Communications) Email: <mailto:sebastian.wagner@tcl.com>
 * \date 30-09-2019
 * \version 1.0
 * \note
 * \warning
 */

#ifndef __NR_LDPC_CNPROC__H__
#define __NR_LDPC_CNPROC__H__

/**
   \brief Performs CN processing for BG2 on the CN processing buffer and stores the results in the CN processing results buffer.
   \param p_lut Pointer to decoder LUTs
   \param p_procBuf Pointer to processing buffers
   \param Z Lifting size
*/

#define conditional_negate(a,b,z) _mm512_mask_sub_epi8(a,_mm512_movepi8_mask(b),z,a)

#ifdef __AVX512BW__
static inline void nrLDPC_cnProc_BG2_AVX512(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;

    __m512i* p_cnProcBuf;
    __m512i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint32_t M;
    uint32_t i;
    uint32_t j;
    uint32_t k;
    // Offset to each bit within a group in terms of 32 Byte
    uint32_t bitOffsetInGroup;

    __m512i zmm0, min, sgn, zeros;
    zeros  = _mm512_setzero_si512();
//     maxLLR = _mm512_set1_epi8((char)127);
    __m512i* p_cnProcBufResBit;

    const __m512i* p_ones   = (__m512i*) ones512_epi8;
    const __m512i* p_maxLLR = (__m512i*) maxLLR512_epi8;

    // LUT with offsets for bits that need to be processed
    // 1. bit proc requires LLRs of 2. and 3. bit, 2.bits of 1. and 3. etc.
    // Offsets are in units of bitOffsetInGroup
    const uint8_t lut_idxCnProcG3[3][2] = {{72,144}, {0,144}, {0,72}};

    // =====================================================================
    // Process group with 3 BNs

    if (lut_numCnInCnGroups[0] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[0]*Z + 63)>>6;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[0]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 3
        p_cnProcBuf    = (__m512i*) &cnProcBuf   [lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (__m512i*) &cnProcBufRes[lut_startAddrCnGroups[0]];

        // Loop over every BN
        for (j=0; j<3; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            __m512i *pj0 = &p_cnProcBuf[(lut_idxCnProcG3[j][0]/2)];
            __m512i *pj1 = &p_cnProcBuf[(lut_idxCnProcG3[j][1]/2)];

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
              //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
                zmm0 = pj0[i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // 32 CNs of second BN
                //  zmm0 = p_cnProcBuf[(lut_idxCnProcG3[j][1]/2) + i];
                zmm0 = pj1[i];
                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                sgn  = _mm512_xor_si512(sgn, zmm0);

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
                //p_cnProcBufResBit[i]=_mm512_sign_epi8(min, sgn);
            }
        }
    }

    // =====================================================================
    // Process group with 4 BNs

    // Offset is 20*384/32 = 240
    const uint16_t lut_idxCnProcG4[4][3] = {{240,480,720}, {0,480,720}, {0,240,720}, {0,240,480}};

    if (lut_numCnInCnGroups[1] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[1]*Z + 63)>>6;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[1]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG4[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<3; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG4[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 5 BNs

    // Offset is 9*384/32 = 108
    const uint16_t lut_idxCnProcG5[5][4] = {{108,216,324,432}, {0,216,324,432},
                                            {0,108,324,432}, {0,108,216,432}, {0,108,216,324}};

    if (lut_numCnInCnGroups[2] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[2]*Z + 63)>>6;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[2]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG5[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<4; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG5[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 6 BNs

    // Offset is 3*384/32 = 36
    const uint16_t lut_idxCnProcG6[6][5] = {{36,72,108,144,180}, {0,72,108,144,180},
                                            {0,36,108,144,180}, {0,36,72,144,180},
                                            {0,36,72,108,180}, {0,36,72,108,144}};

    if (lut_numCnInCnGroups[3] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[3]*Z + 63)>>6;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[3]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG6[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<5; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG6[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 8 BNs

    // Offset is 2*384/32 = 24
    const uint8_t lut_idxCnProcG8[8][7] = {{24,48,72,96,120,144,168}, {0,48,72,96,120,144,168},
                                           {0,24,72,96,120,144,168}, {0,24,48,96,120,144,168},
                                           {0,24,48,72,120,144,168}, {0,24,48,72,96,144,168},
                                           {0,24,48,72,96,120,168}, {0,24,48,72,96,120,144}};

    if (lut_numCnInCnGroups[4] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[4]*Z + 63)>>6;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[4]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG8[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<7; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG8[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 10 BNs

    // Offset is 2*384/32 = 24
    const uint8_t lut_idxCnProcG10[10][9] = {{24,48,72,96,120,144,168,192,216}, {0,48,72,96,120,144,168,192,216},
                                             {0,24,72,96,120,144,168,192,216}, {0,24,48,96,120,144,168,192,216},
                                             {0,24,48,72,120,144,168,192,216}, {0,24,48,72,96,144,168,192,216},
                                             {0,24,48,72,96,120,168,192,216}, {0,24,48,72,96,120,144,192,216},
                                             {0,24,48,72,96,120,144,168,216}, {0,24,48,72,96,120,144,168,192}};

    if (lut_numCnInCnGroups[5] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[5]*Z + 63)>>6;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[5]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG10[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<9; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG10[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

}

#else

static inline void nrLDPC_cnProc_BG2(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;

    __m256i* p_cnProcBuf;
    __m256i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint32_t M;
    uint32_t i;
    uint32_t j;
    uint32_t k;
    // Offset to each bit within a group in terms of 32 Byte
    uint32_t bitOffsetInGroup;

    __m256i ymm0, min, sgn;
    __m256i* p_cnProcBufResBit;

    const __m256i* p_ones   = (__m256i*) ones256_epi8;
    const __m256i* p_maxLLR = (__m256i*) maxLLR256_epi8;

    // LUT with offsets for bits that need to be processed
    // 1. bit proc requires LLRs of 2. and 3. bit, 2.bits of 1. and 3. etc.
    // Offsets are in units of bitOffsetInGroup
    const uint8_t lut_idxCnProcG3[3][2] = {{72,144}, {0,144}, {0,72}};

    // =====================================================================
    // Process group with 3 BNs

    if (lut_numCnInCnGroups[0] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[0]*Z + 31)>>5;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[0]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 3
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[0]];

        // Loop over every BN
        for (j=0; j<3; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            __m256i *pj0 = &p_cnProcBuf[lut_idxCnProcG3[j][0]];
            __m256i *pj1 = &p_cnProcBuf[lut_idxCnProcG3[j][1]];

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
              //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
                ymm0 = pj0[i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // 32 CNs of second BN
                //  ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][1] + i];
                ymm0 = pj1[i];
                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                sgn  = _mm256_sign_epi8(sgn, ymm0);

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
                //p_cnProcBufResBit[i]=_mm256_sign_epi8(min, sgn);
            }
        }
    }

    // =====================================================================
    // Process group with 4 BNs

    // Offset is 20*384/32 = 240
    const uint16_t lut_idxCnProcG4[4][3] = {{240,480,720}, {0,480,720}, {0,240,720}, {0,240,480}};

    if (lut_numCnInCnGroups[1] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[1]*Z + 31)>>5;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[1]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 4
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

        // Loop over every BN
        for (j=0; j<4; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG4[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<3; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG4[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 5 BNs

    // Offset is 9*384/32 = 108
    const uint16_t lut_idxCnProcG5[5][4] = {{108,216,324,432}, {0,216,324,432},
                                            {0,108,324,432}, {0,108,216,432}, {0,108,216,324}};

    if (lut_numCnInCnGroups[2] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[2]*Z + 31)>>5;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[2]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 5
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[2]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[2]];

        // Loop over every BN
        for (j=0; j<5; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG5[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<4; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG5[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 6 BNs

    // Offset is 3*384/32 = 36
    const uint16_t lut_idxCnProcG6[6][5] = {{36,72,108,144,180}, {0,72,108,144,180},
                                            {0,36,108,144,180}, {0,36,72,144,180},
                                            {0,36,72,108,180}, {0,36,72,108,144}};

    if (lut_numCnInCnGroups[3] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[3]*Z + 31)>>5;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[3]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 6
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[3]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[3]];

        // Loop over every BN
        for (j=0; j<6; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG6[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<5; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG6[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 8 BNs

    // Offset is 2*384/32 = 24
    const uint8_t lut_idxCnProcG8[8][7] = {{24,48,72,96,120,144,168}, {0,48,72,96,120,144,168},
                                           {0,24,72,96,120,144,168}, {0,24,48,96,120,144,168},
                                           {0,24,48,72,120,144,168}, {0,24,48,72,96,144,168},
                                           {0,24,48,72,96,120,168}, {0,24,48,72,96,120,144}};

    if (lut_numCnInCnGroups[4] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[4]*Z + 31)>>5;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[4]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 8
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[4]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[4]];

        // Loop over every BN
        for (j=0; j<8; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG8[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<7; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG8[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 10 BNs

    // Offset is 2*384/32 = 24
    const uint8_t lut_idxCnProcG10[10][9] = {{24,48,72,96,120,144,168,192,216}, {0,48,72,96,120,144,168,192,216},
                                             {0,24,72,96,120,144,168,192,216}, {0,24,48,96,120,144,168,192,216},
                                             {0,24,48,72,120,144,168,192,216}, {0,24,48,72,96,144,168,192,216},
                                             {0,24,48,72,96,120,168,192,216}, {0,24,48,72,96,120,144,192,216},
                                             {0,24,48,72,96,120,144,168,216}, {0,24,48,72,96,120,144,168,192}};

    if (lut_numCnInCnGroups[5] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[5]*Z + 31)>>5;
        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[5]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 10
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[5]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[5]];

        // Loop over every BN
        for (j=0; j<10; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG10[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<9; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG10[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

}

#endif

/**
   \brief Performs CN processing for BG1 on the CN processing buffer and stores the results in the CN processing results buffer.
   \param p_lut Pointer to decoder LUTs
   \param Z Lifting size
*/

#ifdef __AVX512BW__


static inline void nrLDPC_cnProc_BG1_AVX512(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;

    __m512i* p_cnProcBuf;
    __m512i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint32_t M;
    uint32_t i;
    uint32_t j;
    uint32_t k;
    // Offset to each bit within a group in terms of 32 Byte
    uint32_t bitOffsetInGroup;

    __m512i zmm0, min, sgn, zeros;

     zeros  = _mm512_setzero_si512();
    // maxLLR = _mm512_set1_epi8((char)127);
    __m512i* p_cnProcBufResBit;


    const __m512i* p_ones   = (__m512i*) ones512_epi8;
    const __m512i* p_maxLLR = (__m512i*) maxLLR512_epi8;




    // LUT with offsets for bits that need to be processed
    // 1. bit proc requires LLRs of 2. and 3. bit, 2.bits of 1. and 3. etc.
    // Offsets are in units of bitOffsetInGroup (1*384/32)
    const uint8_t lut_idxCnProcG3[3][2] = {{12,24}, {0,24}, {0,12}};

    // =====================================================================
    // Process group with 3 BNs

    if (lut_numCnInCnGroups[0] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[0]*Z + 63)>>6;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[0]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG3[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // 32 CNs of second BN
                zmm0 = p_cnProcBuf[(lut_idxCnProcG3[j][1]/2) + i];
                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
               sgn  = _mm512_xor_si512(sgn, zmm0);


                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 4 BNs

    // Offset is 5*384/32 = 60
    const uint8_t lut_idxCnProcG4[4][3] = {{60,120,180}, {0,120,180}, {0,60,180}, {0,60,120}};

    if (lut_numCnInCnGroups[1] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[1]*Z + 63)>>6;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[1]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG4[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<3; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG4[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 5 BNs

    // Offset is 18*384/32 = 216
    const uint16_t lut_idxCnProcG5[5][4] = {{216,432,648,864}, {0,432,648,864},
                                            {0,216,648,864}, {0,216,432,864}, {0,216,432,648}};

    if (lut_numCnInCnGroups[2] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[2]*Z + 63)>>6;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[2]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG5[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<4; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG5[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 6 BNs

    // Offset is 8*384/32 = 96
    const uint16_t lut_idxCnProcG6[6][5] = {{96,192,288,384,480}, {0,192,288,384,480},
                                            {0,96,288,384,480}, {0,96,192,384,480},
                                            {0,96,192,288,480}, {0,96,192,288,384}};

    if (lut_numCnInCnGroups[3] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[3]*Z + 63)>>6;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[3]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG6[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<5; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG6[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 7 BNs

    // Offset is 5*384/32 = 60
    const uint16_t lut_idxCnProcG7[7][6] = {{60,120,180,240,300,360}, {0,120,180,240,300,360},
                                            {0,60,180,240,300,360},   {0,60,120,240,300,360},
                                            {0,60,120,180,300,360},   {0,60,120,180,240,360},
                                            {0,60,120,180,240,300}};

    if (lut_numCnInCnGroups[4] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[4]*Z + 63)>>6;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[4]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG7[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<6; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG7[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 8 BNs

    // Offset is 2*384/32 = 24
    const uint8_t lut_idxCnProcG8[8][7] = {{24,48,72,96,120,144,168}, {0,48,72,96,120,144,168},
                                           {0,24,72,96,120,144,168}, {0,24,48,96,120,144,168},
                                           {0,24,48,72,120,144,168}, {0,24,48,72,96,144,168},
                                           {0,24,48,72,96,120,168}, {0,24,48,72,96,120,144}};

    if (lut_numCnInCnGroups[5] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[5]*Z + 63)>>6;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[5]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG8[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<7; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG8[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 9 BNs

    // Offset is 2*384/32 = 24
    const uint8_t lut_idxCnProcG9[9][8] = {{24,48,72,96,120,144,168,192}, {0,48,72,96,120,144,168,192},
                                           {0,24,72,96,120,144,168,192}, {0,24,48,96,120,144,168,192},
                                           {0,24,48,72,120,144,168,192}, {0,24,48,72,96,144,168,192},
                                           {0,24,48,72,96,120,168,192}, {0,24,48,72,96,120,144,192},
                                           {0,24,48,72,96,120,144,168}};

    if (lut_numCnInCnGroups[6] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[6]*Z + 63)>>6;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[6]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG9[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<8; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG9[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 10 BNs

    // Offset is 1*384/32 = 12
    const uint8_t lut_idxCnProcG10[10][9] = {{12,24,36,48,60,72,84,96,108}, {0,24,36,48,60,72,84,96,108},
                                             {0,12,36,48,60,72,84,96,108}, {0,12,24,48,60,72,84,96,108},
                                             {0,12,24,36,60,72,84,96,108}, {0,12,24,36,48,72,84,96,108},
                                             {0,12,24,36,48,60,84,96,108}, {0,12,24,36,48,60,72,96,108},
                                             {0,12,24,36,48,60,72,84,108}, {0,12,24,36,48,60,72,84,96}};

    if (lut_numCnInCnGroups[7] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[7]*Z + 63)>>6;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[7]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG10[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<9; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG10[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 19 BNs

    // Offset is 4*384/32 = 12
    const uint16_t lut_idxCnProcG19[19][18] = {{48,96,144,192,240,288,336,384,432,480,528,576,624,672,720,768,816,864}, {0,96,144,192,240,288,336,384,432,480,528,576,624,672,720,768,816,864},
                                               {0,48,144,192,240,288,336,384,432,480,528,576,624,672,720,768,816,864}, {0,48,96,192,240,288,336,384,432,480,528,576,624,672,720,768,816,864},
                                               {0,48,96,144,240,288,336,384,432,480,528,576,624,672,720,768,816,864}, {0,48,96,144,192,288,336,384,432,480,528,576,624,672,720,768,816,864},
                                               {0,48,96,144,192,240,336,384,432,480,528,576,624,672,720,768,816,864}, {0,48,96,144,192,240,288,384,432,480,528,576,624,672,720,768,816,864},
                                               {0,48,96,144,192,240,288,336,432,480,528,576,624,672,720,768,816,864}, {0,48,96,144,192,240,288,336,384,480,528,576,624,672,720,768,816,864},
                                               {0,48,96,144,192,240,288,336,384,432,528,576,624,672,720,768,816,864}, {0,48,96,144,192,240,288,336,384,432,480,576,624,672,720,768,816,864},
                                               {0,48,96,144,192,240,288,336,384,432,480,528,624,672,720,768,816,864}, {0,48,96,144,192,240,288,336,384,432,480,528,576,672,720,768,816,864},
                                               {0,48,96,144,192,240,288,336,384,432,480,528,576,624,720,768,816,864}, {0,48,96,144,192,240,288,336,384,432,480,528,576,624,672,768,816,864},
                                               {0,48,96,144,192,240,288,336,384,432,480,528,576,624,672,720,816,864}, {0,48,96,144,192,240,288,336,384,432,480,528,576,624,672,720,768,864},
                                               {0,48,96,144,192,240,288,336,384,432,480,528,576,624,672,720,768,816}};

    if (lut_numCnInCnGroups[8] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[8]*Z + 63)>>6;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[8]*NR_LDPC_ZMAX)>>6;

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
                // Abs and sign of 32 CNs (first BN)
                zmm0 = p_cnProcBuf[(lut_idxCnProcG19[j][0]/2) + i];
                sgn  = _mm512_xor_si512(*p_ones, zmm0);
                min  = _mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<18; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG19[j][k]/2) + i];
                    min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
                   sgn  = _mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = _mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

}

#else
static inline void nrLDPC_cnProc_BG1(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;

    __m256i* p_cnProcBuf;
    __m256i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint32_t M;
    uint32_t i;
    uint32_t j;
    uint32_t k;
    // Offset to each bit within a group in terms of 32 Byte
    uint32_t bitOffsetInGroup;

    __m256i ymm0, min, sgn;
    __m256i* p_cnProcBufResBit;

    const __m256i* p_ones   = (__m256i*) ones256_epi8;
    const __m256i* p_maxLLR = (__m256i*) maxLLR256_epi8;

    // LUT with offsets for bits that need to be processed
    // 1. bit proc requires LLRs of 2. and 3. bit, 2.bits of 1. and 3. etc.
    // Offsets are in units of bitOffsetInGroup (1*384/32)
    const uint8_t lut_idxCnProcG3[3][2] = {{12,24}, {0,24}, {0,12}};

    // =====================================================================
    // Process group with 3 BNs

    if (lut_numCnInCnGroups[0] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[0]*Z + 31)>>5;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[0]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 3
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[0]];

        // Loop over every BN
        for (j=0; j<3; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // 32 CNs of second BN
                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][1] + i];
                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                sgn  = _mm256_sign_epi8(sgn, ymm0);


                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 4 BNs

    // Offset is 5*384/32 = 60
    const uint8_t lut_idxCnProcG4[4][3] = {{60,120,180}, {0,120,180}, {0,60,180}, {0,60,120}};

    if (lut_numCnInCnGroups[1] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[1]*Z + 31)>>5;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[1]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 4
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

        // Loop over every BN
        for (j=0; j<4; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG4[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<3; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG4[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 5 BNs

    // Offset is 18*384/32 = 216
    const uint16_t lut_idxCnProcG5[5][4] = {{216,432,648,864}, {0,432,648,864},
                                            {0,216,648,864}, {0,216,432,864}, {0,216,432,648}};

    if (lut_numCnInCnGroups[2] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[2]*Z + 31)>>5;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[2]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 5
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[2]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[2]];

        // Loop over every BN
        for (j=0; j<5; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG5[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<4; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG5[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 6 BNs

    // Offset is 8*384/32 = 96
    const uint16_t lut_idxCnProcG6[6][5] = {{96,192,288,384,480}, {0,192,288,384,480},
                                            {0,96,288,384,480}, {0,96,192,384,480},
                                            {0,96,192,288,480}, {0,96,192,288,384}};

    if (lut_numCnInCnGroups[3] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[3]*Z + 31)>>5;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[3]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 6
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[3]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[3]];

        // Loop over every BN
        for (j=0; j<6; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG6[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<5; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG6[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 7 BNs

    // Offset is 5*384/32 = 60
    const uint16_t lut_idxCnProcG7[7][6] = {{60,120,180,240,300,360}, {0,120,180,240,300,360},
                                            {0,60,180,240,300,360},   {0,60,120,240,300,360},
                                            {0,60,120,180,300,360},   {0,60,120,180,240,360},
                                            {0,60,120,180,240,300}};

    if (lut_numCnInCnGroups[4] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[4]*Z + 31)>>5;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[4]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 7
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[4]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[4]];

        // Loop over every BN
        for (j=0; j<7; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG7[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<6; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG7[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 8 BNs

    // Offset is 2*384/32 = 24
    const uint8_t lut_idxCnProcG8[8][7] = {{24,48,72,96,120,144,168}, {0,48,72,96,120,144,168},
                                           {0,24,72,96,120,144,168}, {0,24,48,96,120,144,168},
                                           {0,24,48,72,120,144,168}, {0,24,48,72,96,144,168},
                                           {0,24,48,72,96,120,168}, {0,24,48,72,96,120,144}};

    if (lut_numCnInCnGroups[5] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[5]*Z + 31)>>5;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[5]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 8
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[5]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[5]];

        // Loop over every BN
        for (j=0; j<8; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG8[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<7; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG8[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 9 BNs

    // Offset is 2*384/32 = 24
    const uint8_t lut_idxCnProcG9[9][8] = {{24,48,72,96,120,144,168,192}, {0,48,72,96,120,144,168,192},
                                           {0,24,72,96,120,144,168,192}, {0,24,48,96,120,144,168,192},
                                           {0,24,48,72,120,144,168,192}, {0,24,48,72,96,144,168,192},
                                           {0,24,48,72,96,120,168,192}, {0,24,48,72,96,120,144,192},
                                           {0,24,48,72,96,120,144,168}};

    if (lut_numCnInCnGroups[6] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[6]*Z + 31)>>5;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[6]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 9
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[6]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[6]];

        // Loop over every BN
        for (j=0; j<9; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG9[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<8; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG9[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 10 BNs

    // Offset is 1*384/32 = 12
    const uint8_t lut_idxCnProcG10[10][9] = {{12,24,36,48,60,72,84,96,108}, {0,24,36,48,60,72,84,96,108},
                                             {0,12,36,48,60,72,84,96,108}, {0,12,24,48,60,72,84,96,108},
                                             {0,12,24,36,60,72,84,96,108}, {0,12,24,36,48,72,84,96,108},
                                             {0,12,24,36,48,60,84,96,108}, {0,12,24,36,48,60,72,96,108},
                                             {0,12,24,36,48,60,72,84,108}, {0,12,24,36,48,60,72,84,96}};

    if (lut_numCnInCnGroups[7] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[7]*Z + 31)>>5;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[7]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 10
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[7]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[7]];

        // Loop over every BN
        for (j=0; j<10; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG10[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<9; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG10[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

    // =====================================================================
    // Process group with 19 BNs

    // Offset is 4*384/32 = 12
    const uint16_t lut_idxCnProcG19[19][18] = {{48,96,144,192,240,288,336,384,432,480,528,576,624,672,720,768,816,864}, {0,96,144,192,240,288,336,384,432,480,528,576,624,672,720,768,816,864},
                                               {0,48,144,192,240,288,336,384,432,480,528,576,624,672,720,768,816,864}, {0,48,96,192,240,288,336,384,432,480,528,576,624,672,720,768,816,864},
                                               {0,48,96,144,240,288,336,384,432,480,528,576,624,672,720,768,816,864}, {0,48,96,144,192,288,336,384,432,480,528,576,624,672,720,768,816,864},
                                               {0,48,96,144,192,240,336,384,432,480,528,576,624,672,720,768,816,864}, {0,48,96,144,192,240,288,384,432,480,528,576,624,672,720,768,816,864},
                                               {0,48,96,144,192,240,288,336,432,480,528,576,624,672,720,768,816,864}, {0,48,96,144,192,240,288,336,384,480,528,576,624,672,720,768,816,864},
                                               {0,48,96,144,192,240,288,336,384,432,528,576,624,672,720,768,816,864}, {0,48,96,144,192,240,288,336,384,432,480,576,624,672,720,768,816,864},
                                               {0,48,96,144,192,240,288,336,384,432,480,528,624,672,720,768,816,864}, {0,48,96,144,192,240,288,336,384,432,480,528,576,672,720,768,816,864},
                                               {0,48,96,144,192,240,288,336,384,432,480,528,576,624,720,768,816,864}, {0,48,96,144,192,240,288,336,384,432,480,528,576,624,672,768,816,864},
                                               {0,48,96,144,192,240,288,336,384,432,480,528,576,624,672,720,816,864}, {0,48,96,144,192,240,288,336,384,432,480,528,576,624,672,720,768,864},
                                               {0,48,96,144,192,240,288,336,384,432,480,528,576,624,672,720,768,816}};

    if (lut_numCnInCnGroups[8] > 0)
    {
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M = (lut_numCnInCnGroups[8]*Z + 31)>>5;

        // Set the offset to each bit within a group in terms of 32 Byte
        bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[8]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 19
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[8]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[8]];

        // Loop over every BN
        for (j=0; j<19; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
                ymm0 = p_cnProcBuf[lut_idxCnProcG19[j][0] + i];
                sgn  = _mm256_sign_epi8(*p_ones, ymm0);
                min  = _mm256_abs_epi8(ymm0);

                // Loop over BNs
                for (k=1; k<18; k++)
                {
                    ymm0 = p_cnProcBuf[lut_idxCnProcG19[j][k] + i];
                    min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
                    sgn  = _mm256_sign_epi8(sgn, ymm0);
                }

                // Store result
                min = _mm256_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
                p_cnProcBufResBit++;
            }
        }
    }

}
#endif
/**
   \brief Performs parity check for BG1 on the CN processing buffer. Stops as soon as error is detected.
   \param p_lut Pointer to decoder LUTs
   \param Z Lifting size
   \return 32-bit parity check indicator
*/
static inline uint32_t nrLDPC_cnProcPc_BG1(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;

    __m256i* p_cnProcBuf;
    __m256i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint32_t M;
    uint32_t i;
    uint32_t j;
    uint32_t pcRes = 0;
    uint32_t pcResSum = 0;
    uint32_t Mrem;
    uint32_t M32;

    __m256i ymm0, ymm1;

    // =====================================================================
    // Process group with 3 BNs

    if (lut_numCnInCnGroups[0] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[0]*Z;
        // Remainder modulo 32
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 3
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[0]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<3; j++)
            {
                // BN offset is units of (1*384/32) = 12
                ymm0 = p_cnProcBuf   [j*12 + i];
                ymm1 = p_cnProcBufRes[j*12 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<3; j++)
        {
            // BN offset is units of (1*384/32) = 12
            ymm0 = p_cnProcBuf   [j*12 + i];
            ymm1 = p_cnProcBufRes[j*12 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 4
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<4; j++)
            {
                // BN offset is units of 5*384/32 = 60
                ymm0 = p_cnProcBuf   [j*60 + i];
                ymm1 = p_cnProcBufRes[j*60 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<4; j++)
        {
            // BN offset is units of 5*384/32 = 60
            ymm0 = p_cnProcBuf   [j*60 + i];
            ymm1 = p_cnProcBufRes[j*60 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 5
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[2]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[2]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<5; j++)
            {
                // BN offset is units of 18*384/32 = 216
                ymm0 = p_cnProcBuf   [j*216 + i];
                ymm1 = p_cnProcBufRes[j*216 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;

        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<5; j++)
        {
            // BN offset is units of 18*384/32 = 216
            ymm0 = p_cnProcBuf   [j*216 + i];
            ymm1 = p_cnProcBufRes[j*216 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 6
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[3]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[3]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<6; j++)
            {
                // BN offset is units of 8*384/32 = 96
                ymm0 = p_cnProcBuf   [j*96 + i];
                ymm1 = p_cnProcBufRes[j*96 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<6; j++)
        {
            // BN offset is units of 8*384/32 = 96
            ymm0 = p_cnProcBuf   [j*96 + i];
            ymm1 = p_cnProcBufRes[j*96 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 7
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[4]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[4]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<7; j++)
            {
                // BN offset is units of 5*384/32 = 60
                ymm0 = p_cnProcBuf   [j*60 + i];
                ymm1 = p_cnProcBufRes[j*60 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<7; j++)
        {
            // BN offset is units of 5*384/32 = 60
            ymm0 = p_cnProcBuf   [j*60 + i];
            ymm1 = p_cnProcBufRes[j*60 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 8
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[5]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[5]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<8; j++)
            {
                // BN offset is units of 2*384/32 = 24
                ymm0 = p_cnProcBuf   [j*24 + i];
                ymm1 = p_cnProcBufRes[j*24 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<8; j++)
        {
            // BN offset is units of 2*384/32 = 24
            ymm0 = p_cnProcBuf   [j*24 + i];
            ymm1 = p_cnProcBufRes[j*24 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 9
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[6]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[6]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<9; j++)
            {
                // BN offset is units of 2*384/32 = 24
                ymm0 = p_cnProcBuf   [j*24 + i];
                ymm1 = p_cnProcBufRes[j*24 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<9; j++)
        {
            // BN offset is units of 2*384/32 = 24
            ymm0 = p_cnProcBuf   [j*24 + i];
            ymm1 = p_cnProcBufRes[j*24 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 10
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[7]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[7]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<10; j++)
            {
                // BN offset is units of 1*384/32 = 12
                ymm0 = p_cnProcBuf   [j*12 + i];
                ymm1 = p_cnProcBufRes[j*12 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<10; j++)
        {
            // BN offset is units of 1*384/32 = 12
            ymm0 = p_cnProcBuf   [j*12 + i];
            ymm1 = p_cnProcBufRes[j*12 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 19
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[8]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[8]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN (Last BN is connected to multiple CNs)
            // Compute PC for 32 CNs at once
            for (j=0; j<19; j++)
            {
                // BN offset is units of 4*384/32 = 48
                ymm0 = p_cnProcBuf   [j*48 + i];
                ymm1 = p_cnProcBufRes[j*48 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN (Last BN is connected to multiple CNs)
        // Compute PC for 32 CNs at once
        for (j=0; j<19; j++)
        {
            // BN offset is units of 4*384/32 = 48
            ymm0 = p_cnProcBuf   [j*48 + i];
            ymm1 = p_cnProcBufRes[j*48 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
static inline uint32_t nrLDPC_cnProcPc_BG2(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;

    __m256i* p_cnProcBuf;
    __m256i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint32_t M;
    uint32_t i;
    uint32_t j;
    uint32_t pcRes = 0;
    uint32_t pcResSum = 0;
    uint32_t Mrem;
    uint32_t M32;

    __m256i ymm0, ymm1;

    // =====================================================================
    // Process group with 3 BNs

    if (lut_numCnInCnGroups[0] > 0)
    {
        // Reset results
        pcResSum = 0;

        // Number of CNs in group
        M = lut_numCnInCnGroups[0]*Z;
        // Remainder modulo 32
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 3
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[0]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<3; j++)
            {
                // BN offset is units of (6*384/32) = 72
                ymm0 = p_cnProcBuf   [j*72 + i];
                ymm1 = p_cnProcBufRes[j*72 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<3; j++)
        {
            // BN offset is units of (6*384/32) = 72
            ymm0 = p_cnProcBuf   [j*72 + i];
            ymm1 = p_cnProcBufRes[j*72 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 4
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<4; j++)
            {
                // BN offset is units of 20*384/32 = 240
                ymm0 = p_cnProcBuf   [j*240 + i];
                ymm1 = p_cnProcBufRes[j*240 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<4; j++)
        {
            // BN offset is units of 20*384/32 = 240
            ymm0 = p_cnProcBuf   [j*240 + i];
            ymm1 = p_cnProcBufRes[j*240 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 5
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[2]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[2]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<5; j++)
            {
                // BN offset is units of 9*384/32 = 108
                ymm0 = p_cnProcBuf   [j*108 + i];
                ymm1 = p_cnProcBufRes[j*108 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<5; j++)
        {
            // BN offset is units of 9*384/32 = 108
            ymm0 = p_cnProcBuf   [j*108 + i];
            ymm1 = p_cnProcBufRes[j*108 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 6
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[3]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[3]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<6; j++)
            {
                // BN offset is units of 3*384/32 = 36
                ymm0 = p_cnProcBuf   [j*36 + i];
                ymm1 = p_cnProcBufRes[j*36 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<6; j++)
        {
            // BN offset is units of 3*384/32 = 36
            ymm0 = p_cnProcBuf   [j*36 + i];
            ymm1 = p_cnProcBufRes[j*36 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 8
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[4]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[4]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<8; j++)
            {
                // BN offset is units of 2*384/32 = 24
                ymm0 = p_cnProcBuf   [j*24 + i];
                ymm1 = p_cnProcBufRes[j*24 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<8; j++)
        {
            // BN offset is units of 2*384/32 = 24
            ymm0 = p_cnProcBuf   [j*24 + i];
            ymm1 = p_cnProcBufRes[j*24 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

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
        Mrem = M&31;
        // Number of groups of 32 CNs for parallel processing
        // Ceil for values not divisible by 32
        M32 = (M + 31)>>5;

        // Set pointers to start of group 10
        p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[5]];
        p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[5]];

        // Loop over CNs
        for (i=0; i<(M32-1); i++)
        {
            pcRes = 0;
            // Loop over every BN
            // Compute PC for 32 CNs at once
            for (j=0; j<10; j++)
            {
                // BN offset is units of 2*384/32 = 24
                ymm0 = p_cnProcBuf   [j*24 + i];
                ymm1 = p_cnProcBufRes[j*24 + i];

                // Add BN and input LLR, extract the sign bit
                // and add in GF(2) (xor)
                pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
            }

            // If no error pcRes should be 0
            pcResSum |= pcRes;
        }

        // Last 32 CNs might not be full valid 32 depending on Z
        pcRes = 0;
        // Loop over every BN
        // Compute PC for 32 CNs at once
        for (j=0; j<10; j++)
        {
            // BN offset is units of 2*384/32 = 24
            ymm0 = p_cnProcBuf   [j*24 + i];
            ymm1 = p_cnProcBufRes[j*24 + i];

            // Add BN and input LLR, extract the sign bit
            // and add in GF(2) (xor)
            pcRes ^= _mm256_movemask_epi8(_mm256_adds_epi8(ymm0,ymm1));
        }

        // If no error pcRes should be 0
        // Only use valid CNs
        pcResSum |= (pcRes&(0xFFFFFFFF>>(32-Mrem)));

        // If PC failed we can stop here
        if (pcResSum > 0)
        {
            return pcResSum;
        }
    }

    return pcResSum;
}

#endif



