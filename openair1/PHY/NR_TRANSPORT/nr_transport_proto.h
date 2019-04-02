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

/*! \file PHY/NR_TRANSPORT/nr_transport_proto.h.c
* \brief Function prototypes for PHY physical/transport channel processing and generation
* \author Ahmed Hussein
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: ahmed.hussein@iis.fraunhofer.de
* \note
* \warning
*/

#include "PHY/defs_nr_common.h"


/** \brief This function generates log-likelihood ratios (decoder input) for single-stream QPSK received waveforms.
    @param rxdataF_comp Compensated channel output
    @param ulsch_llr llr output
    @param nb_re number of REs for this allocation
    @param symbol OFDM symbol index in sub-frame
*/
void nr_ulsch_qpsk_llr(int32_t *rxdataF_comp,
                       int16_t *ulsch_llr,                          
                       uint32_t nb_re,
                       uint8_t  symbol);

/** \brief This function generates log-likelihood ratios (decoder input) for single-stream 16 QAM received waveforms.
    @param rxdataF_comp Compensated channel output
    @param ul_ch_mag uplink channel magnitude multiplied by the 1st amplitude threshold in QAM 16
    @param ulsch_llr llr output
    @param nb_re number of RBs for this allocation
    @param symbol OFDM symbol index in sub-frame
*/
void nr_ulsch_16qam_llr(int32_t *rxdataF_comp,
                        int32_t **ul_ch_mag,
                        int16_t  *ulsch_llr,
                        uint32_t nb_re,
                        uint8_t  symbol);

/** \brief This function generates log-likelihood ratios (decoder input) for single-stream 64 QAM received waveforms.
    @param rxdataF_comp Compensated channel output
    @param ul_ch_mag  uplink channel magnitude multiplied by the 1st amplitude threshold in QAM 64
    @param ul_ch_magb uplink channel magnitude multiplied by the 2bd amplitude threshold in QAM 64
    @param ulsch_llr llr output
    @param nb_re number of REs for this allocation
    @param symbol OFDM symbol index in sub-frame
*/
void nr_ulsch_64qam_llr(int32_t *rxdataF_comp,
                        int32_t **ul_ch_mag,
                        int32_t **ul_ch_magb,
                        int16_t  *ulsch_llr,
                        uint32_t nb_re,
                        uint8_t  symbol);


/** \brief This function computes the log-likelihood ratios for 4, 16, and 64 QAM
    @param rxdataF_comp Compensated channel output
    @param ul_ch_mag  uplink channel magnitude multiplied by the 1st amplitude threshold in QAM 64
    @param ul_ch_magb uplink channel magnitude multiplied by the 2bd amplitude threshold in QAM 64
    @param ulsch_llr llr output
    @param nb_re number of REs for this allocation
    @param symbol OFDM symbol index in sub-frame
    @param mod_order modulation order
*/
void nr_ulsch_compute_llr(int32_t *rxdataF_comp,
                          int32_t **ul_ch_mag,
                          int32_t **ul_ch_magb,
                          int16_t  *ulsch_llr,
                          uint32_t nb_re,
                          uint8_t  symbol,
                          uint8_t  mod_order);