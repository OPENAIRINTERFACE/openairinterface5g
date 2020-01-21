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

/*! \file PHY/NR_TRANSPORT/nr_ulsch_llr_computation.c
 * \brief Top-level routines for LLR computation of the PDSCH physical channel
 * \author Ahmed Hussein
 * \date 2019
 * \version 0.1
 * \company Fraunhofer IIS
 * \email: ahmed.hussein@iis.fraunhofer.de
 * \note
 * \warning
 */

#include "PHY/defs_nr_common.h"
#include "PHY/sse_intrin.h"
#include "PHY/impl_defs_top.h"

__m128i  xmm0 __attribute__ ((aligned(32)));
__m128i  xmm1 __attribute__ ((aligned(32)));
__m128i  xmm2 __attribute__ ((aligned(32)));


//----------------------------------------------------------------------------------------------
// QPSK
//----------------------------------------------------------------------------------------------
void nr_ulsch_qpsk_llr(int32_t *rxdataF_comp,
                      int16_t  *ulsch_llr,
                      uint32_t nb_re,
                      uint8_t  symbol)
{
  int i;

  uint32_t *rxF   = (uint32_t*)rxdataF_comp;
  uint32_t *llr32 = (uint32_t*)ulsch_llr;

  if (!llr32) {
    LOG_E(PHY,"nr_ulsch_qpsk_llr: llr is null, symbol %d, llr32 = %p\n",symbol, llr32);
  }

  for (i = 0; i < nb_re; i++) {
    *llr32 = *rxF;
    rxF++;
    llr32++;
  }
}

//----------------------------------------------------------------------------------------------
// 16-QAM
//----------------------------------------------------------------------------------------------

