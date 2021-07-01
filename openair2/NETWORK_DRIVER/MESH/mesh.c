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
                          nas_mesh.c  -  description
                             -------------------
    copyright            : (C) 2003-2008 by Eurecom
    email                : navid.nikaein@eurecom.fr
                          lionel.gauthier@eurecom.fr
                           raymond.knopp@eurecom.fr


***************************************************************************/

#include <linux/version.h>
#include "local.h"
#include "proto_extern.h"

//---------------------------------------------------------------------------
void nas_mesh_init(int inst)
{
  //---------------------------------------------------------------------------
  //  struct cx_entity *cx;
  printk("NAS_MESH_INIT Complete\n");

  // Request the establishment of a connexion
  //  cx=nas_COMMON_search_cx(0);
  //  if (cx==NULL)
  //  {
  //    printk("NAS_MESH_INIT: connexion failure\n");
  //    return;
  //  }
  //  cx->countimer=5;
  //  cx->state=NAS_CX_CONNECTING;
  //  cx->retry=0;
  //  cx->cellid=25;
}

//---------------------------------------------------------------------------
//For demo, add automatically a classifier
//Equivalent to class add send 0 -f qos <x> -cr 0
void nas_mesh_start_broadcast_sclassifier(struct cx_entity *cx,struct rb_entity *rb)
{

  struct classifier_entity *gc;
  // Start debug information
#ifdef NAS_DEBUG_CLASS
  printk("NAS_MESH_START_BROADCAST_SCLASS - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_MESH_START_DEFAULT_SCLASS - input parameter cx is NULL \n");
#endif
    return;
  }

  if (rb==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_MESH_START_DEFAULT_SCLASS - input parameter rb is NULL \n");
#endif
    return;
  }

  // End debug information
  gc=nas_CLASS_add_sclassifier(cx, NAS_DSCP_DEFAULT, 6);

  //  gc=nas_CLASS_add_sclassifier(cx, 5, 0);
  if (gc==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_MESH_START_DEFAULT_SCLASS - Classifier not added \n");
#endif
    return;
  }

  gc->fct = nas_COMMON_QOS_send;
  gc->rab_id =rb->rab_id; //5
  gc->rb= rb;
  gc->version = NAS_VERSION_DEFAULT;
  gc->protocol= NAS_PROTOCOL_ICMP6;
  //  gc->sport = NAS_PORT_DEFAULT;
  //  gc->dport = NAS_PORT_DEFAULT;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_MESH_START_DEFAULT_SCLASS - end \n");
  nas_print_classifier(gc);
#endif

}
void nas_mesh_start_default_sclassifier(struct cx_entity *cx,struct rb_entity *rb)
{
  //---------------------------------------------------------------------------
#ifdef DEMO_3GSM
  struct classifier_entity *gc;

  // Start debug information
#ifdef NAS_DEBUG_CLASS
  printk("NAS_MESH_START_DEFAULT_SCLASS - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_MESH_START_DEFAULT_SCLASS - input parameter cx is NULL \n");
#endif
    return;
  }

  if (rb==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_MESH_START_DEFAULT_SCLASS - input parameter rb is NULL \n");
#endif
    return;
  }

  // End debug information
  gc=nas_CLASS_add_sclassifier(cx, NAS_DSCP_DEFAULT, 5);

  //  gc=nas_CLASS_add_sclassifier(cx, 5, 0);
  if (gc==NULL) {
#ifdef NAS_DEBUG_CLASS
    printk("NAS_MESH_START_DEFAULT_SCLASS - Classifier not added \n");
#endif
    return;
  }

  gc->fct = nas_COMMON_QOS_send;
  gc->rab_id =rb->rab_id; //5
  gc->rb= rb;
  gc->version = NAS_VERSION_DEFAULT;
  gc->protocol= NAS_PROTOCOL_DEFAULT;
  //  gc->sport = NAS_PORT_DEFAULT;
  //  gc->dport = NAS_PORT_DEFAULT;
#ifdef NAS_DEBUG_CLASS
  printk("NAS_MESH_START_DEFAULT_SCLASS - end \n");
  nas_print_classifier(gc);
#endif
#endif
}

