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
 *      conmnc_digit_lengtht@openairinterface.org
 */

/*! \file RRC/LTE/defs.h
* \brief RRC struct definitions and function prototypes
* \author Navid Nikaein and Raymond Knopp
* \date 2010 - 2014
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr
*/

#ifndef __OPENAIR_RRC_DEFS_H__
#define __OPENAIR_RRC_DEFS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "collection/tree.h"
#include "common/ngran_types.h"
#include "rrc_types.h"
//#include "PHY/phy_defs.h"
#include "LAYER2/RLC/rlc.h"
#include "RRC/NR/nr_rrc_types.h"
#include "NR_UE-MRDC-Capability.h"
#include "NR_UE-NR-Capability.h"


#include "COMMON/platform_constants.h"
#include "COMMON/platform_types.h"

#include "LAYER2/MAC/mac.h"

//for D2D
#define DEBUG_CTRL_SOCKET

#define CONTROL_SOCKET_PORT_NO 8888
#define MAX_NUM_DEST           10
//netlink
//#define DEBUG_PDCP
#define UE_IP_PDCP_NETLINK_ID  31
#define PDCP_PID               1
#define NETLINK_HEADER_SIZE    16
#define SL_DEFAULT_RAB_ID      3
#define SLRB_ID                3

#define MAX_PAYLOAD 1024 /* maximum payload size*/

#define MAX_NUM_NEIGH_CELLs 6 /* maximum neighbouring cells number */

#define MAX_NUM_GNB_CELLs 1   /* maximum gNB cells number */

#define UE_STATE_NOTIFICATION_INTERVAL      50

#define IPV4_ADDR    "%u.%u.%u.%u"
#define IPV4_ADDR_FORMAT(aDDRESS)                 \
  (uint8_t)((aDDRESS)  & 0x000000ff),         \
  (uint8_t)(((aDDRESS) & 0x0000ff00) >> 8 ),  \
  (uint8_t)(((aDDRESS) & 0x00ff0000) >> 16),  \
  (uint8_t)(((aDDRESS) & 0xff000000) >> 24)


//-----------------------------------------------------
// header for Control socket

//Primitives
#define SESSION_INIT_REQ                    1
#define UE_STATUS_INFO                      2
#define GROUP_COMMUNICATION_ESTABLISH_REQ   3
#define GROUP_COMMUNICATION_ESTABLISH_RSP   4
#define DIRECT_COMMUNICATION_ESTABLISH_REQ  5
#define DIRECT_COMMUNICATION_ESTABLISH_RSP  6
#define GROUP_COMMUNICATION_RELEASE_REQ     7
#define GROUP_COMMUNICATION_RELEASE_RSP     8
#define PC5S_ESTABLISH_REQ                  9
#define PC5S_ESTABLISH_RSP                  10
#define PC5_DISCOVERY_MESSAGE             11


#define PC5_DISCOVERY_PAYLOAD_SIZE      29

#define DEBUG_SCG_CONFIG 1

typedef enum {
  UE_STATE_OFF_NETWORK,
  UE_STATE_ON_NETWORK
} SL_UE_STATE_t;

typedef enum {
  GROUP_COMMUNICATION_RELEASE_OK = 0,
  GROUP_COMMUNICATION_RELEASE_FAILURE
} Group_Communication_Status_t;

struct GroupCommunicationEstablishReq {
  uint32_t sourceL2Id;
  uint32_t groupL2Id;
  uint32_t groupIpAddress;
  uint8_t pppp;
};

struct GroupCommunicationReleaseReq {
  uint32_t sourceL2Id;
  uint32_t groupL2Id;
  int slrb_id;
};

struct DirectCommunicationEstablishReq {
  uint32_t sourceL2Id;
  uint32_t destinationL2Id;
  uint32_t pppp;
};

struct PC5SEstablishReq {
  uint8_t type;
  uint32_t sourceL2Id;
  uint32_t destinationL2Id;
};

struct PC5SEstablishRsp {
  uint32_t slrbid_lcid28;
  uint32_t slrbid_lcid29;
  uint32_t slrbid_lcid30;
};


//PC5_DISCOVERY MESSAGE
typedef struct  {
  unsigned char payload[PC5_DISCOVERY_PAYLOAD_SIZE];
  uint32_t measuredPower;
}  __attribute__((__packed__)) PC5DiscoveryMessage ;


struct sidelink_ctrl_element {
  unsigned short type;
  union {
    struct GroupCommunicationEstablishReq group_comm_establish_req;
    struct DirectCommunicationEstablishReq direct_comm_establish_req;
    Group_Communication_Status_t group_comm_release_rsp;
    //struct DirectCommunicationReleaseReq  direct_comm_release_req;
    SL_UE_STATE_t ue_state;
    int slrb_id;
    struct PC5SEstablishReq pc5s_establish_req;
    struct PC5SEstablishRsp pc5s_establish_rsp;
    PC5DiscoveryMessage pc5_discovery_message;
  } sidelinkPrimitive;
};


//global variables
extern struct sockaddr_in clientaddr;
extern int slrb_id;
extern pthread_mutex_t slrb_mutex;

//the thread function
void *send_UE_status_notification(void *);

