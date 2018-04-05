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
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"

#include "LAYER2/MAC/proto.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

//Agent-related headers
#include "flexran_agent_extern.h"
#include "flexran_agent_mac.h"

#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#endif

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

extern RAN_CONTEXT_t RC;

uint16_t pdcch_order_table[6] = { 31, 31, 511, 2047, 2047, 8191 };


void
schedule_SRS(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)
{


  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_list_t *UE_list = &eNB->UE_list;
  nfapi_ul_config_request_body_t *ul_req;
  int CC_id, UE_id;
  COMMON_channels_t *cc = RC.mac[module_idP]->common_channels;
  SoundingRS_UL_ConfigCommon_t *soundingRS_UL_ConfigCommon;
  struct SoundingRS_UL_ConfigDedicated *soundingRS_UL_ConfigDedicated;
  uint8_t TSFC;
  uint16_t deltaTSFC;		// bitmap
  uint8_t srs_SubframeConfig;
  
  // table for TSFC (Period) and deltaSFC (offset)
  const uint16_t deltaTSFCTabType1[15][2] = { {1, 1}, {1, 2}, {2, 2}, {1, 5}, {2, 5}, {4, 5}, {8, 5}, {3, 5}, {12, 5}, {1, 10}, {2, 10}, {4, 10}, {8, 10}, {351, 10}, {383, 10} };	// Table 5.5.3.3-2 3GPP 36.211 FDD
  const uint16_t deltaTSFCTabType2[14][2] = { {2, 5}, {6, 5}, {10, 5}, {18, 5}, {14, 5}, {22, 5}, {26, 5}, {30, 5}, {70, 10}, {74, 10}, {194, 10}, {326, 10}, {586, 10}, {210, 10} };	// Table 5.5.3.3-2 3GPP 36.211 TDD
  
  uint16_t srsPeriodicity, srsOffset;
  
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    soundingRS_UL_ConfigCommon = &cc[CC_id].radioResourceConfigCommon->soundingRS_UL_ConfigCommon;
    // check if SRS is enabled in this frame/subframe
    if (soundingRS_UL_ConfigCommon) {
      srs_SubframeConfig = soundingRS_UL_ConfigCommon->choice.setup.srs_SubframeConfig;
      if (cc[CC_id].tdd_Config == NULL) {	// FDD
	deltaTSFC = deltaTSFCTabType1[srs_SubframeConfig][0];
	TSFC = deltaTSFCTabType1[srs_SubframeConfig][1];
      } else {		// TDD
	deltaTSFC = deltaTSFCTabType2[srs_SubframeConfig][0];
	TSFC = deltaTSFCTabType2[srs_SubframeConfig][1];
      }
      // Sounding reference signal subframes are the subframes satisfying ns/2 mod TSFC (- deltaTSFC
      uint16_t tmp = (subframeP % TSFC);
      
      if ((1 << tmp) & deltaTSFC) {
	// This is an SRS subframe, loop over UEs
	for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
	  if (RC.mac[module_idP]->UE_list.active[UE_id] != TRUE)
	    continue;
	  ul_req = &RC.mac[module_idP]->UL_req[CC_id].ul_config_request_body;
	  // drop the allocation if the UE hasn't send RRCConnectionSetupComplete yet
	  if (mac_eNB_get_rrc_status(module_idP,UE_RNTI(module_idP, UE_id)) < RRC_CONNECTED) continue;
	  
	  AssertFatal(UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated != NULL,
		      "physicalConfigDedicated is null for UE %d\n",
		      UE_id);
	  
	  if ((soundingRS_UL_ConfigDedicated = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->soundingRS_UL_ConfigDedicated) != NULL) {
	    if (soundingRS_UL_ConfigDedicated->present == SoundingRS_UL_ConfigDedicated_PR_setup) {
	      get_srs_pos(&cc[CC_id],
			  soundingRS_UL_ConfigDedicated->choice.
			  setup.srs_ConfigIndex,
			  &srsPeriodicity, &srsOffset);
	      if (((10 * frameP + subframeP) % srsPeriodicity) == srsOffset) {
		// Program SRS
		ul_req->srs_present = 1;
		nfapi_ul_config_request_pdu_t * ul_config_pdu = &ul_req->ul_config_pdu_list[ul_req->number_of_pdus];
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
		ul_config_pdu->srs_pdu.srs_pdu_rel8.sounding_reference_cyclic_shift = soundingRS_UL_ConfigDedicated->choice.setup.cyclicShift;		//              ul_config_pdu->srs_pdu.srs_pdu_rel10.antenna_port                   = ;//
		//              ul_config_pdu->srs_pdu.srs_pdu_rel13.number_of_combs                = ;//
		RC.mac[module_idP]->UL_req[CC_id].sfn_sf = (frameP << 4) + subframeP;
		RC.mac[module_idP]->UL_req[CC_id].header.message_id = NFAPI_UL_CONFIG_REQUEST;
		ul_req->number_of_pdus++;
	      }	// if (((10*frameP+subframeP) % srsPeriodicity) == srsOffset)
	    }	// if (soundingRS_UL_ConfigDedicated->present == SoundingRS_UL_ConfigDedicated_PR_setup)
	  }		// if ((soundingRS_UL_ConfigDedicated = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->soundingRS_UL_ConfigDedicated)!=NULL)
	}		// for (UE_id ...
      }			// if((1<<tmp) & deltaTSFC)
      
    }			// SRS config
  }
}

