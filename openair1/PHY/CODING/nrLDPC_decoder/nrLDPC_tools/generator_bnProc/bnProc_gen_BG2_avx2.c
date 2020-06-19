#include <stdint.h>
#include <immintrin.h>
#include "../../nrLDPCdecoder_defs.h"
#include "../../nrLDPC_types.h"
#include "../../nrLDPC_bnProc.h"
#include "../../nrLDPC_cnProc.h"
#include "../../nrLDPC_init.h"

void nrLDPC_bnProc_BG2_generator_AVX2(uint16_t Z,int R)
{
  const char *ratestr[3]={"15","13","23"};

  if (R<0 || R>2) {printf("Illegal R %d\n",R); abort();}


 // system("mkdir -p ../ldpc_gen_files");

  char fname[50];
  sprintf(fname,"../ldpc_gen_files/bnProc/nrLDPC_bnProc_BG2_Z%d_R%s_AVX2.c",Z,ratestr[R]);
  FILE *fd=fopen(fname,"w");
  if (fd == NULL) {printf("Cannot create \n");abort();}

  fprintf(fd,"#include <stdint.h>\n");
  fprintf(fd,"#include <immintrin.h>\n");
    
    fprintf(fd,"void nrLDPC_bnProc_BG2_Z%d_R%s_AVX2(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes  ) {\n",Z, ratestr[R]);
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


    uint32_t M;
    //uint32_t M32rem;
   // uint32_t i;
    uint32_t k;
    // Offset to each bit within a group in terms of 32 Byte
    uint32_t cnOffsetInGroup;
    uint8_t idxBnGroup = 0;


    
    fprintf(fd,"        __m256i* p_bnProcBuf; \n");
    fprintf(fd,"        __m256i* p_bnProcBufRes; \n");
    fprintf(fd,"        __m256i* p_llrRes; \n");
    fprintf(fd,"        __m256i* p_res; \n");



// =====================================================================
    // Process group with 1 CN
    // Already done in bnProcBufPc

    // =====================================================================

fprintf(fd,  "// Process group with 2 CNs \n");

    if (lut_numBnInBnGroups[1] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[1]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[1]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<2; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"             p_res++;\n");
        fprintf(fd,"             p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }

        
       
    }

    // =====================================================================


fprintf(fd,  "// Process group with 3 CNs \n");



    if (lut_numBnInBnGroups[2] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[2]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[2]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
        for (k=0; k<3; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        }
    }



    // =====================================================================
   

fprintf(fd,  "// Process group with 4 CNs \n");

 

    if (lut_numBnInBnGroups[3] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[3]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[3]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
        for (k=0; k<4; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        }
    }


   // =====================================================================

    
    fprintf(fd,  "// Process group with 5 CNs \n");



    if (lut_numBnInBnGroups[4] > 0)
    {
    // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[4]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[4]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<5; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }



   // =====================================================================
 
    
fprintf(fd,  "// Process group with 6 CNs \n");

 // Process group with 6 CNs

    if (lut_numBnInBnGroups[5] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[5]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[5]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<6; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }


   // =====================================================================

    
fprintf(fd,  "// Process group with 7 CNs \n");

 // Process group with 7 CNs

    if (lut_numBnInBnGroups[6] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[6]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[6]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<7; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }


   // =====================================================================
 
    
fprintf(fd,  "// Process group with 8 CNs \n");

 // Process group with 8 CNs

    if (lut_numBnInBnGroups[7] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[7]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[7]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<8; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }

   // =====================================================================

    
fprintf(fd,  "// Process group with 9 CNs \n");

 // Process group with 9 CNs

    if (lut_numBnInBnGroups[8] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[8]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[8]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<9; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }


   // =====================================================================

fprintf(fd,  "// Process group with 10 CNs \n");

 // Process group with 10 CNs

    if (lut_numBnInBnGroups[9] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[9]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[9]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<10; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }



    // =====================================================================
   
fprintf(fd,  "// Process group with 11 CNs \n");

    if (lut_numBnInBnGroups[10] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[10]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[10]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<11; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }
      // =====================================================================



fprintf(fd,  "// Process group with 12 CNs \n");


    if (lut_numBnInBnGroups[11] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[11]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[11]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<12; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }

    // =====================================================================



fprintf(fd,  "// Process group with 13 CNs \n");



    if (lut_numBnInBnGroups[12] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[12]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[12]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<1; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }



    // =====================================================================
   

fprintf(fd,  "// Process group with 14 CNs \n");



    if (lut_numBnInBnGroups[13] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[13]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[13]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<14; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }


   // =====================================================================

    
fprintf(fd,  "// Process group with 15 CNs \n");



    if (lut_numBnInBnGroups[14] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[14]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[14]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<15; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }



   // =====================================================================

    
fprintf(fd,  "// Process group with 16 CNs \n");

 

    if (lut_numBnInBnGroups[15] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[15]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[15]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<16; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
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
        M = (lut_numBnInBnGroups[16]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[16]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<17; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }


   // =====================================================================
 
    
fprintf(fd,  "// Process group with 18 CNs \n");

 // Process group with 8 CNs

    if (lut_numBnInBnGroups[17] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[17]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[17]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<18; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }

   // =====================================================================
 
    
fprintf(fd,  "// Process group with 19 CNs \n");



    if (lut_numBnInBnGroups[18] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[18]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[18]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<19; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }


   // =====================================================================
  
    
fprintf(fd,  "// Process group with 20 CNs \n");



    if (lut_numBnInBnGroups[19] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[19]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[19]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<20; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }





    // =====================================================================
   
fprintf(fd,  "// Process group with 21 CNs \n");



 

    if (lut_numBnInBnGroups[20] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[20]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[20]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<21; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }
      // =====================================================================



fprintf(fd,  "// Process group with 22 CNs \n");


    if (lut_numBnInBnGroups[21] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[21]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[21]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<22; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }

    // =====================================================================
  


fprintf(fd,  "// Process group with 23 CNs \n");



    if (lut_numBnInBnGroups[22] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[22]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[22]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<23; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }



    // =====================================================================
 

fprintf(fd,  "// Process group with 24 CNs \n");

 // Process group with 4 CNs

    if (lut_numBnInBnGroups[23] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[23]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[23]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<24; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }


   // =====================================================================
  
    
fprintf(fd,  "// Process group with 25 CNs \n");



    if (lut_numBnInBnGroups[24] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[24]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[24]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<25; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }



   // =====================================================================
   
    
fprintf(fd,  "// Process group with 26 CNs \n");

 

    if (lut_numBnInBnGroups[25] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[25]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[25]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<26; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }


   // =====================================================================

    
fprintf(fd,  "// Process group with 27 CNs \n");

 // Process group with 17 CNs

    if (lut_numBnInBnGroups[26] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[26]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[26]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<27; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }


   // =====================================================================
    
    
fprintf(fd,  "// Process group with 28 CNs \n");

 // Process group with 8 CNs

    if (lut_numBnInBnGroups[27] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[27]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[27]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<28; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }

   // =====================================================================
  
fprintf(fd,  "// Process group with 29 CNs \n");

 // Process group with 9 CNs

    if (lut_numBnInBnGroups[28] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[28]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[28]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<29; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }


   // =====================================================================

fprintf(fd,  "// Process group with 30 CNs \n");

 // Process group with 20 CNs

    if (lut_numBnInBnGroups[29] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        M = (lut_numBnInBnGroups[29]*Z + 31)>>5;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[29]*NR_LDPC_ZMAX)>>5;

        // Set pointers to start of group 2
        fprintf(fd,"    p_bnProcBuf     = (__m256i*) &bnProcBuf    [%d];\n",lut_startAddrBnGroups[idxBnGroup]);
        fprintf(fd,"   p_bnProcBufRes    = (__m256i*) &bnProcBufRes   [%d];\n",lut_startAddrBnGroups[idxBnGroup]); 
            
            // Loop over CNs
        for (k=0; k<30; k++)
        {
        fprintf(fd,"            p_res = &p_bnProcBufRes[%d];\n", k*cnOffsetInGroup);
        fprintf(fd,"            p_llrRes = (__m256i*) &llrRes  [%d];\n",lut_startAddrBnGroupsLlr[idxBnGroup]);

          // Loop over BNs
        fprintf(fd,"            for (int i=0;i<%d;i++) {\n",M);
        fprintf(fd,"            *p_res = _mm256_subs_epi8(*p_llrRes, p_bnProcBuf[%d + i]);\n",k*cnOffsetInGroup);
        fprintf(fd,"            p_res++;\n");
        fprintf(fd,"            p_llrRes++;\n");

         fprintf(fd,"}\n");
        
        }
    }

    fprintf(fd,"}\n");
  fclose(fd);
}//end of the function  nrLDPC_bnProc_BG1




