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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "msc.h"
#include "intertask_interface.h"
#include "mme_app_itti_messaging.h"

#include "mme_config.h"

#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "mcc_mnc_itu.h"

#include "assertions.h"

static
int mme_app_request_authentication_info(const char   *imsi,
                                        const uint8_t nb_of_vectors,
                                        const plmn_t *plmn,
                                        const uint8_t *auts);

static
int mme_app_request_authentication_info(const char   *imsi,
                                        const uint8_t nb_of_vectors,
                                        const plmn_t *plmn,
                                        const uint8_t *auts)
{
  s6a_auth_info_req_t *auth_info_req = NULL;
  MessageDef          *message_p     = NULL;
  int                  imsi_length   = strlen(imsi);

  DevAssert(plmn != NULL);

  message_p = itti_alloc_new_message(TASK_MME_APP, S6A_AUTH_INFO_REQ);

  auth_info_req = &message_p->ittiMsg.s6a_auth_info_req;
  memset(auth_info_req, 0, sizeof(*auth_info_req));

  strncpy(auth_info_req->imsi, imsi, imsi_length);
  auth_info_req->imsi_length = imsi_length;
  //MME_APP_IMSI_TO_STRING(imsi, auth_info_req->imsi);

  memcpy(&auth_info_req->visited_plmn, plmn, sizeof(plmn_t));
  MME_APP_DEBUG("%s visited_plmn MCC %X%X%X MNC %X%X%X\n",
                __FUNCTION__,
                auth_info_req->visited_plmn.MCCdigit1,
                auth_info_req->visited_plmn.MCCdigit2,
                auth_info_req->visited_plmn.MCCdigit3,
                auth_info_req->visited_plmn.MNCdigit1,
                auth_info_req->visited_plmn.MNCdigit2,
                auth_info_req->visited_plmn.MNCdigit3);
  uint8_t *ptr = (uint8_t *)&auth_info_req->visited_plmn;
  MME_APP_DEBUG("%s visited_plmn %02X%02X%02X\n",
                __FUNCTION__,
                ptr[0],
                ptr[1],
                ptr[2]);

  auth_info_req->nb_of_vectors = nb_of_vectors;

  if (auts != NULL) {
    auth_info_req->re_synchronization = 1;
    memcpy(auth_info_req->auts, auts, sizeof(auth_info_req->auts));
  } else {
    auth_info_req->re_synchronization = 0;
    memset(auth_info_req->auts, 0, sizeof(auth_info_req->auts));
  }

  MSC_LOG_TX_MESSAGE(
  		MSC_MMEAPP_MME,
  		MSC_S6A_MME,
  		NULL,0,
  		"0 S6A_AUTH_INFO_REQ IMSI %s visited_plmn %02X%02X%02X re_sync %u",
  		imsi,ptr[0],ptr[1],ptr[2],auth_info_req->re_synchronization);

  return itti_send_msg_to_task(TASK_S6A, INSTANCE_DEFAULT, message_p);
}

int mme_app_handle_nas_auth_resp(const nas_auth_resp_t * const nas_auth_resp_pP)
{
  struct ue_context_s *ue_context = NULL;
  uint64_t             imsi       = 0;

  DevAssert(nas_auth_resp_pP != NULL);

  MME_APP_STRING_TO_IMSI((char *)nas_auth_resp_pP->imsi, &imsi);

  MME_APP_DEBUG("Handling imsi %"IMSI_FORMAT"\n", imsi);


  if ((ue_context = mme_ue_context_exists_imsi(&mme_app_desc.mme_ue_contexts,
                    imsi)) == NULL) {
    MME_APP_ERROR("That's embarrassing as we don't know this IMSI\n");
    MSC_LOG_EVENT(
    		MSC_MMEAPP_MME,
    		"0 NAS_AUTH_RESP Unknown imsi %"IMSI_FORMAT,imsi);
    AssertFatal(0, "That's embarrassing as we don't know this IMSI\n");
    return -1;
  }

  /* Consider the UE authenticated */
  ue_context->imsi_auth = IMSI_AUTHENTICATED;

  /* TODO: Get keys... */

  /* Now generate S6A ULR */
  {
    MessageDef                *message_p = NULL;
    s6a_update_location_req_t *s6a_ulr   = NULL;

    message_p = itti_alloc_new_message(TASK_MME_APP, S6A_UPDATE_LOCATION_REQ);

    if (message_p == NULL) {
      return -1;
    }

    s6a_ulr = &message_p->ittiMsg.s6a_update_location_req;

    memcpy(s6a_ulr->imsi, nas_auth_resp_pP->imsi, 16);
    s6a_ulr->initial_attach = INITIAL_ATTACH;
    s6a_ulr->rat_type = RAT_EUTRAN;
    /* Check if we already have UE data */
    s6a_ulr->skip_subscriber_data = 0;

    MSC_LOG_TX_MESSAGE(
    		MSC_MMEAPP_MME,
    		MSC_S6A_MME,
    		NULL,0,
    		" S6A_UPDATE_LOCATION_REQ IMSI %s RAT_EUTRAN",
    		imsi);

    return itti_send_msg_to_task(TASK_S6A, INSTANCE_DEFAULT, message_p);
  }
  return -1;
}

