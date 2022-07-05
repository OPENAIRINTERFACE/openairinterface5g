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

/*! \file flexran_agent_ran_api.c
 * \brief FlexRAN RAN API abstraction
 * \author N. Nikaein, X. Foukas, S. SHARIAT BAGHERI and R. Schmidt
 * \date 2017
 * \version 0.1
 */

#include <dlfcn.h>
#include "flexran_agent_ran_api.h"
#include "s1ap_eNB_ue_context.h"
#include "s1ap_eNB_management_procedures.h"
#include "openair2/LAYER2/MAC/slicing/slicing.h"

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;

static inline int phy_is_present(mid_t mod_id, uint8_t cc_id) {
  return RC.eNB && RC.eNB[mod_id] && RC.eNB[mod_id][cc_id];
}

static inline int mac_is_present(mid_t mod_id) {
  return RC.mac && RC.mac[mod_id];
}

static inline int rrc_is_present(mid_t mod_id) {
  return RC.rrc && RC.rrc[mod_id];
}

uint32_t flexran_get_current_time_ms(mid_t mod_id, int subframe_flag) {
  if (!mac_is_present(mod_id)) return 0;

  if (subframe_flag == 1)
    return RC.mac[mod_id]->frame*10 + RC.mac[mod_id]->subframe;
  else
    return RC.mac[mod_id]->frame*10;
}

frame_t flexran_get_current_frame(mid_t mod_id) {
  if (!mac_is_present(mod_id)) return 0;

  //  #warning "SFN will not be in [0-1023] when oaisim is used"
  return RC.mac[mod_id]->frame;
}

frame_t flexran_get_current_system_frame_num(mid_t mod_id) {
  return flexran_get_current_frame(mod_id) % 1024;
}

sub_frame_t flexran_get_current_subframe(mid_t mod_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->subframe;
}

uint32_t flexran_get_sfn_sf(mid_t mod_id) {
  if (!mac_is_present(mod_id)) return 0;

  return flexran_get_current_frame(mod_id) * 10 + flexran_get_current_subframe(mod_id);
}

uint16_t flexran_get_future_sfn_sf(mid_t mod_id, int ahead_of_time) {
  if (!mac_is_present(mod_id)) return 0;

  frame_t frame = flexran_get_current_system_frame_num(mod_id);
  sub_frame_t subframe = flexran_get_current_subframe(mod_id);
  uint16_t sfn_sf, frame_mask, sf_mask;
  int additional_frames;
  subframe = (subframe + ahead_of_time) % 10;

  if (subframe < flexran_get_current_subframe(mod_id))
    frame = (frame + 1) % 1024;

  additional_frames = ahead_of_time / 10;
  frame = (frame + additional_frames) % 1024;
  frame_mask = (1 << 12) - 1;
  sf_mask = (1 << 4) - 1;
  sfn_sf = (subframe & sf_mask) | ((frame & frame_mask) << 4);
  return sfn_sf;
}

int flexran_get_mac_num_ues(mid_t mod_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.num_UEs;
}

int flexran_get_num_ue_lcs(mid_t mod_id, mid_t ue_id) {
  if (!mac_is_present(mod_id)) return 0;

  // Not sure whether this is needed: if (!rrc_is_present(mod_id)) return 0;
  const rnti_t rnti = flexran_get_mac_ue_crnti(mod_id, ue_id);
  const int s = mac_eNB_get_rrc_status(mod_id, rnti);

  if (s < RRC_CONNECTED)
    return 0;
  else if (s == RRC_CONNECTED)
    return 1;
  else
    return 3;
}

int flexran_get_mac_ue_id_rnti(mid_t mod_id, rnti_t rnti) {
  int n;

  if (!mac_is_present(mod_id)) return 0;

  /* get the (active) UE with RNTI i */
  for (n = 0; n < MAX_MOBILES_PER_ENB; ++n) {
    if (RC.mac[mod_id]->UE_info.active[n] == true
        && rnti == UE_RNTI(mod_id, n)) {
      return n;
    }
  }

  return 0;
}

int flexran_get_mac_ue_id(mid_t mod_id, int i) {
  int n;

  if (!mac_is_present(mod_id)) return 0;

  /* get the (i+1)'th active UE */
  for (n = 0; n < MAX_MOBILES_PER_ENB; ++n) {
    if (RC.mac[mod_id]->UE_info.active[n] == true) {
      if (i == 0)
        return n;

      --i;
    }
  }

  return 0;
}

rnti_t flexran_get_mac_ue_crnti(mid_t mod_id, mid_t ue_id) {
  if (!mac_is_present(mod_id)) return 0;

  return UE_RNTI(mod_id, ue_id);
}

int flexran_get_ue_bsr_ul_buffer_info(mid_t mod_id, mid_t ue_id, lcid_t lcid) {
  if (!mac_is_present(mod_id)) return -1;

  return RC.mac[mod_id]->UE_info.UE_template[UE_PCCID(mod_id, ue_id)][ue_id].ul_buffer_info[lcid];
}

int8_t flexran_get_ue_phr(mid_t mod_id, mid_t ue_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.UE_template[UE_PCCID(mod_id, ue_id)][ue_id].phr_info;
}

uint8_t flexran_get_ue_wcqi(mid_t mod_id, mid_t ue_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.UE_sched_ctrl[ue_id].dl_cqi[0];
}

rlc_buffer_occupancy_t flexran_get_tx_queue_size(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id) {
  if (!mac_is_present(mod_id)) return 0;

  rnti_t rnti = flexran_get_mac_ue_crnti(mod_id, ue_id);
  frame_t frame = flexran_get_current_frame(mod_id);
  sub_frame_t subframe = flexran_get_current_subframe(mod_id);
  mac_rlc_status_resp_t rlc_status = mac_rlc_status_ind(mod_id,rnti, mod_id, frame, subframe, ENB_FLAG_YES,MBMS_FLAG_NO, channel_id, 0, 0
                                                       );
  return rlc_status.bytes_in_buffer;
}

rlc_buffer_occupancy_t flexran_get_num_pdus_buffer(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id) {
  if (!mac_is_present(mod_id)) return 0;

  rnti_t rnti = flexran_get_mac_ue_crnti(mod_id,ue_id);
  frame_t frame = flexran_get_current_frame(mod_id);
  sub_frame_t subframe = flexran_get_current_subframe(mod_id);
  mac_rlc_status_resp_t rlc_status = mac_rlc_status_ind(mod_id,rnti, mod_id, frame, subframe, ENB_FLAG_YES,MBMS_FLAG_NO, channel_id, 0, 0
                                                       );
  return rlc_status.pdus_in_buffer;
}

frame_t flexran_get_hol_delay(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id) {
  if (!mac_is_present(mod_id)) return 0;

  rnti_t rnti = flexran_get_mac_ue_crnti(mod_id,ue_id);
  frame_t frame = flexran_get_current_frame(mod_id);
  sub_frame_t subframe = flexran_get_current_subframe(mod_id);
  mac_rlc_status_resp_t rlc_status = mac_rlc_status_ind(mod_id, rnti, mod_id, frame, subframe, ENB_FLAG_YES, MBMS_FLAG_NO, channel_id, 0, 0
                                                       );
  return rlc_status.head_sdu_creation_time;
}

int32_t flexran_get_TA(mid_t mod_id, mid_t ue_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  int32_t tau = RC.eNB[mod_id][cc_id]->UE_stats[ue_id].timing_advance_update;

  switch (flexran_get_N_RB_DL(mod_id, cc_id)) {
    case 6:
      return tau;

    case 15:
      return tau / 2;

    case 25:
      return tau / 4;

    case 50:
      return tau / 8;

    case 75:
      return tau / 12;

    case 100:
      if (flexran_get_threequarter_fs(mod_id, cc_id) == 0)
        return tau / 16;
      else
        return tau / 12;

    default:
      return 0;
  }
}

uint32_t flexran_get_total_size_dl_mac_sdus(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].total_sdu_bytes;
}

uint32_t flexran_get_total_size_ul_mac_sdus(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  uint64_t bytes = 0;

  for (int i = 0; i < NB_RB_MAX; ++i) {
    bytes += RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].num_bytes_rx[i];
  }

  return bytes;
}

uint32_t flexran_get_TBS_dl(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].TBS;
}

uint32_t flexran_get_TBS_ul(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].ulsch_TBS;
}

uint16_t flexran_get_num_prb_retx_dl_per_ue(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].rbs_used_retx;
}

uint32_t flexran_get_num_prb_retx_ul_per_ue(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].rbs_used_retx_rx;
}

uint16_t flexran_get_num_prb_dl_tx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].rbs_used;
}

uint16_t flexran_get_num_prb_ul_rx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].rbs_used_rx;
}

uint8_t flexran_get_ue_wpmi(mid_t mod_id, mid_t ue_id, uint8_t cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.UE_sched_ctrl[ue_id].periodic_wideband_pmi[cc_id];
}

uint8_t flexran_get_mcs1_dl(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].dlsch_mcs1;
}

uint8_t flexran_get_mcs2_dl(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].dlsch_mcs2;
}

uint8_t flexran_get_mcs1_ul(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].ulsch_mcs1;
}

uint8_t flexran_get_mcs2_ul(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].ulsch_mcs2;
}

uint32_t flexran_get_total_prb_dl_tx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].total_rbs_used;
}

uint32_t flexran_get_total_prb_ul_rx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].total_rbs_used_rx;
}

uint32_t flexran_get_total_num_pdu_dl(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].total_num_pdus;
}

uint32_t flexran_get_total_num_pdu_ul(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].total_num_pdus_rx;
}

uint64_t flexran_get_total_TBS_dl(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].total_pdu_bytes;
}

uint64_t flexran_get_total_TBS_ul(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].total_ulsch_TBS;
}

int flexran_get_harq_round(mid_t mod_id, uint8_t cc_id, mid_t ue_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].harq_round;
}

uint32_t flexran_get_num_mac_sdu_tx(mid_t mod_id, mid_t ue_id, int cc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].num_mac_sdu_tx;
}

unsigned char flexran_get_mac_sdu_lcid_index(mid_t mod_id, mid_t ue_id, int cc_id, int index) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].lcid_sdu[index];
}

uint32_t flexran_get_mac_sdu_size(mid_t mod_id, mid_t ue_id, int cc_id, int lcid) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.eNB_UE_stats[cc_id][ue_id].sdu_length_tx[lcid];
}


/* TODO needs to be revised */
void flexran_update_TA(mid_t mod_id, mid_t ue_id, uint8_t cc_id) {
  /*
    UE_info_t *UE_info=&eNB_mac_inst[mod_id].UE_info;
    UE_sched_ctrl *ue_sched_ctl = &UE_info->UE_sched_ctrl[ue_id];

    if (ue_sched_ctl->ta_timer == 0) {

      // WE SHOULD PROTECT the eNB_UE_stats with a mutex here ...
      //    LTE_eNB_UE_stats    *eNB_UE_stats = mac_xface->get_eNB_UE_stats(mod_id, CC_id, rnti);
      //ue_sched_ctl->ta_timer          = 20; // wait 20 subframes before taking TA measurement from PHY
      ue_sched_ctl->ta_update = flexran_get_TA(mod_id, ue_id, CC_id);

      // clear the update in case PHY does not have a new measurement after timer expiry
      //    eNB_UE_stats->timing_advance_update       = 0;
    } else {
      ue_sched_ctl->ta_timer--;
      ue_sched_ctl->ta_update         = 0;  // don't trigger a timing advance command
    }
  #warning "Implement flexran_update_TA() in RAN API"
  */
}

/* TODO needs to be revised, looks suspicious: why do we need UE stats? */
int flexran_get_MAC_CE_bitmap_TA(mid_t mod_id, mid_t ue_id, uint8_t cc_id) {
  /*
  #warning "Implement flexran_get_MAC_CE_bitmap_TA() in RAN API"
  */
  if (!phy_is_present(mod_id, cc_id)) return 0;

  /* UE_stats can not be null, they are an array in RC
  LTE_eNB_UE_stats *eNB_UE_stats = mac_xface->get_eNB_UE_stats(mod_id,CC_id,rnti);

  if (eNB_UE_stats == NULL) {
    return 0;
  }
  */

  if (flexran_get_TA(mod_id, ue_id, cc_id) != 0) {
    return PROTOCOL__FLEX_CE_TYPE__FLPCET_TA;
  } else {
    return 0;
  }
}

int flexran_get_active_CC(mid_t mod_id, mid_t ue_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.numactiveCCs[ue_id];
}

uint8_t flexran_get_current_RI(mid_t mod_id, mid_t ue_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->UE_stats[ue_id].rank;
}

int flexran_get_tpc(mid_t mod_id, mid_t ue_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  /* before: tested that UL_rssi != NULL and set parameter ([0]), but it is a
   * static array -> target_rx_power is useless in old ifs?! */
  int pCCid = UE_PCCID(mod_id,ue_id);
  int32_t target_rx_power = RC.eNB[mod_id][pCCid]->frame_parms.ul_power_control_config_common.p0_NominalPUSCH;
  int32_t normalized_rx_power = RC.eNB[mod_id][cc_id]->UE_stats[ue_id].UL_rssi[0];
  int tpc;

  if (normalized_rx_power > target_rx_power + 1)
    tpc = 0;  //-1
  else if (normalized_rx_power < target_rx_power - 1)
    tpc = 2;  //+1
  else
    tpc = 1;  //0

  return tpc;
}