#include "LTE_SystemInformationBlockType1.h"
#include "LTE_SystemInformation.h"
#include "LTE_RRCConnectionReconfiguration.h"
#include "LTE_RRCConnectionReconfigurationComplete.h"
#include "LTE_RRCConnectionSetup.h"
#include "LTE_RRCConnectionSetupComplete.h"
#include "LTE_RRCConnectionRequest.h"
#include "LTE_RRCConnectionReestablishmentRequest.h"
#include "LTE_BCCH-DL-SCH-Message.h"
#include "LTE_SBCCH-SL-BCH-MessageType.h"
#include "LTE_BCCH-BCH-Message.h"
#include "LTE_MCCH-Message.h"
#include "LTE_MBSFNAreaConfiguration-r9.h"
#include "LTE_SCellToAddMod-r10.h"
#include "LTE_AS-Config.h"
#include "LTE_AS-Context.h"
#include "LTE_UE-EUTRA-Capability.h"
#include "LTE_MeasResults.h"
#include "LTE_SidelinkUEInformation-r12.h"


/* for ImsiMobileIdentity_t */
#include "MobileIdentity.h"

#include "LTE_DRX-Config.h"

/* correct Rel(8|10)/Rel14 differences
 * the code is in favor of Rel14, those defines do the translation
 */



#define NB_SIG_CNX_CH 1
#define NB_CNX_CH MAX_MOBILES_PER_ENB
#define NB_SIG_CNX_UE 2 //MAX_MANAGED_RG_PER_MOBILE
#define NB_CNX_UE 2//MAX_MANAGED_RG_PER_MOBILE


#include "intertask_interface.h"




#include "commonDef.h"


//--------
typedef unsigned int uid_t;
#define UID_LINEAR_ALLOCATOR_BITMAP_SIZE (((MAX_MOBILES_PER_ENB/8)/sizeof(unsigned int)) + 1)
typedef struct uid_linear_allocator_s {
  unsigned int   bitmap[UID_LINEAR_ALLOCATOR_BITMAP_SIZE];
} uid_allocator_t;

//--------



#define PROTOCOL_RRC_CTXT_UE_FMT           PROTOCOL_CTXT_FMT
#define PROTOCOL_RRC_CTXT_UE_ARGS(CTXT_Pp) PROTOCOL_CTXT_ARGS(CTXT_Pp)

#define PROTOCOL_RRC_CTXT_FMT           PROTOCOL_CTXT_FMT
#define PROTOCOL_RRC_CTXT_ARGS(CTXT_Pp) PROTOCOL_CTXT_ARGS(CTXT_Pp)
/** @defgroup _rrc RRC
 * @ingroup _oai2
 * @{
 */


#define UE_MODULE_INVALID ((module_id_t) ~0) // FIXME attention! depends on type uint8_t!!!
#define UE_INDEX_INVALID  ((module_id_t) ~0) // FIXME attention! depends on type uint8_t!!! used to be -1

typedef enum {
  RRC_OK=0,
  RRC_ConnSetup_failed,
  RRC_PHY_RESYNCH,
  RRC_Handover_failed,
  RRC_HO_STARTED
} RRC_status_t;

typedef enum UE_STATE_e {
  RRC_INACTIVE=0,
  RRC_IDLE,
  RRC_SI_RECEIVED,
  RRC_CONNECTED,
  RRC_RECONFIGURED,
  RRC_HO_EXECUTION,
  RRC_NR_NSA,
  RRC_NR_NSA_RECONFIGURED,
  RRC_NR_NSA_DELETED
} UE_STATE_t;

typedef enum HO_STATE_e {
  HO_IDLE=0,
  HO_MEASUREMENT,
  HO_PREPARE,
  HO_CMD, // initiated by the src eNB
  HO_COMPLETE, // initiated by the target eNB
  HO_REQUEST,
  HO_ACK,
  HO_FORWARDING,
  HO_CONFIGURED,
  HO_END_MARKER,
  HO_FORWARDING_COMPLETE,
  HO_RELEASE,
  HO_CANCEL
} HO_STATE_t;

typedef enum DATA_FORWARDING_STATE_e {
  FORWARDING_EMPTY=0,
  FORWARDING_NO_EMPTY
} DATA_FORWARDING_STATE_t;

typedef enum DATA_ENDMARK_STATE_e {
  ENDMARK_EMPTY=0,
  ENDMARK_NO_EMPTY
} DATA_ENDMARK_STATE_t;

typedef enum SL_TRIGGER_e {
  SL_RECEIVE_COMMUNICATION=0,
  SL_TRANSMIT_RELAY_ONE_TO_ONE,
  SL_TRANSMIT_RELAY_ONE_TO_MANY,
  SL_TRANSMIT_NON_RELAY_ONE_TO_ONE,
  SL_TRANSMIT_NON_RELAY_ONE_TO_MANY,
  SL_RECEIVE_DISCOVERY,
  SL_TRANSMIT_NON_PS_DISCOVERY,
  SL_TRANSMIT_PS_DISCOVERY,
  SL_RECEIVE_V2X,
  SL_TRANSMIT_V2X,
  SL_REQUEST_DISCOVERY_TRANSMISSION_GAPS,
  SL_REQUEST_DISCOVERY_RECEPTION_GAPS
} SL_TRIGGER_t;

