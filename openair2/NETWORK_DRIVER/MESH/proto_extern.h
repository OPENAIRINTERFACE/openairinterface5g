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
                          nas_proto_extern.h  -  description
                             -------------------
    copyright            : (C) 2002 by Eurecom
    email                : navid.nikaein@eurecom.fr
                           lionel.gauthier@eurecom.fr
                           knopp@eurecom.fr

***************************************************************************/

#ifndef __NETWORK_DRIVER_MESH_PROTO_EXTERN__H__
#define __NETWORK_DRIVER_MESH_PROTO_EXTERN__H__

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

#include "local.h"

// device.c


/** @defgroup _nas_mesh_impl_ NAS Mesh Network Device
* @ingroup _ref_implementation_
* @{
\fn int find_inst(struct net_device *dev)
\brief This function determines the instance id for a particular device pointer.
@param dev Pointer to net_device structure
 */
int find_inst(struct net_device *dev);

// nas_common.c
/**
\fn void nas_COMMON_receive(unsigned short dlen, void* pdcp_sdu,int inst,struct classifier_entity *rclass,nasRadioBearerId_t rb_id)
\brief Receive data from FIFO (QOS)
@param dlen Length of SDU in bytes
@param pdcp_sdu Pointer to received SDU
@param inst Instance number
@param rclass RX Classifier entity
@param rb_id Radio Bearer Id
 */
void nas_COMMON_receive(unsigned short dlen,
                        void *pdcp_sdu,
                        int inst,
                        struct classifier_entity *rclass,
                        nasRadioBearerId_t rb_id);

/**
\fn void nas_COMMON_QOS_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc,int inst)
\brief Request the transfer of data (QoS SAP)
@param skb pointer to socket buffer
@param cx pointer to connection entity for SDU
@param gc pointer to classifier entity for SDU
@param inst device instance
 */
void nas_COMMON_QOS_send(struct sk_buff *skb,
                         struct cx_entity *cx,
                         struct classifier_entity *gc,int inst, struct nas_priv *gpriv);

/**
\fn void nas_COMMON_del_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc,int inst)
\brief  Delete the data
@param skb pointer to socket buffer
@param cx pointer to connection entity for SDU
@param gc pointer to classifier entity for SDU
@param inst device instance
 */
void nas_COMMON_del_send(struct sk_buff *skb,
                         struct cx_entity *cx,
                         struct classifier_entity *gc,int inst, struct nas_priv *gpriv);

/**
\fn struct rb_entity *nas_COMMON_add_rb(struct cx_entity *cx, nasRadioBearerId_t rabi, nasQoSTrafficClass_t qos)
\brief Add a radio-bearer descriptor
@param cx pointer to connection entity
@param rabi radio-bearer index
@param qos NAS QOS traffic class
 */
struct rb_entity *nas_COMMON_add_rb(
  struct cx_entity *cx, nasRadioBearerId_t rabi,
  nasQoSTrafficClass_t qos);

/**
\fn struct rb_entity *nas_COMMON_search_rb(struct cx_entity *cx, nasRadioBearerId_t rabi)
\brief Search for a radio-bearer entity for a particular connection and radio-bearer index
@param cx pointer to connection entity
@param rabi radio-bearer index
@returns A pointer to the radio-bearer entity
 */
struct rb_entity *nas_COMMON_search_rb(
  struct cx_entity *cx, nasRadioBearerId_t rabi);

/**
\fn struct cx_entity *nas_COMMON_search_cx(nasLocalConnectionRef_t lcr,struct nas_priv *gpriv)
\brief  Search for a connection entity based on its index and pointer to nas_priv
@param lcr index of local connection
@param gpriv pointer to nas_priv for device
@returns A pointer to the connection entity
 */
struct cx_entity *nas_COMMON_search_cx(
  nasLocalConnectionRef_t lcr,
  struct nas_priv *gpriv);

/**
\fn struct classifier_entity *nas_COMMON_search_class_for_rb(nasRadioBearerId_t rab_id,struct nas_priv *priv)
\brief  Search for an RX classifier entity based on a RB id and pointer to nas_priv
@param rab_id Index of RAB for search
@param priv pointer to nas_priv for device
@returns A pointer to the corresponding RX classifier entity
 */
struct classifier_entity
*nas_COMMON_search_class_for_rb(nasRadioBearerId_t
                                rab_id,struct nas_priv *priv);

/**
\fn void nas_COMMON_flush_rb(struct cx_entity *cx)
\brief Clear all RB's for a particular connection
@param cx pointer to connection entity
 */
void nas_COMMON_flush_rb(struct cx_entity *cx);

/**
\fn int nas_netlink_send(unsigned char *data,unsigned int len)
\brief Request the transfer of data by PDCP via netlink socket
@param data pointer to SDU
@param len length of SDU in bytes
@returns Numeber of bytes transfered by netlink socket
 */
