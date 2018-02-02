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

/*! \file s1ap_eNB_nas_procedures.c
 * \brief S1AP eNb NAS procedure handler
 * \author  S. Roux and Navid Nikaein 
 * \date 2010 - 2015
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _s1ap
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "assertions.h"
#include "conversions.h"

#include "intertask_interface.h"

#include "s1ap_common.h"
#include "s1ap_eNB_defs.h"

#include "s1ap_eNB_itti_messaging.h"

#include "s1ap_ies_defs.h"
#include "s1ap_eNB_encoder.h"
#include "s1ap_eNB_nnsf.h"
#include "s1ap_eNB_ue_context.h"
#include "s1ap_eNB_nas_procedures.h"
#include "s1ap_eNB_management_procedures.h"
#include "msc.h"

//------------------------------------------------------------------------------
int s1ap_eNB_handle_nas_first_req(
  instance_t instance, s1ap_nas_first_req_t *s1ap_nas_first_req_p)
//------------------------------------------------------------------------------
{
  s1ap_eNB_instance_t          *instance_p = NULL;
  struct s1ap_eNB_mme_data_s   *mme_desc_p = NULL;
  struct s1ap_eNB_ue_context_s *ue_desc_p  = NULL;

  s1ap_message message;

  S1ap_InitialUEMessageIEs_t *initial_ue_message_p = NULL;

  uint8_t  *buffer = NULL;
  uint32_t  length = 0;

  DevAssert(s1ap_nas_first_req_p != NULL);

  /* Retrieve the S1AP eNB instance associated with Mod_id */
  instance_p = s1ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  memset(&message, 0, sizeof(s1ap_message));

  message.direction     = S1AP_PDU_PR_initiatingMessage;
  message.procedureCode = S1ap_ProcedureCode_id_initialUEMessage;

  initial_ue_message_p = &message.msg.s1ap_InitialUEMessageIEs;

  /* Select the MME corresponding to the provided GUMMEI. */
  if (s1ap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_gummei) {
    mme_desc_p = s1ap_eNB_nnsf_select_mme_by_gummei(
                   instance_p,
                   s1ap_nas_first_req_p->establishment_cause,
                   s1ap_nas_first_req_p->ue_identity.gummei);
  }

  if (mme_desc_p == NULL) {
    /* Select the MME corresponding to the provided s-TMSI. */
    if (s1ap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_s_tmsi) {
      mme_desc_p = s1ap_eNB_nnsf_select_mme_by_mme_code(
                     instance_p,
                     s1ap_nas_first_req_p->establishment_cause,
                     s1ap_nas_first_req_p->ue_identity.s_tmsi.mme_code);
    }
  }

  if (mme_desc_p == NULL) {
    /*
     * If no MME corresponds to the GUMMEI or the s-TMSI, selects the MME with the
     * highest capacity.
     */
    mme_desc_p = s1ap_eNB_nnsf_select_mme(
                   instance_p,
                   s1ap_nas_first_req_p->establishment_cause);
  }

  if (mme_desc_p == NULL) {
    /*
     * In case eNB has no MME associated, the eNB should inform RRC and discard
     * this request.
     */

    S1AP_WARN("No MME is associated to the eNB\n");
    // TODO: Inform RRC
    return -1;
  }

  /* The eNB should allocate a unique eNB UE S1AP ID for this UE. The value
   * will be used for the duration of the connectivity.
   */
  ue_desc_p = s1ap_eNB_allocate_new_UE_context();
  DevAssert(ue_desc_p != NULL);

  /* Keep a reference to the selected MME */
  ue_desc_p->mme_ref       = mme_desc_p;
  ue_desc_p->ue_initial_id = s1ap_nas_first_req_p->ue_initial_id;
  ue_desc_p->eNB_instance  = instance_p;

  do {
    struct s1ap_eNB_ue_context_s *collision_p;

    /* Peek a random value for the eNB_ue_s1ap_id */
    ue_desc_p->eNB_ue_s1ap_id = (random() + random()) & 0x00ffffff;

    if ((collision_p = RB_INSERT(s1ap_ue_map, &instance_p->s1ap_ue_head, ue_desc_p))
        == NULL) {
      S1AP_DEBUG("Found usable eNB_ue_s1ap_id: 0x%06x %d(10)\n",
                 ue_desc_p->eNB_ue_s1ap_id,
                 ue_desc_p->eNB_ue_s1ap_id);
      /* Break the loop as the id is not already used by another UE */
      break;
    }
  } while(1);

  initial_ue_message_p->eNB_UE_S1AP_ID = ue_desc_p->eNB_ue_s1ap_id;
  /* Prepare the NAS PDU */
  initial_ue_message_p->nas_pdu.buf  = s1ap_nas_first_req_p->nas_pdu.buffer;
  initial_ue_message_p->nas_pdu.size = s1ap_nas_first_req_p->nas_pdu.length;

  /* Set the establishment cause according to those provided by RRC */
  DevCheck(s1ap_nas_first_req_p->establishment_cause < RRC_CAUSE_LAST,
           s1ap_nas_first_req_p->establishment_cause, RRC_CAUSE_LAST, 0);
  initial_ue_message_p->rrC_Establishment_Cause = s1ap_nas_first_req_p->establishment_cause;

  if (s1ap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_s_tmsi) {
    S1AP_DEBUG("S_TMSI_PRESENT\n");
    initial_ue_message_p->presenceMask |= S1AP_INITIALUEMESSAGEIES_S_TMSI_PRESENT;

    MME_CODE_TO_OCTET_STRING(s1ap_nas_first_req_p->ue_identity.s_tmsi.mme_code,
                             &initial_ue_message_p->s_tmsi.mMEC);
    M_TMSI_TO_OCTET_STRING(s1ap_nas_first_req_p->ue_identity.s_tmsi.m_tmsi,
                           &initial_ue_message_p->s_tmsi.m_TMSI);
  }

  if (s1ap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_gummei) {
    S1AP_DEBUG("GUMMEI_ID_PRESENT\n");
    initial_ue_message_p->presenceMask |= S1AP_INITIALUEMESSAGEIES_GUMMEI_ID_PRESENT;

    MCC_MNC_TO_PLMNID(
      s1ap_nas_first_req_p->ue_identity.gummei.mcc,
      s1ap_nas_first_req_p->ue_identity.gummei.mnc,
      s1ap_nas_first_req_p->ue_identity.gummei.mnc_len,
      &initial_ue_message_p->gummei_id.pLMN_Identity);

    MME_GID_TO_OCTET_STRING(s1ap_nas_first_req_p->ue_identity.gummei.mme_group_id,
                            &initial_ue_message_p->gummei_id.mME_Group_ID);
    MME_CODE_TO_OCTET_STRING(s1ap_nas_first_req_p->ue_identity.gummei.mme_code,
                             &initial_ue_message_p->gummei_id.mME_Code);
  }

  /* Assuming TAI is the TAI from the cell */
  INT16_TO_OCTET_STRING(instance_p->tac, &initial_ue_message_p->tai.tAC);
  MCC_MNC_TO_PLMNID(instance_p->mcc,
                    instance_p->mnc,
                    instance_p->mnc_digit_length,
                    &initial_ue_message_p->tai.pLMNidentity);

  /* Set the EUTRAN CGI
   * The cell identity is defined on 28 bits but as we use macro enb id,
   * we have to pad.
   */
