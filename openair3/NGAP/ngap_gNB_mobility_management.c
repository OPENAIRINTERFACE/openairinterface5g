/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "conversions.h"
#include "ngap_common.h"
#include "ngap_msg_includes.h"
#include "ngap_gNB_defs.h"
#include "ngap_gNB_ue_context.h"
#include "oai_asn1.h"
#include "ngap_gNB_management_procedures.h"
#include "ngap_gNB_encoder.h"
#include "ngap_gNB_itti_messaging.h"
#include "NR_HandoverPreparationInformation.h"
#include "conversions.h"

/** @brief UE Mobility Management: encode Handover Required
 *         (9.2.3.1 of 3GPP TS 38.413) NG-RAN node → AMF */
NGAP_NGAP_PDU_t *encode_ng_handover_required(const ngap_handover_required_t *msg)
{
  NGAP_NGAP_PDU_t *pdu = calloc_or_fail(1, sizeof(*pdu));

  /* Prepare the NGAP message to encode */
  pdu->present = NGAP_NGAP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu->choice.initiatingMessage, head);
  head->procedureCode = NGAP_ProcedureCode_id_HandoverPreparation;
  head->criticality = NGAP_Criticality_reject;
  head->value.present = NGAP_InitiatingMessage__value_PR_HandoverRequired;
  NGAP_HandoverRequired_t *out = &head->value.choice.HandoverRequired;

  // AMF UE NGAP ID (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequiredIEs_t, ie1);
  ie1->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie1->criticality = NGAP_Criticality_reject;
  ie1->value.present = NGAP_HandoverRequiredIEs__value_PR_AMF_UE_NGAP_ID;
  asn_uint642INTEGER(&ie1->value.choice.AMF_UE_NGAP_ID, msg->amf_ue_ngap_id);

  // RAN UE NGAP ID (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequiredIEs_t, ie2);
  ie2->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie2->criticality = NGAP_Criticality_reject;
  ie2->value.present = NGAP_HandoverRequiredIEs__value_PR_RAN_UE_NGAP_ID;
  ie2->value.choice.RAN_UE_NGAP_ID = msg->gNB_ue_ngap_id;

  // Handover Type (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequiredIEs_t, ie3);
  ie3->id = NGAP_ProtocolIE_ID_id_HandoverType;
  ie3->criticality = NGAP_Criticality_reject;
  ie3->value.present = NGAP_HandoverRequiredIEs__value_PR_HandoverType;
  ie3->value.choice.HandoverType = msg->handoverType;

  // Cause (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequiredIEs_t, ie4);
  ie4->id = NGAP_ProtocolIE_ID_id_Cause;
  ie4->criticality = NGAP_Criticality_ignore;
  ie4->value.present = NGAP_HandoverRequiredIEs__value_PR_Cause;
  encode_ngap_cause(&ie4->value.choice.Cause, &msg->cause);

  // Target ID (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequiredIEs_t, ie5);
  encode_ngap_target_id(ie5, &msg->target_gnb_id);

  // PDU Session Resource List (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequiredIEs_t, ie6);
  ie6->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceListHORqd;
  ie6->criticality = NGAP_Criticality_reject;
  ie6->value.present = NGAP_HandoverRequiredIEs__value_PR_PDUSessionResourceListHORqd;
  for (int i = 0; i < msg->nb_of_pdusessions; ++i) {
    asn1cSequenceAdd(ie6->value.choice.PDUSessionResourceListHORqd.list, NGAP_PDUSessionResourceItemHORqd_t, hoRequiredPduSession);
    // PDU Session ID (M)
    hoRequiredPduSession->pDUSessionID = msg->pdusessions[i].pdusession_id;
    // Handover Required Transfer (M) - ASN.1 encoding of empty transfer structure
    // This is a mandatory field in the NGAP Handover Required message, however it contains
    // only Direct Forwarding Path Availability IE, which is optional and is not sent
    NGAP_HandoverRequiredTransfer_t hoRequiredTransfer = {0};
    uint8_t ho_req_transfer_transparent_container_buffer[128] = {0};
    if (LOG_DEBUGFLAG(DEBUG_ASN1))
      xer_fprint(stdout, &asn_DEF_NGAP_HandoverRequiredTransfer, &hoRequiredTransfer);
    asn_enc_rval_t enc_rval = aper_encode_to_buffer(&asn_DEF_NGAP_HandoverRequiredTransfer,
                                                    NULL,
                                                    &hoRequiredTransfer,
                                                    ho_req_transfer_transparent_container_buffer,
                                                    128);
    AssertFatal(enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n", enc_rval.failed_type->name, enc_rval.encoded);
    hoRequiredPduSession->handoverRequiredTransfer.buf = CALLOC(1, (enc_rval.encoded + 7) / 8);
    memcpy(hoRequiredPduSession->handoverRequiredTransfer.buf,
           ho_req_transfer_transparent_container_buffer,
           (enc_rval.encoded + 7) / 8);
    hoRequiredPduSession->handoverRequiredTransfer.size = (enc_rval.encoded + 7) / 8;

    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_HandoverRequiredTransfer, &hoRequiredTransfer);
  }

  // Source NG-RAN Node to Target NG-RAN Node Transparent Container (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequiredIEs_t, ie);
  ie->id = NGAP_ProtocolIE_ID_id_SourceToTarget_TransparentContainer;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_HandoverRequiredIEs__value_PR_SourceToTarget_TransparentContainer;

  NGAP_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer_t *source2target = calloc_or_fail(1, sizeof(*source2target));

  // RRC Container (M) (HandoverPreparationInformation)
  source2target->rRCContainer.size = msg->source2target->handoverInfo.len;
  source2target->rRCContainer.buf = malloc_or_fail(msg->source2target->handoverInfo.len);
  memcpy(source2target->rRCContainer.buf, msg->source2target->handoverInfo.buf, source2target->rRCContainer.size);

  // PDU Session Resource Information List (O)
  asn1cCalloc(source2target->pDUSessionResourceInformationList, pduSessionList);
  for (uint16_t i = 0; i < msg->nb_of_pdusessions; ++i) {
    const pdusession_resource_t *pduSession = &msg->pdusessions[i];
    NGAP_DEBUG("Handover Required: preparing PDU Session Resource Information List for PDU Session ID %d\n",
               pduSession->pdusession_id);
    asn1cSequenceAdd(pduSessionList->list, NGAP_PDUSessionResourceInformationItem_t, item);
    // PDU Session ID (M)
    item->pDUSessionID = msg->source2target->pdu_session_resource[i].pdusession_id;
    // QoS Flow Information List (M)
    for (int q = 0; q < msg->source2target->pdu_session_resource[i].nb_of_qos_flow; ++q) {
      asn1cSequenceAdd(item->qosFlowInformationList.list, NGAP_QosFlowInformationItem_t, qosFlowInfo);
      qosFlowInfo->qosFlowIdentifier = msg->source2target->pdu_session_resource[i].qos_flow_info[q].qfi;
    }
  }

  // Target Cell ID (NG-RAN CGI) (M)
  source2target->targetCell_ID.present = NGAP_NGRAN_CGI_PR_nR_CGI;
  asn1cCalloc(source2target->targetCell_ID.choice.nR_CGI, tNrCGI);
  encode_ngap_nr_cgi(tNrCGI, &msg->target_gnb_id.plmn_identity, msg->source2target->targetCellId.nrCellIdentity);

  // UE history Information (M)
  asn1cSequenceAdd(source2target->uEHistoryInformation.list, NGAP_LastVisitedCellItem_t, lastVisitedCell);
  lastVisitedCell->iE_Extensions = NULL;
  // Last Visited Cell Information (M)
  lastVisitedCell->lastVisitedCellInformation.present = NGAP_LastVisitedCellInformation_PR_nGRANCell;
  // CHOICE (M): NG-RAN Cell
  asn1cCalloc(lastVisitedCell->lastVisitedCellInformation.choice.nGRANCell, lastVisitedNR);
  // Cell Type (M)
  lastVisitedNR->cellType.cellSize = msg->source2target->ue_history_info.type;
  // Global Cell ID (M)
  lastVisitedNR->globalCellID.present = NGAP_NGRAN_CGI_PR_nR_CGI;
  asn1cCalloc(lastVisitedNR->globalCellID.choice.nR_CGI, lastVisitedNrCGI);
  cell_id_t *cell = &msg->source2target->ue_history_info.id;
  encode_ngap_nr_cgi(lastVisitedNrCGI, &cell->plmn_identity, cell->nrCellIdentity);
  // HO Cause Value (O)
  if (msg->source2target->ue_history_info.cause) {
    asn1cCalloc(lastVisitedNR->hOCauseValue, lastVisitedCause);
    encode_ngap_cause(lastVisitedCause, msg->source2target->ue_history_info.cause);
  }
  // Time UE Stayed in Cell (M)
  lastVisitedNR->timeUEStayedInCell = msg->source2target->ue_history_info.time_in_cell;

  if (LOG_DEBUGFLAG(DEBUG_ASN1))
    xer_fprint(stdout, &asn_DEF_NGAP_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer, source2target);

  uint8_t source_to_target_transparent_container_buf[16384] = {0};
  asn_enc_rval_t enc_rval = aper_encode_to_buffer(&asn_DEF_NGAP_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer,
                                                  NULL,
                                                  (void *)source2target,
                                                  (void *)&source_to_target_transparent_container_buf,
                                                  16384);
  ASN_STRUCT_FREE(asn_DEF_NGAP_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer, source2target);
  if (enc_rval.encoded < 0) {
    AssertFatal(enc_rval.encoded > 0,
                "HO LOG: Source to Transparent ASN1 message encoding failed (%s, %lu)!\n",
                enc_rval.failed_type->name,
                enc_rval.encoded);
    return NULL;
  }

  int total_bytes = (enc_rval.encoded + 7) / 8;
  int ret = OCTET_STRING_fromBuf(&ie->value.choice.SourceToTarget_TransparentContainer,
                                 (const char *)&source_to_target_transparent_container_buf,
                                 total_bytes);
  if (ret != 0) {
    LOG_E(NR_RRC, "HO LOG: Can not perform OCTET_STRING_fromBuf for the SourceToTarget_TransparentContainer");
    return NULL;
  }

  return pdu;
}

