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

/*! \file rrc_eNB_S1AP.c
 * \brief rrc S1AP procedures for eNB
 * \author Navid Nikaein, Laurent Winckel, Sebastien ROUX, and Lionel GAUTHIER
 * \date 2013-2016
 * \version 1.0
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr
 */

#if defined(ENABLE_USE_MME)
# include "defs.h"
# include "extern.h"
# include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
# include "RRC/LITE/MESSAGES/asn1_msg.h"
# include "RRC/LITE/defs.h"
# include "rrc_eNB_UE_context.h"
# include "rrc_eNB_S1AP.h"
# include "enb_config.h"
# include "common/ran_context.h"

# if defined(ENABLE_ITTI)
#   include "asn1_conversions.h"
#   include "intertask_interface.h"
#   include "pdcp.h"
#   include "pdcp_primitives.h"
#   include "s1ap_eNB.h"
# else
#   include "../../S1AP/s1ap_eNB.h"
# endif

#if defined(ENABLE_SECURITY)
#   include "UTIL/OSA/osa_defs.h"
#endif
#include "msc.h"

#include "UERadioAccessCapabilityInformation.h"

#include "gtpv1u_eNB_task.h"
#include "RRC/LITE/rrc_eNB_GTPV1U.h"

#include "TLVDecoder.h"
#include "S1ap-NAS-PDU.h"
#include "flexran_agent_common_internal.h"

extern RAN_CONTEXT_t RC;

/* Value to indicate an invalid UE initial id */
static const uint16_t UE_INITIAL_ID_INVALID = 0;

/* Masks for S1AP Encryption algorithms, EEA0 is always supported (not coded) */
static const uint16_t S1AP_ENCRYPTION_EEA1_MASK = 0x8000;
static const uint16_t S1AP_ENCRYPTION_EEA2_MASK = 0x4000;

/* Masks for S1AP Integrity algorithms, EIA0 is always supported (not coded) */
static const uint16_t S1AP_INTEGRITY_EIA1_MASK = 0x8000;
static const uint16_t S1AP_INTEGRITY_EIA2_MASK = 0x4000;

#if defined(Rel10) || defined(Rel14)
# define INTEGRITY_ALGORITHM_NONE SecurityAlgorithmConfig__integrityProtAlgorithm_eia0_v920
#else
#ifdef EXMIMO_IOT
# define INTEGRITY_ALGORITHM_NONE SecurityAlgorithmConfig__integrityProtAlgorithm_eia2
#else
# define INTEGRITY_ALGORITHM_NONE SecurityAlgorithmConfig__integrityProtAlgorithm_reserved
#endif
#endif

void extract_imsi(uint8_t *pdu_buf, uint32_t pdu_len, rrc_eNB_ue_context_t *ue_context_pP)
{
  /* Process NAS message locally to get the IMSI */
  nas_message_t nas_msg;
  memset(&nas_msg, 0, sizeof(nas_message_t));

  int size = 0;

  nas_message_security_header_t *header = &nas_msg.header;
  /* Decode the first octet of the header (security header type or EPS
  * bearer identity, and protocol discriminator) */
  DECODE_U8((char *) pdu_buf, *(uint8_t*) (header), size);

  /* Decode NAS message only if decodable*/
  if (!(header->security_header_type <= SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED
      && header->protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE
      && pdu_len > NAS_MESSAGE_SECURITY_HEADER_SIZE))
    return;

  if (header->security_header_type != SECURITY_HEADER_TYPE_NOT_PROTECTED) {
    /* Decode the message authentication code */
    DECODE_U32((char *) pdu_buf+size, header->message_authentication_code, size);
    /* Decode the sequence number */
    DECODE_U8((char *) pdu_buf+size, header->sequence_number, size);
  }

  /* Note: the value of the pointer (i.e. the address) is given by value, so we
   * can modify it as we want. The callee retains the original address! */
  pdu_buf += size;
  pdu_len -= size;

  /* Decode plain NAS message */
  EMM_msg *e_msg = &nas_msg.plain.emm;
  emm_msg_header_t *emm_header = &e_msg->header;

  /* First decode the EMM message header */
  int e_head_size = 0;

  /* Check that buffer contains more than only the header */
  if (pdu_len <= sizeof(emm_msg_header_t))
    return;

  /* Decode the security header type and the protocol discriminator */
  DECODE_U8(pdu_buf + e_head_size, *(uint8_t *)(emm_header), e_head_size);
  /* Decode the message type */
  DECODE_U8(pdu_buf + e_head_size, emm_header->message_type, e_head_size);

  /* Check that this is the right message */
  if (emm_header->protocol_discriminator != EPS_MOBILITY_MANAGEMENT_MESSAGE)
    return;

  pdu_buf += e_head_size;
  pdu_len -= e_head_size;

  if (emm_header->message_type == IDENTITY_RESPONSE) {
    decode_identity_response(&e_msg->identity_response, pdu_buf, pdu_len);

    if (e_msg->identity_response.mobileidentity.imsi.typeofidentity == MOBILE_IDENTITY_IMSI) {
      memcpy(&ue_context_pP->ue_context.imsi,
             &e_msg->identity_response.mobileidentity.imsi,
             sizeof(ImsiMobileIdentity_t));
    }
  } else if (emm_header->message_type == ATTACH_REQUEST) {
    decode_attach_request(&e_msg->attach_request, pdu_buf, pdu_len);

    if (e_msg->attach_request.oldgutiorimsi.imsi.typeofidentity == MOBILE_IDENTITY_IMSI) {
      /* the following is very dirty, we cast (implicitly) from
       * ImsiEpsMobileIdentity_t to ImsiMobileIdentity_t*/
      memcpy(&ue_context_pP->ue_context.imsi,
             &e_msg->attach_request.oldgutiorimsi.imsi,
             sizeof(ImsiMobileIdentity_t));
    }
  }
}

# if defined(ENABLE_ITTI)
//------------------------------------------------------------------------------
struct rrc_ue_s1ap_ids_s*
rrc_eNB_S1AP_get_ue_ids(
  eNB_RRC_INST* const rrc_instance_pP,
  const uint16_t ue_initial_id,
  const uint32_t eNB_ue_s1ap_id
)
//------------------------------------------------------------------------------
{
  rrc_ue_s1ap_ids_t *result = NULL;
  rrc_ue_s1ap_ids_t *result2 = NULL;
  hashtable_rc_t     h_rc;

  // we assume that a rrc_ue_s1ap_ids_s is initially inserted in initial_id2_s1ap_ids
  if (eNB_ue_s1ap_id > 0) {
	h_rc = hashtable_get(rrc_instance_pP->s1ap_id2_s1ap_ids, (hash_key_t)eNB_ue_s1ap_id, (void**)&result);
  }
  if (ue_initial_id != UE_INITIAL_ID_INVALID) {
    h_rc = hashtable_get(rrc_instance_pP->initial_id2_s1ap_ids, (hash_key_t)ue_initial_id, (void**)&result);
	if  (h_rc == HASH_TABLE_OK) {
	  if (eNB_ue_s1ap_id > 0) {
	    h_rc = hashtable_get(rrc_instance_pP->s1ap_id2_s1ap_ids, (hash_key_t)eNB_ue_s1ap_id, (void**)&result2);
	    if  (h_rc != HASH_TABLE_OK) {
		  result2 = malloc(sizeof(*result2));
		  if (NULL != result2) {
		    *result2 = *result;
		    result2->eNB_ue_s1ap_id = eNB_ue_s1ap_id;
		    result->eNB_ue_s1ap_id  = eNB_ue_s1ap_id;
            h_rc = hashtable_insert(rrc_instance_pP->s1ap_id2_s1ap_ids,
		    		               (hash_key_t)eNB_ue_s1ap_id,
		    		               result2);
            if (h_rc != HASH_TABLE_OK) {
              LOG_E(S1AP, "[eNB %ld] Error while hashtable_insert in s1ap_id2_s1ap_ids eNB_ue_s1ap_id %"PRIu32"\n",
		    		  rrc_instance_pP - RC.rrc[0], eNB_ue_s1ap_id);
            }
		  }
		}
	  }
	}
  }
  return result;
}
//------------------------------------------------------------------------------
void
rrc_eNB_S1AP_remove_ue_ids(
  eNB_RRC_INST*              const rrc_instance_pP,
  struct rrc_ue_s1ap_ids_s* const ue_ids_pP
)
//------------------------------------------------------------------------------
{
  const uint16_t ue_initial_id  = ue_ids_pP->ue_initial_id;
  const uint32_t eNB_ue_s1ap_id = ue_ids_pP->eNB_ue_s1ap_id;
  hashtable_rc_t h_rc;
  if (rrc_instance_pP == NULL) {
    LOG_E(RRC, "Bad RRC instance\n");
    return;
  }

  if (ue_ids_pP == NULL) {
    LOG_E(RRC, "Trying to free a NULL S1AP UE IDs\n");
    return;
  }

  if (eNB_ue_s1ap_id > 0) {
	h_rc = hashtable_remove(rrc_instance_pP->s1ap_id2_s1ap_ids, (hash_key_t)eNB_ue_s1ap_id);
	if (h_rc != HASH_TABLE_OK) {
	  LOG_W(RRC, "S1AP Did not find entry in hashtable s1ap_id2_s1ap_ids for eNB_ue_s1ap_id %u\n", eNB_ue_s1ap_id);
	} else {
	  LOG_W(RRC, "S1AP removed entry in hashtable s1ap_id2_s1ap_ids for eNB_ue_s1ap_id %u\n", eNB_ue_s1ap_id);
	}
  }

  if (ue_initial_id != UE_INITIAL_ID_INVALID) {
    h_rc = hashtable_remove(rrc_instance_pP->initial_id2_s1ap_ids, (hash_key_t)ue_initial_id);
	if (h_rc != HASH_TABLE_OK) {
	  LOG_W(RRC, "S1AP Did not find entry in hashtable initial_id2_s1ap_ids for ue_initial_id %u\n", ue_initial_id);
	} else {
	  LOG_W(RRC, "S1AP removed entry in hashtable initial_id2_s1ap_ids for ue_initial_id %u\n", ue_initial_id);
	}
  }
}

/*! \fn uint16_t get_next_ue_initial_id(uint8_t mod_id)
 *\brief provide an UE initial ID for S1AP initial communication.
 *\param mod_id Instance ID of eNB.
 *\return the UE initial ID.
 */
//------------------------------------------------------------------------------
static uint16_t
get_next_ue_initial_id(
  const module_id_t mod_id
)
//------------------------------------------------------------------------------
{
  static uint16_t ue_initial_id[NUMBER_OF_eNB_MAX];
  ue_initial_id[mod_id]++;

  /* Never use UE_INITIAL_ID_INVALID this is the invalid id! */
  if (ue_initial_id[mod_id] == UE_INITIAL_ID_INVALID) {
    ue_initial_id[mod_id]++;
  }

  return ue_initial_id[mod_id];
}




/*! \fn uint8_t get_UE_index_from_s1ap_ids(uint8_t mod_id, uint16_t ue_initial_id, uint32_t eNB_ue_s1ap_id)
 *\brief retrieve UE index in the eNB from the UE initial ID if not equal to UE_INDEX_INVALID or
 *\brief from the eNB_ue_s1ap_id previously transmitted by S1AP.
 *\param mod_id Instance ID of eNB.
 *\param ue_initial_id The UE initial ID sent to S1AP.
 *\param eNB_ue_s1ap_id The value sent by S1AP.
 *\return the UE index or UE_INDEX_INVALID if not found.
 */
static struct rrc_eNB_ue_context_s*
rrc_eNB_get_ue_context_from_s1ap_ids(
  const instance_t  instanceP,
  const uint16_t    ue_initial_idP,
  const uint32_t    eNB_ue_s1ap_idP
)
{
  rrc_ue_s1ap_ids_t* temp = NULL;
  temp =
    rrc_eNB_S1AP_get_ue_ids(
      RC.rrc[ENB_INSTANCE_TO_MODULE_ID(instanceP)],
      ue_initial_idP,
      eNB_ue_s1ap_idP);

  if (temp) {

    return rrc_eNB_get_ue_context(
             RC.rrc[ENB_INSTANCE_TO_MODULE_ID(instanceP)],
             temp->ue_rnti);
  }

  return NULL;
}

/*! \fn e_SecurityAlgorithmConfig__cipheringAlgorithm rrc_eNB_select_ciphering(uint16_t algorithms)
 *\brief analyze available encryption algorithms bit mask and return the relevant one.
 *\param algorithms The bit mask of available algorithms received from S1AP.
 *\return the selected algorithm.
 */
static CipheringAlgorithm_r12_t rrc_eNB_select_ciphering(uint16_t algorithms)
{

//#warning "Forced   return SecurityAlgorithmConfig__cipheringAlgorithm_eea0, to be deleted in future"
  return CipheringAlgorithm_r12_eea0;

  if (algorithms & S1AP_ENCRYPTION_EEA2_MASK) {
    return CipheringAlgorithm_r12_eea2;
  }

  if (algorithms & S1AP_ENCRYPTION_EEA1_MASK) {
    return CipheringAlgorithm_r12_eea1;
  }

  return CipheringAlgorithm_r12_eea0;
}

