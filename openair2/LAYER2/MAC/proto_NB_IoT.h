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

/*! \file LAYER2/MAC/proto_NB_IoT.h
 * \brief MAC functions prototypes for eNB and UE
 * \author Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email navid.nikaein@eurecom.fr
 * \version 1.0
 */

#ifndef __LAYER2_MAC_PROTO_NB_IoT_H__
#define __LAYER2_MAC_PROTO_NB_IoT_H__

#include "openair1/PHY/LTE_TRANSPORT/defs_NB_IoT.h"
#include "LAYER2/MAC/defs_NB_IoT.h"
#include "COMMON/platform_types.h"
#include "openair2/RRC/LTE/defs_NB_IoT.h"
/** \addtogroup _mac
 *  @{
 */

int l2_init_eNB_NB_IoT(void);

///config
void rrc_mac_config_req_NB_IoT(
    module_id_t                             Mod_idP,
    int                                     CC_idP,
    int                                     rntiP,
    rrc_eNB_carrier_data_NB_IoT_t           *carrier,
    SystemInformationBlockType1_NB_t        *sib1_NB_IoT,
    RadioResourceConfigCommonSIB_NB_r13_t   *radioResourceConfigCommon,
    PhysicalConfigDedicated_NB_r13_t        *physicalConfigDedicated,
    LogicalChannelConfig_NB_r13_t           *logicalChannelConfig,            //FIXME: decide how to use it
    uint8_t                                 ded_flag,
    uint8_t                                 ue_list_ded_num);

///system
void init_mac_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst);
//void init_rrc_NB_IoT();
void release_mac_inst(uint8_t order);
eNB_MAC_INST_NB_IoT *get_mac_inst(uint8_t order);
uint8_t register_mac_inst(eNB_MAC_INST_NB_IoT *inst, uint8_t order);

///tool
void init_tool(sib1_NB_IoT_sched_t *config);
void UE_info_setting(UE_TEMPLATE_NB_IoT *UE_info);
UE_TEMPLATE_NB_IoT *get_ue_from_rnti(eNB_MAC_INST_NB_IoT *inst, rnti_t rnti);
	
///scheduler
void eNB_dlsch_ulsch_scheduler_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, uint32_t abs_subframe);
void eNB_scheduler_computing_flag_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, uint32_t abs_subframe, uint32_t *scheduler_flags, uint32_t *common_flags, uint32_t *max_subframe);

//Calvin temp define self-tools
int init_debug(eNB_MAC_INST_NB_IoT *inst);
void maintain_available_resource(eNB_MAC_INST_NB_IoT *mac_inst);
void extend_available_resource_DL(eNB_MAC_INST_NB_IoT *mac_inst, int max_subframe);
available_resource_DL_t *check_sibs_resource(eNB_MAC_INST_NB_IoT *mac_inst, int check_start_subframe, int check_end_subframe, int num_subframe, int *residual_subframe, int *out_last_subframe, int *out_first_subframe);
uint32_t calculate_DLSF(eNB_MAC_INST_NB_IoT *mac_inst, int abs_start_subframe, int abs_end_subframe);
void init(eNB_MAC_INST_NB_IoT *mac_inst);
void init_dl_list(eNB_MAC_INST_NB_IoT *mac_inst);
int is_dlsf(eNB_MAC_INST_NB_IoT *mac_inst, int abs_subframe);
void fill_resource_DL(eNB_MAC_INST_NB_IoT *mac_inst, available_resource_DL_t *node, int start_subframe, int end_subframe, schedule_result_t *new_node);
available_resource_DL_t *check_resource_DL(eNB_MAC_INST_NB_IoT *mac_inst, int check_subframe, int num_subframes, int *out_last_subframe, int *out_first_subframe);
void print_available_resource_DL(eNB_MAC_INST_NB_IoT *mac_inst);
void print_schedule_result_DL(void);
void print_schedule_result_UL(void);
void add_ue(eNB_MAC_INST_NB_IoT *mac_inst, uint16_t rnti, ce_level_t ce, uint32_t PHR, uint32_t ul_total_buffer);
void remove_ue(eNB_MAC_INST_NB_IoT *mac_inst, uint16_t rnti, ce_level_t ce);
//	SIBs
void schedule_sibs(eNB_MAC_INST_NB_IoT *mac_inst, uint32_t sibs_order, int start_subframe);

