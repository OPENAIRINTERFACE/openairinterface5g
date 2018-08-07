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
Source      esm_ebr.c

Version     0.1

Date        2013/01/29

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions used to handle state of EPS bearer contexts
        and manage ESM messages re-transmission.

*****************************************************************************/

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy

#include "emmData.h"
#include "esm_ebr.h"
#include "commonDef.h"
#include "nas_log.h"
#include "utils.h"


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of EPS bearer context status */
const char *esm_ebr_state2str(int esmebrstate) {
static const char *_esm_ebr_state_str[ESM_EBR_STATE_MAX] = {
  "BEARER CONTEXT INACTIVE",
  "BEARER CONTEXT ACTIVE",
};
  return _esm_ebr_state_str[esmebrstate];
}

/*
 * ----------------------
 * User notification data
 * ----------------------
 */
/* User notification callback executed whenever an EPS bearer context becomes
 * active or inactive */
static esm_indication_callback_t _esm_ebr_callback;
/* PDN connection and EPS bearer status [NW/UE][Dedicated/Default][status] */
static const network_pdn_state_t _esm_ebr_pdn_state[2][2][2] = {
  /* Status modification triggerer by the network */
  {
    /* Dedicated EPS bearer */
    {NET_PDN_NW_DEDICATED_DEACT, NET_PDN_NW_DEDICATED_ACT},
    /* Default EPS bearer */
    {NET_PDN_NW_DEFAULT_DEACT, 0}
  },
  /* Status modification triggered by the UE */
  {
    /* Dedicated EPS bearer */
    {NET_PDN_MT_DEDICATED_DEACT, NET_PDN_MT_DEDICATED_ACT},
    /* Default EPS bearer */
    {NET_PDN_MT_DEFAULT_DEACT, NET_PDN_MT_DEFAULT_ACT}
  }
};

/* Returns the index of the next available entry in the list of EPS bearer
 * context data */

static int _esm_ebr_get_available_entry(esm_ebr_data_t *esm_ebr_data);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_initialize()                                      **
 **                                                                        **
 ** Description: Initialize EPS bearer context data                        **
 **                                                                        **
 ** Inputs:  cb:        User notification callback                 **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **                                                                        **
 ***************************************************************************/

esm_ebr_data_t *esm_ebr_initialize(void)
{

  LOG_FUNC_IN;

  int i;
  esm_ebr_data_t *esm_ebr_data = calloc_or_fail(sizeof(esm_ebr_data_t));

  esm_ebr_data->index = 0;

  /* Initialize EPS bearer context data */
  for (i = 0; i < ESM_EBR_DATA_SIZE + 1; i++) {
    esm_ebr_data->context[i] = NULL;
  }

  LOG_FUNC_OUT;
  return esm_ebr_data;
}

void esm_ebr_register_callback(esm_indication_callback_t cb)
{

  LOG_FUNC_IN;

  /* Initialize the user notification callback */
  _esm_ebr_callback = *cb;

  LOG_FUNC_OUT;
}


