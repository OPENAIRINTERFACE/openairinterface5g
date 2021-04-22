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

#define NR_PUSCH_x 2 // UCI placeholder bit TS 38.212 V15.4.0 subclause 5.3.3.1
#define NR_PUSCH_y 3 // UCI placeholder bit

// Functions below implement 36-211 and 36-212

/** @addtogroup _PHY_TRANSPORT_
 * @{
 */

/** \fn free_ue_dlsch(NR_UE_DLSCH_t *dlsch)
    \brief This function frees memory allocated for a particular DLSCH at UE
    @param dlsch Pointer to DLSCH to be removed
*/
void free_nr_ue_dlsch(NR_UE_DLSCH_t **dlsch,uint8_t N_RB_DL);


/** \fn new_ue_dlsch(uint8_t Kmimo,uint8_t Mdlharq,uint32_t Nsoft,uint8_t abstraction_flag)
    \brief This function allocates structures for a particular DLSCH at UE
    @returns Pointer to DLSCH to be removed
    @param Kmimo Kmimo factor from 36-212/36-213
    @param Mdlharq Maximum number of HARQ rounds (36-212/36-213)
    @param Nsoft Soft-LLR buffer size from UE-Category
    @params N_RB_DL total number of resource blocks (determine the operating BW)
    @param abstraction_flag Flag to indicate abstracted interface
*/
NR_UE_DLSCH_t *new_nr_ue_dlsch(uint8_t Kmimo,uint8_t Mdlharq,uint32_t Nsoft,uint8_t max_turbo_iterations,uint16_t N_RB_DL, uint8_t abstraction_flag);


void free_nr_ue_ulsch(NR_UE_ULSCH_t **ulsch,unsigned char N_RB_UL);

NR_UE_ULSCH_t *new_nr_ue_ulsch(uint16_t N_RB_UL, int number_of_harq_pids, uint8_t abstraction_flag);

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
                   uint16_t nb_rb,
                   uint8_t beamforming_mode);

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

int32_t nr_dlsch_qpsk_llr_SIC(NR_DL_FRAME_PARMS *frame_parms,
                           int **rxdataF_comp,
                           int32_t **sic_buffer,
                           int **rho_i,
                           short *dlsch_llr,
                           uint8_t num_pdcch_symbols,
                           uint16_t nb_rb,
                           uint8_t subframe,
                           uint16_t mod_order_0,
                           uint32_t rb_alloc);

void nr_dlsch_16qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                     int32_t *rxdataF_comp,
                     int16_t *dlsch_llr,
                     int32_t *dl_ch_mag,
                     uint8_t symbol,
                     uint32_t len,
                     uint8_t first_symbol_flag,
                     uint16_t nb_rb,
                     uint8_t beamforming_mode);
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
void nr_dlsch_16qam_llr_SIC (NR_DL_FRAME_PARMS *frame_parms,
                          int32_t **rxdataF_comp,
                          int32_t **sic_buffer,  //Q15
                          int32_t **rho_i,
                          int16_t *dlsch_llr,
                          uint8_t num_pdcch_symbols,
                          int32_t **dl_ch_mag,
                          uint16_t nb_rb,
                          uint8_t subframe,
                          uint16_t mod_order_0,
                          uint32_t rb_alloc);

void dlsch_64qam_llr_SIC(NR_DL_FRAME_PARMS *frame_parms,
                         int32_t **rxdataF_comp,
                         int32_t **sic_buffer,  //Q15
                         int32_t **rho_i,
                         int16_t *dlsch_llr,
                         uint8_t num_pdcch_symbols,
                         int32_t **dl_ch_mag,
                         int32_t **dl_ch_magb,
                         uint16_t nb_rb,
                         uint8_t subframe,
                         uint16_t mod_order_0,
                         uint32_t rb_alloc);

void nr_dlsch_64qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                     int32_t *rxdataF_comp,
                     int16_t *dlsch_llr,
                     int32_t *dl_ch_mag,
                     int32_t *dl_ch_magb,
                     uint8_t symbol,
                     uint32_t len,
                     uint8_t first_symbol_flag,
                     uint16_t nb_rb,
                     uint8_t beamforming_mode);

void nr_dlsch_256qam_llr(NR_DL_FRAME_PARMS *frame_parms,
                     int32_t *rxdataF_comp,
                     int16_t *dlsch_llr,
                     int32_t *dl_ch_mag,
                     int32_t *dl_ch_magb,
                     int32_t *dl_ch_magr,
                     uint8_t symbol,
                     uint32_t len,
                     uint8_t first_symbol_flag,
                     uint16_t nb_rb,
                     uint8_t beamforming_mode);

/** \fn dlsch_siso(NR_DL_FRAME_PARMS *frame_parms,
    int32_t **rxdataF_comp,
    int32_t **rxdataF_comp_i,
    uint8_t l,
    uint16_t nb_rb)
    \brief This function does the first stage of llr computation for SISO, by just extracting the pilots, PBCH and primary/secondary synchronization sequences.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param l symbol in sub-frame
    @param nb_rb Number of RBs in this allocation
*/

void dlsch_siso(NR_DL_FRAME_PARMS *frame_parms,
                int32_t **rxdataF_comp,
                int32_t **rxdataF_comp_i,
                uint8_t l,
                uint16_t nb_rb);

/** \fn dlsch_alamouti(NR_DL_FRAME_PARMS *frame_parms,
    int32_t **rxdataF_comp,
    int32_t **dl_ch_mag,
    int32_t **dl_ch_magb,
    uint8_t symbol,
    uint16_t nb_rb)
    \brief This function does Alamouti combining on RX and prepares LLR inputs by skipping pilots, PBCH and primary/secondary synchronization signals.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param dl_ch_mag First squared-magnitude of channel (16QAM and 64QAM) for LLR computation.  Alamouti combining should be performed on this as well. Result is stored in first antenna position
    @param dl_ch_magb Second squared-magnitude of channel (64QAM only) for LLR computation.  Alamouti combining should be performed on this as well. Result is stored in first antenna position
    @param symbol Symbol in sub-frame
    @param nb_rb Number of RBs in this allocation
*/
void dlsch_alamouti(NR_DL_FRAME_PARMS *frame_parms,
                    int32_t **rxdataF_comp,
                    int32_t **dl_ch_mag,
                    int32_t **dl_ch_magb,
                    uint8_t symbol,
                    uint16_t nb_rb);

