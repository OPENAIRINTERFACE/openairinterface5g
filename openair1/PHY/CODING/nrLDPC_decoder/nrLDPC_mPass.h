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

/*!\file nrLDPC_mPass.h
 * \brief Defines the functions for message passing
 * \author Sebastian Wagner (TCL Communications) Email: <mailto:sebastian.wagner@tcl.com>
 * \date 27-03-2018
 * \version 1.0
 * \note
 * \warning
 */

#ifndef __NR_LDPC_MPASS__H__
#define __NR_LDPC_MPASS__H__

#include <string.h>
#include "nrLDPC_defs.h"

/**
   \brief Circular memcpy
   (src) str2 = |xxxxxxxxxxxxxxxxxxxx\--------|
                                      \
   (dst) str1 =               |--------xxxxxxxxxxxxxxxxxxxxx|
   \param str1 Pointer to the start of the destination buffer
   \param str2 Pointer to the source buffer
   \param Z Lifting size
   \param cshift Cyclic shift
*/
static inline void *nrLDPC_circ_memcpy(int8_t *str1, const int8_t *str2, uint16_t Z, uint16_t cshift)
{
    uint16_t rem = Z - cshift;
    memcpy(str1+cshift, str2    , rem);
    memcpy(str1       , str2+rem, cshift);

    //mexPrintf("memcpy(%p,%p,%d) | memcpy(%p,%p,%d) | rem = %d, cshift = %d\n", str1+cshift, str2, rem, str1,str2+rem,cshift,rem,cshift);
    return(str1);
}

/**
   \brief Circular memcpy
   (src) str2 = |xxxxxxxxxxxxxxxxxxxx\--------|
                                      \
   (dst) str1 =               |--------xxxxxxxxxxxxxxxxxxxxx|
   \param str1 Pointer to the start of the destination buffer
   \param str2 Pointer to the source buffer
   \param Z Lifting size
   \param cshift Cyclic shift
*/
static inline void *nrLDPC_inv_circ_memcpy(int8_t *str1, const int8_t *str2, uint16_t Z, uint16_t cshift)
{
    uint16_t rem = Z - cshift;
    memcpy(str1     , str2+cshift, rem);
    memcpy(str1+rem , str2       , cshift);

    return(str1);
}

/**
   \brief Copies the input LLRs to their corresponding place in the LLR processing buffer.
   \param p_lut Pointer to decoder LUTs
   \param llr Pointer to input LLRs
   \param Z Lifting size
   \param BG Base graph
*/
static inline void nrLDPC_llr2llrProcBuf(t_nrLDPC_lut* p_lut, int8_t* llr, t_nrLDPC_procBuf* p_procBuf, uint16_t Z, uint8_t BG)
{
    uint32_t i;
    const uint8_t numBn2CnG1 = p_lut->numBnInBnGroups[0];
    uint32_t colG1 = NR_LDPC_START_COL_PARITY_BG1*Z;

    const uint16_t* lut_llr2llrProcBufAddr = p_lut->llr2llrProcBufAddr;
    const uint8_t*  lut_llr2llrProcBufNumBn = p_lut->llr2llrProcBufNumBn;
    const uint8_t*  lut_llr2llrProcBufNumEl = p_lut->llr2llrProcBufNumEl;

    uint16_t numLlr = 0;
    int8_t* llrProcBuf = p_procBuf->llrProcBuf;

    if (BG == 2)
    {
        colG1 = NR_LDPC_START_COL_PARITY_BG2*Z;
    }

    // Copy LLRs connected to 1 CN
    if (numBn2CnG1 > 0)
    {
        memcpy(&llrProcBuf[0], &llr[colG1], numBn2CnG1*Z);
    }

    // First 2 columns might be set to zero directly if it's true they always belong to the groups with highest number of connected CNs...
    for (i=0; i<(*lut_llr2llrProcBufNumEl); i++)
    {
        numLlr = lut_llr2llrProcBufNumBn[i]*Z;
        memcpy(&llrProcBuf[lut_llr2llrProcBufAddr[i]], llr, numLlr);
        llr+=numLlr;
    }
}

