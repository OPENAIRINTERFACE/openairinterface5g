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

#ifndef _NAS_CST
#define _NAS_CST

#define MAX_MEASURE_NB 5
#define NAS_MAX_LENGTH 180

//Debug flags
//#define NAS_DEBUG_DC
//#define NAS_DEBUG_SEND
//#define NAS_DEBUG_RECEIVE
//#define NAS_DEBUG_CLASS
//#define NAS_DEBUG_GC
//#define NAS_DEBUG_DC_MEASURE
//#define NAS_DEBUG_TIMER
//#define NAS_DEBUG_DEVICE
//#define NAS_DEBUG_INTERRUPT
//#define NAS_DEBUG_TOOL

// Other flags
#define DEMO_3GSM

// General Constants
#define NAS_MTU 1500
#define NAS_TX_QUEUE_LEN 100
#define NAS_ADDR_LEN 8
#define NAS_INET6_ADDRSTRLEN 46
#define NAS_INET_ADDRSTRLEN 16

#define NAS_CX_MAX 128 // 32  //Identical to RRC constant
//#define NAS_CX_MULTICAST_ALLNODE 2

#define NAS_RETRY_LIMIT_DEFAULT 5

#define NAS_MESSAGE_MAXLEN 5004

#define NAS_SIG_SRB3 3
#define NAS_SIG_SRB4 3 // not used yet

//peer-to-peer messages between NAS entities
#define NAS_CMD_OPEN_RB 1

//#define NAS_IID1_CONTROL 0x0
//#define NAS_IID2_CONTROL __constant_htonl(0xffffffff)

//#define NAS_STATE_IDLE      0
//#define NAS_STATE_CONNECTED     1
//#define NAS_STATE_ESTABLISHMENT_REQUEST 2
//#define NAS_STATE_ESTABLISHMENT_FAILURE 3
//#define NAS_STATE_RELEASE_FAILURE   4
#define NAS_CX_RELEASE_UNDEF_CAUSE 1

// MT+RG NAS States
#define NAS_IDLE                  0x01
// Connection
#define NAS_CX_FACH               0x06
#define NAS_CX_DCH                0x0A
#define NAS_CX_RECEIVED           0x10
#define NAS_CX_CONNECTING         0x04
#define NAS_CX_RELEASING          0x08
#define NAS_CX_CONNECTING_FAILURE 0x14
#define NAS_CX_RELEASING_FAILURE  0x18
// Radio Bearers
#define NAS_RB_ESTABLISHING       0x24
#define NAS_RB_RELEASING          0x28
#define NAS_RB_DCH                0x2A


#define NAS_TIMER_ESTABLISHMENT_DEFAULT 12
#define NAS_TIMER_RELEASE_DEFAULT       2
#define NAS_TIMER_IDLE                  UINT_MAX
#define NAS_TIMER_TICK                  HZ

#define NAS_PDCPH_SIZE                  sizeof(struct pdcp_data_req_header_s)
#define NAS_IPV4_SIZE                   20
#define NAS_IPV6_SIZE                   40

#define NAS_DIRECTION_SEND      0
#define NAS_DIRECTION_RECEIVE   1
#define NAS_DIRECTION_FORWARD   2

// function number
#define NAS_FCT_DEL_SEND        1
#define NAS_FCT_QOS_SEND        2
#define NAS_FCT_DC_SEND         3
#define NAS_FCT_CTL_SEND        4

// type of IOCTL command
#define NAS_IOCTL_RRM 0x89F0

// Error cause
#define NAS_ERROR_ALREADYEXIST          1
#define NAS_ERROR_NOMEMORY    3
#define NAS_ERROR_NOTMT     9
#define NAS_ERROR_NOTRG     10
#define NAS_ERROR_NOTIDLE     11
#define NAS_ERROR_NOTCONNECTED    12
#define NAS_ERROR_NORB            14
#define NAS_ERROR_NOTCORRECTVALUE 32
#define NAS_ERROR_NOTCORRECTLCR         33
#define NAS_ERROR_NOTCORRECTDIR         34
#define NAS_ERROR_NOTCORRECTDSCP  35
#define NAS_ERROR_NOTCORRECTVERSION 36
#define NAS_ERROR_NOTCORRECTRABI  37


/**********************************************************/
/* Constants related with IP protocols                    */
/**********************************************************/

//#define NAS_PORT_CONTROL __constant_htons(0xc45)
//#define NAS_PORT_AUTHENTICATION __constant_htons(1811)

#define NAS_TRAFFICCLASS_MASK __constant_htonl(0x0fc00000)

// Network control codepoint 111000 + IP version 6
#define NAS_FLOWINFO_NCONTROL __constant_htonl(0x6e000000)
// network control codepoint 111000
#define NAS_DSCP_NCONTROL 56   //0x38
// default codepoint 1000000
#define NAS_DSCP_DEFAULT 64
#define NAS_DSCP_MAX 193

#define NAS_PROTOCOL_DEFAULT 0
#define NAS_PROTOCOL_TCP IPPROTO_TCP
#define NAS_PROTOCOL_UDP IPPROTO_UDP
#define NAS_PROTOCOL_ICMP4 IPPROTO_ICMP
#define NAS_PROTOCOL_ICMP6 IPPROTO_ICMPV6

#define NAS_PORT_DEFAULT  __constant_htons(65535)
#define NAS_PORT_HTTP   __constant_htons(80)

#define NAS_VERSION_DEFAULT   0
#define NAS_VERSION_4   4
#define NAS_VERSION_6   0 //?MW

#define NAS_DEFAULT_IPV4_ADDR 0
#define NAS_DEFAULT_IPV6_ADDR0 0
#define NAS_DEFAULT_IPV6_ADDR1 0
#define NAS_DEFAULT_IPV6_ADDR2 0
#define NAS_DEFAULT_IPV6_ADDR3 0


#define NAS_MPLS_VERSION_CODE 99

#define NB_INSTANCES_MAX 64 //16


#endif