int flexran_get_harq(mid_t       mod_id,
                     uint8_t     cc_id,
                     mid_t       ue_id,
                     frame_t     frame,
                     sub_frame_t subframe,
                     uint8_t    *pid,
                     uint8_t    *round,
                     uint8_t     harq_flag) {
  /* TODO: Add int TB in function parameters to get the status of the second
   * TB. This can be done to by editing in get_ue_active_harq_pid function in
   * line 272 file: phy_procedures_lte_eNB.c to add DLSCH_ptr =
   * PHY_vars_eNB_g[Mod_id][CC_id]->dlsch_eNB[(uint32_t)UE_id][1];*/
  /* TODO IMPLEMENT */
  /*
  uint8_t harq_pid;
  uint8_t harq_round;

  if (mac_xface_not_ready()) return 0 ;

  uint16_t rnti = flexran_get_mac_ue_crnti(mod_id,ue_id);
  if (harq_flag == openair_harq_DL){

      mac_xface->get_ue_active_harq_pid(mod_id,CC_id,rnti,frame,subframe,&harq_pid,&harq_round,openair_harq_DL);

   } else if (harq_flag == openair_harq_UL){

     mac_xface->get_ue_active_harq_pid(mod_id,CC_id,rnti,frame,subframe,&harq_pid,round,openair_harq_UL);
   }
   else {

      LOG_W(FLEXRAN_AGENT,"harq_flag is not recongnized");
   }


  *pid = harq_pid;
  *round = harq_round;*/
  /* if (round > 0) { */
  /*   *status = 1; */
  /* } else { */
  /*   *status = 0; */
  /* } */
  /*return *round;*/
  /*
  #warning "Implement flexran_get_harq() in RAN API"
  */
  return 0;
}

int32_t flexran_get_p0_pucch_dbm(mid_t mod_id, mid_t ue_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->UE_stats[ue_id].Po_PUCCH_dBm;
}

int8_t flexran_get_p0_nominal_pucch(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.ul_power_control_config_common.p0_NominalPUCCH;
}

int32_t flexran_get_p0_pucch_status(mid_t mod_id, mid_t ue_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->UE_stats[ue_id].Po_PUCCH_update;
}

int flexran_update_p0_pucch(mid_t mod_id, mid_t ue_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  RC.eNB[mod_id][cc_id]->UE_stats[ue_id].Po_PUCCH_update = 0;
  return 0;
}


/*
 * ************************************
 * Get Messages for eNB Configuration Reply
 * ************************************
 */
uint8_t flexran_get_threequarter_fs(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.threequarter_fs;
}


uint8_t flexran_get_hopping_offset(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.pusch_config_common.pusch_HoppingOffset;
}

Protocol__FlexHoppingMode flexran_get_hopping_mode(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return -1;

  switch (RC.eNB[mod_id][cc_id]->frame_parms.pusch_config_common.hoppingMode) {
    case interSubFrame:
      return PROTOCOL__FLEX_HOPPING_MODE__FLHM_INTER;

    case intraAndInterSubFrame:
      return PROTOCOL__FLEX_HOPPING_MODE__FLHM_INTERINTRA;
  }

  return -1;
}

uint8_t flexran_get_n_SB(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.pusch_config_common.n_SB;
}

Protocol__FlexQam flexran_get_enable64QAM(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  if (RC.eNB[mod_id][cc_id]->frame_parms.pusch_config_common.enable64QAM == true)
    return PROTOCOL__FLEX_QAM__FLEQ_MOD_64QAM;
  else
    return PROTOCOL__FLEX_QAM__FLEQ_MOD_16QAM;
}

Protocol__FlexPhichDuration flexran_get_phich_duration(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return -1;

  switch (RC.eNB[mod_id][cc_id]->frame_parms.phich_config_common.phich_duration) {
    case normal:
      return PROTOCOL__FLEX_PHICH_DURATION__FLPD_NORMAL;

    case extended:
      return PROTOCOL__FLEX_PHICH_DURATION__FLPD_EXTENDED;
  }

  return -1;
}

Protocol__FlexPhichResource flexran_get_phich_resource(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return -1;

  switch (RC.eNB[mod_id][cc_id]->frame_parms.phich_config_common.phich_resource) {
    case oneSixth:
      return PROTOCOL__FLEX_PHICH_RESOURCE__FLPR_ONE_SIXTH;

    case half:
      return PROTOCOL__FLEX_PHICH_RESOURCE__FLPR_HALF;

    case one:
      return PROTOCOL__FLEX_PHICH_RESOURCE__FLPR_ONE;

    case two:
      return PROTOCOL__FLEX_PHICH_RESOURCE__FLPR_TWO;
  }

  return -1;
}

uint16_t flexran_get_n1pucch_an(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.pucch_config_common.n1PUCCH_AN;
}

uint8_t flexran_get_nRB_CQI(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.pucch_config_common.nRB_CQI;
}

uint8_t flexran_get_deltaPUCCH_Shift(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.pucch_config_common.deltaPUCCH_Shift;
}

uint8_t flexran_get_prach_ConfigIndex(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
}

uint8_t flexran_get_prach_FreqOffset(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset;
}

uint8_t flexran_get_maxHARQ_Msg3Tx(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.maxHARQ_Msg3Tx;
}

Protocol__FlexUlCyclicPrefixLength flexran_get_ul_cyclic_prefix_length(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return -1;

  switch (RC.eNB[mod_id][cc_id]->frame_parms.Ncp_UL) {
    case EXTENDED:
      return PROTOCOL__FLEX_UL_CYCLIC_PREFIX_LENGTH__FLUCPL_EXTENDED;

    case NORMAL:
      return PROTOCOL__FLEX_UL_CYCLIC_PREFIX_LENGTH__FLUCPL_NORMAL;
  }

  return -1;
}

Protocol__FlexDlCyclicPrefixLength flexran_get_dl_cyclic_prefix_length(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return -1;

  switch (RC.eNB[mod_id][cc_id]->frame_parms.Ncp) {
    case EXTENDED:
      return PROTOCOL__FLEX_DL_CYCLIC_PREFIX_LENGTH__FLDCPL_EXTENDED;

    case NORMAL:
      return PROTOCOL__FLEX_DL_CYCLIC_PREFIX_LENGTH__FLDCPL_NORMAL;
  }

  return -1;
}

uint16_t flexran_get_cell_id(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.Nid_cell;
}

uint8_t flexran_get_srs_BandwidthConfig(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.soundingrs_ul_config_common.srs_BandwidthConfig;
}

uint8_t flexran_get_srs_SubframeConfig(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.soundingrs_ul_config_common.srs_SubframeConfig;
}

uint8_t flexran_get_srs_MaxUpPts(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.soundingrs_ul_config_common.srs_MaxUpPts;
}

uint8_t flexran_get_N_RB_DL(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.N_RB_DL;
}

uint8_t flexran_get_N_RB_UL(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.N_RB_UL;
}

uint8_t flexran_get_N_RBG(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.N_RBG;
}

uint8_t flexran_get_subframe_assignment(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.tdd_config;
}

uint8_t flexran_get_special_subframe_assignment(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.tdd_config_S;
}

long flexran_get_ra_ResponseWindowSize(mid_t mod_id, uint8_t cc_id) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->configuration.radioresourceconfig[cc_id].rach_raResponseWindowSize;
}

long flexran_get_mac_ContentionResolutionTimer(mid_t mod_id, uint8_t cc_id) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->configuration.radioresourceconfig[cc_id].rach_macContentionResolutionTimer;
}

Protocol__FlexDuplexMode flexran_get_duplex_mode(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  switch (RC.eNB[mod_id][cc_id]->frame_parms.frame_type) {
    case TDD:
      return PROTOCOL__FLEX_DUPLEX_MODE__FLDM_TDD;

    case FDD:
      return PROTOCOL__FLEX_DUPLEX_MODE__FLDM_FDD;

    default:
      return -1;
  }
}

long flexran_get_si_window_length(mid_t mod_id, uint8_t cc_id) {
  if (!rrc_is_present(mod_id) || !RC.rrc[mod_id]->carrier[cc_id].sib1) return 0;

  return RC.rrc[mod_id]->carrier[cc_id].sib1->si_WindowLength;
}

uint8_t flexran_get_sib1_length(mid_t mod_id, uint8_t cc_id) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->carrier[cc_id].sizeof_SIB1;
}

uint8_t flexran_get_num_pdcch_symb(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->pdcch_vars[0].num_pdcch_symbols;
}



/*
 * ************************************
 * Get Messages for UE Configuration Reply
 * ************************************
 */
int flexran_get_rrc_num_ues(mid_t mod_id) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->Nb_ue;
}

rnti_t flexran_get_rrc_rnti_nth_ue(mid_t mod_id, int index) {
  if (!rrc_is_present(mod_id)) return 0;

  struct rrc_eNB_ue_context_s *ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_ue_tree_s, &RC.rrc[mod_id]->rrc_ue_head) {
    if (index == 0) return ue_context_p->ue_context.rnti;

    --index;
  }
  return 0;
}

int flexran_get_rrc_rnti_list(mid_t mod_id, rnti_t *list, int max_list) {
  if (!rrc_is_present(mod_id)) return 0;

  int n = 0;
  struct rrc_eNB_ue_context_s *ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_ue_tree_s, &RC.rrc[mod_id]->rrc_ue_head) {
    if (n >= max_list) break;

    list[n] = ue_context_p->ue_context.rnti;
    ++n;
  }
  return n;
}

LTE_TimeAlignmentTimer_t  flexran_get_time_alignment_timer(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.mac_MainConfig) return -1;

  return ue_context_p->ue_context.mac_MainConfig->timeAlignmentTimerDedicated;
}

Protocol__FlexMeasGapConfigPattern flexran_get_meas_gap_config(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measGapConfig) return -1;

  if (ue_context_p->ue_context.measGapConfig->present != LTE_MeasGapConfig_PR_setup) return -1;

  switch (ue_context_p->ue_context.measGapConfig->choice.setup.gapOffset.present) {
    case LTE_MeasGapConfig__setup__gapOffset_PR_gp0:
      return PROTOCOL__FLEX_MEAS_GAP_CONFIG_PATTERN__FLMGCP_GP1;

    case LTE_MeasGapConfig__setup__gapOffset_PR_gp1:
      return PROTOCOL__FLEX_MEAS_GAP_CONFIG_PATTERN__FLMGCP_GP2;

    default:
      return PROTOCOL__FLEX_MEAS_GAP_CONFIG_PATTERN__FLMGCP_OFF;
  }
}


long flexran_get_meas_gap_config_offset(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measGapConfig) return -1;

  if (ue_context_p->ue_context.measGapConfig->present != LTE_MeasGapConfig_PR_setup) return -1;

  switch (ue_context_p->ue_context.measGapConfig->choice.setup.gapOffset.present) {
    case LTE_MeasGapConfig__setup__gapOffset_PR_gp0:
      return ue_context_p->ue_context.measGapConfig->choice.setup.gapOffset.choice.gp0;

    case LTE_MeasGapConfig__setup__gapOffset_PR_gp1:
      return ue_context_p->ue_context.measGapConfig->choice.setup.gapOffset.choice.gp1;

    default:
      return -1;
  }
}

uint8_t flexran_get_rrc_status(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return 0;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return RRC_INACTIVE;

  return ue_context_p->ue_context.StatusRrc;
}

uint64_t flexran_get_ue_aggregated_max_bitrate_dl(mid_t mod_id, mid_t ue_id) {
  if (!mac_is_present(mod_id)) return 0;

  return 0; //RC.mac[mod_id]->UE_info.UE_sched_ctrl[ue_id].ue_AggregatedMaximumBitrateDL;
}

uint64_t flexran_get_ue_aggregated_max_bitrate_ul(mid_t mod_id, mid_t ue_id) {
  if (!mac_is_present(mod_id)) return 0;

  return 0; //RC.mac[mod_id]->UE_info.UE_sched_ctrl[ue_id].ue_AggregatedMaximumBitrateUL;
}

int flexran_get_half_duplex(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.UE_Capability) return -1;

  LTE_SupportedBandListEUTRA_t *bands = &ue_context_p->ue_context.UE_Capability->rf_Parameters.supportedBandListEUTRA;

  for (int i = 0; i < bands->list.count; i++) {
    if (bands->list.array[i]->halfDuplex > 0) return 1;
  }

  return 0;
}

int flexran_get_intra_sf_hopping(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.UE_Capability) return -1;

  if (!ue_context_p->ue_context.UE_Capability->featureGroupIndicators) return -1;

  /* According to TS 36.331 Annex B.1, Intra SF Hopping is bit 1 (leftmost bit)
   * in this bitmap, i.e. the eighth bit (from right) in the first bye (from
   * left) */
  BIT_STRING_t *fgi = ue_context_p->ue_context.UE_Capability->featureGroupIndicators;
  uint8_t buf = fgi->buf[0];
  return (buf >> 7) & 1;
}

int flexran_get_type2_sb_1(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.UE_Capability) return -1;

  if (!ue_context_p->ue_context.UE_Capability->featureGroupIndicators) return -1;

  /* According to TS 36.331 Annex B.1, Predefined intra- and inter-sf or
   * predfined inter-sf frequency hopping for PUSCH with N_sb>1 is bit 21 (bit
   * 1 is leftmost bit) in this bitmap, i.e. the fourth bit (from right) in the
   * third byte (from left) */
  BIT_STRING_t *fgi = ue_context_p->ue_context.UE_Capability->featureGroupIndicators;
  uint8_t buf = fgi->buf[2];
  return (buf >> 3) & 1;
}

long flexran_get_ue_category(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.UE_Capability) return -1;

  return ue_context_p->ue_context.UE_Capability->ue_Category;
}

int flexran_get_res_alloc_type1(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.UE_Capability) return -1;

  if (!ue_context_p->ue_context.UE_Capability->featureGroupIndicators) return -1;

  /* According to TS 36.331 Annex B.1, Resource allocation type 1 for PDSCH is
   * bit 2 (bit 1 is leftmost bit) in this bitmap, i.e. the seventh bit (from
   * right) in the first byte (from left) */
  BIT_STRING_t *fgi = ue_context_p->ue_context.UE_Capability->featureGroupIndicators;
  uint8_t buf = fgi->buf[0];
  return (buf >> 6) & 1;
}

long flexran_get_ue_transmission_mode(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->antennaInfo) return -1;

  return ue_context_p->ue_context.physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode;
}

