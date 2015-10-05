/*----------------------------------------------------------------------------*
 *                                                                            *
 *                             n w - g t p v 2 u                              *
 *    G P R S   T u n n e l i n g    P r o t o c o l   v 2 u    S t a c k     *
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
#include <ctype.h>

#include "NwTypes.h"
#include "NwLog.h"
#include "NwUtils.h"
#include "NwGtpv1uLog.h"
#include "NwGtpv1u.h"
#include "NwGtpv1uPrivate.h"
#include "NwGtpv1uTrxn.h"

/*--------------------------------------------------------------------------*
 *                 P R I V A T E  D E C L A R A T I O N S                   *
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

static NwGtpv1uTrxnT *gpGtpv1uTrxnPool = NULL;

/*--------------------------------------------------------------------------*
 *                   P R I V A T E      F U N C T I O N S                   *
 *--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * Send msg retransmission to peer via data request to UDP Entity
 *--------------------------------------------------------------------------*/

static NwGtpv1uRcT
nwGtpv1uTrxnSendMsgRetransmission(NwGtpv1uTrxnT *thiz)
{
  NwGtpv1uRcT rc;

  NW_ASSERT(thiz);
  NW_ASSERT(thiz->pMsg);

  rc = thiz->pStack->udp.udpDataReqCallback(thiz->pStack->udp.hUdp,
       thiz->pMsg->msgBuf,
       thiz->pMsg->msgLen,
       thiz->pMsg->msgBufOffset,
       thiz->peerIp,
       thiz->peerPort);

  return rc;
}

static NwGtpv1uRcT
nwGtpv1uTrxnPeerRspTimeout(void *arg)
{
  NwGtpv1uRcT rc = NW_GTPV1U_OK;
  NwGtpv1uTrxnT *thiz;
  NwGtpv1uStackT *pStack;
  NwGtpv1uTimeoutInfoT *timeoutInfo = arg;

  thiz = ((NwGtpv1uTrxnT *)timeoutInfo->timeoutArg);
  pStack = thiz->pStack;

  NW_ASSERT(pStack);

  NW_LOG(pStack, NW_LOG_LEVEL_WARN, "T3 timer expired for transaction 0x%p",
         thiz);

  rc = nwGtpv1uTrxnSendMsgRetransmission(thiz);

  if(thiz->maxRetries) {
    rc = pStack->tmrMgr.tmrStartCallback(pStack->tmrMgr.tmrMgrHandle,
                                         thiz->t3Timer, 0, NW_GTPV1U_TMR_TYPE_ONE_SHOT, (void *)timeoutInfo,
                                         &thiz->hRspTmr);
    thiz->maxRetries--;
  } else {
    NwGtpv1uUlpApiT ulpApi;
    ulpApi.apiType                      = NW_GTPV1U_ULP_API_RSP_FAILURE;
    ulpApi.apiInfo.recvMsgInfo.msgType  = nwGtpv1uMsgGetMsgType((
                                            NwGtpv1uMsgHandleT)thiz->pMsg);
    ulpApi.apiInfo.recvMsgInfo.hUlpTrxn = thiz->hUlpTrxn;
    ulpApi.apiInfo.recvMsgInfo.peerIp   = thiz->peerIp;
    ulpApi.apiInfo.recvMsgInfo.peerPort = thiz->peerPort;
    thiz->hRspTmr = 0;

    rc = nwGtpv1uTrxnDelete(&thiz);
    NW_ASSERT(rc == NW_GTPV1U_OK);

    rc = pStack->ulp.ulpReqCallback(pStack->ulp.hUlp, &ulpApi);
    NW_ASSERT(rc == NW_GTPV1U_OK);
  }

  return rc;
}

/**
  Send timer start request to TmrMgr Entity.

  @param[in] thiz : Pointer to transaction
  @param[in] timeoutCallbackFunc : Timeout handler callback function.
  @return NW_GTPV1U_OK on success.
 */

static NwGtpv1uRcT
nwGtpv1uTrxnStartPeerRspTimer(NwGtpv1uTrxnT *thiz,
                              NwGtpv1uRcT (*timeoutCallbackFunc)(void *))
{
  NwGtpv1uRcT rc;
  NwGtpv1uTimeoutInfoT *timeoutInfo;

  NW_ASSERT(thiz->pStack->tmrMgr.tmrStartCallback != NULL);

  timeoutInfo                           = &thiz->peerRspTimeoutInfo;
  timeoutInfo->timeoutArg               = thiz;
  timeoutInfo->timeoutCallbackFunc      = timeoutCallbackFunc;
  timeoutInfo->hStack                   = (NwGtpv1uStackHandleT)thiz->pStack;

  rc = thiz->pStack->tmrMgr.tmrStartCallback(thiz->pStack->tmrMgr.tmrMgrHandle,
       thiz->t3Timer, 0, NW_GTPV1U_TMR_TYPE_ONE_SHOT, (void *)timeoutInfo,
       &thiz->hRspTmr);

  return rc;
}

