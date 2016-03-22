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
/*! \file l1_helper.c
* \brief phy helper function
* \author Navid Nikaein, Raymond Knopp
* \date 2012 - 2014
* \version 1.0
* \email navid.nikaein@eurecom.fr
* @ingroup _mac

*/

#include "defs.h"
#include "extern.h"
#include "UTIL/LOG/log.h"
#include "proto.h"

int8_t get_Po_NOMINAL_PUSCH(module_id_t module_idP,uint8_t CC_id)
{
  RACH_ConfigCommon_t *rach_ConfigCommon = NULL;

  if (CC_id>0) {
    LOG_E(MAC,"Transmission on secondary CCs is not supported yet\n");
    //mac_xface->macphy_exit("MAC FATAL  CC_id>0");
    return 0; 
  }

  if (UE_mac_inst[module_idP].radioResourceConfigCommon) {
    rach_ConfigCommon = &UE_mac_inst[module_idP].radioResourceConfigCommon->rach_ConfigCommon;
  }
  else {
    LOG_E(MAC,"[UE %d] CCid %d FATAL radioResourceConfigCommon is NULL !!!\n",module_idP,CC_id);
    //mac_xface->macphy_exit("FATAL radioResourceConfigCommon is NULL");
    return 0;
  }

  return(-120 + (rach_ConfigCommon->powerRampingParameters.preambleInitialReceivedTargetPower<<1) +
         get_DELTA_PREAMBLE(module_idP,CC_id));
}

int8_t get_deltaP_rampup(module_id_t module_idP,uint8_t CC_id)
{

  if (CC_id>0) {
    LOG_E(MAC,"Transmission on secondary CCs is not supported yet\n");
    mac_xface->macphy_exit("MAC FATAL  CC_id>0");
    return 0; // not reached
  }

  LOG_D(MAC,"[PUSCH]%d dB\n",UE_mac_inst[module_idP].RA_PREAMBLE_TRANSMISSION_COUNTER<<1);
  return((int8_t)(UE_mac_inst[module_idP].RA_PREAMBLE_TRANSMISSION_COUNTER<<1));

}
