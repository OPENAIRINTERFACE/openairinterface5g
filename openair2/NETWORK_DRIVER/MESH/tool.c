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

#include "local.h"
#include "proto_extern.h"

//#include <linux/in.h>
//#include <net/ndisc.h>
//#include <linux/icmpv6.h>
//#include <linux/icmp.h>
//#include <linux/udp.h>
//#include <linux/tcp.h>

//#define NAS_DEBUG_TOOL 1

//---------------------------------------------------------------------------
//
void nas_TOOL_fct(struct classifier_entity *gc, uint8_t fct)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_FCT - begin \n");
#endif

  if (gc==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_FCT - input parameter gc is NULL \n");
#endif
    return;
  }

  // End debug information
  switch(fct) {
  case NAS_FCT_QOS_SEND:
    gc->fct=nas_COMMON_QOS_send;
    break;

  case NAS_FCT_CTL_SEND:
    gc->fct=nas_CTL_send;
    break;

  case NAS_FCT_DC_SEND:
    gc->fct=nas_mesh_DC_send_sig_data_request;
    break;

  case NAS_FCT_DEL_SEND:
    gc->fct=nas_COMMON_del_send;
    break;

  default:
    gc->fct=nas_COMMON_del_send;
  }
}

//---------------------------------------------------------------------------
uint8_t nas_TOOL_invfct(struct classifier_entity *gc)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_INVFCT - begin \n");
#endif

  if (gc==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_INVFCT - input parameter gc is NULL \n");
#endif
    return 0;
  }

  // End debug information
  if (gc->fct==nas_COMMON_QOS_send)
    return NAS_FCT_QOS_SEND;

  if (gc->fct==nas_CTL_send)
    return NAS_FCT_CTL_SEND;

  if (gc->fct==nas_COMMON_del_send)
    return NAS_FCT_DEL_SEND;

  if (gc->fct==nas_mesh_DC_send_sig_data_request)

    return NAS_FCT_DC_SEND;

  return 0;
}

//---------------------------------------------------------------------------
uint8_t nas_TOOL_get_dscp6(struct ipv6hdr *iph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_GET_DSCP6 - begin \n");
#endif

  if (iph==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_GET_DSCP6 - input parameter iph is NULL \n");
#endif
    return 0;
  }

  // End debug information
  return (ntohl(((*(__u32 *)iph)&NAS_TRAFFICCLASS_MASK)))>>22;
  //return ntohs(*(const __be16 *)iph) >> 4; // see linux/dsfield.h

}

//---------------------------------------------------------------------------
uint8_t nas_TOOL_get_dscp4(struct iphdr *iph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_GET_DSCP4 - begin \n");
#endif

  if (iph==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_GET_DSCP4 - input parameter iph is NULL \n");
#endif
    return 0;
  }

  // End debug information
  return (iph->tos);

}

//---------------------------------------------------------------------------
int nas_TOOL_network6(struct in6_addr *addr, struct in6_addr *prefix, uint8_t plen)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_NETWORK6 - begin \n");
#endif

  if (addr==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_NETWORK6 - input parameter addr is NULL \n");
#endif
    return 0;
  }

  if (prefix==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_NETWORK6 - input parameter prefix is NULL \n");
#endif
    return 0;
  }

  // End debug information
  switch(plen/32) {
  case 0:
    return (((addr->s6_addr32[0]>>(32-plen))<<(32-plen))==prefix->s6_addr[0]);

  case 1:
    return ((addr->s6_addr32[0]==prefix->s6_addr[0])&&
            (((addr->s6_addr32[1]>>(64-plen))<<(64-plen))==prefix->s6_addr[1]));

  case 2:
    return ((addr->s6_addr32[0]==prefix->s6_addr[0])&&
            (addr->s6_addr32[1]==prefix->s6_addr[1])&&
            (((addr->s6_addr32[2]>>(96-plen))<<(96-plen))==prefix->s6_addr[2]));

  case 3:
    return ((addr->s6_addr32[0]==prefix->s6_addr[0])&&
            (addr->s6_addr32[1]==prefix->s6_addr[1])&&
            (addr->s6_addr32[2]==prefix->s6_addr[2])&&
            (((addr->s6_addr32[3]>>(128-plen))<<(128-plen))==prefix->s6_addr[3]));

  default:
    return ((addr->s6_addr32[0]==prefix->s6_addr[0])&&
            (addr->s6_addr32[1]==prefix->s6_addr[1])&&
            (addr->s6_addr32[2]==prefix->s6_addr[2])&&
            (addr->s6_addr32[3]==prefix->s6_addr[3]));
  }
}

