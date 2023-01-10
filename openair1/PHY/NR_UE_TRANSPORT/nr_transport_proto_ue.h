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

/*! \file PHY/NR_UE_TRANSPORT/transport_proto_ue.h
 * \brief Function prototypes for PHY physical/transport channel processing and generation V8.6 2009-03
 * \author R. Knopp, F. Kaltenberger
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */
#ifndef __NR_TRANSPORT_PROTO_UE__H__
#define __NR_TRANSPORT_PROTO_UE__H__
#include "PHY/defs_nr_UE.h"
#include "SCHED_NR_UE/defs.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include <math.h>
#include "nfapi_interface.h"
#include <openair1/PHY/LTE_TRANSPORT/transport_proto.h>

#define NR_PUSCH_x 2 // UCI placeholder bit TS 38.212 V15.4.0 subclause 5.3.3.1
#define NR_PUSCH_y 3 // UCI placeholder bit

// Functions below implement 36-211 and 36-212

/** @addtogroup _PHY_TRANSPORT_
 * @{
 */


/** \brief This function initialises structures for DLSCH at UE
*/
void nr_ue_dlsch_init(NR_UE_DLSCH_t *dlsch_list, int num_dlsch, uint8_t max_ldpc_iterations);

/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream QPSK/QPSK reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void nr_qpsk_qpsk(int16_t *stream0_in,
               int16_t *stream1_in,
               int16_t *stream0_out,
               int16_t *rho01,
               int32_t length);

/** \brief This function perform LLR computation for dual-stream (QPSK/QPSK) transmission.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param rho_i Correlation between channel of signal and inteference
    @param dlsch_llr llr output
    @param symbol OFDM symbol index in sub-frame
    @param first_symbol_flag flag to indicate this is the first symbol of the dlsch
    @param nb_rb number of RBs for this allocation
    @param pbch_pss_sss_adj Number of channel bits taken by PBCH/PSS/SSS
    @param llr128p pointer to pointer to symbol in dlsch_llr*/
int32_t nr_dlsch_qpsk_qpsk_llr(NR_DL_FRAME_PARMS *frame_parms,
                            int32_t **rxdataF_comp,
                            int32_t **rxdataF_comp_i,
                            int32_t **rho_i,
                            int16_t *dlsch_llr,
                            uint8_t symbol,
                            uint32_t len,
                            uint8_t first_symbol_flag,
                            uint16_t nb_rb,
                            uint16_t pbch_pss_sss_adj,
                            int16_t **llr128p);

/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream QPSK/16QAM reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void nr_qpsk_qam16(int16_t *stream0_in,
                int16_t *stream1_in,
                short *ch_mag_i,
                int16_t *stream0_out,
                int16_t *rho01,
                int32_t length);

/** \brief This function perform LLR computation for dual-stream (QPSK/16QAM) transmission.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param rho_i Correlation between channel of signal and inteference
    @param dlsch_llr llr output
    @param symbol OFDM symbol index in sub-frame
    @param first_symbol_flag flag to indicate this is the first symbol of the dlsch
    @param nb_rb number of RBs for this allocation
    @param pbch_pss_sss_adj Number of channel bits taken by PBCH/PSS/SSS
    @param llr128p pointer to pointer to symbol in dlsch_llr*/
int32_t nr_dlsch_qpsk_16qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                             int32_t **rxdataF_comp,
                             int32_t **rxdataF_comp_i,
                             int **dl_ch_mag_i, //|h_1|^2*(2/sqrt{10})
                             int32_t **rho_i,
                             int16_t *dlsch_llr,
                             uint8_t symbol,
                             uint8_t first_symbol_flag,
                             uint16_t nb_rb,
                             uint16_t pbch_pss_sss_adj,
                             int16_t **llr128p);


/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream QPSK/64QAM reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void nr_qpsk_qam64(int16_t *stream0_in,
                int16_t *stream1_in,
                short *ch_mag_i,
                int16_t *stream0_out,
                int16_t *rho01,
                int32_t length);

/** \brief This function perform LLR computation for dual-stream (QPSK/64QAM) transmission.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param rho_i Correlation between channel of signal and inteference
    @param dlsch_llr llr output
    @param symbol OFDM symbol index in sub-frame
    @param first_symbol_flag flag to indicate this is the first symbol of the dlsch
    @param nb_rb number of RBs for this allocation
    @param pbch_pss_sss_adj Number of channel bits taken by PBCH/PSS/SSS
    @param llr128p pointer to pointer to symbol in dlsch_llr*/
