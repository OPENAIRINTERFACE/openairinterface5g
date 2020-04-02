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

/*!
\brief This function performs channel estimation including frequency and temporal interpolation
\param phy_vars_ue Pointer to UE PHY variables
\param eNB_id Index of target eNB
\param eNB_offset Offset for interfering eNB (in terms cell ID mod 3)
\param Ns slot number (0..19)
\param p antenna port
\param l symbol within slot
\param symbol symbol within frame
*/
int nr_pdcch_channel_estimation(PHY_VARS_NR_UE *ue,
                                uint8_t eNB_offset,
                                unsigned char Ns,
                                unsigned char symbol,
                                unsigned short coreset_start_subcarrier,
                                unsigned short nb_rb_coreset);

int nr_pbch_dmrs_correlation(PHY_VARS_NR_UE *ue,
                             uint8_t eNB_offset,
                             unsigned char Ns,
                             unsigned char symbol,
                             int dmrss,
                             NR_UE_SSB *current_ssb);

int nr_pbch_channel_estimation(PHY_VARS_NR_UE *ue,
                               uint8_t eNB_offset,
                               unsigned char Ns,
                               unsigned char symbol,
                               int dmrss,
                               uint8_t ssb_index,
                               uint8_t n_hf);

int nr_pdsch_channel_estimation(PHY_VARS_NR_UE *ue,
                                uint8_t eNB_offset,
                                unsigned char Ns,
                                unsigned short p,
                                unsigned char symbol,
                                unsigned short bwp_start_subcarrier,
                                unsigned short nb_rb_pdsch);

void nr_adjust_synch_ue(NR_DL_FRAME_PARMS *frame_parms,
                        PHY_VARS_NR_UE *ue,
                        module_id_t eNB_id,
                        uint8_t subframe,
                        unsigned char clear,
                        short coef);
                      
void nr_ue_measurements(PHY_VARS_NR_UE *ue,
                        unsigned int subframe_offset,
                        unsigned char N0_symbol,
                        unsigned char abstraction_flag,
                        unsigned char rank_adaptation,
                        uint8_t subframe);

void phy_adjust_gain_nr(PHY_VARS_NR_UE *ue,
                        uint32_t rx_power_fil_dB,
                        uint8_t eNB_id);

int16_t get_nr_PL(PHY_VARS_NR_UE *ue,uint8_t gNB_index);

#endif
