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
Source      IdleMode.c

Version     0.1

Date        2012/10/18

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines EMM procedures executed by the Non-Access Stratum
        when the UE is in idle mode.

        When a UE is switched on, a Public Land Mobile Network is
        selected and the UE searches for a suitable cell of this PLMN
        to camp on. The UE will, if necessary, register its presence
        in the registration area of the chosen cell and as outcome of
        a successful Location Registration the selected PLMN becomes
        the registered PLMN.

        If the UE loses coverage of the registered PLMN, either a new
        PLMN is selected automatically (automatic mode), or an indi-
        cation of which PLMNs are available is given to the user, so
        that a manual selection can be made (manual mode).

        If the UE is unable to find a suitable cell to camp on, or
        the USIM is not inserted, or if the location registration
        failed under certain conditions, it attempts to camp on a
        cell irrespective of the PLMN identity, and enters a "limited
        service" state in which it can only attempt to make emergency
        calls.

*****************************************************************************/


#include "emm_proc.h"
#include "nas_log.h"

#include "emm_sap.h"

#include "IdleMode.h"
#include "emmData.h"

#include <assert.h>
#include <stdio.h>  // sprintf
#include <string.h> // strlen, strncpy

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

static int _IdleMode_plmn_str(char *plmn_str, const plmn_t *plmn);
static int _IldlMode_get_opnn_id(const plmn_t *plmn);
static int _IdleMode_get_suitable_cell(int index);

/*
 * A list of PLMN identities in priority order is maintained locally
 * to perform the PLMN selection procedure.
 *
 * In automatic mode of operation, this list is used for PLMN selection when
 * the UE is switched on, or upon recovery from lack of coverage, or when the
 * user requests the UE to initiate PLMN reselection, and registration.
 * In manual mode of operation, this list is displayed to the user that may
 * select an available PLMN and initiate registration.
 *
 * The list may contain PLMN identifiers in the following order:
 * - The last registered PLMN or each equivalent PLMN present in the list of
 *   "equivalent PLMNs" (EPLMN_MAX), when UE is switched on or following
 *   recovery from lack of coverage;
 * - The highest priority PLMN in the list of "equivalent HPLMNs" or the
 *   HPLMN derived from the IMSI (1)
 * - Each PLMN/access technology combination in the "User Controlled PLMN
 *   Selector with Access Technology" (PLMN_MAX)
 * - Each PLMN/access technology combination in the "Operator Controlled PLMN
 *   Selector with Access Technology" (OPLMN_MAX)
 * - Other PLMN/access technology combinations with received high quality
 *   signal in random order (TODO)
 * - Other PLMN/access technology combinations in order of decreasing signal
 *   quality (TODO)
 * - The last selected PLMN again (1)
 */
static struct {
  int n_plmns;
#define EMM_PLMN_LIST_SIZE (EMM_DATA_EPLMN_MAX + EMM_DATA_PLMN_MAX +    \
                            EMM_DATA_OPLMN_MAX + 2)
  plmn_t *plmn[EMM_PLMN_LIST_SIZE];
  int index;    /* Index of the PLMN for which selection is ongoing        */
  int hplmn;    /* Index of the home PLMN or the highest priority
           * equivalent home PLMN                    */
  int fplmn;    /* Index of the first forbidden PLMN               */
  int splmn;    /* Index of the currently selected PLMN            */
  int rplmn;    /* Index of the currently registered PLMN          */
  struct plmn_param_t {
    char fullname[NET_FORMAT_LONG_SIZE+1];   /* PLMN full identifier     */
    char shortname[NET_FORMAT_SHORT_SIZE+1]; /* PLMN short identifier    */
    char num[NET_FORMAT_NUM_SIZE+1];     /* PLMN numeric identifier  */
    int stat; /* Indication of the PLMN availability             */
    int tac;  /* Location/Tracking Area Code                 */
    int ci;   /* Serving cell identifier                     */
    int rat;  /* Radio Access Technology supported by the serving cell   */
  } param[EMM_PLMN_LIST_SIZE];
} _emm_plmn_list;

/* Callback executed whenever a network indication is received */
static IdleMode_callback_t _emm_indication_notify;

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_initialize()                                     **
 **                                                                        **
 ** Description: Initializes EMM internal data used when the UE operates   **
 **      in idle mode                                              **
 **                                                                        **
 ** Inputs:  cb:        The function to executed whenever a net-   **
 **             work indication is received                **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_plmn_list, _emm_indication_notify     **
 **                                                                        **
 ***************************************************************************/
