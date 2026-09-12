/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "e1ap.h"
#include "e1ap_common.h"
#include "gnb_config.h"
#include "openair2/SDAP/nr_sdap/nr_sdap_entity.h"
#include "openair3/UTILS/conversions.h"
#include "openair2/RRC/NR/MESSAGES/asn1_msg.h"
#include "common/openairinterface5g_limits.h"
#include "common/utils/LOG/log.h"
#include "common/utils/utils.h"
#include "openair2/F1AP/f1ap_common.h"
#include "e1ap_default_values.h"
#include "gtp_itf.h"
#include "openair2/LAYER2/nr_pdcp/cucp_cuup_handler.h"
#include "lib/e1ap_bearer_context_management.h"
#include "lib/e1ap_interface_management.h"

#define E1AP_NUM_MSG_HANDLERS 14
typedef int (*e1ap_message_processing_t)(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *message_p);

/**
 * @brief E1AP messages handlers
 */
const e1ap_message_processing_t e1ap_message_processing[E1AP_NUM_MSG_HANDLERS][3] = {

    {0, 0, 0}, /* Reset */
    {0, 0, 0}, /* ErrorIndication */
    {0, 0, 0}, /* privateMessage */
    {e1apCUCP_handle_SETUP_REQUEST, e1apCUUP_handle_SETUP_RESPONSE, e1apCUUP_handle_SETUP_FAILURE}, /* gNBCUUPE1Setup */
    {0, 0, 0}, /* gNBCUCPE1Setup */
    {0, 0, 0}, /* gNBCUUPConfigurationUpdate */
    {0, 0, 0}, /* gNBCUCPConfigurationUpdate */
    {0, 0, 0}, /* E1Release */
    {e1apCUUP_handle_BEARER_CONTEXT_SETUP_REQUEST,
     e1apCUCP_handle_BEARER_CONTEXT_SETUP_RESPONSE,
     e1apCUCP_handle_BEARER_CONTEXT_SETUP_FAILURE}, /* bearerContextSetup */
    {e1apCUUP_handle_BEARER_CONTEXT_MODIFICATION_REQUEST,
     e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_RESPONSE,
     e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_FAILURE}, /* bearerContextModification */
    {e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_REQUIRED,
     e1apCUUP_handle_BEARER_CONTEXT_MODIFICATION_CONFIRM,
     0}, /* bearerContextModificationRequired */
    {e1apCUUP_handle_BEARER_CONTEXT_RELEASE_COMMAND, e1apCUCP_handle_BEARER_CONTEXT_RELEASE_COMPLETE, 0}, /* bearerContextRelease */
    {0, 0, 0}, /* bearerContextReleaseRequired */
    {0, 0, 0} /* bearerContextInactivityNotification */
};

const char *const e1ap_direction2String(int e1ap_dir)
{
  static const char *e1ap_direction_String[] = {
      "", /* Nothing */
      "Initiating message", /* initiating message */
      "Successfull outcome", /* successfull outcome */
      "UnSuccessfull outcome", /* successfull outcome */
  };
  return (e1ap_direction_String[e1ap_dir]);
}

static int e1ap_handle_message(instance_t instance, sctp_assoc_t assoc_id, const uint8_t *const data, const uint32_t data_length)
{
  E1AP_E1AP_PDU_t pdu = {0};
  int ret;
  DevAssert(data != NULL);

  if (e1ap_decode_pdu(&pdu, data, data_length) < 0) {
    LOG_E(E1AP, "Failed to decode PDU\n");
    return -1;
  }
  const E1AP_ProcedureCode_t procedureCode = pdu.choice.initiatingMessage->procedureCode;
  /* Checking procedure Code and direction of message */
  if ((procedureCode >= E1AP_NUM_MSG_HANDLERS) || (pdu.present > E1AP_E1AP_PDU_PR_unsuccessfulOutcome)
      || (pdu.present <= E1AP_E1AP_PDU_PR_NOTHING)) {
    LOG_E(E1AP, "[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n", assoc_id, procedureCode, pdu.present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_E1AP_E1AP_PDU, &pdu);
    return -1;
  }

  if (e1ap_message_processing[procedureCode][pdu.present - 1] == NULL) {
    // No handler present. This can mean not implemented or no procedure for eNB
    // (wrong direction).
    LOG_E(E1AP,
          "[SCTP %d] No handler for procedureCode %ld in %s\n",
          assoc_id,
          procedureCode,
          e1ap_direction2String(pdu.present - 1));
    ret = -1;
  } else {
    /* Calling the right handler */
    LOG_D(E1AP, "Calling handler with instance %ld\n", instance);
    ret = (*e1ap_message_processing[procedureCode][pdu.present - 1])(assoc_id, getCxtE1(instance), &pdu);
  }

  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_E1AP_E1AP_PDU, &pdu);
  return ret;
}

static void e1_task_handle_sctp_data_ind(instance_t instance, sctp_data_ind_t *sctp_data_ind)
{
  int result;
  DevAssert(sctp_data_ind != NULL);
  e1ap_handle_message(instance, sctp_data_ind->assoc_id, sctp_data_ind->buffer, sctp_data_ind->buffer_length);
  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
}

void e1ap_send_SETUP_RESPONSE(sctp_assoc_t assoc_id, const e1ap_setup_resp_t *e1ap_setup_resp)
{
  E1AP_E1AP_PDU_t *pdu = encode_e1ap_cuup_setup_response(e1ap_setup_resp);
  e1ap_encode_send(CPtype, assoc_id, pdu, 0, __func__);
}

