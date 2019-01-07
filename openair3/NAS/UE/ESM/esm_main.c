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
Source      esm_main.c

Version     0.1

Date        2012/12/04

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the EPS Session Management procedure call manager,
        the main entry point for elementary ESM processing.

*****************************************************************************/

#include "esm_main.h"
#include "commonDef.h"
#include "nas_log.h"
#include "utils.h"

#include "emmData.h"
#include "esmData.h"
#include "esm_pt.h"
#include "esm_ebr.h"
#include "user_defs.h"

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
 ** Name:    esm_main_initialize()                                     **
 **                                                                        **
 ** Description: Initializes ESM internal data                             **
 **                                                                        **
 ** Inputs:  cb:        The user notification callback             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **                                                                        **
 ***************************************************************************/
void esm_main_initialize(nas_user_t *user, esm_indication_callback_t cb)
{
  LOG_FUNC_IN;

  int i;

  esm_data_t *esm_data = calloc_or_fail(sizeof(esm_data_t));
  user->esm_data = esm_data;

  default_eps_bearer_context_data_t *default_eps_bearer_context = calloc(1, sizeof(default_eps_bearer_context_data_t));
  if ( default_eps_bearer_context == NULL ) {
    LOG_TRACE(ERROR, "ESM-MAIN  - Can't malloc default_eps_bearer_context");
    exit(EXIT_FAILURE);
  }
  default_eps_bearer_context->ebi = ESM_EBI_UNASSIGNED;
  user->default_eps_bearer_context_data = default_eps_bearer_context;
  /* Total number of active EPS bearer contexts */
  esm_data->n_ebrs = 0;
  /* List of active PDN connections */
  esm_data->n_pdns = 0;

  for (i = 0; i < ESM_DATA_PDN_MAX + 1; i++) {
    esm_data->pdn[i].pid = -1;
    esm_data->pdn[i].is_active = FALSE;
    esm_data->pdn[i].data = NULL;
  }

  /* Emergency bearer services indicator */
  esm_data->emergency = FALSE;

  /* Initialize the procedure transaction identity manager */

  user->esm_pt_data = esm_pt_initialize();

  /* Initialize the EPS bearer context manager */
  user->esm_ebr_data = esm_ebr_initialize();
  // FIXME only one callback for all user or many for many ?
  esm_ebr_register_callback(cb);
  LOG_FUNC_OUT;
}


