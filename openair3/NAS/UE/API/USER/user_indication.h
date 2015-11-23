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

Source    user_indication.h

Version   0.1

Date    2012/10/25

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Defines functions which allow the user application to register
    procedures to be executed upon receiving asynchronous notifi-
    cation.

*****************************************************************************/
#ifndef __USER_IND_H__
#define __USER_IND_H__

#include "commonDef.h"
#include "networkDef.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * Type of notification that may be asynchronously received by the user
 */
typedef enum {
  USER_IND_REG = 0, /* Registration notification  */
  USER_IND_LOC, /* Location notification  */
  USER_IND_PLMN,  /* Network notification   */
  USER_IND_PDN, /* PDN connection notification  */
  USER_IND_MAX
} user_ind_t;

/*
 * Registration notification are received whenever there is a change in the
 * UE's network registration status in GERAN/UTRAN/E-UTRAN
 */
typedef struct {
  Stat_t status;  /* network registration status  */
} user_ind_reg_t;

/*
 * Location notification are received whenever there is a change in the
 * network serving cell in GERAN/UTRAN/E-UTRAN
 */
typedef struct {
  Stat_t status;  /* network registration status  */
  tac_t tac;    /* Location/Tracking area code  */
  ci_t ci;    /* GERAN/UTRAN/E-UTRAN cell ID  */
  AcT_t AcT;    /* Supported Access Technology  */
} user_ind_loc_t;

/*
 * PDN connection notification are received whenever the user or the network
 * has activated or desactivated a PDN connection
 */
typedef struct {
  uint8_t cid;    /* PDN connection identifier  */
  network_pdn_state_t status; /* PDN connection status  */
} user_ind_pdn_t;

/*
 * Structure of asynchronous notification received by the user
 */
typedef struct {
  union {
    user_ind_reg_t reg; /* Registration notification  */
    user_ind_loc_t loc; /* Location notification  */
    user_ind_pdn_t pdn; /* PDN connection notification  */
  } notification;
} user_indication_t;

/*
 * Type of procedure executed upon receiving registered notification
 */
typedef int (*user_ind_callback_t) (unsigned char, const void*, size_t);

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int user_ind_register(user_ind_t ind, unsigned char id, user_ind_callback_t cb);
int user_ind_deregister(user_ind_t ind);
int user_ind_notify(user_ind_t ind, const void* data, size_t size);

#endif /* __USER_IND_H__*/