/** \fn dlsch_antcyc(NR_DL_FRAME_PARMS *frame_parms,
    int32_t **rxdataF_comp,
    int32_t **dl_ch_mag,
    int32_t **dl_ch_magb,
    uint8_t symbol,
    uint16_t nb_rb)
    \brief This function does antenna selection (based on antenna cycling pattern) on RX and prepares LLR inputs by skipping pilots, PBCH and primary/secondary synchronization signals.  Note that this is not LTE, it is just included for comparison purposes.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param dl_ch_mag First squared-magnitude of channel (16QAM and 64QAM) for LLR computation.  Alamouti combining should be performed on this as well. Result is stored in first antenna position
    @param dl_ch_magb Second squared-magnitude of channel (64QAM only) for LLR computation.  Alamouti combining should be performed on this as well. Result is stored in first antenna position
    @param symbol Symbol in sub-frame
    @param nb_rb Number of RBs in this allocation
*/
void dlsch_antcyc(NR_DL_FRAME_PARMS *frame_parms,
                  int32_t **rxdataF_comp,
                  int32_t **dl_ch_mag,
                  int32_t **dl_ch_magb,
                  uint8_t symbol,
                  uint16_t nb_rb);

/** \fn dlsch_detection_mrc(NR_DL_FRAME_PARMS *frame_parms,
    int32_t **rxdataF_comp,
    int32_t **rxdataF_comp_i,
    int32_t **rho,
    int32_t **rho_i,
    int32_t **dl_ch_mag,
    int32_t **dl_ch_magb,
    uint8_t symbol,
    uint16_t nb_rb,
    uint8_t dual_stream_UE)

    \brief This function does maximal-ratio combining for dual-antenna receivers.
    @param frame_parms Frame descriptor structure
    @param rxdataF_comp Compensated channel output
    @param rxdataF_comp_i Compensated channel output for interference
    @param rho Cross correlation between spatial channels
    @param rho_i Cross correlation between signal and inteference channels
    @param dl_ch_mag First squared-magnitude of channel (16QAM and 64QAM) for LLR computation.  Alamouti combining should be performed on this as well. Result is stored in first antenna position
    @param dl_ch_magb Second squared-magnitude of channel (64QAM only) for LLR computation.  Alamouti combining should be performed on this as well. Result is stored in first antenna position
    @param symbol Symbol in sub-frame
    @param nb_rb Number of RBs in this allocation
    @param dual_stream_UE Flag to indicate dual-stream detection
*/
void dlsch_detection_mrc(NR_DL_FRAME_PARMS *frame_parms,
                         int32_t **rxdataF_comp,
                         int32_t **rxdataF_comp_i,
                         int32_t **rho,
                         int32_t **rho_i,
                         int32_t **dl_ch_mag,
                         int32_t **dl_ch_magb,
                         int32_t **dl_ch_mag_i,
                         int32_t **dl_ch_magb_i,
                         uint8_t symbol,
                         uint16_t nb_rb,
                         uint8_t dual_stream_UE);

void dlsch_detection_mrc_TM34(NR_DL_FRAME_PARMS *frame_parms,
                              NR_UE_PDSCH *lte_ue_pdsch_vars,
                              int harq_pid,
                              int round,
                              unsigned char symbol,
                              unsigned short nb_rb,
                              unsigned char dual_stream_UE);

/** \fn dlsch_extract_rbs_single(int32_t **rxdataF,
    int32_t **dl_ch_estimates,
    int32_t **rxdataF_ext,
    int32_t **dl_ch_estimates_ext,
    uint32_t *rb_alloc,
    uint8_t symbol,
    NR_DL_FRAME_PARMS *frame_parms)
    \brief This function extracts the received resource blocks, both channel estimates and data symbols,
    for the current allocation and for single antenna eNB transmission.
    @param rxdataF Raw FFT output of received signal
    @param dl_ch_estimates Channel estimates of current slot
    @param rxdataF_ext FFT output for RBs in this allocation
    @param dl_ch_estimates_ext Channel estimates for RBs in this allocation
    @param rb_alloc RB allocation vector
    @param symbol Symbol to extract
    @param n_dmrs_cdm_groups
    @param frame_parms Pointer to frame descriptor
*/
unsigned short nr_dlsch_extract_rbs_single(int **rxdataF,
                                        int **dl_ch_estimates,
                                        int **rxdataF_ext,
                                        int **dl_ch_estimates_ext,
                                        unsigned char symbol,
                                        uint8_t pilots,
                                        uint8_t config_type,
                                        unsigned short start_rb,
                                        unsigned short nb_rb_pdsch,
                                        uint8_t n_dmrs_cdm_groups,
                                        NR_DL_FRAME_PARMS *frame_parms,
                                        uint16_t dlDmrsSymbPos);

