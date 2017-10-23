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

/*! \file nasmt_ascontrol.c
* \brief Access Stratum Control functions for OpenAirInterface CELLULAR version - MT
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#ifdef NODE_MT
#include "nasmt_variables.h"
#include "nasmt_proto.h"

//---------------------------------------------------------------------------
void nasmt_ASCTL_init(void)
{
  //---------------------------------------------------------------------------
  //  struct cx_entity *cx;
  int i;
  gpriv->next_sclassref = NASMT_DEFAULTRAB_CLASSREF;

  for (i = 0; i<NASMT_MBMS_SVCES_MAX; i++) {
    gpriv->cx->joined_services[i]= -1;
  }

  printk("nasmt_ASCTL_init Complete\n");
}

//---------------------------------------------------------------------------
int nasmt_ASCTL_write(int sap, unsigned char *data_buffer, unsigned int data_length)
{
  //---------------------------------------------------------------------------
  int bytes_wrote = 0;
#ifdef PDCP_USE_NETLINK
  unsigned char xmit_buffer [NAS_MESSAGE_MAXLEN];
  //MT xmit on DC-SAP only
  xmit_buffer[0] = RRC_NAS_DC0_IN;
  memcpy (&((char*)xmit_buffer)[1], data_buffer, data_length);
  bytes_wrote = nasmt_netlink_send(xmit_buffer,data_length, NASNL_DEST_RRC);
#else
  //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);  //original version
  bytes_wrote = rtf_put(sap, data_buffer, data_length);
#endif //PDCP_USE_NETLINK
  return bytes_wrote;
}

//---------------------------------------------------------------------------
//For demo, add automatically a classifier
//Equivalent to class add send 0 -f qos <x> -cr 0
void nasmt_ASCTL_start_default_sclassifier(struct cx_entity *cx,struct rb_entity *rb)
{
  //---------------------------------------------------------------------------
  struct classifier_entity *gc;

  // Start debug information
#ifdef NAS_DEBUG_CLASS
  printk("\nnasmt_ASCTL_start_default_sclass - begin \n");
#endif

  if (!cx || !rb) {
    printk("nasmt_ASCTL_start_default_sclass - input parameter cx or rb is NULL \n");
    return;
  }

  // End debug information
  //
  gc=nasmt_CLASS_add_sclassifier(cx, NAS_DSCP_DEFAULT, gpriv->next_sclassref);

  //  gc=nasmt_CLASS_add_sclassifier(cx, 5, 0);
  if (gc==NULL) {
    printk("nasmt_ASCTL_start_default_sclass - Error - Classifier not added \n");
    return;
  }

  gc->fct = nasmt_COMMON_QOS_send;
  gc->rab_id =rb->rab_id;
  gc->rb= rb;
  gc->version = NASMT_DEFAULTRAB_IPVERSION;
  gc->protocol= NAS_PROTOCOL_DEFAULT;
  gc->dplen= NAS_DEFAULT_IPv6_PREFIX_LENGTH;
  //gc->daddr.ipv6.s6_addr32[2] = cx->iid6[0];
  //gc->daddr.ipv6.s6_addr32[3] = cx->iid6[1];

#ifdef NAS_DEBUG_CLASS
  printk("nasmt_ASCTL_start_default_sclass - end \n");
  nasmt_TOOL_print_classifier(gc);
#endif
}

//---------------------------------------------------------------------------
//For demo, add automatically a classifier
//Equivalent to class add send 0 -f qos <x> -cr 0
void nasmt_ASCTL_start_sclassifier(struct cx_entity *cx,struct rb_entity *rb)
{
  //---------------------------------------------------------------------------
  struct classifier_entity *gc;

  // Start debug information
#ifdef NAS_DEBUG_CLASS
  printk("\nnasmt_ASCTL_start_sclass - begin \n");
#endif

  if (cx==NULL) {
    printk("nasmt_ASCTL_start_sclass - input parameter cx is NULL \n");
    return;
  }

  if (rb==NULL) {
    printk("nasmt_ASCTL_start_sclass - input parameter rb is NULL \n");
    return;
  }

  // End debug information
  //
  gc=nasmt_CLASS_add_sclassifier(cx, rb->dscp, gpriv->next_sclassref);

  //  gc=nasmt_CLASS_add_sclassifier(cx, 5, 0);
  if (gc==NULL) {
    printk("nasmt_ASCTL_start_sclass - Error - Classifier not added \n");
    return;
  }

  gc->fct = nasmt_COMMON_QOS_send;
  gc->rab_id =rb->rab_id;
  gc->rb= rb;
  gc->version = NASMT_DEFAULTRAB_IPVERSION;
  gc->protocol= NAS_PROTOCOL_DEFAULT;
  gc->dplen= NAS_DEFAULT_IPv6_PREFIX_LENGTH;
  gc->daddr.ipv6.s6_addr32[2] = cx->iid6[0];
  gc->daddr.ipv6.s6_addr32[3] = cx->iid6[1];
#ifdef NAS_DEBUG_CLASS
  printk("nasmt_ASCTL_start_sclass - end \n");
  nasmt_TOOL_print_classifier(gc);
#endif
}

//---------------------------------------------------------------------------
void nasmt_ASCTL_timer(unsigned long data)
{
  //---------------------------------------------------------------------------
  uint8_t cxi;
  struct cx_entity *cx;
  struct rb_entity *rb;
  spin_lock(&gpriv->lock);
#ifdef NAS_DEBUG_TIMER
  printk("nasmt_ASCTL_timer - begin \n");
#endif
  (gpriv->timer).function=nasmt_ASCTL_timer;
  (gpriv->timer).expires=jiffies+NAS_TIMER_TICK;
  (gpriv->timer).data=0L;

  for (cxi=0; cxi<NAS_CX_MAX; ++cxi) {
    cx=gpriv->cx+cxi;

    if (cx==NULL) {
#ifdef NAS_DEBUG_TIMER
      printk("nasmt_ASCTL_timer - No pointer for connection %d \n", cxi);
#endif
      continue;
    }

    if (cx->countimer!=NAS_TIMER_IDLE) {
#ifdef NAS_DEBUG_TIMER
      printk("nasmt_ASCTL_timer: lcr %u, countimer %u\n", cx->lcr, cx->countimer);
#endif

      if (cx->countimer==0) {
        switch (cx->state) {
        case NAS_CX_CONNECTING:
        case NAS_CX_CONNECTING_FAILURE:
          if (cx->retry<gpriv->retry_limit)
            nasmt_ASCTL_DC_send_cx_establish_request(cx);
          else {
            printk("nasmt_ASCTL_timer: Establishment failure\n");
            cx->state=NAS_IDLE;
            cx->retry=0;
            cx->countimer=NAS_TIMER_IDLE;
          }

          break;

        case NAS_CX_RELEASING_FAILURE:
          nasmt_ASCTL_DC_send_cx_release_request(cx);
          break;

        default:
          printk("nasmt_ASCTL_timer: default value\n");
          cx->countimer=NAS_TIMER_IDLE;
        }
      } else
        --cx->countimer;
    }

    for (rb=cx->rb; rb!=NULL; rb=rb->next) {
      if (rb->countimer!=NAS_TIMER_IDLE) {
#ifdef NAS_DEBUG_TIMER
        printk("nasmt_ASCTL_timer : rb countimer %d, rb state %d\n", rb->countimer, rb->state);
#endif

        if (rb->countimer==0) {
          switch (rb->state) {
          case NAS_RB_DCH:
#ifdef DEMO_3GSM
            if (cx->num_rb == 1) {
              nasmt_ASCTL_start_default_sclassifier(cx, rb);
            }

#endif
            nasmt_ASCTL_start_sclassifier(cx, rb);
            rb->countimer=NAS_TIMER_IDLE;
            break;

          default:
            rb->countimer=NAS_TIMER_IDLE;
          }
        } else {
          --rb->countimer;
          printk("nasmt_ASCTL_timer : rb countimer-- %d, rb state %d\n", rb->countimer, rb->state);
        }
      }
    }
  }

  add_timer(&gpriv->timer);
  spin_unlock(&gpriv->lock);
}

//---------------------------------------------------------------------------
// Request the sleep of a connexion
int nasmt_ASCTL_enter_sleep_mode(struct cx_entity *cx)
{
  //---------------------------------------------------------------------------
  uint8_t sig_category;
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_enter_sleep_mode - begin \n");
#endif

  if (!cx) {
    printk("nasmt_ASCTL_enter_sleep_mode - input parameter cx is NULL \n");
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information
  // send peer message to NASRG
  sig_category = NAS_CMD_ENTER_SLEEP;
  printk("nasmt_ASCTL_enter_sleep_mode - sig_category %u \n", sig_category);
  nasmt_ASCTL_DC_send_peer_sig_data_request(cx, sig_category);
  cx->state=NAS_CX_RELEASING;
  return 0;
}

//---------------------------------------------------------------------------
// Request to reactivate a connexion
int nasmt_ASCTL_leave_sleep_mode(struct cx_entity *cx)
{
  //---------------------------------------------------------------------------
  uint8_t sig_category;
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_leave_sleep_mode - begin \n");
#endif

  if (!cx) {
    printk("nasmt_ASCTL_leave_sleep_mode - input parameter cx is NULL \n");
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information
  cx->state=NAS_CX_DCH;
  // send peer message to NASRG
  sig_category = NAS_CMD_LEAVE_SLEEP;
  printk("nasmt_ASCTL_leave_sleep_mode - sig_category %u \n", sig_category);
  nasmt_ASCTL_DC_send_peer_sig_data_request(cx, sig_category);
  return 0;
}

/***********************************************
 *  Transmit Functions                         *
 ***********************************************/

