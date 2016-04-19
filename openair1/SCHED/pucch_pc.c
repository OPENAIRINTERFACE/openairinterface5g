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

/*! \file pusch_pc.c
 * \brief Implementation of UE PUSCH Power Control procedures from 36.213 LTE specifications (Section
 * \author R. Knopp
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs.h"
#include "PHY/LTE_TRANSPORT/proto.h"
#include "PHY/extern.h"

int8_t pucch_power_cntl(PHY_VARS_UE *phy_vars_ue,uint8_t subframe,uint8_t eNB_id,PUCCH_FMT_t pucch_fmt)
{

  int8_t Po_PUCCH;
  //uint8_t harq_pid;

  // P_pucch =  P_opucch+ PL + h(nCQI,nHARQ) + delta_pucchF(pucch_fmt) + g(i))
  //
  //if ((pucch_fmt == pucch_format1a) ||
  //    (pucch_fmt == pucch_format1b)) {  // Update g_pucch based on TPC/delta_PUCCH received in PDCCH for this process
    //harq_pid = phy_vars_ue->dlsch_ue[eNB_id][0]->harq_ack[subframe].harq_id;
    //this is now done in dci_tools
    //phy_vars_ue->g_pucch[eNB_id] += phy_vars_ue->dlsch_ue[eNB_id][0]->harq_processes[harq_pid]->delta_PUCCH;
  //}

  Po_PUCCH = get_PL(phy_vars_ue->Mod_id,phy_vars_ue->CC_id,eNB_id)+
    phy_vars_ue->lte_frame_parms.ul_power_control_config_common.p0_NominalPUCCH+
    phy_vars_ue->dlsch_ue[eNB_id][0]->g_pucch;

  switch (pucch_fmt) {
  case pucch_format1:
  case pucch_format2a:
  case pucch_format2b:
    Po_PUCCH += (-2+(phy_vars_ue->lte_frame_parms.ul_power_control_config_common.deltaF_PUCCH_Format1<<1));
    break;

  case pucch_format1a:
  case pucch_format1b:
    Po_PUCCH += (1+(phy_vars_ue->lte_frame_parms.ul_power_control_config_common.deltaF_PUCCH_Format1b<<1));
    break;

  case pucch_format2:
    switch (phy_vars_ue->lte_frame_parms.ul_power_control_config_common.deltaF_PUCCH_Format2a) {
    case 0:
      Po_PUCCH -= 2;
      break;

    case 2:
      Po_PUCCH += 1;
      break;

    case 3:
      Po_PUCCH += 2;
      break;

    case 1:
    default:
      break;
    }

    break;
  }

  if (pucch_fmt!=pucch_format1) {
    LOG_I(PHY,"[UE  %d][PDSCH %x] frame %d, subframe %d: Po_PUCCH %d dBm : Po_NOMINAL_PUCCH %d dBm, PL %d dB, g_pucch %d dB\n",
          phy_vars_ue->Mod_id,
          phy_vars_ue->dlsch_ue[eNB_id][0]->rnti,phy_vars_ue->frame_tx,subframe,
          Po_PUCCH,
          phy_vars_ue->lte_frame_parms.ul_power_control_config_common.p0_NominalPUCCH,
          get_PL(phy_vars_ue->Mod_id,phy_vars_ue->CC_id,eNB_id),
          phy_vars_ue->dlsch_ue[eNB_id][0]->g_pucch);
  } else {
    LOG_I(PHY,"[UE  %d][SR %x] frame %d, subframe %d: Po_PUCCH %d dBm : Po_NOMINAL_PUCCH %d dBm, PL %d dB g_pucch %d dB\n",
          phy_vars_ue->Mod_id,
          phy_vars_ue->dlsch_ue[eNB_id][0]->rnti,phy_vars_ue->frame_tx,subframe,
          Po_PUCCH,
          phy_vars_ue->lte_frame_parms.ul_power_control_config_common.p0_NominalPUCCH,
          get_PL(phy_vars_ue->Mod_id,phy_vars_ue->CC_id,eNB_id),
          phy_vars_ue->dlsch_ue[eNB_id][0]->g_pucch);
  }

  return(Po_PUCCH);
}