/** \fn dlsch_extract_rbs_multiple(int32_t **rxdataF,
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
unsigned short nr_dlsch_extract_rbs_multiple(int **rxdataF,
                                        int **dl_ch_estimates,
                                        int **rxdataF_ext,
                                        int **dl_ch_estimates_ext,
                                        unsigned char symbol,
                                        uint8_t pilots,
                                        uint8_t config_type,
                                        unsigned short start_rb,
                                        unsigned short nb_rb_pdsch,
                                        uint8_t n_dmrs_cdm_groups,
                                        uint8_t Nl,
                                        NR_DL_FRAME_PARMS *frame_parms,
                                        uint16_t dlDmrsSymbPos);

/** \fn dlsch_extract_rbs_TM7(int32_t **rxdataF,
    int32_t **dl_bf_ch_estimates,
    int32_t **rxdataF_ext,
    int32_t **dl_bf_ch_estimates_ext,
    uint32_t *rb_alloc,
    uint8_t symbol,
    uint8_t subframe,
    uint32_t high_speed_flag,
    NR_DL_FRAME_PARMS *frame_parms)
    \brief This function extracts the received resource blocks, both channel estimates and data symbols,
    for the current allocation and for single antenna eNB transmission.
    @param rxdataF Raw FFT output of received signal
    @param dl_bf_ch_estimates Beamforming channel estimates of current slot
    @param rxdataF_ext FFT output for RBs in this allocation
    @param dl_bf_ch_estimates_ext Beamforming channel estimates for RBs in this allocation
    @param rb_alloc RB allocation vector
    @param symbol Symbol to extract
    @param subframe Subframe number
    @param high_speed_flag
    @param frame_parms Pointer to frame descriptor
*/
uint16_t dlsch_extract_rbs_TM7(int32_t **rxdataF,
                               int32_t **dl_bf_ch_estimates,
                               int32_t **rxdataF_ext,
                               int32_t **dl_bf_ch_estimates_ext,
                               uint32_t *rb_alloc,
                               uint8_t symbol,
                               uint8_t subframe,
                               uint32_t high_speed_flag,
                               NR_DL_FRAME_PARMS *frame_parms);

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
void nr_dlsch_channel_compensation(int32_t **rxdataF_ext,
                                int32_t **dl_ch_estimates_ext,
                                int32_t **dl_ch_mag,
                                int32_t **dl_ch_magb,
                                int32_t **dl_ch_magr,
                                int32_t **rxdataF_comp,
                                int32_t ***rho,
                                NR_DL_FRAME_PARMS *frame_parms,
                                uint8_t nb_aatx,
                                uint8_t symbol,
                                int length,
                                uint8_t first_symbol_flag,
                                uint8_t mod_order,
                                uint16_t nb_rb,
                                uint8_t output_shift,
                                PHY_NR_MEASUREMENTS *phy_measurements);

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

void dlsch_dual_stream_correlation(NR_DL_FRAME_PARMS *frame_parms,
                                   unsigned char symbol,
                                   unsigned short nb_rb,
                                   int **dl_ch_estimates_ext,
                                   int **dl_ch_estimates_ext_i,
                                   int **dl_ch_rho_ext,
                                   unsigned char output_shift);

void dlsch_dual_stream_correlationTM34(NR_DL_FRAME_PARMS *frame_parms,
                                   unsigned char symbol,
                                   unsigned short nb_rb,
                                   int **dl_ch_estimates_ext,
                                   int **dl_ch_estimates_ext_i,
                                   int **dl_ch_rho_ext,
                                   unsigned char output_shift0,
                                   unsigned char output_shift1);
//This function is used to compute multiplications in Hhermitian * H matrix
void conjch0_mult_ch1(int *ch0,
                      int *ch1,
                      int32_t *ch0conj_ch1,
                      unsigned short nb_rb,
                      unsigned char output_shift0);

void construct_HhH_elements(int *ch0conj_ch0,
                         int *ch1conj_ch1,
                         int *ch2conj_ch2,
                         int *ch3conj_ch3,
                         int *ch0conj_ch1,
                         int *ch1conj_ch0,
                         int *ch2conj_ch3,
                         int *ch3conj_ch2,
                         int32_t *after_mf_00,
                         int32_t *after_mf_01,
                         int32_t *after_mf_10,
                         int32_t *after_mf_11,
                         unsigned short nb_rb);

void squared_matrix_element(int32_t *Hh_h_00,
                            int32_t *Hh_h_00_sq,
                            unsigned short nb_rb);

void dlsch_channel_level_TM34_meas(int *ch00,
                                   int *ch01,
                                   int *ch10,
                                   int *ch11,
                                   int *avg_0,
                                   int *avg_1,
                                   unsigned short nb_rb);

void nr_dlsch_channel_level_median(int **dl_ch_estimates_ext,
                                int32_t *median,
                                int n_tx,
                                int n_rx,
                                int length,
                                int start_point);

void nr_dlsch_detection_mrc(int **rxdataF_comp,
                            int ***rho,
                            int **dl_ch_mag,
                            int **dl_ch_magb,
                            int **dl_ch_magr,
                            short n_tx,
                            short n_rx,
                            unsigned char symbol,
                            unsigned short nb_rb,
                            int length);

void nr_dlsch_detection_mrc_core(int **rxdataF_comp,
                              int **rxdataF_comp_i,
                              int **rho,
                              int **rho_i,
                              int **dl_ch_mag,
                              int **dl_ch_magb,
                              int **dl_ch_mag_i,
                              int **dl_ch_magb_i,
                              unsigned char n_tx,
                              unsigned char n_rx,
                              int length,
                              int start_point);

void det_HhH(int32_t *after_mf_00,
             int32_t *after_mf_01,
             int32_t *after_mf_10,
             int32_t *after_mf_11,
             int32_t *det_fin_128,
             unsigned short nb_rb);

void numer(int32_t *Hh_h_00_sq,
           int32_t *Hh_h_01_sq,
           int32_t *Hh_h_10_sq,
           int32_t *Hh_h_11_sq,
           int32_t *num_fin,
           unsigned short nb_rb);

uint8_t rank_estimation_tm3_tm4(int *dl_ch_estimates_00,
                                int *dl_ch_estimates_01,
                                int *dl_ch_estimates_10,
                                int *dl_ch_estimates_11,
                                unsigned short nb_rb);

void dlsch_channel_compensation_TM56(int **rxdataF_ext,
                                     int **dl_ch_estimates_ext,
                                     int **dl_ch_mag,
                                     int **dl_ch_magb,
                                     int **rxdataF_comp,
                                     unsigned char *pmi_ext,
                                     NR_DL_FRAME_PARMS *frame_parms,
                                     PHY_NR_MEASUREMENTS *phy_measurements,
                                     int eNB_id,
                                     unsigned char symbol,
                                     unsigned char mod_order,
                                     unsigned short nb_rb,
                                     unsigned char output_shift,
                                     unsigned char dl_power_off);


void dlsch_channel_compensation_TM34(NR_DL_FRAME_PARMS *frame_parms,
                                    NR_UE_PDSCH *lte_ue_pdsch_vars,
                                    PHY_NR_MEASUREMENTS *phy_measurements,
                                    int eNB_id,
                                    unsigned char symbol,
                                    unsigned char mod_order0,
                                    unsigned char mod_order1,
                                    int harq_pid,
                                    int round,
                                    MIMO_mode_t mimo_mode,
                                    unsigned short nb_rb,
                                    unsigned char output_shift0,
                                    unsigned char output_shift1);


