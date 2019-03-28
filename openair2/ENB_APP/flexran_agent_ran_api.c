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

static inline int phy_is_present(mid_t mod_id, uint8_t cc_id)
{
  return RC.eNB && RC.eNB[mod_id] && RC.eNB[mod_id][cc_id];
}

static inline int mac_is_present(mid_t mod_id)
{
  return RC.mac && RC.mac[mod_id];
}

static inline int rrc_is_present(mid_t mod_id)
{
  return RC.rrc && RC.rrc[mod_id];
}

uint32_t flexran_get_current_time_ms(mid_t mod_id, int subframe_flag)
{
  if (!mac_is_present(mod_id)) return 0;
  if (subframe_flag == 1)
    return RC.mac[mod_id]->frame*10 + RC.mac[mod_id]->subframe;
  else
    return RC.mac[mod_id]->frame*10;
}

frame_t flexran_get_current_frame(mid_t mod_id)
{
  if (!mac_is_present(mod_id)) return 0;
  //  #warning "SFN will not be in [0-1023] when oaisim is used"
  return RC.mac[mod_id]->frame;
}

frame_t flexran_get_current_system_frame_num(mid_t mod_id)
{
  return flexran_get_current_frame(mod_id) % 1024;
}

sub_frame_t flexran_get_current_subframe(mid_t mod_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->subframe;
}

/* Why uint16_t, frame_t and sub_frame_t are defined as uint32_t? */
uint16_t flexran_get_sfn_sf(mid_t mod_id)
{
  if (!mac_is_present(mod_id)) return 0;
  frame_t frame = flexran_get_current_system_frame_num(mod_id);
  sub_frame_t subframe = flexran_get_current_subframe(mod_id);
  uint16_t sfn_sf, frame_mask, sf_mask;

  frame_mask = (1 << 12) - 1;
  sf_mask = (1 << 4) - 1;
  sfn_sf = (subframe & sf_mask) | ((frame & frame_mask) << 4);

  return sfn_sf;
}

uint16_t flexran_get_future_sfn_sf(mid_t mod_id, int ahead_of_time)
{
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

int flexran_get_mac_num_ues(mid_t mod_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.num_UEs;
}

int flexran_get_num_ue_lcs(mid_t mod_id, mid_t ue_id)
{
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

int flexran_get_mac_ue_id_rnti(mid_t mod_id, rnti_t rnti)
{
  int n;
  if (!mac_is_present(mod_id)) return 0;
  /* get the (active) UE with RNTI i */
  for (n = 0; n < MAX_MOBILES_PER_ENB; ++n) {
    if (RC.mac[mod_id]->UE_list.active[n] == TRUE
        && rnti == UE_RNTI(mod_id, n)) {
      return n;
    }
  }
  return 0;

}

int flexran_get_mac_ue_id(mid_t mod_id, int i)
{
  int n;
  if (!mac_is_present(mod_id)) return 0;
  /* get the (i+1)'th active UE */
  for (n = 0; n < MAX_MOBILES_PER_ENB; ++n) {
    if (RC.mac[mod_id]->UE_list.active[n] == TRUE) {
      if (i == 0)
        return n;
      --i;
    }
  }
  return 0;
}

rnti_t flexran_get_mac_ue_crnti(mid_t mod_id, mid_t ue_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return UE_RNTI(mod_id, ue_id);
}

int flexran_get_ue_bsr_ul_buffer_info(mid_t mod_id, mid_t ue_id, lcid_t lcid)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->UE_list.UE_template[UE_PCCID(mod_id, ue_id)][ue_id].ul_buffer_info[lcid];
}

int8_t flexran_get_ue_phr(mid_t mod_id, mid_t ue_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.UE_template[UE_PCCID(mod_id, ue_id)][ue_id].phr_info;
}

uint8_t flexran_get_ue_wcqi(mid_t mod_id, mid_t ue_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.UE_sched_ctrl[ue_id].dl_cqi[0];
}

rlc_buffer_occupancy_t flexran_get_tx_queue_size(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id)
{
  if (!mac_is_present(mod_id)) return 0;
  rnti_t rnti = flexran_get_mac_ue_crnti(mod_id, ue_id);
  frame_t frame = flexran_get_current_frame(mod_id);
  sub_frame_t subframe = flexran_get_current_subframe(mod_id);
  mac_rlc_status_resp_t rlc_status = mac_rlc_status_ind(mod_id,rnti, mod_id, frame, subframe, ENB_FLAG_YES,MBMS_FLAG_NO, channel_id, 0
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                                    ,0, 0
#endif
                                                    );
  return rlc_status.bytes_in_buffer;
}

rlc_buffer_occupancy_t flexran_get_num_pdus_buffer(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id)
{
  if (!mac_is_present(mod_id)) return 0;
  rnti_t rnti = flexran_get_mac_ue_crnti(mod_id,ue_id);
  frame_t frame = flexran_get_current_frame(mod_id);
  sub_frame_t subframe = flexran_get_current_subframe(mod_id);
  mac_rlc_status_resp_t rlc_status = mac_rlc_status_ind(mod_id,rnti, mod_id, frame, subframe, ENB_FLAG_YES,MBMS_FLAG_NO, channel_id, 0
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                                    ,0, 0
#endif
                                                    );
  return rlc_status.pdus_in_buffer;
}

frame_t flexran_get_hol_delay(mid_t mod_id, mid_t ue_id, logical_chan_id_t channel_id)
{
  if (!mac_is_present(mod_id)) return 0;
  rnti_t rnti = flexran_get_mac_ue_crnti(mod_id,ue_id);
  frame_t frame = flexran_get_current_frame(mod_id);
  sub_frame_t subframe = flexran_get_current_subframe(mod_id);
  mac_rlc_status_resp_t rlc_status = mac_rlc_status_ind(mod_id, rnti, mod_id, frame, subframe, ENB_FLAG_YES, MBMS_FLAG_NO, channel_id, 0
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
                                                    ,0, 0
#endif
                                                    );
  return rlc_status.head_sdu_creation_time;
}

int32_t flexran_get_TA(mid_t mod_id, mid_t ue_id, uint8_t cc_id)
{
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

uint32_t flexran_get_total_size_dl_mac_sdus(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].total_sdu_bytes;
}

uint32_t flexran_get_total_size_ul_mac_sdus(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  uint64_t bytes = 0;
  for (int i = 0; i < NB_RB_MAX; ++i) {
    bytes += RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].num_bytes_rx[i];
  }
  return bytes;
}

