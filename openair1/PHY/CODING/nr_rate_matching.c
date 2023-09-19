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

/* file: nr_rate_matching.c
   purpose: Procedures for rate matching/interleaving for NR LDPC
   author: hongzhi.wang@tcl.com
*/

#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/sse_intrin.h"

//#define RM_DEBUG 1

static const uint8_t index_k0[2][4] = {{0, 17, 33, 56}, {0, 13, 25, 43}};

void nr_interleaving_ldpc(uint32_t E, uint8_t Qm, uint8_t *e,uint8_t *f)
{
  uint32_t EQm;

  EQm = E/Qm;
  memset(f,0,E*sizeof(uint8_t));
  uint8_t *e0,*e1,*e2,*e3,*e4,*e5,*e6,*e7;
  uint8_t *fp;
#if 0 //def __WASAVX2__
  simde__m256i tmp0,tmp1,tmp2,tmp0b,tmp1b,tmp3,tmp4,tmp5;
  simde__m256i *e0_256,*e1_256,*e2_256,*e3_256,*e4_256,*e5_256,*e6_256,*e7_256;

  simde__m256i *f_256=(simde__m256i *)f;

  uint8_t *fp2;
  switch(Qm) {
  case 2:
    e0=e;
    e1=e0+EQm;
    e0_256=(simde__m256i *)e0;
    e1_256=(simde__m256i *)e1;
    for (int k=0,j=0;j<EQm>>5;j++,k+=2) {
      f_256[k]   = simde_mm256_unpacklo_epi8(e0_256[j],e1_256[j]);
      f_256[k+1] = simde_mm256_unpackhi_epi8(e0_256[j],e1_256[j]); 
    }
    break;
  case 4:
    e0=e;
    e1=e0+EQm;
    e2=e1+EQm;
    e3=e2+EQm;
    e0_256=(simde__m256i *)e0;
    e1_256=(simde__m256i *)e1;
    e2_256=(simde__m256i *)e2;
    e3_256=(simde__m256i *)e3;
    for (int k=0,j=0;j<EQm>>5;j++,k+=4) {
      tmp0   = simde_mm256_unpacklo_epi8(e0_256[j],e1_256[j]); // e0(i) e1(i) e0(i+1) e1(i+1) .... e0(i+15) e1(i+15)
      tmp1   = simde_mm256_unpacklo_epi8(e2_256[j],e3_256[j]); // e2(i) e3(i) e2(i+1) e3(i+1) .... e2(i+15) e3(i+15)
      f_256[k]   = simde_mm256_unpacklo_epi8(tmp0,tmp1);   // e0(i) e1(i) e2(i) e3(i) ... e0(i+7) e1(i+7) e2(i+7) e3(i+7)
      f_256[k+1] = simde_mm256_unpackhi_epi8(tmp0,tmp1);   // e0(i+8) e1(i+8) e2(i+8) e3(i+8) ... e0(i+15) e1(i+15) e2(i+15) e3(i+15)
      tmp0   = simde_mm256_unpackhi_epi8(e0_256[j],e1_256[j]); // e0(i+16) e1(i+16) e0(i+17) e1(i+17) .... e0(i+31) e1(i+31)
      tmp1   = simde_mm256_unpackhi_epi8(e2_256[j],e3_256[j]); // e2(i+16) e3(i+16) e2(i+17) e3(i+17) .... e2(i+31) e3(i+31)
      f_256[k+2] = simde_mm256_unpacklo_epi8(tmp0,tmp1);
      f_256[k+3] = simde_mm256_unpackhi_epi8(tmp0,tmp1); 
    }
    break;
  case 6:
    e0=e;
    e1=e0+EQm;
    e2=e1+EQm;
    e3=e2+EQm;
    e4=e3+EQm;
    e5=e4+EQm;
    e0_256=(simde__m256i *)e0;
    e1_256=(simde__m256i *)e1;
    e2_256=(simde__m256i *)e2;
    e3_256=(simde__m256i *)e3;
    e4_256=(simde__m256i *)e4;
    e5_256=(simde__m256i *)e5;

    for (int j=0,k=0;j<EQm>>5;j++,k+=192) {
      fp  = f+k;
      fp2 = fp+96;

      tmp0   = simde_mm256_unpacklo_epi8(e0_256[j],e1_256[j]); // e0(i) e1(i) e0(i+1) e1(i+1) .... e0(i+15) e1(i+15)
      tmp1   = simde_mm256_unpacklo_epi8(e2_256[j],e3_256[j]); // e2(i) e3(i) e2(i+1) e3(i+1) .... e2(i+15) e3(i+15)
      tmp0b  = simde_mm256_unpacklo_epi16(tmp0,tmp1); // e0(i) e1(i) e2(i) e3(i) ... e0(i+7) e1(i+7) e2(i+7) e3(i+7)
      tmp1b  = simde_mm256_unpackhi_epi16(tmp0,tmp1); // e0(i+8) e1(i+8) e2(i+8) e3(i+8) ... e0(i+15) e1(i+15) e2(i+15) e3(i+15)
      tmp0   = simde_mm256_unpacklo_epi8(e4_256[j],e5_256[j]); // e4(i) e5(i) e4(i+1) e5(i+1) .... e4(i+15) e5(i+15)
      *((uint32_t*)fp)      = simde_mm256_extract_epi32(tmp0b,0);
      *((uint16_t*)(fp+4))  = simde_mm256_extract_epi16(tmp0,0);
      *((uint32_t*)(fp+6))  = simde_mm256_extract_epi32(tmp0b,1);
      *((uint16_t*)(fp+10)) = simde_mm256_extract_epi16(tmp0,1);
      *((uint32_t*)(fp+12)) = simde_mm256_extract_epi32(tmp0b,2);
      *((uint16_t*)(fp+16)) = simde_mm256_extract_epi16(tmp0,2);
      *((uint32_t*)(fp+18)) = simde_mm256_extract_epi32(tmp0b,3);
      *((uint16_t*)(fp+22)) = simde_mm256_extract_epi16(tmp0,3);
      *((uint32_t*)(fp+24)) = simde_mm256_extract_epi32(tmp0b,4);
      *((uint16_t*)(fp+26)) = simde_mm256_extract_epi16(tmp0,4);
      *((uint32_t*)(fp+30)) = simde_mm256_extract_epi32(tmp0b,5);
      *((uint16_t*)(fp+34)) = simde_mm256_extract_epi16(tmp0,5);
      *((uint32_t*)(fp+36)) = simde_mm256_extract_epi32(tmp0,6);
      *((uint16_t*)(fp+40)) = simde_mm256_extract_epi16(tmp0,6);
      *((uint32_t*)(fp+42)) = simde_mm256_extract_epi32(tmp0b,7);
      *((uint16_t*)(fp+46)) = simde_mm256_extract_epi16(tmp0,7);

      *((uint32_t*)(fp+48)) = simde_mm256_extract_epi32(tmp1b,0);
      *((uint16_t*)(fp+52)) = simde_mm256_extract_epi16(tmp0,8);
      *((uint32_t*)(fp+56)) = simde_mm256_extract_epi32(tmp1b,1);
      *((uint16_t*)(fp+60)) = simde_mm256_extract_epi16(tmp0,9);
      *((uint32_t*)(fp+62)) = simde_mm256_extract_epi32(tmp1b,2);
      *((uint16_t*)(fp+66)) = simde_mm256_extract_epi16(tmp0,10);
      *((uint32_t*)(fp+68)) = simde_mm256_extract_epi32(tmp1b,3);
      *((uint16_t*)(fp+72)) = simde_mm256_extract_epi16(tmp0,11);
      *((uint32_t*)(fp+74)) = simde_mm256_extract_epi32(tmp1b,4);
      *((uint16_t*)(fp+76)) = simde_mm256_extract_epi16(tmp0,12);
      *((uint32_t*)(fp+80)) = simde_mm256_extract_epi32(tmp1b,5);
      *((uint16_t*)(fp+82)) = simde_mm256_extract_epi16(tmp0,13);
      *((uint32_t*)(fp+86)) = simde_mm256_extract_epi32(tmp1b,6);
      *((uint16_t*)(fp+90)) = simde_mm256_extract_epi16(tmp0,14);
      *((uint32_t*)(fp+92)) = simde_mm256_extract_epi32(tmp1b,7);
      *((uint16_t*)(fp+94)) = simde_mm256_extract_epi16(tmp0,15);

      tmp0   = simde_mm256_unpackhi_epi8(e0_256[j],e1_256[j]); // e0(i+16) e1(i+16) e0(i+17) e1(i+17) .... e0(i+31) e1(i+31)
      tmp1   = simde_mm256_unpackhi_epi8(e2_256[j],e3_256[j]); // e2(i+16) e3(i+16) e2(i+17) e3(i+17) .... e2(i+31) e3(i+31)
      tmp0b  = simde_mm256_unpacklo_epi16(tmp0,tmp1); // e0(i+16) e1(i+16) e2(i+16) e3(i+16) ... e0(i+23) e1(i+23) e2(i+23) e3(i+23)
      tmp1b  = simde_mm256_unpackhi_epi16(tmp0,tmp1); // e0(i+24) e1(i+24) e2(i+24) e3(i+24) ... e0(i+31) e1(i+31) e2(i+31) e3(i+31)
      tmp0   = simde_mm256_unpackhi_epi8(e4_256[j],e5_256[j]); // e4(i+16) e5(i+16) e4(i+17) e5(i+17) .... e4(i+31) e5(i+31)
      *((uint32_t*)fp2)      = simde_mm256_extract_epi32(tmp0b,0);
      *((uint16_t*)(fp2+4))  = simde_mm256_extract_epi16(tmp0,0);
      *((uint32_t*)(fp2+6))  = simde_mm256_extract_epi32(tmp0b,1);
      *((uint16_t*)(fp2+10)) = simde_mm256_extract_epi16(tmp0,1);
      *((uint32_t*)(fp2+12)) = simde_mm256_extract_epi32(tmp0b,2);
      *((uint16_t*)(fp2+16)) = simde_mm256_extract_epi16(tmp0,2);
      *((uint32_t*)(fp2+18)) = simde_mm256_extract_epi32(tmp0b,3);
      *((uint16_t*)(fp2+22)) = simde_mm256_extract_epi16(tmp0,3);
      *((uint32_t*)(fp2+24)) = simde_mm256_extract_epi32(tmp0b,4);
      *((uint16_t*)(fp2+26)) = simde_mm256_extract_epi16(tmp0,4);
      *((uint32_t*)(fp2+30)) = simde_mm256_extract_epi32(tmp0b,5);
      *((uint16_t*)(fp2+34)) = simde_mm256_extract_epi16(tmp0,5);
      *((uint32_t*)(fp2+36)) = simde_mm256_extract_epi32(tmp0,6);
      *((uint16_t*)(fp2+40)) = simde_mm256_extract_epi16(tmp0,6);
      *((uint32_t*)(fp2+42)) = simde_mm256_extract_epi32(tmp0b,7);
      *((uint16_t*)(fp2+46)) = simde_mm256_extract_epi16(tmp0,7);

      *((uint32_t*)(fp2+48)) = simde_mm256_extract_epi32(tmp1b,0);
      *((uint16_t*)(fp2+52)) = simde_mm256_extract_epi16(tmp0,8);
      *((uint32_t*)(fp2+56)) = simde_mm256_extract_epi32(tmp1b,1);
      *((uint16_t*)(fp2+60)) = simde_mm256_extract_epi16(tmp0,9);
      *((uint32_t*)(fp2+62)) = simde_mm256_extract_epi32(tmp1b,2);
      *((uint16_t*)(fp2+66)) = simde_mm256_extract_epi16(tmp0,10);
      *((uint32_t*)(fp2+68)) = simde_mm256_extract_epi32(tmp1b,3);
      *((uint16_t*)(fp2+72)) = simde_mm256_extract_epi16(tmp0,11);
      *((uint32_t*)(fp2+74)) = simde_mm256_extract_epi32(tmp1b,4);
      *((uint16_t*)(fp2+76)) = simde_mm256_extract_epi16(tmp0,12);
      *((uint32_t*)(fp2+80)) = simde_mm256_extract_epi32(tmp1b,5);
      *((uint16_t*)(fp2+82)) = simde_mm256_extract_epi16(tmp0,13);
      *((uint32_t*)(fp2+86)) = simde_mm256_extract_epi32(tmp1b,6);
      *((uint16_t*)(fp2+90)) = simde_mm256_extract_epi16(tmp0,14);
      *((uint32_t*)(fp2+92)) = simde_mm256_extract_epi32(tmp1b,7);
      *((uint16_t*)(fp2+94)) = simde_mm256_extract_epi16(tmp0,15);
    }
    break;
  case 8:
    e0=e;
    e1=e0+EQm;
    e2=e1+EQm;
    e3=e2+EQm;
    e4=e3+EQm;
    e5=e4+EQm;
    e6=e5+EQm;
    e7=e6+EQm;

    e0_256=(simde__m256i *)e0;
    e1_256=(simde__m256i *)e1;
    e2_256=(simde__m256i *)e2;
    e3_256=(simde__m256i *)e3;
    e4_256=(simde__m256i *)e4;
    e5_256=(simde__m256i *)e5;
    e6_256=(simde__m256i *)e6;
    e7_256=(simde__m256i *)e7;
    for (int k=0,j=0;j<EQm>>5;j++,k+=8) {
      tmp0   = simde_mm256_unpacklo_epi8(e0_256[j],e1_256[j]); // e0(i) e1(i) e0(i+1) e1(i+1) .... e0(i+15) e1(i+15)
      tmp1   = simde_mm256_unpacklo_epi8(e2_256[j],e3_256[j]); // e2(i) e3(i) e2(i+1) e3(i+1) .... e2(i+15) e3(i+15)
      tmp2   = simde_mm256_unpacklo_epi8(e4_256[j],e5_256[j]); // e4(i) e5(i) e4(i+1) e5(i+1) .... e4(i+15) e5(i+15)
      tmp3   = simde_mm256_unpacklo_epi8(e6_256[j],e7_256[j]); // e6(i) e7(i) e6(i+1) e7(i+1) .... e6(i+15) e7(i+15)
      tmp4   = simde_mm256_unpacklo_epi16(tmp0,tmp1);  // e0(i) e1(i) e2(i) e3(i) ... e0(i+7) e1(i+7) e2(i+7) e3(i+7)
      tmp5   = simde_mm256_unpacklo_epi16(tmp2,tmp3);  // e4(i) e5(i) e6(i) e7(i) ... e4(i+7) e5(i+7) e6(i+7) e7(i+7)
      f_256[k]   = simde_mm256_unpacklo_epi16(tmp4,tmp5);  // e0(i) e1(i) e2(i) e3(i) e4(i) e5(i) e6(i) e7(i)... e0(i+3) e1(i+3) e2(i+3) e3(i+3) e4(i+3) e5(i+3) e6(i+3) e7(i+3))
      f_256[k+1] = simde_mm256_unpackhi_epi16(tmp4,tmp5);  // e0(i+4) e1(i+4) e2(i+4) e3(i+4) e4(i+4) e5(i+4) e6(i+4) e7(i+4)... e0(i+7) e1(i+7) e2(i+7) e3(i+7) e4(i+7) e5(i+7) e6(i+7) e7(i+7))

      tmp4   = simde_mm256_unpackhi_epi16(tmp0,tmp1);  // e0(i+8) e1(i+8) e2(i+8) e3(i+8) ... e0(i+15) e1(i+15) e2(i+15) e3(i+15)
      tmp5   = simde_mm256_unpackhi_epi16(tmp2,tmp3);  // e4(i+8) e5(i+8) e6(i+8) e7(i+8) ... e4(i+15) e5(i+15) e6(i+15) e7(i+15)
      f_256[k+2]   = simde_mm256_unpacklo_epi16(tmp4,tmp5);  // e0(i+8) e1(i+8) e2(i+8) e3(i+8) e4(i+8) e5(i+8) e6(i+8) e7(i+8)... e0(i+11) e1(i+11) e2(i+11) e3(i+11) e4(i+11) e5(i+11) e6(i+11) e7(i+11))
      f_256[k+3] = simde_mm256_unpackhi_epi16(tmp4,tmp5);  // e0(i+12) e1(i+12) e2(i+12) e3(i+12) e4(i+12) e5(i+12) e6(i+12) e7(i+12)... e0(i+15) e1(i+15) e2(i+15) e3(i+15) e4(i+15) e5(i+15) e6(i+15) e7(i+15))

      tmp0   = simde_mm256_unpackhi_epi8(e0_256[j],e1_256[j]); // e0(i+16) e1(i+16) e0(i+17) e1(i+17) .... e0(i+31) e1(i+31)
      tmp1   = simde_mm256_unpackhi_epi8(e2_256[j],e3_256[j]); // e2(i+16) e3(i+16) e2(i+17) e3(i+17) .... e2(i+31) e3(i+31)
      tmp2   = simde_mm256_unpackhi_epi8(e4_256[j],e5_256[j]); // e4(i+16) e5(i+16) e4(i+17) e5(i+17) .... e4(i+31) e5(i+31)
      tmp3   = simde_mm256_unpackhi_epi8(e6_256[j],e7_256[j]); // e6(i+16) e7(i+16) e6(i+17) e7(i+17) .... e6(i+31) e7(i+31)
      tmp4   = simde_mm256_unpacklo_epi16(tmp0,tmp1);  // e0(i+!6) e1(i+16) e2(i+16) e3(i+16) ... e0(i+23) e1(i+23) e2(i+23) e3(i+23)
      tmp5   = simde_mm256_unpacklo_epi16(tmp2,tmp3);  // e4(i+16) e5(i+16) e6(i+16) e7(i+16) ... e4(i+23) e5(i+23) e6(i+23) e7(i+23)
      f_256[k+4] = simde_mm256_unpacklo_epi16(tmp4,tmp5);  // e0(i+16) e1(i+16) e2(i+16) e3(i+16) e4(i+16) e5(i+16) e6(i+16) e7(i+16)... e0(i+19) e1(i+19) e2(i+19) e3(i+19) e4(i+19) e5(i+19) e6(i+19) e7(i+19))
      f_256[k+5] = simde_mm256_unpackhi_epi16(tmp4,tmp5);  // e0(i+20) e1(i+20) e2(i+20) e3(i+20) e4(i+20) e5(i+20) e6(i+20) e7(i+20)... e0(i+23) e1(i+23) e2(i+23) e3(i+23) e4(i+23) e5(i+23) e6(i+23) e7(i+23))

      tmp4   = simde_mm256_unpackhi_epi16(tmp0,tmp1);  // e0(i+24) e1(i+24) e2(i+24) e3(i+24) ... e0(i+31) e1(i+31) e2(i+31) e3(i+31)
      tmp5   = simde_mm256_unpackhi_epi16(tmp2,tmp3);  // e4(i+24) e5(i+24) e6(i+24) e7(i+24) ... e4(i+31) e5(i+31) e6(i+31) e7(i+31)
      f_256[k+6] = simde_mm256_unpacklo_epi16(tmp4,tmp5);  // e0(i+24) e1(i+24) e2(i+24) e3(i+24) e4(i+24) e5(i+24) e6(i+24) e7(i+24)... e0(i+27) e1(i+27) e2(i+27) e3(i+27) e4(i+27) e5(i+27) e6(i+27) e7(i+27))
      f_256[k+7] = simde_mm256_unpackhi_epi16(tmp4,tmp5);  // e0(i+28) e1(i+28) e2(i+28) e3(i+28) e4(i+28) e5(i+28) e6(i+28) e7(i+28)... e0(i+31) e1(i+31) e2(i+31) e3(i+31) e4(i+31) e5(i+31) e6(i+31) e7(i+31))
    }
    break;
  default: AssertFatal(1==0,"Should be here!\n");
  }

#else
  //original unoptimized loops
    /*
    for (int j = 0; j< EQm; j++,j2+=2){
      for (int i = 0; i< Qm; i++){
		  f[(i+j*Qm)] = e[(i*EQm + j)];
	  }
    }
    */

  fp=f;
  switch (Qm) {
  case 2:
    e0=e;
    e1=e0+EQm;
    for (int j = 0, j2 = 0; j< EQm; j++,j2+=2){
      fp=&f[j2];
      fp[0] = e0[j];
      fp[1] = e1[j];
    }
    break;
  case 4:
    e0=e;
    e1=e0+EQm;
    e2=e1+EQm;
    e3=e2+EQm;
    for (int j = 0, j2 = 0; j< EQm; j++,j2+=4){
      fp=&f[j2];
      fp[0] = e0[j];
      fp[1] = e1[j];
      fp[2] = e2[j];
      fp[3] = e3[j];
    }
    break;
  case 6:
    e0=e;
    e1=e0+EQm;
    e2=e1+EQm;
    e3=e2+EQm;
    e4=e3+EQm;
    e5=e4+EQm;
    fp = f;
    for (int j = 0; j< EQm; j++){
      *fp++ = e0[j];
      *fp++ = e1[j];
      *fp++ = e2[j];
      *fp++ = e3[j];
      *fp++ = e4[j];
      *fp++ = e5[j];
    }
    break;
  case 8:
    e0=e;
    e1=e0+EQm;
    e2=e1+EQm;
    e3=e2+EQm;
    e4=e3+EQm;
    e5=e4+EQm;
    e6=e5+EQm;
    e7=e6+EQm;
    for (int j = 0, j2 = 0; j< EQm; j++,j2+=8){
      fp=&f[j2];
      fp[0] = e0[j];
      fp[1] = e1[j];
      fp[2] = e2[j];
      fp[3] = e3[j];
      fp[4] = e4[j];
      fp[5] = e5[j];
      fp[6] = e6[j];
      fp[7] = e7[j];
    }
    break;
  default: AssertFatal(1==0,"Should never be here!\n");
  }
#endif
}




