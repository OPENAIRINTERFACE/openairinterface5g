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

#ifndef __NW_GTPV2C_H__
#define __NW_GTPV2C_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "NwTypes.h"
#include "NwError.h"

/** @mainpage

  @section intro Introduction

  nw-gtpv2c library is a free and open source control plane implementation of GPRS
  Tunneling protocol v2 also known as eGTPc as per 3GPP TS29274-930.
  The library is published under BSD three clause license.

  @section scope Scope

  The stack library also does basic tasks like packet/header validation,
  retransmission and message parsing.

  @section design Design Philosophy

  The stack is fully-asynchronous in design for compatibility with event loop
  mechanisms such as select, poll, etc. and can also be used for multi-threaded
  applications. It should compile on Linux, *BSD, Mac OS X, Solaris and Windows
  (cygwin).

  The stack is designed for high portability not only for the hardware and OS it will
  run on but also for the application software that uses it. The stack doesn't mandate
  conditions on the user application architecture or design. The stack relies on
  the user application for infrastructure utilities such as I/O, timers,
  logs and multithreading. This realized by using callback mechanisms and enables the
  stack library to seamlessly integrate without or very little changes to the existing
  application framework.

  The stack architecture builds upon following mentioned entities that are external to it.

  User Layer Protocol (ULP) Entity:
  This layer implements the intelligent logic for the application and sits on top of the
  stack.

  UDP Entity:
  This is the layer below the stack and is responsible for UDP I/O with the stack and network.
  It may or may not be housed in ULP.

  Timer Manager Entity:
  Timer Manager Entity provides the stack with infrastructure for timer CRUD operations.

  Log Manager Entity:
  Log Manager Entity provides the stack with callbacks for logging operations. It may
  or may not be housed in ULP.

  The application may implement all above entities as a single or multiple object.

  @section applications Applications and Usage

  Please refer sample applications under 'test-app' directory for usage examples.

 */

/**
 * @file NwGtpv2c.h
 * @author Amit Chawre
 * @brief
 *
 * This header file contains all required definitions and functions
 * prototypes for using nw-gtpv2c library.
 *
 **/

/*--------------------------------------------------------------------------*
 *            S T A C K    H A N D L E    D E F I N I T I O N S             *
 *--------------------------------------------------------------------------*/

typedef NwPtrT  NwGtpv2cStackHandleT;                           /**< Gtpv2c Stack Handle                */
typedef NwPtrT  NwGtpv2cUlpHandleT;                             /**< Gtpv2c Stack Ulp Entity Handle     */
typedef NwPtrT  NwGtpv2cUdpHandleT;                             /**< Gtpv2c Stack Udp Entity Handle     */
typedef NwPtrT  NwGtpv2cTimerMgrHandleT;                        /**< Gtpv2c Stack Timer Manager Handle  */
typedef NwPtrT  NwGtpv2cMemMgrHandleT;                          /**< Gtpv2c Stack Memory Manager Handle */
typedef NwPtrT  NwGtpv2cLogMgrHandleT;                          /**< Gtpv2c Stack Log Manager Handle    */
typedef NwPtrT  NwGtpv2cTimerHandleT;                           /**< Gtpv2c Stack Timer Handle          */
typedef NwPtrT  NwGtpv2cMsgHandleT;                             /**< Gtpv2c Msg Handle                  */
typedef NwPtrT  NwGtpv2cTrxnHandleT;                            /**< Gtpv2c Transaction Handle          */
typedef NwPtrT  NwGtpv2cTunnelHandleT;                          /**< Gtpv2c Ulp Tunnel Handle           */
typedef NwPtrT  NwGtpv2cUlpTrxnHandleT;                         /**< Gtpv2c Ulp Transaction Handle      */
typedef NwPtrT  NwGtpv2cUlpTunnelHandleT;                       /**< Gtpv2c Ulp Tunnel Handle           */

typedef uint8_t   NwGtpv2cMsgTypeT;                               /**< Gtpv2c Msg Type                    */

typedef struct NwGtpv2cStackConfigS {
  uint16_t                                __tbd;
} NwGtpv2cStackConfigT;

/*--------------------------------------------------------------------------*
 *            S T A C K        A P I      D E F I N I T I O N S             *
 *--------------------------------------------------------------------------*/

