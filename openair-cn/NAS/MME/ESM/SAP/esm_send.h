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
Source      esm_send.h

Version     0.1

Date        2013/02/11

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions executed at the ESM Service Access
        Point to send EPS Session Management messages to the
        EPS Mobility Management sublayer.

*****************************************************************************/
#ifndef __ESM_SEND_H__
#define __ESM_SEND_H__

#include "EsmStatus.h"

#include "PdnConnectivityReject.h"
#include "PdnDisconnectReject.h"
#include "BearerResourceAllocationReject.h"
#include "BearerResourceModificationReject.h"

#include "ActivateDefaultEpsBearerContextRequest.h"
#include "ActivateDedicatedEpsBearerContextRequest.h"
#include "ModifyEpsBearerContextRequest.h"
#include "DeactivateEpsBearerContextRequest.h"

#include "EsmInformationRequest.h"

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
 * Functions executed by both the UE and the MME to send ESM messages
 * --------------------------------------------------------------------------
 */
int esm_send_status(int pti, int ebi, esm_status_msg *msg, int esm_cause);


/*
 * --------------------------------------------------------------------------
 * Functions executed by the MME to send ESM message to the UE
 * --------------------------------------------------------------------------
 */
/*
 * Transaction related messages
 * ----------------------------
 */
int esm_send_pdn_connectivity_reject(int pti, pdn_connectivity_reject_msg *msg,
                                     int esm_cause);

int esm_send_pdn_disconnect_reject(int pti, pdn_disconnect_reject_msg *msg,
                                   int esm_cause);

/*
 * Messages related to EPS bearer contexts
 * ---------------------------------------
 */
int esm_send_activate_default_eps_bearer_context_request(int pti, int ebi,
    activate_default_eps_bearer_context_request_msg *msg, const OctetString *apn,
    const ProtocolConfigurationOptions *pco, int pdn_type, const OctetString *pdn_addr,
    const EpsQualityOfService *qos, int esm_cause);

int esm_send_activate_dedicated_eps_bearer_context_request(int pti, int ebi,
    activate_dedicated_eps_bearer_context_request_msg *msg, int linked_ebi,
    const EpsQualityOfService *qos, PacketFilters *pkfs, int n_pkfs);

int esm_send_deactivate_eps_bearer_context_request(int pti, int ebi,
    deactivate_eps_bearer_context_request_msg *msg, int esm_cause);

#endif /* __ESM_SEND_H__*/
