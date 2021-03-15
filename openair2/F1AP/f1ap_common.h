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

/*! \file f1ap_common.h
 * \brief f1ap procedures for both CU and DU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#if HAVE_CONFIG_H_
# include "config.h"
#endif

#ifndef F1AP_COMMON_H_
#define F1AP_COMMON_H_

#include "openairinterface5g_limits.h"

#define F1AP_UE_IDENTIFIER_NUMBER 3
#define F1AP_TRANSACTION_IDENTIFIER_NUMBER 3

#if defined(EMIT_ASN_DEBUG_EXTERN)
inline void ASN_DEBUG(const char *fmt, ...);
#endif

#include "F1AP_RAT-FrequencyPriorityInformation.h"
#include "F1AP_DLUPTNLInformation-ToBeSetup-Item.h"
#include "F1AP_PrivateMessage.h"
#include "F1AP_Cause.h"
#include "F1AP_Pre-emptionVulnerability.h"
#include "F1AP_NRPCI.h"
#include "F1AP_Transmission-Bandwidth.h"
#include "F1AP_SIB1-message.h"
#include "F1AP_SibtypetobeupdatedListItem.h"
#include "F1AP_GNBCUConfigurationUpdateAcknowledge.h"
#include "F1AP_DRBs-Setup-Item.h"
#include "F1AP_EUTRA-NR-CellResourceCoordinationReqAck-Container.h"
#include "F1AP_NR-CGI-List-For-Restart-Item.h"
#include "F1AP_GNB-CU-Name.h"
#include "F1AP_PagingDRX.h"
#include "F1AP_RepetitionPeriod.h"
#include "F1AP_DRBs-ToBeModified-List.h"
#include "F1AP_ExecuteDuplication.h"
#include "F1AP_SCell-FailedtoSetupMod-List.h"
#include "F1AP_NRNRB.h"
#include "F1AP_SCell-ToBeSetup-List.h"
#include "F1AP_F1AP-PDU.h"
#include "F1AP_MaskedIMEISV.h"
#include "F1AP_ProtocolIE-Container.h"
#include "F1AP_GNB-CU-TNL-Association-To-Update-Item.h"
#include "F1AP_Cells-to-be-Activated-List-Item.h"
#include "F1AP_DRBs-Required-ToBeModified-Item.h"
#include "F1AP_BitRate.h"
#include "F1AP_SRBs-ToBeSetup-List.h"
#include "F1AP_CriticalityDiagnostics-IE-Item.h"
#include "F1AP_GNB-CU-TNL-Association-To-Update-List.h"
#include "F1AP_DRB-Notify-List.h"
#include "F1AP_UEContextReleaseCommand.h"
#include "F1AP_ProtocolIE-SingleContainer.h"
#include "F1AP_DRBs-ToBeReleased-List.h"
#include "F1AP_PWS-Failed-NR-CGI-List.h"
#include "F1AP_InitialULRRCMessageTransfer.h"
#include "F1AP_Served-Cell-Information.h"
#include "F1AP_ServedPLMNs-Item.h"
#include "F1AP_Served-EUTRA-Cells-Information.h"
#include "F1AP_Cells-Broadcast-Cancelled-Item.h"
#include "F1AP_F1SetupRequest.h"
#include "F1AP_Served-Cells-To-Add-Item.h"
#include "F1AP_F1SetupFailure.h"
#include "F1AP_ULUPTNLInformation-ToBeSetup-Item.h"
#include "F1AP_GNB-CU-TNL-Association-To-Add-Item.h"
#include "F1AP_DUtoCURRCContainer.h"
#include "F1AP_GNBDUResourceCoordinationRequest.h"
#include "F1AP_DRBs-FailedToBeSetup-List.h"
#include "F1AP_UPTransportLayerInformation.h"
#include "F1AP_RRCContainer.h"
#include "F1AP_Notification-Cause.h"
#include "F1AP_UEIdentityIndexValue.h"
#include "F1AP_SRBs-ToBeSetupMod-List.h"
#include "F1AP_GNB-DU-Served-Cells-Item.h"
#include "F1AP_RLCMode.h"
#include "F1AP_NRSCS.h"
#include "F1AP_SliceSupportList.h"
#include "F1AP_GTP-TEID.h"
#include "F1AP_UEContextModificationRequest.h"
#include "F1AP_MeasConfig.h"
#include "F1AP_Flows-Mapped-To-DRB-List.h"
#include "F1AP_Cells-to-be-Deactivated-List-Item.h"
#include "F1AP_QoSFlowLevelQoSParameters.h"
#include "F1AP_GNB-CU-UE-F1AP-ID.h"
#include "F1AP_CauseTransport.h"
#include "F1AP_DRBs-ToBeReleased-Item.h"
#include "F1AP_SCell-ToBeSetupMod-Item.h"
#include "F1AP_CellGroupConfig.h"
#include "F1AP_PWSSystemInformation.h"
#include "F1AP_DRBs-Modified-List.h"
#include "F1AP_HandoverPreparationInformation.h"
#include "F1AP_InactivityMonitoringResponse.h"
#include "F1AP_Served-Cells-To-Delete-List.h"
#include "F1AP_ProtocolExtensionField.h"
#include "F1AP_GNB-CU-TNL-Association-To-Remove-List.h"
#include "F1AP_SRBID.h"
#include "F1AP_DRB-Activity-List.h"
#include "F1AP_DRBs-FailedToBeModified-Item.h"
#include "F1AP_TransactionID.h"
#include "F1AP_AllocationAndRetentionPriority.h"
#include "F1AP_ShortDRXCycleLength.h"
#include "F1AP_DRB-Information.h"
#include "F1AP_TimeToWait.h"
#include "F1AP_NonDynamic5QIDescriptor.h"
#include "F1AP_C-RNTI.h"
#include "F1AP_MIB-message.h"
#include "F1AP_Served-Cells-To-Modify-List.h"
#include "F1AP_NRCGI.h"
#include "F1AP_DuplicationActivation.h"
#include "F1AP_CauseProtocol.h"
#include "F1AP_SCell-FailedtoSetup-Item.h"
#include "F1AP_PagingIdentity.h"
#include "F1AP_NGRANAllocationAndRetentionPriority.h"
#include "F1AP_TypeOfError.h"
#include "F1AP_GNB-CU-TNL-Association-To-Add-List.h"
#include "F1AP_DRBs-Required-ToBeReleased-Item.h"
#include "F1AP_EUTRA-Mode-Info.h"
#include "F1AP_FiveGS-TAC.h"
#include "F1AP_Cells-to-be-Activated-List.h"
#include "F1AP_PagingCell-list.h"
#include "F1AP_NotificationControl.h"
#include "F1AP_ProtectedEUTRAResourceIndication.h"
#include "F1AP_CUtoDURRCInformation.h"
#include "F1AP_SystemInformationDeliveryCommand.h"
#include "F1AP_AveragingWindow.h"
#include "F1AP_SRBs-ToBeSetupMod-Item.h"
#include "F1AP_NumberOfBroadcasts.h"
#include "F1AP_Cells-Broadcast-Completed-List.h"
#include "F1AP_GNB-CU-TNL-Association-Setup-Item.h"
#include "F1AP_PWSCancelResponse.h"
#include "F1AP_SpectrumSharingGroupID.h"
#include "F1AP_RANUEPagingIdentity.h"
#include "F1AP_CG-ConfigInfo.h"
#include "F1AP_PagingCell-Item.h"
#include "F1AP_GNB-CU-TNL-Association-To-Remove-Item.h"
#include "F1AP_UE-CapabilityRAT-ContainerList.h"
#include "F1AP_PWSCancelRequest.h"
#include "F1AP_PriorityLevel.h"
#include "F1AP_ProtocolIE-ContainerPair.h"
#include "F1AP_FullConfiguration.h"
#include "F1AP_NRCellIdentity.h"
#include "F1AP_ProtocolExtensionContainer.h"
#include "F1AP_PWSRestartIndication.h"
#include "F1AP_DRBs-ModifiedConf-List.h"
#include "F1AP_GNB-CU-TNL-Association-Failed-To-Setup-List.h"
#include "F1AP_UEContextSetupRequest.h"
#include "F1AP_PWSFailureIndication.h"
#include "F1AP_UE-associatedLogicalF1-ConnectionListResAck.h"
#include "F1AP_DRBs-ToBeSetupMod-Item.h"
#include "F1AP_SRBs-FailedToBeSetup-List.h"
#include "F1AP_Criticality.h"
#include "F1AP_UEContextModificationConfirm.h"
#include "F1AP_Broadcast-To-Be-Cancelled-List.h"
#include "F1AP_UEContextReleaseComplete.h"
#include "F1AP_PrivateIE-Container.h"
#include "F1AP_CellULConfigured.h"
#include "F1AP_DRB-Activity.h"
#include "F1AP_GNB-CU-TNL-Association-Failed-To-Setup-Item.h"
#include "F1AP_ProtocolIE-ID.h"
#include "F1AP_PrivateIE-ID.h"
#include "F1AP_WriteReplaceWarningResponse.h"
#include "F1AP_CauseMisc.h"
#include "F1AP_SRBs-Required-ToBeReleased-Item.h"
#include "F1AP_Cells-Broadcast-Cancelled-List.h"
#include "F1AP_ULUEConfiguration.h"
#include "F1AP_RAT-FrequencySelectionPriority.h"
#include "F1AP_UEInactivityNotification.h"
#include "F1AP_DLRRCMessageTransfer.h"
#include "F1AP_TriggeringMessage.h"
#include "F1AP_DRBs-ToBeSetup-List.h"
#include "F1AP_Cells-to-be-Barred-Item.h"
#include "F1AP_UE-associatedLogicalF1-ConnectionItem.h"
#include "F1AP_Cancel-all-Warning-Messages-Indicator.h"
#include "F1AP_SCell-FailedtoSetupMod-Item.h"
#include "F1AP_DRBs-FailedToBeSetupMod-List.h"
#include "F1AP_ProtocolIE-ID.h"
#include "F1AP_TransportLayerAddress.h"
#include "F1AP_GNB-DU-System-Information.h"
#include "F1AP_PWS-Failed-NR-CGI-Item.h"
#include "F1AP_Notify.h"
#include "F1AP_UEContextModificationResponse.h"
#include "F1AP_DRBID.h"
#include "F1AP_GNBDUResourceCoordinationResponse.h"
#include "F1AP_UEContextModificationRequired.h"
#include "F1AP_InitiatingMessage.h"
#include "F1AP_SliceSupportItem.h"
#include "F1AP_ProtocolIE-FieldPair.h"
#include "F1AP_EUTRA-TDD-Info.h"
#include "F1AP_GNBDUConfigurationUpdateFailure.h"
#include "F1AP_ULUPTNLInformation-ToBeSetup-List.h"
#include "F1AP_WriteReplaceWarningRequest.h"
#include "F1AP_ServCellIndex.h"
#include "F1AP_ResetAcknowledge.h"
#include "F1AP_SRBs-FailedToBeSetupMod-Item.h"
#include "F1AP_OffsetToPointA.h"
#include "F1AP_ProcedureCode.h"
#include "F1AP_GTPTunnel.h"
#include "F1AP_TDD-Info.h"
#include "F1AP_Pre-emptionCapability.h"
#include "F1AP_MaxDataBurstVolume.h"
#include "F1AP_SUL-Information.h"
#include "F1AP_CriticalityDiagnostics-IE-List.h"
#include "F1AP_EUTRA-FDD-Info.h"
#include "F1AP_Served-Cells-To-Delete-Item.h"
#include "F1AP_Candidate-SpCell-Item.h"
#include "F1AP_Cells-To-Be-Broadcast-List.h"
#include "F1AP_ULRRCMessageTransfer.h"
#include "F1AP_Cells-to-be-Deactivated-List.h"
#include "F1AP_DRBs-Required-ToBeReleased-List.h"
#include "F1AP_Served-Cells-To-Add-List.h"
#include "F1AP_Potential-SpCell-List.h"
#include "F1AP_EUTRANQoS.h"
#include "F1AP_Dynamic5QIDescriptor.h"
#include "F1AP_GNBCUConfigurationUpdateFailure.h"
#include "F1AP_DuplicationIndication.h"
#include "F1AP_GNB-DU-Served-Cells-List.h"
#include "F1AP_QoS-Characteristics.h"
#include "F1AP_UE-associatedLogicalF1-ConnectionListRes.h"
#include "F1AP_ResourceCoordinationTransferContainer.h"
#include "F1AP_DRXCycle.h"
#include "F1AP_DRBs-FailedToBeSetup-Item.h"
#include "F1AP_PrivateIE-Field.h"
#include "F1AP_SRBs-ToBeReleased-List.h"
#include "F1AP_MeasGapConfig.h"
#include "F1AP_NR-Mode-Info.h"
#include "F1AP_Protected-EUTRA-Resources-List.h"
#include "F1AP_SRBs-FailedToBeSetup-Item.h"
#include "F1AP_ResetAll.h"
#include "F1AP_SCell-FailedtoSetup-List.h"
#include "F1AP_UEContextModificationFailure.h"
#include "F1AP_CNUEPagingIdentity.h"
#include "F1AP_DRBs-ToBeSetupMod-List.h"
#include "F1AP_GNBDUConfigurationUpdate.h"
#include "F1AP_DRBs-ToBeSetup-Item.h"
#include "F1AP_UnsuccessfulOutcome.h"
#include "F1AP_SRBs-FailedToBeSetupMod-List.h"
#include "F1AP_SCell-ToBeRemoved-Item.h"
#include "F1AP_InactivityMonitoringRequest.h"
#include "F1AP_Cells-Failed-to-be-Activated-List-Item.h"
#include "F1AP_DRBs-Modified-Item.h"
#include "F1AP_SRBs-Required-ToBeReleased-List.h"
#include "F1AP_GBR-QosInformation.h"
#include "F1AP_SCell-ToBeRemoved-List.h"
#include "F1AP_RANAC.h"
#include "F1AP_GNB-DU-UE-F1AP-ID.h"
#include "F1AP_CauseRadioNetwork.h"
#include "F1AP_DRB-Notify-Item.h"
#include "F1AP_GNBDUConfigurationUpdateAcknowledge.h"
#include "F1AP_GNB-CUSystemInformation.h"
#include "F1AP_ProtocolIE-Field.h"
#include "F1AP_Served-Cells-To-Modify-Item.h"
#include "F1AP_Flows-Mapped-To-DRB-Item.h"
#include "F1AP_SupportedSULFreqBandItem.h"
#include "F1AP_UEContextReleaseRequest.h"
#include "F1AP_GNB-DU-Name.h"
#include "F1AP_DRBs-ToBeModified-Item.h"
#include "F1AP_EUTRA-NR-CellResourceCoordinationReq-Container.h"
#include "F1AP_DRBs-SetupMod-List.h"
#include "F1AP_DRBs-Required-ToBeModified-List.h"
#include "F1AP_DUtoCURRCInformation.h"
#include "F1AP_MaxPacketLossRate.h"
#include "F1AP_PacketDelayBudget.h"
#include "F1AP_GNBCUConfigurationUpdate.h"
#include "F1AP_Cells-Broadcast-Completed-Item.h"
#include "F1AP_PagingPriority.h"
#include "F1AP_Cells-Failed-to-be-Activated-List.h"
#include "F1AP_Endpoint-IP-address-and-port.h"
#include "F1AP_PacketErrorRate.h"
#include "F1AP_PLMN-Identity.h"
#include "F1AP_asn_constant.h"
#include "F1AP_ResetType.h"
#include "F1AP_FDD-Info.h"
#include "F1AP_DLUPTNLInformation-ToBeSetup-List.h"
#include "F1AP_NR-CGI-List-For-Restart-List.h"
#include "F1AP_F1SetupResponse.h"
#include "F1AP_UEContextSetupResponse.h"
#include "F1AP_CP-TransportLayerAddress.h"
#include "F1AP_Broadcast-To-Be-Cancelled-Item.h"
#include "F1AP_ErrorIndication.h"
#include "F1AP_SubscriberProfileIDforRFP.h"
#include "F1AP_SNSSAI.h"
#include "F1AP_DRBs-ModifiedConf-Item.h"
#include "F1AP_GNB-CU-TNL-Association-Setup-List.h"
#include "F1AP_DRB-Activity-Item.h"
#include "F1AP_LCID.h"
#include "F1AP_ULConfiguration.h"
#include "F1AP_ShortDRXCycleTimer.h"
#include "F1AP_FreqBandNrItem.h"
#include "F1AP_Cells-to-be-Barred-List.h"
#include "F1AP_Presence.h"
#include "F1AP_CellBarred.h"
#include "F1AP_RequestType.h"
#include "F1AP_NRFreqInfo.h"
#include "F1AP_Potential-SpCell-Item.h"
#include "F1AP_NumberofBroadcastRequest.h"
#include "F1AP_TNLAssociationUsage.h"
#include "F1AP_SCell-ToBeSetupMod-List.h"
#include "F1AP_DRBs-Setup-List.h"
#include "F1AP_Reset.h"
#include "F1AP_CriticalityDiagnostics.h"
#include "F1AP_Paging.h"
#include "F1AP_LongDRXCycleLength.h"
#include "F1AP_GNB-DU-ID.h"
#include "F1AP_SuccessfulOutcome.h"
#include "F1AP_Configured-EPS-TAC.h"
#include "F1AP_Candidate-SpCell-List.h"
#include "F1AP_SRBs-ToBeReleased-Item.h"
#include "F1AP_QoSInformation.h"
#include "F1AP_SCell-ToBeSetup-Item.h"
#include "F1AP_SRBs-ToBeSetup-Item.h"
#include "F1AP_GBR-QoSFlowInformation.h"
#include "F1AP_SCellIndex.h"
#include "F1AP_DRBs-SetupMod-Item.h"
#include "F1AP_UEContextSetupFailure.h"
#include "F1AP_DRBs-FailedToBeModified-List.h"
#include "F1AP_DRBs-FailedToBeSetupMod-Item.h"
#include "F1AP_ProtocolExtensionID.h"
#include "F1AP_Cells-To-Be-Broadcast-Item.h"
#include "F1AP_QCI.h"

#include "conversions.h"
#include "platform_types.h"
#include "common/utils/LOG/log.h"
#include "intertask_interface.h"
#include "sctp_messages_types.h"
#include "f1ap_messages_types.h"
#include <arpa/inet.h>
#include "T.h"
#include "common/ran_context.h"
#include "msc.h"

/* Checking version of ASN1C compiler */
#if (ASN1C_ENVIRONMENT_VERSION < ASN1C_MINIMUM_VERSION)
# error "You are compiling f1ap with the wrong version of ASN1C"
#endif

