


#include <stdint.h>
#include <immintrin.h>
#include "../../nrLDPCdecoder_defs.h"
#include "../../nrLDPC_types.h"


void nrLDPC_bnProcPc_BG2_generator_AVX512(int R)
{
  const char *ratestr[3]={"15","13","23"};

  if (R<0 || R>2) {printf("Illegal R %d\n",R); abort();}


 // system("mkdir -p ../ldpc_gen_files");

  char fname[50];
  sprintf(fname,"../ldpc_gen_files/bn_avx512/nrLDPC_bnProcPc_BG2_R%s_AVX512.h",ratestr[R]);
  FILE *fd=fopen(fname,"w");
  if (fd == NULL) {printf("Cannot create \n");abort();}

//  fprintf(fd,"#include <stdint.h>\n");
  //fprintf(fd,"#include <immintrin.h>\n");

  fprintf(fd,"static inline void nrLDPC_bnProcPc_BG2_R%s_AVX512(int8_t* bnProcBuf,int8_t* llrRes ,  int8_t* llrProcBuf, uint16_t Z ) {\n",ratestr[R]);
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

    fprintf(fd,"   __m512i zmm0,zmm1,zmmRes0,zmmRes1;  \n");


    fprintf(fd,"        __m256i* p_bnProcBuf; \n");
    fprintf(fd,"        __m256i* p_llrProcBuf;\n");
    fprintf(fd,"        __m512i* p_llrRes; \n");
    fprintf(fd,"         uint32_t M ;\n");


fprintf(fd,  "// Process group with 1 CNs \n");


 // Process group with 2 CNs

    if (lut_numBnInBnGroups[0] > 0)
    {
        // If elements in group move to next address
       // idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[0] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[0]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"           zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"           zmm1 = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");

            // Loop over CNs
        /*for (k=1; k<1; k++)
        {
        fprintf(fd,"           zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "          zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }
*/
            // Add LLR from receiver input
        fprintf(fd,"           zmm0    = _mm512_cvtepi8_epi16(p_bnProcBuf[j+1]);\n");
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"           zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");


        fprintf(fd,"}\n");
    }
      // =====================================================================
    // Process group with 2 CNs


fprintf(fd,  "// Process group with 2 CNs \n");

 // Process group with 2 CNs

    if (lut_numBnInBnGroups[1] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[1] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[1]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"           zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf[j + 1]);\n");

            // Loop over CNs
        for (k=1; k<2; k++)
        {
        fprintf(fd,"           zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "          zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"           zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"           zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");


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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[2] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[2]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<3; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
            }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[3] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[3]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<4; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[4] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[4]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<5; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[5] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[5]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<6; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[6] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[6]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<7; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        //fprintf(fd,"         (__m512i*) &llrRes[%d + i]    = _mm512_permutex_epi64(zmm0, 0xD8);\n",lut_startAddrBnGroupsLlr[idxBnGroup]>>5 );
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[7] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[7]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<8; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        //fprintf(fd,"         (__m512i*) &llrRes[%d + i]    = _mm512_permutex_epi64(zmm0, 0xD8);\n",lut_startAddrBnGroupsLlr[idxBnGroup]>>5 );

        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[8] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[8]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<9; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        //fprintf(fd,"         (__m512i*) &llrRes[%d + i]    = _mm512_permutex_epi64(zmm0, 0xD8);\n",lut_startAddrBnGroupsLlr[idxBnGroup]>>5 );
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[9] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[9]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<10; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[10] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[10]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"           zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<11; k++)
        {
        fprintf(fd,"           zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "          zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"           zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"           zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[11] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[11]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"           zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<12; k++)
        {
        fprintf(fd,"           zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "          zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"           zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"           zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[12] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[12]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<13; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
            }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[13] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[13]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<14; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[14] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[14]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<15; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
         fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[15] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[15]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<16; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[16] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[16]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<17; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[17] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[17]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<18; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[18] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[18]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<19; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[19] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[19]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<20; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[20] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[20]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"           zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<21; k++)
        {
        fprintf(fd,"           zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "          zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"           zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"           zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[21] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[21]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"            for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"           zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<22; k++)
        {
        fprintf(fd,"           zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "          zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"           zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"           zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"           zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"           zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"           zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
         fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[22] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[22]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<23; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
            }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[23] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[23]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<24; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[24] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[24]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<25; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[25] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[25]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<26; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[26] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[26]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<27; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[27] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[27]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<28; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[28] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[28]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<29; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

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
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[29] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[29]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"    p_llrProcBuf    = (__m256i*) &llrProcBuf   [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        fprintf(fd,"    p_llrRes        = (__m512i*) &llrRes       [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);
        // Loop over BNs
        fprintf(fd,"        for (int i=0,j=0;i<M;i++,j+=2) {\n");
            // First 16 LLRs of first CN
        fprintf(fd,"       zmmRes0 = _mm512_cvtepi8_epi16(p_bnProcBuf [j]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_cvtepi8_epi16(p_bnProcBuf [j +1]);\n");

            // Loop over CNs
        for (k=1; k<30; k++)
        {
        fprintf(fd,"       zmm0 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j]);\n", k*cnOffsetInGroup);
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1 = _mm512_cvtepi8_epi16(p_bnProcBuf[%d + j +1]);\n", k*cnOffsetInGroup);

        fprintf(fd, "      zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1); \n");
        }

            // Add LLR from receiver input
        fprintf(fd,"       zmm0    = _mm512_cvtepi8_epi16(p_llrProcBuf[j]);\n");
        fprintf(fd,"       zmmRes0 = _mm512_adds_epi16(zmmRes0,zmm0);\n");

        fprintf(fd,"       zmm1    = _mm512_cvtepi8_epi16(p_llrProcBuf[j +1 ]);\n");
        fprintf(fd,"       zmmRes1 = _mm512_adds_epi16(zmmRes1,zmm1);\n");

            // Pack results back to epi8
        fprintf(fd,"       zmm0 = _mm512_packs_epi16(zmmRes0,zmmRes1);\n");
            //zmm0     = [zmmRes1[255:256]zmmRes0[255:256]zmmRes1[127:0]zmmRes0[127:0]]
            // p_llrRes = [zmmRes1[255:256]zmmRes1[127:0]zmmRes0[255:256]zmmRes0[127:0]]
        fprintf(fd,"            p_llrRes[i] = _mm512_permutex_epi64(zmm0, 0xD8);\n");

        fprintf(fd,"}\n");
    }

    fprintf(fd,"}\n");
  fclose(fd);
}//end of the function  nrLDPC_bnProcPc_BG2