BOOLEAN_t flexran_get_tti_bundling(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.mac_MainConfig) return -1;

  if (!ue_context_p->ue_context.mac_MainConfig->ul_SCH_Config) return -1;

  return ue_context_p->ue_context.mac_MainConfig->ul_SCH_Config->ttiBundling;
}

long flexran_get_maxHARQ_TX(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.mac_MainConfig) return -1;

  if (!ue_context_p->ue_context.mac_MainConfig->ul_SCH_Config) return -1;

  return *(ue_context_p->ue_context.mac_MainConfig->ul_SCH_Config->maxHARQ_Tx);
}

long flexran_get_beta_offset_ack_index(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated) return -1;

  return ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_ACK_Index;
}

long flexran_get_beta_offset_ri_index(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated) return -1;

  return ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_RI_Index;
}

long flexran_get_beta_offset_cqi_index(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated) return -1;

  return ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_CQI_Index;
}

BOOLEAN_t flexran_get_simultaneous_ack_nack_cqi(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic) return -1;

  return ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.simultaneousAckNackAndCQI;
}

BOOLEAN_t flexran_get_ack_nack_simultaneous_trans(mid_t mod_id, uint8_t cc_id) {
  if (!rrc_is_present(mod_id)) return -1;

  if (!RC.rrc[mod_id]->carrier[cc_id].sib2) return -1;

  return RC.rrc[mod_id]->carrier[cc_id].sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission;
}

Protocol__FlexAperiodicCqiReportMode flexran_get_aperiodic_cqi_rep_mode(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig) return -1;

  switch (*ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig->cqi_ReportModeAperiodic) {
    case LTE_CQI_ReportModeAperiodic_rm12:
      return PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_RM12;

    case LTE_CQI_ReportModeAperiodic_rm20:
      return PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_RM20;

    case LTE_CQI_ReportModeAperiodic_rm22:
      return PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_RM22;

    case LTE_CQI_ReportModeAperiodic_rm30:
      return PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_RM30;

    case LTE_CQI_ReportModeAperiodic_rm31:
      return PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_RM31;

    case LTE_CQI_ReportModeAperiodic_rm32_v1250:
      return PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_RM32_v1250;

    case LTE_CQI_ReportModeAperiodic_rm10_v1310:
      return PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_RM10_v1310;

    case LTE_CQI_ReportModeAperiodic_rm11_v1310:
      return PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_RM11_v1310;

    default:
      return PROTOCOL__FLEX_APERIODIC_CQI_REPORT_MODE__FLACRM_NONE;
  }
}

long flexran_get_tdd_ack_nack_feedback_mode(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode) return -1;

  return *(ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode);
}

long flexran_get_ack_nack_repetition_factor(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated) return -1;

  return ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated->ackNackRepetition.choice.setup.repetitionFactor;
}

long flexran_get_extended_bsr_size(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.mac_MainConfig) return -1;

  if (!ue_context_p->ue_context.mac_MainConfig->ext2) return -1;

  if (!ue_context_p->ue_context.mac_MainConfig->ext2->mac_MainConfig_v1020) return -1;

  return *(ue_context_p->ue_context.mac_MainConfig->ext2->mac_MainConfig_v1020->extendedBSR_Sizes_r10);
}

int flexran_get_ue_transmission_antenna(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;

  if (!ue_context_p->ue_context.physicalConfigDedicated->antennaInfo) return -1;

  switch (ue_context_p->ue_context.physicalConfigDedicated->antennaInfo->choice.explicitValue.ue_TransmitAntennaSelection.choice.setup) {
    case LTE_AntennaInfoDedicated__ue_TransmitAntennaSelection__setup_closedLoop:
      return 2;

    case LTE_AntennaInfoDedicated__ue_TransmitAntennaSelection__setup_openLoop:
      return 1;

    default:
      return 0;
  }
}

uint64_t flexran_get_ue_imsi(mid_t mod_id, rnti_t rnti) {
  uint64_t imsi;

  if (!rrc_is_present(mod_id)) return 0;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return 0;

  imsi  = ue_context_p->ue_context.imsi.digit15;
  imsi += ue_context_p->ue_context.imsi.digit14 * 10;              // pow(10, 1)
  imsi += ue_context_p->ue_context.imsi.digit13 * 100;             // pow(10, 2)
  imsi += ue_context_p->ue_context.imsi.digit12 * 1000;            // pow(10, 3)
  imsi += ue_context_p->ue_context.imsi.digit11 * 10000;           // pow(10, 4)
  imsi += ue_context_p->ue_context.imsi.digit10 * 100000;          // pow(10, 5)
  imsi += ue_context_p->ue_context.imsi.digit9  * 1000000;         // pow(10, 6)
  imsi += ue_context_p->ue_context.imsi.digit8  * 10000000;        // pow(10, 7)
  imsi += ue_context_p->ue_context.imsi.digit7  * 100000000;       // pow(10, 8)
  imsi += ue_context_p->ue_context.imsi.digit6  * 1000000000;      // pow(10, 9)
  imsi += ue_context_p->ue_context.imsi.digit5  * 10000000000;     // pow(10, 10)
  imsi += ue_context_p->ue_context.imsi.digit4  * 100000000000;    // pow(10, 11)
  imsi += ue_context_p->ue_context.imsi.digit3  * 1000000000000;   // pow(10, 12)
  imsi += ue_context_p->ue_context.imsi.digit2  * 10000000000000;  // pow(10, 13)
  imsi += ue_context_p->ue_context.imsi.digit1  * 100000000000000; // pow(10, 14)
  return imsi;
}

long flexran_get_lcg(mid_t mod_id, mid_t ue_id, mid_t lc_id) {
  if (!mac_is_present(mod_id)) return 0;

  return RC.mac[mod_id]->UE_info.UE_template[UE_PCCID(mod_id, ue_id)][ue_id].lcgidmap[lc_id];
}

/* TODO Navid: needs to be revised */
int flexran_get_direction(mid_t ue_id, mid_t lc_id) {
  switch (lc_id) {
    case DCCH:
    case DCCH1:
      return 2;

    case DTCH:
      return 1;

    default:
      return -1;
  }
}

uint8_t flexran_get_antenna_ports(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.nb_antenna_ports_eNB;
}

uint32_t flexran_agent_get_operating_dl_freq(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.dl_CarrierFreq / 1000000;
}

uint32_t flexran_agent_get_operating_ul_freq(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.ul_CarrierFreq / 1000000;
}

uint8_t flexran_agent_get_operating_eutra_band(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.eutra_band;
}

int8_t flexran_agent_get_operating_pdsch_refpower(mid_t mod_id, uint8_t cc_id) {
  if (!phy_is_present(mod_id, cc_id)) return 0;

  return RC.eNB[mod_id][cc_id]->frame_parms.pdsch_config_common.referenceSignalPower;
}

long flexran_agent_get_operating_pusch_p0(mid_t mod_id, uint8_t cc_id) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->configuration.radioresourceconfig[cc_id].pusch_p0_Nominal;
}

void flexran_agent_set_operating_dl_freq(mid_t mod_id, uint8_t cc_id, uint32_t dl_freq_mhz) {
  if (phy_is_present(mod_id, cc_id)) {
    RC.eNB[mod_id][cc_id]->frame_parms.dl_CarrierFreq = dl_freq_mhz * 1000000;
  } else {
    LOG_E(FLEXRAN_AGENT, "can not set dl_CarrierFreq to %d MHz in PHY: PHY is not present\n", dl_freq_mhz);
  }

  if (rrc_is_present(mod_id)) {
    RC.rrc[mod_id]->configuration.downlink_frequency[cc_id] = dl_freq_mhz * 1000000;
  } else {
    LOG_E(FLEXRAN_AGENT, "can not set downlink_frequency to %d MHz in RRC: RRC is not present\n", dl_freq_mhz);
  }
}

void flexran_agent_set_operating_ul_freq(mid_t mod_id, uint8_t cc_id, int32_t ul_freq_mhz_offset) {
  if (phy_is_present(mod_id, cc_id)) {
    uint32_t new_ul_freq_mhz = flexran_agent_get_operating_dl_freq(mod_id, cc_id) + ul_freq_mhz_offset;
    RC.eNB[mod_id][cc_id]->frame_parms.ul_CarrierFreq = new_ul_freq_mhz * 1000000;
  } else {
    LOG_E(FLEXRAN_AGENT, "can not set ul_CarrierFreq using offset %d MHz in PHY: PHY is not present\n", ul_freq_mhz_offset);
  }

  if (rrc_is_present(mod_id)) {
    RC.rrc[mod_id]->configuration.uplink_frequency_offset[cc_id] = ul_freq_mhz_offset;
  } else {
    LOG_E(FLEXRAN_AGENT, "can not set uplink_frequency_offset to %d MHz in RRC: RRC is not present\n", ul_freq_mhz_offset);
  }
}

void flexran_agent_set_operating_eutra_band(mid_t mod_id, uint8_t cc_id, uint8_t eutra_band) {
  if (phy_is_present(mod_id, cc_id)) {
    RC.eNB[mod_id][cc_id]->frame_parms.eutra_band = eutra_band;
  } else {
    LOG_E(FLEXRAN_AGENT, "can not set eutra_band to %d in PHY: PHY is not present\n", eutra_band);
  }

  if (rrc_is_present(mod_id)) {
    RC.rrc[mod_id]->configuration.eutra_band[cc_id] = eutra_band;
  } else {
    LOG_E(FLEXRAN_AGENT, "can not set eutra_band to %d in RRC: RRC is not present\n", eutra_band);
  }
}

/* Sets both DL/UL */
void flexran_agent_set_operating_bandwidth(mid_t mod_id, uint8_t cc_id, uint8_t N_RB) {
  if (phy_is_present(mod_id, cc_id)) {
    RC.eNB[mod_id][cc_id]->frame_parms.N_RB_DL = N_RB;
    RC.eNB[mod_id][cc_id]->frame_parms.N_RB_UL = N_RB;
  } else {
    LOG_E(FLEXRAN_AGENT, "can not set N_RB_DL and N_RB_UL to %d in PHY: PHY is not present\n", N_RB);
  }

  if (rrc_is_present(mod_id)) {
    RC.rrc[mod_id]->configuration.N_RB_DL[cc_id] = N_RB;
  } else {
    LOG_E(FLEXRAN_AGENT, "can not set N_RB_DL to %d in RRC: RRC is not present\n", N_RB);
  }
}

void flexran_agent_set_operating_frame_type(mid_t mod_id, uint8_t cc_id, frame_type_t frame_type) {
  if (phy_is_present(mod_id, cc_id)) {
    RC.eNB[mod_id][cc_id]->frame_parms.frame_type = frame_type;
  } else {
    LOG_E(FLEXRAN_AGENT, "can not set frame_type to %d in PHY: PHY is not present\n", frame_type);
  }

  if (rrc_is_present(mod_id)) {
    RC.rrc[mod_id]->configuration.frame_type[cc_id] = frame_type;
  } else {
    LOG_E(FLEXRAN_AGENT, "can not set frame_type to %d in RRC: RRC is not present\n", frame_type);
  }
}

/*********** PDCP  *************/

uint16_t flexran_get_pdcp_uid_from_rnti(mid_t mod_id, rnti_t rnti) {
  if (rnti == NOT_A_RNTI) return 0;

  if (mod_id < 0 || mod_id >= RC.nb_inst)
    return 0;

  for (uint16_t pdcp_uid = 0; pdcp_uid < MAX_MOBILES_PER_ENB; ++pdcp_uid) {
    if (pdcp_enb[mod_id].rnti[pdcp_uid] == rnti)
      return pdcp_uid;
  }

  return 0;
}

/*PDCP super frame counter flexRAN*/
uint32_t flexran_get_pdcp_sfn(mid_t mod_id) {
  if (mod_id < 0 || mod_id >= RC.nb_inst)
    return 0;

  return pdcp_enb[mod_id].sfn;
}

/*PDCP super frame counter flexRAN*/
void flexran_set_pdcp_tx_stat_window(mid_t mod_id, uint16_t uid, uint16_t obs_window) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB)
    return;

  Pdcp_stats_tx_window_ms[mod_id][uid] = obs_window > 0 ? obs_window : 1000;
}

/*PDCP super frame counter flexRAN*/
void flexran_set_pdcp_rx_stat_window(mid_t mod_id, uint16_t uid, uint16_t obs_window) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB)
    return;

  Pdcp_stats_rx_window_ms[mod_id][uid] = obs_window > 0 ? obs_window : 1000;
}

/*PDCP num tx pdu status flexRAN*/
uint32_t flexran_get_pdcp_tx(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_tx[mod_id][uid][lcid];
}

/*PDCP num tx bytes status flexRAN*/
uint32_t flexran_get_pdcp_tx_bytes(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_tx_bytes[mod_id][uid][lcid];
}

/*PDCP number of transmit packet / second status flexRAN*/
uint32_t flexran_get_pdcp_tx_w(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_tx_w[mod_id][uid][lcid];
}

/*PDCP throughput (bit/s) status flexRAN*/
uint32_t flexran_get_pdcp_tx_bytes_w(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_tx_bytes_w[mod_id][uid][lcid];
}

/*PDCP tx sequence number flexRAN*/
uint32_t flexran_get_pdcp_tx_sn(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_tx_sn[mod_id][uid][lcid];
}

/*PDCP tx aggregated packet arrival  flexRAN*/
uint32_t flexran_get_pdcp_tx_aiat(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_tx_aiat[mod_id][uid][lcid];
}

/*PDCP tx aggregated packet arrival  flexRAN*/
uint32_t flexran_get_pdcp_tx_aiat_w(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_tx_aiat_w[mod_id][uid][lcid];
}

/*PDCP num rx pdu status flexRAN*/
uint32_t flexran_get_pdcp_rx(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_rx[mod_id][uid][lcid];
}

/*PDCP num rx bytes status flexRAN*/
uint32_t flexran_get_pdcp_rx_bytes(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_rx_bytes[mod_id][uid][lcid];
}

