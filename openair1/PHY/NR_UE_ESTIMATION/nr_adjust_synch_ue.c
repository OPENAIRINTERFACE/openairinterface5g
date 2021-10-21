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
#include "PHY/defs_nr_UE.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include "PHY/impl_defs_top.h"

#include "executables/softmodem-common.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

#define DEBUG_PHY

// Adjust location synchronization point to account for drift
// The adjustment is performed once per frame based on the
// last channel estimate of the receiver

void nr_adjust_synch_ue(NR_DL_FRAME_PARMS *frame_parms,
                        PHY_VARS_NR_UE *ue,
                        module_id_t gNB_id,
                        uint8_t frame,
                        uint8_t subframe,
                        unsigned char clear,
                        short coef)
{

  static int count_max_pos_ok = 0;
  static int first_time = 1;
  int max_val = 0, max_pos = 0;
  const int sync_pos = 0;
  uint8_t sync_offset = 0;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_ADJUST_SYNCH, VCD_FUNCTION_IN);

  short ncoef = 32767 - coef;

  LOG_D(PHY,"AbsSubframe %d: rx_offset (before) = %d\n",subframe,ue->rx_offset);

  // search for maximum position within the cyclic prefix
  for (int i = -frame_parms->nb_prefix_samples/2; i < frame_parms->nb_prefix_samples/2; i++) {
    int temp = 0;

    int j = (i < 0) ? (i + frame_parms->ofdm_symbol_size) : i;
    for (int aa = 0; aa < frame_parms->nb_antennas_rx; aa++) {
      int Re = ((int16_t*)ue->pbch_vars[gNB_id]->dl_ch_estimates_time[aa])[(j<<1)];
      int Im = ((int16_t*)ue->pbch_vars[gNB_id]->dl_ch_estimates_time[aa])[1+(j<<1)];
      temp += (Re*Re/2) + (Im*Im/2);
    }

    if (temp > max_val) {
      max_pos = i;
      max_val = temp;
    }
  }

  // filter position to reduce jitter
  if (clear == 1)
    ue->max_pos_fil = max_pos;
  else
    ue->max_pos_fil = ((ue->max_pos_fil * coef) + (max_pos * ncoef)) >> 15;

  // do not filter to have proactive timing adjustment
  //ue->max_pos_fil = max_pos;

  int diff = ue->max_pos_fil - sync_pos;

  if (frame_parms->freq_range==nr_FR2) 
    sync_offset = 2;
  else
    sync_offset = 0;

  if ( abs(diff) < (SYNCH_HYST+sync_offset) )
    ue->rx_offset = 0;
  else
    ue->rx_offset = diff;

  if(abs(diff)<5)
    count_max_pos_ok ++;
  else
    count_max_pos_ok = 0;
      
  //printf("adjust sync count_max_pos_ok = %d\n",count_max_pos_ok);

  if(count_max_pos_ok > 10 && first_time == 1)
  {
    first_time = 0;
    ue->time_sync_cell = 1;
    if (get_softmodem_params()->do_ra || get_softmodem_params()->sa) {
      LOG_I(PHY,"[UE%d] Sending synch status to higher layers\n",ue->Mod_id);
      //mac_resynch();
      //dl_phy_sync_success(ue->Mod_id,frame,0,1);//ue->common_vars.eNb_id);
      ue->UE_mode[0] = PRACH;
      ue->prach_resources[gNB_id]->sync_frame = frame;
      ue->prach_resources[gNB_id]->init_msg1 = 0;
    } else {
      ue->UE_mode[0] = PUSCH;
    }
  }

  if (ue->rx_offset < 0)
    ue->rx_offset += frame_parms->samples_per_frame;

  if (ue->rx_offset >= frame_parms->samples_per_frame)
    ue->rx_offset -= frame_parms->samples_per_frame;

#ifdef DEBUG_PHY
  LOG_D(PHY,"AbsSubframe %d: diff = %i, rx_offset (final) = %i : clear = %d, max_pos = %d, max_pos_fil = %d, max_val = %d, sync_pos %d\n",
        subframe,
        diff,
        ue->rx_offset,
        clear,
        max_pos,
        ue->max_pos_fil,
        max_val,
        sync_pos);
#endif //DEBUG_PHY

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_ADJUST_SYNCH, VCD_FUNCTION_OUT);
}
