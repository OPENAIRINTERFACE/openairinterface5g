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

/*! \file ngap_gNB_nas_procedures.c
 * \brief NGAP gNb NAS procedure handler
 * \author  Yoshio INOUE, Masayuki HARADA 
 * \date 2020
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \version 1.0
 * @ingroup _ngap
 */

 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "assertions.h"
#include "conversions.h"

#include "intertask_interface.h"

#include "ngap_common.h"
#include "ngap_gNB_defs.h"

#include "ngap_gNB_itti_messaging.h"

#include "ngap_gNB_encoder.h"
#include "ngap_gNB_nnsf.h"
#include "ngap_gNB_ue_context.h"
#include "ngap_gNB_nas_procedures.h"
#include "ngap_gNB_management_procedures.h"

//------------------------------------------------------------------------------
int ngap_gNB_handle_nas_first_req(
    instance_t instance, ngap_nas_first_req_t *ngap_nas_first_req_p)
//------------------------------------------------------------------------------
{
  ngap_gNB_instance_t          *instance_p = NULL;
  struct ngap_gNB_amf_data_s   *amf_desc_p = NULL;
  struct ngap_gNB_ue_context_s *ue_desc_p  = NULL;
  NGAP_NGAP_PDU_t               pdu;
  NGAP_InitialUEMessage_t      *out;
  NGAP_InitialUEMessage_IEs_t  *ie;
  NGAP_UserLocationInformationNR_t *userinfo_nr_p = NULL;
  uint8_t  *buffer = NULL;
  uint32_t  length = 0;
  DevAssert(ngap_nas_first_req_p != NULL);
  /* Retrieve the NGAP gNB instance associated with Mod_id */
  instance_p = ngap_gNB_get_instance(instance);
  DevAssert(instance_p != NULL);
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (NGAP_InitiatingMessage_t *)calloc(1,sizeof(NGAP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = NGAP_ProcedureCode_id_InitialUEMessage;
  pdu.choice.initiatingMessage->criticality = NGAP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = NGAP_InitiatingMessage__value_PR_InitialUEMessage;
  out = &pdu.choice.initiatingMessage->value.choice.InitialUEMessage;

  /* Select the AMF corresponding to the provided GUAMI. */
  //TODO have not be test. it's should be test
  if (ngap_nas_first_req_p->ue_identity.presenceMask & NGAP_UE_IDENTITIES_guami) {
    amf_desc_p = ngap_gNB_nnsf_select_amf_by_guami(
                     instance_p,
                     ngap_nas_first_req_p->establishment_cause,
                     ngap_nas_first_req_p->ue_identity.guami);

    if (amf_desc_p) {
        NGAP_INFO("[gNB %ld] Chose AMF '%s' (assoc_id %d) through GUAMI MCC %d MNC %d AMFRI %d AMFSI %d AMFPT %d\n",
                  instance,
                  amf_desc_p->amf_name,
                  amf_desc_p->assoc_id,
                  ngap_nas_first_req_p->ue_identity.guami.mcc,
                  ngap_nas_first_req_p->ue_identity.guami.mnc,
                  ngap_nas_first_req_p->ue_identity.guami.amf_region_id,
                  ngap_nas_first_req_p->ue_identity.guami.amf_set_id,
                  ngap_nas_first_req_p->ue_identity.guami.amf_pointer);
    }
  }

  if (amf_desc_p == NULL) {
    /* Select the AMF corresponding to the provided s-TMSI. */
    //TODO have not be test. it's should be test
    if (ngap_nas_first_req_p->ue_identity.presenceMask & NGAP_UE_IDENTITIES_FiveG_s_tmsi) {
        amf_desc_p = ngap_gNB_nnsf_select_amf_by_amf_setid(
                         instance_p,
                         ngap_nas_first_req_p->establishment_cause,
                         ngap_nas_first_req_p->selected_plmn_identity,
                         ngap_nas_first_req_p->ue_identity.s_tmsi.amf_set_id);

        if (amf_desc_p) {
            NGAP_INFO("[gNB %ld] Chose AMF '%s' (assoc_id %d) through S-TMSI AMFSI %d and selected PLMN Identity index %d MCC %d MNC %d\n",
                      instance,
                      amf_desc_p->amf_name,
                      amf_desc_p->assoc_id,
                      ngap_nas_first_req_p->ue_identity.s_tmsi.amf_set_id,
                      ngap_nas_first_req_p->selected_plmn_identity,
                      instance_p->mcc[ngap_nas_first_req_p->selected_plmn_identity],
                      instance_p->mnc[ngap_nas_first_req_p->selected_plmn_identity]);
        }
    }
  }

  if (amf_desc_p == NULL) {
    /* Select AMF based on the selected PLMN identity, received through RRC
     * Connection Setup Complete */
    //TODO have not be test. it's should be test
    amf_desc_p = ngap_gNB_nnsf_select_amf_by_plmn_id(
                     instance_p,
                     ngap_nas_first_req_p->establishment_cause,
                     ngap_nas_first_req_p->selected_plmn_identity);

    if (amf_desc_p) {
        NGAP_INFO("[gNB %ld] Chose AMF '%s' (assoc_id %d) through selected PLMN Identity index %d MCC %d MNC %d\n",
                  instance,
                  amf_desc_p->amf_name,
                  amf_desc_p->assoc_id,
                  ngap_nas_first_req_p->selected_plmn_identity,
                  instance_p->mcc[ngap_nas_first_req_p->selected_plmn_identity],
                  instance_p->mnc[ngap_nas_first_req_p->selected_plmn_identity]);
    }
  }

  if (amf_desc_p == NULL) {
    /*
     * If no AMF corresponds to the GUAMI, the s-TMSI, or the selected PLMN
     * identity, selects the AMF with the highest capacity.
     */
    //TODO have not be test. it's should be test
    amf_desc_p = ngap_gNB_nnsf_select_amf(
                     instance_p,
                     ngap_nas_first_req_p->establishment_cause);

    if (amf_desc_p) {
        NGAP_INFO("[gNB %ld] Chose AMF '%s' (assoc_id %d) through highest relative capacity\n",
                  instance,
                  amf_desc_p->amf_name,
                  amf_desc_p->assoc_id);
    }
  }

  if (amf_desc_p == NULL) {
    /*
     * In case gNB has no AMF associated, the gNB should inform RRC and discard
     * this request.
     */
    NGAP_WARN("No AMF is associated to the gNB\n");
    // TODO: Inform RRC
    return -1;
  }

  /* The gNB should allocate a unique gNB UE NGAP ID for this UE. The value
   * will be used for the duration of the connectivity.
   */
  ue_desc_p = ngap_gNB_allocate_new_UE_context();
  DevAssert(ue_desc_p != NULL);
  /* Keep a reference to the selected AMF */
  ue_desc_p->amf_ref       = amf_desc_p;
  ue_desc_p->ue_initial_id = ngap_nas_first_req_p->ue_initial_id;
  ue_desc_p->gNB_instance  = instance_p;
  ue_desc_p->selected_plmn_identity = ngap_nas_first_req_p->selected_plmn_identity;

  do {
    struct ngap_gNB_ue_context_s *collision_p;
    /* Peek a random value for the gNB_ue_ngap_id */
    ue_desc_p->gNB_ue_ngap_id = (random() + random()) & 0xffffffff;

    if ((collision_p = RB_INSERT(ngap_ue_map, &instance_p->ngap_ue_head, ue_desc_p))
            == NULL) {
        NGAP_DEBUG("Found usable gNB_ue_ngap_id: 0x%08x %u(10)\n",
                   ue_desc_p->gNB_ue_ngap_id,
                   ue_desc_p->gNB_ue_ngap_id);
        /* Break the loop as the id is not already used by another UE */
        break;
    }
  } while(1);

  /* mandatory */
  ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_RAN_UE_NGAP_ID;
  ie->value.choice.RAN_UE_NGAP_ID = ue_desc_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_NAS_PDU;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_NAS_PDU;
#if 1
  ie->value.choice.NAS_PDU.buf = ngap_nas_first_req_p->nas_pdu.buffer;
#else
  ie->value.choice.NAS_PDU.buf = malloc(ngap_nas_first_req_p->nas_pdu.length);
  memcpy(ie->value.choice.NAS_PDU.buf,
         ngap_nas_first_req_p->nas_pdu.buffer,
         ngap_nas_first_req_p->nas_pdu.length);
#endif
  ie->value.choice.NAS_PDU.size = ngap_nas_first_req_p->nas_pdu.length;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  
  /* mandatory */
  ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_UserLocationInformation;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_UserLocationInformation;

  ie->value.choice.UserLocationInformation.present = NGAP_UserLocationInformation_PR_userLocationInformationNR;

  ie->value.choice.UserLocationInformation.choice.userLocationInformationNR = 
      CALLOC(1, sizeof(*ie->value.choice.UserLocationInformation.choice.userLocationInformationNR));
  userinfo_nr_p = ie->value.choice.UserLocationInformation.choice.userLocationInformationNR;

  /* Set nRCellIdentity. default userLocationInformationNR */
  MACRO_GNB_ID_TO_CELL_IDENTITY(instance_p->gNB_id,
                                    0, // Cell ID
                                    &userinfo_nr_p->nR_CGI.nRCellIdentity);
  MCC_MNC_TO_TBCD(instance_p->mcc[ue_desc_p->selected_plmn_identity],
                  instance_p->mnc[ue_desc_p->selected_plmn_identity],
                  instance_p->mnc_digit_length[ue_desc_p->selected_plmn_identity],
                  &userinfo_nr_p->nR_CGI.pLMNIdentity);

  /* Set TAI */
  INT24_TO_OCTET_STRING(instance_p->tac, &userinfo_nr_p->tAI.tAC);
  MCC_MNC_TO_PLMNID(instance_p->mcc[ue_desc_p->selected_plmn_identity],
                    instance_p->mnc[ue_desc_p->selected_plmn_identity],
                    instance_p->mnc_digit_length[ue_desc_p->selected_plmn_identity],
                    &userinfo_nr_p->tAI.pLMNIdentity);

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  /* Set the establishment cause according to those provided by RRC */
  DevCheck(ngap_nas_first_req_p->establishment_cause < NGAP_RRC_CAUSE_LAST,
           ngap_nas_first_req_p->establishment_cause, NGAP_RRC_CAUSE_LAST, 0);
  
  /* mandatory */
  ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_RRCEstablishmentCause;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_RRCEstablishmentCause;
  ie->value.choice.RRCEstablishmentCause = ngap_nas_first_req_p->establishment_cause;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  if (ngap_nas_first_req_p->ue_identity.presenceMask & NGAP_UE_IDENTITIES_FiveG_s_tmsi) {
    NGAP_DEBUG("FIVEG_S_TMSI_PRESENT\n");
    ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_FiveG_S_TMSI;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_FiveG_S_TMSI;
    AMF_SETID_TO_BIT_STRING(ngap_nas_first_req_p->ue_identity.s_tmsi.amf_set_id,
                             &ie->value.choice.FiveG_S_TMSI.aMFSetID);
    AMF_SETID_TO_BIT_STRING(ngap_nas_first_req_p->ue_identity.s_tmsi.amf_pointer,
                             &ie->value.choice.FiveG_S_TMSI.aMFPointer);
    M_TMSI_TO_OCTET_STRING(ngap_nas_first_req_p->ue_identity.s_tmsi.m_tmsi,
                             &ie->value.choice.FiveG_S_TMSI.fiveG_TMSI);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }


  /* optional */
  ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_UEContextRequest;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_UEContextRequest;
  ie->value.choice.UEContextRequest = NGAP_UEContextRequest_requested;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
      /* Failed to encode message */
      DevMessage("Failed to encode initial UE message\n");
  }

  /* Update the current NGAP UE state */
  ue_desc_p->ue_state = NGAP_UE_WAITING_CSR;
  /* Assign a stream for this UE :
   * From 3GPP 38.412 7)Transport layers:
   *  Within the SCTP association established between one AMF and gNB pair:
   *  - a single pair of stream identifiers shall be reserved for the sole use
   *      of NGAP elementary procedures that utilize non UE-associated signalling.
   *  - At least one pair of stream identifiers shall be reserved for the sole use
   *      of NGAP elementary procedures that utilize UE-associated signallings.
   *      However a few pairs (i.e. more than one) should be reserved.
   *  - A single UE-associated signalling shall use one SCTP stream and
   *      the stream should not be changed during the communication of the
   *      UE-associated signalling.
   */
  amf_desc_p->nextstream = (amf_desc_p->nextstream + 1) % amf_desc_p->out_streams;

  if ((amf_desc_p->nextstream == 0) && (amf_desc_p->out_streams > 1)) {
    amf_desc_p->nextstream += 1;
  }

  ue_desc_p->tx_stream = amf_desc_p->nextstream;
  /* Send encoded message over sctp */
  ngap_gNB_itti_send_sctp_data_req(instance_p->instance, amf_desc_p->assoc_id,
                                   buffer, length, ue_desc_p->tx_stream);

  return 0;
}

//------------------------------------------------------------------------------
int ngap_gNB_handle_nas_downlink(uint32_t         assoc_id,
                                 uint32_t         stream,
                                 NGAP_NGAP_PDU_t *pdu)
//------------------------------------------------------------------------------
{

  ngap_gNB_amf_data_t             *amf_desc_p        = NULL;
  ngap_gNB_ue_context_t           *ue_desc_p         = NULL;
  ngap_gNB_instance_t             *ngap_gNB_instance = NULL;
  NGAP_DownlinkNASTransport_t     *container;
  NGAP_DownlinkNASTransport_IEs_t *ie;
  NGAP_RAN_UE_NGAP_ID_t            gnb_ue_ngap_id;
  uint64_t                         amf_ue_ngap_id;
  DevAssert(pdu != NULL);

  /* UE-related procedure -> stream != 0 */
  // if (stream == 0) {
  //     NGAP_ERROR("[SCTP %d] Received UE-related procedure on stream == 0\n",
  //                assoc_id);
  //     return -1;
  // }

  if ((amf_desc_p = ngap_gNB_get_AMF(NULL, assoc_id, 0)) == NULL) {
    NGAP_ERROR(
        "[SCTP %d] Received NAS downlink message for non existing AMF context\n",
        assoc_id);
    return -1;
  }

  ngap_gNB_instance = amf_desc_p->ngap_gNB_instance;
  /* Prepare the NGAP message to encode */
  container = &pdu->choice.initiatingMessage->value.choice.DownlinkNASTransport;
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_DownlinkNASTransport_IEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID, true);
  asn_INTEGER2ulong(&(ie->value.choice.AMF_UE_NGAP_ID), &amf_ue_ngap_id);


  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_DownlinkNASTransport_IEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID, true);
  gnb_ue_ngap_id = ie->value.choice.RAN_UE_NGAP_ID;

  if ((ue_desc_p = ngap_gNB_get_ue_context(ngap_gNB_instance,
                   gnb_ue_ngap_id)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received NAS downlink message for non existing UE context gNB_UE_NGAP_ID: 0x%lx\n",
               assoc_id,
               gnb_ue_ngap_id);
    return -1;
  }

  if (0 == ue_desc_p->rx_stream) {
    ue_desc_p->rx_stream = stream;
  } else if (stream != ue_desc_p->rx_stream) {
    NGAP_ERROR("[SCTP %d] Received UE-related procedure on stream %u, expecting %u\n",
               assoc_id, stream, ue_desc_p->rx_stream);
    return -1;
  }

  /* Is it the first outcome of the AMF for this UE ? If so store the amf
   * UE ngap id.
   */
  if (ue_desc_p->amf_ue_ngap_id == 0) {
    ue_desc_p->amf_ue_ngap_id = amf_ue_ngap_id;
  } else {
    /* We already have a amf ue ngap id check the received is the same */
    if (ue_desc_p->amf_ue_ngap_id != amf_ue_ngap_id) {
        NGAP_ERROR("[SCTP %d] Mismatch in AMF UE NGAP ID (0x%lx != 0x%"PRIx64"\n",
                   assoc_id,
                   amf_ue_ngap_id,
                   (uint64_t)ue_desc_p->amf_ue_ngap_id
                  );
        return -1;
    }
  }

  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_DownlinkNASTransport_IEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_NAS_PDU, true);
  /* Forward the NAS PDU to NR-RRC */
  ngap_gNB_itti_send_nas_downlink_ind(ngap_gNB_instance->instance,
                                      ue_desc_p->ue_initial_id,
                                      ue_desc_p->gNB_ue_ngap_id,
                                      ie->value.choice.NAS_PDU.buf,
                                      ie->value.choice.NAS_PDU.size);

  return 0;
}

