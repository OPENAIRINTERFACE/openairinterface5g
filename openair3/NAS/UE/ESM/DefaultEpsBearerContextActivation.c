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
Source      DefaultEpsBearerContextActivation.c

Version     0.1

Date        2013/01/28

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the default EPS bearer context activation ESM
        procedure executed by the Non-Access Stratum.

        The purpose of the default bearer context activation procedure
        is to establish a default EPS bearer context between the UE
        and the EPC.

        The procedure is initiated by the network as a response to
        the PDN CONNECTIVITY REQUEST message received from the UE.
        It can be part of the attach procedure.

*****************************************************************************/

#include "esm_proc.h"
#include "commonDef.h"
#include "nas_log.h"

#include "esm_cause.h"
#include "esm_ebr.h"
#include "esm_ebr_context.h"

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


/*
 * --------------------------------------------------------------------------
 *    Default EPS bearer context activation procedure executed by the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_default_eps_bearer_context_request()             **
 **                                                                        **
 ** Description: Creates local default EPS bearer context upon receipt of  **
 **      the ACTIVATE DEFAULT EPS BEARER CONTEXT REQUEST message.  **
 **                                                                        **
 ** Inputs:  pid:       PDN connection identifier                  **
 **      ebi:       EPS bearer identity                        **
 **      qos:       EPS bearer level QoS parameters            **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
int esm_proc_default_eps_bearer_context_request(nas_user_t *user, int pid, int ebi,
    const esm_proc_qos_t *qos,
    int *esm_cause)
{
  LOG_FUNC_IN;
  esm_data_t *esm_data = user->esm_data;
  default_eps_bearer_context_data_t *default_eps_bearer_context_data = user->default_eps_bearer_context_data;
  int rc = RETURNerror;

  LOG_TRACE(INFO, "ESM-PROC  - Default EPS bearer context activation "
            "requested by the network (ebi=%d)", ebi);

  /* Assign default EPS bearer context */
  int new_ebi = esm_ebr_assign(user->esm_ebr_data, ebi, pid+1, true);

  if (new_ebi == ESM_EBI_UNASSIGNED) {
    /* 3GPP TS 24.301, section 6.4.1.5, abnormal cases a and b
     * Default EPS bearer context activation request for an already
     * activated default or dedicated EPS bearer context
     */
    int old_pid, old_bid;
    /* Locally deactivate the existing EPS bearer context and proceed
     * with the requested default EPS bearer context activation */
    rc = esm_proc_eps_bearer_context_deactivate(user, true, ebi,
         &old_pid, &old_bid);

    if (rc != RETURNok) {
      /* Failed to release EPS bearer context */
      *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    } else {
      /* Assign new default EPS bearer context */
      ebi = esm_ebr_assign(user->esm_ebr_data, ebi, pid+1, true);
    }
  }

  if (ebi != ESM_EBI_UNASSIGNED) {
    /* Create new default EPS bearer context */
    ebi = esm_ebr_context_create(esm_data, user->ueid, pid, ebi, true, qos, NULL);

    if (ebi != ESM_EBI_UNASSIGNED) {
      /* Default EPS bearer contextx successfully created */
      default_eps_bearer_context_data->ebi = ebi;
      rc = RETURNok;
    } else {
      /* No resource available */
      LOG_TRACE(WARNING, "ESM-PROC  - Failed to create new default EPS "
                "bearer context");
      *esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_default_eps_bearer_context_accept()              **
 **                                                                        **
 ** Description: Performs default EPS bearer context activation procedure  **
 **      accepted by the UE.                                       **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.1.3                           **
 **      The UE accepts default EPS bearer context activation by   **
 **      sending ACTIVATE DEFAULT EPS BEARER CONTEXT ACCEPT mes-   **
 **      sage and entering the state BEARER CONTEXT ACTIVE.        **
 **      If the default bearer is activated as part of the attach  **
 **      procedure, the UE shall send the accept message together  **
 **      with ATTACH COMPLETE message.                             **
 **                                                                        **
 ** Inputs:  is_standalone: Indicates whether the activate default EPS **
 **             bearer context accept has to be sent stand-**
 **             alone or together within an attach comple- **
 **             te message                                 **
 **      ebi:       EPS bearer identity                        **
 **      msg:       Encoded ESM message to be sent             **
 **      ue_triggered:  true if the EPS bearer context procedure   **
 **             was triggered by the UE (should be always  **
 **             true)                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_default_eps_bearer_context_accept(nas_user_t *user, bool is_standalone, int ebi,
    OctetString *msg, bool ue_triggered)
{
  LOG_FUNC_IN;

  int rc = RETURNok;
  esm_ebr_data_t *esm_ebr_data = user->esm_ebr_data;
  user_api_id_t *user_api_id = user->user_api_id;

  LOG_TRACE(INFO,"ESM-PROC  - Default EPS bearer context activation "
            "accepted by the UE (ebi=%d)", ebi);

  if (is_standalone) {
    emm_sap_t emm_sap;
    emm_esm_data_t *emm_esm = &emm_sap.u.emm_esm.u.data;
    /*
     * Notity EMM that ESM PDU has to be forwarded to lower layers
     */
    emm_sap.primitive = EMMESM_UNITDATA_REQ;
    emm_sap.u.emm_esm.ueid = user->ueid;
    emm_esm->msg.length = msg->length;
    emm_esm->msg.value = msg->value;
    rc = emm_sap_send(user, &emm_sap);
  }

  if (rc != RETURNerror) {
    /* Set the EPS bearer context state to ACTIVE */
    rc = esm_ebr_set_status(user_api_id, esm_ebr_data, ebi, ESM_EBR_ACTIVE, ue_triggered);

    if (rc != RETURNok) {
      /* The EPS bearer context was already in ACTIVE state */
      LOG_TRACE(WARNING, "ESM-PROC  - EBI %d was already ACTIVE", ebi);
      /* Accept network retransmission of already accepted activate
       * default EPS bearer context request */
      LOG_FUNC_RETURN (RETURNok);
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_default_eps_bearer_context_reject()              **
 **                                                                        **
 ** Description: Performs default EPS bearer context activation procedure  **
 **      not accepted by the UE.                                   **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.1.4                           **
 **      The UE rejects default EPS bearer context activation by   **
 **      sending ACTIVATE DEFAULT EPS BEARER CONTEXT REJECT mes-   **
 **      sage and entering the state BEARER CONTEXT INACTIVE.      **
 **      If the default EPS bearer context activation is part of   **
 **      the attach procedure, the ESM sublayer shall notify the   **
 **      EMM sublayer of an ESM failure.                           **
 **                                                                        **
 ** Inputs:  is_standalone: Indicates whether the activate default EPS **
 **             bearer context accept has to be sent stand-**
 **             alone or together within an attach comple- **
 **             te message                                 **
 **      ebi:       EPS bearer identity                        **
 **      msg:       Encoded ESM message to be sent             **
 **      ue_triggered:  Not used                                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_default_eps_bearer_context_reject(nas_user_t *user, bool is_standalone, int ebi,
    OctetString *msg, bool ue_triggered)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(WARNING, "ESM-PROC  - Default EPS bearer context activation "
            "not accepted by the UE (ebi=%d)", ebi);

  if ( !esm_ebr_is_not_in_use(user->esm_ebr_data, ebi) ) {
    /* Release EPS bearer data currently in use */
    rc = esm_ebr_release(user->esm_ebr_data, ebi);
  }

  if (rc != RETURNok) {
    LOG_TRACE(WARNING, "ESM-PROC  - Failed to release EPS bearer data");
  } else if (is_standalone) {
    emm_sap_t emm_sap;
    emm_esm_data_t *emm_esm = &emm_sap.u.emm_esm.u.data;
    /*
     * Notity EMM that ESM PDU has to be forwarded to lower layers
     */
    emm_sap.primitive = EMMESM_UNITDATA_REQ;
    emm_sap.u.emm_esm.ueid = user->ueid;
    emm_esm->msg.length = msg->length;
    emm_esm->msg.value = msg->value;
    rc = emm_sap_send(user, &emm_sap);
  } else {
    /* An error is returned to notify EMM that the default EPS bearer
     * activation procedure initiated as part of the initial attach
     * procedure has failed
     */
    rc = RETURNerror;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_default_eps_bearer_context_complete()            **
 **                                                                        **
 ** Description: Terminates the default EPS bearer context activation pro- **
 **      cedure upon receiving indication from the EPS Mobility    **
 **      Management sublayer that the ACTIVATE DEFAULT EPS BEARER  **
 **      CONTEXT ACCEPT message has been successfully delivered to **
 **      the MME.                                                  **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
int esm_proc_default_eps_bearer_context_complete(default_eps_bearer_context_data_t *default_eps_bearer_context_data)
{
  LOG_FUNC_IN;

  LOG_TRACE(INFO,
            "ESM-PROC  - Default EPS bearer context activation complete");

  /* Reset default EPS bearer context internal data */
  default_eps_bearer_context_data->ebi = ESM_EBI_UNASSIGNED;

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_default_eps_bearer_context_failure()             **
 **                                                                        **
 ** Description: Performs default EPS bearer context activation procedure  **
 **      upon receiving transmission failure of ESM message indi-  **
 **      cation from the EPS Mobility Management sublayer          **
 **                                                                        **
 **      The UE releases the default EPS bearer context previously **
 **      allocated before the ACTIVATE DEFAULT EPS BEARER CONTEXT  **
 **      ACCEPT message was sent.                                  **
 **                                                                        **
 ** Inputs:  None                                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
int esm_proc_default_eps_bearer_context_failure(nas_user_t *user)
{
  LOG_FUNC_IN;
  default_eps_bearer_context_data_t *default_eps_bearer_context_data = user->default_eps_bearer_context_data;

  int ebi = default_eps_bearer_context_data->ebi;
  int pid, bid;

  LOG_TRACE(WARNING,
            "ESM-PROC  - Default EPS bearer context activation failure");

  /* Release the default EPS bearer context and enter state INACTIVE */
  int rc = esm_proc_eps_bearer_context_deactivate(user, true, ebi, &pid, &bid);

  if (rc != RETURNerror) {
    /* Reset default EPS bearer context internal data */
    default_eps_bearer_context_data->ebi = ESM_EBI_UNASSIGNED;
  }

  LOG_FUNC_RETURN (rc);
}
