/*----------------------------------------------------------------------------*
 *                                                                            *
 *            M I N I M A L I S T I C     U L P     E N T I T Y               *
 *                                                                            *
 *                    Copyright (C) 2010 Amit Chawre.                         *
 *                                                                            *
 *----------------------------------------------------------------------------*/

/**
 * @file NwMiniUlpEntity.c
 * @brief This file contains example of a minimalistic ULP entity.
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "NwEvt.h"
#include "NwGtpv1u.h"
#include "NwGtpv1uIe.h"
#include "NwMiniLogMgrEntity.h"
#include "NwMiniUlpEntity.h"

#ifndef NW_ASSERT
#define NW_ASSERT assert
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 * Private Functions
 *--------------------------------------------------------------------------*/

#define MAX_UDP_PAYLOAD_LEN                     (4096)

NwGtpv1uRcT
nwMiniUlpSendEchoRequestToPeer(NwMiniUlpEntityT *thiz, uint32_t peerIp)
{
  NwGtpv1uRcT rc;
  NwGtpv1uUlpApiT           ulpReq;

  /*
   *  Send Message Request to Gtpv1u Stack Instance
   */

  ulpReq.apiType = NW_GTPV1U_ULP_API_INITIAL_REQ;

  ulpReq.apiInfo.initialReqInfo.hUlpTrxn        = (NwGtpv1uUlpTrxnHandleT)thiz;
  ulpReq.apiInfo.initialReqInfo.teid            = 0x00;
  ulpReq.apiInfo.initialReqInfo.peerIp          = (peerIp);
  ulpReq.apiInfo.initialReqInfo.peerPort        = 2152;

  /* Send Echo Request*/

  rc = nwGtpv1uMsgNew( thiz->hGtpv1uStack,
                       NW_TRUE,          /* SeqNum flag    */
                       NW_FALSE,
                       NW_FALSE,
                       NW_GTP_ECHO_REQ,  /* Msg Type             */
                       0x00000000UL,     /* TEID                 */
                       0x5678,           /* Seq Number           */
                       0,
                       0,
                       (&ulpReq.hMsg));

  NW_ASSERT( rc == NW_GTPV1U_OK );

  rc = nwGtpv1uMsgAddIeTV1((ulpReq.hMsg), NW_GTPV1U_IE_RECOVERY,
                           thiz->restartCounter);
  NW_ASSERT( rc == NW_GTPV1U_OK );

  rc = nwGtpv1uProcessUlpReq(thiz->hGtpv1uStack, &ulpReq);
  NW_ASSERT( rc == NW_GTPV1U_OK );

  return NW_GTPV1U_OK;
}

static
void NW_EVT_CALLBACK(nwMiniUlpDataIndicationCallbackData)
{
  NwMiniUlpEntityT *thiz = (NwMiniUlpEntityT *) arg;
  NwGtpv1uRcT         rc;
  uint8_t         udpBuf[MAX_UDP_PAYLOAD_LEN];
  NwS32T        bytesRead;
  uint32_t        peerLen;
  struct sockaddr_in peer;

  peerLen = sizeof(peer);

  bytesRead = recvfrom(thiz->hSocket, udpBuf, MAX_UDP_PAYLOAD_LEN , 0,
                       (struct sockaddr *) &peer,(socklen_t *) &peerLen);

  if(bytesRead) {
    NW_LOG(NW_LOG_LEVEL_DEBG, "Received UDP message of length %u from %X:%u",
           bytesRead, ntohl(peer.sin_addr.s_addr), ntohs(peer.sin_port));
    rc = nwMiniUlpTpduSend(thiz, udpBuf, bytesRead, thiz->localPort[fd]);
  } else {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
  }
}

/*---------------------------------------------------------------------------
 * Public Functions
 *--------------------------------------------------------------------------*/

NwGtpv1uRcT
nwMiniUlpInit(NwMiniUlpEntityT *thiz, NwGtpv1uStackHandleT hGtpv1uStack)
{
  NwGtpv1uRcT rc;
  thiz->hGtpv1uStack = hGtpv1uStack;

  return NW_GTPV1U_OK;
}

NwGtpv1uRcT
nwMiniUlpDestroy(NwMiniUlpEntityT *thiz)
{
  NW_ASSERT(thiz);
  memset(thiz, 0, sizeof(NwMiniUlpEntityT));
  return NW_GTPV1U_OK;
}

NwGtpv1uRcT
nwMiniUlpCreateConn(NwMiniUlpEntityT *thiz, char *localIpStr, uint16_t localport,
                    char *peerIpStr)
{
  NwGtpv1uRcT rc;
  int sd;
  struct sockaddr_in addr;
  NwGtpv1uUlpApiT           ulpReq;

  strcpy(thiz->peerIpStr, peerIpStr);

  /*
   * Create local tunnel endpoint
   */

  NW_LOG(NW_LOG_LEVEL_NOTI, "Creating tunnel endpoint with teid %d", localport);
  ulpReq.apiType                                        =
    NW_GTPV1U_ULP_API_CREATE_TUNNEL_ENDPOINT;
  ulpReq.apiInfo.createTunnelEndPointInfo.teid          = localport;
  ulpReq.apiInfo.createTunnelEndPointInfo.hUlpSession   =
    (NwGtpv1uUlpSessionHandleT)thiz;

  rc = nwGtpv1uProcessUlpReq(thiz->hGtpv1uStack, &ulpReq);
  NW_ASSERT( rc == NW_GTPV1U_OK );

  thiz->hGtpv1uConn = ulpReq.apiInfo.createTunnelEndPointInfo.hStackSession;

  /*
   * Create local udp listening endpoint
   */

  sd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sd < 0) {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }

  addr.sin_family       = AF_INET;
  addr.sin_port         = htons(localport);
  addr.sin_addr.s_addr  = inet_addr(localIpStr);
  memset(addr.sin_zero, '\0', sizeof (addr.sin_zero));

  if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }

  event_set(&(thiz->ev[sd]), sd, EV_READ|EV_PERSIST,
            nwMiniUlpDataIndicationCallbackData, thiz);
  event_add(&(thiz->ev[sd]), NULL);

  thiz->localPort[sd] = localport;

  /*
   * Create local udp for sendign data
   */

  sd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sd < 0) {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }


  thiz->hSocket = sd;
  return NW_GTPV1U_OK;
}

