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
Source      EmmRegisteredPlmnSearch.c

Version     0.1

Date        2012/10/03

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Implements the EPS Mobility Management procedures executed
        when the EMM-SAP is in EMM-REGISTERED.PLMN-SEARCH state.

        In EMM-REGISTERED.PLMN-SEARCH state, the UE is searching
        for PLMNs.

*****************************************************************************/


#include "emm_fsm.h"
#include "commonDef.h"
#include "nas_log.h"

#include <assert.h>

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
 ** Name:    EmmRegisteredPlmnSearch()                                 **
 **                                                                        **
 ** Description: Handles the behaviour of the UE while the EMM-SAP is in   **
 **      EMM-REGISTERED.PLMN-SEARCH state.                         **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmRegisteredPlmnSearch(const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  assert(emm_fsm_get_status() == EMM_REGISTERED_PLMN_SEARCH);

  /* TODO */

  LOG_FUNC_RETURN (RETURNok);
}

#if 0
/****************************************************************************
 **                                                                        **
 ** Name:    EmmRegisteredPlmnSearch_xxx()                             **
 **                                                                        **
 ** Description: Procedure executed when xxx                               **
 **      while the EMM-SAP is in EMM-REGISTERED.PLMN-SEARCH state. **
 **                                                                        **
 ** Inputs:  evt:       The received EMM-SAP event                 **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    emm_fsm_status                             **
 **                                                                        **
 ***************************************************************************/
int EmmRegisteredPlmnSearch_xxx(const emm_reg_t *evt)
{
  LOG_FUNC_IN;

  assert(emm_fsm_get_status() == EMM_REGISTERED_PLMN_SEARCH);

  /* TODO */

  LOG_FUNC_RETURN (RETURNok);
}
#endif

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

