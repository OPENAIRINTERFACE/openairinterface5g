/*----------------------------------------------------------------------------*
 *                                                                            *
 *            M I N I M A L I S T I C     U D P     E N T I T Y               *
 *                                                                            *
 *                    Copyright (C) 2010 Amit Chawre.                         *
 *                                                                            *
 *----------------------------------------------------------------------------*/


/**
 * @file NwMiniUdpEntity.c
 * @brief This file contains example of a minimalistic ULP entity.
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "NwEvt.h"
#include "NwGtpv1u.h"
#include "NwMiniLogMgrEntity.h"
#include "NwMiniUdpEntity.h"

#ifndef NW_ASSERT
#define NW_ASSERT assert
#endif

#define MAX_UDP_PAYLOAD_LEN             (4096)

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 * Private functions
 *--------------------------------------------------------------------------*/

static
void NW_TMR_CALLBACK(nwUdpDataIndicationCallbackData)
{
  NwGtpv1uRcT         rc;
  uint8_t         udpBuf[MAX_UDP_PAYLOAD_LEN];
  NwS32T        bytesRead;
  uint32_t        peerLen;
  struct sockaddr_in peer;
  NwMiniUdpEntityT *thiz = (NwMiniUdpEntityT *) arg;

  peerLen = sizeof(peer);

  bytesRead = recvfrom(thiz->hSocket, udpBuf, MAX_UDP_PAYLOAD_LEN , 0,
                       (struct sockaddr *) &peer,(socklen_t *) &peerLen);

  if(bytesRead) {
    NW_LOG(NW_LOG_LEVEL_DEBG, "Received UDP message of length %u from %X:%u",
           bytesRead, ntohl(peer.sin_addr.s_addr), ntohs(peer.sin_port));
    rc = nwGtpv1uProcessUdpReq(thiz->hGtpv1uStack, udpBuf, bytesRead,
                               peer.sin_port, peer.sin_addr.s_addr);
  } else {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
  }
}


/*---------------------------------------------------------------------------
 * Public functions
 *--------------------------------------------------------------------------*/

NwGtpv1uRcT nwMiniUdpInit(NwMiniUdpEntityT *thiz,
                          NwGtpv1uStackHandleT hGtpv1uStack, uint8_t *ipAddr)
{
  int sd;
  struct sockaddr_in addr;

  sd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sd < 0) {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }

  addr.sin_family       = AF_INET;
  addr.sin_port         = htons(2152);
  addr.sin_addr.s_addr  = inet_addr(ipAddr);
  memset(addr.sin_zero, '\0', sizeof (addr.sin_zero));

  if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
    NW_ASSERT(0);
  }

  event_set(&(thiz->ev), sd, EV_READ|EV_PERSIST, nwUdpDataIndicationCallbackData,
            thiz);
  event_add(&(thiz->ev), NULL);

  thiz->hSocket = sd;
  thiz->hGtpv1uStack = hGtpv1uStack;

  return NW_GTPV1U_OK;
}

NwGtpv1uRcT nwMiniUdpDestroy(NwMiniUdpEntityT *thiz)
{
  close(thiz->hSocket);
}

NwGtpv1uRcT nwMiniUdpDataReq(NwGtpv1uUdpHandleT udpHandle,
                             uint8_t *dataBuf,
                             uint32_t dataSize,
                             uint32_t peerIpAddr,
                             uint32_t peerPort)
{
  struct sockaddr_in peerAddr;
  NwS32T bytesSent;
  NwMiniUdpEntityT *thiz = (NwMiniUdpEntityT *) udpHandle;

  peerAddr.sin_family       = AF_INET;
  peerAddr.sin_port         = htons(peerPort);
  peerAddr.sin_addr.s_addr  = (peerIpAddr);
  memset(peerAddr.sin_zero, '\0', sizeof (peerAddr.sin_zero));

  NW_LOG(NW_LOG_LEVEL_DEBG,
         "Sending %u bytes on handle 0x%x to peer %u.%u.%u.%u:%u", dataSize, udpHandle,
         (peerIpAddr & 0x000000ff),
         (peerIpAddr & 0x0000ff00) >> 8,
         (peerIpAddr & 0x00ff0000) >> 16,
         (peerIpAddr & 0xff000000) >> 24,
         peerPort);

  bytesSent = sendto (thiz->hSocket, dataBuf, dataSize, 0,
                      (struct sockaddr *) &peerAddr, sizeof(peerAddr));

  if(bytesSent < 0) {
    NW_LOG(NW_LOG_LEVEL_ERRO, "%s", strerror(errno));
  } else {
    NW_LOG(NW_LOG_LEVEL_DEBG, "Sent %u bytes on handle 0x%x to peer %u.%u.%u.%u",
           dataSize, udpHandle,
           (peerIpAddr & 0x000000ff),
           (peerIpAddr & 0x0000ff00) >> 8,
           (peerIpAddr & 0x00ff0000) >> 16,
           (peerIpAddr & 0xff000000) >> 24);

  }

  return NW_GTPV1U_OK;
}



#ifdef __cplusplus
}
#endif

