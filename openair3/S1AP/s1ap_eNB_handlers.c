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
// #include "s1ap_eNB.h"
#include "s1ap_eNB_defs.h"
#include "s1ap_eNB_handlers.h"
#include "s1ap_eNB_decoder.h"
#include "s1ap_eNB_encoder.h"

#include "s1ap_eNB_itti_messaging.h"

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
                                      S1AP_S1AP_PDU_t       *pdu);
static
int s1ap_eNB_handle_s1_setup_failure(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     S1AP_S1AP_PDU_t       *pdu);

static
int s1ap_eNB_handle_error_indication(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     S1AP_S1AP_PDU_t       *pdu);

static
int s1ap_eNB_handle_initial_context_request(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu);

static
int s1ap_eNB_handle_ue_context_release_command(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu);

static
int s1ap_eNB_handle_e_rab_setup_request(uint32_t               assoc_id,
                                        uint32_t               stream,
                                        S1AP_S1AP_PDU_t       *pdu);

static
int s1ap_eNB_handle_paging(uint32_t               assoc_id,
                           uint32_t               stream,
                           S1AP_S1AP_PDU_t       *pdu);

static
int s1ap_eNB_handle_e_rab_modify_request(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu);

static
int s1ap_eNB_handle_e_rab_release_command(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu);

static
int s1ap_eNB_handle_s1_path_switch_request_ack(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu);

static
int s1ap_eNB_handle_s1_path_switch_request_failure(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu);

static
int s1ap_eNB_handle_s1_ENDC_e_rab_modification_confirm(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu);

static int s1ap_eNB_snd_s1_setup_request(
  s1ap_eNB_instance_t *instance_p,
  s1ap_eNB_mme_data_t *s1ap_mme_data_p);

int s1ap_timer_setup(
  uint32_t      interval_sec,
  uint32_t      interval_us,
  task_id_t     task_id,
  int32_t       instance,
  uint32_t      timer_kind,
  timer_type_t  type,
  void         *timer_arg,
  long         *timer_id);

/* Handlers matrix. Only eNB related procedure present here */
s1ap_message_decoded_callback messages_callback[][3] = {
  { 0, 0, 0 }, /* HandoverPreparation */
  { 0, 0, 0 }, /* HandoverResourceAllocation */
  { 0, 0, 0 }, /* HandoverNotification */
  { 0, s1ap_eNB_handle_s1_path_switch_request_ack, s1ap_eNB_handle_s1_path_switch_request_failure }, /* PathSwitchRequest */
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
  { 0, 0, 0 }, /* Kill */
  { 0, 0, 0 }, /* DownlinkUEAssociatedLPPaTransport  */
  { 0, 0, 0 }, /* UplinkUEAssociatedLPPaTransport */
  { 0, 0, 0 }, /* DownlinkNonUEAssociatedLPPaTransport */
  { 0, 0, 0 }, /* UplinkNonUEAssociatedLPPaTransport */
  { 0, 0, 0 }, /* UERadioCapabilityMatch */
  { 0, 0, 0 }, /* PWSRestartIndication */
  { 0, s1ap_eNB_handle_s1_ENDC_e_rab_modification_confirm, 0 }, /* E_RABModificationIndication */
};
char *s1ap_direction2String(int s1ap_dir) {
  static char *s1ap_direction_String[] = {
    "", /* Nothing */
    "Originating message", /* originating message */
    "Successfull outcome", /* successfull outcome */
    "UnSuccessfull outcome", /* successfull outcome */
  };
  return(s1ap_direction_String[s1ap_dir]);
}
void s1ap_handle_s1_setup_message(s1ap_eNB_mme_data_t *mme_desc_p, int sctp_shutdown) {
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
        message_p = itti_alloc_new_message(TASK_S1AP, 0, S1AP_DEREGISTERED_ENB_IND);
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
      message_p = itti_alloc_new_message(TASK_S1AP, 0, S1AP_REGISTER_ENB_CNF);
      S1AP_REGISTER_ENB_CNF(message_p).nb_mme = mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb;
      itti_send_msg_to_task(TASK_ENB_APP, mme_desc_p->s1ap_eNB_instance->instance, message_p);
    }
  }
}

int s1ap_eNB_handle_message(uint32_t assoc_id, int32_t stream,
                            const uint8_t *const data, const uint32_t data_length) {
  S1AP_S1AP_PDU_t pdu;
  int ret;
  DevAssert(data != NULL);
  memset(&pdu, 0, sizeof(pdu));

  if (s1ap_eNB_decode_pdu(&pdu, data, data_length) < 0) {
    S1AP_ERROR("Failed to decode PDU\n");
    return -1;
  }

  /* Checking procedure Code and direction of message */
  if (pdu.choice.initiatingMessage.procedureCode >= sizeof(messages_callback) / (3 * sizeof(
        s1ap_message_decoded_callback))
      || (pdu.present > S1AP_S1AP_PDU_PR_unsuccessfulOutcome)) {
    S1AP_ERROR("[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, pdu.choice.initiatingMessage.procedureCode, pdu.present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1AP_S1AP_PDU, &pdu);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for eNB (wrong direction).
   */
  if (messages_callback[pdu.choice.initiatingMessage.procedureCode][pdu.present - 1] == NULL) {
    S1AP_ERROR("[SCTP %d] No handler for procedureCode %ld in %s\n",
               assoc_id, pdu.choice.initiatingMessage.procedureCode,
               s1ap_direction2String(pdu.present - 1));
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1AP_S1AP_PDU, &pdu);
    return -1;
  }

  /* Calling the right handler */
  ret = (*messages_callback[pdu.choice.initiatingMessage.procedureCode][pdu.present - 1])
        (assoc_id, stream, &pdu);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_S1AP_S1AP_PDU, &pdu);
  return ret;
}

