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
#include "PHY/extern.h"

#ifdef EXMIMO
#include "openair0_lib.h"
extern int card;
#endif

void
phy_adjust_gain (PHY_VARS_UE *phy_vars_ue, uint32_t rx_power_fil_dB, uint8_t eNB_id)
{

#ifdef EXMIMO
  exmimo_config_t *p_exmimo_config = openair0_exmimo_pci[card].exmimo_config_ptr;
  uint16_t i;
#endif

  LOG_D(PHY,"Gain control: rssi %d (%d,%d)\n",
         rx_power_fil_dB,
         phy_vars_ue->PHY_measurements.rssi,
         phy_vars_ue->PHY_measurements.rx_power_avg_dB[eNB_id]
        );

  // Gain control with hysterisis
  // Adjust gain in phy_vars_ue->rx_vars[0].rx_total_gain_dB

  if (rx_power_fil_dB < TARGET_RX_POWER - 5) //&& (phy_vars_ue->rx_total_gain_dB < MAX_RF_GAIN) )
    phy_vars_ue->rx_total_gain_dB+=5;
  else if (rx_power_fil_dB > TARGET_RX_POWER + 5) //&& (phy_vars_ue->rx_total_gain_dB > MIN_RF_GAIN) )
    phy_vars_ue->rx_total_gain_dB-=5;

  if (phy_vars_ue->rx_total_gain_dB>MAX_RF_GAIN) {
    /*
    if ((openair_daq_vars.rx_rf_mode==0) && (openair_daq_vars.mode == openair_NOT_SYNCHED)) {
      openair_daq_vars.rx_rf_mode=1;
      phy_vars_ue->rx_total_gain_dB = max(MIN_RF_GAIN,MAX_RF_GAIN-25);
    }
    else {
    */
    phy_vars_ue->rx_total_gain_dB = MAX_RF_GAIN;
  } else if (phy_vars_ue->rx_total_gain_dB<MIN_RF_GAIN) {
    /*
    if ((openair_daq_vars.rx_rf_mode==1) && (openair_daq_vars.mode == openair_NOT_SYNCHED)) {
      openair_daq_vars.rx_rf_mode=0;
      phy_vars_ue->rx_total_gain_dB = min(MAX_RF_GAIN,MIN_RF_GAIN+25);
    }
    else {
    */
    phy_vars_ue->rx_total_gain_dB = MIN_RF_GAIN;
  }

  LOG_D(PHY,"Gain control: rx_total_gain_dB = %d (max %d,rxpf %d)\n",phy_vars_ue->rx_total_gain_dB,MAX_RF_GAIN,rx_power_fil_dB);

#ifdef EXMIMO

  if (phy_vars_ue->rx_total_gain_dB>phy_vars_ue->rx_gain_max[0]) {
    phy_vars_ue->rx_total_gain_dB = phy_vars_ue->rx_gain_max[0];

    for (i=0; i<phy_vars_ue->lte_frame_parms.nb_antennas_rx; i++) {
      p_exmimo_config->rf.rx_gain[i][0] = 30;
    }

  } else if (phy_vars_ue->rx_total_gain_dB<(phy_vars_ue->rx_gain_max[0]-30)) {
    // for the moment we stay in max gain mode
    phy_vars_ue->rx_total_gain_dB = phy_vars_ue->rx_gain_max[0] - 30;

    for (i=0; i<phy_vars_ue->lte_frame_parms.nb_antennas_rx; i++) {
      p_exmimo_config->rf.rx_gain[i][0] = 0;
    }

    /*
      phy_vars_ue->rx_gain_mode[0] = byp;
      phy_vars_ue->rx_gain_mode[1] = byp;
      exmimo_pci_interface->rf.rf_mode0 = 22991; //bypass
      exmimo_pci_interface->rf.rf_mode1 = 22991; //bypass

      if (phy_vars_ue->rx_total_gain_dB<(phy_vars_ue->rx_gain_byp[0]-50)) {
      exmimo_pci_interface->rf.rx_gain00 = 0;
      exmimo_pci_interface->rf.rx_gain10 = 0;
      }
    */
  } else {

    for (i=0; i<phy_vars_ue->lte_frame_parms.nb_antennas_rx; i++) {
      p_exmimo_config->rf.rx_gain[i][0] =  30 - phy_vars_ue->rx_gain_max[0] + phy_vars_ue->rx_total_gain_dB;
    }
  }

  /*
    break;
  case med_gain:
  case byp_gain:
      if (phy_vars_ue->rx_total_gain_dB>phy_vars_ue->rx_gain_byp[0]) {
          phy_vars_ue->rx_gain_mode[0]   = max_gain;
          phy_vars_ue->rx_gain_mode[1]   = max_gain;
          exmimo_pci_interface->rf.rf_mode0 = 55759; //max gain
          exmimo_pci_interface->rf.rf_mode1 = 55759; //max gain

          if (phy_vars_ue->rx_total_gain_dB>phy_vars_ue->rx_gain_max[0]) {
              exmimo_pci_interface->rf.rx_gain00 = 50;
              exmimo_pci_interface->rf.rx_gain10 = 50;
          }
          else {
              exmimo_pci_interface->rf.rx_gain00 = 50 - phy_vars_ue->rx_gain_max[0] + phy_vars_ue->rx_total_gain_dB;
              exmimo_pci_interface->rf.rx_gain10 = 50 - phy_vars_ue->rx_gain_max[1] + phy_vars_ue->rx_total_gain_dB;
          }
      }
      else if (phy_vars_ue->rx_total_gain_dB<(phy_vars_ue->rx_gain_byp[0]-50)) {
          exmimo_pci_interface->rf.rx_gain00 = 0;
          exmimo_pci_interface->rf.rx_gain10 = 0;
      }
      else {
          exmimo_pci_interface->rf.rx_gain00 = 50 - phy_vars_ue->rx_gain_byp[0] + phy_vars_ue->rx_total_gain_dB;
          exmimo_pci_interface->rf.rx_gain10 = 50 - phy_vars_ue->rx_gain_byp[1] + phy_vars_ue->rx_total_gain_dB;
      }
      break;
  default:
      exmimo_pci_interface->rf.rx_gain00 = 50;
      exmimo_pci_interface->rf.rx_gain10 = 50;
      break;
  }
      */
#endif

#ifdef DEBUG_PHY
  /*  if ((phy_vars_ue->frame%100==0) || (phy_vars_ue->frame < 10))
  msg("[PHY][ADJUST_GAIN] frame %d,  rx_power = %d, rx_power_fil = %d, rx_power_fil_dB = %d, coef=%d, ncoef=%d, rx_total_gain_dB = %d (%d,%d,%d)\n",
    phy_vars_ue->frame,rx_power,rx_power_fil,rx_power_fil_dB,coef,ncoef,phy_vars_ue->rx_total_gain_dB,
  TARGET_RX_POWER,MAX_RF_GAIN,MIN_RF_GAIN);
  */
#endif //DEBUG_PHY

}