//---------------------------------------------------------------------------
// Request the establishment of a connexion (DC channel)
int nasmt_ASCTL_DC_send_cx_establish_request(struct cx_entity *cx)
{
  //---------------------------------------------------------------------------
  struct nas_ue_dc_element *p;
  int bytes_wrote = 0;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_send_cx_establish - begin \n");
#endif

  if (!cx) {
    printk("nasmt_ASCTL_DC_send_cx_establish - input parameter cx is NULL \n");
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
    nasmt_TOOL_print_buffer((char *)p,p->length);
#endif
    ++cx->retry;

    //bytes_wrote = rtf_put(cx->sap[GRAAL_DC_INPUT_SAPI], p, p->length); //original version
    bytes_wrote = nasmt_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)p, p->length);
    //printk("nasmt_ASCTL_DC_send_cx_establish_request - Wrote %d bytes to RRC NAS_DC_INPUT_SAPI\n", bytes_wrote);

    cx->countimer=gpriv->timer_establishment;

    if (bytes_wrote==p->length) {
      cx->state=NAS_CX_CONNECTING;
#ifdef NAS_DEBUG_DC
      printk("nasmt_ASCTL_DC_send_cx_establish - Message sent successfully in DC-FIFO\n");
      printk(" Local Connection reference %u\n", p->nasUEDCPrimitive.conn_establish_req.localConnectionRef);
      printk(" Cell Identification %u\n", p->nasUEDCPrimitive.conn_establish_req.cellId);
      nasmt_TOOL_print_state(cx->state);
#endif
    } else {
      cx->state=NAS_CX_CONNECTING_FAILURE;
      printk("nasmt_ASCTL_DC_send_cx_establish - Message sent failure in DC-FIFO\n");
      nasmt_TOOL_print_state(cx->state);
    }

    return bytes_wrote;

  default:
    return -NAS_ERROR_NOTIDLE;
