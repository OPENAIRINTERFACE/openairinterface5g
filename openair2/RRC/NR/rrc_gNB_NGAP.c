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

/*! \file rrc_gNB_NGAP.h
 * \brief rrc NGAP procedures for gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \version 0.1
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 *         (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com) 
 */

#include "rrc_gNB_NGAP.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "rrc_eNB_S1AP.h"
#include "gnb_config.h"
#include "common/ran_context.h"

#include "oai_asn1.h"
#include "intertask_interface.h"
#include "nr_pdcp/nr_pdcp_oai_api.h"
#include "pdcp_primitives.h"
#include "SDAP/nr_sdap/nr_sdap.h"

#include "openair3/ocp-gtpu/gtp_itf.h"
#include <openair3/ocp-gtpu/gtp_itf.h>
#include "RRC/LTE/rrc_eNB_GTPV1U.h"
#include "RRC/NR/rrc_gNB_GTPV1U.h"

#include "S1AP_NAS-PDU.h"
#include "executables/softmodem-common.h"
#include "openair3/SECU/key_nas_deriver.h"

#include "ngap_gNB_defs.h"
#include "ngap_gNB_ue_context.h"
#include "ngap_gNB_management_procedures.h"
#include "NR_ULInformationTransfer.h"
#include "RRC/NR/MESSAGES/asn1_msg.h"
#include "NR_UERadioAccessCapabilityInformation.h"
#include "NR_UE-CapabilityRAT-ContainerList.h"
#include "NGAP_CauseRadioNetwork.h"
#include "f1ap_messages_types.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "openair2/E1AP/e1ap_asnc.h"
#include "NGAP_asn_constant.h"
#include "NGAP_PDUSessionResourceSetupRequestTransfer.h"
#include "NGAP_PDUSessionResourceModifyRequestTransfer.h"
#include "NGAP_ProtocolIE-Field.h"
#include "NGAP_GTPTunnel.h"
#include "NGAP_QosFlowSetupRequestItem.h"
#include "NGAP_QosFlowAddOrModifyRequestItem.h"
#include "NGAP_NonDynamic5QIDescriptor.h"
#include "NGAP_Dynamic5QIDescriptor.h"
#include "conversions.h"
#include "RRC/NR/rrc_gNB_radio_bearers.h"

#include "uper_encoder.h"

extern RAN_CONTEXT_t RC;

/* Masks for NGAP Encryption algorithms, NEA0 is always supported (not coded) */
static const uint16_t NGAP_ENCRYPTION_NEA1_MASK = 0x8000;
static const uint16_t NGAP_ENCRYPTION_NEA2_MASK = 0x4000;
static const uint16_t NGAP_ENCRYPTION_NEA3_MASK = 0x2000;

/* Masks for NGAP Integrity algorithms, NIA0 is always supported (not coded) */
static const uint16_t NGAP_INTEGRITY_NIA1_MASK = 0x8000;
static const uint16_t NGAP_INTEGRITY_NIA2_MASK = 0x4000;
static const uint16_t NGAP_INTEGRITY_NIA3_MASK = 0x2000;

#define INTEGRITY_ALGORITHM_NONE NR_IntegrityProtAlgorithm_nia0

static int rrc_gNB_process_security(const protocol_ctxt_t *const ctxt_pP, rrc_gNB_ue_context_t *const ue_context_pP, ngap_security_capabilities_t *security_capabilities_pP);

/*! \fn void process_gNB_security_key (const protocol_ctxt_t* const ctxt_pP, eNB_RRC_UE_t * const ue_context_pP, uint8_t *security_key)
 *\brief save security key.
 *\param ctxt_pP         Running context.
 *\param ue_context_pP   UE context.
 *\param security_key_pP The security key received from NGAP.
 */
//------------------------------------------------------------------------------
void process_gNB_security_key (
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t  *const ue_context_pP,
  uint8_t               *security_key_pP
)
//------------------------------------------------------------------------------
{
  char ascii_buffer[65];
  uint8_t i;
  gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;

  /* Saves the security key */
  memcpy(UE->kgnb, security_key_pP, SECURITY_KEY_LENGTH);
  memset(UE->nh, 0, SECURITY_KEY_LENGTH);
  UE->nh_ncc = -1;

  for (i = 0; i < 32; i++) {
    sprintf(&ascii_buffer[2 * i], "%02X", UE->kgnb[i]);
  }

  ascii_buffer[2 * i] = '\0';
  LOG_I(NR_RRC, "[gNB %d][UE %x] Saved security key %s\n", ctxt_pP->module_id, UE->rnti, ascii_buffer);
}

//------------------------------------------------------------------------------
void
nr_rrc_pdcp_config_security(
    const protocol_ctxt_t  *const ctxt_pP,
    rrc_gNB_ue_context_t   *const ue_context_pP,
    const uint8_t          enable_ciphering
)
//------------------------------------------------------------------------------
{
  uint8_t kRRCenc[16] = {0};
  uint8_t kRRCint[16] = {0};
  uint8_t kUPenc[16] = {0};
  //uint8_t                            *k_kdf  = NULL;
  static int                          print_keys= 1;
  gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;

  /* Derive the keys from kgnb */
  if (UE->Srb[1].Active || UE->Srb[2].Active) {
    nr_derive_key(UP_ENC_ALG, UE->ciphering_algorithm, UE->kgnb, kUPenc);
  }

  nr_derive_key(RRC_ENC_ALG, UE->ciphering_algorithm, UE->kgnb, kRRCenc);
  nr_derive_key(RRC_INT_ALG, UE->integrity_algorithm, UE->kgnb, kRRCint);

  if ( LOG_DUMPFLAG( DEBUG_SECURITY ) ) {
    if (print_keys == 1 ) {
      print_keys =0;
      LOG_DUMPMSG(NR_RRC, DEBUG_SECURITY, UE->kgnb, 32, "\nKgNB:");
      LOG_DUMPMSG(NR_RRC, DEBUG_SECURITY, kRRCenc, 16,"\nKRRCenc:" );
      LOG_DUMPMSG(NR_RRC, DEBUG_SECURITY, kRRCint, 16,"\nKRRCint:" );
    }
  }

  uint8_t security_mode =
      enable_ciphering ? UE->ciphering_algorithm | (UE->integrity_algorithm << 4) : 0 | (UE->integrity_algorithm << 4);
  nr_pdcp_config_set_security(ctxt_pP->rntiMaybeUEid, DCCH, security_mode, kRRCenc, kRRCint, kUPenc);
}

//------------------------------------------------------------------------------
/*
* Initial UE NAS message on S1AP.
*/
void
rrc_gNB_send_NGAP_NAS_FIRST_REQ(
    const protocol_ctxt_t     *const ctxt_pP,
    rrc_gNB_ue_context_t      *ue_context_pP,
    NR_RRCSetupComplete_IEs_t *rrcSetupComplete
)
//------------------------------------------------------------------------------
{
  // gNB_RRC_INST *rrc = RC.nrrrc[ctxt_pP->module_id];
  MessageDef         *message_p         = NULL;
  gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;

  message_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_NAS_FIRST_REQ);
  ngap_nas_first_req_t *req = &NGAP_NAS_FIRST_REQ(message_p);
  memset(req, 0, sizeof(*req));

  req->gNB_ue_ngap_id = UE->rrc_ue_id;

  /* Assume that cause is coded in the same way in RRC and NGap, just check that the value is in NGap range */
  AssertFatal(UE->establishment_cause < NGAP_RRC_CAUSE_LAST, "Establishment cause invalid (%jd/%d) for gNB %d!", UE->establishment_cause, NGAP_RRC_CAUSE_LAST, ctxt_pP->module_id);
  req->establishment_cause = UE->establishment_cause;

  /* Forward NAS message */
  req->nas_pdu.length = rrcSetupComplete->dedicatedNAS_Message.size;
  req->nas_pdu.buffer = malloc(req->nas_pdu.length);
  AssertFatal(req->nas_pdu.buffer != NULL, "out of memory\n");
  memcpy(req->nas_pdu.buffer, rrcSetupComplete->dedicatedNAS_Message.buf, req->nas_pdu.length);
  // extract_imsi(NGAP_NAS_FIRST_REQ (message_p).nas_pdu.buffer,
  //              NGAP_NAS_FIRST_REQ (message_p).nas_pdu.length,
  //              ue_context_pP);

  /* Fill UE identities with available information */
  req->ue_identity.presenceMask = NGAP_UE_IDENTITIES_NONE;
  if (UE->Initialue_identity_5g_s_TMSI.presence) {
    /* Fill s-TMSI */
    req->ue_identity.presenceMask = NGAP_UE_IDENTITIES_FiveG_s_tmsi;
    req->ue_identity.s_tmsi.amf_set_id = UE->Initialue_identity_5g_s_TMSI.amf_set_id;
    req->ue_identity.s_tmsi.amf_pointer = UE->Initialue_identity_5g_s_TMSI.amf_pointer;
    req->ue_identity.s_tmsi.m_tmsi = UE->Initialue_identity_5g_s_TMSI.fiveg_tmsi;
  }

  /* selected_plmn_identity: IE is 1-based, convert to 0-based (C array) */
  int selected_plmn_identity = rrcSetupComplete->selectedPLMN_Identity - 1;
  req->selected_plmn_identity = selected_plmn_identity;

  if (rrcSetupComplete->registeredAMF != NULL) {
      NR_RegisteredAMF_t *r_amf = rrcSetupComplete->registeredAMF;
      req->ue_identity.presenceMask |= NGAP_UE_IDENTITIES_guami;

      if (r_amf->plmn_Identity != NULL) {
          if ((r_amf->plmn_Identity->mcc != NULL) && (r_amf->plmn_Identity->mcc->list.count > 0)) {
              /* Use first indicated PLMN MCC if it is defined */
              req->ue_identity.guami.mcc = *r_amf->plmn_Identity->mcc->list.array[selected_plmn_identity];
              LOG_I(NGAP, "[gNB %d] Build NGAP_NAS_FIRST_REQ adding in s_TMSI: GUMMEI MCC %u ue %x\n", ctxt_pP->module_id, req->ue_identity.guami.mcc, UE->rnti);
          }

          if (r_amf->plmn_Identity->mnc.list.count > 0) {
              /* Use first indicated PLMN MNC if it is defined */
              req->ue_identity.guami.mnc = *r_amf->plmn_Identity->mnc.list.array[selected_plmn_identity];
              LOG_I(NGAP, "[gNB %d] Build NGAP_NAS_FIRST_REQ adding in s_TMSI: GUMMEI MNC %u ue %x\n", ctxt_pP->module_id, req->ue_identity.guami.mnc, UE->rnti);
          }
      } else {
          /* TODO */
      }

      /* amf_Identifier */
      uint32_t amf_Id = BIT_STRING_to_uint32(&r_amf->amf_Identifier);
      req->ue_identity.guami.amf_region_id = amf_Id >> 16;
      req->ue_identity.guami.amf_set_id = UE->Initialue_identity_5g_s_TMSI.amf_set_id;
      req->ue_identity.guami.amf_pointer = UE->Initialue_identity_5g_s_TMSI.amf_pointer;

      // fixme: illogical place to set UE values, should be in the function that call this one
      UE->ue_guami.mcc = req->ue_identity.guami.mcc;
      UE->ue_guami.mnc = req->ue_identity.guami.mnc;
      UE->ue_guami.mnc_len = req->ue_identity.guami.mnc_len;
      UE->ue_guami.amf_region_id = req->ue_identity.guami.amf_region_id;
      UE->ue_guami.amf_set_id = req->ue_identity.guami.amf_set_id;
      UE->ue_guami.amf_pointer = req->ue_identity.guami.amf_pointer;

      LOG_I(NGAP,
            "[gNB %d] Build NGAP_NAS_FIRST_REQ adding in s_TMSI: GUAMI amf_set_id %u amf_region_id %u ue %x\n",
            ctxt_pP->module_id,
            req->ue_identity.guami.amf_set_id,
            req->ue_identity.guami.amf_region_id,
            UE->rnti);
  }

  itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, message_p);
}

