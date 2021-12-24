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
 * Author and copyright: Laurent Thomas, open-cells.com
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
#ifndef NR_NAS_DEFS_H
#define NR_NAS_DEFS_H

#include <common/utils/LOG/log.h>
#include <openair3/UICC/usim_interface.h>
#include <openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h>

/* TS 24.007 possible L3 formats:
   Table 11.1: Formats of information elements
   Format Meaning IEI present LI present Value part present
   T Type only yes no no
   V Value only no no yes
   TV Type and Value yes no yes
   LV Length and Value no yes yes
   TLV Type, Length and Value yes yes yes
   LV-E Length and Value no yes yes
   TLV-E Type, Length and Value yes yes yes
*/

/* Map task id to printable name. */
typedef struct {
  int id;
  char text[256];
} text_info_t;

static inline const char * idToText(const text_info_t* table, int size, int id) {
  for(int i=0; i<size; i++)
    if (table[i].id==id)
      return table[i].text;
  LOG_E(NAS,"impossible id %x\n", id);
  return "IMPOSSIBLE";
}
#define idStr(TaBle, iD) idToText(TaBle, sizeof(TaBle)/sizeof(text_info_t), iD)

#define TO_TEXT(LabEl, nUmID) {nUmID, #LabEl},
#define TO_ENUM(LabEl, nUmID ) LabEl = nUmID,

//TS 24.501, chap 9.2 => TS 24.007
typedef enum {
  SGSsessionmanagementmessages=0x2e, //LTEbox: 0xC0 ???
  SGSmobilitymanagementmessages=0x7e,
} Extendedprotocoldiscriminator_t;

#define FOREACH_TYPE(TYPE_DEF) \
  TYPE_DEF(  Registrationrequest,0x41 )\
TYPE_DEF(  Registrationaccept,0x42 )\
TYPE_DEF(  Registrationcomplete,0x43 )\
TYPE_DEF(  Registrationreject,0x44 )\
TYPE_DEF(  DeregistrationrequestUEoriginating,0x45 )\
TYPE_DEF(  DeregistrationacceptUEoriginating,0x46 )\
TYPE_DEF(  DeregistrationrequestUEterminated,0x47 )\
TYPE_DEF(  DeregistrationacceptUEterminated,0x48 )\
TYPE_DEF(  Servicerequest,0x4c )\
TYPE_DEF(  Servicereject,0x4d )\
TYPE_DEF(  Serviceaccept,0x4e )\
TYPE_DEF(  Controlplaneservicerequest,0x4f )\
TYPE_DEF(  Networkslicespecificauthenticationcommand,0x50 )\
TYPE_DEF(  Networkslicespecificauthenticationcomplete,0x51 )\
TYPE_DEF(  Networkslicespecificauthenticationresult,0x52 )\
TYPE_DEF(  Configurationupdatecommand,0x54 )\
TYPE_DEF(  Configurationupdatecomplete,0x55 )\
TYPE_DEF(  Authenticationrequest,0x56 )\
TYPE_DEF(  Authenticationresponse,0x57 )\
TYPE_DEF(  Authenticationreject,0x58 )\
TYPE_DEF(  Authenticationfailure,0x59 )\
TYPE_DEF(  Authenticationresult,0x5a )\
TYPE_DEF(  Identityrequest,0x5b )\
TYPE_DEF(  Identityresponse,0x5c )\
TYPE_DEF(  Securitymodecommand,0x5d )\
TYPE_DEF(  Securitymodecomplete,0x5e )\
TYPE_DEF(  Securitymodereject,0x5f )\
TYPE_DEF(  SGMMstatus,0x64 )\
TYPE_DEF(  Notification,0x65 )\
TYPE_DEF(  Notificationresponse,0x66 )\
TYPE_DEF(  ULNAStransport,0x67 )\
TYPE_DEF(  DLNAStransport,0x68 )\
TYPE_DEF(  PDUsessionestablishmentrequest,0xc1 )\
TYPE_DEF(  PDUsessionestablishmentaccept,0xc2 )\
TYPE_DEF(  PDUsessionestablishmentreject,0xc3 )\
TYPE_DEF(  PDUsessionauthenticationcommand,0xc5 )\
TYPE_DEF(  PDUsessionauthenticationcomplete,0xc6 )\
TYPE_DEF(  PDUsessionauthenticationresult,0xc7 )\
TYPE_DEF(  PDUsessionmodificationrequest,0xc9 )\
TYPE_DEF(  PDUsessionmodificationreject,0xca )\
TYPE_DEF(  PDUsessionmodificationcommand,0xcb )\
TYPE_DEF(  PDUsessionmodificationcomplete,0xcc )\
TYPE_DEF(  PDUsessionmodificationcommandreject,0xcd )\
TYPE_DEF(  PDUsessionreleaserequest,0xd1 )\
TYPE_DEF(  PDUsessionreleasereject,0xd2 )\
TYPE_DEF(  PDUsessionreleasecommand,0xd3 )\
TYPE_DEF(  PDUsessionreleasecomplete,0xd4 )\
TYPE_DEF(  SGSMstatus,0xd6 )\