//---------------------------------------------------------------------------
int nas_TOOL_network4(uint32_t *addr, uint32_t *prefix, uint8_t plen)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_NETWORK4 - begin \n");
#endif

  if (addr==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_NETWORK4 - input parameter addr is NULL \n");
#endif
    return 0;
  }

  if (prefix==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_NETWORK4 - input parameter prefix is NULL \n");
#endif
    return 0;
  }

  // End debug information
  if (plen>=32)
    return (*addr==*prefix);
  else
    return (((*addr>>(32-plen))<<(32-plen))==*prefix);
}

//---------------------------------------------------------------------------
//struct udphdr *nas_TOOL_get_udp6(struct ipv6hdr *iph){
//---------------------------------------------------------------------------
//  return (struct udphdr *)((char *)iph+NAS_IPV6_SIZE); // to modify
//}

//---------------------------------------------------------------------------
uint8_t *nas_TOOL_get_protocol6(struct ipv6hdr *iph, uint8_t *protocol)
{
  //---------------------------------------------------------------------------
  uint16_t size;
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_GET_PROTOCOL6 - begin \n");
#endif

  if (iph==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_GET_PROTOCOL6 - input parameter iph is NULL \n");
#endif
    return NULL;
  }

  if (protocol==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_GET_PROTOCOL6 - input parameter protocol is NULL \n");
#endif
    return NULL;
  }

  // End debug information
  *protocol=iph->nexthdr;
  size=NAS_IPV6_SIZE;

  while (1) {
    switch(*protocol) {
    case IPPROTO_UDP:
    case IPPROTO_TCP:
    case IPPROTO_ICMPV6:
      return (uint8_t *)((uint8_t *)iph+size);

    case IPPROTO_HOPOPTS:
    case IPPROTO_ROUTING:
    case IPPROTO_DSTOPTS:
      *protocol=((uint8_t *)iph+size)[0];
      size+=((uint8_t *)iph+size)[1]*8+8;
      break;

    case IPPROTO_FRAGMENT:
      *protocol=((uint8_t *)iph+size)[0];
      size+=((uint8_t *)iph+size)[1]+8;
      break;

    case IPPROTO_NONE:
    case IPPROTO_AH:
    case IPPROTO_ESP:
    default:
      return NULL;
    }
  }
}

//---------------------------------------------------------------------------
uint8_t *nas_TOOL_get_protocol4(struct iphdr *iph, uint8_t *protocol)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_GET_PROTOCOL4 - begin \n");
#endif

  if (iph==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_GET_PROTOCOL4 - input parameter iph is NULL \n");
#endif
    return NULL;
  }

  if (protocol==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_GET_PROTOCOL4 - input parameter protocol is NULL \n");
#endif
    return NULL;
  }

  // End debug information
  *protocol=iph->protocol;

  switch(*protocol) {
  case IPPROTO_UDP:
  case IPPROTO_TCP:
  case IPPROTO_ICMP:
    return (uint8_t *)((uint8_t *)iph+iph->tot_len);

  default:
    return NULL;
  }
}

//---------------------------------------------------------------------------
// Convert the IMEI to iid
void nas_TOOL_imei2iid(uint8_t *imei, uint8_t *iid)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_IMEI2IID - begin \n");
#endif

  if (imei==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_IMEI2IID - input parameter imei is NULL \n");
#endif
    return;
  }

  if (iid==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_IMEI2IID - input parameter iid is NULL \n");
#endif
    return;
  }

  // End debug information
  memset(iid, 0, NAS_ADDR_LEN);
  iid[0] = 0x03;
  iid[1] = 16*imei[0]+imei[1];
  iid[2] = 16*imei[2]+imei[3];
  iid[3] = 16*imei[4]+imei[5];
  iid[4] = 16*imei[6]+imei[7];
  iid[5] = 16*imei[8]+imei[9];
  iid[6] = 16*imei[10]+imei[11];
  iid[7] = 16*imei[12]+imei[13];
}

//struct udphdr *nas_TOOL_get_udp4(struct iphdr *iph)
//{
//  return (struct udphdr *)((char *)iph+NAS_IPV4_SIZE); // to modify
//}


