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

#ifndef NAS_CTL_H
#define NAS_CTL_H

#include <asm/byteorder.h>
#include <asm/types.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/if.h>


//#include <linux/ipv6.h>

#define NAS_MSG_MAXLEN 1100/// change???

// type of CTL message
#define NAS_MSG_STATISTIC_REQUEST   1
#define NAS_MSG_STATISTIC_REPLY     2
#define NAS_MSG_ECHO_REQUEST      3
#define NAS_MSG_ECHO_REPLY      4
#define NAS_MSG_CX_ESTABLISHMENT_REQUEST  5
#define NAS_MSG_CX_ESTABLISHMENT_REPLY  6
#define NAS_MSG_CX_RELEASE_REQUEST    7
#define NAS_MSG_CX_RELEASE_REPLY    8
#define NAS_MSG_CX_LIST_REQUEST   9
#define NAS_MSG_CX_LIST_REPLY     10
#define NAS_MSG_RB_ESTABLISHMENT_REQUEST  11
#define NAS_MSG_RB_ESTABLISHMENT_REPLY  12
#define NAS_MSG_RB_RELEASE_REQUEST    13
#define NAS_MSG_RB_RELEASE_REPLY    14
#define NAS_MSG_RB_LIST_REQUEST   15
#define NAS_MSG_RB_LIST_REPLY     16
#define NAS_MSG_CLASS_ADD_REQUEST   17
#define NAS_MSG_CLASS_ADD_REPLY   18
#define NAS_MSG_CLASS_DEL_REQUEST   19
#define NAS_MSG_CLASS_DEL_REPLY   20
#define NAS_MSG_CLASS_LIST_REQUEST    21
#define NAS_MSG_CLASS_LIST_REPLY    22
#define NAS_MSG_MEAS_REQUEST    23
#define NAS_MSG_MEAS_REPLY    24
#define NAS_MSG_IMEI_REQUEST    25
#define NAS_MSG_IMEI_REPLY    26

// Max number of entry of a message list
#define NAS_LIST_CX_MAX 32
#define NAS_LIST_RB_MAX 32
#define NAS_LIST_CLASS_MAX  32

typedef unsigned short nasMsgType_t;

struct nas_ioctl {
  char name[IFNAMSIZ];
  nasMsgType_t type;
  char *msg;
};

struct nas_msg_statistic_reply {
  unsigned int rx_packets;
  unsigned int tx_packets;
  unsigned int rx_bytes;
  unsigned int tx_bytes;
  unsigned int rx_errors;
  unsigned int tx_errors;
  unsigned int rx_dropped;
  unsigned int tx_dropped;
};

struct nas_msg_cx_list_reply {
  nasLocalConnectionRef_t lcr;  // Local Connection reference
  unsigned char state;
  nasCellID_t cellid;   // cell identification
  unsigned int iid6[2];       // IPv6  interface identification
  unsigned char iid4;       // IPv4 interface identification
  unsigned short num_rb;
  unsigned short nsclassifier;
};
struct nas_msg_cx_establishment_reply {
  int status;
};
struct nas_msg_cx_establishment_request {
  nasLocalConnectionRef_t lcr;  // Local Connection reference
  nasCellID_t cellid; // Cell identification
};
struct nas_msg_cx_release_reply {
  int status;
};
struct nas_msg_cx_release_request {
  nasLocalConnectionRef_t lcr; // Local Connection reference
};

struct nas_msg_rb_list_reply {
  nasRadioBearerId_t rab_id;
  nasSapId_t sapi;
  nasQoSTrafficClass_t qos;
  unsigned char state;
};
struct nas_msg_rb_list_request {
  nasLocalConnectionRef_t lcr;  // Local Connection reference
};
struct nas_msg_rb_establishment_reply {
  int status;
};
struct nas_msg_rb_establishment_request {
  nasLocalConnectionRef_t lcr;  // Local Connection reference
  nasRadioBearerId_t rab_id;
  nasQoSTrafficClass_t qos;
};

struct nas_msg_rb_release_reply {
  int status;
};
struct nas_msg_rb_release_request {
  nasLocalConnectionRef_t lcr; // Local Connection reference
  nasRadioBearerId_t rab_id;
};
/*
struct saddr {

  struct in6_addr ipv6;
  unsigned int ipv4;
};

struct daddr {

  struct in6_addr ipv6;
  unsigned int ipv4;
  unsigned int mpls_label;
};
*/
struct nas_msg_class_add_request {
  nasLocalConnectionRef_t lcr; // Local Connection reference
  nasRadioBearerId_t rab_id;
  nasRadioBearerId_t rab_id_rx;
  unsigned char dir; // direction (send or receive, forward)
  unsigned char dscp; // codepoint
  unsigned char fct;
  unsigned short classref;
  unsigned char version;

  //struct daddr daddr;
  //struct saddr saddr;

  unsigned char splen; // prefix length

  union {
    struct in6_addr ipv6;

    // begin navid
    //in_addr_t    ipv4;
    unsigned int ipv4;
    //end navid

    unsigned int mpls_label;
  } daddr; // IP destination address

  union {
    struct in6_addr ipv6;

    // begin navid
    //in_addr_t    ipv4;
    unsigned int ipv4;
    //end navid

  } saddr; // IP source address


  unsigned char dplen; // prefix length

  unsigned char protocol;            //!< transport layer protocol type (ANY,TCP,UDP,ICMPv4,ICMPv6)
  unsigned char protocol_message_type;   //!< transport layer protocol message (ROUTER_ADV, ROUTER_SOLL, etc.)
  unsigned short sport;            //!< source port
  unsigned short dport;            //!< destination port
};

struct nas_msg_class_add_reply {
  int status;
};
struct nas_msg_class_del_request {
  nasLocalConnectionRef_t lcr; // Local Connection reference
  unsigned char dir; // direction (send or receive)
  unsigned char dscp; // codepoint
  unsigned short classref;
};
struct nas_msg_class_del_reply {
  int status;
};
#define nas_msg_class_list_reply nas_msg_class_add_request
struct nas_msg_class_list_request {
  nasLocalConnectionRef_t lcr;  // Local Connection reference
  unsigned char dir;
  unsigned char dscp;
};


// Messages for Measurement transfer - MW 01/04/2005
typedef unsigned int nioctlProviderId_t;
typedef unsigned short nioctlSignalLoss_t;
typedef struct nioctlMeasures {
  nasCellID_t cell_id;
  nasSigLevel_t level;
  nioctlProviderId_t provider_id;
} nioctlMeasures_t;

struct nas_msg_measure_request {
  nasNumRGsMeas_t num_cells;
  nasCellID_t cellid[MAX_MEASURE_NB]; // Cell identification
  unsigned short num_providers;
  nioctlProviderId_t provider_id[MAX_MEASURE_NB]; // Provider identification
};
struct nas_msg_measure_reply {
  nasNumRGsMeas_t num_cells;
  nioctlMeasures_t measures[MAX_MEASURE_NB];
  nioctlSignalLoss_t signal_lost_flag;
};

// Messages for Measurement transfer - MW 01/04/2005
typedef unsigned int nioctlL2Id_t[2];

struct nas_msg_l2id_reply {
  nioctlL2Id_t l2id;
};


#endif
