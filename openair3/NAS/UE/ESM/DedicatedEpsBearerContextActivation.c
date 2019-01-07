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
Source      DedicatedEpsBearerContextActivation.c

Version     0.1

Date        2013/07/16

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the dedicated EPS bearer context activation ESM
        procedure executed by the Non-Access Stratum.

        The purpose of the dedicated EPS bearer context activation
        procedure is to establish an EPS bearer context with specific
        QoS and TFT between the UE and the EPC.

        The procedure is initiated by the network, but may be requested
        by the UE by means of the UE requested bearer resource alloca-
        tion procedure or the UE requested bearer resource modification
        procedure.
        It can be part of the attach procedure or be initiated together
        with the default EPS bearer context activation procedure when
        the UE initiated stand-alone PDN connectivity procedure.

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
 *    Dedicated EPS bearer context activation procedure executed by the MME
 * --------------------------------------------------------------------------
 */

/*
 * --------------------------------------------------------------------------
 *    Dedicated EPS bearer context activation procedure executed by the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_dedicated_eps_bearer_context_request()           **
 **                                                                        **
 ** Description: Creates local dedicated EPS bearer context upon receipt   **
 **      of the ACTIVATE DEDICATED EPS BEARER CONTEXT REQUEST      **
 **      message.                                                  **
 **                                                                        **
 ** Inputs:  ebi:       EPS bearer identity                        **
 **      default_ebi:   EPS bearer identity of the associated de-  **
 **             fault EPS bearer context                   **
 **      esm_qos:   EPS bearer level QoS parameters            **
 **      tft:       Traffic flow template parameters           **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_dedicated_eps_bearer_context_request(nas_user_t *user, int ebi, int default_ebi,
    const esm_proc_qos_t *qos,
    const esm_proc_tft_t *tft,
    int *esm_cause)
{
  LOG_FUNC_IN;
  esm_data_t *esm_data = user->esm_data;
  int rc = RETURNerror;

  LOG_TRACE(INFO, "ESM-PROC  - Dedicated EPS bearer context activation "
            "requested by the network (ebi=%d)", ebi);

  /* Get the PDN connection the dedicated EPS bearer is linked to */
  int pid = esm_ebr_context_get_pid(esm_data, default_ebi);

  if (pid < 0) {
    /* 3GPP TS 24.301, section 6.4.2.5, abnormal case c
     * No default EPS bearer context with linked EPS bearer identity
     * activated
     */
    LOG_TRACE(WARNING, "ESM-PROC  - Failed to get PDN connection the "
              "dedicated EPS bearer is linked to (ebi=%d)", default_ebi);
    *esm_cause = ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY;
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Assign dedicated EPS bearer context */
  int new_ebi = esm_ebr_assign(user->esm_ebr_data, ebi, pid+1, FALSE);

  if (new_ebi == ESM_EBI_UNASSIGNED) {
    /* 3GPP TS 24.301, section 6.4.2.5, abnormal cases a and b
     * Dedicated EPS bearer context activation request for an already
     * activated default or dedicated EPS bearer context
     */
    int old_pid, old_bid;
    /* Locally deactivate the existing EPS bearer context and proceed
     * with the requested dedicated EPS bearer context activation */
    rc = esm_proc_eps_bearer_context_deactivate(user, TRUE, ebi,
         &old_pid, &old_bid);

    if (rc != RETURNok) {
      /* Failed to release EPS bearer context */
      *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    } else {
      /* Assign new dedicated EPS bearer context */
      ebi = esm_ebr_assign(user->esm_ebr_data, ebi, pid+1, FALSE);
    }
  }

  if (ebi != ESM_EBI_UNASSIGNED) {
    /* Check syntactical errors in packet filters */
    rc = esm_ebr_context_check_tft(esm_data, pid, ebi, tft,
                                   ESM_EBR_CONTEXT_TFT_CREATE);

    if (rc != RETURNok) {
      /* Syntactical errors in packet filters */
      LOG_TRACE(WARNING, "ESM-PROC  - Syntactical errors in packet "
                "filters");
      *esm_cause = ESM_CAUSE_SYNTACTICAL_ERROR_IN_PACKET_FILTER;
    } else {
      /* Create new dedicated EPS bearer context */
      default_ebi = esm_ebr_context_create(esm_data, user->ueid, pid, ebi, FALSE, qos, tft);

      if (default_ebi != ESM_EBI_UNASSIGNED) {
        /* Dedicated EPS bearer contextx successfully created */
        rc = RETURNok;
      } else {
        /* No resource available */
        LOG_TRACE(WARNING, "ESM-PROC  - Failed to create new dedicated "
                  "EPS bearer context");
        *esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
      }
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_dedicated_eps_bearer_context_accept()            **
 **                                                                        **
 ** Description: Performs dedicated EPS bearer context activation proce-   **
 **      dure accepted by the UE.                                  **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.2.3                           **
 **      The UE accepts dedicated EPS bearer context activation by **
 **      sending ACTIVATE DEDICATED EPS BEARER CONTEXT ACCEPT mes- **
 **      sage and entering the state BEARER CONTEXT ACTIVE.        **
 **                                                                        **
 ** Inputs:  is_standalone: Not used                                   **
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
int esm_proc_dedicated_eps_bearer_context_accept(nas_user_t *user, int is_standalone, int ebi,
    OctetString *msg, int ue_triggered)
{
  LOG_FUNC_IN;

  int rc;
  esm_ebr_data_t *esm_ebr_data = user->esm_ebr_data;
  user_api_id_t *user_api_id = user->user_api_id;

  LOG_TRACE(INFO,"ESM-PROC  - Dedicated EPS bearer context activation "
            "accepted by the UE (ebi=%d)", ebi);

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
    /* Set the EPS bearer context state to ACTIVE */
    rc = esm_ebr_set_status(user_api_id, esm_ebr_data, ebi, ESM_EBR_ACTIVE, ue_triggered);

    if (rc != RETURNok) {
      /* The EPS bearer context was already in ACTIVE state */
      LOG_TRACE(WARNING, "ESM-PROC  - EBI %d was already ACTIVE", ebi);
      /* Accept network retransmission of already accepted activate
       * dedicated EPS bearer context request */
      LOG_FUNC_RETURN (RETURNok);
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_dedicated_eps_bearer_context_reject()            **
 **                                                                        **
 ** Description: Performs dedicated EPS bearer context activation proce-   **
 **      dure not accepted by the UE.                              **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.2.4                           **
 **      The UE rejects dedicated EPS bearer context activation by **
 **      sending ACTIVATE DEDICATED EPS BEARER CONTEXT REJECT mes- **
 **      sage.                                                     **
 **                                                                        **
 ** Inputs:  is_standalone: Not used                                   **
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
int esm_proc_dedicated_eps_bearer_context_reject(nas_user_t *user, int is_standalone, int ebi,
    OctetString *msg, int ue_triggered)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(WARNING, "ESM-PROC  - Dedicated EPS bearer context activation "
            "not accepted by the UE (ebi=%d)", ebi);

  if ( !esm_ebr_is_not_in_use(user->esm_ebr_data, ebi) ) {
    /* Release EPS bearer data currently in use */
    rc = esm_ebr_release(user->esm_ebr_data, ebi);
  }

  if (rc != RETURNok) {
    LOG_TRACE(WARNING, "ESM-PROC  - Failed to release EPS bearer data");
  } else {
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
