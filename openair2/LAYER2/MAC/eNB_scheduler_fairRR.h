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

/*! \file eNB_scheduler_fairRR.h
 * \brief eNB scheduler fair round robin header
 * \author Masayuki Harada
 * \date 2018
 * \email masayuki.harada@jp.fujitsu.com
 * \version 1.0
 * @ingroup _mac
 */


#ifndef __LAYER2_MAC_ENB_SCHEDULER_FAIRRR_H__
#define __LAYER2_MAC_ENB_SCHEDULER_FAIRRR_H__

/* define */
enum SCH_UE_PRIORITY {
  SCH_PRIORITY_NONE,
  SCH_DL_SI,
  SCH_DL_PAGING,
  SCH_DL_MSG2,
  SCH_DL_MSG4,
  SCH_UL_PRACH,
  SCH_UL_MSG3,
  SCH_DL_RETRANS,
  SCH_UL_RETRANS,
  SCH_DL_FIRST,
  SCH_UL_FIRST,
  SCH_UL_INACTIVE
};

typedef struct {
  int UE_id;
  enum SCH_UE_PRIORITY ue_priority;
  rnti_t rnti;
  uint16_t nb_rb;
} DLSCH_UE_INFO;

typedef struct {
  uint16_t    ue_num;
  DLSCH_UE_INFO list[20];
} DLSCH_UE_SELECT;

typedef struct {
  int     UE_id;
  enum SCH_UE_PRIORITY ue_priority;
  uint8_t start_rb;
  uint8_t nb_rb;
  uint16_t ul_total_buffer;
} ULSCH_UE_INFO;

typedef struct {
  uint8_t ue_num;
  ULSCH_UE_INFO list[20];
} ULSCH_UE_SELECT;

/* proto */
void set_dl_ue_select_msg2(int CC_idP, uint16_t nb_rb, int UE_id, rnti_t rnti);
void set_dl_ue_select_msg4(int CC_idP, uint16_t nb_rb, int UE_id, rnti_t rnti);

void dlsch_scheduler_pre_ue_select_fairRR(
    module_id_t     module_idP,
    frame_t         frameP,
    sub_frame_t     subframeP,
    int*            mbsfn_flag,
    uint16_t        nb_rbs_required[MAX_NUM_CCs][MAX_MOBILES_PER_ENB],
    DLSCH_UE_SELECT dlsch_ue_select[MAX_NUM_CCs]);

void dlsch_scheduler_pre_processor_allocate_fairRR(
    module_id_t Mod_id,
    int UE_id,
    uint8_t CC_id,
    int N_RBG,
    uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
    uint16_t nb_rbs_remaining[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
    uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX]);

void dlsch_scheduler_pre_processor_fairRR (module_id_t   Mod_id,
                                    frame_t       frameP,
                                    sub_frame_t   subframeP,
                                    int           N_RBG[MAX_NUM_CCs],
                                    int           *mbsfn_flag);

void fill_DLSCH_dci_fairRR(
	       module_id_t module_idP,
	       frame_t frameP,
	       sub_frame_t subframeP,
	       int* mbsfn_flagP);

void schedule_ue_spec_fairRR(module_id_t module_idP,
		 frame_t frameP, sub_frame_t subframeP, int *mbsfn_flag);

void ulsch_scheduler_pre_ue_select_fairRR(
    module_id_t       module_idP,
    frame_t           frameP,
    sub_frame_t       subframeP,
    sub_frame_t       sched_subframeP,
    ULSCH_UE_SELECT   ulsch_ue_select[MAX_NUM_CCs]);

void ulsch_scheduler_pre_processor_fairRR(module_id_t module_idP,
                                   frame_t frameP,
                                   sub_frame_t subframeP,
                                   sub_frame_t sched_subframeP,
                                   ULSCH_UE_SELECT ulsch_ue_select[MAX_NUM_CCs]);

void schedule_ulsch_fairRR(module_id_t module_idP, frame_t frameP,
	       sub_frame_t subframeP);

void schedule_ulsch_rnti_fairRR(module_id_t   module_idP,
                         frame_t       frameP,
                         sub_frame_t   subframeP,
                         unsigned char sched_subframeP,
                         ULSCH_UE_SELECT  ulsch_ue_select[MAX_NUM_CCs]);


/* extern */
extern DLSCH_UE_SELECT dlsch_ue_select[MAX_NUM_CCs];
extern int last_dlsch_ue_id[MAX_NUM_CCs];
extern int last_ulsch_ue_id[MAX_NUM_CCs];

#endif
