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
Source    emm_cause.h

Version   0.1

Date    2013/01/30

Product   NAS stack

Subsystem EPS Mobility Management

Author    Frederic Maurel

Description Defines error cause code returned upon receiving unknown,
    unforeseen, and erroneous EPS mobility management protocol
    data.

*****************************************************************************/
#ifndef __EMM_CAUSE_H__
#define __EMM_CAUSE_H__

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * Cause code used to notify that the EPS mobility management procedure
 * has been successfully processed
 */
#define EMM_CAUSE_SUCCESS     (-1)

/*
 * Causes related to UE identification (TS 24.301 - Annex A1)
 */
#define EMM_CAUSE_IMSI_UNKNOWN_IN_HSS   2
#define EMM_CAUSE_ILLEGAL_UE      3
#define EMM_CAUSE_ILLEGAL_ME      6
#define EMM_CAUSE_INVALID_UE      9
#define EMM_CAUSE_IMPLICITLY_DETACHED   10

/*
 * Causes related to subscription options (TS 24.301 - Annex A2)
 */
#define EMM_CAUSE_IMEI_NOT_ACCEPTED   5
#define EMM_CAUSE_EPS_NOT_ALLOWED   7
#define EMM_CAUSE_BOTH_NOT_ALLOWED    8
#define EMM_CAUSE_PLMN_NOT_ALLOWED    11
#define EMM_CAUSE_TA_NOT_ALLOWED    12
#define EMM_CAUSE_ROAMING_NOT_ALLOWED   13
#define EMM_CAUSE_EPS_NOT_ALLOWED_IN_PLMN 14
#define EMM_CAUSE_NO_SUITABLE_CELLS   15
#define EMM_CAUSE_CSG_NOT_AUTHORIZED    25
#define EMM_CAUSE_NOT_AUTHORIZED_IN_PLMN  35
#define EMM_CAUSE_NO_EPS_BEARER_CTX_ACTIVE  40

/*
 * Causes related to PLMN specific network failures and congestion/
 * authentication failures (TS 24.301 - Annex A3)
 */
#define EMM_CAUSE_MSC_NOT_REACHABLE   16
#define EMM_CAUSE_NETWORK_FAILURE   17
#define EMM_CAUSE_CS_DOMAIN_NOT_AVAILABLE 18
#define EMM_CAUSE_ESM_FAILURE     19
#define EMM_CAUSE_MAC_FAILURE     20
#define EMM_CAUSE_SYNCH_FAILURE     21
#define EMM_CAUSE_CONGESTION      22
#define EMM_CAUSE_UE_SECURITY_MISMATCH    23
#define EMM_CAUSE_SECURITY_MODE_REJECTED  24
#define EMM_CAUSE_NON_EPS_AUTH_UNACCEPTABLE 26
#define EMM_CAUSE_CS_SERVICE_NOT_AVAILABLE  39

/*
 * Causes related to invalid messages  (TS 24.301 - Annex A5)
 */
#define EMM_CAUSE_SEMANTICALLY_INCORRECT  95
#define EMM_CAUSE_INVALID_MANDATORY_INFO  96
#define EMM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED  97
#define EMM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE 98
#define EMM_CAUSE_IE_NOT_IMPLEMENTED    99
#define EMM_CAUSE_CONDITIONAL_IE_ERROR    100
#define EMM_CAUSE_MESSAGE_NOT_COMPATIBLE  101
#define EMM_CAUSE_PROTOCOL_ERROR    111

/*
 * TS 24.301 - Table 9.9.3.9.1
 * Any other value received by the mobile station shall be treated as cause
 * code #111 "protocol error, unspecified".
 * Any other value received by the network shall be treated as cause code #111
 * "protocol error, unspecified".
 */

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* __EMM_CAUSE_H__*/
