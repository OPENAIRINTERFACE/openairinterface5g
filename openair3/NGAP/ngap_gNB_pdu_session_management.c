/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "ngap_gNB_pdu_session_management.h"
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "INTEGER.h"
#include "ngap_msg_includes.h"
#include "OCTET_STRING.h"
#include "aper_encoder.h"
#include "assertions.h"
#include "conversions.h"
#include "ngap_common.h"
#include "ngap_gNB_encoder.h"
#include "ngap_gNB_itti_messaging.h"
#include "ngap_gNB_ue_context.h"
#include "ngap_gNB_management_procedures.h"
#include "oai_asn1.h"
#include "ds/byte_array.h"

int ngap_gNB_pdusession_setup_resp(instance_t instance, ngap_pdusession_setup_resp_t *pdusession_setup_resp_p)
{
  ngap_gNB_instance_t *ngap_gNB_instance_p = NULL;
  struct ngap_gNB_ue_context_s *ue_context_p = NULL;
  NGAP_NGAP_PDU_t pdu = {0};
  uint8_t *buffer = NULL;
  uint32_t length;

  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(pdusession_setup_resp_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_get_ue_context(pdusession_setup_resp_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%08x\n", pdusession_setup_resp_p->gNB_ue_ngap_id);
    return -1;
  }

  /* Uplink NAS transport can occur either during an ngap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED || ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
    NGAP_WARN(
        "You are attempting to send NAS data over non-connected "
        "gNB ue ngap id: %08x, current state: %d\n",
        pdusession_setup_resp_p->gNB_ue_ngap_id,
        ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, successfulOutcome);
  successfulOutcome->procedureCode = NGAP_ProcedureCode_id_PDUSessionResourceSetup;
  successfulOutcome->criticality = NGAP_Criticality_reject;
  successfulOutcome->value.present = NGAP_SuccessfulOutcome__value_PR_PDUSessionResourceSetupResponse;
  NGAP_PDUSessionResourceSetupResponse_t *out = &successfulOutcome->value.choice.PDUSessionResourceSetupResponse;
  /* mandatory */
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceSetupResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceSetupResponseIEs__value_PR_AMF_UE_NGAP_ID;
    asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  }
  /* mandatory */
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceSetupResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceSetupResponseIEs__value_PR_RAN_UE_NGAP_ID;
    ie->value.choice.RAN_UE_NGAP_ID = pdusession_setup_resp_p->gNB_ue_ngap_id;
  }

  /* optional */
  if (pdusession_setup_resp_p->nb_of_pdusessions > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceSetupResponseIEs_t, ie3);
    ie3->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListSURes;
    ie3->criticality = NGAP_Criticality_ignore;
    ie3->value.present = NGAP_PDUSessionResourceSetupResponseIEs__value_PR_PDUSessionResourceSetupListSURes;

    for (int i = 0; i < pdusession_setup_resp_p->nb_of_pdusessions; i++) {
      pdusession_setup_t *pdusession = pdusession_setup_resp_p->pdusessions + i;
      asn1cSequenceAdd(ie3->value.choice.PDUSessionResourceSetupListSURes.list, NGAP_PDUSessionResourceSetupItemSURes_t, item);
      /* pDUSessionID */
      item->pDUSessionID = pdusession->pdusession_id;

      // PDU Session Resource Setup Response Transfer (Mandatory)
      byte_array_t ba = encode_ngap_pdusession_setup_response_transfer(pdusession);
      if (ba.buf == NULL) {
        NGAP_ERROR("Failed to encode PDUSessionResourceSetupResponseTransfer for PDU session %ld\n", item->pDUSessionID);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
        return -1;
      }
      item->pDUSessionResourceSetupResponseTransfer.buf = ba.buf;
      item->pDUSessionResourceSetupResponseTransfer.size = ba.len;
    }
  }

  /* optional */
  if (pdusession_setup_resp_p->nb_of_pdusessions_failed > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceSetupResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceFailedToSetupListSURes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceSetupResponseIEs__value_PR_PDUSessionResourceFailedToSetupListSURes;

    for (int i = 0; i < pdusession_setup_resp_p->nb_of_pdusessions_failed; i++) {
      pdusession_failed_t *pdusession_failed = pdusession_setup_resp_p->pdusessions_failed + i;
      asn1cSequenceAdd(ie->value.choice.PDUSessionResourceFailedToSetupListSURes.list,
                       NGAP_PDUSessionResourceFailedToSetupItemSURes_t,
                       item);
      item->pDUSessionID = pdusession_failed->pdusession_id;

      byte_array_t ba = encode_ngap_pdusession_setup_unsuccessful_transfer(&pdusession_failed->cause);
      if (ba.buf == NULL) {
        NGAP_ERROR("Failed to encode PDUSessionResourceSetupUnsuccessfulTransfer for PDU session %ld\n", item->pDUSessionID);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
        return -1;
      }
      item->pDUSessionResourceSetupUnsuccessfulTransfer.buf = ba.buf;
      item->pDUSessionResourceSetupUnsuccessfulTransfer.size = ba.len;
    }
  }

  /* optional */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceSetupResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceSetupResponseIEs__value_PR_CriticalityDiagnostics;
  }

  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NGAP_ERROR("Failed to encode uplink transport\n");
    /* Encode procedure has failed... */
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
    return -1;
  }

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id,
                                   buffer,
                                   length,
                                   ue_context_p->tx_stream);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
  return 0;
}

