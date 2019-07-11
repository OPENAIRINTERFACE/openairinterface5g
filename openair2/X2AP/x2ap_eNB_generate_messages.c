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

/*! \file x2ap_eNB_generate_messages.c
 * \brief x2ap procedures for eNB
 * \author Konstantinos Alexandris <Konstantinos.Alexandris@eurecom.fr>, Cedric Roux <Cedric.Roux@eurecom.fr>, Navid Nikaein <Navid.Nikaein@eurecom.fr>
 * \date 2018
 * \version 1.0
 */

#include "intertask_interface.h"

#include "X2AP_LastVisitedCell-Item.h"

#include "x2ap_common.h"
#include "x2ap_eNB.h"
#include "x2ap_eNB_generate_messages.h"
#include "x2ap_eNB_encoder.h"
#include "x2ap_eNB_decoder.h"
#include "x2ap_ids.h"

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
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.present = X2AP_EUTRA_Mode_Info_PR_tDD;
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.eARFCN = instance_p->fdd_earfcn_DL[i];
          switch (instance_p->subframeAssignment[i]) {
            case 0:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa0;
              break;
            case 1:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa1;
              break;
            case 2:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa2;
              break;
            case 3:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa3;
              break;
            case 4:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa4;
              break;
            case 5:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa5;
              break;
            case 6:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa6;
              break;
            default:
              AssertFatal(0,"Failed: Check value for subframeAssignment");
              break;
          }
          switch (instance_p->specialSubframe[i]) {
            case 0:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp0;
              break;
            case 1:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp1;
              break;
            case 2:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp2;
              break;
            case 3:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp3;
              break;
            case 4:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp4;
              break;
            case 5:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp5;
              break;
            case 6:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp6;
              break;
            case 7:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp7;
              break;
            case 8:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp8;
              break;
            default:
              AssertFatal(0,"Failed: Check value for subframeAssignment");
              break;
          }
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.cyclicPrefixDL=X2AP_CyclicPrefixDL_normal;
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.cyclicPrefixUL=X2AP_CyclicPrefixUL_normal;
          
          switch (instance_p->N_RB_DL[i]) {
            case 6:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw6;
              break;
            case 15:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw15;
              break;
            case 25:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw25;
              break;
            case 50:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw50;
              break;
            case 75:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw75;
              break;
            case 100:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw100;
              break;
            default:
              AssertFatal(0,"Failed: Check value for N_RB_DL/N_RB_UL");
              break;
          }
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

