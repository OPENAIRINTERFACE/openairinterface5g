/*----------------------------------------------------------------------------*
 *                                                                            *
 *                              n w - g t p v 2 c                             *
 *    G P R S   T u n n e l i n g    P r o t o c o l   v 2 c    S t a c k     *
 *                                                                            *
 *                                                                            *
 * Copyright (c) 2010-2011 Amit Chawre                                        *
 * All rights reserved.                                                       *
 *                                                                            *
 * Redistribution and use in source and binary forms, with or without         *
 * modification, are permitted provided that the following conditions         *
 * are met:                                                                   *
 *                                                                            *
 * 1. Redistributions of source code must retain the above copyright          *
 *    notice, this list of conditions and the following disclaimer.           *
 * 2. Redistributions in binary form must reproduce the above copyright       *
 *    notice, this list of conditions and the following disclaimer in the     *
 *    documentation and/or other materials provided with the distribution.    *
 * 3. The name of the author may not be used to endorse or promote products   *
 *    derived from this software without specific prior written permission.   *
 *                                                                            *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR       *
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES  *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.    *
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,           *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT   *
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY      *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT        *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF   *
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.          *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "NwTypes.h"
#include "NwLog.h"
#include "NwUtils.h"
#include "NwGtpv2cLog.h"
#include "NwGtpv2c.h"
#include "NwGtpv2cPrivate.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct NwGtpv2cMsgIeInfo {
  uint8_t ieType;
  uint8_t ieMinLength;
  uint8_t ieInstance;
  uint8_t iePresence;
  struct NwGtpv2cMsgIeInfo *pGroupedIeInfo;
} NwGtpv2cMsgIeInfoT;

static
NwGtpv2cMsgIeInfoT createSessionReqBearerCtxtTobeCreatedIeInfoTbl[] = {
  { NW_GTPV2C_IE_EBI             ,     1,    NW_GTPV2C_IE_INSTANCE_ZERO  , NW_GTPV2C_IE_PRESENCE_MANDATORY   , NULL},
  { NW_GTPV2C_IE_BEARER_TFT      ,     0,    NW_GTPV2C_IE_INSTANCE_ZERO  , NW_GTPV2C_IE_PRESENCE_OPTIONAL    , NULL},
  { NW_GTPV2C_IE_FTEID           ,     9,    NW_GTPV2C_IE_INSTANCE_ZERO  , NW_GTPV2C_IE_PRESENCE_CONDITIONAL , NULL},
  { NW_GTPV2C_IE_FTEID           ,     9,    NW_GTPV2C_IE_INSTANCE_ONE   , NW_GTPV2C_IE_PRESENCE_CONDITIONAL , NULL},
  { NW_GTPV2C_IE_FTEID           ,     9,    NW_GTPV2C_IE_INSTANCE_TWO   , NW_GTPV2C_IE_PRESENCE_CONDITIONAL , NULL},
  { NW_GTPV2C_IE_FTEID           ,     9,    NW_GTPV2C_IE_INSTANCE_THREE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL , NULL},
  { NW_GTPV2C_IE_FTEID           ,     9,    NW_GTPV2C_IE_INSTANCE_FOUR  , NW_GTPV2C_IE_PRESENCE_CONDITIONAL , NULL},
  { NW_GTPV2C_IE_BEARER_LEVEL_QOS,    18,    NW_GTPV2C_IE_INSTANCE_ZERO  , NW_GTPV2C_IE_PRESENCE_MANDATORY   , NULL},
  //   { NW_GTPV2C_IE_IMSI           ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO  , NW_GTPV2C_IE_PRESENCE_CONDITIONAL , NULL},

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT createSessionReqBearerCtxtTobeRemovedIeInfoTbl[] = {
  { NW_GTPV2C_IE_IMSI           ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT echoRspIeInfoTbl[] = {
  { NW_GTPV2C_IE_RECOVERY       ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY, NULL},

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT createSessionReqIeInfoTbl[] = {
  { NW_GTPV2C_IE_IMSI           ,       8,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_MSISDN         ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_MEI            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL},
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_RAT_TYPE       ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL},
  { NW_GTPV2C_IE_INDICATION     ,       3,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_APN            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL},
  { NW_GTPV2C_IE_APN_RESTRICTION,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL},
  { NW_GTPV2C_IE_SELECTION_MODE ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL},
  { NW_GTPV2C_IE_PDN_TYPE       ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL},
  { NW_GTPV2C_IE_PAA            ,       5,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL},
  { NW_GTPV2C_IE_AMBR           ,       8,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_EBI            ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_PCO            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_TRACE_INFORMATION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_FQ_CSID        ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_FQ_CSID        ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_BEARER_CONTEXT ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , createSessionReqBearerCtxtTobeCreatedIeInfoTbl},
  { NW_GTPV2C_IE_BEARER_CONTEXT ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, createSessionReqBearerCtxtTobeRemovedIeInfoTbl},
  { NW_GTPV2C_IE_SERVING_NETWORK,       3,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL},
  { NW_GTPV2C_IE_RECOVERY       ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_CHARGING_CHARACTERISTICS, 0,   NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_LDN            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },
  { NW_GTPV2C_IE_LDN            ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT modifyBearerReqIeInfoTbl[] = {
  { NW_GTPV2C_IE_MEI            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_SERVING_NETWORK,       3,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL},
  { NW_GTPV2C_IE_ULI            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_RAT_TYPE       ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_INDICATION     ,       3,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_BEARER_CONTEXT ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_BEARER_CONTEXT ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT deleteSessionReqIeInfoTbl[] = {
  { NW_GTPV2C_IE_CAUSE          ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_EBI            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_ULI            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_INDICATION     ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_PCO            ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_RECOVERY       ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT createSessionRspIeInfoTbl[] = {
  { NW_GTPV2C_IE_CAUSE          ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_PAA            ,       5,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_APN_RESTRICTION,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_BEARER_CONTEXT ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_BEARER_CONTEXT ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_RECOVERY       ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT modifyBearerRspIeInfoTbl[] = {
  { NW_GTPV2C_IE_CAUSE          ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_PAA            ,       5,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_APN_RESTRICTION,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_BEARER_CONTEXT ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_RECOVERY       ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT deleteSessionRspIeInfoTbl[] = {
  { NW_GTPV2C_IE_CAUSE          ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_RECOVERY       ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_PCO            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

/* Message sent over s10 interface */
static
NwGtpv2cMsgIeInfoT identificationReqIeInfoTbl[] = {
  { NW_GTPV2C_IE_GUTI           ,          6,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_COMPLETE_REQUEST_MESSAGE, 1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_SERVING_NETWORK,          0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,        0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT identificationRspIeInfoTbl[] = {
  { NW_GTPV2C_IE_CAUSE          ,          0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_IMSI           ,          8,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_MM_CONTEXT     ,          1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_TRACE_INFORMATION,        0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,        0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT contextReqIeInfoTbl[] = {
  { NW_GTPV2C_IE_IMSI           ,          8,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_GUTI           ,          5,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_ULI            ,          7,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_PTMSI          ,          0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_COMPLETE_REQUEST_MESSAGE, 0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_FTEID          ,          9,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_UDP_SOURCE_PORT_NUMBER,   9,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_RAT_TYPE       ,          1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_INDICATION     ,          0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },
  { NW_GTPV2C_IE_HOP_COUNTER    ,          0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },
  { NW_GTPV2C_IE_SERVING_NETWORK,          3,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_LDN            ,          3,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,        0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT contextRspIeInfoTbl[] = {
  { NW_GTPV2C_IE_CAUSE          ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY           , NULL },
  { NW_GTPV2C_IE_IMSI           ,       8,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL         , NULL },
  { NW_GTPV2C_IE_MM_CONTEXT     ,       4,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL         , NULL },
  { NW_GTPV2C_IE_PDN_CONNECTION ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL         , NULL },
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL         , NULL },
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL         , NULL },
  { NW_GTPV2C_IE_FQDN           ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL         , NULL },
  { NW_GTPV2C_IE_INDICATION     ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL         , NULL },
  { NW_GTPV2C_IE_TRACE_INFORMATION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL         , NULL },
  { NW_GTPV2C_IE_IP_ADDRESS     ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL         , NULL },
  { NW_GTPV2C_IE_IP_ADDRESS     ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL         , NULL },
  { NW_GTPV2C_IE_RFSP_INDEX     ,       2,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_RFSP_INDEX     ,       2,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_UE_TIME_ZONE   ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_LDN            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL            , NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL            , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT contextAckIeInfoTbl[] = {
  { NW_GTPV2C_IE_CAUSE          ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_INDICATION     ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT forwardRelocationReqIeInfoTbl[] = {
  { NW_GTPV2C_IE_IMSI           ,       8,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_PDN_CONNECTION ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_FQDN           ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_MM_CONTEXT     ,       4,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_INDICATION     ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CONTAINER    ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CONTAINER    ,       1,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_TARGET_IDENTIFICATION, 0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_IP_ADDRESS     ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_IP_ADDRESS     ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CAUSE        ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CAUSE        ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CONTAINER    ,       1,      NW_GTPV2C_IE_INSTANCE_TWO , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_SOURCE_IDENTIFICATION, 1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CAUSE        ,       0,      NW_GTPV2C_IE_INSTANCE_TWO , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_SELECTED_PLMN_ID,      0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_RECOVERY       ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_TRACE_INFORMATION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_RFSP_INDEX     ,       2,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_RFSP_INDEX     ,       2,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_CSG_ID         ,       4,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_CSG_MEMBERSHIP_INDICATION, 1,  NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_UE_TIME_ZONE   ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_LDN            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },
  { NW_GTPV2C_IE_ADDITIONAL_MM_CTXT_FOR_SRVCC,0,NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_ADDITIONAL_FLAGS_FOR_SRVCC, 0, NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_MSISDN         ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL_OPTIONAL, NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0 }
};

static
NwGtpv2cMsgIeInfoT forwardRelocationRspIeInfoTbl[] = {
  { NW_GTPV2C_IE_CAUSE          ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_FTEID          ,       9,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_INDICATION     ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_BEARER_CONTEXT ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_BEARER_CONTEXT ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_BEARER_CONTEXT ,       0,      NW_GTPV2C_IE_INSTANCE_TWO , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CAUSE        ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CAUSE        ,       0,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CONTAINER    ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CONTAINER    ,       1,      NW_GTPV2C_IE_INSTANCE_ONE , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CONTAINER    ,       1,      NW_GTPV2C_IE_INSTANCE_TWO , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_F_CAUSE        ,       0,      NW_GTPV2C_IE_INSTANCE_TWO , NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_LDN            ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT forwardRelocationCompleteNtfIeInfoTbl[] = {
  { NW_GTPV2C_IE_INDICATION     ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_CONDITIONAL, NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

static
NwGtpv2cMsgIeInfoT forwardRelocationCompleteAckIeInfoTbl[] = {
  { NW_GTPV2C_IE_CAUSE          ,       0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_MANDATORY  , NULL },
  { NW_GTPV2C_IE_RECOVERY       ,       1,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },
  { NW_GTPV2C_IE_PRIVATE_EXTENSION,     0,      NW_GTPV2C_IE_INSTANCE_ZERO, NW_GTPV2C_IE_PRESENCE_OPTIONAL   , NULL },

  /* Do not add below this */
  { 0, 0, 0}
};

/*----------------------------------------------------------------------------*
 *                     P R I V A T E     F U N C T I O N S                    *
 *----------------------------------------------------------------------------*/

static uint32_t
nwGtpv2cMsgGroupedIeCount(NwGtpv2cMsgIeInfoT *pMsgIeInfo)
{
  uint32_t count = 0;
  NwGtpv2cMsgIeInfoT *pGroupedIeInfo = pMsgIeInfo;

  while (pGroupedIeInfo++) {
    if (pGroupedIeInfo->ieType == 0) {
      break;
    }

    count ++;
  }

  return count;
}

static NwRcT
nwGtpv2cMsgIeParseInfoUpdate(NwGtpv2cMsgIeParseInfoT *thiz,
                             NwGtpv2cMsgIeInfoT      *pMsgIeInfo)
{
  uint32_t i, j;

  for (i = 0; pMsgIeInfo[i].ieType; i++) {
    if (pMsgIeInfo[i].pGroupedIeInfo) {
      NwGtpv2cGroupedIeParseInfoT *pMsgIeParseInfo;

      NW_GTPV2C_MALLOC(thiz->hStack, sizeof(NwGtpv2cGroupedIeParseInfoT),
                       pMsgIeParseInfo, NwGtpv2cGroupedIeParseInfoT*);

      pMsgIeParseInfo->groupedIeType = pMsgIeInfo[i].ieType;
      pMsgIeParseInfo->hStack = thiz->hStack;

      for (j = 0; pMsgIeInfo[i].pGroupedIeInfo[j].ieType; j++) {
        pMsgIeParseInfo->ieParseInfo[pMsgIeInfo[i].pGroupedIeInfo[j].ieType][pMsgIeInfo[i].pGroupedIeInfo[j].ieInstance].ieMinLength =
          pMsgIeInfo[i].pGroupedIeInfo[j].ieMinLength;
        pMsgIeParseInfo->ieParseInfo[pMsgIeInfo[i].pGroupedIeInfo[j].ieType][pMsgIeInfo[i].pGroupedIeInfo[j].ieInstance].iePresence =
          pMsgIeInfo[i].pGroupedIeInfo[j].iePresence;

        if(pMsgIeInfo[i].pGroupedIeInfo[j].iePresence == NW_GTPV2C_IE_PRESENCE_MANDATORY)
          pMsgIeParseInfo->mandatoryIeCount++;
      }

      thiz->ieParseInfo[pMsgIeInfo[i].ieType][pMsgIeInfo[i].ieInstance].pGroupedIeInfo = pMsgIeParseInfo;
    } else {
      thiz->ieParseInfo[pMsgIeInfo[i].ieType][pMsgIeInfo[i].ieInstance].pGroupedIeInfo = NULL;
    }

    thiz->ieParseInfo[pMsgIeInfo[i].ieType][pMsgIeInfo[i].ieInstance].iePresence     = pMsgIeInfo[i].iePresence;
    thiz->ieParseInfo[pMsgIeInfo[i].ieType][pMsgIeInfo[i].ieInstance].ieMinLength    = pMsgIeInfo[i].ieMinLength;

    if(pMsgIeInfo[i].iePresence == NW_GTPV2C_IE_PRESENCE_MANDATORY)
      thiz->mandatoryIeCount++;
  }

  return NW_OK;
}

static NwRcT
nwGtpv2cMsgGroupedIeParse(NW_IN NwGtpv2cGroupedIeParseInfoT* thiz,
                          NW_IN uint8_t  ieType,
                          NW_IN uint16_t ieLength,
                          NW_IN uint8_t  ieInstance,
                          NW_IN uint8_t  *pIeValue)
{
  NW_ASSERT(thiz);

  NW_LOG(thiz->hStack, NW_LOG_LEVEL_DEBG, "Received grouped IE %u with instance %u of length %u in msg-type %u!",
         ieType, ieInstance, ieLength, thiz->groupedIeType);

  return NW_OK;
}

/*----------------------------------------------------------------------------*
 *                       P U B L I C   F U N C T I O N S                      *
 *----------------------------------------------------------------------------*/

/**
 * Constructor
 * @return Pointer to the object on success.
 */

NwGtpv2cMsgIeParseInfoT*
nwGtpv2cMsgIeParseInfoNew(NwGtpv2cStackHandleT hStack, uint8_t msgType)
{
  NwRcT rc;
  NwGtpv2cMsgIeParseInfoT *thiz;

  NW_GTPV2C_MALLOC(hStack, sizeof(NwGtpv2cMsgIeParseInfoT), thiz, NwGtpv2cMsgIeParseInfoT*);

  if(thiz) {
    memset(thiz, 0, sizeof(NwGtpv2cMsgIeParseInfoT));

    thiz->hStack  = hStack;
    thiz->msgType = msgType;

    switch(msgType) {
    case NW_GTP_ECHO_RSP: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, echoRspIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_CREATE_SESSION_REQ: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, createSessionReqIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_CREATE_SESSION_RSP: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, createSessionRspIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_DELETE_SESSION_REQ: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, deleteSessionReqIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_DELETE_SESSION_RSP: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, deleteSessionRspIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_MODIFY_BEARER_REQ: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, modifyBearerReqIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_MODIFY_BEARER_RSP: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, modifyBearerRspIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_FORWARD_RELOCATION_REQ: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, forwardRelocationReqIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_FORWARD_RELOCATION_RSP: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, forwardRelocationRspIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_FORWARD_RELOCATION_COMPLETE_NTF: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, forwardRelocationCompleteNtfIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_FORWARD_RELOCATION_COMPLETE_ACK: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, forwardRelocationCompleteAckIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_CONTEXT_REQ: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, contextReqIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_CONTEXT_RSP: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, contextRspIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_CONTEXT_ACK: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, contextAckIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_IDENTIFICATION_REQ: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, identificationReqIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    case NW_GTP_IDENTIFICATION_RSP: {
      rc = nwGtpv2cMsgIeParseInfoUpdate(thiz, identificationRspIeInfoTbl);
      NW_ASSERT(NW_OK == rc);
    }
    break;

    default: {
      free(thiz);
      thiz = NULL;
    }
    break;
    }
  }

  return thiz;
}

/**
 * Destructor
 * @return NW_OK on success.
 */

NwRcT
nwGtpv2cMsgIeParseInfoDelete(NwGtpv2cMsgIeParseInfoT* thiz)
{
  NW_GTPV2C_FREE(thiz->hStack, thiz);
  return NW_OK;
}

/**
 * Parse message
 * @return NW_OK on success.
 */

NwRcT
nwGtpv2cMsgIeParse(NW_IN NwGtpv2cMsgIeParseInfoT* thiz,
                   NW_IN       NwGtpv2cMsgHandleT hMsg,
                   NW_INOUT    NwGtpv2cErrorT     *pError)
{
  NwRcT                 rc = NW_OK;
  uint16_t                mandatoryIeCount =0;
  uint8_t                 *pIeBufStart;
  uint8_t                 *pIeBufEnd;
  uint16_t                ieType;
  uint16_t                ieLength;
  uint16_t                ieInstance;
  NwGtpv2cIeTlvT        *pIe;
  NwGtpv2cMsgT          *pMsg = (NwGtpv2cMsgT*) hMsg;
  uint8_t                 flags = *((uint8_t*)(pMsg->msgBuf));

  pIeBufStart = (uint8_t *) (pMsg->msgBuf + (flags & 0x08 ? 12: 8));
  pIeBufEnd   = (uint8_t *) (pMsg->msgBuf + pMsg->msgLen);

  //memset(pMsg->pIe, 0, sizeof(uint8_t*) * (NW_GTPV2C_IE_TYPE_MAXIMUM) * (NW_GTPV2C_IE_INSTANCE_MAXIMUM));
  memset(pMsg->isIeValid, (NW_FALSE), sizeof(uint8_t) * (NW_GTPV2C_IE_TYPE_MAXIMUM) * (NW_GTPV2C_IE_INSTANCE_MAXIMUM));

  while (pIeBufStart < pIeBufEnd) {
    pIe         = (NwGtpv2cIeTlvT*) pIeBufStart;
    ieType      = pIe->t;
    ieLength    = ntohs(pIe->l);
    ieInstance  = pIe->i & 0x0F;

    NW_ASSERT(NW_GTPV2C_IE_INSTANCE_MAXIMUM >= ieInstance);

    NW_LOG(thiz->hStack, NW_LOG_LEVEL_DEBG, "Received IE %u with instance %u of length %u in msg-type %u!", ieType, ieInstance, ieLength, thiz->msgType);

    if(pIeBufStart + 4 + ieLength > pIeBufEnd) {
      NW_LOG(thiz->hStack, NW_LOG_LEVEL_ERRO, "Invalid length for IE of type %u and instance %u!", ieType, ieInstance);
      pError->cause                     = NW_GTPV2C_CAUSE_INVALID_LENGTH;
      pError->offendingIe.type          = ieType;
      pError->offendingIe.instance      = ieInstance;
      return NW_FAILURE;
    }

    if((thiz->ieParseInfo[ieType][ieInstance].iePresence)) {
      if((ieLength < (thiz->ieParseInfo[ieType][ieInstance].ieMinLength))) {
        if(thiz->ieParseInfo[ieType][ieInstance].iePresence == NW_GTPV2C_IE_PRESENCE_OPTIONAL) {
          /* Ignore TLV */
          pIeBufStart += (ieLength + 4);
          continue;
        } else {
          pError->cause                     = NW_GTPV2C_CAUSE_MANDATORY_IE_INCORRECT;
          pError->offendingIe.type          = ieType;
          pError->offendingIe.instance      = ieInstance;
          NW_LOG(thiz->hStack, NW_LOG_LEVEL_ERRO, "Mandatory IE of type %u and instance %u incorrect!", ieType, ieInstance);
          return NW_FAILURE;
        }
      }

      if(pMsg->isIeValid[ieType][ieInstance] == NW_TRUE) {
        /*
         * If an information element is repeated in a GTP signalling
         * message in which repetition of the information element is
         * not specified, only the contents of the information element
         * appearing first shall be handled and all subsequent repetitions
         * of the information element shall be ignored.
         * TODO: Add handling for IEs for which repetition is allowed.
         */
        pIeBufStart += (ieLength + 4);
        continue;
      }

      pMsg->pIe[ieType][ieInstance] = (uint8_t*) pIeBufStart;
      pMsg->isIeValid[ieType][ieInstance] = NW_TRUE;

      if(thiz->ieParseInfo[ieType][ieInstance].pGroupedIeInfo) {
        /* Parse the grouped IE */
        rc = nwGtpv2cMsgGroupedIeParse(thiz->ieParseInfo[ieType][ieInstance].pGroupedIeInfo, ieType, ieLength, ieInstance, ((uint8_t*) pIe) + 4);

        if (rc != NW_OK) {
          pError->cause                     = NW_GTPV2C_CAUSE_MANDATORY_IE_INCORRECT;
          pError->offendingIe.type          = ieType;
          pError->offendingIe.instance      = ieInstance;
          NW_LOG(thiz->hStack, NW_LOG_LEVEL_ERRO, "Mandatory IE of type %u and instance %u incorrect!", ieType, ieInstance);
          return NW_FAILURE;
        }
      }

      if(thiz->ieParseInfo[ieType][ieInstance].iePresence == NW_GTPV2C_IE_PRESENCE_MANDATORY) {
        mandatoryIeCount++;
      }
    } else {
      NW_LOG(thiz->hStack, NW_LOG_LEVEL_WARN, "Unexpected IE %u with instance %u of length %u received in msg %u!", ieType, ieInstance, ieLength, thiz->msgType);
    }

    pIeBufStart += (ieLength + 4);
  }

  if((NW_OK == rc) && (mandatoryIeCount != thiz->mandatoryIeCount)) {
    for(ieType = 0; ieType < NW_GTPV2C_IE_TYPE_MAXIMUM; ieType++) {
      for(ieInstance = 0; ieInstance < NW_GTPV2C_IE_INSTANCE_MAXIMUM; ieInstance++) {
        if(thiz->ieParseInfo[ieType][ieInstance].iePresence == NW_GTPV2C_IE_PRESENCE_MANDATORY) {
          if(pMsg->isIeValid[ieType][ieInstance] == NW_FALSE) {
            NW_LOG(thiz->hStack, NW_LOG_LEVEL_ERRO, "Mandatory IE of type %u and instance %u missing in msg type %u", ieType, ieInstance, pMsg->msgType);
            pError->cause = NW_GTPV2C_CAUSE_MANDATORY_IE_MISSING;
            pError->offendingIe.type          = ieType;
            pError->offendingIe.instance      = ieInstance;
            return NW_FAILURE;
          }
        }
      }
    }

    NW_LOG(thiz->hStack, NW_LOG_LEVEL_CRIT, "Unknown mandatory IE missing. Msg parser formed incorrectly!");
    pError->cause = NW_GTPV2C_CAUSE_MANDATORY_IE_MISSING;
    pError->offendingIe.type          = 0;
    pError->offendingIe.instance      = 0;
    return NW_FAILURE;
  }

  pError->cause = NW_GTPV2C_CAUSE_REQUEST_ACCEPTED;
  return NW_OK;
}


#ifdef __cplusplus
}
#endif

/*--------------------------------------------------------------------------*
 *                          E N D   O F   F I L E                           *
 *--------------------------------------------------------------------------*/