int32_t nr_dlsch_qpsk_64qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                             int32_t **rxdataF_comp,
                             int32_t **rxdataF_comp_i,
                             int **dl_ch_mag_i, //|h_1|^2*(2/sqrt{10})
                             int32_t **rho_i,
                             int16_t *dlsch_llr,
                             uint8_t symbol,
                             uint8_t first_symbol_flag,
                             uint16_t nb_rb,
                             uint16_t pbch_pss_sss_adj,
                             int16_t **llr128p);


/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream 16QAM/QPSK reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void nr_qam16_qpsk(short *stream0_in,
                short *stream1_in,
                short *ch_mag,
                short *stream0_out,
                short *rho01,
                int length);
/** \brief This function perform LLR computation for dual-stream (16QAM/QPSK) transmission.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param rho_i Correlation between channel of signal and inteference
    @param dlsch_llr llr output
    @param symbol OFDM symbol index in sub-frame
    @param first_symbol_flag flag to indicate this is the first symbol of the dlsch
    @param nb_rb number of RBs for this allocation
    @param pbch_pss_sss_adj Number of channel bits taken by PBCH/PSS/SSS
    @param llr16p pointer to pointer to symbol in dlsch_llr*/
int nr_dlsch_16qam_qpsk_llr(NR_DL_FRAME_PARMS *frame_parms,
                         int **rxdataF_comp,
                         int **rxdataF_comp_i,
                         int **dl_ch_mag,   //|h_0|^2*(2/sqrt{10})
                         int **rho_i,
                         short *dlsch_llr,
                         unsigned char symbol,
                         unsigned char first_symbol_flag,
                         unsigned short nb_rb,
                         uint16_t pbch_pss_sss_adjust,
                         short **llr16p);

/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream 16QAM/16QAM reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void nr_qam16_qam16(short *stream0_in,
                 short *stream1_in,
                 short *ch_mag,
                 short *ch_mag_i,
                 short *stream0_out,
                 short *rho01,
                 int length);

/** \brief This function perform LLR computation for dual-stream (16QAM/16QAM) transmission.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param rho_i Correlation between channel of signal and inteference
    @param dlsch_llr llr output
    @param symbol OFDM symbol index in sub-frame
    @param first_symbol_flag flag to indicate this is the first symbol of the dlsch
    @param nb_rb number of RBs for this allocation
    @param pbch_pss_sss_adj Number of channel bits taken by PBCH/PSS/SSS
    @param llr16p pointer to pointer to symbol in dlsch_llr*/
int nr_dlsch_16qam_16qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                          int **rxdataF_comp,
                          int **rxdataF_comp_i,
                          int **dl_ch_mag,   //|h_0|^2*(2/sqrt{10})
                          int **dl_ch_mag_i, //|h_1|^2*(2/sqrt{10})
                          int **rho_i,
                          short *dlsch_llr,
                          unsigned char symbol,
						  uint32_t len,
                          unsigned char first_symbol_flag,
                          unsigned short nb_rb,
                          uint16_t pbch_pss_sss_adjust,
                          short **llr16p);

/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream 16QAM/64QAM reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void nr_qam16_qam64(short *stream0_in,
                 short *stream1_in,
                 short *ch_mag,
                 short *ch_mag_i,
                 short *stream0_out,
                 short *rho01,
                 int length);

/** \brief This function perform LLR computation for dual-stream (16QAM/64QAM) transmission.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param rho_i Correlation between channel of signal and inteference
    @param dlsch_llr llr output
    @param symbol OFDM symbol index in sub-frame
    @param first_symbol_flag flag to indicate this is the first symbol of the dlsch
    @param nb_rb number of RBs for this allocation
    @param pbch_pss_sss_adj Number of channel bits taken by PBCH/PSS/SSS
    @param llr16p pointer to pointer to symbol in dlsch_llr*/
