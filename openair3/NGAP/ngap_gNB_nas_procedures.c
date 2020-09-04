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
 * \brief NGAP eNb NAS procedure handler
 * \author  S. Roux and Navid Nikaein
 * \date 2010 - 2015
 * \email: navid.nikaein@eurecom.fr
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
#include "msc.h"

//------------------------------------------------------------------------------
int ngap_gNB_handle_nas_first_req(
    instance_t instance, ngap_nas_first_req_t *ngap_nas_first_req_p)
//------------------------------------------------------------------------------
{
    ngap_gNB_instance_t          *instance_p = NULL;
    struct ngap_gNB_mme_data_s   *mme_desc_p = NULL;
    struct ngap_gNB_ue_context_s *ue_desc_p  = NULL;
    NGAP_NGAP_PDU_t               pdu;
    NGAP_InitialUEMessage_t      *out;
    NGAP_InitialUEMessage_IEs_t  *ie;
    uint8_t  *buffer = NULL;
    uint32_t  length = 0;
    DevAssert(ngap_nas_first_req_p != NULL);
    /* Retrieve the NGAP gNB instance associated with Mod_id */
    instance_p = ngap_gNB_get_instance(instance);
    DevAssert(instance_p != NULL);
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage.procedureCode = NGAP_ProcedureCode_id_initialUEMessage;
    pdu.choice.initiatingMessage.criticality = NGAP_Criticality_ignore;
    pdu.choice.initiatingMessage.value.present = NGAP_InitiatingMessage__value_PR_InitialUEMessage;
    out = &pdu.choice.initiatingMessage.value.choice.InitialUEMessage;

    /* Select the MME corresponding to the provided GUMMEI. */
    if (ngap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_gummei) {
        mme_desc_p = ngap_gNB_nnsf_select_mme_by_gummei(
                         instance_p,
                         ngap_nas_first_req_p->establishment_cause,
                         ngap_nas_first_req_p->ue_identity.gummei);

        if (mme_desc_p) {
            NGAP_INFO("[gNB %d] Chose MME '%s' (assoc_id %d) through GUMMEI MCC %d MNC %d MMEGI %d MMEC %d\n",
                      instance,
                      mme_desc_p->mme_name,
                      mme_desc_p->assoc_id,
                      ngap_nas_first_req_p->ue_identity.gummei.mcc,
                      ngap_nas_first_req_p->ue_identity.gummei.mnc,
                      ngap_nas_first_req_p->ue_identity.gummei.mme_group_id,
                      ngap_nas_first_req_p->ue_identity.gummei.mme_code);
        }
    }

    if (mme_desc_p == NULL) {
        /* Select the MME corresponding to the provided s-TMSI. */
        if (ngap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_s_tmsi) {
            mme_desc_p = ngap_gNB_nnsf_select_mme_by_mme_code(
                             instance_p,
                             ngap_nas_first_req_p->establishment_cause,
                             ngap_nas_first_req_p->selected_plmn_identity,
                             ngap_nas_first_req_p->ue_identity.s_tmsi.mme_code);

            if (mme_desc_p) {
                NGAP_INFO("[gNB %d] Chose MME '%s' (assoc_id %d) through S-TMSI MMEC %d and selected PLMN Identity index %d MCC %d MNC %d\n",
                          instance,
                          mme_desc_p->mme_name,
                          mme_desc_p->assoc_id,
                          ngap_nas_first_req_p->ue_identity.s_tmsi.mme_code,
                          ngap_nas_first_req_p->selected_plmn_identity,
                          instance_p->mcc[ngap_nas_first_req_p->selected_plmn_identity],
                          instance_p->mnc[ngap_nas_first_req_p->selected_plmn_identity]);
            }
        }
    }

    if (mme_desc_p == NULL) {
        /* Select MME based on the selected PLMN identity, received through RRC
         * Connection Setup Complete */
        mme_desc_p = ngap_gNB_nnsf_select_mme_by_plmn_id(
                         instance_p,
                         ngap_nas_first_req_p->establishment_cause,
                         ngap_nas_first_req_p->selected_plmn_identity);

        if (mme_desc_p) {
            NGAP_INFO("[gNB %d] Chose MME '%s' (assoc_id %d) through selected PLMN Identity index %d MCC %d MNC %d\n",
                      instance,
                      mme_desc_p->mme_name,
                      mme_desc_p->assoc_id,
                      ngap_nas_first_req_p->selected_plmn_identity,
                      instance_p->mcc[ngap_nas_first_req_p->selected_plmn_identity],
                      instance_p->mnc[ngap_nas_first_req_p->selected_plmn_identity]);
        }
    }

    if (mme_desc_p == NULL) {
        /*
         * If no MME corresponds to the GUMMEI, the s-TMSI, or the selected PLMN
         * identity, selects the MME with the highest capacity.
         */
        mme_desc_p = ngap_gNB_nnsf_select_mme(
                         instance_p,
                         ngap_nas_first_req_p->establishment_cause);

        if (mme_desc_p) {
            NGAP_INFO("[gNB %d] Chose MME '%s' (assoc_id %d) through highest relative capacity\n",
                      instance,
                      mme_desc_p->mme_name,
                      mme_desc_p->assoc_id);
        }
    }

    if (mme_desc_p == NULL) {
        /*
         * In case gNB has no MME associated, the gNB should inform RRC and discard
         * this request.
         */
        NGAP_WARN("No MME is associated to the gNB\n");
        // TODO: Inform RRC
        return -1;
    }

    /* The gNB should allocate a unique gNB UE NGAP ID for this UE. The value
     * will be used for the duration of the connectivity.
     */
    ue_desc_p = ngap_gNB_allocate_new_UE_context();
    DevAssert(ue_desc_p != NULL);
    /* Keep a reference to the selected MME */
    ue_desc_p->mme_ref       = mme_desc_p;
    ue_desc_p->ue_initial_id = ngap_nas_first_req_p->ue_initial_id;
    ue_desc_p->gNB_instance  = instance_p;
    ue_desc_p->selected_plmn_identity = ngap_nas_first_req_p->selected_plmn_identity;

    do {
        struct ngap_gNB_ue_context_s *collision_p;
        /* Peek a random value for the gNB_ue_ngap_id */
        ue_desc_p->gNB_ue_ngap_id = (random() + random()) & 0x00ffffff;

        if ((collision_p = RB_INSERT(ngap_ue_map, &instance_p->ngap_ue_head, ue_desc_p))
                == NULL) {
            NGAP_DEBUG("Found usable gNB_ue_ngap_id: 0x%06x %u(10)\n",
                       ue_desc_p->gNB_ue_ngap_id,
                       ue_desc_p->gNB_ue_ngap_id);
            /* Break the loop as the id is not already used by another UE */
            break;
        }
    } while(1);

    /* mandatory */
    ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_GNB_UE_NGAP_ID;
    ie->value.choice.GNB_UE_NGAP_ID = ue_desc_p->gNB_ue_ngap_id;
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
    ie->id = NGAP_ProtocolIE_ID_id_TAI;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_TAI;
    /* Assuming TAI is the TAI from the cell */
    INT16_TO_OCTET_STRING(instance_p->tac, &ie->value.choice.TAI.tAC);
    MCC_MNC_TO_PLMNID(instance_p->mcc[ue_desc_p->selected_plmn_identity],
                      instance_p->mnc[ue_desc_p->selected_plmn_identity],
                      instance_p->mnc_digit_length[ue_desc_p->selected_plmn_identity],
                      &ie->value.choice.TAI.pLMNidentity);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_EUTRAN_CGI;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_EUTRAN_CGI;
    /* Set the EUTRAN CGI
     * The cell identity is defined on 28 bits but as we use macro enb id,
     * we have to pad.
     */
    //#warning "TODO get cell id from RRC"
    MACRO_GNB_ID_TO_CELL_IDENTITY(instance_p->gNB_id,
                                  0, // Cell ID
                                  &ie->value.choice.EUTRAN_CGI.cell_ID);
    MCC_MNC_TO_TBCD(instance_p->mcc[ue_desc_p->selected_plmn_identity],
                    instance_p->mnc[ue_desc_p->selected_plmn_identity],
                    instance_p->mnc_digit_length[ue_desc_p->selected_plmn_identity],
                    &ie->value.choice.EUTRAN_CGI.pLMNidentity);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* Set the establishment cause according to those provided by RRC */
    DevCheck(ngap_nas_first_req_p->establishment_cause < RRC_CAUSE_LAST,
             ngap_nas_first_req_p->establishment_cause, RRC_CAUSE_LAST, 0);
    /* mandatory */
    ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_RRC_Establishment_Cause;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_RRC_Establishment_Cause;
    ie->value.choice.RRC_Establishment_Cause = ngap_nas_first_req_p->establishment_cause;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* optional */
    if (ngap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_s_tmsi) {
        NGAP_DEBUG("S_TMSI_PRESENT\n");
        ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_S_TMSI;
        ie->criticality = NGAP_Criticality_reject;
        ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_S_TMSI;
        MME_CODE_TO_OCTET_STRING(ngap_nas_first_req_p->ue_identity.s_tmsi.mme_code,
                                 &ie->value.choice.S_TMSI.mMEC);
        M_TMSI_TO_OCTET_STRING(ngap_nas_first_req_p->ue_identity.s_tmsi.m_tmsi,
                               &ie->value.choice.S_TMSI.m_TMSI);
        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }


    /* optional */
    if (ngap_nas_first_req_p->ue_identity.presenceMask & UE_IDENTITIES_gummei) {
        NGAP_DEBUG("GUMMEI_ID_PRESENT\n");
        ie = (NGAP_InitialUEMessage_IEs_t *)calloc(1, sizeof(NGAP_InitialUEMessage_IEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_GUMMEI_ID;
        ie->criticality = NGAP_Criticality_reject;
        ie->value.present = NGAP_InitialUEMessage_IEs__value_PR_GUMMEI;
        MCC_MNC_TO_PLMNID(
            ngap_nas_first_req_p->ue_identity.gummei.mcc,
            ngap_nas_first_req_p->ue_identity.gummei.mnc,
            ngap_nas_first_req_p->ue_identity.gummei.mnc_len,
            &ie->value.choice.GUMMEI.pLMN_Identity);
        MME_GID_TO_OCTET_STRING(ngap_nas_first_req_p->ue_identity.gummei.mme_group_id,
                                &ie->value.choice.GUMMEI.mME_Group_ID);
        MME_CODE_TO_OCTET_STRING(ngap_nas_first_req_p->ue_identity.gummei.mme_code,
                                 &ie->value.choice.GUMMEI.mME_Code);
        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }



    if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        /* Failed to encode message */
        DevMessage("Failed to encode initial UE message\n");
    }

    /* Update the current NGAP UE state */
    ue_desc_p->ue_state = NGAP_UE_WAITING_CSR;
    /* Assign a stream for this UE :
     * From 3GPP 36.412 7)Transport layers:
     *  Within the SCTP association established between one MME and gNB pair:
     *  - a single pair of stream identifiers shall be reserved for the sole use
     *      of NGAP elementary procedures that utilize non UE-associated signalling.
     *  - At least one pair of stream identifiers shall be reserved for the sole use
     *      of NGAP elementary procedures that utilize UE-associated signallings.
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
        MSC_NGAP_GNB,
        MSC_NGAP_MME,
        (const char *)NULL,
        0,
        MSC_AS_TIME_FMT" initialUEMessage initiatingMessage gNB_ue_ngap_id %u",
        0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
        ue_desc_p->gNB_ue_ngap_id);
    /* Send encoded message over sctp */
    ngap_gNB_itti_send_sctp_data_req(instance_p->instance, mme_desc_p->assoc_id,
                                     buffer, length, ue_desc_p->tx_stream);
    return 0;
}

//------------------------------------------------------------------------------
int ngap_gNB_handle_nas_downlink(uint32_t         assoc_id,
                                 uint32_t         stream,
                                 NGAP_NGAP_PDU_t *pdu)
//------------------------------------------------------------------------------
{
    ngap_gNB_mme_data_t             *mme_desc_p        = NULL;
    ngap_gNB_ue_context_t           *ue_desc_p         = NULL;
    ngap_gNB_instance_t             *ngap_gNB_instance = NULL;
    NGAP_DownlinkNASTransport_t     *container;
    NGAP_DownlinkNASTransport_IEs_t *ie;
    NGAP_GNB_UE_NGAP_ID_t            enb_ue_ngap_id;
    NGAP_MME_UE_NGAP_ID_t            mme_ue_ngap_id;
    DevAssert(pdu != NULL);

    /* UE-related procedure -> stream != 0 */
    if (stream == 0) {
        NGAP_ERROR("[SCTP %d] Received UE-related procedure on stream == 0\n",
                   assoc_id);
        return -1;
    }

    if ((mme_desc_p = ngap_gNB_get_MME(NULL, assoc_id, 0)) == NULL) {
        NGAP_ERROR(
            "[SCTP %d] Received NAS downlink message for non existing MME context\n",
            assoc_id);
        return -1;
    }

    ngap_gNB_instance = mme_desc_p->ngap_gNB_instance;
    /* Prepare the NGAP message to encode */
    container = &pdu->choice.initiatingMessage.value.choice.DownlinkNASTransport;
    NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_DownlinkNASTransport_IEs_t, ie, container,
                               NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID, true);
    mme_ue_ngap_id = ie->value.choice.MME_UE_NGAP_ID;

    NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_DownlinkNASTransport_IEs_t, ie, container,
                               NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID, true);
    enb_ue_ngap_id = ie->value.choice.GNB_UE_NGAP_ID;

    if ((ue_desc_p = ngap_gNB_get_ue_context(ngap_gNB_instance,
                     enb_ue_ngap_id)) == NULL) {
        MSC_LOG_RX_DISCARDED_MESSAGE(
            MSC_NGAP_GNB,
            MSC_NGAP_MME,
            NULL,
            0,
            MSC_AS_TIME_FMT" downlinkNASTransport  gNB_ue_ngap_id %u mme_ue_ngap_id %u",
            enb_ue_ngap_id,
            mme_ue_ngap_id);
        NGAP_ERROR("[SCTP %d] Received NAS downlink message for non existing UE context gNB_UE_NGAP_ID: 0x%lx\n",
                   assoc_id,
                   enb_ue_ngap_id);
        return -1;
    }

    if (0 == ue_desc_p->rx_stream) {
        ue_desc_p->rx_stream = stream;
    } else if (stream != ue_desc_p->rx_stream) {
        NGAP_ERROR("[SCTP %d] Received UE-related procedure on stream %u, expecting %u\n",
                   assoc_id, stream, ue_desc_p->rx_stream);
        return -1;
    }

    /* Is it the first outcome of the MME for this UE ? If so store the mme
     * UE ngap id.
     */
    if (ue_desc_p->mme_ue_ngap_id == 0) {
        ue_desc_p->mme_ue_ngap_id = mme_ue_ngap_id;
    } else {
        /* We already have a mme ue ngap id check the received is the same */
        if (ue_desc_p->mme_ue_ngap_id != mme_ue_ngap_id) {
            NGAP_ERROR("[SCTP %d] Mismatch in MME UE NGAP ID (0x%lx != 0x%"PRIx32"\n",
                       assoc_id,
                       mme_ue_ngap_id,
                       ue_desc_p->mme_ue_ngap_id
                      );
            return -1;
        }
    }

    MSC_LOG_RX_MESSAGE(
        MSC_NGAP_GNB,
        MSC_NGAP_MME,
        NULL,
        0,
        MSC_AS_TIME_FMT" downlinkNASTransport  gNB_ue_ngap_id %u mme_ue_ngap_id %u",
        assoc_id,
        mme_ue_ngap_id);

    NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_DownlinkNASTransport_IEs_t, ie, container,
                               NGAP_ProtocolIE_ID_id_NAS_PDU, true);
    /* Forward the NAS PDU to RRC */
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
    uint8_t  *buffer;
    uint32_t  length;
    DevAssert(ngap_uplink_nas_p != NULL);
    /* Retrieve the NGAP gNB instance associated with Mod_id */
    ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
    DevAssert(ngap_gNB_instance_p != NULL);

    if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p, ngap_uplink_nas_p->gNB_ue_ngap_id)) == NULL) {
        /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
        NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: %06x\n",
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
    pdu.choice.initiatingMessage.procedureCode = NGAP_ProcedureCode_id_uplinkNASTransport;
    pdu.choice.initiatingMessage.criticality = NGAP_Criticality_ignore;
    pdu.choice.initiatingMessage.value.present = NGAP_InitiatingMessage__value_PR_UplinkNASTransport;
    out = &pdu.choice.initiatingMessage.value.choice.UplinkNASTransport;
    /* mandatory */
    ie = (NGAP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(NGAP_UplinkNASTransport_IEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_UplinkNASTransport_IEs__value_PR_MME_UE_NGAP_ID;
    ie->value.choice.MME_UE_NGAP_ID = ue_context_p->mme_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(NGAP_UplinkNASTransport_IEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_UplinkNASTransport_IEs__value_PR_GNB_UE_NGAP_ID;
    ie->value.choice.GNB_UE_NGAP_ID = ue_context_p->gNB_ue_ngap_id;
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
    ie->id = NGAP_ProtocolIE_ID_id_EUTRAN_CGI;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_UplinkNASTransport_IEs__value_PR_EUTRAN_CGI;
    MCC_MNC_TO_PLMNID(
        ngap_gNB_instance_p->mcc[ue_context_p->selected_plmn_identity],
        ngap_gNB_instance_p->mnc[ue_context_p->selected_plmn_identity],
        ngap_gNB_instance_p->mnc_digit_length[ue_context_p->selected_plmn_identity],
        &ie->value.choice.EUTRAN_CGI.pLMNidentity);
    //#warning "TODO get cell id from RRC"
    MACRO_GNB_ID_TO_CELL_IDENTITY(ngap_gNB_instance_p->gNB_id,
                                  0,
                                  &ie->value.choice.EUTRAN_CGI.cell_ID);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_UplinkNASTransport_IEs_t *)calloc(1, sizeof(NGAP_UplinkNASTransport_IEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_TAI;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_UplinkNASTransport_IEs__value_PR_TAI;
    MCC_MNC_TO_PLMNID(
        ngap_gNB_instance_p->mcc[ue_context_p->selected_plmn_identity],
        ngap_gNB_instance_p->mnc[ue_context_p->selected_plmn_identity],
        ngap_gNB_instance_p->mnc_digit_length[ue_context_p->selected_plmn_identity],
        &ie->value.choice.TAI.pLMNidentity);
    TAC_TO_ASN1(ngap_gNB_instance_p->tac, &ie->value.choice.TAI.tAC);
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* optional */


    if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        NGAP_ERROR("Failed to encode uplink NAS transport\n");
        /* Encode procedure has failed... */
        return -1;
    }

    MSC_LOG_TX_MESSAGE(
        MSC_NGAP_GNB,
        MSC_NGAP_MME,
        (const char *)NULL,
        0,
        MSC_AS_TIME_FMT" uplinkNASTransport initiatingMessage gNB_ue_ngap_id %u mme_ue_ngap_id %u",
        0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->gNB_ue_ngap_id,
        ue_context_p->mme_ue_ngap_id);
    /* UE associated signalling -> use the allocated stream */
    ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
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
        NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: %06x\n",
                  ngap_nas_non_delivery_ind->gNB_ue_ngap_id);
        MSC_LOG_EVENT(
            MSC_NGAP_GNB,
            MSC_AS_TIME_FMT" Sent of NAS_NON_DELIVERY_IND to MME failed, no context for gNB_ue_ngap_id %06x",
            ngap_nas_non_delivery_ind->gNB_ue_ngap_id);
        return -1;
    }

    /* Prepare the NGAP message to encode */
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
    pdu.choice.initiatingMessage.procedureCode = NGAP_ProcedureCode_id_NASNonDeliveryIndication;
    pdu.choice.initiatingMessage.criticality = NGAP_Criticality_ignore;
    pdu.choice.initiatingMessage.value.present = NGAP_InitiatingMessage__value_PR_NASNonDeliveryIndication;
    out = &pdu.choice.initiatingMessage.value.choice.NASNonDeliveryIndication;
    /* mandatory */
    ie = (NGAP_NASNonDeliveryIndication_IEs_t *)calloc(1, sizeof(NGAP_NASNonDeliveryIndication_IEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_NASNonDeliveryIndication_IEs__value_PR_MME_UE_NGAP_ID;
    ie->value.choice.MME_UE_NGAP_ID = ue_context_p->mme_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_NASNonDeliveryIndication_IEs_t *)calloc(1, sizeof(NGAP_NASNonDeliveryIndication_IEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_NASNonDeliveryIndication_IEs__value_PR_GNB_UE_NGAP_ID;
    ie->value.choice.GNB_UE_NGAP_ID = ue_context_p->gNB_ue_ngap_id;
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
        MSC_LOG_EVENT(
            MSC_NGAP_GNB,
            MSC_AS_TIME_FMT" Sent of NAS_NON_DELIVERY_IND to MME failed (encoding)");
        return -1;
    }

    MSC_LOG_TX_MESSAGE(
        MSC_NGAP_GNB,
        MSC_NGAP_MME,
        (const char *)buffer,
        length,
        MSC_AS_TIME_FMT" NASNonDeliveryIndication initiatingMessage gNB_ue_ngap_id %u mme_ue_ngap_id %u",
        0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
        ue_context_p->gNB_ue_ngap_id,
        ue_context_p->mme_ue_ngap_id);
    /* UE associated signalling -> use the allocated stream */
    ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
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
    uint32_t length;
    int      i;
    /* Retrieve the NGAP gNB instance associated with Mod_id */
    ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
    DevAssert(initial_ctxt_resp_p != NULL);
    DevAssert(ngap_gNB_instance_p != NULL);

    if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                        initial_ctxt_resp_p->gNB_ue_ngap_id)) == NULL) {
        /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
        NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%06x\n",
                  initial_ctxt_resp_p->gNB_ue_ngap_id);
        return -1;
    }

    /* Uplink NAS transport can occur either during an ngap connected state
     * or during initial attach (for example: NAS authentication).
     */
    if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED ||
            ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
        NGAP_WARN("You are attempting to send NAS data over non-connected "
                  "gNB ue ngap id: %06x, current state: %d\n",
                  initial_ctxt_resp_p->gNB_ue_ngap_id, ue_context_p->ue_state);
        return -1;
    }

    /* Prepare the NGAP message to encode */
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome.procedureCode = NGAP_ProcedureCode_id_InitialContextSetup;
    pdu.choice.successfulOutcome.criticality = NGAP_Criticality_reject;
    pdu.choice.successfulOutcome.value.present = NGAP_SuccessfulOutcome__value_PR_InitialContextSetupResponse;
    out = &pdu.choice.successfulOutcome.value.choice.InitialContextSetupResponse;
    /* mandatory */
    ie = (NGAP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(NGAP_InitialContextSetupResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_InitialContextSetupResponseIEs__value_PR_MME_UE_NGAP_ID;
    ie->value.choice.MME_UE_NGAP_ID = ue_context_p->mme_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(NGAP_InitialContextSetupResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_InitialContextSetupResponseIEs__value_PR_GNB_UE_NGAP_ID;
    ie->value.choice.GNB_UE_NGAP_ID = initial_ctxt_resp_p->gNB_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(NGAP_InitialContextSetupResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_E_RABSetupListCtxtSURes;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_InitialContextSetupResponseIEs__value_PR_E_RABSetupListCtxtSURes;

    for (i = 0; i < initial_ctxt_resp_p->nb_of_e_rabs; i++) {
        NGAP_E_RABSetupItemCtxtSUResIEs_t *item;
        /* mandatory */
        item = (NGAP_E_RABSetupItemCtxtSUResIEs_t *)calloc(1, sizeof(NGAP_E_RABSetupItemCtxtSUResIEs_t));
        item->id = NGAP_ProtocolIE_ID_id_E_RABSetupItemCtxtSURes;
        item->criticality = NGAP_Criticality_ignore;
        item->value.present = NGAP_E_RABSetupItemCtxtSUResIEs__value_PR_E_RABSetupItemCtxtSURes;
        item->value.choice.E_RABSetupItemCtxtSURes.e_RAB_ID = initial_ctxt_resp_p->e_rabs[i].e_rab_id;
        GTP_TEID_TO_ASN1(initial_ctxt_resp_p->e_rabs[i].gtp_teid, &item->value.choice.E_RABSetupItemCtxtSURes.gTP_TEID);
        item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.buf = malloc(initial_ctxt_resp_p->e_rabs[i].gNB_addr.length);
        memcpy(item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.buf,
               initial_ctxt_resp_p->e_rabs[i].gNB_addr.buffer,
               initial_ctxt_resp_p->e_rabs[i].gNB_addr.length);
        item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.size = initial_ctxt_resp_p->e_rabs[i].gNB_addr.length;
        item->value.choice.E_RABSetupItemCtxtSURes.transportLayerAddress.bits_unused = 0;
        NGAP_DEBUG("initial_ctxt_resp_p: e_rab ID %ld, enb_addr %d.%d.%d.%d, SIZE %ld \n",
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
        ie = (NGAP_InitialContextSetupResponseIEs_t *)calloc(1, sizeof(NGAP_InitialContextSetupResponseIEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_E_RABFailedToSetupListCtxtSURes;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_InitialContextSetupResponseIEs__value_PR_E_RABList;

        for (i = 0; i < initial_ctxt_resp_p->nb_of_e_rabs_failed; i++) {
            NGAP_E_RABItemIEs_t *item;
            /* mandatory */
            item = (NGAP_E_RABItemIEs_t *)calloc(1, sizeof(NGAP_E_RABItemIEs_t));
            item->id = NGAP_ProtocolIE_ID_id_E_RABItem;
            item->criticality = NGAP_Criticality_ignore;
            item->value.present = NGAP_E_RABItemIEs__value_PR_E_RABItem;
            item->value.choice.E_RABItem.e_RAB_ID = initial_ctxt_resp_p->e_rabs_failed[i].e_rab_id;
            item->value.choice.E_RABItem.cause.present = initial_ctxt_resp_p->e_rabs_failed[i].cause;

            switch(item->value.choice.E_RABItem.cause.present) {
            case NGAP_Cause_PR_radioNetwork:
                item->value.choice.E_RABItem.cause.choice.radioNetwork = initial_ctxt_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_transport:
                item->value.choice.E_RABItem.cause.choice.transport = initial_ctxt_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_nas:
                item->value.choice.E_RABItem.cause.choice.nas = initial_ctxt_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_protocol:
                item->value.choice.E_RABItem.cause.choice.protocol = initial_ctxt_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_misc:
                item->value.choice.E_RABItem.cause.choice.misc = initial_ctxt_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_NOTHING:
            default:
                break;
            }

            NGAP_DEBUG("initial context setup response: failed e_rab ID %ld\n", item->value.choice.E_RABItem.e_RAB_ID);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABList.list, item);
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

    if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        NGAP_ERROR("Failed to encode uplink NAS transport\n");
        /* Encode procedure has failed... */
        return -1;
    }

    MSC_LOG_TX_MESSAGE(
        MSC_NGAP_GNB,
        MSC_NGAP_MME,
        (const char *)buffer,
        length,
        MSC_AS_TIME_FMT" InitialContextSetup successfulOutcome gNB_ue_ngap_id %u mme_ue_ngap_id %u",
        0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
        initial_ctxt_resp_p->gNB_ue_ngap_id,
        ue_context_p->mme_ue_ngap_id);
    /* UE associated signalling -> use the allocated stream */
    ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
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
    NGAP_UECapabilityInfoIndication_t    *out;
    NGAP_UECapabilityInfoIndicationIEs_t *ie;
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

    /* UE capabilities message can occur either during an ngap connected state
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
    pdu.choice.initiatingMessage.procedureCode = NGAP_ProcedureCode_id_UECapabilityInfoIndication;
    pdu.choice.initiatingMessage.criticality = NGAP_Criticality_ignore;
    pdu.choice.initiatingMessage.value.present = NGAP_InitiatingMessage__value_PR_UECapabilityInfoIndication;
    out = &pdu.choice.initiatingMessage.value.choice.UECapabilityInfoIndication;
    /* mandatory */
    ie = (NGAP_UECapabilityInfoIndicationIEs_t *)calloc(1, sizeof(NGAP_UECapabilityInfoIndicationIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_UECapabilityInfoIndicationIEs__value_PR_MME_UE_NGAP_ID;
    ie->value.choice.MME_UE_NGAP_ID = ue_context_p->mme_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_UECapabilityInfoIndicationIEs_t *)calloc(1, sizeof(NGAP_UECapabilityInfoIndicationIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_UECapabilityInfoIndicationIEs__value_PR_GNB_UE_NGAP_ID;
    ie->value.choice.GNB_UE_NGAP_ID = ue_cap_info_ind_p->gNB_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_UECapabilityInfoIndicationIEs_t *)calloc(1, sizeof(NGAP_UECapabilityInfoIndicationIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_UERadioCapability;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_UECapabilityInfoIndicationIEs__value_PR_UERadioCapability;
    ie->value.choice.UERadioCapability.buf = ue_cap_info_ind_p->ue_radio_cap.buffer;
    ie->value.choice.UERadioCapability.size = ue_cap_info_ind_p->ue_radio_cap.length;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* optional */

    if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        /* Encode procedure has failed... */
        NGAP_ERROR("Failed to encode UE capabilities indication\n");
        return -1;
    }

    MSC_LOG_TX_MESSAGE(
        MSC_NGAP_GNB,
        MSC_NGAP_MME,
        (const char *)buffer,
        length,
        MSC_AS_TIME_FMT" UECapabilityInfoIndication initiatingMessage gNB_ue_ngap_id %u mme_ue_ngap_id %u",
        0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
        ue_cap_info_ind_p->gNB_ue_ngap_id,
        ue_context_p->mme_ue_ngap_id);
    /* UE associated signalling -> use the allocated stream */
    ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);
    return 0;
}

//------------------------------------------------------------------------------
int ngap_gNB_e_rab_setup_resp(instance_t instance,
                              ngap_e_rab_setup_resp_t *e_rab_setup_resp_p)
//------------------------------------------------------------------------------
{
    ngap_gNB_instance_t          *ngap_gNB_instance_p = NULL;
    struct ngap_gNB_ue_context_s *ue_context_p        = NULL;
    NGAP_NGAP_PDU_t               pdu;
    NGAP_E_RABSetupResponse_t    *out;
    NGAP_E_RABSetupResponseIEs_t *ie;
    uint8_t  *buffer  = NULL;
    uint32_t length;
    int      i;
    /* Retrieve the NGAP gNB instance associated with Mod_id */
    ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
    DevAssert(e_rab_setup_resp_p != NULL);
    DevAssert(ngap_gNB_instance_p != NULL);

    if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                        e_rab_setup_resp_p->gNB_ue_ngap_id)) == NULL) {
        /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
        NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%06x\n",
                  e_rab_setup_resp_p->gNB_ue_ngap_id);
        return -1;
    }

    /* Uplink NAS transport can occur either during an ngap connected state
     * or during initial attach (for example: NAS authentication).
     */
    if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED ||
            ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
        NGAP_WARN("You are attempting to send NAS data over non-connected "
                  "gNB ue ngap id: %06x, current state: %d\n",
                  e_rab_setup_resp_p->gNB_ue_ngap_id, ue_context_p->ue_state);
        return -1;
    }

    /* Prepare the NGAP message to encode */
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome.procedureCode = NGAP_ProcedureCode_id_E_RABModify;
    pdu.choice.successfulOutcome.criticality = NGAP_Criticality_reject;
    pdu.choice.successfulOutcome.value.present = NGAP_SuccessfulOutcome__value_PR_E_RABSetupResponse;
    out = &pdu.choice.successfulOutcome.value.choice.E_RABSetupResponse;
    /* mandatory */
    ie = (NGAP_E_RABSetupResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABSetupResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_E_RABSetupResponseIEs__value_PR_MME_UE_NGAP_ID;
    ie->value.choice.MME_UE_NGAP_ID = ue_context_p->mme_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_E_RABSetupResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABSetupResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_E_RABSetupResponseIEs__value_PR_GNB_UE_NGAP_ID;
    ie->value.choice.GNB_UE_NGAP_ID = e_rab_setup_resp_p->gNB_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* optional */
    if (e_rab_setup_resp_p->nb_of_e_rabs > 0) {
        ie = (NGAP_E_RABSetupResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABSetupResponseIEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_E_RABSetupListBearerSURes;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_E_RABSetupResponseIEs__value_PR_E_RABSetupListBearerSURes;

        for (i = 0; i < e_rab_setup_resp_p->nb_of_e_rabs; i++) {
            NGAP_E_RABSetupItemBearerSUResIEs_t *item;
            /* mandatory */
            item = (NGAP_E_RABSetupItemBearerSUResIEs_t *)calloc(1, sizeof(NGAP_E_RABSetupItemBearerSUResIEs_t));
            item->id = NGAP_ProtocolIE_ID_id_E_RABSetupItemBearerSURes;
            item->criticality = NGAP_Criticality_ignore;
            item->value.present = NGAP_E_RABSetupItemBearerSUResIEs__value_PR_E_RABSetupItemBearerSURes;
            item->value.choice.E_RABSetupItemBearerSURes.e_RAB_ID = e_rab_setup_resp_p->e_rabs[i].e_rab_id;
            GTP_TEID_TO_ASN1(e_rab_setup_resp_p->e_rabs[i].gtp_teid, &item->value.choice.E_RABSetupItemBearerSURes.gTP_TEID);
            item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.buf = malloc(e_rab_setup_resp_p->e_rabs[i].gNB_addr.length);
            memcpy(item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.buf,
                   e_rab_setup_resp_p->e_rabs[i].gNB_addr.buffer,
                   e_rab_setup_resp_p->e_rabs[i].gNB_addr.length);
            item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.size = e_rab_setup_resp_p->e_rabs[i].gNB_addr.length;
            item->value.choice.E_RABSetupItemBearerSURes.transportLayerAddress.bits_unused = 0;
            NGAP_DEBUG("e_rab_setup_resp: e_rab ID %ld, teid %u, enb_addr %d.%d.%d.%d, SIZE %ld\n",
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
        ie = (NGAP_E_RABSetupResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABSetupResponseIEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_E_RABFailedToSetupListBearerSURes;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_E_RABSetupResponseIEs__value_PR_E_RABList;

        for (i = 0; i < e_rab_setup_resp_p->nb_of_e_rabs_failed; i++) {
            NGAP_E_RABItemIEs_t *item;
            item = (NGAP_E_RABItemIEs_t *)calloc(1, sizeof(NGAP_E_RABItemIEs_t));
            item->id = NGAP_ProtocolIE_ID_id_E_RABItem;
            item->criticality = NGAP_Criticality_ignore;
            item->value.present = NGAP_E_RABItemIEs__value_PR_E_RABItem;
            item->value.choice.E_RABItem.e_RAB_ID = e_rab_setup_resp_p->e_rabs_failed[i].e_rab_id;
            item->value.choice.E_RABItem.cause.present = e_rab_setup_resp_p->e_rabs_failed[i].cause;

            switch(item->value.choice.E_RABItem.cause.present) {
            case NGAP_Cause_PR_radioNetwork:
                item->value.choice.E_RABItem.cause.choice.radioNetwork = e_rab_setup_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_transport:
                item->value.choice.E_RABItem.cause.choice.transport = e_rab_setup_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_nas:
                item->value.choice.E_RABItem.cause.choice.nas = e_rab_setup_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_protocol:
                item->value.choice.E_RABItem.cause.choice.protocol = e_rab_setup_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_misc:
                item->value.choice.E_RABItem.cause.choice.misc = e_rab_setup_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_NOTHING:
            default:
                break;
            }

            NGAP_DEBUG("e_rab_modify_resp: failed e_rab ID %ld\n", item->value.choice.E_RABItem.e_RAB_ID);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABList.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* optional */
    if (0) {
        ie = (NGAP_E_RABSetupResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABSetupResponseIEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_CriticalityDiagnostics;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_E_RABSetupResponseIEs__value_PR_CriticalityDiagnostics;
        // ie->value.choice.CriticalityDiagnostics = ;
        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* NGAP_E_RABSetupListBearerSURes_t  e_RABSetupListBearerSURes;
    memset(&e_RABSetupListBearerSURes, 0, sizeof(NGAP_E_RABSetupListBearerSURes_t));
    if (ngap_encode_ngap_e_rabsetuplistbearersures(&e_RABSetupListBearerSURes, &initial_ies_p->e_RABSetupListBearerSURes.ngap_E_RABSetupItemBearerSURes) < 0 )
      return -1;
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_E_RABSetupListBearerSURes, &e_RABSetupListBearerSURes);
    */
    fprintf(stderr, "start encode\n");

    if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        NGAP_ERROR("Failed to encode uplink transport\n");
        /* Encode procedure has failed... */
        return -1;
    }

    MSC_LOG_TX_MESSAGE(
        MSC_NGAP_GNB,
        MSC_NGAP_MME,
        (const char *)buffer,
        length,
        MSC_AS_TIME_FMT" E_RAN Setup successfulOutcome gNB_ue_ngap_id %u mme_ue_ngap_id %u",
        0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
        e_rab_setup_resp_p->gNB_ue_ngap_id,
        ue_context_p->mme_ue_ngap_id);
    /* UE associated signalling -> use the allocated stream */
    ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);
    return 0;
}

//------------------------------------------------------------------------------
int ngap_gNB_e_rab_modify_resp(instance_t instance,
                               ngap_e_rab_modify_resp_t *e_rab_modify_resp_p)
//------------------------------------------------------------------------------
{
    ngap_gNB_instance_t           *ngap_gNB_instance_p = NULL;
    struct ngap_gNB_ue_context_s  *ue_context_p        = NULL;
    NGAP_NGAP_PDU_t                pdu;
    NGAP_E_RABModifyResponse_t    *out;
    NGAP_E_RABModifyResponseIEs_t *ie;
    uint8_t  *buffer  = NULL;
    uint32_t length;
    int      i;
    /* Retrieve the NGAP gNB instance associated with Mod_id */
    ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
    DevAssert(e_rab_modify_resp_p != NULL);
    DevAssert(ngap_gNB_instance_p != NULL);

    if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                        e_rab_modify_resp_p->gNB_ue_ngap_id)) == NULL) {
        /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
        NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%06x\n",
                  e_rab_modify_resp_p->gNB_ue_ngap_id);
        return -1;
    }

    /* Uplink NAS transport can occur either during an ngap connected state
     * or during initial attach (for example: NAS authentication).
     */
    if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED ||
            ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
        NGAP_WARN("You are attempting to send NAS data over non-connected "
                  "gNB ue ngap id: %06x, current state: %d\n",
                  e_rab_modify_resp_p->gNB_ue_ngap_id, ue_context_p->ue_state);
        return -1;
    }

    /* Prepare the NGAP message to encode */
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome.procedureCode = NGAP_ProcedureCode_id_E_RABModify;
    pdu.choice.successfulOutcome.criticality = NGAP_Criticality_reject;
    pdu.choice.successfulOutcome.value.present = NGAP_SuccessfulOutcome__value_PR_E_RABModifyResponse;
    out = &pdu.choice.successfulOutcome.value.choice.E_RABModifyResponse;
    /* mandatory */
    ie = (NGAP_E_RABModifyResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABModifyResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_E_RABModifyResponseIEs__value_PR_MME_UE_NGAP_ID;
    ie->value.choice.MME_UE_NGAP_ID = ue_context_p->mme_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_E_RABModifyResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABModifyResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_E_RABModifyResponseIEs__value_PR_GNB_UE_NGAP_ID;
    ie->value.choice.GNB_UE_NGAP_ID = e_rab_modify_resp_p->gNB_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* optional */
    if (e_rab_modify_resp_p->nb_of_e_rabs > 0) {
        ie = (NGAP_E_RABModifyResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABModifyResponseIEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_E_RABModifyListBearerModRes;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_E_RABModifyResponseIEs__value_PR_E_RABModifyListBearerModRes;

        for (i = 0; i < e_rab_modify_resp_p->nb_of_e_rabs; i++) {
            NGAP_E_RABModifyItemBearerModResIEs_t *item;
            item = (NGAP_E_RABModifyItemBearerModResIEs_t *)calloc(1, sizeof(NGAP_E_RABModifyItemBearerModResIEs_t));
            item->id = NGAP_ProtocolIE_ID_id_E_RABModifyItemBearerModRes;
            item->criticality = NGAP_Criticality_ignore;
            item->value.present = NGAP_E_RABModifyItemBearerModResIEs__value_PR_E_RABModifyItemBearerModRes;
            item->value.choice.E_RABModifyItemBearerModRes.e_RAB_ID = e_rab_modify_resp_p->e_rabs[i].e_rab_id;
            NGAP_DEBUG("e_rab_modify_resp: modified e_rab ID %ld\n", item->value.choice.E_RABModifyItemBearerModRes.e_RAB_ID);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABModifyListBearerModRes.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* optional */
    if (e_rab_modify_resp_p->nb_of_e_rabs_failed > 0) {
        ie = (NGAP_E_RABModifyResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABModifyResponseIEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_E_RABFailedToModifyList;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_E_RABModifyResponseIEs__value_PR_E_RABList;

        for (i = 0; i < e_rab_modify_resp_p->nb_of_e_rabs_failed; i++) {
            NGAP_E_RABItemIEs_t *item;
            item = (NGAP_E_RABItemIEs_t *)calloc(1, sizeof(NGAP_E_RABItemIEs_t));
            item->id = NGAP_ProtocolIE_ID_id_E_RABItem;
            item->criticality = NGAP_Criticality_ignore;
            item->value.present = NGAP_E_RABItemIEs__value_PR_E_RABItem;
            item->value.choice.E_RABItem.e_RAB_ID = e_rab_modify_resp_p->e_rabs_failed[i].e_rab_id;
            item->value.choice.E_RABItem.cause.present = e_rab_modify_resp_p->e_rabs_failed[i].cause;

            switch(item->value.choice.E_RABItem.cause.present) {
            case NGAP_Cause_PR_radioNetwork:
                item->value.choice.E_RABItem.cause.choice.radioNetwork = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_transport:
                item->value.choice.E_RABItem.cause.choice.transport = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_nas:
                item->value.choice.E_RABItem.cause.choice.nas = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_protocol:
                item->value.choice.E_RABItem.cause.choice.protocol = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_misc:
                item->value.choice.E_RABItem.cause.choice.misc = e_rab_modify_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_NOTHING:
            default:
                break;
            }

            NGAP_DEBUG("e_rab_modify_resp: failed e_rab ID %ld\n", item->value.choice.E_RABItem.e_RAB_ID);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABList.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* optional */
    if (0) {
        ie = (NGAP_E_RABModifyResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABModifyResponseIEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_CriticalityDiagnostics;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_E_RABModifyResponseIEs__value_PR_CriticalityDiagnostics;
        // ie->value.choice.CriticalityDiagnostics = ;
        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    fprintf(stderr, "start encode\n");

    if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        NGAP_ERROR("Failed to encode uplink transport\n");
        /* Encode procedure has failed... */
        return -1;
    }

    MSC_LOG_TX_MESSAGE(
        MSC_NGAP_GNB,
        MSC_NGAP_MME,
        (const char *)buffer,
        length,
        MSC_AS_TIME_FMT" E_RAN Modify successful Outcome gNB_ue_ngap_id %u mme_ue_ngap_id %u",
        0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
        e_rab_modify_resp_p->gNB_ue_ngap_id,
        ue_context_p->mme_ue_ngap_id);
    /* UE associated signalling -> use the allocated stream */
    ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);
    return 0;
}
//------------------------------------------------------------------------------
int ngap_gNB_e_rab_release_resp(instance_t instance,
                                ngap_e_rab_release_resp_t *e_rab_release_resp_p)
//------------------------------------------------------------------------------
{
    ngap_gNB_instance_t            *ngap_gNB_instance_p = NULL;
    struct ngap_gNB_ue_context_s   *ue_context_p        = NULL;
    NGAP_NGAP_PDU_t                 pdu;
    NGAP_E_RABReleaseResponse_t    *out;
    NGAP_E_RABReleaseResponseIEs_t *ie;
    uint8_t  *buffer  = NULL;
    uint32_t length;
    int      i;
    /* Retrieve the NGAP gNB instance associated with Mod_id */
    ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
    DevAssert(e_rab_release_resp_p != NULL);
    DevAssert(ngap_gNB_instance_p != NULL);

    if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
                        e_rab_release_resp_p->gNB_ue_ngap_id)) == NULL) {
        /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
        NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: %u\n",
                  e_rab_release_resp_p->gNB_ue_ngap_id);
        return -1;
    }

    /* Prepare the NGAP message to encode */
    memset(&pdu, 0, sizeof(pdu));
    pdu.present = NGAP_NGAP_PDU_PR_successfulOutcome;
    pdu.choice.successfulOutcome.procedureCode = NGAP_ProcedureCode_id_E_RABRelease;
    pdu.choice.successfulOutcome.criticality = NGAP_Criticality_reject;
    pdu.choice.successfulOutcome.value.present = NGAP_SuccessfulOutcome__value_PR_E_RABReleaseResponse;
    out = &pdu.choice.successfulOutcome.value.choice.E_RABReleaseResponse;
    /* mandatory */
    ie = (NGAP_E_RABReleaseResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABReleaseResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_E_RABReleaseResponseIEs__value_PR_MME_UE_NGAP_ID;
    ie->value.choice.MME_UE_NGAP_ID = ue_context_p->mme_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    /* mandatory */
    ie = (NGAP_E_RABReleaseResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABReleaseResponseIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
    ie->criticality = NGAP_Criticality_ignore;
    ie->value.present = NGAP_E_RABReleaseResponseIEs__value_PR_GNB_UE_NGAP_ID;
    ie->value.choice.GNB_UE_NGAP_ID = e_rab_release_resp_p->gNB_ue_ngap_id;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

    /* optional */
    if (e_rab_release_resp_p->nb_of_e_rabs_released > 0) {
        ie = (NGAP_E_RABReleaseResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABReleaseResponseIEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_E_RABReleaseListBearerRelComp;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_E_RABReleaseResponseIEs__value_PR_E_RABReleaseListBearerRelComp;

        for (i = 0; i < e_rab_release_resp_p->nb_of_e_rabs_released; i++) {
            NGAP_E_RABReleaseItemBearerRelCompIEs_t *item;
            item = (NGAP_E_RABReleaseItemBearerRelCompIEs_t *)calloc(1, sizeof(NGAP_E_RABReleaseItemBearerRelCompIEs_t));
            item->id = NGAP_ProtocolIE_ID_id_E_RABReleaseItemBearerRelComp;
            item->criticality = NGAP_Criticality_ignore;
            item->value.present = NGAP_E_RABReleaseItemBearerRelCompIEs__value_PR_E_RABReleaseItemBearerRelComp;
            item->value.choice.E_RABReleaseItemBearerRelComp.e_RAB_ID = e_rab_release_resp_p->e_rab_release[i].e_rab_id;
            NGAP_DEBUG("e_rab_release_resp: e_rab ID %ld\n", item->value.choice.E_RABReleaseItemBearerRelComp.e_RAB_ID);
            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABReleaseListBearerRelComp.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }

    /* optional */
    if (e_rab_release_resp_p->nb_of_e_rabs_failed > 0) {
        ie = (NGAP_E_RABReleaseResponseIEs_t *)calloc(1, sizeof(NGAP_E_RABReleaseResponseIEs_t));
        ie->id = NGAP_ProtocolIE_ID_id_E_RABFailedToReleaseList;
        ie->criticality = NGAP_Criticality_ignore;
        ie->value.present = NGAP_E_RABReleaseResponseIEs__value_PR_E_RABList;

        for (i = 0; i < e_rab_release_resp_p->nb_of_e_rabs_failed; i++) {
            NGAP_E_RABItemIEs_t *item;
            item = (NGAP_E_RABItemIEs_t *)calloc(1, sizeof(NGAP_E_RABItemIEs_t));
            item->id = NGAP_ProtocolIE_ID_id_E_RABItem;
            item->criticality = NGAP_Criticality_ignore;
            item->value.present = NGAP_E_RABItemIEs__value_PR_E_RABItem;
            item->value.choice.E_RABItem.e_RAB_ID = e_rab_release_resp_p->e_rabs_failed[i].e_rab_id;
            item->value.choice.E_RABItem.cause.present = e_rab_release_resp_p->e_rabs_failed[i].cause;

            switch(item->value.choice.E_RABItem.cause.present) {
            case NGAP_Cause_PR_radioNetwork:
                item->value.choice.E_RABItem.cause.choice.radioNetwork = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_transport:
                item->value.choice.E_RABItem.cause.choice.transport = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_nas:
                item->value.choice.E_RABItem.cause.choice.nas = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_protocol:
                item->value.choice.E_RABItem.cause.choice.protocol = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_misc:
                item->value.choice.E_RABItem.cause.choice.misc = e_rab_release_resp_p->e_rabs_failed[i].cause_value;
                break;

            case NGAP_Cause_PR_NOTHING:
            default:
                break;
            }

            ASN_SEQUENCE_ADD(&ie->value.choice.E_RABList.list, item);
        }

        ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
    }



    if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
        NGAP_ERROR("Failed to encode release response\n");
        /* Encode procedure has failed... */
        return -1;
    }

    MSC_LOG_TX_MESSAGE(
        MSC_NGAP_GNB,
        MSC_NGAP_MME,
        (const char *)buffer,
        length,
        MSC_AS_TIME_FMT" E_RAN Release successfulOutcome gNB_ue_ngap_id %u mme_ue_ngap_id %u",
        0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
        e_rab_release_resp_p->gNB_ue_ngap_id,
        ue_context_p->mme_ue_ngap_id);
    /* UE associated signalling -> use the allocated stream */
    ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                     ue_context_p->mme_ref->assoc_id, buffer,
                                     length, ue_context_p->tx_stream);
    NGAP_INFO("e_rab_release_response sended gNB_UE_NGAP_ID %d  mme_ue_ngap_id %d nb_of_e_rabs_released %d nb_of_e_rabs_failed %d\n",
              e_rab_release_resp_p->gNB_ue_ngap_id, ue_context_p->mme_ue_ngap_id,e_rab_release_resp_p->nb_of_e_rabs_released,e_rab_release_resp_p->nb_of_e_rabs_failed);
    return 0;
}

int ngap_gNB_path_switch_req(instance_t instance,
                             ngap_path_switch_req_t *path_switch_req_p)
//------------------------------------------------------------------------------
{
  ngap_gNB_instance_t          *ngap_gNB_instance_p = NULL;
  struct ngap_gNB_ue_context_s *ue_context_p        = NULL;
  struct ngap_gNB_mme_data_s   *mme_desc_p = NULL;

  NGAP_NGAP_PDU_t                 pdu;
  NGAP_PathSwitchRequest_t       *out;
  NGAP_PathSwitchRequestIEs_t    *ie;

  NGAP_E_RABToBeSwitchedDLItemIEs_t *e_RABToBeSwitchedDLItemIEs;
  NGAP_E_RABToBeSwitchedDLItem_t    *e_RABToBeSwitchedDLItem;

  uint8_t  *buffer = NULL;
  uint32_t length;
  int      ret = 0;//-1;

  /* Retrieve the NGAP gNB instance associated with Mod_id */
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);

  DevAssert(path_switch_req_p != NULL);
  DevAssert(ngap_gNB_instance_p != NULL);

  //if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
    //                                          path_switch_req_p->gNB_ue_ngap_id)) == NULL) {
    /* The context for this gNB ue ngap id doesn't exist in the map of gNB UEs */
    //NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%06x\n",
      //        path_switch_req_p->gNB_ue_ngap_id);
    //return -1;
  //}

  /* Uplink NAS transport can occur either during an ngap connected state
   * or during initial attach (for example: NAS authentication).
   */
  //if (!(ue_context_p->ue_state == NGAP_UE_CONNECTED ||
       // ue_context_p->ue_state == NGAP_UE_WAITING_CSR)) {
    //NGAP_WARN("You are attempting to send NAS data over non-connected "
        //      "gNB ue ngap id: %06x, current state: %d\n",
          //    path_switch_req_p->gNB_ue_ngap_id, ue_context_p->ue_state);
    //return -1;
  //}

  /* Select the MME corresponding to the provided GUMMEI. */
  mme_desc_p = ngap_gNB_nnsf_select_mme_by_gummei_no_cause(ngap_gNB_instance_p, path_switch_req_p->ue_gummei);

  if (mme_desc_p == NULL) {
    /*
     * In case gNB has no MME associated, the gNB should inform RRC and discard
     * this request.
     */

    NGAP_WARN("No MME is associated to the gNB\n");
    // TODO: Inform RRC
    return -1;
  }

  /* The gNB should allocate a unique gNB UE NGAP ID for this UE. The value
   * will be used for the duration of the connectivity.
   */
  ue_context_p = ngap_gNB_allocate_new_UE_context();
  DevAssert(ue_context_p != NULL);

  /* Keep a reference to the selected MME */
  ue_context_p->mme_ref       = mme_desc_p;
  ue_context_p->ue_initial_id = path_switch_req_p->ue_initial_id;
  ue_context_p->gNB_instance  = ngap_gNB_instance_p;

  do {
    struct ngap_gNB_ue_context_s *collision_p;

    /* Peek a random value for the gNB_ue_ngap_id */
    ue_context_p->gNB_ue_ngap_id = (random() + random()) & 0x00ffffff;

    if ((collision_p = RB_INSERT(ngap_ue_map, &ngap_gNB_instance_p->ngap_ue_head, ue_context_p))
        == NULL) {
      NGAP_DEBUG("Found usable gNB_ue_ngap_id: 0x%06x %u(10)\n",
                 ue_context_p->gNB_ue_ngap_id,
                 ue_context_p->gNB_ue_ngap_id);
      /* Break the loop as the id is not already used by another UE */
      break;
    }
  } while(1);
  
  ue_context_p->mme_ue_ngap_id = path_switch_req_p->mme_ue_ngap_id;

  /* Prepare the NGAP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = NGAP_ProcedureCode_id_PathSwitchRequest;
  pdu.choice.initiatingMessage.criticality = NGAP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = NGAP_InitiatingMessage__value_PR_PathSwitchRequest;
  out = &pdu.choice.initiatingMessage.value.choice.PathSwitchRequest;

  /* mandatory */
  ie = (NGAP_PathSwitchRequestIEs_t *)calloc(1, sizeof(NGAP_PathSwitchRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_PathSwitchRequestIEs__value_PR_GNB_UE_NGAP_ID;
  ie->value.choice.GNB_UE_NGAP_ID = ue_context_p->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  if (path_switch_req_p->nb_of_e_rabs > 0) {
    ie = (NGAP_PathSwitchRequestIEs_t *)calloc(1, sizeof(NGAP_PathSwitchRequestIEs_t));
    ie->id = NGAP_ProtocolIE_ID_id_E_RABToBeSwitchedDLList;
    ie->criticality = NGAP_Criticality_reject;
    ie->value.present = NGAP_PathSwitchRequestIEs__value_PR_E_RABToBeSwitchedDLList;

    for (int i = 0; i < path_switch_req_p->nb_of_e_rabs; i++) {
      e_RABToBeSwitchedDLItemIEs = (NGAP_E_RABToBeSwitchedDLItemIEs_t *)calloc(1, sizeof(NGAP_E_RABToBeSwitchedDLItemIEs_t));
      e_RABToBeSwitchedDLItemIEs->id = NGAP_ProtocolIE_ID_id_E_RABToBeSwitchedDLItem;
      e_RABToBeSwitchedDLItemIEs->criticality = NGAP_Criticality_reject;
      e_RABToBeSwitchedDLItemIEs->value.present = NGAP_E_RABToBeSwitchedDLItemIEs__value_PR_E_RABToBeSwitchedDLItem;

      e_RABToBeSwitchedDLItem = &e_RABToBeSwitchedDLItemIEs->value.choice.E_RABToBeSwitchedDLItem;
      e_RABToBeSwitchedDLItem->e_RAB_ID = path_switch_req_p->e_rabs_tobeswitched[i].e_rab_id;
      INT32_TO_OCTET_STRING(path_switch_req_p->e_rabs_tobeswitched[i].gtp_teid, &e_RABToBeSwitchedDLItem->gTP_TEID);

      e_RABToBeSwitchedDLItem->transportLayerAddress.size  = path_switch_req_p->e_rabs_tobeswitched[i].gNB_addr.length;
      e_RABToBeSwitchedDLItem->transportLayerAddress.bits_unused = 0;

      e_RABToBeSwitchedDLItem->transportLayerAddress.buf = calloc(1,e_RABToBeSwitchedDLItem->transportLayerAddress.size);

      memcpy (e_RABToBeSwitchedDLItem->transportLayerAddress.buf,
                path_switch_req_p->e_rabs_tobeswitched[i].gNB_addr.buffer,
                path_switch_req_p->e_rabs_tobeswitched[i].gNB_addr.length);

      NGAP_DEBUG("path_switch_req: e_rab ID %ld, teid %u, enb_addr %d.%d.%d.%d, SIZE %zu\n",
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
  ie = (NGAP_PathSwitchRequestIEs_t *)calloc(1, sizeof(NGAP_PathSwitchRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_SourceMME_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_PathSwitchRequestIEs__value_PR_MME_UE_NGAP_ID;
  ie->value.choice.MME_UE_NGAP_ID = path_switch_req_p->mme_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (NGAP_PathSwitchRequestIEs_t *)calloc(1, sizeof(NGAP_PathSwitchRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_EUTRAN_CGI;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_PathSwitchRequestIEs__value_PR_EUTRAN_CGI;
  MACRO_GNB_ID_TO_CELL_IDENTITY(ngap_gNB_instance_p->gNB_id,
                                0,
                                &ie->value.choice.EUTRAN_CGI.cell_ID);
  MCC_MNC_TO_TBCD(ngap_gNB_instance_p->mcc[0],
                  ngap_gNB_instance_p->mnc[0],
                  ngap_gNB_instance_p->mnc_digit_length[0],
                  &ie->value.choice.EUTRAN_CGI.pLMNidentity);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (NGAP_PathSwitchRequestIEs_t *)calloc(1, sizeof(NGAP_PathSwitchRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_TAI;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_PathSwitchRequestIEs__value_PR_TAI;
  /* Assuming TAI is the TAI from the cell */
  INT16_TO_OCTET_STRING(ngap_gNB_instance_p->tac, &ie->value.choice.TAI.tAC);
  MCC_MNC_TO_PLMNID(ngap_gNB_instance_p->mcc[0],
                    ngap_gNB_instance_p->mnc[0],
                    ngap_gNB_instance_p->mnc_digit_length[0],
                    &ie->value.choice.TAI.pLMNidentity);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (NGAP_PathSwitchRequestIEs_t *)calloc(1, sizeof(NGAP_PathSwitchRequestIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_UESecurityCapabilities;
  ie->criticality = NGAP_Criticality_ignore;
  ie->value.present = NGAP_PathSwitchRequestIEs__value_PR_UESecurityCapabilities;
  ENCRALG_TO_BIT_STRING(path_switch_req_p->security_capabilities.encryption_algorithms,
              &ie->value.choice.UESecurityCapabilities.encryptionAlgorithms);
  INTPROTALG_TO_BIT_STRING(path_switch_req_p->security_capabilities.integrity_algorithms,
              &ie->value.choice.UESecurityCapabilities.integrityProtectionAlgorithms);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (ngap_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NGAP_ERROR("Failed to encode Path Switch Req \n");
    /* Encode procedure has failed... */
    return -1;
  }

  /* Update the current NGAP UE state */
  ue_context_p->ue_state = NGAP_UE_WAITING_CSR;

  /* Assign a stream for this UE :
   * From 3GPP 36.412 7)Transport layers:
   *  Within the SCTP association established between one MME and gNB pair:
   *  - a single pair of stream identifiers shall be reserved for the sole use
   *      of NGAP elementary procedures that utilize non UE-associated signalling.
   *  - At least one pair of stream identifiers shall be reserved for the sole use
   *      of NGAP elementary procedures that utilize UE-associated signallings.
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
    MSC_NGAP_GNB,
    MSC_NGAP_MME,
    (const char *)buffer,
    length,
    MSC_AS_TIME_FMT" E_RAN Setup successfulOutcome gNB_ue_ngap_id %u mme_ue_ngap_id %u",
    0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_p->gNB_ue_ngap_id,
    path_switch_req_p->mme_ue_ngap_id);

  /* UE associated signalling -> use the allocated stream */
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                   mme_desc_p->assoc_id, buffer,
                                   length, ue_context_p->tx_stream);

  return ret;
}


//-----------------------------------------------------------------------------
/*
* gNB generate a S1 E_RAB Modification Indication towards MME
*/
/*int ngap_gNB_generate_E_RAB_Modification_Indication(
		instance_t instance,
  ngap_e_rab_modification_ind_t *e_rab_modification_ind)
//-----------------------------------------------------------------------------
{
  struct ngap_gNB_ue_context_s        *ue_context_p        = NULL;
  NGAP_NGAP_PDU_t            pdu;
  NGAP_E_RABModificationIndication_t     *out = NULL;
  NGAP_E_RABModificationIndicationIEs_t   *ie = NULL;
  NGAP_E_RABToBeModifiedItemBearerModInd_t 	  *E_RAB_ToBeModifiedItem_BearerModInd = NULL;
  NGAP_E_RABToBeModifiedItemBearerModIndIEs_t *E_RAB_ToBeModifiedItem_BearerModInd_IEs = NULL;

  NGAP_E_RABNotToBeModifiedItemBearerModInd_t 	  *E_RAB_NotToBeModifiedItem_BearerModInd = NULL;
  NGAP_E_RABNotToBeModifiedItemBearerModIndIEs_t  *E_RAB_NotToBeModifiedItem_BearerModInd_IEs = NULL;


  ngap_gNB_instance_t          *ngap_gNB_instance_p = NULL;
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  uint8_t  *buffer = NULL;
  uint32_t  len = 0;
  int       ret = 0;
  DevAssert(ngap_gNB_instance_p != NULL);
  DevAssert(e_rab_modification_ind != NULL);

  int num_e_rabs_tobemodified = e_rab_modification_ind->nb_of_e_rabs_tobemodified;
  int num_e_rabs_nottobemodified = e_rab_modification_ind->nb_of_e_rabs_nottobemodified;

  uint32_t CSG_id = 0;

  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
		  e_rab_modification_ind->gNB_ue_ngap_id)) == NULL) {
          // The context for this gNB ue ngap id doesn't exist in the map of gNB UEs 
          NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%06x\n",
        		  e_rab_modification_ind->gNB_ue_ngap_id);
          return -1;
  }

  // Prepare the NGAP message to encode 
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = NGAP_ProcedureCode_id_E_RABModificationIndication;
  pdu.choice.initiatingMessage.criticality = NGAP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = NGAP_InitiatingMessage__value_PR_E_RABModificationIndication;
  out = &pdu.choice.initiatingMessage.value.choice.E_RABModificationIndication;
  // mandatory 
  ie = (NGAP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(NGAP_E_RABModificationIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_MME_UE_NGAP_ID;
  ie->value.choice.MME_UE_NGAP_ID = e_rab_modification_ind->mme_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ie = (NGAP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(NGAP_E_RABModificationIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_GNB_UE_NGAP_ID;
  ie->value.choice.GNB_UE_NGAP_ID = e_rab_modification_ind->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  //E-RABs to be modified list
  ie = (NGAP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(NGAP_E_RABModificationIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_E_RABToBeModifiedListBearerModInd;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_E_RABToBeModifiedListBearerModInd;

  //The following two for-loops here will probably need to change. We should do a different type of search
  for(int i=0; i<num_e_rabs_tobemodified; i++){
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs = (NGAP_E_RABToBeModifiedItemBearerModIndIEs_t *)calloc(1,sizeof(NGAP_E_RABToBeModifiedItemBearerModIndIEs_t));
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->id = NGAP_ProtocolIE_ID_id_E_RABToBeModifiedItemBearerModInd;
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->criticality = NGAP_Criticality_reject;
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->value.present = NGAP_E_RABToBeModifiedItemBearerModIndIEs__value_PR_E_RABToBeModifiedItemBearerModInd;
	  E_RAB_ToBeModifiedItem_BearerModInd = &E_RAB_ToBeModifiedItem_BearerModInd_IEs->value.choice.E_RABToBeModifiedItemBearerModInd;

	  {
	  E_RAB_ToBeModifiedItem_BearerModInd->e_RAB_ID = e_rab_modification_ind->e_rabs_tobemodified[i].e_rab_id;

	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size  = e_rab_modification_ind->e_rabs_tobemodified[i].gNB_addr.length/8;
	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.bits_unused = e_rab_modification_ind->e_rabs_tobemodified[i].gNB_addr.length%8;
	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.buf = calloc(1, E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size);
	  memcpy (E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.buf, e_rab_modification_ind->e_rabs_tobemodified[i].gNB_addr.buffer,
			  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size);

	  INT32_TO_OCTET_STRING(e_rab_modification_ind->e_rabs_tobemodified[i].gtp_teid, &E_RAB_ToBeModifiedItem_BearerModInd->dL_GTP_TEID);

	  }
	  ASN_SEQUENCE_ADD(&ie->value.choice.E_RABToBeModifiedListBearerModInd.list, E_RAB_ToBeModifiedItem_BearerModInd_IEs);
  }

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  //E-RABs NOT to be modified list
  ie = (NGAP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(NGAP_E_RABModificationIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_E_RABNotToBeModifiedListBearerModInd;
  ie->criticality = NGAP_Criticality_reject;
  if(num_e_rabs_nottobemodified > 0) {
	  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_E_RABNotToBeModifiedListBearerModInd;

	  for(int i=0; i<num_e_rabs_nottobemodified; i++){
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs = (NGAP_E_RABNotToBeModifiedItemBearerModIndIEs_t *)calloc(1,sizeof(NGAP_E_RABNotToBeModifiedItemBearerModIndIEs_t));
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->id = NGAP_ProtocolIE_ID_id_E_RABNotToBeModifiedItemBearerModInd;
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->criticality = NGAP_Criticality_reject;
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->value.present = NGAP_E_RABNotToBeModifiedItemBearerModIndIEs__value_PR_E_RABNotToBeModifiedItemBearerModInd;
		  E_RAB_NotToBeModifiedItem_BearerModInd = &E_RAB_NotToBeModifiedItem_BearerModInd_IEs->value.choice.E_RABNotToBeModifiedItemBearerModInd;

		  {
			  E_RAB_NotToBeModifiedItem_BearerModInd->e_RAB_ID = e_rab_modification_ind->e_rabs_nottobemodified[i].e_rab_id;

			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size  = e_rab_modification_ind->e_rabs_nottobemodified[i].gNB_addr.length/8;
			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.bits_unused = e_rab_modification_ind->e_rabs_nottobemodified[i].gNB_addr.length%8;
			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.buf =
	  	    				calloc(1, E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size);
			  memcpy (E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.buf, e_rab_modification_ind->e_rabs_nottobemodified[i].gNB_addr.buffer,
					  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size);

			  INT32_TO_OCTET_STRING(e_rab_modification_ind->e_rabs_nottobemodified[i].gtp_teid, &E_RAB_NotToBeModifiedItem_BearerModInd->dL_GTP_TEID);

		  }
		  ASN_SEQUENCE_ADD(&ie->value.choice.E_RABNotToBeModifiedListBearerModInd.list, E_RAB_NotToBeModifiedItem_BearerModInd_IEs);
	  }
  }
  else{
	  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_E_RABNotToBeModifiedListBearerModInd;
	  ie->value.choice.E_RABNotToBeModifiedListBearerModInd.list.size = 0;
  }  
  
	   

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ie = (NGAP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(NGAP_E_RABModificationIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_CSGMembershipInfo;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_CSGMembershipInfo;
  ie->value.choice.CSGMembershipInfo.cSGMembershipStatus = NGAP_CSGMembershipStatus_member;
  INT32_TO_BIT_STRING(CSG_id, &ie->value.choice.CSGMembershipInfo.cSG_Id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);


  if (ngap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    NGAP_ERROR("Failed to encode S1 E-RAB modification indication \n");
    return -1;
  }

  // Non UE-Associated signalling -> stream = 0 
  NGAP_INFO("Size of encoded message: %d \n", len);
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                       ue_context_p->mme_ref->assoc_id, buffer,
                                       len, ue_context_p->tx_stream);  

//ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance, ue_context_p->mme_ref->assoc_id, buffer, len, 0);
  return ret;
}*/

int ngap_gNB_generate_E_RAB_Modification_Indication(
		instance_t instance,
  ngap_e_rab_modification_ind_t *e_rab_modification_ind)
//-----------------------------------------------------------------------------
{
  struct ngap_gNB_ue_context_s        *ue_context_p        = NULL;
  NGAP_NGAP_PDU_t            pdu;
  NGAP_E_RABModificationIndication_t     *out = NULL;
  NGAP_E_RABModificationIndicationIEs_t   *ie = NULL;
  NGAP_E_RABToBeModifiedItemBearerModInd_t 	  *E_RAB_ToBeModifiedItem_BearerModInd = NULL;
  NGAP_E_RABToBeModifiedItemBearerModIndIEs_t *E_RAB_ToBeModifiedItem_BearerModInd_IEs = NULL;

  //NGAP_E_RABNotToBeModifiedItemBearerModInd_t 	  *E_RAB_NotToBeModifiedItem_BearerModInd = NULL;
  //NGAP_E_RABNotToBeModifiedItemBearerModIndIEs_t  *E_RAB_NotToBeModifiedItem_BearerModInd_IEs = NULL;


  ngap_gNB_instance_t          *ngap_gNB_instance_p = NULL;
  ngap_gNB_instance_p = ngap_gNB_get_instance(instance);
  uint8_t  *buffer = NULL;
  uint32_t  len = 0;
  int       ret = 0;
  DevAssert(ngap_gNB_instance_p != NULL);
  DevAssert(e_rab_modification_ind != NULL);

  int num_e_rabs_tobemodified = e_rab_modification_ind->nb_of_e_rabs_tobemodified;
  //int num_e_rabs_nottobemodified = e_rab_modification_ind->nb_of_e_rabs_nottobemodified;

  //uint32_t CSG_id = 0;
  //uint32_t pseudo_gtp_teid = 10;

  if ((ue_context_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p,
		  e_rab_modification_ind->gNB_ue_ngap_id)) == NULL) {
          // The context for this gNB ue ngap id doesn't exist in the map of gNB UEs 
          NGAP_WARN("Failed to find ue context associated with gNB ue ngap id: 0x%06x\n",
        		  e_rab_modification_ind->gNB_ue_ngap_id);
          return -1;
  }

  // Prepare the NGAP message to encode 
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NGAP_NGAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = NGAP_ProcedureCode_id_E_RABModificationIndication;
  pdu.choice.initiatingMessage.criticality = NGAP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = NGAP_InitiatingMessage__value_PR_E_RABModificationIndication;
  out = &pdu.choice.initiatingMessage.value.choice.E_RABModificationIndication;
  /* mandatory */
  ie = (NGAP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(NGAP_E_RABModificationIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_MME_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_MME_UE_NGAP_ID;
  ie->value.choice.MME_UE_NGAP_ID = e_rab_modification_ind->mme_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  ie = (NGAP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(NGAP_E_RABModificationIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_gNB_UE_NGAP_ID;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_GNB_UE_NGAP_ID;
  ie->value.choice.GNB_UE_NGAP_ID = e_rab_modification_ind->gNB_ue_ngap_id;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  //E-RABs to be modified list
  ie = (NGAP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(NGAP_E_RABModificationIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_E_RABToBeModifiedListBearerModInd;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_E_RABToBeModifiedListBearerModInd;

  //The following two for-loops here will probably need to change. We should do a different type of search
  for(int i=0; i<num_e_rabs_tobemodified; i++){
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs = (NGAP_E_RABToBeModifiedItemBearerModIndIEs_t *)calloc(1,sizeof(NGAP_E_RABToBeModifiedItemBearerModIndIEs_t));
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->id = NGAP_ProtocolIE_ID_id_E_RABToBeModifiedItemBearerModInd;
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->criticality = NGAP_Criticality_reject;
	  E_RAB_ToBeModifiedItem_BearerModInd_IEs->value.present = NGAP_E_RABToBeModifiedItemBearerModIndIEs__value_PR_E_RABToBeModifiedItemBearerModInd;
	  E_RAB_ToBeModifiedItem_BearerModInd = &E_RAB_ToBeModifiedItem_BearerModInd_IEs->value.choice.E_RABToBeModifiedItemBearerModInd;

	  {
	  E_RAB_ToBeModifiedItem_BearerModInd->e_RAB_ID = e_rab_modification_ind->e_rabs_tobemodified[i].e_rab_id;

	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size  = e_rab_modification_ind->e_rabs_tobemodified[i].gNB_addr.length/8;
	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.bits_unused = e_rab_modification_ind->e_rabs_tobemodified[i].gNB_addr.length%8;
	  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.buf = calloc(1, E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size);
	  memcpy (E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.buf, e_rab_modification_ind->e_rabs_tobemodified[i].gNB_addr.buffer,
			  E_RAB_ToBeModifiedItem_BearerModInd->transportLayerAddress.size);

	  INT32_TO_OCTET_STRING(e_rab_modification_ind->e_rabs_tobemodified[i].gtp_teid, &E_RAB_ToBeModifiedItem_BearerModInd->dL_GTP_TEID);

	  }
	  ASN_SEQUENCE_ADD(&ie->value.choice.E_RABToBeModifiedListBearerModInd.list, E_RAB_ToBeModifiedItem_BearerModInd_IEs);
  }

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  //E-RABs NOT to be modified list
  /*ie = (NGAP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(NGAP_E_RABModificationIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_E_RABNotToBeModifiedListBearerModInd;
  ie->criticality = NGAP_Criticality_reject;
  //if(num_e_rabs_nottobemodified > 0) {
	  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_E_RABNotToBeModifiedListBearerModInd;

	  for(int i=0; i<num_e_rabs_tobemodified; i++){
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs = (NGAP_E_RABNotToBeModifiedItemBearerModIndIEs_t *)calloc(1,sizeof(NGAP_E_RABNotToBeModifiedItemBearerModIndIEs_t));
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->id = NGAP_ProtocolIE_ID_id_E_RABNotToBeModifiedItemBearerModInd;
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->criticality = NGAP_Criticality_reject;
		  E_RAB_NotToBeModifiedItem_BearerModInd_IEs->value.present = NGAP_E_RABNotToBeModifiedItemBearerModIndIEs__value_PR_E_RABNotToBeModifiedItemBearerModInd;
		  E_RAB_NotToBeModifiedItem_BearerModInd = &E_RAB_NotToBeModifiedItem_BearerModInd_IEs->value.choice.E_RABNotToBeModifiedItemBearerModInd;

		  {
			  E_RAB_NotToBeModifiedItem_BearerModInd->e_RAB_ID = 10; //e_rab_modification_ind->e_rabs_tobemodified[i].e_rab_id;

			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size  = e_rab_modification_ind->e_rabs_tobemodified[i].gNB_addr.length/8;
			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.bits_unused = e_rab_modification_ind->e_rabs_tobemodified[i].gNB_addr.length%8;
			  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.buf =
	  	    				calloc(1, E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size);
			  memcpy (E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.buf, e_rab_modification_ind->e_rabs_tobemodified[i].gNB_addr.buffer,
					  E_RAB_NotToBeModifiedItem_BearerModInd->transportLayerAddress.size);

			  //INT32_TO_OCTET_STRING(e_rab_modification_ind->e_rabs_tobemodified[i].gtp_teid, &E_RAB_NotToBeModifiedItem_BearerModInd->dL_GTP_TEID);
			    INT32_TO_OCTET_STRING(pseudo_gtp_teid, &E_RAB_NotToBeModifiedItem_BearerModInd->dL_GTP_TEID);

		  }
		  ASN_SEQUENCE_ADD(&ie->value.choice.E_RABNotToBeModifiedListBearerModInd.list, E_RAB_NotToBeModifiedItem_BearerModInd_IEs);
	  }
 // }
  //else{
//	  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_E_RABNotToBeModifiedListBearerModInd;
//	  ie->value.choice.E_RABNotToBeModifiedListBearerModInd.list.size = 0;
//  } / 
  
	   

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);*/

  /*ie = (NGAP_E_RABModificationIndicationIEs_t *)calloc(1, sizeof(NGAP_E_RABModificationIndicationIEs_t));
  ie->id = NGAP_ProtocolIE_ID_id_CSGMembershipInfo;
  ie->criticality = NGAP_Criticality_reject;
  ie->value.present = NGAP_E_RABModificationIndicationIEs__value_PR_CSGMembershipInfo;
  ie->value.choice.CSGMembershipInfo.cSGMembershipStatus = NGAP_CSGMembershipStatus_member;
  INT32_TO_BIT_STRING(CSG_id, &ie->value.choice.CSGMembershipInfo.cSG_Id);
  ie->value.choice.CSGMembershipInfo.cSG_Id.bits_unused=5; 
  ie->value.choice.CSGMembershipInfo.cellAccessMode = NGAP_CellAccessMode_hybrid;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);*/
  
  if (ngap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    NGAP_ERROR("Failed to encode S1 E-RAB modification indication \n");
    return -1;
  }

  // Non UE-Associated signalling -> stream = 0 
  NGAP_INFO("Size of encoded message: %d \n", len);
  ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance,
                                       ue_context_p->mme_ref->assoc_id, buffer,
                                       len, ue_context_p->tx_stream);  

//ngap_gNB_itti_send_sctp_data_req(ngap_gNB_instance_p->instance, ue_context_p->mme_ref->assoc_id, buffer, len, 0);
  return ret;
}


