/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
 * \brief layer 2 interface, used to support different RRC sublayer
 */

#include "L2_interface_ue.h"
#include "assertions.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"

typedef uint32_t channel_t;

void nr_mac_rrc_meas_ind_ue(module_id_t module_id, uint32_t gNB_index, uint16_t Nid_cell, bool csi_meas, bool is_neighboring_cell, int rsrp_dBm)
{
  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_MEAS_DATA_IND);
  NR_RRC_MAC_MEAS_DATA_IND(message_p).gnb_index = gNB_index;
  NR_RRC_MAC_MEAS_DATA_IND(message_p).Nid_cell = Nid_cell;
  NR_RRC_MAC_MEAS_DATA_IND(message_p).is_csi_meas = csi_meas;
  NR_RRC_MAC_MEAS_DATA_IND(message_p).is_neighboring_cell = is_neighboring_cell;
  NR_RRC_MAC_MEAS_DATA_IND(message_p).rsrp_dBm = rsrp_dBm;
  itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(module_id), message_p);
}

void nr_mac_rrc_sync_ind(const module_id_t module_id, const frame_t frame, const bool in_sync)
{
  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_SYNC_IND);
  NR_RRC_MAC_SYNC_IND (message_p).frame = frame;
  NR_RRC_MAC_SYNC_IND (message_p).in_sync = in_sync;
  itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(module_id), message_p);
}

void nr_mac_rrc_data_ind_ue(const module_id_t module_id,
                            const uint8_t gNB_index,
                            const int hfn,
                            const frame_t frame,
                            const int slot,
                            const uint32_t cellid,
                            const long arfcn,
                            const channel_t channel,
                            const uint8_t* pduP,
                            const sdu_size_t pdu_len)
{
  sdu_size_t sdu_size = 0;
  MessageDef *message_p;
  switch(channel) {
    case NR_BCCH_BCH:
    case NR_BCCH_DL_SCH:
      message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_BCCH_DATA_IND);
      memset(NR_RRC_MAC_BCCH_DATA_IND (message_p).sdu, 0, BCCH_SDU_SIZE);
      if (pdu_len > 0) {
        LOG_T(NR_RRC,
              "[UE %d] Received SDU for NR-BCCH-DL-SCH on SRB %u from gNB %d\n",
              module_id,
              channel & RAB_OFFSET,
              gNB_index);

        int msg_sdu_size = BCCH_SDU_SIZE;

        if (pdu_len > msg_sdu_size) {
          LOG_E(NR_RRC, "SDU larger than NR-BCCH-DL-SCH SDU buffer size (%d, %d)", sdu_size, msg_sdu_size);
          sdu_size = msg_sdu_size;
        } else {
          sdu_size = pdu_len;
        }

        memcpy(NR_RRC_MAC_BCCH_DATA_IND (message_p).sdu, pduP, sdu_size);
        NR_RRC_MAC_BCCH_DATA_IND (message_p).sdu_size = sdu_size;
      }
      else {
        NR_RRC_MAC_BCCH_DATA_IND (message_p).sdu_size = 0;
      }
      NR_RRC_MAC_BCCH_DATA_IND (message_p).hfn = hfn;
      NR_RRC_MAC_BCCH_DATA_IND (message_p).frame = frame; //frameP
      NR_RRC_MAC_BCCH_DATA_IND (message_p).slot = slot;
      NR_RRC_MAC_BCCH_DATA_IND (message_p).gnb_index = gNB_index;
      NR_RRC_MAC_BCCH_DATA_IND (message_p).phycellid = cellid;
      NR_RRC_MAC_BCCH_DATA_IND (message_p).ssb_arfcn = arfcn;
      NR_RRC_MAC_BCCH_DATA_IND (message_p).is_bch = (channel == NR_BCCH_BCH);
      itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(module_id), message_p);
      break;
    case NR_SBCCH_SL_BCH:
      if (pdu_len > 0) {
        LOG_T(NR_RRC, "[UE %d] Received SL-MIB for NR_SBCCH_SL_BCH.\n", module_id);
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
        NR_RRC_MAC_SBCCH_DATA_IND (message_p).hfn = hfn;
        NR_RRC_MAC_SBCCH_DATA_IND (message_p).frame = frame; //frameP
        NR_RRC_MAC_SBCCH_DATA_IND (message_p).slot = slot;
        NR_RRC_MAC_SBCCH_DATA_IND (message_p).sdu_size = sdu_size;
        NR_RRC_MAC_SBCCH_DATA_IND(message_p).rx_slss_id = cellid; // cellid is rx slss id
        itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(module_id), message_p);
      }
      break;
    default:
      AssertFatal(false, "Invalid channel in data indication\n");
  }
}

