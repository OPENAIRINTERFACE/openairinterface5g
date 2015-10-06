/*----------------------------------------------------------------------------*
 *                                                                            *
 *                             n w - g t p v 2 u                              *
 *    G P R S    T u n n e l i n g    P r o t o c o l   v 2 u   S t a c k     *
 *                                                                            *
 *           M I N I M A L I S T I C     D E M O N S T R A T I O N            *
 *                                                                            *
 *                    Copyright (C) 2010 Amit Chawre.                         *
 *                                                                            *
 *----------------------------------------------------------------------------*/


/**
 * @file hello-world.c
 * @brief This is a test program demostrating usage of nw-gtpv2 library.
*/

#include <stdio.h>
#include <assert.h>
#include "NwEvt.h"
#include "NwGtpv1u.h"

#include "NwMiniLogMgrEntity.h"
#include "NwMiniTmrMgrEntity.h"
#include "NwMiniUdpEntity.h"
#include "NwMiniUlpEntity.h"

#ifndef NW_ASSERT
#define NW_ASSERT assert
#endif

/*---------------------------------------------------------------------------
 *                T H E      M A I N      F U N C T I O N
 *--------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
  NwGtpv1uRcT rc;
  char                         *logLevelStr;
  uint32_t                        logLevel;
  uint32_t                        num_of_connections;

  NwGtpv1uStackHandleT          hGtpv1uStack = 0;
  NwMiniUlpEntityT              ulpObj;
  NwMiniUdpEntityT              udpObj;
  NwMiniLogMgrT                 logObj;
  NwGtpv1uUlpEntityT            ulp;
  NwGtpv1uUdpEntityT            udp;
  NwGtpv1uTimerMgrEntityT       tmrMgr;
  NwGtpv1uLogMgrEntityT         logMgr;

  if(argc != 4) {
    printf("Usage: %s <num-of-connections> <local-ip> <peer-ip>\n", argv[0]);
    exit(0);
  }


  logLevelStr = getenv ("NW_LOG_LEVEL");

  if(logLevelStr == NULL) {
    logLevel = NW_LOG_LEVEL_INFO;
  } else {
    if(strncmp(logLevelStr, "EMER",4) == 0) {
      logLevel = NW_LOG_LEVEL_EMER;
    } else if(strncmp(logLevelStr, "ALER",4) == 0) {
      logLevel = NW_LOG_LEVEL_ALER;
    } else if(strncmp(logLevelStr, "CRIT",4) == 0) {
      logLevel = NW_LOG_LEVEL_CRIT;
    } else if(strncmp(logLevelStr, "ERRO",4) == 0) {
      logLevel = NW_LOG_LEVEL_ERRO ;
    } else if(strncmp(logLevelStr, "WARN",4) == 0) {
      logLevel = NW_LOG_LEVEL_WARN;
    } else if(strncmp(logLevelStr, "NOTI",4) == 0) {
      logLevel = NW_LOG_LEVEL_NOTI;
    } else if(strncmp(logLevelStr, "INFO",4) == 0) {
      logLevel = NW_LOG_LEVEL_INFO;
    } else if(strncmp(logLevelStr, "DEBG",4) == 0) {
      logLevel = NW_LOG_LEVEL_DEBG;
    }
  }

  /*---------------------------------------------------------------------------
   *  Initialize event library
   *--------------------------------------------------------------------------*/

  NW_EVT_INIT();

  /*---------------------------------------------------------------------------
   *  Initialize Log Manager
   *--------------------------------------------------------------------------*/

  nwMiniLogMgrInit(&logObj, logLevel);

  /*---------------------------------------------------------------------------
   *  Create GTPv1u Stack Instance
   *--------------------------------------------------------------------------*/

  rc = nwGtpv1uInitialize(&hGtpv1uStack, GTPU_STACK_ENB);

  if(rc != NW_GTPV1U_OK) {
    NW_LOG(NW_LOG_LEVEL_ERRO,
           "Failed to create gtpv1u stack instance. Error '%u' occured", rc);
    exit(1);
  }

  NW_LOG(NW_LOG_LEVEL_INFO, "Gtpv1u Stack Handle '%X' Creation Successful!",
         hGtpv1uStack);

  /*---------------------------------------------------------------------------
   * Set up Ulp Entity
   *--------------------------------------------------------------------------*/

  rc = nwMiniUlpInit(&ulpObj, hGtpv1uStack);
  NW_ASSERT( rc == NW_GTPV1U_OK );

  ulp.hUlp = (NwGtpv1uUlpHandleT) &ulpObj;
  ulp.ulpReqCallback = nwMiniUlpProcessStackReqCallback;

  rc = nwGtpv1uSetUlpEntity(hGtpv1uStack, &ulp);
  NW_ASSERT( rc == NW_GTPV1U_OK );


  /*---------------------------------------------------------------------------
   * Set up Udp Entity
   *--------------------------------------------------------------------------*/

  rc = nwMiniUdpInit(&udpObj, hGtpv1uStack, (argv[2]));
  NW_ASSERT( rc == NW_GTPV1U_OK );

  udp.hUdp = (NwGtpv1uUdpHandleT) &udpObj;
  udp.udpDataReqCallback = nwMiniUdpDataReq;

  rc = nwGtpv1uSetUdpEntity(hGtpv1uStack, &udp);
  NW_ASSERT( rc == NW_GTPV1U_OK );

  /*---------------------------------------------------------------------------
   * Set up Log Entity
   *--------------------------------------------------------------------------*/

  tmrMgr.tmrMgrHandle = 0;
  tmrMgr.tmrStartCallback = nwTimerStart;
  tmrMgr.tmrStopCallback = nwTimerStop;

  rc = nwGtpv1uSetTimerMgrEntity(hGtpv1uStack, &tmrMgr);
  NW_ASSERT( rc == NW_GTPV1U_OK );

  /*---------------------------------------------------------------------------
   * Set up Log Entity
   *--------------------------------------------------------------------------*/

  logMgr.logMgrHandle   = (NwGtpv1uLogMgrHandleT) &logObj;
  logMgr.logReqCallback  = nwMiniLogMgrLogRequest;

  rc = nwGtpv1uSetLogMgrEntity(hGtpv1uStack, &logMgr);
  NW_ASSERT( rc == NW_GTPV1U_OK );

  /*---------------------------------------------------------------------------
   * Set GTPv1u log level
   *--------------------------------------------------------------------------*/

  rc = nwGtpv1uSetLogLevel(hGtpv1uStack, logLevel);

  /*---------------------------------------------------------------------------
   *  Send Create Session Request to GTPv1u Stack Instance
   *--------------------------------------------------------------------------*/

  num_of_connections = atoi(argv[1]);

  while ( num_of_connections-- ) {
    //rc = nwMiniUlpCreateConn(&ulpObj, argv[2], 1234 + num_of_connections, argv[3]);
    rc = nwMiniUlpSendEchoRequestToPeer(&ulpObj, inet_addr(argv[3]));
    NW_ASSERT( rc == NW_GTPV1U_OK );
  }

  /*---------------------------------------------------------------------------
   * Event loop
   *--------------------------------------------------------------------------*/

  NW_EVT_LOOP();

  NW_LOG(NW_LOG_LEVEL_ERRO, "Exit from eventloop, no events to process!");

  /*---------------------------------------------------------------------------
   *  Send Destroy Session Request to GTPv1u Stack Instance
   *--------------------------------------------------------------------------*/

  rc = nwMiniUlpDestroyConn(&ulpObj);
  NW_ASSERT( rc == NW_GTPV1U_OK );


  /*---------------------------------------------------------------------------
   *  Destroy GTPv1u Stack Instance
   *--------------------------------------------------------------------------*/

  rc = nwGtpv1uFinalize(hGtpv1uStack);

  if(rc != NW_GTPV1U_OK) {
    NW_LOG(NW_LOG_LEVEL_ERRO,
           "Failed to finalize gtpv1u stack instance. Error '%u' occured", rc);
  } else {
    NW_LOG(NW_LOG_LEVEL_INFO, "Gtpv1u Stack Handle '%X' Finalize Successful!",
           hGtpv1uStack);
  }


  return rc;
}