#ifndef FALSE
# define FALSE (0)
#endif
#ifndef TRUE
# define TRUE  (!FALSE)
#endif

#define F1AP_UE_ID_FMT  "0x%06"PRIX32

#include "assertions.h"

#if defined(ENB_MODE)
# include "common/utils/LOG/log.h"
# include "f1ap_default_values.h"
# define F1AP_ERROR(x, args...) LOG_E(F1AP, x, ##args)
# define F1AP_WARN(x, args...)  LOG_W(F1AP, x, ##args)
# define F1AP_TRAF(x, args...)  LOG_I(F1AP, x, ##args)
# define F1AP_INFO(x, args...) LOG_I(F1AP, x, ##args)
# define F1AP_DEBUG(x, args...) LOG_I(F1AP, x, ##args)
#else
//# include "mme_default_values.h"
# define F1AP_ERROR(x, args...) do { fprintf(stdout, "[F1AP][E]"x, ##args); } while(0)
# define F1AP_WARN(x, args...)  do { fprintf(stdout, "[F1AP][W]"x, ##args); } while(0)
# define F1AP_TRAF(x, args...)  do { fprintf(stdout, "[F1AP][T]"x, ##args); } while(0)
# define F1AP_INFO(x, args...) do { fprintf(stdout, "[F1AP][I]"x, ##args); } while(0)
# define F1AP_DEBUG(x, args...) do { fprintf(stdout, "[F1AP][D]"x, ##args); } while(0)
#endif

