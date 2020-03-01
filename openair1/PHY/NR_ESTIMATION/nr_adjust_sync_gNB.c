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

/*! \file PHY/NR_ESTIMATION/nr_adjust_sync_gNB.c
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

