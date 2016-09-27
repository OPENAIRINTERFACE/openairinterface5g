/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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
