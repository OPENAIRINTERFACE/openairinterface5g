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

/*
                                play_scenario_s1ap.c
                                -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <crypt.h>
#include <sys/time.h>
#include "tree.h"
#include "queue.h"


#include "intertask_interface.h"
#include "platform_types.h"
#include "assertions.h"
#include "conversions.h"
#include "s1ap_common.h"
#include "play_scenario_s1ap_eNB_defs.h"
#include "play_scenario.h"
#include "msc.h"
//------------------------------------------------------------------------------
s1ap_eNB_internal_data_t s1ap_eNB_internal_data;
RB_GENERATE(s1ap_mme_map, s1ap_eNB_mme_data_s, entry, et_s1ap_eNB_compare_assoc_id);
//------------------------------------------------------------------------------
extern et_scenario_t  *g_scenario;
extern uint32_t        g_constraints;
//------------------------------------------------------------------------------
int et_s1ap_eNB_compare_assoc_id(
  struct s1ap_eNB_mme_data_s *p1, struct s1ap_eNB_mme_data_s *p2)
{
  if (p1->assoc_id == -1) {
    if (p1->cnx_id < p2->cnx_id) {
      return -1;
    }
    if (p1->cnx_id > p2->cnx_id) {
      return 1;
    }
  } else {
    if (p1->assoc_id < p2->assoc_id) {
      return -1;
    }
    if (p1->assoc_id > p2->assoc_id) {
      return 1;
    }
  }

  /* Matching reference */
  return 0;
}
//------------------------------------------------------------------------------
uint32_t et_s1ap_generate_eNB_id(void)
{
  char    *out;
  char     hostname[50];
  int      ret;
  uint32_t eNB_id;

  /* Retrieve the host name */
  ret = gethostname(hostname, sizeof(hostname));
  DevAssert(ret == 0);

  out = crypt(hostname, "eurecom");
  DevAssert(out != NULL);

  eNB_id = ((out[0] << 24) | (out[1] << 16) | (out[2] << 8) | out[3]);

  return eNB_id;
}
//------------------------------------------------------------------------------
uint16_t et_s1ap_eNB_fetch_add_global_cnx_id(void)
{
  return ++s1ap_eNB_internal_data.global_cnx_id;
}

//------------------------------------------------------------------------------
void et_s1ap_eNB_prepare_internal_data(void)
{
  memset(&s1ap_eNB_internal_data, 0, sizeof(s1ap_eNB_internal_data));
  STAILQ_INIT(&s1ap_eNB_internal_data.s1ap_eNB_instances_head);
}

//------------------------------------------------------------------------------
void et_s1ap_eNB_insert_new_instance(s1ap_eNB_instance_t *new_instance_p)
{
  DevAssert(new_instance_p != NULL);

  STAILQ_INSERT_TAIL(&s1ap_eNB_internal_data.s1ap_eNB_instances_head,
                     new_instance_p, s1ap_eNB_entries);
}

//------------------------------------------------------------------------------
struct s1ap_eNB_mme_data_s *et_s1ap_eNB_get_MME(
  s1ap_eNB_instance_t *instance_p,
  int32_t assoc_id, uint16_t cnx_id)
{
  struct s1ap_eNB_mme_data_s  temp;
  struct s1ap_eNB_mme_data_s *found;

  memset(&temp, 0, sizeof(struct s1ap_eNB_mme_data_s));

  temp.assoc_id = assoc_id;
  temp.cnx_id   = cnx_id;

  if (instance_p == NULL) {
    STAILQ_FOREACH(instance_p, &s1ap_eNB_internal_data.s1ap_eNB_instances_head,
                   s1ap_eNB_entries) {
      found = RB_FIND(s1ap_mme_map, &instance_p->s1ap_mme_head, &temp);

      if (found != NULL) {
        return found;
      }
    }
  } else {
    return RB_FIND(s1ap_mme_map, &instance_p->s1ap_mme_head, &temp);
  }

  return NULL;
}

