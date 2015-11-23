/***************************************************************************
                          nasrg_proto.h  -  description
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
/*! \file nasrg_proto.h
* \brief Function prototypes for OpenAirInterface CELLULAR version - RG
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#ifndef _NASRGD_PROTO_H
#define _NASRGD_PROTO_H

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

// #include "rrc_nas_primitives.h"
// #include "protocol_vars_extern.h"
// #include "as_sap.h"
// #include "rrc_qos.h"
// #include "rrc_sap.h"

// nasrg_netlink.c
void nasrg_netlink_release(void);
int nasrg_netlink_init(void);
int nasrg_netlink_send(unsigned char *data_buffer, unsigned int data_length, int destination);

// nasrg_common.c
//void nasrg_COMMON_receive(uint16_t hlen, uint16_t dlength, int sap);
void nasrg_COMMON_receive(uint16_t bytes_read, uint16_t payload_length, void *data_buffer, int rb_id, int sap);

void nasrg_COMMON_QOS_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc);
void nasrg_COMMON_QOS_send_test_netlink(struct sk_buff *skb);
void nasrg_COMMON_del_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc);
#ifndef PDCP_USE_NETLINK
void nasrg_COMMON_QOS_receive(struct cx_entity *cx);
#else
void nasrg_COMMON_QOS_receive(struct nlmsghdr *nlh);
#endif
struct rb_entity *nasrg_COMMON_add_rb(struct cx_entity *cx, nasRadioBearerId_t rabi, nasQoSTrafficClass_t qos);
struct rb_entity *nasrg_COMMON_search_rb(struct cx_entity *cx, nasRadioBearerId_t rabi);
struct cx_entity *nasrg_COMMON_search_cx(nasLocalConnectionRef_t lcr);
void nasrg_COMMON_del_rb(struct cx_entity *cx, nasRadioBearerId_t rab_id, nasIPdscp_t dscp);
void nasrg_COMMON_flush_rb(struct cx_entity *cx);

//nasrg_ascontrol.c
void nasrg_ASCTL_init(void);
void nasrg_ASCTL_timer(unsigned long data);
void nasrg_ASCTL_DC_send_sig_data_request(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc);
//int nasrg_ASCTL_DC_receive(struct cx_entity *cx);
int nasrg_ASCTL_DC_receive(struct cx_entity *cx, char *buffer);
//
int nasrg_ASCTL_DC_send_cx_establish_confirm(struct cx_entity *cx, uint8_t response);
int nasrg_ASCTL_DC_send_rb_establish_request(struct cx_entity *cx, struct rb_entity *rb);
int nasrg_ASCTL_DC_send_rb_release_request(struct cx_entity *cx, struct rb_entity *rb);
int nasrg_ASCTL_GC_send_mbms_bearer_establish_req(int mbms_ix );
int nasrg_ASCTL_GC_send_mbms_bearer_release_req(int mbms_ix);
int nasrg_ASCTL_DC_send_mbms_ue_notify_req(struct cx_entity *cx);
int nasrg_ASCTL_DC_send_eNBmeasurement_req(struct cx_entity *cx);

void nasrg_ASCTL_start_default_mbms_service(void);

// nasrg_iocontrol.c
void nasrg_CTL_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc);
int nasrg_CTL_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);

// nasrg_classifier.c
void nasrg_CLASS_send(struct sk_buff *skb);
struct classifier_entity *nasrg_CLASS_add_sclassifier(struct cx_entity *cx, uint8_t dscp, uint16_t classref);
struct classifier_entity *nasrg_CLASS_add_rclassifier(uint8_t dscp, uint16_t classref);
struct classifier_entity *nasrg_CLASS_add_mbmsclassifier(int mbms_ix, uint16_t classref);
void nasrg_CLASS_del_sclassifier(struct cx_entity *cx, uint8_t dscp, uint16_t classref);
void nasrg_CLASS_del_rclassifier(uint8_t dscp, uint16_t classref);
void nasrg_CLASS_del_mbmsclassifier(int mbms_ix, uint16_t classref);
void nasrg_CLASS_flush_sclassifier(struct cx_entity *cx);
void nasrg_CLASS_flush_rclassifier(void);
void nasrg_CLASS_flush_mbmsclassifier(void);

// nasrg_tool.c
uint8_t nasrg_TOOL_invfct(struct classifier_entity *gc);
void nasrg_TOOL_fct(struct classifier_entity *gc, uint8_t fct);
void nasrg_TOOL_imei2iid(uint8_t *imei, uint8_t *iid);
void nasrg_TOOL_RGimei2iid(uint8_t *imei, uint8_t *iid);
uint8_t nasrg_TOOL_get_dscp6(struct ipv6hdr *iph);
uint8_t nasrg_TOOL_get_dscp4(struct iphdr *iph);
uint8_t *nasrg_TOOL_get_protocol6(struct ipv6hdr *iph, uint8_t *protocol);
uint8_t *nasrg_TOOL_get_protocol4(struct iphdr *iph, uint8_t *protocol);
char *nasrg_TOOL_get_udpmsg(struct udphdr *udph);
uint16_t nasrg_TOOL_udpcksum(struct in6_addr *saddr, struct in6_addr *daddr, uint8_t proto, uint32_t udplen, void *data);
int nasrg_TOOL_network6(struct in6_addr *addr, struct in6_addr *prefix, uint8_t plen);
int nasrg_TOOL_network4(uint32_t *addr, uint32_t *prefix, uint8_t plen);

void nasrg_TOOL_pk_icmp6(struct icmp6hdr *icmph);

void nasrg_TOOL_pk_all(struct sk_buff *skb);
void nasrg_TOOL_pk_ipv6(struct ipv6hdr *iph);
void nasrg_TOOL_print_state(uint8_t state);
void nasrg_TOOL_print_buffer(unsigned char * buffer,int length);
void nasrg_TOOL_print_rb_entity(struct rb_entity *rb);
void nasrg_TOOL_print_classifier(struct classifier_entity *gc);

#endif
