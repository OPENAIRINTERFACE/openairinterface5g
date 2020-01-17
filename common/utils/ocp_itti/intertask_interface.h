/*
  Author: Laurent THOMAS, Open Cells
  Copyleft: OpenAirInterface software alliance and it's licence
*/
#ifndef INTERTASK_INTERFACE_H_
#define INTERTASK_INTERFACE_H_
#include <stdint.h>
#include <sys/epoll.h>

#include <mem_block.h>
#include <assertions.h>


typedef enum timer_type_s {
  TIMER_PERIODIC,
  TIMER_ONE_SHOT,
  TIMER_TYPE_MAX,
} timer_type_t;

typedef struct {
  void *arg;
  long  timer_id;
} timer_has_expired_t;

typedef struct {
  uint32_t      interval_sec;
  uint32_t      interval_us;
  long     task_id;
  int32_t       instance;
  timer_type_t  type;
  void         *timer_arg;
  long          timer_id;
} timer_create_t;

typedef struct {
  long     task_id;
  long          timer_id;
} timer_delete_t;


typedef struct itti_lte_time_s {
  uint32_t frame;
  uint8_t slot;
} itti_lte_time_t;


typedef struct IttiMsgEmpty_s {
} IttiMsgEmpty;

typedef struct IttiMsgText_s {
  uint32_t  size;
  char      text[];
} IttiMsgText;

#include <openair2/COMMON/phy_messages_types.h>
#include <openair2/COMMON/mac_messages_types.h>
#include <openair2/COMMON/rlc_messages_types.h>
#include <openair2/COMMON/pdcp_messages_types.h>
#include <openair2/COMMON/networkDef.h>
#include <openair2/COMMON/as_message.h>
#include <openair2/RRC/LTE/rrc_types.h>
#include <openair2/COMMON/rrc_messages_types.h>