/**
   \brief Copies the input LLRs to their corresponding place in the CN processing buffer.
   \param p_lut Pointer to decoder LUTs
   \param llr Pointer to input LLRs
   \param numLLR Number of LLR values
   \param Z Lifting size
   \param BG Base graph
*/
static inline void nrLDPC_llr2CnProcBuf(t_nrLDPC_lut* p_lut, int8_t* llr, t_nrLDPC_procBuf* p_procBuf, uint16_t numLLR, uint16_t Z, uint8_t BG)
{
    const uint32_t* lut_llr2CnProcBuf = p_lut->llr2CnProcBuf;
    const uint8_t* lut_numEdgesPerBn = p_lut->numEdgesPerBn;

    int8_t* cnProcBuf = p_procBuf->cnProcBuf;

    int8_t curLLR;
    uint8_t numEdges;
    uint32_t i;
    uint32_t j;
    uint32_t k;
    uint8_t startColParity = NR_LDPC_START_COL_PARITY_BG1;
    uint32_t colG1;
    const uint32_t* p_lutEntry = lut_llr2CnProcBuf;

    if (BG == 2)
    {
        startColParity = NR_LDPC_START_COL_PARITY_BG2;
    }
    colG1 = startColParity*Z;

    // BNs connected to more than 1 CN
    for (k=0; k<startColParity; k++)
    {
        numEdges = lut_numEdgesPerBn[k];

        for (i=0; i<Z; i++)
        {
            curLLR = llr[i];

            for (j=0; j<numEdges; j++)
            {
                cnProcBuf[*p_lutEntry++] = curLLR;
            }
        }
    }

    // Copy LLRs connected to 1 CN
    for (i=colG1; i<numLLR; i++)
    {
        cnProcBuf[*p_lutEntry++] = llr[i];
    }

}

/**
   \brief Copies the values in the CN processing results buffer to their corresponding place in the BN processing buffer for BG2.
   \param p_lut Pointer to decoder LUTs
   \param Z Lifting size
*/
static inline void nrLDPC_cn2bnProcBuf(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint32_t* lut_cn2bnProcBuf = p_lut->cn2bnProcBuf;
    const uint8_t*  lut_numCnInCnGroups = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;
    int8_t* bnProcBuf    = p_procBuf->bnProcBuf;

    const uint32_t* p_lut_cn2bn;
    int8_t* p_cnProcBufRes;
    uint32_t bitOffsetInGroup;
    uint32_t i;
    uint32_t j;
    uint32_t M;

    // =====================================================================
    // CN group with 3 BNs

    p_lut_cn2bn = &lut_cn2bnProcBuf[0];
    M = lut_numCnInCnGroups[0]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[0]*NR_LDPC_ZMAX;

    for (j=0; j<3; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[0] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
    }

    // =====================================================================
    // CN group with 4 BNs

    p_lut_cn2bn += (M*3); // Number of elements of previous group
    M = lut_numCnInCnGroups[1]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[1]*NR_LDPC_ZMAX;

    for (j=0; j<4; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[1] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
    }

    // =====================================================================
    // CN group with 5 BNs

    p_lut_cn2bn += (M*4); // Number of elements of previous group
    M = lut_numCnInCnGroups[2]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[2]*NR_LDPC_ZMAX;

    for (j=0; j<5; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[2] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
    }

    // =====================================================================
    // CN group with 6 BNs

    p_lut_cn2bn += (M*5); // Number of elements of previous group
    M = lut_numCnInCnGroups[3]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[3]*NR_LDPC_ZMAX;

    for (j=0; j<6; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[3] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
    }

    // =====================================================================
    // CN group with 8 BNs

    p_lut_cn2bn += (M*6); // Number of elements of previous group
    M = lut_numCnInCnGroups[4]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[4]*NR_LDPC_ZMAX;

    for (j=0; j<8; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[4] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
    }

    // =====================================================================
    // CN group with 10 BNs

    p_lut_cn2bn += (M*8); // Number of elements of previous group
    M = lut_numCnInCnGroups[5]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[5]*NR_LDPC_ZMAX;

    for (j=0; j<10; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[5] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
    }

}

