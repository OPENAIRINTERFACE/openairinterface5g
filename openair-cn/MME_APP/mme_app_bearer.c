/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file mme_app_bearer.c
* \brief
* \author Sebastien ROUX, Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intertask_interface.h"
#include "mme_config.h"

#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "sgw_lite_ie_defs.h"

#include "secu_defs.h"

#include "assertions.h"
#include "common_types.h"
#include "msc.h"


//------------------------------------------------------------------------------
int
mme_app_send_s11_release_access_bearers_req(
  struct ue_context_s * const ue_context_pP)
//------------------------------------------------------------------------------
{
  uint8_t                     i                 = 0;
  task_id_t                   to_task           = TASK_UNKNOWN;
  /* Keep the identifier to the default APN */
  context_identifier_t        context_identifier;
  MessageDef                 *message_p         = NULL;
  SgwReleaseAccessBearersRequest    *release_access_bearers_request_p = NULL;


  DevAssert(ue_context_pP != NULL);

#if !defined(ENABLE_STANDALONE_EPC)
  to_task = TASK_S11;
#else
  to_task = TASK_SPGW_APP;
#endif

  message_p = itti_alloc_new_message(TASK_MME_APP, SGW_RELEASE_ACCESS_BEARERS_REQUEST);
  release_access_bearers_request_p = &message_p->ittiMsg.sgwReleaseAccessBearersRequest;
  memset(release_access_bearers_request_p, 0, sizeof(SgwReleaseAccessBearersRequest));

  release_access_bearers_request_p->teid             = ue_context_pP->sgw_s11_teid;
  release_access_bearers_request_p->num_rabs         = 1;
  release_access_bearers_request_p->list_of_rabs[0]  = ue_context_pP->default_bearer_id;
  release_access_bearers_request_p->originating_node = NODE_TYPE_MME;

  MSC_LOG_TX_MESSAGE(
  		MSC_MMEAPP_MME,
  		(to_task == TASK_S11) ? MSC_S11_MME:MSC_SP_GWAPP_MME,
  		NULL,0,
  		"0 SGW_RELEASE_ACCESS_BEARERS_REQUEST teid %u ebi %u",
  		release_access_bearers_request_p->teid,
  		release_access_bearers_request_p->list_of_rabs[0]);

  itti_send_msg_to_task(to_task, INSTANCE_DEFAULT, message_p);
}



//------------------------------------------------------------------------------
int
mme_app_send_s11_create_session_req(
  struct ue_context_s * const ue_context_pP)