//#define MAX_MOBILES_PER_ENB MAX_MOBILES_PER_RG
#define RRM_FREE(p)       if ( (p) != NULL) { free(p) ; p=NULL ; }
#define RRM_MALLOC(t,n)   (t *) malloc16( sizeof(t) * n )
#define RRM_CALLOC(t,n)   (t *) malloc16( sizeof(t) * n)
#define RRM_CALLOC2(t,s)  (t *) malloc16( s )

#define MAX_MEAS_OBJ 7
#define MAX_MEAS_CONFIG 7
#define MAX_MEAS_ID 7

#define PAYLOAD_SIZE_MAX 1024
#define RRC_BUF_SIZE 512
#define UNDEF_SECURITY_MODE 0xff
#define NO_SECURITY_MODE 0x20

#define CBA_OFFSET        0xfff4
// #define NUM_MAX_CBA_GROUP 4 // in the platform_constants

/* TS 36.331: RRC-TransactionIdentifier ::= INTEGER (0..3) */
#define RRC_TRANSACTION_IDENTIFIER_NUMBER  3

typedef struct {
  unsigned short transport_block_size;   /*!< \brief Minimum PDU size in bytes provided by RLC to MAC layer interface */
  unsigned short max_transport_blocks;   /*!< \brief Maximum PDU size in bytes provided by RLC to MAC layer interface */
  unsigned long  Guaranteed_bit_rate;    /*!< \brief Guaranteed Bit Rate (average) to be offered by MAC layer scheduling*/
  unsigned long  Max_bit_rate;           /*!< \brief Maximum Bit Rate that can be offered by MAC layer scheduling*/
  uint8_t  Delay_class;                  /*!< \brief Delay class offered by MAC layer scheduling*/
  uint8_t  Target_bler;                  /*!< \brief Target Average Transport Block Error rate*/
  uint8_t  Lchan_t;                      /*!< \brief Logical Channel Type (BCCH,CCCH,DCCH,DTCH_B,DTCH,MRBCH)*/
} __attribute__ ((__packed__))  LCHAN_DESC;

#define LCHAN_DESC_SIZE sizeof(LCHAN_DESC)

typedef struct UE_RRC_INFO_s {
  UE_STATE_t State;
  uint8_t SIB1systemInfoValueTag;
  uint32_t SIStatus;
  uint32_t SIcnt;
  uint8_t SIB1systemInfoValueTag_MBMS;
  uint32_t SIStatus_MBMS;
  uint32_t SIcnt_MBMS;
  uint8_t MCCHStatus[8]; // MAX_MBSFN_AREA
  uint8_t SIwindowsize; //!< Corresponds to the SIB1 si-WindowLength parameter. The unit is ms. Possible values are (final): 1,2,5,10,15,20,40
  uint8_t SIwindowsize_MBMS; //!< Corresponds to the SIB1 si-WindowLength parameter. The unit is ms. Possible values are (final): 1,2,5,10,15,20,40
  uint8_t handoverTarget;
  HO_STATE_t ho_state;
  uint16_t SIperiod ; //!< Corresponds to the SIB1 si-Periodicity parameter (multiplied by 10). Possible values are (final): 80,160,320,640,1280,2560,5120
  uint16_t SIperiod_MBMS; //!< Corresponds to the SIB1-MBMS si-Periodicity parameter (multiplied by 10). Possible values are (final): 80,160,320,640,1280,2560,5120 TODO
  unsigned short UE_index;
  uint32_t T300_active;
  uint32_t T300_cnt;
  uint32_t T304_active;
  uint32_t T304_cnt;
  uint32_t T310_active;
  uint32_t T310_cnt;
  uint32_t N310_cnt;
  uint32_t N311_cnt;
  rnti_t   rnti;
  struct LTE_DL_DCCH_Message *dl_dcch_msg;
} __attribute__ ((__packed__)) UE_RRC_INFO;

typedef struct UE_S_TMSI_s {
  boolean_t  presence;
  mme_code_t mme_code;
  m_tmsi_t   m_tmsi;
} __attribute__ ((__packed__)) UE_S_TMSI;


typedef enum e_rab_satus_e {
  E_RAB_STATUS_NEW,
  E_RAB_STATUS_DONE, // from the eNB perspective
  E_RAB_STATUS_ESTABLISHED, // get the reconfigurationcomplete form UE
  E_RAB_STATUS_REESTABLISHED, // after HO
  E_RAB_STATUS_TOMODIFY,      // ENDC NSA
  E_RAB_STATUS_FAILED,
  E_RAB_STATUS_TORELEASE  // to release DRB between eNB and UE
} e_rab_status_t;

typedef struct e_rab_param_s {
  e_rab_t param;
  uint8_t status;
  uint8_t xid; // transaction_id
  s1ap_Cause_t cause;
  uint8_t cause_value;
} __attribute__ ((__packed__)) e_rab_param_t;


/* Intermediate structure for Handover management. Associated per-UE in eNB_RRC_INST */
typedef struct HANDOVER_INFO_s {
  uint8_t ho_prepare;
  uint8_t ho_complete;
  HO_STATE_t state; //current state of handover
  uint32_t modid_s; //module_idP of serving cell
  uint32_t modid_t; //module_idP of target cell
  int assoc_id;
  uint8_t ueid_s; //UE index in serving cell
  uint8_t ueid_t; //UE index in target cell
  LTE_AS_Config_t as_config; /* these two parameters are taken from 36.331 section 10.2.2: HandoverPreparationInformation-r8-IEs */
  LTE_AS_Context_t as_context; /* They are mandatory for HO */
  uint8_t buf[RRC_BUF_SIZE];  /* ASN.1 encoded handoverCommandMessage */
  int size;   /* size of above message in bytes */
  int x2_id;   /* X2AP UE ID in the target eNB */
  uint32_t x2u_teid;
  DATA_FORWARDING_STATE_t forwarding_state;
  DATA_ENDMARK_STATE_t endmark_state;
} HANDOVER_INFO;

