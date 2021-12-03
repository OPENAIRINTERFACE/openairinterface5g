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

/*! \file gNB_scheduler_srs.c
 * \brief MAC procedures related to SRS
 * \date 2021
 * \version 1.0
 */

#include <softmodem-common.h>
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "common/ran_context.h"
#include "nfapi/oai_integration/vendor_ext.h"

extern RAN_CONTEXT_t RC;

void nr_configure_srs(nfapi_nr_srs_pdu_t *srs_pdu, int module_id, int CC_id, int UE_id, NR_SRS_Resource_t *srs_resource) {

  gNB_MAC_INST *nrmac = RC.nrmac[module_id];
  NR_ServingCellConfigCommon_t *scc = nrmac->common_channels[CC_id].ServingCellConfigCommon;
  NR_UE_info_t *UE_info = &nrmac->UE_info;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

  NR_BWP_t ubwp = sched_ctrl->active_ubwp ?
                  sched_ctrl->active_ubwp->bwp_Common->genericParameters :
                  scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;

  srs_pdu->rnti = UE_info->rnti[UE_id];
  srs_pdu->handle = 0;
  srs_pdu->bwp_size = NRRIV2BW(ubwp.locationAndBandwidth, MAX_BWP_SIZE);;
  srs_pdu->bwp_start = NRRIV2PRBOFFSET(ubwp.locationAndBandwidth, MAX_BWP_SIZE);;
  srs_pdu->subcarrier_spacing = ubwp.subcarrierSpacing;
  srs_pdu->cyclic_prefix = 0;
  srs_pdu->num_ant_ports = srs_resource->nrofSRS_Ports;
  srs_pdu->num_symbols = srs_resource->resourceMapping.nrofSymbols;
  srs_pdu->num_repetitions = srs_resource->resourceMapping.repetitionFactor;
  srs_pdu->time_start_position = srs_resource->resourceMapping.startPosition;
  srs_pdu->config_index = srs_resource->freqHopping.c_SRS;
  srs_pdu->sequence_id = srs_resource->sequenceId;
  srs_pdu->bandwidth_index = srs_resource->freqHopping.b_SRS;
  srs_pdu->comb_size = srs_resource->transmissionComb.present - 1;

  switch(srs_resource->transmissionComb.present) {
    case NR_SRS_Resource__transmissionComb_PR_n2:
      srs_pdu->comb_offset = srs_resource->transmissionComb.choice.n2->combOffset_n2;
      srs_pdu->cyclic_shift = srs_resource->transmissionComb.choice.n2->cyclicShift_n2;
      break;
    case NR_SRS_Resource__transmissionComb_PR_n4:
      srs_pdu->comb_offset = srs_resource->transmissionComb.choice.n4->combOffset_n4;
      srs_pdu->cyclic_shift = srs_resource->transmissionComb.choice.n4->cyclicShift_n4;
      break;
    default:
      LOG_W(NR_MAC, "Invalid or not implemented comb_size!\n");
  }

  srs_pdu->frequency_position = srs_resource->freqDomainPosition;
  srs_pdu->frequency_shift = srs_resource->freqDomainShift;
  srs_pdu->frequency_hopping = srs_resource->freqHopping.b_hop;
  srs_pdu->group_or_sequence_hopping = srs_resource->groupOrSequenceHopping;
  srs_pdu->resource_type = srs_resource->resourceType.present - 1;
  srs_pdu->t_srs = srs_period[srs_resource->resourceType.choice.periodic->periodicityAndOffset_p.present];
  srs_pdu->t_offset = get_nr_srs_offset(srs_resource->resourceType.choice.periodic->periodicityAndOffset_p);
}

void nr_fill_nfapi_srs(int module_id, int CC_id, int UE_id, sub_frame_t slot, NR_SRS_Resource_t *srs_resource) {

  nfapi_nr_ul_tti_request_t *future_ul_tti_req = &RC.nrmac[module_id]->UL_tti_req_ahead[0][slot];

  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE;
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_srs_pdu_t);
  nfapi_nr_srs_pdu_t *srs_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].srs_pdu;
  memset(srs_pdu, 0, sizeof(nfapi_nr_srs_pdu_t));
  future_ul_tti_req->n_pdus += 1;

  nr_configure_srs(srs_pdu, module_id, CC_id, UE_id, srs_resource);
}