//Forward declaration
#define F1AP_FIND_PROTOCOLIE_BY_ID(IE_TYPE, ie, container, IE_ID, mandatory) \
  do {\
    IE_TYPE **ptr; \
    ie = NULL; \
    for (ptr = container->protocolIEs.list.array; \
         ptr < &container->protocolIEs.list.array[container->protocolIEs.list.count]; \
         ptr++) { \
      if((*ptr)->id == IE_ID) { \
        ie = *ptr; \
        break; \
      } \
    } \
    if (mandatory) DevAssert(ie != NULL); \
  } while(0)

/** \brief Function callback prototype.
 **/
typedef int (*f1ap_message_decoded_callback)(
  instance_t             instance,
  uint32_t               assoc_id,
  uint32_t               stream,
  F1AP_F1AP_PDU_t       *message_p
);

typedef struct f1ap_cudu_ue_inst_s {
  // used for eNB stats generation
  rnti_t      rnti;
  module_id_t f1ap_uid;
  module_id_t mac_uid;
  module_id_t du_ue_f1ap_id;
  module_id_t cu_ue_f1ap_id;
} f1ap_cudu_ue_t;

typedef struct f1ap_cudu_inst_s {
  uint16_t num_ues;
  f1ap_cudu_ue_t f1ap_ue[MAX_MOBILES_PER_ENB];
} f1ap_cudu_inst_t;



