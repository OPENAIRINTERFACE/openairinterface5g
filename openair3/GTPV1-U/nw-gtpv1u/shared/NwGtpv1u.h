/*----------------------------------------------------------------------------*
 *                                                                            *
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

#ifndef __NW_GTPV1U_H__
#define __NW_GTPV1U_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "NwTypes.h"
#include "NwGtpv1uError.h"


/**
 * @file NwGtpv1u.h
 * @author Amit Chawre
 * @brief
 *
 * This header file contains all required definitions and functions
 * prototypes for using nw-gtpv1u library.
 *
 **/

#define NW_GTPU_VERSION                                         (0x01)
#define NW_GTP_PROTOCOL_TYPE_GTP                                (0x01)
#define NW_GTP_PROTOCOL_TYPE_GTP_PRIME                          (0x00)

/*--------------------------------------------------------------------------*
 *                   S H A R E D     A P I    M A C R O S                   *
 *--------------------------------------------------------------------------*/

#define NW_GTP_ECHO_REQ                                         (1)
#define NW_GTP_ECHO_RSP                                         (2)
#define NW_GTP_ERROR_INDICATION                                 (26)
#define NW_GTP_SUPPORTED_EXTENSION_HEADER_INDICATION            (31)
#define NW_GTP_END_MARKER                                       (254)
#define NW_GTP_GPDU                                             (255)

/*---------------------------------------------------------------------------
 * Opaque Gtpv2 Stack Handles
 *--------------------------------------------------------------------------*/

typedef NwPtrT  NwGtpv1uStackHandleT;                     /**< Gtpv2 Stack Handle               */
typedef NwPtrT  NwGtpv1uUlpHandleT;                       /**< Gtpv2 Stack Ulp Entity Handle    */
typedef NwPtrT  NwGtpv1uUdpHandleT;                       /**< Gtpv2 Stack Udp Entity Handle    */
typedef NwPtrT  NwGtpv1uMemMgrHandleT;                    /**< Gtpv2 Stack Mem Manager Handle   */
typedef NwPtrT  NwGtpv1uTimerMgrHandleT;                  /**< Gtpv2 Stack Timer Manager Handle */
typedef NwPtrT  NwGtpv1uLogMgrHandleT;                    /**< Gtpv2 Stack Log Mnagaer Handle   */
typedef NwPtrT  NwGtpv1uTimerHandleT;                     /**< Gtpv2 Stack Timer Handle         */
typedef NwPtrT  NwGtpv1uMsgHandleT;                       /**< Gtpv2 Msg Handle                 */

typedef struct NwGtpv1uStackConfig {
  uint16_t udpSrcPort;
} NwGtpv1uStackConfigT;

/*--------------------------------------------------------------------------*
 *            S T A C K        A P I      D E F I N I T I O N S             *
 *--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * Gtpv2 Stack ULP API type definitions
 *--------------------------------------------------------------------------*/

/**
 * APIs types between ULP and Stack
 */

typedef enum {
  /* APIs from ULP to stack */

  NW_GTPV1U_ULP_API_CREATE_TUNNEL_ENDPOINT = 0x00000000,        /**< Create a local teid context on stack       */
  NW_GTPV1U_ULP_API_DESTROY_TUNNEL_ENDPOINT,                    /**< Delete a local teid context on stack       */
  NW_GTPV1U_ULP_API_INITIAL_REQ,                                /**< Send a Initial Request over a session      */
  NW_GTPV1U_ULP_API_TRIGGERED_REQ,                              /**< Send a Initial Request over a session      */
  NW_GTPV1U_ULP_API_TRIGGERED_RSP,                              /**< Send a Trigger Response over a session     */
  NW_GTPV1U_ULP_API_SEND_TPDU,                                  /**< Send a T-PDU message over teid context     */

  /* APIs from stack to ULP */

  NW_GTPV1U_ULP_API_RECV_TPDU,                                  /**< Receive a gtpv1u T-PDU from stack          */
  NW_GTPV1U_ULP_API_RECV_MSG,                                   /**< Receive a gtpv1u message from stack        */
  NW_GTPV1U_ULP_API_RSP_FAILURE,                                /**< Rsp failure for gtpv2 message from stack   */

  /* Do not add below this */

  NW_GTPV1U_ULP_API_END            = 0xFFFFFFFF,
} NwGtpv1uUlpApiTypeT;