int
mme_app_handle_authentication_info_answer(
  const s6a_auth_info_ans_t * const s6a_auth_info_ans_pP)
{
  struct ue_context_s *ue_context = NULL;
  uint64_t             imsi       = 0;

  DevAssert(s6a_auth_info_ans_pP != NULL);

  MME_APP_STRING_TO_IMSI((char *)s6a_auth_info_ans_pP->imsi, &imsi);

  MME_APP_DEBUG("Handling imsi %"IMSI_FORMAT"\n", imsi);


  if ((ue_context = mme_ue_context_exists_imsi(&mme_app_desc.mme_ue_contexts,
                    imsi)) == NULL) {
    MME_APP_ERROR("That's embarrassing as we don't know this IMSI\n");
    MSC_LOG_EVENT(
    		MSC_MMEAPP_MME,
    		"0 S6A_AUTH_INFO_ANS Unknown imsi %"IMSI_FORMAT,imsi);
    return -1;
  }

  if ((s6a_auth_info_ans_pP->result.present == S6A_RESULT_BASE) &&
      (s6a_auth_info_ans_pP->result.choice.base == DIAMETER_SUCCESS)) {
    /* S6A procedure has succeeded.
     * We have to request UE authentication.
     */

    /* Check that list is not empty and contain only one element */
    DevCheck(s6a_auth_info_ans_pP->auth_info.nb_of_vectors == 1,
             s6a_auth_info_ans_pP->auth_info.nb_of_vectors, 1, 0);

    if (ue_context->vector_list == NULL) {
      ue_context->vector_list = malloc(sizeof(eutran_vector_t));
      DevAssert(ue_context->vector_list != NULL);
    } else {
      /* Some vector already exist */
      ue_context->vector_list = realloc(
                                  ue_context->vector_list,
                                  (ue_context->nb_of_vectors + s6a_auth_info_ans_pP->auth_info.nb_of_vectors) * sizeof(eutran_vector_t));

      DevAssert(ue_context->vector_list != NULL);
    }

    memcpy(&ue_context->vector_list[ue_context->nb_of_vectors],
           &s6a_auth_info_ans_pP->auth_info.eutran_vector, sizeof(eutran_vector_t));

    ue_context->vector_in_use = &ue_context->vector_list[ue_context->nb_of_vectors];

    ue_context->nb_of_vectors += s6a_auth_info_ans_pP->auth_info.nb_of_vectors;

    MME_APP_DEBUG("INFORMING NAS ABOUT AUTH RESP SUCCESS got %u vector(s)\n",
                  s6a_auth_info_ans_pP->auth_info.nb_of_vectors);

    mme_app_itti_auth_rsp(ue_context->ue_id,
                          1,
                          &s6a_auth_info_ans_pP->auth_info.eutran_vector);
  } else {
    MME_APP_ERROR("INFORMING NAS ABOUT AUTH RESP ERROR CODE\n");
    MSC_LOG_EVENT(
    		MSC_MMEAPP_MME,
    		"0 S6A_AUTH_INFO_ANS S6A Failure imsi %"IMSI_FORMAT,imsi);



    /* Inform NAS layer with the right failure */
    if (s6a_auth_info_ans_pP->result.present == S6A_RESULT_BASE) {
      mme_app_itti_auth_fail(ue_context->ue_id, s6a_error_2_nas_cause(
                               s6a_auth_info_ans_pP->result.choice.base, 0));
    } else {
      mme_app_itti_auth_fail(ue_context->ue_id, s6a_error_2_nas_cause(
                               s6a_auth_info_ans_pP->result.choice.experimental, 1));
    }
  }

  return 0;
}


