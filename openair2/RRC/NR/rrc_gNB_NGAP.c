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

#include "asn1_conversions.h"
#include "intertask_interface.h"
#include "pdcp.h"
#include "pdcp_primitives.h"

#include "gtpv1u_eNB_task.h"
#include "gtpv1u_gNB_task.h"
#include <openair3/ocp-gtpu/gtp_itf.h>
#include "RRC/LTE/rrc_eNB_GTPV1U.h"
#include "RRC/NR/rrc_gNB_GTPV1U.h"

#include "S1AP_NAS-PDU.h"
#include "executables/softmodem-common.h"
#include "UTIL/OSA/osa_defs.h"
#include "ngap_gNB_defs.h"
#include "ngap_gNB_ue_context.h"
#include "ngap_gNB_management_procedures.h"
#include "NR_ULInformationTransfer.h"
#include "RRC/NR/MESSAGES/asn1_msg.h"
#include "NR_UERadioAccessCapabilityInformation.h"
#include "NR_UE-CapabilityRAT-ContainerList.h"
#include "NGAP_Cause.h"
#include "NGAP_CauseRadioNetwork.h"
#include "f1ap_messages_types.h"

extern RAN_CONTEXT_t RC;

/* Value to indicate an invalid UE initial id */
static const uint16_t UE_INITIAL_ID_INVALID = 0;

/* Masks for NGAP Encryption algorithms, NEA0 is always supported (not coded) */
static const uint16_t NGAP_ENCRYPTION_NEA1_MASK = 0x8000;
static const uint16_t NGAP_ENCRYPTION_NEA2_MASK = 0x4000;
static const uint16_t NGAP_ENCRYPTION_NEA3_MASK = 0x2000;

/* Masks for NGAP Integrity algorithms, NIA0 is always supported (not coded) */
static const uint16_t NGAP_INTEGRITY_NIA1_MASK = 0x8000;
static const uint16_t NGAP_INTEGRITY_NIA2_MASK = 0x4000;
static const uint16_t NGAP_INTEGRITY_NIA3_MASK = 0x2000;