//#warning "TODO get cell id from RRC"
  MACRO_ENB_ID_TO_CELL_IDENTITY(instance_p->eNB_id,
		  0, // Cell ID
          &initial_ue_message_p->eutran_cgi.cell_ID);
  MCC_MNC_TO_TBCD(instance_p->mcc,
                  instance_p->mnc,
                  instance_p->mnc_digit_length,
                  &initial_ue_message_p->eutran_cgi.pLMNidentity);

  if (s1ap_eNB_encode_pdu(&message, &buffer, &length) < 0) {
    /* Failed to encode message */
    DevMessage("Failed to encode initial UE message\n");
  }

  /* Update the current S1AP UE state */
  ue_desc_p->ue_state = S1AP_UE_WAITING_CSR;

  /* Assign a stream for this UE :
   * From 3GPP 36.412 7)Transport layers:
   *  Within the SCTP association established between one MME and eNB pair:
   *  - a single pair of stream identifiers shall be reserved for the sole use
   *      of S1AP elementary procedures that utilize non UE-associated signalling.
   *  - At least one pair of stream identifiers shall be reserved for the sole use
   *      of S1AP elementary procedures that utilize UE-associated signallings.
   *      However a few pairs (i.e. more than one) should be reserved.
   *  - A single UE-associated signalling shall use one SCTP stream and
   *      the stream should not be changed during the communication of the
   *      UE-associated signalling.
   */
  mme_desc_p->nextstream = (mme_desc_p->nextstream + 1) % mme_desc_p->out_streams;

  if ((mme_desc_p->nextstream == 0) && (mme_desc_p->out_streams > 1)) {
    mme_desc_p->nextstream += 1;
  }

  ue_desc_p->tx_stream = mme_desc_p->nextstream;

  MSC_LOG_TX_MESSAGE(
    MSC_S1AP_ENB,
    MSC_S1AP_MME,
    (const char *)NULL,
    0,
    MSC_AS_TIME_FMT" initialUEMessage initiatingMessage eNB_ue_s1ap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    initial_ue_message_p->eNB_UE_S1AP_ID);

  /* Send encoded message over sctp */
  s1ap_eNB_itti_send_sctp_data_req(instance_p->instance, mme_desc_p->assoc_id,
                                   buffer, length, ue_desc_p->tx_stream);

  return 0;
}

//------------------------------------------------------------------------------
int s1ap_eNB_handle_nas_downlink(const uint32_t               assoc_id,
                                 const uint32_t               stream,
                                 struct s1ap_message_s* message_p)