int nr_dlsch_16qam_64qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                          int **rxdataF_comp,
                          int **rxdataF_comp_i,
                          int **dl_ch_mag,   //|h_0|^2*(2/sqrt{10})
                          int **dl_ch_mag_i, //|h_1|^2*(2/sqrt{10})
                          int **rho_i,
                          short *dlsch_llr,
                          unsigned char symbol,
                          unsigned char first_symbol_flag,
                          unsigned short nb_rb,
                          uint16_t pbch_pss_sss_adjust,
                          short **llr16p);

/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream 64QAM/64QAM reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void nr_qam64_qpsk(short *stream0_in,
                short *stream1_in,
                short *ch_mag,
                short *stream0_out,
                short *rho01,
                int length);

/** \brief This function perform LLR computation for dual-stream (64QAM/64QAM) transmission.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param rho_i Correlation between channel of signal and inteference
    @param dlsch_llr llr output
    @param symbol OFDM symbol index in sub-frame
    @param first_symbol_flag flag to indicate this is the first symbol of the dlsch
    @param nb_rb number of RBs for this allocation
    @param pbch_pss_sss_adj Number of channel bits taken by PBCH/PSS/SSS
    @param llr16p pointer to pointer to symbol in dlsch_llr*/
int nr_dlsch_64qam_qpsk_llr(NR_DL_FRAME_PARMS *frame_parms,
                         int **rxdataF_comp,
                         int **rxdataF_comp_i,
                         int **dl_ch_mag,
                         int **rho_i,
                         short *dlsch_llr,
                         unsigned char symbol,
                         unsigned char first_symbol_flag,
                         unsigned short nb_rb,
                         uint16_t pbch_pss_sss_adjust,
                         short **llr16p);

/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream 64QAM/16QAM reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void nr_qam64_qam16(short *stream0_in,
                 short *stream1_in,
                 short *ch_mag,
                 short *ch_mag_i,
                 short *stream0_out,
                 short *rho01,
                 int length);

/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream 64QAM/16QAM reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void qam64_qam16_avx2(short *stream0_in,
                      short *stream1_in,
                      short *ch_mag,
                      short *ch_mag_i,
                      short *stream0_out,
                      short *rho01,
                      int length);

/** \brief This function perform LLR computation for dual-stream (64QAM/16QAM) transmission.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param rho_i Correlation between channel of signal and inteference
    @param dlsch_llr llr output
    @param symbol OFDM symbol index in sub-frame
    @param first_symbol_flag flag to indicate this is the first symbol of the dlsch
    @param nb_rb number of RBs for this allocation
    @param pbch_pss_sss_adj Number of channel bits taken by PBCH/PSS/SSS
    @param llr16p pointer to pointer to symbol in dlsch_llr*/
int nr_dlsch_64qam_16qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                          int **rxdataF_comp,
                          int **rxdataF_comp_i,
                          int **dl_ch_mag,
                          int **dl_ch_mag_i,
                          int **rho_i,
                          short *dlsch_llr,
                          unsigned char symbol,
                          unsigned char first_symbol_flag,
                          unsigned short nb_rb,
                          uint16_t pbch_pss_sss_adjust,
                          short **llr16p);

/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream 64QAM/64QAM reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void qam64_qam64(short *stream0_in,
                 short *stream1_in,
                 short *ch_mag,
                 short *ch_mag_i,
                 short *stream0_out,
                 short *rho01,
                 int length);

/** \brief This function computes the LLRs for ML (max-logsum approximation) dual-stream 64QAM/64QAM reception.
    @param stream0_in Input from channel compensated (MR combined) stream 0
    @param stream1_in Input from channel compensated (MR combined) stream 1
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param stream0_out Output from LLR unit for stream0
    @param rho01 Cross-correlation between channels (MR combined)
    @param length in complex channel outputs*/
void qam64_qam64_avx2(int32_t *stream0_in,
                      int32_t *stream1_in,
                      int32_t *ch_mag,
                      int32_t *ch_mag_i,
                      int16_t *stream0_out,
                      int32_t *rho01,
                      int length);

/** \brief This function perform LLR computation for dual-stream (64QAM/64QAM) transmission.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param ch_mag   Input from scaled channel magnitude square of h0'*g0
    @param ch_mag_i Input from scaled channel magnitude square of h0'*g1
    @param rho_i Correlation between channel of signal and inteference
    @param dlsch_llr llr output
    @param symbol OFDM symbol index in sub-frame
    @param first_symbol_flag flag to indicate this is the first symbol of the dlsch
    @param nb_rb number of RBs for this allocation
    @param pbch_pss_sss_adj Number of channel bits taken by PBCH/PSS/SSS
    @param llr16p pointer to pointer to symbol in dlsch_llr*/