#include <openair3/NAS/COMMON/UTIL/OctetString.h>
#include <openair3/NAS/COMMON/IES/AccessPointName.h>
#include <openair3/NAS/COMMON/IES/AdditionalUpdateResult.h>
#include <openair3/NAS/COMMON/IES/AdditionalUpdateType.h>
#include <openair3/NAS/COMMON/IES/ApnAggregateMaximumBitRate.h>
#include <openair3/NAS/COMMON/IES/AuthenticationFailureParameter.h>
#include <openair3/NAS/COMMON/IES/AuthenticationParameterAutn.h>
#include <openair3/NAS/COMMON/IES/AuthenticationParameterRand.h>
#include <openair3/NAS/COMMON/IES/AuthenticationResponseParameter.h>
#include <openair3/NAS/COMMON/IES/CipheringKeySequenceNumber.h>
#include <openair3/NAS/COMMON/IES/Cli.h>
#include <openair3/NAS/COMMON/IES/CsfbResponse.h>
#include <openair3/NAS/COMMON/IES/DaylightSavingTime.h>
#include <openair3/NAS/COMMON/IES/DetachType.h>
#include <openair3/NAS/COMMON/IES/DrxParameter.h>
#include <openair3/NAS/COMMON/IES/EmergencyNumberList.h>
#include <openair3/NAS/COMMON/IES/EmmCause.h>
#include <openair3/NAS/COMMON/IES/EpsAttachResult.h>
#include <openair3/NAS/COMMON/IES/EpsAttachType.h>
#include <openair3/NAS/COMMON/IES/EpsBearerContextStatus.h>
#include <openair3/NAS/COMMON/IES/EpsBearerIdentity.h>
#include <openair3/NAS/COMMON/IES/EpsMobileIdentity.h>
#include <openair3/NAS/COMMON/IES/EpsNetworkFeatureSupport.h>
#include <openair3/NAS/COMMON/IES/EpsQualityOfService.h>
#include <openair3/NAS/COMMON/IES/EpsUpdateResult.h>
#include <openair3/NAS/COMMON/IES/EpsUpdateType.h>
#include <openair3/NAS/COMMON/IES/EsmCause.h>
#include <openair3/NAS/COMMON/IES/EsmInformationTransferFlag.h>
#include <openair3/NAS/COMMON/IES/EsmMessageContainer.h>
#include <openair3/NAS/COMMON/IES/GprsTimer.h>
#include <openair3/NAS/COMMON/IES/GutiType.h>
#include <openair3/NAS/COMMON/IES/IdentityType2.h>
#include <openair3/NAS/COMMON/IES/ImeisvRequest.h>
#include <openair3/NAS/COMMON/IES/KsiAndSequenceNumber.h>
#include <openair3/NAS/COMMON/IES/LcsClientIdentity.h>
#include <openair3/NAS/COMMON/IES/LcsIndicator.h>
#include <openair3/NAS/COMMON/IES/LinkedEpsBearerIdentity.h>
#include <openair3/NAS/COMMON/IES/LlcServiceAccessPointIdentifier.h>
#include <openair3/NAS/COMMON/IES/LocationAreaIdentification.h>
#include <openair3/NAS/COMMON/IES/MessageType.h>
#include <openair3/NAS/COMMON/IES/MobileIdentity.h>
#include <openair3/NAS/COMMON/IES/MobileStationClassmark2.h>
#include <openair3/NAS/COMMON/IES/MobileStationClassmark3.h>
#include <openair3/NAS/COMMON/IES/MsNetworkCapability.h>
#include <openair3/NAS/COMMON/IES/MsNetworkFeatureSupport.h>
#include <openair3/NAS/COMMON/IES/NasKeySetIdentifier.h>
#include <openair3/NAS/COMMON/IES/NasMessageContainer.h>
#include <openair3/NAS/COMMON/IES/NasRequestType.h>
#include <openair3/NAS/COMMON/IES/NasSecurityAlgorithms.h>
#include <openair3/NAS/COMMON/IES/NetworkName.h>
#include <openair3/NAS/COMMON/IES/Nonce.h>
#include <openair3/NAS/COMMON/IES/PacketFlowIdentifier.h>
#include <openair3/NAS/COMMON/IES/PagingIdentity.h>
#include <openair3/NAS/COMMON/IES/PdnAddress.h>
#include <openair3/NAS/COMMON/IES/PdnType.h>
#include <openair3/NAS/COMMON/IES/PlmnList.h>
#include <openair3/NAS/COMMON/IES/ProcedureTransactionIdentity.h>
#include <openair3/NAS/COMMON/IES/ProtocolConfigurationOptions.h>
#include <openair3/NAS/COMMON/IES/ProtocolDiscriminator.h>
#include <openair3/NAS/COMMON/IES/PTmsiSignature.h>
#include <openair3/NAS/COMMON/IES/QualityOfService.h>
#include <openair3/NAS/COMMON/IES/RadioPriority.h>
#include <openair3/NAS/COMMON/IES/SecurityHeaderType.h>
#include <openair3/NAS/COMMON/IES/ServiceType.h>
#include <openair3/NAS/COMMON/IES/ShortMac.h>
#include <openair3/NAS/COMMON/IES/SsCode.h>
#include <openair3/NAS/COMMON/IES/SupportedCodecList.h>
#include <openair3/NAS/COMMON/IES/TimeZoneAndTime.h>
#include <openair3/NAS/COMMON/IES/TimeZone.h>
#include <openair3/NAS/COMMON/IES/TmsiStatus.h>
#include <openair3/NAS/COMMON/IES/TrackingAreaIdentity.h>
#include <openair3/NAS/COMMON/IES/TrackingAreaIdentityList.h>
#include <openair3/NAS/COMMON/IES/TrafficFlowAggregateDescription.h>
#include <openair3/NAS/COMMON/IES/TrafficFlowTemplate.h>
#include <openair3/NAS/COMMON/IES/TransactionIdentifier.h>
#include <openair3/NAS/COMMON/IES/UeNetworkCapability.h>
#include <openair3/NAS/COMMON/IES/UeRadioCapabilityInformationUpdateNeeded.h>
#include <openair3/NAS/COMMON/IES/UeSecurityCapability.h>
#include <openair3/NAS/COMMON/IES/VoiceDomainPreferenceAndUeUsageSetting.h>
#include <openair3/NAS/COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextAccept.h>
#include <openair3/NAS/COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextReject.h>
#include <openair3/NAS/COMMON/ESM/MSG/ActivateDedicatedEpsBearerContextRequest.h>
#include <openair3/NAS/COMMON/ESM/MSG/ActivateDefaultEpsBearerContextAccept.h>
#include <openair3/NAS/COMMON/ESM/MSG/ActivateDefaultEpsBearerContextReject.h>
#include <openair3/NAS/COMMON/ESM/MSG/ActivateDefaultEpsBearerContextRequest.h>
#include <openair3/NAS/COMMON/ESM/MSG/BearerResourceAllocationReject.h>
#include <openair3/NAS/COMMON/ESM/MSG/BearerResourceAllocationRequest.h>
#include <openair3/NAS/COMMON/ESM/MSG/BearerResourceModificationReject.h>
#include <openair3/NAS/COMMON/ESM/MSG/BearerResourceModificationRequest.h>
#include <openair3/NAS/COMMON/ESM/MSG/DeactivateEpsBearerContextAccept.h>
#include <openair3/NAS/COMMON/ESM/MSG/DeactivateEpsBearerContextRequest.h>
#include <openair3/NAS/COMMON/ESM/MSG/esm_cause.h>
#include <openair3/NAS/COMMON/ESM/MSG/EsmInformationRequest.h>
#include <openair3/NAS/COMMON/ESM/MSG/EsmInformationResponse.h>
#include <openair3/NAS/COMMON/ESM/MSG/EsmStatus.h>
#include <openair3/NAS/COMMON/ESM/MSG/ModifyEpsBearerContextAccept.h>
#include <openair3/NAS/COMMON/ESM/MSG/ModifyEpsBearerContextReject.h>
#include <openair3/NAS/COMMON/ESM/MSG/ModifyEpsBearerContextRequest.h>
#include <openair3/NAS/COMMON/ESM/MSG/PdnConnectivityReject.h>
#include <openair3/NAS/COMMON/ESM/MSG/PdnConnectivityRequest.h>
#include <openair3/NAS/COMMON/ESM/MSG/PdnDisconnectReject.h>
#include <openair3/NAS/COMMON/ESM/MSG/PdnDisconnectRequest.h>
#include <openair3/NAS/COMMON/ESM/MSG/esm_msgDef.h>
#include <openair3/NAS/COMMON/ESM/MSG/esm_msg.h>

