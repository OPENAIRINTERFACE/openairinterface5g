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
#include "LAYER2/NR_MAC_UE/nr_l1_helpers.h"
#include "NR_P-Max.h"

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
