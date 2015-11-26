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
Source      esm_pt.c

Version     0.1

Date        2013/01/03

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions used to handle ESM procedure transactions.

*****************************************************************************/

#include "esm_pt.h"

#include "commonDef.h"
#include "nas_log.h"

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/*
 * Minimal and maximal value of a procedure transaction identity:
 * The Procedure Transaction Identity (PTI) identifies bi-directional
 * messages flows
 */
#define ESM_PTI_MIN     (PROCEDURE_TRANSACTION_IDENTITY_FIRST)
#define ESM_PTI_MAX     (PROCEDURE_TRANSACTION_IDENTITY_LAST)

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of ESM procedure transaction status */
static const char *_esm_pt_state_str[ESM_PT_STATE_MAX] = {
  "PROCEDURE TRANSACTION INACTIVE",
  "PROCEDURE TRANSACTION PENDING"
};

/*
 * --------------------------
 * Procedure transaction data
 * --------------------------
 */
typedef struct {
  unsigned char pti;      /* Procedure transaction identity   */
  esm_pt_state status;    /* Procedure transaction status     */
  struct nas_timer_t timer;   /* Retransmission timer         */
  esm_pt_timer_data_t *args;  /* Retransmission timer parameters data */
} esm_pt_context_t;

/*
 * ------------------------------
 * List of procedure transactions
 * ------------------------------
 */
static struct {
  unsigned char index;    /* Index of the next procedure transaction
                 * identity to be used */
#define ESM_PT_DATA_SIZE (ESM_PTI_MAX - ESM_PTI_MIN + 1)
  esm_pt_context_t *context[ESM_PT_DATA_SIZE + 1];
} _esm_pt_data;

/* Return the index of the next available entry in the list of procedure
 * transaction data */
static int _esm_pt_get_available_entry(void);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_pt_initialize()                                       **
 **                                                                        **
 ** Description: Initialize ESM procedure transaction data                 **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _esm_pt_data                               **
 **                                                                        **
 ***************************************************************************/