uint32_t flexran_get_TBS_dl(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].TBS;
}

uint32_t flexran_get_TBS_ul(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].ulsch_TBS;
}

uint16_t flexran_get_num_prb_retx_dl_per_ue(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].rbs_used_retx;
}

uint32_t flexran_get_num_prb_retx_ul_per_ue(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].rbs_used_retx_rx;
}

uint16_t flexran_get_num_prb_dl_tx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].rbs_used;
}

uint16_t flexran_get_num_prb_ul_rx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].rbs_used_rx;
}

uint8_t flexran_get_ue_wpmi(mid_t mod_id, mid_t ue_id, uint8_t cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.UE_sched_ctrl[ue_id].periodic_wideband_pmi[cc_id];
}

uint8_t flexran_get_mcs1_dl(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].dlsch_mcs1;
}

uint8_t flexran_get_mcs2_dl(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].dlsch_mcs2;
}

uint8_t flexran_get_mcs1_ul(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].ulsch_mcs1;
}

uint8_t flexran_get_mcs2_ul(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].ulsch_mcs2;
}

uint32_t flexran_get_total_prb_dl_tx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].total_rbs_used;
}

uint32_t flexran_get_total_prb_ul_rx_per_ue(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].total_rbs_used_rx;
}

uint32_t flexran_get_total_num_pdu_dl(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].total_num_pdus;
}

uint32_t flexran_get_total_num_pdu_ul(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].total_num_pdus_rx;
}

uint64_t flexran_get_total_TBS_dl(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].total_pdu_bytes;
}

uint64_t flexran_get_total_TBS_ul(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].total_ulsch_TBS;
}

int flexran_get_harq_round(mid_t mod_id, uint8_t cc_id, mid_t ue_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].harq_round;
}

uint32_t flexran_get_num_mac_sdu_tx(mid_t mod_id, mid_t ue_id, int cc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].num_mac_sdu_tx;
}

unsigned char flexran_get_mac_sdu_lcid_index(mid_t mod_id, mid_t ue_id, int cc_id, int index)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].lcid_sdu[index];
}

uint32_t flexran_get_mac_sdu_size(mid_t mod_id, mid_t ue_id, int cc_id, int lcid)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.eNB_UE_stats[cc_id][ue_id].sdu_length_tx[lcid];
}


/* TODO needs to be revised */
void flexran_update_TA(mid_t mod_id, mid_t ue_id, uint8_t cc_id)
{
/*
  UE_list_t *UE_list=&eNB_mac_inst[mod_id].UE_list;
  UE_sched_ctrl *ue_sched_ctl = &UE_list->UE_sched_ctrl[ue_id];

  if (ue_sched_ctl->ta_timer == 0) {
    
    // WE SHOULD PROTECT the eNB_UE_stats with a mutex here ...                                                                         
    //    LTE_eNB_UE_stats		*eNB_UE_stats = mac_xface->get_eNB_UE_stats(mod_id, CC_id, rnti);
    //ue_sched_ctl->ta_timer		      = 20;	// wait 20 subframes before taking TA measurement from PHY                                         
    ue_sched_ctl->ta_update = flexran_get_TA(mod_id, ue_id, CC_id);

    // clear the update in case PHY does not have a new measurement after timer expiry                                               
    //    eNB_UE_stats->timing_advance_update	      = 0;
  } else {
    ue_sched_ctl->ta_timer--;
    ue_sched_ctl->ta_update		      = 0;	// don't trigger a timing advance command      
  }
#warning "Implement flexran_update_TA() in RAN API"
*/
}