/*---------------------------------------------------------------------------
 * Gtpv2 Stack API information elements definitions
 *--------------------------------------------------------------------------*/

typedef NwPtrT  NwGtpv1uStackSessionHandleT;/**< Gtpv2 Stack session Handle */
typedef uint8_t   NwGtpv1uMsgTypeT;           /**< Gtpv2 Msg Type     */
typedef NwPtrT  NwGtpv1uTrxnHandleT;        /**< Gtpv2 Transaction Handle */
typedef NwPtrT  NwGtpv1uUlpTrxnHandleT;     /**< Gtpv2 Ulp Transaction Handle */
typedef NwPtrT  NwGtpv1uUlpSessionHandleT;  /**< Gtpv2 Ulp session Handle */

/**
 * API information elements between ULP and Stack for
 * creating a session.
 */

typedef struct {
  NW_IN    uint32_t                       teid;
  NW_IN    NwGtpv1uUlpSessionHandleT    hUlpSession;
  NW_OUT   NwGtpv1uStackSessionHandleT  hStackSession;
} NwGtpv1uCreateTunnelEndPointT;

/**
 * API information elements between ULP and Stack for
 * destroying a session.
 */

typedef struct {
  NW_IN   NwGtpv1uStackSessionHandleT   hStackSessionHandle;
} NwGtpv1uDestroyTunnelEndPointT;

/**
 * API information elements between ULP and Stack for
 * sending a Gtpv1u initial message.
 */

typedef struct {
  NW_IN    NwGtpv1uUlpTrxnHandleT        hUlpTrxn;
  NW_IN    uint32_t                        peerIp;
  NW_IN    uint32_t                        peerPort;
  NW_IN    uint8_t                         flags;
  NW_IN    uint32_t                        teid;
} NwGtpv1uInitialReqInfoT;

/**
 * API information elements between ULP and Stack for
 * sending a Gtpv1u triggered response message.
 */

typedef struct {
  NW_IN    NwGtpv1uUlpTrxnHandleT        hUlpTrxn;
  NW_IN    uint32_t                        peerIp;
  NW_IN    uint32_t                        peerPort;
  NW_IN    uint8_t                         flags;
  NW_IN    uint32_t                        teid;
  NW_IN    uint32_t                        seqNum;
  NW_IN    NwGtpv1uMsgHandleT            hMsg;
} NwGtpv1uTriggeredRspInfoT;

/**
 * API information elements between ULP and Stack for
 * sending a Gtpv1u triggered request message.
 */

typedef struct {
  NW_IN    NwGtpv1uUlpTrxnHandleT        hUlpTrxn;
  NW_IN    uint32_t                        peerIp;
  NW_IN    uint32_t                        peerPort;
  NW_IN    uint8_t                         flags;
  NW_IN    uint32_t                        teid;
  NW_IN    uint32_t                        seqNum;
  NW_IN    NwGtpv1uMsgHandleT            hMsg;
} NwGtpv1uTriggeredReqInfoT;


/**
 * API information elements between ULP and Stack for
 * sending a Gtpv2 message over a session.
 */

typedef struct {
  NW_IN    uint32_t                       teid;
  NW_IN    uint32_t                       ipAddr;
  NW_IN    uint8_t                        flags;
  NW_IN    NwGtpv1uMsgHandleT           hMsg;
} NwGtpv1uSendtoInfoT;


/**
 * API information elements between ULP and Stack for
 * sending a Gtpv2 message over a session.
 */

typedef struct {
  NW_OUT   NwGtpv1uStackSessionHandleT  hStackSessionHandle;
  NW_INOUT NwGtpv1uTrxnHandleT          hTrxn;
  NW_IN    NwGtpv1uUlpTrxnHandleT       hUlpTrxn;
  NW_IN    NwGtpv1uMsgTypeT            msgType;
  NW_IN    uint8_t                        flags;
  NW_IN    NwGtpv1uMsgHandleT           hMsg;
} NwGtpv1uSendMsgInfoT;