/**
 * @brief E1 Setup Failure ASN1 messager encoder
 */
static void e1apCUCP_send_SETUP_FAILURE(sctp_assoc_t assoc_id, const e1ap_setup_fail_t *msg)
{
  LOG_D(E1AP, "CU-CP: Encoding E1AP Setup Failure for transac_id %ld...\n", msg->transac_id);
  E1AP_E1AP_PDU_t *pdu = encode_e1ap_cuup_setup_failure(msg);
  e1ap_encode_send(CPtype, assoc_id, pdu, 0, __func__);
}

int e1apCUCP_handle_SETUP_REQUEST(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);
  UNUSED(inst);
  /* Create ITTI message and send to queue */
  MessageDef *msg_p = itti_alloc_new_message(TASK_CUCP_E1, 0 /*unused by callee*/, E1AP_SETUP_REQ);
  // Decode E1 CU-UP Setup Request
  if (!decode_e1ap_cuup_setup_request(pdu, &E1AP_SETUP_REQ(msg_p))) {
    free_e1ap_cuup_setup_request(&E1AP_SETUP_REQ(msg_p));
    return -1;
  }

  msg_p->ittiMsgHeader.originInstance = assoc_id;

  if (E1AP_SETUP_REQ(msg_p).supported_plmns > 0) {
    itti_send_msg_to_task(TASK_RRC_GNB, 0 /*unused by callee*/, msg_p);
  } else {
    e1ap_cause_t cause = { .type = E1AP_CAUSE_PROTOCOL, .value = E1AP_PROTOCOL_CAUSE_SEMANTIC_ERROR};
    e1ap_setup_fail_t setup_fail = {.transac_id = E1AP_SETUP_REQ(msg_p).transac_id, .cause = cause};
    e1apCUCP_send_SETUP_FAILURE(assoc_id, &setup_fail);
    itti_free(TASK_CUCP_E1, msg_p);
    return -1;
  }

  return 0;
}

int e1apCUUP_handle_SETUP_RESPONSE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(assoc_id);
  // Decode E1 CU-UP Setup Response
  e1ap_setup_resp_t out = {0};
  if (!decode_e1ap_cuup_setup_response(pdu, &out)) {
    free_e1ap_cuup_setup_response(&out);
    return -1;
  }
  long old_transaction_id = inst->cuup.setupReq.transac_id;
  if (old_transaction_id != out.transac_id) {
    LOG_E(E1AP, "Transaction IDs do not match %ld != %ld\n", old_transaction_id, out.transac_id);
    free_e1ap_cuup_setup_response(&out);
    return -1;
  }
  free_e1ap_cuup_setup_response(&out);

  e1_reset(); // reset all UE contexts, if any: see 38.463 sec 8.2.3
  return 0;
}

/**
 * @brief E1 Setup Failure ASN1 messager decoder on CU-UP
 */
int e1apCUUP_handle_SETUP_FAILURE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(assoc_id);
  LOG_D(E1AP, "CU-UP: Decoding E1AP Setup Failure...\n");
  E1AP_GNB_CU_UP_E1SetupFailureIEs_t *ie;
  DevAssert(pdu != NULL);
  E1AP_GNB_CU_UP_E1SetupFailure_t *in = &pdu->choice.unsuccessfulOutcome->value.choice.GNB_CU_UP_E1SetupFailure;
  /* Transaction ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(E1AP_GNB_CU_UP_E1SetupFailureIEs_t, ie, in, E1AP_ProtocolIE_ID_id_TransactionID, true);
  long transaction_id;
  long old_transaction_id = inst->cuup.setupReq.transac_id;
  transaction_id = ie->value.choice.TransactionID;
  if (old_transaction_id != transaction_id)
    LOG_E(E1AP, "Transaction IDs do not match %ld != %ld\n", old_transaction_id, transaction_id);
  E1AP_free_transaction_identifier(transaction_id);

  /* Cause */
  F1AP_FIND_PROTOCOLIE_BY_ID(E1AP_GNB_CU_UP_E1SetupFailureIEs_t, ie, in, E1AP_ProtocolIE_ID_id_Cause, true);

  LOG_E(E1AP, "received E1 Setup Failure, please check the CU-CP output and the CU-UP parameters\n");
  exit(1);
  return 0;
}

