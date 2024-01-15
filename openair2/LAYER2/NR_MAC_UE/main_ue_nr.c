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

/* \file main_ue_nr.c
 * \brief top init of Layer 2
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

//#include "defs.h"
#include "mac_proto.h"
#include "radio/COMMON/common_lib.h"
//#undef MALLOC
#include "assertions.h"
#include "executables/softmodem-common.h"
#include "nr_rlc/nr_rlc_oai_api.h"
#include "RRC/NR_UE/rrc_proto.h"
#include <pthread.h>
#include "openair2/LAYER2/RLC/rlc.h"
static NR_UE_MAC_INST_t *nr_ue_mac_inst; 

void send_srb0_rrc(int rnti, const uint8_t *sdu, sdu_size_t sdu_len, void *data)
{
  AssertFatal(sdu_len > 0 && sdu_len < CCCH_SDU_SIZE, "invalid CCCH SDU size %d\n", sdu_len);

  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_CCCH_DATA_IND);
  memset(NR_RRC_MAC_CCCH_DATA_IND(message_p).sdu, 0, sdu_len);
  memcpy(NR_RRC_MAC_CCCH_DATA_IND(message_p).sdu, sdu, sdu_len);
  NR_RRC_MAC_CCCH_DATA_IND(message_p).sdu_size = sdu_len;
  NR_RRC_MAC_CCCH_DATA_IND(message_p).rnti = rnti;
  itti_send_msg_to_task(TASK_RRC_NRUE, 0, message_p);
}

void send_msg3_rrc_request(module_id_t mod_id, int rnti)
{
  nr_rlc_activate_srb0(rnti, NULL, send_srb0_rrc);
  nr_mac_rrc_msg3_ind(mod_id, rnti);
}

void nr_ue_init_mac(module_id_t module_idP)
{
  LOG_I(NR_MAC, "[UE%d] Applying default macMainConfig\n", module_idP);
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_idP);
  mac->first_sync_frame = -1;
  mac->get_sib1 = false;
  mac->get_otherSI = false;
  mac->phy_config_request_sent = false;
  memset(&mac->phy_config, 0, sizeof(mac->phy_config));
  mac->state = UE_NOT_SYNC;
  mac->si_window_start = -1;
  mac->servCellIndex = 0;
  mac->harq_ACK_SpatialBundlingPUCCH = false;
  mac->harq_ACK_SpatialBundlingPUSCH = false;

  memset(&mac->ssb_measurements, 0, sizeof(mac->ssb_measurements));
  memset(&mac->ul_time_alignment, 0, sizeof(mac->ul_time_alignment));
  for (int i = 0; i < MAX_NUM_BWP_UE; i++) {
    memset(&mac->ssb_list[i], 0, sizeof(mac->ssb_list[i]));
    memset(&mac->prach_assoc_pattern[i], 0, sizeof(mac->prach_assoc_pattern[i]));
  }
}

void nr_ue_mac_default_configs(NR_UE_MAC_INST_t *mac)
{
  // default values as defined in 38.331 sec 9.2.2
  mac->scheduling_info.retxBSR_Timer = NR_BSR_Config__retxBSR_Timer_sf10240;
  mac->scheduling_info.periodicBSR_Timer = NR_BSR_Config__periodicBSR_Timer_infinity;
  mac->scheduling_info.SR_COUNTER = 0;
  mac->scheduling_info.SR_pending = 0;
  mac->scheduling_info.sr_ProhibitTimer = 0;
  mac->scheduling_info.sr_ProhibitTimer_Running = 0;
  mac->scheduling_info.sr_id = -1; // invalid init value

  // set init value 0xFFFF, make sure periodic timer and retx time counters are NOT active, after bsr transmission set the value
  // configured by the NW.
  mac->scheduling_info.periodicBSR_SF = MAC_UE_BSR_TIMER_NOT_RUNNING;
  mac->scheduling_info.retxBSR_SF = MAC_UE_BSR_TIMER_NOT_RUNNING;
  mac->BSR_reporting_active = BSR_TRIGGER_NONE;

  for (int i = 0; i < NR_MAX_NUM_LCID; i++) {
    LOG_D(NR_MAC, "Applying default logical channel config for LCGID %d\n", i);
    mac->scheduling_info.lc_sched_info[i].Bj = -1;
    mac->scheduling_info.lc_sched_info[i].bucket_size = -1;
    mac->scheduling_info.lc_sched_info[i].LCGID = 0; // defaults to 0 irrespective of SRB or DRB
    mac->scheduling_info.lc_sched_info[i].LCID_status = LCID_EMPTY;
    mac->scheduling_info.lc_sched_info[i].LCID_buffer_remain = 0;
    for (int k = 0; k < NR_MAX_HARQ_PROCESSES; k++)
      mac->UL_ndi[k] = -1; // initialize to invalid value
  }
}

NR_UE_MAC_INST_t *nr_l2_init_ue()
{
  //init mac here
  nr_ue_mac_inst = (NR_UE_MAC_INST_t *)calloc(NB_NR_UE_MAC_INST, sizeof(NR_UE_MAC_INST_t));

  for (int j = 0; j < NB_NR_UE_MAC_INST; j++) {
    nr_ue_init_mac(j);
    NR_UE_MAC_INST_t *mac = get_mac_inst(j);
    nr_ue_mac_default_configs(mac);
    if (get_softmodem_params()->sa)
      ue_init_config_request(mac, get_softmodem_params()->numerology);
  }
  int rc = rlc_module_init(0);
  AssertFatal(rc == 0, "%s: Could not initialize RLC layer\n", __FUNCTION__);

  return (nr_ue_mac_inst);
}

NR_UE_MAC_INST_t *get_mac_inst(module_id_t module_id) {
  return &nr_ue_mac_inst[(int)module_id];
}

void reset_mac_inst(NR_UE_MAC_INST_t *nr_mac)
{
  // MAC reset according to 38.321 Section 5.12

  nr_ue_mac_default_configs(nr_mac);
  // initialize Bj for each logical channel to zero
  // Done in default config but to -1 (is that correct?)

  // stop all running timers
  // TODO

  // consider all timeAlignmentTimers as expired and perform the corresponding actions in clause 5.2
  // TODO

  // set the NDIs for all uplink HARQ processes to the value 0
  for (int k = 0; k < NR_MAX_HARQ_PROCESSES; k++)
    nr_mac->UL_ndi[k] = -1; // initialize to invalid value

  // stop any ongoing RACH procedure
  if (nr_mac->ra.ra_state < RA_SUCCEEDED)
    nr_mac->ra.ra_state = RA_UE_IDLE;

  // discard explicitly signalled contention-free Random Access Resources
  // TODO not sure what needs to be done here

  // flush Msg3 buffer
  // TODO we don't have a Msg3 buffer

  // cancel any triggered Scheduling Request procedure
  // Done in default config

  // cancel any triggered Buffer Status Reporting procedure
  // Done in default config

  // cancel any triggered Power Headroom Reporting procedure
  // TODO PHR not implemented yet

  // flush the soft buffers for all DL HARQ processes
  for (int k = 0; k < NR_MAX_HARQ_PROCESSES; k++)
    memset(&nr_mac->dl_harq_info[k], 0, sizeof(NR_UE_HARQ_STATUS_t));

  // for each DL HARQ process, consider the next received transmission for a TB as the very first transmission
  // TODO there is nothing in the MAC indicating first transmission

  // release, if any, Temporary C-RNTI
  nr_mac->ra.t_crnti = 0;

  // reset BFI_COUNTER
  // TODO beam failure procedure not implemented
}

void release_mac_configuration(NR_UE_MAC_INST_t *mac)
{
  asn1cFreeStruc(asn_DEF_NR_MIB, mac->mib);
  asn1cFreeStruc(asn_DEF_NR_SI_SchedulingInfo, mac->si_SchedulingInfo);
  asn1cFreeStruc(asn_DEF_NR_TDD_UL_DL_ConfigCommon, mac->tdd_UL_DL_ConfigurationCommon);
  NR_UE_ServingCell_Info_t *sc = &mac->sc_info;
  asn1cFreeStruc(asn_DEF_NR_CrossCarrierSchedulingConfig, sc->crossCarrierSchedulingConfig);
  asn1cFreeStruc(asn_DEF_NR_SRS_CarrierSwitching, sc->carrierSwitching);
  asn1cFreeStruc(asn_DEF_NR_UplinkConfig, sc->supplementaryUplink);
  asn1cFreeStruc(asn_DEF_NR_PDSCH_CodeBlockGroupTransmission, sc->pdsch_CGB_Transmission);
  asn1cFreeStruc(asn_DEF_NR_PUSCH_CodeBlockGroupTransmission, sc->pusch_CGB_Transmission);
  asn1cFreeStruc(asn_DEF_NR_CSI_MeasConfig, sc->csi_MeasConfig);
  asn1cFreeStruc(asn_DEF_NR_CSI_AperiodicTriggerStateList, sc->aperiodicTriggerStateList);
  free(sc->xOverhead_PDSCH);
  free(sc->nrofHARQ_ProcessesForPDSCH);
  free(sc->rateMatching_PUSCH);
  free(sc->xOverhead_PUSCH);
  free(sc->maxMIMO_Layers_PDSCH);
  free(sc->maxMIMO_Layers_PUSCH);
  memset(&mac->sc_info, 0, sizeof(mac->sc_info));

  for (int i = 0; i < mac->dl_BWPs.count; i++)
    release_dl_BWP(mac, i);
  for (int i = 0; i < mac->ul_BWPs.count; i++)
    release_ul_BWP(mac, i);
  mac->current_DL_BWP = NULL;
  mac->current_UL_BWP = NULL;

  for (int i = 0; i < NR_MAX_NUM_LCID; i++) {
    nr_release_mac_config_logicalChannelBearer(mac, i + 1);
    memset(&mac->lc_ordered_info[i], 0, sizeof(nr_lcordered_info_t));
  }

  memset(&mac->ssb_measurements, 0, sizeof(mac->ssb_measurements));
  memset(&mac->csirs_measurements, 0, sizeof(mac->csirs_measurements));
  memset(&mac->ul_time_alignment, 0, sizeof(mac->ul_time_alignment));
}

void reset_ra(RA_config_t *ra)
{
  if(ra->rach_ConfigDedicated)
    asn1cFreeStruc(asn_DEF_NR_RACH_ConfigDedicated, ra->rach_ConfigDedicated);
  memset(ra, 0, sizeof(RA_config_t));
}