#define INTEGRITY_ALGORITHM_NONE NR_IntegrityProtAlgorithm_nia0

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
    rrc_ue_ngap_ids_t *result2 = NULL;
  /*****************************/
  instance_t instance = 0;
  ngap_gNB_instance_t *ngap_gNB_instance_p = NULL;
  ngap_gNB_ue_context_t *ue_desc_p = NULL;
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  /*****************************/
  hashtable_rc_t     h_rc;

  if (ue_initial_id != UE_INITIAL_ID_INVALID) {
    h_rc = hashtable_get(rrc_instance_pP->initial_id2_ngap_ids, (hash_key_t)ue_initial_id, (void **)&result);

    if (h_rc == HASH_TABLE_OK) {
      if (gNB_ue_ngap_idP > 0) {
        h_rc = hashtable_get(rrc_instance_pP->ngap_id2_ngap_ids, (hash_key_t)gNB_ue_ngap_idP, (void **)&result2);

        if (h_rc != HASH_TABLE_OK) { // this case is equivalent to associate gNB_ue_ngap_idP and ue_initial_id
          result2 = malloc(sizeof(*result2));

          if (NULL != result2) {
            *result2 = *result;
            result2->gNB_ue_ngap_id = gNB_ue_ngap_idP;
            result->gNB_ue_ngap_id  = gNB_ue_ngap_idP;
            h_rc = hashtable_insert(rrc_instance_pP->ngap_id2_ngap_ids, (hash_key_t)gNB_ue_ngap_idP, result2);

            if (h_rc != HASH_TABLE_OK) {
              LOG_E(NGAP, "[gNB %ld] Error while hashtable_insert in ngap_id2_ngap_ids gNB_ue_ngap_idP %"PRIu32"\n",
                    rrc_instance_pP - RC.nrrrc[0],
                    gNB_ue_ngap_idP);
            }
          }
        } else { // here we should check that the association was done correctly
          if ((result->ue_initial_id != result2->ue_initial_id) || (result->gNB_ue_ngap_id != result2->gNB_ue_ngap_id)) {
            LOG_E(NGAP, "[gNB %ld] Error while hashtable_get, two rrc_ue_ngap_ids_t that should be equal, are not:\n \
              ue_initial_id 1 = %"PRIu16",\n \
              ue_initial_id 2 = %"PRIu16",\n \
              gNB_ue_ngap_idP 1 = %"PRIu32",\n \
              gNB_ue_ngap_idP 2 = %"PRIu32"\n",
                  rrc_instance_pP - RC.nrrrc[0],
                  result->ue_initial_id,
                  result2->ue_initial_id,
                  result->gNB_ue_ngap_id,
                  result2->gNB_ue_ngap_id);
            // Still return *result
          }
        }
      } // end if if (gNB_ue_ngap_idP > 0)
    } else { // end if (h_rc == HASH_TABLE_OK)
      LOG_E(NGAP, "[gNB %ld] In hashtable_get, couldn't find in initial_id2_ngap_ids ue_initial_id %"PRIu16"\n",
            rrc_instance_pP - RC.nrrrc[0],
            ue_initial_id);
      return NULL;
      /*
      * At the moment this is written, this case shouldn't (cannot) happen and is equivalent to an error.
      * One could try to find the struct instance based on ngap_id2_ngap_ids and gNB_ue_ngap_idP (if > 0),
      * but this behavior is not expected at the moment.
      */
    } // end else (h_rc != HASH_TABLE_OK)
  } else { // end if (ue_initial_id != UE_INITIAL_ID_INVALID)
    if (gNB_ue_ngap_idP > 0) {
      h_rc = hashtable_get(rrc_instance_pP->ngap_id2_ngap_ids, (hash_key_t)gNB_ue_ngap_idP, (void **)&result);

      if (h_rc != HASH_TABLE_OK) {
        /*
        * This case is uncommon, but can happen when:
        * -> if the first NAS message was a Detach Request (non exhaustiv), the UE RRC context exist
        * but is not associated with gNB_ue_ngap_id
        * -> ... (?)
        */
        LOG_E(NGAP, "[gNB %ld] In hashtable_get, couldn't find in ngap_id2_ngap_ids gNB_ue_ngap_idP %"PRIu32", trying to find it through NGAP context\n",
              rrc_instance_pP - RC.nrrrc[0],
              gNB_ue_ngap_idP);
        instance = GNB_MODULE_ID_TO_INSTANCE(rrc_instance_pP - RC.nrrrc[0]); // get gNB instance
        ngap_gNB_instance_p = ngap_gNB_get_instance(instance); // get ngap_gNB_instance

        if (ngap_gNB_instance_p != NULL) {
          ue_desc_p = ngap_gNB_get_ue_context(ngap_gNB_instance_p, gNB_ue_ngap_idP); // get s1ap_eNB_ue_context
        } else {
          LOG_E(NGAP, "[gNB instance %ld] Couldn't find the gNB NGAP context\n",
                instance);
          return NULL;
        }

        if (ue_desc_p != NULL) {
          struct ngap_gNB_ue_context_s *ngap_ue_context_p = NULL;

          if ((ngap_ue_context_p = RB_REMOVE(ngap_ue_map, &ngap_gNB_instance_p->ngap_ue_head, ue_desc_p)) != NULL) {
            LOG_E(NR_RRC, "Removed UE context gNB_ue_ngap_id %u\n", ngap_ue_context_p->gNB_ue_ngap_id);
            ngap_gNB_free_ue_context(ngap_ue_context_p);
          } else {
            LOG_E(NR_RRC, "Removing UE context gNB_ue_ngap_id %u: did not find context\n",ue_desc_p->gNB_ue_ngap_id);
          }

          return NULL; //skip the operation below to avoid loop
          result = rrc_gNB_NGAP_get_ue_ids(rrc_instance_pP, ue_desc_p->ue_initial_id, gNB_ue_ngap_idP);

          if (ue_desc_p->ue_initial_id != UE_INITIAL_ID_INVALID) {
            result = rrc_gNB_NGAP_get_ue_ids(rrc_instance_pP, ue_desc_p->ue_initial_id, gNB_ue_ngap_idP);

            if (result != NULL) {
              ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[GNB_INSTANCE_TO_MODULE_ID(instance)], result->ue_rnti);

              if ((ue_context_p != NULL) && (ue_context_p->ue_context.gNB_ue_ngap_id == 0)) {
                ue_context_p->ue_context.gNB_ue_ngap_id = gNB_ue_ngap_idP;
              } else {
                LOG_E(NR_RRC, "[gNB %ld] Incoherence between RRC context and NGAP context (%d != %d) for UE RNTI %d or UE RRC context doesn't exist\n",
                      rrc_instance_pP - RC.nrrrc[0],
                      (ue_context_p==NULL)?99999:ue_context_p->ue_context.gNB_ue_ngap_id,
                      gNB_ue_ngap_idP,
                      result->ue_rnti);
              }
            }
          } else {
            LOG_E(NGAP, "[gNB %ld] NGAP context found but ue_initial_id is invalid (0)\n", rrc_instance_pP - RC.nrrrc[0]);
            return NULL;
          }
        } else {
          LOG_E(NGAP, "[gNB %ld] In hashtable_get, couldn't find in ngap_id2_ngap_ids gNB_ue_ngap_idP %"PRIu32", because ue_initial_id is invalid in NGAP context\n",
                rrc_instance_pP - RC.nrrrc[0],
                gNB_ue_ngap_idP);
          return NULL;
        }
      } // end if (h_rc != HASH_TABLE_OK)
    } // end if (gNB_ue_ngap_idP > 0)
  } // end else (ue_initial_id == UE_INITIAL_ID_INVALID)

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
  /* Saves the security key */
  memcpy (ue_context_pP->ue_context.kgnb, security_key_pP, SECURITY_KEY_LENGTH);
  memset (ue_context_pP->ue_context.nh, 0, SECURITY_KEY_LENGTH);
  ue_context_pP->ue_context.nh_ncc = -1;

  for (i = 0; i < 32; i++) {
    sprintf(&ascii_buffer[2 * i], "%02X", ue_context_pP->ue_context.kgnb[i]);
  }

  ascii_buffer[2 * i] = '\0';
  LOG_I(NR_RRC, "[gNB %d][UE %x] Saved security key %s\n", ctxt_pP->module_id, ue_context_pP->ue_context.rnti, ascii_buffer);
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
  NR_SRB_ToAddModList_t              *SRB_configList = ue_context_pP->ue_context.SRB_configList;
  (void)SRB_configList;
  uint8_t                            *kRRCenc = NULL;
  uint8_t                            *kRRCint = NULL;
  uint8_t                            *kUPenc = NULL;
  //uint8_t                            *k_kdf  = NULL;
  static int                          print_keys= 1;


  /* Derive the keys from kgnb */
  if (SRB_configList != NULL) {
    nr_derive_key_up_enc(ue_context_pP->ue_context.ciphering_algorithm,
                         ue_context_pP->ue_context.kgnb,
                         &kUPenc);
  }

  nr_derive_key_rrc_enc(ue_context_pP->ue_context.ciphering_algorithm,
                        ue_context_pP->ue_context.kgnb,
                        &kRRCenc);
  nr_derive_key_rrc_int(ue_context_pP->ue_context.integrity_algorithm,
                        ue_context_pP->ue_context.kgnb,
                        &kRRCint);
  if (!IS_SOFTMODEM_IQPLAYER) {
    SET_LOG_DUMP(DEBUG_SECURITY) ;
  }


  if ( LOG_DUMPFLAG( DEBUG_SECURITY ) ) {
    if (print_keys == 1 ) {
      print_keys =0;
      LOG_DUMPMSG(NR_RRC, DEBUG_SECURITY, ue_context_pP->ue_context.kgnb, 32,"\nKgNB:" );
      LOG_DUMPMSG(NR_RRC, DEBUG_SECURITY, kRRCenc, 16,"\nKRRCenc:" );
      LOG_DUMPMSG(NR_RRC, DEBUG_SECURITY, kRRCint, 16,"\nKRRCint:" );
    }
  }

  pdcp_config_set_security(
      ctxt_pP,
      NULL,      /* pdcp_pP not used anymore in NR */
      DCCH,
      DCCH+2,
      enable_ciphering ?
             ue_context_pP->ue_context.ciphering_algorithm
          | (ue_context_pP->ue_context.integrity_algorithm << 4)
        :    0
          | (ue_context_pP->ue_context.integrity_algorithm << 4),
      kRRCenc,
      kRRCint,
      kUPenc);
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
  hashtable_rc_t      h_rc;

  message_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_NAS_FIRST_REQ);
  memset(&message_p->ittiMsg.ngap_nas_first_req, 0, sizeof(ngap_nas_first_req_t));
  ue_context_pP->ue_context.ue_initial_id = get_next_ue_initial_id(ctxt_pP->module_id);
  NGAP_NAS_FIRST_REQ(message_p).ue_initial_id = ue_context_pP->ue_context.ue_initial_id;
  rrc_ue_ngap_ids_p = malloc(sizeof(rrc_ue_ngap_ids_t));
  rrc_ue_ngap_ids_p->ue_initial_id  = ue_context_pP->ue_context.ue_initial_id;
  rrc_ue_ngap_ids_p->gNB_ue_ngap_id = UE_INITIAL_ID_INVALID;
  rrc_ue_ngap_ids_p->ue_rnti        = ctxt_pP->rnti;

  h_rc = hashtable_insert(RC.nrrrc[ctxt_pP->module_id]->initial_id2_ngap_ids,
                          (hash_key_t)ue_context_pP->ue_context.ue_initial_id,
                          rrc_ue_ngap_ids_p);

  if (h_rc != HASH_TABLE_OK) {
    LOG_E(NGAP, "[gNB %d] Error while hashtable_insert in initial_id2_ngap_ids ue_initial_id %u\n",
          ctxt_pP->module_id,
          ue_context_pP->ue_context.ue_initial_id);
  }

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
    protocol_ctxt_t                 ctxt={0};
    uint8_t                         pdu_sessions_done = 0;
    gtpv1u_gnb_create_tunnel_req_t  create_tunnel_req;
    gtpv1u_gnb_create_tunnel_resp_t create_tunnel_resp;
    uint8_t                         inde_list[NR_NB_RB_MAX - 3]= {0};
    int                             ret = 0;

    ue_initial_id  = NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).ue_initial_id;
    gNB_ue_ngap_id = NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).gNB_ue_ngap_id;

    ue_context_p   = rrc_gNB_get_ue_context_from_ngap_ids(instance, ue_initial_id, gNB_ue_ngap_id);
    LOG_I(NR_RRC, "[gNB %ld] Received %s: ue_initial_id %d, gNB_ue_ngap_id %u \n",
        instance, msg_name, ue_initial_id, gNB_ue_ngap_id);

    if (ue_context_p == NULL) {
      /* Can not associate this message to an UE index, send a failure to NGAP and discard it! */
      MessageDef *msg_fail_p = NULL;
      LOG_W(NR_RRC, "[gNB %ld] In NGAP_INITIAL_CONTEXT_SETUP_REQ: unknown UE from NGAP ids (%d, %u)\n", instance, ue_initial_id, gNB_ue_ngap_id);
      msg_fail_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_INITIAL_CONTEXT_SETUP_FAIL);
      NGAP_INITIAL_CONTEXT_SETUP_FAIL (msg_fail_p).gNB_ue_ngap_id = gNB_ue_ngap_id;
      // TODO add failure cause when defined!
      itti_send_msg_to_task (TASK_NGAP, instance, msg_fail_p);
      return (-1);
    } else {
      PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);
      ue_context_p->ue_context.gNB_ue_ngap_id = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).gNB_ue_ngap_id;
      ue_context_p->ue_context.amf_ue_ngap_id = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).amf_ue_ngap_id;
      ue_context_p->ue_context.nas_pdu_flag = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).nas_pdu_flag;

      uint8_t nb_pdusessions_tosetup = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).nb_of_pdusessions;
      if (nb_pdusessions_tosetup != 0) {
        memset(&create_tunnel_req, 0, sizeof(gtpv1u_gnb_create_tunnel_req_t));
        for (int i = 0; i < NR_NB_RB_MAX - 3; i++) {
          if(ue_context_p->ue_context.pduSession[i].status >= PDU_SESSION_STATUS_DONE)
            continue;
          ue_context_p->ue_context.pduSession[i].status        = PDU_SESSION_STATUS_NEW;
          ue_context_p->ue_context.pduSession[i].param         = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).pdusession_param[pdu_sessions_done];
          create_tunnel_req.pdusession_id[pdu_sessions_done]   = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).pdusession_param[pdu_sessions_done].pdusession_id;
          create_tunnel_req.incoming_rb_id[pdu_sessions_done]  = i+1;
          create_tunnel_req.outgoing_teid[pdu_sessions_done]    = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).pdusession_param[pdu_sessions_done].gtp_teid;
          create_tunnel_req.dst_addr[pdu_sessions_done].length = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).pdusession_param[pdu_sessions_done].upf_addr.length;
          memcpy(create_tunnel_req.dst_addr[pdu_sessions_done].buffer,
                  NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).pdusession_param[pdu_sessions_done].upf_addr.buffer,
                  sizeof(uint8_t)*20);
          LOG_I(NR_RRC, "PDUSESSION SETUP: local index %d teid %u, pdusession id %d \n",
                i,
                create_tunnel_req.outgoing_teid[pdu_sessions_done],
                create_tunnel_req.pdusession_id[pdu_sessions_done]);
          inde_list[pdu_sessions_done] = i;
          pdu_sessions_done++;

          if(pdu_sessions_done >= nb_pdusessions_tosetup) {
            break;
          }
        }

        ue_context_p->ue_context.nb_of_pdusessions = NGAP_INITIAL_CONTEXT_SETUP_REQ (msg_p).nb_of_pdusessions;
        create_tunnel_req.rnti                     = ue_context_p->ue_context.rnti;
        create_tunnel_req.num_tunnels              = pdu_sessions_done;

        ret = gtpv1u_create_ngu_tunnel(
                instance,
                &create_tunnel_req,
                &create_tunnel_resp);
        if (ret != 0) {
          LOG_E(NR_RRC,"rrc_gNB_process_NGAP_INITIAL_CONTEXT_SETUP_REQ : gtpv1u_create_ngu_tunnel failed,start to release UE %x\n",ue_context_p->ue_context.rnti);
          ue_context_p->ue_context.ue_release_timer_ng = 1;
          ue_context_p->ue_context.ue_release_timer_thres_ng = 100;
          ue_context_p->ue_context.ue_release_timer = 0;
          ue_context_p->ue_context.ue_reestablishment_timer = 0;
          ue_context_p->ue_context.ul_failure_timer = 20000;
          ue_context_p->ue_context.ul_failure_timer = 0;
          return (0);
        }

        nr_rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(
          &ctxt,
          &create_tunnel_resp,
          &inde_list[0]);
        ue_context_p->ue_context.setup_pdu_sessions += nb_pdusessions_tosetup;
        ue_context_p->ue_context.established_pdu_sessions_flag = 1;
      }

      /* NAS PDU */
      if (NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).nas_pdu_flag == 1) {
        ue_context_p->ue_context.nas_pdu_flag   = NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).nas_pdu_flag;
        ue_context_p->ue_context.nas_pdu.length = NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).nas_pdu.length;
        ue_context_p->ue_context.nas_pdu.buffer = NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).nas_pdu.buffer;
      }
        
      /* security */
      rrc_gNB_process_security(&ctxt, ue_context_p, &(NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).security_capabilities));
      process_gNB_security_key (
        &ctxt,
        ue_context_p,
        NGAP_INITIAL_CONTEXT_SETUP_REQ(msg_p).security_key);

      /* configure only integrity, ciphering comes after receiving SecurityModeComplete */
      nr_rrc_pdcp_config_security(&ctxt, ue_context_p, 0);

      rrc_gNB_generate_SecurityModeCommand (&ctxt, ue_context_p);

    // in case, send the S1SP initial context response if it is not sent with the attach complete message
    if (ue_context_p->ue_context.StatusRrc == NR_RRC_RECONFIGURED) {
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
  int pdusession;
  int pdu_sessions_done = 0;
  int pdu_sessions_failed = 0;
  int qos_flow_index = 0;
  msg_p = itti_alloc_new_message (TASK_RRC_ENB, 0, NGAP_INITIAL_CONTEXT_SETUP_RESP);
  NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).gNB_ue_ngap_id = ue_context_pP->ue_context.gNB_ue_ngap_id;

  for (pdusession = 0; pdusession < ue_context_pP->ue_context.nb_of_pdusessions; pdusession++) {
    if (ue_context_pP->ue_context.pduSession[pdusession].status == PDU_SESSION_STATUS_DONE) {
      pdu_sessions_done++;
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[pdusession].pdusession_id = ue_context_pP->ue_context.pduSession[pdusession].param.pdusession_id;
      // TODO add other information from S1-U when it will be integrated
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[pdusession].gtp_teid = ue_context_pP->ue_context.gnb_gtp_teid[pdusession];
      memcpy(NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[pdusession].gNB_addr.buffer , ue_context_pP->ue_context.gnb_gtp_addrs[pdusession].buffer, 20);
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[pdusession].gNB_addr.length = 4;
      ue_context_pP->ue_context.pduSession[pdusession].status = PDU_SESSION_STATUS_ESTABLISHED;
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[pdusession].nb_of_qos_flow = ue_context_pP->ue_context.pduSession[pdusession].param.nb_qos;
      for (qos_flow_index = 0; qos_flow_index < ue_context_pP->ue_context.pduSession[pdusession].param.nb_qos; qos_flow_index++) {
        NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[pdusession].associated_qos_flows[qos_flow_index].qfi =
            ue_context_pP->ue_context.pduSession[pdusession].param.qos[qos_flow_index].qfi;
        NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions[pdusession].associated_qos_flows[qos_flow_index].qos_flow_mapping_ind = QOSFLOW_MAPPING_INDICATION_DL;
      }
    } else {
      pdu_sessions_failed++;
      ue_context_pP->ue_context.pduSession[pdusession].status = PDU_SESSION_STATUS_FAILED;
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions_failed[pdusession].pdusession_id = ue_context_pP->ue_context.pduSession[pdusession].param.pdusession_id;
      // TODO add cause when it will be integrated
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions_failed[pdusession].cause = NGAP_Cause_PR_radioNetwork;
      NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).pdusessions_failed[pdusession].cause_value = NGAP_CauseRadioNetwork_unknown_PDU_session_ID;
    }
  }

  NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).nb_of_pdusessions = pdu_sessions_done;
  NGAP_INITIAL_CONTEXT_SETUP_RESP (msg_p).nb_of_pdusessions_failed = pdu_sessions_failed;
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

  LOG_I(RRC, "selecting ciphering algorithm %d\n", ret);

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

  LOG_I(RRC, "selecting integrity algorithm %d\n", ret);

  return ret;
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
        "[gNB %d] NAS security_capabilities.encryption_algorithms %u AS ciphering_algorithm %lu NAS security_capabilities.integrity_algorithms %u AS integrity_algorithm %u\n",
        ctxt_pP->module_id,
        ue_context_pP->ue_context.security_capabilities.nRencryption_algorithms,
        (unsigned long)ue_context_pP->ue_context.ciphering_algorithm,
        ue_context_pP->ue_context.security_capabilities.nRintegrity_algorithms,
        ue_context_pP->ue_context.integrity_algorithm);
  /* Select relevant algorithms */
  cipheringAlgorithm = rrc_gNB_select_ciphering(ctxt_pP, ue_context_pP->ue_context.security_capabilities.nRencryption_algorithms);

  if (ue_context_pP->ue_context.ciphering_algorithm != cipheringAlgorithm) {
    ue_context_pP->ue_context.ciphering_algorithm = cipheringAlgorithm;
    changed = TRUE;
  }

  integrityProtAlgorithm = rrc_gNB_select_integrity(ctxt_pP, ue_context_pP->ue_context.security_capabilities.nRintegrity_algorithms);

  if (ue_context_pP->ue_context.integrity_algorithm != integrityProtAlgorithm) {
    ue_context_pP->ue_context.integrity_algorithm = integrityProtAlgorithm;
    changed = TRUE;
  }

  LOG_I (NR_RRC, "[gNB %d][UE %x] Selected security algorithms (%p): %lx, %x, %s\n",
         ctxt_pP->module_id,
         ue_context_pP->ue_context.rnti,
         security_capabilities_pP,
         (unsigned long)cipheringAlgorithm,
         integrityProtAlgorithm,
         changed ? "changed" : "same");
  return changed;
}