static void fill_CONFIGURATION_UPDATE(E1AP_E1AP_PDU_t *pdu)
{
  /* Create */
  /* 0. pdu Type */
  pdu->present = E1AP_E1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu->choice.initiatingMessage, msg);
  msg->procedureCode = E1AP_ProcedureCode_id_gNB_CU_UP_ConfigurationUpdate;
  msg->criticality   = E1AP_Criticality_reject;
  msg->value.present = E1AP_InitiatingMessage__value_PR_GNB_CU_UP_ConfigurationUpdate;
  E1AP_GNB_CU_UP_ConfigurationUpdate_t *out = &pdu->choice.initiatingMessage->value.choice.GNB_CU_UP_ConfigurationUpdate;
  /* mandatory */
  /* c1. Transaction ID (integer value) */
  asn1cSequenceAdd(out->protocolIEs.list, E1AP_GNB_CU_UP_ConfigurationUpdateIEs_t, ieC1);
  ieC1->id                         = E1AP_ProtocolIE_ID_id_TransactionID;
  ieC1->criticality                = E1AP_Criticality_reject;
  ieC1->value.present              = E1AP_GNB_CU_UP_ConfigurationUpdateIEs__value_PR_TransactionID;
  ieC1->value.choice.TransactionID = 0;//get this from stored transaction IDs in CU
  /* mandatory */
  /* c2. Supported PLMNs */
  asn1cSequenceAdd(out->protocolIEs.list, E1AP_GNB_CU_UP_ConfigurationUpdateIEs_t, ieC2);
  ieC2->id = E1AP_ProtocolIE_ID_id_SupportedPLMNs;
  ieC2->criticality = E1AP_Criticality_reject;
  ieC2->value.present = E1AP_GNB_CU_UP_ConfigurationUpdateIEs__value_PR_SupportedPLMNs_List;

  int numSupportedPLMNs = 1;

  for (int i = 0; i < numSupportedPLMNs; i++) {
    asn1cSequenceAdd(ieC2->value.choice.SupportedPLMNs_List.list, E1AP_SupportedPLMNs_Item_t, supportedPLMN);
    /* 5.1 PLMN Identity */
    OCTET_STRING_fromBuf(&supportedPLMN->pLMN_Identity, "OAI", strlen("OAI"));
  }

  /* mandatory */
  /* c3. TNLA to remove list */
  asn1cSequenceAdd(out->protocolIEs.list, E1AP_GNB_CU_UP_ConfigurationUpdateIEs_t, ieC3);
  ieC3->id = E1AP_ProtocolIE_ID_id_GNB_CU_UP_TNLA_To_Remove_List;
  ieC3->criticality = E1AP_Criticality_reject;
  ieC3->value.present = E1AP_GNB_CU_UP_ConfigurationUpdateIEs__value_PR_GNB_CU_UP_TNLA_To_Remove_List;

  int numTNLAtoRemoveList = 1;

  for (int i = 0; i < numTNLAtoRemoveList; i++) {
    asn1cSequenceAdd(ieC2->value.choice.GNB_CU_UP_TNLA_To_Remove_List.list, E1AP_GNB_CU_UP_TNLA_To_Remove_Item_t, TNLAtoRemove);
    TNLAtoRemove->tNLAssociationTransportLayerAddress.present = E1AP_CP_TNL_Information_PR_endpoint_IP_Address;
    TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(1234, &TNLAtoRemove->tNLAssociationTransportLayerAddress.choice.endpoint_IP_Address); // TODO: correct me
  }
}

/*
  E1 configuration update: can be sent in both ways, to be refined
*/

void e1apCUUP_send_CONFIGURATION_UPDATE(sctp_assoc_t assoc_id)
{
  E1AP_E1AP_PDU_t *pdu = calloc_or_fail(1, sizeof(*pdu));
  fill_CONFIGURATION_UPDATE(pdu);
  e1ap_encode_send(UPtype, assoc_id, pdu, 0, __func__);
}

int e1apCUCP_send_gNB_DU_CONFIGURATION_FAILURE(void)
{
  AssertFatal(false, "Not implemented yet\n");
  return -1;
}

int e1apCUCP_send_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(void)
{
  AssertFatal(false, "Not implemented yet\n");
  return -1;
}

void e1apCUCP_send_BEARER_CONTEXT_SETUP_REQUEST(sctp_assoc_t assoc_id, e1ap_bearer_setup_req_t *const bearerCxt)
{
  E1AP_E1AP_PDU_t *pdu = encode_E1_bearer_context_setup_request(bearerCxt);
  e1ap_encode_send(CPtype, assoc_id, pdu, 0, __func__);
}

void e1apCUUP_send_BEARER_CONTEXT_SETUP_RESPONSE(sctp_assoc_t assoc_id, const e1ap_bearer_setup_resp_t *resp)
{
  // Encode
  E1AP_E1AP_PDU_t *pdu = encode_E1_bearer_context_setup_response(resp);
  e1ap_encode_send(UPtype, assoc_id, pdu, 0, __func__);
}

static void e1apCUUP_send_BEARER_CONTEXT_SETUP_FAILURE(sctp_assoc_t assoc_id, const e1ap_bearer_context_setup_failure_t *msg)
{
  E1AP_E1AP_PDU_t *pdu = encode_E1_bearer_context_setup_failure(msg);
  e1ap_encode_send(UPtype, assoc_id, pdu, 0, __func__);
}

int e1apCUUP_handle_BEARER_CONTEXT_SETUP_REQUEST(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *e1_inst, const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(assoc_id);
  UNUSED(e1_inst);
  // Decode E1AP message
  e1ap_bearer_setup_req_t bearerCxt = {0};
  if (!decode_E1_bearer_context_setup_request(pdu, &bearerCxt)) {
    LOG_E(E1AP, "Failed to decode E1 Bearer Context Setup Request\n");
    return -1;
  }
  e1_bearer_context_setup(&bearerCxt);
  free_e1ap_context_setup_request(&bearerCxt);
  return 0;
}

int e1apCUCP_handle_BEARER_CONTEXT_SETUP_RESPONSE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(inst);
  MessageDef *msg = itti_alloc_new_message(TASK_CUCP_E1, 0, E1AP_BEARER_CONTEXT_SETUP_RESP);
  // Decode
  e1ap_bearer_setup_resp_t *bearerCxt = &E1AP_BEARER_CONTEXT_SETUP_RESP(msg);
  if (!decode_E1_bearer_context_setup_response(pdu, bearerCxt)) {
    LOG_E(E1AP, "Failed to decode Bearer Context Setup Response\n");
    return -1;
  }
  msg->ittiMsgHeader.originInstance = assoc_id;
  // Fixme: instance is the NGAP instance, no good way to set it here
  instance_t instance = 0;
  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg);

  return 0;
}

