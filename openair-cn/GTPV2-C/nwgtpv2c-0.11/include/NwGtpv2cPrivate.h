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

#ifndef __NW_GTPV2C_PRIVATE_H__
#define __NW_GTPV2C_PRIVATE_H__

#include <sys/time.h>

#include "tree.h"
#include "queue.h"

#include "NwTypes.h"
#include "NwError.h"
#include "NwGtpv2c.h"
#include "NwGtpv2cIe.h"
#include "NwGtpv2cMsg.h"
#include "NwGtpv2cMsgIeParseInfo.h"
#include "NwGtpv2cTunnel.h"

/**
 * @file NwGtpv2cPrivate.h
 * @brief This header file contains nw-gtpv2c private definitions not to be
 * exposed to user application.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define NW_GTPV2C_MALLOC(_stack, _size, _mem, _type)                    \
  do {                                                                  \
    if(((NwGtpv2cStackT*)(_stack))->memMgr.memAlloc && ((NwGtpv2cStackT*)(_stack))->memMgr.memFree )\
    {                                                                   \
      _mem = (_type) ((NwGtpv2cStackT*) (_stack))->memMgr.memAlloc(((NwGtpv2cStackT*) (_stack))->memMgr.hMemMgr, _size, __FILE__, __LINE__);\
    }                                                                   \
    else                                                                \
    {                                                                   \
      _mem = (_type) malloc (_size);                                    \
    }                                                                   \
  } while (0)

#define NW_GTPV2C_FREE(_stack, _mem)                                    \
  do {                                                                  \
    if(((NwGtpv2cStackT*)(_stack))->memMgr.memAlloc && ((NwGtpv2cStackT*)(_stack))->memMgr.memFree )\
    {                                                                   \
      ((NwGtpv2cStackT*)(_stack))->memMgr.memFree(((NwGtpv2cStackT*) (_stack))->memMgr.hMemMgr, _mem, __FILE__, __LINE__);\
    }                                                                   \
    else                                                                \
    {                                                                   \
      free ((void*)_mem);                                               \
    }                                                                   \
  } while (0)

/*--------------------------------------------------------------------------*
 *  G T P V 2 C   S T A C K   O B J E C T   T Y P E    D E F I N I T I O N  *
 *--------------------------------------------------------------------------*/

/**
 * gtpv2c stack class definition
 */

typedef struct NwGtpv2cStack {
  uint32_t                        id;
  NwGtpv2cUlpEntityT            ulp;
  NwGtpv2cUdpEntityT            udp;
  NwGtpv2cMemMgrEntityT         memMgr;
  NwGtpv2cTimerMgrEntityT       tmrMgr;
  NwGtpv2cLogMgrEntityT         logMgr;

  uint32_t                        seqNum;
  uint32_t                        logLevel;
  uint32_t                        restartCounter;

  NwGtpv2cMsgIeParseInfoT       *pGtpv2cMsgIeParseInfo[NW_GTP_MSG_END];
  struct NwGtpv2cTimeoutInfo    *activeTimerInfo;

  RB_HEAD( NwGtpv2cTunnelMap, NwGtpv2cTunnel                ) tunnelMap;
  RB_HEAD( NwGtpv2cOutstandingTxSeqNumTrxnMap, NwGtpv2cTrxn ) outstandingTxSeqNumMap;
  RB_HEAD( NwGtpv2cOutstandingRxSeqNumTrxnMap, NwGtpv2cTrxn ) outstandingRxSeqNumMap;
  RB_HEAD( NwGtpv2cActiveTimerList, NwGtpv2cTimeoutInfo     ) activeTimerList;
  NwHandleT                     hTmrMinHeap;
} NwGtpv2cStackT;


/*--------------------------------------------------------------------------*
 * Timeout Info Type Definition
 *--------------------------------------------------------------------------*/

/**
 * gtpv2c timeout info
 */

typedef struct NwGtpv2cTimeoutInfo {
  NwGtpv2cStackHandleT          hStack;
  struct timeval                tvTimeout;
  uint32_t                        tmrType;
  void*                         timeoutArg;
  NwRcT                         (*timeoutCallbackFunc)(void*);
  NwGtpv2cTimerHandleT          hTimer;
  RB_ENTRY (NwGtpv2cTimeoutInfo)       activeTimerListRbtNode;            /**< RB Tree Data Structure Node        */
  uint32_t                        timerMinHeapIndex;
  struct NwGtpv2cTimeoutInfo *next;
} NwGtpv2cTimeoutInfoT;


/*---------------------------------------------------------------------------
 * GTPv2c Message Container Definition
 *--------------------------------------------------------------------------*/

#define NW_GTPV2C_MAX_MSG_LEN                                    (1024)  /**< Maximum supported gtpv2c packet length including header */

