/**
* @file ori.h
* @brief Open Radio Interface (ORI) C library header file.
* @author AW2S (http://www.aw2s.com)
* @version 1.8
* @date January 18, 2022
*
* This file is the AW2S REC ORI C library header, and contains all necessary
* functions prototypes, enumerations, data structures and type definitions
* to implement the ORI shared library in an user application.
*/

#ifndef ORI_H_
#define ORI_H_

#define ORILIB_VERSION_MAJOR 1
#define ORILIB_VERSION_MINOR 8

#include <stdint.h>

/*-----------------------------------------------------------------------------------------
 * ORI_RESULT ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_Result_e
 * @brief ORI result codes enumeration.
 */
typedef enum
{
	ORI_Result_SUCCESS = 0,								/**< */
	ORI_Result_FAIL_SYNTAX_ERROR,						/**< */
	ORI_Result_FAIL_UNRECOGNIZED_MESSAGE,				/**< */
	ORI_Result_FAIL_RE_BUSY,							/**< */
	ORI_Result_FAIL_MISSING_PARAMETER,					/**< */
	ORI_Result_FAIL_PARAMETER_ERROR,					/**< */
	ORI_Result_FAIL_FRAME_ERROR,						/**< */
	ORI_Result_FAIL_INVALID_TIMEDATA,					/**< */
	ORI_Result_SUCCESS_SOFTWARE_ALREADY_EXISTING,		/**< */
	ORI_Result_FAIL_FTP_ERROR,							/**< */
	ORI_Result_FAIL_BROKEN_IMAGE,						/**< */
	ORI_Result_FAIL_NO_COMPATIBLE_IMAGE,				/**< */
	ORI_Result_FAIL_CANNOT_STORE,						/**< */
	ORI_Result_FAIL_NOSUCH_IMAGE,						/**< */
	ORI_Result_FAIL_UNKNOWN_OBJECT,						/**< */
	ORI_Result_FAIL_UNKNOWN_PARAM,						/**< */
	ORI_Result_FAIL_PARAMETER_FAIL,						/**< */
	ORI_Result_FAIL_NOSUCH_RESOURCE,					/**< */
	ORI_Result_FAIL_PARAM_READONLY,						/**< */
	ORI_Result_FAIL_PARAM_LOCKREQUIRED,					/**< */
	ORI_Result_FAIL_VALUE_OUTOF_RANGE,					/**< */
	ORI_Result_FAIL_VALUE_TYPE_ERROR,					/**< */
	ORI_Result_FAIL_UNKNOWN_OBJTYPE,					/**< */
	ORI_Result_FAIL_STATIC_OBJTYPE,						/**< */
	ORI_Result_FAIL_CHILD_NOTALLOWED,					/**< */
	ORI_Result_FAIL_OUTOF_RESOURCES,					/**< */
	ORI_Result_FAIL_LOCKREQUIRED,						/**< */
	ORI_Result_FAIL_UNKNOWN_STATETYPE,					/**< */
	ORI_Result_FAIL_UNKNOWN_STATEVALUE,					/**< */
	ORI_Result_FAIL_STATE_READONLY,						/**< */
	ORI_Result_FAIL_RESOURCE_UNAVAILABLE,				/**< */
	ORI_Result_FAIL_RESOURCE_INUSE,						/**< */
	ORI_Result_FAIL_PARENT_CHILD_CONFLICT,				/**< */
	ORI_Result_FAIL_PRECONDITION_NOTMET,				/**< */
	ORI_Result_FAIL_NOSUCH_FILE,						/**< */
	ORI_Result_FAIL_SIZE_LIMIT,							/**< */
	ORI_Result_FAIL_ACTIVATION_ERROR,					/**< */
	ORI_Result_FAIL_ALD_UNAVAILABLE,					/**< */
	ORI_Result_FAIL_BUS_UNAVAILABLE,					/**< */
	ORI_Result_OsError,									/**< */
	ORI_Result_BadParameter,							/**< */
	ORI_Result_InvalidIpAddress,						/**< */
	ORI_Result_ConnectionFailed,						/**< */
	ORI_Result_ConnectionTimedOut,						/**< */
	ORI_Result_NotConnected,							/**< */
	ORI_Result_Unknown,									/**< */
} ORI_Result_e;

/**
 * @brief Get a string describing an ::ORI_Result_e.
 * @param result The ::ORI_Result_e.
 * @return The describing string.
 */
const char * 					ORI_Result_Print(ORI_Result_e result);

/**
 * @brief Get an ::ORI_Result_e from a string.
 * @param resultString The string to match.
 * @return The matched ::ORI_Result_e, or ::ORI_Result_Unknown if failure.
 */
ORI_Result_e 					ORI_Result_Enum(const char * resultString);



/*-----------------------------------------------------------------------------------------
 * ORI_INDICATIONTYPE ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_IndicationType_e
 * @brief ORI indication type enumeration.
 */
typedef enum
{
	ORI_IndicationType_FileTransferComplete = 0,		/**< */
	ORI_IndicationType_ObjectStateChange,				/**< */
	ORI_IndicationType_FaultChange,						/**< */
	ORI_IndicationType_FileAvailable,					/**< */
	ORI_IndicationType_UploadFileCmpl,					/**< */
	ORI_IndicationType_AisgScanDeviceCompl,				/**< */
	ORI_IndicationType_AisgAldRx,						/**< */
	ORI_IndicationType_Unknown,							/**< */
} ORI_IndicationType_e;

/**
 * @brief Get a string describing an ::ORI_IndicationType_e.
 * @param indicationType The ::ORI_IndicationType_e.
 * @return The describing string.
 */
const char * 					ORI_IndicationType_Print(ORI_IndicationType_e indicationType);

/**
 * @brief Get an ::ORI_IndicationType_e from a string.
 * @param indicationTypeString The string to match.
 * @return The matched ::ORI_IndicationType_e, or ::ORI_IndicationType_Unknown if failure.
 */
ORI_IndicationType_e 			ORI_IndicationType_Enum(const char * indicationTypeString);



/*-----------------------------------------------------------------------------------------
 * ORI_OBJECTTYPE ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_ObjectType_e
 * @brief ORI object type enumeration.
 */
typedef enum
{
	ORI_ObjectType_RE = 0,								/**< */
	ORI_ObjectType_AntennaPort,							/**< */
	ORI_ObjectType_TxUtra,								/**< */
	ORI_ObjectType_TxEUtraFDD,							/**< */
	ORI_ObjectType_TxEUtraTDD,							/**< */
	ORI_ObjectType_TxGSM,								/**< */
	ORI_ObjectType_TxNRFDD,								/**< */
	ORI_ObjectType_TxNRTDD,								/**< */
	ORI_ObjectType_RxUtra,								/**< */
	ORI_ObjectType_RxEUtraFDD,							/**< */
	ORI_ObjectType_RxEUtraTDD,							/**< */
	ORI_ObjectType_RxGSM,								/**< */
	ORI_ObjectType_RxNRFDD,								/**< */
	ORI_ObjectType_RxNRTDD,								/**< */
	ORI_ObjectType_ORILink,								/**< */
	ORI_ObjectType_ExternalEventPort,					/**< */
	ORI_ObjectType_AISGPort,							/**< */
	ORI_ObjectType_AISGALD,								/**< */
	ORI_ObjectType_Log,									/**< */
	ORI_ObjectType_DLRoutedIQData,						/**< */
	ORI_ObjectType_ULRoutedIQData,						/**< */
	ORI_ObjectType_DLRoutedCWBlock,						/**< */
	ORI_ObjectType_ULRoutedCWBlock,						/**< */
	ORI_ObjectType_Invalid,								/**< */
} ORI_ObjectType_e;

/**
 * @brief Get a string describing an ::ORI_ObjectType_e.
 * @param objectType The ::ORI_ObjectType_e.
 * @return The describing string.
 */
const char * 					ORI_ObjectType_Print(ORI_ObjectType_e objectType);

/**
 * @brief Get an ::ORI_ObjectType_e from a string.
 * @param objectTypeString The string to match.
 * @return The matched ::ORI_ObjectType_e, or ::ORI_ObjectType_Invalid if failure.
 */
ORI_ObjectType_e 				ORI_ObjectType_Enum(const char * objectTypeString);



/*-----------------------------------------------------------------------------------------
 * ORI_OBJECTPARAM ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_ObjectParam_e
 * @brief ORI object parameter enumeration.
 *
 * All the parameters of all objects are in this list, but only once
 * (therefore some parameters will be used by several object types).
 */
typedef enum
{
	ORI_ObjectParam_RE_VendorID = 0,					/**< */
	ORI_ObjectParam_RE_ProductID,						/**< */
	ORI_ObjectParam_RE_ProductRevision,					/**< */
	ORI_ObjectParam_RE_SerialNumber,					/**< */
	ORI_ObjectParam_RE_ProtocolVer,						/**< */
	ORI_ObjectParam_RE_AGCTargetLevCfgGran,				/**< */
	ORI_ObjectParam_RE_AGCSettlingTimeCfgGran,			/**< */
	ORI_ObjectParam_RE_AGCSettlingTimeCap,				/**< */
	ORI_ObjectParam_RE_AWS_uptime,						/**< */
	ORI_ObjectParam_RE_AWS_inputVoltage,				/**< */
	ORI_ObjectParam_RE_AWS_inputCurrent,				/**< */
	ORI_ObjectParam_RE_AWS_productTemp,					/**< */
	ORI_ObjectParam_RE_AWS_cpuTemp,						/**< */
	ORI_ObjectParam_RE_AWS_paTemp,						/**< */
	ORI_ObjectParam_RE_AWS_rxPwrOffset,					/**< */
	ORI_ObjectParam_Port_portLabel,						/**< */
	ORI_ObjectParam_Port_AWS_outputPwr,					/**< */
	ORI_ObjectParam_Port_AWS_inputPwr,					/**< */
	ORI_ObjectParam_Port_AWS_returnLoss,				/**< */
	ORI_ObjectParam_SigPath_axcW,						/**< */
	ORI_ObjectParam_SigPath_axcB,						/**< */
	ORI_ObjectParam_SigPath_oriLink,					/**< */
	ORI_ObjectParam_SigPath_uarfcn,						/**< */
	ORI_ObjectParam_SigPath_antPort,					/**< */
	ORI_ObjectParam_SigPath_chanBW,						/**< */
	ORI_ObjectParam_SigPath_tddSpecialSFConfig,			/**< */
	ORI_ObjectParam_SigPath_tddULDLConfig,				/**< */
	ORI_ObjectParam_SigPath_earfcn,						/**< */
	ORI_ObjectParam_SigPath_freqBandInd,				/**< */
	ORI_ObjectParam_SigPath_sigmaIQ,					/**< */
	ORI_ObjectParam_SigPath_AWS_enableCompRateChange,	/**< */
	ORI_ObjectParam_SigPath_AWS_enableCompBitChange,	/**< */
	ORI_ObjectParam_SigPath_AWS_measuredPwr,			/**< */
	ORI_ObjectParam_SigPath_AWS_axcIncr,				/**< */
	ORI_ObjectParam_SigPath_AWS_arfcn,					/**< */
	ORI_ObjectParam_TxSigPath_t2a,						/**< */
	ORI_ObjectParam_TxSigPath_maxTxPwr,					/**< */
	ORI_ObjectParam_TxSigPath_dlCalRE,					/**< */
	ORI_ObjectParam_TxSigPath_tddCPLengthDL,			/**< */
	ORI_ObjectParam_TxSigPath_dlCalREMax,				/**< */
	ORI_ObjectParam_TxSigPath_enableIQDLComp,			/**< */
	ORI_ObjectParam_TxSigPath_AWS_enPeakCancel,			/**< */
	ORI_ObjectParam_RxSigPath_ulCalREMax,				/**< */
	ORI_ObjectParam_RxSigPath_ta3,						/**< */
	ORI_ObjectParam_RxSigPath_ulCalRE,					/**< */
	ORI_ObjectParam_RxSigPath_rtwpGroup,				/**< */
	ORI_ObjectParam_RxSigPath_tddCPLengthUL,			/**< */
	ORI_ObjectParam_RxSigPath_ulFeedAdj,				/**< */
	ORI_ObjectParam_RxSigPath_ulTgtRMSLvl,				/**< */
	ORI_ObjectParam_RxSigPath_ulAGCSetlgTime,			/**< */
	ORI_ObjectParam_RxSigPath_TxSigPath,				/**< */
	ORI_ObjectParam_RxSigPath_enableIQULComp,			/**< */
	ORI_ObjectParam_ORILink_portRoleCapability,			/**< */
	ORI_ObjectParam_ORILink_portRole,					/**< */
	ORI_ObjectParam_ORILink_bitRateSupport,				/**< */
	ORI_ObjectParam_ORILink_bitRateRequested,			/**< */
	ORI_ObjectParam_ORILink_bitRateOperational,			/**< */
	ORI_ObjectParam_ORILink_localPortID,				/**< */
	ORI_ObjectParam_ORILink_remotePortID,				/**< */
	ORI_ObjectParam_ORILink_toffset,					/**< */
	ORI_ObjectParam_ORILink_oriLinkType,				/**< */
	ORI_ObjectParam_ORILink_AWS_localMAC,				/**< */
	ORI_ObjectParam_ORILink_AWS_remoteMAC,				/**< */
	ORI_ObjectParam_ORILink_AWS_t14,					/**< */
	ORI_ObjectParam_ORILink_AWS_sfpTxPow,				/**< */
	ORI_ObjectParam_ORILink_AWS_sfpRxPow,				/**< */
	ORI_ObjectParam_ORILink_AWS_remoteIP,				/**< */
	ORI_ObjectParam_ORILink_AWS_localIP,				/**< */
	ORI_ObjectParam_ORILink_AWS_remoteUdpPort,			/**< */
	ORI_ObjectParam_ORILink_AWS_localUdpPort,			/**< */
	ORI_ObjectParam_AISGPort_busPowerEnable,			/**< */
	ORI_ObjectParam_AISGALD_deviceType,					/**< */
	ORI_ObjectParam_AISGALD_UID,						/**< */
	ORI_ObjectParam_AISGALD_releaseID,					/**< */
	ORI_ObjectParam_AISGALD_aisgVersion,				/**< */
	ORI_ObjectParam_AISGALD_deviceTypeVersion,			/**< */
	ORI_ObjectParam_AISGALD_frameLength,				/**< */
	ORI_ObjectParam_AISGALD_hdlcAdress,					/**< */
	ORI_ObjectParam_Log_logTypeID,						/**< */
	ORI_ObjectParam_Log_description,					/**< */
	ORI_ObjectParam_Log_logCategory,					/**< */
	ORI_ObjectParam_Log_maxREfileSize,					/**< */
	ORI_ObjectParam_Log_maxRECfileSize,					/**< */
	ORI_ObjectParam_Log_enableNotification,				/**< */
	ORI_ObjectParam_Log_fileAvailable,					/**< */
	ORI_ObjectParam_Log_overflowBehaviour,				/**< */
	ORI_ObjectParam_Log_recordingEnabled,				/**< */
	ORI_ObjectParam_Log_logPeriod,						/**< */
	ORI_ObjectParam_Log_timerType,						/**< */
	ORI_ObjectParam_Routed_MasterPortOriLink,			/**< */
	ORI_ObjectParam_Routed_SlavePortOriLink,			/**< */
	ORI_ObjectParam_RoutedIQData_IQsubBlockSize,		/**< */
	ORI_ObjectParam_RoutedIQData_MasterPortIQblkW,		/**< */
	ORI_ObjectParam_RoutedIQData_MasterPortIQblkB,		/**< */
	ORI_ObjectParam_RoutedIQData_SlavePortIQW,			/**< */
	ORI_ObjectParam_RoutedIQData_SlavePortIQB,			/**< */
	ORI_ObjectParam_RoutedIQData_TBDelayDL,				/**< */
	ORI_ObjectParam_RoutedIQData_TBDelayUL,				/**< */
	ORI_ObjectParam_RoutedCWBlock_CtrlBlockSize,		/**< */
	ORI_ObjectParam_RoutedCWBlock_SubChannelStart,		/**< */
	ORI_ObjectParam_RoutedCWBlock_Ydepth,				/**< */
	ORI_ObjectParam_RoutedCWBlock_SlavePortYoffset,		/**< */
	ORI_ObjectParam_RoutedCWBlock_MasterPortYoffset,	/**< */
	ORI_ObjectParam_All,								/**< */
	ORI_ObjectParam_Invalid,							/**< */
} ORI_ObjectParam_e;

