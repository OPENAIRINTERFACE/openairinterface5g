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

Source    user_api.h

Version   0.1

Date    2012/02/28

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Implements the API used by the NAS layer running in the UE
    to send/receive message to/from the user application layer

*****************************************************************************/
#ifndef __USER_API_H__
#define __USER_API_H__

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

int user_api_initialize(const char* host, const char* port, const char* devname, const char* devparams);

int user_api_emm_callback(Stat_t stat, tac_t tac, ci_t ci, AcT_t AcT, const char* data, size_t size);
int user_api_esm_callback(int cid, network_pdn_state_t state);

int user_api_get_fd(void);
const void* user_api_get_data(int index);

int user_api_read_data(int fd);
int user_api_set_data(char *message);
int user_api_send_data(int fd, int length);
void user_api_close(int fd);

int user_api_decode_data(int length);
int user_api_encode_data(const void* data, int add_success_code);

#endif /* __USER_API_H__ */