NGAP_NGAP_PDU_t *encode_ng_handover_failure(const ngap_handover_failure_t *msg)
{
  NGAP_NGAP_PDU_t *pdu = calloc_or_fail(1, sizeof(*pdu));

  /* Prepare the NGAP message to encode */
  pdu->present = NGAP_NGAP_PDU_PR_unsuccessfulOutcome;
  asn1cCalloc(pdu->choice.unsuccessfulOutcome, head);
  head->procedureCode = NGAP_ProcedureCode_id_HandoverResourceAllocation;
  head->criticality = NGAP_Criticality_reject;
  head->value.present = NGAP_UnsuccessfulOutcome__value_PR_HandoverFailure;
  NGAP_HandoverFailure_t *out = &head->value.choice.HandoverFailure;

  // AMF_UE_NGAP_ID (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverFailureIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_HandoverFailureIEs__value_PR_AMF_UE_NGAP_ID;
    asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, msg->amf_ue_ngap_id);
  }

  // Cause (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverFailureIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_Cause;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_HandoverFailureIEs__value_PR_Cause;
    encode_ngap_cause(&ie->value.choice.Cause, &msg->cause);
  }

  return pdu;
}

static void free_ng_handover_request(ngap_handover_request_t *msg)
{
  free_byte_array(msg->ue_ho_prep_info);
  free_byte_array(msg->ue_cap);
  free(msg->mobility_restriction);
}

