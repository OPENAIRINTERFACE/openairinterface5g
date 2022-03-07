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
 * \brief eNB scheduler fair round robin
 * \author Masayuki Harada
 * \date 2018
 * \email masayuki.harada@jp.fujitsu.com
 * \version 1.0
 * @ingroup _mac
 */

#define _GNU_SOURCE
#include <stdlib.h>

#include "assertions.h"

#include "PHY/phy_extern.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"
#include "SIMULATION/TOOLS/sim.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/MAC/eNB_scheduler_fairRR.h"
#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "rlc.h"
#include "common/utils/lte/prach_utils.h"
#include "T.h"


#ifdef PHY_TX_THREAD
  extern volatile int16_t phy_tx_txdataF_end;
  extern int oai_exit;
#endif
extern uint16_t sfnsf_add_subframe(uint16_t frameP, uint16_t subframeP, int offset);
extern void add_subframe(uint16_t *frameP, uint16_t *subframeP, int offset);

/* internal vars */
DLSCH_UE_SELECT dlsch_ue_select[MAX_NUM_CCs];
int last_dlsch_ue_id[MAX_NUM_CCs] = {-1};
int last_dlsch_ue_id_volte[MAX_NUM_CCs] = {-1};
int last_ulsch_ue_id[MAX_NUM_CCs] = {-1};
int last_ulsch_ue_id_volte[MAX_NUM_CCs] = {-1};

#if defined(PRE_SCD_THREAD)
  uint16_t pre_nb_rbs_required[2][MAX_NUM_CCs][NUMBER_OF_UE_MAX];
  uint8_t dlsch_ue_select_tbl_in_use;
  uint8_t new_dlsch_ue_select_tbl_in_use;
  boolean_t pre_scd_activeUE[NUMBER_OF_UE_MAX];
  eNB_UE_STATS pre_scd_eNB_UE_stats[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
#endif

#define DEBUG_eNB_SCHEDULER 1
#define DEBUG_HEADER_PARSING 1
//#define DEBUG_PACKET_TRACE 1

void set_dl_ue_select_msg2(int CC_idP, uint16_t nb_rb, int UE_id, rnti_t rnti) {
  dlsch_ue_select[CC_idP].list[dlsch_ue_select[CC_idP].ue_num].ue_priority = SCH_DL_MSG2;
  dlsch_ue_select[CC_idP].list[dlsch_ue_select[CC_idP].ue_num].nb_rb = nb_rb;
  dlsch_ue_select[CC_idP].list[dlsch_ue_select[CC_idP].ue_num].UE_id = UE_id;
  dlsch_ue_select[CC_idP].list[dlsch_ue_select[CC_idP].ue_num].rnti = rnti;
  dlsch_ue_select[CC_idP].ue_num++;
}
void set_dl_ue_select_msg4(int CC_idP, uint16_t nb_rb, int UE_id, rnti_t rnti) {
  dlsch_ue_select[CC_idP].list[dlsch_ue_select[CC_idP].ue_num].ue_priority = SCH_DL_MSG4;
  dlsch_ue_select[CC_idP].list[dlsch_ue_select[CC_idP].ue_num].nb_rb = nb_rb;
  dlsch_ue_select[CC_idP].list[dlsch_ue_select[CC_idP].ue_num].UE_id = UE_id;
  dlsch_ue_select[CC_idP].list[dlsch_ue_select[CC_idP].ue_num].rnti = rnti;
  dlsch_ue_select[CC_idP].ue_num++;
}
#if defined(PRE_SCD_THREAD)
inline uint16_t search_rbs_required(uint16_t mcs, uint16_t TBS,uint16_t NB_RB, uint16_t step_size) {
  uint16_t nb_rb,i_TBS,tmp_TBS;
  i_TBS=get_I_TBS(mcs);

  for(nb_rb=step_size; nb_rb<NB_RB; nb_rb+=step_size) {
    tmp_TBS = TBStable[i_TBS][nb_rb-1]>>3;

    if(TBS<tmp_TBS)return(nb_rb);
  }

  return NB_RB;
}
void pre_scd_nb_rbs_required(    module_id_t     module_idP,
                                 frame_t         frameP,
                                 sub_frame_t     subframeP,
                                 int             min_rb_unit[MAX_NUM_CCs],
                                 uint16_t        nb_rbs_required[MAX_NUM_CCs][NUMBER_OF_UE_MAX]) {
  int                          CC_id=0,UE_id, lc_id, N_RB_DL;
  UE_TEMPLATE                  UE_template;
  eNB_UE_STATS                 *eNB_UE_stats;
  rnti_t                       rnti;
  mac_rlc_status_resp_t        rlc_status;
  uint16_t                     step_size=2;
  N_RB_DL = to_prb(RC.mac[module_idP]->common_channels[CC_id].mib->message.dl_Bandwidth);

  if(N_RB_DL==50) step_size=3;

  if(N_RB_DL==100) step_size=4;

  memset(nb_rbs_required, 0, sizeof(uint16_t)*MAX_NUM_CCs*NUMBER_OF_UE_MAX);
  UE_info_t *UE_info = &RC.mac[module_idP]->UE_info;

  for (UE_id = 0; UE_id <NUMBER_OF_UE_MAX; UE_id++) {
    if (pre_scd_activeUE[UE_id] != TRUE)
      continue;

    // store dlsch buffer
    // clear logical channel interface variables
    UE_template.dl_buffer_total = 0;
    rnti = UE_RNTI(module_idP, UE_id);

    for (lc_id = DCCH; lc_id <= DTCH; lc_id++) {
      rlc_status =
        mac_rlc_status_ind(module_idP, rnti, module_idP, frameP, subframeP,
                           ENB_FLAG_YES, MBMS_FLAG_NO, lc_id, 0, 0
                          );
      UE_template.dl_buffer_total += rlc_status.bytes_in_buffer; //storing the total dlsch buffer
    }

    // end of store dlsch buffer
    // assgin rbs required
    // Calculate the number of RBs required by each UE on the basis of logical channel's buffer
    //update CQI information across component carriers
    eNB_UE_stats = &pre_scd_eNB_UE_stats[CC_id][UE_id];
    eNB_UE_stats->dlsch_mcs1 = cqi_to_mcs[UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id]];

    if (UE_template.dl_buffer_total > 0) {
      nb_rbs_required[CC_id][UE_id] = search_rbs_required(eNB_UE_stats->dlsch_mcs1, UE_template.dl_buffer_total, N_RB_DL, step_size);
    }
  }
}
#endif

int cc_id_end(uint8_t *cc_id_flag ) {
  int end_flag = 1;

  for (int CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    if (cc_id_flag[CC_id]==0) {
      end_flag = 0;
      break;
    }
  }

  return end_flag;
}

void dlsch_scheduler_pre_ue_select_fairRR(
  module_id_t     module_idP,
  frame_t         frameP,
  sub_frame_t     subframeP,
  int            *mbsfn_flag,
  uint16_t        nb_rbs_required[MAX_NUM_CCs][MAX_MOBILES_PER_ENB],
  DLSCH_UE_SELECT dlsch_ue_select[MAX_NUM_CCs]) {
  eNB_MAC_INST                   *eNB      = RC.mac[module_idP];
  COMMON_channels_t              *cc       = eNB->common_channels;
  UE_info_t                      *UE_info  = &eNB->UE_info;
  UE_sched_ctrl_t                  *ue_sched_ctl;
  uint8_t                        CC_id;
  int                            UE_id;
  unsigned char                  round             = 0;
  unsigned char                  harq_pid          = 0;
  rnti_t                         rnti;
  uint16_t                       i;
  unsigned char                  aggregation;
  int                            format_flag;
  nfapi_dl_config_request_body_t *DL_req;
  nfapi_dl_config_request_pdu_t  *dl_config_pdu;
  uint16_t                       dlsch_ue_max_num[MAX_NUM_CCs] = {0};
  uint16_t                       saved_dlsch_dci[MAX_NUM_CCs] = {0};
  uint8_t                        end_flag[MAX_NUM_CCs] = {0};

  // Initialization
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    dlsch_ue_max_num[CC_id] = eNB->ue_multiple_max;
    // save origin DL PDU number
    DL_req          = &eNB->DL_req[CC_id].dl_config_request_body;
    saved_dlsch_dci[CC_id] = DL_req->number_pdu;
  }

  // Insert DLSCH(retransmission) UE into selected UE list
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    if (mbsfn_flag[CC_id]>0) {
      continue;
    }

    DL_req          = &eNB->DL_req[CC_id].dl_config_request_body;

    for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
      if (UE_info->active[UE_id] == FALSE) {
        continue;
      }

      rnti = UE_RNTI(module_idP, UE_id);

      if (rnti == NOT_A_RNTI) {
        continue;
      }


      if (UE_info->UE_sched_ctrl[UE_id].ul_failure_timer > 0
          || UE_info->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
        continue;

      if(mac_eNB_get_rrc_status(module_idP,rnti) < RRC_CONNECTED) {
        continue;
      }

      ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
      harq_pid = frame_subframe2_dl_harq_pid(cc[CC_id].tdd_Config,frameP,subframeP);
      round = ue_sched_ctl->round[CC_id][harq_pid];

      if (round != 8) {  // retransmission
        if(UE_info->UE_template[CC_id][UE_id].nb_rb[harq_pid] == 0) {
          continue;
        }

        switch (get_tmode(module_idP, CC_id, UE_id)) {
          case 1:
          case 2:
          case 7:
            aggregation = get_aggregation(get_bw_index(module_idP, CC_id),
                                          ue_sched_ctl->dl_cqi[CC_id],
                                          format1);
            break;

          case 3:
            aggregation = get_aggregation(get_bw_index(module_idP,CC_id),
                                          ue_sched_ctl->dl_cqi[CC_id],
                                          format2A);
            break;

          default:
            LOG_W(MAC,"Unsupported transmission mode %d\n", get_tmode(module_idP,CC_id,UE_id));
            aggregation = 2;
            break;
        }

        format_flag = 1;

        if (!CCE_allocation_infeasible(module_idP,
                                       CC_id,
                                       format_flag,
                                       subframeP,
                                       aggregation,
                                       rnti)) {
          dl_config_pdu = &DL_req->dl_config_pdu_list[DL_req->number_pdu];
          dl_config_pdu->pdu_type                                     = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti              = rnti;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type         = (format_flag == 0)?2:1;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level = aggregation;
          DL_req->number_pdu++;
          nb_rbs_required[CC_id][UE_id] = UE_info->UE_template[CC_id][UE_id].nb_rb[harq_pid];
          // Insert DLSCH(retransmission) UE into selected UE list
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].UE_id = UE_id;
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].ue_priority = SCH_DL_RETRANS;
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].rnti = rnti;
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].nb_rb = nb_rbs_required[CC_id][UE_id];
          dlsch_ue_select[CC_id].ue_num++;

          if (dlsch_ue_select[CC_id].ue_num == dlsch_ue_max_num[CC_id]) {
            end_flag[CC_id] = 1;
            break;
          }
        } else {
          if (cc[CC_id].tdd_Config != NULL) { //TDD
            set_ue_dai (subframeP,
                        UE_id,
                        CC_id,
                        cc[CC_id].tdd_Config->subframeAssignment,
                        UE_info);
            // update UL DAI after DLSCH scheduling
            set_ul_DAI(module_idP,UE_id,CC_id,frameP,subframeP);
          }

          add_ue_dlsch_info(module_idP,
                            CC_id,
                            UE_id,
                            subframeP,
                            S_DL_NONE,
                            rnti);
          end_flag[CC_id] = 1;
          break;
        }
      }
    }
  }

  if(cc_id_end(end_flag) == 1) {
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
      DL_req          = &eNB->DL_req[CC_id].dl_config_request_body;
      DL_req->number_pdu = saved_dlsch_dci[CC_id];
    }

    return;
  }

  // Insert DLSCH(first transmission) UE into selected UE list (UE_id > last_dlsch_ue_id[CC_id])
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    if (mbsfn_flag[CC_id]>0) {
      continue;
    }

    DL_req          = &eNB->DL_req[CC_id].dl_config_request_body;

    for (UE_id = (last_dlsch_ue_id[CC_id]+1); UE_id <NUMBER_OF_UE_MAX; UE_id++) {
      if(end_flag[CC_id] == 1) {
        break;
      }

      if (UE_info->active[UE_id] == FALSE) {
        continue;
      }

      rnti = UE_RNTI(module_idP,UE_id);

      if (rnti == NOT_A_RNTI)
        continue;

      if (UE_info->UE_sched_ctrl[UE_id].ul_failure_timer > 0
          || UE_info->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
        continue;

      if(mac_eNB_get_rrc_status(module_idP,rnti) < RRC_CONNECTED) {
        continue;
      }

      ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];

      for(i = 0; i<dlsch_ue_select[CC_id].ue_num; i++) {
        if(dlsch_ue_select[CC_id].list[i].UE_id == UE_id) {
          break;
        }
      }

      if(i < dlsch_ue_select[CC_id].ue_num)
        continue;

      harq_pid = frame_subframe2_dl_harq_pid(cc[CC_id].tdd_Config,frameP,subframeP);
      round = ue_sched_ctl->round[CC_id][harq_pid];

      if (round == 8) {
        if (nb_rbs_required[CC_id][UE_id] == 0) {
          continue;
        }

        switch (get_tmode(module_idP, CC_id, UE_id)) {
          case 1:
          case 2:
          case 7:
            aggregation = get_aggregation(get_bw_index(module_idP, CC_id),
                                          ue_sched_ctl->dl_cqi[CC_id],
                                          format1);
            break;

          case 3:
            aggregation = get_aggregation(get_bw_index(module_idP,CC_id),
                                          ue_sched_ctl->dl_cqi[CC_id],
                                          format2A);
            break;

          default:
            LOG_W(MAC,"Unsupported transmission mode %d\n", get_tmode(module_idP,CC_id,UE_id));
            aggregation = 2;
            break;
        }

        format_flag = 1;

        if (!CCE_allocation_infeasible(module_idP,
                                       CC_id,
                                       format_flag,
                                       subframeP,
                                       aggregation,
                                       rnti)) {
          dl_config_pdu = &DL_req->dl_config_pdu_list[DL_req->number_pdu];
          dl_config_pdu->pdu_type                                     = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti              = rnti;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type         = (format_flag == 0)?2:1;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level = aggregation;
          DL_req->number_pdu++;
          // Insert DLSCH(first transmission) UE into selected selected UE list
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].ue_priority = SCH_DL_FIRST;
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].nb_rb = nb_rbs_required[CC_id][UE_id];
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].UE_id = UE_id;
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].rnti = rnti;
          dlsch_ue_select[CC_id].ue_num++;

          if (dlsch_ue_select[CC_id].ue_num == dlsch_ue_max_num[CC_id]) {
            end_flag[CC_id] = 1;
            break;
          }
        } else {
          if (cc[CC_id].tdd_Config != NULL) { //TDD
            set_ue_dai (subframeP,
                        UE_id,
                        CC_id,
                        cc[CC_id].tdd_Config->subframeAssignment,
                        UE_info);
            // update UL DAI after DLSCH scheduling
            set_ul_DAI(module_idP,UE_id,CC_id,frameP,subframeP);
          }

          add_ue_dlsch_info(module_idP,
                            CC_id,
                            UE_id,
                            subframeP,
                            S_DL_NONE,
                            rnti);
          end_flag[CC_id] = 1;
          break;
        }
      }
    }
  }

  if(cc_id_end(end_flag) == 1) {
    for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
      DL_req          = &eNB->DL_req[CC_id].dl_config_request_body;
      DL_req->number_pdu = saved_dlsch_dci[CC_id];
    }

    return;
  }

  // Insert DLSCH(first transmission) UE into selected UE list (UE_id <= last_dlsch_ue_id[CC_id])
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    if (mbsfn_flag[CC_id]>0) {
      continue;
    }

    DL_req          = &eNB->DL_req[CC_id].dl_config_request_body;

    for (UE_id = 0; UE_id <= last_dlsch_ue_id[CC_id]; UE_id++) {
      if(end_flag[CC_id] == 1) {
        break;
      }

      if (UE_info->active[UE_id] == FALSE) {
        continue;
      }

      rnti = UE_RNTI(module_idP,UE_id);

      if (rnti == NOT_A_RNTI)
        continue;

      if (UE_info->UE_sched_ctrl[UE_id].ul_failure_timer > 0
          || UE_info->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
        continue;

      if(mac_eNB_get_rrc_status(module_idP,rnti) < RRC_CONNECTED) {
        continue;
      }

      ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];

      for(i = 0; i<dlsch_ue_select[CC_id].ue_num; i++) {
        if(dlsch_ue_select[CC_id].list[i].UE_id == UE_id) {
          break;
        }
      }

      if(i < dlsch_ue_select[CC_id].ue_num)
        continue;

      harq_pid = frame_subframe2_dl_harq_pid(cc[CC_id].tdd_Config,frameP,subframeP);
      round = ue_sched_ctl->round[CC_id][harq_pid];

      if (round == 8) {
        if (nb_rbs_required[CC_id][UE_id] == 0) {
          continue;
        }

        switch (get_tmode(module_idP, CC_id, UE_id)) {
          case 1:
          case 2:
          case 7:
            aggregation = get_aggregation(get_bw_index(module_idP, CC_id),
                                          ue_sched_ctl->dl_cqi[CC_id],
                                          format1);
            break;

          case 3:
            aggregation = get_aggregation(get_bw_index(module_idP,CC_id),
                                          ue_sched_ctl->dl_cqi[CC_id],
                                          format2A);
            break;

          default:
            LOG_W(MAC,"Unsupported transmission mode %d\n", get_tmode(module_idP,CC_id,UE_id));
            aggregation = 2;
            break;
        }

        format_flag = 1;

        if (!CCE_allocation_infeasible(module_idP,
                                       CC_id,
                                       format_flag,
                                       subframeP,
                                       aggregation,
                                       rnti)) {
          dl_config_pdu = &DL_req->dl_config_pdu_list[DL_req->number_pdu];
          dl_config_pdu->pdu_type                                     = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti              = rnti;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type         = (format_flag == 0)?2:1;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level = aggregation;
          DL_req->number_pdu++;
          // Insert DLSCH(first transmission) UE into selected selected UE list
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].ue_priority = SCH_DL_FIRST;
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].nb_rb = nb_rbs_required[CC_id][UE_id];
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].UE_id = UE_id;
          dlsch_ue_select[CC_id].list[dlsch_ue_select[CC_id].ue_num].rnti = rnti;
          dlsch_ue_select[CC_id].ue_num++;

          if (dlsch_ue_select[CC_id].ue_num == dlsch_ue_max_num[CC_id]) {
            end_flag[CC_id] = 1;
            break;
          }
        } else {
          if (cc[CC_id].tdd_Config != NULL) { //TDD
            set_ue_dai (subframeP,
                        UE_id,
                        CC_id,
                        cc[CC_id].tdd_Config->subframeAssignment,
                        UE_info);
            // update UL DAI after DLSCH scheduling
            set_ul_DAI(module_idP,UE_id,CC_id,frameP,subframeP);
          }

          add_ue_dlsch_info(module_idP,
                            CC_id,
                            UE_id,
                            subframeP,
                            S_DL_NONE,
                            rnti);
          end_flag[CC_id] = 1;
          break;
        }
      }
    }
  }

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    DL_req          = &eNB->DL_req[CC_id].dl_config_request_body;
    DL_req->number_pdu = saved_dlsch_dci[CC_id];
  }

  return;
}

