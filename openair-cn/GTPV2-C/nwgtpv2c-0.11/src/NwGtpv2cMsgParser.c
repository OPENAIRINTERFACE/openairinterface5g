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

#include "NwTypes.h"
#include "NwUtils.h"
#include "NwGtpv2cLog.h"
#include "NwGtpv2c.h"
#include "NwGtpv2cPrivate.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"
#include "NwGtpv2cMsgParser.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate a gtpv2c message Parser.
 *
 * @param[in] hGtpcStackHandle : gtpv2c stack handle.
 * @param[in] msgType : Message type for this message parser.
 * @param[out] pthiz : Pointer to message parser handle.
 */

NwRcT
nwGtpv2cMsgParserNew( NW_IN NwGtpv2cStackHandleT hGtpcStackHandle,
                      NW_IN uint8_t     msgType,
                      NW_IN NwRcT (*ieReadCallback) (uint8_t ieType,
                          uint8_t ieLength,
                          uint8_t ieInstance,
                          uint8_t* ieValue,
                          void* ieReadCallbackArg),
                      NW_IN void* ieReadCallbackArg,
                      NW_IN NwGtpv2cMsgParserT **pthiz)
{
  NwGtpv2cMsgParserT* thiz;
  //  NW_GTPV2C_MALLOC(hGtpcStackHandle, sizeof(NwGtpv2cMsgParserT), thiz, NwGtpv2cMsgParserT*);
  thiz = (NwGtpv2cMsgParserT*) malloc (sizeof(NwGtpv2cMsgParserT));

  if(thiz) {
    memset(thiz, 0, sizeof(NwGtpv2cMsgParserT));
    thiz->msgType = msgType;
    thiz->hStack  = hGtpcStackHandle;
    *pthiz = thiz;
    thiz->ieReadCallback      = ieReadCallback;
    thiz->ieReadCallbackArg   = ieReadCallbackArg;
    return NW_OK;
  }

  return NW_FAILURE;
}

/**
 * Free a gtpv2c message parser.
 *
 * @param[in] hGtpcStackHandle : gtpv2c stack handle.
 * @param[in] thiz : Message parser handle.
 */

NwRcT
nwGtpv2cMsgParserDelete( NW_IN NwGtpv2cStackHandleT hGtpcStackHandle,
                         NW_IN NwGtpv2cMsgParserT* thiz)
{
  NW_GTPV2C_FREE(hGtpcStackHandle, thiz);
  return NW_OK;
}

NwRcT
nwGtpv2cMsgParserUpdateIeReadCallback( NW_IN NwGtpv2cMsgParserT* thiz,
                                       NW_IN NwRcT (*ieReadCallback) (uint8_t ieType,
                                           uint8_t ieLength,
                                           uint8_t ieInstance,
                                           uint8_t* ieValue,
                                           void* ieReadCallbackArg))
{
  if(thiz) {
    thiz->ieReadCallback      = ieReadCallback;
    return NW_OK;
  }

  return NW_FAILURE;
}

NwRcT
nwGtpv2cMsgParserUpdateIeReadCallbackArg( NW_IN NwGtpv2cMsgParserT* thiz,
    NW_IN void* ieReadCallbackArg)
{
  if(thiz) {
    thiz->ieReadCallbackArg   = ieReadCallbackArg;
    return NW_OK;
  }

  return NW_FAILURE;
}

