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

#include "openair0_lib.h"
#include "gain_control.h"
//#define DEBUG_PHY
#ifdef DEBUG_PHY
#include "UTIL/LOG/log_extern.h"
#endif

void gain_control_all (unsigned int rx_power_fil_dB, unsigned int card) {
  unsigned int ant;

  for (ant=0; ant<4; ant++)
    gain_control (rx_power_fil_dB,ant,card);
}

void gain_control (unsigned int rx_power_fil_dB, unsigned int ant, unsigned int card) {

  exmimo_config_t *p_exmimo_config = openair0_exmimo_pci[card].exmimo_config_ptr;
  unsigned int rf_mode, rx_gain, LNA;

  rx_gain = p_exmimo_config->rf.rx_gain[ant][0];
  rf_mode = p_exmimo_config->rf.rf_mode[ant];
  LNA = rf_mode & LNAGAINMASK;

  // Gain control with hysterisis

  if (rx_power_fil_dB < TARGET_RX_POWER_MIN) { //increase gain
    switch (LNA) {
    case LNAByp:
      p_exmimo_config->rf.rf_mode[ant] = (rf_mode & (~LNAGAINMASK)) | LNAMed;
      break;

    case LNAMed:
      p_exmimo_config->rf.rf_mode[ant] = (rf_mode & (~LNAGAINMASK)) | LNAMax;
      break;

    case LNAMax:
      p_exmimo_config->rf.rx_gain[ant][0] = min(30,rx_gain+5);
      break;

    default:
      break;
    }
  } else if (rx_power_fil_dB > TARGET_RX_POWER_MAX) { //decrease gain
    if (rx_gain==0) {
      switch (LNA) {
      case LNAMax:
        p_exmimo_config->rf.rf_mode[ant] = (rf_mode & (~LNAGAINMASK)) | LNAMed;
        break;

      case LNAMed:
        p_exmimo_config->rf.rf_mode[ant] = (rf_mode & (~LNAGAINMASK)) | LNAByp;
        break;

      case LNAByp:
        break;

      default:
        break;
      }
    } else {
      p_exmimo_config->rf.rx_gain[ant][0] = max(0,(int)rx_gain-5);
    }
  }

#ifdef DEBUG_PHY
  LOG_D(PHY,"AGC for chain %d: rx_power_fil_dB=%d, rx_gain=%d, LNA=%d (1=Byp,2=Med,3=Max)\n",ant,rx_power_fil_dB,p_exmimo_config->rf.rx_gain[ant][0],(p_exmimo_config->rf.rf_mode[ant]&LNAGAINMASK)>>14);
#endif //DEBUG_PHY
}