/** \brief This function computes the average channel level over all allocated RBs and antennas (TX/RX) in order to compute output shift for compensated signal
    @param dl_ch_estimates_ext Channel estimates in allocated RBs
    @param frame_parms Pointer to frame descriptor
    @param avg Pointer to average signal strength
    @param pilots_flag Flag to indicate pilots in symbol
    @param nb_rb Number of allocated RBs
*/
void nr_dlsch_channel_level(int **dl_ch_estimates_ext,
                         NR_DL_FRAME_PARMS *frame_parms,
                         uint8_t n_tx,
                         int32_t *avg,
                         uint8_t symbol,
						 uint32_t len,
                         unsigned short nb_rb);


void dlsch_channel_level_TM34(int **dl_ch_estimates_ext,
                              NR_DL_FRAME_PARMS *frame_parms,
                              unsigned char *pmi_ext,
                              int *avg_0,
                              int *avg_1,
                              uint8_t symbol,
                              unsigned short nb_rb,
                              MIMO_mode_t mimo_mode);


void dlsch_channel_level_TM56(int32_t **dl_ch_estimates_ext,
                              NR_DL_FRAME_PARMS *frame_parms,
                              unsigned char *pmi_ext,
                              int32_t *avg,
                              uint8_t symbol_mod,
                              uint16_t nb_rb);

void dlsch_channel_level_TM7(int32_t **dl_bf_ch_estimates_ext,
                         NR_DL_FRAME_PARMS *frame_parms,
                         int32_t *avg,
                         uint8_t pilots_flag,
                         uint16_t nb_rb);

void nr_dlsch_scale_channel(int32_t **dl_ch_estimates_ext,
                         NR_DL_FRAME_PARMS *frame_parms,
                         uint8_t n_tx,
                         uint8_t n_rx,
                         NR_UE_DLSCH_t **dlsch_ue,
                         uint8_t symbol,
                         uint8_t start_symbol,
                         uint32_t len,
                         uint16_t nb_rb);

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
                         uint8_t harq_pid,
                         uint8_t is_crnti,
                         uint8_t llr8_flag);

int nr_ulsch_encoding(NR_UE_ULSCH_t *ulsch,
                     NR_DL_FRAME_PARMS* frame_parms,
                     uint8_t harq_pid,
                     unsigned int G);

/*! \brief Perform PUSCH scrambling. TS 38.211 V15.4.0 subclause 6.3.1.1
  @param[in] in, Pointer to input bits
  @param[in] size, of input bits
  @param[in] Nid, cell id
  @param[in] n_RNTI, CRNTI
  @param[out] out, the scrambled bits
*/

void nr_pusch_codeword_scrambling(uint8_t *in,
                         uint32_t size,
                         uint32_t Nid,
                         uint32_t n_RNTI,
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
                               uint8_t thread_id,
                               int gNB_id);


/** \brief This function does IFFT for PUSCH
*/

uint8_t nr_ue_pusch_common_procedures(PHY_VARS_NR_UE *UE,
                                      uint8_t slot,
                                      NR_DL_FRAME_PARMS *frame_parms,
                                      uint8_t Nl);



uint32_t  nr_dlsch_decoding_mthread(PHY_VARS_NR_UE *phy_vars_ue,
                         UE_nr_rxtx_proc_t *proc,
                         int eNB_id,
                         short *dlsch_llr,
                         NR_DL_FRAME_PARMS *frame_parms,
                         NR_UE_DLSCH_t *dlsch,
                         NR_DL_UE_HARQ_t *harq_process,
                         uint32_t frame,
                         uint16_t nb_symb_sch,
                         uint8_t nr_slot_rx,
                         uint8_t harq_pid,
                         uint8_t is_crnti,
                         uint8_t llr8_flag);

void *nr_dlsch_decoding_2thread0(void *arg);

void *nr_dlsch_decoding_2thread1(void *arg);


void nr_dlsch_unscrambling(int16_t* llr,
			   uint32_t size,
			   uint8_t q,
			   uint32_t Nid,
			   uint32_t n_RNTI);

uint32_t dlsch_decoding_emul(PHY_VARS_NR_UE *phy_vars_ue,
                             uint8_t subframe,
                             PDSCH_t dlsch_id,
                             uint8_t eNB_id);

int32_t nr_rx_pdcch(PHY_VARS_NR_UE *ue,
                    UE_nr_rxtx_proc_t *proc);


/*! \brief Extract PSS and SSS resource elements
  @param phy_vars_ue Pointer to UE variables
  @param[out] pss_ext contain the PSS signals after the extraction
  @param[out] sss_ext contain the SSS signals after the extraction
  @returns 0 on success
*/
int pss_sss_extract(PHY_VARS_NR_UE *phy_vars_ue,
                    int32_t pss_ext[4][72],
                    int32_t sss_ext[4][72],
                                        uint8_t subframe);

/*! \brief Extract only PSS resource elements
  @param phy_vars_ue Pointer to UE variables
  @param[out] pss_ext contain the PSS signals after the extraction
  @returns 0 on success
*/
int pss_only_extract(PHY_VARS_NR_UE *phy_vars_ue,
                    int32_t pss_ext[4][72],
                    uint8_t subframe);

/*! \brief Extract only SSS resource elements
  @param phy_vars_ue Pointer to UE variables
  @param[out] sss_ext contain the SSS signals after the extraction
  @returns 0 on success
*/
int sss_only_extract(PHY_VARS_NR_UE *phy_vars_ue,
                    int32_t sss_ext[4][72],
                    uint8_t subframe);

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
int nr_rx_pbch( PHY_VARS_NR_UE *ue,
		     UE_nr_rxtx_proc_t *proc,
		     NR_UE_PBCH *nr_ue_pbch_vars,
		     NR_DL_FRAME_PARMS *frame_parms,
		     uint8_t eNB_id,
                     uint8_t i_ssb,
		     MIMO_mode_t mimo_mode,
		     uint32_t high_speed_flag);

int nr_pbch_detection(UE_nr_rxtx_proc_t *proc,
		      PHY_VARS_NR_UE *ue,
                      int pbch_initial_symbol);

