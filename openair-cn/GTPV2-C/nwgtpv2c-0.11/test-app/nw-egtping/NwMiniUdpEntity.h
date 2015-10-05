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
#include <assert.h>
#include "NwEvt.h"
#include "NwLog.h"

#ifndef NW_ASSERT
#define NW_ASSERT assert
#endif

#ifndef __NW_MINI_UDP_ENTITY_H__
#define __NW_MINI_UDP_ENTITY_H__

typedef struct {
  uint32_t                        ipv4Addr;
  uint32_t                        hSocket;
  NwEventT                      ev;
  NwGtpv2cStackHandleT          hGtpv2cStack;
  uint32_t                        packetsSent;
  uint32_t                        packetsRcvd;
} NwGtpv2cNodeUdpT;

#ifdef __cplusplus
extern "C" {
#endif

NwRcT nwGtpv2cUdpInit(NwGtpv2cNodeUdpT* thiz, NwGtpv2cStackHandleT hGtpv2cStack, uint8_t* ipAddrStr);

NwRcT nwGtpv2cUdpDestroy(NwGtpv2cNodeUdpT* thiz);

NwRcT nwGtpv2cUdpDataReq(NwGtpv2cUdpHandleT udpHandle,
                         uint8_t* dataBuf,
                         uint32_t dataSize,
                         uint32_t peerIp,
                         uint32_t peerPort);

NwRcT nwGtpv2cUdpReset(NwGtpv2cNodeUdpT* thiz);

#ifdef __cplusplus
}
#endif

#endif