int nr_dlsch_64qam_64qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                          int **rxdataF_comp,
                          int **rxdataF_comp_i,
                          int **dl_ch_mag,
                          int **dl_ch_mag_i,
                          int **rho_i,
                          short *dlsch_llr,
                          unsigned char symbol,
						  uint32_t len,
                          unsigned char first_symbol_flag,
                          unsigned short nb_rb,
                          uint16_t pbch_pss_sss_adjust,
                          //short **llr16p,
                          uint32_t llr_offset);


/** \brief This function generates log-likelihood ratios (decoder input) for single-stream QPSK received waveforms.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param dlsch_llr llr output
    @param symbol OFDM symbol index in sub-frame
    @param first_symbol_flag
    @param nb_rb number of RBs for this allocation
    @param pbch_pss_sss_adj Number of channel bits taken by PBCH/PSS/SSS
    @param llr128p pointer to pointer to symbol in dlsch_llr
    @param beamforming_mode beamforming mode
*/
int32_t nr_dlsch_qpsk_llr(NR_DL_FRAME_PARMS *frame_parms,
                   int32_t *rxdataF_comp,
                   int16_t *dlsch_llr,
                   uint8_t symbol,
                   uint32_t len,
                   uint8_t first_symbol_flag,
                   uint16_t nb_rb);

/**
   \brief This function generates log-likelihood ratios (decoder input) for single-stream 16QAM received waveforms
   @param frame_parms Frame descriptor structure
   @param rxdataF_comp Compensated channel output
   @param dlsch_llr llr output
   @param dl_ch_mag Squared-magnitude of channel in each resource element position corresponding to allocation and weighted for mid-point in 16QAM constellation
   @param symbol OFDM symbol index in sub-frame
   @param first_symbol_flag
   @param nb_rb number of RBs for this allocation
   @param pbch_pss_sss_adjust  Adjustment factor in RE for PBCH/PSS/SSS allocations
   @param llr128p pointer to pointer to symbol in dlsch_llr
   @param beamforming_mode beamforming mode
*/

void nr_dlsch_16qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                     int32_t *rxdataF_comp,
                     int16_t *dlsch_llr,
                     int32_t *dl_ch_mag,
                     uint8_t symbol,
                     uint32_t len,
                     uint8_t first_symbol_flag,
                     uint16_t nb_rb);
/**
   \brief This function generates log-likelihood ratios (decoder input) for single-stream 16QAM received waveforms
   @param frame_parms Frame descriptor structure
   @param rxdataF_comp Compensated channel output
   @param dlsch_llr llr output
   @param dl_ch_mag Squared-magnitude of channel in each resource element position corresponding to allocation, weighted by first mid-point of 64-QAM constellation
   @param dl_ch_magb Squared-magnitude of channel in each resource element position corresponding to allocation, weighted by second mid-point of 64-QAM constellation
   @param symbol OFDM symbol index in sub-frame
   @param first_symbol_flag
   @param nb_rb number of RBs for this allocation
   @param pbch_pss_sss_adjust PBCH/PSS/SSS RE adjustment (in REs)
   @param beamforming_mode beamforming mode
*/

void nr_dlsch_64qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                     int32_t *rxdataF_comp,
                     int16_t *dlsch_llr,
                     int32_t *dl_ch_mag,
                     int32_t *dl_ch_magb,
                     uint8_t symbol,
                     uint32_t len,
                     uint8_t first_symbol_flag,
                     uint16_t nb_rb);

void nr_dlsch_256qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                     int32_t *rxdataF_comp,
                     int16_t *dlsch_llr,
                     int32_t *dl_ch_mag,
                     int32_t *dl_ch_magb,
                     int32_t *dl_ch_magr,
                     uint8_t symbol,
                     uint32_t len,
                     uint8_t first_symbol_flag,
                     uint16_t nb_rb);


