/*----------------------------------------------------------------------------*
 *                                                                            *
 *         M I N I M A L I S T I C    T M R M G R     E N T I T Y             *
 *                                                                            *
 *                    Copyright (C) 2010 Amit Chawre.                         *
 *                                                                            *
 *----------------------------------------------------------------------------*/

/**
 * @file NwMiniTmrMgrEntity.c
 * @brief This file ontains example of a minimalistic timer manager entity.
*/

#include <stdio.h>
#include <assert.h>
#include "NwEvt.h"
#include "NwGtpv1u.h"
#include "NwMiniLogMgrEntity.h"
#include "NwMiniTmrMgrEntity.h"

#ifndef NW_ASSERT
#define NW_ASSERT assert
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------
 * Private functions
 *--------------------------------------------------------------------------*/

static void
NW_TMR_CALLBACK(nwMiniTmrMgrHandleTimeout)
{
  NwGtpv1uRcT rc;
  NwMiniTmrMgrEntityT *pTmr = (NwMiniTmrMgrEntityT *) arg;

  /*---------------------------------------------------------------------------
   *  Send Timeout Request to GTPv1u Stack Instance
   *--------------------------------------------------------------------------*/

  rc = nwGtpv1uProcessTimeout(pTmr->timeoutArg);
  NW_ASSERT( rc == NW_GTPV1U_OK );

  free(pTmr);

  return;
}

/*---------------------------------------------------------------------------
 * Public functions
 *--------------------------------------------------------------------------*/

NwGtpv1uRcT nwTimerStart( NwGtpv1uTimerMgrHandleT tmrMgrHandle,
                          uint32_t timeoutSec,
                          uint32_t timeoutUsec,
                          uint32_t tmrType,
                          void  *timeoutArg,
                          NwGtpv1uTimerHandleT *hTmr)
{
  NwGtpv1uRcT rc = NW_GTPV1U_OK;
  NwMiniTmrMgrEntityT *pTmr;
  struct timeval tv;

  NW_LOG(NW_LOG_LEVEL_INFO,
         "Received start timer request from stack with timer type %u, arg %x, for %u sec and %u usec",
         tmrType, timeoutArg, timeoutSec, timeoutUsec);

  pTmr = (NwMiniTmrMgrEntityT *) malloc (sizeof(NwMiniTmrMgrEntityT));

  /* set the timevalues*/
  timerclear(&tv);
  tv.tv_sec     = timeoutSec;
  tv.tv_usec    = timeoutUsec;

  pTmr->timeoutArg = timeoutArg;
  evtimer_set(&pTmr->ev, nwMiniTmrMgrHandleTimeout, pTmr);

  /*add event*/

  event_add(&(pTmr->ev), &tv);

  *hTmr = (NwGtpv1uTimerHandleT)pTmr;

  return rc;
}

NwGtpv1uRcT nwTimerStop( NwGtpv1uTimerMgrHandleT tmrMgrHandle,
                         NwGtpv1uTimerHandleT hTmr)
{
  NW_LOG(NW_LOG_LEVEL_INFO,
         "Received stop timer request from stack for timer handle %u", hTmr);
  evtimer_del(&(((NwMiniTmrMgrEntityT *)hTmr)->ev));
  free((void *)hTmr);
  return NW_GTPV1U_OK;
}

#ifdef __cplusplus
}
#endif
