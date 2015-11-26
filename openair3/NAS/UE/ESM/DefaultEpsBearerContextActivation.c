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

/*
 * --------------------------------------------------------------------------
 * Internal data handled by the default EPS bearer context activation
 * procedure in the UE
 * --------------------------------------------------------------------------
 */
static struct {
  int ebi;    /* EPS bearer identity of the default EPS bearer associated
         * to the PDN connection to be activated */
} _default_eps_bearer_context_data = {ESM_EBI_UNASSIGNED};



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
 **      Others:    _default_eps_bearer_context_data           **
 **                                                                        **
 ***************************************************************************/
int esm_proc_default_eps_bearer_context_request(int pid, int ebi,
    const esm_proc_qos_t *qos,
    int *esm_cause)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  LOG_TRACE(INFO, "ESM-PROC  - Default EPS bearer context activation "
            "requested by the network (ebi=%d)", ebi);

  /* Assign default EPS bearer context */
  int new_ebi = esm_ebr_assign(ebi, pid+1, TRUE);

  if (new_ebi == ESM_EBI_UNASSIGNED) {
    /* 3GPP TS 24.301, section 6.4.1.5, abnormal cases a and b
     * Default EPS bearer context activation request for an already
     * activated default or dedicated EPS bearer context
     */
    int old_pid, old_bid;
    /* Locally deactivate the existing EPS bearer context and proceed
     * with the requested default EPS bearer context activation */
    rc = esm_proc_eps_bearer_context_deactivate(TRUE, ebi,
         &old_pid, &old_bid);

    if (rc != RETURNok) {
      /* Failed to release EPS bearer context */
      *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    } else {
      /* Assign new default EPS bearer context */
      ebi = esm_ebr_assign(ebi, pid+1, TRUE);
    }
  }

  if (ebi != ESM_EBI_UNASSIGNED) {
    /* Create new default EPS bearer context */
    ebi = esm_ebr_context_create(pid, ebi, TRUE, qos, NULL);

    if (ebi != ESM_EBI_UNASSIGNED) {
      /* Default EPS bearer contextx successfully created */
      _default_eps_bearer_context_data.ebi = ebi;
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
 **      ue_triggered:  TRUE if the EPS bearer context procedure   **
 **             was triggered by the UE (should be always  **
 **             TRUE)                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_default_eps_bearer_context_accept(int is_standalone, int ebi,
    OctetString *msg, int ue_triggered)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO,"ESM-PROC  - Default EPS bearer context activation "
            "accepted by the UE (ebi=%d)", ebi);

  if (is_standalone) {
    emm_sap_t emm_sap;
    emm_esm_data_t *emm_esm = &emm_sap.u.emm_esm.u.data;
    /*
     * Notity EMM that ESM PDU has to be forwarded to lower layers
     */
    emm_sap.primitive = EMMESM_UNITDATA_REQ;
    emm_sap.u.emm_esm.ueid = 0;
    emm_esm->msg.length = msg->length;
    emm_esm->msg.value = msg->value;
    rc = emm_sap_send(&emm_sap);
  }

  if (rc != RETURNerror) {
    /* Set the EPS bearer context state to ACTIVE */
    rc = esm_ebr_set_status(ebi, ESM_EBR_ACTIVE, ue_triggered);

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
int esm_proc_default_eps_bearer_context_reject(int is_standalone, int ebi,
    OctetString *msg, int ue_triggered)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(WARNING, "ESM-PROC  - Default EPS bearer context activation "
            "not accepted by the UE (ebi=%d)", ebi);

  if ( !esm_ebr_is_not_in_use(ebi) ) {
    /* Release EPS bearer data currently in use */
    rc = esm_ebr_release(ebi);
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
    emm_sap.u.emm_esm.ueid = 0;
    emm_esm->msg.length = msg->length;
    emm_esm->msg.value = msg->value;
    rc = emm_sap_send(&emm_sap);
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
 **      Others:    _default_eps_bearer_context_data           **
 **                                                                        **
 ***************************************************************************/
int esm_proc_default_eps_bearer_context_complete(void)
{
  LOG_FUNC_IN;

  LOG_TRACE(INFO,
            "ESM-PROC  - Default EPS bearer context activation complete");

  /* Reset default EPS bearer context internal data */
  _default_eps_bearer_context_data.ebi = ESM_EBI_UNASSIGNED;

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
 **      Others:    _default_eps_bearer_context_data           **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _default_eps_bearer_context_data           **
 **                                                                        **
 ***************************************************************************/
int esm_proc_default_eps_bearer_context_failure(void)
{
  LOG_FUNC_IN;

  int ebi = _default_eps_bearer_context_data.ebi;
  int pid, bid;

  LOG_TRACE(WARNING,
            "ESM-PROC  - Default EPS bearer context activation failure");

  /* Release the default EPS bearer context and enter state INACTIVE */
  int rc = esm_proc_eps_bearer_context_deactivate(TRUE, ebi, &pid, &bid);

  if (rc != RETURNerror) {
    /* Reset default EPS bearer context internal data */
    _default_eps_bearer_context_data.ebi = ESM_EBI_UNASSIGNED;
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *              Timer handlers
 * --------------------------------------------------------------------------
 */
