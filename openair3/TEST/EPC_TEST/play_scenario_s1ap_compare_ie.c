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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*
                                play_scenario_s1ap_compare_ie.c
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
#include "tree.h"
#include "queue.h"


#include "intertask_interface.h"
#include "timer.h"
#include "platform_types.h"
#include "assertions.h"
#include "conversions.h"
#include "s1ap_common.h"
#include "play_scenario_s1ap_eNB_defs.h"
#include "play_scenario.h"
#include "msc.h"
//------------------------------------------------------------------------------
extern et_scenario_t  *g_scenario;
extern uint32_t        g_constraints;
//------------------------------------------------------------------------------

long et_s1ap_ies_is_matching(const S1AP_PDU_PR present, s1ap_message * const m1, s1ap_message * const m2, const uint32_t constraints)
{
  long ret = 0;
  AssertFatal(m1 != NULL, "bad parameter m1");
  AssertFatal(m2 != NULL, "bad parameter m2");
  AssertFatal((present == S1AP_PDU_PR_initiatingMessage) ||
              (present == S1AP_PDU_PR_successfulOutcome) ||
              (present == S1AP_PDU_PR_unsuccessfulOutcome) , "Bad parameter S1AP_PDU_PR present ");
  AssertFatal( m1->procedureCode == m2->procedureCode, "Bad parameters: no matching procedure codes");


  // some cases can never occur since uplink only.
  switch (m1->procedureCode) {
    case  S1ap_ProcedureCode_id_HandoverPreparation:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_handoverrequiredies(
                &m1->msg.s1ap_HandoverRequiredIEs,
                &m2->msg.s1ap_HandoverRequiredIEs);
      } else {
        ret = s1ap_compare_s1ap_handovercommandies(
                &m1->msg.s1ap_HandoverCommandIEs,
                &m2->msg.s1ap_HandoverCommandIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_HandoverResourceAllocation:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_handoverrequesties(
                &m1->msg.s1ap_HandoverRequestIEs,
                &m2->msg.s1ap_HandoverRequestIEs);
      } else if (present == S1AP_PDU_PR_successfulOutcome) {
        ret = s1ap_compare_s1ap_handoverrequestacknowledgeies(
                &m1->msg.s1ap_HandoverRequestAcknowledgeIEs,
                &m2->msg.s1ap_HandoverRequestAcknowledgeIEs);
      } else if (present == S1AP_PDU_PR_unsuccessfulOutcome) {
        ret = s1ap_compare_s1ap_handoverfailureies(
                &m1->msg.s1ap_HandoverFailureIEs,
                &m2->msg.s1ap_HandoverFailureIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_HandoverNotification:
        ret = s1ap_compare_s1ap_handovernotifyies(
                &m1->msg.s1ap_HandoverNotifyIEs,
                &m2->msg.s1ap_HandoverNotifyIEs);
      break;

    case  S1ap_ProcedureCode_id_PathSwitchRequest:
      ret = s1ap_compare_s1ap_pathswitchrequesties(
              &m1->msg.s1ap_PathSwitchRequestIEs,
              &m2->msg.s1ap_PathSwitchRequestIEs);
      break;

    case  S1ap_ProcedureCode_id_HandoverCancel:
      ret = s1ap_compare_s1ap_handovercancelies(
              &m1->msg.s1ap_HandoverCancelIEs,
              &m2->msg.s1ap_HandoverCancelIEs);
      break;

    case  S1ap_ProcedureCode_id_E_RABSetup:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_e_rabsetuprequesties(
                &m1->msg.s1ap_E_RABSetupRequestIEs,
                &m2->msg.s1ap_E_RABSetupRequestIEs);
      } else  {
        ret = s1ap_compare_s1ap_e_rabsetupresponseies(
                &m1->msg.s1ap_E_RABSetupResponseIEs,
                &m2->msg.s1ap_E_RABSetupResponseIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_E_RABModify:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_e_rabmodifyrequesties(
                &m1->msg.s1ap_E_RABModifyRequestIEs,
                &m2->msg.s1ap_E_RABModifyRequestIEs);
      } else  {
        ret = s1ap_compare_s1ap_e_rabmodifyresponseies(
                &m1->msg.s1ap_E_RABModifyResponseIEs,
                &m2->msg.s1ap_E_RABModifyResponseIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_E_RABRelease:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_e_rabreleasecommandies(
                &m1->msg.s1ap_E_RABReleaseCommandIEs,
                &m2->msg.s1ap_E_RABReleaseCommandIEs);
      } else  {
        ret = s1ap_compare_s1ap_e_rabreleaseresponseies(
                &m1->msg.s1ap_E_RABReleaseResponseIEs,
                &m2->msg.s1ap_E_RABReleaseResponseIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_E_RABReleaseIndication:
      ret = s1ap_compare_s1ap_e_rabreleaseindicationies(
              &m1->msg.s1ap_E_RABReleaseIndicationIEs,
              &m2->msg.s1ap_E_RABReleaseIndicationIEs);
      break;

    case  S1ap_ProcedureCode_id_InitialContextSetup:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_initialcontextsetuprequesties(
                &m1->msg.s1ap_InitialContextSetupRequestIEs,
                &m2->msg.s1ap_InitialContextSetupRequestIEs);
      } else  {
        ret = s1ap_compare_s1ap_initialcontextsetupresponseies(
                &m1->msg.s1ap_InitialContextSetupResponseIEs,
                &m2->msg.s1ap_InitialContextSetupResponseIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_Paging:
      ret = s1ap_compare_s1ap_pagingies(
              &m1->msg.s1ap_PagingIEs,
              &m2->msg.s1ap_PagingIEs);
      break;

    case  S1ap_ProcedureCode_id_downlinkNASTransport:
      ret = s1ap_compare_s1ap_downlinknastransporties(
              &m1->msg.s1ap_DownlinkNASTransportIEs,
              &m2->msg.s1ap_DownlinkNASTransportIEs);
      break;

    case  S1ap_ProcedureCode_id_initialUEMessage:
      ret = s1ap_compare_s1ap_initialuemessageies(
              &m1->msg.s1ap_InitialUEMessageIEs,
              &m2->msg.s1ap_InitialUEMessageIEs);
      break;

    case  S1ap_ProcedureCode_id_uplinkNASTransport:
      ret = s1ap_compare_s1ap_uplinknastransporties(
              &m1->msg.s1ap_UplinkNASTransportIEs,
              &m2->msg.s1ap_UplinkNASTransportIEs);
      break;

    case  S1ap_ProcedureCode_id_Reset:
      ret = s1ap_compare_s1ap_reseties(
              &m1->msg.s1ap_ResetIEs,
              &m2->msg.s1ap_ResetIEs);
      break;

    case  S1ap_ProcedureCode_id_ErrorIndication:
      ret = s1ap_compare_s1ap_errorindicationies(
              &m1->msg.s1ap_ErrorIndicationIEs,
              &m2->msg.s1ap_ErrorIndicationIEs);
      break;

    case  S1ap_ProcedureCode_id_NASNonDeliveryIndication:
      ret = s1ap_compare_s1ap_nasnondeliveryindication_ies(
              &m1->msg.s1ap_NASNonDeliveryIndication_IEs,
              &m2->msg.s1ap_NASNonDeliveryIndication_IEs);
      break;

    case  S1ap_ProcedureCode_id_S1Setup:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_s1setuprequesties(
                &m1->msg.s1ap_S1SetupRequestIEs,
                &m2->msg.s1ap_S1SetupRequestIEs);
      } else if (present == S1AP_PDU_PR_successfulOutcome) {
        ret = s1ap_compare_s1ap_s1setupresponseies(
                &m1->msg.s1ap_S1SetupResponseIEs,
                &m2->msg.s1ap_S1SetupResponseIEs);
      } else if (present == S1AP_PDU_PR_unsuccessfulOutcome) {
        ret = s1ap_compare_s1ap_s1setupfailureies(
                &m1->msg.s1ap_S1SetupFailureIEs,
                &m2->msg.s1ap_S1SetupFailureIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_UEContextReleaseRequest:
      ret = s1ap_compare_s1ap_uecontextreleaserequesties(
                &m1->msg.s1ap_UEContextReleaseRequestIEs,
                &m2->msg.s1ap_UEContextReleaseRequestIEs);
      break;

    case  S1ap_ProcedureCode_id_DownlinkS1cdma2000tunneling:
      ret = s1ap_compare_s1ap_downlinks1cdma2000tunnelingies(
                &m1->msg.s1ap_DownlinkS1cdma2000tunnelingIEs,
                &m2->msg.s1ap_DownlinkS1cdma2000tunnelingIEs);
      break;

    case  S1ap_ProcedureCode_id_UplinkS1cdma2000tunneling:
      ret = s1ap_compare_s1ap_uplinks1cdma2000tunnelingies(
                &m1->msg.s1ap_UplinkS1cdma2000tunnelingIEs,
                &m2->msg.s1ap_UplinkS1cdma2000tunnelingIEs);
      break;

    case  S1ap_ProcedureCode_id_UEContextModification:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_uecontextmodificationrequesties(
                &m1->msg.s1ap_UEContextModificationRequestIEs,
                &m2->msg.s1ap_UEContextModificationRequestIEs);
      } else if (present == S1AP_PDU_PR_successfulOutcome) {
        ret = s1ap_compare_s1ap_uecontextmodificationresponseies(
                &m1->msg.s1ap_UEContextModificationResponseIEs,
                &m2->msg.s1ap_UEContextModificationResponseIEs);
      } else if (present == S1AP_PDU_PR_unsuccessfulOutcome) {
        ret = s1ap_compare_s1ap_uecontextmodificationfailureies(
                &m1->msg.s1ap_UEContextModificationFailureIEs,
                &m2->msg.s1ap_UEContextModificationFailureIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_UECapabilityInfoIndication:
      ret = s1ap_compare_s1ap_uecapabilityinfoindicationies(
                &m1->msg.s1ap_UECapabilityInfoIndicationIEs,
                &m2->msg.s1ap_UECapabilityInfoIndicationIEs);
      break;

    case  S1ap_ProcedureCode_id_UEContextRelease:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_uecontextreleasecommandies(
                &m1->msg.s1ap_UEContextReleaseCommandIEs,
                &m2->msg.s1ap_UEContextReleaseCommandIEs);
      } else  {
        ret = s1ap_compare_s1ap_uecontextreleasecompleteies(
                &m1->msg.s1ap_UEContextReleaseCompleteIEs,
                &m2->msg.s1ap_UEContextReleaseCompleteIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_eNBStatusTransfer:
      ret = s1ap_compare_s1ap_enbstatustransferies(
                &m1->msg.s1ap_ENBStatusTransferIEs,
                &m2->msg.s1ap_ENBStatusTransferIEs);
      break;

    case  S1ap_ProcedureCode_id_MMEStatusTransfer:
      ret = s1ap_compare_s1ap_mmestatustransferies(
                &m1->msg.s1ap_MMEStatusTransferIEs,
                &m2->msg.s1ap_MMEStatusTransferIEs);
      break;

    case  S1ap_ProcedureCode_id_DeactivateTrace:
      ret = s1ap_compare_s1ap_deactivatetraceies(
                &m1->msg.s1ap_DeactivateTraceIEs,
                &m2->msg.s1ap_DeactivateTraceIEs);
      break;

    case  S1ap_ProcedureCode_id_TraceStart:
      ret = s1ap_compare_s1ap_tracestarties(
                &m1->msg.s1ap_TraceStartIEs,
                &m2->msg.s1ap_TraceStartIEs);
      break;

    case  S1ap_ProcedureCode_id_TraceFailureIndication:
      ret = s1ap_compare_s1ap_tracefailureindicationies(
                &m1->msg.s1ap_TraceFailureIndicationIEs,
                &m2->msg.s1ap_TraceFailureIndicationIEs);
      break;

    case  S1ap_ProcedureCode_id_ENBConfigurationUpdate:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_enbconfigurationupdateies(
                &m1->msg.s1ap_ENBConfigurationUpdateIEs,
                &m2->msg.s1ap_ENBConfigurationUpdateIEs);
      } else if (present == S1AP_PDU_PR_successfulOutcome) {
        ret = s1ap_compare_s1ap_enbconfigurationupdateacknowledgeies(
                &m1->msg.s1ap_ENBConfigurationUpdateAcknowledgeIEs,
                &m2->msg.s1ap_ENBConfigurationUpdateAcknowledgeIEs);
      } else {
        ret = s1ap_compare_s1ap_enbconfigurationupdatefailureies(
                &m1->msg.s1ap_ENBConfigurationUpdateFailureIEs,
                &m2->msg.s1ap_ENBConfigurationUpdateFailureIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_MMEConfigurationUpdate:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_mmeconfigurationupdateies(
                &m1->msg.s1ap_MMEConfigurationUpdateIEs,
                &m2->msg.s1ap_MMEConfigurationUpdateIEs);
      } else if (present == S1AP_PDU_PR_successfulOutcome) {
        ret = s1ap_compare_s1ap_mmeconfigurationupdateacknowledgeies(
                &m1->msg.s1ap_MMEConfigurationUpdateAcknowledgeIEs,
                &m2->msg.s1ap_MMEConfigurationUpdateAcknowledgeIEs);
      } else {
        ret = s1ap_compare_s1ap_mmeconfigurationupdatefailureies(
                &m1->msg.s1ap_MMEConfigurationUpdateFailureIEs,
                &m2->msg.s1ap_MMEConfigurationUpdateFailureIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_LocationReportingControl:
      ret = s1ap_compare_s1ap_locationreportingcontrolies(
                &m1->msg.s1ap_LocationReportingControlIEs,
                &m2->msg.s1ap_LocationReportingControlIEs);
      break;

    case  S1ap_ProcedureCode_id_LocationReportingFailureIndication:
      ret = s1ap_compare_s1ap_locationreportingfailureindicationies(
                &m1->msg.s1ap_LocationReportingFailureIndicationIEs,
                &m2->msg.s1ap_LocationReportingFailureIndicationIEs);
      break;

    case  S1ap_ProcedureCode_id_LocationReport:
      ret = s1ap_compare_s1ap_locationreporties(
                &m1->msg.s1ap_LocationReportIEs,
                &m2->msg.s1ap_LocationReportIEs);
      break;

    case  S1ap_ProcedureCode_id_OverloadStart:
      ret = s1ap_compare_s1ap_overloadstarties(
                &m1->msg.s1ap_OverloadStartIEs,
                &m2->msg.s1ap_OverloadStartIEs);
      break;

    case  S1ap_ProcedureCode_id_OverloadStop:
      ret = s1ap_compare_s1ap_overloadstopies(
                &m1->msg.s1ap_OverloadStopIEs,
                &m2->msg.s1ap_OverloadStopIEs);
      break;

    case  S1ap_ProcedureCode_id_WriteReplaceWarning:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_writereplacewarningrequesties(
                &m1->msg.s1ap_WriteReplaceWarningRequestIEs,
                &m2->msg.s1ap_WriteReplaceWarningRequestIEs);
      } else  {
        ret = s1ap_compare_s1ap_writereplacewarningresponseies(
                &m1->msg.s1ap_WriteReplaceWarningResponseIEs,
                &m2->msg.s1ap_WriteReplaceWarningResponseIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_eNBDirectInformationTransfer:
      ret = s1ap_compare_s1ap_enbdirectinformationtransferies(
                &m1->msg.s1ap_ENBDirectInformationTransferIEs,
                &m2->msg.s1ap_ENBDirectInformationTransferIEs);
      break;

    case  S1ap_ProcedureCode_id_MMEDirectInformationTransfer:

      ret = s1ap_compare_s1ap_mmedirectinformationtransferies(
                &m1->msg.s1ap_MMEDirectInformationTransferIEs,
                &m2->msg.s1ap_MMEDirectInformationTransferIEs);
      break;

    case  S1ap_ProcedureCode_id_PrivateMessage:
    case  S1ap_ProcedureCode_id_eNBConfigurationTransfer:
    case  S1ap_ProcedureCode_id_MMEConfigurationTransfer:

    case  S1ap_ProcedureCode_id_CellTrafficTrace:
      ret = s1ap_compare_s1ap_celltraffictraceies(
                &m1->msg.s1ap_CellTrafficTraceIEs,
                &m2->msg.s1ap_CellTrafficTraceIEs);
      break;

    case  S1ap_ProcedureCode_id_Kill:
      if (present == S1AP_PDU_PR_initiatingMessage) {
        ret = s1ap_compare_s1ap_killrequesties(
                &m1->msg.s1ap_KillRequestIEs,
                &m2->msg.s1ap_KillRequestIEs);
      } else  {
        ret = s1ap_compare_s1ap_killresponseies(
                &m1->msg.s1ap_KillResponseIEs,
                &m2->msg.s1ap_KillResponseIEs);
      }
      break;

    case  S1ap_ProcedureCode_id_downlinkUEAssociatedLPPaTransport:
      ret = s1ap_compare_s1ap_downlinkueassociatedlppatransport_ies(
                &m1->msg.s1ap_DownlinkUEAssociatedLPPaTransport_IEs,
                &m2->msg.s1ap_DownlinkUEAssociatedLPPaTransport_IEs);
      break;

    case  S1ap_ProcedureCode_id_uplinkUEAssociatedLPPaTransport:
      ret = s1ap_compare_s1ap_uplinkueassociatedlppatransport_ies(
                &m1->msg.s1ap_UplinkUEAssociatedLPPaTransport_IEs,
                &m2->msg.s1ap_UplinkUEAssociatedLPPaTransport_IEs);
      break;

    case  S1ap_ProcedureCode_id_downlinkNonUEAssociatedLPPaTransport:
      ret = s1ap_compare_s1ap_downlinknonueassociatedlppatransport_ies(
                &m1->msg.s1ap_DownlinkNonUEAssociatedLPPaTransport_IEs,
                &m2->msg.s1ap_DownlinkNonUEAssociatedLPPaTransport_IEs);
      break;

    case  S1ap_ProcedureCode_id_uplinkNonUEAssociatedLPPaTransport:
      ret = s1ap_compare_s1ap_uplinknonueassociatedlppatransport_ies(
                &m1->msg.s1ap_UplinkNonUEAssociatedLPPaTransport_IEs,
                &m2->msg.s1ap_UplinkNonUEAssociatedLPPaTransport_IEs);
      break;

    default:
      AssertFatal(0, "Unknown procedure code %ld", m1->procedureCode);
  }
  return ret;
}
