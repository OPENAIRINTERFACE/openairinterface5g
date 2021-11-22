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
#include "LAYER2/MAC/mac.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "common/ran_context.h"
#include "nfapi/oai_integration/vendor_ext.h"

extern RAN_CONTEXT_t RC;

/*******************************************************************
*
* NAME :         nr_schedule_srs
*
* PARAMETERS :   module id
*                frame number for possible SRS reception
*                slot number for possible SRS reception
*
* DESCRIPTION :  It informs the PHY layer that has an SRS to receive.
*                Only for periodic scheduling yet.
*
*********************************************************************/
void nr_schedule_srs(int module_id, frame_t frame, sub_frame_t slot) {

  gNB_MAC_INST *nrmac = RC.nrmac[module_id];
  NR_UE_info_t *UE_info = &nrmac->UE_info;
  const NR_list_t *UE_list = &UE_info->list;

  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {

    const int CC_id = 0;
    NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels[CC_id].ServingCellConfigCommon;
    NR_CellGroupConfig_t *cg = UE_info->CellGroup[UE_id];
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

    if(!UE_info->active[UE_id]) {
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
      uint16_t offset = srs_resource->resourceType.choice.periodic->periodicityAndOffset_p.choice.sl40;

      int n_slots_frame = nr_slots_per_frame[ubwp.subcarrierSpacing];

      // Check if UE transmitted the SRS here
      if ((frame * n_slots_frame + slot - offset) % period == 0) {
        LOG_W(NR_MAC,"(%d.%d) SRS is received here, but the procedures are not implemented yet!\n", frame, slot);
      }
    }
  }
}