static void fill_qos(NGAP_QosFlowSetupRequestList_t *qos, pdusession_t *session)
{
  DevAssert(qos->list.count > 0);
  DevAssert(qos->list.count <= NGAP_maxnoofQosFlows);
  for (int qosIdx = 0; qosIdx < qos->list.count; qosIdx++) {
    NGAP_QosFlowSetupRequestItem_t *qosFlowItem_p = qos->list.array[qosIdx];
    // Set the QOS informations
    session->qos[qosIdx].qfi = (uint8_t)qosFlowItem_p->qosFlowIdentifier;
    NGAP_QosCharacteristics_t *qosChar = &qosFlowItem_p->qosFlowLevelQosParameters.qosCharacteristics;
    AssertFatal(qosChar, "Qos characteristics are not available for qos flow index %d\n", qosIdx);
    if (qosChar->present == NGAP_QosCharacteristics_PR_nonDynamic5QI) {
      AssertFatal(qosChar->choice.dynamic5QI, "Non-Dynamic 5QI is NULL\n");
      session->qos[qosIdx].fiveQI_type = non_dynamic;
      session->qos[qosIdx].fiveQI = (uint64_t)qosChar->choice.nonDynamic5QI->fiveQI;
    } else {
      AssertFatal(qosChar->choice.dynamic5QI, "Dynamic 5QI is NULL\n");
      session->qos[qosIdx].fiveQI_type = dynamic;
      session->qos[qosIdx].fiveQI = (uint64_t)(*qosChar->choice.dynamic5QI->fiveQI);
    }

    ngap_allocation_retention_priority_t *tmp = &session->qos[qosIdx].allocation_retention_priority;
    NGAP_AllocationAndRetentionPriority_t *tmp2 = &qosFlowItem_p->qosFlowLevelQosParameters.allocationAndRetentionPriority;
    tmp->priority_level = tmp2->priorityLevelARP;
    tmp->pre_emp_capability = tmp2->pre_emptionCapability;
    tmp->pre_emp_vulnerability = tmp2->pre_emptionVulnerability;
  }
  session->nb_qos = qos->list.count;
}

static int decodePDUSessionResourceSetup(pdusession_t *session)
{
  NGAP_PDUSessionResourceSetupRequestTransfer_t *pdusessionTransfer = NULL;
  asn_codec_ctx_t st = {.max_stack_size = 100 * 1000};
  asn_dec_rval_t dec_rval =
      aper_decode(&st, &asn_DEF_NGAP_PDUSessionResourceSetupRequestTransfer, (void **)&pdusessionTransfer, session->pdusessionTransfer.buffer, session->pdusessionTransfer.length, 0, 0);

  if (dec_rval.code != RC_OK) {
    LOG_E(NR_RRC, "can not decode PDUSessionResourceSetupRequestTransfer\n");
    return -1;
  }

  for (int i = 0; i < pdusessionTransfer->protocolIEs.list.count; i++) {
    NGAP_PDUSessionResourceSetupRequestTransferIEs_t *pdusessionTransfer_ies = pdusessionTransfer->protocolIEs.list.array[i];
    switch (pdusessionTransfer_ies->id) {
        /* optional PDUSessionAggregateMaximumBitRate */
      case NGAP_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate:
        break;

        /* mandatory UL-NGU-UP-TNLInformation */
      case NGAP_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation: {
        NGAP_GTPTunnel_t *gTPTunnel_p = pdusessionTransfer_ies->value.choice.UPTransportLayerInformation.choice.gTPTunnel;

        /* Set the transport layer address */
        memcpy(session->upf_addr.buffer, gTPTunnel_p->transportLayerAddress.buf, gTPTunnel_p->transportLayerAddress.size);

        session->upf_addr.length = gTPTunnel_p->transportLayerAddress.size * 8 - gTPTunnel_p->transportLayerAddress.bits_unused;

        /* GTP tunnel endpoint ID */
        OCTET_STRING_TO_INT32(&gTPTunnel_p->gTP_TEID, session->gtp_teid);
      }

      break;

        /* optional AdditionalUL-NGU-UP-TNLInformation */
      case NGAP_ProtocolIE_ID_id_AdditionalUL_NGU_UP_TNLInformation:
        break;

        /* optional DataForwardingNotPossible */
      case NGAP_ProtocolIE_ID_id_DataForwardingNotPossible:
        break;

        /* mandatory PDUSessionType */
      case NGAP_ProtocolIE_ID_id_PDUSessionType:
        session->pdu_session_type = (uint8_t)pdusessionTransfer_ies->value.choice.PDUSessionType;
        AssertFatal(session->pdu_session_type == PDUSessionType_ipv4 || session->pdu_session_type == PDUSessionType_ipv4v6, "To be developped: support not IPv4 sessions\n");
        break;

        /* optional SecurityIndication */
      case NGAP_ProtocolIE_ID_id_SecurityIndication:
        break;

        /* optional NetworkInstance */
      case NGAP_ProtocolIE_ID_id_NetworkInstance:
        break;

        /* mandatory QosFlowSetupRequestList */
      case NGAP_ProtocolIE_ID_id_QosFlowSetupRequestList:
        fill_qos(&pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList, session);
        break;

        /* optional CommonNetworkInstance */
      case NGAP_ProtocolIE_ID_id_CommonNetworkInstance:
        break;

      default:
        LOG_E(NR_RRC, "could not found protocolIEs id %ld\n", pdusessionTransfer_ies->id);
        return -1;
    }
  }
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceSetupRequestTransfer,pdusessionTransfer );

  return 0;
}

