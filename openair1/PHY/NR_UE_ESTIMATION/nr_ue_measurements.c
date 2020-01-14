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

#include "PHY/defs_nr_UE.h"

int16_t get_nr_PL(PHY_VARS_NR_UE *ue,uint8_t gNB_index)
{



  LOG_D(PHY,"get_PL : rsrp %f dBm/RE (%f), eNB power %d dBm/RE\n",
        (1.0*dB_fixed_times10(ue->measurements.rsrp[gNB_index])-(10.0*ue->rx_total_gain_dB))/10.0,
        10*log10((double)ue->measurements.rsrp[gNB_index]),
        ue->frame_parms.ss_PBCH_BlockPower);

  return((int16_t)(((10*ue->rx_total_gain_dB) -
                    dB_fixed_times10(ue->measurements.rsrp[gNB_index])+
                    //        dB_fixed_times10(RSoffset*12*ue_g[Mod_id][CC_id]->frame_parms.N_RB_DL) +
                    (ue->frame_parms.ss_PBCH_BlockPower*10))/10));
}

