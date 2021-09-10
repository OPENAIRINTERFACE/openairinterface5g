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

/*! \file ngap_gNB_handlers.c
 * \brief ngap messages handlers for gNB part
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \date 2020
 * \version 0.1
 */
 
#include <stdint.h>

#include "intertask_interface.h"

#include "asn1_conversions.h"

#include "ngap_common.h"
// #include "ngap_gNB.h"
#include "ngap_gNB_defs.h"
#include "ngap_gNB_handlers.h"
#include "ngap_gNB_decoder.h"

#include "ngap_gNB_ue_context.h"
#include "ngap_gNB_trace.h"
#include "ngap_gNB_nas_procedures.h"
#include "ngap_gNB_management_procedures.h"

#include "ngap_gNB_default_values.h"

#include "assertions.h"
#include "conversions.h"
#include "msc.h"
#include "NGAP_NonDynamic5QIDescriptor.h"

static
int ngap_gNB_handle_ng_setup_response(uint32_t               assoc_id,
                                      uint32_t               stream,
                                      NGAP_NGAP_PDU_t       *pdu);
static
int ngap_gNB_handle_ng_setup_failure(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     NGAP_NGAP_PDU_t       *pdu);

static
int ngap_gNB_handle_error_indication(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     NGAP_NGAP_PDU_t       *pdu);

static
int ngap_gNB_handle_initial_context_request(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu);

static
int ngap_gNB_handle_ue_context_release_command(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu);

static
int ngap_gNB_handle_pdusession_setup_request(uint32_t               assoc_id,
                                        uint32_t               stream,
                                        NGAP_NGAP_PDU_t       *pdu);

static
int ngap_gNB_handle_paging(uint32_t               assoc_id,
                           uint32_t               stream,
                           NGAP_NGAP_PDU_t       *pdu);

static
int ngap_gNB_handle_pdusession_modify_request(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu);

static
int ngap_gNB_handle_pdusession_release_command(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu);

static
int ngap_gNB_handle_ng_path_switch_request_ack(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu);

static
int ngap_gNB_handle_ng_path_switch_request_failure(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu);

static
int ngap_gNB_handle_ng_ENDC_pdusession_modification_confirm(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu);

/* Handlers matrix. Only gNB related procedure present here */
ngap_message_decoded_callback ngap_messages_callback[][3] = {
  { 0, 0, 0 }, /* AMFConfigurationUpdate */
  { 0, 0, 0 }, /* AMFStatusIndication */
  { 0, 0, 0 }, /* CellTrafficTrace */
  { ngap_gNB_handle_deactivate_trace, 0, 0 }, /* DeactivateTrace */
  { ngap_gNB_handle_nas_downlink, 0, 0 }, /* DownlinkNASTransport */
  { 0, 0, 0 }, /* DownlinkNonUEAssociatedNRPPaTransport */
  { 0, 0, 0 }, /* DownlinkRANConfigurationTransfer */
  { 0, 0, 0 }, /* DownlinkRANStatusTransfer */
  { 0, 0, 0 }, /* DownlinkUEAssociatedNRPPaTransport */
  { ngap_gNB_handle_error_indication, 0, 0 }, /* ErrorIndication */
  { 0, 0, 0 }, /* HandoverCancel */
  { 0, 0, 0 }, /* HandoverNotification */
  { 0, 0, 0 }, /* HandoverPreparation */
  { 0, 0, 0 }, /* HandoverResourceAllocation */
  { ngap_gNB_handle_initial_context_request, 0, 0 }, /* InitialContextSetup */
  { 0, 0, 0 }, /* InitialUEMessage */
  { 0, 0, 0 }, /* LocationReportingControl */
  { 0, 0, 0 }, /* LocationReportingFailureIndication */
  { 0, 0, 0 }, /* LocationReport */
  { 0, 0, 0 }, /* NASNonDeliveryIndication */
  { 0, 0, 0 }, /* NGReset */
  { 0, ngap_gNB_handle_ng_setup_response, ngap_gNB_handle_ng_setup_failure }, /* NGSetup */
  { 0, 0, 0 }, /* OverloadStart */
  { 0, 0, 0 }, /* OverloadStop */
  { ngap_gNB_handle_paging, 0, 0 }, /* Paging */
  { 0, ngap_gNB_handle_ng_path_switch_request_ack, ngap_gNB_handle_ng_path_switch_request_failure }, /* PathSwitchRequest */
  { ngap_gNB_handle_pdusession_modify_request, 0, 0 }, /* PDUSessionResourceModify */
  { 0, ngap_gNB_handle_ng_ENDC_pdusession_modification_confirm, 0 }, /* PDUSessionResourceModifyIndication */
  { ngap_gNB_handle_pdusession_release_command, 0, 0 }, /* PDUSessionResourceRelease */
  { ngap_gNB_handle_pdusession_setup_request, 0, 0 }, /* PDUSessionResourceSetup */
  { 0, 0, 0 }, /* PDUSessionResourceNotify */
  { 0, 0, 0 }, /* PrivateMessage */
  { 0, 0, 0 }, /* PWSCancel */
  { 0, 0, 0 }, /* PWSFailureIndication */
  { 0, 0, 0 }, /* PWSRestartIndication */
  { 0, 0, 0 }, /* RANConfigurationUpdate */
  { 0, 0, 0 }, /* RerouteNASRequest */
  { 0, 0, 0 }, /* RRCInactiveTransitionReport */
  { 0, 0, 0 }, /* TraceFailureIndication */
  { ngap_gNB_handle_trace_start, 0, 0 }, /* TraceStart */
  { 0, 0, 0 }, /* UEContextModification */
  { ngap_gNB_handle_ue_context_release_command, 0, 0 }, /* UEContextRelease */
  { 0, 0, 0 }, /* UEContextReleaseRequest */
  { 0, 0, 0 }, /* UERadioCapabilityCheck */
  { 0, 0, 0 }, /* UERadioCapabilityInfoIndication */
  { 0, 0, 0 }, /* UETNLABindingRelease */
  { 0, 0, 0 }, /* UplinkNASTransport */
  { 0, 0, 0 }, /* UplinkNonUEAssociatedNRPPaTransport */
  { 0, 0, 0 }, /* UplinkRANConfigurationTransfer */
  { 0, 0, 0 }, /* UplinkRANStatusTransfer */
  { 0, 0, 0 }, /* UplinkUEAssociatedNRPPaTransport */
  { 0, 0, 0 }, /* WriteReplaceWarning */
  { 0, 0, 0 }, /* SecondaryRATDataUsageReport */

};
char *ngap_direction2String(int ngap_dir) {
  static char *ngap_direction_String[] = {
    "", /* Nothing */
    "Originating message", /* originating message */
    "Successfull outcome", /* successfull outcome */
    "UnSuccessfull outcome", /* successfull outcome */
  };
  return(ngap_direction_String[ngap_dir]);
}
void ngap_handle_ng_setup_message(ngap_gNB_amf_data_t *amf_desc_p, int sctp_shutdown) {
  if (sctp_shutdown) {
    /* A previously connected AMF has been shutdown */

    /* TODO check if it was used by some gNB and send a message to inform these gNB if there is no more associated AMF */
    if (amf_desc_p->state == NGAP_GNB_STATE_CONNECTED) {
      amf_desc_p->state = NGAP_GNB_STATE_DISCONNECTED;

      if (amf_desc_p->ngap_gNB_instance->ngap_amf_associated_nb > 0) {
        /* Decrease associated AMF number */
        amf_desc_p->ngap_gNB_instance->ngap_amf_associated_nb --;
      }

      /* If there are no more associated AMF, inform gNB app */
      if (amf_desc_p->ngap_gNB_instance->ngap_amf_associated_nb == 0) {
        MessageDef                 *message_p;
        message_p = itti_alloc_new_message(TASK_NGAP, 0, NGAP_DEREGISTERED_GNB_IND);
        NGAP_DEREGISTERED_GNB_IND(message_p).nb_amf = 0;
        itti_send_msg_to_task(TASK_GNB_APP, amf_desc_p->ngap_gNB_instance->instance, message_p);
      }
    }
  } else {
    /* Check that at least one setup message is pending */
    DevCheck(amf_desc_p->ngap_gNB_instance->ngap_amf_pending_nb > 0, amf_desc_p->ngap_gNB_instance->instance,
             amf_desc_p->ngap_gNB_instance->ngap_amf_pending_nb, 0);

    if (amf_desc_p->ngap_gNB_instance->ngap_amf_pending_nb > 0) {
      /* Decrease pending messages number */
      amf_desc_p->ngap_gNB_instance->ngap_amf_pending_nb --;
    }

    /* If there are no more pending messages, inform gNB app */
    if (amf_desc_p->ngap_gNB_instance->ngap_amf_pending_nb == 0) {
      MessageDef                 *message_p;
      message_p = itti_alloc_new_message(TASK_NGAP, 0, NGAP_REGISTER_GNB_CNF);
      NGAP_REGISTER_GNB_CNF(message_p).nb_amf = amf_desc_p->ngap_gNB_instance->ngap_amf_associated_nb;
      itti_send_msg_to_task(TASK_GNB_APP, amf_desc_p->ngap_gNB_instance->instance, message_p);
    }
  }
}

int ngap_gNB_handle_message(uint32_t assoc_id, int32_t stream,
                            const uint8_t *const data, const uint32_t data_length) {
  NGAP_NGAP_PDU_t pdu;
  int ret;
  DevAssert(data != NULL);
  memset(&pdu, 0, sizeof(pdu));

  if (ngap_gNB_decode_pdu(&pdu, data, data_length) < 0) {
    NGAP_ERROR("Failed to decode PDU\n");
    return -1;
  }

  /* Checking procedure Code and direction of message */
  if (pdu.choice.initiatingMessage->procedureCode >= sizeof(ngap_messages_callback) / (3 * sizeof(
        ngap_message_decoded_callback))
      || (pdu.present > NGAP_NGAP_PDU_PR_unsuccessfulOutcome)) {
    NGAP_ERROR("[SCTP %d] Either procedureCode %ld or direction %d exceed expected\n",
               assoc_id, pdu.choice.initiatingMessage->procedureCode, pdu.present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
    return -1;
  }

  /* No handler present.
   * This can mean not implemented or no procedure for gNB (wrong direction).
   */
  if (ngap_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1] == NULL) {
    NGAP_ERROR("[SCTP %d] No handler for procedureCode %ld in %s\n",
               assoc_id, pdu.choice.initiatingMessage->procedureCode,
               ngap_direction2String(pdu.present - 1));
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
    return -1;
  }

  /* Calling the right handler */
  ret = (*ngap_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1])
        (assoc_id, stream, &pdu);
  ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NGAP_NGAP_PDU, &pdu);
  return ret;
}

