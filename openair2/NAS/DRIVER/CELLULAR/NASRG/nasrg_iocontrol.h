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

/*! \file nasrg_iocontrol.h
* \brief I/O control constnats and structures for upper layers of driver for OpenAirInterface CELLULAR version - RG
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#ifndef NASRGD_CTL_H
#define NASRGD_CTL_H

#include <asm/byteorder.h>
#include <asm/types.h>
#include <linux/udp.h>
#include <linux/tcp.h>

#define NAS_MSG_MAXLEN 1100

// type of CTL message
#define NAS_MSG_STATISTIC_REQUEST       1
#define NAS_MSG_STATISTIC_REPLY         2
#define NAS_MSG_ECHO_REQUEST   3
#define NAS_MSG_ECHO_REPLY   4
#define NAS_MSG_CX_ESTABLISHMENT_REQUEST 5
#define NAS_MSG_CX_ESTABLISHMENT_REPLY 6
#define NAS_MSG_CX_RELEASE_REQUEST  7
#define NAS_MSG_CX_RELEASE_REPLY  8
#define NAS_MSG_CX_LIST_REQUEST  9
#define NAS_MSG_CX_LIST_REPLY   10
#define NAS_MSG_RB_ESTABLISHMENT_REQUEST 11
#define NAS_MSG_RB_ESTABLISHMENT_REPLY 12
#define NAS_MSG_RB_RELEASE_REQUEST  13
#define NAS_MSG_RB_RELEASE_REPLY  14
#define NAS_MSG_RB_LIST_REQUEST  15
#define NAS_MSG_RB_LIST_REPLY   16
#define NAS_MSG_CLASS_ADD_REQUEST  17
#define NAS_MSG_CLASS_ADD_REPLY  18
#define NAS_MSG_CLASS_DEL_REQUEST  19
#define NAS_MSG_CLASS_DEL_REPLY  20
#define NAS_MSG_CLASS_LIST_REQUEST  21
#define NAS_MSG_CLASS_LIST_REPLY  22
#define NAS_MSG_NEIGHBOUR_REQUEST  23
#define NAS_MSG_NEIGHBOUR_REPLY  24
//MBMS
#define NAS_RG_MSG_MT_MCAST_JOIN           30
#define NAS_RG_MSG_MT_MCAST_LEAVE          31
#define NAS_RG_MSG_MT_MCAST_REPLY          32
// ENB MEASURES (MEDIEVAL DEMO 3)
#define NAS_MSG_ENB_MEAS_TRIGGER  33
#define NAS_MSG_ENB_MEAS_RETRIEVE 34


// Max number of entry of a message list
#define NAS_LIST_CX_MAX 32
#define NAS_LIST_RB_MAX 32
#define NAS_LIST_CLASS_MAX 32

typedef uint16_t nasMsgType_t;

struct nas_ioctl {
  char name[IFNAMSIZ];
  nasMsgType_t type;
  char *msg;
};

//****
struct nas_msg_statistic_reply {
  uint32_t rx_packets;
  uint32_t tx_packets;
  uint32_t rx_bytes;
  uint32_t tx_bytes;
  uint32_t rx_errors;
  uint32_t tx_errors;
  uint32_t rx_dropped;
  uint32_t tx_dropped;
};

//****
struct nas_msg_cx_list_reply {
  nasLocalConnectionRef_t lcr;  // Local Connection reference
  uint8_t state;
  nasCellID_t cellid;  // cell identification
  uint32_t iid6[2];    // IPv6  interface identification
  uint8_t iid4;    // IPv4 interface identification
  uint16_t num_rb;
  uint16_t nsclassifier;
};
//****
struct nas_msg_cx_establishment_reply {
  int status;
};
struct nas_msg_cx_establishment_request {
  nasLocalConnectionRef_t lcr; // Local Connection reference
  nasCellID_t cellid; // Cell identification
};
//****
struct nas_msg_cx_release_reply {
  int status;
};
struct nas_msg_cx_release_request {
  nasLocalConnectionRef_t lcr; // Local Connection reference
};

//****
struct nas_msg_rb_list_reply {
  nasRadioBearerId_t rab_id;
  nasSapId_t sapi;
  nasQoSTrafficClass_t qos;
  uint8_t state;
  uint32_t cnxid;
};
struct nas_msg_rb_list_request {
  nasLocalConnectionRef_t lcr;  // Local Connection reference
};
//****
struct nas_msg_rb_establishment_reply {
  int status;
  uint32_t cnxid;
};
struct nas_msg_rb_establishment_request {
  nasLocalConnectionRef_t lcr; // Local Connection reference
  nasRadioBearerId_t rab_id;
  nasQoSTrafficClass_t qos;
  uint16_t dscp_ul;
  uint16_t dscp_dl;
  uint16_t mcast_flag;
  uint8_t mcast_group[16];
  uint32_t cnxid;
};

//****
struct nas_msg_rb_release_reply {
  int status;
  uint32_t cnxid;
};
struct nas_msg_rb_release_request {
  nasLocalConnectionRef_t lcr; // Local Connection reference
  nasRadioBearerId_t rab_id;
  uint16_t mcast_flag;
  uint32_t cnxid;
};

//****
struct nas_msg_class_add_request {
  nasLocalConnectionRef_t lcr; // Local Connection reference
  nasRadioBearerId_t rab_id;
  uint8_t dir; // direction (send or receive)
  uint8_t dscp; // codepoint
  uint8_t fct;
  uint16_t classref;
  uint8_t version;
  union {
    struct in6_addr ipv6;
    uint32_t ipv4;
  } saddr; // IP source address
  uint8_t splen; // prefix length
  union {
    struct in6_addr ipv6;
    uint32_t ipv4;
  } daddr; // IP destination address
  uint8_t dplen; // prefix length
  uint8_t protocol;  // high layer protocol type
  uint16_t sport;  // source port
  uint16_t dport;  // destination port
};
struct nas_msg_class_add_reply {
  int status;
};
//****
struct nas_msg_class_del_request {
  nasLocalConnectionRef_t lcr; // Local Connection reference
  uint8_t dir; // direction (send or receive)
  uint8_t dscp; // codepoint
  uint16_t classref;
};
struct nas_msg_class_del_reply {
  int status;
};
//****
#define nas_msg_class_list_reply nas_msg_class_add_request
struct nas_msg_class_list_request {
  nasLocalConnectionRef_t lcr;  // Local Connection reference
  uint8_t dir;
  uint8_t dscp;
};

//****
struct nas_msg_neighbour_cell_list_reply {
  nasCellID_t cellid;  // cell identification
  uint32_t iid6[4];    // IPv6  address of access router
};

//MBMS
//****
struct nas_msg_mt_mcast_join {
  nasLocalConnectionRef_t  ue_id;
  uint32_t   cnxid;
  nasRadioBearerId_t rab_id;
};

struct nas_msg_mt_mcast_leave {
  nasLocalConnectionRef_t  ue_id;
  uint32_t   cnxid;
  nasRadioBearerId_t rab_id;
};

struct nas_msg_mt_mcast_reply {
  nasLocalConnectionRef_t  ue_id;
  uint32_t   cnxid;
  int  result;
};
//****

//MEDIEVAL DEMO 3
//****
struct nas_msg_enb_measure_trigger {
  nasCellID_t cell_id;
};
struct nas_msg_enb_measure_trigger_reply {
  int status;
};

struct nas_msg_enb_measure_retrieve {
  nasCellID_t cell_id;
  nasNumConnUEs_t num_UEs;
  nasENbMeasures_t measures[MAX_MEASURE_UE];
  nasENbMeasure_t totalNumPRBs;
};


#endif
