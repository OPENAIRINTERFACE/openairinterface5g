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

/*! \file gNB_scheduler_ulsch.c
 * \brief gNB procedures for the ULSCH transport channel
 * \author Navid Nikaein and Raymond Knopp, Guido Casati
 * \date 2019
 * \email: guido.casati@iis.fraunhofer.de
 * \version 1.0
 * @ingroup _mac
 */

#include "LAYER2/NR_MAC_gNB/mac_proto.h"

/*
* When data are received on PHY and transmitted to MAC
*/
void nr_rx_sdu(const module_id_t gnb_mod_idP,
               const int CC_idP,
               const frame_t frameP,
               const sub_frame_t subframeP,
               const rnti_t rntiP,
               uint8_t *sduP,
               const uint16_t sdu_lenP,
               const uint16_t timing_advance,
               const uint8_t ul_cqi){
  int current_rnti = 0, UE_id = -1, harq_pid = 0;
  gNB_MAC_INST *gNB_mac = NULL;
  NR_UE_list_t *UE_list = NULL;
  UE_sched_ctrl_t *UE_scheduling_control = NULL;

  current_rnti = rntiP;
  UE_id = find_nr_UE_id(gnb_mod_idP, current_rnti);
  gNB_mac = RC.nrmac[gnb_mod_idP];
  UE_list = &gNB_mac->UE_list;

  if (UE_id != -1) {
    UE_scheduling_control = &(UE_list->UE_sched_ctrl[UE_id]);

    LOG_D(MAC, "[gNB %d][PUSCH %d] CC_id %d %d.%d Received ULSCH sdu round %d from PHY (rnti %x, UE_id %d) ul_cqi %d\n",
          gnb_mod_idP,
          harq_pid,
          CC_idP,
          frameP,
          subframeP,
          UE_scheduling_control->round_UL[CC_idP][harq_pid],
          current_rnti,
          UE_id,
          ul_cqi);

    if (sduP != NULL)
      UE_scheduling_control->ta_update = timing_advance;
  }
}