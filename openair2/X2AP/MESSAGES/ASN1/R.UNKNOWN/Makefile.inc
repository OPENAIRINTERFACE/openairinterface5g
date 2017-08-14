ASN_MODULE_SOURCES=    \
    Criticality.c  \
    Presence.c \
    PrivateIE-ID.c \
    ProcedureCode.c    \
    ProtocolIE-ID.c    \
    TriggeringMessage.c    \
    IE-Extensions.c    \
    AS-SecurityInformation.c   \
    AllocationAndRetentionPriority.c   \
    ActivatedCellList.c    \
    ActivatedCellList-Item.c   \
    BitRate.c  \
    BroadcastPLMNs-Item.c  \
    CapacityValue.c    \
    CellCapacityClassValue.c   \
    CellToReport-List.c    \
    CellToReport-Item.c    \
    CellMeasurementResult-List.c   \
    CellMeasurementResult-Item.c   \
    ReportingPeriodicity.c \
    Cause.c    \
    CauseMisc.c    \
    CauseProtocol.c    \
    CauseRadioNetwork.c    \
    CauseTransport.c   \
    Cell-Size.c    \
    CellInformation-List.c \
    CellInformation-Item.c \
    CellType.c \
    CompositeAvailableCapacityGroup.c  \
    CompositeAvailableCapacity.c   \
    COUNTvalue.c   \
    CriticalityDiagnostics.c   \
    CriticalityDiagnostics-IE-List.c   \
    CriticalityDiagnostics-IE-Elm.c    \
    CRNTI.c    \
    CyclicPrefixDL.c   \
    CyclicPrefixUL.c   \
    DeactivationIndication.c   \
    DL-Forwarding.c    \
    E-RABs-Admitted-List.c \
    E-RABs-Admitted-Item.c \
    E-RABs-ToBeSetup-List.c    \
    E-RABs-ToBeSetup-Item.c    \
    E-RABs-SubjectToStatusTransfer-List.c  \
    E-RABs-SubjectToStatusTransfer-Item.c  \
    EARFCN.c   \
    FDD-Info.c \
    TDD-Info.c \
    EUTRA-Mode-Info.c  \
    ECGI.c \
    ENB-ID.c   \
    EncryptionAlgorithms.c \
    EPLMNs.c   \
    E-RAB-ID.c \
    E-RAB-Level-QoS-Parameters.c   \
    E-RAB-List.c   \
    E-RAB-Item.c   \
    EUTRANCellIdentifier.c \
    EUTRANTraceID.c    \
    EventType.c    \
    ForbiddenInterRATs.c   \
    ForbiddenTAs.c \
    ForbiddenTAs-Item.c    \
    ForbiddenTACs.c    \
    ForbiddenLAs.c \
    ForbiddenLAs-Item.c    \
    ForbiddenLACs.c    \
    Fourframes.c   \
    GBR-QosInformation.c   \
    GlobalENB-ID.c \
    GTPtunnelEndpoint.c    \
    GTP-TEI.c  \
    GUGroupIDList.c    \
    GU-Group-ID.c  \
    GUMMEI.c   \
    HandoverReportType.c   \
    HandoverRestrictionList.c  \
    HFN.c  \
    HWLoadIndicator.c  \
    IntegrityProtectionAlgorithms.c    \
    InterfacesToTrace.c    \
    Key-eNodeB-Star.c  \
    LAC.c  \
    LastVisitedCell-Item.c \
    LastVisitedEUTRANCellInformation.c \
    LastVisitedUTRANCellInformation.c  \
    LastVisitedGERANCellInformation.c  \
    LoadIndicator.c    \
    LocationReportingInformation.c \
    MME-Group-ID.c \
    MME-Code.c \
    Measurement-ID.c   \
    MBSFN-Subframe-Infolist.c  \
    MBSFN-Subframe-Info.c  \
    MobilityParametersModificationRange.c  \
    MobilityParametersInformation.c    \
    Neighbour-Information.c    \
    Neighbour-Information-Elm.c    \
    NextHopChainingCount.c \
    Number-of-Antennaports.c   \
    Oneframe.c \
    PDCP-SN.c  \
    PCI.c  \
    PLMN-Identity.c    \
    PRACH-Configuration.c  \
    Pre-emptionCapability.c    \
    Pre-emptionVulnerability.c \
    PriorityLevel.c    \
    QCI.c  \
    ReceiveStatusofULPDCPSDUs.c    \
    Registration-Request.c \
    RelativeNarrowbandTxPower.c    \
    ReportArea.c   \
    ReportCharacteristics.c    \
    RNTP-Threshold.c   \
    RRC-Context.c  \
    RadioResourceStatus.c  \
    DL-GBR-PRB-usage.c \
    UL-GBR-PRB-usage.c \
    DL-non-GBR-PRB-usage.c \
    UL-non-GBR-PRB-usage.c \
    DL-Total-PRB-usage.c   \
    UL-Total-PRB-usage.c   \
    RadioframeAllocationPeriod.c   \
    RadioframeAllocationOffset.c   \
    S1TNLLoadIndicator.c   \
    ServedCellsToActivate.c    \
    ServedCellsToActivate-Item.c   \
    ServedCellsToModify.c  \
    ServedCellsToModify-Item.c \
    Old-ECGIs.c    \
    ServedCells.c  \
    ServedCellItem.c   \
    ServedCell-Information.c   \
    ShortMAC-I.c   \
    SRVCCOperationPossible.c   \
    SubframeAssignment.c   \
    SpecialSubframe-Info.c \
    SpecialSubframePatterns.c  \
    SubscriberProfileIDforRFP.c    \
    SubframeAllocation.c   \
    TAC.c  \
    TargeteNBtoSource-eNBTransparentContainer.c    \
    TimeToWait.c   \
    Time-UE-StayedInCell.c \
    TraceActivation.c  \
    TraceCollectionEntityIPAddress.c   \
    TraceDepth.c   \
    Transmission-Bandwidth.c   \
    TransportLayerAddress.c    \
    TypeOfError.c  \
    UE-HistoryInformation.c    \
    UE-S1AP-ID.c   \
    UE-ID.c   \
    UEAggregateMaximumBitRate.c    \
    UESecurityCapabilities.c   \
    UE-ContextInformation.c    \
    UL-InterferenceOverloadIndication.c    \
    UL-InterferenceOverloadIndication-Item.c   \
    UL-HighInterferenceIndicationInfo.c    \
    UL-HighInterferenceIndicationInfo-Item.c   \
    UL-HighInterferenceIndication.c    \
    UE-RLF-Report-Container.c  \
    IE.c   \
    PDU.c  \
    InitiatingMessage.c    \
    SuccessfulOutcome.c    \
    UnsuccessfulOutcome.c  \
    HandoverRequest.c  \
    HandoverRequestAcknowledge.c   \
    HandoverPreparationFailure.c   \
    SNStatusTransfer.c \
    UEContextRelease.c \
    HandoverCancel.c   \
    HandoverReport.c   \
    ErrorIndication.c  \
    ResetRequest.c \
    ResetResponse.c    \
    X2SetupRequest.c   \
    X2SetupResponse.c  \
    X2SetupFailure.c   \
    LoadInformation.c  \
    ENBConfigurationUpdate.c   \
    ENBConfigurationUpdateAcknowledge.c    \
    ENBConfigurationUpdateFailure.c    \
    ResourceStatusRequest.c    \
    ResourceStatusResponse.c   \
    ResourceStatusFailure.c    \
    ResourceStatusUpdate.c \
    RLFIndication.c    \
    PrivateMessage.c   \
    MobilityChangeRequest.c    \
    MobilityChangeAcknowledge.c    \
    MobilityChangeFailure.c    \
    CellActivationRequest.c    \
    CellActivationResponse.c   \
    CellActivationFailure.c

