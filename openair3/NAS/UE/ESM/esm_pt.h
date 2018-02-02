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
Source      esm_pt.h

Version     0.1

Date        2013/01/03

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions used to handle ESM procedure transactions.

*****************************************************************************/
#ifndef __ESM_PT_H__
#define __ESM_PT_H__

#include "OctetString.h"
#include "nas_timer.h"
#include "user_defs.h"
#include "esm_pt_defs.h"

#include "ProcedureTransactionIdentity.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Unassigned procedure transaction identity value */
#define ESM_PT_UNASSIGNED   (PROCEDURE_TRANSACTION_IDENTITY_UNASSIGNED)

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int esm_pt_is_reserved(int pti);

esm_pt_data_t *esm_pt_initialize(void);

int esm_pt_assign(esm_pt_data_t *esm_pt_data);
int esm_pt_release(esm_pt_data_t *esm_pt_data, int pti);

int esm_pt_start_timer(nas_user_t *user, int pti, const OctetString *msg, long sec,
                       nas_timer_callback_t cb);
int esm_pt_stop_timer(esm_pt_data_t *esm_pt_data, int pti);

int esm_pt_set_status(esm_pt_data_t *esm_pt_data, int pti, esm_pt_state status);
esm_pt_state esm_pt_get_status(esm_pt_data_t *esm_pt_data, int pti);
int esm_pt_get_pending_pti(esm_pt_data_t *esm_pt_data, esm_pt_state status);

int esm_pt_is_not_in_use(esm_pt_data_t *esm_pt_data, int pti);

#endif /* __ESM_PT_H__*/