void esm_pt_initialize(void)
{
  LOG_FUNC_IN;

  int i;

  _esm_pt_data.index = 0;

  for (i = 0; i < ESM_PT_DATA_SIZE + 1; i++) {
    _esm_pt_data.context[i] = NULL;
  }

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_pt_assign()                                           **
 **                                                                        **
 ** Description: Assigns a new procedure transaction identity              **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The identity of the new procedure transac- **
 **             tion when successfully assigned;           **
 **             the unassigned PTI (0) otherwise.          **
 **      Others:    _esm_pt_data                               **
 **                                                                        **
 ***************************************************************************/
int esm_pt_assign(void)
{
  LOG_FUNC_IN;

  /* Search for an available procedure transaction identity */
  int i = _esm_pt_get_available_entry();

  if (i < 0) {
    LOG_FUNC_RETURN (ESM_PT_UNASSIGNED);
  }

  /* Assign new procedure transaction */
  _esm_pt_data.context[i] =
    (esm_pt_context_t *)malloc(sizeof(esm_pt_context_t));

  if (_esm_pt_data.context[i] == NULL) {
    LOG_FUNC_RETURN (ESM_PT_UNASSIGNED);
  }

  /* Store the index of the next available procedure transaction identity */
  _esm_pt_data.index = i + 1;

  /* An available procedure transaction identity is found */
  _esm_pt_data.context[i]->pti = i + ESM_PTI_MIN;
  /* Set the procedure transaction status to INACTIVE */
  _esm_pt_data.context[i]->status = ESM_PT_INACTIVE;
  /* Disable the retransmission timer */
  _esm_pt_data.context[i]->timer.id = NAS_TIMER_INACTIVE_ID;
  /* Setup retransmission timer parameters */
  _esm_pt_data.context[i]->args = NULL;

  LOG_TRACE(INFO, "ESM-FSM   - Procedure transaction identity %d assigned",
            _esm_pt_data.context[i]->pti);
  LOG_FUNC_RETURN (_esm_pt_data.context[i]->pti);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_pt_release()                                          **
 **                                                                        **
 ** Description: Release the given procedure transaction identity          **
 **                                                                        **
 ** Inputs:  pti:       The identity of the procedure transaction  **
 **             to release                                 **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok if the procedure transaction iden-**
 **             tity has been successfully released;       **
 **             RETURNerror otherwise.                     **
 **      Others:    _esm_pt_data                               **
 **                                                                        **
 ***************************************************************************/
int esm_pt_release(int pti)
{
  LOG_FUNC_IN;

  if ( (pti < ESM_PTI_MIN) || (pti > ESM_PTI_MAX) ) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Get procedure transaction data */
  esm_pt_context_t *ctx = _esm_pt_data.context[pti - ESM_PTI_MIN];

  if ( (ctx == NULL) || (ctx->pti != pti) ) {
    /* Procedure transaction not assigned */
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Do not release active procedure transaction */
  if (ctx->status != ESM_PT_INACTIVE) {
    LOG_TRACE(ERROR, "ESM-FSM   - Procedure transaction is not INACTIVE");
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Stop the retransmission timer if still running */
  if (ctx->timer.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "ESM-FSM   - Stop retransmission timer %d",
              ctx->timer.id);
    ctx->timer.id = nas_timer_stop(ctx->timer.id);
  }

  /* Release the retransmisison timer parameters */
  if (ctx->args) {
    if (ctx->args->msg.length > 0) {
      free(ctx->args->msg.value);
    }

    free(ctx->args);
    ctx->args = NULL;
  }

  /* Release transaction procedure data */
  free(_esm_pt_data.context[pti - ESM_PTI_MIN]);
  _esm_pt_data.context[pti - ESM_PTI_MIN] = NULL;

  LOG_TRACE(INFO, "ESM-FSM   - Procedure transaction %d released", pti);

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_pt_start_timer()                                      **
 **                                                                        **
 ** Description: Start the timer of the specified procedure transaction to **
 **      expire after a given time interval. Timer expiration will **
 **      schedule execution of the callback function where stored  **
 **      ESM message should be re-transmit.                        **
 **                                                                        **
 ** Inputs:  pti:       The identity of the procedure transaction  **
 **      msg:       The encoded ESM message to be stored       **
 **      sec:       The value of the time interval in seconds  **
 **      cb:        Function executed upon timer expiration    **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _esm_pt_data                               **
 **                                                                        **
 ***************************************************************************/
int esm_pt_start_timer(int pti, const OctetString *msg,
                       long sec, nas_timer_callback_t cb)
{
  LOG_FUNC_IN;

  if ( (pti < ESM_PTI_MIN) || (pti > ESM_PTI_MAX) ) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Get procedure transaction data */
  esm_pt_context_t *ctx = _esm_pt_data.context[pti - ESM_PTI_MIN];

  if ( (ctx == NULL) || (ctx->pti != pti) ) {
    /* Procedure transaction not assigned */
    LOG_FUNC_RETURN (RETURNerror);
  }

  if (ctx->timer.id != NAS_TIMER_INACTIVE_ID) {
    if (ctx->args) {
      /* Re-start the retransmission timer */
      ctx->timer.id = nas_timer_restart(ctx->timer.id);
    }
  } else {
    /* Setup the retransmission timer parameters */
    ctx->args = (esm_pt_timer_data_t *)malloc(sizeof(esm_pt_timer_data_t));

    if (ctx->args) {
      /* Set the EPS bearer identity */
      ctx->args->pti = pti;
      /* Reset the retransmission counter */
      ctx->args->count = 0;
      /* Set the ESM message to be re-transmited */
      ctx->args->msg.value = (uint8_t *)malloc(msg->length);
      ctx->args->msg.length = 0;

      if (ctx->args->msg.value) {
        memcpy(ctx->args->msg.value, msg->value, msg->length);
        ctx->args->msg.length = msg->length;
      }

      /* Setup the retransmission timer to expire at the given
       * time interval */
      ctx->timer.id = nas_timer_start(sec, cb, ctx->args);
      ctx->timer.sec = sec;
    }
  }

  if ( (ctx->args != NULL) && (ctx->timer.id != NAS_TIMER_INACTIVE_ID) ) {
    LOG_TRACE(INFO, "ESM-FSM   - Retransmission timer %d expires in "
              "%ld seconds", ctx->timer.id, ctx->timer.sec);
    LOG_FUNC_RETURN (RETURNok);
  }

  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_pt_stop_timer()                                       **
 **                                                                        **
 ** Description: Stop the timer previously started for the given procedure **
 **      transaction                                               **
 **                                                                        **
 ** Inputs:  pti:       The identity of the procedure transaction  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _esm_pt_data                               **
 **                                                                        **
 ***************************************************************************/
int esm_pt_stop_timer(int pti)
{
  LOG_FUNC_IN;

  if ( (pti < ESM_PTI_MIN) || (pti > ESM_PTI_MAX) ) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Get procedure transaction data */
  esm_pt_context_t *ctx = _esm_pt_data.context[pti - ESM_PTI_MIN];

  if ( (ctx == NULL) || (ctx->pti != pti) ) {
    /* Procedure transaction not assigned */
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Stop the retransmission timer if still running */
  if (ctx->timer.id != NAS_TIMER_INACTIVE_ID) {
    LOG_TRACE(INFO, "ESM-FSM   - Stop retransmission timer %d",
              ctx->timer.id);
    ctx->timer.id = nas_timer_stop(ctx->timer.id);
  }

  /* Release the retransmisison timer parameters */
  if (ctx->args) {
    if (ctx->args->msg.length > 0) {
      free(ctx->args->msg.value);
    }

    free(ctx->args);
    ctx->args = NULL;
  }

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_pt_set_status()                                       **
 **                                                                        **
 ** Description: Set the status of the specified procedure transaction to  **
 **      the given state                                           **
 **                                                                        **
 ** Inputs:  pti:       The identity of the procedure transaction  **
 **      status:    The new ESM procedure transaction status   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _esm_pt_data                               **
 **                                                                        **
 ***************************************************************************/
int esm_pt_set_status(int pti, esm_pt_state status)
{
  LOG_FUNC_IN;

  esm_pt_state old_status;

  if ( (pti < ESM_PTI_MIN) || (pti > ESM_PTI_MAX) ) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Get procedure transaction data */
  esm_pt_context_t *ctx = _esm_pt_data.context[pti - ESM_PTI_MIN];

  if ( (ctx == NULL) || (ctx->pti != pti) ) {
    /* Procedure transaction not assigned */
    LOG_TRACE(ERROR, "ESM-FSM   - Procedure transaction not assigned "
              "(pti=%d)", pti);
    LOG_FUNC_RETURN (RETURNerror);
  }

  old_status = ctx->status;

  if (status < ESM_PT_STATE_MAX) {
    LOG_TRACE(INFO, "ESM-FSM   - Status of procedure transaction %d changed:"
              " %s ===> %s", pti,
              _esm_pt_state_str[old_status], _esm_pt_state_str[status]);

    if (status != old_status) {
      ctx->status = status;
      LOG_FUNC_RETURN (RETURNok);
    }
  }

  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_pt_get_status()                                       **
 **                                                                        **
 ** Description: Get the current status value of the specified procedure   **
 **      transaction                                               **
 **                                                                        **
 ** Inputs:  pti:       The identity of the procedure transaction  **
 **      Others:    _esm_pt_data                               **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The current value of the ESM procedure     **
 **             transaction status                         **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
esm_pt_state esm_pt_get_status(int pti)
{
  if ( (pti < ESM_PTI_MIN) || (pti > ESM_PTI_MAX) ) {
    return (ESM_PT_INACTIVE);
  }

  if (_esm_pt_data.context[pti - ESM_PTI_MIN] == NULL) {
    /* Procedure transaction not allocated */
    return (ESM_PT_INACTIVE);
  }

  if (_esm_pt_data.context[pti - ESM_PTI_MIN]->pti != pti) {
    /* Procedure transaction not assigned */
    return (ESM_PT_INACTIVE);
  }

  return (_esm_pt_data.context[pti - ESM_PTI_MIN]->status);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_pt_get_pending_pti()                                  **
 **                                                                        **
 ** Description: Returns the procedure transaction identity assigned to    **
 **      the first PDN connection entry which is pending in the    **
 **      given state                                               **
 **                                                                        **
 ** Inputs:  status:    The PDN connection status                  **
 **      Others:    _esm_pt_data                               **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The procedure transaction identity of the  **
 **             PDN connection entry if it exists;         **
 **             the unassigned PTI (0) otherwise.          **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_pt_get_pending_pti(esm_pt_state status)
{
  LOG_FUNC_IN;

  int i;

  for (i = 0; i < ESM_PT_DATA_SIZE; i++) {
    if (_esm_pt_data.context[i] == NULL) {
      continue;
    }

    if (_esm_pt_data.context[i]->status != status) {
      continue;
    }

    /* PDN connection entry found */
    break;
  }

  if (i < ESM_PT_DATA_SIZE) {
    LOG_FUNC_RETURN (_esm_pt_data.context[i]->pti);
  }

  /* PDN connection entry not found */
  LOG_FUNC_RETURN (ESM_PT_UNASSIGNED);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_pt_is_not_in_use()                                    **
 **                                                                        **
 ** Description: Check whether the given procedure transaction identity    **
 **      does not match an assigned PTI value currently in use     **
 **                                                                        **
 ** Inputs:  pti:       The identity of the procedure transaction  **
 **      Others:    _esm_pt_data                               **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE, FALSE                                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_pt_is_not_in_use(int pti)
{
  return ( (pti == ESM_PT_UNASSIGNED) ||
           (_esm_pt_data.context[pti - ESM_PTI_MIN] == NULL) ||
           (_esm_pt_data.context[pti - ESM_PTI_MIN]->pti) != pti);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_pt_is_reserved()                                      **
 **                                                                        **
 ** Description: Check whether the given procedure transaction identity is **
 **      a reserved value                                          **
 **                                                                        **
 ** Inputs:  pti:       The identity of the procedure transaction  **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE, FALSE                                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_pt_is_reserved(int pti)
{
  return ( (pti != ESM_PT_UNASSIGNED) && (pti > ESM_PTI_MAX) );
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    _esm_pt_get_available_entry()                             **
 **                                                                        **
 ** Description: Returns the index of the next available entry in the list **
 **      of procedure transaction data                             **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _esm_pt_data                               **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the next available procedure  **
 **             transaction data entry; -1 if no any entry **
 **             is available.                              **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _esm_pt_get_available_entry(void)
{
  int i;

  for (i = _esm_pt_data.index; i < ESM_PT_DATA_SIZE; i++) {
    if (_esm_pt_data.context[i] != NULL) {
      continue;
    }

    return i;
  }

  for (i = 0; i < _esm_pt_data.index; i++) {
    if (_esm_pt_data.context[i] != NULL) {
      continue;
    }

    return i;
  }

  /* No available PTI entry found */
  return (-1);
}
