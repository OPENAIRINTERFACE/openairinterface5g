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

#include "EmmCommon.h"
#include "emmData.h"


/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/* Type of network attachment */
typedef enum {
  EMM_ATTACH_TYPE_EPS = 0,
  EMM_ATTACH_TYPE_IMSI,
  EMM_ATTACH_TYPE_EMERGENCY,
  EMM_ATTACH_TYPE_RESERVED,
} emm_proc_attach_type_t;

/* Type of network detach */
typedef enum {
  EMM_DETACH_TYPE_EPS = 0,
  EMM_DETACH_TYPE_IMSI,
  EMM_DETACH_TYPE_EPS_IMSI,
  EMM_DETACH_TYPE_REATTACH,
  EMM_DETACH_TYPE_NOT_REATTACH,
  EMM_DETACH_TYPE_RESERVED,
} emm_proc_detach_type_t;

/* Type of requested identity */
typedef enum {
  EMM_IDENT_TYPE_NOT_AVAILABLE = 0,
  EMM_IDENT_TYPE_IMSI,
  EMM_IDENT_TYPE_IMEI,
  EMM_IDENT_TYPE_IMEISV,
  EMM_IDENT_TYPE_TMSI
} emm_proc_identity_type_t;

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
int emm_proc_status(unsigned int ueid, int emm_cause);

/*
 *---------------------------------------------------------------------------
 *              Lower layer procedure
 *---------------------------------------------------------------------------
 */


/*
 *---------------------------------------------------------------------------
 *              UE's Idle mode procedure
 *---------------------------------------------------------------------------
 */


/*
 * --------------------------------------------------------------------------
 *              Attach procedure
 * --------------------------------------------------------------------------
 */


int emm_proc_attach_request(unsigned int ueid, emm_proc_attach_type_t type,
                            int native_ksi, int ksi, int native_guti, GUTI_t *guti, imsi_t *imsi,
                            imei_t *imei, tai_t *tai, int eea, int eia, int ucs2, int uea, int uia, int gea,
                            int umts_present, int gprs_present, const OctetString *esm_msg);
int emm_proc_attach_reject(unsigned int ueid, int emm_cause);
int emm_proc_attach_complete(unsigned int ueid, const OctetString *esm_msg);
int emm_proc_tracking_area_update_reject(unsigned int ueid, int emm_cause);

/*
 * --------------------------------------------------------------------------
 *              Detach procedure
 * --------------------------------------------------------------------------
 */

int emm_proc_detach(unsigned int ueid, emm_proc_detach_type_t type);
int emm_proc_detach_request(unsigned int ueid, emm_proc_detach_type_t type,
                            int switch_off, int native_ksi, int ksi, GUTI_t *guti, imsi_t *imsi,
                            imei_t *imei);

/*
 * --------------------------------------------------------------------------
 *              Identification procedure
 * --------------------------------------------------------------------------
 */
int emm_proc_identification(unsigned int                   ueid,
                            emm_data_context_t            *emm_ctx,
                            emm_proc_identity_type_t       type,
                            emm_common_success_callback_t  success,
                            emm_common_reject_callback_t   reject,
                            emm_common_failure_callback_t  failure);
int emm_proc_identification_complete(unsigned int ueid, const imsi_t *imsi,
                                     const imei_t *imei, uint32_t *tmsi);

/*
 * --------------------------------------------------------------------------
 *              Authentication procedure
 * --------------------------------------------------------------------------
 */

int emm_proc_authentication(void *ctx, unsigned int ueid, int ksi,
                            const OctetString *_rand, const OctetString *autn,
                            emm_common_success_callback_t success,
                            emm_common_reject_callback_t reject,
                            emm_common_failure_callback_t failure);
int emm_proc_authentication_complete(unsigned int ueid, int emm_cause,
                                     const OctetString *res);

int emm_attach_security(void *args);

/*
 * --------------------------------------------------------------------------
 *          Security mode control procedure
 * --------------------------------------------------------------------------
 */

int emm_proc_security_mode_control(unsigned int ueid, int ksi,
                                   int eea, int eia,int ucs2, int uea, int uia, int gea,
                                   int umts_present, int gprs_present,
                                   emm_common_success_callback_t success,
                                   emm_common_reject_callback_t reject,
                                   emm_common_failure_callback_t failure);
int emm_proc_security_mode_complete(unsigned int ueid);
int emm_proc_security_mode_reject(unsigned int ueid);

/*
 *---------------------------------------------------------------------------
 *             Network indication handlers
 *---------------------------------------------------------------------------
 */


#endif /* __EMM_PROC_H__*/