static void trigger_bearer_setup(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, int n, pdusession_t *sessions, uint64_t ueAggMaxBitRateDownlink)
{
  e1ap_bearer_setup_req_t bearer_req = {0};

  e1ap_nssai_t cuup_nssai = {0};
  for (int i = 0; i < n; i++) {
    rrc_pdu_session_param_t *pduSession = find_pduSession(UE, sessions[i].pdusession_id, true);
    pdusession_t *session = &pduSession->param;
    session->pdusession_id = sessions[i].pdusession_id;
    LOG_I(NR_RRC, "Adding pdusession %d, total nb of sessions %d\n", session->pdusession_id, UE->nb_of_pdusessions);
    session->pdu_session_type = sessions[i].pdu_session_type;
    session->nas_pdu = sessions[i].nas_pdu;
    session->pdusessionTransfer = sessions[i].pdusessionTransfer;
    session->nssai = sessions[i].nssai;
    decodePDUSessionResourceSetup(session);
    bearer_req.gNB_cu_cp_ue_id = UE->rrc_ue_id;
    bearer_req.cipheringAlgorithm = UE->ciphering_algorithm;
    bearer_req.integrityProtectionAlgorithm = UE->integrity_algorithm;
    nr_derive_key(UP_ENC_ALG, UE->ciphering_algorithm, UE->kgnb, (uint8_t *)bearer_req.encryptionKey);
    nr_derive_key(UP_INT_ALG, UE->integrity_algorithm, UE->kgnb, (uint8_t *)bearer_req.integrityProtectionKey);
    bearer_req.ueDlAggMaxBitRate = ueAggMaxBitRateDownlink;
    pdu_session_to_setup_t *pdu = bearer_req.pduSession + bearer_req.numPDUSessions;
    bearer_req.numPDUSessions++;
    pdu->sessionId = session->pdusession_id;
    pdu->nssai = sessions[i].nssai;
    if (cuup_nssai.sst == 0)
      cuup_nssai = pdu->nssai; /* for CU-UP selection below */

    pdu->integrityProtectionIndication = rrc->security.do_drb_integrity ? E1AP_IntegrityProtectionIndication_required : E1AP_IntegrityProtectionIndication_not_needed;

    pdu->confidentialityProtectionIndication = rrc->security.do_drb_ciphering ? E1AP_ConfidentialityProtectionIndication_required : E1AP_ConfidentialityProtectionIndication_not_needed;
    pdu->teId = session->gtp_teid;
    memcpy(&pdu->tlAddress, session->upf_addr.buffer, 4); // Fixme: dirty IPv4 target

    /* we assume for the moment one DRB per PDU session. Activate the bearer,
     * and configure in RRC. */
    int drb_id = get_next_available_drb_id(UE);
    drb_t *rrc_drb = generateDRB(UE,
                                 drb_id,
                                 pduSession,
                                 rrc->configuration.enable_sdap,
                                 rrc->security.do_drb_integrity,
                                 rrc->security.do_drb_ciphering);

    pdu->numDRB2Setup = 1; // One DRB per PDU Session. TODO: Remove hardcoding
    for (int j=0; j < pdu->numDRB2Setup; j++) {
      DRB_nGRAN_to_setup_t *drb = pdu->DRBnGRanList + j;

      drb->id = rrc_drb->drb_id;

      struct sdap_config_s *sdap_config = &rrc_drb->cnAssociation.sdap_config;
      drb->defaultDRB = sdap_config->defaultDRB;
      drb->sDAP_Header_UL = sdap_config->sdap_HeaderUL;
      drb->sDAP_Header_DL = sdap_config->sdap_HeaderDL;

      struct pdcp_config_s *pdcp_config = &rrc_drb->pdcp_config;
      drb->pDCP_SN_Size_UL = pdcp_config->pdcp_SN_SizeUL;
      drb->pDCP_SN_Size_DL = pdcp_config->pdcp_SN_SizeDL;
      drb->discardTimer = pdcp_config->discardTimer;
      drb->reorderingTimer = pdcp_config->t_Reordering;

      drb->rLC_Mode = rrc->configuration.um_on_default_drb ? E1AP_RLC_Mode_rlc_um_bidirectional : E1AP_RLC_Mode_rlc_am;

      drb->numCellGroups = 1; // assume one cell group associated with a DRB

      for (int k=0; k < drb->numCellGroups; k++) {
        cell_group_t *cellGroup = drb->cellGroupList + k;
        cellGroup->id = 0; // MCG
      }

      drb->numQosFlow2Setup = session->nb_qos;
      for (int k=0; k < drb->numQosFlow2Setup; k++) {
        qos_flow_to_setup_t *qos_flow = drb->qosFlows + k;
        pdusession_level_qos_parameter_t *qos_session = session->qos + k;

        qos_characteristics_t *qos_char = &qos_flow->qos_params.qos_characteristics;
        qos_flow->qfi = qos_session->qfi;
        qos_char->qos_type = qos_session->fiveQI_type;
        if (qos_char->qos_type == dynamic) {
          qos_char->dynamic.fiveqi = qos_session->fiveQI;
          qos_char->dynamic.qos_priority_level = qos_session->qos_priority;
        } else {
          qos_char->non_dynamic.fiveqi = qos_session->fiveQI;
          qos_char->non_dynamic.qos_priority_level = qos_session->qos_priority;
        }

        ngran_allocation_retention_priority_t *rent_priority = &qos_flow->qos_params.alloc_reten_priority;
        ngap_allocation_retention_priority_t *rent_priority_in = &qos_session->allocation_retention_priority;
        rent_priority->priority_level = rent_priority_in->priority_level;
        rent_priority->preemption_capability = rent_priority_in->pre_emp_capability;
        rent_priority->preemption_vulnerability = rent_priority_in->pre_emp_vulnerability;
      }
    }
  }
  /* Limitation: we assume one fixed CU-UP per UE. We base the selection on
   * NSSAI, but the UE might have multiple PDU sessions with differing slices,
   * in which we might need to select different CU-UPs. In this case, we would
   * actually need to group the E1 bearer context setup for the different
   * CU-UPs, and send them to the different CU-UPs. */
  sctp_assoc_t assoc_id = get_new_cuup_for_ue(rrc, UE, cuup_nssai.sst, cuup_nssai.sd);
  rrc->cucp_cuup.bearer_context_setup(assoc_id, &bearer_req);
}

//------------------------------------------------------------------------------
int rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ(MessageDef *msg_p, instance_t instance)
//------------------------------------------------------------------------------
{
  protocol_ctxt_t ctxt = {0};
  ngap_initial_context_setup_req_t *req = &NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p);

  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[instance], req->gNB_ue_ngap_id);
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index, send a failure to NGAP and discard it! */
    MessageDef *msg_fail_p = NULL;
    LOG_W(NR_RRC, "[gNB %ld] In NGAP_INITIAL_CONTEXT_SETUP_REQ: unknown UE from NGAP ids (%u)\n", instance, req->gNB_ue_ngap_id);
    msg_fail_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_INITIAL_CONTEXT_SETUP_FAIL);
    NGAP_INITIAL_CONTEXT_SETUP_FAIL(msg_fail_p).gNB_ue_ngap_id = req->gNB_ue_ngap_id;
    // TODO add failure cause when defined!
    itti_send_msg_to_task(TASK_NGAP, instance, msg_fail_p);
    return (-1);
  }
  PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, UE->rrc_ue_id, 0, 0);
  UE->amf_ue_ngap_id = req->amf_ue_ngap_id;

  /* NAS PDU */
  // this is malloced pointers, we pass it for later free()
  UE->nas_pdu = req->nas_pdu;

  /* security */
  rrc_gNB_process_security(&ctxt, ue_context_p, &req->security_capabilities);
  process_gNB_security_key(&ctxt, ue_context_p, req->security_key);

  /* configure only integrity, ciphering comes after receiving SecurityModeComplete */
  nr_rrc_pdcp_config_security(&ctxt, ue_context_p, 0);

  uint8_t nb_pdusessions_tosetup = req->nb_of_pdusessions;
  /* if there are PDU sessions to setup, first send them to the CU-UP, then
   * send the UE Context setup with Security commend. Else go to the security
   * command directly. */
  if (nb_pdusessions_tosetup > 0) {
    trigger_bearer_setup(RC.nrrrc[instance],
                         UE,
                         req->nb_of_pdusessions,
                         req->pdusession_param,
                         /*req->ueAggMaxBitRateDownlink*/ 0);
  } else {
    rrc_gNB_generate_SecurityModeCommand(&ctxt, ue_context_p, 0, NULL);
  }

  return 0;
}

//------------------------------------------------------------------------------
void rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP(const protocol_ctxt_t *const ctxt_pP, rrc_gNB_ue_context_t *const ue_context_pP)
//------------------------------------------------------------------------------
{
  MessageDef *msg_p = NULL;
  int pdu_sessions_done = 0;
  int pdu_sessions_failed = 0;
  msg_p = itti_alloc_new_message (TASK_RRC_ENB, 0, NGAP_INITIAL_CONTEXT_SETUP_RESP);
  ngap_initial_context_setup_resp_t *resp = &NGAP_INITIAL_CONTEXT_SETUP_RESP(msg_p);
  gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;

  resp->gNB_ue_ngap_id = UE->rrc_ue_id;

  for (int pdusession = 0; pdusession < UE->nb_of_pdusessions; pdusession++) {
    rrc_pdu_session_param_t *session = &UE->pduSession[pdusession];
    if (session->status == PDU_SESSION_STATUS_DONE) {
      pdu_sessions_done++;
      resp->pdusessions[pdusession].pdusession_id = session->param.pdusession_id;
      resp->pdusessions[pdusession].gtp_teid = session->param.gNB_teid_N3;
      memcpy(resp->pdusessions[pdusession].gNB_addr.buffer,
             session->param.gNB_addr_N3.buffer,
             sizeof(resp->pdusessions[pdusession].gNB_addr.buffer));
      resp->pdusessions[pdusession].gNB_addr.length = 4; // Fixme: IPv4 hard coded here
      resp->pdusessions[pdusession].nb_of_qos_flow = session->param.nb_qos;
      for (int qos_flow_index = 0; qos_flow_index < session->param.nb_qos; qos_flow_index++) {
        resp->pdusessions[pdusession].associated_qos_flows[qos_flow_index].qfi = session->param.qos[qos_flow_index].qfi;
        resp->pdusessions[pdusession].associated_qos_flows[qos_flow_index].qos_flow_mapping_ind = QOSFLOW_MAPPING_INDICATION_DL;
      }
    } else if (session->status != PDU_SESSION_STATUS_ESTABLISHED) {
      session->status = PDU_SESSION_STATUS_FAILED;
      pdusession_failed_t *fail = &resp->pdusessions_failed[pdu_sessions_failed];
      fail->pdusession_id = session->param.pdusession_id;
      fail->cause = NGAP_CAUSE_RADIO_NETWORK;
      fail->cause_value = NGAP_CauseRadioNetwork_unknown_PDU_session_ID;
      pdu_sessions_failed++;
    }
  }

  resp->nb_of_pdusessions = pdu_sessions_done;
  resp->nb_of_pdusessions_failed = pdu_sessions_failed;
  itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);
}

