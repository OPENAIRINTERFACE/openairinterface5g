
//TS 24.501, chap 9.2 => TS 24.007
typedef enum {
	SGSsessionmanagementmessages=0x2e, //LTEbox: 0xC0 ???
SGSmobilitymanagementmessages=0x7e,
} Extendedprotocoldiscriminator_t;

typedef enum {
Registrationrequest=0x41,
Registrationaccept=0x42,
Registrationcomplete=0x43,
Registrationreject=0x44,
DeregistrationrequestUEoriginating=0x45,
DeregistrationacceptUEoriginating=0x46,
DeregistrationrequestUEterminated=0x47,
DeregistrationacceptUEterminated=0x48,
Servicerequest=0x4c,
Servicereject=0x4d,
Serviceaccept=0x4e,
Controlplaneservicerequest=0x4f,
Networkslicespecificauthenticationcommand=0x50,
Networkslicespecificauthenticationcomplete=0x51,
Networkslicespecificauthenticationresult=0x52,
Configurationupdatecommand=0x54,
Configurationupdatecomplete=0x55,
Authenticationrequest=0x56,
Authenticationresponse=0x57,
Authenticationreject=0x58,
Authenticationfailure=0x59,
Authenticationresult=0x5a,
Identityrequest=0x5b,
Identityresponse=0x5c,
Securitymodecommand=0x5d,
Securitymodecomplete=0x5e,
Securitymodereject=0x5f,
SGMMstatus=0x64,
Notification=0x65,
Notificationresponse=0x66,
ULNAStransport=0x67,
DLNAStransport=0x68,
PDUsessionestablishmentrequest=0xc1,
PDUsessionestablishmentaccept=0xc2,
PDUsessionestablishmentreject=0xc3,
PDUsessionauthenticationcommand=0xc5,
PDUsessionauthenticationcomplete=0xc6,
PDUsessionauthenticationresult=0xc7,
PDUsessionmodificationrequest=0xc9,
PDUsessionmodificationreject=0xca,
PDUsessionmodificationcommand=0xcb,
PDUsessionmodificationcomplete=0xcc,
PDUsessionmodificationcommandreject=0xcd,
PDUsessionreleaserequest=0xd1,
PDUsessionreleasereject=0xd2,
PDUsessionreleasecommand=0xd3,
PDUsessionreleasecomplete=0xd4,
SGSMstatus=0xd6,
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
SUCI=0,
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
typedef struct {
	int id;
	char text[256];
} cause_text_info_t; 
#define CAUSE_TEXT(LabEl, nUmID) {nUmID, #LabEl},

static const cause_text_info_t cause_text_info[] = {
  FOREACH_CAUSE(CAUSE_TEXT)
};

#define CAUSE_ENUM(LabEl, nUmID ) LabEl = nUmID,
//! Tasks id of each task
typedef enum {
  FOREACH_CAUSE(CAUSE_ENUM)
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

static const cause_text_info_t cause_secu_text_info[] = {
  FOREACH_CAUSE_SECU(CAUSE_TEXT)
};

#define CAUSE_ENUM(LabEl, nUmID ) LabEl = nUmID,
//! Tasks id of each task
typedef enum {
  FOREACH_CAUSE_SECU(CAUSE_ENUM)
} cause_secu_id_t;

		typedef struct {
	Extendedprotocoldiscriminator_t epd:8;
	Security_header_t sh:8;
	SGSmobilitymanagementmessages_t mt:8;
        identitytype_t it:8;
} Identityrequest_t;

// the message continues with the identity value, depending on identity type, see TS 14.501, 9.11.3.4
typedef struct {
	Extendedprotocoldiscriminator_t epd:8;
	Security_header_t sh:8;
	SGSmobilitymanagementmessages_t mt:8;
        identitytype_t mi:8;
} Identityresponse_t;


