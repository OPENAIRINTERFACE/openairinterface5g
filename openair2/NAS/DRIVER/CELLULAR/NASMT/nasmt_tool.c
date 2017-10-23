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
                          nasmt_tool.c  -  description
 ***************************************************************************

 ***************************************************************************/
/*! \file nasmt_tool.c
* \brief Tool functions for OpenAirInterface CELLULAR version - MT
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/

#include "nasmt_variables.h"
#include "nasmt_proto.h"

//---------------------------------------------------------------------------
//
void nasmt_TOOL_fct(struct classifier_entity *gc, uint8_t fct)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_fct - begin \n");
#endif

  if (gc==NULL) {
    printk("nasmt_TOOL_fct - input parameter gc is NULL \n");
    return;
  }

  // End debug information
  switch(fct) {
  case NAS_FCT_QOS_SEND:
    gc->fct=nasmt_COMMON_QOS_send;
    break;

  case NAS_FCT_CTL_SEND:
    gc->fct=nasmt_CTL_send;
    break;

  case NAS_FCT_DC_SEND:
    gc->fct=nasmt_ASCTL_DC_send_sig_data_request;
    break;

  case NAS_FCT_DEL_SEND:
    gc->fct=nasmt_COMMON_del_send;
    break;

  default:
    gc->fct=nasmt_COMMON_del_send;
  }
}

//---------------------------------------------------------------------------
uint8_t nasmt_TOOL_invfct(struct classifier_entity *gc)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_invfct - begin \n");
#endif

  if (gc==NULL) {
    printk("nasmt_TOOL_invfct - input parameter gc is NULL \n");
    return 0;
  }

  // End debug information
  if (gc->fct==nasmt_COMMON_QOS_send)
    return NAS_FCT_QOS_SEND;

  if (gc->fct==nasmt_CTL_send)
    return NAS_FCT_CTL_SEND;

  if (gc->fct==nasmt_COMMON_del_send)
    return NAS_FCT_DEL_SEND;

  if (gc->fct==nasmt_ASCTL_DC_send_sig_data_request)
    return NAS_FCT_DC_SEND;

  return 0;
}

//---------------------------------------------------------------------------
uint8_t nasmt_TOOL_get_dscp6(struct ipv6hdr *iph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_get_dscp6 - begin \n");
#endif

  if (iph==NULL) {
    printk("nasmt_TOOL_get_dscp6 - input parameter ipv6hdr is NULL \n");
    return 0;
  }

  // End debug information
  //  return (ntohl(((*(__u32 *)iph)&NAS_TRAFFICCLASS_MASK)))>>22; //Yan
  return (ntohl(((*(__u32 *)iph)&NAS_TRAFFICCLASS_MASK)))>>20;
}

//---------------------------------------------------------------------------
uint8_t nasmt_TOOL_get_dscp4(struct iphdr *iph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_get_dscp4 - begin \n");
#endif

  if (iph==NULL) {
    printk("nasmt_TOOL_get_dscp4 - input parameter ipv4hdr is NULL \n");
    return 0;
  }

  // End debug information
  return ((iph->tos)>>5)<<3;
}


//---------------------------------------------------------------------------
uint8_t *nasmt_TOOL_get_protocol6(struct ipv6hdr *iph, uint8_t *protocol)
{
  //---------------------------------------------------------------------------
  uint16_t size;
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_get_protocol6 - begin \n");
#endif

  if (iph==NULL) {
    printk("nasmt_TOOL_get_protocol6 - input parameter ipv6hdr is NULL \n");
    return NULL;
  }

  if (protocol==NULL) {
    printk("nasmt_TOOL_get_protocol6 - input parameter protocol is NULL \n");
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
uint8_t *nasmt_TOOL_get_protocol4(struct iphdr *iph, uint8_t *protocol)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_get_protocol4 - begin \n");
#endif

  if (iph==NULL) {
    printk("nasmt_TOOL_get_protocol4 - input parameter iph is NULL \n");
    return NULL;
  }

  if (protocol==NULL) {
    printk("nasmt_TOOL_get_protocol4 - input parameter protocol is NULL \n");
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
void nasmt_TOOL_imei2iid(uint8_t *imei, uint8_t *iid)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_imei2iid - begin \n");
#endif

  if (imei==NULL) {
    printk("nasmt_TOOL_imei2iid - input parameter imei is NULL \n");
    return;
  }

  if (iid==NULL) {
    printk("nasmt_TOOL_imei2iid - input parameter iid is NULL \n");
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

//---------------------------------------------------------------------------
// Convert the IMEI to iid
void nasmt_TOOL_eth_imei2iid(unsigned char *imei, unsigned char *addr ,unsigned char *iid, unsigned char len)
{
  //---------------------------------------------------------------------------
  unsigned int index;
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_eth_imei2iid - begin \n");
#endif

  if (!imei || !addr || !iid) {
    printk("OAI_NW_DRV_TOOL_eNB_IMEI2IID - input parameter imei, addr or iid is NULL \n");
    return;
  }

  // End debug information
  // set addr (should be device HW address)
  memset(addr, 0, len);
  addr[0] = 0x00;  // to be compatible between link local and global

  // len -1 because of insertion of 0 above
  for (index = 0; index < (len-1); index++) {
    addr[index+1] = 16*imei[index*2]+imei[index*2+1];
  }

  // set iid6
  memset(iid, 0, NAS_ADDR_LEN);
  iid[0] = 0x02;
  iid[1] = 16*imei[0]+imei[1];
  iid[2] = 16*imei[2]+imei[3];
  iid[3] = 0xff;
  iid[4] = 0xfe;
  iid[5] = 16*imei[4]+imei[5];
  iid[6] = 16*imei[6]+imei[7];
  iid[7] = 16*imei[8]+imei[9];

}

//---------------------------------------------------------------------------
void nasmt_TOOL_pk_icmp6(struct icmp6hdr *icmph)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef GRAAL_DEBUG_TOOL
  printk("nasmt_TOOL_pk_icmp6 - begin \n");
#endif

  if (!icmph) {
    printk("nasmt_TOOL_pk_icmp6 - input parameter icmph is NULL \n");
    return;
  }

  // End debug information
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

//---------------------------------------------------------------------------
void nasmt_TOOL_print_state(uint8_t state)
{
  //---------------------------------------------------------------------------
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
    printk("nasmt_TOOL_print_state - Unknown state\n");
  }
}

//-----------------------------------------------------------------------------
// Print the content of a buffer in hexadecimal
void nasmt_TOOL_print_buffer(unsigned char * buffer,int length)
{
  //-----------------------------------------------------------------------------
  int i;

  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_print_buffer - begin \n");
#endif

  if (buffer==NULL) {
    printk("nasmt_TOOL_print_buffer - input parameter buffer is NULL \n");
    return;
  }

  // End debug information
  printk("\nBuffer content: ");

  for (i=0; i<length; i++)
    printk("-%hx-",buffer[i]);

  printk(",\t length %d\n", length);
}
//-----------------------------------------------------------------------------
void nasmt_TOOL_print_rb_entity(struct rb_entity *rb)
{
  //-----------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_print_rb_entity - begin \n");
#endif

  if (rb==NULL) {
    printk("nasmt_TOOL_print_rb_entity - input parameter rb is NULL \n");
    return;
  }

  // End debug information
  printk("\nrb_entity content: rab_id %d, sapi %d, qos %d, dscp %d, \n", rb->rab_id, rb->sapi, rb->qos, rb->dscp);
  printk("state %d, retry %d, countimer %d\n",rb->state, rb->retry, rb->countimer);
};

//-----------------------------------------------------------------------------
void nasmt_TOOL_print_classifier(struct classifier_entity *gc)
{
  //-----------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_TOOL
  printk("nasmt_TOOL_print_classifier - begin \n");
#endif

  if (gc==NULL) {
    printk("nasmt_TOOL_print_classifier - input parameter gc is NULL \n");
    return;
  }

  // End debug information
  printk("\nClassifier content: classref %d, version %d, splen %d, dplen %d,\n", gc->classref, gc->version, gc->splen, gc->dplen);
  printk("protocol %d, sport %d, dport %d, rab_id %d\n", gc->protocol, gc->sport, gc->dport, gc->rab_id);
  nasmt_TOOL_print_rb_entity(gc->rb);
};