//---------------------------------------------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
void nas_mesh_timer(struct timer_list *t)
#else
void nas_mesh_timer(unsigned long data)
#endif
{
  //---------------------------------------------------------------------------
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
  struct nas_priv *gpriv=from_timer(gpriv, t, timer);
  #else
  struct nas_priv *gpriv=(struct nas_priv *) data;
  #endif
  uint8_t cxi;
  struct cx_entity *cx;
  struct rb_entity *rb;
  //  spin_lock(&gpriv->lock);
#ifdef NAS_DEBUG_TIMER
  printk("NAS_MESH_TIMER - begin \n");
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
timer_setup(&gpriv->timer, nas_mesh_timer, 0);
  mod_timer(&gpriv->timer, jiffies+NAS_TIMER_TICK);
  #else
  (gpriv->timer).function=nas_mesh_timer;
  (gpriv->timer).expires=jiffies+NAS_TIMER_TICK;
  (gpriv->timer).data=data;
  #endif

  return;


  for (cxi=0; cxi<NAS_CX_MAX; ++cxi) {
    cx=gpriv->cx+cxi;

    if (cx==NULL) {
#ifdef NAS_DEBUG_TIMER
      printk("NAS_MESH_TIMER - No pointer for connection %d \n", cxi);
#endif
      continue;
    }

    if (cx->countimer!=NAS_TIMER_IDLE) {
#ifdef NAS_DEBUG_TIMER
      printk("NAS_MESH_TIMER: lcr %u, countimer %u\n", cx->lcr, cx->countimer);
#endif

      if (cx->countimer==0) {
        switch (cx->state) {
        case NAS_CX_CONNECTING:
        case NAS_CX_CONNECTING_FAILURE:
          if (cx->retry<gpriv->retry_limit)
            nas_mesh_DC_send_cx_establish_request(cx,gpriv);
          else {
            printk("NAS_MESH_TIMER: Establishment failure\n");
            cx->state=NAS_IDLE;
            cx->retry=0;
            cx->countimer=NAS_TIMER_IDLE;
          }

          break;

        case NAS_CX_RELEASING_FAILURE:
          nas_mesh_DC_send_cx_release_request(cx,gpriv);
          break;

        default:
          printk("NAS_MESH_TIMER: default value\n");
          cx->countimer=NAS_TIMER_IDLE;
        }
      } else
        --cx->countimer;
    }

    for (rb=cx->rb; rb!=NULL; rb=rb->next) {
      if (rb->countimer!=NAS_TIMER_IDLE) {
#ifdef NAS_DEBUG_TIMER
        printk("NAS_MESH_TIMER : rb countimer %d, rb state %d\n", rb->countimer, rb->state);
#endif

        if (rb->countimer==0) {
          switch (rb->state) {
          case NAS_RB_DCH:
            nas_mesh_start_default_sclassifier(cx, rb);
            rb->countimer=NAS_TIMER_IDLE;
            break;

          default:
            rb->countimer=NAS_TIMER_IDLE;
          }
        } else {
          --rb->countimer;
          printk("NAS_MESH_TIMER : rb countimer-- %d, rb state %d\n", rb->countimer, rb->state);
        }
      }
    }
  }

  //  add_timer(&gpriv->timer);
  //  spin_unlock(&gpriv->lock);
}