/*! \fn e_SecurityAlgorithmConfig__integrityProtAlgorithm rrc_eNB_select_integrity(uint16_t algorithms)
 *\brief analyze available integrity algorithms bit mask and return the relevant one.
 *\param algorithms The bit mask of available algorithms received from S1AP.
 *\return the selected algorithm.
 */
static e_SecurityAlgorithmConfig__integrityProtAlgorithm rrc_eNB_select_integrity(uint16_t algorithms)
{

  if (algorithms & S1AP_INTEGRITY_EIA2_MASK) {
    return SecurityAlgorithmConfig__integrityProtAlgorithm_eia2;
  }

  if (algorithms & S1AP_INTEGRITY_EIA1_MASK) {
    return SecurityAlgorithmConfig__integrityProtAlgorithm_eia1;
  }

  return INTEGRITY_ALGORITHM_NONE;
}

/*! \fn int rrc_eNB_process_security (uint8_t mod_id, uint8_t ue_index, security_capabilities_t *security_capabilities)
 *\brief save and analyze available security algorithms bit mask and select relevant ones.
 *\param mod_id Instance ID of eNB.
 *\param ue_index Instance ID of UE in the eNB.
 *\param security_capabilities The security capabilities received from S1AP.
 *\return TRUE if at least one algorithm has been changed else FALSE.
 */
static int
rrc_eNB_process_security(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  security_capabilities_t* security_capabilities_pP
)
{
  boolean_t                                         changed = FALSE;
  CipheringAlgorithm_r12_t                          cipheringAlgorithm;
  e_SecurityAlgorithmConfig__integrityProtAlgorithm integrityProtAlgorithm;

  /* Save security parameters */
  ue_context_pP->ue_context.security_capabilities = *security_capabilities_pP;

  // translation
  LOG_D(RRC,
        "[eNB %d] NAS security_capabilities.encryption_algorithms %u AS ciphering_algorithm %lu NAS security_capabilities.integrity_algorithms %u AS integrity_algorithm %u\n",
        ctxt_pP->module_id,
        ue_context_pP->ue_context.security_capabilities.encryption_algorithms,
        (unsigned long)ue_context_pP->ue_context.ciphering_algorithm,
        ue_context_pP->ue_context.security_capabilities.integrity_algorithms,
        ue_context_pP->ue_context.integrity_algorithm);
  /* Select relevant algorithms */
  cipheringAlgorithm = rrc_eNB_select_ciphering (ue_context_pP->ue_context.security_capabilities.encryption_algorithms);

  if (ue_context_pP->ue_context.ciphering_algorithm != cipheringAlgorithm) {
    ue_context_pP->ue_context.ciphering_algorithm = cipheringAlgorithm;
    changed = TRUE;
  }

  integrityProtAlgorithm = rrc_eNB_select_integrity (ue_context_pP->ue_context.security_capabilities.integrity_algorithms);

  if (ue_context_pP->ue_context.integrity_algorithm != integrityProtAlgorithm) {
    ue_context_pP->ue_context.integrity_algorithm = integrityProtAlgorithm;
    changed = TRUE;
  }

  LOG_I (RRC, "[eNB %d][UE %x] Selected security algorithms (%p): %lx, %x, %s\n",
         ctxt_pP->module_id,
         ue_context_pP->ue_context.rnti,
         security_capabilities_pP,
         (unsigned long)cipheringAlgorithm,
         integrityProtAlgorithm,
         changed ? "changed" : "same");

  return changed;
}

/*! \fn void process_eNB_security_key (const protocol_ctxt_t* const ctxt_pP, eNB_RRC_UE_t * const ue_context_pP, uint8_t *security_key)
 *\brief save security key.
 *\param ctxt_pP         Running context.
 *\param ue_context_pP   UE context.
 *\param security_key_pP The security key received from S1AP.
 */
//------------------------------------------------------------------------------
static void process_eNB_security_key (
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  uint8_t*               security_key_pP
)
//------------------------------------------------------------------------------
{
#if defined(ENABLE_SECURITY)
  char ascii_buffer[65];
  uint8_t i;

  /* Saves the security key */
  memcpy (ue_context_pP->ue_context.kenb, security_key_pP, SECURITY_KEY_LENGTH);
  memset (ue_context_pP->ue_context.nh, 0, SECURITY_KEY_LENGTH);
  ue_context_pP->ue_context.nh_ncc = -1;

  for (i = 0; i < 32; i++) {
    sprintf(&ascii_buffer[2 * i], "%02X", ue_context_pP->ue_context.kenb[i]);
  }

  ascii_buffer[2 * i] = '\0';

  LOG_I (RRC, "[eNB %d][UE %x] Saved security key %s\n", ctxt_pP->module_id, ue_context_pP->ue_context.rnti, ascii_buffer);
#endif
}


//------------------------------------------------------------------------------
void
rrc_pdcp_config_security(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  const uint8_t send_security_mode_command
)
//------------------------------------------------------------------------------
{

#if defined(ENABLE_SECURITY)


  SRB_ToAddModList_t*                 SRB_configList = ue_context_pP->ue_context.SRB_configList;
  uint8_t                            *kRRCenc = NULL;
  uint8_t                            *kRRCint = NULL;
  uint8_t                            *kUPenc = NULL;
  pdcp_t                             *pdcp_p   = NULL;
  static int                          print_keys= 1;
  hashtable_rc_t                      h_rc;
  hash_key_t                          key;

  /* Derive the keys from kenb */
  if (SRB_configList != NULL) {
    derive_key_up_enc(ue_context_pP->ue_context.ciphering_algorithm,
                      ue_context_pP->ue_context.kenb,
                      &kUPenc);
  }

  derive_key_rrc_enc(ue_context_pP->ue_context.ciphering_algorithm,
                     ue_context_pP->ue_context.kenb,
                     &kRRCenc);
  derive_key_rrc_int(ue_context_pP->ue_context.integrity_algorithm,
                     ue_context_pP->ue_context.kenb,
                     &kRRCint);

#if !defined(USRP_REC_PLAY)
#define DEBUG_SECURITY 1
#endif
  
#if defined (DEBUG_SECURITY)
#undef msg
#define msg printf

  if (print_keys ==1 ) {
    print_keys =0;
    int i;
    msg("\nKeNB:");

    for(i = 0; i < 32; i++) {
      msg("%02x", ue_context_pP->ue_context.kenb[i]);
    }

    msg("\n");

    msg("\nKRRCenc:");

    for(i = 0; i < 32; i++) {
      msg("%02x", kRRCenc[i]);
    }

    msg("\n");

    msg("\nKRRCint:");

    for(i = 0; i < 32; i++) {
      msg("%02x", kRRCint[i]);
    }

    msg("\n");

  }

#endif //DEBUG_SECURITY
  key = PDCP_COLL_KEY_VALUE(ctxt_pP->module_id, ctxt_pP->rnti, ctxt_pP->enb_flag, DCCH, SRB_FLAG_YES);
  h_rc = hashtable_get(pdcp_coll_p, key, (void**)&pdcp_p);


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
    LOG_E(RRC,
          PROTOCOL_RRC_CTXT_UE_FMT"Could not get PDCP instance for SRB DCCH %u\n",
          PROTOCOL_RRC_CTXT_UE_ARGS(ctxt_pP),
          DCCH);
  }

#endif
}

//------------------------------------------------------------------------------
void
rrc_eNB_send_S1AP_INITIAL_CONTEXT_SETUP_RESP(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP
)
//------------------------------------------------------------------------------
{
  MessageDef      *msg_p         = NULL;
  int e_rab;
  int e_rabs_done = 0;
  int e_rabs_failed = 0;

  msg_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_INITIAL_CONTEXT_SETUP_RESP);
  S1AP_INITIAL_CONTEXT_SETUP_RESP (msg_p).eNB_ue_s1ap_id = ue_context_pP->ue_context.eNB_ue_s1ap_id;

  for (e_rab = 0; e_rab < ue_context_pP->ue_context.nb_of_e_rabs; e_rab++) {
    if (ue_context_pP->ue_context.e_rab[e_rab].status == E_RAB_STATUS_DONE) {
      e_rabs_done++;
      S1AP_INITIAL_CONTEXT_SETUP_RESP (msg_p).e_rabs[e_rab].e_rab_id = ue_context_pP->ue_context.e_rab[e_rab].param.e_rab_id;
      // TODO add other information from S1-U when it will be integrated
      S1AP_INITIAL_CONTEXT_SETUP_RESP (msg_p).e_rabs[e_rab].gtp_teid = ue_context_pP->ue_context.enb_gtp_teid[e_rab];
      S1AP_INITIAL_CONTEXT_SETUP_RESP (msg_p).e_rabs[e_rab].eNB_addr = ue_context_pP->ue_context.enb_gtp_addrs[e_rab];
      S1AP_INITIAL_CONTEXT_SETUP_RESP (msg_p).e_rabs[e_rab].eNB_addr.length = 4;
      ue_context_pP->ue_context.e_rab[e_rab].status = E_RAB_STATUS_ESTABLISHED;
    } else {
      e_rabs_failed++;
      ue_context_pP->ue_context.e_rab[e_rab].status = E_RAB_STATUS_FAILED;
      S1AP_INITIAL_CONTEXT_SETUP_RESP (msg_p).e_rabs_failed[e_rab].e_rab_id = ue_context_pP->ue_context.e_rab[e_rab].param.e_rab_id;
      // TODO add cause when it will be integrated
    }
  }

  MSC_LOG_TX_MESSAGE(
    MSC_RRC_ENB,
    MSC_S1AP_ENB,
    (const char *)&S1AP_INITIAL_CONTEXT_SETUP_RESP (msg_p),
    sizeof(s1ap_initial_context_setup_resp_t),
    MSC_AS_TIME_FMT" INITIAL_CONTEXT_SETUP_RESP UE %X eNB_ue_s1ap_id %u e_rabs:%u succ %u fail",
    MSC_AS_TIME_ARGS(ctxt_pP),
    ue_context_pP->ue_id_rnti,
    S1AP_INITIAL_CONTEXT_SETUP_RESP (msg_p).eNB_ue_s1ap_id,
    e_rabs_done, e_rabs_failed);


  S1AP_INITIAL_CONTEXT_SETUP_RESP (msg_p).nb_of_e_rabs = e_rabs_done;
  S1AP_INITIAL_CONTEXT_SETUP_RESP (msg_p).nb_of_e_rabs_failed = e_rabs_failed;

  itti_send_msg_to_task (TASK_S1AP, ctxt_pP->instance, msg_p);
}
# endif

//------------------------------------------------------------------------------
void
rrc_eNB_send_S1AP_UPLINK_NAS(
  const protocol_ctxt_t*    const ctxt_pP,
  rrc_eNB_ue_context_t*             const ue_context_pP,
  UL_DCCH_Message_t*        const ul_dcch_msg
)
//------------------------------------------------------------------------------
{
#if defined(ENABLE_ITTI)
  {
    ULInformationTransfer_t *ulInformationTransfer = &ul_dcch_msg->message.choice.c1.choice.ulInformationTransfer;

    if ((ulInformationTransfer->criticalExtensions.present == ULInformationTransfer__criticalExtensions_PR_c1)
    && (ulInformationTransfer->criticalExtensions.choice.c1.present
    == ULInformationTransfer__criticalExtensions__c1_PR_ulInformationTransfer_r8)
    && (ulInformationTransfer->criticalExtensions.choice.c1.choice.ulInformationTransfer_r8.dedicatedInfoType.present
    == ULInformationTransfer_r8_IEs__dedicatedInfoType_PR_dedicatedInfoNAS)) {
      /* This message hold a dedicated info NAS payload, forward it to NAS */
      struct ULInformationTransfer_r8_IEs__dedicatedInfoType *dedicatedInfoType =
          &ulInformationTransfer->criticalExtensions.choice.c1.choice.ulInformationTransfer_r8.dedicatedInfoType;
      uint32_t pdu_length;
      uint8_t *pdu_buffer;
      MessageDef *msg_p;

      pdu_length = dedicatedInfoType->choice.dedicatedInfoNAS.size;
      pdu_buffer = dedicatedInfoType->choice.dedicatedInfoNAS.buf;

      msg_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_UPLINK_NAS);
      S1AP_UPLINK_NAS (msg_p).eNB_ue_s1ap_id = ue_context_pP->ue_context.eNB_ue_s1ap_id;
      S1AP_UPLINK_NAS (msg_p).nas_pdu.length = pdu_length;
      S1AP_UPLINK_NAS (msg_p).nas_pdu.buffer = pdu_buffer;

      extract_imsi(S1AP_UPLINK_NAS (msg_p).nas_pdu.buffer,
                   S1AP_UPLINK_NAS (msg_p).nas_pdu.length,
                   ue_context_pP);

      itti_send_msg_to_task (TASK_S1AP, ctxt_pP->instance, msg_p);
    }
  }
