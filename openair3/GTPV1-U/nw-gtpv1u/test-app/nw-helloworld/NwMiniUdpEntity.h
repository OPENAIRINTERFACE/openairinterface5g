/*----------------------------------------------------------------------------*
 *                                                                            *
 *            M I N I M A L I S T I C     U D P     E N T I T Y               *
 *                                                                            *
 *                    Copyright (C) 2010 Amit Chawre.                         *
 *                                                                            *
 *----------------------------------------------------------------------------*/


/**
 * @file NwMiniUdpEntity.h
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
  uint32_t        hSocket;
  NwEventT      ev;
  NwGtpv1uStackHandleT hGtpv1uStack;
} NwMiniUdpEntityT;

#ifdef __cplusplus
extern "C" {
#endif

NwGtpv1uRcT nwMiniUdpInit(NwMiniUdpEntityT* thiz, NwGtpv1uStackHandleT hGtpv1uStack, uint8_t* ipAddr);

NwGtpv1uRcT nwMiniUdpDestroy(NwMiniUdpEntityT* thiz);

NwGtpv1uRcT nwMiniUdpDataReq(NwGtpv1uUdpHandleT udpHandle,
                             uint8_t* dataBuf,
                             uint32_t dataSize,
                             uint32_t peerAddr,
                             uint32_t peerPort);

#ifdef __cplusplus
}
#endif

#endif