#define NW_GTPV2C_ULP_API_FLAG_NONE                                     (0x00 << 24)
#define NW_GTPV2C_ULP_API_FLAG_CREATE_LOCAL_TUNNEL                      (0x01 << 24)
#define NW_GTPV2C_ULP_API_FLAG_DELETE_LOCAL_TUNNEL                      (0x02 << 24)
#define NW_GTPV2C_ULP_API_FLAG_IS_COMMAND_MESSAGE                       (0x03 << 24)

/*---------------------------------------------------------------------------
 * Gtpv2c Stack ULP API type definitions
 *--------------------------------------------------------------------------*/

/**
 * APIs types between ULP and Stack
 */

typedef enum {
  /* APIs from ULP to stack */

  NW_GTPV2C_ULP_API_INITIAL_REQ  = 0x00000000,          /**< Send a initial message                     */
  NW_GTPV2C_ULP_API_TRIGGERED_REQ,                      /**< Send a triggered req message               */
  NW_GTPV2C_ULP_API_TRIGGERED_RSP,                      /**< Send a triggered rsp message               */

  /* APIs from stack to ULP */

  NW_GTPV2C_ULP_API_INITIAL_REQ_IND,                    /**< Receive a initial message from stack       */
  NW_GTPV2C_ULP_API_TRIGGERED_RSP_IND,                  /**< Recieve a triggered rsp message from stack */
  NW_GTPV2C_ULP_API_TRIGGERED_REQ_IND,                  /**< Recieve a triggered req message from stack */
  NW_GTPV2C_ULP_API_RSP_FAILURE_IND,                    /**< Rsp failure for gtpv2 message from stack   */

  /* Local tunnel management APIs from ULP to stack */

  NW_GTPV2C_ULP_CREATE_LOCAL_TUNNEL,                    /**< Create a local tunnel                      */
  NW_GTPV2C_ULP_DELETE_LOCAL_TUNNEL,                    /**< Delete a local tunnel                      */

  /* Do not add below this */
  NW_GTPV2C_ULP_API_END = 0xFFFFFFFF,

} NwGtpv2cUlpApiTypeT;

/**
 * Error information of incoming GTP messages
 */

typedef struct NwGtpv2cErrorS {
  NW_IN    uint8_t                        cause;
  NW_IN    uint8_t                        flags;
  struct {
    NW_IN    uint8_t                      type;
    NW_IN    uint8_t                      instance;
  }                                     offendingIe;
} NwGtpv2cErrorT;

/**
 * API information elements between ULP and Stack for
 * sending a Gtpv2c initial message.
 */

typedef struct {
  NW_INOUT NwGtpv2cTunnelHandleT        hTunnel;        /**< Tunnel handle over which the mesasge is to be sent.*/
  NW_IN    uint16_t                       t3Timer;
  NW_IN    uint16_t                       maxRetries;
  NW_IN    NwGtpv2cUlpTrxnHandleT       hUlpTrxn;       /**< Optional handle to be returned in rsp of this msg. */

  NW_IN    uint32_t                       peerIp;         /**< Required only in case when hTunnel == 0            */
  NW_IN    uint32_t                       teidLocal;      /**< Required only in case when hTunnel == 0            */
  NW_IN    NwGtpv2cUlpTunnelHandleT     hUlpTunnel;     /**< Required only in case when hTunnel == 0            */
} NwGtpv2cInitialReqInfoT;

/**
 * API information elements between ULP and Stack for
 * sending a Gtpv2c triggered request message.
 */

typedef struct {
  NW_IN    NwGtpv2cTunnelHandleT        hTunnel;        /**< Tunnel handle over which the mesasge is to be sent */
  NW_IN    NwGtpv2cTrxnHandleT          hTrxn;          /**< Request Trxn handle which to which triggered req is being sent */
  NW_IN    uint16_t                       t3Timer;
  NW_IN    uint16_t                       maxRetries;
  NW_IN    NwGtpv2cUlpTrxnHandleT       hUlpTrxn;       /**< Optional handle to be returned in rsp of this msg. */
  NW_IN    uint32_t                       peerIp;         /**< Required only in case when hTunnel == 0            */
  NW_IN    uint32_t                       teidLocal;      /**< Required only in case when hTunnel == 0            */
  NW_IN    NwGtpv2cUlpTunnelHandleT     hUlpTunnel;     /**< Required only in case when hTunnel == 0            */

} NwGtpv2cTriggeredReqInfoT;

/**
 * API information elements between ULP and Stack for
 * sending a Gtpv2c triggered response message.
 */