//------------------------------------------------------------------------------
int
rrc_gNB_process_NGAP_DOWNLINK_NAS(
  MessageDef *msg_p,
  const char *msg_name,
  instance_t  instance,
  mui_t      *rrc_gNB_mui
)
//------------------------------------------------------------------------------
{
    uint16_t ue_initial_id;
    uint32_t gNB_ue_ngap_id;
    uint32_t length;
    uint8_t *buffer;
    struct rrc_gNB_ue_context_s *ue_context_p = NULL;
    protocol_ctxt_t              ctxt;
    memset(&ctxt, 0, sizeof(protocol_ctxt_t));
    ue_initial_id  = NGAP_DOWNLINK_NAS (msg_p).ue_initial_id;
    gNB_ue_ngap_id = NGAP_DOWNLINK_NAS (msg_p).gNB_ue_ngap_id;
    ue_context_p = rrc_gNB_get_ue_context_from_ngap_ids(instance, ue_initial_id, gNB_ue_ngap_id);
    LOG_I(NR_RRC, "[gNB %ld] Received %s: ue_initial_id %d, gNB_ue_ngap_id %u\n",
            instance,
            msg_name,
            ue_initial_id,
            gNB_ue_ngap_id);

    if (ue_context_p == NULL) {
        /* Can not associate this message to an UE index, send a failure to NGAP and discard it! */
        MessageDef *msg_fail_p;
        LOG_W(NR_RRC, "[gNB %ld] In NGAP_DOWNLINK_NAS: unknown UE from NGAP ids (%d, %u)\n", instance, ue_initial_id, gNB_ue_ngap_id);
        msg_fail_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_NAS_NON_DELIVERY_IND);
        NGAP_NAS_NON_DELIVERY_IND (msg_fail_p).gNB_ue_ngap_id = gNB_ue_ngap_id;
        NGAP_NAS_NON_DELIVERY_IND (msg_fail_p).nas_pdu.length = NGAP_DOWNLINK_NAS (msg_p).nas_pdu.length;
        NGAP_NAS_NON_DELIVERY_IND (msg_fail_p).nas_pdu.buffer = NGAP_DOWNLINK_NAS (msg_p).nas_pdu.buffer;
        // TODO add failure cause when defined!
        itti_send_msg_to_task (TASK_NGAP, instance, msg_fail_p);
        return (-1);
    } else {
        PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);

        /* Is it the first income from NGAP ? */
        if (ue_context_p->ue_context.gNB_ue_ngap_id == 0) {
            ue_context_p->ue_context.gNB_ue_ngap_id = NGAP_DOWNLINK_NAS (msg_p).gNB_ue_ngap_id;
        }

        /* Create message for PDCP (DLInformationTransfer_t) */
        length = do_NR_DLInformationTransfer (
                instance,
                &buffer,
                rrc_gNB_get_next_transaction_identifier (instance),
                NGAP_DOWNLINK_NAS (msg_p).nas_pdu.length,
                NGAP_DOWNLINK_NAS (msg_p).nas_pdu.buffer);
        LOG_DUMPMSG(NR_RRC, DEBUG_RRC, buffer, length, "[MSG] RRC DL Information Transfer\n");
        /*
        * switch UL or DL NAS message without RRC piggybacked to SRB2 if active.
        */
       switch (RC.nrrrc[ctxt.module_id]->node_type) {
        case ngran_gNB_CU:
          /* Transfer data to PDCP */
          nr_rrc_data_req (
              &ctxt,
              ue_context_p->ue_context.Srb2.Active == 1 ? ue_context_p->ue_context.Srb2.Srb_info.Srb_id : ue_context_p->ue_context.Srb1.Srb_info.Srb_id,
              (*rrc_gNB_mui)++,
              SDU_CONFIRM_NO,
              length,
              buffer,
              PDCP_TRANSMISSION_MODE_CONTROL);
          break;

        case ngran_gNB_DU:
          // nothing to do for DU
          AssertFatal(1==0,"nothing to do for DU\n");
          break;

        case ngran_gNB:
        {
          // rrc_mac_config_req_gNB
#ifdef ITTI_SIM
        uint8_t *message_buffer;
        message_buffer = itti_malloc (TASK_RRC_GNB, TASK_RRC_UE_SIM, length);
        memcpy (message_buffer, buffer, length);
        MessageDef *message_p = itti_alloc_new_message (TASK_RRC_GNB, 0, GNB_RRC_DCCH_DATA_IND);
        GNB_RRC_DCCH_DATA_IND (message_p).rbid = DCCH;
        GNB_RRC_DCCH_DATA_IND (message_p).sdu = message_buffer;
        GNB_RRC_DCCH_DATA_IND (message_p).size  = length;
        itti_send_msg_to_task (TASK_RRC_UE_SIM, instance, message_p);
        LOG_I(NR_RRC, "Send DL NAS message \n");
#else
          /* Transfer data to PDCP */
          nr_rrc_data_req (
              &ctxt,
              ue_context_p->ue_context.Srb2.Active == 1 ? ue_context_p->ue_context.Srb2.Srb_info.Srb_id : ue_context_p->ue_context.Srb1.Srb_info.Srb_id,
              (*rrc_gNB_mui)++,
              SDU_CONFIRM_NO,
              length,
              buffer,
              PDCP_TRANSMISSION_MODE_CONTROL);
#endif
        }
          break;

        default :
            LOG_W(NR_RRC, "Unknown node type %d\n", RC.nrrrc[ctxt.module_id]->node_type);
      }
        return (0);
    }
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
    uint32_t pdu_length;
    uint8_t *pdu_buffer;
    MessageDef *msg_p;
    NR_ULInformationTransfer_t *ulInformationTransfer = ul_dcch_msg->message.choice.c1->choice.ulInformationTransfer;

    if (ulInformationTransfer->criticalExtensions.present == NR_ULInformationTransfer__criticalExtensions_PR_ulInformationTransfer) {
        pdu_length = ulInformationTransfer->criticalExtensions.choice.ulInformationTransfer->dedicatedNAS_Message->size;
        pdu_buffer = ulInformationTransfer->criticalExtensions.choice.ulInformationTransfer->dedicatedNAS_Message->buf;
        msg_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_UPLINK_NAS);
        NGAP_UPLINK_NAS (msg_p).gNB_ue_ngap_id = ue_context_pP->ue_context.gNB_ue_ngap_id;
        NGAP_UPLINK_NAS (msg_p).nas_pdu.length = pdu_length;
        NGAP_UPLINK_NAS (msg_p).nas_pdu.buffer = pdu_buffer;
        // extract_imsi(NGAP_UPLINK_NAS (msg_p).nas_pdu.buffer,
        //               NGAP_UPLINK_NAS (msg_p).nas_pdu.length,
        //               ue_context_pP);
        itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);
        LOG_I(NR_RRC,"Send RRC GNB UL Information Transfer \n");
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
  int pdusession;
  int qos_flow_index;

  msg_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_PDUSESSION_SETUP_RESP);
  NGAP_PDUSESSION_SETUP_RESP(msg_p).gNB_ue_ngap_id = ue_context_pP->ue_context.gNB_ue_ngap_id;

  for (pdusession = 0; pdusession < ue_context_pP->ue_context.setup_pdu_sessions; pdusession++) {
    // if (xid == ue_context_pP->ue_context.pdusession[pdusession].xid) {
      if (ue_context_pP->ue_context.pduSession[pdusession].status == PDU_SESSION_STATUS_DONE) {
        pdusession_setup_t * tmp=&NGAP_PDUSESSION_SETUP_RESP(msg_p).pdusessions[pdu_sessions_done];
        tmp->pdusession_id = ue_context_pP->ue_context.pduSession[pdusession].param.pdusession_id;
        // tmp->pdusession_id = 1;
        tmp->nb_of_qos_flow = ue_context_pP->ue_context.pduSession[pdusession].param.nb_qos;
        tmp->gtp_teid = ue_context_pP->ue_context.gnb_gtp_teid[pdusession];
        tmp->gNB_addr.pdu_session_type = PDUSessionType_ipv4;
        tmp->gNB_addr.length = ue_context_pP->ue_context.gnb_gtp_addrs[pdusession].length;
        memcpy(tmp->gNB_addr.buffer,
               ue_context_pP->ue_context.gnb_gtp_addrs[pdusession].buffer, tmp->gNB_addr.length);
        for (qos_flow_index = 0; qos_flow_index < tmp->nb_of_qos_flow; qos_flow_index++) {
          tmp->associated_qos_flows[qos_flow_index].qfi =
            ue_context_pP->ue_context.pduSession[pdusession].param.qos[qos_flow_index].qfi;
          tmp->associated_qos_flows[qos_flow_index].qos_flow_mapping_ind = QOSFLOW_MAPPING_INDICATION_DL;
        }

        ue_context_pP->ue_context.pduSession[pdusession].status = PDU_SESSION_STATUS_ESTABLISHED;
        LOG_I (NR_RRC,"gnb_gtp_addr (msg index %d, pdu_sessions index %d, status %d, xid %d): nb_of_pdusessions %d,  pdusession_id %d, teid: %u \n ",
               pdu_sessions_done, pdusession, ue_context_pP->ue_context.pduSession[pdusession].status, xid,
               ue_context_pP->ue_context.nb_of_pdusessions,
               NGAP_PDUSESSION_SETUP_RESP (msg_p).pdusessions[pdu_sessions_done].pdusession_id,
               NGAP_PDUSESSION_SETUP_RESP (msg_p).pdusessions[pdu_sessions_done].gtp_teid);
        pdu_sessions_done++;
      } else if ((ue_context_pP->ue_context.pduSession[pdusession].status == PDU_SESSION_STATUS_NEW) ||
                 (ue_context_pP->ue_context.pduSession[pdusession].status == PDU_SESSION_STATUS_ESTABLISHED)) {
        LOG_D (NR_RRC,"PDU-SESSION is NEW or already ESTABLISHED\n");
      } else { /* to be improved */
        ue_context_pP->ue_context.pduSession[pdusession].status = PDU_SESSION_STATUS_FAILED;
        NGAP_PDUSESSION_SETUP_RESP (msg_p).pdusessions_failed[pdu_sessions_failed].pdusession_id = ue_context_pP->ue_context.pduSession[pdusession].param.pdusession_id;
        pdu_sessions_failed++;
        // TODO add cause when it will be integrated
      }
        NGAP_PDUSESSION_SETUP_RESP(msg_p).nb_of_pdusessions = pdu_sessions_done;
        NGAP_PDUSESSION_SETUP_RESP(msg_p).nb_of_pdusessions_failed = pdu_sessions_failed;
    // } else {
    //   LOG_D(NR_RRC,"xid does not corresponds  (context pdu_sessions index %d, status %d, xid %d/%d) \n ",
    //         pdusession, ue_context_pP->ue_context.pdusession[pdusession].status, xid, ue_context_pP->ue_context.pdusession[pdusession].xid);
    // }
  }

  if ((pdu_sessions_done > 0) ) {
    LOG_I(NR_RRC,"NGAP_PDUSESSION_SETUP_RESP: sending the message: nb_of_pdusessions %d, total pdu_sessions %d, index %d\n",
          ue_context_pP->ue_context.nb_of_pdusessions, ue_context_pP->ue_context.setup_pdu_sessions, pdusession);
    itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);
  }

  for(int i = 0; i < NB_RB_MAX; i++) {
    ue_context_pP->ue_context.pduSession[i].xid = -1;
  }

  return;
}

