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

/*! \file nasrg_variables.h
* \brief Variable and structure definitions for OpenAirInterface CELLULAR version - RG
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#ifndef _NASRGD_VAR_H
#define _NASRGD_VAR_H

#include <linux/if_arp.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/ipv6.h>
#include <linux/ip.h>
#include <linux/sysctl.h>
#include <linux/timer.h>
#include <linux/unistd.h>
//#include <asm/unistd.h>
#include <asm/param.h>
//#include <sys/sysctl.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#include <linux/icmpv6.h>
#include <linux/in.h>
#include <net/ndisc.h>

#include "rrc_nas_primitives.h"
#include "rrc_qos_classes.h"
#include "rrc_nas_sap.h"

#include "nasrg_constant.h"
#include "nasrg_sap.h"

struct cx_entity;

struct rb_entity {
  //  uint16_t default_rab;
  uint32_t   cnxid;
  nasRadioBearerId_t rab_id;  //ue_rbId
  nasRadioBearerId_t rg_rbId;
  nasRadioBearerId_t mbms_rbId;
  nasSapId_t sapi;
  nasQoSTrafficClass_t qos;
  nasQoSTrafficClass_t RadioQosClass;
  nasIPdscp_t dscp;  //this is DL dscp
  nasIPdscp_t dscp_ul;
  uint8_t state;
  uint8_t result;
  uint8_t retry;
  uint32_t countimer;
  //for MBMS
  uint16_t serviceId;
  uint16_t sessionId;
  uint16_t duration;
  uint8_t  mcast_address[16];
  //
  struct rb_entity *next;
};

struct classifier_entity {
  uint32_t classref;               // classifier identity
  uint8_t version;                 // IP version 4 or 6
  union {
    struct in6_addr ipv6;
    uint32_t ipv4;
  } saddr;                    // IP source address
  uint8_t splen;                   // IP prefix size
  union {
    struct in6_addr ipv6;
    uint32_t ipv4;
  } daddr;                    // IP destination address
  uint8_t dplen;                   // IP prefix size
  uint8_t protocol;                 // layer 4 protocol type (tcp, udp, ...)
  uint16_t sport;                   // source port
  uint16_t dport;                   // destination port
  struct rb_entity *rb;
  nasRadioBearerId_t rab_id;  // RAB identification
  void (*fct)(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc);
  struct classifier_entity *next;
};


struct cx_entity {
  int sap[NAS_SAPI_CX_MAX];
  uint8_t state;                     // state of the connection
  nasLocalConnectionRef_t lcr;  // Local connection reference
  nasCellID_t cellid;            // cell identification
  uint32_t countimer;                // timeout's counter
  uint8_t retry;                      // number of retransmissions
  struct classifier_entity *sclassifier[NAS_DSCP_MAX]; // send classifiers table
  uint16_t nsclassifier;
  uint32_t iid6[2];                   // IPv6  interface identification
  uint8_t iid4;                       // IPv4 interface identification
  struct rb_entity *rb;         // RB entities for RABs
  uint16_t num_rb;
  // MW - 17/5/05
  int  ue_id;
  int rrc_state;
  // MBMS
  int requested_joined_services[NASRG_MBMS_SVCES_MAX];
  int requested_left_services[NASRG_MBMS_SVCES_MAX];
  int joined_services[NASRG_MBMS_SVCES_MAX];
};

//struct mbms_rb_entity{
//  uint32_t   cnxid;
//  nasRadioBearerId_t mbms_rbId;
//  nasSapId_t sapi;
//  nasQoSTrafficClass_t qos;
//  nasQoSTrafficClass_t RadioQosClass;
//  uint8_t state;
//  uint8_t result;
//  uint8_t retry;
//  uint32_t countimer;
//
//  uint16_t serviceId;
//  uint16_t sessionId;
//  uint16_t duration;
//};
//

struct nas_priv {
  int irq;
  int rx_flags;
  struct timer_list timer;
  spinlock_t lock;
  struct net_device_stats stats;
  uint8_t retry_limit;
  uint32_t timer_establishment;
  uint32_t timer_release;
  struct cx_entity cx[NAS_CX_MAX];
  struct classifier_entity *rclassifier[NAS_DSCP_MAX]; // receive classifier
  int nrclassifier;
  uint32_t next_sclassref;
  int sap[NAS_SAPI_MAX];
  uint8_t xbuffer[NAS_MAX_LENGTH]; // transmission buffer
  uint8_t rbuffer[NAS_MAX_LENGTH]; // reception buffer
  // MW - 17/5/05
  int broadcast_counter;
  int SIB18_counter;
  // MBMS
  struct rb_entity mbms_rb[NASRG_MBMS_SVCES_MAX];
  struct classifier_entity *mbmsclassifier[NASRG_MBMS_SVCES_MAX]; // mbms classifier
  int nmbmsclassifier;
  uint32_t next_mbmsclassref;
  //Added for demo 3 - MW
  nasCellID_t measured_cell_id;
  uint16_t num_UEs;
  uint32_t rlcBufferOccupancy[NAS_CX_MAX];
  uint32_t scheduledPRB[NAS_CX_MAX];
  uint32_t totalDataVolume[NAS_CX_MAX];
  uint32_t totalNumPRBs;
};

struct ipversion {
#if defined(__LITTLE_ENDIAN_BITFIELD)
  uint8_t    reserved:4,
             version:4;
#else
  uint8_t    version:4,
             reserved:4;
#endif
};

typedef struct pdcp_data_req_header_t {
  unsigned int             rb_id;
  unsigned int           data_size;
  int            inst;
} pdcp_data_req_header_t;

typedef struct pdcp_data_ind_header_t {
  unsigned int            rb_id;
  unsigned int           data_size;
  int            inst;
} pdcp_data_ind_header_t;
//
extern struct nas_priv *gpriv;
extern struct net_device *gdev;
//extern int bytes_wrote;
//extern int bytes_read;

extern uint8_t NAS_RG_IMEI[14];
extern uint8_t NAS_NULL_IMEI[14];

//global variables shared with RRC
extern int *pt_nas_rg_irq;
//extern uint16_t *pt_rg_own_cell_id;
extern uint16_t local_rg_cell_id;
#endif
