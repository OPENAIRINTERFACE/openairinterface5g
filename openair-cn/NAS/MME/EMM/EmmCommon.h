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
Source      EmmCommon.h

Version     0.1

Date        2013/04/19

Product     NAS stack

Subsystem   EPS Mobility Management

Author      Frederic Maurel

Description Defines callback functions executed within EMM common procedures
        by the Non-Access Stratum running at the network side.

        Following EMM common procedures can always be initiated by the
        network whilst a NAS signalling connection exists:

        GUTI reallocation
        authentication
        security mode control
        identification
        EMM information

*****************************************************************************/
#ifndef __EMM_COMMON_H__
#define __EMM_COMMON_H__

/****************************************************************************/
/*********************  G L O B A L    C O N S T A N T S  *******************/
/****************************************************************************/

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * Type of EMM procedure callback functions
 * ----------------------------------------
 * EMM procedure to be executed under certain conditions, when an EMM common
 * procedure has been initiated by the ongoing EMM procedure.
 * - The EMM common procedure successfully completed
 * - The EMM common procedure failed or is rejected
 * - Lower layer failure occured before the EMM common procedure completion
 */
typedef int (*emm_common_success_callback_t)(void *);
typedef int (*emm_common_reject_callback_t) (void *);
typedef int (*emm_common_failure_callback_t)(void *);

/*
 * Type of EMM common procedure callback function
 * ----------------------------------------------
 * EMM common procedure to be executed when the ongoing EMM procedure is
 * aborted.
 */
typedef int (*emm_common_abort_callback_t)(void *);

/****************************************************************************/
/********************  G L O B A L    V A R I A B L E S  ********************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int emm_proc_common_initialize(unsigned int ueid,
                               emm_common_success_callback_t success,
                               emm_common_reject_callback_t reject,
                               emm_common_failure_callback_t failure,
                               emm_common_abort_callback_t abort,
                               void *args);

int emm_proc_common_success(unsigned int ueid);
int emm_proc_common_reject(unsigned int ueid);
int emm_proc_common_failure(unsigned int ueid);
int emm_proc_common_abort(unsigned int ueid);

void *emm_proc_common_get_args(unsigned int ueid);

#endif /* __EMM_COMMON_H__*/