int nas_netlink_send(unsigned char *data,unsigned int len);

/**
\fn void nas_COMMON_QOS_receive(struct nlmsghdr *nlh)
\brief Request a PDU from PDCP
@param nlh pointer to netlink message header
 */
void nas_COMMON_QOS_receive(struct nlmsghdr *nlh);

//nasmesh.c

/**
  \brief Initialize an interface for a particular instance ID.
*/
void nas_mesh_init(int inst   //!< Instance ID

                  );


int  nas_mesh_DC_receive(struct cx_entity *cx,struct nas_priv *gpriv);
int  nas_mesh_GC_receive(struct nas_priv *gpriv);
int  nas_mesh_DC_send_cx_establish_request(struct cx_entity *cx,struct nas_priv *gpriv);
int  nas_mesh_DC_send_cx_release_request(struct cx_entity *cx,struct nas_priv *gpriv);
void nas_mesh_DC_send_sig_data_request(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc,int inst, struct nas_priv *gpriv);

// iocontrol.c
void nas_CTL_send(struct sk_buff *skb,
                  struct cx_entity *cx,
                  struct classifier_entity *gc,int inst, struct nas_priv *gpriv);
//int nas_CTL_receive_authentication(struct ipv6hdr *iph, struct cx-entity *cx, unsigned char sapi);
int nas_CTL_ioctl(struct net_device *dev,
                  struct ifreq *ifr, int cmd);

// classifier.c

/**
  \brief Send a socket received from IP to classifier for a particular instance ID.
*/
void nas_CLASS_send(struct sk_buff *skb,    //!< Pointer to socket buffer
                    int inst                //!< Instance ID
                   );
/**
  \brief
*/
struct classifier_entity *nas_CLASS_add_sclassifier(struct cx_entity *cx, unsigned char dscp, unsigned short classref);

/**
  \brief Send a socket received from IP to classifier for a particular instance ID.
*/
struct classifier_entity
*nas_CLASS_add_fclassifier(struct cx_entity *cx,
    unsigned char dscp,
    unsigned short classref
                                                   );

/**
  \brief Send a socket received from IP to classifier for a particular instance ID.
*/
struct classifier_entity
*nas_CLASS_add_rclassifier(unsigned char dscp,
    unsigned short classref,
    struct nas_priv*
                                                   );

/**
  \brief
*/
void nas_CLASS_del_sclassifier(struct cx_entity *cx,
                               unsigned char dscp,
                               unsigned short classref
                              );

/**
  \brief
*/
void nas_CLASS_del_fclassifier(struct cx_entity *cx,
                               unsigned char dscp,
                               unsigned short classref
                              );

/**
  \brief
*/
void nas_CLASS_del_rclassifier(unsigned char dscp,
                               unsigned short classref,
                               struct nas_priv*
                              );

/**
  \brief
*/
void nas_CLASS_flush_sclassifier(struct cx_entity *cx
                                );

/**
  \brief
*/
void nas_CLASS_flush_fclassifier(struct cx_entity *cx
                                );

/**
  \brief
*/
void nas_CLASS_flush_rclassifier(struct nas_priv *gpriv
                                );


// nas_tool.c
unsigned char nas_TOOL_invfct(struct classifier_entity *gc);
void nas_TOOL_fct(struct classifier_entity *gc, unsigned char fct);
void nas_TOOL_imei2iid(unsigned char *imei, unsigned char *iid);
unsigned char nas_TOOL_get_dscp6(struct ipv6hdr *iph);
unsigned char nas_TOOL_get_dscp4(struct iphdr *iph);
unsigned char *nas_TOOL_get_protocol6(struct ipv6hdr *iph, unsigned char *protocol);
unsigned char *nas_TOOL_get_protocol4(struct iphdr *iph, unsigned char *protocol);
char *nas_TOOL_get_udpmsg(struct udphdr *udph);
unsigned short nas_TOOL_udpcksum(struct in6_addr *saddr, struct in6_addr *daddr, unsigned char proto, unsigned int udplen, void *data);
int nas_TOOL_network6(struct in6_addr *addr, struct in6_addr *prefix, unsigned char plen);
int nas_TOOL_network4(unsigned int *addr, unsigned int *prefix, unsigned char plen);
void print_TOOL_pk_icmp6(struct icmp6hdr *icmph);

void print_TOOL_pk_all(struct sk_buff *skb);
void print_TOOL_pk_ipv6(struct ipv6hdr *iph);
void print_TOOL_state(unsigned char state);
void nas_tool_print_buffer(char * buffer,int length);
void nas_print_rb_entity(struct rb_entity *rb);
void nas_print_classifier(struct classifier_entity *gc);

// nas_netlink.c

void nas_netlink_release(void);
int nas_netlink_init(void);

/** @} */
#endif
