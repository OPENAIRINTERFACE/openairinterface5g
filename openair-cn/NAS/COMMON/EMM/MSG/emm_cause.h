/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
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