/* TODO needs to be revised, looks suspicious: why do we need UE stats? */
int flexran_get_MAC_CE_bitmap_TA(mid_t mod_id, mid_t ue_id, uint8_t cc_id)
{
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

int flexran_get_active_CC(mid_t mod_id, mid_t ue_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.numactiveCCs[ue_id];
}

uint8_t flexran_get_current_RI(mid_t mod_id, mid_t ue_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->UE_stats[ue_id].rank;
}

int flexran_get_tpc(mid_t mod_id, mid_t ue_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;

  /* before: tested that UL_rssi != NULL and set parameter ([0]), but it is a
   * static array -> target_rx_power is useless in old ifs?! */
  int pCCid = UE_PCCID(mod_id,ue_id);
  int32_t target_rx_power = RC.eNB[mod_id][pCCid]->frame_parms.ul_power_control_config_common.p0_NominalPUSCH;
  int32_t normalized_rx_power = RC.eNB[mod_id][cc_id]->UE_stats[ue_id].UL_rssi[0];

  int tpc;
  if (normalized_rx_power > target_rx_power + 1)
    tpc = 0;	//-1
  else if (normalized_rx_power < target_rx_power - 1)
    tpc = 2;	//+1
  else
    tpc = 1;	//0
  return tpc;
}

int flexran_get_harq(mid_t       mod_id,
                     uint8_t     cc_id,
                     mid_t       ue_id,
                     frame_t     frame,
                     sub_frame_t subframe,
                     uint8_t    *pid,
                     uint8_t    *round,
                     uint8_t     harq_flag)
{
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

int32_t flexran_get_p0_pucch_dbm(mid_t mod_id, mid_t ue_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->UE_stats[ue_id].Po_PUCCH_dBm;
}

int8_t flexran_get_p0_nominal_pucch(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.ul_power_control_config_common.p0_NominalPUCCH;
}

int32_t flexran_get_p0_pucch_status(mid_t mod_id, mid_t ue_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->UE_stats[ue_id].Po_PUCCH_update;
}

int flexran_update_p0_pucch(mid_t mod_id, mid_t ue_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  RC.eNB[mod_id][cc_id]->UE_stats[ue_id].Po_PUCCH_update = 0;
  return 0;
}


/*
 * ************************************
 * Get Messages for eNB Configuration Reply
 * ************************************
 */
uint8_t flexran_get_threequarter_fs(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.threequarter_fs;
}


uint8_t flexran_get_hopping_offset(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.pusch_config_common.pusch_HoppingOffset;
}

Protocol__FlexHoppingMode flexran_get_hopping_mode(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return -1;
  switch (RC.eNB[mod_id][cc_id]->frame_parms.pusch_config_common.hoppingMode) {
  case interSubFrame:
    return PROTOCOL__FLEX_HOPPING_MODE__FLHM_INTER;
  case intraAndInterSubFrame:
    return PROTOCOL__FLEX_HOPPING_MODE__FLHM_INTERINTRA;
  }
  return -1;
}

uint8_t flexran_get_n_SB(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.pusch_config_common.n_SB;
}

Protocol__FlexQam flexran_get_enable64QAM(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  if (RC.eNB[mod_id][cc_id]->frame_parms.pusch_config_common.enable64QAM == TRUE)
    return PROTOCOL__FLEX_QAM__FLEQ_MOD_64QAM;
  else
    return PROTOCOL__FLEX_QAM__FLEQ_MOD_16QAM;
}

Protocol__FlexPhichDuration flexran_get_phich_duration(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return -1;
  switch (RC.eNB[mod_id][cc_id]->frame_parms.phich_config_common.phich_duration) {
  case normal:
    return PROTOCOL__FLEX_PHICH_DURATION__FLPD_NORMAL;
  case extended:
    return PROTOCOL__FLEX_PHICH_DURATION__FLPD_EXTENDED;
  }
  return -1;
}

Protocol__FlexPhichResource flexran_get_phich_resource(mid_t mod_id, uint8_t cc_id)
{
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

uint16_t flexran_get_n1pucch_an(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.pucch_config_common.n1PUCCH_AN;
}

uint8_t flexran_get_nRB_CQI(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.pucch_config_common.nRB_CQI;
}

uint8_t flexran_get_deltaPUCCH_Shift(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.pucch_config_common.deltaPUCCH_Shift;
}

uint8_t flexran_get_prach_ConfigIndex(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
}

uint8_t flexran_get_prach_FreqOffset(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset;
}

uint8_t flexran_get_maxHARQ_Msg3Tx(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.maxHARQ_Msg3Tx;
}

Protocol__FlexUlCyclicPrefixLength flexran_get_ul_cyclic_prefix_length(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return -1;
  switch (RC.eNB[mod_id][cc_id]->frame_parms.Ncp_UL) {
  case EXTENDED:
    return PROTOCOL__FLEX_UL_CYCLIC_PREFIX_LENGTH__FLUCPL_EXTENDED;
  case NORMAL:
    return PROTOCOL__FLEX_UL_CYCLIC_PREFIX_LENGTH__FLUCPL_NORMAL;
  }
  return -1;
}

Protocol__FlexDlCyclicPrefixLength flexran_get_dl_cyclic_prefix_length(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return -1;
  switch (RC.eNB[mod_id][cc_id]->frame_parms.Ncp) {
  case EXTENDED:
    return PROTOCOL__FLEX_DL_CYCLIC_PREFIX_LENGTH__FLDCPL_EXTENDED;
  case NORMAL:
    return PROTOCOL__FLEX_DL_CYCLIC_PREFIX_LENGTH__FLDCPL_NORMAL;
  }
  return -1;
}

uint16_t flexran_get_cell_id(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.Nid_cell;
}

uint8_t flexran_get_srs_BandwidthConfig(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.soundingrs_ul_config_common.srs_BandwidthConfig;
}

uint8_t flexran_get_srs_SubframeConfig(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.soundingrs_ul_config_common.srs_SubframeConfig;
}

uint8_t flexran_get_srs_MaxUpPts(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.soundingrs_ul_config_common.srs_MaxUpPts;
}

uint8_t flexran_get_N_RB_DL(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.N_RB_DL;
}

uint8_t flexran_get_N_RB_UL(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.N_RB_UL;
}

uint8_t flexran_get_N_RBG(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.N_RBG;
}

uint8_t flexran_get_subframe_assignment(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.tdd_config;
}

uint8_t flexran_get_special_subframe_assignment(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.tdd_config_S;
}

long flexran_get_ra_ResponseWindowSize(mid_t mod_id, uint8_t cc_id)
{
  if (!rrc_is_present(mod_id)) return 0;
  return RC.rrc[mod_id]->configuration.radioresourceconfig[cc_id].rach_raResponseWindowSize;
}

long flexran_get_mac_ContentionResolutionTimer(mid_t mod_id, uint8_t cc_id)
{
  if (!rrc_is_present(mod_id)) return 0;
  return RC.rrc[mod_id]->configuration.radioresourceconfig[cc_id].rach_macContentionResolutionTimer;
}

Protocol__FlexDuplexMode flexran_get_duplex_mode(mid_t mod_id, uint8_t cc_id)
{
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

long flexran_get_si_window_length(mid_t mod_id, uint8_t cc_id)
{
  if (!rrc_is_present(mod_id) || !RC.rrc[mod_id]->carrier[cc_id].sib1) return 0;
  return RC.rrc[mod_id]->carrier[cc_id].sib1->si_WindowLength;
}

uint8_t flexran_get_sib1_length(mid_t mod_id, uint8_t cc_id)
{
  if (!rrc_is_present(mod_id)) return 0;
  return RC.rrc[mod_id]->carrier[cc_id].sizeof_SIB1;
}

uint8_t flexran_get_num_pdcch_symb(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->pdcch_vars[0].num_pdcch_symbols;
}



/*
 * ************************************
 * Get Messages for UE Configuration Reply
 * ************************************
 */
int flexran_get_rrc_num_ues(mid_t mod_id)
{
  if (!rrc_is_present(mod_id)) return 0;
  return RC.rrc[mod_id]->Nb_ue;
}

rnti_t flexran_get_rrc_rnti_nth_ue(mid_t mod_id, int index)
{
  if (!rrc_is_present(mod_id)) return 0;
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_ue_tree_s, &RC.rrc[mod_id]->rrc_ue_head) {
    if (index == 0) return ue_context_p->ue_context.rnti;
    --index;
  }
  return 0;
}

int flexran_get_rrc_rnti_list(mid_t mod_id, rnti_t *list, int max_list)
{
  if (!rrc_is_present(mod_id)) return 0;
  int n = 0;
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_ue_tree_s, &RC.rrc[mod_id]->rrc_ue_head) {
    if (n >= max_list) break;
    list[n] = ue_context_p->ue_context.rnti;
    ++n;
  }
  return n;
}

LTE_TimeAlignmentTimer_t  flexran_get_time_alignment_timer(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.mac_MainConfig) return -1;
  return ue_context_p->ue_context.mac_MainConfig->timeAlignmentTimerDedicated;
}

Protocol__FlexMeasGapConfigPattern flexran_get_meas_gap_config(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
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


long flexran_get_meas_gap_config_offset(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
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

uint8_t flexran_get_rrc_status(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return 0;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return RRC_INACTIVE;
  return ue_context_p->ue_context.Status;
}

uint64_t flexran_get_ue_aggregated_max_bitrate_dl(mid_t mod_id, mid_t ue_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.UE_sched_ctrl[ue_id].ue_AggregatedMaximumBitrateDL;
}

uint64_t flexran_get_ue_aggregated_max_bitrate_ul(mid_t mod_id, mid_t ue_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.UE_sched_ctrl[ue_id].ue_AggregatedMaximumBitrateUL;
}

int flexran_get_half_duplex(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.UE_Capability) return -1;
  LTE_SupportedBandListEUTRA_t *bands = &ue_context_p->ue_context.UE_Capability->rf_Parameters.supportedBandListEUTRA;
  for (int i = 0; i < bands->list.count; i++) {
    if (bands->list.array[i]->halfDuplex > 0) return 1;
  }
  return 0;
}

int flexran_get_intra_sf_hopping(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
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

int flexran_get_type2_sb_1(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
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

long flexran_get_ue_category(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.UE_Capability) return -1;
  return ue_context_p->ue_context.UE_Capability->ue_Category;
}

int flexran_get_res_alloc_type1(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
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

long flexran_get_ue_transmission_mode(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated->antennaInfo) return -1;
  return ue_context_p->ue_context.physicalConfigDedicated->antennaInfo->choice.explicitValue.transmissionMode;
}

BOOLEAN_t flexran_get_tti_bundling(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.mac_MainConfig) return -1;
  if (!ue_context_p->ue_context.mac_MainConfig->ul_SCH_Config) return -1;
  return ue_context_p->ue_context.mac_MainConfig->ul_SCH_Config->ttiBundling;
}

long flexran_get_maxHARQ_TX(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.mac_MainConfig) return -1;
  if (!ue_context_p->ue_context.mac_MainConfig->ul_SCH_Config) return -1;
  return *(ue_context_p->ue_context.mac_MainConfig->ul_SCH_Config->maxHARQ_Tx);
}

long flexran_get_beta_offset_ack_index(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated) return -1;
  return ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_ACK_Index;
}

