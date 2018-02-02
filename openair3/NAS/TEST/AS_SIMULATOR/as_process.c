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
      Eurecom OpenAirInterface 3
      Copyright(c) 2012 Eurecom

Source    as_process.c

Version   0.1

Date    2013/04/11

Product   Access-Stratum sublayer simulator

Subsystem AS message processing

Author    Frederic Maurel

Description Defines functions executed by the Access-Stratum sublayer
    upon receiving AS messages from the Non-Access-Stratum.

*****************************************************************************/

#include "as_process.h"
#include "nas_process.h"

#include "commonDef.h"
#include "as_data.h"

#include <sys/types.h>
#include <stdio.h>  // snprintf
#include <string.h> // memcpy

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* Message Sequence Chart Generator's buffer */
#define MSCGEN_BUFFER_SIZE  1024
static char _mscgen_buffer[MSCGEN_BUFFER_SIZE];

/* Tracking area code */
#define DEFAULT_TAC 0xCAFE    // two byte in hexadecimal format

/* Cell identity */
#define DEFAULT_CI  0x01020304  // four byte in hexadecimal format

/* Reference signal received power */
#define DEFAULT_RSRP  27

/* Reference signal received quality */
#define DEFAULT_RSRQ  55

static ssize_t _process_plmn(char* buffer, const plmn_t* plmn, size_t len);
static int _process_dump(char* buffer, int len, const char* msg, int size);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * =============================================================================
 *  Functions used to process messages received from the UE NAS process
 * =============================================================================
 */

/*
 * -----------------------------------------------------------------------------
 *        Process cell information request message
 * -----------------------------------------------------------------------------
 */
int process_cell_info_req(int msg_id, const cell_info_req_t* req,
                          cell_info_cnf_t* cnf)
{
  int index = 0;

  printf("INFO\t: %s - Process cell information request\n", __FUNCTION__);

  index += _process_plmn(_mscgen_buffer, &req->plmnID, MSCGEN_BUFFER_SIZE);
  snprintf(_mscgen_buffer + index, MSCGEN_BUFFER_SIZE - index,
           ", rat = 0x%x%c", req->rat, '\0');
  MSCGEN("[MSC_MSG][%s][%s][--- Cell Information Request (0x%.4x)\\n%s --->][%s]\n",
         getTime(), _ue_id, msg_id, _mscgen_buffer, _as_id);

  /* Setup cell information confirm message */
  cnf->errCode = AS_SUCCESS;
  cnf->tac = DEFAULT_TAC;
  cnf->cellID = DEFAULT_CI;
  cnf->rat = AS_EUTRAN;
  cnf->rsrp = DEFAULT_RSRP;
  cnf->rsrq = DEFAULT_RSRQ;

  snprintf(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
           "tac = 0x%.4x, cellID = 0x%.8x, rat = 0x%x, RSRQ = %u, RSRP = %u%c",
           cnf->tac, cnf->cellID, cnf->rat, cnf->rsrq, cnf->rsrp, '\0');
  MSCGEN("[MSC_MSG][%s][%s][--- Cell Information Confirm (0x%.4x)\\n%s --->][%s]\n",
         getTime(), _as_id, AS_CELL_INFO_CNF, _mscgen_buffer, _ue_id);

  printf("INFO\t: %s - Send cell informtion confirm\n", __FUNCTION__);
  return (AS_CELL_INFO_CNF);
}

/*
 * -----------------------------------------------------------------------------
 *  Process NAS signalling connection establishment request message
 * -----------------------------------------------------------------------------
 */
int process_nas_establish_req(int msg_id, const nas_establish_req_t* req,
                              nas_establish_ind_t* ind,
                              nas_establish_cnf_t* cnf)
{
  int bytes;

  printf("INFO\t: %s - Process NAS signalling connection establishment "
         "request\n", __FUNCTION__);

  // Attach_Request.Pdn_Connectivity_Request.ebi = 5
  //req->initialNasMsg.data[23] = 0x52; // MME DEBUG (Reject Attachment)
  // Attach_Request.Pdn_Connectivity_Request.messagetype = WRONG_MESSAGE_TYPE
  //req->initialNasMsg.data[25] = 0xc4; // MME DEBUG (Send ESM status)

  snprintf(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
           "cause = %s, type = %s, tmsi = {MMEcode = %d, m_tmsi = %u}%c",
           rrcCause(req->cause), rrcType(req->type),
           req->s_tmsi.MMEcode, req->s_tmsi.m_tmsi, '\0');
  MSCGEN("[MSC_MSG][%s][%s][--- NAS Signalling Connection Establishment Request "
         "(0x%.4x)\\n%s --->][%s]\n",
         getTime(), _ue_id, msg_id, _mscgen_buffer, _as_id);

  _process_dump(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
                (char*)req->initialNasMsg.data, req->initialNasMsg.length);
  MSCGEN("[MSC_RBOX][%s][%s][%s]\n", _ue_id, _as_id, _mscgen_buffer);

  /* Process initial NAS message */
  bytes = nas_process(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
                      (char*)req->initialNasMsg.data,
                      req->initialNasMsg.length);

  if (bytes < 0) {
    MSCGEN("[MSC_MSG][%s][%s][--- NAS Signalling Connection Establishment "
           "Indication (0x%.4x)\\ntac = 0x%.4x\\n%s ---x][%s]\n", getTime(),
           _as_id, AS_NAS_ESTABLISH_IND, ind->tac, _mscgen_buffer, _mme_id);
    /* Setup NAS signalling connection establishment confirmation message */
    cnf->errCode = AS_TERMINATED_AS;
    return (0);
  }

  /* Setup NAS signalling connection establishment indication message */
  ind->UEid = 1;  // Valid UEid starts at index 1
  ind->tac = DEFAULT_TAC;

  if (req->initialNasMsg.length > 0) {
    ind->initialNasMsg.data = (Byte_t*)malloc(req->initialNasMsg.length);

    if (ind->initialNasMsg.data) {
      memcpy(ind->initialNasMsg.data, req->initialNasMsg.data,
             req->initialNasMsg.length);
      ind->initialNasMsg.length = req->initialNasMsg.length;
    }
  }

  // Attach_Request.messagetype = WRONG_MESSAGE_TYPE
  //ind->initialNasMsg.data[1] = 0x47; // MME DEBUG (Send EMM status)

  /* Setup NAS signalling connection establishment confirmation message */
  cnf->errCode = AS_SUCCESS;
  // Transmission failure of Attach Request message
  //cnf->errCode = AS_FAILURE; // UE DEBUG

  MSCGEN("[MSC_MSG][%s][%s][--- NAS Signalling Connection Establishment Indication "
         "(0x%.4x)\\nUEid = %u, tac = 0x%.4x\\n%s --->][%s]\n", getTime(),
         _as_id, AS_NAS_ESTABLISH_IND, ind->UEid, ind->tac, _mscgen_buffer, _mme_id);

  printf("INFO\t: %s - Send NAS signalling connection establishment "
         "indication\n", __FUNCTION__);
  return (AS_NAS_ESTABLISH_IND);
}

