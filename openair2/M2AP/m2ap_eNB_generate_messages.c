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

/*! \file m2ap_eNB_generate_messages.c
 * \brief m2ap procedures for eNB
 * \author Javier Morgade <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#include "intertask_interface.h"

//#include "M2AP_LastVisitedCell-Item.h"

#include "m2ap_common.h"
#include "m2ap_eNB.h"
#include "m2ap_eNB_generate_messages.h"
#include "m2ap_encoder.h"
#include "m2ap_decoder.h"
#include "m2ap_ids.h"

#include "m2ap_itti_messaging.h"

#include "msc.h"
#include "assertions.h"
#include "conversions.h"

//int m2ap_eNB_generate_m2_setup_request(
//  m2ap_eNB_instance_t *instance_p, m2ap_eNB_data_t *m2ap_eNB_data_p)
//{
//  module_id_t enb_mod_idP=0;
//  module_id_t du_mod_idP=0;
//
//  M2AP_M2AP_PDU_t          pdu;
//  M2AP_M2SetupRequest_t    *out;
//  M2AP_M2SetupRequest_Ies_t *ie;
//
//  uint8_t  *buffer;
//  uint32_t  len;
//  int       i = 0;
//  int       j = 0;
//
//  /* Create */
//  /* 0. pdu Type */
//  memset(&pdu, 0, sizeof(pdu));
//  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
//  //pdu.choice.initiatingMessage = (M2AP_InitiatingMessage_t *)calloc(1, sizeof(M2AP_InitiatingMessage_t));
//  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_m2Setup;
//  pdu.choice.initiatingMessage.criticality   = M2AP_Criticality_reject;
//  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_M2SetupRequest;
//  out = &pdu.choice.initiatingMessage.value.choice.M2SetupRequest;
//
//  /* mandatory */
//  /* c1. GlobalENB_ID (integer value) */
//  ie = (M2AP_M2SetupRequest_Ies_t *)calloc(1, sizeof(M2AP_M2SetupRequest_Ies_t));
//  ie->id                        = M2AP_ProtocolIE_ID_id_GlobalENB_ID;
//  ie->criticality               = M2AP_Criticality_reject;
//  ie->value.present             = M2AP_M2SetupRequest_Ies__value_PR_GlobalENB_ID;
//  //ie->value.choice.GlobalENB_ID.eNB_ID = 1;//M2AP_get_next_transaction_identifier(enb_mod_idP, du_mod_idP);
//  MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
//                  &ie->value.choice.GlobalENB_ID.pLMN_Identity);
//  ie->value.choice.GlobalENB_ID.eNB_ID.present = M2AP_ENB_ID_PR_macro_eNB_ID;
//  MACRO_ENB_ID_TO_BIT_STRING(instance_p->eNB_id,
//                           &ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID);
//  M2AP_INFO("%d -> %02x%02x%02x\n", instance_p->eNB_id,
//          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[0],
//          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[1],
//          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[2]);
//
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  ///* mandatory */
//  ///* c2. GNB_eNB_ID (integrer value) */
//  //ie = (M2AP_M2SetupRequest_Ies_t *)calloc(1, sizeof(M2AP_M2SetupRequest_Ies_t));
//  //ie->id                        = M2AP_ProtocolIE_ID_id_gNB_eNB_ID;
//  //ie->criticality               = M2AP_Criticality_reject;
//  //ie->value.present             = M2AP_M2SetupRequestIEs__value_PR_GNB_eNB_ID;
//  //asn_int642INTEGER(&ie->value.choice.GNB_eNB_ID, f1ap_du_data->gNB_eNB_id);
//  //ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* optional */
//  /* c3. ENBname */
//  if (m2ap_eNB_data_p->eNB_name != NULL) {
//    ie = (M2AP_M2SetupRequest_Ies_t *)calloc(1, sizeof(M2AP_M2SetupRequest_Ies_t));
//    ie->id                        = M2AP_ProtocolIE_ID_id_ENBname;
//    ie->criticality               = M2AP_Criticality_ignore;
//    ie->value.present             = M2AP_M2SetupRequest_Ies__value_PR_ENBname;
//    //OCTET_STRING_fromBuf(&ie->value.choice.ENB_Name, m2ap_eNB_data_p->eNB_name,
//                         //strlen(m2ap_eNB_data_p->eNB_name));
//    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//  }
//
//  /* mandatory */
//  /* c4. serverd cells list */
//  ie = (M2AP_M2SetupRequest_Ies_t *)calloc(1, sizeof(M2AP_M2SetupRequest_Ies_t));
//  ie->id                        = M2AP_ProtocolIE_ID_id_ENB_MBMS_Configuration_data_List;
//  ie->criticality               = M2AP_Criticality_reject;
//  ie->value.present             = M2AP_M2SetupRequest_Ies__value_PR_ENB_MBMS_Configuration_data_List;
//
//  int num_mbms_available = 1;//m2ap_du_data->num_mbms_available;
//  LOG_D(M2AP, "num_mbms_available = %d \n", num_mbms_available);
//
// for (i=0;
//       i<num_mbms_available;
//       i++) {
//        /* mandatory */
//        /* 4.1 serverd cells item */
//
//        M2AP_ENB_MBMS_Configuration_data_ItemIEs_t *mbms_configuration_data_list_item_ies;
//        mbms_configuration_data_list_item_ies = (M2AP_ENB_MBMS_Configuration_data_ItemIEs_t *)calloc(1, sizeof(M2AP_ENB_MBMS_Configuration_data_ItemIEs_t));
//        mbms_configuration_data_list_item_ies->id = M2AP_ProtocolIE_ID_id_ENB_MBMS_Configuration_data_Item;
//        mbms_configuration_data_list_item_ies->criticality = M2AP_Criticality_reject;
//        mbms_configuration_data_list_item_ies->value.present = M2AP_ENB_MBMS_Configuration_data_ItemIEs__value_PR_ENB_MBMS_Configuration_data_Item;
//
//	M2AP_ENB_MBMS_Configuration_data_Item_t *mbms_configuration_data_item;
//	mbms_configuration_data_item = &mbms_configuration_data_list_item_ies->value.choice.ENB_MBMS_Configuration_data_Item;
//	{
//		MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
//                  &mbms_configuration_data_item->eCGI.pLMN_Identity);
//        	MACRO_ENB_ID_TO_CELL_IDENTITY(instance_p->eNB_id,0,
//                                   &mbms_configuration_data_item->eCGI.eUTRANcellIdentifier);
//		M2AP_MBMS_Service_Area_t * mbms_service_area;
//		mbms_service_area = (M2AP_MBMS_Service_Area_t*)calloc(1,sizeof(M2AP_MBMS_Service_Area_t));
//		ASN_SEQUENCE_ADD(&mbms_configuration_data_item->mbmsServiceAreaList.list,mbms_service_area);
//
//
//	}
//
//
//        //M2AP_ENB_MBMS_Configuration_data_Item_t mbms_configuration_data_item;
//        //memset((void *)&mbms_configuration_data_item, 0, sizeof(M2AP_ENB_MBMS_Configuration_data_Item_t));
//	
//	//M2AP_ECGI_t      eCGI;
//		//M2AP_PLMN_Identity_t     pLMN_Identity;
//		//M2AP_EUTRANCellIdentifier_t      eUTRANcellIdentifier
//	//M2AP_MBSFN_SynchronisationArea_ID_t      mbsfnSynchronisationArea;
//	//M2AP_MBMS_Service_Area_ID_List_t         mbmsServiceAreaList;
//
//
//	ASN_SEQUENCE_ADD(&ie->value.choice.ENB_MBMS_Configuration_data_List.list,mbms_configuration_data_list_item_ies);
//
// }
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  LOG_W(M2AP,"m2ap_eNB_data_p->assoc_id %d\n",m2ap_eNB_data_p->assoc_id);
//  /* encode */
//  if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
//    LOG_E(M2AP, "Failed to encode M2 setup request\n");
//    return -1;
//  }
//
// 
//  LOG_W(M2AP,"pdu.present %d\n",pdu.present);
// // MSC_LOG_TX_MESSAGE(
// // MSC_M2AP_eNB,
// // MSC_M2AP_MCE,
// // (const char *)buffer,
// // len,
// // MSC_AS_TIME_FMT" M2_SETUP_REQUEST initiatingMessage MCEname %s",
// // 0,0,//MSC_AS_TIME_ARGS(ctxt_pP),
// // m2ap_eNB_data_p->ENBname);
//
//  m2ap_eNB_itti_send_sctp_data_req(instance_p, m2ap_eNB_data_p->assoc_id, buffer, len, 0);
//
//  return 0;
//
//
//}