long flexran_get_beta_offset_ri_index(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated) return -1;
  return ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_RI_Index;
}

long flexran_get_beta_offset_cqi_index(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated) return -1;
  return ue_context_p->ue_context.physicalConfigDedicated->pusch_ConfigDedicated->betaOffset_CQI_Index;
}

BOOLEAN_t flexran_get_simultaneous_ack_nack_cqi(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic) return -1;
  return ue_context_p->ue_context.physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic->choice.setup.simultaneousAckNackAndCQI;
}

BOOLEAN_t flexran_get_ack_nack_simultaneous_trans(mid_t mod_id, uint8_t cc_id)
{
  if (!rrc_is_present(mod_id)) return -1;
  if (!RC.rrc[mod_id]->carrier[cc_id].sib2) return -1;
  return RC.rrc[mod_id]->carrier[cc_id].sib2->radioResourceConfigCommon.soundingRS_UL_ConfigCommon.choice.setup.ackNackSRS_SimultaneousTransmission;
}

Protocol__FlexAperiodicCqiReportMode flexran_get_aperiodic_cqi_rep_mode(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
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

long flexran_get_tdd_ack_nack_feedback_mode(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode) return -1;
  return *(ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated->tdd_AckNackFeedbackMode);
}

long flexran_get_ack_nack_repetition_factor(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated) return -1;
  if (!ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated) return -1;
  return ue_context_p->ue_context.physicalConfigDedicated->pucch_ConfigDedicated->ackNackRepetition.choice.setup.repetitionFactor;
}

long flexran_get_extended_bsr_size(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.mac_MainConfig) return -1;
  if (!ue_context_p->ue_context.mac_MainConfig->ext2) return -1;
  if (!ue_context_p->ue_context.mac_MainConfig->ext2->mac_MainConfig_v1020) return -1;
  return *(ue_context_p->ue_context.mac_MainConfig->ext2->mac_MainConfig_v1020->extendedBSR_Sizes_r10);
}