NwRcT
nwGtpv2cMsgParserAddIe( NW_IN NwGtpv2cMsgParserT* thiz,
                        NW_IN uint8_t ieType,
                        NW_IN uint8_t ieInstance,
                        NW_IN uint8_t iePresence,
                        NW_IN NwRcT (*ieReadCallback) (uint8_t ieType,
                            uint8_t ieLength,
                            uint8_t ieInstance,
                            uint8_t* ieValue,
                            void* ieReadCallbackArg),
                        NW_IN void* ieReadCallbackArg)
{
  NW_ASSERT(thiz);

  if(thiz->ieParseInfo[ieType][ieInstance].iePresence == 0) {
    NW_ASSERT(ieInstance <= NW_GTPV2C_IE_INSTANCE_MAXIMUM);

    thiz->ieParseInfo[ieType][ieInstance].ieReadCallback      = ieReadCallback;
    thiz->ieParseInfo[ieType][ieInstance].ieReadCallbackArg   = ieReadCallbackArg;
    thiz->ieParseInfo[ieType][ieInstance].iePresence          = iePresence;

    if (iePresence == NW_GTPV2C_IE_PRESENCE_MANDATORY) {
      thiz->mandatoryIeCount++;
    }
  } else {
    NW_LOG(thiz->hStack, NW_LOG_LEVEL_ERRO, "Cannot add IE to parser for type %u and instance %u. IE info already exists!", ieType, ieInstance);
  }

  return NW_OK;
}

NwRcT
nwGtpv2cMsgParserUpdateIe( NW_IN NwGtpv2cMsgParserT* thiz,
                           NW_IN uint8_t ieType,
                           NW_IN uint8_t ieInstance,
                           NW_IN uint8_t iePresence,
                           NW_IN NwRcT (*ieReadCallback) (uint8_t ieType,
                               uint8_t ieLength,
                               uint8_t ieInstance,
                               uint8_t* ieValue,
                               void* ieReadCallbackArg),
                           NW_IN void* ieReadCallbackArg)
{
  NW_ASSERT(thiz);

  if(thiz->ieParseInfo[ieType][ieInstance].iePresence) {
    thiz->ieParseInfo[ieType][ieInstance].ieReadCallback      = ieReadCallback;
    thiz->ieParseInfo[ieType][ieInstance].ieReadCallbackArg   = ieReadCallbackArg;

    thiz->ieParseInfo[ieType][ieInstance].iePresence          = iePresence;
  } else {
    NW_LOG(thiz->hStack, NW_LOG_LEVEL_ERRO, "Cannot update IE info for type %u and instance %u. IE info does not exist!", ieType, ieInstance);
  }


  return NW_OK;
}



