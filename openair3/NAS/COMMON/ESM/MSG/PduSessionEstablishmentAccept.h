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

#ifndef PDU_SESSION_ESTABLISHMENT_ACCEPT_H_
#define PDU_SESSION_ESTABLISHMENT_ACCEPT_H_

#include <stdint.h>

/* PDU Session Establish Accept Optional IE Identifiers - TS 24.501 Table 8.3.2.1.1 */

#define IEI_5GSM_CAUSE      0x59 /* 5GSM cause 9.11.4.2  */
#define IEI_PDU_ADDRESS     0x29 /* PDU address 9.11.4.10 */
#define IEI_RQ_TIMER_VALUE  0x56 /* GPRS timer 9.11.2.3  */
#define IEI_SNSSAI          0x22 /* S-NSSAI 9.11.2.8  */
#define IEI_ALWAYSON_PDU    0x80 /* Always-on PDU session indication 9.11.4.3 */
#define IEI_MAPPED_EPS      0x75 /* Mapped EPS bearer contexts 9.11.4.8  */
#define IEI_EAP_MSG         0x78 /* EAP message 9.11.2.2  */
#define IEI_AUTH_QOS_DESC   0x79 /* QoS flow descriptions 9.11.4.12 */
#define IEI_EXT_CONF_OPT    0x7b /* Extended protocol configuration options 9.11.4.6  */
#define IEI_DNN             0x25 /* DNN 9.11.2.1B  */

/* PDU Session type value - TS 24.501 Table 9.11.4.10.1*/

#define PDU_SESSION_TYPE_IPV4   0b001
#define PDU_SESSION_TYPE_IPV6   0b010
#define PDU_SESSION_TYPE_IPV4V6 0b011

/* Rule operation codes - TS 24.501 Table 9.11.4.13.1 */

#define ROC_RESERVED_0                  0b000 /* Reserved */
#define ROC_CREATE_NEW_QOS_RULE         0b001 /* Create new QoS rule */
#define ROC_DELETE_QOS_RULE             0b010 /* Delete existing QoS rule */
#define ROC_MODIFY_QOS_RULE_ADD_PF      0b011 /* Modify existing QoS rule and add packet filters */
#define ROC_MODIFY_QOS_RULE_REPLACE_PF  0b100 /* Modify existing QoS rule and replace all packet filters */
#define ROC_MODIFY_QOS_RULE_DELETE_PF   0b101 /* Modify existing QoS rule and delete packet filters */
#define ROC_MODIFY_QOS_RULE_WITHOUT_PF  0b110 /* Modify existing QoS rule without modifying packet filters */
#define ROC_RESERVED_1                  0b111 /* Reserved */

/* DNN - APN 
 * TS 23.003 9.1
 * The APN is composed of two parts, the APN Network Identifier (9.1.1) & The APN Operator Identifier (9.1.2).
 * 
 * The DNN information element has a length in the range of 3 to 102 octets.
 * The Header is consisted of two octets, the DNN IEI and the Length of the DNN contents fields, each is 1 octet. 
 * The DNN value payload starts from the 3rd octet.
 * The accumulated max length of APN payload is 100 octets.
 * The min length of the APN payload is 1 octet.
 */
#define APN_MAX_LEN 100
#define APN_MIN_LEN 1

/* Mandatory Presence IE - TS 24.501 Table 8.3.2.1.1 */

typedef struct packet_filter_create_qos_rule_s {
  uint8_t pf_dir; /* Packet filter direction */
  uint8_t pf_id;  /* Packet filter identifier */
  uint8_t length; /* Length of packet filter contents */
} packet_filter_type1_t; /* TS 24.501 Figure 9.11.4.13.3 */

typedef struct packet_filter_modify_qos_rule_s {
  uint8_t pf_id;  /* Packet filter identifier */
} packet_filter_type2_t; /* TS 24.501 Figure 9.11.4.13.4 */

typedef struct packet_filter_s {
  union pf_type {
    packet_filter_type1_t type_1;
    packet_filter_type2_t type_2;
  } pf_type;
} packet_filter_t;

