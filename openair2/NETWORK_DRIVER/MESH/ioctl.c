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
#include "ioctl.h"
#include "proto_extern.h"

//#include <linux/in.h>
#include <asm/uaccess.h>
#include <asm/checksum.h>
#include <asm/uaccess.h>


// Statistic
//---------------------------------------------------------------------------
void nas_set_msg_statistic_reply(struct nas_msg_statistic_reply *msgrep,
                                 struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  msgrep->rx_packets=priv->stats.rx_packets;
  msgrep->tx_packets=priv->stats.tx_packets;
  msgrep->rx_bytes=priv->stats.rx_bytes;
  msgrep->tx_bytes=priv->stats.tx_bytes;
  msgrep->rx_errors=priv->stats.rx_errors;
  msgrep->tx_errors=priv->stats.tx_errors;
  msgrep->rx_dropped=priv->stats.rx_dropped;
  msgrep->tx_dropped=priv->stats.tx_dropped;
}

//---------------------------------------------------------------------------
int nas_ioCTL_statistic_request(struct nas_ioctl *gifr,
                                struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct nas_msg_statistic_reply msgrep;
  printk("NAS_IOCTL_STATISTIC: stat requested\n");
  nas_set_msg_statistic_reply(&msgrep,priv);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("NAS_IOCTL_STATISTIC: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Connections List
//---------------------------------------------------------------------------
void nas_set_msg_cx_list_reply(uint8_t *msgrep,
                               struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  nasLocalConnectionRef_t lcr;
  struct nas_msg_cx_list_reply *list;
  msgrep[0]=NAS_CX_MAX;
  list=(struct nas_msg_cx_list_reply *)(msgrep+1);

  for(lcr=0; lcr<NAS_CX_MAX; ++lcr) {
    cx=nas_COMMON_search_cx(lcr,priv);
    list[lcr].lcr=lcr;
    list[lcr].state=cx->state;
    list[lcr].cellid=cx->cellid;
    list[lcr].iid4=cx->iid4;
    list[lcr].iid6[0]=cx->iid6[0];
    list[lcr].iid6[1]=cx->iid6[1];
    list[lcr].num_rb=cx->num_rb;
    list[lcr].nsclassifier=cx->nsclassifier;
    printk("NAS_SET_MSG_CX_LIST_REPLY: nsc=%u\n",cx->nsclassifier);
  }
}

//---------------------------------------------------------------------------
int nas_ioCTL_cx_list_request(struct nas_ioctl *gifr,
                              struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  uint8_t msgrep[NAS_CX_MAX*sizeof(struct nas_msg_cx_list_reply)+1];
  printk("NAS_IOCTL_CX_LIST: connection list requested\n");
  nas_set_msg_cx_list_reply(msgrep,priv);

  if (copy_to_user(gifr->msg, msgrep, NAS_CX_MAX*sizeof(struct nas_msg_cx_list_reply)+1)) {
    printk("NAS_IOCTL_CX_LIST: copy_to_user failure\n");
    return -EFAULT;
  }

  printk("NAS_IOCTL_CX_LIST: end\n");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Connection Establishment
//---------------------------------------------------------------------------
void nas_set_msg_cx_establishment_reply(struct nas_msg_cx_establishment_reply *msgrep,
                                        struct nas_msg_cx_establishment_request *msgreq,
                                        struct nas_priv *priv)
{
  //---------------------------------------------------------------------------

  struct cx_entity *cx;
  cx=nas_COMMON_search_cx(msgreq->lcr,priv);

  if (cx!=NULL) {
    cx->cellid=msgreq->cellid;
    msgrep->status=nas_mesh_DC_send_cx_establish_request(cx,priv);
  } else
    msgrep->status=-NAS_ERROR_NOTCORRECTLCR;

}
//---------------------------------------------------------------------------
int nas_ioCTL_cx_establishment_request(struct nas_ioctl *gifr,
                                       struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct nas_msg_cx_establishment_request msgreq;
  struct nas_msg_cx_establishment_reply msgrep;
  printk("NAS_IOCTL_ESTABLISHMENT: connection establishment requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("NAS_IOCTL_CX_ESTABLISHMENT: copy_from_user failure\n");
    return -EFAULT;
  }

  nas_set_msg_cx_establishment_reply(&msgrep, &msgreq,priv);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("NAS_IOCTL_CX_ESTABLISHMENT: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Connection Release
//---------------------------------------------------------------------------
void nas_set_msg_cx_release_reply(struct nas_msg_cx_release_reply *msgrep,
                                  struct nas_msg_cx_release_request *msgreq,
                                  struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  cx=nas_COMMON_search_cx(msgreq->lcr,priv);

  if (cx!=NULL)
    msgrep->status=nas_mesh_DC_send_cx_release_request(cx,priv);
  else
    msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
}

//---------------------------------------------------------------------------
// Request the release of a connection
int nas_ioCTL_cx_release_request(struct nas_ioctl *gifr,struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct nas_msg_cx_release_request msgreq;
  struct nas_msg_cx_release_reply msgrep;

  printk("NAS_IOCTL_CX_RELEASE: connection release requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("NAS_IOCTL_CX_RELEASE: copy_from_user failure\n");
    return -EFAULT;
  }

  nas_set_msg_cx_release_reply(&msgrep, &msgreq,priv);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("NAS_IOCTL_CX_RELEASE: copy_to_user failure\n");
    return -EFAULT;
  }

  printk("NAS_IOCTL_CX_RELEASE: end\n");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Radio Bearer List
//---------------------------------------------------------------------------
void nas_set_msg_rb_list_reply(uint8_t *msgrep,
                               struct nas_msg_rb_list_request *msgreq,
                               struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  cx=nas_COMMON_search_cx(msgreq->lcr,priv);

  if (cx!=NULL) {
    uint8_t rbi;
    struct rb_entity *rb;
    struct nas_msg_rb_list_reply *list;

    if (cx->num_rb > NAS_LIST_RB_MAX)
      msgrep[0] = NAS_LIST_RB_MAX;
    else
      msgrep[0] = cx->num_rb;

    list=(struct nas_msg_rb_list_reply *)(msgrep+1);

    for (rb=cx->rb, rbi=0; (rb!=NULL)&&(rbi<msgrep[0]); rb=rb->next, ++rbi) {
      list[rbi].state=rb->state;
      list[rbi].rab_id=rb->rab_id;
      list[rbi].sapi=rb->sapi;
      list[rbi].qos=rb->qos;
    }
  } else
    msgrep[0]=0;
}

//---------------------------------------------------------------------------
int nas_ioCTL_rb_list_request(struct nas_ioctl *gifr,
                              struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  uint8_t msgrep[NAS_LIST_RB_MAX*sizeof(struct nas_msg_rb_list_reply)+1];
  struct nas_msg_rb_list_request msgreq;
  printk("NAS_IOCTL_RB_LIST: Radio Bearer list requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("NAS_IOCTL_RB_LIST: copy_from_user failure\n");
    return -EFAULT;
  }

  nas_set_msg_rb_list_reply(msgrep, &msgreq,priv);

  if (copy_to_user(gifr->msg, msgrep, NAS_LIST_RB_MAX*sizeof(struct nas_msg_rb_list_reply)+1)) {
    printk("NAS_IOCTL_RB_LIST: copy_to_user failure\n");
    return -EFAULT;
  }

  printk("NAS_IOCTL_CX_LIST: end\n");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Radio Bearer Establishment
//---------------------------------------------------------------------------
void nas_set_msg_rb_establishment_reply(struct nas_msg_rb_establishment_reply *msgrep,
                                        struct nas_msg_rb_establishment_request *msgreq,
                                        struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  //  if ((msgreq->rab_id<3)||(msgreq->rab_id>127))
  if ((msgreq->rab_id<1)||(msgreq->rab_id>MAX_RABS)) // navid : increase the number
    msgrep->status=-NAS_ERROR_NOTCORRECTRABI;
  else {
    struct cx_entity *cx;
    cx=nas_COMMON_search_cx(msgreq->lcr,priv);

    if (cx==NULL)
      msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
    else {
      struct rb_entity *rb;
      rb=nas_COMMON_add_rb(cx, msgreq->rab_id, msgreq->qos);

      if (rb!=NULL) {
        //          rb->cnxid = msgreq->cnxid;
        //          msgrep->status=nas_rg_DC_send_rb_establish_request(cx, rb);
      } else
        msgrep->status=-NAS_ERROR_NOMEMORY;

      //        msgrep->cnxid  = msgreq->cnxid;
    }
  }
}

//---------------------------------------------------------------------------
int nas_ioCTL_rb_establishment_request(struct nas_ioctl *gifr,
                                       struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct nas_msg_rb_establishment_request msgreq;
  struct nas_msg_rb_establishment_reply msgrep;
  printk("NAS_IOCTL_RB_ESTABLISHMENT: Radio bearer establishment requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("NAS_IOCTL_RB_ESTABLISHMENT: copy_from_user failure\n");
    return -EFAULT;
  }

  nas_set_msg_rb_establishment_reply(&msgrep, &msgreq,priv);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("NAS_IOCTL_RB_ESTABLISHMENT: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Radio Bearer Release
//---------------------------------------------------------------------------
void nas_set_msg_rb_release_reply(struct nas_msg_rb_release_reply *msgrep,
                                  struct nas_msg_rb_release_request *msgreq,
                                  struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  if (msgreq->lcr<NAS_CX_MAX) {
    struct rb_entity *rb;
    struct cx_entity *cx;
    cx=nas_COMMON_search_cx(msgreq->lcr,priv);
    rb=nas_COMMON_search_rb(cx, msgreq->rab_id);

    if (rb!=NULL) {
      //msgrep->status=nas_rg_DC_send_rb_release_request(cx, rb);
    } else
      msgrep->status=-NAS_ERROR_NOTCONNECTED;

    //      msgrep->cnxid  = msgreq->cnxid;
  } else
    msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
}

//---------------------------------------------------------------------------
int nas_ioCTL_rb_release_request(
  struct nas_ioctl *gifr,
  struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct nas_msg_rb_release_request msgreq;
  struct nas_msg_rb_release_reply msgrep;
  printk("NAS_IOCTL_RB_RELEASE: Radio bearer release requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("NAS_IOCTL_RB_RELEASE: copy_from_user failure\n");
    return -EFAULT;
  }

  nas_set_msg_rb_release_reply(&msgrep, &msgreq, priv);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("NAS_IOCTL_RB_RELEASE: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Classifier List
//---------------------------------------------------------------------------
void nas_set_msg_class_list_reply(
  uint8_t *msgrep,
  struct nas_msg_class_list_request *msgreq,
  struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  struct classifier_entity *gc;
  struct nas_msg_class_list_reply *list;
  uint8_t cli;
  list=(struct nas_msg_class_list_reply *)(msgrep+1);

  switch(msgreq->dir) {
  case NAS_DIRECTION_SEND:
    cx=nas_COMMON_search_cx(msgreq->lcr,priv);

    if (cx==NULL) {
      msgrep[0]=0;
      return;
    }

    gc=cx->sclassifier[msgreq->dscp];
    break;

  case NAS_DIRECTION_RECEIVE:
    cx=NULL;
    gc=priv->rclassifier[msgreq->dscp];
    break;

  default:
    cx=NULL;
    msgrep[0]=0;
    return;
  }

  for (cli=0; (gc!=NULL)&&(cli<NAS_LIST_CLASS_MAX); gc=gc->next, ++cli) {
    list[cli].classref=gc->classref;
    list[cli].lcr=msgreq->lcr;
    list[cli].dir=msgreq->dir;
    list[cli].dscp=msgreq->dscp;
    list[cli].rab_id=gc->rab_id;
    list[cli].version=gc->version;

    switch(gc->version) {
    case 4:
      list[cli].saddr.ipv4 = gc->saddr.ipv4;
      list[cli].daddr.ipv4 = gc->daddr.ipv4;
      break;

    case 6:
      list[cli].saddr.ipv6 = gc->saddr.ipv6;
      list[cli].daddr.ipv6 = gc->daddr.ipv6;
      break;
    }

    list[cli].protocol=gc->protocol;
    list[cli].sport=ntohs(gc->sport);
    list[cli].dport=ntohs(gc->dport);
    list[cli].splen=gc->splen;
    list[cli].dplen=gc->dplen;
    list[cli].fct=nas_TOOL_invfct(gc);
  }

  msgrep[0]=cli;
}

//---------------------------------------------------------------------------
int nas_ioCTL_class_list_request(
  struct nas_ioctl *gifr,
  struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  uint8_t msgrep[NAS_LIST_CLASS_MAX*sizeof(struct nas_msg_class_list_reply)+1];
  struct nas_msg_class_list_request msgreq;
  printk("NAS_IOCTL_CLASS_LIST: classifier list requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("NAS_IOCTL_CLASS_LIST: copy_from_user failure\n");
    return -EFAULT;
  }

  nas_set_msg_class_list_reply(msgrep, &msgreq,priv);

  if (copy_to_user(gifr->msg, msgrep, NAS_LIST_CLASS_MAX*sizeof(struct nas_msg_class_list_reply)+1)) {
    printk("NAS_IOCTL_CLASS_LIST: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Request the addition of a classifier rule
//---------------------------------------------------------------------------
void nas_set_msg_class_add_reply(
  struct nas_msg_class_add_reply   *msgrep,
  struct nas_msg_class_add_request *msgreq,
  struct nas_priv                  *priv)
{
  //---------------------------------------------------------------------------
  struct classifier_entity *gc,*gc2;
  unsigned char *saddr,*daddr;
  unsigned int *saddr32,*daddr32;

  printk("[NAS][CLASS] nas_set_msg_class_add_reply\n");


  if (msgreq->dscp>NAS_DSCP_MAX) {
    printk("NAS_SET_MSG_CLASS_ADD_REPLY: Incoherent parameter value\n");
    msgrep->status=-NAS_ERROR_NOTCORRECTDSCP;
    return;
  }

  if (msgreq->dir==NAS_DIRECTION_SEND) {


    struct cx_entity *cx;
    cx=nas_COMMON_search_cx(msgreq->lcr,priv);

    if (cx!=NULL) {
      printk("NAS_SET_MSG_CLASS_ADD_REPLY: DSCP/EXP %d, Classref %d, RB %u\n", msgreq->dscp, msgreq->classref,msgreq->rab_id );
      gc=nas_CLASS_add_sclassifier(cx, msgreq->dscp, msgreq->classref);

      printk("NAS_SET_MSG_CLASS_ADD_REPLY: %p %p\n" , msgreq, gc);

      if (gc==NULL) {
        msgrep->status=-NAS_ERROR_NOMEMORY;
        return;
      }
    } else {
      msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
      return;
    }

    gc->rab_id=msgreq->rab_id;

    gc->rb=nas_COMMON_search_rb(cx, gc->rab_id);
    printk("NAS_SET_MSG_CLASS_ADD_REPLY: gc_rb %p %u \n", gc->rb, gc->rab_id);
  } else {
    if (msgreq->dir==NAS_DIRECTION_RECEIVE) {
      gc=nas_CLASS_add_rclassifier(msgreq->dscp,
                                   msgreq->classref,
                                   priv);

      if (gc==NULL) {
        msgrep->status=-NAS_ERROR_NOMEMORY;
        return;
      }

      gc->rab_id=msgreq->rab_id;

    } else {
      msgrep->status=-NAS_ERROR_NOTCORRECTDIR;
      return;
    }

    for (gc2 = priv->rclassifier[msgreq->dscp]; gc2!=NULL ; gc2 = gc2->next)
      printk("[NAS][CLASS] Add Receive Classifier dscp %d: rab_id %d (%p,next %p)\n",msgreq->dscp,gc2->rab_id,gc2,gc2->next);
  }

  printk("[NAS][CLASS] Getting addresses ...\n");

  nas_TOOL_fct(gc, msgreq->fct);
  gc->version=msgreq->version;

  switch(gc->version) {
  case 4:
    gc->saddr.ipv4=msgreq->saddr.ipv4;
    gc->daddr.ipv4=msgreq->daddr.ipv4;



    // #ifdef NAS_CLASS_DEBUG
    saddr = (unsigned char *)&gc->saddr.ipv4;
    daddr = (unsigned char *)&gc->daddr.ipv4;

    printk("[NAS][CLASS] Adding IPv4 %d.%d.%d.%d -> %d.%d.%d.%d\n",
           saddr[0],saddr[1],saddr[2],saddr[3],
           daddr[0],daddr[1],daddr[2],daddr[3]);



    //#endif
    gc->splen=msgreq->splen;
    gc->dplen=msgreq->dplen;
    break;

  case 6:
    memcpy(&gc->saddr.ipv6,&msgreq->saddr.ipv6,16);
    memcpy(&gc->daddr.ipv6,&msgreq->daddr.ipv6,16);

    saddr32 = (unsigned int *)&gc->saddr.ipv6;
    daddr32 = (unsigned int *)&gc->daddr.ipv6;

    printk("[NAS][CLASS] Adding IPv6 %X:%X:%X:%X -> %X.%X.%X.%X\n",
           saddr32[0],saddr32[1],saddr32[2],saddr32[3],
           daddr32[0],daddr32[1],daddr32[2],daddr32[3]);
    gc->splen=msgreq->splen;
    gc->dplen=msgreq->dplen;
    break;

  case NAS_MPLS_VERSION_CODE:

    printk("[NAS][CLASS] Adding MPLS label %d with exp %d\n",
           msgreq->daddr.mpls_label,msgreq->dscp);
    gc->daddr.mpls_label = msgreq->daddr.mpls_label;

    break;

  case 0:
    gc->saddr.ipv6.s6_addr32[0]=0;
    gc->daddr.ipv6.s6_addr32[1]=0;
    gc->saddr.ipv6.s6_addr32[2]=0;
    gc->daddr.ipv6.s6_addr32[3]=0;
    gc->splen=0;
    gc->dplen=0;
    break;

  default:
    msgrep->status=-NAS_ERROR_NOTCORRECTVERSION;
    kfree(gc);
    return;
  }

  gc->protocol=msgreq->protocol;
  gc->protocol_message_type=msgreq->protocol_message_type;
  gc->sport=htons(msgreq->sport);
  gc->dport=htons(msgreq->dport);
  msgrep->status=0;
}

//---------------------------------------------------------------------------
int nas_ioCTL_class_add_request(struct nas_ioctl *gifr,
                                struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct nas_msg_class_add_request msgreq;
  struct nas_msg_class_add_reply msgrep;


  printk("NAS_IOCTL_CLASS_ADD: Add classifier components requested\n");
  printk("NAS_IOCTL_CLASS_ADD: size of gifr msg %zd\n", sizeof(gifr->msg));


  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("NAS_IOCTL_CLASS_ADD: copy_from_user failure\n");
    return -EFAULT;
  }

  nas_set_msg_class_add_reply(&msgrep, &msgreq,priv);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("NAS_IOCTL_CLASS_ADD: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Request the deletion of a classifier rule
//---------------------------------------------------------------------------
void nas_set_msg_class_del_reply(struct nas_msg_class_del_reply *msgrep,
                                 struct nas_msg_class_del_request *msgreq,
                                 struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  if (msgreq->dscp>NAS_DSCP_DEFAULT) {
    printk("NAS_SET_MSG_CLASS_DEL_REPLY: Incoherent parameter value\n");
    msgrep->status=-NAS_ERROR_NOTCORRECTDSCP;
    return;
  }

  if (msgreq->dir==NAS_DIRECTION_SEND) {
    struct cx_entity *cx;
    cx=nas_COMMON_search_cx(msgreq->lcr,priv);

    if (cx!=NULL)
      nas_CLASS_del_sclassifier(cx, msgreq->dscp, msgreq->classref);
    else {
      msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
      return;
    }
  } else {
    if (msgreq->dir==NAS_DIRECTION_RECEIVE)
      nas_CLASS_del_rclassifier(msgreq->dscp, msgreq->classref,priv);
    else {
      msgrep->status=-NAS_ERROR_NOTCORRECTDIR;
      return;
    }
  }

  msgrep->status=0;
}

//---------------------------------------------------------------------------
int nas_ioCTL_class_del_request(struct nas_ioctl *gifr,
                                struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct nas_msg_class_del_request msgreq;
  struct nas_msg_class_del_reply msgrep;
  printk("NAS_IOCTL_CLASS_DEL: Del classifier components requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("NAS_IOCTL_CLASS_DEL: copy_from_user failure\n");
    return -EFAULT;
  }

  nas_set_msg_class_del_reply(&msgrep, &msgreq,priv);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("NAS_IOCTL_CLASS_DEL: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Measurement
// Messages for Measurement transfer

//---------------------------------------------------------------------------
void nas_set_msg_measure_reply(struct nas_msg_measure_reply *msgrep, struct nas_msg_measure_request *msgreq,
                               struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  int lcr=0; // Temp lcr->mt =0
  int i;

  cx=nas_COMMON_search_cx(lcr,priv);

  if (cx!=NULL) {
    msgrep->num_cells = cx->num_measures;

    for (i=0; i<cx->num_measures; i++) {
      msgrep-> measures[i].cell_id = cx->meas_cell_id[i];
      msgrep-> measures[i].level = cx->meas_level[i];
      msgrep-> measures[i].provider_id = cx->provider_id[i];
    }

    msgrep->signal_lost_flag = 0;
  }
}
//---------------------------------------------------------------------------
int nas_ioCTL_measure_request(struct nas_ioctl *gifr,
                              struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct nas_msg_measure_request msgreq;
  struct nas_msg_measure_reply msgrep;
  printk("NAS_IOCTL_MEASURE: Measurement requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("NAS_IOCTL_MEASURE: copy_from_user failure\n");
    return -EFAULT;
  }

  nas_set_msg_measure_reply(&msgrep, &msgreq,priv);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("NAS_IOCTL_MEASURE: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// IMEI
// Messages for IMEI transfer
//---------------------------------------------------------------------------
void nas_set_msg_imei_reply(struct nas_msg_l2id_reply *msgrep,
                            struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  int lcr=0; // Temp lcr->mt =0

  cx=nas_COMMON_search_cx(lcr,priv);

  if (cx!=NULL) {
    msgrep->l2id[0] = cx->iid6[0];
    msgrep->l2id[1] = cx->iid6[1];
  }
}
//---------------------------------------------------------------------------
int nas_ioCTL_imei_request(struct nas_ioctl *gifr,
                           struct nas_priv *priv)
{
  //---------------------------------------------------------------------------
  struct nas_msg_l2id_reply msgrep;
  printk("NAS_IOCTL_IMEI: IMEI requested\n");
  nas_set_msg_imei_reply(&msgrep,priv);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("NAS_IOCTL_IMEI: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IOCTL command
//---------------------------------------------------------------------------
int nas_CTL_ioctl(struct net_device *dev,
                  struct ifreq *ifr,
                  int cmd)
{
  //---------------------------------------------------------------------------
  struct nas_ioctl *gifr;
  struct nas_priv *priv=netdev_priv(dev);

  int r;

  //  printk("NAS_CTL_IOCTL: begin ioctl for instance %d\n",find_inst(dev));

  switch(cmd) {
  case NAS_IOCTL_RRM:
    gifr=(struct nas_ioctl *)ifr;

    switch(gifr->type) {
    case NAS_MSG_STATISTIC_REQUEST:
      r=nas_ioCTL_statistic_request(gifr,priv);
      break;

    case NAS_MSG_CX_ESTABLISHMENT_REQUEST:
      r=nas_ioCTL_cx_establishment_request(gifr,priv);
      break;

    case NAS_MSG_CX_RELEASE_REQUEST:
      r=nas_ioCTL_cx_release_request(gifr,priv);
      break;

    case NAS_MSG_CX_LIST_REQUEST:
      r=nas_ioCTL_cx_list_request(gifr,priv);
      break;

    case NAS_MSG_RB_ESTABLISHMENT_REQUEST:
      r=nas_ioCTL_rb_establishment_request(gifr,priv);
      break;

    case NAS_MSG_RB_RELEASE_REQUEST:
      r= nas_ioCTL_rb_release_request(gifr,priv);
      break;

    case NAS_MSG_RB_LIST_REQUEST:
      r=nas_ioCTL_rb_list_request(gifr,priv);
      break;

    case NAS_MSG_CLASS_ADD_REQUEST:
      r=nas_ioCTL_class_add_request(gifr,priv);
      break;

    case NAS_MSG_CLASS_LIST_REQUEST:
      r=nas_ioCTL_class_list_request(gifr,priv);
      break;

    case NAS_MSG_CLASS_DEL_REQUEST:
      r=nas_ioCTL_class_del_request(gifr,priv);
      break;

    case NAS_MSG_MEAS_REQUEST:
      r=nas_ioCTL_measure_request(gifr,priv);
      break;

    case NAS_MSG_IMEI_REQUEST:
      r=nas_ioCTL_imei_request(gifr,priv);
      break;

    default:
      //  printk("NAS_IOCTL_RRM: unkwon request type, type=%x\n", gifr->type);
      r=-EFAULT;
    }

    break;

  default:
    //      printk("NAS_CTL_IOCTL: Unknown ioctl command, cmd=%x\n", cmd);
    r=-EFAULT;
  }

  //  printk("NAS_CTL_IOCTL: end\n");
  return r;
}

//---------------------------------------------------------------------------
void nas_CTL_send(struct sk_buff *skb,
                  struct cx_entity *cx,
                  struct classifier_entity *gc, int inst, struct nas_priv *gpriv)
{
  //---------------------------------------------------------------------------
  printk("NAS_CTL_SEND - void \n");
}