int x2ap_eNB_generate_x2_setup_response(x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p)
{
  X2AP_X2AP_PDU_t                     pdu;
  X2AP_X2SetupResponse_t              *out;
  X2AP_X2SetupResponse_IEs_t          *ie;
  X2AP_PLMN_Identity_t                *plmn;
  ServedCells__Member                 *servedCellMember;
  X2AP_GU_Group_ID_t                  *gu;

  uint8_t  *buffer;
  uint32_t  len;
  int       ret = 0;

  DevAssert(instance_p != NULL);
  DevAssert(x2ap_eNB_data_p != NULL);

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
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.present = X2AP_EUTRA_Mode_Info_PR_tDD;
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.eARFCN = instance_p->fdd_earfcn_DL[i];
          switch (instance_p->subframeAssignment[i]) {
            case 0:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa0;
              break;
            case 1:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa1;
              break;
            case 2:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa2;
              break;
            case 3:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa3;
              break;
            case 4:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa4;
              break;
            case 5:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa5;
              break;
            case 6:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.subframeAssignment = X2AP_SubframeAssignment_sa6;
              break;
            default:
              AssertFatal(0,"Failed: Check value for subframeAssignment");
              break;
          }
          switch (instance_p->specialSubframe[i]) {
            case 0:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp0;
              break;
            case 1:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp1;
              break;
            case 2:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp2;
              break;
            case 3:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp3;
              break;
            case 4:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp4;
              break;
            case 5:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp5;
              break;
            case 6:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp6;
              break;
            case 7:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp7;
              break;
            case 8:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.specialSubframePatterns = X2AP_SpecialSubframePatterns_ssp8;
              break;
            default:
              AssertFatal(0,"Failed: Check value for subframeAssignment");
              break;
          }
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.cyclicPrefixDL=X2AP_CyclicPrefixDL_normal;
          servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.specialSubframe_Info.cyclicPrefixUL=X2AP_CyclicPrefixUL_normal;
          
          switch (instance_p->N_RB_DL[i]) {
            case 6:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw6;
              break;
            case 15:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw15;
              break;
            case 25:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw25;
              break;
            case 50:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw50;
              break;
            case 75:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw75;
              break;
            case 100:
              servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.tDD.transmission_Bandwidth = X2AP_Transmission_Bandwidth_bw100;
              break;
            default:
              AssertFatal(0,"Failed: Check value for N_RB_DL/N_RB_UL");
              break;
          }
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

int x2ap_eNB_generate_x2_handover_request (x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                           x2ap_handover_req_t *x2ap_handover_req, int ue_id)
{

  X2AP_X2AP_PDU_t                     pdu;
  X2AP_HandoverRequest_t              *out;
  X2AP_HandoverRequest_IEs_t          *ie;
  X2AP_E_RABs_ToBeSetup_ItemIEs_t     *e_RABS_ToBeSetup_ItemIEs;
  X2AP_E_RABs_ToBeSetup_Item_t        *e_RABs_ToBeSetup_Item;
  X2AP_LastVisitedCell_Item_t         *lastVisitedCell_Item;

  uint8_t  *buffer;
  uint32_t  len;
  int       ret = 0;

  DevAssert(instance_p != NULL);
  DevAssert(x2ap_eNB_data_p != NULL);

  /* Prepare the X2AP handover message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = X2AP_X2AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = X2AP_ProcedureCode_id_handoverPreparation;
  pdu.choice.initiatingMessage.criticality = X2AP_Criticality_reject;
  pdu.choice.initiatingMessage.value.present = X2AP_InitiatingMessage__value_PR_HandoverRequest;
  out = &pdu.choice.initiatingMessage.value.choice.HandoverRequest;

  /* mandatory */
  ie = (X2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(X2AP_HandoverRequest_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_HandoverRequest_IEs__value_PR_UE_X2AP_ID;
  ie->value.choice.UE_X2AP_ID = x2ap_id_get_id_source(&instance_p->id_manager, ue_id);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(X2AP_HandoverRequest_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_Cause;
  ie->criticality = X2AP_Criticality_ignore;
  ie->value.present = X2AP_HandoverRequest_IEs__value_PR_Cause;
  ie->value.choice.Cause.present = X2AP_Cause_PR_radioNetwork;
  ie->value.choice.Cause.choice.radioNetwork = X2AP_CauseRadioNetwork_handover_desirable_for_radio_reasons;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(X2AP_HandoverRequest_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_TargetCell_ID;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_HandoverRequest_IEs__value_PR_ECGI;
  MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                       &ie->value.choice.ECGI.pLMN_Identity);
  MACRO_ENB_ID_TO_CELL_IDENTITY(x2ap_eNB_data_p->eNB_id, 0, &ie->value.choice.ECGI.eUTRANcellIdentifier);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(X2AP_HandoverRequest_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_GUMMEI_ID;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_HandoverRequest_IEs__value_PR_GUMMEI;
  MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                       &ie->value.choice.GUMMEI.gU_Group_ID.pLMN_Identity);
  //@TODO: consider to update these values
  INT16_TO_OCTET_STRING(x2ap_handover_req->ue_gummei.mme_group_id, &ie->value.choice.GUMMEI.gU_Group_ID.mME_Group_ID);
  MME_CODE_TO_OCTET_STRING(x2ap_handover_req->ue_gummei.mme_code, &ie->value.choice.GUMMEI.mME_Code);
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(X2AP_HandoverRequest_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_UE_ContextInformation;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_HandoverRequest_IEs__value_PR_UE_ContextInformation;
  //@TODO: consider to update this value
  ie->value.choice.UE_ContextInformation.mME_UE_S1AP_ID = x2ap_handover_req->mme_ue_s1ap_id;

  KENB_STAR_TO_BIT_STRING(x2ap_handover_req->kenb,&ie->value.choice.UE_ContextInformation.aS_SecurityInformation.key_eNodeB_star);

  if (x2ap_handover_req->kenb_ncc >=0) { // Check this condition
    ie->value.choice.UE_ContextInformation.aS_SecurityInformation.nextHopChainingCount = x2ap_handover_req->kenb_ncc;
  }
  else {
    ie->value.choice.UE_ContextInformation.aS_SecurityInformation.nextHopChainingCount = 1;
  }

  ENCRALG_TO_BIT_STRING(x2ap_handover_req->security_capabilities.encryption_algorithms,
              &ie->value.choice.UE_ContextInformation.uESecurityCapabilities.encryptionAlgorithms);

  INTPROTALG_TO_BIT_STRING(x2ap_handover_req->security_capabilities.integrity_algorithms,
              &ie->value.choice.UE_ContextInformation.uESecurityCapabilities.integrityProtectionAlgorithms);

  //@TODO: update with proper UEAMPR
  UEAGMAXBITRTD_TO_ASN_PRIMITIVES(3L,&ie->value.choice.UE_ContextInformation.uEaggregateMaximumBitRate.uEaggregateMaximumBitRateDownlink);
  UEAGMAXBITRTU_TO_ASN_PRIMITIVES(6L,&ie->value.choice.UE_ContextInformation.uEaggregateMaximumBitRate.uEaggregateMaximumBitRateUplink);
  {
    for (int i=0;i<x2ap_handover_req->nb_e_rabs_tobesetup;i++) {
      e_RABS_ToBeSetup_ItemIEs = (X2AP_E_RABs_ToBeSetup_ItemIEs_t *)calloc(1,sizeof(X2AP_E_RABs_ToBeSetup_ItemIEs_t));
      e_RABS_ToBeSetup_ItemIEs->id = X2AP_ProtocolIE_ID_id_E_RABs_ToBeSetup_Item;
      e_RABS_ToBeSetup_ItemIEs->criticality = X2AP_Criticality_ignore;
      e_RABS_ToBeSetup_ItemIEs->value.present = X2AP_E_RABs_ToBeSetup_ItemIEs__value_PR_E_RABs_ToBeSetup_Item;
      e_RABs_ToBeSetup_Item = &e_RABS_ToBeSetup_ItemIEs->value.choice.E_RABs_ToBeSetup_Item;
      {
        e_RABs_ToBeSetup_Item->e_RAB_ID = x2ap_handover_req->e_rabs_tobesetup[i].e_rab_id;
        e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.qCI = x2ap_handover_req->e_rab_param[i].qos.qci;
        e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.priorityLevel = x2ap_handover_req->e_rab_param[i].qos.allocation_retention_priority.priority_level;
        e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionCapability = x2ap_handover_req->e_rab_param[i].qos.allocation_retention_priority.pre_emp_capability;
        e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionVulnerability = x2ap_handover_req->e_rab_param[i].qos.allocation_retention_priority.pre_emp_vulnerability;
        e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.size = (uint8_t)(x2ap_handover_req->e_rabs_tobesetup[i].eNB_addr.length/8);
        e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.bits_unused = x2ap_handover_req->e_rabs_tobesetup[i].eNB_addr.length%8;
        e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.buf =
                        calloc(1,e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.size);

        memcpy (e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.buf,
                        x2ap_handover_req->e_rabs_tobesetup[i].eNB_addr.buffer,
                        e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.size);

        INT32_TO_OCTET_STRING(x2ap_handover_req->e_rabs_tobesetup[i].gtp_teid,&e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.gTP_TEID);
      }
      ASN_SEQUENCE_ADD(&ie->value.choice.UE_ContextInformation.e_RABs_ToBeSetup_List.list, e_RABS_ToBeSetup_ItemIEs);
    }
  }

  OCTET_STRING_fromBuf(&ie->value.choice.UE_ContextInformation.rRC_Context, (char*) x2ap_handover_req->rrc_buffer, x2ap_handover_req->rrc_buffer_size);

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(X2AP_HandoverRequest_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_UE_HistoryInformation;
  ie->criticality = X2AP_Criticality_ignore;
  ie->value.present = X2AP_HandoverRequest_IEs__value_PR_UE_HistoryInformation;
  //@TODO: consider to update this value
  {
   lastVisitedCell_Item = (X2AP_LastVisitedCell_Item_t *)calloc(1, sizeof(X2AP_LastVisitedCell_Item_t));
   lastVisitedCell_Item->present = X2AP_LastVisitedCell_Item_PR_e_UTRAN_Cell;
   MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
                       &lastVisitedCell_Item->choice.e_UTRAN_Cell.global_Cell_ID.pLMN_Identity);
   MACRO_ENB_ID_TO_CELL_IDENTITY(0, 0, &lastVisitedCell_Item->choice.e_UTRAN_Cell.global_Cell_ID.eUTRANcellIdentifier);
   lastVisitedCell_Item->choice.e_UTRAN_Cell.cellType.cell_Size = X2AP_Cell_Size_small;
   lastVisitedCell_Item->choice.e_UTRAN_Cell.time_UE_StayedInCell = 2;
   ASN_SEQUENCE_ADD(&ie->value.choice.UE_HistoryInformation.list, lastVisitedCell_Item);
  }

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (x2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    X2AP_ERROR("Failed to encode X2 handover request\n");
    abort();
    return -1;
  }

  MSC_LOG_TX_MESSAGE (MSC_X2AP_SRC_ENB, MSC_X2AP_TARGET_ENB, NULL, 0, "0 X2Handover/initiatingMessage assoc_id %u", x2ap_eNB_data_p->assoc_id);

  x2ap_eNB_itti_send_sctp_data_req(instance_p->instance, x2ap_eNB_data_p->assoc_id, buffer, len, 1);

  return ret;
}

int x2ap_eNB_generate_x2_handover_request_ack (x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                               x2ap_handover_req_ack_t *x2ap_handover_req_ack)
{

  X2AP_X2AP_PDU_t                        pdu;
  X2AP_HandoverRequestAcknowledge_t      *out;
  X2AP_HandoverRequestAcknowledge_IEs_t  *ie;
  X2AP_E_RABs_Admitted_ItemIEs_t         *e_RABS_Admitted_ItemIEs;
  X2AP_E_RABs_Admitted_Item_t            *e_RABs_Admitted_Item;
  int                                    ue_id;
  int                                    id_source;
  int                                    id_target;

  uint8_t  *buffer;
  uint32_t  len;
  int       ret = 0;

  DevAssert(instance_p != NULL);
  DevAssert(x2ap_eNB_data_p != NULL);

  ue_id     = x2ap_handover_req_ack->x2_id_target;
  id_source = x2ap_id_get_id_source(&instance_p->id_manager, ue_id);
  id_target = ue_id;

  /* Prepare the X2AP handover message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = X2AP_X2AP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome.procedureCode = X2AP_ProcedureCode_id_handoverPreparation;
  pdu.choice.successfulOutcome.criticality = X2AP_Criticality_reject;
  pdu.choice.successfulOutcome.value.present = X2AP_SuccessfulOutcome__value_PR_HandoverRequestAcknowledge;
  out = &pdu.choice.successfulOutcome.value.choice.HandoverRequestAcknowledge;

  /* mandatory */
  ie = (X2AP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(X2AP_HandoverRequestAcknowledge_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID;
  ie->criticality = X2AP_Criticality_ignore;
  ie->value.present = X2AP_HandoverRequestAcknowledge_IEs__value_PR_UE_X2AP_ID;
  ie->value.choice.UE_X2AP_ID = id_source;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(X2AP_HandoverRequestAcknowledge_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_New_eNB_UE_X2AP_ID;
  ie->criticality = X2AP_Criticality_ignore;
  ie->value.present = X2AP_HandoverRequestAcknowledge_IEs__value_PR_UE_X2AP_ID_1;
  ie->value.choice.UE_X2AP_ID_1 = id_target;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(X2AP_HandoverRequestAcknowledge_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_E_RABs_Admitted_List;
  ie->criticality = X2AP_Criticality_ignore;
  ie->value.present = X2AP_HandoverRequestAcknowledge_IEs__value_PR_E_RABs_Admitted_List;

  {
      for (int i=0;i<x2ap_handover_req_ack->nb_e_rabs_tobesetup;i++) {
        e_RABS_Admitted_ItemIEs = (X2AP_E_RABs_Admitted_ItemIEs_t *)calloc(1,sizeof(X2AP_E_RABs_Admitted_ItemIEs_t));
        e_RABS_Admitted_ItemIEs->id = X2AP_ProtocolIE_ID_id_E_RABs_Admitted_Item;
        e_RABS_Admitted_ItemIEs->criticality = X2AP_Criticality_ignore;
        e_RABS_Admitted_ItemIEs->value.present = X2AP_E_RABs_Admitted_ItemIEs__value_PR_E_RABs_Admitted_Item;
        e_RABs_Admitted_Item = &e_RABS_Admitted_ItemIEs->value.choice.E_RABs_Admitted_Item;
        {
          e_RABs_Admitted_Item->e_RAB_ID = x2ap_handover_req_ack->e_rabs_tobesetup[i].e_rab_id;
	  e_RABs_Admitted_Item->uL_GTP_TunnelEndpoint = NULL;
	  e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint = (X2AP_GTPtunnelEndpoint_t *)calloc(1, sizeof(*e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint));
		  
	  e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.size = (uint8_t)x2ap_handover_req_ack->e_rabs_tobesetup[i].eNB_addr.length;
          e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.bits_unused = 0;
          e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.buf =
                        calloc(1, e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.size);

          memcpy (e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.buf,
                  x2ap_handover_req_ack->e_rabs_tobesetup[i].eNB_addr.buffer,
                  e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.size);

          X2AP_DEBUG("X2 handover response target ip addr. length %lu bits_unused %d buf %d.%d.%d.%d\n",
		  e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.size,
		  e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.bits_unused,
		  e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.buf[0],
		  e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.buf[1],
		  e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.buf[2],
		  e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->transportLayerAddress.buf[3]);
		  
          INT32_TO_OCTET_STRING(x2ap_handover_req_ack->e_rabs_tobesetup[i].gtp_teid, &e_RABs_Admitted_Item->dL_GTP_TunnelEndpoint->gTP_TEID);
        }
        ASN_SEQUENCE_ADD(&ie->value.choice.E_RABs_Admitted_List.list, e_RABS_Admitted_ItemIEs);
      }
  }

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(X2AP_HandoverRequestAcknowledge_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_TargeteNBtoSource_eNBTransparentContainer;
  ie->criticality = X2AP_Criticality_ignore;
  ie->value.present = X2AP_HandoverRequestAcknowledge_IEs__value_PR_TargeteNBtoSource_eNBTransparentContainer;

  OCTET_STRING_fromBuf(&ie->value.choice.TargeteNBtoSource_eNBTransparentContainer, (char*) x2ap_handover_req_ack->rrc_buffer, x2ap_handover_req_ack->rrc_buffer_size);

  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (x2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    X2AP_ERROR("Failed to encode X2 handover response\n");
    abort();
    return -1;
  }

  MSC_LOG_TX_MESSAGE (MSC_X2AP_SRC_ENB, MSC_X2AP_TARGET_ENB, NULL, 0, "0 X2Handover/successfulOutcome assoc_id %u", x2ap_eNB_data_p->assoc_id);

  x2ap_eNB_itti_send_sctp_data_req(instance_p->instance, x2ap_eNB_data_p->assoc_id, buffer, len, 1);

  return ret;
}

int x2ap_eNB_generate_x2_ue_context_release (x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p, x2ap_ue_context_release_t *x2ap_ue_context_release)
{

  X2AP_X2AP_PDU_t                pdu;
  X2AP_UEContextRelease_t        *out;
  X2AP_UEContextRelease_IEs_t    *ie;
  int                            ue_id;
  int                            id_source;
  int                            id_target;

  uint8_t  *buffer;
  uint32_t  len;
  int       ret = 0;

  DevAssert(instance_p != NULL);
  DevAssert(x2ap_eNB_data_p != NULL);

  ue_id = x2ap_find_id_from_rnti(&instance_p->id_manager, x2ap_ue_context_release->rnti);
  if (ue_id == -1) {
    X2AP_ERROR("could not find UE %x\n", x2ap_ue_context_release->rnti);
    exit(1);
  }
  id_source = x2ap_id_get_id_source(&instance_p->id_manager, ue_id);
  id_target = ue_id;

  /* Prepare the X2AP ue context relase message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = X2AP_X2AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = X2AP_ProcedureCode_id_uEContextRelease;
  pdu.choice.initiatingMessage.criticality = X2AP_Criticality_ignore;
  pdu.choice.initiatingMessage.value.present = X2AP_InitiatingMessage__value_PR_UEContextRelease;
  out = &pdu.choice.initiatingMessage.value.choice.UEContextRelease;

  /* mandatory */
  ie = (X2AP_UEContextRelease_IEs_t *)calloc(1, sizeof(X2AP_UEContextRelease_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_UEContextRelease_IEs__value_PR_UE_X2AP_ID;
  ie->value.choice.UE_X2AP_ID = id_source;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* mandatory */
  ie = (X2AP_UEContextRelease_IEs_t *)calloc(1, sizeof(X2AP_UEContextRelease_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_New_eNB_UE_X2AP_ID;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_UEContextRelease_IEs__value_PR_UE_X2AP_ID_1;
  ie->value.choice.UE_X2AP_ID_1 = id_target;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (x2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    X2AP_ERROR("Failed to encode X2 UE Context Release\n");
    abort();
    return -1;
  }

  MSC_LOG_TX_MESSAGE (MSC_X2AP_SRC_ENB, MSC_X2AP_TARGET_ENB, NULL, 0, "0 X2UEContextRelease/initiatingMessage assoc_id %u", x2ap_eNB_data_p->assoc_id);

  x2ap_eNB_itti_send_sctp_data_req(instance_p->instance, x2ap_eNB_data_p->assoc_id, buffer, len, 1);

  return ret;
}

int x2ap_eNB_generate_x2_handover_cancel (x2ap_eNB_instance_t *instance_p, x2ap_eNB_data_t *x2ap_eNB_data_p,
                                          int x2_ue_id,
                                          x2ap_handover_cancel_cause_t cause)
{
  X2AP_X2AP_PDU_t              pdu;
  X2AP_HandoverCancel_t        *out;
  X2AP_HandoverCancel_IEs_t    *ie;
  int                          ue_id;
  int                          id_source;
  int                          id_target;

  uint8_t  *buffer;
  uint32_t  len;
  int       ret = 0;

  DevAssert(instance_p != NULL);
  DevAssert(x2ap_eNB_data_p != NULL);

  ue_id = x2_ue_id;
  id_source = ue_id;
  id_target = x2ap_id_get_id_target(&instance_p->id_manager, ue_id);

  /* Prepare the X2AP handover cancel message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = X2AP_X2AP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage.procedureCode = X2AP_ProcedureCode_id_handoverCancel;
  pdu.choice.initiatingMessage.criticality = X2AP_Criticality_ignore;
  pdu.choice.initiatingMessage.value.present = X2AP_InitiatingMessage__value_PR_HandoverCancel;
  out = &pdu.choice.initiatingMessage.value.choice.HandoverCancel;

  /* mandatory */
  ie = (X2AP_HandoverCancel_IEs_t *)calloc(1, sizeof(X2AP_HandoverCancel_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_Old_eNB_UE_X2AP_ID;
  ie->criticality = X2AP_Criticality_reject;
  ie->value.present = X2AP_HandoverCancel_IEs__value_PR_UE_X2AP_ID;
  ie->value.choice.UE_X2AP_ID = id_source;
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  /* optional */
  if (id_target != -1) {
    ie = (X2AP_HandoverCancel_IEs_t *)calloc(1, sizeof(X2AP_HandoverCancel_IEs_t));
    ie->id = X2AP_ProtocolIE_ID_id_New_eNB_UE_X2AP_ID;
    ie->criticality = X2AP_Criticality_ignore;
    ie->value.present = X2AP_HandoverCancel_IEs__value_PR_UE_X2AP_ID_1;
    ie->value.choice.UE_X2AP_ID_1 = id_target;
    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
  }

  /* mandatory */
  ie = (X2AP_HandoverCancel_IEs_t *)calloc(1, sizeof(X2AP_HandoverCancel_IEs_t));
  ie->id = X2AP_ProtocolIE_ID_id_Cause;
  ie->criticality = X2AP_Criticality_ignore;
  ie->value.present = X2AP_HandoverCancel_IEs__value_PR_Cause;
  switch (cause) {
  case X2AP_T_RELOC_PREP_TIMEOUT:
    ie->value.choice.Cause.present = X2AP_Cause_PR_radioNetwork;
    ie->value.choice.Cause.choice.radioNetwork =
      X2AP_CauseRadioNetwork_trelocprep_expiry;
    break;
  case X2AP_TX2_RELOC_OVERALL_TIMEOUT:
    ie->value.choice.Cause.present = X2AP_Cause_PR_radioNetwork;
    ie->value.choice.Cause.choice.radioNetwork =
      X2AP_CauseRadioNetwork_tx2relocoverall_expiry;
    break;
  default:
    /* we can't come here */
    X2AP_ERROR("unhandled cancel cause\n");
    exit(1);
  }
  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);

  if (x2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    X2AP_ERROR("Failed to encode X2 Handover Cancel\n");
    abort();
    return -1;
  }

  MSC_LOG_TX_MESSAGE (MSC_X2AP_SRC_ENB, MSC_X2AP_TARGET_ENB, NULL, 0, "0 X2HandoverCancel/initiatingMessage assoc_id %u", x2ap_eNB_data_p->assoc_id);

  x2ap_eNB_itti_send_sctp_data_req(instance_p->instance, x2ap_eNB_data_p->assoc_id, buffer, len, 1);

  return ret;
}