void
schedule_CSI(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)
{
  eNB_MAC_INST                   *eNB = RC.mac[module_idP];
  UE_list_t                      *UE_list = &eNB->UE_list;
  COMMON_channels_t              *cc;
  nfapi_ul_config_request_body_t *ul_req;
  int                            CC_id, UE_id;
  struct CQI_ReportPeriodic      *cqi_ReportPeriodic;
  uint16_t                       Npd, N_OFFSET_CQI;
  int                            H;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

    cc = &eNB->common_channels[CC_id];
    for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
      if (UE_list->active[UE_id] != TRUE)
	continue;

      ul_req = &RC.mac[module_idP]->UL_req[CC_id].ul_config_request_body;

      // drop the allocation if the UE hasn't send RRCConnectionSetupComplete yet
      if (mac_eNB_get_rrc_status(module_idP, UE_RNTI(module_idP, UE_id)) < RRC_CONNECTED) continue;

      AssertFatal(UE_list->
		  UE_template[CC_id][UE_id].physicalConfigDedicated
		  != NULL,
		  "physicalConfigDedicated is null for UE %d\n",
		  UE_id);

      if (UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->cqi_ReportConfig) {
	if ((cqi_ReportPeriodic = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->cqi_ReportConfig->cqi_ReportPeriodic) != NULL
	    && (cqi_ReportPeriodic->present != CQI_ReportPeriodic_PR_release)) {
	  //Rel8 Periodic CQI/PMI/RI reporting

	  get_csi_params(cc, cqi_ReportPeriodic, &Npd,
			 &N_OFFSET_CQI, &H);

	  if ((((frameP * 10) + subframeP) % Npd) == N_OFFSET_CQI) {	// CQI opportunity
	    UE_list->UE_sched_ctrl[UE_id].feedback_cnt[CC_id] = (((frameP * 10) + subframeP) / Npd) % H;
	    // Program CQI
	    nfapi_ul_config_request_pdu_t *ul_config_pdu = &ul_req->ul_config_pdu_list[ul_req->number_of_pdus];
	    memset((void *) ul_config_pdu, 0,
		   sizeof(nfapi_ul_config_request_pdu_t));
	    ul_config_pdu->pdu_type                                                          = NFAPI_UL_CONFIG_UCI_CQI_PDU_TYPE;
	    ul_config_pdu->pdu_size                                                          = 2 + (uint8_t) (2 + sizeof(nfapi_ul_config_uci_cqi_pdu));
	    ul_config_pdu->uci_cqi_pdu.ue_information.ue_information_rel8.tl.tag             = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG;
	    ul_config_pdu->uci_cqi_pdu.ue_information.ue_information_rel8.rnti               = UE_list->UE_template[CC_id][UE_id].rnti;
	    ul_config_pdu->uci_cqi_pdu.cqi_information.cqi_information_rel8.tl.tag           = NFAPI_UL_CONFIG_REQUEST_CQI_INFORMATION_REL8_TAG;
	    ul_config_pdu->uci_cqi_pdu.cqi_information.cqi_information_rel8.pucch_index      = cqi_ReportPeriodic->choice.setup.cqi_PUCCH_ResourceIndex;
	    ul_config_pdu->uci_cqi_pdu.cqi_information.cqi_information_rel8.dl_cqi_pmi_size  = get_rel8_dl_cqi_pmi_size(&UE_list->UE_sched_ctrl[UE_id], CC_id, cc,
															get_tmode(module_idP, CC_id, UE_id),
															cqi_ReportPeriodic);
	    ul_req->number_of_pdus++;
	    ul_req->tl.tag                                                                   = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;

#if defined(Rel10) || defined(Rel14)
	    // PUT rel10-13 UCI options here
#endif
	  } else
	    if ((cqi_ReportPeriodic->choice.setup.ri_ConfigIndex)
		&& ((((frameP * 10) + subframeP) % ((H * Npd) << (*cqi_ReportPeriodic->choice.setup.ri_ConfigIndex / 161))) == N_OFFSET_CQI + (*cqi_ReportPeriodic->choice.setup.ri_ConfigIndex % 161))) {	// RI opportunity
	      // Program RI
	      nfapi_ul_config_request_pdu_t *ul_config_pdu = &ul_req->ul_config_pdu_list[ul_req->number_of_pdus];
	      memset((void *) ul_config_pdu, 0,
		     sizeof(nfapi_ul_config_request_pdu_t));
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
	}		// if ((cqi_ReportPeriodic = cqi_ReportConfig->cqi_ReportPeriodic)!=NULL) {
      }			// if (UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->cqi_ReportConfig)
    }			// for (UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id]) {
  }				// for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
}