int decode_ng_handover_request(ngap_handover_request_t *out, const NGAP_NGAP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);
  NGAP_HandoverRequest_t *container = &pdu->choice.initiatingMessage->value.choice.HandoverRequest;
  NGAP_HandoverRequestIEs_t *ie;

  // Handover Type (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t, ie, container, NGAP_ProtocolIE_ID_id_HandoverType, true);
  out->ho_type = ie->value.choice.HandoverType;
  if (out->ho_type != HANDOVER_TYPE_INTRA5GS) {
    NGAP_ERROR("Only Intra5GS Handover is supported at the moment!\n");
    return -1;
  }

  // AMF UE NGAP ID (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t, ie, container, NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID, true);
  asn_INTEGER2ulong(&(ie->value.choice.AMF_UE_NGAP_ID), &out->amf_ue_ngap_id);

  // GUAMI (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t, ie, container, NGAP_ProtocolIE_ID_id_GUAMI, true);
  out->guami = decode_ngap_guami(&ie->value.choice.GUAMI);

  // UE Aggregate Maximum Bit Rate (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t, ie, container, NGAP_ProtocolIE_ID_id_UEAggregateMaximumBitRate, true);
  out->ue_ambr = decode_ngap_UEAggregateMaximumBitRate(&ie->value.choice.UEAggregateMaximumBitRate);

  // Allowed NSSAI (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t, ie, container, NGAP_ProtocolIE_ID_id_AllowedNSSAI, true);
  NGAP_DEBUG("AllowedNSSAI.list.count %d\n", ie->value.choice.AllowedNSSAI.list.count);
  out->nb_allowed_nssais = ie->value.choice.AllowedNSSAI.list.count;
  for (int i = 0; i < out->nb_allowed_nssais; i++) {
    out->allowed_nssai[i] = decode_ngap_nssai(&ie->value.choice.AllowedNSSAI.list.array[i]->s_NSSAI);
  }

  // UE Security Capabilities (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t, ie, container, NGAP_ProtocolIE_ID_id_UESecurityCapabilities, true);
  out->security_capabilities = decode_ngap_security_capabilities(&ie->value.choice.UESecurityCapabilities);

  // Security Context (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t, ie, container, NGAP_ProtocolIE_ID_id_SecurityContext, true);
  NGAP_SecurityContext_t *sc = &ie->value.choice.SecurityContext;
  memcpy(&out->security_context.next_hop, sc->nextHopNH.buf, sc->nextHopNH.size);
  out->security_context.next_hop_chain_count = sc->nextHopChainingCount;

  // Mobility Restriction List (O)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t, ie, container, NGAP_ProtocolIE_ID_id_MobilityRestrictionList, false);
  if (ie != NULL) {
    out->mobility_restriction = calloc_or_fail(1, sizeof(*out->mobility_restriction));
    *out->mobility_restriction = decode_ngap_mobility_restriction(&ie->value.choice.MobilityRestrictionList);
  }

  // Cause (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t, ie, container, NGAP_ProtocolIE_ID_id_Cause, true);
  out->cause = decode_ngap_cause(&ie->value.choice.Cause);

  // Source to Target Transparent Container
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t,
                             ie,
                             container,
                             NGAP_ProtocolIE_ID_id_SourceToTarget_TransparentContainer,
                             true);
  NGAP_SourceToTarget_TransparentContainer_t *choice = &ie->value.choice.SourceToTarget_TransparentContainer;
  byte_array_t sourceToTargetTransparentContainer = create_byte_array(choice->size, choice->buf);
  NGAP_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer_t *source2target = NULL;
  asn_dec_rval_t dec_rval = aper_decode_complete(NULL,
                                                 &asn_DEF_NGAP_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer,
                                                 (void **)&source2target,
                                                 sourceToTargetTransparentContainer.buf,
                                                 sourceToTargetTransparentContainer.len);

  free_byte_array(sourceToTargetTransparentContainer);
  if (dec_rval.code != RC_OK) {
    free_ng_handover_request(out);
    NGAP_ERROR("Failed to decode sourceToTargetTransparentContainer\n");
    return -1;
  }

  if (LOG_DEBUGFLAG(DEBUG_ASN1))
    xer_fprint(stdout, &asn_DEF_NGAP_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer, source2target);

  // Extract Cell Identity
  BIT_STRING_TO_NR_CELL_IDENTITY(&source2target->targetCell_ID.choice.nR_CGI->nRCellIdentity, out->nr_cell_id);

  // Handover Preparation Information: store and decode
  out->ue_ho_prep_info = create_byte_array(source2target->rRCContainer.size, source2target->rRCContainer.buf);
  NR_HandoverPreparationInformation_t *hoPrepInformation = NULL;
  asn_dec_rval_t hoPrep_dec_rval = uper_decode_complete(NULL,
                                                        &asn_DEF_NR_HandoverPreparationInformation,
                                                        (void **)&hoPrepInformation,
                                                        source2target->rRCContainer.buf,
                                                        source2target->rRCContainer.size);
  AssertFatal(hoPrep_dec_rval.code == RC_OK && hoPrep_dec_rval.consumed > 0, "Handover Prep Info decode error\n");
  if (hoPrep_dec_rval.code != RC_OK && !hoPrep_dec_rval.consumed) {
    NGAP_ERROR("Failed to decode HandoverPreparationInformation, abort Handover Request decoding\n");
    free_ng_handover_request(out);
    ASN_STRUCT_FREE(asn_DEF_NGAP_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer, source2target);
    return -1;
  }

  ASN_STRUCT_FREE(asn_DEF_NGAP_SourceNGRANNode_ToTargetNGRANNode_TransparentContainer, source2target);

  if (LOG_DEBUGFLAG(DEBUG_ASN1))
    xer_fprint(stdout, &asn_DEF_NR_HandoverPreparationInformation, hoPrepInformation);

  // Decode UE capabilities and store
  NR_HandoverPreparationInformation_IEs_t *hpi =
      hoPrepInformation->criticalExtensions.choice.c1->choice.handoverPreparationInformation;
  const NR_UE_CapabilityRAT_ContainerList_t *ue_CapabilityRAT_ContainerList = &hpi->ue_CapabilityRAT_List;
  ssize_t encoded = uper_encode_to_new_buffer(&asn_DEF_NR_UE_CapabilityRAT_ContainerList,
                                              NULL,
                                              ue_CapabilityRAT_ContainerList,
                                              (void **)&out->ue_cap.buf);
  DevAssert(encoded > 0);
  out->ue_cap.len = encoded;

  ASN_STRUCT_FREE(asn_DEF_NR_HandoverPreparationInformation, hoPrepInformation);

  if (out->ue_cap.len <= 0) {
    free_ng_handover_request(out);
    NGAP_ERROR("could not encode UE-CapabilityRAT-ContainerList\n");
    return -1;
  }

  // PDU Session Resource Setup List (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverRequestIEs_t,
                             ie,
                             container,
                             NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListHOReq,
                             true);
  out->nb_of_pdusessions = ie->value.choice.PDUSessionResourceSetupListHOReq.list.count;
  for (int pduSesIdx = 0; pduSesIdx < out->nb_of_pdusessions; ++pduSesIdx) {
    NGAP_PDUSessionResourceSetupItemHOReq_t *item_p = ie->value.choice.PDUSessionResourceSetupListHOReq.list.array[pduSesIdx];
    // PDU Session ID (M)
    ho_request_pdusession_t *setup = &out->pduSessionResourceSetupList[pduSesIdx];
    setup->pdusession_id = item_p->pDUSessionID;
    setup->pdu_session_type = PDUSessionType_ipv4;
    // S-NSSAI (M)
    setup->nssai = decode_ngap_nssai(&item_p->s_NSSAI);
    // Handover Request Transfer (M)
    bool ret = decodePDUSessionResourceSetup(&setup->pdusessionTransfer, item_p->handoverRequestTransfer);
    if (!ret) {
      free_ng_handover_request(out);
      NGAP_ERROR("Failed to decode pDUSessionResourceSetupRequestTransfer in NG Initial Context Setup Request\n");
      return -1;
    }
  }
  return 0;
}

