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
#include "NwEvt.h"
#include "NwLog.h"

#ifndef NW_ASSERT
#define NW_ASSERT assert
#endif

#ifndef __NW_MINI_LOG_MGR_H__
#define __NW_MINI_LOG_MGR_H__

extern uint32_t g_log_level;


#define NW_LOG( _logLevel, ...)                                         \
  do {                                                                  \
    if(g_log_level >= _logLevel)                                        \
    {                                                                   \
      char _logStr[1024];                                               \
      snprintf(_logStr, 1024, __VA_ARGS__);                             \
      printf("NWGTPv2U-APP  %s - %s <%s,%d>\n", gLogLevelStr[_logLevel], _logStr, basename(__FILE__), __LINE__);\
    }                                                                   \
  } while(0)

typedef struct {
  uint8_t  logLevel;
} NwMiniLogMgrT;

#ifdef __cplusplus
extern "C" {
#endif

NwGtpv1uRcT nwMiniLogMgrLogRequest (NwGtpv1uLogMgrHandleT logMgrHandle,
                                    uint32_t logLevel,
                                    NwCharT* file,
                                    uint32_t line,
                                    NwCharT* logStr);

#ifdef __cplusplus
}
#endif

#endif
