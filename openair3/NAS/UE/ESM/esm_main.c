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

#include "emmData.h"
#include "esmData.h"
#include "esm_pt.h"
#include "esm_ebr.h"

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
 **      Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
void esm_main_initialize(esm_indication_callback_t cb)
{
  LOG_FUNC_IN;

  int i;

  /* Total number of active EPS bearer contexts */
  _esm_data.n_ebrs = 0;
  /* List of active PDN connections */
  _esm_data.n_pdns = 0;

  for (i = 0; i < ESM_DATA_PDN_MAX + 1; i++) {
    _esm_data.pdn[i].pid = -1;
    _esm_data.pdn[i].is_active = FALSE;
    _esm_data.pdn[i].data = NULL;
  }

  /* Emergency bearer services indicator */
  _esm_data.emergency = FALSE;

  /* Initialize the procedure transaction identity manager */
  esm_pt_initialize();

  /* Initialize the EPS bearer context manager */
  esm_ebr_initialize(cb);

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
void esm_main_cleanup(void)
{
  LOG_FUNC_IN;

  {
    int i;
    int pid;
    int bid;

    /* De-activate EPS bearers and clean up PDN connections */
    for (pid = 0; pid < ESM_DATA_PDN_MAX; pid++) {
      if (_esm_data.pdn[pid].data) {
        esm_pdn_t *pdn = _esm_data.pdn[pid].data;

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
        free(_esm_data.pdn[pid].data);
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
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The maximum number of PDN connections that **
 **             may be defined                             **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_get_nb_pdns_max(void)
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
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The number of active PDN connections       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_get_nb_pdns(void)
{
  LOG_FUNC_IN;

  LOG_FUNC_RETURN (_esm_data.n_pdns);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_main_has_emergency()                                  **
 **                                                                        **
 ** Description: Check whether a PDN connection for emergency bearer ser-  **
 **      vices is established                                      **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    TRUE if a PDN connection for emergency     **
 **             bearer services is established             **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_has_emergency(void)
{
  LOG_FUNC_IN;

  LOG_FUNC_RETURN (_esm_data.emergency);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_main_get_pdn_status()                                 **
 **                                                                        **
 ** Description: Get the status of the specified PDN connection            **
 **                                                                        **
 ** Inputs:  cid:       PDN connection identifier                  **
 **      Others:    _esm_data                                  **
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
int esm_main_get_pdn_status(int cid, int *state)
{
  LOG_FUNC_IN;

  unsigned int pid = cid - 1;

  if (pid >= ESM_DATA_PDN_MAX) {
    return (FALSE);
  } else if (pid != _esm_data.pdn[pid].pid) {
    LOG_TRACE(WARNING, "ESM-MAIN  - PDN connection %d is not defined", cid);
    return (FALSE);
  } else if (_esm_data.pdn[pid].data == NULL) {
    LOG_TRACE(ERROR, "ESM-MAIN  - PDN connection %d has not been allocated",
              cid);
    return (FALSE);
  }

  if (_esm_data.pdn[pid].data->bearer[0] != NULL) {
    /* The status of a PDN connection is the status of the default EPS bearer
     * that has been assigned to this PDN connection at activation time */
    int ebi = _esm_data.pdn[pid].data->bearer[0]->ebi;
    *state = (esm_ebr_get_status(ebi) == ESM_EBR_ACTIVE);
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
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     type:      PDN connection type (IPv4, IPv6, IPv4v6)   **
 **      apn:       Access Point logical Name in used          **
 **      is_emergency:  Emergency bearer services indicator        **
 **      is_active: Active PDN connection indicator            **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_get_pdn(int cid, int *type, const char **apn,
                     int *is_emergency, int *is_active)
{
  LOG_FUNC_IN;

  unsigned int pid = cid - 1;

  if (pid >= ESM_DATA_PDN_MAX) {
    return (RETURNerror);
  } else if (pid != _esm_data.pdn[pid].pid) {
    LOG_TRACE(WARNING, "ESM-MAIN  - PDN connection %d is not defined", cid);
    return (RETURNerror);
  } else if (_esm_data.pdn[pid].data == NULL) {
    LOG_TRACE(ERROR, "ESM-MAIN  - PDN connection %d has not been allocated",
              cid);
    return (RETURNerror);
  }

  /* Get the PDN type */
  *type = _esm_data.pdn[pid].data->type;

  /* Get the Access Point Name */
  if (_esm_data.pdn[pid].data->apn.length > 0) {
    *apn = (char *)(_esm_data.pdn[pid].data->apn.value);
  } else {
    *apn = NULL;
  }

  /* Get the emergency bearer services indicator */
  *is_emergency = _esm_data.pdn[pid].data->is_emergency;
  /* Get the active PDN connection indicator */
  *is_active = _esm_data.pdn[pid].is_active;

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
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     ipv4adddr: IPv4 address                               **
 **      ipv6adddr: IPv6 address                               **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_main_get_pdn_addr(int cid, const char **ipv4addr, const char **ipv6addr)
{
  LOG_FUNC_IN;

  unsigned int pid = cid - 1;

  if (pid >= ESM_DATA_PDN_MAX) {
    return (RETURNerror);
  } else if (pid != _esm_data.pdn[pid].pid) {
    LOG_TRACE(WARNING, "ESM-MAIN  - PDN connection %d is not defined", cid);
    return (RETURNerror);
  } else if (_esm_data.pdn[pid].data == NULL) {
    LOG_TRACE(ERROR, "ESM-MAIN  - PDN connection %d has not been allocated",
              cid);
    return (RETURNerror);
  } else if (!_esm_data.pdn[pid].is_active) {
    /* No any IP address has been assigned to this PDN connection */
    return (RETURNok);
  }

  if (_esm_data.pdn[pid].data->type == NET_PDN_TYPE_IPV4) {
    /* Get IPv4 address */
    *ipv4addr = _esm_data.pdn[pid].data->ip_addr;
  } else if (_esm_data.pdn[pid].data->type == NET_PDN_TYPE_IPV6) {
    /* Get IPv6 address */
    *ipv6addr = _esm_data.pdn[pid].data->ip_addr;
  } else {
    /* IPv4v6 dual-stack terminal */
    *ipv4addr = _esm_data.pdn[pid].data->ip_addr;
    *ipv6addr = _esm_data.pdn[pid].data->ip_addr+ESM_DATA_IPV4_ADDRESS_SIZE;
  }

  LOG_FUNC_RETURN (RETURNok);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