typedef struct PER_EVENT_s {
  long maxReportCells;
} PER_EVENT_t;

typedef struct A1_EVENT_s {
  long threshold_RSRP;
  long hysteresis;
  long timeToTrigger;
  long maxReportCells;
} A1_EVENT_t;

typedef struct A2_EVENT_s {
  long threshold_RSRP;
  long hysteresis;
  long timeToTrigger;
  long maxReportCells;
} A2_EVENT_t;

typedef struct A3_EVENT_s {
  long a3_offset;
  int reportOnLeave;
  long hysteresis;
  long timeToTrigger;
  long maxReportCells;
} A3_EVENT_t;


typedef struct A4_EVENT_s {
  long threshold_RSRP;
  long hysteresis;
  long timeToTrigger;
  long maxReportCells;
} A4_EVENT_t;

typedef struct A5_EVENT_s {
  long threshold_RSRP_1;
  long threshold_RSRP_2;
  long hysteresis;
  long timeToTrigger;
  long maxReportCells;
} A5_EVENT_t;

typedef struct EVENTS_s {
  PER_EVENT_t *per_event;

  A1_EVENT_t *a1_event;

  A2_EVENT_t *a2_event;

  A3_EVENT_t *a3_event;

  A4_EVENT_t *a4_event;

  A5_EVENT_t *a5_event;
} EVENTS_t;

typedef struct MEASUREMENT_INFO_s {
  //TODO: Extend to multiple meas objects for OFP/OFN offsets
  long  offsetFreq;
  //TODO: extend to multiple carriers for OCP/OCN offsets
  long cellIndividualOffset[MAX_NUM_NEIGH_CELLs+1];
  long filterCoefficientRSRP;
  long filterCoefficientRSRQ;
  EVENTS_t *events;
} MEASUREMENT_INFO;


#define RRC_HEADER_SIZE_MAX 64
#define RRC_BUFFER_SIZE_MAX 1024
typedef struct {
  char Payload[RRC_BUFFER_SIZE_MAX];
  char Header[RRC_HEADER_SIZE_MAX];
  uint16_t payload_size;
} RRC_BUFFER;
#define RRC_BUFFER_SIZE sizeof(RRC_BUFFER)

typedef struct RB_INFO_s {
  uint16_t Rb_id;  //=Lchan_id
  LCHAN_DESC Lchan_desc[2];
  //  MAC_MEAS_REQ_ENTRY *Meas_entry;
} RB_INFO;

typedef struct SRB_INFO_s {
  uint16_t Srb_id;  //=Lchan_id
  RRC_BUFFER Rx_buffer;
  RRC_BUFFER Tx_buffer;
  LCHAN_DESC Lchan_desc[2];
  unsigned int Trans_id;
  uint8_t Active;
} SRB_INFO;

typedef struct RB_INFO_TABLE_ENTRY_s {
  RB_INFO Rb_info;
  uint8_t Active;
  uint32_t Next_check_frame;
  uint8_t StatusRb;
} RB_INFO_TABLE_ENTRY;

typedef struct SRB_INFO_TABLE_ENTRY_s {
  SRB_INFO Srb_info;
  uint8_t Active;
  uint8_t StatusSrb;
  uint32_t Next_check_frame;
} SRB_INFO_TABLE_ENTRY;

typedef struct MEAS_REPORT_LIST_s {
  LTE_MeasId_t measId;
  //CellsTriggeredList  cellsTriggeredList;//OPTIONAL
  uint32_t numberOfReportsSent;
} MEAS_REPORT_LIST;

typedef struct HANDOVER_INFO_UE_s {
  LTE_PhysCellId_t targetCellId;
  uint8_t measFlag;
} HANDOVER_INFO_UE;

typedef struct rrc_gummei_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_len;
  uint8_t  mme_code;
  uint16_t mme_group_id;
} rrc_gummei_t;

typedef struct {
  uint16_t ciphering_algorithms;
  uint16_t integrity_algorithms;
  uint16_t sk_counter;
  uint8_t  kgNB[32];
} lte_rrc_nr_security_t;

