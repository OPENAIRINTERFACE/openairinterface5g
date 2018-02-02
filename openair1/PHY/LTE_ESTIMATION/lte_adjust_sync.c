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
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "UTIL/LOG/vcd_signal_dumper.h"

#define DEBUG_PHY

// Adjust location synchronization point to account for drift
// The adjustment is performed once per frame based on the
// last channel estimate of the receiver

void lte_adjust_synch(LTE_DL_FRAME_PARMS *frame_parms,
                      PHY_VARS_UE *ue,
                      unsigned char eNB_id,
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
      Re = ((int16_t*)ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates_time[eNB_id][aa])[(i<<2)];
      Im = ((int16_t*)ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates_time[eNB_id][aa])[1+(i<<2)];
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

  if(subframe == 6)
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
              dl_phy_sync_success(ue->Mod_id,ue->proc.proc_rxtx[0].frame_rx,0,1);//ue->common_vars.eNb_id);
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


int lte_est_timing_advance(LTE_DL_FRAME_PARMS *frame_parms,
                           LTE_eNB_SRS *lte_eNB_srs,
                           unsigned int  *eNB_id,
                           unsigned char clear,
                           unsigned char number_of_cards,
                           short coef)

{

  static int max_pos_fil2 = 0;
  int temp, i, aa, max_pos = 0,ind;
  int max_val=0;
  short Re,Im,ncoef;
#ifdef DEBUG_PHY
  char fname[100],vname[100];
#endif

  ncoef = 32768 - coef;

  for (ind=0; ind<number_of_cards; ind++) {

    if (ind==0)
      max_val=0;


    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
      // do ifft of channel estimate
      switch(frame_parms->N_RB_DL) {
      case 6:
	dft128((int16_t*) &lte_eNB_srs->srs_ch_estimates[aa][0],
	       (int16_t*) lte_eNB_srs->srs_ch_estimates_time[aa],
	       1);
	break;
      case 25:
	dft512((int16_t*) &lte_eNB_srs->srs_ch_estimates[aa][0],
	       (int16_t*) lte_eNB_srs->srs_ch_estimates_time[aa],
	       1);
	break;
      case 50:
	dft1024((int16_t*) &lte_eNB_srs->srs_ch_estimates[aa][0],
		(int16_t*) lte_eNB_srs->srs_ch_estimates_time[aa],
		1);
	break;
      case 100:
	dft2048((int16_t*) &lte_eNB_srs->srs_ch_estimates[aa][0],
	       (int16_t*) lte_eNB_srs->srs_ch_estimates_time[aa],
	       1);
	break;
      }
#ifdef DEBUG_PHY
      sprintf(fname,"srs_ch_estimates_time_%d%d.m",ind,aa);
      sprintf(vname,"srs_time_%d%d",ind,aa);
      write_output(fname,vname,lte_eNB_srs->srs_ch_estimates_time[aa],frame_parms->ofdm_symbol_size*2,2,1);
#endif
    }

    // we only use channel estimates from tx antenna 0 here
    // remember we fixed the SRS to use only every second subcarriers
    for (i = 0; i < frame_parms->ofdm_symbol_size/2; i++) {
      temp = 0;

      for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        Re = ((int16_t*)lte_eNB_srs->srs_ch_estimates_time[aa])[(i<<1)];
        Im = ((int16_t*)lte_eNB_srs->srs_ch_estimates_time[aa])[1+(i<<1)];
        temp += (Re*Re/2) + (Im*Im/2);
      }

      if (temp > max_val) {
        max_pos = i;
        max_val = temp;
        *eNB_id = ind;
      }
    }
  }

  // filter position to reduce jitter
  if (clear == 1)
    max_pos_fil2 = max_pos;
  else
    max_pos_fil2 = ((max_pos_fil2 * coef) + (max_pos * ncoef)) >> 15;

  return(max_pos_fil2);
}


int lte_est_timing_advance_pusch(PHY_VARS_eNB* eNB,uint8_t UE_id)
{
  int temp, i, aa, max_pos=0, max_val=0;
  short Re,Im;

  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;
  LTE_eNB_PUSCH *eNB_pusch_vars = eNB->pusch_vars[UE_id];
  int32_t **ul_ch_estimates_time=  eNB_pusch_vars->drs_ch_estimates_time;
  uint8_t cyclic_shift = 0;
  int sync_pos = (frame_parms->ofdm_symbol_size-cyclic_shift*frame_parms->ofdm_symbol_size/12)%(frame_parms->ofdm_symbol_size);

  AssertFatal(frame_parms->ofdm_symbol_size > 127,"frame_parms->ofdm_symbol_size %d<128\n",frame_parms->ofdm_symbol_size);
  AssertFatal(frame_parms->nb_antennas_rx >0 && frame_parms->nb_antennas_rx<3,"frame_parms->nb_antennas_rx %d not in [0,1]\n",
	      frame_parms->nb_antennas_rx);
  for (i = 0; i < frame_parms->ofdm_symbol_size; i++) {
    temp = 0;

    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
      Re = ((int16_t*)ul_ch_estimates_time[aa])[(i<<1)];
      Im = ((int16_t*)ul_ch_estimates_time[aa])[1+(i<<1)];
      temp += (Re*Re/2) + (Im*Im/2);
    }

    if (temp > max_val) {
      max_pos = i;
      max_val = temp;
    }
  }

  if (max_pos>frame_parms->ofdm_symbol_size/2)
    max_pos = max_pos-frame_parms->ofdm_symbol_size;

  //#ifdef DEBUG_PHY
  LOG_D(PHY,"frame %d: max_pos = %d, sync_pos=%d\n",eNB->proc.frame_rx,max_pos,sync_pos);
  //#endif //DEBUG_PHY

  return max_pos - sync_pos;
}