int e1apCUCP_handle_BEARER_CONTEXT_SETUP_FAILURE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(inst);
  e1ap_bearer_context_setup_failure_t fail = {0};
  if (!decode_E1_bearer_context_setup_failure(&fail, pdu)) {
    LOG_E(E1AP, "Failed to decode Bearer Context Setup Failure\n");
    free_e1_bearer_context_setup_failure(&fail);
    return -1;
  }
  LOG_E(E1AP, "Received Bearer Context Setup Failure with cause value%d \n", fail.cause.value);

  MessageDef *msg = itti_alloc_new_message(TASK_CUCP_E1, 0, E1AP_BEARER_CONTEXT_SETUP_FAILURE);
  E1AP_BEARER_CONTEXT_SETUP_FAILURE(msg) = fail;
  msg->ittiMsgHeader.originInstance = assoc_id;
  instance_t instance = 0;
  itti_send_msg_to_task(TASK_RRC_GNB, instance, msg);
  return 0;
}

static void e1apCUCP_send_BEARER_CONTEXT_MODIFICATION_REQUEST(sctp_assoc_t assoc_id, e1ap_bearer_mod_req_t *const bearerCxt)
{
  E1AP_E1AP_PDU_t *pdu = encode_E1_bearer_context_mod_request(bearerCxt);
  e1ap_encode_send(CPtype, assoc_id, pdu, 0, __func__);
}

static int e1apCUUP_send_BEARER_CONTEXT_MODIFICATION_RESPONSE(sctp_assoc_t assoc_id, const e1ap_bearer_modif_resp_t *resp)
{
  E1AP_E1AP_PDU_t *pdu = encode_E1_bearer_context_mod_response(resp);
  return e1ap_encode_send(UPtype, assoc_id, pdu, 0, __func__);
}

static int e1apCUUP_send_BEARER_CONTEXT_MODIFICATION_FAILURE(sctp_assoc_t assoc_id, const e1ap_bearer_context_mod_failure_t *out)
{
  E1AP_E1AP_PDU_t *pdu = encode_E1_bearer_context_mod_failure(out);
  e1ap_encode_send(UPtype, assoc_id, pdu, 0, __func__);
  return 0;
}

/**
 * @brief Extract Bearer Context Modification Request from E1AP PDU
 *        and prepare Bearer Context Modification Response
 */
int e1apCUUP_handle_BEARER_CONTEXT_MODIFICATION_REQUEST(sctp_assoc_t assoc_id,
                                                        e1ap_upcp_inst_t *e1_inst,
                                                        const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(assoc_id);
  UNUSED(e1_inst);
  e1ap_bearer_mod_req_t bearerCxt = {0};
  if (!decode_E1_bearer_context_mod_request(pdu, &bearerCxt)) {
    LOG_E(E1AP, "Failed to handle Bearer Context Modification Request\n");
    free_e1ap_context_mod_request(&bearerCxt);
    return -1;
  }
  e1_bearer_context_modif(&bearerCxt);
  free_e1ap_context_mod_request(&bearerCxt);
  return 0;
}

int e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_RESPONSE(sctp_assoc_t assoc_id,
                                                         e1ap_upcp_inst_t *e1_inst,
                                                         const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(assoc_id);
  UNUSED(e1_inst);
  MessageDef *msg = itti_alloc_new_message(TASK_CUUP_E1, 0, E1AP_BEARER_CONTEXT_MODIFICATION_RESP);
  e1ap_bearer_modif_resp_t *modif = &E1AP_BEARER_CONTEXT_MODIFICATION_RESP(msg);
  if(!decode_E1_bearer_context_mod_response(modif, pdu)) {
    free_e1ap_context_mod_response(modif);
    return -1;
  }
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
  return 0;
}

int e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_FAILURE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(assoc_id);
  UNUSED(inst);
  e1ap_bearer_context_mod_failure_t failure = {0};
  if (!decode_E1_bearer_context_mod_failure(&failure, pdu)) {
    free_E1_bearer_context_mod_failure(&failure);
    return -1;
  }
  LOG_E(E1AP,
        "Received Bearer Context Modification Failure from CU-UP %d with cause value %d \n",
        failure.gNB_cu_up_ue_id,
        failure.cause.value);
  MessageDef *msg = itti_alloc_new_message(TASK_CUUP_E1, 0, E1AP_BEARER_CONTEXT_MODIFICATION_FAIL);
  E1AP_BEARER_CONTEXT_MODIFICATION_FAIL(msg) = cp_E1_bearer_context_mod_failure(&failure);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
  return 0;
}

static int e1apCUUP_send_BEARER_CONTEXT_MODIFICATION_REQUIRED(sctp_assoc_t assoc_id, const e1ap_bearer_mod_required_t *req)
{
  E1AP_E1AP_PDU_t *pdu = encode_E1_bearer_context_mod_required(req);
  return e1ap_encode_send(UPtype, assoc_id, pdu, 0, __func__);
}

