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

#include "rrc_gNB_NGAP.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "rrc_eNB_S1AP.h"
#include "gnb_config.h"
#include "common/ran_context.h"
#include "gtpv1u.h"

#include "asn1_conversions.h"
#include "intertask_interface.h"
#include "pdcp.h"
#include "pdcp_primitives.h"

#include "msc.h"

#include "gtpv1u_eNB_task.h"
#include "RRC/LTE/rrc_eNB_GTPV1U.h"

#include "S1AP_NAS-PDU.h"
#include "executables/softmodem-common.h"

extern RAN_CONTEXT_t RC;

/* Value to indicate an invalid UE initial id */
static const uint16_t UE_INITIAL_ID_INVALID = 0;

/*! \fn uint16_t get_next_ue_initial_id(uint8_t mod_id)
 *\brief provide an UE initial ID for NGAP initial communication.
 *\param mod_id Instance ID of gNB.
 *\return the UE initial ID.
 */
//------------------------------------------------------------------------------
static uint16_t
get_next_ue_initial_id(
    const module_id_t mod_id
)
//------------------------------------------------------------------------------
{
    static uint16_t ue_initial_id[NUMBER_OF_gNB_MAX];
    ue_initial_id[mod_id]++;

    /* Never use UE_INITIAL_ID_INVALID this is the invalid id! */
    if (ue_initial_id[mod_id] == UE_INITIAL_ID_INVALID) {
        ue_initial_id[mod_id]++;
    }

    return ue_initial_id[mod_id];
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
    gNB_RRC_INST *rrc = RC.nrrrc[ctxt_pP->module_id];
    MessageDef         *message_p         = NULL;
    rrc_ue_ngap_ids_t  *rrc_ue_ngap_ids_p = NULL;
    hashtable_rc_t      h_rc;

    message_p = itti_alloc_new_message(TASK_RRC_GNB, NGAP_NAS_FIRST_REQ);
    memset(&message_p->ittiMsg.ngap_nas_first_req, 0, sizeof(ngap_nas_first_req_t));
    ue_context_pP->ue_context.ue_initial_id = get_next_ue_initial_id(ctxt_pP->module_id);
    NGAP_NAS_FIRST_REQ(message_p).ue_initial_id = ue_context_pP->ue_context.ue_initial_id;
    rrc_ue_ngap_ids_p = malloc(sizeof(rrc_ue_ngap_ids_t));
    rrc_ue_ngap_ids_p->ue_initial_id  = ue_context_pP->ue_context.ue_initial_id;
    rrc_ue_ngap_ids_p->gNB_ue_ngap_id = UE_INITIAL_ID_INVALID;
    rrc_ue_ngap_ids_p->ue_rnti        = ctxt_pP->rnti;

    // h_rc = hashtable_insert(RC.nrrrc[ctxt_pP->module_id]->initial_id2_s1ap_ids,
    //                         (hash_key_t)ue_context_pP->ue_context.ue_initial_id,
    //                         rrc_ue_s1ap_ids_p);

    // if (h_rc != HASH_TABLE_OK) {
    //   LOG_E(S1AP, "[eNB %d] Error while hashtable_insert in initial_id2_s1ap_ids ue_initial_id %u\n",
    //         ctxt_pP->module_id,
    //         ue_context_pP->ue_context.ue_initial_id);
    // }

    /* Assume that cause is coded in the same way in RRC and NGap, just check that the value is in NGap range */
    AssertFatal(ue_context_pP->ue_context.establishment_cause < NGAP_RRC_CAUSE_LAST,
                "Establishment cause invalid (%jd/%d) for gNB %d!",
                ue_context_pP->ue_context.establishment_cause,
                NGAP_RRC_CAUSE_LAST,
                ctxt_pP->module_id);
    NGAP_NAS_FIRST_REQ(message_p).establishment_cause = ue_context_pP->ue_context.establishment_cause;
    
    /* Forward NAS message */
    NGAP_NAS_FIRST_REQ(message_p).nas_pdu.buffer = rrcSetupComplete->dedicatedNAS_Message.buf;
    NGAP_NAS_FIRST_REQ(message_p).nas_pdu.length = rrcSetupComplete->dedicatedNAS_Message.size;
    // extract_imsi(NGAP_NAS_FIRST_REQ (message_p).nas_pdu.buffer,
    //              NGAP_NAS_FIRST_REQ (message_p).nas_pdu.length,
    //              ue_context_pP);

    /* Fill UE identities with available information */
    NGAP_NAS_FIRST_REQ(message_p).ue_identity.presenceMask       = NGAP_UE_IDENTITIES_NONE;
    /* Fill s-TMSI */
    NGAP_NAS_FIRST_REQ(message_p).ue_identity.s_tmsi.amf_set_id  = ue_context_pP->ue_context.Initialue_identity_5g_s_TMSI.amf_set_id;
    NGAP_NAS_FIRST_REQ(message_p).ue_identity.s_tmsi.amf_pointer = ue_context_pP->ue_context.Initialue_identity_5g_s_TMSI.amf_pointer;
    NGAP_NAS_FIRST_REQ(message_p).ue_identity.s_tmsi.m_tmsi      = ue_context_pP->ue_context.Initialue_identity_5g_s_TMSI.fiveg_tmsi;

    /* selected_plmn_identity: IE is 1-based, convert to 0-based (C array) */
    int selected_plmn_identity = rrcSetupComplete->selectedPLMN_Identity - 1;
    NGAP_NAS_FIRST_REQ(message_p).selected_plmn_identity = selected_plmn_identity;

    if (rrcSetupComplete->registeredAMF != NULL) {
        NR_RegisteredAMF_t *r_amf = rrcSetupComplete->registeredAMF;
        NGAP_NAS_FIRST_REQ(message_p).ue_identity.presenceMask |= NGAP_UE_IDENTITIES_guami;

        if (r_amf->plmn_Identity != NULL) {
            if ((r_amf->plmn_Identity->mcc != NULL) && (r_amf->plmn_Identity->mcc->list.count > 0)) {
                /* Use first indicated PLMN MCC if it is defined */
                NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.mcc = *r_amf->plmn_Identity->mcc->list.array[selected_plmn_identity];
                LOG_I(NGAP, "[gNB %d] Build NGAP_NAS_FIRST_REQ adding in s_TMSI: GUMMEI MCC %u ue %x\n",
                    ctxt_pP->module_id,
                    NGAP_NAS_FIRST_REQ (message_p).ue_identity.guami.mcc,
                    ue_context_pP->ue_context.rnti);
            }

            if (r_amf->plmn_Identity->mnc.list.count > 0) {
                /* Use first indicated PLMN MNC if it is defined */
                NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.mnc = *r_amf->plmn_Identity->mnc.list.array[selected_plmn_identity];
                LOG_I(NGAP, "[gNB %d] Build NGAP_NAS_FIRST_REQ adding in s_TMSI: GUMMEI MNC %u ue %x\n",
                    ctxt_pP->module_id,
                    NGAP_NAS_FIRST_REQ (message_p).ue_identity.guami.mnc,
                    ue_context_pP->ue_context.rnti);
            }
        } else {
            /* TODO */
        }

        /* amf_Identifier */
        uint32_t amf_Id = BIT_STRING_to_uint32(&r_amf->amf_Identifier);
        NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.amf_region_id = amf_Id >> 16;
        NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.amf_set_id    = ue_context_pP->ue_context.Initialue_identity_5g_s_TMSI.amf_set_id;
        NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.amf_pointer   = ue_context_pP->ue_context.Initialue_identity_5g_s_TMSI.amf_pointer;

        ue_context_pP->ue_context.ue_guami.mcc = NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.mcc;
        ue_context_pP->ue_context.ue_guami.mnc = NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.mnc;
        ue_context_pP->ue_context.ue_guami.mnc_len = NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.mnc_len;
        ue_context_pP->ue_context.ue_guami.amf_region_id = NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.amf_region_id;
        ue_context_pP->ue_context.ue_guami.amf_set_id = NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.amf_set_id;
        ue_context_pP->ue_context.ue_guami.amf_pointer = NGAP_NAS_FIRST_REQ(message_p).ue_identity.guami.amf_pointer;

        MSC_LOG_TX_MESSAGE(MSC_NGAP_GNB,
                           MSC_NGAP_AMF,
                           (const char *)&message_p->ittiMsg.ngap_nas_first_req,
                           sizeof(ngap_nas_first_req_t),
                           MSC_AS_TIME_FMT" NGAP_NAS_FIRST_REQ gNB %u UE %x",
                           MSC_AS_TIME_ARGS(ctxt_pP),
                           ctxt_pP->module_id,
                           ctxt_pP->rnti);
        LOG_I(NGAP, "[gNB %d] Build NGAP_NAS_FIRST_REQ adding in s_TMSI: GUAMI amf_set_id %u amf_region_id %u ue %x\n",
              ctxt_pP->module_id,
              NGAP_NAS_FIRST_REQ (message_p).ue_identity.guami.amf_set_id,
              NGAP_NAS_FIRST_REQ (message_p).ue_identity.guami.amf_region_id,
              ue_context_pP->ue_context.rnti);
    }

    itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, message_p);
}