//int m2ap_MCE_generate_m2_setup_response(m2ap_eNB_instance_t *instance_p, m2ap_eNB_data_t *m2ap_eNB_data_p)
//{
//  M2AP_M2AP_PDU_t                     pdu;
//  M2AP_M2SetupResponse_t              *out;
//  M2AP_M2SetupResponse_Ies_t          *ie;
//  //M2AP_PLMN_Identity_t                *plmn;
//  //ServedCells__Member                 *servedCellMember;
//  //M2AP_GU_Group_ID_t                  *gu;
//
//  uint8_t  *buffer;
//  uint32_t  len;
//  int       ret = 0;
//
//  DevAssert(instance_p != NULL);
//  DevAssert(m2ap_eNB_data_p != NULL);
//
//  /* Prepare the M2AP message to encode */
//  memset(&pdu, 0, sizeof(pdu));
//  pdu.present = M2AP_M2AP_PDU_PR_successfulOutcome;
//  pdu.choice.successfulOutcome.procedureCode = M2AP_ProcedureCode_id_m2Setup;
//  pdu.choice.successfulOutcome.criticality = M2AP_Criticality_reject;
//  pdu.choice.successfulOutcome.value.present = M2AP_SuccessfulOutcome__value_PR_M2SetupResponse;
//  out = &pdu.choice.successfulOutcome.value.choice.M2SetupResponse;
//
//  /* mandatory */
//  ie = (M2AP_M2SetupResponse_Ies_t *)calloc(1, sizeof(M2AP_M2SetupResponse_Ies_t));
//  //ie->id = M2AP_ProtocolIE_ID_id_GlobalENB_ID;
//  //ie->criticality = M2AP_Criticality_reject;
//  //ie->value.present = M2AP_M2SetupResponse_IEs__value_PR_GlobalENB_ID;
//  //MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
//  //                  &ie->value.choice.GlobalENB_ID.pLMN_Identity);
//  //ie->value.choice.GlobalENB_ID.eNB_ID.present = M2AP_ENB_ID_PR_macro_eNB_ID;
//  //MACRO_ENB_ID_TO_BIT_STRING(instance_p->eNB_id,
//  //                           &ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID);
//  //M2AP_INFO("%d -> %02x%02x%02x\n", instance_p->eNB_id,
//  //          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[0],
//  //          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[1],
//  //          ie->value.choice.GlobalENB_ID.eNB_ID.choice.macro_eNB_ID.buf[2]);
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_M2SetupResponse_Ies_t *)calloc(1, sizeof(M2AP_M2SetupResponse_Ies_t));
//  //ie->id = M2AP_ProtocolIE_ID_id_ServedCells;
//  //ie->criticality = M2AP_Criticality_reject;
//  //ie->value.present = M2AP_M2SetupResponse_IEs__value_PR_ServedCells;
//  //{
//  //  for (int i = 0; i<instance_p->num_cc; i++){
//  //    servedCellMember = (ServedCells__Member *)calloc(1,sizeof(ServedCells__Member));
//  //    {
//  //      servedCellMember->servedCellInfo.pCI = instance_p->Nid_cell[i];
//
//  //      MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
//  //                    &servedCellMember->servedCellInfo.cellId.pLMN_Identity);
//  //      MACRO_ENB_ID_TO_CELL_IDENTITY(instance_p->eNB_id,0,
//  //                                 &servedCellMember->servedCellInfo.cellId.eUTRANcellIdentifier);
//
//  //      INT16_TO_OCTET_STRING(instance_p->tac, &servedCellMember->servedCellInfo.tAC);
//  //      plmn = (M2AP_PLMN_Identity_t *)calloc(1,sizeof(M2AP_PLMN_Identity_t));
//  //      {
//  //        MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length, plmn);
//  //        ASN_SEQUENCE_ADD(&servedCellMember->servedCellInfo.broadcastPLMNs.list, plmn);
//  //      }
//
//  //      if (instance_p->frame_type[i] == FDD) {
//  //        servedCellMember->servedCellInfo.eUTRA_Mode_Info.present = M2AP_EUTRA_Mode_Info_PR_fDD;
//  //        servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_EARFCN = instance_p->fdd_earfcn_DL[i];
//  //        servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_EARFCN = instance_p->fdd_earfcn_UL[i];
//  //        switch (instance_p->N_RB_DL[i]) {
//  //          case 6:
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw6;
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw6;
//  //            break;
//  //          case 15:
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw15;
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw15;
//  //            break;
//  //          case 25:
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw25;
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw25;
//  //            break;
//  //          case 50:
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw50;
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw50;
//  //            break;
//  //          case 75:
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw75;
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw75;
//  //            break;
//  //          case 100:
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.uL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw100;
//  //            servedCellMember->servedCellInfo.eUTRA_Mode_Info.choice.fDD.dL_Transmission_Bandwidth = M2AP_Transmission_Bandwidth_bw100;
//  //            break;
//  //          default:
//  //            AssertFatal(0,"Failed: Check value for N_RB_DL/N_RB_UL");
//  //            break;
//  //        }
//  //      }
//  //      else {
//  //        AssertFatal(0,"M2Setupresponse not supported for TDD!");
//  //      }
//  //    }
//  //    ASN_SEQUENCE_ADD(&ie->value.choice.ServedCells.list, servedCellMember);
//  //  }
//  //}
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_M2SetupResponse_Ies_t *)calloc(1, sizeof(M2AP_M2SetupResponse_Ies_t));
//  //ie->id = M2AP_ProtocolIE_ID_id_GUGroupIDList;
//  //ie->criticality = M2AP_Criticality_reject;
//  //ie->value.present = M2AP_M2SetupResponse_IEs__value_PR_GUGroupIDList;
//  //{
//  //  gu = (M2AP_GU_Group_ID_t *)calloc(1, sizeof(M2AP_GU_Group_ID_t));
//  //  {
//  //    MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
//  //                  &gu->pLMN_Identity);
//  //    //@TODO: consider to update this value
//  //    INT16_TO_OCTET_STRING(0, &gu->mME_Group_ID);
//  //  }
//  //  ASN_SEQUENCE_ADD(&ie->value.choice.GUGroupIDList.list, gu);
//  //}
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
//    M2AP_ERROR("Failed to encode M2 setup response\n");
//    return -1;
//  }
//
//  m2ap_eNB_data_p->state = M2AP_ENB_STATE_READY;
//
//  //MSC_LOG_TX_MESSAGE (MSC_M2AP_SRC_ENB, MSC_M2AP_TARGET_ENB, NULL, 0, "0 M2Setup/successfulOutcome assoc_id %u", m2ap_eNB_data_p->assoc_id);
//
//  m2ap_eNB_itti_send_sctp_data_req(instance_p->instance, m2ap_eNB_data_p->assoc_id, buffer, len, 0);
//
//  return ret;
//}

