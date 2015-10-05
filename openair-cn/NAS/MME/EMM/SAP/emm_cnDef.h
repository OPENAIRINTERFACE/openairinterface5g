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

Source      emm_cnDef.h

Version     0.1

Date        2013/12/05

Product     NAS stack

Subsystem   EPS Core Network

Author      Sebastien Roux, Lionel GAUTHIER

Description

*****************************************************************************/

#if defined(NAS_BUILT_IN_EPC)
#include "intertask_interface.h"
#endif

#ifndef __EMM_CNDEF_H__
#define __EMM_CNDEF_H__

typedef enum emmcn_primitive_s {
  _EMMCN_START = 400,
#if defined(NAS_BUILT_IN_EPC)
  _EMMCN_AUTHENTICATION_PARAM_RES,
  _EMMCN_AUTHENTICATION_PARAM_FAIL,
  _EMMCN_DEREGISTER_UE,
  _EMMCN_PDN_CONNECTIVITY_RES, // LG
  _EMMCN_PDN_CONNECTIVITY_FAIL,// LG
#endif
  _EMMCN_END
} emm_cn_primitive_t;

#if defined(NAS_BUILT_IN_EPC)
typedef nas_auth_param_rsp_t        emm_cn_auth_res_t;
typedef nas_auth_param_fail_t       emm_cn_auth_fail_t;
typedef nas_pdn_connectivity_rsp_t  emm_cn_pdn_res_t;
typedef nas_pdn_connectivity_fail_t emm_cn_pdn_fail_t;

typedef struct emm_cn_deregister_ue_s {
  uint32_t UEid;
} emm_cn_deregister_ue_t;

typedef struct emm_mme_ul_s {
  emm_cn_primitive_t primitive;
  union {
    emm_cn_auth_res_t       *auth_res;
    emm_cn_auth_fail_t      *auth_fail;
    emm_cn_deregister_ue_t   deregister;
    emm_cn_pdn_res_t        *emm_cn_pdn_res;
    emm_cn_pdn_fail_t       *emm_cn_pdn_fail;
  } u;
} emm_cn_t;
#endif

#endif /* __EMM_CNDEF_H__ */