void
mme_app_handle_nas_auth_param_req(
  const nas_auth_param_req_t * const nas_auth_param_req_pP)
{
  plmn_t              *visited_plmn  = NULL;
  struct ue_context_s *ue_context    = NULL;
  uint64_t             imsi          = 0;
  int                  mnc_length    = 0;
  plmn_t               visited_plmn_from_req = {
    .MCCdigit3 = 0,
    .MCCdigit2 = 0,
    .MCCdigit1 = 0,
    .MNCdigit1 = 0,
    .MNCdigit2 = 0,
    .MNCdigit3 = 0,
  };
  DevAssert(nas_auth_param_req_pP != NULL);

  visited_plmn = &visited_plmn_from_req;

  visited_plmn_from_req.MCCdigit1 = nas_auth_param_req_pP->imsi[0];
  visited_plmn_from_req.MCCdigit2 = nas_auth_param_req_pP->imsi[1];
  visited_plmn_from_req.MCCdigit3 = nas_auth_param_req_pP->imsi[2];

  mnc_length = find_mnc_length(
                 nas_auth_param_req_pP->imsi[0],
                 nas_auth_param_req_pP->imsi[1],
                 nas_auth_param_req_pP->imsi[2],
                 nas_auth_param_req_pP->imsi[3],
                 nas_auth_param_req_pP->imsi[4],
                 nas_auth_param_req_pP->imsi[5]
               );

  if (mnc_length == 2) {
    visited_plmn_from_req.MNCdigit1 = nas_auth_param_req_pP->imsi[3];
    visited_plmn_from_req.MNCdigit2 = nas_auth_param_req_pP->imsi[4];
    visited_plmn_from_req.MNCdigit3 = 15;
  } else if (mnc_length == 3) {
    visited_plmn_from_req.MNCdigit1 = nas_auth_param_req_pP->imsi[3];
    visited_plmn_from_req.MNCdigit2 = nas_auth_param_req_pP->imsi[4];
    visited_plmn_from_req.MNCdigit3 = nas_auth_param_req_pP->imsi[5];
  } else {
    AssertFatal(0, "MNC Not found (mcc_mnc_list)");
  }

  if (mnc_length == 3) {
    MME_APP_DEBUG("%s visited_plmn_from_req  %1d%1d%1d.%1d%1d%1d\n",
                  __FUNCTION__,
                  visited_plmn_from_req.MCCdigit1,
                  visited_plmn_from_req.MCCdigit2,
                  visited_plmn_from_req.MCCdigit3,
                  visited_plmn_from_req.MNCdigit1,
                  visited_plmn_from_req.MNCdigit2,
                  visited_plmn_from_req.MNCdigit3);
  } else {
    MME_APP_DEBUG("%s visited_plmn_from_req  %1d%1d%1d.%1d%1d\n",
                  __FUNCTION__,
                  visited_plmn_from_req.MCCdigit1,
                  visited_plmn_from_req.MCCdigit2,
                  visited_plmn_from_req.MCCdigit3,
                  visited_plmn_from_req.MNCdigit1,
                  visited_plmn_from_req.MNCdigit2);
  }

  MME_APP_STRING_TO_IMSI(nas_auth_param_req_pP->imsi, &imsi);

  MME_APP_DEBUG("%s Handling imsi %"IMSI_FORMAT"\n", __FUNCTION__, imsi);
  MME_APP_DEBUG("%s Handling imsi from req  %s (mnc length %d)\n",
                __FUNCTION__,
                nas_auth_param_req_pP->imsi,
                mnc_length);

  /* Fetch the context associated with this IMSI */
  ue_context = mme_ue_context_exists_imsi(&mme_app_desc.mme_ue_contexts, imsi);

  if (ue_context == NULL) {
    /* Currently no context available -> trigger an authentication request
     * to the HSS.
     */
    MME_APP_DEBUG("UE context search by IMSI failed, try by ue id\n");
    ue_context = mme_ue_context_exists_nas_ue_id(&mme_app_desc.mme_ue_contexts, nas_auth_param_req_pP->ue_id);

    if (ue_context == NULL) {
      // should have been created by initial ue message
      MME_APP_ERROR("UE context doesn't exist -> create one\n");

      if ((ue_context = mme_create_new_ue_context()) == NULL) {
        /* Error during ue context malloc */
        /* TODO */
        DevMessage("mme_create_new_ue_context");
        return;
      }

      ue_context->ue_id          = nas_auth_param_req_pP->ue_id;
      ue_context->mme_ue_s1ap_id = nas_auth_param_req_pP->ue_id;
      DevAssert(mme_insert_ue_context(&mme_app_desc.mme_ue_contexts, ue_context) == 0);
    }

    ue_context->imsi  = imsi;

    /* We have no vector for this UE, send an authentication request
     * to the HSS.
     */

    /* Acquire the current time */
    time(&ue_context->cell_age);

    memcpy(&ue_context->guti.gummei.plmn, visited_plmn, sizeof(plmn_t));
    MME_APP_DEBUG("and we have no auth. vector for it, request"
                  " authentication information\n");
    mme_app_request_authentication_info(nas_auth_param_req_pP->imsi, 1, visited_plmn, NULL);
  } else {
    memcpy(&ue_context->guti.gummei.plmn, visited_plmn, sizeof(plmn_t));

    mme_app_request_authentication_info(nas_auth_param_req_pP->imsi, 1, visited_plmn, nas_auth_param_req_pP->auts);
  }
}
