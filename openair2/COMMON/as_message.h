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

/*****************************************************************************

Source      as_message.h

Version     0.1

Date        2012/10/18

Product     NAS stack

Subsystem   Application Programming Interface

Author      Frederic Maurel

Description Defines the messages supported by the Access Stratum sublayer
        protocol (usually RRC and S1AP for E-UTRAN) and functions used
        to encode and decode

*****************************************************************************/
#ifndef __AS_MESSAGE_H__
#define __AS_MESSAGE_H__

#include "commonDef.h"
#include "networkDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *              Access Stratum message types
 * --------------------------------------------------------------------------
 */
#define AS_REQUEST  0x0100
#define AS_RESPONSE 0x0200
#define AS_INDICATION   0x0400
#define AS_CONFIRM  0x0800

/*
 * --------------------------------------------------------------------------
 *          Access Stratum message identifiers
 * --------------------------------------------------------------------------
 */

/* Broadcast information */
#define AS_BROADCAST_INFO       0x01
#define AS_BROADCAST_INFO_IND       (AS_BROADCAST_INFO | AS_INDICATION)

/* Cell information relevant for cell selection processing */
#define AS_CELL_INFO            0x02
#define AS_CELL_INFO_REQ        (AS_CELL_INFO | AS_REQUEST)
#define AS_CELL_INFO_CNF        (AS_CELL_INFO | AS_CONFIRM)
#define AS_CELL_INFO_IND        (AS_CELL_INFO | AS_INDICATION)

/* Paging information */
#define AS_PAGING           0x03
#define AS_PAGING_REQ           (AS_PAGING | AS_REQUEST)
#define AS_PAGING_IND           (AS_PAGING | AS_INDICATION)

/* NAS signalling connection establishment */
#define AS_NAS_ESTABLISH        0x04
#define AS_NAS_ESTABLISH_REQ        (AS_NAS_ESTABLISH | AS_REQUEST)
#define AS_NAS_ESTABLISH_IND        (AS_NAS_ESTABLISH | AS_INDICATION)
#define AS_NAS_ESTABLISH_RSP        (AS_NAS_ESTABLISH | AS_RESPONSE)
#define AS_NAS_ESTABLISH_CNF        (AS_NAS_ESTABLISH | AS_CONFIRM)

/* NAS signalling connection release */
#define AS_NAS_RELEASE          0x05
#define AS_NAS_RELEASE_REQ      (AS_NAS_RELEASE | AS_REQUEST)
#define AS_NAS_RELEASE_IND      (AS_NAS_RELEASE | AS_INDICATION)

/* Uplink information transfer */
#define AS_UL_INFO_TRANSFER     0x06
#define AS_UL_INFO_TRANSFER_REQ     (AS_UL_INFO_TRANSFER | AS_REQUEST)
#define AS_UL_INFO_TRANSFER_CNF     (AS_UL_INFO_TRANSFER | AS_CONFIRM)
#define AS_UL_INFO_TRANSFER_IND     (AS_UL_INFO_TRANSFER | AS_INDICATION)

/* Downlink information transfer */
#define AS_DL_INFO_TRANSFER     0x07
#define AS_DL_INFO_TRANSFER_REQ     (AS_DL_INFO_TRANSFER | AS_REQUEST)
#define AS_DL_INFO_TRANSFER_CNF     (AS_DL_INFO_TRANSFER | AS_CONFIRM)
#define AS_DL_INFO_TRANSFER_IND     (AS_DL_INFO_TRANSFER | AS_INDICATION)

/* Radio Access Bearer establishment */
#define AS_RAB_ESTABLISH        0x08
#define AS_RAB_ESTABLISH_REQ        (AS_RAB_ESTABLISH | AS_REQUEST)
#define AS_RAB_ESTABLISH_IND        (AS_RAB_ESTABLISH | AS_INDICATION)
#define AS_RAB_ESTABLISH_RSP        (AS_RAB_ESTABLISH | AS_RESPONSE)
#define AS_RAB_ESTABLISH_CNF        (AS_RAB_ESTABLISH | AS_CONFIRM)