int e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_REQUIRED(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(inst);
  e1ap_bearer_mod_required_t required = {0};
  if (!decode_E1_bearer_context_mod_required(pdu, &required)) {
    free_e1ap_context_mod_required(&required);
    return -1;
  }
  MessageDef *msg = itti_alloc_new_message(TASK_CUUP_E1, 0, E1AP_BEARER_CONTEXT_MODIFICATION_REQUIRED);
  msg->ittiMsgHeader.originInstance = assoc_id;
  E1AP_BEARER_CONTEXT_MODIFICATION_REQUIRED(msg) = cp_bearer_context_mod_required(&required);
  free_e1ap_context_mod_required(&required);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
  return 0;
}

static int e1apCUCP_send_BEARER_CONTEXT_MODIFICATION_CONFIRM(sctp_assoc_t assoc_id, const e1ap_bearer_mod_confirm_t *conf)
{
  E1AP_E1AP_PDU_t *pdu = encode_E1_bearer_context_mod_confirm(conf);
  return e1ap_encode_send(CPtype, assoc_id, pdu, 0, __func__);
}

int e1apCUUP_handle_BEARER_CONTEXT_MODIFICATION_CONFIRM(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(assoc_id);
  UNUSED(inst);
  e1ap_bearer_mod_confirm_t conf = {0};
  if (!decode_E1_bearer_context_mod_confirm(&conf, pdu)) {
    free_e1ap_context_mod_confirm(&conf);
    return -1;
  }
  e1_bearer_context_mod_confirm(&conf);
  free_e1ap_context_mod_confirm(&conf);
  return 0;
}

/*
  BEARER CONTEXT RELEASE
*/

static int e1apCUCP_send_BEARER_CONTEXT_RELEASE_COMMAND(sctp_assoc_t assoc_id, e1ap_bearer_release_cmd_t *const cmd)
{
  E1AP_E1AP_PDU_t *pdu = encode_e1_bearer_context_release_command(cmd);
  return e1ap_encode_send(CPtype, assoc_id, pdu, 0, __func__);
}

int e1apCUUP_send_BEARER_CONTEXT_RELEASE_COMPLETE(sctp_assoc_t assoc_id, const e1ap_bearer_release_cplt_t *cplt)
{
  E1AP_E1AP_PDU_t *pdu = encode_e1_bearer_context_release_complete(cplt);
  return e1ap_encode_send(CPtype, assoc_id, pdu, 0, __func__);
}

int e1apCUUP_handle_BEARER_CONTEXT_RELEASE_COMMAND(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(assoc_id);
  UNUSED(inst);
  e1ap_bearer_release_cmd_t release = {0};
  if(!decode_e1_bearer_context_release_command(&release, pdu)) {
    free_e1_bearer_context_release_command(&release);
    return -1;
  }
  e1_bearer_release_cmd(&release);
  return 0;
}

int e1apCUCP_handle_BEARER_CONTEXT_RELEASE_COMPLETE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *e1_inst, const E1AP_E1AP_PDU_t *pdu)
{
  UNUSED(assoc_id);
  UNUSED(e1_inst);
  e1ap_bearer_release_cplt_t bearerCxt = {0};
  if(!decode_e1_bearer_context_release_complete(&bearerCxt, pdu)) {
    free_e1_bearer_context_release_complete(&bearerCxt);
    return -1;
  }
  MessageDef *msg = itti_alloc_new_message(TASK_CUUP_E1, 0, E1AP_BEARER_CONTEXT_RELEASE_CPLT);
  e1ap_bearer_release_cplt_t *cplt = &E1AP_BEARER_CONTEXT_RELEASE_CPLT(msg);
  *cplt = cp_bearer_context_release_complete(&bearerCxt);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
  return 0;
}

static instance_t cuup_task_create_gtpu_instance_to_du(const e1ap_net_config_t *c)
{
  openAddr_t tmp = {0};
  strncpy(tmp.originHost, c->localAddressF1U, sizeof(tmp.originHost) - 1);
  sprintf(tmp.originService, "%d", c->localPortF1U);
  sprintf(tmp.destinationService, "%d", c->remotePortF1U);
  return gtpv1Init(tmp);
}

static void e1_task_send_sctp_association_req(long task_id, instance_t instance, e1ap_net_config_t *e1ap_nc)
{
  DevAssert(e1ap_nc != NULL);
  getCxtE1(instance)->sockState = SCTP_STATE_CLOSED;
  MessageDef *message_p = itti_alloc_new_message(task_id, 0, SCTP_NEW_ASSOCIATION_REQ);
  sctp_new_association_req_t *sctp_new_req = &message_p->ittiMsg.sctp_new_association_req;
  sctp_new_req->ulp_cnx_id = instance;
  sctp_new_req->port = E1AP_PORT_NUMBER;
  sctp_new_req->ppid = E1AP_SCTP_PPID;
  // remote
  sctp_new_req->remote_address = e1ap_nc->CUCP_e1_ip_address;
  // local
  sctp_new_req->local_address = e1ap_nc->CUUP_e1_ip_address;
  itti_send_msg_to_task(TASK_SCTP, instance, message_p);
}

static void e1apCUUP_send_SETUP_REQUEST(sctp_assoc_t assoc_id, e1ap_setup_req_t *setup)
{
  setup->transac_id = E1AP_get_next_transaction_identifier();
  LOG_D(E1AP, "Transaction ID of setup request %ld\n", setup->transac_id);
  E1AP_E1AP_PDU_t *pdu = encode_e1ap_cuup_setup_request(setup);
  e1ap_encode_send(UPtype, assoc_id, pdu, 0, __func__);
}