#else
  {
    ULInformationTransfer_t *ulInformationTransfer;
    ulInformationTransfer =
    &ul_dcch_msg->message.choice.c1.choice.
    ulInformationTransfer;

    if (ulInformationTransfer->criticalExtensions.present ==
    ULInformationTransfer__criticalExtensions_PR_c1) {
      if (ulInformationTransfer->criticalExtensions.choice.c1.present ==
      ULInformationTransfer__criticalExtensions__c1_PR_ulInformationTransfer_r8) {

        ULInformationTransfer_r8_IEs_t
        *ulInformationTransferR8;
        ulInformationTransferR8 =
        &ulInformationTransfer->criticalExtensions.choice.
        c1.choice.ulInformationTransfer_r8;

        if (ulInformationTransferR8->dedicatedInfoType.present ==
            ULInformationTransfer_r8_IEs__dedicatedInfoType_PR_dedicatedInfoNAS) {

          extract_imsi(ulInformationTransferR8->dedicatedInfoType.choice.dedicatedInfoNAS.buf,
                       ulInformationTransferR8->dedicatedInfoType.choice.dedicatedInfoNAS.size,
                       ue_context_pP);

          s1ap_eNB_new_data_request (mod_id, ue_index,
              ulInformationTransferR8->dedicatedInfoType.choice.dedicatedInfoNAS.buf,
              ulInformationTransferR8->dedicatedInfoType.choice.dedicatedInfoNAS.size);
        }
      }
    }
  }
#endif
}

//------------------------------------------------------------------------------
void rrc_eNB_send_S1AP_UE_CAPABILITIES_IND(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  UL_DCCH_Message_t* ul_dcch_msg
)
//------------------------------------------------------------------------------
{
  UECapabilityInformation_t *ueCapabilityInformation = &ul_dcch_msg->message.choice.c1.choice.ueCapabilityInformation;
  /* 4096 is arbitrary, should be big enough */
  unsigned char buf[4096];
  unsigned char *buf2;
  UERadioAccessCapabilityInformation_t rac;

  if (ueCapabilityInformation->criticalExtensions.present != UECapabilityInformation__criticalExtensions_PR_c1
      || ueCapabilityInformation->criticalExtensions.choice.c1.present != UECapabilityInformation__criticalExtensions__c1_PR_ueCapabilityInformation_r8) {
    LOG_E(RRC, "[eNB %d][UE %x] bad UE capabilities\n", ctxt_pP->module_id, ue_context_pP->ue_context.rnti);
    return;
  }

  asn_enc_rval_t ret = uper_encode_to_buffer(&asn_DEF_UECapabilityInformation, ueCapabilityInformation, buf, 4096);
  if (ret.encoded == -1) abort();

  memset(&rac, 0, sizeof(UERadioAccessCapabilityInformation_t));

  rac.criticalExtensions.present = UERadioAccessCapabilityInformation__criticalExtensions_PR_c1;
  rac.criticalExtensions.choice.c1.present = UERadioAccessCapabilityInformation__criticalExtensions__c1_PR_ueRadioAccessCapabilityInformation_r8;
  rac.criticalExtensions.choice.c1.choice.ueRadioAccessCapabilityInformation_r8.ue_RadioAccessCapabilityInfo.buf = buf;
  rac.criticalExtensions.choice.c1.choice.ueRadioAccessCapabilityInformation_r8.ue_RadioAccessCapabilityInfo.size = (ret.encoded+7)/8;
  rac.criticalExtensions.choice.c1.choice.ueRadioAccessCapabilityInformation_r8.nonCriticalExtension = NULL;

  /* 8192 is arbitrary, should be big enough */
  buf2 = malloc16(8192);
  if (buf2 == NULL) abort();
  ret = uper_encode_to_buffer(&asn_DEF_UERadioAccessCapabilityInformation, &rac, buf2, 8192);
  if (ret.encoded == -1) abort();

  MessageDef *msg_p;

  msg_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_UE_CAPABILITIES_IND);
  S1AP_UE_CAPABILITIES_IND (msg_p).eNB_ue_s1ap_id = ue_context_pP->ue_context.eNB_ue_s1ap_id;
  S1AP_UE_CAPABILITIES_IND (msg_p).ue_radio_cap.length = (ret.encoded+7)/8;
  S1AP_UE_CAPABILITIES_IND (msg_p).ue_radio_cap.buffer = buf2;

  itti_send_msg_to_task (TASK_S1AP, ctxt_pP->instance, msg_p);
}

//------------------------------------------------------------------------------
void
rrc_eNB_send_S1AP_NAS_FIRST_REQ(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  RRCConnectionSetupComplete_r8_IEs_t* rrcConnectionSetupComplete
)
//------------------------------------------------------------------------------


{
  eNB_RRC_INST *rrc=RC.rrc[ctxt_pP->module_id];
#if defined(ENABLE_ITTI)
  {
    MessageDef*         message_p         = NULL;
    rrc_ue_s1ap_ids_t*  rrc_ue_s1ap_ids_p = NULL;
    hashtable_rc_t      h_rc;

    message_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_NAS_FIRST_REQ);
    memset(&message_p->ittiMsg.s1ap_nas_first_req, 0, sizeof(s1ap_nas_first_req_t));

    ue_context_pP->ue_context.ue_initial_id = get_next_ue_initial_id (ctxt_pP->module_id);
    S1AP_NAS_FIRST_REQ (message_p).ue_initial_id = ue_context_pP->ue_context.ue_initial_id;

    rrc_ue_s1ap_ids_p = malloc(sizeof(*rrc_ue_s1ap_ids_p));
    rrc_ue_s1ap_ids_p->ue_initial_id  = ue_context_pP->ue_context.ue_initial_id;
    rrc_ue_s1ap_ids_p->eNB_ue_s1ap_id = UE_INITIAL_ID_INVALID;
    rrc_ue_s1ap_ids_p->ue_rnti        = ctxt_pP->rnti;

    h_rc = hashtable_insert(RC.rrc[ctxt_pP->module_id]->initial_id2_s1ap_ids,
    		               (hash_key_t)ue_context_pP->ue_context.ue_initial_id,
    		               rrc_ue_s1ap_ids_p);
    if (h_rc != HASH_TABLE_OK) {
      LOG_E(S1AP, "[eNB %d] Error while hashtable_insert in initial_id2_s1ap_ids ue_initial_id %u\n",
    		  ctxt_pP->module_id, ue_context_pP->ue_context.ue_initial_id);
    }

    /* Assume that cause is coded in the same way in RRC and S1ap, just check that the value is in S1ap range */
    AssertFatal(ue_context_pP->ue_context.establishment_cause < RRC_CAUSE_LAST,
    "Establishment cause invalid (%jd/%d) for eNB %d!",
    ue_context_pP->ue_context.establishment_cause, RRC_CAUSE_LAST, ctxt_pP->module_id);

    S1AP_NAS_FIRST_REQ (message_p).establishment_cause = ue_context_pP->ue_context.establishment_cause;

    /* Forward NAS message */S1AP_NAS_FIRST_REQ (message_p).nas_pdu.buffer =
    rrcConnectionSetupComplete->dedicatedInfoNAS.buf;
    S1AP_NAS_FIRST_REQ (message_p).nas_pdu.length = rrcConnectionSetupComplete->dedicatedInfoNAS.size;

    extract_imsi(S1AP_NAS_FIRST_REQ (message_p).nas_pdu.buffer,
                 S1AP_NAS_FIRST_REQ (message_p).nas_pdu.length,
                 ue_context_pP);

    /* Fill UE identities with available information */
    {
      S1AP_NAS_FIRST_REQ (message_p).ue_identity.presenceMask = UE_IDENTITIES_NONE;

      if (ue_context_pP->ue_context.Initialue_identity_s_TMSI.presence) {
        /* Fill s-TMSI */
        UE_S_TMSI* s_TMSI = &ue_context_pP->ue_context.Initialue_identity_s_TMSI;

        S1AP_NAS_FIRST_REQ (message_p).ue_identity.presenceMask |= UE_IDENTITIES_s_tmsi;
        S1AP_NAS_FIRST_REQ (message_p).ue_identity.s_tmsi.mme_code = s_TMSI->mme_code;
        S1AP_NAS_FIRST_REQ (message_p).ue_identity.s_tmsi.m_tmsi = s_TMSI->m_tmsi;
        LOG_I(S1AP, "[eNB %d] Build S1AP_NAS_FIRST_REQ with s_TMSI: MME code %u M-TMSI %u ue %x\n",
            ctxt_pP->module_id,
            S1AP_NAS_FIRST_REQ (message_p).ue_identity.s_tmsi.mme_code,
            S1AP_NAS_FIRST_REQ (message_p).ue_identity.s_tmsi.m_tmsi,
            ue_context_pP->ue_context.rnti);
      }

      if (rrcConnectionSetupComplete->registeredMME != NULL) {
        /* Fill GUMMEI */
        struct RegisteredMME *r_mme = rrcConnectionSetupComplete->registeredMME;
        //int selected_plmn_identity = rrcConnectionSetupComplete->selectedPLMN_Identity;

        S1AP_NAS_FIRST_REQ (message_p).ue_identity.presenceMask |= UE_IDENTITIES_gummei;

        if (r_mme->plmn_Identity != NULL) {
          if ((r_mme->plmn_Identity->mcc != NULL) && (r_mme->plmn_Identity->mcc->list.count > 0)) {
            /* Use first indicated PLMN MCC if it is defined */
            S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mcc = *r_mme->plmn_Identity->mcc->list.array[0];
            LOG_I(S1AP, "[eNB %d] Build S1AP_NAS_FIRST_REQ adding in s_TMSI: GUMMEI MCC %u ue %x\n",
                ctxt_pP->module_id,
                S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mcc,
                ue_context_pP->ue_context.rnti);
          }

          if (r_mme->plmn_Identity->mnc.list.count > 0) {
            /* Use first indicated PLMN MNC if it is defined */
            S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mnc = *r_mme->plmn_Identity->mnc.list.array[0];
            LOG_I(S1AP, "[eNB %d] Build S1AP_NAS_FIRST_REQ adding in s_TMSI: GUMMEI MNC %u ue %x\n",
                  ctxt_pP->module_id,
                  S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mnc,
                  ue_context_pP->ue_context.rnti);
          }
        } else {
	  //          const Enb_properties_array_t   *enb_properties_p  = NULL;
	  //          enb_properties_p = enb_config_get();

          // actually the eNB configuration contains only one PLMN (can be up to 6)
          S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mcc = rrc->mcc;
          S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mnc = rrc->mnc;
          S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mnc_len = rrc->mnc_digit_length;
        }

        S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mme_code     = BIT_STRING_to_uint8 (&r_mme->mmec);
        S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mme_group_id = BIT_STRING_to_uint16 (&r_mme->mmegi);

        MSC_LOG_TX_MESSAGE(
          MSC_S1AP_ENB,
          MSC_S1AP_MME,
          (const char *)&message_p->ittiMsg.s1ap_nas_first_req,
          sizeof(s1ap_nas_first_req_t),
          MSC_AS_TIME_FMT" S1AP_NAS_FIRST_REQ eNB %u UE %x",
          MSC_AS_TIME_ARGS(ctxt_pP),
          ctxt_pP->module_id,
          ctxt_pP->rnti);

        LOG_I(S1AP, "[eNB %d] Build S1AP_NAS_FIRST_REQ adding in s_TMSI: GUMMEI mme_code %u mme_group_id %u ue %x\n",
              ctxt_pP->module_id,
              S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mme_code,
              S1AP_NAS_FIRST_REQ (message_p).ue_identity.gummei.mme_group_id,
              ue_context_pP->ue_context.rnti);
      }
    }
    itti_send_msg_to_task (TASK_S1AP, ctxt_pP->instance, message_p);
  }
#else
  {
    s1ap_eNB_new_data_request (
      ctxt_pP->module_id,
      ue_context_pP,
      rrcConnectionSetupComplete->dedicatedInfoNAS.
      buf,
      rrcConnectionSetupComplete->dedicatedInfoNAS.
      size);
  }
#endif
}