/**
   \brief Copies the values in the CN processing results buffer to their corresponding place in the BN processing buffer for BG1.
   \param p_lut Pointer to decoder LUTs
   \param Z Lifting size
*/
static inline void nrLDPC_cn2bnProcBuf_BG1(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    //const uint32_t* lut_cn2bnProcBuf = p_lut->cn2bnProcBuf;
    const uint8_t*  lut_numCnInCnGroups = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    const uint16_t (*lut_circShift_CNG3) [lut_numCnInCnGroups_BG1_R13[0]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[0]]) p_lut->circShift[0];
    const uint16_t (*lut_circShift_CNG4) [lut_numCnInCnGroups_BG1_R13[1]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[1]]) p_lut->circShift[1];
    const uint16_t (*lut_circShift_CNG5) [lut_numCnInCnGroups_BG1_R13[2]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[2]]) p_lut->circShift[2];
    const uint16_t (*lut_circShift_CNG6) [lut_numCnInCnGroups_BG1_R13[3]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[3]]) p_lut->circShift[3];
    const uint16_t (*lut_circShift_CNG7) [lut_numCnInCnGroups_BG1_R13[4]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[4]]) p_lut->circShift[4];
    const uint16_t (*lut_circShift_CNG8) [lut_numCnInCnGroups_BG1_R13[5]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[5]]) p_lut->circShift[5];
    const uint16_t (*lut_circShift_CNG9) [lut_numCnInCnGroups_BG1_R13[6]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[6]]) p_lut->circShift[6];
    const uint16_t (*lut_circShift_CNG10)[lut_numCnInCnGroups_BG1_R13[7]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[7]]) p_lut->circShift[7];
    const uint16_t (*lut_circShift_CNG19)[lut_numCnInCnGroups_BG1_R13[8]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[8]]) p_lut->circShift[8];

    const uint32_t (*lut_startAddrBnProcBuf_CNG3) [lut_numCnInCnGroups[0]] = (uint32_t(*)[lut_numCnInCnGroups[0]]) p_lut->startAddrBnProcBuf[0];
    const uint32_t (*lut_startAddrBnProcBuf_CNG4) [lut_numCnInCnGroups[1]] = (uint32_t(*)[lut_numCnInCnGroups[1]]) p_lut->startAddrBnProcBuf[1];
    const uint32_t (*lut_startAddrBnProcBuf_CNG5) [lut_numCnInCnGroups[2]] = (uint32_t(*)[lut_numCnInCnGroups[2]]) p_lut->startAddrBnProcBuf[2];
    const uint32_t (*lut_startAddrBnProcBuf_CNG6) [lut_numCnInCnGroups[3]] = (uint32_t(*)[lut_numCnInCnGroups[3]]) p_lut->startAddrBnProcBuf[3];
    const uint32_t (*lut_startAddrBnProcBuf_CNG7) [lut_numCnInCnGroups[4]] = (uint32_t(*)[lut_numCnInCnGroups[4]]) p_lut->startAddrBnProcBuf[4];
    const uint32_t (*lut_startAddrBnProcBuf_CNG8) [lut_numCnInCnGroups[5]] = (uint32_t(*)[lut_numCnInCnGroups[5]]) p_lut->startAddrBnProcBuf[5];
    const uint32_t (*lut_startAddrBnProcBuf_CNG9) [lut_numCnInCnGroups[6]] = (uint32_t(*)[lut_numCnInCnGroups[6]]) p_lut->startAddrBnProcBuf[6];
    const uint32_t (*lut_startAddrBnProcBuf_CNG10)[lut_numCnInCnGroups[7]] = (uint32_t(*)[lut_numCnInCnGroups[7]]) p_lut->startAddrBnProcBuf[7];
    const uint32_t (*lut_startAddrBnProcBuf_CNG19)[lut_numCnInCnGroups[8]] = (uint32_t(*)[lut_numCnInCnGroups[8]]) p_lut->startAddrBnProcBuf[8];

    //const uint8_t (*lut_bnPosBnProcBuf_CNG3) [lut_numCnInCnGroups[0]] = (uint8_t(*)[lut_numCnInCnGroups[0]]) p_lut->bnPosBnProcBuf[0];
    const uint8_t (*lut_bnPosBnProcBuf_CNG4) [lut_numCnInCnGroups[1]] = (uint8_t(*)[lut_numCnInCnGroups[1]]) p_lut->bnPosBnProcBuf[1];
    const uint8_t (*lut_bnPosBnProcBuf_CNG5) [lut_numCnInCnGroups[2]] = (uint8_t(*)[lut_numCnInCnGroups[2]]) p_lut->bnPosBnProcBuf[2];
    const uint8_t (*lut_bnPosBnProcBuf_CNG6) [lut_numCnInCnGroups[3]] = (uint8_t(*)[lut_numCnInCnGroups[3]]) p_lut->bnPosBnProcBuf[3];
    const uint8_t (*lut_bnPosBnProcBuf_CNG7) [lut_numCnInCnGroups[4]] = (uint8_t(*)[lut_numCnInCnGroups[4]]) p_lut->bnPosBnProcBuf[4];
    const uint8_t (*lut_bnPosBnProcBuf_CNG8) [lut_numCnInCnGroups[5]] = (uint8_t(*)[lut_numCnInCnGroups[5]]) p_lut->bnPosBnProcBuf[5];
    const uint8_t (*lut_bnPosBnProcBuf_CNG9) [lut_numCnInCnGroups[6]] = (uint8_t(*)[lut_numCnInCnGroups[6]]) p_lut->bnPosBnProcBuf[6];
    const uint8_t (*lut_bnPosBnProcBuf_CNG10)[lut_numCnInCnGroups[7]] = (uint8_t(*)[lut_numCnInCnGroups[7]]) p_lut->bnPosBnProcBuf[7];
    const uint8_t (*lut_bnPosBnProcBuf_CNG19)[lut_numCnInCnGroups[8]] = (uint8_t(*)[lut_numCnInCnGroups[8]]) p_lut->bnPosBnProcBuf[8];

    int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;
    int8_t* bnProcBuf    = p_procBuf->bnProcBuf;

    //const uint32_t* p_lut_cn2bn;
    int8_t* p_cnProcBufRes;
    uint32_t bitOffsetInGroup;
    uint32_t i;
    uint32_t j;
    //uint32_t M;
    uint32_t idxBn = 0;

    // =====================================================================
    // CN group with 3 BNs

    //p_lut_cn2bn = &lut_cn2bnProcBuf[0];
    //M = lut_numCnInCnGroups[0]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[0]*NR_LDPC_ZMAX;

    for (j=0; j<3; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[0] + j*bitOffsetInGroup];

        nrLDPC_circ_memcpy(&bnProcBuf[lut_startAddrBnProcBuf_CNG3[j][0]],p_cnProcBufRes,Z,lut_circShift_CNG3[j][0]);
    }

    // =====================================================================
    // CN group with 4 BNs

    //p_lut_cn2bn += (M*3); // Number of elements of previous group
    //M = lut_numCnInCnGroups[1]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[1]*NR_LDPC_ZMAX;

    for (j=0; j<4; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[1] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[1]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG4[j][i] + lut_bnPosBnProcBuf_CNG4[j][i]*Z;
            nrLDPC_circ_memcpy(&bnProcBuf[idxBn],p_cnProcBufRes,Z,lut_circShift_CNG4[j][i]);
            p_cnProcBufRes += Z;
        }
    }

    // =====================================================================
    // CN group with 5 BNs

    //p_lut_cn2bn += (M*4); // Number of elements of previous group
    //M = lut_numCnInCnGroups[2]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[2]*NR_LDPC_ZMAX;

    for (j=0; j<5; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[2] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[2]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG5[j][i] + lut_bnPosBnProcBuf_CNG5[j][i]*Z;
            nrLDPC_circ_memcpy(&bnProcBuf[idxBn],p_cnProcBufRes,Z,lut_circShift_CNG5[j][i]);
            p_cnProcBufRes += Z;
        }
    }

    // =====================================================================
    // CN group with 6 BNs

    //p_lut_cn2bn += (M*5); // Number of elements of previous group
    //M = lut_numCnInCnGroups[3]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[3]*NR_LDPC_ZMAX;

    for (j=0; j<6; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[3] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[3]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG6[j][i] + lut_bnPosBnProcBuf_CNG6[j][i]*Z;
            nrLDPC_circ_memcpy(&bnProcBuf[idxBn],p_cnProcBufRes,Z,lut_circShift_CNG6[j][i]);
            p_cnProcBufRes += Z;
        }
    }

    // =====================================================================
    // CN group with 7 BNs

    //p_lut_cn2bn += (M*6); // Number of elements of previous group
    //M = lut_numCnInCnGroups[4]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[4]*NR_LDPC_ZMAX;

    for (j=0; j<7; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[4] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[4]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG7[j][i] + lut_bnPosBnProcBuf_CNG7[j][i]*Z;
            nrLDPC_circ_memcpy(&bnProcBuf[idxBn],p_cnProcBufRes,Z,lut_circShift_CNG7[j][i]);
            p_cnProcBufRes += Z;
        }
    }

    // =====================================================================
    // CN group with 8 BNs

    //p_lut_cn2bn += (M*7); // Number of elements of previous group
    //M = lut_numCnInCnGroups[5]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[5]*NR_LDPC_ZMAX;

    for (j=0; j<8; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[5] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[5]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG8[j][i] + lut_bnPosBnProcBuf_CNG8[j][i]*Z;
            nrLDPC_circ_memcpy(&bnProcBuf[idxBn],p_cnProcBufRes,Z,lut_circShift_CNG8[j][i]);
            p_cnProcBufRes += Z;
        }
    }

    // =====================================================================
    // CN group with 9 BNs

    //p_lut_cn2bn += (M*8); // Number of elements of previous group
    //M = lut_numCnInCnGroups[6]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[6]*NR_LDPC_ZMAX;

    for (j=0; j<9; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[6] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[6]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG9[j][i] + lut_bnPosBnProcBuf_CNG9[j][i]*Z;
            nrLDPC_circ_memcpy(&bnProcBuf[idxBn],p_cnProcBufRes,Z,lut_circShift_CNG9[j][i]);
            p_cnProcBufRes += Z;
        }
    }

    // =====================================================================
    // CN group with 10 BNs

    //p_lut_cn2bn += (M*9); // Number of elements of previous group
    //M = lut_numCnInCnGroups[7]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[7]*NR_LDPC_ZMAX;

    for (j=0; j<10; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[7] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[7]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG10[j][i] + lut_bnPosBnProcBuf_CNG10[j][i]*Z;
            nrLDPC_circ_memcpy(&bnProcBuf[idxBn],p_cnProcBufRes,Z,lut_circShift_CNG10[j][i]);
            p_cnProcBufRes += Z;
        }
    }

    // =====================================================================
    // CN group with 19 BNs

    //p_lut_cn2bn += (M*10); // Number of elements of previous group
    //M = lut_numCnInCnGroups[8]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[8]*NR_LDPC_ZMAX;

    for (j=0; j<19; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[8] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[8]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG19[j][i] + lut_bnPosBnProcBuf_CNG19[j][i]*Z;
            nrLDPC_circ_memcpy(&bnProcBuf[idxBn],p_cnProcBufRes,Z,lut_circShift_CNG19[j][i]);
            p_cnProcBufRes += Z;
        }
    }

}

