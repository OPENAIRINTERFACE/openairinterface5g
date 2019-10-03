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

/*! \file eNB_scheduler.c
 * \brief eNB scheduler top level function operates on per subframe basis
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 0.5
 * @ingroup _mac

 */

#include "assertions.h"
#include "targets/RT/USER/lte-softmodem.h"
#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_extern.h"

#include "LAYER2/MAC/mac_proto.h"
#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

//Agent-related headers
#include "flexran_agent_extern.h"
#include "flexran_agent_mac.h"

/* for fair round robin SCHED */
#include "eNB_scheduler_fairRR.h"

#if defined(ENABLE_ITTI)
  #include "intertask_interface.h"
#endif

#include "assertions.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

extern RAN_CONTEXT_t RC;


uint16_t pdcch_order_table[6] = { 31, 31, 511, 2047, 2047, 8191 };

//-----------------------------------------------------------------------------
/*
 * Schedule periodic SRS
 */
void schedule_SRS(module_id_t module_idP,
                  frame_t frameP,
                  sub_frame_t subframeP)
//-----------------------------------------------------------------------------
{
  int CC_id = 0;
  int UE_id = -1;
  uint8_t TSFC = 0;
  uint8_t srs_SubframeConfig = 0;
  uint16_t srsPeriodicity = 0;
  uint16_t srsOffset = 0;
  uint16_t deltaTSFC = 0;  // bitmap
  // table for TSFC (Period) and deltaSFC (offset)
  const uint16_t deltaTSFCTabType1[15][2] = { {1, 1}, {1, 2}, {2, 2}, {1, 5}, {2, 5}, {4, 5}, {8, 5}, {3, 5}, {12, 5}, {1, 10}, {2, 10}, {4, 10}, {8, 10}, {351, 10}, {383, 10} };  // Table 5.5.3.3-2 3GPP 36.211 FDD
  const uint16_t deltaTSFCTabType2[14][2] = { {2, 5}, {6, 5}, {10, 5}, {18, 5}, {14, 5}, {22, 5}, {26, 5}, {30, 5}, {70, 10}, {74, 10}, {194, 10}, {326, 10}, {586, 10}, {210, 10} }; // Table 5.5.3.3-2 3GPP 36.211 TDD
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_list_t *UE_list = &(eNB->UE_list);
  nfapi_ul_config_request_body_t *ul_req = NULL;
  UE_sched_ctrl_t *UE_scheduling_control = NULL;
  COMMON_channels_t *cc = eNB->common_channels;
  LTE_SoundingRS_UL_ConfigCommon_t *soundingRS_UL_ConfigCommon = NULL;
  struct LTE_SoundingRS_UL_ConfigDedicated *soundingRS_UL_ConfigDedicated = NULL;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    soundingRS_UL_ConfigCommon = &(cc[CC_id].radioResourceConfigCommon->soundingRS_UL_ConfigCommon);

    /* Check if SRS is enabled in this frame/subframe */
    if (soundingRS_UL_ConfigCommon) {
      srs_SubframeConfig = soundingRS_UL_ConfigCommon->choice.setup.srs_SubframeConfig;

      if (cc[CC_id].tdd_Config == NULL) {  // FDD
        deltaTSFC = deltaTSFCTabType1[srs_SubframeConfig][0];
        TSFC = deltaTSFCTabType1[srs_SubframeConfig][1];
      } else {  // TDD
        deltaTSFC = deltaTSFCTabType2[srs_SubframeConfig][0];
        TSFC = deltaTSFCTabType2[srs_SubframeConfig][1];
      }

      /* Sounding reference signal subframes are the subframes satisfying ns/2 mod TSFC (- deltaTSFC) */
      uint16_t tmp = (subframeP % TSFC);

      if ((1 << tmp) & deltaTSFC) {
        /* This is an SRS subframe, loop over UEs */
        for (UE_id = 0; UE_id < MAX_MOBILES_PER_ENB; UE_id++) {
          if (!UE_list->active[UE_id]) {
            continue;
          }

          /* Drop the allocation if the UE hasn't send RRCConnectionSetupComplete yet */
          if (mac_eNB_get_rrc_status(module_idP, UE_RNTI(module_idP, UE_id)) < RRC_CONNECTED) {
            continue;
          }

	      if(UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated == NULL) {
	        LOG_E(MAC,"physicalConfigDedicated is null for UE %d\n",UE_id);
	        printf("physicalConfigDedicated is null for UE %d\n",UE_id);
	        return;
	      }

          /* CDRX condition on Active Time and SRS type-0 report (36.321 5.7) */
          UE_scheduling_control = &(UE_list->UE_sched_ctrl[UE_id]);

          /* Test if Active Time not running since 6+ subframes */
          if (UE_scheduling_control->cdrx_configured == TRUE && UE_scheduling_control->in_active_time == FALSE) {
            /*
             * TODO: 6+ subframes condition not checked here
             */
            continue;
          }

          ul_req = &(eNB->UL_req[CC_id].ul_config_request_body);

          if ((soundingRS_UL_ConfigDedicated = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->soundingRS_UL_ConfigDedicated) != NULL) {
            if (soundingRS_UL_ConfigDedicated->present == LTE_SoundingRS_UL_ConfigDedicated_PR_setup) {
              get_srs_pos(&cc[CC_id],
                          soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex,
                          &srsPeriodicity,
                          &srsOffset);

              if (((10 * frameP + subframeP) % srsPeriodicity) == srsOffset) {
                // Program SRS
                ul_req->srs_present = 1;
                nfapi_ul_config_request_pdu_t *ul_config_pdu = &(ul_req->ul_config_pdu_list[ul_req->number_of_pdus]);
                memset((void *) ul_config_pdu, 0, sizeof(nfapi_ul_config_request_pdu_t));
                ul_config_pdu->pdu_type =  NFAPI_UL_CONFIG_SRS_PDU_TYPE;
                ul_config_pdu->pdu_size =  2 + (uint8_t) (2 + sizeof(nfapi_ul_config_srs_pdu));
                ul_config_pdu->srs_pdu.srs_pdu_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL8_TAG;
                ul_config_pdu->srs_pdu.srs_pdu_rel8.size = (uint8_t)sizeof(nfapi_ul_config_srs_pdu);
                ul_config_pdu->srs_pdu.srs_pdu_rel8.rnti = UE_list->UE_template[CC_id][UE_id].rnti;
                ul_config_pdu->srs_pdu.srs_pdu_rel8.srs_bandwidth = soundingRS_UL_ConfigDedicated->choice.setup.srs_Bandwidth;
                ul_config_pdu->srs_pdu.srs_pdu_rel8.frequency_domain_position = soundingRS_UL_ConfigDedicated->choice.setup.freqDomainPosition;
                ul_config_pdu->srs_pdu.srs_pdu_rel8.srs_hopping_bandwidth = soundingRS_UL_ConfigDedicated->choice.setup.srs_HoppingBandwidth;;
                ul_config_pdu->srs_pdu.srs_pdu_rel8.transmission_comb = soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb;
                ul_config_pdu->srs_pdu.srs_pdu_rel8.i_srs = soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex;
                ul_config_pdu->srs_pdu.srs_pdu_rel8.sounding_reference_cyclic_shift = soundingRS_UL_ConfigDedicated->choice.setup.cyclicShift;

                eNB->UL_req[CC_id].sfn_sf = (frameP << 4) + subframeP;
                eNB->UL_req[CC_id].header.message_id = NFAPI_UL_CONFIG_REQUEST;
                ul_req->number_of_pdus++;
              } // if (((10*frameP+subframeP) % srsPeriodicity) == srsOffset)
            } // if (soundingRS_UL_ConfigDedicated->present == SoundingRS_UL_ConfigDedicated_PR_setup)
          } // if ((soundingRS_UL_ConfigDedicated = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->soundingRS_UL_ConfigDedicated)!=NULL)
        } // end for loop on UE_id
      } // if((1<<tmp) & deltaTSFC)
    } // SRS config not NULL
  } // end for loop on CC_id
}

