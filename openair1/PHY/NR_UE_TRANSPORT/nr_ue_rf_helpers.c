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

/*! \file PHY/NR_UE_TRANSPORT/nr_ue_rf_config.c
* \brief      Functional helpers to configure the RF boards at UE side
* \author     Guido Casati
* \date       2020
* \version    0.1
* \company    Fraunhofer IIS
* \email:     guido.casati@iis.fraunhofer.de
*/

#include "PHY/defs_nr_UE.h"
#include "PHY/phy_extern_nr_ue.h"
#include "nr_transport_proto_ue.h"
#include "executables/softmodem-common.h"

void nr_get_carrier_frequencies(NR_DL_FRAME_PARMS *fp, uint64_t *dl_carrier, uint64_t *ul_carrier){

  if (get_softmodem_params()->phy_test==1 || get_softmodem_params()->do_ra==1 || !downlink_frequency[0][0]) {
    *dl_carrier = fp->dl_CarrierFreq;
  } else {
    *dl_carrier = downlink_frequency[0][0];
  }

  if (uplink_frequency_offset[0][0])
    *ul_carrier = *dl_carrier + uplink_frequency_offset[0][0];
  else
    *ul_carrier = *dl_carrier + fp->ul_CarrierFreq - fp->dl_CarrierFreq;

}

void nr_rf_card_config(openair0_config_t *openair0_cfg,
                       double rx_gain_offset,
                       uint64_t ul_carrier,
                       uint64_t dl_carrier,
                       int freq_offset){

  uint8_t mod_id     = 0;
  uint8_t cc_id      = 0;
  PHY_VARS_NR_UE *ue = PHY_vars_UE_g[mod_id][cc_id];
  int rf_chain       = ue->rf_map.chain;
  double rx_gain     = ue->rx_total_gain_dB;
  double tx_gain     = ue->tx_total_gain_dB;

  for (int i = rf_chain; i < rf_chain + 4; i++) {

    if (i < openair0_cfg->rx_num_channels)
      openair0_cfg->rx_freq[i + rf_chain] = dl_carrier + freq_offset;
    else
      openair0_cfg->rx_freq[i] = 0.0;

    if (i<openair0_cfg->tx_num_channels)
      openair0_cfg->tx_freq[i] = ul_carrier + freq_offset;
    else
      openair0_cfg->tx_freq[i] = 0.0;

    if (tx_gain)
      openair0_cfg->tx_gain[i] = tx_gain;
    if (rx_gain)
      openair0_cfg->rx_gain[i] = rx_gain - rx_gain_offset;

    openair0_cfg->autocal[i] = 1;

    if (i < openair0_cfg->rx_num_channels) {
      LOG_I(PHY, "HW: Configuring channel %d (rf_chain %d): setting tx_gain %f, rx_gain %f, tx_freq %f Hz, rx_freq %f Hz\n",
        i,
        rf_chain,
        openair0_cfg->tx_gain[i],
        openair0_cfg->rx_gain[i],
        openair0_cfg->tx_freq[i],
        openair0_cfg->rx_freq[i]);
    }

  }
}