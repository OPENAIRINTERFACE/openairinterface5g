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
Source      emm_main.h

Version     0.1

Date        2012/10/10

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EPS Mobility Management procedure call manager,
        the main entry point for elementary EMM processing.

*****************************************************************************/
#ifndef __EMM_MAIN_H__
#define __EMM_MAIN_H__

#include "commonDef.h"
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

void emm_main_initialize(emm_indication_callback_t cb, const char *imei);

void emm_main_cleanup(void);


/* User's getter of UE's identity */
const imsi_t *emm_main_get_imsi(void);

/* User's getter of the subscriber dialing number */
const msisdn_t *emm_main_get_msisdn(void);

/* User's getter/setter for network selection */
int emm_main_set_plmn_selection_mode(int mode, int format,
                                     const network_plmn_t *plmn, int rat);
int emm_main_get_plmn_selection_mode(void);
int emm_main_get_plmn_list(const char **plist);
const char *emm_main_get_selected_plmn(network_plmn_t *plmn, int format);

/* User's getter for network registration */
Stat_t emm_main_get_plmn_status(void);
tac_t emm_main_get_plmn_tac(void);
ci_t emm_main_get_plmn_ci(void);
AcT_t emm_main_get_plmn_rat(void);
const char *emm_main_get_registered_plmn(network_plmn_t *plmn, int format);

/* User's getter for network attachment */
int emm_main_is_attached(void);
int emm_main_is_emergency(void);


#endif /* __EMM_MAIN_H__*/
