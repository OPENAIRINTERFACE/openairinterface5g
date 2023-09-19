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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../../nrLDPCdecoder_defs.h"

void nrLDPC_cnProc_BG1_generator_AVX512(const char *dir, int R)
{
  const char *ratestr[3] = {"13", "23", "89"};

  if (R < 0 || R > 2) {
    printf("Illegal R %d\n", R);
    abort();
  }

  // system("mkdir -p ../ldpc_gen_files");

  char fname[FILENAME_MAX + 1];
  snprintf(fname, sizeof(fname), "%s/cnProc_avx512/nrLDPC_cnProc_BG1_R%s_AVX512.h", dir, ratestr[R]);
  FILE *fd = fopen(fname, "w");
  if (fd == NULL) {
    printf("Cannot create file %s\n", fname);
    abort();
  }

  // fprintf(fd,"#include <stdint.h>\n");
  // fprintf(fd,"#include \"PHY/sse_intrin.h\"\n");

  // fprintf(fd,   "#define conditional_negate(a,b,z) simde_mm512_mask_sub_epi8(a,simde_mm512_movepi8_mask(b),z,a)\n");

  fprintf(fd, "static inline void nrLDPC_cnProc_BG1_R%s_AVX512(int8_t* cnProcBuf, int8_t* cnProcBufRes, uint16_t Z) {\n", ratestr[R]);

  const uint8_t *lut_numCnInCnGroups;
  const uint32_t *lut_startAddrCnGroups = lut_startAddrCnGroups_BG1;

  if (R == 0)
    lut_numCnInCnGroups = lut_numCnInCnGroups_BG1_R13;
  else if (R == 1)
    lut_numCnInCnGroups = lut_numCnInCnGroups_BG1_R23;
  else if (R == 2)
    lut_numCnInCnGroups = lut_numCnInCnGroups_BG1_R89;
  else {
    printf("aborting, illegal R %d\n", R);
    fclose(fd);
    abort();
  }

  uint32_t j;
  uint32_t k;
  // Offset to each bit within a group in terms of 64  Byte
  uint32_t bitOffsetInGroup;

  fprintf(fd, "                uint32_t M, i;\n");
  fprintf(fd, "                simde__m512i zmm0, min, sgn,zeros,maxLLR, ones;\n");

  fprintf(fd, "                  zeros  = simde_mm512_setzero_si512();\n");
  fprintf(fd, "                  maxLLR = simde_mm512_set1_epi8((char)127);\n");
  fprintf(fd, "                 ones = simde_mm512_set1_epi8((char)1);\n");

  // =====================================================================
  // Process group with 3 BNs
  fprintf(fd, "//Process group with 3 BNs\n");
  // LUT with offsets for bits that need to be processed
  // 1. bit proc requires LLRs of 2. and 3. bit, 2.bits of 1. and 3. etc.
  // Offsets are in units of bitOffsetInGroup (1*384/32)12
  // Offsets are in units of bitOffsetInGroup (1*384/32)12

  const uint8_t lut_idxCnProcG3[3][2] = {{12, 24}, {0, 24}, {0, 12}};

  if (lut_numCnInCnGroups[0] > 0) {
    // Number of groups of 64  CNs for parallel processing
    // Ceil for values not divisible by 64
    // M = (lut_numCnInCnGroups[0]*Z + 63)>>6;

    fprintf(fd, " M = (%d*Z + 63)>>6;\n", lut_numCnInCnGroups[0]);

    // Set the offset to each bit within a group in terms of 64  Byte
    bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[0] * NR_LDPC_ZMAX) >> 6;

    // Loop over every BN

    for (j = 0; j < 3; j++) {
      fprintf(fd, "            for (i=0;i<M;i++) {\n");
      // Abs and sign of 64  CNs (first BN)
      //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
      fprintf(fd,
              "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
              (lut_startAddrCnGroups[0] >> 6) + lut_idxCnProcG3[j][0] / 2);
      fprintf(fd, "                sgn  = simde_mm512_xor_si512(ones, zmm0);\n");
      fprintf(fd, "                min  = simde_mm512_abs_epi8(zmm0);\n");

      // for (k=1; k<2; k++)
      //{
      fprintf(fd,
              "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
              (lut_startAddrCnGroups[0] >> 6) + lut_idxCnProcG3[j][1] / 2);

      //                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
      fprintf(fd, "                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));\n");

      //                sgn  = simde_mm512_sign_epi8(*p_ones, zmm0);
      fprintf(fd, "                sgn  = simde_mm512_xor_si512(sgn, zmm0);\n");
      // }

      // Store result
      //                min = simde_mm512_min_epu8(min, *maxLLR); // 128 in epi8 is -127
      fprintf(fd, "                min = simde_mm512_min_epu8(min, maxLLR);\n");
      //                *p_cnProcBufResBit = simde_mm512_sign_epi8(min, sgn);
      //                p_cnProcBufResBit++;
      fprintf(fd,
              "                ((simde__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",
              (lut_startAddrCnGroups[0] >> 6) + (j * bitOffsetInGroup));
      fprintf(fd, "            }\n");
    }
  }

  // =====================================================================
  // Process group with 4 BNs
  fprintf(fd, "//Process group with 4 BNs\n");
  // Offset is 5*384/32 = 30
  const uint8_t lut_idxCnProcG4[4][3] = {{60, 120, 180}, {0, 120, 180}, {0, 60, 180}, {0, 60, 120}};

  if (lut_numCnInCnGroups[1] > 0) {
    // Number of groups of 64  CNs for parallel processing
    // Ceil for values not divisible by 64
    // M = (lut_numCnInCnGroups[1]*Z + 63)>>6;
    fprintf(fd, " M = (%d*Z + 63)>>6;\n", lut_numCnInCnGroups[1]);
    // Set the offset to each bit within a group in terms of 64  Byte
    bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[1] * NR_LDPC_ZMAX) >> 6;

    // Loop over every BN
    for (j = 0; j < 4; j++) {
      // Loop over CNs
      //      for (i=0; i<M; i++,iprime++)
      //            {
      fprintf(fd, "            for (i=0;i<M;i++) {\n");
      // Abs and sign of 64  CNs (first BN)
      //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
      fprintf(fd,
              "              zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
              (lut_startAddrCnGroups[1] >> 6) + lut_idxCnProcG4[j][0] / 2);
      fprintf(fd, "                sgn  = simde_mm512_xor_si512(ones, zmm0);\n");
      fprintf(fd, "                min  = simde_mm512_abs_epi8(zmm0);\n");

      // Loop over BNs
      for (k = 1; k < 3; k++) {
        fprintf(fd,
                "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
                (lut_startAddrCnGroups[1] >> 6) + lut_idxCnProcG4[j][k] / 2);

        //                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
        fprintf(fd, "                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));\n");

        //                sgn  = simde_mm512_sign_epi8(*p_ones, zmm0);
        fprintf(fd, "                sgn  = simde_mm512_xor_si512(sgn, zmm0);\n");
      }

      // Store result
      //                min = simde_mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
      fprintf(fd, "                min = simde_mm512_min_epu8(min, maxLLR);\n");
      //                *p_cnProcBufResBit = simde_mm512_sign_epi8(min, sgn);
      //                p_cnProcBufResBit++;
      fprintf(fd,
              "                ((simde__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",
              (lut_startAddrCnGroups[1] >> 6) + (j * bitOffsetInGroup));
      fprintf(fd, "            }\n");
    }
  }

  // =====================================================================
  // Process group with 5 BNs
  fprintf(fd, "//Process group with 5 BNs\n");
  // Offset is 18*384/32 = 216
  const uint16_t lut_idxCnProcG5[5][4] = {{216, 432, 648, 864}, {0, 432, 648, 864}, {0, 216, 648, 864}, {0, 216, 432, 864}, {0, 216, 432, 648}};

  if (lut_numCnInCnGroups[2] > 0) {
    // Number of groups of 64  CNs for parallel processing
    // Ceil for values not divisible by 64
    // M = (lut_numCnInCnGroups[2]*Z + 63)>>6;
    fprintf(fd, " M = (%d*Z + 63)>>6;\n", lut_numCnInCnGroups[2]);

    // Set the offset to each bit within a group in terms of 64  Byte
    bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[2] * NR_LDPC_ZMAX) >> 6;

    // Loop over every BN

    for (j = 0; j < 5; j++) {
      fprintf(fd, "            for (i=0;i<M;i++) {\n");
      // Abs and sign of 64  CNs (first BN)
      //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
      fprintf(fd,
              "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
              (lut_startAddrCnGroups[2] >> 6) + lut_idxCnProcG5[j][0] / 2);
      fprintf(fd, "                sgn  = simde_mm512_xor_si512(ones, zmm0);\n");
      fprintf(fd, "                min  = simde_mm512_abs_epi8(zmm0);\n");

      // Loop over BNs
      for (k = 1; k < 4; k++) {
        fprintf(fd,
                "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
                (lut_startAddrCnGroups[2] >> 6) + lut_idxCnProcG5[j][k] / 2);

        //                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
        fprintf(fd, "                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));\n");

        //                sgn  = simde_mm512_sign_epi8(*p_ones, zmm0);
        fprintf(fd, "                sgn  = simde_mm512_xor_si512(sgn, zmm0);\n");
      }

      // Store result
      //                min = simde_mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
      fprintf(fd, "                min = simde_mm512_min_epu8(min, maxLLR);\n");

      fprintf(fd,
              "                ((simde__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",
              (lut_startAddrCnGroups[2] >> 6) + (j * bitOffsetInGroup));
      fprintf(fd, "           }\n");
    }
  }

  // =====================================================================
  // Process group with 6 BNs
  fprintf(fd, "//Process group with 6 BNs\n");
  // Offset is 8*384/32 = 48
  const uint16_t lut_idxCnProcG6[6][5] = {{96, 192, 288, 384, 480}, {0, 192, 288, 384, 480}, {0, 96, 288, 384, 480}, {0, 96, 192, 384, 480}, {0, 96, 192, 288, 480}, {0, 96, 192, 288, 384}};

  if (lut_numCnInCnGroups[3] > 0) {
    // Number of groups of 64  CNs for parallel processing
    // Ceil for values not divisible by 64
    // M = (lut_numCnInCnGroups[3]*Z + 63)>>6;

    fprintf(fd, "M = (%d*Z + 63)>>6;\n", lut_numCnInCnGroups[3]);

    // Set the offset to each bit within a group in terms of 64  Byte
    bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[3] * NR_LDPC_ZMAX) >> 6;

    // Loop over every BN

    for (j = 0; j < 6; j++) {
      fprintf(fd, "            for (i=0;i<M;i++) {\n");
      // Abs and sign of 64  CNs (first BN)
      //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
      fprintf(fd,
              "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
              (lut_startAddrCnGroups[3] >> 6) + lut_idxCnProcG6[j][0] / 2);
      fprintf(fd, "                sgn  = simde_mm512_xor_si512(ones, zmm0);\n");
      fprintf(fd, "                min  = simde_mm512_abs_epi8(zmm0);\n");

      // Loop over BNs
      for (k = 1; k < 5; k++) {
        fprintf(fd,
                "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
                (lut_startAddrCnGroups[3] >> 6) + lut_idxCnProcG6[j][k] / 2);

        //                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
        fprintf(fd, "                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));\n");

        //                sgn  = simde_mm512_sign_epi8(*p_ones, zmm0);
        fprintf(fd, "                sgn  = simde_mm512_xor_si512(sgn, zmm0);\n");
      }

      // Store result
      //                min = simde_mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
      fprintf(fd, "                min = simde_mm512_min_epu8(min, maxLLR);\n");

      fprintf(fd,
              "                ((simde__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",
              (lut_startAddrCnGroups[3] >> 6) + (j * bitOffsetInGroup));
      fprintf(fd, "            }\n");
    }
  }

  // =====================================================================
  // Process group with 7 BNs
  fprintf(fd, "//Process group with 7 BNs\n");
  // Offset is 5*384/32 = 30
  const uint16_t lut_idxCnProcG7[7][6] = {{60, 120, 180, 240, 300, 360},
                                          {0, 120, 180, 240, 300, 360},
                                          {0, 60, 180, 240, 300, 360},
                                          {0, 60, 120, 240, 300, 360},
                                          {0, 60, 120, 180, 300, 360},
                                          {0, 60, 120, 180, 240, 360},
                                          {0, 60, 120, 180, 240, 300}};

  if (lut_numCnInCnGroups[4] > 0) {
    // Number of groups of 64  CNs for parallel processing
    // Ceil for values not divisible by 64
    // M = (lut_numCnInCnGroups[4]*Z + 63)>>6;
    fprintf(fd, "M = (%d*Z + 63)>>6;\n", lut_numCnInCnGroups[4]);

    // Set the offset to each bit within a group in terms of 64  Byte
    bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[4] * NR_LDPC_ZMAX) >> 6;

    // Loop over every BN

    for (j = 0; j < 7; j++) {
      // Loop over CNs
      //      for (i=0; i<M; i++,iprime++)
      //            {
      fprintf(fd, "            for (i=0;i<M;i++) {\n");
      // Abs and sign of 64  CNs (first BN)
      //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
      fprintf(fd,
              "                zmm0= ((simde__m512i*)cnProcBuf)[%d+i];\n",
              (lut_startAddrCnGroups[4] >> 6) + lut_idxCnProcG7[j][0] / 2);
      fprintf(fd, "                sgn  = simde_mm512_xor_si512(ones, zmm0);\n");
      fprintf(fd, "                min  = simde_mm512_abs_epi8(zmm0);\n");

      // Loop over BNs
      for (k = 1; k < 6; k++) {
        fprintf(fd,
                "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
                (lut_startAddrCnGroups[4] >> 6) + lut_idxCnProcG7[j][k] / 2);

        //                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
        fprintf(fd, "                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));\n");

        //                sgn  = simde_mm512_sign_epi8(*p_ones, zmm0);
        fprintf(fd, "                sgn  = simde_mm512_xor_si512(sgn, zmm0);\n");
      }

      // Store result
      //                min = simde_mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
      fprintf(fd, "                min = simde_mm512_min_epu8(min, maxLLR);\n");

      fprintf(fd,
              "                ((simde__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",
              (lut_startAddrCnGroups[4] >> 6) + (j * bitOffsetInGroup));
      fprintf(fd, "            }\n");
    }
  }

  // =====================================================================
  // Process group with 8 BNs
  fprintf(fd, "//Process group with 8 BNs\n");
  // Offset is 2*384/32 = 24
  const uint8_t lut_idxCnProcG8[8][7] = {{24, 48, 72, 96, 120, 144, 168},
                                         {0, 48, 72, 96, 120, 144, 168},
                                         {0, 24, 72, 96, 120, 144, 168},
                                         {0, 24, 48, 96, 120, 144, 168},
                                         {0, 24, 48, 72, 120, 144, 168},
                                         {0, 24, 48, 72, 96, 144, 168},
                                         {0, 24, 48, 72, 96, 120, 168},
                                         {0, 24, 48, 72, 96, 120, 144}};

  if (lut_numCnInCnGroups[5] > 0) {
    // Number of groups of 64  CNs for parallel processing
    // Ceil for values not divisible by 64
    // M = (lut_numCnInCnGroups[5]*Z + 63)>>6;
    fprintf(fd, "M = (%d*Z + 63)>>6;\n", lut_numCnInCnGroups[5]);

    // Set the offset to each bit within a group in terms of 64  Byte
    bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[5] * NR_LDPC_ZMAX) >> 6;

    // Loop over every BN

    for (j = 0; j < 8; j++) {
      // Loop over CNs
      //      for (i=0; i<M; i++,iprime++)
      //            {
      fprintf(fd, "            for (i=0;i<M;i++) {\n");
      // Abs and sign of 64  CNs (first BN)
      //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
      fprintf(fd,
              "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
              (lut_startAddrCnGroups[5] >> 6) + lut_idxCnProcG8[j][0] / 2);
      fprintf(fd, "                sgn  = simde_mm512_xor_si512(ones, zmm0);\n");
      fprintf(fd, "                min  = simde_mm512_abs_epi8(zmm0);\n");

      // Loop over BNs
      for (k = 1; k < 7; k++) {
        fprintf(fd,
                "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
                (lut_startAddrCnGroups[5] >> 6) + lut_idxCnProcG8[j][k] / 2);

        //                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
        fprintf(fd, "                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));\n");

        //                sgn  = simde_mm512_sign_epi8(*p_ones, zmm0);
        fprintf(fd, "                sgn  = simde_mm512_xor_si512(sgn, zmm0);\n");
      }

      // Store result
      //                min = simde_mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
      fprintf(fd, "                min = simde_mm512_min_epu8(min, maxLLR);\n");

      fprintf(fd,
              "                ((simde__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",
              (lut_startAddrCnGroups[5] >> 6) + (j * bitOffsetInGroup));
      fprintf(fd, "              }\n");
    }
  }

  // =====================================================================
  // Process group with 9 BNs

  fprintf(fd, "//Process group with 9 BNs\n");
  // Offset is 2*384/32 = 12
  const uint8_t lut_idxCnProcG9[9][8] = {{24, 48, 72, 96, 120, 144, 168, 192},
                                         {0, 48, 72, 96, 120, 144, 168, 192},
                                         {0, 24, 72, 96, 120, 144, 168, 192},
                                         {0, 24, 48, 96, 120, 144, 168, 192},
                                         {0, 24, 48, 72, 120, 144, 168, 192},
                                         {0, 24, 48, 72, 96, 144, 168, 192},
                                         {0, 24, 48, 72, 96, 120, 168, 192},
                                         {0, 24, 48, 72, 96, 120, 144, 192},
                                         {0, 24, 48, 72, 96, 120, 144, 168}};

  if (lut_numCnInCnGroups[6] > 0) {
    // Number of groups of 64  CNs for parallel processing
    // Ceil for values not divisible by 64
    // M = (lut_numCnInCnGroups[5]*Z + 63)>>6;
    fprintf(fd, "M = (%d*Z + 63)>>6;\n", lut_numCnInCnGroups[6]);

    // Set the offset to each bit within a group in terms of 64  Byte
    bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[6] * NR_LDPC_ZMAX) >> 6;

    // Loop over every BN

    for (j = 0; j < 9; j++) {
      // Loop over CNs
      //      for (i=0; i<M; i++,iprime++)
      //            {
      fprintf(fd, "            for (i=0;i<M;i++) {\n");
      // Abs and sign of 64  CNs (first BN)
      //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
      fprintf(fd,
              "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
              (lut_startAddrCnGroups[6] >> 6) + lut_idxCnProcG9[j][0] / 2);
      fprintf(fd, "                sgn  = simde_mm512_xor_si512(ones, zmm0);\n");
      fprintf(fd, "                min  = simde_mm512_abs_epi8(zmm0);\n");

      // Loop over BNs
      for (k = 1; k < 8; k++) {
        fprintf(fd,
                "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
                (lut_startAddrCnGroups[6] >> 6) + lut_idxCnProcG9[j][k] / 2);

        //                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
        fprintf(fd, "                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));\n");

        //                sgn  = simde_mm512_sign_epi8(*p_ones, zmm0);
        fprintf(fd, "                sgn  = simde_mm512_xor_si512(sgn, zmm0);\n");
      }

      // Store result
      //                min = simde_mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
      fprintf(fd, "                min = simde_mm512_min_epu8(min, maxLLR);\n");

      fprintf(fd,
              "                ((simde__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",
              (lut_startAddrCnGroups[6] >> 6) + (j * bitOffsetInGroup));
      fprintf(fd, "              }\n");
    }
  }

  // =====================================================================
  // Process group with 10 BNs
  fprintf(fd, "//Process group with 10 BNs\n");
  // Offset is 1*384/32 = 6
  const uint8_t lut_idxCnProcG10[10][9] = {{12, 24, 36, 48, 60, 72, 84, 96, 108},
                                           {0, 24, 36, 48, 60, 72, 84, 96, 108},
                                           {0, 12, 36, 48, 60, 72, 84, 96, 108},
                                           {0, 12, 24, 48, 60, 72, 84, 96, 108},
                                           {0, 12, 24, 36, 60, 72, 84, 96, 108},
                                           {0, 12, 24, 36, 48, 72, 84, 96, 108},
                                           {0, 12, 24, 36, 48, 60, 84, 96, 108},
                                           {0, 12, 24, 36, 48, 60, 72, 96, 108},
                                           {0, 12, 24, 36, 48, 60, 72, 84, 108},
                                           {0, 12, 24, 36, 48, 60, 72, 84, 96}};

  if (lut_numCnInCnGroups[7] > 0) {
    // Number of groups of 64  CNs for parallel processing
    // Ceil for values not divisible by 64
    // M = (lut_numCnInCnGroups[7]*Z + 63)>>6;
    fprintf(fd, " M = (%d*Z + 63)>>6;\n", lut_numCnInCnGroups[7]);

    // Set the offset to each bit within a group in terms of 64  Byte
    bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[7] * NR_LDPC_ZMAX) >> 6;

    // Loop over every BN

    for (j = 0; j < 10; j++) {
      // Loop over CNs
      //      for (i=0; i<M; i++,iprime++)
      //            {
      fprintf(fd, "            for (i=0;i<M;i++) {\n");
      // Abs and sign of 64  CNs (first BN)
      //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
      fprintf(fd,
              "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
              (lut_startAddrCnGroups[7] >> 6) + lut_idxCnProcG10[j][0] / 2);
      fprintf(fd, "                sgn  = simde_mm512_xor_si512(ones, zmm0);\n");
      fprintf(fd, "                min  = simde_mm512_abs_epi8(zmm0);\n");

      // Loop over BNs
      for (k = 1; k < 9; k++) {
        fprintf(fd,
                "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
                (lut_startAddrCnGroups[7] >> 6) + lut_idxCnProcG10[j][k] / 2);

        //                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
        fprintf(fd, "                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));\n");

        //                sgn  = simde_mm512_sign_epi8(*p_ones, zmm0);
        fprintf(fd, "                sgn  = simde_mm512_xor_si512(sgn, zmm0);\n");
      }

      // Store result
      //                min = simde_mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
      fprintf(fd, "                min = simde_mm512_min_epu8(min, maxLLR);\n");

      fprintf(fd,
              "                ((simde__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min,sgn,zeros);\n",
              (lut_startAddrCnGroups[7] >> 6) + (j * bitOffsetInGroup));
      fprintf(fd, "            }\n");
    }
  }

  // =====================================================================
  // Process group with 19 BNs
  fprintf(fd, "//Process group with 19 BNs\n");
  // Offset is 4*384/32 = 24
  const uint16_t lut_idxCnProcG19[19][18] = {{48, 96, 144, 192, 240, 288, 336, 384, 432, 480, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 96, 144, 192, 240, 288, 336, 384, 432, 480, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 144, 192, 240, 288, 336, 384, 432, 480, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 192, 240, 288, 336, 384, 432, 480, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 240, 288, 336, 384, 432, 480, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 288, 336, 384, 432, 480, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 336, 384, 432, 480, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 384, 432, 480, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 432, 480, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 384, 480, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 384, 432, 528, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 384, 432, 480, 576, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 384, 432, 480, 528, 624, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 384, 432, 480, 528, 576, 672, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 384, 432, 480, 528, 576, 624, 720, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 384, 432, 480, 528, 576, 624, 672, 768, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 384, 432, 480, 528, 576, 624, 672, 720, 816, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 384, 432, 480, 528, 576, 624, 672, 720, 768, 864},
                                             {0, 48, 96, 144, 192, 240, 288, 336, 384, 432, 480, 528, 576, 624, 672, 720, 768, 816}};

  if (lut_numCnInCnGroups[8] > 0) {
    // Number of groups of 64  CNs for parallel processing
    // Ceil for values not divisible by 64
    // M = (lut_numCnInCnGroups[8]*Z + 63)>>6;
    fprintf(fd, " M = (%d*Z + 63)>>6;\n", lut_numCnInCnGroups[8]);

    // Set the offset to each bit within a group in terms of 64  Byte
    bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[8] * NR_LDPC_ZMAX) >> 6;

    // Loop over every BN

    for (j = 0; j < 19; j++) {
      // Loop over CNs
      //      for (i=0; i<M; i++,iprime++)
      //            {
      fprintf(fd, "            for (i=0;i<M;i++) {\n");
      // Abs and sign of 64  CNs (first BN)
      //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
      fprintf(fd,
              "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
              (lut_startAddrCnGroups[8] >> 6) + lut_idxCnProcG19[j][0] / 2);
      fprintf(fd, "                sgn  = simde_mm512_xor_si512(ones, zmm0);\n");
      fprintf(fd, "                min  = simde_mm512_abs_epi8(zmm0);\n");

      // Loop over BNs
      for (k = 1; k < 18; k++) {
        fprintf(fd,
                "                zmm0 = ((simde__m512i*)cnProcBuf)[%d+i];\n",
                (lut_startAddrCnGroups[8] >> 6) + lut_idxCnProcG19[j][k] / 2);

        //                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));
        fprintf(fd, "                min  = simde_mm512_min_epu8(min, simde_mm512_abs_epi8(zmm0));\n");

        //                sgn  = simde_mm512_sign_epi8(*p_ones, zmm0);
        fprintf(fd, "                sgn  = simde_mm512_xor_si512(sgn, zmm0);\n");
      }

      // Store result
      //                min = simde_mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
      fprintf(fd, "                min = simde_mm512_min_epu8(min, maxLLR);\n");

      fprintf(fd,
              "                ((simde__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",
              (lut_startAddrCnGroups[8] >> 6) + (j * bitOffsetInGroup));
      fprintf(fd, "            }\n");
    }
  }

  fprintf(fd, "}\n");
  fclose(fd);
} // end of the function  nrLDPC_cnProc_BG1