/*******************************************************************
*
* NAME :         nr_schedule_srs
*
* PARAMETERS :   module id
*                frame number for possible SRS reception
*
* DESCRIPTION :  It informs the PHY layer that has an SRS to receive.
*                Only for periodic scheduling yet.
*
*********************************************************************/
void nr_schedule_srs(int module_id, frame_t frame) {

  gNB_MAC_INST *nrmac = RC.nrmac[module_id];
  NR_UE_info_t *UE_info = &nrmac->UE_info;
  const NR_list_t *UE_list = &UE_info->list;

  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {

    const int CC_id = 0;
    NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels[CC_id].ServingCellConfigCommon;
    NR_CellGroupConfig_t *cg = UE_info->CellGroup[UE_id];
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

    sched_ctrl->sched_srs.frame = -1;
    sched_ctrl->sched_srs.slot = -1;
    sched_ctrl->sched_srs.srs_scheduled = false;

    if(!UE_info->Msg4_ACKed[UE_id]) {
      continue;
    }

    NR_SRS_Config_t *srs_config = NULL;
    if (cg &&
        cg->spCellConfig &&
        cg->spCellConfig->spCellConfigDedicated &&
        cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
        cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP) {
      srs_config = cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->srs_Config->choice.setup;
    } else {
      continue;
    }

    for(int rs = 0; rs < srs_config->srs_ResourceSetToAddModList->list.count; rs++) {

      // Find periodic resource set
      NR_SRS_ResourceSet_t *srs_resource_set = srs_config->srs_ResourceSetToAddModList->list.array[rs];
      if (srs_resource_set->resourceType.present != NR_SRS_ResourceSet__resourceType_PR_periodic) {
        continue;
      }

      // Find the corresponding srs resource
      NR_SRS_Resource_t *srs_resource = NULL;
      for (int r1 = 0; r1 < srs_resource_set->srs_ResourceIdList->list.count; r1++) {
        for (int r2 = 0; r2 < srs_config->srs_ResourceToAddModList->list.count; r2++) {
          if ((*srs_resource_set->srs_ResourceIdList->list.array[r1] ==
               srs_config->srs_ResourceToAddModList->list.array[r2]->srs_ResourceId) &&
              (srs_config->srs_ResourceToAddModList->list.array[r2]->resourceType.present ==
               NR_SRS_Resource__resourceType_PR_periodic)) {
            srs_resource = srs_config->srs_ResourceToAddModList->list.array[r2];
            break;
          }
        }
      }

      if (srs_resource == NULL) {
        continue;
      }

      NR_BWP_t ubwp = sched_ctrl->active_ubwp ?
                        sched_ctrl->active_ubwp->bwp_Common->genericParameters :
                        scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;

      uint16_t period = srs_period[srs_resource->resourceType.choice.periodic->periodicityAndOffset_p.present];
      uint16_t offset = get_nr_srs_offset(srs_resource->resourceType.choice.periodic->periodicityAndOffset_p);

      int n_slots_frame = nr_slots_per_frame[ubwp.subcarrierSpacing];

      // Check if UE will transmit the SRS in this frame
      if ( ((frame - offset/n_slots_frame)*n_slots_frame)%period == 0) {
        LOG_D(NR_MAC,"(%d.%d) Scheduling SRS reception\n", frame, offset%n_slots_frame);
        nr_fill_nfapi_srs(module_id, CC_id, UE_id, offset%n_slots_frame, srs_resource);
        sched_ctrl->sched_srs.frame = frame;
        sched_ctrl->sched_srs.slot = offset%n_slots_frame;
        sched_ctrl->sched_srs.srs_scheduled = true;
      }
    }
  }
}