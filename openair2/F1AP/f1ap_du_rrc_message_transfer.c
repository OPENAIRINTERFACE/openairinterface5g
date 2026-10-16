/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "f1ap_common.h"
#include "f1ap_encoder.h"
#include "f1ap_itti_messaging.h"

#include "f1ap_du_rrc_message_transfer.h"

#include "uper_decoder.h"

#include "NR_DL-CCCH-Message.h"
#include "NR_UL-CCCH-Message.h"
#include "NR_DL-DCCH-Message.h"
#include "NR_UL-DCCH-Message.h"
// for SRB1_logicalChannelConfig_defaultValue
#include "common/ran_context.h"

#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "intertask_interface.h"

#include "openair2/LAYER2/NR_MAC_gNB/mac_rrc_dl_handler.h"

#include "f1ap_rrc_message_transfer.h"

/*  DL RRC Message Transfer */
int DU_handle_DL_RRC_MESSAGE_TRANSFER(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  UNUSED(instance);
  UNUSED(assoc_id);
  UNUSED(stream);
  f1ap_dl_rrc_message_t msg = {0};
  if (!decode_dl_rrc_message_transfer(pdu, &msg)) {
    LOG_E(F1AP, "cannot decode F1 DL RRC message Transfer\n");
    free_dl_rrc_message_transfer(&msg);
    return -1;
  }

  dl_rrc_message_transfer(&msg);
  /* Free DL RRC Message Transfer */
  free_dl_rrc_message_transfer(&msg);
  return 0;
}

/*  UL RRC Message Transfer */
int DU_send_INITIAL_UL_RRC_MESSAGE_TRANSFER(sctp_assoc_t assoc_id, const f1ap_initial_ul_rrc_message_t *msg)
{
  /* encode Initial UL RRC Message Transfer */
  F1AP_F1AP_PDU_t *pdu = encode_initial_ul_rrc_message_transfer(msg);
  /* free F1AP message after encoding */
  free_initial_ul_rrc_message_transfer(msg);

  if (pdu == NULL) {
    LOG_E(F1AP, "cannot encode F1 INITIAL UL RRC MESSAGE TRANSFER, can't send message\n");
    ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
    return -1;
  }

  /* encode F1AP PDU */
  uint8_t *buffer = NULL;
  uint32_t len = 0;
  if (f1ap_encode_pdu(pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 INITIAL UL RRC MESSAGE TRANSFER\n");
    ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
    return -1;
  }
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);

  /* Send to ITTI */
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int DU_send_UL_NR_RRC_MESSAGE_TRANSFER(sctp_assoc_t assoc_id, const f1ap_ul_rrc_message_t *msg)
{
  LOG_D(F1AP,
        "size %d UE RNTI %x in SRB %d\n",
        msg->rrc_container_length,
        msg->gNB_DU_ue_id,
        msg->srb_id);
  /* encode UL RRC Message Transfer */
  F1AP_F1AP_PDU_t *pdu = encode_ul_rrc_message_transfer(msg);
  if (pdu == NULL) {
    LOG_E(F1AP, "Failed to encode F1 UL RRC MESSAGE TRANSFER: cannot send message\n");
    ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
    return -1;
  }
  /* encode F1AP PDU */
  uint8_t *buffer = NULL;
  uint32_t len;
  if (f1ap_encode_pdu(pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UL RRC MESSAGE TRANSFER \n");
    ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
    return -1;
  }
  /* Free F1AP PDU */
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
  /* Send to ITTI */
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}