/*
 * -----------------------------------------------------------------------------
 *    Process Uplink data transfer request message
 * -----------------------------------------------------------------------------
 */
int process_ul_info_transfer_req(int msg_id, const ul_info_transfer_req_t* req,
                                 ul_info_transfer_ind_t* ind,
                                 ul_info_transfer_cnf_t* cnf)
{
  int bytes;

  printf("INFO\t: %s - Process uplink data transfer request\n", __FUNCTION__);

  // Attach_Complete.Activate_Default_EPS_Bearer_Context_Accept.ebi = UNASSIGNED
  //if (req->nasMsg.data[7] == 0x43) {
  //  req->nasMsg.data[10] = 0x02; // MME DEBUG (Send ESM status)
  //}
  // Deactivate_EPS_Bearer_Context_Accept.ebi = UNASSIGNED
  //if (req->nasMsg.data[8] == 0xCE) {
  //  req->nasMsg.data[6] = 0x02; // MME DEBUG (Discard ESM message)
  //}

  snprintf(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
           "tmsi = {MMEcode = %d, m_tmsi = %u}%c",
           req->s_tmsi.MMEcode, req->s_tmsi.m_tmsi, '\0');
  MSCGEN("[MSC_MSG][%s][%s][--- Uplink Information Request "
         "(0x%.4x)\\n%s --->][%s]\n",
         getTime(), _ue_id, msg_id, _mscgen_buffer, _as_id);

  _process_dump(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
                (char*)req->nasMsg.data, req->nasMsg.length);
  MSCGEN("[MSC_RBOX][%s][%s][%s]\n", _ue_id, _as_id, _mscgen_buffer);

  /* Process NAS message */
  bytes = nas_process(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
                      (char*)req->nasMsg.data,
                      req->nasMsg.length);

  if (bytes < 0) {
    MSCGEN("[MSC_MSG][%s][%s][--- Uplink Information Indication "
           "(0x%.4x)\\n%s ---x][%s]\n", getTime(),
           _as_id, AS_UL_INFO_TRANSFER_IND, _mscgen_buffer, _mme_id);
    /* Setup uplink information confirmation message */
    cnf->errCode = AS_TERMINATED_AS;
    return (0);
  }

  /* Setup uplink information indication message */
  ind->UEid = 1;  // Valid UEid starts at index 1

  if (req->nasMsg.length > 0) {
    ind->nasMsg.data = (Byte_t*)malloc(req->nasMsg.length);

    if (ind->nasMsg.data) {
      memcpy(ind->nasMsg.data, req->nasMsg.data, req->nasMsg.length);
      ind->nasMsg.length = req->nasMsg.length;
    }
  }

  // Identity-Response.messagetype = WRONG_MESSAGE_TYPE
  //if (ind->nasMsg.data[1] == 0x56) {
  //  ind->nasMsg.data[1] = 0x47; // MME DEBUG (Send EMM status)
  //}
  // Authentication-Response.messagetype = WRONG_MESSAGE_TYPE
  //if (ind->nasMsg.data[1] == 0x53) {
  //  ind->nasMsg.data[1] = 0x47; // MME DEBUG (Send EMM status)
  //}
  // Security-Mode-Complete.messagetype = WRONG_MESSAGE_TYPE
  //if (ind->nasMsg.data[7] == 0x5e) {
  //  ind->nasMsg.data[7] = 0x47; // MME DEBUG (Send EMM status)
  //}

  /* Setup uplink information confirmation message */
  cnf->UEid = 0;
  cnf->errCode = AS_SUCCESS;
  // Transmission failure of Attach Complete message
  //if (req->nasMsg.data[7] == 0x43) {
  //  cnf->errCode = AS_FAILURE; // UE DEBUG
  //}

  MSCGEN("[MSC_MSG][%s][%s][--- Uplink Information Indication "
         "(0x%.4x), UEid = %u\\n%s --->][%s]\n", getTime(),
         _as_id, AS_UL_INFO_TRANSFER_IND, ind->UEid, _mscgen_buffer, _mme_id);

  printf("INFO\t: %s - Send uplink data transfer indication\n", __FUNCTION__);
  return (AS_UL_INFO_TRANSFER_IND);
}

