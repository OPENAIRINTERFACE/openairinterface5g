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