static
int s1ap_eNB_handle_s1_setup_failure(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     S1AP_S1AP_PDU_t       *pdu) {
  S1AP_S1SetupFailure_t      *container;
  S1AP_S1SetupFailureIEs_t   *ie;
  s1ap_eNB_mme_data_t        *mme_desc_p;
  uint32_t                   interval_sec = 0;
  uint32_t                   timer_kind = 0;
  s1ap_eNB_instance_t        *instance_p;

  DevAssert(pdu != NULL);
  container = &pdu->choice.unsuccessfulOutcome.value.choice.S1SetupFailure;

  /* S1 Setup Failure == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    S1AP_WARN("[SCTP %d] Received s1 setup failure on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received S1 setup failure for non existing "
               "MME context\n", assoc_id);
    return -1;
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_S1SetupFailureIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_Cause,true);

  if(ie == NULL) {
    return -1;
  }

  if ((ie->value.choice.Cause.present == S1AP_Cause_PR_misc) &&
      (ie->value.choice.Cause.choice.misc == S1AP_CauseMisc_unspecified)) {
    S1AP_WARN("Received s1 setup failure for MME... MME is not ready\n");
  } else {
    S1AP_ERROR("Received s1 setup failure for MME... please check your parameters\n");
  }

  if( mme_desc_p->timer_id != S1AP_TIMERID_INIT ) {
    s1ap_timer_remove( mme_desc_p->timer_id );
    mme_desc_p->timer_id = S1AP_TIMERID_INIT;
  }
  instance_p = mme_desc_p->s1ap_eNB_instance;
  if( ( instance_p->s1_setupreq_count >= mme_desc_p->s1_setupreq_cnt) ||
      ( instance_p->s1_setupreq_count == 0xffff) ) {
    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_S1SetupFailureIEs_t, ie, container,
                               S1AP_ProtocolIE_ID_id_TimeToWait, false);
    if( ie != NULL ) {
      switch(ie->value.choice.TimeToWait)
      {
        case S1AP_TimeToWait_v1s:
          interval_sec = 1;
          break;
        case S1AP_TimeToWait_v2s:
          interval_sec = 2;
          break;
        case S1AP_TimeToWait_v5s:
          interval_sec = 5;
          break;
        case S1AP_TimeToWait_v10s:
          interval_sec = 10;
          break;
        case S1AP_TimeToWait_v20s:
          interval_sec = 20;
          break;
        case S1AP_TimeToWait_v60s:
          interval_sec = 60;
          break;
        default:
          interval_sec = instance_p->s1_setupreq_wait_timer;
          break;
      }
    } else {
      interval_sec = instance_p->s1_setupreq_wait_timer;
    }
    
    timer_kind = mme_desc_p->cnx_id;
    timer_kind = timer_kind | S1AP_MMEIND;
    timer_kind = timer_kind | S1_SETREQ_WAIT;
    
    if( s1ap_timer_setup(interval_sec, 0, TASK_S1AP, instance_p->instance, timer_kind, S1AP_TIMER_ONE_SHOT,
      NULL, &mme_desc_p->timer_id) < 0 ) {
      S1AP_ERROR("Timer Start NG(S1 Setup Request) : MME=%d\n",mme_desc_p->cnx_id);
      s1ap_eNB_snd_s1_setup_request( instance_p, mme_desc_p );
    }
  } else {
    S1AP_ERROR("Retransmission count exceeded of S1 SETUP REQUEST : MME=%d\n",mme_desc_p->cnx_id);
  }
  return 0;
}

static
int s1ap_eNB_handle_s1_setup_response(uint32_t               assoc_id,
                                      uint32_t               stream,
                                      S1AP_S1AP_PDU_t       *pdu) {
  S1AP_S1SetupResponse_t    *container;
  S1AP_S1SetupResponseIEs_t *ie;
  s1ap_eNB_mme_data_t       *mme_desc_p;
  int i;
  DevAssert(pdu != NULL);
  container = &pdu->choice.successfulOutcome.value.choice.S1SetupResponse;

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

  /* Set the capacity of this MME */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_S1SetupResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_RelativeMMECapacity, true);
  if(ie == NULL) {
    return -1;
  }
  mme_desc_p->relative_mme_capacity = ie->value.choice.RelativeMMECapacity;

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_S1SetupResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_ServedGUMMEIs, true);
  if(ie == NULL) {
    return -1;
  }

  if( mme_desc_p->timer_id != S1AP_TIMERID_INIT )
  {
    s1ap_timer_remove( mme_desc_p->timer_id );
    mme_desc_p->timer_id = S1AP_TIMERID_INIT;
  }
  mme_desc_p->s1_setupreq_cnt = 0;
  mme_desc_p->sctp_req_cnt = 0;
  /* The list of served gummei can contain at most 8 elements.
   * LTE related gummei is the first element in the list, i.e with an id of 0.
   */
  S1AP_DEBUG("servedGUMMEIs.list.count %d\n", ie->value.choice.ServedGUMMEIs.list.count);
  DevAssert(ie->value.choice.ServedGUMMEIs.list.count > 0);
  DevAssert(ie->value.choice.ServedGUMMEIs.list.count <= S1AP_maxnoofRATs);

  for (i = 0; i < ie->value.choice.ServedGUMMEIs.list.count; i++) {
    S1AP_ServedGUMMEIsItem_t *gummei_item_p;
    struct served_gummei_s   *new_gummei_p;
    int j;
    gummei_item_p = ie->value.choice.ServedGUMMEIs.list.array[i];
    new_gummei_p = calloc(1, sizeof(struct served_gummei_s));
    STAILQ_INIT(&new_gummei_p->served_plmns);
    STAILQ_INIT(&new_gummei_p->served_group_ids);
    STAILQ_INIT(&new_gummei_p->mme_codes);
    S1AP_DEBUG("servedPLMNs.list.count %d\n", gummei_item_p->servedPLMNs.list.count);

    for (j = 0; j < gummei_item_p->servedPLMNs.list.count; j++) {
      S1AP_PLMNidentity_t *plmn_identity_p;
      struct plmn_identity_s *new_plmn_identity_p;
      plmn_identity_p = gummei_item_p->servedPLMNs.list.array[j];
      new_plmn_identity_p = calloc(1, sizeof(struct plmn_identity_s));
      TBCD_TO_MCC_MNC(plmn_identity_p, new_plmn_identity_p->mcc,
                      new_plmn_identity_p->mnc, new_plmn_identity_p->mnc_digit_length);
      STAILQ_INSERT_TAIL(&new_gummei_p->served_plmns, new_plmn_identity_p, next);
      new_gummei_p->nb_served_plmns++;
    }

    for (j = 0; j < gummei_item_p->servedGroupIDs.list.count; j++) {
      S1AP_MME_Group_ID_t       *mme_group_id_p;
      struct served_group_id_s *new_group_id_p;
      mme_group_id_p = gummei_item_p->servedGroupIDs.list.array[j];
      new_group_id_p = calloc(1, sizeof(struct served_group_id_s));
      OCTET_STRING_TO_INT16(mme_group_id_p, new_group_id_p->mme_group_id);
      STAILQ_INSERT_TAIL(&new_gummei_p->served_group_ids, new_group_id_p, next);
      new_gummei_p->nb_group_id++;
    }

    for (j = 0; j < gummei_item_p->servedMMECs.list.count; j++) {
      S1AP_MME_Code_t        *mme_code_p;
      struct mme_code_s *new_mme_code_p;
      mme_code_p = gummei_item_p->servedMMECs.list.array[j];
      new_mme_code_p = calloc(1, sizeof(struct mme_code_s));
      OCTET_STRING_TO_INT8(mme_code_p, new_mme_code_p->mme_code);
      STAILQ_INSERT_TAIL(&new_gummei_p->mme_codes, new_mme_code_p, next);
      new_gummei_p->nb_mme_code++;
    }

    STAILQ_INSERT_TAIL(&mme_desc_p->served_gummei, new_gummei_p, next);
  }
  
  /* Optionaly set the mme name */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_S1SetupResponseIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MMEname, false);

  if (ie) {
    mme_desc_p->mme_name = calloc(ie->value.choice.MMEname.size + 1, sizeof(char));
    memcpy(mme_desc_p->mme_name, ie->value.choice.MMEname.buf,
           ie->value.choice.MMEname.size);
    /* Convert the mme name to a printable string */
    mme_desc_p->mme_name[ie->value.choice.MMEname.size] = '\0';
  }

  /* The association is now ready as eNB and MME know parameters of each other.
   * Mark the association as UP to enable UE contexts creation.
   */
  mme_desc_p->state = S1AP_ENB_STATE_CONNECTED;
  mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb ++;
  return 0;
}