static NR_CipheringAlgorithm_t rrc_gNB_select_ciphering(
    const protocol_ctxt_t *const ctxt_pP,
    uint16_t algorithms)
{
  gNB_RRC_INST *rrc = RC.nrrrc[ctxt_pP->module_id];
  int i;
  /* preset nea0 as fallback */
  int ret = 0;

  /* Select ciphering algorithm based on gNB configuration file and
   * UE's supported algorithms.
   * We take the first from the list that is supported by the UE.
   * The ordering of the list comes from the configuration file.
   */
  for (i = 0; i < rrc->security.ciphering_algorithms_count; i++) {
    int nea_mask[4] = {
      0,
      NGAP_ENCRYPTION_NEA1_MASK,
      NGAP_ENCRYPTION_NEA2_MASK,
      NGAP_ENCRYPTION_NEA3_MASK
    };
    if (rrc->security.ciphering_algorithms[i] == 0) {
      /* nea0 */
      break;
    }
    if (algorithms & nea_mask[rrc->security.ciphering_algorithms[i]]) {
      ret = rrc->security.ciphering_algorithms[i];
      break;
    }
  }

  LOG_D(RRC, "selecting ciphering algorithm %d\n", ret);

  return ret;
}

static e_NR_IntegrityProtAlgorithm rrc_gNB_select_integrity(
    const protocol_ctxt_t *const ctxt_pP,
    uint16_t algorithms)
{
  gNB_RRC_INST *rrc = RC.nrrrc[ctxt_pP->module_id];
  int i;
  /* preset nia0 as fallback */
  int ret = 0;

  /* Select integrity algorithm based on gNB configuration file and
   * UE's supported algorithms.
   * We take the first from the list that is supported by the UE.
   * The ordering of the list comes from the configuration file.
   */
  for (i = 0; i < rrc->security.integrity_algorithms_count; i++) {
    int nia_mask[4] = {
      0,
      NGAP_INTEGRITY_NIA1_MASK,
      NGAP_INTEGRITY_NIA2_MASK,
      NGAP_INTEGRITY_NIA3_MASK
    };
    if (rrc->security.integrity_algorithms[i] == 0) {
      /* nia0 */
      break;
    }
    if (algorithms & nia_mask[rrc->security.integrity_algorithms[i]]) {
      ret = rrc->security.integrity_algorithms[i];
      break;
    }
  }

  LOG_D(RRC, "selecting integrity algorithm %d\n", ret);

  return ret;
}

static int rrc_gNB_process_security(const protocol_ctxt_t *const ctxt_pP, rrc_gNB_ue_context_t *const ue_context_pP, ngap_security_capabilities_t *security_capabilities_pP)
{
  bool                                                  changed = false;
  NR_CipheringAlgorithm_t                               cipheringAlgorithm;
  e_NR_IntegrityProtAlgorithm                           integrityProtAlgorithm;
  gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;

  /* Save security parameters */
  UE->security_capabilities = *security_capabilities_pP;
  // translation
  LOG_D(NR_RRC,
        "[gNB %d] NAS security_capabilities.encryption_algorithms %u AS ciphering_algorithm %lu NAS security_capabilities.integrity_algorithms %u AS integrity_algorithm %u\n",
        ctxt_pP->module_id,
        UE->security_capabilities.nRencryption_algorithms,
        (unsigned long)UE->ciphering_algorithm,
        UE->security_capabilities.nRintegrity_algorithms,
        UE->integrity_algorithm);
  /* Select relevant algorithms */
  cipheringAlgorithm = rrc_gNB_select_ciphering(ctxt_pP, UE->security_capabilities.nRencryption_algorithms);

  if (UE->ciphering_algorithm != cipheringAlgorithm) {
    UE->ciphering_algorithm = cipheringAlgorithm;
    changed = true;
  }

  integrityProtAlgorithm = rrc_gNB_select_integrity(ctxt_pP, UE->security_capabilities.nRintegrity_algorithms);

  if (UE->integrity_algorithm != integrityProtAlgorithm) {
    UE->integrity_algorithm = integrityProtAlgorithm;
    changed = true;
  }

  LOG_I(NR_RRC,
        "[gNB %d][UE %d] Selected security algorithms (%p): ciphering %lx, integrity %x (algorithms %s)\n",
        ctxt_pP->module_id,
        UE->rrc_ue_id,
        security_capabilities_pP,
        cipheringAlgorithm,
        integrityProtAlgorithm,
        changed ? "changed" : "are the same");
  return changed;
}

//------------------------------------------------------------------------------
int rrc_gNB_process_NGAP_DOWNLINK_NAS(MessageDef *msg_p, instance_t instance, mui_t *rrc_gNB_mui)
//------------------------------------------------------------------------------
{
  uint32_t length;
  uint8_t buffer[4096];
  protocol_ctxt_t ctxt = {0};
  ngap_downlink_nas_t *req = &NGAP_DOWNLINK_NAS(msg_p);
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[instance], req->gNB_ue_ngap_id);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index, send a failure to NGAP and discard it! */
    MessageDef *msg_fail_p;
    LOG_W(NR_RRC, "[gNB %ld] In NGAP_DOWNLINK_NAS: unknown UE from NGAP ids (%u)\n", instance, req->gNB_ue_ngap_id);
    msg_fail_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_NAS_NON_DELIVERY_IND);
    ngap_nas_non_delivery_ind_t *msg = &NGAP_NAS_NON_DELIVERY_IND(msg_fail_p);
    msg->gNB_ue_ngap_id = req->gNB_ue_ngap_id;
    msg->nas_pdu.length = req->nas_pdu.length;
    msg->nas_pdu.buffer = req->nas_pdu.buffer;
    // TODO add failure cause when defined!
    itti_send_msg_to_task(TASK_NGAP, instance, msg_fail_p);
    return (-1);
  }

  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, UE->rrc_ue_id, 0, 0);

  /* Create message for PDCP (DLInformationTransfer_t) */
  length = do_NR_DLInformationTransfer(buffer,
                                       sizeof(buffer),
                                       rrc_gNB_get_next_transaction_identifier(instance),
                                       req->nas_pdu.length,
                                       req->nas_pdu.buffer);
  LOG_DUMPMSG(NR_RRC, DEBUG_RRC, buffer, length, "[MSG] RRC DL Information Transfer\n");
  /*
   * switch UL or DL NAS message without RRC piggybacked to SRB2 if active.
   */
  AssertFatal(!NODE_IS_DU(RC.nrrrc[ctxt.module_id]->node_type), "illegal node type DU: receiving NGAP messages at this node\n");
  /* Transfer data to PDCP */
  rb_id_t srb_id = UE->Srb[2].Active ? DCCH1 : DCCH;
  nr_rrc_transfer_protected_rrc_message(RC.nrrrc[instance], UE, srb_id, buffer, length);
  return 0;
}

//------------------------------------------------------------------------------
void
rrc_gNB_send_NGAP_UPLINK_NAS(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_gNB_ue_context_t     *const ue_context_pP,
  NR_UL_DCCH_Message_t     *const ul_dcch_msg
)
//------------------------------------------------------------------------------
{
    MessageDef *msg_p;
    NR_ULInformationTransfer_t *ulInformationTransfer = ul_dcch_msg->message.choice.c1->choice.ulInformationTransfer;
    gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;

    if (ulInformationTransfer->criticalExtensions.present == NR_ULInformationTransfer__criticalExtensions_PR_ulInformationTransfer) {
      NR_DedicatedNAS_Message_t *nas = ulInformationTransfer->criticalExtensions.choice.ulInformationTransfer->dedicatedNAS_Message;
        uint8_t *buf = malloc(nas->size);
        AssertFatal(buf != NULL, "out of memory\n");
        memcpy(buf, nas->buf, nas->size);
        msg_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_UPLINK_NAS);
        NGAP_UPLINK_NAS(msg_p).gNB_ue_ngap_id = UE->rrc_ue_id;
        NGAP_UPLINK_NAS (msg_p).nas_pdu.length = nas->size;
        NGAP_UPLINK_NAS (msg_p).nas_pdu.buffer = buf;
        // extract_imsi(NGAP_UPLINK_NAS (msg_p).nas_pdu.buffer,
        //               NGAP_UPLINK_NAS (msg_p).nas_pdu.length,
        //               ue_context_pP);
        itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);
        LOG_D(NR_RRC,"Send RRC GNB UL Information Transfer \n");
    }
}