NGAP_NGAP_PDU_t *encode_ng_handover_request_ack(ngap_handover_request_ack_t *msg)
{
  NGAP_NGAP_PDU_t *pdu = calloc_or_fail(1, sizeof(*pdu));

  pdu->present = NGAP_NGAP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu->choice.successfulOutcome, head);
  head->procedureCode = NGAP_ProcedureCode_id_HandoverResourceAllocation;
  head->criticality = NGAP_Criticality_reject;
  head->value.present = NGAP_SuccessfulOutcome__value_PR_HandoverRequestAcknowledge;
  NGAP_HandoverRequestAcknowledge_t *out = &head->value.choice.HandoverRequestAcknowledge;

  // AMF UE NGAP ID (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequestAcknowledgeIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_HandoverRequestAcknowledgeIEs__value_PR_AMF_UE_NGAP_ID;
    asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, msg->amf_ue_ngap_id);
  }

  // RAN UE NGAP ID (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequestAcknowledgeIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_HandoverRequestAcknowledgeIEs__value_PR_RAN_UE_NGAP_ID;
    ie->value.choice.RAN_UE_NGAP_ID = msg->gNB_ue_ngap_id;
  }

  // Target to Source Transparent Container (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequestAcknowledgeIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_TargetToSource_TransparentContainer;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_HandoverRequestAcknowledgeIEs__value_PR_TargetToSource_TransparentContainer;
    NGAP_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer_t *t2s = calloc_or_fail(1, sizeof(*t2s));
    // RRC Container (M)
    {
      // Encode Handover Command
      t2s->rRCContainer.size = msg->target2source.len;
      t2s->rRCContainer.buf = malloc_or_fail(msg->target2source.len);
      memcpy(t2s->rRCContainer.buf, msg->target2source.buf, msg->target2source.len);
      byte_array_t ba = { .buf = NULL, .len = 0 };
      ba.len = aper_encode_to_new_buffer(&asn_DEF_NGAP_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer, NULL, t2s, (void **)&ba.buf);
      OCTET_STRING_fromBuf(&ie->value.choice.TargetToSource_TransparentContainer, (const char *)ba.buf, ba.len);
      ASN_STRUCT_FREE(asn_DEF_NGAP_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer, t2s);
      free_byte_array(ba);
    }
  }

  // PDU Session Resource Admitted List (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverRequestAcknowledgeIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceAdmittedList;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_HandoverRequestAcknowledgeIEs__value_PR_PDUSessionResourceAdmittedList;

    for (int pduSesIdx = 0; pduSesIdx < msg->nb_of_pdusessions; pduSesIdx++) {
      asn1cSequenceAdd(ie->value.choice.PDUSessionResourceAdmittedList.list, NGAP_PDUSessionResourceAdmittedItem_t, item);
      item->pDUSessionID = msg->pdusessions[pduSesIdx].pdu_session_id;
      /* dLQosFlowPerTNLInformation */
      NGAP_HandoverRequestAcknowledgeTransfer_t transfer = {0};
      transfer.dL_NGU_UP_TNLInformation.present = NGAP_UPTransportLayerInformation_PR_gTPTunnel;
      asn1cCalloc(transfer.dL_NGU_UP_TNLInformation.choice.gTPTunnel, tmp);
      GTP_TEID_TO_ASN1(msg->pdusessions[pduSesIdx].ack_transfer.gtp_teid, &tmp->gTP_TEID);
      tnl_to_bitstring(&tmp->transportLayerAddress, msg->pdusessions[pduSesIdx].ack_transfer.gNB_addr);

      for (int j = 0; j < msg->pdusessions[pduSesIdx].ack_transfer.nb_of_qos_flow; j++) {
        asn1cSequenceAdd(transfer.qosFlowSetupResponseList.list, NGAP_QosFlowItemWithDataForwarding_t, qosItem);
        qosItem->qosFlowIdentifier = msg->pdusessions[pduSesIdx].ack_transfer.qos_setup_list[j].qfi;
        qosItem->dataForwardingAccepted = calloc_or_fail(1, sizeof(*qosItem->dataForwardingAccepted));
        *qosItem->dataForwardingAccepted = NGAP_DataForwardingAccepted_data_forwarding_accepted;
      }

      void *buf;
      ssize_t encoded = aper_encode_to_new_buffer(&asn_DEF_NGAP_HandoverRequestAcknowledgeTransfer, NULL, &transfer, &buf);
      AssertFatal(encoded > 0, "ASN1 message encoding failed !\n");
      item->handoverRequestAcknowledgeTransfer.buf = buf;
      item->handoverRequestAcknowledgeTransfer.size = encoded;

      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_HandoverRequestAcknowledgeTransfer, &transfer);
    }
  }

  return pdu;
}

void free_ng_handover_req_ack(ngap_handover_request_ack_t *msg)
{
  free_byte_array(msg->target2source);
}