# if defined(ENABLE_ITTI)
//------------------------------------------------------------------------------
int
rrc_eNB_process_S1AP_DOWNLINK_NAS(
  MessageDef* msg_p,
  const char* msg_name,
  instance_t instance,
  mui_t* rrc_eNB_mui
)
//------------------------------------------------------------------------------
{
  uint16_t ue_initial_id;
  uint32_t eNB_ue_s1ap_id;
  uint32_t length;
  uint8_t *buffer;
  uint8_t srb_id; 
  
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  protocol_ctxt_t              ctxt;
  ue_initial_id = S1AP_DOWNLINK_NAS (msg_p).ue_initial_id;
  eNB_ue_s1ap_id = S1AP_DOWNLINK_NAS (msg_p).eNB_ue_s1ap_id;
  ue_context_p = rrc_eNB_get_ue_context_from_s1ap_ids(instance, ue_initial_id, eNB_ue_s1ap_id);


  LOG_I(RRC, "[eNB %d] Received %s: ue_initial_id %d, eNB_ue_s1ap_id %d\n",
        instance,
        msg_name,
        ue_initial_id,
        eNB_ue_s1ap_id);

  if (ue_context_p == NULL) {

    MSC_LOG_RX_MESSAGE(
      MSC_RRC_ENB,
      MSC_S1AP_ENB,
      NULL,
      0,
      MSC_AS_TIME_FMT" DOWNLINK-NAS UE initial id %u eNB_ue_s1ap_id %u",
      0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
      ue_initial_id,
      eNB_ue_s1ap_id);

    /* Can not associate this message to an UE index, send a failure to S1AP and discard it! */
    MessageDef *msg_fail_p;

    LOG_W(RRC, "[eNB %d] In S1AP_DOWNLINK_NAS: unknown UE from S1AP ids (%d, %d)\n", instance, ue_initial_id, eNB_ue_s1ap_id);

    msg_fail_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_NAS_NON_DELIVERY_IND);
    S1AP_NAS_NON_DELIVERY_IND (msg_fail_p).eNB_ue_s1ap_id = eNB_ue_s1ap_id;
    S1AP_NAS_NON_DELIVERY_IND (msg_fail_p).nas_pdu.length = S1AP_DOWNLINK_NAS (msg_p).nas_pdu.length;
    S1AP_NAS_NON_DELIVERY_IND (msg_fail_p).nas_pdu.buffer = S1AP_DOWNLINK_NAS (msg_p).nas_pdu.buffer;

    // TODO add failure cause when defined!


    MSC_LOG_TX_MESSAGE(
      MSC_RRC_ENB,
      MSC_S1AP_ENB,
      (const char *)NULL,
      0,
      MSC_AS_TIME_FMT" S1AP_NAS_NON_DELIVERY_IND UE initial id %u eNB_ue_s1ap_id %u (ue ctxt !found)",
      0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
      ue_initial_id,
      eNB_ue_s1ap_id);

    itti_send_msg_to_task (TASK_S1AP, instance, msg_fail_p);
    return (-1);
  } else {
    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);

    srb_id = ue_context_p->ue_context.Srb2.Srb_info.Srb_id;
  

    /* Is it the first income from S1AP ? */
    if (ue_context_p->ue_context.eNB_ue_s1ap_id == 0) {
      ue_context_p->ue_context.eNB_ue_s1ap_id = S1AP_DOWNLINK_NAS (msg_p).eNB_ue_s1ap_id;
    }

    MSC_LOG_RX_MESSAGE(
      MSC_RRC_ENB,
      MSC_S1AP_ENB,
      (const char *)NULL,
      0,
      MSC_AS_TIME_FMT" DOWNLINK-NAS UE initial id %u eNB_ue_s1ap_id %u",
      0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
      ue_initial_id,
      S1AP_DOWNLINK_NAS (msg_p).eNB_ue_s1ap_id);


    /* Create message for PDCP (DLInformationTransfer_t) */
    length = do_DLInformationTransfer (
               instance,
               &buffer,
               rrc_eNB_get_next_transaction_identifier (instance),
               S1AP_DOWNLINK_NAS (msg_p).nas_pdu.length,
               S1AP_DOWNLINK_NAS (msg_p).nas_pdu.buffer);

#ifdef RRC_MSG_PRINT
    int i=0;
    LOG_F(RRC,"[MSG] RRC DL Information Transfer\n");

    for (i = 0; i < length; i++) {
      LOG_F(RRC,"%02x ", ((uint8_t*)buffer)[i]);
    }

    LOG_F(RRC,"\n");
#endif
    /* 
     * switch UL or DL NAS message without RRC piggybacked to SRB2 if active. 
     */
    /* Transfer data to PDCP */
    rrc_data_req (
		  &ctxt,
		  srb_id,
		  *rrc_eNB_mui++,
		  SDU_CONFIRM_NO,
		  length,
		  buffer,
		  PDCP_TRANSMISSION_MODE_CONTROL);
    
    return (0);
  }
}

/*------------------------------------------------------------------------------*/
int rrc_eNB_process_S1AP_INITIAL_CONTEXT_SETUP_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance)
{
  uint16_t                        ue_initial_id;
  uint32_t                        eNB_ue_s1ap_id;
  //MessageDef                     *message_gtpv1u_p = NULL;
  gtpv1u_enb_create_tunnel_req_t  create_tunnel_req;
  gtpv1u_enb_create_tunnel_resp_t create_tunnel_resp;
  uint8_t                         inde_list[NB_RB_MAX - 3]={0};

  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  protocol_ctxt_t              ctxt;
  ue_initial_id  = S1AP_INITIAL_CONTEXT_SETUP_REQ (msg_p).ue_initial_id;
  eNB_ue_s1ap_id = S1AP_INITIAL_CONTEXT_SETUP_REQ (msg_p).eNB_ue_s1ap_id;
  ue_context_p   = rrc_eNB_get_ue_context_from_s1ap_ids(instance, ue_initial_id, eNB_ue_s1ap_id);
  LOG_I(RRC, "[eNB %d] Received %s: ue_initial_id %d, eNB_ue_s1ap_id %d, nb_of_e_rabs %d\n",
        instance, msg_name, ue_initial_id, eNB_ue_s1ap_id, S1AP_INITIAL_CONTEXT_SETUP_REQ (msg_p).nb_of_e_rabs);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index, send a failure to S1AP and discard it! */
    MessageDef *msg_fail_p = NULL;

    LOG_W(RRC, "[eNB %d] In S1AP_INITIAL_CONTEXT_SETUP_REQ: unknown UE from S1AP ids (%d, %d)\n", instance, ue_initial_id, eNB_ue_s1ap_id);

    msg_fail_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_INITIAL_CONTEXT_SETUP_FAIL);
    S1AP_INITIAL_CONTEXT_SETUP_FAIL (msg_fail_p).eNB_ue_s1ap_id = eNB_ue_s1ap_id;

    // TODO add failure cause when defined!

    itti_send_msg_to_task (TASK_S1AP, instance, msg_fail_p);
    return (-1);
  } else {

    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);
    ue_context_p->ue_context.eNB_ue_s1ap_id = S1AP_INITIAL_CONTEXT_SETUP_REQ (msg_p).eNB_ue_s1ap_id;

    /* Save e RAB information for later */
    {
      int i;

      memset(&create_tunnel_req, 0 , sizeof(create_tunnel_req));
      ue_context_p->ue_context.nb_of_e_rabs = S1AP_INITIAL_CONTEXT_SETUP_REQ (msg_p).nb_of_e_rabs;
     
      for (i = 0; i < ue_context_p->ue_context.nb_of_e_rabs; i++) {
        ue_context_p->ue_context.e_rab[i].status = E_RAB_STATUS_NEW;
        ue_context_p->ue_context.e_rab[i].param = S1AP_INITIAL_CONTEXT_SETUP_REQ (msg_p).e_rab_param[i];


        create_tunnel_req.eps_bearer_id[i]       = S1AP_INITIAL_CONTEXT_SETUP_REQ (msg_p).e_rab_param[i].e_rab_id;
        create_tunnel_req.sgw_S1u_teid[i]        = S1AP_INITIAL_CONTEXT_SETUP_REQ (msg_p).e_rab_param[i].gtp_teid;

        memcpy(&create_tunnel_req.sgw_addr[i],
               &S1AP_INITIAL_CONTEXT_SETUP_REQ (msg_p).e_rab_param[i].sgw_addr,
               sizeof(transport_layer_addr_t));
        inde_list[create_tunnel_req.num_tunnels]= i;
        create_tunnel_req.num_tunnels++;
      }
    
      create_tunnel_req.rnti       = ue_context_p->ue_context.rnti; // warning put zero above
//      create_tunnel_req.num_tunnels    = i;

      gtpv1u_create_s1u_tunnel(
        instance,
        &create_tunnel_req,
        &create_tunnel_resp);

      rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP(
          &ctxt,
          &create_tunnel_resp,
          &inde_list[0]); 
      ue_context_p->ue_context.setup_e_rabs=ue_context_p->ue_context.nb_of_e_rabs;
    }

    /* TODO parameters yet to process ... */
    {
      //      S1AP_INITIAL_CONTEXT_SETUP_REQ(msg_p).ue_ambr;
    }

    rrc_eNB_process_security (
      &ctxt,
      ue_context_p,
      &S1AP_INITIAL_CONTEXT_SETUP_REQ(msg_p).security_capabilities);
    process_eNB_security_key (
      &ctxt,
      ue_context_p,
      S1AP_INITIAL_CONTEXT_SETUP_REQ(msg_p).security_key);

    {
      uint8_t send_security_mode_command = TRUE;

#ifndef EXMIMO_IOT

      if ((ue_context_p->ue_context.ciphering_algorithm == SecurityAlgorithmConfig__cipheringAlgorithm_eea0)
          && (ue_context_p->ue_context.integrity_algorithm == INTEGRITY_ALGORITHM_NONE)) {
        send_security_mode_command = FALSE;
      }

#endif
      rrc_pdcp_config_security(
        &ctxt,
        ue_context_p,
        send_security_mode_command);

      if (send_security_mode_command) {

        rrc_eNB_generate_SecurityModeCommand (
          &ctxt,
          ue_context_p);
        send_security_mode_command = FALSE;
        // apply ciphering after RRC security command mode
        rrc_pdcp_config_security(
          &ctxt,
          ue_context_p,
          send_security_mode_command);
      } else {
        rrc_eNB_generate_UECapabilityEnquiry (&ctxt, ue_context_p);
      }
    }
    return (0);
  }
}

/*------------------------------------------------------------------------------*/
int rrc_eNB_process_S1AP_UE_CTXT_MODIFICATION_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance)
{
  uint32_t eNB_ue_s1ap_id;
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  protocol_ctxt_t              ctxt;

  eNB_ue_s1ap_id = S1AP_UE_CTXT_MODIFICATION_REQ (msg_p).eNB_ue_s1ap_id;
  ue_context_p   = rrc_eNB_get_ue_context_from_s1ap_ids(instance, UE_INITIAL_ID_INVALID, eNB_ue_s1ap_id);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index, send a failure to S1AP and discard it! */
    MessageDef *msg_fail_p;

    LOG_W(RRC, "[eNB %d] In S1AP_UE_CTXT_MODIFICATION_REQ: unknown UE from eNB_ue_s1ap_id (%d)\n", instance, eNB_ue_s1ap_id);

    msg_fail_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_UE_CTXT_MODIFICATION_FAIL);
    S1AP_UE_CTXT_MODIFICATION_FAIL (msg_fail_p).eNB_ue_s1ap_id = eNB_ue_s1ap_id;

    // TODO add failure cause when defined!

    itti_send_msg_to_task (TASK_S1AP, instance, msg_fail_p);
    return (-1);
  } else {

    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);
    /* TODO parameters yet to process ... */
    {
      if (S1AP_UE_CTXT_MODIFICATION_REQ(msg_p).present & S1AP_UE_CONTEXT_MODIFICATION_UE_AMBR) {
        //        S1AP_UE_CTXT_MODIFICATION_REQ(msg_p).ue_ambr;
      }
    }

    if (S1AP_UE_CTXT_MODIFICATION_REQ(msg_p).present & S1AP_UE_CONTEXT_MODIFICATION_UE_SECU_CAP) {
      if (rrc_eNB_process_security (
            &ctxt,
            ue_context_p,
            &S1AP_UE_CTXT_MODIFICATION_REQ(msg_p).security_capabilities)) {
        /* transmit the new security parameters to UE */
        rrc_eNB_generate_SecurityModeCommand (
          &ctxt,
          ue_context_p);
      }
    }

    if (S1AP_UE_CTXT_MODIFICATION_REQ(msg_p).present & S1AP_UE_CONTEXT_MODIFICATION_SECURITY_KEY) {
      process_eNB_security_key (
        &ctxt,
        ue_context_p,
        S1AP_UE_CTXT_MODIFICATION_REQ(msg_p).security_key);

      /* TODO reconfigure lower layers... */
    }

    /* Send the response */
    {
      MessageDef *msg_resp_p;

      msg_resp_p = itti_alloc_new_message(TASK_RRC_ENB, S1AP_UE_CTXT_MODIFICATION_RESP);
      S1AP_UE_CTXT_MODIFICATION_RESP(msg_resp_p).eNB_ue_s1ap_id = eNB_ue_s1ap_id;

      itti_send_msg_to_task(TASK_S1AP, instance, msg_resp_p);
    }

    return (0);
  }
}

/*------------------------------------------------------------------------------*/
int rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_REQ (MessageDef *msg_p, const char *msg_name, instance_t instance)
{
  uint32_t eNB_ue_s1ap_id;
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;

  eNB_ue_s1ap_id = S1AP_UE_CONTEXT_RELEASE_REQ(msg_p).eNB_ue_s1ap_id;
  ue_context_p   = rrc_eNB_get_ue_context_from_s1ap_ids(instance, UE_INITIAL_ID_INVALID, eNB_ue_s1ap_id);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index, send a failure to S1AP and discard it! */
    MessageDef *msg_fail_p;

    LOG_W(RRC, "[eNB %d] In S1AP_UE_CONTEXT_RELEASE_REQ: unknown UE from eNB_ue_s1ap_id (%d)\n",
          instance,
          eNB_ue_s1ap_id);

    msg_fail_p = itti_alloc_new_message(TASK_RRC_ENB, S1AP_UE_CONTEXT_RELEASE_RESP); /* TODO change message ID. */
    S1AP_UE_CONTEXT_RELEASE_RESP(msg_fail_p).eNB_ue_s1ap_id = eNB_ue_s1ap_id;

    // TODO add failure cause when defined!

    itti_send_msg_to_task(TASK_S1AP, instance, msg_fail_p);
    return (-1);
  } else {
    /* TODO release context. */

    /* Send the response */
    {
      MessageDef *msg_resp_p;

      msg_resp_p = itti_alloc_new_message(TASK_RRC_ENB, S1AP_UE_CONTEXT_RELEASE_RESP);
      S1AP_UE_CONTEXT_RELEASE_RESP(msg_resp_p).eNB_ue_s1ap_id = eNB_ue_s1ap_id;

      itti_send_msg_to_task(TASK_S1AP, instance, msg_resp_p);
    }

    return (0);
  }
}