static
int ngap_gNB_handle_ng_setup_failure(uint32_t               assoc_id,
                                     uint32_t               stream,
                                     NGAP_NGAP_PDU_t       *pdu) {
  NGAP_NGSetupFailure_t      *container;
  NGAP_NGSetupFailureIEs_t   *ie;
  ngap_gNB_amf_data_t        *amf_desc_p;
  DevAssert(pdu != NULL);
  container = &pdu->choice.unsuccessfulOutcome->value.choice.NGSetupFailure;

  /* S1 Setup Failure == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    NGAP_WARN("[SCTP %d] Received s1 setup failure on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  if ((amf_desc_p = ngap_gNB_get_AMF(NULL, assoc_id, 0)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received S1 setup response for non existing "
               "AMF context\n", assoc_id);
    return -1;
  }

  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_NGSetupFailureIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_Cause,true);

  if ((ie->value.choice.Cause.present == NGAP_Cause_PR_misc) &&
      (ie->value.choice.Cause.choice.misc == NGAP_CauseMisc_unspecified)) {
    NGAP_WARN("Received NG setup failure for AMF... AMF is not ready\n");
  } else {
    NGAP_ERROR("Received NG setup failure for AMF... please check your parameters\n");
  }

  amf_desc_p->state = NGAP_GNB_STATE_WAITING;
  ngap_handle_ng_setup_message(amf_desc_p, 0);
  return 0;
}

static
int ngap_gNB_handle_ng_setup_response(uint32_t               assoc_id,
                                      uint32_t               stream,
                                      NGAP_NGAP_PDU_t       *pdu) {
  NGAP_NGSetupResponse_t    *container;
  NGAP_NGSetupResponseIEs_t *ie;
  ngap_gNB_amf_data_t       *amf_desc_p;
  int i;
  DevAssert(pdu != NULL);
  container = &pdu->choice.successfulOutcome->value.choice.NGSetupResponse;

  /* NG Setup Response == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    NGAP_ERROR("[SCTP %d] Received ng setup response on stream != 0 (%d)\n",
               assoc_id, stream);
    return -1;
  }

  if ((amf_desc_p = ngap_gNB_get_AMF(NULL, assoc_id, 0)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received NG setup response for non existing "
               "AMF context\n", assoc_id);
    return -1;
  }

  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_NGSetupResponseIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_ServedGUAMIList, true);

  /* The list of served guami can contain at most 256 elements.
   * NR related guami is the first element in the list, i.e with an id of 0.
   */
  NGAP_DEBUG("servedGUAMIs.list.count %d\n", ie->value.choice.ServedGUAMIList.list.count);
  DevAssert(ie->value.choice.ServedGUAMIList.list.count > 0);
  DevAssert(ie->value.choice.ServedGUAMIList.list.count <= NGAP_maxnoofServedGUAMIs);

  for (i = 0; i < ie->value.choice.ServedGUAMIList.list.count; i++) {
    NGAP_ServedGUAMIItem_t  *guami_item_p;
    struct served_guami_s   *new_guami_p;

    guami_item_p = ie->value.choice.ServedGUAMIList.list.array[i];
    new_guami_p = calloc(1, sizeof(struct served_guami_s));
    STAILQ_INIT(&new_guami_p->served_plmns);
    STAILQ_INIT(&new_guami_p->served_region_ids);
    STAILQ_INIT(&new_guami_p->amf_set_ids);
    STAILQ_INIT(&new_guami_p->amf_pointers);
    
    NGAP_PLMNIdentity_t *plmn_identity_p;
    struct plmn_identity_s *new_plmn_identity_p;
    plmn_identity_p = &guami_item_p->gUAMI.pLMNIdentity;
    new_plmn_identity_p = calloc(1, sizeof(struct plmn_identity_s));
    TBCD_TO_MCC_MNC(plmn_identity_p, new_plmn_identity_p->mcc,
                    new_plmn_identity_p->mnc, new_plmn_identity_p->mnc_digit_length);
    STAILQ_INSERT_TAIL(&new_guami_p->served_plmns, new_plmn_identity_p, next);
    new_guami_p->nb_served_plmns++;
    
    NGAP_AMFRegionID_t        *amf_region_id_p;
    struct served_region_id_s *new_region_id_p;
    amf_region_id_p = &guami_item_p->gUAMI.aMFRegionID;
    new_region_id_p = calloc(1, sizeof(struct served_region_id_s));
    OCTET_STRING_TO_INT8(amf_region_id_p, new_region_id_p->amf_region_id);
    STAILQ_INSERT_TAIL(&new_guami_p->served_region_ids, new_region_id_p, next);
    new_guami_p->nb_region_id++;

    NGAP_AMFSetID_t        *amf_set_id_p;
    struct amf_set_id_s    *new_amf_set_id_p;
    amf_set_id_p = &guami_item_p->gUAMI.aMFSetID;
    new_amf_set_id_p = calloc(1, sizeof(struct amf_set_id_s));
    OCTET_STRING_TO_INT16(amf_set_id_p, new_amf_set_id_p->amf_set_id);
    STAILQ_INSERT_TAIL(&new_guami_p->amf_set_ids, new_amf_set_id_p, next);
    new_guami_p->nb_amf_set_id++;

    NGAP_AMFPointer_t        *amf_pointer_p;
    struct amf_pointer_s     *new_amf_pointer_p;
    amf_pointer_p = &guami_item_p->gUAMI.aMFPointer;
    new_amf_pointer_p = calloc(1, sizeof(struct amf_pointer_s));
    OCTET_STRING_TO_INT8(amf_pointer_p, new_amf_pointer_p->amf_pointer);
    STAILQ_INSERT_TAIL(&new_guami_p->amf_pointers, new_amf_pointer_p, next);
    new_guami_p->nb_amf_pointer++;

    STAILQ_INSERT_TAIL(&amf_desc_p->served_guami, new_guami_p, next);
  }

  /* Set the capacity of this AMF */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_NGSetupResponseIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_RelativeAMFCapacity, true);

  amf_desc_p->relative_amf_capacity = ie->value.choice.RelativeAMFCapacity;

  /* mandatory set the amf name */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_NGSetupResponseIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_AMFName, true);

  if (ie) {
    amf_desc_p->amf_name = calloc(ie->value.choice.AMFName.size + 1, sizeof(char));
    memcpy(amf_desc_p->amf_name, ie->value.choice.AMFName.buf,
           ie->value.choice.AMFName.size);
    /* Convert the amf name to a printable string */
    amf_desc_p->amf_name[ie->value.choice.AMFName.size] = '\0';
  }

  
  /* mandatory set the plmn supports */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_NGSetupResponseIEs_t, ie, container,
                               NGAP_ProtocolIE_ID_id_PLMNSupportList, true);

  NGAP_DEBUG("PLMNSupportList.list.count %d\n", ie->value.choice.PLMNSupportList.list.count);
  DevAssert(ie->value.choice.PLMNSupportList.list.count > 0);
  DevAssert(ie->value.choice.PLMNSupportList.list.count <= NGAP_maxnoofPLMNs);

  STAILQ_INIT(&amf_desc_p->plmn_supports);

  for (i = 0; i < ie->value.choice.ServedGUAMIList.list.count; i++) {
    NGAP_PLMNSupportItem_t *plmn_support_item_p;
    struct plmn_support_s  *new_plmn_support_p;
    NGAP_SliceSupportItem_t  *slice_support_item_p;
    struct slice_support_s *new_slice_support_p;

    plmn_support_item_p = ie->value.choice.PLMNSupportList.list.array[i];

    new_plmn_support_p = calloc(1, sizeof(struct plmn_support_s));
    
    TBCD_TO_MCC_MNC(&plmn_support_item_p->pLMNIdentity, new_plmn_support_p->plmn_identity.mcc,
                    new_plmn_support_p->plmn_identity.mnc, new_plmn_support_p->plmn_identity.mnc_digit_length);

    NGAP_DEBUG("PLMNSupportList.list.count %d\n", plmn_support_item_p->sliceSupportList.list.count);
    DevAssert(plmn_support_item_p->sliceSupportList.list.count > 0);
    DevAssert(plmn_support_item_p->sliceSupportList.list.count <= NGAP_maxnoofSliceItems);

    STAILQ_INIT(&new_plmn_support_p->slice_supports);
    for(int j=0; j<plmn_support_item_p->sliceSupportList.list.count; j++) {
      slice_support_item_p = plmn_support_item_p->sliceSupportList.list.array[j];
      
      new_slice_support_p = calloc(1, sizeof(struct slice_support_s));

      OCTET_STRING_TO_INT8(&slice_support_item_p->s_NSSAI.sST, new_slice_support_p->sST);

      if(slice_support_item_p->s_NSSAI.sD != NULL) {
        new_slice_support_p->sD_flag = 1;
        new_slice_support_p->sD[0] = slice_support_item_p->s_NSSAI.sD->buf[0];
        new_slice_support_p->sD[1] = slice_support_item_p->s_NSSAI.sD->buf[1];
        new_slice_support_p->sD[2] = slice_support_item_p->s_NSSAI.sD->buf[2];
      }
      STAILQ_INSERT_TAIL(&new_plmn_support_p->slice_supports, new_slice_support_p, next);
    }

    STAILQ_INSERT_TAIL(&amf_desc_p->plmn_supports, new_plmn_support_p, next);
  }

  /* The association is now ready as gNB and AMF know parameters of each other.
   * Mark the association as UP to enable UE contexts creation.
   */
  amf_desc_p->state = NGAP_GNB_STATE_CONNECTED;
  amf_desc_p->ngap_gNB_instance->ngap_amf_associated_nb ++;
  ngap_handle_ng_setup_message(amf_desc_p, 0);

  return 0;
}


