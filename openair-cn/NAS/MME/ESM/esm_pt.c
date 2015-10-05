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
Source      esm_pt.c

Version     0.1

Date        2013/01/03

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions used to handle ESM procedure transactions.

*****************************************************************************/

#include "esm_pt.h"


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


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/



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