/** @brief Decoder for the NG Handover Command */
int decode_ng_handover_command(ngap_handover_command_t *msg, NGAP_NGAP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);
  NGAP_HandoverCommandIEs_t *ie;
  NGAP_HandoverCommand_t *container = &pdu->choice.successfulOutcome->value.choice.HandoverCommand;

  // AMF UE NGAP ID (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverCommandIEs_t, ie, container, NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID, true);
  asn_INTEGER2ulong(&(ie->value.choice.AMF_UE_NGAP_ID), &msg->amf_ue_ngap_id);

  // RAN UE NGAP ID (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverCommandIEs_t, ie, container, NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID, true);
  msg->gNB_ue_ngap_id = ie->value.choice.RAN_UE_NGAP_ID;

  // Handover Type (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverCommandIEs_t, ie, container, NGAP_ProtocolIE_ID_id_HandoverType, true);
  if (ie->value.choice.HandoverType != HANDOVER_TYPE_INTRA5GS) {
    NGAP_ERROR("Only Intra 5GS Handover is supported at the moment!\n");
    return -1;
  }

  // PDU Session Resource Handover List (O)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverCommandIEs_t, ie, container, NGAP_ProtocolIE_ID_id_PDUSessionResourceHandoverList, false);
  if (ie != NULL) {
    msg->nb_of_pdusessions = ie->value.choice.PDUSessionResourceHandoverList.list.count;

    for (int i = 0; i < msg->nb_of_pdusessions; ++i) {
      NGAP_PDUSessionResourceHandoverItem_t *item = ie->value.choice.PDUSessionResourceHandoverList.list.array[i];

      // PDU Session ID (M)
      msg->pdu_sessions[i].pdusession_id = item->pDUSessionID;

      // Handover Command Transfer (M)
      NGAP_HandoverCommandTransfer_t *hoCommandTransfer = NULL;
      asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                                                     &asn_DEF_NGAP_HandoverCommandTransfer,
                                                     (void **)&hoCommandTransfer,
                                                     item->handoverCommandTransfer.buf,
                                                     item->handoverCommandTransfer.size);
      if (dec_rval.code != RC_OK) {
        NGAP_ERROR("Failed to decode handoverCommandTransfer\n");
        continue;
      }

      // DL Forwarding UP TNL Information (O)
      if (hoCommandTransfer->dLForwardingUP_TNLInformation) {
        NGAP_UPTransportLayerInformation_t *up_tnl = hoCommandTransfer->dLForwardingUP_TNLInformation;
        if (hoCommandTransfer->dLForwardingUP_TNLInformation->present == NGAP_UPTransportLayerInformation_PR_gTPTunnel) {
          OCTET_STRING_TO_INT32(&(up_tnl->choice.gTPTunnel->gTP_TEID), msg->pdu_sessions[i].ho_command_transfer.gtp_teid);
          bitstring_to_tnl(&msg->pdu_sessions[i].ho_command_transfer.gNB_addr, up_tnl->choice.gTPTunnel->transportLayerAddress);
        } else {
          NGAP_WARN("Missing DL Forwarding UP TNL Information\n");
        }
      }
      ASN_STRUCT_FREE(asn_DEF_NGAP_HandoverCommandTransfer, hoCommandTransfer);
    }
  }

  // Target to Source Transparent Container (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverCommandIEs_t,
                             ie,
                             container,
                             NGAP_ProtocolIE_ID_id_TargetToSource_TransparentContainer,
                             true);
  NGAP_TargetToSource_TransparentContainer_t *target2source = &(ie->value.choice.TargetToSource_TransparentContainer);
  NGAP_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer_t *transparentContainer = NULL;
  asn_dec_rval_t dec_rval = aper_decode_complete(NULL,
                                                 &asn_DEF_NGAP_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer,
                                                 (void **)&transparentContainer,
                                                 target2source->buf,
                                                 target2source->size);
  if (dec_rval.code != RC_OK) {
    NGAP_ERROR("Failed to decode TargetToSource_TransparentContainer\n");
    return -1;
  }

  msg->handoverCommand = create_byte_array(transparentContainer->rRCContainer.size, transparentContainer->rRCContainer.buf);
  ASN_STRUCT_FREE(asn_DEF_NGAP_TargetNGRANNode_ToSourceNGRANNode_TransparentContainer, transparentContainer);

  return 0;
}

void free_ng_handover_command(ngap_handover_command_t *msg)
{
  free_byte_array(msg->handoverCommand);
}

NGAP_NGAP_PDU_t *encode_ng_handover_notify(const ngap_handover_notify_t *msg)
{
  NGAP_NGAP_PDU_t *pdu = calloc_or_fail(1, sizeof(*pdu));

  // Message Type (M)
  pdu->present = NGAP_NGAP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu->choice.initiatingMessage, head);
  head->procedureCode = NGAP_ProcedureCode_id_HandoverNotification;
  head->criticality = NGAP_Criticality_ignore;
  head->value.present = NGAP_InitiatingMessage__value_PR_HandoverNotify;
  NGAP_HandoverNotify_t *out = &head->value.choice.HandoverNotify;

  // AMF UE NGAP ID (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverNotifyIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_HandoverNotifyIEs__value_PR_AMF_UE_NGAP_ID;
    asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, msg->amf_ue_ngap_id);
  }

  // RAN UE NGAP ID (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverNotifyIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_HandoverNotifyIEs__value_PR_RAN_UE_NGAP_ID;
    ie->value.choice.RAN_UE_NGAP_ID = msg->gNB_ue_ngap_id;
  }

  // User Location Information (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverNotifyIEs_t, ie);
    ie->id = NGAP_ProtocolIE_ID_id_UserLocationInformation;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_HandoverNotifyIEs__value_PR_UserLocationInformation;
    ie->value.choice.UserLocationInformation.present = NGAP_UserLocationInformation_PR_userLocationInformationNR;
    asn1cCalloc(ie->value.choice.UserLocationInformation.choice.userLocationInformationNR, userinfo_nr_p);
    // NR User Location Information
    const target_ran_node_id_t *target = &msg->user_info.target_ng_ran;
    // CGI (M)
    MACRO_GNB_ID_TO_CELL_IDENTITY(target->targetgNBId, msg->user_info.nrCellIdentity, &userinfo_nr_p->nR_CGI.nRCellIdentity);
    MCC_MNC_TO_TBCD(target->plmn_identity.mcc,
                    target->plmn_identity.mnc,
                    target->plmn_identity.mnc_digit_length,
                    &userinfo_nr_p->nR_CGI.pLMNIdentity);
    // TAI (M)
    INT24_TO_OCTET_STRING(target->tac, &userinfo_nr_p->tAI.tAC);
    MCC_MNC_TO_PLMNID(target->plmn_identity.mcc,
                      target->plmn_identity.mnc,
                      target->plmn_identity.mnc_digit_length,
                      &userinfo_nr_p->tAI.pLMNIdentity);
  }

  return pdu;
}

/** @brief NGAP Handover Cancel encoding (9.2.3.11 3GPP TS 38.413 Handover Cancel)
 * 8.4.5 Handover Cancellation (source NG-RAN -> AMF) */
