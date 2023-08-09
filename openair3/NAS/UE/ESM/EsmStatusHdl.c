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
Source      EsmStatus.c

Version     0.1

Date        2013/06/17

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the ESM status procedure executed by the Non-Access
        Stratum.

        ESM status procedure can be related to an EPS bearer context
        or to a procedure transaction.

        The purpose of the sending of the ESM STATUS message is to
        report at any time certain error conditions detected upon
        receipt of ESM protocol data. The ESM STATUS message can be
        sent by both the MME and the UE.

*****************************************************************************/

#include "esm_proc.h"
#include "commonDef.h"
#include "nas_log.h"

#include "esm_cause.h"

#include "emm_sap.h"

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
 ** Name:    esm_proc_status_ind()                                     **
 **                                                                        **
 ** Description: Processes received ESM status message.                    **
 **                                                                        **
 **      3GPP TS 24.301, section 6.7                               **
 **      Upon receiving ESM Status message the UE/MME shall take   **
 **      different actions depending on the received ESM cause     **
 **      value.                                                    **
 **                                                                        **
 ** Inputs:  ueid:      UE lower layer identifier                  **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      esm_cause: Received ESM cause code                    **
 **             failure                                    **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_status_ind(
  int pti, int ebi, int *esm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO,"ESM-PROC  - ESM status procedure requested (cause=%d)",
            *esm_cause);

  LOG_TRACE(DEBUG, "ESM-PROC  - To be implemented");

  switch (*esm_cause) {
  case ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY:
    /*
     * Abort any ongoing ESM procedure related to the received EPS
     * bearer identity, stop any related timer, and deactivate the
     * corresponding EPS bearer context locally
     */
    /* TODO */
    rc = RETURNok;
    break;

  case ESM_CAUSE_INVALID_PTI_VALUE:
    /*
     * Abort any ongoing ESM procedure related to the received PTI
     * value and stop any related timer
     */
    /* TODO */
    rc = RETURNok;
    break;

  case ESM_CAUSE_MESSAGE_TYPE_NOT_IMPLEMENTED:
    /*
     * Abort any ongoing ESM procedure related to the PTI or
     * EPS bearer identity and stop any related timer
     */
    /* TODO */
    rc = RETURNok;
    break;

  default:
    /*
     * No state transition and no specific action shall be taken;
     * local actions are possible
     */
    /* TODO */
    rc = RETURNok;
    break;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_status()                                         **
 **                                                                        **
 ** Description: Initiates ESM status procedure.                           **
 **                                                                        **
 ** Inputs:  is_standalone: Not used - Always true                     **
 **      ueid:      UE lower layer identifier                  **
 **      ebi:       Not used                                   **
 **      msg:       Encoded ESM status message to be sent      **
 **      ue_triggered:  Not used                                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_status(nas_user_t *user, bool is_standalone,
                    int ebi, OctetString *msg,
                    bool ue_triggered)
{
  LOG_FUNC_IN;

  int rc;
  emm_sap_t emm_sap;

  LOG_TRACE(INFO,"ESM-PROC  - ESM status procedure requested");

  /*
   * Notity EMM that ESM PDU has to be forwarded to lower layers
   */
  emm_sap.primitive = EMMESM_UNITDATA_REQ;
  emm_sap.u.emm_esm.ueid = user->ueid;
  emm_sap.u.emm_esm.u.data.msg.length = msg->length;
  emm_sap.u.emm_esm.u.data.msg.value = msg->value;
  rc = emm_sap_send(user, &emm_sap);

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