uint16_t rx_pbch_emul(PHY_VARS_NR_UE *phy_vars_ue,
                      uint8_t eNB_id,
                      uint8_t pbch_phase);


/*! \brief PBCH unscrambling
  This is similar to pbch_scrabling with the difference that inputs are signed s16s (llr values) and instead of flipping bits we change signs.
  \param frame_parms Pointer to frame descriptor
  \param llr Output of the demodulator
  \param length Length of the sequence
  \param frame_mod4 Frame number modulo 4*/
void pbch_unscrambling(NR_DL_FRAME_PARMS *frame_parms,
                       int8_t* llr,
                       uint32_t length,
                       uint8_t frame_mod4);


void generate_64qam_table(void);
void generate_16qam_table(void);
void generate_qpsk_table(void);

uint16_t extract_crc(uint8_t *dci,uint8_t DCI_LENGTH);

/*! \brief LLR from two streams. This function takes two streams (qpsk modulated) and calculates the LLR, considering one stream as interference.
  \param stream0_in pointer to first stream0
  \param stream1_in pointer to first stream1
  \param stream0_out pointer to output stream
  \param rho01 pointer to correlation matrix
  \param length*/
void qpsk_qpsk_TM3456(short *stream0_in,
                      short *stream1_in,
                      short *stream0_out,
                      short *rho01,
                      int length
                     );

/** \brief Attempt decoding of a particular DCI with given length and format.
    @param DCI_LENGTH length of DCI in bits
    @param DCI_FMT Format of DCI
    @param e e-sequence (soft bits)
    @param decoded_output Output of Viterbi decoder
*/
void dci_decoding(uint8_t DCI_LENGTH,
                  uint8_t DCI_FMT,
                  int8_t *e,
                  uint8_t *decoded_output);

/** \brief Do 36.213 DCI decoding procedure by searching different RNTI options and aggregation levels.  Currently does
    not employ the complexity reducing procedure based on RNTI.
    @param phy_vars_ue UE variables
    @param dci_alloc Pointer to DCI_ALLOC_t array to store results for DLSCH/ULSCH programming
    @param do_common If 1 perform search in common search-space else ue-specific search-space
    @param eNB_id eNB Index on which to act
    @param subframe Index of subframe
    @returns bitmap of occupied CCE positions (i.e. those detected)
*/
uint16_t dci_decoding_procedure(PHY_VARS_NR_UE *phy_vars_ue,
                                DCI_ALLOC_t *dci_alloc,
                                int do_common,
                                int16_t eNB_id,
                                uint8_t subframe);

uint16_t dci_CRNTI_decoding_procedure(PHY_VARS_NR_UE *ue,
                                DCI_ALLOC_t *dci_alloc,
                                uint8_t DCIFormat,
                                uint8_t agregationLevel,
                                int16_t eNB_id,
                                uint8_t subframe);

uint16_t dci_decoding_procedure_emul(NR_UE_PDCCH **lte_ue_pdcch_vars,
                                     uint8_t num_ue_spec_dci,
                                     uint8_t num_common_dci,
                                     DCI_ALLOC_t *dci_alloc_tx,
                                     DCI_ALLOC_t *dci_alloc_rx,
                                     int16_t eNB_id);

/** \brief Compute I_TBS (transport-block size) based on I_MCS for PDSCH.  Implements table 7.1.7.1-1 from 36.213.
    @param I_MCS */
uint8_t get_I_TBS(uint8_t I_MCS);

/** \brief Compute I_TBS (transport-block size) based on I_MCS for PUSCH.  Implements table 8.6.1-1 from 36.213.
    @param I_MCS */
unsigned char get_I_TBS_UL(unsigned char I_MCS);

/** \brief Compute Q (modulation order) based on downlink I_MCS. Implements table 7.1.7.1-1 from 36.213.
    @param I_MCS
    @param nb_rb
    @return Transport block size */
uint32_t get_TBS_DL(uint8_t mcs, uint16_t nb_rb);

/** \brief Compute Q (modulation order) based on uplink I_MCS. Implements table 7.1.7.1-1 from 36.213.
    @param I_MCS
    @param nb_rb
    @return Transport block size */
uint32_t get_TBS_UL(uint8_t mcs, uint16_t nb_rb);

/* \brief Return bit-map of resource allocation for a given DCI rballoc (RIV format) and vrb type
   @param N_RB_DL number of PRB on DL
   @param indicator for even/odd slot
   @param vrb vrb index
   @param Ngap Gap indicator
*/
uint32_t get_prb(int N_RB_DL,int odd_slot,int vrb,int Ngap);

/* \brief Return prb for a given vrb index
   @param vrb_type VRB type (0=localized,1=distributed)
   @param rb_alloc_dci rballoc field from DCI
*/
uint32_t get_rballoc(vrb_t vrb_type,uint16_t rb_alloc_dci);


/* \brief Return bit-map of resource allocation for a given DCI rballoc (RIV format) and vrb type
   @returns Transmission mode (1-7)
*/
uint8_t get_transmission_mode(module_id_t Mod_id, uint8_t CC_id, rnti_t rnti);


/* \brief
   @param ra_header Header of resource allocation (0,1) (See sections 7.1.6.1/7.1.6.2 of 36.213 Rel8.6)
   @param rb_alloc Bitmap allocation from DCI (format 1,2)
   @returns number of physical resource blocks
*/
uint32_t conv_nprb(uint8_t ra_header,uint32_t rb_alloc,int N_RB_DL);

int adjust_G(NR_DL_FRAME_PARMS *frame_parms,uint32_t *rb_alloc,uint8_t mod_order,uint8_t subframe);
int adjust_G2(NR_DL_FRAME_PARMS *frame_parms,uint32_t *rb_alloc,uint8_t mod_order,uint8_t subframe,uint8_t symbol);


#ifndef modOrder
#define modOrder(I_MCS,I_TBS) ((I_MCS-I_TBS)*2+2) // Find modulation order from I_TBS and I_MCS
#endif

