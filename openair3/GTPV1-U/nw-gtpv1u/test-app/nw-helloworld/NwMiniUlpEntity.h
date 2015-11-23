/*----------------------------------------------------------------------------*
 *                                                                            *
 *            M I N I M A L I S T I C     U L P     E N T I T Y               *
 *                                                                            *
 *                    Copyright (C) 2010 Amit Chawre.                         *
 *                                                                            *
 *----------------------------------------------------------------------------*/

/**
 * @file NwMiniUlpEntity.h
 * @brief This file contains example of a minimalistic ULP entity.
*/

#include <stdio.h>
#include <assert.h>
#include "NwEvt.h"
#include "NwLog.h"

#ifndef __NW_MINI_ULP_H__
#define __NW_MINI_ULP_H__

typedef struct {
  int                           hSocket;
  uint16_t                        seqNum;
  uint8_t                         restartCounter;
  uint8_t                         localIpStr[16];
  uint8_t                         peerIpStr[16];
  uint32_t                        localPort[1025];
  NwEventT                      ev[1025];
  NwGtpv1uStackHandleT          hGtpv1uStack;
  NwGtpv1uStackSessionHandleT   hGtpv1uConn;
} NwMiniUlpEntityT;

#ifdef __cplusplus
extern "C" {
#endif

NwGtpv1uRcT
nwMiniUlpInit(NwMiniUlpEntityT* thiz, NwGtpv1uStackHandleT hGtpv1uStack);

NwGtpv1uRcT
nwMiniUlpDestroy(NwMiniUlpEntityT* thiz);

NwGtpv1uRcT
nwMiniUlpCreateConn(NwMiniUlpEntityT* thiz, char* localIpStr, uint16_t localPort, char* peerIpStr);

NwGtpv1uRcT
nwMiniUlpDestroyConn(NwMiniUlpEntityT* thiz);

NwGtpv1uRcT
nwMiniUlpSendMsg(NwMiniUlpEntityT* thiz);

NwGtpv1uRcT
nwMiniUlpTpduSend(NwMiniUlpEntityT* thiz, uint8_t* tpduBuf, uint32_t tpduLen , uint16_t fromPort);

NwGtpv1uRcT
nwMiniUlpProcessStackReqCallback (NwGtpv1uUlpHandleT hUlp,
                                  NwGtpv1uUlpApiT *pUlpApi);

#ifdef __cplusplus
}
#endif

#endif