//-----------------------------------------------------------------------------
/*
* Schedule the CSI (CQI/PMI/RI/PTI/CRI) periodic reception
*/
void schedule_CSI(module_id_t module_idP,
                  frame_t frameP,
                  sub_frame_t subframeP)
//-----------------------------------------------------------------------------
{
  int                            CC_id = 0;
  int                            UE_id = 0;
  int                            H = 0;
  uint16_t                       Npd = 0;
  uint16_t                       N_OFFSET_CQI = 0;
  struct LTE_CQI_ReportPeriodic  *cqi_ReportPeriodic = NULL;
  eNB_MAC_INST                   *eNB = RC.mac[module_idP];
  UE_list_t                      *UE_list = &eNB->UE_list;
  COMMON_channels_t              *cc = NULL;
  nfapi_ul_config_request_body_t *ul_req = NULL;
  UE_sched_ctrl_t *UE_scheduling_control = NULL;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    cc = &eNB->common_channels[CC_id];

    for (UE_id = 0; UE_id < MAX_MOBILES_PER_ENB; UE_id++) {
      if (UE_list->active[UE_id] == FALSE) {
        continue;
      }

      /* Drop the allocation if the UE hasn't sent RRCConnectionSetupComplete yet */
      if (mac_eNB_get_rrc_status(module_idP, UE_RNTI(module_idP, UE_id)) < RRC_CONNECTED) {
        continue;
      }

      AssertFatal(UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated != NULL,
                  "physicalConfigDedicated is null for UE %d\n",
                  UE_id);
      /*
      * CDRX condition on Active Time and CSI report on PUCCH (36.321 5.7).
      * Here we consider classic periodic reports on PUCCH without PUSCH simultaneous transmission condition.
      * TODO: add the handling or test on simultaneous PUCCH/PUSCH transmission
      */
      UE_scheduling_control = &(UE_list->UE_sched_ctrl[UE_id]);

      if (UE_scheduling_control->cdrx_configured == TRUE) {
        /* Test if CQI masking activated */
        if (UE_scheduling_control->cqi_mask_boolean == TRUE) {
          // CQI masking => test if onDurationTime not running since 6+ subframe
          if (UE_scheduling_control->on_duration_timer == 0) {
            /*
             * TODO: 6+ subframes condition not checked here
             */
            continue;
          }
        } else { // No CQI masking => test if Active Time not running since 6+ subframe
          if (UE_scheduling_control->in_active_time == FALSE) {
            /*
             * TODO: 6+ subframes condition not checked here
             */
            continue;
          }
        }
      }

      ul_req = &(eNB->UL_req[CC_id].ul_config_request_body);

      if (UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->cqi_ReportConfig != NULL) {
        cqi_ReportPeriodic = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic;

        if (cqi_ReportPeriodic != NULL) {
          /* Rel8 Periodic CSI (CQI/PMI/RI) reporting */
          if (cqi_ReportPeriodic->present != LTE_CQI_ReportPeriodic_PR_release) {
            get_csi_params(cc, cqi_ReportPeriodic, &Npd, &N_OFFSET_CQI, &H);

            if ((((frameP * 10) + subframeP) % Npd) == N_OFFSET_CQI) {  // CQI periodic opportunity
              UE_scheduling_control->feedback_cnt[CC_id] = (((frameP * 10) + subframeP) / Npd) % H;

              // Program CQI
              nfapi_ul_config_request_pdu_t *ul_config_pdu = &ul_req->ul_config_pdu_list[ul_req->number_of_pdus];
              memset((void *) ul_config_pdu, 0, sizeof(nfapi_ul_config_request_pdu_t));

              ul_config_pdu->pdu_type                                                          = NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE;
              ul_config_pdu->pdu_size                                                          = 2 + (uint8_t) (2 + sizeof(nfapi_ul_config_uci_cqi_pdu));
              ul_config_pdu->uci_cqi_pdu.ue_information.ue_information_rel8.tl.tag             = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG;
              ul_config_pdu->uci_cqi_pdu.ue_information.ue_information_rel8.rnti               = UE_list->UE_template[CC_id][UE_id].rnti;
              ul_config_pdu->uci_cqi_pdu.cqi_information.cqi_information_rel8.tl.tag           = NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL8_TAG;
              ul_config_pdu->uci_cqi_pdu.cqi_information.cqi_information_rel8.pucch_index      = cqi_ReportPeriodic->choice.setup.cqi_PUCCH_ResourceIndex;
              ul_config_pdu->uci_cqi_pdu.cqi_information.cqi_information_rel8.dl_cqi_pmi_size  = get_rel8_dl_cqi_pmi_size(&UE_list->UE_sched_ctrl[UE_id], CC_id, cc, get_tmode(module_idP, CC_id, UE_id), cqi_ReportPeriodic);
              ul_req->number_of_pdus++;
              ul_req->tl.tag                                                                   = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;

#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
              // PUT rel10-13 UCI options here
#endif
            } else if (cqi_ReportPeriodic->choice.setup.ri_ConfigIndex != NULL) {
              if ((((frameP * 10) + subframeP) % ((H * Npd) << (*cqi_ReportPeriodic->choice.setup.ri_ConfigIndex / 161))) ==
                  N_OFFSET_CQI + (*cqi_ReportPeriodic->choice.setup.ri_ConfigIndex % 161)) {  // RI opportunity

                // Program RI
                nfapi_ul_config_request_pdu_t *ul_config_pdu = &ul_req->ul_config_pdu_list[ul_req->number_of_pdus];
                memset((void *) ul_config_pdu, 0, sizeof(nfapi_ul_config_request_pdu_t));

                ul_config_pdu->pdu_type                                                          = NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE;
                ul_config_pdu->pdu_size                                                          = 2 + (uint8_t) (2 + sizeof(nfapi_ul_config_uci_cqi_pdu));
                ul_config_pdu->uci_cqi_pdu.ue_information.ue_information_rel8.tl.tag             = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG;
                ul_config_pdu->uci_cqi_pdu.ue_information.ue_information_rel8.rnti               = UE_list->UE_template[CC_id][UE_id].rnti;
                ul_config_pdu->uci_cqi_pdu.cqi_information.cqi_information_rel8.tl.tag           = NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL8_TAG;
                ul_config_pdu->uci_cqi_pdu.cqi_information.cqi_information_rel8.pucch_index      = cqi_ReportPeriodic->choice.setup.cqi_PUCCH_ResourceIndex;
                ul_config_pdu->uci_cqi_pdu.cqi_information.cqi_information_rel8.dl_cqi_pmi_size  = (cc->p_eNB == 2) ? 1 : 2;
                RC.mac[module_idP]->UL_req[CC_id].sfn_sf                                         = (frameP << 4) + subframeP;
                ul_req->number_of_pdus++;
                ul_req->tl.tag                                                                   = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;
              }
            }
          } // if CSI Periodic is not release state
        } // if (cqi_ReportPeriodic != NULL)
      } // if cqi_ReportConfig != NULL
    } // for (UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id]) {
  } // for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
}