//------------------------------------------------------------------------------
{
  uint8_t                     i                 = 0;
  task_id_t                   to_task           = TASK_UNKNOWN;
  /* Keep the identifier to the default APN */
  context_identifier_t        context_identifier;
  MessageDef                 *message_p         = NULL;
  SgwCreateSessionRequest    *session_request_p = NULL;
  struct apn_configuration_s *default_apn_p     = NULL;


  DevAssert(ue_context_pP != NULL);

#if !defined(ENABLE_STANDALONE_EPC)
  to_task = TASK_S11;
#else
  to_task = TASK_SPGW_APP;
#endif


  MME_APP_DEBUG("Handling imsi %"IMSI_FORMAT"\n", ue_context_pP->imsi);

  if (ue_context_pP->sub_status != SS_SERVICE_GRANTED) {
    /* HSS rejected the bearer creation or roaming is not allowed for this
     * UE. This result will trigger an ESM Failure message sent to UE.
     */
    DevMessage("Not implemented: ACCESS NOT GRANTED, send ESM Failure to NAS\n");
  }

  message_p = itti_alloc_new_message(TASK_MME_APP, SGW_CREATE_SESSION_REQUEST);

  /* WARNING:
   * Some parameters should be provided by NAS Layer:
   * - ue_time_zone
   * - mei
   * - uli
   * - uci
   * Some parameters should be provided by HSS:
   * - PGW address for CP
   * - paa
   * - ambr
   * and by MME Application layer:
   * - selection_mode
   * Set these parameters with random values for now.
   */

  session_request_p = &message_p->ittiMsg.sgwCreateSessionRequest;
  memset(session_request_p, 0, sizeof(SgwCreateSessionRequest));

  /* As the create session request is the first exchanged message and as
   * no tunnel had been previously setup, the distant teid is set to 0.
   * The remote teid will be provided in the response message.
   */
  session_request_p->teid = 0;

  MME_APP_IMSI_TO_STRING(ue_context_pP->imsi, (char *)session_request_p->imsi.digit);
  // message content was set to 0
  session_request_p->imsi.length = strlen((const char *)session_request_p->imsi.digit);

  /* Copy the MSISDN */
  memcpy(
    session_request_p->msisdn.digit,
    ue_context_pP->msisdn,
    ue_context_pP->msisdn_length);
  session_request_p->msisdn.length = ue_context_pP->msisdn_length;

  session_request_p->rat_type = RAT_EUTRAN;

  /* Copy the subscribed ambr to the sgw create session request message */
  memcpy(
    &session_request_p->ambr,
    &ue_context_pP->subscribed_ambr,
    sizeof(ambr_t));

  if (ue_context_pP->apn_profile.nb_apns == 0) {
    DevMessage("No APN returned by the HSS");
  }

  context_identifier = ue_context_pP->apn_profile.context_identifier;

  for (i = 0; i < ue_context_pP->apn_profile.nb_apns; i++) {
    default_apn_p = &ue_context_pP->apn_profile.apn_configuration[i];

    /* OK we got our default APN */
    if (default_apn_p->context_identifier == context_identifier)
      break;
  }

  if (!default_apn_p) {
    /* Unfortunately we didn't find our default APN... */
    DevMessage("No default APN found");
  }

  memcpy(&session_request_p->bearer_to_create.bearer_level_qos.gbr,
         &default_apn_p->ambr, sizeof(ambr_t));
  memcpy(&session_request_p->bearer_to_create.bearer_level_qos.mbr,
         &default_apn_p->ambr, sizeof(ambr_t));

  session_request_p->bearer_to_create.bearer_level_qos.qci =
    default_apn_p->subscribed_qos.qci;

  session_request_p->bearer_to_create.bearer_level_qos.pvi =
    default_apn_p->subscribed_qos.allocation_retention_priority.pre_emp_vulnerability;
  session_request_p->bearer_to_create.bearer_level_qos.pci =
    default_apn_p->subscribed_qos.allocation_retention_priority.pre_emp_capability;
  session_request_p->bearer_to_create.bearer_level_qos.pl =
    default_apn_p->subscribed_qos.allocation_retention_priority.priority_level;

  /* Asking for default bearer in initial UE message.
   * Use the address of ue_context as unique TEID: Need to find better here
   * and will generate unique id only for 32 bits platforms.
   */
  session_request_p->sender_fteid_for_cp.teid           = (uint32_t)ue_context_pP;
  session_request_p->sender_fteid_for_cp.interface_type = S11_MME_GTP_C;
  session_request_p->bearer_to_create.eps_bearer_id     = 5;

  ue_context_pP->mme_s11_teid = session_request_p->sender_fteid_for_cp.teid;
  ue_context_pP->sgw_s11_teid = 0;

  memcpy(session_request_p->apn, default_apn_p->service_selection,
         default_apn_p->service_selection_length);

  /* Set PDN type for pdn_type and PAA even if this IE is redundant */
  session_request_p->pdn_type = default_apn_p->pdn_type;
  session_request_p->paa.pdn_type = default_apn_p->pdn_type;

  if (default_apn_p->nb_ip_address == 0) {
    /* UE DHCPv4 allocated ip address */
    memset(session_request_p->paa.ipv4_address, 0, 4);
    memset(session_request_p->paa.ipv6_address, 0, 16);
  } else {
    uint8_t j;

    for (j = 0; j < default_apn_p->nb_ip_address; j++) {
      ip_address_t *ip_address;
      ip_address = &default_apn_p->ip_address[j];

      if (ip_address->pdn_type == IPv4) {
        memcpy(session_request_p->paa.ipv4_address, ip_address->address.ipv4_address, 4);
      } else if (ip_address->pdn_type == IPv6) {
        memcpy(session_request_p->paa.ipv6_address, ip_address->address.ipv6_address, 16);
      }

      //             free(ip_address);
    }
  }

  if ((ue_context_pP->pending_pdn_connectivity_req_pco.length >= PCO_MIN_LENGTH) && (ue_context_pP->pending_pdn_connectivity_req_pco.length <= PCO_MAX_LENGTH)) {
	  memcpy(&session_request_p->pco.byte[0],
			  &ue_context_pP->pending_pdn_connectivity_req_pco.byte[0],
			  ue_context_pP->pending_pdn_connectivity_req_pco.length);
	  session_request_p->pco.length = ue_context_pP->pending_pdn_connectivity_req_pco.length;
  }

  config_read_lock(&mme_config);
  session_request_p->peer_ip = mme_config.ipv4.sgw_ip_address_for_S11;
  config_unlock(&mme_config);

  session_request_p->serving_network.mcc[0] = ue_context_pP->e_utran_cgi.plmn.MCCdigit1;
  session_request_p->serving_network.mcc[1] = ue_context_pP->e_utran_cgi.plmn.MCCdigit2;
  session_request_p->serving_network.mcc[2] = ue_context_pP->e_utran_cgi.plmn.MCCdigit3;

  session_request_p->serving_network.mnc[0] = ue_context_pP->e_utran_cgi.plmn.MNCdigit1;
  session_request_p->serving_network.mnc[1] = ue_context_pP->e_utran_cgi.plmn.MNCdigit2;
  session_request_p->serving_network.mnc[2] = ue_context_pP->e_utran_cgi.plmn.MNCdigit3;

  session_request_p->selection_mode = MS_O_N_P_APN_S_V;

  MSC_LOG_TX_MESSAGE(
  		MSC_MMEAPP_MME,
  		(to_task == TASK_S11) ? MSC_S11_MME:MSC_SP_GWAPP_MME,
  		NULL,0,
  		"0 SGW_CREATE_SESSION_REQUEST imsi %"IMSI_FORMAT,ue_context_pP->imsi);

  return itti_send_msg_to_task(to_task, INSTANCE_DEFAULT, message_p);
}



//------------------------------------------------------------------------------
int
mme_app_handle_nas_pdn_connectivity_req(
  nas_pdn_connectivity_req_t * const nas_pdn_connectivity_req_pP)