/**
 * @brief SCTP association response handler (CU-CP to/from CU-UP)
 * it behaves differently depending on the type of E1 instance,
 * CUCP: informs RRC of E1 connection loss with CU-UP
 * CUUP: triggers a new SCTP association request by sending an ITTI to the CU-UP task
 * @param type indicates whether the handler is for the CU-CP or CU-UP
 */
static void e1_task_handle_sctp_association_resp(E1_t type,
                                                 instance_t instance,
                                                 sctp_new_association_resp_t *sctp_new_association_resp)
{
  DevAssert(sctp_new_association_resp != NULL);
  getCxtE1(instance)->sockState = sctp_new_association_resp->sctp_state;

  // Handle SCTP establishment failure
  if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
    if (type == CPtype) {
      // Inform RRC that the CU-UP is gone
      LOG_W(E1AP,
            "Lost connection (%s) with CU-UP (assoc_id %d)\n",
            sctp_state_s[sctp_new_association_resp->sctp_state],
            sctp_new_association_resp->assoc_id);
      MessageDef *message_p = itti_alloc_new_message(TASK_CUCP_E1, 0, E1AP_LOST_CONNECTION);
      message_p->ittiMsgHeader.originInstance = sctp_new_association_resp->assoc_id;
      itti_send_msg_to_task(TASK_RRC_GNB, instance, message_p);
    } else if (type == UPtype) {
      // Trigger new E1 association when no CU-CP is connected
      LOG_W(E1AP,
            "Lost connection (%s) with CU-CP (assoc_id %d): trigger new E1 association\n",
            sctp_state_s[sctp_new_association_resp->sctp_state],
            sctp_new_association_resp->assoc_id);
      long timer_id;
      timer_setup(1, 0, TASK_CUUP_E1, 0, TIMER_ONE_SHOT, NULL, &timer_id);
    }
    return;
  }

  if (type == UPtype) {
    e1ap_upcp_inst_t *inst = getCxtE1(instance);
    inst->cuup.assoc_id = sctp_new_association_resp->assoc_id;

    if (getCxtE1(instance)->gtpInstF1U < 0)
      getCxtE1(instance)->gtpInstF1U = cuup_task_create_gtpu_instance_to_du(&inst->net_config);
    if (getCxtE1(instance)->gtpInstF1U < 0)
      LOG_E(E1AP, "Failed to create CUUP F1-U UDP listener\n");
    cuup_init_n3(instance);
    e1apCUUP_send_SETUP_REQUEST(inst->cuup.assoc_id, &inst->cuup.setupReq);
  }
}

void cuup_init_n3(instance_t instance)
{
  if (getCxtE1(instance)->gtpInstN3 < 0) {
    e1ap_net_config_t *nc = &getCxtE1(instance)->net_config;
    openAddr_t tmp = {0};
    strcpy(tmp.originHost, nc->localAddressN3);
    sprintf(tmp.originService, "%d", nc->localPortN3);
    sprintf(tmp.destinationService, "%d", nc->remotePortN3);
    LOG_I(GTPU, "Configuring GTPu address : %s, port : %s\n", tmp.originHost, tmp.originService);
    // Fixme: fully inconsistent instances management
    // dirty global var is a bad fix
    extern instance_t legacyInstanceMapping;
    legacyInstanceMapping = getCxtE1(instance)->gtpInstN3 = gtpv1Init(tmp);
  }
  if (getCxtE1(instance)->gtpInstN3 < 0)
    LOG_E(E1AP, "Failed to create CUUP N3 UDP listener\n");
  extern instance_t *N3GTPUInst;
  N3GTPUInst = &getCxtE1(instance)->gtpInstN3;
}

void cucp_task_send_sctp_init_req(instance_t instance, char *my_addr)
{
  size_t addr_len = strlen(my_addr) + 1;
  LOG_I(E1AP, "E1AP_CUCP_SCTP_REQ(create socket) for %s len %ld\n", my_addr, addr_len);
  MessageDef *message_p = itti_alloc_new_message_sized(TASK_CUCP_E1, 0, SCTP_INIT_MSG, sizeof(sctp_init_t) + addr_len);
  sctp_init_t *init = &SCTP_INIT_MSG(message_p);
  init->port = E1AP_PORT_NUMBER;
  init->ppid = E1AP_SCTP_PPID;
  char *addr_buf = (char *) (init + 1); // address after sctp_init ITTI message end, allocated above
  init->bind_address = addr_buf;
  memcpy(addr_buf, my_addr, addr_len);
  itti_send_msg_to_task(TASK_SCTP, instance, message_p);
}

void e1_task_handle_sctp_association_ind(E1_t type, instance_t instance, sctp_new_association_ind_t *sctp_new_ind)
{
  if (!getCxtE1(instance))
    createE1inst(type, instance, 0, NULL, NULL);
  e1ap_upcp_inst_t *inst = getCxtE1(instance);
  inst->sockState = SCTP_STATE_ESTABLISHED;
  if (type == UPtype)
    inst->cuup.assoc_id = sctp_new_ind->assoc_id;
}

/**
 * @brief Handle the timer triggered by a connection loss with CU-CP
 *        it uses a stored network configuration and send a new SCTP association request
 */
static void e1apHandleTimer(instance_t myInstance)
{
  LOG_W(E1AP, "Try to reconnect to CP\n");
  if (getCxtE1(myInstance)->sockState != SCTP_STATE_ESTABLISHED)
    e1_task_send_sctp_association_req(TASK_CUUP_E1, myInstance, &getCxtE1(myInstance)->net_config);
}