typedef struct {
  NW_IN    NwGtpv2cTrxnHandleT          hTrxn;          /**< Request Trxn handle which to which triggered rsp is being sent */
  NW_IN    uint32_t                       teidLocal;      /**< Required only if NW_GTPV2C_ULP_API_FLAG_CREATE_LOCAL_TUNNEL is set to flags. */
  NW_IN    NwGtpv2cUlpTunnelHandleT     hUlpTunnel;     /**< Required only if NW_GTPV2C_ULP_API_FLAG_CREATE_LOCAL_TUNNEL is set to flags. */

  NW_OUT   NwGtpv2cTunnelHandleT        hTunnel;        /**< Returned only in case flags is set to
                                                             NW_GTPV2C_ULP_API_FLAG_CREATE_LOCAL_TUNNEL */
} NwGtpv2cTriggeredRspInfoT;

/**
 * API information elements between ULP and Stack for
 * sending a Gtpv2c initial message.
 */

typedef struct {
  NW_IN    NwGtpv2cErrorT               error;
  NW_IN    NwGtpv2cTrxnHandleT          hTrxn;
  NW_IN    NwGtpv2cUlpTrxnHandleT       hUlpTrxn;
  NW_IN    NwGtpv2cMsgTypeT             msgType;
  NW_IN    uint32_t                       peerIp;
  NW_IN    uint32_t                       peerPort;
  NW_IN    NwGtpv2cUlpTunnelHandleT     hUlpTunnel;
  NW_INOUT NwGtpv2cTunnelHandleT        hTunnel;
} NwGtpv2cInitialReqIndInfoT;

/**
 * API information elements between ULP and Stack for
 * sending a Gtpv2c triggered request message.
 */

typedef struct {
  NW_IN    NwGtpv2cErrorT               error;
  NW_IN    NwGtpv2cTrxnHandleT          hTrxn;
  NW_IN    NwGtpv2cUlpTrxnHandleT       hUlpTrxn;
  NW_IN    NwGtpv2cMsgTypeT             msgType;
  NW_IN    uint32_t                       seqNum;
  NW_IN    uint32_t                       teidLocal;
  NW_IN    uint32_t                       teidRemote;
  NW_IN    NwGtpv2cUlpTunnelHandleT     hUlpTunnel;
} NwGtpv2cTriggeredReqIndInfoT;

/**
 * API information elements between ULP and Stack for
 * sending a Gtpv2c triggered response message.
 */

typedef struct {
  NW_IN    NwGtpv2cErrorT               error;
  NW_IN    NwGtpv2cUlpTrxnHandleT       hUlpTrxn;
  NW_IN    NwGtpv2cUlpTunnelHandleT     hUlpTunnel;
  NW_IN    NwGtpv2cMsgTypeT             msgType;
} NwGtpv2cTriggeredRspIndInfoT;

/**
 * API information elements between ULP and Stack for
 * receving a path failure indication from stack.
 */

typedef struct {
  NW_IN    NwGtpv2cUlpTrxnHandleT       hUlpTrxn;
  NW_IN    NwGtpv2cUlpTunnelHandleT     hUlpTunnel;
} NwGtpv2cRspFailureIndInfoT;

/**
 * API information elements between ULP and Stack for
 * creating local tunnel.
 */

typedef struct {
  NW_OUT   NwGtpv2cTunnelHandleT         hTunnel;
  NW_IN    NwGtpv2cUlpTunnelHandleT      hUlpTunnel;
  NW_IN    uint32_t                        teidLocal;
  NW_IN    uint32_t                        peerIp;
} NwGtpv2cCreateLocalTunnelInfoT;


/**
 * API information elements between ULP and Stack for
 * deleting a local tunnel.
 */

typedef struct {
  NW_IN    NwGtpv2cTunnelHandleT         hTunnel;
} NwGtpv2cDeleteLocalTunnelInfoT;


/**
 * API container structure between ULP and Stack.
 */

typedef struct {
  NwGtpv2cUlpApiTypeT                   apiType;                /**< First bytes of this field is used as flag holder   */
  NwGtpv2cMsgHandleT                    hMsg;                   /**< Handle associated with this API                    */
  union {
    NwGtpv2cInitialReqInfoT             initialReqInfo;
    NwGtpv2cTriggeredRspInfoT           triggeredRspInfo;
    NwGtpv2cTriggeredReqInfoT           triggeredReqInfo;
    NwGtpv2cInitialReqIndInfoT          initialReqIndInfo;
    NwGtpv2cTriggeredRspIndInfoT        triggeredRspIndInfo;
    NwGtpv2cTriggeredReqIndInfoT        triggeredReqIndInfo;
    NwGtpv2cRspFailureIndInfoT          rspFailureInfo;
    NwGtpv2cCreateLocalTunnelInfoT      createLocalTunnelInfo;
    NwGtpv2cDeleteLocalTunnelInfoT      deleteLocalTunnelInfo;
  } apiInfo;
} NwGtpv2cUlpApiT;


