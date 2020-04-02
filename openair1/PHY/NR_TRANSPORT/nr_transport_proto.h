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

/** \brief This function is the top-level entry point to PUSCH demodulation, after frequency-domain transformation and channel estimation.  It performs
    - RB extraction (signal and channel estimates)
    - channel compensation (matched filtering)
    - RE extraction (dmrs)
    - antenna combining (MRC, Alamouti, cycling)
    - LLR computation
    This function supports TM1, 2, 3, 5, and 6.
    @param ue Pointer to PHY variables
    @param UE_id id of current UE
    @param frame Frame number
    @param nr_tti_rx TTI number
    @param symbol Symbol on which to act (within-in nr_TTI_rx)
    @param harq_pid HARQ process ID
*/
void nr_rx_pusch(PHY_VARS_gNB *gNB,
                 uint8_t UE_id,
                 uint32_t frame,
                 uint8_t nr_tti_rx,
                 unsigned char symbol,
                 unsigned char harq_pid);


/** \brief This function performs RB extraction (signal and channel estimates) (currently signal only until channel estimation and compensation are implemented)
    @param rxdataF pointer to the received frequency domain signal
    @param rxdataF_ext pointer to the extracted frequency domain signal
    @param rb_alloc RB allocation map (used for Resource Allocation Type 0 in NR)
    @param symbol Symbol on which to act (within-in nr_TTI_rx)
    @param start_rb The starting RB in the RB allocation (used for Resource Allocation Type 1 in NR)
    @param nb_rb_pusch The number of RBs allocated (used for Resource Allocation Type 1 in NR)
    @param frame_parms, Pointer to frame descriptor structure
    @param is_dmrs_symbol, flag to indicate wether this OFDM symbol contains DMRS symbols or not.

*/
void nr_ulsch_extract_rbs_single(int32_t **rxdataF,
                                 NR_gNB_PUSCH *pusch_vars,
                                 unsigned char symbol,
                                 uint8_t is_dmrs_symbol,
                                 nfapi_nr_pusch_pdu_t *pusch_pdu,
                                 NR_DL_FRAME_PARMS *frame_parms);

void nr_ulsch_scale_channel(int32_t **ul_ch_estimates_ext,
                            NR_DL_FRAME_PARMS *frame_parms,
                            NR_gNB_ULSCH_t **ulsch_gNB,
                            uint8_t symbol,
                            uint8_t start_symbol,
                            uint16_t nb_rb,
                            pusch_dmrs_type_t pusch_dmrs_type);


/** \brief This function computes the average channel level over all allocated RBs and antennas (TX/RX) in order to compute output shift for compensated signal
    @param ul_ch_estimates_ext Channel estimates in allocated RBs
    @param frame_parms Pointer to frame descriptor
    @param avg Pointer to average signal strength
    @param pilots_flag Flag to indicate pilots in symbol
    @param nb_rb Number of allocated RBs
*/
void nr_ulsch_channel_level(int **ul_ch_estimates_ext,
                            NR_DL_FRAME_PARMS *frame_parms,
                            int32_t *avg,
                            uint8_t symbol,
                            uint32_t len,
                            unsigned short nb_rb);


/** \brief This function performs channel compensation (matched filtering) on the received RBs for this allocation.  In addition, it computes the squared-magnitude of the channel with weightings for 16QAM/64QAM detection as well as dual-stream detection (cross-correlation)
    @param rxdataF_ext Frequency-domain received signal in RBs to be demodulated
    @param ul_ch_estimates_ext Frequency-domain channel estimates in RBs to be demodulated
    @param ul_ch_mag First Channel magnitudes (16QAM/64QAM)
    @param ul_ch_magb Second weighted Channel magnitudes (64QAM)
    @param rxdataF_comp Compensated received waveform
    @param frame_parms Pointer to frame descriptor
    @param symbol Symbol on which to operate
    @param Qm Modulation order of allocation
    @param nb_rb Number of RBs in allocation
    @param output_shift Rescaling for compensated output (should be energy-normalizing)
*/
void nr_ulsch_channel_compensation(int **rxdataF_ext,
                                int **ul_ch_estimates_ext,
                                int **ul_ch_mag,
                                int **ul_ch_magb,
                                int **rxdataF_comp,
                                int **rho,
                                NR_DL_FRAME_PARMS *frame_parms,
                                unsigned char symbol,
                                uint8_t is_dmrs_symbol,
                                unsigned char mod_order,
                                unsigned short nb_rb,
                                unsigned char output_shift);

/*!
\brief This function implements the idft transform precoding in PUSCH
\param z Pointer to input in frequnecy domain, and it is also the output in time domain
\param Msc_PUSCH number of allocated data subcarriers
*/
void nr_idft(uint32_t *z, uint32_t Msc_PUSCH);

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
                        uint32_t nb_rb,
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
                        uint32_t nb_rb,
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
                          uint32_t nb_rb,
                          uint32_t nb_re,
                          uint8_t  symbol,
                          uint8_t  mod_order);

void nr_fill_ulsch(PHY_VARS_gNB *gNB,
                   int frame,
                   int slot,
                   nfapi_nr_pusch_pdu_t *ulsch_pdu);

uint32_t nr_get_code_rate_dl(uint8_t Imcs, uint8_t table_idx);

uint8_t nr_get_Qm_ul(uint8_t Imcs, uint8_t table_idx);

uint8_t nr_get_Qm_dl(uint8_t Imcs, uint8_t table_idx);

uint32_t nr_get_code_rate_ul(uint8_t Imcs, uint8_t table_idx);

uint32_t nr_get_code_rate_dl(uint8_t Imcs, uint8_t table_idx);

void rx_nr_prach(PHY_VARS_gNB *gNB,
		 int frame,
		 int subframe,
		 uint16_t *max_preamble,
		 uint16_t *max_preamble_energy,
		 uint16_t *max_preamble_delay
		 );