/**
 * API information elements between ULP and Stack for
 * receiving a Gtpv2 message over a session from stack.
 */

typedef struct {
  NW_IN    NwGtpv1uUlpSessionHandleT    hUlpSession;
  NW_IN    NwGtpv1uUlpTrxnHandleT       hUlpTrxn;
  NW_IN    uint32_t                       teid;
  NW_IN    uint32_t                       peerIp;
  NW_IN    uint32_t                       peerPort;
  NW_IN    uint32_t                       msgType;      /**< Message type                       */
  NW_IN    NwGtpv1uMsgHandleT           hMsg;         /**< Gtp Message handle                 */
} NwGtpv1uRecvMsgInfoT;

/**
 * API information elements between ULP and Stack for
 * receiving a Gtpv2 message over a session from stack.
 */

typedef struct {
  NW_IN    NwGtpv1uUlpSessionHandleT    hUlpSession;
  NW_IN    NwGtpv1uTrxnHandleT          hTrxn;
} NwGtpv1uNackInfoT;

/*---------------------------------------------------------------------------
 * Gtpv2 Stack API structure definition
 *--------------------------------------------------------------------------*/

/**
 * API structure between ULP and Stack
 */

typedef struct {
  NwGtpv1uUlpApiTypeT                   apiType;
  NwGtpv1uMsgHandleT                    hMsg;
  union {
    NwGtpv1uCreateTunnelEndPointT       createTunnelEndPointInfo;
    NwGtpv1uDestroyTunnelEndPointT      destroyTunnelEndPointInfo;
    NwGtpv1uInitialReqInfoT             initialReqInfo;
    NwGtpv1uTriggeredRspInfoT           triggeredRspInfo;
    NwGtpv1uTriggeredReqInfoT           triggeredReqInfo;
    NwGtpv1uSendtoInfoT                 sendtoInfo;
    NwGtpv1uSendMsgInfoT                sendMsgInfo;
    NwGtpv1uRecvMsgInfoT                recvMsgInfo;
    NwGtpv1uNackInfoT                   nackMsgInfo;
  } apiInfo;
} NwGtpv1uUlpApiT;


/*--------------------------------------------------------------------------*
 *           S T A C K    E N T I T I E S    D E F I N I T I O N S          *
 *--------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
 * ULP Entity Definitions
 *--------------------------------------------------------------------------*/

/**
 * Gtpv2 ULP entity definition
 */

typedef struct {
  NwGtpv1uUlpHandleT        hUlp;
  NwGtpv1uRcT (*ulpReqCallback) ( NW_IN        NwGtpv1uUlpHandleT hUlp,
                                  NW_IN        NwGtpv1uUlpApiT *pUlpApi);
} NwGtpv1uUlpEntityT;


/*---------------------------------------------------------------------------
 * UDP Entity Definitions
 *--------------------------------------------------------------------------*/

/**
 * Gtpv2 UDP entity definition
 */

typedef struct {
  NwGtpv1uUdpHandleT        hUdp;
  NwGtpv1uRcT (*udpDataReqCallback) ( NW_IN     NwGtpv1uUdpHandleT udpHandle,
                                      NW_IN     uint8_t* dataBuf,
                                      NW_IN     uint32_t dataSize,
                                      NW_IN     uint32_t dataOffset,
                                      NW_IN     uint32_t peerIP,
                                      NW_IN     uint16_t peerPort);
} NwGtpv1uUdpEntityT;

/**
 * Gtpv1u Memory Manager entity definition
 */

typedef struct {
  NwGtpv1uMemMgrHandleT         hMemMgr;
  void* (*memAlloc)( NW_IN      NwGtpv1uMemMgrHandleT hMemMgr,
                     NW_IN      uint32_t memSize,
                     NW_IN      NwCharT* fileName,
                     NW_IN      uint32_t lineNumber);

  void (*memFree) ( NW_IN       NwGtpv1uMemMgrHandleT hMemMgr,
                    NW_IN       void* hMem,
                    NW_IN       NwCharT* fileName,
                    NW_IN       uint32_t lineNumber);
} NwGtpv1uMemMgrEntityT;