//------------------------------------------------------------------------------
s1ap_eNB_instance_t *et_s1ap_eNB_get_instance(instance_t instance)
{
  s1ap_eNB_instance_t *temp = NULL;

  STAILQ_FOREACH(temp, &s1ap_eNB_internal_data.s1ap_eNB_instances_head,
                 s1ap_eNB_entries) {
    if (temp->instance == instance) {
      /* Matching occurence */
      return temp;
    }
  }

  return NULL;
}
//------------------------------------------------------------------------------
void et_s1ap_eNB_itti_send_sctp_data_req(instance_t instance, int32_t assoc_id, uint8_t *buffer,
                                      uint32_t buffer_length, uint16_t stream)
{
  MessageDef      *message_p;
  sctp_data_req_t *sctp_data_req;

  message_p = itti_alloc_new_message(TASK_S1AP, SCTP_DATA_REQ);

  sctp_data_req = &message_p->ittiMsg.sctp_data_req;

  sctp_data_req->assoc_id      = assoc_id;
  sctp_data_req->buffer        = buffer;
  sctp_data_req->buffer_length = buffer_length;
  sctp_data_req->stream        = stream;

  itti_send_msg_to_task(TASK_SCTP, instance, message_p);
}
//------------------------------------------------------------------------------
int et_handle_s1ap_mismatch_mme_ue_s1ap_id(et_packet_t * const spacket, et_packet_t * const rx_packet)
{
  S1AP_MME_UE_S1AP_ID_t scenario_mme_ue_s1ap_id = 0;
  S1AP_MME_UE_S1AP_ID_t rx_mme_ue_s1ap_id       = 0;
  S1AP_PDU_PR           present;

  present = rx_packet->sctp_hdr.u.data_hdr.payload.pdu.present;

  switch (rx_packet->sctp_hdr.u.data_hdr.payload.message.procedureCode) {
    case S1AP_ProcedureCode_id_HandoverPreparation:
      if (present == S1AP_S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverRequiredIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverRequiredIEs.mme_ue_s1ap_id;
      } else {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverCommandIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverCommandIEs.mme_ue_s1ap_id;
      }
      break;

    case S1AP_ProcedureCode_id_HandoverResourceAllocation:
      if (present == S1AP_S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverRequestIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverRequestIEs.mme_ue_s1ap_id;
      } else if (present == S1AP_S1AP_PDU_PR_successfulOutcome) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverRequestAcknowledgeIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverRequestAcknowledgeIEs.mme_ue_s1ap_id;
      } else if (present == S1AP_S1AP_PDU_PR_unsuccessfulOutcome) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverFailureIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverFailureIEs.mme_ue_s1ap_id;
      }
      break;

    case S1AP_ProcedureCode_id_HandoverNotification:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverNotifyIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverNotifyIEs.mme_ue_s1ap_id;
      break;

    case S1AP_ProcedureCode_id_PathSwitchRequest:
      //rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_PathSwitchRequestIEs.mme_ue_s1ap_id;
      //scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_PathSwitchRequestIEs.mme_ue_s1ap_id;
      break;

    case S1AP_ProcedureCode_id_HandoverCancel:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverCancelIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_HandoverCancelIEs.mme_ue_s1ap_id;
      break;

    case  S1AP_ProcedureCode_id_E_RABSetup:
      if (present == S1AP_S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABSetupRequestIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABSetupRequestIEs.mme_ue_s1ap_id;
      } else  {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABSetupResponseIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABSetupResponseIEs.mme_ue_s1ap_id;
      }
      break;

    case  S1ap_ProcedureCode_id_E_RABModify:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABModifyRequestIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABModifyRequestIEs.mme_ue_s1ap_id;
      } else  {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABModifyResponseIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABModifyResponseIEs.mme_ue_s1ap_id;
      }
      break;

    case  S1ap_ProcedureCode_id_E_RABRelease:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABReleaseCommandIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABReleaseCommandIEs.mme_ue_s1ap_id;
      } else  {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABReleaseResponseIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABReleaseResponseIEs.mme_ue_s1ap_id;
      }
      break;

    case  S1ap_ProcedureCode_id_E_RABReleaseIndication:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABReleaseIndicationIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_E_RABReleaseIndicationIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_InitialContextSetup:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_InitialContextSetupRequestIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_InitialContextSetupRequestIEs.mme_ue_s1ap_id;
      } else  {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_InitialContextSetupResponseIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_InitialContextSetupResponseIEs.mme_ue_s1ap_id;
      }
      break;

    case  S1ap_ProcedureCode_id_Paging:
      //rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_PagingIEs.mme_ue_s1ap_id;
      //scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_PagingIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_downlinkNASTransport:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_DownlinkNASTransportIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_DownlinkNASTransportIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_initialUEMessage:
      //rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_InitialUEMessageIEs.mme_ue_s1ap_id;
      //scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_InitialUEMessageIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_uplinkNASTransport:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UplinkNASTransportIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UplinkNASTransportIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_Reset:
      //rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ResetIEs.mme_ue_s1ap_id;
      //scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ResetIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_ErrorIndication:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ErrorIndicationIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ErrorIndicationIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_NASNonDeliveryIndication:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_NASNonDeliveryIndication_IEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_NASNonDeliveryIndication_IEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_S1Setup:
      /*if (present == S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_S1SetupRequestIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_S1SetupRequestIEs.mme_ue_s1ap_id;
      } else if (present == S1AP_PDU_PR_successfulOutcome) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_S1SetupResponseIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_S1SetupResponseIEs.mme_ue_s1ap_id;
      } else if (present == S1AP_PDU_PR_unsuccessfulOutcome) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_S1SetupFailureIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_S1SetupFailureIEs.mme_ue_s1ap_id;
      }*/
      break;

    case  S1ap_ProcedureCode_id_UEContextReleaseRequest:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextReleaseRequestIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextReleaseRequestIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_DownlinkS1cdma2000tunneling:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_DownlinkS1cdma2000tunnelingIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_DownlinkS1cdma2000tunnelingIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_UplinkS1cdma2000tunneling:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UplinkS1cdma2000tunnelingIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UplinkS1cdma2000tunnelingIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_UEContextModification:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextModificationRequestIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextModificationRequestIEs.mme_ue_s1ap_id;
      } else if (present == S1AP_PDU_PR_successfulOutcome) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextModificationResponseIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextModificationResponseIEs.mme_ue_s1ap_id;
      } else if (present == S1AP_PDU_PR_unsuccessfulOutcome) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextModificationFailureIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextModificationFailureIEs.mme_ue_s1ap_id;
      }
      break;

    case  S1ap_ProcedureCode_id_UECapabilityInfoIndication:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UECapabilityInfoIndicationIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UECapabilityInfoIndicationIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_UEContextRelease:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        switch (rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextReleaseCommandIEs.uE_S1AP_IDs.present) {
          case S1ap_UE_S1AP_IDs_PR_uE_S1AP_ID_pair:
            rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextReleaseCommandIEs.uE_S1AP_IDs.choice.uE_S1AP_ID_pair.mME_UE_S1AP_ID;
            scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextReleaseCommandIEs.uE_S1AP_IDs.choice.uE_S1AP_ID_pair.mME_UE_S1AP_ID;
            break;
          case S1ap_UE_S1AP_IDs_PR_mME_UE_S1AP_ID:
            rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextReleaseCommandIEs.uE_S1AP_IDs.choice.mME_UE_S1AP_ID;
            scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextReleaseCommandIEs.uE_S1AP_IDs.choice.mME_UE_S1AP_ID;
            break;
        }
      } else  {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextReleaseCompleteIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UEContextReleaseCompleteIEs.mme_ue_s1ap_id;
      }
      break;

    case  S1ap_ProcedureCode_id_eNBStatusTransfer:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ENBStatusTransferIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ENBStatusTransferIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_MMEStatusTransfer:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_MMEStatusTransferIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_MMEStatusTransferIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_DeactivateTrace:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_DeactivateTraceIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_DeactivateTraceIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_TraceStart:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_TraceStartIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_TraceStartIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_TraceFailureIndication:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_TraceFailureIndicationIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_TraceFailureIndicationIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_ENBConfigurationUpdate:
      /*if (present == S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ENBConfigurationUpdateIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ENBConfigurationUpdateIEs.mme_ue_s1ap_id;
      } else if (present == S1AP_PDU_PR_successfulOutcome) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ENBConfigurationUpdateAcknowledgeIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ENBConfigurationUpdateAcknowledgeIEs.mme_ue_s1ap_id;
      } else {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ENBConfigurationUpdateFailureIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ENBConfigurationUpdateFailureIEs.mme_ue_s1ap_id;
      }*/
      break;

    case  S1ap_ProcedureCode_id_MMEConfigurationUpdate:
      /*if (present == S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_MMEConfigurationUpdateIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_MMEConfigurationUpdateIEs.mme_ue_s1ap_id;
      } else if (present == S1AP_PDU_PR_successfulOutcome) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_MMEConfigurationUpdateAcknowledgeIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_MMEConfigurationUpdateAcknowledgeIEs.mme_ue_s1ap_id;
      } else {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_MMEConfigurationUpdateFailureIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_MMEConfigurationUpdateFailureIEs.mme_ue_s1ap_id;
      }*/
      break;

    case  S1ap_ProcedureCode_id_LocationReportingControl:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_LocationReportingControlIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_LocationReportingControlIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_LocationReportingFailureIndication:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_LocationReportingFailureIndicationIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_LocationReportingFailureIndicationIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_LocationReport:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_LocationReportIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_LocationReportIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_OverloadStart:
      //rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_OverloadStartIEs.mme_ue_s1ap_id;
      //scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_OverloadStartIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_OverloadStop:
      //rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_OverloadStopIEs.mme_ue_s1ap_id;
      //scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_OverloadStopIEs.mme_ue_s1ap_id;
      break;

    case  S1ap_ProcedureCode_id_WriteReplaceWarning:
      /*if (present == S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_WriteReplaceWarningRequestIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_WriteReplaceWarningRequestIEs.mme_ue_s1ap_id;
      } else  {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_WriteReplaceWarningResponseIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_WriteReplaceWarningResponseIEs.mme_ue_s1ap_id;
      }*/
      break;

    case S1AP_ProcedureCode_id_eNBDirectInformationTransfer:
      //rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ENBDirectInformationTransferIEs.mme_ue_s1ap_id;
      //scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_ENBDirectInformationTransferIEs.mme_ue_s1ap_id;
      break;

    case S1AP_ProcedureCode_id_MMEDirectInformationTransfer:
      //rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_MMEDirectInformationTransferIEs.mme_ue_s1ap_id;
      //scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_MMEDirectInformationTransferIEs.mme_ue_s1ap_id;
      break;

    case S1AP_ProcedureCode_id_PrivateMessage:
    case S1AP_ProcedureCode_id_eNBConfigurationTransfer:
    case S1AP_ProcedureCode_id_MMEConfigurationTransfer:
      AssertFatal(0, "TODO");
      break;

    case S1AP_ProcedureCode_id_CellTrafficTrace:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_CellTrafficTraceIEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_CellTrafficTraceIEs.mme_ue_s1ap_id;
      break;

    case S1AP_ProcedureCode_id_Kill:
      /*if (present == S1AP_S1AP_PDU_PR_initiatingMessage) {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_KillRequestIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_KillRequestIEs.mme_ue_s1ap_id;
      } else  {
        rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_KillResponseIEs.mme_ue_s1ap_id;
        scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_KillResponseIEs.mme_ue_s1ap_id;
      }*/
      break;

    case S1AP_ProcedureCode_id_downlinkUEAssociatedLPPaTransport:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_DownlinkUEAssociatedLPPaTransport_IEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_DownlinkUEAssociatedLPPaTransport_IEs.mme_ue_s1ap_id;
      break;

    case S1AP_ProcedureCode_id_uplinkUEAssociatedLPPaTransport:
      rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UplinkUEAssociatedLPPaTransport_IEs.mme_ue_s1ap_id;
      scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UplinkUEAssociatedLPPaTransport_IEs.mme_ue_s1ap_id;
      break;

    case S1AP_ProcedureCode_id_downlinkNonUEAssociatedLPPaTransport:
      //rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_DownlinkNonUEAssociatedLPPaTransport_IEs.mme_ue_s1ap_id;
      //scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_DownlinkNonUEAssociatedLPPaTransport_IEs.mme_ue_s1ap_id;
      break;

    case S1AP_ProcedureCode_id_uplinkNonUEAssociatedLPPaTransport:
      //rx_mme_ue_s1ap_id       = rx_packet->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UplinkNonUEAssociatedLPPaTransport_IEs.mme_ue_s1ap_id;
      //scenario_mme_ue_s1ap_id = spacket->sctp_hdr.u.data_hdr.payload.message.msg.s1ap_UplinkNonUEAssociatedLPPaTransport_IEs.mme_ue_s1ap_id;
      break;

    default:
      AssertFatal(0, "Unknown procedure code %ld", rx_packet->sctp_hdr.u.data_hdr.payload.message.procedureCode);
  }

  if (scenario_mme_ue_s1ap_id != rx_mme_ue_s1ap_id) {
    S1AP_DEBUG("%s() Updating  mme_ue_s1ap_id %u -> %u \n", __FUNCTION__, scenario_mme_ue_s1ap_id, rx_mme_ue_s1ap_id);
    et_packet_t *p = spacket;

    while (p) {
      et_s1ap_update_mme_ue_s1ap_id(p, scenario_mme_ue_s1ap_id, rx_mme_ue_s1ap_id);
      p = p->next;
    }
    return 0;
  }
  return 1;
}
//------------------------------------------------------------------------------
asn_comp_rval_t * et_s1ap_is_matching(et_s1ap_t * const s1ap1, et_s1ap_t * const s1ap2, const uint32_t constraints)
{
  asn_comp_rval_t *rv = NULL;
  if (s1ap1->pdu.present != s1ap2->pdu.present)  {rv  = calloc(1, sizeof(asn_comp_rval_t)); rv->err_code = ET_ERROR_MATCH_PACKET_S1AP_PRESENT; return rv;}
  switch (s1ap1->pdu.present) {
    case  S1AP_PDU_PR_NOTHING:
      break;
    case  S1AP_PDU_PR_initiatingMessage:
      if (s1ap1->pdu.choice.initiatingMessage.procedureCode != s1ap2->pdu.choice.initiatingMessage.procedureCode)
        {rv  = calloc(1, sizeof(asn_comp_rval_t)); rv->err_code = ET_ERROR_MATCH_PACKET_S1AP_PROCEDURE_CODE; return rv;}
      if (s1ap1->pdu.choice.initiatingMessage.criticality != s1ap2->pdu.choice.initiatingMessage.criticality)
        {rv  = calloc(1, sizeof(asn_comp_rval_t)); rv->err_code = ET_ERROR_MATCH_PACKET_S1AP_CRITICALITY; return rv;}
      break;
    case  S1AP_PDU_PR_successfulOutcome:
      if (s1ap1->pdu.choice.successfulOutcome.procedureCode != s1ap2->pdu.choice.successfulOutcome.procedureCode)
        {rv  = calloc(1, sizeof(asn_comp_rval_t)); rv->err_code = ET_ERROR_MATCH_PACKET_S1AP_PROCEDURE_CODE; return rv;}
      if (s1ap1->pdu.choice.successfulOutcome.criticality != s1ap2->pdu.choice.successfulOutcome.criticality)
        {rv  = calloc(1, sizeof(asn_comp_rval_t)); rv->err_code = ET_ERROR_MATCH_PACKET_S1AP_CRITICALITY; return rv;}
      break;
    case  S1AP_PDU_PR_unsuccessfulOutcome:
      if (s1ap1->pdu.choice.unsuccessfulOutcome.procedureCode != s1ap2->pdu.choice.unsuccessfulOutcome.procedureCode)
        {rv  = calloc(1, sizeof(asn_comp_rval_t)); rv->err_code = ET_ERROR_MATCH_PACKET_S1AP_PROCEDURE_CODE; return rv;}
      if (s1ap1->pdu.choice.unsuccessfulOutcome.criticality != s1ap2->pdu.choice.unsuccessfulOutcome.criticality)
        {rv  = calloc(1, sizeof(asn_comp_rval_t)); rv->err_code = ET_ERROR_MATCH_PACKET_S1AP_CRITICALITY; return rv;}
      break;
    default:
      AssertFatal(0, "Unknown pdu.present %d", s1ap1->pdu.present);
  }

  if (s1ap1->binary_stream_allocated_size == s1ap2->binary_stream_allocated_size) {
    if (memcmp((void*)s1ap1->binary_stream, (void*)s1ap2->binary_stream, s1ap1->binary_stream_allocated_size) ==  0) return NULL;
  }
  // if no matching, may be the scenario need minor corrections (same enb_ue_s1ap_id but need to update mme_ue_s1ap_id)
  return et_s1ap_ies_is_matching(s1ap1->pdu.present, &s1ap1->message, &s1ap2->message, constraints);
}

