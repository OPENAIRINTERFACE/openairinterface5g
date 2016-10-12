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

/*! \file srs_pc.c
 * \brief Implementation of UE SRS Power Control procedures from 36.213 LTE specifications (Section
 * \author H. Bilel
 * \date 2016
 * \version 0.1
 * \company TCL
 * \email: haithem.bilel@alcatelOneTouch.com
 * \note
 * \warning
 */

#include "PHY/defs.h"
#include "PHY/LTE_TRANSPORT/proto.h"
#include "PHY/extern.h"

void srs_power_cntl(PHY_VARS_UE *ue,UE_rxtx_proc_t *proc,uint8_t eNB_id,uint8_t *pnb_rb_srs, uint8_t abstraction_flag)
{

  int16_t PL;
  int8_t  P_opusch;
  int8_t  Psrs_offset;
  int16_t P_srs;
  int16_t f_pusch;
  uint8_t alpha;
  uint8_t Msrs = 0;
  

  SOUNDINGRS_UL_CONFIG_DEDICATED *psoundingrs_ul_config_dedicated = &ue->soundingrs_ul_config_dedicated[eNB_id];
  LTE_DL_FRAME_PARMS             *pframe_parms                    = &ue->frame_parms;
  
  uint8_t Bsrs  = psoundingrs_ul_config_dedicated->srs_Bandwidth;
  uint8_t Csrs  = pframe_parms->soundingrs_ul_config_common.srs_BandwidthConfig;
  LOG_I(PHY," SRS Power Control; AbsSubframe %d.%d, eNB_id %d, N_RB_UL %d, srs_Bandwidth %d, srs_BandwidthConfig %d \n",proc->frame_tx,proc->subframe_tx,eNB_id,pframe_parms->N_RB_UL,Bsrs,Csrs);
  
  if (pframe_parms->N_RB_UL < 41)
  {
    Msrs    = Nb_6_40[Csrs][Bsrs];
  } 
  else if (pframe_parms->N_RB_UL < 61)
  {
    Msrs    = Nb_41_60[Csrs][Bsrs];
  } 
  else if (pframe_parms->N_RB_UL < 81)
  {
    Msrs    = Nb_61_80[Csrs][Bsrs];
  } 
  else if (pframe_parms->N_RB_UL <111)
  {
    Msrs    = Nb_81_110[Csrs][Bsrs];
  }

  // SRS Power control 36.213 5.1.3.1
  // P_srs   =  P_srs_offset+ 10log10(Msrs) + P_opusch(j) + alpha*PL + f(i))

  Psrs_offset = (ue->ul_power_control_dedicated[eNB_id].pSRS_Offset - 3);
  P_opusch    = ue->frame_parms.ul_power_control_config_common.p0_NominalPUSCH;
  f_pusch     = ue->ulsch[eNB_id]->f_pusch;
  alpha       = alpha_lut[ue->frame_parms.ul_power_control_config_common.alpha];
  PL          = get_PL(ue->Mod_id,ue->CC_id,eNB_id);

  LOG_I(PHY," SRS Power Control; eNB_id %d, p0_NominalPUSCH %d, alpha %d \n",eNB_id,P_opusch,alpha);
  LOG_I(PHY," SRS Power Control; eNB_id %d, pSRS_Offset[dB] %d, Msrs %d, PL %d, f_pusch %d \n",eNB_id,Psrs_offset,Msrs,PL,f_pusch);

  P_srs  = P_opusch + Psrs_offset + f_pusch;
  P_srs += (((int32_t)alpha * (int32_t)PL) + hundred_times_log10_NPRB[Msrs-1])/100 ;
  
  ue->ulsch[eNB_id]->Po_SRS = P_srs < ue->tx_power_max_dBm ? P_srs:ue->tx_power_max_dBm; // MIN(PcMax,Psrs)

  pnb_rb_srs[0]             = Msrs;
  LOG_I(PHY," SRS Power Control; eNB_id %d, P_srs[dBm] %d, P_cmax[dBm] %d, Psrs[dBm] %d\n",eNB_id,P_srs,ue->tx_power_max_dBm,ue->ulsch[eNB_id]->Po_SRS);
}
