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

#if HAVE_CONFIG_H_
# include "config.h"
#endif

#include "X2ap-ABSInformationFDD.h"
#include "X2ap-ABSInformation.h"
#include "X2ap-ABSInformationTDD.h"
#include "X2ap-ABS-Status.h"
#include "X2ap-ActivatedCellList.h"
#include "X2ap-AllocationAndRetentionPriority.h"
#include "X2ap-AreaScopeOfMDT.h"
#include "X2ap-AS-SecurityInformation.h"
#include "X2ap-BandInfo.h"
#include "X2ap-BitRate.h"
#include "X2ap-BroadcastPLMNs-Item.h"

#include "X2ap-CapacityValue.h"
#include "X2ap-Cause.h"
#include "X2ap-CauseMisc.h"
#include "X2ap-CauseProtocol.h"
#include "X2ap-CauseRadioNetwork.h"
#include "X2ap-CauseTransport.h"
#include "X2ap-CellActivationFailure.h"
#include "X2ap-CellActivationRequest.h"
#include "X2ap-CellActivationResponse.h"
#include "X2ap-CellBasedMDT.h"
#include "X2ap-CellCapacityClassValue.h"
#include "X2ap-CellIdListforMDT.h"
#include "X2ap-CellInformation-Item.h"
#include "X2ap-CellInformation-List.h"
#include "X2ap-Cell-Size.h"
#include "X2ap-CellToReport-Item.h"
#include "X2ap-CellToReport-List.h"
#include "X2ap-CellType.h"
#include "X2ap-CellMeasurementResult-Item.h" 
#include "X2ap-CellMeasurementResult-List.h"
#include "X2ap-CompleteFailureCauseInformation-Item.h"
#include "X2ap-CompleteFailureCauseInformation-List.h"
#include "X2ap-CompositeAvailableCapacityGroup.h"
#include "X2ap-CompositeAvailableCapacity.h"
#include "X2ap-COUNTvalue.h"
#include "X2ap-CriticalityDiagnostics.h"
#include "X2ap-CriticalityDiagnostics-IE-List.h"
#include "X2ap-Criticality.h"
#include "X2ap-CRNTI.h"
#include "X2ap-CSG-Id.h"
#include "X2ap-CSGMembershipStatus.h"
#include "X2ap-CyclicPrefixDL.h"
#include "X2ap-CyclicPrefixUL.h"
#include "X2ap-DeactivationIndication.h"
#include "X2ap-DL-ABS-status.h"
#include "X2ap-DL-Forwarding.h"
#include "X2ap-DL-GBR-PRB-usage.h"
#include "X2ap-DL-non-GBR-PRB-usage.h"
#include "X2ap-DL-Total-PRB-usage.h"
#include "X2ap-EARFCN.h"
#include "X2ap-ECGI.h"
#include "X2ap-ENB-ID.h"
#include "X2ap-ENBConfigurationUpdateFailure.h"
#include "X2ap-ENBConfigurationUpdateAcknowledge.h"
#include "X2ap-ENBConfigurationUpdate.h"
#include "X2ap-EncryptionAlgorithms.h"
#include "X2ap-EPLMNs.h"
#include "X2ap-E-RAB-ID.h"
#include "X2ap-E-RAB-Item.h"
#include "X2ap-E-RABs-Admitted-List.h"
#include "X2ap-E-RAB-Level-QoS-Parameters.h"
#include "X2ap-E-RAB-List.h"
#include "X2ap-E-RABs-SubjectToStatusTransfer-Item.h"
#include "X2ap-E-RABs-Admitted-Item.h"
#include "X2ap-E-RABs-SubjectToStatusTransfer-List.h"
#include "X2ap-ErrorIndication.h"
#include "X2ap-EUTRA-Mode-Info.h"
#include "X2ap-EUTRANCellIdentifier.h"
#include "X2ap-EUTRANTraceID.h"
#include "X2ap-EventType.h"

