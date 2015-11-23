/*----------------------------------------------------------------------------*
 *                                                                            *
 *                             n w - g t p v 2 u                              *
 *                             n w - g t p v 2 u                              *
 *    G P R S    T u n n e l i n g    P r o t o c o l   v 2 u   S t a c k     *
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

#ifndef __NW_GTPV1U_PRIVATE_H__
#define __NW_GTPV1U_PRIVATE_H__

#include "tree.h"
#include "queue.h"

#include "NwTypes.h"
#include "NwGtpv1uError.h"
#include "NwGtpv1u.h"
#include "NwGtpv1uMsg.h"


/**
 * @file NwGtpv1uPrivate.h
 * @brief This header file contains nw-gtpv1u private definitions not to be
 * exposed to user application.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define NW_GTPV1U_MALLOC(_stack, _size, _mem, _type)                    \
  do {                                                                  \
    if(((NwGtpv1uStackT*)(_stack))->memMgr.memAlloc && ((NwGtpv1uStackT*)(_stack))->memMgr.memFree )\
    {                                                                   \
      _mem = (_type) ((NwGtpv1uStackT*) (_stack))->memMgr.memAlloc(((NwGtpv1uStackT*) (_stack))->memMgr.hMemMgr, _size, __FILE__, __LINE__);\
    }                                                                   \
    else                                                                \
    {                                                                   \
      _mem = (_type) malloc (_size);                                    \
    }                                                                   \
  } while (0)

/*--------------------------------------------------------------------------*
 *    G T P U   S T A C K   O B J E C T   T Y P E    D E F I N I T I O N    *
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*
 * GRPS Tunneling Protocol Stack Class Definition
 *--------------------------------------------------------------------------*/

/**
 * gtpv1u stack class definition
 */

typedef struct NwGtpv1uStack {
  uint32_t                        stackType;/**< GTPU_STACK_ENB or GTPU_STACK_SGW   */
  NwPtrT                        id;
  uint32_t                        seq;
  NwGtpv1uUlpEntityT            ulp;
  NwGtpv1uUdpEntityT            udp;
  NwGtpv1uMemMgrEntityT         memMgr;
  NwGtpv1uTimerMgrEntityT       tmrMgr;
  NwGtpv1uLogMgrEntityT         logMgr;
  uint32_t                        logLevel;
  RB_HEAD( NwGtpv1uOutstandingTxSeqNumTrxnMap, NwGtpv1uTrxn) outstandingTxSeqNumMap;
  RB_HEAD( NwGtpv1uOutstandingRxSeqNumTrxnMap, NwGtpv1uTrxn) outstandingRxSeqNumMap;
  RB_HEAD(NwGtpv1uTunnelEndPointTMap, NwGtpv1uTunnelEndPoint) sessionMap;
  RB_HEAD(NwGtpv1uTunnelEndPointIdentifierMap, NwGtpv1uTunnelEndPoint) teidMap;
} NwGtpv1uStackT;

/**
 * GTP Tunnel End Point class definition
 */

typedef struct NwGtpv1uTunnelEndPoint {
  uint32_t                        teid;                                   /**< Gtpu Tunnel End Point Identifier   */
  uint32_t                        peerAddr;                               /**< Peer IP address for the session    */
  NwGtpv1uStackT*               pStack;                                 /**< Pointer to the parent stack        */
  NwGtpv1uUlpSessionHandleT     hUlpSession;                            /**< ULP session handle for the session */
  RB_ENTRY (NwGtpv1uTunnelEndPoint)    sessionMapRbtNode;               /**< RB Tree Data Structure Node        */
  struct NwGtpv1uTunnelEndPoint *next;
} NwGtpv1uTunnelEndPointT;


/*--------------------------------------------------------------------------*
 * Timeout Info Type Definition
 *--------------------------------------------------------------------------*/

/**
 * gtpv1u timeout info
 */

typedef struct NwGtpv1uTimeoutInfo {
  NwGtpv1uStackHandleT hStack;
  void* timeoutArg;
  NwGtpv1uRcT (*timeoutCallbackFunc)(void*);
} NwGtpv1uTimeoutInfoT;

/**
 * Start a transaction response timer
 *
 * @param[in] thiz Pointer to stack instance
 * @param[in] timeoutArg Arg to timeout function.
 * @param[out] phTmr Pointer to timer handle.
 * @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpStartTrxnPeerRspTimer(NwGtpv1uStackT* thiz, NwGtpv1uTimeoutInfoT* timeoutInfo, NwGtpv1uTimerHandleT* phTmr);

/**
 * Stop a transaction response timer
 *
 * @param[in] thiz Pointer to stack instance
 * @param[out] phTmr Pointer to timer handle.
 * @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpStopTrxnPeerRspTimer(NwGtpv1uStackT* thiz, NwGtpv1uTimerHandleT* phTmr);


#define NW_GTPV1U_MAX_MSG_LEN                                    (4096)  /**< Maximum supported gtpv1u packet length including header */

/**
 * NwGtpv1uMsgT holds gtpv1u messages to/from the peer.
 */
typedef struct NwGtpv1uMsg {
  uint8_t         version;
  uint8_t         protocolType;
  uint8_t         extHdrFlag;
  uint8_t         seqNumFlag;
  uint16_t        npduNumFlag;
  uint32_t        msgType;
  uint16_t        msgLen;
  uint32_t        teid;
  uint16_t        seqNum;
  uint8_t         npduNum;
  uint8_t         nextExtHdrType;
  uint8_t*        msgBuf;
  uint32_t        msgBufLen;
  uint32_t        msgBufOffset;
  struct NwGtpv1uMsg* next;
} NwGtpv1uMsgT;


/*--------------------------------------------------------------------------*
 * R6/R4 Transaction Context Type Definition
 *--------------------------------------------------------------------------*/

/**
 * Transaction structure
 */

typedef struct NwGtpv1uTrxn {
  uint32_t                        seqNum;
  uint32_t                        peerIp;
  uint32_t                        peerPort;
  uint8_t                         maxRetries;
  uint8_t                         t3Timer;
  NwGtpv1uTimerHandleT          hRspTmr;
  NwGtpv1uTimeoutInfoT          peerRspTimeoutInfo;
  NwGtpv1uStackT*               pStack;
  NwGtpv1uTunnelEndPointT*      pSession;
  uint32_t                        hUlpTrxn;
  NwGtpv1uMsgT*                 pMsg;
  RB_ENTRY (NwGtpv1uTrxn)       outstandingTxSeqNumMapRbtNode;            /**< RB Tree Data Structure Node        */
  RB_ENTRY (NwGtpv1uTrxn)       outstandingRxSeqNumMapRbtNode;            /**< RB Tree Data Structure Node        */
  struct NwGtpv1uTrxn*          next;
} NwGtpv1uTrxnT;


/**
 * GTPv2 message header structure
 */

#pragma pack(1)

typedef struct NwGtpv1uMsgHeader {
  uint8_t PN:1;
  uint8_t S:1;
  uint8_t E:1;
  uint8_t spare:1;
  uint8_t PT:1;
  uint8_t version:3;
  uint8_t msgType;
  uint16_t msgLength;
  uint32_t teid;
} NwGtpv1uMsgHeaderT;

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif  /* __NW_GTPV1U_PRIVATE_H__ */
/*--------------------------------------------------------------------------*
 *                      E N D     O F    F I L E                            *
 *--------------------------------------------------------------------------*/

