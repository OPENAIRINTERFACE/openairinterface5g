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

Source      emm_sap.h

Version     0.1

Date        2012/10/01

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EMM Service Access Points at which the EPS
        Mobility Management sublayer provides procedures for the
        control of security and mobility when the User Equipment
        is using the Evolved UTRA Network.

*****************************************************************************/
#ifndef __EMM_SAP_H__
#define __EMM_SAP_H__

#include "emmData.h"
#include "emm_regDef.h"
#include "emm_esmDef.h"
#include "emm_asDef.h"
#include "user_defs.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * EPS Mobility Management primitives
 * ----------------------------------
 * EMMREG-SAP provides registration services for location updating and
 * attach/detach procedures;
 * EMMESM-SAP provides interlayer services to the EPS Session Management
 * sublayer for service registration and activate/deactivate PDP context;
 * EMMAS-SAP provides services to the Access Stratum sublayer for NAS message
 * transfer;
 */
typedef enum {
  /* EMMREG-SAP */
  EMMREG_S1_ENABLED       = _EMMREG_S1_ENABLED,
  EMMREG_S1_DISABLED      = _EMMREG_S1_DISABLED,
  EMMREG_NO_IMSI          = _EMMREG_NO_IMSI,
  EMMREG_NO_CELL          = _EMMREG_NO_CELL,
  EMMREG_REGISTER_REQ     = _EMMREG_REGISTER_REQ,
  EMMREG_REGISTER_CNF     = _EMMREG_REGISTER_CNF,
  EMMREG_REGISTER_REJ     = _EMMREG_REGISTER_REJ,
  EMMREG_ATTACH_INIT      = _EMMREG_ATTACH_INIT,
  EMMREG_ATTACH_REQ       = _EMMREG_ATTACH_REQ,
  EMMREG_ATTACH_FAILED    = _EMMREG_ATTACH_FAILED,
  EMMREG_ATTACH_EXCEEDED  = _EMMREG_ATTACH_EXCEEDED,
  EMMREG_AUTH_REJ         = _EMMREG_AUTH_REJ,
  EMMREG_ATTACH_CNF       = _EMMREG_ATTACH_CNF,
  EMMREG_ATTACH_REJ       = _EMMREG_ATTACH_REJ,
  EMMREG_DETACH_INIT      = _EMMREG_DETACH_INIT,
  EMMREG_DETACH_REQ       = _EMMREG_DETACH_REQ,
  EMMREG_DETACH_FAILED    = _EMMREG_DETACH_FAILED,
  EMMREG_DETACH_CNF       = _EMMREG_DETACH_CNF,
  EMMREG_TAU_REQ          = _EMMREG_TAU_REQ,
  EMMREG_TAU_CNF          = _EMMREG_TAU_CNF,
  EMMREG_TAU_REJ          = _EMMREG_TAU_REJ,
  EMMREG_SERVICE_REQ      = _EMMREG_SERVICE_REQ,
  EMMREG_SERVICE_CNF      = _EMMREG_SERVICE_CNF,
  EMMREG_SERVICE_REJ      = _EMMREG_SERVICE_REJ,
  EMMREG_LOWERLAYER_SUCCESS   = _EMMREG_LOWERLAYER_SUCCESS,
  EMMREG_LOWERLAYER_FAILURE   = _EMMREG_LOWERLAYER_FAILURE,
  EMMREG_LOWERLAYER_RELEASE   = _EMMREG_LOWERLAYER_RELEASE,
  /* EMMESM-SAP */
  EMMESM_ESTABLISH_REQ    = _EMMESM_ESTABLISH_REQ,
  EMMESM_ESTABLISH_CNF    = _EMMESM_ESTABLISH_CNF,
  EMMESM_ESTABLISH_REJ    = _EMMESM_ESTABLISH_REJ,
  EMMESM_RELEASE_IND      = _EMMESM_RELEASE_IND,
  EMMESM_UNITDATA_REQ     = _EMMESM_UNITDATA_REQ,
  EMMESM_UNITDATA_IND     = _EMMESM_UNITDATA_IND,
  /* EMMAS-SAP */
  EMMAS_SECURITY_REQ      = _EMMAS_SECURITY_REQ,
  EMMAS_SECURITY_IND      = _EMMAS_SECURITY_IND,
  EMMAS_SECURITY_RES      = _EMMAS_SECURITY_RES,
  EMMAS_SECURITY_REJ      = _EMMAS_SECURITY_REJ,
  EMMAS_ESTABLISH_REQ     = _EMMAS_ESTABLISH_REQ,
  EMMAS_ESTABLISH_CNF     = _EMMAS_ESTABLISH_CNF,
  EMMAS_ESTABLISH_REJ     = _EMMAS_ESTABLISH_REJ,
  EMMAS_RELEASE_REQ       = _EMMAS_RELEASE_REQ,
  EMMAS_RELEASE_IND       = _EMMAS_RELEASE_IND,
  EMMAS_DATA_REQ          = _EMMAS_DATA_REQ,
  EMMAS_DATA_IND          = _EMMAS_DATA_IND,
  EMMAS_PAGE_IND          = _EMMAS_PAGE_IND,
  EMMAS_STATUS_IND        = _EMMAS_STATUS_IND,
  EMMAS_CELL_INFO_REQ     = _EMMAS_CELL_INFO_REQ,
  EMMAS_CELL_INFO_RES     = _EMMAS_CELL_INFO_RES,
  EMMAS_CELL_INFO_IND     = _EMMAS_CELL_INFO_IND,
} emm_primitive_t;

/*
 * Minimal identifier for EMM-SAP primitives
 */
#define EMMREG_PRIMITIVE_MIN    _EMMREG_START
#define EMMESM_PRIMITIVE_MIN    _EMMESM_START
#define EMMAS_PRIMITIVE_MIN     _EMMAS_START
#define EMMCN_PRIMITIVE_MIN     _EMMCN_START

/*
 * Maximal identifier for EMM-SAP primitives
 */
#define EMMREG_PRIMITIVE_MAX    _EMMREG_END
#define EMMESM_PRIMITIVE_MAX    _EMMESM_END
#define EMMAS_PRIMITIVE_MAX     _EMMAS_END
#define EMMCN_PRIMITIVE_MAX     _EMMCN_END

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * Structure of EPS Mobility Management primitive
 */
typedef struct emm_sap_s {
  emm_primitive_t primitive;
  union {
    emm_reg_t emm_reg;  /* EMMREG-SAP primitives    */
    emm_esm_t emm_esm;  /* EMMESM-SAP primitives    */
    emm_as_t  emm_as;   /* EMMAS-SAP primitives     */
  } u;
} emm_sap_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

void emm_sap_initialize(nas_user_t *user);

int emm_sap_send(nas_user_t *user, emm_sap_t *msg);

#endif /* __EMM_SAP_H__*/
