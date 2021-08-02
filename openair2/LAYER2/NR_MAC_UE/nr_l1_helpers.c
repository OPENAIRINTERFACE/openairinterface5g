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
* \brief PHY/MAC helper functions
* \author Guido Casati
* \date 2019
* \version 2.0
* \email guido.casati@iis.fraunhofer.de
* @ingroup _mac

*/

#include "PHY/defs_nr_common.h"
//#include "PHY/impl_defs_top.h"

#include "mac_defs.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"
#include "NR_P-Max.h"

/* TS 38.321 subclause 7.3 - return DELTA_PREAMBLE values in dB */
int8_t nr_get_DELTA_PREAMBLE(module_id_t mod_id, int CC_id, uint16_t prach_format){

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = (mac->scc!=NULL) ? 
    mac->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup:
    mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup;
  NR_SubcarrierSpacing_t scs = *nr_rach_ConfigCommon->msg1_SubcarrierSpacing;
  int prach_sequence_length = (mac->scc!=NULL)?(mac->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present - 1) : (mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present-1);
  uint8_t prachConfigIndex, mu;

  AssertFatal(CC_id == 0, "Transmission on secondary CCs is not supported yet\n");

  // SCS configuration from msg1_SubcarrierSpacing and table 4.2-1 in TS 38.211

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
    AssertFatal(1 == 0,"Unknown msg1_SubcarrierSpacing %lu\n", scs);
  }

  // Preamble formats given by prach_ConfigurationIndex and tables 6.3.3.2-2 and 6.3.3.2-2 in TS 38.211

  prachConfigIndex = nr_rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;

  if (prach_sequence_length == 0) {
    AssertFatal(prach_format < 4, "Illegal PRACH format %d for sequence length 839\n", prach_format);
    switch (prach_format) {

      // long preamble formats
      case 0:
      case 3:
      return  0;

      case 1:           
      return -3;

      case 2:
      return -6;
    }
  } else {
    switch (prach_format) { // short preamble formats
      case 0:
      case 3:
      return 8 + 3*mu;

      case 1:
      case 4:
      case 8:
      return 5 + 3*mu;

      case 2:
      case 5:
      return 3 + 3*mu;

      case 6:
      return 3*mu;

      case 7:
      return 5 + 3*mu;

      default:
      AssertFatal(1 == 0, "[UE %d] ue_procedures.c: FATAL, Illegal preambleFormat %d, prachConfigIndex %d\n", mod_id, prach_format, prachConfigIndex);
    }
  }
  return 0;
}

// TS 38.321 subclause 5.1.3 - RA preamble transmission - ra_PREAMBLE_RECEIVED_TARGET_POWER configuration
// Measurement units:
// - preambleReceivedTargetPower      dBm (-202..-60, 2 dBm granularity)
// - delta_preamble                   dB
// - RA_PREAMBLE_POWER_RAMPING_STEP   dB
// - POWER_OFFSET_2STEP_RA            dB
// returns receivedTargerPower in dBm
int nr_get_Po_NOMINAL_PUSCH(NR_PRACH_RESOURCES_t *prach_resources, module_id_t mod_id, uint8_t CC_id){
  
  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  int8_t receivedTargerPower;
  int8_t delta_preamble;

  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = (mac->scc != NULL) ? mac->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup: mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup;
  long preambleReceivedTargetPower = nr_rach_ConfigCommon->rach_ConfigGeneric.preambleReceivedTargetPower;
  delta_preamble = nr_get_DELTA_PREAMBLE(mod_id, CC_id, prach_resources->prach_format);

  receivedTargerPower = preambleReceivedTargetPower + delta_preamble + (prach_resources->RA_PREAMBLE_POWER_RAMPING_COUNTER - 1) * prach_resources->RA_PREAMBLE_POWER_RAMPING_STEP + prach_resources->POWER_OFFSET_2STEP_RA;

  LOG_D(MAC, "In %s: receivedTargerPower is %d dBm \n", __FUNCTION__, receivedTargerPower);

  return receivedTargerPower;
}