void IdleMode_initialize(IdleMode_callback_t cb)
{
  /* Initialize the list of available PLMNs */
  _emm_plmn_list.n_plmns = 0;
  _emm_plmn_list.index = 0;
  _emm_plmn_list.hplmn = -1;
  _emm_plmn_list.fplmn = -1;
  _emm_plmn_list.splmn = -1;
  _emm_plmn_list.rplmn = -1;

  /* Initialize the network notification handler */
  _emm_indication_notify = *cb;

  /* Initialize EMM Service Access Point */
  emm_sap_initialize();
}

/*
 *---------------------------------------------------------------------------
 *  Functions used to get information from the local list of available PLMNs
 *---------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_get_nb_plmns()                                   **
 **                                                                        **
 ** Description: Get the number of available PLMNs in the ordered list.    **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The number of PLMNs in the ordered list of **
 **             available PLMNs                            **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int IdleMode_get_nb_plmns(void)
{
  return _emm_plmn_list.n_plmns;
}

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_get_hplmn_index()                                **
 **                                                                        **
 ** Description: Get the index of the Home PLMN or the highest priority    **
 **      Equivalent HPLMN in the ordered list of available PLMNs.  **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the HPLMN or the first EHPLMN **
 **             in the list                                **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int IdleMode_get_hplmn_index(void)
{
  return _emm_plmn_list.hplmn;
}

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_get_rplmn_index()                                **
 **                                                                        **
 ** Description: Get the index of the registered PLMN in the ordered list  **
 **      of available PLMNs.                                       **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the registered PLMN in the    **
 **             list                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int IdleMode_get_rplmn_index(void)
{
  return _emm_plmn_list.rplmn;
}

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_get_splmn_index()                                **
 **                                                                        **
 ** Description: Get the index of the selected PLMN in the ordered list of **
 **      available PLMNs.                                          **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the selected PLMN in the list **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int IdleMode_get_splmn_index(void)
{
  return _emm_plmn_list.splmn;
}

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_update_plmn_list()                               **
 **                                                                        **
 ** Description: Updates the string representation of the list of opera-   **
 **      tors present in the network                               **
 **                                                                        **
 ** Inputs:  i:     Index of the first operator to update      **
 **          Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The size of the list in bytes              **
 **          Others:    _emm_data.plist                            **
 **                                                                        **
 ***************************************************************************/
int IdleMode_update_plmn_list(int i)
{
  int offset = 0;
  int n = 1;

  while ( (i < _emm_plmn_list.n_plmns) && (offset < EMM_DATA_BUFFER_SIZE) ) {
    struct plmn_param_t *plmn = &(_emm_plmn_list.param[i++]);

    if (n++ > 1) {
      offset += snprintf(_emm_data.plist.buffer + offset,
                         EMM_DATA_BUFFER_SIZE - offset, ",");
    }

    offset += snprintf(_emm_data.plist.buffer + offset,
                       EMM_DATA_BUFFER_SIZE - offset, "(%d,%s,%s,%s",
                       plmn->stat, plmn->fullname,
                       plmn->shortname, plmn->num);

    if (plmn->rat != NET_ACCESS_UNAVAILABLE) {
      offset += snprintf(_emm_data.plist.buffer + offset,
                         EMM_DATA_BUFFER_SIZE - offset, ",%d",
                         plmn->rat);
    }

    offset += snprintf(_emm_data.plist.buffer + offset,
                       EMM_DATA_BUFFER_SIZE - offset, ")");
  }

  return (offset);
}

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_get_plmn_fullname()                              **
 **                                                                        **
 ** Description: Get the full name of the PLMN at the given index in the   **
 **      ordered list of available PLMNs.                          **
 **                                                                        **
 ** Inputs:  plmn:      The PLMN of which the name is queried      **
 **      index:     The index of the PLMN in the ordered list  **
 **             of available PLMNs                         **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     size:      The length of the PLMN's name              **
 **      Return:    A pointer to the full name of the PLMN     **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const char *IdleMode_get_plmn_fullname(const plmn_t *plmn, int index,
                                       size_t *len)
{
  if (index < _emm_plmn_list.n_plmns) {
    assert( PLMNS_ARE_EQUAL(*plmn, *_emm_plmn_list.plmn[index]) );
    *len = strlen(_emm_plmn_list.param[index].fullname);
    return _emm_plmn_list.param[index].fullname;
  }

  return NULL;
}

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_get_plmn_shortname()                             **
 **                                                                        **
 ** Description: Get the short name of the PLMN at the given index in the  **
 **      ordered list of available PLMNs.                          **
 **                                                                        **
 ** Inputs:  plmn:      The PLMN of which the name is queried      **
 **      index:     The index of the PLMN in the ordered list  **
 **             of available PLMNs                         **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     size:      The length of the PLMN's name              **
 **      Return:    A pointer to the short name of the PLMN    **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const char *IdleMode_get_plmn_shortname(const plmn_t *plmn, int index,
                                        size_t *len)
{
  if (index < _emm_plmn_list.n_plmns) {
    assert( PLMNS_ARE_EQUAL(*plmn, *_emm_plmn_list.plmn[index]) );
    *len = strlen(_emm_plmn_list.param[index].shortname);
    return _emm_plmn_list.param[index].shortname;
  }

  return NULL;
}

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_get_plmn_id()                                    **
 **                                                                        **
 ** Description: Get the numeric identifier of the PLMN at the given index **
 **      in the ordered list of available PLMNs.                   **
 **                                                                        **
 ** Inputs:  plmn:      The PLMN of which the name is queried      **
 **      index:     The index of the PLMN in the ordered list  **
 **             of available PLMNs                         **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     size:      The length of the PLMN's name              **
 **      Return:    A pointer to the numeric identifier of the **
 **             PLMN                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