//-----------------------------------------------------------------------------
/*
* Schedule a possible Scheduling Request reception
*/
void
schedule_SR (module_id_t module_idP,
             frame_t frameP,
             sub_frame_t subframeP)
//-----------------------------------------------------------------------------
{
  int skip_ue = 0;
  int is_harq = 0;
  int pdu_list_index = 0;
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_list_t *UE_list = &eNB->UE_list;
  nfapi_ul_config_request_t      *ul_req = NULL;
  nfapi_ul_config_request_body_t *ul_req_body = NULL;
  LTE_SchedulingRequestConfig_t  *SRconfig = NULL;
  nfapi_ul_config_sr_information sr;

  for (int CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    eNB->UL_req[CC_id].sfn_sf = (frameP << 4) + subframeP;

    for (int UE_id = 0; UE_id < MAX_MOBILES_PER_ENB; UE_id++) {
      if (!UE_list->active[UE_id]) {
        continue;
      }

      if (UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated == NULL) continue;

      if ((SRconfig = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->schedulingRequestConfig) != NULL) {
        if (SRconfig->present == LTE_SchedulingRequestConfig_PR_setup) {
          if (SRconfig->choice.setup.sr_ConfigIndex <= 4) {          // 5 ms SR period
            if ((subframeP % 5) != SRconfig->choice.setup.sr_ConfigIndex) continue;
          } else if (SRconfig->choice.setup.sr_ConfigIndex <= 14) {  // 10 ms SR period
            if (subframeP != (SRconfig->choice.setup.sr_ConfigIndex - 5)) continue;
          } else if (SRconfig->choice.setup.sr_ConfigIndex <= 34) {  // 20 ms SR period
            if ((10 * (frameP & 1) + subframeP) != (SRconfig->choice.setup.sr_ConfigIndex - 15)) continue;
          } else if (SRconfig->choice.setup.sr_ConfigIndex <= 74) {  // 40 ms SR period
            if ((10 * (frameP & 3) + subframeP) != (SRconfig->choice.setup.sr_ConfigIndex - 35)) continue;
          } else if (SRconfig->choice.setup.sr_ConfigIndex <= 154) {  // 80 ms SR period
            if ((10 * (frameP & 7) + subframeP) != (SRconfig->choice.setup.sr_ConfigIndex - 75)) continue;
          }
        }  // SRconfig->present == SchedulingRequestConfig_PR_setup)
      }  // SRconfig = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->schedulingRequestConfig)!=NULL)

      /* If we get here there is some PUCCH1 reception to schedule for SR */
      ul_req = &(eNB->UL_req[CC_id]);
      ul_req_body = &(ul_req->ul_config_request_body);
      skip_ue = 0;
      is_harq = 0;
      pdu_list_index = 0;

      /* Check that there is no existing UL grant for ULSCH which overrides the SR */
      for (int i = 0; i < ul_req_body->number_of_pdus; i++) {
        if (((ul_req_body->ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE) ||
             (ul_req_body->ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE) ||
             (ul_req_body->ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE) ||
             (ul_req_body->ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE)) &&
            (ul_req_body->ul_config_pdu_list[i].ulsch_pdu.ulsch_pdu_rel8.rnti == UE_list->UE_template[CC_id][UE_id].rnti)) {
          skip_ue = 1;
          pdu_list_index = i;
          break;
        }
        /* If there is already an HARQ pdu, convert to SR_HARQ */
        else if ((ul_req_body->ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE) &&
                 (ul_req_body->ul_config_pdu_list[i].uci_harq_pdu.ue_information.ue_information_rel8.rnti == UE_list->UE_template[CC_id][UE_id].rnti)) {
          is_harq = 1;
          pdu_list_index = i;
          break;
        }
      }

      /* Drop the allocation because ULSCH will handle it with BSR */
      if (skip_ue == 1) continue;

      LOG_D(MAC, "Frame %d, Subframe %d : Scheduling SR for UE %d/%x is_harq:%d \n",
            frameP,
            subframeP,
            UE_id,
            UE_list->UE_template[CC_id][UE_id].rnti,
            is_harq);

      /* Check Rel10 or Rel8 SR */
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
      if ((UE_list-> UE_template[CC_id][UE_id].physicalConfigDedicated->ext2) &&
          (UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->ext2->schedulingRequestConfig_v1020) &&
          (UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->ext2->schedulingRequestConfig_v1020)) {
        sr.sr_information_rel10.tl.tag                    = NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL10_TAG;
        sr.sr_information_rel10.number_of_pucch_resources = 1;
        sr.sr_information_rel10.pucch_index_p1            = *UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->ext2->schedulingRequestConfig_v1020->sr_PUCCH_ResourceIndexP1_r10;

        LOG_D(MAC, "REL10 PUCCH INDEX P1:%d \n", sr.sr_information_rel10.pucch_index_p1);

      } else
#endif
      {
        sr.sr_information_rel8.tl.tag      = NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL8_TAG;
        sr.sr_information_rel8.pucch_index = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex;

        LOG_D(MAC, "REL8 PUCCH INDEX:%d\n", sr.sr_information_rel8.pucch_index);
      }

      /* If there is already an HARQ pdu, convert to SR_HARQ */
      if (is_harq) {
        nfapi_ul_config_harq_information harq                                            = ul_req_body->ul_config_pdu_list[pdu_list_index].uci_harq_pdu.harq_information;
        ul_req_body->ul_config_pdu_list[pdu_list_index].pdu_type                         = NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE;
        ul_req_body->ul_config_pdu_list[pdu_list_index].uci_sr_harq_pdu.sr_information   = sr;
        ul_req_body->ul_config_pdu_list[pdu_list_index].uci_sr_harq_pdu.harq_information = harq;
      } else {
        ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].pdu_type                                              = NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE;
        ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].uci_sr_pdu.ue_information.ue_information_rel8.tl.tag  = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG;
        ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].uci_sr_pdu.ue_information.ue_information_rel8.rnti    = UE_list->UE_template[CC_id][UE_id].rnti;
        ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].uci_sr_pdu.ue_information.ue_information_rel11.tl.tag = 0;
        ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].uci_sr_pdu.ue_information.ue_information_rel13.tl.tag = 0;
        ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].uci_sr_pdu.sr_information                             = sr;
        ul_req_body->number_of_pdus++;
      }  // if (is_harq)

      ul_req_body->tl.tag = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;

    }  // for (int UE_id = 0; UE_id < MAX_MOBILES_PER_ENB; UE_id++)
  }  // for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++)
}