NwRcT
nwGtpv2cMsgParserRun( NW_IN NwGtpv2cMsgParserT *thiz,
                      NW_IN NwGtpv2cMsgHandleT  hMsg,
                      NW_OUT uint8_t             *pOffendingIeType,
                      NW_OUT uint8_t             *pOffendingIeInstance,
                      NW_OUT uint16_t            *pOffendingIeLength)
{
  NwRcT                 rc = NW_OK;
  uint8_t                 flags;
  uint16_t                mandatoryIeCount =0;
  NwGtpv2cIeTlvT        *pIe;
  uint8_t                 *pIeStart;
  uint8_t                 *pIeEnd;
  uint16_t                ieLength;
  NwGtpv2cMsgT          *pMsg = (NwGtpv2cMsgT*) hMsg;

  NW_ASSERT(pMsg);
  flags    = *((uint8_t*)(pMsg->msgBuf));
  pIeStart = (uint8_t *) (pMsg->msgBuf + (flags & 0x08 ? 12: 8));
  pIeEnd   = (uint8_t *) (pMsg->msgBuf + pMsg->msgLen);

  memset(thiz->pIe, 0, sizeof(uint8_t*) * (NW_GTPV2C_IE_TYPE_MAXIMUM) * (NW_GTPV2C_IE_INSTANCE_MAXIMUM));
  memset(pMsg->pIe, 0, sizeof(uint8_t*) * (NW_GTPV2C_IE_TYPE_MAXIMUM) * (NW_GTPV2C_IE_INSTANCE_MAXIMUM));

  while (pIeStart < pIeEnd) {
    pIe = (NwGtpv2cIeTlvT*) pIeStart;
    ieLength = ntohs(pIe->l);

    if(pIeStart + 4 + ieLength > pIeEnd) {
      *pOffendingIeType = pIe->t;
      *pOffendingIeLength = pIe->l;
      *pOffendingIeInstance = pIe->i;
      return NW_GTPV2C_MSG_MALFORMED;
    }

    if((thiz->ieParseInfo[pIe->t][pIe->i].iePresence)) {
      thiz->pIe[pIe->t][pIe->i] = (uint8_t*) pIeStart;
      pMsg->pIe[pIe->t][pIe->i] = (uint8_t*) pIeStart;

      NW_LOG(thiz->hStack, NW_LOG_LEVEL_DEBG, "Received IE %u of length %u!", pIe->t, ieLength);

      if((thiz->ieParseInfo[pIe->t][pIe->i].ieReadCallback) != NULL ) {
        rc = thiz->ieParseInfo[pIe->t][pIe->i].ieReadCallback(pIe->t, ieLength, pIe->i, pIeStart + 4, thiz->ieParseInfo[pIe->t][pIe->i].ieReadCallbackArg);

        if(NW_OK == rc) {
          if(thiz->ieParseInfo[pIe->t][pIe->i].iePresence == NW_GTPV2C_IE_PRESENCE_MANDATORY)
            mandatoryIeCount++;
        } else {
          NW_LOG(thiz->hStack, NW_LOG_LEVEL_ERRO, "Error while parsing IE %u with instance %u and length %u!", pIe->t, pIe->i, ieLength);
          break;
        }
      } else {
        if((thiz->ieReadCallback) != NULL ) {
          NW_LOG(thiz->hStack, NW_LOG_LEVEL_DEBG, "Received IE %u of length %u!", pIe->t, ieLength);
          rc = thiz->ieReadCallback(pIe->t, ieLength, pIe->i, pIeStart + 4, thiz->ieReadCallbackArg);

          if(NW_OK == rc) {
            if(thiz->ieParseInfo[pIe->t][pIe->i].iePresence == NW_GTPV2C_IE_PRESENCE_MANDATORY)
              mandatoryIeCount++;
          } else {
            NW_LOG(thiz->hStack, NW_LOG_LEVEL_ERRO, "Error while parsing IE %u of length %u!", pIe->t, ieLength);
            break;
          }
        } else {
          NW_LOG(thiz->hStack, NW_LOG_LEVEL_WARN, "No parse method defined for received IE type %u of length %u in message %u!", pIe->t, ieLength, thiz->msgType);
        }
      }
    } else {
      NW_LOG(thiz->hStack, NW_LOG_LEVEL_WARN, "Unexpected IE %u of length %u received in msg %u!", pIe->t, ieLength, thiz->msgType);
    }

    pIeStart += (ieLength + 4);
  }

  if((NW_OK == rc) && (mandatoryIeCount != thiz->mandatoryIeCount)) {
    uint16_t t, i;
    *pOffendingIeType     = 0;
    *pOffendingIeInstance = 0;
    *pOffendingIeLength   = 0;

    for(t = 0; t < NW_GTPV2C_IE_TYPE_MAXIMUM; t++) {
      for(i = 0; i < NW_GTPV2C_IE_INSTANCE_MAXIMUM; i++) {
        if(thiz->ieParseInfo[t][i].iePresence == NW_GTPV2C_IE_PRESENCE_MANDATORY) {
          if(thiz->pIe[t][i] == NULL) {
            *pOffendingIeType = t;
            *pOffendingIeInstance = i;
            return NW_GTPV2C_MANDATORY_IE_MISSING;
          }
        }
      }
    }

    NW_LOG(thiz->hStack, NW_LOG_LEVEL_WARN, "Unknown mandatory IE missing. Parser formed incorrectly! %u:%u", mandatoryIeCount, thiz->mandatoryIeCount);
    return NW_GTPV2C_MANDATORY_IE_MISSING;
  }

  return rc;
}

#ifdef __cplusplus
}
#endif


/*--------------------------------------------------------------------------*
 *                      E N D     O F    F I L E                            *
 *--------------------------------------------------------------------------*/