#include <openair3/NAS/COMMON/EMM/MSG/AttachAccept.h>
#include <openair3/NAS/COMMON/EMM/MSG/AttachComplete.h>
#include <openair3/NAS/COMMON/EMM/MSG/AttachReject.h>
#include <openair3/NAS/COMMON/EMM/MSG/AttachRequest.h>
#include <openair3/NAS/COMMON/EMM/MSG/AuthenticationFailure.h>
#include <openair3/NAS/COMMON/EMM/MSG/AuthenticationReject.h>
#include <openair3/NAS/COMMON/EMM/MSG/AuthenticationRequest.h>
#include <openair3/NAS/COMMON/EMM/MSG/AuthenticationResponse.h>
#include <openair3/NAS/COMMON/EMM/MSG/CsServiceNotification.h>
#include <openair3/NAS/COMMON/EMM/MSG/DetachAccept.h>
#include <openair3/NAS/COMMON/EMM/MSG/DetachRequest.h>
#include <openair3/NAS/COMMON/EMM/MSG/DownlinkNasTransport.h>
#include <openair3/NAS/COMMON/EMM/MSG/emm_cause.h>
#include <openair3/NAS/COMMON/EMM/MSG/EmmInformation.h>
#include <openair3/NAS/COMMON/EMM/MSG/EmmStatus.h>
#include <openair3/NAS/COMMON/EMM/MSG/ExtendedServiceRequest.h>
#include <openair3/NAS/COMMON/EMM/MSG/GutiReallocationCommand.h>
#include <openair3/NAS/COMMON/EMM/MSG/GutiReallocationComplete.h>
#include <openair3/NAS/COMMON/EMM/MSG/IdentityRequest.h>
#include <openair3/NAS/COMMON/EMM/MSG/IdentityResponse.h>
#include <openair3/NAS/COMMON/EMM/MSG/NASSecurityModeCommand.h>
#include <openair3/NAS/COMMON/EMM/MSG/NASSecurityModeComplete.h>
#include <openair3/NAS/COMMON/EMM/MSG/SecurityModeReject.h>
#include <openair3/NAS/COMMON/EMM/MSG/ServiceReject.h>
#include <openair3/NAS/COMMON/EMM/MSG/ServiceRequest.h>
#include <openair3/NAS/COMMON/EMM/MSG/TrackingAreaUpdateAccept.h>
#include <openair3/NAS/COMMON/EMM/MSG/TrackingAreaUpdateComplete.h>
#include <openair3/NAS/COMMON/EMM/MSG/TrackingAreaUpdateReject.h>
#include <openair3/NAS/COMMON/EMM/MSG/TrackingAreaUpdateRequest.h>
#include <openair3/NAS/COMMON/EMM/MSG/UplinkNasTransport.h>
#include <openair3/NAS/COMMON/EMM/MSG/emm_msgDef.h>
#include <openair3/NAS/COMMON/EMM/MSG/emm_msg.h>