static
int s1ap_eNB_handle_error_indication(uint32_t         assoc_id,
                                     uint32_t         stream,
                                     S1AP_S1AP_PDU_t *pdu) {
  S1AP_ErrorIndication_t    *container;
  S1AP_ErrorIndicationIEs_t *ie;
  s1ap_eNB_mme_data_t        *mme_desc_p;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.ErrorIndication;

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

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ErrorIndicationIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, false);

  /* optional */
  if (ie != NULL) {
    S1AP_WARN("Received S1 Error indication MME UE S1AP ID 0x%lx\n", ie->value.choice.MME_UE_S1AP_ID);
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ErrorIndicationIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, false);

  /* optional */
  if (ie != NULL) {
    S1AP_WARN("Received S1 Error indication eNB UE S1AP ID 0x%lx\n", ie->value.choice.ENB_UE_S1AP_ID);
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ErrorIndicationIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_Cause, false);

  /* optional */
  if (ie) {
    switch(ie->value.choice.Cause.present) {
      case S1AP_Cause_PR_NOTHING:
        S1AP_WARN("Received S1 Error indication cause NOTHING\n");
        break;

      case S1AP_Cause_PR_radioNetwork:
        switch (ie->value.choice.Cause.choice.radioNetwork) {
          case S1AP_CauseRadioNetwork_unspecified:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_unspecified\n");
            break;

          case S1AP_CauseRadioNetwork_tx2relocoverall_expiry:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_tx2relocoverall_expiry\n");
            break;

          case S1AP_CauseRadioNetwork_successful_handover:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_successful_handover\n");
            break;

          case S1AP_CauseRadioNetwork_release_due_to_eutran_generated_reason:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_release_due_to_eutran_generated_reason\n");
            break;

          case S1AP_CauseRadioNetwork_handover_cancelled:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_handover_cancelled\n");
            break;

          case S1AP_CauseRadioNetwork_partial_handover:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_partial_handover\n");
            break;

          case S1AP_CauseRadioNetwork_ho_failure_in_target_EPC_eNB_or_target_system:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_ho_failure_in_target_EPC_eNB_or_target_system\n");
            break;

          case S1AP_CauseRadioNetwork_ho_target_not_allowed:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_ho_target_not_allowed\n");
            break;

          case S1AP_CauseRadioNetwork_tS1relocoverall_expiry:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_tS1relocoverall_expiry\n");
            break;

          case S1AP_CauseRadioNetwork_tS1relocprep_expiry:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_tS1relocprep_expiry\n");
            break;

          case S1AP_CauseRadioNetwork_cell_not_available:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_cell_not_available\n");
            break;

          case S1AP_CauseRadioNetwork_unknown_targetID:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_unknown_targetID\n");
            break;

          case S1AP_CauseRadioNetwork_no_radio_resources_available_in_target_cell:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_no_radio_resources_available_in_target_cell\n");
            break;

          case S1AP_CauseRadioNetwork_unknown_mme_ue_s1ap_id:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_unknown_mme_ue_s1ap_id\n");
            break;

          case S1AP_CauseRadioNetwork_unknown_enb_ue_s1ap_id:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_unknown_enb_ue_s1ap_id\n");
            break;

          case S1AP_CauseRadioNetwork_unknown_pair_ue_s1ap_id:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_unknown_pair_ue_s1ap_id\n");
            break;

          case S1AP_CauseRadioNetwork_handover_desirable_for_radio_reason:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_handover_desirable_for_radio_reason\n");
            break;

          case S1AP_CauseRadioNetwork_time_critical_handover:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_time_critical_handover\n");
            break;

          case S1AP_CauseRadioNetwork_resource_optimisation_handover:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_resource_optimisation_handover\n");
            break;

          case S1AP_CauseRadioNetwork_reduce_load_in_serving_cell:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_reduce_load_in_serving_cell\n");
            break;

          case S1AP_CauseRadioNetwork_user_inactivity:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_user_inactivity\n");
            break;

          case S1AP_CauseRadioNetwork_radio_connection_with_ue_lost:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_radio_connection_with_ue_lost\n");
            break;

          case S1AP_CauseRadioNetwork_load_balancing_tau_required:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_load_balancing_tau_required\n");
            break;

          case S1AP_CauseRadioNetwork_cs_fallback_triggered:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_cs_fallback_triggered\n");
            break;

          case S1AP_CauseRadioNetwork_ue_not_available_for_ps_service:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_ue_not_available_for_ps_service\n");
            break;

          case S1AP_CauseRadioNetwork_radio_resources_not_available:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_radio_resources_not_available\n");
            break;

          case S1AP_CauseRadioNetwork_failure_in_radio_interface_procedure:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_failure_in_radio_interface_procedure\n");
            break;

          case S1AP_CauseRadioNetwork_invalid_qos_combination:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_invals1ap_id_qos_combination\n");
            break;

          case S1AP_CauseRadioNetwork_interrat_redirection:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_interrat_redirection\n");
            break;

          case S1AP_CauseRadioNetwork_interaction_with_other_procedure:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_interaction_with_other_procedure\n");
            break;

          case S1AP_CauseRadioNetwork_unknown_E_RAB_ID:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_unknown_E_RAB_ID\n");
            break;

          case S1AP_CauseRadioNetwork_multiple_E_RAB_ID_instances:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_multiple_E_RAB_ID_instances\n");
            break;

          case S1AP_CauseRadioNetwork_encryption_and_or_integrity_protection_algorithms_not_supported:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_encryption_and_or_integrity_protection_algorithms_not_supported\n");
            break;

          case S1AP_CauseRadioNetwork_s1_intra_system_handover_triggered:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_s1_intra_system_handover_triggered\n");
            break;

          case S1AP_CauseRadioNetwork_s1_inter_system_handover_triggered:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_s1_inter_system_handover_triggered\n");
            break;

          case S1AP_CauseRadioNetwork_x2_handover_triggered:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_x2_handover_triggered\n");
            break;

          case S1AP_CauseRadioNetwork_redirection_towards_1xRTT:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_redirection_towards_1xRTT\n");
            break;

          case S1AP_CauseRadioNetwork_not_supported_QCI_value:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_not_supported_QCI_value\n");
            break;
          case S1AP_CauseRadioNetwork_invalid_CSG_Id:
            S1AP_WARN("Received S1 Error indication S1AP_CauseRadioNetwork_invals1ap_id_CSG_Id\n");
            break;

          default:
            S1AP_WARN("Received S1 Error indication cause radio network case not handled\n");
        }

        break;

      case S1AP_Cause_PR_transport:
        switch (ie->value.choice.Cause.choice.transport) {
          case S1AP_CauseTransport_transport_resource_unavailable:
            S1AP_WARN("Received S1 Error indication S1AP_CauseTransport_transport_resource_unavailable\n");
            break;

          case S1AP_CauseTransport_unspecified:
            S1AP_WARN("Received S1 Error indication S1AP_CauseTransport_unspecified\n");
            break;

          default:
            S1AP_WARN("Received S1 Error indication cause transport case not handled\n");
        }

        break;

      case S1AP_Cause_PR_nas:
        switch (ie->value.choice.Cause.choice.nas) {
          case S1AP_CauseNas_normal_release:
            S1AP_WARN("Received S1 Error indication S1AP_CauseNas_normal_release\n");
            break;

          case S1AP_CauseNas_authentication_failure:
            S1AP_WARN("Received S1 Error indication S1AP_CauseNas_authentication_failure\n");
            break;

          case S1AP_CauseNas_detach:
            S1AP_WARN("Received S1 Error indication S1AP_CauseNas_detach\n");
            break;

          case S1AP_CauseNas_unspecified:
            S1AP_WARN("Received S1 Error indication S1AP_CauseNas_unspecified\n");
            break;
          case S1AP_CauseNas_csg_subscription_expiry:
            S1AP_WARN("Received S1 Error indication S1AP_CauseNas_csg_subscription_expiry\n");
            break;

          default:
            S1AP_WARN("Received S1 Error indication cause nas case not handled\n");
        }

        break;

      case S1AP_Cause_PR_protocol:
        switch (ie->value.choice.Cause.choice.protocol) {
          case S1AP_CauseProtocol_transfer_syntax_error:
            S1AP_WARN("Received S1 Error indication S1AP_CauseProtocol_transfer_syntax_error\n");
            break;

          case S1AP_CauseProtocol_abstract_syntax_error_reject:
            S1AP_WARN("Received S1 Error indication S1AP_CauseProtocol_abstract_syntax_error_reject\n");
            break;

          case S1AP_CauseProtocol_abstract_syntax_error_ignore_and_notify:
            S1AP_WARN("Received S1 Error indication S1AP_CauseProtocol_abstract_syntax_error_ignore_and_notify\n");
            break;

          case S1AP_CauseProtocol_message_not_compatible_with_receiver_state:
            S1AP_WARN("Received S1 Error indication S1AP_CauseProtocol_message_not_compatible_with_receiver_state\n");
            break;

          case S1AP_CauseProtocol_semantic_error:
            S1AP_WARN("Received S1 Error indication S1AP_CauseProtocol_semantic_error\n");
            break;

          case S1AP_CauseProtocol_abstract_syntax_error_falsely_constructed_message:
            S1AP_WARN("Received S1 Error indication S1AP_CauseProtocol_abstract_syntax_error_falsely_constructed_message\n");
            break;

          case S1AP_CauseProtocol_unspecified:
            S1AP_WARN("Received S1 Error indication S1AP_CauseProtocol_unspecified\n");
            break;

          default:
            S1AP_WARN("Received S1 Error indication cause protocol case not handled\n");
        }

        break;

      case S1AP_Cause_PR_misc:
        switch (ie->value.choice.Cause.choice.protocol) {
          case S1AP_CauseMisc_control_processing_overload:
            S1AP_WARN("Received S1 Error indication S1AP_CauseMisc_control_processing_overload\n");
            break;

          case S1AP_CauseMisc_not_enough_user_plane_processing_resources:
            S1AP_WARN("Received S1 Error indication S1AP_CauseMisc_not_enough_user_plane_processing_resources\n");
            break;

          case S1AP_CauseMisc_hardware_failure:
            S1AP_WARN("Received S1 Error indication S1AP_CauseMisc_hardware_failure\n");
            break;

          case S1AP_CauseMisc_om_intervention:
            S1AP_WARN("Received S1 Error indication S1AP_CauseMisc_om_intervention\n");
            break;

          case S1AP_CauseMisc_unspecified:
            S1AP_WARN("Received S1 Error indication S1AP_CauseMisc_unspecified\n");
            break;

          case S1AP_CauseMisc_unknown_PLMN:
            S1AP_WARN("Received S1 Error indication S1AP_CauseMisc_unknown_PLMN\n");
            break;

          default:
            S1AP_WARN("Received S1 Error indication cause misc case not handled\n");
        }

        break;
    }
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_ErrorIndicationIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_CriticalityDiagnostics, false);

  if (ie) {
    if( ie->value.choice.CriticalityDiagnostics.procedureCode ) {
      S1AP_WARN("Received S1 Error indication CriticalityDiagnostics procedureCode = %ld\n", *ie->value.choice.CriticalityDiagnostics.procedureCode);
    }
    // TODO continue
  }

  // TODO continue
  return 0;
}


