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

/*! \file common.c
* \brief
* \author Navid Nikaein and Raymond Knopp, Lionel GAUTHIER
* \date 2013
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr, lionel.gauthier@eurecom.fr
*/

#include "local.h"
#include "proto_extern.h"
#ifndef OAI_NW_DRIVER_USE_NETLINK
#include "rtai_fifos.h"
#endif


#include <linux/inetdevice.h>

#include <net/tcp.h>
#include <net/udp.h>

#define NIPADDR(addr) \
        (uint8_t)(addr & 0x000000FF), \
        (uint8_t)((addr & 0x0000FF00) >> 8), \
        (uint8_t)((addr & 0x00FF0000) >> 16), \
        (uint8_t)((addr & 0xFF000000) >> 24)

#define NIP6ADDR(addr) \
        ntohs((addr)->s6_addr16[0]), \
        ntohs((addr)->s6_addr16[1]), \
        ntohs((addr)->s6_addr16[2]), \
        ntohs((addr)->s6_addr16[3]), \
        ntohs((addr)->s6_addr16[4]), \
        ntohs((addr)->s6_addr16[5]), \
        ntohs((addr)->s6_addr16[6]), \
        ntohs((addr)->s6_addr16[7])


//#define OAI_DRV_DEBUG_SEND
//#define OAI_DRV_DEBUG_RECEIVE