void
schedule_SR(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)
{
  eNB_MAC_INST                   *eNB = RC.mac[module_idP];
  UE_list_t                      *UE_list = &eNB->UE_list;
  nfapi_ul_config_request_t      *ul_req;
  nfapi_ul_config_request_body_t *ul_req_body;
  int                            CC_id;
  int                            UE_id;
  SchedulingRequestConfig_t      *SRconfig;
  int                            skip_ue;
  int                            is_harq;
  nfapi_ul_config_sr_information sr;
  int                            i;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    RC.mac[module_idP]->UL_req[CC_id].sfn_sf = (frameP << 4) + subframeP;

    for (UE_id = 0; UE_id < NUMBER_OF_UE_MAX; UE_id++) {
      if (RC.mac[module_idP]->UE_list.active[UE_id] != TRUE) continue;

      ul_req        = &RC.mac[module_idP]->UL_req[CC_id];
      ul_req_body   = &ul_req->ul_config_request_body;

      // drop the allocation if the UE hasn't send RRCConnectionSetupComplete yet
      if (mac_eNB_get_rrc_status(module_idP, UE_RNTI(module_idP, UE_id)) < RRC_CONNECTED) continue;

      AssertFatal(UE_list->
		  UE_template[CC_id][UE_id].physicalConfigDedicated!= NULL,
		  "physicalConfigDedicated is null for UE %d\n",
		  UE_id);

      if ((SRconfig = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->schedulingRequestConfig) != NULL) {
	if (SRconfig->present == SchedulingRequestConfig_PR_setup) {
	  if (SRconfig->choice.setup.sr_ConfigIndex <= 4) {	// 5 ms SR period
	    if ((subframeP % 5) != SRconfig->choice.setup.sr_ConfigIndex) continue;
	  } else if (SRconfig->choice.setup.sr_ConfigIndex <= 14) {	// 10 ms SR period
	    if (subframeP != (SRconfig->choice.setup.sr_ConfigIndex - 5)) continue;
	  } else if (SRconfig->choice.setup.sr_ConfigIndex <= 34) {	// 20 ms SR period
	    if ((10 * (frameP & 1) + subframeP) != (SRconfig->choice.setup.sr_ConfigIndex - 15)) continue;
	  } else if (SRconfig->choice.setup.sr_ConfigIndex <= 74) {	// 40 ms SR period
	    if ((10 * (frameP & 3) + subframeP) != (SRconfig->choice.setup.sr_ConfigIndex - 35)) continue;
	  } else if (SRconfig->choice.setup.sr_ConfigIndex <= 154) {	// 80 ms SR period
	    if ((10 * (frameP & 7) + subframeP) != (SRconfig->choice.setup.sr_ConfigIndex - 75)) continue;
	  }
	}		// SRconfig->present == SchedulingRequestConfig_PR_setup)
      }			// SRconfig = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->schedulingRequestConfig)!=NULL)

      // if we get here there is some PUCCH1 reception to schedule for SR

      skip_ue = 0;
      is_harq = 0;
      // check that there is no existing UL grant for ULSCH which overrides the SR
      for (i = 0; i < ul_req_body->number_of_pdus; i++) {
	if (((ul_req_body->ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE) || 
	     (ul_req_body->ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE) || 
	     (ul_req_body->ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE) || 
	     (ul_req_body->ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE)) && 
	    (ul_req_body->ul_config_pdu_list[i].ulsch_pdu.ulsch_pdu_rel8.rnti == UE_list->UE_template[CC_id][UE_id].rnti)) {
	  skip_ue = 1;
	  break;
	}
	/* if there is already an HARQ pdu, convert to SR_HARQ */
	else if ((ul_req_body->ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE) && 
		 (ul_req_body->ul_config_pdu_list[i].uci_harq_pdu.ue_information.ue_information_rel8.rnti == UE_list->UE_template[CC_id][UE_id].rnti)) {
	  is_harq = 1;
	  break;
	}
      }

      // drop the allocation because ULSCH with handle it with BSR
      if (skip_ue == 1) continue;

      LOG_D(MAC,"Frame %d, Subframe %d : Scheduling SR for UE %d/%x is_harq:%d\n",frameP,subframeP,UE_id,UE_list->UE_template[CC_id][UE_id].rnti, is_harq);

      // check Rel10 or Rel8 SR
#if defined(Rel10) || defined(Rel14)
      if ((UE_list-> UE_template[CC_id][UE_id].physicalConfigDedicated->ext2)
	  && (UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->ext2->schedulingRequestConfig_v1020)
	  && (UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->ext2->schedulingRequestConfig_v1020)) {
	sr.sr_information_rel10.tl.tag                    = NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL10_TAG;
	sr.sr_information_rel10.number_of_pucch_resources = 1;
	sr.sr_information_rel10.pucch_index_p1            = *UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->ext2->schedulingRequestConfig_v1020->sr_PUCCH_ResourceIndexP1_r10;
	LOG_D(MAC,"REL10 PUCCH INDEX P1:%d\n", sr.sr_information_rel10.pucch_index_p1);
      } else
#endif
	{
	  sr.sr_information_rel8.tl.tag                   = NFAPI_UL_CONFIG_REQUEST_SR_INFORMATION_REL8_TAG;
	  sr.sr_information_rel8.pucch_index              = UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->schedulingRequestConfig->choice.setup.sr_PUCCH_ResourceIndex;
	  LOG_D(MAC,"REL8 PUCCH INDEX:%d\n", sr.sr_information_rel8.pucch_index);
	}

      /* if there is already an HARQ pdu, convert to SR_HARQ */
      if (is_harq) {
	nfapi_ul_config_harq_information h                                                                                 = ul_req_body->ul_config_pdu_list[i].uci_harq_pdu.harq_information;
	ul_req_body->ul_config_pdu_list[i].pdu_type                                                                        = NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE;
	ul_req_body->ul_config_pdu_list[i].uci_sr_harq_pdu.sr_information                                                  = sr;
	ul_req_body->ul_config_pdu_list[i].uci_sr_harq_pdu.harq_information                                                = h;
      } else {
	ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].pdu_type                                              = NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE;
	ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].uci_sr_pdu.ue_information.ue_information_rel8.tl.tag  = NFAPI_UL_CONFIG_REQUEST_UE_INFORMATION_REL8_TAG;
	ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].uci_sr_pdu.ue_information.ue_information_rel8.rnti    = UE_list->UE_template[CC_id][UE_id].rnti;
	ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].uci_sr_pdu.ue_information.ue_information_rel11.tl.tag = 0;
	ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].uci_sr_pdu.ue_information.ue_information_rel13.tl.tag = 0;
	ul_req_body->ul_config_pdu_list[ul_req_body->number_of_pdus].uci_sr_pdu.sr_information                             = sr;
	ul_req_body->number_of_pdus++;
      }			/* if (is_harq) */
      ul_req_body->tl.tag                                                                                                  = NFAPI_UL_CONFIG_REQUEST_BODY_TAG;
    }			// for (UE_id=UE_list->head; UE_id>=0; UE_id=UE_list->next[UE_id])
  }				// for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++)
}