//------------------------------------------------------------------------------
{
  const S1ap_DownlinkNASTransportIEs_t *downlink_NAS_transport_p = NULL;

  s1ap_eNB_mme_data_t   *mme_desc_p                        = NULL;
  s1ap_eNB_ue_context_t *ue_desc_p                         = NULL;
  s1ap_eNB_instance_t   *s1ap_eNB_instance                 = NULL;

  DevAssert(message_p != NULL);

  downlink_NAS_transport_p = &message_p->msg.s1ap_DownlinkNASTransportIEs;

  /* UE-related procedure -> stream != 0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received UE-related procedure on stream == 0\n",
               assoc_id);
    return -1;
  }

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR(
      "[SCTP %d] Received NAS downlink message for non existing MME context\n",
      assoc_id);
    return -1;
  }

  s1ap_eNB_instance = mme_desc_p->s1ap_eNB_instance;

  if ((ue_desc_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance,
                   downlink_NAS_transport_p->eNB_UE_S1AP_ID)) == NULL) {
    MSC_LOG_RX_DISCARDED_MESSAGE(
      MSC_S1AP_ENB,
      MSC_S1AP_MME,
      (const char *)downlink_NAS_transport_p,
      sizeof(S1ap_DownlinkNASTransportIEs_t),
      MSC_AS_TIME_FMT" downlinkNASTransport  eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
      0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
      downlink_NAS_transport_p->eNB_UE_S1AP_ID,
      downlink_NAS_transport_p->mme_ue_s1ap_id);

    /* TODO: fix this log - the original version is suspicious (twice downlink_NAS_transport_p->eNB_UE_S1AP_ID?) */
    /*S1AP_ERROR("[SCTP %d] Received NAS downlink message for non existing UE context eNB_UE_S1AP_ID: 0x%"PRIx32" %u\n",
               assoc_id,
               downlink_NAS_transport_p->eNB_UE_S1AP_ID,
               downlink_NAS_transport_p->eNB_UE_S1AP_ID);*/
    S1AP_ERROR("[SCTP %d] Received NAS downlink message for non existing UE context eNB_UE_S1AP_ID: 0x%lx\n",
               assoc_id,
               downlink_NAS_transport_p->eNB_UE_S1AP_ID);
    return -1;
  }

  if (0 == ue_desc_p->rx_stream) {
	ue_desc_p->rx_stream = stream;
  } else if (stream != ue_desc_p->rx_stream) {
    S1AP_ERROR("[SCTP %d] Received UE-related procedure on stream %u, expecting %u\n",
               assoc_id, stream, ue_desc_p->rx_stream);
    return -1;
  }

  /* Is it the first outcome of the MME for this UE ? If so store the mme
   * UE s1ap id.
   */
  if (ue_desc_p->mme_ue_s1ap_id == 0) {
    ue_desc_p->mme_ue_s1ap_id = downlink_NAS_transport_p->mme_ue_s1ap_id;
  } else {
    /* We already have a mme ue s1ap id check the received is the same */
    if (ue_desc_p->mme_ue_s1ap_id != downlink_NAS_transport_p->mme_ue_s1ap_id) {
      S1AP_ERROR("[SCTP %d] Mismatch in MME UE S1AP ID (0x%lx != 0x%"PRIx32"\n",
                 assoc_id,
                 downlink_NAS_transport_p->mme_ue_s1ap_id,
                 ue_desc_p->mme_ue_s1ap_id
                );
      return -1;
    }
  }

  MSC_LOG_RX_MESSAGE(
    MSC_S1AP_ENB,
    MSC_S1AP_MME,
    (const char *)downlink_NAS_transport_p,
    sizeof(S1ap_DownlinkNASTransportIEs_t),
    MSC_AS_TIME_FMT" downlinkNASTransport  eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    downlink_NAS_transport_p->eNB_UE_S1AP_ID,
    downlink_NAS_transport_p->mme_ue_s1ap_id);

  /* Forward the NAS PDU to RRC */
  s1ap_eNB_itti_send_nas_downlink_ind(s1ap_eNB_instance->instance,
                                      ue_desc_p->ue_initial_id,
                                      ue_desc_p->eNB_ue_s1ap_id,
                                      downlink_NAS_transport_p->nas_pdu.buf,
                                      downlink_NAS_transport_p->nas_pdu.size);

  // LG: Why set to 0 ??
  //ue_desc_p->ue_initial_id = 0;

  return 0;
}

//------------------------------------------------------------------------------
int s1ap_eNB_nas_uplink(instance_t instance, s1ap_uplink_nas_t *s1ap_uplink_nas_p)
//------------------------------------------------------------------------------
{
  struct s1ap_eNB_ue_context_s *ue_context_p;
  s1ap_eNB_instance_t          *s1ap_eNB_instance_p;
  S1ap_UplinkNASTransportIEs_t *uplink_NAS_transport_p;

  s1ap_message  message;

  uint8_t  *buffer;
  uint32_t length;

  DevAssert(s1ap_uplink_nas_p != NULL);

  /* Retrieve the S1AP eNB instance associated with Mod_id */
  s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);
  DevAssert(s1ap_eNB_instance_p != NULL);

  if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p, s1ap_uplink_nas_p->eNB_ue_s1ap_id)) == NULL) {
    /* The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs */
    S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: %06x\n",
              s1ap_uplink_nas_p->eNB_ue_s1ap_id);
    return -1;
  }

  /* Uplink NAS transport can occur either during an s1ap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == S1AP_UE_CONNECTED ||
        ue_context_p->ue_state == S1AP_UE_WAITING_CSR)) {
    S1AP_WARN("You are attempting to send NAS data over non-connected "
              "eNB ue s1ap id: %u, current state: %d\n",
              s1ap_uplink_nas_p->eNB_ue_s1ap_id, ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the S1AP message to encode */
  memset(&message, 0, sizeof(s1ap_message));

  message.direction     = S1AP_PDU_PR_initiatingMessage;
  message.procedureCode = S1ap_ProcedureCode_id_uplinkNASTransport;

  uplink_NAS_transport_p = &message.msg.s1ap_UplinkNASTransportIEs;

  uplink_NAS_transport_p->mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;
  uplink_NAS_transport_p->eNB_UE_S1AP_ID = ue_context_p->eNB_ue_s1ap_id;

  uplink_NAS_transport_p->nas_pdu.buf  = s1ap_uplink_nas_p->nas_pdu.buffer;
  uplink_NAS_transport_p->nas_pdu.size = s1ap_uplink_nas_p->nas_pdu.length;

  MCC_MNC_TO_PLMNID(
    s1ap_eNB_instance_p->mcc,
    s1ap_eNB_instance_p->mnc,
    s1ap_eNB_instance_p->mnc_digit_length,
    &uplink_NAS_transport_p->eutran_cgi.pLMNidentity);

//#warning "TODO get cell id from RRC"
  MACRO_ENB_ID_TO_CELL_IDENTITY(s1ap_eNB_instance_p->eNB_id,
          0,
          &uplink_NAS_transport_p->eutran_cgi.cell_ID);

  /* MCC/MNC should be repeated in TAI and EUTRAN CGI */
  MCC_MNC_TO_PLMNID(
    s1ap_eNB_instance_p->mcc,
    s1ap_eNB_instance_p->mnc,
    s1ap_eNB_instance_p->mnc_digit_length,
    &uplink_NAS_transport_p->tai.pLMNidentity);

  TAC_TO_ASN1(s1ap_eNB_instance_p->tac, &uplink_NAS_transport_p->tai.tAC);

  if (s1ap_eNB_encode_pdu(&message, &buffer, &length) < 0) {
    S1AP_ERROR("Failed to encode uplink NAS transport\n");
    /* Encode procedure has failed... */
    return -1;
  }

  MSC_LOG_TX_MESSAGE(
    MSC_S1AP_ENB,
    MSC_S1AP_MME,
    (const char *)NULL,
    0,
    MSC_AS_TIME_FMT" uplinkNASTransport initiatingMessage eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    uplink_NAS_transport_p->eNB_UE_S1AP_ID,
    uplink_NAS_transport_p->mme_ue_s1ap_id);


  /* UE associated signalling -> use the allocated stream */
  s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                   ue_context_p->mme_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return 0;
}