static const text_info_t message_text_info[] = {
  FOREACH_TYPE(TO_TEXT)
};

//! Tasks id of each task
typedef enum {
  FOREACH_TYPE(TO_ENUM)
} SGSmobilitymanagementmessages_t;

// TS 24.501
typedef enum {
  notsecurityprotected=0,
  Integrityprotected=1,
  Integrityprotectedandciphered=2,
  Integrityprotectedwithnew5GNASsecuritycontext=3,
  Integrityprotectedandcipheredwithnew5GNASsecuritycontext=4,
} Security_header_t;

typedef enum {
  SUCI=1,
  SGGUTI,
  IMEI,
  SGSTMSI,
  IMEISV,
  MACaddress,
  EUI64,
} identitytype_t;


// table  9.11.3.2.1
#define FOREACH_CAUSE(CAUSE_DEF) \
  CAUSE_DEF(Illegal_UE,0x3 )\
  CAUSE_DEF(PEI_not_accepted,0x5 )\
  CAUSE_DEF(Illegal_ME,0x6 )\
  CAUSE_DEF(SGS_services_not_allowed,0x7 )\
  CAUSE_DEF(UE_identity_cannot_be_derived_by_the_network,0x9 )\
  CAUSE_DEF(Implicitly_de_registered,0x0a )\
  CAUSE_DEF(PLMN_not_allowed,0x0b )\
  CAUSE_DEF(Tracking_area_not_allowed,0x0c )\
  CAUSE_DEF(Roaming_not_allowed_in_this_tracking_area,0x0d )\
  CAUSE_DEF(No_suitable_cells_in_tracking_area,0x0f )\
  CAUSE_DEF(MAC_failure,0x14 )\
  CAUSE_DEF(Synch_failure,0x15 )\
  CAUSE_DEF(Congestion,0x16 )\
  CAUSE_DEF(UE_security_capabilities_mismatch,0x17 )\
  CAUSE_DEF(Security_mode_rejected_unspecified,0x18 )\
  CAUSE_DEF(Non_5G_authentication_unacceptable,0x1a )\
  CAUSE_DEF(N1_mode_not_allowed,0x1b )\
  CAUSE_DEF(Restricted_service_area,0x1c )\
  CAUSE_DEF(Redirection_to_EPC_required,0x1f )\
  CAUSE_DEF(LADN_not_available,0x2b )\
  CAUSE_DEF(No_network_slices_available,0x3e )\
  CAUSE_DEF(Maximum_number_of_PDU_sessions_reached,0x41 )\
  CAUSE_DEF(Insufficient_resources_for_specific_slice_and_DNN,0x43 )\
  CAUSE_DEF(Insufficient_resources_for_specific_slice,0x45 )\
  CAUSE_DEF(ngKSI_already_in_use,0x47 )\
  CAUSE_DEF(Non_3GPP_access_to_5GCN_not_allowed,0x48 )\
  CAUSE_DEF(Serving_network_not_authorized,0x49 )\
  CAUSE_DEF(Temporarily_not_authorized_for_this_SNPN,0x4A )\
  CAUSE_DEF(Permanently_not_authorized_for_this_SNPN,0x4b )\
  CAUSE_DEF(Not_authorized_for_this_CAG_or_authorized_for_CAG_cells_only,0x4c )\
  CAUSE_DEF(Wireline_access_area_not_allowed,0x4d )\
  CAUSE_DEF(Payload_was_not_forwarded,0x5a )\
  CAUSE_DEF(DNN_not_supported_or_not_subscribed_in_the_slice,0x5b )\
  CAUSE_DEF(Insufficient_user_plane_resources_for_the_PDU_session,0x5c )\
  CAUSE_DEF(Semantically_incorrect_message,0x5f )\
  CAUSE_DEF(Invalid_mandatory_information,0x60 )\
  CAUSE_DEF(Message_type_non_existent_or_not_implemented,0x61 )\
  CAUSE_DEF(Message_type_not_compatible_with_the_protocol_state,0x62 )\
  CAUSE_DEF(Information_element_non_existent_or_not_implemented,0x63 )\
  CAUSE_DEF(Conditional_IE_error,0x64 )\
  CAUSE_DEF(Message_not_compatible_with_the_protocol_state,0x65 )\
  CAUSE_DEF(Protocol_error_unspecified,0x67 )