//------------------------------------------------------------------------------
int
rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ(
  MessageDef *msg_p,
  const char *msg_name,
  instance_t instance
)
//------------------------------------------------------------------------------
{
  uint16_t                        ue_initial_id;
  uint32_t                        gNB_ue_ngap_id;
  rrc_gNB_ue_context_t            *ue_context_p = NULL;
  protocol_ctxt_t                 ctxt={0};
  gtpv1u_gnb_create_tunnel_req_t  create_tunnel_req={0};
  gtpv1u_gnb_create_tunnel_resp_t create_tunnel_resp={0};
  uint8_t                         pdu_sessions_done;
  uint8_t                         inde_list[NR_NB_RB_MAX - 3]= {0};
  int                             ret = 0;

  ue_initial_id  = NGAP_PDUSESSION_SETUP_REQ(msg_p).ue_initial_id;
  gNB_ue_ngap_id = NGAP_PDUSESSION_SETUP_REQ(msg_p).gNB_ue_ngap_id;
  ue_context_p   = rrc_gNB_get_ue_context_from_ngap_ids(instance, ue_initial_id, gNB_ue_ngap_id);
  LOG_I(NR_RRC, "[gNB %ld] Received %s: ue_initial_id %d, gNB_ue_ngap_id %u \n",
    instance, msg_name, ue_initial_id, gNB_ue_ngap_id);

  if (ue_context_p == NULL) {
    MessageDef *msg_fail_p = NULL;
    LOG_W(NR_RRC, "[gNB %ld] In NGAP_PDUSESSION_SETUP_REQ: unknown UE from NGAP ids (%d, %u)\n", instance, ue_initial_id, gNB_ue_ngap_id);
    msg_fail_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_PDUSESSION_SETUP_REQUEST_FAIL);
    NGAP_PDUSESSION_SETUP_REQ(msg_fail_p).gNB_ue_ngap_id = gNB_ue_ngap_id;
    // TODO add failure cause when defined!
    itti_send_msg_to_task (TASK_NGAP, instance, msg_fail_p);
    return (-1);
  } else {
    memset(&create_tunnel_req, 0, sizeof(gtpv1u_gnb_create_tunnel_req_t));
    uint8_t nb_pdusessions_tosetup = NGAP_PDUSESSION_SETUP_REQ(msg_p).nb_pdusessions_tosetup;
    pdu_sessions_done = 0;

    PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, GNB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0, 0);
    for (int i = 0; i < NR_NB_RB_MAX - 3; i++) {
      if(ue_context_p->ue_context.pduSession[i].status >= PDU_SESSION_STATUS_DONE)
        continue;
      ue_context_p->ue_context.pduSession[i].status      = PDU_SESSION_STATUS_NEW;
      ue_context_p->ue_context.pduSession[i].param       = NGAP_PDUSESSION_SETUP_REQ(msg_p).pdusession_setup_params[pdu_sessions_done];
      create_tunnel_req.pdusession_id[pdu_sessions_done] = NGAP_PDUSESSION_SETUP_REQ(msg_p).pdusession_setup_params[pdu_sessions_done].pdusession_id;
      create_tunnel_req.incoming_rb_id[pdu_sessions_done]= i+1;
      create_tunnel_req.outgoing_teid[pdu_sessions_done]  = NGAP_PDUSESSION_SETUP_REQ(msg_p).pdusession_setup_params[pdu_sessions_done].gtp_teid;
      memcpy(create_tunnel_req.dst_addr[pdu_sessions_done].buffer,
              NGAP_PDUSESSION_SETUP_REQ(msg_p).pdusession_setup_params[pdu_sessions_done].upf_addr.buffer,
              sizeof(uint8_t)*20);
      create_tunnel_req.dst_addr[pdu_sessions_done].length = NGAP_PDUSESSION_SETUP_REQ(msg_p).pdusession_setup_params[pdu_sessions_done].upf_addr.length;
      LOG_I(NR_RRC,"NGAP PDUSESSION SETUP REQ: local index %d teid %u, pdusession id %d \n",
            i,
            create_tunnel_req.outgoing_teid[pdu_sessions_done],
            create_tunnel_req.pdusession_id[pdu_sessions_done]);
      inde_list[pdu_sessions_done] = i;
      pdu_sessions_done++;

      if(pdu_sessions_done >= nb_pdusessions_tosetup) {
        break;
      }
    }

    ue_context_p->ue_context.nb_of_pdusessions = NGAP_PDUSESSION_SETUP_REQ(msg_p).nb_pdusessions_tosetup;
    ue_context_p->ue_context.gNB_ue_ngap_id    = NGAP_PDUSESSION_SETUP_REQ(msg_p).gNB_ue_ngap_id;
    ue_context_p->ue_context.amf_ue_ngap_id    = NGAP_PDUSESSION_SETUP_REQ(msg_p).amf_ue_ngap_id;
    create_tunnel_req.rnti                     = ue_context_p->ue_context.rnti;
    create_tunnel_req.num_tunnels              = pdu_sessions_done;

    ret = gtpv1u_create_ngu_tunnel(
            instance,
            &create_tunnel_req,
            &create_tunnel_resp);
    if (ret != 0) {
      LOG_E(NR_RRC,"rrc_gNB_process_NGAP_PDUSESSION_SETUP_REQ : gtpv1u_create_ngu_tunnel failed,start to release UE %x\n",ue_context_p->ue_context.rnti);
      ue_context_p->ue_context.ue_release_timer_ng = 1;
      ue_context_p->ue_context.ue_release_timer_thres_ng = 100;
      ue_context_p->ue_context.ue_release_timer = 0;
      ue_context_p->ue_context.ue_reestablishment_timer = 0;
      ue_context_p->ue_context.ul_failure_timer = 20000; // set ul_failure to 20000 for triggering rrc_eNB_send_S1AP_UE_CONTEXT_RELEASE_REQ
      // rrc_gNB_free_UE(ctxt.module_id,ue_context_p);
      ue_context_p->ue_context.ul_failure_timer = 0;
      return (0);
    }
    nr_rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(
      &ctxt,
      &create_tunnel_resp,
      &inde_list[0]);
    ue_context_p->ue_context.setup_pdu_sessions += nb_pdusessions_tosetup;

    // TEST 
    // ue_context_p->ue_context.pdusession[0].status = PDU_SESSION_STATUS_DONE;
    // rrc_gNB_send_NGAP_PDUSESSION_SETUP_RESP(&ctxt, ue_context_p, 0);
    rrc_gNB_generate_dedicatedRRCReconfiguration(&ctxt, ue_context_p);
    return(0);
  }
}