typedef struct eNB_RRC_UE_s {
  uint8_t                            primaryCC_id;
  LTE_SCellToAddMod_r10_t            sCell_config[2];
  LTE_SRB_ToAddModList_t            *SRB_configList;
  LTE_SRB_ToAddModList_t            *SRB_configList2[RRC_TRANSACTION_IDENTIFIER_NUMBER];
  LTE_DRB_ToAddModList_t            *DRB_configList;
  LTE_DRB_ToAddModList_t            *DRB_configList2[RRC_TRANSACTION_IDENTIFIER_NUMBER];
  LTE_DRB_ToReleaseList_t           *DRB_Release_configList2[RRC_TRANSACTION_IDENTIFIER_NUMBER];
  uint8_t                            DRB_active[NB_RB_MAX -2];
  struct LTE_PhysicalConfigDedicated    *physicalConfigDedicated;
  struct LTE_SPS_Config             *sps_Config;
  LTE_MeasObjectToAddMod_t          *MeasObj[MAX_MEAS_OBJ];
  struct LTE_ReportConfigToAddMod   *ReportConfig[MAX_MEAS_CONFIG];
  struct LTE_QuantityConfig         *QuantityConfig;
  struct LTE_MeasIdToAddMod         *MeasId[MAX_MEAS_ID];
  LTE_MAC_MainConfig_t              *mac_MainConfig;
  LTE_MeasGapConfig_t               *measGapConfig;
  SRB_INFO                           SI;
  SRB_INFO                           Srb0;
  SRB_INFO_TABLE_ENTRY               Srb1;
  SRB_INFO_TABLE_ENTRY               Srb2;
  LTE_MeasConfig_t                  *measConfig;
  HANDOVER_INFO                     *handover_info;
  MEASUREMENT_INFO                  *measurement_info;
  LTE_MeasResults_t                 *measResults;
  LTE_MobilityControlInfo_t         *mobilityInfo;

  LTE_UE_EUTRA_Capability_t         *UE_Capability;
  int                                UE_Capability_size;
  NR_UE_MRDC_Capability_t           *UE_Capability_MRDC;
  int                UE_MRDC_Capability_size;
  NR_UE_NR_Capability_t       *UE_Capability_nr;
  int                UE_NR_Capability_size;
  ImsiMobileIdentity_t               imsi;

  /* KeNB as derived from KASME received from EPC */
  uint8_t kenb[32];
  int8_t  kenb_ncc;
  uint8_t nh[32];
  int8_t  nh_ncc;
  /* Used integrity/ciphering algorithms */
  LTE_CipheringAlgorithm_r12_t                          ciphering_algorithm;
  e_LTE_SecurityAlgorithmConfig__integrityProtAlgorithm integrity_algorithm;

  uint8_t                            StatusRrc; // RRC status, type enum UE_STATE_t
  rnti_t                             rnti;
  int                                gnb_rnti;     //RNTI of the UE at the gNB if in ENDC connection
  int                                gnb_x2_assoc_id;
  uint64_t                           random_ue_identity;


  /* Information from UE RRC ConnectionRequest */
  UE_S_TMSI                          Initialue_identity_s_TMSI;
  LTE_EstablishmentCause_t           establishment_cause;

  /* Information from UE RRC ConnectionReestablishmentRequest */
  LTE_ReestablishmentCause_t         reestablishment_cause;

  /* UE id for initial connection to S1AP */
  uint16_t                           ue_initial_id;

  /* Information from S1AP initial_context_setup_req */
  uint32_t                           eNB_ue_s1ap_id :24;

  uint32_t                           mme_ue_s1ap_id;
  rrc_gummei_t                       ue_gummei;

  security_capabilities_t            security_capabilities;

  /* security capabilities and settings for an UE in ENDC mode */
  lte_rrc_nr_security_t              nr_security;

  int                                next_hop_chain_count;

  uint8_t                            next_security_key[SECURITY_KEY_LENGTH];

  /* Total number of e_rab already setup in the list */
  uint8_t                            setup_e_rabs;
  /* Number of e_rab to be setup in the list */
  uint8_t                            nb_of_e_rabs;
  /* Number of e_rab to be modified in the list */
  uint8_t                            nb_of_modify_e_rabs;
  uint8_t                            nb_of_failed_e_rabs;
  uint8_t              nb_of_modify_endc_e_rabs;
  e_rab_param_t                      modify_e_rab[NB_RB_MAX];//[S1AP_MAX_E_RAB];
  /* list of e_rab to be setup by RRC layers */
  e_rab_param_t                      e_rab[NB_RB_MAX];//[S1AP_MAX_E_RAB];
  /* UE aggregate maximum bitrate */
  ambr_t ue_ambr;
  //release e_rabs
  uint8_t                            nb_release_of_e_rabs;
  /* list of e_rab to be released by RRC layers */
  uint8_t                            e_rabs_tobereleased[NB_RB_MAX];
  e_rab_failed_t                     e_rabs_release_failed[S1AP_MAX_E_RAB];
  // LG: For GTPV1 TUNNELS
  uint32_t                           enb_gtp_teid[S1AP_MAX_E_RAB];
  transport_layer_addr_t             enb_gtp_addrs[S1AP_MAX_E_RAB];
  rb_id_t                            enb_gtp_ebi[S1AP_MAX_E_RAB];
  // LG: For GTPV1 TUNNELS ENDC
  uint32_t                           gnb_gtp_endc_teid[S1AP_MAX_E_RAB];
  transport_layer_addr_t             gnb_gtp_endc_addrs[S1AP_MAX_E_RAB];
  rb_id_t                            gnb_gtp_endc_ebi[S1AP_MAX_E_RAB];
  /* Total number of e_rab already setup in the list */
  uint8_t                            nb_x2u_e_rabs;
  // LG: For GTPV1 TUNNELS(X2U)
  uint32_t                           enb_gtp_x2u_teid[S1AP_MAX_E_RAB];
  transport_layer_addr_t             enb_gtp_x2u_addrs[S1AP_MAX_E_RAB];
  rb_id_t                            enb_gtp_x2u_ebi[S1AP_MAX_E_RAB];
  uint32_t                           ul_failure_timer;
  uint32_t                           ue_release_timer;
  uint32_t                           ue_release_timer_thres;
  uint32_t                           ue_release_timer_s1;
  uint32_t                           ue_release_timer_thres_s1;
  uint32_t                           ue_release_timer_rrc;
  uint32_t                           ue_release_timer_thres_rrc;
  uint32_t                           ue_reestablishment_timer;
  uint32_t                           ue_reestablishment_timer_thres;
  /* RRC inactivity timer: on timeout, should release RRC connection for inactivity on all E-RABs */
  uint32_t                           ue_rrc_inactivity_timer;
  uint8_t                            e_rab_release_command_flag;
  int8_t                             reestablishment_xid;
  int                                does_nr;
  int                                nr_capabilities_requested;
} eNB_RRC_UE_t;

