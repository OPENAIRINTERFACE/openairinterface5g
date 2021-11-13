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

/*! \file     gNB_scheduler_SRS.c
 * \brief     primitives used for sounding reference signal
 * \date      2021
 */

#include "platform_types.h"

/* MAC */
#include "nr_mac_gNB.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"

/* Utils */
#include "common/utils/LOG/vcd_signal_dumper.h"

void nr_schedule_srs_aperiodic(module_id_t module_idP,
                               frame_t frameP,
                               sub_frame_t slotP,
                               NR_SRS_ResourceSet_t *srs_resource_set) {
  LOG_W(NR_PHY, "Aperiodic SRS scheduling is not implemented yet!\n");
}

void nr_schedule_srs_semi_persistent(module_id_t module_idP,
                               frame_t frameP,
                               sub_frame_t slotP,
                               NR_SRS_ResourceSet_t *srs_resource_set) {
  LOG_W(NR_PHY, "Semi-persistent SRS scheduling is not implemented yet!\n");
}

void nr_schedule_srs_periodic(module_id_t module_idP,
                                     frame_t frameP,
                                     sub_frame_t slotP,
                                     NR_SRS_ResourceSet_t *srs_resource_set) {
  LOG_W(NR_PHY, "Periodic SRS scheduling is not implemented yet!\n");
}

void nr_schedule_srs(module_id_t module_idP, frame_t frameP, sub_frame_t slotP) {

  gNB_MAC_INST *mac = RC.nrmac[module_idP];

  for (int UE_id = 0; UE_id < MAX_MOBILES_PER_GNB; UE_id++) {
    NR_CellGroupConfig_t *cg = mac->UE_info.CellGroup[UE_id];
    NR_BWP_UplinkDedicated_t *ubwpd = NULL;
    if (cg &&
        cg->spCellConfig &&
        cg->spCellConfig->spCellConfigDedicated &&
        cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
        cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP) {
      ubwpd = cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP;
    } else {
      continue;
    }

    NR_SRS_Config_t *srs_Config = ubwpd->srs_Config->choice.setup;

    for(int rs = 0; rs < srs_Config->srs_ResourceSetToAddModList->list.count; rs++) {
      NR_SRS_ResourceSet_t *srs_resource_set = srs_Config->srs_ResourceSetToAddModList->list.array[rs];
      switch (srs_resource_set->resourceType.present) {
        case NR_SRS_ResourceSet__resourceType_PR_aperiodic:
          nr_schedule_srs_aperiodic(module_idP, frameP, slotP, srs_resource_set);
          break;
        case NR_SRS_ResourceSet__resourceType_PR_semi_persistent:
          nr_schedule_srs_semi_persistent(module_idP, frameP, slotP, srs_resource_set);
          break;
        case NR_SRS_ResourceSet__resourceType_PR_periodic:
          nr_schedule_srs_periodic(module_idP, frameP, slotP, srs_resource_set);
          break;
        default:
          break;
      }
    }
  }
}