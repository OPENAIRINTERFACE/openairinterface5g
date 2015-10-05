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
Source      nas_proc.h

Version     0.1

Date        2012/09/20

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Maurel

Description NAS procedure call manager

*****************************************************************************/
#ifndef __NAS_PROC_H__
#define __NAS_PROC_H__

#if defined(NAS_BUILT_IN_EPC)
# include "mme_config.h"
# include "emm_cnDef.h"
#endif

#include "commonDef.h"
#include "networkDef.h"

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


# if defined(NAS_BUILT_IN_EPC)
void nas_proc_initialize(mme_config_t *mme_config_p);
# else
void nas_proc_initialize(void);
# endif

void nas_proc_cleanup(void);

/*
 * --------------------------------------------------------------------------
 *          NAS procedures triggered by the user
 * --------------------------------------------------------------------------
 */


/*
 * --------------------------------------------------------------------------
 *      NAS procedures triggered by the network
 * --------------------------------------------------------------------------
 */



int nas_proc_establish_ind(uint32_t ueid, tac_t tac, const Byte_t *data,
                           uint32_t len);

int nas_proc_dl_transfer_cnf(uint32_t ueid);
int nas_proc_dl_transfer_rej(uint32_t ueid);
int nas_proc_ul_transfer_ind(uint32_t ueid, const Byte_t *data, uint32_t len);

/*
 * --------------------------------------------------------------------------
 *      NAS procedures triggered by the mme applicative layer
 * --------------------------------------------------------------------------
 */
#if defined(NAS_BUILT_IN_EPC)
int nas_proc_auth_param_res(emm_cn_auth_res_t *emm_cn_auth_res);
int nas_proc_auth_param_fail(emm_cn_auth_fail_t *emm_cn_auth_fail);
int nas_proc_deregister_ue(uint32_t ue_id);
int nas_proc_pdn_connectivity_res(nas_pdn_connectivity_rsp_t *nas_pdn_connectivity_rsp);
int nas_proc_pdn_connectivity_fail(nas_pdn_connectivity_fail_t *nas_pdn_connectivity_fail);
#endif

#endif /* __NAS_PROC_H__*/
