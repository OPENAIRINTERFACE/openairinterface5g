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

/*! \file PHY/NR_ESTIMATION/nr_measurements_gNB.c
* \brief TA estimation for TA updates
* \author Ahmed Hussein
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: ahmed.hussein@iis.fraunhofer.de
* \note
* \warning
*/

#include "PHY/types.h"
#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "nr_ul_estimation.h"

int nr_est_timing_advance_pusch(PHY_VARS_gNB* gNB, int UE_id)
{
  int temp, i, aa, max_pos = 0, max_val = 0;
  short Re, Im;
  uint8_t cyclic_shift = 0;
  
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  NR_gNB_PUSCH *gNB_pusch_vars   = gNB->pusch_vars[UE_id];
  int32_t **ul_ch_estimates_time = gNB_pusch_vars->ul_ch_estimates_time;
  
  int sync_pos = (frame_parms->ofdm_symbol_size - cyclic_shift*frame_parms->ofdm_symbol_size/12) % (frame_parms->ofdm_symbol_size);

  for (i = 0; i < frame_parms->ofdm_symbol_size; i++) {
    temp = 0;

    for (aa = 0; aa < frame_parms->nb_antennas_rx; aa++) {
      Re = ((int16_t*)ul_ch_estimates_time[aa])[(i<<1)];
      Im = ((int16_t*)ul_ch_estimates_time[aa])[1+(i<<1)];
      temp += (Re*Re/2) + (Im*Im/2);      
    }

    if (temp > max_val) {
      max_pos = i;
      max_val = temp;
    }
  }

  if (max_pos > frame_parms->ofdm_symbol_size/2)
    max_pos = max_pos - frame_parms->ofdm_symbol_size;


  return max_pos - sync_pos;
}


void gNB_I0_measurements(PHY_VARS_gNB *gNB) {

  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  NR_gNB_COMMON *common_vars = &gNB->common_vars;
  PHY_MEASUREMENTS_gNB *measurements = &gNB->measurements;
  uint32_t *rb_mask = gNB->rb_mask_ul;
  int symbol = gNB->ulmask_symb;
  int rb, offset, nb_rb;
  uint32_t n0_power_tot, n0_subband_power_temp=0;
  int32_t *ul_ch;

  if (symbol>-1) {
    n0_power_tot = 0;
    for (int aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      nb_rb = 0;
      for (rb=0; rb<frame_parms->N_RB_UL; rb++) {
        if ((rb_mask[rb>>5]&(1<<(rb&31))) == 0) {  // check that rb was not used in this subframe
          nb_rb++;
          offset = (frame_parms->first_carrier_offset + (rb*12))%frame_parms->ofdm_symbol_size;
          offset += (symbol*frame_parms->ofdm_symbol_size);
          ul_ch  = &common_vars->rxdataF[aarx][offset];
          //TODO what about DC?
          n0_subband_power_temp += signal_energy_nodc(ul_ch,12);
        }
      }
      measurements->n0_power[aarx] = n0_subband_power_temp/nb_rb;
      measurements->n0_power_dB[aarx] = dB_fixed(measurements->n0_power[aarx]);
      n0_power_tot += measurements->n0_power[aarx];
    }
    measurements->n0_power_tot_dB = dB_fixed(n0_power_tot);
  }
}