/* Radio Access Bearer release */
#define AS_RAB_RELEASE          0x09
#define AS_RAB_RELEASE_REQ      (AS_RAB_RELEASE | AS_REQUEST)
#define AS_RAB_RELEASE_IND      (AS_RAB_RELEASE | AS_INDICATION)

/* NAS Cause */
#define EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED (8)
#define EPS_SERVICES_NOT_ALLOWED                      (7)
#define PLMN_NOT_ALLOWED                              (11)
#define TRACKING_AREA_NOT_ALLOWED                     (12)
#define ROAMING_NOT_ALLOWED_IN_THIS_TRACKING_AREA     (13)
#define EPS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN         (14)
#define NO_SUITABLE_CELLS_IN_TRACKING_AREA            (15)
#define NETWORK_FAILURE                               (17)
#define ESM_FAILURE                                   (19)

typedef enum nas_cause_s {
  NAS_CAUSE_EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED = EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED,
  NAS_CAUSE_EPS_SERVICES_NOT_ALLOWED                  = EPS_SERVICES_NOT_ALLOWED,
  NAS_CAUSE_PLMN_NOT_ALLOWED                          = PLMN_NOT_ALLOWED,
  NAS_CAUSE_TRACKING_AREA_NOT_ALLOWED                 = TRACKING_AREA_NOT_ALLOWED,
  NAS_CAUSE_ROAMING_NOT_ALLOWED_IN_THIS_TRACKING_AREA = ROAMING_NOT_ALLOWED_IN_THIS_TRACKING_AREA,
  NAS_CAUSE_EPS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN     = EPS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN,
  NAS_CAUSE_NO_SUITABLE_CELLS_IN_TRACKING_AREA        = NO_SUITABLE_CELLS_IN_TRACKING_AREA,
  NAS_CAUSE_NETWORK_FAILURE                           = NETWORK_FAILURE,
  NAS_CAUSE_ESM_FAILURE                               = ESM_FAILURE
} nas_cause_t;

/*
 * --------------------------------------------------------------------------
 *          Access Stratum message global parameters
 * --------------------------------------------------------------------------
 */

/* Error code */
typedef enum nas_error_code_s {
  AS_SUCCESS = 1, /* Success code, transaction is going on    */
  AS_TERMINATED_NAS,  /* Transaction terminated by NAS        */
  AS_TERMINATED_AS,   /* Transaction terminated by AS         */
  AS_FAILURE      /* Failure code                 */
} nas_error_code_t;

/* Core network domain */
typedef enum core_network_s {
  AS_PS = 1,      /* Packet-Switched  */
  AS_CS       /* Circuit-Switched */
} core_network_t;

/* SAE Temporary Mobile Subscriber Identity */
typedef struct as_stmsi_s {
  uint8_t MMEcode;    /* MME code that allocated the GUTI     */
  uint32_t m_tmsi;    /* M-Temporary Mobile Subscriber Identity   */
} as_stmsi_t;

/* Dedicated NAS information */
typedef struct as_nas_info_s {
  uint32_t length;    /* Length of the NAS information data       */
  Byte_t* data;   /* Dedicated NAS information data container */
} as_nas_info_t;

/* Radio Access Bearer identity */
typedef uint8_t as_rab_id_t;

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *              Broadcast information
 * --------------------------------------------------------------------------
 */

/*
 * AS->NAS - Broadcast information indication
 * AS may asynchronously report to NAS available PLMNs within specific
 * location area
 */
typedef struct broadcast_info_ind_s {
#define PLMN_LIST_MAX_SIZE  6
  PLMN_LIST_T(PLMN_LIST_MAX_SIZE) plmnIDs; /* List of PLMN identifiers */
  ci_t cellID;    /* Identity of the cell serving the listed PLMNs */
  tac_t tac;      /* Code of the tracking area the cell belongs to */
} broadcast_info_ind_t;

