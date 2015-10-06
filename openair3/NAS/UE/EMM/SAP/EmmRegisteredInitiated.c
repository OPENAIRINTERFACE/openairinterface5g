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
Source      EmmRegisteredInitiated.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-REGISTERED-INITIATED state.

        In EMM-REGISTERED-INITIATED state, the attach or the combined
        attach procedure has been started and the UE is waiting for a
        response from the MME.

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
 ** Name:    EmmRegisteredInitiated()                                  **
 **                                                                        **
 ** Description: Handles the behaviour of the UE while the EMM-SAP is in   **
 **      EMM-REGISTERED-INITIATED state.                           **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmRegisteredInitiated(const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  assert(emm_fsm_get_status() == EMM_REGISTERED_INITIATED);

  switch (evt->primitive) {
  case _EMMREG_ATTACH_INIT:

    /*
     * Attach procedure has to be restarted (timers T3402 or T3411
     * expired)
     */

    /* Move to the corresponding initial EMM state */
    if (evt->u.attach.is_emergency) {
      rc = emm_fsm_set_status(EMM_DEREGISTERED_LIMITED_SERVICE);
    } else {
      rc = emm_fsm_set_status(EMM_DEREGISTERED_NORMAL_SERVICE);
    }

    if (rc != RETURNerror) {
      /* Restart the attach procedure */
      rc = emm_proc_attach_restart();
    }

    break;

  case _EMMREG_ATTACH_FAILED:
    /*
     * Attempt to attach to the network failed (abnormal case or
     * timer T3410 expired). The network attach procedure shall be
     * restarted when timer T3411 expires.
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED_ATTEMPTING_TO_ATTACH);
    break;

  case _EMMREG_ATTACH_EXCEEDED:
    /*
     * Attempt to attach to the network failed (abnormal case or
     * timer T3410 expired) and the attach attempt counter reached
     * its maximum value. The state is changed to EMM-DEREGISTERED.
     * ATTEMPTING-TO-ATTACH or optionally to EMM-DEREGISTERED.PLMN-
     * SEARCH in order to perform a PLMN selection.
     */
    /* TODO: ATTEMPTING-TO-ATTACH or PLMN-SEARCH ??? */
    rc = emm_fsm_set_status(EMM_DEREGISTERED_ATTEMPTING_TO_ATTACH);
    break;

  case _EMMREG_ATTACH_CNF:
    /*
     * EPS network attach accepted by the network;
     * enter state EMM-REGISTERED.
     */
    rc = emm_fsm_set_status(EMM_REGISTERED);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that the MT is registered
       */
      rc = emm_proc_registration_notify(NET_REG_STATE_HN);

      if (rc != RETURNok) {
        LOG_TRACE(WARNING, "EMM-FSM   - "
                  "Failed to notify registration update");
      }
    }

    break;

  case _EMMREG_AUTH_REJ:

    /*
     * UE authentication rejected by the network;
     * abort any EMM signalling procedure
     */
  case _EMMREG_ATTACH_REJ:
    /*
     * EPS network attach rejected by the network;
     * enter state EMM-DEREGISTERED.
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that the MT's registration is denied
       */
      rc = emm_proc_registration_notify(NET_REG_STATE_DENIED);

      if (rc != RETURNok) {
        LOG_TRACE(WARNING, "EMM-FSM   - "
                  "Failed to notify registration update");
      }
    }

    break;

  case _EMMREG_REGISTER_REQ:
    /*
     * The UE has to select a new PLMN to register to
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED_PLMN_SEARCH);

    if (rc != RETURNerror) {
      /* Process the network registration request */
      rc = emm_fsm_process(evt);
    }

    break;

  case _EMMREG_REGISTER_REJ:
    /*
     * The UE failed to register to the network for normal EPS service
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED_LIMITED_SERVICE);
    break;

  case _EMMREG_NO_IMSI:
    /*
     * The UE failed to register to the network for emergency
     * bearer services
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED_NO_IMSI);
    break;

  case _EMMREG_DETACH_INIT:
    /*
     * Initiate detach procedure for EPS services
     */
    rc = emm_proc_detach(EMM_DETACH_TYPE_EPS, evt->u.detach.switch_off);
    break;

  case _EMMREG_DETACH_REQ:
    /*
     * An EPS network detach has been requested (Detach Request
     * message successfully delivered to the network);
     * enter state EMM-DEREGISTERED-INITIATED
     */
    rc = emm_fsm_set_status(EMM_DEREGISTERED_INITIATED);
    break;

  case _EMMREG_LOWERLAYER_SUCCESS:
    /*
     * Data transfer message has been successfully delivered;
     * The NAS message may be Attach Complete, Detach Request or
     * any message transfered by EMM common procedures requested
     * by the network.
     */
    rc = emm_proc_lowerlayer_success();
    break;

  case _EMMREG_LOWERLAYER_FAILURE:
    /*
     * Data transfer message failed to be delivered;
     * The NAS message may be Attach Complete, Detach Request or
     * any message transfered by EMM common procedures requested
     * by the network.
     */
    rc = emm_proc_lowerlayer_failure(FALSE);
    break;

  case _EMMREG_LOWERLAYER_RELEASE:
    /*
     * NAS signalling connection has been released before the Attach
     * Accept, Attach Reject, or any message transfered by EMM common
     * procedures requested by the network, is received.
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