//------------------------------------------------------------------------------
void rrc_eNB_send_S1AP_UE_CONTEXT_RELEASE_REQ (
  const module_id_t                        enb_mod_idP,
  const rrc_eNB_ue_context_t*        const ue_context_pP,
  const s1ap_Cause_t                       causeP,
  const long                               cause_valueP
)
//------------------------------------------------------------------------------
{
  if (ue_context_pP == NULL) {
    LOG_W(RRC,
          "[eNB] In S1AP_UE_CONTEXT_RELEASE_COMMAND: invalid  UE\n");
  } else {
	  MSC_LOG_TX_MESSAGE(
			  MSC_RRC_ENB,
	  		  MSC_S1AP_ENB,
	  		  NULL,0,
	  		  "0 S1AP_UE_CONTEXT_RELEASE_REQ eNB_ue_s1ap_id 0x%06"PRIX32" ",
	  		  ue_context_pP->ue_context.eNB_ue_s1ap_id);

    MessageDef *msg_context_release_req_p = NULL;
    msg_context_release_req_p = itti_alloc_new_message(TASK_RRC_ENB, S1AP_UE_CONTEXT_RELEASE_REQ);
    S1AP_UE_CONTEXT_RELEASE_REQ(msg_context_release_req_p).eNB_ue_s1ap_id = ue_context_pP->ue_context.eNB_ue_s1ap_id;
    S1AP_UE_CONTEXT_RELEASE_REQ(msg_context_release_req_p).cause          = causeP;
    S1AP_UE_CONTEXT_RELEASE_REQ(msg_context_release_req_p).cause_value    = cause_valueP;
    itti_send_msg_to_task(TASK_S1AP, ENB_MODULE_ID_TO_INSTANCE(enb_mod_idP), msg_context_release_req_p);
  }
}


/*------------------------------------------------------------------------------*/
int rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_COMMAND (MessageDef *msg_p, const char *msg_name, instance_t instance)
{
  uint32_t eNB_ue_s1ap_id;
  protocol_ctxt_t              ctxt;
  struct rrc_eNB_ue_context_s *ue_context_p = NULL;
  struct rrc_ue_s1ap_ids_s    *rrc_ue_s1ap_ids = NULL;

  eNB_ue_s1ap_id = S1AP_UE_CONTEXT_RELEASE_COMMAND(msg_p).eNB_ue_s1ap_id;


  ue_context_p   = rrc_eNB_get_ue_context_from_s1ap_ids(instance, UE_INITIAL_ID_INVALID, eNB_ue_s1ap_id);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index */
    MessageDef *msg_complete_p;

    LOG_W(RRC,
          "[eNB %d] In S1AP_UE_CONTEXT_RELEASE_COMMAND: unknown UE from eNB_ue_s1ap_id (%d)\n",
          instance,
          eNB_ue_s1ap_id);

    MSC_LOG_EVENT(
          MSC_RRC_ENB,
  		  "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE eNB_ue_s1ap_id 0x%06"PRIX32" context not found",
  		eNB_ue_s1ap_id);

    MSC_LOG_TX_MESSAGE(
          MSC_RRC_ENB,
  		  MSC_S1AP_ENB,
  		  NULL,0,
  		  "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE eNB_ue_s1ap_id 0x%06"PRIX32" ",
  		eNB_ue_s1ap_id);

    msg_complete_p = itti_alloc_new_message(TASK_RRC_ENB, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
    S1AP_UE_CONTEXT_RELEASE_COMPLETE(msg_complete_p).eNB_ue_s1ap_id = eNB_ue_s1ap_id;
    itti_send_msg_to_task(TASK_S1AP, instance, msg_complete_p);

    rrc_ue_s1ap_ids = rrc_eNB_S1AP_get_ue_ids(
    		RC.rrc[instance],
    		UE_INITIAL_ID_INVALID,
    		eNB_ue_s1ap_id);

    if (NULL != rrc_ue_s1ap_ids) {
      rrc_eNB_S1AP_remove_ue_ids(
    		  RC.rrc[instance],
    		  rrc_ue_s1ap_ids);
    }
    return (-1);
  } else {
    ue_context_p->ue_context.ue_release_timer_s1 = 0;
    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);
    rrc_eNB_generate_RRCConnectionRelease(&ctxt, ue_context_p);
    /*
          LOG_W(RRC,
                  "[eNB %d] In S1AP_UE_CONTEXT_RELEASE_COMMAND: TODO call rrc_eNB_connection_release for eNB %d\n",
                  instance,
                  eNB_ue_s1ap_id);
    */
    {
      int      e_rab;
      //int      mod_id = 0;
      MessageDef *msg_delete_tunnels_p = NULL;

      MSC_LOG_TX_MESSAGE(
            MSC_RRC_ENB,
            MSC_GTPU_ENB,
            NULL,0,
            "0 GTPV1U_ENB_DELETE_TUNNEL_REQ rnti %x ",
            eNB_ue_s1ap_id);

      msg_delete_tunnels_p = itti_alloc_new_message(TASK_RRC_ENB, GTPV1U_ENB_DELETE_TUNNEL_REQ);
      memset(&GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p),
             0,
             sizeof(GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p)));

      // do not wait response
      GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p).rnti = ue_context_p->ue_context.rnti;

      for (e_rab = 0; e_rab < ue_context_p->ue_context.nb_of_e_rabs; e_rab++) {
        GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p).eps_bearer_id[GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p).num_erab++] =
          ue_context_p->ue_context.enb_gtp_ebi[e_rab];
        // erase data
        ue_context_p->ue_context.enb_gtp_teid[e_rab] = 0;
        memset(&ue_context_p->ue_context.enb_gtp_addrs[e_rab], 0, sizeof(ue_context_p->ue_context.enb_gtp_addrs[e_rab]));
        ue_context_p->ue_context.enb_gtp_ebi[e_rab]  = 0;
      }

      itti_send_msg_to_task(TASK_GTPV1_U, instance, msg_delete_tunnels_p);


      MSC_LOG_TX_MESSAGE(
            MSC_RRC_ENB,
            MSC_S1AP_ENB,
            NULL,0,
            "0 S1AP_UE_CONTEXT_RELEASE_COMPLETE eNB_ue_s1ap_id 0x%06"PRIX32" ",
            eNB_ue_s1ap_id);

      MessageDef *msg_complete_p = NULL;
      msg_complete_p = itti_alloc_new_message(TASK_RRC_ENB, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
      S1AP_UE_CONTEXT_RELEASE_COMPLETE(msg_complete_p).eNB_ue_s1ap_id = eNB_ue_s1ap_id;
      itti_send_msg_to_task(TASK_S1AP, instance, msg_complete_p);

      rrc_ue_s1ap_ids = rrc_eNB_S1AP_get_ue_ids(
      		RC.rrc[instance],
      		UE_INITIAL_ID_INVALID,
      		eNB_ue_s1ap_id);

      if (NULL != rrc_ue_s1ap_ids) {
        rrc_eNB_S1AP_remove_ue_ids(
      		  RC.rrc[instance],
      		  rrc_ue_s1ap_ids);
      }
    }

    return (0);
  }
}

int rrc_eNB_process_S1AP_E_RAB_SETUP_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance)
{
  uint16_t                        ue_initial_id;
  uint32_t                        eNB_ue_s1ap_id;
  gtpv1u_enb_create_tunnel_req_t  create_tunnel_req;
  gtpv1u_enb_create_tunnel_resp_t create_tunnel_resp;
  uint8_t                         inde_list[NB_RB_MAX - 3]={0};

  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  protocol_ctxt_t              ctxt;
  uint8_t                      e_rab_done;

  ue_initial_id  = S1AP_E_RAB_SETUP_REQ (msg_p).ue_initial_id;
  eNB_ue_s1ap_id = S1AP_E_RAB_SETUP_REQ (msg_p).eNB_ue_s1ap_id;
  ue_context_p   = rrc_eNB_get_ue_context_from_s1ap_ids(instance, ue_initial_id, eNB_ue_s1ap_id);
  LOG_I(RRC, "[eNB %d] Received %s: ue_initial_id %d, eNB_ue_s1ap_id %d, nb_of_e_rabs %d\n",
        instance, msg_name, ue_initial_id, eNB_ue_s1ap_id, S1AP_E_RAB_SETUP_REQ (msg_p).nb_e_rabs_tosetup);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index, send a failure to S1AP and discard it! */
    MessageDef *msg_fail_p = NULL;

    LOG_W(RRC, "[eNB %d] In S1AP_E_RAB_SETUP_REQ: unknown UE from S1AP ids (%d, %d)\n", instance, ue_initial_id, eNB_ue_s1ap_id);

    msg_fail_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_E_RAB_SETUP_REQUEST_FAIL);
    S1AP_E_RAB_SETUP_REQ  (msg_fail_p).eNB_ue_s1ap_id = eNB_ue_s1ap_id;

    // TODO add failure cause when defined!

    itti_send_msg_to_task (TASK_S1AP, instance, msg_fail_p);
    return (-1);
  } else {

    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);
    ue_context_p->ue_context.eNB_ue_s1ap_id = S1AP_E_RAB_SETUP_REQ  (msg_p).eNB_ue_s1ap_id;

    /* Save e RAB information for later */
    {
      int i;

      memset(&create_tunnel_req, 0 , sizeof(create_tunnel_req));
      uint8_t nb_e_rabs_tosetup = S1AP_E_RAB_SETUP_REQ  (msg_p).nb_e_rabs_tosetup;
      e_rab_done = 0;

      // keep the previous bearer
      // the index for the rec
      for (i = 0; 
	  // i < nb_e_rabs_tosetup; 
           i < NB_RB_MAX - 3;  // loop all e-rabs in e_rab[] 
	   i++) {
	//if (ue_context_p->ue_context.e_rab[i+ue_context_p->ue_context.setup_e_rabs].status == E_RAB_STATUS_DONE) 
	//  LOG_W(RRC,"E-RAB already configured, reconfiguring\n");
        // check e-rab status, if e rab status is greater than E_RAB_STATUS_DONE, don't not config this one
        if(ue_context_p->ue_context.e_rab[i].status >= E_RAB_STATUS_DONE)
            continue;
        //ue_context_p->ue_context.e_rab[i+ue_context_p->ue_context.setup_e_rabs].status = E_RAB_STATUS_NEW;
        //ue_context_p->ue_context.e_rab[i+ue_context_p->ue_context.setup_e_rabs].param = S1AP_E_RAB_SETUP_REQ  (msg_p).e_rab_setup_params[i];
        ue_context_p->ue_context.e_rab[i].status = E_RAB_STATUS_NEW;
        ue_context_p->ue_context.e_rab[i].param = S1AP_E_RAB_SETUP_REQ  (msg_p).e_rab_setup_params[e_rab_done];

        create_tunnel_req.eps_bearer_id[e_rab_done]       = S1AP_E_RAB_SETUP_REQ  (msg_p).e_rab_setup_params[e_rab_done].e_rab_id;
        create_tunnel_req.sgw_S1u_teid[e_rab_done]        = S1AP_E_RAB_SETUP_REQ  (msg_p).e_rab_setup_params[e_rab_done].gtp_teid;

        memcpy(&create_tunnel_req.sgw_addr[e_rab_done],
               & S1AP_E_RAB_SETUP_REQ (msg_p).e_rab_setup_params[e_rab_done].sgw_addr,
               sizeof(transport_layer_addr_t));
	
	LOG_I(RRC,"E_RAB setup REQ: local index %d teid %u, eps id %d \n", 
	      i,
	      create_tunnel_req.sgw_S1u_teid[e_rab_done],
	       create_tunnel_req.eps_bearer_id[i] );
        inde_list[e_rab_done] = i;
        e_rab_done++;        
        if(e_rab_done >= nb_e_rabs_tosetup){
            break;
        }
      }
      ue_context_p->ue_context.nb_of_e_rabs=nb_e_rabs_tosetup;
     
     
      create_tunnel_req.rnti       = ue_context_p->ue_context.rnti; // warning put zero above
      create_tunnel_req.num_tunnels    = e_rab_done;
      
      // NN: not sure if we should create a new tunnel: need to check teid, etc.
      gtpv1u_create_s1u_tunnel(
        instance,
        &create_tunnel_req,
        &create_tunnel_resp);

      rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP(
          &ctxt,
          &create_tunnel_resp,
          &inde_list[0]);

      ue_context_p->ue_context.setup_e_rabs+=nb_e_rabs_tosetup;

    }

    /* TODO parameters yet to process ... */
    {
      //      S1AP_INITIAL_CONTEXT_SETUP_REQ(msg_p).ue_ambr;
    }

    rrc_eNB_generate_dedicatedRRCConnectionReconfiguration(&ctxt, ue_context_p, 0);
							 
    return (0);
  }
}