/*
 * --------------------------------------------------------------------------
 *     Cell information relevant for cell selection processing
 * --------------------------------------------------------------------------
 */

/* Radio access technologies supported by the network */
#define AS_GSM              (1 << NET_ACCESS_GSM)
#define AS_COMPACT          (1 << NET_ACCESS_COMPACT)
#define AS_UTRAN            (1 << NET_ACCESS_UTRAN)
#define AS_EGPRS            (1 << NET_ACCESS_EGPRS)
#define AS_HSDPA            (1 << NET_ACCESS_HSDPA)
#define AS_HSUPA            (1 << NET_ACCESS_HSUPA)
#define AS_HSDUPA           (1 << NET_ACCESS_HSDUPA)
#define AS_EUTRAN           (1 << NET_ACCESS_EUTRAN)

/*
 * NAS->AS -K_eNB refresh request
 * NAS request AS to refresh its KeNB key
 */
typedef struct kenb_refresh_req_s {
  Byte_t kenb[32];
} kenb_refresh_req_t;

typedef struct nsa_oai_tun_nsa_s {
  uint8_t buffer[500];
} nas_oai_tun_nsa_t;

/*
 * NAS->AS - Cell Information request
 * NAS request AS to search for a suitable cell belonging to the selected
 * PLMN to camp on.
 */
typedef struct cell_info_req_s {
  plmn_t plmnID;  /* Selected PLMN identity           */
  Byte_t rat;     /* Bitmap - set of radio access technologies    */
} cell_info_req_t;

/*
 * AS->NAS - Cell Information confirm
 * AS search for a suitable cell and respond to NAS. If found, the cell
 * is selected to camp on.
 */
typedef struct cell_info_cnf_s {
  uint8_t errCode;    /* Error code                     */
  ci_t cellID;    /* Identity of the cell serving the selected PLMN */
  tac_t tac;      /* Code of the tracking area the cell belongs to  */
  AcT_t rat;      /* Radio access technology supported by the cell  */
  uint8_t rsrq;   /* Reference signal received quality         */
  uint8_t rsrp;   /* Reference signal received power       */
} cell_info_cnf_t;

/*
 * AS->NAS - Cell Information indication
 * AS may change cell selection if a more suitable cell is found.
 */
typedef struct cell_info_ind_s {
  ci_t cellID;    /* Identity of the new serving cell      */
  tac_t tac;      /* Code of the tracking area the cell belongs to */
} cell_info_ind_t;

/*
 * --------------------------------------------------------------------------
 *              Paging information
 * --------------------------------------------------------------------------
 */

/* Paging cause */
typedef enum paging_cause_s {
  AS_CONNECTION_ESTABLISH,    /* Establish NAS signalling connection  */
  AS_EPS_ATTACH,      /* Perform local detach and initiate EPS
                 * attach procedure         */
  AS_CS_FALLBACK      /* Inititate CS fallback procedure  */
} paging_cause_t;

/*
 * NAS->AS - Paging Information request
 * NAS requests the AS that NAS signalling messages or user data is pending
 * to be sent.
 */
typedef struct paging_req_s {
  as_stmsi_t s_tmsi;  /* UE identity                  */
  uint8_t CN_domain;  /* Core network domain              */
} paging_req_t;

/*
 * AS->NAS - Paging Information indication
 * AS reports to the NAS that appropriate procedure has to be initiated.
 */
typedef struct paging_ind_s {
  paging_cause_t cause;  /* Paging cause                 */
} paging_ind_t;

/*
 * --------------------------------------------------------------------------
 *          NAS signalling connection establishment
 * --------------------------------------------------------------------------
 */