static void dlsch_scheduler_pre_processor_reset_fairRR(
    module_id_t module_idP,
    frame_t frameP,
    sub_frame_t subframeP,
    int min_rb_unit[NFAPI_CC_MAX],
    uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
    uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX],
    uint8_t MIMO_mode_indicator[NFAPI_CC_MAX][N_RBG_MAX]) {
  int UE_id;
  uint8_t CC_id;
  int i, j;
  UE_info_t *UE_info;
  UE_sched_ctrl_t *ue_sched_ctl;
  int N_RB_DL, RBGsize, RBGsize_last;
  int N_RBG[NFAPI_CC_MAX];
  rnti_t rnti;
  uint8_t *vrb_map;
  COMMON_channels_t *cc;

  //
  for (CC_id = 0; CC_id < RC.nb_mac_CC[module_idP]; CC_id++) {
    // initialize harq_pid and round
    cc = &RC.mac[module_idP]->common_channels[CC_id];
    N_RBG[CC_id] = to_rbg(cc->mib->message.dl_Bandwidth);
    min_rb_unit[CC_id] = get_min_rb_unit(module_idP, CC_id);

    for (UE_id = 0; UE_id < MAX_MOBILES_PER_ENB; ++UE_id) {
      UE_info = &RC.mac[module_idP]->UE_info;
      ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
      rnti = UE_RNTI(module_idP, UE_id);

      if (rnti == NOT_A_RNTI)
        continue;

      if (UE_info->active[UE_id] != TRUE)
        continue;

      LOG_D(MAC, "Running preprocessor for UE %d (%x)\n", UE_id, rnti);

      // initialize harq_pid and round
      if (ue_sched_ctl->ta_timer)
        ue_sched_ctl->ta_timer--;

      /*
         eNB_UE_stats *eNB_UE_stats;

         if (eNB_UE_stats == NULL)
         return;


         mac_xface->get_ue_active_harq_pid(module_idP,CC_id,rnti,
         frameP,subframeP,
         &ue_sched_ctl->harq_pid[CC_id],
         &ue_sched_ctl->round[CC_id],
         openair_harq_DL);


         if (ue_sched_ctl->ta_timer == 0) {

         // WE SHOULD PROTECT the eNB_UE_stats with a mutex here ...

         ue_sched_ctl->ta_timer = 20;  // wait 20 subframes before taking TA measurement from PHY
         switch (N_RB_DL) {
         case 6:
         ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update;
         break;

         case 15:
         ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/2;
         break;

         case 25:
         ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/4;
         break;

         case 50:
         ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/8;
         break;

         case 75:
         ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/12;
         break;

         case 100:
         ue_sched_ctl->ta_update = eNB_UE_stats->timing_advance_update/16;
         break;
         }
         // clear the update in case PHY does not have a new measurement after timer expiry
         eNB_UE_stats->timing_advance_update =  0;
         }
         else {
         ue_sched_ctl->ta_timer--;
         ue_sched_ctl->ta_update =0; // don't trigger a timing advance command
         }


         if (UE_id==0) {
         VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_TIMING_ADVANCE,ue_sched_ctl->ta_update);
         }
       */
      nb_rbs_required[CC_id][UE_id] = 0;
      ue_sched_ctl->pre_nb_available_rbs[CC_id] = 0;
      ue_sched_ctl->dl_pow_off[CC_id] = 2;

      for (i = 0; i < N_RBG[CC_id]; i++) {
        ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 0;
      }
    }

    N_RB_DL = to_prb(RC.mac[module_idP]->common_channels[CC_id].mib->message.dl_Bandwidth);

    switch (N_RB_DL) {
      case 6:
        RBGsize = 1;
        RBGsize_last = 1;
        break;

      case 15:
        RBGsize = 2;
        RBGsize_last = 1;
        break;

      case 25:
        RBGsize = 2;
        RBGsize_last = 1;
        break;

      case 50:
        RBGsize = 3;
        RBGsize_last = 2;
        break;

      case 75:
        RBGsize = 4;
        RBGsize_last = 3;
        break;

      case 100:
        RBGsize = 4;
        RBGsize_last = 4;
        break;

      default:
        AssertFatal(1 == 0, "unsupported RBs (%d)\n", N_RB_DL);
    }

    vrb_map = RC.mac[module_idP]->common_channels[CC_id].vrb_map;

    // Initialize Subbands according to VRB map
    for (i = 0; i < N_RBG[CC_id]; i++) {
      int rb_size = i == N_RBG[CC_id] - 1 ? RBGsize_last : RBGsize;

      // for SI-RNTI,RA-RNTI and P-RNTI allocations
      for (j = 0; j < rb_size; j++) {
        if (vrb_map[j + (i*RBGsize)] != 0) {
          rballoc_sub[CC_id][i] = 1;
          LOG_D(MAC, "Frame %d, subframe %d : vrb %d allocated\n", frameP, subframeP, j + (i*RBGsize));
          break;
        }
      }

      //LOG_D(MAC, "Frame %d Subframe %d CC_id %d RBG %i : rb_alloc %d\n",
      //frameP, subframeP, CC_id, i, rballoc_sub[CC_id][i]);
      MIMO_mode_indicator[CC_id][i] = 2;
    }
  }
}

// This function returns the estimated number of RBs required by each UE for downlink scheduling
static void assign_rbs_required_fairRR(
    module_id_t Mod_id,
    frame_t frameP,
    sub_frame_t subframe,
    uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB]) {
  uint16_t TBS = 0;
  int UE_id, n, i, j, CC_id, pCCid, tmp;
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;
  eNB_UE_STATS *eNB_UE_stats, *eNB_UE_stats_i, *eNB_UE_stats_j;
  int N_RB_DL;

  // clear rb allocations across all CC_id
  for (UE_id = 0; UE_id < MAX_MOBILES_PER_ENB; UE_id++) {
    if (UE_info->active[UE_id] != TRUE)
      continue;

    pCCid = UE_PCCID(Mod_id, UE_id);

    // update CQI information across component carriers
    for (n = 0; n < UE_info->numactiveCCs[UE_id]; n++) {
      CC_id = UE_info->ordered_CCids[n][UE_id];
      eNB_UE_stats = &UE_info->eNB_UE_stats[CC_id][UE_id];
      eNB_UE_stats->dlsch_mcs1 = cqi_to_mcs[UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id]];
    }

    // provide the list of CCs sorted according to MCS
    for (i = 0; i < UE_info->numactiveCCs[UE_id]; ++i) {
      eNB_UE_stats_i =
          &UE_info->eNB_UE_stats[UE_info->ordered_CCids[i][UE_id]][UE_id];

      for (j = i + 1; j < UE_info->numactiveCCs[UE_id]; j++) {
        DevAssert(j < NFAPI_CC_MAX);
        eNB_UE_stats_j =
            &UE_info->eNB_UE_stats[UE_info->ordered_CCids[j][UE_id]][UE_id];

        if (eNB_UE_stats_j->dlsch_mcs1 > eNB_UE_stats_i->dlsch_mcs1) {
          tmp = UE_info->ordered_CCids[i][UE_id];
          UE_info->ordered_CCids[i][UE_id] = UE_info->ordered_CCids[j][UE_id];
          UE_info->ordered_CCids[j][UE_id] = tmp;
        }
      }
    }

    if (UE_info->UE_template[pCCid][UE_id].dl_buffer_total > 0) {
      LOG_D(MAC, "[preprocessor] assign RB for UE %d\n", UE_id);

      for (i = 0; i < UE_info->numactiveCCs[UE_id]; i++) {
        CC_id = UE_info->ordered_CCids[i][UE_id];
        eNB_UE_stats = &UE_info->eNB_UE_stats[CC_id][UE_id];
        const int min_rb_unit = get_min_rb_unit(Mod_id, CC_id);

        if (eNB_UE_stats->dlsch_mcs1 == 0) {
          nb_rbs_required[CC_id][UE_id] = 4; // don't let the TBS get too small
        } else {
          nb_rbs_required[CC_id][UE_id] = min_rb_unit;
        }

        TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, nb_rbs_required[CC_id][UE_id]);
        LOG_D(MAC,
              "[preprocessor] start RB assignement for UE %d CC_id %d dl "
              "buffer %d (RB unit %d, MCS %d, TBS %d) \n",
              UE_id,
              CC_id,
              UE_info->UE_template[pCCid][UE_id].dl_buffer_total,
              nb_rbs_required[CC_id][UE_id],
              eNB_UE_stats->dlsch_mcs1,
              TBS);
        N_RB_DL = to_prb(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);

        /* calculating required number of RBs for each UE */
        while (TBS < UE_info->UE_template[pCCid][UE_id].dl_buffer_total) {
          nb_rbs_required[CC_id][UE_id] += min_rb_unit;

          if (nb_rbs_required[CC_id][UE_id] > N_RB_DL) {
            TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, N_RB_DL);
            nb_rbs_required[CC_id][UE_id] = N_RB_DL;
            break;
          }

          TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1,
                           nb_rbs_required[CC_id][UE_id]);
        } // end of while

        LOG_D(MAC,
              "[eNB %d] Frame %d: UE %d on CC %d: RB unit %d,  nb_required RB "
              "%d (TBS %d, mcs %d)\n",
              Mod_id,
              frameP,
              UE_id,
              CC_id,
              min_rb_unit,
              nb_rbs_required[CC_id][UE_id],
              TBS,
              eNB_UE_stats->dlsch_mcs1);
      }
    }
  }
}

void dlsch_scheduler_pre_processor_allocate_fairRR(
    module_id_t Mod_id,
    int UE_id,
    uint8_t CC_id,
    int N_RBG,
    uint16_t nb_rbs_required[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
    uint16_t nb_rbs_remaining[NFAPI_CC_MAX][MAX_MOBILES_PER_ENB],
    uint8_t rballoc_sub[NFAPI_CC_MAX][N_RBG_MAX]) {
  int i;
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;
  UE_sched_ctrl_t *ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
  int N_RB_DL =
      to_prb(RC.mac[Mod_id]->common_channels[CC_id].mib->message.dl_Bandwidth);
  const int min_rb_unit = get_min_rb_unit(Mod_id, CC_id);

  for (i = 0; i < N_RBG; i++) {
    if (rballoc_sub[CC_id][i] != 0)
      continue;

    if (ue_sched_ctl->rballoc_sub_UE[CC_id][i] != 0)
      continue;

    if (nb_rbs_remaining[CC_id][UE_id] <= 0)
      continue;

    if (ue_sched_ctl->pre_nb_available_rbs[CC_id]
        >= nb_rbs_required[CC_id][UE_id])
      continue;

    if (ue_sched_ctl->dl_pow_off[CC_id] == 0)
      continue;

    if ((i == N_RBG - 1) && ((N_RB_DL == 25) || (N_RB_DL == 50))) {
      // Allocating last, smaller RBG
      if (nb_rbs_remaining[CC_id][UE_id] >= min_rb_unit - 1) {
        rballoc_sub[CC_id][i] = 1;
        ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 1;

        nb_rbs_remaining[CC_id][UE_id] =
            nb_rbs_remaining[CC_id][UE_id] - min_rb_unit + 1;
        ue_sched_ctl->pre_nb_available_rbs[CC_id] =
            ue_sched_ctl->pre_nb_available_rbs[CC_id] + min_rb_unit - 1;
      }
    } else {
      // Allocating a standard-sized RBG
      if (nb_rbs_remaining[CC_id][UE_id] >= min_rb_unit) {
        rballoc_sub[CC_id][i] = 1;
        ue_sched_ctl->rballoc_sub_UE[CC_id][i] = 1;

        nb_rbs_remaining[CC_id][UE_id] =
            nb_rbs_remaining[CC_id][UE_id] - min_rb_unit;
        ue_sched_ctl->pre_nb_available_rbs[CC_id] =
            ue_sched_ctl->pre_nb_available_rbs[CC_id] + min_rb_unit;
      }
    }
  }
}

// This function assigns pre-available RBS to each UE in specified sub-bands before scheduling is done
void dlsch_scheduler_pre_processor_fairRR (module_id_t   Mod_id,
    frame_t       frameP,
    sub_frame_t   subframeP,
    int           N_RBG[MAX_NUM_CCs],
    int           *mbsfn_flag) {
  unsigned char rballoc_sub[MAX_NUM_CCs][N_RBG_MAX],harq_pid=0,Round=0;
  uint16_t                temp_total_rbs_count;
  unsigned char           temp_total_ue_count;
  unsigned char MIMO_mode_indicator[MAX_NUM_CCs][N_RBG_MAX];
  int                     UE_id, i;
  uint16_t                j;
  uint16_t                nb_rbs_required[MAX_NUM_CCs][MAX_MOBILES_PER_ENB];
  uint16_t                nb_rbs_required_remaining[MAX_NUM_CCs][MAX_MOBILES_PER_ENB];
  //  uint16_t                nb_rbs_required_remaining_1[MAX_NUM_CCs][NUMBER_OF_UE_MAX];
  uint16_t                average_rbs_per_user[MAX_NUM_CCs] = {0};
  rnti_t             rnti;
  int                min_rb_unit[MAX_NUM_CCs];
  //  uint16_t r1=0;
  uint8_t CC_id;
  UE_info_t *UE_info = &RC.mac[Mod_id]->UE_info;
  int N_RB_DL;
  UE_sched_ctrl_t *ue_sched_ctl;
  //  int rrc_status           = RRC_IDLE;
  COMMON_channels_t *cc;
#ifdef TM5
  int harq_pid1 = 0;
  int round1 = 0, round2 = 0;
  int UE_id2;
  uint16_t i1, i2, i3;
  rnti_t rnti1, rnti2;
  LTE_eNB_UE_stats *eNB_UE_stats1 = NULL;
  LTE_eNB_UE_stats *eNB_UE_stats2 = NULL;
  UE_sched_ctrl_t *ue_sched_ctl1, *ue_sched_ctl2;
#endif
  memset(rballoc_sub[0],0,(MAX_NUM_CCs)*(N_RBG_MAX)*sizeof(unsigned char));
  memset(min_rb_unit,0,sizeof(min_rb_unit));
  memset(MIMO_mode_indicator[0], 0, MAX_NUM_CCs*N_RBG_MAX*sizeof(unsigned char));

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    if (mbsfn_flag[CC_id] > 0)  // If this CC is allocated for MBSFN skip it here
      continue;

    min_rb_unit[CC_id] = get_min_rb_unit(Mod_id, CC_id);

    for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
      if (UE_info->active[i] != TRUE)
        continue;

      UE_id = i;
      // Initialize scheduling information for all active UEs
      dlsch_scheduler_pre_processor_reset_fairRR(
          Mod_id,
          frameP,
          subframeP,
          min_rb_unit,
          (uint16_t(*)[MAX_MOBILES_PER_ENB])nb_rbs_required,
          rballoc_sub,
          MIMO_mode_indicator);
    }
  }

#if (!defined(PRE_SCD_THREAD))
  // Store the DLSCH buffer for each logical channel and for PCCID (assume 0)
  store_dlsch_buffer(Mod_id, 0, frameP, subframeP);
  // Calculate the number of RBs required by each UE on the basis of logical channel's buffer
  assign_rbs_required_fairRR(Mod_id, frameP, subframeP, nb_rbs_required);
#else
  memcpy(nb_rbs_required, pre_nb_rbs_required[dlsch_ue_select_tbl_in_use], sizeof(uint16_t)*MAX_NUM_CCs*MAX_MOBILES_PER_ENB);
#endif
  dlsch_scheduler_pre_ue_select_fairRR(Mod_id,frameP,subframeP, mbsfn_flag,nb_rbs_required,dlsch_ue_select);

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    average_rbs_per_user[CC_id] = 0;
    cc = &RC.mac[Mod_id]->common_channels[CC_id];
    // Get total available RBS count and total UE count
    N_RB_DL = to_prb(cc->mib->message.dl_Bandwidth);
    temp_total_rbs_count = 0;

    for(uint8_t rbg_i = 0; rbg_i < N_RBG[CC_id]; rbg_i++ ) {
      if(rballoc_sub[CC_id][rbg_i] == 0) {
        if((rbg_i == N_RBG[CC_id] -1) &&
            ((N_RB_DL == 25) || (N_RB_DL == 50))) {
          temp_total_rbs_count += (min_rb_unit[CC_id] -1);
        } else {
          temp_total_rbs_count += min_rb_unit[CC_id];
        }
      }
    }

    temp_total_ue_count = dlsch_ue_select[CC_id].ue_num;

    for (i = 0; i < dlsch_ue_select[CC_id].ue_num; i++) {
      if(dlsch_ue_select[CC_id].list[i].ue_priority == SCH_DL_MSG2) {
        temp_total_ue_count--;
        continue;
      }

      if(dlsch_ue_select[CC_id].list[i].ue_priority == SCH_DL_MSG4) {
        temp_total_ue_count--;
        continue;
      }

      UE_id = dlsch_ue_select[CC_id].list[i].UE_id;
      nb_rbs_required[CC_id][UE_id] = dlsch_ue_select[CC_id].list[i].nb_rb;
      average_rbs_per_user[CC_id] = (uint16_t)round((double)temp_total_rbs_count/(double)temp_total_ue_count);

      if( average_rbs_per_user[CC_id] < min_rb_unit[CC_id] ) {
        temp_total_ue_count--;
        dlsch_ue_select[CC_id].ue_num--;
        i--;
        continue;
      }

      rnti = dlsch_ue_select[CC_id].list[i].rnti;
      ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
      harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config,frameP,subframeP);
      Round    = ue_sched_ctl->round[CC_id][harq_pid];

      //if (mac_eNB_get_rrc_status(Mod_id, rnti) < RRC_RECONFIGURED || round > 0) {
      if (mac_eNB_get_rrc_status(Mod_id, rnti) < RRC_RECONFIGURED || Round != 8) {  // FIXME
        nb_rbs_required_remaining[CC_id][UE_id] = dlsch_ue_select[CC_id].list[i].nb_rb;
      } else {
        nb_rbs_required_remaining[CC_id][UE_id] = cmin(average_rbs_per_user[CC_id], dlsch_ue_select[CC_id].list[i].nb_rb);
      }

      LOG_T(MAC,"calling dlsch_scheduler_pre_processor_allocate .. \n ");
      dlsch_scheduler_pre_processor_allocate_fairRR(
          Mod_id,
          UE_id,
          CC_id,
          N_RBG[CC_id],
          nb_rbs_required,
          nb_rbs_required_remaining,
          rballoc_sub);
      temp_total_rbs_count -= ue_sched_ctl->pre_nb_available_rbs[CC_id];
      temp_total_ue_count--;

      if (ue_sched_ctl->pre_nb_available_rbs[CC_id] == 0) {
        dlsch_ue_select[CC_id].ue_num = i;
        break;
      }

      if (temp_total_rbs_count == 0) {
        dlsch_ue_select[CC_id].ue_num = i+1;
        break;
      }

      LOG_D(MAC,
            "DLSCH UE Select: frame %d subframe %d pre_nb_available_rbs %d(i %d UE_id %d nb_rbs_required %d nb_rbs_required_remaining %d average_rbs_per_user %d (temp_total rbs_count %d ue_num %d) available_prbs %d)\n",
            frameP,subframeP,ue_sched_ctl->pre_nb_available_rbs[CC_id],i,UE_id,nb_rbs_required[CC_id][UE_id],nb_rbs_required_remaining[CC_id][UE_id],
            average_rbs_per_user[CC_id],temp_total_rbs_count,temp_total_ue_count,RC.mac[Mod_id]->eNB_stats[CC_id].available_prbs);
