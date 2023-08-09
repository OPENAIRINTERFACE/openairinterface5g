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


int16_t nr_zeros[8] __attribute__ ((aligned(16))) = {0,0,0,0,0,0,0,0};
int16_t nr_ones[8] __attribute__ ((aligned(16))) = {0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff};
#if defined(__x86_64__) || defined(__i386__)
__m128i rho_rpi __attribute__ ((aligned(16)));
__m128i rho_rmi __attribute__((aligned(16)));
#endif

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

#if defined(__x86_64__) || defined(__i386__)
  __m128i *rxF = (__m128i*)&rxdataF_comp[(symbol*nb_rb*12)];
  __m128i *ch_mag;
  __m128i llr128[2];
  uint32_t *llr32;
#elif defined(__arm__) || defined(__aarch64__)
  int16x8_t *rxF = (int16x8_t*)&rxdataF_comp[(symbol*nb_rb*12)];
  int16x8_t *ch_mag;
  int16x8_t xmm0;
  int16_t *llr16;
#endif


  int i;
  unsigned char len_mod4=0;


#if defined(__x86_64__) || defined(__i386__)
    llr32 = (uint32_t*)dlsch_llr;
#elif defined(__arm__) || defined(__aarch64__)
    llr16 = (int16_t*)dlsch_llr;
#endif

#if defined(__x86_64__) || defined(__i386__)
    ch_mag = (__m128i *)dl_ch_mag;
#elif defined(__arm__) || defined(__aarch64__)
    ch_mag = (int16x8_t *)dl_ch_mag;
