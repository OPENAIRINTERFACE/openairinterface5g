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
#include "openair2/SDAP/nr_sdap/nr_sdap.h"

extern char *baseNetAddress;

void capture_pdu_session_establishment_accept_msg(uint8_t *buffer, uint32_t msg_length)
{
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
  /* Supports the capture of only one QoS Rule, it should be changed for multiple QoS Rules */
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
  psea_msg.sess_ambr.length = *(buffer + offset++);
  offset += psea_msg.sess_ambr.length; /* Ommit the Seassion-AMBR */

  /* Optional Presence IEs */
  uint8_t psea_iei = *(buffer + offset++);

  while(offset < msg_length) {
    switch (psea_iei) {
      case IEI_5GSM_CAUSE: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received 5GSM Cause IEI\n");
        offset++;
        break;

      case IEI_PDU_ADDRESS:
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received PDU Address IE\n");
        psea_msg.pdu_addr_ie.pdu_length  = *(buffer + offset++);
        psea_msg.pdu_addr_ie.pdu_type    = *(buffer + offset++);

        if (psea_msg.pdu_addr_ie.pdu_type == PDU_SESSION_TYPE_IPV4) {
          psea_msg.pdu_addr_ie.pdu_addr_oct1 = *(buffer + offset++);
          psea_msg.pdu_addr_ie.pdu_addr_oct2 = *(buffer + offset++);
          psea_msg.pdu_addr_ie.pdu_addr_oct3 = *(buffer + offset++);
          psea_msg.pdu_addr_ie.pdu_addr_oct4 = *(buffer + offset++);
          nas_getparams();
          sprintf(baseNetAddress, "%d.%d", psea_msg.pdu_addr_ie.pdu_addr_oct1, psea_msg.pdu_addr_ie.pdu_addr_oct2);
          nas_config(1, psea_msg.pdu_addr_ie.pdu_addr_oct3, psea_msg.pdu_addr_ie.pdu_addr_oct4, "oaitun_ue");
          LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received UE IP: %d.%d.%d.%d\n",
                psea_msg.pdu_addr_ie.pdu_addr_oct1,
                psea_msg.pdu_addr_ie.pdu_addr_oct2,
                psea_msg.pdu_addr_ie.pdu_addr_oct3,
                psea_msg.pdu_addr_ie.pdu_addr_oct4);
        } else {
          offset += psea_msg.pdu_addr_ie.pdu_length;
        }

        psea_iei = *(buffer + offset++);
        break;

      case IEI_RQ_TIMER_VALUE: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received RQ timer value IE\n");
        offset++; /* TS 24.008 10.5.7.3 */
        psea_iei = *(buffer + offset++);
        break;

      case IEI_SNSSAI: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received S-NSSAI IE\n");
        uint8_t snssai_length = *(buffer + offset);
        offset += (snssai_length + sizeof(snssai_length));
        psea_iei = *(buffer + offset++);
        break;

      case IEI_ALWAYSON_PDU: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received Always-on PDU Session indication IE\n");
        offset++;
        psea_iei = *(buffer + offset++);
        break;

      case IEI_MAPPED_EPS: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received Mapped EPS bearer context IE\n");
        uint16_t mapped_eps_length = htons(*(uint16_t *)(buffer + offset));
        offset += mapped_eps_length;
        psea_iei = *(buffer + offset++);
        break;

      case IEI_EAP_MSG: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received EAP message IE\n");
        uint16_t eap_length = htons(*(uint16_t *)(buffer + offset));
        offset += (eap_length + sizeof(eap_length));
        psea_iei = *(buffer + offset++);
        break;

      case IEI_AUTH_QOS_DESC: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received Authorized QoS flow descriptions IE\n");
        psea_msg.qos_fd_ie.length = htons(*(uint16_t *)(buffer + offset));
        offset += (psea_msg.qos_fd_ie.length + sizeof(psea_msg.qos_fd_ie.length));
        psea_iei = *(buffer + offset++);
        break;

      case IEI_EXT_CONF_OPT: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received Extended protocol configuration options IE\n");
        psea_msg.ext_pp_ie.length = htons(*(uint16_t *)(buffer + offset));
        offset += (psea_msg.ext_pp_ie.length + sizeof(psea_msg.ext_pp_ie.length ));
        psea_iei = *(buffer + offset++);
        break;

      case IEI_DNN:
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received DNN IE\n");
        psea_msg.dnn_ie.dnn_length = *(buffer + offset++);
        char apn[APN_MAX_LEN];

        if(psea_msg.dnn_ie.dnn_length <= APN_MAX_LEN &&
           psea_msg.dnn_ie.dnn_length >= APN_MIN_LEN) {
          for (int i = 0 ; i < psea_msg.dnn_ie.dnn_length ; ++i)
            apn[i] = *(buffer + offset + i);
          LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - APN: %s\n", apn);
        } else
          LOG_E(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - DNN IE has invalid length\n");

        offset = msg_length;
        break;

      default:
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Not known IE\n");
        offset = msg_length;
        break;
    }
  }

  set_qfi_pduid(qos_rule.qfi, psea_msg.pdu_id);
  return;
}
