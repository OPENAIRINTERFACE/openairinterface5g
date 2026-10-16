/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "f1ap_common.h"
#include "f1ap_encoder.h"
#include "f1ap_itti_messaging.h"
#include "f1ap_cu_ue_context_management.h"
#include "lib/f1ap_ue_context.h"
#include <string.h>

#include "openair2/RRC/NR/rrc_gNB_NGAP.h"

#include "F1AP_UEContextModificationRequired.h"
#include "F1AP_InitiatingMessage.h"
#include "F1AP_SuccessfulOutcome.h"
#include "F1AP_UnsuccessfulOutcome.h"
#include "F1AP_ProtocolIE-Field.h"

#ifdef E2_AGENT
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "openair2/E2AP/RAN_FUNCTION/O-RAN/ran_func_rc_extern.h"
#endif

int CU_send_UE_CONTEXT_SETUP_REQUEST(sctp_assoc_t assoc_id, const f1ap_ue_context_setup_req_t *req)
{
  F1AP_F1AP_PDU_t *pdu = encode_ue_context_setup_req(req);

  uint8_t *buffer = NULL;
  uint32_t len = 0;
  if (f1ap_encode_pdu(pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE CONTEXT SETUP REQUEST\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);

#ifdef E2_AGENT
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[0], req->gNB_CU_ue_id);
  signal_ue_id(&ue_context_p->ue_context, F1_NETWORK_INTERFACE_TYPE, 0);
#endif

  return 0;
}

int CU_handle_UE_CONTEXT_SETUP_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  UNUSED(stream);
  f1ap_ue_context_setup_resp_t resp = {0};
  if (!decode_ue_context_setup_resp(pdu, &resp)) {
    LOG_E(F1AP, "cannot decode F1 UE Context Setup Resp\n");
    free_ue_context_setup_resp(&resp);
    return -1;
  }

  MessageDef *msg_p = itti_alloc_new_message(TASK_DU_F1, 0, F1AP_UE_CONTEXT_SETUP_RESP);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  F1AP_UE_CONTEXT_SETUP_RESP(msg_p) = resp;
  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg_p);
  return 0;
}

int CU_handle_UE_CONTEXT_SETUP_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  UNUSED(instance);
  UNUSED(assoc_id);
  UNUSED(stream);
  UNUSED(pdu);
  AssertFatal(1==0,"Not implemented yet\n");
}

int CU_handle_UE_CONTEXT_RELEASE_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  UNUSED(stream);
  f1ap_ue_context_rel_req_t req = {0};
  if (!decode_ue_context_rel_req(pdu, &req)) {
    LOG_E(F1AP, "cannot decode F1 UE Context Release Request\n");
    free_ue_context_rel_req(&req);
    return -1;
  }

  MessageDef *msg = itti_alloc_new_message(TASK_CU_F1, 0,  F1AP_UE_CONTEXT_RELEASE_REQ);
  msg->ittiMsgHeader.originInstance = assoc_id;
  F1AP_UE_CONTEXT_RELEASE_REQ(msg) = req;
  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg);
  return 0;
}

int CU_send_UE_CONTEXT_RELEASE_COMMAND(sctp_assoc_t assoc_id, f1ap_ue_context_rel_cmd_t *cmd)
{
  F1AP_F1AP_PDU_t *pdu = encode_ue_context_rel_cmd(cmd);

  uint8_t *buffer = NULL;
  uint32_t len = 0;
  if (f1ap_encode_pdu(pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 context release command\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
  return 0;
}

int CU_handle_UE_CONTEXT_RELEASE_COMPLETE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  UNUSED(stream);
  f1ap_ue_context_rel_cplt_t cplt = {0};
  if (!decode_ue_context_rel_cplt(pdu, &cplt)) {
    LOG_E(F1AP, "cannot decode F1 UE Context Release Complete\n");
    free_ue_context_rel_cplt(&cplt);
    return -1;
  }

  MessageDef *msg_p = itti_alloc_new_message(TASK_DU_F1, 0,  F1AP_UE_CONTEXT_RELEASE_COMPLETE);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  F1AP_UE_CONTEXT_RELEASE_COMPLETE(msg_p) = cplt;
  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg_p);
  return 0;
}