const char *IdleMode_get_plmn_id(const plmn_t *plmn, int index, size_t *len)
{
  if (index < _emm_plmn_list.n_plmns) {
    assert( PLMNS_ARE_EQUAL(*plmn, *_emm_plmn_list.plmn[index]) );
    *len = strlen(_emm_plmn_list.param[index].num);
    return _emm_plmn_list.param[index].num;
  }

  return NULL;
}

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_get_plmn_fullname_index()                        **
 **                                                                        **
 ** Description: Search the list of available PLMNs for the index of the   **
 **      PLMN identifier given by the specified fullname           **
 **                                                                        **
 ** Inputs:  plmn:      The full name of the PLMN                  **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the PLMN, if found in the     **
 **             list of available PLMNs; -1 otherwise.     **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int IdleMode_get_plmn_fullname_index(const char *plmn)
{
  int index;

  /* Get the index of the PLMN identifier with specified full name */
  for (index = 0; index < _emm_plmn_list.n_plmns; index++) {
    if ( strncmp(plmn, _emm_plmn_list.param[index].fullname,
                 NET_FORMAT_LONG_SIZE) != 0 ) {
      continue;
    }

    return (index);
  }

  return (-1);
}

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_get_plmn_shortname_index()                       **
 **                                                                        **
 ** Description: Search the list of available PLMNs for the index of the   **
 **      PLMN identifier given by the specified shortname          **
 **                                                                        **
 ** Inputs:  plmn:      The short name of the PLMN                 **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the PLMN, if found in the     **
 **             list of available PLMNs; -1 otherwise.     **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int IdleMode_get_plmn_shortname_index(const char *plmn)
{
  int index;

  /* Get the index of the PLMN identifier with specified short name */
  for (index = 0; index < _emm_plmn_list.n_plmns; index++) {
    if ( !strncmp(plmn, _emm_plmn_list.param[index].shortname,
                  NET_FORMAT_SHORT_SIZE) ) {
      continue;
    }

    return (index);
  }

  return (-1);
}

/****************************************************************************
 **                                                                        **
 ** Name:    IdleMode_get_plmn_id_index()                              **
 **                                                                        **
 ** Description: Search the list of available PLMNs for the index of the   **
 **      PLMN identifier given by the specified numeric identifier **
 **                                                                        **
 ** Inputs:  plmn:      The numeric identifier of the PLMN         **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the PLMN, if found in the     **
 **             list of available PLMNs; -1 otherwise.     **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int IdleMode_get_plmn_id_index(const char *plmn)
{
  int index;

  /* Get the index of the PLMN identifier with specified numeric identifier */
  for (index = 0; index < _emm_plmn_list.n_plmns; index++) {
    if ( !strncmp(plmn, _emm_plmn_list.param[index].num,
                  NET_FORMAT_LONG_SIZE) ) {
      continue;
    }

    return (index);
  }

  return (-1);
}

/*
 *---------------------------------------------------------------------------
 *              Idle mode EMM procedures
 *---------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_initialize()                                     **
 **                                                                        **
 ** Description: Initialize the ordered list of available PLMNs candidate  **
 **      to PLMN selection procedure.                              **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    _emm_plmn_list                             **
 **                                                                        **
 ***************************************************************************/
