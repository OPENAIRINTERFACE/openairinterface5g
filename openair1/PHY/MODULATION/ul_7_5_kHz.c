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

#include "PHY/defs_eNB.h"
#include <math.h>
#include "PHY/sse_intrin.h"
#include "modulation_extern.h"

void remove_7_5_kHz(RU_t *ru,uint8_t slot)
{


  int32_t **rxdata=ru->common.rxdata;
  int32_t **rxdata_7_5kHz=ru->common.rxdata_7_5kHz;
  uint16_t len;
  uint32_t *kHz7_5ptr;
  simde__m128i *rxptr128, *rxptr128_7_5kHz, *kHz7_5ptr128, kHz7_5_2, mmtmp_re, mmtmp_im, mmtmp_re2, mmtmp_im2;
  uint32_t slot_offset,slot_offset2;
  uint8_t aa;
  uint32_t i;
  LTE_DL_FRAME_PARMS *frame_parms=ru->frame_parms;

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


  slot_offset = ((uint32_t)slot * frame_parms->samples_per_tti/2)-ru->N_TA_offset;
  slot_offset2 = (uint32_t)(slot&1) * frame_parms->samples_per_tti/2;

  len = frame_parms->samples_per_tti/2;

  for (aa=0; aa<ru->nb_rx; aa++) {
    rxptr128 = (simde__m128i *)&rxdata[aa][slot_offset];
    rxptr128_7_5kHz = (simde__m128i *)&rxdata_7_5kHz[aa][slot_offset2];
    kHz7_5ptr128 = (simde__m128i *)kHz7_5ptr;
    // apply 7.5 kHz

    //      if (((slot>>1)&1) == 0) { // apply the sinusoid from the table directly
    for (i=0; i<(len>>2); i++) {
      kHz7_5_2 = simde_mm_sign_epi16(*kHz7_5ptr128, *(simde__m128i *)&conjugate75_2[0]);
      mmtmp_re = simde_mm_madd_epi16(*rxptr128, kHz7_5_2);
      // Real part of complex multiplication (note: 7_5kHz signal is conjugated for this to work)
      mmtmp_im = simde_mm_shufflelo_epi16(kHz7_5_2, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
      mmtmp_im = simde_mm_shufflehi_epi16(mmtmp_im, SIMDE_MM_SHUFFLE(2, 3, 0, 1));
      mmtmp_im = simde_mm_sign_epi16(mmtmp_im, *(simde__m128i *)&conjugate75[0]);
      mmtmp_im = simde_mm_madd_epi16(mmtmp_im, rxptr128[0]);
      mmtmp_re = simde_mm_srai_epi32(mmtmp_re, 15);
      mmtmp_im = simde_mm_srai_epi32(mmtmp_im, 15);
      mmtmp_re2 = simde_mm_unpacklo_epi32(mmtmp_re, mmtmp_im);
      mmtmp_im2 = simde_mm_unpackhi_epi32(mmtmp_re, mmtmp_im);

      rxptr128_7_5kHz[0] = simde_mm_packs_epi32(mmtmp_re2, mmtmp_im2);
      rxptr128++;
      rxptr128_7_5kHz++;
      kHz7_5ptr128++;
    }
    // undo 7.5 kHz offset for symbol 3 in case RU is slave (for OTA synchronization)
    if (ru->is_slave == 1 && slot == 2){
      memcpy((void*)&rxdata_7_5kHz[aa][(3*frame_parms->ofdm_symbol_size)+
                                (2*frame_parms->nb_prefix_samples)+
                                frame_parms->nb_prefix_samples0],
             (void*)&rxdata[aa][slot_offset+ru->N_TA_offset+
                           (3*frame_parms->ofdm_symbol_size)+
                           (2*frame_parms->nb_prefix_samples)+
                           frame_parms->nb_prefix_samples0],
             (frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples)*sizeof(int32_t));
}  
}
}