static
int ngap_gNB_handle_error_indication(uint32_t         assoc_id,
                                     uint32_t         stream,
                                     NGAP_NGAP_PDU_t *pdu) {

  NGAP_ErrorIndication_t    *container;
  NGAP_ErrorIndicationIEs_t *ie;
  ngap_gNB_amf_data_t        *amf_desc_p;
  uint64_t                 amf_ue_ngap_id;
    
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.ErrorIndication;

  /* NG Setup Failure == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    NGAP_WARN("[SCTP %d] Received ng Error indication on stream != 0 (%d)\n",
              assoc_id, stream);
  }

  if ((amf_desc_p = ngap_gNB_get_AMF(NULL, assoc_id, 0)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received ng Error indication for non existing "
               "AMF context\n", assoc_id);
    return -1;
  }

  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_ErrorIndicationIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID, false);

  /* optional */
  if (ie != NULL) {
    asn_INTEGER2ulong(&(ie->value.choice.AMF_UE_NGAP_ID), &amf_ue_ngap_id);
    NGAP_WARN("Received NG Error indication AMF UE NGAP ID 0x%lx\n", amf_ue_ngap_id);
  }

  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_ErrorIndicationIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID, false);

  /* optional */
  if (ie != NULL) {
    NGAP_WARN("Received NG Error indication gNB UE NGAP ID 0x%lx\n", ie->value.choice.RAN_UE_NGAP_ID);
  }

  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_ErrorIndicationIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_Cause, false);

  /* optional */
  if (ie) {
    switch(ie->value.choice.Cause.present) {
      case NGAP_Cause_PR_NOTHING:
        NGAP_WARN("Received NG Error indication cause NOTHING\n");
        break;

      case NGAP_Cause_PR_radioNetwork:
        switch (ie->value.choice.Cause.choice.radioNetwork) {
          case NGAP_CauseRadioNetwork_unspecified:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_unspecified\n");
            break;

          case NGAP_CauseRadioNetwork_txnrelocoverall_expiry:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_txnrelocoverall_expiry\n");
            break;

          case NGAP_CauseRadioNetwork_successful_handover:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_successful_handover\n");
            break;

          case NGAP_CauseRadioNetwork_release_due_to_ngran_generated_reason:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_release_due_to_ngran_generated_reason\n");
            break;
          
          case NGAP_CauseRadioNetwork_release_due_to_5gc_generated_reason:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_release_due_to_5gc_generated_reason\n");
            break;

          case NGAP_CauseRadioNetwork_handover_cancelled:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_handover_cancelled\n");
            break;

          case NGAP_CauseRadioNetwork_partial_handover:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_partial_handover\n");
            break;

          case NGAP_CauseRadioNetwork_ho_failure_in_target_5GC_ngran_node_or_target_system:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_ho_failure_in_target_5GC_ngran_node_or_target_system\n");
            break;

          case NGAP_CauseRadioNetwork_ho_target_not_allowed:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_ho_target_not_allowed\n");
            break;

          case NGAP_CauseRadioNetwork_tngrelocoverall_expiry:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_tngrelocoverall_expiry\n");
            break;

          case NGAP_CauseRadioNetwork_tngrelocprep_expiry:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_tngrelocprep_expiry\n");
            break;

          case NGAP_CauseRadioNetwork_cell_not_available:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_cell_not_available\n");
            break;

          case NGAP_CauseRadioNetwork_unknown_targetID:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_unknown_targetID\n");
            break;

          case NGAP_CauseRadioNetwork_no_radio_resources_available_in_target_cell:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_no_radio_resources_available_in_target_cell\n");
            break;

          case NGAP_CauseRadioNetwork_unknown_local_UE_NGAP_ID:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_unknown_local_UE_NGAP_ID\n");
            break;

          case NGAP_CauseRadioNetwork_inconsistent_remote_UE_NGAP_ID:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_inconsistent_remote_UE_NGAP_ID\n");
            break;

          case NGAP_CauseRadioNetwork_handover_desirable_for_radio_reason:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_handover_desirable_for_radio_reason\n");
            break;

          case NGAP_CauseRadioNetwork_time_critical_handover:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_time_critical_handover\n");
            break;

          case NGAP_CauseRadioNetwork_resource_optimisation_handover:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_resource_optimisation_handover\n");
            break;

          case NGAP_CauseRadioNetwork_reduce_load_in_serving_cell:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_reduce_load_in_serving_cell\n");
            break;

          case NGAP_CauseRadioNetwork_user_inactivity:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_user_inactivity\n");
            break;

          case NGAP_CauseRadioNetwork_radio_connection_with_ue_lost:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_radio_connection_with_ue_lost\n");
            break;

          case NGAP_CauseRadioNetwork_radio_resources_not_available:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_radio_resources_not_available\n");
            break;

          case NGAP_CauseRadioNetwork_invalid_qos_combination:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_invalid_qos_combination\n");
            break;

          case NGAP_CauseRadioNetwork_failure_in_radio_interface_procedure:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_failure_in_radio_interface_procedure\n");
            break;

          case NGAP_CauseRadioNetwork_interaction_with_other_procedure:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_interaction_with_other_procedure\n");
            break;

          case NGAP_CauseRadioNetwork_unknown_PDU_session_ID:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_unknown_PDU_session_ID\n");
            break;

          case NGAP_CauseRadioNetwork_unkown_qos_flow_ID:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_unkown_qos_flow_ID\n");
            break;

          case NGAP_CauseRadioNetwork_multiple_PDU_session_ID_instances:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_multiple_PDU_session_ID_instances\n");
            break;

          case NGAP_CauseRadioNetwork_multiple_qos_flow_ID_instances:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_multiple_qos_flow_ID_instances\n");
            break;

          case NGAP_CauseRadioNetwork_encryption_and_or_integrity_protection_algorithms_not_supported:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_encryption_and_or_integrity_protection_algorithms_not_supported\n");
            break;

          case NGAP_CauseRadioNetwork_ng_intra_system_handover_triggered:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_ng_intra_system_handover_triggered\n");
            break;

          case NGAP_CauseRadioNetwork_ng_inter_system_handover_triggered:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_ng_inter_system_handover_triggered\n");
            break;

          case NGAP_CauseRadioNetwork_xn_handover_triggered:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_xn_handover_triggered\n");
            break;

          case NGAP_CauseRadioNetwork_not_supported_5QI_value:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_not_supported_5QI_value\n");
            break;

          case NGAP_CauseRadioNetwork_ue_context_transfer:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_ue_context_transfer\n");
            break;

          case NGAP_CauseRadioNetwork_ims_voice_eps_fallback_or_rat_fallback_triggered:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_ims_voice_eps_fallback_or_rat_fallback_triggered\n");
            break;

          case NGAP_CauseRadioNetwork_up_integrity_protection_not_possible:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_up_integrity_protection_not_possible\n");
            break;

          case NGAP_CauseRadioNetwork_up_confidentiality_protection_not_possible:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_up_confidentiality_protection_not_possible\n");
            break;

          case NGAP_CauseRadioNetwork_slice_not_supported:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_slice_not_supported\n");
            break;

          case NGAP_CauseRadioNetwork_ue_in_rrc_inactive_state_not_reachable:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_ue_in_rrc_inactive_state_not_reachable\n");
            break;

          case NGAP_CauseRadioNetwork_redirection:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_redirection\n");
            break;

          case NGAP_CauseRadioNetwork_resources_not_available_for_the_slice:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_resources_not_available_for_the_slice\n");
            break;

          case NGAP_CauseRadioNetwork_ue_max_integrity_protected_data_rate_reason:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_ue_max_integrity_protected_data_rate_reason\n");
            break;

          case NGAP_CauseRadioNetwork_release_due_to_cn_detected_mobility:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_release_due_to_cn_detected_mobility\n");
            break;

          case NGAP_CauseRadioNetwork_n26_interface_not_available:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_n26_interface_not_available\n");
            break;

          case NGAP_CauseRadioNetwork_release_due_to_pre_emption:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_release_due_to_pre_emption\n");
            break;

          case NGAP_CauseRadioNetwork_multiple_location_reporting_reference_ID_instances:
            NGAP_WARN("Received NG Error indication NGAP_CauseRadioNetwork_multiple_location_reporting_reference_ID_instances\n");
            break;

          default:
            NGAP_WARN("Received NG Error indication cause radio network case not handled\n");
        }

        break;

      case NGAP_Cause_PR_transport:
        switch (ie->value.choice.Cause.choice.transport) {
          case NGAP_CauseTransport_transport_resource_unavailable:
            NGAP_WARN("Received NG Error indication NGAP_CauseTransport_transport_resource_unavailable\n");
            break;

          case NGAP_CauseTransport_unspecified:
            NGAP_WARN("Received NG Error indication NGAP_CauseTransport_unspecified\n");
            break;

          default:
            NGAP_WARN("Received NG Error indication cause transport case not handled\n");
        }

        break;

      case NGAP_Cause_PR_nas:
        switch (ie->value.choice.Cause.choice.nas) {
          case NGAP_CauseNas_normal_release:
            NGAP_WARN("Received NG Error indication NGAP_CauseNas_normal_release\n");
            break;

          case NGAP_CauseNas_authentication_failure:
            NGAP_WARN("Received NG Error indication NGAP_CauseNas_authentication_failure\n");
            break;

          case NGAP_CauseNas_deregister:
            NGAP_WARN("Received NG Error indication NGAP_CauseNas_deregister\n");
            break;

          case NGAP_CauseNas_unspecified:
            NGAP_WARN("Received NG Error indication NGAP_CauseNas_unspecified\n");
            break;

          default:
            NGAP_WARN("Received NG Error indication cause nas case not handled\n");
        }

        break;

      case NGAP_Cause_PR_protocol:
        switch (ie->value.choice.Cause.choice.protocol) {
          case NGAP_CauseProtocol_transfer_syntax_error:
            NGAP_WARN("Received NG Error indication NGAP_CauseProtocol_transfer_syntax_error\n");
            break;

          case NGAP_CauseProtocol_abstract_syntax_error_reject:
            NGAP_WARN("Received NG Error indication NGAP_CauseProtocol_abstract_syntax_error_reject\n");
            break;

          case NGAP_CauseProtocol_abstract_syntax_error_ignore_and_notify:
            NGAP_WARN("Received NG Error indication NGAP_CauseProtocol_abstract_syntax_error_ignore_and_notify\n");
            break;

          case NGAP_CauseProtocol_message_not_compatible_with_receiver_state:
            NGAP_WARN("Received NG Error indication NGAP_CauseProtocol_message_not_compatible_with_receiver_state\n");
            break;

          case NGAP_CauseProtocol_semantic_error:
            NGAP_WARN("Received NG Error indication NGAP_CauseProtocol_semantic_error\n");
            break;

          case NGAP_CauseProtocol_abstract_syntax_error_falsely_constructed_message:
            NGAP_WARN("Received NG Error indication NGAP_CauseProtocol_abstract_syntax_error_falsely_constructed_message\n");
            break;

          case NGAP_CauseProtocol_unspecified:
            NGAP_WARN("Received NG Error indication NGAP_CauseProtocol_unspecified\n");
            break;

          default:
            NGAP_WARN("Received NG Error indication cause protocol case not handled\n");
        }

        break;

      case NGAP_Cause_PR_misc:
        switch (ie->value.choice.Cause.choice.protocol) {
          case NGAP_CauseMisc_control_processing_overload:
            NGAP_WARN("Received NG Error indication NGAP_CauseMisc_control_processing_overload\n");
            break;

          case NGAP_CauseMisc_not_enough_user_plane_processing_resources:
            NGAP_WARN("Received NG Error indication NGAP_CauseMisc_not_enough_user_plane_processing_resources\n");
            break;

          case NGAP_CauseMisc_hardware_failure:
            NGAP_WARN("Received NG Error indication NGAP_CauseMisc_hardware_failure\n");
            break;

          case NGAP_CauseMisc_om_intervention:
            NGAP_WARN("Received NG Error indication NGAP_CauseMisc_om_intervention\n");
            break;

          case NGAP_CauseMisc_unspecified:
            NGAP_WARN("Received NG Error indication NGAP_CauseMisc_unspecified\n");
            break;

          case NGAP_CauseMisc_unknown_PLMN:
            NGAP_WARN("Received NG Error indication NGAP_CauseMisc_unknown_PLMN\n");
            break;

          default:
            NGAP_WARN("Received NG Error indication cause misc case not handled\n");
        }

        break;
        
      default:
       NGAP_WARN("Received NG Error indication cause NGAP_Cause_PR_choice_Extensions\n");
       break;
      
    }
  }


  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_ErrorIndicationIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_CriticalityDiagnostics, false);

  if (ie) {
    // TODO continue
  }


  // TODO continue
  return 0;
}