/*PDCP number of received packet / second  flexRAN*/
uint32_t flexran_get_pdcp_rx_w(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_rx_w[mod_id][uid][lcid];
}

/*PDCP gootput (bit/s) status flexRAN*/
uint32_t flexran_get_pdcp_rx_bytes_w(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_rx_bytes_w[mod_id][uid][lcid];
}

/*PDCP rx sequence number flexRAN*/
uint32_t flexran_get_pdcp_rx_sn(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_rx_sn[mod_id][uid][lcid];
}

/*PDCP rx aggregated packet arrival  flexRAN*/
uint32_t flexran_get_pdcp_rx_aiat(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_rx_aiat[mod_id][uid][lcid];
}

/*PDCP rx aggregated packet arrival  flexRAN*/
uint32_t flexran_get_pdcp_rx_aiat_w(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_rx_aiat_w[mod_id][uid][lcid];
}

/*PDCP num of received outoforder pdu status flexRAN*/
uint32_t flexran_get_pdcp_rx_oo(mid_t mod_id, uint16_t uid, lcid_t lcid) {
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;

  return Pdcp_stats_rx_outoforder[mod_id][uid][lcid];
}

/******************** RRC *****************************/
/* RRC Wrappers */
int flexran_call_rrc_reconfiguration (mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  protocol_ctxt_t  ctxt;
  memset(&ctxt, 0, sizeof(ctxt));
  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, mod_id, ENB_FLAG_YES, ue_context_p->ue_context.rnti, flexran_get_current_frame(mod_id), flexran_get_current_subframe (mod_id), mod_id);
  flexran_rrc_eNB_generate_defaultRRCConnectionReconfiguration(&ctxt, ue_context_p, 0);
  return 0;
}

int flexran_call_rrc_trigger_handover (mid_t mod_id, rnti_t rnti, int target_cell_id) {
  if (!rrc_is_present(mod_id)) return -1;

  protocol_ctxt_t  ctxt;
  memset(&ctxt, 0, sizeof(ctxt));
  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, mod_id, ENB_FLAG_YES, ue_context_p->ue_context.rnti, flexran_get_current_frame(mod_id), flexran_get_current_subframe (mod_id), mod_id);
  return flexran_rrc_eNB_trigger_handover(mod_id, &ctxt, ue_context_p, target_cell_id);
}

/* RRC Getters */

LTE_MeasId_t  flexran_get_rrc_pcell_measid(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measResults) return -1;

  return ue_context_p->ue_context.measResults->measId;
}

float flexran_get_rrc_pcell_rsrp(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measResults) return -1;

  return RSRP_meas_mapping[ue_context_p->ue_context.measResults->measResultPCell.rsrpResult];
}

float flexran_get_rrc_pcell_rsrq(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measResults) return -1;

  return RSRQ_meas_mapping[ue_context_p->ue_context.measResults->measResultPCell.rsrqResult];
}

/*Number of neighbouring cells for specific UE*/
int flexran_get_rrc_num_ncell(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return 0;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return 0;

  if (!ue_context_p->ue_context.measResults) return 0;

  if (!ue_context_p->ue_context.measResults->measResultNeighCells) return 0;

  //if (ue_context_p->ue_context.measResults->measResultNeighCells->present != LTE_MeasResults__measResultNeighCells_PR_measResultListEUTRA) return 0;
  return ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.count;
}

long flexran_get_rrc_neigh_phy_cell_id(mid_t mod_id, rnti_t rnti, long cell_id) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measResults) return -1;

  if (!ue_context_p->ue_context.measResults->measResultNeighCells) return -1;

  //if (ue_context_p->ue_context.measResults->measResultNeighCells->present != LTE_MeasResults__measResultNeighCells_PR_measResultListEUTRA) return -1;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]) return -1;

  return ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->physCellId;
}

int flexran_get_rrc_neigh_cgi(mid_t mod_id, rnti_t rnti, long cell_id) {
  if (!rrc_is_present(mod_id)) return 0;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return 0;

  if (!ue_context_p->ue_context.measResults) return 0;

  if (!ue_context_p->ue_context.measResults->measResultNeighCells) return 0;

  //if (ue_context_p->ue_context.measResults->measResultNeighCells->present != LTE_MeasResults__measResultNeighCells_PR_measResultListEUTRA) return 0;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]) return 0;

  return (!ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->cgi_Info)?0:1;
}

uint32_t flexran_get_rrc_neigh_cgi_cell_id(mid_t mod_id, rnti_t rnti, long cell_id) {
  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  uint8_t *cId = ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->cgi_Info->cellGlobalId.cellIdentity.buf;
  return ((cId[0] << 20) + (cId[1] << 12) + (cId[2] << 4) + (cId[3] >> 4));
}

uint32_t flexran_get_rrc_neigh_cgi_tac(mid_t mod_id, rnti_t rnti, long cell_id) {
  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  uint8_t *tac = ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->cgi_Info->trackingAreaCode.buf;
  return (tac[0] << 8) + (tac[1]);
}

int flexran_get_rrc_neigh_cgi_num_mnc(mid_t mod_id, rnti_t rnti, long cell_id) {
  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  return ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->cgi_Info->cellGlobalId.plmn_Identity.mnc.list.count;
}

int flexran_get_rrc_neigh_cgi_num_mcc(mid_t mod_id, rnti_t rnti, long cell_id) {
  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  return ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->cgi_Info->cellGlobalId.plmn_Identity.mcc->list.count;
}

uint32_t flexran_get_rrc_neigh_cgi_mnc(mid_t mod_id, rnti_t rnti, long cell_id, int mnc_id) {
  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  int num_mnc = ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->cgi_Info->cellGlobalId.plmn_Identity.mnc.list.count;
  return *(ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->cgi_Info->cellGlobalId.plmn_Identity.mnc.list.array[mnc_id]) *
         ((uint32_t) pow(10, num_mnc - mnc_id - 1));
}

uint32_t flexran_get_rrc_neigh_cgi_mcc(mid_t mod_id, rnti_t rnti, long cell_id, int mcc_id) {
  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  int num_mcc = ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->cgi_Info->cellGlobalId.plmn_Identity.mcc->list.count;
  return *(ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->cgi_Info->cellGlobalId.plmn_Identity.mcc->list.array[mcc_id]) *
         ((uint32_t) pow(10, num_mcc - mcc_id - 1));
}

float flexran_get_rrc_neigh_rsrp(mid_t mod_id, rnti_t rnti, long cell_id) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measResults) return -1;

  if (!ue_context_p->ue_context.measResults->measResultNeighCells) return -1;

  //if (ue_context_p->ue_context.measResults->measResultNeighCells->present != LTE_MeasResults__measResultNeighCells_PR_measResultListEUTRA) return -1;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]) return -1;

  if (!ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->measResult.rsrpResult) return -1;

  return RSRP_meas_mapping[*(ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->measResult.rsrpResult)];
}

float flexran_get_rrc_neigh_rsrq(mid_t mod_id, rnti_t rnti, long cell_id) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measResults) return -1;

  if (!ue_context_p->ue_context.measResults->measResultNeighCells) return -1;

  //if (ue_context_p->ue_context.measResults->measResultNeighCells->present != LTE_MeasResults__measResultNeighCells_PR_measResultListEUTRA) return -1;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->measResult.rsrqResult) return -1;

  return RSRQ_meas_mapping[*(ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->measResult.rsrqResult)];
}

/* Measurement offsets */

long flexran_get_rrc_ofp(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  return ue_context_p->ue_context.measurement_info->offsetFreq;
}

long flexran_get_rrc_ofn(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  return ue_context_p->ue_context.measurement_info->offsetFreq;
}

long flexran_get_rrc_ocp(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  switch (ue_context_p->ue_context.measurement_info->cellIndividualOffset[0]) {
    case LTE_Q_OffsetRange_dB_24:
      return -24;

    case LTE_Q_OffsetRange_dB_22:
      return -22;

    case LTE_Q_OffsetRange_dB_20:
      return -20;

    case LTE_Q_OffsetRange_dB_18:
      return -18;

    case LTE_Q_OffsetRange_dB_16:
      return -16;

    case LTE_Q_OffsetRange_dB_14:
      return -14;

    case LTE_Q_OffsetRange_dB_12:
      return -12;

    case LTE_Q_OffsetRange_dB_10:
      return -10;

    case LTE_Q_OffsetRange_dB_8:
      return -8;

    case LTE_Q_OffsetRange_dB_6:
      return -6;

    case LTE_Q_OffsetRange_dB_5:
      return -5;

    case LTE_Q_OffsetRange_dB_4:
      return -4;

    case LTE_Q_OffsetRange_dB_3:
      return -3;

    case LTE_Q_OffsetRange_dB_2:
      return -2;

    case LTE_Q_OffsetRange_dB_1:
      return -1;

    case LTE_Q_OffsetRange_dB0:
      return  0;

    case LTE_Q_OffsetRange_dB1:
      return  1;

    case LTE_Q_OffsetRange_dB2:
      return  2;

    case LTE_Q_OffsetRange_dB3:
      return  3;

    case LTE_Q_OffsetRange_dB4:
      return  4;

    case LTE_Q_OffsetRange_dB5:
      return  5;

    case LTE_Q_OffsetRange_dB6:
      return  6;

    case LTE_Q_OffsetRange_dB8:
      return  8;

    case LTE_Q_OffsetRange_dB10:
      return 10;

    case LTE_Q_OffsetRange_dB12:
      return 12;

    case LTE_Q_OffsetRange_dB14:
      return 14;

    case LTE_Q_OffsetRange_dB16:
      return 16;

    case LTE_Q_OffsetRange_dB18:
      return 18;

    case LTE_Q_OffsetRange_dB20:
      return 20;

    case LTE_Q_OffsetRange_dB22:
      return 22;

    case LTE_Q_OffsetRange_dB24:
      return 24;

    default:
      return -99;
  }
}

long flexran_get_rrc_ocn(mid_t mod_id, rnti_t rnti, long cell_id) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  switch (ue_context_p->ue_context.measurement_info->cellIndividualOffset[cell_id+1]) {
    case LTE_Q_OffsetRange_dB_24:
      return -24;

    case LTE_Q_OffsetRange_dB_22:
      return -22;

    case LTE_Q_OffsetRange_dB_20:
      return -20;

    case LTE_Q_OffsetRange_dB_18:
      return -18;

    case LTE_Q_OffsetRange_dB_16:
      return -16;

    case LTE_Q_OffsetRange_dB_14:
      return -14;

    case LTE_Q_OffsetRange_dB_12:
      return -12;

    case LTE_Q_OffsetRange_dB_10:
      return -10;

    case LTE_Q_OffsetRange_dB_8:
      return -8;

    case LTE_Q_OffsetRange_dB_6:
      return -6;

    case LTE_Q_OffsetRange_dB_5:
      return -5;

    case LTE_Q_OffsetRange_dB_4:
      return -4;

    case LTE_Q_OffsetRange_dB_3:
      return -3;

    case LTE_Q_OffsetRange_dB_2:
      return -2;

    case LTE_Q_OffsetRange_dB_1:
      return -1;

    case LTE_Q_OffsetRange_dB0:
      return  0;

    case LTE_Q_OffsetRange_dB1:
      return  1;

    case LTE_Q_OffsetRange_dB2:
      return  2;

    case LTE_Q_OffsetRange_dB3:
      return  3;

    case LTE_Q_OffsetRange_dB4:
      return  4;

    case LTE_Q_OffsetRange_dB5:
      return  5;

    case LTE_Q_OffsetRange_dB6:
      return  6;

    case LTE_Q_OffsetRange_dB8:
      return  8;

    case LTE_Q_OffsetRange_dB10:
      return 10;

    case LTE_Q_OffsetRange_dB12:
      return 12;

    case LTE_Q_OffsetRange_dB14:
      return 14;

    case LTE_Q_OffsetRange_dB16:
      return 16;

    case LTE_Q_OffsetRange_dB18:
      return 18;

    case LTE_Q_OffsetRange_dB20:
      return 20;

    case LTE_Q_OffsetRange_dB22:
      return 22;

    case LTE_Q_OffsetRange_dB24:
      return 24;

    default:
      return -99;
  }
}

long flexran_get_filter_coeff_rsrp(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  switch (ue_context_p->ue_context.measurement_info->filterCoefficientRSRP) {
    case LTE_FilterCoefficient_fc0:
      return 0;

    case LTE_FilterCoefficient_fc1:
      return 1;

    case LTE_FilterCoefficient_fc2:
      return 2;

    case LTE_FilterCoefficient_fc3:
      return 3;

    case LTE_FilterCoefficient_fc4:
      return 4;

    case LTE_FilterCoefficient_fc5:
      return 5;

    case LTE_FilterCoefficient_fc6:
      return 6;

    case LTE_FilterCoefficient_fc7:
      return 7;

    case LTE_FilterCoefficient_fc8:
      return 8;

    case LTE_FilterCoefficient_fc9:
      return 9;

    case LTE_FilterCoefficient_fc11:
      return 11;

    case LTE_FilterCoefficient_fc13:
      return 13;

    case LTE_FilterCoefficient_fc15:
      return 15;

    case LTE_FilterCoefficient_fc17:
      return 17;

    case LTE_FilterCoefficient_fc19:
      return 19;

    case LTE_FilterCoefficient_spare1:
      return -1; /* spare means no coefficient */

    default:
      return -1;
  }
}

long flexran_get_filter_coeff_rsrq(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  switch (ue_context_p->ue_context.measurement_info->filterCoefficientRSRQ) {
    case LTE_FilterCoefficient_fc0:
      return 0;

    case LTE_FilterCoefficient_fc1:
      return 1;

    case LTE_FilterCoefficient_fc2:
      return 2;

    case LTE_FilterCoefficient_fc3:
      return 3;

    case LTE_FilterCoefficient_fc4:
      return 4;

    case LTE_FilterCoefficient_fc5:
      return 5;

    case LTE_FilterCoefficient_fc6:
      return 6;

    case LTE_FilterCoefficient_fc7:
      return 7;

    case LTE_FilterCoefficient_fc8:
      return 8;

    case LTE_FilterCoefficient_fc9:
      return 9;

    case LTE_FilterCoefficient_fc11:
      return 11;

    case LTE_FilterCoefficient_fc13:
      return 13;

    case LTE_FilterCoefficient_fc15:
      return 15;

    case LTE_FilterCoefficient_fc17:
      return 17;

    case LTE_FilterCoefficient_fc19:
      return 19;

    case LTE_FilterCoefficient_spare1:
      return -1; /* spare means no coefficient */

    default:
      return -1;
  }
}