int emm_proc_initialize(void)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc;
  int i;

  if (!_emm_data.usim_is_valid) {
    /* The USIM application is not present or not valid */
    LOG_TRACE(WARNING, "EMM-IDLE  - USIM is not valid");
    emm_sap.primitive = EMMREG_NO_IMSI;
  } else {
    /* The highest priority is given to either the "equivalent PLMNs"
     * if available, or the last registered PLMN */
    if (_emm_data.nvdata.eplmn.n_plmns > 0) {
      for (i=0; i < _emm_data.nvdata.eplmn.n_plmns; i++) {
        _emm_plmn_list.plmn[_emm_plmn_list.n_plmns++] =
          &_emm_data.nvdata.eplmn.plmn[i];
      }
    } else if ( PLMN_IS_VALID(_emm_data.nvdata.rplmn) ) {
      _emm_plmn_list.plmn[_emm_plmn_list.n_plmns++] =
        &_emm_data.nvdata.rplmn;
    }

    /* Update the index of the HPLMN or EHPLM of highest priority.
     * When switched on, the UE will try to automatically register
     * to each previous PLMN within the ordered list of available
     * PLMNs regardless of the network selection mode of operation */
    _emm_plmn_list.hplmn = _emm_plmn_list.n_plmns - 1;
    // LG_emm_plmn_list.hplmn = _emm_plmn_list.n_plmns;

    /* Add the highest priority PLMN in the list of "equivalent HPLMNs"
       if present and not empty, or the HPLMN derived from the IMSI */
    if (_emm_data.ehplmn.n_plmns > 0) {
      _emm_plmn_list.plmn[_emm_plmn_list.n_plmns++] =
        &_emm_data.ehplmn.plmn[0];
    } else {
      _emm_plmn_list.plmn[_emm_plmn_list.n_plmns++] = &_emm_data.hplmn;
    }

    /* Each PLMN/access technology combination in the "User
     * Controlled PLMN Selector with Access Technology" */
    for (i=0; i < _emm_data.plmn.n_plmns; i++) {
      _emm_plmn_list.plmn[_emm_plmn_list.n_plmns++] =
        &_emm_data.plmn.plmn[i];
    }

    /* Each PLMN/access technology combination in the "Operator
     * Controlled PLMN Selector with Access Technology" */
    for (i=0; i < _emm_data.oplmn.n_plmns; i++) {
      _emm_plmn_list.plmn[_emm_plmn_list.n_plmns++] =
        &_emm_data.oplmn.plmn[i];
    }

    /* Other PLMN/access technology combinations with received
     * high quality signal in random order */

    /* Other PLMN/access technology combinations in order of
     * decreasing signal quality */

    /* TODO: Schedule periodic network selection attemps (hpplmn timer) */

    /* Initialize the PLMNs' parameters */
    for (i=0; i < _emm_plmn_list.n_plmns; i++) {
      struct plmn_param_t *plmn = &(_emm_plmn_list.param[i]);
      int id = _IldlMode_get_opnn_id(_emm_plmn_list.plmn[i]);

      if (id < 0) {
        plmn->fullname[0] = '\0';
        plmn->shortname[0] = '\0';
      } else {
        strncpy(plmn->fullname, _emm_data.opnn[id].fullname,
                NET_FORMAT_LONG_SIZE);
        strncpy(plmn->shortname, _emm_data.opnn[id].shortname,
                NET_FORMAT_SHORT_SIZE);
      }

      (void)_IdleMode_plmn_str(plmn->num, _emm_plmn_list.plmn[i]);
      plmn->stat = NET_OPER_UNKNOWN;
      plmn->tac = 0;
      plmn->ci = 0;
      plmn->rat = NET_ACCESS_UNAVAILABLE;
    }

    LOG_TRACE(INFO, "EMM-IDLE  - %d PLMNs available for network selection",
              _emm_plmn_list.n_plmns);

    /* Notify EMM that PLMN selection procedure has to be executed */
    emm_sap.primitive = EMMREG_REGISTER_REQ;
    emm_sap.u.emm_reg.u.regist.index = 0;
  }

  rc = emm_sap_send(&emm_sap);

  LOG_FUNC_RETURN(rc);

  /* TODO: Update the list of PLMNs upon receiving AS system information */
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_plmn_selection()                                 **
 **                                                                        **
 ** Description: Performs the network selection procedure when the UE is   **
 **      swicthed on.                                              **
 **                                                                        **
 **      The MS shall select the registered PLMN or equivalent     **
 **      PLMN (if it is available) using all access technologies   **
 **      that the MS is capable.                                   **
 **      If there is no registered PLMN, or if registration is     **
 **      not possible due to the PLMN being unavailable or regis-  **
 **      tration failure, the MS performs the automatic or the ma- **
 **      nual procedure depending on its PLMN selection operating  **
 **      mode.                                                     **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    _emm_plmn_list, _emm_data                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_plmn_list.index                       **
 **                                                                        **
 ***************************************************************************/
