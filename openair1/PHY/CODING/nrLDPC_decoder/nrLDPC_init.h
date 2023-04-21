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
 * \date 30-09-2019
 * \version 1.0
 * \note
 * \warning
 */

#ifndef __NR_LDPC_INIT__H__
#define __NR_LDPC_INIT__H__
#include <string.h>
#include "nrLDPC_lut.h"
#include "nrLDPCdecoder_defs.h"

#define expandArr8(namE)                                                         \
  (const arr8_t)                                                                 \
  {                                                                              \
    (uint8_t*)namE, sizeof(namE) / sizeof(*namE), sizeof(*namE) / sizeof(**namE) \
  }
#define expandArr16(namE)                                                         \
  (const arr16_t)                                                                 \
  {                                                                               \
    (uint16_t*)namE, sizeof(namE) / sizeof(*namE), sizeof(*namE) / sizeof(**namE) \
  }
#define expandArr32(namE)                                                         \
  (const arr32_t)                                                                 \
  {                                                                               \
    (uint32_t*)namE, sizeof(namE) / sizeof(*namE), sizeof(*namE) / sizeof(**namE) \
  }
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
    memset(p_lut, 0, sizeof(*p_lut));
    if (BG == 2)
    {
        // LUT that only depend on BG
        p_lut->startAddrCnGroups = lut_startAddrCnGroups_BG2;
        p_lut->posBnInCnProcBuf[0] = expandArr8(posBnInCnProcBuf_BG2_CNG3);
        p_lut->posBnInCnProcBuf[1] = expandArr8(posBnInCnProcBuf_BG2_CNG4);
        p_lut->posBnInCnProcBuf[2] = expandArr8(posBnInCnProcBuf_BG2_CNG5);
        p_lut->posBnInCnProcBuf[3] = expandArr8(posBnInCnProcBuf_BG2_CNG6);
        p_lut->posBnInCnProcBuf[4] = expandArr8(posBnInCnProcBuf_BG2_CNG8);
        p_lut->posBnInCnProcBuf[5] = expandArr8(posBnInCnProcBuf_BG2_CNG10);

        // LUT that only depend on R
        if (R == 15)
        {
          p_lut->startAddrBnProcBuf[0] = expandArr32(startAddrBnProcBuf_BG2_R15_CNG3);
          p_lut->startAddrBnProcBuf[1] = expandArr32(startAddrBnProcBuf_BG2_R15_CNG4);
          p_lut->startAddrBnProcBuf[2] = expandArr32(startAddrBnProcBuf_BG2_R15_CNG5);
          p_lut->startAddrBnProcBuf[3] = expandArr32(startAddrBnProcBuf_BG2_R15_CNG6);
          p_lut->startAddrBnProcBuf[4] = expandArr32(startAddrBnProcBuf_BG2_R15_CNG8);
          p_lut->startAddrBnProcBuf[5] = expandArr32(startAddrBnProcBuf_BG2_R15_CNG10);

          p_lut->bnPosBnProcBuf[0] = expandArr8(bnPosBnProcBuf_BG2_R15_CNG3);
          p_lut->bnPosBnProcBuf[1] = expandArr8(bnPosBnProcBuf_BG2_R15_CNG4);
          p_lut->bnPosBnProcBuf[2] = expandArr8(bnPosBnProcBuf_BG2_R15_CNG5);
          p_lut->bnPosBnProcBuf[3] = expandArr8(bnPosBnProcBuf_BG2_R15_CNG6);
          p_lut->bnPosBnProcBuf[4] = expandArr8(bnPosBnProcBuf_BG2_R15_CNG8);
          p_lut->bnPosBnProcBuf[5] = expandArr8(bnPosBnProcBuf_BG2_R15_CNG10);

          p_lut->llr2llrProcBufAddr = llr2llrProcBufAddr_BG2_R15;
          p_lut->llr2llrProcBufBnPos = llr2llrProcBufBnPos_BG2_R15;

          p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG2_R15;
          p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG2_R15;
          p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG2_R15;
          p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG2_R15;

          numLLR = NR_LDPC_NCOL_BG2_R15 * Z;
        }
        else if (R == 13)
        {
          p_lut->startAddrBnProcBuf[1] = expandArr32(startAddrBnProcBuf_BG2_R13_CNG4);
          p_lut->startAddrBnProcBuf[2] = expandArr32(startAddrBnProcBuf_BG2_R13_CNG5);
          p_lut->startAddrBnProcBuf[3] = expandArr32(startAddrBnProcBuf_BG2_R13_CNG6);
          p_lut->startAddrBnProcBuf[4] = expandArr32(startAddrBnProcBuf_BG2_R13_CNG8);
          p_lut->startAddrBnProcBuf[5] = expandArr32(startAddrBnProcBuf_BG2_R13_CNG10);

          p_lut->bnPosBnProcBuf[1] = expandArr8(bnPosBnProcBuf_BG2_R13_CNG4);
          p_lut->bnPosBnProcBuf[2] = expandArr8(bnPosBnProcBuf_BG2_R13_CNG5);
          p_lut->bnPosBnProcBuf[3] = expandArr8(bnPosBnProcBuf_BG2_R13_CNG6);
          p_lut->bnPosBnProcBuf[4] = expandArr8(bnPosBnProcBuf_BG2_R13_CNG8);
          p_lut->bnPosBnProcBuf[5] = expandArr8(bnPosBnProcBuf_BG2_R13_CNG10);

          p_lut->llr2llrProcBufAddr = llr2llrProcBufAddr_BG2_R13;
          p_lut->llr2llrProcBufBnPos = llr2llrProcBufBnPos_BG2_R13;

          p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG2_R13;
          p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG2_R13;
          p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG2_R13;
          p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG2_R13;

          numLLR = NR_LDPC_NCOL_BG2_R13 * Z;
        }
        else if (R == 23)
        {
          p_lut->startAddrBnProcBuf[1] = expandArr32(startAddrBnProcBuf_BG2_R23_CNG4);
          p_lut->startAddrBnProcBuf[3] = expandArr32(startAddrBnProcBuf_BG2_R23_CNG6);
          p_lut->startAddrBnProcBuf[4] = expandArr32(startAddrBnProcBuf_BG2_R23_CNG8);
          p_lut->startAddrBnProcBuf[5] = expandArr32(startAddrBnProcBuf_BG2_R23_CNG10);

          p_lut->bnPosBnProcBuf[1] = expandArr8(bnPosBnProcBuf_BG2_R23_CNG4);
          p_lut->bnPosBnProcBuf[3] = expandArr8(bnPosBnProcBuf_BG2_R23_CNG6);
          p_lut->bnPosBnProcBuf[4] = expandArr8(bnPosBnProcBuf_BG2_R23_CNG8);
          p_lut->bnPosBnProcBuf[5] = expandArr8(bnPosBnProcBuf_BG2_R23_CNG10);

          p_lut->llr2llrProcBufAddr = llr2llrProcBufAddr_BG2_R23;
          p_lut->llr2llrProcBufBnPos = llr2llrProcBufBnPos_BG2_R23;

          p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG2_R23;
          p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG2_R23;
          p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG2_R23;
          p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG2_R23;

          numLLR = NR_LDPC_NCOL_BG2_R23 * Z;
        }

        // LUT that depend on Z and R
        switch (Z)
        {
        case 2:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z2_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z2_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z2_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z2_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z2_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z2_CNG10);
          break;
        }
        case 3:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z3_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z3_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z3_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z3_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z3_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z3_CNG10);
          break;
        }
        case 4:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z4_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z4_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z4_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z4_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z4_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z4_CNG10);
          break;
        }
        case 5:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z5_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z5_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z5_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z5_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z5_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z5_CNG10);
          break;
        }
        case 6:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z6_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z6_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z6_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z6_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z6_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z6_CNG10);
          break;
        }
        case 7:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z7_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z7_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z7_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z7_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z7_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z7_CNG10);
          break;
        }
        case 8:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z8_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z8_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z8_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z8_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z8_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z8_CNG10);
          break;
        }
        case 9:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z9_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z9_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z9_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z9_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z9_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z9_CNG10);
          break;
        }
        case 10:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z10_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z10_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z10_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z10_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z10_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z10_CNG10);
          break;
        }
        case 11:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z11_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z11_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z11_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z11_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z11_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z11_CNG10);
          break;
        }
        case 12:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z12_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z12_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z12_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z12_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z12_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z12_CNG10);
          break;
        }
        case 13:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z13_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z13_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z13_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z13_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z13_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z13_CNG10);
          break;
        }
        case 14:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z14_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z14_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z14_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z14_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z14_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z14_CNG10);
          break;
        }
        case 15:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z15_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z15_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z15_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z15_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z15_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z15_CNG10);
          break;
        }
        case 16:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z16_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z16_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z16_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z16_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z16_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z16_CNG10);
          break;
        }
        case 18:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z18_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z18_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z18_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z18_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z18_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z18_CNG10);
          break;
        }
        case 20:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z20_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z20_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z20_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z20_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z20_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z20_CNG10);
          break;
        }
        case 22:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z22_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z22_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z22_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z22_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z22_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z22_CNG10);
          break;
        }
        case 24:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z24_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z24_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z24_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z24_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z24_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z24_CNG10);
          break;
        }
        case 26:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z26_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z26_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z26_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z26_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z26_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z26_CNG10);
          break;
        }
        case 28:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z28_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z28_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z28_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z28_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z28_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z28_CNG10);
          break;
        }
        case 30:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z30_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z30_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z30_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z30_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z30_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z30_CNG10);
          break;
        }
        case 32:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z32_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z32_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z32_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z32_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z32_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z32_CNG10);
          break;
        }
        case 36:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z36_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z36_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z36_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z36_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z36_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z36_CNG10);
          break;
        }
        case 40:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z40_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z40_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z40_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z40_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z40_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z40_CNG10);
          break;
        }
        case 44:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z44_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z44_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z44_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z44_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z44_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z44_CNG10);
          break;
        }
        case 48:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z48_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z48_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z48_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z48_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z48_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z48_CNG10);
          break;
        }
        case 52:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z52_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z52_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z52_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z52_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z52_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z52_CNG10);
          break;
        }
        case 56:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z56_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z56_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z56_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z56_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z56_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z56_CNG10);
          break;
        }
        case 60:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z60_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z60_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z60_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z60_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z60_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z60_CNG10);
          break;
        }
        case 64:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z64_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z64_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z64_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z64_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z64_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z64_CNG10);
          break;
        }
        case 72:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z72_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z72_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z72_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z72_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z72_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z72_CNG10);
          break;
        }
        case 80:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z80_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z80_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z80_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z80_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z80_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z80_CNG10);
          break;
        }
        case 88:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z88_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z88_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z88_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z88_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z88_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z88_CNG10);
          break;
        }
        case 96:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z96_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z96_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z96_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z96_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z96_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z96_CNG10);
          break;
        }
        case 104:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z104_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z104_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z104_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z104_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z104_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z104_CNG10);
          break;
        }
        case 112:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z112_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z112_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z112_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z112_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z112_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z112_CNG10);
          break;
        }
        case 120:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z120_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z120_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z120_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z120_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z120_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z120_CNG10);
          break;
        }
        case 128:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z128_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z128_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z128_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z128_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z128_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z128_CNG10);
          break;
        }
        case 144:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z144_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z144_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z144_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z144_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z144_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z144_CNG10);
          break;
        }
        case 160:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z160_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z160_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z160_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z160_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z160_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z160_CNG10);
          break;
        }
        case 176:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z176_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z176_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z176_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z176_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z176_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z176_CNG10);
          break;
        }
        case 192:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z192_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z192_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z192_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z192_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z192_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z192_CNG10);
          break;
        }
        case 208:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z208_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z208_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z208_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z208_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z208_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z208_CNG10);
          break;
        }
        case 224:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z224_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z224_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z224_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z224_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z224_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z224_CNG10);
          break;
        }
        case 240:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z240_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z240_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z240_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z240_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z240_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z240_CNG10);
          break;
        }
        case 256:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z256_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z256_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z256_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z256_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z256_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z256_CNG10);
          break;
        }
        case 288:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z288_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z288_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z288_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z288_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z288_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z288_CNG10);
          break;
        }
        case 320:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z320_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z320_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z320_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z320_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z320_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z320_CNG10);
          break;
        }
        case 352:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z352_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z352_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z352_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z352_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z352_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z352_CNG10);
          break;
        }
        case 384:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG2_Z384_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG2_Z384_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG2_Z384_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG2_Z384_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG2_Z384_CNG8);
          p_lut->circShift[5] = expandArr16(circShift_BG2_Z384_CNG10);
          break;
        }
        default: {
        }
        }
    }
    else
    {   // BG == 1
        // LUT that only depend on BG
        p_lut->startAddrCnGroups = lut_startAddrCnGroups_BG1;

        p_lut->posBnInCnProcBuf[0] = expandArr8(posBnInCnProcBuf_BG1_CNG3);
        p_lut->posBnInCnProcBuf[1] = expandArr8(posBnInCnProcBuf_BG1_CNG4);
        p_lut->posBnInCnProcBuf[2] = expandArr8(posBnInCnProcBuf_BG1_CNG5);
        p_lut->posBnInCnProcBuf[3] = expandArr8(posBnInCnProcBuf_BG1_CNG6);
        p_lut->posBnInCnProcBuf[4] = expandArr8(posBnInCnProcBuf_BG1_CNG7);
        p_lut->posBnInCnProcBuf[5] = expandArr8(posBnInCnProcBuf_BG1_CNG8);
        p_lut->posBnInCnProcBuf[6] = expandArr8(posBnInCnProcBuf_BG1_CNG9);
        p_lut->posBnInCnProcBuf[7] = expandArr8(posBnInCnProcBuf_BG1_CNG10);
        p_lut->posBnInCnProcBuf[8] = expandArr8(posBnInCnProcBuf_BG1_CNG19);

        // LUT that only depend on R
        if (R == 13)
        {
          p_lut->startAddrBnProcBuf[0] = expandArr32(startAddrBnProcBuf_BG1_R13_CNG3);
          p_lut->startAddrBnProcBuf[1] = expandArr32(startAddrBnProcBuf_BG1_R13_CNG4);
          p_lut->startAddrBnProcBuf[2] = expandArr32(startAddrBnProcBuf_BG1_R13_CNG5);
          p_lut->startAddrBnProcBuf[3] = expandArr32(startAddrBnProcBuf_BG1_R13_CNG6);
          p_lut->startAddrBnProcBuf[4] = expandArr32(startAddrBnProcBuf_BG1_R13_CNG7);
          p_lut->startAddrBnProcBuf[5] = expandArr32(startAddrBnProcBuf_BG1_R13_CNG8);
          p_lut->startAddrBnProcBuf[6] = expandArr32(startAddrBnProcBuf_BG1_R13_CNG9);
          p_lut->startAddrBnProcBuf[7] = expandArr32(startAddrBnProcBuf_BG1_R13_CNG10);
          p_lut->startAddrBnProcBuf[8] = expandArr32(startAddrBnProcBuf_BG1_R13_CNG19);

          p_lut->bnPosBnProcBuf[1] = expandArr8(bnPosBnProcBuf_BG1_R13_CNG4);
          p_lut->bnPosBnProcBuf[2] = expandArr8(bnPosBnProcBuf_BG1_R13_CNG5);
          p_lut->bnPosBnProcBuf[3] = expandArr8(bnPosBnProcBuf_BG1_R13_CNG6);
          p_lut->bnPosBnProcBuf[4] = expandArr8(bnPosBnProcBuf_BG1_R13_CNG7);
          p_lut->bnPosBnProcBuf[5] = expandArr8(bnPosBnProcBuf_BG1_R13_CNG8);
          p_lut->bnPosBnProcBuf[6] = expandArr8(bnPosBnProcBuf_BG1_R13_CNG9);
          p_lut->bnPosBnProcBuf[7] = expandArr8(bnPosBnProcBuf_BG1_R13_CNG10);
          p_lut->bnPosBnProcBuf[8] = expandArr8(bnPosBnProcBuf_BG1_R13_CNG19);

          p_lut->llr2llrProcBufAddr = llr2llrProcBufAddr_BG1_R13;
          p_lut->llr2llrProcBufBnPos = llr2llrProcBufBnPos_BG1_R13;

          p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG1_R13;
          p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG1_R13;
          p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG1_R13;
          p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG1_R13;

          numLLR = NR_LDPC_NCOL_BG1_R13 * Z;
        }
        else if (R == 23)
        {
          p_lut->startAddrBnProcBuf[0] = expandArr32(startAddrBnProcBuf_BG1_R23_CNG3);
          p_lut->startAddrBnProcBuf[4] = expandArr32(startAddrBnProcBuf_BG1_R23_CNG7);
          p_lut->startAddrBnProcBuf[5] = expandArr32(startAddrBnProcBuf_BG1_R23_CNG8);
          p_lut->startAddrBnProcBuf[6] = expandArr32(startAddrBnProcBuf_BG1_R23_CNG9);
          p_lut->startAddrBnProcBuf[7] = expandArr32(startAddrBnProcBuf_BG1_R23_CNG10);
          p_lut->startAddrBnProcBuf[8] = expandArr32(startAddrBnProcBuf_BG1_R23_CNG19);

          p_lut->bnPosBnProcBuf[4] = expandArr8(bnPosBnProcBuf_BG1_R23_CNG7);
          p_lut->bnPosBnProcBuf[5] = expandArr8(bnPosBnProcBuf_BG1_R23_CNG8);
          p_lut->bnPosBnProcBuf[6] = expandArr8(bnPosBnProcBuf_BG1_R23_CNG9);
          p_lut->bnPosBnProcBuf[7] = expandArr8(bnPosBnProcBuf_BG1_R23_CNG10);
          p_lut->bnPosBnProcBuf[8] = expandArr8(bnPosBnProcBuf_BG1_R23_CNG19);

          p_lut->llr2llrProcBufAddr = llr2llrProcBufAddr_BG1_R23;
          p_lut->llr2llrProcBufBnPos = llr2llrProcBufBnPos_BG1_R23;

          p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG1_R23;
          p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG1_R23;
          p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG1_R23;
          p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG1_R23;

          numLLR = NR_LDPC_NCOL_BG1_R23 * Z;
        }
        else if (R == 89)
        {
          p_lut->startAddrBnProcBuf[0] = expandArr32(startAddrBnProcBuf_BG1_R89_CNG3);
          p_lut->startAddrBnProcBuf[8] = expandArr32(startAddrBnProcBuf_BG1_R89_CNG19);

          p_lut->bnPosBnProcBuf[8] = expandArr8(bnPosBnProcBuf_BG1_R89_CNG19);

          p_lut->llr2llrProcBufAddr = llr2llrProcBufAddr_BG1_R89;
          p_lut->llr2llrProcBufBnPos = llr2llrProcBufBnPos_BG1_R89;

          p_lut->numCnInCnGroups = lut_numCnInCnGroups_BG1_R89;
          p_lut->numBnInBnGroups = lut_numBnInBnGroups_BG1_R89;
          p_lut->startAddrBnGroups = lut_startAddrBnGroups_BG1_R89;
          p_lut->startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG1_R89;

          numLLR = NR_LDPC_NCOL_BG1_R89 * Z;
        }

        // LUT that depend on Z and R
        switch (Z)
        {
        case 2:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z2_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z2_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z2_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z2_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z2_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z2_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z2_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z2_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z2_CNG19);
          break;
        }
        case 3:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z3_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z3_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z3_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z3_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z3_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z3_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z3_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z3_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z3_CNG19);
          break;
        }
        case 4:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z4_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z4_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z4_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z4_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z4_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z4_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z4_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z4_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z4_CNG19);
          break;
        }
        case 5:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z5_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z5_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z5_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z5_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z5_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z5_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z5_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z5_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z5_CNG19);
          break;
        }
        case 6:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z6_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z6_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z6_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z6_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z6_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z6_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z6_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z6_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z6_CNG19);
          break;
        }
        case 7:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z7_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z7_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z7_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z7_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z7_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z7_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z7_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z7_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z7_CNG19);
          break;
        }
        case 8:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z8_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z8_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z8_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z8_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z8_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z8_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z8_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z8_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z8_CNG19);
          break;
        }
        case 9:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z9_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z9_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z9_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z9_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z9_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z9_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z9_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z9_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z9_CNG19);
          break;
        }
        case 10:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z10_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z10_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z10_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z10_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z10_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z10_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z10_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z10_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z10_CNG19);
          break;
        }
        case 11:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z11_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z11_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z11_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z11_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z11_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z11_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z11_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z11_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z11_CNG19);
          break;
        }
        case 12:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z12_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z12_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z12_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z12_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z12_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z12_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z12_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z12_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z12_CNG19);
          break;
        }
        case 13:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z13_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z13_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z13_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z13_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z13_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z13_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z13_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z13_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z13_CNG19);
          break;
        }
        case 14:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z14_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z14_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z14_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z14_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z14_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z14_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z14_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z14_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z14_CNG19);
          break;
        }
        case 15:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z15_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z15_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z15_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z15_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z15_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z15_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z15_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z15_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z15_CNG19);
          break;
        }
        case 16:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z16_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z16_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z16_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z16_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z16_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z16_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z16_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z16_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z16_CNG19);
          break;
        }
        case 18:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z18_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z18_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z18_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z18_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z18_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z18_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z18_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z18_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z18_CNG19);
          break;
        }
        case 20:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z20_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z20_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z20_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z20_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z20_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z20_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z20_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z20_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z20_CNG19);
          break;
        }
        case 22:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z22_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z22_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z22_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z22_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z22_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z22_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z22_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z22_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z22_CNG19);
          break;
        }
        case 24:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z24_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z24_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z24_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z24_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z24_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z24_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z24_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z24_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z24_CNG19);
          break;
        }
        case 26:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z26_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z26_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z26_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z26_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z26_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z26_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z26_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z26_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z26_CNG19);
          break;
        }
        case 28:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z28_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z28_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z28_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z28_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z28_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z28_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z28_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z28_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z28_CNG19);
          break;
        }
        case 30:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z30_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z30_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z30_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z30_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z30_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z30_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z30_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z30_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z30_CNG19);
          break;
        }
        case 32:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z32_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z32_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z32_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z32_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z32_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z32_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z32_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z32_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z32_CNG19);
          break;
        }
        case 36:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z36_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z36_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z36_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z36_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z36_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z36_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z36_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z36_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z36_CNG19);
          break;
        }
        case 40:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z40_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z40_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z40_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z40_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z40_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z40_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z40_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z40_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z40_CNG19);
          break;
        }
        case 44:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z44_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z44_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z44_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z44_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z44_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z44_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z44_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z44_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z44_CNG19);
          break;
        }
        case 48:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z48_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z48_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z48_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z48_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z48_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z48_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z48_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z48_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z48_CNG19);
          break;
        }
        case 52:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z52_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z52_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z52_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z52_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z52_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z52_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z52_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z52_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z52_CNG19);
          break;
        }
        case 56:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z56_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z56_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z56_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z56_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z56_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z56_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z56_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z56_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z56_CNG19);
          break;
        }
        case 60:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z60_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z60_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z60_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z60_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z60_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z60_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z60_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z60_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z60_CNG19);
          break;
        }
        case 64:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z64_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z64_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z64_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z64_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z64_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z64_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z64_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z64_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z64_CNG19);
          break;
        }
        case 72:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z72_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z72_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z72_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z72_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z72_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z72_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z72_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z72_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z72_CNG19);
          break;
        }
        case 80:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z80_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z80_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z80_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z80_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z80_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z80_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z80_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z80_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z80_CNG19);
          break;
        }
        case 88:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z88_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z88_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z88_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z88_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z88_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z88_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z88_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z88_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z88_CNG19);
          break;
        }
        case 96:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z96_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z96_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z96_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z96_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z96_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z96_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z96_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z96_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z96_CNG19);
          break;
        }
        case 104:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z104_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z104_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z104_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z104_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z104_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z104_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z104_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z104_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z104_CNG19);
          break;
        }
        case 112:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z112_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z112_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z112_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z112_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z112_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z112_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z112_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z112_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z112_CNG19);
          break;
        }
        case 120:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z120_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z120_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z120_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z120_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z120_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z120_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z120_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z120_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z120_CNG19);
          break;
        }
        case 128:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z128_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z128_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z128_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z128_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z128_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z128_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z128_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z128_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z128_CNG19);
          break;
        }
        case 144:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z144_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z144_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z144_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z144_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z144_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z144_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z144_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z144_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z144_CNG19);
          break;
        }
        case 160:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z160_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z160_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z160_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z160_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z160_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z160_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z160_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z160_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z160_CNG19);
          break;
        }
        case 176:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z176_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z176_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z176_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z176_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z176_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z176_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z176_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z176_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z176_CNG19);
          break;
        }
        case 192:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z192_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z192_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z192_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z192_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z192_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z192_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z192_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z192_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z192_CNG19);
          break;
        }
        case 208:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z208_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z208_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z208_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z208_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z208_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z208_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z208_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z208_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z208_CNG19);
          break;
        }
        case 224:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z224_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z224_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z224_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z224_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z224_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z224_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z224_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z224_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z224_CNG19);
          break;
        }
        case 240:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z240_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z240_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z240_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z240_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z240_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z240_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z240_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z240_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z240_CNG19);
          break;
        }
        case 256:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z256_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z256_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z256_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z256_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z256_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z256_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z256_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z256_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z256_CNG19);
          break;
        }
        case 288:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z288_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z288_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z288_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z288_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z288_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z288_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z288_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z288_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z288_CNG19);
          break;
        }
        case 320:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z320_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z320_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z320_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z320_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z320_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z320_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z320_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z320_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z320_CNG19);
          break;
        }
        case 352:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z352_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z352_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z352_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z352_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z352_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z352_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z352_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z352_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z352_CNG19);
          break;
        }
        case 384:
        {
          p_lut->circShift[0] = expandArr16(circShift_BG1_Z384_CNG3);
          p_lut->circShift[1] = expandArr16(circShift_BG1_Z384_CNG4);
          p_lut->circShift[2] = expandArr16(circShift_BG1_Z384_CNG5);
          p_lut->circShift[3] = expandArr16(circShift_BG1_Z384_CNG6);
          p_lut->circShift[4] = expandArr16(circShift_BG1_Z384_CNG7);
          p_lut->circShift[5] = expandArr16(circShift_BG1_Z384_CNG8);
          p_lut->circShift[6] = expandArr16(circShift_BG1_Z384_CNG9);
          p_lut->circShift[7] = expandArr16(circShift_BG1_Z384_CNG10);
          p_lut->circShift[8] = expandArr16(circShift_BG1_Z384_CNG19);
          break;
        }
        default: {
        }
        }
    }

    return numLLR;
}

#endif