/* Map task id to printable name. */

#define CAUSE_TEXT(LabEl, nUmID) {nUmID, #LabEl},

static const text_info_t cause_text_info[] = {
  FOREACH_CAUSE(TO_TEXT)
};

#define CAUSE_ENUM(LabEl, nUmID ) LabEl = nUmID,
//! Tasks id of each task
typedef enum {
  FOREACH_CAUSE(TO_ENUM)
} cause_id_t;

//_table_9.11.4.2.1:_5GSM_cause_information_element
#define FOREACH_CAUSE_SECU(CAUSE_SECU_DEF) \
  CAUSE_SECU_DEF(Security_Operator_determined_barring,0x08 )\
  CAUSE_SECU_DEF(Security_Insufficient_resources,0x1a )\
  CAUSE_SECU_DEF(Security_Missing_or_unknown_DNN,0x1b )\
  CAUSE_SECU_DEF(Security_Unknown_PDU_session_type,0x1c )\
  CAUSE_SECU_DEF(Security_User_authentication_or_authorization_failed,0x1d )\
  CAUSE_SECU_DEF(Security_Request_rejected_unspecified,0x1f )\
  CAUSE_SECU_DEF(Security_Service_option_not_supported,0x20 )\
  CAUSE_SECU_DEF(Security_Requested_service_option_not_subscribed,0x21 )\
  CAUSE_SECU_DEF(Security_Service_option_temporarily_out_of_order,0x22 )\
  CAUSE_SECU_DEF(Security_PTI_already_in_use,0x23 )\
  CAUSE_SECU_DEF(Security_Regular_deactivation,0x24 )\
  CAUSE_SECU_DEF(Security_Network_failure,0x26 )\
  CAUSE_SECU_DEF(Security_Reactivation_requested,0x27 )\
  CAUSE_SECU_DEF(Security_Semantic_error_in_the_TFT_operation,0x29 )\
  CAUSE_SECU_DEF(Security_Syntactical_error_in_the_TFT_operation,0x2a )\
  CAUSE_SECU_DEF(Security_Invalid_PDU_session_identity,0x2b )\
  CAUSE_SECU_DEF(Security_Semantic_errors_in_packet_filter,0x2c )\
  CAUSE_SECU_DEF(Security_Syntactical_error_in_packet_filter,0x2d )\
  CAUSE_SECU_DEF(Security_Out_of_LADN_service_area,0x2e )\
  CAUSE_SECU_DEF(Security_PTI_mismatch,0x2f )\
  CAUSE_SECU_DEF(Security_PDU_session_type_IPv4_only_allowed,0x32 )\
  CAUSE_SECU_DEF(Security_PDU_session_type_IPv6_only_allowed,0x33 )\
  CAUSE_SECU_DEF(Security_PDU_session_does_not_exist,0x36 )\
  CAUSE_SECU_DEF(Security_PDU_session_type_IPv4v6_only_allowed,0x39 )\
  CAUSE_SECU_DEF(Security_PDU_session_type_Unstructured_only_allowed,0x3a )\
  CAUSE_SECU_DEF(Security_PDU_session_type_Ethernet_only_allowed,0x3d )\
  CAUSE_SECU_DEF(Security_Insufficient_resources_for_specific_slice_and_DNN,0x43 )\
  CAUSE_SECU_DEF(Security_Not_supported_SSC_mode,0x44 )\
  CAUSE_SECU_DEF(Security_Insufficient_resources_for_specific_slice,0x45 )\
  CAUSE_SECU_DEF(Security_Missing_or_unknown_DNN_in_a_slice,0x46 )\
  CAUSE_SECU_DEF(Security_Invalid_PTI_value,0x51 )\
  CAUSE_SECU_DEF(Security_Maximum_data_rate_per_UE_for_user_plane_integrity_protection_is_too_low,0x52 )\
  CAUSE_SECU_DEF(Security_Semantic_error_in_the_QoS_operation,0x53 )\
  CAUSE_SECU_DEF(Security_Syntactical_error_in_the_QoS_operation,0x54 )\
  CAUSE_SECU_DEF(Security_Invalid_mapped_EPS_bearer_identity,0x55 )\
  CAUSE_SECU_DEF(Security_Semantically_incorrect_message,0x5f )\
  CAUSE_SECU_DEF(Security_Invalid_mandatory_information,0x60 )\
  CAUSE_SECU_DEF(Security_Message_type_non_existent_or_not_implemented,0x61 )\
  CAUSE_SECU_DEF(Security_Message_type_not_compatible_with_the_protocol_state,0x62 )\
  CAUSE_SECU_DEF(Security_Information_element_non_existent_or_not_implemented,0x63 )\
  CAUSE_SECU_DEF(Security_Conditional_IE_error,0x64 )\
  CAUSE_SECU_DEF(Security_Message_not_compatible_with_the_protocol_state,0x65 )\
  CAUSE_SECU_DEF(Security_Protocol_error_unspecified,0x6f )

