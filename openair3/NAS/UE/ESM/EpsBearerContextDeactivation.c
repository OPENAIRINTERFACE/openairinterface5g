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
Source      EpsBearerContextDeactivation.c

Version     0.1

Date        2013/05/22

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the EPS bearer context deactivation ESM procedure
        executed by the Non-Access Stratum.

        The purpose of the EPS bearer context deactivation procedure
        is to deactivate an EPS bearer context or disconnect from a
        PDN by deactivating all EPS bearer contexts to the PDN.
        The EPS bearer context deactivation procedure is initiated
        by the network, and it may be triggered by the UE by means
        of the UE requested bearer resource modification procedure
        or UE requested PDN disconnect procedure.

*****************************************************************************/

#include "esm_proc.h"
#include "commonDef.h"
#include "nas_log.h"

#include "emmData.h"
#include "esmData.h"
#include "esm_cause.h"
#include "esm_ebr.h"
#include "esm_ebr_context.h"

#include "esm_main.h"

#include "emm_sap.h"
#include "esm_sap.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 * Internal data handled by the EPS bearer context deactivation procedure
 * in the UE
 * --------------------------------------------------------------------------
 */
static int _eps_bearer_release(int ebi, int *pid, int *bid);


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/*
 * --------------------------------------------------------------------------
 *  EPS bearer context deactivation procedure executed by the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_eps_bearer_context_deactivate()                  **
 **                                                                        **
 ** Description: Locally releases the EPS bearer context identified by the **
 **      given EPS bearer identity, without peer-to-peer signal-   **
 **      ling between the UE and the MME, or checks whether the UE **
 **      has an EPS bearer context with specified EPS bearer iden- **
 **      tity activated.                                           **
 **                                                                        **
 ** Inputs:  is local:  TRUE if the EPS bearer context has to be   **
 **             locally released without peer-to-peer si-  **
 **             gnalling between the UE and the MME        **
 **      ebi:       EPS bearer identity of the EPS bearer con- **
 **             text to be deactivated                     **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     pid:       Identifier of the PDN connection the EPS   **
 **             bearer belongs to                          **
 **      bid:       Identifier of the released EPS bearer con- **
 **             text entry                                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_eps_bearer_context_deactivate(int is_local, int ebi,
    int *pid, int *bid)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;
  int i;

  if (is_local) {
    if (ebi != ESM_SAP_ALL_EBI) {
      /* Locally release the EPS bearer context */
      rc = _eps_bearer_release(ebi, pid, bid);
    } else {
      /* Locally release all the EPS bearer contexts */
      *bid = 0;

      for (*pid = 0; *pid < ESM_DATA_PDN_MAX; (*pid)++) {
        if (_esm_data.pdn[*pid].data) {
          rc = _eps_bearer_release(ESM_EBI_UNASSIGNED, pid, bid);

          if (rc != RETURNok) {
            break;
          }
        }
      }
    }

    LOG_FUNC_RETURN (rc);
  }

  LOG_TRACE(WARNING, "ESM-PROC  - EPS bearer context deactivation (ebi=%d)",
            ebi);

  if (*pid < ESM_DATA_PDN_MAX) {
    if (_esm_data.pdn[*pid].pid != *pid) {
      LOG_TRACE(ERROR, "ESM-PROC  - PDN connection identifier %d "
                "is not valid", *pid);
    } else if (_esm_data.pdn[*pid].data == NULL) {
      LOG_TRACE(ERROR, "ESM-PROC  - PDN connection %d has not been "
                "allocated", *pid);
    } else if (!_esm_data.pdn[*pid].is_active) {
      LOG_TRACE(WARNING, "ESM-PROC  - PDN connection %d is not active",
                *pid);
    } else {
      esm_pdn_t *pdn = _esm_data.pdn[*pid].data;

      for (i = 0; i < pdn->n_bearers; i++) {
        if (pdn->bearer[i]->ebi != ebi) {
          continue;
        }

        /* The EPS bearer context to be released is valid */
        rc = RETURNok;
      }
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_eps_bearer_context_deactivate_request()          **
 **                                                                        **
 ** Description: Deletes the EPS bearer context identified by the EPS bea- **
 **      rer identity upon receipt of the DEACTIVATE EPS BEARER    **
 **      CONTEXT REQUEST message.                                  **
 **                                                                        **
 ** Inputs:  ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_eps_bearer_context_deactivate_request(int ebi, int *esm_cause)
{
  LOG_FUNC_IN;

  int pid, bid;
  int rc = RETURNok;

  LOG_TRACE(INFO, "ESM-PROC  - EPS bearer context deactivation "
            "requested by the network (ebi=%d)", ebi);

  /* Release the EPS bearer context entry */
  if (esm_ebr_context_release(ebi, &pid, &bid) == ESM_EBI_UNASSIGNED) {
    LOG_TRACE(WARNING, "ESM-PROC  - Failed to release EPS bearer context");
    *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    LOG_FUNC_RETURN (RETURNerror);
  }

  if (bid == 0) {
    /* The EPS bearer identity is that of the default bearer assigned to
     * the PDN connection */
    if (*esm_cause == ESM_CAUSE_REACTIVATION_REQUESTED) {
      esm_sap_t esm_sap;
      int active = FALSE;

      /* 3GPP TS 24.301, section 6.4.4.3
       * The UE should re-initiate the UE requested PDN connectivity
       * procedure for the APN associated to the PDN it was connected
       * to in order to reactivate the EPS bearer context
       */
      LOG_TRACE(WARNING, "ESM-PROC  - The network requests PDN "
                "connection reactivation");

      /* Get PDN context parameters */
      rc = esm_main_get_pdn(pid + 1, &esm_sap.data.pdn_connect.pdn_type,
                            &esm_sap.data.pdn_connect.apn,
                            &esm_sap.data.pdn_connect.is_emergency,
                            &active);

      if (rc != RETURNerror) {
        if (active) {
          LOG_TRACE(ERROR, "ESM-PROC  - Connectivity to APN %s "
                    "has not been deactivated",
                    esm_sap.data.pdn_connect.apn);
          *esm_cause = ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED;
          LOG_FUNC_RETURN (RETURNerror);
        }

        /*
         * Notify ESM to re-initiate PDN connectivity procedure
         */
        esm_sap.primitive = ESM_PDN_CONNECTIVITY_REQ;
        esm_sap.is_standalone = TRUE;
        esm_sap.data.pdn_connect.is_defined = TRUE;
        esm_sap.data.pdn_connect.cid = pid + 1;
        rc = esm_sap_send(&esm_sap);
      }
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_eps_bearer_context_deactivate_accept()           **
 **                                                                        **
 ** Description: Performs EPS bearer context deactivation procedure accep- **
 **      ted by the UE.                                            **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.4.3                           **
 **      The UE accepts EPS bearer context deactivation by sending **
 **      DEACTIVATE EPS BEARER CONTEXT ACCEPT message and entering **
 **      the state BEARER CONTEXT INACTIVE.                        **
 **                                                                        **
 ** Inputs:  is_standalone: Should be always TRUE                      **
 **      ebi:       EPS bearer identity                        **
 **      msg:       Encoded ESM message to be sent             **
 **      ue_triggered:  TRUE if the EPS bearer context procedure   **
 **             was triggered by the UE                    **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_eps_bearer_context_deactivate_accept(int is_standalone, int ebi,
    OctetString *msg, int ue_triggered)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO,"ESM-PROC  - EPS bearer context deactivation accepted");

  if (is_standalone) {
    emm_sap_t emm_sap;
    /*
     * Notity EMM that ESM PDU has to be forwarded to lower layers
     */
    emm_sap.primitive = EMMESM_UNITDATA_REQ;
    emm_sap.u.emm_esm.ueid = 0;
    emm_sap.u.emm_esm.u.data.msg.length = msg->length;
    emm_sap.u.emm_esm.u.data.msg.value = msg->value;
    rc = emm_sap_send(&emm_sap);
  }

  if (rc != RETURNerror) {
    /* Set the EPS bearer context state to INACTIVE */
    rc = esm_ebr_set_status(ebi, ESM_EBR_INACTIVE, ue_triggered);

    if (rc != RETURNok) {
      /* The EPS bearer context was already in INACTIVE state */
      LOG_TRACE(WARNING, "ESM-PROC  - EBI %d was already INACTIVE", ebi);
      /* Accept network retransmission of already accepted deactivate
       * EPS bearer context request */
      LOG_FUNC_RETURN (RETURNok);
    }

    /* Release EPS bearer data */
    rc = esm_ebr_release(ebi);
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



/*
 * --------------------------------------------------------------------------
 *              UE specific local functions
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _eps_bearer_release()                                     **
 **                                                                        **
 ** Description: Releases the EPS bearer context identified by the given   **
 **      EPS bearer identity and enters state INACTIVE.            **
 **                                                                        **
 ** Inputs:  ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     pid:       Identifier of the PDN connection the EPS   **
 **             bearer belongs to                          **
 **      bid:       Identifier of the released EPS bearer con- **
 **             text entry                                 **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _eps_bearer_release(int ebi, int *pid, int *bid)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  /* Release the EPS bearer context entry */
  ebi = esm_ebr_context_release(ebi, pid, bid);

  if (ebi == ESM_EBI_UNASSIGNED) {
    LOG_TRACE(WARNING, "ESM-PROC  - Failed to release EPS bearer context");
  } else {
    /* Set the EPS bearer context state to INACTIVE */
    rc = esm_ebr_set_status(ebi, ESM_EBR_INACTIVE, FALSE);

    if (rc != RETURNok) {
      /* The EPS bearer context was already in INACTIVE state */
      LOG_TRACE(WARNING, "ESM-PROC  - EBI %d was already INACTIVE", ebi);
    } else {
      /* Release EPS bearer data */
      rc = esm_ebr_release(ebi);

      if (rc != RETURNok) {
        LOG_TRACE(WARNING,
                  "ESM-PROC  - Failed to release EPS bearer data");
      }
    }
  }

  LOG_FUNC_RETURN (rc);
}