#ifdef NAS_DEBUG_DC
    printk("nasmt_ASCTL_DC_send_cx_establish - NAS_ERROR_NOTIDLE \n");
#endif
  }
}

//---------------------------------------------------------------------------
// Request the release of a connexion (DC channel)
int nasmt_ASCTL_DC_send_cx_release_request(struct cx_entity *cx)
{
  //---------------------------------------------------------------------------
  struct nas_ue_dc_element *p;
  int bytes_wrote = 0;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_send_cx_release - begin \n");
#endif

  if (cx==NULL) {
    printk("nasmt_ASCTL_DC_send_cx_release - input parameter cx is NULL \n");
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

    //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length); //original version
    bytes_wrote = nasmt_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)p, p->length);
    //printk("nasmt_ASCTL_DC_send_cx_release - Wrote %d bytes to RRC NAS_DC_INPUT_SAPI\n", bytes_wrote);

    if (bytes_wrote==p->length) {
      cx->state=NAS_IDLE;
      cx->iid4=0;
      //      nasmt_TOOL_imei2iid(NAS_NULL_IMEI, (uint8_t *)cx->iid6);
      nasmt_COMMON_flush_rb(cx);
      nasmt_CLASS_flush_sclassifier(cx);

#ifdef NAS_DEBUG_DC
      printk("nasmt_ASCTL_DC_send_cx_release - Message sent successfully in DC-FIFO\n");
      printk(" Local Connection Reference %u\n", p->nasUEDCPrimitive.conn_release_req.localConnectionRef);
      printk(" Release Cause %u\n", p->nasUEDCPrimitive.conn_release_req.releaseCause);
      nasmt_TOOL_print_state(cx->state);
#endif
    } else {
      ++cx->retry;
      cx->countimer=gpriv->timer_release;
      cx->state=NAS_CX_RELEASING_FAILURE;
      printk("nasmt_ASCTL_DC_send_cx_release - Message sent failure in DC-FIFO\n");
      nasmt_TOOL_print_state(cx->state);
    }

    return bytes_wrote;

  default:
    return -NAS_ERROR_NOTCONNECTED;
#ifdef NAS_DEBUG_DC
    printk("nasmt_ASCTL_DC_send_cx_release - NAS_ERROR_NOTCONNECTED \n");
#endif
  }
}

//---------------------------------------------------------------------------
// Request the transfer of data (DC SAP)
void nasmt_ASCTL_DC_send_sig_data_request(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc)
{
  //---------------------------------------------------------------------------
  struct nas_ue_dc_element *p;
  char data_type = 'A';
  int bytes_wrote = 0;
#ifdef PDCP_USE_NETLINK
  unsigned char xbuffer[NAS_MESSAGE_MAXLEN];
  int count=0;
#endif
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_send_sig_data - begin \n");
#endif

  if (!skb || !gc || !cx) {
    printk("nasmt_ASCTL_DC_send_sig_data - input parameter skb, gc or cx is NULL \n");
    return;
  }

  // End debug information
  if (cx->state!=NAS_CX_DCH) {
    printk("nasmt_ASCTL_DC_send_sig_data - Not connected, so the message is dropped\n");
    ++gpriv->stats.tx_dropped;
    return;
  }

  p = (struct nas_ue_dc_element *)(gpriv->xbuffer);
  p->type = DATA_TRANSFER_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASDataReq);
  p->nasUEDCPrimitive.data_transfer_req.localConnectionRef = cx->lcr;
  p->nasUEDCPrimitive.data_transfer_req.priority = 3;  // TBD
  p->nasUEDCPrimitive.data_transfer_req.nasDataLength = (skb->len)+1; //adds category character

  //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length); //original version
#ifdef PDCP_USE_NETLINK
  memcpy(xbuffer,(unsigned char *)p, p->length);
  count = p->length;
  bytes_wrote = count;
#else
  bytes_wrote = nasmt_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)p, p->length);