int ngap_gNB_pdusession_modify_resp(instance_t instance, ngap_pdusession_modify_resp_t *pdusession_modify_resp_p)
{
  ngap_gNB_instance_t *ngap_gNB_instance_p = NULL;
  struct ngap_gNB_ue_context_s *ue_context_p = NULL;
  NGAP_NGAP_PDU_t pdu;
  uint8_t *buffer = NULL;
  uint32_t length;

  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(pdusession_modify_resp_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_get_ue_context(pdusession_modify_resp_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%08x\n", pdusession_modify_resp_p->gNB_ue_ngap_id);
    return -1;
  }

  /* Uplink NAS transport can occur either during an ngap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED || ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
    NGAP_WARN(
        "You are attempting to send NAS data over non-connected "
        "gNB ue ngap id: %08x, current state: %d\n",
        pdusession_modify_resp_p->gNB_ue_ngap_id,
        ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, head);
  head->procedureCode = NGAP_ProcedureCode_id_PDUSessionResourceModify;
  head->criticality = NGAP_Criticality_reject;
  head->value.present = NGAP_SuccessfulOutcome__value_PR_PDUSessionResourceModifyResponse;
  NGAP_PDUSessionResourceModifyResponse_t *out = &head->value.choice.PDUSessionResourceModifyResponse;
  /* mandatory */
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceModifyResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceModifyResponseIEs__value_PR_AMF_UE_NGAP_ID;
    asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, pdusession_modify_resp_p->amf_ue_ngap_id);
  }
  /* mandatory */
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceModifyResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceModifyResponseIEs__value_PR_RAN_UE_NGAP_ID;
    ie->value.choice.RAN_UE_NGAP_ID = pdusession_modify_resp_p->gNB_ue_ngap_id;
  }

  /* PDUSessionResourceModifyListModRes optional */
  if (pdusession_modify_resp_p->nb_of_pdusessions > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceModifyResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceModifyListModRes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceModifyResponseIEs__value_PR_PDUSessionResourceModifyListModRes;

    for (int i = 0; i < pdusession_modify_resp_p->nb_of_pdusessions; i++) {
      asn1cSequenceAdd(ie->value.choice.PDUSessionResourceModifyListModRes.list, NGAP_PDUSessionResourceModifyItemModRes_t, item);
      item->pDUSessionID = pdusession_modify_resp_p->pdusessions[i].pdusession_id;

      NGAP_PDUSessionResourceModifyResponseTransfer_t transfer = {0};
      if (pdusession_modify_resp_p->pdusessions[i].nb_of_qos_flow > 0) {
        asn1cCalloc(transfer.qosFlowAddOrModifyResponseList, tmp);
        for (int qos_flow_index = 0; qos_flow_index < pdusession_modify_resp_p->pdusessions[i].nb_of_qos_flow; qos_flow_index++) {
          asn1cSequenceAdd(tmp->list, NGAP_QosFlowAddOrModifyResponseItem_t, qos);
          qos->qosFlowIdentifier = pdusession_modify_resp_p->pdusessions[i].qos[qos_flow_index].qfi;
        }
      }
      void *buf = NULL;
      ssize_t encoded = aper_encode_to_new_buffer(&asn_DEF_NGAP_PDUSessionResourceModifyResponseTransfer, NULL, &transfer, &buf);
      if (encoded < 0) {
        NGAP_ERROR("Failed to encode PDUSessionResourceModifyResponseTransfer for PDU session %ld\n", item->pDUSessionID);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceModifyResponseTransfer, &transfer);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
        return -1;
      }
      item->pDUSessionResourceModifyResponseTransfer.buf = buf;
      item->pDUSessionResourceModifyResponseTransfer.size = encoded;

      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceModifyResponseTransfer, &transfer);

      NGAP_DEBUG("pdusession_modify_resp: modified pdusession ID %ld\n", item->pDUSessionID);
    }
  }

  /* optional */
  if (pdusession_modify_resp_p->nb_of_pdusessions_failed > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceModifyResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceFailedToModifyListModRes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceModifyResponseIEs__value_PR_PDUSessionResourceFailedToModifyListModRes;

    for (int i = 0; i < pdusession_modify_resp_p->nb_of_pdusessions_failed; i++) {
      asn1cSequenceAdd(ie->value.choice.PDUSessionResourceFailedToModifyListModRes.list,
                       NGAP_PDUSessionResourceFailedToModifyItemModRes_t,
                       item);
      item->pDUSessionID = pdusession_modify_resp_p->pdusessions_failed[i].pdusession_id;

      NGAP_PDUSessionResourceModifyUnsuccessfulTransfer_t pdusessionTransfer = {0};

      encode_ngap_cause(&pdusessionTransfer.cause, &pdusession_modify_resp_p->pdusessions_failed[i].cause);

      void *buf = NULL;
      ssize_t encoded =
          aper_encode_to_new_buffer(&asn_DEF_NGAP_PDUSessionResourceModifyUnsuccessfulTransfer, NULL, &pdusessionTransfer, &buf);
      if (encoded < 0) {
        NGAP_ERROR("Failed to encode PDUSessionResourceModifyUnsuccessfulTransfer for PDU session %ld\n", item->pDUSessionID);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceModifyUnsuccessfulTransfer, &pdusessionTransfer);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
        return -1;
      }
      item->pDUSessionResourceModifyUnsuccessfulTransfer.buf = buf;
      item->pDUSessionResourceModifyUnsuccessfulTransfer.size = encoded;

      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceModifyUnsuccessfulTransfer, &pdusessionTransfer);

      NGAP_INFO("pdusession_modify_resp: failed pdusession ID %ld\n", item->pDUSessionID);
    }
  }

  /* optional */
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceModifyResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceModifyResponseIEs__value_PR_CriticalityDiagnostics;
  }

  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NGAP_ERROR("Failed to encode uplink transport\n");
    /* Encode procedure has failed... */
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
    return -1;
  }

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id,
                                   buffer,
                                   length,
                                   ue_context_p->tx_stream);

  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
  return 0;
}

