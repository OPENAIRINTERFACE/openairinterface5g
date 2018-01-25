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

/*! \file s1ap_eNB_handlers.c
 * \brief s1ap messages handlers for eNB part
 * \author Sebastien ROUX and Navid Nikaein
 * \email navid.nikaein@eurecom.fr
 * \date 2013 - 2015
 * \version 0.1
 */

#include <stdint.h>

#include "intertask_interface.h"

#include "asn1_conversions.h"

#include "s1ap_common.h"
#include "s1ap_ies_defs.h"
// #include "s1ap_eNB.h"
#include "s1ap_eNB_defs.h"
#include "s1ap_eNB_handlers.h"
#include "s1ap_eNB_decoder.h"

#include "s1ap_eNB_ue_context.h"
#include "s1ap_eNB_trace.h"
#include "s1ap_eNB_nas_procedures.h"
#include "s1ap_eNB_management_procedures.h"

#include "s1ap_eNB_default_values.h"

#include "assertions.h"
#include "conversions.h"
#include "msc.h"

static
int s1ap_eNB_handle_s1_setup_response(uint32_t               assoc_id,
                                      uint32_t               stream,
                                      struct s1ap_message_s *message_p);
static
int s1ap_eNB_handle_s1_setup_failure(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     struct s1ap_message_s *message_p);

static
int s1ap_eNB_handle_error_indication(uint32_t               assoc_id,
    uint32_t               stream,
    struct s1ap_message_s *message_p);

static
int s1ap_eNB_handle_initial_context_request(uint32_t               assoc_id,
    uint32_t               stream,
    struct s1ap_message_s *message_p);

static
int s1ap_eNB_handle_ue_context_release_command(uint32_t               assoc_id,
    uint32_t               stream,
    struct s1ap_message_s *s1ap_message_p);


static
int s1ap_eNB_handle_e_rab_setup_request(uint32_t               assoc_id,
					uint32_t               stream,
					struct s1ap_message_s *s1ap_message_p);

static
int s1ap_eNB_handle_paging(uint32_t               assoc_id,
    uint32_t               stream,
    struct s1ap_message_s *message_p);

static
int s1ap_eNB_handle_e_rab_modify_request(uint32_t               assoc_id,
    uint32_t               stream,
    struct s1ap_message_s *s1ap_message_p);

static
int s1ap_eNB_handle_e_rab_release_command(uint32_t               assoc_id,
    uint32_t               stream,
    struct s1ap_message_s *s1ap_message_p);

/* Handlers matrix. Only eNB related procedure present here */
s1ap_message_decoded_callback messages_callback[][3] = {
  { 0, 0, 0 }, /* HandoverPreparation */
  { 0, 0, 0 }, /* HandoverResourceAllocation */
  { 0, 0, 0 }, /* HandoverNotification */
  { 0, 0, 0 }, /* PathSwitchRequest */
  { 0, 0, 0 }, /* HandoverCancel */
  { s1ap_eNB_handle_e_rab_setup_request, 0, 0 }, /* E_RABSetup */
  { s1ap_eNB_handle_e_rab_modify_request, 0, 0 }, /* E_RABModify */
  { s1ap_eNB_handle_e_rab_release_command, 0, 0 }, /* E_RABRelease */
  { 0, 0, 0 }, /* E_RABReleaseIndication */
  { s1ap_eNB_handle_initial_context_request, 0, 0 }, /* InitialContextSetup */
  { s1ap_eNB_handle_paging, 0, 0 }, /* Paging */
  { s1ap_eNB_handle_nas_downlink, 0, 0 }, /* downlinkNASTransport */
  { 0, 0, 0 }, /* initialUEMessage */
  { 0, 0, 0 }, /* uplinkNASTransport */
  { 0, 0, 0 }, /* Reset */
  { s1ap_eNB_handle_error_indication, 0, 0 }, /* ErrorIndication */
  { 0, 0, 0 }, /* NASNonDeliveryIndication */
  { 0, s1ap_eNB_handle_s1_setup_response, s1ap_eNB_handle_s1_setup_failure }, /* S1Setup */
  { 0, 0, 0 }, /* UEContextReleaseRequest */
  { 0, 0, 0 }, /* DownlinkS1cdma2000tunneling */
  { 0, 0, 0 }, /* UplinkS1cdma2000tunneling */
  { 0, 0, 0 }, /* UEContextModification */
  { 0, 0, 0 }, /* UECapabilityInfoIndication */
  { s1ap_eNB_handle_ue_context_release_command, 0, 0 }, /* UEContextRelease */
  { 0, 0, 0 }, /* eNBStatusTransfer */
  { 0, 0, 0 }, /* MMEStatusTransfer */
  { s1ap_eNB_handle_deactivate_trace, 0, 0 }, /* DeactivateTrace */
  { s1ap_eNB_handle_trace_start, 0, 0 }, /* TraceStart */
  { 0, 0, 0 }, /* TraceFailureIndication */
  { 0, 0, 0 }, /* ENBConfigurationUpdate */
  { 0, 0, 0 }, /* MMEConfigurationUpdate */
  { 0, 0, 0 }, /* LocationReportingControl */
  { 0, 0, 0 }, /* LocationReportingFailureIndication */
  { 0, 0, 0 }, /* LocationReport */
  { 0, 0, 0 }, /* OverloadStart */
  { 0, 0, 0 }, /* OverloadStop */
  { 0, 0, 0 }, /* WriteReplaceWarning */
  { 0, 0, 0 }, /* eNBDirectInformationTransfer */
  { 0, 0, 0 }, /* MMEDirectInformationTransfer */
  { 0, 0, 0 }, /* PrivateMessage */
  { 0, 0, 0 }, /* eNBConfigurationTransfer */
  { 0, 0, 0 }, /* MMEConfigurationTransfer */
  { 0, 0, 0 }, /* CellTrafficTrace */
#if defined(UPDATE_RELEASE_9)
  { 0, 0, 0 }, /* Kill */
  { 0, 0, 0 }, /* DownlinkUEAssociatedLPPaTransport  */
  { 0, 0, 0 }, /* UplinkUEAssociatedLPPaTransport */
  { 0, 0, 0 }, /* DownlinkNonUEAssociatedLPPaTransport */
  { 0, 0, 0 }, /* UplinkNonUEAssociatedLPPaTransport */
#endif
};

static const char *s1ap_direction2String[] = {
  "", /* Nothing */
  "Originating message", /* originating message */
  "Successfull outcome", /* successfull outcome */
  "UnSuccessfull outcome", /* successfull outcome */
};

void s1ap_handle_s1_setup_message(s1ap_eNB_mme_data_t *mme_desc_p, int sctp_shutdown)
{
  if (sctp_shutdown) {
    /* A previously connected MME has been shutdown */

    /* TODO check if it was used by some eNB and send a message to inform these eNB if there is no more associated MME */
    if (mme_desc_p->state == S1AP_ENB_STATE_CONNECTED) {
      mme_desc_p->state = S1AP_ENB_STATE_DISCONNECTED;

      if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb > 0) {
        /* Decrease associated MME number */
        mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb --;
      }

      /* If there are no more associated MME, inform eNB app */
      if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb == 0) {
        MessageDef                 *message_p;

        message_p = itti_alloc_new_message(TASK_S1AP, S1AP_DEREGISTERED_ENB_IND);
        S1AP_DEREGISTERED_ENB_IND(message_p).nb_mme = 0;
        itti_send_msg_to_task(TASK_ENB_APP, mme_desc_p->s1ap_eNB_instance->instance, message_p);
      }
    }
  } else {
    /* Check that at least one setup message is pending */
    DevCheck(mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb > 0, mme_desc_p->s1ap_eNB_instance->instance,
             mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb, 0);

    if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb > 0) {
      /* Decrease pending messages number */
      mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb --;
    }

    /* If there are no more pending messages, inform eNB app */
    if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb == 0) {
      MessageDef                 *message_p;

      message_p = itti_alloc_new_message(TASK_S1AP, S1AP_REGISTER_ENB_CNF);
      S1AP_REGISTER_ENB_CNF(message_p).nb_mme = mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb;
      itti_send_msg_to_task(TASK_ENB_APP, mme_desc_p->s1ap_eNB_instance->instance, message_p);
    }
  }
}

