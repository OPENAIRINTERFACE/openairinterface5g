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
Source      user_defs.h

Version     0.1

Date        2016/07/01

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Leroy

Description NAS type definition to manage a user equipment

*****************************************************************************/
#ifndef __USER_DEFS_H__
#define __USER_DEFS_H__

#include "nas_proc_defs.h"
#include "esmData.h"
#include "esm_pt_defs.h"
#include "EMM/emm_fsm_defs.h"
#include "EMM/emmData.h"
#include "EMM/Authentication.h"
#include "EMM/IdleMode_defs.h"
#include "EMM/LowerLayer_defs.h"
#include "API/USIM/usim_api.h"
#include "API/USER/user_api_defs.h"
#include "SecurityModeControl.h"
#include "userDef.h"
#include "at_response.h"

typedef struct {
  int ueid; /* UE lower layer identifier */
  proc_data_t proc;
  // Eps Session Management
  esm_data_t *esm_data; // ESM internal data (used within ESM only)
  esm_pt_data_t *esm_pt_data;
  esm_ebr_data_t *esm_ebr_data;  // EPS bearer contexts
  default_eps_bearer_context_data_t *default_eps_bearer_context_data;
  // Eps Mobility Management
  emm_fsm_state_t emm_fsm_status; // Current EPS Mobility Management status
  emm_data_t *emm_data; // EPS mobility management data
  emm_plmn_list_t *emm_plmn_list; // list of PLMN identities
  authentication_data_t *authentication_data;
  security_data_t *security_data; //Internal data used for security mode control procedure
  // Hardware persistent storage
  usim_data_t usim_data; // USIM application data
  const char *usim_data_store; // USIM application data filename
  user_nvdata_t *nas_user_nvdata; //UE parameters stored in the UE's non-volatile memory device
  const char *user_nvdata_store; //UE parameters stored in the UE's non-volatile memory device
  //
  nas_user_context_t *nas_user_context;
  at_response_t *at_response; // data structure returned to the user as the result of NAS procedure function call
  //
  user_at_commands_t *user_at_commands; //decoded data received from the user application layer
  user_api_id_t *user_api_id;
  lowerlayer_data_t *lowerlayer_data;
} nas_user_t;

#endif