/** \fn uint8_t I_TBS2I_MCS(uint8_t I_TBS);
    \brief This function maps I_tbs to I_mcs according to Table 7.1.7.1-1 in 3GPP TS 36.213 V8.6.0. Where there is two supported modulation orders for the same I_TBS then either high or low modulation is chosen by changing the equality of the two first comparisons in the if-else statement.
    \param I_TBS Index of Transport Block Size
    \return I_MCS given I_TBS
*/
uint8_t I_TBS2I_MCS(uint8_t I_TBS);

/** \fn uint8_t SE2I_TBS(float SE,
    uint8_t N_PRB,
    uint8_t symbPerRB);
    \brief This function maps a requested throughput in number of bits to I_tbs. The throughput is calculated as a function of modulation order, RB allocation and number of symbols per RB. The mapping orginates in the "Transport block size table" (Table 7.1.7.2.1-1 in 3GPP TS 36.213 V8.6.0)
    \param SE Spectral Efficiency (before casting to integer, multiply by 1024, remember to divide result by 1024!)
    \param N_PRB Number of PhysicalResourceBlocks allocated \sa lte_frame_parms->N_RB_DL
    \param symbPerRB Number of symbols per resource block allocated to this channel
    \return I_TBS given an SE and an N_PRB
*/
uint8_t SE2I_TBS(float SE,
                 uint8_t N_PRB,
                 uint8_t symbPerRB);
/** \brief This function generates the sounding reference symbol (SRS) for the uplink according to 36.211 v8.6.0. If IFFT_FPGA is defined, the SRS is quantized to a QPSK sequence.
    @param frame_parms LTE DL Frame Parameters
    @param soundingrs_ul_config_dedicated Dynamic configuration from RRC during Connection Establishment
    @param txdataF pointer to the frequency domain TX signal
    @returns 0 on success*/
int generate_srs(NR_DL_FRAME_PARMS *frame_parms,
		 SOUNDINGRS_UL_CONFIG_DEDICATED *soundingrs_ul_config_dedicated,
		 int *txdataF,
		 int16_t amp,
		 uint32_t subframe);


/*!
  \brief This function is similar to generate_srs_tx but generates a conjugate sequence for channel estimation. If IFFT_FPGA is defined, the SRS is quantized to a QPSK sequence.
  @param phy_vars_ue Pointer to PHY_VARS structure
  @param eNB_id Index of destination eNB for this SRS
  @param amp Linear amplitude of SRS
  @param subframe Index of subframe on which to act
  @returns 0 on success, -1 on error with message
*/

int32_t generate_srs_tx(PHY_VARS_NR_UE *phy_vars_ue,
                        uint8_t eNB_id,
                        int16_t amp,
                        uint32_t subframe);

/*!
  \brief This function generates the downlink reference signal for the PUSCH according to 36.211 v8.6.0. The DRS occuies the RS defined by rb_alloc and the symbols 2 and 8 for extended CP and 3 and 10 for normal CP.
*/

int32_t generate_drs_pusch(PHY_VARS_NR_UE *phy_vars_ue,
                           UE_nr_rxtx_proc_t *proc,
                           uint8_t eNB_id,
                           int16_t amp,
                           uint32_t subframe,
                           uint32_t first_rb,
                           uint32_t nb_rb,
                           uint8_t ant);

/*!
  \brief This function initializes the Group Hopping, Sequence Hopping and nPRS sequences for PUCCH/PUSCH according to 36.211 v8.6.0. It should be called after configuration of UE (reception of SIB2/3) and initial configuration of eNB (or after reconfiguration of cell-specific parameters).
  @param frame_parms Pointer to a NR_DL_FRAME_PARMS structure (eNB or UE)*/
void init_ul_hopping(NR_DL_FRAME_PARMS *frame_parms);


/*!
  \brief This function implements the initialization of paging parameters for UE (See Section 7, 36.304).It must be called after setting IMSImod1024 during UE startup and after receiving SIB2
  @param ue Pointer to UE context
  @param defaultPagingCycle T from 36.304 (0=32,1=64,2=128,3=256)
  @param nB nB from 36.304 (0=4T,1=2T,2=T,3=T/2,4=T/4,5=T/8,6=T/16,7=T/32*/
int init_ue_paging_info(PHY_VARS_NR_UE *ue, long defaultPagingCycle, long nB);

int32_t compareints (const void * a, const void * b);


void ulsch_modulation(int32_t **txdataF,
                      int16_t amp,
                      frame_t frame,
                      uint32_t subframe,
                      NR_DL_FRAME_PARMS *frame_parms,
                      NR_UE_ULSCH_t *ulsch);


uint8_t allowed_ulsch_re_in_dmrs_symbol(uint16_t k,
                                        uint16_t start_sc,
                                        uint8_t numDmrsCdmGrpsNoData,
                                        uint8_t dmrs_type);


int generate_ue_dlsch_params_from_dci(int frame,
                                      uint8_t subframe,
                                      void *dci_pdu,
                                      rnti_t rnti,
                                      DCI_format_t dci_format,
                                      NR_UE_PDCCH *pdcch_vars,
                                      NR_UE_PDSCH *pdsch_vars,
                                      NR_UE_DLSCH_t **dlsch,
                                      NR_DL_FRAME_PARMS *frame_parms,
                                      PDSCH_CONFIG_DEDICATED *pdsch_config_dedicated,
                                      uint16_t si_rnti,
                                      uint16_t ra_rnti,
                                      uint16_t p_rnti,
                                      uint8_t beamforming_mode,
                                      uint16_t tc_rnti);


int generate_ue_ulsch_params_from_dci(void *dci_pdu,
                                      rnti_t rnti,
                                      uint8_t subframe,
                                      DCI_format_t dci_format,
                                      PHY_VARS_NR_UE *phy_vars_ue,
                                      UE_nr_rxtx_proc_t *proc,
                                      uint16_t si_rnti,
                                      uint16_t ra_rnti,
                                      uint16_t p_rnti,
                                      uint16_t cba_rnti,
                                      uint8_t eNB_id,
                                      uint8_t use_srs);

int32_t generate_ue_ulsch_params_from_rar(PHY_VARS_NR_UE *phy_vars_ue,
                                          UE_nr_rxtx_proc_t *proc,
                                          uint8_t eNB_id);