/*NN: careful about the typcast of xid (long -> uint8_t*/
int rrc_eNB_send_S1AP_E_RAB_SETUP_RESP(const protocol_ctxt_t* const ctxt_pP,
				   rrc_eNB_ue_context_t*          const ue_context_pP,
				   uint8_t xid ){  

  MessageDef      *msg_p         = NULL;
  int e_rab;
  int e_rabs_done = 0;
  int e_rabs_failed = 0;
    
  msg_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_E_RAB_SETUP_RESP);
  S1AP_E_RAB_SETUP_RESP (msg_p).eNB_ue_s1ap_id = ue_context_pP->ue_context.eNB_ue_s1ap_id;
 
  for (e_rab = 0; e_rab <  ue_context_pP->ue_context.setup_e_rabs ; e_rab++) {

    /* only respond to the corresponding transaction */ 
    //if (((xid+1)%4) == ue_context_pP->ue_context.e_rab[e_rab].xid) {
    if (xid == ue_context_pP->ue_context.e_rab[e_rab].xid) {
      
      if (ue_context_pP->ue_context.e_rab[e_rab].status == E_RAB_STATUS_DONE) {
     
	S1AP_E_RAB_SETUP_RESP (msg_p).e_rabs[e_rabs_done].e_rab_id = ue_context_pP->ue_context.e_rab[e_rab].param.e_rab_id;
	// TODO add other information from S1-U when it will be integrated
	S1AP_E_RAB_SETUP_RESP (msg_p).e_rabs[e_rabs_done].gtp_teid = ue_context_pP->ue_context.enb_gtp_teid[e_rab];
	S1AP_E_RAB_SETUP_RESP (msg_p).e_rabs[e_rabs_done].eNB_addr = ue_context_pP->ue_context.enb_gtp_addrs[e_rab];
	//S1AP_E_RAB_SETUP_RESP (msg_p).e_rabs[e_rab].eNB_addr.length += 4;
	ue_context_pP->ue_context.e_rab[e_rab].status = E_RAB_STATUS_ESTABLISHED;
	
	LOG_I (RRC,"enb_gtp_addr (msg index %d, e_rab index %d, status %d, xid %d): nb_of_e_rabs %d,  e_rab_id %d, teid: %u, addr: %d.%d.%d.%d \n ",
	       e_rabs_done,  e_rab, ue_context_pP->ue_context.e_rab[e_rab].status, xid,
	       ue_context_pP->ue_context.nb_of_e_rabs,
	       S1AP_E_RAB_SETUP_RESP (msg_p).e_rabs[e_rabs_done].e_rab_id,
	       S1AP_E_RAB_SETUP_RESP (msg_p).e_rabs[e_rabs_done].gtp_teid,
	       S1AP_E_RAB_SETUP_RESP (msg_p).e_rabs[e_rabs_done].eNB_addr.buffer[0],
	       S1AP_E_RAB_SETUP_RESP (msg_p).e_rabs[e_rabs_done].eNB_addr.buffer[1],
	       S1AP_E_RAB_SETUP_RESP (msg_p).e_rabs[e_rabs_done].eNB_addr.buffer[2],
	       S1AP_E_RAB_SETUP_RESP (msg_p).e_rabs[e_rabs_done].eNB_addr.buffer[3]);
	
	e_rabs_done++;
      } else if ((ue_context_pP->ue_context.e_rab[e_rab].status == E_RAB_STATUS_NEW)  || 
		 (ue_context_pP->ue_context.e_rab[e_rab].status == E_RAB_STATUS_ESTABLISHED)){
	LOG_D (RRC,"E-RAB is NEW or already ESTABLISHED\n");
      }else { /* to be improved */
	ue_context_pP->ue_context.e_rab[e_rab].status = E_RAB_STATUS_FAILED;
	S1AP_E_RAB_SETUP_RESP  (msg_p).e_rabs_failed[e_rabs_failed].e_rab_id = ue_context_pP->ue_context.e_rab[e_rab].param.e_rab_id;
	e_rabs_failed++;
	// TODO add cause when it will be integrated
      } 
      
      S1AP_E_RAB_SETUP_RESP (msg_p).nb_of_e_rabs = e_rabs_done;
      S1AP_E_RAB_SETUP_RESP (msg_p).nb_of_e_rabs_failed = e_rabs_failed;
      // NN: add conditions for e_rabs_failed 
    } else {
      /*debug info for the xid */ 
      LOG_D(RRC,"xid does not corresponds  (context e_rab index %d, status %d, xid %d/%d) \n ",
      	     e_rab, ue_context_pP->ue_context.e_rab[e_rab].status, xid, ue_context_pP->ue_context.e_rab[e_rab].xid);
    } 
  }
      if ((e_rabs_done > 0) ){  

	LOG_I(RRC,"S1AP_E_RAB_SETUP_RESP: sending the message: nb_of_erabs %d, total e_rabs %d, index %d\n",
	      ue_context_pP->ue_context.nb_of_e_rabs, ue_context_pP->ue_context.setup_e_rabs, e_rab);
	MSC_LOG_TX_MESSAGE(
			   MSC_RRC_ENB,
			   MSC_S1AP_ENB,
			   (const char *)&S1AP_E_RAB_SETUP_RESP (msg_p),
			   sizeof(s1ap_e_rab_setup_resp_t),
			   MSC_AS_TIME_FMT" E_RAB_SETUP_RESP UE %X eNB_ue_s1ap_id %u e_rabs:%u succ %u fail",
			   MSC_AS_TIME_ARGS(ctxt_pP),
			   ue_context_pP->ue_id_rnti,
			   S1AP_E_RAB_SETUP_RESP (msg_p).eNB_ue_s1ap_id,
			   e_rabs_done, e_rabs_failed);
	
	
	itti_send_msg_to_task (TASK_S1AP, ctxt_pP->instance, msg_p);
      }
  for(int i = 0; i < NB_RB_MAX; i++) {
      ue_context_pP->ue_context.e_rab[i].xid = -1;
  }
  
  return 0;
}

int rrc_eNB_process_S1AP_E_RAB_MODIFY_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance)
{
  int                             i;
  uint16_t                        ue_initial_id;
  uint32_t                        eNB_ue_s1ap_id;
  struct rrc_eNB_ue_context_s* ue_context_p = NULL;
  protocol_ctxt_t              ctxt;

  ue_initial_id  = S1AP_E_RAB_MODIFY_REQ (msg_p).ue_initial_id;
  eNB_ue_s1ap_id = S1AP_E_RAB_MODIFY_REQ (msg_p).eNB_ue_s1ap_id;
  ue_context_p   = rrc_eNB_get_ue_context_from_s1ap_ids(instance, ue_initial_id, eNB_ue_s1ap_id);
  LOG_D(RRC, "[eNB %d] Received %s: ue_initial_id %d, eNB_ue_s1ap_id %d, nb_of_e_rabs %d\n",
        instance, msg_name, ue_initial_id, eNB_ue_s1ap_id, S1AP_E_RAB_MODIFY_REQ (msg_p).nb_e_rabs_tomodify);

  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index, send a failure to S1AP and discard it! */
    LOG_W(RRC, "[eNB %d] In S1AP_E_RAB_MODIFY_REQ: unknown UE from S1AP ids (%d, %d)\n", instance, ue_initial_id, eNB_ue_s1ap_id);
    int nb_of_e_rabs_failed = 0;
    MessageDef *msg_fail_p = NULL;

    msg_fail_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_E_RAB_MODIFY_RESP);

    S1AP_E_RAB_MODIFY_RESP (msg_fail_p).eNB_ue_s1ap_id = S1AP_E_RAB_MODIFY_REQ (msg_p).eNB_ue_s1ap_id;
    S1AP_E_RAB_MODIFY_RESP (msg_fail_p).nb_of_e_rabs = 0;

    for (nb_of_e_rabs_failed = 0; nb_of_e_rabs_failed < S1AP_E_RAB_MODIFY_REQ (msg_p).nb_e_rabs_tomodify; nb_of_e_rabs_failed++) {
      S1AP_E_RAB_MODIFY_RESP (msg_fail_p).e_rabs_failed[nb_of_e_rabs_failed].e_rab_id =
              S1AP_E_RAB_MODIFY_REQ (msg_p).e_rab_modify_params[nb_of_e_rabs_failed].e_rab_id;
      S1AP_E_RAB_MODIFY_RESP (msg_fail_p).e_rabs_failed[nb_of_e_rabs_failed].cause = S1AP_CAUSE_RADIO_NETWORK;
      S1AP_E_RAB_MODIFY_RESP (msg_fail_p).e_rabs_failed[nb_of_e_rabs_failed].cause_value = 31;//S1ap_CauseRadioNetwork_multiple_E_RAB_ID_instances;
    }
    S1AP_E_RAB_MODIFY_RESP (msg_fail_p).nb_of_e_rabs_failed = nb_of_e_rabs_failed;

    itti_send_msg_to_task(TASK_S1AP, instance, msg_fail_p);
    return (-1);

  } else {
    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);
    ue_context_p->ue_context.eNB_ue_s1ap_id = eNB_ue_s1ap_id;

    /* Save e RAB information for later */
    {
      int j;
      boolean_t is_treated[S1AP_MAX_E_RAB] = {FALSE};
      uint8_t nb_of_failed_e_rabs = 0;

      // keep the previous bearer
      // the index for the rec
      for (i = 0; i < S1AP_E_RAB_MODIFY_REQ (msg_p).nb_e_rabs_tomodify; i++) {
        if (is_treated[i] == TRUE) {
          // already treated
          continue;
        }
        for (j = i+1; j < S1AP_E_RAB_MODIFY_REQ (msg_p).nb_e_rabs_tomodify; j++) {
          if (is_treated[j] == FALSE &&
            S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[j].e_rab_id == S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[i].e_rab_id) {
            // handle multiple E-RAB ID
            ue_context_p->ue_context.modify_e_rab[j].status = E_RAB_STATUS_NEW;
            ue_context_p->ue_context.modify_e_rab[j].param.e_rab_id = S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[j].e_rab_id;
            ue_context_p->ue_context.modify_e_rab[j].cause = S1AP_CAUSE_RADIO_NETWORK;
            ue_context_p->ue_context.modify_e_rab[j].cause_value = 31;//S1ap_CauseRadioNetwork_multiple_E_RAB_ID_instances;
            nb_of_failed_e_rabs++;
            is_treated[i] = TRUE;
            is_treated[j] = TRUE;
          }
        }
        if (is_treated[i] == TRUE) {
          // handle multiple E-RAB ID
          ue_context_p->ue_context.modify_e_rab[i].status = E_RAB_STATUS_NEW;
          ue_context_p->ue_context.modify_e_rab[i].param.e_rab_id = S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[i].e_rab_id;
          ue_context_p->ue_context.modify_e_rab[i].cause = S1AP_CAUSE_RADIO_NETWORK;
          ue_context_p->ue_context.modify_e_rab[i].cause_value = 31;//S1ap_CauseRadioNetwork_multiple_E_RAB_ID_instances;
          nb_of_failed_e_rabs++;
          continue;
        }

        if (S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[i].nas_pdu.length == 0) {
          // nas_pdu.length == 0
          ue_context_p->ue_context.modify_e_rab[i].status = E_RAB_STATUS_NEW;
          ue_context_p->ue_context.modify_e_rab[i].param.e_rab_id = S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[i].e_rab_id;
          ue_context_p->ue_context.modify_e_rab[i].cause = S1AP_CAUSE_NAS;
          ue_context_p->ue_context.modify_e_rab[i].cause_value = 3;//S1ap_CauseNas_unspecified;
          nb_of_failed_e_rabs++;
          is_treated[i] = TRUE;
          continue;
        }

        for (j = 0; j < NB_RB_MAX-3; j++) {
          if (ue_context_p->ue_context.e_rab[j].param.e_rab_id == S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[i].e_rab_id) {
            if(ue_context_p->ue_context.e_rab[j].status == E_RAB_STATUS_TORELEASE || ue_context_p->ue_context.e_rab[j].status == E_RAB_STATUS_DONE){
              break;
            }
            ue_context_p->ue_context.modify_e_rab[i].status = E_RAB_STATUS_NEW;
            ue_context_p->ue_context.modify_e_rab[i].cause = S1AP_CAUSE_NOTHING;
            ue_context_p->ue_context.modify_e_rab[i].param.e_rab_id = S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[i].e_rab_id;
            ue_context_p->ue_context.modify_e_rab[i].param.qos =  S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[i].qos;
            ue_context_p->ue_context.modify_e_rab[i].param.nas_pdu.length = S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[i].nas_pdu.length;
            ue_context_p->ue_context.modify_e_rab[i].param.nas_pdu.buffer = S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[i].nas_pdu.buffer;
            ue_context_p->ue_context.modify_e_rab[i].param.sgw_addr = ue_context_p->ue_context.e_rab[j].param.sgw_addr;
            ue_context_p->ue_context.modify_e_rab[i].param.gtp_teid = ue_context_p->ue_context.e_rab[j].param.gtp_teid;

            is_treated[i] = TRUE;
            break;
          }
        }

        if (is_treated[i] == FALSE) {
          // handle Unknown E-RAB ID
          ue_context_p->ue_context.modify_e_rab[i].status = E_RAB_STATUS_NEW;
          ue_context_p->ue_context.modify_e_rab[i].param.e_rab_id = S1AP_E_RAB_MODIFY_REQ(msg_p).e_rab_modify_params[i].e_rab_id;
          ue_context_p->ue_context.modify_e_rab[i].cause = S1AP_CAUSE_RADIO_NETWORK;
          ue_context_p->ue_context.modify_e_rab[i].cause_value = 30;//S1ap_CauseRadioNetwork_unknown_E_RAB_ID;
          nb_of_failed_e_rabs++;
          is_treated[i] = TRUE;
        }
      }

      ue_context_p->ue_context.nb_of_modify_e_rabs = S1AP_E_RAB_MODIFY_REQ  (msg_p).nb_e_rabs_tomodify;
      ue_context_p->ue_context.nb_of_failed_e_rabs = nb_of_failed_e_rabs;
    }

    /* TODO parameters yet to process ... */
    {
      //      S1AP_INITIAL_CONTEXT_SETUP_REQ(msg_p).ue_ambr;
    }

    if (ue_context_p->ue_context.nb_of_failed_e_rabs < ue_context_p->ue_context.nb_of_modify_e_rabs) {
      if (0 == rrc_eNB_modify_dedicatedRRCConnectionReconfiguration(&ctxt, ue_context_p, 0)) {
        return (0);
      }
    }

    {
      int nb_of_e_rabs_failed = 0;
      MessageDef *msg_fail_p = NULL;

      msg_fail_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_E_RAB_MODIFY_RESP);

      S1AP_E_RAB_MODIFY_RESP (msg_fail_p).eNB_ue_s1ap_id = S1AP_E_RAB_MODIFY_REQ (msg_p).eNB_ue_s1ap_id;
