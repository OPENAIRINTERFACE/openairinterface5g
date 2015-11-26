/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/
/***************************************************************************
                          local.h  -  description
                             -------------------
    copyright            : (C) 2002 by Eurecom
    email                : navid.nikaein@eurecom.fr
                          lionel.gauthier@eurecom.fr
                           knopp@eurecom.fr
 ***************************************************************************

 ***************************************************************************/

#ifndef LOCAL_H
#define LOCAL_H

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
#include <asm/param.h>
//#include <sys/sysctl.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#include <linux/icmpv6.h>
#include <linux/in.h>
#include <net/ndisc.h>


//#include "rrc_nas_primitives.h"
//#include "rrc_sap.h"

#define PDCP2PDCP_USE_RT_FIFO 21
#define NAS2PDCP_FIFO 22

#include "constant.h"
#include "sap.h"
#include "rrc_nas_primitives.h"
#include "COMMON/platform_types.h"

struct rb_entity {
  nasRadioBearerId_t rab_id;
  nasSapId_t sapi;
  nasQoSTrafficClass_t qos;
  uint8_t state;
  uint8_t retry;
  uint32_t countimer;
  struct rb_entity *next;
};

struct cx_entity {
  int sap[NAS_SAPI_CX_MAX];
  uint8_t state;      // state of the connection
  nasLocalConnectionRef_t lcr;  // Local connection reference
  nasCellID_t cellid;   // cell identification
  uint32_t countimer;     // timeout's counter
  uint8_t retry;      // number of retransmission
  struct classifier_entity *sclassifier[NAS_DSCP_MAX]; // send classifier;
  struct classifier_entity *fclassifier[NAS_DSCP_MAX]; // send classifier;
  uint16_t nsclassifier;
  uint16_t nfclassifier;
  uint32_t iid6[2];       // IPv6  interface identification
  uint8_t iid4;       // IPv4 interface identification
  struct rb_entity *rb;
  uint16_t num_rb;
  int lastRRCprimitive;
  //measures
  int req_prov_id[MAX_MEASURE_NB];
  int num_measures;
  int meas_cell_id[MAX_MEASURE_NB];
  int meas_level[MAX_MEASURE_NB];
  int provider_id[MAX_MEASURE_NB];

};

struct classifier_entity {
  uint32_t classref;
  struct classifier_entity *next;
  uint8_t version;
  union {
    struct in6_addr ipv6;
    uint32_t ipv4;
  } saddr; // IP source address
  uint8_t splen; // prefix length
  union {
    struct in6_addr ipv6;
    uint32_t ipv4;
    unsigned int mpls_label;
  } daddr; // IP destination address
  uint8_t dplen; // prefix length
  uint8_t protocol;   // high layer protocol type
  unsigned char protocol_message_type;
  uint16_t sport;   // source port
  uint16_t dport;   // destination port
  struct rb_entity *rb;      //pointer to rb_entity for sending function or receiving in case of forwarding rule
  struct rb_entity *rb_rx;   //pointer to rb_entity for receiving (in case of forwarding rule)
  nasRadioBearerId_t rab_id;            // RAB identification for sending
  nasRadioBearerId_t rab_id_rx;   // RAB identification for receiving (in case of forwarding rule)
  void (*fct)(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc,int inst);
};



//#define NAS_RETRY_LIMIT_DEFAULT 5

struct nas_priv {
  int irq;
  struct timer_list timer;
  spinlock_t lock;
  struct net_device_stats stats;
  uint8_t retry_limit;
  uint32_t timer_establishment;
  uint32_t timer_release;
  struct cx_entity cx[NAS_CX_MAX];
  struct classifier_entity *rclassifier[NAS_DSCP_MAX]; // receive classifier
  uint16_t nrclassifier;
  int sap[NAS_SAPI_MAX];
  struct sock *nl_sk;
  uint8_t nlmsg[NAS_MAX_LENGTH+sizeof(struct nlmsghdr)];
  uint8_t xbuffer[NAS_MAX_LENGTH]; // transmition buffer
  uint8_t rbuffer[NAS_MAX_LENGTH]; // reception buffer
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

typedef struct pdcp_data_req_header_s {
  rb_id_t             rb_id;
  sdu_size_t          data_size;
  signed int          inst;
  ip_traffic_type_t   traffic_type;
} pdcp_data_req_header_t;

typedef struct pdcp_data_ind_header_s {
  rb_id_t             rb_id;
  sdu_size_t          data_size;
  signed int          inst;
  ip_traffic_type_t   dummy_traffic_type;
} pdcp_data_ind_header_t;

extern struct net_device *nasdev[NB_INSTANCES_MAX];
//extern int bytes_wrote;
//extern int bytes_read;

extern uint8_t NAS_NULL_IMEI[14];

//global variables shared with RRC
#ifndef PDCP_USE_NETLINK
extern int pdcp_2_nas_irq;
#endif
//extern uint8_t nas_IMEI[14];

#endif
