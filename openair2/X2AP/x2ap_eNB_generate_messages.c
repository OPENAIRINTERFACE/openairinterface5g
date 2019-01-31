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

#include "intertask_interface.h"

#include "x2ap_common.h"
#include "x2ap_eNB.h"
#include "x2ap_eNB_generate_messages.h"
#include "x2ap_eNB_encoder.h"

#include "x2ap_eNB_itti_messaging.h"

#include "msc.h"
#include "assertions.h"
#include "conversions.h"

int x2ap_eNB_generate_x2_setup_request(
  x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p)
{
  X2AP_X2AP_PDU_t                     pdu;
  X2AP_X2SetupRequest_t              *out;
  X2AP_X2SetupRequest_IEs_t          *ie;
  X2AP_PLMN_Identity_t               *plmn;
  ServedCells__Member                *servedCellMember;
  X2AP_GU_Group_ID_t                 *gu;

  uint8_t  *buffer;
  uint32_t  len;
  int       ret = 0;

  DevAssert(instance_p != NULL);
  DevAssert(x2ap_eNB_data_p != NULL);

  x2ap_eNB_data_p->state = X2AP_ENB_STATE_WAITING;

  /* Prepare the X2AP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = X2AP_X2AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = X2AP_ProcedureCode_id_x2Setup;
  pdu.choice.initiatingMessage.criticality = X2AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = X2AP_InitiatingMessage__value_PR_X2SetupRequest;
  out = &pdu.choice.initiatingMessage.value.choice.X2SetupRequest;

  /* mandatory */
  ie = (X2AP_X2SetupRequest_IEs_t *)calloc(1, sizeof(X2AP_X2SetupRequest_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_GlobalENB_ID;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_X2SetupRequest_IEs__value_PR_GlobalENB_ID;
  MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                    &ie->value.choice.GlobalENB_ID.pLMN_Identity);
  ie->value.choice.GlobalENB_ID.eNB_ID.present = X2AP_ENB_ID_PR_macro_eNB_ID;
  MACRO_ENB_ID_TO_BIT_STRING(instance_p->eNB_id,
                             &ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID);
  X2AP_INFO("%d -> %02x%02x%02x\n", instance_p->eNB_id,
            ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[0],
            ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[1],
            ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[2]);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_X2SetupRequest_IEs_t *)calloc(1, sizeof(X2AP_X2SetupRequest_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_ServedCells;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_X2SetupRequest_IEs__value_PR_ServedCells;
  {
    for (int i = 0; i<instance_p->num_cc; i++){
      servedCellMember = (ServedCells__Member *)calloc(1,sizeof(ServedCells__Member));
      {
        servedCellMember->servedCellInfo.pCI = instance_p->Nid_cell[i];

        MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                      &servedCellMember->servedCellInfo.cellId.pLMN_Identity);
        MACRO_ENB_ID_TO_CELL_IDENTITY(instance_p->eNB_id,0,
                                   &servedCellMember->servedCellInfo.cellId.eUTRANcellIdentifier);

        INT16_TO_OCTET_STRING(instance_p->tac, &servedCellMember->servedCellInfo.tAC);
        plmn = (X2AP_PLMN_Identity_t *)calloc(1,sizeof(X2AP_PLMN_Identity_t));
        {
          MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length, plmn);
          ASN_SEQUENCE_ADD(&servedCellMember->servedCellInfo.broadcastPLMNs.list, plmn);
        }

	if (instance_p->frame_type[i] == FDD) {
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.present = X2AP_EUTRA_Mode_Info_PR_fDD;
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_EARFCN = instance_p->fdd_earfcn_DL[i];
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_EARFCN = instance_p->fdd_earfcn_UL[i];
          switch (instance_p->N_RB_DL[i]) {
            case 6:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw6;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw6;
              break;
            case 15:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw15;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw15;
              break;
            case 25:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw25;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw25;
              break;
            case 50:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw50;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw50;
              break;
            case 75:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw75;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw75;
              break;
            case 100:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw100;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw100;
              break;
            default:
              AssertFatal(0,"Failed: Check value for N_RB_DL/N_RB_UL");
              break;
          }
        }
        else {
          AssertFatal(0,"X2Setuprequest not supported for TDD!");
        }
      }
      ASN_SEQUENCE_ADD(&ie->value.choice.ServedCells.list, servedCellMember);
    }
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_X2SetupRequest_IEs_t *)calloc(1, sizeof(X2AP_X2SetupRequest_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_GUGroupIDList;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_X2SetupRequest_IEs__value_PR_GUGroupIDList;
  {
    gu = (X2AP_GU_Group_ID_t *)calloc(1, sizeof(X2AP_GU_Group_ID_t));
    {
      MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                    &gu->pLMN_Identity);
      //@TODO: consider to update this value
      INT16_TO_OCTET_STRING(0, &gu->mME_Group_ID);
    }
    ASN_SEQUENCE_ADD(&ie->value.choice.GUGroupIDList.list, gu);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (x2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    X2AP_ERROR("Failed to encode X2 setup request\n");
    return -1;
  }

  MSC_LOG_TX_MESSAGE (MSC_X2AP_SRC_ENB, MSC_X2AP_TARGET_ENB, NULL, 0, "0 X2Setup/initiatingMessage assoc_id %u", x2ap_eNB_data_p->assoc_id);

  x2ap_eNB_itti_send_sctp_data_req(instance_p->instance, x2ap_eNB_data_p->assoc_id, buffer, len, 0);

  return ret;
}

int x2ap_eNB_generate_x2_setup_response(x2ap_eNB_data_t *x2ap_eNB_data_p)
{
  X2AP_X2AP_PDU_t                     pdu;
  X2AP_X2SetupResponse_t              *out;
  X2AP_X2SetupResponse_IEs_t          *ie;
  X2AP_PLMN_Identity_t                *plmn;
  ServedCells__Member                 *servedCellMember;
  X2AP_GU_Group_ID_t                  *gu;

  x2ap_eNB_instance_t                 *instance_p;

  uint8_t  *buffer;
  uint32_t  len;
  int       ret = 0;

  DevAssert(x2ap_eNB_data_p != NULL);

  /* get the eNB instance */
  instance_p = x2ap_eNB_data_p->x2ap_eNB_instance;

  DevAssert(instance_p != NULL);

  /* Prepare the X2AP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = X2AP_X2AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = X2AP_ProcedureCode_id_x2Setup;
  pdu.choice.successfulOutcome.criticality = X2AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = X2AP_SuccessfulOutcome__value_PR_X2SetupResponse;
  out = &pdu.choice.successfulOutcome.value.choice.X2SetupResponse;

  /* mandatory */
  ie = (X2AP_X2SetupResponse_IEs_t *)calloc(1, sizeof(X2AP_X2SetupResponse_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_GlobalENB_ID;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_X2SetupResponse_IEs__value_PR_GlobalENB_ID;
  MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                    &ie->value.choice.GlobalENB_ID.pLMN_Identity);
  ie->value.choice.GlobalENB_ID.eNB_ID.present = X2AP_ENB_ID_PR_macro_eNB_ID;
  MACRO_ENB_ID_TO_BIT_STRING(instance_p->eNB_id,
                             &ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID);
  X2AP_INFO("%d -> %02x%02x%02x\n", instance_p->eNB_id,
            ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[0],
            ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[1],
            ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[2]);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_X2SetupResponse_IEs_t *)calloc(1, sizeof(X2AP_X2SetupResponse_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_ServedCells;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_X2SetupResponse_IEs__value_PR_ServedCells;
  {
    for (int i = 0; i<instance_p->num_cc; i++){
      servedCellMember = (ServedCells__Member *)calloc(1,sizeof(ServedCells__Member));
      {
        servedCellMember->servedCellInfo.pCI = instance_p->Nid_cell[i];

        MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                      &servedCellMember->servedCellInfo.cellId.pLMN_Identity);
        MACRO_ENB_ID_TO_CELL_IDENTITY(instance_p->eNB_id,0,
                                   &servedCellMember->servedCellInfo.cellId.eUTRANcellIdentifier);

        INT16_TO_OCTET_STRING(instance_p->tac, &servedCellMember->servedCellInfo.tAC);
        plmn = (X2AP_PLMN_Identity_t *)calloc(1,sizeof(X2AP_PLMN_Identity_t));
        {
          MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length, plmn);
          ASN_SEQUENCE_ADD(&servedCellMember->servedCellInfo.broadcastPLMNs.list, plmn);
        }

	if (instance_p->frame_type[i] == FDD) {
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.present = X2AP_EUTRA_Mode_Info_PR_fDD;
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_EARFCN = instance_p->fdd_earfcn_DL[i];
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_EARFCN = instance_p->fdd_earfcn_UL[i];
          switch (instance_p->N_RB_DL[i]) {
            case 6:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw6;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw6;
              break;
            case 15:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw15;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw15;
              break;
            case 25:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw25;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw25;
              break;
            case 50:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw50;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw50;
              break;
            case 75:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw75;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw75;
              break;
            case 100:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw100;
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw100;
              break;
            default:
              AssertFatal(0,"Failed: Check value for N_RB_DL/N_RB_UL");
              break;
          }
        }
        else {
          AssertFatal(0,"X2Setupresponse not supported for TDD!");
        }
      }
      ASN_SEQUENCE_ADD(&ie->value.choice.ServedCells.list, servedCellMember);
    }
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_X2SetupResponse_IEs_t *)calloc(1, sizeof(X2AP_X2SetupResponse_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_GUGroupIDList;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_X2SetupResponse_IEs__value_PR_GUGroupIDList;
  {
    gu = (X2AP_GU_Group_ID_t *)calloc(1, sizeof(X2AP_GU_Group_ID_t));
    {
      MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                    &gu->pLMN_Identity);
      //@TODO: consider to update this value
      INT16_TO_OCTET_STRING(0, &gu->mME_Group_ID);
    }
    ASN_SEQUENCE_ADD(&ie->value.choice.GUGroupIDList.list, gu);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (x2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    X2AP_ERROR("Failed to encode X2 setup response\n");
    return -1;
  }

  x2ap_eNB_data_p->state = X2AP_ENB_STATE_READY;

  MSC_LOG_TX_MESSAGE (MSC_X2AP_SRC_ENB, MSC_X2AP_TARGET_ENB, NULL, 0, "0 X2Setup/successfulOutcome assoc_id %u", x2ap_eNB_data_p->assoc_id);

  x2ap_eNB_itti_send_sctp_data_req(instance_p->instance, x2ap_eNB_data_p->assoc_id, buffer, len, 0);

  return ret;
}

int x2ap_eNB_generate_x2_setup_failure(instance_t instance,
                                       uint32_t assoc_id,
                                       X2AP_Cause_PR cause_type,
                                       long cause_value,
                                       long time_to_wait)
{
  X2AP_X2AP_PDU_t                     pdu;
  X2AP_X2SetupFailure_t              *out;
  X2AP_X2SetupFailure_IEs_t          *ie;

  uint8_t  *buffer;
  uint32_t  len;
  int       ret = 0;

  /* Prepare the X2AP message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = X2AP_X2AP_PDU_PR_unsuccessfulOutcome;
  pdu.choice.unsuccessfulOutcome.procedureCode = X2AP_ProcedureCode_id_x2Setup;
  pdu.choice.unsuccessfulOutcome.criticality = X2AP_Criticality_reject;
  pdu.choice.unsuccessfulOutcome.value.present = X2AP_UnsuccessfulOutcome__value_PR_X2SetupFailure;
  out = &pdu.choice.unsuccessfulOutcome.value.choice.X2SetupFailure;

  /* mandatory */
  ie = (X2AP_X2SetupFailure_IEs_t *)calloc(1, sizeof(X2AP_X2SetupFailure_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_Cause;
  ie->criticality = X2AP_Criticality_ignore;
  ie->value.present = X2AP_X2SetupFailure_IEs__value_PR_Cause;

  x2ap_eNB_set_cause (&ie->value.choice.Cause, cause_type, cause_value);

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional: consider to handle this later */
  ie = (X2AP_X2SetupFailure_IEs_t *)calloc(1, sizeof(X2AP_X2SetupFailure_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_TimeToWait;
  ie->criticality = X2AP_Criticality_ignore;
  ie->value.present = X2AP_X2SetupFailure_IEs__value_PR_TimeToWait;

  if (time_to_wait > -1) {
    ie->value.choice.TimeToWait = time_to_wait;
  }

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (x2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    X2AP_ERROR("Failed to encode X2 setup failure\n");
    return -1;
  }

  MSC_LOG_TX_MESSAGE (MSC_X2AP_SRC_ENB,
                      MSC_X2AP_TARGET_ENB, NULL, 0,
                      "0 X2Setup/unsuccessfulOutcome  assoc_id %u cause %u value %u",
                      assoc_id, cause_type, cause_value);

  x2ap_eNB_itti_send_sctp_data_req(instance, assoc_id, buffer, len, 0);

  return ret;
}

int x2ap_eNB_set_cause (X2AP_Cause_t * cause_p,
                        X2AP_Cause_PR cause_type,
                        long cause_value)
{

  DevAssert (cause_p != NULL);
  cause_p->present = cause_type;

  switch (cause_type) {
  case X2AP_Cause_PR_radioNetwork:
    cause_p->choice.misc = cause_value;
    break;

  case X2AP_Cause_PR_transport:
    cause_p->choice.misc = cause_value;
    break;

  case X2AP_Cause_PR_protocol:
    cause_p->choice.misc = cause_value;
    break;

  case X2AP_Cause_PR_misc:
    cause_p->choice.misc = cause_value;
    break;

  default:
    return -1;
  }

  return 0;
}