//------------------------------------------------------------------------------
void
rrc_gNB_send_NGAP_PDUSESSION_SETUP_RESP(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_gNB_ue_context_t     *const ue_context_pP,
  uint8_t                   xid
)
//------------------------------------------------------------------------------
{
  MessageDef *msg_p;
  int pdu_sessions_done = 0;
  int pdu_sessions_failed = 0;

  msg_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_PDUSESSION_SETUP_RESP);
  ngap_pdusession_setup_resp_t *resp = &NGAP_PDUSESSION_SETUP_RESP(msg_p);
  gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;
  resp->gNB_ue_ngap_id = UE->rrc_ue_id;

  for (int pdusession = 0; pdusession < UE->nb_of_pdusessions; pdusession++) {
    rrc_pdu_session_param_t *session = &UE->pduSession[pdusession];
    if (session->status == PDU_SESSION_STATUS_DONE) {
      pdusession_setup_t *tmp = &resp->pdusessions[pdu_sessions_done];
      tmp->pdusession_id = session->param.pdusession_id;
      tmp->nb_of_qos_flow = session->param.nb_qos;
      tmp->gtp_teid = session->param.gNB_teid_N3;
      tmp->pdu_session_type = session->param.pdu_session_type;
      tmp->gNB_addr.length = session->param.gNB_addr_N3.length;
      memcpy(tmp->gNB_addr.buffer, session->param.gNB_addr_N3.buffer, tmp->gNB_addr.length);
      for (int qos_flow_index = 0; qos_flow_index < tmp->nb_of_qos_flow; qos_flow_index++) {
        tmp->associated_qos_flows[qos_flow_index].qfi = session->param.qos[qos_flow_index].qfi;
        tmp->associated_qos_flows[qos_flow_index].qos_flow_mapping_ind = QOSFLOW_MAPPING_INDICATION_DL;
      }

      session->status = PDU_SESSION_STATUS_ESTABLISHED;
      LOG_I(NR_RRC,
            "msg index %d, pdu_sessions index %d, status %d, xid %d): nb_of_pdusessions %d,  pdusession_id %d, teid: %u \n ",
            pdu_sessions_done,
            pdusession,
            session->status,
            xid,
            UE->nb_of_pdusessions,
            tmp->pdusession_id,
            tmp->gtp_teid);
      pdu_sessions_done++;
    } else if (session->status != PDU_SESSION_STATUS_ESTABLISHED) {
      session->status = PDU_SESSION_STATUS_FAILED;
      pdusession_failed_t *fail = &resp->pdusessions_failed[pdu_sessions_failed];
      fail->pdusession_id = session->param.pdusession_id;
      fail->cause = NGAP_CAUSE_RADIO_NETWORK;
      fail->cause_value = NGAP_CauseRadioNetwork_unknown_PDU_session_ID;
      pdu_sessions_failed++;
    }
    resp->nb_of_pdusessions = pdu_sessions_done;
    resp->nb_of_pdusessions_failed = pdu_sessions_failed;
    // } else {
    //   LOG_D(NR_RRC,"xid does not corresponds  (context pdu_sessions index %d, status %d, xid %d/%d) \n ",
    //         pdusession, UE->pdusession[pdusession].status, xid, UE->pdusession[pdusession].xid);
    // }
  }

  if ((pdu_sessions_done > 0 || pdu_sessions_failed)) {
    LOG_I(NR_RRC, "NGAP_PDUSESSION_SETUP_RESP: sending the message\n");
    itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);
  }

  for(int i = 0; i < NB_RB_MAX; i++) {
    UE->pduSession[i].xid = -1;
  }

  return;
}

/* \brief checks if any transaction is ongoing for any xid of this UE */
static bool transaction_ongoing(const gNB_RRC_UE_t *UE)
{
  for (int xid = 0; xid < 4; ++xid)
    if (UE->xids[xid] != RRC_ACTION_NONE)
      return true;
  return false;
}

/* \brief delays the ongoing transaction (in msg_p) by setting a timer to wait
 * 10ms; upon expiry, delivers to RRC, which sends the message to itself */
static void delay_transaction(MessageDef *msg_p, int wait_us)
{
  MessageDef *new = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_PDUSESSION_SETUP_REQ);
  ngap_pdusession_setup_req_t *n = &NGAP_PDUSESSION_SETUP_REQ(new);
  *n = NGAP_PDUSESSION_SETUP_REQ(msg_p);

  int instance = msg_p->ittiMsgHeader.originInstance;
  long timer_id;
  timer_setup(0, wait_us, TASK_RRC_GNB, instance, TIMER_ONE_SHOT, new, &timer_id);
}

//------------------------------------------------------------------------------
void rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ(MessageDef *msg_p, instance_t instance)
//------------------------------------------------------------------------------
{
  protocol_ctxt_t                 ctxt={0};

  ngap_pdusession_setup_req_t* msg=&NGAP_PDUSESSION_SETUP_REQ(msg_p);
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[instance], msg->gNB_ue_ngap_id);
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, GNB_FLAG_YES, UE->rnti, 0, 0, 0);
  gNB_RRC_INST *rrc = RC.nrrrc[ctxt.module_id];
  LOG_I(NR_RRC, "[gNB %ld] gNB_ue_ngap_id %u \n", instance, msg->gNB_ue_ngap_id);

  if (ue_context_p == NULL) {
    MessageDef *msg_fail_p = NULL;
    LOG_W(NR_RRC, "[gNB %ld] In NGAP_PDUSESSION_SETUP_REQ: unknown UE from NGAP ids (%u)\n", instance, msg->gNB_ue_ngap_id);
    msg_fail_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_PDUSESSION_SETUP_REQUEST_FAIL);
    NGAP_PDUSESSION_SETUP_FAIL(msg_fail_p).gNB_ue_ngap_id = msg->gNB_ue_ngap_id;
    // TODO add failure cause when defined!
    itti_send_msg_to_task (TASK_NGAP, instance, msg_fail_p);
    return ;
  }

  AssertFatal(UE->rrc_ue_id == msg->gNB_ue_ngap_id, "logic bug\n");
  UE->amf_ue_ngap_id = msg->amf_ue_ngap_id;

  /* This is a hack. We observed that with some UEs, PDU session requests might
   * come in quick succession, faster than the RRC reconfiguration for the PDU
   * session requests can be carried out (UE is doing reconfig, and second PDU
   * session request arrives). We don't have currently the means to "queue up"
   * these transactions, which would probably involve some rework of the RRC.
   * To still allow these requests to come in and succeed, we below check and delay transactions
   * for 10ms. However, to not accidentally end up in infinite loops, the
   * maximum number is capped on a per-UE basis as indicated in variable
   * max_delays_pdu_session. */
  if (UE->max_delays_pdu_session > 0 && transaction_ongoing(UE)) {
    int wait_us = 10000;
    LOG_D(RRC, "UE %d: delay PDU session setup by %d us, pending %d retries\n", UE->rrc_ue_id, wait_us, UE->max_delays_pdu_session);
    delay_transaction(msg_p, wait_us);
    UE->max_delays_pdu_session--;
    return;
  }

  trigger_bearer_setup(rrc, UE, msg->nb_pdusessions_tosetup, msg->pdusession_setup_params, msg->ueAggMaxBitRateDownlink);
  return;
}

static void fill_qos2(NGAP_QosFlowAddOrModifyRequestList_t *qos, pdusession_t *session)
{
  // we need to duplicate the function fill_qos because all data types are slightly different
  DevAssert(qos->list.count > 0);
  DevAssert(qos->list.count <= NGAP_maxnoofQosFlows);
  for (int qosIdx = 0; qosIdx < qos->list.count; qosIdx++) {
    NGAP_QosFlowAddOrModifyRequestItem_t *qosFlowItem_p = qos->list.array[qosIdx];
    // Set the QOS informations
    session->qos[qosIdx].qfi = (uint8_t)qosFlowItem_p->qosFlowIdentifier;
    NGAP_QosCharacteristics_t *qosChar = &qosFlowItem_p->qosFlowLevelQosParameters->qosCharacteristics;
    AssertFatal(qosChar, "Qos characteristics are not available for qos flow index %d\n", qosIdx);
    if (qosChar->present == NGAP_QosCharacteristics_PR_nonDynamic5QI) {
      AssertFatal(qosChar->choice.dynamic5QI, "Non-Dynamic 5QI is NULL\n");
      session->qos[qosIdx].fiveQI_type = non_dynamic;
      session->qos[qosIdx].fiveQI = (uint64_t)qosChar->choice.nonDynamic5QI->fiveQI;
    } else {
      AssertFatal(qosChar->choice.dynamic5QI, "Dynamic 5QI is NULL\n");
      session->qos[qosIdx].fiveQI_type = dynamic;
      session->qos[qosIdx].fiveQI = (uint64_t)(*qosChar->choice.dynamic5QI->fiveQI);
    }

    ngap_allocation_retention_priority_t *tmp = &session->qos[qosIdx].allocation_retention_priority;
    NGAP_AllocationAndRetentionPriority_t *tmp2 = &qosFlowItem_p->qosFlowLevelQosParameters->allocationAndRetentionPriority;
    tmp->priority_level = tmp2->priorityLevelARP;
    tmp->pre_emp_capability = tmp2->pre_emptionCapability;
    tmp->pre_emp_vulnerability = tmp2->pre_emptionVulnerability;
  }
  session->nb_qos = qos->list.count;
}