#include "X2ap-FDD-Info.h"
#include "X2ap-ForbiddenInterRATs.h"
#include "X2ap-ForbiddenLACs.h"
#include "X2ap-ForbiddenLAs.h"
#include "X2ap-ForbiddenLAs-Item.h"
#include "X2ap-ForbiddenTACs.h"
#include "X2ap-ForbiddenTAs.h"
#include "X2ap-ForbiddenTAs-Item.h"
#include "X2ap-Fourframes.h"
#include "X2ap-FreqBandIndicator.h"
#include "X2ap-GBR-QosInformation.h"
#include "X2ap-GlobalENB-ID.h"
#include "X2ap-GTP-TEI.h"
#include "X2ap-GTPtunnelEndpoint.h"
#include "X2ap-GU-Group-ID.h"
#include "X2ap-GUGroupIDList.h"
#include "X2ap-GUMMEI.h"
#include "X2ap-HandoverCancel.h"
#include "X2ap-HandoverReportType.h"
#include "X2ap-HandoverRequest.h"
#include "X2ap-HandoverReport.h"
#include "X2ap-HandoverRestrictionList.h"
#include "X2ap-HandoverPreparationFailure.h"
#include "X2ap-HandoverRequestAcknowledge.h"

#include "X2ap-HFN.h"
#include "X2ap-HWLoadIndicator.h"
#include "X2ap-IE-Extensions.h"
#include "X2ap-IE.h"
#include "X2ap-IntegrityProtectionAlgorithms.h"
#include "X2ap-InterfacesToTrace.h"
#include "X2ap-InvokeIndication.h"
#include "X2ap-Key-eNodeB-Star.h"
#include "X2ap-LAC.h"
#include "X2ap-LastVisitedCell-Item.h"
#include "X2ap-LastVisitedEUTRANCellInformation.h"
#include "X2ap-LastVisitedGERANCellInformation.h"
#include "X2ap-LastVisitedUTRANCellInformation.h"
#include "X2ap-LoadIndicator.h"
#include "X2ap-LoadInformation.h"
#include "X2ap-LocationReportingInformation.h"
#include "X2ap-ManagementBasedMDTallowed.h"
#include "X2ap-MBMS-Service-Area-Identity.h"
#include "X2ap-MBMS-Service-Area-Identity-List.h"
#include "X2ap-MBSFN-Subframe-Info.h"
#include "X2ap-MBSFN-Subframe-Infolist.h"
#include "X2ap-MDT-Activation.h"
#include "X2ap-MDT-Configuration.h"
#include "X2ap-Measurement-ID.h"
#include "X2ap-MeasurementsToActivate.h"
#include "X2ap-MeasurementThresholdA2.h"
#include "X2ap-MeasurementInitiationResult-Item.h"
#include "X2ap-MeasurementInitiationResult-List.h"

#include "X2ap-MeasurementFailureCause-Item.h"
#include "X2ap-MME-Code.h"
#include "X2ap-MME-Group-ID.h"
#include "X2ap-MobilityChangeAcknowledge.h"
#include "X2ap-MobilityChangeFailure.h"
#include "X2ap-MobilityChangeRequest.h"
#include "X2ap-MobilityParametersInformation.h"
#include "X2ap-MobilityParametersModificationRange.h"
#include "X2ap-MultibandInfoList.h"
#include "X2ap-Neighbour-Information.h"
#include "X2ap-NextHopChainingCount.h"
#include "X2ap-Number-of-Antennaports.h"
#include "X2ap-Oneframe.h"
#include "X2ap-Old-ECGIs.h"
#include "X2ap-PartialSuccessIndicator.h"
#include "X2ap-PCI.h"
#include "X2ap-PDCP-SN.h"
#include "X2AP-PDU.h"
#include "X2ap-PeriodicReportingMDT.h"
#include "X2ap-PLMN-Identity.h"
#include "X2ap-PRACH-Configuration.h"
#include "X2ap-Pre-emptionCapability.h"
#include "X2ap-Pre-emptionVulnerability.h"
#include "X2ap-Presence.h"
#include "X2ap-PriorityLevel.h"
#include "X2ap-PrivateIE-ID.h"
#include "X2ap-ProcedureCode.h"
#include "X2ap-ProtocolIE-ID.h"
#include "X2ap-QCI.h"
#include "X2ap-RadioframeAllocationOffset.h"
#include "X2ap-RadioframeAllocationPeriod.h"
#include "X2ap-RadioResourceStatus.h"
#include "X2ap-ReceiveStatusofULPDCPSDUs.h"
#include "X2ap-Registration-Request.h"
#include "X2ap-RelativeNarrowbandTxPower.h"
#include "X2ap-ReportAmountMDT.h"
#include "X2ap-ReportArea.h"
#include "X2ap-ReportCharacteristics.h"
#include "X2ap-ReportingTriggerMDT.h"
#include "X2ap-ReportIntervalMDT.h"
#include "X2ap-ReportingPeriodicity.h"
#include "X2ap-ResourceStatusFailure.h"
#include "X2ap-ResourceStatusResponse.h"
#include "X2ap-ResourceStatusUpdate.h"