static
int ngap_gNB_handle_initial_context_request(uint32_t   assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu) {

  int i;
  ngap_gNB_amf_data_t   *amf_desc_p       = NULL;
  ngap_gNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;
  NGAP_InitialContextSetupRequest_t    *container;
  NGAP_InitialContextSetupRequestIEs_t *ie;
  NGAP_RAN_UE_NGAP_ID_t    ran_ue_ngap_id;
  uint64_t                 amf_ue_ngap_id;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.InitialContextSetupRequest;

  if ((amf_desc_p = ngap_gNB_get_AMF(NULL, assoc_id, 0)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received initial context setup request for non "
               "existing AMF context\n", assoc_id);
    return -1;
  }

  /* id-AMF-UE-NGAP-ID */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_InitialContextSetupRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    asn_INTEGER2ulong(&(ie->value.choice.AMF_UE_NGAP_ID), &amf_ue_ngap_id);
  } else {
    return -1;
  }

  /* id-RAN-UE-NGAP-ID */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_InitialContextSetupRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    ran_ue_ngap_id = ie->value.choice.RAN_UE_NGAP_ID;

    if ((ue_desc_p = ngap_gNB_get_ue_context(amf_desc_p->ngap_gNB_instance,
                     ran_ue_ngap_id)) == NULL) {
      NGAP_ERROR("[SCTP %d] Received initial context setup request for non "
                 "existing UE context 0x%06lx\n", assoc_id,
                 ran_ue_ngap_id);
      return -1;
    }
  } else {
    return -1;
  }

  /* Initial context request = UE-related procedure -> stream != 0 */
  //if (stream == 0) {
  //  NGAP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
  //             assoc_id, stream);
  //  return -1;
  //}

  ue_desc_p->rx_stream = stream;
  ue_desc_p->amf_ue_ngap_id = amf_ue_ngap_id;
  
  message_p        = itti_alloc_new_message(TASK_NGAP, 0, NGAP_INITIAL_CONTEXT_SETUP_REQ);
  NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).ue_initial_id  = ue_desc_p->ue_initial_id;
  ue_desc_p->ue_initial_id = 0;
  NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).gNB_ue_ngap_id = ue_desc_p->gNB_ue_ngap_id;
  NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).amf_ue_ngap_id = ue_desc_p->amf_ue_ngap_id;
  /* id-UEAggregateMaximumBitRate */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_InitialContextSetupRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_UEAggregateMaximumBitRate, false);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    asn_INTEGER2ulong(&(ie->value.choice.UEAggregateMaximumBitRate.uEAggregateMaximumBitRateUL),
                      &(NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).ue_ambr.br_ul));
    asn_INTEGER2ulong(&(ie->value.choice.UEAggregateMaximumBitRate.uEAggregateMaximumBitRateDL),
                      &(NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).ue_ambr.br_dl));
  } else {
    NGAP_ERROR("could not found NGAP_ProtocolIE_ID_id_UEAggregateMaximumBitRate\n");
  }


  /* id-GUAMI */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_InitialContextSetupRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_GUAMI, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    TBCD_TO_MCC_MNC(&ie->value.choice.GUAMI.pLMNIdentity, NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).guami.mcc,
                    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).guami.mnc, NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).guami.mnc_len);
    
    OCTET_STRING_TO_INT8(&ie->value.choice.GUAMI.aMFRegionID, NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).guami.amf_region_id);
    OCTET_STRING_TO_INT16(&ie->value.choice.GUAMI.aMFSetID, NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).guami.amf_set_id);
    OCTET_STRING_TO_INT8(&ie->value.choice.GUAMI.aMFPointer, NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).guami.amf_pointer);
  } else {/* ie != NULL */
    return -1;
  }

  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_InitialContextSetupRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListCxtReq, false);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).nb_of_pdusessions =
      ie->value.choice.PDUSessionResourceSetupListCxtReq.list.count;

    for (i = 0; i < ie->value.choice.PDUSessionResourceSetupListCxtReq.list.count; i++) {
      NGAP_PDUSessionResourceSetupItemCxtReq_t *item_p;
      asn_dec_rval_t                           dec_rval;
      NGAP_PDUSessionResourceSetupRequestTransfer_t *pdusessionTransfer_p = NULL;
      NGAP_PDUSessionResourceSetupRequestTransferIEs_t *pdusessionTransfer_ies = NULL;
        
      item_p = (NGAP_PDUSessionResourceSetupItemCxtReq_t *)ie->value.choice.PDUSessionResourceSetupListCxtReq.list.array[i];
      NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].pdusession_id = item_p->pDUSessionID;

      if (item_p->nAS_PDU != NULL) {
        /* Only copy NAS pdu if present */
        NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].nas_pdu.length = item_p->nAS_PDU->size;
        NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].nas_pdu.buffer =
          malloc(sizeof(uint8_t) * item_p->nAS_PDU->size);
        memcpy(NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].nas_pdu.buffer,
               item_p->nAS_PDU->buf, item_p->nAS_PDU->size);
        NGAP_DEBUG("Received NAS message with the PDUSESSION setup procedure\n");
      } else {
        NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].nas_pdu.length = 0;
        NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].nas_pdu.buffer = NULL;
      }


      dec_rval = aper_decode(NULL,
                               &asn_DEF_NGAP_PDUSessionResourceSetupRequestTransfer,
                               (void **)&pdusessionTransfer_p,
                               item_p->pDUSessionResourceSetupRequestTransfer.buf,
                               item_p->pDUSessionResourceSetupRequestTransfer.size, 0, 0);

      if(dec_rval.code != RC_OK) {
        NGAP_ERROR("could not decode PDUSessionResourceSetupRequestTransfer\n");
        return -1;
      }

      for(int j=0; j< pdusessionTransfer_p->protocolIEs.list.count; j++) {
        pdusessionTransfer_ies = pdusessionTransfer_p->protocolIEs.list.array[j];
        switch(pdusessionTransfer_ies->id) {
          /* optional PDUSessionAggregateMaximumBitRate */
          case NGAP_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate:
            break;
        
          /* mandatory UL-NGU-UP-TNLInformation */
          case NGAP_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation:
            {
              NGAP_GTPTunnel_t  *gTPTunnel_p;
              gTPTunnel_p = pdusessionTransfer_ies->value.choice.UPTransportLayerInformation.choice.gTPTunnel;
              
              /* Set the transport layer address */
              memcpy(NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].upf_addr.buffer,
                     gTPTunnel_p->transportLayerAddress.buf, gTPTunnel_p->transportLayerAddress.size);
              
              NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].upf_addr.length =
                gTPTunnel_p->transportLayerAddress.size * 8 - gTPTunnel_p->transportLayerAddress.bits_unused;
              
              /* GTP tunnel endpoint ID */
              OCTET_STRING_TO_INT32(&gTPTunnel_p->gTP_TEID, NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].gtp_teid);
            }
            
            break;
            
          /* optional AdditionalUL-NGU-UP-TNLInformation */
          case NGAP_ProtocolIE_ID_id_AdditionalUL_NGU_UP_TNLInformation:
            break;

          /* optional DataForwardingNotPossible */
          case NGAP_ProtocolIE_ID_id_DataForwardingNotPossible:
            break;

          /* mandatory PDUSessionType */
          case NGAP_ProtocolIE_ID_id_PDUSessionType:
            NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].upf_addr.pdu_session_type = (uint8_t)pdusessionTransfer_ies->value.choice.PDUSessionType;
            break;

          /* optional SecurityIndication */
          case NGAP_ProtocolIE_ID_id_SecurityIndication:
            break;

          /* optional NetworkInstance */
          case NGAP_ProtocolIE_ID_id_NetworkInstance:
            break;

          /* mandatory QosFlowSetupRequestList */
          case NGAP_ProtocolIE_ID_id_QosFlowSetupRequestList:
            {
              NGAP_QosFlowSetupRequestItem_t *qosFlowItem_p;
              
              NGAP_DEBUG("QosFlowSetupRequestList.list.count %d\n", pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.count);
              DevAssert(pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.count > 0);
              DevAssert(pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.count <= NGAP_maxnoofQosFlows);

              NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].nb_qos = pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.count;
                
              for(int qosIdx = 0; qosIdx < pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.count; qosIdx++) {
                qosFlowItem_p = pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.array[qosIdx];
                
                /* Set the QOS informations */
                NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].qos[qosIdx].qfi = (uint8_t)qosFlowItem_p->qosFlowIdentifier;
                if(qosFlowItem_p->qosFlowLevelQosParameters.qosCharacteristics.present == NGAP_QosCharacteristics_PR_nonDynamic5QI){
                  if(qosFlowItem_p->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI != NULL){
                    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].qos[qosIdx].fiveQI =
                        (uint64_t)qosFlowItem_p->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI->fiveQI;
                  }
                }
              
                NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].qos[qosIdx].allocation_retention_priority.priority_level =
                  qosFlowItem_p->qosFlowLevelQosParameters.allocationAndRetentionPriority.priorityLevelARP;
                NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].qos[qosIdx].allocation_retention_priority.pre_emp_capability =
                  qosFlowItem_p->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionCapability;
                NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).pdusession_param[i].qos[qosIdx].allocation_retention_priority.pre_emp_vulnerability =
                  qosFlowItem_p->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionVulnerability;
              }
            }
          
            break;
            
          /* optional CommonNetworkInstance */
          case NGAP_ProtocolIE_ID_id_CommonNetworkInstance:
            break;
            
          default:
            NGAP_ERROR("could not found protocolIEs id %ld\n", pdusessionTransfer_ies->id);
            return -1;
        }
      }

    } /* for i... */
  } else {/* ie != NULL */
    NGAP_ERROR("could not found NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListCxtReq\n");
  }
  
  /* id-AllowedNSSAI */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_InitialContextSetupRequestIEs_t, ie, container,
                               NGAP_ProtocolIE_ID_id_AllowedNSSAI, true);
  
  //if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    NGAP_AllowedNSSAI_Item_t *allow_nssai_item_p = NULL;

    //NGAP_DEBUG("AllowedNSSAI.list.count %d\n", ie->value.choice.AllowedNSSAI.list.count);
    //DevAssert(ie->value.choice.AllowedNSSAI.list.count > 0);
    //DevAssert(ie->value.choice.AllowedNSSAI.list.count <= NGAP_maxnoofAllowedS_NSSAIs);

    if (ie == NULL) {
        NGAP_WARN("AllowedNSSAI not present, forging 2 NSSAI\n");

    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).nb_allowed_nssais = 2;

    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[0].sST = 01;

    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[0].sD_flag = 1;
    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[0].sD[0] = 01;
    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[0].sD[1] = 02;
    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[0].sD[2] = 03;

    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[1].sST = 01;
 
    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[1].sD_flag = 1;
    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[1].sD[0] = 00;//11;
    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[1].sD[1] = 00;//22;
    	NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[1].sD[2] = 01;//33;
    } else {
    NGAP_INFO("AllowedNSSAI.list.count %d\n", ie->value.choice.AllowedNSSAI.list.count);
    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).nb_allowed_nssais = ie->value.choice.AllowedNSSAI.list.count;
    
    for(i = 0; i < ie->value.choice.AllowedNSSAI.list.count; i++) {
      allow_nssai_item_p = ie->value.choice.AllowedNSSAI.list.array[i];
      
      OCTET_STRING_TO_INT8(&allow_nssai_item_p->s_NSSAI.sST, NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[i].sST);

      if(allow_nssai_item_p->s_NSSAI.sD != NULL) {
        NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[i].sD_flag = 1;
        NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[i].sD[0] = allow_nssai_item_p->s_NSSAI.sD->buf[0];
        NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[i].sD[1] = allow_nssai_item_p->s_NSSAI.sD->buf[1];
        NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).allowed_nssai[i].sD[2] = allow_nssai_item_p->s_NSSAI.sD->buf[2];
      }
    }
   }
  //} else {/* ie != NULL */
  //  return -1;
  //}

  /* id-UESecurityCapabilities */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_InitialContextSetupRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_UESecurityCapabilities, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_capabilities.nRencryption_algorithms =
      BIT_STRING_to_uint16(&ie->value.choice.UESecurityCapabilities.nRencryptionAlgorithms);
    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_capabilities.nRintegrity_algorithms =
      BIT_STRING_to_uint16(&ie->value.choice.UESecurityCapabilities.nRintegrityProtectionAlgorithms);
    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_capabilities.eUTRAencryption_algorithms =
      BIT_STRING_to_uint16(&ie->value.choice.UESecurityCapabilities.eUTRAencryptionAlgorithms);
    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_capabilities.eUTRAintegrity_algorithms =
      BIT_STRING_to_uint16(&ie->value.choice.UESecurityCapabilities.eUTRAintegrityProtectionAlgorithms);
    /* id-SecurityKey : Copy the security key */
  } else {/* ie != NULL */
    return -1;
  }

  /* id-MobilityRestrictionList */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_InitialContextSetupRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_SecurityKey, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    memcpy(&NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).security_key,
           ie->value.choice.SecurityKey.buf, ie->value.choice.SecurityKey.size);
    
  } else {/* ie != NULL */
    return -1;
  }

  /* id-NAS-PDU */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_InitialContextSetupRequestIEs_t, ie, container,
                               NGAP_ProtocolIE_ID_id_MobilityRestrictionList, false);
  
  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    NGAP_MobilityRestrictionList_t *mobility_rest_list_p = NULL;
    mobility_rest_list_p = &ie->value.choice.MobilityRestrictionList;

    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).mobility_restriction_flag   = 1;
    TBCD_TO_MCC_MNC(&mobility_rest_list_p->servingPLMN, 
                    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).mobility_restriction.serving_plmn.mcc,
                    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).mobility_restriction.serving_plmn.mnc, 
                    NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).mobility_restriction.serving_plmn.mnc_digit_length);
  } 


  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_InitialContextSetupRequestIEs_t, ie, container,
                                 NGAP_ProtocolIE_ID_id_NAS_PDU, false);
    
  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    if(ie->value.choice.NAS_PDU.size > 0) {
      NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).nas_pdu_flag   = 1;
      NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).nas_pdu.length = ie->value.choice.NAS_PDU.size;
      NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).nas_pdu.buffer = malloc(sizeof(uint8_t) * ie->value.choice.NAS_PDU.size);
      memcpy(NGAP_INITIAL_CONTEXT_SETUP_REQ(message_p).nas_pdu.buffer,
             ie->value.choice.NAS_PDU.buf, ie->value.choice.NAS_PDU.size);
    }
      
  } 

  itti_send_msg_to_task(TASK_RRC_GNB, ue_desc_p->gNB_instance->instance, message_p);

  return 0;
}