/**
   \brief Copies the values in the BN processing results buffer to their corresponding place in the CN processing buffer for BG2.
   \param p_lut Pointer to decoder LUTs
   \param Z Lifting size
*/
static inline void nrLDPC_bn2cnProcBuf(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
    const uint32_t* lut_cn2bnProcBuf = p_lut->cn2bnProcBuf;
    const uint8_t*  lut_numCnInCnGroups = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf = p_procBuf->cnProcBuf;
    int8_t* bnProcBufRes = p_procBuf->bnProcBufRes;

    int8_t* p_cnProcBuf;
    const uint32_t* p_lut_cn2bn;
    uint32_t bitOffsetInGroup;
    uint32_t i;
    uint32_t j;
    uint32_t M;

    // For CN groups 3 to 6 no need to send the last BN back since it's single edge
    // and BN processing does not change the value already in the CN proc buf

    // =====================================================================
    // CN group with 3 BNs

    p_lut_cn2bn = &lut_cn2bnProcBuf[0];
    M = lut_numCnInCnGroups[0]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[0]*NR_LDPC_ZMAX;

    for (j=0; j<2; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[0] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

    // =====================================================================
    // CN group with 4 BNs

    p_lut_cn2bn += (M*3); // Number of elements of previous group
    M = lut_numCnInCnGroups[1]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[1]*NR_LDPC_ZMAX;

    for (j=0; j<3; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[1] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

    // =====================================================================
    // CN group with 5 BNs

    p_lut_cn2bn += (M*4); // Number of elements of previous group
    M = lut_numCnInCnGroups[2]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[2]*NR_LDPC_ZMAX;

    for (j=0; j<4; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[2] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

    // =====================================================================
    // CN group with 6 BNs

    p_lut_cn2bn += (M*5); // Number of elements of previous group
    M = lut_numCnInCnGroups[3]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[3]*NR_LDPC_ZMAX;

    for (j=0; j<5; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[3] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

    // =====================================================================
    // CN group with 8 BNs

    p_lut_cn2bn += (M*6); // Number of elements of previous group
    M = lut_numCnInCnGroups[4]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[4]*NR_LDPC_ZMAX;

    for (j=0; j<8; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[4] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

    // =====================================================================
    // CN group with 10 BNs

    p_lut_cn2bn += (M*8); // Number of elements of previous group
    M = lut_numCnInCnGroups[5]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG2_R15[5]*NR_LDPC_ZMAX;

    for (j=0; j<10; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[5] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

}

/**
   \brief Copies the values in the BN processing results buffer to their corresponding place in the CN processing buffer for BG1.
   \param p_lut Pointer to decoder LUTs
   \param Z Lifting size
*/
static inline void nrLDPC_bn2cnProcBuf_BG1(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z)
{
//    const uint32_t* lut_cn2bnProcBuf = p_lut->cn2bnProcBuf;
    const uint8_t*  lut_numCnInCnGroups = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    const uint16_t (*lut_circShift_CNG3) [lut_numCnInCnGroups_BG1_R13[0]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[0]]) p_lut->circShift[0];
    const uint16_t (*lut_circShift_CNG4) [lut_numCnInCnGroups_BG1_R13[1]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[1]]) p_lut->circShift[1];
    const uint16_t (*lut_circShift_CNG5) [lut_numCnInCnGroups_BG1_R13[2]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[2]]) p_lut->circShift[2];
    const uint16_t (*lut_circShift_CNG6) [lut_numCnInCnGroups_BG1_R13[3]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[3]]) p_lut->circShift[3];
    const uint16_t (*lut_circShift_CNG7) [lut_numCnInCnGroups_BG1_R13[4]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[4]]) p_lut->circShift[4];
    const uint16_t (*lut_circShift_CNG8) [lut_numCnInCnGroups_BG1_R13[5]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[5]]) p_lut->circShift[5];
    const uint16_t (*lut_circShift_CNG9) [lut_numCnInCnGroups_BG1_R13[6]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[6]]) p_lut->circShift[6];
    const uint16_t (*lut_circShift_CNG10)[lut_numCnInCnGroups_BG1_R13[7]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[7]]) p_lut->circShift[7];
    const uint16_t (*lut_circShift_CNG19)[lut_numCnInCnGroups_BG1_R13[8]] = (uint16_t(*)[lut_numCnInCnGroups_BG1_R13[8]]) p_lut->circShift[8];

    const uint32_t (*lut_startAddrBnProcBuf_CNG3) [lut_numCnInCnGroups[0]] = (uint32_t(*)[lut_numCnInCnGroups[0]]) p_lut->startAddrBnProcBuf[0];
    const uint32_t (*lut_startAddrBnProcBuf_CNG4) [lut_numCnInCnGroups[1]] = (uint32_t(*)[lut_numCnInCnGroups[1]]) p_lut->startAddrBnProcBuf[1];
    const uint32_t (*lut_startAddrBnProcBuf_CNG5) [lut_numCnInCnGroups[2]] = (uint32_t(*)[lut_numCnInCnGroups[2]]) p_lut->startAddrBnProcBuf[2];
    const uint32_t (*lut_startAddrBnProcBuf_CNG6) [lut_numCnInCnGroups[3]] = (uint32_t(*)[lut_numCnInCnGroups[3]]) p_lut->startAddrBnProcBuf[3];
    const uint32_t (*lut_startAddrBnProcBuf_CNG7) [lut_numCnInCnGroups[4]] = (uint32_t(*)[lut_numCnInCnGroups[4]]) p_lut->startAddrBnProcBuf[4];
    const uint32_t (*lut_startAddrBnProcBuf_CNG8) [lut_numCnInCnGroups[5]] = (uint32_t(*)[lut_numCnInCnGroups[5]]) p_lut->startAddrBnProcBuf[5];
    const uint32_t (*lut_startAddrBnProcBuf_CNG9) [lut_numCnInCnGroups[6]] = (uint32_t(*)[lut_numCnInCnGroups[6]]) p_lut->startAddrBnProcBuf[6];
    const uint32_t (*lut_startAddrBnProcBuf_CNG10)[lut_numCnInCnGroups[7]] = (uint32_t(*)[lut_numCnInCnGroups[7]]) p_lut->startAddrBnProcBuf[7];
    const uint32_t (*lut_startAddrBnProcBuf_CNG19)[lut_numCnInCnGroups[8]] = (uint32_t(*)[lut_numCnInCnGroups[8]]) p_lut->startAddrBnProcBuf[8];

