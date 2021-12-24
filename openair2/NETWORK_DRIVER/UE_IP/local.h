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
                          local.h  -  description
                             -------------------
    copyright            : (C) 2002 by Eurecom
    email                : navid.nikaein@eurecom.fr
                          lionel.gauthier@eurecom.fr,
                           knopp@eurecom.fr

 ***************************************************************************/

#ifndef UE_IP_LOCAL_H
#define UE_IP_LOCAL_H

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

#include "constant.h"
#include "platform_types.h"
#include "sap.h"

typedef struct ue_ip_priv_s {
  int                        irq;
  int                        rx_flags;
  struct timer_list          timer;
  spinlock_t                 lock;
  struct net_device_stats    stats;
  uint8_t                    retry_limit;
  uint32_t                   timer_establishment;
  uint32_t                   timer_release;
  struct sock               *nl_sk;
  uint8_t                    nlmsg[UE_IP_PRIMITIVE_MAX_LENGTH+sizeof(struct nlmsghdr)];
  uint8_t                    xbuffer[UE_IP_PRIMITIVE_MAX_LENGTH]; // transmission buffer
  uint8_t                    rbuffer[UE_IP_PRIMITIVE_MAX_LENGTH]; // reception buffer
} ue_ip_priv_t;

typedef struct ipversion_s {
#if defined(__LITTLE_ENDIAN_BITFIELD)
  uint8_t    reserved:4,
             version:4;
#else
  uint8_t    version:4,
             reserved:4;
#endif
} ipversion_t;


typedef struct pdcp_data_req_header_s {
  rb_id_t             rb_id;
  sdu_size_t          data_size;
  signed int          inst;
  ip_traffic_type_t   traffic_type;
  uint32_t sourceL2Id;
  uint32_t destinationL2Id;
} pdcp_data_req_header_t;

typedef struct pdcp_data_ind_header_s {
  rb_id_t             rb_id;
  sdu_size_t          data_size;
  signed int          inst;
  ip_traffic_type_t   dummy_traffic_type;
  uint32_t sourceL2Id;
  uint32_t destinationL2Id;
} pdcp_data_ind_header_t;



extern struct net_device *ue_ip_dev[UE_IP_NB_INSTANCES_MAX];


#endif
