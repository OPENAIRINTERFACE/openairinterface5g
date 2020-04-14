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

#include "PHY/types.h"
#include "PHY/defs_UE.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"
#include "PHY/impl_defs_top.h"

#include "common/utils/LOG/vcd_signal_dumper.h"

#define DEBUG_PHY

// Adjust location synchronization point to account for drift
// The adjustment is performed once per frame based on the
// last channel estimate of the receiver

void lte_adjust_synch(LTE_DL_FRAME_PARMS *frame_parms,
                      PHY_VARS_UE *ue,
                      module_id_t eNB_id,
		              uint8_t subframe,
                      unsigned char clear,
                      short coef)
{

  static int max_pos_fil = 0;
  static int count_max_pos_ok = 0;
  static int first_time = 1;
  int temp = 0, i, aa, max_val = 0, max_pos = 0;
  int diff;
  short Re,Im,ncoef;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_ADJUST_SYNCH, VCD_FUNCTION_IN);

  ncoef = 32767 - coef;

#ifdef DEBUG_PHY
  LOG_D(PHY,"AbsSubframe %d.%d: rx_offset (before) = %d\n",ue->proc.proc_rxtx[0].frame_rx%1024,subframe,ue->rx_offset);
#endif //DEBUG_PHY


  // we only use channel estimates from tx antenna 0 here
  for (i = 0; i < frame_parms->nb_prefix_samples; i++) {
    temp = 0;

    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
      Re = ((int16_t*)ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates_time[eNB_id][aa])[(i<<1)];
      Im = ((int16_t*)ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates_time[eNB_id][aa])[1+(i<<1)];
      temp += (Re*Re/2) + (Im*Im/2);
    }

    if (temp > max_val) {
      max_pos = i;
      max_val = temp;
    }
  }

  // filter position to reduce jitter
  if (clear == 1)
    max_pos_fil = max_pos;
  else
    max_pos_fil = ((max_pos_fil * coef) + (max_pos * ncoef)) >> 15;

  // do not filter to have proactive timing adjustment
  max_pos_fil = max_pos;

  if(subframe == 5)
  {
      diff = max_pos_fil - (frame_parms->nb_prefix_samples>>3);

      if ( abs(diff) < SYNCH_HYST )
          ue->rx_offset = 0;
      else
          ue->rx_offset = diff;

      if(abs(diff)<5)
          count_max_pos_ok ++;
      else
          count_max_pos_ok = 0;


      if(count_max_pos_ok > 10 && first_time == 1)
      {
          first_time = 0;
          ue->time_sync_cell = 1;
          if (ue->mac_enabled==1) {
              LOG_I(PHY,"[UE%d] Sending synch status to higher layers\n",ue->Mod_id);
              //mac_resynch();
              //dl_phy_sync_success(ue->Mod_id,ue->proc.proc_rxtx[0].frame_rx,0,1);//ue->common_vars.eNb_id);
              ue->UE_mode[0] = PRACH;
          }
          else {
              ue->UE_mode[0] = PUSCH;
          }
      }

      if ( ue->rx_offset < 0 )
          ue->rx_offset += FRAME_LENGTH_COMPLEX_SAMPLES;

      if ( ue->rx_offset >= FRAME_LENGTH_COMPLEX_SAMPLES )
          ue->rx_offset -= FRAME_LENGTH_COMPLEX_SAMPLES;



      #ifdef DEBUG_PHY
      LOG_D(PHY,"AbsSubframe %d.%d: ThreadId %d diff =%i rx_offset (final) = %i : clear %d,max_pos = %d,max_pos_fil = %d (peak %d) max_val %d target_pos %d \n",
              ue->proc.proc_rxtx[ue->current_thread_id[subframe]].frame_rx,
              subframe,
              ue->current_thread_id[subframe],
              diff,
              ue->rx_offset,
              clear,
              max_pos,
              max_pos_fil,
              temp,max_val,
              (frame_parms->nb_prefix_samples>>3));
      #endif //DEBUG_PHY

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_ADJUST_SYNCH, VCD_FUNCTION_OUT);
  }
}
