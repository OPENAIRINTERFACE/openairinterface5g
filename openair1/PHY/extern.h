/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
#ifndef __PHY_EXTERN_H__
#define __PHY_EXTERN_H__

#include "PHY/defs.h"
#include "PHY_INTERFACE/defs.h"

extern  char* namepointer_chMag ;
extern char* namepointer_log2;
extern  char fmageren_name2[512];

extern unsigned int RX_DMA_BUFFER[4][NB_ANTENNAS_RX];
extern unsigned int TX_DMA_BUFFER[4][NB_ANTENNAS_TX];

#include "PHY/LTE_TRANSPORT/extern.h"
#include "SIMULATION/ETH_TRANSPORT/extern.h"

extern unsigned int DAQ_MBOX;
extern int number_of_cards;

//extern PHY_CONFIG *PHY_config;
//extern PHY_VARS *PHY_vars;

extern PHY_VARS_UE ***PHY_vars_UE_g;
extern PHY_VARS_eNB ***PHY_vars_eNB_g;
extern PHY_VARS_RN **PHY_vars_RN_g;
extern LTE_DL_FRAME_PARMS *lte_frame_parms_g;

extern MAC_xface *mac_xface;


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


#endif /*__PHY_EXTERN_H__ */

