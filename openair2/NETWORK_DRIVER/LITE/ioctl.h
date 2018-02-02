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

/***************************************************************************
                          ioctl.h  -  description
                             -------------------
    copyright            : (C) 2002 by Eurecom
    email                : michelle.wetterwald@eurecom.fr
                           yan.moret@eurecom.fr
                           knopp@eurecom.fr
 ***************************************************************************

 ***************************************************************************/
#ifndef OAI_IOCTL_H
#define OAI_IOCTL_H

#include <asm/byteorder.h>
#include <asm/types.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/if.h>

#include "sap.h"


//#include <linux/ipv6.h>

#define OAI_NW_DRV_MSG_MAXLEN 1100/// change???

// type of CTL message
#define OAI_NW_DRV_MSG_STATISTIC_REQUEST               1
#define OAI_NW_DRV_MSG_STATISTIC_REPLY                 2
#define OAI_NW_DRV_MSG_ECHO_REQUEST                    3
#define OAI_NW_DRV_MSG_ECHO_REPLY                      4
#define OAI_NW_DRV_MSG_CX_ESTABLISHMENT_REQUEST        5
#define OAI_NW_DRV_MSG_CX_ESTABLISHMENT_REPLY          6
#define OAI_NW_DRV_MSG_CX_RELEASE_REQUEST              7
#define OAI_NW_DRV_MSG_CX_RELEASE_REPLY                8
#define OAI_NW_DRV_MSG_CX_LIST_REQUEST                 9
#define OAI_NW_DRV_MSG_CX_LIST_REPLY                   10
#define OAI_NW_DRV_MSG_RB_ESTABLISHMENT_REQUEST        11
#define OAI_NW_DRV_MSG_RB_ESTABLISHMENT_REPLY          12
#define OAI_NW_DRV_MSG_RB_RELEASE_REQUEST              13
#define OAI_NW_DRV_MSG_RB_RELEASE_REPLY                14
#define OAI_NW_DRV_MSG_RB_LIST_REQUEST                 15
#define OAI_NW_DRV_MSG_RB_LIST_REPLY                   16
#define OAI_NW_DRV_MSG_CLASS_ADD_REQUEST               17
#define OAI_NW_DRV_MSG_CLASS_ADD_REPLY                 18
#define OAI_NW_DRV_MSG_CLASS_DEL_REQUEST               19
#define OAI_NW_DRV_MSG_CLASS_DEL_REPLY                 20
#define OAI_NW_DRV_MSG_CLASS_LIST_REQUEST              21
#define OAI_NW_DRV_MSG_CLASS_LIST_REPLY                22
#define OAI_NW_DRV_MSG_MEAS_REQUEST                    23
#define OAI_NW_DRV_MSG_MEAS_REPLY                      24
#define OAI_NW_DRV_MSG_IMEI_REQUEST                    25
#define OAI_NW_DRV_MSG_IMEI_REPLY                      26

// Max number of entry of a message list
#define OAI_NW_DRV_LIST_CX_MAX                         32
#define OAI_NW_DRV_LIST_RB_MAX 32
#define OAI_NW_DRV_LIST_CLASS_MAX                      32

typedef unsigned short oai_nw_drv_MsgType_t;

struct oai_nw_drv_ioctl {
  char name[IFNAMSIZ];
  oai_nw_drv_MsgType_t type;
  char *msg;
};

struct oai_nw_drv_msg_statistic_reply {
  unsigned int rx_packets;
  unsigned int tx_packets;
  unsigned int rx_bytes;
  unsigned int tx_bytes;
  unsigned int rx_errors;
  unsigned int tx_errors;
  unsigned int rx_dropped;
  unsigned int tx_dropped;
};