/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_assign()                                          **
 **                                                                        **
 ** Description: Assigns a new EPS bearer context                          **
 **                                                                        **
 **      ebi:       Identity of the new EPS bearer context     **
 **      cid:       Identifier of the PDN context the EPS bea- **
 **             rer context is associated to               **
 **      default_ebr    TRUE if the new bearer context is associa- **
 **             ted to a default EPS bearer                **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The identity of the new EPS bearer context **
 **             if successfully assigned;                  **
 **             the not assigned EBI (0) otherwise.        **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_assign(esm_ebr_data_t *esm_ebr_data, int ebi, int cid, int default_ebr)
{
  esm_ebr_context_t *ebr_ctx = NULL;
  int                i;

  LOG_FUNC_IN;

  ebr_ctx = esm_ebr_data->context[ebi - ESM_EBI_MIN];


  if (ebi != ESM_EBI_UNASSIGNED) {
    if ( (ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX) ) {
      LOG_FUNC_RETURN (ESM_EBI_UNASSIGNED);
    } else if (ebr_ctx != NULL) {
      LOG_TRACE(WARNING, "ESM-FSM   - EPS bearer context already "
                "assigned (ebi=%d)", ebi);
      LOG_FUNC_RETURN (ESM_EBI_UNASSIGNED);
    }

    /* The specified EPS bearer context is available */
    i = ebi - ESM_EBI_MIN;
  } else {
    /* Search for an available EPS bearer identity */
    i = _esm_ebr_get_available_entry(esm_ebr_data);

    if (i < 0) {
      LOG_FUNC_RETURN(ESM_EBI_UNASSIGNED);
    }

    /* An available EPS bearer context is found */
    ebi = i + ESM_EBI_MIN;
  }

  /* Assign new EPS bearer context */
  ebr_ctx =
    (esm_ebr_context_t *)malloc(sizeof(esm_ebr_context_t));

  if (ebr_ctx == NULL) {
    LOG_FUNC_RETURN(ESM_EBI_UNASSIGNED);
  }


  esm_ebr_data->context[ebi - ESM_EBI_MIN] = ebr_ctx;

  /* Store the index of the next available EPS bearer identity */
  esm_ebr_data->index = i + 1;

  /* Set the EPS bearer identity */
  ebr_ctx->ebi = ebi;
  /* Set the EPS bearer context status to INACTIVE */
  ebr_ctx->status = ESM_EBR_INACTIVE;
  /* Set the default EPS bearer indicator */
  ebr_ctx->is_default_ebr = default_ebr;
  /* Set the identifier of the PDN context the EPS bearer is assigned to */
  ebr_ctx->cid = cid;

  LOG_TRACE(INFO, "ESM-FSM   - EPS bearer context %d assigned", ebi);
  LOG_FUNC_RETURN(ebr_ctx->ebi);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_release()                                         **
 **                                                                        **
 ** Description: Release the given EPS bearer identity                     **
 **                                                                        **
 ** Inputs:   **
 **      ebi:       The identity of the EPS bearer context to  **
 **             be released                                **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok if the EPS bearer context has     **
 **             been successfully released;                **
 **             RETURNerror otherwise.                     **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_release(esm_ebr_data_t *esm_ebr_data,
  int ebi)
{
  esm_ebr_context_t *ebr_ctx;

  LOG_FUNC_IN;

  if ( (ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX) ) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Get EPS bearer context data */
  ebr_ctx = esm_ebr_data->context[ebi - ESM_EBI_MIN];

  if ( (ebr_ctx == NULL) || (ebr_ctx->ebi != ebi) ) {
    /* EPS bearer context not assigned */
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Do not release active EPS bearer context */
  if (ebr_ctx->status != ESM_EBR_INACTIVE) {
    LOG_TRACE(ERROR, "ESM-FSM   - EPS bearer context is not INACTIVE");
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Release EPS bearer context data */
  free(ebr_ctx);
  ebr_ctx = NULL;

  LOG_TRACE(INFO, "ESM-FSM   - EPS bearer context %d released", ebi);
  LOG_FUNC_RETURN (RETURNok);
}



/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_set_status()                                      **
 **                                                                        **
 ** Description: Set the status of the specified EPS bearer context to the **
 **      given state                                               **
 **                                                                        **
 **      ebi:       The identity of the EPS bearer             **
 **      status:    The new EPS bearer context status          **
 **      ue_requested:  TRUE/FALSE if the modification of the EPS  **
 **             bearer context status was requested by the **
 **             UE/network                                 **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_set_status(user_api_id_t *user_api_id, esm_ebr_data_t *esm_ebr_data,
  int ebi, esm_ebr_state status, int ue_requested)
{
  esm_ebr_context_t *ebr_ctx;
  esm_ebr_state old_status;

  LOG_FUNC_IN;


  if ( (ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX) ) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Get EPS bearer context data */
  ebr_ctx = esm_ebr_data->context[ebi - ESM_EBI_MIN];

  if ( (ebr_ctx == NULL) || (ebr_ctx->ebi != ebi) ) {
    /* EPS bearer context not assigned */
    LOG_TRACE(ERROR, "ESM-FSM   - EPS bearer context not assigned "
              "(ebi=%d)", ebi);
    LOG_FUNC_RETURN (RETURNerror);
  }

  old_status = ebr_ctx->status;

  if (status < ESM_EBR_STATE_MAX) {
    LOG_TRACE(INFO, "ESM-FSM   - Status of EPS bearer context %d changed:"
              " %s ===> %s", ebi,
              esm_ebr_state2str(old_status), esm_ebr_state2str(status));

    if (status != old_status) {
      ebr_ctx->status = status;
      /*
       * Notify the user that the state of the EPS bearer has changed
       */
      (*_esm_ebr_callback)(user_api_id, ebr_ctx->cid,
                           _esm_ebr_pdn_state[ue_requested][ebr_ctx->is_default_ebr][status]);
      LOG_FUNC_RETURN (RETURNok);
    }
  }

  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_get_status()                                      **
 **                                                                        **
 ** Description: Get the current status value of the specified EPS bearer  **
 **      context                                                   **
 **                                                                        **
 **      ebi:       The identity of the EPS bearer             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The current value of the EPS bearer con-   **
 **             text status                                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/

esm_ebr_state esm_ebr_get_status(esm_ebr_data_t *esm_ebr_data,
  int ebi)
{

  if ( (ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX) ) {
    return (ESM_EBR_INACTIVE);
  }

  if (esm_ebr_data->context[ebi - ESM_EBI_MIN] == NULL) {
    /* EPS bearer context not allocated */
    return (ESM_EBR_INACTIVE);
  }

  if (esm_ebr_data->context[ebi - ESM_EBI_MIN]->ebi != ebi) {
    /* EPS bearer context not assigned */
    return (ESM_EBR_INACTIVE);
  }

  return (esm_ebr_data->context[ebi - ESM_EBI_MIN]->status);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_is_reserved()                                     **
 **                                                                        **
 ** Description: Check whether the given EPS bearer identity is a reserved **
 **      value                                                     **
 **                                                                        **
 ** Inputs:  ebi:       The identity of the EPS bearer             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE, FALSE                                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_is_reserved(esm_ebr_data_t *esm_ebr_data, int ebi)
{
  return ( (ebi != ESM_EBI_UNASSIGNED) && (ebi < ESM_EBI_MIN) );
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_is_not_in_use()                                   **
 **                                                                        **
 ** Description: Check whether the given EPS bearer identity does not      **
 **      match an assigned EBI value currently in use              **
 **                                                                        **
 **      ebi:       The identity of the EPS bearer             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE, FALSE                                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_is_not_in_use(esm_ebr_data_t *esm_ebr_data,
  int ebi)
{

  return ( (ebi == ESM_EBI_UNASSIGNED) ||
           (esm_ebr_data->context[ebi - ESM_EBI_MIN] == NULL) ||
           (esm_ebr_data->context[ebi - ESM_EBI_MIN]->ebi) != ebi);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    _esm_ebr_get_available_entry()                            **
 **                                                                        **
 ** Description: Returns the index of the next available entry in the list **
 **      of EPS bearer context data                                **
 **                                                                        **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the next available EPS bearer **
 **             context data entry; -1 if no any entry is  **
 **             available.                                 **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _esm_ebr_get_available_entry(esm_ebr_data_t *esm_ebr_data)
{
  int i;

  for (i = esm_ebr_data->index; i < ESM_EBR_DATA_SIZE; i++) {
    if (esm_ebr_data->context[i] != NULL) {
      continue;
    }

    return i;
  }

  for (i = 0; i < esm_ebr_data->index; i++) {
    if (esm_ebr_data->context[i] != NULL) {
      continue;
    }

    return i;
  }

  /* No available EBI entry found */
  return (-1);
}