static
int ngap_gNB_handle_ue_context_release_command(uint32_t   assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu) {

  ngap_gNB_amf_data_t   *amf_desc_p       = NULL;
  ngap_gNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;
  uint64_t                            amf_ue_ngap_id;
  NGAP_RAN_UE_NGAP_ID_t               gnb_ue_ngap_id;
  NGAP_UEContextReleaseCommand_t     *container;
  NGAP_UEContextReleaseCommand_IEs_t *ie;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.UEContextReleaseCommand;

  if ((amf_desc_p = ngap_gNB_get_AMF(NULL, assoc_id, 0)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received UE context release command for non "
               "existing AMF context\n", assoc_id);
    return -1;
  }

  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_UEContextReleaseCommand_IEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_UE_NGAP_IDs, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    switch (ie->value.choice.UE_NGAP_IDs.present) {
      case NGAP_UE_NGAP_IDs_PR_uE_NGAP_ID_pair:
        gnb_ue_ngap_id = ie->value.choice.UE_NGAP_IDs.choice.uE_NGAP_ID_pair->rAN_UE_NGAP_ID;
        asn_INTEGER2ulong(&(ie->value.choice.UE_NGAP_IDs.choice.uE_NGAP_ID_pair->aMF_UE_NGAP_ID), &amf_ue_ngap_id);
        MSC_LOG_RX_MESSAGE(
          MSC_NGAP_GNB,
          MSC_NGAP_AMF,
          NULL,0,
          "0 UEContextRelease/%s gNB_ue_ngap_id "NGAP_UE_ID_FMT" amf_ue_ngap_id "NGAP_UE_ID_FMT" len %u",
          ngap_direction2String(pdu->present - 1),
          gnb_ue_ngap_id,
          amf_ue_ngap_id);

        if ((ue_desc_p = ngap_gNB_get_ue_context(amf_desc_p->ngap_gNB_instance,
                         gnb_ue_ngap_id)) == NULL) {
          NGAP_ERROR("[SCTP %d] Received UE context release command for non "
                     "existing UE context 0x%06lx\n",
                     assoc_id,
                     gnb_ue_ngap_id);
          return -1;
        } else {
          MSC_LOG_TX_MESSAGE(
            MSC_NGAP_GNB,
            MSC_RRC_GNB,
            NULL,0,
            "0 NGAP_UE_CONTEXT_RELEASE_COMMAND/%d gNB_ue_ngap_id "NGAP_UE_ID_FMT" ",
            gnb_ue_ngap_id);
          message_p    = itti_alloc_new_message(TASK_NGAP, 0, NGAP_UE_CONTEXT_RELEASE_COMMAND);

          if (ue_desc_p->amf_ue_ngap_id == 0) { // case of Detach Request and switch off from RRC_IDLE mode
            ue_desc_p->amf_ue_ngap_id = amf_ue_ngap_id;
          }

          NGAP_UE_CONTEXT_RELEASE_COMMAND(message_p).gNB_ue_ngap_id = gnb_ue_ngap_id;
          itti_send_msg_to_task(TASK_RRC_GNB, ue_desc_p->gNB_instance->instance, message_p);
          return 0;
        }

        break;

      //#warning "TODO mapping amf_ue_ngap_id  gnb_ue_ngap_id?"

      case NGAP_UE_NGAP_IDs_PR_aMF_UE_NGAP_ID:
        asn_INTEGER2ulong(&(ie->value.choice.UE_NGAP_IDs.choice.aMF_UE_NGAP_ID), &amf_ue_ngap_id);
        NGAP_ERROR("TO DO mapping amf_ue_ngap_id  gnb_ue_ngap_id");
        (void)amf_ue_ngap_id; /* TODO: remove - it's to remove gcc warning about unused var */

      case NGAP_UE_NGAP_IDs_PR_NOTHING:
      default:
        NGAP_ERROR("NGAP_UE_CONTEXT_RELEASE_COMMAND not processed, missing info elements");
        return -1;
    }
  } else {
    return -1;
  }

  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_UEContextReleaseCommand_IEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_Cause, true);
  /* TBD */

  return 0;
}