ASN_MODULE_HEADERS=    \
    Criticality.h  \
    Presence.h \
    PrivateIE-ID.h \
    ProcedureCode.h    \
    ProtocolIE-ID.h    \
    TriggeringMessage.h    \
    IE-Extensions.h    \
    AS-SecurityInformation.h   \
    AllocationAndRetentionPriority.h   \
    ActivatedCellList.h    \
    ActivatedCellList-Item.h   \
    BitRate.h  \
    BroadcastPLMNs-Item.h  \
    CapacityValue.h    \
    CellCapacityClassValue.h   \
    CellToReport-List.h    \
    CellToReport-Item.h    \
    CellMeasurementResult-List.h   \
    CellMeasurementResult-Item.h   \
    ReportingPeriodicity.h \
    Cause.h    \
    CauseMisc.h    \
    CauseProtocol.h    \
    CauseRadioNetwork.h    \
    CauseTransport.h   \
    Cell-Size.h    \
    CellInformation-List.h \
    CellInformation-Item.h \
    CellType.h \
    CompositeAvailableCapacityGroup.h  \
    CompositeAvailableCapacity.h   \
    COUNTvalue.h   \
    CriticalityDiagnostics.h   \
    CriticalityDiagnostics-IE-List.h   \
    CriticalityDiagnostics-IE-Elm.h    \
    CRNTI.h    \
    CyclicPrefixDL.h   \
    CyclicPrefixUL.h   \
    DeactivationIndication.h   \
    DL-Forwarding.h    \
    E-RABs-Admitted-List.h \
    E-RABs-Admitted-Item.h \
    E-RABs-ToBeSetup-List.h    \
    E-RABs-ToBeSetup-Item.h    \
    E-RABs-SubjectToStatusTransfer-List.h  \
    E-RABs-SubjectToStatusTransfer-Item.h  \
    EARFCN.h   \
    FDD-Info.h \
    TDD-Info.h \
    EUTRA-Mode-Info.h  \
    ECGI.h \
    ENB-ID.h   \
    EncryptionAlgorithms.h \
    EPLMNs.h   \
    E-RAB-ID.h \
    E-RAB-Level-QoS-Parameters.h   \
    E-RAB-List.h   \
    E-RAB-Item.h   \
    EUTRANCellIdentifier.h \
    EUTRANTraceID.h    \
    EventType.h    \
    ForbiddenInterRATs.h   \
    ForbiddenTAs.h \
    ForbiddenTAs-Item.h    \
    ForbiddenTACs.h    \
    ForbiddenLAs.h \
    ForbiddenLAs-Item.h    \
    ForbiddenLACs.h    \
    Fourframes.h   \
    GBR-QosInformation.h   \
    GlobalENB-ID.h \
    GTPtunnelEndpoint.h    \
    GTP-TEI.h  \
    GUGroupIDList.h    \
    GU-Group-ID.h  \
    GUMMEI.h   \
    HandoverReportType.h   \
    HandoverRestrictionList.h  \
    HFN.h  \
    HWLoadIndicator.h  \
    IntegrityProtectionAlgorithms.h    \
    InterfacesToTrace.h    \
    Key-eNodeB-Star.h  \
    LAC.h  \
    LastVisitedCell-Item.h \
    LastVisitedEUTRANCellInformation.h \
    LastVisitedUTRANCellInformation.h  \
    LastVisitedGERANCellInformation.h  \
    LoadIndicator.h    \
    LocationReportingInformation.h \
    MME-Group-ID.h \
    MME-Code.h \
    Measurement-ID.h   \
    MBSFN-Subframe-Infolist.h  \
    MBSFN-Subframe-Info.h  \
    MobilityParametersModificationRange.h  \
    MobilityParametersInformation.h    \
    Neighbour-Information.h    \
    Neighbour-Information-Elm.h    \
    NextHopChainingCount.h \
    Number-of-Antennaports.h   \
    Oneframe.h \
    PDCP-SN.h  \
    PCI.h  \
    PLMN-Identity.h    \
    PRACH-Configuration.h  \
    Pre-emptionCapability.h    \
    Pre-emptionVulnerability.h \
    PriorityLevel.h    \
    QCI.h  \
    ReceiveStatusofULPDCPSDUs.h    \
    Registration-Request.h \
    RelativeNarrowbandTxPower.h    \
    ReportArea.h   \
    ReportCharacteristics.h    \
    RNTP-Threshold.h   \
    RRC-Context.h  \
    RadioResourceStatus.h  \
    DL-GBR-PRB-usage.h \
    UL-GBR-PRB-usage.h \
    DL-non-GBR-PRB-usage.h \
    UL-non-GBR-PRB-usage.h \
    DL-Total-PRB-usage.h   \
    vUL-Total-PRB-usage.h   \
    RadioframeAllocationPeriod.h   \
    RadioframeAllocationOffset.h   \
    S1TNLLoadIndicator.h   \
    ServedCellsToActivate.h    \
    ServedCellsToActivate-Item.h   \
    ServedCellsToModify.h  \
    ServedCellsToModify-Item.h \
    Old-ECGIs.h    \
    ServedCells.h  \
    ServedCellItem.h   \
    ServedCell-Information.h   \
    ShortMAC-I.h   \
    SRVCCOperationPossible.h   \
    SubframeAssignment.h   \
    SpecialSubframe-Info.h \
    SpecialSubframePatterns.h  \
    SubscriberProfileIDforRFP.h    \
    SubframeAllocation.h   \
    TAC.h  \
    TargeteNBtoSource-eNBTransparentContainer.h    \
    TimeToWait.h   \
    Time-UE-StayedInCell.h \
    TraceActivation.h  \
    TraceCollectionEntityIPAddress.h   \
    TraceDepth.h   \
    Transmission-Bandwidth.h   \
    TransportLayerAddress.h    \
    TypeOfError.h  \
    UE-HistoryInformation.h    \
    UE-S1AP-ID.h   \
    UE-ID.h   \
    UEAggregateMaximumBitRate.h    \
    UESecurityCapabilities.h   \
    UE-ContextInformation.h    \
    UL-InterferenceOverloadIndication.h    \
    UL-InterferenceOverloadIndication-Item.h   \
    UL-HighInterferenceIndicationInfo.h    \
    UL-HighInterferenceIndicationInfo-Item.h   \
    UL-HighInterferenceIndication.h    \
    UE-RLF-Report-Container.h  \
    IE.h   \
    PDU.h \
    InitiatingMessage.h    \
    SuccessfulOutcome.h    \
    UnsuccessfulOutcome.h  \
    HandoverRequest.h  \
    HandoverRequestAcknowledge.h   \
    HandoverPreparationFailure.h   \
    SNStatusTransfer.h \
    UEContextRelease.h \
    HandoverCancel.h   \
    HandoverReport.h   \
    ErrorIndication.h  \
    ResetRequest.h \
    ResetResponse.h    \
    X2SetupRequest.h   \
    X2SetupResponse.h  \
    X2SetupFailure.h   \
    LoadInformation.h  \
    ENBConfigurationUpdate.h   \
    ENBConfigurationUpdateAcknowledge.h    \
    ENBConfigurationUpdateFailure.h    \
    ResourceStatusRequest.h    \
    ResourceStatusResponse.h   \
    ResourceStatusFailure.h    \
    ResourceStatusUpdate.h \
    RLFIndication.h    \
    PrivateMessage.h   \
    MobilityChangeRequest.h    \
    MobilityChangeAcknowledge.h    \
    MobilityChangeFailure.h    \
    CellActivationRequest.h    \
    CellActivationResponse.h   \
    CellActivationFailure.h

