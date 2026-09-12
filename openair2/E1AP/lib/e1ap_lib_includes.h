/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E1AP_LIB_INCLUDES_H_
#define E1AP_LIB_INCLUDES_H_

#include "E1AP_E1AP-PDU.h"
#include "E1AP_ProcedureCode.h"
#include "E1AP_SuccessfulOutcome.h"
#include "E1AP_UnsuccessfulOutcome.h"
#include "E1AP_CriticalityDiagnostics-IE-List.h"
#include "E1AP_InitiatingMessage.h"
#include "E1AP_ProtocolIE-ID.h"
#include "E1AP_ProtocolIE-Field.h"
#include "E1AP_UP-TNL-Information.h"
#include "E1AP_DefaultDRB.h"
#include "E1AP_QoS-Characteristics.h"
#include "E1AP_PDU-Session-Resource-To-Setup-Item.h"
#include "E1AP_GTPTunnel.h"
#include "E1AP_DRB-To-Setup-Item-NG-RAN.h"
#include "E1AP_T-ReorderingTimer.h"
#include "E1AP_QoS-Flow-QoS-Parameter-Item.h"
#include "E1AP_Cell-Group-Information-Item.h"
#include "E1AP_Dynamic5QIDescriptor.h"
#include "E1AP_Non-Dynamic5QIDescriptor.h"
#include "E1AP_SecurityResult.h"
#include "E1AP_Data-Forwarding-Information.h"
#include "E1AP_PDCP-SN-Status-Information.h"
#include "E1AP_PDCP-Count.h"

// E1 Bearer Context Setup Request
#include "E1AP_BearerContextSetupRequest.h"
#include "E1AP_GNB-CU-CP-E1SetupRequest.h"
#include "E1AP_GNB-CU-UP-E1SetupRequest.h"
#include "E1AP_System-BearerContextSetupRequest.h"
#include "E1AP_MaximumIPdatarate.h"
#include "E1AP_PDU-Session-Type.h"
#include "E1AP_Data-Forwarding-Information.h"
// E1 Bearer Context Setup Response
#include "E1AP_PDU-Session-Resource-Setup-Item.h"
#include "E1AP_DRB-Setup-Item-NG-RAN.h"
#include "E1AP_UP-Parameters-Item.h"
#include "E1AP_QoS-Flow-Item.h"
#include "E1AP_DRB-Failed-List-NG-RAN.h"
#include "E1AP_DRB-Failed-Item-NG-RAN.h"
// E1 Setup
#include "E1AP_SupportedPLMNs-Item.h"
#include "E1AP_Slice-Support-List.h"
#include "E1AP_Slice-Support-Item.h"
#include "E1AP_ProtocolIE-Field.h"
#include "E1AP_Transport-UP-Layer-Addresses-Info-To-Add-List.h"
#include "E1AP_Transport-UP-Layer-Addresses-Info-To-Add-Item.h"
#include "E1AP_Transport-UP-Layer-Addresses-Info-To-Remove-List.h"
#include "E1AP_Transport-UP-Layer-Addresses-Info-To-Remove-Item.h"
#include "E1AP_GTPTLAs.h"
#include "E1AP_GTPTLA-Item.h"
// E1 Bearer Context Modification Request
#include "E1AP_PDU-Session-Resource-To-Modify-Item.h"
#include "E1AP_DRB-To-Modify-List-NG-RAN.h"
#include "E1AP_DRB-To-Modify-Item-NG-RAN.h"
#include "E1AP_DRB-To-Remove-List-NG-RAN.h"
#include "E1AP_DRB-To-Remove-Item-NG-RAN.h"
#include "E1AP_PDU-Session-Resource-To-Setup-Mod-Item.h"
#include "E1AP_DRB-To-Setup-Mod-Item-NG-RAN.h"
#include "E1AP_PDU-Session-Resource-To-Remove-Item.h"
#include "E1AP_ProtocolExtensionContainer.h"
#include "E1AP_ProtocolExtensionField.h"
// E1 Bearer Context Modification Required / Confirm
#include "E1AP_BearerContextModificationRequired.h"
#include "E1AP_System-BearerContextModificationRequired.h"
#include "E1AP_BearerContextModificationConfirm.h"
// E1 Bearer Context Modification Response
#include "E1AP_PDU-Session-Resource-Modified-Item.h"
#include "E1AP_DRB-Modified-List-NG-RAN.h"
#include "E1AP_DRB-Modified-Item-NG-RAN.h"
#include "E1AP_DRB-Failed-To-Modify-List-NG-RAN.h"
#include "E1AP_QoS-Flow-Failed-List.h"
#include "E1AP_QoS-Flow-Failed-Item.h"
#include "E1AP_DRB-Failed-To-Modify-Item-NG-RAN.h"
#include "E1AP_ActivityNotificationLevel.h"

#endif /* E1AP_LIB_INCLUDES_H_ */