//------------------------------------------------------------------------------
et_packet_t* et_build_packet_from_s1ap_data_ind(et_event_s1ap_data_ind_t * const s1ap_data_ind)
{
  et_packet_t     * packet    = NULL;
  AssertFatal (NULL != s1ap_data_ind, "Bad parameter sctp_data_ind\n");
  packet = calloc(1, sizeof(*packet));
  packet->action                                        = ET_PACKET_ACTION_S1C_NULL;
  //packet->time_relative_to_first_packet.tv_sec          = 0;
  //packet->time_relative_to_first_packet.tv_usec         = 0;
  //packet->time_relative_to_last_sent_packet.tv_sec      = 0;
  //packet->time_relative_to_last_sent_packet.tv_usec     = 0;
  //packet->time_relative_to_last_received_packet.tv_sec  = 0;
  //packet->time_relative_to_last_received_packet.tv_usec = 0;
  //packet->original_frame_number                         = 0;
  //packet->packet_number                                 = 0;
  packet->enb_instance = 0; //TODO
  //packet->ip_hdr;
  // keep in mind: allocated buffer: sctp_datahdr.payload.binary_stream
  packet->sctp_hdr.chunk_type = SCTP_CID_DATA;
  memcpy((void*)&packet->sctp_hdr.u.data_hdr, (void*)&s1ap_data_ind->sctp_datahdr, sizeof(packet->sctp_hdr));
  //packet->next = NULL;
  packet->status = ET_PACKET_STATUS_RECEIVED;
  //packet->timer_id = 0;
  AssertFatal(0 == gettimeofday(&packet->timestamp_packet, NULL), "gettimeofday() Failed");
  return packet;
}