//------------------------------------------------------------------------------
int
rrc_gNB_process_NGAP_PDUSESSION_MODIFY_REQ(
  MessageDef *msg_p,
  const char *msg_name,
  instance_t instance
)
//------------------------------------------------------------------------------
{
  uint16_t                         ue_initial_id;
  uint32_t                         gNB_ue_ngap_id;
  uint8_t                          nb_pdusessions_tomodify;
  rrc_gNB_ue_context_t            *ue_context_p = NULL;
  uint8_t                          i;
  uint8_t                          qos_flow_index;
  protocol_ctxt_t                  ctxt;

  ue_initial_id  = NGAP_PDUSESSION_MODIFY_REQ(msg_p).ue_initial_id;
  gNB_ue_ngap_id = NGAP_PDUSESSION_MODIFY_REQ(msg_p).gNB_ue_ngap_id;
  nb_pdusessions_tomodify = NGAP_PDUSESSION_MODIFY_REQ(msg_p).nb_pdusessions_tomodify;

  ue_context_p = rrc_gNB_get_ue_context_from_ngap_ids(instance, ue_initial_id, gNB_ue_ngap_id);
  if (ue_context_p == NULL) {
    LOG_W(NR_RRC, "[gNB %ld] In NGAP_PDUSESSION_MODIFY_REQ: unknown UE from NGAP ids (%d, %u)\n", instance, ue_initial_id, gNB_ue_ngap_id);
    // TODO 
    return (-1);
  } else {
    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);
    ctxt.eNB_index = 0;

    ue_context_p->ue_context.gNB_ue_ngap_id = gNB_ue_ngap_id;
    {
      int j;
      boolean_t is_treated[NGAP_MAX_PDUSESSION] = {FALSE};
      uint8_t nb_of_failed_pdusessions = 0;

      for (i = 0; i < nb_pdusessions_tomodify; i++) {
        if (is_treated[i] == TRUE) {
          continue;
        }
        
        //Check if same PDU session ID to handle multiple pdu sessions
        for (j = i+1; j < nb_pdusessions_tomodify; j++) {
          if (is_treated[j] == FALSE &&
              NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[j].pdusession_id == 
                NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].pdusession_id) {
            // handle multiple pdu session id
            LOG_D(NR_RRC, "handle multiple pdu session id \n");
            ue_context_p->ue_context.modify_pdusession[j].status              = PDU_SESSION_STATUS_NEW;
            ue_context_p->ue_context.modify_pdusession[j].param.pdusession_id = 
              NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[j].pdusession_id;
            ue_context_p->ue_context.modify_pdusession[j].cause               = NGAP_CAUSE_RADIO_NETWORK;
            ue_context_p->ue_context.modify_pdusession[j].cause_value         = NGAP_CauseRadioNetwork_multiple_PDU_session_ID_instances;
            nb_of_failed_pdusessions++;
            is_treated[i] = TRUE;
            is_treated[j] = TRUE;
          }
        }
        // handle multiple pdu session id case
        if (is_treated[i] == TRUE) {
          LOG_D(NR_RRC, "handle multiple pdu session id \n");
          ue_context_p->ue_context.modify_pdusession[i].status              = PDU_SESSION_STATUS_NEW;
          ue_context_p->ue_context.modify_pdusession[i].param.pdusession_id = 
            NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].pdusession_id;
          ue_context_p->ue_context.modify_pdusession[i].cause               = NGAP_CAUSE_RADIO_NETWORK;
          ue_context_p->ue_context.modify_pdusession[i].cause_value         = NGAP_CauseRadioNetwork_multiple_PDU_session_ID_instances;
          nb_of_failed_pdusessions++;
          continue;
        }

        // Check pdu session ID is established
        for (j = 0; j < NR_NB_RB_MAX -3; j++) {
          if (ue_context_p->ue_context.pduSession[j].param.pdusession_id == 
            NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].pdusession_id) {
            if (ue_context_p->ue_context.pduSession[j].status == PDU_SESSION_STATUS_TORELEASE ||
                ue_context_p->ue_context.pduSession[j].status == PDU_SESSION_STATUS_DONE) {
              break;
            }
            // Found established pdu session, prepare to send RRC message
            ue_context_p->ue_context.modify_pdusession[i].status              = PDU_SESSION_STATUS_NEW;
            ue_context_p->ue_context.modify_pdusession[i].param.pdusession_id = 
            NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].pdusession_id;
            ue_context_p->ue_context.modify_pdusession[i].cause               = NGAP_CAUSE_NOTHING;
            if (NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].nas_pdu.buffer != NULL) {
              ue_context_p->ue_context.modify_pdusession[i].param.nas_pdu.buffer = 
                NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].nas_pdu.buffer;
              ue_context_p->ue_context.modify_pdusession[i].param.nas_pdu.length =
                NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].nas_pdu.length;
            }
            // Save new pdu session parameters, qos, upf addr, teid
            for (qos_flow_index = 0; qos_flow_index < NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].nb_qos; qos_flow_index++) {
              ue_context_p->ue_context.modify_pdusession[i].param.qos[qos_flow_index] =
                NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].qos[qos_flow_index];
            }
            ue_context_p->ue_context.modify_pdusession[i].param.nb_qos = 
            NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].nb_qos;
            
            ue_context_p->ue_context.modify_pdusession[i].param.upf_addr = 
              ue_context_p->ue_context.pduSession[j].param.upf_addr;
            ue_context_p->ue_context.modify_pdusession[i].param.gtp_teid = 
              ue_context_p->ue_context.pduSession[j].param.gtp_teid;
            
            is_treated[i] = TRUE;
            break;
          }
        }

        // handle Unknown pdu session ID
        if (is_treated[i] == FALSE) {
          LOG_D(NR_RRC, "handle Unknown pdu session ID \n");
          ue_context_p->ue_context.modify_pdusession[i].status              = PDU_SESSION_STATUS_NEW;
          ue_context_p->ue_context.modify_pdusession[i].param.pdusession_id = 
            NGAP_PDUSESSION_MODIFY_REQ(msg_p).pdusession_modify_params[i].pdusession_id;
          ue_context_p->ue_context.modify_pdusession[i].cause               = NGAP_CAUSE_RADIO_NETWORK;
          ue_context_p->ue_context.modify_pdusession[i].cause_value         = NGAP_CauseRadioNetwork_unknown_PDU_session_ID;
          nb_of_failed_pdusessions++;
          is_treated[i] = TRUE;
        }
      }

      ue_context_p->ue_context.nb_of_modify_pdusessions = nb_pdusessions_tomodify;
      ue_context_p->ue_context.nb_of_failed_pdusessions = nb_of_failed_pdusessions;
    }

    if (ue_context_p->ue_context.nb_of_failed_pdusessions < ue_context_p->ue_context.nb_of_modify_pdusessions) {
      LOG_D(NR_RRC, "generate RRCReconfiguration \n");
      rrc_gNB_modify_dedicatedRRCReconfiguration(&ctxt, ue_context_p);
    } else { // all pdu modification failed
      LOG_I(NR_RRC, "pdu session modify failed, fill NGAP_PDUSESSION_MODIFY_RESP with the pdu session information that failed to modify \n");
      uint8_t nb_of_pdu_sessions_failed = 0;
      MessageDef *msg_fail_p = NULL;
      msg_fail_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_PDUSESSION_MODIFY_RESP);
      if (msg_fail_p == NULL) {
        LOG_E(NR_RRC, "itti_alloc_new_message failed, msg_fail_p is NULL \n");
        return (-1);
      }

      NGAP_PDUSESSION_MODIFY_RESP(msg_fail_p).gNB_ue_ngap_id = gNB_ue_ngap_id;
      NGAP_PDUSESSION_MODIFY_RESP(msg_fail_p).nb_of_pdusessions = 0;

      for (nb_of_pdu_sessions_failed = 0; nb_of_pdu_sessions_failed < ue_context_p->ue_context.nb_of_failed_pdusessions; nb_of_pdu_sessions_failed++) {
        NGAP_PDUSESSION_MODIFY_RESP(msg_fail_p).pdusessions_failed[nb_of_pdu_sessions_failed].pdusession_id = 
          ue_context_p->ue_context.modify_pdusession[nb_of_pdu_sessions_failed].param.pdusession_id;
        NGAP_PDUSESSION_MODIFY_RESP(msg_fail_p).pdusessions_failed[nb_of_pdu_sessions_failed].cause = 
          ue_context_p->ue_context.modify_pdusession[nb_of_pdu_sessions_failed].cause;
        NGAP_PDUSESSION_MODIFY_RESP(msg_fail_p).pdusessions_failed[nb_of_pdu_sessions_failed].cause_value = 
          ue_context_p->ue_context.modify_pdusession[nb_of_pdu_sessions_failed].cause_value;
      }

      NGAP_PDUSESSION_MODIFY_RESP(msg_fail_p).nb_of_pdusessions_failed = 
        ue_context_p->ue_context.nb_of_failed_pdusessions;
      itti_send_msg_to_task(TASK_NGAP, instance, msg_fail_p);
      ue_context_p->ue_context.nb_of_modify_pdusessions = 0;
      ue_context_p->ue_context.nb_of_failed_pdusessions = 0;
      memset(ue_context_p->ue_context.modify_pdusession, 0, sizeof(ue_context_p->ue_context.modify_pdusession));
      return (0);
    }
  }
  return 0;
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
  MessageDef *msg_p         = NULL;
  int i, j;
  uint8_t qos_flow_index;
  uint8_t pdu_sessions_failed = 0;
  uint8_t pdu_sessions_done = 0;
  
  msg_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_PDUSESSION_MODIFY_RESP);
  if (msg_p == NULL) {
    LOG_E(NR_RRC, "itti_alloc_new_message failed, msg_p is NULL \n");
    return (-1);
  }

  LOG_I(NR_RRC, "send message NGAP_PDUSESSION_MODIFY_RESP \n");

  NGAP_PDUSESSION_MODIFY_RESP(msg_p).gNB_ue_ngap_id = ue_context_pP->ue_context.gNB_ue_ngap_id;

  for (i = 0; i < ue_context_pP->ue_context.nb_of_modify_pdusessions; i++) {
    if (xid == ue_context_pP->ue_context.modify_pdusession[i].xid) {
      if (ue_context_pP->ue_context.modify_pdusession[i].status == PDU_SESSION_STATUS_DONE) {
        for (j = 0; j < ue_context_pP->ue_context.setup_pdu_sessions; j++) {
          if (ue_context_pP->ue_context.modify_pdusession[i].param.pdusession_id == 
            ue_context_pP->ue_context.pduSession[j].param.pdusession_id) {
            LOG_I(NR_RRC, "update pdu session %d \n", ue_context_pP->ue_context.pduSession[j].param.pdusession_id);
            // Update ue_context_pP->ue_context.pduSession
            ue_context_pP->ue_context.pduSession[j].status = PDU_SESSION_STATUS_ESTABLISHED;
            ue_context_pP->ue_context.pduSession[j].cause  = NGAP_CAUSE_NOTHING;
            for (qos_flow_index = 0; qos_flow_index < ue_context_pP->ue_context.modify_pdusession[i].param.nb_qos; qos_flow_index++) {
              ue_context_pP->ue_context.pduSession[j].param.qos[qos_flow_index] = 
                ue_context_pP->ue_context.modify_pdusession[i].param.qos[qos_flow_index];
            }
            break;
          }
        }

        if (j < ue_context_pP->ue_context.setup_pdu_sessions) {
          NGAP_PDUSESSION_MODIFY_RESP(msg_p).pdusessions[pdu_sessions_done].pdusession_id = 
            ue_context_pP->ue_context.modify_pdusession[i].param.pdusession_id;
          for (qos_flow_index = 0; qos_flow_index < ue_context_pP->ue_context.modify_pdusession[i].param.nb_qos; qos_flow_index++) {
            NGAP_PDUSESSION_MODIFY_RESP(msg_p).pdusessions[pdu_sessions_done].qos[qos_flow_index].qfi = 
              ue_context_pP->ue_context.modify_pdusession[i].param.qos[qos_flow_index].qfi;
          }
          NGAP_PDUSESSION_MODIFY_RESP(msg_p).pdusessions[pdu_sessions_done].nb_of_qos_flow = 
            ue_context_pP->ue_context.modify_pdusession[i].param.nb_qos;
          LOG_I(NR_RRC, "Modify Resp (msg index %d, pdu session index %d, status %d, xid %d): nb_of_modify_pdusessions %d,  pdusession_id %d \n ",
                 pdu_sessions_done,  i, ue_context_pP->ue_context.modify_pdusession[i].status, xid,
                 ue_context_pP->ue_context.nb_of_modify_pdusessions,
                 NGAP_PDUSESSION_MODIFY_RESP(msg_p).pdusessions[pdu_sessions_done].pdusession_id);
          pdu_sessions_done++;
        } else {
          NGAP_PDUSESSION_MODIFY_RESP(msg_p).pdusessions_failed[pdu_sessions_failed].pdusession_id = 
            ue_context_pP->ue_context.modify_pdusession[i].param.pdusession_id;
          NGAP_PDUSESSION_MODIFY_RESP(msg_p).pdusessions_failed[pdu_sessions_failed].cause = NGAP_CAUSE_RADIO_NETWORK;
          NGAP_PDUSESSION_MODIFY_RESP(msg_p).pdusessions_failed[pdu_sessions_failed].cause_value = NGAP_CauseRadioNetwork_unknown_PDU_session_ID;
          pdu_sessions_failed++;
        }
      } else if ((ue_context_pP->ue_context.modify_pdusession[i].status == PDU_SESSION_STATUS_NEW) ||
                 (ue_context_pP->ue_context.modify_pdusession[i].status == PDU_SESSION_STATUS_ESTABLISHED)) {
        LOG_D (NR_RRC, "PDU SESSION is NEW or already ESTABLISHED\n");
      } else if (ue_context_pP->ue_context.modify_pdusession[i].status == PDU_SESSION_STATUS_FAILED) {
        NGAP_PDUSESSION_MODIFY_RESP(msg_p).pdusessions_failed[pdu_sessions_failed].pdusession_id = 
          ue_context_pP->ue_context.modify_pdusession[i].param.pdusession_id;
        NGAP_PDUSESSION_MODIFY_RESP(msg_p).pdusessions_failed[pdu_sessions_failed].cause = 
          ue_context_pP->ue_context.modify_pdusession[i].cause;
        NGAP_PDUSESSION_MODIFY_RESP(msg_p).pdusessions_failed[pdu_sessions_failed].cause_value = 
          ue_context_pP->ue_context.modify_pdusession[i].cause_value;
        pdu_sessions_failed++;
      }
    } else {
      LOG_D(NR_RRC,"xid does not correspond (context pdu session index %d, status %d, xid %d/%d) \n ",
            i, ue_context_pP->ue_context.modify_pdusession[i].status, xid, ue_context_pP->ue_context.modify_pdusession[i].xid);
    }
  }

  NGAP_PDUSESSION_MODIFY_RESP(msg_p).nb_of_pdusessions = pdu_sessions_done;
  NGAP_PDUSESSION_MODIFY_RESP(msg_p).nb_of_pdusessions_failed = pdu_sessions_failed;

  if (pdu_sessions_done > 0 || pdu_sessions_failed > 0) {
    LOG_D(NR_RRC,"NGAP_PDUSESSION_MODIFY_RESP: sending the message: nb_of_pdusessions %d, total pdu session %d, index %d\n",
          ue_context_pP->ue_context.nb_of_modify_pdusessions, ue_context_pP->ue_context.setup_pdu_sessions, i);
    itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);
  } else {
    itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
  }

  return 0;
}