int flexran_get_ue_transmission_antenna(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
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

uint64_t flexran_get_ue_imsi(mid_t mod_id, rnti_t rnti)
{
  uint64_t imsi;
  if (!rrc_is_present(mod_id)) return 0;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
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

long flexran_get_lcg(mid_t mod_id, mid_t ue_id, mid_t lc_id)
{
  if (!mac_is_present(mod_id)) return 0;
  return RC.mac[mod_id]->UE_list.UE_template[UE_PCCID(mod_id, ue_id)][ue_id].lcgidmap[lc_id];
}

/* TODO Navid: needs to be revised */
int flexran_get_direction(mid_t ue_id, mid_t lc_id)
{
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

uint8_t flexran_get_antenna_ports(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.nb_antenna_ports_eNB;
}

uint32_t flexran_agent_get_operating_dl_freq(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.dl_CarrierFreq / 1000000;
}

uint32_t flexran_agent_get_operating_ul_freq(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.ul_CarrierFreq / 1000000;
}

uint8_t flexran_agent_get_operating_eutra_band(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.eutra_band;
}

int8_t flexran_agent_get_operating_pdsch_refpower(mid_t mod_id, uint8_t cc_id)
{
  if (!phy_is_present(mod_id, cc_id)) return 0;
  return RC.eNB[mod_id][cc_id]->frame_parms.pdsch_config_common.referenceSignalPower;
}

long flexran_agent_get_operating_pusch_p0(mid_t mod_id, uint8_t cc_id)
{
  if (!rrc_is_present(mod_id)) return 0;
  return RC.rrc[mod_id]->configuration.radioresourceconfig[cc_id].pusch_p0_Nominal;
}

void flexran_agent_set_operating_dl_freq(mid_t mod_id, uint8_t cc_id, uint32_t dl_freq_mhz)
{
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

void flexran_agent_set_operating_ul_freq(mid_t mod_id, uint8_t cc_id, int32_t ul_freq_mhz_offset)
{
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

void flexran_agent_set_operating_eutra_band(mid_t mod_id, uint8_t cc_id, uint8_t eutra_band)
{
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
void flexran_agent_set_operating_bandwidth(mid_t mod_id, uint8_t cc_id, uint8_t N_RB)
{
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

void flexran_agent_set_operating_frame_type(mid_t mod_id, uint8_t cc_id, lte_frame_type_t frame_type)
{
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

uint16_t flexran_get_pdcp_uid_from_rnti(mid_t mod_id, rnti_t rnti)
{
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
uint32_t flexran_get_pdcp_sfn(mid_t mod_id)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst)
    return 0;
  return pdcp_enb[mod_id].sfn;
}

/*PDCP super frame counter flexRAN*/
void flexran_set_pdcp_tx_stat_window(mid_t mod_id, uint16_t uid, uint16_t obs_window)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB)
    return;
  Pdcp_stats_tx_window_ms[mod_id][uid] = obs_window > 0 ? obs_window : 1000;
}

/*PDCP super frame counter flexRAN*/
void flexran_set_pdcp_rx_stat_window(mid_t mod_id, uint16_t uid, uint16_t obs_window)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB)
    return;
  Pdcp_stats_rx_window_ms[mod_id][uid] = obs_window > 0 ? obs_window : 1000;
}

/*PDCP num tx pdu status flexRAN*/
uint32_t flexran_get_pdcp_tx(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_tx[mod_id][uid][lcid];
}

/*PDCP num tx bytes status flexRAN*/
uint32_t flexran_get_pdcp_tx_bytes(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_tx_bytes[mod_id][uid][lcid];
}

/*PDCP number of transmit packet / second status flexRAN*/
uint32_t flexran_get_pdcp_tx_w(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_tx_w[mod_id][uid][lcid];
}

/*PDCP throughput (bit/s) status flexRAN*/
uint32_t flexran_get_pdcp_tx_bytes_w(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_tx_bytes_w[mod_id][uid][lcid];
}

/*PDCP tx sequence number flexRAN*/
uint32_t flexran_get_pdcp_tx_sn(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_tx_sn[mod_id][uid][lcid];
}

/*PDCP tx aggregated packet arrival  flexRAN*/
uint32_t flexran_get_pdcp_tx_aiat(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_tx_aiat[mod_id][uid][lcid];
}

/*PDCP tx aggregated packet arrival  flexRAN*/
uint32_t flexran_get_pdcp_tx_aiat_w(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_tx_aiat_w[mod_id][uid][lcid];
}

/*PDCP num rx pdu status flexRAN*/
uint32_t flexran_get_pdcp_rx(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_rx[mod_id][uid][lcid];
}

/*PDCP num rx bytes status flexRAN*/
uint32_t flexran_get_pdcp_rx_bytes(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_rx_bytes[mod_id][uid][lcid];
}

/*PDCP number of received packet / second  flexRAN*/
uint32_t flexran_get_pdcp_rx_w(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_rx_w[mod_id][uid][lcid];
}

/*PDCP gootput (bit/s) status flexRAN*/
uint32_t flexran_get_pdcp_rx_bytes_w(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_rx_bytes_w[mod_id][uid][lcid];
}

/*PDCP rx sequence number flexRAN*/
uint32_t flexran_get_pdcp_rx_sn(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_rx_sn[mod_id][uid][lcid];
}

/*PDCP rx aggregated packet arrival  flexRAN*/
uint32_t flexran_get_pdcp_rx_aiat(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_rx_aiat[mod_id][uid][lcid];
}

/*PDCP rx aggregated packet arrival  flexRAN*/
uint32_t flexran_get_pdcp_rx_aiat_w(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_rx_aiat_w[mod_id][uid][lcid];
}

/*PDCP num of received outoforder pdu status flexRAN*/
uint32_t flexran_get_pdcp_rx_oo(mid_t mod_id, uint16_t uid, lcid_t lcid)
{
  if (mod_id < 0 || mod_id >= RC.nb_inst || uid < 0
      || uid >= MAX_MOBILES_PER_ENB || lcid < 0 || lcid >= NB_RB_MAX)
    return 0;
  return Pdcp_stats_rx_outoforder[mod_id][uid][lcid];
}

/******************** RRC *****************************/

LTE_MeasId_t  flexran_get_rrc_pcell_measid(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.measResults) return -1;
  return ue_context_p->ue_context.measResults->measId;
}

float flexran_get_rrc_pcell_rsrp(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.measResults) return -1;
  return RSRP_meas_mapping[ue_context_p->ue_context.measResults->measResultPCell.rsrpResult];
}

float flexran_get_rrc_pcell_rsrq(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.measResults) return -1;
  return RSRQ_meas_mapping[ue_context_p->ue_context.measResults->measResultPCell.rsrqResult];
}

/*Number of neighbouring cells for specific UE*/
int flexran_get_rrc_num_ncell(mid_t mod_id, rnti_t rnti)
{
  if (!rrc_is_present(mod_id)) return 0;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return 0;
  if (!ue_context_p->ue_context.measResults) return 0;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells) return 0;
  if (ue_context_p->ue_context.measResults->measResultNeighCells->present != LTE_MeasResults__measResultNeighCells_PR_measResultListEUTRA) return 0;
  return ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.count;
}

long flexran_get_rrc_neigh_phy_cell_id(mid_t mod_id, rnti_t rnti, long cell_id)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.measResults) return -1;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells) return -1;
  if (ue_context_p->ue_context.measResults->measResultNeighCells->present != LTE_MeasResults__measResultNeighCells_PR_measResultListEUTRA) return -1;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]) return -1;
  return ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->physCellId;
}

float flexran_get_rrc_neigh_rsrp(mid_t mod_id, rnti_t rnti, long cell_id)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.measResults) return -1;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells) return -1;
  if (ue_context_p->ue_context.measResults->measResultNeighCells->present != LTE_MeasResults__measResultNeighCells_PR_measResultListEUTRA) return -1;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]) return -1;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->measResult.rsrpResult) return 0;
  return RSRP_meas_mapping[*(ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->measResult.rsrpResult)];
}