//---------------------------------------------------------------------------
// Request the establishment of a connexion (DC channel)
int nas_mesh_DC_send_cx_establish_request(struct cx_entity *cx,struct nas_priv *gpriv)
{
  //---------------------------------------------------------------------------
  struct nas_ue_dc_element *p;
  int bytes_wrote=0;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_SEND_CX_ESTABLISH - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_SEND_CX_ESTABLISH - input parameter cx is NULL \n");
#endif
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information

  switch (cx->state) {
  case NAS_CX_CONNECTING:
  case NAS_CX_CONNECTING_FAILURE:
  case NAS_IDLE:
    p= (struct nas_ue_dc_element *)(gpriv->xbuffer);
    p->type = CONN_ESTABLISH_REQ;
    p->length =  NAS_TL_SIZE + sizeof(struct NASConnEstablishReq);
    p->nasUEDCPrimitive.conn_establish_req.localConnectionRef = cx->lcr;
    p->nasUEDCPrimitive.conn_establish_req.cellId = cx->cellid;
#ifdef NAS_DEBUG_DC
    printk ("\nCONN_ESTABLISH_REQ Buffer to Xmit: ");
    nas_tool_print_buffer((char *)p,p->length);
#endif
    ++cx->retry;
#ifdef PDCP_USE_NETLINK
#else
    //    bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);
#endif
    cx->countimer=gpriv->timer_establishment;

    if (bytes_wrote==p->length) {
      cx->state=NAS_CX_CONNECTING;
#ifdef NAS_DEBUG_DC
      printk("nas_mesh_DC_send_cx_establish_req: Message sent successfully in DC-FIFO\n");
      printk(" Local Connection reference %u\n", p->nasUEDCPrimitive.conn_establish_req.localConnectionRef);
      printk(" Cell Identification %u\n", p->nasUEDCPrimitive.conn_establish_req.cellId);
      print_TOOL_state(cx->state);
#endif
    } else {
      cx->state=NAS_CX_CONNECTING_FAILURE;
      printk("NAS_MESH_DC_SEND_CX_ESTABLISHMENT_REQUEST: Message sent failure in DC-FIFO\n");
      print_TOOL_state(cx->state);
    }

    return bytes_wrote;

  default:
    return -NAS_ERROR_NOTIDLE;
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_SEND_CX_ESTABLISH - NAS_ERROR_NOTIDLE \n");
#endif
  }
}

//---------------------------------------------------------------------------
// Request the release of a connexion (DC channel)
int nas_mesh_DC_send_cx_release_request(struct cx_entity *cx,
                                        struct nas_priv *gpriv)
{
  //---------------------------------------------------------------------------
  struct nas_ue_dc_element *p;
  int bytes_wrote=0;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_SEND_CX_RELEASE - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_SEND_CX_RELEASE - input parameter cx is NULL \n");
#endif
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information
  switch (cx->state) {
  case NAS_CX_RELEASING_FAILURE:
  case NAS_CX_DCH:
    p= (struct nas_ue_dc_element *)(gpriv->xbuffer);
    p->type = CONN_RELEASE_REQ;
    p->length =  NAS_TL_SIZE + sizeof(struct NASConnReleaseReq);
    p->nasUEDCPrimitive.conn_release_req.localConnectionRef = cx->lcr;
    p->nasUEDCPrimitive.conn_release_req.releaseCause = NAS_CX_RELEASE_UNDEF_CAUSE;
#ifdef PDCP_USE_NETLINK

#else
    //      bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);
#endif

    if (bytes_wrote==p->length) {
      cx->state=NAS_IDLE;
      cx->iid4=0;
      //      nas_TOOL_imei2iid(NAS_NULL_IMEI, (uint8_t *)cx->iid6);
      nas_COMMON_flush_rb(cx);

#ifdef NAS_DEBUG_DC
      printk("NAS_MESH_DC_SEND_CX_RELEASE_REQUEST: Message sent successfully in DC-FIFO\n");
      printk(" Local Connection Reference %u\n", p->nasUEDCPrimitive.conn_release_req.localConnectionRef);
      printk(" Release Cause %u\n", p->nasUEDCPrimitive.conn_release_req.releaseCause);
      print_TOOL_state(cx->state);
#endif
    } else {
      ++cx->retry;
      cx->countimer=gpriv->timer_release;
      cx->state=NAS_CX_RELEASING_FAILURE;
      printk("NAS_MESH_DC_SEND_CX_RELEASE_REQUEST: Message sent failure in DC-FIFO\n");
      print_TOOL_state(cx->state);
    }

    return bytes_wrote;

  default:
    return -NAS_ERROR_NOTCONNECTED;
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_SEND_CX_RELEASE_REQUEST - NAS_ERROR_NOTCONNECTED \n");
#endif
  }
}

//---------------------------------------------------------------------------
// Request the transfer of data (DC SAP)
void nas_mesh_DC_send_sig_data_request(struct sk_buff *skb,
                                       struct cx_entity *cx,
                                       struct classifier_entity *gc,
                                       int inst,
                                       struct nas_priv *gpriv)
{
  //---------------------------------------------------------------------------
  struct nas_ue_dc_element *p;
  //char data_type = 'A';
  int bytes_wrote=0;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_SEND_SIG - begin \n");
#endif

  if (skb==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_SEND_SIG - input parameter skb is NULL \n");
#endif
    return;
  }

