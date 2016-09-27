/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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
