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

/*! \file classifier.c
* \brief Classify IP packets
* \author  Navid Nikaein, Lionel GAUTHIER,  Raymond knopp
* \company Eurecom
* \email:  navid.nikaein@eurecom.fr, lionel.gauthier@eurecom.fr, knopp@eurecom.fr
*/


#include "local.h"
#include "proto_extern.h"

#include <net/ip6_fib.h>
#include <net/route.h>


//#define MPLS

#ifdef MPLS
  #include <net/mpls.h>
#endif


//#define NAS_DEBUG_CLASS 1
//#define NAS_DEBUG_SEND 1

//---------------------------------------------------------------------------
// Add a new classifier rule (send direction)
struct classifier_entity *nas_CLASS_add_sclassifier(struct cx_entity *cx, uint8_t dscp, uint16_t classref) {
  //---------------------------------------------------------------------------
  struct classifier_entity *gc;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_ADD_SCLASSIFIER: begin for dscp %d, classref %d\n", dscp,classref);
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_CLASS_ADD_SCLASSIFIER - input parameter cx is NULL \n");
#endif
    return NULL;
  }

  for (gc=cx->sclassifier[dscp]; gc!=NULL; gc=gc->next) {
    if (gc->classref==classref) {
#ifdef NAS_DEBUG_CLASS
      printk("NAS_CLASS_ADD_SCLASSIFIER: classifier already exist for dscp %d, classref %d\n",dscp,classref);
#endif
      return gc;
    }
  }

  gc=(struct classifier_entity *)kmalloc(sizeof(struct classifier_entity), GFP_KERNEL);

  if (gc==NULL)
    return NULL;

  gc->next=cx->sclassifier[dscp];
  gc->classref=classref;
  cx->sclassifier[dscp]=gc;
  ++cx->nsclassifier;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_ADD_SCLASSIFIER: classifier created for dscp %d, classref %d\n",dscp,classref);
#endif
  return gc;
}

//---------------------------------------------------------------------------
// Add a new classifier rule (receive direction)
struct classifier_entity *nas_CLASS_add_rclassifier(uint8_t dscp,
    uint16_t classref,
    struct nas_priv *gpriv) {
  //---------------------------------------------------------------------------
  struct classifier_entity *gc;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_ADD_RCLASSIFIER: begin\n");
#endif

  for (gc=gpriv->rclassifier[dscp]; gc!=NULL; gc=gc->next) {
    if (gc->classref==classref) {
#ifdef NAS_DEBUG_CLASS
      printk("NAS_CLASS_ADD_RCLASSIFIER: classifier already exist for dscp %d, classref %d\n",dscp,classref);
#endif
      return gc;
    }
  }

  gc=(struct classifier_entity *)kmalloc(sizeof(struct classifier_entity), GFP_KERNEL);

  if (gc==NULL)
    return NULL;

  gc->next=gpriv->rclassifier[dscp];
  gc->classref=classref;
  gpriv->rclassifier[dscp]=gc;
  ++gpriv->nrclassifier;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_ADD_RCLASSIFIER: classifier created for dscp %d, classref %d\n",dscp,classref);
#endif
  return gc;
}

//---------------------------------------------------------------------------
// Add a new classifier rule (forwarding)
struct classifier_entity *nas_CLASS_add_fclassifier(struct cx_entity *cx, uint8_t dscp, uint16_t classref) {
  //---------------------------------------------------------------------------
  struct classifier_entity *gc;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_ADD_FCLASSIFIER: begin for dscp %d, classref %d\n", dscp,classref);
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_CLASS_ADD_FCLASSIFIER - input parameter cx is NULL \n");
#endif
    return NULL;
  }

  for (gc=cx->fclassifier[dscp]; gc!=NULL; gc=gc->next) {
    if (gc->classref==classref) {
#ifdef NAS_DEBUG_CLASS
      printk("NAS_CLASS_ADD_SCLASSIFIER: classifier already exist for dscp %d, classref %d\n",dscp,classref);
#endif
      return gc;
    }
  }

  gc=(struct classifier_entity *)kmalloc(sizeof(struct classifier_entity), GFP_KERNEL);

  if (gc==NULL)
    return NULL;

  gc->next=cx->fclassifier[dscp];
  gc->classref=classref;
  cx->fclassifier[dscp]=gc;
  ++cx->nfclassifier;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_ADD_FCLASSIFIER: classifier created for dscp %d, classref %d\n",dscp,classref);
