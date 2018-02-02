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
* \brief implementation of emultor tx and rx
* \author Navid Nikaein, Lionel GAUTHIER, and Raymomd Knopp
* \date 2011
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
#ifdef OAI_NW_DRIVER_TYPE_ETHERNET
#include <linux/etherdevice.h>
#endif

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
void oai_nw_drv_common_class_wireless2ip(uint16_t dlen,
    void *pdcp_sdu,
    int inst,
    OaiNwDrvRadioBearerId_t rb_id)
{

  //---------------------------------------------------------------------------
  struct sk_buff      *skb;
  struct ipversion    *ipv;
  struct oai_nw_drv_priv     *gpriv=netdev_priv(oai_nw_drv_dev[inst]);
  unsigned int         hard_header_len = 0;
  uint16_t                 *p_ether_type;
  uint16_t                  ether_type;
#ifdef OAI_DRV_DEBUG_RECEIVE
  int i;
  unsigned char *addr;
#endif
  unsigned char        protocol;
  struct iphdr        *network_header;

#ifdef NAS_ADDRESS_FIX
  uint32_t odaddr,osaddr;
  unsigned char *addr,*daddr,*saddr,*ifaddr;
  uint16_t *cksum,check;
#endif // NAS_ADDRESS_FIX
#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[OAI_IP_DRV][%s] begin RB %d Inst %d Length %d bytes\n",__FUNCTION__, rb_id,inst,dlen);
#endif

  skb = dev_alloc_skb( dlen + 2 );

  if(!skb) {
    printk("[OAI_IP_DRV][%s] low on memory\n",__FUNCTION__);
    ++gpriv->stats.rx_dropped;
    return;
  }

  skb_reserve(skb,2);
  memcpy(skb_put(skb, dlen), pdcp_sdu,dlen);

  skb->dev = oai_nw_drv_dev[inst];
  hard_header_len = oai_nw_drv_dev[inst]->hard_header_len;

  skb_set_mac_header(skb, 0);
  skb_set_network_header(skb, hard_header_len);
  skb->mark = rb_id;
  skb->pkt_type = PACKET_HOST;


#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[OAI_IP_DRV][%s] Receiving packet @%p of size %d from PDCP \n",__FUNCTION__, skb->data, skb->len);

  for (i=0; i<skb->len; i++)
    printk("%2x ",((unsigned char *)(skb->data))[i]);

  printk("\n");
#endif



  // LG TEST skb->ip_summed = CHECKSUM_NONE;
  skb->ip_summed = CHECKSUM_UNNECESSARY;


  ipv = (struct ipversion *)skb_network_header(skb);

  switch (ipv->version) {

  case 6:
#ifdef OAI_DRV_DEBUG_RECEIVE
    printk("[OAI_IP_DRV][%s] receive IPv6 message\n",__FUNCTION__);
#endif
    skb_set_network_header(skb, hard_header_len);

    //skb->network_header = &skb->data[hard_header_len];
    if (hard_header_len == 0) {
      skb->protocol = htons(ETH_P_IPV6);
    } else {
#ifdef OAI_NW_DRIVER_TYPE_ETHERNET
      skb->protocol = eth_type_trans(skb, oai_nw_drv_dev[inst]);
#else
#endif
    }

    //printk("Writing packet with protocol %x\n",ntohs(skb->protocol));
    break;

  case 4:
#ifdef NAS_ADDRESS_FIX
    // Make the third byte of both the source and destination equal to the fourth of the destination
    daddr = (unsigned char *)&((struct iphdr *)&skb->data[hard_header_len])->daddr;
    odaddr = ((struct iphdr *)skb->data)->daddr;
    //sn = addr[3];
    saddr = (unsigned char *)&((struct iphdr *)&skb->data[hard_header_len])->saddr;
    osaddr = ((struct iphdr *)&skb->data[hard_header_len])->saddr;

    if (daddr[0] == saddr[0]) {// same network
      daddr[2] = daddr[3]; // set third byte of destination to that of local machine so that local IP stack accepts the packet
      saddr[2] = daddr[3]; // set third byte of source to that of local machine so that local IP stack accepts the packet
    }  else { // get the 3rd byte from device address in net_device structure
      ifaddr = (unsigned char *)(&(((struct in_device *)((oai_nw_drv_dev[inst])->ip_ptr))->ifa_list->ifa_local));

      if (saddr[0] == ifaddr[0]) { // source is in same network as local machine
        daddr[0] += saddr[3];        // fix address of remote destination to undo change at source
        saddr[2] =  ifaddr[2];       // set third byte to that of local machine so that local IP stack accepts the packet
      } else {                         // source is remote machine from outside network
        saddr[0] -= daddr[3];        // fix address of remote source to be understood by destination
        daddr[2] =  daddr[3];        // fix 3rd byte of local address to be understood by IP stack of
        // destination
      }
    }

#endif //NAS_ADDRESS_FIX
#ifdef OAI_DRV_DEBUG_RECEIVE
    //printk("NAS_TOOL_RECEIVE: receive IPv4 message\n");
    addr = (unsigned char *)&((struct iphdr *)&skb->data[hard_header_len])->saddr;

    if (addr) {
      //addr[2]^=0x01;
      printk("[OAI_IP_DRV][%s] Source %d.%d.%d.%d\n",__FUNCTION__, addr[0],addr[1],addr[2],addr[3]);
    }

    addr = (unsigned char *)&((struct iphdr *)&skb->data[hard_header_len])->daddr;

    if (addr) {
      //addr[2]^=0x01;
      printk("[OAI_IP_DRV][%s] Dest %d.%d.%d.%d\n",__FUNCTION__, addr[0],addr[1],addr[2],addr[3]);
    }

    printk("[OAI_IP_DRV][%s] protocol  %d\n",__FUNCTION__, ((struct iphdr *)&skb->data[hard_header_len])->protocol);
#endif

    skb_set_network_header(skb, hard_header_len);
    //skb->network_header = &skb->data[hard_header_len];
    network_header = (struct iphdr *)skb_network_header(skb);
    protocol = network_header->protocol;

#ifdef OAI_DRV_DEBUG_RECEIVE

    switch (protocol) {
    case IPPROTO_IP:
      printk("[OAI_IP_DRV][%s] Received Raw IPv4 packet\n",__FUNCTION__);
      break;

    case IPPROTO_IPV6:
      printk("[OAI_IP_DRV][%s] Received Raw IPv6 packet\n",__FUNCTION__);
      break;

    case IPPROTO_ICMP:
      printk("[OAI_IP_DRV][%s] Received Raw ICMP packet\n",__FUNCTION__);
      break;

    case IPPROTO_TCP:
      printk("[OAI_IP_DRV][%s] Received TCP packet\n",__FUNCTION__);
      break;

    case IPPROTO_UDP:
      printk("[OAI_IP_DRV][%s] Received UDP packet\n",__FUNCTION__);
      break;

    default:
      break;
    }

#endif

#ifdef NAS_ADDRESS_FIX
    network_header->check = 0;
    network_header->check = ip_fast_csum((unsigned char *) network_header, network_header->ihl);
    //printk("[OAI_IP_DRV][COMMON][RECEIVE] IP Fast Checksum %x \n", network_header->check);

    switch(protocol) {
    case IPPROTO_TCP:

      cksum  = (uint16_t*)&(((struct tcphdr*)((network_header + (network_header->ihl<<2))))->check);
      check  = csum_tcpudp_magic(((struct iphdr *)network_header)->saddr, ((struct iphdr *)network_header)->daddr, 0,0, ~(*cksum));
      //check  = csum_tcpudp_magic(((struct iphdr *)network_header)->saddr, ((struct iphdr *)network_header)->daddr, tcp_hdrlen(skb), IPPROTO_TCP, ~(*cksum));
      //check  = csum_tcpudp_magic(((struct iphdr *)network_header)->saddr, ((struct iphdr *)network_header)->daddr, dlen, IPPROTO_TCP, ~(*cksum));

      *cksum = csum_tcpudp_magic(~osaddr, ~odaddr, 0, 0, ~check);
      //*cksum = csum_tcpudp_magic(~osaddr, ~odaddr, dlen, IPPROTO_TCP, ~check);
#ifdef OAI_DRV_DEBUG_RECEIVE
      printk("[OAI_IP_DRV][%s] Inst %d TCP packet calculated CS %x, CS = %x (before), SA (%x)%x, DA (%x)%x\n",__FUNCTION__,
             inst,
             network_header->check,
             *cksum,
             osaddr,
             ((struct iphdr *)skb->data)->saddr,
             odaddr,
             ((struct iphdr *)skb->data)->daddr);

      printk("[OAI_IP_DRV][%s] Inst %d TCP packet NEW CS %x\n",__FUNCTION__,
             inst,
             *cksum);
#endif
      break;

    case IPPROTO_UDP:
      cksum  = (uint16_t*)&(((struct udphdr*)((network_header + (network_header->ihl<<2))))->check);
      check = csum_tcpudp_magic(((struct iphdr *)network_header)->saddr, ((struct iphdr *)network_header)->daddr, 0,0, ~(*cksum));
      // check = csum_tcpudp_magic(((struct iphdr *)network_header)->saddr, ((struct iphdr *)network_header)->daddr, udp_hdr(skb)->len, IPPROTO_UDP, ~(*cksum));
      //check = csum_tcpudp_magic(((struct iphdr *)network_header)->saddr, ((struct iphdr *)network_header)->daddr, dlen, IPPROTO_UDP, ~(*cksum));

      *cksum= csum_tcpudp_magic(~osaddr, ~odaddr,0,0, ~check);
      //*cksum= csum_tcpudp_magic(~osaddr, ~odaddr,udp_hdr(skb)->len, IPPROTO_UDP, ~check);
      //*cksum= csum_tcpudp_magic(~osaddr, ~odaddr,dlen, IPPROTO_UDP, ~check);

#ifdef OAI_DRV_DEBUG_RECEIVE
      printk("[OAI_IP_DRV][%s] Inst %d UDP packet CS = %x (before), SA (%x)%x, DA (%x)%x\n",__FUNCTION__,
             inst,*cksum,osaddr,((struct iphdr *)&skb->data[hard_header_len])->saddr,odaddr,((struct iphdr *)&skb->data[hard_header_len])->daddr);

      printk("[OAI_IP_DRV][%s] Inst %d UDP packet NEW CS %x\n",__FUNCTION__,inst,*cksum);
#endif
      //if ((check = *cksum) != 0) {
      // src, dst, len, proto, sum
      //          }
      break;

    default:
      break;
    }

#endif //NAS_ADDRESS_FIX

    if (hard_header_len == 0) {
      skb->protocol = htons(ETH_P_IP);
    } else {
#ifdef OAI_NW_DRIVER_TYPE_ETHERNET
      skb->protocol = eth_type_trans(skb, oai_nw_drv_dev[inst]);
#else
#endif
    }

    //printk("[OAI_IP_DRV][COMMON] Writing packet with protocol %x\n",ntohs(skb->protocol));
    break;

  default:
    // fill skb->pkt_type, skb->dev

    skb->protocol = eth_type_trans(skb, oai_nw_drv_dev[inst]);
    // minus 1(short) instead of 2(bytes) because uint16_t*
    p_ether_type = (uint16_t *)(skb_network_header(skb)-2);
    ether_type = ntohs(*p_ether_type);

    switch (ether_type) {
    case ETH_P_ARP:
#ifdef OAI_DRV_DEBUG_RECEIVE
      printk("[OAI_IP_DRV][%s] ether_type = ETH_P_ARP\n",__FUNCTION__);
#endif
      //skb->pkt_type = PACKET_HOST;
      skb->protocol = htons(ETH_P_ARP);
      break;

    default:
      ;
    }

    printk("[OAI_IP_DRV][%s] begin RB %d Inst %d Length %d bytes\n",__FUNCTION__,rb_id,inst,dlen);
    printk("[OAI_IP_DRV][%s] Inst %d: receive unknown message (version=%d)\n",__FUNCTION__,inst,ipv->version);
  }

  ++gpriv->stats.rx_packets;
  gpriv->stats.rx_bytes += dlen;
#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[OAI_IP_DRV][%s] sending packet of size %d to kernel\n",__FUNCTION__,skb->len);

  for (i=0; i<skb->len; i++)
    printk("%2x ",((unsigned char *)(skb->data))[i]);

  printk("\n");
#endif //OAI_DRV_DEBUG_RECEIVE
  netif_rx(skb);
#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[OAI_IP_DRV][%s] end\n",__FUNCTION__);
#endif
}