//------------------------------------------------------------------------------
// return 0 if packet was waited
int et_scenario_set_packet_received(et_packet_t * const packet)
{
  et_packet_t * p = NULL;
  int           rc = 0;

  packet->status = ET_PACKET_STATUS_RECEIVED;
  S1AP_DEBUG("Packet received:          num %u  | original frame number %u \n", packet->packet_number, packet->original_frame_number);
  S1AP_DEBUG("Last Packet received:     num %u  | original frame number %u \n", g_scenario->last_rx_packet->packet_number, g_scenario->last_rx_packet->original_frame_number);

  p = g_scenario->last_rx_packet;
  while (NULL != p) {
    if (ET_PACKET_ACTION_S1C_RECEIVE == p->action) {
      if ((ET_PACKET_STATUS_RECEIVED == p->status) || (ET_PACKET_STATUS_NOT_TAKEN_IN_ACCOUNT == p->status)) {
        g_scenario->last_rx_packet = p;
        g_scenario->time_last_rx_packet.tv_sec   = p->timestamp_packet.tv_sec;
        g_scenario->time_last_rx_packet.tv_usec  = p->timestamp_packet.tv_usec;
        S1AP_DEBUG("Set Last Packet received: num %u  | original frame number %u \n", g_scenario->last_rx_packet->packet_number, g_scenario->last_rx_packet->original_frame_number);
        S1AP_DEBUG("Set time_last_rx_packet %ld.%06d\n", g_scenario->time_last_rx_packet.tv_sec, g_scenario->time_last_rx_packet.tv_usec);
      } else {
        break;
      }
    }
    p = p->next;
  }

  if (0 != packet->timer_id) {
    rc = timer_remove(packet->timer_id);
    AssertFatal(rc == 0, "TODO: Debug Timer on Rx packet num %u unknown", packet->packet_number);
    g_scenario->timer_count--;
    return rc;
  }
  return 1;
}

