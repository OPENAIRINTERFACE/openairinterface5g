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
#include "UTIL/OSA/osa_defs.h"

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
* Get the UE NG struct containing hashtables NG_id/UE_id.
* Is also used to set the NG_id of the UE, depending on inputs.
*/
struct rrc_ue_ngap_ids_s *
rrc_gNB_NGAP_get_ue_ids(
    gNB_RRC_INST   *const rrc_instance_pP,
    const uint16_t ue_initial_id,
    const uint32_t gNB_ue_ngap_idP
)
//------------------------------------------------------------------------------
{
  rrc_ue_ngap_ids_t *result = NULL;

  /* TODO */

  return result;
}

//------------------------------------------------------------------------------
static struct rrc_gNB_ue_context_s *
rrc_gNB_get_ue_context_from_ngap_ids(
    const instance_t  instanceP,
    const uint16_t    ue_initial_idP,
    const uint32_t    gNB_ue_ngap_idP
) 
//------------------------------------------------------------------------------
{
  rrc_ue_ngap_ids_t *temp = NULL;
  temp = rrc_gNB_NGAP_get_ue_ids(RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instanceP)], ue_initial_idP, gNB_ue_ngap_idP);

  if (temp != NULL) {
    return rrc_gNB_get_ue_context(RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instanceP)], temp->ue_rnti);
  }

  return NULL;
}

//------------------------------------------------------------------------------
void
nr_rrc_pdcp_config_security(
    const protocol_ctxt_t  *const ctxt_pP,
    rrc_gNB_ue_context_t   *const ue_context_pP,
    const uint8_t          send_security_mode_command
)
//------------------------------------------------------------------------------
{
  NR_SRB_ToAddModList_t              *SRB_configList = ue_context_pP->ue_context.SRB_configList;
  (void)SRB_configList;
  uint8_t                            *kRRCenc = NULL;
  uint8_t                            *kRRCint = NULL;
  uint8_t                            *kUPenc = NULL;
  pdcp_t                             *pdcp_p   = NULL;
  static int                          print_keys= 1;
  hashtable_rc_t                      h_rc;
  hash_key_t                          key;

#ifndef PHYSIM
  /* Derive the keys from kgnb */
  if (SRB_configList != NULL) {
    derive_key_up_enc(ue_context_pP->ue_context.ciphering_algorithm,
                    ue_context_pP->ue_context.kgnb,
                    &kUPenc);
  }

  derive_key_rrc_enc(ue_context_pP->ue_context.ciphering_algorithm,
                      ue_context_pP->ue_context.kgnb,
                      &kRRCenc);
  derive_key_rrc_int(ue_context_pP->ue_context.integrity_algorithm,
                      ue_context_pP->ue_context.kgnb,
                      &kRRCint);
#endif
  if (!IS_SOFTMODEM_IQPLAYER) {
    SET_LOG_DUMP(DEBUG_SECURITY) ;
  }


  if ( LOG_DUMPFLAG( DEBUG_SECURITY ) ) {
    if (print_keys == 1 ) {
      print_keys =0;
      LOG_DUMPMSG(NR_RRC, DEBUG_SECURITY, ue_context_pP->ue_context.kgnb, 32,"\nKgNB:" );
      LOG_DUMPMSG(NR_RRC, DEBUG_SECURITY, kRRCenc, 32,"\nKRRCenc:" );
      LOG_DUMPMSG(NR_RRC, DEBUG_SECURITY, kRRCint, 32,"\nKRRCint:" );
    }
  }

  key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, DCCH, SRB_FLAG_YES);
  h_rc = hashtable_get(pdcp_coll_p, key, (void **)&pdcp_p);

  if (h_rc == HASH_TABLE_OK) {
    pdcp_config_set_security(
        ctxt_pP,
        pdcp_p,
        DCCH,
        DCCH+2,
        (send_security_mode_command == TRUE)  ?
        0 | (ue_context_pP->ue_context.integrity_algorithm << 4) :
        (ue_context_pP->ue_context.ciphering_algorithm )         |
        (ue_context_pP->ue_context.integrity_algorithm << 4),
        kRRCenc,
        kRRCint,
        kUPenc);
  } else {
    LOG_E(NR_RRC,
        PROTOCOL_NR_RRC_CTXT_UE_FMT"Could not get PDCP instance for SRB DCCH %u\n",
        PROTOCOL_NR_RRC_CTXT_UE_ARGS(ctxt_pP),
        DCCH);
  }
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
  rrc_ue_ngap_ids_t  *rrc_ue_ngap_ids_p = NULL;
  // hashtable_rc_t      h_rc;

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