//------------------------------------------------------------------------------
int ngap_gNB_nas_uplink(instance_t instance, ngap_uplink_nas_t *ngap_uplink_nas_p)
//------------------------------------------------------------------------------
{
  struct ngap_gNB_ue_context_s  *ue_context_p;
  ngap_gNB_instance_t           *ngap_gNB_instance_p;
  NGAP_NGAP_PDU_t                pdu;
  NGAP_UplinkNASTransport_t     *out;
  NGAP_UplinkNASTransport_IEs_t *ie;
  NGAP_UserLocationInformationNR_t *userinfo_nr_p = NULL;
  uint8_t  *buffer;
  uint32_t  length;
  DevAssert(ngap_uplink_nas_p != NULL);
  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p, ngap_uplink_nas_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: %08x\n",
              ngap_uplink_nas_p->gNB_ue_ngap_id);
    return -1;
  }

  /* Uplink NAS transport can occur either during an ngap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED ||
          ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
    NGAP_WARN("You are attempting to send NAS data over non-connected "
              "gNB ue ngap id: %u, current state: %d\n",
              ngap_uplink_nas_p->gNB_ue_ngap_id, ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (NGAP_InitiatingMessage_t *)calloc(1,sizeof(NGAP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = NGAP_ProcedureCode_id_UplinkNASTransport;
  pdu.choice.initiatingMessage->criticality = NGAP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = NGAP_InitiatingMessage__value_PR_UplinkNASTransport;
  out = &pdu.choice.initiatingMessage->value.choice.UplinkNASTransport;
  /* mandatory */
  ie = (NGAP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(NGAP_UplinkNASTransport_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_UplinkNASTransport_IEs__value_PR_AMF_UE_NGAP_ID;
  //ie->value.choice.AMF_UE_NGAP_ID = ue_context_p->amf_ue_ngap_id;
  asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(NGAP_UplinkNASTransport_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_UplinkNASTransport_IEs__value_PR_RAN_UE_NGAP_ID;
  ie->value.choice.RAN_UE_NGAP_ID = ue_context_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(NGAP_UplinkNASTransport_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_NAS_PDU;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_UplinkNASTransport_IEs__value_PR_NAS_PDU;
  ie->value.choice.NAS_PDU.buf = ngap_uplink_nas_p->nas_pdu.buffer;
  ie->value.choice.NAS_PDU.size = ngap_uplink_nas_p->nas_pdu.length;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(NGAP_UplinkNASTransport_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_UserLocationInformation;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_UplinkNASTransport_IEs__value_PR_UserLocationInformation;

  ie->value.choice.UserLocationInformation.present = NGAP_UserLocationInformation_PR_userLocationInformationNR;
  ie->value.choice.UserLocationInformation.choice.userLocationInformationNR = CALLOC(1,sizeof(struct NGAP_UserLocationInformationNR));
  userinfo_nr_p = ie->value.choice.UserLocationInformation.choice.userLocationInformationNR;

  /* Set nRCellIdentity. default userLocationInformationNR */
  MACRO_GNB_ID_TO_CELL_IDENTITY(ngap_gNB_instance_p->gNB_id,
                                    0, // Cell ID
                                    &userinfo_nr_p->nR_CGI.nRCellIdentity);
  MCC_MNC_TO_TBCD(ngap_gNB_instance_p->mcc[ue_context_p->selected_plmn_identity],
                  ngap_gNB_instance_p->mnc[ue_context_p->selected_plmn_identity],
                  ngap_gNB_instance_p->mnc_digit_length[ue_context_p->selected_plmn_identity],
                  &userinfo_nr_p->nR_CGI.pLMNIdentity);

  /* Set TAI */
  INT24_TO_OCTET_STRING(ngap_gNB_instance_p->tac, &userinfo_nr_p->tAI.tAC);
  MCC_MNC_TO_PLMNID(ngap_gNB_instance_p->mcc[ue_context_p->selected_plmn_identity],
                    ngap_gNB_instance_p->mnc[ue_context_p->selected_plmn_identity],
                    ngap_gNB_instance_p->mnc_digit_length[ue_context_p->selected_plmn_identity],
                    &userinfo_nr_p->tAI.pLMNIdentity);


  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NGAP_ERROR("Failed to encode uplink NAS transport\n");
    /* Encode procedure has failed... */
    return -1;
  }

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return 0;
}


//------------------------------------------------------------------------------
int ngap_gNB_nas_non_delivery_ind(instance_t instance,
                                  ngap_nas_non_delivery_ind_t *ngap_nas_non_delivery_ind)
//------------------------------------------------------------------------------
{
  struct ngap_gNB_ue_context_s        *ue_context_p;
  ngap_gNB_instance_t                 *ngap_gNB_instance_p;
  NGAP_NGAP_PDU_t                      pdu;
  NGAP_NASNonDeliveryIndication_t     *out;
  NGAP_NASNonDeliveryIndication_IEs_t *ie;
  uint8_t  *buffer;
  uint32_t  length;
  DevAssert(ngap_nas_non_delivery_ind != NULL);
  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p, ngap_nas_non_delivery_ind->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: %08x\n",
              ngap_nas_non_delivery_ind->gNB_ue_ngap_id);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (NGAP_InitiatingMessage_t *)calloc(1,sizeof(NGAP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = NGAP_ProcedureCode_id_NASNonDeliveryIndication;
  pdu.choice.initiatingMessage->criticality = NGAP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = NGAP_InitiatingMessage__value_PR_NASNonDeliveryIndication;
  out = &pdu.choice.initiatingMessage->value.choice.NASNonDeliveryIndication;
  /* mandatory */
  ie = (NGAP_NASNonDeliveryIndication_IEs_t *)calloc(1, sizeof(NGAP_NASNonDeliveryIndication_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_NASNonDeliveryIndication_IEs__value_PR_AMF_UE_NGAP_ID;
  asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_NASNonDeliveryIndication_IEs_t *)calloc(1, sizeof(NGAP_NASNonDeliveryIndication_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_NASNonDeliveryIndication_IEs__value_PR_RAN_UE_NGAP_ID;
  ie->value.choice.RAN_UE_NGAP_ID = ue_context_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_NASNonDeliveryIndication_IEs_t *)calloc(1, sizeof(NGAP_NASNonDeliveryIndication_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_NAS_PDU;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_NASNonDeliveryIndication_IEs__value_PR_NAS_PDU;
  ie->value.choice.NAS_PDU.buf = ngap_nas_non_delivery_ind->nas_pdu.buffer;
  ie->value.choice.NAS_PDU.size = ngap_nas_non_delivery_ind->nas_pdu.length;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_NASNonDeliveryIndication_IEs_t *)calloc(1, sizeof(NGAP_NASNonDeliveryIndication_IEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_Cause;
  ie->criticality = NGAP_Criticality_ignore;
  /* Send a dummy cause */
  ie->value.present = NGAP_NASNonDeliveryIndication_IEs__value_PR_Cause;
  ie->value.choice.Cause.present = NGAP_Cause_PR_radioNetwork;
  ie->value.choice.Cause.choice.radioNetwork = NGAP_CauseRadioNetwork_radio_connection_with_ue_lost;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NGAP_ERROR("Failed to encode NAS NON delivery indication\n");
    /* Encode procedure has failed... */
    return -1;
  }

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return 0;
}

//------------------------------------------------------------------------------
int ngap_gNB_initial_ctxt_resp(
    instance_t instance, ngap_initial_context_setup_resp_t *initial_ctxt_resp_p)
//------------------------------------------------------------------------------
{

  ngap_gNB_instance_t                   *ngap_gNB_instance_p = NULL;
  struct ngap_gNB_ue_context_s          *ue_context_p        = NULL;
  NGAP_NGAP_PDU_t                        pdu;
  NGAP_InitialContextSetupResponse_t    *out;
  NGAP_InitialContextSetupResponseIEs_t *ie;
  uint8_t  *buffer = NULL;
  uint8_t pdusessionTransfer_buffer[1000];
  uint32_t length;
  int      i;
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };

  
  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(initial_ctxt_resp_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                      initial_ctxt_resp_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%08x\n",
              initial_ctxt_resp_p->gNB_ue_ngap_id);
    return -1;
  }

  /* Uplink NAS transport can occur either during an ngap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED ||
          ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
    NGAP_WARN("You are attempting to send NAS data over non-connected "
              "gNB ue ngap id: %08x, current state: %d\n",
              initial_ctxt_resp_p->gNB_ue_ngap_id, ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (NGAP_SuccessfulOutcome_t *)calloc(1,sizeof(struct NGAP_SuccessfulOutcome));
  pdu.choice.successfulOutcome->procedureCode = NGAP_ProcedureCode_id_InitialContextSetup;
  pdu.choice.successfulOutcome->criticality = NGAP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = NGAP_SuccessfulOutcome__value_PR_InitialContextSetupResponse;
  out = &pdu.choice.successfulOutcome->value.choice.InitialContextSetupResponse;
  /* mandatory */
  ie = (NGAP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(NGAP_InitialContextSetupResponseIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_InitialContextSetupResponseIEs__value_PR_AMF_UE_NGAP_ID;
  //ie->value.choice.AMF_UE_NGAP_ID = ue_context_p->amf_ue_ngap_id;
  asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(NGAP_InitialContextSetupResponseIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_InitialContextSetupResponseIEs__value_PR_RAN_UE_NGAP_ID;
  ie->value.choice.RAN_UE_NGAP_ID = initial_ctxt_resp_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  if (initial_ctxt_resp_p->nb_of_pdusessions){
    ie = (NGAP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(NGAP_InitialContextSetupResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListCxtRes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_InitialContextSetupResponseIEs__value_PR_PDUSessionResourceSetupListCxtRes;
    for (i = 0; i < initial_ctxt_resp_p->nb_of_pdusessions; i++) {
      NGAP_PDUSessionResourceSetupItemCxtRes_t *item;
      NGAP_PDUSessionResourceSetupResponseTransfer_t     *pdusessionTransfer_p = NULL;

      /* mandatory */
      item = (NGAP_PDUSessionResourceSetupItemCxtRes_t *)calloc(1, sizeof(NGAP_PDUSessionResourceSetupItemCxtRes_t));

      /* pDUSessionID */
      item->pDUSessionID = initial_ctxt_resp_p->pdusessions[i].pdusession_id;

      /* dLQosFlowPerTNLInformation */
      pdusessionTransfer_p = (NGAP_PDUSessionResourceSetupResponseTransfer_t *)calloc(1, sizeof(NGAP_PDUSessionResourceSetupResponseTransfer_t));

      pdusessionTransfer_p->dLQosFlowPerTNLInformation.uPTransportLayerInformation.present = NGAP_UPTransportLayerInformation_PR_gTPTunnel;
    
      pdusessionTransfer_p->dLQosFlowPerTNLInformation.uPTransportLayerInformation.choice.gTPTunnel = (NGAP_GTPTunnel_t *)calloc(1, sizeof(NGAP_GTPTunnel_t));
      struct NGAP_GTPTunnel *tmp=pdusessionTransfer_p->dLQosFlowPerTNLInformation.uPTransportLayerInformation.choice.gTPTunnel;
      GTP_TEID_TO_ASN1(initial_ctxt_resp_p->pdusessions[i].gtp_teid, &tmp->gTP_TEID);

      tmp->transportLayerAddress.buf = malloc(initial_ctxt_resp_p->pdusessions[i].gNB_addr.length);

      memcpy(tmp->transportLayerAddress.buf,
             initial_ctxt_resp_p->pdusessions[i].gNB_addr.buffer,
             initial_ctxt_resp_p->pdusessions[i].gNB_addr.length);
      tmp->transportLayerAddress.size = initial_ctxt_resp_p->pdusessions[i].gNB_addr.length;
      tmp->transportLayerAddress.bits_unused = 0;

      NGAP_DEBUG("initial_ctxt_resp_p: pdusession ID %ld\n",
                 item->pDUSessionID);

      /* associatedQosFlowList. number of 1? */
      for(int j=0; j < initial_ctxt_resp_p->pdusessions[i].nb_of_qos_flow; j++) {
        NGAP_AssociatedQosFlowItem_t *ass_qos_item_p;
        ass_qos_item_p = (NGAP_AssociatedQosFlowItem_t *)calloc(1, sizeof(NGAP_AssociatedQosFlowItem_t));

        /* qosFlowIdentifier */
        ass_qos_item_p->qosFlowIdentifier = initial_ctxt_resp_p->pdusessions[i].associated_qos_flows[j].qfi;

        /* qosFlowMappingIndication */
        //if(initial_ctxt_resp_p->pdusessions[i].associated_qos_flows[j].qos_flow_mapping_ind != QOSFLOW_MAPPING_INDICATION_NON) {
        //  ass_qos_item_p->qosFlowMappingIndication = malloc(sizeof(*ass_qos_item_p->qosFlowMappingIndication));
        //  *ass_qos_item_p->qosFlowMappingIndication = initial_ctxt_resp_p->pdusessions[i].associated_qos_flows[j].qos_flow_mapping_ind;
        // }
        ASN_SEQUENCE_ADD(&pdusessionTransfer_p->dLQosFlowPerTNLInformation.associatedQosFlowList.list, ass_qos_item_p);
      }
               
      //res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_NGAP_PDUSessionResourceSetupResponseTransfer, pdusessionTransfer_p);
      //item->pDUSessionResourceSetupResponseTransfer.buf = res.buffer;
      //item->pDUSessionResourceSetupResponseTransfer.size = res.result.encoded;

      if (asn1_xer_print) {
        xer_fprint(stdout, &asn_DEF_NGAP_PDUSessionResourceSetupResponseTransfer, pdusessionTransfer_p);
      }

      memset(pdusessionTransfer_buffer, 0, 1000);
      asn_enc_rval_t  enc_rval = aper_encode_to_buffer(&asn_DEF_NGAP_PDUSessionResourceSetupResponseTransfer, 
          NULL, 
          pdusessionTransfer_p, 
          pdusessionTransfer_buffer,1000);
      AssertFatal (enc_rval.encoded > 0, "ASN1 message encoding failed (%s, %lu)!\n", enc_rval.failed_type->name, enc_rval.encoded);
      item->pDUSessionResourceSetupResponseTransfer.buf = pdusessionTransfer_buffer; 
      item->pDUSessionResourceSetupResponseTransfer.size = enc_rval.encoded;

      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceSetupResponseTransfer, pdusessionTransfer_p);
    
      ASN_SEQUENCE_ADD(&ie->value.choice.PDUSessionResourceSetupListCxtRes.list, item);
    }

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }
  /* optional */
  if (initial_ctxt_resp_p->nb_of_pdusessions_failed) {
    ie = (NGAP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(NGAP_InitialContextSetupResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceFailedToSetupListCxtRes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_InitialContextSetupResponseIEs__value_PR_PDUSessionResourceFailedToSetupListCxtRes;

    for (i = 0; i < initial_ctxt_resp_p->nb_of_pdusessions_failed; i++) {
        NGAP_PDUSessionResourceFailedToSetupItemCxtRes_t *item= calloc(1, sizeof *item);
        NGAP_PDUSessionResourceSetupUnsuccessfulTransfer_t *pdusessionUnTransfer_p = calloc(1, sizeof *pdusessionUnTransfer_p);
    
        /* pDUSessionID */
        item->pDUSessionID = initial_ctxt_resp_p->pdusessions_failed[i].pdusession_id;

        /* cause */

        pdusessionUnTransfer_p->cause.present = initial_ctxt_resp_p->pdusessions_failed[i].cause;
        switch(pdusessionUnTransfer_p->cause.present) {
          case NGAP_Cause_PR_radioNetwork:
              pdusessionUnTransfer_p->cause.choice.radioNetwork = initial_ctxt_resp_p->pdusessions_failed[i].cause_value;
              break;

          case NGAP_Cause_PR_transport:
              pdusessionUnTransfer_p->cause.choice.transport = initial_ctxt_resp_p->pdusessions_failed[i].cause_value;
              break;

          case NGAP_Cause_PR_nas:
              pdusessionUnTransfer_p->cause.choice.nas = initial_ctxt_resp_p->pdusessions_failed[i].cause_value;
              break;

          case NGAP_Cause_PR_protocol:
              pdusessionUnTransfer_p->cause.choice.protocol = initial_ctxt_resp_p->pdusessions_failed[i].cause_value;
              break;

          case NGAP_Cause_PR_misc:
              pdusessionUnTransfer_p->cause.choice.misc = initial_ctxt_resp_p->pdusessions_failed[i].cause_value;
              break;

          case NGAP_Cause_PR_NOTHING:
          default:
              break;
        }

        NGAP_DEBUG("initial context setup response: failed pdusession ID %ld\n", item->pDUSessionID);

        memset(&res, 0, sizeof(res));
        res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_NGAP_PDUSessionResourceSetupUnsuccessfulTransfer, pdusessionUnTransfer_p);
        item->pDUSessionResourceSetupUnsuccessfulTransfer.buf = res.buffer;
        item->pDUSessionResourceSetupUnsuccessfulTransfer.size = res.result.encoded;

        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceSetupUnsuccessfulTransfer, pdusessionUnTransfer_p);
    
        ASN_SEQUENCE_ADD(&ie->value.choice.PDUSessionResourceFailedToSetupListCxtRes.list, item);
    }

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  if (0) {
    ie = (NGAP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(NGAP_InitialContextSetupResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_InitialContextSetupResponseIEs__value_PR_CriticalityDiagnostics;
    // ie->value.choice.CriticalityDiagnostics =;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  if (asn1_xer_print) {
    xer_fprint(stdout, &asn_DEF_NGAP_NGAP_PDU, &pdu);
  }

  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NGAP_ERROR("Failed to encode InitialContextSetupResponse\n");
    /* Encode procedure has failed... */
    return -1;
  }

    /* UE associated signalling -> use the allocated stream */
    LOG_I(NR_RRC,"Send message to sctp: NGAP_InitialContextSetupResponse\n");
    ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                     ue_context_p->amf_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);

    return 0;
}

//------------------------------------------------------------------------------
int ngap_gNB_ue_capabilities(instance_t instance,
                             ngap_ue_cap_info_ind_t *ue_cap_info_ind_p)
//------------------------------------------------------------------------------
{
  ngap_gNB_instance_t          *ngap_gNB_instance_p;
  struct ngap_gNB_ue_context_s *ue_context_p;
  NGAP_NGAP_PDU_t                       pdu;
  NGAP_UERadioCapabilityInfoIndication_t    *out;
  NGAP_UERadioCapabilityInfoIndicationIEs_t *ie;
  uint8_t  *buffer;
  uint32_t length;
  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(ue_cap_info_ind_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                      ue_cap_info_ind_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: %u\n",
              ue_cap_info_ind_p->gNB_ue_ngap_id);
    return -1;
  }

  /* UE radio capabilities message can occur either during an ngap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED ||
          ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
    NGAP_WARN("You are attempting to send NAS data over non-connected "
              "gNB ue ngap id: %u, current state: %d\n",
              ue_cap_info_ind_p->gNB_ue_ngap_id, ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (NGAP_InitiatingMessage_t *)calloc(1,sizeof(NGAP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = NGAP_ProcedureCode_id_UERadioCapabilityInfoIndication;
  pdu.choice.initiatingMessage->criticality = NGAP_Criticality_ignore;
  pdu.choice.initiatingMessage->value.present = NGAP_InitiatingMessage__value_PR_UERadioCapabilityInfoIndication;
  out = &pdu.choice.initiatingMessage->value.choice.UERadioCapabilityInfoIndication;
  /* mandatory */
  ie = (NGAP_UERadioCapabilityInfoIndicationIEs_t *)calloc(1, sizeof(NGAP_UERadioCapabilityInfoIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_UERadioCapabilityInfoIndicationIEs__value_PR_AMF_UE_NGAP_ID;
  //ie->value.choice.AMF_UE_NGAP_ID = ue_context_p->amf_ue_ngap_id;
  asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_UERadioCapabilityInfoIndicationIEs_t *)calloc(1, sizeof(NGAP_UERadioCapabilityInfoIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_UERadioCapabilityInfoIndicationIEs__value_PR_RAN_UE_NGAP_ID;
  ie->value.choice.RAN_UE_NGAP_ID = ue_cap_info_ind_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_UERadioCapabilityInfoIndicationIEs_t *)calloc(1, sizeof(NGAP_UERadioCapabilityInfoIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_UERadioCapability;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_UERadioCapabilityInfoIndicationIEs__value_PR_UERadioCapability;
  ie->value.choice.UERadioCapability.buf = ue_cap_info_ind_p->ue_radio_cap.buffer;
  ie->value.choice.UERadioCapability.size = ue_cap_info_ind_p->ue_radio_cap.length;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* optional */
  //NGAP_UERadioCapabilityForPaging TBD
  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    /* Encode procedure has failed... */
    NGAP_ERROR("Failed to encode UE radio capabilities indication\n");
    return -1;
  }

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);
  return 0;
}

//------------------------------------------------------------------------------
int ngap_gNB_pdusession_setup_resp(instance_t instance,
                              ngap_pdusession_setup_resp_t *pdusession_setup_resp_p)
//------------------------------------------------------------------------------
{
  ngap_gNB_instance_t          *ngap_gNB_instance_p = NULL;
  struct ngap_gNB_ue_context_s *ue_context_p        = NULL;
  NGAP_NGAP_PDU_t               pdu;
  NGAP_PDUSessionResourceSetupResponse_t    *out;
  NGAP_PDUSessionResourceSetupResponseIEs_t *ie;
  uint8_t  *buffer  = NULL;
  uint32_t length;

  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(pdusession_setup_resp_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                      pdusession_setup_resp_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%08x\n",
              pdusession_setup_resp_p->gNB_ue_ngap_id);
    return -1;
  }

  /* Uplink NAS transport can occur either during an ngap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED ||
          ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
    NGAP_WARN("You are attempting to send NAS data over non-connected "
              "gNB ue ngap id: %08x, current state: %d\n",
              pdusession_setup_resp_p->gNB_ue_ngap_id, ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = calloc(1,sizeof *pdu.choice.successfulOutcome);
  pdu.choice.successfulOutcome->procedureCode = NGAP_ProcedureCode_id_PDUSessionResourceSetup;
  pdu.choice.successfulOutcome->criticality = NGAP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = NGAP_SuccessfulOutcome__value_PR_PDUSessionResourceSetupResponse;
  out = &pdu.choice.successfulOutcome->value.choice.PDUSessionResourceSetupResponse;
  /* mandatory */
  ie = calloc(1, sizeof *ie);
  ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_PDUSessionResourceSetupResponseIEs__value_PR_AMF_UE_NGAP_ID;
  asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = calloc(1, sizeof *ie );
  ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_PDUSessionResourceSetupResponseIEs__value_PR_RAN_UE_NGAP_ID;
  ie->value.choice.RAN_UE_NGAP_ID = pdusession_setup_resp_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  if (pdusession_setup_resp_p->nb_of_pdusessions > 0) {
    ie = calloc(1, sizeof *ie );
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListSURes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceSetupResponseIEs__value_PR_PDUSessionResourceSetupListSURes;

    for (int i = 0; i < pdusession_setup_resp_p->nb_of_pdusessions; i++) {
      NGAP_PDUSessionResourceSetupItemSURes_t *item=calloc(1, sizeof *item );
      NGAP_PDUSessionResourceSetupResponseTransfer_t *pdusessionTransfer_p = calloc(1, sizeof *pdusessionTransfer_p );

      /* pDUSessionID */
      item->pDUSessionID = pdusession_setup_resp_p->pdusessions[i].pdusession_id;

      /* dLQosFlowPerTNLInformation */

      pdusessionTransfer_p->dLQosFlowPerTNLInformation.uPTransportLayerInformation.present = NGAP_UPTransportLayerInformation_PR_gTPTunnel;
      pdusessionTransfer_p->dLQosFlowPerTNLInformation.uPTransportLayerInformation.choice.gTPTunnel =
                calloc(1, sizeof(struct NGAP_GTPTunnel));
      struct NGAP_GTPTunnel *tmp=pdusessionTransfer_p->dLQosFlowPerTNLInformation.uPTransportLayerInformation.choice.gTPTunnel;
      GTP_TEID_TO_ASN1(pdusession_setup_resp_p->pdusessions[i].gtp_teid, &tmp->gTP_TEID);
      if (pdusession_setup_resp_p->pdusessions[i].gNB_addr.length) {
        tmp->transportLayerAddress.buf = malloc(pdusession_setup_resp_p->pdusessions[i].gNB_addr.length);

        memcpy(tmp->transportLayerAddress.buf,
               pdusession_setup_resp_p->pdusessions[i].gNB_addr.buffer,
               pdusession_setup_resp_p->pdusessions[i].gNB_addr.length);
        tmp->transportLayerAddress.size = pdusession_setup_resp_p->pdusessions[i].gNB_addr.length;
        tmp->transportLayerAddress.bits_unused = 0;

        NGAP_DEBUG("pdusession_setup_resp_p: pdusession ID %ld, gnb_addr %d.%d.%d.%d, SIZE %ld \n",
                   item->pDUSessionID,
                   tmp->transportLayerAddress.buf[0],
                   tmp->transportLayerAddress.buf[1],
                   tmp->transportLayerAddress.buf[2],
                   tmp->transportLayerAddress.buf[3],
                   tmp->transportLayerAddress.size);
      }

      /* associatedQosFlowList. number of 1? */
      for(int j=0; j < pdusession_setup_resp_p->pdusessions[i].nb_of_qos_flow; j++) {
        NGAP_AssociatedQosFlowItem_t *ass_qos_item_p = calloc(1, sizeof *ass_qos_item_p);

        /* qosFlowIdentifier */
        ass_qos_item_p->qosFlowIdentifier = pdusession_setup_resp_p->pdusessions[i].associated_qos_flows[j].qfi;

        /* qosFlowMappingIndication */
        //if(pdusession_setup_resp_p->pdusessions[i].associated_qos_flows[j].qos_flow_mapping_ind != QOSFLOW_MAPPING_INDICATION_NON) {
        //  ass_qos_item_p->qosFlowMappingIndication = malloc(sizeof(*ass_qos_item_p->qosFlowMappingIndication));
        //  *ass_qos_item_p->qosFlowMappingIndication = pdusession_setup_resp_p->pdusessions[i].associated_qos_flows[j].qos_flow_mapping_ind;
        //}
        ASN_SEQUENCE_ADD(&pdusessionTransfer_p->dLQosFlowPerTNLInformation.associatedQosFlowList.list, ass_qos_item_p);
      }

      asn_encode_to_new_buffer_result_t res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER,
								       &asn_DEF_NGAP_PDUSessionResourceSetupResponseTransfer, pdusessionTransfer_p);
      AssertFatal (res.buffer, "ASN1 message encoding failed (%s, %lu)!\n", res.result.failed_type->name, res.result.encoded);
      item->pDUSessionResourceSetupResponseTransfer.buf = res.buffer;
      item->pDUSessionResourceSetupResponseTransfer.size = res.result.encoded;

      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceSetupResponseTransfer, pdusessionTransfer_p);

      ASN_SEQUENCE_ADD(&ie->value.choice.PDUSessionResourceSetupListSURes.list, item);
    }
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  if (pdusession_setup_resp_p->nb_of_pdusessions_failed > 0) {
    ie = calloc(1, sizeof *ie);
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceFailedToSetupListSURes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceSetupResponseIEs__value_PR_PDUSessionResourceFailedToSetupListSURes;

    for (int i = 0; i < pdusession_setup_resp_p->nb_of_pdusessions_failed; i++) {
      LOG_W(NGAP,"add a failed session\n");
      NGAP_PDUSessionResourceFailedToSetupItemSURes_t *item=calloc(1, sizeof *item);
      NGAP_PDUSessionResourceSetupUnsuccessfulTransfer_t *pdusessionUnTransfer_p = calloc(1, sizeof *pdusessionUnTransfer_p);

      /* pDUSessionID */
      item->pDUSessionID = pdusession_setup_resp_p->pdusessions_failed[i].pdusession_id;

      /* cause */
      pdusessionUnTransfer_p->cause.present = pdusession_setup_resp_p->pdusessions_failed[i].cause;
      switch(pdusessionUnTransfer_p->cause.present) {
        case NGAP_Cause_PR_radioNetwork:
            pdusessionUnTransfer_p->cause.choice.radioNetwork = pdusession_setup_resp_p->pdusessions_failed[i].cause_value;
            break;

        case NGAP_Cause_PR_transport:
            pdusessionUnTransfer_p->cause.choice.transport = pdusession_setup_resp_p->pdusessions_failed[i].cause_value;
            break;

        case NGAP_Cause_PR_nas:
            pdusessionUnTransfer_p->cause.choice.nas = pdusession_setup_resp_p->pdusessions_failed[i].cause_value;
            break;

        case NGAP_Cause_PR_protocol:
            pdusessionUnTransfer_p->cause.choice.protocol = pdusession_setup_resp_p->pdusessions_failed[i].cause_value;
            break;

        case NGAP_Cause_PR_misc:
            pdusessionUnTransfer_p->cause.choice.misc = pdusession_setup_resp_p->pdusessions_failed[i].cause_value;
            break;

        case NGAP_Cause_PR_NOTHING:
        default:
            break;
      }

      NGAP_DEBUG("pdusession setup response: failed pdusession ID %ld\n", item->pDUSessionID);

     asn_encode_to_new_buffer_result_t res =
	       asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER,
				  &asn_DEF_NGAP_PDUSessionResourceSetupUnsuccessfulTransfer, pdusessionUnTransfer_p);     
      item->pDUSessionResourceSetupUnsuccessfulTransfer.buf = res.buffer;
      item->pDUSessionResourceSetupUnsuccessfulTransfer.size = res.result.encoded;

      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceSetupUnsuccessfulTransfer, pdusessionUnTransfer_p);

      ASN_SEQUENCE_ADD(&ie->value.choice.PDUSessionResourceFailedToSetupListSURes.list, item);
    }

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  if (0) {
    ie = calloc(1, sizeof *ie);
      ie->id = NGAP_ProtocolIE_ID_id_CriticalityDiagnostics;
      ie->criticality = NGAP_Criticality_ignore;
      ie->value.present = NGAP_PDUSessionResourceSetupResponseIEs__value_PR_CriticalityDiagnostics;
      // ie->value.choice.CriticalityDiagnostics = ;
      ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }


  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
      NGAP_ERROR("Failed to encode uplink transport\n");
      /* Encode procedure has failed... */
      return -1;
  }

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);
  return 0;
}

//------------------------------------------------------------------------------
int ngap_gNB_pdusession_modify_resp(instance_t instance,
                               ngap_pdusession_modify_resp_t *pdusession_modify_resp_p)
//------------------------------------------------------------------------------
{
  ngap_gNB_instance_t                         *ngap_gNB_instance_p = NULL;
  struct ngap_gNB_ue_context_s                *ue_context_p        = NULL;
  NGAP_NGAP_PDU_t                             pdu;
  NGAP_PDUSessionResourceModifyResponse_t     *out;
  NGAP_PDUSessionResourceModifyResponseIEs_t  *ie;
  uint8_t  *buffer  = NULL;
  uint32_t length;
  int      i;
  asn_encode_to_new_buffer_result_t res = { NULL, {0, NULL, NULL} };
  
  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(pdusession_modify_resp_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                      pdusession_modify_resp_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%08x\n",
              pdusession_modify_resp_p->gNB_ue_ngap_id);
    return -1;
  }

  /* Uplink NAS transport can occur either during an ngap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED ||
          ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
    NGAP_WARN("You are attempting to send NAS data over non-connected "
              "gNB ue ngap id: %08x, current state: %d\n",
              pdusession_modify_resp_p->gNB_ue_ngap_id, ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (NGAP_SuccessfulOutcome_t *)calloc(1,sizeof(struct NGAP_SuccessfulOutcome));
  pdu.choice.successfulOutcome->procedureCode = NGAP_ProcedureCode_id_PDUSessionResourceModify;
  pdu.choice.successfulOutcome->criticality = NGAP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = NGAP_SuccessfulOutcome__value_PR_PDUSessionResourceModifyResponse;
  out = &pdu.choice.successfulOutcome->value.choice.PDUSessionResourceModifyResponse;
  /* mandatory */
  ie = (NGAP_PDUSessionResourceModifyResponseIEs_t *)calloc(1, sizeof(NGAP_PDUSessionResourceModifyResponseIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_PDUSessionResourceModifyResponseIEs__value_PR_AMF_UE_NGAP_ID;
  asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_PDUSessionResourceModifyResponseIEs_t *)calloc(1, sizeof(NGAP_PDUSessionResourceModifyResponseIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_PDUSessionResourceModifyResponseIEs__value_PR_RAN_UE_NGAP_ID;
  ie->value.choice.RAN_UE_NGAP_ID = pdusession_modify_resp_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* PDUSessionResourceModifyListModRes optional */
  if (pdusession_modify_resp_p->nb_of_pdusessions > 0) {
    ie = (NGAP_PDUSessionResourceModifyResponseIEs_t *)calloc(1, sizeof(NGAP_PDUSessionResourceModifyResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceModifyListModRes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceModifyResponseIEs__value_PR_PDUSessionResourceModifyListModRes;

    for (i = 0; i < pdusession_modify_resp_p->nb_of_pdusessions; i++) {
      NGAP_PDUSessionResourceModifyItemModRes_t *item;
      NGAP_PDUSessionResourceModifyResponseTransfer_t *transfer_p = NULL;
  
      item = (NGAP_PDUSessionResourceModifyItemModRes_t *)calloc(1, sizeof(NGAP_PDUSessionResourceModifyItemModRes_t));
      item->pDUSessionID = pdusession_modify_resp_p->pdusessions[i].pdusession_id;

      transfer_p = (NGAP_PDUSessionResourceModifyResponseTransfer_t *)calloc(1, sizeof(NGAP_PDUSessionResourceModifyResponseTransfer_t));
      transfer_p->qosFlowAddOrModifyResponseList = (NGAP_QosFlowAddOrModifyResponseList_t *)calloc(1, sizeof(NGAP_QosFlowAddOrModifyResponseList_t));

      uint8_t qos_flow_index;
      for (qos_flow_index = 0; qos_flow_index < pdusession_modify_resp_p->pdusessions[i].nb_of_qos_flow; qos_flow_index++) {
        NGAP_QosFlowAddOrModifyResponseItem_t *qosFlowAddOrModifyResponseItem_p = calloc(1, sizeof(NGAP_QosFlowAddOrModifyResponseItem_t));
        qosFlowAddOrModifyResponseItem_p->qosFlowIdentifier = 
          pdusession_modify_resp_p->pdusessions[i].qos[qos_flow_index].qfi;
        ASN_SEQUENCE_ADD(&transfer_p->qosFlowAddOrModifyResponseList->list, qosFlowAddOrModifyResponseItem_p);
      }

      memset(&res, 0, sizeof(res));
      res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_NGAP_PDUSessionResourceModifyResponseTransfer, transfer_p);
      item->pDUSessionResourceModifyResponseTransfer.buf = res.buffer;
      item->pDUSessionResourceModifyResponseTransfer.size = res.result.encoded;

      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceModifyResponseTransfer, transfer_p);

      NGAP_DEBUG("pdusession_modify_resp: modified pdusession ID %ld\n", item->pDUSessionID);
      ASN_SEQUENCE_ADD(&ie->value.choice.PDUSessionResourceModifyListModRes.list, item);
    }

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  if (pdusession_modify_resp_p->nb_of_pdusessions_failed > 0) {
    ie = (NGAP_PDUSessionResourceModifyResponseIEs_t *)calloc(1, sizeof(NGAP_PDUSessionResourceModifyResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceFailedToModifyListModRes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceModifyResponseIEs__value_PR_PDUSessionResourceFailedToModifyListModRes;

    for (i = 0; i < pdusession_modify_resp_p->nb_of_pdusessions_failed; i++) {
      NGAP_PDUSessionResourceFailedToModifyItemModRes_t *item;
      NGAP_PDUSessionResourceModifyUnsuccessfulTransfer_t *pdusessionTransfer_p = NULL;
  
      item = (NGAP_PDUSessionResourceFailedToModifyItemModRes_t *)calloc(1, sizeof(NGAP_PDUSessionResourceFailedToModifyItemModRes_t));
      item->pDUSessionID = pdusession_modify_resp_p->pdusessions_failed[i].pdusession_id;

      pdusessionTransfer_p = (NGAP_PDUSessionResourceModifyUnsuccessfulTransfer_t *)calloc(1, sizeof(NGAP_PDUSessionResourceModifyUnsuccessfulTransfer_t));
      pdusessionTransfer_p->cause.present = pdusession_modify_resp_p->pdusessions_failed[i].cause;
      
      switch(pdusessionTransfer_p->cause.present) {
      case NGAP_Cause_PR_radioNetwork:
          pdusessionTransfer_p->cause.choice.radioNetwork  = pdusession_modify_resp_p->pdusessions_failed[i].cause_value;
          break;

      case NGAP_Cause_PR_transport:
          pdusessionTransfer_p->cause.choice.transport     = pdusession_modify_resp_p->pdusessions_failed[i].cause_value;
          break;

      case NGAP_Cause_PR_nas:
          pdusessionTransfer_p->cause.choice.nas           = pdusession_modify_resp_p->pdusessions_failed[i].cause_value;
          break;

      case NGAP_Cause_PR_protocol:
          pdusessionTransfer_p->cause.choice.protocol      = pdusession_modify_resp_p->pdusessions_failed[i].cause_value;
          break;

      case NGAP_Cause_PR_misc:
          pdusessionTransfer_p->cause.choice.misc          = pdusession_modify_resp_p->pdusessions_failed[i].cause_value;
          break;

      case NGAP_Cause_PR_NOTHING:
      default:
          break;
      }

      memset(&res, 0, sizeof(res));
      res = asn_encode_to_new_buffer(NULL, ATS_ALIGNED_CANONICAL_PER, &asn_DEF_NGAP_PDUSessionResourceModifyUnsuccessfulTransfer, pdusessionTransfer_p);
      item->pDUSessionResourceModifyUnsuccessfulTransfer.buf = res.buffer;
      item->pDUSessionResourceModifyUnsuccessfulTransfer.size = res.result.encoded;

      ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_PDUSessionResourceModifyUnsuccessfulTransfer, pdusessionTransfer_p);

      
      NGAP_DEBUG("pdusession_modify_resp: failed pdusession ID %ld\n", item->pDUSessionID);
      ASN_SEQUENCE_ADD(&ie->value.choice.PDUSessionResourceFailedToModifyListModRes.list, item);
    }

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  if (0) {
    ie = (NGAP_PDUSessionResourceModifyResponseIEs_t *)calloc(1, sizeof(NGAP_PDUSessionResourceModifyResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_PDUSessionResourceModifyResponseIEs__value_PR_CriticalityDiagnostics;
    // ie->value.choice.CriticalityDiagnostics = ;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  fprintf(stderr, "start encode\n");

  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NGAP_ERROR("Failed to encode uplink transport\n");
    /* Encode procedure has failed... */
    return -1;
  }

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return 0;
}
//------------------------------------------------------------------------------
int ngap_gNB_pdusession_release_resp(instance_t instance,
                                ngap_pdusession_release_resp_t *pdusession_release_resp_p)
//------------------------------------------------------------------------------
{
  ngap_gNB_instance_t            *ngap_gNB_instance_p = NULL;
  struct ngap_gNB_ue_context_s   *ue_context_p        = NULL;
  NGAP_NGAP_PDU_t                 pdu;
  NGAP_PDUSessionResourceReleaseResponse_t    *out;
  NGAP_PDUSessionResourceReleaseResponseIEs_t *ie;
  uint8_t  *buffer  = NULL;
  uint32_t length;
  int      i;
  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  DevAssert(pdusession_release_resp_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                      pdusession_release_resp_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%08x\n",
              pdusession_release_resp_p->gNB_ue_ngap_id);
    return -1;
  }

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (NGAP_SuccessfulOutcome_t *)calloc(1,sizeof(struct NGAP_SuccessfulOutcome));
  pdu.choice.successfulOutcome->procedureCode = NGAP_ProcedureCode_id_PDUSessionResourceRelease;
  pdu.choice.successfulOutcome->criticality = NGAP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = NGAP_SuccessfulOutcome__value_PR_PDUSessionResourceReleaseResponse;
  out = &pdu.choice.successfulOutcome->value.choice.PDUSessionResourceReleaseResponse;
  /* mandatory */
  ie = (NGAP_PDUSessionResourceReleaseResponseIEs_t *)calloc(1, sizeof(NGAP_PDUSessionResourceReleaseResponseIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_PDUSessionResourceReleaseResponseIEs__value_PR_AMF_UE_NGAP_ID;
  asn_uint642INTEGER(&ie->value.choice.AMF_UE_NGAP_ID, ue_context_p->amf_ue_ngap_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (NGAP_PDUSessionResourceReleaseResponseIEs_t *)calloc(1, sizeof(NGAP_PDUSessionResourceReleaseResponseIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_PDUSessionResourceReleaseResponseIEs__value_PR_RAN_UE_NGAP_ID;
  ie->value.choice.RAN_UE_NGAP_ID = pdusession_release_resp_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* optional */
    if (pdusession_release_resp_p->nb_of_pdusessions_released > 0) {
        ie = (NGAP_PDUSessionResourceReleaseResponseIEs_t *)calloc(1, sizeof(NGAP_PDUSessionResourceReleaseResponseIEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_PDUSessionResourceReleasedListRelRes;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_PDUSessionResourceReleaseResponseIEs__value_PR_PDUSessionResourceReleasedListRelRes;

        for (i = 0; i < pdusession_release_resp_p->nb_of_pdusessions_released; i++) {
            NGAP_PDUSessionResourceReleasedItemRelRes_t *item;
            item = (NGAP_PDUSessionResourceReleasedItemRelRes_t *)calloc(1, sizeof(NGAP_PDUSessionResourceReleasedItemRelRes_t));
        
            item->pDUSessionID = pdusession_release_resp_p->pdusession_release[i].pdusession_id;
            if(pdusession_release_resp_p->pdusession_release[i].transfer_length > 0){
                item->pDUSessionResourceReleaseResponseTransfer.size = pdusession_release_resp_p->pdusession_release[i].transfer_length;
                item->pDUSessionResourceReleaseResponseTransfer.buf = malloc(sizeof(uint8_t) * pdusession_release_resp_p->pdusession_release[i].transfer_length);
                memcpy(item->pDUSessionResourceReleaseResponseTransfer.buf,
                       pdusession_release_resp_p->pdusession_release[i].transfer_buffer,
                       pdusession_release_resp_p->pdusession_release[i].transfer_length);
            }else {
                item->pDUSessionResourceReleaseResponseTransfer.size = 0;
                item->pDUSessionResourceReleaseResponseTransfer.buf = NULL;
                NGAP_DEBUG("pdusession_release_resp: transfer_buffer is NULL!\n");
            }
            NGAP_DEBUG("pdusession_release_resp: pdusession ID %ld\n", item->pDUSessionID);
            ASN_SEQUENCE_ADD(&ie->value.choice.PDUSessionResourceReleasedListRelRes.list, item);
        }

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }


  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NGAP_ERROR("Failed to encode release response\n");
    /* Encode procedure has failed... */
    return -1;
  }

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   ue_context_p->amf_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);
  NGAP_INFO("pdusession_release_response sended gNB_UE_NGAP_ID %u  amf_ue_ngap_id %lu nb_of_pdusessions_released %d nb_of_pdusessions_failed %d\n",
            pdusession_release_resp_p->gNB_ue_ngap_id, (uint64_t)ue_context_p->amf_ue_ngap_id,pdusession_release_resp_p->nb_of_pdusessions_released,pdusession_release_resp_p->nb_of_pdusessions_failed);

  return 0;
}

int ngap_gNB_path_switch_req(instance_t instance,
                             ngap_path_switch_req_t *path_switch_req_p)
//------------------------------------------------------------------------------
{
  //TODO

  return 0;
}

int ngap_gNB_generate_PDUSESSION_Modification_Indication(
		instance_t instance,
  ngap_pdusession_modification_ind_t *pdusession_modification_ind)
//-----------------------------------------------------------------------------
{
  //TODO
  return 0;
}


