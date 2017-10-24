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

/*! \file nasmt_proto.h
* \brief Function prototypes for OpenAirInterface CELLULAR version - MT
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#ifndef _NASMTD_PROTO_H
#define _NASMTD_PROTO_H

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

//#include "rrc_nas_primitives.h"
//#include "protocol_vars_extern.h"
//#include "as_sap.h"
//#include "rrc_qos.h"
//#include "rrc_sap.h"

// nasmt_netlink.c
void nasmt_netlink_release(void);
int nasmt_netlink_init(void);
int nasmt_netlink_send(unsigned char *data_buffer, unsigned int data_length, int destination);

// nasmt_common.c
//void nasmt_COMMON_receive(uint16_t hlen, uint16_t dlength, int sap);
void nasmt_COMMON_receive(uint16_t bytes_read, uint16_t payload_length, void *data_buffer, int rb_id, int sap);

void nasmt_COMMON_QOS_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc);
void nasmt_COMMON_del_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc);
#ifndef PDCP_USE_NETLINK
void nasmt_COMMON_QOS_receive(struct cx_entity *cx);
#else
void nasmt_COMMON_QOS_receive(struct nlmsghdr *nlh);
#endif
struct rb_entity *nasmt_COMMON_add_rb(struct cx_entity *cx, nasRadioBearerId_t rabi, nasQoSTrafficClass_t qos);
struct rb_entity *nasmt_COMMON_search_rb(struct cx_entity *cx, nasRadioBearerId_t rabi);
struct cx_entity *nasmt_COMMON_search_cx(nasLocalConnectionRef_t lcr);
void nasmt_COMMON_del_rb(struct cx_entity *cx, nasRadioBearerId_t rab_id, nasIPdscp_t dscp);
void nasmt_COMMON_flush_rb(struct cx_entity *cx);


//nasmt_ascontrol.c
void nasmt_ASCTL_init(void);
void nasmt_ASCTL_timer(unsigned long data);
int  nasmt_ASCTL_DC_receive(struct cx_entity *cx, char *buffer);
int  nasmt_ASCTL_GC_receive(char *buffer);
int  nasmt_ASCTL_DC_send_cx_establish_request(struct cx_entity *cx);
int  nasmt_ASCTL_DC_send_cx_release_request(struct cx_entity *cx);
void nasmt_ASCTL_DC_send_sig_data_request(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc);
void nasmt_ASCTL_DC_send_peer_sig_data_request(struct cx_entity *cx, uint8_t sig_category);
int nasmt_ASCTL_leave_sleep_mode(struct cx_entity *cx);
int nasmt_ASCTL_enter_sleep_mode(struct cx_entity *cx);

// nasmt_iocontrol.c
void nasmt_CTL_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc);
int nasmt_CTL_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);

// nasmt_classifier.c
void nasmt_CLASS_send(struct sk_buff *skb);
struct classifier_entity *nasmt_CLASS_add_sclassifier(struct cx_entity *cx, uint8_t dscp, uint16_t classref);
struct classifier_entity *nasmt_CLASS_add_rclassifier(uint8_t dscp, uint16_t classref);
void nasmt_CLASS_del_sclassifier(struct cx_entity *cx, uint8_t dscp, uint16_t classref);
void nasmt_CLASS_del_rclassifier(uint8_t dscp, uint16_t classref);
void nasmt_CLASS_flush_sclassifier(struct cx_entity *cx);
void nasmt_CLASS_flush_rclassifier(void);

// nasmt_tool.c
uint8_t nasmt_TOOL_invfct(struct classifier_entity *gc);
void nasmt_TOOL_fct(struct classifier_entity *gc, uint8_t fct);
void nasmt_TOOL_imei2iid(uint8_t *imei, uint8_t *iid);

void nasmt_TOOL_eth_imei2iid(unsigned char *imei, unsigned char *addr ,unsigned char *iid, unsigned char len);
uint8_t nasmt_TOOL_get_dscp6(struct ipv6hdr *iph);
uint8_t nasmt_TOOL_get_dscp4(struct iphdr *iph);
uint8_t *nasmt_TOOL_get_protocol6(struct ipv6hdr *iph, uint8_t *protocol);
uint8_t *nasmt_TOOL_get_protocol4(struct iphdr *iph, uint8_t *protocol);

void nasmt_TOOL_pk_icmp6(struct icmp6hdr *icmph);

void nasmt_TOOL_print_state(uint8_t state);
void nasmt_TOOL_print_buffer(unsigned char * buffer,int length);
void nasmt_TOOL_print_rb_entity(struct rb_entity *rb);
void nasmt_TOOL_print_classifier(struct classifier_entity *gc);

#endif