/*
 * -----------------------------------------------------------------------------
 *      Process NAS signalling connection release request message
 * -----------------------------------------------------------------------------
 */
int process_nas_release_req(int msg_id, const nas_release_req_t* req)
{
  printf("INFO\t: %s - Process NAS signalling connection release request\n",
         __FUNCTION__);

  MSCGEN("[MSC_MSG][%s][%s][--- NAS Signalling Release Request "
         "(0x%.4x), tmsi = {MMEcode = %d, m_tmsi = %u}, "
         "cause = %s (%u) --->][%s]\n", getTime(), _ue_id, msg_id,
         req->s_tmsi.MMEcode, req->s_tmsi.m_tmsi,
         rrcReleaseCause(req->cause), req->cause, _as_id);
  return (AS_NAS_RELEASE_REQ);
}

/*
 * =============================================================================
 *  Functions used to process messages received from the MME NAS process
 * =============================================================================
 */

/*
 * -----------------------------------------------------------------------------
 *  Process NAS signalling connection establishment response message
 * -----------------------------------------------------------------------------
 */
int process_nas_establish_rsp(int msg_id, const nas_establish_rsp_t* rsp,
                              nas_establish_cnf_t* cnf)
{
  int bytes;

  printf("INFO\t: %s - Process NAS signalling connection establishment "
         "response\n", __FUNCTION__);

  // Attach_Accept
  //if (rsp->nasMsg.data[7] == 0x42) {
  // Activate_Default_EPS_Bearer_Context_Request.ebi = UNASSIGNED
  //rsp->nasMsg.data[19] = 0x02; // UE DEBUG (Send ESM status)
  // Activate_Default_EPS_Bearer_Context_Request.messagetype = WRONG_MESSAGE_TYPE
  //rsp->nasMsg.data[21] = 0xc4; // MME DEBUG (Send ESM status)
  //}

  MSCGEN("[MSC_MSG][%s][%s][--- NAS Signalling Connection Establishment "
         "Response (0x%.4x)\\nerrCode = %s, UEid = %u --->][%s]\n",
         getTime(), _mme_id, msg_id,
         rrcErrCode(rsp->errCode), rsp->UEid, _as_id);

  _process_dump(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
                (char*)rsp->nasMsg.data, rsp->nasMsg.length);
  MSCGEN("[MSC_RBOX][%s][%s][%s]\n", _mme_id, _as_id, _mscgen_buffer);

  // Discard Attach Accept message
  //return (0); // UE DEBUG

  /* Process initial NAS message */
  bytes = nas_process(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
                      (char*)rsp->nasMsg.data,
                      rsp->nasMsg.length);

  if (bytes < 0) {
    MSCGEN("[MSC_MSG][%s][%s][--- NAS Signalling Connection Establishment "
           "Confirm (0x%.4x)\\n%s ---x][%s]\n", getTime(),
           _as_id, AS_NAS_ESTABLISH_CNF, _mscgen_buffer, _ue_id);
    return (0);
  }

  /* Setup NAS signalling connection establishment confirm message */
  cnf->errCode = rsp->errCode;

  if (rsp->nasMsg.length > 0) {
    cnf->nasMsg.data = (Byte_t*)malloc(rsp->nasMsg.length);

    if (cnf->nasMsg.data) {
      memcpy(cnf->nasMsg.data, rsp->nasMsg.data,
             rsp->nasMsg.length);
      cnf->nasMsg.length = rsp->nasMsg.length;
    }
  }

  MSCGEN("[MSC_MSG][%s][%s][--- NAS Signalling Connection Establishment "
         "Confirm (0x%.4x)\\n%s --->][%s]\n", getTime(),
         _as_id, AS_NAS_ESTABLISH_CNF, _mscgen_buffer, _ue_id);

  printf("INFO\t: %s - Send NAS signalling connection establishment "
         "confirm\n", __FUNCTION__);
  return (AS_NAS_ESTABLISH_CNF);
}

/*
 * -----------------------------------------------------------------------------
 *    Process Downlink data transfer request message
 * -----------------------------------------------------------------------------
 */