extern uint8_t nfapi_mode;

void
check_ul_failure(module_id_t module_idP, int CC_id, int UE_id,
		 frame_t frameP, sub_frame_t subframeP)
{
  UE_list_t                 *UE_list = &RC.mac[module_idP]->UE_list;
  nfapi_dl_config_request_t  *DL_req = &RC.mac[module_idP]->DL_req[0];
  uint16_t                      rnti = UE_RNTI(module_idP, UE_id);
  COMMON_channels_t              *cc = RC.mac[module_idP]->common_channels;

  // check uplink failure
  if ((UE_list->UE_sched_ctrl[UE_id].ul_failure_timer > 0) &&
      (UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync == 0)) {
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
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type             = 1;	// CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power    = 6000;	// equal to RS power

      AssertFatal((cc[CC_id].mib->message.dl_Bandwidth >= 0) && (cc[CC_id].mib->message.dl_Bandwidth < 6),
		  "illegal dl_Bandwidth %d\n",
		  (int) cc[CC_id].mib->message.dl_Bandwidth);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = pdcch_order_table[cc[CC_id].mib->message.dl_Bandwidth];
      DL_req[CC_id].dl_config_request_body.number_dci++;
      DL_req[CC_id].dl_config_request_body.number_pdu++;
      DL_req[CC_id].dl_config_request_body.tl.tag                      = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
      LOG_I(MAC,
	    "UE %d rnti %x: sending PDCCH order for RAPROC (failure timer %d), resource_block_coding %d \n",
	    UE_id, rnti,
	    UE_list->UE_sched_ctrl[UE_id].ul_failure_timer,
	    dl_config_pdu->dci_dl_pdu.
	    dci_dl_pdu_rel8.resource_block_coding);
    } else {		// ra_pdcch_sent==1
      LOG_I(MAC,
	    "UE %d rnti %x: sent PDCCH order for RAPROC waiting (failure timer %d) \n",
	    UE_id, rnti,
	    UE_list->UE_sched_ctrl[UE_id].ul_failure_timer);
      if ((UE_list->UE_sched_ctrl[UE_id].ul_failure_timer % 40) == 0) UE_list->UE_sched_ctrl[UE_id].ra_pdcch_order_sent = 0;	// resend every 4 frames
    }

    UE_list->UE_sched_ctrl[UE_id].ul_failure_timer++;
    // check threshold
    if (UE_list->UE_sched_ctrl[UE_id].ul_failure_timer > 20000) {
      // inform RRC of failure and clear timer
      LOG_I(MAC,
	    "UE %d rnti %x: UL Failure after repeated PDCCH orders: Triggering RRC \n",
	    UE_id, rnti);
      mac_eNB_rrc_ul_failure(module_idP, CC_id, frameP, subframeP,rnti);
      UE_list->UE_sched_ctrl[UE_id].ul_failure_timer = 0;
      UE_list->UE_sched_ctrl[UE_id].ul_out_of_sync   = 1;

      //Inform the controller about the UE deactivation. Should be moved to RRC agent in the future
      if (rrc_agent_registered[module_idP]) {
        LOG_W(MAC, "notify flexran Agent of UE state change\n");
        agent_rrc_xface[module_idP]->flexran_agent_notify_ue_state_change(module_idP,
            rnti, PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED);
      }
    }
  }				// ul_failure_timer>0

