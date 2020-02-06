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

/*! \file nr_l1_helper.c
* \brief PHY helper function adapted to NR
* \author Guido Casati
* \date 2019
* \version 1.0
* \email guido.casati@iis.fraunhofer.de
* @ingroup _mac

*/

#include "PHY/defs_common.h" // TBR
#include "PHY/defs_nr_common.h"

#include "mac_defs.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"

/* TS 38.321 subclause 7.3 - return DELTA_PREAMBLE values in dB */
int8_t nr_get_DELTA_PREAMBLE(module_id_t mod_id, int CC_id){

    NR_UE_MAC_INST_t *nrUE_mac_inst = get_mac_inst(mod_id);
    NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = nrUE_mac_inst->nr_rach_ConfigCommon;

    AssertFatal(CC_id == 0, "Transmission on secondary CCs is not supported yet\n");

    uint8_t preambleFormat, prachConfigIndex;
    lte_frame_type_t frame_type = TDD; // TODO TBR retrieve frame type. Currently hardcoded to TDD.
    nr_frequency_range_e fr = nr_FR1;  // TODO TBR retrieve frame type. Currently hardcoded to FR1.
    int mu; 

    // SCS configuration from msg1_SubcarrierSpacing and table 4.2-1 in TS 38.211

    NR_SubcarrierSpacing_t scs = nr_rach_ConfigCommon->msg1_SubcarrierSpacing;

    switch (scs){
      case NR_SubcarrierSpacing_kHz15:
      mu = 0;
      break;

      case NR_SubcarrierSpacing_kHz30:
      mu = 1;
      break;

      case NR_SubcarrierSpacing_kHz60:
      mu = 2;
      break;

      case NR_SubcarrierSpacing_kHz120:
      mu = 3;
      break;

      case NR_SubcarrierSpacing_kHz240:
      mu = 4;
      break;

      case NR_SubcarrierSpacing_spare3:
      mu = 5;
      break;

      case NR_SubcarrierSpacing_spare2:
      mu = 6;
      break;

      case NR_SubcarrierSpacing_spare1:
      mu = 7;
      break;

      default:
      AssertFatal(1 == 0,"Unknown msg1_SubcarrierSpacing %d\n", scs);
    }

    // Preamble formats given by prach_ConfigurationIndex and tables 6.3.3.2-2 and 6.3.3.2-2 in TS 38.211

    prachConfigIndex = nr_rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;
    preambleFormat = get_nr_prach_fmt(prachConfigIndex,frame_type,fr);

    switch (preambleFormat) {
      // long preamble formats
      case 0:
      case 3:
      return  0;

      case 1:           
      return -3;

      case 2:
      return -6;

      // short preamble formats
      case 0xa1:
      case 0xb1:
      return 8 + 3*mu;

      case 0xa2:
      case 0xb2:
      case 0xc2:
      return 5 + 3*mu;

      case 0xa3:
      case 0xb3:
      return 3 + 3*mu;

      case 0xb4:      
      return 3*mu;

      case 0xc0:
      return 5 + 3*mu;

      default:
      AssertFatal(1 == 0, "[UE %d] ue_procedures.c: FATAL, Illegal preambleFormat %d, prachConfigIndex %d\n", mod_id, preambleFormat, prachConfigIndex);
    }
    return;
}

/* TS 38.321 subclause 5.1.3 - RA preamble transmission - ra_PREAMBLE_RECEIVED_TARGET_POWER configuration */
int8_t nr_get_Po_NOMINAL_PUSCH(module_id_t mod_id, uint8_t CC_id){
  
  NR_UE_MAC_INST_t *nr_UE_mac_inst = get_mac_inst(mod_id);
  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = nr_UE_mac_inst->nr_rach_ConfigCommon;
  NR_PRACH_RESOURCES_t *prach_resources = &nr_UE_mac_inst->RA_prach_resources;

  AssertFatal(nr_rach_ConfigCommon != NULL, "[UE %d] CCid %d FATAL nr_rach_ConfigCommon is NULL !!!\n", mod_id, CC_id);

  int8_t receivedTargerPower = nr_rach_ConfigCommon->rach_ConfigGeneric.preambleReceivedTargetPower + nr_get_DELTA_PREAMBLE(mod_id, CC_id) + (nr_UE_mac_inst->RA_PREAMBLE_POWER_RAMPING_COUNTER - 1) * prach_resources->RA_PREAMBLE_POWER_RAMPING_STEP;

  return receivedTargerPower;
}