/** @brief PDU Session Resource Release Response Transfer encoding (9.3.4.21 3GPP TS 38.413)
 *  The transfer structure contains only an optional Secondary RAT Usage Information IE.
 *  Since we don't use secondary RAT (MR-DC), we encode an empty structure. */
static byte_array_t encode_ngap_pdusession_release_response_transfer(void)
{
  NGAP_PDUSessionResourceReleaseResponseTransfer_t pdusessionTransfer = {0};
  byte_array_t out = {0};

  void *buf = NULL;
  ssize_t encoded =
      aper_encode_to_new_buffer(&asn_DEF_NGAP_PDUSessionResourceReleaseResponseTransfer, NULL, &pdusessionTransfer, &buf);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceReleaseResponseTransfer, &pdusessionTransfer);
  if (encoded < 0)
    return out;
  out.buf = buf;
  out.len = encoded;
  return out;
}

int ngap_gNB_pdusession_release_resp(instance_t instance, ngap_pdusession_release_resp_t *pdusession_release_resp_p)
{
  ngap_gNB_instance_t *ngap_gNB_instance_p = NULL;
  struct ngap_gNB_ue_context_s *ue_context_p = NULL;
  NGAP_NGAP_PDU_t pdu;
  uint8_t *buffer = NULL;
  uint32_t length;
  int i;
  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(pdusession_release_resp_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_get_ue_context(pdusession_release_resp_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%08x\n", pdusession_release_resp_p->gNB_ue_ngap_id);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, head);
  head->procedureCode = NGAP_ProcedureCode_id_PDUSessionResourceRelease;
  head->criticality = NGAP_Criticality_reject;
  head->value.present = NGAP_SuccessfulOutcome__value_PR_PDUSessionResourceReleaseResponse;
  NGAP_PDUSessionResourceReleaseResponse_t *out = &head->value.choice.PDUSessionResourceReleaseResponse;
  /* mandatory */
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceReleaseResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceReleaseResponseIEs__value_PR_AMF_UE_NGAP_ID;
    asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  }
  /* mandatory */
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceReleaseResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceReleaseResponseIEs__value_PR_RAN_UE_NGAP_ID;
    ie->value.choice.RAN_UE_NGAP_ID = pdusession_release_resp_p->gNB_ue_ngap_id;
  }

  /* PDU Session Resource Released List (mandatory) */
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceReleaseResponseIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceReleasedListRelRes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceReleaseResponseIEs__value_PR_PDUSessionResourceReleasedListRelRes;

    for (i = 0; i < pdusession_release_resp_p->nb_of_pdusessions_released; i++) {
      NGAP_PDUSessionResourceReleasedListRelRes_t *list = &ie->value.choice.PDUSessionResourceReleasedListRelRes;
      asn1cSequenceAdd(list->list, NGAP_PDUSessionResourceReleasedItemRelRes_t, item);
      pdusession_release_t *r = &pdusession_release_resp_p->pdusession_release[i];
      /* PDU Session ID (mandatory) */
      item->pDUSessionID = r->pdusession_id;
      /* PDU Session Resource Release Response Transfer (mandatory) */
      // Empty transfer is valid since Secondary RAT Usage Information is optional and not used
      byte_array_t transfer = encode_ngap_pdusession_release_response_transfer();
      if (transfer.buf == NULL) {
        NGAP_ERROR("Failed to encode PDUSessionResourceReleaseResponseTransfer for PDU session %ld\n", item->pDUSessionID);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
        return -1;
      }
      OCTET_STRING_fromBuf(&item->pDUSessionResourceReleaseResponseTransfer, (const char *)transfer.buf, transfer.len);
      free_byte_array(transfer);
      NGAP_DEBUG("PDU Session Resource Release Response: pdusession ID %ld\n", item->pDUSessionID);
    }
  }

  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NGAP_ERROR("Failed to encode release response\n");
    /* Encode procedure has failed... */
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
    return -1;
  }

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id,
                                   buffer,
                                   length,
                                   ue_context_p->tx_stream);
  NGAP_INFO("pdusession_release_response sended gNB_UE_NGAP_ID %u  amf_ue_ngap_id %lu nb_of_pdusessions_released %d \n",
            pdusession_release_resp_p->gNB_ue_ngap_id,
            (uint64_t)ue_context_p->amf_ue_ngap_id,
            pdusession_release_resp_p->nb_of_pdusessions_released);

  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
  return 0;
}