float flexran_get_rrc_neigh_rsrq(mid_t mod_id, rnti_t rnti, long cell_id)
{
  if (!rrc_is_present(mod_id)) return -1;
  struct rrc_eNB_ue_context_s* ue_context_p = rrc_eNB_get_ue_context(RC.rrc[mod_id], rnti);
  if (!ue_context_p) return -1;
  if (!ue_context_p->ue_context.measResults) return -1;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells) return -1;
  if (ue_context_p->ue_context.measResults->measResultNeighCells->present != LTE_MeasResults__measResultNeighCells_PR_measResultListEUTRA) return -1;
  if (!ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->measResult.rsrqResult) return 0;
  return RSRQ_meas_mapping[*(ue_context_p->ue_context.measResults->measResultNeighCells->choice.measResultListEUTRA.list.array[cell_id]->measResult.rsrqResult)];
}

uint8_t flexran_get_rrc_num_plmn_ids(mid_t mod_id)
{
  if (!rrc_is_present(mod_id)) return 0;
  return RC.rrc[mod_id]->configuration.num_plmn;
}

uint16_t flexran_get_rrc_mcc(mid_t mod_id, uint8_t index)
{
  if (!rrc_is_present(mod_id)) return 0;
  return RC.rrc[mod_id]->configuration.mcc[index];
}

uint16_t flexran_get_rrc_mnc(mid_t mod_id, uint8_t index)
{
  if (!rrc_is_present(mod_id)) return 0;
  return RC.rrc[mod_id]->configuration.mnc[index];
}

uint8_t flexran_get_rrc_mnc_digit_length(mid_t mod_id, uint8_t index)
{
  if (!rrc_is_present(mod_id)) return 0;
  return RC.rrc[mod_id]->configuration.mnc_digit_length[index];
}

/**************************** SLICING ****************************/
int flexran_get_ue_dl_slice_id(mid_t mod_id, mid_t ue_id)
{
  if (!mac_is_present(mod_id)) return -1;
  int slice_idx = RC.mac[mod_id]->UE_list.assoc_dl_slice_idx[ue_id];
  if (slice_idx >= 0 && slice_idx < RC.mac[mod_id]->slice_info.n_dl)
    return RC.mac[mod_id]->slice_info.dl[slice_idx].id;
  return 0;
}

void flexran_set_ue_dl_slice_idx(mid_t mod_id, mid_t ue_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return;
  if (flexran_get_mac_ue_crnti(mod_id, ue_id) == NOT_A_RNTI) return;
  if (!flexran_dl_slice_exists(mod_id, slice_idx)) return;
  RC.mac[mod_id]->UE_list.assoc_dl_slice_idx[ue_id] = slice_idx;
}

int flexran_get_ue_ul_slice_id(mid_t mod_id, mid_t ue_id)
{
  if (!mac_is_present(mod_id)) return -1;
  int slice_idx = RC.mac[mod_id]->UE_list.assoc_ul_slice_idx[ue_id];
  if (slice_idx >= 0 && slice_idx < RC.mac[mod_id]->slice_info.n_ul)
    return RC.mac[mod_id]->slice_info.ul[slice_idx].id;
  return 0;
}

void flexran_set_ue_ul_slice_idx(mid_t mod_id, mid_t ue_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return;
  if (flexran_get_mac_ue_crnti(mod_id, ue_id) == NOT_A_RNTI) return;
  if (!flexran_ul_slice_exists(mod_id, slice_idx)) return;
  RC.mac[mod_id]->UE_list.assoc_ul_slice_idx[ue_id] = slice_idx;
}

int flexran_dl_slice_exists(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return slice_idx >= 0 && slice_idx < RC.mac[mod_id]->slice_info.n_dl;
}

int flexran_create_dl_slice(mid_t mod_id, slice_id_t slice_id)
{
  if (!mac_is_present(mod_id)) return -1;
  int newidx = RC.mac[mod_id]->slice_info.n_dl;
  if (newidx >= MAX_NUM_SLICES) return -1;
  ++RC.mac[mod_id]->slice_info.n_dl;
  flexran_set_dl_slice_id(mod_id, newidx, slice_id);
  return newidx;
}

int flexran_find_dl_slice(mid_t mod_id, slice_id_t slice_id)
{
  if (!mac_is_present(mod_id)) return -1;
  slice_info_t *sli = &RC.mac[mod_id]->slice_info;
  int n = sli->n_dl;
  for (int i = 0; i < n; i++) {
    if (sli->dl[i].id == slice_id) return i;
  }
  return -1;
}

int flexran_remove_dl_slice(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  slice_info_t *sli = &RC.mac[mod_id]->slice_info;
  if (sli->n_dl <= 1) return -1;

  if (sli->dl[slice_idx].sched_name) free(sli->dl[slice_idx].sched_name);
  --sli->n_dl;
  /* move last element to the position of the removed one */
  if (slice_idx != sli->n_dl)
    memcpy(&sli->dl[slice_idx], &sli->dl[sli->n_dl], sizeof(sli->dl[sli->n_dl]));
  memset(&sli->dl[sli->n_dl], 0, sizeof(sli->dl[sli->n_dl]));

  /* all UEs that have been in the old slice are put into slice index 0 */
  int *assoc_list = RC.mac[mod_id]->UE_list.assoc_dl_slice_idx;
  for (int i = 0; i < MAX_MOBILES_PER_ENB; ++i) {
    if (assoc_list[i] == slice_idx)
      assoc_list[i] = 0;
  }
  return sli->n_dl;
}

int flexran_get_num_dl_slices(mid_t mod_id)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.n_dl;
}

int flexran_get_intraslice_sharing_active(mid_t mod_id)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.intraslice_share_active;
}
void flexran_set_intraslice_sharing_active(mid_t mod_id, int intraslice_active)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.intraslice_share_active = intraslice_active;
}

int flexran_get_interslice_sharing_active(mid_t mod_id)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.interslice_share_active;
}
void flexran_set_interslice_sharing_active(mid_t mod_id, int interslice_active)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.interslice_share_active = interslice_active;
}

slice_id_t flexran_get_dl_slice_id(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.dl[slice_idx].id;
}
void flexran_set_dl_slice_id(mid_t mod_id, int slice_idx, slice_id_t slice_id)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.dl[slice_idx].id = slice_id;
}

int flexran_get_dl_slice_percentage(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.dl[slice_idx].pct * 100.0f;
}
void flexran_set_dl_slice_percentage(mid_t mod_id, int slice_idx, int percentage)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.dl[slice_idx].pct = percentage / 100.0f;
}