/** \fn dlsch_extract_rbs(int32_t **rxdataF,
    int32_t **dl_ch_estimates,
    int32_t **rxdataF_ext,
    int32_t **dl_ch_estimates_ext,
    unsigned char symbol
    uint8_t pilots,
    uint8_t config_type,
    unsigned short start_rb,
    unsigned short nb_rb_pdsch,
    uint8_t n_dmrs_cdm_groups,
    uint8_t Nl,
    NR_DL_FRAME_PARMS *frame_parms,
    uint16_t dlDmrsSymbPos)
    \brief This function extracts the received resource blocks, both channel estimates and data symbols,
    for the current allocation and for multiple layer antenna gNB transmission.
    @param rxdataF Raw FFT output of received signal
    @param dl_ch_estimates Channel estimates of current slot
    @param rxdataF_ext FFT output for RBs in this allocation
    @param dl_ch_estimates_ext Channel estimates for RBs in this allocation
    @param Nl nb of antenna layers
    @param symbol Symbol to extract
    @param n_dmrs_cdm_groups
    @param frame_parms Pointer to frame descriptor
*/
void nr_dlsch_extract_rbs(uint32_t rxdataF_sz,
                          c16_t rxdataF[][rxdataF_sz],
                          const uint32_t rx_size,
                          const uint32_t pdsch_est_size,
                          int32_t dl_ch_estimates[][pdsch_est_size],
                          int32_t rxdataF_ext[][rx_size],
                          int32_t dl_ch_estimates_ext[][rx_size],
                          unsigned char symbol,
                          uint8_t pilots,
                          uint8_t config_type,
                          unsigned short start_rb,
                          unsigned short nb_rb_pdsch,
                          uint8_t n_dmrs_cdm_groups,
                          uint8_t Nl,
                          NR_DL_FRAME_PARMS *frame_parms,
                          uint16_t dlDmrsSymbPos,
                          int chest_time_type);

/** \brief This function performs channel compensation (matched filtering) on the received RBs for this allocation.  In addition, it computes the squared-magnitude of the channel with weightings for 16QAM/64QAM detection as well as dual-stream detection (cross-correlation)
    @param rxdataF_ext Frequency-domain received signal in RBs to be demodulated
    @param dl_ch_estimates_ext Frequency-domain channel estimates in RBs to be demodulated
    @param dl_ch_mag First Channel magnitudes (16QAM/64QAM)
    @param dl_ch_magb Second weighted Channel magnitudes (64QAM)
    @param rxdataF_comp Compensated received waveform
    @param rho Cross-correlation between two spatial channels on each RX antenna
    @param frame_parms Pointer to frame descriptor
    @param symbol Symbol on which to operate
    @param first_symbol_flag set to 1 on first DLSCH symbol
    @param mod_order Modulation order of allocation
    @param nb_rb Number of RBs in allocation
    @param output_shift Rescaling for compensated output (should be energy-normalizing)
    @param phy_measurements Pointer to UE PHY measurements
*/
void nr_dlsch_channel_compensation(uint32_t rx_size,
                                   int32_t rxdataF_ext[][rx_size],
                                   int32_t dl_ch_estimates_ext[][rx_size],
                                   int32_t dl_ch_mag[][rx_size],
                                   int32_t dl_ch_magb[][rx_size],
                                   int32_t dl_ch_magr[][rx_size],
                                   int32_t rxdataF_comp[][rx_size],
                                   int ***rho,
                                   NR_DL_FRAME_PARMS *frame_parms,
                                   uint8_t nb_aatx,
                                   unsigned char symbol,
                                   int length,
                                   uint8_t first_symbol_flag,
                                   unsigned char mod_order,
                                   unsigned short nb_rb,
                                   unsigned char output_shift,
                                   PHY_NR_MEASUREMENTS *measurements);

void nr_dlsch_channel_compensation_core(int **rxdataF_ext,
                                     int **dl_ch_estimates_ext,
                                     int **dl_ch_mag,
                                     int **dl_ch_magb,
                                     int **rxdataF_comp,
                                     int ***rho,
                                     unsigned char n_tx,
                                     unsigned char n_rx,
                                     unsigned char mod_order,
                                     unsigned char output_shift,
                                     int length,
                                     int start_point);

void nr_dlsch_deinterleaving(uint8_t symbol,
                             uint8_t start_symbol,
                             uint16_t L,
                             uint16_t *llr,
                             uint16_t *llr_deint,
                             uint16_t nb_rb_pdsch);