#ifdef TM5
      // TODO: data channel TM5: to be re-visited
#endif
    }
  }

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    for (i = 0; i < dlsch_ue_select[CC_id].ue_num; i++) {
      if(dlsch_ue_select[CC_id].list[i].ue_priority == SCH_DL_MSG2) {
        continue;
      }

      if(dlsch_ue_select[CC_id].list[i].ue_priority == SCH_DL_MSG4) {
        continue;
      }

      UE_id = dlsch_ue_select[CC_id].list[i].UE_id;
      ue_sched_ctl = &RC.mac[Mod_id]->UE_info.UE_sched_ctrl[UE_id];
      //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].dl_pow_off = dl_pow_off[UE_id];

      if (ue_sched_ctl->pre_nb_available_rbs[CC_id] > 0) {
        LOG_D(MAC,
              "******************DL Scheduling Information for UE%d ************************\n",
              UE_id);
        LOG_D(MAC, "dl power offset UE%d = %d \n", UE_id,
              ue_sched_ctl->dl_pow_off[CC_id]);
        LOG_D(MAC,
              "***********RB Alloc for every subband for UE%d ***********\n",
              UE_id);

        for (j = 0; j < N_RBG[CC_id]; j++) {
          //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].rballoc_sub[i] = rballoc_sub_UE[CC_id][UE_id][i];
          LOG_D(MAC, "RB Alloc for UE%d and Subband%d = %d\n",
                UE_id, j,
                ue_sched_ctl->rballoc_sub_UE[CC_id][j]);
        }

        //PHY_vars_eNB_g[Mod_id]->mu_mimo_mode[UE_id].pre_nb_available_rbs = pre_nb_available_rbs[CC_id][UE_id];
        LOG_D(MAC, "Total RBs allocated for UE%d = %d\n", UE_id,
              ue_sched_ctl->pre_nb_available_rbs[CC_id]);
      }
    }
  }
}


//------------------------------------------------------------------------------
void
schedule_ue_spec_fairRR(module_id_t module_idP,
                        frame_t frameP, sub_frame_t subframeP, int *mbsfn_flag)
//------------------------------------------------------------------------------
{
  uint8_t CC_id;
  int UE_id;
  //    unsigned char aggregation;
  mac_rlc_status_resp_t rlc_status;
  unsigned char header_len_dcch = 0, header_len_dcch_tmp = 0;
  unsigned char header_len_dtch = 0, header_len_dtch_tmp = 0, header_len_dtch_last = 0;
  unsigned char ta_len = 0;
  unsigned char sdu_lcids[NB_RB_MAX], lcid, offset, num_sdus = 0;
  uint16_t nb_rb, nb_rb_temp, nb_available_rb;
  uint16_t TBS, j, sdu_lengths[NB_RB_MAX], padding = 0, post_padding = 0;
  rnti_t rnti = 0;
  unsigned char dlsch_buffer[MAX_DLSCH_PAYLOAD_BYTES];
  unsigned char round = 0;
  unsigned char harq_pid = 0;
  eNB_UE_STATS *eNB_UE_stats = NULL;
  uint16_t sdu_length_total = 0;
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  COMMON_channels_t *cc = eNB->common_channels;
  UE_info_t *UE_info = &eNB->UE_info;
  // int continue_flag = 0;
  int32_t snr, target_snr;
  int32_t tpc = 1;
  UE_sched_ctrl_t *ue_sched_ctl;
  int mcs;
  int i;
  int min_rb_unit[MAX_NUM_CCs];
  int N_RB_DL[MAX_NUM_CCs];
  int total_nb_available_rb[MAX_NUM_CCs];
  int N_RBG[MAX_NUM_CCs];
  nfapi_dl_config_request_body_t *dl_req;
  nfapi_dl_config_request_pdu_t *dl_config_pdu;
  int tdd_sfa;
  int ta_update;
#ifdef DEBUG_eNB_SCHEDULER
  int k;
#endif

  if(is_pmch_subframe(frameP,subframeP,&RC.eNB[module_idP][0]->frame_parms)){
       //LOG_E(MAC,"fairRR Frame[%d] SF:%d This SF should not be allocated\n",frameP,subframeP);
       return;
  }

  start_meas(&eNB->schedule_dlsch);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH, VCD_FUNCTION_IN);

  // for TDD: check that we have to act here, otherwise return
  if (cc[0].tdd_Config) {
    tdd_sfa = cc[0].tdd_Config->subframeAssignment;

    switch (subframeP) {
      case 0:
        // always continue
        break;

      case 1:
        return;
        break;

      case 2:
        return;
        break;

      case 3:
        if ((tdd_sfa != 2) && (tdd_sfa != 5))
          return;

        break;

      case 4:
        if ((tdd_sfa != 1) && (tdd_sfa != 2) && (tdd_sfa != 4)
            && (tdd_sfa != 5))
          return;

        break;

      case 5:
        break;

      case 6:
      case 7:
        if ((tdd_sfa != 3)&& (tdd_sfa != 4) && (tdd_sfa != 5))
          return;

        break;

      case 8:
        if ((tdd_sfa != 2) && (tdd_sfa != 3) && (tdd_sfa != 4)
            && (tdd_sfa != 5))
          return;

        break;

      case 9:
        if (tdd_sfa == 0)
          return;

        break;
    }
  }

  //weight = get_ue_weight(module_idP,UE_id);
  //    aggregation = 2;
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    N_RB_DL[CC_id] = to_prb(cc[CC_id].mib->message.dl_Bandwidth);
    min_rb_unit[CC_id] = get_min_rb_unit(module_idP, CC_id);
    // get number of PRBs less those used by common channels
    total_nb_available_rb[CC_id] = N_RB_DL[CC_id];

    for (i = 0; i < N_RB_DL[CC_id]; i++)
      if (cc[CC_id].vrb_map[i] != 0)
        total_nb_available_rb[CC_id]--;

    N_RBG[CC_id] = to_rbg(cc[CC_id].mib->message.dl_Bandwidth);
    // store the global enb stats:
    eNB->eNB_stats[CC_id].num_dlactive_UEs = UE_info->num_UEs;
    eNB->eNB_stats[CC_id].available_prbs =
      total_nb_available_rb[CC_id];
    eNB->eNB_stats[CC_id].total_available_prbs +=
      total_nb_available_rb[CC_id];
    eNB->eNB_stats[CC_id].dlsch_bytes_tx = 0;
    eNB->eNB_stats[CC_id].dlsch_pdus_tx = 0;
  }

  /// CALLING Pre_Processor for downlink scheduling (Returns estimation of RBs required by each UE and the allocation on sub-band)
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR,VCD_FUNCTION_IN);
  start_meas(&eNB->schedule_dlsch_preprocessor);
  dlsch_scheduler_pre_processor_fairRR(module_idP,
                                       frameP,
                                       subframeP,
                                       N_RBG,
                                       mbsfn_flag);
  stop_meas(&eNB->schedule_dlsch_preprocessor);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR,VCD_FUNCTION_OUT);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "doing schedule_ue_spec for CC_id %d\n",CC_id);
    dl_req        = &eNB->DL_req[CC_id].dl_config_request_body;

    if (mbsfn_flag[CC_id]>0)
      continue;

    for (i = 0; i < dlsch_ue_select[CC_id].ue_num; i++) {
      if(dlsch_ue_select[CC_id].list[i].ue_priority == SCH_DL_MSG2) {
        continue;
      }

      if(dlsch_ue_select[CC_id].list[i].ue_priority == SCH_DL_MSG4) {
        continue;
      }

      UE_id = dlsch_ue_select[CC_id].list[i].UE_id;
      rnti = UE_RNTI(module_idP,UE_id);

      if (rnti==NOT_A_RNTI) {
        LOG_E(MAC,"Cannot find rnti for UE_id %d (num_UEs %d)\n",UE_id,UE_info->num_UEs);
        continue;
      }

      eNB_UE_stats = &UE_info->eNB_UE_stats[CC_id][UE_id];
      ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];

      /*
            switch(get_tmode(module_idP,CC_id,UE_id)){
            case 1:
            case 2:
            case 7:
              aggregation = get_aggregation(get_bw_index(module_idP,CC_id),
                                            ue_sched_ctl->dl_cqi[CC_id],
                                            format1);
              break;
            case 3:
              aggregation = get_aggregation(get_bw_index(module_idP,CC_id),
                                            ue_sched_ctl->dl_cqi[CC_id],
                                            format2A);
              break;
            default:
              LOG_W(MAC,"Unsupported transmission mode %d\n", get_tmode(module_idP,CC_id,UE_id));
              aggregation = 2;
              break;
            }
      */
      if (cc[CC_id].tdd_Config != NULL) { //TDD
        set_ue_dai (subframeP,
                    UE_id,
                    CC_id,
                    cc[CC_id].tdd_Config->subframeAssignment,
                    UE_info);
        // update UL DAI after DLSCH scheduling
        set_ul_DAI(module_idP,UE_id,CC_id,frameP,subframeP);
      }

      nb_available_rb = ue_sched_ctl->pre_nb_available_rbs[CC_id];
      harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config,frameP,subframeP);
      round = ue_sched_ctl->round[CC_id][harq_pid];
      UE_info->eNB_UE_stats[CC_id][UE_id].crnti = rnti;
      UE_info->eNB_UE_stats[CC_id][UE_id].rrc_status = mac_eNB_get_rrc_status(module_idP, rnti);
      UE_info->eNB_UE_stats[CC_id][UE_id].harq_pid = harq_pid;
      UE_info->eNB_UE_stats[CC_id][UE_id].harq_round = round;

      if (UE_info->eNB_UE_stats[CC_id][UE_id].rrc_status < RRC_RECONFIGURED) {
        UE_info->UE_sched_ctrl[UE_id].uplane_inactivity_timer = 0;
      }

      if (UE_info->eNB_UE_stats[CC_id][UE_id].rrc_status < RRC_CONNECTED)
        continue;

      sdu_length_total = 0;
      num_sdus = 0;

      /*
         DevCheck(((eNB_UE_stats->dl_cqi < MIN_CQI_VALUE) || (eNB_UE_stats->dl_cqi > MAX_CQI_VALUE)),
         eNB_UE_stats->dl_cqi, MIN_CQI_VALUE, MAX_CQI_VALUE);
       */
      if (NFAPI_MODE != NFAPI_MONOLITHIC) {
        eNB_UE_stats->dlsch_mcs1 = cqi_to_mcs[ue_sched_ctl->dl_cqi[CC_id]];
      } else {
        eNB_UE_stats->dlsch_mcs1 = cqi_to_mcs[ue_sched_ctl->dl_cqi[CC_id]];
      }

      //eNB_UE_stats->dlsch_mcs1 = cmin(eNB_UE_stats->dlsch_mcs1, openair_daq_vars.target_ue_dl_mcs);

      // store stats
      //UE_info->eNB_UE_stats[CC_id][UE_id].dl_cqi= eNB_UE_stats->dl_cqi;

      // initializing the rb allocation indicator for each UE
      for (j = 0; j < N_RBG[CC_id]; j++) {
        UE_info->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = 0;
      }

      LOG_D(MAC,
            "[eNB %d] Frame %d: Scheduling UE %d on CC_id %d (rnti %x, harq_pid %d, round %d, rb %d, cqi %d, mcs %d, rrc %d)\n",
            module_idP, frameP, UE_id, CC_id, rnti, harq_pid, round,
            nb_available_rb, ue_sched_ctl->dl_cqi[CC_id],
            eNB_UE_stats->dlsch_mcs1,
            UE_info->eNB_UE_stats[CC_id][UE_id].rrc_status);

      /* process retransmission  */

      if (round != 8) {
        // get freq_allocation
        nb_rb = UE_info->UE_template[CC_id][UE_id].nb_rb[harq_pid];
        TBS =
          get_TBS_DL(UE_info->UE_template[CC_id][UE_id].oldmcs1[harq_pid],
                     nb_rb);

        if (nb_rb <= nb_available_rb) {
          if (cc[CC_id].tdd_Config != NULL) {
            UE_info->UE_template[CC_id][UE_id].DAI++;
            update_ul_dci(module_idP, CC_id, rnti,
                          UE_info->UE_template[CC_id][UE_id].DAI,subframeP);
            LOG_D(MAC,
                  "DAI update: CC_id %d subframeP %d: UE %d, DAI %d\n",
                  CC_id, subframeP, UE_id,
                  UE_info->UE_template[CC_id][UE_id].DAI);
          }

          if (nb_rb == ue_sched_ctl->pre_nb_available_rbs[CC_id]) {
            for (j = 0; j < N_RBG[CC_id]; j++) {  // for indicating the rballoc for each sub-band
              UE_info->UE_template[CC_id][UE_id].
              rballoc_subband[harq_pid][j] =
                ue_sched_ctl->rballoc_sub_UE[CC_id][j];
            }
          } else {
            nb_rb_temp = nb_rb;
            j = 0;

            while ((nb_rb_temp > 0) && (j < N_RBG[CC_id])) {
              if (ue_sched_ctl->rballoc_sub_UE[CC_id][j] ==
                  1) {
                if (UE_info->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j])
                  LOG_W(MAC, "WARN: rballoc_subband not free for retrans?\n");

                UE_info->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];

                if ((j == N_RBG[CC_id] - 1) &&
                    ((N_RB_DL[CC_id] == 25) ||
                     (N_RB_DL[CC_id] == 50))) {
                  nb_rb_temp =
                    nb_rb_temp - min_rb_unit[CC_id] +
                    1;
                } else {
                  nb_rb_temp =
                    nb_rb_temp - min_rb_unit[CC_id];
                }
              }

              j = j + 1;
            }
          }

          nb_available_rb -= nb_rb;
          /*
             eNB->mu_mimo_mode[UE_id].pre_nb_available_rbs = nb_rb;
             eNB->mu_mimo_mode[UE_id].dl_pow_off = ue_sched_ctl->dl_pow_off[CC_id];

             for(j=0; j<N_RBG[CC_id]; j++) {
             eNB->mu_mimo_mode[UE_id].rballoc_sub[j] = UE_info->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j];
             }
           */

          switch (get_tmode(module_idP, CC_id, UE_id)) {
            case 1:
            case 2:
            case 7:
            default:
              LOG_D(MAC,"retransmission DL_REQ: rnti:%x\n",rnti);
              dl_config_pdu =
                &dl_req->dl_config_pdu_list[dl_req->
                                            number_pdu];
              memset((void *) dl_config_pdu, 0,
                     sizeof(nfapi_dl_config_request_pdu_t));
              dl_config_pdu->pdu_type =
                NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
              dl_config_pdu->pdu_size =
                (uint8_t) (2 +
                           sizeof(nfapi_dl_config_dci_dl_pdu));
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.
              dci_format = NFAPI_DL_DCI_FORMAT_1;
              dl_config_pdu->dci_dl_pdu.
              dci_dl_pdu_rel8.aggregation_level =
                get_aggregation(get_bw_index
                                (module_idP, CC_id),
                                ue_sched_ctl->dl_cqi[CC_id],
                                format1);
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti =
                rnti;
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type = 1;  // CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power = 6000;  // equal to RS power
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.
              harq_process = harq_pid;
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc = 1;  // dont adjust power when retransmitting
              dl_config_pdu->dci_dl_pdu.
              dci_dl_pdu_rel8.new_data_indicator_1 = UE_info->UE_template[CC_id][UE_id].oldNDI[harq_pid];
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1 = UE_info->UE_template[CC_id][UE_id].oldmcs1[harq_pid];
              dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1 = round & 3;

              if (cc[CC_id].tdd_Config != NULL) { //TDD
                dl_config_pdu->dci_dl_pdu.
                dci_dl_pdu_rel8.downlink_assignment_index = (UE_info->UE_template[CC_id][UE_id].DAI - 1) & 3;
                LOG_D(MAC,
                      "[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, dai %d, mcs %d\n",
                      module_idP, CC_id, harq_pid, round,
                      UE_info->UE_template[CC_id][UE_id].DAI - 1,
                      UE_info-> UE_template[CC_id][UE_id].oldmcs1[harq_pid]);
              } else {
                LOG_D(MAC,
                      "[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, mcs %d\n",
                      module_idP, CC_id, harq_pid, round,
                      UE_info->UE_template[CC_id][UE_id].oldmcs1[harq_pid]);
              }

              if (!CCE_allocation_infeasible
                  (module_idP, CC_id, 1, subframeP,
                   dl_config_pdu->dci_dl_pdu.
                   dci_dl_pdu_rel8.aggregation_level, rnti)) {
                dl_req->number_dci++;
                dl_req->number_pdu++;
                dl_req->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
                eNB->DL_req[CC_id].sfn_sf = frameP<<4 | subframeP;
                eNB->DL_req[CC_id].header.message_id = NFAPI_DL_CONFIG_REQUEST;
                fill_nfapi_dlsch_config(&dl_req->dl_config_pdu_list[dl_req->number_pdu],
                                        TBS,
                                        -1
                                        /* retransmission, no pdu_index */
                                        , rnti, 0,  // type 0 allocation from 7.1.6 in 36.213
                                        0,  // virtual_resource_block_assignment_flag, unused here
                                        0,  // resource_block_coding, to be filled in later
                                        getQm(UE_info->UE_template[CC_id][UE_id].oldmcs1[harq_pid]), round & 3, // redundancy version
                                        1,  // transport blocks
                                        0,  // transport block to codeword swap flag
                                        cc[CC_id].p_eNB == 1 ? 0 : 1, // transmission_scheme
                                        1,  // number of layers
                                        1,  // number of subbands
                                        //                      uint8_t codebook_index,
                                        4,  // UE category capacity
                                        UE_info->UE_template[CC_id][UE_id].physicalConfigDedicated->pdsch_ConfigDedicated->p_a, 0,  // delta_power_offset for TM5
                                        0,  // ngap
                                        0,  // nprb
                                        cc[CC_id].p_eNB == 1 ? 1 : 2, // transmission mode
                                        0,  //number of PRBs treated as one subband, not used here
                                        0 // number of beamforming vectors, not used here
                                       );
                dl_req->number_pdu++;
                LOG_D(MAC,
                      "Filled NFAPI configuration for DCI/DLSCH %d, retransmission round %d\n",
                      eNB->pdu_index[CC_id], round);
                program_dlsch_acknak(module_idP, CC_id, UE_id,
                                     frameP, subframeP,
                                     dl_config_pdu->
                                     dci_dl_pdu.dci_dl_pdu_rel8.
                                     cce_idx);
                // No TX request for retransmission (check if null request for FAPI)
              } else {
                LOG_W(MAC,
                      "Frame %d, Subframe %d: Dropping DLSCH allocation for UE %d\%x, infeasible CCE allocation\n",
                      frameP, subframeP, UE_id, rnti);
              }
          }

          add_ue_dlsch_info(module_idP,
                            CC_id,
                            UE_id,
                            subframeP,
                            S_DL_SCHEDULED,
                            rnti);
          UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_rounds[round]++;
          UE_info->eNB_UE_stats[CC_id][UE_id].num_retransmission += 1;
          UE_info->eNB_UE_stats[CC_id][UE_id].rbs_used_retx = nb_rb;
          UE_info->eNB_UE_stats[CC_id][UE_id].total_rbs_used_retx += nb_rb;
          UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1 = eNB_UE_stats->dlsch_mcs1;
          UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs2 = eNB_UE_stats->dlsch_mcs1;
        } else {
          LOG_D(MAC,
                "[eNB %d] Frame %d CC_id %d : don't schedule UE %d, its retransmission takes more resources than we have\n",
                module_idP, frameP, CC_id, UE_id);
        }
      } else {    /* This is a potentially new SDU opportunity */
        rlc_status.bytes_in_buffer = 0;
        // Now check RLC information to compute number of required RBs
        // get maximum TBS size for RLC request
        TBS =
          get_TBS_DL(eNB_UE_stats->dlsch_mcs1, nb_available_rb);
        // check first for RLC data on DCCH
        // add the length for  all the control elements (timing adv, drx, etc) : header + payload

        if (ue_sched_ctl->ta_timer == 0) {
          ta_update = ue_sched_ctl->ta_update;

          /* if we send TA then set timer to not send it for a while */
          if (ta_update != 31)
            ue_sched_ctl->ta_timer = 20;

          /* reset ta_update */
          ue_sched_ctl->ta_update = 31;
          ue_sched_ctl->ta_update_f = 31.0;
        } else {
          ta_update = 31;
        }

        ta_len = (ta_update != 31) ? 2 : 0;
        header_len_dcch = 2;  // 2 bytes DCCH SDU subheader

        if (TBS - ta_len - header_len_dcch > 0) {
          rlc_status = mac_rlc_status_ind(module_idP, rnti, module_idP, frameP, subframeP, ENB_FLAG_YES, MBMS_FLAG_NO, DCCH, 0, 0); // transport block set size
          sdu_lengths[0] = 0;

          if (rlc_status.bytes_in_buffer > 0) { // There is DCCH to transmit
            LOG_D(MAC,
                  "[eNB %d] SFN/SF %d.%d, DL-DCCH->DLSCH CC_id %d, Requesting %d bytes from RLC (RRC message)\n",
                  module_idP, frameP, subframeP, CC_id,
                  TBS - header_len_dcch);
            sdu_lengths[0] = mac_rlc_data_req(module_idP, rnti, module_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_NO, DCCH,
                                              TBS - ta_len - header_len_dcch,
                                              (char *) &dlsch_buffer[0],0, 0
                                             );

            if((rrc_release_info.num_UEs > 0) && (rlc_am_mui.rrc_mui_num > 0)) {
              while(pthread_mutex_trylock(&rrc_release_freelist)) {
                /* spin... */
              }

              uint16_t release_total = 0;

              for(uint16_t release_num = 0; release_num < NUMBER_OF_UE_MAX; release_num++) {
                if(rrc_release_info.RRC_release_ctrl[release_num].flag > 0) {
                  release_total++;
                } else {
                  continue;
                }

                if(rrc_release_info.RRC_release_ctrl[release_num].flag == 1) {
                  if(rrc_release_info.RRC_release_ctrl[release_num].rnti == rnti) {
                    for(uint16_t mui_num = 0; mui_num < rlc_am_mui.rrc_mui_num; mui_num++) {
                      if(rrc_release_info.RRC_release_ctrl[release_num].rrc_eNB_mui == rlc_am_mui.rrc_mui[mui_num]) {
                        rrc_release_info.RRC_release_ctrl[release_num].flag = 3;
                        LOG_D(MAC,"DLSCH Release send:index %d rnti %x mui %d mui_num %d flag 1->3\n",release_num,rnti,rlc_am_mui.rrc_mui[mui_num],mui_num);
                        break;
                      }
                    }
                  }
                }

                if(rrc_release_info.RRC_release_ctrl[release_num].flag == 2) {
                  if(rrc_release_info.RRC_release_ctrl[release_num].rnti == rnti) {
                    for(uint16_t mui_num = 0; mui_num < rlc_am_mui.rrc_mui_num; mui_num++) {
                      if(rrc_release_info.RRC_release_ctrl[release_num].rrc_eNB_mui == rlc_am_mui.rrc_mui[mui_num]) {
                        rrc_release_info.RRC_release_ctrl[release_num].flag = 4;
                        LOG_D(MAC,"DLSCH Release send:index %d rnti %x mui %d mui_num %d flag 2->4\n",release_num,rnti,rlc_am_mui.rrc_mui[mui_num],mui_num);
                        break;
                      }
                    }
                  }
                }

                if(release_total >= rrc_release_info.num_UEs)
                  break;
              }

              pthread_mutex_unlock(&rrc_release_freelist);
            }

            RA_t *ra = &eNB->common_channels[CC_id].ra[0];

            for (uint8_t ra_ii = 0; ra_ii < NB_RA_PROC_MAX; ra_ii++) {
              if((ra[ra_ii].rnti == rnti) && (ra[ra_ii].state == MSGCRNTI)) {
                for(uint16_t mui_num = 0; mui_num < rlc_am_mui.rrc_mui_num; mui_num++) {
                  if(ra[ra_ii].crnti_rrc_mui == rlc_am_mui.rrc_mui[mui_num]) {
                    ra[ra_ii].crnti_harq_pid = harq_pid;
                    ra[ra_ii].state = MSGCRNTI_ACK;
                    break;
                  }
                }
              }
            }

            T(T_ENB_MAC_UE_DL_SDU, T_INT(module_idP),
              T_INT(CC_id), T_INT(rnti), T_INT(frameP),
              T_INT(subframeP), T_INT(harq_pid), T_INT(DCCH),
              T_INT(sdu_lengths[0]));
            LOG_D(MAC,
                  "[eNB %d][DCCH] CC_id %d frame %d subframe %d UE_id %d/%x Got %d bytes bytes_in_buffer %d from release_num %d\n",
                  module_idP, CC_id, frameP, subframeP, UE_id, rnti, sdu_lengths[0],rlc_status.bytes_in_buffer,rrc_release_info.num_UEs);
            sdu_length_total = sdu_lengths[0];
            sdu_lcids[0] = DCCH;
            UE_info->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[DCCH] += 1;
            UE_info->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[DCCH] += sdu_lengths[0];
            num_sdus = 1;
#ifdef DEBUG_eNB_SCHEDULER
            LOG_T(MAC,
                  "[eNB %d][DCCH] CC_id %d Got %d bytes :",
                  module_idP, CC_id, sdu_lengths[0]);

            for (k = 0; k < sdu_lengths[0]; k++) {
              LOG_T(MAC, "%x ", dlsch_buffer[k]);
            }

            LOG_T(MAC, "\n");
#endif
          } else {
            header_len_dcch = 0;
            sdu_length_total = 0;
          }
        }

        // check for DCCH1 and update header information (assume 2 byte sub-header)
        if (TBS - ta_len - header_len_dcch - sdu_length_total > 0) {
          rlc_status = mac_rlc_status_ind(module_idP, rnti, module_idP, frameP, subframeP, ENB_FLAG_YES, MBMS_FLAG_NO, DCCH + 1, 0, 0
                                         ); // transport block set size less allocations for timing advance and
          // DCCH SDU
          sdu_lengths[num_sdus] = 0;

          if (rlc_status.bytes_in_buffer > 0) {
            LOG_D(MAC,
                  "[eNB %d], Frame %d, DCCH1->DLSCH, CC_id %d, Requesting %d bytes from RLC (RRC message)\n",
                  module_idP, frameP, CC_id,
                  TBS - header_len_dcch - sdu_length_total);
            sdu_lengths[num_sdus] += mac_rlc_data_req(module_idP, rnti, module_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_NO, DCCH + 1,
                                                      TBS - ta_len - header_len_dcch - sdu_length_total,
                                     (char *) &dlsch_buffer[sdu_length_total],0, 0
                                                     );
            T(T_ENB_MAC_UE_DL_SDU, T_INT(module_idP),
              T_INT(CC_id), T_INT(rnti), T_INT(frameP),
              T_INT(subframeP), T_INT(harq_pid),
              T_INT(DCCH + 1), T_INT(sdu_lengths[num_sdus]));
            sdu_lcids[num_sdus] = DCCH1;
            sdu_length_total += sdu_lengths[num_sdus];
            header_len_dcch += 2;
            UE_info->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[DCCH1] += 1;
            UE_info->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[DCCH1] += sdu_lengths[num_sdus];
            num_sdus++;
#ifdef DEBUG_eNB_SCHEDULER
            LOG_T(MAC,
                  "[eNB %d][DCCH1] CC_id %d Got %d bytes :",
                  module_idP, CC_id, sdu_lengths[num_sdus]);

            for (k = 0; k < sdu_lengths[num_sdus]; k++) {
              LOG_T(MAC, "%x ", dlsch_buffer[k]);
            }

            LOG_T(MAC, "\n");
#endif
          }
        }

        // assume the max dtch header size, and adjust it later
        header_len_dtch = 0;
        header_len_dtch_last = 0; // the header length of the last mac sdu

        // lcid has to be sorted before the actual allocation (similar struct as ue_list).
        /* TODO limited lcid for performance */
        for (lcid = DTCH; lcid >= DTCH; lcid--) {
          // TBD: check if the lcid is active
          header_len_dtch += 3;
          header_len_dtch_last = 3;
          LOG_D(MAC, "[eNB %d], Frame %d, DTCH%d->DLSCH, Checking RLC status (tbs %d, len %d)\n",
                module_idP,
                frameP,
                lcid,
                TBS,
                TBS - ta_len - header_len_dcch - sdu_length_total - header_len_dtch);

          if (TBS - ta_len - header_len_dcch - sdu_length_total - header_len_dtch > 0) {  // NN: > 2 ?
            rlc_status = mac_rlc_status_ind(module_idP,
                                            rnti,
                                            module_idP,
                                            frameP,
                                            subframeP,
                                            ENB_FLAG_YES,
                                            MBMS_FLAG_NO,
                                            lcid,
                                            0, 0
                                           );

            if (rlc_status.bytes_in_buffer > 0) {
              LOG_D(MAC,"[eNB %d][USER-PLANE DEFAULT DRB] Frame %d : DTCH->DLSCH, Requesting %d bytes from RLC (lcid %d total hdr len %d)\n",
                    module_idP,
                    frameP,
                    TBS - header_len_dcch - sdu_length_total - header_len_dtch,
                    lcid,
                    header_len_dtch);
              sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP,
                                      rnti,
                                      module_idP,
                                      frameP,
                                      ENB_FLAG_YES,
                                      MBMS_FLAG_NO,
                                      lcid,
                                      TBS - ta_len - header_len_dcch - sdu_length_total - header_len_dtch,
                                      (char *)&dlsch_buffer[sdu_length_total], 0, 0
                                                      );
              T(T_ENB_MAC_UE_DL_SDU,
                T_INT(module_idP),
                T_INT(CC_id),
                T_INT(rnti),
                T_INT(frameP),
                T_INT(subframeP),
                T_INT(harq_pid),
                T_INT(lcid),
                T_INT(sdu_lengths[num_sdus]));
              LOG_D(MAC, "[eNB %d][USER-PLANE DEFAULT DRB] Got %d bytes for DTCH %d \n",
                    module_idP,
                    sdu_lengths[num_sdus],
                    lcid);
              sdu_lcids[num_sdus] = lcid;
              sdu_length_total += sdu_lengths[num_sdus];
              UE_info->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[lcid] += 1;
              UE_info->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[lcid] += sdu_lengths[num_sdus];

              if (sdu_lengths[num_sdus] < 128) {
                header_len_dtch--;
                header_len_dtch_last--;
              }

              num_sdus++;
              UE_info->UE_sched_ctrl[UE_id].uplane_inactivity_timer = 0;
            } else { // no data for this LCID
              header_len_dtch -= 3;
            }
          } else {  // no TBS left
            header_len_dtch -= 3;
            break;
          }
        }

        if (header_len_dtch == 0)
          header_len_dtch_last = 0;

        // there is at least one SDU
        // if (num_sdus > 0 ){
        if ((sdu_length_total + header_len_dcch +
             header_len_dtch) > 0) {
          // Now compute number of required RBs for total sdu length
          // Assume RAH format 2
          // adjust  header lengths
          header_len_dcch_tmp = header_len_dcch;
          header_len_dtch_tmp = header_len_dtch;

          if (header_len_dtch == 0) {
            header_len_dcch = (header_len_dcch > 0) ? 1 : 0;  //header_len_dcch;  // remove length field
          } else {
            header_len_dtch_last -= 1;  // now use it to find how many bytes has to be removed for the last MAC SDU
            header_len_dtch = (header_len_dtch > 0) ? header_len_dtch - header_len_dtch_last : header_len_dtch; // remove length field for the last SDU
          }

          mcs = eNB_UE_stats->dlsch_mcs1;
          nb_rb = min_rb_unit[CC_id];
          TBS = get_TBS_DL(mcs, nb_rb);

          while (TBS <
                 (sdu_length_total + header_len_dcch +
                  header_len_dtch + ta_len)) {
            nb_rb += min_rb_unit[CC_id];  //

            if (nb_rb > nb_available_rb) {  // if we've gone beyond the maximum number of RBs
              // (can happen if N_RB_DL is odd)
              TBS =
                get_TBS_DL(eNB_UE_stats->dlsch_mcs1,
                           nb_available_rb);
              nb_rb = nb_available_rb;
              break;
            }

            TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, nb_rb);
          }

          if (nb_rb == ue_sched_ctl->pre_nb_available_rbs[CC_id]) {
            for (j = 0; j < N_RBG[CC_id]; j++) {  // for indicating the rballoc for each sub-band
              UE_info->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
            }
          } else {
            nb_rb_temp = nb_rb;
            j = 0;

            while ((nb_rb_temp > 0) && (j < N_RBG[CC_id])) {
              if (ue_sched_ctl->rballoc_sub_UE[CC_id][j] ==
                  1) {
                UE_info->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];

                if ((j == N_RBG[CC_id] - 1) &&
                    ((N_RB_DL[CC_id] == 25) ||
                     (N_RB_DL[CC_id] == 50))) {
                  nb_rb_temp =
                    nb_rb_temp - min_rb_unit[CC_id] +
                    1;
                } else {
                  nb_rb_temp =
                    nb_rb_temp - min_rb_unit[CC_id];
                }
              }

              j = j + 1;
            }
          }

          // decrease mcs until TBS falls below required length
          while ((TBS >
                  (sdu_length_total + header_len_dcch +
                   header_len_dtch + ta_len)) && (mcs > 0)) {
            mcs--;
            TBS = get_TBS_DL(mcs, nb_rb);
          }

          // if we have decreased too much or we don't have enough RBs, increase MCS
          while ((TBS <
                  (sdu_length_total + header_len_dcch +
                   header_len_dtch + ta_len))
                 && (((ue_sched_ctl->dl_pow_off[CC_id] > 0)
                      && (mcs < 28))
                     || ((ue_sched_ctl->dl_pow_off[CC_id] == 0)
                         && (mcs <= 15)))) {
            mcs++;
            TBS = get_TBS_DL(mcs, nb_rb);
          }

          LOG_D(MAC,
                "dlsch_mcs before and after the rate matching = (%d, %d)\n",
                eNB_UE_stats->dlsch_mcs1, mcs);