//    const uint8_t (*lut_bnPosBnProcBuf_CNG3) [lut_numCnInCnGroups[0]] = (uint8_t(*)[lut_numCnInCnGroups[0]]) p_lut->bnPosBnProcBuf[0];
    const uint8_t (*lut_bnPosBnProcBuf_CNG4) [lut_numCnInCnGroups[1]] = (uint8_t(*)[lut_numCnInCnGroups[1]]) p_lut->bnPosBnProcBuf[1];
    const uint8_t (*lut_bnPosBnProcBuf_CNG5) [lut_numCnInCnGroups[2]] = (uint8_t(*)[lut_numCnInCnGroups[2]]) p_lut->bnPosBnProcBuf[2];
    const uint8_t (*lut_bnPosBnProcBuf_CNG6) [lut_numCnInCnGroups[3]] = (uint8_t(*)[lut_numCnInCnGroups[3]]) p_lut->bnPosBnProcBuf[3];
    const uint8_t (*lut_bnPosBnProcBuf_CNG7) [lut_numCnInCnGroups[4]] = (uint8_t(*)[lut_numCnInCnGroups[4]]) p_lut->bnPosBnProcBuf[4];
    const uint8_t (*lut_bnPosBnProcBuf_CNG8) [lut_numCnInCnGroups[5]] = (uint8_t(*)[lut_numCnInCnGroups[5]]) p_lut->bnPosBnProcBuf[5];
    const uint8_t (*lut_bnPosBnProcBuf_CNG9) [lut_numCnInCnGroups[6]] = (uint8_t(*)[lut_numCnInCnGroups[6]]) p_lut->bnPosBnProcBuf[6];
    const uint8_t (*lut_bnPosBnProcBuf_CNG10)[lut_numCnInCnGroups[7]] = (uint8_t(*)[lut_numCnInCnGroups[7]]) p_lut->bnPosBnProcBuf[7];
    const uint8_t (*lut_bnPosBnProcBuf_CNG19)[lut_numCnInCnGroups[8]] = (uint8_t(*)[lut_numCnInCnGroups[8]]) p_lut->bnPosBnProcBuf[8];

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* bnProcBufRes = p_procBuf->bnProcBufRes;

    int8_t* p_cnProcBuf;