struct oai_nw_drv_msg_cx_list_reply {
  OaiNwDrvLocalConnectionRef_t lcr;    // Local Connection reference
  unsigned char state;
  OaiNwDrvCellID_t cellid;     // cell identification
  unsigned int iid6[2];           // IPv6  interface identification
  unsigned char iid4;             // IPv4 interface identification
  unsigned short num_rb;
  unsigned short nsclassifier;
};
struct oai_nw_drv_msg_cx_establishment_reply {
  int status;
};
struct oai_nw_drv_msg_cx_establishment_request {
  OaiNwDrvLocalConnectionRef_t lcr;    // Local Connection reference
  OaiNwDrvCellID_t cellid; // Cell identification
};
struct oai_nw_drv_msg_cx_release_reply {
  int status;
};
struct oai_nw_drv_msg_cx_release_request {
  OaiNwDrvLocalConnectionRef_t lcr; // Local Connection reference
};

struct oai_nw_drv_msg_rb_list_reply {
  OaiNwDrvRadioBearerId_t rab_id;
  OaiNwDrvSapId_t sapi;
  OaiNwDrvQoSTrafficClass_t qos;
  unsigned char state;
};
struct oai_nw_drv_msg_rb_list_request {
  OaiNwDrvLocalConnectionRef_t lcr;    // Local Connection reference
};
struct oai_nw_drv_msg_rb_establishment_reply {
  int status;
};
struct oai_nw_drv_msg_rb_establishment_request {
  OaiNwDrvLocalConnectionRef_t lcr;    // Local Connection reference
  OaiNwDrvRadioBearerId_t rab_id;
  OaiNwDrvQoSTrafficClass_t qos;
};

struct oai_nw_drv_msg_rb_release_reply {
  int status;
};
struct oai_nw_drv_msg_rb_release_request {
  OaiNwDrvLocalConnectionRef_t lcr; // Local Connection reference
  OaiNwDrvRadioBearerId_t rab_id;
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
struct oai_nw_drv_msg_class_add_request {
  OaiNwDrvLocalConnectionRef_t lcr; // Local Connection reference
  OaiNwDrvRadioBearerId_t rab_id;
  OaiNwDrvRadioBearerId_t rab_id_rx;
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
  unsigned short sport;              //!< source port
  unsigned short dport;              //!< destination port
};

struct oai_nw_drv_msg_class_add_reply {
  int status;
};
struct oai_nw_drv_msg_class_del_request {
  OaiNwDrvLocalConnectionRef_t lcr; // Local Connection reference
  unsigned char dir; // direction (send or receive)
  unsigned char dscp; // codepoint
  unsigned short classref;
};
struct oai_nw_drv_msg_class_del_reply {
  int status;
};
#define oai_nw_drv_msg_class_list_reply oai_nw_drv_msg_class_add_request
struct oai_nw_drv_msg_class_list_request {
  OaiNwDrvLocalConnectionRef_t lcr;    // Local Connection reference
  unsigned char dir;
  unsigned char dscp;
};


// Messages for Measurement transfer - MW 01/04/2005
typedef unsigned int nioctlProviderId_t;
typedef unsigned short nioctlSignalLoss_t;
typedef struct nioctlMeasures {
  OaiNwDrvCellID_t cell_id;
  OaiNwDrvSigLevel_t level;
  nioctlProviderId_t provider_id;
} nioctlMeasures_t;

struct oai_nw_drv_msg_measure_request {
  OaiNwDrvNumRGsMeas_t num_cells;
  OaiNwDrvCellID_t cellid[OAI_NW_DRV_MAX_MEASURE_NB]; // Cell identification
  unsigned short num_providers;
  nioctlProviderId_t provider_id[OAI_NW_DRV_MAX_MEASURE_NB]; // Provider identification
};
struct oai_nw_drv_msg_measure_reply {
  OaiNwDrvNumRGsMeas_t num_cells;
  nioctlMeasures_t measures[OAI_NW_DRV_MAX_MEASURE_NB];
  nioctlSignalLoss_t signal_lost_flag;
};

// Messages for Measurement transfer - MW 01/04/2005
typedef unsigned int nioctlL2Id_t[2];

struct oai_nw_drv_msg_l2id_reply {
  nioctlL2Id_t l2id;
};


#endif