//------------------------------------------------------------------------------
void
rrc_gNB_send_NGAP_UE_CONTEXT_RELEASE_REQ(
  const module_id_t gnb_mod_idP,
  const rrc_gNB_ue_context_t *const ue_context_pP,
  const ngap_Cause_t causeP,
  const long cause_valueP)
//------------------------------------------------------------------------------
{
  if (ue_context_pP == NULL) {
    LOG_E(RRC, "[gNB] In NGAP_UE_CONTEXT_RELEASE_REQ: invalid UE\n");
  } else {
    MessageDef *msg_context_release_req_p = NULL;
    msg_context_release_req_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_UE_CONTEXT_RELEASE_REQ);
    NGAP_UE_CONTEXT_RELEASE_REQ(msg_context_release_req_p).gNB_ue_ngap_id    = ue_context_pP->ue_context.gNB_ue_ngap_id;
    NGAP_UE_CONTEXT_RELEASE_REQ(msg_context_release_req_p).cause             = causeP;
    NGAP_UE_CONTEXT_RELEASE_REQ(msg_context_release_req_p).cause_value       = cause_valueP;
    NGAP_UE_CONTEXT_RELEASE_REQ(msg_context_release_req_p).nb_of_pdusessions = ue_context_pP->ue_context.setup_pdu_sessions;
    for (int pdusession = 0; pdusession < ue_context_pP->ue_context.setup_pdu_sessions; pdusession++) {
      NGAP_UE_CONTEXT_RELEASE_REQ(msg_context_release_req_p).pdusessions[pdusession].pdusession_id = ue_context_pP->ue_context.pduSession[pdusession].param.pdusession_id;
    }
    itti_send_msg_to_task(TASK_NGAP, GNB_MODULE_ID_TO_INSTANCE(gnb_mod_idP), msg_context_release_req_p);
  }
}
/*------------------------------------------------------------------------------*/
int 
rrc_gNB_process_NGAP_UE_CONTEXT_RELEASE_REQ (
  MessageDef *msg_p, 
  const char *msg_name, 
  instance_t instance) 
{
  uint32_t gNB_ue_ngap_id;
  struct rrc_gNB_ue_context_s *ue_context_p = NULL;
  gNB_ue_ngap_id = NGAP_UE_CONTEXT_RELEASE_REQ(msg_p).gNB_ue_ngap_id;
  ue_context_p   = rrc_gNB_get_ue_context_from_ngap_ids(instance, UE_INITIAL_ID_INVALID, gNB_ue_ngap_id);

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
    {
      MessageDef *msg_resp_p;
      msg_resp_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_UE_CONTEXT_RELEASE_RESP);
      NGAP_UE_CONTEXT_RELEASE_RESP(msg_resp_p).gNB_ue_ngap_id = gNB_ue_ngap_id;
      itti_send_msg_to_task(TASK_NGAP, instance, msg_resp_p);
    }
    return (0);
  }
}