/*---------------------------------------------------------------------------
 * Timer Entity Definitions
 *--------------------------------------------------------------------------*/

#define NW_GTPV1U_TMR_TYPE_ONE_SHOT                                  (0)
#define NW_GTPV1U_TMR_TYPE_REPETITIVE                                (1)

/**
 * Gtpv2 Timer Manager entity definition
 */

typedef struct {
  NwGtpv1uTimerMgrHandleT        tmrMgrHandle;
  NwGtpv1uRcT (*tmrStartCallback)( NW_IN       NwGtpv1uTimerMgrHandleT tmrMgrHandle,
                                   NW_IN       uint32_t timeoutSecs,
                                   NW_IN       uint32_t timeoutUsec,
                                   NW_IN       uint32_t tmrType,
                                   NW_IN       void* tmrArg,
                                   NW_OUT      NwGtpv1uTimerHandleT* tmrHandle);

  NwGtpv1uRcT (*tmrStopCallback) ( NW_IN       NwGtpv1uTimerMgrHandleT tmrMgrHandle,
                                   NW_IN       NwGtpv1uTimerHandleT tmrHandle);
} NwGtpv1uTimerMgrEntityT;


/*---------------------------------------------------------------------------
 * Log Entity Definitions
 *--------------------------------------------------------------------------*/

/**
 * Gtpv2 Log manager entity definition
 */

typedef struct {
  NwGtpv1uLogMgrHandleT          logMgrHandle;
  NwGtpv1uRcT (*logReqCallback) (NW_IN      NwGtpv1uLogMgrHandleT logMgrHandle,
                                 NW_IN      uint32_t logLevel,
                                 NW_IN      NwCharT* file,
                                 NW_IN      uint32_t line,
                                 NW_IN      NwCharT* logStr);
} NwGtpv1uLogMgrEntityT;


/*--------------------------------------------------------------------------*
 *                     P U B L I C   F U N C T I O N S                      *
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 *  Constructor
 *--------------------------------------------------------------------------*/

/**
 Initialize the nw-gtpv1u stack.

 @param[in,out] phGtpv1uStackHandle : Pointer to stack handle
 @param[in]     stackType : GTPU_STACK_ENB or GTPU_STACK_SGW
 */

NwGtpv1uRcT
nwGtpv1uInitialize( NW_INOUT NwGtpv1uStackHandleT* phGtpv1uStackHandle, uint32_t stackType);

/*---------------------------------------------------------------------------
 * Destructor
 *--------------------------------------------------------------------------*/

/**
 Destroy the nw-gtpv1u stack.

 @param[in] hGtpv1uStackHandle : Stack handle
 */

NwGtpv1uRcT
nwGtpv1uFinalize( NW_IN  NwGtpv1uStackHandleT hGtpv1uStackHandle);

/*---------------------------------------------------------------------------
 * Configuration Get/Set Operations
 *--------------------------------------------------------------------------*/

/**
 Set Configuration for the nw-gtpv1u stack.

 @param[in,out] phGtpv1uStackHandle : Pointer to stack handle
 */

NwGtpv1uRcT
NwGtpv1uConfigSet( NW_IN NwGtpv1uStackHandleT* phGtpv1uStackHandle, NW_IN NwGtpv1uStackConfigT* pConfig);

/**
 Get Configuration for the nw-gtpv1u stack.

 @param[in,out] phGtpv1uStackHandle : Pointer to stack handle
 */

NwGtpv1uRcT
NwGtpv1uConfigGet( NW_IN NwGtpv1uStackHandleT* phGtpv1uStackHandle, NW_OUT NwGtpv1uStackConfigT* pConfig);

