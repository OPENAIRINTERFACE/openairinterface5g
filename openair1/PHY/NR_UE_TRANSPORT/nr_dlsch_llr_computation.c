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

/*! \file PHY/NR_UE_TRANSPORT/nr_dlsch_llr_computation.c
 * \brief Top-level routines for LLR computation of the PDSCH physical channel
 * \author H. WANG
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email:
 * \note
 * \warning
 */

#include "PHY/defs_nr_UE.h"
#include "PHY/phy_extern_nr_ue.h"
#include "nr_transport_proto_ue.h"
#include "PHY/TOOLS/tools_defs.h"
#include "PHY/sse_intrin.h"

//#define DEBUG_LLR_SIC

int16_t nr_zeros[8] __attribute__((aligned(16))) = {0, 0, 0, 0, 0, 0, 0, 0};

//==============================================================================================
// SINGLE-STREAM
//==============================================================================================

//----------------------------------------------------------------------------------------------
// QPSK
//----------------------------------------------------------------------------------------------

int nr_dlsch_qpsk_llr(NR_DL_FRAME_PARMS *frame_parms,
                   int32_t *rxdataF_comp,
                   int16_t *dlsch_llr,
                   uint8_t symbol,
                   uint32_t len,
                   uint8_t first_symbol_flag,
                   uint16_t nb_rb)
{

  c16_t *rxF   = (c16_t *)&rxdataF_comp[((int32_t)symbol*nb_rb*12)];
  c16_t *llr32 = (c16_t *)dlsch_llr;
  int i;

  if (!llr32) {
    LOG_E(PHY,"nr_dlsch_qpsk_llr: llr is null, symbol %d, llr32=%p\n",symbol, llr32);
    return(-1);
  }

  /*
  LOG_I(PHY,"dlsch_qpsk_llr: [symb %d / Length %d]: @LLR Buff %x, @LLR Buff(symb) %x \n",
             symbol,
             len,
             dlsch_llr,
             llr32);
  */
  for (i=0; i<len; i++) {
    //*llr32 = *rxF;
    llr32->r = rxF->r >> 3;
    llr32->i = rxF->i >> 3;
    LOG_D(PHY,"dlsch_qpsk_llr %d : (%d,%d)\n", i, llr32->r, llr32->i);
    rxF++;
    llr32++;
  }
  return(0);
}


//----------------------------------------------------------------------------------------------
// 16-QAM
//----------------------------------------------------------------------------------------------

void nr_dlsch_16qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                     int32_t *rxdataF_comp,
                     int16_t *dlsch_llr,
                     int32_t *dl_ch_mag,
                     uint8_t symbol,
                     uint32_t len,
                     uint8_t first_symbol_flag,
                     uint16_t nb_rb)
{

  simde__m128i *rxF = (simde__m128i*)&rxdataF_comp[(symbol*nb_rb*12)];
  simde__m128i *ch_mag;
  simde__m128i llr128[2];
  uint32_t *llr32;


  int i;
  unsigned char len_mod4=0;


    llr32 = (uint32_t*)dlsch_llr;
    ch_mag = (simde__m128i *)dl_ch_mag;


 // printf("len=%d\n", len);
  len_mod4 = len&3;
 // printf("len_mod4=%d\n", len_mod4);
  len>>=2;  // length in quad words (4 REs)
 // printf("len>>=2=%d\n", len);
  len+=(len_mod4==0 ? 0 : 1);
 // printf("len+=%d\n", len);
  for (i=0; i<len; i++) {

    simde__m128i xmm0 =simde_mm_abs_epi16(rxF[i]);
    xmm0 =simde_mm_subs_epi16(ch_mag[i],xmm0);

    // lambda_1=y_R, lambda_2=|y_R|-|h|^2, lamda_3=y_I, lambda_4=|y_I|-|h|^2
    llr128[0] =simde_mm_unpacklo_epi32(rxF[i],xmm0);
    llr128[1] =simde_mm_unpackhi_epi32(rxF[i],xmm0);
    llr32[0] =simde_mm_extract_epi32(llr128[0],0); //((uint32_t *)&llr128[0])[0];
    llr32[1] =simde_mm_extract_epi32(llr128[0],1); //((uint32_t *)&llr128[0])[1];
    llr32[2] =simde_mm_extract_epi32(llr128[0],2); //((uint32_t *)&llr128[0])[2];
    llr32[3] =simde_mm_extract_epi32(llr128[0],3); //((uint32_t *)&llr128[0])[3];
    llr32[4] =simde_mm_extract_epi32(llr128[1],0); //((uint32_t *)&llr128[1])[0];
    llr32[5] =simde_mm_extract_epi32(llr128[1],1); //((uint32_t *)&llr128[1])[1];
    llr32[6] =simde_mm_extract_epi32(llr128[1],2); //((uint32_t *)&llr128[1])[2];
    llr32[7] =simde_mm_extract_epi32(llr128[1],3); //((uint32_t *)&llr128[1])[3];
    llr32+=8;

  }

 simde_mm_empty();
 simde_m_empty();
}