void
ue_ip_common_class_wireless2ip(
  sdu_size_t data_lenP,
  void      *pdcp_sdu_pP,
  int        instP,
  rb_id_t    rb_idP)
{

  //---------------------------------------------------------------------------
  struct sk_buff      *skb_p           = NULL;
  ipversion_t         *ipv_p           = NULL;
  ue_ip_priv_t        *gpriv_p         = netdev_priv(ue_ip_dev[instP]);
  unsigned int         hard_header_len = 0;
#ifdef OAI_DRV_DEBUG_RECEIVE
  int                  i;
  unsigned char       *addr_p          = 0;
#endif
  unsigned char        protocol;
  struct iphdr        *network_header_p  = NULL;

#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[UE_IP_DRV][%s] begin RB %d Inst %d Length %d bytes\n",__FUNCTION__, rb_idP,instP,data_lenP);
#endif

  skb_p = dev_alloc_skb( data_lenP + 2 );

  if(!skb_p) {
    printk("[UE_IP_DRV][%s] low on memory\n",__FUNCTION__);
    ++gpriv_p->stats.rx_dropped;
    return;
  }

  skb_reserve(skb_p,2);
  memcpy(skb_put(skb_p, data_lenP), pdcp_sdu_pP,data_lenP);

  skb_p->dev = ue_ip_dev[instP];
  hard_header_len = ue_ip_dev[instP]->hard_header_len;

  skb_set_mac_header(skb_p, 0);
  skb_set_network_header(skb_p, hard_header_len);
skb_p->mark = rb_idP;
  //printk("[NAC_COMMIN_RECEIVE]: Packet Type %d (%d,%d)",skb_p->pkt_type,PACKET_HOST,PACKET_BROADCAST);
  skb_p->pkt_type = PACKET_HOST;


#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[UE_IP_DRV][%s] Receiving packet of size %d from PDCP \n",__FUNCTION__, skb_p->len);

  for (i=0; i<skb_p->len; i++) {
    printk("%2x ",((unsigned char *)(skb_p->data))[i]);
  }

  printk("\n");
#endif
#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[UE_IP_DRV][%s] skb_p->data           @ %p\n",__FUNCTION__,  skb_p->data);
  printk("[UE_IP_DRV][%s] skb_p->mac_header     @ %p\n",__FUNCTION__,  skb_p->mac_header);
#endif



  // LG TEST skb_p->ip_summed = CHECKSUM_NONE;
  skb_p->ip_summed = CHECKSUM_UNNECESSARY;
  ipv_p = (ipversion_t *)((void *)&(skb_p->data[hard_header_len]));

  switch (ipv_p->version) {

  case 6:
#ifdef OAI_DRV_DEBUG_RECEIVE
    printk("[UE_IP_DRV][%s] receive IPv6 message\n",__FUNCTION__);
#endif
    skb_set_network_header(skb_p, hard_header_len);
    //skb_p->network_header_p = &skb_p->data[hard_header_len];

    if (hard_header_len == 0) {
      skb_p->protocol = htons(ETH_P_IPV6);
    } else {
#ifdef OAI_NW_DRIVER_TYPE_ETHERNET
      skb_p->protocol = eth_type_trans(skb_p, ue_ip_dev[instP]);
#else
#endif
    }

    //printk("Writing packet with protocol %x\n",ntohs(skb_p->protocol));
    break;

  case 4:

#ifdef OAI_DRV_DEBUG_RECEIVE
    //printk("NAS_TOOL_RECEIVE: receive IPv4 message\n");
    addr_p = (unsigned char *)&((struct iphdr *)&skb_p->data[hard_header_len])->saddr;

    if (addr_p) {
      printk("[UE_IP_DRV][%s] Source %d.%d.%d.%d\n",__FUNCTION__, addr_p[0],addr_p[1],addr_p[2],addr_p[3]);
    }

    addr_p = (unsigned char *)&((struct iphdr *)&skb_p->data[hard_header_len])->daddr;

    if (addr_p) {
      printk("[UE_IP_DRV][%s] Dest %d.%d.%d.%d\n",__FUNCTION__, addr_p[0],addr_p[1],addr_p[2],addr_p[3]);
    }

    printk("[UE_IP_DRV][%s] protocol  %d\n",__FUNCTION__, ((struct iphdr *)&skb_p->data[hard_header_len])->protocol);
#endif

    skb_set_network_header(skb_p, hard_header_len);
    //network_header_p = (struct iphdr *)skb_network_header(skb_p);
    network_header_p = (struct iphdr *)skb_network_header(skb_p);
    protocol = network_header_p->protocol;

#ifdef OAI_DRV_DEBUG_RECEIVE

    switch (protocol) {
    case IPPROTO_IP:
      printk("[UE_IP_DRV][%s] Received Raw IPv4 packet\n",__FUNCTION__);
      break;

    case IPPROTO_IPV6:
      printk("[UE_IP_DRV][%s] Received Raw IPv6 packet\n",__FUNCTION__);
      break;

    case IPPROTO_ICMP:
      printk("[UE_IP_DRV][%s] Received Raw ICMP packet\n",__FUNCTION__);
      break;

    case IPPROTO_TCP:
      printk("[UE_IP_DRV][%s] Received TCP packet\n",__FUNCTION__);
      break;

    case IPPROTO_UDP:
      printk("[UE_IP_DRV][%s] Received UDP packet\n",__FUNCTION__);
      break;

    default:
      break;
    }

#endif

    if (hard_header_len == 0) {
      skb_p->protocol = htons(ETH_P_IP);
    }

    //printk("[UE_IP_DRV][COMMON] Writing packet with protocol %x\n",ntohs(skb_p->protocol));
    break;

  default:
    printk("[UE_IP_DRV][%s] begin RB %d Inst %d Length %d bytes\n",__FUNCTION__,rb_idP,instP,data_lenP);
    printk("[UE_IP_DRV][%s] Inst %d: receive unknown message (version=%d)\n",__FUNCTION__,instP,ipv_p->version);
  }

  ++gpriv_p->stats.rx_packets;
  gpriv_p->stats.rx_bytes += data_lenP;
#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[UE_IP_DRV][%s] sending packet of size %d to kernel\n",__FUNCTION__,skb_p->len);

  for (i=0; i<skb_p->len; i++) {
    printk("%2x ",((unsigned char *)(skb_p->data))[i]);
  }

  printk("\n");
#endif //OAI_DRV_DEBUG_RECEIVE
  netif_rx(skb_p);
#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[UE_IP_DRV][%s] end\n",__FUNCTION__);
#endif
}

//---------------------------------------------------------------------------
// Delete the data
void ue_ip_common_ip2wireless_drop(struct sk_buff *skb_pP,  int instP)
{
  //---------------------------------------------------------------------------
  ue_ip_priv_t *priv_p=netdev_priv(ue_ip_dev[instP]);
  ++priv_p->stats.tx_dropped;
}

