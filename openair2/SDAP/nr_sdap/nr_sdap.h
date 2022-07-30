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

#ifndef _NR_SDAP_GNB_H_
#define _NR_SDAP_GNB_H_

#include "openair2/COMMON/platform_types.h"
#include "common/utils/LOG/log.h"
#include "nr_sdap_entity.h"

/*
 * TS 37.324 4.4 Functions
 * Transfer of user plane data
 * Downlink - gNB
 * Uplink   - nrUE
 */
bool sdap_data_req(protocol_ctxt_t *ctxt_p,
                   const srb_flag_t srb_flag,
                   const rb_id_t rb_id,
                   const mui_t mui,
                   const confirm_t confirm,
                   const sdu_size_t sdu_buffer_size,
                   unsigned char *const sdu_buffer,
                   const pdcp_transmission_mode_t pt_mode,
                   const uint32_t *sourceL2Id,
                   const uint32_t *destinationL2Id,
                   const uint8_t qfi,
                   const bool rqi,
                   const int pdusession_id);

/*
 * TS 37.324 4.4 Functions
 * Transfer of user plane data
 * Uplink   - gNB
 * Downlink - nrUE
 */
void sdap_data_ind(rb_id_t pdcp_entity,
                   int is_gnb,
                   int has_sdap,
                   int has_sdapULheader,
                   int pdusession_id,
                   int rnti,
                   char *buf,
                   int size
                  );

#endif
