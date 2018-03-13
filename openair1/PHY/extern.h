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

#ifndef __PHY_EXTERN_H__
#define __PHY_EXTERN_H__

#include "PHY/defs.h"
#include "common/ran_context.h"

extern  char* namepointer_chMag ;
extern char* namepointer_log2;
extern  char fmageren_name2[512];

extern unsigned int RX_DMA_BUFFER[4][NB_ANTENNAS_RX];
extern unsigned int TX_DMA_BUFFER[4][NB_ANTENNAS_TX];

#include "PHY/LTE_TRANSPORT/extern.h"
#include "SIMULATION/ETH_TRANSPORT/extern.h"

extern unsigned int DAQ_MBOX;
extern int number_of_cards;


#ifndef OCP_FRAMEWORK
extern PHY_VARS_UE ***PHY_vars_UE_g;
//extern PHY_VARS_eNB ***PHY_vars_eNB_g;
extern RAN_CONTEXT_t RC;
extern PHY_VARS_RN **PHY_vars_RN_g;
extern LTE_DL_FRAME_PARMS *lte_frame_parms_g;
#else
#define MAX_UE 10
#define MAX_eNB 20
extern PHY_VARS_UE * PHY_vars_UE_g[MAX_UE][MAX_NUM_CCs];
extern PHY_VARS_eNB * PHY_vars_eNB_g[MAX_eNB][MAX_NUM_CCs];
#endif

extern short primary_synch0[144];
extern short primary_synch1[144];
extern short primary_synch2[144];
extern unsigned char primary_synch0_tab[72];
extern unsigned char primary_synch1_tab[72];
extern unsigned char primary_synch2_tab[72];
extern int16_t *primary_synch0_time; //!< index: [0..ofdm_symbol_size*2[
extern int16_t *primary_synch1_time; //!< index: [0..ofdm_symbol_size*2[
extern int16_t *primary_synch2_time; //!< index: [0..ofdm_symbol_size*2[
extern int *sync_corr_ue0; //!< index [0..10*samples_per_tti[
extern int *sync_corr_ue1; //!< index [0..10*samples_per_tti[
extern int *sync_corr_ue2; //!< index [0..10*samples_per_tti[

extern int flagMag;
//extern short **txdataF_rep_tmp;

extern char mode_string[4][20];

#include "PHY/LTE_TRANSPORT/extern.h"

extern unsigned char NB_RU;

#ifndef OPENAIR2
extern unsigned char NB_eNB_INST;
extern unsigned char NB_UE_INST;
extern unsigned char NB_RN_INST;
#endif

extern unsigned int ULSCH_max_consecutive_errors;
extern int flag_LA;
extern double sinr_bler_map[MCS_COUNT][2][MCS_TABLE_LENGTH_MAX];
extern double sinr_bler_map_up[MCS_COUNT][2][16];
extern int table_length[MCS_COUNT];
extern double sinr_to_cqi[4][16];
extern int cqi_to_mcs[16];

//for MU-MIMO abstraction using MIESM
//this 2D arrarays contains SINR, MI and RBIR in rows 1, 2, and 3 respectively
extern double MI_map_4qam[3][162];
extern double MI_map_16qam[3][197];
extern double MI_map_64qam[3][227];

extern double beta1_dlsch_MI[6][MCS_COUNT];
extern double beta2_dlsch_MI[6][MCS_COUNT];

extern double q_qpsk[8];
extern double q_qam16[8];
extern double q_qam64[8];

extern double p_qpsk[8];
extern double p_qam16[8];
extern double p_qam64[8];

extern double beta1_dlsch[6][MCS_COUNT];
extern double beta2_dlsch[6][MCS_COUNT];

extern char eNB_functions[6][20];
extern char eNB_timing[2][20];
extern char ru_if_types[MAX_RU_IF_TYPES][20];

extern int16_t unscrambling_lut[65536*16];
extern uint8_t scrambling_lut[65536*16];

extern unsigned short msrsb_6_40[8][4];
extern unsigned short msrsb_41_60[8][4];
extern unsigned short msrsb_61_80[8][4];
extern unsigned short msrsb_81_110[8][4];
extern unsigned short Nb_6_40[8][4];
extern unsigned short Nb_41_60[8][4];
extern unsigned short Nb_61_80[8][4];
extern unsigned short Nb_81_110[8][4];

extern uint16_t hundred_times_log10_NPRB[100];
extern uint8_t alpha_lut[8];
extern uint8_t max_turbo_iterations;
#endif /*__PHY_EXTERN_H__ */