typedef uid_t ue_uid_t;

typedef struct rrc_eNB_ue_context_s {
  /* Tree related data */
  RB_ENTRY(rrc_eNB_ue_context_s) entries;

  /* Uniquely identifies the UE between MME and eNB within the eNB.
   * This id is encoded on 24bits.
   */
  rnti_t         ue_id_rnti;

  // another key for protocol layers but should not be used as a key for RB tree
  ue_uid_t       local_uid;

  /* UE id for initial connection to S1AP */
  struct eNB_RRC_UE_s   ue_context;
} rrc_eNB_ue_context_t;

typedef struct {
  uint8_t                           *MIB;
  uint8_t                           sizeof_MIB;
  uint8_t                           *SIB1;
  uint8_t                           sizeof_SIB1;
  uint8_t                           *SIB23;
  uint8_t                           sizeof_SIB23;
  uint8_t                           *SIB1_BR;
  uint8_t                           sizeof_SIB1_BR;
  uint8_t                           *SIB23_BR;
  uint8_t                           sizeof_SIB23_BR;
  uint8_t                           *MIB_FeMBMS;
  uint8_t                           sizeof_MIB_FeMBMS;
  uint8_t                           *SIB1_MBMS;
  uint8_t                           sizeof_SIB1_MBMS;
  int                                   physCellId;
  int                                   Ncp;
  int                                   p_eNB;
  uint32_t                              dl_CarrierFreq;
  uint32_t                              ul_CarrierFreq;
  uint32_t                              eutra_band;
  uint32_t                              N_RB_DL;
  uint32_t                              pbch_repetition;
  LTE_BCCH_BCH_Message_t                mib;
  LTE_BCCH_BCH_Message_t                *mib_DU;
  LTE_BCCH_DL_SCH_Message_t             siblock1;
  LTE_BCCH_DL_SCH_Message_t             siblock1_BR;
  LTE_BCCH_DL_SCH_Message_t             *siblock1_DU;
  LTE_BCCH_DL_SCH_Message_t             systemInformation;
  LTE_BCCH_DL_SCH_Message_t             systemInformation_BR;
  LTE_BCCH_BCH_Message_MBMS_t            mib_fembms;
  LTE_BCCH_DL_SCH_Message_MBMS_t         siblock1_MBMS;
  LTE_BCCH_DL_SCH_Message_MBMS_t         systemInformation_MBMS;
  LTE_SchedulingInfo_MBMS_r14_t 	 schedulingInfo_MBMS;
  LTE_PLMN_IdentityInfo_t		 PLMN_identity_info_MBMS[6];
  LTE_MCC_MNC_Digit_t			 dummy_mcc_MBMS[6][3], dummy_mnc_MBMS[6][3];
  LTE_SystemInformationBlockType1_t     *sib1;
  LTE_SystemInformationBlockType2_t     *sib2;
  LTE_SystemInformationBlockType3_t     *sib3;
  LTE_SystemInformationBlockType1_t     *sib1_BR;
  LTE_SystemInformationBlockType2_t     *sib2_BR;
  LTE_SystemInformationBlockType1_MBMS_r14_t *sib1_MBMS;
  LTE_SystemInformationBlockType13_r9_t *sib13_MBMS;
  uint8_t       FeMBMS_flag;
  LTE_SystemInformationBlockType13_r9_t *sib13;
  uint8_t                           MBMS_flag;
  uint8_t                           num_mbsfn_sync_area;
  uint8_t                           **MCCH_MESSAGE; //  MAX_MBSFN_AREA
  uint8_t                           sizeof_MCCH_MESSAGE[8];// MAX_MBSFN_AREA
  LTE_MCCH_Message_t                mcch;
  LTE_MBSFNAreaConfiguration_r9_t  *mcch_message;
  SRB_INFO                          MCCH_MESS[8];// MAX_MBSFN_AREA
  uint8_t                           **MCCH_MESSAGE_COUNTING; //  MAX_MBSFN_AREA
  uint8_t                           sizeof_MCCH_MESSAGE_COUNTING[8];// MAX_MBSFN_AREA
  LTE_MCCH_Message_t                mcch_counting;
  LTE_MBMSCountingRequest_r10_t    *mcch_message_counting;
  SRB_INFO                          MCCH_MESS_COUNTING[8];// MAX_MBSFN_AREA

  //TTN - SIB 18,19,21 for D2D
  LTE_SystemInformationBlockType18_r12_t *sib18;
  LTE_SystemInformationBlockType19_r12_t *sib19;
  LTE_SystemInformationBlockType21_r14_t *sib21;
  // End - TTN
  SRB_INFO                          SI;
  uint8_t                           *paging[MAX_MOBILES_PER_ENB];
  uint32_t                           sizeof_paging[MAX_MOBILES_PER_ENB];
} rrc_eNB_carrier_data_t;