void nr_dlsch_channel_level_median(uint32_t rx_size,
                                   int32_t dl_ch_estimates[][rx_size],
                                   int32_t *median,
                                   int n_tx,
                                   int n_rx,
                                   int length,
                                   int start_point);

void nr_dlsch_detection_mrc(uint32_t rx_size,
                            int32_t rxdataF_comp[][rx_size],
                            int ***rho,
                            int32_t dl_ch_mag[][rx_size],
                            int32_t dl_ch_magb[][rx_size],
                            int32_t dl_ch_magr[][rx_size],
                            short n_tx,
                            short n_rx,
                            unsigned char symbol,
                            unsigned short nb_rb,
                            int length);

void nr_conjch0_mult_ch1(int *ch0,
                         int *ch1,
                         int32_t *ch0conj_ch1,
                         unsigned short nb_rb,
                         unsigned char output_shift0);

void nr_a_sum_b(__m128i *input_x,
                __m128i *input_y,
                unsigned short nb_rb);


/** \brief This function computes the average channel level over all allocated RBs and antennas (TX/RX) in order to compute output shift for compensated signal
    @param dl_ch_estimates_ext Channel estimates in allocated RBs
    @param frame_parms Pointer to frame descriptor
    @param avg Pointer to average signal strength
    @param pilots_flag Flag to indicate pilots in symbol
    @param nb_rb Number of allocated RBs
*/
void nr_dlsch_channel_level(uint32_t rx_size,
                            int32_t dl_ch_estimates[][rx_size],
			                      NR_DL_FRAME_PARMS *frame_parms,
			                      uint8_t n_tx,
			                      int32_t *avg,
			                      uint8_t symbol,
			                      uint32_t len,
			                      unsigned short nb_rb);

void nr_dlsch_scale_channel(uint32_t rx_size,
                            int32_t dl_ch_estimates[][rx_size],
			                      NR_DL_FRAME_PARMS *frame_parms,
			                      uint8_t n_tx,
			                      uint8_t n_rx,
			                      uint8_t symbol,
			                      uint8_t pilots,
			                      uint32_t len,
			                      unsigned short nb_rb);

/** \brief This is the top-level entry point for DLSCH decoding in UE.  It should be replicated on several
    threads (on multi-core machines) corresponding to different HARQ processes. The routine first
    computes the segmentation information, followed by rate dematching and sub-block deinterleaving the of the
    received LLRs computed by dlsch_demodulation for each transport block segment. It then calls the
    turbo-decoding algorithm for each segment and stops after either after unsuccesful decoding of at least
    one segment or correct decoding of all segments.  Only the segment CRCs are check for the moment, the
    overall CRC is ignored.  Finally transport block reassembly is performed.
    @param phy_vars_ue Pointer to ue variables
    @param dlsch_llr Pointer to LLR values computed by dlsch_demodulation
    @param lte_frame_parms Pointer to frame descriptor
    @param dlsch Pointer to DLSCH descriptor
    @param frame Frame number
    @param nr_slot_rx Slot number
    @param num_pdcch_symbols Number of PDCCH symbols
    @param is_crnti indicates if PDSCH belongs to a CRNTI (necessary for parallelizing decoding threads)
    @param llr8_flag If 1, indicate that the 8-bit turbo decoder should be used
    @returns 0 on success, 1 on unsuccessful decoding
*/

uint32_t  nr_dlsch_decoding(PHY_VARS_NR_UE *phy_vars_ue,
                         UE_nr_rxtx_proc_t *proc,
                         int eNB_id,
                         short *dlsch_llr,
                         NR_DL_FRAME_PARMS *frame_parms,
                         NR_UE_DLSCH_t *dlsch,
                         NR_DL_UE_HARQ_t *harq_process,
                         uint32_t frame,
                         uint16_t nb_symb_sch,
                         uint8_t nr_slot_rx,
                         uint8_t harq_pid);

int nr_ulsch_encoding(PHY_VARS_NR_UE *ue,
                     NR_UE_ULSCH_t *ulsch,
                     NR_DL_FRAME_PARMS* frame_parms,
                     uint8_t harq_pid,
                     unsigned int G);