void
check_ul_failure(module_id_t module_idP, int CC_id, int UE_id,
                 frame_t frameP, sub_frame_t subframeP) {
  UE_list_t                 *UE_list = &RC.mac[module_idP]->UE_list;
  nfapi_dl_config_request_t  *DL_req = &RC.mac[module_idP]->DL_req[0];
  uint16_t                      rnti = UE_RNTI(module_idP, UE_id);
  COMMON_channels_t              *cc = RC.mac[module_idP]->common_channels;

  // check uplink failure
  if ((UE_list->UE_sched_ctrl[UE_id].ul_failure_timer > 0) &&
      (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 0)) {
    if (UE_list->UE_sched_ctrl[UE_id].ul_failure_timer == 1)
      LOG_I(MAC, "UE %d rnti %x: UL Failure timer %d \n", UE_id, rnti,
            UE_list->UE_sched_ctrl[UE_id].ul_failure_timer);

    if (UE_list->UE_sched_ctrl[UE_id].ra_pdcch_order_sent == 0) {
      UE_list->UE_sched_ctrl[UE_id].ra_pdcch_order_sent = 1;
      // add a format 1A dci for this UE to request an RA procedure (only one UE per subframe)
      nfapi_dl_config_request_pdu_t *dl_config_pdu                    = &DL_req[CC_id].dl_config_request_body.dl_config_pdu_list[DL_req[CC_id].dl_config_request_body.number_pdu];
      memset((void *) dl_config_pdu, 0,sizeof(nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type                                         = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
      dl_config_pdu->pdu_size                                         = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag                = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format            = NFAPI_DL_DCI_FORMAT_1A;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level     = get_aggregation(get_bw_index(module_idP, CC_id),
          UE_list->UE_sched_ctrl[UE_id].
          dl_cqi[CC_id], format1A);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                  = rnti;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type             = 1;  // CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power    = 6000; // equal to RS power
      AssertFatal((cc[CC_id].mib->message.dl_Bandwidth >= 0) && (cc[CC_id].mib->message.dl_Bandwidth < 6),
                  "illegal dl_Bandwidth %d\n",
                  (int) cc[CC_id].mib->message.dl_Bandwidth);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = pdcch_order_table[cc[CC_id].mib->message.dl_Bandwidth];
      DL_req[CC_id].dl_config_request_body.number_dci++;
      DL_req[CC_id].dl_config_request_body.number_pdu++;
      DL_req[CC_id].dl_config_request_body.tl.tag                      = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
      DL_req[CC_id].sfn_sf = frameP<<4 | subframeP;
      LOG_D(MAC,
            "UE %d rnti %x: sending PDCCH order for RAPROC (failure timer %d), resource_block_coding %d \n",
            UE_id, rnti,
            UE_list->UE_sched_ctrl[UE_id].ul_failure_timer,
            dl_config_pdu->dci_dl_pdu.
            dci_dl_pdu_rel8.resource_block_coding);
    } else {    // ra_pdcch_sent==1
      LOG_D(MAC,
            "UE %d rnti %x: sent PDCCH order for RAPROC waiting (failure timer %d) \n",
            UE_id, rnti,
            UE_list->UE_sched_ctrl[UE_id].ul_failure_timer);

      if ((UE_list->UE_sched_ctrl[UE_id].ul_failure_timer % 80) == 0) UE_list->UE_sched_ctrl[UE_id].ra_pdcch_order_sent = 0;  // resend every 8 frames
    }

    UE_list->UE_sched_ctrl[UE_id].ul_failure_timer++;

    // check threshold
    if (UE_list->UE_sched_ctrl[UE_id].ul_failure_timer > 4000) {
      // note: probably ul_failure_timer should be less than UE radio link failure time(see T310/N310/N311)
      if (NODE_IS_DU(RC.rrc[module_idP]->node_type)) {
        MessageDef *m = itti_alloc_new_message(TASK_MAC_ENB, F1AP_UE_CONTEXT_RELEASE_REQ);
        F1AP_UE_CONTEXT_RELEASE_REQ(m).rnti = rnti;
        F1AP_UE_CONTEXT_RELEASE_REQ(m).cause = F1AP_CAUSE_RADIO_NETWORK;
        F1AP_UE_CONTEXT_RELEASE_REQ(m).cause_value = 1; // 1 = F1AP_CauseRadioNetwork_rl_failure
        F1AP_UE_CONTEXT_RELEASE_REQ(m).rrc_container = NULL;
        F1AP_UE_CONTEXT_RELEASE_REQ(m).rrc_container_length = 0;
        itti_send_msg_to_task(TASK_DU_F1, module_idP, m);
      } else {
        // inform RRC of failure and clear timer
        LOG_I(MAC, "UE %d rnti %x: UL Failure after repeated PDCCH orders: Triggering RRC \n",
              UE_id,
              rnti);
        mac_eNB_rrc_ul_failure(module_idP,
                               CC_id,
                               frameP,
                               subframeP,
                               rnti);
      }

      UE_list->UE_sched_ctrl[UE_id].ul_failure_timer = 0;
      UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync   = 1;
    }
  }       // ul_failure_timer>0
  
  UE_list->UE_sched_ctrl[UE_id].uplane_inactivity_timer++;
  if((U_PLANE_INACTIVITY_VALUE != 0) && (UE_list->UE_sched_ctrl[UE_id].uplane_inactivity_timer > (U_PLANE_INACTIVITY_VALUE * 10))){
     LOG_D(MAC,"UE %d rnti %x: U-Plane Failure after repeated PDCCH orders: Triggering RRC \n",UE_id,rnti); 
     mac_eNB_rrc_uplane_failure(module_idP,CC_id,frameP,subframeP,rnti);
     UE_list->UE_sched_ctrl[UE_id].uplane_inactivity_timer  = 0;
  }// time > 60s
}

void
clear_nfapi_information(eNB_MAC_INST *eNB, int CC_idP,
                        frame_t frameP, sub_frame_t subframeP) {
  nfapi_dl_config_request_t      *DL_req = &eNB->DL_req[0];
  nfapi_ul_config_request_t      *UL_req = &eNB->UL_req[0];
  nfapi_hi_dci0_request_t   *HI_DCI0_req = &eNB->HI_DCI0_req[CC_idP][subframeP];
  nfapi_tx_request_t             *TX_req = &eNB->TX_req[0];
  eNB->pdu_index[CC_idP] = 0;

  if (NFAPI_MODE == NFAPI_MODE_PNF || NFAPI_MODE == NFAPI_MONOLITHIC) { // monolithic or PNF
    DL_req[CC_idP].dl_config_request_body.number_pdcch_ofdm_symbols           = 1;
    DL_req[CC_idP].dl_config_request_body.number_dci                          = 0;
    DL_req[CC_idP].dl_config_request_body.number_pdu                          = 0;
    DL_req[CC_idP].dl_config_request_body.number_pdsch_rnti                   = 0;
    DL_req[CC_idP].dl_config_request_body.transmission_power_pcfich           = 6000;
    DL_req[CC_idP].sfn_sf                                                     = subframeP + (frameP<<4);
    HI_DCI0_req->hi_dci0_request_body.sfnsf                                   = subframeP + (frameP<<4);
    HI_DCI0_req->hi_dci0_request_body.number_of_dci                           = 0;
    UL_req[CC_idP].ul_config_request_body.number_of_pdus                      = 0;
    UL_req[CC_idP].ul_config_request_body.rach_prach_frequency_resources      = 0; // ignored, handled by PHY for now
    UL_req[CC_idP].ul_config_request_body.srs_present                         = 0; // ignored, handled by PHY for now
    TX_req[CC_idP].tx_request_body.number_of_pdus                 = 0;
  }
}

void
copy_ulreq(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP) {
  int CC_id;
  eNB_MAC_INST *mac = RC.mac[module_idP];

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    nfapi_ul_config_request_t *ul_req_tmp             = &mac->UL_req_tmp[CC_id][subframeP];
    nfapi_ul_config_request_t *ul_req                 = &mac->UL_req[CC_id];
    nfapi_ul_config_request_pdu_t *ul_req_pdu         = ul_req->ul_config_request_body.ul_config_pdu_list;
    *ul_req = *ul_req_tmp;
    // Restore the pointer
    ul_req->ul_config_request_body.ul_config_pdu_list = ul_req_pdu;
    ul_req->sfn_sf                                    = (frameP<<4) + subframeP;
    ul_req_tmp->ul_config_request_body.number_of_pdus = 0;

    if (ul_req->ul_config_request_body.number_of_pdus>0) {
      LOG_D(PHY, "%s() active NOW (frameP:%d subframeP:%d) pdus:%d\n", __FUNCTION__, frameP, subframeP, ul_req->ul_config_request_body.number_of_pdus);
    }

    memcpy((void *)ul_req->ul_config_request_body.ul_config_pdu_list,
           (void *)ul_req_tmp->ul_config_request_body.ul_config_pdu_list,
           ul_req->ul_config_request_body.number_of_pdus*sizeof(nfapi_ul_config_request_pdu_t));
  }
}

void
eNB_dlsch_ulsch_scheduler(module_id_t module_idP,
                          frame_t frameP,
                          sub_frame_t subframeP)
{
  int               mbsfn_status[MAX_NUM_CCs];
  protocol_ctxt_t   ctxt;
  rnti_t            rnti  = 0;
  int               CC_id = 0;
  int               UE_id = -1;
  eNB_MAC_INST      *eNB                    = RC.mac[module_idP];
  UE_list_t         *UE_list                = &(eNB->UE_list);
  COMMON_channels_t *cc                     = eNB->common_channels;
  UE_sched_ctrl_t     *UE_scheduling_control  = NULL;

  start_meas(&(eNB->eNB_scheduler));

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER, VCD_FUNCTION_IN);

  eNB->frame    = frameP;
  eNB->subframe = subframeP;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    mbsfn_status[CC_id] = 0;

    /* Clear vrb_maps */
    memset(cc[CC_id].vrb_map, 0, 100);
    memset(cc[CC_id].vrb_map_UL, 0, 100);

#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))
    cc[CC_id].mcch_active = 0;
#endif
    clear_nfapi_information(RC.mac[module_idP], CC_id, frameP, subframeP);
  }

  /* Refresh UE list based on UEs dropped by PHY in previous subframe */
  for (UE_id = 0; UE_id < MAX_MOBILES_PER_ENB; UE_id++) {
    if (UE_list->active[UE_id]) {
      rnti = UE_RNTI(module_idP, UE_id);
      CC_id = UE_PCCID(module_idP, UE_id);

      UE_scheduling_control = &(UE_list->UE_sched_ctrl[UE_id]);

      if (((frameP & 127) == 0) && (subframeP == 0)) {
        LOG_I(MAC,"UE  rnti %x : %s, PHR %d dB DL CQI %d PUSCH SNR %d PUCCH SNR %d\n",
              rnti,
              UE_scheduling_control->ul_out_of_sync == 0 ? "in synch" : "out of sync",
              UE_list->UE_template[CC_id][UE_id].phr_info,
              UE_scheduling_control->dl_cqi[CC_id],
              (5 * UE_scheduling_control->pusch_snr[CC_id] - 640) / 10,
              (5 * UE_scheduling_control->pucch1_snr[CC_id] - 640) / 10);
      }

      RC.eNB[module_idP][CC_id]->pusch_stats_bsr[UE_id][(frameP * 10) + subframeP] = -63;

      if (UE_id == UE_list->head) {
        VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BSR, RC.eNB[module_idP][CC_id]->pusch_stats_bsr[UE_id][(frameP * 10) + subframeP]);
      }

      /* Set and increment CDRX related timers */
      if (UE_scheduling_control->cdrx_configured == TRUE) {
        boolean_t harq_active_time_condition = FALSE;
        UE_TEMPLATE *UE_template = NULL;

        unsigned long active_time_condition = 0; // variable used only for tracing purpose

        /* (UL and DL) HARQ RTT timers and DRX retransmission timers */
        for (int harq_process_id = 0; harq_process_id < 8; harq_process_id++) {
          /* DL asynchronous HARQ process */
          if (UE_scheduling_control->drx_retransmission_timer[harq_process_id] > 0) {
            UE_scheduling_control->drx_retransmission_timer[harq_process_id]++;

            if (UE_scheduling_control->drx_retransmission_timer[harq_process_id] > UE_scheduling_control->drx_retransmission_timer_thres[harq_process_id]) {
              UE_scheduling_control->drx_retransmission_timer[harq_process_id] = 0;
            }
          }

          if (UE_scheduling_control->harq_rtt_timer[CC_id][harq_process_id] > 0) {
            UE_scheduling_control->harq_rtt_timer[CC_id][harq_process_id]++;

            if (UE_scheduling_control->harq_rtt_timer[CC_id][harq_process_id] > 8) {
              /* Note: here drx_retransmission_timer is restarted instead of started in the specification */
              UE_scheduling_control->drx_retransmission_timer[harq_process_id] = 1; // started when HARQ RTT timer expires
              UE_scheduling_control->harq_rtt_timer[CC_id][harq_process_id] = 0;
            }
          }

          /* UL asynchronous HARQ process: only UL HARQ RTT timer is implemented (hence not implemented) */
          if (UE_scheduling_control->ul_harq_rtt_timer[CC_id][harq_process_id] > 0) {
            UE_scheduling_control->ul_harq_rtt_timer[CC_id][harq_process_id]++;

            if (UE_scheduling_control->ul_harq_rtt_timer[CC_id][harq_process_id] > 4) {
              /* 
               * TODO: implement the handling of UL asynchronous HARQ
               * drx_ULRetransmissionTimer should be (re)started here
               */
              UE_scheduling_control->ul_harq_rtt_timer[CC_id][harq_process_id] = 0;
            }
          }

          /* UL synchronous HARQ process */
          if (UE_scheduling_control->ul_synchronous_harq_timer[CC_id][harq_process_id] > 0) {
            UE_scheduling_control->ul_synchronous_harq_timer[CC_id][harq_process_id]++;

            if (UE_scheduling_control->ul_synchronous_harq_timer[CC_id][harq_process_id] > 5) {
              harq_active_time_condition = TRUE;
              UE_scheduling_control->ul_synchronous_harq_timer[CC_id][harq_process_id] = 0;
              active_time_condition = 5; // for tracing purpose
            }
          }
        }

        /* On duration timer */
        if (UE_scheduling_control->on_duration_timer > 0) {
          UE_scheduling_control->on_duration_timer++;

          if (UE_scheduling_control->on_duration_timer > UE_scheduling_control->on_duration_timer_thres) {
            UE_scheduling_control->on_duration_timer = 0;
          }
        }

        /* DRX inactivity timer */
        if (UE_scheduling_control->drx_inactivity_timer > 0) {
          UE_scheduling_control->drx_inactivity_timer++;

          if (UE_scheduling_control->drx_inactivity_timer > (UE_scheduling_control->drx_inactivity_timer_thres + 1)) {
            /* Note: the +1 on the threshold is due to information in table C-1 of 36.321 */
            UE_scheduling_control->drx_inactivity_timer = 0;

            /* When timer expires switch into short or long DRX cycle */
            if (UE_scheduling_control->drx_shortCycle_timer_thres > 0) {
              UE_scheduling_control->in_short_drx_cycle = TRUE;
              UE_scheduling_control->drx_shortCycle_timer = 0;
              UE_scheduling_control->in_long_drx_cycle = FALSE;
            } else {
              UE_scheduling_control->in_long_drx_cycle = TRUE;
            }
          }
        }

        /* Short DRX Cycle */
        if (UE_scheduling_control->in_short_drx_cycle == TRUE) {
          UE_scheduling_control->drx_shortCycle_timer++;

          /* When the Short DRX cycles are over, switch to long DRX cycle */
          if (UE_scheduling_control->drx_shortCycle_timer > UE_scheduling_control->drx_shortCycle_timer_thres) {
            UE_scheduling_control->drx_shortCycle_timer = 0;
            UE_scheduling_control->in_short_drx_cycle = FALSE;
            UE_scheduling_control->in_long_drx_cycle = TRUE;
            UE_scheduling_control->drx_longCycle_timer = 0;
          }
        } else {
          UE_scheduling_control->drx_shortCycle_timer = 0;
        }

        /* Long DRX Cycle */
        if (UE_scheduling_control->in_long_drx_cycle == TRUE) {
          UE_scheduling_control->drx_longCycle_timer++;

          if (UE_scheduling_control->drx_longCycle_timer > UE_scheduling_control->drx_longCycle_timer_thres) {
            UE_scheduling_control->drx_longCycle_timer = 1;
          }
        } else {
          UE_scheduling_control->drx_longCycle_timer = 0;
        }

        /* Check for error cases */
        if ((UE_scheduling_control->in_short_drx_cycle == TRUE) && (UE_scheduling_control->in_long_drx_cycle == TRUE)) {
          LOG_E(MAC, "Error in C-DRX: UE id %d is in both short and long DRX cycle. Should not happen. Back it to long cycle only\n", UE_id);
          UE_scheduling_control->in_short_drx_cycle = FALSE;
        }

        /* Condition to start On Duration Timer */
        if (UE_scheduling_control->in_short_drx_cycle == TRUE && UE_scheduling_control->on_duration_timer == 0) {
          if (((frameP * 10) + subframeP) % (UE_scheduling_control->short_drx_cycle_duration) ==
              (UE_scheduling_control->drx_start_offset) % (UE_scheduling_control->short_drx_cycle_duration)) {

            UE_scheduling_control->on_duration_timer = 1;
          }
        } else if (UE_scheduling_control->in_long_drx_cycle == TRUE && UE_scheduling_control->on_duration_timer == 0) {
          if (((frameP * 10) + subframeP) % (UE_scheduling_control->drx_longCycle_timer_thres) ==
              (UE_scheduling_control->drx_start_offset)) {

            UE_scheduling_control->on_duration_timer = 1;
          }
        }

        /* Update Active Time status of UE
         * Based on 36.321 5.7 the differents conditions for the UE to be in Acttive Should be check ONLY
         * here for the current subframe. The variable 'UE_scheduling_control->in_active_time' should be updated
         * ONLY here. The variable can then be used for testing the actual state of the UE for scheduling purpose.
         */
        UE_template = &(UE_list->UE_template[CC_id][UE_id]);
        /* (a)synchronous HARQ processes handling for Active Time */
        for (int harq_process_id = 0; harq_process_id < 8; harq_process_id++) {
          if (UE_scheduling_control->drx_retransmission_timer[harq_process_id] > 0) {
            harq_active_time_condition = TRUE;
            active_time_condition = 2; // for tracing purpose
            break;
          }
        }

        /* Active time conditions */
        if (UE_scheduling_control->on_duration_timer > 0 ||
            UE_scheduling_control->drx_inactivity_timer > 1 ||
            harq_active_time_condition ||
            UE_template->ul_SR > 0) {

          UE_scheduling_control->in_active_time = TRUE;

        } else {
          UE_scheduling_control->in_active_time = FALSE;
        }

        /* BEGIN VCD */
        if (UE_id == 0) {
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_ON_DURATION_TIMER, (unsigned long) UE_scheduling_control->on_duration_timer);
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DRX_INACTIVITY, (unsigned long) UE_scheduling_control->drx_inactivity_timer);
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DRX_SHORT_CYCLE, (unsigned long) UE_scheduling_control->drx_shortCycle_timer);
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DRX_LONG_CYCLE, (unsigned long) UE_scheduling_control->drx_longCycle_timer);
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DRX_RETRANSMISSION_HARQ0, (unsigned long) UE_scheduling_control->drx_retransmission_timer[0]);
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DRX_ACTIVE_TIME, (unsigned long) UE_scheduling_control->in_active_time);

          /* For tracing purpose */
          if (UE_template->ul_SR > 0) {
            active_time_condition = 1;
          } else if ((UE_scheduling_control->on_duration_timer > 0) && (active_time_condition == 0)) {
            active_time_condition = 3;
          } else if ((UE_scheduling_control->drx_inactivity_timer > 1) && (active_time_condition == 0)) {
            active_time_condition = 4;
          }

          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DRX_ACTIVE_TIME_CONDITION, (unsigned long) active_time_condition);
        }
        /* END VCD */

        /* DCI0 ongoing timer */
        if (UE_scheduling_control->dci0_ongoing_timer > 0) {
            if (UE_scheduling_control->dci0_ongoing_timer > 7) {
              UE_scheduling_control->dci0_ongoing_timer = 0;
            } else {
              UE_scheduling_control->dci0_ongoing_timer++;
            }
        }

      } else { // else: CDRX not configured
        /* Note: (UL) HARQ RTT timers processing is done here and can be used by other features than CDRX */
        /* HARQ RTT timers */
        for (int harq_process_id = 0; harq_process_id < 8; harq_process_id++) {
          if (UE_scheduling_control->harq_rtt_timer[CC_id][harq_process_id] > 0) {
            UE_scheduling_control->harq_rtt_timer[CC_id][harq_process_id]++;

            if (UE_scheduling_control->harq_rtt_timer[CC_id][harq_process_id] > 8) {
              UE_scheduling_control->harq_rtt_timer[CC_id][harq_process_id] = 0;
            }
          }

          if (UE_scheduling_control->ul_harq_rtt_timer[CC_id][harq_process_id] > 0) {
            UE_scheduling_control->ul_harq_rtt_timer[CC_id][harq_process_id]++;

            if (UE_scheduling_control->ul_harq_rtt_timer[CC_id][harq_process_id] > 4) {
              UE_scheduling_control->ul_harq_rtt_timer[CC_id][harq_process_id] = 0;
            }
          }
        } // end loop harq process
      } // end else CDRX not configured

      /* Increment these timers, they are cleared when we receive an sdu */
      UE_scheduling_control->ul_inactivity_timer++;
      UE_scheduling_control->cqi_req_timer++;
      
      LOG_D(MAC, "UE %d/%x : ul_inactivity %d, cqi_req %d\n",
            UE_id,
            rnti,
            UE_scheduling_control->ul_inactivity_timer,
            UE_scheduling_control->cqi_req_timer);

      check_ul_failure(module_idP, CC_id, UE_id, frameP, subframeP);

      if (UE_scheduling_control->ue_reestablishment_reject_timer > 0) {
        UE_scheduling_control->ue_reestablishment_reject_timer++;

        if (UE_scheduling_control->ue_reestablishment_reject_timer >= UE_scheduling_control->ue_reestablishment_reject_timer_thres) {
          UE_scheduling_control->ue_reestablishment_reject_timer = 0;

          /* Clear reestablish_rnti_map */
          if (UE_scheduling_control->ue_reestablishment_reject_timer_thres > 20) {
            for (int ue_id_l = 0; ue_id_l < MAX_MOBILES_PER_ENB; ue_id_l++) {
              if (reestablish_rnti_map[ue_id_l][0] == rnti) {
                /* Clear currentC-RNTI from map */
                reestablish_rnti_map[ue_id_l][0] = 0;
                reestablish_rnti_map[ue_id_l][1] = 0;
                break;
              }
            }

            PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES, rnti, 0, 0,module_idP);
            rrc_rlc_remove_ue(&ctxt);
            pdcp_remove_UE(&ctxt);
          }

          /* Note: This should not be done in the MAC! */
          for (int ii=0; ii<MAX_MOBILES_PER_ENB; ii++) {
            LTE_eNB_ULSCH_t *ulsch = RC.eNB[module_idP][CC_id]->ulsch[ii];

            if((ulsch != NULL) && (ulsch->rnti == rnti)) {
              void clean_eNb_ulsch(LTE_eNB_ULSCH_t *ulsch);

              LOG_I(MAC, "clean_eNb_ulsch UE %x \n", rnti);

              clean_eNb_ulsch(ulsch);
            }
          }

          for (int ii=0; ii<MAX_MOBILES_PER_ENB; ii++) {
            LTE_eNB_DLSCH_t *dlsch = RC.eNB[module_idP][CC_id]->dlsch[ii][0];

            if((dlsch != NULL) && (dlsch->rnti == rnti)) {
              void clean_eNb_dlsch(LTE_eNB_DLSCH_t *dlsch);

              LOG_I(MAC, "clean_eNb_dlsch UE %x \n", rnti);

              clean_eNb_dlsch(dlsch);
            }
          }

          for (int j = 0; j < 10; j++) {
            nfapi_ul_config_request_body_t *ul_req_tmp = NULL;
            ul_req_tmp = &(eNB->UL_req_tmp[CC_id][j].ul_config_request_body);

            if (ul_req_tmp) {
              int pdu_number = ul_req_tmp->number_of_pdus;

              for (int pdu_index = pdu_number-1; pdu_index >= 0; pdu_index--) {
                if (ul_req_tmp->ul_config_pdu_list[pdu_index].ulsch_pdu.ulsch_pdu_rel8.rnti == rnti) {
                  LOG_I(MAC, "remove UE %x from ul_config_pdu_list %d/%d\n",
                        rnti,
                        pdu_index,
                        pdu_number);

                  if (pdu_index < pdu_number -1) {
                    memcpy(&ul_req_tmp->ul_config_pdu_list[pdu_index],
                           &ul_req_tmp->ul_config_pdu_list[pdu_index+1],
                          (pdu_number-1-pdu_index) * sizeof(nfapi_ul_config_request_pdu_t));
                  }

                  ul_req_tmp->number_of_pdus--;
                }
              } // end for pdu_index
            } // end if (ul_req_tmp)
          } // end for j

          rrc_mac_remove_ue(module_idP,rnti);

        } // end if (UE_scheduling_control->ue_reestablishment_reject_timer >= UE_scheduling_control->ue_reestablishment_reject_timer_thres)
      } // end if (UE_scheduling_control->ue_reestablishment_reject_timer > 0)
    } // end if UE active
  } // end for loop on UE_id

