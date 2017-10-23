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

/*! \file nasmt_common.c
* \brief Common functions for OpenAirInterface CELLULAR version - MT
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#include "nasmt_variables.h"
#include "nasmt_proto.h"

#include <linux/inetdevice.h>
#ifdef NAS_DRIVER_TYPE_ETHERNET
#include <linux/etherdevice.h>
#endif

//---------------------------------------------------------------------------
// Receive data from FIFO (QOS or DC)
//void nasmt_COMMON_receive(uint16_t hlen, uint16_t dlen, int sap){
//void nasmt_COMMON_receive(uint16_t hlen, uint16_t dlen, void *pdcp_sdu, int sap){
void nasmt_COMMON_receive(uint16_t bytes_read, uint16_t payload_length, void *data_buffer, int rb_id, int sap)
{
  //---------------------------------------------------------------------------
  struct sk_buff *skb;
  struct ipversion *ipv;
  unsigned int hard_header_len;
  uint16_t  *p_ether_type;
  uint16_t  ether_type;

#ifdef NAS_DEBUG_RECEIVE
  printk("nasmt_COMMON_receive: begin\n");
#endif
#ifdef PDCP_USE_NETLINK

  // data_buffer is NULL if FIFOs
  if (!data_buffer) {
    printk("nasmt_COMMON_receive - input parameter data_buffer is NULL \n");
    return;
  }

#endif

  skb = dev_alloc_skb( payload_length + 2 );

  if(!skb) {
    printk("nasmt_COMMON_receive: low on memory\n");
    ++gpriv->stats.rx_dropped;
    return;
  }

  skb_reserve(skb,2);

#ifndef PDCP_USE_NETLINK
  bytes_read += rtf_get(sap, skb_put(skb, payload_length), payload_length);

  if (bytes_read != NAS_PDCPH_SIZE + payload_length) {
    printk("nasmt_COMMON_receive: problem while reading DC sap\n");
    kfree(skb->data);
    dev_kfree_skb(skb);
    return;
  }

#else
  memcpy(skb_put(skb, payload_length), data_buffer, payload_length);
  bytes_read += payload_length;
#endif

#ifdef NAS_DEBUG_RECEIVE
  printk("nasmt_COMMON_receive: received packet from PDCP, length %d\n", bytes_read);
#endif

  skb->dev = gdev;
  hard_header_len = gdev->hard_header_len;
  skb->mac_header = skb->data;
  skb->pkt_type = PACKET_HOST;
  skb->ip_summed = CHECKSUM_UNNECESSARY;

  ipv = (struct ipversion *)&(skb->data[hard_header_len]);

  switch (ipv->version) {
  case 6:
#ifdef NAS_DEBUG_RECEIVE_BASIC
    printk("nasmt_COMMON_receive: receive IPv6 message\n");
#endif
    skb->network_header = &skb->data[hard_header_len];
    // set  protocol default value
    skb->protocol = htons(ETH_P_IPV6);
    // If type Ethernet, correct it
#ifdef NAS_DRIVER_TYPE_ETHERNET
    skb->protocol = eth_type_trans(skb, gdev);
#endif
    break;

  case 4:
#ifdef NAS_DEBUG_RECEIVE_BASIC
    printk("nasmt_COMMON_receive: receive IPv4 message\n");
#endif

#ifdef NAS_DEBUG_RECEIVE
    addr = (unsigned char *)&((struct iphdr *)&skb->data[hard_header_len])->saddr;

    if (addr) {
      printk("nasmt_COMMON_receive: Source %d.%d.%d.%d\n",addr[0],addr[1],addr[2],addr[3]);
    }

    addr = (unsigned char *)&((struct iphdr *)&skb->data[hard_header_len])->daddr;

    if (addr) {
      printk("[NAS][COMMON][RECEIVE] Dest %d.%d.%d.%d\n",addr[0],addr[1],addr[2],addr[3]);
    }

    printk("[NAS][COMMON][RECEIVE] protocol  %d\n",((struct iphdr *)&skb->data[hard_header_len])->protocol);
#endif

    skb->network_header = &skb->data[hard_header_len];
    // set  protocol default value
    skb->protocol = htons(ETH_P_IP);
    // If type Ethernet, correct it
#ifdef NAS_DRIVER_TYPE_ETHERNET
    skb->protocol = eth_type_trans(skb, gdev);
#endif
    break;

  default:
    printk("nasmt_COMMON_receive: Packet is not IPv4 or IPv6 (version=%d)\n", ipv->version);

#ifdef NAS_DRIVER_TYPE_ETHERNET
#ifdef NAS_DEBUG_RECEIVE
    printk("nasmt_COMMON_receive: ether_type=%04X\n", ether_type);
#endif
    skb->protocol = eth_type_trans(skb, gdev);
    // minus 1(short) instead of 2(bytes) because uint16_t*
    p_ether_type = (uint16_t *)&(skb->mac_header[hard_header_len-2]);
    ether_type = ntohs(*p_ether_type);
#ifdef NAS_DEBUG_RECEIVE
    printk("nasmt_COMMON_receive: ether_type=%04X\n", ether_type);
#endif

    switch (ether_type) {
    case ETH_P_ARP:
      printk("[NAS][COMMON] ether_type = ETH_P_ARP\n");
      skb->protocol = htons(ETH_P_ARP);
      skb->network_header = &skb->mac_header[hard_header_len];
      break;

    default:
      break;
    }

#endif
  }

  ++gpriv->stats.rx_packets;
  gpriv->stats.rx_bytes += bytes_read;
#ifdef NAS_DEBUG_RECEIVE
  printk("nasmt_COMMON_receive: forwarding packet of size %d to kernel\n",skb->len);
#endif

  netif_rx(skb);
#ifdef NAS_DEBUG_RECEIVE
  printk("nasmt_COMMON_receive: end\n");
#endif
}

//---------------------------------------------------------------------------
// Delete the data
void nasmt_COMMON_del_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *sp)
{
  //---------------------------------------------------------------------------
#ifdef NAS_DEBUG_SEND
  printk("nasmt_COMMON_del_send - updating statistics \n");
#endif
  ++gpriv->stats.tx_dropped;
}

//---------------------------------------------------------------------------
// Request the transfer of data (QoS SAP)
void nasmt_COMMON_QOS_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc)
{
  //---------------------------------------------------------------------------
  //struct pdcp_data_req     pdcph;
  struct pdcp_data_req_header_t     pdcph;
  int bytes_wrote = 0;

  // Start debug information
#ifdef NAS_DEBUG_SEND
  printk("nasmt_COMMON_QOS_send - begin \n");
#endif

  //  if (cx->state!=NAS_STATE_CONNECTED) // <--- A REVOIR
  //  {
  //    gpriv->stats.tx_dropped ++;
  //    printk("NAS_QOS_SEND: No connected, so message are dropped \n");
  //    return;
  //  }
  if (!skb || !gc || !cx) {
    printk("nasmt_COMMON_QOS_send - input parameter skb|gc|cx is NULL \n");
    return;
  }

  // End debug information

  if (gc->rb==NULL) {
    gc->rb = nasmt_COMMON_search_rb(cx, gc->rab_id);

    if (gc->rb==NULL) {
      ++gpriv->stats.tx_dropped;
      printk("nasmt_COMMON_QOS_send: No corresponding Radio Bearer, so message is dropped, rab_id=%u \n", gc->rab_id);
      return;
    }
  }

#ifdef NAS_DEBUG_SEND
  printk("nasmt_COMMON_QOS_send #1 :");
  printk("lcr %u, rab_id %u, rab_id %u\n", cx->lcr, (gc->rb)->rab_id, gc->rab_id);
  nasmt_TOOL_print_classifier(gc);
#endif

  pdcph.data_size  = skb->len;
  pdcph.rb_id      = ((gc->rb)->rab_id+(32*cx->lcr))-NAS_SIG_NUM_SRB;
  pdcph.inst       = 0;

#ifdef PDCP_USE_NETLINK
  bytes_wrote = nasmt_netlink_send((unsigned char *)&pdcph,NAS_PDCPH_SIZE, NASNL_DEST_PDCP);
  //printk("nasmt_COMMON_QOS_send - Wrote %d bytes (header for %d byte skb) to PDCP via netlink\n", bytes_wrote,skb->len);
#else
  //bytes_wrote = rtf_put(gpriv->sap[(gc->rb)->sapi], &pdcph, NAS_PDCPH_SIZE);
  bytes_wrote = rtf_put(NAS2PDCP_FIFO, &pdcph, NAS_PDCPH_SIZE);
  //printk("nasmt_COMMON_QOS_send - Wrote %d bytes (header for %d byte skb) to PDCP fifo\n", bytes_wrote,skb->len);
#endif //PDCP_USE_NETLINK

  if (bytes_wrote != NAS_PDCPH_SIZE) {
    printk("nasmt_COMMON_QOS_send: problem while writing PDCP's header\n");
    printk("rb_id %d, SAP index %d, Wrote %d to fifo %d, Header Size %d \n", pdcph.rb_id , (gc->rb)->sapi, bytes_wrote, NAS2PDCP_FIFO, NAS_PDCPH_SIZE);
    gpriv->stats.tx_dropped ++;
    return;
  }

#ifdef  PDCP_USE_NETLINK
  bytes_wrote += nasmt_netlink_send((unsigned char *)skb->data,skb->len, NASNL_DEST_PDCP);
#else
  //bytes_wrote += rtf_put(gpriv->sap[(gc->rb)->sapi], skb->data, skb->len);
  bytes_wrote += rtf_put(NAS2PDCP_FIFO, skb->data, skb->len);
#endif //PDCP_USE_NETLINK

  if (bytes_wrote != skb->len + NAS_PDCPH_SIZE) {
    printk("nasmt_COMMON_QOS_send: problem while writing PDCP's data\n"); // congestion
    printk("rb_id %d, SAP index %d, Wrote %d to fifo %d, Header Size %d \n", pdcph.rb_id , (gc->rb)->sapi, bytes_wrote, NAS2PDCP_FIFO, NAS_PDCPH_SIZE);
    gpriv->stats.tx_dropped ++;
    return;
  }

#ifdef NAS_DEBUG_SEND
  printk("nasmt_COMMON_QOS_send - %d bytes wrote to rb_id %d, sap %d \n", bytes_wrote, pdcph.rb_id,NAS2PDCP_FIFO);
#endif
  gpriv->stats.tx_bytes   += skb->len;
  gpriv->stats.tx_packets ++;
#ifdef NAS_DEBUG_SEND
  printk("nasmt_COMMON_QOS_send - end \n");
#endif
}

#ifndef PDCP_USE_NETLINK
//---------------------------------------------------------------------------
void nasmt_COMMON_QOS_receive(struct cx_entity *cx)
{
  //---------------------------------------------------------------------------
  uint8_t sapi;
  //struct pdcp_data_ind     pdcph;
  struct pdcp_data_ind_header_t  pdcph;
  int bytes_read = 0;

  // Start debug information
#ifdef NAS_DEBUG_RECEIVE
  printk("nasmt_COMMON_QOS_receive - begin \n");
#endif

  if (!cx) {
    printk("nasmt_COMMON_QOS_receive - input parameter cx is NULL \n");
    return;
  }

  // End debug information

  // LG force the use of only 1 rt fifo
  sapi = NAS_DRB_OUTPUT_SAPI;

  bytes_read =  rtf_get(gpriv->sap[sapi], &pdcph, NAS_PDCPH_SIZE);

  while (bytes_read>0) {
    if (bytes_read != NAS_PDCPH_SIZE) {
      printk("nasmt_COMMON_QOS_receive: problem while reading PDCP header\n");
      return;
    }

    //void nasmt_COMMON_receive(uint16_t bytes_read, uint16_t payload_length, void *data_buffer, int rb_id, int sap);
    // data_buffer is NULL because FIFO should be read directly in the skbuff (LITE has an intermediary buffer)
    nasmt_COMMON_receive(NAS_PDCPH_SIZE, pdcph.data_size, NULL, pdcph->rb_id, gpriv->sap[sapi]);
    // check if another frame is in the FIFO, otherwise return
    bytes_read =  rtf_get(gpriv->sap[sapi], &pdcph, NAS_PDCPH_SIZE);
  }

#ifdef NAS_DEBUG_RECEIVE
  printk("nasmt_COMMON_QOS_receive - end \n");
#endif
}

#else
//---------------------------------------------------------------------------
void nasmt_COMMON_QOS_receive(struct nlmsghdr *nlh)
{
  //---------------------------------------------------------------------------

  struct pdcp_data_ind_header_t  *pdcph;

  // Start debug information
#ifdef NAS_DEBUG_RECEIVE
  printk("nasmt_COMMON_QOS_receive - begin \n");
#endif

  if (!nlh) {
    printk("nasmt_COMMON_QOS_receive - input parameter nlh is NULL \n");
    return;
  }

  // End debug information
  pdcph = (struct pdcp_data_ind_header_t *)NLMSG_DATA(nlh);

#ifdef NAS_DEBUG_RECEIVE
  printk("nasmt_COMMON_QOS_receive - receive from PDCP, size %d, rab %d\\n", pdcph->data_size, pdcph->rb_id);
#endif //NAS_DEBUG_RECEIVE

  //void nasmt_COMMON_receive(uint16_t bytes_read, uint16_t payload_length, void *data_buffer, int rb_id, int sap);
  nasmt_COMMON_receive(NAS_PDCPH_SIZE + pdcph->data_size, pdcph->data_size, (unsigned char *)NLMSG_DATA(nlh) + NAS_PDCPH_SIZE, pdcph->rb_id, 0);

}
#endif //PDCP_USE_NETLINK


//---------------------------------------------------------------------------
struct cx_entity *nasmt_COMMON_search_cx(nasLocalConnectionRef_t lcr)
{
  //---------------------------------------------------------------------------
#ifdef NAS_DEBUG_CLASS
  printk("nasmt_COMMON_search_cx - lcr %d\n",lcr);
#endif

  if (lcr<NAS_CX_MAX)
    return gpriv->cx+lcr;
  else
    return NULL;
}

//---------------------------------------------------------------------------
// Search a Radio Bearer
struct rb_entity *nasmt_COMMON_search_rb(struct cx_entity *cx, nasRadioBearerId_t rab_id)
{
  //---------------------------------------------------------------------------
  struct rb_entity *rb;
#ifdef NAS_DEBUG_CLASS
  printk("nasmt_COMMON_search_rb - rab_id %d\n", rab_id);
#endif

  if (!cx) {
    printk("nasmt_COMMON_search_rb - input parameter cx is NULL \n");
    return NULL;
  }

  for (rb=cx->rb; rb!=NULL; rb=rb->next) {
    if (rb->rab_id==rab_id)
      return rb;
  }

  return NULL;
}

//---------------------------------------------------------------------------
struct rb_entity *nasmt_COMMON_add_rb(struct cx_entity *cx, nasRadioBearerId_t rab_id, nasQoSTrafficClass_t qos)
{
  //---------------------------------------------------------------------------
  struct rb_entity *rb;
#ifdef NAS_DEBUG_CLASS
  printk("nasmt_COMMON_add_rb - begin for rab_id %d , qos %d\n", rab_id, qos );
#endif

  if (cx==NULL) {
    printk("nasmt_COMMON_add_rb - input parameter cx is NULL \n");
    return NULL;
  }

  rb=nasmt_COMMON_search_rb(cx, rab_id);

  if (rb==NULL) {
    rb=(struct rb_entity *)kmalloc(sizeof(struct rb_entity), GFP_KERNEL);

    if (rb!=NULL) {
      rb->retry=0;
      rb->countimer=NAS_TIMER_IDLE;
      rb->rab_id=rab_id;
      //      rb->rab_id=rab_id+(32*cx->lcr);
#ifdef NAS_DEBUG_DC
      printk("nasmt_COMMON_add_rb: rb rab_id=%u, rab_id=%u, mt_id=%u\n",rb->rab_id,rab_id, cx->lcr);
#endif
      rb->qos=qos;
      rb->sapi=NAS_DRB_INPUT_SAPI;
      // LG force the use of only one rt-fifo rb->sapi=NAS_BA_INPUT_SAPI;
      rb->state=NAS_IDLE;
      rb->next=cx->rb;
      cx->rb=rb;
      ++cx->num_rb;
    } else
      printk("nasmt_COMMON_add_rb: no memory\n");
  }

#ifdef NAS_DEBUG_CLASS
  printk("nasmt_COMMON_add_rb - end \n" );
#endif
  return rb;
}

//---------------------------------------------------------------------------
// free the memory that has previously been allocated to rb and remove from linked list
void nasmt_COMMON_del_rb(struct cx_entity *cx, nasRadioBearerId_t rab_id, nasIPdscp_t dscp)
{
  //---------------------------------------------------------------------------
  struct rb_entity *rb, *curr_rb, *prev_rb;
  struct classifier_entity *p;
  uint16_t classref=0;

  // Start debug information
#ifdef NAS_DEBUG_CLASS
  printk("nasmt_COMMON_del_rb - begin\n");
#endif

  if (cx==NULL) {
    printk("nasmt_COMMON_del_rb - input parameter cx is NULL \n");
    return;
  }

  // End debug information

  // Clear the associated classifier
  for (p=cx->sclassifier[dscp]; p!=NULL; p=p->next) {
    if (p->classref>=classref) {
      classref=p->classref;
#ifdef NAS_DEBUG_CLASS
      printk("nasmt_COMMON_del_rb: classifier found for dscp %u \n", dscp);
#endif
    }
  }

  nasmt_CLASS_del_sclassifier(cx, dscp, classref);

  // Now, delete the RB
  curr_rb = NULL;
  prev_rb = NULL;

  for (rb=cx->rb; rb!=NULL; rb=rb->next) {
    if (rb->rab_id == rab_id) {
      curr_rb = rb;

      if (prev_rb!=NULL) {
        prev_rb->next = rb->next;
      } else {
        cx->rb=rb->next;
      }

      break;
    } else {
      prev_rb = rb;
    }
  }

  if (curr_rb!= NULL) {
    printk("nasmt_COMMON_del_rb: del rab_id %u\n", rb->rab_id);
    kfree(rb);
    (cx->num_rb)--;
  } else {
    printk("\n\n--nasmt_COMMON_del_rb: ERROR, invalid rab_id %u\n", rb->rab_id);
  }

#ifdef NAS_DEBUG_CLASS
  printk("nasmt_COMMON_del_rb - end\n");
#endif
}

//---------------------------------------------------------------------------
void nasmt_COMMON_flush_rb(struct cx_entity *cx)
{
  //---------------------------------------------------------------------------
  struct rb_entity *rb;
  struct classifier_entity *gc;
  uint8_t dscp;
  // Start debug information
#ifdef NAS_DEBUG_CLASS
  printk("nasmt_COMMON_flush_rb - begin\n");
#endif

  if (cx==NULL) {
    printk("nasmt_COMMON_flush_rb - input parameter cx is NULL \n");
    return;
  }

  // End debug information
  for (rb=cx->rb; rb!=NULL; rb=cx->rb) {
    printk("nasmt_COMMON_flush_rb: del rab_id %u\n", rb->rab_id);
    cx->rb=rb->next;
    kfree(rb);
  }

  cx->num_rb=0;
  cx->rb=NULL;

  for(dscp=0; dscp<NAS_DSCP_MAX; ++dscp) {
    for (gc=cx->sclassifier[dscp]; gc!=NULL; gc=gc->next)
      gc->rb=NULL;
  }

#ifdef NAS_DEBUG_CLASS
  printk("nasmt_COMMON_flush_rb - end\n");
#endif
}