#if 0
  /* U-plane inactivity timer is disabled. Uncomment to re-enable. */
  UE_list->UE_sched_ctrl[UE_id].uplane_inactivity_timer++;
  if(UE_list->UE_sched_ctrl[UE_id].uplane_inactivity_timer > (U_PLANE_INACTIVITY_VALUE*subframe_num(&RC.eNB[module_idP][CC_id]->frame_parms))){
    LOG_D(MAC,"UE %d rnti %x: U-Plane Failure after repeated PDCCH orders: Triggering RRC \n",UE_id,rnti); 
    mac_eNB_rrc_uplane_failure(module_idP,CC_id,frameP,subframeP,rnti);
    UE_list->UE_sched_ctrl[UE_id].uplane_inactivity_timer  = 0;
  }// time > 60s
#endif
}

void
clear_nfapi_information(eNB_MAC_INST * eNB, int CC_idP,
			frame_t frameP, sub_frame_t subframeP)
{
  nfapi_dl_config_request_t      *DL_req = &eNB->DL_req[0];
  nfapi_ul_config_request_t      *UL_req = &eNB->UL_req[0];
  nfapi_hi_dci0_request_t   *HI_DCI0_req = &eNB->HI_DCI0_req[0];
  nfapi_tx_request_t             *TX_req = &eNB->TX_req[0];

  eNB->pdu_index[CC_idP] = 0;

  if (nfapi_mode==0 || nfapi_mode == 1) { // monolithic or PNF

    DL_req[CC_idP].dl_config_request_body.number_pdcch_ofdm_symbols           = 1;
    DL_req[CC_idP].dl_config_request_body.number_dci                          = 0;
    DL_req[CC_idP].dl_config_request_body.number_pdu                          = 0;
    DL_req[CC_idP].dl_config_request_body.number_pdsch_rnti                   = 0;
    DL_req[CC_idP].dl_config_request_body.transmission_power_pcfich           = 6000;

    HI_DCI0_req[CC_idP].hi_dci0_request_body.sfnsf                            = subframeP + (frameP<<4);
    HI_DCI0_req[CC_idP].hi_dci0_request_body.number_of_dci                    = 0;


    UL_req[CC_idP].ul_config_request_body.number_of_pdus                      = 0;
    UL_req[CC_idP].ul_config_request_body.rach_prach_frequency_resources      = 0; // ignored, handled by PHY for now
    UL_req[CC_idP].ul_config_request_body.srs_present                         = 0; // ignored, handled by PHY for now

    TX_req[CC_idP].tx_request_body.number_of_pdus                 = 0;

  }
}