#endif
  return gc;
}


//---------------------------------------------------------------------------
void nas_CLASS_flush_sclassifier(struct cx_entity *cx) {
  //---------------------------------------------------------------------------
  uint8_t dscpi;
  struct classifier_entity *gc;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_FLUSH_SCLASSIFIER: begin\n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_CLASS_FLUSH_SCLASSIFIER - input parameter cx is NULL \n");
#endif
    return;
  }

  //
  for (dscpi=0; dscpi<NAS_DSCP_MAX; ++dscpi) {
    for (gc=cx->sclassifier[dscpi]; gc!=NULL; gc=cx->sclassifier[dscpi]) {
      cx->sclassifier[dscpi]=gc->next;
      kfree(gc);
    }
  }

  cx->nsclassifier=0;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_FLUSH_SCLASSIFIER: end\n");
#endif
}
//---------------------------------------------------------------------------
void nas_CLASS_flush_fclassifier(struct cx_entity *cx) {
  //---------------------------------------------------------------------------
  uint8_t dscpi;
  struct classifier_entity *gc;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_FLUSH_FCLASSIFIER: begin\n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_CLASS_FLUSH_FCLASSIFIER - input parameter cx is NULL \n");
#endif
    return;
  }

  //
  for (dscpi=0; dscpi<NAS_DSCP_MAX; ++dscpi) {
    for (gc=cx->fclassifier[dscpi]; gc!=NULL; gc=cx->fclassifier[dscpi]) {
      cx->fclassifier[dscpi]=gc->next;
      kfree(gc);
    }
  }

  cx->nfclassifier=0;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_FLUSH_FCLASSIFIER: end\n");
#endif
}

//---------------------------------------------------------------------------
void nas_CLASS_flush_rclassifier(struct nas_priv *gpriv) {
  //---------------------------------------------------------------------------
  uint8_t dscpi;
  struct classifier_entity *gc;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_FLUSH_RCLASSIFIER: begin\n");
#endif

  for (dscpi=0; dscpi<NAS_DSCP_MAX; ++dscpi) {
    for (gc=gpriv->rclassifier[dscpi]; gc!=NULL; gc=gpriv->rclassifier[dscpi]) {
      gpriv->rclassifier[dscpi]=gc->next;
      kfree(gc);
    }
  }

  gpriv->nrclassifier=0;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_FLUSH_RCLASSIFIER: end\n");
#endif
}

//---------------------------------------------------------------------------
// Delete a classifier rule (send direction)
void nas_CLASS_del_sclassifier(struct cx_entity *cx, uint8_t dscp, uint16_t classref) {
  //---------------------------------------------------------------------------
  struct classifier_entity *p,*np;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_DEL_SCLASSIFIER: begin\n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_CLASS_DEL_SCLASSIFIER - input parameter cx is NULL \n");
#endif
    return;
  }

  //
  p=cx->sclassifier[dscp];

  if (p==NULL)
    return;

  if (p->classref==classref) {
    cx->sclassifier[dscp]=p->next;
    kfree(p);
    --cx->nsclassifier;
    return;
  }

  for (np=p->next; np!=NULL; p=np) {
    if (np->classref==classref) {
      p->next=np->next;
      kfree(np);
      --cx->nsclassifier;
      return;
    }
  }

#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_DEL_SCLASSIFIER: end\n");
#endif
}