//      S1AP_E_RAB_MODIFY_RESP (msg_fail_p).e_rabs[S1AP_MAX_E_RAB];
      S1AP_E_RAB_MODIFY_RESP (msg_fail_p).nb_of_e_rabs = 0;

      for(nb_of_e_rabs_failed = 0; nb_of_e_rabs_failed < ue_context_p->ue_context.nb_of_failed_e_rabs; nb_of_e_rabs_failed++) {
        S1AP_E_RAB_MODIFY_RESP (msg_fail_p).e_rabs_failed[nb_of_e_rabs_failed].e_rab_id =
                ue_context_p->ue_context.modify_e_rab[nb_of_e_rabs_failed].param.e_rab_id;
        S1AP_E_RAB_MODIFY_RESP (msg_fail_p).e_rabs_failed[nb_of_e_rabs_failed].cause = ue_context_p->ue_context.modify_e_rab[nb_of_e_rabs_failed].cause;
      }
      S1AP_E_RAB_MODIFY_RESP (msg_fail_p).nb_of_e_rabs_failed = nb_of_e_rabs_failed;

      itti_send_msg_to_task (TASK_S1AP, instance, msg_fail_p);

      ue_context_p->ue_context.nb_of_modify_e_rabs = 0;
      ue_context_p->ue_context.nb_of_failed_e_rabs = 0;
      memset(ue_context_p->ue_context.modify_e_rab, 0, sizeof(ue_context_p->ue_context.modify_e_rab));

      return (0);
    }
  }  // end of ue_context_p != NULL
}

/*NN: careful about the typcast of xid (long -> uint8_t*/
int rrc_eNB_send_S1AP_E_RAB_MODIFY_RESP(const protocol_ctxt_t* const ctxt_pP,
           rrc_eNB_ue_context_t*          const ue_context_pP,
           uint8_t xid ) {

  MessageDef      *msg_p         = NULL;
  int i;
  int e_rab;
  int e_rabs_done = 0;
  int e_rabs_failed = 0;
  msg_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_E_RAB_MODIFY_RESP);
  S1AP_E_RAB_MODIFY_RESP (msg_p).eNB_ue_s1ap_id = ue_context_pP->ue_context.eNB_ue_s1ap_id;

  for (e_rab = 0; e_rab < ue_context_pP->ue_context.nb_of_modify_e_rabs; e_rab++) {

    /* only respond to the corresponding transaction */
    if (xid == ue_context_pP->ue_context.modify_e_rab[e_rab].xid) {
      if (ue_context_pP->ue_context.modify_e_rab[e_rab].status == E_RAB_STATUS_DONE) {
        for (i = 0; i < ue_context_pP->ue_context.setup_e_rabs; i++) {
          if (ue_context_pP->ue_context.modify_e_rab[e_rab].param.e_rab_id == ue_context_pP->ue_context.e_rab[i].param.e_rab_id) {
            // Update ue_context_pP->ue_context.e_rab
            ue_context_pP->ue_context.e_rab[i].status = E_RAB_STATUS_ESTABLISHED;
            ue_context_pP->ue_context.e_rab[i].param.qos = ue_context_pP->ue_context.modify_e_rab[e_rab].param.qos;
            ue_context_pP->ue_context.e_rab[i].cause = S1AP_CAUSE_NOTHING;
            break;
          }
        }
        if (i < ue_context_pP->ue_context.setup_e_rabs) {
          S1AP_E_RAB_MODIFY_RESP (msg_p).e_rabs[e_rabs_done].e_rab_id = ue_context_pP->ue_context.modify_e_rab[e_rab].param.e_rab_id;
              // TODO add other information from S1-U when it will be integrated

          LOG_D (RRC,"enb_gtp_addr (msg index %d, e_rab index %d, status %d, xid %d): nb_of_modify_e_rabs %d,  e_rab_id %d \n ",
              e_rabs_done,  e_rab, ue_context_pP->ue_context.modify_e_rab[e_rab].status, xid,
              ue_context_pP->ue_context.nb_of_modify_e_rabs,
              S1AP_E_RAB_MODIFY_RESP (msg_p).e_rabs[e_rabs_done].e_rab_id);

          e_rabs_done++;
        } else {
          // unexpected
          S1AP_E_RAB_MODIFY_RESP (msg_p).e_rabs_failed[e_rabs_failed].e_rab_id = ue_context_pP->ue_context.modify_e_rab[e_rab].param.e_rab_id;

          S1AP_E_RAB_MODIFY_RESP (msg_p).e_rabs_failed[e_rabs_failed].cause = S1AP_CAUSE_RADIO_NETWORK;
          S1AP_E_RAB_MODIFY_RESP (msg_p).e_rabs_failed[e_rabs_failed].cause_value = 30;//S1ap_CauseRadioNetwork_unknown_E_RAB_ID;

          e_rabs_failed++;
        }
      } else if ((ue_context_pP->ue_context.modify_e_rab[e_rab].status == E_RAB_STATUS_NEW) ||
          (ue_context_pP->ue_context.modify_e_rab[e_rab].status == E_RAB_STATUS_ESTABLISHED)){
        LOG_D (RRC,"E-RAB is NEW or already ESTABLISHED\n");
      } else {  /* status == E_RAB_STATUS_FAILED; */
        S1AP_E_RAB_MODIFY_RESP (msg_p).e_rabs_failed[e_rabs_failed].e_rab_id = ue_context_pP->ue_context.modify_e_rab[e_rab].param.e_rab_id;
        // add failure cause when defined
        S1AP_E_RAB_MODIFY_RESP (msg_p).e_rabs_failed[e_rabs_failed].cause = ue_context_pP->ue_context.modify_e_rab[e_rab].cause;

        e_rabs_failed++;
      }
    } else {
      /*debug info for the xid */
      LOG_D(RRC,"xid does not corresponds  (context e_rab index %d, status %d, xid %d/%d) \n ",
             e_rab, ue_context_pP->ue_context.modify_e_rab[e_rab].status, xid, ue_context_pP->ue_context.modify_e_rab[e_rab].xid);
    }
  }


  S1AP_E_RAB_MODIFY_RESP (msg_p).nb_of_e_rabs = e_rabs_done;
  S1AP_E_RAB_MODIFY_RESP (msg_p).nb_of_e_rabs_failed = e_rabs_failed;
  // NN: add conditions for e_rabs_failed
  if (e_rabs_done > 0 || e_rabs_failed > 0) {
    LOG_D(RRC,"S1AP_E_RAB_MODIFY_RESP: sending the message: nb_of_modify_e_rabs %d, total e_rabs %d, index %d\n",
    ue_context_pP->ue_context.nb_of_modify_e_rabs, ue_context_pP->ue_context.setup_e_rabs, e_rab);
MSC_LOG_TX_MESSAGE(
     MSC_RRC_ENB,
     MSC_S1AP_ENB,
     (const char *)&S1AP_E_RAB_SETUP_RESP (msg_p),
     sizeof(s1ap_e_rab_setup_resp_t),
     MSC_AS_TIME_FMT" E_RAB_MODIFY_RESP UE %X eNB_ue_s1ap_id %u e_rabs:%u succ %u fail",
     MSC_AS_TIME_ARGS(ctxt_pP),
     ue_context_pP->ue_id_rnti,
     S1AP_E_RAB_MODIFY_RESP (msg_p).eNB_ue_s1ap_id,
     e_rabs_done, e_rabs_failed);

    itti_send_msg_to_task (TASK_S1AP, ctxt_pP->instance, msg_p);
  }

  return 0;
}
int rrc_eNB_process_S1AP_E_RAB_RELEASE_COMMAND(MessageDef *msg_p, const char *msg_name, instance_t instance){
    uint16_t                        mme_ue_s1ap_id;
    uint32_t                        eNB_ue_s1ap_id;
    struct rrc_eNB_ue_context_s*    ue_context_p = NULL;
    protocol_ctxt_t                 ctxt;
    e_rab_release_t e_rab_release_params[S1AP_MAX_E_RAB];
    uint8_t nb_e_rabs_torelease;
    int erab;
    int i;
    uint8_t b_existed,is_existed;
    uint8_t xid;
    uint8_t e_rab_release_drb;
    MessageDef *                    msg_delete_tunnels_p = NULL;
    e_rab_release_drb = 0;
    memcpy(&e_rab_release_params[0], &(S1AP_E_RAB_RELEASE_COMMAND (msg_p).e_rab_release_params[0]), sizeof(e_rab_release_t)*S1AP_MAX_E_RAB);

    mme_ue_s1ap_id  = S1AP_E_RAB_RELEASE_COMMAND (msg_p).mme_ue_s1ap_id;
    eNB_ue_s1ap_id = S1AP_E_RAB_RELEASE_COMMAND (msg_p).eNB_ue_s1ap_id;
    nb_e_rabs_torelease = S1AP_E_RAB_RELEASE_COMMAND (msg_p).nb_e_rabs_torelease;
    ue_context_p   = rrc_eNB_get_ue_context_from_s1ap_ids(instance, UE_INITIAL_ID_INVALID, eNB_ue_s1ap_id);
    if(ue_context_p != NULL){
        PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, instance, ENB_FLAG_YES, ue_context_p->ue_context.rnti, 0, 0);

        xid = rrc_eNB_get_next_transaction_identifier(ctxt.module_id);

        LOG_D(RRC,"S1AP-E-RAB Release Command: MME_UE_S1AP_ID %d  ENB_UE_S1AP_ID %d release_e_rabs %d \n",
            mme_ue_s1ap_id, eNB_ue_s1ap_id,nb_e_rabs_torelease);
        for(erab = 0; erab < nb_e_rabs_torelease; erab++){
            b_existed = 0;
            is_existed = 0;
            for ( i = erab-1;  i>= 0; i--){
                if (e_rab_release_params[erab].e_rab_id == e_rab_release_params[i].e_rab_id){
                    is_existed = 1;
                    break;
                }
            }
            if(is_existed == 1){
                //e_rab_id is existed
                continue;
            }
            for ( i = 0;  i < NB_RB_MAX; i++){
                if (e_rab_release_params[erab].e_rab_id == ue_context_p->ue_context.e_rab[i].param.e_rab_id){
                    b_existed = 1;
                    break;
                }
            }
            if(b_existed == 0) {
                //no e_rab_id
                ue_context_p->ue_context.e_rabs_release_failed[ue_context_p->ue_context.nb_release_of_e_rabs].e_rab_id = e_rab_release_params[erab].e_rab_id;
                ue_context_p->ue_context.e_rabs_release_failed[ue_context_p->ue_context.nb_release_of_e_rabs].cause = S1AP_CAUSE_RADIO_NETWORK;
                ue_context_p->ue_context.e_rabs_release_failed[ue_context_p->ue_context.nb_release_of_e_rabs].cause_value = 30;
                ue_context_p->ue_context.nb_release_of_e_rabs++;
            } else {
                if(ue_context_p->ue_context.e_rab[i].status == E_RAB_STATUS_FAILED){
                    ue_context_p->ue_context.e_rab[i].xid = xid;
                    continue;
                } else if(ue_context_p->ue_context.e_rab[i].status == E_RAB_STATUS_ESTABLISHED){
                    ue_context_p->ue_context.e_rab[i].status = E_RAB_STATUS_TORELEASE;
                    ue_context_p->ue_context.e_rab[i].xid = xid;
                    e_rab_release_drb++;
                }else{
                    //e_rab_id status NG
                    ue_context_p->ue_context.e_rabs_release_failed[ue_context_p->ue_context.nb_release_of_e_rabs].e_rab_id = e_rab_release_params[erab].e_rab_id;
                    ue_context_p->ue_context.e_rabs_release_failed[ue_context_p->ue_context.nb_release_of_e_rabs].cause = S1AP_CAUSE_RADIO_NETWORK;
                    ue_context_p->ue_context.e_rabs_release_failed[ue_context_p->ue_context.nb_release_of_e_rabs].cause_value = 0;
                    ue_context_p->ue_context.nb_release_of_e_rabs++;
                }
            }
        }
        if(e_rab_release_drb > 0) {
            //RRCConnectionReconfiguration To UE
            rrc_eNB_generate_dedicatedRRCConnectionReconfiguration_release(&ctxt, ue_context_p, xid, S1AP_E_RAB_RELEASE_COMMAND (msg_p).nas_pdu.length, S1AP_E_RAB_RELEASE_COMMAND (msg_p).nas_pdu.buffer);
        } else {
            //gtp tunnel delete
            msg_delete_tunnels_p = itti_alloc_new_message(TASK_RRC_ENB, GTPV1U_ENB_DELETE_TUNNEL_REQ);
            memset(&GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p), 0, sizeof(GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p)));
            GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p).rnti = ue_context_p->ue_context.rnti;
            for(i = 0; i < NB_RB_MAX; i++){
               if(xid == ue_context_p->ue_context.e_rab[i].xid){
                 GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p).eps_bearer_id[GTPV1U_ENB_DELETE_TUNNEL_REQ(msg_delete_tunnels_p).num_erab++] = ue_context_p->ue_context.enb_gtp_ebi[i];
                 ue_context_p->ue_context.enb_gtp_teid[i] = 0;
                 memset(&ue_context_p->ue_context.enb_gtp_addrs[i], 0, sizeof(ue_context_p->ue_context.enb_gtp_addrs[i]));
                 ue_context_p->ue_context.enb_gtp_ebi[i]  = 0;
               }
            }

            itti_send_msg_to_task(TASK_GTPV1_U, instance, msg_delete_tunnels_p);

            //S1AP_E_RAB_RELEASE_RESPONSE
            rrc_eNB_send_S1AP_E_RAB_RELEASE_RESPONSE(&ctxt, ue_context_p, xid);
        }
    } else {
        LOG_E(RRC,"S1AP-E-RAB Release Command: MME_UE_S1AP_ID %d  ENB_UE_S1AP_ID %d  Error ue_context_p NULL \n",
            S1AP_E_RAB_RELEASE_COMMAND (msg_p).mme_ue_s1ap_id, S1AP_E_RAB_RELEASE_COMMAND (msg_p).eNB_ue_s1ap_id);
         return -1;
    }

    return 0;
}