int emm_proc_plmn_selection(int index)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  if (_emm_data.plmn_mode != EMM_DATA_PLMN_AUTO) {
    /*
     * Manual or manual/automatic mode of operation
     * --------------------------------------------
     */
    if (index >= _emm_plmn_list.hplmn) {
      /*
       * Selection of the last registered or equivalent PLMNs failed
       */
      if (_emm_data.plmn_index < 0) {
        /*
         * The user did not select any PLMN yet; display the ordered
         * list of available PLMNs to the user
         */
        index = -1;
        rc = emm_proc_network_notify(_emm_plmn_list.hplmn);

        if (rc != RETURNok) {
          LOG_TRACE(WARNING, "EMM-IDLE  - Failed to notify "
                    "network list update");
        }
      } else {
        /*
         * Try to register to the PLMN manually selected by the user
         */
        index = _emm_data.plmn_index;
      }
    }
  }

  if ( !(index < 0) ) {
    /*
     * Search for a suitable cell of the currently selected PLMN:
     * It can be the last registered or one of the equivalent PLMNs
     * if available, or the PLMN selected by the user in manual mode,
     * or any other PLMN in the ordered list of available PLMNs in
     * automatic mode.
     */
    _emm_plmn_list.index = index;
    rc = _IdleMode_get_suitable_cell(index);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_plmn_end()                                       **
 **                                                                        **
 ** Description: Completes the network selection procedure or triggers a   **
 **      new one until a suitable or acceptable cell of the chosen **
 **      PLMN is found.                                            **
 **                                                                        **
 **      If a suitable cell of the selected PLMN has been found,   **
 **      that cell is chosen to camp on and provide available ser- **
 **      vices, and tunes to its control channel.                  **
 **      The MS will then register its presence in the registra-   **
 **      tion area of the chosen cell, if it is capable of servi-  **
 **      ces which require registration. As outcome of a success-  **
 **      ful Location Registration the selected PLMN becomes the   **
 **      registered PLMN.                                          **
 **                                                                        **
 **              When no suitable cell can be found, the MS is unable to   **
 **      obtain normal service from a PLMN. Then the MS attempts   **
 **      to camp on an acceptable cell, irrespective of its PLMN   **
 **      identity, so that only emergency calls can be made.       **
 **                                                                        **
 ** Inputs:  found:     TRUE if a suitable cell of the chosen      **
 **             PLMN has been found; FALSE otherwise.      **
 **      tac:       The code of the location/tracking area the **
 **             chosen PLMN belongs to                     **
 **      ci:        The identifier of the cell                 **
 **      rat:       The radio access technology supported by   **
 **             the cell                                   **
 **      Others:    _emm_plmn_list, _emm_data                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    _emm_plmn_list, _emm_data                  **
 **                                                                        **
 ***************************************************************************/
int emm_proc_plmn_selection_end(int found, tac_t tac, ci_t ci, AcT_t rat)
{
  LOG_FUNC_IN;

  emm_sap_t emm_sap;
  int rc = RETURNerror;
  int index = _emm_plmn_list.index;
  int select_next_plmn = FALSE;

  LOG_TRACE(INFO, "EMM-IDLE  - %s cell found for PLMN %d in %s mode",
            (found)? "One" : "No", index,
            (_emm_data.plmn_mode == EMM_DATA_PLMN_AUTO)? "Automatic" :
            (_emm_data.plmn_mode == EMM_DATA_PLMN_MANUAL)? "Manual" :
            "Automatic/manual");

  if (found) {
    int is_forbidden = FALSE;

    /* Select the PLMN of which a suitable cell has been found */
    _emm_data.splmn = *_emm_plmn_list.plmn[index];

    /* Update the selected PLMN's parameters */
    _emm_plmn_list.param[index].tac = tac;
    _emm_plmn_list.param[index].ci = ci;
    _emm_plmn_list.param[index].rat = rat;

    /* Update the location data and notify EMM that data have changed */
    rc = emm_proc_location_notify(tac, ci , rat);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "EMM-IDLE  - Failed to notify location update");
    }

    if (_emm_data.plmn_mode == EMM_DATA_PLMN_AUTO) {
      /*
       * Automatic mode of operation
       * ---------------------------
       */
      int i;

      /* Check if the selected PLMN is in the forbidden list */
      for (i = 0; i < _emm_data.fplmn.n_plmns; i++) {
        if (PLMNS_ARE_EQUAL(_emm_data.splmn, _emm_data.fplmn.plmn[i])) {
          is_forbidden = TRUE;
          break;
        }
      }

      if (!is_forbidden) {
        for (i = 0; i < _emm_data.fplmn_gprs.n_plmns; i++) {
          if (PLMNS_ARE_EQUAL(_emm_data.splmn,
                              _emm_data.fplmn_gprs.plmn[i])) {
            is_forbidden = TRUE;
            break;
          }
        }
      }

      /* Check if the selected PLMN belongs to a forbidden
       * tracking area */
      tai_t tai;
      tai.plmn = _emm_data.splmn;
      tai.tac = tac;

      if (!is_forbidden) {
        for (i = 0; i < _emm_data.ftai.n_tais; i++) {
          if (TAIS_ARE_EQUAL(tai, _emm_data.ftai.tai[i])) {
            is_forbidden = TRUE;
            break;
          }
        }
      }

      if (!is_forbidden) {
        for (i = 0; i < _emm_data.ftai_roaming.n_tais; i++) {
          if (TAIS_ARE_EQUAL(tai, _emm_data.ftai_roaming.tai[i])) {
            is_forbidden = TRUE;
            break;
          }
        }
      }
    }

    if (is_forbidden) {
      /* The selected cell is known not to be able to provide normal
       * service */
      LOG_TRACE(INFO, "EMM-IDLE  - UE may camp on this acceptable cell ",
                "for limited services");

      /* Save the index of the first forbidden PLMN */
      if (_emm_plmn_list.fplmn < 0) {
        _emm_plmn_list.fplmn = index;
      }

      _emm_plmn_list.param[index].stat = NET_OPER_FORBIDDEN;
    } else {
      /* A suitable cell has been found and the PLMN or tracking area
       * is not in the forbidden list */
      LOG_TRACE(INFO, "EMM-IDLE  - UE may camp on this suitable cell ",
                "for normal services");
      _emm_plmn_list.fplmn = -1;
      _emm_plmn_list.param[index].stat = NET_OPER_CURRENT;
      emm_sap.primitive = EMMREG_REGISTER_CNF;
    }

    /* Duplicate the new selected PLMN at the end of the ordered list */
    _emm_plmn_list.plmn[_emm_plmn_list.n_plmns] = &_emm_data.splmn;
  }

  else if (_emm_data.plmn_mode == EMM_DATA_PLMN_AUTO) {
    /*
     * Automatic mode of operation
     * ---------------------------
     * No suitable cell of the chosen PLMN has been found;
     * Try to select the next PLMN in the ordered list of available PLMNs
     */
    index += 1;
    select_next_plmn = TRUE;

    /* Bypass the previously selected PLMN */
    if (index == _emm_plmn_list.splmn) {
      index += 1;
    }
  }

  else if (_emm_data.plmn_index < 0) {
    /*
     * Manual or manual/automatic mode of operation
     * --------------------------------------------
     * Attempt to automatically find a suitable cell of the last
     * registered or equivalent PLMNs is ongoing
     */
    index += 1;
    select_next_plmn = TRUE;
  }

  else if (_emm_data.plmn_mode == EMM_DATA_PLMN_MANUAL) {
    /*
     * Manual mode of operation
     * ------------------------
     * No suitable cell of the PLMN selected by the user has been found
     */
    emm_sap.primitive = EMMREG_NO_CELL;
  }

  else {
    /*
     * Manual/automatic mode of operation
     * --------------------------------------------
     * Attempt to find a suitable cell of the PLMN selected by the user
     * failed; Try to automatically select another PLMN
     */
    _emm_data.plmn_mode = EMM_DATA_PLMN_AUTO;
    index = _emm_plmn_list.hplmn;
    select_next_plmn = TRUE;
  }

  /*
   * Force an attempt to register to the next PLMN
   */
  if (select_next_plmn) {
    int last_plmn_index = _emm_plmn_list.n_plmns;

    if (_emm_plmn_list.splmn != -1) {
      /* The last attempt was to register the previously selected PLMN */
      last_plmn_index += 1;
    }

    if (index < last_plmn_index) {
      /* Try to select the next PLMN in the list of available PLMNs */
      _emm_plmn_list.index = index;
      rc = emm_proc_plmn_selection(index);
    } else {
      /* No suitable cell of any PLMN within the ordered list
       * of available PLMNs has been found */
      select_next_plmn = FALSE;
      emm_sap.primitive = EMMREG_NO_CELL;
    }
  }

  /*
   * Or terminate the PLMN selection procedure
   */
  if (!select_next_plmn) {
    if (!(_emm_plmn_list.fplmn) < 0) { // FIXME this comparison makes no sense (bool < 0)
      /* There were one or more PLMNs which were available and allowable,
       * but an LR failure made registration on those PLMNs unsuccessful
       * or an entry in any of the forbidden area lists prevented a
       * registration attempt; select the first such PLMN and enters a
       * limited service state. */
      index = _emm_plmn_list.fplmn;
      _emm_plmn_list.fplmn = -1;
      emm_sap.primitive = EMMREG_REGISTER_REJ;
    }

    /* Update the availability indicator of the previously selected PLMN */
    if (_emm_plmn_list.splmn != -1) {
      _emm_plmn_list.param[_emm_plmn_list.splmn].stat = NET_OPER_UNKNOWN;
    }

    /* Update the index of the new selected PLMN */
    if (emm_sap.primitive != EMMREG_NO_CELL) {
      _emm_plmn_list.splmn = index;
    } else {
      _emm_plmn_list.splmn = -1;
    }

    /*
     * Notify EMM that PLMN selection procedure has completed
     */
    rc = emm_sap_send(&emm_sap);

    if (_emm_plmn_list.splmn != -1) {
      if (_emm_plmn_list.splmn == _emm_plmn_list.rplmn) {
        /* The selected PLMN is the registered PLMN */
        LOG_TRACE(INFO, "EMM-IDLE  - The selected PLMN is the registered PLMN");
        _emm_data.is_rplmn = TRUE;
      } else if (_emm_plmn_list.splmn < _emm_plmn_list.hplmn) {
        /* The selected PLMN is in the list of equivalent PLMNs */
        LOG_TRACE(INFO, "EMM-IDLE  - The selected PLMN is in the list of equivalent PLMNs");
        _emm_data.is_eplmn = TRUE;
      }

      /*
       * Notify EMM that an attach procedure has to be initiated
       * to register the presence of the UE to the selected PLMN
       */
      emm_sap.primitive = EMMREG_ATTACH_INIT;
      rc = emm_sap_send(&emm_sap);
    }
  }

  LOG_FUNC_RETURN (rc);
}

