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

#include "PHY/defs_UE.h"
#include "PHY/phy_extern_ue.h"
#include "modulation_extern.h"
#include <math.h>
#include "PHY/sse_intrin.h"

extern short conjugate75[8];
extern short conjugate75_2[8];
extern short negate[8];

void apply_7_5_kHz(PHY_VARS_UE *ue,int32_t*txdata,uint8_t slot)
{


  uint16_t len;
  uint32_t *kHz7_5ptr;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *txptr128,*kHz7_5ptr128,mmtmp_re,mmtmp_im,mmtmp_re2,mmtmp_im2;
#elif defined(__arm__)
  int16x8_t *txptr128,*kHz7_5ptr128;
  int32x4_t mmtmp_re,mmtmp_im;
  int32x4_t mmtmp0,mmtmp1;
#endif
  uint32_t slot_offset;
  //   uint8_t aa;
  uint32_t i;
  LTE_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;

  switch (frame_parms->N_RB_UL) {

  case 6:
    kHz7_5ptr = (frame_parms->Ncp==0) ? (uint32_t*)s6n_kHz_7_5 : (uint32_t*)s6e_kHz_7_5;
    break;

  case 15:
    kHz7_5ptr = (frame_parms->Ncp==0) ? (uint32_t*)s15n_kHz_7_5 : (uint32_t*)s15e_kHz_7_5;
    break;

  case 25:
    kHz7_5ptr = (frame_parms->Ncp==0) ? (uint32_t*)s25n_kHz_7_5 : (uint32_t*)s25e_kHz_7_5;
    break;

  case 50:
    kHz7_5ptr = (frame_parms->Ncp==0) ? (uint32_t*)s50n_kHz_7_5 : (uint32_t*)s50e_kHz_7_5;
    break;

  case 75:
    kHz7_5ptr = (frame_parms->Ncp==0) ? (uint32_t*)s75n_kHz_7_5 : (uint32_t*)s75e_kHz_7_5;
    break;

  case 100:
    kHz7_5ptr = (frame_parms->Ncp==0) ? (uint32_t*)s100n_kHz_7_5 : (uint32_t*)s100e_kHz_7_5;
    break;

  default:
    kHz7_5ptr = (frame_parms->Ncp==0) ? (uint32_t*)s25n_kHz_7_5 : (uint32_t*)s25e_kHz_7_5;
    break;
  }

  slot_offset = (uint32_t)slot * frame_parms->samples_per_tti/2;
  len = frame_parms->samples_per_tti/2;

#if defined(__x86_64__) || defined(__i386__)
  txptr128 = (__m128i *)&txdata[slot_offset];
  kHz7_5ptr128 = (__m128i *)kHz7_5ptr;
#elif defined(__arm__)
  txptr128 = (int16x8_t*)&txdata[slot_offset];
  kHz7_5ptr128 = (int16x8_t*)kHz7_5ptr;
#endif
  // apply 7.5 kHz

  for (i=0; i<(len>>2); i++) {
#if defined(__x86_64__) || defined(__i386__)
    mmtmp_re = _mm_madd_epi16(*txptr128,*kHz7_5ptr128);
    // Real part of complex multiplication (note: 7_5kHz signal is conjugated for this to work)
    mmtmp_im = _mm_shufflelo_epi16(*kHz7_5ptr128,_MM_SHUFFLE(2,3,0,1));
    mmtmp_im = _mm_shufflehi_epi16(mmtmp_im,_MM_SHUFFLE(2,3,0,1));
    mmtmp_im = _mm_sign_epi16(mmtmp_im,*(__m128i*)&conjugate75[0]);
    mmtmp_im = _mm_madd_epi16(mmtmp_im,txptr128[0]);
    mmtmp_re = _mm_srai_epi32(mmtmp_re,15);
    mmtmp_im = _mm_srai_epi32(mmtmp_im,15);
    mmtmp_re2 = _mm_unpacklo_epi32(mmtmp_re,mmtmp_im);
    mmtmp_im2 = _mm_unpackhi_epi32(mmtmp_re,mmtmp_im);

    txptr128[0] = _mm_packs_epi32(mmtmp_re2,mmtmp_im2);
    txptr128++;
    kHz7_5ptr128++;  
#elif defined(__arm__)

    mmtmp0 = vmull_s16(((int16x4_t*)txptr128)[0],((int16x4_t*)kHz7_5ptr128)[0]);
        //mmtmp0 = [Re(ch[0])Re(rx[0]) Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1]) Im(ch[1])Im(ch[1])] 
    mmtmp1 = vmull_s16(((int16x4_t*)txptr128)[1],((int16x4_t*)kHz7_5ptr128)[1]);
        //mmtmp1 = [Re(ch[2])Re(rx[2]) Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3]) Im(ch[3])Im(ch[3])] 
    mmtmp_re = vcombine_s32(vpadd_s32(vget_low_s32(mmtmp0),vget_high_s32(mmtmp0)),
                            vpadd_s32(vget_low_s32(mmtmp1),vget_high_s32(mmtmp1)));
        //mmtmp_re = [Re(ch[0])Re(rx[0])+Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1])+Im(ch[1])Im(ch[1]) Re(ch[2])Re(rx[2])+Im(ch[2])Im(ch[2]) Re(ch[3])Re(rx[3])+Im(ch[3])Im(ch[3])] 

    mmtmp0 = vmull_s16(vrev32_s16(vmul_s16(((int16x4_t*)txptr128)[0],*(int16x4_t*)conjugate75_2)),((int16x4_t*)kHz7_5ptr128)[0]);
        //mmtmp0 = [-Im(ch[0])Re(rx[0]) Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1]) Re(ch[1])Im(rx[1])]
    mmtmp1 = vmull_s16(vrev32_s16(vmul_s16(((int16x4_t*)txptr128)[1],*(int16x4_t*)conjugate75_2)), ((int16x4_t*)kHz7_5ptr128)[1]);
        //mmtmp0 = [-Im(ch[2])Re(rx[2]) Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3]) Re(ch[3])Im(rx[3])]
    mmtmp_im = vcombine_s32(vpadd_s32(vget_low_s32(mmtmp0),vget_high_s32(mmtmp0)),
                            vpadd_s32(vget_low_s32(mmtmp1),vget_high_s32(mmtmp1)));
        //mmtmp_im = [-Im(ch[0])Re(rx[0])+Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1])+Re(ch[1])Im(rx[1]) -Im(ch[2])Re(rx[2])+Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3])+Re(ch[3])Im(rx[3])]

    txptr128[0] = vcombine_s16(vmovn_s32(mmtmp_re),vmovn_s32(mmtmp_im));
    txptr128++;
    kHz7_5ptr128++;
#endif
  }

  //}
}


