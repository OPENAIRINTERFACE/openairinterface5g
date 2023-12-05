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


#ifndef __PLATFORM_CONSTANTS_H__
#define __PLATFORM_CONSTANTS_H__

#define NL_MAX_PAYLOAD 18000 /* this should cover the max mtu size*/

#ifdef LARGE_SCALE
#define NB_MODULES_MAX 128
#define NB_NODE_MAX 128
#else
#define NB_MODULES_MAX 32
#define NB_NODE_MAX 32
#endif

#define MAX_IP_PACKET_SIZE 10000 // 9000

#define MAX_MODULES NB_MODULES_MAX
#define MAX_NR_RRC_UE_CONTEXTS 64

#ifndef UE_EXPANSION
#ifdef LARGE_SCALE
#define MAX_MOBILES_PER_ENB 128
#define MAX_MOBILES_PER_ENB_NB_IoT 128
#define MAX_eNB 2
#define MAX_gNB 2
#else
#define MAX_MOBILES_PER_ENB 40
#define MAX_MOBILES_PER_ENB_NB_IoT 40
#define MAX_eNB 2
#define MAX_gNB 2
#endif
#else
#define MAX_MOBILES_PER_ENB 256
#define MAX_MOBILES_PER_ENB_NB_IoT 256
#define MAX_eNB 2
#define MAX_gNB 2
#endif

#define NUMBER_OF_NR_UCI_STATS_MAX 16
#define MAX_MANAGED_ENB_PER_MOBILE 2
#define MAX_MANAGED_GNB_PER_MOBILE 2

/// NB-IOT
#define NB_RB_MAX_NB_IOT (2 + 3) /* 2 from LTE_maxDRB_NB_r13 in LTE_asn_constant.h + 3 SRBs */

#define DEFAULT_RAB_ID 1

#define NB_RB_MAX (11 + 3) /* LTE_maxDRB from LTE_asn_constant.h + 3 SRBs */
#define NR_NB_RB_MAX (29 + 3) /* NR_maxDRB from NR_asn_constant.hm + 3 SRBs */

#define NGAP_MAX_PDU_SESSION (256) /* As defined in TS 38.413 9.2.1.1 Range Bound for PDU Sessions. */

#define MAX_DRBS_PER_UE         (32)  /* Maximum number of Data Radio Bearers per UE
                                       * defined for NGAP in TS 38.413 - maxnoofDRBs */
#define MAX_PDUS_PER_UE         (8)   /* Maximum number of PDU Sessions per UE */

#define NB_RB_MBMS_MAX (29 * 16) /* 29 = LTE_maxSessionPerPMCH + 16 = LTE_maxServiceCount from LTE_asn_constant.h */

#define NB_RAB_MAX 11 /* from LTE_maxDRB in LTE_asn_constant.h */
#define RAB_OFFSET 0x000F

// RLC Entity
#define RLC_TX_MAXSIZE       10000000
#define RLC_RX_MAXSIZE       10000000
#define SEND_MRW_ON 240
#define MAX_ANT 8
// CBA constant
#define NUM_MAX_CBA_GROUP 4

#define printk printf
#define RETURNok (0)
#define RETURNerror (-1)
#define DEFAULT_NAS_PATH "PWD"
#define UNUSED_VARIABLE(vARIABLE) (void)(vARIABLE)

#endif /* __PLATFORM_CONSTANTS_H__ */