int CU_send_UE_CONTEXT_MODIFICATION_REQUEST(sctp_assoc_t assoc_id, const f1ap_ue_context_mod_req_t *req)
{
  F1AP_F1AP_PDU_t *pdu = encode_ue_context_mod_req(req);

  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  if (f1ap_encode_pdu(pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE Context Modification Request\n");
    return -1;
  }

  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
  return 0;
}

int CU_handle_UE_CONTEXT_MODIFICATION_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  UNUSED(stream);
  f1ap_ue_context_mod_resp_t resp = {0};
  if (!decode_ue_context_mod_resp(pdu, &resp)) {
    LOG_E(F1AP, "cannot decode F1 UE Context Modification Response\n");
    free_ue_context_mod_resp(&resp);
    return -1;
  }
  MessageDef *msg_p = itti_alloc_new_message(TASK_DU_F1, 0, F1AP_UE_CONTEXT_MODIFICATION_RESP);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  F1AP_UE_CONTEXT_MODIFICATION_RESP(msg_p) = resp;
  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg_p);
  return 0;
}

int CU_handle_UE_CONTEXT_MODIFICATION_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  UNUSED(instance);
  UNUSED(assoc_id);
  UNUSED(stream);
  UNUSED(pdu);
  AssertFatal(1 == 0, "Not implemented yet\n");
}

