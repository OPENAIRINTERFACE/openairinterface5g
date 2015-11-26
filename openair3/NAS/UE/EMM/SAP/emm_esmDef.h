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
Source      emm_esmDef.h

Version     0.1

Date        2012/10/16

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines the EMMESM Service Access Point that provides
        interlayer services to the EPS Session Management sublayer
        for service registration and activate/deactivate PDN
        connections.

*****************************************************************************/
#ifndef __EMM_ESMDEF_H__
#define __EMM_ESMDEF_H__

#include "OctetString.h"
#include "emmData.h"

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * EMMESM-SAP primitives
 */
typedef enum {
  _EMMESM_START = 100,
  _EMMESM_ESTABLISH_REQ,
  _EMMESM_ESTABLISH_CNF,
  _EMMESM_ESTABLISH_REJ,
  _EMMESM_RELEASE_IND,
  _EMMESM_UNITDATA_REQ,
  _EMMESM_UNITDATA_IND,
  _EMMESM_END
} emm_esm_primitive_t;

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * EMMESM primitive for connection establishment
 * ---------------------------------------------
 */
typedef struct {
  int is_emergency;   /* Indicates whether the PDN connection is established
             * for emergency bearer services only       */
  int is_attached;    /* Indicates whether the UE remains attached to the
             * network                  */
} emm_esm_establish_t;

/*
 * EMMESM primitive for data transfer
 * ----------------------------------
 */
typedef struct {
  OctetString msg;        /* ESM message to be transfered     */
} emm_esm_data_t;

/*
 * ---------------------------------
 * Structure of EMMESM-SAP primitive
 * ---------------------------------
 */
typedef struct {
  emm_esm_primitive_t primitive;
  unsigned int ueid;
  union {
    emm_esm_establish_t establish;
    emm_esm_data_t data;
  } u;
  /* TODO: complete emm_esm_t structure definition */
} emm_esm_t;

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

#endif /* __EMM_ESMDEF_H__*/
