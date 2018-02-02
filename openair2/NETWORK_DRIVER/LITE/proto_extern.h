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
                          proto_extern.h  -  description
                             -------------------
    copyright            : (C) 2002 by Eurecom
    email                : michelle.wetterwald@eurecom.fr
                           yan.moret@eurecom.fr
                           knopp@eurecom.fr
               ***************************************************************************

***************************************************************************/

#ifndef _NAS_PROTO_H
#define _NAS_PROTO_H

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


/** @defgroup _oai_nw_drv_impl_ OAI Network Device for RRC Lite
* @ingroup _ref_implementation_
* @{
\fn int oai_nw_drv_find_inst(struct net_device *dev)
\brief This function determines the instance id for a particular device pointer.
@param dev Pointer to net_device structure
 */
int oai_nw_drv_find_inst(struct net_device *dev);

// common.c
/**
\fn void oai_nw_drv_common_class_wireless2ip(unsigned short dlen, void* pdcp_sdu,int inst,OaiNwDrvRadioBearerId_t rb_id)
\brief Receive classified LTE packet, build skbuff struct with it and deliver it to the OS network layer.
@param dlen Length of SDU in bytes
@param pdcp_sdu Pointer to received SDU
@param inst Instance number
@param rb_id Radio Bearer Id
 */
void oai_nw_drv_common_class_wireless2ip(unsigned short dlen,
    void *pdcp_sdu,
    int inst,
    OaiNwDrvRadioBearerId_t rb_id);

/**
\fn void oai_nw_drv_common_ip2wireless(struct sk_buff *skb, int inst)
\brief Request the transfer of data (QoS SAP)
@param skb pointer to socket buffer
@param inst device instance
 */
void oai_nw_drv_common_ip2wireless(struct sk_buff *skb, int inst);

/**
\fn void oai_nw_drv_common_ip2wireless_drop(struct sk_buff *skb, int inst)
\brief  Drop the IP packet comming from the OS network layer.
@param skb pointer to socket buffer
@param inst device instance
 */
void oai_nw_drv_common_ip2wireless_drop(struct sk_buff *skb, int inst);

#ifndef OAI_NW_DRIVER_USE_NETLINK
/**
\fn void oai_nw_drv_common_wireless2ip()
\brief Retrieve PDU from PDCP through RT-fifos for delivery to the IP stack.
 */
void oai_nw_drv_common_wireless2ip(void);
#else
/**
\fn void oai_nw_drv_common_wireless2ip(struct nlmsghdr *nlh)
\brief Retrieve PDU from PDCP through netlink sockets for delivery to the IP stack.
 */
void oai_nw_drv_common_wireless2ip(struct nlmsghdr *nlh);
#endif //OAI_NW_DRIVER_USE_NETLINK


#ifdef OAI_NW_DRIVER_USE_NETLINK
/**
\fn int oai_nw_drv_netlink_send(unsigned char *data,unsigned int len)
\brief Request the transfer of data by PDCP via netlink socket
@param data pointer to SDU
@param len length of SDU in bytes
@returns Numeber of bytes transfered by netlink socket
 */
int oai_nw_drv_netlink_send(unsigned char *data,unsigned int len);

/**
\fn void oai_nw_drv_COMMON_QOS_receive(struct nlmsghdr *nlh)
\brief Request a PDU from PDCP
@param nlh pointer to netlink message header
 */
void oai_nw_drv_COMMON_QOS_receive(struct nlmsghdr *nlh);

#endif //OAI_NW_DRIVER_USE_NETLINK



// iocontrol.c
void oai_nw_drv_CTL_send(struct sk_buff *skb, int inst);
int oai_nw_drv_CTL_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);

// classifier.c

/**
  \brief Find the IP traffic type (UNICAST, MULTICAST, BROADCAST) of the IP packet attached to sk_buff.
*/
ip_traffic_type_t oai_nw_drv_find_traffic_type(struct sk_buff  *skb);



// tool.c
void oai_nw_drv_TOOL_imei2iid(unsigned char *imei, unsigned char *iid);
void oai_nw_drv_TOOL_eNB_imei2iid(unsigned char *imei, unsigned char *iid, unsigned char len);
unsigned char oai_nw_drv_TOOL_get_dscp6(struct ipv6hdr *iph);
unsigned char oai_nw_drv_TOOL_get_dscp4(struct iphdr *iph);
unsigned char *oai_nw_drv_TOOL_get_protocol6(struct ipv6hdr *iph, unsigned char *protocol);
unsigned char *oai_nw_drv_TOOL_get_protocol4(struct iphdr *iph, unsigned char *protocol);
char *oai_nw_drv_TOOL_get_udpmsg(struct udphdr *udph);
unsigned short oai_nw_drv_TOOL_udpcksum(struct in6_addr *saddr, struct in6_addr *daddr, unsigned char proto, unsigned int udplen, void *data);
int oai_nw_drv_TOOL_network6(struct in6_addr *addr, struct in6_addr *prefix, unsigned char plen);
int oai_nw_drv_TOOL_network4(unsigned int *addr, unsigned int *prefix, unsigned char plen);
void print_TOOL_pk_icmp6(struct icmp6hdr *icmph);

void print_TOOL_pk_all(struct sk_buff *skb);
void print_TOOL_pk_ipv6(struct ipv6hdr *iph);
void print_TOOL_state(unsigned char state);
void oai_nw_drv_tool_print_buffer(char * buffer,int length);
#ifdef OAI_NW_DRIVER_USE_NETLINK
// netlink.c

void oai_nw_drv_netlink_release(void);
int oai_nw_drv_netlink_init(void);

#endif

/** @} */
#endif