typedef struct eNB_RRC_INST_s {
  /// southbound midhaul configuration
  ngran_node_t                    node_type;
  eth_params_t                    eth_params_s;
  char                            *node_name;
  uint32_t                        node_id;
  rrc_eNB_carrier_data_t          carrier[MAX_NUM_CCs];
  uid_allocator_t                    uid_allocator; // for rrc_ue_head
  RB_HEAD(rrc_ue_tree_s, rrc_eNB_ue_context_s)     rrc_ue_head; // ue_context tree key search by rnti
  uint8_t                           HO_flag;
  uint8_t                            Nb_ue;
  hash_table_t                      *initial_id2_s1ap_ids; // key is    content is rrc_ue_s1ap_ids_t
  hash_table_t                      *s1ap_id2_s1ap_ids   ; // key is    content is rrc_ue_s1ap_ids_t

#ifdef LOCALIZATION
  /// localization type, 0: power based, 1: time based
  uint8_t loc_type;
  /// epoch timestamp in millisecond, RRC
  int32_t reference_timestamp_ms;
  /// aggregate physical states every n millisecond
  int32_t aggregation_period_ms;
  /// localization list for aggregated measurements from PHY
  struct list loc_list;
#endif

  //RRC configuration
  RrcConfigurationReq configuration;

  /// NR cell id
  uint64_t nr_cellid;

  // X2 handover controlled by network
  int x2_ho_net_control;

  // Neighborouring cells id
  int num_neigh_cells;
  int num_neigh_cells_cc[MAX_NUM_CCs];
  uint32_t neigh_cells_id[MAX_NUM_NEIGH_CELLs][MAX_NUM_CCs];

  // gNB cells connected to this eNB
  int num_gnb_cells;
  int num_gnb_cells_cc[MAX_NUM_GNB_CELLs];
  uint32_t gnb_cells_id[MAX_NUM_GNB_CELLs][MAX_NUM_CCs];

  // Nr scc freq band and SSB absolute frequency
  uint32_t nr_gnb_freq_band[MAX_NUM_GNB_CELLs][MAX_NUM_CCs];
  int nr_scg_ssb_freq;

  // other RAN parameters
  int srb1_timer_poll_retransmit;
  int srb1_poll_pdu;
  int srb1_poll_byte;
  int srb1_max_retx_threshold;
  int srb1_timer_reordering;
  int srb1_timer_status_prohibit;
  int srs_enable[MAX_NUM_CCs];
  int cell_info_configured;
  pthread_mutex_t cell_info_mutex;
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;

} eNB_RRC_INST;

#define MAX_UE_CAPABILITY_SIZE 255
typedef struct OAI_UECapability_s {
  uint8_t sdu[MAX_UE_CAPABILITY_SIZE];
  uint8_t sdu_size;
  LTE_UE_EUTRA_Capability_t *UE_EUTRA_Capability;
} OAI_UECapability_t;