int s1ap_eNB_handle_message(uint32_t assoc_id, int32_t stream,
                            const uint8_t * const data, const uint32_t data_length)
{
  struct s1ap_message_s message;

  DevAssert(data != NULL);

  memset(&message, 0, sizeof(struct s1ap_message_s));

  if (s1ap_eNB_decode_pdu(&message, data, data_length) < 0) {
    S1AP_ERROR("Failed to decode PDU\n");
    return -1;
  }

  /* Checking procedure Code and direction of message */
  if (message.procedureCode > sizeof(messages_callback) / (3 * sizeof(
        s1ap_message_decoded_callback))
      || (message.direction > S1AP_PDU_PR_unsuccessfulOutcome)) {
    S1AP_ERROR("[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, message.procedureCode, message.direction);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for eNB (wrong direction).
   */
  if (messages_callback[message.procedureCode][message.direction-1] == NULL) {
    S1AP_ERROR("[SCTP %d] No handler for procedureCode %ld in %s\n",
               assoc_id, message.procedureCode,
               s1ap_direction2String[message.direction]);
    return -1;
  }

  /* Calling the right handler */
  return (*messages_callback[message.procedureCode][message.direction-1])
         (assoc_id, stream, &message);
}

static
int s1ap_eNB_handle_s1_setup_failure(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     struct s1ap_message_s *message_p)
{
  S1ap_S1SetupFailureIEs_t   *s1_setup_failure_p;
  s1ap_eNB_mme_data_t        *mme_desc_p;

  DevAssert(message_p != NULL);

  s1_setup_failure_p = &message_p->msg.s1ap_S1SetupFailureIEs;

  /* S1 Setup Failure == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    S1AP_WARN("[SCTP %d] Received s1 setup failure on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received S1 setup response for non existing "
               "MME context\n", assoc_id);
    return -1;
  }

  if ((s1_setup_failure_p->cause.present == S1ap_Cause_PR_misc) &&
      (s1_setup_failure_p->cause.choice.misc == S1ap_CauseMisc_unspecified)) {
    S1AP_WARN("Received s1 setup failure for MME... MME is not ready\n");
  } else {
    S1AP_ERROR("Received s1 setup failure for MME... please check your parameters\n");
  }

  mme_desc_p->state = S1AP_ENB_STATE_WAITING;
  s1ap_handle_s1_setup_message(mme_desc_p, 0);

  return 0;
}

static
int s1ap_eNB_handle_s1_setup_response(uint32_t               assoc_id,
                                      uint32_t               stream,
                                      struct s1ap_message_s *message_p)
{
  S1ap_S1SetupResponseIEs_t *s1SetupResponse_p;
  s1ap_eNB_mme_data_t       *mme_desc_p;
  int i;

  DevAssert(message_p != NULL);

  s1SetupResponse_p = &message_p->msg.s1ap_S1SetupResponseIEs;

  /* S1 Setup Response == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    S1AP_ERROR("[SCTP %d] Received s1 setup response on stream != 0 (%d)\n",
               assoc_id, stream);
    return -1;
  }

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received S1 setup response for non existing "
               "MME context\n", assoc_id);
    return -1;
  }

  /* The list of served gummei can contain at most 8 elements.
   * LTE related gummei is the first element in the list, i.e with an id of 0.
   */
  S1AP_DEBUG("servedGUMMEIs.list.count %d\n",s1SetupResponse_p->servedGUMMEIs.list.count); 
  DevAssert(s1SetupResponse_p->servedGUMMEIs.list.count > 0);
  DevAssert(s1SetupResponse_p->servedGUMMEIs.list.count <= 8);


  for (i = 0; i < s1SetupResponse_p->servedGUMMEIs.list.count; i++) {
    struct S1ap_ServedGUMMEIsItem *gummei_item_p;
    struct served_gummei_s        *new_gummei_p;
    int j;

    gummei_item_p = (struct S1ap_ServedGUMMEIsItem *)
                    s1SetupResponse_p->servedGUMMEIs.list.array[i];
    new_gummei_p = calloc(1, sizeof(struct served_gummei_s));

    STAILQ_INIT(&new_gummei_p->served_plmns);
    STAILQ_INIT(&new_gummei_p->served_group_ids);
    STAILQ_INIT(&new_gummei_p->mme_codes);
    
    S1AP_DEBUG("servedPLMNs.list.count %d\n",gummei_item_p->servedPLMNs.list.count);
    for (j = 0; j < gummei_item_p->servedPLMNs.list.count; j++) {
      S1ap_PLMNidentity_t *plmn_identity_p;
      struct plmn_identity_s *new_plmn_identity_p;
      
      plmn_identity_p = gummei_item_p->servedPLMNs.list.array[j];
      new_plmn_identity_p = calloc(1, sizeof(struct plmn_identity_s));
      TBCD_TO_MCC_MNC(plmn_identity_p, new_plmn_identity_p->mcc,
                      new_plmn_identity_p->mnc, new_plmn_identity_p->mnc_digit_length);
      STAILQ_INSERT_TAIL(&new_gummei_p->served_plmns, new_plmn_identity_p, next);
      new_gummei_p->nb_served_plmns++;
    }

    for (j = 0; j < gummei_item_p->servedGroupIDs.list.count; j++) {
      S1ap_MME_Group_ID_t           *mme_group_id_p;
      struct served_group_id_s *new_group_id_p;

      mme_group_id_p = gummei_item_p->servedGroupIDs.list.array[j];
      new_group_id_p = calloc(1, sizeof(struct served_group_id_s));
      OCTET_STRING_TO_INT16(mme_group_id_p, new_group_id_p->mme_group_id);
      STAILQ_INSERT_TAIL(&new_gummei_p->served_group_ids, new_group_id_p, next);
      new_gummei_p->nb_group_id++;
    }

    for (j = 0; j < gummei_item_p->servedMMECs.list.count; j++) {
      S1ap_MME_Code_t        *mme_code_p;
      struct mme_code_s *new_mme_code_p;

      mme_code_p = gummei_item_p->servedMMECs.list.array[j];
      new_mme_code_p = calloc(1, sizeof(struct mme_code_s));

      OCTET_STRING_TO_INT8(mme_code_p, new_mme_code_p->mme_code);
      STAILQ_INSERT_TAIL(&new_gummei_p->mme_codes, new_mme_code_p, next);
      new_gummei_p->nb_mme_code++;
    }

    STAILQ_INSERT_TAIL(&mme_desc_p->served_gummei, new_gummei_p, next);
  }

  /* Free contents of the list */
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1ap_ServedGUMMEIs,
                                (void *)&s1SetupResponse_p->servedGUMMEIs);
  /* Set the capacity of this MME */
  mme_desc_p->relative_mme_capacity = s1SetupResponse_p->relativeMMECapacity;

  /* Optionaly set the mme name */
  if (s1SetupResponse_p->presenceMask & S1AP_S1SETUPRESPONSEIES_MMENAME_PRESENT) {
    mme_desc_p->mme_name = calloc(s1SetupResponse_p->mmEname.size + 1, sizeof(char));
    memcpy(mme_desc_p->mme_name, s1SetupResponse_p->mmEname.buf,
           s1SetupResponse_p->mmEname.size);
    /* Convert the mme name to a printable string */
    mme_desc_p->mme_name[s1SetupResponse_p->mmEname.size] = '\0';
  }

  /* The association is now ready as eNB and MME know parameters of each other.
   * Mark the association as UP to enable UE contexts creation.
   */
  mme_desc_p->state = S1AP_ENB_STATE_CONNECTED;
  mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb ++;
  s1ap_handle_s1_setup_message(mme_desc_p, 0);

#if 0
  /* We call back our self
   * -> generate a dummy initial UE message
   */
  {
    s1ap_nas_first_req_t s1ap_nas_first_req;

    memset(&s1ap_nas_first_req, 0, sizeof(s1ap_nas_first_req_t));

    s1ap_nas_first_req.rnti = 0xC03A;
    s1ap_nas_first_req.establishment_cause = RRC_CAUSE_MO_DATA;
    s1ap_nas_first_req.ue_identity.presenceMask = UE_IDENTITIES_gummei;

    s1ap_nas_first_req.ue_identity.gummei.mcc = 208;
    s1ap_nas_first_req.ue_identity.gummei.mnc = 34;
    s1ap_nas_first_req.ue_identity.gummei.mme_code = 0;
    s1ap_nas_first_req.ue_identity.gummei.mme_group_id = 0;

    /* NAS Attach request with IMSI */
    static uint8_t nas_attach_req_imsi[] = {
      0x07, 0x41,
      /* EPS Mobile identity = IMSI */
      0x71, 0x08, 0x29, 0x80, 0x43, 0x21, 0x43, 0x65, 0x87,
      0xF9,
      /* End of EPS Mobile Identity */
      0x02, 0xE0, 0xE0, 0x00, 0x20, 0x02, 0x03,
      0xD0, 0x11, 0x27, 0x1A, 0x80, 0x80, 0x21, 0x10, 0x01, 0x00, 0x00,
      0x10, 0x81, 0x06, 0x00, 0x00, 0x00, 0x00, 0x83, 0x06, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x0A, 0x00, 0x52, 0x12, 0xF2,
      0x01, 0x27, 0x11,
    };

    /* NAS Attach request with GUTI */
    static uint8_t nas_attach_req_guti[] = {
      0x07, 0x41,
      /* EPS Mobile identity = IMSI */
      0x71, 0x0B, 0xF6, 0x12, 0xF2, 0x01, 0x80, 0x00, 0x01, 0xE0, 0x00,
      0xDA, 0x1F,
      /* End of EPS Mobile Identity */
      0x02, 0xE0, 0xE0, 0x00, 0x20, 0x02, 0x03,
      0xD0, 0x11, 0x27, 0x1A, 0x80, 0x80, 0x21, 0x10, 0x01, 0x00, 0x00,
      0x10, 0x81, 0x06, 0x00, 0x00, 0x00, 0x00, 0x83, 0x06, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x0A, 0x00, 0x52, 0x12, 0xF2,
      0x01, 0x27, 0x11,
    };

    s1ap_nas_first_req.nas_pdu.buffer = nas_attach_req_guti;
    s1ap_nas_first_req.nas_pdu.length = sizeof(nas_attach_req_guti);

    s1ap_eNB_handle_nas_first_req(mme_desc_p->s1ap_eNB_instance->instance,
                                  &s1ap_nas_first_req);
  }
#endif

  return 0;
}


static
int s1ap_eNB_handle_error_indication(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     struct s1ap_message_s *message_p)
{
  S1ap_ErrorIndicationIEs_t   *s1_error_indication_p;
  s1ap_eNB_mme_data_t        *mme_desc_p;

  DevAssert(message_p != NULL);

  s1_error_indication_p = &message_p->msg.s1ap_ErrorIndicationIEs;

  /* S1 Setup Failure == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    S1AP_WARN("[SCTP %d] Received s1 Error indication on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received S1 Error indication for non existing "
               "MME context\n", assoc_id);
    return -1;
  }
  if ( s1_error_indication_p->presenceMask & S1AP_ERRORINDICATIONIES_MME_UE_S1AP_ID_PRESENT) {
	  	S1AP_WARN("Received S1 Error indication MME UE S1AP ID 0x%lx\n", s1_error_indication_p->mme_ue_s1ap_id);
  }
  if ( s1_error_indication_p->presenceMask & S1AP_ERRORINDICATIONIES_ENB_UE_S1AP_ID_PRESENT) {
  	S1AP_WARN("Received S1 Error indication eNB UE S1AP ID 0x%lx\n", s1_error_indication_p->eNB_UE_S1AP_ID);
  }

  if ( s1_error_indication_p->presenceMask & S1AP_ERRORINDICATIONIES_CAUSE_PRESENT) {
    switch(s1_error_indication_p->cause.present) {
      case S1ap_Cause_PR_NOTHING:
    	S1AP_WARN("Received S1 Error indication cause NOTHING\n");
      break;
      case S1ap_Cause_PR_radioNetwork:
      	switch (s1_error_indication_p->cause.choice.radioNetwork) {
	      case S1ap_CauseRadioNetwork_unspecified:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_unspecified\n");
            break;
  	      case S1ap_CauseRadioNetwork_tx2relocoverall_expiry:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_tx2relocoverall_expiry\n");
            break;
  	      case S1ap_CauseRadioNetwork_successful_handover:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_successful_handover\n");
            break;
  	      case S1ap_CauseRadioNetwork_release_due_to_eutran_generated_reason:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_release_due_to_eutran_generated_reason\n");
            break;
  	      case S1ap_CauseRadioNetwork_handover_cancelled:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_handover_cancelled\n");
            break;
  	      case S1ap_CauseRadioNetwork_partial_handover:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_partial_handover\n");
            break;
  	      case S1ap_CauseRadioNetwork_ho_failure_in_target_EPC_eNB_or_target_system:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_ho_failure_in_target_EPC_eNB_or_target_system\n");
            break;
  	      case S1ap_CauseRadioNetwork_ho_target_not_allowed:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_ho_target_not_allowed\n");
            break;
  	      case S1ap_CauseRadioNetwork_tS1relocoverall_expiry:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_tS1relocoverall_expiry\n");
            break;
  	      case S1ap_CauseRadioNetwork_tS1relocprep_expiry:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_tS1relocprep_expiry\n");
            break;
  	      case S1ap_CauseRadioNetwork_cell_not_available:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_cell_not_available\n");
            break;
  	      case S1ap_CauseRadioNetwork_unknown_targetID:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_unknown_targetID\n");
            break;
  	      case S1ap_CauseRadioNetwork_no_radio_resources_available_in_target_cell:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_no_radio_resources_available_in_target_cell\n");
            break;
  	      case S1ap_CauseRadioNetwork_unknown_mme_ue_s1ap_id:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_unknown_mme_ue_s1ap_id\n");
            break;
  	      case S1ap_CauseRadioNetwork_unknown_enb_ue_s1ap_id:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_unknown_enb_ue_s1ap_id\n");
            break;
  	      case S1ap_CauseRadioNetwork_unknown_pair_ue_s1ap_id:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_unknown_pair_ue_s1ap_id\n");
            break;
  	      case S1ap_CauseRadioNetwork_handover_desirable_for_radio_reason:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_handover_desirable_for_radio_reason\n");
            break;
  	      case S1ap_CauseRadioNetwork_time_critical_handover:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_time_critical_handover\n");
            break;
  	      case S1ap_CauseRadioNetwork_resource_optimisation_handover:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_resource_optimisation_handover\n");
            break;
  	      case S1ap_CauseRadioNetwork_reduce_load_in_serving_cell:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_reduce_load_in_serving_cell\n");
            break;
  	      case S1ap_CauseRadioNetwork_user_inactivity:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_user_inactivity\n");
            break;
  	      case S1ap_CauseRadioNetwork_radio_connection_with_ue_lost:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_radio_connection_with_ue_lost\n");
            break;
  	      case S1ap_CauseRadioNetwork_load_balancing_tau_required:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_load_balancing_tau_required\n");
            break;
  	      case S1ap_CauseRadioNetwork_cs_fallback_triggered:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_cs_fallback_triggered\n");
            break;
  	      case S1ap_CauseRadioNetwork_ue_not_available_for_ps_service:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_ue_not_available_for_ps_service\n");
            break;
  	      case S1ap_CauseRadioNetwork_radio_resources_not_available:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_radio_resources_not_available\n");
            break;
  	      case S1ap_CauseRadioNetwork_failure_in_radio_interface_procedure:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_failure_in_radio_interface_procedure\n");
            break;
  	      case S1ap_CauseRadioNetwork_invals1ap_id_qos_combination:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_invals1ap_id_qos_combination\n");
            break;
  	      case S1ap_CauseRadioNetwork_interrat_redirection:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_interrat_redirection\n");
            break;
  	      case S1ap_CauseRadioNetwork_interaction_with_other_procedure:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_interaction_with_other_procedure\n");
            break;
  	      case S1ap_CauseRadioNetwork_unknown_E_RAB_ID:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_unknown_E_RAB_ID\n");
            break;
  	      case S1ap_CauseRadioNetwork_multiple_E_RAB_ID_instances:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_multiple_E_RAB_ID_instances\n");
            break;
  	      case S1ap_CauseRadioNetwork_encryption_and_or_integrity_protection_algorithms_not_supported:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_encryption_and_or_integrity_protection_algorithms_not_supported\n");
            break;
  	      case S1ap_CauseRadioNetwork_s1_intra_system_handover_triggered:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_s1_intra_system_handover_triggered\n");
            break;
  	      case S1ap_CauseRadioNetwork_s1_inter_system_handover_triggered:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_s1_inter_system_handover_triggered\n");
            break;
  	      case S1ap_CauseRadioNetwork_x2_handover_triggered:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_x2_handover_triggered\n");
            break;
  	      case S1ap_CauseRadioNetwork_redirection_towards_1xRTT:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_redirection_towards_1xRTT\n");
            break;
  	      case S1ap_CauseRadioNetwork_not_supported_QCI_value:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_not_supported_QCI_value\n");
            break;
  	      case S1ap_CauseRadioNetwork_invals1ap_id_CSG_Id:
            S1AP_WARN("Received S1 Error indication S1ap_CauseRadioNetwork_invals1ap_id_CSG_Id\n");
            break;
      	  default:
            S1AP_WARN("Received S1 Error indication cause radio network case not handled\n");
      	}
      break;

      case S1ap_Cause_PR_transport:
      	switch (s1_error_indication_p->cause.choice.transport) {
    	  case S1ap_CauseTransport_transport_resource_unavailable:
            S1AP_WARN("Received S1 Error indication S1ap_CauseTransport_transport_resource_unavailable\n");
            break;
    	  case S1ap_CauseTransport_unspecified:
            S1AP_WARN("Received S1 Error indication S1ap_CauseTransport_unspecified\n");
            break;
      	  default:
            S1AP_WARN("Received S1 Error indication cause transport case not handled\n");
      	}
      break;

      case S1ap_Cause_PR_nas:
      	switch (s1_error_indication_p->cause.choice.nas) {
    	  case S1ap_CauseNas_normal_release:
            S1AP_WARN("Received S1 Error indication S1ap_CauseNas_normal_release\n");
            break;
      	  case S1ap_CauseNas_authentication_failure:
            S1AP_WARN("Received S1 Error indication S1ap_CauseNas_authentication_failure\n");
            break;
      	  case S1ap_CauseNas_detach:
            S1AP_WARN("Received S1 Error indication S1ap_CauseNas_detach\n");
            break;
      	  case S1ap_CauseNas_unspecified:
            S1AP_WARN("Received S1 Error indication S1ap_CauseNas_unspecified\n");
            break;
      	  case S1ap_CauseNas_csg_subscription_expiry:
            S1AP_WARN("Received S1 Error indication S1ap_CauseNas_csg_subscription_expiry\n");
            break;
      	  default:
            S1AP_WARN("Received S1 Error indication cause nas case not handled\n");
      	}
      break;

      case S1ap_Cause_PR_protocol:
      	switch (s1_error_indication_p->cause.choice.protocol) {
      	  case S1ap_CauseProtocol_transfer_syntax_error:
            S1AP_WARN("Received S1 Error indication S1ap_CauseProtocol_transfer_syntax_error\n");
            break;
      	  case S1ap_CauseProtocol_abstract_syntax_error_reject:
            S1AP_WARN("Received S1 Error indication S1ap_CauseProtocol_abstract_syntax_error_reject\n");
            break;
      	  case S1ap_CauseProtocol_abstract_syntax_error_ignore_and_notify:
            S1AP_WARN("Received S1 Error indication S1ap_CauseProtocol_abstract_syntax_error_ignore_and_notify\n");
            break;
      	  case S1ap_CauseProtocol_message_not_compatible_with_receiver_state:
            S1AP_WARN("Received S1 Error indication S1ap_CauseProtocol_message_not_compatible_with_receiver_state\n");
            break;
      	  case S1ap_CauseProtocol_semantic_error:
            S1AP_WARN("Received S1 Error indication S1ap_CauseProtocol_semantic_error\n");
            break;
      	  case S1ap_CauseProtocol_abstract_syntax_error_falsely_constructed_message:
            S1AP_WARN("Received S1 Error indication S1ap_CauseProtocol_abstract_syntax_error_falsely_constructed_message\n");
            break;
      	  case S1ap_CauseProtocol_unspecified:
            S1AP_WARN("Received S1 Error indication S1ap_CauseProtocol_unspecified\n");
            break;
      	  default:
            S1AP_WARN("Received S1 Error indication cause protocol case not handled\n");
      	}
      break;

      case S1ap_Cause_PR_misc:
        switch (s1_error_indication_p->cause.choice.protocol) {
          case S1ap_CauseMisc_control_processing_overload:
            S1AP_WARN("Received S1 Error indication S1ap_CauseMisc_control_processing_overload\n");
            break;
          case S1ap_CauseMisc_not_enough_user_plane_processing_resources:
        	S1AP_WARN("Received S1 Error indication S1ap_CauseMisc_not_enough_user_plane_processing_resources\n");
        	break;
          case S1ap_CauseMisc_hardware_failure:
        	S1AP_WARN("Received S1 Error indication S1ap_CauseMisc_hardware_failure\n");
        	break;
          case S1ap_CauseMisc_om_intervention:
        	S1AP_WARN("Received S1 Error indication S1ap_CauseMisc_om_intervention\n");
        	break;
          case S1ap_CauseMisc_unspecified:
        	S1AP_WARN("Received S1 Error indication S1ap_CauseMisc_unspecified\n");
        	break;
          case S1ap_CauseMisc_unknown_PLMN:
        	S1AP_WARN("Received S1 Error indication S1ap_CauseMisc_unknown_PLMN\n");
        	break;
          default:
            S1AP_WARN("Received S1 Error indication cause misc case not handled\n");
        }
      break;
    }
  }
  if ( s1_error_indication_p->presenceMask & S1AP_ERRORINDICATIONIES_CRITICALITYDIAGNOSTICS_PRESENT) {
    // TODO continue
  }
  // TODO continue

  return 0;
}


static
int s1ap_eNB_handle_initial_context_request(uint32_t               assoc_id,
    uint32_t               stream,
    struct s1ap_message_s *s1ap_message_p)
{
  int i;

  s1ap_eNB_mme_data_t   *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;

  S1ap_InitialContextSetupRequestIEs_t *initialContextSetupRequest_p;
  DevAssert(s1ap_message_p != NULL);

  initialContextSetupRequest_p = &s1ap_message_p->msg.s1ap_InitialContextSetupRequestIEs;

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received initial context setup request for non "
               "existing MME context\n", assoc_id);
    return -1;
  }

  if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
                   initialContextSetupRequest_p->eNB_UE_S1AP_ID)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received initial context setup request for non "
               "existing UE context 0x%06lx\n", assoc_id,
               initialContextSetupRequest_p->eNB_UE_S1AP_ID);
    return -1;
  }

  /* Initial context request = UE-related procedure -> stream != 0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
               assoc_id, stream);
    return -1;
  }

  ue_desc_p->rx_stream = stream;

  ue_desc_p->mme_ue_s1ap_id = initialContextSetupRequest_p->mme_ue_s1ap_id;

  message_p        = itti_alloc_new_message(TASK_S1AP, S1AP_INITIAL_CONTEXT_SETUP_REQ);

  S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).ue_initial_id  = ue_desc_p->ue_initial_id;
  ue_desc_p->ue_initial_id = 0;

  S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).eNB_ue_s1ap_id = ue_desc_p->eNB_ue_s1ap_id;
  S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).nb_of_e_rabs =
    initialContextSetupRequest_p->e_RABToBeSetupListCtxtSUReq.s1ap_E_RABToBeSetupItemCtxtSUReq.count;

  S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).ue_ambr.br_ul = 64;// TO DO(bitrate_t)(initialContextSetupRequest_p->uEaggregateMaximumBitrate.uEaggregateMaximumBitRateUL);
  S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).ue_ambr.br_dl = 1024;//(bitrate_t)(initialContextSetupRequest_p->uEaggregateMaximumBitrate.uEaggregateMaximumBitRateDL);

  S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_capabilities.encryption_algorithms =
    BIT_STRING_to_uint16(&initialContextSetupRequest_p->ueSecurityCapabilities.encryptionAlgorithms);
  S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_capabilities.integrity_algorithms =
    BIT_STRING_to_uint16(&initialContextSetupRequest_p->ueSecurityCapabilities.integrityProtectionAlgorithms);

  /* Copy the security key */
  memcpy(&S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_key,
         initialContextSetupRequest_p->securityKey.buf, initialContextSetupRequest_p->securityKey.size);

  for (i = 0; i < initialContextSetupRequest_p->e_RABToBeSetupListCtxtSUReq.s1ap_E_RABToBeSetupItemCtxtSUReq.count; i++) {
    S1ap_E_RABToBeSetupItemCtxtSUReq_t *item_p;

    item_p = (S1ap_E_RABToBeSetupItemCtxtSUReq_t *)initialContextSetupRequest_p->e_RABToBeSetupListCtxtSUReq.s1ap_E_RABToBeSetupItemCtxtSUReq.array[i];

    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].e_rab_id = item_p->e_RAB_ID;

    if (item_p->nAS_PDU != NULL) {
      /* Only copy NAS pdu if present */
      S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].nas_pdu.length = item_p->nAS_PDU->size;

      S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].nas_pdu.buffer =
        malloc(sizeof(uint8_t) * item_p->nAS_PDU->size);

      memcpy(S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].nas_pdu.buffer,
             item_p->nAS_PDU->buf, item_p->nAS_PDU->size); 
      S1AP_DEBUG("Received NAS message with the E_RAB setup procedure\n");
    } else {
      S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].nas_pdu.length = 0;
      S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].nas_pdu.buffer = NULL;
    }

    /* Set the transport layer address */
    memcpy(S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].sgw_addr.buffer,
           item_p->transportLayerAddress.buf, item_p->transportLayerAddress.size);
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].sgw_addr.length =
      item_p->transportLayerAddress.size * 8 - item_p->transportLayerAddress.bits_unused;

    /* GTP tunnel endpoint ID */
    OCTET_STRING_TO_INT32(&item_p->gTP_TEID, S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].gtp_teid);

    /* Set the QOS informations */
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].qos.qci = item_p->e_RABlevelQoSParameters.qCI;

    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].qos.allocation_retention_priority.priority_level =
      item_p->e_RABlevelQoSParameters.allocationRetentionPriority.priorityLevel;
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].qos.allocation_retention_priority.pre_emp_capability =
      item_p->e_RABlevelQoSParameters.allocationRetentionPriority.pre_emptionCapability;
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).e_rab_param[i].qos.allocation_retention_priority.pre_emp_vulnerability =
      item_p->e_RABlevelQoSParameters.allocationRetentionPriority.pre_emptionVulnerability;
  }

  itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);

  return 0;
}