/* Periodic event */

long flexran_get_rrc_per_event_maxReportCells(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->per_event) return -1;

  return ue_context_p->ue_context.measurement_info->events->per_event->maxReportCells;
}

/* A3 event */

long flexran_get_rrc_a3_event_hysteresis(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->a3_event) return -1;

  return ue_context_p->ue_context.measurement_info->events->a3_event->hysteresis;
}

long flexran_get_rrc_a3_event_timeToTrigger(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->a3_event) return -1;

  switch (ue_context_p->ue_context.measurement_info->events->a3_event->timeToTrigger) {
    case LTE_TimeToTrigger_ms0:
      return 0;

    case LTE_TimeToTrigger_ms40:
      return 40;

    case LTE_TimeToTrigger_ms64:
      return 64;

    case LTE_TimeToTrigger_ms80:
      return 80;

    case LTE_TimeToTrigger_ms100:
      return 100;

    case LTE_TimeToTrigger_ms128:
      return 128;

    case LTE_TimeToTrigger_ms160:
      return 160;

    case LTE_TimeToTrigger_ms256:
      return 256;

    case LTE_TimeToTrigger_ms320:
      return 320;

    case LTE_TimeToTrigger_ms480:
      return 480;

    case LTE_TimeToTrigger_ms512:
      return 512;

    case LTE_TimeToTrigger_ms640:
      return 640;

    case LTE_TimeToTrigger_ms1024:
      return 1024;

    case LTE_TimeToTrigger_ms1280:
      return 1280;

    case LTE_TimeToTrigger_ms2560:
      return 2560;

    case LTE_TimeToTrigger_ms5120:
      return 5120;

    default:
      return -1;
  }
}

long flexran_get_rrc_a3_event_maxReportCells(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->a3_event) return -1;

  return ue_context_p->ue_context.measurement_info->events->a3_event->maxReportCells;
}

long flexran_get_rrc_a3_event_a3_offset(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->a3_event) return -1;

  return ue_context_p->ue_context.measurement_info->events->a3_event->a3_offset;
}

int flexran_get_rrc_a3_event_reportOnLeave(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->a3_event) return -1;

  return ue_context_p->ue_context.measurement_info->events->a3_event->reportOnLeave;
}

/* RRC Setters */

/* Measurement offsets */

int flexran_set_rrc_ofp(mid_t mod_id, rnti_t rnti, long offsetFreq) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!((offsetFreq >= -15) && (offsetFreq <= 15))) return -1;

  ue_context_p->ue_context.measurement_info->offsetFreq = offsetFreq;
  return 0;
}

int flexran_set_rrc_ofn(mid_t mod_id, rnti_t rnti, long offsetFreq) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!((offsetFreq >= -15) && (offsetFreq <= 15))) return -1;

  ue_context_p->ue_context.measurement_info->offsetFreq = offsetFreq;
  return 0;
}

int flexran_set_rrc_ocp(mid_t mod_id, rnti_t rnti, long cellIndividualOffset) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  LTE_Q_OffsetRange_t *cio = &ue_context_p->ue_context.measurement_info->cellIndividualOffset[0];

  switch (cellIndividualOffset) {
    case -24:
      *cio = LTE_Q_OffsetRange_dB_24;
      break;

    case -22:
      *cio = LTE_Q_OffsetRange_dB_22;
      break;

    case -20:
      *cio = LTE_Q_OffsetRange_dB_20;
      break;

    case -18:
      *cio = LTE_Q_OffsetRange_dB_18;
      break;

    case -16:
      *cio = LTE_Q_OffsetRange_dB_16;
      break;

    case -14:
      *cio = LTE_Q_OffsetRange_dB_14;
      break;

    case -12:
      *cio = LTE_Q_OffsetRange_dB_12;
      break;

    case -10:
      *cio = LTE_Q_OffsetRange_dB_10;
      break;

    case -8:
      *cio = LTE_Q_OffsetRange_dB_8;
      break;

    case -6:
      *cio = LTE_Q_OffsetRange_dB_6;
      break;

    case -5:
      *cio = LTE_Q_OffsetRange_dB_5;
      break;

    case -4:
      *cio = LTE_Q_OffsetRange_dB_4;
      break;

    case -3:
      *cio = LTE_Q_OffsetRange_dB_3;
      break;

    case -2:
      *cio = LTE_Q_OffsetRange_dB_2;
      break;

    case -1:
      *cio = LTE_Q_OffsetRange_dB_1;
      break;

    case 0:
      *cio = LTE_Q_OffsetRange_dB0;
      break;

    case 1:
      *cio = LTE_Q_OffsetRange_dB1;
      break;

    case 2:
      *cio = LTE_Q_OffsetRange_dB2;
      break;

    case 3:
      *cio = LTE_Q_OffsetRange_dB3;
      break;

    case 4:
      *cio = LTE_Q_OffsetRange_dB4;
      break;

    case 5:
      *cio = LTE_Q_OffsetRange_dB5;
      break;

    case 6:
      *cio = LTE_Q_OffsetRange_dB6;
      break;

    case 8:
      *cio = LTE_Q_OffsetRange_dB8;
      break;

    case 10:
      *cio = LTE_Q_OffsetRange_dB10;
      break;

    case 12:
      *cio = LTE_Q_OffsetRange_dB12;
      break;

    case 14:
      *cio = LTE_Q_OffsetRange_dB14;
      break;

    case 16:
      *cio = LTE_Q_OffsetRange_dB16;
      break;

    case 18:
      *cio = LTE_Q_OffsetRange_dB18;
      break;

    case 20:
      *cio = LTE_Q_OffsetRange_dB20;
      break;

    case 22:
      *cio = LTE_Q_OffsetRange_dB22;
      break;

    case 24:
      *cio = LTE_Q_OffsetRange_dB24;
      break;

    default:
      return -1;
  }

  return 0;
}

int flexran_set_rrc_ocn(mid_t mod_id, rnti_t rnti, long cell_id, long cellIndividualOffset) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  LTE_Q_OffsetRange_t *cio = &ue_context_p->ue_context.measurement_info->cellIndividualOffset[cell_id+1];

  switch (cellIndividualOffset) {
    case -24:
      *cio = LTE_Q_OffsetRange_dB_24;
      break;

    case -22:
      *cio = LTE_Q_OffsetRange_dB_22;
      break;

    case -20:
      *cio = LTE_Q_OffsetRange_dB_20;
      break;

    case -18:
      *cio = LTE_Q_OffsetRange_dB_18;
      break;

    case -16:
      *cio = LTE_Q_OffsetRange_dB_16;
      break;

    case -14:
      *cio = LTE_Q_OffsetRange_dB_14;
      break;

    case -12:
      *cio = LTE_Q_OffsetRange_dB_12;
      break;

    case -10:
      *cio = LTE_Q_OffsetRange_dB_10;
      break;

    case -8:
      *cio = LTE_Q_OffsetRange_dB_8;
      break;

    case -6:
      *cio = LTE_Q_OffsetRange_dB_6;
      break;

    case -5:
      *cio = LTE_Q_OffsetRange_dB_5;
      break;

    case -4:
      *cio = LTE_Q_OffsetRange_dB_4;
      break;

    case -3:
      *cio = LTE_Q_OffsetRange_dB_3;
      break;

    case -2:
      *cio = LTE_Q_OffsetRange_dB_2;
      break;

    case -1:
      *cio = LTE_Q_OffsetRange_dB_1;
      break;

    case 0:
      *cio = LTE_Q_OffsetRange_dB0;
      break;

    case 1:
      *cio = LTE_Q_OffsetRange_dB1;
      break;

    case 2:
      *cio = LTE_Q_OffsetRange_dB2;
      break;

    case 3:
      *cio = LTE_Q_OffsetRange_dB3;
      break;

    case 4:
      *cio = LTE_Q_OffsetRange_dB4;
      break;

    case 5:
      *cio = LTE_Q_OffsetRange_dB5;
      break;

    case 6:
      *cio = LTE_Q_OffsetRange_dB6;
      break;

    case 8:
      *cio = LTE_Q_OffsetRange_dB8;
      break;

    case 10:
      *cio = LTE_Q_OffsetRange_dB10;
      break;

    case 12:
      *cio = LTE_Q_OffsetRange_dB12;
      break;

    case 14:
      *cio = LTE_Q_OffsetRange_dB14;
      break;

    case 16:
      *cio = LTE_Q_OffsetRange_dB16;
      break;

    case 18:
      *cio = LTE_Q_OffsetRange_dB18;
      break;

    case 20:
      *cio = LTE_Q_OffsetRange_dB20;
      break;

    case 22:
      *cio = LTE_Q_OffsetRange_dB22;
      break;

    case 24:
      *cio = LTE_Q_OffsetRange_dB24;
      break;

    default:
      return -1;
  }

  return 0;
}

int flexran_set_filter_coeff_rsrp(mid_t mod_id, rnti_t rnti, long filterCoefficientRSRP) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  LTE_FilterCoefficient_t *fc = &ue_context_p->ue_context.measurement_info->filterCoefficientRSRP;

  switch (filterCoefficientRSRP) {
    case 0:
      *fc = LTE_FilterCoefficient_fc0;
      break;

    case 1:
      *fc = LTE_FilterCoefficient_fc1;
      break;

    case 2:
      *fc = LTE_FilterCoefficient_fc2;
      break;

    case 3:
      *fc = LTE_FilterCoefficient_fc3;
      break;

    case 4:
      *fc = LTE_FilterCoefficient_fc4;
      break;

    case 5:
      *fc = LTE_FilterCoefficient_fc5;
      break;

    case 6:
      *fc = LTE_FilterCoefficient_fc6;
      break;

    case 7:
      *fc = LTE_FilterCoefficient_fc7;
      break;

    case 8:
      *fc = LTE_FilterCoefficient_fc8;
      break;

    case 9:
      *fc = LTE_FilterCoefficient_fc9;
      break;

    case 11:
      *fc = LTE_FilterCoefficient_fc11;
      break;

    case 13:
      *fc = LTE_FilterCoefficient_fc13;
      break;

    case 15:
      *fc = LTE_FilterCoefficient_fc15;
      break;

    case 17:
      *fc = LTE_FilterCoefficient_fc17;
      break;

    case 19:
      *fc = LTE_FilterCoefficient_fc19;
      break;

    case -1:
      *fc = LTE_FilterCoefficient_spare1;
      break;

    default:
      return -1;
  }

  return 0;
}

int flexran_set_filter_coeff_rsrq(mid_t mod_id, rnti_t rnti, long filterCoefficientRSRQ) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  LTE_FilterCoefficient_t *fc = &ue_context_p->ue_context.measurement_info->filterCoefficientRSRQ;

  switch (filterCoefficientRSRQ) {
    case 0:
      *fc = LTE_FilterCoefficient_fc0;
      break;

    case 1:
      *fc = LTE_FilterCoefficient_fc1;
      break;

    case 2:
      *fc = LTE_FilterCoefficient_fc2;
      break;

    case 3:
      *fc = LTE_FilterCoefficient_fc3;
      break;

    case 4:
      *fc = LTE_FilterCoefficient_fc4;
      break;

    case 5:
      *fc = LTE_FilterCoefficient_fc5;
      break;

    case 6:
      *fc = LTE_FilterCoefficient_fc6;
      break;

    case 7:
      *fc = LTE_FilterCoefficient_fc7;
      break;

    case 8:
      *fc = LTE_FilterCoefficient_fc8;
      break;

    case 9:
      *fc = LTE_FilterCoefficient_fc9;
      break;

    case 11:
      *fc = LTE_FilterCoefficient_fc11;
      break;

    case 13:
      *fc = LTE_FilterCoefficient_fc13;
      break;

    case 15:
      *fc = LTE_FilterCoefficient_fc15;
      break;

    case 17:
      *fc = LTE_FilterCoefficient_fc17;
      break;

    case 19:
      *fc = LTE_FilterCoefficient_fc19;
      break;

    case -1:
      *fc = LTE_FilterCoefficient_spare1;
      break;

    default:
      return -1;
  }

  return 0;
}

/* Periodic event */

int flexran_set_rrc_per_event_maxReportCells(mid_t mod_id, rnti_t rnti, long maxReportCells) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->per_event) return -1;

  if (!((maxReportCells >= 1) && (maxReportCells <= 8))) return -1;

  ue_context_p->ue_context.measurement_info->events->per_event->maxReportCells = maxReportCells;
  return 0;
}

/* A3 event */

int flexran_set_rrc_a3_event_hysteresis(mid_t mod_id, rnti_t rnti, long hysteresis) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->a3_event) return -1;

  if (!((hysteresis >=0) && (hysteresis <= 30))) return -1;

  ue_context_p->ue_context.measurement_info->events->a3_event->hysteresis = hysteresis;
  return 0;
}