#ifdef DEBUG_eNB_SCHEDULER
          LOG_D(MAC,
                "[eNB %d] CC_id %d Generated DLSCH header (mcs %d, TBS %d, nb_rb %d)\n",
                module_idP, CC_id, mcs, TBS, nb_rb);
          // msg("[MAC][eNB ] Reminder of DLSCH with random data %d %d %d %d \n",
          //  TBS, sdu_length_total, offset, TBS-sdu_length_total-offset);
#endif

          if ((TBS - header_len_dcch - header_len_dtch -
               sdu_length_total - ta_len) <= 2) {
            padding =
              (TBS - header_len_dcch - header_len_dtch -
               sdu_length_total - ta_len);
            post_padding = 0;
          } else {
            padding = 0;

            // adjust the header len
            if (header_len_dtch == 0) {
              header_len_dcch = header_len_dcch_tmp;
            } else {  //if (( header_len_dcch==0)&&((header_len_dtch==1)||(header_len_dtch==2)))
              header_len_dtch = header_len_dtch_tmp;
            }

            post_padding = TBS - sdu_length_total - header_len_dcch - header_len_dtch - ta_len; // 1 is for the postpadding header
          }

#ifdef PHY_TX_THREAD
          struct timespec time_req, time_rem;
          time_req.tv_sec = 0;
          time_req.tv_nsec = 10000;

          while((!oai_exit)&&(phy_tx_txdataF_end == 0)) {
            nanosleep(&time_req,&time_rem);
            continue;
          }

#endif
          offset = generate_dlsch_header((unsigned char *) UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0], num_sdus,  //num_sdus
                                         sdu_lengths, //
                                         sdu_lcids, 255,  // no drx
                                         ta_update, // timing advance
                                         NULL,  // contention res id
                                         padding, post_padding);

          //#ifdef DEBUG_eNB_SCHEDULER
          if (ta_update != 31) {
            LOG_D(MAC,
                  "[eNB %d][DLSCH] Frame %d Generate header for UE_id %d on CC_id %d: sdu_length_total %d, num_sdus %d, sdu_lengths[0] %d, sdu_lcids[0] %d => payload offset %d,timing advance value : %d, padding %d,post_padding %d,(mcs %d, TBS %d, nb_rb %d),header_dcch %d, header_dtch %d\n",
                  module_idP, frameP, UE_id, CC_id,
                  sdu_length_total, num_sdus, sdu_lengths[0],
                  sdu_lcids[0], offset, ta_update, padding,
                  post_padding, mcs, TBS, nb_rb,
                  header_len_dcch, header_len_dtch);
          }

          //#endif
#ifdef DEBUG_eNB_SCHEDULER
          LOG_T(MAC, "[eNB %d] First 16 bytes of DLSCH : \n",module_idP );

          for (k = 0; k < 16; k++) {
            LOG_T(MAC, "%x.", dlsch_buffer[k]);
          }

          LOG_T(MAC, "\n");