//---------------------------------------------------------------------------
// Delete the data
void oai_nw_drv_common_ip2wireless_drop(struct sk_buff *skb, int inst)
{
  //---------------------------------------------------------------------------
  struct oai_nw_drv_priv *priv=netdev_priv(oai_nw_drv_dev[inst]);
  ++priv->stats.tx_dropped;
}

//---------------------------------------------------------------------------
// Request the transfer of data (QoS SAP)
void oai_nw_drv_common_ip2wireless(struct sk_buff *skb, int inst)
{
  //---------------------------------------------------------------------------
  struct pdcp_data_req_header_s     pdcph;
  struct oai_nw_drv_priv *priv=netdev_priv(oai_nw_drv_dev[inst]);
#ifdef LOOPBACK_TEST
  int i;
#endif
#ifdef OAI_DRV_DEBUG_SEND
  int j;
#endif
  unsigned int bytes_wrote;
  // Start debug information
#ifdef OAI_DRV_DEBUG_SEND
  printk("[OAI_IP_DRV][%s] inst %d begin \n",__FUNCTION__,inst);
#endif

  if (skb==NULL) {
#ifdef OAI_DRV_DEBUG_SEND
    printk("[OAI_IP_DRV][%s] input parameter skb is NULL \n",__FUNCTION__);
#endif
    return;
  }

  pdcph.data_size    = skb->len;
  pdcph.rb_id        = skb->mark;
  pdcph.inst         = inst;
  pdcph.traffic_type = oai_nw_drv_find_traffic_type(skb);


  bytes_wrote = oai_nw_drv_netlink_send((char *)&pdcph,OAI_NW_DRV_PDCPH_SIZE);
#ifdef OAI_DRV_DEBUG_SEND
  printk("[OAI_IP_DRV][%s] Wrote %d bytes (header for %d byte skb) to PDCP RB %d via netlink\n",__FUNCTION__,
         bytes_wrote,skb->len, pdcph.rb_id);
#endif


  if (bytes_wrote != OAI_NW_DRV_PDCPH_SIZE) {
    printk("[OAI_IP_DRV][%s] problem while writing PDCP's header (bytes wrote = %d to fifo %d)\n",__FUNCTION__,bytes_wrote,IP2PDCP_FIFO);
    printk("rb_id %u, Wrote %u, Header Size %lu \n", pdcph.rb_id , bytes_wrote, OAI_NW_DRV_PDCPH_SIZE);
    priv->stats.tx_dropped ++;
    return;
  }

  bytes_wrote += oai_nw_drv_netlink_send((char *)skb->data,skb->len);


  if (bytes_wrote != skb->len+OAI_NW_DRV_PDCPH_SIZE) {
    printk("[OAI_IP_DRV][%s] Inst %d, RB_ID %u: problem while writing PDCP's data, bytes_wrote = %u, Data_len %u, PDCPH_SIZE %lu\n",
           __FUNCTION__,
           inst,
           pdcph.rb_id,
           bytes_wrote,
           skb->len,
           OAI_NW_DRV_PDCPH_SIZE); // congestion

    priv->stats.tx_dropped ++;
    return;
  }

#ifdef OAI_DRV_DEBUG_SEND
  printk("[OAI_IP_DRV][%s] Sending packet of size %d to PDCP traffic type %d\n",__FUNCTION__,skb->len, pdcph.traffic_type);

  for (j=0; j<skb->len; j++)
    printk("%2x ",((unsigned char *)(skb->data))[j]);

  printk("\n");
#endif

  priv->stats.tx_bytes   += skb->len;
  priv->stats.tx_packets ++;
#ifdef OAI_DRV_DEBUG_SEND
  printk("[OAI_IP_DRV][%s] end \n",__FUNCTION__);
#endif
}


//---------------------------------------------------------------------------
void oai_nw_drv_common_wireless2ip(struct nlmsghdr *nlh)
{
  //---------------------------------------------------------------------------

  struct pdcp_data_ind_header_s     *pdcph = (struct pdcp_data_ind_header_s *)NLMSG_DATA(nlh);
  struct oai_nw_drv_priv *priv;

  priv = netdev_priv(oai_nw_drv_dev[pdcph->inst]);

#ifdef OAI_DRV_DEBUG_RECEIVE
  printk("[OAI_IP_DRV][%s] QOS receive from PDCP, size %d, rab %d, inst %d\n",__FUNCTION__,
         pdcph->data_size,pdcph->rb_id,pdcph->inst);
#endif //OAI_DRV_DEBUG_RECEIVE

  oai_nw_drv_common_class_wireless2ip(pdcph->data_size,
                                      (unsigned char *)NLMSG_DATA(nlh) + OAI_NW_DRV_PDCPH_SIZE,
                                      pdcph->inst,
                                      pdcph->rb_id);
}


