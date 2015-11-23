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
                          proto_extern.h  -  description
                             -------------------
    copyright            : (C) 2002 by Eurecom
    email                : navid.nikaein@eurecom.fr
                           lionel.gauthier@eurecom.fr
                           knopp@eurecom.fr

***************************************************************************/

#ifndef _UE_IP_PROTO_H
#define _UE_IP_PROTO_H

#include <linux/if_arp.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/ipv6.h>
#include <linux/ip.h>
#include <linux/sysctl.h>
#include <linux/timer.h>
#include <asm/param.h>
//#include <sys/sysctl.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#include <linux/icmpv6.h>
#include <linux/in.h>
#include <net/ndisc.h>

#include "local.h"

// device.c


/** @defgroup _ue_ip_impl_ OAI Network Device for RRC Lite
* @ingroup _ref_implementation_
* @{
\fn int ue_ip_find_inst(struct net_device *dev)
\brief This function determines the instance id for a particular device pointer.
@param dev Pointer to net_device structure
 */
int ue_ip_find_inst(struct net_device *dev);

// common.c
/**
\fn void ue_ip_common_class_wireless2ip(unsigned short dlen, void* pdcp_sdu,int inst,struct classifier_entity *rclass,OaiNwDrvRadioBearerId_t rb_id)
\brief Receive classified LTE packet, build skbuff struct with it and deliver it to the OS network layer.
@param dlen Length of SDU in bytes
@param pdcp_sdu Pointer to received SDU
@param inst Instance number
@param rb_id Radio Bearer Id
 */
void ue_ip_common_class_wireless2ip(sdu_size_t dlen,
                                    void    *pdcp_sdu,
                                    int     inst,
                                    rb_id_t rb_id);

/**
\fn void ue_ip_common_ip2wireless(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc,int inst)
\brief Request the transfer of data (QoS SAP)
@param skb pointer to socket buffer
@param inst device instance
 */
void ue_ip_common_ip2wireless(struct sk_buff *skb, int inst);

/**
\fn void ue_ip_common_ip2wireless_drop(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc,int inst)
\brief  Drop the IP packet comming from the OS network layer.
@param skb pointer to socket buffer
@param inst device instance
 */
void ue_ip_common_ip2wireless_drop(struct sk_buff *skb, int inst);

#ifndef OAI_NW_DRIVER_USE_NETLINK
/**
\fn void ue_ip_common_wireless2ip()
\brief Retrieve PDU from PDCP through RT-fifos for delivery to the IP stack.
 */
void ue_ip_common_wireless2ip(void);
#else
/**
\fn void ue_ip_common_wireless2ip(struct nlmsghdr *nlh)
\brief Retrieve PDU from PDCP through netlink sockets for delivery to the IP stack.
 */
void ue_ip_common_wireless2ip(struct nlmsghdr *nlh);
#endif //OAI_NW_DRIVER_USE_NETLINK


#ifdef OAI_NW_DRIVER_USE_NETLINK
/**
\fn int ue_ip_netlink_send(unsigned char *data,unsigned int len)
\brief Request the transfer of data by PDCP via netlink socket
@param data pointer to SDU
@param len length of SDU in bytes
@returns Numeber of bytes transfered by netlink socket
 */
int ue_ip_netlink_send(unsigned char *data,unsigned int len);

/**
\fn void ue_ip_COMMON_QOS_receive(struct nlmsghdr *nlh)
\brief Request a PDU from PDCP
@param nlh pointer to netlink message header
 */
void ue_ip_COMMON_QOS_receive(struct nlmsghdr *nlh);

#endif //OAI_NW_DRIVER_USE_NETLINK


// netlink.c

void ue_ip_netlink_release(void);
int ue_ip_netlink_init(void);


/** @} */
#endif