NGAP_NGAP_PDU_t *encode_ng_handover_cancel(const ngap_handover_cancel_t *msg)
{
  DevAssert(msg != NULL);

  NGAP_NGAP_PDU_t *pdu = calloc_or_fail(1, sizeof(*pdu));

  pdu->present = NGAP_NGAP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu->choice.initiatingMessage, head);
  head->procedureCode = NGAP_ProcedureCode_id_HandoverCancel;
  head->criticality = NGAP_Criticality_reject;
  head->value.present = NGAP_InitiatingMessage__value_PR_HandoverCancel;
  NGAP_HandoverCancel_t *out = &head->value.choice.HandoverCancel;

  // AMF UE NGAP ID (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverCancelIEs_t, ie1);
  ie1->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie1->criticality = NGAP_Criticality_reject;
  ie1->value.present = NGAP_HandoverCancelIEs__value_PR_AMF_UE_NGAP_ID;
  asn_uint642INTEGER(&ie1->value.choice.AMF_UE_NGAP_ID, msg->amf_ue_ngap_id);

  // RAN UE NGAP ID (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverCancelIEs_t, ie2);
  ie2->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie2->criticality = NGAP_Criticality_reject;
  ie2->value.present = NGAP_HandoverCancelIEs__value_PR_RAN_UE_NGAP_ID;
  ie2->value.choice.RAN_UE_NGAP_ID = msg->gNB_ue_ngap_id;

  // Cause (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_HandoverCancelIEs_t, ie3);
  ie3->id = NGAP_ProtocolIE_ID_id_Cause;
  ie3->criticality = NGAP_Criticality_reject;
  ie3->value.present = NGAP_HandoverCancelIEs__value_PR_Cause;
  encode_ngap_cause(&ie3->value.choice.Cause, &msg->cause);

  return pdu;
}

/** @brief Decode NGAP Handover Cancel Acknowledge (9.2.3.12 3GPP TS 38.413) */
int decode_ng_handover_cancel_ack(ngap_handover_cancel_ack_t *out, const NGAP_NGAP_PDU_t *pdu)
{
  DevAssert(out != NULL);
  DevAssert(pdu != NULL);

  NGAP_HandoverCancelAcknowledge_t *container = &pdu->choice.successfulOutcome->value.choice.HandoverCancelAcknowledge;
  NGAP_HandoverCancelAcknowledgeIEs_t *ie;

  /* AMF UE NGAP ID (M) */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverCancelAcknowledgeIEs_t, ie, container, NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID, true);
  asn_INTEGER2ulong(&ie->value.choice.AMF_UE_NGAP_ID, &out->amf_ue_ngap_id);

  /* RAN UE NGAP ID (M) */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_HandoverCancelAcknowledgeIEs_t, ie, container, NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID, true);
  out->gNB_ue_ngap_id = ie->value.choice.RAN_UE_NGAP_ID;

  return 0;
}

/** @brief Encode NGAP UL RAN Status Transfer (9.2.3.14 of 3GPP TS 38.413) */
NGAP_NGAP_PDU_t *encode_ng_ul_ran_status_transfer(const ngap_ran_status_transfer_t *msg)
{
  NGAP_NGAP_PDU_t *pdu = calloc_or_fail(1, sizeof(*pdu));
  pdu->present = NGAP_NGAP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu->choice.initiatingMessage, head);
  head->procedureCode = NGAP_ProcedureCode_id_UplinkRANStatusTransfer;
  head->criticality = NGAP_Criticality_ignore;
  head->value.present = NGAP_InitiatingMessage__value_PR_UplinkRANStatusTransfer;
  NGAP_UplinkRANStatusTransfer_t *out = &head->value.choice.UplinkRANStatusTransfer;

  // AMF UE NGAP ID (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_UplinkRANStatusTransferIEs_t, ie1);
  ie1->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie1->criticality = NGAP_Criticality_reject;
  ie1->value.present = NGAP_UplinkRANStatusTransferIEs__value_PR_AMF_UE_NGAP_ID;
  asn_uint642INTEGER(&ie1->value.choice.AMF_UE_NGAP_ID, msg->amf_ue_ngap_id);

  // RAN UE NGAP ID (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_UplinkRANStatusTransferIEs_t, ie2);
  ie2->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie2->criticality = NGAP_Criticality_reject;
  ie2->value.present = NGAP_UplinkRANStatusTransferIEs__value_PR_RAN_UE_NGAP_ID;
  ie2->value.choice.RAN_UE_NGAP_ID = msg->gnb_ue_ngap_id;

  // RAN Status Transfer Transparent Container (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_UplinkRANStatusTransferIEs_t, ie3);
  ie3->id = NGAP_ProtocolIE_ID_id_RANStatusTransfer_TransparentContainer;
  ie3->criticality = NGAP_Criticality_reject;
  ie3->value.present = NGAP_UplinkRANStatusTransferIEs__value_PR_RANStatusTransfer_TransparentContainer;
  NGAP_RANStatusTransfer_TransparentContainer_t *container = &ie3->value.choice.RANStatusTransfer_TransparentContainer;

  for (int i = 0; i < msg->ran_status.nb_drb; ++i) {
    const ngap_drb_status_t *s = &msg->ran_status.drb_status_list[i];
    asn1cSequenceAdd(container->dRBsSubjectToStatusTransferList.list, NGAP_DRBsSubjectToStatusTransferItem_t, drb_item);
    drb_item->dRB_ID = s->drb_id;

    // UL COUNT
    if (s->ul_count.sn_len == NGAP_SN_LENGTH_18) {
      drb_item->dRBStatusUL.present = NGAP_DRBStatusUL_PR_dRBStatusUL18;
      asn1cCalloc(drb_item->dRBStatusUL.choice.dRBStatusUL18, ul18);
      ul18->uL_COUNTValue.pDCP_SN18 = s->ul_count.pdcp_sn;
      ul18->uL_COUNTValue.hFN_PDCP_SN18 = s->ul_count.hfn;
    } else {
      drb_item->dRBStatusUL.present = NGAP_DRBStatusUL_PR_dRBStatusUL12;
      asn1cCalloc(drb_item->dRBStatusUL.choice.dRBStatusUL12, ul12);
      ul12->uL_COUNTValue.pDCP_SN12 = s->ul_count.pdcp_sn;
      ul12->uL_COUNTValue.hFN_PDCP_SN12 = s->ul_count.hfn;
    }

    // DL COUNT
    if (s->dl_count.sn_len == NGAP_SN_LENGTH_18) {
      drb_item->dRBStatusDL.present = NGAP_DRBStatusDL_PR_dRBStatusDL18;
      asn1cCalloc(drb_item->dRBStatusDL.choice.dRBStatusDL18, dl18);
      dl18->dL_COUNTValue.pDCP_SN18 = s->dl_count.pdcp_sn;
      dl18->dL_COUNTValue.hFN_PDCP_SN18 = s->dl_count.hfn;
    } else {
      drb_item->dRBStatusDL.present = NGAP_DRBStatusDL_PR_dRBStatusDL12;
      asn1cCalloc(drb_item->dRBStatusDL.choice.dRBStatusDL12, dl12);
      dl12->dL_COUNTValue.pDCP_SN12 = s->dl_count.pdcp_sn;
      dl12->dL_COUNTValue.hFN_PDCP_SN12 = s->dl_count.hfn;
    }
  }

  if (LOG_DEBUGFLAG(DEBUG_ASN1))
    xer_fprint(stdout, &asn_DEF_NGAP_RANStatusTransfer_TransparentContainer, &container);

  return pdu;
}