void process_msg_rcc_to_mac(nr_mac_rrc_message_t *msg, int instance_id)
{
  switch (msg->payload_type) {
    case NR_MAC_RRC_CONFIG_RESET:
      nr_rrc_mac_config_req_reset(instance_id, msg->payload.config_reset.cause);
      break;
    case NR_MAC_RRC_CONFIG_CG:
      nr_rrc_mac_config_req_cg(instance_id,
                               0,
                               msg->payload.config_cg.hfn,
                               msg->payload.config_cg.frame,
                               msg->payload.config_cg.cellGroupConfig,
                               msg->payload.config_cg.UE_NR_Capability);
      ASN_STRUCT_FREE(asn_DEF_NR_CellGroupConfig, msg->payload.config_cg.cellGroupConfig);
      break;
    case NR_MAC_RRC_CONFIG_MIB:
      nr_rrc_mac_config_req_mib(instance_id,
                                0,
                                msg->payload.config_mib.bcch->message.choice.mib,
                                msg->payload.config_mib.access_barred);
      ASN_STRUCT_FREE(asn_DEF_NR_BCCH_BCH_Message, msg->payload.config_mib.bcch);
      break;
    case NR_MAC_RRC_CONFIG_SIB1: {
      NR_SIB1_t *sib1 = msg->payload.config_sib1.sib1;
      bool can_start_ra = msg->payload.config_sib1.can_start_ra;
      bool is_dedicated = msg->payload.config_sib1.is_dedicated_sib1;
      nr_rrc_mac_config_req_sib1(instance_id, 0, sib1, can_start_ra, is_dedicated);
      SEQUENCE_free(&asn_DEF_NR_SIB1, msg->payload.config_sib1.sib1, ASFM_FREE_EVERYTHING);
    } break;
    case NR_MAC_RRC_CONFIG_OTHER_SIB: {
      nr_rrc_mac_config_other_sib(instance_id,
                                  msg->payload.config_other_sib.sib19,
                                  msg->payload.config_other_sib.hfn,
                                  msg->payload.config_other_sib.frame,
                                  msg->payload.config_other_sib.can_start_ra);
    } break;
    case NR_MAC_RRC_START_RA:
      nr_rrc_mac_start_ra(instance_id, msg->payload.start_ra.cause);
      break;
    case NR_MAC_RRC_SCHED_SIB:
      nr_rrc_mac_sched_sib(instance_id, msg->payload.sched_sib.get_sib);
      break;
    case NR_MAC_RRC_RESUME_RB:
      nr_rrc_mac_resume_rb(instance_id, msg->payload.resume_rb.is_srb, msg->payload.resume_rb.rb_id);
      break;
    default:
      LOG_E(NR_MAC, "Unexpected msg from RRC: %d\n", msg->payload_type);
  }
}

void nr_mac_rrc_verification_failed(const module_id_t mod_id)
{
  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_VERIFY);
  NR_RRC_MAC_VERIFY (message_p).L2_RRC_verification_failed = true;
  itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(mod_id), message_p);
}

void nr_mac_rrc_inactivity_timer_ind(const module_id_t mod_id)
{
  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_INAC_IND);
  NR_RRC_MAC_INAC_IND (message_p).inactivity_timer_expired = true;
  itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(mod_id), message_p);
}

void nr_mac_rrc_msg3_ind(const module_id_t mod_id, const int rnti, bool prepare_payload)
{
  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_MSG3_IND);
  NR_RRC_MAC_MSG3_IND (message_p).rnti = rnti;
  NR_RRC_MAC_MSG3_IND (message_p).prepare_payload = prepare_payload;
  itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(mod_id), message_p);
}

void nr_ue_rrc_timer_trigger(int instance, int hfn, int frame, int gnb_id)
{
  MessageDef *message_p;
  message_p = itti_alloc_new_message(TASK_RRC_NRUE, 0, NRRRC_FRAME_PROCESS);
  NRRRC_FRAME_PROCESS(message_p).hfn = hfn;
  NRRRC_FRAME_PROCESS(message_p).frame = frame;
  NRRRC_FRAME_PROCESS(message_p).gnb_id = gnb_id;
  LOG_D(NR_MAC, "MAC/PHY send to RRC we have a new frame %d\n", frame);
  itti_send_msg_to_task(TASK_RRC_NRUE, instance, message_p);
}

void nr_mac_rrc_ra_ind(const module_id_t mod_id, bool success)
{
  MessageDef *message_p = itti_alloc_new_message(TASK_MAC_UE, 0, NR_RRC_MAC_RA_IND);
  NR_RRC_MAC_RA_IND (message_p).RA_succeeded = success;
  itti_send_msg_to_task(TASK_RRC_NRUE, GNB_MODULE_ID_TO_INSTANCE(mod_id), message_p);
}