static void decodePDUSessionResourceModify(pdusession_t *param, const ngap_pdu_t pdu)
{
  NGAP_PDUSessionResourceModifyRequestTransfer_t *pdusessionTransfer = NULL;
  asn_dec_rval_t dec_rval = aper_decode(NULL, &asn_DEF_NGAP_PDUSessionResourceModifyRequestTransfer, (void **)&pdusessionTransfer, pdu.buffer, pdu.length, 0, 0);

  if (dec_rval.code != RC_OK) {
    LOG_E(NR_RRC, "could not decode PDUSessionResourceModifyRequestTransfer\n");
    return;
  }

  for (int j = 0; j < pdusessionTransfer->protocolIEs.list.count; j++) {
    NGAP_PDUSessionResourceModifyRequestTransferIEs_t *pdusessionTransfer_ies = pdusessionTransfer->protocolIEs.list.array[j];
    switch (pdusessionTransfer_ies->id) {
        /* optional PDUSessionAggregateMaximumBitRate */
      case NGAP_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate:
        // TODO
        LOG_E(NR_RRC, "Cant' handle NGAP_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate\n");
        break;

        /* optional UL-NGU-UP-TNLModifyList */
      case NGAP_ProtocolIE_ID_id_UL_NGU_UP_TNLModifyList:
        // TODO
        LOG_E(NR_RRC, "Cant' handle NGAP_ProtocolIE_ID_id_UL_NGU_UP_TNLModifyList\n");
        break;

        /* optional NetworkInstance */
      case NGAP_ProtocolIE_ID_id_NetworkInstance:
        // TODO
        LOG_E(NR_RRC, "Cant' handle NGAP_ProtocolIE_ID_id_NetworkInstance\n");
        break;

        /* optional QosFlowAddOrModifyRequestList */
      case NGAP_ProtocolIE_ID_id_QosFlowAddOrModifyRequestList:
        fill_qos2(&pdusessionTransfer_ies->value.choice.QosFlowAddOrModifyRequestList, param);
        break;

        /* optional QosFlowToReleaseList */
      case NGAP_ProtocolIE_ID_id_QosFlowToReleaseList:
        // TODO
        LOG_E(NR_RRC, "Can't handle NGAP_ProtocolIE_ID_id_QosFlowToReleaseList\n");
        break;

        /* optional AdditionalUL-NGU-UP-TNLInformation */
      case NGAP_ProtocolIE_ID_id_AdditionalUL_NGU_UP_TNLInformation:
        // TODO
        LOG_E(NR_RRC, "Cant' handle NGAP_ProtocolIE_ID_id_AdditionalUL_NGU_UP_TNLInformation\n");
        break;

        /* optional CommonNetworkInstance */
      case NGAP_ProtocolIE_ID_id_CommonNetworkInstance:
        // TODO
        LOG_E(NR_RRC, "Cant' handle NGAP_ProtocolIE_ID_id_CommonNetworkInstance\n");
        break;

      default:
        LOG_E(NR_RRC, "could not found protocolIEs id %ld\n", pdusessionTransfer_ies->id);
        return;
    }
  }
    ASN_STRUCT_FREE(asn_DEF_NGAP_PDUSessionResourceModifyRequestTransfer,pdusessionTransfer );
}

//------------------------------------------------------------------------------
int rrc_gNB_process_NGAP_PDUSESSION_MODIFY_REQ(MessageDef *msg_p, instance_t instance)
//------------------------------------------------------------------------------
{
  rrc_gNB_ue_context_t *ue_context_p = NULL;

  protocol_ctxt_t ctxt;
  ngap_pdusession_modify_req_t *req = &NGAP_PDUSESSION_MODIFY_REQ(msg_p);

  ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[instance], req->gNB_ue_ngap_id);
  if (ue_context_p == NULL) {
    LOG_W(NR_RRC, "[gNB %ld] In NGAP_PDUSESSION_MODIFY_REQ: unknown UE from NGAP ids (%u)\n", instance, req->gNB_ue_ngap_id);
    // TO implement return setup failed
    return (-1);
  }
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, UE->rrc_ue_id, 0, 0);
  ctxt.eNB_index = 0;
  bool all_failed = true;
  for (int i = 0; i < req->nb_pdusessions_tomodify; i++) {
    rrc_pdu_session_param_t *sess;
    const pdusession_t *sessMod = req->pdusession_modify_params + i;
    for (sess = UE->pduSession; sess < UE->pduSession + UE->nb_of_pdusessions; sess++)
      if (sess->param.pdusession_id == sessMod->pdusession_id)
        break;
    if (sess == UE->pduSession + UE->nb_of_pdusessions) {
      LOG_W(NR_RRC, "Requested modification of non-existing PDU session, refusing modification\n");
      UE->nb_of_pdusessions++;
      sess->status = PDU_SESSION_STATUS_FAILED;
      sess->param.pdusession_id = sessMod->pdusession_id;
      sess->cause = NGAP_CAUSE_RADIO_NETWORK;
      UE->pduSession[i].cause_value = NGAP_CauseRadioNetwork_unknown_PDU_session_ID;
    } else {
      all_failed = false;
      sess->status = PDU_SESSION_STATUS_NEW;
      sess->param.pdusession_id = sessMod->pdusession_id;
      sess->cause = NGAP_CAUSE_RADIO_NETWORK;
      sess->cause_value = NGAP_CauseRadioNetwork_multiple_PDU_session_ID_instances;
      sess->status = PDU_SESSION_STATUS_NEW;
      sess->param.pdusession_id = sessMod->pdusession_id;
      sess->cause = NGAP_CAUSE_NOTHING;
      if (sessMod->nas_pdu.buffer != NULL) {
        UE->pduSession[i].param.nas_pdu = sessMod->nas_pdu;
      }
      // Save new pdu session parameters, qos, upf addr, teid
      decodePDUSessionResourceModify(&sess->param, UE->pduSession[i].param.pdusessionTransfer);
      sess->param.UPF_addr_N3 = sessMod->upf_addr;
      sess->param.UPF_teid_N3 = sessMod->gtp_teid;
    }
  }

  if (!all_failed) {
    LOG_D(NR_RRC, "generate RRCReconfiguration \n");
    rrc_gNB_modify_dedicatedRRCReconfiguration(&ctxt, ue_context_p);
  } else {
    LOG_I(NR_RRC,
          "pdu session modify failed, fill NGAP_PDUSESSION_MODIFY_RESP with the pdu session information that failed to modify \n");
    MessageDef *msg_fail_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_PDUSESSION_MODIFY_RESP);
    if (msg_fail_p == NULL) {
      LOG_E(NR_RRC, "itti_alloc_new_message failed, msg_fail_p is NULL \n");
      return (-1);
    }
    ngap_pdusession_modify_resp_t *msg = &NGAP_PDUSESSION_MODIFY_RESP(msg_fail_p);
    msg->gNB_ue_ngap_id = req->gNB_ue_ngap_id;
    msg->nb_of_pdusessions = 0;

    for (int i = 0; i < UE->nb_of_pdusessions; i++) {
      if (UE->pduSession[i].status == PDU_SESSION_STATUS_FAILED) {
        msg->pdusessions_failed[i].pdusession_id = UE->pduSession[i].param.pdusession_id;
        msg->pdusessions_failed[i].cause = UE->pduSession[i].cause;
        msg->pdusessions_failed[i].cause_value = UE->pduSession[i].cause_value;
      }
    }
    itti_send_msg_to_task(TASK_NGAP, instance, msg_fail_p);
  }
  return (0);
}

