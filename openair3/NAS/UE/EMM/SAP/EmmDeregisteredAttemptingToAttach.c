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
Source      EmmDeregisteredAttemptingToAttach.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-DEREGISTERED.ATTEMPTING-TO-ATTACH
        state.

        In EMM-DEREGISTERED.ATTEMPTING-TO-ATTACH state, the EPS update
        status is EU2, and a previous attach was not successful.

*****************************************************************************/


#include "emm_fsm.h"
#include "commonDef.h"
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
 ** Name:    EmmDeregisteredAttemptingToAttach()                       **
 **                                                                        **
 ** Description: Handles the behaviour of the UE while the EMM-SAP is in   **
 **      EMM-DEREGISTERED.ATTEMPTING-TO-ATTACH state.              **
 **                                                                        **
 **              3GPP TS 24.301, section 5.2.2.3.3                         **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmDeregisteredAttemptingToAttach(nas_user_t *user, const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  assert(emm_fsm_get_status(user) == EMM_DEREGISTERED_ATTEMPTING_TO_ATTACH);

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

  case _EMMREG_LOWERLAYER_SUCCESS:
    /*
     * Data successfully delivered to the network
     */
    rc = emm_proc_lowerlayer_success(user->lowerlayer_data);
    break;

  case _EMMREG_LOWERLAYER_FAILURE:
    /*
     * Data failed to be delivered to the network
     */
    rc = emm_proc_lowerlayer_failure(user->lowerlayer_data, false);
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