static
int s1ap_eNB_handle_ue_context_release_command(uint32_t               assoc_id,
    uint32_t               stream,
    struct s1ap_message_s *s1ap_message_p)
{
  s1ap_eNB_mme_data_t   *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;

  S1ap_UEContextReleaseCommandIEs_t *ueContextReleaseCommand_p;
  DevAssert(s1ap_message_p != NULL);

  ueContextReleaseCommand_p = &s1ap_message_p->msg.s1ap_UEContextReleaseCommandIEs;

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received UE context release command for non "
               "existing MME context\n", assoc_id);
    return -1;
  }

  S1ap_MME_UE_S1AP_ID_t    mme_ue_s1ap_id;
  S1ap_ENB_UE_S1AP_ID_t    enb_ue_s1ap_id;

  switch (ueContextReleaseCommand_p->uE_S1AP_IDs.present) {
  case S1ap_UE_S1AP_IDs_PR_uE_S1AP_ID_pair:
    enb_ue_s1ap_id = ueContextReleaseCommand_p->uE_S1AP_IDs.choice.uE_S1AP_ID_pair.eNB_UE_S1AP_ID;
    mme_ue_s1ap_id = ueContextReleaseCommand_p->uE_S1AP_IDs.choice.uE_S1AP_ID_pair.mME_UE_S1AP_ID;

    MSC_LOG_RX_MESSAGE(
    		MSC_S1AP_ENB,
    		MSC_S1AP_MME,
    		NULL,0,
    		"0 UEContextRelease/%s eNB_ue_s1ap_id "S1AP_UE_ID_FMT" mme_ue_s1ap_id "S1AP_UE_ID_FMT" len %u",
  		s1ap_direction2String[s1ap_message_p->direction],
  		enb_ue_s1ap_id,
  		mme_ue_s1ap_id);

    if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
                     enb_ue_s1ap_id)) == NULL) {
      S1AP_ERROR("[SCTP %d] Received UE context release command for non "
                 "existing UE context 0x%06lx\n",
                 assoc_id,
                 enb_ue_s1ap_id);
      /*MessageDef *msg_complete_p;
      msg_complete_p = itti_alloc_new_message(TASK_RRC_ENB, S1AP_UE_CONTEXT_RELEASE_COMPLETE);
      S1AP_UE_CONTEXT_RELEASE_COMPLETE(msg_complete_p).eNB_ue_s1ap_id = enb_ue_s1ap_id;
      itti_send_msg_to_task(TASK_S1AP, ue_desc_p->eNB_instance->instance <=> 0, msg_complete_p);
      */
      return -1;
    } else {
      MSC_LOG_TX_MESSAGE(
    		  MSC_S1AP_ENB,
    		  MSC_RRC_ENB,
    		  NULL,0,
    		  "0 S1AP_UE_CONTEXT_RELEASE_COMMAND/%d eNB_ue_s1ap_id "S1AP_UE_ID_FMT" ",
    		  enb_ue_s1ap_id);

      message_p        = itti_alloc_new_message(TASK_S1AP, S1AP_UE_CONTEXT_RELEASE_COMMAND);
      S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).eNB_ue_s1ap_id = enb_ue_s1ap_id;
      itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);
      return 0;
    }

    break;