int process_dl_info_transfer_req(int msg_id, const dl_info_transfer_req_t* req,
                                 dl_info_transfer_ind_t* ind,
                                 dl_info_transfer_cnf_t* cnf)
{
  int bytes;

  printf("INFO\t: %s - Process downlink data transfer request\n",
         __FUNCTION__);

  // Activate_default_EPS_Bearer_Context_Request.ebi = 6 (already active)
  //if (req->nasMsg.data[8] == 0xC1) {
  //  req->nasMsg.data[6] = 0x52; // UE DEBUG (Accept with same ebi)
  //}
  // Deactivate_EPS_Bearer_Context_Request.ebi = UNASSIGNED
  //if (req->nasMsg.data[8] == 0xCD) {
  //  req->nasMsg.data[6] = 0x02; // UE DEBUG
  //}

  MSCGEN("[MSC_MSG][%s][%s][--- Downlink Information Request "
         "(0x%.4x), UEid = %u --->][%s]\n",
         getTime(), _mme_id, msg_id, req->UEid, _as_id);

  _process_dump(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
                (char*)req->nasMsg.data, req->nasMsg.length);
  MSCGEN("[MSC_RBOX][%s][%s][%s]\n", _mme_id, _as_id, _mscgen_buffer);

  /* Process NAS message */
  bytes = nas_process(_mscgen_buffer, MSCGEN_BUFFER_SIZE,
                      (char*)req->nasMsg.data,
                      req->nasMsg.length);

  if (bytes < 0) {
    MSCGEN("[MSC_MSG][%s][%s][--- Downlink Information Indication "
           "(0x%.4x)\\n%s ---x][%s]\n", getTime(),
           _as_id, AS_DL_INFO_TRANSFER_IND, _mscgen_buffer, _ue_id);
    /* Setup downlink information confirmation message */
    cnf->UEid = req->UEid;
    cnf->errCode = AS_TERMINATED_AS;
    return (0);
  }

  /* Setup downlink information indication message */
  if (req->nasMsg.length > 0) {
    ind->nasMsg.data = (Byte_t*)malloc(req->nasMsg.length);

    if (ind->nasMsg.data) {
      memcpy(ind->nasMsg.data, req->nasMsg.data, req->nasMsg.length);
      ind->nasMsg.length = req->nasMsg.length;
    }
  }

  /* Setup downlink information confirmation message */
  cnf->UEid = req->UEid;
  cnf->errCode = AS_SUCCESS;

  //if (req->nasMsg.data[1] == 0x55) {
  // Transmission failure of identification request message
  //  cnf->errCode = AS_FAILURE; // MME DEBUG
  // Discard identification request message
  //  return (0); // MME DEBUG
  //}

  MSCGEN("[MSC_MSG][%s][%s][--- Downlink Information Indication "
         "(0x%.4x)\\n%s --->][%s]\n", getTime(),
         _as_id, AS_DL_INFO_TRANSFER_IND, _mscgen_buffer, _ue_id);

  printf("INFO\t: %s - Send downlink data transfer indication\n", __FUNCTION__);
  return (AS_DL_INFO_TRANSFER_IND);
}

/*
 * -----------------------------------------------------------------------------
 *      Process NAS signalling connection release indication message
 * -----------------------------------------------------------------------------
 */