int rrc_eNB_send_S1AP_E_RAB_RELEASE_RESPONSE(const protocol_ctxt_t* const ctxt_pP, rrc_eNB_ue_context_t* const ue_context_pP, uint8_t xid){
    int e_rabs_released = 0;
    MessageDef   *msg_p;

    msg_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_E_RAB_RELEASE_RESPONSE);
    S1AP_E_RAB_RELEASE_RESPONSE (msg_p).eNB_ue_s1ap_id = ue_context_pP->ue_context.eNB_ue_s1ap_id;
    
    for (int i = 0;  i < NB_RB_MAX; i++){
        if (xid == ue_context_pP->ue_context.e_rab[i].xid){
            S1AP_E_RAB_RELEASE_RESPONSE (msg_p).e_rab_release[e_rabs_released].e_rab_id = ue_context_pP->ue_context.e_rab[i].param.e_rab_id;
            e_rabs_released++;
            //clear
            memset(&ue_context_pP->ue_context.e_rab[i],0,sizeof(e_rab_param_t));
        }
    }
    S1AP_E_RAB_RELEASE_RESPONSE (msg_p).nb_of_e_rabs_released = e_rabs_released;
    S1AP_E_RAB_RELEASE_RESPONSE (msg_p).nb_of_e_rabs_failed = ue_context_pP->ue_context.nb_release_of_e_rabs;
    memcpy(&(S1AP_E_RAB_RELEASE_RESPONSE (msg_p).e_rabs_failed[0]),&ue_context_pP->ue_context.e_rabs_release_failed[0],sizeof(e_rab_failed_t)*ue_context_pP->ue_context.nb_release_of_e_rabs);

    ue_context_pP->ue_context.setup_e_rabs -= e_rabs_released;
    LOG_I(RRC,"S1AP-E-RAB RELEASE RESPONSE: ENB_UE_S1AP_ID %d release_e_rabs %d setup_e_rabs %d \n",
              S1AP_E_RAB_RELEASE_RESPONSE (msg_p).eNB_ue_s1ap_id,
              e_rabs_released, ue_context_pP->ue_context.setup_e_rabs);
    
    itti_send_msg_to_task (TASK_S1AP, ctxt_pP->instance, msg_p);
    //clear xid
    for(int i = 0; i < NB_RB_MAX; i++) {
        ue_context_pP->ue_context.e_rab[i].xid = -1;
    }
    //clear release e_rabs
    ue_context_pP->ue_context.nb_release_of_e_rabs = 0;
    memset(&ue_context_pP->ue_context.e_rabs_release_failed[0],0,sizeof(e_rab_failed_t)*S1AP_MAX_E_RAB);
    return 0;
}

/*------------------------------------------------------------------------------*/
int rrc_eNB_process_PAGING_IND(MessageDef *msg_p, const char *msg_name, instance_t instance)
{
  const unsigned int Ttab[4] = {32,64,128,256};
  uint8_t Tc,Tue;  /* DRX cycle of UE */
  uint32_t pcch_nB;  /* 4T, 2T, T, T/2, T/4, T/8, T/16, T/32 */
  uint32_t N;  /* N: min(T,nB). total count of PF in one DRX cycle */
  uint32_t Ns = 0;  /* Ns: max(1,nB/T) */
  uint8_t i_s;  /* i_s = floor(UE_ID/N) mod Ns */
  uint32_t T;  /* DRX cycle */
  for (uint16_t tai_size = 0; tai_size < S1AP_PAGING_IND(msg_p).tai_size; tai_size++) {
       LOG_D(RRC,"[eNB %d] In S1AP_PAGING_IND: MCC %d, MNC %d, TAC %d\n", instance, S1AP_PAGING_IND(msg_p).plmn_identity[tai_size].mcc,
             S1AP_PAGING_IND(msg_p).plmn_identity[tai_size].mnc, S1AP_PAGING_IND(msg_p).tac[tai_size]);
      if (RC.rrc[instance]->configuration.mcc == S1AP_PAGING_IND(msg_p).plmn_identity[tai_size].mcc
          && RC.rrc[instance]->configuration.mnc == S1AP_PAGING_IND(msg_p).plmn_identity[tai_size].mnc
          && RC.rrc[instance]->configuration.tac == S1AP_PAGING_IND(msg_p).tac[tai_size]) {
          for (uint8_t CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
              lte_frame_type_t frame_type = RC.eNB[instance][CC_id]->frame_parms.frame_type;
              /* get nB from configuration */
              /* get default DRX cycle from configuration */
              Tc = (uint8_t)RC.rrc[instance]->configuration.pcch_defaultPagingCycle[CC_id];
              if (Tc < PCCH_Config__defaultPagingCycle_rf32 || Tc > PCCH_Config__defaultPagingCycle_rf256) {
                  continue;
              }
              Tue = (uint8_t)S1AP_PAGING_IND(msg_p).paging_drx;
              /* set T = min(Tc,Tue) */
              T = Tc < Tue ? Ttab[Tc] : Ttab[Tue];
              /* set pcch_nB = PCCH-Config->nB */
              pcch_nB = (uint32_t)RC.rrc[instance]->configuration.pcch_nB[CC_id];
              switch (pcch_nB) {
                case PCCH_Config__nB_fourT:
                    Ns = 4;
                    break;
                case PCCH_Config__nB_twoT:
                    Ns = 2;
                    break;
                default:
                    Ns = 1;
                    break;
              }
              /* set N = min(T,nB) */
              if (pcch_nB > PCCH_Config__nB_oneT) {
                switch (pcch_nB) {
                case PCCH_Config__nB_halfT:
                  N = T/2;
                  break;
                case PCCH_Config__nB_quarterT:
                  N = T/4;
                  break;
                case PCCH_Config__nB_oneEighthT:
                  N = T/8;
                  break;
                case PCCH_Config__nB_oneSixteenthT:
                  N = T/16;
                  break;
                case PCCH_Config__nB_oneThirtySecondT:
                  N = T/32;
                  break;
                default:
                  /* pcch_nB error */
                  LOG_E(RRC, "[eNB %d] In S1AP_PAGING_IND:  pcch_nB error (pcch_nB %d) \n",
                      instance, pcch_nB);
                  return (-1);
                }
              } else {
                N = T;
              }

              /* insert data to UE_PF_PO or update data in UE_PF_PO */
              pthread_mutex_lock(&ue_pf_po_mutex);
              uint8_t i = 0;
              for (i = 0; i < NUMBER_OF_UE_MAX; i++) {
                if ((UE_PF_PO[CC_id][i].enable_flag == TRUE && UE_PF_PO[CC_id][i].ue_index_value == (uint16_t)(S1AP_PAGING_IND(msg_p).ue_index_value))
                    || (UE_PF_PO[CC_id][i].enable_flag != TRUE)) {
                    /* set T = min(Tc,Tue) */
                    UE_PF_PO[CC_id][i].T = T;
                    /* set UE_ID */
                    UE_PF_PO[CC_id][i].ue_index_value = (uint16_t)S1AP_PAGING_IND(msg_p).ue_index_value;
                    /* calculate PF and PO */
                    /* set PF_min : SFN mod T = (T div N)*(UE_ID mod N) */
                    UE_PF_PO[CC_id][i].PF_min = (T / N) * (UE_PF_PO[CC_id][i].ue_index_value % N);
                    /* set PO */
                    /* i_s = floor(UE_ID/N) mod Ns */
                    i_s = (uint8_t)((UE_PF_PO[CC_id][i].ue_index_value / N) % Ns);
                    if (Ns == 1) {
                        UE_PF_PO[CC_id][i].PO = (frame_type==FDD) ? 9 : 0;
                    } else if (Ns==2) {
                        UE_PF_PO[CC_id][i].PO = (frame_type==FDD) ? (4+(5*i_s)) : (5*i_s);
                    } else if (Ns==4) {
                        UE_PF_PO[CC_id][i].PO = (frame_type==FDD) ? (4*(i_s&1)+(5*(i_s>>1))) : ((i_s&1)+(5*(i_s>>1)));
                    }
                    if (UE_PF_PO[CC_id][i].enable_flag == TRUE) {
                        //paging exist UE log
                        LOG_D(RRC,"[eNB %d] CC_id %d In S1AP_PAGING_IND: Update exist UE %d, T %d, PF %d, PO %d\n", instance, CC_id, UE_PF_PO[CC_id][i].ue_index_value, T, UE_PF_PO[CC_id][i].PF_min, UE_PF_PO[CC_id][i].PO);
                    } else {
                        /* set enable_flag */
                        UE_PF_PO[CC_id][i].enable_flag = TRUE;
                        //paging new UE log
                        LOG_D(RRC,"[eNB %d] CC_id %d In S1AP_PAGING_IND: Insert a new UE %d, T %d, PF %d, PO %d\n", instance, CC_id, UE_PF_PO[CC_id][i].ue_index_value, T, UE_PF_PO[CC_id][i].PF_min, UE_PF_PO[CC_id][i].PO);
                    }
                    break;
                }
              }
              pthread_mutex_unlock(&ue_pf_po_mutex);

              uint32_t length;
              uint8_t buffer[RRC_BUF_SIZE];
              uint8_t *message_buffer;
              /* Transfer data to PDCP */
              MessageDef *message_p;
              message_p = itti_alloc_new_message (TASK_RRC_ENB, RRC_PCCH_DATA_REQ);
              /* Create message for PDCP (DLInformationTransfer_t) */
              length = do_Paging (instance,
                                  buffer,
                                  S1AP_PAGING_IND(msg_p).ue_paging_identity,
                                  S1AP_PAGING_IND(msg_p).cn_domain);
              message_buffer = itti_malloc (TASK_RRC_ENB, TASK_PDCP_ENB, length);
              /* Uses a new buffer to avoid issue with PDCP buffer content that could be changed by PDCP (asynchronous message handling). */
              memcpy (message_buffer, buffer, length);
              RRC_PCCH_DATA_REQ (message_p).sdu_size  = length;
              RRC_PCCH_DATA_REQ (message_p).sdu_p     = message_buffer;
              RRC_PCCH_DATA_REQ (message_p).mode      = PDCP_TRANSMISSION_MODE_TRANSPARENT;  /* not used */
              RRC_PCCH_DATA_REQ (message_p).rnti      = P_RNTI;
              RRC_PCCH_DATA_REQ (message_p).ue_index  = i;
              RRC_PCCH_DATA_REQ (message_p).CC_id  = CC_id;
              LOG_D(RRC, "[eNB %d] CC_id %d In S1AP_PAGING_IND: send encdoed buffer to PDCP buffer_size %d\n", instance, CC_id, length);
              itti_send_msg_to_task (TASK_PDCP_ENB, instance, message_p);
          }
      }
  }

  return (0);
}

# endif /* defined(ENABLE_ITTI) */
#endif /* defined(ENABLE_USE_MME) */