//#warning "TODO mapping mme_ue_s1ap_id  enb_ue_s1ap_id?"

  case S1ap_UE_S1AP_IDs_PR_mME_UE_S1AP_ID:
    mme_ue_s1ap_id = ueContextReleaseCommand_p->uE_S1AP_IDs.choice.mME_UE_S1AP_ID;
    S1AP_ERROR("TO DO mapping mme_ue_s1ap_id  enb_ue_s1ap_id");
    (void)mme_ue_s1ap_id; /* TODO: remove - it's to remove gcc warning about unused var */

  case S1ap_UE_S1AP_IDs_PR_NOTHING:
  default:
    S1AP_ERROR("S1AP_UE_CONTEXT_RELEASE_COMMAND not processed, missing info elements");
    return -1;
  }
}

static
int s1ap_eNB_handle_e_rab_setup_request(uint32_t               assoc_id,
					uint32_t               stream,
					struct s1ap_message_s *s1ap_message_p) {

  int i;

  s1ap_eNB_mme_data_t   *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;

  S1ap_E_RABSetupRequestIEs_t         *s1ap_E_RABSetupRequest;
  DevAssert(s1ap_message_p != NULL);

  s1ap_E_RABSetupRequest = &s1ap_message_p->msg.s1ap_E_RABSetupRequestIEs;

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received initial context setup request for non "
               "existing MME context\n", assoc_id);
    return -1;
  }

    
  if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
                   s1ap_E_RABSetupRequest->eNB_UE_S1AP_ID)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received initial context setup request for non "
               "existing UE context 0x%06lx\n", assoc_id,
               s1ap_E_RABSetupRequest->eNB_UE_S1AP_ID);
    return -1;
  }

  /* Initial context request = UE-related procedure -> stream != 0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
               assoc_id, stream);
    return -1;
  }

  ue_desc_p->rx_stream = stream;

  if ( ue_desc_p->mme_ue_s1ap_id != s1ap_E_RABSetupRequest->mme_ue_s1ap_id){
    S1AP_WARN("UE context mme_ue_s1ap_id is different form that of the message (%d != %ld)", 
	      ue_desc_p->mme_ue_s1ap_id, s1ap_E_RABSetupRequest->mme_ue_s1ap_id);

  }
  message_p        = itti_alloc_new_message(TASK_S1AP, S1AP_E_RAB_SETUP_REQ);
 
  S1AP_E_RAB_SETUP_REQ(message_p).ue_initial_id  = ue_desc_p->ue_initial_id;
  
  S1AP_E_RAB_SETUP_REQ(message_p).mme_ue_s1ap_id  = s1ap_E_RABSetupRequest->mme_ue_s1ap_id;
  S1AP_E_RAB_SETUP_REQ(message_p).eNB_ue_s1ap_id  = s1ap_E_RABSetupRequest->eNB_UE_S1AP_ID;
   
   S1AP_E_RAB_SETUP_REQ(message_p).nb_e_rabs_tosetup =
    s1ap_E_RABSetupRequest->e_RABToBeSetupListBearerSUReq.s1ap_E_RABToBeSetupItemBearerSUReq.count;
 
  for (i = 0; i < s1ap_E_RABSetupRequest->e_RABToBeSetupListBearerSUReq.s1ap_E_RABToBeSetupItemBearerSUReq.count; i++) {
    S1ap_E_RABToBeSetupItemBearerSUReq_t *item_p;
   
    item_p = (S1ap_E_RABToBeSetupItemBearerSUReq_t *)s1ap_E_RABSetupRequest->e_RABToBeSetupListBearerSUReq.s1ap_E_RABToBeSetupItemBearerSUReq.array[i];

    S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].e_rab_id = item_p->e_RAB_ID;

    // check for the NAS PDU
    if (item_p->nAS_PDU.size > 0 ) {
      S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].nas_pdu.length = item_p->nAS_PDU.size;

      S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].nas_pdu.buffer = malloc(sizeof(uint8_t) * item_p->nAS_PDU.size);

      memcpy(S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].nas_pdu.buffer,
             item_p->nAS_PDU.buf, item_p->nAS_PDU.size); 
      // S1AP_INFO("received a NAS PDU with size %d (%02x.%02x)\n",S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].nas_pdu.length, item_p->nAS_PDU.buf[0], item_p->nAS_PDU.buf[1]);
    } else {
      S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].nas_pdu.length = 0;
      S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].nas_pdu.buffer = NULL;
      
      S1AP_WARN("NAS PDU is not provided, generate a E_RAB_SETUP Failure (TBD) back to MME \n");
      // return -1;
    }

    /* Set the transport layer address */
    memcpy(S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].sgw_addr.buffer,
           item_p->transportLayerAddress.buf, item_p->transportLayerAddress.size);
    S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].sgw_addr.length =
      item_p->transportLayerAddress.size * 8 - item_p->transportLayerAddress.bits_unused;

    /* S1AP_INFO("sgw addr %s  len: %d (size %d, index %d)\n", 
	      S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].sgw_addr.buffer,
	      S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].sgw_addr.length,
	      item_p->transportLayerAddress.size, i);
    */
    /* GTP tunnel endpoint ID */
    OCTET_STRING_TO_INT32(&item_p->gTP_TEID, S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].gtp_teid);

    /* Set the QOS informations */
    S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].qos.qci = item_p->e_RABlevelQoSParameters.qCI;

    S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].qos.allocation_retention_priority.priority_level =
      item_p->e_RABlevelQoSParameters.allocationRetentionPriority.priorityLevel;
    S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].qos.allocation_retention_priority.pre_emp_capability =
      item_p->e_RABlevelQoSParameters.allocationRetentionPriority.pre_emptionCapability;
    S1AP_E_RAB_SETUP_REQ(message_p).e_rab_setup_params[i].qos.allocation_retention_priority.pre_emp_vulnerability =
      item_p->e_RABlevelQoSParameters.allocationRetentionPriority.pre_emptionVulnerability;
  }

  itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);

  return 0;
}

