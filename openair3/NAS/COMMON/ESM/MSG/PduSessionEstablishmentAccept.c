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

#include "PduSessionEstablishmentAccept.h"
#include "common/utils/LOG/log.h"
#include "nr_nas_msg_sim.h"
#include "openair2/RRC/NAS/nas_config.h"

void capture_pdu_session_establishment_accept(uint8_t *buffer, uint32_t msg_length){
  uint8_t offset = 0;
  security_protected_nas_5gs_msg_t       sec_nas_hdr;
  security_protected_plain_nas_5gs_msg_t sec_nas_msg;
  pdu_session_establishment_accept_msg_t psea_msg;
  sec_nas_hdr.epd = *(buffer + (offset++));
  sec_nas_hdr.sht = *(buffer + (offset++));
  sec_nas_hdr.mac = htonl(*(uint32_t *)(buffer + offset));
  offset+=sizeof(sec_nas_hdr.mac);
  sec_nas_hdr.sqn = *(buffer + (offset++));
  sec_nas_msg.epd          = *(buffer + (offset++));
  sec_nas_msg.sht          = *(buffer + (offset++));
  sec_nas_msg.msg_type     = *(buffer + (offset++));
  sec_nas_msg.payload_type = *(buffer + (offset++));
  sec_nas_msg.payload_len  = htons(*(uint16_t *)(buffer + offset));
  offset+=sizeof(sec_nas_msg.payload_len);

  return;
}