NwGtpv1uRcT
nwMiniUlpDestroyConn(NwMiniUlpEntityT *thiz)
{
  NwGtpv1uRcT rc;
  NwGtpv1uUlpApiT           ulpReq;
  /*---------------------------------------------------------------------------
   *  Send Destroy Session Request to GTPv1u Stack Instance
   *--------------------------------------------------------------------------*/

  ulpReq.apiType = NW_GTPV1U_ULP_API_DESTROY_TUNNEL_ENDPOINT;
  ulpReq.apiInfo.destroyTunnelEndPointInfo.hStackSessionHandle = thiz->hGtpv1uConn;

  rc = nwGtpv1uProcessUlpReq(thiz->hGtpv1uStack, &ulpReq);
  NW_ASSERT( rc == NW_GTPV1U_OK );

  thiz->hGtpv1uConn = 0;

  return NW_GTPV1U_OK;
}


NwGtpv1uRcT
nwMiniUlpTpduSend(NwMiniUlpEntityT *thiz, uint8_t *tpduBuf, uint32_t tpduLen ,
                  uint16_t fromPort)
{
  NwGtpv1uRcT rc;
  NwGtpv1uUlpApiT           ulpReq;

  /*
   *  Send Message Request to GTPv1u Stack Instance
   */

  ulpReq.apiType                        = NW_GTPV1U_ULP_API_SEND_TPDU;
  ulpReq.apiInfo.sendtoInfo.teid        = fromPort;
  ulpReq.apiInfo.sendtoInfo.ipAddr      = inet_addr(thiz->peerIpStr);

  rc = nwGtpv1uGpduMsgNew( thiz->hGtpv1uStack,
                           fromPort,
                           NW_FALSE,
                           thiz->seqNum++,
                           tpduBuf,
                           tpduLen,
                           &(ulpReq.apiInfo.sendtoInfo.hMsg));

  NW_ASSERT( rc == NW_GTPV1U_OK );

  rc = nwGtpv1uProcessUlpReq(thiz->hGtpv1uStack, &ulpReq);
  NW_ASSERT( rc == NW_GTPV1U_OK );

  rc = nwGtpv1uMsgDelete(thiz->hGtpv1uStack, (ulpReq.apiInfo.sendtoInfo.hMsg));
  NW_ASSERT( rc == NW_GTPV1U_OK );

  return NW_GTPV1U_OK;
}

NwGtpv1uRcT
nwMiniUlpProcessStackReqCallback (NwGtpv1uUlpHandleT hUlp,
                                  NwGtpv1uUlpApiT *pUlpApi)
{
  NwMiniUlpEntityT *thiz;
  NW_ASSERT(pUlpApi != NULL);

  thiz = (NwMiniUlpEntityT *) hUlp;

  switch(pUlpApi->apiType) {
  case NW_GTPV1U_ULP_API_RECV_TPDU: {
    struct sockaddr_in peerAddr;
    NwS32T bytesSent;
    uint8_t dataBuf[4096];
    uint32_t dataSize;
    uint32_t peerIpAddr = (inet_addr(thiz->peerIpStr));

    NW_ASSERT( NW_GTPV1U_OK == nwGtpv1uMsgGetTpdu(pUlpApi->apiInfo.recvMsgInfo.hMsg,
               dataBuf, &dataSize) );

    NW_LOG(NW_LOG_LEVEL_DEBG, "Received TPDU from gtpv1u stack %u!",
           pUlpApi->apiInfo.recvMsgInfo.teid);

    peerAddr.sin_family       = AF_INET;
    peerAddr.sin_port         = htons(pUlpApi->apiInfo.recvMsgInfo.teid);
    peerAddr.sin_addr.s_addr  = (peerIpAddr);
    memset(peerAddr.sin_zero, '\0', sizeof (peerAddr.sin_zero));

    bytesSent = sendto (thiz->hSocket, dataBuf, dataSize, 0,
                        (struct sockaddr *) &peerAddr, sizeof(peerAddr));

    if(bytesSent < 0) {
      NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
      NW_ASSERT(0);
    } else {
      NW_LOG(NW_LOG_LEVEL_DEBG, "Sent %u bytes to peer %u.%u.%u.%u", dataSize,
             (peerIpAddr & 0x000000ff),
             (peerIpAddr & 0x0000ff00) >> 8,
             (peerIpAddr & 0x00ff0000) >> 16,
             (peerIpAddr & 0xff000000) >> 24);
    }

    NW_ASSERT(nwGtpv1uMsgDelete(thiz->hGtpv1uStack,
                                (pUlpApi->apiInfo.recvMsgInfo.hMsg)) == NW_GTPV1U_OK);

  }
  break;

  default:
    NW_LOG(NW_LOG_LEVEL_WARN, "Received undefined UlpApi (%d) from gtpv1u stack!",
           pUlpApi->apiType);
  }

  return NW_GTPV1U_OK;
}

#ifdef __cplusplus
}
#endif

