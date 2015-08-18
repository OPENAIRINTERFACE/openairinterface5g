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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
#ifdef USER_MODE
#include <string.h>
#endif
#include "defs.h"
#include "PHY/defs.h"
#include "filt12_32.h"
//#define DEBUG_CH

/*To be accomplished*/
int lte_dl_bf_channel_estimation(PHY_VARS_UE *phy_vars_ue,
                                 uint8_t eNB_id,
                                 uint8_t eNB_offset,
                                 unsigned char Ns,
                                 unsigned char p,
                                 unsigned char symbol)
{
  
  unsigned char aarx;
  int uespec_pilot[9][200];
  short *pil, *rxF;

  //LTE_UE_PDSCH *lte_ue_pdsch_vars = phy_vars_ue->lte_ue_pdsch_vars[eNB_id];
  int **rxdataF = phy_vars_ue->lte_ue_common_vars.rxdataF;
  int32_t **dl_bf_ch_estimates = phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_bf_ch_estimates;

  int beamforming_mode = phy_vars_ue->transmission_mode>7?phy_vars_ue->transmission_mode : 0;
  
  // define interpolation filters
  //....

  //ch_offset     = phy_vars_ue->lte_frame_parms.ofdm_symbol_size*symbol;

  //generate ue specific pilots
  if(beamforming_mode==7)
    lte_dl_ue_spec_rx(phy_vars_ue,&uespec_pilot[p-5][0],Ns,p,0);
  else if (beamforming_mode>7)
    lte_dl_ue_spec_rx(phy_vars_ue,&uespec_pilot[p-6][0],Ns,p,0);
  else if (beamforming_mode==0)
    msg("No beamforming is performed.\n");
  else
    msg("Beamforming mode not supported yet.\n");
  

  if(beamforming_mode==7) {
   
    for (aarx=0; aarx<phy_vars_ue->lte_frame_parms.nb_antennas_rx;aarx++) {

    /*  pil   = (short *)&uespec_pilot[0][0];
      rxF   = (short *)&rxdataF_uespec[aarx][(symbol-1)/3*frame_parms->N_RB_DL*(3+frame_parms->Ncp)];
      dl_ch = (short *)&dl_bf_ch_estimates[aarx][ch_offset];

      memset(dl_ch,0,4*(phy_vars_ue->lte_frame_parms.ofdm_symbol_size));
      //estimation and interpolation */
    }

  } else if (beamforming_mode==0){
    msg("No beamforming is performed.\n");
    return(-1);
  } else {
    msg("Beamforming mode is not supported yet.\n");
    return(-1);
  }
  
  //temporal interpolation

  return(0);

}
