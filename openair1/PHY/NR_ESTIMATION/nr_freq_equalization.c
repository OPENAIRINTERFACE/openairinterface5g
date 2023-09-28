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
#include "PHY/sse_intrin.h"
#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"

// Reference of openair1/PHY/LTE_ESTIMATION/freq_equalization.c

// This is 4096/(1:4096) in simde__m128i format
static simde__m128i nr_inv_ch[4096]; /* = {0, 4096/1, 4096/2, 4096/3, 4096/4...}*/

void nr_init_fde() 
{
  for (int i = 1;i < 4096; i++) 
    nr_inv_ch[i] = simde_mm_set1_epi16(4096/i);
}

void nr_freq_equalization (NR_DL_FRAME_PARMS *frame_parms,
                           int32_t *rxdataF_comp,
                           int32_t *ul_ch_mag,
                           int32_t *ul_ch_magb,
                           uint8_t symbol,
                           uint16_t Msc_RS,
                           uint8_t Qm) 
{
  simde__m128i *rxdataF_comp128 = (simde__m128i *)rxdataF_comp;
  simde__m128i *ul_ch_mag128    = (simde__m128i *)ul_ch_mag;
  simde__m128i *ul_ch_magb128   = (simde__m128i *)ul_ch_magb;

  AssertFatal(symbol < frame_parms->symbols_per_slot, "symbol %d >= %d\n",
              symbol, frame_parms->symbols_per_slot);
  AssertFatal(Msc_RS <= frame_parms->N_RB_UL*12, "Msc_RS %d >= %d\n",
              Msc_RS, frame_parms->N_RB_UL*12);

  for (uint16_t re = 0; re < (Msc_RS >> 2); re++) {
    int16_t amp = (*((int16_t*)&ul_ch_mag128[re]));

    if (amp > 4095)
      amp = 4095;

    rxdataF_comp128[re] = simde_mm_srai_epi16(simde_mm_mullo_epi16(rxdataF_comp128[re],nr_inv_ch[amp]),3);

    if (Qm == 4)
      ul_ch_mag128[re]  = simde_mm_set1_epi16(324);  // this is 512*2/sqrt(10)
    else if (Qm == 6) {
      ul_ch_mag128[re]  = simde_mm_set1_epi16(316);  // this is 512*4/sqrt(42)
      ul_ch_magb128[re] = simde_mm_set1_epi16(158);  // this is 512*2/sqrt(42)
    } else if(Qm != 2)
      AssertFatal(1, "nr_freq_equalization(), Qm = %d, should be 2, 4 or 6. symbol=%d, Msc_RS=%d\n", Qm, symbol, Msc_RS);
  }
}