int process_nas_release_ind(int msg_id, const nas_release_req_t* req,
                            nas_release_ind_t* ind)
{
  printf("INFO\t: %s - Process NAS signalling connection release request\n",
         __FUNCTION__);

  MSCGEN("[MSC_MSG][%s][%s][--- NAS Signalling Connection Release Request "
         "(0x%.4x), UEid = %u, cause = %s (%u) --->][%s]\n",
         getTime(), _mme_id, msg_id, req->UEid,
         rrcReleaseCause(req->cause), req->cause, _as_id);

  /* Forward NAS release indication to the UE */
  ind->cause = req->cause;
  MSCGEN("[MSC_MSG][%s][%s][--- NAS Signalling Connection Release Indication "
         "(0x%.4x), cause = %s (%u) --->][%s]\n",
         getTime(), _as_id, msg_id, rrcReleaseCause(req->cause),
         ind->cause, _ue_id);

  printf("INFO\t: %s - Send NAS signalling connection release indication\n",
         __FUNCTION__);
  return (AS_NAS_RELEASE_IND);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * -----------------------------------------------------------------------------
 *        Display PLMN identity
 * -----------------------------------------------------------------------------
 */
static ssize_t _process_plmn(char* buffer, const plmn_t* plmn, size_t len)
{
  int index = 0;
  index += snprintf(buffer + index, len - index, "plmnID = %u%u%u%u%u",
                    plmn->MCCdigit1, plmn->MCCdigit2, plmn->MCCdigit3,
                    plmn->MNCdigit1, plmn->MNCdigit2);

  if ( (index < len) && (plmn->MNCdigit3 != 0xf) ) {
    index += snprintf(buffer + index, len - index, "%u", plmn->MNCdigit3);
  }

  return (index);
}

/*
 * -----------------------------------------------------------------------------
 *        Message dump utility
 * -----------------------------------------------------------------------------
 */
static int _process_dump(char* buffer, int len, const char* msg, int size)
{
  int index = 0;

  for (int i = 0; i < size; i++) {
    if ( (i%16) == 0 ) index += snprintf(buffer + index,
                                           len - index, "\\n");

    index += snprintf(buffer + index, len - index,
                      " %.2hx", (unsigned char)(msg[i]));
  }

  buffer[index] = '\0';
  return (index);
}

/*
        04 01 04 10 00 00 0f 00 00 00 01 00 00 00 20 8f
        10 00 22 00 00 00 07 41 71 0b f6 02 f8 01 01 02
        0f 00 00 00 01 05 80 80 00 00 00 00 04 02 01 d0
        31 52 02 f8 01 00 01 e0
*/
/*
  U16 msgID = [04 01] - AS_NAS_ESTABLISH_REQ
  U8 cause = [04] - NET_ESTABLISH_CAUSE_MO_SIGNAL
  U8 type = [10]  - NET_ESTABLISH_TYPE_ORIGINATING_SIGNAL
  as_stmsi_t s_tmsi
  U8 MMEcode = [0f]
  U32 m_tmsi = [00 00 00 01]
  plmn_t plmnID = [20 8f 10]
  Byte_t MCCdigit2:4 = [0]
  Byte_t MCCdigit1:4 = [2]
  Byte_t MNCdigit3:4 = [f]
  Byte_t MCCdigit3:4 = [8]
  Byte_t MNCdigit2:4 = [0]
  Byte_t MNCdigit1:4 = [1]

  U32 Length = [22 00 00 00] (34 octets)
  U8* initialNasMsg = [07 41 71 0b f6 02 f8 01 01 02 0f 00 00 00 01 05]
            [80 80 00 00 00 00 04 02 01 d0 31 52 02 f8 01 00]
          [01 e0]

  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [41]  - Attach Request

  U8:4 NAS KSI = [7]  - No key available
  U8:4 Attach Type = [1]
  EPS Mobile Identity = [0b f6 02 f8 01 01 02 0f 00 00 00 01]
  Length = [0b] (11 bytes)
  U8:4 = 1111 = [f]
  U8:1 oddeven = 0
  U8:3 typeofidentity = 110 = [6] - GUTI
  U8:4 mccdigit2 = [0]
  U8:4 mccdigit1 = [2]
  U8:4 mncdigit3 = [f]
  U8:4 mccdigit3 = [8]
  U8:4 mncdigit2 = [0]
  U8:4 mncdigit1 = [1]
  U16  mmegroupid = [01 02]
  U8   mmecode = [0f]
  U32  mtmsi = [00 00 00 01]
  UE network capability = [05 80 80 00 00 00]
  Length = [05] (5 bytes)
  U8   eea = [80] - EEA0
  U8   eia = [80] - EIA0
  U8   uea = [00]
  U8:1 ucs2 = 0
  U8:7 uia = 0000000 = [00]
  U8:3 spare = 000
  U8:1 csfb = 0
  U8:1 lpp = 0
  U8:1 lcs = 0
  U8:1 srvcc = 0
  U8:1 nf = 0 = [00]

  ESM message container = [00 04 02 01 d0 31]
  Length = [00 04] (4 bytes)
  ESM message = [02 01 d0 31]
  U8:4 EPS bearer identity = [0]  - Not assigned
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [01]
  U8   ESM message identifier = [d0]  - PDN Connectivity Request
  U8:4 Pdn type = [3]     - IPv4v6
  U8:4 Request type = [1]   - Initial request

  Last TAI = [52 02 f8 01 00 01]  - PLMN = 20810, TAC = 00 01
  Old GUTI type = [e0]      - Native GUTI
*/

/*
        07 04 00 00 00 00 03 00 00 00 07 44 03
*/
/*
  U16 msgID = [07 04] - AS_DL_INFO_TRANSFER_IND
  U32 UEid = [00 00 00 00]

  U32 Length = [03 00 00 00] (3 octets)
  U8* NasMsg = [07 44 03]

  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [44]  - Attach Reject

  U8   EMM cause code = [03]    - EMM_CAUSE_ILLEGAL_UE
*/

/*
        07 04 00 00 00 00 10 00 00 00 27 ab ab ab ab 00
        07 44 13 78 00 04 02 01 d1 1f
*/
/*
  U16 msgID = [07 04] - AS_DL_INFO_TRANSFER_IND
  U32 UEid = [00 00 00 00]

  U32 Length = [10 00 00 00] (16 octets)
  U8* NasMsg = [27 ab ab ab ab 00 07 44 13 78 00 04 02 01 d1 1f]

  U8:4 Security header type = [2] - Integrity protected and cyphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - 0

  NAS message = [07 44 13 78 00 04 02 d1 1f]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [44]  - Attach Reject

  U8   EMM cause code = [13]    - #19 ESM failure

  ESM message container = [00 04 02 01 d1 1f]
  U16 Length = [00 04] (4 octets)
  U8* EsmMsg = [02 01 d1 1f]
  U8:4 EPS bearer identity = [0]  - Not assigned
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [01]
  U8   ESM message identifier = [d1]  - PDN Connectivity Reject

  U8   ESM cause code = [1f]    - REQUEST_REJECTED_UNSPECIFIED
*/

/*
        04 08 01 00 00 00 46 00 00 00 27 ab ab ab ab 00
        07 42 01 e0 06 24 02 f8 01 00 01 00 26 52 01 c1
        05 03 48 87 40 78 0e 77 77 77 2e 65 75 72 65 63
        6f 6d 2e 66 72 0d 03 c0 a8 02 3c 02 21 70 ff c0
        a8 02 3c 50 0b f6 02 f8 01 01 02 0f 00 00 00 01
*/
/*
  U16 msgID = [04 08] - AS_NAS_ESTABLISH_CNF
  U8 errCode = [01] - SUCCESS

  U32 Length = [00 00 00 46] (70 octets)
  U8* NasMsg = [27 ab ab ab ab 00 07 42 01 e0 06 24 02 f8 01 00]
           [01 00 26 52 01 c1 05 03 48 87 40 78 0e 77 77 77]
         [2e 65 75 72 65 63 6f 6d 2e 66 72 0d 03 c0 a8 02]
         [3c 02 21 70 ff c0 a8 02 3c 50 0b f6 02 f8 01 01]
         [02 0f 00 00 00 01]

  U8:4 Security header type = [2] - Integrity protected and cyphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - 0

  NAS message = [07 42 01 e0 06 24 02 f8 01 00 01 00 26 52 01 c1]
    [05 03 48 87 40 78 0e 77 77 77 2e 65 75 72 65 63]
    [6f 6d 2e 66 72 0d 03 c0 a8 02 3c 02 21 70 ff c0]
    [a8 02 3c 50 0b f6 02 f8 01 01 02 0f 00 00 00 01]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [42]  - Attach Accept

  U8:4 Spare = [0]
  U8:4 EPS attach result = [1]    - EPS
  U8:3 T3412 unit = 111     - Deactivated
  U8:5 T3412 value = 00000

  TAI list = [06 24 02 f8 01 00 01]
  Length = [06]
  U8:1 spare = 0
  U8:2 Type of list = 01    - list of TACs belonging to one PLMN,
              with consecutive TAC values
  U8:5 Number of elements = 0 0100  - 4 consecutive TACs
  TAI = [02 f8 01 00 01]    - PLMN = 20810, TAC = 00 01

  ESM message container = [00 26 52 01 c1 05 03 48 87 40 78 0e 77 77 77 2e]
          [65 75 72 65 63 6f 6d 2e 66 72 0d 03 c0 a8 02 3c]
        [02 21 70 ff c0 a8 02 3c]
  U16 Length = [00 26] (38 octets)
  U8:4 EPS bearer identity = [5]  - ebi = 5
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [01]
  U8   ESM message identifier = [c1]  - Activate Default EPS Bearer Context Request

  EPS Qos = [05 03 48 87 40 78]
  Length = [05] - 5 octets
  QCI = [03]  - 3
  Maximum bit rate for uplink = [48]    - 128K
  Maximum bit rate for downlink = [87]    - 1024K
  Guaranteed bit rate for uplink = [40]   - 64K
  Guaranteed bit rate for downlink = [78] - 512K

  Access point name  = [0e 77 77 77 2e 65 75 72 65 63 6f 6d 2e 66 72]
  Length = [0e] - 14 octets
  APN = "www.eurecom.fr"

  PDN address = [0d 03 c0 a8 02 3c 02 21 70 ff c0 a8 02 3c]
  Length = [0d] - 13 octets
  U8:5 Spare = 00000  - 0
  U8:3 Pdn type = 011 - IPv4v6
  PDN address information = [c0 a8 02 3c 02 21 70 ff c0 a8 02 3c]
*/

/*
        06 01 00 00 00 00 0f 00 00 00 01 00 00 00 0d 00
        00 00 27 ab ab ab ab 00 07 43 00 03 52 00 c2
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [0f]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 0d] (13 octets)
  U8* NasMsg = [27 ab ab ab ab 00 07 43 00 03 52 00 c2]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [07 43 00 03 52 00 c2]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [43]  - Attach Complete

  ESM message container = [00 03 52 00 c2]
  Length = [00 03] (3 bytes)
  ESM message = [52 00 c2]
  U8:4 EPS bearer identity = [5]  - ebi = 5
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [00]  - unassigned
  U8   ESM message identifier = [c2]  - Activate Default EPS Bearer Context Accept
*/

/*
        07 04 00 00 00 00 03 00 00 00 07 55 10
*/
/*
  U16 msgID = [07 04] - AS_DL_INFO_TRANSFER_IND
  U32 UEid = [00 00 00 00]

  U32 Length = [03 00 00 00] (3 octets)
  U8* NasMsg = [07 55 10]

  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [55]  - Identity Request

  U8:4 Identity type = [1]  - IMSI
  U8:4 spare = [0]
*/

/*
        06 01 00 00 00 00 0f 00 00 00 01 00 00 00 0b 00
        00 00 07 56 08 21 80 01 0f 00 10 32 f4
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [0f]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 0b] (11 octets)
  U8* NasMsg = [07 56 08 21 80 01 0f 00 10 32 f4]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [56]  - Identity Response

  Mobile identity = [08 21 80 01 0f 00 10 32 f4]
  Length = [08] - 8 octets
  U8:4 Identity digit 1 = [2]  - 2
  U8:1 odd/even indicator = [0]  - even number of identity digits
  U8:3 Type of identity = [001]  - IMSI
  U8:4 Identity digit 3 = [8]  - 8
  U8:4 Identity digit 2 = [0]  - 0
  U8:4 Identity digit 5 = [0]  - 0
  U8:4 Identity digit 4 = [1]  - 1
  U8:4 Identity digit 7 = [0]  - 0
  U8:4 Identity digit 6 = [f]  - f
  U8:4 Identity digit 9 = [0]  - 0
  U8:4 Identity digit 8 = [0]  - 0
  U8:4 Identity digit 11 = [1]   - 1
  U8:4 Identity digit 10 = [0]   - 0
  U8:4 Identity digit 13 = [3]   - 3
  U8:4 Identity digit 12 = [2]   - 2
  U8:4 Identity digit 15 = [f]   - f
  U8:4 Identity digit 16 = [4]   - 4

IMSI = 20810f00001234f (2081000001234)
*/

/*
        07 04 00 00 00 00 24 00 00 00 07 52 00 00 00 00
        00 00 00 00 00 00 00 00 00 01 02 03 04 10 00 00
        00 00 00 00 80 05 04 03 02 00 00 00 00 00
*/
/*
  U16 msgID = [07 04] - AS_DL_INFO_TRANSFER_IND
  U32 UEid = [00 00 00 00]

  U32 Length = [24 00 00 00] (36 octets)
  U8* NasMsg = [07 52 00 00 00 00 00 00 00 00 00 00 00 00 00 01]
         [02 03 04 10 00 00 00 00 00 00 80 05 04 03 02 00]
         [00 00 00 00]

  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [52]  - Authentication Request

  U8:4 NAS key set identifier = [0] - 000
  U8:4 spare = [0]
  RAND  = [00 00 00 00 00 00 00 00 00 00 00 00 01 02 03 04]
  AUTHN Length = [10] - 16 octets
  AUTHN = [00 00 00 00 00 00 80 05 04 03 02 00 00 00 00 00]
*/

/*
        06 01 00 00 00 00 0f 00 00 00 01 00 00 00 13 00
        00 00 07 53 10 67 70 3a 31 f2 2a 2d 51 00 00 00
        00 00 00 00 00
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [0f]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 13] (19 octets)
  U8* NasMsg = [07 53 10 67 70 3a 31 f2 2a 2d 51 00 00 00 00 00 00 00 00]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [53]  - Authentication Response

  RES Length = [10] - 16 octets
  RES = [67 70 3a 31 f2 2a 2d 51 00 00 00 00 00 00 00 00]
*/

/*
        07 04 00 00 00 00 10 00 00 00 37 ab ab ab ab 00
        07 5d 00 00 05 80 80 00 00 00
*/
/*
  U16 msgID = [07 04] - AS_DL_INFO_TRANSFER_IND
  U32 UEid = [00 00 00 00]

  U32 Length = [10 00 00 00] (16 octets)
  U8* NasMsg = [37 ab ab ab ab 00 07 5d 00 00 05 80 80 00 00 00]

  U8:4 Security header type = [3] - Integrity protected with
            new EPS security context
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - 0

  NAS message = [07 5d 00 00 05 80 80 00 00 00]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [5d]  - Security Mode Command

  U8   Selected NAS security algorithms = [00]
  U8:4 Spare half octet = [0]
  U8:4 NAS key set identifier = [0]
  Replayed UE security capabilities = [05 80 80 00 00 00]
  U8 EEA = [80] - EEA0
  U8 EIA = [80] - EIA0
  U8 UEA = [00]
  U8 UIA = [00]
  U8 GEA = [00]
*/

/*
        06 01 00 00 00 00 0f 00 00 00 01 00 00 00 08 00
        00 00 47 ab ab ab ab 00 07 5e
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [0f]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 08] (8 octets)
  U8* NasMsg = [47 ab ab ab ab 00 07 5e]

  U8:4 Security header type = [4] - Integrity protected and ciphered with
            new EPS security context
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [07 5e]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [5e]  - Security Mode Complete
*/

/*
        06 01 00 00 00 00 0f 00 00 00 01 00 00 00 09 00
        00 00 27 ab ab ab ab 00 07 5f 18
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [0f]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 09] (9 octets)
  U8* NasMsg = [27 ab ab ab ab 00 07 5f 18]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [07 5f 18]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [5f]  - Security Mode Reject

  U8   EMM cause code = [18]    - EMM_CAUSE_SECURITY_MODE_REJECTED
*/

/*
        07 01 00 00 00 00 00 00 00 00 00 00 00 00 04 00
        00 00 52 00 e8 6f
*/
/*
  U16 msgID = [07 01] - AS_DL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [00]   - !!!
  U32 m_tmsi = [00 00 00 00]  - !!!

  U32 Length = [00 00 00 04] (4 octets)
  NAS message = [52 00 e8 6f]

  U8:4 EPS bearer identity = [5]  - ebi = 5
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [00]  - unassigned
  U8   ESM message identifier = [e8]  - ESM Status

  U8   ESM cause code = [6f]    - #111 PROTOCOL_ERROR
*/

/*
        06 01 00 00 00 00 12 00 00 00 01 00 00 00 0a 00
        00 00 27 ab ab ab ab 00 02 01 e8 2b
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [12]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 0a] (10 octets)
  U8* NasMsg = [27 ab ab ab ab 00 02 01 e8 2b]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [02 01 e8 2b]
  U8:4 EPS bearer identity = [0]  - Not assigned
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [01]
  U8   ESM message identifier = [e8]  - ESM Status

  U8   ESM cause code = [2b]    - #43 INVALID_EPS_BEARER_IDENTITY
*/

/*
        06 01 00 00 00 00 12 00 00 00 01 00 00 00 0a 00
        00 00 27 ab ab ab ab 00 02 03 d2 06
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [12]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 0a] (10 octets)
  U8* NasMsg = [27 ab ab ab ab 00 02 03 d2 06]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [02 03 d2 06]
  U8:4 EPS bearer identity = [0]  - Not assigned
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [03]
  U8   ESM message identifier = [d2]  - PDN Disconnect Request

  U8:4 Spare half octet = [0]
  U8:4 Linked EPS bearer identity = [6] - ebi = 6
*/

/*
        06 08 00 00 00 00 01 00 00 00
*/
/*
  U16 msgID = [06 08] - AS_UL_INFO_TRANSFER_CNF
  U32 UEid = [00 00 00 00]
  U8 errCode = [01] - SUCCESS
*/

/*
        06 01 00 00 00 00 12 00 00 00 01 00 00 00 09 00
        00 00 27 ab ab ab ab 00 82 00 ce
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [12]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 09] (9 octets)
  U8* NasMsg = [27 ab ab ab ab 00 82 00 ce]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [82 00 ce]
  U8:4 EPS bearer identity = [8]  - 8
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [00]
  U8   ESM message identifier = [ce]  - Deactivate EPS Bearer Context Accept
*/

/*
        07 01 01 00 00 00 00 00 00 00 00 00 00 00 0a 00
        00 00 27 ab ab ab ab 00 62 02 cd 24
*/
/*
  U16 msgID = [07 01] - AS_DL_INFO_TRANSFER_REQ
  U32 UEid = [01 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [00]
  U32 m_tmsi = [00 00 00 00]

  U32 Length = [00 00 00 0a] (10 octets)
  NAS message = [27 ab ab ab ab 00 62 02 cd 24]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [62 02 cd 24]
  U8:4 EPS bearer identity = [6]  - ebi = 6
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [02]
  U8   ESM message identifier = [cd]  - Deactivate EPS Bearer Context Request

  U8   ESM cause code = [24]    - #36 REGULAR_DEACTIVATION
*/

/*
        04 08 01 00 00 00 00 00 00 00
*/
/*
  U16 msgID = [04 08] - AS_NAS_ESTABLISH_CNF
  U8 errCode = [01] - SUCCESS
  U32 Length = [00 00 00 00] (0 octets)
*/


/*
        07 01 01 00 00 00 00 00 00 00 00 00 00 00 0e 00
        00 00 27 ab ab ab ab 00 72 00 c5 06 01 02 01 00
*/
/*
  U16 msgID = [07 01] - AS_DL_INFO_TRANSFER_REQ
  U32 UEid = [01 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [00]
  U32 m_tmsi = [00 00 00 00]

  U32 Length = [00 00 00 0e] (14 octets)
  NAS message = [27 ab ab ab ab 00 72 00 c5 06 01 02 01 00]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [72 00 c5 06 00 00 00 00]
  U8:4 EPS bearer identity = [7]  - ebi = 7
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity - Unassigned
  U8   ESM message identifier = [c5]  - Activate dedicated EPS Bearer Context Request

  U8:4 Spare half octet = [0]
  U8:4 Linked EPS bearer identity = [6]
  U8 QoS length = [01]
  U8 QCI = [02]
  U8 TFT length = [01]
  U8 TFT = [00]
*/

/*
        07 04 00 00 00 00 36 00 00 00 27 ab ab ab ab 00
        72 00 c5 06 01 02 29 22 11 02 11 10 c0 a8 0c 01
        ff ff ff 00 30 11 40 27 11 50 2e e1 22 03 11 10
        c0 a8 0c 01 ff ff ff 00 30 11 40 27 12 50 2e e2
*/
/*
  U16 msgID = [07 04] - AS_DL_INFO_TRANSFER_IND
  U32 UEid = [00 00 00 00]

  U32 Length = [36 00 00 00] (54 octets)
  U8* NasMsg = [27 ab ab ab ab 00 72 00 c5 06 01 02 29 22 11 02]
           [11 10 c0 a8 0c 01 ff ff ff 00 30 11 40 27 11 50]
         [2e e1 22 03 11 10 c0 a8 0c 01 ff ff ff 00 30 11]
         [40 27 12 50 2e e2]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - 0

  NAS message = [72 00 c5 06 01 02 29 22 11 02 11 10 c0 a8 0c 01]
      [ff ff ff 00 30 11 40 27 11 50 2e e1 22 03 11 10]
    [c0 a8 0c 01 ff ff ff 00 30 11 40 27 12 50 2e e2]
  U8:4 EPS bearer identity = [7]  - ebi = 7
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity - Unassigned
  U8   ESM message identifier = [c5]  - Activate dedicated EPS Bearer Context Request

  U8:4 Spare half octet = [0]
  U8:4 Linked EPS bearer identity = [6]
  U8 QoS length = [01]  - 1 octet
  U8 QCI = [02]
  U8 TFT length = [29]  - 41 octets
  U8 TFT = [22 11 02 11 10 c0 a8 0c 01 ff ff ff 00 30 11 40]
       [27 11 50 2e e1 22 03 11 10 c0 a8 0c 01 ff ff ff]
     [00 30 11 40 27 12 50 2e e2]
*/

/*
        06 01 00 00 00 00 12 00 00 00 01 00 00 00 0a 00
        00 00 27 ab ab ab ab 00 72 00 c7 2d
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [12]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 0a] (10 octets)
  U8* NasMsg = [27 ab ab ab ab 00 72 00 c7 2d]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [72 00 c7 2d]
  U8:4 EPS bearer identity = [7]  - 7
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [00]
  U8   ESM message identifier = [c7]  - Activate Dedicated EPS Bearer Context
              Reject

  U8   ESM cause code = [2d]    - SYNTACTICAL_ERROR_IN_PACKET_FILTER
*/

/*
        06 01 00 00 00 00 12 00 00 00 01 00 00 00 09 00
        00 00 27 ab ab ab ab 00 72 00 c6
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [12]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 09] (9 octets)
  U8* NasMsg = [27 ab ab ab ab 00 72 00 c6]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [72 00 c6]
  U8:4 EPS bearer identity = [7]  - 7
  U8:4 Protocol discriminator = [2] - ESM message
  U8   Procedure Transaction Identity = [00]
  U8   ESM message identifier = [c6]  - Activate Dedicated EPS Bearer Context
              Accept
*/

/*
        06 01 00 00 00 00 12 00 00 00 01 00 00 00 15 00
        00 00 27 ab ab ab ab 00 07 45 01 0b f6 02 f8 01
        01 02 12 00 00 00 01
*/
/*
  U16 msgID = [06 01] - AS_UL_INFO_TRANSFER_REQ
  U32 UEid = [00 00 00 00]
  as_stmsi_t s_tmsi
  U8 MMEcode = [12]
  U32 m_tmsi = [00 00 00 01]

  U32 Length = [00 00 00 15] (21 octets)
  U8* NasMsg = [27 ab ab ab ab 00 07 45 01 0b f6 02 f8 01 01 02]
         [12 00 00 00 01]

  U8:4 Security header type = [2] - Integrity protected and ciphered
  U8:4 Protocol discriminator = [7] - EMM message
  U32  Message authentication code = [ab ab ab ab]
  U8   Sequence number = [00]   - Uplink counter value

  NAS message = [07 45 01 0b f6 02 f8 01 01 02 12 00 00 00 01]
  U8:4 Security header type = [0] - Not protected
  U8:4 Protocol discriminator = [7] - EMM message
  U8   EMM message identifier = [45]  - Detach Request

  U8:4 NAS key set identifier = [0] - Native 0
  U8:4 Detach type = [1]    - EPS Detach

  Mobile identity = [0b f6 02 f8 01 01 02 12 00 00 00 01]
  Length = [0b] - 11 octets
  U8:4 = 1111 = [f]
  U8:1 odd/even indicator = [0]  - even number of identity digits
  U8:3 Type of identity = [110]  - GUTI
  plmn = [02 f8 01]    - 20810
  U16  mmegroupid = [01 02]
  U8   mmecode = [12]
  U32  mtmsi = [00 00 00 01]
*/