#include <openair3/NAS/COMMON/API/NETWORK/nas_message.h>
#include <openair2/COMMON/nas_messages_types.h>
#if ENABLE_RAL
  #include <ral_messages_types.h>
#endif
#include <openair2/COMMON/s1ap_messages_types.h>
#include <openair2/COMMON/x2ap_messages_types.h>
#include <openair2/COMMON/m2ap_messages_types.h>
#include <openair2/COMMON/m3ap_messages_types.h>
#include <openair2/COMMON/sctp_messages_types.h>
#include <openair2/COMMON/udp_messages_types.h>
#include <openair2/COMMON/gtpv1_u_messages_types.h>
#include <openair3/SCTP/sctp_eNB_task.h>
#include <openair3/NAS/UE/nas_proc_defs.h>
#include <openair3/NAS/UE/ESM/esmData.h>
#include <openair3/NAS/COMMON/UTIL/nas_timer.h>
#include <openair3/NAS/UE/ESM/esm_pt_defs.h>
#include <openair3/NAS/UE/EMM/emm_proc_defs.h>
#include <openair3/NAS/UE/EMM/emmData.h>
#include <openair3/NAS/UE/EMM/IdleMode_defs.h>
#include <openair3/NAS/UE/EMM/emm_fsm_defs.h>
#include <openair3/NAS/UE/EMM/emmData.h>
#include <openair3/NAS/COMMON/securityDef.h>
#include <openair3/NAS/UE/EMM/Authentication.h>
#include <openair3/NAS/UE/EMM/SecurityModeControl.h>
#include <openair3/NAS/UE/API/USIM/usim_api.h>
#include <openair3/NAS/COMMON/userDef.h>
#include <openair3/NAS/UE/API/USER/at_command.h>
#include <openair3/NAS/UE/API/USER/at_response.h>
#include <openair3/NAS/UE/API/USER/user_api_defs.h>
#include <openair3/NAS/UE/EMM/LowerLayer_defs.h>
#include <openair3/NAS/UE/user_defs.h>
#include <openair3/NAS/UE/nas_ue_task.h>
#include <openair3/S1AP/s1ap_eNB.h>
#include <openair3/MME_APP/mme_app.h>
//#include <proto.h>

#include <openair3/GTPV1-U/gtpv1u_eNB_task.h>
void *rrc_enb_process_itti_msg(void *);
#include <openair3/SCTP/sctp_eNB_task.h>
#include <openair3/S1AP/s1ap_eNB.h>

/*
  static const char *const messages_definition_xml = {
  #include <messages_xml.h>
  };
*/

typedef uint32_t MessageHeaderSize;
typedef uint32_t itti_message_types_t;
typedef unsigned long message_number_t;
#define MESSAGE_NUMBER_SIZE (sizeof(unsigned long))

typedef enum task_priorities_e {
  TASK_PRIORITY_MAX       = 100,
  TASK_PRIORITY_MAX_LEAST = 85,
  TASK_PRIORITY_MED_PLUS  = 70,
  TASK_PRIORITY_MED       = 55,
  TASK_PRIORITY_MED_LEAST = 40,
  TASK_PRIORITY_MIN_PLUS  = 25,
  TASK_PRIORITY_MIN       = 10,
} task_priorities_t;

typedef struct {
  task_priorities_t priority;
  unsigned int queue_size;
  /* Printable name */
  char name[256];
  void *(*func)(void *) ;
  void *(*threadFunc)(void *) ;
} task_info_t;
//
//TASK_DEF(TASK_RRC_ENB,  TASK_PRIORITY_MED,  200, NULL,NULL)
//TASK_DEF(TASK_RRC_ENB,  TASK_PRIORITY_MED,  200, NULL, NULL)
//TASK_DEF(TASK_GTPV1_U,  TASK_PRIORITY_MED,  1000,NULL, NULL)
//TASK_DEF(TASK_UDP,      TASK_PRIORITY_MED,  1000, NULL, NULL)

