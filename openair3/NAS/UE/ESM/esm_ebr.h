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
Source      esm_ebr.h

Version     0.1

Date        2013/01/29

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions used to handle state of EPS bearer contexts
        and manage ESM messages re-transmission.

*****************************************************************************/
#ifndef __ESM_EBR_H__
#define __ESM_EBR_H__

#include "OctetString.h"

#include "networkDef.h"
#include "esmData.h"

#include "nas_timer.h"
#include "user_defs.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Unassigned EPS bearer identity value */
#define ESM_EBI_UNASSIGNED  (EPS_BEARER_IDENTITY_UNASSIGNED)

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * User notification callback, executed whenever a change of status with
 * respect of PDN connection or EPS bearer context is notified by the EPS
 * Session Management sublayer
 */
typedef int (*esm_indication_callback_t) (user_api_id_t *user_api_id, int, network_pdn_state_t);

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void esm_ebr_register_callback(esm_indication_callback_t cb);

int esm_ebr_is_reserved(esm_ebr_data_t *esm_ebr_data, int ebi);

esm_ebr_data_t *esm_ebr_initialize(void);
int esm_ebr_assign(esm_ebr_data_t *esm_ebr_data, int ebi, int cid, int default_ebr);
int esm_ebr_release(esm_ebr_data_t *esm_ebr_data, int ebi);

int esm_ebr_set_status(user_api_id_t *user_api_id, esm_ebr_data_t *esm_ebr_data, int ebi, esm_ebr_state status, int ue_requested);
esm_ebr_state esm_ebr_get_status(esm_ebr_data_t *esm_ebr_data, int ebi);

int esm_ebr_is_not_in_use(esm_ebr_data_t *esm_ebr_data, int ebi);

#endif /* __ESM_EBR_H__*/