/*--------------------------------------------------------------------------*
 *           S T A C K    E N T I T I E S    D E F I N I T I O N S          *
 *--------------------------------------------------------------------------*/

/**
 * Gtpv2c ULP entity definition
 */

typedef struct {
  NwGtpv2cUlpHandleT        hUlp;
  NwRcT (*ulpReqCallback) ( NW_IN        NwGtpv2cUlpHandleT hUlp,
                            NW_IN        NwGtpv2cUlpApiT *pUlpApi);
} NwGtpv2cUlpEntityT;


/**
 * Gtpv2c UDP entity definition
 */

typedef struct {
  NwGtpv2cUdpHandleT        hUdp;
  NwRcT (*udpDataReqCallback) ( NW_IN     NwGtpv2cUdpHandleT udpHandle,
                                NW_IN     uint8_t* dataBuf,
                                NW_IN     uint32_t dataSize,
                                NW_IN     uint32_t peerIp,
                                NW_IN     uint32_t peerPort);
} NwGtpv2cUdpEntityT;

/**
 * Gtpv2c Memory Manager entity definition
 */

typedef struct {
  NwGtpv2cMemMgrHandleT         hMemMgr;
  void* (*memAlloc)( NW_IN      NwGtpv2cMemMgrHandleT hMemMgr,
                     NW_IN      uint32_t memSize,
                     NW_IN      NwCharT* fileName,
                     NW_IN      uint32_t lineNumber);

  void (*memFree) ( NW_IN       NwGtpv2cMemMgrHandleT hMemMgr,
                    NW_IN       void* hMem,
                    NW_IN       NwCharT* fileName,
                    NW_IN       uint32_t lineNumber);
} NwGtpv2cMemMgrEntityT;


#define NW_GTPV2C_TMR_TYPE_ONE_SHOT                                  (0)
#define NW_GTPV2C_TMR_TYPE_REPETITIVE                                (1)
/**
 * Gtpv2c Timer Manager entity definition
 */

typedef struct {
  NwGtpv2cTimerMgrHandleT        tmrMgrHandle;
  NwRcT (*tmrStartCallback)( NW_IN       NwGtpv2cTimerMgrHandleT tmrMgrHandle,
                             NW_IN       uint32_t timeoutSec,
                             NW_IN       uint32_t timeoutUsec,
                             NW_IN       uint32_t tmrType,
                             NW_IN       void* tmrArg,
                             NW_OUT      NwGtpv2cTimerHandleT* tmrHandle);

  NwRcT (*tmrStopCallback) ( NW_IN       NwGtpv2cTimerMgrHandleT tmrMgrHandle,
                             NW_IN       NwGtpv2cTimerHandleT tmrHandle);
} NwGtpv2cTimerMgrEntityT;

/**
 * Gtpv2c Log manager entity definition
 */

typedef struct {
  NwGtpv2cLogMgrHandleT          logMgrHandle;
  NwRcT (*logReqCallback) (NW_IN      NwGtpv2cLogMgrHandleT logMgrHandle,
                           NW_IN      uint32_t logLevel,
                           NW_IN      NwCharT* filename,
                           NW_IN      uint32_t line,
                           NW_IN      NwCharT* logStr);
} NwGtpv2cLogMgrEntityT;


/*--------------------------------------------------------------------------*
 *                     P U B L I C   F U N C T I O N S                      *
 *--------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/**
 Constructor. Initialize nw-gtpv2c stack instance.

 @param[in,out] phGtpcStackHandle : Pointer to stack instance handle
 */

NwRcT
nwGtpv2cInitialize( NW_INOUT NwGtpv2cStackHandleT* phGtpcStackHandle);

/**
 Destructor. Destroy nw-gtpv2c stack instance .

 @param[in] hGtpcStackHandle : Stack instance handle
 */

NwRcT
nwGtpv2cFinalize( NW_IN  NwGtpv2cStackHandleT hGtpcStackHandle);

/**
 Set Configuration for the nw-gtpv2c stack.

 @param[in,out] phGtpcStackHandle : Pointer to stack handle
 */