static
int ngap_gNB_handle_pdusession_setup_request(uint32_t         assoc_id,
                                        uint32_t         stream,
                                        NGAP_NGAP_PDU_t *pdu) {
//  NGAP_AMF_UE_NGAP_ID_t       amf_ue_ngap_id;
  uint64_t                      amf_ue_ngap_id;
  NGAP_RAN_UE_NGAP_ID_t         ran_ue_ngap_id;
  ngap_gNB_amf_data_t          *amf_desc_p       = NULL;
  ngap_gNB_ue_context_t        *ue_desc_p        = NULL;
  MessageDef                   *message_p        = NULL;
  NGAP_PDUSessionResourceSetupRequest_t     *container;
  NGAP_PDUSessionResourceSetupRequestIEs_t  *ie;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.PDUSessionResourceSetupRequest;

  if ((amf_desc_p = ngap_gNB_get_AMF(NULL, assoc_id, 0)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received pdu session resource setup request for non "
               "existing AMF context\n", assoc_id);
    return -1;
  }

  /* id-AMF-UE-NGAP-ID */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceSetupRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
     asn_INTEGER2ulong(&(ie->value.choice.AMF_UE_NGAP_ID), &amf_ue_ngap_id);
  } else {
    return -1;
  }

  /* id-gNB-UE-NGAP-ID */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceSetupRequestIEs_t, ie, container,
                         NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    ran_ue_ngap_id = ie->value.choice.RAN_UE_NGAP_ID;
  } else {
    return -1;
  }

  if ((ue_desc_p = ngap_gNB_get_ue_context(amf_desc_p->ngap_gNB_instance,
                   ran_ue_ngap_id)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received pdu session resource setup request for non "
               "existing UE context 0x%06lx\n", assoc_id,
               ran_ue_ngap_id);
    return -1;
  }

  /* Initial context request = UE-related procedure -> stream != 0 */
  // if (stream == 0) {
  //   NGAP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
  //              assoc_id, stream);
  //   return -1;
  // }

  ue_desc_p->rx_stream = stream;

  if ( ue_desc_p->amf_ue_ngap_id != amf_ue_ngap_id) {
    NGAP_WARN("UE context amf_ue_ngap_id is different form that of the message (%lu != %lu)",
              (uint64_t)ue_desc_p->amf_ue_ngap_id, amf_ue_ngap_id);
  }

  message_p        = itti_alloc_new_message(TASK_NGAP, 0, NGAP_PDUSESSION_SETUP_REQ);
  NGAP_PDUSESSION_SETUP_REQ(message_p).ue_initial_id   = ue_desc_p->ue_initial_id;
  ue_desc_p->ue_initial_id = 0;
  NGAP_PDUSESSION_SETUP_REQ(message_p).gNB_ue_ngap_id = ue_desc_p->gNB_ue_ngap_id;
  NGAP_PDUSESSION_SETUP_REQ(message_p).amf_ue_ngap_id = ue_desc_p->amf_ue_ngap_id;

  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceSetupRequestIEs_t, ie, container,
                         NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListSUReq, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    NGAP_PDUSESSION_SETUP_REQ(message_p).nb_pdusessions_tosetup =
      ie->value.choice.PDUSessionResourceSetupListSUReq.list.count;

    for (int i = 0; i < ie->value.choice.PDUSessionResourceSetupListSUReq.list.count; i++) {
      NGAP_PDUSessionResourceSetupItemSUReq_t          *item_p;
      asn_dec_rval_t                                   dec_rval;
      NGAP_PDUSessionResourceSetupRequestTransfer_t    *pdusessionTransfer_p   = NULL;
      NGAP_PDUSessionResourceSetupRequestTransferIEs_t *pdusessionTransfer_ies = NULL;

      item_p = (NGAP_PDUSessionResourceSetupItemSUReq_t *)ie->value.choice.PDUSessionResourceSetupListSUReq.list.array[i];
      NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].pdusession_id = item_p->pDUSessionID;


      // check for the NAS PDU
      if (item_p->pDUSessionNAS_PDU->size > 0 ) {
        NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].nas_pdu.length = item_p->pDUSessionNAS_PDU->size;
        NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].nas_pdu.buffer = malloc(sizeof(uint8_t) * item_p->pDUSessionNAS_PDU->size);
        memcpy(NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].nas_pdu.buffer,
               item_p->pDUSessionNAS_PDU->buf, item_p->pDUSessionNAS_PDU->size);
        // NGAP_INFO("received a NAS PDU with size %d (%02x.%02x)\n",NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].nas_pdu.length, item_p->nAS_PDU.buf[0], item_p->nAS_PDU.buf[1]);
      } else {
        NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].nas_pdu.length = 0;
        NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].nas_pdu.buffer = NULL;
        NGAP_WARN("NAS PDU is not provided, generate a PDUSESSION_SETUP Failure (TBD) back to AMF \n");
      }

      dec_rval = aper_decode(NULL,
                               &asn_DEF_NGAP_PDUSessionResourceSetupRequestTransfer,
                               (void **)&pdusessionTransfer_p,
                               item_p->pDUSessionResourceSetupRequestTransfer.buf,
                               item_p->pDUSessionResourceSetupRequestTransfer.size, 0, 0);

      if(dec_rval.code != RC_OK) {
        NGAP_ERROR("could not decode PDUSessionResourceSetupRequestTransfer\n");
        return -1;
      }

      for(int j=0; j< pdusessionTransfer_p->protocolIEs.list.count; j++) {
        pdusessionTransfer_ies = pdusessionTransfer_p->protocolIEs.list.array[j];

        switch(pdusessionTransfer_ies->id) {
          /* optional PDUSessionAggregateMaximumBitRate */
          case NGAP_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate:
            break;

          /* mandatory UL-NGU-UP-TNLInformation */
          case NGAP_ProtocolIE_ID_id_UL_NGU_UP_TNLInformation:
          {
          	NGAP_GTPTunnel_t  *gTPTunnel_p;
              gTPTunnel_p = pdusessionTransfer_ies->value.choice.UPTransportLayerInformation.choice.gTPTunnel;
              /* The transport layer address for the IP packets */
              OCTET_STRING_TO_INT32(&gTPTunnel_p->gTP_TEID, NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].gtp_teid);
              NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].upf_addr.length =
              		gTPTunnel_p->transportLayerAddress.size * 8 - gTPTunnel_p->transportLayerAddress.bits_unused;
              memcpy(NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].upf_addr.buffer ,
              		gTPTunnel_p->transportLayerAddress.buf, gTPTunnel_p->transportLayerAddress.size);
          }
            break;

          /* optional AdditionalUL-NGU-UP-TNLInformation */
          case NGAP_ProtocolIE_ID_id_AdditionalUL_NGU_UP_TNLInformation:
            break;

          /* optional DataForwardingNotPossible */
          case NGAP_ProtocolIE_ID_id_DataForwardingNotPossible:
            break;

          /* mandatory PDUSessionType */
          case NGAP_ProtocolIE_ID_id_PDUSessionType:
            NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].upf_addr.pdu_session_type = (uint8_t)pdusessionTransfer_ies->value.choice.PDUSessionType;
            break;

          /* optional SecurityIndication */
          case NGAP_ProtocolIE_ID_id_SecurityIndication:
            break;

          /* optional NetworkInstance */
          case NGAP_ProtocolIE_ID_id_NetworkInstance:
            break;

          /* mandatory QosFlowSetupRequestList */
          case NGAP_ProtocolIE_ID_id_QosFlowSetupRequestList:
          {
            NGAP_QosFlowSetupRequestItem_t *qosFlowItem_p;

            NGAP_DEBUG("servedGUAMIs.list.count %d\n", pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.count);
            DevAssert(pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.count > 0);
            DevAssert(pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.count <= NGAP_maxnoofQosFlows);

            NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].nb_qos = pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.count;

            for(int qosIdx = 0; qosIdx < pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.count; qosIdx++){
              qosFlowItem_p = pdusessionTransfer_ies->value.choice.QosFlowSetupRequestList.list.array[qosIdx];

              /* Set the QOS informations */
              NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].qos[qosIdx].qfi = (uint8_t)qosFlowItem_p->qosFlowIdentifier;
              if(qosFlowItem_p->qosFlowLevelQosParameters.qosCharacteristics.present == NGAP_QosCharacteristics_PR_nonDynamic5QI){
                if(qosFlowItem_p->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI != NULL){
                  NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].qos[qosIdx].fiveQI = 
                    (uint64_t)qosFlowItem_p->qosFlowLevelQosParameters.qosCharacteristics.choice.nonDynamic5QI->fiveQI;
                }
              }
              NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].qos[qosIdx].allocation_retention_priority.priority_level =
                qosFlowItem_p->qosFlowLevelQosParameters.allocationAndRetentionPriority.priorityLevelARP;
              NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].qos[qosIdx].allocation_retention_priority.pre_emp_capability =
                qosFlowItem_p->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionCapability;
              NGAP_PDUSESSION_SETUP_REQ(message_p).pdusession_setup_params[i].qos[qosIdx].allocation_retention_priority.pre_emp_vulnerability =
                qosFlowItem_p->qosFlowLevelQosParameters.allocationAndRetentionPriority.pre_emptionVulnerability;
            }
          }
          break;

          /* optional CommonNetworkInstance */
          case NGAP_ProtocolIE_ID_id_CommonNetworkInstance:
            break;

          default:
            NGAP_ERROR("could not found protocolIEs id %ld\n", pdusessionTransfer_ies->id);
            return -1;
        }
      }/* for j... */
    } /* for i... */
    itti_send_msg_to_task(TASK_RRC_GNB, ue_desc_p->gNB_instance->instance, message_p);
  }else {/* ie == NULL */
    NGAP_ERROR("could not found NGAP_ProtocolIE_ID_id_PDUSessionResourceSetupListSUReq\n");
    return -1;
  }
  return 0;
}