//------------------------------------------------------------------------------
void s1ap_eNB_nas_non_delivery_ind(instance_t instance,
                                   s1ap_nas_non_delivery_ind_t *s1ap_nas_non_delivery_ind)
//------------------------------------------------------------------------------
{
  struct s1ap_eNB_ue_context_s *ue_context_p;
  s1ap_eNB_instance_t          *s1ap_eNB_instance_p;

  S1ap_NASNonDeliveryIndication_IEs_t *nas_non_delivery_p;

  s1ap_message  message;

  uint8_t  *buffer;
  uint32_t length;

  DevAssert(s1ap_nas_non_delivery_ind != NULL);

  /* Retrieve the S1AP eNB instance associated with Mod_id */
  s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);
  DevAssert(s1ap_eNB_instance_p != NULL);

  if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p, s1ap_nas_non_delivery_ind->eNB_ue_s1ap_id)) == NULL) {
    /* The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs */
    S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: %06x\n",
              s1ap_nas_non_delivery_ind->eNB_ue_s1ap_id);
    MSC_LOG_EVENT(
      MSC_S1AP_ENB,
      MSC_AS_TIME_FMT" Sent of NAS_NON_DELIVERY_IND to MME failed, no context for eNB_ue_s1ap_id %06x",
      s1ap_nas_non_delivery_ind->eNB_ue_s1ap_id);
    return;
  }

  /* Prepare the S1AP message to encode */
  memset(&message, 0, sizeof(s1ap_message));

  message.direction     = S1AP_PDU_PR_initiatingMessage;
  message.procedureCode = S1ap_ProcedureCode_id_NASNonDeliveryIndication;

  nas_non_delivery_p = &message.msg.s1ap_NASNonDeliveryIndication_IEs;
  nas_non_delivery_p->eNB_UE_S1AP_ID = ue_context_p->eNB_ue_s1ap_id;
  nas_non_delivery_p->mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;
  nas_non_delivery_p->nas_pdu.buf    = s1ap_nas_non_delivery_ind->nas_pdu.buffer;
  nas_non_delivery_p->nas_pdu.size   = s1ap_nas_non_delivery_ind->nas_pdu.length;

  /* Send a dummy cause */
  nas_non_delivery_p->cause.present = S1ap_Cause_PR_radioNetwork;
  nas_non_delivery_p->cause.choice.radioNetwork = S1ap_CauseRadioNetwork_radio_connection_with_ue_lost;

  if (s1ap_eNB_encode_pdu(&message, &buffer, &length) < 0) {
    S1AP_ERROR("Failed to encode NAS NON delivery indication\n");
    /* Encode procedure has failed... */
    MSC_LOG_EVENT(
      MSC_S1AP_ENB,
      MSC_AS_TIME_FMT" Sent of NAS_NON_DELIVERY_IND to MME failed (encoding)");
    return;
  }


  MSC_LOG_TX_MESSAGE(
    MSC_S1AP_ENB,
    MSC_S1AP_MME,
    (const char *)s1ap_nas_non_delivery_ind,
    sizeof(s1ap_nas_non_delivery_ind_t),
    MSC_AS_TIME_FMT" NASNonDeliveryIndication initiatingMessage eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    nas_non_delivery_p->eNB_UE_S1AP_ID,
    nas_non_delivery_p->mme_ue_s1ap_id);


  /* UE associated signalling -> use the allocated stream */
  s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                   ue_context_p->mme_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);
}

//------------------------------------------------------------------------------
int s1ap_eNB_initial_ctxt_resp(
  instance_t instance, s1ap_initial_context_setup_resp_t *initial_ctxt_resp_p)
//------------------------------------------------------------------------------
{
  s1ap_eNB_instance_t          *s1ap_eNB_instance_p = NULL;
  struct s1ap_eNB_ue_context_s *ue_context_p        = NULL;

  S1ap_InitialContextSetupResponseIEs_t *initial_ies_p  = NULL;

  s1ap_message  message;

  uint8_t  *buffer  = NULL;
  uint32_t length;
  int      ret = -1;
  int      i;

  /* Retrieve the S1AP eNB instance associated with Mod_id */
  s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);

  DevAssert(initial_ctxt_resp_p != NULL);
  DevAssert(s1ap_eNB_instance_p != NULL);

  if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p,
                      initial_ctxt_resp_p->eNB_ue_s1ap_id)) == NULL) {
    /* The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs */
    S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: 0x%06x\n",
              initial_ctxt_resp_p->eNB_ue_s1ap_id);
    return -1;
  }

  /* Uplink NAS transport can occur either during an s1ap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == S1AP_UE_CONNECTED ||
        ue_context_p->ue_state == S1AP_UE_WAITING_CSR)) {
    S1AP_WARN("You are attempting to send NAS data over non-connected "
              "eNB ue s1ap id: %06x, current state: %d\n",
              initial_ctxt_resp_p->eNB_ue_s1ap_id, ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the S1AP message to encode */
  memset(&message, 0, sizeof(s1ap_message));

  message.direction     = S1AP_PDU_PR_successfulOutcome;
  message.procedureCode = S1ap_ProcedureCode_id_InitialContextSetup;

  initial_ies_p = &message.msg.s1ap_InitialContextSetupResponseIEs;

  initial_ies_p->eNB_UE_S1AP_ID = initial_ctxt_resp_p->eNB_ue_s1ap_id;
  initial_ies_p->mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;

  for (i = 0; i < initial_ctxt_resp_p->nb_of_e_rabs; i++) {
    S1ap_E_RABSetupItemCtxtSURes_t *new_item;

    new_item = calloc(1, sizeof(S1ap_E_RABSetupItemCtxtSURes_t));

    new_item->e_RAB_ID = initial_ctxt_resp_p->e_rabs[i].e_rab_id;
    GTP_TEID_TO_ASN1(initial_ctxt_resp_p->e_rabs[i].gtp_teid, &new_item->gTP_TEID);
    new_item->transportLayerAddress.buf = initial_ctxt_resp_p->e_rabs[i].eNB_addr.buffer;
    new_item->transportLayerAddress.size = initial_ctxt_resp_p->e_rabs[i].eNB_addr.length;
    new_item->transportLayerAddress.bits_unused = 0;

    S1AP_DEBUG("initial_ctxt_resp_p: e_rab ID %ld, enb_addr %d.%d.%d.%d, SIZE %d \n", 
	      new_item->e_RAB_ID,
	      new_item->transportLayerAddress.buf[0],
	      new_item->transportLayerAddress.buf[1],
	      new_item->transportLayerAddress.buf[2],
	      new_item->transportLayerAddress.buf[3],
	      new_item->transportLayerAddress.size);

    ASN_SEQUENCE_ADD(&initial_ies_p->e_RABSetupListCtxtSURes.s1ap_E_RABSetupItemCtxtSURes,
                     new_item);
  }

  if (s1ap_eNB_encode_pdu(&message, &buffer, &length) < 0) {
    S1AP_ERROR("Failed to encode uplink NAS transport\n");
    /* Encode procedure has failed... */
    return -1;
  }

  MSC_LOG_TX_MESSAGE(
    MSC_S1AP_ENB,
    MSC_S1AP_MME,
    (const char *)buffer,
    length,
    MSC_AS_TIME_FMT" InitialContextSetup successfulOutcome eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    initial_ies_p->eNB_UE_S1AP_ID,
    initial_ies_p->mme_ue_s1ap_id);

  /* UE associated signalling -> use the allocated stream */
  s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                   ue_context_p->mme_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return ret;
}

