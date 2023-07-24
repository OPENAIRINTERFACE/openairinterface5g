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

/* \file l2_interface_ue.c
 * \brief layer 2 interface, used to support different RRC sublayer
 * \author R. Knopp, K.H. HSU
 * \date 2018
 * \version 0.1
 * \company Eurecom / NTUST
 * \email: knopp@eurecom.fr, kai-hsiang.hsu@eurecom.fr
 * \note
 * \warning
 */

#include "rrc_defs.h"
#include "rrc_proto.h"
#include "assertions.h"
#include "MAC/mac.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"

typedef uint32_t channel_t;

void nr_mac_rrc_sync_ind(const module_id_t module_id,
                         const frame_t frame,
                         const bool in_sync)
{
  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_SYNC_IND);
  NR_RRC_MAC_SYNC_IND (message_p).frame = frame;
  NR_RRC_MAC_SYNC_IND (message_p).in_sync = in_sync;
  itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(module_id), message_p);
}

int8_t nr_mac_rrc_data_ind_ue(const module_id_t module_id,
                              const int CC_id,
                              const uint8_t gNB_index,
                              const frame_t frame,
                              const int slot,
                              const rnti_t rnti,
                              const channel_t channel,
                              const uint8_t* pduP,
                              const sdu_size_t pdu_len)
{
  sdu_size_t sdu_size = 0;

  switch(channel) {
    case NR_BCCH_BCH:
    case NR_BCCH_DL_SCH:
      if (pdu_len>0) {
        LOG_T(NR_RRC, "[UE %d] Received SDU for NR-BCCH-DL-SCH on SRB %u from gNB %d\n", module_id, channel & RAB_OFFSET,
              gNB_index);

        MessageDef *message_p;
        int msg_sdu_size = BCCH_SDU_SIZE;

        if (pdu_len > msg_sdu_size) {
          LOG_E(NR_RRC, "SDU larger than NR-BCCH-DL-SCH SDU buffer size (%d, %d)", sdu_size, msg_sdu_size);
          sdu_size = msg_sdu_size;
        } else {
          sdu_size = pdu_len;
        }

        message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_BCCH_DATA_IND);
        memset(NR_RRC_MAC_BCCH_DATA_IND (message_p).sdu, 0, BCCH_SDU_SIZE);
        memcpy(NR_RRC_MAC_BCCH_DATA_IND (message_p).sdu, pduP, sdu_size);
        NR_RRC_MAC_BCCH_DATA_IND (message_p).frame = frame; //frameP
        NR_RRC_MAC_BCCH_DATA_IND (message_p).slot = slot;
        NR_RRC_MAC_BCCH_DATA_IND (message_p).sdu_size = sdu_size;
        NR_RRC_MAC_BCCH_DATA_IND (message_p).gnb_index = gNB_index;
        NR_RRC_MAC_BCCH_DATA_IND (message_p).is_bch = (channel == NR_BCCH_BCH);
        itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(module_id), message_p);
      }
      break;

    case CCCH:
      AssertFatal(false, "use RLC instead\n");
      break;

    case NR_SBCCH_SL_BCH:
      if (pdu_len>0) {
        LOG_T(NR_RRC, "[UE %d] Received SL-MIB for NR_SBCCH_SL_BCH.\n", module_id);

        MessageDef *message_p;
        int msg_sdu_size = BCCH_SDU_SIZE;

        if (pdu_len > msg_sdu_size) {
          LOG_E(NR_RRC, "SDU larger than NR_SBCCH_SL_BCH SDU buffer size (%d, %d)", sdu_size, msg_sdu_size);
          sdu_size = msg_sdu_size;
        } else {
          sdu_size = pdu_len;
        }

        message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_SBCCH_DATA_IND);
        memset(NR_RRC_MAC_SBCCH_DATA_IND (message_p).sdu, 0, BCCH_SDU_SIZE);
        memcpy(NR_RRC_MAC_SBCCH_DATA_IND (message_p).sdu, pduP, sdu_size);
        NR_RRC_MAC_SBCCH_DATA_IND (message_p).frame = frame; //frameP
        NR_RRC_MAC_SBCCH_DATA_IND (message_p).slot = slot;
        NR_RRC_MAC_SBCCH_DATA_IND (message_p).sdu_size = sdu_size;
        NR_RRC_MAC_SBCCH_DATA_IND (message_p).gnb_index = gNB_index;
        NR_RRC_MAC_SBCCH_DATA_IND (message_p).rx_slss_id = rnti;//rx_slss_id is rnti
        itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(module_id), message_p);
      }
      break;

    default:
      break;
  }

  return(0);
}

void nr_mac_rrc_msg3_ind(const module_id_t mod_id, const int rnti)
{
  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_MSG3_IND);
  NR_RRC_MAC_MSG3_IND (message_p).rnti = rnti;
  itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(mod_id), message_p);
}

void nr_ue_rrc_timer_trigger(int instance, int frame, int gnb_id)
{
  MessageDef *message_p;
  message_p = itti_alloc_new_message(TASK_RRC_NRUE, 0, NRRRC_FRAME_PROCESS);
  NRRRC_FRAME_PROCESS(message_p).frame = frame;
  NRRRC_FRAME_PROCESS(message_p).gnb_id = gnb_id;
  LOG_D(NR_RRC, "RRC timer trigger: frame %d\n", frame);
  itti_send_msg_to_task(TASK_RRC_NRUE, instance, message_p);
}

void nr_mac_rrc_ra_ind(const module_id_t mod_id, int frame, bool success)
{
  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_RA_IND);
  NR_RRC_MAC_RA_IND (message_p).frame = frame;
  NR_RRC_MAC_RA_IND (message_p).RA_succeeded = success;
  itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(mod_id), message_p);
}