//---------------------------------------------------------------------------
// Request the transfer of data (QoS SAP)
void
ue_ip_common_ip2wireless(
  struct sk_buff *skb_pP,
  int instP
)
{
  //---------------------------------------------------------------------------
  struct pdcp_data_req_header_s     pdcph;
  ue_ip_priv_t                     *priv_p=netdev_priv(ue_ip_dev[instP]);
#ifdef LOOPBACK_TEST
  int i;
#endif
#ifdef OAI_DRV_DEBUG_SEND
  int j;
#endif
  unsigned int bytes_wrote;
  // Start debug information
#ifdef OAI_DRV_DEBUG_SEND
  printk("[UE_IP_DRV][%s] inst %d begin \n",__FUNCTION__,instP);
#endif

  if (skb_pP==NULL) {
#ifdef OAI_DRV_DEBUG_SEND
    printk("[UE_IP_DRV][%s] input parameter skb is NULL \n",__FUNCTION__);
#endif
    return;
  }


  pdcph.data_size  = skb_pP->len;

  if (skb_pP->mark) {
    pdcph.rb_id      = skb_pP->mark;
  } else {
    pdcph.rb_id      = UE_IP_DEFAULT_RAB_ID;
  }

  pdcph.inst       = instP;


  bytes_wrote = ue_ip_netlink_send((char *)&pdcph,UE_IP_PDCPH_SIZE);
#ifdef OAI_DRV_DEBUG_SEND
  printk("[UE_IP_DRV][%s] Wrote %d bytes (header for %d byte skb) to PDCP via netlink\n",__FUNCTION__,
         bytes_wrote,skb_pP->len);
#endif

  if (bytes_wrote != UE_IP_PDCPH_SIZE) {
    printk("[UE_IP_DRV][%s] problem while writing PDCP's header (bytes wrote = %d)\n",__FUNCTION__,bytes_wrote);
    printk("rb_id %d, Wrote %d, Header Size %d \n", pdcph.rb_id , bytes_wrote, UE_IP_PDCPH_SIZE);
    priv_p->stats.tx_dropped ++;
    return;
  }

  bytes_wrote += ue_ip_netlink_send((char *)skb_pP->data,skb_pP->len);


  if (bytes_wrote != skb_pP->len+UE_IP_PDCPH_SIZE) {
    printk("[UE_IP_DRV][%s] Inst %d, RB_ID %d: problem while writing PDCP's data, bytes_wrote = %d, Data_len %d, PDCPH_SIZE %d\n",
           __FUNCTION__,
           instP,
           pdcph.rb_id,
           bytes_wrote,
           skb_pP->len,
           UE_IP_PDCPH_SIZE); // congestion

    priv_p->stats.tx_dropped ++;
    return;
  }

#ifdef OAI_DRV_DEBUG_SEND
  printk("[UE_IP_DRV][%s] Sending packet of size %d to PDCP \n",__FUNCTION__,skb_pP->len);

  for (j=0; j<skb_pP->len; j++) {
    printk("%2x ",((unsigned char *)(skb_pP->data))[j]);
  }

  printk("\n");
#endif

  priv_p->stats.tx_bytes   += skb_pP->len;
  priv_p->stats.tx_packets ++;
#ifdef OAI_DRV_DEBUG_SEND
  printk("[UE_IP_DRV][%s] end \n",__FUNCTION__);
#endif
}

//---------------------------------------------------------------------------
void ue_ip_common_wireless2ip(struct nlmsghdr *nlh_pP)
{
  //---------------------------------------------------------------------------

  struct pdcp_data_ind_header_s     *pdcph_p = (struct pdcp_data_ind_header_s *)NLMSG_DATA(nlh_pP);
  ue_ip_priv_t                      *priv_p;

  priv_p = netdev_priv(ue_ip_dev[pdcph_p->inst]);


#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[UE_IP_DRV][%s] QOS receive from PDCP, size %d, rab %d, inst %d\n",__FUNCTION__,
         pdcph_p->data_size,pdcph_p->rb_id,pdcph_p->inst);
#endif

  ue_ip_common_class_wireless2ip(pdcph_p->data_size,
                                 (unsigned char *)NLMSG_DATA(nlh_pP) + UE_IP_PDCPH_SIZE,
                                 pdcph_p->inst,
                                 pdcph_p->rb_id);

}

