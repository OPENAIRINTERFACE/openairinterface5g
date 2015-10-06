/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
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
typedef int (*esm_proc_procedure_t) (int, int, OctetString *, int);

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
int esm_proc_status(int is_standalone, int pti, OctetString *msg,
                    int sent_by_ue);


/*
 * --------------------------------------------------------------------------
 *          PDN connectivity procedure
 * --------------------------------------------------------------------------
 */
int esm_proc_pdn_connectivity(int cid, int to_define,
                              esm_proc_pdn_type_t pdn_type, const OctetString *apn, int is_emergency,
                              unsigned int *pti);
int esm_proc_pdn_connectivity_request(int is_standalone, int pti,
                                      OctetString *msg, int sent_by_ue);
int esm_proc_pdn_connectivity_accept(int pti, esm_proc_pdn_type_t pdn_type,
                                     const OctetString *pdn_address, const OctetString *apn, int *esm_cause);
int esm_proc_pdn_connectivity_reject(int pti, int *esm_cause);
int esm_proc_pdn_connectivity_complete(void);
int esm_proc_pdn_connectivity_failure(int is_pending);


/*
 * --------------------------------------------------------------------------
 *              PDN disconnect procedure
 * --------------------------------------------------------------------------
 */
int esm_proc_pdn_disconnect(int cid, unsigned int *pti, unsigned int *ebi);
int esm_proc_pdn_disconnect_request(int is_standalone, int pti,
                                    OctetString *msg, int sent_by_ue);

int esm_proc_pdn_disconnect_accept(int pti, int *esm_cause);
int esm_proc_pdn_disconnect_reject(int pti, int *esm_cause);

/*
 * --------------------------------------------------------------------------
 *      Default EPS bearer context activation procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_default_eps_bearer_context_request(int pid, int ebi,
    const esm_proc_qos_t *esm_qos, int *esm_cause);
int esm_proc_default_eps_bearer_context_complete(void);
int esm_proc_default_eps_bearer_context_failure(void);

int esm_proc_default_eps_bearer_context_accept(int is_standalone, int ebi,
    OctetString *msg, int ue_triggered);
int esm_proc_default_eps_bearer_context_reject(int is_standalone, int ebi,
    OctetString *msg, int ue_triggered);

/*
 * --------------------------------------------------------------------------
 *      Dedicated EPS bearer context activation procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_dedicated_eps_bearer_context_request(int ebi, int default_ebi,
    const esm_proc_qos_t *qos, const esm_proc_tft_t *tft, int *esm_cause);

int esm_proc_dedicated_eps_bearer_context_accept(int is_standalone, int ebi,
    OctetString *msg, int ue_triggered);
int esm_proc_dedicated_eps_bearer_context_reject(int is_standalone, int ebi,
    OctetString *msg, int ue_triggered);

/*
 * --------------------------------------------------------------------------
 *      EPS bearer context deactivation procedure
 * --------------------------------------------------------------------------
 */

int esm_proc_eps_bearer_context_deactivate(int is_local, int ebi, int *pid,
    int *bid);
int esm_proc_eps_bearer_context_deactivate_request(int ebi, int *esm_cause);

int esm_proc_eps_bearer_context_deactivate_accept(int is_standalone, int ebi,
    OctetString *msg, int ue_triggered);

#endif /* __ESM_PROC_H__*/
