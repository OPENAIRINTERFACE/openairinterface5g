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
#include "user_defs.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * User notification callback, executed whenever a change of data with
 * respect of network information (e.g. network registration and/or
 * location change, new PLMN becomes available) is notified by the
 * EPS Mobility Management sublayer
 */
typedef int (*emm_indication_callback_t) (user_api_id_t *user_api_id, Stat_t, tac_t, ci_t, AcT_t,
    const char*, size_t);

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void emm_main_initialize(nas_user_t *user, emm_indication_callback_t cb, const char *imei);

void emm_main_cleanup(nas_user_t *user);


/* User's getter of UE's identity */
const imsi_t *emm_main_get_imsi(emm_data_t *emm_data);

/* User's getter of the subscriber dialing number */
const msisdn_t *emm_main_get_msisdn(nas_user_t *user);

/* User's getter/setter for network selection */
int emm_main_set_plmn_selection_mode(nas_user_t *user, int mode, int format,
                                     const network_plmn_t *plmn, int rat);
int emm_main_get_plmn_selection_mode(emm_data_t *emm_data);
int emm_main_get_plmn_list(emm_plmn_list_t *emm_plmn_list, emm_data_t *emm_data, const char **plist);
const char *emm_main_get_selected_plmn(emm_plmn_list_t *emm_plmn_list, emm_data_t *emm_data, network_plmn_t *plmn, int format);

/* User's getter for network registration */
Stat_t emm_main_get_plmn_status(emm_data_t *emm_data);
tac_t emm_main_get_plmn_tac(emm_data_t *emm_data);
ci_t emm_main_get_plmn_ci(emm_data_t *emm_data);
AcT_t emm_main_get_plmn_rat(emm_data_t *emm_data);
const char *emm_main_get_registered_plmn(emm_plmn_list_t *emm_plmn_list, emm_data_t *emm_data, network_plmn_t *plmn, int format);

/* User's getter for network attachment */
int emm_main_is_attached(emm_data_t *emm_data);
int emm_main_is_emergency(emm_data_t *emm_data);


#endif /* __EMM_MAIN_H__*/