/**
 * @brief Get a string describing an ::ORI_ObjectParam_e.
 * @param objectParam The ::ORI_ObjectParam_e.
 * @return The describing string.
 */
const char * 					ORI_ObjectParam_Print(ORI_ObjectParam_e objectParam);

/**
 * @brief Get an ::ORI_ObjectParam_e from a string.
 * @param objectParamString The string to match.
 * @return The matched ::ORI_ObjectParam_e, or ::ORI_ObjectParam_Invalid if failure.
 */
ORI_ObjectParam_e 				ORI_ObjectParam_Enum(const char * objectParamString);



/*-----------------------------------------------------------------------------------------
 * ORI_STATETYPE ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_StateType_e
 * @brief ORI state type enumeration.
 */
typedef enum
{
	ORI_StateType_AST = 0,								/**< */
	ORI_StateType_FST,									/**< */
	ORI_StateType_All,									/**< */
	ORI_StateType_Invalid,								/**< */
} ORI_StateType_e;

/**
 * @brief Get a string describing an ::ORI_StateType_e.
 * @param stateType The ::ORI_StateType_e.
 * @return The describing string.
 */
const char * 					ORI_StateType_Print(ORI_StateType_e stateType);

/**
 * @brief Get an ::ORI_StateType_e from a string.
 * @param stateTypeString The string to match.
 * @return The matched ::ORI_StateType_e, or ::ORI_StateType_Invalid if failure.
 */
ORI_StateType_e 				ORI_StateType_Enum(const char * stateTypeString);



/*-----------------------------------------------------------------------------------------
 * ORI_AST ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_AST_e
 * @brief ORI AST enumeration.
 */
typedef enum
{
	ORI_AST_Locked = 0,									/**< */
	ORI_AST_Unlocked,									/**< */
	ORI_AST_Invalid,									/**< */
} ORI_AST_e;

/**
 * @brief Get a string describing an ::ORI_AST_e.
 * @param ast The ::ORI_AST_e.
 * @return The describing string.
 */
const char * 					ORI_AST_Print(ORI_AST_e ast);

/**
 * @brief Get an ::ORI_AST_e from a string.
 * @param astString The string to match.
 * @return The matched ::ORI_AST_e, or ::ORI_AST_Invalid if failure.
 */
ORI_AST_e 						ORI_AST_Enum(const char * astString);



/*-----------------------------------------------------------------------------------------
 * ORI_FST ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_FST_e
 * @brief ORI FST enumeration.
 */
typedef enum
{
	ORI_FST_PreOperational = 0,							/**< */
	ORI_FST_Operational,								/**< */
	ORI_FST_Degraded,									/**< */
	ORI_FST_Failed,										/**< */
	ORI_FST_NotOperational,								/**< */
	ORI_FST_Disabled,									/**< */
	ORI_FST_Invalid,									/**< */
} ORI_FST_e;

/**
 * @brief Get a string describing an ::ORI_FST_e.
 * @param fst The ::ORI_FST_e.
 * @return The describing string.
 */
const char * 					ORI_FST_Print(ORI_FST_e fst);

/**
 * @brief Get an ::ORI_FST_e from a string.
 * @param fstString The string to match.
 * @return The matched ::ORI_FST_e, or ::ORI_FST_Invalid if failure.
 */
ORI_FST_e 						ORI_FST_Enum(const char * fstString);



/*-----------------------------------------------------------------------------------------
 * ORI_AGCGRANULARITY ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_agcGranularity_e
 * @brief AGC granularity capability enumeration.
 */
typedef enum
{
	ORI_agcGranularity_RxSigPath = 0,					/**< */
	ORI_agcGranularity_RE,								/**< */
	ORI_agcGranularity_Invalid,							/**< */
} ORI_agcGranularity_e;

/**
 * @brief Get a string describing an ::ORI_agcGranularity_e.
 * @param agcGranularity The ::ORI_agcGranularity_e.
 * @return The describing string.
 */
const char * 					ORI_agcGranularity_Print(ORI_agcGranularity_e agcGranularity);

/**
 * @brief Get an ::ORI_agcGranularity_e from a string.
 * @param agcGranularityString The string to match.
 * @return The matched ::ORI_agcGranularity_e, or ::ORI_agcGranularity_Invalid if failure.
 */
ORI_agcGranularity_e 			ORI_agcGranularity_Enum(const char * agcGranularityString);



/*-----------------------------------------------------------------------------------------
 * ORI_TDDCPLENGTH ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_tddCPLength_e
 * @brief TDD cyclic prefix length enumeration.
 */
typedef enum
{
	ORI_tddCPLength_Normal = 0,							/**< */
	ORI_tddCPLength_Extended,							/**< */
	ORI_tddCPLength_Invalid,							/**< */
} ORI_tddCPLength_e;

/**
 * @brief Get a string describing an ::ORI_tddCPLength_e.
 * @param tddCPLength The ::ORI_tddCPLength_e.
 * @return The describing string.
 */
const char * 					ORI_tddCPLength_Print(ORI_tddCPLength_e tddCPLength);

/**
 * @brief Get a ::ORI_tddCPLength_e from a string.
 * @param tddCPLengthString The string to match.
 * @return The matched ::ORI_tddCPLength_e, or ::ORI_tddCPLength_Invalid if failure.
 */
ORI_tddCPLength_e 				ORI_tddCPLength_Enum(const char * tddCPLengthString);



/*-----------------------------------------------------------------------------------------
 * ORI_FREQBAND ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_freqBand_e
 * @brief GSM frequency band enumeration.
 */
typedef enum
{
	ORI_freqBand_GSM850 = 0,							/**< */
	ORI_freqBand_PGSM900,								/**< */
	ORI_freqBand_DCS1800,								/**< */
	ORI_freqBand_PCS1900,								/**< */
	ORI_freqBand_Invalid,								/**< */
} ORI_freqBand_e;

/**
 * @brief Get a string describing an ::ORI_freqBand_e.
 * @param freqBand The ::ORI_freqBand_e.
 * @return The describing string.
 */
const char * 					ORI_freqBand_Print(ORI_freqBand_e freqBand);

/**
 * @brief Get a ::ORI_freqBand_e from a string.
 * @param freqBandString The string to match.
 * @return The matched ::ORI_freqBand_e, or ::ORI_freqBand_Invalid if failure.
 */
ORI_freqBand_e 					ORI_freqBand_Enum(const char * freqBandString);



/*-----------------------------------------------------------------------------------------
 * ORI_PORTROLE ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_portRole_e
 * @brief ORI link port role enumeration.
 */
typedef enum
{
	ORI_portRole_Master = 0,							/**< */
	ORI_portRole_Slave,									/**< */
	ORI_portRole_Invalid,								/**< */
} ORI_portRole_e;

/**
 * @brief Get a string describing an ::ORI_portRole_e.
 * @param portRole The ::ORI_portRole_e.
 * @return The describing string.
 */
const char * 					ORI_portRole_Print(ORI_portRole_e portRole);

/**
 * @brief Get a ::ORI_portRole_e from a string.
 * @param portRoleString The string to match.
 * @return The matched ::ORI_portRole_e, or ::ORI_portRole_Invalid if failure.
 */
ORI_portRole_e 					ORI_portRole_Enum(const char * portRoleString);



/*-----------------------------------------------------------------------------------------
 * ORI_PORTROLECAPABILITY ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_portRoleCapability_e
 * @brief ORI Link port role capability enumeration.
 */
typedef enum
{
	ORI_portRoleCapability_MO = 0,						/**< */
	ORI_portRoleCapability_SO,							/**< */
	ORI_portRoleCapability_MS,							/**< */
	ORI_portRoleCapability_Invalid,						/**< */
} ORI_portRoleCapability_e;

/**
 * @brief Get a string describing an ::ORI_portRoleCapability_e.
 * @param portRoleCapability The ::ORI_portRoleCapability_e.
 * @return The describing string.
 */
const char * 					ORI_portRoleCapability_Print(ORI_portRoleCapability_e portRoleCapability);

/**
 * @brief Get an ::ORI_portRoleCapability_e from a string.
 * @param portRoleCapabilityString The string to match.
 * @return The matched ::ORI_portRoleCapability_e, or ::ORI_portRoleCapability_Invalid if failure.
 */
ORI_portRoleCapability_e 		ORI_portRoleCapability_Enum(const char * portRoleCapabilityString);



/*-----------------------------------------------------------------------------------------
 * ORI_LINKTYPE ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_linkType_e
 * @brief ORI Link type enumeration.
 */
typedef enum
{
	ORI_linkType_Active = 0,							/**< */
	ORI_linkType_Passive,								/**< */
	ORI_linkType_Invalid,								/**< */
} ORI_linkType_e;


/**
 * @brief Get a string describing an ::ORI_linkType_e.
 * @param linkType The ::ORI_linkType_e.
 * @return The describing string.
 */
const char * 					ORI_linkType_Print(ORI_linkType_e linkType);

/**
 * @brief Get an ::ORI_linkType_e from a string.
 * @param linkTypeString The string to match.
 * @return The matched ::ORI_linkType_e, or ::ORI_linkType_Invalid if failure.
 */
ORI_linkType_e 					ORI_linkType_Enum(const char * linkTypeString);



/*-----------------------------------------------------------------------------------------
 * ORI_BOOLEAN ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_Boolean_e
 * @brief ORI boolean enumeration.
 */
typedef enum
{
	ORI_Boolean_False = 0,								/**< */
	ORI_Boolean_True,									/**< */
	ORI_Boolean_Invalid,								/**< */
} ORI_Boolean_e;

/**
 * @brief Get a string describing an ::ORI_Boolean_e.
 * @param boolean The ::ORI_Boolean_e.
 * @return The describing string.
 */
const char * 					ORI_Boolean_Print(ORI_Boolean_e boolean);

/**
 * @brief Get an ::ORI_Boolean_e from a string.
 * @param booleanString The string to match.
 * @return The matched ::ORI_Boolean_e, or ::ORI_Boolean_Invalid if failure.
 */
ORI_Boolean_e 					ORI_Boolean_Enum(const char * booleanString);



/*-----------------------------------------------------------------------------------------
 * ORI_LOGCATEGORY ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_logCategory_e
 * @brief Log category enumeration.
 *
 */
typedef enum
{
	ORI_logCategory_Fault = 0,							/**< */
	ORI_logCategory_Perf,								/**< */
	ORI_logCategory_System,								/**< */
	ORI_logCategory_Crash,								/**< */
	ORI_logCategory_Application,						/**< */
	ORI_logCategory_Debug,								/**< */
	ORI_logCategory_Other,								/**< */
	ORI_logCategory_Invalid,							/**< */
} ORI_logCategory_e;

/**
 * @brief Get a string describing an ::ORI_logCategory_e.
 * @param logCategory The ::ORI_logCategory_e.
 * @return The describing string.
 */
const char * 					ORI_logCategory_Print(ORI_logCategory_e logCategory);

/**
 * @brief Get an ::ORI_logCategory_e from a string.
 * @param logCategoryString The string to match.
 * @return The matched ::ORI_logCategory_e, or ::ORI_logCategory_Invalid if failure.
 */
ORI_logCategory_e 				ORI_logCategory_Enum(const char * logCategoryString);



/*-----------------------------------------------------------------------------------------
 * ORI_OVERFLOWBEHAVIOUR ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_overflowBehaviour_e
 * @brief Log overflow behaviour enumeration.
 *
 */
typedef enum
{
	ORI_overflowBehaviour_Stop = 0,						/**< */
	ORI_overflowBehaviour_Fifo,							/**< */
	ORI_overflowBehaviour_Invalid,						/**< */
} ORI_overflowBehaviour_e;

/**
 * @brief Get a string describing an ::ORI_overflowBehaviour_e.
 * @param overflowBehaviour The ::ORI_overflowBehaviour_e.
 * @return The describing string.
 */
const char * 					ORI_overflowBehaviour_Print(ORI_overflowBehaviour_e overflowBehaviour);

/**
 * @brief Get an ::ORI_overflowBehaviour_e from a string.
 * @param overflowBehaviourString The string to match.
 * @return The matched ::ORI_overflowBehaviour_e, or ::ORI_overflowBehaviour_Invalid if failure.
 */