//------------------------------------------------------------------------------
int
rrc_gNB_send_NGAP_PDUSESSION_MODIFY_RESP(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_gNB_ue_context_t     *const ue_context_pP,
  uint8_t                   xid
)
//------------------------------------------------------------------------------
{
  MessageDef *msg_p = NULL;
  uint8_t pdu_sessions_failed = 0;
  uint8_t pdu_sessions_done = 0;
  gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;

  msg_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_PDUSESSION_MODIFY_RESP);
  if (msg_p == NULL) {
    LOG_E(NR_RRC, "itti_alloc_new_message failed, msg_p is NULL \n");
    return (-1);
  }
  ngap_pdusession_modify_resp_t *resp = &NGAP_PDUSESSION_MODIFY_RESP(msg_p);
  LOG_I(NR_RRC, "send message NGAP_PDUSESSION_MODIFY_RESP \n");

  resp->gNB_ue_ngap_id = UE->rrc_ue_id;

  for (int i = 0; i < UE->nb_of_pdusessions; i++) {
    if (xid != UE->pduSession[i].xid) {
      LOG_W(NR_RRC, "xid does not correspond (context pdu session index %d, status %d, xid %d/%d) \n ", i, UE->pduSession[i].status, xid, UE->pduSession[i].xid);
      continue;
    }
    if (UE->pduSession[i].status == PDU_SESSION_STATUS_DONE) {
      rrc_pdu_session_param_t *pduSession = find_pduSession(UE, UE->pduSession[i].param.pdusession_id, false);
      if (pduSession) {
        LOG_I(NR_RRC, "update pdu session %d \n", pduSession->param.pdusession_id);
        // Update UE->pduSession
        pduSession->status = PDU_SESSION_STATUS_ESTABLISHED;
        pduSession->cause = NGAP_CAUSE_NOTHING;
        for (int qos_flow_index = 0; qos_flow_index < UE->pduSession[i].param.nb_qos; qos_flow_index++) {
          pduSession->param.qos[qos_flow_index] = UE->pduSession[i].param.qos[qos_flow_index];
        }
        resp->pdusessions[pdu_sessions_done].pdusession_id = UE->pduSession[i].param.pdusession_id;
        for (int qos_flow_index = 0; qos_flow_index < UE->pduSession[i].param.nb_qos; qos_flow_index++) {
          resp->pdusessions[pdu_sessions_done].qos[qos_flow_index].qfi = UE->pduSession[i].param.qos[qos_flow_index].qfi;
        }
        resp->pdusessions[pdu_sessions_done].pdusession_id = UE->pduSession[i].param.pdusession_id;
        resp->pdusessions[pdu_sessions_done].nb_of_qos_flow = UE->pduSession[i].param.nb_qos;
        LOG_I(NR_RRC,
              "Modify Resp (msg index %d, pdu session index %d, status %d, xid %d): nb_of_pduSessions %d,  pdusession_id %d \n ",
              pdu_sessions_done,
              i,
              UE->pduSession[i].status,
              xid,
              UE->nb_of_pdusessions,
              resp->pdusessions[pdu_sessions_done].pdusession_id);
        pdu_sessions_done++;
      } else {
        LOG_W(NR_RRC, "PDU SESSION modify of a not existing pdu session %d \n", UE->pduSession[i].param.pdusession_id);
        resp->pdusessions_failed[pdu_sessions_failed].pdusession_id = UE->pduSession[i].param.pdusession_id;
        resp->pdusessions_failed[pdu_sessions_failed].cause = NGAP_CAUSE_RADIO_NETWORK;
        resp->pdusessions_failed[pdu_sessions_failed].cause_value = NGAP_CauseRadioNetwork_unknown_PDU_session_ID;
        pdu_sessions_failed++;
      }
    } else if ((UE->pduSession[i].status == PDU_SESSION_STATUS_NEW) || (UE->pduSession[i].status == PDU_SESSION_STATUS_ESTABLISHED)) {
      LOG_D(NR_RRC, "PDU SESSION is NEW or already ESTABLISHED\n");
    } else if (UE->pduSession[i].status == PDU_SESSION_STATUS_FAILED) {
      resp->pdusessions_failed[pdu_sessions_failed].pdusession_id = UE->pduSession[i].param.pdusession_id;
      resp->pdusessions_failed[pdu_sessions_failed].cause = UE->pduSession[i].cause;
      resp->pdusessions_failed[pdu_sessions_failed].cause_value = UE->pduSession[i].cause_value;
      pdu_sessions_failed++;
    }
    else
      LOG_W(NR_RRC,
            "Modify pdu session %d, unknown state %d \n ",
            UE->pduSession[i].param.pdusession_id,
            UE->pduSession[i].status);
  }

  resp->nb_of_pdusessions = pdu_sessions_done;
  resp->nb_of_pdusessions_failed = pdu_sessions_failed;

  if (pdu_sessions_done > 0 || pdu_sessions_failed > 0) {
    LOG_D(NR_RRC, "NGAP_PDUSESSION_MODIFY_RESP: sending the message (total pdu session %d)\n", UE->nb_of_pdusessions);
    itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);
  } else {
    itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
  }

  return 0;
}

//------------------------------------------------------------------------------
void rrc_gNB_send_NGAP_UE_CONTEXT_RELEASE_REQ(const module_id_t gnb_mod_idP, const rrc_gNB_ue_context_t *const ue_context_pP, const ngap_Cause_t causeP, const long cause_valueP)
//------------------------------------------------------------------------------
{
  if (ue_context_pP == NULL) {
    LOG_E(RRC, "[gNB] In NGAP_UE_CONTEXT_RELEASE_REQ: invalid UE\n");
  } else {
    const gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;
    MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_UE_CONTEXT_RELEASE_REQ);
    ngap_ue_release_req_t *req = &NGAP_UE_CONTEXT_RELEASE_REQ(msg);
    memset(req, 0, sizeof(*req));
    req->gNB_ue_ngap_id = UE->rrc_ue_id;
    req->cause = causeP;
    req->cause_value = cause_valueP;
    for (int i = 0; i < UE->nb_of_pdusessions; i++) {
      req->pdusessions[i].pdusession_id = UE->pduSession[i].param.pdusession_id;
      req->nb_of_pdusessions++;
    }
    itti_send_msg_to_task(TASK_NGAP, GNB_MODULE_ID_TO_INSTANCE(gnb_mod_idP), msg);
  }
}
/*------------------------------------------------------------------------------*/
int rrc_gNB_process_NGAP_UE_CONTEXT_RELEASE_REQ(MessageDef *msg_p, instance_t instance)
{
  uint32_t gNB_ue_ngap_id;
  gNB_ue_ngap_id = NGAP_UE_CONTEXT_RELEASE_REQ(msg_p).gNB_ue_ngap_id;
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[instance], gNB_ue_ngap_id);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index, send a failure to ngAP and discard it! */
    MessageDef *msg_fail_p;
    LOG_W(RRC, "[gNB %ld] In NGAP_UE_CONTEXT_RELEASE_REQ: unknown UE from gNB_ue_ngap_id (%u)\n",
          instance,
          gNB_ue_ngap_id);
    msg_fail_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_UE_CONTEXT_RELEASE_RESP); /* TODO change message ID. */
    NGAP_UE_CONTEXT_RELEASE_RESP(msg_fail_p).gNB_ue_ngap_id = gNB_ue_ngap_id;
    // TODO add failure cause when defined!
    itti_send_msg_to_task(TASK_NGAP, instance, msg_fail_p);
    return (-1);
  } else {
    /* TODO release context. */
    /* Send the response */
    MessageDef *msg_resp_p;
    msg_resp_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_UE_CONTEXT_RELEASE_RESP);
    NGAP_UE_CONTEXT_RELEASE_RESP(msg_resp_p).gNB_ue_ngap_id = gNB_ue_ngap_id;
    itti_send_msg_to_task(TASK_NGAP, instance, msg_resp_p);
    return (0);
  }
}

//-----------------------------------------------------------------------------
/*
* Process the NG command NGAP_UE_CONTEXT_RELEASE_COMMAND, sent by AMF.
* The gNB should remove all pdu session, NG context, and other context of the UE.
*/
int rrc_gNB_process_NGAP_UE_CONTEXT_RELEASE_COMMAND(MessageDef *msg_p, instance_t instance)
{
  //-----------------------------------------------------------------------------
  uint32_t gNB_ue_ngap_id = 0;
  protocol_ctxt_t ctxt;
  gNB_ue_ngap_id = NGAP_UE_CONTEXT_RELEASE_COMMAND(msg_p).gNB_ue_ngap_id;
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[instance], gNB_ue_ngap_id);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index */
    MessageDef *msg_complete_p = NULL;
    LOG_W(NR_RRC, "[gNB %ld] In NGAP_UE_CONTEXT_RELEASE_COMMAND: unknown UE from gNB_ue_ngap_id (%u)\n",
          instance,
          gNB_ue_ngap_id);
    msg_complete_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_UE_CONTEXT_RELEASE_COMPLETE);
    NGAP_UE_CONTEXT_RELEASE_COMPLETE(msg_complete_p).gNB_ue_ngap_id = gNB_ue_ngap_id;
    itti_send_msg_to_task(TASK_NGAP, instance, msg_complete_p);
    return -1;
  }

  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, UE->rrc_ue_id, 0, 0);
  ctxt.eNB_index = 0;
  rrc_gNB_generate_RRCRelease(&ctxt, ue_context_p);
  return 0;
}

void rrc_gNB_send_NGAP_UE_CONTEXT_RELEASE_COMPLETE(
  instance_t instance,
  uint32_t   gNB_ue_ngap_id) {
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_UE_CONTEXT_RELEASE_COMPLETE);
  NGAP_UE_CONTEXT_RELEASE_COMPLETE(msg).gNB_ue_ngap_id = gNB_ue_ngap_id;
  itti_send_msg_to_task(TASK_NGAP, instance, msg);
}

void rrc_gNB_send_NGAP_UE_CAPABILITIES_IND(const protocol_ctxt_t *const ctxt_pP,
                                           rrc_gNB_ue_context_t *const ue_context_pP,
                                           const NR_UECapabilityInformation_t *const ue_cap_info)
//------------------------------------------------------------------------------
{
  NR_UE_CapabilityRAT_ContainerList_t *ueCapabilityRATContainerList =
      ue_cap_info->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList;
  void *buf;
  NR_UERadioAccessCapabilityInformation_t rac = {0};
  gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;

  if (ueCapabilityRATContainerList->list.count == 0) {
    LOG_W(RRC, "[gNB %d][UE %x] bad UE capabilities\n", ctxt_pP->module_id, UE->rnti);
    }

    int ret = uper_encode_to_new_buffer(&asn_DEF_NR_UE_CapabilityRAT_ContainerList, NULL, ueCapabilityRATContainerList, &buf);
    AssertFatal(ret > 0, "fail to encode ue capabilities\n");

    rac.criticalExtensions.present = NR_UERadioAccessCapabilityInformation__criticalExtensions_PR_c1;
    asn1cCalloc(rac.criticalExtensions.choice.c1, c1);
    c1->present = NR_UERadioAccessCapabilityInformation__criticalExtensions__c1_PR_ueRadioAccessCapabilityInformation;
    asn1cCalloc(c1->choice.ueRadioAccessCapabilityInformation, info);
    info->ue_RadioAccessCapabilityInfo.buf = buf;
    info->ue_RadioAccessCapabilityInfo.size = ret;
    info->nonCriticalExtension = NULL;
    /* 8192 is arbitrary, should be big enough */
    void *buf2 = NULL;
    int encoded = uper_encode_to_new_buffer(&asn_DEF_NR_UERadioAccessCapabilityInformation, NULL, &rac, &buf2);

    AssertFatal(encoded > 0, "fail to encode ue capabilities\n");
    ;
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NR_UERadioAccessCapabilityInformation, &rac);
    MessageDef *msg_p;
    msg_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_UE_CAPABILITIES_IND);
    ngap_ue_cap_info_ind_t *ind = &NGAP_UE_CAPABILITIES_IND(msg_p);
    memset(ind, 0, sizeof(*ind));
    ind->gNB_ue_ngap_id = UE->rrc_ue_id;
    ind->ue_radio_cap.length = encoded;
    ind->ue_radio_cap.buffer = buf2;
    itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);
    LOG_I(NR_RRC,"Send message to ngap: NGAP_UE_CAPABILITIES_IND\n");
}

