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
#include <stdlib.h>
#include <stdint.h>
#include "PHY/sse_intrin.h"
#include "../../nrLDPCdecoder_defs.h"
#include "../../nrLDPC_types.h"


void nrLDPC_bnProc_BG2_generator_AVX512(const char *dir, int R)
{
  const char *ratestr[3]={"15","13","23"};

  if (R<0 || R>2) {printf("Illegal R %d\n",R); abort();}


 // system("mkdir -p ../ldpc_gen_files");

  char fname[FILENAME_MAX+1];
  snprintf(fname, sizeof(fname), "%s/bnProc_avx512/nrLDPC_bnProc_BG2_R%s_AVX512.h", dir, ratestr[R]);
  FILE *fd=fopen(fname,"w");
  if (fd == NULL) {
    printf("Cannot create file %s\n", fname);
    abort();
  }

  fprintf(fd,"#include <stdint.h>\n");
  fprintf(fd,"#include \"PHY/sse_intrin.h\"\n");

    fprintf(fd,"void nrLDPC_bnProc_BG2_R%s_AVX512(int8_t* bnProcBuf,int8_t* bnProcBufRes,  int8_t* llrRes, uint16_t Z  ) {\n",ratestr[R]);
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
    fprintf(fd,"        uint32_t M, i; \n");



// =====================================================================
    // Process group with 1 CN
    // Already done in bnProcBufPc

    // =====================================================================

fprintf(fd,  "// Process group with 2 CNs \n");

    if (lut_numBnInBnGroups[1] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs or parallel processing
        fprintf(fd," M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[1] );


        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[1]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<2; k++)
        {
    
          // Loop over BNs
        fprintf(fd,"            for (i=0;i<M;i++) {\n");
        fprintf(fd,
                "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
                "((simde__m512i*) bnProcBuf)[%d + i]);\n",
                (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
                (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
                (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

        fprintf(fd, "}\n");
        }



    }

    // =====================================================================


fprintf(fd,  "// Process group with 3 CNs \n");



    if (lut_numBnInBnGroups[2] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
         fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[2] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[2]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
        // fprintf(fd,"    ((simde__m512i*) bnProcBuf)     = ((simde__m512i*) &bnProcBuf)
        // [%d];\n",lut_startAddrBnGroups[idxBnGroup]);

        for (k=0; k<3; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }



    // =====================================================================


fprintf(fd,  "// Process group with 4 CNs \n");



    if (lut_numBnInBnGroups[3] > 0)
    {
        // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[3] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[3]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
      
    

        for (k=0; k<4; k++)
        {
  
          // Loop over BNs
        fprintf(fd,"            for (i=0;i<M;i++) {\n");
        fprintf(fd,
                "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
                "((simde__m512i*) bnProcBuf)[%d + i]);\n",
                ((lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup),
                (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
                ((lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup));

        fprintf(fd, "}\n");
        }
    }


   // =====================================================================


    fprintf(fd,  "// Process group with 5 CNs \n");



    if (lut_numBnInBnGroups[4] > 0)
    {
    // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[4] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[4]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<5; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[5] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[5]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<6; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[6] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[6]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<7; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[7] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[7]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<8; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[8] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[8]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<9; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[9] );

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[9]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<10; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }



    // =====================================================================

fprintf(fd,  "// Process group with 11 CNs \n");

    if (lut_numBnInBnGroups[10] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[10] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[10]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<11; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }
      // =====================================================================



fprintf(fd,  "// Process group with 12 CNs \n");


    if (lut_numBnInBnGroups[11] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[11] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[11]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<12; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }

    // =====================================================================



fprintf(fd,  "// Process group with 13 CNs \n");



    if (lut_numBnInBnGroups[12] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[12] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[12]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<13; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }



    // =====================================================================


fprintf(fd,  "// Process group with 14 CNs \n");



    if (lut_numBnInBnGroups[13] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[13] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[13]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<14; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }


   // =====================================================================


fprintf(fd,  "// Process group with 15 CNs \n");



    if (lut_numBnInBnGroups[14] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[14] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[14]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<15; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }



   // =====================================================================


fprintf(fd,  "// Process group with 16 CNs \n");



    if (lut_numBnInBnGroups[15] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[15] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[15]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<16; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[16] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[16]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<17; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[17] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[17]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<18; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }

   // =====================================================================


fprintf(fd,  "// Process group with 19 CNs \n");



    if (lut_numBnInBnGroups[18] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[18] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[18]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<19; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }


   // =====================================================================


fprintf(fd,  "// Process group with 20 CNs \n");



    if (lut_numBnInBnGroups[19] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[19] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[19]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<20; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }





    // =====================================================================

fprintf(fd,  "// Process group with 21 CNs \n");





    if (lut_numBnInBnGroups[20] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[20] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[20]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<21; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }
      // =====================================================================



fprintf(fd,  "// Process group with 22 CNs \n");


    if (lut_numBnInBnGroups[21] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[21] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[21]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<22; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }

    // =====================================================================



fprintf(fd,  "// Process group with <23 CNs \n");



    if (lut_numBnInBnGroups[22] > 0)
    {
  // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[22] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[22]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<23; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[23] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[23]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<24; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }


   // =====================================================================


fprintf(fd,  "// Process group with 25 CNs \n");



    if (lut_numBnInBnGroups[24] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[24] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[24]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<25; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }



   // =====================================================================


fprintf(fd,  "// Process group with 26 CNs \n");



    if (lut_numBnInBnGroups[25] > 0)
    {
 // If elements in group move to next address
        idxBnGroup++;

        // Number of groups of 32 BNs for parallel processing
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[25] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[25]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<26; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[26] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[26]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<27; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[27] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[27]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<28; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[28] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[28]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<29; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
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
        fprintf(fd,"       M = (%d*Z + 63)>>6;\n",lut_numBnInBnGroups[29] );;

        // Set the offset to each CN within a group in terms of 16 Byte
        cnOffsetInGroup = (lut_numBnInBnGroups[29]*NR_LDPC_ZMAX)>>6;

        // Set pointers to start of group 2
  
            // Loop over CNs
        for (k=0; k<30; k++)
        {
  

          // Loop over BNs
       fprintf(fd,"            for (i=0;i<M;i++) {\n");
       fprintf(fd,
               "            ((simde__m512i*)bnProcBufRes)[%d + i ] = simde_mm512_subs_epi8(((simde__m512i*)llrRes)[%d + i ], "
               "((simde__m512i*) bnProcBuf)[%d + i]);\n",
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup,
               (lut_startAddrBnGroupsLlr[idxBnGroup] >> 6),
               (lut_startAddrBnGroups[idxBnGroup] >> 6) + k * cnOffsetInGroup);

       fprintf(fd, "}\n");
        }
    }

    fprintf(fd,"}\n");
  fclose(fd);
}//end of the function  nrLDPC_bnProc_BG1