//------------------------------------------------------------------------------
int s1ap_eNB_ue_capabilities(instance_t instance,
                             s1ap_ue_cap_info_ind_t *ue_cap_info_ind_p)
//------------------------------------------------------------------------------
{
  s1ap_eNB_instance_t          *s1ap_eNB_instance_p;
  struct s1ap_eNB_ue_context_s *ue_context_p;

  S1ap_UECapabilityInfoIndicationIEs_t *ue_cap_info_ind_ies_p;

  s1ap_message  message;

  uint8_t  *buffer;
  uint32_t length;
  int      ret = -1;

  /* Retrieve the S1AP eNB instance associated with Mod_id */
  s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);

  DevAssert(ue_cap_info_ind_p != NULL);
  DevAssert(s1ap_eNB_instance_p != NULL);

  if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p,
                      ue_cap_info_ind_p->eNB_ue_s1ap_id)) == NULL) {
    /* The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs */
    S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: %u\n",
              ue_cap_info_ind_p->eNB_ue_s1ap_id);
    return -1;
  }

  /* UE capabilities message can occur either during an s1ap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == S1AP_UE_CONNECTED ||
        ue_context_p->ue_state == S1AP_UE_WAITING_CSR)) {
    S1AP_WARN("You are attempting to send NAS data over non-connected "
              "eNB ue s1ap id: %u, current state: %d\n",
              ue_cap_info_ind_p->eNB_ue_s1ap_id, ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the S1AP message to encode */
  memset(&message, 0, sizeof(s1ap_message));

  message.direction     = S1AP_PDU_PR_initiatingMessage;
  message.procedureCode = S1ap_ProcedureCode_id_UECapabilityInfoIndication;

  ue_cap_info_ind_ies_p = &message.msg.s1ap_UECapabilityInfoIndicationIEs;

  ue_cap_info_ind_ies_p->ueRadioCapability.buf = ue_cap_info_ind_p->ue_radio_cap.buffer;
  ue_cap_info_ind_ies_p->ueRadioCapability.size = ue_cap_info_ind_p->ue_radio_cap.length;

  ue_cap_info_ind_ies_p->eNB_UE_S1AP_ID = ue_cap_info_ind_p->eNB_ue_s1ap_id;
  ue_cap_info_ind_ies_p->mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;

  if (s1ap_eNB_encode_pdu(&message, &buffer, &length) < 0) {
    /* Encode procedure has failed... */
    S1AP_ERROR("Failed to encode UE capabilities indication\n");
    return -1;
  }

  free(ue_cap_info_ind_p->ue_radio_cap.buffer);

  MSC_LOG_TX_MESSAGE(
    MSC_S1AP_ENB,
    MSC_S1AP_MME,
    (const char *)buffer,
    length,
    MSC_AS_TIME_FMT" UECapabilityInfoIndication initiatingMessage eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    ue_cap_info_ind_ies_p->eNB_UE_S1AP_ID,
    ue_cap_info_ind_ies_p->mme_ue_s1ap_id);

  /* UE associated signalling -> use the allocated stream */
  s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                   ue_context_p->mme_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return ret;
}

//------------------------------------------------------------------------------
int s1ap_eNB_e_rab_setup_resp(instance_t instance, 
			      s1ap_e_rab_setup_resp_t *e_rab_setup_resp_p)