static
int s1ap_eNB_handle_paging(uint32_t               assoc_id,
    uint32_t               stream,
    struct s1ap_message_s *s1ap_message_p)
{
  S1ap_PagingIEs_t *paging_p;
  s1ap_eNB_mme_data_t   *mme_desc_p        = NULL;
  s1ap_eNB_instance_t   *s1ap_eNB_instance = NULL;
  MessageDef            *message_p         = NULL;

  DevAssert(s1ap_message_p != NULL);
  // received Paging Message from MME
  S1AP_DEBUG("[SCTP %d] Received Paging Message From MME\n",assoc_id);

  paging_p = &s1ap_message_p->msg.s1ap_PagingIEs;

  /* Paging procedure -> stream != 0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received Paging procedure on stream (%d)\n",
               assoc_id, stream);
    return -1;
  }

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received Paging for non "
               "existing MME context\n", assoc_id);
    return -1;
  }

  s1ap_eNB_instance = mme_desc_p->s1ap_eNB_instance;
  if (s1ap_eNB_instance == NULL) {
    S1AP_ERROR("[SCTP %d] Received Paging for non existing MME context : s1ap_eNB_instance is NULL\n",
               assoc_id);
    return -1;
  }

  message_p = itti_alloc_new_message(TASK_S1AP, S1AP_PAGING_IND);

  /* convert S1ap_PagingIEs_t to s1ap_paging_ind_t */
  /* convert UE Identity Index value */
  S1AP_PAGING_IND(message_p).ue_index_value  = BIT_STRING_to_uint32(&paging_p->ueIdentityIndexValue);
  S1AP_DEBUG("[SCTP %d] Received Paging ue_index_value (%d)\n",
            assoc_id,(uint32_t)S1AP_PAGING_IND(message_p).ue_index_value);