int flexran_set_rrc_a3_event_timeToTrigger(mid_t mod_id, rnti_t rnti, long timeToTrigger) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->a3_event) return -1;

  LTE_TimeToTrigger_t *ttt = &ue_context_p->ue_context.measurement_info->events->a3_event->timeToTrigger;

  switch (timeToTrigger) {
    case 0:
      *ttt = LTE_TimeToTrigger_ms0;
      break;

    case 40:
      *ttt = LTE_TimeToTrigger_ms40;
      break;

    case 64:
      *ttt = LTE_TimeToTrigger_ms64;
      break;

    case 80:
      *ttt = LTE_TimeToTrigger_ms80;
      break;

    case 100:
      *ttt = LTE_TimeToTrigger_ms100;
      break;

    case 128:
      *ttt = LTE_TimeToTrigger_ms128;
      break;

    case 160:
      *ttt = LTE_TimeToTrigger_ms160;
      break;

    case 256:
      *ttt = LTE_TimeToTrigger_ms256;
      break;

    case 320:
      *ttt = LTE_TimeToTrigger_ms320;
      break;

    case 480:
      *ttt = LTE_TimeToTrigger_ms480;
      break;

    case 512:
      *ttt = LTE_TimeToTrigger_ms512;
      break;

    case 640:
      *ttt = LTE_TimeToTrigger_ms640;
      break;

    case 1024:
      *ttt = LTE_TimeToTrigger_ms1024;
      break;

    case 1280:
      *ttt = LTE_TimeToTrigger_ms1280;
      break;

    case 2560:
      *ttt = LTE_TimeToTrigger_ms2560;
      break;

    case 5120:
      *ttt = LTE_TimeToTrigger_ms5120;
      break;

    default:
      return -1;
  }

  return 0;
}

int flexran_set_rrc_a3_event_maxReportCells(mid_t mod_id, rnti_t rnti, long maxReportCells) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->a3_event) return -1;

  if (!((maxReportCells >= 1) && (maxReportCells <= 8))) return -1;

  ue_context_p->ue_context.measurement_info->events->a3_event->maxReportCells = maxReportCells;
  return 0;
}

int flexran_set_rrc_a3_event_a3_offset(mid_t mod_id, rnti_t rnti, long a3_offset) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->a3_event) return -1;

  if (!((a3_offset >= -30) && (a3_offset <= 30))) return -1;

  ue_context_p->ue_context.measurement_info->events->a3_event->a3_offset = a3_offset;
  return 0;
}

int flexran_set_rrc_a3_event_reportOnLeave(mid_t mod_id, rnti_t rnti, int reportOnLeave) {
  if (!rrc_is_present(mod_id)) return -1;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return -1;

  if (!ue_context_p->ue_context.measurement_info) return -1;

  if (!ue_context_p->ue_context.measurement_info->events) return -1;

  if (!ue_context_p->ue_context.measurement_info->events->a3_event) return -1;

  if (!((reportOnLeave == 0) || (reportOnLeave == 1))) return -1;

  ue_context_p->ue_context.measurement_info->events->a3_event->reportOnLeave = reportOnLeave;
  return 0;
}

int flexran_set_x2_ho_net_control(mid_t mod_id, int x2_ho_net_control) {
  if (!rrc_is_present(mod_id)) return -1;

  if (!((x2_ho_net_control == 0) || (x2_ho_net_control == 1))) return -1;

  RC.rrc[mod_id]->x2_ho_net_control = x2_ho_net_control;
  return 0;
}

int flexran_get_x2_ho_net_control(mid_t mod_id) {
  if (!rrc_is_present(mod_id)) return -1;

  return RC.rrc[mod_id]->x2_ho_net_control;
}

uint8_t flexran_get_rrc_num_plmn_ids(mid_t mod_id) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->configuration.num_plmn;
}

uint16_t flexran_get_rrc_mcc(mid_t mod_id, uint8_t index) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->configuration.mcc[index];
}

uint16_t flexran_get_rrc_mnc(mid_t mod_id, uint8_t index) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->configuration.mnc[index];
}

uint8_t flexran_get_rrc_mnc_digit_length(mid_t mod_id, uint8_t index) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->configuration.mnc_digit_length[index];
}

int flexran_get_rrc_num_adj_cells(mid_t mod_id) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->num_neigh_cells;
}

int flexran_agent_rrc_gtp_num_e_rab(mid_t mod_id, rnti_t rnti) {
  if (!rrc_is_present(mod_id)) return 0;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return 0;

  return ue_context_p->ue_context.setup_e_rabs;
}

int flexran_agent_rrc_gtp_get_e_rab_id(mid_t mod_id, rnti_t rnti, int index) {
  if (!rrc_is_present(mod_id)) return 0;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return 0;

  return ue_context_p->ue_context.e_rab[index].param.e_rab_id;
}

int flexran_agent_rrc_gtp_get_teid_enb(mid_t mod_id, rnti_t rnti, int index) {
  if (!rrc_is_present(mod_id)) return 0;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return 0;

  return ue_context_p->ue_context.enb_gtp_teid[index];
}

int flexran_agent_rrc_gtp_get_teid_sgw(mid_t mod_id, rnti_t rnti, int index) {
  if (!rrc_is_present(mod_id)) return 0;

  struct rrc_eNB_ue_context_s *ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);

  if (!ue_context_p) return 0;

  return ue_context_p->ue_context.e_rab[index].param.gtp_teid;
}

uint32_t flexran_get_rrc_enb_ue_s1ap_id(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return 0;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  return ue_context_p->ue_context.eNB_ue_s1ap_id;
}

/**************************** SLICING ****************************/
Protocol__FlexSliceAlgorithm flexran_get_dl_slice_algo(mid_t mod_id) {
  if (!mac_is_present(mod_id)) return PROTOCOL__FLEX_SLICE_ALGORITHM__None;

  switch (RC.mac[mod_id]->pre_processor_dl.algorithm) {
    case STATIC_SLICING:
      return PROTOCOL__FLEX_SLICE_ALGORITHM__Static;
    default:
      return PROTOCOL__FLEX_SLICE_ALGORITHM__None;
  }
}

int flexran_set_dl_slice_algo(mid_t mod_id, Protocol__FlexSliceAlgorithm algo) {
  if (!mac_is_present(mod_id)) return 0;
  eNB_MAC_INST *mac = RC.mac[mod_id];
  const int cc_id = 0;

  pp_impl_param_t dl = mac->pre_processor_dl;
  switch (algo) {
    case PROTOCOL__FLEX_SLICE_ALGORITHM__Static:
      mac->pre_processor_dl = static_dl_init(mod_id, cc_id);
      break;
    default:
      mac->pre_processor_dl.algorithm = 0;
      mac->pre_processor_dl.dl = dlsch_scheduler_pre_processor;
      mac->pre_processor_dl.dl_algo.data = mac->pre_processor_dl.dl_algo.setup();
      mac->pre_processor_dl.slices = NULL;
      break;
  }
  if (dl.slices)
    dl.destroy(&dl.slices);
  if (dl.dl_algo.data)
    dl.dl_algo.unset(&dl.dl_algo.data);
  return 1;
}

Protocol__FlexSliceAlgorithm flexran_get_ul_slice_algo(mid_t mod_id) {
  if (!mac_is_present(mod_id)) return PROTOCOL__FLEX_SLICE_ALGORITHM__None;

  switch (RC.mac[mod_id]->pre_processor_ul.algorithm) {
    case STATIC_SLICING:
      return PROTOCOL__FLEX_SLICE_ALGORITHM__Static;
    default:
      return PROTOCOL__FLEX_SLICE_ALGORITHM__None;
  }
}

int flexran_set_ul_slice_algo(mid_t mod_id, Protocol__FlexSliceAlgorithm algo) {
  if (!mac_is_present(mod_id)) return 0;
  eNB_MAC_INST *mac = RC.mac[mod_id];
  const int cc_id = 0;

  pp_impl_param_t ul = mac->pre_processor_ul;
  switch (algo) {
    case PROTOCOL__FLEX_SLICE_ALGORITHM__Static:
      mac->pre_processor_ul = static_ul_init(mod_id, cc_id);
      break;
    default:
      mac->pre_processor_ul.algorithm = 0;
      mac->pre_processor_ul.ul = ulsch_scheduler_pre_processor;
      mac->pre_processor_ul.ul_algo.data = mac->pre_processor_ul.ul_algo.setup();
      mac->pre_processor_ul.slices = NULL;
      break;
  }
  if (ul.slices)
    ul.destroy(&ul.slices);
  if (ul.ul_algo.data)
    ul.ul_algo.unset(&ul.ul_algo.data);
  return 1;
}

int flexran_get_ue_dl_slice_id(mid_t mod_id, mid_t ue_id) {
  if (!mac_is_present(mod_id)) return -1;
  slice_info_t *slices = RC.mac[mod_id]->pre_processor_dl.slices;
  if (!slices) return -1;
  const int idx = slices->UE_assoc_slice[ue_id];
  return slices->s[idx]->id;
}

int flexran_set_ue_dl_slice_id(mid_t mod_id, mid_t ue_id, slice_id_t slice_id) {
  if (!mac_is_present(mod_id)) return 0;
  int idx = flexran_find_dl_slice(mod_id, slice_id);
  if (idx < 0) return 0;
  pp_impl_param_t *dl = &RC.mac[mod_id]->pre_processor_dl;
  dl->move_UE(dl->slices, ue_id, idx);
  return 1;
}

int flexran_get_ue_ul_slice_id(mid_t mod_id, mid_t ue_id) {
  if (!mac_is_present(mod_id)) return -1;
  slice_info_t *slices = RC.mac[mod_id]->pre_processor_ul.slices;
  if (!slices) return -1;
  const int idx = slices->UE_assoc_slice[ue_id];
  return slices->s[idx]->id;
}

int flexran_set_ue_ul_slice_id(mid_t mod_id, mid_t ue_id, slice_id_t slice_id) {
  if (!mac_is_present(mod_id)) return 0;
  int idx = flexran_find_ul_slice(mod_id, slice_id);
  if (idx < 0) return 0;
  pp_impl_param_t *ul = &RC.mac[mod_id]->pre_processor_ul;
  ul->move_UE(ul->slices, ue_id, idx);
  return 1;
}

int flexran_create_dl_slice(mid_t mod_id, const Protocol__FlexSlice *s, void *object) {
  if (!mac_is_present(mod_id)) return 0;
  void *params = NULL;
  switch (s->params_case) {
    case PROTOCOL__FLEX_SLICE__PARAMS_STATIC:
      params = malloc(sizeof(static_slice_param_t));
      if (!params) return 0;
      ((static_slice_param_t *)params)->posLow = s->static_->poslow;
      ((static_slice_param_t *)params)->posHigh = s->static_->poshigh;
      break;
    default:
      break;
  }
  pp_impl_param_t *dl = &RC.mac[mod_id]->pre_processor_dl;
  char *l = s->label ? strdup(s->label) : NULL;
  void *algo = &dl->dl_algo; // default scheduler
  if (s->scheduler) {
    algo = dlsym(object, s->scheduler);
    if (!algo) {
      free(params);
      LOG_E(FLEXRAN_AGENT, "cannot locate scheduler '%s'\n", s->scheduler);
      return -15;
    }
  }
  return dl->addmod_slice(dl->slices, s->id, l, algo, params);
}

int flexran_remove_dl_slice(mid_t mod_id, const Protocol__FlexSlice *s) {
  if (!mac_is_present(mod_id)) return 0;
  const int idx = flexran_find_dl_slice(mod_id, s->id);
  if (idx < 0) return 0;
  pp_impl_param_t *dl = &RC.mac[mod_id]->pre_processor_dl;
  return dl->remove_slice(dl->slices, idx);
}

int flexran_find_dl_slice(mid_t mod_id, slice_id_t slice_id) {
  if (!mac_is_present(mod_id)) return -1;
  slice_info_t *si = RC.mac[mod_id]->pre_processor_dl.slices;
  for (int i = 0; i < si->num; ++i)
    if (si->s[i]->id == slice_id)
      return i;
  return -1;
}

void flexran_get_dl_slice(mid_t mod_id,
                          int slice_idx,
                          Protocol__FlexSlice *slice,
                          Protocol__FlexSliceAlgorithm algo) {
  if (!mac_is_present(mod_id)) return;
  slice_t *s_ = RC.mac[mod_id]->pre_processor_dl.slices->s[slice_idx];
  slice->has_id = 1;
  slice->id = s_->id;
  slice->label = s_->label;
  slice->scheduler = s_->dl_algo.name;
  slice->params_case = PROTOCOL__FLEX_SLICE__PARAMS__NOT_SET;
  switch (algo) {
    case PROTOCOL__FLEX_SLICE_ALGORITHM__Static:
      slice->static_ = malloc(sizeof(Protocol__FlexSliceStatic));
      if (!slice->static_) return;
      protocol__flex_slice_static__init(slice->static_);
      slice->static_->has_poslow = 1;
      slice->static_->poslow = ((static_slice_param_t *)s_->algo_data)->posLow;
      slice->static_->has_poshigh = 1;
      slice->static_->poshigh = ((static_slice_param_t *)s_->algo_data)->posHigh;
      slice->params_case = PROTOCOL__FLEX_SLICE__PARAMS_STATIC;
      break;
    default:
      break;
  }
}

int flexran_get_num_dl_slices(mid_t mod_id) {
  if (!mac_is_present(mod_id)) return 0;
  if (!RC.mac[mod_id]->pre_processor_dl.slices) return 0;
  return RC.mac[mod_id]->pre_processor_dl.slices->num;
}

int flexran_create_ul_slice(mid_t mod_id, const Protocol__FlexSlice *s, void *object) {
  if (!mac_is_present(mod_id)) return -1;
  void *params = NULL;
  switch (s->params_case) {
    case PROTOCOL__FLEX_SLICE__PARAMS_STATIC:
      params = malloc(sizeof(static_slice_param_t));
      if (!params) return 0;
      ((static_slice_param_t *)params)->posLow = s->static_->poslow;
      ((static_slice_param_t *)params)->posHigh = s->static_->poshigh;
      break;
    default:
      break;
  }
  pp_impl_param_t *ul = &RC.mac[mod_id]->pre_processor_ul;
  char *l = s->label ? strdup(s->label) : NULL;
  void *algo = &ul->ul_algo; // default scheduler
  if (s->scheduler) {
    algo = dlsym(object, s->scheduler);
    if (!algo) {
      free(params);
      LOG_E(FLEXRAN_AGENT, "cannot locate scheduler '%s'\n", s->scheduler);
      return -15;
    }
  }
  return ul->addmod_slice(ul->slices, s->id, l, algo, params);
}