/**
 * NwGtpv2cMsgT holds gtpv2c messages to/from the peer.
 */
typedef struct NwGtpv2cMsgS {
  uint8_t                         version;
  uint8_t                         teidPresent;
  uint8_t                         msgType;
  uint16_t                        msgLen;
  uint32_t                        teid;
  uint32_t                        seqNum;
  uint8_t*                        pMsgStart;

#define NW_GTPV2C_MAX_GROUPED_IE_DEPTH                                  (2)
  struct {
    NwGtpv2cIeTlvT *pIe[NW_GTPV2C_MAX_GROUPED_IE_DEPTH];
    uint8_t         top;
  } groupedIeEncodeStack;

  NwBoolT                       isIeValid[NW_GTPV2C_IE_TYPE_MAXIMUM][NW_GTPV2C_IE_INSTANCE_MAXIMUM];
  uint8_t                         *pIe[NW_GTPV2C_IE_TYPE_MAXIMUM][NW_GTPV2C_IE_INSTANCE_MAXIMUM];
  uint8_t                         msgBuf[NW_GTPV2C_MAX_MSG_LEN];
  NwGtpv2cStackHandleT          hStack;
  struct NwGtpv2cMsgS*          next;
} NwGtpv2cMsgT;

/**
 * Transaction structure
 */

typedef struct NwGtpv2cTrxn {
  uint32_t                        seqNum;
  uint32_t                        peerIp;
  uint32_t                        peerPort;
  uint8_t                         t3Timer;
  uint8_t                         maxRetries;
  NwGtpv2cMsgT*                 pMsg;
  NwGtpv2cStackT*               pStack;
  NwGtpv2cTimerHandleT          hRspTmr;                                /**< Handle to reponse timer            */
  NwGtpv2cTunnelHandleT         hTunnel;                                /**< Handle to local tunnel context     */
  NwGtpv2cUlpTrxnHandleT        hUlpTrxn;                               /**< Handle to ULP tunnel context       */
  RB_ENTRY (NwGtpv2cTrxn)       outstandingTxSeqNumMapRbtNode;          /**< RB Tree Data Structure Node        */
  RB_ENTRY (NwGtpv2cTrxn)       outstandingRxSeqNumMapRbtNode;          /**< RB Tree Data Structure Node        */
  struct NwGtpv2cTrxn*          next;
} NwGtpv2cTrxnT;

/**
 *  GTPv2c Path Context
 */

typedef struct NwGtpv2cPathS {
  uint32_t                        hUlpPath;                               /**< Handle to ULP path contect         */
  uint32_t                        ipv4Address;
  uint32_t                        restartCounter;
  uint16_t                        t3ResponseTimout;
  uint16_t                        n3RequestCount;
  NwGtpv2cTimerHandleT          hKeepAliveTmr;                          /**< Handle to path keep alive echo timer */
  RB_ENTRY (NwGtpv2cPathS)      pathMapRbtNode;
} NwGtpv2cPathT;


RB_PROTOTYPE(NwGtpv2cTunnelMap, NwGtpv2cTunnel, tunnelMapRbtNode, nwGtpv2cCompareTunnel)
RB_PROTOTYPE(NwGtpv2cOutstandingTxSeqNumTrxnMap, NwGtpv2cTrxn, outstandingTxSeqNumMapRbtNode, nwGtpv2cCompareSeqNum)
RB_PROTOTYPE(NwGtpv2cOutstandingRxSeqNumTrxnMap, NwGtpv2cTrxn, outstandingRxSeqNumMapRbtNode, nwGtpv2cCompareSeqNum)
RB_PROTOTYPE(NwGtpv2cActiveTimerList, NwGtpv2cTimeoutInfo, activeTimerListRbtNode, nwGtpv2cCompareOutstandingTxRexmitTime)

/**
 * Start Timer with ULP Timer Manager
 */

NwRcT
nwGtpv2cStartTimer(NwGtpv2cStackT* thiz,
                   uint32_t timeoutSec,
                   uint32_t timeoutUsec,
                   uint32_t tmrType,
                   NwRcT (*timeoutCallbackFunc)(void*),
                   void*  timeoutCallbackArg,
                   NwGtpv2cTimerHandleT *phTimer);


/**
 * Stop Timer with ULP Timer Manager
 */

NwRcT
nwGtpv2cStopTimer(NwGtpv2cStackT* thiz,
                  NwGtpv2cTimerHandleT hTimer);

#ifdef __cplusplus
}
#endif

#endif  /* __NW_GTPV2C_PRIVATE_H__ */
/*--------------------------------------------------------------------------*
 *                      E N D     O F    F I L E                            *
 *--------------------------------------------------------------------------*/

