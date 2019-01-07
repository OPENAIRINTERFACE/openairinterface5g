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
#include "esm_ebr.h"
#include "esmData.h"
#include "user_defs.h"

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

void esm_main_initialize(nas_user_t *user, esm_indication_callback_t cb);

void esm_main_cleanup(esm_data_t *esm_data);


/* User's getter for PDN connections and EPS bearer contexts */
int esm_main_get_nb_pdns_max(esm_data_t *esm_data);
int esm_main_get_nb_pdns(esm_data_t *esm_data);
int esm_main_has_emergency(esm_data_t *esm_data);
int esm_main_get_pdn_status(nas_user_t *user, int cid, int *state);
int esm_main_get_pdn(esm_data_t *esm_data, int cid, int *type, const char **apn, int *is_emergency,
                     int *is_active);
int esm_main_get_pdn_addr(esm_data_t *esm_data, int cid, const char **ipv4addr, const char **ipv6addr);


#endif /* __ESM_MAIN_H__*/
