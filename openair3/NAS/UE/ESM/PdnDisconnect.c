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
Source      PdnDisconnect.c

Version     0.1

Date        2013/05/15

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the PDN disconnect ESM procedure executed by the
        Non-Access Stratum.

        The PDN disconnect procedure is used by the UE to request
        disconnection from one PDN.

        All EPS bearer contexts established towards this PDN, inclu-
        ding the default EPS bearer context, are released.

*****************************************************************************/

#include "esm_proc.h"
#include "commonDef.h"
#include "nas_log.h"

#include "esmData.h"
#include "esm_cause.h"
#include "esm_pt.h"

#include "esm_sap.h"

#include "emm_sap.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/



/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the PDN disconnect procedure in the UE
 * --------------------------------------------------------------------------
 */
/*
 * PDN disconnection handlers
 */
static int _pdn_disconnect_get_default_ebi(esm_data_t *esm_data, int pti);

/*
 * Timer handlers
 */
static void *_pdn_disconnect_t3492_handler(void *);

/* Maximum value of the PDN disconnect request retransmission counter */
#define ESM_PDN_DISCONNECT_COUNTER_MAX 5



/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *        PDN disconnect procedure executed by the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_disconnect()                                 **
 **                                                                        **
 ** Description: Return the procedure transaction identity assigned to the **
 **      PDN connection and the EPS bearer identity of the default **
 **      bearer associated to the PDN context with specified iden- **
 **      tifier                                                    **
 **                                                                        **
 ** Inputs:  cid:       PDN context identifier                     **
 **                                                                        **
 ** Outputs:     pti:       Procedure transaction identity assigned to **
 **             the PDN connection to be released          **
 **      ebi:       EPS bearer identity of the default bearer  **
 **             associated to the specified PDN context    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_disconnect(esm_data_t *esm_data, int cid, unsigned int *pti, unsigned int *ebi)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;
  int pid = cid - 1;

  if (pid < ESM_DATA_PDN_MAX) {
    if (pid != esm_data->pdn[pid].pid) {
      LOG_TRACE(WARNING, "ESM-PROC  - PDN connection identifier %d is "
                "not valid", pid);
    } else if (esm_data->pdn[pid].data == NULL) {
      LOG_TRACE(ERROR, "ESM-PROC  - PDN connection %d has not been "
                "allocated", pid);
    } else if (!esm_data->pdn[pid].is_active) {
      LOG_TRACE(WARNING, "ESM-PROC  - PDN connection is not active");
    } else {
      /* Get the procedure transaction identity assigned to the PDN
       * connection to be released */
      *pti = esm_data->pdn[pid].data->pti;
      /* Get the EPS bearer identity of the default bearer associated
       * with the PDN to disconnect from */
      *ebi = esm_data->pdn[pid].data->bearer[0]->ebi;
      rc = RETURNok;
    }
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_disconnect_request()                         **
 **                                                                        **
 ** Description: Initiates PDN disconnection procedure in order to request **
 **      disconnection from a PDN.                                 **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.2.2                           **
 **      The UE requests PDN disconnection from a PDN by sending a **
 **      PDN DISCONNECT REQUEST message to the MME, starting timer **
 **      T3492 and entering state PROCEDURE TRANSACTION PENDING.   **
 **                                                                        **
 ** Inputs:  is_standalone: Should be always true                      **
 **      pti:       Procedure transaction identity             **
 **      msg:       Encoded PDN disconnect request message to  **
 **             be sent                                    **
 **      sent_by_ue:    Not used                                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_disconnect_request(nas_user_t *user, bool is_standalone, int pti,
                                    OctetString *msg, bool sent_by_ue)
{
  LOG_FUNC_IN;

  int rc = RETURNok;
  esm_pt_data_t *esm_pt_data = user->esm_pt_data;
  LOG_TRACE(INFO, "ESM-PROC  - Initiate PDN disconnection (pti=%d)", pti);

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

    if (rc != RETURNerror) {
      /* Start T3482 retransmission timer */
      rc = esm_pt_start_timer(user, pti, msg, T3492_DEFAULT_VALUE,
                              _pdn_disconnect_t3492_handler);
    }
  }

  if (rc != RETURNerror) {
    /* Set the procedure transaction state to PENDING */
    rc = esm_pt_set_status(esm_pt_data, pti, ESM_PT_PENDING);

    if (rc != RETURNok) {
      /* The procedure transaction was already in PENDING state */
      LOG_TRACE(WARNING, "ESM-PROC  - PTI %d was already PENDING", pti);
    }
  }

  LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_disconnect_accept()                          **
 **                                                                        **
 ** Description: Performs PDN disconnection procedure accepted by the net- **
 **      work.                                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.2.3                           **
 **      The shall stop timer T3492 and enter the state PROCEDURE  **
 **      TRANSACTION INACTIVE.                                     **
 **                                                                        **
 ** Inputs:  pti:       Identifies the UE requested PDN disconnect **
 **             procedure accepted by the network          **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    The identifier of the PDN context to de-   **
 **             activate when successfully found;          **
 **             RETURNerror otherwise.                     **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_disconnect_accept(esm_pt_data_t *esm_pt_data, int pti, int *esm_cause)
{
  LOG_FUNC_IN;

  LOG_TRACE(INFO, "ESM-PROC  - PDN disconnection accepted by the network "
            "(pti=%d)", pti);

  /* Stop T3492 timer if running */
  (void) esm_pt_stop_timer(esm_pt_data, pti);
  /* Set the procedure transaction state to INACTIVE */
  int rc = esm_pt_set_status(esm_pt_data, pti, ESM_PT_INACTIVE);

  if (rc != RETURNok) {
    /* The procedure transaction was already in INACTIVE state */
    LOG_TRACE(WARNING, "ESM-PROC  - PTI %d was already INACTIVE", pti);
    *esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE;
  } else {
    /* Immediately release the transaction identity assigned to this
     * procedure */
    rc = esm_pt_release(esm_pt_data, pti);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "ESM-PROC  - Failed to release PTI %d", pti);
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_disconnect_reject()                          **
 **                                                                        **
 ** Description: Performs PDN disconnection procedure not accepted by the  **
 **      network.                                                  **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.2.4                           **
 **      Upon receipt of the PDN DISCONNECT REJECT message, the UE **
 **      shall stop timer T3492 and enter the state PROCEDURE      **
 **      TRANSACTION INACTIVE and abort the PDN disconnection pro- **
 **      cedure.                                                   **
 **                                                                        **
 ** Inputs:  pti:       Identifies the UE requested PDN disconnec- **
 **             tion procedure rejected by the network     **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_disconnect_reject(nas_user_t *user, int pti, int *esm_cause)
{
  LOG_FUNC_IN;
  esm_data_t *esm_data = user->esm_data;
  int rc;

  LOG_TRACE(WARNING, "ESM-PROC  - PDN disconnection rejected by the network "
            "(pti=%d), ESM cause = %d", pti, *esm_cause);

  /* Stop T3492 timer if running */
  (void) esm_pt_stop_timer(user->esm_pt_data, pti);
  /* Set the procedure transaction state to INACTIVE */
  rc = esm_pt_set_status(user->esm_pt_data, pti, ESM_PT_INACTIVE);

  if (rc != RETURNok) {
    /* The procedure transaction was already in INACTIVE state
     * as the request may have already been rejected */
    LOG_TRACE(WARNING, "ESM-PROC  - PTI %d was already INACTIVE", pti);
    *esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE;
  } else {
    /* Release the transaction identity assigned to this procedure */
    rc = esm_pt_release(user->esm_pt_data, pti);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "ESM-PROC  - Failed to release PTI %d", pti);
      *esm_cause = ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED;
    } else if (*esm_cause != ESM_CAUSE_LAST_PDN_DISCONNECTION_NOT_ALLOWED) {
      /* Get the identity of the default EPS bearer context allocated to
       * the PDN connection entry assigned to this procedure transaction */
      int ebi = _pdn_disconnect_get_default_ebi(esm_data, pti);

      if (ebi < 0) {
        LOG_TRACE(ERROR, "ESM-PROC  - No default EPS bearer found");
        *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
        LOG_FUNC_RETURN (RETURNerror);
      }

      /*
       * Notify ESM that all EPS bearer contexts to this PDN have to be
       * locally deactivated
       */
      esm_sap_t esm_sap;
      esm_sap.primitive = ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ;
      esm_sap.is_standalone = true;
      esm_sap.recv = NULL;
      esm_sap.send.length = 0;
      esm_sap.data.eps_bearer_context_deactivate.ebi = ebi;
      rc = esm_sap_send(user, &esm_sap);

      if (rc != RETURNok) {
        *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
      }
    }
  }

  LOG_FUNC_RETURN(rc);
}