//----------------------------------------------------------------------------------------------
// 64-QAM
//----------------------------------------------------------------------------------------------

void nr_dlsch_64qam_llr(NR_DL_FRAME_PARMS *frame_parms,
			int32_t *rxdataF_comp,
			int16_t *dlsch_llr,
			int32_t *dl_ch_mag,
			int32_t *dl_ch_magb,
			uint8_t symbol,
			uint32_t len,
			uint8_t first_symbol_flag,
			uint16_t nb_rb)
{
  simde__m128i *rxF = (simde__m128i*)&rxdataF_comp[(symbol*nb_rb*12)];
  simde__m128i *ch_mag,*ch_magb;
  int i,len2;
  unsigned char len_mod4;
  int16_t *llr2;

  llr2 = dlsch_llr;

  ch_mag = (simde__m128i *)dl_ch_mag;
  ch_magb = (simde__m128i *)dl_ch_magb;

//  printf("nr_dlsch_64qam_llr: symbol %d,nb_rb %d, len %d,pbch_pss_sss_adjust %d\n",symbol,nb_rb,len,pbch_pss_sss_adjust);

/*  LOG_I(PHY,"nr_dlsch_64qam_llr [symb %d / FirstSym %d / Length %d]: @LLR Buff %x \n",
             symbol,
             first_symbol_flag,
             len,
             dlsch_llr,
             pllr_symbol);*/

  len_mod4 =len&3;
  len2=len>>2;  // length in quad words (4 REs)
  len2+=((len_mod4==0)?0:1);

  for (i=0; i<len2; i++) {
     simde__m128i xmm1, xmm2;
    xmm1 =simde_mm_abs_epi16(rxF[i]);
    xmm1 =simde_mm_subs_epi16(ch_mag[i],xmm1);
    xmm2 =simde_mm_abs_epi16(xmm1);
    xmm2 =simde_mm_subs_epi16(ch_magb[i],xmm2);

    // loop over all LLRs in quad word (24 coded bits)
    /*
      for (j=0;j<8;j+=2) {
      llr2[0] = ((short *)&rxF[i])[j];
      llr2[1] = ((short *)&rxF[i])[j+1];
      llr2[2] = ((short *)&xmm1)[j];
      llr2[3] = ((short *)&xmm1)[j+1];
      llr2[4] = ((short *)&xmm2)[j];
      llr2[5] = ((short *)&xmm2)[j+1];

     llr2+=6;
      }
    */
    llr2[0] = ((short *)&rxF[i])[0];
    llr2[1] = ((short *)&rxF[i])[1];
    llr2[2] =simde_mm_extract_epi16(xmm1,0);
    llr2[3] =simde_mm_extract_epi16(xmm1,1);//((short *)&xmm1)[j+1];
    llr2[4] =simde_mm_extract_epi16(xmm2,0);//((short *)&xmm2)[j];
    llr2[5] =simde_mm_extract_epi16(xmm2,1);//((short *)&xmm2)[j+1];

    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[2];
    llr2[1] = ((short *)&rxF[i])[3];
    llr2[2] =simde_mm_extract_epi16(xmm1,2);
    llr2[3] =simde_mm_extract_epi16(xmm1,3);//((short *)&xmm1)[j+1];
    llr2[4] =simde_mm_extract_epi16(xmm2,2);//((short *)&xmm2)[j];
    llr2[5] =simde_mm_extract_epi16(xmm2,3);//((short *)&xmm2)[j+1];

    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[4];
    llr2[1] = ((short *)&rxF[i])[5];
    llr2[2] =simde_mm_extract_epi16(xmm1,4);
    llr2[3] =simde_mm_extract_epi16(xmm1,5);//((short *)&xmm1)[j+1];
    llr2[4] =simde_mm_extract_epi16(xmm2,4);//((short *)&xmm2)[j];
    llr2[5] =simde_mm_extract_epi16(xmm2,5);//((short *)&xmm2)[j+1];
    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[6];
    llr2[1] = ((short *)&rxF[i])[7];
    llr2[2] =simde_mm_extract_epi16(xmm1,6);
    llr2[3] =simde_mm_extract_epi16(xmm1,7);//((short *)&xmm1)[j+1];
    llr2[4] =simde_mm_extract_epi16(xmm2,6);//((short *)&xmm2)[j];
    llr2[5] =simde_mm_extract_epi16(xmm2,7);//((short *)&xmm2)[j+1];
    llr2+=6;

  }

 simde_mm_empty();
 simde_m_empty();
}