void nr_deinterleaving_ldpc(uint32_t E, uint8_t Qm, int16_t *e,int16_t *f)
{ 

  
  switch(Qm) {
  case 2:
    {
      AssertFatal(E%2==0,"");
      int16_t *e1=e+(E/2);
      int16_t *end=f+E-1;
      while( f<end ){
        *e++  = *f++;
        *e1++ = *f++;
      }
    }
    break;
  case 4:
    {
      AssertFatal(E%4==0,"");
      int16_t *e1=e+(E/4);
      int16_t *e2=e1+(E/4);
      int16_t *e3=e2+(E/4);
      int16_t *end=f+E-3;
      while( f<end ){ 
        *e++  = *f++;
        *e1++ = *f++;
        *e2++ = *f++;
        *e3++ = *f++;
      }
    }
    break;
  case 6:
    {
      AssertFatal(E%6==0,"");
      int16_t *e1=e+(E/6);
      int16_t *e2=e1+(E/6);
      int16_t *e3=e2+(E/6);
      int16_t *e4=e3+(E/6);
      int16_t *e5=e4+(E/6);
      int16_t *end=f+E-5;
     while( f<end ){ 
        *e++  = *f++;
        *e1++ = *f++;
        *e2++ = *f++;
        *e3++ = *f++;
        *e4++ = *f++;
        *e5++ = *f++;
      }
    }
    break;
  case 8:
    {
      AssertFatal(E%8==0,"");
      int16_t *e1=e+(E/8);
      int16_t *e2=e1+(E/8);
      int16_t *e3=e2+(E/8);
      int16_t *e4=e3+(E/8);
      int16_t *e5=e4+(E/8);
      int16_t *e6=e5+(E/8);
      int16_t *e7=e6+(E/8);
      int16_t *end=f+E-7;
      while( f<end ){
        *e++  = *f++;
        *e1++ = *f++;
        *e2++ = *f++;
        *e3++ = *f++;
        *e4++ = *f++;
        *e5++ = *f++;
        *e6++ = *f++;
        *e7++ = *f++;
      }
    }
    break;
  default:
    AssertFatal(1==0,"Should not get here : Qm %d\n",Qm);
    break;
  }

}

