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

void
phy_adjust_gain (PHY_VARS_UE *ue, uint32_t rx_power_fil_dB, uint8_t eNB_id)
{

  LOG_D(PHY,"Gain control: rssi %d (%d,%d)\n",
	rx_power_fil_dB,
	ue->measurements.rssi,
	ue->measurements.rx_power_avg_dB[eNB_id]
        );

  // Gain control with hysterisis
  // Adjust gain in ue->rx_vars[0].rx_total_gain_dB

  if (rx_power_fil_dB < TARGET_RX_POWER - 5) //&& (ue->rx_total_gain_dB < MAX_RF_GAIN) )
    ue->rx_total_gain_dB+=5;
  else if (rx_power_fil_dB > TARGET_RX_POWER + 5) //&& (ue->rx_total_gain_dB > MIN_RF_GAIN) )
    ue->rx_total_gain_dB-=5;

  if (ue->rx_total_gain_dB>MAX_RF_GAIN) {
    /*
    if ((openair_daq_vars.rx_rf_mode==0) && (openair_daq_vars.mode == openair_NOT_SYNCHED)) {
      openair_daq_vars.rx_rf_mode=1;
      ue->rx_total_gain_dB = max(MIN_RF_GAIN,MAX_RF_GAIN-25);
    }
    else {
    */
    ue->rx_total_gain_dB = MAX_RF_GAIN;
  } else if (ue->rx_total_gain_dB<MIN_RF_GAIN) {
    /*
    if ((openair_daq_vars.rx_rf_mode==1) && (openair_daq_vars.mode == openair_NOT_SYNCHED)) {
      openair_daq_vars.rx_rf_mode=0;
      ue->rx_total_gain_dB = min(MAX_RF_GAIN,MIN_RF_GAIN+25);
    }
    else {
    */
    ue->rx_total_gain_dB = MIN_RF_GAIN;
  }

  LOG_D(PHY,"Gain control: rx_total_gain_dB = %d (max %d,rxpf %d)\n",ue->rx_total_gain_dB,MAX_RF_GAIN,rx_power_fil_dB);

#ifdef DEBUG_PHY
  /*  if ((ue->frame%100==0) || (ue->frame < 10))
  msg("[PHY][ADJUST_GAIN] frame %d,  rx_power = %d, rx_power_fil = %d, rx_power_fil_dB = %d, coef=%d, ncoef=%d, rx_total_gain_dB = %d (%d,%d,%d)\n",
    ue->frame,rx_power,rx_power_fil,rx_power_fil_dB,coef,ncoef,ue->rx_total_gain_dB,
  TARGET_RX_POWER,MAX_RF_GAIN,MIN_RF_GAIN);
  */
#endif //DEBUG_PHY

}