//---------------------------------------------------------------------------
// Delete a classifier rule (send direction)
void nas_CLASS_del_fclassifier(struct cx_entity *cx, uint8_t dscp, uint16_t classref) {
  //---------------------------------------------------------------------------
  struct classifier_entity *p,*np;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_DEL_FCLASSIFIER: begin\n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_CLASS_DEL_FCLASSIFIER - input parameter cx is NULL \n");
#endif
    return;
  }

  //
  p=cx->fclassifier[dscp];

  if (p==NULL)
    return;

  if (p->classref==classref) {
    cx->fclassifier[dscp]=p->next;
    kfree(p);
    --cx->nfclassifier;
    return;
  }

  for (np=p->next; np!=NULL; p=np) {
    if (np->classref==classref) {
      p->next=np->next;
      kfree(np);
      --cx->nfclassifier;
      return;
    }
  }

#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_DEL_FCLASSIFIER: end\n");
#endif
}

//---------------------------------------------------------------------------
// Delete a classifier rule (receive direction)
void nas_CLASS_del_rclassifier(uint8_t dscp, uint16_t classref,struct nas_priv *gpriv) {
  //---------------------------------------------------------------------------
  struct classifier_entity *p,*np;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_DEL_RCLASSIFIER: begin\n");
#endif
  p=gpriv->rclassifier[dscp];

  if (p==NULL)
    return;

  if (p->classref==classref) {
    gpriv->rclassifier[dscp]=p->next;
    kfree(p);
    --gpriv->nrclassifier;
    return;
  }

  for (np=p->next; np!=NULL; p=np) {
    if (np->classref==classref) {
      p->next=np->next;
      kfree(np);
      --gpriv->nrclassifier;
      return;
    }
  }

#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_DEL_RCLASSIFIER: end\n");
#endif
}

//---------------------------------------------------------------------------
// Search the entity with the IPv6 address 'addr'
// Navid: the ipv6 classifier is not fully tested
struct cx_entity *nas_CLASS_cx6(struct sk_buff *skb,
                                unsigned char dscp,
                                struct nas_priv *gpriv,
                                int inst,
                                unsigned char *cx_searcher) {
  //---------------------------------------------------------------------------
  unsigned char cxi;
  unsigned int *addr,*dst=NULL;
  struct cx_entity *default_ip=NULL;
  struct classifier_entity *p=NULL;

  if (skb!=NULL) {
    for (cxi=*cx_searcher; cxi<NAS_CX_MAX; cxi++) {
      (*cx_searcher)++;
      p = gpriv->cx[cxi].sclassifier[dscp];

      while (p!=NULL) {
        if (p->version == 6) {   // verify that this is an IPv4 rule
          if ((addr = (unsigned int *)(&(p->daddr.ipv6)))== NULL) {
            printk("nas_CLASS_cx6: addr is null \n");
            p = p->next;
            continue;
          }

#ifdef NAS_DEBUG_CLASS
          printk("cx %d : %X,%X.%X,%X\n",cxi,addr[0],addr[1],addr[2],addr[3]);
#endif //NAS_DEBUG_CLASS


          if ( (dst = &((struct iphdr *)(skb_network_header(skb)))->daddr) == NULL) {
            printk("nas_CLASS_cx6: dst addr is null \n");
            p = p->next;
            continue;
          }

          if ((addr[0] == dst[0]) &&
              (addr[1] == dst[1]) &&
              (addr[2] == dst[2]) &&
              (addr[3] == dst[3])) {
            //#ifdef NAS_DEBUG_CLASS
            printk("nas_CLASS_cx6: found cx %d: %X.%X.%X.%X\n",cxi,
                   dst[0],
                   dst[1],
                   dst[2],
                   dst[3]);
            //#endif //NAS_DEBUG_CLASS
            return gpriv->cx+cxi;
          }

          /*
               else if ((addr[0]==NAS_DEFAULT_IPV6_ADDR0) &&
               (addr[1]==NAS_DEFAULT_IPV6_ADDR1) &&
               (addr[2]==NAS_DEFAULT_IPV6_ADDR2) &&
               (addr[3]==NAS_DEFAULT_IPV6_ADDR3))
               default_ip = gpriv->cx+cxi;
          */
        }

        // Go to next classifier entry for connection
        p = p->next;
      }
    }
  }


  if (dst ) {
    printk("nas_CLASS_cx6 NOT FOUND: %X.%X.%X.%X\n",
           dst[0],
           dst[1],
           dst[2],
           dst[3]);
  }

  return default_ip;
}