int nr_get_R_ldpc_decoder(int rvidx,
                          int E,
                          int BG,
                          int Z,
                          int *llrLen,
                          int round) {
  AssertFatal(BG == 1 || BG == 2, "Unknown BG %d\n", BG);

  int Ncb = (BG==1)?(66*Z):(50*Z);
  int infoBits = (index_k0[BG-1][rvidx] * Z + E);

  if (round == 0) *llrLen = infoBits;
  if (infoBits > Ncb) infoBits = Ncb;
  if (infoBits > *llrLen) *llrLen = infoBits;

  int sysBits = (BG==1)?(22*Z):(10*Z);
  float decoderR = (float)sysBits/(infoBits + 2*Z);

  if (BG == 2)
    if (decoderR < 0.3333)
      return 15;
    else if (decoderR < 0.6667)
      return 13;
    else
      return 23;
  else
    if (decoderR < 0.6667)
      return 13;
    else if (decoderR < 0.8889)
      return 23;
    else
      return 89;
}

int nr_rate_matching_ldpc(uint32_t Tbslbrm,
                          uint8_t BG,
                          uint16_t Z,
                          uint8_t *w,
                          uint8_t *e,
                          uint8_t C,
			  uint32_t F,
			  uint32_t Foffset,
                          uint8_t rvidx,
                          uint32_t E)
{
  uint32_t Ncb,ind,k=0,Nref,N;

  if (C==0) {
    LOG_E(PHY,"nr_rate_matching: invalid parameters (C %d\n",C);
    return -1;
  }

  //Bit selection
  N = (BG==1)?(66*Z):(50*Z);

  if (Tbslbrm == 0)
      Ncb = N;
  else {
      Nref = 3*Tbslbrm/(2*C); //R_LBRM = 2/3
      Ncb = min(N, Nref);
  }

  ind = (index_k0[BG-1][rvidx]*Ncb/N)*Z;

#ifdef RM_DEBUG
  printf("nr_rate_matching_ldpc: E %u, F %u, Foffset %u, k0 %u, Ncb %u, rvidx %d, Tbslbrm %u\n", E, F, Foffset,ind, Ncb, rvidx, Tbslbrm);
#endif

  if (Foffset > E) {
    LOG_E(PHY,"nr_rate_matching: invalid parameters (Foffset %d > E %d) F %d, k0 %d, Ncb %d, rvidx %d, Tbslbrm %d\n",Foffset,E,F, ind, Ncb, rvidx, Tbslbrm);
    return -1;
  }
  if (Foffset > Ncb) {
    LOG_E(PHY,"nr_rate_matching: invalid parameters (Foffset %d > Ncb %d)\n",Foffset,Ncb);
    return -1;
  }

  if (ind >= Foffset && ind < (F+Foffset)) ind = F+Foffset;

  if (ind < Foffset) { // case where we have some bits before the filler and the rest after
    memcpy((void*)e,(void*)(w+ind),Foffset-ind);

    if (E + F <= Ncb-ind) { // E+F doesn't contain all coded bits
      memcpy((void*)(e+Foffset-ind),(void*)(w+Foffset+F),E-Foffset+ind);
      k=E;
    }
    else {
      memcpy((void*)(e+Foffset-ind),(void*)(w+Foffset+F),Ncb-Foffset-F);
      k=Ncb-F-ind;
    }
  }
  else {
    if (E <= Ncb-ind) { //E+F doesn't contain all coded bits
      memcpy((void*)(e),(void*)(w+ind),E);
      k=E;
    }
    else {
      memcpy((void*)(e),(void*)(w+ind),Ncb-ind);
      k=Ncb-ind;
    }
  }

  while(k<E) { // case where we do repetitions (low mcs)
    for (ind=0; (ind<Ncb)&&(k<E); ind++) {

#ifdef RM_DEBUG
      printf("RM_TX k%u Ind: %u (%d)\n",k,ind,w[ind]);
#endif

      if (w[ind] != NR_NULL) e[k++]=w[ind];
    }
  }


  return 0;
}

