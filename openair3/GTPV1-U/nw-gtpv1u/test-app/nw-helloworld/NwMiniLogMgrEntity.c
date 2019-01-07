/*----------------------------------------------------------------------------*
 *                                                                            *
 *         M I N I M A L I S T I C    L O G M G R     E N T I T Y             *
 *                                                                            *
 *                    Copyright (C) 2010 Amit Chawre.                         *
 *                                                                            *
 *----------------------------------------------------------------------------*/

/**
 * @file hello-world.c
 * @brief This file contains example of a minimalistic log manager entity.
*/

#include <stdio.h>
#include <assert.h>
#include "NwLog.h"
#include "NwEvt.h"
#include "NwGtpv1u.h"

#include "NwMiniLogMgrEntity.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t g_log_level = NW_LOG_LEVEL_INFO;

/*---------------------------------------------------------------------------
 * Public functions
 *--------------------------------------------------------------------------*/

NwGtpv1uRcT nwMiniLogMgrInit(NwMiniLogMgrT *thiz, uint32_t logLevel )
{
  thiz->logLevel = logLevel;
  return NW_GTPV1U_OK;
}

NwGtpv1uRcT nwMiniLogMgrSetLogLevel(NwMiniLogMgrT *thiz, uint32_t logLevel)
{
  thiz->logLevel = logLevel;
}

NwGtpv1uRcT nwMiniLogMgrLogRequest (NwGtpv1uLogMgrHandleT hLogMgr,
                                    uint32_t logLevel,
                                    NwCharT *file,
                                    uint32_t line,
                                    NwCharT *logStr)
{
  NwMiniLogMgrT *thiz = (NwMiniLogMgrT *) hLogMgr;

  if(thiz->logLevel >= logLevel) {
    printf("NWGTPv2U-STK  %s - %s <%s,%u>\n", gLogLevelStr[logLevel], logStr,
           basename(file), line);
  }

  return NW_GTPV1U_OK;
}

#ifdef __cplusplus
}
#endif