//---------------------------------------------------------------------------
// Search the entity with the IPv4 address 'addr'
struct cx_entity *nas_CLASS_cx4(struct sk_buff *skb,
                                unsigned char dscp,
                                struct nas_priv *gpriv,
                                int inst,
                                unsigned char *cx_searcher) {
  //---------------------------------------------------------------------------
  unsigned char cxi;
  unsigned char *addr;
  uint32_t daddr;
  struct cx_entity *default_ip=NULL;
  struct classifier_entity *p=NULL;

  //  if (inst >0)
  //    return(gpriv->cx);  //dump to clusterhead

  if (skb!=NULL) {
    daddr = ((struct iphdr *)(skb_network_header(skb)))->daddr;

    if (daddr!=0) {
#ifdef NAS_DEBUG_CLASS
      printk("[NAS][CLASS][IPv4] Searching for %d.%d.%d.%d\n",
             ((unsigned char *)&daddr)[0],
             ((unsigned char *)&daddr)[1],
             ((unsigned char *)&daddr)[2],
             ((unsigned char *)&daddr)[3]);
#endif

      for (cxi=*cx_searcher; cxi<NAS_CX_MAX; ++cxi) {
        (*cx_searcher)++;
        p = gpriv->cx[cxi].sclassifier[dscp];

        while (p!=NULL) {
          if (p->version == 4) {   // verify that this is an IPv4 rule
#ifdef NAS_DEBUG_CLASS
            addr = (char *)(&(p->daddr.ipv4));
            printk("found classifier cx %d for destination: %d.%d.%d.%d\n",cxi,addr[0],addr[1],addr[2],addr[3]);
#endif //NAS_DEBUG_CLASS

            if ((p->daddr.ipv4)== daddr) {
              addr = (char *)(&(p->daddr.ipv4));
#ifdef NAS_DEBUG_CLASS
              printk("found cx %d: %d.%d.%d.%d\n",cxi,
                     addr[0],
                     addr[1],
                     addr[2],
                     addr[3]);
#endif //NAS_DEBUG_CLASS
              return gpriv->cx+cxi;
            }

            /*
              else if (gpriv->cx[cxi].sclassifier[dscp]->daddr.ipv4==NAS_DEFAULT_IPV4_ADDR) {
              #ifdef NAS_DEBUG_CLASS
              printk("found default_ip rule\n");
              #endif //NAS_DEBUG_CLASS
              default_ip = gpriv->cx+cxi;
              }
            */
          }

          // goto to next classification rule for the connection
          p = p->next;
        }
      }
    }
  }

  return default_ip;
}