int CU_handle_UE_CONTEXT_MODIFICATION_REQUIRED(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);
  UNUSED(stream);
  MessageDef *msg_p = itti_alloc_new_message(TASK_DU_F1, 0, F1AP_UE_CONTEXT_MODIFICATION_REQUIRED);
  msg_p->ittiMsgHeader.originInstance = assoc_id;
  f1ap_ue_context_modif_required_t *required = &F1AP_UE_CONTEXT_MODIFICATION_REQUIRED(msg_p);

  F1AP_UEContextModificationRequired_t *container = &pdu->choice.initiatingMessage->value.choice.UEContextModificationRequired;
  F1AP_UEContextModificationRequiredIEs_t *ie = NULL;

  /* required: GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  required->gNB_CU_ue_id = ie->value.choice.GNB_CU_UE_F1AP_ID;

  /* required: GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  required->gNB_DU_ue_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

  /* optional: Resource Coordination Transfer Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_ResourceCoordinationTransferContainer,
                             false);
  AssertFatal(ie == NULL, "handling of Resource Coordination Transfer Container not implemented\n");

  /* optional: DU to CU RRC Information */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_DUtoCURRCInformation,
                             false);
  if (ie != NULL) {
    F1AP_DUtoCURRCInformation_t *du2cu = &ie->value.choice.DUtoCURRCInformation;
    required->du_to_cu_rrc_information = malloc(sizeof(*required->du_to_cu_rrc_information));
    AssertFatal(required->du_to_cu_rrc_information != NULL, "memory allocation failed\n");

    required->du_to_cu_rrc_information->cellGroupConfig = malloc(du2cu->cellGroupConfig.size);
    AssertFatal(required->du_to_cu_rrc_information->cellGroupConfig != NULL, "memory allocation failed\n");
    memcpy(required->du_to_cu_rrc_information->cellGroupConfig, du2cu->cellGroupConfig.buf, du2cu->cellGroupConfig.size);
    required->du_to_cu_rrc_information->cellGroupConfig_length = du2cu->cellGroupConfig.size;

    AssertFatal(du2cu->measGapConfig == NULL, "handling of measGapConfig not implemented\n");
    AssertFatal(du2cu->requestedP_MaxFR1 == NULL, "handling of requestedP_MaxFR1 not implemented\n");
  }

  /* optional: DRB Required to Be Modified List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_DRBs_Required_ToBeModified_List,
                             false);
  AssertFatal(ie == NULL, "handling of DRBs Required to be modified list not implemented\n");

  /* optional: SRB Required to be Released List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_SRBs_Required_ToBeReleased_List,
                             false);
  AssertFatal(ie == NULL, "handling of SRBs Required to be released list not implemented\n");

  /* optional: DRB Required to be Released List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_DRBs_Required_ToBeReleased_List,
                             false);
  AssertFatal(ie == NULL, "handling of DRBs Required to be released list not implemented\n");

  /* mandatory: Cause */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t, ie, container, F1AP_ProtocolIE_ID_id_Cause, true);
  switch (ie->value.choice.Cause.present) {
    case F1AP_Cause_PR_radioNetwork:
      required->cause = F1AP_CAUSE_RADIO_NETWORK;
      required->cause_value = ie->value.choice.Cause.choice.radioNetwork;
      break;
    case F1AP_Cause_PR_transport:
      required->cause = F1AP_CAUSE_TRANSPORT;
      required->cause_value = ie->value.choice.Cause.choice.transport;
      break;
    case F1AP_Cause_PR_protocol:
      required->cause = F1AP_CAUSE_PROTOCOL;
      required->cause_value = ie->value.choice.Cause.choice.protocol;
      break;
    case F1AP_Cause_PR_misc:
      required->cause = F1AP_CAUSE_MISC;
      required->cause_value = ie->value.choice.Cause.choice.misc;
      break;
    default:
      LOG_W(F1AP, "Unknown cause for UE Context Modification required message\n");
      /* fall through */
    case F1AP_Cause_PR_NOTHING:
      required->cause = F1AP_CAUSE_NOTHING;
      break;
  }

  /* optional: BH RLC Channel Required to be Released List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_BHChannels_Required_ToBeReleased_List,
                             false);
  AssertFatal(ie == NULL, "handling of BH RLC Channel Required to be Released list not implemented\n");

  /* optional: SL DRB Required to Be Modified List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_SLDRBs_Required_ToBeModified_List,
                             false);
  AssertFatal(ie == NULL, "handling of SL DRB Required to be modified list not implemented\n");

  /* optional: SL DRB Required to be Released List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_SLDRBs_Required_ToBeReleased_List,
                             false);
  AssertFatal(ie == NULL, "handling of SL DRBs Required to be released list not implemented\n");

  /* optional: Candidate Cells To Be Cancelled List */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_UEContextModificationRequiredIEs_t,
                             ie,
                             container,
                             F1AP_ProtocolIE_ID_id_Candidate_SpCell_List,
                             false);
  AssertFatal(ie == NULL, "handling of candidate cells to be cancelled list not implemented\n");

  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg_p);
  return 0;
}

