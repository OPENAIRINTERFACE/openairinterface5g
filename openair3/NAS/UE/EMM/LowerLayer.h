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
Source      lowerlayer.h

Version     0.1

Date        2013/06/19

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines EMM procedures executed by the Non-Access Stratum
        upon receiving notifications from lower layers so that data
        transfer succeed or failed, or NAS signalling connection is
        released, or ESM unit data has been received from under layer,
        and to request ESM unit data transfer to under layer.

*****************************************************************************/
#ifndef __LOWERLAYER_H__
#define __LOWERLAYER_H__

#include "OctetString.h"
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
 *              Lower layer procedure
 *---------------------------------------------------------------------------
 */
int emm_proc_lowerlayer_initialize(lowerlayer_data_t *lowerlayer_data, lowerlayer_success_callback_t success,
                                   lowerlayer_failure_callback_t failure,
                                   lowerlayer_release_callback_t release,
                                   void *args);
int emm_proc_lowerlayer_success(lowerlayer_data_t *lowerlayer_data);
int emm_proc_lowerlayer_failure(lowerlayer_data_t *lowerlayer_data, bool is_initial);
int emm_proc_lowerlayer_release(lowerlayer_data_t *lowerlayer_data);


int lowerlayer_success(nas_user_t *user);
int lowerlayer_failure(nas_user_t *user);
int lowerlayer_establish(nas_user_t *user);
int lowerlayer_release(nas_user_t *user, int cause);

int lowerlayer_data_ind(nas_user_t *user, const OctetString *data);
int lowerlayer_data_req(nas_user_t *user, const OctetString *data);

#endif /* __LOWERLAYER_H__*/