#endif
  //printk("nasmt_ASCTL_DC_send_sig_data - Wrote %d bytes to RRC NAS_DC_INPUT_SAPI\n", bytes_wrote);

  if (bytes_wrote!=p->length) {
    printk("nasmt_ASCTL_DC_send_sig_data - Header sent failure in DC-FIFO\n");
    return;
  }

#ifdef PDCP_USE_NETLINK
  memcpy(&(xbuffer[count]),(unsigned char *)&data_type, 1);
  count += 1;
  bytes_wrote = count;
#else
  bytes_wrote += nasmt_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)&data_type, 1);
#endif

#ifdef PDCP_USE_NETLINK
  memcpy(&(xbuffer[count]),(unsigned char *)skb->data, skb->len);
  count += skb->len;
  bytes_wrote = nasmt_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], xbuffer, count);
#else
  bytes_wrote += nasmt_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)skb->data, skb->len);
#endif

  if (bytes_wrote != p->length + skb->len + 1) {
    printk("nasmt_ASCTL_DC_send_sig_data - Data sent failure in DC-FIFO\n");
    return;
  }

  gpriv->stats.tx_bytes   += skb->len;
  gpriv->stats.tx_packets ++;
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_send_sig_data - end \n");
#endif
}

//---------------------------------------------------------------------------
// Request the transfer of data (DC SAP)
void nasmt_ASCTL_DC_send_peer_sig_data_request(struct cx_entity *cx, uint8_t sig_category)
{
  //---------------------------------------------------------------------------
  struct nas_ue_dc_element *p;
  uint8_t nasmt_data[10];
  unsigned int nasmt_length;
  char data_type = 'Z';
  int bytes_wrote = 0;
#ifdef PDCP_USE_NETLINK
  unsigned char xbuffer[NAS_MESSAGE_MAXLEN];
  int count=0;
#endif

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_send_peer_sig_data - begin \n");
#endif

  if (!cx) {
    printk("nasmt_ASCTL_DC_send_peer_sig_data - input parameter cx is NULL \n");
    return;
  }

  // End debug information

  if (cx->state!=NAS_CX_DCH) {
    printk("nasmt_ASCTL_DC_send_peer_sig_data: Not connected, so the message is dropped\n");
    return;
  }

  // Initialize peer message
  nasmt_length = 10;
  memset (nasmt_data, 0, nasmt_length);
  nasmt_data[0]= sig_category;
  //
  p = (struct nas_ue_dc_element *)(gpriv->xbuffer);
  p->type = DATA_TRANSFER_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASDataReq);
  p->nasUEDCPrimitive.data_transfer_req.localConnectionRef = cx->lcr;
  p->nasUEDCPrimitive.data_transfer_req.priority = 3;  // TBD
  p->nasUEDCPrimitive.data_transfer_req.nasDataLength = (nasmt_length)+1; //adds category character

  //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length); //original version
#ifdef PDCP_USE_NETLINK
  memcpy(xbuffer,(unsigned char *)p, p->length);
  count = p->length;
  bytes_wrote = count;
#else
  bytes_wrote = nasmt_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)p, p->length);
#endif
  //printk("nasmt_ASCTL_DC_send_sig_data - Wrote %d bytes to RRC NAS_DC_INPUT_SAPI\n", bytes_wrote);

  if (bytes_wrote!=p->length) {
    printk("nasmt_ASCTL_DC_send_peer_sig_data - Header sent failure in DC-FIFO\n");
    return;
  }

#ifdef PDCP_USE_NETLINK
  memcpy(&(xbuffer[count]),(unsigned char *)&data_type, 1);
  count += 1;
#else
  bytes_wrote += nasmt_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)&data_type, 1);
#endif

#ifdef PDCP_USE_NETLINK
  memcpy(&(xbuffer[count]),(unsigned char *)nasmt_data, nasmt_length);
  count += nasmt_length;
  bytes_wrote = nasmt_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], xbuffer, count);
#else
  bytes_wrote += nasmt_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (char *)nasmt_data, nasmt_length);
#endif

  if (bytes_wrote != p->length + nasmt_length + 1) {
    printk("nasmt_ASCTL_DC_send_peer_sig_data - Data sent failure in DC-FIFO\n");
    return;
  }

#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_send_peer_sig_data - end \n");
#endif
}

/***************************************************************************
     Reception side
 ***************************************************************************/
//---------------------------------------------------------------------------
// Decode CONN_ESTABLISH_RESP message from RRC
void nasmt_ASCTL_DC_decode_cx_establish_resp(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------

  uint8_t sig_category;
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_decode_cx_establish - begin \n");
#endif

  if (!cx || !p) {
    printk("nasmt_ASCTL_DC_decode_cx_establish - input parameter cx or p is NULL \n");
    return;
  }

  // End debug information
  cx->retry=0;

  if (p->nasUEDCPrimitive.conn_establish_resp.status == TERMINATED) {
    cx->state=NAS_CX_DCH; //to be changed to NAS_CX_FACH
    cx->iid4=1;
    nasmt_TOOL_imei2iid(NAS_RG_IMEI, (uint8_t *)cx->iid6);
    sig_category = NAS_CMD_OPEN_RB;
    //For demo, add automatically a radio bearer
#ifdef DEMO_3GSM
    printk("nasmt_ASCTL_DC_decode_cx_establish - sig_category %u \n", sig_category);
    nasmt_ASCTL_DC_send_peer_sig_data_request(cx, sig_category);
#endif
  } else {
    cx->state=NAS_IDLE;
  }

#ifdef NAS_DEBUG_DC
  printk(" nasmt_ASCTL_DC_decode_cx_establish: CONN_ESTABLISH_RESP\n");
  printk(" Local Connection reference %u\n",p->nasUEDCPrimitive.conn_establish_resp.localConnectionRef);
  printk(" Connection Establishment status %u\n",p->nasUEDCPrimitive.conn_establish_resp.status);
  nasmt_TOOL_print_state(cx->state);
#endif
}

