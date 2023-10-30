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

/*! \file PHY/impl_defs_top.h
* \brief More defines and structure definitions
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/

#ifndef __PHY_IMPLEMENTATION_DEFS_NB_IOT_H__
#define __PHY_IMPLEMENTATION_DEFS_NB_IOT_H__

#include "common/openairinterface5g_limits.h"
#define ONE_OVER_SQRT2_Q15_NB_IoT 23170

#define NUMBER_OF_SUBBANDS_MAX_NB_IoT 13
typedef enum {no_relay_NB_IoT=1,unicast_relay_type1_NB_IoT,unicast_relay_type2_NB_IoT, multicast_relay_NB_IoT} relaying_type_t_NB_IoT;
typedef struct {
  //unsigned int   rx_power[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];     //! estimated received signal power (linear)
  //unsigned short rx_power_dB[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];  //! estimated received signal power (dB)
  //unsigned short rx_avg_power_dB[NUMBER_OF_CONNECTED_eNB_MAX];              //! estimated avg received signal power (dB)

  // RRC measurements
  uint32_t rssi;
  int n_adj_cells;
  unsigned int adj_cell_id[6];
  uint32_t rsrq[7];
  uint32_t rsrp[7];
  float rsrp_filtered[7]; // after layer 3 filtering
  float rsrq_filtered[7];
  // common measurements
  //! estimated noise power (linear)
  unsigned int   n0_power[NB_ANTENNAS_RX];
  //! estimated noise power (dB)
  unsigned short n0_power_dB[NB_ANTENNAS_RX];
  //! total estimated noise power (linear)
  unsigned int   n0_power_tot;
  //! total estimated noise power (dB)
  unsigned short n0_power_tot_dB;
  //! average estimated noise power (linear)
  unsigned int   n0_power_avg;
  //! average estimated noise power (dB)
  unsigned short n0_power_avg_dB;
  //! total estimated noise power (dBm)
  short n0_power_tot_dBm;

  // UE measurements
  //! estimated received spatial signal power (linear)
  int            rx_spatial_power[NUMBER_OF_CONNECTED_eNB_MAX][2][2];
  //! estimated received spatial signal power (dB)
  unsigned short rx_spatial_power_dB[NUMBER_OF_CONNECTED_eNB_MAX][2][2];

  /// estimated received signal power (sum over all TX antennas)
  //int            wideband_cqi[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];
  int            rx_power[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];
  /// estimated received signal power (sum over all TX antennas)
  //int            wideband_cqi_dB[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];
  unsigned short rx_power_dB[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];

  /// estimated received signal power (sum over all TX/RX antennas)
  int            rx_power_tot[NUMBER_OF_CONNECTED_eNB_MAX]; //NEW
  /// estimated received signal power (sum over all TX/RX antennas)
  unsigned short rx_power_tot_dB[NUMBER_OF_CONNECTED_eNB_MAX]; //NEW

  //! estimated received signal power (sum of all TX/RX antennas, time average)
  int            rx_power_avg[NUMBER_OF_CONNECTED_eNB_MAX];
  //! estimated received signal power (sum of all TX/RX antennas, time average, in dB)
  unsigned short rx_power_avg_dB[NUMBER_OF_CONNECTED_eNB_MAX];

  /// SINR (sum of all TX/RX antennas, in dB)
  int            wideband_cqi_tot[NUMBER_OF_CONNECTED_eNB_MAX];
  /// SINR (sum of all TX/RX antennas, time average, in dB)
  int            wideband_cqi_avg[NUMBER_OF_CONNECTED_eNB_MAX];

  //! estimated rssi (dBm)
  short          rx_rssi_dBm[NUMBER_OF_CONNECTED_eNB_MAX];
  //! estimated correlation (wideband linear) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation[NUMBER_OF_CONNECTED_eNB_MAX][2];
  //! estimated correlation (wideband dB) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation_dB[NUMBER_OF_CONNECTED_eNB_MAX][2];

  /// Wideband CQI (sum of all RX antennas, in dB, for precoded transmission modes (3,4,5,6), up to 4 spatial streams)
  int            precoded_cqi_dB[NUMBER_OF_CONNECTED_eNB_MAX+1][4];
  /// Subband CQI per RX antenna (= SINR)
  int            subband_cqi[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX][NUMBER_OF_SUBBANDS_MAX_NB_IoT];
  /// Total Subband CQI  (= SINR)
  int            subband_cqi_tot[NUMBER_OF_CONNECTED_eNB_MAX][NUMBER_OF_SUBBANDS_MAX_NB_IoT];
  /// Subband CQI in dB (= SINR dB)
  int            subband_cqi_dB[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX][NUMBER_OF_SUBBANDS_MAX_NB_IoT];
  /// Total Subband CQI
  int            subband_cqi_tot_dB[NUMBER_OF_CONNECTED_eNB_MAX][NUMBER_OF_SUBBANDS_MAX_NB_IoT];
  /// Wideband PMI for each RX antenna
  int            wideband_pmi_re[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];
  /// Wideband PMI for each RX antenna
  int            wideband_pmi_im[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];
  ///Subband PMI for each RX antenna
  int            subband_pmi_re[NUMBER_OF_CONNECTED_eNB_MAX][NUMBER_OF_SUBBANDS_MAX_NB_IoT][NB_ANTENNAS_RX];
  ///Subband PMI for each RX antenna
  int            subband_pmi_im[NUMBER_OF_CONNECTED_eNB_MAX][NUMBER_OF_SUBBANDS_MAX_NB_IoT][NB_ANTENNAS_RX];
  /// chosen RX antennas (1=Rx antenna 1, 2=Rx antenna 2, 3=both Rx antennas)
  unsigned char           selected_rx_antennas[NUMBER_OF_CONNECTED_eNB_MAX][NUMBER_OF_SUBBANDS_MAX_NB_IoT];
  /// Wideband Rank indication
  unsigned char  rank[NUMBER_OF_CONNECTED_eNB_MAX];
  /// Number of RX Antennas
  unsigned char  nb_antennas_rx;
  /// DLSCH error counter
  // short          dlsch_errors;

} PHY_MEASUREMENTS_NB_IoT;


typedef struct {
  //unsigned int   rx_power[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];     //! estimated received signal power (linear)
  //unsigned short rx_power_dB[NUMBER_OF_CONNECTED_eNB_MAX][NB_ANTENNAS_RX];  //! estimated received signal power (dB)
  //unsigned short rx_avg_power_dB[NUMBER_OF_CONNECTED_eNB_MAX];              //! estimated avg received signal power (dB)

  // common measurements
  //! estimated noise power (linear)
  unsigned int   n0_power[NB_ANTENNAS_RX];
  //! estimated noise power (dB)
  unsigned short n0_power_dB[NB_ANTENNAS_RX];
  //! total estimated noise power (linear)
  unsigned int   n0_power_tot;
  //! estimated avg noise power (dB)
  unsigned short n0_power_tot_dB;
  //! estimated avg noise power (dB)
  short n0_power_tot_dBm;
  //! estimated avg noise power per RB per RX ant (lin)
  unsigned short n0_subband_power[NB_ANTENNAS_RX][100];
  //! estimated avg noise power per RB per RX ant (dB)
  unsigned short n0_subband_power_dB[NB_ANTENNAS_RX][100];
  //! estimated avg noise power per RB (dB)
  short n0_subband_power_tot_dB[100];
  //! estimated avg noise power per RB (dBm)
  short n0_subband_power_tot_dBm[100];
  // eNB measurements (per user)
  //! estimated received spatial signal power (linear)
  unsigned int   rx_spatial_power[NUMBER_OF_UE_MAX_NB_IoT][2][2];
  //! estimated received spatial signal power (dB)
  unsigned short rx_spatial_power_dB[NUMBER_OF_UE_MAX_NB_IoT][2][2];
  //! estimated rssi (dBm)
  short          rx_rssi_dBm[NUMBER_OF_UE_MAX_NB_IoT];
  //! estimated correlation (wideband linear) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation[NUMBER_OF_UE_MAX_NB_IoT][2];
  //! estimated correlation (wideband dB) between spatial channels (computed in dlsch_demodulation)
  int            rx_correlation_dB[NUMBER_OF_UE_MAX_NB_IoT][2];

  /// Wideband CQI (= SINR)
  int            wideband_cqi[NUMBER_OF_UE_MAX_NB_IoT][NB_ANTENNAS_RX];
  /// Wideband CQI in dB (= SINR dB)
  int            wideband_cqi_dB[NUMBER_OF_UE_MAX_NB_IoT][NB_ANTENNAS_RX];
  /// Wideband CQI (sum of all RX antennas, in dB)
  char           wideband_cqi_tot[NUMBER_OF_UE_MAX_NB_IoT];
  /// Subband CQI per RX antenna and RB (= SINR)
  int            subband_cqi[NUMBER_OF_UE_MAX_NB_IoT][NB_ANTENNAS_RX][100];
  /// Total Subband CQI and RB (= SINR)
  int            subband_cqi_tot[NUMBER_OF_UE_MAX_NB_IoT][100];
  /// Subband CQI in dB and RB (= SINR dB)
  int            subband_cqi_dB[NUMBER_OF_UE_MAX_NB_IoT][NB_ANTENNAS_RX][100];
  /// Total Subband CQI and RB
  int            subband_cqi_tot_dB[NUMBER_OF_UE_MAX_NB_IoT][100];

} PHY_MEASUREMENTS_eNB_NB_IoT;

#endif //__PHY_IMPLEMENTATION_DEFS_H__
