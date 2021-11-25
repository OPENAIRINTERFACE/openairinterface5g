
#include <stdio.h>
#include <stdint.h>
#include "../../nrLDPCdecoder_defs.h"

void nrLDPC_cnProc_BG2_generator_AVX512(int R)
{
  const char *ratestr[3]={"15","13","23"};

  if (R<0 || R>2) {printf("Illegal R %d\n",R); abort();}


 // system("mkdir -p ../ldpc_gen_files");

  char fname[50];
  sprintf(fname,"../ldpc_gen_files/cnProc_avx512/nrLDPC_cnProc_BG2_R%s_AVX512.h",ratestr[R]);
  FILE *fd=fopen(fname,"w");
  if (fd == NULL) {printf("Cannot create \n");abort();}

  //fprintf(fd,"#include <stdint.h>\n");
//  fprintf(fd,"#include <immintrin.h>\n");


  fprintf(fd,   "#define conditional_negate(a,b,z) _mm512_mask_sub_epi8(a,_mm512_movepi8_mask(b),z,a)\n");


  //fprintf(fd,"static inline void nrLDPC_cnProc_BG2_Z%d_R%s_AVX512(int8_t* cnProcBuf,int8_t* cnProcBufRes, uint16_t Z) {\n",Z,ratestr[R]);
 fprintf(fd,"void nrLDPC_cnProc_BG2_R%s_AVX512(t_nrLDPC_lut* p_lut, t_nrLDPC_procBuf* p_procBuf, uint16_t Z) {\n",ratestr[R]);
  const uint8_t*  lut_numCnInCnGroups;
  const uint32_t* lut_startAddrCnGroups = lut_startAddrCnGroups_BG2;

  if (R==0)      lut_numCnInCnGroups = lut_numCnInCnGroups_BG2_R15;
  else if (R==1) lut_numCnInCnGroups = lut_numCnInCnGroups_BG2_R13;
  else if (R==2) lut_numCnInCnGroups = lut_numCnInCnGroups_BG2_R23;
  else { printf("aborting, illegal R %d\n",R); fclose(fd);abort();}


  // Number of CNs in Groups
  //uint32_t M;
  uint32_t j;
  uint32_t k;
  // Offset to each bit within a group in terms of 64  Byte
  uint32_t bitOffsetInGroup;

 fprintf(fd,"                uint32_t M;\n");  
  fprintf(fd,"                int8_t* cnProcBuf    = p_procBuf->cnProcBuf;\n");
  fprintf(fd,"                int8_t* cnProcBufRes = p_procBuf->cnProcBufRes;\n");

  fprintf(fd,"                __m512i zmm0, min, sgn,zeros,maxLLR;\n");
  fprintf(fd,"                zeros  = _mm512_setzero_si512();\n");
  fprintf(fd,"                maxLLR = _mm512_set1_epi8((char)127);\n");
  // =====================================================================
  // Process group with 3 BNs
  fprintf(fd,"//Process group with 3 BNs\n");
  // LUT with offsets for bits that need to be processed
  // 1. bit proc requires LLRs of 2. and 3. bit, 2.bits of 1. and 3. etc.
  // Offsets are in units of bitOffsetInGroup 
   const uint8_t lut_idxCnProcG3[3][2] = {{72,144}, {0,144}, {0,72}};


  if (lut_numCnInCnGroups[0] > 0)
    {
      // Number of groups of 64  CNs for parallel processing
      // Ceil for values not divisible by 64
       fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numCnInCnGroups[1] );

      // Set the offset to each bit within a group in terms of 64  Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[0]*NR_LDPC_ZMAX)>>6;


      // Loop over every BN

      for (j=0; j<3; j++)
        {


          fprintf(fd,"            for (int i=0;i<M;i+=2) {\n");
          // Abs and sign of 64  CNs (first BN)
          //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
          fprintf(fd,"                zmm0 = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[0]>>6)+lut_idxCnProcG3[j][0]/2);
          //                sgn  = _mm512_sign_epi8(ones, zmm0);
          //                min  = _mm512_abs_epi8(zmm0);
          fprintf(fd,"                min  = _mm512_abs_epi8(zmm0);\n");

          // 32 CNs of second BN
          //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][1] + i];
          fprintf(fd,"                zmm0 = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[0]>>6)+lut_idxCnProcG3[j][1]/2);

          //                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
          fprintf(fd,"                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));\n");

          //                sgn  = _mm512_sign_epi8(sgn, zmm0);
          fprintf(fd,"                sgn  = _mm512_xor_si512(sgn, zmm0);\n");

          // Store result
          //                min = _mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
          fprintf(fd,"                min = _mm512_min_epu8(min, maxLLR);\n");

          fprintf(fd,"                ((__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",(lut_startAddrCnGroups[0]>>6)+(j*bitOffsetInGroup));

          // Abs and sign of 64  CNs (first BN)
          //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
          fprintf(fd,"                sgn = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[0]>>6)+(lut_idxCnProcG3[j][0]/2)+1);
          //                sgn  = _mm512_sign_epi8(ones, zmm0);
          //                min  = _mm512_abs_epi8(zmm0);
          fprintf(fd,"                min  = _mm512_abs_epi8(sgn);\n");

          // 32 CNs of second BN
          //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][1] + i];
          fprintf(fd,"                zmm0 = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[0]>>6)+(lut_idxCnProcG3[j][1]/2)+1);

          //                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
          fprintf(fd,"                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));\n");

          //                sgn  = _mm512_sign_epi8(sgn, zmm0);
          fprintf(fd,"                sgn  = _mm512_xor_si512(sgn, zmm0);\n");

          // Store result
          //                min = _mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
          fprintf(fd,"                min = _mm512_min_epu8(min, maxLLR);\n");

          fprintf(fd,"                ((__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",(lut_startAddrCnGroups[0]>>6)+(j*bitOffsetInGroup)+1);

          fprintf(fd,"            }\n");
        }
    }

  // =====================================================================
  // Process group with 4 BNs
  fprintf(fd,"//Process group with 4 BNs\n");
 // Offset is 20*384/32 = 240
    const uint16_t lut_idxCnProcG4[4][3] = {{240,480,720}, {0,480,720}, {0,240,720}, {0,240,480}};


  if (lut_numCnInCnGroups[1] > 0)
    {
      // Number of groups of 64  CNs for parallel processing
      // Ceil for values not divisible by 64
      fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numCnInCnGroups[1] );

      // Set the offset to each bit within a group in terms of 64  Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[1]*NR_LDPC_ZMAX)>>6;

      // Loop over every BN
      for (j=0; j<4; j++)
	{
          fprintf(fd,"            for (int i=0;i<M;i++) {\n");
          // Abs and sign of 64  CNs (first BN)
          //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
          fprintf(fd,"                sgn = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[1]>>6)+lut_idxCnProcG4[j][0]/2);
          //                sgn  = _mm512_sign_epi8(ones, zmm0);
          //                min  = _mm512_abs_epi8(zmm0);
          fprintf(fd,"                min  = _mm512_abs_epi8(sgn);\n");


          // Loop over BNs
          for (k=1; k<3; k++)
            {
              fprintf(fd,"                zmm0 = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[1]>>6)+lut_idxCnProcG4[j][k]/2);

              //                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
              fprintf(fd,"                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));\n");

              //                sgn  = _mm512_sign_epi8(sgn, zmm0);
              fprintf(fd,"                sgn  = _mm512_xor_si512(sgn, zmm0);\n");
            }

          // Store result
          //                min = _mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
          fprintf(fd,"                min = _mm512_min_epu8(min, maxLLR);\n");
          //                *p_cnProcBufResBit = _mm512_sign_epi8(min, sgn);
              //                p_cnProcBufResBit++;
          fprintf(fd,"                ((__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",(lut_startAddrCnGroups[1]>>6)+(j*bitOffsetInGroup));
          fprintf(fd,"            }\n");
        }
    }


  // =====================================================================
  // Process group with 5 BNs
  fprintf(fd,"//Process group with 5 BNs\n");
    // Offset is 9*384/32 = 108
    const uint16_t lut_idxCnProcG5[5][4] = {{108,216,324,432}, {0,216,324,432},
                                            {0,108,324,432}, {0,108,216,432}, {0,108,216,324}};



  if (lut_numCnInCnGroups[2] > 0)
    {
      // Number of groups of 64  CNs for parallel processing
      // Ceil for values not divisible by 64
       fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numCnInCnGroups[2] );

      // Set the offset to each bit within a group in terms of 64  Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[2]*NR_LDPC_ZMAX)>>6;

      // Loop over every BN

      for (j=0; j<5; j++)
        {

          fprintf(fd,"            for (int i=0;i<M;i++) {\n");
          // Abs and sign of 64  CNs (first BN)
          //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
          fprintf(fd,"                sgn = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[2]>>6)+lut_idxCnProcG5[j][0]/2);
          //                sgn  = _mm512_sign_epi8(ones, zmm0);
          //                min  = _mm512_abs_epi8(zmm0);
          fprintf(fd,"                min  = _mm512_abs_epi8(sgn);\n");


          // Loop over BNs
          for (k=1; k<4; k++)
            {
              fprintf(fd,"                zmm0 = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[2]>>6)+lut_idxCnProcG5[j][k]/2);

              //                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
              fprintf(fd,"                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));\n");

              //                sgn  = _mm512_sign_epi8(sgn, zmm0);
              fprintf(fd,"                sgn  = _mm512_xor_si512(sgn, zmm0);\n");
            }

          // Store result
          //                min = _mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
          fprintf(fd,"                min = _mm512_min_epu8(min, maxLLR);\n");

          fprintf(fd,"                ((__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",(lut_startAddrCnGroups[2]>>6)+(j*bitOffsetInGroup));
          fprintf(fd,"           }\n");
        }
    }

  // =====================================================================
  // Process group with 6 BNs
  fprintf(fd,"//Process group with 6 BNs\n");
    // Offset is 3*384/32 = 36
  const uint16_t lut_idxCnProcG6[6][5] = {{36,72,108,144,180}, {0,72,108,144,180},
                                            {0,36,108,144,180}, {0,36,72,144,180},
                                            {0,36,72,108,180}, {0,36,72,108,144}};



  if (lut_numCnInCnGroups[3] > 0)
    {
      // Number of groups of 64  CNs for parallel processing
      // Ceil for values not divisible by 64
       fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numCnInCnGroups[3] );

      // Set the offset to each bit within a group in terms of 64  Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[3]*NR_LDPC_ZMAX)>>6;

      // Loop over every BN

      for (j=0; j<6; j++)
        {

          fprintf(fd,"            for (int i=0;i<M;i++) {\n");
          // Abs and sign of 64  CNs (first BN)
          //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
          fprintf(fd,"                sgn = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[3]>>6)+lut_idxCnProcG6[j][0]/2);
          //                sgn  = _mm512_sign_epi8(ones, zmm0);
          //                min  = _mm512_abs_epi8(zmm0);
          fprintf(fd,"                min  = _mm512_abs_epi8(sgn);\n");


          // Loop over BNs
          for (k=1; k<5; k++)
            {
              fprintf(fd,"                zmm0 = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[3]>>6)+lut_idxCnProcG6[j][k]/2);

              //                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
              fprintf(fd,"                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));\n");

              //                sgn  = _mm512_sign_epi8(sgn, zmm0);
              fprintf(fd,"                sgn  = _mm512_xor_si512(sgn, zmm0);\n");
            }

          // Store result
          //                min = _mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
          fprintf(fd,"                min = _mm512_min_epu8(min, maxLLR);\n");

          fprintf(fd,"                ((__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",(lut_startAddrCnGroups[3]>>6)+(j*bitOffsetInGroup));
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
                                           {0,24,48,72,96,120,168}, {0,24,48,72,96,120,144}};


  if (lut_numCnInCnGroups[4] > 0)
    {
      // Number of groups of 64  CNs for parallel processing
      // Ceil for values not divisible by 64
       fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numCnInCnGroups[4] );

      // Set the offset to each bit within a group in terms of 64  Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[4]*NR_LDPC_ZMAX)>>6;

      // Loop over every BN

      for (j=0; j<8; j++)
        {
 
          fprintf(fd,"            for (int i=0;i<M;i++) {\n");
          // Abs and sign of 64  CNs (first BN)
          //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
          fprintf(fd,"                sgn = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[4]>>6)+lut_idxCnProcG8[j][0]/2);
          //                sgn  = _mm512_sign_epi8(ones, zmm0);
          //                min  = _mm512_abs_epi8(zmm0);
          fprintf(fd,"                min  = _mm512_abs_epi8(sgn);\n");


          // Loop over BNs
          for (k=1; k<7; k++)
            {
              fprintf(fd,"                zmm0 = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[4]>>6)+lut_idxCnProcG8[j][k]/2);

              //                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
              fprintf(fd,"                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));\n");

              //                sgn  = _mm512_sign_epi8(sgn, zmm0);
              fprintf(fd,"                sgn  = _mm512_xor_si512(sgn, zmm0);\n");
            }

          // Store result
          //                min = _mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
          fprintf(fd,"                min = _mm512_min_epu8(min, maxLLR);\n");
      
          fprintf(fd,"                ((__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min, sgn,zeros);\n",(lut_startAddrCnGroups[4]>>6)+(j*bitOffsetInGroup));
          fprintf(fd,"              }\n");
        }
    }


  // =====================================================================
  // Process group with 10 BNs
  fprintf(fd,"//Process group with 10 BNs\n");
     
  const uint8_t lut_idxCnProcG10[10][9] = {{24,48,72,96,120,144,168,192,216}, {0,48,72,96,120,144,168,192,216},
                                             {0,24,72,96,120,144,168,192,216}, {0,24,48,96,120,144,168,192,216},
                                             {0,24,48,72,120,144,168,192,216}, {0,24,48,72,96,144,168,192,216},
                                             {0,24,48,72,96,120,168,192,216}, {0,24,48,72,96,120,144,192,216},
                                             {0,24,48,72,96,120,144,168,216}, {0,24,48,72,96,120,144,168,192}};


  if (lut_numCnInCnGroups[5] > 0)
    {
      // Number of groups of 64  CNs for parallel processing
      // Ceil for values not divisible by 64
       fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numCnInCnGroups[5] );

      // Set the offset to each bit within a group in terms of 64  Byte
      bitOffsetInGroup = (lut_numCnInCnGroups_BG2_R15[5]*NR_LDPC_ZMAX)>>6;

     // Loop over every BN

      for (j=0; j<10; j++)
        {

          fprintf(fd,"            for (int i=0;i<M;i++) {\n");
          // Abs and sign of 64  CNs (first BN)
          //                zmm0 = p_cnProcBuf[lut_idxCnProcG3[j][0] + i];
          fprintf(fd,"                sgn = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[5]>>6)+lut_idxCnProcG10[j][0]/2);
          //                sgn  = _mm512_sign_epi8(ones, zmm0);
          //                min  = _mm512_abs_epi8(zmm0);
          fprintf(fd,"                min  = _mm512_abs_epi8(sgn);\n");


          // Loop over BNs
          for (k=1; k<9; k++)
            {
              fprintf(fd,"                zmm0 = ((__m512i*)cnProcBuf)[%d+i];\n",(lut_startAddrCnGroups[5]>>6)+lut_idxCnProcG10[j][k]/2);

              //                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));
              fprintf(fd,"                min  = _mm512_min_epu8(min, _mm512_abs_epi8(zmm0));\n");

              //                sgn  = _mm512_sign_epi8(sgn, zmm0);
              fprintf(fd,"                sgn  = _mm512_xor_si512(sgn, zmm0);\n");
            }

          // Store result
          //                min = _mm512_min_epu8(min, maxLLR); // 128 in epi8 is -127
          fprintf(fd,"                min = _mm512_min_epu8(min, maxLLR);\n");

          fprintf(fd,"                ((__m512i*)cnProcBufRes)[%d+i] = conditional_negate(min,sgn,zeros);\n",(lut_startAddrCnGroups[5]>>6)+(j*bitOffsetInGroup));
          fprintf(fd,"            }\n");
        }
    }

 
  fprintf(fd,"}\n");
  fclose(fd);
}//end of the function  nrLDPC_cnProc_BG2








