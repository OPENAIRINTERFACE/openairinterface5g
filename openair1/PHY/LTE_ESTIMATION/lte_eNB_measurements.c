/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "PHY/sse_intrin.h"


//#define k1 1000
#define k1 1024
#define k2 (1024-k1)

int32_t rx_power_avg_eNB[3][3];


void lte_eNB_I0_measurements(PHY_VARS_eNB *phy_vars_eNB,
			     int subframe,
                             unsigned char eNB_id,
                             unsigned char clear)
{

  LTE_eNB_COMMON *eNB_common_vars = &phy_vars_eNB->lte_eNB_common_vars;
  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_eNB->lte_frame_parms;
  PHY_MEASUREMENTS_eNB *phy_measurements = &phy_vars_eNB->PHY_measurements_eNB[eNB_id];
  uint32_t *rb_mask = phy_vars_eNB->rb_mask_ul;

  uint32_t aarx /* ,rx_power_correction */;
  uint32_t rb;
  int32_t *ul_ch;
  int32_t n0_power_tot;
  int len;
  int offset;
  // noise measurements
  // for the moment we measure the noise on the 7th OFDM symbol (in S subframe)

  phy_measurements->n0_power_tot = 0;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    if (clear == 1)
      phy_measurements->n0_power[aarx]=0;


    phy_measurements->n0_power[aarx] = ((k1*signal_energy(&eNB_common_vars->rxdata[eNB_id][aarx][(frame_parms->samples_per_tti<<1) -frame_parms->ofdm_symbol_size],
							  frame_parms->ofdm_symbol_size)) + k2*phy_measurements->n0_power[aarx])>>10;
    //phy_measurements->n0_power[aarx] = (phy_measurements->n0_power[aarx]) * 12*frame_parms->N_RB_DL)/(frame_parms->ofdm_symbol_size);
    phy_measurements->n0_power_dB[aarx] = (unsigned short) dB_fixed(phy_measurements->n0_power[aarx]);
    phy_measurements->n0_power_tot +=  phy_measurements->n0_power[aarx];
  }

  phy_measurements->n0_power_tot_dB = (unsigned short) dB_fixed(phy_measurements->n0_power_tot);

  phy_measurements->n0_power_tot_dBm = phy_measurements->n0_power_tot_dB - phy_vars_eNB->rx_total_gain_eNB_dB;
  //      printf("n0_power %d\n",phy_measurements->n0_power_tot_dB);


  for (rb=0; rb<frame_parms->N_RB_UL; rb++) {

    n0_power_tot=0;
    if ((rb_mask[rb>>5]&(1<<(rb&31))) == 0) {  // check that rb was not used in this subframe
      for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

      // select the 7th symbol in an uplink subframe
	offset = (frame_parms->first_carrier_offset + (rb*12))%frame_parms->ofdm_symbol_size;
	offset += (7*frame_parms->ofdm_symbol_size);
	ul_ch  = &eNB_common_vars->rxdataF[eNB_id][aarx][offset];
	len = 12;
	// just do first half of middle PRB for odd number of PRBs
	if (((frame_parms->N_RB_UL&1) == 1) && 
	    (rb==(frame_parms->N_RB_UL>>1))) {
	  len=6;
	}
	if (clear == 1)
	  phy_measurements->n0_subband_power[aarx][rb]=0;

	AssertFatal(ul_ch, "RX signal buffer (freq) problem");


	phy_measurements->n0_subband_power[aarx][rb] = signal_energy_nodc(ul_ch,len);
	//((k1*(signal_energy_nodc(ul_ch,len))) 
	  //  + (k2*phy_measurements->n0_subband_power[aarx][rb]));  
	  
	phy_measurements->n0_subband_power_dB[aarx][rb] = dB_fixed(phy_measurements->n0_subband_power[aarx][rb]);
	//		printf("subframe %d (%d): eNb %d, aarx %d, rb %d len %d: energy %d (%d dB)\n",subframe,offset,eNB_id,aarx,rb,len,signal_energy_nodc(ul_ch,len),  
	//	       phy_measurements->n0_subband_power_dB[aarx][rb]);
	n0_power_tot += phy_measurements->n0_subband_power[aarx][rb];
      }
      
      phy_measurements->n0_subband_power_tot_dB[rb] = dB_fixed(n0_power_tot);
      phy_measurements->n0_subband_power_tot_dBm[rb] = phy_measurements->n0_subband_power_tot_dB[rb] - phy_vars_eNB->rx_total_gain_eNB_dB - dB_fixed(frame_parms->N_RB_UL);
      
    }
  }
}

