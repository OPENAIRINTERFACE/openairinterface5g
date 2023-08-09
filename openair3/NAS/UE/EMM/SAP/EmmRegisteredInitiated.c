/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

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
int EmmRegisteredInitiated(nas_user_t *user, const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;
  emm_data_t *emm_data = user->emm_data;
  user_api_id_t *user_api_id = user->user_api_id;

  assert(emm_fsm_get_status(user) == EMM_REGISTERED_INITIATED);

  switch (evt->primitive) {
  case _EMMREG_ATTACH_INIT:

    /*
     * Attach procedure has to be restarted (timers T3402 or T3411
     * expired)
     */

    /* Move to the corresponding initial EMM state */
    if (evt->u.attach.is_emergency) {
      rc = emm_fsm_set_status(user, EMM_DEREGISTERED_LIMITED_SERVICE);
    } else {
      rc = emm_fsm_set_status(user, EMM_DEREGISTERED_NORMAL_SERVICE);
    }

    if (rc != RETURNerror) {
      /* Restart the attach procedure */
      rc = emm_proc_attach_restart(user);
    }

    break;

  case _EMMREG_ATTACH_FAILED:
    /*
     * Attempt to attach to the network failed (abnormal case or
     * timer T3410 expired). The network attach procedure shall be
     * restarted when timer T3411 expires.
     */
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED_ATTEMPTING_TO_ATTACH);
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
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED_ATTEMPTING_TO_ATTACH);
    break;

  case _EMMREG_ATTACH_CNF:
    /*
     * EPS network attach accepted by the network;
     * enter state EMM-REGISTERED.
     */
    rc = emm_fsm_set_status(user, EMM_REGISTERED);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that the MT is registered
       */
      rc = emm_proc_registration_notify(user_api_id, emm_data, NET_REG_STATE_HN);

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
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED);

    if (rc != RETURNerror) {
      /*
       * Notify EMM that the MT's registration is denied
       */
      rc = emm_proc_registration_notify(user_api_id, emm_data, NET_REG_STATE_DENIED);

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
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED_PLMN_SEARCH);

    if (rc != RETURNerror) {
      /* Process the network registration request */
      rc = emm_fsm_process(user, evt);
    }

    break;

  case _EMMREG_REGISTER_REJ:
    /*
     * The UE failed to register to the network for normal EPS service
     */
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED_LIMITED_SERVICE);
    break;

  case _EMMREG_NO_IMSI:
    /*
     * The UE failed to register to the network for emergency
     * bearer services
     */
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED_NO_IMSI);
    break;

  case _EMMREG_DETACH_INIT:
    /*
     * Initiate detach procedure for EPS services
     */
    rc = emm_proc_detach(user, EMM_DETACH_TYPE_EPS, evt->u.detach.switch_off);
    break;

  case _EMMREG_DETACH_REQ:
    /*
     * An EPS network detach has been requested (Detach Request
     * message successfully delivered to the network);
     * enter state EMM-DEREGISTERED-INITIATED
     */
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED_INITIATED);
    break;

  case _EMMREG_LOWERLAYER_SUCCESS:
    /*
     * Data transfer message has been successfully delivered;
     * The NAS message may be Attach Complete, Detach Request or
     * any message transfered by EMM common procedures requested
     * by the network.
     */
    rc = emm_proc_lowerlayer_success(user->lowerlayer_data);
    break;

  case _EMMREG_LOWERLAYER_FAILURE:
    /*
     * Data transfer message failed to be delivered;
     * The NAS message may be Attach Complete, Detach Request or
     * any message transfered by EMM common procedures requested
     * by the network.
     */
    rc = emm_proc_lowerlayer_failure(user->lowerlayer_data, false);
    break;

  case _EMMREG_LOWERLAYER_RELEASE:
    /*
     * NAS signalling connection has been released before the Attach
     * Accept, Attach Reject, or any message transfered by EMM common
     * procedures requested by the network, is received.
     */
    rc = emm_proc_lowerlayer_release(user->lowerlayer_data);
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