//------------------------------------------------------------------------------
int et_s1ap_process_rx_packet(et_event_s1ap_data_ind_t * const s1ap_data_ind)
{
  et_packet_t      *packet       = NULL;
  et_packet_t      *rx_packet    = NULL;
  unsigned long int not_found    = 1;
  asn_comp_rval_t  *comp_results = NULL;
  asn_comp_rval_t  *comp_results2 = NULL;
  unsigned char     error_code   = 0;

  AssertFatal (NULL != s1ap_data_ind, "Bad parameter sctp_data_ind\n");
  rx_packet = et_build_packet_from_s1ap_data_ind(s1ap_data_ind);

  if (NULL == g_scenario->last_rx_packet) {
    packet = g_scenario->list_packet;
    while (NULL != packet) {
      if (packet->action == ET_PACKET_ACTION_S1C_RECEIVE) {
        if ((ET_PACKET_STATUS_RECEIVED == packet->status) || (ET_PACKET_STATUS_NOT_TAKEN_IN_ACCOUNT == packet->status)) {
          g_scenario->last_rx_packet = packet;
          if  (ET_PACKET_STATUS_NOT_TAKEN_IN_ACCOUNT != packet->status) {
            g_scenario->time_last_rx_packet.tv_sec  = packet->timestamp_packet.tv_sec;
            g_scenario->time_last_rx_packet.tv_usec = packet->timestamp_packet.tv_usec;
          }
          S1AP_DEBUG("Set Last Packet received: num %u  | original frame number %u \n", g_scenario->last_rx_packet->packet_number, g_scenario->last_rx_packet->original_frame_number);
          S1AP_DEBUG("Set time_last_rx_packet %ld.%06d\n", g_scenario->time_last_rx_packet.tv_sec, g_scenario->time_last_rx_packet.tv_usec);
        } else {
          break;
        }
      }
      packet = packet->next;
    }
    packet = g_scenario->list_packet;
  } else {
    packet = g_scenario->last_rx_packet->next;
  }
  // not_found threshold may sure depend on number of mme, may be not sure on number of UE
  while ((NULL != packet) && (not_found < 9)) {
    S1AP_DEBUG("%s() Considering packet num %d original frame number %u\n", __FUNCTION__, packet->packet_number, packet->original_frame_number);
    if (packet->action == ET_PACKET_ACTION_S1C_RECEIVE) {
      comp_results = et_sctp_is_matching(&packet->sctp_hdr, &rx_packet->sctp_hdr, g_constraints);
      if (NULL == comp_results) {
        S1AP_DEBUG("Compare RX packet with packet: num %u  | original frame number %u \n", packet->packet_number, packet->original_frame_number);
        packet->timestamp_packet.tv_sec = rx_packet->timestamp_packet.tv_sec;
        packet->timestamp_packet.tv_usec = rx_packet->timestamp_packet.tv_usec;
        return et_scenario_set_packet_received(packet);
      } else {
        S1AP_DEBUG("Compare RX packet with packet: num %u  | original frame number %u failed\n",
          packet->packet_number, packet->original_frame_number);
        while (comp_results) {
          S1AP_DEBUG("Result err code %s(%u) ASN1 struct name %s\n",
                     et_error_match2str(comp_results->err_code), comp_results->err_code, comp_results->name);

          // (each asn1 rc <= 166 (enum e_S1AP_ProtocolIE_ID, in generated file S1AP_ProtocolIE_ID.h))
          if (comp_results->err_code == COMPARE_ERR_CODE_NO_MATCH) {
            //TODO MME_UE_S1AP_ID, etc.
            // get latest error code
            if (strcmp(comp_results->name, "S1ap-MME-UE-S1AP-ID") == 0) {
              if (0 == et_handle_s1ap_mismatch_mme_ue_s1ap_id((et_packet_t *const)packet, (et_packet_t *const)rx_packet)) {
                packet->timestamp_packet.tv_sec = rx_packet->timestamp_packet.tv_sec;
                packet->timestamp_packet.tv_usec = rx_packet->timestamp_packet.tv_usec;
                return et_scenario_set_packet_received(packet);
              }
            } else if (strcmp(comp_results->name, "S1ap-TransportLayerAddress") == 0) {
              S1AP_WARN("Some work needed there for %s, TODO in generic_scenario.xsl, add sgw conf file in the process, anyway continuing...\n",comp_results->name);
              packet->timestamp_packet.tv_sec = rx_packet->timestamp_packet.tv_sec;
              packet->timestamp_packet.tv_usec = rx_packet->timestamp_packet.tv_usec;
              return et_scenario_set_packet_received(packet);
            } else {
              S1AP_WARN("\n\nRX PACKET:\n");
              et_display_packet_sctp(&rx_packet->sctp_hdr);
              S1AP_WARN("\n\nWAITED PACKET:\n");
              et_display_packet_sctp(&packet->sctp_hdr);
              AssertFatal(0,"Some work needed there");
            }
          }
          comp_results2 = comp_results;
          comp_results = comp_results2->next;
          et_free_pointer(comp_results2);
        }
      }
    }
    not_found += 1;
    packet = packet->next;
  }
  et_display_packet_sctp(&rx_packet->sctp_hdr);
  AssertFatal(0, "Rx packet not found in scenario (see dump above)");
  return -1;
}