/**
  Send timer stop request to TmrMgr Entity.

  @param[in] thiz : Pointer to transaction
  @return NW_GTPV1U_OK on success.
 */

static NwGtpv1uRcT
nwGtpv1uTrxnStopPeerRspTimer(NwGtpv1uTrxnT *thiz)
{
  NwGtpv1uRcT rc;

  NW_ASSERT(thiz->pStack->tmrMgr.tmrStopCallback != NULL);

  rc = thiz->pStack->tmrMgr.tmrStopCallback(thiz->pStack->tmrMgr.tmrMgrHandle,
       thiz->hRspTmr);

  thiz->hRspTmr = 0;

  return rc;
}

/*--------------------------------------------------------------------------*
 *                      P U B L I C    F U N C T I O N S                    *
 *--------------------------------------------------------------------------*/

/**
 * Constructor
 *
 * @param[in] thiz : Pointer to stack
 * @param[out] ppTrxn : Pointer to pointer to Trxn object.
 * @return NW_GTPV1U_OK on success.
 */
NwGtpv1uRcT
nwGtpv1uTrxnNew( NW_IN  NwGtpv1uStackT *thiz,
                 NW_OUT NwGtpv1uTrxnT **ppTrxn)
{
  NwGtpv1uRcT rc = NW_GTPV1U_OK;
  NwGtpv1uTrxnT *pTrxn;

  if(gpGtpv1uTrxnPool) {
    pTrxn = gpGtpv1uTrxnPool;
    gpGtpv1uTrxnPool = gpGtpv1uTrxnPool->next;
  } else {
    NW_GTPV1U_MALLOC(thiz, sizeof(NwGtpv1uTrxnT), pTrxn, NwGtpv1uTrxnT *);
  }

  if (pTrxn) {
    pTrxn->maxRetries   = 2;
    pTrxn->pStack       = thiz;
    pTrxn->t3Timer      = 2;
    pTrxn->seqNum       = thiz->seq;

    /* Increment sequence number */
    thiz->seq++;

    if(thiz->seq == 0x800000) {
      thiz->seq = 0;
    }

  } else {
    rc = NW_GTPV1U_FAILURE;
  }

  NW_LOG(thiz, NW_LOG_LEVEL_DEBG, "Created transaction 0x%p", pTrxn);

  *ppTrxn = pTrxn;

  return rc;
}

/**
 * Overloaded Constructor
 *
 * @param[in] thiz : Pointer to stack.
 * @param[in] seqNum : Sequence number for this transaction.
 * @param[out] ppTrxn : Pointer to pointer to Trxn object.
 * @return NW_GTPV1U_OK on success.
 */
NwGtpv1uRcT
nwGtpv1uTrxnWithSeqNew( NW_IN  NwGtpv1uStackT *thiz,
                        NW_IN  uint32_t seqNum,
                        NW_OUT NwGtpv1uTrxnT **ppTrxn)
{
  NwGtpv1uRcT rc = NW_GTPV1U_OK;
  NwGtpv1uTrxnT *pTrxn;

  if(gpGtpv1uTrxnPool) {
    pTrxn = gpGtpv1uTrxnPool;
    gpGtpv1uTrxnPool = gpGtpv1uTrxnPool->next;
  } else {
    NW_GTPV1U_MALLOC(thiz, sizeof(NwGtpv1uTrxnT), pTrxn, NwGtpv1uTrxnT *);
  }


  if (pTrxn) {
    pTrxn->maxRetries   = 2;
    pTrxn->pStack       = thiz;
    pTrxn->t3Timer      = 2;
    pTrxn->seqNum       = seqNum;
    pTrxn->pMsg         = NULL;
  } else {
    rc = NW_GTPV1U_FAILURE;
  }

  NW_LOG(thiz, NW_LOG_LEVEL_DEBG, "Created transaction 0x%p", pTrxn);

  *ppTrxn = pTrxn;

  return rc;
}

/**
 * Destructor
 *
 * @param[out] pthiz : Pointer to pointer to Trxn object.
 * @return NW_GTPV1U_OK on success.
 */