static
int ngap_gNB_handle_paging(uint32_t               assoc_id,
                           uint32_t               stream,
                           NGAP_NGAP_PDU_t       *pdu) {

  ngap_gNB_amf_data_t   *amf_desc_p        = NULL;
  ngap_gNB_instance_t   *ngap_gNB_instance = NULL;
  MessageDef            *message_p         = NULL;
  NGAP_Paging_t         *container;
  NGAP_PagingIEs_t      *ie;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.Paging;
  // received Paging Message from AMF
  NGAP_DEBUG("[SCTP %d] Received Paging Message From AMF\n",assoc_id);

  /* Paging procedure -> stream != 0 */
  if (stream == 0) {
    LOG_W(NGAP,"[SCTP %d] Received Paging procedure on stream (%d)\n",
          assoc_id, stream);
    return -1;
  }

  if ((amf_desc_p = ngap_gNB_get_AMF(NULL, assoc_id, 0)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received Paging for non "
               "existing AMF context\n", assoc_id);
    return -1;
  }

  ngap_gNB_instance = amf_desc_p->ngap_gNB_instance;

  if (ngap_gNB_instance == NULL) {
    NGAP_ERROR("[SCTP %d] Received Paging for non existing AMF context : ngap_gNB_instance is NULL\n",
               assoc_id);
    return -1;
  }

  message_p = itti_alloc_new_message(TASK_NGAP, 0, NGAP_PAGING_IND);
  /* convert NGAP_PagingIEs_t to ngap_paging_ind_t */
  /* id-UEIdentityIndexValue : convert UE Identity Index value */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PagingIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_UEPagingIdentity, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    OCTET_STRING_TO_INT16(&ie->value.choice.UEPagingIdentity.choice.fiveG_S_TMSI->aMFSetID, NGAP_PAGING_IND(message_p).ue_paging_identity.s_tmsi.amf_set_id);
    OCTET_STRING_TO_INT8(&ie->value.choice.UEPagingIdentity.choice.fiveG_S_TMSI->aMFPointer, NGAP_PAGING_IND(message_p).ue_paging_identity.s_tmsi.amf_pointer);
    OCTET_STRING_TO_INT32(&ie->value.choice.UEPagingIdentity.choice.fiveG_S_TMSI->fiveG_TMSI, NGAP_PAGING_IND(message_p).ue_paging_identity.s_tmsi.m_tmsi);

    NGAP_DEBUG("[SCTP %d] Received Paging Identity amf_set_id %d, amf_pointer %d, m_tmsi %d\n",
               assoc_id,
               NGAP_PAGING_IND(message_p).ue_paging_identity.s_tmsi.amf_set_id,
               NGAP_PAGING_IND(message_p).ue_paging_identity.s_tmsi.amf_pointer,
               NGAP_PAGING_IND(message_p).ue_paging_identity.s_tmsi.m_tmsi);
  } else {
    return -1;
  }


  NGAP_PAGING_IND(message_p).paging_drx = NGAP_PAGING_DRX_256;
  /* id-pagingDRX */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PagingIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_PagingDRX, false);

  /* optional */
  if (ie) {
    NGAP_PAGING_IND(message_p).paging_drx = ie->value.choice.PagingDRX;
  } else {
    NGAP_PAGING_IND(message_p).paging_drx = NGAP_PAGING_DRX_256;
  }

  memset (&NGAP_PAGING_IND(message_p).plmn_identity[0], 0, sizeof(plmn_identity_t)*256);
  memset (&NGAP_PAGING_IND(message_p).tac[0], 0, sizeof(int16_t)*256);
  NGAP_PAGING_IND(message_p).tai_size = 0;
  /* id-TAIList */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PagingIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_TAIListForPaging, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    NGAP_INFO("[SCTP %d] Received Paging taiList For Paging: count %d\n", assoc_id, ie->value.choice.TAIListForPaging.list.count);

    for (int i = 0; i < ie->value.choice.TAIListForPaging.list.count; i++) {
      NGAP_TAIListForPagingItem_t *item_p;
      item_p = (NGAP_TAIListForPagingItem_t *)ie->value.choice.TAIListForPaging.list.array[i];
      TBCD_TO_MCC_MNC(&(item_p->tAI.pLMNIdentity), NGAP_PAGING_IND(message_p).plmn_identity[i].mcc,
                      NGAP_PAGING_IND(message_p).plmn_identity[i].mnc,
                      NGAP_PAGING_IND(message_p).plmn_identity[i].mnc_digit_length);
      OCTET_STRING_TO_INT16(&(item_p->tAI.tAC), NGAP_PAGING_IND(message_p).tac[i]);
      NGAP_PAGING_IND(message_p).tai_size++;
      NGAP_DEBUG("[SCTP %d] Received Paging: MCC %d, MNC %d, TAC %d\n", assoc_id,
                 NGAP_PAGING_IND(message_p).plmn_identity[i].mcc,
                 NGAP_PAGING_IND(message_p).plmn_identity[i].mnc,
                 NGAP_PAGING_IND(message_p).tac[i]);
    }
  } else {
    return -1;
  }

  //paging parameter values
  NGAP_DEBUG("[SCTP %d] Received Paging parameters: Paging Identity amf_set_id %d amf_pointer %d m_tmsi %d paging_drx %d paging_priority %d\n",assoc_id,
             NGAP_PAGING_IND(message_p).ue_paging_identity.s_tmsi.amf_set_id,
             NGAP_PAGING_IND(message_p).ue_paging_identity.s_tmsi.amf_pointer,
             NGAP_PAGING_IND(message_p).ue_paging_identity.s_tmsi.m_tmsi,
             NGAP_PAGING_IND(message_p).paging_drx, NGAP_PAGING_IND(message_p).paging_priority);
  /* send message to RRC */
  itti_send_msg_to_task(TASK_RRC_GNB, ngap_gNB_instance->instance, message_p);

  return 0;
}

static
int ngap_gNB_handle_pdusession_modify_request(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu) {

  int i, nb_of_pdusessions_failed;
  ngap_gNB_amf_data_t           *amf_desc_p       = NULL;
  ngap_gNB_ue_context_t         *ue_desc_p        = NULL;
  MessageDef                    *message_p        = NULL;
  NGAP_PDUSessionResourceModifyRequest_t     *container;
  NGAP_PDUSessionResourceModifyRequestIEs_t  *ie;
  NGAP_RAN_UE_NGAP_ID_t         gnb_ue_ngap_id;
  uint64_t                      amf_ue_ngap_id;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.PDUSessionResourceModifyRequest;

  if ((amf_desc_p = ngap_gNB_get_AMF(NULL, assoc_id, 0)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received PDUSession Resource modify request for non "
               "existing AMF context\n", assoc_id);
    return -1;
  }

  /* id-AMF-UE-NGAP-ID */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceModifyRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    asn_INTEGER2ulong(&(ie->value.choice.AMF_UE_NGAP_ID), &amf_ue_ngap_id);
  } else {
    return -1;
  }

  /* id-RAN-UE-NGAP-ID */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceModifyRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    gnb_ue_ngap_id = ie->value.choice.RAN_UE_NGAP_ID;
  } else {
    return -1;
  }

  if ((ue_desc_p = ngap_gNB_get_ue_context(amf_desc_p->ngap_gNB_instance,
                   gnb_ue_ngap_id)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received PDUSession Resource modify request for non "
               "existing UE context 0x%08lx\n", assoc_id,
               gnb_ue_ngap_id);
    return -1;
  }

  /* PDUSession Resource modify request = UE-related procedure -> stream != 0 */