/*! \brief Perform PUSCH scrambling. TS 38.211 V15.4.0 subclause 6.3.1.1
  @param[in] in, Pointer to input bits
  @param[in] size, of input bits
  @param[in] Nid, cell id
  @param[in] n_RNTI, CRNTI
  @param[in] uci_on_pusch whether UCI placeholder bits need to be scrambled (true -> no optimized scrambling)
  @param[out] out, the scrambled bits
*/
void nr_pusch_codeword_scrambling(uint8_t *in,
                                  uint32_t size,
                                  uint32_t Nid,
                                  uint32_t n_RNTI,
                                  bool uci_on_pusch,
                                  uint32_t* out);

/** \brief Perform the following functionalities:
    - encoding
    - scrambling
    - modulation
    - transform precoding
*/

void nr_ue_ulsch_procedures(PHY_VARS_NR_UE *UE,
                               unsigned char harq_pid,
                               uint32_t frame,
                               uint8_t slot,
                               int gNB_id,
                               nr_phy_data_tx_t *phy_data);


/** \brief This function does IFFT for PUSCH
*/

uint8_t nr_ue_pusch_common_procedures(PHY_VARS_NR_UE *UE,
                                      uint8_t slot,
                                      NR_DL_FRAME_PARMS *frame_parms,
                                      uint8_t Nl);

void nr_dlsch_unscrambling(int16_t* llr,
			   uint32_t size,
			   uint8_t q,
			   uint32_t Nid,
			   uint32_t n_RNTI);

int32_t nr_rx_pdcch(PHY_VARS_NR_UE *ue,
                    UE_nr_rxtx_proc_t *proc,
                    int32_t pdcch_est_size,
                    int32_t pdcch_dl_ch_estimates[][pdcch_est_size],
                    int16_t *pdcch_e_rx,
                    fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15,
                    c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]);


/*! \brief Performs detection of SSS to find cell ID and other framing parameters (FDD/TDD, normal/extended prefix)
  @param phy_vars_ue Pointer to UE variables
  @param tot_metric Pointer to variable containing maximum metric under framing hypothesis (to be compared to other hypotheses
  @param flip_max Pointer to variable indicating if start of frame is in second have of RX buffer (i.e. PSS/SSS is flipped)
  @param phase_max Pointer to variable (0 ... 6) containing rought phase offset between PSS and SSS (can be used for carrier
  frequency adjustment. 0 means -pi/3, 6 means pi/3.
  @returns 0 on success
*/
int rx_sss(PHY_VARS_NR_UE *phy_vars_ue,int32_t *tot_metric,uint8_t *flip_max,uint8_t *phase_max);

/*! \brief receiver for the PBCH
  \returns number of tx antennas or -1 if error
*/
int nr_rx_pbch(PHY_VARS_NR_UE *ue,
               UE_nr_rxtx_proc_t *proc,
               const int estimateSz,
               struct complex16 dl_ch_estimates[][estimateSz],
               NR_UE_PBCH *nr_ue_pbch_vars,
               NR_DL_FRAME_PARMS *frame_parms,
               uint8_t i_ssb,
               MIMO_mode_t mimo_mode,
               nr_phy_data_t *phy_data,
               fapiPbch_t* result,
               c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]);

int nr_pbch_detection(UE_nr_rxtx_proc_t *proc,
                      PHY_VARS_NR_UE *ue,
                      int pbch_initial_symbol,
                      nr_phy_data_t *phy_data,
                      c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]);


#ifndef modOrder
#define modOrder(I_MCS,I_TBS) ((I_MCS-I_TBS)*2+2) // Find modulation order from I_TBS and I_MCS
#endif

int dump_ue_stats(PHY_VARS_NR_UE *phy_vars_ue, UE_nr_rxtx_proc_t *proc, char* buffer, int length, runmode_t mode, int input_level_dBm);

/*!
  \brief This function performs the initial cell search procedure - PSS detection, SSS detection and PBCH detection.  At the
  end, the basic frame parameters are known (Frame configuration - TDD/FDD and cyclic prefix length,
  N_RB_DL, PHICH_CONFIG and Nid_cell) and the UE can begin decoding PDCCH and DLSCH SI to retrieve the rest.  Once these
  parameters are know, the routine calls some basic initialization routines (cell-specific reference signals, etc.)
  @param phy_vars_ue Pointer to UE variables
  @param mode current running mode
*/
int nr_initial_sync(UE_nr_rxtx_proc_t *proc,
                    PHY_VARS_NR_UE *phy_vars_ue, 
                    int n_frames,
                    int sa);