/**
 * @brief E1AP Centralized Unit Control Plane (CU-CP) task function.
 *
 * This is the main task loop for the E1AP CUCP module:
 * it listens for incoming messages from the Inter-Task Interface (ITTI)
 * and calls the relevant handlers for each message type.
 *
 */
void *E1AP_CUCP_task(void *arg)
{
  UNUSED(arg);
  LOG_I(E1AP, "Starting E1AP at CU CP\n");
  MessageDef *msg = NULL;
  e1ap_common_init();
  int result;

  while (1) {
    itti_receive_msg(TASK_CUCP_E1, &msg);
    instance_t myInstance = ITTI_MSG_DESTINATION_INSTANCE(msg);
    const int msgType = ITTI_MSG_ID(msg);
    LOG_D(E1AP, "CUCP received %s for instance %ld\n", messages_info[msgType].name, myInstance);
    sctp_assoc_t assoc_id = msg->ittiMsgHeader.originInstance;

    switch (ITTI_MSG_ID(msg)) {
      case SCTP_NEW_ASSOCIATION_IND:
        e1_task_handle_sctp_association_ind(CPtype, ITTI_MSG_ORIGIN_INSTANCE(msg), &msg->ittiMsg.sctp_new_association_ind);
        break;

      case SCTP_NEW_ASSOCIATION_RESP:
        e1_task_handle_sctp_association_resp(CPtype, ITTI_MSG_ORIGIN_INSTANCE(msg), &msg->ittiMsg.sctp_new_association_resp);
        break;

      case E1AP_REGISTER_REQ: {
        // note: we use E1AP_REGISTER_REQ only to set up sockets, the
        // E1AP_SETUP_REQ is not used and comes through the socket from the
        // CU-UP later!
        e1ap_net_config_t *nc = &E1AP_REGISTER_REQ(msg).net_config;
        AssertFatal(nc->CUCP_e1_ip_address.ipv4 != 0, "No IPv4 address configured\n");
        cucp_task_send_sctp_init_req(0, nc->CUCP_e1_ip_address.ipv4_address);
      } break;

      case E1AP_SETUP_REQ:
        AssertFatal(false, "should not receive E1AP_SETUP_REQ in CU-CP!\n");
        break;

      case SCTP_DATA_IND:
        e1_task_handle_sctp_data_ind(myInstance, &msg->ittiMsg.sctp_data_ind);
        break;

      case E1AP_SETUP_RESP:
        e1ap_send_SETUP_RESPONSE(assoc_id, &E1AP_SETUP_RESP(msg));
        free_e1ap_cuup_setup_response(&E1AP_SETUP_RESP(msg));
        break;

      case E1AP_SETUP_FAIL:
        e1apCUCP_send_SETUP_FAILURE(assoc_id, &E1AP_SETUP_FAIL(msg));
        free_e1ap_cuup_setup_failure(&E1AP_SETUP_FAIL(msg));
        break;

      case E1AP_BEARER_CONTEXT_SETUP_REQ:
        e1apCUCP_send_BEARER_CONTEXT_SETUP_REQUEST(assoc_id, &E1AP_BEARER_CONTEXT_SETUP_REQ(msg));
        free_e1ap_context_setup_request(&E1AP_BEARER_CONTEXT_SETUP_REQ(msg));
        break;

      case E1AP_BEARER_CONTEXT_MODIFICATION_REQ:
        e1apCUCP_send_BEARER_CONTEXT_MODIFICATION_REQUEST(assoc_id, &E1AP_BEARER_CONTEXT_MODIFICATION_REQ(msg));
        free_e1ap_context_mod_request(&E1AP_BEARER_CONTEXT_MODIFICATION_REQ(msg));
        break;

      case E1AP_BEARER_CONTEXT_MODIFICATION_CONFIRM:
        e1apCUCP_send_BEARER_CONTEXT_MODIFICATION_CONFIRM(assoc_id, &E1AP_BEARER_CONTEXT_MODIFICATION_CONFIRM(msg));
        free_e1ap_context_mod_confirm(&E1AP_BEARER_CONTEXT_MODIFICATION_CONFIRM(msg));
        break;

      case E1AP_BEARER_CONTEXT_RELEASE_CMD:
        e1apCUCP_send_BEARER_CONTEXT_RELEASE_COMMAND(assoc_id, &E1AP_BEARER_CONTEXT_RELEASE_CMD(msg));
        free_e1_bearer_context_release_command(&E1AP_BEARER_CONTEXT_RELEASE_CMD(msg));
        break;

      default:
        LOG_E(E1AP, "Unknown message received in TASK_CUCP_E1\n");
        break;
    }

    result = itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d) in E1AP_CUCP_task!\n", result);
    msg = NULL;
  }
}

/**
 * @brief E1AP Centralized Unit User Plane (CU-UP) task function.
 *
 * This is the main task loop for the E1AP CUUP module:
 * it listens for incoming messages from the Inter-Task Interface (ITTI)
 * and calls the relevant handlers for each message type.
 *
 */