#endif


 // printf("len=%d\n", len);
  len_mod4 = len&3;
 // printf("len_mod4=%d\n", len_mod4);
  len>>=2;  // length in quad words (4 REs)
 // printf("len>>=2=%d\n", len);
  len+=(len_mod4==0 ? 0 : 1);
 // printf("len+=%d\n", len);
  for (i=0; i<len; i++) {

#if defined(__x86_64__) || defined(__i386)
    __m128i xmm0;
    xmm0 = _mm_abs_epi16(rxF[i]);
    xmm0 = _mm_subs_epi16(ch_mag[i],xmm0);

    // lambda_1=y_R, lambda_2=|y_R|-|h|^2, lamda_3=y_I, lambda_4=|y_I|-|h|^2
    llr128[0] = _mm_unpacklo_epi32(rxF[i],xmm0);
    llr128[1] = _mm_unpackhi_epi32(rxF[i],xmm0);
    llr32[0] = _mm_extract_epi32(llr128[0],0); //((uint32_t *)&llr128[0])[0];
    llr32[1] = _mm_extract_epi32(llr128[0],1); //((uint32_t *)&llr128[0])[1];
    llr32[2] = _mm_extract_epi32(llr128[0],2); //((uint32_t *)&llr128[0])[2];
    llr32[3] = _mm_extract_epi32(llr128[0],3); //((uint32_t *)&llr128[0])[3];
    llr32[4] = _mm_extract_epi32(llr128[1],0); //((uint32_t *)&llr128[1])[0];
    llr32[5] = _mm_extract_epi32(llr128[1],1); //((uint32_t *)&llr128[1])[1];
    llr32[6] = _mm_extract_epi32(llr128[1],2); //((uint32_t *)&llr128[1])[2];
    llr32[7] = _mm_extract_epi32(llr128[1],3); //((uint32_t *)&llr128[1])[3];
    llr32+=8;
#elif defined(__arm__) || defined(__aarch64__)
    xmm0 = vabsq_s16(rxF[i]);
    xmm0 = vqsubq_s16(ch_mag[i],xmm0);
    // lambda_1=y_R, lambda_2=|y_R|-|h|^2, lamda_3=y_I, lambda_4=|y_I|-|h|^2

    llr16[0] = vgetq_lane_s16(rxF[i],0);
    llr16[1] = vgetq_lane_s16(rxF[i],1);
    llr16[2] = vgetq_lane_s16(xmm0,0);
    llr16[3] = vgetq_lane_s16(xmm0,1);
    llr16[4] = vgetq_lane_s16(rxF[i],2);
    llr16[5] = vgetq_lane_s16(rxF[i],3);
    llr16[6] = vgetq_lane_s16(xmm0,2);
    llr16[7] = vgetq_lane_s16(xmm0,3);
    llr16[8] = vgetq_lane_s16(rxF[i],4);
    llr16[9] = vgetq_lane_s16(rxF[i],5);
    llr16[10] = vgetq_lane_s16(xmm0,4);
    llr16[11] = vgetq_lane_s16(xmm0,5);
    llr16[12] = vgetq_lane_s16(rxF[i],6);
    llr16[13] = vgetq_lane_s16(rxF[i],6);
    llr16[14] = vgetq_lane_s16(xmm0,7);
    llr16[15] = vgetq_lane_s16(xmm0,7);
    llr16+=16;
#endif

  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
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
#if defined(__x86_64__) || defined(__i386__)
  __m128i *rxF = (__m128i*)&rxdataF_comp[(symbol*nb_rb*12)];
  __m128i *ch_mag,*ch_magb;
#elif defined(__arm__) || defined(__aarch64__)
  int16x8_t *rxF = (int16x8_t*)&rxdataF_comp[(symbol*nb_rb*12)];
  int16x8_t *ch_mag,*ch_magb,xmm1,xmm2;
#endif
  int i,len2;
  unsigned char len_mod4;
  int16_t *llr2;

  llr2 = dlsch_llr;

#if defined(__x86_64__) || defined(__i386__)
  ch_mag = (__m128i *)dl_ch_mag;
  ch_magb = (__m128i *)dl_ch_magb;
#elif defined(__arm__) || defined(__aarch64__)
  ch_mag = (int16x8_t *)dl_ch_mag;
  ch_magb = (int16x8_t *)dl_ch_magb;
#endif

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

#if defined(__x86_64__) || defined(__i386__)
    __m128i xmm1, xmm2;

    xmm1 = _mm_abs_epi16(rxF[i]);
    xmm1 = _mm_subs_epi16(ch_mag[i],xmm1);
    xmm2 = _mm_abs_epi16(xmm1);
    xmm2 = _mm_subs_epi16(ch_magb[i],xmm2);
#elif defined(__arm__) || defined(__aarch64__)
    xmm1 = vabsq_s16(rxF[i]);
    xmm1 = vsubq_s16(ch_mag[i],xmm1);
    xmm2 = vabsq_s16(xmm1);
    xmm2 = vsubq_s16(ch_magb[i],xmm2);
#endif
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
#if defined(__x86_64__) || defined(__i386__)
    llr2[2] = _mm_extract_epi16(xmm1,0);
    llr2[3] = _mm_extract_epi16(xmm1,1);//((short *)&xmm1)[j+1];
    llr2[4] = _mm_extract_epi16(xmm2,0);//((short *)&xmm2)[j];
    llr2[5] = _mm_extract_epi16(xmm2,1);//((short *)&xmm2)[j+1];
#elif defined(__arm__) || defined(__aarch64__)
    llr2[2] = vgetq_lane_s16(xmm1,0);
    llr2[3] = vgetq_lane_s16(xmm1,1);//((short *)&xmm1)[j+1];
    llr2[4] = vgetq_lane_s16(xmm2,0);//((short *)&xmm2)[j];
    llr2[5] = vgetq_lane_s16(xmm2,1);//((short *)&xmm2)[j+1];
#endif

    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[2];
    llr2[1] = ((short *)&rxF[i])[3];
#if defined(__x86_64__) || defined(__i386__)
    llr2[2] = _mm_extract_epi16(xmm1,2);
    llr2[3] = _mm_extract_epi16(xmm1,3);//((short *)&xmm1)[j+1];
    llr2[4] = _mm_extract_epi16(xmm2,2);//((short *)&xmm2)[j];
    llr2[5] = _mm_extract_epi16(xmm2,3);//((short *)&xmm2)[j+1];
#elif defined(__arm__) || defined(__aarch64__)
    llr2[2] = vgetq_lane_s16(xmm1,2);
    llr2[3] = vgetq_lane_s16(xmm1,3);//((short *)&xmm1)[j+1];
    llr2[4] = vgetq_lane_s16(xmm2,2);//((short *)&xmm2)[j];
    llr2[5] = vgetq_lane_s16(xmm2,3);//((short *)&xmm2)[j+1];
#endif

    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[4];
    llr2[1] = ((short *)&rxF[i])[5];
#if defined(__x86_64__) || defined(__i386__)
    llr2[2] = _mm_extract_epi16(xmm1,4);
    llr2[3] = _mm_extract_epi16(xmm1,5);//((short *)&xmm1)[j+1];
    llr2[4] = _mm_extract_epi16(xmm2,4);//((short *)&xmm2)[j];
    llr2[5] = _mm_extract_epi16(xmm2,5);//((short *)&xmm2)[j+1];
#elif defined(__arm__) || defined(__aarch64__)
    llr2[2] = vgetq_lane_s16(xmm1,4);
    llr2[3] = vgetq_lane_s16(xmm1,5);//((short *)&xmm1)[j+1];
    llr2[4] = vgetq_lane_s16(xmm2,4);//((short *)&xmm2)[j];
    llr2[5] = vgetq_lane_s16(xmm2,5);//((short *)&xmm2)[j+1];
#endif
    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[6];
    llr2[1] = ((short *)&rxF[i])[7];
#if defined(__x86_64__) || defined(__i386__)
    llr2[2] = _mm_extract_epi16(xmm1,6);
    llr2[3] = _mm_extract_epi16(xmm1,7);//((short *)&xmm1)[j+1];
    llr2[4] = _mm_extract_epi16(xmm2,6);//((short *)&xmm2)[j];
    llr2[5] = _mm_extract_epi16(xmm2,7);//((short *)&xmm2)[j+1];
#elif defined(__arm__) || defined(__aarch64__)
    llr2[2] = vgetq_lane_s16(xmm1,6);
    llr2[3] = vgetq_lane_s16(xmm1,7);//((short *)&xmm1)[j+1];
    llr2[4] = vgetq_lane_s16(xmm2,6);//((short *)&xmm2)[j];
    llr2[5] = vgetq_lane_s16(xmm2,7);//((short *)&xmm2)[j+1];
#endif
    llr2+=6;

  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
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
  __m128i *rxF = (__m128i*)&rxdataF_comp[(symbol*nb_rb*12)];
  __m128i *ch_mag,*ch_magb,*ch_magr;

  int i,len2;
  unsigned char len_mod4;
  int16_t *llr2;

  llr2 = dlsch_llr;

  ch_mag = (__m128i *)dl_ch_mag;
  ch_magb = (__m128i *)dl_ch_magb;
  ch_magr = (__m128i *)dl_ch_magr;

  len_mod4 =len&3;
  len2=len>>2;  // length in quad words (4 REs)
  len2+=((len_mod4==0)?0:1);

  for (i=0; i<len2; i++) {
    __m128i xmm1 = _mm_abs_epi16(rxF[i]);
    xmm1 = _mm_subs_epi16(ch_mag[i],xmm1);
    __m128i xmm2 = _mm_abs_epi16(xmm1);
    xmm2 = _mm_subs_epi16(ch_magb[i],xmm2);
    __m128i xmm3 = _mm_abs_epi16(xmm2);
    xmm3 = _mm_subs_epi16(ch_magr[i], xmm3);

    llr2[0] = ((short *)&rxF[i])[0];
    llr2[1] = ((short *)&rxF[i])[1];
    llr2[2] = _mm_extract_epi16(xmm1,0);
    llr2[3] = _mm_extract_epi16(xmm1,1);//((short *)&xmm1)[j+1];
    llr2[4] = _mm_extract_epi16(xmm2,0);//((short *)&xmm2)[j];
    llr2[5] = _mm_extract_epi16(xmm2,1);//((short *)&xmm2)[j+1];
    llr2[6] = _mm_extract_epi16(xmm3,0);
    llr2[7] = _mm_extract_epi16(xmm3,1);

    llr2+=8;
    llr2[0] = ((short *)&rxF[i])[2];
    llr2[1] = ((short *)&rxF[i])[3];
    llr2[2] = _mm_extract_epi16(xmm1,2);
    llr2[3] = _mm_extract_epi16(xmm1,3);//((short *)&xmm1)[j+1];
    llr2[4] = _mm_extract_epi16(xmm2,2);//((short *)&xmm2)[j];
    llr2[5] = _mm_extract_epi16(xmm2,3);//((short *)&xmm2)[j+1];
    llr2[6] = _mm_extract_epi16(xmm3,2);
    llr2[7] = _mm_extract_epi16(xmm3,3);

    llr2+=8;
    llr2[0] = ((short *)&rxF[i])[4];
    llr2[1] = ((short *)&rxF[i])[5];
    llr2[2] = _mm_extract_epi16(xmm1,4);
    llr2[3] = _mm_extract_epi16(xmm1,5);//((short *)&xmm1)[j+1];
    llr2[4] = _mm_extract_epi16(xmm2,4);//((short *)&xmm2)[j];
    llr2[5] = _mm_extract_epi16(xmm2,5);//((short *)&xmm2)[j+1];
    llr2[6] = _mm_extract_epi16(xmm3,4);
    llr2[7] = _mm_extract_epi16(xmm3,5);

    llr2+=8;
    llr2[0] = ((short *)&rxF[i])[6];
    llr2[1] = ((short *)&rxF[i])[7];
    llr2[2] = _mm_extract_epi16(xmm1,6);
    llr2[3] = _mm_extract_epi16(xmm1,7);//((short *)&xmm1)[j+1];
    llr2[4] = _mm_extract_epi16(xmm2,6);//((short *)&xmm2)[j];
    llr2[5] = _mm_extract_epi16(xmm2,7);//((short *)&xmm2)[j+1];
    llr2[6] = _mm_extract_epi16(xmm3,6);
    llr2[7] = _mm_extract_epi16(xmm3,7);
    llr2+=8;

  }

  _mm_empty();
  _m_empty();
}