//   if (stream == 0) {
//     NGAP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
//                assoc_id, stream);
//     return -1;
//   }

  ue_desc_p->rx_stream = stream;

  if (ue_desc_p->amf_ue_ngap_id != amf_ue_ngap_id) {
    NGAP_WARN("UE context amf_ue_ngap_id is different form that of the message (%ld != %ld)",
              (uint64_t)ue_desc_p->amf_ue_ngap_id, amf_ue_ngap_id);
    message_p = itti_alloc_new_message (TASK_RRC_GNB, 0, NGAP_PDUSESSION_MODIFY_RESP);
    NGAP_PDUSESSION_MODIFY_RESP (message_p).gNB_ue_ngap_id = gnb_ue_ngap_id;
    NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceModifyRequestIEs_t, ie, container,
                               NGAP_ProtocolIE_ID_id_PDUSessionResourceModifyListModReq, true);

    if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
      for(nb_of_pdusessions_failed = 0; nb_of_pdusessions_failed < ie->value.choice.PDUSessionResourceModifyListModReq.list.count; nb_of_pdusessions_failed++) {
        NGAP_PDUSessionResourceModifyItemModReq_t *item_p;
        item_p = (NGAP_PDUSessionResourceModifyItemModReq_t *)ie->value.choice.PDUSessionResourceModifyListModReq.list.array[nb_of_pdusessions_failed];
        
        NGAP_PDUSESSION_MODIFY_RESP(message_p).pdusessions_failed[nb_of_pdusessions_failed].pdusession_id = item_p->pDUSessionID;
        NGAP_PDUSESSION_MODIFY_RESP(message_p).pdusessions_failed[nb_of_pdusessions_failed].cause = NGAP_Cause_PR_radioNetwork;
        NGAP_PDUSESSION_MODIFY_RESP(message_p).pdusessions_failed[nb_of_pdusessions_failed].cause_value = NGAP_CauseRadioNetwork_unknown_local_UE_NGAP_ID;
      }
    } else {
      return -1;
    }

    NGAP_PDUSESSION_MODIFY_RESP(message_p).nb_of_pdusessions_failed = nb_of_pdusessions_failed;
    ngap_gNB_pdusession_modify_resp(amf_desc_p->ngap_gNB_instance->instance,
                               &NGAP_PDUSESSION_MODIFY_RESP(message_p));
    itti_free(TASK_RRC_GNB,message_p);
    message_p = NULL;
    return -1;
  }

  message_p        = itti_alloc_new_message(TASK_NGAP, 0, NGAP_PDUSESSION_MODIFY_REQ);
  NGAP_PDUSESSION_MODIFY_REQ(message_p).ue_initial_id  = ue_desc_p->ue_initial_id;
  NGAP_PDUSESSION_MODIFY_REQ(message_p).amf_ue_ngap_id  = amf_ue_ngap_id;
  NGAP_PDUSESSION_MODIFY_REQ(message_p).gNB_ue_ngap_id  = gnb_ue_ngap_id;
  /* id-PDUSessionResourceModifyListModReq */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceModifyRequestIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_PDUSessionResourceModifyListModReq, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    NGAP_PDUSESSION_MODIFY_REQ(message_p).nb_pdusessions_tomodify =
      ie->value.choice.PDUSessionResourceModifyListModReq.list.count;

    for (i = 0; i < ie->value.choice.PDUSessionResourceModifyListModReq.list.count; i++) {
      NGAP_PDUSessionResourceModifyItemModReq_t         *item_p;
      asn_dec_rval_t                                    dec_rval;
      NGAP_PDUSessionResourceModifyRequestTransfer_t    *pdusessionTransfer_p = NULL;
      NGAP_PDUSessionResourceModifyRequestTransferIEs_t *pdusessionTransfer_ies = NULL;
        
      item_p = (NGAP_PDUSessionResourceModifyItemModReq_t *)ie->value.choice.PDUSessionResourceModifyListModReq.list.array[i];
    
      NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].pdusession_id = item_p->pDUSessionID;

      // check for the NAS PDU
      if (item_p->nAS_PDU != NULL && item_p->nAS_PDU->size > 0 ) {
        NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].nas_pdu.length = item_p->nAS_PDU->size;
        NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].nas_pdu.buffer = malloc(sizeof(uint8_t) * item_p->nAS_PDU->size);
        memcpy(NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].nas_pdu.buffer,
               item_p->nAS_PDU->buf, item_p->nAS_PDU->size);
      } else {
        NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].nas_pdu.length = 0;
        NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].nas_pdu.buffer = NULL;
        continue;
      }

      dec_rval = aper_decode(NULL,
                             &asn_DEF_NGAP_PDUSessionResourceModifyRequestTransfer,
                             (void **)&pdusessionTransfer_p,
                             item_p->pDUSessionResourceModifyRequestTransfer.buf,
                             item_p->pDUSessionResourceModifyRequestTransfer.size, 0, 0);

      if(dec_rval.code != RC_OK) {
        NGAP_ERROR("could not decode PDUSessionResourceModifyRequestTransfer\n");
        return -1;
      }

      for(int j=0; j< pdusessionTransfer_p->protocolIEs.list.count; j++) {
        pdusessionTransfer_ies = pdusessionTransfer_p->protocolIEs.list.array[j];
        switch(pdusessionTransfer_ies->id) {
          /* optional PDUSessionAggregateMaximumBitRate */
          case NGAP_ProtocolIE_ID_id_PDUSessionAggregateMaximumBitRate:
            break;
        
          /* optional UL-NGU-UP-TNLModifyList */
          case NGAP_ProtocolIE_ID_id_UL_NGU_UP_TNLModifyList:
            break;
            
          /* optional NetworkInstance */
          case NGAP_ProtocolIE_ID_id_NetworkInstance:
            break;

          /* optional QosFlowAddOrModifyRequestList */
          case NGAP_ProtocolIE_ID_id_QosFlowAddOrModifyRequestList:
            {
              NGAP_QosFlowAddOrModifyRequestItem_t *qosFlowItem_p;
              
              NGAP_DEBUG("QosFlowAddOrModifyRequestList.list.count %d\n", pdusessionTransfer_ies->value.choice.QosFlowAddOrModifyRequestList.list.count);
              DevAssert(pdusessionTransfer_ies->value.choice.QosFlowAddOrModifyRequestList.list.count > 0);
              DevAssert(pdusessionTransfer_ies->value.choice.QosFlowAddOrModifyRequestList.list.count <= NGAP_maxnoofQosFlows);

              NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].nb_qos = pdusessionTransfer_ies->value.choice.QosFlowAddOrModifyRequestList.list.count;
                
              for(int qosIdx = 0; qosIdx < pdusessionTransfer_ies->value.choice.QosFlowAddOrModifyRequestList.list.count; qosIdx++) {
                qosFlowItem_p = pdusessionTransfer_ies->value.choice.QosFlowAddOrModifyRequestList.list.array[qosIdx];
                
                /* Set the QOS informations */
                NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].qos[qosIdx].qfi = (uint8_t)qosFlowItem_p->qosFlowIdentifier;
                if(qosFlowItem_p->qosFlowLevelQosParameters) {
                  NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].qos[qosIdx].allocation_retention_priority.priority_level =
                    qosFlowItem_p->qosFlowLevelQosParameters->allocationAndRetentionPriority.priorityLevelARP;
                  NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].qos[qosIdx].allocation_retention_priority.pre_emp_capability =
                    qosFlowItem_p->qosFlowLevelQosParameters->allocationAndRetentionPriority.pre_emptionCapability;
                  NGAP_PDUSESSION_MODIFY_REQ(message_p).pdusession_modify_params[i].qos[qosIdx].allocation_retention_priority.pre_emp_vulnerability =
                    qosFlowItem_p->qosFlowLevelQosParameters->allocationAndRetentionPriority.pre_emptionVulnerability;
                }
              }
            }
            break;

          /* optional QosFlowToReleaseList */
          case NGAP_ProtocolIE_ID_id_QosFlowToReleaseList:
            break;

          /* optional AdditionalUL-NGU-UP-TNLInformation */
          case NGAP_ProtocolIE_ID_id_AdditionalUL_NGU_UP_TNLInformation:
            break;

          /* optional CommonNetworkInstance */
          case NGAP_ProtocolIE_ID_id_CommonNetworkInstance:
            break;
            
          default:
            NGAP_ERROR("could not found protocolIEs id %ld\n", pdusessionTransfer_ies->id);
            return -1;
        }
      }
    }

    itti_send_msg_to_task(TASK_RRC_GNB, ue_desc_p->gNB_instance->instance, message_p);
  } else { /* of if (ie != NULL)*/
    return -1;
  }

  return 0;
}
// handle pdu session release command and send it to rrc_end
static
int ngap_gNB_handle_pdusession_release_command(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu) {

  int i;
  ngap_gNB_amf_data_t   *amf_desc_p       = NULL;
  ngap_gNB_ue_context_t *ue_desc_p        = NULL;
  MessageDef            *message_p        = NULL;
  NGAP_PDUSessionResourceReleaseCommand_t     *container;
  NGAP_PDUSessionResourceReleaseCommandIEs_t  *ie;
  NGAP_RAN_UE_NGAP_ID_t           gnb_ue_ngap_id;
  uint64_t                        amf_ue_ngap_id;
  DevAssert(pdu != NULL);
  container = &pdu->choice.initiatingMessage->value.choice.PDUSessionResourceReleaseCommand;

  if ((amf_desc_p = ngap_gNB_get_AMF(NULL, assoc_id, 0)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received pdu session release command for non existing AMF context\n", assoc_id);
    return -1;
  }


  /* id-AMF-UE-NGAP-ID */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceReleaseCommandIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_AMF_UE_NGAP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    asn_INTEGER2ulong(&(ie->value.choice.AMF_UE_NGAP_ID), &amf_ue_ngap_id);
  } else {
    return -1;
  }

  /* id-RAN-UE-NGAP-ID */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceReleaseCommandIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_RAN_UE_NGAP_ID, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    gnb_ue_ngap_id = ie->value.choice.RAN_UE_NGAP_ID;
  } else {
    return -1;
  }

  if ((ue_desc_p = ngap_gNB_get_ue_context(amf_desc_p->ngap_gNB_instance,
                   gnb_ue_ngap_id)) == NULL) {
    NGAP_ERROR("[SCTP %d] Received PDUSession Resource release command for non existing UE context 0x%08lx\n", assoc_id,
               ie->value.choice.RAN_UE_NGAP_ID);
    return -1;
  }

  /* Initial context request = UE-related procedure -> stream != 0 */
  // if (stream == 0) {
  //   NGAP_ERROR("[SCTP %d] Received UE-related procedure on stream (%d)\n",
  //              assoc_id, stream);
  //   return -1;
  // }

  ue_desc_p->rx_stream = stream;

  if (ue_desc_p->amf_ue_ngap_id != amf_ue_ngap_id) {
    NGAP_WARN("UE context amf_ue_ngap_id is different form that of the message (%lu != %lu)",
              (uint64_t)ue_desc_p->amf_ue_ngap_id, amf_ue_ngap_id);
  }

  NGAP_DEBUG("[SCTP %d] Received pdu session release command for gNB_UE_NGAP_ID %lu amf_ue_ngap_id %lu\n",
             assoc_id, gnb_ue_ngap_id, amf_ue_ngap_id);
  message_p = itti_alloc_new_message(TASK_NGAP, 0, NGAP_PDUSESSION_RELEASE_COMMAND);
  NGAP_PDUSESSION_RELEASE_COMMAND(message_p).gNB_ue_ngap_id = gnb_ue_ngap_id;
  NGAP_PDUSESSION_RELEASE_COMMAND(message_p).amf_ue_ngap_id = amf_ue_ngap_id;
  /* id-NAS-PDU */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceReleaseCommandIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_NAS_PDU, false);

  if(ie && ie->value.choice.NAS_PDU.size > 0) {
    NGAP_PDUSESSION_RELEASE_COMMAND(message_p).nas_pdu.length = ie->value.choice.NAS_PDU.size;
    NGAP_PDUSESSION_RELEASE_COMMAND(message_p).nas_pdu.buffer = malloc(sizeof(uint8_t) * ie->value.choice.NAS_PDU.size);
    memcpy(NGAP_PDUSESSION_RELEASE_COMMAND(message_p).nas_pdu.buffer,
           ie->value.choice.NAS_PDU.buf,
           ie->value.choice.NAS_PDU.size);
  } else {
    NGAP_PDUSESSION_RELEASE_COMMAND(message_p).nas_pdu.length = 0;
    NGAP_PDUSESSION_RELEASE_COMMAND(message_p).nas_pdu.buffer = NULL;
  }

  /* id-PDUSessionResourceToReleaseListRelCmd */
  NGAP_FIND_PROTOCOLIE_BY_ID(NGAP_PDUSessionResourceReleaseCommandIEs_t, ie, container,
                             NGAP_ProtocolIE_ID_id_PDUSessionResourceToReleaseListRelCmd, true);

  if (ie != NULL) { /* checked by macro but cppcheck doesn't see it */
    NGAP_PDUSESSION_RELEASE_COMMAND(message_p).nb_pdusessions_torelease = ie->value.choice.PDUSessionResourceToReleaseListRelCmd.list.count;

    for (i = 0; i < ie->value.choice.PDUSessionResourceToReleaseListRelCmd.list.count; i++) {
      NGAP_PDUSessionResourceToReleaseItemRelCmd_t *item_p;
      item_p = (NGAP_PDUSessionResourceToReleaseItemRelCmd_t *)ie->value.choice.PDUSessionResourceToReleaseListRelCmd.list.array[i];
    
      NGAP_PDUSESSION_RELEASE_COMMAND(message_p).pdusession_release_params[i].pdusession_id = item_p->pDUSessionID;
      if(item_p->pDUSessionResourceReleaseCommandTransfer.size > 0) {
        NGAP_PDUSESSION_RELEASE_COMMAND(message_p).pdusession_release_params[i].transfer_length = item_p->pDUSessionResourceReleaseCommandTransfer.size;
        NGAP_PDUSESSION_RELEASE_COMMAND(message_p).pdusession_release_params[i].transfer_buffer = malloc(sizeof(uint8_t) * item_p->pDUSessionResourceReleaseCommandTransfer.size);
        memcpy(NGAP_PDUSESSION_RELEASE_COMMAND(message_p).pdusession_release_params[i].transfer_buffer,
               item_p->pDUSessionResourceReleaseCommandTransfer.buf,
               item_p->pDUSessionResourceReleaseCommandTransfer.size);
      }else {
        NGAP_PDUSESSION_RELEASE_COMMAND(message_p).pdusession_release_params[i].transfer_length = 0;
        NGAP_PDUSESSION_RELEASE_COMMAND(message_p).pdusession_release_params[i].transfer_buffer = NULL;
        NGAP_ERROR("[NGAP] Received pdu session release command for pDUSessionResourceReleaseCommandTransfer is NULL!\n");
      }
      NGAP_DEBUG("[NGAP] Received pdu session release command for pDUSessionID id %ld\n", item_p->pDUSessionID);
    }
  } else {
    return -1;
  }

  itti_send_msg_to_task(TASK_RRC_GNB, ue_desc_p->gNB_instance->instance, message_p);

  return 0;
}

static
int ngap_gNB_handle_ng_path_switch_request_ack(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu) {
  // TODO
  return 0;
}

static
int ngap_gNB_handle_ng_path_switch_request_failure(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu) {

  // TODO
  return 0;
}

static
int ngap_gNB_handle_ng_ENDC_pdusession_modification_confirm(uint32_t               assoc_id,
    uint32_t               stream,
    NGAP_NGAP_PDU_t       *pdu){

	LOG_W(NGAP, "Implementation of NGAP Pdusession Modification confirm handler is pending...\n");
	return 0;
}