/** @brief Encode NGAP Path Switch Request (9.2.3.8 3GPP TS 38.413) NG-RAN node -> AMF */
NGAP_NGAP_PDU_t *encode_ng_path_switch_request(const ngap_path_switch_req_t *msg)
{
  NGAP_NGAP_PDU_t *pdu = calloc_or_fail(1,sizeof(*pdu));

  /* Prepare the NGAP message to encode */
  pdu->present = NGAP_NGAP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu->choice.initiatingMessage, head);
  head->procedureCode = NGAP_ProcedureCode_id_PathSwitchRequest;
  head->criticality = NGAP_Criticality_reject;
  head->value.present = NGAP_InitiatingMessage__value_PR_PathSwitchRequest;
  NGAP_PathSwitchRequest_t *out = &head->value.choice.PathSwitchRequest;

  // RAN UE NGAP ID (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_PathSwitchRequestIEs_t, ie1);
  ie1->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie1->criticality = NGAP_Criticality_reject;
  ie1->value.present = NGAP_PathSwitchRequestIEs__value_PR_RAN_UE_NGAP_ID;
  ie1->value.choice.RAN_UE_NGAP_ID = msg->gNB_ue_ngap_id;

  // Source AMF UE NGAP ID (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_PathSwitchRequestIEs_t, ie2);
  ie2->id = NGAP_ProtocolIE_ID_id_SourceAMF_UE_NGAP_ID;
  ie2->criticality = NGAP_Criticality_reject;
  ie2->value.present = NGAP_PathSwitchRequestIEs__value_PR_AMF_UE_NGAP_ID;
  asn_uint642INTEGER(&ie2->value.choice.AMF_UE_NGAP_ID, msg->amf_ue_ngap_id);

  // User Location Information (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PathSwitchRequestIEs_t, ie3);
    ie3->id = NGAP_ProtocolIE_ID_id_UserLocationInformation;
    ie3->criticality = NGAP_Criticality_ignore;
    ie3->value.present = NGAP_PathSwitchRequestIEs__value_PR_UserLocationInformation;
    ie3->value.choice.UserLocationInformation.present = NGAP_UserLocationInformation_PR_userLocationInformationNR;
    asn1cCalloc(ie3->value.choice.UserLocationInformation.choice.userLocationInformationNR, userinfo_nr_p);

    // NR user location information
    const target_ran_node_id_t *target = &msg->user_info.target_ng_ran;
    // CGI (M)
    MACRO_GNB_ID_TO_CELL_IDENTITY(target->targetgNBId, msg->user_info.nrCellIdentity, &userinfo_nr_p->nR_CGI.nRCellIdentity);
    MCC_MNC_TO_TBCD(target->plmn_identity.mcc,
                    target->plmn_identity.mnc,
                    target->plmn_identity.mnc_digit_length,
                    &userinfo_nr_p->nR_CGI.pLMNIdentity);
    // TAI (M)
    INT24_TO_OCTET_STRING(target->tac, &userinfo_nr_p->tAI.tAC);
    MCC_MNC_TO_PLMNID(target->plmn_identity.mcc,
                      target->plmn_identity.mnc,
                      target->plmn_identity.mnc_digit_length,
                      &userinfo_nr_p->tAI.pLMNIdentity);
  }

  // UE Security Capabilities (M)
  asn1cSequenceAdd(out->protocolIEs.list, NGAP_PathSwitchRequestIEs_t, ie4);
  ie4->id = NGAP_ProtocolIE_ID_id_UESecurityCapabilities;
  ie4->criticality = NGAP_Criticality_ignore;
  ie4->value.present = NGAP_PathSwitchRequestIEs__value_PR_UESecurityCapabilities;
  encode_ngap_security_capabilities(&ie4->value.choice.UESecurityCapabilities, &msg->security_capabilities);

  // PDU Session Resource to be Switched in Downlink List (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NGAP_PathSwitchRequestIEs_t, ie5);
    ie5->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceToBeSwitchedDLList;
    ie5->criticality = NGAP_Criticality_reject;
    ie5->value.present = NGAP_PathSwitchRequestIEs__value_PR_PDUSessionResourceToBeSwitchedDLList;

    for (const pdusession_setup_t *pdu = msg->pdusessions_tobeswitched; pdu < msg->pdusessions_tobeswitched + msg->nb_of_pdusessions; pdu++) {
      asn1cSequenceAdd(ie5->value.choice.PDUSessionResourceToBeSwitchedDLList.list, NGAP_PDUSessionResourceToBeSwitchedDLItem_t, item);
      item->pDUSessionID = pdu->pdusession_id;
      /* dLQoSFlowPerTNLInformation */
      NGAP_PathSwitchRequestTransfer_t transfer = {0};
      transfer.dL_NGU_UP_TNLInformation.present = NGAP_UPTransportLayerInformation_PR_gTPTunnel;
      asn1cCalloc(transfer.dL_NGU_UP_TNLInformation.choice.gTPTunnel, tmp);
      GTP_TEID_TO_ASN1(pdu->n3_outgoing.teid, &tmp->gTP_TEID);
      tnl_to_bitstring(&tmp->transportLayerAddress, pdu->n3_outgoing.addr);
      for (int j = 0; j < pdu->nb_of_qos_flow; j++) {
        asn1cSequenceAdd(transfer.qosFlowAcceptedList.list, NGAP_QosFlowAcceptedItem_t, qosItem);
        qosItem->qosFlowIdentifier = pdu->associated_qos_flows[j].qfi;
      }

      void *buf;
      ssize_t encoded = aper_encode_to_new_buffer(&asn_DEF_NGAP_PathSwitchRequestTransfer, NULL, &transfer, &buf);
      AssertFatal(encoded > 0, "ASN1 message encoding failed !\n");
      item->pathSwitchRequestTransfer.buf = buf;
      item->pathSwitchRequestTransfer.size = encoded;

      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PathSwitchRequestTransfer, &transfer);
    }
  }
  return pdu;
}

