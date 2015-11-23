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


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

#define ESM_EBR_NB_UE_MAX   1


/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/* String representation of EPS bearer context status */
static const char *_esm_ebr_state_str[ESM_EBR_STATE_MAX] = {
  "BEARER CONTEXT INACTIVE",
  "BEARER CONTEXT ACTIVE",
};

/*
 * ----------------------------------
 * List of EPS bearer contexts per UE
 * ----------------------------------
 */

#if !defined(NAS_BUILT_IN_EPC)
static esm_ebr_data_t _esm_ebr_data[ESM_EBR_NB_UE_MAX];
#endif

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

static int _esm_ebr_get_available_entry(unsigned int ueid);

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
 **      Others:    _esm_ebr_data                              **
 **                                                                        **
 ***************************************************************************/
void esm_ebr_initialize(
  esm_indication_callback_t cb
)
{
#if !defined(NAS_BUILT_IN_EPC)
  int ueid, i;

  LOG_FUNC_IN;

  for (ueid = 0; ueid < ESM_EBR_NB_UE_MAX; ueid++) {
    _esm_ebr_data[ueid].index = 0;

    /* Initialize EPS bearer context data */
    for (i = 0; i < ESM_EBR_DATA_SIZE + 1; i++) {
      _esm_ebr_data[ueid].context[i] = NULL;
    }
  }

  /* Initialize the user notification callback */
  _esm_ebr_callback = *cb;

  LOG_FUNC_OUT;
#endif
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_ebr_assign()                                          **
 **                                                                        **
 ** Description: Assigns a new EPS bearer context                          **
 **                                                                        **
 ** Inputs:  ueid:      Lower layers UE identifier                 **
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
 **      Others:    _esm_ebr_data                              **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_assign(int ebi, int cid, int default_ebr)
{
  esm_ebr_context_t *ebr_ctx = NULL;
  unsigned int       ueid    = 0;
  int                i;

  LOG_FUNC_IN;

  ebr_ctx = _esm_ebr_data[ueid].context[ebi - ESM_EBI_MIN];


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
    i = _esm_ebr_get_available_entry(ueid);

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


  _esm_ebr_data[ueid].context[ebi - ESM_EBI_MIN] = ebr_ctx;

  /* Store the index of the next available EPS bearer identity */
  _esm_ebr_data[ueid].index = i + 1;

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
 ** Inputs:  ueid:      Lower layers UE identifier                 **
 **      ebi:       The identity of the EPS bearer context to  **
 **             be released                                **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok if the EPS bearer context has     **
 **             been successfully released;                **
 **             RETURNerror otherwise.                     **
 **      Others:    _esm_ebr_data                              **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_release(
  int ebi)
{
  unsigned int ueid = 0;
  esm_ebr_context_t *ebr_ctx;

  LOG_FUNC_IN;

  if ( (ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX) ) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Get EPS bearer context data */
  ebr_ctx = _esm_ebr_data[ueid].context[ebi - ESM_EBI_MIN];

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
 ** Inputs:  ueid:      Lower layers UE identifier                 **
 **      ebi:       The identity of the EPS bearer             **
 **      status:    The new EPS bearer context status          **
 **      ue_requested:  TRUE/FALSE if the modification of the EPS  **
 **             bearer context status was requested by the **
 **             UE/network                                 **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _esm_ebr_data                              **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_set_status(
  int ebi, esm_ebr_state status, int ue_requested)
{
  esm_ebr_context_t *ebr_ctx;
  esm_ebr_state old_status;

  LOG_FUNC_IN;

  unsigned int ueid = 0;

  if (ueid >= ESM_EBR_NB_UE_MAX) {
    LOG_FUNC_RETURN (RETURNerror);
  }


  if ( (ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX) ) {
    LOG_FUNC_RETURN (RETURNerror);
  }

  /* Get EPS bearer context data */
  ebr_ctx = _esm_ebr_data[ueid].context[ebi - ESM_EBI_MIN];

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
              _esm_ebr_state_str[old_status], _esm_ebr_state_str[status]);

    if (status != old_status) {
      ebr_ctx->status = status;
      /*
       * Notify the user that the state of the EPS bearer has changed
       */
      (*_esm_ebr_callback)(ebr_ctx->cid,
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
 ** Inputs:  ueid:      Lower layers UE identifier                 **
 **      ebi:       The identity of the EPS bearer             **
 **      Others:    _esm_ebr_data                              **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The current value of the EPS bearer con-   **
 **             text status                                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/

esm_ebr_state esm_ebr_get_status(
  int ebi)
{
  unsigned int ueid = 0;

  if ( (ebi < ESM_EBI_MIN) || (ebi > ESM_EBI_MAX) ) {
    return (ESM_EBR_INACTIVE);
  }

  if (_esm_ebr_data[ueid].context[ebi - ESM_EBI_MIN] == NULL) {
    /* EPS bearer context not allocated */
    return (ESM_EBR_INACTIVE);
  }

  if (_esm_ebr_data[ueid].context[ebi - ESM_EBI_MIN]->ebi != ebi) {
    /* EPS bearer context not assigned */
    return (ESM_EBR_INACTIVE);
  }

  return (_esm_ebr_data[ueid].context[ebi - ESM_EBI_MIN]->status);
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
int esm_ebr_is_reserved(int ebi)
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
 ** Inputs:  ueid:      Lower layers UE identifier                 **
 **      ebi:       The identity of the EPS bearer             **
 **      Others:    _esm_ebr_data                              **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE, FALSE                                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_ebr_is_not_in_use(
  int ebi)
{
  unsigned int ueid = 0;

  return ( (ebi == ESM_EBI_UNASSIGNED) ||
           (_esm_ebr_data[ueid].context[ebi - ESM_EBI_MIN] == NULL) ||
           (_esm_ebr_data[ueid].context[ebi - ESM_EBI_MIN]->ebi) != ebi);
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
 ** Inputs:  ueid:      Lower layers UE identifier                 **
 **      Others:    _esm_ebr_data                              **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the next available EPS bearer **
 **             context data entry; -1 if no any entry is  **
 **             available.                                 **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _esm_ebr_get_available_entry(unsigned int ueid)
{
  int i;

  for (i = _esm_ebr_data[ueid].index; i < ESM_EBR_DATA_SIZE; i++) {
    if (_esm_ebr_data[ueid].context[i] != NULL) {
      continue;
    }

    return i;
  }

  for (i = 0; i < _esm_ebr_data[ueid].index; i++) {
    if (_esm_ebr_data[ueid].context[i] != NULL) {
      continue;
    }

    return i;
  }

  /* No available EBI entry found */
  return (-1);
}