typedef struct UE_RRC_INST_s {
  Rrc_State_t     RrcState;
  Rrc_Sub_State_t RrcSubState;
  plmn_t          plmnID;
  Byte_t          rat;
  uint8_t         selected_plmn_identity;
  as_nas_info_t   initialNasMsg;
  OAI_UECapability_t *UECap;
  uint8_t *UECapability;
  uint8_t UECapability_size;
  UE_RRC_INFO Info[NB_SIG_CNX_UE];
  SRB_INFO Srb0[NB_SIG_CNX_UE];
  SRB_INFO_TABLE_ENTRY Srb1[NB_CNX_UE];
  SRB_INFO_TABLE_ENTRY Srb2[NB_CNX_UE];
  HANDOVER_INFO_UE HandoverInfoUe;
  uint8_t *SIB1[NB_CNX_UE];
  uint8_t sizeof_SIB1[NB_CNX_UE];
  uint8_t *SI[NB_CNX_UE];
  uint8_t sizeof_SI[NB_CNX_UE];
  uint8_t SIB1Status[NB_CNX_UE];
  uint8_t SIStatus[NB_CNX_UE];
  uint8_t *SIB1_MBMS[NB_CNX_UE];
  uint8_t sizeof_SIB1_MBMS[NB_CNX_UE];
  uint8_t *SI_MBMS[NB_CNX_UE];
  uint8_t sizeof_SI_MBMS[NB_CNX_UE];
  uint8_t SIB1Status_MBMS[NB_CNX_UE];
  uint8_t SIStatus_MBMS[NB_CNX_UE];
  LTE_SystemInformationBlockType1_t *sib1[NB_CNX_UE];
  LTE_SystemInformationBlockType1_MBMS_r14_t *sib1_MBMS[NB_CNX_UE];
  LTE_SystemInformation_t *si[NB_CNX_UE]; //!< Temporary storage for an SI message. Decoding happens in decode_SI().
  LTE_SystemInformation_MBMS_r14_t *si_MBMS[NB_CNX_UE]; //!< Temporary storage for an SI message. Decoding happens in decode_SI().
  LTE_SystemInformationBlockType2_t *sib2[NB_CNX_UE];
  LTE_SystemInformationBlockType3_t *sib3[NB_CNX_UE];
  LTE_SystemInformationBlockType4_t *sib4[NB_CNX_UE];
  LTE_SystemInformationBlockType5_t *sib5[NB_CNX_UE];
  LTE_SystemInformationBlockType6_t *sib6[NB_CNX_UE];
  LTE_SystemInformationBlockType7_t *sib7[NB_CNX_UE];
  LTE_SystemInformationBlockType8_t *sib8[NB_CNX_UE];
  LTE_SystemInformationBlockType9_t *sib9[NB_CNX_UE];
  LTE_SystemInformationBlockType10_t *sib10[NB_CNX_UE];
  LTE_SystemInformationBlockType11_t *sib11[NB_CNX_UE];
  uint8_t                           *MIB;
  //SIB18
  LTE_SystemInformationBlockType18_r12_t *sib18[NB_CNX_UE];
  LTE_SystemInformationBlockType19_r12_t *sib19[NB_CNX_UE];
  LTE_SystemInformationBlockType21_r14_t *sib21[NB_CNX_UE];

  LTE_SBCCH_SL_BCH_MessageType_t   mib_sl[NB_CNX_UE];
  /// Preconfiguration for Sidelink
  struct LTE_SL_Preconfiguration_r12 *SL_Preconfiguration[NB_CNX_UE];
  //source L2 Id
  uint32_t sourceL2Id;
  //group L2 Id
  uint32_t groupL2Id;
  //current destination
  uint32_t destinationL2Id;
  //List of destinations
  uint32_t destinationList[MAX_NUM_DEST];
  //sl_discovery..
  SRB_INFO SL_Discovery[NB_CNX_UE];
  uint8_t MBMS_flag;
  uint8_t *MCCH_MESSAGE[NB_CNX_UE];
  uint8_t sizeof_MCCH_MESSAGE[NB_CNX_UE];
  uint8_t MCCH_MESSAGEStatus[NB_CNX_UE];
  LTE_MBSFNAreaConfiguration_r9_t       *mcch_message[NB_CNX_UE];
  LTE_SystemInformationBlockType12_r9_t *sib12[NB_CNX_UE];
  LTE_SystemInformationBlockType13_r9_t *sib13[NB_CNX_UE];
  LTE_SystemInformationBlockType13_r9_t *sib13_MBMS[NB_CNX_UE];
  uint8_t           FeMBMS_flag;
  uint8_t                         num_srb;
  struct LTE_SRB_ToAddMod         *SRB1_config[NB_CNX_UE];
  struct LTE_SRB_ToAddMod         *SRB2_config[NB_CNX_UE];
  struct LTE_DRB_ToAddMod         *DRB_config[NB_CNX_UE][8];
  rb_id_t                         *defaultDRB; // remember the ID of the default DRB
  LTE_MeasObjectToAddMod_t        *MeasObj[NB_CNX_UE][MAX_MEAS_OBJ];
  struct LTE_ReportConfigToAddMod *ReportConfig[NB_CNX_UE][MAX_MEAS_CONFIG];
  struct LTE_QuantityConfig       *QuantityConfig[NB_CNX_UE];
  struct LTE_MeasIdToAddMod       *MeasId[NB_CNX_UE][MAX_MEAS_ID];
  MEAS_REPORT_LIST                *measReportList[NB_CNX_UE][MAX_MEAS_ID];
  uint32_t                        measTimer[NB_CNX_UE][MAX_MEAS_ID][6]; // 6 neighboring cells
  LTE_RSRP_Range_t                s_measure;
  struct LTE_MeasConfig__speedStatePars *speedStatePars;
  struct LTE_PhysicalConfigDedicated  *physicalConfigDedicated[NB_CNX_UE];
  struct LTE_SPS_Config           *sps_Config[NB_CNX_UE];
  LTE_MAC_MainConfig_t            *mac_MainConfig[NB_CNX_UE];
  LTE_MeasGapConfig_t             *measGapConfig[NB_CNX_UE];
  double                          filter_coeff_rsrp; // [7] ???
  double                          filter_coeff_rsrq; // [7] ???
  float                           rsrp_db[7];
  float                           rsrq_db[7];
  float                           rsrp_db_filtered[7];
  float                           rsrq_db_filtered[7];
  int                             subframeCount;
  /* KeNB as computed from parameters within USIM card */
  uint8_t kenb[32];
  uint8_t nh[32];
  int8_t  nh_ncc;

  /* Used integrity/ciphering algorithms */
  LTE_CipheringAlgorithm_r12_t                          ciphering_algorithm;
  e_LTE_SecurityAlgorithmConfig__integrityProtAlgorithm integrity_algorithm;
  /// Used for Sidelink Preconfiguration
  LTE_DRB_ToAddModList_t *DRB_configList;
} UE_RRC_INST;

typedef struct UE_PF_PO_s {
  boolean_t enable_flag;  /* flag indicate whether current object is used */
  uint16_t ue_index_value;  /* UE index value */
  uint8_t PF_min;  /* minimal value of Paging Frame (PF) */
  uint8_t PO;  /* Paging Occasion (PO) */
  uint32_t T;  /* DRX cycle */
} UE_PF_PO_t;

#include "rrc_proto.h"

#endif
/** @} */