#define FOREACH_TASK(TASK_DEF) \
  TASK_DEF(TASK_UNKNOWN,  TASK_PRIORITY_MED, 50, NULL, NULL)  \
  TASK_DEF(TASK_TIMER,    TASK_PRIORITY_MED, 10, NULL, NULL)   \
  TASK_DEF(TASK_L2L1,     TASK_PRIORITY_MAX, 200, NULL, NULL)   \
  TASK_DEF(TASK_BM,       TASK_PRIORITY_MED, 200, NULL, NULL)   \
  TASK_DEF(TASK_PHY_ENB,  TASK_PRIORITY_MED, 200, NULL, NULL)   \
  TASK_DEF(TASK_MAC_ENB,  TASK_PRIORITY_MED, 200, NULL, NULL)   \
  TASK_DEF(TASK_RLC_ENB,  TASK_PRIORITY_MED, 200, NULL, NULL)   \
  TASK_DEF(TASK_RRC_ENB_NB_IoT,  TASK_PRIORITY_MED, 200, NULL, NULL) \
  TASK_DEF(TASK_PDCP_ENB, TASK_PRIORITY_MED, 200, NULL, NULL)   \
  TASK_DEF(TASK_DATA_FORWARDING, TASK_PRIORITY_MED, 200, NULL, NULL)   \
  TASK_DEF(TASK_END_MARKER, TASK_PRIORITY_MED, 200, NULL, NULL)   \
  TASK_DEF(TASK_RRC_ENB,  TASK_PRIORITY_MED,  200, NULL,NULL)\
  TASK_DEF(TASK_RAL_ENB,  TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_S1AP,     TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_X2AP,     TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_M2AP_ENB,     TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_M2AP_MCE,     TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_M3AP,     TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_M3AP_MME,     TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_M3AP_MCE,     TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_SCTP,     TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_ENB_APP,  TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_MCE_APP,  TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_MME_APP,  TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_FLEXRAN_AGENT,TASK_PRIORITY_MED, 200, NULL, NULL) \
  TASK_DEF(TASK_PHY_UE,   TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_MAC_UE,   TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_RLC_UE,   TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_PDCP_UE,  TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_RRC_UE,   TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_NAS_UE,   TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_RAL_UE,   TASK_PRIORITY_MED,  200, NULL, NULL)  \
  TASK_DEF(TASK_MSC,      TASK_PRIORITY_MED,  200, NULL, NULL)\
  TASK_DEF(TASK_GTPV1_U,  TASK_PRIORITY_MED,  1000,NULL, NULL)\
  TASK_DEF(TASK_UDP,      TASK_PRIORITY_MED,  1000, NULL, NULL)\
  TASK_DEF(TASK_CU_F1,    TASK_PRIORITY_MED,  200, NULL, NULL) \
  TASK_DEF(TASK_DU_F1,    TASK_PRIORITY_MED,  200, NULL, NULL) \
  TASK_DEF(TASK_MAX,      TASK_PRIORITY_MED,  200, NULL, NULL)

#define TASK_DEF(TaskID, pRIO, qUEUEsIZE, FuNc, ThreadFunc)          { pRIO, qUEUEsIZE, #TaskID, FuNc, ThreadFunc },

/* Map task id to printable name. */
static const task_info_t tasks_info[] = {
  FOREACH_TASK(TASK_DEF)
};

#define TASK_ENUM(TaskID, pRIO, qUEUEsIZE, FuNc,ThreadFunc ) TaskID,
//! Tasks id of each task
typedef enum {
  FOREACH_TASK(TASK_ENUM)
} task_id_t;


typedef task_id_t thread_id_t;

typedef enum message_priorities_e {
  MESSAGE_PRIORITY_MAX       = 100,
  MESSAGE_PRIORITY_MAX_LEAST = 85,
  MESSAGE_PRIORITY_MED_PLUS  = 70,
  MESSAGE_PRIORITY_MED       = 55,
  MESSAGE_PRIORITY_MED_LEAST = 40,
  MESSAGE_PRIORITY_MIN_PLUS  = 25,
  MESSAGE_PRIORITY_MIN       = 10,
} message_priorities_t;