int nr_rate_matching_ldpc_rx(uint32_t Tbslbrm,
                             uint8_t BG,
                             uint16_t Z,
                             int16_t *w,
                             int16_t *soft_input,
                             uint8_t C,
                             uint8_t rvidx,
                             uint8_t clear,
                             uint32_t E,
                             uint32_t F,
                             uint32_t Foffset)
{
  uint32_t Ncb,ind,k,Nref,N;

#ifdef RM_DEBUG
  int nulled=0;
#endif

  if (C==0) {
    LOG_E(PHY,"nr_rate_matching: invalid parameters (C %d\n",C);
    return -1;
  }

  //Bit selection
  N = (BG==1)?(66*Z):(50*Z);

  if (Tbslbrm == 0)
    Ncb = N;
  else {
    Nref = (3*Tbslbrm/(2*C)); //R_LBRM = 2/3
    Ncb = min(N, Nref);
  }

  ind = (index_k0[BG-1][rvidx]*Ncb/N)*Z;
  if (Foffset > E) {
    LOG_E(PHY,"nr_rate_matching: invalid parameters (Foffset %d > E %d)\n",Foffset,E);
    return -1;
  }
  if (Foffset > Ncb) {
    LOG_E(PHY,"nr_rate_matching: invalid parameters (Foffset %d > Ncb %d)\n",Foffset,Ncb);
    return -1;
  }

#ifdef RM_DEBUG
  printf("nr_rate_matching_ldpc_rx: Clear %d, E %u, k0 %u, Ncb %u, rvidx %d, Tbslbrm %u\n", clear, E, ind, Ncb, rvidx, Tbslbrm);
#endif

  if (clear == 1)
    memset(w, 0, Ncb * sizeof(int16_t));

  k=0;

  if (ind < Foffset)
    for (; (ind<Foffset)&&(k<E); ind++) {
#ifdef RM_DEBUG
      printf("RM_RX k%u Ind %u(before filler): %d (%d)=>",k,ind,w[ind],soft_input[k]);
#endif
      w[ind]+=soft_input[k++];
#ifdef RM_DEBUG
      printf("%d\n",w[ind]);
#endif
    }
  if (ind >= Foffset && ind < Foffset+F) ind=Foffset+F;

  for (; (ind<Ncb)&&(k<E); ind++) {
#ifdef RM_DEBUG
    printf("RM_RX k%u Ind %u(after filler) %d (%d)=>",k,ind,w[ind],soft_input[k]);
#endif
      w[ind] += soft_input[k++];
#ifdef RM_DEBUG
      printf("%d\n",w[ind]);
#endif
  }

  while(k<E) {
    for (ind=0; (ind<Foffset)&&(k<E); ind++) {
#ifdef RM_DEBUG
      printf("RM_RX k%u Ind %u(before filler) %d(%d)=>",k,ind,w[ind],soft_input[k]);
#endif
      w[ind]+=soft_input[k++];
#ifdef RM_DEBUG
      printf("%d\n",w[ind]);
#endif
    }
    for (ind=Foffset+F; (ind<Ncb)&&(k<E); ind++) {
#ifdef RM_DEBUG
      printf("RM_RX (after filler) k%u Ind: %u (%d)(soft in %d)=>",k,ind,w[ind],soft_input[k]);
#endif
      w[ind] += soft_input[k++];
#ifdef RM_DEBUG
      printf("%d\n",w[ind]);
#endif
    }
  }

  return 0;
}