int CU_send_UE_CONTEXT_MODIFICATION_CONFIRM(sctp_assoc_t assoc_id, f1ap_ue_context_modif_confirm_t *confirm)
{
  F1AP_F1AP_PDU_t pdu = {0};
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextModificationRequired;
  tmp->criticality = F1AP_Criticality_reject;
  tmp->value.present = F1AP_SuccessfulOutcome__value_PR_UEContextModificationConfirm;
  F1AP_UEContextModificationConfirm_t *out = &tmp->value.choice.UEContextModificationConfirm;

  /* mandatory: GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationConfirmIEs_t, ie1);
  ie1->id = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality = F1AP_Criticality_reject;
  ie1->value.present = F1AP_UEContextModificationConfirmIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = confirm->gNB_CU_ue_id;

  /* mandatory: GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationConfirmIEs_t, ie2);
  ie2->id = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality = F1AP_Criticality_reject;
  ie2->value.present = F1AP_UEContextModificationConfirmIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = confirm->gNB_DU_ue_id;

  /* optional: Resource Coordination Transfer Container */
  /* not implemented*/

  /* optional: DRB Modified List */
  /* not implemented*/

  /* optional: RRC Container */
  if (confirm->rrc_container != NULL) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationConfirmIEs_t, ie);
    ie->id = F1AP_ProtocolIE_ID_id_RRCContainer;
    ie->criticality = F1AP_Criticality_ignore;
    ie->value.present = F1AP_UEContextModificationConfirmIEs__value_PR_RRCContainer;
    OCTET_STRING_fromBuf(&ie->value.choice.RRCContainer, (const char *)confirm->rrc_container, confirm->rrc_container_length);
  }

  /* optional: CriticalityDiagnostics */
  /* not implemented*/

  /* optional: Execute Duplication */
  /* not implemented*/

  /* optional: Resource Coordination Transfer Information */
  /* not implemented*/

  /* optional: SL DRB Modified List */
  /* not implemented*/

  /* encode */
  uint8_t *buffer = NULL;
  uint32_t len = 0;
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE Context Modification Confirm\n");
    return -1;
  }
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int CU_send_UE_CONTEXT_MODIFICATION_REFUSE(sctp_assoc_t assoc_id, f1ap_ue_context_modif_refuse_t *refuse)
{
  F1AP_F1AP_PDU_t pdu = {0};
  pdu.present = F1AP_F1AP_PDU_PR_unsuccessfulOutcome;
  asn1cCalloc(pdu.choice.unsuccessfulOutcome, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_UEContextModificationRequired;
  tmp->criticality = F1AP_Criticality_reject;
  tmp->value.present = F1AP_UnsuccessfulOutcome__value_PR_UEContextModificationRefuse;
  F1AP_UEContextModificationRefuse_t *out = &tmp->value.choice.UEContextModificationRefuse;

  /* mandatory: GNB_CU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRefuseIEs_t, ie1);
  ie1->id = F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID;
  ie1->criticality = F1AP_Criticality_reject;
  ie1->value.present = F1AP_UEContextModificationRefuseIEs__value_PR_GNB_CU_UE_F1AP_ID;
  ie1->value.choice.GNB_CU_UE_F1AP_ID = refuse->gNB_CU_ue_id;

  /* mandatory: GNB_DU_UE_F1AP_ID */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRefuseIEs_t, ie2);
  ie2->id = F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID;
  ie2->criticality = F1AP_Criticality_reject;
  ie2->value.present = F1AP_UEContextModificationRefuseIEs__value_PR_GNB_DU_UE_F1AP_ID;
  ie2->value.choice.GNB_DU_UE_F1AP_ID = refuse->gNB_DU_ue_id;

  /* optional: Cause */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_UEContextModificationRefuseIEs_t, ie3);
  ie3->id = F1AP_ProtocolIE_ID_id_Cause;
  ie3->criticality = F1AP_Criticality_reject;
  ie3->value.present = F1AP_UEContextModificationRefuseIEs__value_PR_Cause;
  F1AP_Cause_t *cause = &ie3->value.choice.Cause;
  switch (refuse->cause) {
    case F1AP_CAUSE_RADIO_NETWORK:
      cause->present = F1AP_Cause_PR_radioNetwork;
      cause->choice.radioNetwork = refuse->cause_value;
      break;
    case F1AP_CAUSE_TRANSPORT:
      cause->present = F1AP_Cause_PR_transport;
      cause->choice.transport = refuse->cause_value;
      break;
    case F1AP_CAUSE_PROTOCOL:
      cause->present = F1AP_Cause_PR_protocol;
      cause->choice.protocol = refuse->cause_value;
      break;
    case F1AP_CAUSE_MISC:
      cause->present = F1AP_Cause_PR_misc;
      cause->choice.misc = refuse->cause_value;
      break;
    case F1AP_CAUSE_NOTHING:
    default:
      cause->present = F1AP_Cause_PR_NOTHING;
      break;
  } // switch

  /* optional: CriticalityDiagnostics */
  /* not implemented*/

  /* encode */
  uint8_t *buffer = NULL;
  uint32_t len = 0;
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 UE Context Modification Refuse\n");
    return -1;
  }
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}