//----------------------------------------------------------------------------------------------
// 256-QAM
//----------------------------------------------------------------------------------------------

void nr_dlsch_256qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                     int32_t *rxdataF_comp,
                     int16_t *dlsch_llr,
                     int32_t *dl_ch_mag,
                     int32_t *dl_ch_magb,
                     int32_t *dl_ch_magr,
                     uint8_t symbol,
                     uint32_t len,
                     uint8_t first_symbol_flag,
                     uint16_t nb_rb)
{
  simde__m128i *rxF = (simde__m128i*)&rxdataF_comp[(symbol*nb_rb*12)];
  simde__m128i *ch_mag,*ch_magb,*ch_magr;

  int i,len2;
  unsigned char len_mod4;
  int16_t *llr2;

  llr2 = dlsch_llr;

  ch_mag = (simde__m128i *)dl_ch_mag;
  ch_magb = (simde__m128i *)dl_ch_magb;
  ch_magr = (simde__m128i *)dl_ch_magr;

  len_mod4 =len&3;
  len2=len>>2;  // length in quad words (4 REs)
  len2+=((len_mod4==0)?0:1);

  for (i=0; i<len2; i++) {
    simde__m128i xmm1 = simde_mm_abs_epi16(rxF[i]);
    xmm1 = simde_mm_subs_epi16(ch_mag[i],xmm1);
    simde__m128i xmm2 = simde_mm_abs_epi16(xmm1);
    xmm2 = simde_mm_subs_epi16(ch_magb[i],xmm2);
    simde__m128i xmm3 = simde_mm_abs_epi16(xmm2);
    xmm3 = simde_mm_subs_epi16(ch_magr[i], xmm3);

    llr2[0] = ((short *)&rxF[i])[0];
    llr2[1] = ((short *)&rxF[i])[1];
    llr2[2] = simde_mm_extract_epi16(xmm1,0);
    llr2[3] = simde_mm_extract_epi16(xmm1,1);//((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2,0);//((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2,1);//((short *)&xmm2)[j+1];
    llr2[6] = simde_mm_extract_epi16(xmm3,0);
    llr2[7] = simde_mm_extract_epi16(xmm3,1);

    llr2+=8;
    llr2[0] = ((short *)&rxF[i])[2];
    llr2[1] = ((short *)&rxF[i])[3];
    llr2[2] = simde_mm_extract_epi16(xmm1,2);
    llr2[3] = simde_mm_extract_epi16(xmm1,3);//((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2,2);//((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2,3);//((short *)&xmm2)[j+1];
    llr2[6] = simde_mm_extract_epi16(xmm3,2);
    llr2[7] = simde_mm_extract_epi16(xmm3,3);

    llr2+=8;
    llr2[0] = ((short *)&rxF[i])[4];
    llr2[1] = ((short *)&rxF[i])[5];
    llr2[2] = simde_mm_extract_epi16(xmm1,4);
    llr2[3] = simde_mm_extract_epi16(xmm1,5);//((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2,4);//((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2,5);//((short *)&xmm2)[j+1];
    llr2[6] = simde_mm_extract_epi16(xmm3,4);
    llr2[7] = simde_mm_extract_epi16(xmm3,5);

    llr2+=8;
    llr2[0] = ((short *)&rxF[i])[6];
    llr2[1] = ((short *)&rxF[i])[7];
    llr2[2] = simde_mm_extract_epi16(xmm1,6);
    llr2[3] = simde_mm_extract_epi16(xmm1,7);//((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2,6);//((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2,7);//((short *)&xmm2)[j+1];
    llr2[6] = simde_mm_extract_epi16(xmm3,6);
    llr2[7] = simde_mm_extract_epi16(xmm3,7);
    llr2+=8;

  }

 simde_mm_empty();
 simde_m_empty();
}