/*!
  \brief This function gets the carrier frequencies either from FP or command-line-set global variables, depending on the availability of the latter
  @param fp         Pointer to frame params
  @param dl_Carrier Pointer to DL carrier to be set
  @param ul_Carrier Pointer to UL carrier to be set
*/
void nr_get_carrier_frequencies(PHY_VARS_NR_UE *ue,
                                uint64_t *dl_Carrier,
                                uint64_t *ul_Carrier);

/*!
  \brief This function sets the OAI RF card rx/tx params
  @param openair0_cfg   Pointer OAI config for a specific card
  @param tx_gain_off    Tx gain offset
  @param rx_gain_off    Rx gain offset
  @param ul_Carrier     UL carrier to be set
  @param dl_Carrier     DL carrier to be set
  @param freq_offset    Freq offset to be set
*/
void nr_rf_card_config_gain(openair0_config_t *openair0_cfg,
                            double rx_gain_off);

void nr_rf_card_config_freq(openair0_config_t *openair0_cfg,
                            uint64_t ul_Carrier,
                            uint64_t dl_Carrier,
                            int freq_offset);

void nr_pdcch_unscrambling(int16_t *z,
                           uint16_t scrambling_RNTI,
                           uint32_t length,
                           uint16_t pdcch_DMRS_scrambling_id,
                           int16_t *z2);

uint8_t nr_dci_decoding_procedure(PHY_VARS_NR_UE *ue,
                                  UE_nr_rxtx_proc_t *proc,
                                  int16_t *pdcch_e_rx,
                                  fapi_nr_dci_indication_t *dci_ind,
                                  fapi_nr_dl_config_dci_dl_pdu_rel15_t *rel15);


/** \brief This function is the top-level entry point to PDSCH demodulation, after frequency-domain transformation and channel estimation.  It performs
    - RB extraction (signal and channel estimates)
    - channel compensation (matched filtering)
    - RE extraction (pilot, PBCH, synch. signals)
    - antenna combining (MRC, Alamouti, cycling)
    - LLR computation
    This function supports TM1, 2, 3, 5, and 6.
    @param ue Pointer to PHY variables
    @param type Type of PDSCH (SI_PDSCH,RA_PDSCH,PDSCH,PMCH)
    @param eNB_id eNb index (Nid1) 0,1,2
    @param eNB_id_i Interfering eNB index (Nid1) 0,1,2, or 3 in case of MU-MIMO IC receiver
    @param frame Frame number
    @param nr_slot_rx Slot number
    @param symbol Symbol on which to act (within sub-frame)
    @param first_symbol_flag set to 1 on first DLSCH symbol
*/
int nr_rx_pdsch(PHY_VARS_NR_UE *ue,
                UE_nr_rxtx_proc_t *proc,
                NR_UE_DLSCH_t dlsch[2],
                unsigned char symbol,
                unsigned char first_symbol_flag,
                unsigned char harq_pid,
                uint32_t pdsch_est_size,
                int32_t dl_ch_estimates[][pdsch_est_size],
                int16_t *llr[2],
                c16_t ptrs_phase_per_slot[][NR_SYMBOLS_PER_SLOT],
                int32_t ptrs_re_per_slot[][NR_SYMBOLS_PER_SLOT],
                uint32_t dl_valid_re[NR_SYMBOLS_PER_SLOT],
                uint32_t rx_size,
                int32_t dl_ch_estimates_ext[][rx_size],
                int32_t rxdataF_ext[][rx_size],
                int32_t rxdataF_comp[][rx_size],
                int32_t dl_ch_mag[][rx_size],
                int32_t dl_ch_magb[][rx_size],
                int32_t dl_ch_magr[][rx_size],
                c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP],
                uint32_t llr_offset[NR_SYMBOLS_PER_SLOT],
                int32_t *log2_maxh);

int32_t generate_nr_prach(PHY_VARS_NR_UE *ue, uint8_t gNB_id, int frame, uint8_t slot);

void dump_nrdlsch(PHY_VARS_NR_UE *ue,uint8_t gNB_id,uint8_t nr_slot_rx,unsigned int *coded_bits_per_codeword,int round,  unsigned char harq_pid);
/**@}*/
#endif

