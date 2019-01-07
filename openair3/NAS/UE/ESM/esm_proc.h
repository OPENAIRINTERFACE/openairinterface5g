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
Source      esm_proc.h

Version     0.1

Date        2013/01/02

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the EPS Session Management procedures executed at
        the ESM Service Access Points.

*****************************************************************************/
#ifndef __ESM_PROC_H__
#define __ESM_PROC_H__

#include "networkDef.h"
#include "OctetString.h"
#include "emmData.h"
#include "ProtocolConfigurationOptions.h"
#include "user_defs.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * ESM retransmission timers
 * -------------------------
 */
#define T3482_DEFAULT_VALUE 8   /* PDN connectivity request  */
#define T3492_DEFAULT_VALUE 6   /* PDN disconnect request    */


/* Type of PDN address */
typedef enum {
  ESM_PDN_TYPE_IPV4 = NET_PDN_TYPE_IPV4,
  ESM_PDN_TYPE_IPV6 = NET_PDN_TYPE_IPV6,
  ESM_PDN_TYPE_IPV4V6 = NET_PDN_TYPE_IPV4V6
} esm_proc_pdn_type_t;

/* Type of PDN request */
typedef enum {
  ESM_PDN_REQUEST_INITIAL = 1,
  ESM_PDN_REQUEST_HANDOVER,
  ESM_PDN_REQUEST_EMERGENCY
} esm_proc_pdn_request_t;

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * Type of the ESM procedure callback executed when requested by the UE
 * or initiated by the network
 */
typedef int (*esm_proc_procedure_t) (nas_user_t *user, int, int, OctetString *, int);

/* EPS bearer level QoS parameters */
typedef network_qos_t esm_proc_qos_t;

/* Traffic Flow Template for packet filtering */
typedef network_tft_t esm_proc_tft_t;

typedef ProtocolConfigurationOptions esm_proc_pco_t;

/* PDN connection and EPS bearer context data */
typedef struct {
  OctetString apn;
  esm_proc_pdn_type_t pdn_type;
  OctetString pdn_addr;
  esm_proc_qos_t qos;
  esm_proc_tft_t tft;
  esm_proc_pco_t pco;
} esm_proc_data_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *              ESM status procedure
 * --------------------------------------------------------------------------
 */
int esm_proc_status_ind(int pti, int ebi, int *esm_cause);
int esm_proc_status(nas_user_t *user, int is_standalone, int pti, OctetString *msg,
                    int sent_by_ue);


/*
 * --------------------------------------------------------------------------
 *          PDN connectivity procedure
 * --------------------------------------------------------------------------
 */
int esm_proc_pdn_connectivity(nas_user_t *user, int cid, int to_define,
                              esm_proc_pdn_type_t pdn_type, const OctetString *apn, int is_emergency,
                              unsigned int *pti);
int esm_proc_pdn_connectivity_request(nas_user_t *user, int is_standalone, int pti,
                                      OctetString *msg, int sent_by_ue);
int esm_proc_pdn_connectivity_accept(nas_user_t *user, int pti, esm_proc_pdn_type_t pdn_type,
                                     const OctetString *pdn_address, const OctetString *apn, int *esm_cause);
int esm_proc_pdn_connectivity_reject(nas_user_t *user, int pti, int *esm_cause);
int esm_proc_pdn_connectivity_complete(nas_user_t *user);
int esm_proc_pdn_connectivity_failure(nas_user_t *user, int is_pending);


/*
 * --------------------------------------------------------------------------
 *              PDN disconnect procedure
 * --------------------------------------------------------------------------
 */
int esm_proc_pdn_disconnect(esm_data_t *esm_data, int cid, unsigned int *pti, unsigned int *ebi);
int esm_proc_pdn_disconnect_request(nas_user_t *user, int is_standalone, int pti,
                                    OctetString *msg, int sent_by_ue);

int esm_proc_pdn_disconnect_accept(esm_pt_data_t *esm_pt_data, int pti, int *esm_cause);
int esm_proc_pdn_disconnect_reject(nas_user_t *user, int pti, int *esm_cause);

/*
 * --------------------------------------------------------------------------
 *      Default EPS bearer context activation procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_default_eps_bearer_context_request(nas_user_t *user, int pid, int ebi,
    const esm_proc_qos_t *esm_qos, int *esm_cause);
int esm_proc_default_eps_bearer_context_complete(default_eps_bearer_context_data_t *default_eps_bearer_context_data);
int esm_proc_default_eps_bearer_context_failure(nas_user_t *user);

int esm_proc_default_eps_bearer_context_accept(nas_user_t *user, int is_standalone, int ebi,
    OctetString *msg, int ue_triggered);
int esm_proc_default_eps_bearer_context_reject(nas_user_t *user, int is_standalone, int ebi,
    OctetString *msg, int ue_triggered);

/*
 * --------------------------------------------------------------------------
 *      Dedicated EPS bearer context activation procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_dedicated_eps_bearer_context_request(nas_user_t *user, int ebi, int default_ebi,
    const esm_proc_qos_t *qos, const esm_proc_tft_t *tft, int *esm_cause);

int esm_proc_dedicated_eps_bearer_context_accept(nas_user_t *user, int is_standalone, int ebi,
    OctetString *msg, int ue_triggered);
int esm_proc_dedicated_eps_bearer_context_reject(nas_user_t *user, int is_standalone, int ebi,
    OctetString *msg, int ue_triggered);

/*
 * --------------------------------------------------------------------------
 *      EPS bearer context deactivation procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_eps_bearer_context_deactivate(nas_user_t *user, int is_local, int ebi, int *pid,
    int *bid);
int esm_proc_eps_bearer_context_deactivate_request(nas_user_t *user, int ebi, int *esm_cause);

int esm_proc_eps_bearer_context_deactivate_accept(nas_user_t *user, int is_standalone, int ebi,
    OctetString *msg, int ue_triggered);

#endif /* __ESM_PROC_H__*/