//==============================================================================================
// DUAL-STREAM
//==============================================================================================

//----------------------------------------------------------------------------------------------
// QPSK
//----------------------------------------------------------------------------------------------

simde__m128i  y0r_over2 __attribute__ ((aligned(16)));
simde__m128i  y0i_over2 __attribute__ ((aligned(16)));
simde__m128i  y1r_over2 __attribute__ ((aligned(16)));
simde__m128i  y1i_over2 __attribute__ ((aligned(16)));

simde__m128i  A __attribute__ ((aligned(16)));
simde__m128i  B __attribute__ ((aligned(16)));
simde__m128i  C __attribute__ ((aligned(16)));
simde__m128i  D __attribute__ ((aligned(16)));
simde__m128i  E __attribute__ ((aligned(16)));
simde__m128i  F __attribute__ ((aligned(16)));
simde__m128i  G __attribute__ ((aligned(16)));
simde__m128i  H __attribute__ ((aligned(16)));



void nr_qpsk_qpsk(short *stream0_in,
               short *stream1_in,
               short *stream0_out,
               short *rho01,
               int length
         )
{

  /*
    This function computes the LLRs of stream 0 (s_0) in presence of the interfering stream 1 (s_1) assuming that both symbols are QPSK. It can be used for both MU-MIMO interference-aware receiver or for SU-MIMO receivers.

    Parameters:
    stream0_in = Matched filter output y0' = (h0*g0)*y0
    stream1_in = Matched filter output y1' = (h0*g1)*y0
    stream0_out = LLRs
    rho01 = Correlation between the two effective channels \rho_{10} = (h1*g1)*(h0*g0)
    length = number of resource elements
  */

  simde__m128i *rho01_128i = (simde__m128i *)rho01;
  simde__m128i *stream0_128i_in = (simde__m128i *)stream0_in;
  simde__m128i *stream1_128i_in = (simde__m128i *)stream1_in;
  simde__m128i *stream0_128i_out = (simde__m128i *)stream0_out;
  simde__m128i ONE_OVER_SQRT_8 =simde_mm_set1_epi16(23170); //round(2^16/sqrt(8))

  int i;


  for (i=0; i<length>>2; i+=2) {
    // in each iteration, we take 8 complex samples
    simde__m128i xmm0 = rho01_128i[i]; // 4 symbols
    simde__m128i xmm1 = rho01_128i[i+1];

    // put (rho_r + rho_i)/2sqrt2 in rho_rpi
    // put (rho_r - rho_i)/2sqrt2 in rho_rmi

    xmm0 =simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 =simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 =simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 =simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 =simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 =simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i xmm2 = simde_mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    simde__m128i xmm3 = simde_mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    simde__m128i rho_rpi = simde_mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    simde__m128i rho_rmi = simde_mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // divide by sqrt(8), no shift needed ONE_OVER_SQRT_8 = Q1.16
    rho_rpi =simde_mm_mulhi_epi16(rho_rpi,ONE_OVER_SQRT_8);
    rho_rmi =simde_mm_mulhi_epi16(rho_rmi,ONE_OVER_SQRT_8);
    // Compute LLR for first bit of stream 0

    // Compute real and imaginary parts of MF output for stream 0
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];

    xmm0 =simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 =simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 =simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 =simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 =simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 =simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y0r = simde_mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    simde__m128i y0i = simde_mm_unpackhi_epi64(xmm0, xmm1);

    simde__m128i  y0r_over2 = simde_mm_srai_epi16(y0r, 1); // divide by 2
    simde__m128i  y0i_over2 = simde_mm_srai_epi16(y0i, 1); // divide by 2
    // Compute real and imaginary parts of MF output for stream 1
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];

    xmm0 =simde_mm_shufflelo_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 =simde_mm_shufflehi_epi16(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm0 =simde_mm_shuffle_epi32(xmm0,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 =simde_mm_shufflelo_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 =simde_mm_shufflehi_epi16(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    xmm1 =simde_mm_shuffle_epi32(xmm1,0xd8); // SIMDE_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    simde__m128i y1r = simde_mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    simde__m128i y1i = simde_mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    simde__m128i y1r_over2 = simde_mm_srai_epi16(y1r, 1); // divide by 2
    simde__m128i y1i_over2 = simde_mm_srai_epi16(y1i, 1); // divide by 2

    // Compute the terms for the LLR of first bit

    xmm0 =simde_mm_setzero_si128(); // ZERO

    // 1 term for numerator of LLR
    xmm3 =simde_mm_subs_epi16(y1r_over2,rho_rpi);
    A =simde_mm_abs_epi16(xmm3); // A = |y1r/2 - rho/sqrt(8)|
    xmm2 =simde_mm_adds_epi16(A,y0i_over2); // = |y1r/2 - rho/sqrt(8)| + y0i/2
    xmm3 =simde_mm_subs_epi16(y1i_over2,rho_rmi);
    B =simde_mm_abs_epi16(xmm3); // B = |y1i/2 - rho*/sqrt(8)|
    simde__m128i logmax_num_re0 =simde_mm_adds_epi16(B,xmm2); // = |y1r/2 - rho/sqrt(8)|+|y1i/2 - rho*/sqrt(8)| + y0i/2

    // 2 term for numerator of LLR
    xmm3 =simde_mm_subs_epi16(y1r_over2,rho_rmi);
    C =simde_mm_abs_epi16(xmm3); // C = |y1r/2 - rho*/4|
    xmm2 =simde_mm_subs_epi16(C,y0i_over2); // = |y1r/2 - rho*/4| - y0i/2
    xmm3 =simde_mm_adds_epi16(y1i_over2,rho_rpi);
    D =simde_mm_abs_epi16(xmm3); // D = |y1i/2 + rho/4|
    xmm2 =simde_mm_adds_epi16(xmm2,D); // |y1r/2 - rho*/4| + |y1i/2 + rho/4| - y0i/2
    logmax_num_re0 =simde_mm_max_epi16(logmax_num_re0,xmm2); // max, numerator done

    // 1 term for denominator of LLR
    xmm3 =simde_mm_adds_epi16(y1r_over2,rho_rmi);
    E =simde_mm_abs_epi16(xmm3); // E = |y1r/2 + rho*/4|
    xmm2 =simde_mm_adds_epi16(E,y0i_over2); // = |y1r/2 + rho*/4| + y0i/2
    xmm3 =simde_mm_subs_epi16(y1i_over2,rho_rpi);
    F =simde_mm_abs_epi16(xmm3); // F = |y1i/2 - rho/4|
    simde__m128i logmax_den_re0 =simde_mm_adds_epi16(F,xmm2); // = |y1r/2 + rho*/4| + |y1i/2 - rho/4| + y0i/2

    // 2 term for denominator of LLR
    xmm3 =simde_mm_adds_epi16(y1r_over2,rho_rpi);
    G =simde_mm_abs_epi16(xmm3); // G = |y1r/2 + rho/4|
    xmm2 =simde_mm_subs_epi16(G,y0i_over2); // = |y1r/2 + rho/4| - y0i/2
    xmm3 =simde_mm_adds_epi16(y1i_over2,rho_rmi);
    H =simde_mm_abs_epi16(xmm3); // H = |y1i/2 + rho*/4|
    xmm2 =simde_mm_adds_epi16(xmm2,H); // = |y1r/2 + rho/4| + |y1i/2 + rho*/4| - y0i/2
    logmax_den_re0 =simde_mm_max_epi16(logmax_den_re0,xmm2); // max, denominator done

    // Compute the terms for the LLR of first bit

    // 1 term for nominator of LLR
    xmm2 = simde_mm_adds_epi16(A,y0r_over2);
    simde__m128i logmax_num_im0 = simde_mm_adds_epi16(B, xmm2); // = |y1r/2 - rho/4| + |y1i/2 - rho*/4| + y0r/2

    // 2 term for nominator of LLR
    xmm2 =simde_mm_subs_epi16(E,y0r_over2);
    xmm2 =simde_mm_adds_epi16(xmm2,F); // = |y1r/2 + rho*/4| + |y1i/2 - rho/4| - y0r/2

    logmax_num_im0 =simde_mm_max_epi16(logmax_num_im0,xmm2); // max, nominator done

    // 1 term for denominator of LLR
    xmm2 =simde_mm_adds_epi16(C,y0r_over2);
    simde__m128i logmax_den_im0 =simde_mm_adds_epi16(D,xmm2); // = |y1r/2 - rho*/4| + |y1i/2 + rho/4| - y0r/2

    xmm2 =simde_mm_subs_epi16(G,y0r_over2);
    xmm2 =simde_mm_adds_epi16(xmm2,H); // = |y1r/2 + rho/4| + |y1i/2 + rho*/4| - y0r/2

    logmax_den_im0 =simde_mm_max_epi16(logmax_den_im0,xmm2); // max, denominator done

    // LLR of first bit [L1(1), L1(2), L1(3), L1(4)]
    y0r =simde_mm_adds_epi16(y0r,logmax_num_re0);
    y0r =simde_mm_subs_epi16(y0r,logmax_den_re0);

    // LLR of second bit [L2(1), L2(2), L2(3), L2(4)]
    y0i =simde_mm_adds_epi16(y0i,logmax_num_im0);
    y0i =simde_mm_subs_epi16(y0i,logmax_den_im0);

   simde_mm_storeu_si128(&stream0_128i_out[i],simde_mm_unpacklo_epi16(y0r,y0i)); // = [L1(1), L2(1), L1(2), L2(2)]

    if (i<((length>>1) - 1)) // false if only 2 REs remain
     simde_mm_storeu_si128(&stream0_128i_out[i+1],simde_mm_unpackhi_epi16(y0r,y0i));

  }

}