//------------------------------------------------------------------------------
{
  s1ap_eNB_instance_t          *s1ap_eNB_instance_p = NULL;
  struct s1ap_eNB_ue_context_s *ue_context_p        = NULL;

  S1ap_E_RABSetupResponseIEs_t  *initial_ies_p  = NULL;
 
  s1ap_message  message;

  uint8_t  *buffer  = NULL;
  uint32_t length;
  int      ret = -1;
  int      i;

  /* Retrieve the S1AP eNB instance associated with Mod_id */
  s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);

  DevAssert(e_rab_setup_resp_p != NULL);
  DevAssert(s1ap_eNB_instance_p != NULL);

  if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p,
					      e_rab_setup_resp_p->eNB_ue_s1ap_id)) == NULL) {
    /* The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs */
    S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: 0x%06x\n",
              e_rab_setup_resp_p->eNB_ue_s1ap_id);
    return -1;
  }

  /* Uplink NAS transport can occur either during an s1ap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == S1AP_UE_CONNECTED ||
        ue_context_p->ue_state == S1AP_UE_WAITING_CSR)) {
    S1AP_WARN("You are attempting to send NAS data over non-connected "
              "eNB ue s1ap id: %06x, current state: %d\n",
              e_rab_setup_resp_p->eNB_ue_s1ap_id, ue_context_p->ue_state);
    return -1;
  }
 
  /* Prepare the S1AP message to encode */
  memset(&message, 0, sizeof(s1ap_message));

  message.direction     = S1AP_PDU_PR_successfulOutcome;
  message.procedureCode = S1ap_ProcedureCode_id_E_RABSetup;
  message.criticality   = S1ap_Criticality_reject;

  initial_ies_p = &message.msg.s1ap_E_RABSetupResponseIEs;
  
  initial_ies_p->eNB_UE_S1AP_ID = e_rab_setup_resp_p->eNB_ue_s1ap_id;
  initial_ies_p->mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;
  
  if ( e_rab_setup_resp_p->nb_of_e_rabs >= 1 )
    initial_ies_p->presenceMask |= S1AP_E_RABSETUPRESPONSEIES_E_RABSETUPLISTBEARERSURES_PRESENT; 
  
  for (i = 0; i < e_rab_setup_resp_p->nb_of_e_rabs; i++) { 
    S1ap_E_RABSetupItemBearerSURes_t *new_item;

    new_item = calloc(1, sizeof(S1ap_E_RABSetupItemBearerSURes_t));

    new_item->e_RAB_ID = e_rab_setup_resp_p->e_rabs[i].e_rab_id;
    GTP_TEID_TO_ASN1(e_rab_setup_resp_p->e_rabs[i].gtp_teid, &new_item->gTP_TEID);
        
    /*
    new_item->transportLayerAddress.buf = MALLOC(e_rab_setup_resp_p->e_rabs[i].eNB_addr.length);  
    memcpy (new_item->transportLayerAddress.buf, 
	    e_rab_setup_resp_p->e_rabs[i].eNB_addr.buffer,
	    e_rab_setup_resp_p->e_rabs[i].eNB_addr.length);
    
    */
    /*
      new_item->transportLayerAddress.buf[0] = e_rab_setup_resp_p->e_rabs[i].eNB_addr.buffer[0];
    new_item->transportLayerAddress.buf[1] = e_rab_setup_resp_p->e_rabs[i].eNB_addr.buffer[1];
    new_item->transportLayerAddress.buf[2] = e_rab_setup_resp_p->e_rabs[i].eNB_addr.buffer[2];
    new_item->transportLayerAddress.buf[3] = e_rab_setup_resp_p->e_rabs[i].eNB_addr.buffer[3];
    */
    new_item->transportLayerAddress.buf = e_rab_setup_resp_p->e_rabs[i].eNB_addr.buffer; 
    new_item->transportLayerAddress.size = e_rab_setup_resp_p->e_rabs[i].eNB_addr.length;
    new_item->transportLayerAddress.bits_unused = 0;
    
    S1AP_DEBUG("e_rab_setup_resp: e_rab ID %ld, teid %u, enb_addr %d.%d.%d.%d, SIZE %d\n", 
	       new_item->e_RAB_ID,
	       e_rab_setup_resp_p->e_rabs[i].gtp_teid,
	       new_item->transportLayerAddress.buf[0],
	       new_item->transportLayerAddress.buf[1],
	       new_item->transportLayerAddress.buf[2],
	       new_item->transportLayerAddress.buf[3],
	       new_item->transportLayerAddress.size);
    
    S1ap_IE_t *ie = s1ap_new_ie(S1ap_ProtocolIE_ID_id_E_RABSetupItemBearerSURes,
				S1ap_Criticality_ignore,
				&asn_DEF_S1ap_E_RABSetupItemBearerSURes,
				new_item);
    /*
    S1ap_IE_t *ie = s1ap_new_ie(S1ap_ProtocolIE_ID_id_E_RABSetupListBearerSURes,
				S1ap_Criticality_ignore,
				&asn_DEF_S1ap_E_RABSetupListBearerSURes,
				new_item);
    */
    ASN_SEQUENCE_ADD(&initial_ies_p->e_RABSetupListBearerSURes.s1ap_E_RABSetupItemBearerSURes,
                     ie);
  }
 
  /* S1ap_E_RABSetupListBearerSURes_t  e_RABSetupListBearerSURes;
  memset(&e_RABSetupListBearerSURes, 0, sizeof(S1ap_E_RABSetupListBearerSURes_t));
  if (s1ap_encode_s1ap_e_rabsetuplistbearersures(&e_RABSetupListBearerSURes, &initial_ies_p->e_RABSetupListBearerSURes.s1ap_E_RABSetupItemBearerSURes) < 0 )
    return -1;
 ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1ap_E_RABSetupListBearerSURes, &e_RABSetupListBearerSURes);
  */
  fprintf(stderr, "start encode\n");
  if (s1ap_eNB_encode_pdu(&message, &buffer, &length) < 0) {
    S1AP_ERROR("Failed to encode uplink transport\n");
    /* Encode procedure has failed... */
    return -1;
  }

 
  
  MSC_LOG_TX_MESSAGE(
    MSC_S1AP_ENB,
    MSC_S1AP_MME,
    (const char *)buffer,
    length,
    MSC_AS_TIME_FMT" E_RAN Setup successfulOutcome eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    initial_ies_p->eNB_UE_S1AP_ID,
    initial_ies_p->mme_ue_s1ap_id);

  /* UE associated signalling -> use the allocated stream */
  s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                   ue_context_p->mme_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return ret;
}

//------------------------------------------------------------------------------
int s1ap_eNB_e_rab_modify_resp(instance_t instance,
            s1ap_e_rab_modify_resp_t *e_rab_modify_resp_p)