#endif
          // cycle through SDUs and place in dlsch_buffer
          memcpy(&UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset],dlsch_buffer,sdu_length_total);
          // memcpy(RC.mac[0].DLSCH_pdu[0][0].payload[0][offset],dcch_buffer,sdu_lengths[0]);

          // fill remainder of DLSCH with random data
          for (j=0; j<(TBS-sdu_length_total-offset); j++) {
            UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset+sdu_length_total+j] = (char)(taus()&0xff);
          }

          trace_pdu(DIRECTION_DOWNLINK, (uint8_t *)UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0],
                    TBS, module_idP, WS_C_RNTI, UE_RNTI(module_idP, UE_id),
                    eNB->frame, eNB->subframe,0,0);
          T(T_ENB_MAC_UE_DL_PDU_WITH_DATA, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP), T_INT(subframeP),
            T_INT(harq_pid), T_BUFFER(UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0], TBS));
          UE_info->UE_template[CC_id][UE_id].nb_rb[harq_pid] = nb_rb;
          add_ue_dlsch_info(module_idP,
                            CC_id,
                            UE_id,
                            subframeP,
                            S_DL_SCHEDULED,
                            rnti);
          // store stats
          eNB->eNB_stats[CC_id].dlsch_bytes_tx+=sdu_length_total;
          eNB->eNB_stats[CC_id].dlsch_pdus_tx+=1;
          UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_rounds[0]++;
          UE_info->eNB_UE_stats[CC_id][UE_id].rbs_used = nb_rb;
          UE_info->eNB_UE_stats[CC_id][UE_id].total_rbs_used += nb_rb;
          UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1=eNB_UE_stats->dlsch_mcs1;
          UE_info->eNB_UE_stats[CC_id][UE_id].dlsch_mcs2=mcs;
          UE_info->eNB_UE_stats[CC_id][UE_id].TBS = TBS;
          UE_info->eNB_UE_stats[CC_id][UE_id].overhead_bytes= TBS- sdu_length_total;
          UE_info->eNB_UE_stats[CC_id][UE_id].total_sdu_bytes+= sdu_length_total;
          UE_info->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes+= TBS;
          UE_info->eNB_UE_stats[CC_id][UE_id].total_num_pdus+=1;

          if (cc[CC_id].tdd_Config != NULL) { // TDD
            UE_info->UE_template[CC_id][UE_id].DAI++;
            update_ul_dci(module_idP,CC_id,rnti,UE_info->UE_template[CC_id][UE_id].DAI,subframeP);
          }

          // do PUCCH power control
          // this is the snr
          eNB_UE_stats =  &UE_info->eNB_UE_stats[CC_id][UE_id];
          /* Unit is not dBm, it's special from nfapi */
          snr = ue_sched_ctl->pucch1_snr[CC_id];
          target_snr = eNB->puCch10xSnr/10;
          // this assumes accumulated tpc
          // make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
          int32_t framex10psubframe = UE_info->UE_template[CC_id][UE_id].pucch_tpc_tx_frame*10+UE_info->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe;
          int pucch_tpc_interval=10;

          if (((framex10psubframe+pucch_tpc_interval)<=(frameP*10+subframeP)) || //normal case
              ((framex10psubframe>(frameP*10+subframeP)) && (((10240-framex10psubframe+frameP*10+subframeP)>=pucch_tpc_interval)))) //frame wrap-around
            if (ue_sched_ctl->pucch1_cqi_update[CC_id] == 1) {
              ue_sched_ctl->pucch1_cqi_update[CC_id] = 0;
              UE_info->UE_template[CC_id][UE_id].pucch_tpc_tx_frame=frameP;
              UE_info->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe=subframeP;

              if (snr > target_snr + PUCCH_PCHYST) {
                tpc = 0; //-1
                ue_sched_ctl->pucch_tpc_accumulated[CC_id]--;
              } else if (snr < target_snr - PUCCH_PCHYST) {
                tpc = 2; //+1
                ue_sched_ctl->pucch_tpc_accumulated[CC_id]++;
              } else {
                tpc = 1; //0
              }

              LOG_D(MAC,"[eNB %d] DLSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d, accumulated %d, snr/target snr %d/%d\n",
                    module_idP,frameP, subframeP,harq_pid,tpc,
                    ue_sched_ctl->pucch_tpc_accumulated[CC_id],snr,target_snr);
            } // Po_PUCCH has been updated
            else {
              tpc = 1; //0
            } // time to do TPC update
          else {
            tpc = 1; //0
          }

          dl_config_pdu                                                         = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
          memset((void *)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
          dl_config_pdu->pdu_type                                               = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
          dl_config_pdu->pdu_size                                               = (uint8_t)(2+sizeof(nfapi_dl_config_dci_dl_pdu));
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                  = NFAPI_DL_DCI_FORMAT_1;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level           = get_aggregation(get_bw_index(module_idP,CC_id),ue_sched_ctl->dl_cqi[CC_id],format1);
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag                      = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                        = rnti;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type                   = 1;    // CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power          = 6000; // equal to RS power
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process                = harq_pid;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc                         = tpc; // dont adjust power when retransmitting
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1        = 1-UE_info->UE_template[CC_id][UE_id].oldNDI[harq_pid];
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = mcs;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1        = 0;
          //deactivate second codeword
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_2                       = 0;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_2        = 1;

          if (cc[CC_id].tdd_Config != NULL) { //TDD
            dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.downlink_assignment_index = (UE_info->UE_template[CC_id][UE_id].DAI-1)&3;
            LOG_D(MAC,"[eNB %d] Initial transmission CC_id %d : harq_pid %d, dai %d, mcs %d\n",
                  module_idP,CC_id,harq_pid,
                  (UE_info->UE_template[CC_id][UE_id].DAI-1),
                  mcs);
          } else {
            LOG_D(MAC,"[eNB %d] Initial transmission CC_id %d : harq_pid %d, mcs %d\n",
                  module_idP,CC_id,harq_pid,mcs);
          }

          LOG_D(MAC,"Checking feasibility pdu %d (new sdu)\n",dl_req->number_pdu);

          if (!CCE_allocation_infeasible(module_idP,CC_id,1,subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,rnti)) {
            ue_sched_ctl->round[CC_id][harq_pid] = 0;
            dl_req->number_dci++;
            dl_req->number_pdu++;
            dl_req->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
            eNB->DL_req[CC_id].sfn_sf = frameP<<4 | subframeP;
            eNB->DL_req[CC_id].header.message_id = NFAPI_DL_CONFIG_REQUEST;
            // Toggle NDI for next time
            LOG_D(MAC,"CC_id %d Frame %d, subframeP %d: Toggling Format1 NDI for UE %d (rnti %x/%d) oldNDI %d\n",
                  CC_id, frameP,subframeP,UE_id,
                  rnti,harq_pid,UE_info->UE_template[CC_id][UE_id].oldNDI[harq_pid]);
            UE_info->UE_template[CC_id][UE_id].oldNDI[harq_pid]=1-UE_info->UE_template[CC_id][UE_id].oldNDI[harq_pid];
            UE_info->UE_template[CC_id][UE_id].oldmcs1[harq_pid] = mcs;
            UE_info->UE_template[CC_id][UE_id].oldmcs2[harq_pid] = 0;
            AssertFatal(UE_info->UE_template[CC_id][UE_id].physicalConfigDedicated!=NULL,"physicalConfigDedicated is NULL\n");
            AssertFatal(UE_info->UE_template[CC_id][UE_id].physicalConfigDedicated->pdsch_ConfigDedicated!=NULL,"physicalConfigDedicated->pdsch_ConfigDedicated is NULL\n");
            fill_nfapi_dlsch_config(&dl_req->dl_config_pdu_list[dl_req->number_pdu],
                                    TBS,
                                    eNB->pdu_index[CC_id],
                                    rnti,
                                    0, // type 0 allocation from 7.1.6 in 36.213
                                    0, // virtual_resource_block_assignment_flag, unused here
                                    0, // resource_block_coding, to be filled in later
                                    getQm(mcs),
                                    0, // redundancy version
                                    1, // transport blocks
                                    0, // transport block to codeword swap flag
                                    cc[CC_id].p_eNB == 1 ? 0 : 1, // transmission_scheme
                                    1, // number of layers
                                    1, // number of subbands
                                    //           uint8_t codebook_index,
                                    4, // UE category capacity
                                    UE_info->UE_template[CC_id][UE_id].physicalConfigDedicated->pdsch_ConfigDedicated->p_a,
                                    0, // delta_power_offset for TM5
                                    0, // ngap
                                    0, // nprb
                                    cc[CC_id].p_eNB == 1 ? 1 : 2, // transmission mode
                                    0, //number of PRBs treated as one subband, not used here
                                    0 // number of beamforming vectors, not used here
                                   );
            dl_req->number_pdu++;
            eNB->TX_req[CC_id].sfn_sf = fill_nfapi_tx_req(&eNB->TX_req[CC_id].tx_request_body,
                                        (frameP*10)+subframeP,
                                        TBS,
                                        eNB->pdu_index[CC_id],
                                        eNB->UE_info.DLSCH_pdu[CC_id][0][(unsigned char)UE_id].payload[0]);
            LOG_D(MAC,"Filled NFAPI configuration for DCI/DLSCH/TXREQ %d, new SDU\n",eNB->pdu_index[CC_id]);
            eNB->pdu_index[CC_id]++;
            program_dlsch_acknak(module_idP,CC_id,UE_id,frameP,subframeP,dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.cce_idx);
            last_dlsch_ue_id[CC_id] = UE_id;
          } else {
            LOG_W(MAC,"Frame %d, Subframe %d: Dropping DLSCH allocation for UE %d/%x, infeasible CCE allocations\n",
                  frameP,subframeP,UE_id,rnti);
          }
        } else {  // There is no data from RLC or MAC header, so don't schedule
        }
      }

      if (cc[CC_id].tdd_Config != NULL) { // TDD
        set_ul_DAI(module_idP,UE_id,CC_id,frameP,subframeP);
      }
    } // UE_id loop
  }  // CC_id loop

  fill_DLSCH_dci_fairRR(module_idP,frameP,subframeP,mbsfn_flag);
  stop_meas(&eNB->schedule_dlsch);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH,VCD_FUNCTION_OUT);
}

//------------------------------------------------------------------------------
void
fill_DLSCH_dci_fairRR(
  module_id_t module_idP,
  frame_t frameP,
  sub_frame_t subframeP,
  int *mbsfn_flagP)
//------------------------------------------------------------------------------
{
  // loop over all allocated UEs and compute frequency allocations for PDSCH
  int   UE_id = -1;
  uint8_t            /* first_rb, */ nb_rb=3;
  rnti_t        rnti;
  //unsigned char *vrb_map;
  uint8_t            rballoc_sub[25];
  //uint8_t number_of_subbands=13;
  //unsigned char round;
  unsigned char     harq_pid;
  int               i;
  int               CC_id;
  eNB_MAC_INST      *eNB  =RC.mac[module_idP];
  UE_info_t         *UE_info = &eNB->UE_info;
  int               N_RBG;
  int               N_RB_DL;
  COMMON_channels_t *cc;
  int               j;
  start_meas(&eNB->fill_DLSCH_dci);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_FILL_DLSCH_DCI,VCD_FUNCTION_IN);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC,"Doing fill DCI for CC_id %d\n",CC_id);

    if (mbsfn_flagP[CC_id]>0)
      continue;

    cc              = &eNB->common_channels[CC_id];
    N_RBG           = to_rbg(cc->mib->message.dl_Bandwidth);
    N_RB_DL         = to_prb(cc->mib->message.dl_Bandwidth);

    // UE specific DCIs
    for (j = 0; j < dlsch_ue_select[CC_id].ue_num; j++) {
      if(dlsch_ue_select[CC_id].list[j].ue_priority == SCH_DL_MSG2) {
        continue;
      }

      if(dlsch_ue_select[CC_id].list[j].ue_priority == SCH_DL_MSG4) {
        continue;
      }

      UE_id = dlsch_ue_select[CC_id].list[j].UE_id;
      LOG_T(MAC,"CC_id %d, UE_id: %d => status %d\n",CC_id,UE_id,eNB_dlsch_info[module_idP][CC_id][UE_id].status);

      if (eNB_dlsch_info[module_idP][CC_id][UE_id].status == S_DL_SCHEDULED) {
        // clear scheduling flag
        eNB_dlsch_info[module_idP][CC_id][UE_id].status = S_DL_WAITING;
        rnti = UE_RNTI(module_idP,UE_id);
        harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config,frameP,subframeP);
        nb_rb = UE_info->UE_template[CC_id][UE_id].nb_rb[harq_pid];

        /// Synchronizing rballoc with rballoc_sub
        for(i=0; i<N_RBG; i++) {
          rballoc_sub[i] = UE_info->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][i];
        }

        nfapi_dl_config_request_t      *DL_req         = &RC.mac[module_idP]->DL_req[0];
        nfapi_dl_config_request_pdu_t *dl_config_pdu;

        for (i=0; i<DL_req[CC_id].dl_config_request_body.number_pdu; i++) {
          dl_config_pdu                    = &DL_req[CC_id].dl_config_request_body.dl_config_pdu_list[i];

          if ((dl_config_pdu->pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE)&&
              (dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti == rnti) &&
              (dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format != 1)) {
            dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding    = allocate_prbs_sub(nb_rb,N_RB_DL,N_RBG,rballoc_sub);
            dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_allocation_type = 0;
          } else if ((dl_config_pdu->pdu_type == NFAPI_DL_CONFIG_DLSCH_PDU_TYPE)&&
                     (dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti == rnti) &&
                     (dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type==0)) {
            dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding    = allocate_prbs_sub(nb_rb,N_RB_DL,N_RBG,rballoc_sub);
          }
        }
      }
    }
  }

  stop_meas(&eNB->fill_DLSCH_dci);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
  (VCD_SIGNAL_DUMPER_FUNCTIONS_FILL_DLSCH_DCI, VCD_FUNCTION_OUT);
}



