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

int nas_itti_kenb_refresh_req(const Byte_t kenb[32]);

int nas_itti_cell_info_req(const plmn_t plmnID, const Byte_t rat, int user_id);

int nas_itti_nas_establish_req(as_cause_t cause, as_call_type_t type, as_stmsi_t s_tmsi, plmn_t plmnID, Byte_t *data_pP, uint32_t lengthP, int user_id);

int nas_itti_ul_data_req(const uint32_t ue_idP, void *const data_pP, const uint32_t lengthP, int user_id);

int nas_itti_rab_establish_rsp(const as_stmsi_t s_tmsi, const as_rab_id_t rabID, const nas_error_code_t errCode, int user_id);
# endif
#endif /* NAS_ITTI_MESSAGING_H_ */