//---------------------------------------------------------------------------
// Decode CONN_LOSS_IND message from RRC
void nasmt_ASCTL_DC_decode_cx_loss_ind(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_decode_cx_loss - begin \n");
#endif

  if (cx==NULL) {
    printk("nasmt_ASCTL_DC_decode_cx_loss - input parameter cx is NULL \n");
    return;
  }

  if (p==NULL) {
    printk("nasmt_ASCTL_DC_decode_cx_loss - input parameter p is NULL \n");
    return;
  }

  // End debug information
  cx->state=NAS_IDLE;
  cx->iid4=0;
  //nasmt_TOOL_imei2iid(NAS_NULL_IMEI, (uint8_t *)cx->iid6);
  nasmt_COMMON_flush_rb(cx);
#ifdef NAS_DEBUG_DC
  printk(" nasmt_ASCTL_DC_decode_cx_loss: CONN_LOSS_IND reception\n");
  printk(" Local Connection reference %u\n", p->nasUEDCPrimitive.conn_loss_ind.localConnectionRef);
  nasmt_TOOL_print_state(cx->state);
#endif
}

//---------------------------------------------------------------------------
// Decode CONN_RELEASE_IND message from RRC
//void nasmt_ASCTL_DC_decode_cx_release_ind(struct cx_entity *cx, struct nas_ue_dc_element *p){
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
void nasmt_ASCTL_DC_decode_sig_data_ind(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------
  //  struct pdcp_data_ind_header_t  *pdcph;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_decode_sig_data - begin \n");
#endif

  if (!cx || !p) {
    printk("nasmt_ASCTL_DC_decode_sig_data - input parameter cx or p is NULL \n");
    return;
  }

  // End debug information

#ifdef NAS_DEBUG_DC
  printk(" nasmt_ASCTL_DC_decode_sig_data: DATA_TRANSFER_IND reception\n");
  printk(" Local Connection reference %u\n",p->nasUEDCPrimitive.data_transfer_ind.localConnectionRef);
  printk(" Signaling Priority %u\n",p->nasUEDCPrimitive.data_transfer_ind.priority);
  printk(" NAS Data length %u\n",p->nasUEDCPrimitive.data_transfer_ind.nasDataLength);
  printk(" NAS Data string %s\n", (uint8_t *)p+p->length);
#endif

  //nasmt_COMMON_receive(p->length, p->nasUEDCPrimitive.data_transfer_ind.nasDataLength, cx->sap[GRAAL_DC_OUTPUT_SAPI]); // original
#ifndef PDCP_USE_NETLINK
  //void nasmt_COMMON_receive(uint16_t bytes_read, uint16_t payload_length, void *data_buffer, int rb_id, int sap);
  // data_buffer is NULL because FIFO should be read directly in the skbuff (LITE has an intermediary buffer)
  nasmt_COMMON_receive(p->length, p->nasUEDCPrimitive.data_transfer_ind.nasDataLength, NULL, 2, cx->sap[NAS_DC_OUTPUT_SAPI]);
#else
  nasmt_COMMON_receive(p->length, p->nasUEDCPrimitive.data_transfer_ind.nasDataLength, (unsigned char *)p+p->length, 2, 0);
#endif

}
//---------------------------------------------------------------------------
// Decode RB_ESTABLISH_IND message from RRC
void nasmt_ASCTL_DC_decode_rb_establish_ind(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------
  struct rb_entity *rb;
  int hard_coded_rbId = 3;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_decode_rb_establish - begin \n");
#endif

  if (cx==NULL) {
    printk("nasmt_ASCTL_DC_decode_rb_establish - input parameter cx is NULL \n");
    return;
  }

  if (p==NULL) {
    printk("nasmt_ASCTL_DC_decode_rb_establish - input parameter p is NULL \n");
    return;
  }

  // End debug information
  // TEMP -MW - 26/9/13- Overwrite the rb_id sent by RRC
  p->nasUEDCPrimitive.rb_establish_ind.rbId = hard_coded_rbId;
  // TEMP -MW - 26/9/13- Rest should be kept unchanged
  rb=nasmt_COMMON_search_rb(cx, p->nasUEDCPrimitive.rb_establish_ind.rbId);

  if (rb==NULL) {
    rb=nasmt_COMMON_add_rb(cx, p->nasUEDCPrimitive.rb_establish_ind.rbId, p->nasUEDCPrimitive.rb_establish_ind.QoSclass);
    rb->state=NAS_RB_DCH;
    cx->state=NAS_CX_DCH;
    rb->dscp = p->nasUEDCPrimitive.rb_establish_ind.dscp;
    rb->sapi = p->nasUEDCPrimitive.rb_establish_ind.sapId;
    rb->countimer=1;
#ifdef NAS_DEBUG_DC
    printk(" nasmt_ASCTL_DC_decode_rb_establish: RB_ESTABLISH_IND reception\n");
    printk(" Local Connection reference %u\n",p->nasUEDCPrimitive.rb_establish_ind.localConnectionRef);
    printk(" Radio Bearer Identity %u \n",p->nasUEDCPrimitive.rb_establish_ind.rbId);
    printk(" QoS Traffic Class %u\n",p->nasUEDCPrimitive.rb_establish_ind.QoSclass);
    printk(" DSCP Code %u\n",p->nasUEDCPrimitive.rb_establish_ind.dscp);
    printk(" SAP Id %u\n",p->nasUEDCPrimitive.rb_establish_ind.sapId);
    nasmt_TOOL_print_state(cx->state);
    nasmt_TOOL_print_rb_entity(rb);
#endif
  } else
    printk("NAS_MT_DC_DECODE_RB_ESTABLISH_IND: RB_ESTABLISH_IND reception, Radio bearer already opened\n");
}