//------------------------------------------------------------------------------
{
  struct ue_context_s *ue_context_p = NULL;
  uint64_t             imsi         = 0;

  MME_APP_DEBUG("Received NAS_PDN_CONNECTIVITY_REQ from NAS\n");
  DevAssert(nas_pdn_connectivity_req_pP != NULL);

  MME_APP_STRING_TO_IMSI((char *)nas_pdn_connectivity_req_pP->imsi, &imsi);

  MME_APP_DEBUG("Handling imsi %"IMSI_FORMAT"\n", imsi);


  if ((ue_context_p = mme_ue_context_exists_imsi(&mme_app_desc.mme_ue_contexts,
                      imsi)) == NULL) {
	    MSC_LOG_EVENT(
	    		MSC_MMEAPP_MME,
	    		"NAS_PDN_CONNECTIVITY_REQ Unknown imsi %"IMSI_FORMAT,imsi);
	MME_APP_ERROR("That's embarrassing as we don't know this IMSI\n");
    return -1;
  }

  /* Consider the UE authenticated */
  ue_context_p->imsi_auth = IMSI_AUTHENTICATED;

  // Temp: save request, in near future merge wisely params in context
  memset(ue_context_p->pending_pdn_connectivity_req_imsi,0, 16);
  AssertFatal((nas_pdn_connectivity_req_pP->imsi_length > 0) &&
              (nas_pdn_connectivity_req_pP->imsi_length < 16),
              "BAD IMSI LENGTH %d", nas_pdn_connectivity_req_pP->imsi_length);

  AssertFatal((nas_pdn_connectivity_req_pP->imsi_length > 0) &&
              (nas_pdn_connectivity_req_pP->imsi_length < 16),
              "STOP ON IMSI LENGTH %d", nas_pdn_connectivity_req_pP->imsi_length);
  memcpy(ue_context_p->pending_pdn_connectivity_req_imsi,
         nas_pdn_connectivity_req_pP->imsi,
         nas_pdn_connectivity_req_pP->imsi_length);
  ue_context_p->pending_pdn_connectivity_req_imsi_length = nas_pdn_connectivity_req_pP->imsi_length;
  DUP_OCTET_STRING(nas_pdn_connectivity_req_pP->apn, ue_context_p->pending_pdn_connectivity_req_apn);
  FREE_OCTET_STRING(nas_pdn_connectivity_req_pP->apn)
  // dup OctetString
  DUP_OCTET_STRING(nas_pdn_connectivity_req_pP->pdn_addr, ue_context_p->pending_pdn_connectivity_req_pdn_addr);
  FREE_OCTET_STRING(nas_pdn_connectivity_req_pP->pdn_addr)
  ue_context_p->pending_pdn_connectivity_req_pti          = nas_pdn_connectivity_req_pP->pti;
  ue_context_p->pending_pdn_connectivity_req_ue_id        = nas_pdn_connectivity_req_pP->ue_id;

  memcpy(&ue_context_p->pending_pdn_connectivity_req_pco.byte[0],
         &nas_pdn_connectivity_req_pP->pco.byte[0],
         nas_pdn_connectivity_req_pP->pco.length);
  ue_context_p->pending_pdn_connectivity_req_pco.length = nas_pdn_connectivity_req_pP->pco.length;

  memcpy(&ue_context_p->pending_pdn_connectivity_req_qos,
         &nas_pdn_connectivity_req_pP->qos,
         sizeof(network_qos_t));
  ue_context_p->pending_pdn_connectivity_req_proc_data    = nas_pdn_connectivity_req_pP->proc_data;
  nas_pdn_connectivity_req_pP->proc_data = NULL;
  ue_context_p->pending_pdn_connectivity_req_request_type = nas_pdn_connectivity_req_pP->request_type;

  //if ((nas_pdn_connectivity_req_pP->apn.value == NULL) || (nas_pdn_connectivity_req_pP->apn.length == 0)) {
  /* TODO: Get keys... */
  /* Now generate S6A ULR */
  return mme_app_send_s6a_update_location_req(ue_context_p);
  //} else {
  //return mme_app_send_s11_create_session_req(ue_context_p);

  //}
  //return -1;
}



// sent by NAS
//------------------------------------------------------------------------------
void
mme_app_handle_conn_est_cnf(
  const nas_conn_est_cnf_t * const nas_conn_est_cnf_pP)