int flexran_remove_ul_slice(mid_t mod_id, const Protocol__FlexSlice *s) {
  if (!mac_is_present(mod_id)) return 0;
  const int idx = flexran_find_ul_slice(mod_id, s->id);
  if (idx < 0) return 0;
  pp_impl_param_t *ul = &RC.mac[mod_id]->pre_processor_ul;
  return ul->remove_slice(ul->slices, idx);
}

int flexran_find_ul_slice(mid_t mod_id, slice_id_t slice_id) {
  if (!mac_is_present(mod_id)) return -1;
  slice_info_t *si = RC.mac[mod_id]->pre_processor_ul.slices;
  for (int i = 0; i < si->num; ++i)
    if (si->s[i]->id == slice_id)
      return i;
  return -1;
}

void flexran_get_ul_slice(mid_t mod_id,
                          int slice_idx,
                          Protocol__FlexSlice *slice,
                          Protocol__FlexSliceAlgorithm algo) {
  if (!mac_is_present(mod_id)) return;
  slice_t *s_ = RC.mac[mod_id]->pre_processor_ul.slices->s[slice_idx];
  slice->has_id = 1;
  slice->id = s_->id;
  slice->label = s_->label;
  slice->scheduler = s_->ul_algo.name;
  slice->params_case = PROTOCOL__FLEX_SLICE__PARAMS__NOT_SET;
  switch (algo) {
    case PROTOCOL__FLEX_SLICE_ALGORITHM__Static:
      slice->static_ = malloc(sizeof(Protocol__FlexSliceStatic));
      if (!slice->static_) return;
      protocol__flex_slice_static__init(slice->static_);
      slice->static_->has_poslow = 1;
      slice->static_->poslow = ((static_slice_param_t *)s_->algo_data)->posLow;
      slice->static_->has_poshigh = 1;
      slice->static_->poshigh = ((static_slice_param_t *)s_->algo_data)->posHigh;
      slice->params_case = PROTOCOL__FLEX_SLICE__PARAMS_STATIC;
      break;
    default:
      break;
  }

}

int flexran_get_num_ul_slices(mid_t mod_id) {
  if (!mac_is_present(mod_id)) return 0;
  if (!RC.mac[mod_id]->pre_processor_ul.slices) return 0;
  return RC.mac[mod_id]->pre_processor_ul.slices->num;
}

char *flexran_get_dl_scheduler_name(mid_t mod_id) {
  if (!mac_is_present(mod_id)) return NULL;
  return RC.mac[mod_id]->pre_processor_dl.dl_algo.name;
}

int flexran_set_dl_scheduler(mid_t mod_id, char *sched, void *object) {
  if (!mac_is_present(mod_id)) return -1;
  void *d = dlsym(object, sched);
  if (!d) return -2;
  pp_impl_param_t *dl_pp = &RC.mac[mod_id]->pre_processor_dl;
  dl_pp->dl_algo.unset(&dl_pp->dl_algo.data);
  dl_pp->dl_algo = *(default_sched_dl_algo_t *) d;
  dl_pp->dl_algo.data = dl_pp->dl_algo.setup();
  return 0;
}

char *flexran_get_ul_scheduler_name(mid_t mod_id) {
  if (!mac_is_present(mod_id)) return NULL;
  return RC.mac[mod_id]->pre_processor_ul.ul_algo.name;
}

int flexran_set_ul_scheduler(mid_t mod_id, char *sched, void *object) {
  if (!mac_is_present(mod_id)) return -1;
  void *d = dlsym(object, sched);
  if (!d) return -2;
  pp_impl_param_t *ul_pp = &RC.mac[mod_id]->pre_processor_ul;
  ul_pp->ul_algo.unset(&ul_pp->ul_algo.data);
  ul_pp->ul_algo = *(default_sched_ul_algo_t *) d;
  ul_pp->ul_algo.data = ul_pp->ul_algo.setup();
  return 0;
}

/************************** S1AP **************************/
int flexran_get_s1ap_mme_pending(mid_t mod_id){
  if (!rrc_is_present(mod_id)) return -1;
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap) return -1;
  return s1ap->s1ap_mme_pending_nb;
}

int flexran_get_s1ap_mme_connected(mid_t mod_id){
  if (!rrc_is_present(mod_id)) return -1;
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap) return -1;
  return s1ap->s1ap_mme_associated_nb;
}

char* flexran_get_s1ap_enb_s1_ip(mid_t mod_id){
  if (!rrc_is_present(mod_id)) return NULL;
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap) return NULL;
  if (s1ap->eNB_s1_ip.ipv4)
    return &s1ap->eNB_s1_ip.ipv4_address[0];
  if (s1ap->eNB_s1_ip.ipv6)
    return &s1ap->eNB_s1_ip.ipv6_address[0];
  return NULL;
}

char* flexran_get_s1ap_enb_name(mid_t mod_id){
  if (!rrc_is_present(mod_id)) return NULL;
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap) return NULL;
  return s1ap->eNB_name;
}

int flexran_get_s1ap_nb_mme(mid_t mod_id) {
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap) return 0;
  struct s1ap_eNB_mme_data_s *mme = NULL;
  int count = 0;
  RB_FOREACH(mme, s1ap_mme_map, &s1ap->s1ap_mme_head) {
    count++;
  }
  return count;
}

int flexran_get_s1ap_nb_ue(mid_t mod_id) {
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap) return 0;
  struct s1ap_eNB_ue_context_s *ue = NULL;
  int count = 0;
  RB_FOREACH(ue, s1ap_ue_map, &s1ap->s1ap_ue_head) {
    count++;
  }
  return count;
}

int flexran_get_s1ap_mme_conf(mid_t mod_id, mid_t mme_index, Protocol__FlexS1apMme * mme_conf){
  if (!rrc_is_present(mod_id)) return -1;
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap) return -1;

  struct served_gummei_s   *gummei_p = NULL;
  struct plmn_identity_s   *served_plmn_p = NULL;
  struct served_group_id_s *group_id_p = NULL;
  struct mme_code_s        *mme_code_p = NULL;
  int i = 0;
  Protocol__FlexGummei  **served_gummeis;
  Protocol__FlexPlmn    **requested_plmns;

  struct s1ap_eNB_mme_data_s *mme = NULL;

  RB_FOREACH(mme, s1ap_mme_map, &s1ap->s1ap_mme_head){
    if (mme_index == 0) break;
    mme_index--;
  }
  if (mme_index > 0) return -1;

  if (mme->mme_s1_ip.ipv4) {
    mme_conf->s1_ip = (char*) &mme->mme_s1_ip.ipv4_address[0];
  } else if (mme->mme_s1_ip.ipv6) {
    mme_conf->s1_ip = (char*) &mme->mme_s1_ip.ipv6_address[0];
  }
  mme_conf->name = mme->mme_name;
  mme_conf->has_state = 1;
  mme_conf->state = mme->state;

  mme_conf->n_served_gummeis = 0;
  STAILQ_FOREACH(gummei_p, &mme->served_gummei, next) {
    mme_conf->n_served_gummeis++;
  }
  if (mme_conf->n_served_gummeis > 0) {
    served_gummeis = calloc(mme_conf->n_served_gummeis, sizeof(Protocol__FlexGummei*));
    if(served_gummeis == NULL) return -1;

    STAILQ_FOREACH(gummei_p, &mme->served_gummei, next) {
      served_plmn_p = STAILQ_FIRST(&gummei_p->served_plmns);
      group_id_p = STAILQ_FIRST(&gummei_p->served_group_ids);
      mme_code_p = STAILQ_FIRST(&gummei_p->mme_codes);

      served_gummeis[i] = malloc(sizeof(Protocol__FlexGummei));
      if (!served_gummeis[i]) return -1;
      protocol__flex_gummei__init(served_gummeis[i]);
      served_gummeis[i]->plmn = malloc(sizeof(Protocol__FlexPlmn));
      if (!served_gummeis[i]->plmn) return -1;
      protocol__flex_plmn__init(served_gummeis[i]->plmn);

      if (served_plmn_p) {
        served_gummeis[i]->plmn->has_mcc = 1;
        served_gummeis[i]->plmn->mcc = served_plmn_p->mcc;
        served_gummeis[i]->plmn->has_mnc = 1;
        served_gummeis[i]->plmn->mnc = served_plmn_p->mnc;
        served_gummeis[i]->plmn->has_mnc_length = 1;
        served_gummeis[i]->plmn->mnc_length = served_plmn_p-> mnc_digit_length;
        STAILQ_NEXT(served_plmn_p, next);
      }
      if (group_id_p) {
        served_gummeis[i]->has_mme_group_id = 1;
        served_gummeis[i]->mme_group_id = group_id_p->mme_group_id;
        STAILQ_NEXT(group_id_p, next);
      }
      if (mme_code_p){
        served_gummeis[i]->has_mme_code = 1;
        served_gummeis[i]->mme_code = mme_code_p->mme_code;
        STAILQ_NEXT(mme_code_p, next);
      }
      i++;
    }

    mme_conf->served_gummeis = served_gummeis;
  }

  // requested PLMNS
  mme_conf->n_requested_plmns = mme->broadcast_plmn_num;
  if (mme_conf->n_requested_plmns > 0){
    requested_plmns = calloc(mme_conf->n_requested_plmns, sizeof(Protocol__FlexPlmn*));
    if(requested_plmns == NULL) return -1;
    for(int i = 0; i < mme_conf->n_requested_plmns; i++) {
      requested_plmns[i] = malloc(sizeof(Protocol__FlexPlmn));
      if (!requested_plmns[i]) return -1;
      protocol__flex_plmn__init(requested_plmns[i]);
      requested_plmns[i]->mcc = s1ap->mcc[mme->broadcast_plmn_index[i]];
      requested_plmns[i]->has_mcc = 1;
      requested_plmns[i]->mnc = s1ap->mnc[mme->broadcast_plmn_index[i]];
      requested_plmns[i]->has_mnc = 1;
      requested_plmns[i]->mnc_length = s1ap->mnc_digit_length[mme->broadcast_plmn_index[i]];
      requested_plmns[i]->has_mnc_length = 1;
    }
    mme_conf->requested_plmns = requested_plmns;
  }

  mme_conf->has_rel_capacity = 1;
  mme_conf->rel_capacity = mme->relative_mme_capacity;
  return 0;
}

int flexran_add_s1ap_mme(mid_t mod_id, size_t n_mme, char **mme_ipv4) {
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap) return -1;
  if (!rrc_is_present(mod_id)) return -2;

  /* Reconstruct S1AP_REGISTER_ENB_REQ */
  MessageDef *m = itti_alloc_new_message(TASK_FLEXRAN_AGENT, 0, S1AP_REGISTER_ENB_REQ);
  RCconfig_S1(m, mod_id);

  const int CC_id = 0;
  eNB_RRC_INST *rrc = RC.rrc[CC_id];
  RrcConfigurationReq *conf = &rrc->configuration;
  S1AP_REGISTER_ENB_REQ(m).num_plmn = conf->num_plmn;
  for (int i = 0; i < conf->num_plmn; ++i) {
    S1AP_REGISTER_ENB_REQ(m).mcc[i] = conf->mcc[i];
    S1AP_REGISTER_ENB_REQ(m).mnc[i] = conf->mnc[i];
    S1AP_REGISTER_ENB_REQ(m).mnc_digit_length[i] = conf->mnc_digit_length[i];
  }

  /* reconstruct MME list, it might have been updated since initial
   * configuration */
  S1AP_REGISTER_ENB_REQ(m).nb_mme = 0;
  struct s1ap_eNB_mme_data_s *mme = NULL;
  RB_FOREACH(mme, s1ap_mme_map, &s1ap->s1ap_mme_head) {
    const int n = S1AP_REGISTER_ENB_REQ(m).nb_mme;
    S1AP_REGISTER_ENB_REQ(m).mme_ip_address[n].ipv4 = mme->mme_s1_ip.ipv4;
    strcpy(S1AP_REGISTER_ENB_REQ(m).mme_ip_address[n].ipv4_address, mme->mme_s1_ip.ipv4_address);
    S1AP_REGISTER_ENB_REQ(m).mme_ip_address[n].ipv6 = mme->mme_s1_ip.ipv6;
    strcpy(S1AP_REGISTER_ENB_REQ(m).mme_ip_address[n].ipv6_address, mme->mme_s1_ip.ipv6_address);
    S1AP_REGISTER_ENB_REQ(m).broadcast_plmn_num[n] = mme->broadcast_plmn_num;
    for (int i = 0; i < mme->broadcast_plmn_num; ++i)
      S1AP_REGISTER_ENB_REQ(m).broadcast_plmn_index[n][i] = mme->broadcast_plmn_index[i];
    S1AP_REGISTER_ENB_REQ(m).mme_port[n] = mme->mme_port;
    S1AP_REGISTER_ENB_REQ(m).nb_mme += 1;
  }

  if (S1AP_REGISTER_ENB_REQ(m).nb_mme + n_mme > S1AP_MAX_NB_MME_IP_ADDRESS)
    return -1;

  const int n = S1AP_REGISTER_ENB_REQ(m).nb_mme;
  strcpy(S1AP_REGISTER_ENB_REQ(m).mme_ip_address[n].ipv4_address, mme_ipv4[0]);
  S1AP_REGISTER_ENB_REQ(m).mme_ip_address[n].ipv4 = 1;
  S1AP_REGISTER_ENB_REQ(m).mme_ip_address[n].ipv6 = 0;
  S1AP_REGISTER_ENB_REQ(m).broadcast_plmn_num[n] = S1AP_REGISTER_ENB_REQ(m).num_plmn;
  for (int i = 0; i < S1AP_REGISTER_ENB_REQ(m).num_plmn; ++i)
    S1AP_REGISTER_ENB_REQ(m).broadcast_plmn_index[n][i] = i;
  S1AP_REGISTER_ENB_REQ(m).mme_port[n] = S1AP_PORT_NUMBER;
  S1AP_REGISTER_ENB_REQ(m).nb_mme += 1;

  itti_send_msg_to_task (TASK_S1AP, ENB_MODULE_ID_TO_INSTANCE(mod_id), m);

  return 0;
}

