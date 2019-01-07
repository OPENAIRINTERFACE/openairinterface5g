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
Source      EmmDeregisteredInitiated.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-DEREGISTERED-INITIATED state.

        In EMM-DEREGISTERED-INITIATED state, the UE has requested
        release of the EMM context by starting the detach or combined
        detach procedure and is waiting for a response from the MME.
        The MME has started a detach procedure and is waiting for a
        response from the UE.

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
 ** Name:    EmmDeregisteredInitiated()                                **
 **                                                                        **
 ** Description: Handles the behaviour of the UE and the MME while the     **
 **      EMM-SAP is in EMM-DEREGISTERED-INITIATED state.           **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmDeregisteredInitiated(nas_user_t *user, const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  assert(emm_fsm_get_status(user) == EMM_DEREGISTERED_INITIATED);

  switch (evt->primitive) {

  case _EMMREG_DETACH_CNF:
    /*
     * The UE explicitly detached from the network (all EPS
     * bearer contexts have been deactivated as UE initiated
     * detach procedure successfully completed)
     */
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED);
    break;

  case _EMMREG_DETACH_FAILED:

    /*
     * The detach procedure failed
     */
    if (evt->u.detach.type == EMM_DETACH_TYPE_IMSI) {
      rc = emm_fsm_set_status(user, EMM_REGISTERED_NORMAL_SERVICE);
    } else {
      rc = emm_fsm_set_status(user, EMM_DEREGISTERED);
    }

    break;

  case _EMMREG_LOWERLAYER_SUCCESS:
    /*
     * Ignore Detach Request message successful retransmission
     */
    rc = RETURNok;
    break;

  case _EMMREG_LOWERLAYER_FAILURE:
  case _EMMREG_LOWERLAYER_RELEASE:
    /*
     * Lower layer failure or release of the NAS signalling connection
     * before the Detach Accept is received
     */
    // FIXME review
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

