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
Source      IdleMode.h

Version     0.1

Date        2012/10/23

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the functions used to get information from the list
        of available PLMNs locally maintained when the UE is in
        idle mode.

*****************************************************************************/
#ifndef __IDLEMODE_H__
#define __IDLEMODE_H__

#include "commonDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

typedef int (*IdleMode_callback_t) (int);

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void IdleMode_initialize(IdleMode_callback_t cb);

int IdleMode_get_nb_plmns(void);
int IdleMode_get_hplmn_index(void);
int IdleMode_get_rplmn_index(void);
int IdleMode_get_splmn_index(void);

int IdleMode_update_plmn_list(int index);

const char *IdleMode_get_plmn_fullname(const plmn_t *plmn, int index,
                                       size_t *len);
const char *IdleMode_get_plmn_shortname(const plmn_t *plmn, int index,
                                        size_t *len);
const char *IdleMode_get_plmn_id(const plmn_t *plmn, int index, size_t *len);

int IdleMode_get_plmn_fullname_index(const char *plmn);
int IdleMode_get_plmn_shortname_index(const char *plmn);
int IdleMode_get_plmn_id_index(const char *plmn);

#endif /* __IDLEMODE_H__*/
