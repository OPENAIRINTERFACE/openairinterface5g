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

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "SIMULATION/TOOLS/defs.h"
#include "SIMULATION/RF/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "oaisim_config.h"
#include "init_lte.h"

#ifdef OPENAIR2
#include "LAYER2/MAC/defs.h"
#include "UTIL/OMV/structures.h"
#endif

eNB_MAC_INST* get_eNB_mac_inst(module_id_t module_idP);
OAI_Emulation* get_OAI_emulation(void);
void init_channel_vars(LTE_DL_FRAME_PARMS *frame_parms, double ***s_re,double ***s_im,double ***r_re,double ***r_im,double ***r_re0,double ***r_im0);

void do_UL_sig(channel_desc_t *UE2RU[NUMBER_OF_UE_MAX][NUMBER_OF_RU_MAX][MAX_NUM_CCs],
               node_desc_t *enb_data[NUMBER_OF_RU_MAX],node_desc_t *ue_data[NUMBER_OF_UE_MAX],
	       uint16_t subframe,uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms, 
	       uint32_t frame,int eNB_id,uint8_t CC_id);

void do_DL_sig(channel_desc_t *RU2UE[NUMBER_OF_RU_MAX][NUMBER_OF_UE_MAX][MAX_NUM_CCs],
               node_desc_t *enb_data[NUMBER_OF_RU_MAX],
	       node_desc_t *ue_data[NUMBER_OF_UE_MAX],
	       uint16_t subframe,
	       uint32_t offset,
	       uint32_t length,
	       uint8_t abstraction_flag,LTE_DL_FRAME_PARMS *frame_parms,uint8_t UE_id,int CC_id);

void init_ue(node_desc_t  *ue_data, UE_Antenna ue_ant);//Abstraction changes
void init_enb(node_desc_t  *enb_data, eNB_Antenna enb_ant);//Abstraction changes
void extract_position(node_list* input_node_list, node_desc_t**, int nb_nodes);//Abstraction changes
void get_beta_map(void);//Abstraction changes
void get_MIESM_param(void);

void init_snr(channel_desc_t *,  node_desc_t *, node_desc_t *, double*, double*, uint8_t, uint16_t, uint8_t, uint16_t);//Abstraction changes
void init_snr_up(channel_desc_t *,  node_desc_t *, node_desc_t *, double*, double*, uint16_t, uint16_t);//Abstraction changes
void calculate_sinr(channel_desc_t *,  node_desc_t *, node_desc_t *, double *sinr_dB, uint16_t);//Abstraction changes
void get_beta_map(void);
int dlsch_abstraction_EESM(double* sinr_dB, uint32_t rb_alloc[4], uint8_t mcs, uint8_t); //temporary testing for PHY abstraction
int dlsch_abstraction_MIESM(double* sinr_dB,uint8_t TM, uint32_t rb_alloc[4], uint8_t mcs,uint8_t);
int ulsch_abstraction_MIESM(double* sinr_dB,uint8_t TM, uint8_t mcs,uint16_t nb_rb, uint16_t first_rb);
int ulsch_abstraction(double* sinr_dB,uint8_t TM, uint8_t mcs,uint16_t nb_rb, uint16_t first_rb);

void calc_path_loss(node_desc_t* node_tx, node_desc_t* node_rx, channel_desc_t *ch_desc, Environment_System_Config env_desc, double **SF);

void do_OFDM_mod(int32_t **txdataF, int32_t **txdata, frame_t frame, uint16_t next_slot, LTE_DL_FRAME_PARMS *frame_parms);

void reset_opp_meas(void);
void print_opp_meas(void);

#ifdef OPENAIR2
int omv_write (int pfd,  node_list* enb_node_list, node_list* ue_node_list, Data_Flow_Unit omv_data);
void omv_end (int pfd, Data_Flow_Unit omv_data);
#endif