/**
 Set ULP entity for the stack.

 @param[in] hGtpv1uStackHandle : Stack handle
 @param[in] pUlpEntity : Pointer to ULP entity.
 @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpv1uSetUlpEntity( NW_IN NwGtpv1uStackHandleT hGtpv1uStackHandle,
                      NW_IN NwGtpv1uUlpEntityT* pUlpEntity);

/**
 Set UDP entity for the stack.

 @param[in] hGtpv1uStackHandle : Stack handle
 @param[in] pUdpEntity : Pointer to UDP entity.
 @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpv1uSetUdpEntity( NW_IN NwGtpv1uStackHandleT hGtpv1uStackHandle,
                      NW_IN NwGtpv1uUdpEntityT* pUdpEntity);

/**
 Set MemMgr entity for the stack.

 @param[in] hGtpv1uStackHandle : Stack handle
 @param[in] pMemMgr : Pointer to Memory Manager.
 @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpv1uSetMemMgrEntity( NW_IN NwGtpv1uStackHandleT hGtpv1uStackHandle,
                         NW_IN NwGtpv1uMemMgrEntityT* pMemMgr);


/**
 Set TmrMgr entity for the stack.

 @param[in] hGtpv1uStackHandle : Stack handle
 @param[in] pTmrMgr : Pointer to Timer Manager.
 @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpv1uSetTimerMgrEntity( NW_IN NwGtpv1uStackHandleT hGtpv1uStackHandle,
                           NW_IN NwGtpv1uTimerMgrEntityT* pTmrMgr);

/**
 Set LogMgr entity for the stack.

 @param[in] hGtpv1uStackHandle : Stack handle
 @param[in] pLogMgr : Pointer to Log Manager.
 @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpv1uSetLogMgrEntity( NW_IN NwGtpv1uStackHandleT hGtpv1uStackHandle,
                         NW_IN NwGtpv1uLogMgrEntityT* pLogMgr);

/**
 Set log level for the stack.

 @param[in] hGtpv1uStackHandle : Stack handle
 @param[in] logLevel : Log level.
 @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpv1uSetLogLevel( NW_IN NwGtpv1uStackHandleT hGtpuStackHandle,
                     NW_IN uint32_t logLevel);
/*---------------------------------------------------------------------------
 * Process Request from Udp Layer
 *--------------------------------------------------------------------------*/

/**
 Process Data Request from UDP entity.

 @param[in] hGtpv1uStackHandle : Stack handle
 @param[in] udpData : Pointer to received UDP data.
 @param[in] udpDataLen : Received data length.
 @param[in] dstPort : Received on port.
 @param[in] from : Received from peer information.
 @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpv1uProcessUdpReq( NW_IN NwGtpv1uStackHandleT hGtpv1uStackHandle,
                       NW_IN uint8_t* udpData,
                       NW_IN uint32_t udpDataLen,
                       NW_IN uint16_t peerPort,
                       NW_IN uint32_t peerIP);

/*---------------------------------------------------------------------------
 * Process Request from Upper Layer
 *--------------------------------------------------------------------------*/

/**
 Process Request from ULP entity.

 @param[in] hGtpv1uStackHandle : Stack handle
 @param[in] pLogMgr : Pointer to Ulp Req.
 @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpv1uProcessUlpReq( NW_IN NwGtpv1uStackHandleT hGtpv1uStackHandle,
                       NW_IN NwGtpv1uUlpApiT *ulpReq);


/*---------------------------------------------------------------------------
 * Process Timer timeout Request from Timer Manager
 *--------------------------------------------------------------------------*/

/**
 Process Timer timeout Request from Timer Manager

 @param[in] pLogMgr : Pointer timeout arguments.
 @return NW_GTPV1U_OK on success.
 */

NwGtpv1uRcT
nwGtpv1uProcessTimeout( NW_IN void* timeoutArg);

NwGtpv1uRcT
nwGtpv1uPeerRspTimeout(void* arg);

#ifdef __cplusplus
}
#endif

#endif  /* __NW_GTPV1U_H__ */

/*--------------------------------------------------------------------------*
 *                      E N D     O F    F I L E                            *
 *--------------------------------------------------------------------------*/