//------------------------------------------------------------------------------
{
  s1ap_eNB_instance_t          *s1ap_eNB_instance_p = NULL;
  struct s1ap_eNB_ue_context_s *ue_context_p        = NULL;

  S1ap_E_RABModifyResponseIEs_t  *initial_ies_p  = NULL;

  s1ap_message  message;

  uint8_t  *buffer  = NULL;
  uint32_t length;
  int      ret = -1;
  int      i;

  /* Retrieve the S1AP eNB instance associated with Mod_id */
  s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);

  DevAssert(e_rab_modify_resp_p != NULL);
  DevAssert(s1ap_eNB_instance_p != NULL);

  if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p,
                e_rab_modify_resp_p->eNB_ue_s1ap_id)) == NULL) {
    /* The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs */
    S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: 0x%06x\n",
              e_rab_modify_resp_p->eNB_ue_s1ap_id);
    return -1;
  }

  /* Uplink NAS transport can occur either during an s1ap connected state
   * or during initial attach (for example: NAS authentication).
   */
  if (!(ue_context_p->ue_state == S1AP_UE_CONNECTED ||
        ue_context_p->ue_state == S1AP_UE_WAITING_CSR)) {
    S1AP_WARN("You are attempting to send NAS data over non-connected "
              "eNB ue s1ap id: %06x, current state: %d\n",
              e_rab_modify_resp_p->eNB_ue_s1ap_id, ue_context_p->ue_state);
    return -1;
  }

  /* Prepare the S1AP message to encode */
  memset(&message, 0, sizeof(s1ap_message));

  message.direction     = S1AP_PDU_PR_successfulOutcome;
  message.procedureCode = S1ap_ProcedureCode_id_E_RABModify;
  message.criticality   = S1ap_Criticality_reject;

  initial_ies_p = &message.msg.s1ap_E_RABModifyResponseIEs;

  initial_ies_p->eNB_UE_S1AP_ID = e_rab_modify_resp_p->eNB_ue_s1ap_id;
  initial_ies_p->mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;

  if ( e_rab_modify_resp_p->nb_of_e_rabs >= 1 )
    initial_ies_p->presenceMask |= S1AP_E_RABMODIFYRESPONSEIES_E_RABMODIFYLISTBEARERMODRES_PRESENT;

  for (i = 0; i < e_rab_modify_resp_p->nb_of_e_rabs; i++) {
    S1ap_E_RABModifyItemBearerModRes_t *modify_item;

    modify_item = calloc(1, sizeof(S1ap_E_RABModifyItemBearerModRes_t));

    modify_item->e_RAB_ID = e_rab_modify_resp_p->e_rabs[i].e_rab_id;

    S1AP_DEBUG("e_rab_modify_resp: modified e_rab ID %ld\n",
        modify_item->e_RAB_ID);

    S1ap_IE_t *ie = s1ap_new_ie(S1ap_ProtocolIE_ID_id_E_RABModifyItemBearerModRes,
        S1ap_Criticality_ignore,
        &asn_DEF_S1ap_E_RABModifyItemBearerModRes,
        modify_item);

    ASN_SEQUENCE_ADD(&initial_ies_p->e_RABModifyListBearerModRes.s1ap_E_RABModifyItemBearerModRes,
                     ie);
  }

  if ( e_rab_modify_resp_p->nb_of_e_rabs_failed >= 1 )
    initial_ies_p->presenceMask |= S1AP_E_RABMODIFYRESPONSEIES_E_RABFAILEDTOMODIFYLIST_PRESENT;

  for (i = 0; i < e_rab_modify_resp_p->nb_of_e_rabs_failed; i++) {
    S1ap_E_RABItem_t *failed_item;

    failed_item = calloc(1, sizeof(S1ap_E_RABItem_t));

    failed_item->e_RAB_ID = e_rab_modify_resp_p->e_rabs_failed[i].e_rab_id;
    switch(e_rab_modify_resp_p->e_rabs_failed[i].cause)
    {
    case S1AP_CAUSE_RADIO_NETWORK:
        failed_item->cause.present = S1ap_Cause_PR_radioNetwork;
        failed_item->cause.choice.radioNetwork = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
        break;
    case S1AP_CAUSE_TRANSPORT:
        failed_item->cause.present = S1ap_Cause_PR_transport;
        failed_item->cause.choice.transport = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
        break;
    case S1AP_CAUSE_NAS:
        failed_item->cause.present = S1ap_Cause_PR_nas;
        failed_item->cause.choice.nas = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
        break;
    case S1AP_CAUSE_PROTOCOL:
        failed_item->cause.present = S1ap_Cause_PR_protocol;
        failed_item->cause.choice.protocol = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
        break;
    case S1AP_CAUSE_MISC:
        failed_item->cause.present = S1ap_Cause_PR_misc;
        failed_item->cause.choice.misc = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
        break;
    default:
        break;
    }
    S1AP_DEBUG("e_rab_modify_resp: failed e_rab ID %ld\n",
        failed_item->e_RAB_ID);

    S1ap_IE_t *ie = s1ap_new_ie(S1ap_ProtocolIE_ID_id_E_RABItem,
        S1ap_Criticality_ignore,
        &asn_DEF_S1ap_E_RABItem,
        failed_item);

    ASN_SEQUENCE_ADD(&initial_ies_p->e_RABFailedToModifyList.s1ap_E_RABItem,
                     ie);
  }

  fprintf(stderr, "start encode\n");
  if (s1ap_eNB_encode_pdu(&message, &buffer, &length) < 0) {
    S1AP_ERROR("Failed to encode uplink transport\n");
    /* Encode procedure has failed... */
    return -1;
  }

  MSC_LOG_TX_MESSAGE(
    MSC_S1AP_ENB,
    MSC_S1AP_MME,
    (const char *)buffer,
    length,
    MSC_AS_TIME_FMT" E_RAN Modify successful Outcome eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    initial_ies_p->eNB_UE_S1AP_ID,
    initial_ies_p->mme_ue_s1ap_id);

  /* UE associated signalling -> use the allocated stream */
  s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                   ue_context_p->mme_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return ret;
}
//------------------------------------------------------------------------------
int s1ap_eNB_e_rab_release_resp(instance_t instance,
			      s1ap_e_rab_release_resp_t *e_rab_release_resp_p)
