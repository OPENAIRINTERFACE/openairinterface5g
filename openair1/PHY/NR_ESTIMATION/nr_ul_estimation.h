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
\param bwp_start_subcarrier, first allocated subcarrier
\param nb_rb_pusch, number of allocated RBs for this UE
*/

int nr_pusch_channel_estimation(PHY_VARS_gNB *gNB,
                                unsigned char Ns,
                                unsigned short p,
                                unsigned char symbol,
                                int ul_id,
                                unsigned short bwp_start_subcarrier,
                                nfapi_nr_pusch_pdu_t *pusch_pdu);

void dump_nr_I0_stats(FILE *fd,PHY_VARS_gNB *gNB);

void gNB_I0_measurements(PHY_VARS_gNB *gNB,int slot,int first_symb,int num_symb);

void nr_gnb_measurements(PHY_VARS_gNB *gNB, uint8_t ulsch_id, unsigned char harq_pid, unsigned char symbol, uint8_t nrOfLayers);

int nr_est_timing_advance_pusch(PHY_VARS_gNB* phy_vars_gNB, int UE_id);

void nr_pusch_ptrs_processing(PHY_VARS_gNB *gNB,
                              NR_DL_FRAME_PARMS *frame_parms,
                              nfapi_nr_pusch_pdu_t *rel15_ul,
                              uint8_t ulsch_id,
                              uint8_t nr_tti_rx,
                              unsigned char symbol,
                              uint32_t nb_re_pusch);

int nr_srs_channel_estimation(PHY_VARS_gNB *gNB,
                              int frame,
                              int slot,
                              nfapi_nr_srs_pdu_t *srs_pdu,
                              nr_srs_info_t *nr_srs_info,
                              int32_t *srs_generated_signal,
                              int32_t **srs_received_signal,
                              int32_t **srs_estimated_channel,
                              uint32_t *noise_power);
#endif