#define FOREACH_MSG(INTERNAL_MSG)         \
  INTERNAL_MSG(TIMER_HAS_EXPIRED,  MESSAGE_PRIORITY_MED, timer_has_expired_t, timer_has_expired) \
  INTERNAL_MSG(INITIALIZE_MESSAGE, MESSAGE_PRIORITY_MED, IttiMsgEmpty, initialize_message)  \
  INTERNAL_MSG(ACTIVATE_MESSAGE,   MESSAGE_PRIORITY_MED, IttiMsgEmpty, activate_message) \
  INTERNAL_MSG(DEACTIVATE_MESSAGE, MESSAGE_PRIORITY_MED, IttiMsgEmpty, deactivate_message)  \
  INTERNAL_MSG(TERMINATE_MESSAGE,  MESSAGE_PRIORITY_MAX, IttiMsgEmpty, terminate_message) \
  INTERNAL_MSG(MESSAGE_TEST,       MESSAGE_PRIORITY_MED, IttiMsgEmpty, message_test)

/* This enum defines messages ids. Each one is unique. */
typedef enum {
#define MESSAGE_DEF(iD, pRIO, sTRUCT, fIELDnAME) iD,
  FOREACH_MSG(MESSAGE_DEF)
#include <all_msg.h>
#undef MESSAGE_DEF
  MESSAGES_ID_MAX,
} MessagesIds;

typedef union msg_s {
#define MESSAGE_DEF(iD, pRIO, sTRUCT, fIELDnAME) sTRUCT fIELDnAME;
  FOREACH_MSG(MESSAGE_DEF)
#include <all_msg.h>
#undef MESSAGE_DEF
} msg_t;

typedef struct MessageHeader_s {
  MessagesIds messageId;          /**< Unique message id as referenced in enum MessagesIds */
  task_id_t  originTaskId;        /**< ID of the sender task */
  task_id_t  destinationTaskId;   /**< ID of the destination task */
  instance_t instance;            /**< Task instance for virtualization */
  itti_lte_time_t lte_time;
  MessageHeaderSize ittiMsgSize;         /**< Message size (not including header size) */
} MessageHeader;

typedef struct message_info_s {
  int id;
  message_priorities_t priority;
  /* Message payload size */
  MessageHeaderSize size;
  /* Printable name */
  const char name[256];
} message_info_t;

/* Map message id to message information */
static const message_info_t messages_info[] = {
#define MESSAGE_DEF(iD, pRIO, sTRUCT, fIELDnAME) { iD, pRIO, sizeof(sTRUCT), #iD },
  FOREACH_MSG(MESSAGE_DEF)
#include <all_msg.h>
#undef MESSAGE_DEF
};

typedef struct __attribute__ ((__packed__)) MessageDef_s {
  MessageHeader ittiMsgHeader; /**< Message header */
  msg_t         ittiMsg;
} MessageDef;



/* Extract the instance from a message */
#define ITTI_MESSAGE_GET_INSTANCE(mESSAGE)  ((mESSAGE)->ittiMsgHeader.instance)
#define ITTI_MSG_ID(mSGpTR)                 ((mSGpTR)->ittiMsgHeader.messageId)
#define ITTI_MSG_ORIGIN_ID(mSGpTR)          ((mSGpTR)->ittiMsgHeader.originTaskId)
#define ITTI_MSG_DESTINATION_ID(mSGpTR)     ((mSGpTR)->ittiMsgHeader.destinationTaskId)
#define ITTI_MSG_INSTANCE(mSGpTR)           ((mSGpTR)->ittiMsgHeader.instance)
#define ITTI_MSG_NAME(mSGpTR)               itti_get_message_name(ITTI_MSG_ID(mSGpTR))
#define ITTI_MSG_ORIGIN_NAME(mSGpTR)        itti_get_task_name(ITTI_MSG_ORIGIN_ID(mSGpTR))
#define ITTI_MSG_DESTINATION_NAME(mSGpTR)   itti_get_task_name(ITTI_MSG_DESTINATION_ID(mSGpTR))
#define TIMER_HAS_EXPIRED(mSGpTR)           (mSGpTR)->ittiMsg.timer_has_expired

#define INSTANCE_DEFAULT    (UINT16_MAX - 1)

static inline int64_t clock_difftime_ns(struct timespec start, struct timespec end) {
  return (int64_t)( end.tv_sec-start.tv_sec) * (int64_t)(1000*1000*1000) + end.tv_nsec-start.tv_nsec;
}

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Send a message to a task (could be itself)
  \param task_id Task ID
  \param instance Instance of the task used for virtualization
  \param message Pointer to the message to send
  @returns -1 on failure, 0 otherwise
 **/
