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
Source      esm_ebr_context.h

Version     0.1

Date        2013/05/28

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions used to handle EPS bearer contexts.

*****************************************************************************/
#ifndef __ESM_EBR_CONTEXT_H__
#define __ESM_EBR_CONTEXT_H__

#include "networkDef.h"
#include "user_defs.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/* Traffic flow template operation */
typedef enum {
  ESM_EBR_CONTEXT_TFT_CREATE,
  ESM_EBR_CONTEXT_TFT_DELETE,
  ESM_EBR_CONTEXT_TFT_ADD_PACKET,
  ESM_EBR_CONTEXT_TFT_REPLACE_PACKET,
  ESM_EBR_CONTEXT_TFT_DELETE_PACKET,
} esm_ebr_context_tft_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int esm_ebr_context_create(esm_data_t *esm_data, int ueid, int pid, int ebi, int is_default,
                           const network_qos_t *qos, const network_tft_t *tft);

int esm_ebr_context_release(nas_user_t *user, int ebi, int *pid, int *bid);

int esm_ebr_context_get_pid(esm_data_t *esm_data, int ebi);

int esm_ebr_context_check_tft(esm_data_t *esm_data, int pid, int ebi, const network_tft_t *tft,
                              esm_ebr_context_tft_t operation);


#endif /* __ESM_EBR_CONTEXT_H__ */
