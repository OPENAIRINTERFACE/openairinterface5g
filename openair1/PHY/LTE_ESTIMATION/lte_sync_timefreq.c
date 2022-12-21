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

/*! \file PHY/LTE_ESTIMATION/lte_sync_timefreq.c
* \brief Initial time frequency scan of entire LTE band
* \author R. Knopp
* \date 2014
* \version 0.1
* \company Eurecom
* \email: raymond.knopp@eurecom.fr
* \note
* \warning
*/
/* file: lte_sync_timefreq.c
   purpose: scan for likely cells over entire LTE band using PSS. Provides coarse frequency offset in addtition to 10 top likelihoods per PSS sequence
   author: raymond.knopp@eurecom.fr
   date: 23.01.2015
*/

//#include "defs.h"
#include "PHY/defs_UE.h"
#include "PHY/phy_extern_ue.h"
#include "PHY/phy_extern.h"

#include "pss6144.h"
extern void print_shorts(char *, simde__m128i *);

void lte_sync_timefreq(PHY_VARS_UE *ue,int band,unsigned int DL_freq)
{
  UE_SCAN_INFO_t *scan_info = &ue->scan_info[band];
  int16_t spectrum[12288] __attribute__((aligned(32)));
  int16_t spectrum_p5ms[12288] __attribute__((aligned(32)));
  int i,f,band_idx;
  simde__m128i autocorr0[256 / 4], autocorr1[256 / 4], autocorr2[256 / 4];
  simde__m128i autocorr0_t[256 / 4], autocorr1_t[256 / 4], autocorr2_t[256 / 4];
  simde__m128i tmp_t[256 / 4];
  int32_t *rxp;
  int16_t *sp;
  simde__m128i *sp2 = NULL;
  simde__m128i s;
  int re,re256;
  simde__m128i mmtmp00, mmtmp01, mmtmp02, mmtmp10, mmtmp11, mmtmp12;
  int maxcorr[3],minamp,pos=0,pssind;
  int16_t *pss6144_0 = NULL, *pss6144_1 = NULL, *pss6144_2 = NULL;

  /*  char fname[100],vname[100];*/


  for (i=0; i<38400*4; i+=3072) { // steps of 200 us with 100 us overlap, 0 to 5s
    //  LOG_M("rxsig0.m","rxs0",ue->lte_ue_common_vars.rxdata[0],30720,1,1);

    //for (i = 15360-3072*2; i<15360+3072+1; i+=3072)  {


    //compute frequency-domain representation of 6144-sample chunk

    rxp = &ue->common_vars.rxdata[0][i];
    sp=spectrum;

    while (1) {

      //compute frequency-domain representation of 6144-sample chunk
      dft(DFT_6144,(int16_t *)rxp,
              sp,1);


      /*
      printf("i %d: sp %p\n",i,sp);
      if (i==12288) {
        LOG_M("scan6144F.m","s6144F",sp,6144,1,1);
        LOG_M("scan6144.m","s6144",rxp,6144,1,1);
      LOG_M("pss0_6144.m","pss0",pss6144_0_0,256,1,1);
      }*/

      for (f = -2000; f<2000; f++) { // this is -10MHz to 10 MHz in 5 kHz steps

        if ((f<-256)||(f>=0)) { // no split around DC
          //          printf("No split, f %d (%d)\n",f,f&3);

          // align filters and input buffer pointer to 128-bit
          switch (f&3) {
          case 0:
            pss6144_0 = &pss6144_0_0[0];
            pss6144_1 = &pss6144_1_0[0];
            pss6144_2 = &pss6144_2_0[0];
            sp2 = (f < 0) ? (simde__m128i *)&sp[12288 + (f << 1)] : (simde__m128i *)&sp[(f << 1)];
            break;

          case 1:
            pss6144_0 = &pss6144_0_1[0];
            pss6144_1 = &pss6144_1_1[0];
            pss6144_2 = &pss6144_2_1[0];
            sp2 = (f < 0) ? (simde__m128i *)&sp[12286 + (f << 1)] : (simde__m128i *)&sp[-2 + (f << 1)];
            break;

          case 2:
            pss6144_0 = &pss6144_0_2[0];
            pss6144_1 = &pss6144_1_2[0];
            pss6144_2 = &pss6144_2_2[0];
            sp2 = (f < 0) ? (simde__m128i *)&sp[12284 + (f << 1)] : (simde__m128i *)&sp[-4 + (f << 1)];
            break;

          case 3:
            pss6144_0 = &pss6144_0_3[0];
            pss6144_1 = &pss6144_1_3[0];
            pss6144_2 = &pss6144_2_3[0];
            sp2 = (f < 0) ? (simde__m128i *)&sp[12282 + (f << 1)] : (simde__m128i *)&sp[-6 + (f << 1)];
            break;
          }

          re256=32;

          for (re = 0; re<256/4; re++) {  // loop over 256 points of upsampled PSS
            //      printf("f %d, re %d\n",f,re);
            s = sp2[re];
            mmtmp00 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_0)[re], s), 15);
            mmtmp01 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_1)[re], s), 15);
            mmtmp02 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_2)[re], s), 15);

            s = simde_mm_shufflelo_epi16(s, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
            s = simde_mm_shufflehi_epi16(s, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
            s = simde_mm_sign_epi16(s, *(simde__m128i *)&conjugate[0]);
            mmtmp10 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_0)[re], s), 15);
            mmtmp11 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_1)[re], s), 15);
            mmtmp12 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_2)[re], s), 15);

            autocorr0[re256] =
                simde_mm_packs_epi32(simde_mm_unpacklo_epi32(mmtmp00, mmtmp10), simde_mm_unpackhi_epi32(mmtmp00, mmtmp10));
            autocorr1[re256] =
                simde_mm_packs_epi32(simde_mm_unpacklo_epi32(mmtmp01, mmtmp11), simde_mm_unpackhi_epi32(mmtmp01, mmtmp11));
            autocorr2[re256] =
                simde_mm_packs_epi32(simde_mm_unpacklo_epi32(mmtmp02, mmtmp12), simde_mm_unpackhi_epi32(mmtmp02, mmtmp12));

            re256 = (re256+1)&0x3f;
          }
        } else { // Split around DC, this is the negative frequencies
          //          printf("split around DC, f %d (f/4 %d, f&3 %d)\n",f,f>>2,f&3);

          // align filters and input buffer pointer to 128-bit
          switch (f&3) {
          case 0:
            pss6144_0 = &pss6144_0_0[0];
            pss6144_1 = &pss6144_1_0[0];
            pss6144_2 = &pss6144_2_0[0];
            sp2 = (simde__m128i *)&sp[12288 + (f << 1)];
            break;

          case 1:
            pss6144_0 = &pss6144_0_1[0];
            pss6144_1 = &pss6144_1_1[0];
            pss6144_2 = &pss6144_2_1[0];
            sp2 = (simde__m128i *)&sp[12286 + (f << 1)];
            break;

          case 2:
            pss6144_0 = &pss6144_0_2[0];
            pss6144_1 = &pss6144_1_2[0];
            pss6144_2 = &pss6144_2_2[0];
            sp2 = (simde__m128i *)&sp[12284 + (f << 1)];
            break;

          case 3:
            pss6144_0 = &pss6144_0_3[0];
            pss6144_1 = &pss6144_1_3[0];
            pss6144_2 = &pss6144_2_3[0];
            sp2 = (simde__m128i *)&sp[12282 + (f << 1)];
            break;
          }

          re256 = 32;

          for (re = 0; re<(-f+3)/4; re++) {  // loop over 256 points of upsampled PSS
            s = sp2[re];
            /*            printf("re %d, %p\n",re,&sp2[re]);
                  print_shorts("s",&s);
                  print_shorts("pss",&((simde__m128i*)pss6144_0)[re]);*/

            mmtmp00 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_0)[re], s), 15);
            mmtmp01 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_1)[re], s), 15);
            mmtmp02 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_2)[re], s), 15);

            s = simde_mm_shufflelo_epi16(s, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
            s = simde_mm_shufflehi_epi16(s, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
            s = simde_mm_sign_epi16(s, *(simde__m128i *)&conjugate[0]);
            mmtmp10 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_0)[re], s), 15);
            mmtmp11 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_1)[re], s), 15);
            mmtmp12 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_2)[re], s), 15);

            autocorr0[re256] =
                simde_mm_packs_epi32(simde_mm_unpacklo_epi32(mmtmp00, mmtmp10), simde_mm_unpackhi_epi32(mmtmp00, mmtmp10));
            autocorr1[re256] =
                simde_mm_packs_epi32(simde_mm_unpacklo_epi32(mmtmp01, mmtmp11), simde_mm_unpackhi_epi32(mmtmp01, mmtmp11));
            autocorr2[re256] =
                simde_mm_packs_epi32(simde_mm_unpacklo_epi32(mmtmp02, mmtmp12), simde_mm_unpackhi_epi32(mmtmp02, mmtmp12));

            re256 = (re256+1)&0x3f;
          }

          // This is the +ve frequencies

          // align filters to 128-bit
          sp2 = (simde__m128i *)&sp[0];

          switch (f&3) {
          case 0:
            pss6144_0 = &pss6144_0_0[256];
            pss6144_1 = &pss6144_1_0[256];
            pss6144_2 = &pss6144_2_0[256];
            break;

          case 1:
            pss6144_0 = &pss6144_0_1[256];
            pss6144_1 = &pss6144_1_1[256];
            pss6144_2 = &pss6144_2_1[256];
            break;

          case 2:
            pss6144_0 = &pss6144_0_2[256];
            pss6144_1 = &pss6144_1_2[256];
            pss6144_2 = &pss6144_2_2[256];
            break;

          case 3:
            pss6144_0 = &pss6144_0_3[256];
            pss6144_1 = &pss6144_1_3[256];
            pss6144_2 = &pss6144_2_3[256];
            break;
          }

          for (re = 0; re<(256+f)/4; re++) {  // loop over 256 points of upsampled PSS
            s = sp2[re];
            /*            printf("re %d %p\n",re,&sp2[re]);
                  print_shorts("s",&s);
                  print_shorts("pss",&((simde__m128i*)pss6144_0)[re]);*/
            mmtmp00 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_0)[re], s), 15);
            mmtmp01 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_1)[re], s), 15);
            mmtmp02 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_2)[re], s), 15);

            s = simde_mm_shufflelo_epi16(s, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
            s = simde_mm_shufflehi_epi16(s, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
            s = simde_mm_sign_epi16(s, *(simde__m128i *)&conjugate[0]);
            mmtmp10 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_0)[re], s), 15);
            mmtmp11 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_1)[re], s), 15);
            mmtmp12 = simde_mm_srai_epi32(simde_mm_madd_epi16(((simde__m128i *)pss6144_2)[re], s), 15);

            autocorr0[re256] =
                simde_mm_packs_epi32(simde_mm_unpacklo_epi32(mmtmp00, mmtmp10), simde_mm_unpackhi_epi32(mmtmp00, mmtmp10));
            autocorr1[re256] =
                simde_mm_packs_epi32(simde_mm_unpacklo_epi32(mmtmp01, mmtmp11), simde_mm_unpackhi_epi32(mmtmp01, mmtmp11));
            autocorr2[re256] =
                simde_mm_packs_epi32(simde_mm_unpacklo_epi32(mmtmp02, mmtmp12), simde_mm_unpackhi_epi32(mmtmp02, mmtmp12));

            re256 = (re256+1)&0x3f;
          }

        }

        // ifft, accumulate energy over two half-frames
        idft(IDFT_256,(int16_t*)autocorr0,(int16_t*)tmp_t,1);
        /*
              if (i==12288) {
          sprintf(fname,"corr256F_%d.m",abs(f));
          sprintf(vname,"c256F_%d",abs(f));
                LOG_M(fname,vname,autocorr0,256,1,1);
          sprintf(fname,"corr256_%d.m",abs(f));
          sprintf(vname,"c256_%d",abs(f));
                LOG_M(fname,vname,tmp_t,256,1,1);
          }*/

        memset((void*)autocorr0_t,0,256*4);
        memset((void*)autocorr1_t,0,256*4);
        memset((void*)autocorr2_t,0,256*4);

        for (re=0; re<(256/4); re++)
          autocorr0_t[re] = simde_mm_add_epi32(autocorr0_t[re], simde_mm_madd_epi16(tmp_t[re], tmp_t[re]));

        idft(IDFT_256,(int16_t*)autocorr1,(int16_t*)tmp_t,1);

        for (re=0; re<(256/4); re++)
          autocorr1_t[re] = simde_mm_add_epi32(autocorr1_t[re], simde_mm_madd_epi16(tmp_t[re], tmp_t[re]));

        idft(IDFT_256,(int16_t*)autocorr2,(int16_t*)tmp_t,1);

        for (re=0; re<(256/4); re++)
          autocorr2_t[re] = simde_mm_add_epi32(autocorr2_t[re], simde_mm_madd_epi16(tmp_t[re], tmp_t[re]));

        //compute max correlation over time window
        maxcorr[0] = 0;
        maxcorr[1] = 0;
        maxcorr[2] = 0;

        for (re=0; re<256; re++) {

          if (((int32_t*)autocorr0_t)[re] > maxcorr[0]) {
            maxcorr[0]=((int32_t*)autocorr0_t)[re];
          }

          if (((int32_t*)autocorr1_t)[re] > maxcorr[1])
            maxcorr[1]=((int32_t*)autocorr1_t)[re];

          if (((int32_t*)autocorr2_t)[re] > maxcorr[2])
            maxcorr[2]=((int32_t*)autocorr2_t)[re];
        }


        for (pssind=0; pssind<3; pssind++) {

          minamp=(int)((1<<30)-1);

          for (band_idx=0; band_idx<10; band_idx++)
            if (minamp > scan_info->amp[pssind][band_idx]) {
              minamp = scan_info->amp[pssind][band_idx];
              pos    = band_idx;
            }

          if (maxcorr[pssind]>minamp) {
            scan_info->amp[pssind][pos]=maxcorr[pssind];
            scan_info->freq_offset_Hz[pssind][pos]=((f+128)*5000)+DL_freq;
            printf("pss %d, amp %d (%d>%d) freq %u (%d), i %d\n",pssind,dB_fixed(maxcorr[pssind]),maxcorr[pssind],minamp,((f+128)*5000)+DL_freq,f,i);
          }
        } // loop on pss index
      }

      if (rxp == &ue->common_vars.rxdata[0][i+38400*4]) {
        rxp = &ue->common_vars.rxdata[0][i+38400*4];
        sp=spectrum_p5ms;
      } else {
        break;
      }
    }
  }// loop on time index i

  for (band_idx=0; band_idx<10; band_idx++)
    printf("pss 0: level %d dB, freq %u\n", dB_fixed(scan_info->amp[0][band_idx]),scan_info->freq_offset_Hz[0][band_idx]);

  for (band_idx=0; band_idx<10; band_idx++)
    printf("pss 1: level %d dB, freq %u\n", dB_fixed(scan_info->amp[1][band_idx]),scan_info->freq_offset_Hz[1][band_idx]);

  for (band_idx=0; band_idx<10; band_idx++)
    printf("pss 2: level %d dB, freq %u\n", dB_fixed(scan_info->amp[2][band_idx]),scan_info->freq_offset_Hz[2][band_idx]);
}
