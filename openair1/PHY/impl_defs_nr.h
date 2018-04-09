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

#ifndef __PHY_IMPLEMENTATION_DEFS_NR_H__
#define __PHY_IMPLEMENTATION_DEFS_NR_H__


//#include "impl_defs_lte.h"

typedef struct {
  /// \brief Pointers (dynamic) to the received data in the time domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **rxdata;
  /// \brief Pointers (dynamic) to the received data in the frequency domain.
  /// - first index: rx antenna [0..nb_antennas_rx[
  /// - second index: ? [0..2*ofdm_symbol_size*frame_parms->symbols_per_tti[
  int32_t **rxdataF;
  /// \brief holds the transmit data in the frequency domain.
  /// For IFFT_FPGA this points to the same memory as PHY_vars->rx_vars[a].RX_DMA_BUFFER. //?
  /// - first index: eNB id [0..2] (hard coded)
  /// - second index: tx antenna [0..14[ where 14 is the total supported antenna ports.
  /// - third index: sample [0..]
  int32_t **txdataF;
} NR_gNB_COMMON;
#endif