//RA
void msg3_do_retransmit_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, rnti_t c_rnti);
void msg4_do_retransmit_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, rnti_t c_rnti);
void schedule_RA_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst);
void init_RA_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, uint8_t preamble_index, ce_level_t ce_level, uint32_t sfn_id, uint16_t ta);
void schedule_rar_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, int abs_subframe);
void receive_msg3_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, rnti_t c_rnti, uint32_t phr, uint32_t ul_total_buffer);
void schedule_msg3_retransimission_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, int abs_subframe);
void schedule_msg4_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, int abs_subframe);
void fill_rar_NB_IoT(eNB_MAC_INST_NB_IoT *inst, RA_TEMPLATE_NB_IoT *ra_template, uint8_t msg3_schedule_delay, uint8_t msg3_rep, sched_temp_UL_NB_IoT_t *schedule_template);
void receive_msg4_ack_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, rnti_t rnti);

//USS
void schedule_uss_NB_IoT(module_id_t module_id, eNB_MAC_INST_NB_IoT *mac_inst, uint32_t subframe, uint32_t frame, uint32_t hypersfn, int index_ss);

//DATA
uint8_t *parse_ulsch_header( uint8_t *mac_header,
                             uint8_t *num_ce,
                             uint8_t *num_sdu,
                             uint8_t *rx_ces,
                             uint8_t *rx_lcids,
                             uint16_t *rx_lengths,
                             uint16_t tb_length );

/*******UL Scheduler**********/
void print_scheduling_result_UL(void);
void print_available_UL_resource(void);
/*set nprach configuration at intial time*/
void setting_nprach(void);
/*Uplink main scheduler*/
int schedule_UL_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst,UE_TEMPLATE_NB_IoT *UE_info, uint32_t subframe, uint32_t frame, uint32_t H_SFN);
/*Check available uplink resource list, if there is available uplink resource, return 0, otherwise, return 1*/
int Check_UL_resource(uint32_t DL_end, int total_ru, sched_temp_UL_NB_IoT_t *NPUSCH_info, int multi_tone, int fmt2_flag);
/*Get I Repetition number in DCI*/
int get_I_REP(int N_rep);
/*Get N REP from preamble repeat*/
int get_N_REP(int CE_level);
/*Get TBS from mcs, multi-tone, Iru*/
int get_TBS_UL_NB_IoT(uint32_t mcs,uint32_t multi_tone,int Iru);
/*get I tbs from mcs and multi-tone*/
int get_I_TBS_NB_IoT(int x,int y);
/*Get DCI REP from R max and R*/
int get_DCI_REP(uint32_t R,uint32_t R_max);
/*Check single tone resource list*/
int single_tone_ru_allocation(uint32_t uplink_time, int total_ru, sched_temp_UL_NB_IoT_t *NPUSCH_info, int fmt2_flag);
/*Check multi tone resource list*/
int multi_tone_ru_allocation(uint32_t uplink_time, int total_ru, sched_temp_UL_NB_IoT_t *NPUSCH_info);
/*Generate scheduling result of DCI N0 and Uplink config*/
void generate_scheduling_result_UL(int32_t DCI_subframe, int32_t DCI_end_subframe, uint32_t UL_subframe, uint32_t UL_end_subframe, DCIFormatN0_t *DCI_inst, rnti_t rnti, uint8_t *ul_debug_str, uint8_t *dl_debug_str);
/*Adjust UL resource by removing the used resource*/
void adjust_UL_resource_list(sched_temp_UL_NB_IoT_t *NPUSCH_info);
/*Initialize resource by nprach configuration*/
void Initialize_Resource(void);
/*Function to extend uplink resource grid*/
//void add_UL_Resource(eNB_MAC_INST_NB_IoT *mac_inst);
void add_UL_Resource(void);
/*Get ACK/NAK resource field*/
int get_resource_field_value(int subcarrier, int k0);
/*Get DL Repetition index*/
uint8_t get_index_Rep_dl(uint16_t R);