void nr_ulsch_16qam_llr(int32_t *rxdataF_comp,
                        int32_t **ul_ch_mag,
                        int16_t  *ulsch_llr,
                        uint32_t nb_rb,
                        uint32_t nb_re,
                        uint8_t  symbol)
{

#if defined(__x86_64__) || defined(__i386__)
  __m128i *rxF = (__m128i*)rxdataF_comp;
  __m128i *ch_mag;
  __m128i llr128[2];
  uint32_t *llr32;

#elif defined(__arm__)
  int16x8_t *rxF = (int16x8_t*)&rxdataF_comp;
  int16x8_t *ch_mag;
  int16x8_t xmm0;
  int16_t *llr16;
#endif


  int i;
  unsigned char len_mod4 = 0;


#if defined(__x86_64__) || defined(__i386__)
    llr32 = (uint32_t*)ulsch_llr;
#elif defined(__arm__)
    llr16 = (int16_t*)ulsch_llr;
#endif

#if defined(__x86_64__) || defined(__i386__)
  ch_mag = (__m128i*)&ul_ch_mag[0][(symbol*nb_rb*12)];
#elif defined(__arm__)
  ch_mag = (int16x8_t*)&ul_ch_mag[0][(symbol*nb_rb*12)];
#endif

  len_mod4 = nb_re&3;
  nb_re >>= 2;  // length in quad words (4 REs)
  nb_re += (len_mod4 == 0 ? 0 : 1);

  for (i=0; i<nb_re; i++) {

#if defined(__x86_64__) || defined(__i386)

    xmm0 = _mm_abs_epi16(rxF[i]); // registers of even index in xmm0-> |y_R|, registers of odd index in xmm0-> |y_I|
    
    
    xmm0 = _mm_subs_epi16(ch_mag[i],xmm0); // registers of even index in xmm0-> |y_R|-|h|^2, registers of odd index in xmm0-> |y_I|-|h|^2

    llr128[0] = _mm_unpacklo_epi32(rxF[i],xmm0); // llr128[0] contains the llrs of the 1st and 2nd REs
    llr128[1] = _mm_unpackhi_epi32(rxF[i],xmm0); // llr128[1] contains the llrs of the 3rd and 4th REs
    
    // 1st RE
    llr32[0] = _mm_extract_epi32(llr128[0],0); // llr32[0] low 16 bits-> y_R        , high 16 bits-> y_I
    llr32[1] = _mm_extract_epi32(llr128[0],1); // llr32[1] low 16 bits-> |h|-|y_R|^2, high 16 bits-> |h|-|y_I|^2

    // 2nd RE
    llr32[2] = _mm_extract_epi32(llr128[0],2); // llr32[2] low 16 bits-> y_R        , high 16 bits-> y_I
    llr32[3] = _mm_extract_epi32(llr128[0],3); // llr32[3] low 16 bits-> |h|-|y_R|^2, high 16 bits-> |h|-|y_I|^2

    // 3rd RE
    llr32[4] = _mm_extract_epi32(llr128[1],0); // llr32[4] low 16 bits-> y_R        , high 16 bits-> y_I
    llr32[5] = _mm_extract_epi32(llr128[1],1); // llr32[5] low 16 bits-> |h|-|y_R|^2, high 16 bits-> |h|-|y_I|^2

    // 4th RE
    llr32[6] = _mm_extract_epi32(llr128[1],2); // llr32[6] low 16 bits-> y_R        , high 16 bits-> y_I
    llr32[7] = _mm_extract_epi32(llr128[1],3); // llr32[7] low 16 bits-> |h|-|y_R|^2, high 16 bits-> |h|-|y_I|^2

    llr32+=8;
#elif defined(__arm__)
    xmm0 = vabsq_s16(rxF[i]);
    xmm0 = vqsubq_s16((*(__m128i*)&ones[0]),xmm0);

    llr16[0]  = vgetq_lane_s16(rxF[i],0);
    llr16[1]  = vgetq_lane_s16(rxF[i],1);
    llr16[2]  = vgetq_lane_s16(xmm0,0);
    llr16[3]  = vgetq_lane_s16(xmm0,1);
    llr16[4]  = vgetq_lane_s16(rxF[i],2);
    llr16[5]  = vgetq_lane_s16(rxF[i],3);
    llr16[6]  = vgetq_lane_s16(xmm0,2);
    llr16[7]  = vgetq_lane_s16(xmm0,3);
    llr16[8]  = vgetq_lane_s16(rxF[i],4);
    llr16[9]  = vgetq_lane_s16(rxF[i],5);
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

void nr_ulsch_64qam_llr(int32_t *rxdataF_comp,
                        int32_t **ul_ch_mag,
                        int32_t **ul_ch_magb,
                        int16_t  *ulsch_llr,
                        uint32_t nb_rb,
                        uint32_t nb_re,
                        uint8_t  symbol)
{

#if defined(__x86_64__) || defined(__i386__)
  __m128i *rxF = (__m128i*)rxdataF_comp;
  __m128i *ch_mag,*ch_magb;
#elif defined(__arm__)
  int16x8_t *rxF = (int16x8_t*)&rxdataF_comp;
  int16x8_t *ch_mag,*ch_magb; // [hna] This should be uncommented once channel estimation is implemented
  int16x8_t xmm1,xmm2;
#endif

  int i;
  unsigned char len_mod4;

#if defined(__x86_64__) || defined(__i386__)
  ch_mag = (__m128i*)&ul_ch_mag[0][(symbol*nb_rb*12)];
  ch_magb = (__m128i*)&ul_ch_magb[0][(symbol*nb_rb*12)];
#elif defined(__arm__)
  ch_mag = (int16x8_t*)&ul_ch_mag[0][(symbol*nb_rb*12)];
  ch_magb = (int16x8_t*)&ul_ch_magb[0][(symbol*nb_rb*12)];
#endif

  len_mod4 = nb_re&3;
  nb_re    = nb_re>>2;  // length in quad words (4 REs)
  nb_re   += ((len_mod4 == 0) ? 0 : 1);


  for (i=0; i<nb_re; i++) {

#if defined(__x86_64__) || defined(__i386__)
    xmm1 = _mm_abs_epi16(rxF[i]);
    xmm1 = _mm_subs_epi16(ch_mag[i],xmm1);
    xmm2 = _mm_abs_epi16(xmm1);
    xmm2 = _mm_subs_epi16(ch_magb[i],xmm2);
#elif defined(__arm__)
    xmm1 = vabsq_s16(rxF[i]);
    xmm1 = vsubq_s16(ch_mag[i],xmm1);
    xmm2 = vabsq_s16(xmm1);
    xmm2 = vsubq_s16(ch_magb[i],xmm2);
#endif
    
    // ---------------------------------------
    // 1st RE
    // ---------------------------------------
    ulsch_llr[0] = ((short *)&rxF[i])[0];
    ulsch_llr[1] = ((short *)&rxF[i])[1];
#if defined(__x86_64__) || defined(__i386__)
    ulsch_llr[2] = _mm_extract_epi16(xmm1,0);
    ulsch_llr[3] = _mm_extract_epi16(xmm1,1);
    ulsch_llr[4] = _mm_extract_epi16(xmm2,0);
    ulsch_llr[5] = _mm_extract_epi16(xmm2,1);
#elif defined(__arm__)
    ulsch_llr[2] = vgetq_lane_s16(xmm1,0);
    ulsch_llr[3] = vgetq_lane_s16(xmm1,1);
    ulsch_llr[4] = vgetq_lane_s16(xmm2,0);
    ulsch_llr[5] = vgetq_lane_s16(xmm2,1);
#endif
    // ---------------------------------------

    ulsch_llr+=6;
    
    // ---------------------------------------
    // 2nd RE
    // ---------------------------------------
    ulsch_llr[0] = ((short *)&rxF[i])[2];
    ulsch_llr[1] = ((short *)&rxF[i])[3];
#if defined(__x86_64__) || defined(__i386__)
    ulsch_llr[2] = _mm_extract_epi16(xmm1,2);
    ulsch_llr[3] = _mm_extract_epi16(xmm1,3);
    ulsch_llr[4] = _mm_extract_epi16(xmm2,2);
    ulsch_llr[5] = _mm_extract_epi16(xmm2,3);
#elif defined(__arm__)
    ulsch_llr[2] = vgetq_lane_s16(xmm1,2);
    ulsch_llr[3] = vgetq_lane_s16(xmm1,3);
    ulsch_llr[4] = vgetq_lane_s16(xmm2,2);
    ulsch_llr[5] = vgetq_lane_s16(xmm2,3);
#endif
    // ---------------------------------------

    ulsch_llr+=6;
    
    // ---------------------------------------
    // 3rd RE
    // ---------------------------------------
    ulsch_llr[0] = ((short *)&rxF[i])[4];
    ulsch_llr[1] = ((short *)&rxF[i])[5];
#if defined(__x86_64__) || defined(__i386__)
    ulsch_llr[2] = _mm_extract_epi16(xmm1,4);
    ulsch_llr[3] = _mm_extract_epi16(xmm1,5);
    ulsch_llr[4] = _mm_extract_epi16(xmm2,4);
    ulsch_llr[5] = _mm_extract_epi16(xmm2,5);
#elif defined(__arm__)
    ulsch_llr[2] = vgetq_lane_s16(xmm1,4);
    ulsch_llr[3] = vgetq_lane_s16(xmm1,5);
    ulsch_llr[4] = vgetq_lane_s16(xmm2,4);
    ulsch_llr[5] = vgetq_lane_s16(xmm2,5);
#endif
    // ---------------------------------------

    ulsch_llr+=6;
    
    // ---------------------------------------
    // 4th RE
    // ---------------------------------------
    ulsch_llr[0] = ((short *)&rxF[i])[6];
    ulsch_llr[1] = ((short *)&rxF[i])[7];
#if defined(__x86_64__) || defined(__i386__)
    ulsch_llr[2] = _mm_extract_epi16(xmm1,6);
    ulsch_llr[3] = _mm_extract_epi16(xmm1,7);
    ulsch_llr[4] = _mm_extract_epi16(xmm2,6);
    ulsch_llr[5] = _mm_extract_epi16(xmm2,7);
#elif defined(__arm__)
    ulsch_llr[2] = vgetq_lane_s16(xmm1,6);
    ulsch_llr[3] = vgetq_lane_s16(xmm1,7);
    ulsch_llr[4] = vgetq_lane_s16(xmm2,6);
    ulsch_llr[5] = vgetq_lane_s16(xmm2,7);
#endif
    // ---------------------------------------

    ulsch_llr+=6;
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}


void nr_ulsch_compute_llr(int32_t *rxdataF_comp,
                          int32_t **ul_ch_mag,
                          int32_t **ul_ch_magb,
                          int16_t *ulsch_llr,
                          uint32_t nb_rb,
                          uint32_t nb_re,
                          uint8_t  symbol,
                          uint8_t  mod_order)
{
  switch(mod_order){
    case 2:
      nr_ulsch_qpsk_llr(rxdataF_comp,
                        ulsch_llr,
                        nb_re,
                        symbol);
      break;
    case 4:
      nr_ulsch_16qam_llr(rxdataF_comp,
                         ul_ch_mag,
                         ulsch_llr,
                         nb_rb,
                         nb_re,
                         symbol);
      break;
    case 6:
    nr_ulsch_64qam_llr(rxdataF_comp,
                       ul_ch_mag,
                       ul_ch_magb,
                       ulsch_llr,
                       nb_rb,
                       nb_re,
                       symbol);
      break;
    default:
      LOG_E(PHY,"nr_ulsch_compute_llr: invalid Qm value, symbol = %d, Qm = %d\n",symbol, mod_order);
      break;
  }
}