/** @brief Encode and send NGAP PDU Session Resource Notify (8.2.4 of 3GPP TS 38.413) */
int ngap_gNB_pdusession_resource_notify(instance_t instance, ngap_pdusession_resource_notify_t *msg)
{
  DevAssert(msg);
  NGAP_NGAP_PDU_t pdu;
  uint8_t *buffer = NULL;
  uint32_t length;

  ngap_gNB_instance_t *ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(ngap_gNB_instance_p);

  struct ngap_gNB_ue_context_s *ue_context_p = ngap_get_ue_context(msg->gNB_ue_ngap_id);
  if (!ue_context_p) {
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%08x\n", msg->gNB_ue_ngap_id);
    return -1;
  }

  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, head);
  head->procedureCode = NGAP_ProcedureCode_id_PDUSessionResourceNotify;
  head->criticality = NGAP_Criticality_ignore;
  head->value.present = NGAP_InitiatingMessage__value_PR_PDUSessionResourceNotify;
  NGAP_PDUSessionResourceNotify_t *out = &head->value.choice.PDUSessionResourceNotify;

  /* AMF-UE-NGAP-ID (mandatory) */
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceNotifyIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_PDUSessionResourceNotifyIEs__value_PR_AMF_UE_NGAP_ID;
    asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  }

  /* RAN-UE-NGAP-ID (mandatory) */
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceNotifyIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_PDUSessionResourceNotifyIEs__value_PR_RAN_UE_NGAP_ID;
    ie->value.choice.RAN_UE_NGAP_ID = msg->gNB_ue_ngap_id;
  }

  /* PDUSessionResourceReleasedListNot (optional) */
  if (msg->nb_pdu_sessions_released > 0) {
    if (msg->nb_pdu_sessions_released > NR_MAX_NB_PDU_SESSIONS) {
      NGAP_WARN("PDU Session Resource Notify with invalid released list count %d for gNB_ue_ngap_id %u\n",
                msg->nb_pdu_sessions_released,
                msg->gNB_ue_ngap_id);
      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
      return -1;
    }
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PDUSessionResourceNotifyIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceReleasedListNot;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceNotifyIEs__value_PR_PDUSessionResourceReleasedListNot;
    NGAP_PDUSessionResourceReleasedListNot_t *list = &ie->value.choice.PDUSessionResourceReleasedListNot;

    for (int i = 0; i < msg->nb_pdu_sessions_released; i++) {
      const ngap_pdusession_notify_item_t *released = &msg->pdu_sessions[i];
      asn1cSequenceAdd(list->list, NGAP_PDUSessionResourceReleasedItemNot_t, item);
      item->pDUSessionID = released->pdu_session_id;

      NGAP_PDUSessionResourceNotifyReleasedTransfer_t notifyTransfer = {0};
      encode_ngap_cause(&notifyTransfer.cause, &released->cause);
      asn_encode_to_new_buffer_result_t res = asn_encode_to_new_buffer(NULL,
                                                                       ATS_ALIGNED_CANONICAL_PER,
                                                                       &asn_DEF_NGAP_PDUSessionResourceNotifyReleasedTransfer,
                                                                       &notifyTransfer);
      if (res.buffer == NULL || res.result.encoded <= 0) {
        NGAP_ERROR("Failed to encode PDUSessionResourceNotifyReleasedTransfer for PDU session %u\n", released->pdu_session_id);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceNotifyReleasedTransfer, &notifyTransfer);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
        return -1;
      }
      item->pDUSessionResourceNotifyReleasedTransfer.buf = res.buffer;
      item->pDUSessionResourceNotifyReleasedTransfer.size = res.result.encoded;
      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceNotifyReleasedTransfer, &notifyTransfer);
    }
  }

  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NGAP_ERROR("Failed to encode PDU Session Resource Notify\n");
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
    return -1;
  }

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id,
                                   buffer,
                                   length,
                                   ue_context_p->tx_stream);
  NGAP_INFO("pdusession_resource_notify sent gNB_UE_NGAP_ID %u amf_ue_ngap_id %lu nb_released %d\n",
            msg->gNB_ue_ngap_id,
            ue_context_p->amf_ue_ngap_id,
            msg->nb_pdu_sessions_released);

  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
  return 0;
}
