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
#include "defs.h"
//#define DEBUG_FEP

#define SOFFSET 0


int slot_fep(PHY_VARS_UE *phy_vars_ue,
             unsigned char l,
             unsigned char Ns,
             int sample_offset,
             int no_prefix,
	     int reset_freq_est)
{

  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_ue->lte_frame_parms;
  LTE_UE_COMMON *ue_common_vars   = &phy_vars_ue->lte_ue_common_vars;
  uint8_t eNB_id = 0;//ue_common_vars->eNb_id;
  unsigned char aa;
  unsigned char symbol = l+((7-frame_parms->Ncp)*(Ns&1)); ///symbol within sub-frame
  unsigned int nb_prefix_samples = (no_prefix ? 0 : frame_parms->nb_prefix_samples);
  unsigned int nb_prefix_samples0 = (no_prefix ? 0 : frame_parms->nb_prefix_samples0);
  unsigned int subframe_offset;//,subframe_offset_F;
  unsigned int slot_offset;
  int i;
  unsigned int frame_length_samples = frame_parms->samples_per_tti * 10;
  unsigned int rx_offset;

  void (*dft)(int16_t *,int16_t *, int);
  int tmp_dft_in[2048] __attribute__ ((aligned (32)));  // This is for misalignment issues for 6 and 15 PRBs

  switch (frame_parms->ofdm_symbol_size) {
  case 128:
    dft = dft128;
    break;

  case 256:
    dft = dft256;
    break;

  case 512:
    dft = dft512;
    break;

  case 1024:
    dft = dft1024;
    break;

  case 1536:
    dft = dft1536;
    break;

  case 2048:
    dft = dft2048;
    break;

  default:
    dft = dft512;
    break;
  }

  if (no_prefix) {
    subframe_offset = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_tti * (Ns>>1);
    slot_offset = frame_parms->ofdm_symbol_size * (frame_parms->symbols_per_tti>>1) * (Ns%2);
  } else {
    subframe_offset = frame_parms->samples_per_tti * (Ns>>1);
    slot_offset = (frame_parms->samples_per_tti>>1) * (Ns%2);
  }

  //  subframe_offset_F = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_tti * (Ns>>1);


  if (l<0 || l>=7-frame_parms->Ncp) {
    printf("slot_fep: l must be between 0 and %d\n",7-frame_parms->Ncp);
    return(-1);
  }

  if (Ns<0 || Ns>=20) {
    printf("slot_fep: Ns must be between 0 and 19\n");
    return(-1);
  }



  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
    memset(&ue_common_vars->rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],0,frame_parms->ofdm_symbol_size*sizeof(int));

    rx_offset = sample_offset + slot_offset + nb_prefix_samples0 + subframe_offset - SOFFSET;
    // Align with 256 bit
    //    rx_offset = rx_offset&0xfffffff8;

#ifdef DEBUG_FEP
    //  if (phy_vars_ue->frame <100)
    printf("slot_fep: frame %d: slot %d, symbol %d, nb_prefix_samples %d, nb_prefix_samples0 %d, slot_offset %d, subframe_offset %d, sample_offset %d,rx_offset %d\n", phy_vars_ue->frame_rx,Ns, symbol,
        nb_prefix_samples,nb_prefix_samples0,slot_offset,subframe_offset,sample_offset,rx_offset);
#endif

    if (l==0) {

      if (rx_offset > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((short *)&ue_common_vars->rxdata[aa][frame_length_samples],
               (short *)&ue_common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));

      if ((rx_offset&7)!=0) {  // if input to dft is not 256-bit aligned, issue for size 6,15 and 25 PRBs
        memcpy((void *)tmp_dft_in,
               (void *)&ue_common_vars->rxdata[aa][rx_offset % frame_length_samples],
               frame_parms->ofdm_symbol_size*sizeof(int));
        dft((int16_t *)tmp_dft_in,
            (int16_t *)&ue_common_vars->rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly
        start_meas(&phy_vars_ue->rx_dft_stats);

        dft((int16_t *)&ue_common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&ue_common_vars->rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
        stop_meas(&phy_vars_ue->rx_dft_stats);

      }
    } else {
      rx_offset += (frame_parms->ofdm_symbol_size+nb_prefix_samples)*l;// +
      //                   (frame_parms->ofdm_symbol_size+nb_prefix_samples)*(l-1);

#ifdef DEBUG_FEP
      //  if (phy_vars_ue->frame <100)
      printf("slot_fep: frame %d: slot %d, symbol %d, nb_prefix_samples %d, nb_prefix_samples0 %d, slot_offset %d, subframe_offset %d, sample_offset %d,rx_offset %d\n", phy_vars_ue->frame_rx,Ns, symbol,
          nb_prefix_samples,nb_prefix_samples0,slot_offset,subframe_offset,sample_offset,rx_offset);
#endif

      if (rx_offset > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((void *)&ue_common_vars->rxdata[aa][frame_length_samples],
               (void *)&ue_common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));

      start_meas(&phy_vars_ue->rx_dft_stats);

      if ((rx_offset&7)!=0) {  // if input to dft is not 128-bit aligned, issue for size 6 and 15 PRBs
        memcpy((void *)tmp_dft_in,
               (void *)&ue_common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
               frame_parms->ofdm_symbol_size*sizeof(int));
        dft((int16_t *)tmp_dft_in,
            (int16_t *)&ue_common_vars->rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      } else { // use dft input from RX buffer directly

        dft((int16_t *)&ue_common_vars->rxdata[aa][(rx_offset) % frame_length_samples],
            (int16_t *)&ue_common_vars->rxdataF[aa][frame_parms->ofdm_symbol_size*symbol],1);
      }

      stop_meas(&phy_vars_ue->rx_dft_stats);


    }

  }

  if (phy_vars_ue->perfect_ce == 0) {
    if ((l==0) || (l==(4-frame_parms->Ncp))) {
      for (aa=0; aa<frame_parms->nb_antennas_tx_eNB; aa++) {

#ifdef DEBUG_FEP
        printf("Channel estimation eNB %d, aatx %d, slot %d, symbol %d\n",eNB_id,aa,Ns,l);
#endif
        start_meas(&phy_vars_ue->dlsch_channel_estimation_stats);
        lte_dl_channel_estimation(phy_vars_ue,eNB_id,0,
                                  Ns,
                                  aa,
                                  l,
                                  symbol);
        stop_meas(&phy_vars_ue->dlsch_channel_estimation_stats);

        for (i=0; i<phy_vars_ue->PHY_measurements.n_adj_cells; i++) {
          lte_dl_channel_estimation(phy_vars_ue,eNB_id,i+1,
                                    Ns,
                                    aa,
                                    l,
                                    symbol);
        }
      }


      // do frequency offset estimation here!
      // use channel estimates from current symbol (=ch_t) and last symbol (ch_{t-1})
#ifdef DEBUG_FEP
      printf("Frequency offset estimation\n");
#endif

      if (l==(4-frame_parms->Ncp)) {
        start_meas(&phy_vars_ue->dlsch_freq_offset_estimation_stats);
        lte_est_freq_offset(ue_common_vars->dl_ch_estimates[0],
                            frame_parms,
                            l,
                            &ue_common_vars->freq_offset,
			    reset_freq_est);
        stop_meas(&phy_vars_ue->dlsch_freq_offset_estimation_stats);

      }
    }
  }

#ifdef DEBUG_FEP
  printf("slot_fep: done\n");
#endif
  return(0);
}
