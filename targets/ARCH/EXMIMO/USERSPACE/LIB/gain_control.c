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