void ulsch_scheduler_pre_ue_select_fairRR(
  module_id_t       module_idP,
  frame_t           frameP,
  sub_frame_t       subframeP,
  sub_frame_t       sched_subframeP,
  ULSCH_UE_SELECT   ulsch_ue_select[MAX_NUM_CCs]) {
  eNB_MAC_INST *eNB=RC.mac[module_idP];
  COMMON_channels_t *cc;
  int CC_id,UE_id;
  int ret;
  uint16_t i;
  uint8_t ue_first_num[MAX_NUM_CCs];
  uint8_t first_ue_total[MAX_NUM_CCs][20];
  uint8_t first_ue_id[MAX_NUM_CCs][20];
  uint8_t ul_inactivity_num[MAX_NUM_CCs];
  uint8_t ul_inactivity_id[MAX_NUM_CCs][20]={{0}};
  uint8_t ulsch_ue_max_num[MAX_NUM_CCs];
  uint16_t saved_ulsch_dci[MAX_NUM_CCs];
  rnti_t rnti;
  UE_sched_ctrl_t *UE_sched_ctl = NULL;
  uint8_t cc_id_flag[MAX_NUM_CCs];
  uint8_t harq_pid = 0,round = 0;
  UE_info_t *UE_info= &eNB->UE_info;
  uint8_t                        aggregation;
  int                            format_flag;
  nfapi_hi_dci0_request_body_t   *HI_DCI0_req;
  nfapi_hi_dci0_request_pdu_t    *hi_dci0_pdu;
  int rrc_status;

  for ( CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++ ) {
    //save ulsch dci number
    saved_ulsch_dci[CC_id] = eNB->HI_DCI0_req[CC_id][subframeP].hi_dci0_request_body.number_of_dci;
    // maximum multiplicity number
    ulsch_ue_max_num[CC_id] =eNB->ue_multiple_max;
    cc_id_flag[CC_id] = 0;
    ue_first_num[CC_id] = 0;
    ul_inactivity_num[CC_id] = 0;
  }

  // UE round >0
  for ( UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++ ) {
    if (UE_info->active[UE_id] == FALSE)
      continue;

    rnti = UE_RNTI(module_idP,UE_id);

    if (rnti ==NOT_A_RNTI)
      continue;

    CC_id = UE_PCCID(module_idP,UE_id);

    if (UE_info->UE_template[CC_id][UE_id].configured == FALSE)
      continue;

    if (UE_info->UE_sched_ctrl[UE_id].ul_failure_timer > 0
        || UE_info->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
      continue;

    // UL DCI
    HI_DCI0_req   = &eNB->HI_DCI0_req[CC_id][subframeP].hi_dci0_request_body;


    if ( (ulsch_ue_select[CC_id].ue_num >= ulsch_ue_max_num[CC_id]) || (cc_id_flag[CC_id] == 1) ) {
      cc_id_flag[CC_id] = 1;
      HI_DCI0_req->number_of_dci = saved_ulsch_dci[CC_id];
      ret = cc_id_end(cc_id_flag);

      if ( ret == 0 ) {
        continue;
      }

      if ( ret == 1 ) {
        return;
      }
    }

    if (mac_eNB_get_rrc_status(module_idP, rnti) == RRC_HO_EXECUTION) {
      aggregation = 4;
      
        if(get_aggregation(get_bw_index(module_idP, CC_id),UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],format0)>4)
          aggregation = get_aggregation(get_bw_index(module_idP, CC_id),UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],format0);
    }else{
      //aggregation = 2;
      aggregation = get_aggregation(get_bw_index(module_idP, CC_id),
                                    UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],
                                    format0);
    }

    cc = &eNB->common_channels[CC_id];
    //harq_pid
    harq_pid = subframe2harqpid(cc,(frameP+(sched_subframeP<subframeP ? 1 : 0)),sched_subframeP);
    //round
    round = UE_info->UE_sched_ctrl[UE_id].round_UL[CC_id][harq_pid];


    if ( round > 0 ) {
      hi_dci0_pdu   = &HI_DCI0_req->hi_dci0_pdu_list[HI_DCI0_req->number_of_dci+HI_DCI0_req->number_of_hi];
      format_flag = 2;
      aggregation=get_aggregation(get_bw_index(module_idP,CC_id),UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],format0);

      if (CCE_allocation_infeasible(module_idP,CC_id,format_flag,subframeP,aggregation,rnti) == 1) {
        cc_id_flag[CC_id] = 1;
        continue;
      } else {
        hi_dci0_pdu->pdu_type                               = NFAPI_HI_DCI0_DCI_PDU_TYPE;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti              = rnti;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.aggregation_level = aggregation;
        HI_DCI0_req->number_of_dci++;
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].ue_priority = SCH_UL_RETRANS;
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].start_rb = eNB->UE_info.UE_template[CC_id][UE_id].first_rb_ul[harq_pid];
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].nb_rb = eNB->UE_info.UE_template[CC_id][UE_id].nb_rb_ul[harq_pid];
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].UE_id = UE_id;
        ulsch_ue_select[CC_id].ue_num++;
        continue;
      }
    }

    //
    int bytes_to_schedule = UE_info->UE_template[CC_id][UE_id].estimated_ul_buffer - UE_info->UE_template[CC_id][UE_id].scheduled_ul_bytes;

    if (bytes_to_schedule < 0) {
      bytes_to_schedule = 0;
      UE_info->UE_template[CC_id][UE_id].scheduled_ul_bytes = 0;
    }


    if ( UE_id > last_ulsch_ue_id[CC_id] && ((ulsch_ue_select[CC_id].ue_num+ue_first_num[CC_id]) < ulsch_ue_max_num[CC_id]) ) {
      if ( bytes_to_schedule > 0 ) {
        first_ue_id[CC_id][ue_first_num[CC_id]]= UE_id;
        first_ue_total[CC_id][ue_first_num[CC_id]] = bytes_to_schedule;
        ue_first_num[CC_id]++;
        continue;
      }

      if ( UE_info->UE_template[CC_id][UE_id].ul_SR > 0 ) {
        first_ue_id[CC_id][ue_first_num[CC_id]]= UE_id;
        first_ue_total[CC_id] [ue_first_num[CC_id]] = 0;
        ue_first_num[CC_id]++;
        continue;
      }

      UE_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
      rrc_status = mac_eNB_get_rrc_status(module_idP, rnti);
      if ( ((UE_sched_ctl->ul_inactivity_timer>10)&&(UE_sched_ctl->ul_scheduled==0)&&(rrc_status < RRC_CONNECTED)) ||
           ((UE_sched_ctl->cqi_req_timer>64)&&((rrc_status >= RRC_CONNECTED))) ) {
        first_ue_id[CC_id][ue_first_num[CC_id]]= UE_id;
        first_ue_total[CC_id] [ue_first_num[CC_id]] = 0;
        ue_first_num[CC_id]++;
        continue;
      }

      /*if ( (ulsch_ue_select[CC_id].ue_num+ul_inactivity_num[CC_id] ) < ulsch_ue_max_num[CC_id] ) {
          UE_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
          uint8_t ul_period = 0;
          if (cc->tdd_Config) {
            ul_period = 50;
          } else {
            ul_period = 20;
          }
          if ( ((UE_sched_ctl->ul_inactivity_timer>ul_period)&&(UE_sched_ctl->ul_scheduled==0))  ||
            ((UE_sched_ctl->ul_inactivity_timer>10)&&(UE_sched_ctl->ul_scheduled==0)&&(mac_eNB_get_rrc_status(module_idP,UE_RNTI(module_idP,UE_id)) < RRC_CONNECTED))) {
          ul_inactivity_id[CC_id][ul_inactivity_num[CC_id]]= UE_id;
          ul_inactivity_num[CC_id] ++;
          continue;
        }
      }*/
    }
  }

  for ( CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++ ) {
    HI_DCI0_req   = &eNB->HI_DCI0_req[CC_id][subframeP].hi_dci0_request_body;

    for ( int temp = 0; temp < ue_first_num[CC_id]; temp++ ) {
      if ( (ulsch_ue_select[CC_id].ue_num >= ulsch_ue_max_num[CC_id]) || (cc_id_flag[CC_id] == 1) ) {
        cc_id_flag[CC_id] = 1;
        HI_DCI0_req->number_of_dci = saved_ulsch_dci[CC_id];
        break;
      }

      hi_dci0_pdu   = &HI_DCI0_req->hi_dci0_pdu_list[HI_DCI0_req->number_of_dci+HI_DCI0_req->number_of_hi];
      format_flag = 2;
      rnti = UE_RNTI(module_idP,first_ue_id[CC_id][temp]);
      aggregation=get_aggregation(get_bw_index(module_idP,CC_id),UE_info->UE_sched_ctrl[first_ue_id[CC_id][temp]].dl_cqi[CC_id],format0);

      if (CCE_allocation_infeasible(module_idP,CC_id,format_flag,subframeP,aggregation,rnti) == 1) {
        cc_id_flag[CC_id] = 1;
        break;
      } else {
        hi_dci0_pdu->pdu_type                               = NFAPI_HI_DCI0_DCI_PDU_TYPE;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti              = rnti;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.aggregation_level = aggregation;
        HI_DCI0_req->number_of_dci++;
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].ue_priority = SCH_UL_FIRST;
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].ul_total_buffer = first_ue_total[CC_id][temp];
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].UE_id = first_ue_id[CC_id][temp];
        ulsch_ue_select[CC_id].ue_num++;
      }
    }
  }

  for ( UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++ ) {
    if (UE_info->active[UE_id] == FALSE)
      continue;

    rnti = UE_RNTI(module_idP,UE_id);

    if (rnti ==NOT_A_RNTI)
      continue;

    CC_id = UE_PCCID(module_idP,UE_id);

    if (UE_id > last_ulsch_ue_id[CC_id])
      continue;

    if (UE_info->UE_template[CC_id][UE_id].configured == FALSE)
      continue;

    if (UE_info->UE_sched_ctrl[UE_id].ul_failure_timer > 0
        || UE_info->UE_sched_ctrl[UE_id].ul_out_of_sync == 1)
      continue;

    if ( (ulsch_ue_select[CC_id].ue_num >= ulsch_ue_max_num[CC_id]) || (cc_id_flag[CC_id] == 1) ) {
      cc_id_flag[CC_id] = 1;
      HI_DCI0_req->number_of_dci = saved_ulsch_dci[CC_id];
      ret = cc_id_end(cc_id_flag);

      if ( ret == 0 ) {
        continue;
      }

      if ( ret == 1 ) {
        return;
      }
    }

    for(i = 0; i<ulsch_ue_select[CC_id].ue_num; i++) {
      if(ulsch_ue_select[CC_id].list[i].UE_id == UE_id) {
        break;
      }
    }

    if(i < ulsch_ue_select[CC_id].ue_num)
      continue;

    HI_DCI0_req   = &eNB->HI_DCI0_req[CC_id][subframeP].hi_dci0_request_body;
    //SR BSR
    UE_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
    int bytes_to_schedule = UE_info->UE_template[CC_id][UE_id].estimated_ul_buffer - UE_info->UE_template[CC_id][UE_id].scheduled_ul_bytes;

    if (bytes_to_schedule < 0) {
      bytes_to_schedule = 0;
      UE_info->UE_template[CC_id][UE_id].scheduled_ul_bytes = 0;
    }
    rrc_status = mac_eNB_get_rrc_status(module_idP, rnti);


    if ( (bytes_to_schedule > 0) || (UE_info->UE_template[CC_id][UE_id].ul_SR > 0) ||
         ((UE_sched_ctl->ul_inactivity_timer>64)&&(UE_sched_ctl->ul_scheduled==0))  ||
         ((UE_sched_ctl->ul_inactivity_timer>10)&&(UE_sched_ctl->ul_scheduled==0)&&(rrc_status < RRC_CONNECTED)) ||
         ((UE_sched_ctl->cqi_req_timer>64)&&((rrc_status >= RRC_CONNECTED))) ) {
      hi_dci0_pdu   = &HI_DCI0_req->hi_dci0_pdu_list[HI_DCI0_req->number_of_dci+HI_DCI0_req->number_of_hi];
      format_flag = 2;
      if (mac_eNB_get_rrc_status(module_idP, rnti) == RRC_HO_EXECUTION) {
        aggregation = 4;
      
        if(get_aggregation(get_bw_index(module_idP, CC_id),UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],format0)>4)
          aggregation = get_aggregation(get_bw_index(module_idP, CC_id),UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],format0);
      }else {
        aggregation = get_aggregation(get_bw_index(module_idP, CC_id),
                                    UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],
                                    format0);
      }

      if (CCE_allocation_infeasible(module_idP,CC_id,format_flag,subframeP,aggregation,rnti) == 1) {
        cc_id_flag[CC_id] = 1;
        continue;
      } else {
        hi_dci0_pdu->pdu_type                               = NFAPI_HI_DCI0_DCI_PDU_TYPE;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti              = rnti;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.aggregation_level = aggregation;
        HI_DCI0_req->number_of_dci++;
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].ue_priority = SCH_UL_FIRST;

        if(bytes_to_schedule > 0)
          ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].ul_total_buffer = bytes_to_schedule;
        else if(UE_info->UE_template[CC_id][UE_id].ul_SR > 0)
          ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].ul_total_buffer = 0;

        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].UE_id = UE_id;
        ulsch_ue_select[CC_id].ue_num++;
        continue;
      }
    }

    //inactivity UE
    /*    if ( (ulsch_ue_select[CC_id].ue_num+ul_inactivity_num[CC_id]) < ulsch_ue_max_num[CC_id] ) {
            UE_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
            uint8_t ul_period = 0;
            if (cc->tdd_Config) {
              ul_period = 50;
            } else {
              ul_period = 20;
            }
            if ( ((UE_sched_ctl->ul_inactivity_timer>ul_period)&&(UE_sched_ctl->ul_scheduled==0))  ||
                ((UE_sched_ctl->ul_inactivity_timer>10)&&(UE_sched_ctl->ul_scheduled==0)&&(mac_eNB_get_rrc_status(module_idP,UE_RNTI(module_idP,UE_id)) < RRC_CONNECTED))) {
              ul_inactivity_id[CC_id][ul_inactivity_num[CC_id]]= UE_id;
              ul_inactivity_num[CC_id]++;
              continue;
            }
        }*/
  }

  for ( CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++ ) {
    HI_DCI0_req   = &eNB->HI_DCI0_req[CC_id][subframeP].hi_dci0_request_body;

    for ( int temp = 0; temp < ul_inactivity_num[CC_id]; temp++ ) {
      if ( (ulsch_ue_select[CC_id].ue_num >= ulsch_ue_max_num[CC_id]) || (cc_id_flag[CC_id] == 1) ) {
        HI_DCI0_req   = &eNB->HI_DCI0_req[CC_id][subframeP].hi_dci0_request_body;
        cc_id_flag[CC_id] = 1;
        break;
      }

      hi_dci0_pdu   = &HI_DCI0_req->hi_dci0_pdu_list[HI_DCI0_req->number_of_dci+HI_DCI0_req->number_of_hi];
      format_flag = 2;
      rnti = UE_RNTI(module_idP,ul_inactivity_id[CC_id][temp]);
      aggregation=get_aggregation(get_bw_index(module_idP,CC_id),UE_info->UE_sched_ctrl[ul_inactivity_id[CC_id][temp]].dl_cqi[CC_id],format0);

      if (CCE_allocation_infeasible(module_idP,CC_id,format_flag,subframeP,aggregation,rnti) == 1) {
        cc_id_flag[CC_id] = 1;
        continue;
      } else {
        hi_dci0_pdu->pdu_type                               = NFAPI_HI_DCI0_DCI_PDU_TYPE;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti              = rnti;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.aggregation_level = aggregation;
        HI_DCI0_req->number_of_dci++;
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].ue_priority = SCH_UL_INACTIVE;
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].ul_total_buffer = 0;
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].UE_id = ul_inactivity_id[CC_id][temp];
        ulsch_ue_select[CC_id].ue_num++;
      }
    }

    HI_DCI0_req->number_of_dci = saved_ulsch_dci[CC_id];
  }

  return;
}

uint8_t find_rb_table_index(uint8_t average_rbs) {
  int i;

  for ( i = 0; i < 34; i++ ) {
    if ( rb_table[i] > average_rbs ) {
      return (i-1);
    }
  }

  return i;
}

void ulsch_scheduler_pre_processor_fairRR(module_id_t module_idP,
    frame_t frameP,
    sub_frame_t subframeP,
    sub_frame_t sched_subframeP,
    ULSCH_UE_SELECT ulsch_ue_select[MAX_NUM_CCs]) {
  int                CC_id,ulsch_ue_num;
  eNB_MAC_INST       *eNB = RC.mac[module_idP];
  UE_info_t          *UE_info= &eNB->UE_info;
  UE_TEMPLATE        *UE_template = NULL;
  uint8_t            ue_num_temp;
  uint8_t            total_rbs=0;
  uint8_t            average_rbs;
  uint16_t           first_rb[MAX_NUM_CCs];
  int8_t             mcs;
  uint8_t            snr;
  double             bler_filter=0.9;
  double             bler;
  uint8_t            rb_table_index;
  uint8_t            num_pucch_rb;
  uint32_t           tbs;
  int16_t            tx_power;
  int                UE_id;
  COMMON_channels_t *cc;
  LOG_D(MAC,"In ulsch_preprocessor: ulsch ue select\n");
  //ue select
  ulsch_scheduler_pre_ue_select_fairRR(module_idP,frameP,subframeP,sched_subframeP,ulsch_ue_select);

  // MCS and RB assgin
  for ( CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++ ) {
    cc = &RC.mac[module_idP]->common_channels[CC_id];
    int N_RB_UL = to_prb(cc->ul_Bandwidth);
    if (cc->tdd_Config) { //TDD
      if (N_RB_UL == 25) {
        num_pucch_rb = 1;
      } else if (N_RB_UL == 50) {
        num_pucch_rb = 2;
      } else {
        num_pucch_rb = 3;
      }
    } else {//FDD
      if (N_RB_UL == 25) {
        num_pucch_rb = 1;
      } else {
        num_pucch_rb = 2;
      }
    }

    first_rb[CC_id] = num_pucch_rb;
    ue_num_temp       = ulsch_ue_select[CC_id].ue_num;

    for ( ulsch_ue_num = 0; ulsch_ue_num < ulsch_ue_select[CC_id].ue_num; ulsch_ue_num++ ) {
      UE_id = ulsch_ue_select[CC_id].list[ulsch_ue_num].UE_id;

      if (ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority == SCH_UL_MSG3) {
        first_rb[CC_id] ++;
        ue_num_temp--;
        continue;
      }

      if (ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority == SCH_UL_PRACH) {
        first_rb[CC_id] = ulsch_ue_select[CC_id].list[ulsch_ue_num].start_rb+ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb;
        ue_num_temp--;
        continue;
      }

      if (first_rb[CC_id] >= N_RB_UL-num_pucch_rb ) {
        LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: dropping, not enough RBs\n",
              module_idP,frameP,subframeP,UE_id,UE_RNTI(CC_id,UE_id),CC_id);
        break;
      }

      total_rbs = N_RB_UL-num_pucch_rb-first_rb[CC_id];
      average_rbs = (int)round((double)total_rbs/(double)ue_num_temp);

      if ( average_rbs < 3 ) {
        ue_num_temp--;
        ulsch_ue_num--;
        ulsch_ue_select[CC_id].ue_num--;
        continue;
      }

      if ( ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority == SCH_UL_RETRANS ) {
        if ( ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb <= average_rbs ) {
          // assigne RBS(nb_rb)
          ulsch_ue_select[CC_id].list[ulsch_ue_num].start_rb = first_rb[CC_id];
          first_rb[CC_id] = first_rb[CC_id] + ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb;
        }

        if ( ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb > average_rbs ) {
          if ( ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb <= total_rbs ) {
            // assigne RBS(average_rbs)
            ulsch_ue_select[CC_id].list[ulsch_ue_num].start_rb = first_rb[CC_id];
            first_rb[CC_id] = first_rb[CC_id] + ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb;
          } else {
            // assigne RBS(remain rbs)
            ulsch_ue_select[CC_id].list[ulsch_ue_num].start_rb = first_rb[CC_id];
            rb_table_index = 2;

            while(rb_table[rb_table_index] <= total_rbs) {
              rb_table_index++;
            }

            ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb = rb_table[rb_table_index-1];
            first_rb[CC_id] = first_rb[CC_id] + rb_table[rb_table_index-1];
          }
        }
      } else {
        UE_template = &UE_info->UE_template[CC_id][UE_id];

        int32_t framex10psubframe = UE_template->pusch_bler_calc_frame*10+UE_template->pusch_bler_calc_subframe;
        int pusch_bler_interval=50;
        double total_bler;
        
        if(UE_info->UE_sched_ctrl[UE_id].pusch_rx_num[CC_id] == 0 && UE_info->UE_sched_ctrl[UE_id].pusch_rx_error_num[CC_id] == 0) {
          total_bler = 0;
        }
        else {
          total_bler = (double)UE_info->UE_sched_ctrl[UE_id].pusch_rx_error_num[CC_id] / (double)(UE_info->UE_sched_ctrl[UE_id].pusch_rx_error_num[CC_id] + UE_info->UE_sched_ctrl[UE_id].pusch_rx_num[CC_id]) * 100;
        }

        if (((framex10psubframe+pusch_bler_interval)<=(frameP*10+subframeP)) || //normal case
          ((framex10psubframe>(frameP*10+subframeP)) && (((10240-framex10psubframe+frameP*10+subframeP)>=pusch_bler_interval)))) { //frame wrap-around

          UE_template->pusch_bler_calc_frame=frameP;
          UE_template->pusch_bler_calc_subframe=subframeP;

          int pusch_rx_num_diff       = UE_info->UE_sched_ctrl[UE_id].pusch_rx_num[CC_id] - UE_info->UE_sched_ctrl[UE_id].pusch_rx_num_old[CC_id];
          int pusch_rx_error_num_diff = UE_info->UE_sched_ctrl[UE_id].pusch_rx_error_num[CC_id] - UE_info->UE_sched_ctrl[UE_id].pusch_rx_error_num_old[CC_id];

          if((pusch_rx_num_diff == 0) && (pusch_rx_error_num_diff == 0)) { /* first rx case or no rx among interval time */
            bler = UE_info->UE_sched_ctrl[UE_id].pusch_bler[CC_id];
          }
          else {
            bler = (double)pusch_rx_error_num_diff / (double)(pusch_rx_num_diff + pusch_rx_error_num_diff) * 100;
          }
          UE_info->UE_sched_ctrl[UE_id].pusch_bler[CC_id] = UE_info->UE_sched_ctrl[UE_id].pusch_bler[CC_id] * bler_filter + bler * (1.0-bler_filter);
          UE_info->UE_sched_ctrl[UE_id].pusch_rx_num_old[CC_id] = UE_info->UE_sched_ctrl[UE_id].pusch_rx_num[CC_id];
          UE_info->UE_sched_ctrl[UE_id].pusch_rx_error_num_old[CC_id] = UE_info->UE_sched_ctrl[UE_id].pusch_rx_error_num[CC_id];

          if (eNB->use_mcs_offset == 1) {
            if(bler < eNB->bler_lower) {
              if(UE_info->UE_sched_ctrl[UE_id].mcs_offset[CC_id] !=0) {
                UE_info->UE_sched_ctrl[UE_id].mcs_offset[CC_id]--;
              }
            }
            if(bler >= eNB->bler_upper) {
              UE_info->UE_sched_ctrl[UE_id].mcs_offset[CC_id]++;
              if(UE_info->UE_sched_ctrl[UE_id].mcs_offset[CC_id] >= 20) {
                UE_info->UE_sched_ctrl[UE_id].mcs_offset[CC_id]=20;
              }
            }
          }
        }
        snr = UE_info->UE_sched_ctrl[UE_id].pusch_snr_avg[CC_id];
 
        mcs = 20 - UE_info->UE_sched_ctrl[UE_id].mcs_offset[CC_id];
        if(mcs < 6) {
          mcs = 6;
        }

        if ( ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority  == SCH_UL_FIRST ) {
          int bytes_to_schedule = UE_template->estimated_ul_buffer - UE_template->scheduled_ul_bytes;

          if (bytes_to_schedule < 0) {
            bytes_to_schedule = 0;
            UE_template->scheduled_ul_bytes = 0;
          }

          if ( ulsch_ue_select[CC_id].list[ulsch_ue_num].ul_total_buffer > 0 ) {
            rb_table_index = 2;
            tbs = get_TBS_UL(mcs,rb_table[rb_table_index]);
            tx_power= estimate_ue_tx_power(0,tbs*8,rb_table[rb_table_index],0,cc->Ncp,0);

            while ( (((UE_template->phr_info - tx_power) < 0 ) || (tbs > bytes_to_schedule)) && (mcs > 3) ) {
              mcs--;
              tbs = get_TBS_UL(mcs,rb_table[rb_table_index]);
              tx_power= estimate_ue_tx_power(0,tbs*8,rb_table[rb_table_index],0,cc->Ncp,0);
            }

            while ( (tbs < bytes_to_schedule) && (rb_table[rb_table_index]<(N_RB_UL-num_pucch_rb-first_rb[CC_id])) &&
                    ((UE_template->phr_info - tx_power) > 0) && (rb_table_index < eNB->max_ul_rb_index )) {
              rb_table_index++;
              tbs = get_TBS_UL(mcs,rb_table[rb_table_index]);
              tx_power= estimate_ue_tx_power(0,tbs*8,rb_table[rb_table_index],0,cc->Ncp,0);
            }
            if ( rb_table[rb_table_index]<3 ) {
              rb_table_index=2;
            }

            if ( rb_table[rb_table_index] <= average_rbs ) {
              // assigne RBS( nb_rb)
              first_rb[CC_id] = first_rb[CC_id] + rb_table[rb_table_index];
              UE_info->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul = rb_table[rb_table_index];
              UE_info->UE_template[CC_id][UE_id].pre_allocated_rb_table_index_ul = rb_table_index;
              UE_info->UE_template[CC_id][UE_id].pre_assigned_mcs_ul = mcs;
            }

            if ( rb_table[rb_table_index] > average_rbs ) {
              // assigne RBS(average_rbs)
              rb_table_index = find_rb_table_index(average_rbs);

              if (rb_table_index>=34) {
                LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: average RBs %d > 100\n",
                      module_idP,frameP,subframeP,UE_id,UE_RNTI(CC_id,UE_id),CC_id,average_rbs);
                break;
              }

              first_rb[CC_id] = first_rb[CC_id] + rb_table[rb_table_index];
              UE_info->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul = rb_table[rb_table_index];
              UE_info->UE_template[CC_id][UE_id].pre_allocated_rb_table_index_ul = rb_table_index;
              UE_info->UE_template[CC_id][UE_id].pre_assigned_mcs_ul = mcs;
            }
            LOG_D(MAC,"[eNB %hu] frame %u subframe %u, UE %d/%x CC %d snr %hhu snr_inst %hhu mcs %hhd mcs_offset %hhu bler %lf total_bler %lf ( %lu %lu ) rb_num %hhu phr_info %hhd tx_power %hd bsr %d estimated_ul_buffer %d scheduled_ul_bytes %d\n",
                  module_idP,frameP,subframeP,UE_id,UE_RNTI(CC_id,UE_id),CC_id, snr, UE_info->UE_sched_ctrl[UE_id].pusch_snr[CC_id], mcs, UE_info->UE_sched_ctrl[UE_id].mcs_offset[CC_id], UE_info->UE_sched_ctrl[UE_id].pusch_bler[CC_id], 
                  total_bler, UE_info->UE_sched_ctrl[UE_id].pusch_rx_num[CC_id], UE_info->UE_sched_ctrl[UE_id].pusch_rx_error_num[CC_id], rb_table[rb_table_index-1], UE_template->phr_info, tx_power, bytes_to_schedule, UE_template->estimated_ul_buffer, UE_template->scheduled_ul_bytes);
            
          } else {
            if (mac_eNB_get_rrc_status(module_idP,UE_RNTI(module_idP, UE_id)) < RRC_CONNECTED) {
              // assigne RBS( 6 RBs)
              first_rb[CC_id] = first_rb[CC_id] + 6;
              UE_info->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul = 6;
              UE_info->UE_template[CC_id][UE_id].pre_allocated_rb_table_index_ul = 5;
              UE_info->UE_template[CC_id][UE_id].pre_assigned_mcs_ul = 10;
            } else {
              // assigne RBS( 5 RBs)
              first_rb[CC_id] = first_rb[CC_id] + 5;
              UE_info->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul = 5;
              UE_info->UE_template[CC_id][UE_id].pre_allocated_rb_table_index_ul = 4;
              UE_info->UE_template[CC_id][UE_id].pre_assigned_mcs_ul = 10; 
            }
          }
        } else if ( ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority  == SCH_UL_INACTIVE ) {
          // assigne RBS( 3 RBs)
          first_rb[CC_id] = first_rb[CC_id] + 3;
          UE_info->UE_template[CC_id][UE_id].pre_allocated_nb_rb_ul = 3;
          UE_info->UE_template[CC_id][UE_id].pre_allocated_rb_table_index_ul = 2;
          UE_info->UE_template[CC_id][UE_id].pre_assigned_mcs_ul = 10;
        }
      }

      ue_num_temp--;
    }
  }
}