#ifdef MPLS
//---------------------------------------------------------------------------
// Search the entity with the mpls label and given exp
struct cx_entity *nas_CLASS_MPLS(struct sk_buff *skb,
                                 unsigned char exp,
                                 struct nas_priv *gpriv,
                                 int inst,
                                 unsigned char *cx_searcher) {
  //---------------------------------------------------------------------------
  unsigned char cxi;
  struct cx_entity *default_label=NULL;
  struct classifier_entity *p=NULL;
  //  if (inst >0)
  //    return(gpriv->cx);  //dump to clusterhead
#ifdef NAS_DEBUG_CLASS
  printk("[NAS][CLASS][MPLS] Searching for label %d\n",MPLSCB(skb)->label);
#endif

  for (cxi=*cx_searcher; cxi<NAS_CX_MAX; ++cxi) {
    (*cx_searcher)++;
    p = gpriv->cx[cxi].sclassifier[exp];

    while (p!=NULL) {
      if (p->version == NAS_MPLS_VERSION_CODE) {   // verify that this is an MPLS rule
#ifdef NAS_DEBUG_CLASS
        printk("cx %d : label %d\n",cxi,p->daddr.mpls_label);
#endif //NAS_DEBUG_CLASS

        if (p->daddr.mpls_label==MPLSCB(skb)->label) {
#ifdef NAS_DEBUG_CLASS
          printk("found cx %d: label %d, RB %d\n",cxi,
                 MPLSCB(skb)->label,
                 p->rab_id);
#endif //NAS_DEBUG_CLASS
          return gpriv->cx+cxi;
        }

        /*
        else if (gpriv->cx[cxi].sclassifier[dscp]->daddr.ipv4==NAS_DEFAULT_IPV4_ADDR) {
        #ifdef NAS_DEBUG_CLASS
        printk("found default_ip rule\n");
        #endif //NAS_DEBUG_CLASS
        default_ip = gpriv->cx+cxi;
        }
        */
      }

      // goto to next classification rule for the connection
      p = p->next;
    }
  }

  return default_label;
}

#endif

