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
#include "PHY/phy_extern.h"

#include "PHY/sse_intrin.h"


//#define k1 1000
#define k1 1024
#define k2 (1024-k1)

int32_t rx_power_avg_eNB[3];


void dump_I0_stats(FILE *fd,PHY_VARS_eNB *eNB) {


    int min_I0=1000,max_I0=0;
    int amin=0,amax=0;
    for (int i=0; i<eNB->frame_parms.N_RB_UL; i++) {
      if (i==(eNB->frame_parms.N_RB_UL>>1) - 1) i+=2;

      if (eNB->measurements.n0_subband_power_tot_dB[i]<min_I0) {min_I0 = eNB->measurements.n0_subband_power_tot_dB[i]; amin=i;}

      if (eNB->measurements.n0_subband_power_tot_dB[i]>max_I0) {max_I0 = eNB->measurements.n0_subband_power_tot_dB[i]; amax=i;}
    }

    for (int i=0; i<eNB->frame_parms.N_RB_UL; i++) {
     fprintf(fd,"%2d.",eNB->measurements.n0_subband_power_tot_dB[i]-eNB->measurements.n0_subband_power_avg_dB);
     if (i%25 == 24) fprintf(fd,"\n");
    }

    fprintf(fd,"\nmax_I0 %d (rb %d), min_I0 %d (rb %d), avg I0 %d\n", max_I0, amax, min_I0, amin, eNB->measurements.n0_subband_power_avg_dB);

    fprintf(fd,"prach_I0 = %d.%d dB\n",eNB->measurements.prach_I0/10,eNB->measurements.prach_I0%10);


}
void lte_eNB_I0_measurements(PHY_VARS_eNB *eNB,
			     int subframe,
                             module_id_t eNB_id,
                             unsigned char clear)
{

  LTE_eNB_COMMON *common_vars = &eNB->common_vars;
  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;
  PHY_MEASUREMENTS_eNB *measurements = &eNB->measurements;
  uint32_t *rb_mask = eNB->rb_mask_ul;

  uint32_t aarx /* ,rx_power_correction */;
  uint32_t rb;
  int32_t *ul_ch;
  int32_t n0_power_tot;
  int64_t n0_power_tot2;
  int len;
  int offset;
  // noise measurements
  // for the moment we measure the noise on the 7th OFDM symbol (in S subframe)

  measurements->n0_power_tot = 0;

  if (common_vars->rxdata!=NULL) {
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      if (clear == 1)
	measurements->n0_power[aarx]=0;
      
      
      measurements->n0_power[aarx] = ((k1*signal_energy(&common_vars->rxdata[aarx][(frame_parms->samples_per_tti<<1) -frame_parms->ofdm_symbol_size],
							frame_parms->ofdm_symbol_size)) + k2*measurements->n0_power[aarx])>>10;
      //measurements->n0_power[aarx] = (measurements->n0_power[aarx]) * 12*frame_parms->N_RB_DL)/(frame_parms->ofdm_symbol_size);
      measurements->n0_power_dB[aarx] = (unsigned short) dB_fixed(measurements->n0_power[aarx]);
      measurements->n0_power_tot +=  measurements->n0_power[aarx];
    }
  

    measurements->n0_power_tot_dB = (unsigned short) dB_fixed(measurements->n0_power_tot);
    
    measurements->n0_power_tot_dBm = measurements->n0_power_tot_dB - eNB->rx_total_gain_dB;
  //      printf("n0_power %d\n",measurements->n0_power_tot_dB);

  }

  n0_power_tot2=0;
  int nb_rb=0;
  for (rb=0; rb<frame_parms->N_RB_UL; rb++) {

    n0_power_tot=0; 
    int offset0= (frame_parms->first_carrier_offset + (rb*12))%frame_parms->ofdm_symbol_size;

    if ((rb_mask[rb>>5]&(1<<(rb&31))) == 0) {  // check that rb was not used in this subframe
      nb_rb++;
      for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
        measurements->n0_subband_power[aarx][rb] = 0;
        for (int s=0;s<(14-(frame_parms->Ncp<<1));s++) {
	        offset = offset0 + (s*frame_parms->ofdm_symbol_size);
	        ul_ch  = &common_vars->rxdataF[aarx][offset];
	        len = 12;
	// just do first half of middle PRB for odd number of PRBs
	        if (((frame_parms->N_RB_UL&1) == 1) && 
	            (rb==(frame_parms->N_RB_UL>>1))) {
	           len=6;
	        }

	        AssertFatal(ul_ch, "RX signal buffer (freq) problem");


	        measurements->n0_subband_power[aarx][rb] += signal_energy_nodc(ul_ch,len);
	  
        } // symbol
        measurements->n0_subband_power[aarx][rb]/=(14-(frame_parms->Ncp<<1));
        measurements->n0_subband_power_dB[aarx][rb] = dB_fixed(measurements->n0_subband_power[aarx][rb]);
        n0_power_tot += measurements->n0_subband_power[aarx][rb];

      } //antenna
      n0_power_tot/=frame_parms->nb_antennas_rx;
      n0_power_tot2 += n0_power_tot;
      measurements->n0_subband_power_tot_dB[rb] = dB_fixed(n0_power_tot);
      measurements->n0_subband_power_tot_dBm[rb] = measurements->n0_subband_power_tot_dB[rb] - eNB->rx_total_gain_dB - dB_fixed(frame_parms->N_RB_UL);
      
    } //rb not used in subframe
  } //rb
  if (nb_rb>0) measurements->n0_subband_power_avg_dB = dB_fixed(n0_power_tot2/nb_rb);
}