//------------------------------------------------------------------------------
int
rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ(
    MessageDef *msg_p,
    const char *msg_name,
    instance_t instance
)
//------------------------------------------------------------------------------
{
  uint16_t                        ue_initial_id;
  uint32_t                        gNB_ue_ngap_id;
  rrc_gNB_ue_context_t            *ue_context_p = NULL;
  protocol_ctxt_t                 ctxt;

  ue_initial_id  = NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).ue_initial_id;
  gNB_ue_ngap_id = NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).gNB_ue_ngap_id;

  ue_context_p   = rrc_gNB_get_ue_context_from_ngap_ids(instance, ue_initial_id, gNB_ue_ngap_id);
  LOG_I(NR_RRC, "[gNB %d] Received %s: ue_initial_id %d, gNB_ue_ngap_id %d \n",
      instance, msg_name, ue_initial_id, gNB_ue_ngap_id);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index, send a failure to NGAP and discard it! */
    MessageDef *msg_fail_p = NULL;
    LOG_W(NR_RRC, "[gNB %d] In NGAP_INITIAL_CONTEXT_SETUP_REQ: unknown UE from S1AP ids (%d, %d)\n", instance, ue_initial_id, gNB_ue_ngap_id);
    msg_fail_p = itti_alloc_new_message (TASK_RRC_GNB, NGAP_INITIAL_CONTEXT_SETUP_FAIL);
    NGAP_INITIAL_CONTEXT_SETUP_FAIL (msg_fail_p).gNB_ue_ngap_id = gNB_ue_ngap_id;
    // TODO add failure cause when defined!
    itti_send_msg_to_task (TASK_NGAP, instance, msg_fail_p);
    return (-1);
  } else {
    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);
    ue_context_p->ue_context.gNB_ue_ngap_id = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).gNB_ue_ngap_id;
    ue_context_p->ue_context.amf_ue_ngap_id = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).amf_ue_ngap_id;

    /* NAS PDU */
    if (ue_context_p->ue_context.nas_pdu_flag == 1) {
      ue_context_p->ue_context.nas_pdu.length = NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).nas_pdu.length;
      ue_context_p->ue_context.nas_pdu.buffer = NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).nas_pdu.buffer;
    }

    /* TODO security */
    rrc_gNB_process_security(&ctxt, ue_context_p, &(NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).security_capabilities));

    uint8_t send_security_mode_command = TRUE;

    nr_rrc_pdcp_config_security(
        &ctxt,
        ue_context_p,
        send_security_mode_command);

    if (send_security_mode_command) {
        rrc_gNB_generate_SecurityModeCommand (&ctxt, ue_context_p);
        send_security_mode_command = FALSE;

        nr_rrc_pdcp_config_security(
            &ctxt,
            ue_context_p,
            send_security_mode_command);
    } else {
        /* rrc_gNB_generate_UECapabilityEnquiry */
        rrc_gNB_generate_UECapabilityEnquiry(&ctxt, ue_context_p);
    }

    // in case, send the S1SP initial context response if it is not sent with the attach complete message
    if (ue_context_p->ue_context.Status == NR_RRC_RECONFIGURED) {
        LOG_I(NR_RRC, "Sending rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP, cause %ld\n", ue_context_p->ue_context.reestablishment_cause);
        rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP(&ctxt,ue_context_p);
    }

    return 0;
  }
}