  if (gc==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_SEND_SIG - input parameter gc is NULL \n");
#endif
    return;
  }

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_SEND_SIG - input parameter cx is NULL \n");
#endif
    return;
  }

  // End debug information
  if (cx->state!=NAS_CX_DCH) {
    printk("NAS_MESH_DC_SEND_SIG: Not connected, so the message is dropped\n");
    ++gpriv->stats.tx_dropped;
    return;
  }

  p = (struct nas_ue_dc_element *)(gpriv->xbuffer);
  p->type = DATA_TRANSFER_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASDataReq);
  p->nasUEDCPrimitive.data_transfer_req.localConnectionRef = cx->lcr;
  p->nasUEDCPrimitive.data_transfer_req.priority = 3;  // TBD
  p->nasUEDCPrimitive.data_transfer_req.nasDataLength = (skb->len)+1; //adds category character
#ifdef PDCP_USE_NETLINK
#else
  //  bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);
#endif

  if (bytes_wrote!=p->length) {
    printk("NAS_MESH_DC_SEND_SIG: Header sent failure in DC-FIFO\n");
    return;
  }

#ifdef PDCP_USE_NETLINK
#else
  //  bytes_wrote += rtf_put(cx->sap[NAS_DC_INPUT_SAPI], &data_type, 1);
  //  bytes_wrote += rtf_put(cx->sap[NAS_DC_INPUT_SAPI], skb->data, skb->len);
#endif

  if (bytes_wrote != p->length + skb->len + 1) {
    printk("NAS_MESH_DC_SEND_SIG: Data sent failure in DC-FIFO\n");
    return;
  }

  gpriv->stats.tx_bytes   += skb->len;
  gpriv->stats.tx_packets ++;
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_SEND_SIG - end \n");
#endif
}

//---------------------------------------------------------------------------
// Request the transfer of data (DC SAP)
void nas_mesh_DC_send_peer_sig_data_request(struct cx_entity *cx, uint8_t sig_category,
    struct nas_priv *gpriv)
{
  //---------------------------------------------------------------------------
  struct nas_ue_dc_element *p;
  uint8_t nas_data[10];
  unsigned int nas_length;
  //char data_type = 'Z';
  int bytes_wrote=0;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_PEER_SEND_SIG - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_PEER_SEND_SIG - input parameter cx is NULL \n");
#endif
    return;
  }

  // End debug information

  if (cx->state!=NAS_CX_DCH) {
    printk("NAS_MESH_DC_PEER_SEND_SIG: Not connected, so the message is dropped\n");
    return;
  }

  // Initialize peer message
  nas_length = 10;
  memset (nas_data, 0, nas_length);
  nas_data[0]= sig_category;
  //
  p = (struct nas_ue_dc_element *)(gpriv->xbuffer);
  p->type = DATA_TRANSFER_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASDataReq);
  p->nasUEDCPrimitive.data_transfer_req.localConnectionRef = cx->lcr;
  p->nasUEDCPrimitive.data_transfer_req.priority = 3;  // TBD
  p->nasUEDCPrimitive.data_transfer_req.nasDataLength = (nas_length)+1; //adds category character
#ifdef PDCP_USE_NETLINK
#else
  //  bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);
#endif

  if (bytes_wrote!=p->length) {
    printk("NAS_MESH_DC_PEER_SEND_SIG: Header sent failure in DC-FIFO\n");
    return;
  }

#ifdef PDCP_USE_NETLINK
#else
  //  bytes_wrote += rtf_put(cx->sap[NAS_DC_INPUT_SAPI], &data_type, 1);
  //  bytes_wrote += rtf_put(cx->sap[NAS_DC_INPUT_SAPI], (char *)nas_data, nas_length);
#endif

  if (bytes_wrote != p->length + nas_length + 1) {
    printk("NAS_MESH_DC_PEER_SEND_SIG: Data sent failure in DC-FIFO\n");
    return;
  }

#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_PEER_SEND_SIG - end \n");
#endif
}