ORI_overflowBehaviour_e 		ORI_overflowBehaviour_Enum(const char * overflowBehaviourString);



/*-----------------------------------------------------------------------------------------
 * ORI_TIMERTYPE ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_timerType_e
 * @brief Log timer type enumeration.
 */
typedef enum
{
	ORI_timerType_OneShot = 0,							/**< */
	ORI_timerType_Periodic,								/**< */
	ORI_timerType_Invalid,								/**< */
} ORI_timerType_e;

/**
 * @brief Get a string describing an ::ORI_timerType_e.
 * @param timerType The ::ORI_timerType_e.
 * @return The describing string.
 */
const char * 					ORI_timerType_Print(ORI_timerType_e timerType);

/**
 * @brief Get an ::ORI_timerType_e from a string.
 * @param timerTypeString The string to match.
 * @return The matched ::ORI_timerType_e, or ::ORI_timerType_Invalid if failure.
 */
ORI_timerType_e 				ORI_timerType_Enum(const char * timerTypeString);



/*-----------------------------------------------------------------------------------------
 *	ORI_FAULTTYPE ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_FaultType_e
 * @brief ORI fault types enumeration.
 */
typedef enum
{
	ORI_FaultType_RE_ExtSupplyUnderVolt = 0,			/**< */
	ORI_FaultType_RE_OverTemp,							/**< */
	ORI_FaultType_RE_DigInOverdrive,					/**< */
	ORI_FaultType_RE_RFOutOverdrive,					/**< */
	ORI_FaultType_RE_TXGainFail,						/**< */
	ORI_FaultType_RE_RXGainFail,						/**< */
	ORI_FaultType_AntennaPort_VSWROutOfRange,			/**< */
	ORI_FaultType_AntennaPort_NonAisgTmaMalfct,			/**< */
	ORI_FaultType_ORILink_LinkFail,						/**< */
	ORI_FaultType_ORILink_PortFail,						/**< */
	ORI_FaultType_ORILink_SyncFail,						/**< */
	ORI_FaultType_AISGPort_AisgMalfct,					/**< */
	ORI_FaultType_Invalid,
} ORI_FaultType_e;

/**
 * @brief Get a string describing an ::ORI_FaultType_e.
 * @param faultType The ::ORI_FaultType_e.
 * @return The describing string.
 */
const char * 					ORI_FaultType_Print(ORI_FaultType_e faultType);

/**
 * @brief Get an ::ORI_FaultType_e from a string.
 * @param faultTypeString The string to match.
 * @return The matched ::ORI_FaultType_e, or ::ORI_FaultType_Invalid if failure.
 */
ORI_FaultType_e 				ORI_FaultType_Enum(const char * faultTypeString);



/*-----------------------------------------------------------------------------------------
 * ORI_FAULTSTATE ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_FaultState_e
 * @brief ORI RE fault state enumeration.
 */
typedef enum
{
    ORI_FaultState_Cleared = 0,							/**< */
    ORI_FaultState_Active,								/**< */
    ORI_FaultState_Unknown,								/**< */
} ORI_FaultState_e;

/**
 * @brief Get a string describing an ::ORI_FaultState_e.
 * @param faultState The ::ORI_FaultState_e.
 * @return The describing string.
 */
const char * 					ORI_FaultState_Print(ORI_FaultState_e faultState);

/**
 * @brief Get an ::ORI_FaultState_e from a string.
 * @param faultStateString The string to match.
 * @return The matched ::ORI_FaultState_e, or ::ORI_FaultState_Unknown if failure.
 */
ORI_FaultState_e 				ORI_FaultState_Enum(const char * faultStateString);



/*-----------------------------------------------------------------------------------------
 * ORI_FAULTSEVERITY ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_FaultSeverity_e
 * @brief ORI RE fault severity enumeration.
 */
typedef enum
{
	ORI_FaultSeverity_Failed = 0,						/**< */
	ORI_FaultSeverity_Degraded,							/**< */
	ORI_FaultSeverity_Warning,							/**< */
	ORI_FaultSeverity_Unknown,							/**< */
} ORI_FaultSeverity_e;

/**
 * @brief Get a string describing an ::ORI_FaultSeverity_e.
 * @param faultSeverity The ::ORI_FaultSeverity_e.
 * @return The describing string.
 */
const char * 					ORI_FaultSeverity_Print(ORI_FaultSeverity_e faultSeverity);

/**
 * @brief Get an ::ORI_FaultSeverity_e from a string.
 * @param faultSeverityString The string to match.
 * @return The matched ::ORI_FaultSeverity_e, or ::ORI_FaultSeverity_Unknown if failure.
 */
ORI_FaultSeverity_e 			ORI_FaultSeverity_Enum(const char * faultSeverityString);



/*-----------------------------------------------------------------------------------------
 * ORI_EVENTDRIVENREPORT ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_EventDrivenReport_e
 * @brief Event driven reporting enumeration.
 */
typedef enum
{
	ORI_EventDrivenReport_NoModify = 0,					/**< */
	ORI_EventDrivenReport_True,							/**< */
	ORI_EventDrivenReport_False,						/**< */
	ORI_EventDrivenReport_Invalid,						/**< */
} ORI_EventDrivenReport_e;



/*-----------------------------------------------------------------------------------------
 * ORI_AISGLAYER7COMMAND ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_AisgLayer7Command_e
 * @brief Aisg Layer7 commands byte code enumeration.
 */
typedef enum
{
	ORI_AisgLayer7Command_Unknown 					= 0,			/**< */
	/* Common procedures */
	ORI_AisgLayer7Command_Reset						= 0x03,			/**< */
	ORI_AisgLayer7Command_GetAlarmStatus			= 0x04,			/**< */
	ORI_AisgLayer7Command_GetInfo 					= 0x05,			/**< */
	ORI_AisgLayer7Command_ClearActiveAlarms			= 0x06,			/**< */
	ORI_AisgLayer7Command_SelfTest					= 0x0A,			/**< */
	ORI_AisgLayer7Command_ReadUserData				= 0x10,			/**< */
	ORI_AisgLayer7Command_WriteUserData				= 0x11,			/**< */
	ORI_AisgLayer7Command_AlarmSubscribe			= 0x12,			/**< */
	ORI_AisgLayer7Command_DownloadStart				= 0x40,			/**< */
	ORI_AisgLayer7Command_DownloadApplication		= 0x41,			/**< */
	ORI_AisgLayer7Command_DownloadEnd				= 0x42,			/**< */
	/* Single Antenna RET procedures */
	ORI_AisgLayer7Command_RetSingle_AlarmIndication	= 0x07,			/**< */
	ORI_AisgLayer7Command_RetSingle_SetDeviceData	= 0x0E,			/**< */
	ORI_AisgLayer7Command_RetSingle_GetDeviceData	= 0x0F,			/**< */
	ORI_AisgLayer7Command_RetSingle_Calibrate 		= 0x31,			/**< */
	ORI_AisgLayer7Command_RetSingle_SendConfigData	= 0x32,			/**< */
	ORI_AisgLayer7Command_RetSingle_SetTilt 		= 0x33,			/**< */
	ORI_AisgLayer7Command_RetSingle_GetTilt 		= 0x34,			/**< */
	/* Multi Antenna RET procedures */
	ORI_AisgLayer7Command_RetMulti_Calibrate 		= 0x80,			/**< */
	ORI_AisgLayer7Command_RetMulti_SetTilt 			= 0x81,			/**< */
	ORI_AisgLayer7Command_RetMulti_GetTilt 			= 0x82,			/**< */
	ORI_AisgLayer7Command_RetMulti_SetDeviceData	= 0x83,			/**< */
	ORI_AisgLayer7Command_RetMulti_GetDeviceData	= 0x84,			/**< */
	ORI_AisgLayer7Command_RetMulti_AlarmIndication	= 0x85,			/**< */
	ORI_AisgLayer7Command_RetMulti_ClearActiveAlarms= 0x86,			/**< */
	ORI_AisgLayer7Command_RetMulti_GetAlarmStatus	= 0x87,			/**< */
	ORI_AisgLayer7Command_RetMulti_GetNbrAntenna	= 0x88,			/**< */
	ORI_AisgLayer7Command_RetMulti_SendConfigData	= 0x89,			/**< */
	/* added specific command to check the existence of the aisg port from the model (this is not a real aisg command!) */
	ORI_AisgLayer7Command_CheckAisgPortExist		= 0xFE,			/**< */
	/* added specific command to get the aisg ALD device type from the model (this is not a real aisg command!) */
	ORI_AisgLayer7Command_GetDeviceType				= 0xFF,			/**< */
} ORI_AisgLayer7Command_e;


/*-----------------------------------------------------------------------------------------
 * ORI_AISGRETURNCODE ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_AisgReturnCode_e
 * @brief Aisg return code enumeration.
 */
typedef enum
{
	ORI_AisgReturnCode_OK 					= 0,			/**< */
	ORI_AisgReturnCode_MotorJam				= 0x02,			/**< */
	ORI_AisgReturnCode_ActuatorJam			= 0x03,			/**< */
	ORI_AisgReturnCode_Busy 				= 0x05,			/**< */
	ORI_AisgReturnCode_Checksum 			= 0x06,			/**< */
	ORI_AisgReturnCode_FAIL 				= 0x0B,			/**< */
	ORI_AisgReturnCode_NotCalibrated		= 0x0E,			/**< */
	ORI_AisgReturnCode_NotConfigured		= 0x0F,			/**< */
	ORI_AisgReturnCode_Hardware				= 0x11,			/**< */
	ORI_AisgReturnCode_OutOfRange			= 0x13,			/**< */
	ORI_AisgReturnCode_UnknownProcedure		= 0x19,			/**< */
	ORI_AisgReturnCode_ReadOnly 			= 0x1D,			/**< */
	ORI_AisgReturnCode_UnknownParameter		= 0x1E,			/**< */
	ORI_AisgReturnCode_WorkingSWMissing		= 0x21,			/**< */
	ORI_AisgReturnCode_InvalidFileContent	= 0x22,			/**< */
	ORI_AisgReturnCode_FormatError 			= 0x24,			/**< */
	ORI_AisgReturnCode_UnsupportedProcedure	= 0x25,			/**< */
	ORI_AisgReturnCode_InvalidProcedureSeq	= 0x26,			/**< */
	ORI_AisgReturnCode_ActuatorInterference	= 0x27,			/**< */
	ORI_AisgReturnCode_Unknown,
} ORI_AisgReturnCode_e;

/**
 * @brief Get a string describing an ::ORI_AisgReturnCode_e.
 * @param returnCode The ::ORI_AisgReturnCode_e.
 * @return The describing string.
 */
const char * 					ORI_AisgReturnCode_Print(ORI_AisgReturnCode_e returnCode);

/**
 * @brief Get an ::ORI_AisgReturnCode_e from a string.
 * @param returnCodeString The string to match.
 * @return The matched ::ORI_AisgReturnCode_e, or ::ORI_AisgReturnCode_Unknown if failure.
 */
ORI_AisgReturnCode_e 			ORI_AisgReturnCode_Enum(const char * returnCodeString);


/*-----------------------------------------------------------------------------------------
 * ORI_AISGDEVICEDATA ENUMERATION
 *-----------------------------------------------------------------------------------------*/

/**
 * @enum ORI_AisgDeviceData_e
 * @brief Aisg device data fields enumeration.
 */
typedef enum
{
	ORI_AisgDeviceData_AntennaModelNbr		= 1,			/**< */
	ORI_AisgDeviceData_AntennaSerialNbr		= 2,			/**< */
	ORI_AisgDeviceData_AntennaFreqBand		= 3,			/**< */
	ORI_AisgDeviceData_BeamwidthBand		= 4,			/**< */
	ORI_AisgDeviceData_GainBand				= 5,			/**< */
	ORI_AisgDeviceData_MaxTilt				= 6,			/**< */
	ORI_AisgDeviceData_MinTilt				= 7,			/**< */
	ORI_AisgDeviceData_InstallationDate		= 0x21,			/**< */
	ORI_AisgDeviceData_InstallerID			= 0x22,			/**< */
	ORI_AisgDeviceData_BasestationID		= 0x23,			/**< */
	ORI_AisgDeviceData_SectorID				= 0x24,			/**< */
	ORI_AisgDeviceData_AntennaBearing		= 0x25,			/**< */
	ORI_AisgDeviceData_MechanicalTilt		= 0x26,			/**< */
	ORI_AisgDeviceData_Unknown,
} ORI_AisgDeviceData_e;

/**
 * @brief Get a string describing an ::ORI_AisgDeviceData_e.
 * @param field The ::ORI_AisgDeviceData_e.
 * @return The describing string.
 */
const char * 					ORI_AisgDeviceDataField_Print(ORI_AisgDeviceData_e field);

/**
 * @brief Get an ::ORI_AisgDeviceData_e from a string.
 * @param fieldString The string to match.
 * @return The matched ::ORI_AisgDeviceData_e, or ::ORI_AisgDeviceData_Unknown if failure.
 */
ORI_AisgDeviceData_e 			ORI_AisgDeviceDataField_Enum(const char * fieldString);



/*-----------------------------------------------------------------------------------------
 * OBJECT TYPE REFERENCE
 *-----------------------------------------------------------------------------------------*/

/* Forward declaration of the ORI_Object_s structure. */
typedef struct ORI_Object_s ORI_Object_s;

/**
 * @struct ORI_ObjectTypeRef_s
 * @brief Object type reference structure.
 *
 * This structure represents an ORI ObjectTypeRef, which does not fully describe an ORI object
 * because it is not associated to an instance number.
 */
 typedef struct
{
	ORI_Object_s *		parent;							/**< Parent object reference, may be NULL if no parent. */
	ORI_ObjectType_e	type;							/**< Object type. */
} ORI_ObjectTypeRef_s;





/*-----------------------------------------------------------------------------------------
 * OBJECT PARAMETERS
 *-----------------------------------------------------------------------------------------*/

/**
 * @struct ORI_ObjectParams_RE_s
 * @brief Structure containing the parameters of a RE object.
 */