//    const uint32_t* p_lut_cn2bn;
    uint32_t bitOffsetInGroup;
    uint32_t i;
    uint32_t j;
//    uint32_t M;
    uint32_t idxBn = 0;

    // For CN groups 3 to 19 no need to send the last BN back since it's single edge
    // and BN processing does not change the value already in the CN proc buf

    // =====================================================================
    // CN group with 3 BNs

//    p_lut_cn2bn = &lut_cn2bnProcBuf[0];
    //   M = lut_numCnInCnGroups[0]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[0]*NR_LDPC_ZMAX;

    for (j=0; j<2; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[0] + j*bitOffsetInGroup];

        nrLDPC_inv_circ_memcpy(p_cnProcBuf, &bnProcBufRes[lut_startAddrBnProcBuf_CNG3[j][0]], Z, lut_circShift_CNG3[j][0]);
    }

    // =====================================================================
    // CN group with 4 BNs

    //p_lut_cn2bn += (M*3); // Number of elements of previous group
    //M = lut_numCnInCnGroups[1]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[1]*NR_LDPC_ZMAX;

    for (j=0; j<3; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[1] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[1]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG4[j][i] + lut_bnPosBnProcBuf_CNG4[j][i]*Z;
            nrLDPC_inv_circ_memcpy(p_cnProcBuf, &bnProcBufRes[idxBn], Z, lut_circShift_CNG4[j][i]);
            p_cnProcBuf += Z;
        }
    }

    // =====================================================================
    // CN group with 5 BNs

    //p_lut_cn2bn += (M*4); // Number of elements of previous group
    //M = lut_numCnInCnGroups[2]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[2]*NR_LDPC_ZMAX;

    for (j=0; j<4; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[2] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[2]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG5[j][i] + lut_bnPosBnProcBuf_CNG5[j][i]*Z;
            nrLDPC_inv_circ_memcpy(p_cnProcBuf, &bnProcBufRes[idxBn], Z, lut_circShift_CNG5[j][i]);
            p_cnProcBuf += Z;
        }

    }

    // =====================================================================
    // CN group with 6 BNs

    //p_lut_cn2bn += (M*5); // Number of elements of previous group
    //M = lut_numCnInCnGroups[3]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[3]*NR_LDPC_ZMAX;

    for (j=0; j<5; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[3] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[3]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG6[j][i] + lut_bnPosBnProcBuf_CNG6[j][i]*Z;
            nrLDPC_inv_circ_memcpy(p_cnProcBuf, &bnProcBufRes[idxBn], Z, lut_circShift_CNG6[j][i]);
            p_cnProcBuf += Z;
        }
    }

    // =====================================================================
    // CN group with 7 BNs

    //p_lut_cn2bn += (M*6); // Number of elements of previous group
    //M = lut_numCnInCnGroups[4]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[4]*NR_LDPC_ZMAX;

    for (j=0; j<6; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[4] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[4]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG7[j][i] + lut_bnPosBnProcBuf_CNG7[j][i]*Z;
            nrLDPC_inv_circ_memcpy(p_cnProcBuf, &bnProcBufRes[idxBn], Z, lut_circShift_CNG7[j][i]);
            p_cnProcBuf += Z;
        }
    }

    // =====================================================================
    // CN group with 8 BNs

    //p_lut_cn2bn += (M*7); // Number of elements of previous group
    //M = lut_numCnInCnGroups[5]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[5]*NR_LDPC_ZMAX;

    for (j=0; j<7; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[5] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[5]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG8[j][i] + lut_bnPosBnProcBuf_CNG8[j][i]*Z;
            nrLDPC_inv_circ_memcpy(p_cnProcBuf, &bnProcBufRes[idxBn], Z, lut_circShift_CNG8[j][i]);
            p_cnProcBuf += Z;
        }
    }

    // =====================================================================
    // CN group with 9 BNs

    //p_lut_cn2bn += (M*8); // Number of elements of previous group
    //M = lut_numCnInCnGroups[6]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[6]*NR_LDPC_ZMAX;

    for (j=0; j<8; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[6] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[6]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG9[j][i] + lut_bnPosBnProcBuf_CNG9[j][i]*Z;
            nrLDPC_inv_circ_memcpy(p_cnProcBuf, &bnProcBufRes[idxBn], Z, lut_circShift_CNG9[j][i]);
            p_cnProcBuf += Z;
        }
    }

    // =====================================================================
    // CN group with 10 BNs

    //p_lut_cn2bn += (M*9); // Number of elements of previous group
    //M = lut_numCnInCnGroups[7]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[7]*NR_LDPC_ZMAX;

    for (j=0; j<9; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[7] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[7]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG10[j][i] + lut_bnPosBnProcBuf_CNG10[j][i]*Z;
            nrLDPC_inv_circ_memcpy(p_cnProcBuf, &bnProcBufRes[idxBn], Z, lut_circShift_CNG10[j][i]);
            p_cnProcBuf += Z;
        }
    }

    // =====================================================================
    // CN group with 19 BNs

    //p_lut_cn2bn += (M*10); // Number of elements of previous group
    //M = lut_numCnInCnGroups[8]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[8]*NR_LDPC_ZMAX;

    for (j=0; j<19; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[8] + j*bitOffsetInGroup];

        for (i=0; i<lut_numCnInCnGroups[8]; i++)
        {
            idxBn = lut_startAddrBnProcBuf_CNG19[j][i] + lut_bnPosBnProcBuf_CNG19[j][i]*Z;
            nrLDPC_inv_circ_memcpy(p_cnProcBuf, &bnProcBufRes[idxBn], Z, lut_circShift_CNG19[j][i]);
            p_cnProcBuf += Z;
        }
    }

}

/**
   \brief Copies the values in the LLR results buffer to their corresponding place in the output LLR vector.
   \param p_lut Pointer to decoder LUTs
   \param llrOut Pointer to output LLRs
   \param numLLR Number of LLR values
*/
static inline void nrLDPC_llrRes2llrOut(t_nrLDPC_lut* p_lut, int8_t* llrOut, t_nrLDPC_procBuf* p_procBuf, uint16_t numLLR)
{
    const uint16_t* lut_llr2llrProcBuf = p_lut->llr2llrProcBuf;
    uint32_t i;

    int8_t* llrRes = p_procBuf->llrRes;

    for (i=0; i<numLLR; i++)
    {
        llrOut[i] = llrRes[lut_llr2llrProcBuf[i]];
    }
}

#endif
