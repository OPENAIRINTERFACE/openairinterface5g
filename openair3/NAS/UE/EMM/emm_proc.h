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
Source      emm_proc.h

Version     0.1

Date        2012/10/16

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EPS Mobility Management procedures executed at
        the EMM Service Access Points.

*****************************************************************************/
#ifndef __EMM_PROC_H__
#define __EMM_PROC_H__

#include "commonDef.h"
#include "OctetString.h"
#include "LowerLayer.h"
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

/*
 *---------------------------------------------------------------------------
 *              EMM status procedure
 *---------------------------------------------------------------------------
 */
int emm_proc_status_ind(unsigned int ueid, int emm_cause);
int emm_proc_status(nas_user_t *user, int emm_cause);

/*
 *---------------------------------------------------------------------------
 *              UE's Idle mode procedure
 *---------------------------------------------------------------------------
 */
int emm_proc_initialize(nas_user_t *user);
int emm_proc_plmn_selection(nas_user_t *user, int index);
int emm_proc_plmn_selection_end(nas_user_t *user, int found, tac_t tac, ci_t ci, AcT_t rat);

/*
 * --------------------------------------------------------------------------
 *              Attach procedure
 * --------------------------------------------------------------------------
 */
int emm_proc_attach(nas_user_t *user, emm_proc_attach_type_t type);
int emm_proc_attach_request(void *args);
int emm_proc_attach_accept(nas_user_t *user, long T3412, long T3402, long T3423, int n_tais,
                           tai_t *tai, GUTI_t *guti, int n_eplmns, plmn_t *eplmn,
                           const OctetString *esm_msg);
int emm_proc_attach_reject(nas_user_t *user, int emm_cause, const OctetString *esm_msg);
int emm_proc_attach_complete(void *args);
int emm_proc_attach_failure(int is_initial, void *args);
int emm_proc_attach_release(void *args);
int emm_proc_attach_restart(nas_user_t *user);

int emm_proc_attach_set_emergency(emm_data_t *emm_data);
int emm_proc_attach_set_detach(void *user);



/*
 * --------------------------------------------------------------------------
 *              Detach procedure
 * --------------------------------------------------------------------------
 */
int emm_proc_detach(nas_user_t *user, emm_proc_detach_type_t type, int switch_off);
int emm_proc_detach_request(void *args);
int emm_proc_detach_accept(void *args);
int emm_proc_detach_failure(int is_initial, void *args);
int emm_proc_detach_release(void *args);


/*
 * --------------------------------------------------------------------------
 *              Identification procedure
 * --------------------------------------------------------------------------
 */
int emm_proc_identification_request(nas_user_t *user, emm_proc_identity_type_t type);


/*
 * --------------------------------------------------------------------------
 *              Authentication procedure
 * --------------------------------------------------------------------------
 */
int emm_proc_authentication_request(nas_user_t *user, int native_ksi, int ksi,
                                    const OctetString *rand, const OctetString *autn);
int emm_proc_authentication_reject(nas_user_t *user);
int emm_proc_authentication_delete(nas_user_t *user);


/*
 * --------------------------------------------------------------------------
 *          Security mode control procedure
 * --------------------------------------------------------------------------
 */
int emm_proc_security_mode_command(nas_user_t *user, int native_ksi, int ksi, int seea, int seia,
                                   int reea, int reia, int imeisv_request);

/*
 *---------------------------------------------------------------------------
 *             Network indication handlers
 *---------------------------------------------------------------------------
 */
int emm_proc_registration_notify(user_api_id_t *user_api_id, emm_data_t *emm_data, Stat_t status);
int emm_proc_location_notify(user_api_id_t *user_api_id, emm_data_t *emm_data, tac_t tac, ci_t ci, AcT_t rat);
int emm_proc_network_notify(emm_plmn_list_t *emm_plmn_list, user_api_id_t *user_api_id, emm_data_t *emm_data, int index);

#endif /* __EMM_PROC_H__*/
