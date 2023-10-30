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

#ifndef __NR_UL_ESTIMATION_DEFS__H__
#define __NR_UL_ESTIMATION_DEFS__H__


#include "PHY/defs_gNB.h"
/** @addtogroup _PHY_PARAMETER_ESTIMATION_BLOCKS_
 * @{
 */

/*!
\brief This function performs channel estimation including frequency interpolation
\param gNB Pointer to gNB PHY variables
\param Ns slot number (0..19)
\param p
\param symbol symbol within slot
\param ul_id
\param bwp_start_subcarrier first allocated subcarrier
\param pusch_pdu
\param max_ch maximum value of estimated channel
\param nvar
*/

int nr_pusch_channel_estimation(PHY_VARS_gNB *gNB,
                                unsigned char Ns,
                                unsigned short p,
                                unsigned char symbol,
                                int ul_id,
                                unsigned short bwp_start_subcarrier,
                                nfapi_nr_pusch_pdu_t *pusch_pdu,
                                int *max_ch,
                                uint32_t *nvar);

void dump_nr_I0_stats(FILE *fd,PHY_VARS_gNB *gNB);

void gNB_I0_measurements(PHY_VARS_gNB *gNB,int slot,int first_symb,int num_symb);

void nr_gnb_measurements(PHY_VARS_gNB *gNB,
                         NR_gNB_ULSCH_t *ulsch,
                         NR_gNB_PUSCH *pusch_vars,
                         unsigned char symbol,
                         uint8_t nrOfLayers);

int nr_est_timing_advance_srs(const NR_DL_FRAME_PARMS *frame_parms,
                              const int32_t srs_estimated_channel_time[][frame_parms->ofdm_symbol_size]);

void nr_pusch_ptrs_processing(PHY_VARS_gNB *gNB,
                              NR_DL_FRAME_PARMS *frame_parms,
                              nfapi_nr_pusch_pdu_t *rel15_ul,
                              uint8_t ulsch_id,
                              uint8_t nr_tti_rx,
                              unsigned char symbol,
                              uint32_t nb_re_pusch);

int nr_srs_channel_estimation(const PHY_VARS_gNB *gNB,
                              const int frame,
                              const int slot,
                              const nfapi_nr_srs_pdu_t *srs_pdu,
                              const nr_srs_info_t *nr_srs_info,
                              const int32_t **srs_generated_signal,
                              int32_t srs_received_signal[][gNB->frame_parms.ofdm_symbol_size*(1<<srs_pdu->num_symbols)],
                              int32_t srs_estimated_channel_freq[][1<<srs_pdu->num_ant_ports][gNB->frame_parms.ofdm_symbol_size*(1<<srs_pdu->num_symbols)],
                              int32_t srs_estimated_channel_time[][1<<srs_pdu->num_ant_ports][gNB->frame_parms.ofdm_symbol_size],
                              int32_t srs_estimated_channel_time_shifted[][1<<srs_pdu->num_ant_ports][gNB->frame_parms.ofdm_symbol_size],
                              int8_t *snr_per_rb,
                              int8_t *snr);

void nr_freq_equalization(NR_DL_FRAME_PARMS *frame_parms,
                          int *rxdataF_comp,
                          int *ul_ch_mag,
                          int *ul_ch_mag_b,
                          unsigned char symbol,
                          unsigned short Msc_RS,
                          unsigned char Qm);

void nr_init_fde(void);

#endif
/** @}*/
