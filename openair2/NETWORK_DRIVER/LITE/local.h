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
    email                :navid.nikaein@eurecom.fr
                          lionel.gauthier@eurecom.fr
                           knopp@eurecom.fr
 ***************************************************************************

 ***************************************************************************/

#ifndef OAI_LOCAL_H
#define OAI_LOCAL_H

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

#define PDCP2IP_FIFO 21
#define IP2PDCP_FIFO 22

#include "constant.h"
#include "platform_types.h"
#include "sap.h"

struct cx_entity {
  int                        sap[OAI_NW_DRV_SAPI_CX_MAX];
  uint8_t                         state;                     // state of the connection
  OaiNwDrvLocalConnectionRef_t    lcr;                       // Local connection reference
  OaiNwDrvCellID_t                cellid;                    // cell identification
  uint32_t                        countimer;                 // timeout's counter
  uint8_t                         retry;                     // number of retransmission

  uint32_t                        iid6[2];                   // IPv6  interface identification
  uint8_t                         iid4;                      // IPv4 interface identification
  int                        lastRRCprimitive;
  //measures
  int                        req_prov_id [OAI_NW_DRV_MAX_MEASURE_NB];
  int                        num_measures;
  int                        meas_cell_id[OAI_NW_DRV_MAX_MEASURE_NB];
  int                        meas_level  [OAI_NW_DRV_MAX_MEASURE_NB];
  int                        provider_id [OAI_NW_DRV_MAX_MEASURE_NB];
};

struct oai_nw_drv_priv {
  int                        irq;
  int                        rx_flags;
  struct timer_list          timer;
  spinlock_t                 lock;
  struct net_device_stats    stats;
  uint8_t                         retry_limit;
  uint32_t                        timer_establishment;
  uint32_t                        timer_release;
  struct cx_entity           cx[OAI_NW_DRV_CX_MAX];
  //struct classifier_entity  *rclassifier[OAI_NW_DRV_DSCP_MAX]; // receive classifier
  uint16_t                        nrclassifier;
  int                        sap[OAI_NW_DRV_SAPI_MAX];
  struct sock               *nl_sk;
  uint8_t                         nlmsg[OAI_NW_DRV_PRIMITIVE_MAX_LENGTH+sizeof(struct nlmsghdr)];
  uint8_t                         xbuffer[OAI_NW_DRV_PRIMITIVE_MAX_LENGTH]; // transmition buffer
  uint8_t                         rbuffer[OAI_NW_DRV_PRIMITIVE_MAX_LENGTH]; // reception buffer
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



extern struct net_device *oai_nw_drv_dev[OAI_NW_DRV_NB_INSTANCES_MAX];

extern uint8_t OAI_NW_DRV_NULL_IMEI[14];



#endif