//---------------------------------------------------------------------------
// Decode CONN_ESTABLISH_RESP message from RRC
void nas_mesh_DC_decode_cx_establish_resp(struct cx_entity *cx, struct nas_ue_dc_element *p,struct nas_priv *gpriv)
{
  //---------------------------------------------------------------------------
  uint8_t sig_category;
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_DECODE_CX_ESTABLISH - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_CX_ESTABLISH - input parameter cx is NULL \n");
#endif
    return;
  }

  if (p==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_CX_ESTABLISH - input parameter p is NULL \n");
#endif
    return;
  }

  // End debug information
  cx->retry=0;

  if (p->nasUEDCPrimitive.conn_establish_resp.status == TERMINATED) {
    cx->state=NAS_CX_DCH; //to be changed to NAS_CX_FACH
    cx->iid4=1;
    //nas_TOOL_imei2iid(NAS_RG_IMEI, (uint8_t *)cx->iid6);
    sig_category = NAS_CMD_OPEN_RB;
    //For demo, add automatically a radio bearer
#ifdef DEMO_3GSM
    printk("NAS_MESH_DC_DECODE_CX_ESTABLISH - sig_category %u \n", sig_category);
    nas_mesh_DC_send_peer_sig_data_request(cx, sig_category,gpriv);
#endif
  } else {
    cx->state=NAS_IDLE;
  }

#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_RECEIVE: CONN_ESTABLISH_RESP\n");
  printk("Local Connection reference %u\n",p->nasUEDCPrimitive.conn_establish_resp.localConnectionRef);
  printk("Connection Establishment status %u\n",p->nasUEDCPrimitive.conn_establish_resp.status);
  print_TOOL_state(cx->state);
#endif
}

//---------------------------------------------------------------------------
// Decode CONN_LOSS_IND message from RRC
void nas_mesh_DC_decode_cx_loss_ind(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_DECODE_CX_LOSS - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_CX_LOSS - input parameter cx is NULL \n");
#endif
    return;
  }

  if (p==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_CX_LOSS - input parameter p is NULL \n");
#endif
    return;
  }

  // End debug information
  cx->state=NAS_IDLE;
  cx->iid4=0;
  //nas_TOOL_imei2iid(NAS_NULL_IMEI, (uint8_t *)cx->iid6);
  nas_COMMON_flush_rb(cx);
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_RECEIVE: CONN_LOSS_IND reception\n");
  printk(" Local Connection reference %u\n", p->nasUEDCPrimitive.conn_loss_ind.localConnectionRef);
  print_TOOL_state(cx->state);
#endif
}

//---------------------------------------------------------------------------
// Decode CONN_RELEASE_IND message from RRC
//void nas_mesh_DC_decode_cx_release_ind(struct cx_entity *cx, struct nas_ue_dc_element *p){
//---------------------------------------------------------------------------
//      printk("\t\tCONN_RELEASE_IND\n");
//      printk("\t\tLocal Connection reference %u\n", p->nasUEDCPrimitive.conn_release_ind.localConnectionRef);
//      printk("\t\tRelease cause %u\n", p->nasRGDCPrimitive.conn_release_ind.releaseCause);
//      if (gpriv->cx[cxi].state==NAS_CX_DCH)
//      {
//        gpriv->cx[cxi].state=NAS_IDLE;
//        printk("\t\tMobile no more connected\n");
//        return bytes_read;
//      }
//      printk("\t\tIncoherent state %u\n", gpriv->cx[cxi].state);
//      return bytes_read;
//}

//---------------------------------------------------------------------------
// Decode DATA_TRANSFER_IND message from RRC
void nas_mesh_DC_decode_sig_data_ind(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_DECODE_SIG_DATA_IND - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_SIG_DATA_IND - input parameter cx is NULL \n");
#endif
    return;
  }

  if (p==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_SIG_DATA_IND - input parameter p is NULL \n");
#endif
    return;
  }

  // End debug information
#ifndef PDCP_USE_NETLINK
  //  nas_COMMON_receive(p->length, p->nasUEDCPrimitive.data_transfer_ind.nasDataLength, cx->sap[NAS_DC_OUTPUT_SAPI]);
#endif
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_RECEIVE: DATA_TRANSFER_IND reception\n");
  printk(" Local Connection reference %u\n",p->nasUEDCPrimitive.data_transfer_ind.localConnectionRef);
  printk(" Signaling Priority %u\n",p->nasUEDCPrimitive.data_transfer_ind.priority);
  printk(" NAS Data length %u\n",p->nasUEDCPrimitive.data_transfer_ind.nasDataLength);
  printk(" NAS Data string %s\n", (uint8_t *)p+p->length);
