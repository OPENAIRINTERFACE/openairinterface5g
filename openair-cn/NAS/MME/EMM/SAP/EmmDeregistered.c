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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************

Source      EmmDeregistered.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-DEREGISTERED state.

        In EMM-DEREGISTERED state, no EMM context has been established
        or the EMM context is marked as detached.
        The UE shall start the attach or combined attach procedure to
        establish an EMM context.

        The MME may answer to an attach or a combined attach procedure
        initiated by the UE. It may also answer to a tracking area
        updating procedure or combined tracking area updating procedure
        initiated by a UE if the EMM context is marked as detached.

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
 ** Name:    EmmDeregistered()                                         **
 **                                                                        **
 ** Description: Handles the behaviour of the UE and the MME while the     **
 **      EMM-SAP is in EMM-DEREGISTERED state.                     **
 **                                                                        **
 **              3GPP TS 24.301, section 5.2.2.2                           **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmDeregistered(const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  assert(emm_fsm_get_status(evt->ueid, evt->ctx) == EMM_DEREGISTERED);

  switch (evt->primitive) {

  case _EMMREG_PROC_ABORT:
    /*
     * Ongoing EMM procedure aborted
     */
    rc = RETURNok;
    break;

  case _EMMREG_COMMON_PROC_REQ:
    /*
     * An EMM common procedure has been initiated;
     * enter state EMM-COMMON-PROCEDURE-INITIATED.
     */
    rc = emm_fsm_set_status(evt->ueid, evt->ctx, EMM_COMMON_PROCEDURE_INITIATED);
    break;

  case _EMMREG_ATTACH_CNF:
    /*
     * Attach procedure successful and default EPS bearer
     * context activated;
     * enter state EMM-REGISTERED.
     */
    rc = emm_fsm_set_status(evt->ueid, evt->ctx, EMM_REGISTERED);
    break;

  case _EMMREG_LOWERLAYER_SUCCESS:
    /*
     * Data successfully delivered to the network
     */
    rc = RETURNok;
    break;

  case _EMMREG_LOWERLAYER_FAILURE:
    /*
     * Data failed to be delivered to the network
     */
    rc = RETURNok;
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

