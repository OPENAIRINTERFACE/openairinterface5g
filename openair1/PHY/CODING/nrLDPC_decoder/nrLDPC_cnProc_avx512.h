
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

/*!\file nrLDPC_cnProc_avx512.h
 * \brief Defines the functions for check node processing
 * \author Sebastian Wagner (TCL Communications) Email: <mailto:sebastian.wagner@tcl.com>
 * \date 30-09-2021
 * \version 1.0
 * \note
 * \warning
 */

#ifndef __NR_LDPC_CNPROC__H__
#define __NR_LDPC_CNPROC__H__
#include <simde/x86/avx512.h>
#define conditional_negate(a, b, z) simde_mm512_mask_sub_epi8(a, simde_mm512_movepi8_mask(b), z, a)
static inline void nrLDPC_cnProc_BG2_AVX512(t_nrLDPC_lut* p_lut, int8_t* cnProcBuf, int8_t* cnProcBufRes, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    simde__m512i* p_cnProcBuf;
    simde__m512i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint32_t M;
    uint32_t i;
    uint32_t j;
    uint32_t k;
    // Offset to each bit within a group in terms of 32 Byte
    uint32_t bitOffsetInGroup;

    simde__m512i zmm0, min, sgn, zeros;
    zeros = simde_mm512_setzero_si512();
    //     maxLLR =simde_mm512_set1_epi8((char)127);
    simde__m512i* p_cnProcBufResBit;

    const simde__m512i* p_ones = (simde__m512i*)ones512_epi8;
    const simde__m512i* p_maxLLR = (simde__m512i*)maxLLR512_epi8;

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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[0]];

        // Loop over every BN
        for (j=0; j<3; j++)
        {
            // Set of results pointer to correct BN address
            p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

            simde__m512i* pj0 = &p_cnProcBuf[(lut_idxCnProcG3[j][0] / 2)];
            simde__m512i* pj1 = &p_cnProcBuf[(lut_idxCnProcG3[j][1] / 2)];

            // Loop over CNs
            for (i=0; i<M; i++)
            {
                // Abs and sign of 32 CNs (first BN)
              //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
                zmm0 = pj0[i];
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // 32 CNs of second BN
                //  zmm0 = p_cnProcBuf[(lut_idxCnProcG3[j][1]/2) + i];
                zmm0 = pj1[i];
                min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                sgn = simde_mm512_xor_si512(sgn, zmm0);

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
                // p_cnProcBufResBit[i]=simde_mm512_sign_epi8(min, sgn);
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[1]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[1]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<3; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG4[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[2]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[2]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<4; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG5[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[3]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[3]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<5; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG6[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[4]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[4]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<7; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG8[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[5]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[5]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<9; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG10[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

}

static inline void nrLDPC_cnProc_BG1_AVX512(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint8_t*  lut_numCnInCnGroups   = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;

    simde__m512i* p_cnProcBuf;
    simde__m512i* p_cnProcBufRes;

    // Number of CNs in Groups
    uint32_t M;
    uint32_t i;
    uint32_t j;
    uint32_t k;
    // Offset to each bit within a group in terms of 32 Byte
    uint32_t bitOffsetInGroup;

    simde__m512i zmm0, min, sgn, zeros;

    zeros = simde_mm512_setzero_si512();
    // maxLLR =simde_mm512_set1_epi8((char)127);
    simde__m512i* p_cnProcBufResBit;

    const simde__m512i* p_ones = (simde__m512i*)ones512_epi8;
    const simde__m512i* p_maxLLR = (simde__m512i*)maxLLR512_epi8;

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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[0]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[0]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // 32 CNs of second BN
                zmm0 = p_cnProcBuf[(lut_idxCnProcG3[j][1]/2) + i];
                min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                sgn = simde_mm512_xor_si512(sgn, zmm0);

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[1]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[1]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<3; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG4[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[2]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[2]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<4; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG5[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[3]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[3]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<5; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG6[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[4]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[4]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<6; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG7[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[5]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[5]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<7; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG8[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[6]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[6]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<8; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG9[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[7]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[7]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<9; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG10[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
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
        p_cnProcBuf = (simde__m512i*)&cnProcBuf[lut_startAddrCnGroups[8]];
        p_cnProcBufRes = (simde__m512i*)&cnProcBufRes[lut_startAddrCnGroups[8]];

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
                sgn = simde_mm512_xor_si512(*p_ones, zmm0);
                min = simde_mm512_abs_epi8(zmm0);

                // Loop over BNs
                for (k=1; k<18; k++)
                {
                    zmm0 = p_cnProcBuf[(lut_idxCnProcG19[j][k]/2) + i];
                    min = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
                    sgn = simde_mm512_xor_si512(sgn, zmm0);
                }

                // Store result
                min = simde_mm512_min_epu8(min, *p_maxLLR); // 128 in epi8 is -127
                *p_cnProcBufResBit = conditional_negate(min, sgn,zeros);
                p_cnProcBufResBit++;
            }
        }
    }

}
#endif
