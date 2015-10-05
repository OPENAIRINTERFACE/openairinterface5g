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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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

/*
 * --------------------------------------------------------------------------
 * Internal data handled by the dedicated EPS bearer context activation
 * procedure in the MME
 * --------------------------------------------------------------------------
 */
/*
 * Timer handlers
 */
static void *_dedicated_eps_bearer_activate_t3485_handler(void *);

/* Maximum value of the activate dedicated EPS bearer context request
 * retransmission counter */
#define DEDICATED_EPS_BEARER_ACTIVATE_COUNTER_MAX   5

static int _dedicated_eps_bearer_activate(emm_data_context_t *ctx, int ebi,
    const OctetString *msg);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *    Dedicated EPS bearer context activation procedure executed by the MME
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_dedicated_eps_bearer_context()                   **
 **                                                                        **
 ** Description: Allocates resources required for activation of a dedica-  **
 **      ted EPS bearer context.                                   **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **          pid:       PDN connection identifier                  **
 **      esm_qos:   EPS bearer level QoS parameters            **
 **      tft:       Traffic flow template parameters           **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     ebi:       EPS bearer identity assigned to the new    **
 **             dedicated bearer context                   **
 **      default_ebi:   EPS bearer identity of the associated de-  **
 **             fault EPS bearer context                   **
 **      esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_dedicated_eps_bearer_context(emm_data_context_t *ctx, int pid,
    unsigned int *ebi,
    unsigned int *default_ebi,
    const esm_proc_qos_t *qos,
    const esm_proc_tft_t *tft,
    int *esm_cause)
{
  LOG_FUNC_IN;

  LOG_TRACE(INFO, "ESM-PROC  - Dedicated EPS bearer context activation "
            "(ueid="NAS_UE_ID_FMT", pid=%d)", ctx->ueid, pid);

  /* Assign new EPS bearer context */
  *ebi = esm_ebr_assign(ctx, ESM_EBI_UNASSIGNED);

  if (*ebi != ESM_EBI_UNASSIGNED) {
    /* Create dedicated EPS bearer context */
    *default_ebi = esm_ebr_context_create(ctx, pid, *ebi, FALSE, qos, tft);

    if (*default_ebi == ESM_EBI_UNASSIGNED) {
      /* No resource available */
      LOG_TRACE(WARNING, "ESM-PROC  - Failed to create dedicated EPS "
                "bearer context (ebi=%d)", *ebi);
      *esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
      LOG_FUNC_RETURN (RETURNerror);
    }

    LOG_FUNC_RETURN (RETURNok);
  }

  LOG_TRACE(WARNING, "ESM-PROC  - Failed to assign new EPS bearer context");
  *esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_dedicated_eps_bearer_context_request()           **
 **                                                                        **
 ** Description: Initiates the dedicated EPS bearer context activation pro-**
 **      cedure                                                    **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.2.2                           **
 **      The MME initiates the dedicated EPS bearer context activa-**
 **      tion procedure by sending an ACTIVATE DEDICATED EPS BEA-  **
 **      RER CONTEXT REQUEST message, starting timer T3485 and en- **
 **      tering state BEARER CONTEXT ACTIVE PENDING.               **
 **                                                                        **
 ** Inputs:  is_standalone: Not used (always TRUE)                     **
 **      ueid:      UE lower layer identifier                  **
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
int esm_proc_dedicated_eps_bearer_context_request(int is_standalone,
    emm_data_context_t *ctx, int ebi,
    OctetString *msg, int ue_triggered)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  LOG_TRACE(INFO,"ESM-PROC  - Initiate dedicated EPS bearer context "
            "activation (ueid="NAS_UE_ID_FMT", ebi=%d)", ctx->ueid, ebi);

  /* Send activate dedicated EPS bearer context request message and
   * start timer T3485 */
  rc = _dedicated_eps_bearer_activate(ctx, ebi, msg);

  if (rc != RETURNerror) {
    /* Set the EPS bearer context state to ACTIVE PENDING */
    rc = esm_ebr_set_status(ctx, ebi, ESM_EBR_ACTIVE_PENDING, ue_triggered);

    if (rc != RETURNok) {
      /* The EPS bearer context was already in ACTIVE PENDING state */
      LOG_TRACE(WARNING, "ESM-PROC  - EBI %d was already ACTIVE PENDING",
                ebi);
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_dedicated_eps_bearer_context_accept()            **
 **                                                                        **
 ** Description: Performs dedicated EPS bearer context activation procedu- **
 **      re accepted by the UE.                                    **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.2.3                           **
 **      Upon receipt of the ACTIVATE DEDICATED EPS BEARER CONTEXT **
 **      ACCEPT message, the MME shall stop the timer T3485 and    **
 **      enter the state BEARER CONTEXT ACTIVE.                    **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **      ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_dedicated_eps_bearer_context_accept(emm_data_context_t *ctx, int ebi,
    int *esm_cause)
{
  LOG_FUNC_IN;

  int rc;

  LOG_TRACE(INFO, "ESM-PROC  - Dedicated EPS bearer context activation "
            "accepted by the UE (ueid="NAS_UE_ID_FMT", ebi=%d)", ctx->ueid, ebi);

  /* Stop T3485 timer */
  rc = esm_ebr_stop_timer(ctx, ebi);

  if (rc != RETURNerror) {
    /* Set the EPS bearer context state to ACTIVE */
    rc = esm_ebr_set_status(ctx, ebi, ESM_EBR_ACTIVE, FALSE);

    if (rc != RETURNok) {
      /* The EPS bearer context was already in ACTIVE state */
      LOG_TRACE(WARNING, "ESM-PROC  - EBI %d was already ACTIVE", ebi);
      *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    }
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_dedicated_eps_bearer_context_reject()            **
 **                                                                        **
 ** Description: Performs dedicated EPS bearer context activation procedu- **
 **      re not accepted by the UE.                                **
 **                                                                        **
 **      3GPP TS 24.301, section 6.4.2.4                           **
 **      Upon receipt of the ACTIVATE DEDICATED EPS BEARER CONTEXT **
 **      REJECT message, the MME shall stop the timer T3485, enter **
 **      the state BEARER CONTEXT INACTIVE and abort the dedicated **
 **      EPS bearer context activation procedure.                  **
 **      The MME also requests the lower layer to release the ra-  **
 **      dio resources that were established during the dedicated  **
 **      EPS bearer context activation.                            **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **      ebi:       EPS bearer identity                        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_dedicated_eps_bearer_context_reject(emm_data_context_t *ctx, int ebi,
    int *esm_cause)
{
  int rc;

  LOG_FUNC_IN;

  LOG_TRACE(WARNING, "ESM-PROC  - Dedicated EPS bearer context activation "
            "not accepted by the UE (ueid="NAS_UE_ID_FMT", ebi=%d)", ctx->ueid, ebi);

  /* Stop T3485 timer if running */
  rc = esm_ebr_stop_timer(ctx, ebi);

  if (rc != RETURNerror) {
    int pid, bid;
    /* Release the dedicated EPS bearer context and enter state INACTIVE */
    rc = esm_proc_eps_bearer_context_deactivate(ctx, TRUE, ebi,
         &pid, &bid, NULL);

    if (rc != RETURNok) {
      /* Failed to release the dedicated EPS bearer context */
      *esm_cause = ESM_CAUSE_PROTOCOL_ERROR;
    }
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
/****************************************************************************
 **                                                                        **
 ** Name:    _dedicated_eps_bearer_activate_t3485_handler()            **
 **                                                                        **
 ** Description: T3485 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.4.2.6, case a                   **
 **      On the first expiry of the timer T3485, the MME shall re- **
 **      send the ACTIVATE DEDICATED EPS BEARER CONTEXT REQUEST    **
 **      and shall reset and restart timer T3485. This retransmis- **
 **      sion is repeated four times, i.e. on the fifth expiry of  **
 **      timer T3485, the MME shall abort the procedure, release   **
 **      any resources allocated for this activation and enter the **
 **      state BEARER CONTEXT INACTIVE.                            **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static void *_dedicated_eps_bearer_activate_t3485_handler(void *args)
{
  LOG_FUNC_IN;

  int rc;

  /* Get retransmission timer parameters data */
  esm_ebr_timer_data_t *data = (esm_ebr_timer_data_t *)(args);

  /* Increment the retransmission counter */
  data->count += 1;

  LOG_TRACE(WARNING, "ESM-PROC  - T3485 timer expired (ueid="NAS_UE_ID_FMT", ebi=%d), "
            "retransmission counter = %d",
            data->ueid, data->ebi, data->count);

  if (data->count < DEDICATED_EPS_BEARER_ACTIVATE_COUNTER_MAX) {
    /* Re-send activate dedicated EPS bearer context request message
     * to the UE */
    rc = _dedicated_eps_bearer_activate(data->ctx, data->ebi, &data->msg);
  } else {
    /*
     * The maximum number of activate dedicated EPS bearer context request
     * message retransmission has exceed
     */
    int pid, bid;
    /* Release the dedicated EPS bearer context and enter state INACTIVE */
    rc = esm_proc_eps_bearer_context_deactivate(data->ctx, TRUE,
         data->ebi, &pid, &bid,
         NULL);

    if (rc != RETURNerror) {
      /* Stop timer T3485 */
      rc = esm_ebr_stop_timer(data->ctx, data->ebi);
    }
  }

  LOG_FUNC_RETURN (NULL);
}

/*
 * --------------------------------------------------------------------------
 *              MME specific local functions
 * --------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _dedicated_eps_bearer_activate()                          **
 **                                                                        **
 ** Description: Sends ACTIVATE DEDICATED EPS BEREAR CONTEXT REQUEST mes-  **
 **      sage and starts timer T3485                               **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **      ebi:       EPS bearer identity                        **
 **      msg:       Encoded ESM message to be sent             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    T3485                                      **
 **                                                                        **
 ***************************************************************************/
static int _dedicated_eps_bearer_activate(emm_data_context_t *ctx, int ebi,
    const OctetString *msg)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;

  /*
   * Notify EMM that an activate dedicated EPS bearer context request
   * message has to be sent to the UE
   */
  emm_esm_data_t *emm_esm = &emm_sap.u.emm_esm.u.data;
  emm_sap.primitive = EMMESM_UNITDATA_REQ;
  emm_sap.u.emm_esm.ueid = ctx->ueid;
  emm_sap.u.emm_esm.ctx  = ctx;
  emm_esm->msg = *msg;
  rc = emm_sap_send(&emm_sap);

  if (rc != RETURNerror) {
    /* Start T3485 retransmission timer */
    rc = esm_ebr_start_timer(ctx, ebi, msg, T3485_DEFAULT_VALUE,
                             _dedicated_eps_bearer_activate_t3485_handler);
  }

  LOG_FUNC_RETURN (rc);
}