void free_ng_path_switch_req_ack(ngap_path_switch_req_ack_t *msg)
{
  if (msg == NULL)
    return;

  for (int i = 0; i < msg->nb_of_pdusessions; i++) {
    path_switch_request_ack_transfer_t *transfer = &msg->pdusessions_switched[i].pathSwitchReqAckTransfer;

    free(transfer->n3_incoming);
    transfer->n3_incoming = NULL;

    free(transfer->security_ind);
    transfer->security_ind = NULL;
  }
}

/** @brief Decode NGAP Path Switch Request Acknowledge (9.2.3.9 3GPP TS 38.413) */
int decode_ng_path_switch_request_acknowledge(ngap_path_switch_req_ack_t *msg, NGAP_NGAP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);
  NGAP_PathSwitchRequestAcknowledgeIEs_t *ie;
  NGAP_PathSwitchRequestAcknowledge_t *container = &pdu->choice.successfulOutcome->value.choice.PathSwitchRequestAcknowledge;

  // AMF UE NGAP ID (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PathSwitchRequestAcknowledgeIEs_t, ie, container, NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID, true);
  asn_INTEGER2ulong(&(ie->value.choice.AMF_UE_NGAP_ID), &msg->amf_ue_ngap_id);

  // RAN UE NGAP ID (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PathSwitchRequestAcknowledgeIEs_t, ie, container, NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID, true);
  msg->gNB_ue_ngap_id = ie->value.choice.RAN_UE_NGAP_ID;

  // Security Context (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PathSwitchRequestAcknowledgeIEs_t, ie, container, NGAP_ProtocolIE_ID_id_SecurityContext, true);
  msg->nh_ncc = ie->value.choice.SecurityContext.nextHopChainingCount;
  memcpy(msg->next_security_key, ie->value.choice.SecurityContext.nextHopNH.buf, ie->value.choice.SecurityContext.nextHopNH.size);

  // PDU Session Resource Switched List (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PathSwitchRequestAcknowledgeIEs_t, ie, container, NGAP_ProtocolIE_ID_id_PDUSessionResourceSwitchedList, true);
  msg->nb_of_pdusessions = ie->value.choice.PDUSessionResourceSwitchedList.list.count;

  for (int i = 0; i < msg->nb_of_pdusessions; ++i) {
    NGAP_PDUSessionResourceSwitchedItem_t *item = ie->value.choice.PDUSessionResourceSwitchedList.list.array[i];
    path_switch_request_ack_pdusession_t *switched = &msg->pdusessions_switched[i];
    // PDU Session ID (M)
    switched->pdusession_id = item->pDUSessionID;
    // Path Switch Request Acknowledge Transfer (M)
    NGAP_PathSwitchRequestAcknowledgeTransfer_t *psrAckTransfer = NULL;
    asn_dec_rval_t dec_rval = uper_decode_complete(NULL,
                                                   &asn_DEF_NGAP_PathSwitchRequestAcknowledgeTransfer,
                                                   (void **)&psrAckTransfer,
                                                   item->pathSwitchRequestAcknowledgeTransfer.buf,
                                                   item->pathSwitchRequestAcknowledgeTransfer.size);
    if (dec_rval.code != RC_OK) {
      NGAP_ERROR("Failed to decode Path Switch Request Acknowledge Transfer\n");
      ASN_STRUCT_FREE(asn_DEF_NGAP_PathSwitchRequestAcknowledgeTransfer, psrAckTransfer);
      return -1;
    }

    // UL NG-U UP TNL Information (O)
    if (psrAckTransfer->uL_NGU_UP_TNLInformation != NULL) {
      NGAP_UPTransportLayerInformation_t *up_tnl = psrAckTransfer->uL_NGU_UP_TNLInformation;
      if (up_tnl->present == NGAP_UPTransportLayerInformation_PR_gTPTunnel && up_tnl->choice.gTPTunnel != NULL) {
        switched->pathSwitchReqAckTransfer.n3_incoming = calloc_or_fail(1, sizeof(*switched->pathSwitchReqAckTransfer.n3_incoming));
        OCTET_STRING_TO_INT32(&(up_tnl->choice.gTPTunnel->gTP_TEID), switched->pathSwitchReqAckTransfer.n3_incoming->teid);
        bitstring_to_tnl(&switched->pathSwitchReqAckTransfer.n3_incoming->addr, up_tnl->choice.gTPTunnel->transportLayerAddress);
      }
    }

    // Security Indication (O)
    if (psrAckTransfer->securityIndication != NULL) {
      switched->pathSwitchReqAckTransfer.security_ind = calloc_or_fail(1, sizeof(*switched->pathSwitchReqAckTransfer.security_ind));
      switched->pathSwitchReqAckTransfer.security_ind->integrity_protection_ind = psrAckTransfer->securityIndication->integrityProtectionIndication;
      switched->pathSwitchReqAckTransfer.security_ind->confidentiality_protection_ind = psrAckTransfer->securityIndication->confidentialityProtectionIndication;
    }

    ASN_STRUCT_FREE(asn_DEF_NGAP_PathSwitchRequestAcknowledgeTransfer, psrAckTransfer);
  }

  // Allowed NSSAI (M)
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PathSwitchRequestAcknowledgeIEs_t, ie, container, NGAP_ProtocolIE_ID_id_AllowedNSSAI, true);
  NGAP_DEBUG("AllowedNSSAI.list.count %d\n", ie->value.choice.AllowedNSSAI.list.count);
  msg->nb_allowed_nssais = ie->value.choice.AllowedNSSAI.list.count;
  for (int i = 0; i < msg->nb_allowed_nssais; ++i) {
    msg->allowed_nssai[i] = decode_ngap_nssai(&ie->value.choice.AllowedNSSAI.list.array[i]->s_NSSAI);
  }

  return 0;
}