  S1AP_PAGING_IND(message_p).ue_paging_identity.choice.s_tmsi.mme_code = 0;
  S1AP_PAGING_IND(message_p).ue_paging_identity.choice.s_tmsi.m_tmsi = 0;

  /* convert UE Paging Identity */
  if (paging_p->uePagingID.present == S1ap_UEPagingID_PR_s_TMSI) {
      S1AP_PAGING_IND(message_p).ue_paging_identity.presenceMask = UE_PAGING_IDENTITY_s_tmsi;
      OCTET_STRING_TO_INT8(&paging_p->uePagingID.choice.s_TMSI.mMEC, S1AP_PAGING_IND(message_p).ue_paging_identity.choice.s_tmsi.mme_code);
      OCTET_STRING_TO_INT32(&paging_p->uePagingID.choice.s_TMSI.m_TMSI, S1AP_PAGING_IND(message_p).ue_paging_identity.choice.s_tmsi.m_tmsi);
  } else if (paging_p->uePagingID.present == S1ap_UEPagingID_PR_iMSI) {
      S1AP_PAGING_IND(message_p).ue_paging_identity.presenceMask = UE_PAGING_IDENTITY_imsi;
      S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length = 0;
      for (int i = 0; i < paging_p->uePagingID.choice.iMSI.size; i++) {
          S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2*i] = (uint8_t)(paging_p->uePagingID.choice.iMSI.buf[i] & 0x0F );
          S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length++;
          S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2*i+1] = (uint8_t)((paging_p->uePagingID.choice.iMSI.buf[i]>>4) & 0x0F);
          LOG_D(S1AP,"paging : i %d %d imsi %d %d \n",2*i,2*i+1,S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2*i], S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2*i+1]);
          if (S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2*i+1] == 0x0F) {
              if(i != paging_p->uePagingID.choice.iMSI.size - 1){
                  /* invalid paging_p->uePagingID.choise.iMSI.buffer */
                  S1AP_ERROR("[SCTP %d] Received Paging : uePagingID.choise.iMSI error(i %d 0x0F)\n", assoc_id,i);
                  return -1;
              }
          } else {
              S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length++;
          }
      }
      if (S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length >= S1AP_IMSI_LENGTH) {
          /* invalid paging_p->uePagingID.choise.iMSI.size */
          S1AP_ERROR("[SCTP %d] Received Paging : uePagingID.choise.iMSI.size(%d) is over IMSI length(%d)\n", assoc_id, S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length, S1AP_IMSI_LENGTH);
          return -1;
      }  
} else {
      /* invalid paging_p->uePagingID.present */
      S1AP_ERROR("[SCTP %d] Received Paging : uePagingID.present(%d) is unknown\n", assoc_id, paging_p->uePagingID.present);
      return -1;
  }

#if 0
  /* convert Paging DRX(optional) */
  if (paging_p->presenceMask & S1AP_PAGINGIES_PAGINGDRX_PRESENT) {
      switch(paging_p->pagingDRX) {
        case S1ap_PagingDRX_v32:
          S1AP_PAGING_IND(message_p).paging_drx = PAGING_DRX_32;
         break;
        case S1ap_PagingDRX_v64:
          S1AP_PAGING_IND(message_p).paging_drx = PAGING_DRX_64;
        break;
        case S1ap_PagingDRX_v128:
          S1AP_PAGING_IND(message_p).paging_drx = PAGING_DRX_128;
        break;
        case S1ap_PagingDRX_v256:
          S1AP_PAGING_IND(message_p).paging_drx = PAGING_DRX_256;
        break;
        default:
          // when UE Paging DRX is no value
          S1AP_PAGING_IND(message_p).paging_drx = PAGING_DRX_256;
        break;
      }
  }
