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
Source      EmmDeregisteredLimitedService.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-DEREGISTERED.LIMITED-SERVICE state.

        In EMM-DEREGISTERED.LIMITED-SERVICE state, the EPS update
        status is EU3, and it is known that a selected cell is unable
        to provide normal service.

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
 ** Name:    EmmDeregisteredLimitedService()                           **
 **                                                                        **
 ** Description: Handles the behaviour of the UE while the EMM-SAP is in   **
 **      EMM-DEREGISTERED.LIMITED-SERVICE state.                   **
 **                                                                        **
 **              3GPP TS 24.301, section 5.2.2.3.2                         **
 **      The UE shall initiate an attach or combined attach proce- **
 **      dure when entering a cell which provides normal service.  **
 **      It may initiate attach for emergency bearer services.     **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmDeregisteredLimitedService(const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  assert(emm_fsm_get_status() == EMM_DEREGISTERED_LIMITED_SERVICE);

  switch (evt->primitive) {
  case _EMMREG_REGISTER_REQ:
    /*
     * The user manually re-selected a PLMN to register to
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED_PLMN_SEARCH);

    if (rc != RETURNerror) {
      /* Process the network registration request */
      rc = emm_fsm_process(evt);
    }

    break;

  case _EMMREG_ATTACH_INIT:
    /*
     * Initiate attach procedure for emergency bearer services
     */
    rc = emm_proc_attach(EMM_ATTACH_TYPE_EMERGENCY);
    break;

  case _EMMREG_ATTACH_REQ:
    /*
     * An attach for bearer emergency services has been requested
     * (Attach Request message successfully delivered to the network);
     * enter state EMM-REGISTERED-INITIATED
     */
    rc = emm_fsm_set_status(EMM_REGISTERED_INITIATED);
    break;

  case _EMMREG_LOWERLAYER_SUCCESS:
    /*
     * Initial NAS message has been successfully delivered
     * to the network
     */
    rc = emm_proc_lowerlayer_success();
    break;

  case _EMMREG_LOWERLAYER_FAILURE:
    /*
     * Initial NAS message failed to be delivered to the network
     */
    rc = emm_proc_lowerlayer_failure(TRUE);
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

