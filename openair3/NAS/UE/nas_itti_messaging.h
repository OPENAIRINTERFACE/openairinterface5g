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
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "assertions.h"
#include "intertask_interface.h"
#include "esm_proc.h"
#include "msc.h"

#ifndef NAS_ITTI_MESSAGING_H_
#define NAS_ITTI_MESSAGING_H_

# if (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE))
int nas_itti_plain_msg(
  const char *buffer,
  const nas_message_t *msg,
  const int lengthP,
  const int instance);

int nas_itti_protected_msg(
  const char *buffer,
  const nas_message_t *msg,
  const int lengthP,
  const int instance);
# endif


# if defined(NAS_BUILT_IN_UE)
int nas_itti_cell_info_req(const plmn_t plmnID, const Byte_t rat);

int nas_itti_nas_establish_req(as_cause_t cause, as_call_type_t type, as_stmsi_t s_tmsi, plmn_t plmnID, Byte_t *data_pP, uint32_t lengthP);

int nas_itti_ul_data_req(const uint32_t ue_idP, void *const data_pP, const uint32_t lengthP);

int nas_itti_rab_establish_rsp(const as_stmsi_t s_tmsi, const as_rab_id_t rabID, const nas_error_code_t errCode);
# endif
#endif /* NAS_ITTI_MESSAGING_H_ */
