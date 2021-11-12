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

#include "COMMON/platform_types.h"
#include "common/ran_context.h"
#include "common/utils/LOG/log.h"
#include "NR_BCCH-BCH-Message.h"
#include "NR_ServingCellConfigCommon.h"
#include "NR_MIB.h"

void apply_macrlc_config(gNB_RRC_INST *rrc,
                         rrc_gNB_ue_context_t         *const ue_context_pP,
                         const protocol_ctxt_t        *const ctxt_pP ) {
  abort();
}

boolean_t sdap_gnb_data_req(protocol_ctxt_t *ctxt_p,
                            const srb_flag_t srb_flag,
                            const rb_id_t rb_id,
                            const mui_t mui,
                            const confirm_t confirm,
                            const sdu_size_t sdu_buffer_size,
                            unsigned char *const sdu_buffer,
                            const pdcp_transmission_mode_t pt_mode,
                            const uint32_t *sourceL2Id,
                            const uint32_t *destinationL2Id
                           ) {
abort();
}