double sinr_eff_cqi_calc(PHY_VARS_NR_UE *phy_vars_ue,
                         uint8_t eNB_id,
                                                 uint8_t subframe);

uint8_t sinr2cqi(double sinr,uint8_t trans_mode);


int dump_dci(NR_DL_FRAME_PARMS *frame_parms, DCI_ALLOC_t *dci);

int dump_ue_stats(PHY_VARS_NR_UE *phy_vars_ue, UE_nr_rxtx_proc_t *proc, char* buffer, int length, runmode_t mode, int input_level_dBm);

void init_transport_channels(uint8_t);

void generate_RIV_tables(void);

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
                    int n_frames);

/*!
  \brief This function gets the carrier frequencies either from FP or command-line-set global variables, depending on the availability of the latter
  @param fp         Pointer to frame params
  @param dl_Carrier Pointer to DL carrier to be set
  @param ul_Carrier Pointer to UL carrier to be set
*/
void nr_get_carrier_frequencies(NR_DL_FRAME_PARMS *fp,
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
void nr_rf_card_config(openair0_config_t *openair0_cfg,
                       double rx_gain_off,
                       uint64_t ul_Carrier,
                       uint64_t dl_Carrier,
                       int freq_offset);


void print_CQI(void *o,UCI_format_t uci_format,uint8_t eNB_id,int N_RB_DL);

void fill_CQI(NR_UE_ULSCH_t *ulsch,PHY_NR_MEASUREMENTS *meas,uint8_t eNB_id, uint8_t harq_pid,int N_RB_DL, rnti_t rnti, uint8_t trans_mode,double sinr_eff);

void reset_cba_uci(void *o);

/** \brief  This routine computes the subband PMI bitmap based on measurements (0,1,2,3 for rank 0 and 0,1 for rank 1) in the format needed for UCI
    @param meas pointer to measurements
    @param eNB_id eNB_id
    @param nb_subbands number of subbands
    @returns subband PMI bitmap
*/
uint16_t quantize_subband_pmi(PHY_NR_MEASUREMENTS *meas,uint8_t eNB_id,int nb_subbands);

int32_t pmi_convert_rank1_from_rank2(uint16_t pmi_alloc, int tpmi, int nb_rb);

uint16_t quantize_subband_pmi2(PHY_NR_MEASUREMENTS *meas,uint8_t eNB_id,uint8_t a_id,int nb_subbands);



uint64_t cqi2hex(uint32_t cqi);

uint16_t computeRIV(uint16_t N_RB_DL,uint16_t RBstart,uint16_t Lcrbs);


/** \brief  This routine extracts a single subband PMI from a bitmap coming from UCI or the pmi_extend function
    @param N_RB_DL number of resource blocks
    @param mimo_mode
    @param pmi_alloc subband PMI bitmap
    @param rb resource block for which to extract PMI
    @returns subband PMI
*/
uint8_t get_pmi(uint8_t N_RB_DL,MIMO_mode_t mode, uint32_t pmi_alloc,uint16_t rb);

int get_nCCE_offset_l1(int *CCE_table,
                       const unsigned char L,
                       const int nCCE,
                       const int common_dci,
                       const unsigned short rnti,
                       const unsigned char subframe);

uint16_t get_nCCE(uint8_t num_pdcch_symbols,NR_DL_FRAME_PARMS *frame_parms,uint8_t mi);

uint16_t get_nquad(uint8_t num_pdcch_symbols,NR_DL_FRAME_PARMS *frame_parms,uint8_t mi);

uint8_t get_mi(NR_DL_FRAME_PARMS *frame,uint8_t subframe);

uint16_t get_nCCE_mac(uint8_t Mod_id,uint8_t CC_id,int num_pdcch_symbols,int subframe);

uint8_t get_num_pdcch_symbols(uint8_t num_dci,DCI_ALLOC_t *dci_alloc,NR_DL_FRAME_PARMS *frame_parms,uint8_t subframe);

void pdcch_interleaving(NR_DL_FRAME_PARMS *frame_parms,int32_t **z, int32_t **wbar,uint8_t n_symbols_pdcch,uint8_t mi);

void nr_pdcch_unscrambling(int16_t *z,
                           uint16_t scrambling_RNTI,
                           uint32_t length,
                           uint16_t pdcch_DMRS_scrambling_id,
                           int16_t *z2);

void dlsch_unscrambling(NR_DL_FRAME_PARMS *frame_parms,
                        int mbsfn_flag,
                        NR_UE_DLSCH_t *dlsch,
                        int G,
                        int16_t* llr,
                        uint8_t q,
                        uint8_t Ns);

void init_ncs_cell(NR_DL_FRAME_PARMS *frame_parms,uint8_t ncs_cell[20][7]);

void generate_pucch1x(int32_t **txdataF,
                      NR_DL_FRAME_PARMS *frame_parms,
                      uint8_t ncs_cell[20][7],
                      PUCCH_FMT_t fmt,
                      PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                      uint16_t n1_pucch,
                      uint8_t shortened_format,
                      uint8_t *payload,
                      int16_t amp,
                      uint8_t subframe);

void generate_pucch2x(int32_t **txdataF,
                      NR_DL_FRAME_PARMS *fp,
                      uint8_t ncs_cell[20][7],
                      PUCCH_FMT_t fmt,
                      PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                      uint16_t n2_pucch,
                      uint8_t *payload,
                      int A,
                      int B2,
                      int16_t amp,
                      uint8_t subframe,
                      uint16_t rnti);

void generate_pucch3x(int32_t **txdataF,
                    NR_DL_FRAME_PARMS *frame_parms,
                    uint8_t ncs_cell[20][7],
                    PUCCH_FMT_t fmt,
                    PUCCH_CONFIG_DEDICATED *pucch_config_dedicated,
                    uint16_t n3_pucch,
                    uint8_t shortened_format,
                    uint8_t *payload,
                    int16_t amp,
                    uint8_t subframe,
                    uint16_t rnti);


void init_ulsch_power_LUT(void);

/*!
  \brief Check for PRACH TXop in subframe
  @param frame_parms Pointer to NR_DL_FRAME_PARMS
  @param frame frame index to check
  @param subframe subframe index to check
  @returns 0 on success
*/
int is_prach_subframe(NR_DL_FRAME_PARMS *frame_parms,frame_t frame, uint8_t subframe);

/*!
  \brief Generate PRACH waveform
  @param phy_vars_ue Pointer to ue top-level descriptor
  @param eNB_id Index of destination eNB
  @param subframe subframe index to operate on
  @param index of preamble (0-63)
  @param Nf System frame number
  @returns 0 on success

*/
int32_t generate_prach(PHY_VARS_NR_UE *phy_vars_ue,uint8_t eNB_id,uint8_t subframe,uint16_t Nf);


/*!
  \brief Helper for MAC, returns number of available PRACH in TDD for a particular configuration index
  @param frame_parms Pointer to NR_DL_FRAME_PARMS structure
  @returns 0-5 depending on number of available prach
*/
uint8_t get_num_prach_tdd(module_id_t Mod_id);

/*!
  \brief Return the PRACH format as a function of the Configuration Index and Frame type.
  @param prach_ConfigIndex PRACH Configuration Index
  @param frame_type 0-FDD, 1-TDD
  @returns 0-1 accordingly
*/
uint8_t get_prach_fmt(uint8_t prach_ConfigIndex,lte_frame_type_t frame_type);

/*!
  \brief Helper for MAC, returns frequency index of PRACH resource in TDD for a particular configuration index
  @param frame_parms Pointer to NR_DL_FRAME_PARMS structure
  @returns 0-5 depending on number of available prach
*/
uint8_t get_fid_prach_tdd(module_id_t Mod_id,uint8_t tdd_map_index);

/*!
  \brief Comp ute DFT of PRACH ZC sequences.  Used for generation of prach in UE and reception of PRACH in eNB.
  @param rootSequenceIndex PRACH root sequence
  #param prach_ConfigIndex PRACH Configuration Index
  @param zeroCorrelationZoneConfig PRACH ncs_config
  @param highSpeedFlat PRACH High-Speed Flag
  @param frame_type TDD/FDD flag
  @param Xu DFT output
*/
void compute_prach_seq(uint16_t rootSequenceIndex,
		       uint8_t prach_ConfigIndex,
		       uint8_t zeroCorrelationZoneConfig,
		       uint8_t highSpeedFlag,
		       lte_frame_type_t frame_type,
		       uint32_t X_u[64][839]);


void init_prach_tables(int N_ZC);

void init_unscrambling_lut(void);
void init_scrambling_lut(void);

/*!
  \brief Return the status of MBSFN in this frame/subframe
  @param frame Frame index
  @param subframe Subframe index
  @param frame_parms Pointer to frame parameters
  @returns 1 if subframe is for MBSFN
*/
int is_pmch_subframe(frame_t frame, int subframe, NR_DL_FRAME_PARMS *frame_parms);

uint8_t is_not_pilot(uint8_t pilots, uint8_t re, uint8_t nushift, uint8_t use2ndpilots);

uint8_t is_not_UEspecRS(int8_t lprime, uint8_t re, uint8_t nushift, uint8_t Ncp, uint8_t beamforming_mode);

uint32_t dlsch_decoding_abstraction(double *dlsch_MIPB,
                                    NR_DL_FRAME_PARMS *lte_frame_parms,
                                    NR_UE_DLSCH_t *dlsch,
                                    uint8_t subframe,
                                    uint8_t num_pdcch_symbols);

// DL power control functions
double get_pa_dB(uint8_t pa);


double computeRhoA_UE(PDSCH_CONFIG_DEDICATED *pdsch_config_dedicated,
                      NR_UE_DLSCH_t *dlsch_ue,
                      uint8_t dl_power_off,
                      uint8_t n_antenna_port);

double computeRhoB_UE(PDSCH_CONFIG_DEDICATED  *pdsch_config_dedicated,
                      PDSCH_CONFIG_COMMON *pdsch_config_common,
                      uint8_t n_antenna_port,
                      NR_UE_DLSCH_t *dlsch_ue,
                      uint8_t dl_power_off);

/*void compute_sqrt_RhoAoRhoB(PDSCH_CONFIG_DEDICATED  *pdsch_config_dedicated,
  PDSCH_CONFIG_COMMON *pdsch_config_common,
  uint8_t n_antenna_port,
  NR_UE_DLSCH_t *dlsch_ue);
*/

uint8_t get_prach_prb_offset(NR_DL_FRAME_PARMS *frame_parms,
			     uint8_t prach_ConfigIndex, 
			     uint8_t n_ra_prboffset,
			     uint8_t tdd_mapindex, uint16_t Nf);


uint32_t lte_gold_generic(uint32_t *x1, uint32_t *x2, uint8_t reset);

uint8_t nr_dci_decoding_procedure(PHY_VARS_NR_UE *ue,
                                  UE_nr_rxtx_proc_t *proc,
                                  fapi_nr_dci_indication_t *dci_ind);


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
    @param rx_type. rx_type=RX_IC_single_stream will enable interference cancellation of a second stream when decoding the first stream. In case of TM1, 2, 5, and this can cancel interference from a neighbouring cell given by eNB_id_i. In case of TM5, eNB_id_i should be set to n_connected_eNB to perform multi-user interference cancellation. In case of TM3, eNB_id_i should be set to eNB_id to perform co-channel interference cancellation; this option should be used together with an interference cancellation step [...]. In case of TM3, if rx_type=RX_IC_dual_stream, both streams will be decoded by applying the IC single stream receiver twice.
    @param i_mod Modulation order of the interfering stream
*/
int nr_rx_pdsch(PHY_VARS_NR_UE *ue,
             UE_nr_rxtx_proc_t *proc,
             PDSCH_t type,
             unsigned char eNB_id,
             unsigned char eNB_id_i, //if this == ue->n_connected_eNB, we assume MU interference
             uint32_t frame,
             uint8_t nr_slot_rx,
             unsigned char symbol,
             unsigned char first_symbol_flag,
             RX_type_t rx_type,
             unsigned char i_mod,
		unsigned char harq_pid);

int32_t generate_nr_prach(PHY_VARS_NR_UE *ue, uint8_t gNB_id, uint8_t subframe);

void *dlsch_thread(void *arg);
/**@}*/
#endif