static
int s1ap_eNB_handle_initial_context_request(uint32_t   assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu) {
  int i;
  s1ap_eNB_mme_data_t   *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;
  S1AP_InitialContextSetupRequest_t    *container;
  S1AP_InitialContextSetupRequestIEs_t *ie;
  S1AP_ENB_UE_S1AP_ID_t    enb_ue_s1ap_id;
  S1AP_MME_UE_S1AP_ID_t    mme_ue_s1ap_id;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.InitialContextSetupRequest;

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received initial context setup request for non "
               "existing MME context\n", assoc_id);
    return -1;
  }

  /* id-MME-UE-S1AP-ID */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
  } else {
    return -1;
  }

  /* id-eNB-UE-S1AP-ID */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    enb_ue_s1ap_id = ie->value.choice.ENB_UE_S1AP_ID;

    if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
                     enb_ue_s1ap_id)) == NULL) {
      S1AP_ERROR("[SCTP %d] Received initial context setup request for non "
                 "existing UE context 0x%06lx\n", assoc_id,
                 enb_ue_s1ap_id);
      return -1;
    }
  } else {
    return -1;
  }

  /* Initial context request = UE-related procedure -> stream != 0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
               assoc_id, stream);
    return -1;
  }

  ue_desc_p->rx_stream = stream;
  ue_desc_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
  message_p        = itti_alloc_new_message(TASK_S1AP, 0, S1AP_INITIAL_CONTEXT_SETUP_REQ);
  S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).ue_initial_id  = ue_desc_p->ue_initial_id;
  ue_desc_p->ue_initial_id = 0;
  S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).eNB_ue_s1ap_id = ue_desc_p->eNB_ue_s1ap_id;
  S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).mme_ue_s1ap_id = ue_desc_p->mme_ue_s1ap_id;
  /* id-uEaggregateMaximumBitrate */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_uEaggregateMaximumBitrate, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    asn_INTEGER2ulong(&(ie->value.choice.UEAggregateMaximumBitrate.uEaggregateMaximumBitRateUL),
                      &(S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).ue_ambr.br_ul));
    asn_INTEGER2ulong(&(ie->value.choice.UEAggregateMaximumBitrate.uEaggregateMaximumBitRateDL),
                      &(S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).ue_ambr.br_dl));
    /* id-E-RABToBeSetupListCtxtSUReq */
  } else {
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }


  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_E_RABToBeSetupListCtxtSUReq, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).nb_of_e_rabs =
      ie->value.choice.E_RABToBeSetupListCtxtSUReq.list.count;

    for (i = 0; i < ie->value.choice.E_RABToBeSetupListCtxtSUReq.list.count; i++) {
      S1AP_E_RABToBeSetupItemCtxtSUReq_t *item_p;
      item_p = &(((S1AP_E_RABToBeSetupItemCtxtSUReqIEs_t *)ie->value.choice.E_RABToBeSetupListCtxtSUReq.list.array[i])->value.choice.E_RABToBeSetupItemCtxtSUReq);
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
    } /* for i... */
  } else {/* ie != NULL */
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  /* id-UESecurityCapabilities */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_UESecurityCapabilities, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_capabilities.encryption_algorithms =
      BIT_STRING_to_uint16(&ie->value.choice.UESecurityCapabilities.encryptionAlgorithms);
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_capabilities.integrity_algorithms =
      BIT_STRING_to_uint16(&ie->value.choice.UESecurityCapabilities.integrityProtectionAlgorithms);
  } else {/* ie != NULL */
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  /* id-SecurityKey : Copy the security key */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_SecurityKey, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    memcpy(&S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_key,
           ie->value.choice.SecurityKey.buf, ie->value.choice.SecurityKey.size);
  } else {/* ie != NULL */
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }
  
  /* id-NRUESecurityCapabilities */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_InitialContextSetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_NRUESecurityCapabilities, false);
  if (ie != NULL) {
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).nr_security_capabilities.encryption_algorithms =
      BIT_STRING_to_uint16(&ie->value.choice.NRUESecurityCapabilities.nRencryptionAlgorithms);
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).nr_security_capabilities.integrity_algorithms =
      BIT_STRING_to_uint16(&ie->value.choice.NRUESecurityCapabilities.nRintegrityProtectionAlgorithms);
  } else {
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).nr_security_capabilities.encryption_algorithms = 0;
    S1AP_INITIAL_CONTEXT_SETUP_REQ(message_p).nr_security_capabilities.integrity_algorithms = 0;
  }
  
  itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);

  return 0;
}


static
int s1ap_eNB_handle_ue_context_release_command(uint32_t   assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu) {
  s1ap_eNB_mme_data_t   *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;
  S1AP_MME_UE_S1AP_ID_t  mme_ue_s1ap_id;
  S1AP_ENB_UE_S1AP_ID_t  enb_ue_s1ap_id;
  S1AP_UEContextReleaseCommand_t     *container;
  S1AP_UEContextReleaseCommand_IEs_t *ie;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.UEContextReleaseCommand;

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received UE context release command for non "
               "existing MME context\n", assoc_id);
    return -1;
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_UEContextReleaseCommand_IEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_Cause, true);
  if( ie == NULL ) {
    S1AP_ERROR( "Mandatory Element Nothing : UEContextReleaseCommand(Cause)\n" );
    return -1;
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_UEContextReleaseCommand_IEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_UE_S1AP_IDs, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    switch (ie->value.choice.UE_S1AP_IDs.present) {
      case S1AP_UE_S1AP_IDs_PR_uE_S1AP_ID_pair:
        enb_ue_s1ap_id = ie->value.choice.UE_S1AP_IDs.choice.uE_S1AP_ID_pair.eNB_UE_S1AP_ID;
        mme_ue_s1ap_id = ie->value.choice.UE_S1AP_IDs.choice.uE_S1AP_ID_pair.mME_UE_S1AP_ID;
        MSC_LOG_RX_MESSAGE(
          MSC_S1AP_ENB,
          MSC_S1AP_MME,
          NULL,0,
          "0 UEContextRelease/%s eNB_ue_s1ap_id "S1AP_UE_ID_FMT" mme_ue_s1ap_id "S1AP_UE_ID_FMT" len %u",
          s1ap_direction2String(pdu->present - 1),
          enb_ue_s1ap_id,
          mme_ue_s1ap_id);

        if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
                         enb_ue_s1ap_id)) == NULL) {
          S1AP_ERROR("[SCTP %d] Received UE context release command for non "
                     "existing UE context 0x%06lx\n",
                     assoc_id,
                     enb_ue_s1ap_id);
          return -1;
        } else {
          MSC_LOG_TX_MESSAGE(
            MSC_S1AP_ENB,
            MSC_RRC_ENB,
            NULL,0,
            "0 S1AP_UE_CONTEXT_RELEASE_COMMAND/%d eNB_ue_s1ap_id "S1AP_UE_ID_FMT" ",
            enb_ue_s1ap_id);
          message_p    = itti_alloc_new_message(TASK_S1AP, 0, S1AP_UE_CONTEXT_RELEASE_COMMAND);

          if (ue_desc_p->mme_ue_s1ap_id == 0) { // case of Detach Request and switch off from RRC_IDLE mode
            ue_desc_p->mme_ue_s1ap_id = mme_ue_s1ap_id;
          }

          S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).eNB_ue_s1ap_id = enb_ue_s1ap_id;
          itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);
          return 0;
        }

        break;

      //#warning "TODO mapping mme_ue_s1ap_id  enb_ue_s1ap_id?"

      case S1AP_UE_S1AP_IDs_PR_mME_UE_S1AP_ID:
        mme_ue_s1ap_id = ie->value.choice.UE_S1AP_IDs.choice.mME_UE_S1AP_ID;
        
        RB_FOREACH(ue_desc_p, s1ap_ue_map, &mme_desc_p->s1ap_eNB_instance->s1ap_ue_head)
        {
          if( ue_desc_p->mme_ue_s1ap_id == mme_ue_s1ap_id )
          {
            enb_ue_s1ap_id = ue_desc_p->eNB_ue_s1ap_id;
            
            message_p = itti_alloc_new_message(TASK_S1AP, 0, S1AP_UE_CONTEXT_RELEASE_COMMAND);
            S1AP_UE_CONTEXT_RELEASE_COMMAND(message_p).eNB_ue_s1ap_id = enb_ue_s1ap_id;
            itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);
            
            return 0;
          }
        }
        S1AP_ERROR("[SCTP %d] Received UE context release command(mME_UE_S1AP_ID) for non "
                   "existing UE context 0x%06lx\n",
                   assoc_id,
                   mme_ue_s1ap_id);
        return -1;

      case S1AP_UE_S1AP_IDs_PR_NOTHING:
      default:
        S1AP_ERROR("S1AP_UE_CONTEXT_RELEASE_COMMAND not processed, missing info elements");
        return -1;
    }
  } else {
    S1AP_ERROR( "Mandatory Element Nothing : UEContextReleaseCommand(UE_S1AP_IDs)\n" );
    return -1;
  }

}

