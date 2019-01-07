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
#include "user_defs.h"

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
int EmmDeregistered(nas_user_t *user, const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  assert(emm_fsm_get_status(user) == EMM_DEREGISTERED);


  /* Delete the authentication data RAND and RES */
  rc = emm_proc_authentication_delete(user);

  if (rc != RETURNok) {
    LOG_FUNC_RETURN (rc);
  }

  /* TODO: 3GPP TS 24.301, section 4.4.2.1
   * The UE shall store the current native EPS security context as specified
   * in annex C and mark it as valid only when the UE enters state EMM-
   * DEREGISTERED from any other state except EMM-NULL or when the UE aborts
   * the attach procedure without having left EMM-DEREGISTERED.
   */

  switch (evt->primitive) {


  case _EMMREG_NO_IMSI:
    /*
     * The UE was powered on without a valid USIM application present
     */
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED_NO_IMSI);
    break;

  case _EMMREG_REGISTER_REQ:
    /*
     * The default EMM primary substate when the UE is switched on
     * with valid USIM application shall be PLMN-SEARCH
     */
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED_PLMN_SEARCH);

    if (rc != RETURNerror) {
      /* Process the network registration request */
      rc = emm_fsm_process(user, evt);
    }

    break;

  case _EMMREG_ATTACH_INIT:

    /*
     * Attach procedure has to be restarted (timers T3402 or T3411
     * expired) while the UE locally detached from the network
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

  /* TODO: 3GPP TS 24.301, section 4.4.2.1
   * The UE shall mark the EPS security context on the USIM or in the non-
   * volatile memory as invalid when the UE initiates an attach procedure
   * or when the UE leaves state EMM-DEREGISTERED for any other state except
   * EMM-NULL.
   */

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

