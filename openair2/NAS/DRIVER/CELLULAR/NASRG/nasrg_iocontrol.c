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

/*! \file nasrg_iocontrol.c
* \brief I/O control functions for upper layers of driver for OpenAirInterface CELLULAR version - RG
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#include "nasrg_variables.h"
#include "nasrg_iocontrol.h"
#include "nasrg_proto.h"

//#include <linux/in.h>
#include <asm/uaccess.h>
#include <asm/checksum.h>
#include <asm/uaccess.h>

// Statistic
///////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
void nasrg_set_msg_statistic_reply(struct nas_msg_statistic_reply *msgrep)
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
int nasrg_ioCTL_statistic_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_statistic_reply msgrep;
  printk("nasrg_ioCTL_statistic: stat requested\n");
  nasrg_set_msg_statistic_reply(&msgrep);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_ioCTL_statistic: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}


///////////////////////////////////////////////////////////////////////////////
// Connections List
//---------------------------------------------------------------------------
void nasrg_set_msg_cx_list_reply(uint8_t *msgrep)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  nasLocalConnectionRef_t lcr;
  struct nas_msg_cx_list_reply *list;
  msgrep[0]=NAS_CX_MAX;
  list=(struct nas_msg_cx_list_reply *)(msgrep+1);

  for(lcr=0; lcr<NAS_CX_MAX; ++lcr) {
    cx=nasrg_COMMON_search_cx(lcr);
    list[lcr].lcr=lcr;
    list[lcr].state=cx->state;
    list[lcr].cellid=cx->cellid;
    list[lcr].iid4=cx->iid4;
    list[lcr].iid6[0]=cx->iid6[0];
    list[lcr].iid6[1]=cx->iid6[1];
    list[lcr].num_rb=cx->num_rb;
    list[lcr].nsclassifier=cx->nsclassifier;
    printk("nasrg_set_msg_cx_list: nsc=%u\n",cx->nsclassifier);
  }
}

//---------------------------------------------------------------------------
int nasrg_ioCTL_cx_list_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  uint8_t msgrep[NAS_CX_MAX*sizeof(struct nas_msg_cx_list_reply)+1];
  printk("nasrg_ioCTL_cx_list: connection list requested\n");
  nasrg_set_msg_cx_list_reply(msgrep);

  if (copy_to_user(gifr->msg, msgrep, NAS_CX_MAX*sizeof(struct nas_msg_cx_list_reply)+1)) {
    printk("nasrg_ioCTL_cx_list: copy_to_user failure\n");
    return -EFAULT;
  }

  printk("nasrg_ioCTL_cx_list: end\n");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Connection Establishment
//---------------------------------------------------------------------------
void nasrg_set_msg_cx_establishment_reply(struct nas_msg_cx_establishment_reply *msgrep, struct nas_msg_cx_establishment_request *msgreq)
{
  //---------------------------------------------------------------------------
  msgrep->status=-NAS_ERROR_NOTMT;
}
//---------------------------------------------------------------------------
int nasrg_ioCTL_cx_establishment_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_cx_establishment_request msgreq;
  struct nas_msg_cx_establishment_reply msgrep;
  printk("nasrg_ioCTL_cx_establishment: connection establishment requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_ioCTL_cx_establishment: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_msg_cx_establishment_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_ioCTL_cx_establishment: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Connection Release
//---------------------------------------------------------------------------
void nasrg_set_msg_cx_release_reply(struct nas_msg_cx_release_reply *msgrep, struct nas_msg_cx_release_request *msgreq)
{
  //---------------------------------------------------------------------------
  msgrep->status=-NAS_ERROR_NOTMT;
}

//---------------------------------------------------------------------------
// Request the release of a connection
int nasrg_ioCTL_cx_release_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_cx_release_request msgreq;
  struct nas_msg_cx_release_reply msgrep;

  printk("nasrg_set_msg_cx_release: connection release requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_set_msg_cx_release: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_msg_cx_release_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_set_msg_cx_release: copy_to_user failure\n");
    return -EFAULT;
  }

  printk("nasrg_set_msg_cx_release: end\n");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Radio Bearer List
//---------------------------------------------------------------------------
void nasrg_set_msg_rb_list_reply(uint8_t *msgrep, struct nas_msg_rb_list_request *msgreq)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  cx=nasrg_COMMON_search_cx(msgreq->lcr);

  if (cx!=NULL) {
    uint8_t rbi;
    struct rb_entity *rb;
    struct nas_msg_rb_list_reply *list;

    if (cx->num_rb > NAS_LIST_RB_MAX)
      msgrep[0] = NAS_LIST_RB_MAX;
    else
      msgrep[0] = cx->num_rb;

    list=(struct nas_msg_rb_list_reply *)(msgrep+1);

    // list all radio bearers
    for (rb=cx->rb, rbi=0; (rb!=NULL)&&(rbi<msgrep[0]); rb=rb->next, ++rbi) {
      list[rbi].state=rb->state;
      list[rbi].rab_id=rb->rab_id;
      list[rbi].sapi=rb->sapi;
      list[rbi].qos=rb->qos;
      list[rbi].cnxid=rb->cnxid;
    }

    // check if mt0 requested and multicast bearer started, then add it to the list
    if ((msgreq->lcr==0)&&(gpriv->mbms_rb[0].mbms_rbId!=0)) {
      printk("MT %d, MBMS bearer rb_id %d\n", msgreq->lcr, gpriv->mbms_rb[0].mbms_rbId);
      //++rbi;
      ++msgrep[0];
      list[rbi].state=gpriv->mbms_rb[0].state;
      list[rbi].rab_id=gpriv->mbms_rb[0].mbms_rbId;
      list[rbi].sapi=gpriv->mbms_rb[0].sapi;
      list[rbi].qos=gpriv->mbms_rb[0].qos;
      list[rbi].cnxid=gpriv->mbms_rb[0].cnxid;
      printk("rab_id  cnxid  Sapi  QoS  State\n");
      printk("%u  %u  %u  %u  ", list[rbi].rab_id,list[rbi].cnxid, list[rbi].sapi, list[rbi].qos);
      nasrg_TOOL_print_state(list[rbi].state);
      printk("\n");
    }
  } else
    msgrep[0]=0;
}

//---------------------------------------------------------------------------
int nasrg_ioCTL_rb_list_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  uint8_t msgrep[NAS_LIST_RB_MAX*sizeof(struct nas_msg_rb_list_reply)+1];
  struct nas_msg_rb_list_request msgreq;
  printk("nasrg_ioCTL_rb_list: Radio Bearer list requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_ioCTL_rb_list: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_msg_rb_list_reply(msgrep, &msgreq);

  if (copy_to_user(gifr->msg, msgrep, NAS_LIST_RB_MAX*sizeof(struct nas_msg_rb_list_reply)+1)) {
    printk("nasrg_ioCTL_rb_list: copy_to_user failure\n");
    return -EFAULT;
  }

  printk("nasrg_ioCTL_rb_list: end\n");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Radio Bearer Establishment
//---------------------------------------------------------------------------
void nasrg_set_msg_rb_establishment_reply(struct nas_msg_rb_establishment_reply *msgrep, struct nas_msg_rb_establishment_request *msgreq)
{
  //---------------------------------------------------------------------------
  if ((msgreq->rab_id<5)||(msgreq->rab_id>(NAS_RB_MAX_NUM-1)))
    msgrep->status=-NAS_ERROR_NOTCORRECTRABI;
  else {
    struct cx_entity *cx;
    cx=nasrg_COMMON_search_cx(msgreq->lcr);

    if (cx!=NULL) { //not multicast
      struct rb_entity *rb;
      rb=nasrg_COMMON_add_rb(cx, msgreq->rab_id, msgreq->qos);

      if (rb!=NULL) {
        rb->cnxid = msgreq->cnxid;
        rb->dscp = msgreq->dscp_dl;
        rb->dscp_ul = msgreq->dscp_ul;
        msgrep->status=nasrg_ASCTL_DC_send_rb_establish_request(cx, rb);
      } else {
        msgrep->status=-NAS_ERROR_NOMEMORY;
      }
    } else { //no MT found
      if (msgreq->mcast_flag) { //multicast
        int mbms_ix=0; // should allocate index based on Service_id /cnxid / MC IP address
        //
        gpriv->mbms_rb[mbms_ix].cnxid = msgreq->cnxid;
        gpriv->mbms_rb[mbms_ix].serviceId = msgreq->cnxid;
        gpriv->mbms_rb[mbms_ix].sessionId = NASRG_TEMP_MBMS_SESSION_ID; //Temp hard coded
        gpriv->mbms_rb[mbms_ix].mbms_rbId = msgreq->rab_id;
        gpriv->mbms_rb[mbms_ix].sapi = NAS_DC_INPUT_SAPI;
#ifdef NAS_DEBUG_MBMS_PROT
        gpriv->mbms_rb[mbms_ix].sapi = NAS_DRB_INPUT_SAPI; //Only one RT-FIFO is used
#endif
        gpriv->mbms_rb[mbms_ix].qos = msgreq->qos;
        gpriv->mbms_rb[mbms_ix].dscp = msgreq->dscp_dl;
        gpriv->mbms_rb[mbms_ix].duration = NASRG_TEMP_MBMS_DURATION; //Temp hard coded
        memcpy ((char *)&(gpriv->mbms_rb[mbms_ix].mcast_address),(char *)&(msgreq->mcast_group), 16);
        msgrep->status=nasrg_ASCTL_GC_send_mbms_bearer_establish_req(mbms_ix);
      } else {
        msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
      }

      msgrep->cnxid  = msgreq->cnxid;
    }
  }
}

//---------------------------------------------------------------------------
int nasrg_ioCTL_rb_establishment_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_rb_establishment_request msgreq;
  struct nas_msg_rb_establishment_reply msgrep;
  printk("nasrg_ioCTL_rb_establishment: Radio bearer establishment requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_ioCTL_rb_establishment: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_msg_rb_establishment_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_ioCTL_rb_establishment: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Radio Bearer Release
//---------------------------------------------------------------------------
void nasrg_set_msg_rb_release_reply(struct nas_msg_rb_release_reply *msgrep, struct nas_msg_rb_release_request *msgreq)
{
  //---------------------------------------------------------------------------
  if (msgreq->lcr<NAS_CX_MAX) {
    if (msgreq->rab_id!=NASRG_DEFAULTRAB_RBID) {
      struct rb_entity *rb;
      struct cx_entity *cx;
      cx=nasrg_COMMON_search_cx(msgreq->lcr);
      rb=nasrg_COMMON_search_rb(cx, msgreq->rab_id);

      if ((rb!=NULL)&&(cx!=NULL)) {
        uint8_t dscp;
        msgrep->status=nasrg_ASCTL_DC_send_rb_release_request(cx, rb);
        dscp=rb->dscp;
        nasrg_COMMON_del_rb(cx, msgreq->rab_id, dscp);
      } else
        msgrep->status=-NAS_ERROR_NOTCONNECTED;

      msgrep->cnxid  = msgreq->cnxid;
    } else {
      msgrep->status=-NAS_ERROR_NOTCORRECTRABI;
    }
  } else {
    if (msgreq->mcast_flag) { // multicast
      int mbms_ix=0;  // should search mbms_ix based on cnxid
      msgrep->status=nasrg_ASCTL_GC_send_mbms_bearer_release_req(mbms_ix);
      msgrep->cnxid  = msgreq->cnxid;
    } else {
      msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
    }
  }
}

//---------------------------------------------------------------------------
int nasrg_ioCTL_rb_release_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_rb_release_request msgreq;
  struct nas_msg_rb_release_reply msgrep;
  printk("nasrg_ioCTL_rb_release: Radio bearer release requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_ioCTL_rb_release: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_msg_rb_release_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_ioCTL_rb_release: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Classifier List
//---------------------------------------------------------------------------
void nasrg_set_msg_class_list_reply(uint8_t *msgrep, struct nas_msg_class_list_request *msgreq)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  struct classifier_entity *gc;
  struct nas_msg_class_list_reply *list;
  uint8_t cli;
  list=(struct nas_msg_class_list_reply *)(msgrep+1);

  switch(msgreq->dir) {
  case NAS_DIRECTION_SEND:
    cx=nasrg_COMMON_search_cx(msgreq->lcr);

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
    list[cli].fct=nasrg_TOOL_invfct(gc);
  }

  msgrep[0]=cli;
}

//---------------------------------------------------------------------------
int nasrg_ioCTL_class_list_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  uint8_t msgrep[NAS_LIST_CLASS_MAX*sizeof(struct nas_msg_class_list_reply)+1];
  struct nas_msg_class_list_request msgreq;
  printk("nasrg_ioCTL_class_list: classifier list requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_ioCTL_class_list: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_msg_class_list_reply(msgrep, &msgreq);

  if (copy_to_user(gifr->msg, msgrep, NAS_LIST_CLASS_MAX*sizeof(struct nas_msg_class_list_reply)+1)) {
    printk("nasrg_ioCTL_class_list: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Request the addition of a classifier rule
//---------------------------------------------------------------------------
void nasrg_set_msg_class_add_reply(struct nas_msg_class_add_reply *msgrep, struct nas_msg_class_add_request *msgreq)
{
  //---------------------------------------------------------------------------
  struct classifier_entity *gc;

  if (msgreq->dscp>NAS_DSCP_DEFAULT) {
    printk("nasrg_set_msg_class_add: Incoherent parameter value\n");
    msgrep->status=-NAS_ERROR_NOTCORRECTDSCP;
    return;
  }

  if (msgreq->dir==NAS_DIRECTION_SEND) {
    struct cx_entity *cx;
    cx=nasrg_COMMON_search_cx(msgreq->lcr);

    if (cx!=NULL) {
      printk("nasrg_set_msg_class_add: DSCP %d, Classref %d\n",msgreq->dscp, msgreq->classref );
      gc=nasrg_CLASS_add_sclassifier(cx, msgreq->dscp, msgreq->classref);
      printk("nasrg_set_msg_class_add: %p %p\n" , msgreq, gc);

      if (gc==NULL) {
        msgrep->status=-NAS_ERROR_NOMEMORY;
        return;
      }
    } else {
      msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
      return;
    }

    gc->rab_id=msgreq->rab_id;
    gc->rb=nasrg_COMMON_search_rb(cx, gc->rab_id);
  } else {
    if (msgreq->dir==NAS_DIRECTION_RECEIVE) {
      gc=nasrg_CLASS_add_rclassifier(msgreq->dscp, msgreq->classref);

      if (gc==NULL) {
        msgrep->status=-NAS_ERROR_NOMEMORY;
        return;
      }
    } else {
      msgrep->status=-NAS_ERROR_NOTCORRECTDIR;
      return;
    }
  }

  nasrg_TOOL_fct(gc, msgreq->fct);
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
int nasrg_ioCTL_class_add_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_class_add_request msgreq;
  struct nas_msg_class_add_reply msgrep;
  printk("nasrg_ioCTL_class_add: Add classifier components requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_ioCTL_class_add: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_msg_class_add_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_ioCTL_class_add: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Request the deletion of a classifier rule
//---------------------------------------------------------------------------
void nasrg_set_msg_class_del_reply(struct nas_msg_class_del_reply *msgrep, struct nas_msg_class_del_request *msgreq)
{
  //---------------------------------------------------------------------------
  if (msgreq->dscp>NAS_DSCP_DEFAULT) {
    printk("nasrg_set_msg_class_del: Incoherent parameter value\n");
    msgrep->status=-NAS_ERROR_NOTCORRECTDSCP;
    return;
  }

  if (msgreq->dir==NAS_DIRECTION_SEND) {
    struct cx_entity *cx;
    cx=nasrg_COMMON_search_cx(msgreq->lcr);

    if (cx!=NULL)
      nasrg_CLASS_del_sclassifier(cx, msgreq->dscp, msgreq->classref);
    else {
      msgrep->status=-NAS_ERROR_NOTCORRECTLCR;
      return;
    }
  } else {
    if (msgreq->dir==NAS_DIRECTION_RECEIVE)
      nasrg_CLASS_del_rclassifier(msgreq->dscp, msgreq->classref);
    else {
      msgrep->status=-NAS_ERROR_NOTCORRECTDIR;
      return;
    }
  }

  msgrep->status=0;
}

//---------------------------------------------------------------------------
int nasrg_ioCTL_class_del_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_class_del_request msgreq;
  struct nas_msg_class_del_reply msgrep;
  printk("nasrg_ioCTL_class_del: Del classifier components requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_ioCTL_class_del: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_msg_class_del_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_ioCTL_class_del: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// UE Multicast Join
//---------------------------------------------------------------------------
void nasrg_set_ue_multicast_join_reply(struct nas_msg_mt_mcast_reply *msgrep, struct nas_msg_mt_mcast_join *msgreq)
{
  //---------------------------------------------------------------------------
  if (msgreq->ue_id<NAS_CX_MAX) {
    struct cx_entity *cx;
    cx=nasrg_COMMON_search_cx(msgreq->ue_id);
    cx->requested_joined_services[0] = msgreq->cnxid;
    msgrep->ue_id  = msgreq->ue_id;
    msgrep->result = nasrg_ASCTL_DC_send_mbms_ue_notify_req(cx);
    msgrep->cnxid  = msgreq->cnxid;
  } else {
    msgrep->result=-NAS_ERROR_NOTCORRECTLCR;
  }
}

//---------------------------------------------------------------------------
int nasrg_ioCTL_ue_multicast_join_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_mt_mcast_join msgreq;
  struct nas_msg_mt_mcast_reply msgrep;
  printk("nasrg_ioCTL_ue_multicast_join: UE multicast join requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_ioCTL_ue_multicast_join: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_ue_multicast_join_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_ioCTL_ue_multicast_join: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// UE Multicast Leave
//---------------------------------------------------------------------------
void nasrg_set_ue_multicast_leave_reply(struct nas_msg_mt_mcast_reply *msgrep, struct nas_msg_mt_mcast_leave *msgreq)
{
  //---------------------------------------------------------------------------
  if (msgreq->ue_id<NAS_CX_MAX) {
    struct cx_entity *cx;
    cx=nasrg_COMMON_search_cx(msgreq->ue_id);
    cx->requested_left_services[0] = msgreq->cnxid;
    msgrep->ue_id  = msgreq->ue_id;
    msgrep->result = nasrg_ASCTL_DC_send_mbms_ue_notify_req(cx);
    msgrep->cnxid  = msgreq->cnxid;
  } else {
    msgrep->result=-NAS_ERROR_NOTCORRECTLCR;
  }
}

//---------------------------------------------------------------------------
int nasrg_ioCTL_ue_multicast_leave_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_mt_mcast_leave msgreq;
  struct nas_msg_mt_mcast_reply msgrep;
  printk("nasrg_ioCTL_ue_multicast_leave: UE multicast leave requested\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_ioCTL_ue_multicast_leave: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_ue_multicast_leave_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_ioCTL_ue_multicast_leave: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// ENB Measures
// Messages for triggering measurement
//---------------------------------------------------------------------------
void nasrg_set_msg_eNBmeasure_trigger_reply(struct nas_msg_enb_measure_trigger_reply *msgrep, struct nas_msg_enb_measure_trigger *msgreq)
{
  //---------------------------------------------------------------------------
  struct cx_entity *cx;
  int lcr=0; // Temp lcr->mt =0 (should be GC uplink)

  cx=nasrg_COMMON_search_cx(lcr);

  if (msgreq->cell_id != cx->cellid)
    printk("\nERROR : invalid cell_id received\n\n");

  if (nasrg_ASCTL_DC_send_eNBmeasurement_req(cx)>0)
    msgrep->status = 0;
  else
    msgrep->status = NAS_ERROR_NOTCORRECTVALUE ;
}
//---------------------------------------------------------------------------
int nasrg_ioCTL_eNBmeasure_trigger_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_enb_measure_trigger msgreq;
  struct nas_msg_enb_measure_trigger_reply msgrep;
  printk("nasrg_ioCTL_eNBmeasure_trigger_request: Measures triggered\n");

  if (copy_from_user(&msgreq, gifr->msg, sizeof(msgreq))) {
    printk("nasrg_ioCTL_eNBmeasure_trigger_request: copy_from_user failure\n");
    return -EFAULT;
  }

  nasrg_set_msg_eNBmeasure_trigger_reply(&msgrep, &msgreq);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_ioCTL_eNBmeasure_trigger_request: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// ENb Measurement
// Messages for Measurement retrieval
//---------------------------------------------------------------------------
void nasrg_set_msg_eNBmeasure_retrieve_reply(struct nas_msg_enb_measure_retrieve *msgrep)
{
  //---------------------------------------------------------------------------
  int i;

  msgrep->cell_id = gpriv->measured_cell_id;
  msgrep->num_UEs = gpriv->num_UEs;

  for (i=0; i<gpriv-> num_UEs; i++) {
    msgrep->measures[i].rlcBufferOccupancy = gpriv->rlcBufferOccupancy[i];
    msgrep->measures[i].scheduledPRB = gpriv->scheduledPRB[i];
    msgrep->measures[i].totalDataVolume = gpriv->totalDataVolume[i];
    //clean variables
    gpriv->rlcBufferOccupancy[i] = 0;
    gpriv->scheduledPRB[i] = 0;
    gpriv->totalDataVolume[i] = 0;
  }

  msgrep->totalNumPRBs = gpriv->totalNumPRBs;
  //clean variable
  gpriv->totalNumPRBs = 0;
}
//---------------------------------------------------------------------------
int nasrg_ioCTL_eNBmeasure_retrieve_request(struct nas_ioctl *gifr)
{
  //---------------------------------------------------------------------------
  struct nas_msg_enb_measure_retrieve msgrep;
  printk("nasrg_ioCTL_eNBmeasure_retrieve_request: Measurement requested\n");

  nasrg_set_msg_eNBmeasure_retrieve_reply(&msgrep);

  if (copy_to_user(gifr->msg, &msgrep, sizeof(msgrep))) {
    printk("nasrg_ioCTL_eNBmeasure_retrieve_request: copy_to_user failure\n");
    return -EFAULT;
  }

  return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IOCTL command
//---------------------------------------------------------------------------
int nasrg_CTL_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
  //---------------------------------------------------------------------------
  struct nas_ioctl *gifr;
  int r;
  printk("nasrg_CTL_ioctl: begin\n");
  spin_lock(&gpriv->lock);

  switch(cmd) {
  case NASRG_IOCTL_RAL:
    gifr=(struct nas_ioctl *)ifr;

    switch(gifr->type) {
    case NAS_MSG_STATISTIC_REQUEST:
      r=nasrg_ioCTL_statistic_request(gifr);
      break;

    case NAS_MSG_CX_ESTABLISHMENT_REQUEST:
      r=nasrg_ioCTL_cx_establishment_request(gifr);
      break;

    case NAS_MSG_CX_RELEASE_REQUEST:
      r=nasrg_ioCTL_cx_release_request(gifr);
      break;

    case NAS_MSG_CX_LIST_REQUEST:
      r=nasrg_ioCTL_cx_list_request(gifr);
      break;

    case NAS_MSG_RB_ESTABLISHMENT_REQUEST:
      r=nasrg_ioCTL_rb_establishment_request(gifr);
      break;

    case NAS_MSG_RB_RELEASE_REQUEST:
      r=nasrg_ioCTL_rb_release_request(gifr);
      break;

    case NAS_MSG_RB_LIST_REQUEST:
      r=nasrg_ioCTL_rb_list_request(gifr);
      break;

    case NAS_MSG_CLASS_ADD_REQUEST:
      r=nasrg_ioCTL_class_add_request(gifr);
      break;

    case NAS_MSG_CLASS_LIST_REQUEST:
      r=nasrg_ioCTL_class_list_request(gifr);
      break;

    case NAS_MSG_CLASS_DEL_REQUEST:
      r=nasrg_ioCTL_class_del_request(gifr);
      break;

    case NAS_RG_MSG_MT_MCAST_JOIN:
      r=nasrg_ioCTL_ue_multicast_join_request(gifr);
      break;

    case NAS_RG_MSG_MT_MCAST_LEAVE:
      r=nasrg_ioCTL_ue_multicast_leave_request(gifr);
      break;

    case NAS_MSG_ENB_MEAS_TRIGGER:
      r=nasrg_ioCTL_eNBmeasure_trigger_request(gifr);
      break;

    case NAS_MSG_ENB_MEAS_RETRIEVE:
      r=nasrg_ioCTL_eNBmeasure_retrieve_request(gifr);
      break;

    default:
      printk("nasrg_CTL_ioctl: unkwon request type, type=%x\n", gifr->type);
      r=-EFAULT;
    }

    break;

  default:
    printk("nasrg_CTL_ioctl: Unknown ioctl command, cmd=%x\n", cmd);
    r=-EFAULT;
  }

  spin_unlock(&gpriv->lock);
  printk("nasrg_CTL_ioctl: end\n");
  return r;
}

//---------------------------------------------------------------------------
void nasrg_CTL_send(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc)
{
  //---------------------------------------------------------------------------
  printk("nasrg_CTL_send - void \n");
}