//==============================================================================================
// DUAL-STREAM
//==============================================================================================

//----------------------------------------------------------------------------------------------
// QPSK
//----------------------------------------------------------------------------------------------

#if defined(__x86_64__) || defined(__i386)
__m128i  y0r_over2 __attribute__ ((aligned(16)));
__m128i  y0i_over2 __attribute__ ((aligned(16)));
__m128i  y1r_over2 __attribute__ ((aligned(16)));
__m128i  y1i_over2 __attribute__ ((aligned(16)));

__m128i  A __attribute__ ((aligned(16)));
__m128i  B __attribute__ ((aligned(16)));
__m128i  C __attribute__ ((aligned(16)));
__m128i  D __attribute__ ((aligned(16)));
__m128i  E __attribute__ ((aligned(16)));
__m128i  F __attribute__ ((aligned(16)));
__m128i  G __attribute__ ((aligned(16)));
__m128i  H __attribute__ ((aligned(16)));

#endif

//__m128i ONE_OVER_SQRT_8 __attribute__((aligned(16)));

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

#if defined(__x86_64__) || defined(__i386__)
  __m128i *rho01_128i = (__m128i *)rho01;
  __m128i *stream0_128i_in = (__m128i *)stream0_in;
  __m128i *stream1_128i_in = (__m128i *)stream1_in;
  __m128i *stream0_128i_out = (__m128i *)stream0_out;
  __m128i ONE_OVER_SQRT_8 = _mm_set1_epi16(23170); //round(2^16/sqrt(8))