NwGtpv1uRcT
nwGtpv1uTrxnDelete( NW_INOUT NwGtpv1uTrxnT **pthiz)
{
  NwGtpv1uRcT rc = NW_GTPV1U_OK;
  NwGtpv1uStackT *pStack;
  NwGtpv1uTrxnT *thiz = *pthiz;

  pStack = thiz->pStack;

  if(thiz->hRspTmr) {
    rc = nwGtpv1uTrxnStopPeerRspTimer(thiz);
    NW_ASSERT(rc == NW_GTPV1U_OK);
  }

  if(thiz->pMsg) {
    rc = nwGtpv1uMsgDelete((NwGtpv1uStackHandleT)pStack,
                           (NwGtpv1uMsgHandleT)thiz->pMsg);
    NW_ASSERT(rc == NW_GTPV1U_OK);
  }

  thiz->next = gpGtpv1uTrxnPool;
  gpGtpv1uTrxnPool = thiz;

  NW_LOG(pStack, NW_LOG_LEVEL_DEBG, "Purged transaction 0x%p", thiz);

  *pthiz = NULL;
  return rc;
}


#if 1
/**
 * Send msg to peer via data request to UDP Entity
 *
 * @param[in] thiz : Pointer to stack.
 * @param[in] pTrxn : Pointer to Trxn object.
 * @param[in] peerIp : Peer Ip address.
 * @param[in] peerPort : Peer Ip port.
 * @param[in] pMsg : Message to be sent.
 * @return NW_GTPV1U_OK on success.
 */
NwGtpv1uRcT
nwGtpv1uTrxnCreateAndSendMsg( NW_IN  NwGtpv1uStackT *thiz,
                              NW_IN  NwGtpv1uTrxnT *pTrxn,
                              NW_IN  uint32_t peerIp,
                              NW_IN  uint32_t peerPort,
                              NW_IN  NwGtpv1uMsgT *pMsg)
{
  NwGtpv1uRcT rc;
  uint8_t *msgHdr;

  NW_ASSERT(thiz);
  NW_ASSERT(pMsg);

  msgHdr = &pMsg->msgBuf[pMsg->msgBufOffset];

  NW_ASSERT(msgHdr != NULL);

  *(msgHdr++)         = (pMsg->version << 5)            |
                        (pMsg->protocolType << 4)       |
                        (pMsg->extHdrFlag << 2)         |
                        (pMsg->seqNumFlag << 1)         |
                        (pMsg->npduNumFlag);

  *(msgHdr++)         = (pMsg->msgType);
  *((uint16_t *) msgHdr) = htons(pMsg->msgLen);
  msgHdr += 2;

  *((uint32_t *) msgHdr) = htonl(pMsg->teid);
  msgHdr += 4;

  if(pMsg->seqNumFlag | pMsg->extHdrFlag | pMsg->npduNumFlag) {
    if(pMsg->seqNumFlag) {
      *((uint16_t *) msgHdr) = htons((pTrxn ? pTrxn->seqNum : pMsg->seqNum));
    } else {
      *((uint16_t *) msgHdr) = 0x0000;
    }

    msgHdr += 2;

    if(pMsg->npduNumFlag) {
      *((uint8_t *) msgHdr) = pMsg->npduNumFlag;
    } else {
      *((uint8_t *) msgHdr) = 0x00;
    }

    msgHdr++;

    if(pMsg->extHdrFlag) {
      *((uint8_t *) msgHdr) = pMsg->extHdrFlag;
    } else {
      *((uint8_t *) msgHdr) = 0x00;
    }

    msgHdr++;
  }

  NW_ASSERT(thiz->udp.udpDataReqCallback != NULL);

  rc = thiz->udp.udpDataReqCallback(thiz->udp.hUdp,
                                    pMsg->msgBuf,
                                    pMsg->msgLen,
                                    pMsg->msgBufOffset,
                                    peerIp,
                                    peerPort);

  /* Save the message for retransmission */
  if(rc == NW_GTPV1U_OK && pTrxn) {
    pTrxn->pMsg         = pMsg;
    pTrxn->peerIp       = peerIp;
    pTrxn->peerPort     = peerPort;

    rc = nwGtpv1uTrxnStartPeerRspTimer(pTrxn, nwGtpv1uTrxnPeerRspTimeout);
    NW_ASSERT(rc == NW_GTPV1U_OK);
  }

  return rc;
}
#endif


#ifdef __cplusplus
}
#endif

/*--------------------------------------------------------------------------*
 *                          E N D   O F   F I L E                           *
 *--------------------------------------------------------------------------*/