#endif
  S1AP_PAGING_IND(message_p).paging_drx = PAGING_DRX_256;

  /* convert cnDomain */
  if (paging_p->cnDomain == S1ap_CNDomain_ps) {
      S1AP_PAGING_IND(message_p).cn_domain = CN_DOMAIN_PS;
  } else if (paging_p->cnDomain == S1ap_CNDomain_cs) {
      S1AP_PAGING_IND(message_p).cn_domain = CN_DOMAIN_CS;
  } else {
      /* invalid paging_p->cnDomain */
      S1AP_ERROR("[SCTP %d] Received Paging : cnDomain(%ld) is unknown\n", assoc_id, paging_p->cnDomain);
      return -1;
  }

  memset (&S1AP_PAGING_IND(message_p).plmn_identity[0], 0, sizeof(plmn_identity_t)*256);
  memset (&S1AP_PAGING_IND(message_p).tac[0], 0, sizeof(int16_t)*256);
  S1AP_PAGING_IND(message_p).tai_size = 0;

  for (int i = 0; i < paging_p->taiList.s1ap_TAIItem.count; i++) {
     S1AP_INFO("[SCTP %d] Received Paging taiList: i %d, count %d\n", assoc_id, i, paging_p->taiList.s1ap_TAIItem.count);
     S1ap_TAIItem_t s1ap_TAIItem;
     memset (&s1ap_TAIItem, 0, sizeof(S1ap_TAIItem_t));

     memcpy(&s1ap_TAIItem, paging_p->taiList.s1ap_TAIItem.array[i], sizeof(S1ap_TAIItem_t));

     TBCD_TO_MCC_MNC(&s1ap_TAIItem.tAI.pLMNidentity, S1AP_PAGING_IND(message_p).plmn_identity[i].mcc,
              S1AP_PAGING_IND(message_p).plmn_identity[i].mnc,
              S1AP_PAGING_IND(message_p).plmn_identity[i].mnc_digit_length);
     OCTET_STRING_TO_INT16(&s1ap_TAIItem.tAI.tAC, S1AP_PAGING_IND(message_p).tac[i]);
     S1AP_PAGING_IND(message_p).tai_size++;
     S1AP_DEBUG("[SCTP %d] Received Paging: MCC %d, MNC %d, TAC %d\n", assoc_id, S1AP_PAGING_IND(message_p).plmn_identity[i].mcc, S1AP_PAGING_IND(message_p).plmn_identity[i].mnc, S1AP_PAGING_IND(message_p).tac[i]);
  }

#if 0
 // CSG Id(optional) List is not used
  if (paging_p->presenceMask & S1AP_PAGINGIES_CSG_IDLIST_PRESENT) {
      // TODO
  }

  /* convert pagingPriority (optional) if has value */
  if (paging_p->presenceMask & S1AP_PAGINGIES_PAGINGPRIORITY_PRESENT) {
      switch(paging_p->pagingPriority) {
      case S1ap_PagingPriority_priolevel1:
          S1AP_PAGING_IND(message_p).paging_priority = PAGING_PRIO_LEVEL1;
        break;
      case S1ap_PagingPriority_priolevel2:
          S1AP_PAGING_IND(message_p).paging_priority = PAGING_PRIO_LEVEL2;
        break;
      case S1ap_PagingPriority_priolevel3:
          S1AP_PAGING_IND(message_p).paging_priority = PAGING_PRIO_LEVEL3;
        break;
      case S1ap_PagingPriority_priolevel4:
          S1AP_PAGING_IND(message_p).paging_priority = PAGING_PRIO_LEVEL4;
        break;
      case S1ap_PagingPriority_priolevel5:
          S1AP_PAGING_IND(message_p).paging_priority = PAGING_PRIO_LEVEL5;
        break;
      case S1ap_PagingPriority_priolevel6:
          S1AP_PAGING_IND(message_p).paging_priority = PAGING_PRIO_LEVEL6;
        break;
      case S1ap_PagingPriority_priolevel7:
          S1AP_PAGING_IND(message_p).paging_priority = PAGING_PRIO_LEVEL7;
        break;
      case S1ap_PagingPriority_priolevel8:
          S1AP_PAGING_IND(message_p).paging_priority = PAGING_PRIO_LEVEL8;
        break;
      default:
        /* invalid paging_p->pagingPriority */
        S1AP_ERROR("[SCTP %d] Received paging : pagingPriority(%ld) is invalid\n", assoc_id, paging_p->pagingPriority);
        return -1;
      }
  }
#endif
  //paging parameter values
  S1AP_DEBUG("[SCTP %d] Received Paging parameters: ue_index_value %d  cn_domain %d paging_drx %d paging_priority %d\n",assoc_id,
          S1AP_PAGING_IND(message_p).ue_index_value, S1AP_PAGING_IND(message_p).cn_domain,
          S1AP_PAGING_IND(message_p).paging_drx, S1AP_PAGING_IND(message_p).paging_priority);
  S1AP_DEBUG("[SCTP %d] Received Paging parameters(ue): presenceMask %d  s_tmsi.m_tmsi %d s_tmsi.mme_code %d IMSI length %d (0-5) %d%d%d%d%d%d\n",assoc_id,
          S1AP_PAGING_IND(message_p).ue_paging_identity.presenceMask, S1AP_PAGING_IND(message_p).ue_paging_identity.choice.s_tmsi.m_tmsi,
          S1AP_PAGING_IND(message_p).ue_paging_identity.choice.s_tmsi.mme_code, S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length,
          S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[0], S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[1],
          S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2], S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[3],
          S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[4], S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[5]);

  /* send message to RRC */
  itti_send_msg_to_task(TASK_RRC_ENB, s1ap_eNB_instance->instance, message_p);
  
   return 0;
}