//------------------------------------------------------------------------------
{
  struct ue_context_s                    *ue_context_p        = NULL;
  MessageDef                             *message_p           = NULL;
  mme_app_connection_establishment_cnf_t *establishment_cnf_p = NULL;
  bearer_context_t                       *current_bearer_p    = NULL;
  ebi_t                                   bearer_id           = 0;
  uint8_t                                 keNB[32];

  MME_APP_DEBUG("Received NAS_CONNECTION_ESTABLISHMENT_CNF from NAS\n");

  ue_context_p = mme_ue_context_exists_nas_ue_id(&mme_app_desc.mme_ue_contexts, nas_conn_est_cnf_pP->UEid);


  if (ue_context_p == NULL) {
	MSC_LOG_EVENT(
	    		MSC_MMEAPP_MME,
	    		"NAS_CONNECTION_ESTABLISHMENT_CNF Unknown ue %u",nas_conn_est_cnf_pP->UEid);
	MME_APP_ERROR("UE context doesn't exist for UE %06"PRIX32"/dec%u\n",
                  nas_conn_est_cnf_pP->UEid,
                  nas_conn_est_cnf_pP->UEid);
    return;
  }

  message_p           = itti_alloc_new_message(TASK_MME_APP, MME_APP_CONNECTION_ESTABLISHMENT_CNF);
  establishment_cnf_p = &message_p->ittiMsg.mme_app_connection_establishment_cnf;
  memset(establishment_cnf_p, 0, sizeof(mme_app_connection_establishment_cnf_t));

  memcpy(&establishment_cnf_p->nas_conn_est_cnf,
         nas_conn_est_cnf_pP,
         sizeof(nas_conn_est_cnf_t));

  bearer_id = ue_context_p->default_bearer_id;
  current_bearer_p = &ue_context_p->eps_bearers[bearer_id];
  establishment_cnf_p->eps_bearer_id = bearer_id;

  establishment_cnf_p->bearer_s1u_sgw_fteid.interface_type = S1_U_SGW_GTP_U;
  establishment_cnf_p->bearer_s1u_sgw_fteid.teid = current_bearer_p->s_gw_teid;

  if ((current_bearer_p->s_gw_address.pdn_type == IPv4) ||
      (current_bearer_p->s_gw_address.pdn_type == IPv4_AND_v6)) {
    establishment_cnf_p->bearer_s1u_sgw_fteid.ipv4 = 1;
    memcpy(&establishment_cnf_p->bearer_s1u_sgw_fteid.ipv4_address,
           current_bearer_p->s_gw_address.address.ipv4_address,
           4);
  }

  if ((current_bearer_p->s_gw_address.pdn_type == IPv6) ||
      (current_bearer_p->s_gw_address.pdn_type == IPv4_AND_v6)) {
    establishment_cnf_p->bearer_s1u_sgw_fteid.ipv6 = 1;
    memcpy(establishment_cnf_p->bearer_s1u_sgw_fteid.ipv6_address,
           current_bearer_p->s_gw_address.address.ipv6_address,
           16);
  }

  establishment_cnf_p->bearer_qos_qci                    = current_bearer_p->qci;
  establishment_cnf_p->bearer_qos_prio_level             = current_bearer_p->prio_level;
  establishment_cnf_p->bearer_qos_pre_emp_vulnerability  = current_bearer_p->pre_emp_vulnerability;
  establishment_cnf_p->bearer_qos_pre_emp_capability     = current_bearer_p->pre_emp_capability;
#warning "Check ue_context_p ambr"
  establishment_cnf_p->ambr.br_ul                        = ue_context_p->subscribed_ambr.br_ul;
  establishment_cnf_p->ambr.br_dl                        = ue_context_p->subscribed_ambr.br_dl;

  establishment_cnf_p->security_capabilities_encryption_algorithms = nas_conn_est_cnf_pP->selected_encryption_algorithm;
  establishment_cnf_p->security_capabilities_integrity_algorithms  = nas_conn_est_cnf_pP->selected_integrity_algorithm;
  MME_APP_DEBUG("security_capabilities_encryption_algorithms 0x%04X\n", establishment_cnf_p->security_capabilities_encryption_algorithms);
  MME_APP_DEBUG("security_capabilities_integrity_algorithms  0x%04X\n", establishment_cnf_p->security_capabilities_integrity_algorithms);

  MME_APP_DEBUG("Derive keNB with UL NAS COUNT %x\n", nas_conn_est_cnf_pP->ul_nas_count);
  derive_keNB(ue_context_p->vector_in_use->kasme, nas_conn_est_cnf_pP->ul_nas_count, keNB); //156
  memcpy(establishment_cnf_p->keNB, keNB, 32);

  MSC_LOG_TX_MESSAGE(
  		MSC_MMEAPP_MME,
  		MSC_S1AP_MME,
  		NULL,0,
  		"0 MME_APP_CONNECTION_ESTABLISHMENT_CNF ebi %u s1u_sgw teid %u qci %u prio level %u sea 0x%x sia 0x%x",
  		establishment_cnf_p->eps_bearer_id,
  		establishment_cnf_p->bearer_s1u_sgw_fteid.teid,
  		establishment_cnf_p->bearer_qos_qci,
  		establishment_cnf_p->bearer_qos_prio_level,
  		establishment_cnf_p->security_capabilities_encryption_algorithms,
  		establishment_cnf_p->security_capabilities_integrity_algorithms);

  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
}



// sent by S1AP
//------------------------------------------------------------------------------
void
mme_app_handle_conn_est_ind(
  const mme_app_connection_establishment_ind_t * const conn_est_ind_pP)