/****************************************************************************
 **                                                                        **
 ** Name:        esm_main_cleanup()                                        **
 **                                                                        **
 ** Description: Performs the EPS Session Management clean up procedure    **
 **                                                                        **
 ** Inputs:      None                                                      **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    None                                       **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
void esm_main_cleanup(esm_data_t *esm_data)
{
  LOG_FUNC_IN;

  {
    int i;
    int pid;
    int bid;

    /* De-activate EPS bearers and clean up PDN connections */
    for (pid = 0; pid < ESM_DATA_PDN_MAX; pid++) {
      if (esm_data->pdn[pid].data) {
        esm_pdn_t *pdn = esm_data->pdn[pid].data;

        if (pdn->apn.length > 0) {
          free(pdn->apn.value);
        }

        /* Release EPS bearer contexts */
        for (bid = 0; bid < pdn->n_bearers; bid++) {
          if (pdn->bearer[bid]) {
            LOG_TRACE(WARNING, "ESM-MAIN  - Release EPS bearer "
                      "context (ebi=%d)", pdn->bearer[bid]->ebi);

            /* Delete the TFT */
            for (i = 0; i < pdn->bearer[bid]->tft.n_pkfs; i++) {
              if (pdn->bearer[bid]->tft.pkf[i]) {
                free(pdn->bearer[bid]->tft.pkf[i]);
              }
            }

            free(pdn->bearer[bid]);
          }
        }

        /* Release the PDN connection */
        free(esm_data->pdn[pid].data);
      }
    }
  }

  LOG_FUNC_OUT;
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_main_get_nb_pdn_max()                                 **
 **                                                                        **
 ** Description: Get the maximum number of PDN connections that may be in  **
 **      a defined state at the same time                          **
 **                                                                        **
 ** Inputs:  None                                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The maximum number of PDN connections that **
 **             may be defined                             **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_get_nb_pdns_max(esm_data_t *esm_data)
{
  LOG_FUNC_IN;

  LOG_FUNC_RETURN (ESM_DATA_PDN_MAX);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_main_get_nb_pdns()                                    **
 **                                                                        **
 ** Description: Get the number of active PDN connections                  **
 **                                                                        **
 ** Inputs:  None                                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The number of active PDN connections       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_get_nb_pdns(esm_data_t *esm_data)
{
  LOG_FUNC_IN;

  LOG_FUNC_RETURN (esm_data->n_pdns);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_main_has_emergency()                                  **
 **                                                                        **
 ** Description: Check whether a PDN connection for emergency bearer ser-  **
 **      vices is established                                      **
 **                                                                        **
 ** Inputs:  None                                                      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE if a PDN connection for emergency     **
 **             bearer services is established             **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_has_emergency(esm_data_t *esm_data)
{
  LOG_FUNC_IN;

  LOG_FUNC_RETURN (esm_data->emergency);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_main_get_pdn_status()                                 **
 **                                                                        **
 ** Description: Get the status of the specified PDN connection            **
 **                                                                        **
 ** Inputs:  cid:       PDN connection identifier                  **
 **                                                                        **
 ** Outputs:     state:     TRUE if the current state of the PDN con-  **
 **             nection is ACTIVE; FALSE otherwise.        **
 **      Return:    TRUE if the specified PDN connection has a **
 **             PDN context defined; FALSE if no any PDN   **
 **             context has been defined for the specified **
 **             connection.                                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_get_pdn_status(nas_user_t *user, int cid, int *state)
{
  LOG_FUNC_IN;

  unsigned int pid = cid - 1;
  esm_data_t *esm_data = user->esm_data;
  esm_ebr_data_t *esm_ebr_data = user-> esm_ebr_data;

  if (pid >= ESM_DATA_PDN_MAX) {
    return (FALSE);
  } else if (pid != esm_data->pdn[pid].pid) {
    LOG_TRACE(WARNING, "ESM-MAIN  - PDN connection %d is not defined", cid);
    return (FALSE);
  } else if (esm_data->pdn[pid].data == NULL) {
    LOG_TRACE(ERROR, "ESM-MAIN  - PDN connection %d has not been allocated",
              cid);
    return (FALSE);
  }

  if (esm_data->pdn[pid].data->bearer[0] != NULL) {
    /* The status of a PDN connection is the status of the default EPS bearer
     * that has been assigned to this PDN connection at activation time */
    int ebi = esm_data->pdn[pid].data->bearer[0]->ebi;
    *state = (esm_ebr_get_status(esm_ebr_data, ebi) == ESM_EBR_ACTIVE);
  }

  /* The PDN connection has not been activated yet */
  LOG_FUNC_RETURN (TRUE);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_main_get_pdn()                                        **
 **                                                                        **
 ** Description: Get parameters defined for the specified PDN connection   **
 **                                                                        **
 ** Inputs:  cid:       PDN connection identifier                  **
 **                                                                        **
 ** Outputs:     type:      PDN connection type (IPv4, IPv6, IPv4v6)   **
 **      apn:       Access Point logical Name in used          **
 **      is_emergency:  Emergency bearer services indicator        **
 **      is_active: Active PDN connection indicator            **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_get_pdn(esm_data_t *esm_data, int cid, int *type, const char **apn,
                     int *is_emergency, int *is_active)
{
  LOG_FUNC_IN;

  unsigned int pid = cid - 1;

  if (pid >= ESM_DATA_PDN_MAX) {
    return (RETURNerror);
  } else if (pid != esm_data->pdn[pid].pid) {
    LOG_TRACE(WARNING, "ESM-MAIN  - PDN connection %d is not defined", cid);
    return (RETURNerror);
  } else if (esm_data->pdn[pid].data == NULL) {
    LOG_TRACE(ERROR, "ESM-MAIN  - PDN connection %d has not been allocated",
              cid);
    return (RETURNerror);
  }

  /* Get the PDN type */
  *type = esm_data->pdn[pid].data->type;

  /* Get the Access Point Name */
  if (esm_data->pdn[pid].data->apn.length > 0) {
    *apn = (char *)(esm_data->pdn[pid].data->apn.value);
  } else {
    *apn = NULL;
  }

  /* Get the emergency bearer services indicator */
  *is_emergency = esm_data->pdn[pid].data->is_emergency;
  /* Get the active PDN connection indicator */
  *is_active = esm_data->pdn[pid].is_active;

  LOG_FUNC_RETURN (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_main_get_pdn_addr()                                   **
 **                                                                        **
 ** Description: Get IP address(es) assigned to the specified PDN connec-  **
 **      tion                                                      **
 **                                                                        **
 ** Inputs:  cid:       PDN connection identifier                  **
 **                                                                        **
 ** Outputs:     ipv4adddr: IPv4 address                               **
 **      ipv6adddr: IPv6 address                               **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_get_pdn_addr(esm_data_t *esm_data, int cid, const char **ipv4addr, const char **ipv6addr)
{
  LOG_FUNC_IN;

  unsigned int pid = cid - 1;

  if (pid >= ESM_DATA_PDN_MAX) {
    return (RETURNerror);
  } else if (pid != esm_data->pdn[pid].pid) {
    LOG_TRACE(WARNING, "ESM-MAIN  - PDN connection %d is not defined", cid);
    return (RETURNerror);
  } else if (esm_data->pdn[pid].data == NULL) {
    LOG_TRACE(ERROR, "ESM-MAIN  - PDN connection %d has not been allocated",
              cid);
    return (RETURNerror);
  } else if (!esm_data->pdn[pid].is_active) {
    /* No any IP address has been assigned to this PDN connection */
    return (RETURNok);
  }

  if (esm_data->pdn[pid].data->type == NET_PDN_TYPE_IPV4) {
    /* Get IPv4 address */
    *ipv4addr = esm_data->pdn[pid].data->ip_addr;
  } else if (esm_data->pdn[pid].data->type == NET_PDN_TYPE_IPV6) {
    /* Get IPv6 address */
    *ipv6addr = esm_data->pdn[pid].data->ip_addr;
  } else {
    /* IPv4v6 dual-stack terminal */
    *ipv4addr = esm_data->pdn[pid].data->ip_addr;
    *ipv6addr = esm_data->pdn[pid].data->ip_addr+ESM_DATA_IPV4_ADDRESS_SIZE;
  }

  LOG_FUNC_RETURN (RETURNok);
}