#elif defined(__arm__) || defined(__aarch64__)
  int16x8_t *rho01_128i = (int16x8_t *)rho01;
  int16x8_t *stream0_128i_in = (int16x8_t *)stream0_in;
  int16x8_t *stream1_128i_in = (int16x8_t *)stream1_in;
  int16x8_t *stream0_128i_out = (int16x8_t *)stream0_out;
  int16x8_t ONE_OVER_SQRT_8 = vdupq_n_s16(23170); //round(2^16/sqrt(8))
#endif

  int i;


  for (i=0; i<length>>2; i+=2) {
    // in each iteration, we take 8 complex samples
#if defined(__x86_64__) || defined(__i386__)
    __m128i xmm0 = rho01_128i[i]; // 4 symbols
    __m128i xmm1 = rho01_128i[i + 1];

    // put (rho_r + rho_i)/2sqrt2 in rho_rpi
    // put (rho_r - rho_i)/2sqrt2 in rho_rmi

    xmm0 = _mm_shufflelo_epi16(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = _mm_shufflehi_epi16(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = _mm_shuffle_epi32(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = _mm_shufflelo_epi16(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = _mm_shufflehi_epi16(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = _mm_shuffle_epi32(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    __m128i xmm2 = _mm_unpacklo_epi64(xmm0, xmm1); // Re(rho)
    __m128i xmm3 = _mm_unpackhi_epi64(xmm0, xmm1); // Im(rho)
    __m128i rho_rpi = _mm_adds_epi16(xmm2, xmm3); // rho = Re(rho) + Im(rho)
    __m128i rho_rmi = _mm_subs_epi16(xmm2, xmm3); // rho* = Re(rho) - Im(rho)

    // divide by sqrt(8), no shift needed ONE_OVER_SQRT_8 = Q1.16
    rho_rpi = _mm_mulhi_epi16(rho_rpi,ONE_OVER_SQRT_8);
    rho_rmi = _mm_mulhi_epi16(rho_rmi,ONE_OVER_SQRT_8);
#elif defined(__arm__) || defined(__aarch64__)


#endif
    // Compute LLR for first bit of stream 0

    // Compute real and imaginary parts of MF output for stream 0
#if defined(__x86_64__) || defined(__i386__)
    xmm0 = stream0_128i_in[i];
    xmm1 = stream0_128i_in[i+1];

    xmm0 = _mm_shufflelo_epi16(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = _mm_shufflehi_epi16(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = _mm_shuffle_epi32(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = _mm_shufflelo_epi16(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = _mm_shufflehi_epi16(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = _mm_shuffle_epi32(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    __m128i y0r = _mm_unpacklo_epi64(xmm0, xmm1); // = [y0r(1),y0r(2),y0r(3),y0r(4)]
    __m128i y0i = _mm_unpackhi_epi64(xmm0, xmm1);

    __m128i y0r_over2 = _mm_srai_epi16(y0r, 1); // divide by 2
    __m128i y0i_over2 = _mm_srai_epi16(y0i, 1); // divide by 2
#elif defined(__arm__) || defined(__aarch64__)


#endif
    // Compute real and imaginary parts of MF output for stream 1
#if defined(__x86_64__) || defined(__i386__)
    xmm0 = stream1_128i_in[i];
    xmm1 = stream1_128i_in[i+1];

    xmm0 = _mm_shufflelo_epi16(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = _mm_shufflehi_epi16(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm0 = _mm_shuffle_epi32(xmm0,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = _mm_shufflelo_epi16(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = _mm_shufflehi_epi16(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
    xmm1 = _mm_shuffle_epi32(xmm1,0xd8); //_MM_SHUFFLE(0,2,1,3));
    //xmm0 = [Re(0,1) Re(2,3) Im(0,1) Im(2,3)]
    //xmm1 = [Re(4,5) Re(6,7) Im(4,5) Im(6,7)]
    __m128i y1r = _mm_unpacklo_epi64(xmm0, xmm1); //[y1r(1),y1r(2),y1r(3),y1r(4)]
    __m128i y1i = _mm_unpackhi_epi64(xmm0, xmm1); //[y1i(1),y1i(2),y1i(3),y1i(4)]

    __m128i y1r_over2 = _mm_srai_epi16(y1r, 1); // divide by 2
    __m128i y1i_over2 = _mm_srai_epi16(y1i, 1); // divide by 2

    // Compute the terms for the LLR of first bit

    xmm0 = _mm_setzero_si128(); // ZERO

    // 1 term for numerator of LLR
    xmm3 = _mm_subs_epi16(y1r_over2,rho_rpi);
    A = _mm_abs_epi16(xmm3); // A = |y1r/2 - rho/sqrt(8)|
    xmm2 = _mm_adds_epi16(A,y0i_over2); // = |y1r/2 - rho/sqrt(8)| + y0i/2
    xmm3 = _mm_subs_epi16(y1i_over2,rho_rmi);
    B = _mm_abs_epi16(xmm3); // B = |y1i/2 - rho*/sqrt(8)|
    __m128i logmax_num_re0 = _mm_adds_epi16(B, xmm2); // = |y1r/2 - rho/sqrt(8)|+|y1i/2 - rho*/sqrt(8)| + y0i/2

    // 2 term for numerator of LLR
    xmm3 = _mm_subs_epi16(y1r_over2,rho_rmi);
    C = _mm_abs_epi16(xmm3); // C = |y1r/2 - rho*/4|
    xmm2 = _mm_subs_epi16(C,y0i_over2); // = |y1r/2 - rho*/4| - y0i/2
    xmm3 = _mm_adds_epi16(y1i_over2,rho_rpi);
    D = _mm_abs_epi16(xmm3); // D = |y1i/2 + rho/4|
    xmm2 = _mm_adds_epi16(xmm2,D); // |y1r/2 - rho*/4| + |y1i/2 + rho/4| - y0i/2
    logmax_num_re0 = _mm_max_epi16(logmax_num_re0,xmm2); // max, numerator done

    // 1 term for denominator of LLR
    xmm3 = _mm_adds_epi16(y1r_over2,rho_rmi);
    E = _mm_abs_epi16(xmm3); // E = |y1r/2 + rho*/4|
    xmm2 = _mm_adds_epi16(E,y0i_over2); // = |y1r/2 + rho*/4| + y0i/2
    xmm3 = _mm_subs_epi16(y1i_over2,rho_rpi);
    F = _mm_abs_epi16(xmm3); // F = |y1i/2 - rho/4|
    __m128i logmax_den_re0 = _mm_adds_epi16(F, xmm2); // = |y1r/2 + rho*/4| + |y1i/2 - rho/4| + y0i/2

    // 2 term for denominator of LLR
    xmm3 = _mm_adds_epi16(y1r_over2,rho_rpi);
    G = _mm_abs_epi16(xmm3); // G = |y1r/2 + rho/4|
    xmm2 = _mm_subs_epi16(G,y0i_over2); // = |y1r/2 + rho/4| - y0i/2
    xmm3 = _mm_adds_epi16(y1i_over2,rho_rmi);
    H = _mm_abs_epi16(xmm3); // H = |y1i/2 + rho*/4|
    xmm2 = _mm_adds_epi16(xmm2,H); // = |y1r/2 + rho/4| + |y1i/2 + rho*/4| - y0i/2
    logmax_den_re0 = _mm_max_epi16(logmax_den_re0,xmm2); // max, denominator done

    // Compute the terms for the LLR of first bit

    // 1 term for nominator of LLR
    xmm2 = _mm_adds_epi16(A,y0r_over2);
    __m128i logmax_num_im0 = _mm_adds_epi16(B, xmm2); // = |y1r/2 - rho/4| + |y1i/2 - rho*/4| + y0r/2

    // 2 term for nominator of LLR
    xmm2 = _mm_subs_epi16(E,y0r_over2);
    xmm2 = _mm_adds_epi16(xmm2,F); // = |y1r/2 + rho*/4| + |y1i/2 - rho/4| - y0r/2

    logmax_num_im0 = _mm_max_epi16(logmax_num_im0,xmm2); // max, nominator done

    // 1 term for denominator of LLR
    xmm2 = _mm_adds_epi16(C,y0r_over2);
    __m128i logmax_den_im0 = _mm_adds_epi16(D, xmm2); // = |y1r/2 - rho*/4| + |y1i/2 + rho/4| - y0r/2

    xmm2 = _mm_subs_epi16(G,y0r_over2);
    xmm2 = _mm_adds_epi16(xmm2,H); // = |y1r/2 + rho/4| + |y1i/2 + rho*/4| - y0r/2

    logmax_den_im0 = _mm_max_epi16(logmax_den_im0,xmm2); // max, denominator done

    // LLR of first bit [L1(1), L1(2), L1(3), L1(4)]
    y0r = _mm_adds_epi16(y0r,logmax_num_re0);
    y0r = _mm_subs_epi16(y0r,logmax_den_re0);

    // LLR of second bit [L2(1), L2(2), L2(3), L2(4)]
    y0i = _mm_adds_epi16(y0i,logmax_num_im0);
    y0i = _mm_subs_epi16(y0i,logmax_den_im0);

    _mm_storeu_si128(&stream0_128i_out[i],_mm_unpacklo_epi16(y0r,y0i)); // = [L1(1), L2(1), L1(2), L2(2)]

    if (i<((length>>1) - 1)) // false if only 2 REs remain
      _mm_storeu_si128(&stream0_128i_out[i+1],_mm_unpackhi_epi16(y0r,y0i));

#elif defined(__x86_64__)

#endif
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}