/* Cause of RRC connection establishment */
typedef enum as_cause_s {
  AS_CAUSE_UNKNOWN    = 0,
  AS_CAUSE_EMERGENCY  = NET_ESTABLISH_CAUSE_EMERGENCY,
  AS_CAUSE_HIGH_PRIO  = NET_ESTABLISH_CAUSE_HIGH_PRIO,
  AS_CAUSE_MT_ACCESS  = NET_ESTABLISH_CAUSE_MT_ACCESS,
  AS_CAUSE_MO_SIGNAL  = NET_ESTABLISH_CAUSE_MO_SIGNAL,
  AS_CAUSE_MO_DATA    = NET_ESTABLISH_CAUSE_MO_DATA,
  AS_CAUSE_V1020      = NET_ESTABLISH_CAUSE_V1020
} as_cause_t;

/* Type of the call associated to the RRC connection establishment */
typedef enum as_call_type_s {
  AS_TYPE_ORIGINATING_SIGNAL  = NET_ESTABLISH_TYPE_ORIGINATING_SIGNAL,
  AS_TYPE_EMERGENCY_CALLS     = NET_ESTABLISH_TYPE_EMERGENCY_CALLS,
  AS_TYPE_ORIGINATING_CALLS   = NET_ESTABLISH_TYPE_ORIGINATING_CALLS,
  AS_TYPE_TERMINATING_CALLS   = NET_ESTABLISH_TYPE_TERMINATING_CALLS,
  AS_TYPE_MO_CS_FALLBACK      = NET_ESTABLISH_TYPE_MO_CS_FALLBACK
} as_call_type_t;

/*
 * NAS->AS - NAS signalling connection establishment request
 * NAS requests the AS to perform the RRC connection establishment procedure
 * to transfer initial NAS message to the network while UE is in IDLE mode.
 */
typedef struct nas_establish_req_s {
  as_cause_t      cause;          /* RRC connection establishment cause   */
  as_call_type_t  type;           /* RRC associated call type             */
  as_stmsi_t      s_tmsi;         /* UE identity                          */
  plmn_t          plmnID;         /* Selected PLMN identity               */
  as_nas_info_t   initialNasMsg;  /* Initial NAS message to transfer      */
} nas_establish_req_t;

/*
 * AS->NAS - NAS signalling connection establishment indication
 * AS transfers the initial NAS message to the NAS.
 */
typedef struct nas_establish_ind_s {
  uint32_t      UEid;          /* UE lower layer identifier               */
  tac_t         tac;           /* Code of the tracking area the initiating
                                  * UE belongs to                           */
  as_cause_t    asCause;       /* Establishment cause                     */
  as_nas_info_t initialNasMsg; /* Initial NAS message to transfer         */
} nas_establish_ind_t;

/*
 * NAS->AS - NAS signalling connection establishment response
 * NAS responds to the AS that initial answer message has to be provided to
 * the UE.
 */
typedef struct nas_establish_rsp_s {
  uint32_t         UEid;         /* UE lower layer identifier   */
  as_stmsi_t       s_tmsi;       /* UE identity                 */
  nas_error_code_t errCode;      /* Transaction status          */
  as_nas_info_t    nasMsg;       /* NAS message to transfer     */
  uint32_t         nas_ul_count; /* UL NAS COUNT                */
  uint16_t         selected_encryption_algorithm;
  uint16_t         selected_integrity_algorithm;
} nas_establish_rsp_t;

/*
 * AS->NAS - NAS signalling connection establishment confirm
 * AS transfers the initial answer message to the NAS.
 */
typedef struct nas_establish_cnf_s {
  uint32_t         UEid;            /* UE lower layer identifier   */
  nas_error_code_t errCode;         /* Transaction status          */
  as_nas_info_t    nasMsg;          /* NAS message to transfer     */
  uint32_t         ul_nas_count;
  uint16_t         selected_encryption_algorithm;
  uint16_t         selected_integrity_algorithm;
} nas_establish_cnf_t;