#endif

}
//---------------------------------------------------------------------------
// Decode RB_ESTABLISH_IND message from RRC
void nas_mesh_DC_decode_rb_establish_ind(struct cx_entity *cx, struct nas_ue_dc_element *p,struct nas_priv *gpriv)
{
  //---------------------------------------------------------------------------
  struct rb_entity *rb;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_DECODE_RB_ESTABLISH_IND - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_RB_ESTABLISH_IND - input parameter cx is NULL \n");
#endif
    return;
  }

  if (p==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_RB_ESTABLISH_IND - input parameter p is NULL \n");
#endif
    return;
  }

  // End debug information
  rb=nas_COMMON_search_rb(cx, p->nasUEDCPrimitive.rb_release_ind.rbId);

  if (rb==NULL) {
    rb=nas_COMMON_add_rb(cx, p->nasUEDCPrimitive.rb_establish_ind.rbId, p->nasUEDCPrimitive.rb_establish_ind.QoSclass);
    rb->state=NAS_RB_DCH;
    cx->state=NAS_CX_DCH;
    //For demo, add automatically a classifier
#ifdef DEMO_3GSM
    rb->countimer=gpriv->timer_establishment+10;
#endif
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_RB_ESTABLISH_IND: RB_ESTABLISH_IND reception\n");
    printk(" Local Connection reference %u\n",p->nasUEDCPrimitive.rb_establish_ind.localConnectionRef);
    printk(" Radio Bearer Identity %u \n",p->nasUEDCPrimitive.rb_establish_ind.rbId);
    printk(" QoS Traffic Class %u\n",p->nasUEDCPrimitive.rb_establish_ind.QoSclass);
    printk(" DSCP Code %u\n",p->nasUEDCPrimitive.rb_establish_ind.dscp);
    printk(" SAP Id %u\n",p->nasUEDCPrimitive.rb_establish_ind.sapId);
    print_TOOL_state(cx->state);
    nas_print_rb_entity(rb);
#endif
  } else
    printk("NAS_MESH_DC_DECODE_RB_ESTABLISH_IND: RB_ESTABLISH_IND reception, Radio bearer already opened\n");
}

//---------------------------------------------------------------------------
// Decode RB_RELEASE_IND message from RRC
void nas_mesh_DC_decode_rb_release_ind(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------
  struct rb_entity *rb;
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_DECODE_RB_RELEASE_IND - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_RB_RELEASE_IND - input parameter cx is NULL \n");
#endif
    return;
  }

  if (p==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_RB_RELEASE_IND - input parameter p is NULL \n");
#endif
    return;
  }

  // End debug information
  rb=nas_COMMON_search_rb(cx, p->nasUEDCPrimitive.rb_release_ind.rbId);

  if (rb!=NULL) {
    rb->state=NAS_IDLE;
    //needs also to flush corresponding control block to be coherent with add_rb
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_RB_RELEASE_IND: RB_RELEASE_IND reception\n");
    printk("Local Connection reference %u\n",p->nasUEDCPrimitive.rb_release_ind.localConnectionRef);
    printk("Radio Bearer Identity %u\n",p->nasUEDCPrimitive.rb_release_ind.rbId);
    print_TOOL_state(cx->state);
#endif
  } else
    printk("NAS_DC_RG_RECEIVE: RB_RELEASE_IND reception, No corresponding radio bearer\n");

}
//---------------------------------------------------------------------------
// Decode MEASUREMENT_IND message from RRC
void nas_mesh_DC_decode_measurement_ind(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------
  uint8_t i;
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_DECODE_MEASUREMENT_IND - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_MEASUREMENT_IND - input parameter cx is NULL \n");
#endif
    return;
  }

  if (p==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_DECODE_MEASUREMENT_IND - input parameter p is NULL \n");
#endif
    return;
  }

  // End debug information
#ifdef NAS_DEBUG_DC_MEASURE
  printk("NAS_MESH_DC_RECEIVE: MEASUREMENT_IND reception\n");
  printk(" Local Connection reference: %u\n", p->nasUEDCPrimitive.measurement_ind.localConnectionRef);
  printk(" Number of RGs: %u\n", p->nasUEDCPrimitive.measurement_ind.nb_rg);
  print_TOOL_state(cx->state);

  for (i=0; i<p->nasUEDCPrimitive.measurement_ind.nb_rg; ++i) {
    printk(" RG[%u]:  Cell_Id %u, Level: %u\n", i,
           p->nasUEDCPrimitive.measurement_ind.measures[i].cell_id,
           p->nasUEDCPrimitive.measurement_ind.measures[i].level);
  }