//------------------------------------------------------------------------------
void
rrc_gNB_send_NGAP_PDUSESSION_RELEASE_RESPONSE(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_gNB_ue_context_t     *const ue_context_pP,
  uint8_t                   xid
)
//------------------------------------------------------------------------------
{
  int pdu_sessions_released = 0;
  MessageDef   *msg_p;
  gNB_RRC_UE_t *UE = &ue_context_pP->ue_context;
  msg_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_PDUSESSION_RELEASE_RESPONSE);
  ngap_pdusession_release_resp_t *resp = &NGAP_PDUSESSION_RELEASE_RESPONSE(msg_p);
  memset(resp, 0, sizeof(*resp));
  resp->gNB_ue_ngap_id = UE->rrc_ue_id;

  for (int i = 0; i < UE->nb_of_pdusessions; i++) {
    if (xid == UE->pduSession[i].xid) {
      resp->pdusession_release[pdu_sessions_released].pdusession_id = UE->pduSession[i].param.pdusession_id;
      pdu_sessions_released++;
      //clear
      memset(&UE->pduSession[i], 0, sizeof(*UE->pduSession));
      UE->pduSession[i].status = PDU_SESSION_STATUS_RELEASED;
      LOG_W(NR_RRC, "Released pdu session, but code to finish to free memory\n");
    }
  }

  resp->nb_of_pdusessions_released = pdu_sessions_released;
  resp->nb_of_pdusessions_failed = 0;
  LOG_I(NR_RRC, "NGAP PDUSESSION RELEASE RESPONSE: rrc_ue_id %u release_pdu_sessions %d\n", resp->gNB_ue_ngap_id, pdu_sessions_released);
  itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);
}

//------------------------------------------------------------------------------
int rrc_gNB_process_NGAP_PDUSESSION_RELEASE_COMMAND(MessageDef *msg_p, instance_t instance)
//------------------------------------------------------------------------------
{
  uint32_t gNB_ue_ngap_id;
  protocol_ctxt_t ctxt;
  ngap_pdusession_release_command_t *cmd = &NGAP_PDUSESSION_RELEASE_COMMAND(msg_p);
  gNB_ue_ngap_id = cmd->gNB_ue_ngap_id;
  if (cmd->nb_pdusessions_torelease > NGAP_MAX_PDUSESSION) {
    LOG_E(NR_RRC, "incorrect number of pdu session do release %d\n", cmd->nb_pdusessions_torelease);
    return -1;
  }
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[instance], gNB_ue_ngap_id);

  if (!ue_context_p) {
    LOG_E(NR_RRC, "[gNB %ld] not found ue context gNB_ue_ngap_id %u \n", instance, gNB_ue_ngap_id);
    return -1;
  }

  LOG_I(NR_RRC, "[gNB %ld] gNB_ue_ngap_id %u \n", instance, gNB_ue_ngap_id);
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, UE->rrc_ue_id, 0, 0);
  LOG_I(
      NR_RRC, "PDU Session Release Command: AMF_UE_NGAP_ID %lu  rrc_ue_id %u release_pdusessions %d \n", cmd->amf_ue_ngap_id, gNB_ue_ngap_id, cmd->nb_pdusessions_torelease);
  bool found = false;
  uint8_t xid = rrc_gNB_get_next_transaction_identifier(ctxt.module_id);
  UE->xids[xid] = RRC_PDUSESSION_RELEASE;
  for (int pdusession = 0; pdusession < cmd->nb_pdusessions_torelease; pdusession++) {
    rrc_pdu_session_param_t *pduSession = find_pduSession(UE, cmd->pdusession_release_params[pdusession].pdusession_id, false);
    if (!pduSession) {
      LOG_I(NR_RRC, "no pdusession_id, AMF requested to close it id=%d\n", cmd->pdusession_release_params[pdusession].pdusession_id);
      int j=UE->nb_of_pdusessions++;
      UE->pduSession[j].status = PDU_SESSION_STATUS_FAILED;
      UE->pduSession[j].param.pdusession_id = cmd->pdusession_release_params[pdusession].pdusession_id;
      UE->pduSession[j].cause = NGAP_CAUSE_RADIO_NETWORK;
      UE->pduSession[j].cause_value = 30;
      continue;
    }
    if (pduSession->status == PDU_SESSION_STATUS_FAILED) {
      pduSession->xid = xid;
      continue;
    }
    if (pduSession->status == PDU_SESSION_STATUS_ESTABLISHED) {
      found = true;
      LOG_I(NR_RRC, "RELEASE pdusession %d \n", pduSession->param.pdusession_id);
      pduSession->status = PDU_SESSION_STATUS_TORELEASE;
      pduSession->xid = xid;
    }
  }

  if (found) {
    // TODO RRCReconfiguration To UE
    LOG_I(NR_RRC, "Send RRCReconfiguration To UE \n");
    rrc_gNB_generate_dedicatedRRCReconfiguration_release(&ctxt, ue_context_p, xid, cmd->nas_pdu.length, cmd->nas_pdu.buffer);
  } else {
    // gtp tunnel delete
    LOG_I(NR_RRC, "gtp tunnel delete all tunnels for UE %04x\n", UE->rnti);
    gtpv1u_gnb_delete_tunnel_req_t req = {0};
    req.ue_id = UE->rnti;
    gtpv1u_delete_ngu_tunnel(instance, &req);
    // NGAP_PDUSESSION_RELEASE_RESPONSE
    rrc_gNB_send_NGAP_PDUSESSION_RELEASE_RESPONSE(&ctxt, ue_context_p, xid);
    LOG_I(NR_RRC, "Send PDU Session Release Response \n");
  }
  return 0;
}

void nr_rrc_rx_tx(void) {
  // check timers

  // check if UEs are lost, to remove them from upper layers

  //

}

/*------------------------------------------------------------------------------*/
int rrc_gNB_process_PAGING_IND(MessageDef *msg_p, instance_t instance)
{
  for (uint16_t tai_size = 0; tai_size < NGAP_PAGING_IND(msg_p).tai_size; tai_size++) {
    LOG_I(NR_RRC,"[gNB %ld] In NGAP_PAGING_IND: MCC %d, MNC %d, TAC %d\n", instance, NGAP_PAGING_IND(msg_p).plmn_identity[tai_size].mcc,
          NGAP_PAGING_IND(msg_p).plmn_identity[tai_size].mnc, NGAP_PAGING_IND(msg_p).tac[tai_size]);

    for (uint8_t j = 0; j < RC.nrrrc[instance]->configuration.num_plmn; j++) {
      if (RC.nrrrc[instance]->configuration.mcc[j] == NGAP_PAGING_IND(msg_p).plmn_identity[tai_size].mcc
          && RC.nrrrc[instance]->configuration.mnc[j] == NGAP_PAGING_IND(msg_p).plmn_identity[tai_size].mnc
          && RC.nrrrc[instance]->configuration.tac == NGAP_PAGING_IND(msg_p).tac[tai_size]) {
        for (uint8_t CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
          AssertFatal(false, "to be implemented properly\n");
          if (NODE_IS_CU(RC.nrrrc[instance]->node_type)) {
            MessageDef *m = itti_alloc_new_message(TASK_RRC_GNB, 0, F1AP_PAGING_IND);
            F1AP_PAGING_IND(m).plmn.mcc = RC.nrrrc[j]->configuration.mcc[0];
            F1AP_PAGING_IND(m).plmn.mnc = RC.nrrrc[j]->configuration.mnc[0];
            F1AP_PAGING_IND(m).plmn.mnc_digit_length = RC.nrrrc[j]->configuration.mnc_digit_length[0];
            F1AP_PAGING_IND (m).nr_cellid        = RC.nrrrc[j]->nr_cellid;
            F1AP_PAGING_IND (m).ueidentityindexvalue = (uint16_t)(NGAP_PAGING_IND(msg_p).ue_paging_identity.s_tmsi.m_tmsi%1024);
            F1AP_PAGING_IND (m).fiveg_s_tmsi = NGAP_PAGING_IND(msg_p).ue_paging_identity.s_tmsi.m_tmsi;
            F1AP_PAGING_IND (m).paging_drx = NGAP_PAGING_IND(msg_p).paging_drx;
            LOG_E(F1AP, "ueidentityindexvalue %u fiveg_s_tmsi %ld paging_drx %u\n", F1AP_PAGING_IND (m).ueidentityindexvalue, F1AP_PAGING_IND (m).fiveg_s_tmsi, F1AP_PAGING_IND (m).paging_drx);
            itti_send_msg_to_task(TASK_CU_F1, instance, m);
          } else {
            //rrc_gNB_generate_pcch_msg(NGAP_PAGING_IND(msg_p).ue_paging_identity.s_tmsi.m_tmsi,(uint8_t)NGAP_PAGING_IND(msg_p).paging_drx, instance, CC_id);
          } // end of nodetype check
        } // end of cc loop
      } // end of mcc mnc check
    } // end of num_plmn
  } // end of tai size

  return 0;
}