void lte_eNB_srs_measurements(PHY_VARS_eNB *eNB,
                              module_id_t eNB_id,
                              module_id_t srs_id,
                              unsigned char init_averaging)
{
  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;
  PHY_MEASUREMENTS_eNB *measurements = &eNB->measurements;
  LTE_eNB_SRS *srs_vars = &eNB->srs_vars[srs_id];

  int32_t aarx,rx_power_correction;
  int32_t rx_power;
  uint32_t rb;
  int32_t *ul_ch;

  //printf("Running eNB_srs_measurements for eNB_id %d\n",eNB_id);

  if (init_averaging == 1)
    rx_power_avg_eNB[srs_id] = 0;

  rx_power = 0;


  if ( (frame_parms->ofdm_symbol_size == 128) ||
       (frame_parms->ofdm_symbol_size == 512) )
    rx_power_correction = 2;
  else
    rx_power_correction = 1;



  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {


    measurements->rx_spatial_power[srs_id][0][aarx] =
      ((signal_energy_nodc(&srs_vars->srs_ch_estimates[aarx][frame_parms->first_carrier_offset],
                           (frame_parms->N_RB_DL*6)) +
        signal_energy_nodc(&srs_vars->srs_ch_estimates[aarx][1],
                           (frame_parms->N_RB_DL*6)))*rx_power_correction) -
      measurements->n0_power[aarx];

    measurements->rx_spatial_power[srs_id][0][aarx]<<=1;  // because of noise only in odd samples

    measurements->rx_spatial_power_dB[srs_id][0][aarx] = (unsigned short) dB_fixed(measurements->rx_spatial_power[srs_id][0][aarx]);

    measurements->wideband_cqi[srs_id][aarx] = measurements->rx_spatial_power[srs_id][0][aarx];



    //      measurements->rx_power[UE_id][aarx]/=frame_parms->nb_antennas_tx;
    measurements->wideband_cqi_dB[srs_id][aarx] = (unsigned short) dB_fixed(measurements->wideband_cqi[srs_id][aarx]);
    rx_power += measurements->wideband_cqi[srs_id][aarx];
    //      measurements->rx_avg_power_dB[UE_id] += measurements->rx_power_dB[UE_id][aarx];
  }



  //    measurements->rx_avg_power_dB[UE_id]/=frame_parms->nb_antennas_rx;
  if (init_averaging == 0)
    rx_power_avg_eNB[srs_id] = ((k1*rx_power_avg_eNB[srs_id]) + (k2*rx_power))>>10;
  else
    rx_power_avg_eNB[srs_id] = rx_power;

  measurements->wideband_cqi_tot[srs_id] = dB_fixed2(rx_power,2*measurements->n0_power_tot);
  // times 2 since we have noise only in the odd carriers of the srs comb

  measurements->rx_rssi_dBm[srs_id] = (int32_t)dB_fixed(rx_power_avg_eNB[srs_id])-eNB->rx_total_gain_dB;




  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {


    for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

      //      printf("common_vars->srs_ch_estimates[0] => %x\n",common_vars->srs_ch_estimates[0]);
      if (rb < 12)
        ul_ch    = &srs_vars->srs_ch_estimates[aarx][frame_parms->first_carrier_offset + (rb*12)];
      else if (rb>12)
        ul_ch    = &srs_vars->srs_ch_estimates[aarx][6 + (rb-13)*12];
      else {
        measurements->subband_cqi_dB[srs_id][aarx][rb] = 0;
        continue;
      }

      // cqi
      if (aarx==0)
        measurements->subband_cqi_tot[srs_id][rb]=0;

      measurements->subband_cqi[srs_id][aarx][rb] = (signal_energy_nodc(ul_ch,12))*rx_power_correction - measurements->n0_power[aarx];

      if (measurements->subband_cqi[srs_id][aarx][rb] < 0)
        measurements->subband_cqi[srs_id][aarx][rb]=0;

      measurements->subband_cqi_tot[srs_id][rb] += measurements->subband_cqi[srs_id][aarx][rb];
      measurements->subband_cqi_dB[srs_id][aarx][rb] = dB_fixed2(measurements->subband_cqi[srs_id][aarx][rb],
          2*measurements->n0_power[aarx]);
      // 2*n0_power since we have noise from the odd carriers in the comb of the srs

      //    msg("subband_cqi[%d][%d][%d] => %d (%d dB)\n",eNB_id,aarx,rb,measurements->subband_cqi[aarx][rb],measurements->subband_cqi_dB[aarx][rb]);
    }

  }


  for (rb=0; rb<frame_parms->N_RB_DL; rb++) {
    measurements->subband_cqi_tot_dB[srs_id][rb] = dB_fixed2(measurements->subband_cqi_tot[srs_id][rb],
        measurements->n0_power_tot);
    /*
    if (measurements->subband_cqi_tot_dB[UE_id][rb] == 65)
      msg("eNB meas error *****subband_cqi_tot[%d][%d] %d => %d dB (n0 %d)\n",UE_id,rb,measurements->subband_cqi_tot[UE_id][rb],measurements->subband_cqi_tot_dB[UE_id][rb],measurements->n0_power_tot);
    */
  }

}

void lte_eNB_I0_measurements_emul(PHY_VARS_eNB *eNB,
                                  uint8_t sect_id)
{

  LOG_D(PHY,"EMUL lte_eNB_IO_measurements_emul: eNB %d, sect %d\n",eNB->Mod_id,sect_id);
}