//---------------------------------------------------------------------------
char *nas_TOOL_get_udpmsg(struct udphdr *udph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_GET_UDPMSG - begin \n");
#endif

  if (udph==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_GET_UDPMSG - input parameter udph is NULL \n");
#endif
    return NULL;
  }

  // End debug information
  return ((char *)udph+sizeof(struct udphdr));
}

//---------------------------------------------------------------------------
// Compute the UDP checksum (the data size must be odd)
uint16_t nas_TOOL_udpcksum(struct in6_addr *saddr, struct in6_addr *daddr, uint8_t proto, uint32_t udplen, void *data)
{
  //---------------------------------------------------------------------------
  uint32_t i;
  uint16_t *data16;
  uint32_t csum=0;

  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_UDPCKSUM - begin \n");
#endif

  if (saddr==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_UDPCKSUM - input parameter saddr is NULL \n");
#endif
    return 0;
  }

  if (daddr==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_UDPCKSUM - input parameter daddr is NULL \n");
#endif
    return 0;
  }

  if (data==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_UDPCKSUM - input parameter data is NULL \n");
#endif
    return 0;
  }

  // End debug information
  data16=data;

  for (i=0; i<8; ++i) {
    csum+=ntohs(saddr->s6_addr16[i]);

    if (csum>0xffff)
      csum-=0xffff;
  }

  for (i=0; i<8; ++i) {
    csum+=ntohs(daddr->s6_addr16[i]);

    if (csum>0xffff)
      csum-=0xffff;
  }

  csum+=(udplen>>16); // udplen checksum

  if (csum>0xffff)
    csum -= 0xffff;

  csum+=udplen & 0xffff;

  if (csum>0xffff)
    csum -= 0xffff;

  csum+=proto; // protocol checksum

  if (csum>0xffff)
    csum-=0xffff;

  for (i = 0; 2*i < udplen; i++) {
    csum+=ntohs(data16[i]);

    if (csum>0xffff)
      csum-=0xffff;
  }

  return htons((uint16_t)(~csum)&0xffff);
}

//---------------------------------------------------------------------------
void print_TOOL_pk_udp(struct udphdr *udph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("PRINT_TOOL_PK_UDP - begin \n");
#endif

  if (udph==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_PK_UDP - input parameter udph is NULL \n");
#endif
    return;
  }

  // End debug information
  if (udph!=NULL) {
    printk("UDP:\t source = %u, dest = %u, len = %u, check = %x\n", ntohs(udph->source), ntohs(udph->dest), ntohs(udph->len), udph->check);
  }
}

//---------------------------------------------------------------------------
void print_TOOL_pk_tcp(struct tcphdr *tcph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("PRINT_TOOL_PK_TDP - begin \n");
#endif

  if (tcph==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_PK_TDP - input parameter tcph is NULL \n");
#endif
    return;
  }

  // End debug information
  if (tcph!=NULL) {
    printk("TCP:\t source = %u, dest = %u\n", tcph->source, tcph->dest);
  }
}

//---------------------------------------------------------------------------
void print_TOOL_pk_icmp6(struct icmp6hdr *icmph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("PRINT_TOOL_PK_ICMP6 - begin \n");
#endif

  if (icmph==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_PK_ICMP6 - input parameter icmph is NULL \n");
#endif
    return;
  }

  // End debug information
  if (icmph!=NULL) {
    printk("ICMPv6:\t type= %d, code = %d\n", icmph->icmp6_type, icmph->icmp6_code);

    switch(icmph->icmp6_type) {
    case ICMPV6_DEST_UNREACH:
      printk("Destination unreachable\n");
      break;

    case ICMPV6_PKT_TOOBIG:
      printk("Packet too big\n");
      break;

    case ICMPV6_TIME_EXCEED:
      printk("Time exceeded\n");
      break;

    case ICMPV6_PARAMPROB:
      printk("Parameter problem\n");
      break;

    case ICMPV6_ECHO_REQUEST:
      printk("Echo request\n");
      break;

    case ICMPV6_ECHO_REPLY:
      printk("Echo reply\n");
      break;

    case ICMPV6_MGM_QUERY:
      printk("Multicast listener query\n");
      break;

    case ICMPV6_MGM_REPORT:
      printk("Multicast listener report\n");
      break;

    case ICMPV6_MGM_REDUCTION:
      printk("Multicast listener done\n");
      break;

    case NDISC_ROUTER_SOLICITATION:
      printk("Router solicitation\n");
      break;

    case NDISC_ROUTER_ADVERTISEMENT:
      printk("Router advertisment\n");
      break;

    case NDISC_NEIGHBOUR_SOLICITATION:
      printk("Neighbour solicitation\n");
      break;

    case NDISC_NEIGHBOUR_ADVERTISEMENT:
      printk("Neighbour advertisment\n");
      break;

    case NDISC_REDIRECT:
      printk("redirect message\n");
      break;
    }
  }
}