//---------------------------------------------------------------------------
// Search the sending function
void nas_CLASS_send(struct sk_buff *skb,int inst) {
  //---------------------------------------------------------------------------
  struct classifier_entity *p, *sp;
  uint8_t *protocolh,version;
  uint8_t protocol, dscp /*, exp,label*/ ;
  uint16_t classref;
  struct cx_entity *cx;
  //unsigned int i;
  //unsigned int router_adv = 0;
  struct net_device *dev=nasdev[inst];
  struct nas_priv *gpriv=netdev_priv(dev);
  unsigned char cx_searcher,no_connection=1;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_SEND: begin - inst %d\n",inst);
#endif

  if (skb==NULL) {
#ifdef NAS_DEBUG_SEND
    printk("NAS_CLASS_SEND - input parameter skb is NULL \n");
#endif
    return;
  }

#ifdef NAS_DEBUG_SEND
  printk("[NAS][CLASS][SEND] Got packet from kernel:\n");

  for (i=0; i<256; i++)
    printk("%2x ",((unsigned char *)skb->data)[i]);

  printk("\n");
#endif
  // find all connections related to socket
  cx_searcher = 0;
  no_connection = 1;
  //while (cx_searcher<NAS_CX_MAX) {
  cx = NULL;

  // Address classification
  switch (ntohs(skb->protocol)) {
    case ETH_P_IPV6:
      version = 6;
      protocolh=nas_TOOL_get_protocol6(
                  (struct ipv6hdr *)(skb_network_header(skb)),
                  &protocol);
      dscp=nas_TOOL_get_dscp6(
             (struct ipv6hdr *)(skb_network_header(skb))
           );
#ifdef NAS_DEBUG_CLASS
      printk("NAS_CLASS_SEND: %p %d %p %d %p \n",skb, dscp, gpriv, inst, &cx_searcher);
#endif
      cx=nas_CLASS_cx6(skb,dscp,gpriv,inst,&cx_searcher);
#ifdef NAS_DEBUG_CLASS
      printk("NAS_CLASS_SEND: Got IPv6 packet, dscp = %d\n",dscp);
#endif
      break;

    case ETH_P_IP:
      dscp=nas_TOOL_get_dscp4((struct iphdr *)(skb_network_header(skb)));
      cx=nas_CLASS_cx4(skb,dscp,gpriv,inst,&cx_searcher);
      protocolh=nas_TOOL_get_protocol4(
                  (struct iphdr *)(skb_network_header(skb)),
                  &protocol);
#ifdef NAS_DEBUG_CLASS
      printk("NAS_CLASS_SEND: Got IPv4 packet (%x), dscp = %d, cx = %x\n",ntohs(skb->protocol),dscp,cx);
#endif
      version = 4;
      break;
#ifdef MPLS

    case ETH_P_MPLS_UC:
      cx=nas_CLASS_MPLS(skb,MPLSCB(skb)->exp,gpriv,inst,&cx_searcher);
#ifdef NAS_DEBUG_CLASS
      printk("NAS_CLASS_SEND: Got MPLS unicast packet, exp = %d, label = %d, cx = %x\n",MPLSCB(skb)->exp,MPLSCB(skb)->label,cx);
#endif
      dscp = MPLSCB(skb)->exp;
      version = NAS_MPLS_VERSION_CODE;
      protocol = version;
      break;
#endif

    default:
      printk("NAS_CLASS_SEND: Unknown protocol\n");
      version = 0;
      return;
  }

  // If a valid connection for the DSCP/EXP with destination address
  // is found scan all protocol-based classification rules

  if (cx) {
    classref=0;
    sp=NULL;
#ifdef NAS_DEBUG_CLASS
    printk("[NAS][CLASSIFIER] DSCP/EXP %d : looking for classifier entry\n",dscp);
#endif

    for (p=cx->sclassifier[dscp]; p!=NULL; p=p->next) {
#ifdef NAS_DEBUG_CLASS
      printk("[NAS][CLASSIFIER] DSCP %d p->classref=%d,p->protocol=%d,p->version=%d\n",dscp,p->classref,p->protocol,p->version);
#endif
      // Check if transport protocol/message match
      /*
      if ((p->protocol == protocol))
        if ((protocol == NAS_PROTOCOL_ICMP6) && (version == 6))
          if (p->protocol_message_type == (p->protocol_message_type )) {
            printk("[GRAAL][CLASSIFIER] Router advertisement\n");
          }
      */
      // normal rule checks that network protocol version matches

      if (p->version == version) {
        sp=p;
        classref=sp->classref;
        break;
      }
    }

    if (sp!=NULL) {
#ifdef NAS_DEBUG_CLASS
      char sfct[10], sprotocol[10];

      if (sp->fct==nas_COMMON_QOS_send)
        strcpy(sfct, "qos");

      if (sp->fct==nas_CTL_send)
        strcpy(sfct, "ctl");

      if (sp->fct==nas_COMMON_del_send)
        strcpy(sfct, "del");

      if (sp->fct==nas_mesh_DC_send_sig_data_request)
        strcpy(sfct, "dc");

      switch(protocol) {
        case NAS_PROTOCOL_UDP:
          strcpy(sprotocol, "udp");
          printk("udp packet\n");
          break;

        case NAS_PROTOCOL_TCP:
          strcpy(sprotocol, "tcp");
          printk("tcp packet\n");
          break;

        case NAS_PROTOCOL_ICMP4:
          strcpy(sprotocol, "icmp4");
          printk("icmp4 packet\n");
          break;

        case NAS_PROTOCOL_ICMP6:
          strcpy(sprotocol, "icmp6");
          print_TOOL_pk_icmp6((struct icmp6hdr *)protocolh);
          break;
#ifdef MPLS

        case NAS_MPLS_VERSION_CODE:
          strcpy(sprotocol,"mpls");
          break;
#endif
      }

      printk("NAS_CLASS_SEND: (dscp %u, %s) received, (classref %u, fct %s, rab_id %u) classifier rule\n",
             dscp, sprotocol, sp->classref, sfct, sp->rab_id);
#endif
      sp->fct(skb, cx, sp,inst, NULL);
    } // if classifier entry match found
    else {
      printk("NAS_CLASS_SEND: no corresponding item in the classifier, so the message is dropped\n");
      //  nas_COMMON_del_send(skb, cx, NULL,inst);
    }

    no_connection = 0;
  }   // if connection found

#ifdef NAS_DEBUG_CLASS

  if (no_connection == 1)
    printk("NAS_CLASS_SEND: no corresponding connection, so the message is dropped\n");

#endif /* NAS_DEBUG_CLASS */
  //  }   // while loop over connections
#ifdef NAS_DEBUG_CLASS
  printk("NAS_CLASS_SEND: end\n");
#endif
}