void
schedule_ulsch_fairRR(module_id_t module_idP, frame_t frameP,
                      sub_frame_t subframeP) {
  uint16_t i;
  int CC_id;
  eNB_MAC_INST *mac = RC.mac[module_idP];
  COMMON_channels_t *cc;
  int sched_frame=frameP;
  start_meas(&mac->schedule_ulsch);
  int sched_subframe = (subframeP+4)%10;
  cc = &mac->common_channels[0];
  int tdd_sfa;

  // for TDD: check subframes where we have to act and return if nothing should be done now
  if (cc->tdd_Config) {
    tdd_sfa = cc->tdd_Config->subframeAssignment;

    switch (subframeP) {
      case 0:
        if ((tdd_sfa == 0)||
            (tdd_sfa == 3)) sched_subframe = 4;
        else if (tdd_sfa==6) sched_subframe = 7;
        else return;

        break;

      case 1:
        if ((tdd_sfa==0)||
            (tdd_sfa==1)) sched_subframe = 7;
        else if (tdd_sfa==6) sched_subframe = 8;
        else return;

        break;

      case 2: // Don't schedule UL in subframe 2 for TDD
        return;

      case 3:
        if (tdd_sfa==2) sched_subframe = 7;
        else return;

        break;

      case 4:
        if (tdd_sfa==1) sched_subframe = 8;
        else return;

        break;

      case 5:
        if (tdd_sfa==0)      sched_subframe = 9;
        else if (tdd_sfa==6) sched_subframe = 2;
        else return;

        break;

      case 6:
        if (tdd_sfa==0 || tdd_sfa==1)      sched_subframe = 2;
        else if (tdd_sfa==6) sched_subframe = 3;
        else return;

        break;

      case 7:
        return;

      case 8:
        if ((tdd_sfa>=2) && (tdd_sfa<=5)) sched_subframe=2;
        else return;

        break;

      case 9:
        if ((tdd_sfa==1) || (tdd_sfa==3) || (tdd_sfa==4)) sched_subframe=3;
        else if (tdd_sfa==6) sched_subframe=4;
        else return;

        break;
    }
  }

  if (sched_subframe < subframeP) sched_frame++;

  ULSCH_UE_SELECT ulsch_ue_select[MAX_NUM_CCs];
  memset(ulsch_ue_select, 0, sizeof(ulsch_ue_select));

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    cc = &mac->common_channels[CC_id];
    int N_RB_UL=to_prb(cc->ul_Bandwidth);
    // output of scheduling, the UE numbers in RBs, where it is in the code???
    // check if RA (Msg3) is active in this subframeP, if so skip the PRBs used for Msg3
    // Msg3 is using 1 PRB so we need to increase first_rb accordingly
    // not sure about the break (can there be more than 1 active RA procedure?)
    for (i=0; i<NB_RA_PROC_MAX; i++) {
      if ((cc->ra[i].state == WAITMSG3) &&(cc->ra[i].Msg3_subframe == sched_subframe)) {
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].ue_priority = SCH_UL_MSG3;

        if (cc->tdd_Config == NULL) {
          if(N_RB_UL == 25) {
            ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].start_rb = 1;
          } else {
            ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].start_rb = 2;
          }
        } else {
          switch(N_RB_UL) {
            case 25:
            default:
              ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].start_rb = 1;
              break;

            case 50:
              ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].start_rb = 2;
              break;

            case 100:
              ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].start_rb = 3;
              break;
          }
        }

        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].nb_rb = 1;
        ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].UE_id = -1;
        ulsch_ue_select[CC_id].ue_num++;
        break;
      }
    }

    //PRACH
    if (is_prach_subframe0(cc->tdd_Config!=NULL ? cc->tdd_Config->subframeAssignment : 0,cc->tdd_Config!=NULL ? 1 : 0,
                           cc->radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_ConfigIndex,
                           sched_frame,sched_subframe)==1) {
      ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].ue_priority = SCH_UL_PRACH;
      ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].start_rb = 
           get_prach_prb_offset(cc->tdd_Config!=NULL ? 1 : 0, 
                                cc->tdd_Config!=NULL ? cc->tdd_Config->subframeAssignment : 0,
                                to_prb(cc->ul_Bandwidth),
                                cc->radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_ConfigIndex,
                                cc->radioResourceConfigCommon->prach_Config.prach_ConfigInfo.prach_FreqOffset,
                                0,
                                sched_frame);
      ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].nb_rb = 6;
      ulsch_ue_select[CC_id].list[ulsch_ue_select[CC_id].ue_num].UE_id = -1;
      ulsch_ue_select[CC_id].ue_num++;
    }
  }

  schedule_ulsch_rnti_fairRR(module_idP, frameP, subframeP, sched_subframe,ulsch_ue_select);
  stop_meas(&mac->schedule_ulsch);
}