/*
 *---------------------------------------------------------------------------
 *             Network indication handlers
 *---------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_registration_notify()                            **
 **                                                                        **
 ** Description: Updates the current network registration status and noti- **
 **      fy the EMM main controller that network registration sta- **
 **      tus has changed.                                          **
 **                                                                        **
 ** Inputs:  status:    The new network registraton status         **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **          Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int emm_proc_registration_notify(Stat_t status)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  /* Update the network registration status */
  if (_emm_data.stat != status) {
    _emm_data.stat = status;
    /* Notify EMM that data has changed */
    rc = (*_emm_indication_notify)(1);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_location_notify()                                **
 **                                                                        **
 ** Description: Updates the current location information and notify the   **
 **      EMM main controller that location area has changed.       **
 **                                                                        **
 ** Inputs:  tac:       The code of the new location/tracking area **
 **          ci:        The identifier of the new serving cell     **
 **          rat:       The Radio Access Technology supported by   **
 **             the new serving cell                       **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **          Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int emm_proc_location_notify(tac_t tac, ci_t ci, AcT_t rat)
{
  LOG_FUNC_IN;

  int rc = RETURNok;

  /* Update the location information */
  if ( (_emm_data.tac != tac) ||
       (_emm_data.ci  != ci)  ||
       (_emm_data.rat != rat) ) {
    _emm_data.tac = tac;
    _emm_data.ci = ci;
    _emm_data.rat = rat;
    /* Notify EMM that data has changed */
    rc = (*_emm_indication_notify)(0);
  }

  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    emm_proc_network_notify()                                 **
 **                                                                        **
 ** Description: Updates the string representation of the list of opera-   **
 **      tors present in the network and notify the EMM main con-  **
 **      troller that the list has to be displayed to the user     **
 **      application.                                              **
 **                                                                        **
 ** Inputs:  index:     Index of the first operator to update      **
 **          Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **          Others:    _emm_data                                  **
 **                                                                        **
 ***************************************************************************/
int emm_proc_network_notify(int index)
{
  LOG_FUNC_IN;

  /* Update the list of operators present in the network */
  int size = IdleMode_update_plmn_list(index);
  /* Notify EMM that data has changed */
  int rc = (*_emm_indication_notify)(size);

  LOG_FUNC_RETURN (rc);
}


/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:    _IdleMain_plmn_str()                                      **
 **                                                                        **
 ** Description: Converts a PLMN identifier to a string representation in  **
 **      the numeric format                                        **
 **                                                                        **
 ** Inputs:  plmn:      The PLMN identifier to convert             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     plmn_str:  The PLMN identifier in numeric format      **
 **      Return:    The size in bytes of the string represen-  **
 **             tation of the PLMN identifier              **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _IdleMode_plmn_str(char *plmn_str, const plmn_t *plmn)
{
  char *p = plmn_str;

  if (plmn == NULL) {
    return 0;
  }

  if (plmn->MCCdigit1 != 0x0F) {
    sprintf(p++, "%u", plmn->MCCdigit1);
  }

  if (plmn->MCCdigit2 != 0x0F) {
    sprintf(p++, "%u", plmn->MCCdigit2);
  }

  if (plmn->MCCdigit3 != 0x0F) {
    sprintf(p++, "%u", plmn->MCCdigit3);
  }

  if (plmn->MNCdigit1 != 0x0F) {
    sprintf(p++, "%u", plmn->MNCdigit1);
  }

  if (plmn->MNCdigit2 != 0x0F) {
    sprintf(p++, "%u", plmn->MNCdigit2);
  }

  if (plmn->MNCdigit3 != 0x0F) {
    sprintf(p++, "%u", plmn->MNCdigit3);
  }

  return (p - plmn_str);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _IldlMode_get_opnn_id()                                   **
 **                                                                        **
 ** Description: Get the index of the specified PLMN in the list of opera- **
 **      tor network name records                                  **
 **                                                                        **
 ** Inputs:  plmn:      The PLMN identifier                        **
 **      Others:    _emm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The index of the PLMN if found in the list **
 **             of operator network name records;          **
 **             -1 otherwise;                              **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _IldlMode_get_opnn_id(const plmn_t *plmn)
{
  int i;

  for (i = 0; i < _emm_data.n_opnns; i++) {
    if (plmn->MCCdigit1 != _emm_data.opnn[i].plmn->MCCdigit1) {
      continue;
    }

    if (plmn->MCCdigit2 != _emm_data.opnn[i].plmn->MCCdigit2) {
      continue;
    }

    if (plmn->MCCdigit3 != _emm_data.opnn[i].plmn->MCCdigit3) {
      continue;
    }

    if (plmn->MNCdigit1 != _emm_data.opnn[i].plmn->MNCdigit1) {
      continue;
    }

    if (plmn->MNCdigit2 != _emm_data.opnn[i].plmn->MNCdigit2) {
      continue;
    }

    if (plmn->MNCdigit3 != _emm_data.opnn[i].plmn->MNCdigit3) {
      continue;
    }

    /* Found */
    return (i);
  }

  /* Not found */
  return (-1);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _IdleMode_get_suitable_cell()                             **
 **                                                                        **
 ** Description: Query the Access Stratum to search for a suitable cell    **
 **      that belongs to the selected PLMN.                        **
 **                                                                        **
 ** Inputs:  index:     Index of the selected PLMN in the ordered  **
 **             list of available PLMNs                    **
 **      Others:    _emm_plmn_list.plmn                        **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _IdleMode_get_suitable_cell(int index)
{
  emm_sap_t emm_sap;
  const plmn_t *plmn = _emm_plmn_list.plmn[index];

  LOG_TRACE(INFO, "EMM-IDLE  - Trying to search a suitable cell "
            "of PLMN %d in %s mode", index,
            (_emm_data.plmn_mode == EMM_DATA_PLMN_AUTO)? "Automatic" :
            (_emm_data.plmn_mode == EMM_DATA_PLMN_MANUAL)? "Manual" :
            "Automatic/manual");
  /*
   * Notify EMM-AS SAP that cell information related to the given
   * PLMN are requested from the Access-Stratum
   */
  emm_sap.primitive = EMMAS_CELL_INFO_REQ;
  emm_sap.u.emm_as.u.cell_info.plmnIDs.n_plmns = 1;
  emm_sap.u.emm_as.u.cell_info.plmnIDs.plmn[0] = *plmn;

  if (_emm_data.plmn_rat != NET_ACCESS_UNAVAILABLE) {
    emm_sap.u.emm_as.u.cell_info.rat = (1 << _emm_data.plmn_rat);
  } else {
    emm_sap.u.emm_as.u.cell_info.rat = NET_ACCESS_UNAVAILABLE;
  }

  return emm_sap_send(&emm_sap);
}