void
copy_ulreq(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)
{
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

    if (ul_req->ul_config_request_body.number_of_pdus>0)
      {
        LOG_D(PHY, "%s() active NOW (frameP:%d subframeP:%d) pdus:%d\n", __FUNCTION__, frameP, subframeP, ul_req->ul_config_request_body.number_of_pdus);
      }

    memcpy((void*)ul_req->ul_config_request_body.ul_config_pdu_list,
	   (void*)ul_req_tmp->ul_config_request_body.ul_config_pdu_list,
	   ul_req->ul_config_request_body.number_of_pdus*sizeof(nfapi_ul_config_request_pdu_t));
  }
}

void
eNB_dlsch_ulsch_scheduler(module_id_t module_idP, frame_t frameP,
			  sub_frame_t subframeP)
{

  int               mbsfn_status[MAX_NUM_CCs];
  protocol_ctxt_t   ctxt;

  int               CC_id, i = -1;
  UE_list_t         *UE_list = &RC.mac[module_idP]->UE_list;
  rnti_t            rnti;

  COMMON_channels_t *cc      = RC.mac[module_idP]->common_channels;

  start_meas(&RC.mac[module_idP]->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
    (VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,
     VCD_FUNCTION_IN);

  RC.mac[module_idP]->frame    = frameP;
  RC.mac[module_idP]->subframe = subframeP;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    mbsfn_status[CC_id] = 0;

    // clear vrb_maps
    memset(cc[CC_id].vrb_map, 0, 100);
    memset(cc[CC_id].vrb_map_UL, 0, 100);


#if defined(Rel10) || defined(Rel14)
    cc[CC_id].mcch_active        = 0;
#endif

    clear_nfapi_information(RC.mac[module_idP], CC_id, frameP, subframeP);
  }

  // refresh UE list based on UEs dropped by PHY in previous subframe
  for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
    if (UE_list->active[i] != TRUE)
      continue;

    rnti = UE_RNTI(module_idP, i);
    CC_id = UE_PCCID(module_idP, i);

    if ((frameP == 0) && (subframeP == 0)) {
      LOG_I(MAC,
            "UE  rnti %x : %s, PHR %d dB DL CQI %d PUSCH SNR %d PUCCH SNR %d\n",
            rnti,
            UE_list->UE_sched_ctrl[i].ul_out_of_sync ==
            0 ? "in synch" : "out of sync",
            UE_list->UE_template[CC_id][i].phr_info,
            UE_list->UE_sched_ctrl[i].dl_cqi[CC_id],
            (UE_list->UE_sched_ctrl[i].pusch_snr[CC_id] - 128) / 2,
            (UE_list->UE_sched_ctrl[i].pucch1_snr[CC_id] - 128) / 2);
    }

    RC.eNB[module_idP][CC_id]->pusch_stats_bsr[i][(frameP * 10) +
						  subframeP] = -63;
    if (i == UE_list->head)
      VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME
	(VCD_SIGNAL_DUMPER_VARIABLES_UE0_BSR,
	 RC.eNB[module_idP][CC_id]->
	 pusch_stats_bsr[i][(frameP * 10) + subframeP]);
    // increment this, it is cleared when we receive an sdu
    RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].ul_inactivity_timer++;
    
    RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].cqi_req_timer++;
    LOG_D(MAC, "UE %d/%x : ul_inactivity %d, cqi_req %d\n", i, rnti,
	  RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].
	  ul_inactivity_timer,
	  RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].cqi_req_timer);
    check_ul_failure(module_idP, CC_id, i, frameP, subframeP);
    
    if (RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer > 0) {
      RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer++;
      if(RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer >=
	 RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer_thres) {
	RC.mac[module_idP]->UE_list.UE_sched_ctrl[i].ue_reestablishment_reject_timer = 0;
	for (int ue_id_l = 0; ue_id_l < NUMBER_OF_UE_MAX; ue_id_l++) {
	  if (reestablish_rnti_map[ue_id_l][0] == rnti) {
	    // clear currentC-RNTI from map
	    reestablish_rnti_map[ue_id_l][0] = 0;
	    reestablish_rnti_map[ue_id_l][1] = 0;
	    break;
	  }
	}
	for (int ii=0; ii<NUMBER_OF_UE_MAX; ii++) {
	  LTE_eNB_ULSCH_t *ulsch = RC.eNB[module_idP][CC_id]->ulsch[ii];
	  if((ulsch != NULL) && (ulsch->rnti == rnti)){
	    LOG_I(MAC, "clean_eNb_ulsch UE %x \n", rnti);
	    clean_eNb_ulsch(ulsch);
	  }
	}
	for (int ii=0; ii<NUMBER_OF_UE_MAX; ii++) {
	  LTE_eNB_DLSCH_t *dlsch = RC.eNB[module_idP][CC_id]->dlsch[ii][0];
	  if((dlsch != NULL) && (dlsch->rnti == rnti)){
	    LOG_I(MAC, "clean_eNb_dlsch UE %x \n", rnti);
	    clean_eNb_dlsch(dlsch);
	  }
	}
	
	for(int j = 0; j < 10; j++){
	  nfapi_ul_config_request_body_t *ul_req_tmp = NULL;
	  ul_req_tmp = &RC.mac[module_idP]->UL_req_tmp[CC_id][j].ul_config_request_body;
	  if(ul_req_tmp){
	    int pdu_number = ul_req_tmp->number_of_pdus;
	    for(int pdu_index = pdu_number-1; pdu_index >= 0; pdu_index--){
	      if(ul_req_tmp->ul_config_pdu_list[pdu_index].ulsch_pdu.ulsch_pdu_rel8.rnti == rnti){
		LOG_I(MAC, "remove UE %x from ul_config_pdu_list %d/%d\n", rnti, pdu_index, pdu_number);
		if(pdu_index < pdu_number -1){
		  memcpy(&ul_req_tmp->ul_config_pdu_list[pdu_index], &ul_req_tmp->ul_config_pdu_list[pdu_index+1], (pdu_number-1-pdu_index) * sizeof(nfapi_ul_config_request_pdu_t));
		}
		ul_req_tmp->number_of_pdus--;
	      }
	    }
	  }
	}
	rrc_mac_remove_ue(module_idP,rnti);
      }
    }
  }

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES,
				 NOT_A_RNTI, frameP, subframeP,
				 module_idP);
  pdcp_run(&ctxt);


  rrc_rx_tx(&ctxt, CC_id);