//---------------------------------------------------------------------------
// Decode RB_RELEASE_IND message from RRC
void nasmt_ASCTL_DC_decode_rb_release_ind(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------
  struct rb_entity *rb;
  uint8_t dscp;
  int hard_coded_rbId = 3;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_decode_rb_release - begin \n");
#endif

  if (!cx || !p) {
    printk("nasmt_ASCTL_DC_decode_rb_release - input parameter is NULL \n");
    return;
  }

  // End debug information
  // TEMP -MW - 26/9/13- Overwrite the rb_id sent by RRC
  p->nasUEDCPrimitive.rb_release_ind.rbId = hard_coded_rbId;
  // TEMP -MW - 26/9/13- Rest should be kept unchanged
  rb=nasmt_COMMON_search_rb(cx, p->nasUEDCPrimitive.rb_release_ind.rbId);

  if (rb!=NULL) {
#ifdef NAS_DEBUG_DC
    printk(" nasmt_ASCTL_DC_decode_rb_release : RB_RELEASE_IND reception\n");
    printk(" Local Connection reference %u\n",p->nasUEDCPrimitive.rb_release_ind.localConnectionRef);
    printk(" Radio Bearer Identity %u\n",p->nasUEDCPrimitive.rb_release_ind.rbId);
    nasmt_TOOL_print_state(cx->state);
#endif
    // rb->state=NAS_IDLE;
    dscp = rb->dscp;
    nasmt_COMMON_del_rb(cx, p->nasUEDCPrimitive.rb_release_ind.rbId, dscp);
  } else
    printk("nasmt_ASCTL_DC_decode_rb_release: RB_RELEASE_IND reception, No corresponding radio bearer\n");

}
//---------------------------------------------------------------------------
// Decode MEASUREMENT_IND message from RRC
void nasmt_ASCTL_DC_decode_measurement_ind(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------
  uint8_t i;
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_decode_measurement - begin \n");
#endif

  if (cx==NULL) {
    printk("nasmt_ASCTL_DC_decode_measurement - input parameter cx is NULL \n");
    return;
  }

  if (p==NULL) {
    printk("nasmt_ASCTL_DC_decode_measurement - input parameter p is NULL \n");
    return;
  }

  // End debug information
#ifdef NAS_DEBUG_DC_MEASURE
  printk(" nasmt_ASCTL_DC_decode_measurement : MEASUREMENT_IND reception\n");
  printk(" Local Connection reference: %u\n", p->nasUEDCPrimitive.measurement_ind.localConnectionRef);
  printk(" Number of RGs: %u\n", p->nasUEDCPrimitive.measurement_ind.nb_rg);
  nasmt_TOOL_print_state(cx->state);

  for (i=0; i<p->nasUEDCPrimitive.measurement_ind.nb_rg; ++i) {
    printk(" RG[%u]:  Cell_Id %u, Level: %u\n", i,
           p->nasUEDCPrimitive.measurement_ind.measures[i].cell_id,
           p->nasUEDCPrimitive.measurement_ind.measures[i].level);
  }

#endif
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_decode_measurement - Local cell %d\n",p->nasUEDCPrimitive.measurement_ind.measures[0].cell_id);
#endif
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
// Decode MBMS_UE_NOTIFY_IND message from RRC
void nasmt_ASCTL_DC_decode_mbms_ue_notify_ind(struct cx_entity *cx, struct nas_ue_dc_element *p)
{
  //---------------------------------------------------------------------------
  uint8_t i, j, k;
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasmt_ASCTL_DC_decode_mbms_ue_notify - begin \n");
#endif

  if (!cx || !p) {
    printk("nasmt_ASCTL_DC_decode_mbms_ue_notify - input parameter is NULL \n");
    return;
  }

  for (i = 0; i<NASMT_MBMS_SVCES_MAX; i++) {
    if (p->nasUEDCPrimitive.mbms_ue_notify_ind.joined_services[i].mbms_serviceId >=0) {
      for (j = 0; j<NASMT_MBMS_SVCES_MAX; j++) {
        if (cx->joined_services[j] ==-1) {
          cx->joined_services[j]= p->nasUEDCPrimitive.mbms_ue_notify_ind.joined_services[i].mbms_serviceId;
          break;
        }
      }
    }

    if (p->nasUEDCPrimitive.mbms_ue_notify_ind.left_services[i].mbms_serviceId >=0) {
      for (k = 0; k<NASMT_MBMS_SVCES_MAX; k++) {
        if (cx->joined_services[k] == p->nasUEDCPrimitive.mbms_ue_notify_ind.left_services[i].mbms_serviceId) {
          cx->joined_services[k]=-1;
          break;
        }
      }
    }
  }

  // End debug information
#ifdef NAS_DEBUG_DC
  printk(" nasmt_ASCTL_DC_decode_mbms_ue_notify : MBMS_UE_NOTIFY_IND reception\n");
  printk(" Local Connection reference: %u\n", p->nasUEDCPrimitive.mbms_ue_notify_ind.localConnectionRef);
  nasmt_TOOL_print_state(cx->state);
  printk("Joined services: ");

  for (i = 0; i<MAX_MBMS_SERVICES && (int) (p->nasUEDCPrimitive.mbms_ue_notify_ind.joined_services[i].mbms_serviceId) >= 0; i++)
    printk("%d    ", (p->nasUEDCPrimitive.mbms_ue_notify_ind.joined_services[i].mbms_serviceId));

  printk("\n");
  printk("Left services: ");

  for (i = 0; i<MAX_MBMS_SERVICES && (int) (p->nasUEDCPrimitive.mbms_ue_notify_ind.left_services[i].mbms_serviceId) >= 0; i++)
    printk("%d    ", (p->nasUEDCPrimitive.mbms_ue_notify_ind.left_services[i].mbms_serviceId));

  printk("\n");
#endif
}

//---------------------------------------------------------------------------
// Check if anything in DC FIFO and decode it (MT)
int nasmt_ASCTL_DC_receive(struct cx_entity *cx, char *buffer)
{
  //---------------------------------------------------------------------------
  int bytes_read=0;

  // Start debug information
#ifdef NAS_DEBUG_DC_DETAIL
  printk("nasmt_ASCTL_DC_receive - begin \n");
#endif

  if (!cx) {
    printk("nasmt_ASCTL_DC_receive - input parameter cx is NULL \n");
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information

#ifndef PDCP_USE_NETLINK
  bytes_read = rtf_get(cx->sap[NAS_DC_OUTPUT_SAPI] , gpriv->rbuffer, NAS_TL_SIZE);
#else
  bytes_read = NAS_TL_SIZE;
#endif

  //
  if (bytes_read>0) {
    struct nas_ue_dc_element *p;

#ifndef PDCP_USE_NETLINK
    p= (struct nas_ue_dc_element *)(gpriv->rbuffer);
    //get the rest of the primitive
    bytes_read += rtf_get(cx->sap[NAS_DC_OUTPUT_SAPI], (uint8_t *)p+NAS_TL_SIZE, p->length-NAS_TL_SIZE);

    if (bytes_read!=p->length) {
      printk("nasmt_ASCTL_DC_receive: Problem while reading primitive header\n");
      return bytes_read;
    }

#else
    p= (struct nas_ue_dc_element *)(buffer);
#endif

    switch (p->type) {
    case CONN_ESTABLISH_RESP :
      if (p->nasUEDCPrimitive.conn_establish_resp.localConnectionRef!=cx->lcr)
        printk("nasmt_ASCTL_DC_receive: CONN_ESTABLISH_RESP, Local connection reference not correct %u\n",p->nasUEDCPrimitive.conn_establish_resp.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_CONNECTING:
          nasmt_ASCTL_DC_decode_cx_establish_resp(cx,p);   // process message
          break;

        default:
          printk("nasmt_ASCTL_DC_receive: CONN_ESTABLISH_RESP reception, Invalid state %u\n", cx->state);
        }
      }

      break;

    case CONN_LOSS_IND :
      if (p->nasUEDCPrimitive.conn_loss_ind.localConnectionRef!=cx->lcr)
        printk("nasmt_ASCTL_DC_receive: CONN_LOSS_IND reception, Local connection reference not correct %u\n", p->nasUEDCPrimitive.conn_loss_ind.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_RELEASING_FAILURE:
          cx->retry=0;

        case NAS_CX_DCH:
          nasmt_ASCTL_DC_decode_cx_loss_ind(cx,p);   // process message
          break;

        default:
          printk("nasmt_ASCTL_DC_receive: CONN_LOSS_IND reception, Invalid state %u", cx->state);
        }
      }

      break;

      //    case CONN_RELEASE_IND :
      //      break;
    case DATA_TRANSFER_IND :
      if (p->nasUEDCPrimitive.data_transfer_ind.localConnectionRef!=cx->lcr)
        printk("nasmt_ASCTL_DC_receive: DATA_TRANSFER_IND reception, Local connection reference not correct %u\n", p->nasUEDCPrimitive.conn_loss_ind.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_FACH:
        case NAS_CX_DCH:
          nasmt_ASCTL_DC_decode_sig_data_ind(cx,p);   // process message
          break;

        default:
          printk("nasmt_ASCTL_DC_receive: DATA_TRANSFER_IND reception, Invalid state %u", cx->state);
        }
      }

      break;

    case RB_ESTABLISH_IND :
      if (p->nasUEDCPrimitive.rb_establish_ind.localConnectionRef!=cx->lcr)
        printk("nasmt_ASCTL_DC_receive: RB_ESTABLISH_IND reception, Local connexion reference not correct %u\n", p->nasUEDCPrimitive.rb_establish_ind.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_FACH:
        case NAS_CX_DCH:
          nasmt_ASCTL_DC_decode_rb_establish_ind(cx,p);   // process message
          break;

        default:
          printk("nasmt_ASCTL_DC_receive: RB_ESTABLISH_IND reception, Invalid state %u", cx->state);
        }
      }

      break;

    case RB_RELEASE_IND :
      if (p->nasUEDCPrimitive.rb_release_ind.localConnectionRef!=cx->lcr)
        printk("nasmt_ASCTL_DC_receive: RB_RELEASE_IND reception, Local connection reference not correct %u\n", p->nasUEDCPrimitive.rb_release_ind.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_DCH:
          nasmt_ASCTL_DC_decode_rb_release_ind(cx,p);   // process message
          break;

        default:
          printk("nasmt_ASCTL_DC_receive: RB_RELEASE_IND reception, Invalid state %u", cx->state);
        }
      }

      break;

    case MEASUREMENT_IND :
      if (p->nasUEDCPrimitive.measurement_ind.localConnectionRef!=cx->lcr)
        printk("nasmt_ASCTL_DC_receive: MEASUREMENT_IND reception, Local connection reference not correct %u\n", p->nasUEDCPrimitive.measurement_ind.localConnectionRef);
      else {
        nasmt_ASCTL_DC_decode_measurement_ind(cx,p);
      }

      break;

    case MBMS_UE_NOTIFY_IND :
      if (p->nasUEDCPrimitive.rb_release_ind.localConnectionRef!=cx->lcr)
        printk("nasmt_ASCTL_DC_receive: MBMS_UE_NOTIFY_IND reception, Local connection reference not correct %u\n", p->nasUEDCPrimitive.rb_release_ind.localConnectionRef);
      else {
        switch (cx->state) {
        case NAS_CX_DCH:
          nasmt_ASCTL_DC_decode_mbms_ue_notify_ind(cx,p);   // process message
          break;

        default:
          printk("nasmt_ASCTL_DC_receive: MBMS_UE_NOTIFY_IND reception, Invalid state %u", cx->state);
        }
      }

      break;

    default :
      printk("nasmt_ASCTL_DC_receive: Invalid message received\n");
    }
  }