static
int s1ap_eNB_handle_e_rab_setup_request(uint32_t         assoc_id,
                                        uint32_t         stream,
                                        S1AP_S1AP_PDU_t *pdu) {
  int i;
  S1AP_MME_UE_S1AP_ID_t         mme_ue_s1ap_id;
  S1AP_ENB_UE_S1AP_ID_t         enb_ue_s1ap_id;
  s1ap_eNB_mme_data_t          *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t        *ue_desc_p        = NULL;
  MessageDef                   *message_p        = NULL;
  S1AP_E_RABSetupRequest_t     *container;
  S1AP_E_RABSetupRequestIEs_t  *ie;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.E_RABSetupRequest;

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received E-RAB setup request for non "
               "existing MME context\n", assoc_id);
    return -1;
  }

  /* id-MME-UE-S1AP-ID */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABSetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
  } else {
    return -1;
  }

  /* id-eNB-UE-S1AP-ID */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABSetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    enb_ue_s1ap_id = ie->value.choice.ENB_UE_S1AP_ID;
  } else {
    return -1;
  }

  if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
                   enb_ue_s1ap_id)) == NULL) {
   S1AP_ERROR("[SCTP %d] Received E-RAB setup request for non "
               "existing UE context 0x%06lx\n", assoc_id,
               enb_ue_s1ap_id);
    return -1;
  }

  /* Initial context request = UE-related procedure -> stream != 0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
               assoc_id, stream);
    return -1;
  }

  ue_desc_p->rx_stream = stream;

  if ( ue_desc_p->mme_ue_s1ap_id != mme_ue_s1ap_id) {
    S1AP_WARN("UE context mme_ue_s1ap_id is different form that of the message (%d != %ld)",
              ue_desc_p->mme_ue_s1ap_id, mme_ue_s1ap_id);
  }

  message_p        = itti_alloc_new_message(TASK_S1AP, 0, S1AP_E_RAB_SETUP_REQ);
  S1AP_E_RAB_SETUP_REQ(message_p).ue_initial_id  = ue_desc_p->ue_initial_id;
  S1AP_E_RAB_SETUP_REQ(message_p).mme_ue_s1ap_id  = mme_ue_s1ap_id;
  S1AP_E_RAB_SETUP_REQ(message_p).eNB_ue_s1ap_id  = enb_ue_s1ap_id;
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABSetupRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_E_RABToBeSetupListBearerSUReq, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    S1AP_E_RAB_SETUP_REQ(message_p).nb_e_rabs_tosetup =
      ie->value.choice.E_RABToBeSetupListBearerSUReq.list.count;

    for (i = 0; i < ie->value.choice.E_RABToBeSetupListBearerSUReq.list.count; i++) {
      S1AP_E_RABToBeSetupItemBearerSUReq_t *item_p;
      item_p = &(((S1AP_E_RABToBeSetupItemBearerSUReqIEs_t *)ie->value.choice.E_RABToBeSetupListBearerSUReq.list.array[i])->value.choice.E_RABToBeSetupItemBearerSUReq);
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
    } /* for i... */

    itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);
  } else {
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  return 0;
}