/*
 * --------------------------------------------------------------------------
 *          NAS signalling connection release
 * --------------------------------------------------------------------------
 */

/* Release cause */
typedef enum release_cause_s {
  AS_AUTHENTICATION_FAILURE = 1,  /* Authentication procedure failed   */
  AS_DETACH                       /* Detach requested                  */
} release_cause_t;

/*
 * NAS->AS - NAS signalling connection release request
 * NAS requests the termination of the connection with the UE.
 */
typedef struct nas_release_req_s {
  uint32_t UEid;          /* UE lower layer identifier    */
  as_stmsi_t s_tmsi;      /* UE identity                  */
  release_cause_t cause;  /* Release cause                */
} nas_release_req_t;

/*
 * AS->NAS - NAS signalling connection release indication
 * AS reports that connection has been terminated by the network.
 */
typedef struct nas_release_ind_s {
  release_cause_t cause;      /* Release cause            */
} nas_release_ind_t;

typedef struct nas_deregistration_req_s {
  release_cause_t cause;
} nas_deregistration_req_t;

typedef struct nas_detach_req_s {
  bool wait_release;
} nas_detach_req_t;

/*
 * --------------------------------------------------------------------------
 *              NAS information transfer
 * --------------------------------------------------------------------------
 */

/*
 * NAS->AS - Uplink data transfer request
 * NAS requests the AS to transfer uplink information to the NAS that
 * operates at the network side.
 */
typedef struct ul_info_transfer_req_s {
  uint32_t UEid;      /* UE lower layer identifier        */
  as_stmsi_t s_tmsi;      /* UE identity              */
  as_nas_info_t nasMsg;   /* Uplink NAS message           */
} ul_info_transfer_req_t;

/*
 * AS->NAS - Uplink data transfer confirm
 * AS immediately notifies the NAS whether uplink information has been
 * successfully sent to the network or not.
 */
typedef struct ul_info_transfer_cnf_s {
  uint32_t         UEid;      /* UE lower layer identifier        */
  nas_error_code_t errCode;   /* Transaction status               */
} ul_info_transfer_cnf_t;

/*
 * AS->NAS - Uplink data transfer indication
 * AS delivers the uplink information message to the NAS that operates
 * at the network side.
 */
typedef struct ul_info_transfer_ind_s {
  uint32_t UEid;          /* UE lower layer identifier        */
  as_nas_info_t nasMsg;   /* Uplink NAS message           */
} ul_info_transfer_ind_t;

/*
 * NAS->AS - Downlink data transfer request
 * NAS requests the AS to transfer downlink information to the NAS that
 * operates at the UE side.
 */
typedef ul_info_transfer_req_t dl_info_transfer_req_t;

/*
 * AS->NAS - Downlink data transfer confirm
 * AS immediately notifies the NAS whether downlink information has been
 * successfully sent to the network or not.
 */
typedef ul_info_transfer_cnf_t dl_info_transfer_cnf_t;

/*
 * AS->NAS - Downlink data transfer indication
 * AS delivers the downlink information message to the NAS that operates
 * at the UE side.
 */
typedef ul_info_transfer_ind_t dl_info_transfer_ind_t;

typedef struct nas_pdu_session_req_s {
  int pdusession_id;
  int pdusession_type;
  int sst;
  int sd;
} nas_pdu_session_req_t;

/*
 * --------------------------------------------------------------------------
 *          Radio Access Bearer establishment
 * --------------------------------------------------------------------------
 */

/* TODO: Quality of Service parameters */
typedef struct {
  // This dummy element is to avoid CLANG warning: empty struct has size 0 in C, size 1 in C++
  // To be removed if the structure is filled
  uint32_t dummy;
} as_qos_t;

/*
 * NAS->AS - Radio access bearer establishment request
 * NAS requests the AS to allocate transmission resources to radio access
 * bearer initialized at the network side.
 */