typedef struct
{
	char 					vendorID[4];				/**< RO. Vendor ID as signaled in DHCP code 201, normally 3 characters */
	char 					productID[81];				/**< RO. RE product ID.*/
	char 					productRev[81];				/**< RO. RE product revision. */
	char 					serialNumber[41];			/**< RO. RE serial number. */
	char 					protocolVer[11];			/**< RO. OCP protocol supported. */
	ORI_agcGranularity_e	agcTargetLevCfgGran;		/**< RO. UL AGC target RMS level config granularity. */
	ORI_agcGranularity_e 	agcSettlTimeCfgGran;		/**< RO. UL AGC settling time config granularity. */
	uint16_t				agcSettlTimeCap;			/**< RO. UL AGC settling time capability. */
	uint32_t				AWS_uptime;					/**< RO. AW2S Vendor specific: Uptime since boot in seconds. */
	int32_t					AWS_inputVoltage;			/**< RO. AW2S Vendor specific: RE input voltage in mV. */
	int32_t					AWS_inputCurrent;			/**< RO. AW2S Vendor specific: RE input current in mA. */
	int16_t					AWS_productTemp;			/**< RO. AW2S Vendor specific: RE temperature in degC. */
	int16_t					AWS_cpuTemp;				/**< RO. AW2S Vendor specific: CPU temperature in degC. */
	int16_t					AWS_paTemp;					/**< RO. AW2S Vendor specific: Power amplifier temperature in degC. */
	int16_t					AWS_rxPwrOffset;			/**< RO. AW2S Vendor specific: Receiver dBFS to dBm power conversion offset, unit is dB/10 (e.g. -340 for -34 dB). */
} ORI_ObjectParams_RE_s;

/**
 * @struct ORI_ObjectParams_AntennaPort_s
 * @brief Structure containing the parameters of an Antenna Port object.
 */
typedef struct
{
	char 		portLabel[81];							/**< RO. Physical antenna port label.*/
	int16_t 	AWS_outputPwr;							/**< RO. AW2S Vendor specific: Measured output power for this antenna, unit is dBm/10 (e.g. 400 for 40 dBm). */
	int16_t 	AWS_inputPwr;							/**< RO. AW2S Vendor specific: Measured input power for this antenna, unit is dBm/10 (e.g. -650 for -65 dBm). */
	int16_t 	AWS_returnLoss;							/**< RO. AW2S Vendor specific: Measured return loss for this antenna, unit is dB/10 (e.g. 50 for 5 dB). */
} ORI_ObjectParams_AntennaPort_s;

/**
 * @struct ORI_ObjectParams_TxSigPathUtra_s
 * @brief Structure containing the parameters of a TxSigPath UTRAFDD object.
 */
typedef struct
{
	uint16_t			dlCalREMax;						/**< RO. Max possible buffer in RE for DL timing calibration in Tc/16.*/
	uint32_t			t2a;							/**< RO. RE time delay. */
	uint16_t			dlCalRE;						/**< RW-Locked. Time delay to enable DL timing calibration in Tc/16. */
	int16_t 			maxTxPwr;						/**< RW. Max tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t				axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s *		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	uint32_t			uarfcn;							/**< RW-Locked. Downlink UARFCN. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
	ORI_Boolean_e 		AWS_enPeakCancel;				/**< RW-Locked. AW2S Vendor specific: Peak-Cancellation CFR enablement. */
} ORI_ObjectParams_TxSigPathUtra_s;

/**
 * @struct ORI_ObjectParams_TxSigPathEUtraFDD_s
 * @brief Structure containing the parameters of a TxSigPath EUTRAFDD object.
 */
typedef struct
{
	uint16_t 			chanBW;							/**< RW-Locked. Channel bandwith in MHz/10 (14 / 30 / 50 / 100 / 150 / 200). */
	uint16_t			dlCalREMax;						/**< RO. Max possible buffer in RE for DL timing calibration in Tc/16.*/
	uint32_t			t2a;							/**< RO. RE time delay. */
	uint16_t 			dlCalRE;						/**< RW-Locked. Time delay to enable DL timing calibration in Tc/16. */
	int16_t 			maxTxPwr;						/**< RW. Max tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	uint32_t 			earfcn;							/**< RW-Locked. Downlink EARFCN. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	ORI_Boolean_e		enableIQDLComp;					/**< RW-Locked. IQ data compression enablement. */
	uint32_t			sigmaIQ;						/**< RW-Locked. Sigma IQ value for IQ data compression. */
	ORI_Boolean_e		AWS_enableCompRateChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression sample rate change. */
	ORI_Boolean_e		AWS_enableCompBitChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression bit width change. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
	ORI_Boolean_e 		AWS_enPeakCancel;				/**< RW-Locked. AW2S Vendor specific: Peak-Cancellation CFR enablement. */
} ORI_ObjectParams_TxSigPathEUtraFDD_s;

/**
 * @struct ORI_ObjectParams_TxSigPathEUtraTDD_s
 * @brief Structure containing the parameters of a TxSigPath EUTRATDD object.
 */
typedef struct
{
	uint16_t 			chanBW;							/**< RW-Locked. Channel bandwith in MHz/10 (14 / 30 / 50 / 100 / 150 / 200). */
	uint8_t				tddULDLConfig;					/**< RW-Locked. TDD UL/DL config. */
	uint8_t				tddSpecialSFConfig;				/**< RW-Locked. TDD SSF config. */
	ORI_tddCPLength_e	tddCPLengthDL;					/**< RW-Locked. TDD Cyclic prefix length. */
	uint16_t 			dlCalREMax;						/**< RO. Max possible buffer in RE for DL timing calibration in Tc/16.*/
	uint32_t 			t2a;							/**< RO. RE time delay. */
	uint16_t 			dlCalRE;						/**< RW-Locked. Time delay to enable DL timing calibration in Tc/16. */
	int16_t 			maxTxPwr;						/**< RW. Max tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	uint32_t 			earfcn;							/**< RW-Locked. Downlink EARFCN. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	ORI_Boolean_e		enableIQDLComp;					/**< RW-Locked. IQ data compression enablement. */
	uint32_t			sigmaIQ;						/**< RW-Locked. Sigma IQ value for IQ data compression. */
	ORI_Boolean_e		AWS_enableCompRateChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression sample rate change. */
	ORI_Boolean_e		AWS_enableCompBitChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression bit width change. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
	ORI_Boolean_e 		AWS_enPeakCancel;				/**< RW-Locked. AW2S Vendor specific: Peak-Cancellation CFR enablement. */
} ORI_ObjectParams_TxSigPathEUtraTDD_s;

/**
 * @struct ORI_ObjectParams_TxSigPathGSM_s
 * @brief Structure containing the parameters of a TxSigPath GSM object.
 */
typedef struct
{
	uint16_t 			dlCalREMax;						/**< RO. Max possible buffer in RE for DL timing calibration in Tc/16.*/
	ORI_freqBand_e 		freqBandInd;					/**< GSM frequency band indicator. */
	uint32_t 			t2a;							/**< RO. RE time delay. */
	uint16_t 			dlCalRE;						/**< RW-Locked. Time delay to enable DL timing calibration in Tc/16. */
	int16_t 			maxTxPwr;						/**< RW. Max tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
	ORI_Boolean_e 		AWS_enPeakCancel;				/**< RW-Locked. AW2S Vendor specific: Peak-Cancellation CFR enablement. */
} ORI_ObjectParams_TxSigPathGSM_s;

/**
 * @struct ORI_ObjectParams_TxSigPathNRFDD_s
 * @brief Structure containing the parameters of a TxSigPath NRFDD object.
 */
typedef struct
{
	uint16_t 			chanBW;							/**< RW-Locked. Channel bandwith in MHz/10. */
	uint16_t			dlCalREMax;						/**< RO. Max possible buffer in RE for DL timing calibration in Tc/16.*/
	uint32_t			t2a;							/**< RO. RE time delay. */
	uint16_t 			dlCalRE;						/**< RW-Locked. Time delay to enable DL timing calibration in Tc/16. */
	int16_t 			maxTxPwr;						/**< RW. Max tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	ORI_Boolean_e		AWS_enableCompRateChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression sample rate change. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
	ORI_Boolean_e 		AWS_enPeakCancel;				/**< RW-Locked. AW2S Vendor specific: Peak-Cancellation CFR enablement. */
	uint32_t 			AWS_arfcn;						/**< RW-Locked. ARFCN of the NR carrier. */
} ORI_ObjectParams_TxSigPathNRFDD_s;

/**
 * @struct ORI_ObjectParams_TxSigPathNRTDD_s
 * @brief Structure containing the parameters of a TxSigPath NRTDD object.
 */
typedef struct
{
	uint16_t 			chanBW;							/**< RW-Locked. Channel bandwith in MHz/10. */
	uint16_t			dlCalREMax;						/**< RO. Max possible buffer in RE for DL timing calibration in Tc/16.*/
	uint32_t			t2a;							/**< RO. RE time delay. */
	uint16_t 			dlCalRE;						/**< RW-Locked. Time delay to enable DL timing calibration in Tc/16. */
	int16_t 			maxTxPwr;						/**< RW. Max tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	ORI_Boolean_e		AWS_enableCompRateChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression sample rate change. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Tx power for this path, unit is dBm/10 (e.g. 400 for 40 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
	ORI_Boolean_e 		AWS_enPeakCancel;				/**< RW-Locked. AW2S Vendor specific: Peak-Cancellation CFR enablement. */
	uint32_t 			AWS_arfcn;						/**< RW-Locked. ARFCN of the NR carrier. */
} ORI_ObjectParams_TxSigPathNRTDD_s;

/**
 * @struct ORI_ObjectParams_RxSigPathUtra_s
 * @brief Structure containing the parameters of a RxSigPath UTRAFDD object.
 */
