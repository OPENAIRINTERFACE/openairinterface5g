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
Source      EmmRegistered.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-REGISTERED state.

        In EMM-REGISTERED state, an EMM context has been established
        and a default EPS bearer context has been activated in the UE
        and the MME.
        The UE may initiate sending and receiving user data and signal-
        ling information and reply to paging. Additionally, tracking
        area updating or combined tracking area updating procedure is
        performed.

*****************************************************************************/

#include "emm_fsm.h"
#include "commonDef.h"
#include "networkDef.h"
#include "nas_log.h"

#include "emm_proc.h"

#include <assert.h>

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    EmmRegistered()                                           **
 **                                                                        **
 ** Description: Handles the behaviour of the UE and the MME while the     **
 **      EMM-SAP is in EMM-REGISTERED state.                       **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmRegistered(const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  assert(emm_fsm_get_status() == EMM_REGISTERED);


  switch (evt->primitive) {

  case _EMMREG_DETACH_INIT:
    /*
     * Initiate detach procedure for EPS services
     */
    rc = emm_proc_detach(EMM_DETACH_TYPE_EPS, evt->u.detach.switch_off);
    break;

  case _EMMREG_DETACH_REQ:
    /*
     * Network detach has been requested (Detach Request
     * message successfully delivered to the network);
     * enter state EMM-DEREGISTERED-INITIATED
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED_INITIATED);
    break;

  case _EMMREG_DETACH_CNF:
    /*
     * The UE implicitly detached from the network (all EPS
     * bearer contexts may have been deactivated)
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED);
    break;

  case _EMMREG_TAU_REQ:
    /*
     * TODO: Tracking Area Update has been requested
     */
    LOG_TRACE(ERROR, "EMM-FSM   - Tracking Area Update procedure "
              "is not implemented");
    break;

  case _EMMREG_SERVICE_REQ:
    /*
     * TODO: Service Request has been requested
     */
    LOG_TRACE(ERROR, "EMM-FSM   - Service Request procedure "
              "is not implemented");
    break;

  case _EMMREG_LOWERLAYER_SUCCESS:
    /*
     * Data transfer message has been successfully delivered
     */
    rc = emm_proc_lowerlayer_success();
    break;

  case _EMMREG_LOWERLAYER_FAILURE:
    /*
     * Data transfer message failed to be delivered
     */
    rc = emm_proc_lowerlayer_failure(FALSE);
    break;

  case _EMMREG_LOWERLAYER_RELEASE:
    /*
     * NAS signalling connection has been released
     */
    rc = emm_proc_lowerlayer_release();
    break;

  default:
    LOG_TRACE(ERROR, "EMM-FSM   - Primitive is not valid (%d)",
              evt->primitive);
    break;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

