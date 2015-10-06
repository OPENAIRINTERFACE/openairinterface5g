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
Source      esm_main.h

Version     0.1

Date        2012/12/04

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the EPS Session Management procedure call manager,
        the main entry point for elementary ESM processing.

*****************************************************************************/

#ifndef __ESM_MAIN_H__
#define __ESM_MAIN_H__

#include "networkDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void esm_main_initialize(esm_indication_callback_t cb);

void esm_main_cleanup(void);


/* User's getter for PDN connections and EPS bearer contexts */
int esm_main_get_nb_pdns_max(void);
int esm_main_get_nb_pdns(void);
int esm_main_has_emergency(void);
int esm_main_get_pdn_status(int cid, int *state);
int esm_main_get_pdn(int cid, int *type, const char **apn, int *is_emergency,
                     int *is_active);
int esm_main_get_pdn_addr(int cid, const char **ipv4addr, const char **ipv6addr);


#endif /* __ESM_MAIN_H__*/
