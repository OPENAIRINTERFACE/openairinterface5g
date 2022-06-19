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
#include <immintrin.h>
#include "../../nrLDPCdecoder_defs.h"
#include "../../nrLDPC_types.h"


void nrLDPC_bnProcPc_BG2_generator_AVX2(const char *dir, int R)
{
  const char *ratestr[3]={"15","13","23"};

  if (R<0 || R>2) {printf("Illegal R %d\n",R); abort();}


 // system("mkdir -p ../ldpc_gen_files");

  char fname[FILENAME_MAX+1];
  snprintf(fname, sizeof(fname), "%s/bnProcPc/nrLDPC_bnProcPc_BG2_R%s_AVX2.h", dir, ratestr[R]);
  FILE *fd=fopen(fname,"w");
  if (fd == NULL) {
    printf("Cannot create file %s\n", fname);
    abort();
  }

//  fprintf(fd,"#include <stdint.h>\n");
  //fprintf(fd,"#include <immintrin.h>\n");

  fprintf(fd,"static inline void nrLDPC_bnProcPc_BG2_R%s_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,int8_t* llrRes ,  int8_t* llrProcBuf, uint16_t Z  ) {\n",ratestr[R]);
    const uint8_t*  lut_numBnInBnGroups;
    const uint32_t* lut_startAddrBnGroups;
    const uint16_t* lut_startAddrBnGroupsLlr;
    if (R==0) {


      lut_numBnInBnGroups =  lut_numBnInBnGroups_BG2_R15;
      lut_startAddrBnGroups = lut_startAddrBnGroups_BG2_R15;
      lut_startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG2_R15;

    }
    else if (R==1){

      lut_numBnInBnGroups =  lut_numBnInBnGroups_BG2_R13;
      lut_startAddrBnGroups = lut_startAddrBnGroups_BG2_R13;
      lut_startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG2_R13;
    }
    else if (R==2) {

      lut_numBnInBnGroups = lut_numBnInBnGroups_BG2_R23;
      lut_startAddrBnGroups = lut_startAddrBnGroups_BG2_R23;
      lut_startAddrBnGroupsLlr = lut_startAddrBnGroupsLlr_BG2_R23;
    }
  else { printf("aborting, illegal R %d\n",R); fclose(fd);abort();}




    uint32_t k;
    // Offset to each bit within a group in terms of 32 Byte
    uint32_t cnOffsetInGroup;
    uint8_t idxBnGroup = 0;

    fprintf(fd,"   __m256i ymm0, ymm1, ymmRes0, ymmRes1;  \n");


    fprintf(fd,"        __m128i* p_bnProcBuf; \n");
    fprintf(fd,"        __m128i* p_llrProcBuf;\n");
    fprintf(fd,"        __m256i* p_llrRes; \n");
   // fprintf(fd,"        __m256i* p_bnProcBufRes; \n");
   // fprintf(fd,"        __m256i* p_llrProcBuf256; \n");
    fprintf(fd,"         uint32_t M ;\n");


    fprintf(fd,  "// Process group with 1 CNs \n");
/*

 // Process group with 1 CNs

   // if (lut_numBnInBnGroups[0] > 0)
   // {
        // If elements in group move to next address
       // idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[0] );

        // Set the offset to each CN within a group in terms of 16 Byte
       // cnOffsetInGroup = (lut_numBnInBnGroups[0]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_bnProcBufRes     = (__m256i*) &bnProcBufRes    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
         fprintf(fd,"   p_llrProcBuf256   = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");

         fprintf(fd,"           p_bnProcBufRes[i] = p_llrProcBuf256[i];\n");

            // First 16 LLRs of first CN
        fprintf(fd,"            ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"            ymm1 = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"           ymmRes0 = _mm256_adds_epi16(ymm0, ymm1);\n");


            // Second 16 LLRs of first CN
        fprintf(fd,"            ymm0    = _mm256_cvtepi8_epi16(p_bnProcBuf[j + 1 ]);\n");
        fprintf(fd,"            ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j + 1 ]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_adds_epi16(ymm0, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"            ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");


        fprintf(fd,"}\n");
    //}
  */    // =====================================================================
    // Process group with 2 CNs


fprintf(fd,  "// Process group with 2 CNs \n");

 // Process group with 2 CNs

    if (lut_numBnInBnGroups[1] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[1] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[1]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"            ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf[j + 1]);\n");

            // Loop over CNs
        for (k=1; k<2; k++)
        {
        fprintf(fd,"            ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"            ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "           ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"            ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"            ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"            ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");


        fprintf(fd,"}\n");
    }

    // =====================================================================
    // Process group with 3 CNs


fprintf(fd,  "// Process group with 3 CNs \n");

 // Process group with 3 CNs

    if (lut_numBnInBnGroups[2] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[2] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[2]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<3; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
            }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }



    // =====================================================================
    // Process group with 4 CNs

fprintf(fd,  "// Process group with 4 CNs \n");

 // Process group with 4 CNs

    if (lut_numBnInBnGroups[3] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[3] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[3]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<4; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 5 CNs

fprintf(fd,  "// Process group with 5 CNs \n");

 // Process group with 5 CNs

    if (lut_numBnInBnGroups[4] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[4] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[4]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<5; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }



   // =====================================================================
    // Process group with 6 CNs

fprintf(fd,  "// Process group with 6 CNs \n");

 // Process group with 6 CNs

    if (lut_numBnInBnGroups[5] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[5] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[5]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<6; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 7 CNs

fprintf(fd,  "// Process group with 7 CNs \n");

 // Process group with 7 CNs

    if (lut_numBnInBnGroups[6] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[6] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[6]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<7; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        //fprintf(fd,"         (__m256i*) &llrRes[%d + i]    = _mm256_permute4x64_epi64(ymm0, 0xD8);\n",lut_startAddrBnGroupsLlr[idxBnGroup]>>5 );
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 8 CNs

fprintf(fd,  "// Process group with 8 CNs \n");

 // Process group with 8 CNs

    if (lut_numBnInBnGroups[7] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[7] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[7]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<8; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        //fprintf(fd,"         (__m256i*) &llrRes[%d + i]    = _mm256_permute4x64_epi64(ymm0, 0xD8);\n",lut_startAddrBnGroupsLlr[idxBnGroup]>>5 );

        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }

   // =====================================================================
    // Process group with 9 CNs

fprintf(fd,  "// Process group with 9 CNs \n");

 // Process group with 9 CNs

    if (lut_numBnInBnGroups[8] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[8] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[8]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<9; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        //fprintf(fd,"         (__m256i*) &llrRes[%d + i]    = _mm256_permute4x64_epi64(ymm0, 0xD8);\n",lut_startAddrBnGroupsLlr[idxBnGroup]>>5 );
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 10 CNs

fprintf(fd,  "// Process group with 10 CNs \n");

 // Process group with 10 CNs

    if (lut_numBnInBnGroups[9] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[9] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[9]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<10; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }



    // =====================================================================

fprintf(fd,  "// Process group with 11 CNs \n");



 // Process group with 2 CNs

    if (lut_numBnInBnGroups[10] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[10] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[10]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"            ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<11; k++)
        {
        fprintf(fd,"            ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"            ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "           ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"            ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"            ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"            ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }
      // =====================================================================
    // Process group with 2 CNs


fprintf(fd,  "// Process group with 12 CNs \n");

 // Process group with 2 CNs

    if (lut_numBnInBnGroups[11] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[11] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[11]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"            ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<12; k++)
        {
        fprintf(fd,"            ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"            ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "           ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"            ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"            ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"            ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }

    // =====================================================================
    // Process group with 13 CNs


fprintf(fd,  "// Process group with 13 CNs \n");

 // Process group with 3 CNs

    if (lut_numBnInBnGroups[12] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[12] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[12]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<13; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
            }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }



    // =====================================================================
    // Process group with 4 CNs

fprintf(fd,  "// Process group with 14 CNs \n");

 // Process group with 4 CNs

    if (lut_numBnInBnGroups[13] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[13] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[13]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<14; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 5 CNs

fprintf(fd,  "// Process group with 15 CNs \n");

 // Process group with 5 CNs

    if (lut_numBnInBnGroups[14] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[14] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[14]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<15; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
         fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }



   // =====================================================================
    // Process group with 6 CNs

fprintf(fd,  "// Process group with 16 CNs \n");

 // Process group with 6 CNs

    if (lut_numBnInBnGroups[15] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[15] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[15]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<16; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 17 CNs

fprintf(fd,  "// Process group with 17 CNs \n");

 // Process group with 17 CNs

    if (lut_numBnInBnGroups[16] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[16] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[16]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<17; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 18 CNs

fprintf(fd,  "// Process group with 18 CNs \n");

 // Process group with 8 CNs

    if (lut_numBnInBnGroups[17] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[17] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[17]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<18; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }

   // =====================================================================
    // Process group with 9 CNs

fprintf(fd,  "// Process group with 19 CNs \n");

 // Process group with 9 CNs

    if (lut_numBnInBnGroups[18] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[18] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[18]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<19; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 20 CNs

fprintf(fd,  "// Process group with 20 CNs \n");

 // Process group with 20 CNs

    if (lut_numBnInBnGroups[19] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[19] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[19]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<20; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }





    // =====================================================================

fprintf(fd,  "// Process group with 21 CNs \n");



 // Process group with 2 CNs

    if (lut_numBnInBnGroups[20] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[20] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[20]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"            ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<21; k++)
        {
        fprintf(fd,"            ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"            ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "           ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"            ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"            ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"            ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }
      // =====================================================================
    // Process group with 2 CNs


fprintf(fd,  "// Process group with 22 CNs \n");

 // Process group with 2 CNs

    if (lut_numBnInBnGroups[21] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[21] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[21]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"            ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<22; k++)
        {
        fprintf(fd,"            ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"            ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "           ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"            ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"            ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"            ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"            ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"            ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
         fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }

    // =====================================================================
    // Process group with 13 CNs


fprintf(fd,  "// Process group with <23 CNs \n");

 // Process group with 3 CNs

    if (lut_numBnInBnGroups[22] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[22] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[22]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<23; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }



    // =====================================================================
    // Process group with 4 CNs

fprintf(fd,  "// Process group with 24 CNs \n");

 // Process group with 4 CNs

    if (lut_numBnInBnGroups[23] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[23] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[23]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<24; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 5 CNs

fprintf(fd,  "// Process group with 25 CNs \n");

 // Process group with 5 CNs

    if (lut_numBnInBnGroups[24] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[24] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[24]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<25; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }



   // =====================================================================
    // Process group with 6 CNs

fprintf(fd,  "// Process group with 26 CNs \n");

 // Process group with 6 CNs

    if (lut_numBnInBnGroups[25] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[25] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[25]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<26; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 17 CNs

fprintf(fd,  "// Process group with 27 CNs \n");

 // Process group with 17 CNs

    if (lut_numBnInBnGroups[26] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[26] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[26]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<27; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 18 CNs

fprintf(fd,  "// Process group with 28 CNs \n");

 // Process group with 8 CNs

    if (lut_numBnInBnGroups[27] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[27] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[27]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<28; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }

   // =====================================================================
    // Process group with 9 CNs

fprintf(fd,  "// Process group with 29 CNs \n");

 // Process group with 9 CNs

    if (lut_numBnInBnGroups[28] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[28] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[28]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<29; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }


   // =====================================================================
    // Process group with 20 CNs

fprintf(fd,  "// Process group with 30 CNs \n");

 // Process group with 20 CNs

    if (lut_numBnInBnGroups[29] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numBnInBnGroups[29] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[29]*NR_LDPC_ZMAX)>>4;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m128i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m128i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m256i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"        ymmRes0 = _mm256_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<30; k++)
        {
        fprintf(fd,"        ymm0 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1 = _mm256_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "       ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"        ymm0    = _mm256_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"        ymmRes0 = _mm256_adds_epi16(ymmRes0, ymm0);\n");

        fprintf(fd,"        ymm1    = _mm256_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"        ymmRes1 = _mm256_adds_epi16(ymmRes1, ymm1);\n");

            // Pack results back to epi8
        fprintf(fd,"        ymm0 = _mm256_packs_epi16(ymmRes0, ymmRes1);\n");
            // ymm0     = [ymmRes1[255:128] ymmRes0[255:128] ymmRes1[127:0] ymmRes0[127:0]]
            // p_llrRes = [ymmRes1[255:128] ymmRes1[127:0] ymmRes0[255:128] ymmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm256_permute4x64_epi64(ymm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }

    fprintf(fd,"}\n");
  fclose(fd);
}//end of the function  nrLDPC_bnProcPc_BG2








