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
int EmmRegistered(nas_user_t *user, const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  assert(emm_fsm_get_status(user) == EMM_REGISTERED);


  switch (evt->primitive) {

  case _EMMREG_DETACH_INIT:
    /*
     * Initiate detach procedure for EPS services
     */
    rc = emm_proc_detach(user, EMM_DETACH_TYPE_EPS, evt->u.detach.switch_off);
    break;

  case _EMMREG_DETACH_REQ:
    /*
     * Network detach has been requested (Detach Request
     * message successfully delivered to the network);
     * enter state EMM-DEREGISTERED-INITIATED
     */
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED_INITIATED);
    break;

  case _EMMREG_DETACH_CNF:
    /*
     * The UE implicitly detached from the network (all EPS
     * bearer contexts may have been deactivated)
     */
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED);
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
    rc = emm_proc_lowerlayer_success(user->lowerlayer_data);
    break;

  case _EMMREG_LOWERLAYER_FAILURE:
    /*
     * Data transfer message failed to be delivered
     */
    rc = emm_proc_lowerlayer_failure(user->lowerlayer_data, false);
    break;

  case _EMMREG_LOWERLAYER_RELEASE:
    /*
     * NAS signalling connection has been released
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