static
int s1ap_eNB_handle_paging(uint32_t               assoc_id,
                           uint32_t               stream,
                           S1AP_S1AP_PDU_t       *pdu) {
  s1ap_eNB_mme_data_t   *mme_desc_p        = NULL;
  s1ap_eNB_instance_t   *s1ap_eNB_instance = NULL;
  MessageDef            *message_p         = NULL;
  S1AP_Paging_t         *container;
  S1AP_PagingIEs_t      *ie;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.Paging;
  // received Paging Message from MME
  S1AP_DEBUG("[SCTP %d] Received Paging Message From MME\n",assoc_id);

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

  message_p = itti_alloc_new_message(TASK_S1AP, 0, S1AP_PAGING_IND);
  /* convert S1AP_PagingIEs_t to s1ap_paging_ind_t */
  /* id-UEIdentityIndexValue : convert UE Identity Index value */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PagingIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_UEIdentityIndexValue, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    S1AP_PAGING_IND(message_p).ue_index_value  = BIT_STRING_to_uint32(&ie->value.choice.UEIdentityIndexValue);
    S1AP_DEBUG("[SCTP %d] Received Paging ue_index_value (%d)\n",
               assoc_id,(uint32_t)S1AP_PAGING_IND(message_p).ue_index_value);
    S1AP_PAGING_IND(message_p).ue_paging_identity.choice.s_tmsi.mme_code = 0;
    S1AP_PAGING_IND(message_p).ue_paging_identity.choice.s_tmsi.m_tmsi = 0;
  } else {
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  /* id-UEPagingID */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PagingIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_UEPagingID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    /* convert UE Paging Identity */
    if (ie->value.choice.UEPagingID.present == S1AP_UEPagingID_PR_s_TMSI) {
      S1AP_PAGING_IND(message_p).ue_paging_identity.presenceMask = UE_PAGING_IDENTITY_s_tmsi;
      OCTET_STRING_TO_INT8(&ie->value.choice.UEPagingID.choice.s_TMSI.mMEC, S1AP_PAGING_IND(message_p).ue_paging_identity.choice.s_tmsi.mme_code);
      OCTET_STRING_TO_INT32(&ie->value.choice.UEPagingID.choice.s_TMSI.m_TMSI, S1AP_PAGING_IND(message_p).ue_paging_identity.choice.s_tmsi.m_tmsi);
    } else if (ie->value.choice.UEPagingID.present == S1AP_UEPagingID_PR_iMSI) {
      S1AP_PAGING_IND(message_p).ue_paging_identity.presenceMask = UE_PAGING_IDENTITY_imsi;
      S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length = 0;

      for (int i = 0; i < ie->value.choice.UEPagingID.choice.iMSI.size; i++) {
        S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2*i] = (uint8_t)(ie->value.choice.UEPagingID.choice.iMSI.buf[i] & 0x0F );
        S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length++;
        S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2*i+1] = (uint8_t)((ie->value.choice.UEPagingID.choice.iMSI.buf[i]>>4) & 0x0F);
        LOG_D(S1AP,"paging : i %d %d imsi %d %d \n",2*i,2*i+1,S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2*i], S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2*i+1]);

        if (S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.buffer[2*i+1] == 0x0F) {
          if(i != ie->value.choice.UEPagingID.choice.iMSI.size - 1) {
            /* invalid paging_p->uePagingID.choise.iMSI.buffer */
            S1AP_ERROR("[SCTP %d] Received Paging : uePagingID.choise.iMSI error(i %d 0x0F)\n", assoc_id,i);
            itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
            return -1;
          }
        } else {
          S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length++;
        }
      } /* for i... */

      if (S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length >= S1AP_IMSI_LENGTH) {
        /* invalid paging_p->uePagingID.choise.iMSI.size */
        S1AP_ERROR("[SCTP %d] Received Paging : uePagingID.choise.iMSI.size(%d) is over IMSI length(%d)\n", assoc_id, S1AP_PAGING_IND(message_p).ue_paging_identity.choice.imsi.length, S1AP_IMSI_LENGTH);
        itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
        return -1;
      }
    } else { /* of if (ie->value.choice.UEPagingID.present == S1AP_UEPagingID_PR_iMSI) */
      /* invalid paging_p->uePagingID.present */
      S1AP_ERROR("[SCTP %d] Received Paging : uePagingID.present(%d) is unknown\n", assoc_id, ie->value.choice.UEPagingID.present);
      itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
      return -1;
    }
  } else { /* of ie != NULL */
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  S1AP_PAGING_IND(message_p).paging_drx = PAGING_DRX_256;
  /* id-pagingDRX */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PagingIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_pagingDRX, false);

  /* optional */
  if (ie) {
    S1AP_PAGING_IND(message_p).paging_drx = ie->value.choice.PagingDRX;
  } else {
    S1AP_PAGING_IND(message_p).paging_drx = PAGING_DRX_256;
  }

  /* */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PagingIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_CNDomain, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    /* id-CNDomain : convert cnDomain */
    if (ie->value.choice.CNDomain == S1AP_CNDomain_ps) {
      S1AP_PAGING_IND(message_p).cn_domain = CN_DOMAIN_PS;
    } else if (ie->value.choice.CNDomain == S1AP_CNDomain_cs) {
      S1AP_PAGING_IND(message_p).cn_domain = CN_DOMAIN_CS;
    } else {
      /* invalid paging_p->cnDomain */
      S1AP_ERROR("[SCTP %d] Received Paging : cnDomain(%ld) is unknown\n", assoc_id, ie->value.choice.CNDomain);
      itti_free (ITTI_MSG_ORIGIN_ID(message_p), message_p);
      return -1;
    }
  } else {
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  memset (&S1AP_PAGING_IND(message_p).plmn_identity[0], 0, sizeof(plmn_identity_t)*256);
  memset (&S1AP_PAGING_IND(message_p).tac[0], 0, sizeof(int16_t)*256);
  S1AP_PAGING_IND(message_p).tai_size = 0;
  /* id-TAIList */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PagingIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_TAIList, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    S1AP_INFO("[SCTP %d] Received Paging taiList: count %d\n", assoc_id, ie->value.choice.TAIList.list.count);

    for (int i = 0; i < ie->value.choice.TAIList.list.count; i++) {
      S1AP_TAIItem_t *item_p;
      item_p = &(((S1AP_TAIItemIEs_t *)ie->value.choice.TAIList.list.array[i])->value.choice.TAIItem);
      TBCD_TO_MCC_MNC(&(item_p->tAI.pLMNidentity), S1AP_PAGING_IND(message_p).plmn_identity[i].mcc,
                      S1AP_PAGING_IND(message_p).plmn_identity[i].mnc,
                      S1AP_PAGING_IND(message_p).plmn_identity[i].mnc_digit_length);
      OCTET_STRING_TO_INT16(&(item_p->tAI.tAC), S1AP_PAGING_IND(message_p).tac[i]);
      S1AP_PAGING_IND(message_p).tai_size++;
      S1AP_DEBUG("[SCTP %d] Received Paging: MCC %d, MNC %d, TAC %d\n", assoc_id,
                 S1AP_PAGING_IND(message_p).plmn_identity[i].mcc,
                 S1AP_PAGING_IND(message_p).plmn_identity[i].mnc,
                 S1AP_PAGING_IND(message_p).tac[i]);
    }
  } else {
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

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
    S1AP_S1AP_PDU_t       *pdu) {
  int i, nb_of_e_rabs_failed;
  s1ap_eNB_mme_data_t           *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t         *ue_desc_p        = NULL;
  MessageDef                    *message_p        = NULL;
  S1AP_E_RABModifyRequest_t     *container;
  S1AP_E_RABModifyRequestIEs_t  *ie;
  S1AP_ENB_UE_S1AP_ID_t         enb_ue_s1ap_id;
  S1AP_MME_UE_S1AP_ID_t         mme_ue_s1ap_id;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.E_RABModifyRequest;

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received E-RAB modify request for non "
               "existing MME context\n", assoc_id);
    return -1;
  }

  /* id-MME-UE-S1AP-ID */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModifyRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
  } else {
    return -1;
  }

  /* id-eNB-UE-S1AP-ID */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModifyRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    enb_ue_s1ap_id = ie->value.choice.ENB_UE_S1AP_ID;
  } else {
    return -1;
  }

  if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
                   enb_ue_s1ap_id)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received E-RAB modify request for non "
               "existing UE context 0x%06lx\n", assoc_id,
               enb_ue_s1ap_id);
    return -1;
  }

  /* E-RAB modify request = UE-related procedure -> stream != 0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
               assoc_id, stream);
    return -1;
  }

  ue_desc_p->rx_stream = stream;

  if (ue_desc_p->mme_ue_s1ap_id != mme_ue_s1ap_id) {
    S1AP_WARN("UE context mme_ue_s1ap_id is different form that of the message (%d != %ld)",
              ue_desc_p->mme_ue_s1ap_id, mme_ue_s1ap_id);
    message_p = itti_alloc_new_message (TASK_RRC_ENB, 0, S1AP_E_RAB_MODIFY_RESP);
    S1AP_E_RAB_MODIFY_RESP (message_p).eNB_ue_s1ap_id = enb_ue_s1ap_id;
    S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModifyRequestIEs_t, ie, container,
                               S1AP_ProtocolIE_ID_id_E_RABToBeModifiedListBearerModReq, true);

    if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
      for(nb_of_e_rabs_failed = 0; nb_of_e_rabs_failed < ie->value.choice.E_RABToBeModifiedListBearerModReq.list.count; nb_of_e_rabs_failed++) {
        S1AP_E_RABToBeModifiedItemBearerModReq_t *item_p;
        item_p = &(((S1AP_E_RABToBeModifiedItemBearerModReqIEs_t *)
                    ie->value.choice.E_RABToBeModifiedListBearerModReq.list.array[nb_of_e_rabs_failed])->value.choice.E_RABToBeModifiedItemBearerModReq);
        S1AP_E_RAB_MODIFY_RESP(message_p).e_rabs_failed[nb_of_e_rabs_failed].e_rab_id = item_p->e_RAB_ID;
        S1AP_E_RAB_MODIFY_RESP(message_p).e_rabs_failed[nb_of_e_rabs_failed].cause = S1AP_Cause_PR_radioNetwork;
        S1AP_E_RAB_MODIFY_RESP(message_p).e_rabs_failed[nb_of_e_rabs_failed].cause_value = S1AP_CauseRadioNetwork_unknown_mme_ue_s1ap_id;
      }
    } else {
      itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
      return -1;
    }

    S1AP_E_RAB_MODIFY_RESP(message_p).nb_of_e_rabs_failed = nb_of_e_rabs_failed;
    s1ap_eNB_e_rab_modify_resp(mme_desc_p->s1ap_eNB_instance->instance,
                               &S1AP_E_RAB_MODIFY_RESP(message_p));
    itti_free(TASK_RRC_ENB,message_p);
    message_p = NULL;
    return -1;
  }

  message_p        = itti_alloc_new_message(TASK_S1AP, 0, S1AP_E_RAB_MODIFY_REQ);
  S1AP_E_RAB_MODIFY_REQ(message_p).ue_initial_id  = ue_desc_p->ue_initial_id;
  S1AP_E_RAB_MODIFY_REQ(message_p).mme_ue_s1ap_id  = mme_ue_s1ap_id;
  S1AP_E_RAB_MODIFY_REQ(message_p).eNB_ue_s1ap_id  = enb_ue_s1ap_id;
  /* id-E-RABToBeModifiedListBearerModReq */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABModifyRequestIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_E_RABToBeModifiedListBearerModReq, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    S1AP_E_RAB_MODIFY_REQ(message_p).nb_e_rabs_tomodify =
      ie->value.choice.E_RABToBeModifiedListBearerModReq.list.count;

    for (i = 0; i < ie->value.choice.E_RABToBeModifiedListBearerModReq.list.count; i++) {
      S1AP_E_RABToBeModifiedItemBearerModReq_t *item_p;
      item_p = &(((S1AP_E_RABToBeModifiedItemBearerModReqIEs_t *)ie->value.choice.E_RABToBeModifiedListBearerModReq.list.array[i])->value.choice.E_RABToBeModifiedItemBearerModReq);
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
  } else { /* of if (ie != NULL)*/
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  return 0;
}
// handle e-rab release command and send it to rrc_end
static
int s1ap_eNB_handle_e_rab_release_command(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu) {
  int i;
  s1ap_eNB_mme_data_t   *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;
  S1AP_E_RABReleaseCommand_t     *container;
  S1AP_E_RABReleaseCommandIEs_t  *ie;
  S1AP_ENB_UE_S1AP_ID_t           enb_ue_s1ap_id;
  S1AP_MME_UE_S1AP_ID_t           mme_ue_s1ap_id;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage.value.choice.E_RABReleaseCommand;

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received E-RAB release command for non existing MME context\n", assoc_id);
    return -1;
  }


  /* id-MME-UE-S1AP-ID */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABReleaseCommandIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;
  } else {
    return -1;
  }

  /* id-eNB-UE-S1AP-ID */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABReleaseCommandIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    enb_ue_s1ap_id = ie->value.choice.ENB_UE_S1AP_ID;
  } else {
    return -1;
  }

  if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
                   enb_ue_s1ap_id)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received E-RAB release command for non existing UE context 0x%06lx\n", assoc_id,
               ie->value.choice.ENB_UE_S1AP_ID);
    return -1;
  }

  /* Initial context request = UE-related procedure -> stream != 0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
               assoc_id, stream);
    return -1;
  }

  ue_desc_p->rx_stream = stream;

  if (ue_desc_p->mme_ue_s1ap_id != mme_ue_s1ap_id) {
    S1AP_WARN("UE context mme_ue_s1ap_id is different form that of the message (%d != %ld)",
              ue_desc_p->mme_ue_s1ap_id, mme_ue_s1ap_id);
  }

  S1AP_DEBUG("[SCTP %d] Received E-RAB release command for eNB_UE_S1AP_ID %ld mme_ue_s1ap_id %ld\n",
             assoc_id, enb_ue_s1ap_id, mme_ue_s1ap_id);
  message_p = itti_alloc_new_message(TASK_S1AP, 0, S1AP_E_RAB_RELEASE_COMMAND);
  S1AP_E_RAB_RELEASE_COMMAND(message_p).eNB_ue_s1ap_id = enb_ue_s1ap_id;
  S1AP_E_RAB_RELEASE_COMMAND(message_p).mme_ue_s1ap_id = mme_ue_s1ap_id;
  /* id-E-RABToBeReleasedList */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABReleaseCommandIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_E_RABToBeReleasedList, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    S1AP_E_RAB_RELEASE_COMMAND(message_p).nb_e_rabs_torelease = ie->value.choice.E_RABList.list.count;

    for (i = 0; i < ie->value.choice.E_RABList.list.count; i++) {
      S1AP_E_RABItem_t *item_p;
      item_p = &(((S1AP_E_RABItemIEs_t *)ie->value.choice.E_RABList.list.array[i])->value.choice.E_RABItem);
      S1AP_E_RAB_RELEASE_COMMAND(message_p).e_rab_release_params[i].e_rab_id = item_p->e_RAB_ID;
      S1AP_DEBUG("[SCTP] Received E-RAB release command for e-rab id %ld\n", item_p->e_RAB_ID);
    }
  } else {
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  /* id-NAS-PDU */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_E_RABReleaseCommandIEs_t, ie, container,
                             S1AP_ProtocolIE_ID_id_NAS_PDU, false);

  if(ie && ie->value.choice.NAS_PDU.size > 0) {
    S1AP_E_RAB_RELEASE_COMMAND(message_p).nas_pdu.length = ie->value.choice.NAS_PDU.size;
    S1AP_E_RAB_RELEASE_COMMAND(message_p).nas_pdu.buffer =
      malloc(sizeof(uint8_t) * ie->value.choice.NAS_PDU.size);
    memcpy(S1AP_E_RAB_RELEASE_COMMAND(message_p).nas_pdu.buffer,
           ie->value.choice.NAS_PDU.buf,
           ie->value.choice.NAS_PDU.size);
  } else {
    S1AP_E_RAB_RELEASE_COMMAND(message_p).nas_pdu.length = 0;
    S1AP_E_RAB_RELEASE_COMMAND(message_p).nas_pdu.buffer = NULL;
  }
  itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);
  return 0;
}

