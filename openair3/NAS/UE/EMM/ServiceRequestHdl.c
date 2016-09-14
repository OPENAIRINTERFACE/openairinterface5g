/*****************************************************************************
Source      ServiceRequest.c

Version     0.1

Date        2013/05/07

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the service request EMM procedure executed by the
        Non-Access Stratum.

        The purpose of the service request procedure is to transfer
        the EMM mode from EMM-IDLE to EMM-CONNECTED mode and establish
        the radio and S1 bearers when uplink user data or signalling
        is to be sent.

        This procedure is used when the network has downlink signalling
        pending, the UE has uplink signalling pending, the UE or the
        network has user data pending and the UE is in EMM-IDLE mode.

*****************************************************************************/

#include "emm_proc.h"
#include "nas_log.h"
#include "nas_timer.h"

#include "emmData.h"

#include "emm_sap.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the service request procedure in the UE
 * --------------------------------------------------------------------------
 */
/*
 * Timer handlers
 */
void *_emm_service_t3417_handler(void *);

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the service request procedure in the MME
 * --------------------------------------------------------------------------
 */


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *              Timer handlers
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _emm_service_t3417_handler()                              **
 **                                                                        **
 ** Description: T3417 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 5.6.1.6 case c                    **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void *_emm_service_t3417_handler(void *args)
{
  LOG_FUNC_IN;

  LOG_TRACE(WARNING, "EMM-PROC  - T3417 timer expired");

  /* Stop timer T3417 */
  T3417.id = nas_timer_stop(T3417.id);

  LOG_FUNC_RETURN(NULL);
}