//------------------------------------------------------------------------------
{
  s1ap_eNB_instance_t          *s1ap_eNB_instance_p = NULL;
  struct s1ap_eNB_ue_context_s *ue_context_p        = NULL;

  S1ap_E_RABReleaseResponseIEs_t  *release_response_ies_p  = NULL;

  s1ap_message  message;

  uint8_t  *buffer  = NULL;
  uint32_t length;
  int      ret = -1;
  int      i;
  /* Retrieve the S1AP eNB instance associated with Mod_id */
  s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);
  DevAssert(e_rab_release_resp_p != NULL);
  DevAssert(s1ap_eNB_instance_p != NULL);

  /* Prepare the S1AP message to encode */
  memset(&message, 0, sizeof(s1ap_message));

  message.direction     = S1AP_PDU_PR_successfulOutcome;
  message.procedureCode = S1ap_ProcedureCode_id_E_RABRelease;
  message.criticality = S1ap_Criticality_ignore;

  if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p,
          e_rab_release_resp_p->eNB_ue_s1ap_id)) == NULL) {
    /* The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs */
    S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: %u\n",
            e_rab_release_resp_p->eNB_ue_s1ap_id);
    return -1;
  }

  release_response_ies_p = &message.msg.s1ap_E_RABReleaseResponseIEs;
  release_response_ies_p->eNB_UE_S1AP_ID = e_rab_release_resp_p->eNB_ue_s1ap_id;
  release_response_ies_p->mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;

  if ( e_rab_release_resp_p->nb_of_e_rabs_released > 0 )
      release_response_ies_p->presenceMask |= S1AP_E_RABRELEASERESPONSEIES_E_RABRELEASELISTBEARERRELCOMP_PRESENT;

  //release
  for (i = 0; i < e_rab_release_resp_p->nb_of_e_rabs_released; i++) {

    S1ap_E_RABReleaseItemBearerRelComp_t *new_item;

    new_item = calloc(1, sizeof(S1ap_E_RABReleaseItemBearerRelComp_t));

    new_item->e_RAB_ID = e_rab_release_resp_p->e_rab_release[i].e_rab_id;

    S1AP_DEBUG("e_rab_release_resp: e_rab ID %ld\n",new_item->e_RAB_ID);

    ASN_SEQUENCE_ADD(&release_response_ies_p->e_RABReleaseListBearerRelComp.s1ap_E_RABReleaseItemBearerRelComp,
                     new_item);

  }

  if ( e_rab_release_resp_p->nb_of_e_rabs_failed > 0 )
      release_response_ies_p->presenceMask |= S1AP_E_RABRELEASERESPONSEIES_E_RABFAILEDTORELEASELIST_PRESENT;

  //release failed
  for (i = 0; i < e_rab_release_resp_p->nb_of_e_rabs_failed; i++) {
      S1ap_E_RABItem_t     *new_rabitem;
      new_rabitem = calloc(1, sizeof(S1ap_E_RABItem_t));
      //e_rab_id
      new_rabitem->e_RAB_ID = e_rab_release_resp_p->e_rabs_failed[i].e_rab_id;
      //cause
      switch(e_rab_release_resp_p->e_rabs_failed[i].cause)
      {
        case S1AP_CAUSE_NOTHING:
          new_rabitem->cause.present = S1ap_Cause_PR_NOTHING;
        break;

        case S1AP_CAUSE_RADIO_NETWORK:
          new_rabitem->cause.present = S1ap_Cause_PR_radioNetwork;
          new_rabitem->cause.choice.radioNetwork = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
        break;

        case S1AP_CAUSE_TRANSPORT:
          new_rabitem->cause.present = S1ap_Cause_PR_transport;
          new_rabitem->cause.choice.transport = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
        break;

        case S1AP_CAUSE_NAS:
          new_rabitem->cause.present = S1ap_Cause_PR_nas;
          new_rabitem->cause.choice.nas = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
        break;

        case S1AP_CAUSE_PROTOCOL:
          new_rabitem->cause.present = S1ap_Cause_PR_protocol;
          new_rabitem->cause.choice.protocol = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
        break;

        case S1AP_CAUSE_MISC:
        default:
          new_rabitem->cause.present = S1ap_Cause_PR_misc;
          new_rabitem->cause.choice.misc = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
        break;
      }
      S1AP_DEBUG("e_rab_release_resp: failed e_rab ID %ld\n",new_rabitem->e_RAB_ID);
      ASN_SEQUENCE_ADD(&release_response_ies_p->e_RABFailedToReleaseList.s1ap_E_RABItem, new_rabitem);
  }

  fprintf(stderr, "start encode\n");
  if (s1ap_eNB_encode_pdu(&message, &buffer, &length) < 0) {
    S1AP_ERROR("Failed to encode release response\n");
    /* Encode procedure has failed... */
    return -1;
  }

  MSC_LOG_TX_MESSAGE(
    MSC_S1AP_ENB,
    MSC_S1AP_MME,
    (const char *)buffer,
    length,
    MSC_AS_TIME_FMT" E_RAN Release successfulOutcome eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    e_rab_release_resp_p->eNB_ue_s1ap_id,
    ue_context_p->mme_ue_s1ap_id);

  /* UE associated signalling -> use the allocated stream */
  s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                   ue_context_p->mme_ref->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  S1AP_INFO("e_rab_release_response sended eNB_UE_S1AP_ID %d  mme_ue_s1ap_id %d nb_of_e_rabs_released %d nb_of_e_rabs_failed %d\n",
          e_rab_release_resp_p->eNB_ue_s1ap_id, ue_context_p->mme_ue_s1ap_id,e_rab_release_resp_p->nb_of_e_rabs_released,e_rab_release_resp_p->nb_of_e_rabs_failed);

  return ret;
}