//------------------------------------------------------------------------------
{
  struct ue_context_s *ue_context_p  = NULL;
  MessageDef          *message_p     = NULL;

  MME_APP_DEBUG("Received MME_APP_CONNECTION_ESTABLISHMENT_IND from S1AP\n");

  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id(
                   &mme_app_desc.mme_ue_contexts,
                   conn_est_ind_pP->mme_ue_s1ap_id);



  if (ue_context_p == NULL) {
    MME_APP_DEBUG("We didn't find this mme_ue_s1ap_id in list of UE: %06"PRIX32"/dec%u\n",
                  conn_est_ind_pP->mme_ue_s1ap_id,
                  conn_est_ind_pP->mme_ue_s1ap_id);
    MME_APP_DEBUG("UE context doesn't exist -> create one\n");

    if ((ue_context_p = mme_create_new_ue_context()) == NULL) {
      /* Error during ue context malloc */
      /* TODO */
      DevMessage("mme_create_new_ue_context");
      return;
    }

    // S1AP UE ID AND NAS UE ID ARE THE SAME
    ue_context_p->mme_ue_s1ap_id = conn_est_ind_pP->mme_ue_s1ap_id;
    ue_context_p->ue_id          = conn_est_ind_pP->mme_ue_s1ap_id;
    DevAssert(mme_insert_ue_context(&mme_app_desc.mme_ue_contexts, ue_context_p) == 0);

    // tests
    ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id(
                     &mme_app_desc.mme_ue_contexts,
                     conn_est_ind_pP->mme_ue_s1ap_id);
    AssertFatal(ue_context_p != NULL, "mme_ue_context_exists_mme_ue_s1ap_id Failed");
    ue_context_p = mme_ue_context_exists_nas_ue_id(
                     &mme_app_desc.mme_ue_contexts,
                     conn_est_ind_pP->mme_ue_s1ap_id);
    AssertFatal(ue_context_p != NULL, "mme_ue_context_exists_nas_ue_id Failed");
  }

  message_p  = itti_alloc_new_message(TASK_MME_APP, NAS_CONNECTION_ESTABLISHMENT_IND);
  memcpy(&NAS_CONN_EST_IND(message_p).nas,
         &conn_est_ind_pP->nas,
         sizeof (nas_establish_ind_t));


  MSC_LOG_TX_MESSAGE(
  		MSC_MMEAPP_MME,
  		MSC_NAS_MME,
  		NULL,0,
  		"0 NAS_CONNECTION_ESTABLISHMENT_IND");

  itti_send_msg_to_task(TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
}



//------------------------------------------------------------------------------
int
mme_app_handle_create_sess_resp(
  const SgwCreateSessionResponse * const create_sess_resp_pP)
//------------------------------------------------------------------------------
{
  struct ue_context_s *ue_context_p     = NULL;
  bearer_context_t    *current_bearer_p = NULL;
  MessageDef          *message_p = NULL;

  int16_t bearer_id;

  DevAssert(create_sess_resp_pP != NULL);

  MME_APP_DEBUG("Received SGW_CREATE_SESSION_RESPONSE from S+P-GW\n");

  ue_context_p = mme_ue_context_exists_s11_teid(&mme_app_desc.mme_ue_contexts,
                 create_sess_resp_pP->teid);


  if (ue_context_p == NULL) {
    MME_APP_DEBUG("We didn't find this teid in list of UE: %08x\n",
                  create_sess_resp_pP->teid);
    return -1;
  }

  /* Store the S-GW teid */
  ue_context_p->sgw_s11_teid = create_sess_resp_pP->s11_sgw_teid.teid;

  //---------------------------------------------------------
  // Process SgwCreateSessionResponse.bearer_context_created
  //---------------------------------------------------------
  bearer_id = create_sess_resp_pP->bearer_context_created.eps_bearer_id/* - 5*/;
  /* Depending on s11 result we have to send reject or accept for bearers */
  DevCheck((bearer_id < BEARERS_PER_UE) && (bearer_id >= 0),
           bearer_id,
           BEARERS_PER_UE,
           0);
  ue_context_p->default_bearer_id = bearer_id;

  if (create_sess_resp_pP->bearer_context_created.cause != REQUEST_ACCEPTED) {
    DevMessage("Cases where bearer cause != REQUEST_ACCEPTED are not handled\n");
  }

  DevAssert(create_sess_resp_pP->bearer_context_created.s1u_sgw_fteid.interface_type == S1_U_SGW_GTP_U);

  /* Updating statistics */
  mme_app_desc.mme_ue_contexts.nb_bearers_managed++;
  mme_app_desc.mme_ue_contexts.nb_bearers_since_last_stat++;

  current_bearer_p = &ue_context_p->eps_bearers[bearer_id];
  current_bearer_p->s_gw_teid = create_sess_resp_pP->bearer_context_created.s1u_sgw_fteid.teid;

  switch (create_sess_resp_pP->bearer_context_created.s1u_sgw_fteid.ipv4 +
          (create_sess_resp_pP->bearer_context_created.s1u_sgw_fteid.ipv6 << 1)) {
  default:
  case 0: {
    /* No address provided: impossible case */
    DevMessage("No ip address for user-plane provided...\n");
  }
  break;

  case 1: {
    /* Only IPv4 address */
    current_bearer_p->s_gw_address.pdn_type = IPv4;
    memcpy(current_bearer_p->s_gw_address.address.ipv4_address,
           &create_sess_resp_pP->bearer_context_created.s1u_sgw_fteid.ipv4_address, 4);
  }
  break;

  case 2: {
    /* Only IPv6 address */
    current_bearer_p->s_gw_address.pdn_type = IPv6;
    memcpy(current_bearer_p->s_gw_address.address.ipv6_address,
           create_sess_resp_pP->bearer_context_created.s1u_sgw_fteid.ipv6_address, 16);
  }
  break;

  case 3: {
    /* Both IPv4 and Ipv6 */
    current_bearer_p->s_gw_address.pdn_type = IPv4_AND_v6;
    memcpy(current_bearer_p->s_gw_address.address.ipv4_address,
           &create_sess_resp_pP->bearer_context_created.s1u_sgw_fteid.ipv4_address, 4);
    memcpy(current_bearer_p->s_gw_address.address.ipv6_address,
           create_sess_resp_pP->bearer_context_created.s1u_sgw_fteid.ipv6_address, 16);
  }
  break;
  }

  current_bearer_p->p_gw_teid = create_sess_resp_pP->bearer_context_created.s5_s8_u_pgw_fteid.teid;
  memset(&current_bearer_p->p_gw_address,0, sizeof(ip_address_t));

  if (create_sess_resp_pP->bearer_context_created.bearer_level_qos != NULL) {
    current_bearer_p->qci                    = create_sess_resp_pP->bearer_context_created.bearer_level_qos->qci;
    current_bearer_p->prio_level             = create_sess_resp_pP->bearer_context_created.bearer_level_qos->pl;
    current_bearer_p->pre_emp_vulnerability  = create_sess_resp_pP->bearer_context_created.bearer_level_qos->pvi;
    current_bearer_p->pre_emp_capability     = create_sess_resp_pP->bearer_context_created.bearer_level_qos->pci;
    MME_APP_DEBUG("%s set qci %u in bearer %u\n",
                  __FUNCTION__,
                  current_bearer_p->qci,
                  ue_context_p->default_bearer_id);
  } else {
    // if null, it is not modified
    //current_bearer_p->qci                    = ue_context_p->pending_pdn_connectivity_req_qos.qci;
#warning "may force QCI here to 9"
    current_bearer_p->qci                    = 9;
    current_bearer_p->prio_level             = 1;
    current_bearer_p->pre_emp_vulnerability  = 0;
    current_bearer_p->pre_emp_capability     = 0;
    MME_APP_DEBUG("%s set qci %u in bearer %u (qos not modified by S/P-GW)\n",
                  __FUNCTION__,
                  current_bearer_p->qci,
                  ue_context_p->default_bearer_id);
  }

  mme_app_dump_ue_contexts(&mme_app_desc.mme_ue_contexts);

  {
    //uint8_t *keNB = NULL;

    message_p = itti_alloc_new_message(TASK_MME_APP, NAS_PDN_CONNECTIVITY_RSP);
    memset((void*)&message_p->ittiMsg.nas_pdn_connectivity_rsp,
           0,
           sizeof(nas_pdn_connectivity_rsp_t));

    // moved to NAS_CONNECTION_ESTABLISHMENT_CONF, keNB not handled in NAS MME
    //derive_keNB(ue_context_p->vector_in_use->kasme, 156, &keNB);
    //memcpy(NAS_PDN_CONNECTIVITY_RSP(message_p).keNB, keNB, 32);

    //free(keNB);

    NAS_PDN_CONNECTIVITY_RSP(message_p).pti            = ue_context_p->pending_pdn_connectivity_req_pti;   // NAS internal ref
    NAS_PDN_CONNECTIVITY_RSP(message_p).ue_id          = ue_context_p->pending_pdn_connectivity_req_ue_id; // NAS internal ref

    // TO REWORK:
    if ((ue_context_p->pending_pdn_connectivity_req_apn.value != NULL)
        && (ue_context_p->pending_pdn_connectivity_req_apn.length != 0)) {
      DUP_OCTET_STRING(
        ue_context_p->pending_pdn_connectivity_req_apn,
        NAS_PDN_CONNECTIVITY_RSP(message_p).apn);
      MME_APP_DEBUG("SET APN FROM NAS PDN CONNECTIVITY CREATE: %s\n", NAS_PDN_CONNECTIVITY_RSP(message_p).apn.value);
    } else {
      int i;
      context_identifier_t context_identifier = ue_context_p->apn_profile.context_identifier;

      for (i = 0; i < ue_context_p->apn_profile.nb_apns; i++) {
        if (ue_context_p->apn_profile.apn_configuration[i].context_identifier == context_identifier) {
          AssertFatal(ue_context_p->apn_profile.apn_configuration[i].service_selection_length > 0, "Bad APN string (len = 0)");

          if (ue_context_p->apn_profile.apn_configuration[i].service_selection_length > 0) {
            NAS_PDN_CONNECTIVITY_RSP(message_p).apn.value  = malloc(ue_context_p->apn_profile.apn_configuration[i].service_selection_length + 1);
            NAS_PDN_CONNECTIVITY_RSP(message_p).apn.length = ue_context_p->apn_profile.apn_configuration[i].service_selection_length;
            AssertFatal(ue_context_p->apn_profile.apn_configuration[i].service_selection_length <= APN_MAX_LENGTH,
                        "Bad APN string length %d",
                        ue_context_p->apn_profile.apn_configuration[i].service_selection_length);
            memcpy(NAS_PDN_CONNECTIVITY_RSP(message_p).apn.value,
                   ue_context_p->apn_profile.apn_configuration[i].service_selection,
                   ue_context_p->apn_profile.apn_configuration[i].service_selection_length);
            NAS_PDN_CONNECTIVITY_RSP(message_p).apn.value[ue_context_p->apn_profile.apn_configuration[i].service_selection_length] = '\0';
            MME_APP_DEBUG("SET APN FROM HSS ULA: %s\n", NAS_PDN_CONNECTIVITY_RSP(message_p).apn.value);
            break;
          }
        }
      }
    }

    MME_APP_DEBUG("APN: %s\n", NAS_PDN_CONNECTIVITY_RSP(message_p).apn.value);

    switch (create_sess_resp_pP->paa.pdn_type) {
    case IPv4:
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length = 4;
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value  = malloc(NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length+1);
      DevAssert(NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value != NULL);
      memcpy(
        NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value,
        create_sess_resp_pP->paa.ipv4_address,
        NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length);
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value[NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length] = '0';
      break;

    case IPv6:
      DevAssert(create_sess_resp_pP->paa.ipv6_prefix_length == 64); // NAS seems to only support 64 bits
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length = create_sess_resp_pP->paa.ipv6_prefix_length/8;
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value  = malloc(NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length+1);
      DevAssert(NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value != NULL);
      memcpy(
        NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value,
        create_sess_resp_pP->paa.ipv6_address,
        NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length);
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value[NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length] = '0';
      break;

    case IPv4_AND_v6:
      DevAssert(create_sess_resp_pP->paa.ipv6_prefix_length == 64); // NAS seems to only support 64 bits
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length = 4 + create_sess_resp_pP->paa.ipv6_prefix_length/8;
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value  = malloc(NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length+1);
      DevAssert(NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value != NULL);
      memcpy(
        NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value,
        create_sess_resp_pP->paa.ipv4_address,
        4);
      memcpy(
        &NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value[4],
        create_sess_resp_pP->paa.ipv6_address,
        create_sess_resp_pP->paa.ipv6_prefix_length/8);
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value[NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length] = '0';
      break;

    case IPv4_OR_v6:
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length = 4;
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value  = malloc(NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length+1);
      DevAssert(NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value != NULL);
      memcpy(
        NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value,
        create_sess_resp_pP->paa.ipv4_address,
        NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length);
      NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.value[NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_addr.length] = '0';
      break;

    default:
      DevAssert(0);
    }

    NAS_PDN_CONNECTIVITY_RSP(message_p).pdn_type       = create_sess_resp_pP->paa.pdn_type;
    NAS_PDN_CONNECTIVITY_RSP(message_p).proc_data      = ue_context_p->pending_pdn_connectivity_req_proc_data;    // NAS internal ref
    ue_context_p->pending_pdn_connectivity_req_proc_data = NULL;

#warning "QOS hardcoded here"
    //memcpy(&NAS_PDN_CONNECTIVITY_RSP(message_p).qos,
    //        &ue_context_p->pending_pdn_connectivity_req_qos,
    //        sizeof(network_qos_t));
    NAS_PDN_CONNECTIVITY_RSP(message_p).qos.gbrUL    = 64;  /* 64=64kb/s   Guaranteed Bit Rate for uplink   */
    NAS_PDN_CONNECTIVITY_RSP(message_p).qos.gbrDL    = 120; /* 120=512kb/s Guaranteed Bit Rate for downlink */
    NAS_PDN_CONNECTIVITY_RSP(message_p).qos.mbrUL    = 72; /* 72=128kb/s   Maximum Bit Rate for uplink      */
    NAS_PDN_CONNECTIVITY_RSP(message_p).qos.mbrDL    = 135; /*135=1024kb/s Maximum Bit Rate for downlink    */
    NAS_PDN_CONNECTIVITY_RSP(message_p).qos.qci      = 9; /* QoS Class Identifier                           */

    NAS_PDN_CONNECTIVITY_RSP(message_p).request_type   = ue_context_p->pending_pdn_connectivity_req_request_type; // NAS internal ref
    ue_context_p->pending_pdn_connectivity_req_request_type = 0;

    // here at this point OctetString are saved in resp, no loss of memory (apn, pdn_addr)

    NAS_PDN_CONNECTIVITY_RSP(message_p).eNB_ue_s1ap_id = ue_context_p->eNB_ue_s1ap_id;
    NAS_PDN_CONNECTIVITY_RSP(message_p).mme_ue_s1ap_id = ue_context_p->mme_ue_s1ap_id;

    NAS_PDN_CONNECTIVITY_RSP(message_p).ebi = bearer_id;

    NAS_PDN_CONNECTIVITY_RSP(message_p).qci = current_bearer_p->qci;
    NAS_PDN_CONNECTIVITY_RSP(message_p).prio_level = current_bearer_p->prio_level;
    NAS_PDN_CONNECTIVITY_RSP(message_p).pre_emp_vulnerability = current_bearer_p->pre_emp_vulnerability;
    NAS_PDN_CONNECTIVITY_RSP(message_p).pre_emp_capability = current_bearer_p->pre_emp_capability;

    NAS_PDN_CONNECTIVITY_RSP(message_p).sgw_s1u_teid = current_bearer_p->s_gw_teid;
    memcpy(&NAS_PDN_CONNECTIVITY_RSP(message_p).sgw_s1u_address,
           &current_bearer_p->s_gw_address, sizeof(ip_address_t));

    NAS_PDN_CONNECTIVITY_RSP(message_p).ambr.br_ul = ue_context_p->subscribed_ambr.br_ul;
    NAS_PDN_CONNECTIVITY_RSP(message_p).ambr.br_dl = ue_context_p->subscribed_ambr.br_dl;


    memcpy(&NAS_PDN_CONNECTIVITY_RSP(message_p).pco.byte[0],
           &create_sess_resp_pP->pco.byte[0],
           create_sess_resp_pP->pco.length);
    NAS_PDN_CONNECTIVITY_RSP(message_p).pco.length = create_sess_resp_pP->pco.length;

    MSC_LOG_TX_MESSAGE(
    		MSC_MMEAPP_MME,
    		MSC_NAS_MME,
    		NULL,0,
    		"0 NAS_PDN_CONNECTIVITY_RSP sgw_s1u_teid %u ebi %u qci %u prio %u",
    		current_bearer_p->s_gw_teid,
    		bearer_id,
    		current_bearer_p->qci,
    		current_bearer_p->prio_level);

    return itti_send_msg_to_task(TASK_NAS_MME, INSTANCE_DEFAULT, message_p);
  }
  return 0;
}



//------------------------------------------------------------------------------
void
mme_app_handle_initial_context_setup_rsp(
  const mme_app_initial_context_setup_rsp_t * const initial_ctxt_setup_rsp_pP)
//------------------------------------------------------------------------------
{
  struct ue_context_s *ue_context_p  = NULL;
  MessageDef          *message_p     = NULL;
  task_id_t            to_task       = TASK_UNKNOWN;

  MME_APP_DEBUG("Received MME_APP_INITIAL_CONTEXT_SETUP_RSP from S1AP\n");

  ue_context_p = mme_ue_context_exists_mme_ue_s1ap_id(
                   &mme_app_desc.mme_ue_contexts,
                   initial_ctxt_setup_rsp_pP->mme_ue_s1ap_id);


  if (ue_context_p == NULL) {
    MME_APP_DEBUG("We didn't find this mme_ue_s1ap_id in list of UE: %08x %d(dec)\n",
                  initial_ctxt_setup_rsp_pP->mme_ue_s1ap_id,
                  initial_ctxt_setup_rsp_pP->mme_ue_s1ap_id);
    MSC_LOG_EVENT(
    		MSC_MMEAPP_MME,
    		"MME_APP_INITIAL_CONTEXT_SETUP_RSP Unknown ue %u",initial_ctxt_setup_rsp_pP->mme_ue_s1ap_id);
    return;
  }

#if !defined(ENABLE_STANDALONE_EPC)
  to_task = TASK_S11;
#else
  to_task = TASK_SPGW_APP;
#endif

  message_p = itti_alloc_new_message(TASK_MME_APP, SGW_MODIFY_BEARER_REQUEST);

  AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");
  memset((void*)&message_p->ittiMsg.sgwModifyBearerRequest,
         0,
         sizeof(SgwModifyBearerRequest));

  SGW_MODIFY_BEARER_REQUEST(message_p).teid                      = ue_context_p->sgw_s11_teid;

  /* Delay Value in integer multiples of 50 millisecs, or zero */
  SGW_MODIFY_BEARER_REQUEST(message_p).delay_dl_packet_notif_req = 0; // TO DO

  SGW_MODIFY_BEARER_REQUEST(message_p).bearer_context_to_modify.eps_bearer_id  = initial_ctxt_setup_rsp_pP->eps_bearer_id;
  memcpy(&SGW_MODIFY_BEARER_REQUEST(message_p).bearer_context_to_modify.s1_eNB_fteid,
         &initial_ctxt_setup_rsp_pP->bearer_s1u_enb_fteid,
         sizeof(SGW_MODIFY_BEARER_REQUEST(message_p).bearer_context_to_modify.s1_eNB_fteid));

  SGW_MODIFY_BEARER_REQUEST(message_p).mme_fq_csid.node_id_type  = GLOBAL_UNICAST_IPv4; // TO DO
  SGW_MODIFY_BEARER_REQUEST(message_p).mme_fq_csid.csid          = 0; // TO DO ...
  SGW_MODIFY_BEARER_REQUEST(message_p).indication_flags          = 0; // TO DO
  SGW_MODIFY_BEARER_REQUEST(message_p).rat_type                  = RAT_EUTRAN;

  /* S11 stack specific parameter. Not used in standalone epc mode */
  SGW_MODIFY_BEARER_REQUEST(message_p).trxn                      = NULL;


  MSC_LOG_TX_MESSAGE(
  		MSC_MMEAPP_MME,
  		(to_task == TASK_S11) ? MSC_S11_MME:MSC_SP_GWAPP_MME,
  		NULL,0,
  		"0 SGW_MODIFY_BEARER_REQUEST teid %u ebi %u",
  		SGW_MODIFY_BEARER_REQUEST(message_p).teid,
  		SGW_MODIFY_BEARER_REQUEST(message_p).bearer_context_to_modify.eps_bearer_id);


  itti_send_msg_to_task(to_task, INSTANCE_DEFAULT, message_p);
}



//------------------------------------------------------------------------------
void mme_app_handle_release_access_bearers_resp(
		const SgwReleaseAccessBearersResponse * const rel_access_bearers_rsp_pP)
//------------------------------------------------------------------------------
{
	  MessageDef          *message_p     = NULL;
	  struct ue_context_s *ue_context_p  = NULL;

	  ue_context_p = mme_ue_context_exists_s11_teid(&mme_app_desc.mme_ue_contexts,
			  rel_access_bearers_rsp_pP->teid);


	  if (ue_context_p == NULL) {
	    MME_APP_DEBUG("We didn't find this teid in list of UE: %06"PRIX32"\n",
	    		rel_access_bearers_rsp_pP->teid);
	    return;
	  }

	  message_p = itti_alloc_new_message(TASK_MME_APP, S1AP_UE_CONTEXT_RELEASE_COMMAND);

	  AssertFatal(message_p != NULL, "itti_alloc_new_message Failed");
	  memset((void*)&message_p->ittiMsg.s1ap_ue_context_release_command,
	         0,
	         sizeof(s1ap_ue_context_release_command_t));

	  S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).mme_ue_s1ap_id       = ue_context_p->mme_ue_s1ap_id;
	  MSC_LOG_TX_MESSAGE(
	  		MSC_MMEAPP_MME,
	  		MSC_S1AP_MME,
	  		NULL,0,
	  		"0 S1AP_UE_CONTEXT_RELEASE_COMMAND mme_ue_s1ap_id %06"PRIX32" ",
	  		S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).mme_ue_s1ap_id);


	  itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);
}