#ifdef NAS_DEBUG_DC_DETAIL
  printk("nasmt_ASCTL_DC_receive - end \n");
#endif
  return bytes_read;
}

//---------------------------------------------------------------------------
// Check if anything in GC FIFO and decode it (MT)
int nasmt_ASCTL_GC_receive(char *buffer)
{
  //---------------------------------------------------------------------------
  int bytes_read = 0;

#ifdef NAS_DEBUG_GC
  printk("nasmt_ASCTL_GC_receive - begin \n");
#endif
  // End debug information

#ifndef PDCP_USE_NETLINK
  bytes_read = rtf_get(gpriv->sap[NAS_GC_SAPI], gpriv->rbuffer, NAS_TL_SIZE);
#else
  bytes_read = NAS_TL_SIZE;
#endif

  //
  if (bytes_read>0) {
    struct nas_ue_gc_element *p;
#ifndef PDCP_USE_NETLINK
    p= (struct nas_ue_gc_element *)(gpriv->rbuffer);
    //get the rest of the primitive
    bytes_read += rtf_get(gpriv->sap[NAS_GC_SAPI], (uint8_t *)p+NAS_TL_SIZE, p->length-NAS_TL_SIZE);

    if (bytes_read!=p->length) {
      printk("nasmt_ASCTL_GC_receive: Problem while reading primitive's header\n");
      return bytes_read;
    }

#else
    p= (struct nas_ue_gc_element *)(buffer);
    bytes_read = p->length;
#endif

    // start decoding message
    switch (p->type) {
    case INFO_BROADCAST_IND :
#ifndef PDCP_USE_NETLINK
      bytes_read += rtf_get(gpriv->sap[NAS_GC_SAPI], (uint8_t *)p+p->length, p->nasUEGCPrimitive.broadcast_ind.nasDataLength);

      if (bytes_read!=p->length+p->nasUEGCPrimitive.broadcast_ind.nasDataLength) {
        printk("nasmt_ASCTL_GC_receive: INFO_BROADCAST_IND reception, Problem while reading primitive's data\n");
        return bytes_read;
      }

#endif
#ifdef NAS_DEBUG_GC
      printk(" nasmt_ASCTL_GC_receive : INFO_BROADCAST_IND reception\n");
      printk(" Primitive length %d \n", (int)(p->type));
      printk(" Data length %u\n", p->nasUEGCPrimitive.broadcast_ind.nasDataLength);
      printk(" Data string %s\n", (uint8_t *)p+p->length);
#endif
      return bytes_read;

    default :
      printk("nasmt_ASCTL_GC_receive: Invalid message received, type %d\n", p->type);
      nasmt_TOOL_print_buffer(buffer, 16);
      return -1;
    }
  } else
    return -1;
}

#endif