#if defined(Rel10) || defined(Rel14)

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    if (cc[CC_id].MBMS_flag > 0) {
      start_meas(&RC.mac[module_idP]->schedule_mch);
      mbsfn_status[CC_id] = schedule_MBMS(module_idP, CC_id, frameP, subframeP);
      stop_meas(&RC.mac[module_idP]->schedule_mch);
    }
  }

#endif

  // This schedules MIB
  if ((subframeP == 0) && (frameP & 3) == 0)
      schedule_mib(module_idP, frameP, subframeP);
  // This schedules SI for legacy LTE and eMTC starting in subframeP
  schedule_SI(module_idP, frameP, subframeP);
  // This schedules Paging in subframeP
  schedule_PCH(module_idP,frameP,subframeP);
  // This schedules Random-Access for legacy LTE and eMTC starting in subframeP
  schedule_RA(module_idP, frameP, subframeP);
  // copy previously scheduled UL resources (ULSCH + HARQ)
  copy_ulreq(module_idP, frameP, subframeP);
  // This schedules SRS in subframeP
  schedule_SRS(module_idP, frameP, subframeP);
  // This schedules ULSCH in subframeP (dci0)
  schedule_ulsch(module_idP, frameP, subframeP);
  // This schedules UCI_SR in subframeP
  schedule_SR(module_idP, frameP, subframeP);
  // This schedules UCI_CSI in subframeP
  schedule_CSI(module_idP, frameP, subframeP);
  // This schedules DLSCH in subframeP
  schedule_dlsch(module_idP, frameP, subframeP, mbsfn_status);

  if (RC.flexran[module_idP]->enabled)
    flexran_agent_send_update_stats(module_idP);
  
  // Allocate CCEs for good after scheduling is done
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++)
      allocate_CCEs(module_idP, CC_id, subframeP, 0);

  stop_meas(&RC.mac[module_idP]->eNB_scheduler);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
      (VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ULSCH_SCHEDULER,
      VCD_FUNCTION_OUT);
}