#include "X2ap-RNTP-Threshold.h"
#include "X2ap-RRCConnReestabIndicator.h"
#include "X2ap-RRCConnSetupIndicator.h"
#include "X2ap-RRC-Context.h"
#include "X2ap-RLFIndication.h"
#include "X2ap-ResourceStatusRequest.h"
#include "X2ap-S1TNLLoadIndicator.h"
//nclude "X2ap-ServedCellInformation.h"
#include "X2ap-ServedCells.h"
#include "X2ap-ServedCellsToModify.h"
#include "X2ap-ServedCellsToActivate.h"
#include "X2ap-ShortMAC-I.h"
#include "X2ap-SpecialSubframe-Info.h"
#include "X2ap-SpecialSubframePatterns.h"
#include "X2ap-SRVCCOperationPossible.h"
#include "X2ap-SubframeAllocation.h"
#include "X2ap-SubframeAssignment.h"
#include "X2ap-SubscriberProfileIDforRFP.h"
#include "X2ap-SNStatusTransfer.h"

#include "X2ap-TABasedMDT.h"
#include "X2ap-TAC.h"
#include "X2ap-TAListforMDT.h"
#include "X2ap-TargetCellInUTRAN.h"
#include "X2ap-TargeteNBtoSource-eNBTransparentContainer.h"
#include "X2ap-TDD-Info.h"
#include "X2ap-ThresholdEventA2.h"
#include "X2ap-Threshold-RSRP.h"
#include "X2ap-Threshold-RSRQ.h"
#include "X2ap-TimeToWait.h"
#include "X2ap-Time-UE-StayedInCell-EnhancedGranularity.h"
#include "X2ap-Time-UE-StayedInCell.h"
#include "X2ap-E-RABs-ToBeSetup-Item.h"
#include "X2ap-TraceActivation.h"
#include "X2ap-TraceCollectionEntityIPAddress.h"
#include "X2ap-TraceDepth.h"
#include "X2ap-Transmission-Bandwidth.h"
#include "X2ap-TransportLayerAddress.h"
#include "X2ap-TriggeringMessage.h"
#include "X2ap-TypeOfError.h"
#include "X2ap-UEAggregateMaximumBitRate.h"
#include "X2ap-UE-ContextInformation.h"
#include "X2ap-UE-HistoryInformation.h"
#include "X2ap-UE-RLF-Report-Container.h"
#include "X2ap-UE-S1AP-ID.h"
#include "X2ap-UE-X2AP-ID.h"
#include "X2ap-UEContextRelease.h"
#include "X2ap-UESecurityCapabilities.h"
#include "X2ap-UL-GBR-PRB-usage.h"
#include "X2ap-UL-HighInterferenceIndication.h"
#include "X2ap-UL-HighInterferenceIndicationInfo.h"
#include "X2ap-UL-HighInterferenceIndicationInfo-Item.h"
#include "X2ap-UL-InterferenceOverloadIndication.h"
#include "X2ap-UL-InterferenceOverloadIndication-Item.h"
#include "X2ap-UL-non-GBR-PRB-usage.h"
#include "X2ap-UL-Total-PRB-usage.h"
#include "X2ap-UsableABSInformationFDD.h"
#include "X2ap-UsableABSInformation.h"
#include "X2ap-UsableABSInformationTDD.h"