static const text_info_t cause_secu_text_info[] = {
  FOREACH_CAUSE_SECU(TO_TEXT)
};
								
//! Tasks id of each task
typedef enum {
  FOREACH_CAUSE_SECU(TO_ENUM)
} cause_secu_id_t;
								
// IEI (information element identifier) are spread in each message definition
#define IEI_RAND 0x21
#define IEI_AUTN 0x20
#define IEI_EAP  0x78
#define IEI_AuthenticationResponse 0x2d

//TS 24.501 sub layer states for UE
// for network side, only 5GMMderegistered, 5GMMderegistered_initiated, 5GMMregistered,  5GMMservice_request_initiated are valid
typedef enum {
  SGMMnull,
  SGMMderegistered,
  SGMMderegistered_initiated,
  SGMMregistered,
  SGMMregistered_initiated,
  SGMMservice_request_initiated,
} SGMM_UE_states;

typedef struct {
  Extendedprotocoldiscriminator_t epd:8;
  Security_header_t sh:8;
  SGSmobilitymanagementmessages_t mt:8;
} SGScommonHeader_t;

typedef struct {
  Extendedprotocoldiscriminator_t epd:8;
  Security_header_t sh:8;
  SGSmobilitymanagementmessages_t mt:8;
  identitytype_t it:8;
} Identityrequest_t;

// the message continues with the identity value, depending on identity type, see TS 14.501, 9.11.3.4
typedef struct __attribute__((packed)) {
  Extendedprotocoldiscriminator_t epd:8;
  Security_header_t sh:8;
  SGSmobilitymanagementmessages_t mt:8;
  uint16_t len;
}
Identityresponse_t;