//------------------------------------------------------------------------------
void et_s1ap_eNB_handle_sctp_data_ind(sctp_data_ind_t * const sctp_data_ind)
{
  int            result = 0;
  et_event_t     event;

  DevAssert(sctp_data_ind != NULL);

  memset((void*)&event, 0, sizeof(event));

  event.code = ET_EVENT_RX_S1AP;
  event.u.s1ap_data_ind.sctp_datahdr.tsn                       = 0;
  event.u.s1ap_data_ind.sctp_datahdr.stream                    = sctp_data_ind->stream;
  event.u.s1ap_data_ind.sctp_datahdr.ssn                       = 0;
  event.u.s1ap_data_ind.sctp_datahdr.ppid                      = S1AP_SCTP_PPID;
  event.u.s1ap_data_ind.sctp_datahdr.assoc_id                  = sctp_data_ind->assoc_id;

  event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream_pos = 0;
  event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream_allocated_size = sctp_data_ind->buffer_length;
  event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream = NULL;
  if ((sctp_data_ind->buffer_length > 0) && (NULL != sctp_data_ind->buffer)) {
    event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream   = calloc(1, sctp_data_ind->buffer_length);
    memcpy(event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream,
           sctp_data_ind->buffer,
           sctp_data_ind->buffer_length);

    if (et_s1ap_decode_pdu(
           &event.u.s1ap_data_ind.sctp_datahdr.payload.pdu,
           &event.u.s1ap_data_ind.sctp_datahdr.payload.message,
           event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream,
           event.u.s1ap_data_ind.sctp_datahdr.payload.binary_stream_allocated_size) < 0) {
      AssertFatal (0, "ERROR Cannot decode RX S1AP message!\n");
    }

  }

  result = itti_free(TASK_UNKNOWN, sctp_data_ind->buffer);
  AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);

  et_scenario_fsm_notify_event(event);

  memset((void*)&event, 0, sizeof(event));
  event.code = ET_EVENT_TICK;
  et_scenario_fsm_notify_event(event);

}
//------------------------------------------------------------------------------
void et_s1ap_eNB_register_mme(s1ap_eNB_instance_t *instance_p,
                                  net_ip_address_t    *mme_ip_address,
                                  net_ip_address_t    *local_ip_addr,
                                  uint16_t             in_streams,
                                  uint16_t             out_streams)
{
  MessageDef                 *message_p                   = NULL;
  sctp_new_association_req_t *sctp_new_association_req_p  = NULL;
  s1ap_eNB_mme_data_t        *s1ap_mme_data_p             = NULL;

  DevAssert(instance_p != NULL);
  DevAssert(mme_ip_address != NULL);

  message_p = itti_alloc_new_message(TASK_S1AP, SCTP_NEW_ASSOCIATION_REQ);

  sctp_new_association_req_p = &message_p->ittiMsg.sctp_new_association_req;

  sctp_new_association_req_p->port = S1AP_PORT_NUMBER;
  sctp_new_association_req_p->ppid = S1AP_SCTP_PPID;

  sctp_new_association_req_p->in_streams  = in_streams;
  sctp_new_association_req_p->out_streams = out_streams;

  memcpy(&sctp_new_association_req_p->remote_address,
         mme_ip_address,
         sizeof(*mme_ip_address));

  memcpy(&sctp_new_association_req_p->local_address,
         local_ip_addr,
         sizeof(*local_ip_addr));

  /* Create new MME descriptor */
  s1ap_mme_data_p = calloc(1, sizeof(*s1ap_mme_data_p));
  DevAssert(s1ap_mme_data_p != NULL);

  s1ap_mme_data_p->cnx_id                = et_s1ap_eNB_fetch_add_global_cnx_id();
  sctp_new_association_req_p->ulp_cnx_id = s1ap_mme_data_p->cnx_id;

  s1ap_mme_data_p->assoc_id          = -1;
  s1ap_mme_data_p->s1ap_eNB_instance = instance_p;

  memcpy((void*)&s1ap_mme_data_p->mme_net_ip_address, mme_ip_address, sizeof(*mme_ip_address));


  STAILQ_INIT(&s1ap_mme_data_p->served_gummei);

  /* Insert the new descriptor in list of known MME
   * but not yet associated.
   */
  RB_INSERT(s1ap_mme_map, &instance_p->s1ap_mme_head, s1ap_mme_data_p);
  s1ap_mme_data_p->state = S1AP_ENB_STATE_WAITING;
  instance_p->s1ap_mme_nb ++;
  instance_p->s1ap_mme_pending_nb ++;

  itti_send_msg_to_task(TASK_SCTP, instance_p->instance, message_p);
}
//------------------------------------------------------------------------------
void et_s1ap_update_assoc_id_of_packets(const int32_t assoc_id,
                                        struct s1ap_eNB_instance_s * const s1ap_eNB_instance,
                                        s1ap_eNB_mme_data_t        * const mme_desc_p)
{
  et_packet_t     *packet = NULL;
  int              ret;
  unsigned int     old_enb_port = 0;
  unsigned int     old_mme_port = 0;

  S1AP_DEBUG("%s for SCTP association (%u)\n",__FUNCTION__,assoc_id);

  packet = g_scenario->list_packet;
  while (NULL != packet) {
    switch (packet->sctp_hdr.chunk_type) {

      case SCTP_CID_DATA :
        S1AP_DEBUG("%s for SCTP association (%u) SCTP_CID_DATA\n",__FUNCTION__,assoc_id);
        if ((ET_PACKET_STATUS_NONE == packet->status) || (ET_PACKET_STATUS_SCHEDULED_FOR_RECEIVING == packet->status)) {
          if (0 < old_mme_port) {
            if (packet->action == ET_PACKET_ACTION_S1C_SEND) {
              ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.dst, &mme_desc_p->mme_net_ip_address);
              if (0 == ret) {
                ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.src, &s1ap_eNB_instance->s1c_net_ip_address);
                if (0 == ret) {
                  // same IP src, same IP dst
                  if ((packet->sctp_hdr.dst_port == old_mme_port) && (packet->sctp_hdr.src_port == old_enb_port)) {
                    packet->sctp_hdr.u.data_hdr.assoc_id = assoc_id;
                    S1AP_DEBUG("tPacket:\tnum %u  | original frame number %u \n", packet->packet_number, packet->original_frame_number);
                    S1AP_DEBUG("\tUpdated assoc id: %u\n", assoc_id);
                  }
                }
              }
            } else if (packet->action == ET_PACKET_ACTION_S1C_RECEIVE) {
              ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.src, &mme_desc_p->mme_net_ip_address);
              if (0 == ret) {
                ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.dst, &s1ap_eNB_instance->s1c_net_ip_address);
                if (0 == ret) {
                  // same IP src, same IP dst
                  if ((packet->sctp_hdr.src_port == old_mme_port) && (packet->sctp_hdr.dst_port == old_enb_port)) {
                    packet->sctp_hdr.u.data_hdr.assoc_id = assoc_id;
                    S1AP_DEBUG("tPacket:\tnum %u  | original frame number %u \n", packet->packet_number, packet->original_frame_number);
                    S1AP_DEBUG("\tUpdated assoc id: %u\n", assoc_id);
                  }
                }
              }
            }
          }
        }
        break;

        // Strong assumption
        // in replayed scenario, the order of SCTP INIT packets is supposed to be the same as in the catched scenario
      case SCTP_CID_INIT:
        S1AP_DEBUG("%s for SCTP association (%u) SCTP_CID_INIT\n",__FUNCTION__,assoc_id);
        ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.dst, &mme_desc_p->mme_net_ip_address);
        if (0 == ret) {
          ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.src, &s1ap_eNB_instance->s1c_net_ip_address);
          if (0 == ret) {
            if (0 == old_enb_port) {
              if (ET_PACKET_STATUS_NONE == packet->status) {
                packet->status = ET_PACKET_STATUS_SENT;
                old_enb_port = packet->sctp_hdr.src_port;
                S1AP_DEBUG("%s for SCTP association (%u) SCTP_CID_INIT SUCCESS\n",__FUNCTION__,assoc_id);
              }
            }
          }
        }
        break;

      case SCTP_CID_INIT_ACK:
        S1AP_DEBUG("%s for SCTP association (%u) SCTP_CID_INIT_ACK\n",__FUNCTION__,assoc_id);
        ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.src, &mme_desc_p->mme_net_ip_address);
        if (0 == ret) {
          ret = et_compare_et_ip_to_net_ip_address(&packet->ip_hdr.dst, &s1ap_eNB_instance->s1c_net_ip_address);
          if (0 == ret) {
            if (old_enb_port == packet->sctp_hdr.dst_port) {
              if (ET_PACKET_STATUS_NONE == packet->status) {
                packet->status = ET_PACKET_STATUS_RECEIVED;
                old_mme_port = packet->sctp_hdr.dst_port;
                S1AP_DEBUG("%s for SCTP association (%u) SCTP_CID_INIT_ACK SUCCESS\n",__FUNCTION__,assoc_id);
              }
            }
          }
        }
        break;

      case SCTP_CID_HEARTBEAT:
      case SCTP_CID_HEARTBEAT_ACK:
      case SCTP_CID_COOKIE_ECHO:
      case SCTP_CID_COOKIE_ACK:
      case SCTP_CID_ECN_ECNE:
      case SCTP_CID_ECN_CWR:
        break;

      case SCTP_CID_ABORT:
      case SCTP_CID_SHUTDOWN:
      case SCTP_CID_SHUTDOWN_ACK:
      case SCTP_CID_ERROR:
      case SCTP_CID_SHUTDOWN_COMPLETE:
        //TODO
        break;

      default:
        AssertFatal(0, "Unknown chunk_type %d packet num %u", packet->sctp_hdr.chunk_type, packet->packet_number);
        ;
    }
    packet = packet->next;
  }
}
//------------------------------------------------------------------------------
void et_s1ap_handle_s1_setup_message(s1ap_eNB_mme_data_t *mme_desc_p, int sctp_shutdown)
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
      mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb++;
    }

    et_s1ap_update_assoc_id_of_packets(mme_desc_p->assoc_id,
        mme_desc_p->s1ap_eNB_instance,
        mme_desc_p);


    /* If there are no more pending messages, inform eNB app */
    if (mme_desc_p->s1ap_eNB_instance->s1ap_mme_pending_nb == 0) {
      MessageDef                 *message_p;

      message_p = itti_alloc_new_message(TASK_S1AP, S1AP_REGISTER_ENB_CNF);
      S1AP_REGISTER_ENB_CNF(message_p).nb_mme = mme_desc_p->s1ap_eNB_instance->s1ap_mme_associated_nb;
      itti_send_msg_to_task(TASK_ENB_APP, mme_desc_p->s1ap_eNB_instance->instance, message_p);
    }
  }
}
//------------------------------------------------------------------------------
void et_s1ap_eNB_handle_register_eNB(instance_t instance, s1ap_register_enb_req_t *s1ap_register_eNB)
{
  s1ap_eNB_instance_t *new_instance;
  uint8_t index;

  DevAssert(s1ap_register_eNB != NULL);

  /* Look if the provided instance already exists */
  new_instance = et_s1ap_eNB_get_instance(instance);

  if (new_instance != NULL) {
    /* Checks if it is a retry on the same eNB */
    DevCheck(new_instance->eNB_id == s1ap_register_eNB->eNB_id, new_instance->eNB_id, s1ap_register_eNB->eNB_id, 0);
    DevCheck(new_instance->cell_type == s1ap_register_eNB->cell_type, new_instance->cell_type, s1ap_register_eNB->cell_type, 0);
    DevCheck(new_instance->tac == s1ap_register_eNB->tac, new_instance->tac, s1ap_register_eNB->tac, 0);
    DevCheck(new_instance->mcc == s1ap_register_eNB->mcc, new_instance->mcc, s1ap_register_eNB->mcc, 0);
    DevCheck(new_instance->mnc == s1ap_register_eNB->mnc, new_instance->mnc, s1ap_register_eNB->mnc, 0);
    DevCheck(new_instance->mnc_digit_length == s1ap_register_eNB->mnc_digit_length, new_instance->mnc_digit_length, s1ap_register_eNB->mnc_digit_length, 0);
    DevCheck(memcmp((void*)&new_instance->s1c_net_ip_address, (void*)&s1ap_register_eNB->enb_ip_address, sizeof(new_instance->s1c_net_ip_address)) == 0, 0,0,0);
  } else {
    new_instance = calloc(1, sizeof(s1ap_eNB_instance_t));
    DevAssert(new_instance != NULL);

    RB_INIT(&new_instance->s1ap_ue_head);
    RB_INIT(&new_instance->s1ap_mme_head);

    /* Copy usefull parameters */
    new_instance->instance         = instance;
    new_instance->eNB_name         = s1ap_register_eNB->eNB_name;
    new_instance->eNB_id           = s1ap_register_eNB->eNB_id;
    new_instance->cell_type        = s1ap_register_eNB->cell_type;
    new_instance->tac              = s1ap_register_eNB->tac;
    new_instance->mcc              = s1ap_register_eNB->mcc;
    new_instance->mnc              = s1ap_register_eNB->mnc;
    new_instance->mnc_digit_length = s1ap_register_eNB->mnc_digit_length;
    memcpy((void*)&new_instance->s1c_net_ip_address, (void*)&s1ap_register_eNB->enb_ip_address, sizeof(new_instance->s1c_net_ip_address));

    /* Add the new instance to the list of eNB (meaningfull in virtual mode) */
    et_s1ap_eNB_insert_new_instance(new_instance);

    S1AP_DEBUG("Registered new eNB[%d] and %s eNB id %u\n",
               instance,
               s1ap_register_eNB->cell_type == CELL_MACRO_ENB ? "macro" : "home",
               s1ap_register_eNB->eNB_id);
  }

  DevCheck(s1ap_register_eNB->nb_mme <= S1AP_MAX_NB_MME_IP_ADDRESS,
           S1AP_MAX_NB_MME_IP_ADDRESS, s1ap_register_eNB->nb_mme, 0);

  /* Trying to connect to provided list of MME ip address */
  for (index = 0; index < s1ap_register_eNB->nb_mme; index++) {
    et_s1ap_eNB_register_mme(new_instance,
                      &s1ap_register_eNB->mme_ip_address[index],
                          &s1ap_register_eNB->enb_ip_address,
                          s1ap_register_eNB->sctp_in_streams,
                          s1ap_register_eNB->sctp_out_streams);
  }
}