#include "X2ap-ResetResponse.h"
#include "X2ap-ResetRequest.h"
#include "X2SetupFailure.h"
#include "X2SetupRequest.h"
#include "X2SetupResponse.h"

#include "X2ap-Old-ECGIs.h"
#include "X2ap-Criticality.h"
#include "X2ap-ProcedureCode.h"

#ifndef X2AP_COMMON_H_
#define X2AP_COMMON_H_

/** @defgroup _x2ap_impl_ X2AP Layer Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

/* Checking version of ASN1C compiler */
#if (ASN1C_ENVIRONMENT_VERSION < 923)
# error "You are compiling x2ap with the wrong version of ASN1C"
#endif

#ifndef X2AP_PORT
# define X2AP_PORT 36422
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern int asn_debug;
extern int asn1_xer_print;

#if defined(ENB_MODE)
# include "log.h"
# define X2AP_ERROR(x, args...) LOG_E(X2AP, x, ##args)
# define X2AP_WARN(x, args...)  LOG_W(X2AP, x, ##args)
# define X2AP_DEBUG(x, args...) LOG_D(X2AP, x, ##args)
#else
# define X2AP_ERROR(x, args...) do { fprintf(stdout, "[X2AP][E]"x, ##args); } while(0)
# define X2AP_WARN(x, args...)  do { fprintf(stdout, "[X2AP][W]"x, ##args); } while(0)
# define X2AP_DEBUG(x, args...) do { fprintf(stdout, "[X2AP][D]"x, ##args); } while(0)
#endif

//Forward declaration
struct x2ap_message_s;

/** \brief Function callback prototype.
 **/
typedef int (*x2ap_message_decoded_callback)(
  uint32_t assocId,
  uint32_t stream,
  struct x2ap_message_s *message);

/** \brief Encode a successfull outcome message
 \param buffer pointer to buffer in which data will be encoded
 \param length pointer to the length of buffer
 \param procedureCode Procedure code for the message
 \param criticality Criticality of the message
 \param td ASN1C type descriptor of the sptr
 \param sptr Deferenced pointer to the structure to encode
 @returns size in bytes encded on success or 0 on failure
 **/
ssize_t x2ap_generate_successfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  X2ap_ProcedureCode_t         procedureCode,
  X2ap_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr);

/** \brief Encode an initiating message
 \param buffer pointer to buffer in which data will be encoded
 \param length pointer to the length of buffer
 \param procedureCode Procedure code for the message
 \param criticality Criticality of the message
 \param td ASN1C type descriptor of the sptr
 \param sptr Deferenced pointer to the structure to encode
 @returns size in bytes encded on success or 0 on failure
 **/
ssize_t x2ap_generate_initiating_message(
  uint8_t               **buffer,
  uint32_t               *length,
  X2ap_ProcedureCode_t    procedureCode,
  X2ap_Criticality_t      criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr);

/** \brief Encode an unsuccessfull outcome message
 \param buffer pointer to buffer in which data will be encoded
 \param length pointer to the length of buffer
 \param procedureCode Procedure code for the message
 \param criticality Criticality of the message
 \param td ASN1C type descriptor of the sptr
 \param sptr Deferenced pointer to the structure to encode
 @returns size in bytes encded on success or 0 on failure
 **/
ssize_t x2ap_generate_unsuccessfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  X2ap_ProcedureCode_t         procedureCode,
  X2ap_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr);

/** \brief Generate a new IE
 \param id Protocol ie id of the IE
 \param criticality Criticality of the IE
 \param type ASN1 type descriptor of the IE value
 \param sptr Structure to be encoded in the value field
 @returns a pointer to the newly created IE structure or NULL in case of failure
 **/
X2ap_IE_t *x2ap_new_ie(
  X2ap_ProtocolIE_ID_t   id,
  X2ap_Criticality_t     criticality,
  asn_TYPE_descriptor_t *type,
  void                  *sptr);

/** \brief Handle criticality
 \param criticality Criticality of the IE
 @returns void
 **/
void x2ap_handle_criticality(X2ap_Criticality_t criticality);

#endif /* X2AP_COMMON_H_ */