typedef struct
{
	uint16_t			ulCalREMax;						/**< RO. Max possible buffer in RE for UL timing calibration in Tc/2.*/
	uint32_t 			ta3;							/**< RO. RE time delay. */
	uint16_t 			ulCalRE;						/**< RW-Locked. Time delay to enable UL timing calibration in Tc/2. */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	uint8_t 			rtwpGroup;						/**< RW-Locked. Location of the RTWP measurements for this AxC. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	uint32_t			uarfcn;							/**< RW-Locked. Uplink UARFCN. */
	int16_t 			ulFeedAdj;						/**< RW. Uplink feeder adjustment in dB/10 (e.g. 200 for 20 dB). */
	uint8_t				ulTgtRMSLvl;					/**< RW-Locked. Uplink target RMS level. */
	uint8_t				ulAGCSetlgTime;					/**< RW-Locked. Uplink AGC settling time. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Rx power for this path, unit is dBm/10 (e.g. -650 for -65 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
} ORI_ObjectParams_RxSigPathUtra_s;

/**
 * @struct ORI_ObjectParams_RxSigPathEUtraFDD_s
 * @brief Structure containing the parameters of a RxSigPath EUTRAFDD object.
 *
 */
typedef struct
{
	uint16_t 			chanBW;							/**< RW-Locked. Channel bandwith in MHz/10 (14 / 30 / 50 / 100 / 150 / 200). */
	uint16_t			ulCalREMax;						/**< RO. Max possible buffer in RE for UL timing calibration in Tc/2.*/
	uint32_t  			ta3;							/**< RO. RE time delay. */
	uint16_t 			ulCalRE;						/**< RW-Locked. Time delay to enable UL timing calibration in Tc/2. */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	uint32_t 			earfcn;							/**< RW-Locked. Uplink EARFCN. */
	ORI_Boolean_e		enableIQULComp;					/**< RW-Locked. IQ data compression enablement. */
	uint32_t			sigmaIQ;						/**< RW-Locked. Sigma IQ value for IQ data compression. */
	ORI_Boolean_e		AWS_enableCompRateChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression sample rate change. */
	ORI_Boolean_e		AWS_enableCompBitChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression bit width change. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Rx power for this path, unit is dBm/10 (e.g. -650 for -65 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
} ORI_ObjectParams_RxSigPathEUtraFDD_s;

/**
 * @struct ORI_ObjectParams_RxSigPathEUtraTDD_s
 * @brief Structure containing the parameters of a RxSigPath EUTRATDD object.
 *
 */
typedef struct
{
	uint16_t 			chanBW;							/**< RW-Locked. Channel bandwith in MHz/10 (14 / 30 / 50 / 100 / 150 / 200). */
	uint8_t				tddULDLConfig;					/**< RW-Locked. TDD UL/DL config. */
	uint8_t				tddSpecialSFConfig;				/**< RW-Locked. TDD SSF config. */
	ORI_tddCPLength_e	tddCPLengthUL;					/**< RW-Locked. TDD Cyclic prefix length. */
	uint16_t			ulCalREMax;						/**< RO. Max possible buffer in RE for UL timing calibration in Tc/2.*/
	uint32_t  			ta3;							/**< RO. RE time delay. */
	uint16_t 			ulCalRE;						/**< RW-Locked. Time delay to enable UL timing calibration in Tc/2. */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	uint32_t 			earfcn;							/**< RW-Locked. Uplink EARFCN. */
	ORI_Boolean_e		enableIQULComp;					/**< RW-Locked. IQ data compression enablement. */
	uint32_t			sigmaIQ;						/**< RW-Locked. Sigma IQ value for IQ data compression. */
	ORI_Boolean_e		AWS_enableCompRateChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression sample rate change. */
	ORI_Boolean_e		AWS_enableCompBitChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression bit width change. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Rx power for this path, unit is dBm/10 (e.g. -650 for -65 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
} ORI_ObjectParams_RxSigPathEUtraTDD_s;

/**
 * @struct ORI_ObjectParams_RxSigPathGSM_s
 * @brief Structure containing the parameters of a RxSigPath GSM object.
 */
typedef struct
{
	uint16_t			ulCalREMax;						/**< RO. Max possible buffer in RE for UL timing calibration in Tc/2.*/
	ORI_freqBand_e 		freqBandInd;					/**< GSM frequency band indicator. */
	uint32_t  			ta3;							/**< RO. RE time delay. */
	uint16_t 			ulCalRE;						/**< RW-Locked. Time delay to enable UL timing calibration in Tc/2. */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	int16_t 			ulFeedAdj;						/**< RW. Uplink feeder adjustment in dB/10 (e.g. 200 for 20 dB). */
	ORI_Object_s *		TxSigPath;						/**< RW-Locked. Associated TxSigPath for frequency hopping information. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Rx power for this path, unit is dBm/10 (e.g. -650 for -65 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
} ORI_ObjectParams_RxSigPathGSM_s;

/**
 * @struct ORI_ObjectParams_RxSigPathNRFDD_s
 * @brief Structure containing the parameters of a RxSigPath NRFDD object.
 *
 */
typedef struct
{
	uint16_t 			chanBW;							/**< RW-Locked. Channel bandwith in MHz/10. */
	uint16_t			ulCalREMax;						/**< RO. Max possible buffer in RE for UL timing calibration in Tc/2.*/
	uint32_t  			ta3;							/**< RO. RE time delay. */
	uint16_t 			ulCalRE;						/**< RW-Locked. Time delay to enable UL timing calibration in Tc/2. */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	ORI_Boolean_e		AWS_enableCompRateChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression sample rate change. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Rx power for this path, unit is dBm/10 (e.g. -650 for -65 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
	uint32_t 			AWS_arfcn;						/**< RW-Locked. ARFCN of the NR carrier. */
} ORI_ObjectParams_RxSigPathNRFDD_s;

/**
 * @struct ORI_ObjectParams_RxSigPathNRTDD_s
 * @brief Structure containing the parameters of a RxSigPath NRTDD object.
 *
 */
typedef struct
{
	uint16_t 			chanBW;							/**< RW-Locked. Channel bandwith in MHz/10. */
	uint16_t			ulCalREMax;						/**< RO. Max possible buffer in RE for UL timing calibration in Tc/2.*/
	uint32_t  			ta3;							/**< RO. RE time delay. */
	uint16_t 			ulCalRE;						/**< RW-Locked. Time delay to enable UL timing calibration in Tc/2. */
	uint8_t 			axcW;							/**< RW-Locked. AxC W parameter. */
	uint8_t 			axcB;							/**< RW-Locked. AxC B parameter. */
	ORI_Object_s * 		oriLink;						/**< RW-Locked. ORI Link on which the AxC is mapped. */
	ORI_Object_s *		antPort;						/**< RW-Locked. Reference Antenna port for this signal. */
	ORI_Boolean_e		AWS_enableCompRateChange;		/**< RW-Locked. AW2S Vendor specific: Enable IQ data compression sample rate change. */
	int16_t 			AWS_measuredPwr;				/**< RO. AW2S Vendor specific: Measured Rx power for this path, unit is dBm/10 (e.g. -650 for -65 dBm). */
	uint16_t 			AWS_axcIncr;					/**< RW-Locked. AW2S Vendor specific: AxC increment for each sample, 0 means auto (= packed, no interleaving). */
	uint32_t 			AWS_arfcn;						/**< RW-Locked. ARFCN of the NR carrier. */
} ORI_ObjectParams_RxSigPathNRTDD_s;

/**
 * @struct ORI_ObjectParams_ORILink_s
 * @brief Structure containing the parameters of an ORI Link object.
 */
typedef struct
{
	char 						portLabel[81];			/**< RO. Physical ORI link port label. */
	ORI_portRoleCapability_e	portRoleCapability;		/**< RO. Port role capability. */
	ORI_portRole_e 				portRole;				/**< RW-Locked. Port role. */
	int16_t						bitRateSupport;			/**< RO. Supported line bit rate. */
	uint8_t 					bitRateRequested;		/**< RW-Locked. Requested line bit rate. 0 for auto-negotitation. */
	uint8_t 					bitRateOperational;		/**< RO. Current line bit rate. 0 for link down. */
	uint64_t 					localPortID;			/**< RO. Local end port ID. */
	uint64_t 					remotePortID;			/**< RO. Remote end port ID. */
	uint32_t 					toffset;				/**< RO. CPRI time delay component. */
	ORI_linkType_e				oriLinkType;			/**< RW-Locked. ORI Link type. */
	uint8_t						AWS_localMAC[6];		/**< R0. AW2S Vendor specific: Local MAC address of the ORI link. */
	uint8_t						AWS_remoteMAC[6];		/**< RW. AW2S Vendor specific: Remote MAC address of the ORI link. */
	uint32_t 					AWS_t14;				/**< RO. AW2S Vendor specific: CPRI time delay component. */
	uint32_t 					AWS_sfpTxPow;			/**< RO. AW2S Vendor specific: SFP Tx power. */
	uint32_t 					AWS_sfpRxPow;			/**< RO. AW2S Vendor specific: SFP Rx power. */
	uint8_t						AWS_remoteIP[4];		/**< RW. AW2S Vendor specific: IP of REC for ECPRI Ethernet frame */
	uint8_t						AWS_localIP[4];			/**< RW. AW2S Vendor specific: IP of RE for ECPRI Ethernet frame */
	uint16_t					AWS_remoteUdpPort;		/**< RW. AW2S Vendor specific: UDP port of REC for ECPRI Ethernet frame */
	uint16_t					AWS_localUdpPort;		/**< RO. AW2S Vendor specific: UDP port of RE for ECPRI Ethernet frame */
} ORI_ObjectParams_ORILink_s;

/**
 * @struct ORI_ObjectParams_ExternalEventPort_s
 * @brief Structure containing the parameters of an External Event Port object.
 */
typedef struct
{
	char 				portLabel[81];					/**< RO. External event port label. */
} ORI_ObjectParams_ExternalEventPort_s;

/**
 * @struct ORI_ObjectParams_AISGPort_s
 * @brief Structure containing the parameters of an AISG Port object.
 */
typedef struct
{
	char 						portLabel[81];			/**< RO. AISG port label. */
	ORI_Boolean_e 				busPowerEnable;			/**< RW-Locked. Bus power enablement. */
} ORI_ObjectParams_AISGPort_s;

/**
 * @struct ORI_ObjectParams_AISGALD_s
 * @brief Structure containing the parameters of an AISG ALD object.
 */
typedef struct
{
	uint8_t 				deviceType;					/**< RW-Locked. Device type of the ALD. */
	uint8_t 				UID[20];					/**< RW-Locked. Unique ID array. */
	uint8_t					releaseID;					/**< RO. 3GPP protocol release. */
	uint8_t					aisgVersion;				/**< RO. AISG protocol version. */
	uint8_t					deviceTypeVersion[3];		/**< RO. Device type substance version. */
	uint16_t				frameLength;				/**< RO. Maximum frame length for AISG Layer 7 message payload. */
	uint8_t					hdlcAdress;					/**< RO. Actual HLDC address. */
} ORI_ObjectParams_AISGALD_s;

/**
 * @struct ORI_ObjectParams_Log_s
 * @brief Structure containing the parameters of a Log object.
 */
typedef struct
{
	char 						logTypeID[41];			/**< RO. Log type identifier. */
	char 						description[81];		/**< RO. Log description. */
	ORI_logCategory_e			logCategory;			/**< RO. Log category. */
	uint32_t 					maxREfileSize;			/**< RO. Max RE file size in kB. */
	uint32_t 					maxRECfileSize;			/**< RW. Max REC file size in kB. */
	ORI_Boolean_e 				enableNotification;		/**< RW. Enable REC notification on file transfer availability. */
	ORI_Boolean_e 				fileAvailable;			/**< RO. File is available. */
	ORI_overflowBehaviour_e		overflowBehaviour;		/**< RW. Behaviour on overflow. */
	ORI_Boolean_e				recordingEnabled;		/**< RW. Recording enablement. */
	uint64_t					logPeriod;				/**< RW. Log expiration period in seconds. */
	ORI_timerType_e				timerType;				/**< RW. Log expiration behaviour. */
} ORI_ObjectParams_Log_s;

/**
 * @struct ORI_ObjectParams_DLRoutedIQData_s
 * @brief Structure containing the parameters of a DL Routed IQ Data object.
 */
typedef struct
{
	uint16_t 			IQsubBlockSize;					/**< RW-Locked. Number of bits contained in the IQ data sub-block. */
	ORI_Object_s *		MasterPortOriLink;				/**< RW-Locked. Reference to the master port ORI Link. */
	uint8_t 			MasterPortIQblkW;				/**< RW-Locked. Sub-block start W parameter for master port. */
	uint16_t 			MasterPortIQblkB;				/**< RW-Locked. Sub-block start B parameter for master port. */
	ORI_Object_s *		SlavePortOriLink;				/**< RW-Locked. Reference to the slave port ORI Link. */
	uint8_t				SlavePortIQW;					/**< RW-Locked. Sub-block start W parameter for slave port. */
	uint16_t			SlavePortIQB;					/**< RW-Locked. Sub-block start B parameter for slave port. */
	uint16_t			TBDelayDL;						/**< RO. Internal RE delay from slave port to master port. */
} ORI_ObjectParams_DLRoutedIQData_s;

/**
 * @struct ORI_ObjectParams_ULRoutedIQData_s
 * @brief Structure containing the parameters of a UL Routed IQ Data object.
 */
typedef struct
{
	uint16_t 			IQsubBlockSize;					/**< RW-Locked. Number of bits contained in the IQ data sub-block. */
	ORI_Object_s *		MasterPortOriLink;				/**< RW-Locked. Reference to the master port ORI Link. */
	uint8_t				MasterPortIQblkW;				/**< RW-Locked. Sub-block start W parameter for master port. */
	uint16_t 			MasterPortIQblkB;				/**< RW-Locked. Sub-block start B parameter for master port. */
	ORI_Object_s *		SlavePortOriLink;				/**< RW-Locked. Reference to the slave port ORI Link. */
	uint8_t				SlavePortIQW;					/**< RW-Locked. Sub-block start W parameter for slave port. */
	uint16_t			SlavePortIQB;					/**< RW-Locked. Sub-block start B parameter for slave port. */
	uint16_t			TBDelayUL;						/**< RO. Internal RE delay from master port to slave port. */
} ORI_ObjectParams_ULRoutedIQData_s;

/**
 * @struct ORI_ObjectParams_DLRoutedCWBlock_s
 * @brief Structure containing the parameters of a DL Routed CW Block object.
 */
typedef struct
{
	uint8_t				CtrlBlockSize;					/**< RW-Locked. Number of consecutive sub-channels in the CW block. */
	uint8_t				SubChannelStart;				/**< RW-Locked. Lowest sub-channel of the CW block. */
	uint8_t				Ydepth;							/**< RW-Locked. Number of consecutive Y locations of the sub-channels in the CW block. */
	uint8_t				SlavePortYoffset;				/**< RW-Locked. Lowest Y location of the sub-channel(s) at the slave port. */
	uint8_t				MasterPortYoffset;				/**< RW-Locked. Lowest Y location of the sub-channel(s) at the master port. */
	ORI_Object_s *		SlavePortOriLink;				/**< RW-Locked. Reference to the slave port ORI Link. */
	ORI_Object_s *		MasterPortOriLink;				/**< RW-Locked. Reference to the master port ORI Link. */
} ORI_ObjectParams_DLRoutedCWBlock_s;

/**
 * @struct ORI_ObjectParams_ULRoutedCWBlock_s
 * @brief Structure containing the parameters of a UL Routed CW Block object.
 */
typedef struct
{
	uint8_t				CtrlBlockSize;					/**< RW-Locked. Number of consecutive sub-channels in the CW block. */
	uint8_t				SubChannelStart;				/**< RW-Locked. Lowest sub-channel of the CW block. */
	uint8_t				Ydepth;							/**< RW-Locked. Number of consecutive Y locations of the sub-channels in the CW block. */
	uint8_t				SlavePortYoffset;				/**< RW-Locked. Lowest Y location of the sub-channel(s) at the slave port. */
	uint8_t				MasterPortYoffset;				/**< RW-Locked. Lowest Y location of the sub-channel(s) at the master port. */
	ORI_Object_s *		SlavePortOriLink;				/**< RW-Locked. Reference to the slave port ORI Link. */
	ORI_Object_s *		MasterPortOriLink;				/**< RW-Locked. Reference to the master port ORI Link. */
} ORI_ObjectParams_ULRoutedCWBlock_s;

/**
 * @union ORI_ObjectParams_u
 * @brief Union of all the parameters of the ORI objects. Access each field based on the object type.
 */
typedef union
{
	ORI_ObjectParams_RE_s						RE;						/**< Parameters for ::ORI_ObjectType_RE. */
	ORI_ObjectParams_AntennaPort_s				AntPort;				/**< Parameters for ::ORI_ObjectType_AntennaPort. */
	ORI_ObjectParams_TxSigPathUtra_s	  		TxUtra;					/**< Parameters for ::ORI_ObjectType_TxUtra. */
	ORI_ObjectParams_TxSigPathEUtraFDD_s		TxEUtraFDD;				/**< Parameters for ::ORI_ObjectType_TxEUtraFDD. */
	ORI_ObjectParams_TxSigPathEUtraTDD_s		TxEUtraTDD;				/**< Parameters for ::ORI_ObjectType_TxEUtraTDD. */
	ORI_ObjectParams_TxSigPathGSM_s				TxGSM;					/**< Parameters for ::ORI_ObjectType_TxGSM. */
	ORI_ObjectParams_TxSigPathNRFDD_s			TxNRFDD;				/**< Parameters for ::ORI_ObjectType_TxNRFDD. */
	ORI_ObjectParams_TxSigPathNRTDD_s			TxNRTDD;				/**< Parameters for ::ORI_ObjectType_TxNRTDD. */
	ORI_ObjectParams_RxSigPathUtra_s	  		RxUtra;					/**< Parameters for ::ORI_ObjectType_RxUtra. */
	ORI_ObjectParams_RxSigPathEUtraFDD_s		RxEUtraFDD;				/**< Parameters for ::ORI_ObjectType_RxEUtraFDD. */
	ORI_ObjectParams_RxSigPathEUtraTDD_s		RxEUtraTDD;				/**< Parameters for ::ORI_ObjectType_RxEUtraTDD. */
	ORI_ObjectParams_RxSigPathGSM_s				RxGSM;					/**< Parameters for ::ORI_ObjectType_RxGSM. */
	ORI_ObjectParams_RxSigPathNRFDD_s			RxNRFDD;				/**< Parameters for ::ORI_ObjectType_RxNRFDD. */
	ORI_ObjectParams_RxSigPathNRTDD_s			RxNRTDD;				/**< Parameters for ::ORI_ObjectType_RxNRTDD. */
	ORI_ObjectParams_ORILink_s					ORILink;				/**< Parameters for ::ORI_ObjectType_ORILink. */
	ORI_ObjectParams_ExternalEventPort_s		ExternalEventPort;		/**< Parameters for ::ORI_ObjectType_ExternalEventPort. */
	ORI_ObjectParams_AISGPort_s					AISGPort;				/**< Parameters for ::ORI_ObjectType_AISGPort. */
	ORI_ObjectParams_AISGALD_s					AISGALD;				/**< Parameters for ::ORI_ObjectType_AISGALD. */
	ORI_ObjectParams_Log_s						Log;					/**< Parameters for ::ORI_ObjectType_Log. */
	ORI_ObjectParams_DLRoutedIQData_s			DLRoutedIQData;			/**< Parameters for ::ORI_ObjectType_DLRoutedIQData. */
	ORI_ObjectParams_ULRoutedIQData_s			ULRoutedIQData;			/**< Parameters for ::ORI_ObjectType_ULRoutedIQData. */
	ORI_ObjectParams_DLRoutedCWBlock_s			DLRoutedCWBlock;		/**< Parameters for ::ORI_ObjectType_DLRoutedCWBlock. */
	ORI_ObjectParams_ULRoutedCWBlock_s			ULRoutedCWBlock;		/**< Parameters for ::ORI_ObjectType_ULRoutedCWBlock. */
} ORI_ObjectParams_u;



/*-----------------------------------------------------------------------------------------
 * OBJECT FAULTS
 *-----------------------------------------------------------------------------------------*/

#define FAULT_MAX_AFFECTED_OBJ		20

/**
 * @struct ORI_Fault_s
 * @brief Structure detailing a fault, its state, severity, description and affected objects.
 */
typedef struct
{
	ORI_FaultState_e		state;										/**< Current state of the fault. */
	ORI_FaultSeverity_e		severity;									/**< Severity of the fault. */
	char 					timestamp[256];								/**< Time stamp string of the fault (RE time reference). */
	char					desc[256];									/**< Short text description associated to the fault. */
	uint32_t				numAffectedObjects;							/**< Number of additional objects affected by the fault. */
	ORI_Object_s *			affectedObjects[FAULT_MAX_AFFECTED_OBJ];	/**< List of additional objects affected by the fault. */
} ORI_Fault_s;

/**
 * @struct ORI_ObjectFaults_RE_s
 * @brief Structure containing the faults of a RE object.
 */
typedef struct
{
	ORI_Fault_s			extSuplyUndervolt;				/**< Power supply under voltage. FaultType is ::ORI_FaultType_RE_ExtSupplyUnderVolt */
	ORI_Fault_s			overTemp;						/**< Over temperature. FaultType is ::ORI_FaultType_RE_OverTemp */
	ORI_Fault_s			digInOverdrive;					/**< Input digital signal level overdrive. FaultType is ::ORI_FaultType_RE_DigInOverdrive */
	ORI_Fault_s			rfOutOverdrive;					/**< RF output power overdrive. FaultType is ::ORI_FaultType_RE_RFOutOverdrive */
	ORI_Fault_s			txGainFail;						/**< Tx gain control failure. FaultType is ::ORI_FaultType_RE_TXGainFail */
	ORI_Fault_s			rxGainFail;						/**< Rx gain control failure. FaultType is ::ORI_FaultType_RE_RXGainFail */
} ORI_ObjectFaults_RE_s;

/**
 * @struct ORI_ObjectFaults_AntennaPort_s
 * @brief Structure containing the faults of an Antenna Port object.
 */
typedef struct
{
	ORI_Fault_s			vswrOutOfRange;					/**< VSWR at the antenna port exceeded limit. FaultType is ::ORI_FaultType_AntennaPort_VSWROutOfRange */
	ORI_Fault_s			nonAisgTmaMalfct;				/**< Non AISG TMA malfunction. FaultType is ::ORI_FaultType_AntennaPort_NonAisgTmaMalfct */
} ORI_ObjectFaults_AntennaPort_s;

/**
 * @struct ORI_ObjectFaults_ORILink_s
 * @brief Structure containing the faults of an ORI Link object.
 */
typedef struct
{
	ORI_Fault_s			linkFailure;					/**< LOS, LOF, SDI or RAI received on the ORI Link. FaultType is ::ORI_FaultType_ORILink_LinkFail */
	ORI_Fault_s			portFailure;					/**< Local ORI slave port failure. FaultType is ::ORI_FaultType_ORILink_PortFail */
	ORI_Fault_s			syncFailure;					/**< Synchronization lost on slave port. FaultType is ::ORI_FaultType_ORILink_SyncFail */
} ORI_ObjectFaults_ORILink_s;

/**
 * @struct ORI_ObjectFaults_AISGPort_s
 * @brief Structure containing the faults of an AISG Port object.
 */
typedef struct
{
	ORI_Fault_s			aisgMalfct;						/**< Hardware malfunction on AISG port. FaultType is ::ORI_FaultType_AISGPort_AisgMalfct */
} ORI_ObjectFaults_AISGPort_s;

/**
 * @union ORI_ObjectFaults_u
 * @brief Union of all the faults of the ORI objects. Access each field based on the object type.
 */
typedef union
{
	ORI_ObjectFaults_RE_s						RE;						/**< Faults for ::ORI_ObjectType_RE. */
	ORI_ObjectFaults_AntennaPort_s				AntPort;				/**< Faults for ::ORI_ObjectType_AntennaPort. */
	ORI_ObjectFaults_ORILink_s					ORILink;				/**< Faults for ::ORI_ObjectType_ORILink. */
	ORI_ObjectFaults_AISGPort_s					AISGPort;				/**< Faults for ::ORI_ObjectType_AISGPort. */
} ORI_ObjectFaults_u;



/*-----------------------------------------------------------------------------------------
 * INDICATION DATA STRUCTURES
 *-----------------------------------------------------------------------------------------*/

/**
 * @struct ORI_Ind_TransferFileCmplt_s
 * @brief ORI file transfer complete indication structure.
 *
 * This structure is passed on a file transfer complete indication and gives the result
 * of a file transfer completion.
 */
typedef struct
{
	ORI_Result_e 			result;						/**< Result of the file transfer. */
	char 					failInfo[256];				/**< String indicating the file transfer failure reason, if applicable. */
} ORI_Ind_TransferFileCmplt_s;

/**
 * @struct ORI_Ind_ObjectStateChange_s
 * @brief ORI object state change indication structure.
 *
 * This structure is passed on an object state change indication and gives the affected object and the type of the
 * state that changed. <br>
 * The new state value is updated in the model and can be accessed directly in the object.
 */
typedef struct
{
	ORI_Object_s *			object;						/**< Object for which the state changed. */
	ORI_StateType_e			stateType;					/**< Type of the state that changed. */
} ORI_Ind_ObjectStateChange_s;

/**
 * @struct ORI_Ind_FaultChange_s
 * @brief ORI fault change indication structure.
 *
 * This structure is passed on a fault change indication (activated or cleared) and gives the primary affected object, the fault type and the actual fault structure. <br>
 * The new fault structure is updated in the model and can be accessed directly in the object based on the fault type.
 */
typedef struct
{
	ORI_Object_s *			object;						/**< Primary object affected by the fault. */
	ORI_FaultType_e			faultType;					/**< Type of the fault that changed. */
	ORI_Fault_s *			fault;						/**< Reference to the object's fault structure for the given fault type. */
} ORI_Ind_FaultChange_s;

/**
 * @struct ORI_Ind_FileAvailable_s
 * @brief ORI File Available indication structure.
 *
 * This structure is passed on a file available indication and gives the object concerned by the file available.
 */
typedef struct
{
	ORI_Object_s *			object;						/**< Object concerned by file availability. Valid objects : Log:X. */
} ORI_Ind_FileAvailable_s;

/**
 * @struct ORI_Ind_UploadFileCmpl_s
 * @brief ORI Upload File Complete indication structure.
 *
 * This structure is passed on an upload file complete indication and gives the result and the object concerned by the file upload.
 */
typedef struct
{
	ORI_Result_e			result;						/**< Result of the file transfer. */
	ORI_Object_s *			object;						/**< Object concerned by the file upload. Valid objects : RE:0 or Log:X. */
	char 					failInfo[256];				/**< String indicating the file transfer failure reason, if applicable. */
} ORI_Ind_UploadFileCmpl_s;

/**
 * @struct ORI_Ind_DeviceScanCmpl_s
 * @brief ORI AISG Device Scan Complete indication structure.
 *
 * This structure is passed on an aisg device scan complete indication and gives the result and the object concerned by the device scan.
 */
typedef struct
{
	ORI_Object_s *			object;						/**< Object concerned by the device scan. Valid objects : aisgPort:X. */
	int 					numAlds;					/**< integer indicating the number of ALDs found by the device scan. */
} ORI_Ind_DeviceScanCmpl_s;

/**
 * @struct ORI_Ind_L7respGetAlarm_s
 * @brief ORI AISG Transmit L7 message indication with command Get Alarm Status.
 *
 * This structure is passed on an aisg ALD receive indication and gives layer 7 message Get Alarm Status fields.
 */
typedef struct
{
	unsigned char			returnCode[16];	// returnCode, until 16 values supported
}ORI_Ind_L7respGetAlarm_s;

/**
 * @struct ORI_Ind_L7respGetInfo_s
 * @brief ORI AISG Transmit L7 message indication with command Get Info.
 *
 * This structure is passed on an aisg ALD receive indication and gives layer 7 message Get Info fields.
 */
typedef struct
{
	int 					PNlen;		// Product Number's length
	char 					PN[64];		// Product Number
	int 					SNlen;		// Serial Number's length
	char					SN[64];		// Serial Number
	int						HWverLen;	// Hardware Version's length
	char					HWver[64];	// Hardware Version
	int						SWverLen;	// Software Version's length
	char					SWver[64];	// Software Version
}ORI_Ind_L7respGetInfo_s;

typedef struct
{
	unsigned char			data[80];
}ORI_Ind_L7respReadUserData_s;

/**
* @struct ORI_Ind_L7respGetAlarm_s
* @brief ORI AISG Transmit L7 message indication with command Get Alarm Status.
*
* This structure is passed on an aisg ALD receive indication and gives layer 7 message Get Alarm Status fields.
*/
typedef struct
{
	unsigned char			alarmCode[16];	// alarmCode, until 16 values supported
}ORI_Ind_L7respSelfTest_s;


/**
 * @struct ORI_Ind_L7respGetTilt_s
 * @brief ORI AISG Transmit L7 message indication with command Get Tilt.
 *
 * This structure is passed on an aisg ALD receive indication and gives layer 7 message Get Tilt fields.
 */
typedef struct
{
	int 					Tilt;		// tilt value
}ORI_Ind_L7respGetTilt_s;

/**
 * @struct ORI_Ind_L7respGetDeviceData_s
 * @brief ORI AISG Transmit L7 message indication with command Get Device Data.
 *
 * This structure is passed on an aisg ALD receive indication and gives layer 7 message Get Device Data fields.
 */
typedef struct
{
	unsigned char			fieldNbr;
	char 					antModelNbr[32];		// Antenna Model Number
	char 					antSerialNbr[32];		// Antenna Serial Number
	unsigned short			antFreqBand;			// Antenna Frequency Band
	unsigned short			beamwidthBand[4];		// Beamwidth for each Band
	unsigned char			gainBand[4];			// Gain for each Band
	short					maxTilt;				// Maximum Supported Tilt
	short					minTilt;				// Minimum Supported Tilt
	char 					installationDate[32];	// Installation Date
	char 					installerID[32];		// Installer ID
	char 					basestationID[32];		// BaseStation ID
	char 					sectorID[32];			// sector ID
	unsigned short			antBearing;				// Antenna Bearing
	short					mechanicalTilt;			// Mechanical Tilt
}ORI_Ind_L7respGetDeviceData_s;

/**
 * @struct ORI_Ind_L7indAlarmIndication_s
 * @brief ORI AISG Transmit L7 message indication with alarm indication.
 *
 * This structure is passed on an aisg ALD receive indication and gives layer 7 message Alarm Indication Data fields.
 */
typedef struct
{
	unsigned char			returnCode;
	unsigned char			stateFlag;
}ret_alarm_s;
typedef struct
{
	ret_alarm_s				alarm[32];
}ORI_Ind_L7indAlarmIndication_s;

/**
 * @struct ORI_Ind_L7respAntGetNbr_s
 * @brief ORI AISG Transmit L7 message indication with command Antenna Get Number Of Antennas.
 *
 * This structure is passed on an aisg ALD receive indication and gives layer 7 message Antenna Get Number Of Antennas Data fields.
 */
typedef struct
{
	unsigned char			nbr;
}ORI_Ind_L7respAntGetNbr_s;

/**
 * @struct ORI_Ind_respGetParam_s
 * @brief ORI AISG getParam message indication .
 *
 * This structure is passed on a getParam command to obtain the ALD's deviceType.
 */
typedef struct
{
	unsigned char 			deviceType;
}ORI_Ind_respGetParam_s;

/**
 * @struct ORI_Ind_respCheckPortExist_s
 * @brief ORI AISG check AISG Port existence .
 *
 * This structure is passed on a check aisg port exist command to know if there is an aisg port in this product or not.
 */
typedef struct
{
	ORI_Boolean_e			exist;
}ORI_Ind_respCheckPortExist_s;

/**
 * @struct ORI_Ind_L7msg_s
 * @brief ORI AISG Transmit L7 message indication struct.
 *
 * This struct is passed on an aisg ALD receive indication and gives a layer 7 message.
 */
typedef struct
{
	char							raw[256];
	ORI_AisgLayer7Command_e			command;
	ORI_AisgReturnCode_e			returnCode;
	unsigned char					multiAntennaNbr;
	ORI_Ind_L7respGetAlarm_s		getAlarm;
	ORI_Ind_L7respGetInfo_s			getInfo;
	ORI_Ind_L7respReadUserData_s	readUserData;
	ORI_Ind_L7respSelfTest_s		selfTest;
	ORI_Ind_L7respGetTilt_s			getTilt;
	ORI_Ind_L7respGetDeviceData_s	getDeviceData;
	ORI_Ind_L7indAlarmIndication_s	alarmIndication;
	ORI_Ind_L7respAntGetNbr_s		getNbrAntennas;
	ORI_Ind_respGetParam_s			getParam;
	ORI_Ind_respCheckPortExist_s	checkAisgPortExist;
}ORI_Ind_L7msg_s;

/**
 * @struct ORI_Ind_AisgALDRx_s
 * @brief ORI AISG ALD receive indication structure.
 *
 * This structure is passed on an aisg ALD receive indication and gives a layer 7 message and the object concerned.
 */
typedef struct
{
	ORI_Object_s *			object;						/**< Object concerned by the device scan. Valid objects : aisgPort:X/aisgALD:Y. */
	ORI_Ind_L7msg_s			L7message;					/**< string of a layer7 message sent by the ALD. */
} ORI_Ind_AisgALDRx_s;

/**
 * @union ORI_IndicationValue_u
 * @brief ORI indication value union.
 *
 * This union can be accessed based on the indication type. Its members give details on the received indication.
 */
typedef union
{
	ORI_Ind_TransferFileCmplt_s transferFileCmplt;		/**< File transfer complete structure, access it on an ::ORI_IndicationType_FileTransferComplete. */
	ORI_Ind_ObjectStateChange_s	objectStateChange;		/**< Object state change structure, access it on an ::ORI_IndicationType_ObjectStateChange. */
	ORI_Ind_FaultChange_s		faultChange;			/**< Fault change structure, access it on an ::ORI_IndicationType_FaultChange. */
	ORI_Ind_FileAvailable_s		fileAvailable;			/**< File available structure, access it on an ::ORI_IndicationType_FileAvailable. */
	ORI_Ind_UploadFileCmpl_s	uploadFileCmpl;			/**< Upload file complete structure, access it on an ::ORI_IndicationType_UploadFileCmpl. */
	ORI_Ind_DeviceScanCmpl_s	deviceScanCmpl;			/**< AISG device scan complete structure, access it on an ::ORI_IndicationType_AisgScanDeviceCompl. */
	ORI_Ind_AisgALDRx_s			aisgALDRx;				/**< AISG ALD receive structure, access it on an ::ORI_IndicationType_AisgAldRx. */
} ORI_IndicationValue_u;

/**
 * @typedef ORI_IndCallback_f
 * @brief ORI indication callback function prototype.
 *
 * This is the prototype of the user callback function that is called when an indication has been received on the ORI Link. <br>
 * The indication callback passes in its arguments the @p type describing the type of the indication, such as file transfer completion, RE fault, etc. <br>
 * The @p value parameter shall be used to obtain more details on the indication.
 *
 * @param userData The user data that is from the ORI context structure.
 * @param type An ::ORI_IndicationType_e.
 * @param value An ::ORI_IndicationValue_u.
 * @return Void.
 *
 * @warning The indication callback is called in a separate thread.
 * @warning The state of the model is guaranteed to stay constant during the indication callback.
 * @warning ORI functions (except for ORI MODEL functions) must not be called during the indication callback.
 */
typedef void (ORI_IndCallback_f) (void * userData, ORI_IndicationType_e type, ORI_IndicationValue_u value);





/*-----------------------------------------------------------------------------------------
 * RE VERSION STRUCTURE
 *-----------------------------------------------------------------------------------------*/

/**
 * @struct ORI_REVersion_s
 * @brief ORI RE version query information structure.
 *
 * This structure contains the RE vendor specific version information obtained during a ORI_VersionQuery() procedure.
 */
typedef struct
{
	char 		vendorID[64];							/**< Vendor ID as signaled in DHCP code 201, normally 3 characters */
	char 		productID[64];							/**< */
	char 		productRev[64];							/**< */
	char 		serialNumber[64];						/**< */
	char 		hardwareVer[64];						/**< */
	char 		activeSwUpgradePkgVer[64];				/**< */
	char 		activeSwImgVer[64];						/**< */
	char 		passiveSwUpgradePkgVer[64];				/**< */
	char 		passiveSwImgVer[64];					/**< */
} ORI_REVersion_s;




/*-----------------------------------------------------------------------------------------
 * ORI OBJECT STRUCTURE
 *-----------------------------------------------------------------------------------------*/

/**
 * @struct ORI_Object_s
 * @brief Structure of an ORI Object.
 *
 * ORI Objects are internally created by the ORI library and represent an object present in the RE. <br>
 * Those objects represent and define the resource model of the RE, of which an image is present in the REC (via this library). <br>
 * Such objects can be accessed (for reading and navigating) by the user but must not be modified, created or deleted directly
 * as they are an 'image' of the RE objects. <br>
 * To update the parameters of the object, see ORI_ObjectParamReport(). <br>
 * To modify the parameters of the object, see ORI_ObjectParamModify(). <br>
 * To create an object, see ORI_ObjectCreation(). <br>
 * To delete an object, see ORI_ObjectDeletion(). <br>
 * To update the state of the object, see ORI_ObjectStateReport(). <br>
 * To modify the state of the object, see ORI_ObjectStateModify(). <br>
 * To update the faults of the object, see ORI_ObjectFaultReport(). <br>
 * To find an object in the model, see ORI_FindObject(). <br>
 * To retrieve a specific fault of an object, see ORI_ObjectFault().
 */
struct ORI_Object_s
{
	ORI_ObjectTypeRef_s	typeRef;					/**< Type reference of the object. */
	uint8_t				instanceNumber;				/**< Instance number of the object. */
	ORI_Object_s *		prev;						/**< Previous sibling. */
	ORI_Object_s *		next;						/**< Next sibling. */
	ORI_Object_s *		children;					/**< First child reference. */
	ORI_ObjectParams_u	params;						/**< Parameters union of this object. */
	ORI_AST_e			ast;						/**< Administrative state. */
	ORI_FST_e			fst;						/**< Functional state. */
	ORI_ObjectFaults_u	faults;						/**< Faults union of this object. */
};





/*-----------------------------------------------------------------------------------------
 * CONTEXT
 *-----------------------------------------------------------------------------------------*/

/**
 * @struct ORI_Version_s
 * @brief ORI version structure.
 *
 * This structure contains ORI library version information.
 */
typedef struct
{
	uint8_t					major;					/**< Version major number. */
	uint8_t					minor;					/**< Version minor number. */
} ORI_Version_s;

/**
 * @struct ORI_s
 * @brief ORI context data structure.
 *
 * This structure represents the context data for the ORI C library and shall be passed in ORI function calls. <br>
 * The @p opaque field is a library specific field and must not be modified by the user application. <br>
 * Other fields may be set (and changed) at run-time by the user application.
 *
 * @warning The indication callback is called in a separate thread.
 * @warning The state of the model is guaranteed to stay constant during the indication callback.
 * @warning ORI functions (except for ORI MODEL functions) must not be called during the indication callback.
 */
typedef struct
{
	void * 					opaque; 				/**< Set by the library, DO NOT MODIFY. */
	ORI_IndCallback_f *		indicationCallback;		/**< Pointer to user set ORI indication callback, this function is called when an indication is received on the ORI Link. If @c NULL (default), indications are discarded. */
	void *					userData;				/**< User data for passing into the ORI indication callback */
} ORI_s;



/*-----------------------------------------------------------------------------------------
 * ORI CREATION / DELETION / (DIS)CONNECTION
 *-----------------------------------------------------------------------------------------*/

/**
 * @brief Retrieve ORI library version structure.
 *
 * This functions returns the ORI library version structure (constant).
 *
 * @return Version structure.
 */
ORI_Version_s ORI_LibVersion(void);

/**
 * @brief Create an ORI context.
 *
 * This function creates and initializes an ORI context structure that is to be used when calling
 * other ORI library functions. This function also starts threads for data receiving and callbacks handling.
 *
 * @return The ORI context if creation successful, else @c NULL.
 *
 * @warning The returned ORI context must be free'd with a call to ORI_Free() when the user application is done with ORI.
 */
ORI_s *	ORI_Create(void);

/**
 * @brief Delete an ORI context.
 *
 * This function disconnects from the remote host (if connected), destroys the ORI threads and releases all associated ORI resources.
 *
 * @param ori The ORI context to delete.
 * @return Void.
 */
void ORI_Free(ORI_s * ori);

/**
 * @brief Enable ORI debug outputs.
 *
 * This function enables the ORI debug prints to stdout.
 *
 * @param ori The ORI context.
 * @return Void.
 */
void ORI_EnableDebug(ORI_s * ori);

/**
 * @brief Disable ORI debug outputs.
 *
 * This function disables the ORI debug prints to stdout.
 *
 * @param ori The ORI context.
 * @return Void.
 */
void ORI_DisableDebug(ORI_s * ori);

/**
 * @brief Connect to an ORI remote host (RE).
 *
 * This functions tries to connect the ORI context to a RE for C&M. The IP address @p serverIP of the RE and the network port @p port
 * indicates with which host form the TCP link. The @p timeout_ms specifies the timeout (in milliseconds) of the connection attempt. <br>
 * Once connected, callbacks from the ORI context may be called at any time, and the TCP Link monitoring timer shall be started on both RE and REC side. <br>
 * The REC side TCP link monitoring timer is integrated in the ORI library and is managed by the ORI_HealthCheck() function.
 *
 * @param ori The ORI context.
 * @param serverIP The IP address of the RE in string format (e.g. "192.168.100.1").
 * @param port The port to use for TCP connection.
 * @param timeout_ms The timeout in milliseconds of the connection attempt.
 * @param retry_timer_s Request retry timer in seconds when no response has been received from RE, until TCP link times-out, 0 means no retries will happen.
 * @return The result of the connection, ::ORI_Result_SUCCESS is returned if the connection is established.
 */
ORI_Result_e ORI_Connect(ORI_s * ori, const char * serverIP, int port, unsigned int timeout_ms, uint16_t retry_timer_s);

/**
 * @brief Disconnect from an ORI remote host (RE).
 *
 * This function disconnects the ORI context from the RE. This function does nothing if not currently connected. <br>
 * At disconnection, all buffered ORI response and indication messages are discarded, if any are still waiting to be processed.
 *
 * @param ori The ORI context.
 * @return Void.
 */
void ORI_Disconnect(ORI_s * ori);

/**
 * @brief Retrieve which Ethernet interface is used by the ORI TCP link.
 *
 * Returns the name of the currently (or last before disconnection) Ethernet interface used by the ORI TCP Link.
 *
 * @param ori The ORI context.
 * @return Name of the Ethernet interface (e.g. "eth2").
 */
const char * ORI_InterfaceName(ORI_s * ori);


/*-----------------------------------------------------------------------------------------
 * DEVICE MANAGEMENT
 *-----------------------------------------------------------------------------------------*/

/**
 * @brief Perform a RE Health Check procedure.
 *
 * This function performs the Health Check procedure with the RE to verify that the OCP layer is functioning correctly. <br>
 * A successful health check will update and re-start the TCP link timeout using the new @p tcpLinkMonTimeout parameter
 * of the RE and the ORI library (REC side). <br>
 * It is the application's responsibility to re-call this function within the specified Health Check Idle Timer which should
 * be at least 5 seconds lower than the timeout value of @p tcpLinkMonTimeout.
 *
 * @param ori The ORI context.
 * @param tcpLinkMonTimeout The new TCP Link Monitoring timeout value in seconds to store in the RE. Minimum value: 30 except special value 0 which means no timeout.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_HealthCheck(ORI_s * ori, uint16_t tcpLinkMonTimeout, ORI_Result_e * RE_result);

/**
 * @brief Perform a RE Set Time procedure.
 *
 * This function performs the Set Time procedure with the RE to set the absolute time reference used by the RE. <br>
 * The time reference is obtained from the calling application's local time reference.
 *
 * @param ori The ORI context.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_SetTime(ORI_s * ori, ORI_Result_e * RE_result);

/**
 * @brief Perform a RE Reset procedure.
 *
 * This function performs the Reset procedure with the RE. <br>
 * If successful, the RE shall close the TCP Link and reset itself.
 *
 * @param ori The ORI context.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_Reset(ORI_s * ori, ORI_Result_e * RE_result);



/*-----------------------------------------------------------------------------------------
 * SOFTWARE MANAGEMENT
 *-----------------------------------------------------------------------------------------*/

/**
 * @brief Perform a RE Version Query procedure.
 *
 * This function performs the Version Query procedure with the RE. <br>
 * If successful, the @p REVersion structure shall be filled with all the vendor specific RE version information,
 * containing the product information, hardware version and software images stored in the RE.
 *
 * @param ori The ORI context.
 * @param REVersion The RE version information structure.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_VersionQuery(ORI_s * ori, ORI_REVersion_s * REVersion, ORI_Result_e * RE_result);


/**
 * @brief Perform a RE Software Update Preparation procedure.
 *
 * This function performs the Software Update Preparation procedure with the RE. <br>
 * If successful, the RE shall start downloading the specified Software Upgrade Package @p SwUpgradePkgVer file from the specified
 * FTP server using the login credentials @p ftpSrvIpAddress, @p ftpSrvUserName and @p ftpSrvPassword. <br>
 * The Software file format is RE vendor specific. <br>
 * After download, the RE will store this new software package in non-volatile memory as the passive image. <br>
 * On completion of the process, the RE shall send a message indicating file transfer completion. If a callback is specified in the ORI context,
 * the callback ::ORI_IndCallback_f will be called with the enum ::ORI_IndicationType_FileTransferComplete
 * passed as parameter, as well as the file transfer completion result (success, or failure, with additional information).
 *
 * @param ori The ORI context.
 * @param ftpSrvIpAddress The FTP server address as a decimal point notation (e.g. "192.168.1.1").
 * @param ftpSrvUserName The FTP server login used by the RE.
 * @param ftpSrvPassword The FTP server password used by the RE.
 * @param ftpSrvSwPkgDirPath The FTP server directory location of the software package file.
 * @param SwUpgradePkgVer The name of the software package file.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_SoftwareUpdatePrep(ORI_s * ori, const char * ftpSrvIpAddress, const char * ftpSrvUserName, const char * ftpSrvPassword,
											const char * ftpSrvSwPkgDirPath, const char * SwUpgradePkgVer, ORI_Result_e * RE_result);

/**
 * @brief Perform a RE Software Activation procedure.
 *
 * This function performs the Software Activation procedure with the RE. <br>
 * If successful, the RE shall activate its passive software image, set its previous active image as passive, close the TCP Link then reset itself.
 *
 * @param ori The ORI context.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_SoftwareActivation(ORI_s * ori, ORI_Result_e * RE_result);



/*-----------------------------------------------------------------------------------------
 * CONFIGURATION MANAGEMENT
 *-----------------------------------------------------------------------------------------*/

/**
 * @brief Perform a RE Object Parameter Reporting procedure.
 *
 * This function performs the Object Parameter Reporting procedure with the RE. <br>
 * The target object or set of objects is specified by the arguments @p object and @p wildcard: <br>
 * - @p object is not @c NULL and @p wildcard is 0 : the targeted object is @p object. <br>
 * - @p object is not @c NULL and @p wildcard is not 0 : the targeted objects are all the children of @p object. <br>
 * - @p object is @c NULL : the targeted objects are all the objects in the model. <br>
 *
 * The parameter to report is defined by @p param. If @p param is ::ORI_ObjectParam_All, then all the parameters of the targeted
 * object(s) shall be reported. <br>
 * On success, the parameters of the targeted objects are updated in the model. <br>
 * If the RE reported parameters for objects not present in the model image in the REC, the model is internally updated and
 * the new objects created (this will be the case at initial software alignment, as the ORI GS specifies
 * a Parameter Reporting "ALL objects" and "ALL parameters" shall be done at start-up).
 *
 * @param ori The ORI context.
 * @param object The target object, or parent object, or @c NULL if no specific object; depending on @p wildcard.
 * @param wildcard Wildcard for targeting all objects, or all children objects; depending on @p object being @c NULL.
 * @param param The parameter to report, or all if ::ORI_ObjectParam_All.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_ObjectParamReport(ORI_s * ori, ORI_Object_s * object, int wildcard, ORI_ObjectParam_e param, ORI_Result_e * RE_result);

/**
 * @brief Perform a RE Object Parameter Modify procedure.
 *
 * This function performs the Object Parameter Modify procedure with the RE. <br>
 * The object for which the parameter(s) is(are) to be modified is given by @p object. <br>
 * The members of the @p params union for the specified target object type shall be set prior to calling this function, and each of the
 * parameter to modify must be specified in the @p paramList array. The number of parameters to set must be given by @p numParams. <br>
 * On procedure completion, the @p paramResult[] array will be filled with the results of each independent parameter modify attempt.
 *
 * @param ori The ORI context.
 * @param object The target object.
 * @param params The parameters union with values set for the parameters to modify.
 * @param paramList The array of enumerated parameters to modify. ::ORI_ObjectParam_All is not a valid enumeration for this procedure.
 * @param numParams The number of parameters to modify, must be > 0.
 * @param paramResult The return array for the individual result response for each parameter to modify.
 * @param RE_globalResult The RE response global result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_ObjectParamModify(ORI_s * ori, ORI_Object_s * object, ORI_ObjectParams_u params, ORI_ObjectParam_e paramList[],
										uint32_t numParams, ORI_Result_e paramResult[], ORI_Result_e * RE_globalResult);



/*-----------------------------------------------------------------------------------------
 * OBJECT LIFECYCLE
 *-----------------------------------------------------------------------------------------*/

/**
 * @brief Perform a RE Object Creation procedure.
 *
 * This function performs the Object Creation procedure with the RE. <br>
 * The object type to create is given by @p typeRef. <br>
 * The parameters @p params, @p paramList, @p numParams and @p paramResult function in the same maner as in the ORI_ObjectParamModify() procedure
 * and can be used to set default parameters for the newly created object. @p numParams can be set to 0, in which case no parameter is applied
 * to the new object. <br>
 * The created object is added in the model. <br>
 * This function also returns in @p newObject a reference in the model of the newly created object.
 *
 * @param ori The ORI context.
 * @param typeRef The type reference of the object to create.
 * @param params The parameters union with values set for the parameters to set.
 * @param paramList The array of enumerated parameters to set. ::ORI_ObjectParam_All is not a valid enumeration for this procedure.
 * @param numParams The number of parameters to set, can be 0.
 * @param paramResult The return array for the individual result response for each parameter to set.
 * @param newObject Reference to the newly created object.
 * @param RE_globalResult The RE response global result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_ObjectCreation(ORI_s * ori, ORI_ObjectTypeRef_s typeRef, ORI_ObjectParams_u params, ORI_ObjectParam_e paramList[],
										uint32_t numParams, ORI_Result_e paramResult[], ORI_Object_s ** newObject, ORI_Result_e * RE_globalResult);

/**
 * @brief Perform a RE Object Deletion procedure.
 *
 * This function performs the Object Deletion procedure with the RE. <br>
 * If successful, the object specified by @p object shall be deleted from the RE and removed from the library model.
 *
 * @param ori The ORI context.
 * @param object The object to delete.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_ObjectDeletion(ORI_s * ori, ORI_Object_s * object, ORI_Result_e * RE_result);



/*-----------------------------------------------------------------------------------------
 * OBJECT STATE MANAGEMENT
 *-----------------------------------------------------------------------------------------*/

/**
 * @brief Perform a RE Object State Reporting procedure.
 *
 * This function performs the Object State Reporting procedure with the RE. <br>
 * The target object or set of objects is specified by the arguments @p object and @p wildcard: <br>
 * - @p object is not @c NULL and @p wildcard is 0 : the targeted object is @p object. <br>
 * - @p object is not @c NULL and @p wildcard is not 0 : the targeted objects are all the children of @p object. <br>
 * - @p object is @c NULL : the targeted objects are all the objects in the model. <br>
 *
 * The state type to report is defined by @p stateType. If @p stateType is ::ORI_StateType_All, then all the states of the targeted
 * object(s) shall be reported. <br>
 * On success, the states of the targeted objects are updated in the model. <br>
 * If the RE reported states for objects not present in the model image in the REC, the model is internally updated and
 * the new objects created (this will be the case at initial software alignment, as the ORI GS specifies a State
 * Reporting "ALL objects" and "ALL states" shall be done at start-up).
 * If @p eventDrivenReport is set to ::ORI_EventDrivenReport_True or ::ORI_EventDrivenReport_False the event driven state reporting of the object is enabled
 * or disabled. Pass ::ORI_EventDrivenReport_NoModify to keep the event driven state reporting as is.
 *
 * @param ori The ORI context.
 * @param object The target object, or parent object, or @c NULL if no specific object; depending on @p wildcard.
 * @param wildcard Wildcard for targeting all objects, or all children objects; depending on @p object being @c NULL.
 * @param stateType The parameter to report, or all if ::ORI_StateType_All.
 * @param eventDrivenReport Set event driven state reporting of the object if applicable.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_ObjectStateReport(ORI_s * ori, ORI_Object_s * object, int wildcard, ORI_StateType_e stateType,
											ORI_EventDrivenReport_e eventDrivenReport, ORI_Result_e * RE_result);

/**
 * @brief Perform a RE Object State Modification procedure.
 *
 * This function performs the Object State Modification procedure with the RE. <br>
 * If successful, the administrative state (AST) of the target object will be set to @p ast.
 *
 * @param ori The ORI context.
 * @param object The object on which to set the state.
 * @param ast The administrative state to set.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_ObjectStateModification(ORI_s * ori, ORI_Object_s * object, ORI_AST_e ast, ORI_Result_e * RE_result);



/*-----------------------------------------------------------------------------------------
 * FAULT MANAGEMENT
 *-----------------------------------------------------------------------------------------*/

/**
 * @brief Perform a RE Object Fault Reporting procedure.
 *
 * This function performs the Object Fault Reporting procedure with the RE. <br>
 * The target object or set of objects is specified by the arguments @p object and @p wildcard: <br>
 * - @p object is not @c NULL and @p wildcard is 0 : the targeted object is @p object. <br>
 * - @p object is not @c NULL and @p wildcard is not 0 : the targeted objects are all the children of @p object. <br>
 * - @p object is @c NULL : the targeted objects are all the objects in the model. <br>
 *
 * On success, the faults of the targeted objects are updated in the model. <br>
 * If the RE reported faults for objects not present in the model image in the REC, the model is internally updated and
 * the new objects created (this will be the case at initial software alignment, as the ORI GS specifies a Fault
 * Reporting "ALL objects" shall be done at start-up).
 * If @p eventDrivenReport is set to ::ORI_EventDrivenReport_True or ::ORI_EventDrivenReport_False the event driven fault reporting of the object is enabled
 * or disabled. Pass ::ORI_EventDrivenReport_NoModify to keep the event driven fault reporting as is.
 *
 * @param ori The ORI context.
 * @param object The target object, or parent object, or @c NULL if no specific object; depending on @p wildcard.
 * @param wildcard Wildcard for targeting all objects, or all children objects; depending on @p object being @c NULL.
 * @param eventDrivenReport Set event driven fault reporting of the object if applicable.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_ObjectFaultReport(ORI_s * ori, ORI_Object_s * object, int wildcard, ORI_EventDrivenReport_e eventDrivenReport, ORI_Result_e * RE_result);


/*-----------------------------------------------------------------------------------------
 * LOGGING
 *-----------------------------------------------------------------------------------------*/

/**
 * @brief Perform a File Upload procedure.
 *
 * This function performs a File Upload procedure with the RE or with a Log Object. <br>
 * If successful, a File Upload should be initiated. <br>
 * On completion of the process, the RE shall send a message indicating file upload completion. If a callback is specified in the ORI context,
 * the callback ::ORI_IndCallback_f will be called with the enum ::ORI_IndicationType_UploadFileCmpl
 * passed as parameter, as well as the file upload completion result.
 *
 * @param ori The ORI context.
 * @param object The object targeted for the file upload. Must be a RE or Log object.
 * @param ftpSrvIpAddress The FTP server address as a decimal point notation (e.g. "192.168.1.1").
 * @param ftpSrvUserName The FTP server login used by the RE.
 * @param ftpSrvPassword The FTP server password used by the RE.
 * @param ftpSrvFilePath The FTP server path to the file to upload.
 * @param REFilePath The RE file path for a generic file upload. Used only in the case of a RE object.
 * @param maxUploadFileSize The maximum size in kBytes accepted for uploading the file. Pass 0 for no size limit.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_FileUpload(ORI_s * ori, ORI_Object_s * object, const char * ftpSrvIpAddress, const char * ftpSrvUserName, const char * ftpSrvPassword,
										const char * ftpSrvFilePath, const char * REFilePath,  uint16_t maxUploadFileSize, ORI_Result_e * RE_result);


/*-----------------------------------------------------------------------------------------
 * AISG
 *-----------------------------------------------------------------------------------------*/

/**
 * @brief Perform an AISG Device Scan procedure.
 *
 * This function performs an AISG Scan Device procedure with an AISG Port Object. <br>
 * On completion of the process, the RE shall send a message indicating scan device completion.
 *
 * @param ori The ORI context.
 * @param object The object targeted for the device scan. Must be an AISG Port object.
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_DeviceScan(ORI_s * ori, ORI_Object_s * object, ORI_Result_e * RE_result);

/**
 * @brief Transfer an AISG layer7 message to an ALD.
 *
 * This function transfers an AISG layer7 message to an ALD.
 * On completion of the transfer, the RE shall send a message indicating the ALD's layer7 response.
 *
 * @param ori The ORI context.
 * @param object The object targeted for the layer7 message. Must be an AISG ALD object.
 * @param msgLayer7 The AISG layer7 message.
 * @param deviceDataFieldNbr The field number to store for GetDeviceData message (used in indication parsing).
 * @param RE_result The RE response result of the procedure.
 * @return The result of the procedure.
 */
ORI_Result_e ORI_MsgTransfer(ORI_s * ori, ORI_Object_s * object, const char * msgLayer7, uint8_t deviceDataFieldNbr, ORI_Result_e * RE_result);



/*-----------------------------------------------------------------------------------------
 * ORI MODEL
 *-----------------------------------------------------------------------------------------*/

/**
 * @brief Find an object in the model.
 *
 * This function seeks in the model an object that matches the object type @p type
 * and the instance number @p instanceNumber. <br>
 * If @p parent is @c NULL, only parent-less objects will be in the search. Else, the object
 * to find shall be a direct descendant of @p parent.
 *
 * @param ori The ORI context.
 * @param type The object type of the object to find.
 * @param instanceNumber The instance number of the object to find.
 * @param parent The parent of the object to find, or @c NULL if no parent is expected.
 * @return The found object if any, or @c NULL if none.
 */
ORI_Object_s * ORI_FindObject(ORI_s * ori, ORI_ObjectType_e type, uint8_t instanceNumber, ORI_Object_s * parent);

/**
 * @brief Gets all objects in the model
 *
 * This function fills the array @p objects with reference of all the objects in the model that are a child of @p parent. If @p parent is @c NULL,
 * all the objects in the model are retrieved. <br>
 * The maximum size of the @p objects array shall be given in @p maxObjects. <br>
 *
 * @param ori The ORI context.
 * @param objects The object array to fill.
 * @param maxObjects The maximum number of objects that the array can contain.
 * @param parent If not @c NULL, specifies the parent of the objects to get.
 * @return The number of objects found.
 */
uint32_t ORI_GetAllObjects(ORI_s * ori, ORI_Object_s * objects[], uint32_t maxObjects, ORI_Object_s * parent);

/**
 * @brief Gets all objects of a given type in the model
 *
 * This function fills the array @p objects with reference of all the objects of type @p type in the model that are a child of @p parent. If @p parent is @c NULL,
 * all the objects of type @p type in the model are retrieved. <br>
 * The maximum size of the @p objects array shall be given in @p maxObjects. <br>
 * This functions is the same as ORI_GetAllObjects() but with a type filter.
 *
 * @param ori The ORI context.
 * @param type The type filter of the objects to find.
 * @param objects The object array to fill.
 * @param maxObjects The maximum number of objects that the array can contain.
 * @param parent If not @c NULL, specifies the parent of the objects to get.
 * @return The number of objects found.
 */
uint32_t ORI_GetAllObjectsOfType(ORI_s * ori, ORI_ObjectType_e type, ORI_Object_s * objects[], uint32_t maxObjects, ORI_Object_s * parent);

/**
 * @brief Return the fault structure of an object based on a fault type.
 *
 * This function is a helper function that returns a pointer to a fault structure of an object based on a fault type.
 * If the fault type and the object type does not match (as faults are specific to some objects), @c NULL is returned.
 *
 * @param ori The ORI context.
 * @param object The object to delete.
 * @param faultType The type of the fault structure to return.
 * @return The found fault structure, or @c NULL if none.
 */
ORI_Fault_s * ORI_ObjectFault(ORI_s * ori, ORI_Object_s * object, ORI_FaultType_e faultType);




#endif /* ORI_H_ */
