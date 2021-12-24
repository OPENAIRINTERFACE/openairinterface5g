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
    S1AP_S1AP_PDU_t               pdu;
    S1AP_InitialUEMessage_t      *out;
    S1AP_InitialUEMessage_IEs_t  *ie;
    uint8_t  *buffer = NULL;
    uint32_t  length = 0;
    DevAssert(s1ap_nas_first_req_p != NULL);
    /* Retrieve the S1AP eNB instance associated with Mod_id */
    instance_p = s1ap_eNB_get_instance(instance);
    DevAssert(instance_p != NULL);
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_initialUEMessage;
    pdu.choice.initiatingMessage.criticality = S1AP_Criticality_ignore;
    pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_InitialUEMessage;
    out = &pdu.choice.initiatingMessage.value.choice.InitialUEMessage;

    /* Select the MME corresponding to the provided GUMMEI. */
    if (s1ap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_gummei) {
        mme_desc_p = s1ap_eNB_nnsf_select_mme_by_gummei(
                         instance_p,
                         s1ap_nas_first_req_p->establishment_cause,
                         s1ap_nas_first_req_p->ue_identity.gummei);

        if (mme_desc_p) {
            S1AP_INFO("[eNB %ld] Chose MME '%s' (assoc_id %d) through GUMMEI MCC %d MNC %d MMEGI %d MMEC %d\n",
                      instance,
                      mme_desc_p->mme_name,
                      mme_desc_p->assoc_id,
                      s1ap_nas_first_req_p->ue_identity.gummei.mcc,
                      s1ap_nas_first_req_p->ue_identity.gummei.mnc,
                      s1ap_nas_first_req_p->ue_identity.gummei.mme_group_id,
                      s1ap_nas_first_req_p->ue_identity.gummei.mme_code);
        }
    }

    if (mme_desc_p == NULL) {
        /* Select the MME corresponding to the provided s-TMSI. */
        if (s1ap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_s_tmsi) {
            mme_desc_p = s1ap_eNB_nnsf_select_mme_by_mme_code(
                             instance_p,
                             s1ap_nas_first_req_p->establishment_cause,
                             s1ap_nas_first_req_p->selected_plmn_identity,
                             s1ap_nas_first_req_p->ue_identity.s_tmsi.mme_code);

            if (mme_desc_p) {
                S1AP_INFO("[eNB %ld] Chose MME '%s' (assoc_id %d) through S-TMSI MMEC %d and selected PLMN Identity index %d MCC %d MNC %d\n",
                          instance,
                          mme_desc_p->mme_name,
                          mme_desc_p->assoc_id,
                          s1ap_nas_first_req_p->ue_identity.s_tmsi.mme_code,
                          s1ap_nas_first_req_p->selected_plmn_identity,
                          instance_p->mcc[s1ap_nas_first_req_p->selected_plmn_identity],
                          instance_p->mnc[s1ap_nas_first_req_p->selected_plmn_identity]);
            }
        }
    }

    if (mme_desc_p == NULL) {
        /* Select MME based on the selected PLMN identity, received through RRC
         * Connection Setup Complete */
        mme_desc_p = s1ap_eNB_nnsf_select_mme_by_plmn_id(
                         instance_p,
                         s1ap_nas_first_req_p->establishment_cause,
                         s1ap_nas_first_req_p->selected_plmn_identity);

        if (mme_desc_p) {
            S1AP_INFO("[eNB %ld] Chose MME '%s' (assoc_id %d) through selected PLMN Identity index %d MCC %d MNC %d\n",
                      instance,
                      mme_desc_p->mme_name,
                      mme_desc_p->assoc_id,
                      s1ap_nas_first_req_p->selected_plmn_identity,
                      instance_p->mcc[s1ap_nas_first_req_p->selected_plmn_identity],
                      instance_p->mnc[s1ap_nas_first_req_p->selected_plmn_identity]);
        }
    }

    if (mme_desc_p == NULL) {
        /*
         * If no MME corresponds to the GUMMEI, the s-TMSI, or the selected PLMN
         * identity, selects the MME with the highest capacity.
         */
        mme_desc_p = s1ap_eNB_nnsf_select_mme(
                       instance_p,
                       s1ap_nas_first_req_p->establishment_cause);

        if (mme_desc_p) {
            S1AP_INFO("[eNB %ld] Chose MME '%s' (assoc_id %d) through highest relative capacity\n",
                      instance,
                      mme_desc_p->mme_name,
                      mme_desc_p->assoc_id);
        }
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
    ue_desc_p->selected_plmn_identity = s1ap_nas_first_req_p->selected_plmn_identity;

    do {
        struct s1ap_eNB_ue_context_s *collision_p;
        /* Peek a random value for the eNB_ue_s1ap_id */
        ue_desc_p->eNB_ue_s1ap_id = (random() + random()) & 0x00ffffff;

        if ((collision_p = RB_INSERT(s1ap_ue_map, &instance_p->s1ap_ue_head, ue_desc_p))
                == NULL) {
            S1AP_DEBUG("Found usable eNB_ue_s1ap_id: 0x%06x %u(10)\n",
                       ue_desc_p->eNB_ue_s1ap_id,
                       ue_desc_p->eNB_ue_s1ap_id);
            /* Break the loop as the id is not already used by another UE */
            break;
        }
    } while(1);

    /* mandatory */
    ie = (S1AP_InitialUEMessage_IEs_t *)calloc(1, sizeof(S1AP_InitialUEMessage_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_InitialUEMessage_IEs__value_PR_ENB_UE_S1AP_ID;
    ie->value.choice.ENB_UE_S1AP_ID = ue_desc_p->eNB_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_InitialUEMessage_IEs_t *)calloc(1, sizeof(S1AP_InitialUEMessage_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_NAS_PDU;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_InitialUEMessage_IEs__value_PR_NAS_PDU;
#if 1
    ie->value.choice.NAS_PDU.buf = s1ap_nas_first_req_p->nas_pdu.buffer;
#else
    ie->value.choice.NAS_PDU.buf = malloc(s1ap_nas_first_req_p->nas_pdu.length);
    memcpy(ie->value.choice.NAS_PDU.buf,
           s1ap_nas_first_req_p->nas_pdu.buffer,
           s1ap_nas_first_req_p->nas_pdu.length);
#endif
    ie->value.choice.NAS_PDU.size = s1ap_nas_first_req_p->nas_pdu.length;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_InitialUEMessage_IEs_t *)calloc(1, sizeof(S1AP_InitialUEMessage_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_TAI;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_InitialUEMessage_IEs__value_PR_TAI;
    /* Assuming TAI is the TAI from the cell */
    INT16_TO_OCTET_STRING(instance_p->tac, &ie->value.choice.TAI.tAC);
    MCC_MNC_TO_PLMNID(instance_p->mcc[ue_desc_p->selected_plmn_identity],
                      instance_p->mnc[ue_desc_p->selected_plmn_identity],
                      instance_p->mnc_digit_length[ue_desc_p->selected_plmn_identity],
                      &ie->value.choice.TAI.pLMNidentity);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_InitialUEMessage_IEs_t *)calloc(1, sizeof(S1AP_InitialUEMessage_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_EUTRAN_CGI;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_InitialUEMessage_IEs__value_PR_EUTRAN_CGI;
    /* Set the EUTRAN CGI
     * The cell identity is defined on 28 bits but as we use macro enb id,
     * we have to pad.
     */
    //#warning "TODO get cell id from RRC"
    MACRO_ENB_ID_TO_CELL_IDENTITY(instance_p->eNB_id,
                                  0, // Cell ID
                                  &ie->value.choice.EUTRAN_CGI.cell_ID);
    MCC_MNC_TO_TBCD(instance_p->mcc[ue_desc_p->selected_plmn_identity],
                    instance_p->mnc[ue_desc_p->selected_plmn_identity],
                    instance_p->mnc_digit_length[ue_desc_p->selected_plmn_identity],
                    &ie->value.choice.EUTRAN_CGI.pLMNidentity);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* Set the establishment cause according to those provided by RRC */
    DevCheck(s1ap_nas_first_req_p->establishment_cause < RRC_CAUSE_LAST,
             s1ap_nas_first_req_p->establishment_cause, RRC_CAUSE_LAST, 0);
    /* mandatory */
    ie = (S1AP_InitialUEMessage_IEs_t *)calloc(1, sizeof(S1AP_InitialUEMessage_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_RRC_Establishment_Cause;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_InitialUEMessage_IEs__value_PR_RRC_Establishment_Cause;
    ie->value.choice.RRC_Establishment_Cause = s1ap_nas_first_req_p->establishment_cause;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* optional */
    if (s1ap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_s_tmsi) {
        S1AP_DEBUG("S_TMSI_PRESENT\n");
        ie = (S1AP_InitialUEMessage_IEs_t *)calloc(1, sizeof(S1AP_InitialUEMessage_IEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_S_TMSI;
        ie->criticality = S1AP_Criticality_reject;
        ie->value.present = S1AP_InitialUEMessage_IEs__value_PR_S_TMSI;
        MME_CODE_TO_OCTET_STRING(s1ap_nas_first_req_p->ue_identity.s_tmsi.mme_code,
                                 &ie->value.choice.S_TMSI.mMEC);
        M_TMSI_TO_OCTET_STRING(s1ap_nas_first_req_p->ue_identity.s_tmsi.m_tmsi,
                               &ie->value.choice.S_TMSI.m_TMSI);
        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }


    /* optional */
    if (s1ap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_gummei) {
        S1AP_DEBUG("GUMMEI_ID_PRESENT\n");
        ie = (S1AP_InitialUEMessage_IEs_t *)calloc(1, sizeof(S1AP_InitialUEMessage_IEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_GUMMEI_ID;
        ie->criticality = S1AP_Criticality_reject;
        ie->value.present = S1AP_InitialUEMessage_IEs__value_PR_GUMMEI;
        MCC_MNC_TO_PLMNID(
            s1ap_nas_first_req_p->ue_identity.gummei.mcc,
            s1ap_nas_first_req_p->ue_identity.gummei.mnc,
            s1ap_nas_first_req_p->ue_identity.gummei.mnc_len,
            &ie->value.choice.GUMMEI.pLMN_Identity);
        MME_GID_TO_OCTET_STRING(s1ap_nas_first_req_p->ue_identity.gummei.mme_group_id,
                                &ie->value.choice.GUMMEI.mME_Group_ID);
        MME_CODE_TO_OCTET_STRING(s1ap_nas_first_req_p->ue_identity.gummei.mme_code,
                                 &ie->value.choice.GUMMEI.mME_Code);
        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }



    if (s1ap_eNB_encode_pdu(&pdu, &buffer, &length) < 0) {
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
        ue_desc_p->eNB_ue_s1ap_id);
    /* Send encoded message over sctp */
    s1ap_eNB_itti_send_sctp_data_req(instance_p->instance, mme_desc_p->assoc_id,
                                     buffer, length, ue_desc_p->tx_stream);
    return 0;
}

//------------------------------------------------------------------------------
int s1ap_eNB_handle_nas_downlink(uint32_t         assoc_id,
                                 uint32_t         stream,
                                 S1AP_S1AP_PDU_t *pdu)
//------------------------------------------------------------------------------
{
    s1ap_eNB_mme_data_t             *mme_desc_p        = NULL;
    s1ap_eNB_ue_context_t           *ue_desc_p         = NULL;
    s1ap_eNB_instance_t             *s1ap_eNB_instance = NULL;
    S1AP_DownlinkNASTransport_t     *container;
    S1AP_DownlinkNASTransport_IEs_t *ie;
    S1AP_ENB_UE_S1AP_ID_t            enb_ue_s1ap_id;
    S1AP_MME_UE_S1AP_ID_t            mme_ue_s1ap_id;
    DevAssert(pdu != NULL);

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
    /* Prepare the S1AP message to encode */
    container = &pdu->choice.initiatingMessage.value.choice.DownlinkNASTransport;
    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_DownlinkNASTransport_IEs_t, ie, container,
                               S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);
    if(ie == NULL)
    {
      return -1;
    }
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_DownlinkNASTransport_IEs_t, ie, container,
                               S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
    if(ie == NULL)
    {
      return -1;
    }
    enb_ue_s1ap_id = ie->value.choice.ENB_UE_S1AP_ID;

    if ((ue_desc_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance,
                     enb_ue_s1ap_id)) == NULL) {
        MSC_LOG_RX_DISCARDED_MESSAGE(
            MSC_S1AP_ENB,
            MSC_S1AP_MME,
            NULL,
            0,
            MSC_AS_TIME_FMT" downlinkNASTransport  eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
            enb_ue_s1ap_id,
            mme_ue_s1ap_id);
        S1AP_ERROR("[SCTP %d] Received NAS downlink message for non existing UE context eNB_UE_S1AP_ID: 0x%lx\n",
                   assoc_id,
                   enb_ue_s1ap_id);
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
        ue_desc_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
    } else {
        /* We already have a mme ue s1ap id check the received is the same */
        if (ue_desc_p->mme_ue_s1ap_id != mme_ue_s1ap_id) {
            S1AP_ERROR("[SCTP %d] Mismatch in MME UE S1AP ID (0x%lx != 0x%"PRIx32"\n",
                       assoc_id,
                       mme_ue_s1ap_id,
                       ue_desc_p->mme_ue_s1ap_id
                      );
            return -1;
        }
    }

    MSC_LOG_RX_MESSAGE(
        MSC_S1AP_ENB,
        MSC_S1AP_MME,
        NULL,
        0,
        MSC_AS_TIME_FMT" downlinkNASTransport  eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
        assoc_id,
        mme_ue_s1ap_id);

    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_DownlinkNASTransport_IEs_t, ie, container,
                               S1AP_ProtocolIE_ID_id_NAS_PDU, true);
    if(ie == NULL)
    {
      return -1;
    }
    /* Forward the NAS PDU to RRC */
    s1ap_eNB_itti_send_nas_downlink_ind(s1ap_eNB_instance->instance,
                                        ue_desc_p->ue_initial_id,
                                        ue_desc_p->eNB_ue_s1ap_id,
                                        ie->value.choice.NAS_PDU.buf,
                                        ie->value.choice.NAS_PDU.size);
    return 0;
}

//------------------------------------------------------------------------------
int s1ap_eNB_nas_uplink(instance_t instance, s1ap_uplink_nas_t *s1ap_uplink_nas_p)
//------------------------------------------------------------------------------
{
    struct s1ap_eNB_ue_context_s  *ue_context_p;
    s1ap_eNB_instance_t           *s1ap_eNB_instance_p;
    S1AP_S1AP_PDU_t                pdu;
    S1AP_UplinkNASTransport_t     *out;
    S1AP_UplinkNASTransport_IEs_t *ie;
    uint8_t  *buffer;
    uint32_t  length;
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
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_uplinkNASTransport;
    pdu.choice.initiatingMessage.criticality = S1AP_Criticality_ignore;
    pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_UplinkNASTransport;
    out = &pdu.choice.initiatingMessage.value.choice.UplinkNASTransport;
    /* mandatory */
    ie = (S1AP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(S1AP_UplinkNASTransport_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_UplinkNASTransport_IEs__value_PR_MME_UE_S1AP_ID;
    ie->value.choice.MME_UE_S1AP_ID = ue_context_p->mme_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(S1AP_UplinkNASTransport_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_UplinkNASTransport_IEs__value_PR_ENB_UE_S1AP_ID;
    ie->value.choice.ENB_UE_S1AP_ID = ue_context_p->eNB_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(S1AP_UplinkNASTransport_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_NAS_PDU;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_UplinkNASTransport_IEs__value_PR_NAS_PDU;
    ie->value.choice.NAS_PDU.buf = s1ap_uplink_nas_p->nas_pdu.buffer;
    ie->value.choice.NAS_PDU.size = s1ap_uplink_nas_p->nas_pdu.length;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(S1AP_UplinkNASTransport_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_EUTRAN_CGI;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_UplinkNASTransport_IEs__value_PR_EUTRAN_CGI;
    MCC_MNC_TO_PLMNID(
        s1ap_eNB_instance_p->mcc[ue_context_p->mme_ref->broadcast_plmn_index[0]],
        s1ap_eNB_instance_p->mnc[ue_context_p->mme_ref->broadcast_plmn_index[0]],
        s1ap_eNB_instance_p->mnc_digit_length[ue_context_p->mme_ref->broadcast_plmn_index[0]],
        &ie->value.choice.EUTRAN_CGI.pLMNidentity);
    //#warning "TODO get cell id from RRC"
    MACRO_ENB_ID_TO_CELL_IDENTITY(s1ap_eNB_instance_p->eNB_id,
                                  0,
                                  &ie->value.choice.EUTRAN_CGI.cell_ID);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(S1AP_UplinkNASTransport_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_TAI;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_UplinkNASTransport_IEs__value_PR_TAI;
    MCC_MNC_TO_PLMNID(
        s1ap_eNB_instance_p->mcc[ue_context_p->selected_plmn_identity],
        s1ap_eNB_instance_p->mnc[ue_context_p->selected_plmn_identity],
        s1ap_eNB_instance_p->mnc_digit_length[ue_context_p->selected_plmn_identity],
        &ie->value.choice.TAI.pLMNidentity);
    TAC_TO_ASN1(s1ap_eNB_instance_p->tac, &ie->value.choice.TAI.tAC);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* optional */


    if (s1ap_eNB_encode_pdu(&pdu, &buffer, &length) < 0) {
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
        ue_context_p->eNB_ue_s1ap_id,
        ue_context_p->mme_ue_s1ap_id);
    /* UE associated signalling -> use the allocated stream */
    s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);
    return 0;
}


//------------------------------------------------------------------------------
int s1ap_eNB_nas_non_delivery_ind(instance_t instance,
                                  s1ap_nas_non_delivery_ind_t *s1ap_nas_non_delivery_ind)
//------------------------------------------------------------------------------
{
    struct s1ap_eNB_ue_context_s        *ue_context_p;
    s1ap_eNB_instance_t                 *s1ap_eNB_instance_p;
    S1AP_S1AP_PDU_t                      pdu;
    S1AP_NASNonDeliveryIndication_t     *out;
    S1AP_NASNonDeliveryIndication_IEs_t *ie;
    uint8_t  *buffer;
    uint32_t  length;
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
        return -1;
    }

    /* Prepare the S1AP message to encode */
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_NASNonDeliveryIndication;
    pdu.choice.initiatingMessage.criticality = S1AP_Criticality_ignore;
    pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_NASNonDeliveryIndication;
    out = &pdu.choice.initiatingMessage.value.choice.NASNonDeliveryIndication;
    /* mandatory */
    ie = (S1AP_NASNonDeliveryIndication_IEs_t *)calloc(1, sizeof(S1AP_NASNonDeliveryIndication_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_NASNonDeliveryIndication_IEs__value_PR_MME_UE_S1AP_ID;
    ie->value.choice.MME_UE_S1AP_ID = ue_context_p->mme_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_NASNonDeliveryIndication_IEs_t *)calloc(1, sizeof(S1AP_NASNonDeliveryIndication_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_NASNonDeliveryIndication_IEs__value_PR_ENB_UE_S1AP_ID;
    ie->value.choice.ENB_UE_S1AP_ID = ue_context_p->eNB_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_NASNonDeliveryIndication_IEs_t *)calloc(1, sizeof(S1AP_NASNonDeliveryIndication_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_NAS_PDU;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_NASNonDeliveryIndication_IEs__value_PR_NAS_PDU;
    ie->value.choice.NAS_PDU.buf = s1ap_nas_non_delivery_ind->nas_pdu.buffer;
    ie->value.choice.NAS_PDU.size = s1ap_nas_non_delivery_ind->nas_pdu.length;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_NASNonDeliveryIndication_IEs_t *)calloc(1, sizeof(S1AP_NASNonDeliveryIndication_IEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_Cause;
    ie->criticality = S1AP_Criticality_ignore;
    /* Send a dummy cause */
    ie->value.present = S1AP_NASNonDeliveryIndication_IEs__value_PR_Cause;
    ie->value.choice.Cause.present = S1AP_Cause_PR_radioNetwork;
    ie->value.choice.Cause.choice.radioNetwork = S1AP_CauseRadioNetwork_radio_connection_with_ue_lost;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    if (s1ap_eNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        S1AP_ERROR("Failed to encode NAS NON delivery indication\n");
        /* Encode procedure has failed... */
        MSC_LOG_EVENT(
            MSC_S1AP_ENB,
            MSC_AS_TIME_FMT" Sent of NAS_NON_DELIVERY_IND to MME failed (encoding)");
        return -1;
    }

    MSC_LOG_TX_MESSAGE(
        MSC_S1AP_ENB,
        MSC_S1AP_MME,
        (const char *)buffer,
        length,
        MSC_AS_TIME_FMT" NASNonDeliveryIndication initiatingMessage eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
        0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->eNB_ue_s1ap_id,
        ue_context_p->mme_ue_s1ap_id);
    /* UE associated signalling -> use the allocated stream */
    s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);
    return 0;
}

//------------------------------------------------------------------------------
int s1ap_eNB_initial_ctxt_resp(
    instance_t instance, s1ap_initial_context_setup_resp_t *initial_ctxt_resp_p)
//------------------------------------------------------------------------------
{
    s1ap_eNB_instance_t                   *s1ap_eNB_instance_p = NULL;
    struct s1ap_eNB_ue_context_s          *ue_context_p        = NULL;
    S1AP_S1AP_PDU_t                        pdu;
    S1AP_InitialContextSetupResponse_t    *out;
    S1AP_InitialContextSetupResponseIEs_t *ie;
    uint8_t  *buffer = NULL;
    uint32_t length;
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
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = S1AP_S1AP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome.procedureCode = S1AP_ProcedureCode_id_InitialContextSetup;
    pdu.choice.successfulOutcome.criticality = S1AP_Criticality_reject;
    pdu.choice.successfulOutcome.value.present = S1AP_SuccessfulOutcome__value_PR_InitialContextSetupResponse;
    out = &pdu.choice.successfulOutcome.value.choice.InitialContextSetupResponse;
    /* mandatory */
    ie = (S1AP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(S1AP_InitialContextSetupResponseIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_InitialContextSetupResponseIEs__value_PR_MME_UE_S1AP_ID;
    ie->value.choice.MME_UE_S1AP_ID = ue_context_p->mme_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(S1AP_InitialContextSetupResponseIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_InitialContextSetupResponseIEs__value_PR_ENB_UE_S1AP_ID;
    ie->value.choice.ENB_UE_S1AP_ID = initial_ctxt_resp_p->eNB_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(S1AP_InitialContextSetupResponseIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_E_RABSetupListCtxtSURes;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_InitialContextSetupResponseIEs__value_PR_E_RABSetupListCtxtSURes;

    for (i = 0; i < initial_ctxt_resp_p->nb_of_e_rabs; i++) {
        S1AP_E_RABSetupItemCtxtSUResIEs_t *item;
        /* mandatory */
        item = (S1AP_E_RABSetupItemCtxtSUResIEs_t *)calloc(1, sizeof(S1AP_E_RABSetupItemCtxtSUResIEs_t));
        item->id = S1AP_ProtocolIE_ID_id_E_RABSetupItemCtxtSURes;
        item->criticality = S1AP_Criticality_ignore;
        item->value.present = S1AP_E_RABSetupItemCtxtSUResIEs__value_PR_E_RABSetupItemCtxtSURes;
        item->value.choice.E_RABSetupItemCtxtSURes.e_RAB_ID = initial_ctxt_resp_p->e_rabs[i].e_rab_id;
        GTP_TEID_TO_ASN1(initial_ctxt_resp_p->e_rabs[i].gtp_teid, &item->value.choice.E_RABSetupItemCtxtSURes.gTP_TEID);
        item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.buf = malloc(initial_ctxt_resp_p->e_rabs[i].eNB_addr.length);
        memcpy(item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.buf,
               initial_ctxt_resp_p->e_rabs[i].eNB_addr.buffer,
               initial_ctxt_resp_p->e_rabs[i].eNB_addr.length);
        item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.size = initial_ctxt_resp_p->e_rabs[i].eNB_addr.length;
        item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.bits_unused = 0;
        S1AP_DEBUG("initial_ctxt_resp_p: e_rab ID %ld, enb_addr %d.%d.%d.%d, SIZE %ld \n",
                   item->value.choice.E_RABSetupItemCtxtSURes.e_RAB_ID,
                   item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.buf[0],
                   item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.buf[1],
                   item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.buf[2],
                   item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.buf[3],
                   item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.size);
        ASN_SEQUENCE_ADD(&ie->value.choice.E_RABSetupListCtxtSURes.list, item);
    }

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* optional */
    if (initial_ctxt_resp_p->nb_of_e_rabs_failed) {
        ie = (S1AP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(S1AP_InitialContextSetupResponseIEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_E_RABFailedToSetupListCtxtSURes;
        ie->criticality = S1AP_Criticality_ignore;
        ie->value.present = S1AP_InitialContextSetupResponseIEs__value_PR_E_RABList;

        for (i = 0; i < initial_ctxt_resp_p->nb_of_e_rabs_failed; i++) {
            S1AP_E_RABItemIEs_t *item;
            /* mandatory */
            item = (S1AP_E_RABItemIEs_t *)calloc(1, sizeof(S1AP_E_RABItemIEs_t));
            item->id = S1AP_ProtocolIE_ID_id_E_RABItem;
            item->criticality = S1AP_Criticality_ignore;
            item->value.present = S1AP_E_RABItemIEs__value_PR_E_RABItem;
            item->value.choice.E_RABItem.e_RAB_ID = initial_ctxt_resp_p->e_rabs_failed[i].e_rab_id;
            item->value.choice.E_RABItem.cause.present = initial_ctxt_resp_p->e_rabs_failed[i].cause;

            switch(item->value.choice.E_RABItem.cause.present) {
            case S1AP_Cause_PR_radioNetwork:
                item->value.choice.E_RABItem.cause.choice.radioNetwork = initial_ctxt_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_transport:
                item->value.choice.E_RABItem.cause.choice.transport = initial_ctxt_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_nas:
                item->value.choice.E_RABItem.cause.choice.nas = initial_ctxt_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_protocol:
                item->value.choice.E_RABItem.cause.choice.protocol = initial_ctxt_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_misc:
                item->value.choice.E_RABItem.cause.choice.misc = initial_ctxt_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_NOTHING:
            default:
                break;
            }

            S1AP_DEBUG("initial context setup response: failed e_rab ID %ld\n", item->value.choice.E_RABItem.e_RAB_ID);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABList.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* optional */
    if (0) {
        ie = (S1AP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(S1AP_InitialContextSetupResponseIEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
        ie->criticality = S1AP_Criticality_ignore;
        ie->value.present = S1AP_InitialContextSetupResponseIEs__value_PR_CriticalityDiagnostics;
        // ie->value.choice.CriticalityDiagnostics =;
        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    if (s1ap_eNB_encode_pdu(&pdu, &buffer, &length) < 0) {
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
        initial_ctxt_resp_p->eNB_ue_s1ap_id,
        ue_context_p->mme_ue_s1ap_id);
    /* UE associated signalling -> use the allocated stream */
    s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);
    return 0;
}

//------------------------------------------------------------------------------
int s1ap_eNB_ue_capabilities(instance_t instance,
                             s1ap_ue_cap_info_ind_t *ue_cap_info_ind_p)
//------------------------------------------------------------------------------
{
    s1ap_eNB_instance_t          *s1ap_eNB_instance_p;
    struct s1ap_eNB_ue_context_s *ue_context_p;
    S1AP_S1AP_PDU_t                       pdu;
    S1AP_UECapabilityInfoIndication_t    *out;
    S1AP_UECapabilityInfoIndicationIEs_t *ie;
    uint8_t  *buffer;
    uint32_t length;
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
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_UECapabilityInfoIndication;
    pdu.choice.initiatingMessage.criticality = S1AP_Criticality_ignore;
    pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_UECapabilityInfoIndication;
    out = &pdu.choice.initiatingMessage.value.choice.UECapabilityInfoIndication;
    /* mandatory */
    ie = (S1AP_UECapabilityInfoIndicationIEs_t *)calloc(1, sizeof(S1AP_UECapabilityInfoIndicationIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_UECapabilityInfoIndicationIEs__value_PR_MME_UE_S1AP_ID;
    ie->value.choice.MME_UE_S1AP_ID = ue_context_p->mme_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_UECapabilityInfoIndicationIEs_t *)calloc(1, sizeof(S1AP_UECapabilityInfoIndicationIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_UECapabilityInfoIndicationIEs__value_PR_ENB_UE_S1AP_ID;
    ie->value.choice.ENB_UE_S1AP_ID = ue_cap_info_ind_p->eNB_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_UECapabilityInfoIndicationIEs_t *)calloc(1, sizeof(S1AP_UECapabilityInfoIndicationIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_UERadioCapability;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_UECapabilityInfoIndicationIEs__value_PR_UERadioCapability;
    ie->value.choice.UERadioCapability.buf = ue_cap_info_ind_p->ue_radio_cap.buffer;
    ie->value.choice.UERadioCapability.size = ue_cap_info_ind_p->ue_radio_cap.length;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* optional */

    if (s1ap_eNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        /* Encode procedure has failed... */
        S1AP_ERROR("Failed to encode UE capabilities indication\n");
        return -1;
    }

    MSC_LOG_TX_MESSAGE(
        MSC_S1AP_ENB,
        MSC_S1AP_MME,
        (const char *)buffer,
        length,
        MSC_AS_TIME_FMT" UECapabilityInfoIndication initiatingMessage eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
        0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
        ue_cap_info_ind_p->eNB_ue_s1ap_id,
        ue_context_p->mme_ue_s1ap_id);
    /* UE associated signalling -> use the allocated stream */
    s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);
    return 0;
}

//------------------------------------------------------------------------------
int s1ap_eNB_e_rab_setup_resp(instance_t instance,
                              s1ap_e_rab_setup_resp_t *e_rab_setup_resp_p)
//------------------------------------------------------------------------------
{
    s1ap_eNB_instance_t          *s1ap_eNB_instance_p = NULL;
    struct s1ap_eNB_ue_context_s *ue_context_p        = NULL;
    S1AP_S1AP_PDU_t               pdu;
    S1AP_E_RABSetupResponse_t    *out;
    S1AP_E_RABSetupResponseIEs_t *ie;
    uint8_t  *buffer  = NULL;
    uint32_t length;
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
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = S1AP_S1AP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome.procedureCode = S1AP_ProcedureCode_id_E_RABSetup;
    pdu.choice.successfulOutcome.criticality = S1AP_Criticality_reject;
    pdu.choice.successfulOutcome.value.present = S1AP_SuccessfulOutcome__value_PR_E_RABSetupResponse;
    out = &pdu.choice.successfulOutcome.value.choice.E_RABSetupResponse;
    /* mandatory */
    ie = (S1AP_E_RABSetupResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABSetupResponseIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_E_RABSetupResponseIEs__value_PR_MME_UE_S1AP_ID;
    ie->value.choice.MME_UE_S1AP_ID = ue_context_p->mme_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_E_RABSetupResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABSetupResponseIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_E_RABSetupResponseIEs__value_PR_ENB_UE_S1AP_ID;
    ie->value.choice.ENB_UE_S1AP_ID = e_rab_setup_resp_p->eNB_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* optional */
    if (e_rab_setup_resp_p->nb_of_e_rabs > 0) {
        ie = (S1AP_E_RABSetupResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABSetupResponseIEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_E_RABSetupListBearerSURes;
        ie->criticality = S1AP_Criticality_ignore;
        ie->value.present = S1AP_E_RABSetupResponseIEs__value_PR_E_RABSetupListBearerSURes;

        for (i = 0; i < e_rab_setup_resp_p->nb_of_e_rabs; i++) {
            S1AP_E_RABSetupItemBearerSUResIEs_t *item;
            /* mandatory */
            item = (S1AP_E_RABSetupItemBearerSUResIEs_t *)calloc(1, sizeof(S1AP_E_RABSetupItemBearerSUResIEs_t));
            item->id = S1AP_ProtocolIE_ID_id_E_RABSetupItemBearerSURes;
            item->criticality = S1AP_Criticality_ignore;
            item->value.present = S1AP_E_RABSetupItemBearerSUResIEs__value_PR_E_RABSetupItemBearerSURes;
            item->value.choice.E_RABSetupItemBearerSURes.e_RAB_ID = e_rab_setup_resp_p->e_rabs[i].e_rab_id;
            GTP_TEID_TO_ASN1(e_rab_setup_resp_p->e_rabs[i].gtp_teid, &item->value.choice.E_RABSetupItemBearerSURes.gTP_TEID);
            item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.buf = malloc(e_rab_setup_resp_p->e_rabs[i].eNB_addr.length);
            memcpy(item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.buf,
                   e_rab_setup_resp_p->e_rabs[i].eNB_addr.buffer,
                   e_rab_setup_resp_p->e_rabs[i].eNB_addr.length);
            item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.size = e_rab_setup_resp_p->e_rabs[i].eNB_addr.length;
            item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.bits_unused = 0;
            S1AP_DEBUG("e_rab_setup_resp: e_rab ID %ld, teid %u, enb_addr %d.%d.%d.%d, SIZE %ld\n",
                       item->value.choice.E_RABSetupItemBearerSURes.e_RAB_ID,
                       e_rab_setup_resp_p->e_rabs[i].gtp_teid,
                       item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.buf[0],
                       item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.buf[1],
                       item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.buf[2],
                       item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.buf[3],
                       item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.size);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABSetupListBearerSURes.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* optional */
    if (e_rab_setup_resp_p->nb_of_e_rabs_failed > 0) {
        ie = (S1AP_E_RABSetupResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABSetupResponseIEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_E_RABFailedToSetupListBearerSURes;
        ie->criticality = S1AP_Criticality_ignore;
        ie->value.present = S1AP_E_RABSetupResponseIEs__value_PR_E_RABList;

        for (i = 0; i < e_rab_setup_resp_p->nb_of_e_rabs_failed; i++) {
            S1AP_E_RABItemIEs_t *item;
            item = (S1AP_E_RABItemIEs_t *)calloc(1, sizeof(S1AP_E_RABItemIEs_t));
            item->id = S1AP_ProtocolIE_ID_id_E_RABItem;
            item->criticality = S1AP_Criticality_ignore;
            item->value.present = S1AP_E_RABItemIEs__value_PR_E_RABItem;
            item->value.choice.E_RABItem.e_RAB_ID = e_rab_setup_resp_p->e_rabs_failed[i].e_rab_id;
            item->value.choice.E_RABItem.cause.present = e_rab_setup_resp_p->e_rabs_failed[i].cause;

            switch(item->value.choice.E_RABItem.cause.present) {
            case S1AP_Cause_PR_radioNetwork:
                item->value.choice.E_RABItem.cause.choice.radioNetwork = e_rab_setup_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_transport:
                item->value.choice.E_RABItem.cause.choice.transport = e_rab_setup_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_nas:
                item->value.choice.E_RABItem.cause.choice.nas = e_rab_setup_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_protocol:
                item->value.choice.E_RABItem.cause.choice.protocol = e_rab_setup_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_misc:
                item->value.choice.E_RABItem.cause.choice.misc = e_rab_setup_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_NOTHING:
            default:
                break;
            }

            S1AP_DEBUG("e_rab_setup_resp: failed e_rab ID %ld\n", item->value.choice.E_RABItem.e_RAB_ID);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABList.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* optional */
    if (0) {
        ie = (S1AP_E_RABSetupResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABSetupResponseIEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
        ie->criticality = S1AP_Criticality_ignore;
        ie->value.present = S1AP_E_RABSetupResponseIEs__value_PR_CriticalityDiagnostics;
        // ie->value.choice.CriticalityDiagnostics = ;
        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* S1AP_E_RABSetupListBearerSURes_t  e_RABSetupListBearerSURes;
    memset(&e_RABSetupListBearerSURes, 0, sizeof(S1AP_E_RABSetupListBearerSURes_t));
    if (s1ap_encode_s1ap_e_rabsetuplistbearersures(&e_RABSetupListBearerSURes, &initial_ies_p->e_RABSetupListBearerSURes.s1ap_E_RABSetupItemBearerSURes) < 0 )
      return -1;
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1AP_E_RABSetupListBearerSURes, &e_RABSetupListBearerSURes);
    */
    fprintf(stderr, "start encode\n");

    if (s1ap_eNB_encode_pdu(&pdu, &buffer, &length) < 0) {
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
        e_rab_setup_resp_p->eNB_ue_s1ap_id,
        ue_context_p->mme_ue_s1ap_id);
    /* UE associated signalling -> use the allocated stream */
    s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);
    return 0;
}

//------------------------------------------------------------------------------
int s1ap_eNB_e_rab_modify_resp(instance_t instance,
                               s1ap_e_rab_modify_resp_t *e_rab_modify_resp_p)
//------------------------------------------------------------------------------
{
    s1ap_eNB_instance_t           *s1ap_eNB_instance_p = NULL;
    struct s1ap_eNB_ue_context_s  *ue_context_p        = NULL;
    S1AP_S1AP_PDU_t                pdu;
    S1AP_E_RABModifyResponse_t    *out;
    S1AP_E_RABModifyResponseIEs_t *ie;
    uint8_t  *buffer  = NULL;
    uint32_t length;
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
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = S1AP_S1AP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome.procedureCode = S1AP_ProcedureCode_id_E_RABModify;
    pdu.choice.successfulOutcome.criticality = S1AP_Criticality_reject;
    pdu.choice.successfulOutcome.value.present = S1AP_SuccessfulOutcome__value_PR_E_RABModifyResponse;
    out = &pdu.choice.successfulOutcome.value.choice.E_RABModifyResponse;
    /* mandatory */
    ie = (S1AP_E_RABModifyResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABModifyResponseIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_E_RABModifyResponseIEs__value_PR_MME_UE_S1AP_ID;
    ie->value.choice.MME_UE_S1AP_ID = ue_context_p->mme_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_E_RABModifyResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABModifyResponseIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_E_RABModifyResponseIEs__value_PR_ENB_UE_S1AP_ID;
    ie->value.choice.ENB_UE_S1AP_ID = e_rab_modify_resp_p->eNB_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* optional */
    if (e_rab_modify_resp_p->nb_of_e_rabs > 0) {
        ie = (S1AP_E_RABModifyResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABModifyResponseIEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_E_RABModifyListBearerModRes;
        ie->criticality = S1AP_Criticality_ignore;
        ie->value.present = S1AP_E_RABModifyResponseIEs__value_PR_E_RABModifyListBearerModRes;

        for (i = 0; i < e_rab_modify_resp_p->nb_of_e_rabs; i++) {
            S1AP_E_RABModifyItemBearerModResIEs_t *item;
            item = (S1AP_E_RABModifyItemBearerModResIEs_t *)calloc(1, sizeof(S1AP_E_RABModifyItemBearerModResIEs_t));
            item->id = S1AP_ProtocolIE_ID_id_E_RABModifyItemBearerModRes;
            item->criticality = S1AP_Criticality_ignore;
            item->value.present = S1AP_E_RABModifyItemBearerModResIEs__value_PR_E_RABModifyItemBearerModRes;
            item->value.choice.E_RABModifyItemBearerModRes.e_RAB_ID = e_rab_modify_resp_p->e_rabs[i].e_rab_id;
            S1AP_DEBUG("e_rab_modify_resp: modified e_rab ID %ld\n", item->value.choice.E_RABModifyItemBearerModRes.e_RAB_ID);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABModifyListBearerModRes.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* optional */
    if (e_rab_modify_resp_p->nb_of_e_rabs_failed > 0) {
        ie = (S1AP_E_RABModifyResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABModifyResponseIEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_E_RABFailedToModifyList;
        ie->criticality = S1AP_Criticality_ignore;
        ie->value.present = S1AP_E_RABModifyResponseIEs__value_PR_E_RABList;

        for (i = 0; i < e_rab_modify_resp_p->nb_of_e_rabs_failed; i++) {
            S1AP_E_RABItemIEs_t *item;
            item = (S1AP_E_RABItemIEs_t *)calloc(1, sizeof(S1AP_E_RABItemIEs_t));
            item->id = S1AP_ProtocolIE_ID_id_E_RABItem;
            item->criticality = S1AP_Criticality_ignore;
            item->value.present = S1AP_E_RABItemIEs__value_PR_E_RABItem;
            item->value.choice.E_RABItem.e_RAB_ID = e_rab_modify_resp_p->e_rabs_failed[i].e_rab_id;
            item->value.choice.E_RABItem.cause.present = e_rab_modify_resp_p->e_rabs_failed[i].cause;

            switch(item->value.choice.E_RABItem.cause.present) {
            case S1AP_Cause_PR_radioNetwork:
                item->value.choice.E_RABItem.cause.choice.radioNetwork = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_transport:
                item->value.choice.E_RABItem.cause.choice.transport = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_nas:
                item->value.choice.E_RABItem.cause.choice.nas = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_protocol:
                item->value.choice.E_RABItem.cause.choice.protocol = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_misc:
                item->value.choice.E_RABItem.cause.choice.misc = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_NOTHING:
            default:
                break;
            }

            S1AP_DEBUG("e_rab_modify_resp: failed e_rab ID %ld\n", item->value.choice.E_RABItem.e_RAB_ID);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABList.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* optional */
    if (0) {
        ie = (S1AP_E_RABModifyResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABModifyResponseIEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
        ie->criticality = S1AP_Criticality_ignore;
        ie->value.present = S1AP_E_RABModifyResponseIEs__value_PR_CriticalityDiagnostics;
        // ie->value.choice.CriticalityDiagnostics = ;
        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    fprintf(stderr, "start encode\n");

    if (s1ap_eNB_encode_pdu(&pdu, &buffer, &length) < 0) {
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
        e_rab_modify_resp_p->eNB_ue_s1ap_id,
        ue_context_p->mme_ue_s1ap_id);
    /* UE associated signalling -> use the allocated stream */
    s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);
    return 0;
}
//------------------------------------------------------------------------------
int s1ap_eNB_e_rab_release_resp(instance_t instance,
                                s1ap_e_rab_release_resp_t *e_rab_release_resp_p)
//------------------------------------------------------------------------------
{
    s1ap_eNB_instance_t            *s1ap_eNB_instance_p = NULL;
    struct s1ap_eNB_ue_context_s   *ue_context_p        = NULL;
    S1AP_S1AP_PDU_t                 pdu;
    S1AP_E_RABReleaseResponse_t    *out;
    S1AP_E_RABReleaseResponseIEs_t *ie;
    uint8_t  *buffer  = NULL;
    uint32_t length;
    int      i;
    /* Retrieve the S1AP eNB instance associated with Mod_id */
    s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);
    DevAssert(e_rab_release_resp_p != NULL);
    DevAssert(s1ap_eNB_instance_p != NULL);

    if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p,
                        e_rab_release_resp_p->eNB_ue_s1ap_id)) == NULL) {
        /* The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs */
        S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: %u\n",
                  e_rab_release_resp_p->eNB_ue_s1ap_id);
        return -1;
    }

    /* Prepare the S1AP message to encode */
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = S1AP_S1AP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome.procedureCode = S1AP_ProcedureCode_id_E_RABRelease;
    pdu.choice.successfulOutcome.criticality = S1AP_Criticality_reject;
    pdu.choice.successfulOutcome.value.present = S1AP_SuccessfulOutcome__value_PR_E_RABReleaseResponse;
    out = &pdu.choice.successfulOutcome.value.choice.E_RABReleaseResponse;
    /* mandatory */
    ie = (S1AP_E_RABReleaseResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABReleaseResponseIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_E_RABReleaseResponseIEs__value_PR_MME_UE_S1AP_ID;
    ie->value.choice.MME_UE_S1AP_ID = ue_context_p->mme_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (S1AP_E_RABReleaseResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABReleaseResponseIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_E_RABReleaseResponseIEs__value_PR_ENB_UE_S1AP_ID;
    ie->value.choice.ENB_UE_S1AP_ID = e_rab_release_resp_p->eNB_ue_s1ap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* optional */
    if (e_rab_release_resp_p->nb_of_e_rabs_released > 0) {
        ie = (S1AP_E_RABReleaseResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABReleaseResponseIEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_E_RABReleaseListBearerRelComp;
        ie->criticality = S1AP_Criticality_ignore;
        ie->value.present = S1AP_E_RABReleaseResponseIEs__value_PR_E_RABReleaseListBearerRelComp;

        for (i = 0; i < e_rab_release_resp_p->nb_of_e_rabs_released; i++) {
            S1AP_E_RABReleaseItemBearerRelCompIEs_t *item;
            item = (S1AP_E_RABReleaseItemBearerRelCompIEs_t *)calloc(1, sizeof(S1AP_E_RABReleaseItemBearerRelCompIEs_t));
            item->id = S1AP_ProtocolIE_ID_id_E_RABReleaseItemBearerRelComp;
            item->criticality = S1AP_Criticality_ignore;
            item->value.present = S1AP_E_RABReleaseItemBearerRelCompIEs__value_PR_E_RABReleaseItemBearerRelComp;
            item->value.choice.E_RABReleaseItemBearerRelComp.e_RAB_ID = e_rab_release_resp_p->e_rab_release[i].e_rab_id;
            S1AP_DEBUG("e_rab_release_resp: e_rab ID %ld\n", item->value.choice.E_RABReleaseItemBearerRelComp.e_RAB_ID);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABReleaseListBearerRelComp.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* optional */
    if (e_rab_release_resp_p->nb_of_e_rabs_failed > 0) {
        ie = (S1AP_E_RABReleaseResponseIEs_t *)calloc(1, sizeof(S1AP_E_RABReleaseResponseIEs_t));
        ie->id = S1AP_ProtocolIE_ID_id_E_RABFailedToReleaseList;
        ie->criticality = S1AP_Criticality_ignore;
        ie->value.present = S1AP_E_RABReleaseResponseIEs__value_PR_E_RABList;

        for (i = 0; i < e_rab_release_resp_p->nb_of_e_rabs_failed; i++) {
            S1AP_E_RABItemIEs_t *item;
            item = (S1AP_E_RABItemIEs_t *)calloc(1, sizeof(S1AP_E_RABItemIEs_t));
            item->id = S1AP_ProtocolIE_ID_id_E_RABItem;
            item->criticality = S1AP_Criticality_ignore;
            item->value.present = S1AP_E_RABItemIEs__value_PR_E_RABItem;
            item->value.choice.E_RABItem.e_RAB_ID = e_rab_release_resp_p->e_rabs_failed[i].e_rab_id;
            item->value.choice.E_RABItem.cause.present = e_rab_release_resp_p->e_rabs_failed[i].cause;

            switch(item->value.choice.E_RABItem.cause.present) {
            case S1AP_Cause_PR_radioNetwork:
                item->value.choice.E_RABItem.cause.choice.radioNetwork = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_transport:
                item->value.choice.E_RABItem.cause.choice.transport = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_nas:
                item->value.choice.E_RABItem.cause.choice.nas = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_protocol:
                item->value.choice.E_RABItem.cause.choice.protocol = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_misc:
                item->value.choice.E_RABItem.cause.choice.misc = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
                break;

            case S1AP_Cause_PR_NOTHING:
            default:
                break;
            }

            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABList.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }



    if (s1ap_eNB_encode_pdu(&pdu, &buffer, &length) < 0) {
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
    return 0;
}

int s1ap_eNB_path_switch_req(instance_t instance,
                             s1ap_path_switch_req_t *path_switch_req_p)
//------------------------------------------------------------------------------
{
  s1ap_eNB_instance_t          *s1ap_eNB_instance_p = NULL;
  struct s1ap_eNB_ue_context_s *ue_context_p        = NULL;
  struct s1ap_eNB_mme_data_s   *mme_desc_p = NULL;

  S1AP_S1AP_PDU_t                 pdu;
  S1AP_PathSwitchRequest_t       *out;
  S1AP_PathSwitchRequestIEs_t    *ie;

  S1AP_E_RABToBeSwitchedDLItemIEs_t *e_RABToBeSwitchedDLItemIEs;
  S1AP_E_RABToBeSwitchedDLItem_t    *e_RABToBeSwitchedDLItem;

  uint8_t  *buffer = NULL;
  uint32_t length;
  int      ret = 0;//-1;

  /* Retrieve the S1AP eNB instance associated with Mod_id */
  s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);

  DevAssert(path_switch_req_p != NULL);
  DevAssert(s1ap_eNB_instance_p != NULL);

  //if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p,
    //                                          path_switch_req_p->eNB_ue_s1ap_id)) == NULL) {
    /* The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs */
    //S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: 0x%06x\n",
      //        path_switch_req_p->eNB_ue_s1ap_id);
    //return -1;
  //}

  /* Uplink NAS transport can occur either during an s1ap connected state
   * or during initial attach (for example: NAS authentication).
   */
  //if (!(ue_context_p->ue_state == S1AP_UE_CONNECTED ||
       // ue_context_p->ue_state == S1AP_UE_WAITING_CSR)) {
    //S1AP_WARN("You are attempting to send NAS data over non-connected "
        //      "eNB ue s1ap id: %06x, current state: %d\n",
          //    path_switch_req_p->eNB_ue_s1ap_id, ue_context_p->ue_state);
    //return -1;
  //}

  /* Select the MME corresponding to the provided GUMMEI. */
  mme_desc_p = s1ap_eNB_nnsf_select_mme_by_gummei_no_cause(s1ap_eNB_instance_p, path_switch_req_p->ue_gummei);

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
  ue_context_p = s1ap_eNB_allocate_new_UE_context();
  DevAssert(ue_context_p != NULL);

  /* Keep a reference to the selected MME */
  ue_context_p->mme_ref       = mme_desc_p;
  ue_context_p->ue_initial_id = path_switch_req_p->ue_initial_id;
  ue_context_p->eNB_instance  = s1ap_eNB_instance_p;

  do {
    struct s1ap_eNB_ue_context_s *collision_p;

    /* Peek a random value for the eNB_ue_s1ap_id */
    ue_context_p->eNB_ue_s1ap_id = (random() + random()) & 0x00ffffff;

    if ((collision_p = RB_INSERT(s1ap_ue_map, &s1ap_eNB_instance_p->s1ap_ue_head, ue_context_p))
        == NULL) {
      S1AP_DEBUG("Found usable eNB_ue_s1ap_id: 0x%06x %u(10)\n",
                 ue_context_p->eNB_ue_s1ap_id,
                 ue_context_p->eNB_ue_s1ap_id);
      /* Break the loop as the id is not already used by another UE */
      break;
    }
  } while(1);
  
  ue_context_p->mme_ue_s1ap_id = path_switch_req_p->mme_ue_s1ap_id;

  /* Prepare the S1AP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_PathSwitchRequest;
  pdu.choice.initiatingMessage.criticality = S1AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_PathSwitchRequest;
  out = &pdu.choice.initiatingMessage.value.choice.PathSwitchRequest;

  /* mandatory */
  ie = (S1AP_PathSwitchRequestIEs_t *)calloc(1, sizeof(S1AP_PathSwitchRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_PathSwitchRequestIEs__value_PR_ENB_UE_S1AP_ID;
  ie->value.choice.ENB_UE_S1AP_ID = ue_context_p->eNB_ue_s1ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  if (path_switch_req_p->nb_of_e_rabs > 0) {
    ie = (S1AP_PathSwitchRequestIEs_t *)calloc(1, sizeof(S1AP_PathSwitchRequestIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_E_RABToBeSwitchedDLList;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_PathSwitchRequestIEs__value_PR_E_RABToBeSwitchedDLList;

    for (int i = 0; i < path_switch_req_p->nb_of_e_rabs; i++) {
      e_RABToBeSwitchedDLItemIEs = (S1AP_E_RABToBeSwitchedDLItemIEs_t *)calloc(1, sizeof(S1AP_E_RABToBeSwitchedDLItemIEs_t));
      e_RABToBeSwitchedDLItemIEs->id = S1AP_ProtocolIE_ID_id_E_RABToBeSwitchedDLItem;
      e_RABToBeSwitchedDLItemIEs->criticality = S1AP_Criticality_reject;
      e_RABToBeSwitchedDLItemIEs->value.present = S1AP_E_RABToBeSwitchedDLItemIEs__value_PR_E_RABToBeSwitchedDLItem;

      e_RABToBeSwitchedDLItem = &e_RABToBeSwitchedDLItemIEs->value.choice.E_RABToBeSwitchedDLItem;
      e_RABToBeSwitchedDLItem->e_RAB_ID = path_switch_req_p->e_rabs_tobeswitched[i].e_rab_id;
      INT32_TO_OCTET_STRING(path_switch_req_p->e_rabs_tobeswitched[i].gtp_teid, &e_RABToBeSwitchedDLItem->gTP_TEID);

      e_RABToBeSwitchedDLItem->transportLayerAddress.size  = path_switch_req_p->e_rabs_tobeswitched[i].eNB_addr.length;
      e_RABToBeSwitchedDLItem->transportLayerAddress.bits_unused = 0;

      e_RABToBeSwitchedDLItem->transportLayerAddress.buf = calloc(1,e_RABToBeSwitchedDLItem->transportLayerAddress.size);

      memcpy (e_RABToBeSwitchedDLItem->transportLayerAddress.buf,
                path_switch_req_p->e_rabs_tobeswitched[i].eNB_addr.buffer,
                path_switch_req_p->e_rabs_tobeswitched[i].eNB_addr.length);

      S1AP_DEBUG("path_switch_req: e_rab ID %ld, teid %u, enb_addr %d.%d.%d.%d, SIZE %zu\n",
               e_RABToBeSwitchedDLItem->e_RAB_ID,
               path_switch_req_p->e_rabs_tobeswitched[i].gtp_teid,
               e_RABToBeSwitchedDLItem->transportLayerAddress.buf[0],
               e_RABToBeSwitchedDLItem->transportLayerAddress.buf[1],
               e_RABToBeSwitchedDLItem->transportLayerAddress.buf[2],
               e_RABToBeSwitchedDLItem->transportLayerAddress.buf[3],
               e_RABToBeSwitchedDLItem->transportLayerAddress.size);

      ASN_SEQUENCE_ADD(&ie->value.choice.E_RABToBeSwitchedDLList.list, e_RABToBeSwitchedDLItemIEs);
    }

    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  ie = (S1AP_PathSwitchRequestIEs_t *)calloc(1, sizeof(S1AP_PathSwitchRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_SourceMME_UE_S1AP_ID;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_PathSwitchRequestIEs__value_PR_MME_UE_S1AP_ID;
  ie->value.choice.MME_UE_S1AP_ID = path_switch_req_p->mme_ue_s1ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (S1AP_PathSwitchRequestIEs_t *)calloc(1, sizeof(S1AP_PathSwitchRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_EUTRAN_CGI;
  ie->criticality = S1AP_Criticality_ignore;
  ie->value.present = S1AP_PathSwitchRequestIEs__value_PR_EUTRAN_CGI;
  MACRO_ENB_ID_TO_CELL_IDENTITY(s1ap_eNB_instance_p->eNB_id,
                                0,
                                &ie->value.choice.EUTRAN_CGI.cell_ID);
  MCC_MNC_TO_TBCD(s1ap_eNB_instance_p->mcc[0],
                  s1ap_eNB_instance_p->mnc[0],
                  s1ap_eNB_instance_p->mnc_digit_length[0],
                  &ie->value.choice.EUTRAN_CGI.pLMNidentity);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (S1AP_PathSwitchRequestIEs_t *)calloc(1, sizeof(S1AP_PathSwitchRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_TAI;
  ie->criticality = S1AP_Criticality_ignore;
  ie->value.present = S1AP_PathSwitchRequestIEs__value_PR_TAI;
  /* Assuming TAI is the TAI from the cell */
  INT16_TO_OCTET_STRING(s1ap_eNB_instance_p->tac, &ie->value.choice.TAI.tAC);
  MCC_MNC_TO_PLMNID(s1ap_eNB_instance_p->mcc[0],
                    s1ap_eNB_instance_p->mnc[0],
                    s1ap_eNB_instance_p->mnc_digit_length[0],
                    &ie->value.choice.TAI.pLMNidentity);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (S1AP_PathSwitchRequestIEs_t *)calloc(1, sizeof(S1AP_PathSwitchRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_UESecurityCapabilities;
  ie->criticality = S1AP_Criticality_ignore;
  ie->value.present = S1AP_PathSwitchRequestIEs__value_PR_UESecurityCapabilities;
  ENCRALG_TO_BIT_STRING(path_switch_req_p->security_capabilities.encryption_algorithms,
              &ie->value.choice.UESecurityCapabilities.encryptionAlgorithms);
  INTPROTALG_TO_BIT_STRING(path_switch_req_p->security_capabilities.integrity_algorithms,
              &ie->value.choice.UESecurityCapabilities.integrityProtectionAlgorithms);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (s1ap_eNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    S1AP_ERROR("Failed to encode Path Switch Req \n");
    /* Encode procedure has failed... */
    return -1;
  }

  /* Update the current S1AP UE state */
  ue_context_p->ue_state = S1AP_UE_WAITING_CSR;

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

  ue_context_p->tx_stream = mme_desc_p->nextstream;

  MSC_LOG_TX_MESSAGE(
    MSC_S1AP_ENB,
    MSC_S1AP_MME,
    (const char *)buffer,
    length,
    MSC_AS_TIME_FMT" E_RAN Setup successfulOutcome eNB_ue_s1ap_id %u mme_ue_s1ap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_p->eNB_ue_s1ap_id,
    path_switch_req_p->mme_ue_s1ap_id);

  /* UE associated signalling -> use the allocated stream */
  s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                   mme_desc_p->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return ret;
}


//-----------------------------------------------------------------------------
/*
* eNB generate a S1 E_RAB Modification Indication towards MME
*/
/*int s1ap_eNB_generate_E_RAB_Modification_Indication(
		instance_t instance,
  s1ap_e_rab_modification_ind_t *e_rab_modification_ind)
//-----------------------------------------------------------------------------
{
  struct s1ap_eNB_ue_context_s        *ue_context_p        = NULL;
  S1AP_S1AP_PDU_t            pdu;
  S1AP_E_RABModificationIndication_t     *out = NULL;
  S1AP_E_RABModificationIndicationIEs_t   *ie = NULL;
  S1AP_E_RABToBeModifiedItemBearerModInd_t 	  *E_RAB_ToBeModifiedItem_BearerModInd = NULL;
  S1AP_E_RABToBeModifiedItemBearerModIndIEs_t *E_RAB_ToBeModifiedItem_BearerModInd_IEs = NULL;

  S1AP_E_RABNotToBeModifiedItemBearerModInd_t 	  *E_RAB_NotToBeModifiedItem_BearerModInd = NULL;
  S1AP_E_RABNotToBeModifiedItemBearerModIndIEs_t  *E_RAB_NotToBeModifiedItem_BearerModInd_IEs = NULL;


  s1ap_eNB_instance_t          *s1ap_eNB_instance_p = NULL;
  s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);
  uint8_t  *buffer = NULL;
  uint32_t  len = 0;
  int       ret = 0;
  DevAssert(s1ap_eNB_instance_p != NULL);
  DevAssert(e_rab_modification_ind != NULL);

  int num_e_rabs_tobemodified = e_rab_modification_ind->nb_of_e_rabs_tobemodified;
  int num_e_rabs_nottobemodified = e_rab_modification_ind->nb_of_e_rabs_nottobemodified;

  uint32_t CSG_id = 0;

  if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p,
		  e_rab_modification_ind->eNB_ue_s1ap_id)) == NULL) {
          // The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs 
          S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: 0x%06x\n",
        		  e_rab_modification_ind->eNB_ue_s1ap_id);
          return -1;
  }

  // Prepare the S1AP message to encode 
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_E_RABModificationIndication;
  pdu.choice.initiatingMessage.criticality = S1AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_E_RABModificationIndication;
  out = &pdu.choice.initiatingMessage.value.choice.E_RABModificationIndication;
  // mandatory 
  ie = (S1AP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationIndicationIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_MME_UE_S1AP_ID;
  ie->value.choice.MME_UE_S1AP_ID = e_rab_modification_ind->mme_ue_s1ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ie = (S1AP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationIndicationIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_ENB_UE_S1AP_ID;
  ie->value.choice.ENB_UE_S1AP_ID = e_rab_modification_ind->eNB_ue_s1ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  //E-RABs to be modified list
  ie = (S1AP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationIndicationIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_E_RABToBeModifiedListBearerModInd;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_E_RABToBeModifiedListBearerModInd;

  //The following two for-loops here will probably need to change. We should do a different type of search
  for(int i=0; i<num_e_rabs_tobemodified; i++){
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs = (S1AP_E_RABToBeModifiedItemBearerModIndIEs_t *)calloc(1,sizeof(S1AP_E_RABToBeModifiedItemBearerModIndIEs_t));
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->id = S1AP_ProtocolIE_ID_id_E_RABToBeModifiedItemBearerModInd;
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->criticality = S1AP_Criticality_reject;
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->value.present = S1AP_E_RABToBeModifiedItemBearerModIndIEs__value_PR_E_RABToBeModifiedItemBearerModInd;
	  E_RAB_ToBeModifiedItem_BearerModInd = &E_RAB_ToBeModifiedItem_BearerModInd_IEs->value.choice.E_RABToBeModifiedItemBearerModInd;

	  {
	  E_RAB_ToBeModifiedItem_BearerModInd->e_RAB_ID = e_rab_modification_ind->e_rabs_tobemodified[i].e_rab_id;

	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size  = e_rab_modification_ind->e_rabs_tobemodified[i].eNB_addr.length/8;
	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.bits_unused = e_rab_modification_ind->e_rabs_tobemodified[i].eNB_addr.length%8;
	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.buf = calloc(1, E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size);
	  memcpy (E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.buf, e_rab_modification_ind->e_rabs_tobemodified[i].eNB_addr.buffer,
			  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size);

	  INT32_TO_OCTET_STRING(e_rab_modification_ind->e_rabs_tobemodified[i].gtp_teid, &E_RAB_ToBeModifiedItem_BearerModInd->dL_GTP_TEID);

	  }
	  ASN_SEQUENCE_ADD(&ie->value.choice.E_RABToBeModifiedListBearerModInd.list, E_RAB_ToBeModifiedItem_BearerModInd_IEs);
  }

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  //E-RABs NOT to be modified list
  ie = (S1AP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationIndicationIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_E_RABNotToBeModifiedListBearerModInd;
  ie->criticality = S1AP_Criticality_reject;
  if(num_e_rabs_nottobemodified > 0) {
	  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_E_RABNotToBeModifiedListBearerModInd;

	  for(int i=0; i<num_e_rabs_nottobemodified; i++){
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs = (S1AP_E_RABNotToBeModifiedItemBearerModIndIEs_t *)calloc(1,sizeof(S1AP_E_RABNotToBeModifiedItemBearerModIndIEs_t));
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->id = S1AP_ProtocolIE_ID_id_E_RABNotToBeModifiedItemBearerModInd;
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->criticality = S1AP_Criticality_reject;
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->value.present = S1AP_E_RABNotToBeModifiedItemBearerModIndIEs__value_PR_E_RABNotToBeModifiedItemBearerModInd;
		  E_RAB_NotToBeModifiedItem_BearerModInd = &E_RAB_NotToBeModifiedItem_BearerModInd_IEs->value.choice.E_RABNotToBeModifiedItemBearerModInd;

		  {
			  E_RAB_NotToBeModifiedItem_BearerModInd->e_RAB_ID = e_rab_modification_ind->e_rabs_nottobemodified[i].e_rab_id;

			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size  = e_rab_modification_ind->e_rabs_nottobemodified[i].eNB_addr.length/8;
			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.bits_unused = e_rab_modification_ind->e_rabs_nottobemodified[i].eNB_addr.length%8;
			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.buf =
	  	    				calloc(1, E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size);
			  memcpy (E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.buf, e_rab_modification_ind->e_rabs_nottobemodified[i].eNB_addr.buffer,
					  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size);

			  INT32_TO_OCTET_STRING(e_rab_modification_ind->e_rabs_nottobemodified[i].gtp_teid, &E_RAB_NotToBeModifiedItem_BearerModInd->dL_GTP_TEID);

		  }
		  ASN_SEQUENCE_ADD(&ie->value.choice.E_RABNotToBeModifiedListBearerModInd.list, E_RAB_NotToBeModifiedItem_BearerModInd_IEs);
	  }
  }
  else{
	  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_E_RABNotToBeModifiedListBearerModInd;
	  ie->value.choice.E_RABNotToBeModifiedListBearerModInd.list.size = 0;
  }  
  
	   

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ie = (S1AP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationIndicationIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_CSGMembershipInfo;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_CSGMembershipInfo;
  ie->value.choice.CSGMembershipInfo.cSGMembershipStatus = S1AP_CSGMembershipStatus_member;
  INT32_TO_BIT_STRING(CSG_id, &ie->value.choice.CSGMembershipInfo.cSG_Id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (s1ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    S1AP_ERROR("Failed to encode S1 E-RAB modification indication \n");
    return -1;
  }

  // Non UE-Associated signalling -> stream = 0 
  S1AP_INFO("Size of encoded message: %d \n", len);
  s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                       ue_context_p->mme_ref->assoc_id, buffer,
                                       len, ue_context_p->tx_stream);  

//s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance, ue_context_p->mme_ref->assoc_id, buffer, len, 0);
  return ret;
}*/

int s1ap_eNB_generate_E_RAB_Modification_Indication(
		instance_t instance,
  s1ap_e_rab_modification_ind_t *e_rab_modification_ind)
//-----------------------------------------------------------------------------
{
  struct s1ap_eNB_ue_context_s        *ue_context_p        = NULL;
  S1AP_S1AP_PDU_t            pdu;
  S1AP_E_RABModificationIndication_t     *out = NULL;
  S1AP_E_RABModificationIndicationIEs_t   *ie = NULL;
  S1AP_E_RABToBeModifiedItemBearerModInd_t 	  *E_RAB_ToBeModifiedItem_BearerModInd = NULL;
  S1AP_E_RABToBeModifiedItemBearerModIndIEs_t *E_RAB_ToBeModifiedItem_BearerModInd_IEs = NULL;

  //S1AP_E_RABNotToBeModifiedItemBearerModInd_t 	  *E_RAB_NotToBeModifiedItem_BearerModInd = NULL;
  //S1AP_E_RABNotToBeModifiedItemBearerModIndIEs_t  *E_RAB_NotToBeModifiedItem_BearerModInd_IEs = NULL;


  s1ap_eNB_instance_t          *s1ap_eNB_instance_p = NULL;
  s1ap_eNB_instance_p = s1ap_eNB_get_instance(instance);
  uint8_t  *buffer = NULL;
  uint32_t  len = 0;
  int       ret = 0;
  DevAssert(s1ap_eNB_instance_p != NULL);
  DevAssert(e_rab_modification_ind != NULL);

  int num_e_rabs_tobemodified = e_rab_modification_ind->nb_of_e_rabs_tobemodified;
  //int num_e_rabs_nottobemodified = e_rab_modification_ind->nb_of_e_rabs_nottobemodified;

  //uint32_t CSG_id = 0;
  //uint32_t pseudo_gtp_teid = 10;

  if ((ue_context_p = s1ap_eNB_get_ue_context(s1ap_eNB_instance_p,
		  e_rab_modification_ind->eNB_ue_s1ap_id)) == NULL) {
          // The context for this eNB ue s1ap id doesn't exist in the map of eNB UEs 
          S1AP_WARN("Failed to find ue context associated with eNB ue s1ap id: 0x%06x\n",
        		  e_rab_modification_ind->eNB_ue_s1ap_id);
          return -1;
  }

  // Prepare the S1AP message to encode 
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_E_RABModificationIndication;
  pdu.choice.initiatingMessage.criticality = S1AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_E_RABModificationIndication;
  out = &pdu.choice.initiatingMessage.value.choice.E_RABModificationIndication;
  /* mandatory */
  ie = (S1AP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationIndicationIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_MME_UE_S1AP_ID;
  ie->value.choice.MME_UE_S1AP_ID = e_rab_modification_ind->mme_ue_s1ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ie = (S1AP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationIndicationIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_ENB_UE_S1AP_ID;
  ie->value.choice.ENB_UE_S1AP_ID = e_rab_modification_ind->eNB_ue_s1ap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  //E-RABs to be modified list
  ie = (S1AP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationIndicationIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_E_RABToBeModifiedListBearerModInd;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_E_RABToBeModifiedListBearerModInd;

  //The following two for-loops here will probably need to change. We should do a different type of search
  for(int i=0; i<num_e_rabs_tobemodified; i++){
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs = (S1AP_E_RABToBeModifiedItemBearerModIndIEs_t *)calloc(1,sizeof(S1AP_E_RABToBeModifiedItemBearerModIndIEs_t));
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->id = S1AP_ProtocolIE_ID_id_E_RABToBeModifiedItemBearerModInd;
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->criticality = S1AP_Criticality_reject;
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->value.present = S1AP_E_RABToBeModifiedItemBearerModIndIEs__value_PR_E_RABToBeModifiedItemBearerModInd;
	  E_RAB_ToBeModifiedItem_BearerModInd = &E_RAB_ToBeModifiedItem_BearerModInd_IEs->value.choice.E_RABToBeModifiedItemBearerModInd;

	  {
	  E_RAB_ToBeModifiedItem_BearerModInd->e_RAB_ID = e_rab_modification_ind->e_rabs_tobemodified[i].e_rab_id;

	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size  = e_rab_modification_ind->e_rabs_tobemodified[i].eNB_addr.length/8;
	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.bits_unused = e_rab_modification_ind->e_rabs_tobemodified[i].eNB_addr.length%8;
	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.buf = calloc(1, E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size);
	  memcpy (E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.buf, e_rab_modification_ind->e_rabs_tobemodified[i].eNB_addr.buffer,
			  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size);

	  INT32_TO_OCTET_STRING(e_rab_modification_ind->e_rabs_tobemodified[i].gtp_teid, &E_RAB_ToBeModifiedItem_BearerModInd->dL_GTP_TEID);

	  }
	  ASN_SEQUENCE_ADD(&ie->value.choice.E_RABToBeModifiedListBearerModInd.list, E_RAB_ToBeModifiedItem_BearerModInd_IEs);
  }

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  //E-RABs NOT to be modified list
  /*ie = (S1AP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationIndicationIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_E_RABNotToBeModifiedListBearerModInd;
  ie->criticality = S1AP_Criticality_reject;
  //if(num_e_rabs_nottobemodified > 0) {
	  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_E_RABNotToBeModifiedListBearerModInd;

	  for(int i=0; i<num_e_rabs_tobemodified; i++){
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs = (S1AP_E_RABNotToBeModifiedItemBearerModIndIEs_t *)calloc(1,sizeof(S1AP_E_RABNotToBeModifiedItemBearerModIndIEs_t));
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->id = S1AP_ProtocolIE_ID_id_E_RABNotToBeModifiedItemBearerModInd;
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->criticality = S1AP_Criticality_reject;
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->value.present = S1AP_E_RABNotToBeModifiedItemBearerModIndIEs__value_PR_E_RABNotToBeModifiedItemBearerModInd;
		  E_RAB_NotToBeModifiedItem_BearerModInd = &E_RAB_NotToBeModifiedItem_BearerModInd_IEs->value.choice.E_RABNotToBeModifiedItemBearerModInd;

		  {
			  E_RAB_NotToBeModifiedItem_BearerModInd->e_RAB_ID = 10; //e_rab_modification_ind->e_rabs_tobemodified[i].e_rab_id;

			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size  = e_rab_modification_ind->e_rabs_tobemodified[i].eNB_addr.length/8;
			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.bits_unused = e_rab_modification_ind->e_rabs_tobemodified[i].eNB_addr.length%8;
			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.buf =
	  	    				calloc(1, E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size);
			  memcpy (E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.buf, e_rab_modification_ind->e_rabs_tobemodified[i].eNB_addr.buffer,
					  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size);

			  //INT32_TO_OCTET_STRING(e_rab_modification_ind->e_rabs_tobemodified[i].gtp_teid, &E_RAB_NotToBeModifiedItem_BearerModInd->dL_GTP_TEID);
			    INT32_TO_OCTET_STRING(pseudo_gtp_teid, &E_RAB_NotToBeModifiedItem_BearerModInd->dL_GTP_TEID);

		  }
		  ASN_SEQUENCE_ADD(&ie->value.choice.E_RABNotToBeModifiedListBearerModInd.list, E_RAB_NotToBeModifiedItem_BearerModInd_IEs);
	  }
 // }
  //else{
//	  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_E_RABNotToBeModifiedListBearerModInd;
//	  ie->value.choice.E_RABNotToBeModifiedListBearerModInd.list.size = 0;
//  } / 
  
	   

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);*/

  /*ie = (S1AP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(S1AP_E_RABModificationIndicationIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_CSGMembershipInfo;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_E_RABModificationIndicationIEs__value_PR_CSGMembershipInfo;
  ie->value.choice.CSGMembershipInfo.cSGMembershipStatus = S1AP_CSGMembershipStatus_member;
  INT32_TO_BIT_STRING(CSG_id, &ie->value.choice.CSGMembershipInfo.cSG_Id);
  ie->value.choice.CSGMembershipInfo.cSG_Id.bits_unused=5; 
  ie->value.choice.CSGMembershipInfo.cellAccessMode = S1AP_CellAccessMode_hybrid;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);*/
  
  if (s1ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    S1AP_ERROR("Failed to encode S1 E-RAB modification indication \n");
    return -1;
  }

  // Non UE-Associated signalling -> stream = 0 
  S1AP_INFO("Size of encoded message: %d \n", len);
  s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance,
                                       ue_context_p->mme_ref->assoc_id, buffer,
                                       len, ue_context_p->tx_stream);  

//s1ap_eNB_itti_send_sctp_data_req(s1ap_eNB_instance_p->instance, ue_context_p->mme_ref->assoc_id, buffer, len, 0);
  return ret;
}


