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

#ifndef __NR_ESTIMATION_DEFS__H__
#define __NR_ESTIMATION_DEFS__H__


#include "PHY/defs_nr_UE.h"
//#include "PHY/defs_gNB.h"
/** @addtogroup _PHY_PARAMETER_ESTIMATION_BLOCKS_
 * @{
 */

/*!\brief Timing drift hysterisis in samples*/
#define SYNCH_HYST 2

/* A function to perform the channel estimation of DL PRS signal */
int nr_prs_channel_estimation(uint8_t gNB_id,
                              uint8_t rsc_id,
                              uint8_t rep_num,
                              PHY_VARS_NR_UE *ue,
                              const UE_nr_rxtx_proc_t *proc,
                              NR_DL_FRAME_PARMS *frame_params,
                              c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]);

/* Generic function to find the peak of channel estimation buffer */
void peak_estimator(int32_t *buffer, int32_t buf_len, int32_t *peak_idx, int32_t *peak_val, int32_t mean_val);

/*!
\brief This function performs channel estimation including frequency and temporal interpolation
*/
void nr_pdcch_channel_estimation(PHY_VARS_NR_UE *ue,
                                 const UE_nr_rxtx_proc_t *proc,
                                 unsigned char symbol,
                                 fapi_nr_coreset_t *coreset,
                                 uint16_t first_carrier_offset,
                                 uint16_t BWPStart,
                                 int32_t pdcch_est_size,
                                 int32_t pdcch_dl_ch_estimates[][pdcch_est_size],
                                 c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]);

int nr_pbch_dmrs_correlation(PHY_VARS_NR_UE *ue,
                             const UE_nr_rxtx_proc_t *proc,
                             unsigned char symbol,
                             int dmrss,
                             NR_UE_SSB *current_ssb,
                             c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]);

int nr_pbch_channel_estimation(PHY_VARS_NR_UE *ue,
                               int estimateSz,
                               struct complex16 dl_ch_estimates[][estimateSz],
                               struct complex16 dl_ch_estimates_time[][ue->frame_parms.ofdm_symbol_size],
                               const UE_nr_rxtx_proc_t *proc,
                               unsigned char symbol,
                               int dmrss,
                               uint8_t ssb_index,
                               uint8_t n_hf,
                               c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]);

int nr_pdsch_channel_estimation(PHY_VARS_NR_UE *ue,
                                const UE_nr_rxtx_proc_t *proc,
                                unsigned short p,
                                unsigned char symbol,
                                unsigned char nscid,
                                unsigned short scrambling_id,
                                unsigned short BWPStart,
                                uint8_t config_type,
                                uint16_t rb_offset,
                                unsigned short bwp_start_subcarrier,
                                unsigned short nb_rb_pdsch,
                                uint32_t pdsch_est_size,
                                int32_t dl_ch_estimates[][pdsch_est_size],
                                int rxdataFsize,
                                c16_t rxdataF[][rxdataFsize]);

void nr_adjust_synch_ue(NR_DL_FRAME_PARMS *frame_parms,
                        PHY_VARS_NR_UE *ue,
                        module_id_t gNB_id,
			int estimateSz,
			struct complex16 dl_ch_estimates_time [][estimateSz],
                        uint8_t frame,
                        uint8_t subframe,
                        unsigned char clear,
                        short coef);

void nr_ue_measurements(PHY_VARS_NR_UE *ue,
                        const UE_nr_rxtx_proc_t *proc,
                        NR_UE_DLSCH_t *dlsch,
                        uint32_t pdsch_est_size,
                        int32_t dl_ch_estimates[][pdsch_est_size]);

void nr_ue_ssb_rsrp_measurements(PHY_VARS_NR_UE *ue,
                                 uint8_t gNB_index,
                                 const UE_nr_rxtx_proc_t *proc,
                                 c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]);

void nr_ue_rrc_measurements(PHY_VARS_NR_UE *ue,
                            const UE_nr_rxtx_proc_t *proc,
                            c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP]);

void phy_adjust_gain_nr(PHY_VARS_NR_UE *ue,
                        uint32_t rx_power_fil_dB,
                        uint8_t gNB_id);

void nr_pdsch_ptrs_processing(PHY_VARS_NR_UE *ue,
                              int nbRx,
                              c16_t ptrs_phase_per_slot[][14],
                              int32_t ptrs_re_per_slot[][14],
                              uint32_t rx_size_symbol,
                              int32_t rxdataF_comp[][nbRx][rx_size_symbol * NR_SYMBOLS_PER_SLOT],
                              NR_DL_FRAME_PARMS *frame_parms,
                              NR_DL_UE_HARQ_t *dlsch0_harq,
                              NR_DL_UE_HARQ_t *dlsch1_harq,
                              uint8_t gNB_id,
                              uint8_t nr_slot_rx,
                              unsigned char symbol,
                              uint32_t nb_re_pdsch,
                              uint16_t rnti,
                              NR_UE_DLSCH_t dlsch[2]);

float_t get_nr_RSRP(module_id_t Mod_id,uint8_t CC_id,uint8_t gNB_index);
/** @}*/
#endif
