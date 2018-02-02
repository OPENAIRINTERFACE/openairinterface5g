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

Source    as_message.h

Version   0.1

Date    2012/10/18

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

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

/* Access Stratum Message types */
#define AS_REQUEST  0x0100
#define AS_RESPONSE 0x0200
#define AS_INDICATION 0x0400
#define AS_CONFIRM  0x0800

/*
 * Access Stratum Message identifiers
 * ----------------------------------
 */

/* Cell information relevant for cell selection processing */
#define AS_CELL_INFO      0x01
#define AS_CELL_INFO_REQ    (AS_CELL_INFO | AS_REQUEST)
#define AS_CELL_INFO_RSP    (AS_CELL_INFO | AS_RESPONSE)
#define AS_CELL_INFO_IND    (AS_CELL_INFO | AS_INDICATION)

/* Security mode control */
#define AS_SECURITY     0x02
#define AS_SECURITY_REQ     (AS_SECURITY | AS_REQUEST)
#define AS_SECURITY_RSP     (AS_SECURITY | AS_RESPONSE)

/* Paging information */
#define AS_PAGING     0x03
#define AS_PAGING_IND     (AS_PAGING | AS_INDICATION)

/* NAS signalling connection establishment */
#define AS_NAS_ESTABLISH    0x04
#define AS_NAS_ESTABLISH_REQ    (AS_NAS_ESTABLISH | AS_REQUEST)
#define AS_NAS_ESTABLISH_RSP    (AS_NAS_ESTABLISH | AS_RESPONSE)

/* NAS signalling connection release */
#define AS_NAS_RELEASE      0x05
#define AS_NAS_RELEASE_REQ    (AS_NAS_RELEASE | AS_REQUEST)
#define AS_NAS_RELEASE_IND    (AS_NAS_RELEASE | AS_INDICATION)

/* NAS information transfer */
#define AS_INFO_TRANSFER    0x10
#define AS_UL_INFO_TRANSFER   (AS_INFO_TRANSFER | AS_REQUEST)
#define AS_DL_INFO_TRANSFER   (AS_INFO_TRANSFER | AS_INDICATION)

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *      Available PLMNs and cell Information
 * --------------------------------------------------------------------------
 */

/* Radio access technologies supported by the network */
#define AS_GSM        (1 << NET_ACCESS_GSM)
#define AS_COMPACT      (1 << NET_ACCESS_COMPACT)
#define AS_UTRAN      (1 << NET_ACCESS_UTRAN)
#define AS_EGPRS      (1 << NET_ACCESS_EGPRS)
#define AS_HSDPA      (1 << NET_ACCESS_HSDPA)
#define AS_HSUPA      (1 << NET_ACCESS_HSUPA)
#define AS_HSDUPA     (1 << NET_ACCESS_HSDUPA)
#define AS_EUTRAN     (1 << NET_ACCESS_EUTRAN)

/*
 * NAS->AS -K_eNB refresh request
 * NAS request AS to refresh its KeNB key
 */
typedef struct kenb_refresh_req_s {
  Byte_t kenb[32];
} kenb_refresh_req_t;

/*
 * NAS->AS - Cell Information request
 * NAS request AS to search for a suitable cell belonging to the selected
 * PLMN to camp on.
 */
typedef struct {
  plmn_t plmnID;  /* PLMN identifier         */
  Byte_t rat;   /* Bitmap - set of radio access technologies   */
} cell_info_req_t;

/*
 * AS->NAS - Cell Information response
 * AS search for a suitable cell and respond to NAS. If found, the cell
 * is selected to camp on.
 */
typedef struct {
  plmn_t plmnID;  /* PLMN identifier          */
  TAC_t tac;    /* identifies a tracking area the PLMN belongs to */
  CI_t cellID;  /* identifies a cell within a PLMN      */
  AcT_t rat;    /* radio access technology supported by the cell  */
} cell_info_rsp_t;

/*
 * AS->NAS - Cell Information indication
 * AS Reports available PLMNs with associated Radio Access Technologies
 * to NAS on request from NAS or autonomously.
 */
typedef struct {
#define PLMN_LIST_MAX_SIZE  6
  PLMN_LIST_T(PLMN_LIST_MAX_SIZE) plmnIDs; /* List of PLMN identifiers */
  Byte_t rat[PLMN_LIST_MAX_SIZE]; /* Radio access technologies   */
  TAC_t tac; /* identifies a tracking area within a scope of PLMNs   */
  CI_t cellID;  /* identifies a cell within a PLMN     */
} cell_info_ind_t;

/*
 * --------------------------------------------------------------------------
 *        Security mode control
 * --------------------------------------------------------------------------
 */

/*
 * TODO: NAS->AS - Security command request
 */
typedef struct {} security_req_t;


/*
 * TODO: AS->NAS - Security command response
 */
typedef struct {} security_rsp_t;


/*
 * --------------------------------------------------------------------------
 *        Paging information
 * --------------------------------------------------------------------------
 */