//------------------------------------------------------------------------------
void et_s1ap_eNB_handle_sctp_association_resp(instance_t instance, sctp_new_association_resp_t *sctp_new_association_resp)
{
  s1ap_eNB_instance_t *instance_p      = NULL;
  s1ap_eNB_mme_data_t *s1ap_mme_data_p = NULL;

  DevAssert(sctp_new_association_resp != NULL);

  instance_p = et_s1ap_eNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  s1ap_mme_data_p = et_s1ap_eNB_get_MME(instance_p, -1,
                                     sctp_new_association_resp->ulp_cnx_id);
  DevAssert(s1ap_mme_data_p != NULL);

  if (sctp_new_association_resp->sctp_state != SCTP_STATE_ESTABLISHED) {
    S1AP_WARN("Received unsuccessful result for SCTP association (%u), instance %d, cnx_id %u\n",
              sctp_new_association_resp->sctp_state,
              instance,
              sctp_new_association_resp->ulp_cnx_id);

    et_s1ap_handle_s1_setup_message(s1ap_mme_data_p, sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);

    return;
  }

  S1AP_DEBUG("Received successful result for SCTP association (%u), instance %d, cnx_id %u\n",
             sctp_new_association_resp->sctp_state,
             instance,
             sctp_new_association_resp->ulp_cnx_id);
  /* Update parameters */
  s1ap_mme_data_p->assoc_id    = sctp_new_association_resp->assoc_id;
  s1ap_mme_data_p->in_streams  = sctp_new_association_resp->in_streams;
  s1ap_mme_data_p->out_streams = sctp_new_association_resp->out_streams;

  et_s1ap_handle_s1_setup_message(s1ap_mme_data_p, sctp_new_association_resp->sctp_state == SCTP_STATE_SHUTDOWN);
}