/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *              Timer handlers
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _pdn_disconnect_t3492_handler()                           **
 **                                                                        **
 ** Description: T3492 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.2.5, case a                   **
 **      On the first expiry of the timer T3492, the UE shall re-  **
 **      send the PDN DISCONNECT REQUEST and shall reset and re-   **
 **      start timer T3492. This retransmission is repeated four   **
 **      times, i.e. on the fifth expiry of timer T3492, the UE    **
 **      shall abort the procedure, deactivate all EPS bearer con- **
 **      texts for this PDN connection locally, release the PTI    **
 **      allocated for this invocation and enter the state PROCE-  **
 **      DURE TRANSACTION INACTIVE.                                **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void *_pdn_disconnect_t3492_handler(void *args)
{
  LOG_FUNC_IN;
  nas_user_t *user = args;
  esm_data_t *esm_data = user->esm_data;;
  int rc;

  /* Get retransmission timer parameters data */
  esm_pt_timer_data_t *data = (esm_pt_timer_data_t *)(args);

  /* Increment the retransmission counter */
  data->count += 1;

  LOG_TRACE(WARNING, "ESM-PROC  - T3492 timer expired (pti=%d), "
            "retransmission counter = %d", data->pti, data->count);

  if (data->count < ESM_PDN_DISCONNECT_COUNTER_MAX) {
    emm_sap_t emm_sap;
    emm_esm_data_t *emm_esm = &emm_sap.u.emm_esm.u.data;
    /*
     * Notify EMM that the PDN connectivity request message
     * has to be sent again
     */
    emm_sap.primitive = EMMESM_UNITDATA_REQ;
    emm_sap.u.emm_esm.ueid = user->ueid;
    emm_esm->msg.length = data->msg.length;
    emm_esm->msg.value = data->msg.value;
    rc = emm_sap_send(user, &emm_sap);

    if (rc != RETURNerror) {
      /* Restart the timer T3492 */
      rc = esm_pt_start_timer(user, data->pti, &data->msg, T3492_DEFAULT_VALUE,
                              _pdn_disconnect_t3492_handler);
    }
  } else {
    /* Set the procedure transaction state to INACTIVE */
    rc = esm_pt_set_status(user->esm_pt_data, data->pti, ESM_PT_INACTIVE);

    if (rc != RETURNok) {
      /* The procedure transaction was already in INACTIVE state */
      LOG_TRACE(WARNING, "ESM-PROC  - PTI %d was already INACTIVE",
                data->pti);
    } else {
      /* Release the transaction identity assigned to this procedure */
      rc = esm_pt_release(user->esm_pt_data, data->pti);

      if (rc != RETURNok) {
        LOG_TRACE(WARNING, "ESM-PROC  - Failed to release PTI %d",
                  data->pti);
      } else {
        /* Get the identity of the default EPS bearer context
         * allocated to the PDN connection entry assigned to
         * this procedure transaction */
        int ebi = _pdn_disconnect_get_default_ebi(esm_data, data->pti);

        if (ebi < 0) {
          LOG_TRACE(ERROR, "ESM-PROC  - No default EPS bearer found");
          LOG_FUNC_RETURN (NULL);
        }

        /*
         * Notify ESM that all EPS bearer contexts to this PDN have
         * to be locally deactivated
         */
        esm_sap_t esm_sap;
        esm_sap.primitive = ESM_EPS_BEARER_CONTEXT_DEACTIVATE_REQ;
        esm_sap.is_standalone = true;
        esm_sap.recv = NULL;
        esm_sap.send.length = 0;
        esm_sap.data.eps_bearer_context_deactivate.ebi = ebi;
        rc = esm_sap_send(user, &esm_sap);
      }
    }
  }

  LOG_FUNC_RETURN(NULL);
}

/*
 *---------------------------------------------------------------------------
 *              PDN disconnection handlers
 *---------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _pdn_disconnect_get_default_ebi()                         **
 **                                                                        **
 ** Description: Returns the EPS bearer identity of the default EPS bearer **
 **      context allocated to the PDN connection to which the gi-  **
 **      ven procedure transaction identity has been assigned      **
 **                                                                        **
 ** Inputs:  pti:       The procedure transaction identity         **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The EPS bearer identity of the default EPS **
 **             bearer context, if it exists; -1 otherwise **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _pdn_disconnect_get_default_ebi(esm_data_t *esm_data, int pti)
{
  int ebi = -1;
  int i;

  for (i = 0; i < ESM_DATA_PDN_MAX; i++) {
    if ( (esm_data->pdn[i].pid != -1) && esm_data->pdn[i].data ) {
      if (esm_data->pdn[i].data->pti != pti) {
        continue;
      }

      /* PDN entry found */
      if (esm_data->pdn[i].data->bearer[0] != NULL) {
        /* Get the EPS bearer identity of the default EPS bearer
         * context associated to the PDN connection */
        ebi = esm_data->pdn[i].data->bearer[0]->ebi;
      }

      break;
    }
  }

  return (ebi);
}