uint8_t F1AP_get_next_transaction_identifier(module_id_t enb_mod_idP, module_id_t cu_mod_idP);


int f1ap_add_ue(f1ap_cudu_inst_t *f1_inst,
                module_id_t     module_idP,
                int             CC_idP,
                int             UE_id,
                rnti_t          rntiP);

int f1ap_remove_ue(f1ap_cudu_inst_t *f1_inst,
                   rnti_t            rntiP);

int f1ap_get_du_ue_f1ap_id (f1ap_cudu_inst_t *f1_inst,
                            rnti_t            rntiP);

int f1ap_get_cu_ue_f1ap_id (f1ap_cudu_inst_t *f1_inst,
                            rnti_t            rntiP);


int f1ap_get_rnti_by_du_id(f1ap_cudu_inst_t *f1_inst,
                           module_id_t       du_ue_f1ap_id );


int f1ap_get_rnti_by_cu_id(f1ap_cudu_inst_t *f1_inst,
                           module_id_t       cu_ue_f1ap_id );


int f1ap_get_du_uid(f1ap_cudu_inst_t *f1_inst,
                    module_id_t       du_ue_f1ap_id );

int f1ap_get_cu_uid(f1ap_cudu_inst_t *f1_inst,
                    module_id_t       cu_ue_f1ap_id );

int f1ap_get_uid_by_rnti(f1ap_cudu_inst_t *f1_inst,
                         rnti_t            rntiP );

int f1ap_du_add_cu_ue_id(f1ap_cudu_inst_t *f1_inst,
                         module_id_t       du_ue_f1ap_id,
                         module_id_t       cu_ue_f1ap_id);

int f1ap_cu_add_du_ue_id(f1ap_cudu_inst_t *f1_inst,
                         module_id_t       cu_ue_f1ap_id,
                         module_id_t       du_ue_f1ap_id);

#endif /* F1AP_COMMON_H_ */