int flexran_get_dl_slice_isolation(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.dl[slice_idx].isol;
}
void flexran_set_dl_slice_isolation(mid_t mod_id, int slice_idx, int is_isolated)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.dl[slice_idx].isol = is_isolated;
}

int flexran_get_dl_slice_priority(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.dl[slice_idx].prio;
}
void flexran_set_dl_slice_priority(mid_t mod_id, int slice_idx, int priority)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.dl[slice_idx].prio = priority;
}

int flexran_get_dl_slice_position_low(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.dl[slice_idx].pos_low;
}
void flexran_set_dl_slice_position_low(mid_t mod_id, int slice_idx, int poslow)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.dl[slice_idx].pos_low = poslow;
}

int flexran_get_dl_slice_position_high(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.dl[slice_idx].pos_high;
}
void flexran_set_dl_slice_position_high(mid_t mod_id, int slice_idx, int poshigh)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.dl[slice_idx].pos_high = poshigh;
}

int flexran_get_dl_slice_maxmcs(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.dl[slice_idx].maxmcs;
}
void flexran_set_dl_slice_maxmcs(mid_t mod_id, int slice_idx, int maxmcs)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.dl[slice_idx].maxmcs = maxmcs;
}

int flexran_get_dl_slice_sorting(mid_t mod_id, int slice_idx, Protocol__FlexDlSorting **sorting_list)
{
  if (!mac_is_present(mod_id)) return -1;
  if (!(*sorting_list)) {
    *sorting_list = calloc(CR_NUM, sizeof(Protocol__FlexDlSorting));
    if (!(*sorting_list)) return -1;
  }
  uint32_t policy = RC.mac[mod_id]->slice_info.dl[slice_idx].sorting;
  for (int i = 0; i < CR_NUM; i++) {
    switch (policy >> 4 * (CR_NUM - 1 - i) & 0xF) {
    case CR_ROUND:
      (*sorting_list)[i] = PROTOCOL__FLEX_DL_SORTING__CR_ROUND;
      break;
    case CR_SRB12:
      (*sorting_list)[i] = PROTOCOL__FLEX_DL_SORTING__CR_SRB12;
      break;
    case CR_HOL:
      (*sorting_list)[i] = PROTOCOL__FLEX_DL_SORTING__CR_HOL;
      break;
    case CR_LC:
      (*sorting_list)[i] = PROTOCOL__FLEX_DL_SORTING__CR_LC;
      break;
    case CR_CQI:
      (*sorting_list)[i] = PROTOCOL__FLEX_DL_SORTING__CR_CQI;
      break;
    case CR_LCP:
      (*sorting_list)[i] = PROTOCOL__FLEX_DL_SORTING__CR_LCP;
      break;
    default:
      /* this should not happen, but a "default" */
      (*sorting_list)[i] = PROTOCOL__FLEX_DL_SORTING__CR_ROUND;
      break;
    }
  }
  return CR_NUM;
}
void flexran_set_dl_slice_sorting(mid_t mod_id, int slice_idx, Protocol__FlexDlSorting *sorting_list, int n)
{
  if (!mac_is_present(mod_id)) return;
  uint32_t policy = 0;
  for (int i = 0; i < n && i < CR_NUM; i++) {
    switch (sorting_list[i]) {
    case PROTOCOL__FLEX_DL_SORTING__CR_ROUND:
      policy = policy << 4 | CR_ROUND;
      break;
    case PROTOCOL__FLEX_DL_SORTING__CR_SRB12:
      policy = policy << 4 | CR_SRB12;
      break;
    case PROTOCOL__FLEX_DL_SORTING__CR_HOL:
      policy = policy << 4 | CR_HOL;
      break;
    case PROTOCOL__FLEX_DL_SORTING__CR_LC:
      policy = policy << 4 | CR_LC;
      break;
    case PROTOCOL__FLEX_DL_SORTING__CR_CQI:
      policy = policy << 4 | CR_CQI;
      break;
    case PROTOCOL__FLEX_DL_SORTING__CR_LCP:
      policy = policy << 4 | CR_LCP;
      break;
    default: /* suppresses warnings */
      policy = policy << 4 | CR_ROUND;
      break;
    }
  }
  /* fill up with 0 == CR_ROUND */
  if (CR_NUM > n) policy = policy << 4 * (CR_NUM - n);
  RC.mac[mod_id]->slice_info.dl[slice_idx].sorting = policy;
}

Protocol__FlexDlAccountingPolicy flexran_get_dl_slice_accounting_policy(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return PROTOCOL__FLEX_DL_ACCOUNTING_POLICY__POL_FAIR;
  switch (RC.mac[mod_id]->slice_info.dl[slice_idx].accounting) {
  case POL_FAIR:
    return PROTOCOL__FLEX_DL_ACCOUNTING_POLICY__POL_FAIR;
  case POL_GREEDY:
    return PROTOCOL__FLEX_DL_ACCOUNTING_POLICY__POL_GREEDY;
  default:
    return PROTOCOL__FLEX_DL_ACCOUNTING_POLICY__POL_FAIR;
  }
}
void flexran_set_dl_slice_accounting_policy(mid_t mod_id, int slice_idx, Protocol__FlexDlAccountingPolicy accounting)
{
  if (!mac_is_present(mod_id)) return;
  switch (accounting) {
  case PROTOCOL__FLEX_DL_ACCOUNTING_POLICY__POL_FAIR:
    RC.mac[mod_id]->slice_info.dl[slice_idx].accounting = POL_FAIR;
    return;
  case PROTOCOL__FLEX_DL_ACCOUNTING_POLICY__POL_GREEDY:
    RC.mac[mod_id]->slice_info.dl[slice_idx].accounting = POL_GREEDY;
    return;
  default:
    RC.mac[mod_id]->slice_info.dl[slice_idx].accounting = POL_FAIR;
    return;
  }
}

char *flexran_get_dl_slice_scheduler(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return NULL;
  return RC.mac[mod_id]->slice_info.dl[slice_idx].sched_name;
}
int flexran_set_dl_slice_scheduler(mid_t mod_id, int slice_idx, char *name)
{
  if (!mac_is_present(mod_id)) return 0;
  if (RC.mac[mod_id]->slice_info.dl[slice_idx].sched_name)
    free(RC.mac[mod_id]->slice_info.dl[slice_idx].sched_name);
  RC.mac[mod_id]->slice_info.dl[slice_idx].sched_name = strdup(name);
  RC.mac[mod_id]->slice_info.dl[slice_idx].sched_cb = dlsym(NULL, name);
  return RC.mac[mod_id]->slice_info.dl[slice_idx].sched_cb != NULL;
}

