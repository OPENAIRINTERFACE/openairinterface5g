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

#include "openairinterface5g_limits.h"
#ifndef _UE_IP_CST
#define _UE_IP_CST

#define UE_IP_MAX_LENGTH 180

// General Constants
#define UE_IP_MTU                    1500
#define UE_IP_TX_QUEUE_LEN           100
#define UE_IP_ADDR_LEN               8
#define UE_IP_INET6_ADDRSTRLEN       46
#define UE_IP_INET_ADDRSTRLEN        16
#define UE_IP_DEFAULT_RAB_ID         1

#define UE_IP_RESET_RX_FLAGS         0


#define UE_IP_RETRY_LIMIT_DEFAULT    (int)5

#define UE_IP_MESSAGE_MAXLEN         (int)5004

#define UE_IP_TIMER_ESTABLISHMENT_DEFAULT (int)12
#define UE_IP_TIMER_RELEASE_DEFAULT       (int)2
#define UE_IP_TIMER_IDLE                  UINT_MAX
#define UE_IP_TIMER_TICK                  HZ

#define UE_IP_PDCPH_SIZE                  (int)sizeof(struct pdcp_data_req_header_s)
#define UE_IP_IPV4_SIZE                   (int)20
#define UE_IP_IPV6_SIZE                   (int)40




#define UE_IP_NB_INSTANCES_MAX       NUMBER_OF_UE_MAX /*MAX_MOBILES_PER_ENB*/


#endif