static
int s1ap_eNB_handle_e_rab_modify_request(uint32_t               assoc_id,
          uint32_t               stream,
          struct s1ap_message_s *s1ap_message_p) {

  int i;

  s1ap_eNB_mme_data_t   *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;
  int nb_of_e_rabs_failed = 0;

  S1ap_E_RABModifyRequestIEs_t         *s1ap_E_RABModifyRequest;
  DevAssert(s1ap_message_p != NULL);

  s1ap_E_RABModifyRequest = &s1ap_message_p->msg.s1ap_E_RABModifyRequestIEs;

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received E-RAB modify request for non "
               "existing MME context\n", assoc_id);
    return -1;
  }


  if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
                   s1ap_E_RABModifyRequest->eNB_UE_S1AP_ID)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received E-RAB modify request for non "
               "existing UE context 0x%06lx\n", assoc_id,
               s1ap_E_RABModifyRequest->eNB_UE_S1AP_ID);
    return -1;
  }

  /* E-RAB modify request = UE-related procedure -> stream != 0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
               assoc_id, stream);
    return -1;
  }

  ue_desc_p->rx_stream = stream;

  if ( ue_desc_p->mme_ue_s1ap_id != s1ap_E_RABModifyRequest->mme_ue_s1ap_id){
    S1AP_WARN("UE context mme_ue_s1ap_id is different form that of the message (%d != %ld)",
        ue_desc_p->mme_ue_s1ap_id, s1ap_E_RABModifyRequest->mme_ue_s1ap_id);
    message_p = itti_alloc_new_message (TASK_RRC_ENB, S1AP_E_RAB_MODIFY_RESP);

    S1AP_E_RAB_MODIFY_RESP (message_p).eNB_ue_s1ap_id = s1ap_E_RABModifyRequest->eNB_UE_S1AP_ID;
//        S1AP_E_RAB_MODIFY_RESP (msg_fail_p).e_rabs[S1AP_MAX_E_RAB];
    S1AP_E_RAB_MODIFY_RESP (message_p).nb_of_e_rabs = 0;

    for(nb_of_e_rabs_failed = 0; nb_of_e_rabs_failed < s1ap_E_RABModifyRequest->e_RABToBeModifiedListBearerModReq.s1ap_E_RABToBeModifiedItemBearerModReq.count; nb_of_e_rabs_failed++) {
      S1AP_E_RAB_MODIFY_RESP (message_p).e_rabs_failed[nb_of_e_rabs_failed].e_rab_id =
            ((S1ap_E_RABToBeModifiedItemBearerModReq_t *)s1ap_E_RABModifyRequest->e_RABToBeModifiedListBearerModReq.s1ap_E_RABToBeModifiedItemBearerModReq.array[nb_of_e_rabs_failed])->e_RAB_ID;
      S1AP_E_RAB_MODIFY_RESP (message_p).e_rabs_failed[nb_of_e_rabs_failed].cause = S1AP_CAUSE_RADIO_NETWORK;
      S1AP_E_RAB_MODIFY_RESP (message_p).e_rabs_failed[nb_of_e_rabs_failed].cause_value = 13;//S1ap_CauseRadioNetwork_unknown_mme_ue_s1ap_id;
    }
    S1AP_E_RAB_MODIFY_RESP (message_p).nb_of_e_rabs_failed = nb_of_e_rabs_failed;

    s1ap_eNB_e_rab_modify_resp(mme_desc_p->s1ap_eNB_instance->instance,
                               &S1AP_E_RAB_MODIFY_RESP(message_p));

    message_p = NULL;
    return -1;
  }

  message_p        = itti_alloc_new_message(TASK_S1AP, S1AP_E_RAB_MODIFY_REQ);

  S1AP_E_RAB_MODIFY_REQ(message_p).ue_initial_id  = ue_desc_p->ue_initial_id;

  S1AP_E_RAB_MODIFY_REQ(message_p).mme_ue_s1ap_id  = s1ap_E_RABModifyRequest->mme_ue_s1ap_id;
  S1AP_E_RAB_MODIFY_REQ(message_p).eNB_ue_s1ap_id  = s1ap_E_RABModifyRequest->eNB_UE_S1AP_ID;

  S1AP_E_RAB_MODIFY_REQ(message_p).nb_e_rabs_tomodify =
    s1ap_E_RABModifyRequest->e_RABToBeModifiedListBearerModReq.s1ap_E_RABToBeModifiedItemBearerModReq.count;

  for (i = 0; i < s1ap_E_RABModifyRequest->e_RABToBeModifiedListBearerModReq.s1ap_E_RABToBeModifiedItemBearerModReq.count; i++) {
    S1ap_E_RABToBeModifiedItemBearerModReq_t *item_p;

    item_p = (S1ap_E_RABToBeModifiedItemBearerModReq_t *)s1ap_E_RABModifyRequest->e_RABToBeModifiedListBearerModReq.s1ap_E_RABToBeModifiedItemBearerModReq.array[i];

    S1AP_E_RAB_MODIFY_REQ(message_p).e_rab_modify_params[i].e_rab_id = item_p->e_RAB_ID;

    // check for the NAS PDU
    if (item_p->nAS_PDU.size > 0 ) {
      S1AP_E_RAB_MODIFY_REQ(message_p).e_rab_modify_params[i].nas_pdu.length = item_p->nAS_PDU.size;

      S1AP_E_RAB_MODIFY_REQ(message_p).e_rab_modify_params[i].nas_pdu.buffer = malloc(sizeof(uint8_t) * item_p->nAS_PDU.size);

      memcpy(S1AP_E_RAB_MODIFY_REQ(message_p).e_rab_modify_params[i].nas_pdu.buffer,
             item_p->nAS_PDU.buf, item_p->nAS_PDU.size);
    } else {
      S1AP_E_RAB_MODIFY_REQ(message_p).e_rab_modify_params[i].nas_pdu.length = 0;
      S1AP_E_RAB_MODIFY_REQ(message_p).e_rab_modify_params[i].nas_pdu.buffer = NULL;
      continue;
    }

    /* Set the QOS informations */
    S1AP_E_RAB_MODIFY_REQ(message_p).e_rab_modify_params[i].qos.qci = item_p->e_RABLevelQoSParameters.qCI;

    S1AP_E_RAB_MODIFY_REQ(message_p).e_rab_modify_params[i].qos.allocation_retention_priority.priority_level =
      item_p->e_RABLevelQoSParameters.allocationRetentionPriority.priorityLevel;
    S1AP_E_RAB_MODIFY_REQ(message_p).e_rab_modify_params[i].qos.allocation_retention_priority.pre_emp_capability =
      item_p->e_RABLevelQoSParameters.allocationRetentionPriority.pre_emptionCapability;
    S1AP_E_RAB_MODIFY_REQ(message_p).e_rab_modify_params[i].qos.allocation_retention_priority.pre_emp_vulnerability =
      item_p->e_RABLevelQoSParameters.allocationRetentionPriority.pre_emptionVulnerability;

  }

  itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);

  return 0;
}
// handle e-rab release command and send it to rrc_end
static
int s1ap_eNB_handle_e_rab_release_command(uint32_t               assoc_id,
                                          uint32_t               stream,
                                          struct s1ap_message_s *s1ap_message_p) {

  int i;

  s1ap_eNB_mme_data_t   *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;

  S1ap_E_RABReleaseCommandIEs_t         *s1ap_E_RABReleaseCommand;
  DevAssert(s1ap_message_p != NULL);
  s1ap_E_RABReleaseCommand = &s1ap_message_p->msg.s1ap_E_RABReleaseCommandIEs;
  
  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received E-RAB release command for non existing MME context\n", assoc_id);
    return -1;
  }
  if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
          s1ap_E_RABReleaseCommand->eNB_UE_S1AP_ID)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received E-RAB release command for non existing UE context 0x%06lx\n", assoc_id,
               s1ap_E_RABReleaseCommand->eNB_UE_S1AP_ID);
    return -1;
  }

  /* Initial context request = UE-related procedure -> stream != 0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
               assoc_id, stream);
    return -1;
  }

  ue_desc_p->rx_stream = stream;

  if ( ue_desc_p->mme_ue_s1ap_id != s1ap_E_RABReleaseCommand->mme_ue_s1ap_id){
    S1AP_WARN("UE context mme_ue_s1ap_id is different form that of the message (%d != %ld)",
          ue_desc_p->mme_ue_s1ap_id, s1ap_E_RABReleaseCommand->mme_ue_s1ap_id);
  }

  S1AP_DEBUG("[SCTP %d] Received E-RAB release command for eNB_UE_S1AP_ID %ld mme_ue_s1ap_id %ld\n",
          assoc_id, s1ap_E_RABReleaseCommand->eNB_UE_S1AP_ID, s1ap_E_RABReleaseCommand->mme_ue_s1ap_id);

  message_p        = itti_alloc_new_message(TASK_S1AP, S1AP_E_RAB_RELEASE_COMMAND);

  S1AP_E_RAB_RELEASE_COMMAND(message_p).eNB_ue_s1ap_id = s1ap_E_RABReleaseCommand->eNB_UE_S1AP_ID;
  S1AP_E_RAB_RELEASE_COMMAND(message_p).mme_ue_s1ap_id = s1ap_E_RABReleaseCommand->mme_ue_s1ap_id;
  if(s1ap_E_RABReleaseCommand->nas_pdu.size > 0 ){
    S1AP_E_RAB_RELEASE_COMMAND(message_p).nas_pdu.length = s1ap_E_RABReleaseCommand->nas_pdu.size;

    S1AP_E_RAB_RELEASE_COMMAND(message_p).nas_pdu.buffer =
      malloc(sizeof(uint8_t) * s1ap_E_RABReleaseCommand->nas_pdu.size);

    memcpy(S1AP_E_RAB_RELEASE_COMMAND(message_p).nas_pdu.buffer,
    		s1ap_E_RABReleaseCommand->nas_pdu.buf,
    		s1ap_E_RABReleaseCommand->nas_pdu.size);
  } else {
	  S1AP_E_RAB_RELEASE_COMMAND(message_p).nas_pdu.length = 0;
	  S1AP_E_RAB_RELEASE_COMMAND(message_p).nas_pdu.buffer = NULL;
  }

  S1AP_E_RAB_RELEASE_COMMAND(message_p).nb_e_rabs_torelease = s1ap_E_RABReleaseCommand->e_RABToBeReleasedList.s1ap_E_RABItem.count;
  for(i=0; i < s1ap_E_RABReleaseCommand->e_RABToBeReleasedList.s1ap_E_RABItem.count; i++){
	  S1ap_E_RABItem_t *item_p;
	  item_p = (S1ap_E_RABItem_t*)s1ap_E_RABReleaseCommand->e_RABToBeReleasedList.s1ap_E_RABItem.array[i];
	  S1AP_E_RAB_RELEASE_COMMAND(message_p).e_rab_release_params[i].e_rab_id = item_p->e_RAB_ID;
	  S1AP_DEBUG("[SCTP] Received E-RAB release command for e-rab id %ld\n", item_p->e_RAB_ID);
  }

  itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);

  return 0;
}