void schedule_ulsch_rnti_fairRR(module_id_t   module_idP,
                                frame_t       frameP,
                                sub_frame_t   subframeP,
                                unsigned char sched_subframeP,
                                ULSCH_UE_SELECT  ulsch_ue_select[MAX_NUM_CCs]) {
  int16_t           UE_id;
  uint8_t           aggregation;
  uint16_t          first_rb[MAX_NUM_CCs];
  uint8_t           ULSCH_first_end;
  rnti_t            rnti           = -1;
  uint8_t           round          = 0;
  uint8_t           harq_pid       = 0;
  uint8_t           status         = 0;
  uint8_t           rb_table_index = -1;
  uint32_t          cqi_req,cshift,ndi,tpc;
  int32_t           snr;
  int32_t           target_snr=0;
  int               CC_id,ulsch_ue_num;
  int               N_RB_UL;
  eNB_MAC_INST      *eNB = RC.mac[module_idP];
  COMMON_channels_t *cc;
  UE_info_t         *UE_info=&eNB->UE_info;
  UE_TEMPLATE       *UE_template;
  UE_sched_ctrl_t     *UE_sched_ctrl;
  int               sched_frame=frameP;
  int               rvidx_tab[4] = {0,2,3,1};
  uint16_t          ul_req_index;
  uint8_t           dlsch_flag;

  if (sched_subframeP < subframeP) sched_frame++;

  nfapi_hi_dci0_request_body_t   *hi_dci0_req;
  nfapi_hi_dci0_request_pdu_t    *hi_dci0_pdu;
  nfapi_ul_config_request_body_t *ul_req_tmp;
  nfapi_ul_config_ulsch_harq_information *ulsch_harq_information;
  LOG_D(MAC,"entering ulsch preprocesor for %d.%d\n",sched_frame,sched_subframeP);
  ulsch_scheduler_pre_processor_fairRR(module_idP,
                                       frameP,
                                       subframeP,
                                       sched_subframeP,
                                       ulsch_ue_select);
  LOG_D(MAC,"exiting ulsch preprocesor\n");

  // loop over all active UEs
  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    hi_dci0_req = &eNB->HI_DCI0_req[CC_id][subframeP].hi_dci0_request_body;
    eNB->HI_DCI0_req[CC_id][subframeP].sfn_sf = (frameP<<4)+subframeP;
    ul_req_tmp = &eNB->UL_req_tmp[CC_id][sched_subframeP].ul_config_request_body;
    nfapi_ul_config_request_t *ul_req  = &eNB->UL_req_tmp[CC_id][sched_subframeP];
    ULSCH_first_end = 0;
    cc  = &eNB->common_channels[CC_id];
    // This is the actual CC_id in the list
    N_RB_UL      = to_prb(cc->mib->message.dl_Bandwidth);

    //leave out first RB for PUCCH
    if (cc->tdd_Config == NULL) {
      if(N_RB_UL == 25) {
        first_rb[CC_id] = 1;
      } else {
        first_rb[CC_id] = 2;
      }
    } else {
      switch(N_RB_UL) {
        case 25:
        default:
          first_rb[CC_id] = 1;
          break;

        case 50:
          first_rb[CC_id] = 2;
          break;

        case 100:
          first_rb[CC_id] = 3;
          break;
      }
    }

    for ( ulsch_ue_num = 0; ulsch_ue_num < ulsch_ue_select[CC_id].ue_num; ulsch_ue_num++ ) {
      UE_id = ulsch_ue_select[CC_id].list[ulsch_ue_num].UE_id;

      /* be sure that there are some free RBs */
      if (cc->tdd_Config == NULL) {
        if(N_RB_UL == 25) {
          if (first_rb[CC_id] >= N_RB_UL-1) {
            LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: dropping, not enough RBs\n",
                  module_idP,frameP,subframeP,UE_id,rnti,CC_id);
            break;
          }
        } else {
          if (first_rb[CC_id] >= N_RB_UL-2) {
            LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d: dropping, not enough RBs\n",
                  module_idP,frameP,subframeP,UE_id,rnti,CC_id);
            break;
          }
        }
      } else {
        if(N_RB_UL == 25) {
          if (first_rb[CC_id] >= N_RB_UL-1) {
            LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d N_RB_UL %d first_rb %d: dropping, not enough RBs\n",
                  module_idP,frameP,subframeP,UE_id,rnti,CC_id, N_RB_UL, first_rb[CC_id]);
            break;
          }
        } else if(N_RB_UL == 50) {
          if (first_rb[CC_id] >= N_RB_UL-2) {
            LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d N_RB_UL %d first_rb %d: dropping, not enough RBs\n",
                  module_idP,frameP,subframeP,UE_id,rnti,CC_id, N_RB_UL, first_rb[CC_id]);
            break;
          }
        } else if(N_RB_UL == 100) {
          if (first_rb[CC_id] >= N_RB_UL-3) {
            LOG_W(MAC,"[eNB %d] frame %d subframe %d, UE %d/%x CC %d N_RB_UL %d first_rb %d: dropping, not enough RBs\n",
                  module_idP,frameP,subframeP,UE_id,rnti,CC_id, N_RB_UL, first_rb[CC_id]);
            break;
          }
        }
      }

      //MSG3
      if (ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority == SCH_UL_MSG3) {
        first_rb[CC_id] ++;
        continue;
      }

      //PRACH
      if (ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority == SCH_UL_PRACH) {
        first_rb[CC_id] = ulsch_ue_select[CC_id].list[ulsch_ue_num].start_rb+ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb;
        continue;
      }

      UE_template   = &UE_info->UE_template[CC_id][UE_id];
      UE_sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
      harq_pid      = subframe2harqpid(cc,sched_frame,sched_subframeP);
      rnti = UE_RNTI(CC_id,UE_id);
      if (mac_eNB_get_rrc_status(module_idP, rnti) == RRC_HO_EXECUTION) {
        aggregation = 4;
        if(get_aggregation(get_bw_index(module_idP, CC_id),UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],format0)>4)
          aggregation = get_aggregation(get_bw_index(module_idP, CC_id),UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],format0);
      }else{
        //aggregation = 2;
        aggregation = get_aggregation(get_bw_index(module_idP, CC_id),
                                      UE_info->UE_sched_ctrl[UE_id].dl_cqi[CC_id],
                                      format0);
      }
      LOG_D(MAC,"[eNB %d] frame %d subframe %d,Checking PUSCH %d for UE %d/%x CC %d : aggregation level %d, N_RB_UL %d\n",
            module_idP,frameP,subframeP,harq_pid,UE_id,rnti,CC_id, aggregation,N_RB_UL);
      int bytes_to_schedule = UE_template->estimated_ul_buffer - UE_template->scheduled_ul_bytes;

      if (bytes_to_schedule < 0) {
        bytes_to_schedule = 0;
        UE_template->scheduled_ul_bytes = 0;
      }

      RC.eNB[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP*10)+subframeP] = bytes_to_schedule;
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BO,RC.eNB[module_idP][CC_id]->pusch_stats_BO[UE_id][(frameP*10)+subframeP]);
      status = mac_eNB_get_rrc_status(module_idP,rnti);

      if (status < RRC_CONNECTED)
        cqi_req = 0;
      else if (UE_sched_ctrl->cqi_received == 1){
        LOG_D(MAC,"Clearing CQI request timer\n");
        UE_sched_ctrl->cqi_req_flag = 0;
        UE_sched_ctrl->cqi_received = 0;
        UE_sched_ctrl->cqi_req_timer = 0;
        cqi_req = 0;
      }else if (UE_sched_ctrl->cqi_req_timer>64) {
        cqi_req = 1;

        // To be safe , do not ask CQI in special SFs:36.213/7.2.3 CQI definition
        if (cc->tdd_Config) {
          switch (cc->tdd_Config->subframeAssignment) {
            case 1:
              if( subframeP == 1 || subframeP == 6 ) cqi_req=0;

              break;

            case 3:
              if( subframeP == 1 ) cqi_req=0;

              break;

            default:
              LOG_E(MAC," TDD config not supported\n");
              break;
          }
        }

        if(cqi_req == 1) {
          UE_sched_ctrl->cqi_req_flag |= 1 << sched_subframeP;
        }
      } else
        cqi_req = 0;

      //power control
      //compute the expected ULSCH RX power (for the stats)
      // this is the normalized RX power and this should be constant (regardless of mcs
      snr = UE_sched_ctrl->pusch_snr_avg[CC_id];
      target_snr = eNB->puSch10xSnr / 10;
      // this assumes accumulated tpc
      // make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
      int32_t framex10psubframe = UE_template->pusch_tpc_tx_frame*10+UE_template->pusch_tpc_tx_subframe;
      int pusch_tpc_interval=500;

      if (((framex10psubframe+pusch_tpc_interval)<=(frameP*10+subframeP)) || //normal case
          ((framex10psubframe>(frameP*10+subframeP)) && (((10240-framex10psubframe+frameP*10+subframeP)>=pusch_tpc_interval)))) { //frame wrap-around
        UE_template->pusch_tpc_tx_frame=frameP;
        UE_template->pusch_tpc_tx_subframe=subframeP;

        if (snr > target_snr + PUSCH_PCHYST) {
          tpc = 0; //-1
          UE_sched_ctrl->pusch_tpc_accumulated[CC_id]--;
        } else if (snr < target_snr - PUSCH_PCHYST) {
          tpc = 2; //+1
          UE_sched_ctrl->pusch_tpc_accumulated[CC_id]++;
        } else {
          tpc = 1; //0
        }
      } else {
        tpc = 1; //0
      }

      if (tpc!=1) {
        LOG_D(MAC,"[eNB %d] ULSCH schedulerRR: frame %d, subframe %d, harq_pid %d, tpc %d, accumulated %d, snr/target snr %d/%d\n",
              module_idP,frameP,subframeP,harq_pid,tpc,
              UE_sched_ctrl->pusch_tpc_accumulated[CC_id],snr,target_snr);
      }

      // new transmission
      if ((ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority == SCH_UL_FIRST) ||
          (ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority == SCH_UL_INACTIVE)) {
        LOG_D(MAC,"[eNB %d][PUSCH %d] Frame %d subframe %d Scheduling UE %d/%x (SR %d,UL_inactivity timer %d,UL_failure timer %d,cqi_req_timer %d)\n",
              module_idP,harq_pid,frameP,subframeP,UE_id,rnti,UE_template->ul_SR,
              UE_sched_ctrl->ul_inactivity_timer,
              UE_sched_ctrl->ul_failure_timer,
              UE_sched_ctrl->cqi_req_timer);
        ndi = 1-UE_template->oldNDI_UL[harq_pid];
        UE_template->oldNDI_UL[harq_pid]=ndi;
        UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_rounds[0]++;
        UE_info->eNB_UE_stats[CC_id][UE_id].snr = snr;
        UE_info->eNB_UE_stats[CC_id][UE_id].target_snr = target_snr;
        UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_mcs1=UE_template->pre_assigned_mcs_ul;
        UE_template->mcs_UL[harq_pid] = UE_template->pre_assigned_mcs_ul;//cmin (UE_template->pre_assigned_mcs_ul, openair_daq_vars.target_ue_ul_mcs); // adjust, based on user-defined MCS

        if (UE_template->pre_allocated_rb_table_index_ul >=0) {
          rb_table_index=UE_template->pre_allocated_rb_table_index_ul;
        } else {
          UE_template->mcs_UL[harq_pid]=10;//cmin (10, openair_daq_vars.target_ue_ul_mcs);
          rb_table_index=5; // for PHR
        }
        if( (UE_sched_ctrl->ul_scheduled | (1<<harq_pid))>0 ){
          UE_template->scheduled_ul_bytes -= UE_template->TBS_UL[harq_pid];

          if (UE_template->scheduled_ul_bytes < 0) {
            UE_template->scheduled_ul_bytes = 0;
          }
          UE_template->estimated_ul_buffer -= UE_template->TBS_UL[harq_pid];
          if (UE_template->estimated_ul_buffer < 0) {
            UE_template->estimated_ul_buffer = 0;
          }
        }

        UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_mcs2=UE_template->mcs_UL[harq_pid];
        UE_template->TBS_UL[harq_pid] = get_TBS_UL(UE_template->mcs_UL[harq_pid],rb_table[rb_table_index]);
        UE_info->eNB_UE_stats[CC_id][UE_id].total_rbs_used_rx+=rb_table[rb_table_index];
        UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_TBS=UE_template->TBS_UL[harq_pid];
        T(T_ENB_MAC_UE_UL_SCHEDULE, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP),
          T_INT(subframeP), T_INT(harq_pid), T_INT(UE_template->mcs_UL[harq_pid]), T_INT(first_rb[CC_id]), T_INT(rb_table[rb_table_index]),
          T_INT(UE_template->TBS_UL[harq_pid]), T_INT(ndi));

        if (mac_eNB_get_rrc_status(module_idP,rnti) < RRC_CONNECTED)
          LOG_D(MAC,"[eNB %d][PUSCH %d/%x] CC_id %d Frame %d subframeP %d Scheduled UE %d (mcs %d, first rb %d, nb_rb %d, rb_table_index %d, TBS %d, harq_pid %d)\n",
                module_idP,harq_pid,rnti,CC_id,frameP,subframeP,UE_id,UE_template->mcs_UL[harq_pid],
                first_rb[CC_id],rb_table[rb_table_index],
                rb_table_index,UE_template->TBS_UL[harq_pid],harq_pid);

        // bad indices : 20 (40 PRB), 21 (45 PRB), 22 (48 PRB)
        //store for possible retransmission
        UE_template->nb_rb_ul[harq_pid]    = rb_table[rb_table_index];
        UE_template->first_rb_ul[harq_pid] = first_rb[CC_id];
        UE_template->cqi_req[harq_pid] = cqi_req;
        UE_sched_ctrl->ul_scheduled |= (1<<harq_pid);

        if (UE_id == UE_info->list.head)
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_SCHEDULED,UE_sched_ctrl->ul_scheduled);

        // adjust total UL buffer status by TBS, wait for UL sdus to do final update
        /*LOG_D(MAC,"[eNB %d] CC_id %d UE %d/%x : adjusting ul_total_buffer, old %d, TBS %d\n", module_idP,CC_id,UE_id,rnti,UE_template->ul_total_buffer,UE_template->TBS_UL[harq_pid]);
        if (UE_template->ul_total_buffer > UE_template->TBS_UL[harq_pid])
          UE_template->ul_total_buffer -= UE_template->TBS_UL[harq_pid];
        else
          UE_template->ul_total_buffer = 0;
        LOG_D(MAC,"ul_total_buffer, new %d\n", UE_template->ul_total_buffer);*/
        // Cyclic shift for DM RS
        cshift = 0;// values from 0 to 7 can be used for mapping the cyclic shift (36.211 , Table 5.5.2.1.1-1)
        // save it for a potential retransmission
        UE_template->cshift[harq_pid] = cshift;
        hi_dci0_pdu                                                         = &hi_dci0_req->hi_dci0_pdu_list[hi_dci0_req->number_of_dci+hi_dci0_req->number_of_hi];
        memset((void *)hi_dci0_pdu,0,sizeof(nfapi_hi_dci0_request_pdu_t));
        hi_dci0_pdu->pdu_type                                               = NFAPI_HI_DCI0_DCI_PDU_TYPE;
        hi_dci0_pdu->pdu_size                                               = 2+sizeof(nfapi_hi_dci0_dci_pdu);
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.tl.tag                            = NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dci_format                        = NFAPI_UL_DCI_FORMAT_0;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.aggregation_level                 = aggregation;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti                              = rnti;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.transmission_power                = 6000;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.resource_block_start              = first_rb[CC_id];
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.number_of_resource_block          = rb_table[rb_table_index];
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.mcs_1                             = UE_template->mcs_UL[harq_pid];
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.cyclic_shift_2_for_drms           = cshift;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.frequency_hopping_enabled_flag    = 0;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.new_data_indication_1             = ndi;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.tpc                               = tpc;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.cqi_csi_request                   = cqi_req;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dl_assignment_index               = UE_template->DAI_ul[sched_subframeP];
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.harq_pid                          = harq_pid;
        hi_dci0_req->number_of_dci++;
        hi_dci0_req->sfnsf = sfnsf_add_subframe(sched_frame, sched_subframeP, 0); //(frameP, subframeP, 4)
        hi_dci0_req->tl.tag = NFAPI_HI_DCI0_REQUEST_BODY_TAG;
        nfapi_hi_dci0_request_t        *nfapi_hi_dci0_req = &eNB->HI_DCI0_req[CC_id][subframeP];
        nfapi_hi_dci0_req->sfn_sf = frameP<<4|subframeP; // sfnsf_add_subframe(sched_frame, sched_subframeP, 0); // sunday!
        nfapi_hi_dci0_req->header.message_id = NFAPI_HI_DCI0_REQUEST;
        LOG_D(MAC,"[PUSCH %d] Frame %d, Subframe %d: Adding UL CONFIG.Request for UE %d/%x, ulsch_frame %d, ulsch_subframe %d mcs %d first_rb %d num_rb %d round %d mcs %d sinr %d bler %lf\n",
              harq_pid,frameP,subframeP,UE_id,rnti,sched_frame,sched_subframeP,UE_template->mcs_UL[harq_pid],first_rb[CC_id],rb_table[rb_table_index],0,UE_template->mcs_UL[harq_pid],UE_sched_ctrl->pusch_snr_avg[CC_id],UE_sched_ctrl->pusch_bler[CC_id]);
        ul_req_index = 0;
        dlsch_flag = 0;

        for(ul_req_index = 0; ul_req_index < ul_req_tmp->number_of_pdus; ul_req_index++) {
          if((ul_req_tmp->ul_config_pdu_list[ul_req_index].pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE) &&
              (ul_req_tmp->ul_config_pdu_list[ul_req_index].uci_harq_pdu.ue_information.ue_information_rel8.rnti == rnti)) {
            dlsch_flag = 1;
            LOG_D(MAC,"Frame %d, Subframe %d:rnti %x ul_req_index %d Switched UCI HARQ to ULSCH HARQ(first)\n",frameP,subframeP,rnti,ul_req_index);
            break;
          }
        }

        // Add UL_config PDUs
        fill_nfapi_ulsch_config_request_rel8(&ul_req_tmp->ul_config_pdu_list[ul_req_index],
                                             cqi_req,
                                             cc,
                                             UE_template->physicalConfigDedicated,
                                             get_tmode(module_idP,CC_id,UE_id),
                                             eNB->ul_handle,
                                             rnti,
                                             first_rb[CC_id], // resource_block_start
                                             rb_table[rb_table_index], // number_of_resource_blocks
                                             UE_template->mcs_UL[harq_pid],
                                             cshift, // cyclic_shift_2_for_drms
                                             0, // frequency_hopping_enabled_flag
                                             0, // frequency_hopping_bits
                                             ndi, // new_data_indication
                                             0, // redundancy_version
                                             harq_pid, // harq_process_number
                                             0, // ul_tx_mode
                                             0, // current_tx_nb
                                             0, // n_srs
                                             get_TBS_UL(UE_template->mcs_UL[harq_pid],
                                                 rb_table[rb_table_index])
                                            );

        if (UE_template->rach_resource_type>0) { // This is a BL/CE UE allocation
          fill_nfapi_ulsch_config_request_emtc(&ul_req_tmp->ul_config_pdu_list[ul_req_index],
                                               UE_template->rach_resource_type>2 ? 2 : 1,
                                               1, //total_number_of_repetitions
                                               1, //repetition_number
                                               (frameP*10)+subframeP);
        }

        if(dlsch_flag == 1) {
          if(cqi_req == 1) {
            ul_req_tmp->ul_config_pdu_list[ul_req_index].pdu_type = NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE;
            ulsch_harq_information = &ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.harq_information;
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.tl.tag=
              NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG;
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.n_srs_initial = 0;    // last symbol not punctured
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.initial_number_of_resource_blocks = rb_table[rb_table_index];
          } else {
            ul_req_tmp->ul_config_pdu_list[ul_req_index].pdu_type = NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE;
            ulsch_harq_information = &ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.harq_information;
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.tl.tag =
              NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG;
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.n_srs_initial = 0;  // last symbol not punctured
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.initial_number_of_resource_blocks = rb_table[rb_table_index];
          }

          fill_nfapi_ulsch_harq_information(module_idP, CC_id,rnti, ulsch_harq_information,subframeP);
        } else {
          ul_req_tmp->number_of_pdus++;
        }

        eNB->ul_handle++;
        ul_req->header.message_id = NFAPI_UL_CONFIG_REQUEST;
        ul_req_tmp->tl.tag = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;
        uint16_t ul_sched_frame = sched_frame;
        uint16_t ul_sched_subframeP = sched_subframeP;
        add_subframe(&ul_sched_frame, &ul_sched_subframeP, 2);
        ul_req->sfn_sf = ul_sched_frame<<4|ul_sched_subframeP;
        add_ue_ulsch_info(module_idP,
                          CC_id,
                          UE_id,
                          subframeP,
                          S_UL_SCHEDULED);
        LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframeP %d: Generated ULSCH DCI for next UE_id %d, format 0\n", module_idP,CC_id,frameP,subframeP,UE_id);
        // increment first rb for next UE allocation
        first_rb[CC_id]+=rb_table[rb_table_index];

        if(ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority == SCH_UL_FIRST) {
          UE_template->scheduled_ul_bytes += get_TBS_UL(UE_template->mcs_UL[harq_pid],rb_table[rb_table_index]);
          UE_template->ul_SR = 0;
        }

        if((ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority == SCH_UL_INACTIVE) && (ULSCH_first_end == 0)) {
          ULSCH_first_end = 1;
          last_ulsch_ue_id[CC_id] = ulsch_ue_select[CC_id].list[ulsch_ue_num-1].UE_id;
        }

        if((ulsch_ue_num == ulsch_ue_select[CC_id].ue_num-1) && (ULSCH_first_end == 0)) {
          ULSCH_first_end = 1;
          last_ulsch_ue_id[CC_id] = ulsch_ue_select[CC_id].list[ulsch_ue_num].UE_id;
        }
      } else if (ulsch_ue_select[CC_id].list[ulsch_ue_num].ue_priority == SCH_UL_RETRANS) { // round > 0 => retransmission
        round = UE_sched_ctrl->round_UL[CC_id][harq_pid];
        T(T_ENB_MAC_UE_UL_SCHEDULE_RETRANSMISSION, T_INT(module_idP), T_INT(CC_id), T_INT(rnti), T_INT(frameP),
          T_INT(subframeP), T_INT(harq_pid), T_INT(UE_template->mcs_UL[harq_pid]), T_INT(ulsch_ue_select[CC_id].list[ulsch_ue_num].start_rb), T_INT(ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb),
          T_INT(round));
        UE_info->eNB_UE_stats[CC_id][UE_id].snr = snr;
        UE_info->eNB_UE_stats[CC_id][UE_id].target_snr = target_snr;

        uint8_t mcs_rv = 0;

        if(rvidx_tab[round&3]==1) {
          mcs_rv = 29;
        } else if(rvidx_tab[round&3]==2) {
          mcs_rv = 30;
        } else if(rvidx_tab[round&3]==3) {
          mcs_rv = 31;
        }

        UE_info->eNB_UE_stats[CC_id][UE_id].ulsch_TBS=UE_template->TBS_UL[harq_pid];

        if (mac_eNB_get_rrc_status(module_idP,rnti) < RRC_CONNECTED)
          LOG_D(MAC,"[eNB %d][PUSCH %d/%x] CC_id %d Frame %d subframeP %d Scheduled UE %d (mcs %d, first rb %d, nb_rb %d, TBS %d, harq_pid %d)\n",
                module_idP,harq_pid,rnti,CC_id,frameP,subframeP,UE_id,mcs_rv,first_rb[CC_id],ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb,UE_template->TBS_UL[harq_pid],harq_pid);

        // bad indices : 20 (40 PRB), 21 (45 PRB), 22 (48 PRB)
        //store for possible retransmission
        UE_template->nb_rb_ul[harq_pid]    = ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb;
        UE_template->first_rb_ul[harq_pid] = ulsch_ue_select[CC_id].list[ulsch_ue_num].start_rb;
        cqi_req = UE_template->cqi_req[harq_pid];
        UE_sched_ctrl->ul_scheduled |= (1<<harq_pid);
        // Cyclic shift for DM RS
        cshift = 0;// values from 0 to 7 can be used for mapping the cyclic shift (36.211 , Table 5.5.2.1.1-1)
        hi_dci0_pdu                                                         = &hi_dci0_req->hi_dci0_pdu_list[hi_dci0_req->number_of_dci+hi_dci0_req->number_of_hi];
        memset((void *)hi_dci0_pdu,0,sizeof(nfapi_hi_dci0_request_pdu_t));
        hi_dci0_pdu->pdu_type                                               = NFAPI_HI_DCI0_DCI_PDU_TYPE;
        hi_dci0_pdu->pdu_size                                               = 2+sizeof(nfapi_hi_dci0_dci_pdu);
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.tl.tag                            = NFAPI_HI_DCI0_REQUEST_DCI_PDU_REL8_TAG;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dci_format                        = NFAPI_UL_DCI_FORMAT_0;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.aggregation_level                 = aggregation;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti                              = rnti;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.transmission_power                = 6000;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.resource_block_start              = ulsch_ue_select[CC_id].list[ulsch_ue_num].start_rb;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.number_of_resource_block          = ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.mcs_1                             = mcs_rv;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.cyclic_shift_2_for_drms           = cshift;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.frequency_hopping_enabled_flag    = 0;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.new_data_indication_1             = UE_template->oldNDI_UL[harq_pid];
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.tpc                               = tpc;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.cqi_csi_request                   = cqi_req;
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dl_assignment_index               = UE_template->DAI_ul[sched_subframeP];
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.harq_pid                          = harq_pid;
        hi_dci0_req->number_of_dci++;
        // Add UL_config PDUs
        LOG_D(MAC,"[PUSCH %d] Frame %d, Subframe %d: Adding UL CONFIG.Request for UE %d/%x, ulsch_frame %d, ulsch_subframe %d mcs %d first_rb %d num_rb %d round %d mcs %d sinr %d bler %lf\n",
              harq_pid,frameP,subframeP,UE_id,rnti,sched_frame,sched_subframeP,mcs_rv,ulsch_ue_select[CC_id].list[ulsch_ue_num].start_rb,ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb,UE_sched_ctrl->round_UL[CC_id][harq_pid],UE_template->mcs_UL[harq_pid],UE_sched_ctrl->pusch_snr_avg[CC_id],UE_sched_ctrl->pusch_bler[CC_id]);

        ul_req_index = 0;
        dlsch_flag = 0;

        for(ul_req_index = 0; ul_req_index < ul_req_tmp->number_of_pdus; ul_req_index++) {
          if((ul_req_tmp->ul_config_pdu_list[ul_req_index].pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE) &&
              (ul_req_tmp->ul_config_pdu_list[ul_req_index].uci_harq_pdu.ue_information.ue_information_rel8.rnti == rnti)) {
            dlsch_flag = 1;
            LOG_D(MAC,"Frame %d, Subframe %d:rnti %x ul_req_index %d Switched UCI HARQ to ULSCH HARQ(phich)\n",frameP,subframeP,rnti,ul_req_index);
            break;
          }
        }

        fill_nfapi_ulsch_config_request_rel8(&ul_req_tmp->ul_config_pdu_list[ul_req_index],
                                             cqi_req,
                                             cc,
                                             UE_template->physicalConfigDedicated,
                                             get_tmode(module_idP,CC_id,UE_id),
                                             eNB->ul_handle,
                                             rnti,
                                             ulsch_ue_select[CC_id].list[ulsch_ue_num].start_rb, // resource_block_start
                                             ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb, // number_of_resource_blocks
                                             UE_template->mcs_UL[harq_pid],
                                             cshift, // cyclic_shift_2_for_drms
                                             0, // frequency_hopping_enabled_flag
                                             0, // frequency_hopping_bits
                                             UE_template->oldNDI_UL[harq_pid], // new_data_indication
                                             rvidx_tab[round&3], // redundancy_version
                                             harq_pid, // harq_process_number
                                             0, // ul_tx_mode
                                             0, // current_tx_nb
                                             0, // n_srs
                                             UE_template->TBS_UL[harq_pid]
                                            );

        if (UE_template->rach_resource_type>0) { // This is a BL/CE UE allocation
          fill_nfapi_ulsch_config_request_emtc(&ul_req_tmp->ul_config_pdu_list[ul_req_index],
                                               UE_template->rach_resource_type>2 ? 2 : 1,
                                               1, //total_number_of_repetitions
                                               1, //repetition_number
                                               (frameP*10)+subframeP);
        }

        if(dlsch_flag == 1) {
          if(cqi_req == 1) {
            ul_req_tmp->ul_config_pdu_list[ul_req_index].pdu_type = NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE;
            ulsch_harq_information = &ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.harq_information;
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.tl.tag=
              NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG;
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.n_srs_initial = 0;    // last symbol not punctured
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_cqi_harq_ri_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.initial_number_of_resource_blocks =
              ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb;
          } else {
            ul_req_tmp->ul_config_pdu_list[ul_req_index].pdu_type = NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE;
            ulsch_harq_information = &ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.harq_information;
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.tl.tag =
              NFAPI_UL_CONFIG_REQUEST_INITIAL_TRANSMISSION_PARAMETERS_REL8_TAG;
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.n_srs_initial = 0;  // last symbol not punctured
            ul_req_tmp->ul_config_pdu_list[ul_req_index].ulsch_harq_pdu.initial_transmission_parameters.initial_transmission_parameters_rel8.initial_number_of_resource_blocks =
              ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb;
          }

          fill_nfapi_ulsch_harq_information(module_idP, CC_id,rnti, ulsch_harq_information,subframeP);
        } else {
          ul_req_tmp->number_of_pdus++;
        }

        eNB->ul_handle++;
        ul_req->header.message_id = NFAPI_UL_CONFIG_REQUEST;
        ul_req_tmp->tl.tag = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;
        ul_req->sfn_sf = sched_frame<<4|sched_subframeP;
        LOG_D(MAC,"[eNB %d] CC_id %d Frame %d, subframeP %d: Generated ULSCH DCI for next UE_id %d, format 0(round >0)\n", module_idP,CC_id,frameP,subframeP,UE_id);
        // increment first rb for next UE allocation
        first_rb[CC_id]+=ulsch_ue_select[CC_id].list[ulsch_ue_num].nb_rb;
      }
    } // loop over UE_id
  } // loop of CC_id
}