int flexran_remove_s1ap_mme(mid_t mod_id, size_t n_mme, char **mme_ipv4) {
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap) return -1;
  struct s1ap_eNB_mme_data_s *mme = NULL;
  RB_FOREACH(mme, s1ap_mme_map, &s1ap->s1ap_mme_head) {
    if (mme->mme_s1_ip.ipv4
        && strncmp(mme->mme_s1_ip.ipv4_address, mme_ipv4[0], 16) == 0)
        break;
  }
  if (!mme)
    return -2;

  MessageDef *m = itti_alloc_new_message(TASK_FLEXRAN_AGENT, 0, SCTP_CLOSE_ASSOCIATION);
  SCTP_CLOSE_ASSOCIATION(m).assoc_id = mme->assoc_id;
  itti_send_msg_to_task (TASK_SCTP, ENB_MODULE_ID_TO_INSTANCE(mod_id), m);

  switch (mme->state) {
    case S1AP_ENB_STATE_WAITING:
      s1ap->s1ap_mme_nb -= 1;
      if (s1ap->s1ap_mme_pending_nb > 0)
        s1ap->s1ap_mme_pending_nb -= 1;
      break;
    case S1AP_ENB_STATE_CONNECTED:
    case S1AP_ENB_OVERLOAD: /* I am not sure the decrements are right here */
      s1ap->s1ap_mme_nb -= 1;
      s1ap->s1ap_mme_associated_nb -= 1;
      break;
    case S1AP_ENB_STATE_DISCONNECTED:
    default:
      break;
  }
  RB_REMOVE(s1ap_mme_map, &s1ap->s1ap_mme_head, mme);

  return 0;
}

int flexran_set_new_plmn_id(mid_t mod_id, int CC_id, size_t n_plmn, Protocol__FlexPlmn **plmn_id) {
  if (!rrc_is_present(mod_id))
    return -1;
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap)
    return -2;

  eNB_RRC_INST *rrc = RC.rrc[CC_id];
  RrcConfigurationReq *conf = &rrc->configuration;

  uint8_t num_plmn_old = conf->num_plmn;
  uint16_t mcc[PLMN_LIST_MAX_SIZE];
  uint16_t mnc[PLMN_LIST_MAX_SIZE];
  uint8_t mnc_digit_length[PLMN_LIST_MAX_SIZE];
  for (int i = 0; i < num_plmn_old; ++i) {
    mcc[i] = conf->mcc[i];
    mnc[i] = conf->mnc[i];
    mnc_digit_length[i] = conf->mnc_digit_length[i];
  }

  conf->num_plmn = (uint8_t) n_plmn;
  for (int i = 0; i < conf->num_plmn; ++i) {
    conf->mcc[i] = plmn_id[i]->mcc;
    conf->mnc[i] = plmn_id[i]->mnc;
    conf->mnc_digit_length[i] = plmn_id[i]->mnc_length;
  }

  rrc_eNB_carrier_data_t *carrier = &rrc->carrier[CC_id];
  extern uint8_t do_SIB1(rrc_eNB_carrier_data_t *carrier,
                         int Mod_id,
                         int CC_id,
                         BOOLEAN_t brOption,
                         RrcConfigurationReq *configuration);
  carrier->sizeof_SIB1 = do_SIB1(carrier, mod_id, CC_id, false, conf);
  if (carrier->sizeof_SIB1 < 0)
    return -1337; /* SIB1 encoding failed, hell will probably break loose */

  s1ap->num_plmn = (uint8_t) n_plmn;
  for (int i = 0; i < conf->num_plmn; ++i) {
    s1ap->mcc[i] = plmn_id[i]->mcc;
    s1ap->mnc[i] = plmn_id[i]->mnc;
    s1ap->mnc_digit_length[i] = plmn_id[i]->mnc_length;
  }

  int bpn_failed = 0;
  struct s1ap_eNB_mme_data_s *mme = NULL;
  RB_FOREACH(mme, s1ap_mme_map, &s1ap->s1ap_mme_head) {
    for (int i = 0; i < mme->broadcast_plmn_num; ++i) {
      /* search the new index and update. If we don't find, we count this using
       * bpn_failed to now how many broadcast_plmns could not be updated */
      int idx = mme->broadcast_plmn_index[i];
      int omcc = mcc[idx];
      int omnc = mnc[idx];
      int omncl = mnc_digit_length[idx];
      int j = 0;
      for (j = 0; j < s1ap->num_plmn; ++j) {
        if (s1ap->mcc[j] == omcc
            && s1ap->mnc[j] == omnc
            && s1ap->mnc_digit_length[j] == omncl) {
          mme->broadcast_plmn_index[i] = j;
          break;
        }
      }
      if (j == s1ap->num_plmn) /* could not find the old PLMN in the new ones */
        bpn_failed++;
    }
  }
  if (bpn_failed > 0)
    return -10000 - bpn_failed;
  return 0;
}

int flexran_get_s1ap_ue(mid_t mod_id, rnti_t rnti, Protocol__FlexS1apUe * ue_conf){
  if (!rrc_is_present(mod_id)) return -1;
  s1ap_eNB_instance_t *s1ap = s1ap_eNB_get_instance(mod_id);
  if (!s1ap) return -1;

  uint32_t enb_ue_s1ap_id = flexran_get_rrc_enb_ue_s1ap_id(mod_id, rnti);
  struct s1ap_eNB_ue_context_s *ue = NULL;
  RB_FOREACH(ue, s1ap_ue_map, &s1ap->s1ap_ue_head){
    if (ue->eNB_ue_s1ap_id == enb_ue_s1ap_id) break;
  }
  if (!ue) return 0; // UE does not exist: it might be connected but CN did not answer

  if (ue->mme_ref->mme_s1_ip.ipv4)
    ue_conf->mme_s1_ip = (char*) &ue->mme_ref->mme_s1_ip.ipv4_address[0];
  else if (ue->mme_ref->mme_s1_ip.ipv6)
    ue_conf->mme_s1_ip = (char*) &ue->mme_ref->mme_s1_ip.ipv6_address[0];

  ue_conf->has_enb_ue_s1ap_id = 1;
  ue_conf->enb_ue_s1ap_id = ue->eNB_ue_s1ap_id;
  ue_conf->has_mme_ue_s1ap_id = 1;
  ue_conf->mme_ue_s1ap_id = ue->mme_ue_s1ap_id;

  ue_conf->selected_plmn = malloc(sizeof(Protocol__FlexPlmn));
  if (!ue_conf->selected_plmn) return -1;
  protocol__flex_plmn__init(ue_conf->selected_plmn);

  ue_conf->selected_plmn->has_mcc = 1;
  ue_conf->selected_plmn->mcc = s1ap->mcc[ue->selected_plmn_identity];
  ue_conf->selected_plmn->has_mnc = 1;
  ue_conf->selected_plmn->mnc = s1ap->mnc[ue->selected_plmn_identity];
  ue_conf->selected_plmn->has_mnc_length = 1;
  ue_conf->selected_plmn->mnc_length = s1ap->mnc_digit_length[ue->selected_plmn_identity];
  return 0;
}

/**************************** General BS info  ****************************/
uint64_t flexran_get_bs_id(mid_t mod_id) {
  if (!rrc_is_present(mod_id)) return 0;

  return RC.rrc[mod_id]->nr_cellid;
}

size_t flexran_get_capabilities(mid_t mod_id, Protocol__FlexBsCapability **caps) {
  if (!caps) return 0;

  if (!rrc_is_present(mod_id)) return 0;

  size_t n_caps = 0;

  switch (RC.rrc[mod_id]->node_type) {
    case ngran_eNB_CU:
    case ngran_ng_eNB_CU:
    case ngran_gNB_CU:
      n_caps = 4;
      *caps = calloc(n_caps, sizeof(Protocol__FlexBsCapability));
      AssertFatal(*caps, "could not allocate %zu bytes for Protocol__FlexBsCapability array\n",
                  n_caps * sizeof(Protocol__FlexBsCapability));
      (*caps)[0] = PROTOCOL__FLEX_BS_CAPABILITY__PDCP;
      (*caps)[1] = PROTOCOL__FLEX_BS_CAPABILITY__SDAP;
      (*caps)[2] = PROTOCOL__FLEX_BS_CAPABILITY__RRC;
      (*caps)[3] = PROTOCOL__FLEX_BS_CAPABILITY__S1AP;
      break;
    case ngran_eNB_DU:
    case ngran_gNB_DU:
      n_caps = 5;
      *caps = calloc(n_caps, sizeof(Protocol__FlexBsCapability));
      AssertFatal(*caps, "could not allocate %zu bytes for Protocol__FlexBsCapability array\n",
                  n_caps * sizeof(Protocol__FlexBsCapability));
      (*caps)[0] = PROTOCOL__FLEX_BS_CAPABILITY__LOPHY;
      (*caps)[1] = PROTOCOL__FLEX_BS_CAPABILITY__HIPHY;
      (*caps)[2] = PROTOCOL__FLEX_BS_CAPABILITY__LOMAC;
      (*caps)[3] = PROTOCOL__FLEX_BS_CAPABILITY__HIMAC;
      (*caps)[4] = PROTOCOL__FLEX_BS_CAPABILITY__RLC;
      break;
    case ngran_eNB:
    case ngran_ng_eNB:
    case ngran_gNB:
      n_caps = 9;
      *caps = calloc(n_caps, sizeof(Protocol__FlexBsCapability));
      AssertFatal(*caps, "could not allocate %zu bytes for Protocol__FlexBsCapability array\n",
                  n_caps * sizeof(Protocol__FlexBsCapability));
      (*caps)[0] = PROTOCOL__FLEX_BS_CAPABILITY__LOPHY;
      (*caps)[1] = PROTOCOL__FLEX_BS_CAPABILITY__HIPHY;
      (*caps)[2] = PROTOCOL__FLEX_BS_CAPABILITY__LOMAC;
      (*caps)[3] = PROTOCOL__FLEX_BS_CAPABILITY__HIMAC;
      (*caps)[4] = PROTOCOL__FLEX_BS_CAPABILITY__RLC;
      (*caps)[5] = PROTOCOL__FLEX_BS_CAPABILITY__PDCP;
      (*caps)[6] = PROTOCOL__FLEX_BS_CAPABILITY__SDAP;
      (*caps)[7] = PROTOCOL__FLEX_BS_CAPABILITY__RRC;
      (*caps)[8] = PROTOCOL__FLEX_BS_CAPABILITY__S1AP;
      break;
    case ngran_eNB_MBMS_STA:
      AssertFatal(0, "MBMS STA not supported by FlexRAN!\n");
     break;
  }

  return n_caps;
}

uint32_t flexran_get_capabilities_mask(mid_t mod_id) {
  if (!rrc_is_present(mod_id)) return 0;
  uint32_t mask = 0;
  switch (RC.rrc[mod_id]->node_type) {
    case ngran_eNB_CU:
    case ngran_ng_eNB_CU:
    case ngran_gNB_CU:
      mask = (1 << PROTOCOL__FLEX_BS_CAPABILITY__PDCP)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__SDAP)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__RRC)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__S1AP);
      break;
    case ngran_eNB_DU:
    case ngran_gNB_DU:
      mask = (1 << PROTOCOL__FLEX_BS_CAPABILITY__LOPHY)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__HIPHY)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__LOMAC)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__HIMAC)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__RLC);
      break;
    case ngran_eNB:
    case ngran_ng_eNB:
    case ngran_gNB:
      mask = (1 << PROTOCOL__FLEX_BS_CAPABILITY__LOPHY)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__HIPHY)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__LOMAC)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__HIMAC)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__RLC)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__PDCP)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__SDAP)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__RRC)
           | (1 << PROTOCOL__FLEX_BS_CAPABILITY__S1AP);
      break;
    case ngran_eNB_MBMS_STA:
      AssertFatal(0, "MBMS STA not supported by FlexRAN!\n");
     break;
  }

  return mask;
}

size_t flexran_get_splits(mid_t mod_id, Protocol__FlexBsSplit **splits) {
  size_t n_splits = 0;
  *splits = NULL;
  if (rrc_is_present(mod_id) && !NODE_IS_MONOLITHIC(RC.rrc[mod_id]->node_type))
    n_splits++;
  if (NFAPI_MODE != NFAPI_MONOLITHIC)
    n_splits++;
  if (RC.ru && RC.ru[mod_id] && RC.ru[mod_id]->if_south != LOCAL_RF)
    n_splits++;
  if (n_splits == 0)
    return 0;

  AssertFatal(n_splits < 3, "illegal number of splits (%lu)\n", n_splits);
  *splits = calloc(n_splits, sizeof(Protocol__FlexBsSplit));
  AssertFatal(*splits, "could not allocate Protocol__FlexBsSplit array\n");
  int n = 0;
  if (rrc_is_present(mod_id) && !NODE_IS_MONOLITHIC(RC.rrc[mod_id]->node_type))
    (*splits)[n++] = PROTOCOL__FLEX_BS_SPLIT__F1;
  if (NFAPI_MODE != NFAPI_MONOLITHIC)
    (*splits)[n++] = PROTOCOL__FLEX_BS_SPLIT__nFAPI;
  if (RC.ru && RC.ru[mod_id] && RC.ru[mod_id]->if_south == REMOTE_IF4p5)
    (*splits)[n++] = PROTOCOL__FLEX_BS_SPLIT__IF4p5;
  if (RC.ru && RC.ru[mod_id] && RC.ru[mod_id]->if_south == REMOTE_IF5)
    (*splits)[n++] = PROTOCOL__FLEX_BS_SPLIT__IF5;
  DevAssert(n == n_splits);
  return n_splits;
}
