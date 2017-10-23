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

/*! \file nasmt_iocontrol.c
* \brief I/O control functions for upper layers of driver for OpenAirInterface CELLULAR version - MT
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#include "nasmt_variables.h"
#include "nasmt_iocontrol.h"
#include "nasmt_proto.h"

//#include <linux/in.h>
#include <asm/uaccess.h>
#include <asm/checksum.h>
#include <asm/uaccess.h>

extern int  nasmt_ASCTL_DC_send_cx_establish_request(struct cx_entity *cx);

// Statistic
//---------------------------------------------------------------------------
void nasmt_set_msg_statistic_reply(struct nas_msg_statistic_reply *msgrep)
{
  //---------------------------------------------------------------------------
  msgrep->rx_packets=gpriv->stats.rx_packets;
  msgrep->tx_packets=gpriv->stats.tx_packets;
  msgrep->rx_bytes=gpriv->stats.rx_bytes;
  msgrep->tx_bytes=gpriv->stats.tx_bytes;
  msgrep->rx_errors=gpriv->stats.rx_errors;
  msgrep->tx_errors=gpriv->stats.tx_errors;
  msgrep->rx_dropped=gpriv->stats.rx_dropped;
  msgrep->tx_dropped=gpriv->stats.tx_dropped;
}

//---------------------------------------------------------------------------
int nasmt_ioCTL_statistic_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_statistic_reply msgrep;
  printk("nasmt_ioCTL_statistic: stat requested\n");
  nasmt_set_msg_statistic_reply(&msgrep);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasmt_ioCTL_statistic: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Connections List
//---------------------------------------------------------------------------
void nasmt_set_msg_cx_list_reply(uint8_t *msgrep)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  nasLocalConnectionRef_t lcr;
  struct nas_msg_cx_list_reply *list;
  msgrep[0]=NAS_CX_MAX;
  list=(struct nas_msg_cx_list_reply *)(msgrep+1);

  for(lcr=0; lcr<NAS_CX_MAX; ++lcr) {
    cx=nasmt_COMMON_search_cx(lcr);
    list[lcr].lcr=lcr;
    list[lcr].state=cx->state;
    list[lcr].cellid=cx->cellid;
    list[lcr].iid4=cx->iid4;
    list[lcr].iid6[0]=cx->iid6[0];
    list[lcr].iid6[1]=cx->iid6[1];
    list[lcr].num_rb=cx->num_rb;
    list[lcr].nsclassifier=cx->nsclassifier;
    printk("nasmt_set_msg_cx_list_reply: nsc=%u\n",cx->nsclassifier);
  }
}

//---------------------------------------------------------------------------
int nasmt_ioCTL_cx_list_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  uint8_t msgrep[NAS_CX_MAX*sizeof(struct nas_msg_cx_list_reply)+1];
  printk("nasmt_ioCTL_cx_list: connection list requested\n");
  nasmt_set_msg_cx_list_reply(msgrep);

  if (copy_to_user(gifr->msg, msgrep, NAS_CX_MAX*sizeof(struct nas_msg_cx_list_reply)+1)) {
    printk("nasmt_ioCTL_cx_list: copy_to_user failure\n");
    return -EFAULT;
  }

  printk("nasmt_ioCTL_cx_list: end\n");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Connection Establishment
//---------------------------------------------------------------------------
void nasmt_set_msg_cx_establishment_reply(struct nas_msg_cx_establishment_reply *msgrep, struct nas_msg_cx_establishment_request *msgreq)
{
  //---------------------------------------------------------------------------
#ifdef NODE_RG
  msgrep->status=-NAS_ERROR_NOTMT;
#else
  struct cx_entity *cx;
  cx=nasmt_COMMON_search_cx(msgreq->lcr);

  if (cx!=NULL) {
    if (cx->state == NAS_CX_RELEASING) {
      msgrep->status=nasmt_ASCTL_leave_sleep_mode(cx);
    } else {
      cx->cellid=msgreq->cellid;
      msgrep->status=nasmt_ASCTL_DC_send_cx_establish_request(cx);
    }
  } else
    msgrep->status=-NAS_ERROR_NOTCORRECTLCR;

#endif
}
//---------------------------------------------------------------------------
int nasmt_ioCTL_cx_establishment_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_cx_establishment_request msgreq;
  struct nas_msg_cx_establishment_reply msgrep;
  printk("nasmt_ioCTL_cx_establishment: connection establishment requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasmt_ioCTL_cx_establishment: copy_from_user failure\n");
    return -EFAULT;
  }

  nasmt_set_msg_cx_establishment_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasmt_ioCTL_cx_establishment: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Connection Release
//---------------------------------------------------------------------------
void nasmt_set_msg_cx_release_reply(struct nas_msg_cx_release_reply *msgrep, struct nas_msg_cx_release_request *msgreq)
{
  //---------------------------------------------------------------------------
#ifdef NODE_RG
  msgrep->status=-NAS_ERROR_NOTMT;
#else
  struct cx_entity *cx;
  cx=nasmt_COMMON_search_cx(msgreq->lcr);

  if (cx!=NULL) {
#ifdef ENABLE_SLEEP_MODE
    msgrep->status=nasmt_ASCTL_enter_sleep_mode(cx);
#endif
#ifndef ENABLE_SLEEP_MODE
    msgrep->status=nasmt_ASCTL_DC_send_cx_release_request(cx);
#endif
  } else
    msgrep->status=-NAS_ERROR_NOTCORRECTLCR;

#endif
}

//---------------------------------------------------------------------------
// Request the release of a connection
int nasmt_ioCTL_cx_release_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_cx_release_request msgreq;
  struct nas_msg_cx_release_reply msgrep;

  printk("nasmt_ioCTL_cx_release: connection release requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasmt_ioCTL_cx_release: copy_from_user failure\n");
    return -EFAULT;
  }

  nasmt_set_msg_cx_release_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasmt_ioCTL_cx_release: copy_to_user failure\n");
    return -EFAULT;
  }

  printk("nasmt_ioCTL_cx_release: end\n");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Radio Bearer List
//---------------------------------------------------------------------------
void nasmt_set_msg_rb_list_reply(uint8_t *msgrep, struct nas_msg_rb_list_request *msgreq)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  cx=nasmt_COMMON_search_cx(msgreq->lcr);

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
int nasmt_ioCTL_rb_list_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  uint8_t msgrep[NAS_LIST_RB_MAX*sizeof(struct nas_msg_rb_list_reply)+1];
  struct nas_msg_rb_list_request msgreq;
  printk("nasmt_ioCTL_rb_list: Radio Bearer list requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasmt_ioCTL_rb_list: copy_from_user failure\n");
    return -EFAULT;
  }

  nasmt_set_msg_rb_list_reply(msgrep, &msgreq);

  if (copy_to_user(gifr->msg, msgrep, NAS_LIST_RB_MAX*sizeof(struct nas_msg_rb_list_reply)+1)) {
    printk("nasmt_ioCTL_rb_list: copy_to_user failure\n");
    return -EFAULT;
  }

  printk("nasmt_ioCTL_rb_list: end\n");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Radio Bearer Establishment
//---------------------------------------------------------------------------
void nasmt_set_msg_rb_establishment_reply(struct nas_msg_rb_establishment_reply *msgrep, struct nas_msg_rb_establishment_request *msgreq)
{
  //---------------------------------------------------------------------------
  msgrep->status=-NAS_ERROR_NOTRG;
}

//---------------------------------------------------------------------------
int nasmt_ioCTL_rb_establishment_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_rb_establishment_request msgreq;
  struct nas_msg_rb_establishment_reply msgrep;
  printk("nasmt_ioCTL_rb_establishment: Radio bearer establishment requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasmt_ioCTL_rb_establishment: copy_from_user failure\n");
    return -EFAULT;
  }

  nasmt_set_msg_rb_establishment_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasmt_ioCTL_rb_establishment: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Radio Bearer Release
//---------------------------------------------------------------------------
void nasmt_set_msg_rb_release_reply(struct nas_msg_rb_release_reply *msgrep, struct nas_msg_rb_release_request *msgreq)
{
  //---------------------------------------------------------------------------
  msgrep->status=-NAS_ERROR_NOTRG;
}

//---------------------------------------------------------------------------
int nasmt_ioCTL_rb_release_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_rb_release_request msgreq;
  struct nas_msg_rb_release_reply msgrep;
  printk("nasmt_ioCTL_rb_release: Radio bearer release requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasmt_ioCTL_rb_release: copy_from_user failure\n");
    return -EFAULT;
  }

  nasmt_set_msg_rb_release_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasmt_ioCTL_rb_release: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Classifier List
//---------------------------------------------------------------------------
void nasmt_set_msg_class_list_reply(uint8_t *msgrep, struct nas_msg_class_list_request *msgreq)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  struct classifier_entity *gc;
  struct nas_msg_class_list_reply *list;
  uint8_t cli;
  list=(struct nas_msg_class_list_reply *)(msgrep+1);

  switch(msgreq->dir) {
  case NAS_DIRECTION_SEND:
    cx=nasmt_COMMON_search_cx(msgreq->lcr);

    if (cx==NULL) {
      msgrep[0]=0;
      return;
    }

    gc=cx->sclassifier[msgreq->dscp];
    break;

  case NAS_DIRECTION_RECEIVE:
    cx=NULL;
    gc=gpriv->rclassifier[msgreq->dscp];
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
    list[cli].fct=nasmt_TOOL_invfct(gc);
  }

  msgrep[0]=cli;
}

//---------------------------------------------------------------------------
int nasmt_ioCTL_class_list_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  uint8_t msgrep[NAS_LIST_CLASS_MAX*sizeof(struct nas_msg_class_list_reply)+1];
  struct nas_msg_class_list_request msgreq;
  printk("nasmt_ioCTL_class_list: classifier list requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasmt_ioCTL_class_list: copy_from_user failure\n");
    return -EFAULT;
  }

  nasmt_set_msg_class_list_reply(msgrep, &msgreq);

  if (copy_to_user(gifr->msg, msgrep, NAS_LIST_CLASS_MAX*sizeof(struct nas_msg_class_list_reply)+1)) {
    printk("nasmt_ioCTL_class_list: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Request the addition of a classifier rule
//---------------------------------------------------------------------------
void nasmt_set_msg_class_add_reply(struct nas_msg_class_add_reply *msgrep, struct nas_msg_class_add_request *msgreq)
{
  //---------------------------------------------------------------------------
  struct classifier_entity *gc;

  if (msgreq->dscp>NAS_DSCP_DEFAULT) {
    printk("nasmt_set_msg_class_add_reply: Incoherent parameter value\n");
    msgrep->status=-NAS_ERROR_NOTCORRECTDSCP;
    return;
  }

  if (msgreq->dir==NAS_DIRECTION_SEND) {
    struct cx_entity *cx;
    cx=nasmt_COMMON_search_cx(msgreq->lcr);

    if (cx!=NULL) {
      printk("nasmt_set_msg_class_add_reply: DSCP %d, Classref %d\n",msgreq->dscp, msgreq->classref );
      gc=nasmt_CLASS_add_sclassifier(cx, msgreq->dscp, msgreq->classref);
      printk("nasmt_set_msg_class_add_reply: %p %p\n" , msgreq, gc);

      if (gc==NULL) {
        msgrep->status=-NAS_ERROR_NOMEMORY;
        return;
      }
    } else {
      msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
      return;
    }

    gc->rab_id=msgreq->rab_id;
    gc->rb=nasmt_COMMON_search_rb(cx, gc->rab_id);
  } else {
    if (msgreq->dir==NAS_DIRECTION_RECEIVE) {
      gc=nasmt_CLASS_add_rclassifier(msgreq->dscp, msgreq->classref);

      if (gc==NULL) {
        msgrep->status=-NAS_ERROR_NOMEMORY;
        return;
      }
    } else {
      msgrep->status=-NAS_ERROR_NOTCORRECTDIR;
      return;
    }
  }

  nasmt_TOOL_fct(gc, msgreq->fct);
  gc->version=msgreq->version;

  switch(gc->version) {
  case 4:
    gc->saddr.ipv4=msgreq->saddr.ipv4;
    gc->daddr.ipv4=msgreq->daddr.ipv4;
    gc->splen=msgreq->splen;
    gc->dplen=msgreq->dplen;
    break;

  case 6:
    gc->saddr.ipv6=msgreq->saddr.ipv6;
    gc->daddr.ipv6=msgreq->daddr.ipv6;
    gc->splen=msgreq->splen;
    gc->dplen=msgreq->dplen;
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
  gc->sport=htons(msgreq->sport);
  gc->dport=htons(msgreq->dport);
  msgrep->status=0;
}

//---------------------------------------------------------------------------
int nasmt_ioCTL_class_add_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_class_add_request msgreq;
  struct nas_msg_class_add_reply msgrep;
  printk("nasmt_ioCTL_class_add: Add classifier components requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasmt_ioCTL_class_add: copy_from_user failure\n");
    return -EFAULT;
  }

  nasmt_set_msg_class_add_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasmt_ioCTL_class_add: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Request the deletion of a classifier rule
//---------------------------------------------------------------------------
void nasmt_set_msg_class_del_reply(struct nas_msg_class_del_reply *msgrep, struct nas_msg_class_del_request *msgreq)
{
  //---------------------------------------------------------------------------
  if (msgreq->dscp>NAS_DSCP_DEFAULT) {
    printk("nasmt_set_msg_class_del_reply: Incoherent parameter value\n");
    msgrep->status=-NAS_ERROR_NOTCORRECTDSCP;
    return;
  }

  if (msgreq->dir==NAS_DIRECTION_SEND) {
    struct cx_entity *cx;
    cx=nasmt_COMMON_search_cx(msgreq->lcr);

    if (cx!=NULL)
      nasmt_CLASS_del_sclassifier(cx, msgreq->dscp, msgreq->classref);
    else {
      msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
      return;
    }
  } else {
    if (msgreq->dir==NAS_DIRECTION_RECEIVE)
      nasmt_CLASS_del_rclassifier(msgreq->dscp, msgreq->classref);
    else {
      msgrep->status=-NAS_ERROR_NOTCORRECTDIR;
      return;
    }
  }

  msgrep->status=0;
}

//---------------------------------------------------------------------------
int nasmt_ioCTL_class_del_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_class_del_request msgreq;
  struct nas_msg_class_del_reply msgrep;
  printk("nasmt_ioCTL_class_del: Del classifier components requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasmt_ioCTL_class_del: copy_from_user failure\n");
    return -EFAULT;
  }

  nasmt_set_msg_class_del_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasmt_ioCTL_class_del: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Measurement
// Messages for Measurement transfer

//---------------------------------------------------------------------------
void nasmt_set_msg_measure_reply(struct nas_msg_measure_reply *msgrep, struct nas_msg_measure_request *msgreq)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  int lcr=0; // Temp lcr->mt =0
  int i;

  cx = nasmt_COMMON_search_cx(lcr);

  if (cx!=NULL) {
    msgrep->num_cells = cx->num_measures;

    for (i=0; i<cx->num_measures; i++) {
      msgrep-> measures[i].cell_id = cx->meas_cell_id[i];
      msgrep-> measures[i].level = cx->meas_level[i];
      msgrep-> measures[i].provider_id = cx->provider_id[i];
    }

    msgrep->signal_lost_flag = 0;
  } else {
    //    msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
    //    return;
  }
}
//---------------------------------------------------------------------------
int nasmt_ioCTL_measure_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_measure_request msgreq;
  struct nas_msg_measure_reply msgrep;
  printk("nasmt_ioCTL_measure: Measurement requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasmt_ioCTL_measure: copy_from_user failure\n");
    return -EFAULT;
  }

  nasmt_set_msg_measure_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasmt_ioCTL_measure: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// IMEI
// Messages for IMEI transfer
//---------------------------------------------------------------------------
void nasmt_set_msg_imei_reply(struct nas_msg_l2id_reply *msgrep)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  int lcr=0; // Temp lcr->mt =0
  int i;

  cx=nasmt_COMMON_search_cx(lcr);

  if (cx!=NULL) {
    msgrep->l2id[0] = cx->iid6[0];
    msgrep->l2id[1] = cx->iid6[1];
  } else {
    //    msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
    //    return;
  }
}
//---------------------------------------------------------------------------
int nasmt_ioCTL_imei_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_l2id_reply msgrep;
  printk("nasmt_ioCTL_imei: IMEI requested\n");
  nasmt_set_msg_imei_reply(&msgrep);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasmt_ioCTL_imei: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IOCTL command
//---------------------------------------------------------------------------
int nasmt_CTL_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
  //---------------------------------------------------------------------------
  struct nas_ioctl *gifr;
  int r;
  printk("nasmt_CTL_ioctl: begin\n");

  //spin_lock(&gpriv->lock);
  switch(cmd) {
  case NASMT_IOCTL_RAL:
    gifr=(struct nas_ioctl *)ifr;

    switch(gifr->type) {
    case NAS_MSG_STATISTIC_REQUEST:
      r=nasmt_ioCTL_statistic_request(gifr);
      break;

    case NAS_MSG_CX_ESTABLISHMENT_REQUEST:
      r=nasmt_ioCTL_cx_establishment_request(gifr);
      break;

    case NAS_MSG_CX_RELEASE_REQUEST:
      r=nasmt_ioCTL_cx_release_request(gifr);
      break;

    case NAS_MSG_CX_LIST_REQUEST:
      r=nasmt_ioCTL_cx_list_request(gifr);
      break;

    case NAS_MSG_RB_ESTABLISHMENT_REQUEST:
      r=nasmt_ioCTL_rb_establishment_request(gifr);
      break;

    case NAS_MSG_RB_RELEASE_REQUEST:
      r= nasmt_ioCTL_rb_release_request(gifr);
      break;

    case NAS_MSG_RB_LIST_REQUEST:
      r=nasmt_ioCTL_rb_list_request(gifr);
      break;

    case NAS_MSG_CLASS_ADD_REQUEST:
      r=nasmt_ioCTL_class_add_request(gifr);
      break;

    case NAS_MSG_CLASS_LIST_REQUEST:
      r=nasmt_ioCTL_class_list_request(gifr);
      break;

    case NAS_MSG_CLASS_DEL_REQUEST:
      r=nasmt_ioCTL_class_del_request(gifr);
      break;

    case NAS_MSG_MEAS_REQUEST:
      r=nasmt_ioCTL_measure_request(gifr);
      break;

    case NAS_MSG_IMEI_REQUEST:
      r=nasmt_ioCTL_imei_request(gifr);
      break;

    default:
      printk("nasmt_CTL_ioctl: unkwon request type, type=%x\n", gifr->type);
      r=-EFAULT;
    }

    break;

  default:
    printk("nasmt_CTL_ioctl: Unknown ioctl command, cmd=%x\n", cmd);
    r=-EFAULT;
  }

  //spin_unlock(&gpriv->lock);
  printk("nasmt_CTL_ioctl: end\n");
  return r;
}

//---------------------------------------------------------------------------
void nasmt_CTL_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc)
{
  //---------------------------------------------------------------------------
  printk("nasmt_CTL_send - void \n");
}