/*******DL Scheduler********/
void schedule_DL_NB_IoT(module_id_t module_id, eNB_MAC_INST_NB_IoT *mac_inst, UE_TEMPLATE_NB_IoT *UE_info, uint32_t hyperSF_start, uint32_t frame_start, uint32_t subframe_start);
int check_resource_NPDCCH_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, uint32_t hyperSF_start, uint32_t frame_start, uint32_t subframe_start, sched_temp_DL_NB_IoT_t *NPDCCH_info, uint32_t cdd_num, uint32_t dci_rep);
int check_resource_NPDSCH_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, sched_temp_DL_NB_IoT_t *NPDSCH_info, uint32_t sf_end, uint32_t I_delay, uint32_t R_max, uint32_t R_dl, uint32_t n_sf);
int check_resource_DL_NB_IoT(eNB_MAC_INST_NB_IoT *mac_inst, uint32_t hyperSF_start, uint32_t frame_start, uint32_t subframe_start, uint32_t dlsf_require, sched_temp_DL_NB_IoT_t *schedule_info);
uint32_t get_I_mcs(int CE_level);
uint32_t get_max_tbs(uint32_t I_tbs);
uint32_t get_tbs(uint32_t data_size, uint32_t I_tbs, uint32_t *I_sf);
uint32_t get_num_sf(uint32_t I_sf);
uint32_t get_scheduling_delay(uint32_t I_delay, uint32_t R_max);
uint32_t get_HARQ_delay(int subcarrier_spacing, uint32_t HARQ_delay_index);
//void generate_scheduling_result_DL(uint32_t DCI_subframe, uint32_t NPDSCH_subframe, uint32_t HARQ_subframe, DCIFormatN1_t *DCI, rnti_t rnti, uint32_t TBS, uint8_t *DLSCH_pdu);
void generate_scheduling_result_DL(sched_temp_DL_NB_IoT_t* DCI_info, sched_temp_DL_NB_IoT_t* NPDSCH_info, sched_temp_UL_NB_IoT_t* HARQ_info, DCIFormatN1_t *DCI_inst, rnti_t rnti, uint32_t TBS, uint8_t *DLSCH_pdu);
void fill_DCI_N1(DCIFormatN1_t *DCI_N1, UE_TEMPLATE_NB_IoT *UE_info, uint32_t scheddly, uint32_t I_sf, uint32_t I_harq);
//Transfrom source into hyperSF, Frame, Subframe format
void convert_system_number(uint32_t source_sf,uint32_t *hyperSF, uint32_t *frame, uint32_t *subframe);
//Trnasform hyperSF, Frame, Subframe format into subframe unit
uint32_t convert_system_number_sf(uint32_t hyperSF, uint32_t frame, uint32_t subframe);
/*input start position amd num_dlsf DL subframe, caculate the last subframe number*/
uint32_t cal_num_dlsf(eNB_MAC_INST_NB_IoT *mac_inst, uint32_t hyperSF, uint32_t frame, uint32_t subframe, uint32_t* hyperSF_result, uint32_t* frame_result, uint32_t* subframe_result, uint32_t num_dlsf_require);
void init_dlsf_info(eNB_MAC_INST_NB_IoT *mac_inst, DLSF_INFO_t *DLSF_info);
uint32_t generate_dlsch_header_NB_IoT(uint8_t *pdu, uint32_t num_sdu, logical_chan_id_t *logical_channel, uint32_t *sdu_length, uint8_t flag_drx, uint8_t flag_ta, uint32_t TBS);
void maintain_resource_DL(eNB_MAC_INST_NB_IoT *mac_inst, sched_temp_DL_NB_IoT_t *NPDCCH_info, sched_temp_DL_NB_IoT_t *NPDSCH_info);
void init_tool_sib1(eNB_MAC_INST_NB_IoT *mac_inst);
//int is_dlsf(eNB_MAC_INST_NB_IoT *mac_inst, int abs_subframe);
/**DL test , delete**/
available_resource_DL_t* new_dl_node(uint32_t start_subframe, uint32_t end_subframe, uint32_t dlsf);
void initialize_dl_resource(available_resource_DL_t *DL_Resource_node, uint32_t start_subframe, uint32_t end_subframe, uint32_t dlsf);
void insert_dl_resource(available_resource_DL_t *DL_Resource_node);
void insert_schedule_result(schedule_result_t **list, int subframe, schedule_result_t *node);
//interface with IF

uint8_t *parse_ulsch_header_NB_IoT( uint8_t *mac_header, uint8_t *num_ce, uint8_t *num_sdu, uint8_t *rx_ces, uint8_t *rx_lcids, uint16_t *rx_lengths, uint16_t tb_length);

void rx_sdu_NB_IoT(module_id_t module_id, int CC_id, frame_t frame, sub_frame_t subframe, uint16_t rnti, uint8_t *sdu, uint16_t  length);

int output_handler(eNB_MAC_INST_NB_IoT *mac_inst, module_id_t module_id, int CC_id, uint32_t hypersfn, uint32_t frame, uint32_t subframe, uint8_t MIB_flag, uint8_t SIB1_flag, uint32_t current_time);
// main

void mac_top_init_eNB_NB_IoT(void);

uint32_t to_earfcn_NB_IoT(int eutra_bandP,uint32_t dl_CarrierFreq, float m_dl);

uint32_t from_earfcn_NB_IoT(int eutra_bandP,uint32_t dl_earfcn, float m_dl);

int32_t get_uldl_offset_NB_IoT(int eutra_band);
#endif
