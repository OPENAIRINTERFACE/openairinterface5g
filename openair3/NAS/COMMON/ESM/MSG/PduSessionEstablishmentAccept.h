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

#endif