//------------------------------------------------------------------------------
void *et_s1ap_eNB_task(void *arg)
{
  MessageDef *received_msg = NULL;
  int         result;

  S1AP_DEBUG("Starting S1AP layer\n");

  et_s1ap_eNB_prepare_internal_data();

  itti_mark_task_ready(TASK_S1AP);
  MSC_START_USE();

  while (1) {
    itti_receive_msg(TASK_S1AP, &received_msg);

    switch (ITTI_MSG_ID(received_msg)) {
    case TERMINATE_MESSAGE:
      S1AP_WARN("*** Exiting S1AP thread\n");
      itti_exit_task();
      break;

    case S1AP_REGISTER_ENB_REQ: {
      /* Register a new eNB.
       * in Virtual mode eNBs will be distinguished using the mod_id/
       * Each eNB has to send an S1AP_REGISTER_ENB message with its
       * own parameters.
       */
      et_s1ap_eNB_handle_register_eNB(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                   &S1AP_REGISTER_ENB_REQ(received_msg));
    }
    break;

    case SCTP_NEW_ASSOCIATION_RESP: {
      et_s1ap_eNB_handle_sctp_association_resp(ITTI_MSG_DESTINATION_INSTANCE(received_msg),
                                            &received_msg->ittiMsg.sctp_new_association_resp);
    }
    break;

    case SCTP_DATA_IND: {
      et_s1ap_eNB_handle_sctp_data_ind(&received_msg->ittiMsg.sctp_data_ind);
    }
    break;

    case TIMER_HAS_EXPIRED:
      LOG_I(S1AP, " Received TIMER_HAS_EXPIRED: timer_id %d\n", TIMER_HAS_EXPIRED(received_msg).timer_id);
      {
        et_packet_t * packet = (et_packet_t*)TIMER_HAS_EXPIRED (received_msg).arg;
        et_event_t    event;
        g_scenario->timer_count--;
        if (NULL != packet) {
          if (packet->status == ET_PACKET_STATUS_SCHEDULED_FOR_RECEIVING) {
            memset((void*)&event, 0, sizeof(event));
            event.code = ET_EVENT_RX_PACKET_TIME_OUT;
            event.u.rx_packet_time_out = packet;
            et_scenario_fsm_notify_event(event);
          } else if (packet->status == ET_PACKET_STATUS_SCHEDULED_FOR_SENDING) {
            memset((void*)&event, 0, sizeof(event));
            event.code = ET_EVENT_TX_TIMED_PACKET;
            event.u.tx_timed_packet = packet;
            et_scenario_fsm_notify_event(event);

            et_event_t continue_event;
            continue_event.code = ET_EVENT_TICK;
            et_scenario_fsm_notify_event(continue_event);
          } else if ((packet->status != ET_PACKET_STATUS_SENT) && ((packet->status != ET_PACKET_STATUS_RECEIVED))) {
            AssertFatal (0, "Bad status %d of packet timed out!\n", packet->status);
          }
        } else {
          LOG_W(S1AP, " Received TIMER_HAS_EXPIRED: timer_id %d, no packet attached to timer\n", TIMER_HAS_EXPIRED(received_msg).timer_id);
        }
      }
      if (TIMER_HAS_EXPIRED (received_msg).timer_id == g_scenario->enb_register_retry_timer_id) {
        /* Restart the registration process */
        g_scenario->registered_enb = 0;
        et_eNB_app_register (g_scenario->enb_properties);
      }
      break;

    default:
      S1AP_ERROR("Received unhandled message: %d:%s\n",
                 ITTI_MSG_ID(received_msg), ITTI_MSG_NAME(received_msg));
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(received_msg), received_msg);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);

    received_msg = NULL;
  }

  return NULL;
}