void *E1AP_CUUP_task(void *arg)
{
  UNUSED(arg);
  LOG_I(E1AP, "Starting E1AP at CU UP\n");
  e1ap_common_init();
  int result;

  // SCTP
  while (1) {
    MessageDef *msg = NULL;
    itti_receive_msg(TASK_CUUP_E1, &msg);
    const instance_t myInstance = ITTI_MSG_DESTINATION_INSTANCE(msg);
    const int msgType = ITTI_MSG_ID(msg);
    LOG_D(E1AP, "CUUP received %s for instance %ld\n", messages_info[msgType].name, myInstance);
    switch (msgType) {
      case E1AP_REGISTER_REQ: {
        /* E1AP Register Request triggered at startup
        create a new E1 instance and send the first association request */
        e1ap_register_req_t *reg_req = &E1AP_REGISTER_REQ(msg);
        createE1inst(UPtype, myInstance, reg_req->gnb_id, &reg_req->net_config, &reg_req->setup_req);
        e1_task_send_sctp_association_req(TASK_CUUP_E1, myInstance, &reg_req->net_config);
      } break;

      case SCTP_NEW_ASSOCIATION_RESP:
        e1_task_handle_sctp_association_resp(UPtype, myInstance, &msg->ittiMsg.sctp_new_association_resp);
        if (getCxtE1(myInstance)->sockState == SCTP_STATE_ESTABLISHED)
          LOG_A(E1AP, "E1 connection established (%s)\n", sctp_state_s[getCxtE1(myInstance)->sockState]);
        break;

      case SCTP_DATA_IND:
        e1_task_handle_sctp_data_ind(myInstance, &msg->ittiMsg.sctp_data_ind);
        break;

      case TIMER_HAS_EXPIRED:
        // Timer triggered by a connection loss with CU-CP
        e1apHandleTimer(myInstance);
        break;

      case E1AP_BEARER_CONTEXT_SETUP_RESP: {
        const e1ap_bearer_setup_resp_t *resp = &E1AP_BEARER_CONTEXT_SETUP_RESP(msg);
        const e1ap_upcp_inst_t *inst = getCxtE1(myInstance);
        AssertFatal(inst, "no E1 instance found for instance %ld\n", myInstance);
        e1apCUUP_send_BEARER_CONTEXT_SETUP_RESPONSE(inst->cuup.assoc_id, resp);
        free_e1ap_context_setup_response(resp);
      } break;

      case E1AP_BEARER_CONTEXT_SETUP_FAILURE: {
        const e1ap_bearer_context_setup_failure_t *fail = &E1AP_BEARER_CONTEXT_SETUP_FAILURE(msg);
        const e1ap_upcp_inst_t *inst = getCxtE1(myInstance);
        AssertFatal(inst, "no E1 instance found for instance %ld\n", myInstance);
        e1apCUUP_send_BEARER_CONTEXT_SETUP_FAILURE(inst->cuup.assoc_id, fail);
        free_e1_bearer_context_setup_failure(fail);
      } break;

      case E1AP_BEARER_CONTEXT_MODIFICATION_RESP: {
        const e1ap_bearer_modif_resp_t *resp = &E1AP_BEARER_CONTEXT_MODIFICATION_RESP(msg);
        const e1ap_upcp_inst_t *inst = getCxtE1(myInstance);
        AssertFatal(inst, "no E1 instance found for instance %ld\n", myInstance);
        e1apCUUP_send_BEARER_CONTEXT_MODIFICATION_RESPONSE(inst->cuup.assoc_id, resp);
        free_e1ap_context_mod_response(resp);
      } break;

      case E1AP_BEARER_CONTEXT_MODIFICATION_FAIL: {
        const e1ap_bearer_context_mod_failure_t *fail = &E1AP_BEARER_CONTEXT_MODIFICATION_FAIL(msg);
        const e1ap_upcp_inst_t *inst = getCxtE1(myInstance);
        AssertFatal(inst, "no E1 instance found for instance %ld\n", myInstance);
        e1apCUUP_send_BEARER_CONTEXT_MODIFICATION_FAILURE(inst->cuup.assoc_id, fail);
        free_E1_bearer_context_mod_failure(fail);
      } break;

      case E1AP_BEARER_CONTEXT_MODIFICATION_REQUIRED: {
        const e1ap_bearer_mod_required_t *req = &E1AP_BEARER_CONTEXT_MODIFICATION_REQUIRED(msg);
        const e1ap_upcp_inst_t *inst = getCxtE1(myInstance);
        AssertFatal(inst, "no E1 instance found for instance %ld\n", myInstance);
        e1apCUUP_send_BEARER_CONTEXT_MODIFICATION_REQUIRED(inst->cuup.assoc_id, req);
        free_e1ap_context_mod_required(req);
      } break;

      case E1AP_BEARER_CONTEXT_RELEASE_CPLT: {
        const e1ap_bearer_release_cplt_t *cplt = &E1AP_BEARER_CONTEXT_RELEASE_CPLT(msg);
        const e1ap_upcp_inst_t *inst = getCxtE1(myInstance);
        AssertFatal(inst, "no E1 instance found for instance %ld\n", myInstance);
        e1apCUUP_send_BEARER_CONTEXT_RELEASE_COMPLETE(inst->cuup.assoc_id, cplt);
      } break;

      default:
        LOG_E(E1AP, "Unknown message received in TASK_CUUP_E1\n");
        break;
    }

    result = itti_free(ITTI_MSG_ORIGIN_ID(msg), msg);
    AssertFatal(result == EXIT_SUCCESS, "Failed to free memory (%d) in E1AP_CUUP_task!\n", result);
    msg = NULL;
  }
}
