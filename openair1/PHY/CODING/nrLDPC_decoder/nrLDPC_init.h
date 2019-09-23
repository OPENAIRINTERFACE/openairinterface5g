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

/*!\file nrLDPC_init.h
 * \brief Defines the function to initialize the LDPC decoder and sets correct LUTs.
 * \author Sebastian Wagner (TCL Communications) Email: <mailto:sebastian.wagner@tcl.com>
 * \date 27-03-2018
 * \version 1.0
 * \note
 * \warning
 */

#ifndef __NR_LDPC_INIT__H__
#define __NR_LDPC_INIT__H__

#include "./nrLDPC_lut/nrLDPC_lut.h"
#include "nrLDPC_defs.h"

/**
   \brief Initializes the decoder and sets correct LUTs
   \param p_decParams Pointer to decoder parameters
   \param p_lut Pointer to decoder LUTs
   \return Number of LLR values
*/
static inline uint32_t nrLDPC_init(t_nrLDPC_dec_params* p_decParams, t_nrLDPC_lut* p_lut)
{
    uint32_t numLLR = 0;
    uint8_t BG = p_decParams->BG;
    uint16_t Z = p_decParams->Z;
    uint8_t  R = p_decParams->R;

    if (BG == 2)
    {
        // LUT that only depend on BG
        p_lut->startAddrCnGroups = lut_startAddrCnGroups_BG2;

        // LUT that only depend on R
        if (R == 15)
        {
            p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG2_R15;
            p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG2_R15;
            p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG2_R15;
            p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG2_R15;
            p_lut->numEdgesPerBn = lut_numEdgesPerBn_BG2_R15;
            numLLR = NR_LDPC_NCOL_BG2_R15*Z;
        }
        else if (R == 13)
        {
            p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG2_R13;
            p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG2_R13;
            p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG2_R13;
            p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG2_R13;
            p_lut->numEdgesPerBn = lut_numEdgesPerBn_BG2_R13;
            numLLR = NR_LDPC_NCOL_BG2_R13*Z;
        }
        else if (R == 23)
        {
            p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG2_R23;
            p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG2_R23;
            p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG2_R23;
            p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG2_R23;
            p_lut->numEdgesPerBn = lut_numEdgesPerBn_BG2_R23;
            numLLR = NR_LDPC_NCOL_BG2_R23*Z;
        }

        // LUT that depend on Z and R
        if (Z == 2)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z2_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z2_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z2_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z2_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z2_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z2_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z2_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z2_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z2_R15;
            }
        }
        else if (Z == 3)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z3_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z3_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z3_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z3_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z3_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z3_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z3_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z3_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z3_R15;
            }
        }
        else if (Z == 4)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z4_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z4_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z4_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z4_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z4_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z4_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z4_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z4_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z4_R15;
            }
        }
        else if (Z == 5)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z5_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z5_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z5_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z5_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z5_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z5_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z5_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z5_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z5_R15;
            }
        }
        else if (Z == 6)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z6_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z6_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z6_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z6_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z6_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z6_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z6_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z6_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z6_R15;
            }
        }
        else if (Z == 7)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z7_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z7_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z7_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z7_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z7_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z7_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z7_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z7_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z7_R15;
            }
        }
        else if (Z == 8)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z8_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z8_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z8_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z8_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z8_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z8_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z8_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z8_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z8_R15;
            }
        }
        else if (Z == 9)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z9_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z9_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z9_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z9_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z9_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z9_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z9_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z9_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z9_R15;
            }
        }
        else if (Z == 10)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z10_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z10_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z10_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z10_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z10_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z10_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z10_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z10_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z10_R15;
            }
        }
        else if (Z == 11)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z11_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z11_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z11_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z11_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z11_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z11_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z11_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z11_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z11_R15;
            }
        }
        else if (Z == 12)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z12_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z12_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z12_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z12_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z12_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z12_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z12_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z12_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z12_R15;
            }
        }
        else if (Z == 13)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z13_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z13_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z13_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z13_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z13_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z13_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z13_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z13_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z13_R15;
            }
        }
        else if (Z == 14)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z14_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z14_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z14_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z14_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z14_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z14_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z14_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z14_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z14_R15;
            }
        }
        else if (Z == 15)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z15_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z15_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z15_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z15_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z15_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z15_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z15_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z15_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z15_R15;
            }
        }
        else if (Z == 16)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z16_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z16_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z16_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z16_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z16_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z16_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z16_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z16_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z16_R15;
            }
        }
        else if (Z == 18)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z18_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z18_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z18_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z18_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z18_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z18_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z18_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z18_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z18_R15;
            }
        }
        else if (Z == 20)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z20_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z20_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z20_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z20_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z20_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z20_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z20_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z20_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z20_R15;
            }
        }
        else if (Z == 22)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z22_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z22_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z22_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z22_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z22_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z22_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z22_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z22_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z22_R15;
            }
        }
        else if (Z == 24)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z24_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z24_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z24_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z24_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z24_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z24_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z24_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z24_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z24_R15;
            }
        }
        else if (Z == 26)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z26_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z26_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z26_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z26_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z26_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z26_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z26_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z26_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z26_R15;
            }
        }
        else if (Z == 28)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z28_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z28_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z28_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z28_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z28_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z28_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z28_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z28_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z28_R15;
            }
        }
        else if (Z == 30)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z30_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z30_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z30_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z30_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z30_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z30_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z30_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z30_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z30_R15;
            }
        }
        else if (Z == 32)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z32_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z32_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z32_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z32_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z32_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z32_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z32_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z32_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z32_R15;
            }
        }
        else if (Z == 36)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z36_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z36_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z36_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z36_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z36_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z36_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z36_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z36_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z36_R15;
            }
        }
        else if (Z == 40)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z40_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z40_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z40_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z40_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z40_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z40_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z40_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z40_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z40_R15;
            }
        }
        else if (Z == 44)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z44_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z44_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z44_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z44_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z44_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z44_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z44_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z44_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z44_R15;
            }
        }
        else if (Z == 48)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z48_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z48_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z48_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z48_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z48_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z48_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z48_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z48_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z48_R15;
            }
        }
        else if (Z == 52)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z52_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z52_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z52_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z52_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z52_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z52_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z52_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z52_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z52_R15;
            }
        }
        else if (Z == 56)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z56_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z56_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z56_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z56_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z56_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z56_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z56_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z56_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z56_R15;
            }
        }
        else if (Z == 60)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z60_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z60_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z60_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z60_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z60_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z60_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z60_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z60_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z60_R15;
            }
        }
        else if (Z == 64)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z64_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z64_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z64_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z64_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z64_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z64_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z64_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z64_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z64_R15;
            }
        }
        else if (Z == 72)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z72_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z72_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z72_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z72_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z72_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z72_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z72_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z72_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z72_R15;
            }
        }
        else if (Z == 80)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z80_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z80_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z80_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z80_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z80_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z80_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z80_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z80_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z80_R15;
            }
        }
        else if (Z == 88)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z88_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z88_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z88_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z88_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z88_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z88_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z88_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z88_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z88_R15;
            }
        }
        else if (Z == 96)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z96_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z96_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z96_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z96_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z96_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z96_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z96_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z96_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z96_R15;
            }
        }
        else if (Z == 104)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z104_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z104_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z104_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z104_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z104_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z104_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z104_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z104_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z104_R15;
            }
        }
        else if (Z == 112)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z112_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z112_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z112_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z112_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z112_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z112_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z112_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z112_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z112_R15;
            }
        }
        else if (Z == 120)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z120_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z120_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z120_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z120_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z120_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z120_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z120_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z120_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z120_R15;
            }
        }
        else if (Z == 128)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z128_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z128_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z128_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z128_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z128_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z128_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z128_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z128_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z128_R15;
            }
        }
        else if (Z == 144)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z144_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z144_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z144_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z144_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z144_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z144_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z144_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z144_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z144_R15;
            }
        }
        else if (Z == 160)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z160_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z160_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z160_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z160_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z160_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z160_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z160_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z160_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z160_R15;
            }
        }
        else if (Z == 176)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z176_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z176_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z176_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z176_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z176_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z176_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z176_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z176_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z176_R15;
            }
        }
        else if (Z == 192)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z192_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z192_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z192_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z192_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z192_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z192_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z192_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z192_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z192_R15;
            }
        }
        else if (Z == 208)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z208_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z208_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z208_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z208_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z208_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z208_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z208_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z208_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z208_R15;
            }
        }
        else if (Z == 224)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z224_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z224_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z224_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z224_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z224_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z224_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z224_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z224_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z224_R15;
            }
        }
        else if (Z == 240)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z240_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z240_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z240_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z240_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z240_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z240_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z240_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z240_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z240_R15;
            }
        }
        else if (Z == 256)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z256_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z256_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z256_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z256_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z256_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z256_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z256_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z256_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z256_R15;
            }
        }
        else if (Z == 288)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z288_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z288_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z288_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z288_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z288_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z288_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z288_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z288_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z288_R15;
            }
        }
        else if (Z == 320)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z320_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z320_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z320_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z320_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z320_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z320_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z320_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z320_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z320_R15;
            }
        }
        else if (Z == 352)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z352_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z352_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z352_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z352_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z352_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z352_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z352_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z352_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z352_R15;
            }
        }
        else if (Z == 384)
        {
            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z384_R13;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z384_R13;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z384_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z384_R23;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z384_R23;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z384_R23;
            }
            else if (R == 15)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG2_Z384_R15;
                p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG2_Z384_R15;
                p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG2_Z384_R15;
            }
        }
    }
    else
    {   // BG == 1
        // LUT that only depend on BG
        p_lut->startAddrCnGroups = lut_startAddrCnGroups_BG1;

        // LUT that only depend on R
        if (R == 13)
        {
            p_lut->startAddrBnProcBuf[0] = (const uint32_t**) startAddrBnProcBuf_BG1_R13_CNG3;
            p_lut->startAddrBnProcBuf[1] = (const uint32_t**) startAddrBnProcBuf_BG1_R13_CNG4;
            p_lut->startAddrBnProcBuf[2] = (const uint32_t**) startAddrBnProcBuf_BG1_R13_CNG5;
            p_lut->startAddrBnProcBuf[3] = (const uint32_t**) startAddrBnProcBuf_BG1_R13_CNG6;
            p_lut->startAddrBnProcBuf[4] = (const uint32_t**) startAddrBnProcBuf_BG1_R13_CNG7;
            p_lut->startAddrBnProcBuf[5] = (const uint32_t**) startAddrBnProcBuf_BG1_R13_CNG8;
            p_lut->startAddrBnProcBuf[6] = (const uint32_t**) startAddrBnProcBuf_BG1_R13_CNG9;
            p_lut->startAddrBnProcBuf[7] = (const uint32_t**) startAddrBnProcBuf_BG1_R13_CNG10;
            p_lut->startAddrBnProcBuf[8] = (const uint32_t**) startAddrBnProcBuf_BG1_R13_CNG19;

            p_lut->bnPosBnProcBuf[0] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[1] = (const uint8_t**) bnPosBnProcBuf_BG1_R13_CNG4;
            p_lut->bnPosBnProcBuf[2] = (const uint8_t**) bnPosBnProcBuf_BG1_R13_CNG5;
            p_lut->bnPosBnProcBuf[3] = (const uint8_t**) bnPosBnProcBuf_BG1_R13_CNG6;
            p_lut->bnPosBnProcBuf[4] = (const uint8_t**) bnPosBnProcBuf_BG1_R13_CNG7;
            p_lut->bnPosBnProcBuf[5] = (const uint8_t**) bnPosBnProcBuf_BG1_R13_CNG8;
            p_lut->bnPosBnProcBuf[6] = (const uint8_t**) bnPosBnProcBuf_BG1_R13_CNG9;
            p_lut->bnPosBnProcBuf[7] = (const uint8_t**) bnPosBnProcBuf_BG1_R13_CNG10;
            p_lut->bnPosBnProcBuf[8] = (const uint8_t**) bnPosBnProcBuf_BG1_R13_CNG19;

            p_lut->llr2llrProcBufAddr  = llr2llrProcBufAddr_BG1_R13;
            p_lut->llr2llrProcBufNumBn = llr2llrProcBufNumBn_BG1_R13;
            p_lut->llr2llrProcBufNumEl = &llr2llrProcBufNumEl_BG1_R13;
            
            p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG1_R13;
            p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG1_R13;
            p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG1_R13;
            p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG1_R13;
            p_lut->numEdgesPerBn = lut_numEdgesPerBn_BG1_R13;
            numLLR = NR_LDPC_NCOL_BG1_R13*Z;
        }
        else if (R == 23)
        {
            p_lut->startAddrBnProcBuf[0] = (const uint32_t**) startAddrBnProcBuf_BG1_R23_CNG3;
            p_lut->startAddrBnProcBuf[1] = (const uint32_t**) NULL;
            p_lut->startAddrBnProcBuf[2] = (const uint32_t**) NULL;
            p_lut->startAddrBnProcBuf[3] = (const uint32_t**) NULL;
            p_lut->startAddrBnProcBuf[4] = (const uint32_t**) startAddrBnProcBuf_BG1_R23_CNG7;
            p_lut->startAddrBnProcBuf[5] = (const uint32_t**) startAddrBnProcBuf_BG1_R23_CNG8;
            p_lut->startAddrBnProcBuf[6] = (const uint32_t**) startAddrBnProcBuf_BG1_R23_CNG9;
            p_lut->startAddrBnProcBuf[7] = (const uint32_t**) startAddrBnProcBuf_BG1_R23_CNG10;
            p_lut->startAddrBnProcBuf[8] = (const uint32_t**) startAddrBnProcBuf_BG1_R23_CNG19;

            p_lut->bnPosBnProcBuf[0] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[1] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[2] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[3] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[4] = (const uint8_t**) bnPosBnProcBuf_BG1_R23_CNG7;
            p_lut->bnPosBnProcBuf[5] = (const uint8_t**) bnPosBnProcBuf_BG1_R23_CNG8;
            p_lut->bnPosBnProcBuf[6] = (const uint8_t**) bnPosBnProcBuf_BG1_R23_CNG9;
            p_lut->bnPosBnProcBuf[7] = (const uint8_t**) bnPosBnProcBuf_BG1_R23_CNG10;
            p_lut->bnPosBnProcBuf[8] = (const uint8_t**) bnPosBnProcBuf_BG1_R23_CNG19;

            p_lut->llr2llrProcBufAddr  = llr2llrProcBufAddr_BG1_R23;
            p_lut->llr2llrProcBufNumBn = llr2llrProcBufNumBn_BG1_R23;
            p_lut->llr2llrProcBufNumEl = &llr2llrProcBufNumEl_BG1_R23;
                        
            p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG1_R23;
            p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG1_R23;
            p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG1_R23;
            p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG1_R23;
            p_lut->numEdgesPerBn = lut_numEdgesPerBn_BG1_R23;
            numLLR = NR_LDPC_NCOL_BG1_R23*Z;
        }
        else if (R == 89)
        {
            p_lut->startAddrBnProcBuf[0] = (const uint32_t**) startAddrBnProcBuf_BG1_R89_CNG3;
            p_lut->startAddrBnProcBuf[1] = (const uint32_t**) NULL;
            p_lut->startAddrBnProcBuf[2] = (const uint32_t**) NULL;
            p_lut->startAddrBnProcBuf[3] = (const uint32_t**) NULL;
            p_lut->startAddrBnProcBuf[4] = (const uint32_t**) NULL;
            p_lut->startAddrBnProcBuf[5] = (const uint32_t**) NULL;
            p_lut->startAddrBnProcBuf[6] = (const uint32_t**) NULL;
            p_lut->startAddrBnProcBuf[7] = (const uint32_t**) NULL;
            p_lut->startAddrBnProcBuf[8] = (const uint32_t**) startAddrBnProcBuf_BG1_R89_CNG19;

            p_lut->bnPosBnProcBuf[0] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[1] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[2] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[3] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[4] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[5] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[6] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[7] = (const uint8_t**) NULL;
            p_lut->bnPosBnProcBuf[8] = (const uint8_t**) bnPosBnProcBuf_BG1_R89_CNG19;

            p_lut->llr2llrProcBufAddr  = llr2llrProcBufAddr_BG1_R89;
            p_lut->llr2llrProcBufNumBn = llr2llrProcBufNumBn_BG1_R89;
            p_lut->llr2llrProcBufNumEl = &llr2llrProcBufNumEl_BG1_R89;
                
            p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG1_R89;
            p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG1_R89;
            p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG1_R89;
            p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG1_R89;
            p_lut->numEdgesPerBn = lut_numEdgesPerBn_BG1_R89;
            numLLR = NR_LDPC_NCOL_BG1_R89*Z;
        }

        // LUT that depend on Z and R
        if (Z == 2)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z2_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z2_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z2_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z2_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z2_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z2_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z2_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z2_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z2_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z2_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z2_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z2_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z2_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z2_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z2_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z2_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z2_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z2_R89;
            }
        }
        else if (Z == 3)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z3_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z3_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z3_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z3_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z3_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z3_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z3_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z3_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z3_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z3_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z3_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z3_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z3_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z3_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z3_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z3_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z3_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z3_R89;
            }
        }
        else if (Z == 4)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z4_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z4_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z4_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z4_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z4_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z4_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z4_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z4_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z4_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z4_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z4_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z4_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z4_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z4_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z4_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z4_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z4_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z4_R89;
            }
        }
        else if (Z == 5)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z5_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z5_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z5_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z5_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z5_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z5_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z5_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z5_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z5_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z5_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z5_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z5_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z5_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z5_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z5_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z5_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z5_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z5_R89;
            }
        }
        else if (Z == 6)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z6_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z6_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z6_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z6_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z6_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z6_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z6_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z6_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z6_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z6_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z6_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z6_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z6_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z6_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z6_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z6_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z6_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z6_R89;
            }
        }
        else if (Z == 7)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z7_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z7_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z7_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z7_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z7_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z7_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z7_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z7_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z7_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z7_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z7_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z7_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z7_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z7_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z7_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z7_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z7_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z7_R89;
            }
        }
        else if (Z == 8)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z8_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z8_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z8_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z8_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z8_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z8_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z8_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z8_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z8_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z8_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z8_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z8_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z8_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z8_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z8_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z8_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z8_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z8_R89;
            }
        }
        else if (Z == 9)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z9_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z9_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z9_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z9_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z9_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z9_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z9_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z9_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z9_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z9_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z9_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z9_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z9_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z9_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z9_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z9_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z9_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z9_R89;
            }
        }
        else if (Z == 10)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z10_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z10_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z10_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z10_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z10_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z10_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z10_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z10_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z10_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z10_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z10_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z10_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z10_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z10_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z10_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z10_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z10_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z10_R89;
            }
        }
        else if (Z == 11)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z11_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z11_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z11_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z11_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z11_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z11_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z11_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z11_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z11_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z11_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z11_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z11_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z11_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z11_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z11_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z11_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z11_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z11_R89;
            }
        }
        else if (Z == 12)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z12_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z12_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z12_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z12_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z12_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z12_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z12_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z12_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z12_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z12_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z12_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z12_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z12_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z12_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z12_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z12_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z12_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z12_R89;
            }
        }
        else if (Z == 13)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z13_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z13_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z13_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z13_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z13_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z13_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z13_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z13_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z13_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z13_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z13_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z13_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z13_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z13_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z13_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z13_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z13_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z13_R89;
            }
        }
        else if (Z == 14)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z14_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z14_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z14_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z14_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z14_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z14_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z14_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z14_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z14_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z14_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z14_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z14_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z14_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z14_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z14_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z14_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z14_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z14_R89;
            }
        }
        else if (Z == 15)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z15_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z15_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z15_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z15_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z15_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z15_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z15_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z15_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z15_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z15_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z15_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z15_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z15_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z15_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z15_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z15_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z15_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z15_R89;
            }
        }
        else if (Z == 16)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z16_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z16_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z16_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z16_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z16_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z16_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z16_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z16_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z16_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z16_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z16_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z16_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z16_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z16_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z16_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z16_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z16_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z16_R89;
            }
        }
        else if (Z == 18)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z18_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z18_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z18_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z18_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z18_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z18_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z18_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z18_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z18_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z18_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z18_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z18_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z18_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z18_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z18_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z18_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z18_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z18_R89;
            }
        }
        else if (Z == 20)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z20_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z20_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z20_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z20_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z20_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z20_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z20_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z20_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z20_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z20_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z20_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z20_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z20_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z20_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z20_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z20_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z20_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z20_R89;
            }
        }
        else if (Z == 22)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z22_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z22_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z22_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z22_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z22_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z22_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z22_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z22_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z22_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z22_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z22_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z22_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z22_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z22_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z22_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z22_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z22_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z22_R89;
            }
        }
        else if (Z == 24)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z24_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z24_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z24_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z24_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z24_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z24_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z24_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z24_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z24_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z24_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z24_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z24_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z24_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z24_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z24_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z24_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z24_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z24_R89;
            }
        }
        else if (Z == 26)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z26_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z26_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z26_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z26_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z26_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z26_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z26_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z26_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z26_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z26_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z26_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z26_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z26_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z26_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z26_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z26_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z26_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z26_R89;
            }
        }
        else if (Z == 28)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z28_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z28_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z28_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z28_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z28_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z28_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z28_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z28_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z28_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z28_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z28_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z28_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z28_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z28_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z28_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z28_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z28_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z28_R89;
            }
        }
        else if (Z == 30)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z30_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z30_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z30_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z30_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z30_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z30_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z30_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z30_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z30_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z30_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z30_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z30_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z30_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z30_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z30_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z30_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z30_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z30_R89;
            }
        }
        else if (Z == 32)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z32_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z32_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z32_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z32_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z32_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z32_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z32_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z32_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z32_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z32_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z32_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z32_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z32_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z32_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z32_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z32_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z32_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z32_R89;
            }
        }
        else if (Z == 36)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z36_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z36_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z36_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z36_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z36_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z36_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z36_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z36_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z36_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z36_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z36_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z36_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z36_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z36_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z36_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z36_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z36_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z36_R89;
            }
        }
        else if (Z == 40)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z40_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z40_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z40_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z40_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z40_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z40_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z40_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z40_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z40_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z40_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z40_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z40_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z40_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z40_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z40_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z40_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z40_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z40_R89;
            }
        }
        else if (Z == 44)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z44_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z44_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z44_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z44_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z44_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z44_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z44_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z44_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z44_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z44_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z44_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z44_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z44_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z44_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z44_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z44_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z44_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z44_R89;
            }
        }
        else if (Z == 48)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z48_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z48_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z48_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z48_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z48_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z48_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z48_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z48_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z48_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z48_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z48_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z48_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z48_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z48_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z48_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z48_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z48_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z48_R89;
            }
        }
        else if (Z == 52)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z52_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z52_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z52_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z52_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z52_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z52_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z52_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z52_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z52_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z52_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z52_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z52_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z52_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z52_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z52_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z52_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z52_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z52_R89;
            }
        }
        else if (Z == 56)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z56_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z56_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z56_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z56_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z56_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z56_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z56_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z56_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z56_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z56_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z56_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z56_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z56_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z56_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z56_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z56_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z56_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z56_R89;
            }
        }
        else if (Z == 60)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z60_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z60_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z60_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z60_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z60_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z60_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z60_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z60_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z60_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z60_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z60_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z60_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z60_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z60_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z60_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z60_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z60_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z60_R89;
            }
        }
        else if (Z == 64)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z64_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z64_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z64_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z64_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z64_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z64_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z64_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z64_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z64_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z64_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z64_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z64_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z64_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z64_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z64_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z64_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z64_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z64_R89;
            }
        }
        else if (Z == 72)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z72_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z72_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z72_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z72_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z72_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z72_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z72_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z72_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z72_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z72_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z72_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z72_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z72_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z72_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z72_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z72_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z72_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z72_R89;
            }
        }
        else if (Z == 80)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z80_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z80_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z80_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z80_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z80_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z80_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z80_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z80_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z80_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z80_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z80_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z80_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z80_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z80_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z80_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z80_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z80_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z80_R89;
            }
        }
        else if (Z == 88)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z88_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z88_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z88_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z88_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z88_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z88_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z88_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z88_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z88_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z88_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z88_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z88_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z88_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z88_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z88_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z88_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z88_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z88_R89;
            }
        }
        else if (Z == 96)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z96_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z96_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z96_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z96_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z96_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z96_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z96_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z96_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z96_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z96_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z96_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z96_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z96_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z96_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z96_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z96_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z96_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z96_R89;
            }
        }
        else if (Z == 104)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z104_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z104_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z104_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z104_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z104_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z104_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z104_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z104_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z104_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z104_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z104_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z104_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z104_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z104_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z104_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z104_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z104_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z104_R89;
            }
        }
        else if (Z == 112)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z112_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z112_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z112_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z112_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z112_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z112_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z112_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z112_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z112_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z112_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z112_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z112_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z112_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z112_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z112_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z112_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z112_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z112_R89;
            }
        }
        else if (Z == 120)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z120_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z120_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z120_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z120_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z120_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z120_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z120_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z120_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z120_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z120_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z120_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z120_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z120_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z120_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z120_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z120_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z120_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z120_R89;
            }
        }
        else if (Z == 128)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z128_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z128_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z128_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z128_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z128_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z128_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z128_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z128_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z128_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z128_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z128_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z128_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z128_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z128_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z128_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z128_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z128_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z128_R89;
            }
        }
        else if (Z == 144)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z144_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z144_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z144_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z144_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z144_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z144_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z144_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z144_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z144_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z144_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z144_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z144_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z144_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z144_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z144_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z144_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z144_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z144_R89;
            }
        }
        else if (Z == 160)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z160_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z160_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z160_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z160_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z160_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z160_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z160_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z160_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z160_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z160_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z160_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z160_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z160_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z160_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z160_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z160_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z160_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z160_R89;
            }
        }
        else if (Z == 176)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z176_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z176_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z176_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z176_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z176_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z176_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z176_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z176_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z176_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z176_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z176_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z176_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z176_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z176_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z176_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z176_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z176_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z176_R89;
            }
        }
        else if (Z == 192)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z192_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z192_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z192_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z192_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z192_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z192_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z192_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z192_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z192_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z192_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z192_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z192_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z192_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z192_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z192_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z192_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z192_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z192_R89;
            }
        }
        else if (Z == 208)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z208_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z208_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z208_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z208_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z208_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z208_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z208_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z208_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z208_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z208_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z208_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z208_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z208_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z208_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z208_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z208_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z208_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z208_R89;
            }
        }
        else if (Z == 224)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z224_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z224_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z224_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z224_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z224_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z224_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z224_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z224_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z224_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z224_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z224_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z224_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z224_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z224_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z224_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z224_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z224_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z224_R89;
            }
        }
        else if (Z == 240)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z240_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z240_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z240_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z240_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z240_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z240_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z240_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z240_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z240_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z240_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z240_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z240_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z240_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z240_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z240_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z240_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z240_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z240_R89;
            }
        }
        else if (Z == 256)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z256_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z256_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z256_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z256_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z256_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z256_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z256_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z256_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z256_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z256_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z256_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z256_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z256_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z256_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z256_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z256_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z256_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z256_R89;
            }
        }
        else if (Z == 288)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z288_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z288_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z288_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z288_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z288_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z288_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z288_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z288_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z288_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z288_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z288_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z288_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z288_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z288_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z288_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z288_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z288_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z288_R89;
            }
        }
        else if (Z == 320)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z320_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z320_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z320_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z320_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z320_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z320_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z320_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z320_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z320_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z320_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z320_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z320_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z320_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z320_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z320_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z320_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z320_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z320_R89;
            }
        }
        else if (Z == 352)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z352_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z352_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z352_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z352_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z352_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z352_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z352_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z352_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z352_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z352_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z352_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z352_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z352_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z352_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z352_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z352_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z352_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z352_R89;
            }
        }
        else if (Z == 384)
        {
            p_lut->circShift[0] = (const uint16_t**) circShift_BG1_Z384_CNG3;
            p_lut->circShift[1] = (const uint16_t**) circShift_BG1_Z384_CNG4;
            p_lut->circShift[2] = (const uint16_t**) circShift_BG1_Z384_CNG5;
            p_lut->circShift[3] = (const uint16_t**) circShift_BG1_Z384_CNG6;
            p_lut->circShift[4] = (const uint16_t**) circShift_BG1_Z384_CNG7;
            p_lut->circShift[5] = (const uint16_t**) circShift_BG1_Z384_CNG8;
            p_lut->circShift[6] = (const uint16_t**) circShift_BG1_Z384_CNG9;
            p_lut->circShift[7] = (const uint16_t**) circShift_BG1_Z384_CNG10;
            p_lut->circShift[8] = (const uint16_t**) circShift_BG1_Z384_CNG19;

            if (R == 13)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z384_R13;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z384_R13;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z384_R13;
            }
            else if (R == 23)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z384_R23;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z384_R23;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z384_R23;
            }
            else if (R == 89)
            {
                p_lut->llr2CnProcBuf  = lut_llr2CnProcBuf_BG1_Z384_R89;
                //p_lut->cn2bnProcBuf   = lut_cn2bnProcBuf_BG1_Z384_R89;
                //p_lut->llr2llrProcBuf = lut_llr2llrProcBuf_BG1_Z384_R89;
            }
        }

    }

    return numLLR;
}

#endif
