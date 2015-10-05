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

Source      emm_recv.h

Version     0.1

Date        2013/01/30

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines functions executed at the EMMAS Service Access
        Point upon receiving EPS Mobility Management messages
        from the Access Stratum sublayer.

*****************************************************************************/
#ifndef __EMM_RECV_H__
#define __EMM_RECV_H__

#include "EmmStatus.h"

#include "DetachRequest.h"
#include "DetachAccept.h"


#include "AttachRequest.h"
#include "AttachComplete.h"
#include "TrackingAreaUpdateRequest.h"
#include "TrackingAreaUpdateComplete.h"
#include "ServiceRequest.h"
#include "ExtendedServiceRequest.h"
#include "GutiReallocationComplete.h"
#include "AuthenticationResponse.h"
#include "AuthenticationFailure.h"
#include "IdentityResponse.h"
#include "NASSecurityModeComplete.h"
#include "SecurityModeReject.h"
#include "UplinkNasTransport.h"

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
 * Functions executed by both the UE and the MME upon receiving EMM messages
 * --------------------------------------------------------------------------
 */
int emm_recv_status(unsigned int ueid, emm_status_msg *msg, int *emm_cause);

/*
 * --------------------------------------------------------------------------
 * Functions executed by the UE upon receiving EMM message from the network
 * --------------------------------------------------------------------------
 */


/*
 * --------------------------------------------------------------------------
 * Functions executed by the MME upon receiving EMM message from the UE
 * --------------------------------------------------------------------------
 */
int emm_recv_attach_request(unsigned int ueid, const attach_request_msg *msg,
                            int *emm_cause);
int emm_recv_attach_complete(unsigned int ueid, const attach_complete_msg *msg,
                             int *emm_cause);

int emm_recv_detach_request(unsigned int ueid, const detach_request_msg *msg,
                            int *emm_cause);

int emm_recv_tracking_area_update_request(unsigned int  ueid,
    const tracking_area_update_request_msg *msg,
    int *emm_cause);

int emm_recv_identity_response(unsigned int ueid, identity_response_msg *msg,
                               int *emm_cause);
int emm_recv_authentication_response(unsigned int ueid,
                                     authentication_response_msg *msg, int *emm_cause);
int emm_recv_authentication_failure(unsigned int ueid,
                                    authentication_failure_msg *msg, int *emm_cause);
int emm_recv_security_mode_complete(unsigned int ueid,
                                    security_mode_complete_msg *msg, int *emm_cause);
int emm_recv_security_mode_reject(unsigned int ueid,
                                  security_mode_reject_msg *msg, int *emm_cause);

#endif /* __EMM_RECV_H__*/