NwRcT
nwGtpv2cConfigSet( NW_IN NwGtpv2cStackHandleT* phGtpcStackHandle, NW_IN NwGtpv2cStackConfigT* pConfig);

/**
 Get Configuration for the nw-gtpv2c stack.

 @param[in,out] phGtpcStackHandle : Pointer to stack handle
 */

NwRcT
nwGtpv2cConfigGet( NW_IN NwGtpv2cStackHandleT* phGtpcStackHandle, NW_OUT NwGtpv2cStackConfigT* pConfig);

/**
 Set ULP entity for the stack.

 @param[in] hGtpcStackHandle : Stack handle
 @param[in] pUlpEntity : Pointer to ULP entity.
 @return NW_OK on success.
 */

NwRcT
nwGtpv2cSetUlpEntity( NW_IN NwGtpv2cStackHandleT hGtpcStackHandle,
                      NW_IN NwGtpv2cUlpEntityT* pUlpEntity);

/**
 Set UDP entity for the stack.

 @param[in] hGtpcStackHandle : Stack handle
 @param[in] pUdpEntity : Pointer to UDP entity.
 @return NW_OK on success.
 */

NwRcT
nwGtpv2cSetUdpEntity( NW_IN NwGtpv2cStackHandleT hGtpcStackHandle,
                      NW_IN NwGtpv2cUdpEntityT* pUdpEntity);

/**
 Set MemMgr entity for the stack.

 @param[in] hGtpcStackHandle : Stack handle
 @param[in] pMemMgr : Pointer to Memory Manager.
 @return NW_OK on success.
 */

NwRcT
nwGtpv2cSetMemMgrEntity( NW_IN NwGtpv2cStackHandleT hGtpcStackHandle,
                         NW_IN NwGtpv2cMemMgrEntityT* pMemMgr);


/**
 Set TmrMgr entity for the stack.

 @param[in] hGtpcStackHandle : Stack handle
 @param[in] pTmrMgr : Pointer to Timer Manager.
 @return NW_OK on success.
 */

NwRcT
nwGtpv2cSetTimerMgrEntity( NW_IN NwGtpv2cStackHandleT hGtpcStackHandle,
                           NW_IN NwGtpv2cTimerMgrEntityT* pTmrMgr);

/**
 Set LogMgr entity for the stack.

 @param[in] hGtpcStackHandle : Stack handle
 @param[in] pLogMgr : Pointer to Log Manager.
 @return NW_OK on success.
 */

NwRcT
nwGtpv2cSetLogMgrEntity( NW_IN NwGtpv2cStackHandleT hGtpcStackHandle,
                         NW_IN NwGtpv2cLogMgrEntityT* pLogMgr);

/**
 Set log level for the stack.

 @param[in] hGtpcStackHandle : Stack handle
 @param[in] logLevel : Log Level.
 @return NW_OK on success.
 */

NwRcT
nwGtpv2cSetLogLevel( NW_IN NwGtpv2cStackHandleT hGtpcStackHandle,
                     NW_IN uint32_t logLevel);


/**
 Process Data Request from UDP entity.

 @param[in] hGtpcStackHandle : Stack handle
 @param[in] udpData : Pointer to received UDP data.
 @param[in] udpDataLen : Received data length.
 @param[in] dstPort : Received on port.
 @param[in] from : Received from peer information.
 @return NW_OK on success.
 */

NwRcT
nwGtpv2cProcessUdpReq( NW_IN NwGtpv2cStackHandleT hGtpcStackHandle,
                       NW_IN uint8_t* udpData,
                       NW_IN uint32_t udpDataLen,
                       NW_IN uint16_t peerPort,
                       NW_IN uint32_t peerIP);

/**
 Process Request from ULP entity.

 @param[in] hGtpcStackHandle : Stack handle
 @param[in] pLogMgr : Pointer to Ulp Req.
 @return NW_OK on success.
 */

NwRcT
nwGtpv2cProcessUlpReq( NW_IN NwGtpv2cStackHandleT hGtpcStackHandle,
                       NW_IN NwGtpv2cUlpApiT *ulpReq);


/**
 Process Timer timeout Request from Timer Manager

 @param[in] pLogMgr : Pointer timeout arguments.
 @return NW_OK on success.
 */

NwRcT
nwGtpv2cProcessTimeout( NW_IN void* timeoutArg);


#ifdef __cplusplus
}
#endif

#endif  /* __NW_GTPV2C_H__ */

/*--------------------------------------------------------------------------*
 *                      E N D     O F    F I L E                            *
 *--------------------------------------------------------------------------*/