//int m2ap_MCE_generate_m2_setup_failure(instance_t instance,
//                                       uint32_t assoc_id,
//                                       M2AP_Cause_PR cause_type,
//                                       long cause_value,
//                                       long time_to_wait)
//{
//  M2AP_M2AP_PDU_t                     pdu;
//  M2AP_M2SetupFailure_t              *out;
//  M2AP_M2SetupFailure_Ies_t          *ie;
//
//  uint8_t  *buffer;
//  uint32_t  len;
//  int       ret = 0;
//
//  /* Prepare the M2AP message to encode */
//  memset(&pdu, 0, sizeof(pdu));
//  pdu.present = M2AP_M2AP_PDU_PR_unsuccessfulOutcome;
//  pdu.choice.unsuccessfulOutcome.procedureCode = M2AP_ProcedureCode_id_m2Setup;
//  pdu.choice.unsuccessfulOutcome.criticality = M2AP_Criticality_reject;
//  pdu.choice.unsuccessfulOutcome.value.present = M2AP_UnsuccessfulOutcome__value_PR_M2SetupFailure;
//  out = &pdu.choice.unsuccessfulOutcome.value.choice.M2SetupFailure;
//
//  /* mandatory */
//  ie = (M2AP_M2SetupFailure_Ies_t *)calloc(1, sizeof(M2AP_M2SetupFailure_Ies_t));
//  //ie->id = M2AP_ProtocolIE_ID_id_Cause;
//  //ie->criticality = M2AP_Criticality_ignore;
//  //ie->value.present = M2AP_M2SetupFailure_IEs__value_PR_Cause;
//
//  //m2ap_eNB_set_cause (&ie->value.choice.Cause, cause_type, cause_value);
//
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* optional: consider to handle this later */
//  ie = (M2AP_M2SetupFailure_Ies_t *)calloc(1, sizeof(M2AP_M2SetupFailure_Ies_t));
//  //ie->id = M2AP_ProtocolIE_ID_id_TimeToWait;
//  //ie->criticality = M2AP_Criticality_ignore;
//  //ie->value.present = M2AP_M2SetupFailure_IEs__value_PR_TimeToWait;
//
//  //if (time_to_wait > -1) {
//  //  ie->value.choice.TimeToWait = time_to_wait;
//  //}
//
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  if (m2ap_encode_pdu(&pdu, &buffer, &len) < 0) {
//    M2AP_ERROR("Failed to encode M2 setup failure\n");
//    return -1;
//  }
//
//  //MSC_LOG_TX_MESSAGE (MSC_M2AP_SRC_ENB,
//  //                    MSC_M2AP_TARGET_ENB, NULL, 0,
//  //                    "0 M2Setup/unsuccessfulOutcome  assoc_id %u cause %u value %u",
//  //                    assoc_id, cause_type, cause_value);
//
//  m2ap_eNB_itti_send_sctp_data_req(instance, assoc_id, buffer, len, 0);
//
//  return ret;
//}

int m2ap_eNB_set_cause (M2AP_Cause_t * cause_p,
                        M2AP_Cause_PR cause_type,
                        long cause_value)
{

  DevAssert (cause_p != NULL);
  cause_p->present = cause_type;

  switch (cause_type) {
  case M2AP_Cause_PR_radioNetwork:
    cause_p->choice.misc = cause_value;
    break;

  case M2AP_Cause_PR_transport:
    cause_p->choice.misc = cause_value;
    break;

  case M2AP_Cause_PR_protocol:
    cause_p->choice.misc = cause_value;
    break;

  case M2AP_Cause_PR_misc:
    cause_p->choice.misc = cause_value;
    break;

  default:
    return -1;
  }

  return 0;
}

//int m2ap_eNB_generate_m2_handover_request (m2ap_eNB_instance_t *instance_p, m2ap_eNB_data_t *m2ap_eNB_data_p,
//                                           m2ap_handover_req_t *m2ap_handover_req, int ue_id)
//{
//
//  M2AP_M2AP_PDU_t                     pdu;
//  M2AP_HandoverRequest_t              *out;
//  M2AP_HandoverRequest_IEs_t          *ie;
//  M2AP_E_RABs_ToBeSetup_ItemIEs_t     *e_RABS_ToBeSetup_ItemIEs;
//  M2AP_E_RABs_ToBeSetup_Item_t        *e_RABs_ToBeSetup_Item;
//  M2AP_LastVisitedCell_Item_t         *lastVisitedCell_Item;
//
//  uint8_t  *buffer;
//  uint32_t  len;
//  int       ret = 0;
//
//  DevAssert(instance_p != NULL);
//  DevAssert(m2ap_eNB_data_p != NULL);
//
//  /* Prepare the M2AP handover message to encode */
//  memset(&pdu, 0, sizeof(pdu));
//  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
//  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_handoverPreparation;
//  pdu.choice.initiatingMessage.criticality = M2AP_Criticality_reject;
//  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_HandoverRequest;
//  out = &pdu.choice.initiatingMessage.value.choice.HandoverRequest;
//
//  /* mandatory */
//  ie = (M2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(M2AP_HandoverRequest_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_Old_eNB_UE_M2AP_ID;
//  ie->criticality = M2AP_Criticality_reject;
//  ie->value.present = M2AP_HandoverRequest_IEs__value_PR_UE_M2AP_ID;
//  ie->value.choice.UE_M2AP_ID = m2ap_id_get_id_source(&instance_p->id_manager, ue_id);
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(M2AP_HandoverRequest_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_Cause;
//  ie->criticality = M2AP_Criticality_ignore;
//  ie->value.present = M2AP_HandoverRequest_IEs__value_PR_Cause;
//  ie->value.choice.Cause.present = M2AP_Cause_PR_radioNetwork;
//  ie->value.choice.Cause.choice.radioNetwork = M2AP_CauseRadioNetwork_handover_desirable_for_radio_reasons;
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(M2AP_HandoverRequest_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_TargetCell_ID;
//  ie->criticality = M2AP_Criticality_reject;
//  ie->value.present = M2AP_HandoverRequest_IEs__value_PR_ECGI;
//  MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
//                       &ie->value.choice.ECGI.pLMN_Identity);
//  MACRO_ENB_ID_TO_CELL_IDENTITY(m2ap_eNB_data_p->eNB_id, 0, &ie->value.choice.ECGI.eUTRANcellIdentifier);
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(M2AP_HandoverRequest_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_GUMMEI_ID;
//  ie->criticality = M2AP_Criticality_reject;
//  ie->value.present = M2AP_HandoverRequest_IEs__value_PR_GUMMEI;
//  MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
//                       &ie->value.choice.GUMMEI.gU_Group_ID.pLMN_Identity);
//  //@TODO: consider to update these values
//  INT16_TO_OCTET_STRING(m2ap_handover_req->ue_gummei.mme_group_id, &ie->value.choice.GUMMEI.gU_Group_ID.mME_Group_ID);
//  MME_CODE_TO_OCTET_STRING(m2ap_handover_req->ue_gummei.mme_code, &ie->value.choice.GUMMEI.mME_Code);
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(M2AP_HandoverRequest_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_UE_ContextInformation;
//  ie->criticality = M2AP_Criticality_reject;
//  ie->value.present = M2AP_HandoverRequest_IEs__value_PR_UE_ContextInformation;
//  //@TODO: consider to update this value
//  ie->value.choice.UE_ContextInformation.mME_UE_S1AP_ID = m2ap_handover_req->mme_ue_s1ap_id;
//
//  KENB_STAR_TO_BIT_STRING(m2ap_handover_req->kenb,&ie->value.choice.UE_ContextInformation.aS_SecurityInformation.key_eNodeB_star);
//
//  if (m2ap_handover_req->kenb_ncc >=0) { // Check this condition
//    ie->value.choice.UE_ContextInformation.aS_SecurityInformation.nextHopChainingCount = m2ap_handover_req->kenb_ncc;
//  }
//  else {
//    ie->value.choice.UE_ContextInformation.aS_SecurityInformation.nextHopChainingCount = 1;
//  }
//
//  ENCRALG_TO_BIT_STRING(m2ap_handover_req->security_capabilities.encryption_algorithms,
//              &ie->value.choice.UE_ContextInformation.uESecurityCapabilities.encryptionAlgorithms);
//
//  INTPROTALG_TO_BIT_STRING(m2ap_handover_req->security_capabilities.integrity_algorithms,
//              &ie->value.choice.UE_ContextInformation.uESecurityCapabilities.integrityProtectionAlgorithms);
//
//  //@TODO: update with proper UEAMPR
//  UEAGMAXBITRTD_TO_ASN_PRIMITIVES(3L,&ie->value.choice.UE_ContextInformation.uEaggregateMaximumBitRate.uEaggregateMaximumBitRateDownlink);
//  UEAGMAXBITRTU_TO_ASN_PRIMITIVES(6L,&ie->value.choice.UE_ContextInformation.uEaggregateMaximumBitRate.uEaggregateMaximumBitRateUplink);
//  {
//    for (int i=0;i<m2ap_handover_req->nb_e_rabs_tobesetup;i++) {
//      e_RABS_ToBeSetup_ItemIEs = (M2AP_E_RABs_ToBeSetup_ItemIEs_t *)calloc(1,sizeof(M2AP_E_RABs_ToBeSetup_ItemIEs_t));
//      e_RABS_ToBeSetup_ItemIEs->id = M2AP_ProtocolIE_ID_id_E_RABs_ToBeSetup_Item;
//      e_RABS_ToBeSetup_ItemIEs->criticality = M2AP_Criticality_ignore;
//      e_RABS_ToBeSetup_ItemIEs->value.present = M2AP_E_RABs_ToBeSetup_ItemIEs__value_PR_E_RABs_ToBeSetup_Item;
//      e_RABs_ToBeSetup_Item = &e_RABS_ToBeSetup_ItemIEs->value.choice.E_RABs_ToBeSetup_Item;
//      {
//        e_RABs_ToBeSetup_Item->e_RAB_ID = m2ap_handover_req->e_rabs_tobesetup[i].e_rab_id;
//        e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.qCI = m2ap_handover_req->e_rab_param[i].qos.qci;
//        e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.priorityLevel = m2ap_handover_req->e_rab_param[i].qos.allocation_retention_priority.priority_level;
//        e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionCapability = m2ap_handover_req->e_rab_param[i].qos.allocation_retention_priority.pre_emp_capability;
//        e_RABs_ToBeSetup_Item->e_RAB_Level_QoS_Parameters.allocationAndRetentionPriority.pre_emptionVulnerability = m2ap_handover_req->e_rab_param[i].qos.allocation_retention_priority.pre_emp_vulnerability;
//        e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.size = (uint8_t)(m2ap_handover_req->e_rabs_tobesetup[i].eNB_addr.length/8);
//        e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.bits_unused = m2ap_handover_req->e_rabs_tobesetup[i].eNB_addr.length%8;
//        e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.buf =
//                        calloc(1,e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.size);
//
//        memcpy (e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.buf,
//                        m2ap_handover_req->e_rabs_tobesetup[i].eNB_addr.buffer,
//                        e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.transportLayerAddress.size);
//
//        INT32_TO_OCTET_STRING(m2ap_handover_req->e_rabs_tobesetup[i].gtp_teid,&e_RABs_ToBeSetup_Item->uL_GTPtunnelEndpoint.gTP_TEID);
//      }
//      ASN_SEQUENCE_ADD(&ie->value.choice.UE_ContextInformation.e_RABs_ToBeSetup_List.list, e_RABS_ToBeSetup_ItemIEs);
//    }
//  }
//
//  OCTET_STRING_fromBuf(&ie->value.choice.UE_ContextInformation.rRC_Context, (char*) m2ap_handover_req->rrc_buffer, m2ap_handover_req->rrc_buffer_size);
//
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_HandoverRequest_IEs_t *)calloc(1, sizeof(M2AP_HandoverRequest_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_UE_HistoryInformation;
//  ie->criticality = M2AP_Criticality_ignore;
//  ie->value.present = M2AP_HandoverRequest_IEs__value_PR_UE_HistoryInformation;
//  //@TODO: consider to update this value
//  {
//   lastVisitedCell_Item = (M2AP_LastVisitedCell_Item_t *)calloc(1, sizeof(M2AP_LastVisitedCell_Item_t));
//   lastVisitedCell_Item->present = M2AP_LastVisitedCell_Item_PR_e_UTRAN_Cell;
//   MCC_MNC_TO_PLMNID(instance_p->mcc, instance_p->mnc, instance_p->mnc_digit_length,
//                       &lastVisitedCell_Item->choice.e_UTRAN_Cell.global_Cell_ID.pLMN_Identity);
//   MACRO_ENB_ID_TO_CELL_IDENTITY(0, 0, &lastVisitedCell_Item->choice.e_UTRAN_Cell.global_Cell_ID.eUTRANcellIdentifier);
//   lastVisitedCell_Item->choice.e_UTRAN_Cell.cellType.cell_Size = M2AP_Cell_Size_small;
//   lastVisitedCell_Item->choice.e_UTRAN_Cell.time_UE_StayedInCell = 2;
//   ASN_SEQUENCE_ADD(&ie->value.choice.UE_HistoryInformation.list, lastVisitedCell_Item);
//  }
//
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  if (m2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
//    M2AP_ERROR("Failed to encode X2 handover request\n");
//    abort();
//    return -1;
//  }
//
//  MSC_LOG_TX_MESSAGE (MSC_M2AP_SRC_ENB, MSC_M2AP_TARGET_ENB, NULL, 0, "0 X2Handover/initiatingMessage assoc_id %u", m2ap_eNB_data_p->assoc_id);
//
//  m2ap_eNB_itti_send_sctp_data_req(instance_p->instance, m2ap_eNB_data_p->assoc_id, buffer, len, 1);
//
//  return ret;
//}
//
//int m2ap_eNB_generate_m2_handover_request_ack (m2ap_eNB_instance_t *instance_p, m2ap_eNB_data_t *m2ap_eNB_data_p,
//                                               m2ap_handover_req_ack_t *m2ap_handover_req_ack)
//{
//
//  M2AP_M2AP_PDU_t                        pdu;
//  M2AP_HandoverRequestAcknowledge_t      *out;
//  M2AP_HandoverRequestAcknowledge_IEs_t  *ie;
//  M2AP_E_RABs_Admitted_ItemIEs_t         *e_RABS_Admitted_ItemIEs;
//  M2AP_E_RABs_Admitted_Item_t            *e_RABs_Admitted_Item;
//  int                                    ue_id;
//  int                                    id_source;
//  int                                    id_target;
//
//  uint8_t  *buffer;
//  uint32_t  len;
//  int       ret = 0;
//
//  DevAssert(instance_p != NULL);
//  DevAssert(m2ap_eNB_data_p != NULL);
//
//  ue_id     = m2ap_handover_req_ack->m2_id_target;
//  id_source = m2ap_id_get_id_source(&instance_p->id_manager, ue_id);
//  id_target = ue_id;
//
//  /* Prepare the M2AP handover message to encode */
//  memset(&pdu, 0, sizeof(pdu));
//  pdu.present = M2AP_M2AP_PDU_PR_successfulOutcome;
//  pdu.choice.successfulOutcome.procedureCode = M2AP_ProcedureCode_id_handoverPreparation;
//  pdu.choice.successfulOutcome.criticality = M2AP_Criticality_reject;
//  pdu.choice.successfulOutcome.value.present = M2AP_SuccessfulOutcome__value_PR_HandoverRequestAcknowledge;
//  out = &pdu.choice.successfulOutcome.value.choice.HandoverRequestAcknowledge;
//
//  /* mandatory */
//  ie = (M2AP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(M2AP_HandoverRequestAcknowledge_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_Old_eNB_UE_M2AP_ID;
//  ie->criticality = M2AP_Criticality_ignore;
//  ie->value.present = M2AP_HandoverRequestAcknowledge_IEs__value_PR_UE_M2AP_ID;
//  ie->value.choice.UE_M2AP_ID = id_source;
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(M2AP_HandoverRequestAcknowledge_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_New_eNB_UE_M2AP_ID;
//  ie->criticality = M2AP_Criticality_ignore;
//  ie->value.present = M2AP_HandoverRequestAcknowledge_IEs__value_PR_UE_M2AP_ID_1;
//  ie->value.choice.UE_M2AP_ID_1 = id_target;
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(M2AP_HandoverRequestAcknowledge_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_E_RABs_Admitted_List;
//  ie->criticality = M2AP_Criticality_ignore;
//  ie->value.present = M2AP_HandoverRequestAcknowledge_IEs__value_PR_E_RABs_Admitted_List;
//
//  {
//      for (int i=0;i<m2ap_handover_req_ack->nb_e_rabs_tobesetup;i++) {
//        e_RABS_Admitted_ItemIEs = (M2AP_E_RABs_Admitted_ItemIEs_t *)calloc(1,sizeof(M2AP_E_RABs_Admitted_ItemIEs_t));
//        e_RABS_Admitted_ItemIEs->id = M2AP_ProtocolIE_ID_id_E_RABs_Admitted_Item;
//        e_RABS_Admitted_ItemIEs->criticality = M2AP_Criticality_ignore;
//        e_RABS_Admitted_ItemIEs->value.present = M2AP_E_RABs_Admitted_ItemIEs__value_PR_E_RABs_Admitted_Item;
//        e_RABs_Admitted_Item = &e_RABS_Admitted_ItemIEs->value.choice.E_RABs_Admitted_Item;
//        {
//          e_RABs_Admitted_Item->e_RAB_ID = m2ap_handover_req_ack->e_rabs_tobesetup[i].e_rab_id;
//        }
//        ASN_SEQUENCE_ADD(&ie->value.choice.E_RABs_Admitted_List.list, e_RABS_Admitted_ItemIEs);
//      }
//  }
//
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(M2AP_HandoverRequestAcknowledge_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_TargeteNBtoSource_eNBTransparentContainer;
//  ie->criticality = M2AP_Criticality_ignore;
//  ie->value.present = M2AP_HandoverRequestAcknowledge_IEs__value_PR_TargeteNBtoSource_eNBTransparentContainer;
//
//  OCTET_STRING_fromBuf(&ie->value.choice.TargeteNBtoSource_eNBTransparentContainer, (char*) m2ap_handover_req_ack->rrc_buffer, m2ap_handover_req_ack->rrc_buffer_size);
//
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  if (m2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
//    M2AP_ERROR("Failed to encode X2 handover response\n");
//    abort();
//    return -1;
//  }
//
//  MSC_LOG_TX_MESSAGE (MSC_M2AP_SRC_ENB, MSC_M2AP_TARGET_ENB, NULL, 0, "0 X2Handover/successfulOutcome assoc_id %u", m2ap_eNB_data_p->assoc_id);
//
//  m2ap_eNB_itti_send_sctp_data_req(instance_p->instance, m2ap_eNB_data_p->assoc_id, buffer, len, 1);
//
//  return ret;
//}
//
//int m2ap_eNB_generate_m2_ue_context_release (m2ap_eNB_instance_t *instance_p, m2ap_eNB_data_t *m2ap_eNB_data_p, m2ap_ue_context_release_t *m2ap_ue_context_release)
//{
//
//  M2AP_M2AP_PDU_t                pdu;
//  M2AP_UEContextRelease_t        *out;
//  M2AP_UEContextRelease_IEs_t    *ie;
//  int                            ue_id;
//  int                            id_source;
//  int                            id_target;
//
//  uint8_t  *buffer;
//  uint32_t  len;
//  int       ret = 0;
//
//  DevAssert(instance_p != NULL);
//  DevAssert(m2ap_eNB_data_p != NULL);
//
//  ue_id = m2ap_find_id_from_rnti(&instance_p->id_manager, m2ap_ue_context_release->rnti);
//  if (ue_id == -1) {
//    M2AP_ERROR("could not find UE %x\n", m2ap_ue_context_release->rnti);
//    exit(1);
//  }
//  id_source = m2ap_id_get_id_source(&instance_p->id_manager, ue_id);
//  id_target = ue_id;
//
//  /* Prepare the M2AP ue context relase message to encode */
//  memset(&pdu, 0, sizeof(pdu));
//  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
//  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_uEContextRelease;
//  pdu.choice.initiatingMessage.criticality = M2AP_Criticality_ignore;
//  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_UEContextRelease;
//  out = &pdu.choice.initiatingMessage.value.choice.UEContextRelease;
//
//  /* mandatory */
//  ie = (M2AP_UEContextRelease_IEs_t *)calloc(1, sizeof(M2AP_UEContextRelease_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_Old_eNB_UE_M2AP_ID;
//  ie->criticality = M2AP_Criticality_reject;
//  ie->value.present = M2AP_UEContextRelease_IEs__value_PR_UE_M2AP_ID;
//  ie->value.choice.UE_M2AP_ID = id_source;
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* mandatory */
//  ie = (M2AP_UEContextRelease_IEs_t *)calloc(1, sizeof(M2AP_UEContextRelease_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_New_eNB_UE_M2AP_ID;
//  ie->criticality = M2AP_Criticality_reject;
//  ie->value.present = M2AP_UEContextRelease_IEs__value_PR_UE_M2AP_ID_1;
//  ie->value.choice.UE_M2AP_ID_1 = id_target;
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  if (m2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
//    M2AP_ERROR("Failed to encode X2 UE Context Release\n");
//    abort();
//    return -1;
//  }
//
//  MSC_LOG_TX_MESSAGE (MSC_M2AP_SRC_ENB, MSC_M2AP_TARGET_ENB, NULL, 0, "0 X2UEContextRelease/initiatingMessage assoc_id %u", m2ap_eNB_data_p->assoc_id);
//
//  m2ap_eNB_itti_send_sctp_data_req(instance_p->instance, m2ap_eNB_data_p->assoc_id, buffer, len, 1);
//
//  return ret;
//}
//
//int m2ap_eNB_generate_m2_handover_cancel (m2ap_eNB_instance_t *instance_p, m2ap_eNB_data_t *m2ap_eNB_data_p,
//                                          int m2_ue_id,
//                                          m2ap_handover_cancel_cause_t cause)
//{
//  M2AP_M2AP_PDU_t              pdu;
//  M2AP_HandoverCancel_t        *out;
//  M2AP_HandoverCancel_IEs_t    *ie;
//  int                          ue_id;
//  int                          id_source;
//  int                          id_target;
//
//  uint8_t  *buffer;
//  uint32_t  len;
//  int       ret = 0;
//
//  DevAssert(instance_p != NULL);
//  DevAssert(m2ap_eNB_data_p != NULL);
//
//  ue_id = m2_ue_id;
//  id_source = ue_id;
//  id_target = m2ap_id_get_id_target(&instance_p->id_manager, ue_id);
//
//  /* Prepare the M2AP handover cancel message to encode */
//  memset(&pdu, 0, sizeof(pdu));
//  pdu.present = M2AP_M2AP_PDU_PR_initiatingMessage;
//  pdu.choice.initiatingMessage.procedureCode = M2AP_ProcedureCode_id_handoverCancel;
//  pdu.choice.initiatingMessage.criticality = M2AP_Criticality_ignore;
//  pdu.choice.initiatingMessage.value.present = M2AP_InitiatingMessage__value_PR_HandoverCancel;
//  out = &pdu.choice.initiatingMessage.value.choice.HandoverCancel;
//
//  /* mandatory */
//  ie = (M2AP_HandoverCancel_IEs_t *)calloc(1, sizeof(M2AP_HandoverCancel_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_Old_eNB_UE_M2AP_ID;
//  ie->criticality = M2AP_Criticality_reject;
//  ie->value.present = M2AP_HandoverCancel_IEs__value_PR_UE_M2AP_ID;
//  ie->value.choice.UE_M2AP_ID = id_source;
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  /* optional */
//  if (id_target != -1) {
//    ie = (M2AP_HandoverCancel_IEs_t *)calloc(1, sizeof(M2AP_HandoverCancel_IEs_t));
//    ie->id = M2AP_ProtocolIE_ID_id_New_eNB_UE_M2AP_ID;
//    ie->criticality = M2AP_Criticality_ignore;
//    ie->value.present = M2AP_HandoverCancel_IEs__value_PR_UE_M2AP_ID_1;
//    ie->value.choice.UE_M2AP_ID_1 = id_target;
//    ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//  }
//
//  /* mandatory */
//  ie = (M2AP_HandoverCancel_IEs_t *)calloc(1, sizeof(M2AP_HandoverCancel_IEs_t));
//  ie->id = M2AP_ProtocolIE_ID_id_Cause;
//  ie->criticality = M2AP_Criticality_ignore;
//  ie->value.present = M2AP_HandoverCancel_IEs__value_PR_Cause;
//  switch (cause) {
//  case M2AP_T_RELOC_PREP_TIMEOUT:
//    ie->value.choice.Cause.present = M2AP_Cause_PR_radioNetwork;
//    ie->value.choice.Cause.choice.radioNetwork =
//      M2AP_CauseRadioNetwork_trelocprep_expiry;
//    break;
//  case M2AP_TX2_RELOC_OVERALL_TIMEOUT:
//    ie->value.choice.Cause.present = M2AP_Cause_PR_radioNetwork;
//    ie->value.choice.Cause.choice.radioNetwork =
//      M2AP_CauseRadioNetwork_tx2relocoverall_expiry;
//    break;
//  default:
//    /* we can't come here */
//    M2AP_ERROR("unhandled cancel cause\n");
//    exit(1);
//  }
//  ASN_SEQUENCE_ADD(&out->protocolIEs.list, ie);
//
//  if (m2ap_eNB_encode_pdu(&pdu, &buffer, &len) < 0) {
//    M2AP_ERROR("Failed to encode X2 Handover Cancel\n");
//    abort();
//    return -1;
//  }
//
//  MSC_LOG_TX_MESSAGE (MSC_M2AP_SRC_ENB, MSC_M2AP_TARGET_ENB, NULL, 0, "0 X2HandoverCancel/initiatingMessage assoc_id %u", m2ap_eNB_data_p->assoc_id);
//
//  m2ap_eNB_itti_send_sctp_data_req(instance_p->instance, m2ap_eNB_data_p->assoc_id, buffer, len, 1);
//
//  return ret;
//}