//-----------------------------------------------------------------------------
/*
* Process the NG command NGAP_UE_CONTEXT_RELEASE_COMMAND, sent by AMF.
* The gNB should remove all pdu session, NG context, and other context of the UE.
*/
int
rrc_gNB_process_NGAP_UE_CONTEXT_RELEASE_COMMAND(
  MessageDef *msg_p,
  const char *msg_name,
  instance_t instance) {
  //-----------------------------------------------------------------------------
  uint32_t gNB_ue_ngap_id = 0;
  protocol_ctxt_t ctxt;
  struct rrc_gNB_ue_context_s *ue_context_p = NULL;
  struct rrc_ue_ngap_ids_s *rrc_ue_ngap_ids = NULL;
  gNB_ue_ngap_id = NGAP_UE_CONTEXT_RELEASE_COMMAND(msg_p).gNB_ue_ngap_id;
  ue_context_p = rrc_gNB_get_ue_context_from_ngap_ids(instance, UE_INITIAL_ID_INVALID, gNB_ue_ngap_id);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index */
    MessageDef *msg_complete_p = NULL;
    LOG_W(NR_RRC, "[gNB %ld] In NGAP_UE_CONTEXT_RELEASE_COMMAND: unknown UE from gNB_ue_ngap_id (%u)\n",
          instance,
          gNB_ue_ngap_id);
    msg_complete_p = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_UE_CONTEXT_RELEASE_COMPLETE);
    NGAP_UE_CONTEXT_RELEASE_COMPLETE(msg_complete_p).gNB_ue_ngap_id = gNB_ue_ngap_id;
    itti_send_msg_to_task(TASK_NGAP, instance, msg_complete_p);
    rrc_ue_ngap_ids = rrc_gNB_NGAP_get_ue_ids(RC.nrrrc[instance], UE_INITIAL_ID_INVALID, gNB_ue_ngap_id);

    if (rrc_ue_ngap_ids != NULL) {
      rrc_gNB_NGAP_remove_ue_ids(RC.nrrrc[instance], rrc_ue_ngap_ids);
    }

    return -1;
  } else {
    ue_context_p->ue_context.ue_release_timer_ng = 0;
    ue_context_p->ue_context.ue_release_timer_thres_rrc = 1000;
    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);
    ctxt.eNB_index = 0;
    rrc_gNB_generate_RRCRelease(&ctxt, ue_context_p);
    return 0;
  }
}

void rrc_gNB_send_NGAP_UE_CONTEXT_RELEASE_COMPLETE(
  instance_t instance,
  uint32_t   gNB_ue_ngap_id) {
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, NGAP_UE_CONTEXT_RELEASE_COMPLETE);
  NGAP_UE_CONTEXT_RELEASE_COMPLETE(msg).gNB_ue_ngap_id = gNB_ue_ngap_id;
  itti_send_msg_to_task(TASK_NGAP, instance, msg);
}

//------------------------------------------------------------------------------
/*
* Remove UE ids (ue_initial_id and ng_id) from hashtables.
*/
void
rrc_gNB_NGAP_remove_ue_ids(
  gNB_RRC_INST *const rrc_instance_pP,
  struct rrc_ue_ngap_ids_s *const ue_ids_pP
)
//------------------------------------------------------------------------------
{
  hashtable_rc_t h_rc;

  if (rrc_instance_pP == NULL) {
    LOG_E(NR_RRC, "Bad NR RRC instance\n");
    return;
  }

  if (ue_ids_pP == NULL) {
    LOG_E(NR_RRC, "Trying to free a NULL NGAP UE IDs\n");
    return;
  }

  const uint16_t ue_initial_id  = ue_ids_pP->ue_initial_id;
  const uint32_t gNB_ue_ngap_id = ue_ids_pP->gNB_ue_ngap_id;

  if (gNB_ue_ngap_id > 0) {
    h_rc = hashtable_remove(rrc_instance_pP->ngap_id2_ngap_ids, (hash_key_t)gNB_ue_ngap_id);

    if (h_rc != HASH_TABLE_OK) {
      LOG_W(NR_RRC, "NGAP Did not find entry in hashtable ngap_id2_ngap_ids for gNB_ue_ngap_id %u\n", gNB_ue_ngap_id);
    } else {
      LOG_W(NR_RRC, "NGAP removed entry in hashtable ngap_id2_ngap_ids for gNB_ue_ngap_id %u\n", gNB_ue_ngap_id);
    }
  }

  if (ue_initial_id != UE_INITIAL_ID_INVALID) {
    h_rc = hashtable_remove(rrc_instance_pP->initial_id2_ngap_ids, (hash_key_t)ue_initial_id);

    if (h_rc != HASH_TABLE_OK) {
      LOG_W(NR_RRC, "NGAP Did not find entry in hashtable initial_id2_ngap_ids for ue_initial_id %u\n", ue_initial_id);
    } else {
      LOG_W(NR_RRC, "NGAP removed entry in hashtable initial_id2_ngap_ids for ue_initial_id %u\n", ue_initial_id);
    }
  }
}
void
rrc_gNB_send_NGAP_UE_CAPABILITIES_IND(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_gNB_ue_context_t     *const ue_context_pP,
  NR_UL_DCCH_Message_t     *const ul_dcch_msg
)
//------------------------------------------------------------------------------
{
    NR_UE_CapabilityRAT_ContainerList_t *ueCapabilityRATContainerList = ul_dcch_msg->message.choice.c1->choice.ueCapabilityInformation->criticalExtensions.choice.ueCapabilityInformation->ue_CapabilityRAT_ContainerList;
    /* 4096 is arbitrary, should be big enough */
    unsigned char buf[4096];
    unsigned char *buf2;
    NR_UERadioAccessCapabilityInformation_t rac;
    
    if (ueCapabilityRATContainerList->list.count == 0) {
      LOG_W(RRC, "[gNB %d][UE %x] bad UE capabilities\n", ctxt_pP->module_id, ue_context_pP->ue_context.rnti);
    }
    
    asn_enc_rval_t ret = uper_encode_to_buffer(&asn_DEF_NR_UE_CapabilityRAT_ContainerList, NULL, ueCapabilityRATContainerList, buf, 4096);
    
    if (ret.encoded == -1) abort();
    
    memset(&rac, 0, sizeof(NR_UERadioAccessCapabilityInformation_t));
    rac.criticalExtensions.present = NR_UERadioAccessCapabilityInformation__criticalExtensions_PR_c1;
    rac.criticalExtensions.choice.c1 = calloc(1,sizeof(*rac.criticalExtensions.choice.c1));
    rac.criticalExtensions.choice.c1->present = NR_UERadioAccessCapabilityInformation__criticalExtensions__c1_PR_ueRadioAccessCapabilityInformation;
    rac.criticalExtensions.choice.c1->choice.ueRadioAccessCapabilityInformation = calloc(1,sizeof(NR_UERadioAccessCapabilityInformation_IEs_t));
    rac.criticalExtensions.choice.c1->choice.ueRadioAccessCapabilityInformation->ue_RadioAccessCapabilityInfo.buf = buf;
    rac.criticalExtensions.choice.c1->choice.ueRadioAccessCapabilityInformation->ue_RadioAccessCapabilityInfo.size = (ret.encoded+7)/8;
    rac.criticalExtensions.choice.c1->choice.ueRadioAccessCapabilityInformation->nonCriticalExtension = NULL;
    /* 8192 is arbitrary, should be big enough */
    buf2 = malloc16(8192);
    
    if (buf2 == NULL) abort();
    
    ret = uper_encode_to_buffer(&asn_DEF_NR_UERadioAccessCapabilityInformation, NULL, &rac, buf2, 8192);
    
    if (ret.encoded == -1) abort();

    MessageDef *msg_p;
    msg_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_UE_CAPABILITIES_IND);
    NGAP_UE_CAPABILITIES_IND (msg_p).gNB_ue_ngap_id = ue_context_pP->ue_context.gNB_ue_ngap_id;
    NGAP_UE_CAPABILITIES_IND (msg_p).ue_radio_cap.length = (ret.encoded+7)/8;
    NGAP_UE_CAPABILITIES_IND (msg_p).ue_radio_cap.buffer = buf2;
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
  msg_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_PDUSESSION_RELEASE_RESPONSE);
  NGAP_PDUSESSION_RELEASE_RESPONSE (msg_p).gNB_ue_ngap_id = ue_context_pP->ue_context.gNB_ue_ngap_id;

  for (int i = 0;  i < NB_RB_MAX; i++) {
    if (xid == ue_context_pP->ue_context.pduSession[i].xid) {
      NGAP_PDUSESSION_RELEASE_RESPONSE (msg_p).pdusession_release[pdu_sessions_released].pdusession_id =
          ue_context_pP->ue_context.pduSession[i].param.pdusession_id;
      pdu_sessions_released++;
      //clear
      memset(&ue_context_pP->ue_context.pduSession[i], 0, sizeof(pdu_session_param_t));
    }
  }

  NGAP_PDUSESSION_RELEASE_RESPONSE (msg_p).nb_of_pdusessions_released = pdu_sessions_released;
  NGAP_PDUSESSION_RELEASE_RESPONSE (msg_p).nb_of_pdusessions_failed = ue_context_pP->ue_context.nb_release_of_pdusessions;
  memcpy(&(NGAP_PDUSESSION_RELEASE_RESPONSE (msg_p).pdusessions_failed[0]), &ue_context_pP->ue_context.pdusessions_release_failed[0],
      sizeof(pdusession_failed_t)*ue_context_pP->ue_context.nb_release_of_pdusessions);
  ue_context_pP->ue_context.setup_pdu_sessions -= pdu_sessions_released;
  LOG_I(NR_RRC,"NGAP PDUSESSION RELEASE RESPONSE: GNB_UE_NGAP_ID %u release_pdu_sessions %d setup_pdu_sessions %d \n",
        NGAP_PDUSESSION_RELEASE_RESPONSE (msg_p).gNB_ue_ngap_id,
        pdu_sessions_released, ue_context_pP->ue_context.setup_pdu_sessions);
  itti_send_msg_to_task (TASK_NGAP, ctxt_pP->instance, msg_p);

  //clear xid
  for(int i = 0; i < NB_RB_MAX; i++) {
    ue_context_pP->ue_context.pduSession[i].xid = -1;
  }

  //clear release pdusessions
  ue_context_pP->ue_context.nb_release_of_pdusessions = 0;
  memset(&ue_context_pP->ue_context.pdusessions_release_failed[0], 0, sizeof(pdusession_failed_t)*NGAP_MAX_PDUSESSION);
}

