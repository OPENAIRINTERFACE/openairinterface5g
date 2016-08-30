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

#include "PHY/types.h"
#include "PHY/defs.h"

PHY_VARS_eNB* init_lte_eNB(LTE_DL_FRAME_PARMS *frame_parms,
                           uint8_t eNB_id,
                           uint8_t Nid_cell,
                           uint8_t cooperation_flag,
                           uint8_t abstraction_flag);

PHY_VARS_UE* init_lte_UE(LTE_DL_FRAME_PARMS *frame_parms,
                         uint8_t UE_id,
                         uint8_t abstraction_flag);

PHY_VARS_RN* init_lte_RN(LTE_DL_FRAME_PARMS *frame_parms,
                         uint8_t RN_id,
                         uint8_t eMBMS_active_state);

void init_lte_vars(LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
                   uint8_t frame_type,
                   uint8_t tdd_config,
                   uint8_t tdd_config_S,
                   uint8_t extended_prefix_flag,
                   uint8_t N_RB_DL,
                   uint16_t Nid_cell,
                   uint8_t cooperation_flag,
                   uint8_t nb_antenna_ports,
                   uint8_t abstraction_flag,
                   int nb_antennas_rx,
                   int nb_antennas_tx,
                   int nb_antennas_rx_ue,
                   uint8_t eMBMS_active_state);
