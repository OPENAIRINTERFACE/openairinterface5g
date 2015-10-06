/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************
Source      TrackingAreaUpdate.c

Version     0.1

Date        2013/05/07

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the tracking area update EMM procedure executed by the
        Non-Access Stratum.

        The tracking area updating procedure is always initiated by the
        UE and is used to update the registration of the actual tracking
        area of a UE in the network, to periodically notify the availa-
        bility of the UE to the network, for MME load balancing, to up-
        date certain UE specific parameters in the network.

*****************************************************************************/

#include "emm_proc.h"
#include "nas_log.h"
#include "nas_timer.h"

#include "emmData.h"

#include "emm_sap.h"
#include "emm_cause.h"

#include <string.h> // memcmp, memcpy

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *   Internal data handled by the tracking area update procedure in the UE
 * --------------------------------------------------------------------------
 */
/*
 * Timer handlers
 */
void *_emm_tau_t3430_handler(void *);

/*
 * --------------------------------------------------------------------------
 *   Internal data handled by the tracking area update procedure in the MME
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
 ** Name:        _emm_tau_t3430_handler()                                  **
 **                                                                        **
 ** Description: T3430 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 5.5.3.2.6 case c                  **
 **                                                                        **
 ** Inputs:      args:          handler parameters                         **
 **              Others:        None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **              Return:        None                                       **
 **              Others:        None                                       **
 **                                                                        **
 ***************************************************************************/
void *_emm_tau_t3430_handler(void *args)
{
  LOG_FUNC_IN;

  LOG_TRACE(WARNING, "EMM-PROC  - T3430 timer expired");

  /* Stop timer T3430 */
  T3430.id = nas_timer_stop(T3430.id);

  LOG_FUNC_RETURN(NULL);
}


