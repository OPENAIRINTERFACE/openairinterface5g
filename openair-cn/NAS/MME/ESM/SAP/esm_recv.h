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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
/*****************************************************************************
Source      esm_recv.h

Version     0.1

Date        2013/02/06

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions executed at the ESM Service Access
        Point upon receiving EPS Session Management messages
        from the EPS Mobility Management sublayer.

*****************************************************************************/
#ifndef __ESM_RECV_H__
#define __ESM_RECV_H__

#include "EsmStatus.h"
#include "emmData.h"


#include "PdnConnectivityRequest.h"
#include "PdnDisconnectRequest.h"
#include "BearerResourceAllocationRequest.h"
#include "BearerResourceModificationRequest.h"

#include "ActivateDefaultEpsBearerContextAccept.h"
#include "ActivateDefaultEpsBearerContextReject.h"
#include "ActivateDedicatedEpsBearerContextAccept.h"
#include "ActivateDedicatedEpsBearerContextReject.h"
#include "ModifyEpsBearerContextAccept.h"
#include "ModifyEpsBearerContextReject.h"
#include "DeactivateEpsBearerContextAccept.h"

#include "EsmInformationResponse.h"

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
 * --------------------------------------------------------------------------
 * Functions executed by both the UE and the MME upon receiving ESM messages
 * --------------------------------------------------------------------------
 */

int esm_recv_status(emm_data_context_t *ctx, int pti, int ebi,
                    const esm_status_msg *msg);

/*
 * --------------------------------------------------------------------------
 * Functions executed by the MME upon receiving ESM message from the UE
 * --------------------------------------------------------------------------
 */
/*
 * Transaction related messages
 * ----------------------------
 */
int esm_recv_pdn_connectivity_request(emm_data_context_t *ctx, int pti, int ebi,
                                      const pdn_connectivity_request_msg *msg, unsigned int *new_ebi, void *data);

int esm_recv_pdn_disconnect_request(emm_data_context_t *ctx, int pti, int ebi,
                                    const pdn_disconnect_request_msg *msg,
                                    unsigned int *linked_ebi);

/*
 * Messages related to EPS bearer contexts
 * ---------------------------------------
 */
int esm_recv_activate_default_eps_bearer_context_accept(emm_data_context_t *ctx,
    int pti, int ebi, const activate_default_eps_bearer_context_accept_msg *msg);

int esm_recv_activate_default_eps_bearer_context_reject(emm_data_context_t *ctx,
    int pti, int ebi, const activate_default_eps_bearer_context_reject_msg *msg);

int esm_recv_activate_dedicated_eps_bearer_context_accept(emm_data_context_t *ctx,
    int pti, int ebi, const activate_dedicated_eps_bearer_context_accept_msg *msg);

int esm_recv_activate_dedicated_eps_bearer_context_reject(emm_data_context_t *ctx,
    int pti, int ebi, const activate_dedicated_eps_bearer_context_reject_msg *msg);

int esm_recv_deactivate_eps_bearer_context_accept(emm_data_context_t *ctx, int pti,
    int ebi, const deactivate_eps_bearer_context_accept_msg *msg);

#endif /* __ESM_RECV_H__*/