ASN_MODULE_HEADERS+=ANY.h
ASN_MODULE_SOURCES+=ANY.c
ASN_MODULE_HEADERS+=BOOLEAN.h
ASN_MODULE_SOURCES+=BOOLEAN.c
ASN_MODULE_HEADERS+=INTEGER.h
ASN_MODULE_HEADERS+=NativeEnumerated.h
ASN_MODULE_SOURCES+=INTEGER.c
ASN_MODULE_HEADERS+=NULL.h
ASN_MODULE_SOURCES+=NULL.c
ASN_MODULE_SOURCES+=NativeEnumerated.c
ASN_MODULE_HEADERS+=NativeInteger.h
ASN_MODULE_SOURCES+=NativeInteger.c
ASN_MODULE_HEADERS+=OBJECT_IDENTIFIER.h
ASN_MODULE_SOURCES+=OBJECT_IDENTIFIER.c
ASN_MODULE_HEADERS+=asn_SEQUENCE_OF.h
ASN_MODULE_SOURCES+=asn_SEQUENCE_OF.c
ASN_MODULE_HEADERS+=asn_SET_OF.h
ASN_MODULE_SOURCES+=asn_SET_OF.c
ASN_MODULE_HEADERS+=constr_CHOICE.h
ASN_MODULE_SOURCES+=constr_CHOICE.c
ASN_MODULE_HEADERS+=constr_SEQUENCE.h
ASN_MODULE_SOURCES+=constr_SEQUENCE.c
ASN_MODULE_HEADERS+=constr_SEQUENCE_OF.h
ASN_MODULE_SOURCES+=constr_SEQUENCE_OF.c
ASN_MODULE_HEADERS+=constr_SET_OF.h
ASN_MODULE_SOURCES+=constr_SET_OF.c
ASN_MODULE_HEADERS+=asn_application.h
ASN_MODULE_HEADERS+=asn_system.h
ASN_MODULE_HEADERS+=asn_codecs.h
ASN_MODULE_HEADERS+=asn_internal.h
ASN_MODULE_HEADERS+=OCTET_STRING.h
ASN_MODULE_SOURCES+=OCTET_STRING.c
ASN_MODULE_HEADERS+=BIT_STRING.h
ASN_MODULE_SOURCES+=BIT_STRING.c
ASN_MODULE_SOURCES+=asn_codecs_prim.c
ASN_MODULE_HEADERS+=asn_codecs_prim.h
ASN_MODULE_HEADERS+=ber_tlv_length.h
ASN_MODULE_SOURCES+=ber_tlv_length.c
ASN_MODULE_HEADERS+=ber_tlv_tag.h
ASN_MODULE_SOURCES+=ber_tlv_tag.c
ASN_MODULE_HEADERS+=ber_decoder.h
ASN_MODULE_SOURCES+=ber_decoder.c
ASN_MODULE_HEADERS+=der_encoder.h
ASN_MODULE_SOURCES+=der_encoder.c
ASN_MODULE_HEADERS+=constr_TYPE.h
ASN_MODULE_SOURCES+=constr_TYPE.c
ASN_MODULE_HEADERS+=constraints.h
ASN_MODULE_SOURCES+=constraints.c
ASN_MODULE_HEADERS+=xer_support.h
ASN_MODULE_SOURCES+=xer_support.c
ASN_MODULE_HEADERS+=xer_decoder.h
ASN_MODULE_SOURCES+=xer_decoder.c
ASN_MODULE_HEADERS+=xer_encoder.h
ASN_MODULE_SOURCES+=xer_encoder.c
ASN_MODULE_HEADERS+=per_support.h
ASN_MODULE_SOURCES+=per_support.c
ASN_MODULE_HEADERS+=per_decoder.h
ASN_MODULE_SOURCES+=per_decoder.c
ASN_MODULE_HEADERS+=per_encoder.h
ASN_MODULE_SOURCES+=per_encoder.c
ASN_MODULE_HEADERS+=per_opentype.h
ASN_MODULE_SOURCES+=per_opentype.c
