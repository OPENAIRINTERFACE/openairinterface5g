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
  /* Mandatory Presence IEs */
  psea_msg.epd      = *(buffer + (offset++));
  psea_msg.pdu_id   = *(buffer + (offset++));
  psea_msg.pti      = *(buffer + (offset++));
  psea_msg.msg_type = *(buffer + (offset++));
  psea_msg.pdu_type = *(buffer + offset) & 0x0f;
  psea_msg.ssc_mode = (*(buffer + (offset++)) & 0xf0) >> 4;
  psea_msg.qos_rules.length = htons(*(uint16_t *)(buffer + offset));
  offset+=sizeof(psea_msg.qos_rules.length);
  /* Supports the capture of only one QoS Rule,
     it should be changed for multiple QoS Rules */
  qos_rule_t qos_rule;
  qos_rule.id     =  *(buffer + (offset++));
  qos_rule.length = htons(*(uint16_t *)(buffer + offset));
  offset+=sizeof(qos_rule.length);
  qos_rule.oc     = (*(buffer + offset) & 0xE0) >> 5;
  qos_rule.dqr    = (*(buffer + offset) & 0x10) >> 4;
  qos_rule.nb_pf  =  *(buffer + (offset++)) & 0x0F;

  if(qos_rule.nb_pf) {
    packet_filter_t pf;

    if(qos_rule.oc == ROC_CREATE_NEW_QOS_RULE ||
                      ROC_MODIFY_QOS_RULE_ADD_PF ||
                      ROC_MODIFY_QOS_RULE_REPLACE_PF) {
      pf.pf_type.type_1.pf_dir = (*(buffer + offset) & 0x30) >> 4;
      pf.pf_type.type_1.pf_id  =  *(buffer + offset++) & 0x0F;
      pf.pf_type.type_1.length =  *(buffer + offset++);
      offset += (qos_rule.nb_pf * pf.pf_type.type_1.length); /* Ommit the Packet filter List */
    } else if (qos_rule.oc == ROC_MODIFY_QOS_RULE_DELETE_PF) {
      offset += qos_rule.nb_pf;
    }
  }

  qos_rule.prcd = *(buffer + offset++);
  qos_rule.qfi  = *(buffer + offset++);

  return;
}