/*
 * TODO: AS->NAS - Paging Information indication
 */
typedef struct {} paging_ind_t;

/*
 * --------------------------------------------------------------------------
 *      NAS signalling connection establishment
 * --------------------------------------------------------------------------
 */

/* Cause of RRC connection establishment */
#define AS_CAUSE_EMERGENCY    (NET_ESTABLISH_CAUSE_EMERGENCY)
#define AS_CAUSE_HIGH_PRIO    (NET_ESTABLISH_CAUSE_HIGH_PRIO)
#define AS_CAUSE_MT_ACCESS    (NET_ESTABLISH_CAUSE_MT_ACCESS)
#define AS_CAUSE_MO_SIGNAL    (NET_ESTABLISH_CAUSE_MO_SIGNAL)
#define AS_CAUSE_MO_DATA    (NET_ESTABLISH_CAUSE_MO_DATA)
#define AS_CAUSE_V1020      (NET_ESTABLISH_CAUSE_V1020)

/* Type of the call associated to the RRC connection establishment */
#define AS_TYPE_ORIGINATING_SIGNAL  (NET_ESTABLISH_TYPE_ORIGINATING_SIGNAL)
#define AS_TYPE_EMERGENCY_CALLS   (NET_ESTABLISH_TYPE_EMERGENCY_CALLS)
#define AS_TYPE_ORIGINATING_CALLS (NET_ESTABLISH_TYPE_ORIGINATING_CALLS)
#define AS_TYPE_TERMINATING_CALLS (NET_ESTABLISH_TYPE_TERMINATING_CALLS)
#define AS_TYPE_MO_CS_FALLBACK    (NET_ESTABLISH_TYPE_MO_CS_FALLBACK)


/* Structure of the SAE Temporary Mobile Subscriber Identity */
typedef struct {
  uint8_t MMEcode;  /* MME code that allocated the GUTI   */
  uint32_t m_tmsi;  /* M-Temporary Mobile Subscriber Identity */
} as_stmsi_t;

/* Structure of the dedicated NAS information */
typedef struct {
  uint32_t length;  /* Length of the NAS information data   */
  Byte_t* data; /* Dedicated NAS information data container */
} as_nas_info_t;

/*
 * NAS->AS - NAS signalling connection establishment request
 * NAS request AS to perform the RRC connection establishment procedure
 * to transfer initial NAS message to the network while UE is in IDLE mode.
 */
typedef struct {
  uint8_t cause;    /* Connection establishment cause     */
  uint8_t type;   /* Associated call type         */
  plmn_t plmnID;    /* Identifier of the selected PLMN      */
  as_stmsi_t s_tmsi;    /* SAE Temporary Mobile Subscriber Identity */
  as_nas_info_t initialNasMsg;/* Initial NAS message to transfer      */
} nas_establish_req_t;

/*
 * TODO: AS->NAS - NAS signalling connection establishment response
 */
typedef struct {} nas_establish_rsp_t;

/*
 * --------------------------------------------------------------------------
 *      NAS signalling connection release
 * --------------------------------------------------------------------------
 */

/*
 * TODO: NAS->AS - NAS signalling connection release request
 */
typedef struct {} nas_release_req_t;

/*
 * TODO: AS->NAS - NAS signalling connection release indication
 */
typedef struct {} nas_release_ind_t;

/*
 * --------------------------------------------------------------------------
 *        NAS information transfer
 * --------------------------------------------------------------------------
 */

/*
 * AS->NAS - Downlink data transfer
 * AS notifies upper layer that NAS or non-3GPP dedicated downlink information
 * has to be transfered to NAS.
*/
typedef as_nas_info_t dl_info_transfer_t;

/*
 * NAS->AS - Uplink data transfer request
 * NAS requests under layer to transfer NAS or non-3GPP dedicated uplink
 * information to AS.
 */
typedef as_nas_info_t ul_info_transfer_t;

/*
 * --------------------------------------------------------------------------
 *  Structure of the AS messages handled by the network sublayer
 * --------------------------------------------------------------------------
 */
typedef struct {
  uint16_t msgID;
  union {
    cell_info_req_t cell_info_req;
    cell_info_rsp_t cell_info_rsp;
    cell_info_ind_t cell_info_ind;
    security_req_t security_req;
    security_rsp_t security_rsp;
    paging_ind_t paging_ind;
    nas_establish_req_t nas_establish_req;
    nas_establish_rsp_t nas_establish_rsp;
    nas_release_req_t nas_release_req;
    nas_release_ind_t nas_release_ind;
    ul_info_transfer_t ul_info_transfer;
    dl_info_transfer_t dl_info_transfer;
  } __attribute__((__packed__)) msg;
} as_message_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int as_message_decode(const char* buffer, int length, as_message_t* msg);

int as_message_encode(char* buffer, int length, const as_message_t* msg);

/* Implemented in the network_api.c body file */
int as_message_send(as_message_t* as_msg, const void* nas_msg);

#endif /* __AS_MESSAGE_H__*/
