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

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/*
 * Type of EMM procedure callback function executed whenever data are
 * successfully delivered to the network
 */
typedef int (*lowerlayer_success_callback_t)(void *);

/*
 * Type of EMM procedure callback function executed when data are not
 * delivered to the network because a lower layer failure occurred
 */
typedef int (*lowerlayer_failure_callback_t)(int, void *);

/*
 * Type of EMM procedure callback function executed when NAS signalling
 * connection is released
 */
typedef int (*lowerlayer_release_callback_t)(void *);

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int lowerlayer_success(unsigned int ueid);
int lowerlayer_failure(unsigned int ueid);
int lowerlayer_establish(void);
int lowerlayer_release(int cause);

int lowerlayer_data_ind(unsigned int ueid, const OctetString *data);
int lowerlayer_data_req(unsigned int ueid, const OctetString *data);

#endif /* __LOWERLAYER_H__*/
