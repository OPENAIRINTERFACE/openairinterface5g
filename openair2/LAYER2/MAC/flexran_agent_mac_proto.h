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

/*! \file flexran_agent_mac_proto.h
 * \brief MAC functions for FlexRAN agent
 * \author Xenofon Foukas and Navid Nikaein
 * \date 2016
 * \email: x.foukas@sms.ed.ac.uk
 * \version 0.1
 * @ingroup _mac

 */

#ifndef __LAYER2_MAC_FLEXRAN_AGENT_MAC_PROTO_H__
#define __LAYER2_MAC_FLEXRAN_AGENT_MAC_PROTO_H__

#include "flexran_agent_defs.h"
#include "header.pb-c.h"
#include "flexran.pb-c.h"

/*
 * slice specific scheduler 
 */
typedef void (*slice_scheduler) (module_id_t mod_id,
				 int slice_id,
				 uint32_t frame,
				 uint32_t subframe,
				 int *mbsfn_flag,
				 Protocol__FlexranMessage ** dl_info);



/*
 * top level flexran scheduler used by the eNB scheduler
 */
void flexran_schedule_ue_spec_default(mid_t mod_id,
				      uint32_t frame,
				      uint32_t subframe,
				      int *mbsfn_flag,
				      Protocol__FlexranMessage ** dl_info);
/*
 * slice specific scheduler for embb
 */
void
flexran_schedule_ue_spec_embb(mid_t mod_id,
			      int slice_id,
			      uint32_t frame,
			      uint32_t subframe,
			      int *mbsfn_flag,
			      Protocol__FlexranMessage ** dl_info);
/*
 * slice specific scheduler for urllc
 */
void
flexran_schedule_ue_spec_urllc(mid_t mod_id,
			       int slice_id,
			       uint32_t frame,
			       uint32_t subframe,
			       int *mbsfn_flag,
			       Protocol__FlexranMessage ** dl_info);

/*
 * slice specific scheduler for mmtc
 */
void
flexran_schedule_ue_spec_mmtc(mid_t mod_id,
			      int slice_id,
			      uint32_t frame,
			      uint32_t subframe,
			      int *mbsfn_flag,
			      Protocol__FlexranMessage ** dl_info);
/*
 * slice specific scheduler for best effort traffic 
 */
void
flexran_schedule_ue_spec_be(mid_t mod_id,
			    int slice_id,
			    uint32_t frame,
			    uint32_t subframe,
			    int *mbsfn_flag,
			    Protocol__FlexranMessage ** dl_info);

/*
 * common flexran scheduler function
 */
void
flexran_schedule_ue_spec_common(mid_t mod_id,
				int slice_id,
				uint32_t frame,
				uint32_t subframe,
				int *mbsfn_flag,
				Protocol__FlexranMessage ** dl_info);

uint16_t flexran_nb_rbs_allowed_slice(float rb_percentage, int total_rbs);

int flexran_slice_member(int UE_id, int slice_id);

int flexran_slice_maxmcs(int slice_id);

void _store_dlsch_buffer(module_id_t Mod_id,
			 int slice_id,
			 frame_t frameP, sub_frame_t subframeP);


void _assign_rbs_required(module_id_t Mod_id,
			  int slice_id,
			  frame_t frameP,
			  sub_frame_t subframe,
			  uint16_t
			  nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX],
			  uint16_t
			  nb_rbs_allowed_slice[MAX_NUM_CCs]
			  [MAX_NUM_SLICES], int min_rb_unit[MAX_NUM_CCs]);

void _dlsch_scheduler_pre_processor(module_id_t Mod_id,
				    int slice_id,
				    frame_t frameP,
				    sub_frame_t subframeP,
				    int N_RBG[MAX_NUM_CCs],
				    int *mbsfn_flag);

void _dlsch_scheduler_pre_processor_reset(int module_idP,
					  int UE_id,
					  uint8_t CC_id,
					  int frameP,
					  int subframeP,
					  int N_RBG,
					  uint16_t
					  nb_rbs_required[MAX_NUM_CCs]
					  [NUMBER_OF_UE_MAX],
					  uint16_t
					  nb_rbs_required_remaining
					  [MAX_NUM_CCs][NUMBER_OF_UE_MAX],
					  uint16_t
					  nb_rbs_allowed_slice[MAX_NUM_CCs]
					  [MAX_NUM_SLICES],
					  unsigned char
					  rballoc_sub[MAX_NUM_CCs]
					  [N_RBG_MAX],
					  unsigned char
					  MIMO_mode_indicator[MAX_NUM_CCs]
					  [N_RBG_MAX]);

void _dlsch_scheduler_pre_processor_allocate(module_id_t Mod_id,
					     int UE_id,
					     uint8_t CC_id,
					     int N_RBG,
					     int transmission_mode,
					     int min_rb_unit,
					     uint8_t N_RB_DL,
					     uint16_t
					     nb_rbs_required[MAX_NUM_CCs]
					     [NUMBER_OF_UE_MAX],
					     uint16_t
					     nb_rbs_required_remaining
					     [MAX_NUM_CCs]
					     [NUMBER_OF_UE_MAX],
					     unsigned char
					     rballoc_sub[MAX_NUM_CCs]
					     [N_RBG_MAX],
					     unsigned char
					     MIMO_mode_indicator
					     [MAX_NUM_CCs][N_RBG_MAX]);

/*
 * Default scheduler used by the eNB agent
 */
void flexran_schedule_ue_spec_default(mid_t mod_id, uint32_t frame,
				      uint32_t subframe, int *mbsfn_flag,
				      Protocol__FlexranMessage ** dl_info);

/*
 * Data plane function for applying the DL decisions of the scheduler
 */
void flexran_apply_dl_scheduling_decisions(mid_t mod_id, uint32_t frame,
					   uint32_t subframe,
					   int *mbsfn_flag,
					   Protocol__FlexranMessage *
					   dl_scheduling_info);

/*
 * Data plane function for applying the UE specific DL decisions of the scheduler
 */
void flexran_apply_ue_spec_scheduling_decisions(mid_t mod_id,
						uint32_t frame,
						uint32_t subframe,
						int *mbsfn_flag,
						uint32_t n_dl_ue_data,
						Protocol__FlexDlData **
						dl_ue_data);

/*
 * Data plane function for filling the DCI structure
 */
void flexran_fill_oai_dci(mid_t mod_id, uint32_t CC_id, uint32_t rnti,
			  Protocol__FlexDlDci * dl_dci);

#endif
