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
* \brief gNB measurement routines
* \author Ahmed Hussein, G. Casati, K. Saaifan
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: ahmed.hussein@iis.fraunhofer.de, guido.casati@iis.fraunhofer.de, khodr.saaifan@iis.fraunhofer.de
* \note
* \warning
*/

#include "PHY/types.h"
#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "nr_ul_estimation.h"

extern openair0_config_t openair0_cfg[MAX_CARDS];

int nr_est_timing_advance_pusch(PHY_VARS_gNB* gNB, int UE_id)
{
  int i, aa, max_pos = 0, max_val = 0;
  
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  NR_gNB_PUSCH *gNB_pusch_vars   = gNB->pusch_vars[UE_id];
  int32_t **ul_ch_estimates_time = gNB_pusch_vars->ul_ch_estimates_time;
  
  int sync_pos = frame_parms->nb_prefix_samples / 8;

  for (i = 0; i < frame_parms->ofdm_symbol_size; i++) {
    int temp = 0;

    for (aa = 0; aa < frame_parms->nb_antennas_rx; aa++) {
      short Re = ((int16_t*)ul_ch_estimates_time[aa])[(i<<1)];
      short Im = ((int16_t*)ul_ch_estimates_time[aa])[1+(i<<1)];
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


void dump_nr_I0_stats(FILE *fd,PHY_VARS_gNB *gNB) {


    int min_I0=1000,max_I0=0;
    int amin=0,amax=0;
    for (int i=0; i<gNB->frame_parms.N_RB_UL; i++) {
      if (i==(gNB->frame_parms.N_RB_UL>>1) - 1) i+=2;

      if (gNB->measurements.n0_subband_power_tot_dB[i]<min_I0) {min_I0 = gNB->measurements.n0_subband_power_tot_dB[i]; amin=i;}

      if (gNB->measurements.n0_subband_power_tot_dB[i]>max_I0) {max_I0 = gNB->measurements.n0_subband_power_tot_dB[i]; amax=i;}
    }

    for (int i=0; i<gNB->frame_parms.N_RB_UL; i++) {
     fprintf(fd,"%2d.",gNB->measurements.n0_subband_power_tot_dB[i]-gNB->measurements.n0_subband_power_avg_dB);
     if (i%25 == 24) fprintf(fd,"\n");
    }

    fprintf(fd,"PRACH I0 = %d.%d dB\n",gNB->measurements.prach_I0/10,gNB->measurements.prach_I0%10);


}



void gNB_I0_measurements(PHY_VARS_gNB *gNB,int first_symb,int num_symb) {

  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  NR_gNB_COMMON *common_vars = &gNB->common_vars;
  PHY_MEASUREMENTS_gNB *measurements = &gNB->measurements;
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  double rx_gain = openair0_cfg[0].rx_gain[0];
  double rx_gain_offset = openair0_cfg[0].rx_gain_offset[0];
  uint32_t *rb_mask = gNB->rb_mask_ul;
  int rb, offset, offset0, nb_rb, len;
  uint32_t n0_subband_power_temp = 0;
  int32_t *ul_ch;
  int32_t n0_power_tot;
  int64_t n0_power_tot2;

  nb_rb = 0;
  n0_power_tot2=0;
  for (rb=0; rb<frame_parms->N_RB_UL; rb++) {
    n0_power_tot=0;
    offset0 = (frame_parms->first_carrier_offset + (rb*12))%frame_parms->ofdm_symbol_size;
    if ((rb_mask[rb>>5]&(1<<(rb&31))) == 0) {  // check that rb was not used in this subframe
      nb_rb++;
      for (int aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
	       measurements->n0_subband_power[aarx][rb]=0;
	       for (int s=first_symb;s<(first_symb+num_symb);s++) {

	          offset = offset0 + (s*frame_parms->ofdm_symbol_size);
	          ul_ch  = &common_vars->rxdataF[aarx][offset];
	          len = 12;
	          if (((frame_parms->N_RB_UL&1) == 1) && 
	               (rb==(frame_parms->N_RB_UL>>1))) {
	             len=6;
	          }
	          AssertFatal(ul_ch, "RX signal buffer (freq) problem\n");
	          measurements->n0_subband_power[aarx][rb] += signal_energy_nodc(ul_ch,len);
	        } // symbol
          measurements->n0_subband_power[aarx][rb]/=num_symb;
          measurements->n0_subband_power_dB[aarx][rb] = dB_fixed(measurements->n0_subband_power[aarx][rb]);
          n0_power_tot += measurements->n0_subband_power[aarx][rb];
      } //antenna
      n0_power_tot/=frame_parms->nb_antennas_rx;
      n0_power_tot2 += n0_power_tot;
      measurements->n0_subband_power_tot_dB[rb] = dB_fixed(n0_power_tot);
      measurements->n0_subband_power_tot_dBm[rb] = measurements->n0_subband_power_tot_dB[rb] - gNB->rx_total_gain_dB - dB_fixed(frame_parms->N_RB_UL);
    }
  } //rb

  if (nb_rb>0) measurements->n0_subband_power_avg_dB = dB_fixed(n0_power_tot2/nb_rb);

}


// Scope: This function computes the UL SNR from the UL channel estimates
//
// Todo:
// - averaging IIR filter for RX power and noise
void nr_gnb_measurements(PHY_VARS_gNB *gNB, uint8_t ulsch_id, unsigned char harq_pid, unsigned char symbol){

  int rx_power_tot[NUMBER_OF_NR_ULSCH_MAX];
  int rx_power[NUMBER_OF_NR_ULSCH_MAX][NB_ANTENNAS_RX];
  unsigned short rx_power_avg_dB[NUMBER_OF_NR_ULSCH_MAX];
  unsigned short rx_power_tot_dB[NUMBER_OF_NR_ULSCH_MAX];

  double             rx_gain = openair0_cfg[0].rx_gain[0];
  double      rx_gain_offset = openair0_cfg[0].rx_gain_offset[0];
  PHY_MEASUREMENTS_gNB *meas = &gNB->measurements;
  NR_DL_FRAME_PARMS      *fp = &gNB->frame_parms;
  int              ch_offset = fp->ofdm_symbol_size * symbol;
  int                N_RB_UL = gNB->ulsch[ulsch_id][0]->harq_processes[harq_pid]->ulsch_pdu.rb_size;

  rx_power_tot[ulsch_id] = 0;

  for (int aarx = 0; aarx < fp->nb_antennas_rx; aarx++){

    rx_power[ulsch_id][aarx] = 0;

    for (int aatx = 0; aatx < fp->nb_antennas_tx; aatx++){

      meas->rx_spatial_power[ulsch_id][aatx][aarx] = (signal_energy_nodc(&gNB->pusch_vars[ulsch_id]->ul_ch_estimates[aarx][ch_offset], N_RB_UL * NR_NB_SC_PER_RB));

      if (meas->rx_spatial_power[ulsch_id][aatx][aarx] < 0) {
        meas->rx_spatial_power[ulsch_id][aatx][aarx] = 0;
      }

      meas->rx_spatial_power_dB[ulsch_id][aatx][aarx] = (unsigned short) dB_fixed(meas->rx_spatial_power[ulsch_id][aatx][aarx]);
      rx_power[ulsch_id][aarx] += meas->rx_spatial_power[ulsch_id][aatx][aarx];

    }

    rx_power_tot[ulsch_id] += rx_power[ulsch_id][aarx];

  }

  rx_power_tot_dB[ulsch_id] = (unsigned short) dB_fixed(rx_power_tot[ulsch_id]);
  rx_power_avg_dB[ulsch_id] = rx_power_tot_dB[ulsch_id];

  meas->wideband_cqi_tot[ulsch_id] = dB_fixed2(rx_power_tot[ulsch_id], meas->n0_power_tot);
  meas->rx_rssi_dBm[ulsch_id] = rx_power_avg_dB[ulsch_id] + 30 - 10 * log10(pow(2, 30)) - (rx_gain - rx_gain_offset) - dB_fixed(fp->ofdm_symbol_size);

  LOG_D(PHY, "[ULSCH %d] RSSI %d dBm/RE, RSSI (digital) %d dB (N_RB_UL %d), WBand CQI tot %d dB, N0 Power tot %d\n",
    ulsch_id,
    meas->rx_rssi_dBm[ulsch_id],
    rx_power_avg_dB[ulsch_id],
    N_RB_UL,
    meas->wideband_cqi_tot[ulsch_id],
    meas->n0_power_tot);

}