static
int s1ap_eNB_handle_s1_path_switch_request_ack(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu) {
  s1ap_eNB_mme_data_t   *mme_desc_p       = NULL;
  s1ap_eNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;
  S1AP_PathSwitchRequestAcknowledge_t *pathSwitchRequestAcknowledge;
  S1AP_PathSwitchRequestAcknowledgeIEs_t *ie;
  S1AP_E_RABToBeSwitchedULItemIEs_t *s1ap_E_RABToBeSwitchedULItemIEs;
  S1AP_E_RABToBeSwitchedULItem_t *s1ap_E_RABToBeSwitchedULItem;
  S1AP_E_RABItemIEs_t  *e_RABItemIEs;
  S1AP_E_RABItem_t     *e_RABItem;
  DevAssert(pdu != NULL);
  pathSwitchRequestAcknowledge = &pdu->choice.successfulOutcome.value.choice.PathSwitchRequestAcknowledge;

  /* Path Switch request == UE-related procedure -> stream !=0 */
  if (stream == 0) {
    S1AP_ERROR("[SCTP %d] Received s1 path switch request ack on stream (%d)\n",
               assoc_id, stream);
  }

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received S1 path switch request ack for non existing "
               "MME context\n", assoc_id);
    return -1;
  }

  // send a message to RRC
  message_p        = itti_alloc_new_message(TASK_S1AP, 0, S1AP_PATH_SWITCH_REQ_ACK);
  /* mandatory */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestAcknowledgeIEs_t, ie, pathSwitchRequestAcknowledge,
                             S1AP_ProtocolIE_ID_id_eNB_UE_S1AP_ID, true);
  if (ie == NULL) {
    S1AP_ERROR("[SCTP %d] Received path switch request ack for non "
               "ie context is NULL\n", assoc_id);
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  S1AP_PATH_SWITCH_REQ_ACK(message_p).eNB_ue_s1ap_id = ie->value.choice.ENB_UE_S1AP_ID;

  if ((ue_desc_p = s1ap_eNB_get_ue_context(mme_desc_p->s1ap_eNB_instance,
                   ie->value.choice.ENB_UE_S1AP_ID)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received path switch request ack for non "
               "existing UE context 0x%06lx\n", assoc_id,
               ie->value.choice.ENB_UE_S1AP_ID);
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  S1AP_PATH_SWITCH_REQ_ACK(message_p).ue_initial_id  = ue_desc_p->ue_initial_id;
  /* mandatory */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestAcknowledgeIEs_t, ie, pathSwitchRequestAcknowledge,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID, true);

  if (ie == NULL) {
    S1AP_ERROR("[SCTP %d] Received path switch request ack for non "
               "ie context is NULL\n", assoc_id);
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  S1AP_PATH_SWITCH_REQ_ACK(message_p).mme_ue_s1ap_id = ie->value.choice.MME_UE_S1AP_ID;

  if ( ue_desc_p->mme_ue_s1ap_id != ie->value.choice.MME_UE_S1AP_ID) {
    S1AP_WARN("UE context mme_ue_s1ap_id is different form that of the message (%d != %ld)",
              ue_desc_p->mme_ue_s1ap_id, ie->value.choice.MME_UE_S1AP_ID);
  }

  /* mandatory */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestAcknowledgeIEs_t, ie, pathSwitchRequestAcknowledge,
                             S1AP_ProtocolIE_ID_id_SecurityContext, true);

  if (ie == NULL) {
    S1AP_ERROR("[SCTP %d] Received path switch request ack for non "
               "ie context is NULL\n", assoc_id);
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  S1AP_PATH_SWITCH_REQ_ACK(message_p).next_hop_chain_count =
    ie->value.choice.SecurityContext.nextHopChainingCount;
  memcpy(&S1AP_PATH_SWITCH_REQ_ACK(message_p).next_security_key,
         ie->value.choice.SecurityContext.nextHopParameter.buf,
         ie->value.choice.SecurityContext.nextHopParameter.size);
  /* optional */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestAcknowledgeIEs_t, ie, pathSwitchRequestAcknowledge,
                             S1AP_ProtocolIE_ID_id_uEaggregateMaximumBitrate, false);

  if (ie) {
    asn_INTEGER2ulong(&ie->value.choice.UEAggregateMaximumBitrate.uEaggregateMaximumBitRateUL,
                      &S1AP_PATH_SWITCH_REQ_ACK(message_p).ue_ambr.br_ul);
    asn_INTEGER2ulong(&ie->value.choice.UEAggregateMaximumBitrate.uEaggregateMaximumBitRateDL,
                      &S1AP_PATH_SWITCH_REQ_ACK(message_p).ue_ambr.br_dl);
  } else {
    S1AP_WARN("UEAggregateMaximumBitrate not supported\n");
    S1AP_PATH_SWITCH_REQ_ACK(message_p).ue_ambr.br_ul = 0;
    S1AP_PATH_SWITCH_REQ_ACK(message_p).ue_ambr.br_dl = 0;
  }

  /* optional */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestAcknowledgeIEs_t, ie, pathSwitchRequestAcknowledge,
                             S1AP_ProtocolIE_ID_id_E_RABToBeSwitchedULList, false);

  if (ie) {
    S1AP_PATH_SWITCH_REQ_ACK(message_p).nb_e_rabs_tobeswitched = ie->value.choice.E_RABToBeSwitchedULList.list.count;

    for (int i = 0; i < ie->value.choice.E_RABToBeSwitchedULList.list.count; i++) {
      s1ap_E_RABToBeSwitchedULItemIEs = (S1AP_E_RABToBeSwitchedULItemIEs_t *)ie->value.choice.E_RABToBeSwitchedULList.list.array[i];
      s1ap_E_RABToBeSwitchedULItem = &s1ap_E_RABToBeSwitchedULItemIEs->value.choice.E_RABToBeSwitchedULItem;
      S1AP_PATH_SWITCH_REQ_ACK (message_p).e_rabs_tobeswitched[i].e_rab_id = s1ap_E_RABToBeSwitchedULItem->e_RAB_ID;
      memcpy(S1AP_PATH_SWITCH_REQ_ACK (message_p).e_rabs_tobeswitched[i].sgw_addr.buffer,
             s1ap_E_RABToBeSwitchedULItem->transportLayerAddress.buf, s1ap_E_RABToBeSwitchedULItem->transportLayerAddress.size);
      S1AP_PATH_SWITCH_REQ_ACK (message_p).e_rabs_tobeswitched[i].sgw_addr.length =
        s1ap_E_RABToBeSwitchedULItem->transportLayerAddress.size * 8 - s1ap_E_RABToBeSwitchedULItem->transportLayerAddress.bits_unused;
      OCTET_STRING_TO_INT32(&s1ap_E_RABToBeSwitchedULItem->gTP_TEID,
                            S1AP_PATH_SWITCH_REQ_ACK (message_p).e_rabs_tobeswitched[i].gtp_teid);
    }
  } else {
    S1AP_WARN("E_RABToBeSwitchedULList not supported\n");
    S1AP_PATH_SWITCH_REQ_ACK(message_p).nb_e_rabs_tobeswitched = 0;
  }

  /* optional */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestAcknowledgeIEs_t, ie, pathSwitchRequestAcknowledge,
                             S1AP_ProtocolIE_ID_id_E_RABToBeReleasedList, false);

  if (ie) {
    S1AP_PATH_SWITCH_REQ_ACK(message_p).nb_e_rabs_tobereleased = ie->value.choice.E_RABList.list.count;

    for (int i = 0; i < ie->value.choice.E_RABList.list.count; i++) {
      e_RABItemIEs = (S1AP_E_RABItemIEs_t *)ie->value.choice.E_RABList.list.array[i];
      e_RABItem =  &e_RABItemIEs->value.choice.E_RABItem;
      S1AP_PATH_SWITCH_REQ_ACK (message_p).e_rabs_tobereleased[i].e_rab_id = e_RABItem->e_RAB_ID;
    }
  } else {
    S1AP_WARN("E_RABToBeReleasedList not supported\n");
    S1AP_PATH_SWITCH_REQ_ACK(message_p).nb_e_rabs_tobereleased = 0;
  }

  /* optional */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestAcknowledgeIEs_t, ie, pathSwitchRequestAcknowledge,
                             S1AP_ProtocolIE_ID_id_CriticalityDiagnostics, false);

  if(!ie) {
    S1AP_WARN("Critical Diagnostic not supported\n");
  }

  /* optional */
  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestAcknowledgeIEs_t, ie, pathSwitchRequestAcknowledge,
                             S1AP_ProtocolIE_ID_id_MME_UE_S1AP_ID_2, false);

  if(!ie) {
    S1AP_WARN("MME_UE_S1AP_ID_2 flag not supported\n");
  }

  // TODO continue
  itti_send_msg_to_task(TASK_RRC_ENB, ue_desc_p->eNB_instance->instance, message_p);
  return 0;
}