int itti_send_msg_to_task(task_id_t task_id, instance_t instance, MessageDef *message);

/** \brief Add a new fd to monitor.
   NOTE: it is up to the user to read data associated with the fd
    \param task_id Task ID of the receiving task
    \param fd The file descriptor to monitor
 **/
void itti_subscribe_event_fd(task_id_t task_id, int fd);

/** \brief Remove a fd from the list of fd to monitor
    \param task_id Task ID of the task
    \param fd The file descriptor to remove
 **/
void itti_unsubscribe_event_fd(task_id_t task_id, int fd);

/** \brief Return the list of events excluding the fd associated with itti
    \param task_id Task ID of the task
    \param events events list
    @returns number of events to handle
 **/
int itti_get_events(task_id_t task_id, struct epoll_event **events);

/** \brief Retrieves a message in the queue associated to task_id.
   If the queue is empty, the thread is blocked till a new message arrives.
  \param task_id Task ID of the receiving task
  \param received_msg Pointer to the allocated message
 **/
void itti_receive_msg(task_id_t task_id, MessageDef **received_msg);

/** \brief Try to retrieves a message in the queue associated to task_id.
  \param task_id Task ID of the receiving task
  \param received_msg Pointer to the allocated message
 **/
void itti_poll_msg(task_id_t task_id, MessageDef **received_msg);

/** \brief Start thread associated to the task
   \param task_id task to start
   \param start_routine entry point for the task
   \param args_p Optional argument to pass to the start routine
   @returns -1 on failure, 0 otherwise
 **/
int itti_create_task(task_id_t task_id,
                     void *(*start_routine) (void *),
                     void *args_p);

/** \brief Exit the current task.
 **/
void itti_exit_task(void);

/** \brief Initiate termination of all tasks.
   \param task_id task that is completed
 **/
void itti_terminate_tasks(task_id_t task_id);

// Void for legacy compatibility
void itti_wait_ready(int wait_tasks);
void itti_mark_task_ready(task_id_t task_id);

/** \brief Return the printable string associated with the message
   \param message_id Id of the message
 **/
const char *itti_get_message_name(MessagesIds message_id);

/** \brief Return the printable string associated with a task id
   \param thread_id Id of the task
 **/
const char *itti_get_task_name(task_id_t task_id);

/** \brief Alloc and memset(0) a new itti message.
   \param origin_task_id Task ID of the sending task
   \param message_id Message ID
   @returns NULL in case of failure or newly allocated mesage ref
 **/
MessageDef *itti_alloc_new_message(
  task_id_t         origin_task_id,
  MessagesIds       message_id);

/** \brief Alloc and memset(0) a new itti message.
   \param origin_task_id Task ID of the sending task
   \param message_id Message ID
   \param size size of the payload to send
   @returns NULL in case of failure or newly allocated mesage ref
 **/
MessageDef *itti_alloc_new_message_sized(
  task_id_t         origin_task_id,
  MessagesIds       message_id,
  MessageHeaderSize size);

/** \brief handle signals and wait for all threads to join when the process complete.
   This function should be called from the main thread after having created all ITTI tasks.
 **/
void itti_wait_tasks_end(void);
#define  THREAD_MAX 0 //for compatibility
void itti_set_task_real_time(task_id_t task_id);

/** \brief Send a termination message to all tasks.
   \param task_id task that is broadcasting the message.
 **/
void itti_send_terminate_message(task_id_t task_id);

void *itti_malloc(task_id_t origin_task_id, task_id_t destination_task_id, ssize_t size);
void *calloc_or_fail(size_t size);
void *malloc_or_fail(size_t size);
int memory_read(const char *datafile, void *data, size_t size);
int itti_free(task_id_t task_id, void *ptr);

int itti_init(task_id_t task_max, thread_id_t thread_max, MessagesIds messages_id_max, const task_info_t *tasks_info,
              const message_info_t *messages_info);
int timer_setup(
  uint32_t      interval_sec,
  uint32_t      interval_us,
  task_id_t     task_id,
  int32_t       instance,
  timer_type_t  type,
  void         *timer_arg,
  long         *timer_id);


int timer_remove(long timer_id);
#define timer_stop timer_remove
int signal_handle(int *end);

#ifdef __cplusplus
}
#endif
#endif /* INTERTASK_INTERFACE_H_ */