//---------------------------------------------------------------------------
void print_TOOL_pk_ipv6(struct ipv6hdr *iph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("PRINT_TOOL_PK_IPv6 - begin \n");
#endif

  if (iph==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_PK_IPv6 - input parameter iph is NULL \n");
#endif
    return;
  }

  // End debug information

  if (iph!=NULL) {
    //    char addr[NAS_INET6_ADDRSTRLEN];
    printk("IP:\t version = %u, priority = %u, payload_len = %u\n", iph->version, iph->priority, ntohs(iph->payload_len));
    printk("\t fl0 = %u, fl1 = %u, fl2 = %u\n",iph->flow_lbl[0],iph->flow_lbl[1],iph->flow_lbl[2]);
    printk("\t next header = %u, hop_limit = %u\n", iph->nexthdr, iph->hop_limit);

    //    inet_ntop(AF_INET6, (void *)(&iph->saddr), addr, NAS_INET6_ADDRSTRLEN);
    //    printk("\t saddr = %s",addr);
    //    inet_ntop(AF_INET6, (void *)(&iph->daddr), addr, NAS_INET6_ADDRSTRLEN);
    //    printk(", daddr = %s\n",addr);
    switch(iph->nexthdr) {
    case IPPROTO_UDP:
      print_TOOL_pk_udp((struct udphdr *)((char *)iph+sizeof(struct ipv6hdr)));
      break;

    case IPPROTO_TCP:
      print_TOOL_pk_tcp((struct tcphdr *)((char *)iph+sizeof(struct ipv6hdr)));
      break;

    case IPPROTO_ICMPV6:
      print_TOOL_pk_icmp6((struct icmp6hdr *)((char *)iph+sizeof(struct ipv6hdr)));
      break;

    case IPPROTO_IPV6:
      print_TOOL_pk_ipv6((struct ipv6hdr *)((char *)iph+sizeof(struct ipv6hdr)));
      break;

    default:
      printk("Unknown upper layer\n");
    }
  }
}

//---------------------------------------------------------------------------
void print_TOOL_pk_ipv4(struct iphdr *iph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("PRINT_TOOL_PK_IPv4 - begin \n");
#endif

  if (iph==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_PK_IPv4 - input parameter iph is NULL \n");
#endif
    return;
  }

  // End debug information

  if (iph!=NULL) {
    //    char addr[NAS_INET_ADDRSTRLEN];
    printk("IP:\t version = %u, IP length = %u\n", iph->version, iph->ihl);
    //    inet_ntop(AF_INET, (void *)(&iph->saddr), addr, NAS_INET_ADDRSTRLEN);
    //    printk("\t saddr = %s", addr);
    //    inet_ntop(AF_INET, (void *)(&iph->saddr), addr, NAS_INET_ADDRSTRLEN);
    //    printk("\t saddr = %s", addr);
  }
}

//---------------------------------------------------------------------------
void print_TOOL_pk_all(struct sk_buff *skb)
{
  //---------------------------------------------------------------------------
  printk("Skb:\t %p, len = %u\n", skb, skb->len);
  //navid: need to calculate the current used space: fixme?
  printk("Skb:\t available buf space = %u \n", skb->truesize);

  switch (ntohs(skb->protocol)) {
  case ETH_P_IPV6:
    print_TOOL_pk_ipv6((struct ipv6hdr *)skb_network_header(skb));
    break;

  case ETH_P_IP:
    print_TOOL_pk_ipv4((struct iphdr *)skb_network_header(skb));
    break;
  }
}

//---------------------------------------------------------------------------
/*int isdigit(char c)
{
  switch(c)
  {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    return 1;
  default:
    return 0;
  }
}*/

