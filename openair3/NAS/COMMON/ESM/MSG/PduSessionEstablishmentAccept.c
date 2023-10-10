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

static uint16_t getShort(uint8_t *input)
{
  uint16_t tmp16;
  memcpy(&tmp16, input, sizeof(tmp16));
  return htons(tmp16);
}

void capture_pdu_session_establishment_accept_msg(uint8_t *buffer, uint32_t msg_length)
{
  security_protected_nas_5gs_msg_t       sec_nas_hdr;
  security_protected_plain_nas_5gs_msg_t sec_nas_msg;
  pdu_session_establishment_accept_msg_t psea_msg;
  uint8_t *curPtr = buffer;
  sec_nas_hdr.epd = *curPtr++;
  sec_nas_hdr.sht = *curPtr++;
  uint32_t tmp;
  memcpy(&tmp, buffer, sizeof(tmp));
  sec_nas_hdr.mac = htonl(tmp);
  curPtr += sizeof(sec_nas_hdr.mac);
  sec_nas_hdr.sqn = *curPtr++;
  sec_nas_msg.epd = *curPtr++;
  sec_nas_msg.sht = *curPtr++;
  sec_nas_msg.msg_type = *curPtr++;
  sec_nas_msg.payload_type = *curPtr++;
  sec_nas_msg.payload_len = getShort(curPtr);
  curPtr += sizeof(sec_nas_msg.payload_len);
  /* Mandatory Presence IEs */
  psea_msg.epd = *curPtr++;
  psea_msg.pdu_id = *curPtr++;
  psea_msg.pti = *curPtr++;
  psea_msg.msg_type = *curPtr++;
  psea_msg.pdu_type = *curPtr & 0x0f;
  psea_msg.ssc_mode = (*curPtr++ & 0xf0) >> 4;
  psea_msg.qos_rules.length = getShort(curPtr);
  curPtr += sizeof(psea_msg.qos_rules.length);
  /* Supports the capture of only one QoS Rule, it should be changed for multiple QoS Rules */
  qos_rule_t qos_rule;
  qos_rule.id = *curPtr++;
  qos_rule.length = getShort(curPtr);
  curPtr += sizeof(qos_rule.length);
  qos_rule.oc = (*(curPtr)&0xE0) >> 5;
  qos_rule.dqr = (*(curPtr)&0x10) >> 4;
  qos_rule.nb_pf = *curPtr++ & 0x0F;

  if(qos_rule.nb_pf) {
    packet_filter_t pf;

    if(qos_rule.oc == ROC_CREATE_NEW_QOS_RULE ||
       qos_rule.oc == ROC_MODIFY_QOS_RULE_ADD_PF ||
       qos_rule.oc == ROC_MODIFY_QOS_RULE_REPLACE_PF) {
      pf.pf_type.type_1.pf_dir = (*curPtr & 0x30) >> 4;
      pf.pf_type.type_1.pf_id = *curPtr++ & 0x0F;
      pf.pf_type.type_1.length = *curPtr++;
      curPtr += (qos_rule.nb_pf * pf.pf_type.type_1.length); /* Ommit the Packet filter List */
    } else if (qos_rule.oc == ROC_MODIFY_QOS_RULE_DELETE_PF) {
      curPtr += qos_rule.nb_pf;
    }
  }

  qos_rule.prcd = *curPtr++;
  qos_rule.qfi = *curPtr++;
  psea_msg.sess_ambr.length = *curPtr++;
  curPtr += psea_msg.sess_ambr.length; /* Ommit the Seassion-AMBR */

  /* Optional Presence IEs */
  while (curPtr < buffer + msg_length) {
    uint8_t psea_iei = *curPtr++;
    switch (psea_iei) {
      case IEI_5GSM_CAUSE: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received 5GSM Cause IEI\n");
        curPtr++;
        break;

      case IEI_PDU_ADDRESS:
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received PDU Address IE\n");
        psea_msg.pdu_addr_ie.pdu_length = *curPtr++;
        psea_msg.pdu_addr_ie.pdu_type = *curPtr++;

        if (psea_msg.pdu_addr_ie.pdu_type == PDU_SESSION_TYPE_IPV4) {
          psea_msg.pdu_addr_ie.pdu_addr_oct1 = *curPtr++;
          psea_msg.pdu_addr_ie.pdu_addr_oct2 = *curPtr++;
          psea_msg.pdu_addr_ie.pdu_addr_oct3 = *curPtr++;
          psea_msg.pdu_addr_ie.pdu_addr_oct4 = *curPtr++;
          nas_getparams();
          sprintf(baseNetAddress, "%d.%d", psea_msg.pdu_addr_ie.pdu_addr_oct1, psea_msg.pdu_addr_ie.pdu_addr_oct2);
          nas_config(1, psea_msg.pdu_addr_ie.pdu_addr_oct3, psea_msg.pdu_addr_ie.pdu_addr_oct4, "oaitun_ue");
          LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received UE IP: %d.%d.%d.%d\n",
                psea_msg.pdu_addr_ie.pdu_addr_oct1,
                psea_msg.pdu_addr_ie.pdu_addr_oct2,
                psea_msg.pdu_addr_ie.pdu_addr_oct3,
                psea_msg.pdu_addr_ie.pdu_addr_oct4);
        } else {
          curPtr += psea_msg.pdu_addr_ie.pdu_length;
        }
        break;

      case IEI_RQ_TIMER_VALUE: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received RQ timer value IE\n");
        curPtr++; /* TS 24.008 10.5.7.3 */
        break;

      case IEI_SNSSAI: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received S-NSSAI IE\n");
        uint8_t snssai_length = *curPtr;
        curPtr += (snssai_length + sizeof(snssai_length));
        break;

      case IEI_ALWAYSON_PDU: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received Always-on PDU Session indication IE\n");
        curPtr++;
        break;

      case IEI_MAPPED_EPS: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received Mapped EPS bearer context IE\n");
        uint16_t mapped_eps_length = getShort(curPtr);
        curPtr += mapped_eps_length;
        break;

      case IEI_EAP_MSG: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received EAP message IE\n");
        uint16_t eap_length = getShort(curPtr);
        curPtr += (eap_length + sizeof(eap_length));
        break;

      case IEI_AUTH_QOS_DESC: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received Authorized QoS flow descriptions IE\n");
        psea_msg.qos_fd_ie.length = getShort(curPtr);
        curPtr += (psea_msg.qos_fd_ie.length + sizeof(psea_msg.qos_fd_ie.length));
        break;

      case IEI_EXT_CONF_OPT: /* Ommited */
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received Extended protocol configuration options IE\n");
        psea_msg.ext_pp_ie.length = getShort(curPtr);
        curPtr += (psea_msg.ext_pp_ie.length + sizeof(psea_msg.ext_pp_ie.length));
        break;

      case IEI_DNN:
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Received DNN IE\n");
        psea_msg.dnn_ie.dnn_length = *curPtr++;
        char apn[APN_MAX_LEN];

        if(psea_msg.dnn_ie.dnn_length <= APN_MAX_LEN &&
           psea_msg.dnn_ie.dnn_length >= APN_MIN_LEN) {
          memcpy(apn, curPtr, psea_msg.dnn_ie.dnn_length);
          LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - APN: %s\n", apn);
        } else
          LOG_E(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - DNN IE has invalid length\n");

        curPtr = buffer + msg_length; // we force stop processing
        break;

      default:
        LOG_T(NAS, "PDU SESSION ESTABLISHMENT ACCEPT - Not known IE\n");
        curPtr = buffer + msg_length; // we force stop processing
        break;
    }
  }

  set_qfi_pduid(qos_rule.qfi, psea_msg.pdu_id);
  return;
}
