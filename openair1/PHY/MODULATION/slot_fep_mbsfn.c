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

int slot_fep_mbsfn(PHY_VARS_UE *phy_vars_ue,
                   unsigned char l,
                   int subframe,
                   int sample_offset,
                   int no_prefix)
{

  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_ue->lte_frame_parms;
  LTE_UE_COMMON *ue_common_vars   = &phy_vars_ue->lte_ue_common_vars;
  uint8_t eNB_id = 0;//ue_common_vars->eNb_id;

  unsigned char aa;
  unsigned char frame_type = frame_parms->frame_type; // Frame Type: 0 - FDD, 1 - TDD;
  unsigned int nb_prefix_samples = frame_parms->ofdm_symbol_size>>2;//(no_prefix ? 0 : frame_parms->nb_prefix_samples);
  unsigned int nb_prefix_samples0 = frame_parms->ofdm_symbol_size>>2;//(no_prefix ? 0 : frame_parms->nb_prefix_samples0);
  unsigned int subframe_offset;

  //   int i;
  unsigned int frame_length_samples = frame_parms->samples_per_tti * 10;
  void (*dft)(int16_t *,int16_t *, int);

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
    subframe_offset = frame_parms->ofdm_symbol_size * frame_parms->symbols_per_tti * subframe;

  } else {
    subframe_offset = frame_parms->samples_per_tti * subframe;

  }


  if (l<0 || l>=12) {
    msg("slot_fep_mbsfn: l must be between 0 and 11\n");
    return(-1);
  }

  if (((subframe == 0) || (subframe == 5) ||    // SFn 0,4,5,9;
       (subframe == 4) || (subframe == 9))
      && (frame_type==FDD) )    {   //check for valid MBSFN subframe
    msg("slot_fep_mbsfn: Subframe must be 1,2,3,6,7,8 for FDD, Got %d \n",subframe);
    return(-1);
  } else if (((subframe == 0) || (subframe == 1) || (subframe==2) ||  // SFn 0,4,5,9;
              (subframe == 5) || (subframe == 6))
             && (frame_type==TDD) )   {   //check for valid MBSFN subframe
    msg("slot_fep_mbsfn: Subframe must be 3,4,7,8,9 for TDD, Got %d \n",subframe);
    return(-1);
  }

#ifdef DEBUG_FEP
  msg("slot_fep_mbsfn: subframe %d, symbol %d, nb_prefix_samples %d, nb_prefix_samples0 %d, subframe_offset %d, sample_offset %d\n", subframe, l, nb_prefix_samples,nb_prefix_samples0,subframe_offset,
      sample_offset);
#endif


  for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
    memset(&ue_common_vars->rxdataF[aa][2*frame_parms->ofdm_symbol_size*l],0,2*frame_parms->ofdm_symbol_size*sizeof(int));

    if (l==0) {
      start_meas(&phy_vars_ue->rx_dft_stats);
      dft((int16_t *)&ue_common_vars->rxdata[aa][(sample_offset +
          nb_prefix_samples0 +
          subframe_offset -
          SOFFSET) % frame_length_samples],
          (int16_t *)&ue_common_vars->rxdataF[aa][frame_parms->ofdm_symbol_size*l],1);
      stop_meas(&phy_vars_ue->rx_dft_stats);
    } else {
      if ((sample_offset +
           (frame_parms->ofdm_symbol_size+nb_prefix_samples0+nb_prefix_samples) +
           (frame_parms->ofdm_symbol_size+nb_prefix_samples)*(l-1) +
           subframe_offset-
           SOFFSET) > (frame_length_samples - frame_parms->ofdm_symbol_size))
        memcpy((short *)&ue_common_vars->rxdata[aa][frame_length_samples],
               (short *)&ue_common_vars->rxdata[aa][0],
               frame_parms->ofdm_symbol_size*sizeof(int));

      start_meas(&phy_vars_ue->rx_dft_stats);
      dft((int16_t *)&ue_common_vars->rxdata[aa][(sample_offset +
          (frame_parms->ofdm_symbol_size+nb_prefix_samples0+nb_prefix_samples) +
          (frame_parms->ofdm_symbol_size+nb_prefix_samples)*(l-1) +
          subframe_offset-
          SOFFSET) % frame_length_samples],
          (int16_t *)&ue_common_vars->rxdataF[aa][frame_parms->ofdm_symbol_size*l],1);
      stop_meas(&phy_vars_ue->rx_dft_stats);
    }

  }

  //if ((l==0) || (l==(4-frame_parms->Ncp))) {
  // changed to invoke MBSFN channel estimation in symbols 2,6,10
  if ((l==2)||(l==6)||(l==10)) {
    for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
      if (phy_vars_ue->perfect_ce == 0) {
#ifdef DEBUG_FEP
        msg("Channel estimation eNB %d, aatx %d, subframe %d, symbol %d\n",eNB_id,aa,subframe,l);
#endif

        lte_dl_mbsfn_channel_estimation(phy_vars_ue,
                                        eNB_id,
                                        0,
                                        subframe,
                                        l);
        /*   for (i=0;i<phy_vars_ue->PHY_measurements.n_adj_cells;i++) {
        lte_dl_mbsfn_channel_estimation(phy_vars_ue,
               eNB_id,
             i+1,
               subframe,
               l);
             lte_dl_channel_estimation(phy_vars_ue,eNB_id,0,
           Ns,
           aa,
           l,
           symbol);
           for (i=0;i<phy_vars_ue->PHY_measurements.n_adj_cells;i++) {
        lte_dl_channel_estimation(phy_vars_ue,eNB_id,i+1,
             Ns,
             aa,
             l,
             symbol); */
        //  }

        // do frequency offset estimation here!
        // use channel estimates from current symbol (=ch_t) and last symbol (ch_{t-1})
#ifdef DEBUG_FEP
        msg("Frequency offset estimation\n");
#endif
        // if ((l == 0) || (l==(4-frame_parms->Ncp)))
        /*    if ((l==2)||(l==6)||(l==10))
          lte_mbsfn_est_freq_offset(ue_common_vars->dl_ch_estimates[0],
                  frame_parms,
                  l,
                  &ue_common_vars->freq_offset); */
      }
    }
  }

#ifdef DEBUG_FEP
  msg("slot_fep_mbsfn: done\n");
#endif
  return(0);
}