void lte_eNB_srs_measurements(PHY_VARS_eNB *phy_vars_eNb,
                              unsigned char eNB_id,
                              unsigned char UE_id,
                              unsigned char init_averaging)
{
  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_eNb->lte_frame_parms;
  PHY_MEASUREMENTS_eNB *phy_measurements = &phy_vars_eNb->PHY_measurements_eNB[eNB_id];
  LTE_eNB_SRS *eNB_srs_vars = &phy_vars_eNb->lte_eNB_srs_vars[UE_id];

  int32_t aarx,rx_power_correction;
  int32_t rx_power;
  uint32_t rb;
  int32_t *ul_ch;

  //printf("Running eNB_srs_measurements for eNB_id %d\n",eNB_id);

  if (init_averaging == 1)
    rx_power_avg_eNB[eNB_id][UE_id] = 0;

  rx_power = 0;


  if ( (frame_parms->ofdm_symbol_size == 128) ||
       (frame_parms->ofdm_symbol_size == 512) )
    rx_power_correction = 2;
  else
    rx_power_correction = 1;



  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {


    phy_measurements->rx_spatial_power[UE_id][0][aarx] =
      ((signal_energy_nodc(&eNB_srs_vars->srs_ch_estimates[eNB_id][aarx][frame_parms->first_carrier_offset],
                           (frame_parms->N_RB_DL*6)) +
        signal_energy_nodc(&eNB_srs_vars->srs_ch_estimates[eNB_id][aarx][1],
                           (frame_parms->N_RB_DL*6)))*rx_power_correction) -
      phy_measurements->n0_power[aarx];

    phy_measurements->rx_spatial_power[UE_id][0][aarx]<<=1;  // because of noise only in odd samples

    phy_measurements->rx_spatial_power_dB[UE_id][0][aarx] = (unsigned short) dB_fixed(phy_measurements->rx_spatial_power[UE_id][0][aarx]);

    phy_measurements->wideband_cqi[UE_id][aarx] = phy_measurements->rx_spatial_power[UE_id][0][aarx];



    //      phy_measurements->rx_power[UE_id][aarx]/=frame_parms->nb_antennas_tx;
    phy_measurements->wideband_cqi_dB[UE_id][aarx] = (unsigned short) dB_fixed(phy_measurements->wideband_cqi[UE_id][aarx]);
    rx_power += phy_measurements->wideband_cqi[UE_id][aarx];
    //      phy_measurements->rx_avg_power_dB[UE_id] += phy_measurements->rx_power_dB[UE_id][aarx];
  }



  //    phy_measurements->rx_avg_power_dB[UE_id]/=frame_parms->nb_antennas_rx;
  if (init_averaging == 0)
    rx_power_avg_eNB[UE_id][eNB_id] = ((k1*rx_power_avg_eNB[UE_id][eNB_id]) + (k2*rx_power))>>10;
  else
    rx_power_avg_eNB[UE_id][eNB_id] = rx_power;

  phy_measurements->wideband_cqi_tot[UE_id] = dB_fixed2(rx_power,2*phy_measurements->n0_power_tot);
  // times 2 since we have noise only in the odd carriers of the srs comb

  phy_measurements->rx_rssi_dBm[UE_id] = (int32_t)dB_fixed(rx_power_avg_eNB[UE_id][eNB_id])-phy_vars_eNb->rx_total_gain_eNB_dB;




  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {


    for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

      //      printf("eNB_common_vars->srs_ch_estimates[0] => %x\n",eNB_common_vars->srs_ch_estimates[0]);
      if (rb < 12)
        ul_ch    = &eNB_srs_vars->srs_ch_estimates[eNB_id][aarx][frame_parms->first_carrier_offset + (rb*12)];
      else if (rb>12)
        ul_ch    = &eNB_srs_vars->srs_ch_estimates[eNB_id][aarx][6 + (rb-13)*12];
      else {
        phy_measurements->subband_cqi_dB[UE_id][aarx][rb] = 0;
        continue;
      }

      // cqi
      if (aarx==0)
        phy_measurements->subband_cqi_tot[UE_id][rb]=0;

      phy_measurements->subband_cqi[UE_id][aarx][rb] = (signal_energy_nodc(ul_ch,12))*rx_power_correction - phy_measurements->n0_power[aarx];

      if (phy_measurements->subband_cqi[UE_id][aarx][rb] < 0)
        phy_measurements->subband_cqi[UE_id][aarx][rb]=0;

      phy_measurements->subband_cqi_tot[UE_id][rb] += phy_measurements->subband_cqi[UE_id][aarx][rb];
      phy_measurements->subband_cqi_dB[UE_id][aarx][rb] = dB_fixed2(phy_measurements->subband_cqi[UE_id][aarx][rb],
          2*phy_measurements->n0_power[aarx]);
      // 2*n0_power since we have noise from the odd carriers in the comb of the srs

      //    msg("subband_cqi[%d][%d][%d] => %d (%d dB)\n",eNB_id,aarx,rb,phy_measurements->subband_cqi[eNB_id][aarx][rb],phy_measurements->subband_cqi_dB[eNB_id][aarx][rb]);
    }

  }


  for (rb=0; rb<frame_parms->N_RB_DL; rb++) {
    phy_measurements->subband_cqi_tot_dB[UE_id][rb] = dB_fixed2(phy_measurements->subband_cqi_tot[UE_id][rb],
        phy_measurements->n0_power_tot);
    /*
    if (phy_measurements->subband_cqi_tot_dB[UE_id][rb] == 65)
      msg("eNB meas error *****subband_cqi_tot[%d][%d] %d => %d dB (n0 %d)\n",UE_id,rb,phy_measurements->subband_cqi_tot[UE_id][rb],phy_measurements->subband_cqi_tot_dB[UE_id][rb],phy_measurements->n0_power_tot);
    */
  }

}

void lte_eNB_I0_measurements_emul(PHY_VARS_eNB *phy_vars_eNb,
                                  uint8_t sect_id)
{

  LOG_D(PHY,"EMUL lte_eNB_IO_measurements_emul: eNB %d, sect %d\n",phy_vars_eNb->Mod_id,sect_id);
}






