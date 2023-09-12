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

#ifndef NR_PHY_INIT_H_
#define NR_PHY_INIT_H_

#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_UE.h"

int nr_get_ssb_start_symbol(NR_DL_FRAME_PARMS *fp,uint8_t i_ssb);
int nr_init_frame_parms(nfapi_nr_config_request_scf_t *config, NR_DL_FRAME_PARMS *frame_parms);
int nr_init_frame_parms_ue(NR_DL_FRAME_PARMS *frame_parms, fapi_nr_config_request_t *config, uint16_t nr_band);
void nr_init_frame_parms_ue_sa(NR_DL_FRAME_PARMS *frame_parms, uint64_t downlink_frequency, int32_t uplink_frequency_offset, uint8_t mu, uint16_t nr_band);
void nr_init_frame_parms_ue_sl(NR_DL_FRAME_PARMS *frame_parms, uint64_t sidelink_frequency, uint16_t nr_band);
int init_nr_ue_signal(PHY_VARS_NR_UE *ue,int nb_connected_eNB);
void term_nr_ue_signal(PHY_VARS_NR_UE *ue, int nb_connected_gNB);
void init_nr_ue_transport(PHY_VARS_NR_UE *ue);
void init_N_TA_offset(PHY_VARS_NR_UE *ue);
void nr_dump_frame_parms(NR_DL_FRAME_PARMS *frame_parms);
int phy_init_nr_gNB(PHY_VARS_gNB *gNB);
int init_codebook_gNB(PHY_VARS_gNB *gNB);
void nr_phy_config_request(NR_PHY_Config_t *gNB);
void nr_phy_config_request_sim(PHY_VARS_gNB *gNB,int N_RB_DL,int N_RB_UL,int mu,int Nid_cell,uint64_t position_in_burst);
void phy_free_nr_gNB(PHY_VARS_gNB *gNB);
int l1_north_init_gNB(void);
void init_nr_transport(PHY_VARS_gNB *gNB);
void reset_nr_transport(PHY_VARS_gNB *gNB);

void init_DLSCH_struct(PHY_VARS_gNB *gNB, processingData_L1tx_t *msg);
void reset_DLSCH_struct(const PHY_VARS_gNB *gNB, processingData_L1tx_t *msg);

void RCconfig_nrUE_prs(void *cfg);
void init_nr_prs_ue_vars(PHY_VARS_NR_UE *ue);
void nr_init_dl_harq_processes(NR_DL_UE_HARQ_t harq_list[2][NR_MAX_DLSCH_HARQ_PROCESSES], int number_of_processes, int num_rb);
void nr_init_ul_harq_processes(NR_UL_UE_HARQ_t harq_list[NR_MAX_ULSCH_HARQ_PROCESSES], int number_of_processes, int num_rb, int num_ant_tx);
void free_nr_ue_dl_harq(NR_DL_UE_HARQ_t harq_list[2][NR_MAX_DLSCH_HARQ_PROCESSES], int number_of_processes, int num_rb);
void free_nr_ue_ul_harq(NR_UL_UE_HARQ_t harq_list[NR_MAX_ULSCH_HARQ_PROCESSES], int number_of_processes, int num_rb, int num_ant_tx);

void phy_init_nr_top(PHY_VARS_NR_UE *ue);
void phy_term_nr_top(void);
void init_delay_table(uint16_t ofdm_symbol_size,
                      int max_delay_comp,
                      int max_ofdm_symbol_size,
                      c16_t delay_table[][max_ofdm_symbol_size]);

#endif