//------------------------------------------------------------------------------
int
rrc_gNB_process_NGAP_PDUSESSION_RELEASE_COMMAND(
  MessageDef *msg_p,
  const char *msg_name,
  instance_t instance
)
//------------------------------------------------------------------------------
{
  uint32_t                        gNB_ue_ngap_id;
  rrc_gNB_ue_context_t           *ue_context_p = NULL;
  protocol_ctxt_t                 ctxt;
  pdusession_release_t            pdusession_release_params[NGAP_MAX_PDUSESSION];
  uint8_t                         nb_pdusessions_torelease;
  uint8_t xid;
  int i, pdusession;
  uint8_t b_existed,is_existed;
  uint8_t pdusession_release_drb = 0;

  memcpy(&pdusession_release_params[0], &(NGAP_PDUSESSION_RELEASE_COMMAND (msg_p).pdusession_release_params[0]),
    sizeof(pdusession_release_t)*NGAP_MAX_PDUSESSION);
  gNB_ue_ngap_id = NGAP_PDUSESSION_RELEASE_COMMAND(msg_p).gNB_ue_ngap_id;
  nb_pdusessions_torelease = NGAP_PDUSESSION_RELEASE_COMMAND(msg_p).nb_pdusessions_torelease;
  if (nb_pdusessions_torelease > NGAP_MAX_PDUSESSION) {
    return -1;
  }
  ue_context_p   = rrc_gNB_get_ue_context_from_ngap_ids(instance, UE_INITIAL_ID_INVALID, gNB_ue_ngap_id);
  LOG_I(NR_RRC, "[gNB %ld] Received %s: gNB_ue_ngap_id %u \n", instance, msg_name, gNB_ue_ngap_id);

  if (ue_context_p != NULL) {
    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, GNB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);
    xid = rrc_gNB_get_next_transaction_identifier(ctxt.module_id);
    LOG_I(NR_RRC,"PDU Session Release Command: AMF_UE_NGAP_ID %lu  GNB_UE_NGAP_ID %u release_pdusessions %d \n",
          NGAP_PDUSESSION_RELEASE_COMMAND (msg_p).amf_ue_ngap_id&0x000000FFFFFFFFFF, gNB_ue_ngap_id, nb_pdusessions_torelease);

    for (pdusession = 0; pdusession < nb_pdusessions_torelease; pdusession++) {
      b_existed = 0;
      is_existed = 0;

      for (i = pdusession-1; i >= 0; i--) {
        if (pdusession_release_params[pdusession].pdusession_id == pdusession_release_params[i].pdusession_id) {
          is_existed = 1;
          break;
        }
      }

      if(is_existed == 1) {
        // pdusession_id is existed
        continue;
      }

      for (i = 0;  i < NR_NB_RB_MAX; i++) {
        if (pdusession_release_params[pdusession].pdusession_id == ue_context_p->ue_context.pduSession[i].param.pdusession_id) {
          b_existed = 1;
          break;
        }
      }

      if(b_existed == 0) {
        // no pdusession_id
        LOG_I(NR_RRC, "no pdusession_id \n");
        ue_context_p->ue_context.pdusessions_release_failed[ue_context_p->ue_context.nb_release_of_pdusessions].pdusession_id = pdusession_release_params[pdusession].pdusession_id;
        ue_context_p->ue_context.pdusessions_release_failed[ue_context_p->ue_context.nb_release_of_pdusessions].cause = NGAP_CAUSE_RADIO_NETWORK;
        ue_context_p->ue_context.pdusessions_release_failed[ue_context_p->ue_context.nb_release_of_pdusessions].cause_value = 30;
        ue_context_p->ue_context.nb_release_of_pdusessions++;
      } else {
        if(ue_context_p->ue_context.pduSession[i].status == PDU_SESSION_STATUS_FAILED) {
          ue_context_p->ue_context.pduSession[i].xid = xid;
          continue;
        } else if(ue_context_p->ue_context.pduSession[i].status == PDU_SESSION_STATUS_ESTABLISHED) {
          LOG_I(NR_RRC, "RELEASE pdusession %d \n", ue_context_p->ue_context.pduSession[i].param.pdusession_id);
          ue_context_p->ue_context.pduSession[i].status = PDU_SESSION_STATUS_TORELEASE;
          ue_context_p->ue_context.pduSession[i].xid = xid;
          pdusession_release_drb++;
        } else {
          // pdusession_id status NG
          ue_context_p->ue_context.pdusessions_release_failed[ue_context_p->ue_context.nb_release_of_pdusessions].pdusession_id = pdusession_release_params[pdusession].pdusession_id;
          ue_context_p->ue_context.pdusessions_release_failed[ue_context_p->ue_context.nb_release_of_pdusessions].cause = NGAP_CAUSE_RADIO_NETWORK;
          ue_context_p->ue_context.pdusessions_release_failed[ue_context_p->ue_context.nb_release_of_pdusessions].cause_value = 0;
          ue_context_p->ue_context.nb_release_of_pdusessions++;
        }
      }
    }

    if(pdusession_release_drb > 0) {
      //TODO RRCReconfiguration To UE
      LOG_I(NR_RRC, "Send RRCReconfiguration To UE \n");
      rrc_gNB_generate_dedicatedRRCReconfiguration_release(&ctxt, ue_context_p, xid, NGAP_PDUSESSION_RELEASE_COMMAND (msg_p).nas_pdu.length, NGAP_PDUSESSION_RELEASE_COMMAND (msg_p).nas_pdu.buffer);
    } else {
      //gtp tunnel delete
      LOG_I(NR_RRC, "gtp tunnel delete \n");
      gtpv1u_gnb_delete_tunnel_req_t req={0};
      req.rnti = ue_context_p->ue_context.rnti;

      for(i = 0; i < NB_RB_MAX; i++) {
        if(xid == ue_context_p->ue_context.pduSession[i].xid) {
          req.pdusession_id[req.num_pdusession++] = ue_context_p->ue_context.gnb_gtp_psi[i];
          ue_context_p->ue_context.gnb_gtp_teid[i] = 0;
          memset(&ue_context_p->ue_context.gnb_gtp_addrs[i], 0, sizeof(ue_context_p->ue_context.gnb_gtp_addrs[i]));
          ue_context_p->ue_context.gnb_gtp_psi[i]  = 0;
        }
      }
      gtpv1u_delete_ngu_tunnel(instance, &req);
      //NGAP_PDUSESSION_RELEASE_RESPONSE
      rrc_gNB_send_NGAP_PDUSESSION_RELEASE_RESPONSE(&ctxt, ue_context_p, xid);
      LOG_I(NR_RRC, "Send PDU Session Release Response \n");
    }
  } else {
    LOG_E(NR_RRC, "PDU Session Release Command: AMF_UE_NGAP_ID %lu  GNB_UE_NGAP_ID %u  Error ue_context_p NULL \n",
          NGAP_PDUSESSION_RELEASE_COMMAND (msg_p).amf_ue_ngap_id&0x000000FFFFFFFFFF, NGAP_PDUSESSION_RELEASE_COMMAND(msg_p).gNB_ue_ngap_id);
    return -1;
  }

  return 0;
}

void nr_rrc_rx_tx(void) {
  // check timers

  // check if UEs are lost, to remove them from upper layers

  //

}