typedef struct rab_establish_req_s {
  as_stmsi_t s_tmsi;      /* UE identity                      */
  as_rab_id_t rabID;      /* Radio access bearer identity     */
  as_qos_t QoS;           /* Requested Quality of Service     */
} rab_establish_req_t;

/*
 * AS->NAS - Radio access bearer establishment indication
 * AS notifies the NAS that specific radio access bearer has to be setup.
 */
typedef struct rab_establish_ind_s {
  as_rab_id_t rabID;      /* Radio access bearer identity     */
} rab_establish_ind_t;

/*
 * NAS->AS - Radio access bearer establishment response
 * NAS responds to AS whether the specified radio access bearer has been
 * successfully setup or not.
 */
typedef struct rab_establish_rsp_s {
  as_stmsi_t       s_tmsi;        /* UE identity                      */
  as_rab_id_t      rabID;         /* Radio access bearer identity     */
  nas_error_code_t errCode;       /* Transaction status               */
} rab_establish_rsp_t;

/*
 * AS->NAS - Radio access bearer establishment confirm
 * AS notifies NAS whether the specified radio access bearer has been
 * successfully setup at the UE side or not.
 */
typedef struct rab_establish_cnf_s {
  as_rab_id_t rabID;          /* Radio access bearer identity     */
  nas_error_code_t errCode;   /* Transaction status               */
} rab_establish_cnf_t;

/*
 * --------------------------------------------------------------------------
 *              Radio Access Bearer release
 * --------------------------------------------------------------------------
 */

/*
 * NAS->AS - Radio access bearer release request
 * NAS requests the AS to release transmission resources previously allocated
 * to specific radio access bearer at the network side.
 */
typedef struct rab_release_req_s {
  as_stmsi_t s_tmsi;      /* UE identity                      */
  as_rab_id_t rabID;      /* Radio access bearer identity     */
} rab_release_req_t;

/*
 * AS->NAS - Radio access bearer release indication
 * AS notifies NAS that specific radio access bearer has been released.
 */
typedef struct rab_release_ind_s {
  as_rab_id_t rabID;      /* Radio access bearer identity     */
} rab_release_ind_t;

/*
 * --------------------------------------------------------------------------
 *  Structure of the AS messages handled by the network sublayer
 * --------------------------------------------------------------------------
 */
typedef struct as_message_s {
  uint16_t msgID;
  union {
    broadcast_info_ind_t broadcast_info_ind;
    cell_info_req_t cell_info_req;
    cell_info_cnf_t cell_info_cnf;
    cell_info_ind_t cell_info_ind;
    paging_req_t paging_req;
    paging_ind_t paging_ind;
    nas_establish_req_t nas_establish_req;
    nas_establish_ind_t nas_establish_ind;
    nas_establish_rsp_t nas_establish_rsp;
    nas_establish_cnf_t nas_establish_cnf;
    nas_release_req_t nas_release_req;
    nas_release_ind_t nas_release_ind;
    ul_info_transfer_req_t ul_info_transfer_req;
    ul_info_transfer_cnf_t ul_info_transfer_cnf;
    ul_info_transfer_ind_t ul_info_transfer_ind;
    dl_info_transfer_req_t dl_info_transfer_req;
    dl_info_transfer_cnf_t dl_info_transfer_cnf;
    dl_info_transfer_ind_t dl_info_transfer_ind;
    rab_establish_req_t rab_establish_req;
    rab_establish_ind_t rab_establish_ind;
    rab_establish_rsp_t rab_establish_rsp;
    rab_establish_cnf_t rab_establish_cnf;
    rab_release_req_t rab_release_req;
    rab_release_ind_t rab_release_ind;
  } __attribute__((__packed__)) msg;
} as_message_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int as_message_decode(const char* buffer, as_message_t* msg, int length);

int as_message_encode(char* buffer, as_message_t* msg, int length);

/* Implemented in the network_api.c body file */
int as_message_send(as_message_t* as_msg);

#endif /* __AS_MESSAGE_H__*/
