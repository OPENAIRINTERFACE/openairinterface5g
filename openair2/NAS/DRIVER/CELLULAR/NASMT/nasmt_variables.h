/***************************************************************************
                          nasmt_variables.h  -  description
 ***************************************************************************
  Eurecom OpenAirInterface 2
  Copyright(c) 1999 - 2013 Eurecom

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information
  Openair Admin: openair_admin@eurecom.fr
  Openair Tech : openair_tech@eurecom.fr
  Forums       : http://forums.eurecom.fsr/openairinterface
  Address      : Eurecom, 450 route des Chappes, 06410 Biot Sophia Antipolis, France
*******************************************************************************/
/*! \file nasmt_variables.h
* \brief Variable and structure definitions for OpenAirInterface CELLULAR version - MT
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#ifndef _NASMTD_VAR_H
#define _NASMTD_VAR_H

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

#include "rrc_nas_primitives.h"
#include "rrc_qos_classes.h"
#include "rrc_nas_sap.h"

#include "nasmt_constant.h"
#include "nasmt_sap.h"

struct cx_entity;

struct rb_entity {
  uint32_t   cnxid;
  nasRadioBearerId_t rab_id;  //ue_rbId
  nasSapId_t sapi;
  nasQoSTrafficClass_t qos;
  nasQoSTrafficClass_t RadioQosClass;
  nasIPdscp_t dscp;
  uint8_t state;
  uint8_t result;
  uint8_t retry;
  uint32_t countimer;
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
  nasCellID_t cellid;           // cell identification
  uint32_t countimer;                // timeout's counter
  uint8_t retry;                     // number of retransmissions
  struct classifier_entity *sclassifier[NAS_DSCP_MAX]; // send classifiers table
  uint16_t nsclassifier;
  uint32_t iid6[2];                  // IPv6  interface identification
  uint8_t iid4;                      // IPv4 interface identification
  struct rb_entity *rb;
  uint16_t num_rb;
  //measures
  int req_prov_id[MAX_MEASURE_NB];
  int num_measures;
  int meas_cell_id[MAX_MEASURE_NB];
  int meas_level[MAX_MEASURE_NB];
  int provider_id[MAX_MEASURE_NB];
  //MBMS
  int joined_services[NASMT_MBMS_SVCES_MAX];

};

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
  uint16_t nrclassifier;
  uint32_t next_sclassref;
  int sap[NAS_SAPI_MAX];
  uint8_t xbuffer[NAS_MAX_LENGTH]; // transmission buffer
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

extern uint8_t NAS_NULL_IMEI[14];
extern uint8_t NAS_RG_IMEI[14];

//global variables shared with RRC
extern int *pt_nas_ue_irq;
//extern uint8_t nas_IMEI[14];
#endif