void get_num_re_dmrs(nfapi_nr_ue_pusch_pdu_t *pusch_pdu,
                     uint8_t *nb_dmrs_re_per_rb,
                     uint16_t *number_dmrs_symbols){

  int start_symbol          = pusch_pdu->start_symbol_index;
  uint8_t number_of_symbols = pusch_pdu->nr_of_symbols;
  uint16_t ul_dmrs_symb_pos = pusch_pdu->ul_dmrs_symb_pos;
  uint8_t dmrs_type         = pusch_pdu->dmrs_config_type;
  uint8_t cdm_grps_no_data  = pusch_pdu->num_dmrs_cdm_grps_no_data;

  *number_dmrs_symbols = 0;
  for (int i = start_symbol; i < start_symbol + number_of_symbols; i++) {
    if((ul_dmrs_symb_pos >> i) & 0x01)
      *number_dmrs_symbols += 1;
  }

  *nb_dmrs_re_per_rb = ((dmrs_type == pusch_dmrs_type1) ? 6:4)*cdm_grps_no_data;

}

// Implementation of 6.2.4 Configured ransmitted power
// 3GPP TS 38.101-1 version 16.5.0 Release 16
// -
// The UE is allowed to set its configured maximum output power PCMAX,f,c for carrier f of serving cell c in each slot.
// The configured maximum output power PCMAX,f,c is set within the following bounds: PCMAX_L,f,c <=  PCMAX,f,c <=  PCMAX_H,f,c
// -
// Measurement units:
// - p_max:              dBm
// - delta_TC_c:         dB
// - P_powerclass:       dBm
// - delta_P_powerclass: dB
// - MPR_c:              dB
// - delta_MPR_c:        dB
// - delta_T_IB_c        dB
// - delta_rx_SRS        dB
// note:
// - Assuming:
// -- Powerclass 3 capable UE (which is default power class unless otherwise stated)
// -- Maximum power reduction (MPR_c) for power class 3, QPSK, inner RB allocations
// -- no additional MPR (A_MPR_c)
// todo:
// - in current implementation delta_P_powerclass is not handling power classes different from 3
long nr_get_Pcmax(module_id_t mod_id){

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  uint32_t band = (mac->scc!=NULL) ? *mac->scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0] :
    *mac->scc_SIB->downlinkConfigCommon.frequencyInfoDL.frequencyBandList.list.array[0]->freqBandIndicatorNR;
  NR_P_Max_t p_max           = 0;
  uint8_t P_powerclass       = 23;
  uint8_t delta_P_powerclass = 0;
  uint8_t MPR_c              = 1.5;
  uint8_t delta_MPR_c        = 0;
  uint8_t A_MPR_c            = 0;
  uint8_t delta_T_IB_c       = 0;
  uint8_t delta_TC_c         = 0;
  uint8_t delta_rx_SRS       = 0;
  uint8_t P_MPR_c            = 0;
  long P_cmax_l              = 0;
  long P_cmax_h              = 0;
  long P_cmax                = 0;

  if (band == 28 && mac->phy_config.config_req.carrier_config.uplink_bandwidth == 30){
    delta_MPR_c = 0.5;
  }

  if (mac->cg && mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->ext1){
    if (*mac->cg->spCellConfig->spCellConfigDedicated->uplinkConfig->ext1->powerBoostPi2BPSK == 1){
      // TbD: assuming power class 3 capable UE operating in TDD bands n40, n41, n77, n78, and n79 with Pi/2 BPSK modulation
      delta_P_powerclass = -3;
      p_max += 3;
    }
  }

  NR_P_Max_t *p_Max = (mac->scc!=NULL) ? mac->scc->uplinkConfigCommon->frequencyInfoUL->p_Max : mac->scc_SIB->uplinkConfigCommon->frequencyInfoUL.p_Max;
  if (p_Max){

    p_max += *p_Max;

    LOG_D(MAC, "In %s maximum UL transmission power p_max is %ld dBm \n", __FUNCTION__, p_max);

    P_cmax_l = min(p_max - delta_TC_c, (P_powerclass - delta_P_powerclass) - max(max(MPR_c + delta_MPR_c, A_MPR_c) + delta_T_IB_c + delta_TC_c + delta_rx_SRS, P_MPR_c));

    P_cmax_h = min(p_max, P_powerclass - delta_P_powerclass);

  } else {

    P_cmax_l = (P_powerclass - delta_P_powerclass) - max(max(MPR_c + delta_MPR_c, A_MPR_c) + delta_T_IB_c + delta_TC_c + delta_rx_SRS, P_MPR_c);

    P_cmax_h = P_powerclass - delta_P_powerclass;

  }

  P_cmax = (P_cmax_h + P_cmax_l) / 2;

  LOG_D(MAC, "In %s configured maximum output power:  %ld dBm <= PCMAX %ld dBm <= %ld dBm \n", __FUNCTION__, P_cmax_l, P_cmax, P_cmax_h);

  return P_cmax;

}