typedef struct qos_rule_s {
  uint8_t  id;      /* QoS rule identifier */
  uint16_t length;  /* Length of QoS Rule */
  uint8_t  oc;      /* Rule operation code (3bits) */
  uint8_t  dqr;     /* DQR bit (1 bit) */
  uint8_t  nb_pf;   /* Number of packet filters (4 bits) */
  uint8_t  prcd;    /* QoS rule precedence */
  uint8_t  qfi;     /* QoS Flow Identifier */
} qos_rule_t;

typedef struct auth_qos_rules_s {
  uint16_t length;  /* Length of QoS rules IE */
} auth_qos_rule_t;  /* QoS Rule as defined in 24.501 Figure 9.11.4.13.2 */

typedef struct session_ambr_s {
  uint8_t  length;  /* Length of Session-AMBR contents */
  uint8_t  unit_dl; /* Unit for Session-AMBR for downlink */
  uint16_t sess_dl; /* Session-AMBR for downlink */
  uint8_t  unit_ul; /* Unit for Session-AMBR for uplink */
  uint16_t sess_ul; /* Session-AMBR for uplink */
} session_ambr_t;   /* TS 24.501 Figure 9.11.4.14.1 */

/* Optional Presence IE - TS 24.501 Table 8.3.2.1.1 */

typedef struct pdu_address_s {
  uint8_t pdu_iei;        /* PDU Address IEI (0x29) */
  uint8_t pdu_length;     /* Length of PDU address contents */
  uint8_t pdu_type;       /* PDU session type value */
  uint8_t pdu_addr_oct1;  /* PDU address information (1st Octet)*/
  uint8_t pdu_addr_oct2;  /* PDU address information (2nd Octet)*/
  uint8_t pdu_addr_oct3;  /* PDU address information (3rd Octet)*/
  uint8_t pdu_addr_oct4;  /* PDU address information (4th Octet)*/
} pdu_address_t; /* TS 24.501 9.11.4.10 */

typedef struct dnn_s {
  uint8_t dnn_iei;    /* DNN IEI (0x25) */
  uint8_t dnn_length; /* Length of DNN contents */
} dnn_t; /* TS 24.501 9.11.2.1A */

typedef struct ext_pP_t {
  uint16_t length;
} ext_pP_t; /* TS 24.008 10.5.6.3A - Ommited, only length is processed*/

typedef struct qos_fd_s {
  uint16_t length;
} qos_fd_t; /* TS 24.501 9.11.4.12 - Ommited, only length is processed*/

typedef struct pdu_session_establishment_accept_msg_s {
  /* Mandatory presence */
  uint8_t epd;                /* Extended Protocol Discriminator */
  uint8_t pdu_id;             /* PDU Session ID */
  uint8_t pti;                /* Procedure Transaction Identity */
  uint8_t msg_type;           /* Message Type */
  uint8_t pdu_type;           /* PDU Session Type */
  uint8_t ssc_mode;           /* SSC mode */
  auth_qos_rule_t qos_rules;  /* Authorized QoS rules */
  session_ambr_t  sess_ambr;  /* Session-AMBR */

  /* Optional presence */
  dnn_t         dnn_ie;       /* Data Network Name */
  pdu_address_t pdu_addr_ie;  /* PDU Address */
  ext_pP_t      ext_pp_ie;    /* Extended Protocol Configuration Options */
  qos_fd_t      qos_fd_ie;    /* QoS flow descriptions */
} pdu_session_establishment_accept_msg_t; /* 24.501 Table 8.3.2.1.1 */

typedef struct security_protected_plain_nas_5gs_msg_s {
  uint8_t  epd;           /* Extended Protocol Discriminator */
  uint8_t  sht;           /* Security Header Type */
  uint8_t  msg_type;      /* Message Type */
  uint8_t  payload_type;  /* Payload Container Type */
  uint16_t payload_len;   /* Payload Container Length */
} security_protected_plain_nas_5gs_msg_t;

typedef struct security_protected_nas_5gs_msg_s {
  uint8_t  epd; /* Extended Protocol Discriminator */
  uint8_t  sht; /* Security Header Type */
  uint32_t mac; /* Message Authentication Code */
  uint8_t  sqn; /* Sequence Number */
} security_protected_nas_5gs_msg_t; /* 24.501 Figure 9.1.1.2 */

void capture_pdu_session_establishment_accept_msg(uint8_t *buffer, uint32_t msg_length);

#endif