//------------------------------------------------------------------------------
void
rrc_gNB_send_NGAP_INITIAL_CONTEXT_SETUP_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t          *const ue_context_pP
)
//------------------------------------------------------------------------------
{
  MessageDef      *msg_p         = NULL;
  int e_rab;
  int e_rabs_done = 0;
  int e_rabs_failed = 0;
  msg_p = itti_alloc_new_message (TASK_RRC_ENB, NGAP_INITIAL_CONTEXT_SETUP_RESP);
  NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).gNB_ue_ngap_id = ue_context_pP->ue_context.gNB_ue_ngap_id;

  for (e_rab = 0; e_rab < ue_context_pP->ue_context.nb_of_e_rabs; e_rab++) {
    if (ue_context_pP->ue_context.e_rab[e_rab].status == E_RAB_STATUS_DONE) {
      e_rabs_done++;
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[e_rab].pdusession_id = ue_context_pP->ue_context.e_rab[e_rab].param.e_rab_id;
      // TODO add other information from S1-U when it will be integrated
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[e_rab].gtp_teid = ue_context_pP->ue_context.gnb_gtp_teid[e_rab];
      memcpy(NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[e_rab].gNB_addr.buffer , ue_context_pP->ue_context.gnb_gtp_addrs[e_rab].buffer, 20);
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[e_rab].gNB_addr.length = 4;
      ue_context_pP->ue_context.e_rab[e_rab].status = E_RAB_STATUS_ESTABLISHED;
    } else {
      e_rabs_failed++;
      ue_context_pP->ue_context.e_rab[e_rab].status = E_RAB_STATUS_FAILED;
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions_failed[e_rab].pdusession_id = ue_context_pP->ue_context.e_rab[e_rab].param.e_rab_id;
      // TODO add cause when it will be integrated
    }
  }

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_GNB,
    MSC_S1AP_ENB,
    (const char *)&NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p),
    sizeof(ngap_initial_context_setup_resp_t),
    MSC_AS_TIME_FMT" INITIAL_CONTEXT_SETUP_RESP UE %X eNB_ue_s1ap_id %u e_rabs:%u succ %u fail",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_id_rnti,
    NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).gNB_ue_ngap_id,
    e_rabs_done, e_rabs_failed);
  NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).nb_of_pdusessions = e_rabs_done;
  NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).nb_of_pdusessions_failed = e_rabs_failed;
  itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);
}

static NR_CipheringAlgorithm_t rrc_gNB_select_ciphering(uint16_t algorithms) {
  //#warning "Forced   return NR_CipheringAlgorithm_nea0, to be deleted in future"
  return NR_CipheringAlgorithm_nea0;
}

static e_NR_IntegrityProtAlgorithm rrc_gNB_select_integrity(uint16_t algorithms) {

  // to be continue
  return NR_IntegrityProtAlgorithm_nia0;
}

int
rrc_gNB_process_security(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_gNB_ue_context_t *const ue_context_pP,
  ngap_security_capabilities_t *security_capabilities_pP
) {
  boolean_t                                             changed = FALSE;
  NR_CipheringAlgorithm_t                               cipheringAlgorithm;
  e_NR_IntegrityProtAlgorithm                           integrityProtAlgorithm;
  /* Save security parameters */
  ue_context_pP->ue_context.security_capabilities = *security_capabilities_pP;
  // translation
  LOG_D(NR_RRC,
        "[eNB %d] NAS security_capabilities.encryption_algorithms %u AS ciphering_algorithm %lu NAS security_capabilities.integrity_algorithms %u AS integrity_algorithm %u\n",
        ctxt_pP->module_id,
        ue_context_pP->ue_context.security_capabilities.nRencryption_algorithms,
        (unsigned long)ue_context_pP->ue_context.ciphering_algorithm,
        ue_context_pP->ue_context.security_capabilities.nRintegrity_algorithms,
        ue_context_pP->ue_context.integrity_algorithm);
  /* Select relevant algorithms */
  cipheringAlgorithm = rrc_gNB_select_ciphering (ue_context_pP->ue_context.security_capabilities.nRencryption_algorithms);

  if (ue_context_pP->ue_context.ciphering_algorithm != cipheringAlgorithm) {
    ue_context_pP->ue_context.ciphering_algorithm = cipheringAlgorithm;
    changed = TRUE;
  }

  integrityProtAlgorithm = rrc_gNB_select_integrity (ue_context_pP->ue_context.security_capabilities.nRintegrity_algorithms);

  if (ue_context_pP->ue_context.integrity_algorithm != integrityProtAlgorithm) {
    ue_context_pP->ue_context.integrity_algorithm = integrityProtAlgorithm;
    changed = TRUE;
  }

  LOG_I (NR_RRC, "[eNB %d][UE %x] Selected security algorithms (%p): %lx, %x, %s\n",
         ctxt_pP->module_id,
         ue_context_pP->ue_context.rnti,
         security_capabilities_pP,
         (unsigned long)cipheringAlgorithm,
         integrityProtAlgorithm,
         changed ? "changed" : "same");
  return changed;
}
