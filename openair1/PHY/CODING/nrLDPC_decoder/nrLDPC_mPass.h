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
   \brief Copies the input LLRs to their corresponding place in the LLR processing buffer.
   \param p_lut Pointer to decoder LUTs
   \param llr Pointer to input LLRs
   \param Z Lifting size
   \param BG Base graph
*/
static inline void nrLDPC_llr2llrProcBuf(t_nrLDPC_lut* p_lut, int8_t* llr, t_nrLDPC_procBuf* p_procBuf, uint16_t Z, uint8_t BG)
{
    const uint16_t* lut_llr2llrProcBuf = p_lut->llr2llrProcBuf;
    uint32_t i;
    const uint8_t numBn2CnG1 = p_lut->numBnInBnGroups[0];
    uint32_t colG1 = NR_LDPC_START_COL_PARITY_BG1*Z;

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
    for (i=0; i<colG1; i++)
    {
        llrProcBuf[lut_llr2llrProcBuf[i]] = llr[i];
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
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[0]*NR_LDPC_ZMAX;

    const uint32_t startAddrBnProcBuf_CNG3[3] = {111360, 100224, 0};
    const uint16_t cyclicShiftValues_Z384_CNG3[3] = {332, 181, 0};
    
    for (j=0; j<3; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[0] + j*bitOffsetInGroup];

        /*
        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
        */

        nrLDPC_circ_memcpy(&bnProcBuf[startAddrBnProcBuf_CNG3[j]],p_cnProcBufRes,Z,cyclicShiftValues_Z384_CNG3[j]);
    }

    // =====================================================================
    // CN group with 4 BNs

    p_lut_cn2bn += (M*3); // Number of elements of previous group
    M = lut_numCnInCnGroups[1]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[1]*NR_LDPC_ZMAX;

    const uint32_t startAddrBnProcBuf_CNG4[4][5]     = {{105984, 107904, 120192, 120576, 109440}, {40704, 74880, 43392, 47232, 43008}, {42240, 19200, 62592, 23424, 92928}, {8832, 12672, 13824, 14592, 15744}};

    const uint16_t cyclicShiftValues_Z384_CNG4[4][5] = {{194, 269, 175, 113, 135}, {194, 82, 37, 14, 149}, {101, 115, 312, 218, 15}, {0, 0, 0, 0, 0}};
    //const uint16_t cyclicShiftValues_Z384_CNG4[4][5] = {{175, 113, 194, 269, 135}, {14, 194, 149, 101, 37}, {15, 82, 312, 115, 218}, {0, 0, 0, 0, 0}};
    
    for (j=0; j<4; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[1] + j*bitOffsetInGroup];

        /*
        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
        */

        for (i=0; i<lut_numCnInCnGroups_BG1_R13[1]; i++)
        {
            nrLDPC_circ_memcpy(&bnProcBuf[startAddrBnProcBuf_CNG4[j][i]],p_cnProcBufRes,Z,cyclicShiftValues_Z384_CNG4[j][i]);
            p_cnProcBufRes+=NR_LDPC_ZMAX;
        }

    }

    // =====================================================================
    // CN group with 5 BNs

    p_lut_cn2bn += (M*4); // Number of elements of previous group
    M = lut_numCnInCnGroups[2]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[2]*NR_LDPC_ZMAX;

    const uint32_t startAddrBnProcBuf_CNG5[5][18] = {{116736, 105216, 105600, 117504, 117888, 106368, 118272, 106752, 118656, 107136, 119040, 107520, 119424, 119808, 108288, 108672, 109056, 120960}, {88704, 30336, 39552, 31872, 46848, 58752, 89856, 87936, 90240, 33408, 89472, 41856, 61824, 30720, 73344, 74496, 62208, 92544}, {72576, 88320, 86400, 46464, 33024, 97536, 73728, 90624, 60288, 61440, 32640, 91776, 34176, 91392, 91008, 32256, 98688, 33792}, {59520, 97152, 57216, 31104, 74112, 22272, 21888, 23040, 22656, 75264, 61056, 92160, 97920, 93312, 34560, 98304, 23808, 93696}, {6912, 7296, 8064, 8448, 9216, 9600, 9984, 10368, 10752, 11136, 11520, 11904, 12288, 13056, 13440, 14208, 14976, 15360}}; 

    const uint16_t cyclicShiftValues_Z384_CNG5[5][18] = {{30, 24, 72, 71, 222, 252, 159, 100, 102, 323, 230, 320, 210, 185, 258, 52, 113, 80}, {11, 89, 17, 81, 19, 5, 229, 215, 201, 8, 148, 335, 313, 177, 93, 314, 132, 78}, {233, 61, 383, 76, 244, 147, 260, 258, 175, 361, 202, 2, 297, 289, 346, 139, 114, 163}, {22, 27, 312, 136, 274, 78, 90, 256, 287, 105, 312, 266, 21, 214, 297, 288, 168, 274}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

    for (j=0; j<5; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[2] + j*bitOffsetInGroup];

        /*
        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
            //mexPrintf("bnProcBuf = %p, p_cnProcBufRes = %p\n", &bnProcBuf[p_lut_cn2bn[j*M + i]],&p_cnProcBufRes[i]);
        }
        */
        
        for (i=0; i<lut_numCnInCnGroups_BG1_R13[2]; i++)
        {
            nrLDPC_circ_memcpy(&bnProcBuf[startAddrBnProcBuf_CNG5[j][i]],p_cnProcBufRes,Z,cyclicShiftValues_Z384_CNG5[j][i]);
            //mexPrintf("bnProcBuf = %p, p_cnProcBufRes = %p | p_lut_cn2bn = %d\n", &bnProcBuf[p_lut_cn2bn[j*M + i*Z]],&p_cnProcBufRes[i*Z],p_lut_cn2bn[j*M + i*Z]);
            p_cnProcBufRes+=NR_LDPC_ZMAX;
        }

    }

    // =====================================================================
    // CN group with 6 BNs

    p_lut_cn2bn += (M*5); // Number of elements of previous group
    M = lut_numCnInCnGroups[3]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[3]*NR_LDPC_ZMAX;

    const uint32_t startAddrBnProcBuf_CNG6[6][8] = {{114432, 103680, 115584, 104064, 115968, 116352, 104832, 117120}, {68736, 69888, 55680, 87168, 104448, 71040, 17280, 72192}, {83328, 56832, 59136, 71424, 84864, 29184, 60672, 46080}, {41472, 42624, 57984, 96768, 41088, 58368, 43776, 59904}, {18816, 86016, 71808, 31488, 86784, 87552, 72960, 89088}, {3456, 4608, 4992, 5376, 5760, 6144, 6528, 7680}}; 


    const uint16_t cyclicShiftValues_Z384_CNG6[6][8] = {{313, 13, 260, 130, 145, 187, 205, 298}, {177, 338, 303, 163, 213, 206, 102, 158}, {266, 57, 81, 280, 344, 264, 328, 235}, {115, 289, 358, 132, 242, 341, 213, 339}, {370, 57, 375, 4, 197, 59, 97, 234}, {0, 0, 0, 0, 0, 0, 0, 0}};

    
    for (j=0; j<6; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[3] + j*bitOffsetInGroup];

        /*
        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
        */

        for (i=0; i<lut_numCnInCnGroups_BG1_R13[3]; i++)
        {
            nrLDPC_circ_memcpy(&bnProcBuf[startAddrBnProcBuf_CNG6[j][i]],p_cnProcBufRes,Z,cyclicShiftValues_Z384_CNG6[j][i]);
            p_cnProcBufRes+=NR_LDPC_ZMAX;
        }
    }

    // =====================================================================
    // CN group with 7 BNs

    p_lut_cn2bn += (M*6); // Number of elements of previous group
    M = lut_numCnInCnGroups[4]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[4]*NR_LDPC_ZMAX;

    const uint32_t startAddrBnProcBuf_CNG7[7][5] = {{112512, 102144, 114048, 114816, 115200}, {100992, 28800, 102912, 85632, 103296}, {45312, 45696, 83712, 29568, 85248}, {80256, 81792, 55296, 57600, 70272}, {38784, 39936, 69120, 56448, 96384}, {52608, 54144, 96000, 70656, 21504}, {1152, 2304, 3072, 3840, 4224}}; 



    const uint16_t cyclicShiftValues_Z384_CNG7[7][5] = {{9, 101, 77, 142, 241}, {62, 339, 186, 248, 2}, {316, 274, 174, 137, 210}, {333, 111, 232, 89, 318}, {290, 383, 50, 347, 55}, {114, 354, 74, 12, 269}, {0, 0, 0, 0, 0}};

    
    for (j=0; j<7; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[4] + j*bitOffsetInGroup];

        /*
        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
        */

        for (i=0; i<lut_numCnInCnGroups_BG1_R13[4]; i++)
        {
            nrLDPC_circ_memcpy(&bnProcBuf[startAddrBnProcBuf_CNG7[j][i]],p_cnProcBufRes,Z,cyclicShiftValues_Z384_CNG7[j][i]);
            p_cnProcBufRes+=NR_LDPC_ZMAX;
        }
    }

    // =====================================================================
    // CN group with 8 BNs

    p_lut_cn2bn += (M*7); // Number of elements of previous group
    M = lut_numCnInCnGroups[5]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[5]*NR_LDPC_ZMAX;

    const uint32_t startAddrBnProcBuf_CNG8[8][2] = {{111744, 113664}, {100608, 102528}, {66432, 84096}, {81024, 56064}, {52992, 69504}, {67200, 84480}, {81408, 18432}, {384, 2688}};


    const uint16_t cyclicShiftValues_Z384_CNG8[8][2] = {{195, 48}, {14, 102}, {115, 8}, {166, 47}, {241, 188}, {51, 334}, {157, 115}, {0, 0}};


    
    
    for (j=0; j<8; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[5] + j*bitOffsetInGroup];

        /*
        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
        */

        for (i=0; i<lut_numCnInCnGroups_BG1_R13[5]; i++)
        {
            nrLDPC_circ_memcpy(&bnProcBuf[startAddrBnProcBuf_CNG8[j][i]],p_cnProcBufRes,Z,cyclicShiftValues_Z384_CNG8[j][i]);
            p_cnProcBufRes+=NR_LDPC_ZMAX;
        }
    }

    // =====================================================================
    // CN group with 9 BNs

    p_lut_cn2bn += (M*8); // Number of elements of previous group
    M = lut_numCnInCnGroups[6]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[6]*NR_LDPC_ZMAX;

    const uint32_t startAddrBnProcBuf_CNG9[9][2] = {{112128, 113280}, {38400, 101760}, {80640, 82176}, {52224, 53760}, {66816, 67968}, {53376, 54912}, {95232, 95616}, {39168, 40320}, {768, 1920}};
    const uint16_t cyclicShiftValues_Z384_CNG9[9][2] = {{278, 366}, {257, 232}, {1, 321}, {351, 133}, {92, 57}, {253, 303}, {18, 63}, {225, 82}, {0, 0,}};
    
    for (j=0; j<9; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[6] + j*bitOffsetInGroup];

        /*
        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
        */

        for (i=0; i<lut_numCnInCnGroups_BG1_R13[6]; i++)
        {
            nrLDPC_circ_memcpy(&bnProcBuf[startAddrBnProcBuf_CNG9[j][i]],p_cnProcBufRes,Z,cyclicShiftValues_Z384_CNG9[j][i]);
            p_cnProcBufRes+=NR_LDPC_ZMAX;
        }
    }

    // =====================================================================
    // CN group with 10 BNs

    p_lut_cn2bn += (M*9); // Number of elements of previous group
    M = lut_numCnInCnGroups[7]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[7]*NR_LDPC_ZMAX;

    const uint32_t startAddrBnProcBuf_CNG10[10][1] = {{112896}, {101376}, {67584}, {82560}, {54528}, {29952}, {68352}, {82944}, {21120}, {1536}}; 
    const uint16_t cyclicShiftValues_Z384_CNG10[10][1] = {{307}, {179}, {165}, {18}, {39}, {224}, {368}, {67}, {170}, {0}};
    
    for (j=0; j<10; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[7] + j*bitOffsetInGroup];

        /*
        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
        */

        for (i=0; i<lut_numCnInCnGroups_BG1_R13[7]; i++)
        {
            nrLDPC_circ_memcpy(&bnProcBuf[startAddrBnProcBuf_CNG10[j][i]],p_cnProcBufRes,Z,cyclicShiftValues_Z384_CNG10[j][i]);
            p_cnProcBufRes+=NR_LDPC_ZMAX;
        }
    }

    // =====================================================================
    // CN group with 19 BNs

    p_lut_cn2bn += (M*10); // Number of elements of previous group
    M = lut_numCnInCnGroups[8]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[8]*NR_LDPC_ZMAX;

    const uint32_t startAddrBnProcBuf_CNG19[19][4] = {{109824, 110208, 110592, 110976}, {99072, 25728, 99456, 99840}, {24192, 64128, 27264, 65280}, {62976, 44160, 44544, 44928}, {16128, 16512, 16896, 37248}, {34944, 75648, 36096, 78720}, {24576, 35328, 77184, 37632}, {76032, 26112, 36480, 79104}, {47616, 49152, 27648, 50688}, {76416, 77952, 77568, 79488}, {63360, 48000, 64512, 65664}, {24960, 26496, 49536, 51072}, {48384, 49920, 28032, 51456}, {94080, 48768, 50304, 51840}, {25344, 26880, 94464, 94848}, {35712, 64896, 28416, 38016}, {63744, 78336, 36864, 66048}, {76800, 18048, 20352, 79872}, {17664, 19584, 19968, 20736}}; 

    const uint16_t cyclicShiftValues_Z384_CNG19[19][4] = {{307, 76, 205, 276}, {19, 76, 250, 87}, {50, 73, 328, 0}, {369, 288, 332, 275}, {181, 144, 256, 199}, {216, 331, 161, 153}, {317, 331, 267, 56}, {288, 178, 160, 132}, {109, 295, 63, 305}, {17, 342, 129, 231}, {357, 217, 200, 341}, {215, 99, 88, 212}, {106, 354, 53, 304}, {242, 114, 131, 300}, {180, 331, 240, 271}, {330, 112, 205, 39}, {346, 0, 13, 357}, {1, 0, 0, 1}, {0, 0, 0, 0}};

    
    for (j=0; j<19; j++)
    {
        p_cnProcBufRes = &cnProcBufRes[lut_startAddrCnGroups[8] + j*bitOffsetInGroup];

        /*
        for (i=0; i<M; i++)
        {
            bnProcBuf[p_lut_cn2bn[j*M + i]] = p_cnProcBufRes[i];
        }
        */

        for (i=0; i<lut_numCnInCnGroups_BG1_R13[8]; i++)
        {
            nrLDPC_circ_memcpy(&bnProcBuf[startAddrBnProcBuf_CNG19[j][i]],p_cnProcBufRes,Z,cyclicShiftValues_Z384_CNG19[j][i]);
            p_cnProcBufRes+=NR_LDPC_ZMAX;
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
    const uint32_t* lut_cn2bnProcBuf = p_lut->cn2bnProcBuf;
    const uint8_t*  lut_numCnInCnGroups = p_lut->numCnInCnGroups;
    const uint32_t* lut_startAddrCnGroups = p_lut->startAddrCnGroups;

    int8_t* cnProcBuf    = p_procBuf->cnProcBuf;
    int8_t* bnProcBufRes = p_procBuf->bnProcBufRes;
    
    int8_t* p_cnProcBuf;
    const uint32_t* p_lut_cn2bn;
    uint32_t bitOffsetInGroup;
    uint32_t i;
    uint32_t j;
    uint32_t M;

    // For CN groups 3 to 19 no need to send the last BN back since it's single edge
    // and BN processing does not change the value already in the CN proc buf

    // =====================================================================
    // CN group with 3 BNs

    p_lut_cn2bn = &lut_cn2bnProcBuf[0];
    M = lut_numCnInCnGroups[0]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[0]*NR_LDPC_ZMAX;

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
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[1]*NR_LDPC_ZMAX;

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
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[2]*NR_LDPC_ZMAX;

    const uint32_t startAddrBnProcBuf_CNG5[5][18] = {{116736, 105216, 105600, 117504, 117888, 106368, 118272, 106752, 118656, 107136, 119040, 107520, 119424, 119808, 108288, 108672, 109056, 120960}, {88704, 30336, 39552, 31872, 46848, 58752, 89856, 87936, 90240, 33408, 89472, 41856, 61824, 30720, 73344, 74496, 62208, 92544}, {72576, 88320, 86400, 46464, 33024, 97536, 73728, 90624, 60288, 61440, 32640, 91776, 34176, 91392, 91008, 32256, 98688, 33792}, {59520, 97152, 57216, 31104, 74112, 22272, 21888, 23040, 22656, 75264, 61056, 92160, 97920, 93312, 34560, 98304, 23808, 93696}, {6912, 7296, 8064, 8448, 9216, 9600, 9984, 10368, 10752, 11136, 11520, 11904, 12288, 13056, 13440, 14208, 14976, 15360}}; 

    const uint16_t cyclicShiftValues_Z384_CNG5[5][18] = {{30, 71, 222, 159, 102, 230, 210, 185, 80, 24, 72, 252, 100, 323, 320, 258, 52, 113}, {89, 81, 8, 93, 314, 76, 19, 17, 335, 383, 215, 148, 346, 78, 177, 139, 163, 61}, {229, 289, 361, 11, 201, 2, 214, 233, 260, 312, 5, 175, 313, 136, 202, 297, 132, 22}, {312, 27, 147, 21, 288, 114, 244, 297, 274, 105, 258, 266, 274, 90, 287, 78, 256, 168}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
    
    for (j=0; j<4; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[2] + j*bitOffsetInGroup];

        if (1)
        {
            for (i=0; i<M; i++)
            {
                p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
            }
        }
        /*
        else
        {
            for (i=0; i<lut_numCnInCnGroups_BG1_R13[2]; i++)
            {
                nrLDPC_circ_memcpy(p_cnProcBuf, &bnProcBufRes[startAddrBnProcBuf_CNG5[j][i]], Z, cyclicShiftValues_Z384_CNG5[j][i]);
                p_cnProcBuf+=Z;
            }
        }
        */
    }

    // =====================================================================
    // CN group with 6 BNs

    p_lut_cn2bn += (M*5); // Number of elements of previous group
    M = lut_numCnInCnGroups[3]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[3]*NR_LDPC_ZMAX;

    for (j=0; j<5; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[3] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

    // =====================================================================
    // CN group with 7 BNs

    p_lut_cn2bn += (M*6); // Number of elements of previous group
    M = lut_numCnInCnGroups[4]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[4]*NR_LDPC_ZMAX;

    for (j=0; j<6; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[4] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

    // =====================================================================
    // CN group with 8 BNs

    p_lut_cn2bn += (M*7); // Number of elements of previous group
    M = lut_numCnInCnGroups[5]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[5]*NR_LDPC_ZMAX;

    for (j=0; j<7; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[5] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

    // =====================================================================
    // CN group with 9 BNs

    p_lut_cn2bn += (M*8); // Number of elements of previous group
    M = lut_numCnInCnGroups[6]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[6]*NR_LDPC_ZMAX;

    for (j=0; j<8; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[6] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

    // =====================================================================
    // CN group with 10 BNs

    p_lut_cn2bn += (M*9); // Number of elements of previous group
    M = lut_numCnInCnGroups[7]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[7]*NR_LDPC_ZMAX;

    for (j=0; j<9; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[7] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
        }
    }

    // =====================================================================
    // CN group with 19 BNs

    p_lut_cn2bn += (M*10); // Number of elements of previous group
    M = lut_numCnInCnGroups[8]*Z;
    bitOffsetInGroup = lut_numCnInCnGroups_BG1_R13[8]*NR_LDPC_ZMAX;

    for (j=0; j<19; j++)
    {
        p_cnProcBuf = &cnProcBuf[lut_startAddrCnGroups[8] + j*bitOffsetInGroup];

        for (i=0; i<M; i++)
        {
            p_cnProcBuf[i] = bnProcBufRes[p_lut_cn2bn[j*M + i]];
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