#endif //NAS_DEBUG_DC_MEASURE
  cx->num_measures = p->nasUEDCPrimitive.measurement_ind.nb_rg;

  for (i=0; i<cx->num_measures; i++) {
    cx->meas_cell_id[i]= (int)(p->nasUEDCPrimitive.measurement_ind.measures[i].cell_id);
    cx->meas_level[i] = (int)(p->nasUEDCPrimitive.measurement_ind.measures[i].level);
    //npriv->provider_id[i]=;
  }

  cx->provider_id[0]=25;
  cx->provider_id[1]=1;
  cx->provider_id[2]=25;

}

//---------------------------------------------------------------------------
// Check if anything in DC FIFO and decode it (MESH)
int nas_mesh_DC_receive(struct cx_entity *cx,struct nas_priv *gpriv)
{
  //---------------------------------------------------------------------------
  // Start debug information
  int bytes_read=0;

#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_RECEIVE - begin \n");
#endif

  if (cx==NULL) {
#ifdef NAS_DEBUG_DC
    printk("NAS_MESH_DC_RECEIVE - input parameter cx is NULL \n");
#endif
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information
#ifdef PDCP_USE_NETLINK
#else
  //  bytes_read = rtf_get(cx->sap[NAS_DC_OUTPUT_SAPI] , gpriv->rbuffer, NAS_TL_SIZE);
#endif

  if (bytes_read>0) {
    struct nas_ue_dc_element *p;

    p= (struct nas_ue_dc_element *)(gpriv->rbuffer);
    //get the rest of the primitive
#ifdef PDCP_USE_NETLINK
#else
    //    bytes_read += rtf_get(cx->sap[NAS_DC_OUTPUT_SAPI], (uint8_t *)p+NAS_TL_SIZE, p->length-NAS_TL_SIZE);
#endif

    if (bytes_read!=p->length) {
      printk("NAS_MESH_DC_RECEIVE: Problem while reading primitive header\n");
      return bytes_read;
    }

    switch (p->type) {
    case CONN_ESTABLISH_RESP :
      if (p->nasUEDCPrimitive.conn_establish_resp.localConnectionRef!=cx->lcr)
        printk("NAS_MESH_DC_RECEIVE: CONN_ESTABLISH_RESP, Local connection reference not correct %u\n",p->nasUEDCPrimitive.conn_establish_resp.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_CONNECTING:
          nas_mesh_DC_decode_cx_establish_resp(cx,p,gpriv);   // process message
          break;

        default:
          printk("NAS_MESH_DC_RECEIVE: CONN_ESTABLISH_RESP reception, Invalid state %u\n", cx->state);
        }
      }

      break;

    case CONN_LOSS_IND :
      if (p->nasUEDCPrimitive.conn_loss_ind.localConnectionRef!=cx->lcr)
        printk("NAS_MESH_DC_RECEIVE: CONN_LOSS_IND reception, Local connection reference not correct %u\n", p->nasUEDCPrimitive.conn_loss_ind.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_RELEASING_FAILURE:
          cx->retry=0;
          /* fallthrough */

        case NAS_CX_DCH:
          nas_mesh_DC_decode_cx_loss_ind(cx,p);   // process message
          break;

        default:
          printk("NAS_MESH_DC_RECEIVE: CONN_LOSS_IND reception, Invalid state %u", cx->state);
        }
      }

      break;

      //    case CONN_RELEASE_IND :
      //      break;
    case DATA_TRANSFER_IND :
      if (p->nasUEDCPrimitive.data_transfer_ind.localConnectionRef!=cx->lcr)
        printk("NAS_MESH_DC_RECEIVE: DATA_TRANSFER_IND reception, Local connection reference not correct %u\n", p->nasUEDCPrimitive.conn_loss_ind.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_FACH:
        case NAS_CX_DCH:
          nas_mesh_DC_decode_sig_data_ind(cx,p);   // process message
          break;

        default:
          printk("NAS_MESH_DC_RECEIVE: DATA_TRANSFER_IND reception, Invalid state %u", cx->state);
        }
      }

      break;

    case RB_ESTABLISH_IND :
      if (p->nasUEDCPrimitive.rb_establish_ind.localConnectionRef!=cx->lcr)
        printk("NAS_MESH_DC_RECEIVE: RB_ESTABLISH_IND reception, Local connexion reference not correct %u\n", p->nasUEDCPrimitive.rb_establish_ind.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_FACH:
        case NAS_CX_DCH:
          nas_mesh_DC_decode_rb_establish_ind(cx,p,gpriv);   // process message
          break;

        default:
          printk("NAS_MESH_DC_RECEIVE: RB_ESTABLISH_IND reception, Invalid state %u", cx->state);
        }
      }

      break;

    case RB_RELEASE_IND :
      if (p->nasUEDCPrimitive.rb_release_ind.localConnectionRef!=cx->lcr)
        printk("NAS_DC_MESH_RECEIVE: RB_RELEASE_IND reception, Local connection reference not correct %u\n", p->nasUEDCPrimitive.rb_release_ind.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_DCH:
          nas_mesh_DC_decode_rb_release_ind(cx,p);   // process message
          break;

        default:
          printk("NAS_MESH_DC_RECEIVE: RB_RELEASE_IND reception, Invalid state %u", cx->state);
        }
      }

      break;

    case MEASUREMENT_IND :
      if (p->nasUEDCPrimitive.measurement_ind.localConnectionRef!=cx->lcr)
        printk("NAS_MESH_DC_RECEIVE: MEASUREMENT_IND reception, Local connection reference not correct %u\n", p->nasUEDCPrimitive.measurement_ind.localConnectionRef);
      else {
        nas_mesh_DC_decode_measurement_ind(cx,p);
      }

      break;

    default :
      printk("NAS_MESH_DC_RECEIVE: Invalid message received\n");
    }
  }

