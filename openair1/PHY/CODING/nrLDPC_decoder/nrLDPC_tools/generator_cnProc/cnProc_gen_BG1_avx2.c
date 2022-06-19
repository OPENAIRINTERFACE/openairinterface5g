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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "../../nrLDPCdecoder_defs.h"

void nrLDPC_cnProc_BG1_generator_AVX2(const char* dir, int R)
{
  const char *ratestr[3]={"13","23","89"};

  if (R<0 || R>2) {printf("Illegal R %d\n",R); abort();}


 // system("mkdir -p ../ldpc_gen_files");

  char fname[FILENAME_MAX+1];
  snprintf(fname, sizeof(fname), "%s/cnProc/nrLDPC_cnProc_BG1_R%s_AVX2.h", dir, ratestr[R]);
  FILE *fd=fopen(fname,"w");
  if (fd == NULL) {
    printf("Cannot create file %s\n", fname);
    abort();
  }

  fprintf(fd,"#include <stdint.h>\n");
  fprintf(fd,"#include <immintrin.h>\n");


  fprintf(fd,"static inline void nrLDPC_cnProc_BG1_R%s_AVX2(int8_t* cnProcBuf, int8_t* cnProcBufRes, uint16_t Z) {\n",ratestr[R]);

  const uint8_t*  lut_numCnInCnGroups;
  const uint32_t* lut_startAddrCnGroups = lut_startAddrCnGroups_BG1;

  if (R==0)      lut_numCnInCnGroups = lut_numCnInCnGroups_BG1_R13;
  else if (R==1) lut_numCnInCnGroups = lut_numCnInCnGroups_BG1_R23;
  else if (R==2) lut_numCnInCnGroups = lut_numCnInCnGroups_BG1_R89;
  else { printf("aborting, illegal R %d\n",R); fclose(fd);abort();}



  //__m256i* p_cnProcBuf;
  //__m256i* p_cnProcBufRes;

  // Number of CNs in Groups
  //uint32_t M;
  uint32_t j;
  uint32_t k;
  // Offset to each bit within a group in terms of 32 Byte
  uint32_t bitOffsetInGroup;

  //__m256i ymm0, min, sgn;
  //__m256i* p_cnProcBufResBit;

  // const __m256i* p_ones   = (__m256i*) ones256_epi8;
  // const __m256i* p_maxLLR = (__m256i*) maxLLR256_epi8;

  // LUT with offsets for bits that need to be processed
  // 1. bit proc requires LLRs of 2. and 3. bit, 2.bits of 1. and 3. etc.
  // Offsets are in units of bitOffsetInGroup (1*384/32)
  //    const uint8_t lut_idxCnProcG3[3][2] = {{12,24}, {0,24}, {0,12}};

  // =====================================================================
  // Process group with 3 BNs
  fprintf(fd,"//Process group with 3 BNs\n");
  // LUT with offsets for bits that need to be processed
  // 1. bit proc requires LLRs of 2. and 3. bit, 2.bits of 1. and 3. etc.
  // Offsets are in units of bitOffsetInGroup (1*384/32)
  const uint8_t lut_idxCnProcG3[3][2] = {{12,24}, {0,24}, {0,12}};

  fprintf(fd,"                __m256i ymm0, min, sgn,ones,maxLLR;\n");
  fprintf(fd,"                ones   = _mm256_set1_epi8((char)1);\n");
  fprintf(fd,"                maxLLR = _mm256_set1_epi8((char)127);\n");

  fprintf(fd,"                uint32_t  M;\n");

  if (lut_numCnInCnGroups[0] > 0)
    {
      // Number of groups of 32 CNs for parallel processing
      // Ceil for values not divisible by 32
       fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numCnInCnGroups[0] );

      // Set the offset to each bit within a group in terms of 32 Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[0]*NR_LDPC_ZMAX)>>5;


      // Set pointers to start of group 3
      //p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[0]];
      //p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[0]];

      // Loop over every BN
      
      for (j=0; j<3; j++)
        {
	  // Set of results pointer to correct BN address
	  //p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

	  // Loop over CNs
	  //	  for (i=0; i<M; i++,iprime++)
	  //            {
	  
	  fprintf(fd,"            for (int i=0;i<M;i++) {\n");
	  // Abs and sign of 32 CNs (first BN)
	  //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
	  fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[0]>>5)+lut_idxCnProcG3[j][0]);
	  //                sgn  = _mm256_sign_epi8(ones, ymm0);
	  fprintf(fd,"                sgn  = _mm256_sign_epi8(ones, ymm0);\n");
	  //                min  = _mm256_abs_epi8(ymm0);
	  fprintf(fd,"                min  = _mm256_abs_epi8(ymm0);\n");
	  
	  // 32 CNs of second BN
	  //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][1] + i];
	  fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[0]>>5)+lut_idxCnProcG3[j][1]);
	  
	  //                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
	  fprintf(fd,"                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));\n");
	  
	  //                sgn  = _mm256_sign_epi8(sgn, ymm0);
	  fprintf(fd,"                sgn  = _mm256_sign_epi8(sgn, ymm0);\n");
	  
	  // Store result
	  //                min = _mm256_min_epu8(min, maxLLR); // 128 in epi8 is -127
	  fprintf(fd,"                min = _mm256_min_epu8(min, maxLLR);\n");
	  //                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
	  //                p_cnProcBufResBit++;
	  fprintf(fd,"                ((__m256i*)cnProcBufRes)[%d+i] = _mm256_sign_epi8(min, sgn);\n",(lut_startAddrCnGroups[0]>>5)+(j*bitOffsetInGroup));
	  fprintf(fd,"            }\n");
        }
    }

  // =====================================================================
  // Process group with 4 BNs
  fprintf(fd,"//Process group with 4 BNs\n");
  // Offset is 5*384/32 = 60
  const uint8_t lut_idxCnProcG4[4][3] = {{60,120,180}, {0,120,180}, {0,60,180}, {0,60,120}};

  if (lut_numCnInCnGroups[1] > 0)
    {
      // Number of groups of 32 CNs for parallel processing
      // Ceil for values not divisible by 32
      fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numCnInCnGroups[1] );

      // Set the offset to each bit within a group in terms of 32 Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[1]*NR_LDPC_ZMAX)>>5;


      // Set pointers to start of group 4
      //p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
      //p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

      // Loop over every BN
      
      for (j=0; j<4; j++)
        {
	  // Set of results pointer to correct BN address
	  //p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

	  // Loop over CNs
	  //	  for (i=0; i<M; i++,iprime++)
	  //            {
	  fprintf(fd,"            for (int i=0;i<M;i++) {\n");
	  // Abs and sign of 32 CNs (first BN)
	  //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
	  fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[1]>>5)+lut_idxCnProcG4[j][0]);
	  //                sgn  = _mm256_sign_epi8(ones, ymm0);
	  fprintf(fd,"                sgn  = _mm256_sign_epi8(ones, ymm0);\n");
	  //                min  = _mm256_abs_epi8(ymm0);
	  fprintf(fd,"                min  = _mm256_abs_epi8(ymm0);\n");
	  
	  
	  // Loop over BNs
	  for (k=1; k<3; k++)
	    {
	      fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[1]>>5)+lut_idxCnProcG4[j][k]);
	      
	      //                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
	      fprintf(fd,"                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));\n");
	      
	      //                sgn  = _mm256_sign_epi8(sgn, ymm0);
	      fprintf(fd,"                sgn  = _mm256_sign_epi8(sgn, ymm0);\n");
	    }
	  
	  // Store result
	  //                min = _mm256_min_epu8(min, maxLLR); // 128 in epi8 is -127
	  fprintf(fd,"                min = _mm256_min_epu8(min, maxLLR);\n");
	  //                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
	      //                p_cnProcBufResBit++;
	  fprintf(fd,"                ((__m256i*)cnProcBufRes)[%d+i] = _mm256_sign_epi8(min, sgn);\n",(lut_startAddrCnGroups[1]>>5)+(j*bitOffsetInGroup));
	  fprintf(fd,"            }\n");
        }
    }


  // =====================================================================
  // Process group with 5 BNs
  fprintf(fd,"//Process group with 5 BNs\n");
  // Offset is 18*384/32 = 216
  const uint16_t lut_idxCnProcG5[5][4] = {{216,432,648,864}, {0,432,648,864},
					  {0,216,648,864}, {0,216,432,864}, {0,216,432,648}};


  if (lut_numCnInCnGroups[2] > 0)
    {
      // Number of groups of 32 CNs for parallel processing
      // Ceil for values not divisible by 32
      fprintf(fd," M = (%d*Z + 31)>>5;\n",lut_numCnInCnGroups[2] );

      // Set the offset to each bit within a group in terms of 32 Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[2]*NR_LDPC_ZMAX)>>5;


      // Set pointers to start of group 4
      //p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
      //p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

      // Loop over every BN
      
      for (j=0; j<5; j++)
	{
	  // Set of results pointer to correct BN address
	  //p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

	  // Loop over CNs
	  //	  for (i=0; i<M; i++,iprime++)
	  //            {
	  fprintf(fd,"            for (int i=0;i<M;i++) {\n");
	  // Abs and sign of 32 CNs (first BN)
	  //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
	  fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[2]>>5)+lut_idxCnProcG5[j][0]);
	  //                sgn  = _mm256_sign_epi8(ones, ymm0);
	  fprintf(fd,"                sgn  = _mm256_sign_epi8(ones, ymm0);\n");
	  //                min  = _mm256_abs_epi8(ymm0);
	  fprintf(fd,"                min  = _mm256_abs_epi8(ymm0);\n");
	  
	  
	  // Loop over BNs
	  for (k=1; k<4; k++)
	    {
	      fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[2]>>5)+lut_idxCnProcG5[j][k]);
	      
	      //                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
	      fprintf(fd,"                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));\n");
	      
	      //                sgn  = _mm256_sign_epi8(sgn, ymm0);
	      fprintf(fd,"                sgn  = _mm256_sign_epi8(sgn, ymm0);\n");
	    }
	  
	  // Store result
	  //                min = _mm256_min_epu8(min, maxLLR); // 128 in epi8 is -127
	  fprintf(fd,"                min = _mm256_min_epu8(min, maxLLR);\n");
	  //                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
	  //                p_cnProcBufResBit++;
	  fprintf(fd,"                ((__m256i*)cnProcBufRes)[%d+i] = _mm256_sign_epi8(min, sgn);\n",(lut_startAddrCnGroups[2]>>5)+(j*bitOffsetInGroup));
	  fprintf(fd,"           }\n");
        }
    }

  // =====================================================================
  // Process group with 6 BNs
  fprintf(fd,"//Process group with 6 BNs\n");
  // Offset is 8*384/32 = 96
  const uint16_t lut_idxCnProcG6[6][5] = {{96,192,288,384,480}, {0,192,288,384,480},
					  {0,96,288,384,480}, {0,96,192,384,480},
					  {0,96,192,288,480}, {0,96,192,288,384}};


  if (lut_numCnInCnGroups[3] > 0)
    {
      // Number of groups of 32 CNs for parallel processing
      // Ceil for values not divisible by 32
     fprintf(fd, "M = (%d*Z + 31)>>5;\n",lut_numCnInCnGroups[3] );

      // Set the offset to each bit within a group in terms of 32 Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[3]*NR_LDPC_ZMAX)>>5;


      // Set pointers to start of group 4
      //p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
      //p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

      // Loop over every BN
      
      for (j=0; j<6; j++)
        {
	  // Set of results pointer to correct BN address
	  //p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

	  // Loop over CNs
	  //	  for (i=0; i<M; i++,iprime++)
	  //            {
	  fprintf(fd,"            for (int i=0;i<M;i++) {\n");
	  // Abs and sign of 32 CNs (first BN)
	  //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
	  fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[3]>>5)+lut_idxCnProcG6[j][0]);
	  //                sgn  = _mm256_sign_epi8(ones, ymm0);
	  fprintf(fd,"                sgn  = _mm256_sign_epi8(ones, ymm0);\n");
	  //                min  = _mm256_abs_epi8(ymm0);
	  fprintf(fd,"                min  = _mm256_abs_epi8(ymm0);\n");
	  
	  
	  // Loop over BNs
	  for (k=1; k<5; k++)
	    {
	      fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[3]>>5)+lut_idxCnProcG6[j][k]);
	      
	      //                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
	      fprintf(fd,"                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));\n");
	      
	      //                sgn  = _mm256_sign_epi8(sgn, ymm0);
	      fprintf(fd,"                sgn  = _mm256_sign_epi8(sgn, ymm0);\n");
	    }
	  
	  // Store result
	  //                min = _mm256_min_epu8(min, maxLLR); // 128 in epi8 is -127
	  fprintf(fd,"                min = _mm256_min_epu8(min, maxLLR);\n");
	  //                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
	  //                p_cnProcBufResBit++;
	  fprintf(fd,"                ((__m256i*)cnProcBufRes)[%d+i] = _mm256_sign_epi8(min, sgn);\n",(lut_startAddrCnGroups[3]>>5)+(j*bitOffsetInGroup));
	  fprintf(fd,"            }\n");
	}
    }


  // =====================================================================
  // Process group with 7 BNs
  fprintf(fd,"//Process group with 7 BNs\n");
  // Offset is 5*384/32 = 60
  const uint16_t lut_idxCnProcG7[7][6] = {{60,120,180,240,300,360}, {0,120,180,240,300,360},
					  {0,60,180,240,300,360},   {0,60,120,240,300,360},
					  {0,60,120,180,300,360},   {0,60,120,180,240,360},
					  {0,60,120,180,240,300}};



  if (lut_numCnInCnGroups[4] > 0)
    {
      // Number of groups of 32 CNs for parallel processing
      // Ceil for values not divisible by 32
      fprintf(fd, "M = (%d*Z + 31)>>5;\n",lut_numCnInCnGroups[4] );

      // Set the offset to each bit within a group in terms of 32 Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[4]*NR_LDPC_ZMAX)>>5;


      // Set pointers to start of group 4
      //p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
      //p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

      // Loop over every BN
      
      for (j=0; j<7; j++)
        {
	  // Set of results pointer to correct BN address
	  //p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

	  // Loop over CNs
	  //	  for (i=0; i<M; i++,iprime++)
	  //            {
	  fprintf(fd,"            for (int i=0;i<M;i++) {\n");
	  // Abs and sign of 32 CNs (first BN)
	  //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
	  fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[4]>>5)+lut_idxCnProcG7[j][0]);
	  //                sgn  = _mm256_sign_epi8(ones, ymm0);
	  fprintf(fd,"                sgn  = _mm256_sign_epi8(ones, ymm0);\n");
	  //                min  = _mm256_abs_epi8(ymm0);
	  fprintf(fd,"                min  = _mm256_abs_epi8(ymm0);\n");
	  
	  
	  // Loop over BNs
	  for (k=1; k<6; k++)
	    {
	      fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[4]>>5)+lut_idxCnProcG7[j][k]);
	      
	      //                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
	      fprintf(fd,"                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));\n");
	      
	      //                sgn  = _mm256_sign_epi8(sgn, ymm0);
	      fprintf(fd,"                sgn  = _mm256_sign_epi8(sgn, ymm0);\n");
	    }
	  
	  // Store result
	  //                min = _mm256_min_epu8(min, maxLLR); // 128 in epi8 is -127
	  fprintf(fd,"                min = _mm256_min_epu8(min, maxLLR);\n");
	  //                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
	  //                p_cnProcBufResBit++;
	  fprintf(fd,"                ((__m256i*)cnProcBufRes)[%d+i] = _mm256_sign_epi8(min, sgn);\n",(lut_startAddrCnGroups[4]>>5)+(j*bitOffsetInGroup));
	  fprintf(fd,"            }\n");
	}
    }


  // =====================================================================
  // Process group with 8 BNs
  fprintf(fd,"//Process group with 8 BNs\n");
  // Offset is 2*384/32 = 24
  const uint8_t lut_idxCnProcG8[8][7] = {{24,48,72,96,120,144,168}, {0,48,72,96,120,144,168},
					 {0,24,72,96,120,144,168}, {0,24,48,96,120,144,168},
					 {0,24,48,72,120,144,168}, {0,24,48,72,96,144,168},
					 {0,24,48,72,96,120,168},  {0,24,48,72,96,120,144}};




  if (lut_numCnInCnGroups[5] > 0)
    {
      // Number of groups of 32 CNs for parallel processing
      // Ceil for values not divisible by 32
     fprintf(fd, "M = (%d*Z + 31)>>5;\n",lut_numCnInCnGroups[5] );

      // Set the offset to each bit within a group in terms of 32 Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[5]*NR_LDPC_ZMAX)>>5;


      // Set pointers to start of group 4
      //p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
      //p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

      // Loop over every BN
      
      for (j=0; j<8; j++)
        {
	  // Set of results pointer to correct BN address
	  //p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

	  // Loop over CNs
	  //	  for (i=0; i<M; i++,iprime++)
	  //            {
	  fprintf(fd,"            for (int i=0;i<M;i++) {\n");
	  // Abs and sign of 32 CNs (first BN)
	  //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
	  fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[5]>>5)+lut_idxCnProcG8[j][0]);
	  //                sgn  = _mm256_sign_epi8(ones, ymm0);
	  fprintf(fd,"                sgn  = _mm256_sign_epi8(ones, ymm0);\n");
	  //                min  = _mm256_abs_epi8(ymm0);
	  fprintf(fd,"                min  = _mm256_abs_epi8(ymm0);\n");
	  
	  
	  // Loop over BNs
	  for (k=1; k<7; k++)
	    {
	      fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[5]>>5)+lut_idxCnProcG8[j][k]);
	      
	      //                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
	      fprintf(fd,"                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));\n");
	      
	      //                sgn  = _mm256_sign_epi8(sgn, ymm0);
	      fprintf(fd,"                sgn  = _mm256_sign_epi8(sgn, ymm0);\n");
	    }
	  
	  // Store result
	  //                min = _mm256_min_epu8(min, maxLLR); // 128 in epi8 is -127
	  fprintf(fd,"                min = _mm256_min_epu8(min, maxLLR);\n");
	  //                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
	  //                p_cnProcBufResBit++;
	  fprintf(fd,"                ((__m256i*)cnProcBufRes)[%d+i] = _mm256_sign_epi8(min, sgn);\n",(lut_startAddrCnGroups[5]>>5)+(j*bitOffsetInGroup));
	  fprintf(fd,"              }\n");
        }
    }

  // =====================================================================
  // Process group with 9 BNs
  fprintf(fd,"//Process group with 9 BNs\n");
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
      fprintf(fd, "M = (%d*Z + 31)>>5;\n",lut_numCnInCnGroups[6] );
      // Set the offset to each bit within a group in terms of 32 Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[6]*NR_LDPC_ZMAX)>>5;


      // Set pointers to start of group 9
      //p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
      //p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

      // Loop over every BN
     
      for (j=0; j<9; j++)
        {
	  // Set of results pointer to correct BN address
	  //p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

	  // Loop over CNs
	  //	  for (i=0; i<M; i++,iprime++)
	  //            {
	  fprintf(fd,"            for (int i=0;i<M;i++) {\n");
	  // Abs and sign of 32 CNs (first BN)
	  //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
	  fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[6]>>5)+lut_idxCnProcG9[j][0]);
	  //                sgn  = _mm256_sign_epi8(ones, ymm0);
	  fprintf(fd,"                sgn  = _mm256_sign_epi8(ones, ymm0);\n");
	  //                min  = _mm256_abs_epi8(ymm0);
	  fprintf(fd,"                min  = _mm256_abs_epi8(ymm0);\n");
	  
	  
	  // Loop over BNs
	  for (k=1; k<8; k++)
	    {
	      fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[6]>>5)+lut_idxCnProcG9[j][k]);
	      
	      //                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
	      fprintf(fd,"                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));\n");
	      
	      //                sgn  = _mm256_sign_epi8(sgn, ymm0);
	      fprintf(fd,"                sgn  = _mm256_sign_epi8(sgn, ymm0);\n");
	    }
	  
	  // Store result
	  //                min = _mm256_min_epu8(min, maxLLR); // 128 in epi8 is -127
	  fprintf(fd,"                min = _mm256_min_epu8(min, maxLLR);\n");
	  //                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
	  //                p_cnProcBufResBit++;
	  fprintf(fd,"                ((__m256i*)cnProcBufRes)[%d+i] = _mm256_sign_epi8(min, sgn);\n",(lut_startAddrCnGroups[6]>>5)+(j*bitOffsetInGroup));
	  fprintf(fd,"            }\n");
	}
    }

  // =====================================================================
  // Process group with 10 BNs
  fprintf(fd,"//Process group with 10 BNs\n");
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
      fprintf(fd, " M = (%d*Z + 31)>>5;\n",lut_numCnInCnGroups[7] );

      // Set the offset to each bit within a group in terms of 32 Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[7]*NR_LDPC_ZMAX)>>5;


      // Set pointers to start of group 10
      //p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
      //p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

      // Loop over every BN
      
      for (j=0; j<10; j++)
        {
	  // Set of results pointer to correct BN address
	  //p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

	  // Loop over CNs
	  //	  for (i=0; i<M; i++,iprime++)
	  //            {
	  fprintf(fd,"            for (int i=0;i<M;i++) {\n");
	  // Abs and sign of 32 CNs (first BN)
	  //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
	  fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[7]>>5)+lut_idxCnProcG10[j][0]);
	  //                sgn  = _mm256_sign_epi8(ones, ymm0);
	  fprintf(fd,"                sgn  = _mm256_sign_epi8(ones, ymm0);\n");
	  //                min  = _mm256_abs_epi8(ymm0);
	  fprintf(fd,"                min  = _mm256_abs_epi8(ymm0);\n");
	  
	  
	  // Loop over BNs
	  for (k=1; k<9; k++)
	    {
	      fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[7]>>5)+lut_idxCnProcG10[j][k]);
	      
	      //                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
	      fprintf(fd,"                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));\n");
	      
	      //                sgn  = _mm256_sign_epi8(sgn, ymm0);
	      fprintf(fd,"                sgn  = _mm256_sign_epi8(sgn, ymm0);\n");
	    }
	  
	  // Store result
	  //                min = _mm256_min_epu8(min, maxLLR); // 128 in epi8 is -127
	  fprintf(fd,"                min = _mm256_min_epu8(min, maxLLR);\n");
	  //                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
	  //                p_cnProcBufResBit++;
	  fprintf(fd,"                ((__m256i*)cnProcBufRes)[%d+i] = _mm256_sign_epi8(min, sgn);\n",(lut_startAddrCnGroups[7]>>5)+(j*bitOffsetInGroup));
	  fprintf(fd,"            }\n");
        }
    }


  // =====================================================================
  // Process group with 19 BNs
  fprintf(fd,"//Process group with 19 BNs\n");
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
     fprintf(fd, " M = (%d*Z + 31)>>5;\n",lut_numCnInCnGroups[8] );

      // Set the offset to each bit within a group in terms of 32 Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG1_R13[8]*NR_LDPC_ZMAX)>>5;


      // Set pointers to start of group 19
      //p_cnProcBuf    = (__m256i*) &cnProcBuf   [lut_startAddrCnGroups[1]];
      //p_cnProcBufRes = (__m256i*) &cnProcBufRes[lut_startAddrCnGroups[1]];

      // Loop over every BN
      
      for (j=0; j<19; j++)
        {
	  // Set of results pointer to correct BN address
	  //p_cnProcBufResBit = p_cnProcBufRes + (j*bitOffsetInGroup);

	  // Loop over CNs
	  //	  for (i=0; i<M; i++,iprime++)
	  //            {
	  fprintf(fd,"            for (int i=0;i<M;i++) {\n");
	  // Abs and sign of 32 CNs (first BN)
	  //                ymm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
	  fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[8]>>5)+lut_idxCnProcG19[j][0]);
	  //                sgn  = _mm256_sign_epi8(ones, ymm0);
	  fprintf(fd,"                sgn  = _mm256_sign_epi8(ones, ymm0);\n");
	  //                min  = _mm256_abs_epi8(ymm0);
	  fprintf(fd,"                min  = _mm256_abs_epi8(ymm0);\n");
	  
	  
	  // Loop over BNs
	  for (k=1; k<18; k++)
	    {
	      fprintf(fd,"                ymm0 = ((__m256i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[8]>>5)+lut_idxCnProcG19[j][k]);
	      
	      //                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));
	      fprintf(fd,"                min  = _mm256_min_epu8(min, _mm256_abs_epi8(ymm0));\n");
	      
	      //                sgn  = _mm256_sign_epi8(sgn, ymm0);
	      fprintf(fd,"                sgn  = _mm256_sign_epi8(sgn, ymm0);\n");
	    }
	  
	  // Store result
	  //                min = _mm256_min_epu8(min, maxLLR); // 128 in epi8 is -127
	  fprintf(fd,"                min = _mm256_min_epu8(min, maxLLR);\n");
	  //                *p_cnProcBufResBit = _mm256_sign_epi8(min, sgn);
	  //                p_cnProcBufResBit++;
	  fprintf(fd,"                ((__m256i*)cnProcBufRes)[%d+i] = _mm256_sign_epi8(min, sgn);\n",(lut_startAddrCnGroups[8]>>5)+(j*bitOffsetInGroup));
	  fprintf(fd,"            }\n");
        }
    }

  fprintf(fd,"}\n");
  fclose(fd);
}//end of the function  nrLDPC_cnProc_BG1

