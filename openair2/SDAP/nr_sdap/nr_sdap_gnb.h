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

#ifndef _NR_SDAP_GNB_
#define _NR_SDAP_GNB_

#include "openair2/COMMON/platform_types.h"
#include "common/utils/LOG/log.h"

#define SDAP_BITMASK_DC         (0x80)
#define SDAP_BITMASK_R          (0x40)
#define SDAP_BITMASK_QFI        (0x3F)
#define SDAP_HDR_UL_DATA_PDU    (1)
#define SDAP_HDR_UL_CTRL_PDU    (0)
#define SDAP_HDR_LENGTH         (1)

typedef struct nr_sdap_dl_hdr_s {
  uint8_t QFI:6;
  uint8_t RQI:1;
  uint8_t RDI:1;
} __attribute__((packed)) nr_sdap_dl_hdr_t;

typedef struct nr_sdap_ul_hdr_s {
  uint8_t QFI:6;
  uint8_t R:1;
  uint8_t DC:1;
} __attribute__((packed)) nr_sdap_ul_hdr_t;

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
                           );

void sdap_gnb_ul_header_handler(char sdap_gnb_ul_hdr);

#endif