#if (!defined(PRE_SCD_THREAD))

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES, NOT_A_RNTI, frameP, subframeP, module_idP);
  pdcp_run(&ctxt);
  pdcp_mbms_run(&ctxt);
  rrc_rx_tx(&ctxt, CC_id);

#endif
#if (LTE_RRC_VERSION >= MAKE_VERSION(10, 0, 0))

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    if (cc[CC_id].MBMS_flag > 0) {
      start_meas(&RC.mac[module_idP]->schedule_mch);
      int(*schedule_mch)(module_id_t module_idP, uint8_t CC_id, frame_t frameP, sub_frame_t subframe) = NULL;
      schedule_mch = schedule_MBMS_NFAPI;
      if(schedule_mch){
      	mbsfn_status[CC_id] = schedule_mch(module_idP, CC_id, frameP, subframeP);
      }
      stop_meas(&RC.mac[module_idP]->schedule_mch);
    }
  }

#endif

  static int debug_flag = 0;
  void (*schedule_ulsch_p)(module_id_t module_idP, frame_t frameP, sub_frame_t subframe) = NULL;
  void (*schedule_ue_spec_p)(module_id_t module_idP, frame_t frameP, sub_frame_t subframe, int *mbsfn_flag) = NULL;

  if (eNB->scheduler_mode == SCHED_MODE_DEFAULT) {
    schedule_ulsch_p = schedule_ulsch;
    schedule_ue_spec_p = schedule_dlsch;
  } else if (eNB->scheduler_mode == SCHED_MODE_FAIR_RR) {
    memset(dlsch_ue_select, 0, sizeof(dlsch_ue_select));
    schedule_ulsch_p = schedule_ulsch_fairRR;
    schedule_ue_spec_p = schedule_ue_spec_fairRR;
  }

  if(debug_flag == 0){
    LOG_E(MAC,"SCHED_MODE = %d\n", eNB->scheduler_mode);
    debug_flag = 1;
  }

  /* This schedules MIB */
  if ((subframeP == 0) && (frameP & 3) == 0) 
    schedule_mib(module_idP, frameP, subframeP);

  if (get_softmodem_params()->phy_test == 0) {
    /* This schedules SI for legacy LTE and eMTC starting in subframeP */
    schedule_SI(module_idP, frameP, subframeP);
    /* This schedules Paging in subframeP */
    schedule_PCH(module_idP,frameP,subframeP);
    /* This schedules Random-Access for legacy LTE and eMTC starting in subframeP */
    schedule_RA(module_idP, frameP, subframeP);
    /* Copy previously scheduled UL resources (ULSCH + HARQ) */
    copy_ulreq(module_idP, frameP, subframeP);
    /* This schedules SRS in subframeP */
    schedule_SRS(module_idP, frameP, subframeP);

    /* This schedules ULSCH in subframeP (dci0) */
    if (schedule_ulsch_p != NULL) {
      schedule_ulsch_p(module_idP, frameP, subframeP);
    } else {
      LOG_E(MAC," %s %d: schedule_ulsch_p is NULL, function not called\n",
            __FILE__,
            __LINE__);
    }

    /* This schedules UCI_SR in subframeP */
    schedule_SR(module_idP, frameP, subframeP);
    /* This schedules UCI_CSI in subframeP */
    schedule_CSI(module_idP, frameP, subframeP);
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
    /* This schedules DLSCH in subframeP for BR UE*/
    schedule_ue_spec_br(module_idP, frameP, subframeP);
#endif
    /* This schedules DLSCH in subframeP */
    if (schedule_ue_spec_p != NULL) {
      schedule_ue_spec_p(module_idP, frameP, subframeP, mbsfn_status);
    } else {
      LOG_E(MAC," %s %d: schedule_ue_spec_p is NULL, function not called\n",
            __FILE__,
            __LINE__);
    }

  } else {
    schedule_ulsch_phy_test(module_idP,frameP,subframeP);
    schedule_ue_spec_phy_test(module_idP,frameP,subframeP,mbsfn_status);
  }

  /* Allocate CCEs for good after scheduling is done */
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    if (cc[CC_id].tdd_Config == NULL || !(is_UL_sf(&cc[CC_id],subframeP))) {
      allocate_CCEs(module_idP, CC_id, frameP, subframeP, 2);
    }
  }

  if (flexran_agent_get_mac_xface(module_idP) && subframeP == 9) {
    flexran_agent_slice_update(module_idP);
  }

  stop_meas(&(eNB->eNB_scheduler));

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER, VCD_FUNCTION_OUT);
}