int flexran_create_ul_slice(mid_t mod_id, slice_id_t slice_id)
{
  if (!mac_is_present(mod_id)) return -1;
  int newidx = RC.mac[mod_id]->slice_info.n_ul;
  if (newidx >= MAX_NUM_SLICES) return -1;
  ++RC.mac[mod_id]->slice_info.n_ul;
  flexran_set_ul_slice_id(mod_id, newidx, slice_id);
  return newidx;
}

int flexran_find_ul_slice(mid_t mod_id, slice_id_t slice_id)
{
  if (!mac_is_present(mod_id)) return -1;
  slice_info_t *sli = &RC.mac[mod_id]->slice_info;
  int n = sli->n_ul;
  for (int i = 0; i < n; i++) {
    if (sli->ul[i].id == slice_id) return i;
  }
  return -1;
}

int flexran_remove_ul_slice(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  slice_info_t *sli = &RC.mac[mod_id]->slice_info;
  if (sli->n_ul <= 1) return -1;

  if (sli->ul[slice_idx].sched_name) free(sli->ul[slice_idx].sched_name);
  --sli->n_ul;
  /* move last element to the position of the removed one */
  if (slice_idx != sli->n_ul)
    memcpy(&sli->ul[slice_idx], &sli->ul[sli->n_ul], sizeof(sli->ul[sli->n_ul]));
  memset(&sli->ul[sli->n_ul], 0, sizeof(sli->ul[sli->n_ul]));

  /* all UEs that have been in the old slice are put into slice index 0 */
  int *assoc_list = RC.mac[mod_id]->UE_list.assoc_ul_slice_idx;
  for (int i = 0; i < MAX_MOBILES_PER_ENB; ++i) {
    if (assoc_list[i] == slice_idx)
      assoc_list[i] = 0;
  }
  return sli->n_ul;
}

int flexran_get_num_ul_slices(mid_t mod_id)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.n_ul;
}

int flexran_ul_slice_exists(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return slice_idx >= 0 && slice_idx < RC.mac[mod_id]->slice_info.n_ul;
}

slice_id_t flexran_get_ul_slice_id(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.ul[slice_idx].id;
}
void flexran_set_ul_slice_id(mid_t mod_id, int slice_idx, slice_id_t slice_id)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.ul[slice_idx].id = slice_id;
}

int flexran_get_ul_slice_percentage(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.ul[slice_idx].pct * 100.0f;
}
void flexran_set_ul_slice_percentage(mid_t mod_id, int slice_idx, int percentage)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.ul[slice_idx].pct = percentage / 100.0f;
}

int flexran_get_ul_slice_first_rb(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.ul[slice_idx].first_rb;
}

void flexran_set_ul_slice_first_rb(mid_t mod_id, int slice_idx, int first_rb)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.ul[slice_idx].first_rb = first_rb;
}

int flexran_get_ul_slice_maxmcs(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return -1;
  return RC.mac[mod_id]->slice_info.ul[slice_idx].maxmcs;
}
void flexran_set_ul_slice_maxmcs(mid_t mod_id, int slice_idx, int maxmcs)
{
  if (!mac_is_present(mod_id)) return;
  RC.mac[mod_id]->slice_info.ul[slice_idx].maxmcs = maxmcs;
}

char *flexran_get_ul_slice_scheduler(mid_t mod_id, int slice_idx)
{
  if (!mac_is_present(mod_id)) return NULL;
  return RC.mac[mod_id]->slice_info.ul[slice_idx].sched_name;
}
int flexran_set_ul_slice_scheduler(mid_t mod_id, int slice_idx, char *name)
{
  if (!mac_is_present(mod_id)) return 0;
  if (RC.mac[mod_id]->slice_info.ul[slice_idx].sched_name)
    free(RC.mac[mod_id]->slice_info.ul[slice_idx].sched_name);
  RC.mac[mod_id]->slice_info.ul[slice_idx].sched_name = strdup(name);
  RC.mac[mod_id]->slice_info.ul[slice_idx].sched_cb = dlsym(NULL, name);
  return RC.mac[mod_id]->slice_info.ul[slice_idx].sched_cb != NULL;
}

/**************************** General BS info  ****************************/
uint64_t flexran_get_bs_id(mid_t mod_id)
{
  if (!rrc_is_present(mod_id)) return 0;
  return RC.rrc[mod_id]->nr_cellid;
}

size_t flexran_get_capabilities(mid_t mod_id, Protocol__FlexBsCapability **caps)
{
  if (!caps) return 0;
  if (!rrc_is_present(mod_id)) return 0;
  size_t n_caps = 0;
  switch (RC.rrc[mod_id]->node_type) {
  case ngran_eNB_CU:
  case ngran_ng_eNB_CU:
  case ngran_gNB_CU:
    n_caps = 3;
    *caps = calloc(n_caps, sizeof(Protocol__FlexBsCapability));
    AssertFatal(*caps, "could not allocate %zu bytes for Protocol__FlexBsCapability array\n",
                n_caps * sizeof(Protocol__FlexBsCapability));
    (*caps)[0] = PROTOCOL__FLEX_BS_CAPABILITY__PDCP;
    (*caps)[1] = PROTOCOL__FLEX_BS_CAPABILITY__SDAP;
    (*caps)[2] = PROTOCOL__FLEX_BS_CAPABILITY__RRC;
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
    n_caps = 8;
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
    break;
  }
  return n_caps;
}

uint16_t flexran_get_capabilities_mask(mid_t mod_id)
{
  if (!rrc_is_present(mod_id)) return 0;
  uint16_t mask = 0;
  switch (RC.rrc[mod_id]->node_type) {
  case ngran_eNB_CU:
  case ngran_ng_eNB_CU:
  case ngran_gNB_CU:
    mask = (1 << PROTOCOL__FLEX_BS_CAPABILITY__PDCP)
         | (1 << PROTOCOL__FLEX_BS_CAPABILITY__SDAP)
         | (1 << PROTOCOL__FLEX_BS_CAPABILITY__RRC);
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
         | (1 << PROTOCOL__FLEX_BS_CAPABILITY__RRC);
    break;
  }
  return mask;
}