#ifdef NAS_DEBUG_DC
  printk("NAS_MESH_DC_RECEIVE - end \n");
#endif
  return bytes_read;
}

//---------------------------------------------------------------------------
// Check if anything in GC FIFO and decode it (MESH)
int nas_mesh_GC_receive(struct nas_priv *gpriv)
{
  //---------------------------------------------------------------------------

  int bytes_read=0;

#ifdef NAS_DEBUG_GC
  printk("NAS_MESH_GC_RECEIVE - begin \n");
#endif
#ifdef PDCP_USE_NETLINK
#else
  //  bytes_read = rtf_get(gpriv->sap[NAS_GC_SAPI], gpriv->rbuffer, NAS_TL_SIZE);
#endif

  if (bytes_read>0) {
    struct nas_ue_gc_element *p;
    p= (struct nas_ue_gc_element *)(gpriv->rbuffer);
    //get the rest of the primitive
#ifdef PDCP_USE_NETLINK
#else
    //      bytes_read += rtf_get(gpriv->sap[NAS_GC_SAPI], (uint8_t *)p+NAS_TL_SIZE, p->length-NAS_TL_SIZE);
#endif

    if (bytes_read!=p->length) {
      printk("NAS_MESH_GC_RECEIVE: Problem while reading primitive's header\n");
      return bytes_read;
    }

    // start decoding message
    switch (p->type) {
    case INFO_BROADCAST_IND :
#ifdef PDCP_USE_NETLINK
#else
      //    bytes_read += rtf_get(gpriv->sap[NAS_GC_SAPI], (uint8_t *)p+p->length, p->nasUEGCPrimitive.broadcast_ind.nasDataLength);
#endif
      if (bytes_read!=p->length+p->nasUEGCPrimitive.broadcast_ind.nasDataLength) {
        printk("NAS_MESH_GC_RECEIVE: INFO_BROADCAST_IND reception, Problem while reading primitive's data\n");
        return bytes_read;
      }

#ifdef NAS_DEBUG_GC
      printk("NAS_MESH_GC_RECEIVE: INFO_BROADCAST_IND reception\n");
      printk(" Primitive length %d \n", (int)(p->type));
      printk(" Data length %u\n", p->nasUEGCPrimitive.broadcast_ind.nasDataLength);
      printk(" Data string %s\n", (uint8_t *)p+p->length);
#endif //NAS_DEBUG_GC
      return bytes_read;

    default :
      printk("NAS_MESH_GC_RECEIVE: Invalid message received\n");
      return -1;
    }
  } else
    return -1;
}


