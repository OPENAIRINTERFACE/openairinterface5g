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
int EmmDeregisteredLimitedService(nas_user_t *user, const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  assert(emm_fsm_get_status(user) == EMM_DEREGISTERED_LIMITED_SERVICE);

  switch (evt->primitive) {
  case _EMMREG_REGISTER_REQ:
    /*
     * The user manually re-selected a PLMN to register to
     */
    rc = emm_fsm_set_status(user, EMM_DEREGISTERED_PLMN_SEARCH);

    if (rc != RETURNerror) {
      /* Process the network registration request */
      rc = emm_fsm_process(user, evt);
    }

    break;

  case _EMMREG_ATTACH_INIT:
    /*
     * Initiate attach procedure for emergency bearer services
     */
    rc = emm_proc_attach(user, EMM_ATTACH_TYPE_EMERGENCY);
    break;

  case _EMMREG_ATTACH_REQ:
    /*
     * An attach for bearer emergency services has been requested
     * (Attach Request message successfully delivered to the network);
     * enter state EMM-REGISTERED-INITIATED
     */
    rc = emm_fsm_set_status(user, EMM_REGISTERED_INITIATED);
    break;

  case _EMMREG_LOWERLAYER_SUCCESS:
    /*
     * Initial NAS message has been successfully delivered
     * to the network
     */
    rc = emm_proc_lowerlayer_success(user->lowerlayer_data);
    break;

  case _EMMREG_LOWERLAYER_FAILURE:
    /*
     * Initial NAS message failed to be delivered to the network
     */
    rc = emm_proc_lowerlayer_failure(user->lowerlayer_data, TRUE);
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