static
int s1ap_eNB_handle_s1_path_switch_request_failure(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu) {
  s1ap_eNB_mme_data_t   *mme_desc_p       = NULL;
  S1AP_PathSwitchRequestFailure_t    *pathSwitchRequestFailure;
  S1AP_PathSwitchRequestFailureIEs_t *ie;
  DevAssert(pdu != NULL);
  pathSwitchRequestFailure = &pdu->choice.unsuccessfulOutcome.value.choice.PathSwitchRequestFailure;

  if (stream != 0) {
    S1AP_WARN("[SCTP %d] Received s1 path switch request failure on stream != 0 (%d)\n",
               assoc_id, stream);
  }

  if ((mme_desc_p = s1ap_eNB_get_MME(NULL, assoc_id, 0)) == NULL) {
    S1AP_ERROR("[SCTP %d] Received S1 path switch request failure for non existing "
               "MME context\n", assoc_id);
    return -1;
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestFailureIEs_t, ie, pathSwitchRequestFailure,
                             S1AP_ProtocolIE_ID_id_Cause, true);

  if (ie == NULL) {
    S1AP_ERROR("[SCTP %d] Received S1 path switch request failure for non existing "
               "ie context is NULL\n", assoc_id);
    return -1;
  }

  switch(ie->value.choice.Cause.present) {
    case S1AP_Cause_PR_NOTHING:
      S1AP_WARN("Received S1 Error indication cause NOTHING\n");
      break;

    case S1AP_Cause_PR_radioNetwork:
      S1AP_WARN("Radio Network Layer Cause Failure\n");
      break;

    case S1AP_Cause_PR_transport:
      S1AP_WARN("Transport Layer Cause Failure\n");
      break;

    case S1AP_Cause_PR_nas:
      S1AP_WARN("NAS Cause Failure\n");
      break;

    case S1AP_Cause_PR_misc:
      S1AP_WARN("Miscelaneous Cause Failure\n");
      break;

    default:
      S1AP_WARN("Received an unknown S1 Error indication cause\n");
      break;
  }

  S1AP_FIND_PROTOCOLIE_BY_ID(S1AP_PathSwitchRequestFailureIEs_t, ie, pathSwitchRequestFailure,
                             S1AP_ProtocolIE_ID_id_CriticalityDiagnostics, false);

  if(!ie) {
    S1AP_WARN("Critical Diagnostic not supported\n");
  }

  // TODO continue
  return 0;
}

static
int s1ap_eNB_handle_s1_ENDC_e_rab_modification_confirm(uint32_t               assoc_id,
    uint32_t               stream,
    S1AP_S1AP_PDU_t       *pdu){

	LOG_I(S1AP, "Received S1AP E-RAB Modification confirm message \n");
	return 0;
}

//-----------------------------------------------------------------------------
/*
* eNB generate a S1 setup request towards MME
*/
static int s1ap_eNB_snd_s1_setup_request(
  s1ap_eNB_instance_t *instance_p,
  s1ap_eNB_mme_data_t *s1ap_mme_data_p)
//-----------------------------------------------------------------------------
{
  S1AP_S1AP_PDU_t            pdu;
  S1AP_S1SetupRequest_t     *out = NULL;
  S1AP_S1SetupRequestIEs_t   *ie = NULL;
  S1AP_SupportedTAs_Item_t   *ta = NULL;
  S1AP_PLMNidentity_t      *plmn = NULL;
  uint8_t  *buffer = NULL;
  uint32_t  len = 0;
  int       ret = 0;
  uint32_t                 timer_kind = 0;

  DevAssert(instance_p != NULL);
  DevAssert(s1ap_mme_data_p != NULL);
  s1ap_mme_data_p->state = S1AP_ENB_STATE_WAITING;
  /* Prepare the S1AP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = S1AP_S1AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = S1AP_ProcedureCode_id_S1Setup;
  pdu.choice.initiatingMessage.criticality = S1AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = S1AP_InitiatingMessage__value_PR_S1SetupRequest;
  out = &pdu.choice.initiatingMessage.value.choice.S1SetupRequest;
  /* mandatory */
  ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_Global_ENB_ID;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_S1SetupRequestIEs__value_PR_Global_ENB_ID;
  MCC_MNC_TO_PLMNID(instance_p->mcc[s1ap_mme_data_p->broadcast_plmn_index[0]],
                    instance_p->mnc[s1ap_mme_data_p->broadcast_plmn_index[0]],
                    instance_p->mnc_digit_length[s1ap_mme_data_p->broadcast_plmn_index[0]],
                    &ie->value.choice.Global_ENB_ID.pLMNidentity);
  ie->value.choice.Global_ENB_ID.eNB_ID.present = S1AP_ENB_ID_PR_macroENB_ID;
  MACRO_ENB_ID_TO_BIT_STRING(instance_p->eNB_id,
                             &ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID);
  S1AP_INFO("%d -> %02x%02x%02x\n", instance_p->eNB_id,
            ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.buf[0],
            ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.buf[1],
            ie->value.choice.Global_ENB_ID.eNB_ID.choice.macroENB_ID.buf[2]);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  if (instance_p->eNB_name) {
    ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_eNBname;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_S1SetupRequestIEs__value_PR_ENBname;
    OCTET_STRING_fromBuf(&ie->value.choice.ENBname, instance_p->eNB_name,
                         strlen(instance_p->eNB_name));
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_SupportedTAs;
  ie->criticality = S1AP_Criticality_reject;
  ie->value.present = S1AP_S1SetupRequestIEs__value_PR_SupportedTAs;
  {
    ta = (S1AP_SupportedTAs_Item_t *)calloc(1, sizeof(S1AP_SupportedTAs_Item_t));
    INT16_TO_OCTET_STRING(instance_p->tac, &ta->tAC);
    {
      for (int i = 0; i < s1ap_mme_data_p->broadcast_plmn_num; ++i) {
        plmn = (S1AP_PLMNidentity_t *)calloc(1, sizeof(S1AP_PLMNidentity_t));
        MCC_MNC_TO_TBCD(instance_p->mcc[s1ap_mme_data_p->broadcast_plmn_index[i]],
                        instance_p->mnc[s1ap_mme_data_p->broadcast_plmn_index[i]],
                        instance_p->mnc_digit_length[s1ap_mme_data_p->broadcast_plmn_index[i]],
                        plmn);
        ASN_SEQUENCE_ADD(&ta->broadcastPLMNs.list, plmn);
      }
    }
    ASN_SEQUENCE_ADD(&ie->value.choice.SupportedTAs.list, ta);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  /* mandatory */
  ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
  ie->id = S1AP_ProtocolIE_ID_id_DefaultPagingDRX;
  ie->criticality = S1AP_Criticality_ignore;
  ie->value.present = S1AP_S1SetupRequestIEs__value_PR_PagingDRX;
  ie->value.choice.PagingDRX = instance_p->default_drx;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  if (0) {
    ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_CSG_IdList;
    ie->criticality = S1AP_Criticality_reject;
    ie->value.present = S1AP_S1SetupRequestIEs__value_PR_CSG_IdList;
    // ie->value.choice.CSG_IdList = ;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
#if (S1AP_VERSION >= MAKE_VERSION(13, 0, 0))

  if (0) {
    ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_UE_RetentionInformation;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_S1SetupRequestIEs__value_PR_UE_RetentionInformation;
    // ie->value.choice.UE_RetentionInformation = ;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* optional */
  if (0) {
    ie = (S1AP_S1SetupRequestIEs_t *)calloc(1, sizeof(S1AP_S1SetupRequestIEs_t));
    ie->id = S1AP_ProtocolIE_ID_id_NB_IoT_DefaultPagingDRX;
    ie->criticality = S1AP_Criticality_ignore;
    ie->value.present = S1AP_S1SetupRequestIEs__value_PR_NB_IoT_DefaultPagingDRX;
    // ie->value.choice.NB_IoT_DefaultPagingDRX = ;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

#endif /* #if (S1AP_VERSION >= MAKE_VERSION(14, 0, 0)) */

  if (s1ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    S1AP_ERROR("Failed to encode S1 setup request\n");
    return -1;
  }

  timer_kind = s1ap_mme_data_p->cnx_id;
  timer_kind = timer_kind | S1AP_MMEIND;
  timer_kind = timer_kind | S1_SETRSP_WAIT;
  
  if( s1ap_timer_setup(instance_p->s1_setuprsp_wait_timer, 0, TASK_S1AP, instance_p->instance, timer_kind, S1AP_TIMER_ONE_SHOT,
    NULL, &s1ap_mme_data_p->timer_id) < 0 )
  {
    S1AP_ERROR("Timer Start NG(S1 Setup Response) : MME=%d\n",s1ap_mme_data_p->cnx_id);
  }
  s1ap_mme_data_p->s1_setupreq_cnt++;

  /* Non UE-Associated signalling -> stream = 0 */
  s1ap_eNB_itti_send_sctp_data_req(instance_p->instance, s1ap_mme_data_p->assoc_id, buffer, len, 0);
  return ret;
}