/*int nas_TOOL_inet_pton4(char *src, uint32_t *dst)
{
  uint32_t val;
  int n;
  uint8_t c;
  uint32_t parts[4];

  c = *src;
  val=0;
  n=0
  for (;;)
  {
    for (;;)
    {
      if (isdigit(c))
      {
        val = (val * 10) + c - '0';
        c = *++src;
      }
      else
        break;
    }
    if (c == '.')
    {
      if (n>4)
        return -1;
      parts[n]=val;
      c = *++src;
      ++n;
    }
    else
      break;
  }
  if ((c != '\0')||(n!=3))
    return (0);
  if ((parts[0] | parts[1] | parts[2] | val) > 256)
    return (0);
  val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
  if (dst)
    dst = htonl(val);
  return (1);
}*/

//---------------------------------------------------------------------------
void print_TOOL_state(uint8_t state)
{
  //---------------------------------------------------------------------------
  //  case NAS_STATE_IDLE:printk(" State NAS_STATE_IDLE\n");return;
  //  case NAS_STATE_CONNECTED:printk(" State NAS_STATE_CONNECTED\n");return;
  //  case NAS_STATE_ESTABLISHMENT_REQUEST:printk(" State NAS_STATE_ESTABLISHMENT_REQUEST\n");return;
  //  case NAS_STATE_ESTABLISHMENT_FAILURE:printk(" State NAS_STATE_ESTABLISHMENT_FAILURE\n");return;
  //  case NAS_STATE_RELEASE_FAILURE:printk(" State NAS_STATE_RELEASE_FAILURE\n");return;

  switch(state) {
  case  NAS_IDLE:
    printk("NAS_IDLE\n");
    return;

  case  NAS_CX_FACH:
    printk("NAS_CX_FACH\n");
    return;

  case  NAS_CX_DCH:
    printk("NAS_CX_DCH\n");
    return;

  case  NAS_CX_RECEIVED:
    printk("NAS_CX_RECEIVED\n");
    return;

  case  NAS_CX_CONNECTING:
    printk("NAS_CX_CONNECTING\n");
    return;

  case  NAS_CX_RELEASING:
    printk("NAS_CX_RELEASING\n");
    return;

  case  NAS_CX_CONNECTING_FAILURE:
    printk("NAS_CX_CONNECTING_FAILURE\n");
    return;

  case  NAS_CX_RELEASING_FAILURE:
    printk("NAS_CX_RELEASING_FAILURE\n");
    return;

  case  NAS_RB_ESTABLISHING:
    printk("NAS_RB_ESTABLISHING\n");
    return;

  case  NAS_RB_RELEASING:
    printk("NAS_RB_RELEASING\n");
    return;

  case  NAS_RB_DCH:
    printk("NAS_RB_DCH\n");
    return;

  default:
    printk(" Unknown state\n");
  }
}

//-----------------------------------------------------------------------------
// Print the content of a buffer in hexadecimal
void nas_tool_print_buffer(char * buffer,int length)
{
  //-----------------------------------------------------------------------------
  int i;

  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_TOOL_PRINT_BUFFER - begin \n");
#endif

  if (buffer==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_TOOL_PRINT_BUFFER - input parameter buffer is NULL \n");
#endif
    return;
  }

  // End debug information
  printk("\nBuffer content: ");

  for (i=0; i<length; i++)
    printk("-%hx-",buffer[i]);

  printk(",\t length %d\n", length);
}
//-----------------------------------------------------------------------------
void nas_print_rb_entity(struct rb_entity *rb)
{
  //-----------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_PRINT_RB_ENTITY - begin \n");
#endif

  if (rb==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_PRINT_RB_ENTITY - input parameter rb is NULL \n");
#endif
    return;
  }

  // End debug information
  printk("\nrb_entity content: rab_id %d, sapi %d, qos %d, \n", rb->rab_id, rb->sapi, rb->qos);
  printk("state %d, retry %d, countimer %d\n",rb->state, rb->retry, rb->countimer);
};

//-----------------------------------------------------------------------------
void nas_print_classifier(struct classifier_entity *gc)
{
  //-----------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("NAS_PRINT_GC_ENTITY - begin \n");
#endif

  if (gc==NULL) {
#ifdef NAS_DEBUG_TOOL
    printk("NAS_PRINT_GC_ENTITY - input parameter gc is NULL \n");
#endif
    return;
  }

  // End debug information
  printk("\nClassifier content: classref %d, version %d, splen %d, dplen %d,\n", gc->classref, gc->version, gc->splen, gc->dplen);
  printk("protocol %d, sport %d, dport %d, rab_id %d\n", gc->protocol, gc->sport, gc->dport, gc->rab_id);

  if (gc->rb != NULL) {
    nas_print_rb_entity(gc->rb);
  }
};