typedef struct __attribute__((packed)) {
  Identityresponse_t common;
  identitytype_t mi:8;
  unsigned int supiFormat:4;
  unsigned int identityType:4;
  unsigned int mcc1:4;
  unsigned int mcc2:4;
  unsigned int mcc3:4;
  unsigned int mnc3:4;
  unsigned int mnc1:4;
  unsigned int mnc2:4;
  unsigned int routing1:4;
  unsigned int routing2:4;
  unsigned int routing3:4;
  unsigned int routing4:4;
  unsigned int protectScheme:4;
  unsigned int spare:4;
  uint8_t hplmnId;
}
IdentityresponseIMSI_t;

typedef struct  __attribute__((packed)) {
  Extendedprotocoldiscriminator_t epd:8;
  Security_header_t sh:8;
  SGSmobilitymanagementmessages_t mt:8;
  unsigned int ngKSI:4;
  unsigned int spare:4;
  unsigned int ABBALen:8;
  unsigned int ABBA:16;
  uint8_t ieiRAND;
  uint8_t RAND[16];
  uint8_t ieiAUTN;
  uint8_t AUTNlen;
  uint8_t AUTN[16];
}
authenticationrequestHeader_t;

typedef struct  __attribute__((packed)) {
  Extendedprotocoldiscriminator_t epd:8;
  Security_header_t sh:8;
  SGSmobilitymanagementmessages_t mt:8;
  uint8_t iei;
  uint8_t RESlen;
  uint8_t RES[16];
} authenticationresponse_t;

//AUTHENTICATION RESULT

typedef struct  __attribute__((packed)) {
  Extendedprotocoldiscriminator_t epd:8;
  Security_header_t sh:8;
  SGSmobilitymanagementmessages_t mt:8;

  unsigned int selectedNASsecurityalgorithms;
  unsigned int ngKSI:4; //ngKSI NAS key set identifier 9.11.3.32
  unsigned int spare:4;
  // LV 3-9 bytes Replayed UE security capabilities UE security capability 9.11.3.54
  
  /* optional
     TV (E-, 1 byte) Oprional IMEISV request IMEISV request 9.11.3.28
     TV (57, 2 bytes ) Selected EPS NAS security algorithms  EPS NAS security algorithms 9.11.3.25
     TLV (36, 3 bytes) Additional 5G security information Additional 5G security information 9.11.3.12
     TLV-E (78,, 7-1503 bytes) EAP message EAP message 9.11.2.2
     TLV (38, 4-n)ABBA ABBA 9.11.3.10
     TLV (19, 4-7) Replayed S1 UE security capabilities S1 UE security capability 9.11.3.48A
  */

} securityModeCommand_t;

typedef struct {
  uicc_t *uicc;
} nr_user_nas_t;

#define STATIC_ASSERT(test_for_true) _Static_assert((test_for_true), "(" #test_for_true ") failed")
#define myCalloc(var, type) type * var=(type*)calloc(sizeof(type),1);
#define arrayCpy(tO, FroM)  STATIC_ASSERT(sizeof(tO) == sizeof(FroM)) ; memcpy(tO, FroM, sizeof(tO))
int resToresStar(uint8_t *msg, uicc_t* uicc);

int identityResponse(void **msg, nr_user_nas_t *UE);
int authenticationResponse(void **msg, nr_user_nas_t *UE);
void UEprocessNAS(void *msg,nr_user_nas_t *UE);
void SGSabortUE(void *msg, NRUEcontext_t *UE) ;
void SGSregistrationReq(void *msg, NRUEcontext_t *UE);
void SGSderegistrationUEReq(void *msg, NRUEcontext_t *UE);
void SGSauthenticationResp(void *msg, NRUEcontext_t *UE);
void SGSidentityResp(void *msg, NRUEcontext_t *UE);
void SGSsecurityModeComplete(void *msg, NRUEcontext_t *UE);
void SGSregistrationComplete(void *msg, NRUEcontext_t *UE);
void processNAS(void *msg, NRUEcontext_t *UE);
int identityRequest(void **msg, NRUEcontext_t *UE);
int authenticationRequest(void **msg, NRUEcontext_t *UE);
int securityModeCommand(void **msg, NRUEcontext_t *UE);
void servingNetworkName(uint8_t *msg, char * imsiStr, int nmc_size);

#endif
