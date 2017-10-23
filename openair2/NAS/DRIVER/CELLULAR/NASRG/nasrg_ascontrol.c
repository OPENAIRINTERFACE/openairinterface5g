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

/*! \file nasrg_ascontrol.c
* \brief Access Stratum Control functions for OpenAirInterface CELLULAR version - RG
* \author  michelle.wetterwald, navid.nikaein, raymond.knopp, Lionel Gauthier
* \company Eurecom
* \email: michelle.wetterwald@eurecom.fr, raymond.knopp@eurecom.fr, navid.nikaein@eurecom.fr,  lionel.gauthier@eurecom.fr
*/
/*******************************************************************************/
#ifdef NODE_RG
#include "nasrg_variables.h"
#include "nasrg_proto.h"

//---------------------------------------------------------------------------
void nasrg_ASCTL_init(void)
{
  //---------------------------------------------------------------------------
  int cxi, i;

  gpriv->next_sclassref = NASRG_DEFAULTRAB_CLASSREF;

  // Initialize MBMS services list in MT
  for (cxi=0; cxi<NAS_CX_MAX ; ++cxi) {
    for (i = 0; i<NASRG_MBMS_SVCES_MAX; i++) {
      gpriv->cx[cxi].requested_joined_services[i]= -1;
      gpriv->cx[cxi].requested_left_services[i]= -1;
      gpriv->cx[cxi].joined_services[i]= -1;
    }
  }

  printk("nasrg_ASCTL_init Complete\n");
}

//---------------------------------------------------------------------------
int nasrg_ASCTL_write(int sap, unsigned char *data_buffer, unsigned int data_length)
{
  //---------------------------------------------------------------------------
  int bytes_wrote = 0;
#ifdef PDCP_USE_NETLINK
  unsigned char xmit_buffer [NAS_MESSAGE_MAXLEN];

  //write SAP_Id
  switch (sap) {
  case RRC_DEVICE_GC:
    xmit_buffer[0] = RRC_NAS_GC_IN;
    break;

  case RRC_DEVICE_NT:
    xmit_buffer[0] = RRC_NAS_NT_IN;
    break;

  case RRC_DEVICE_DC_INPUT0:
    xmit_buffer[0] = RRC_NAS_DC0_IN;
    break;

  case RRC_DEVICE_DC_INPUT1:
    xmit_buffer[0] = RRC_NAS_DC1_IN;
    //case RRC_DEVICE_DC_INPUT2:
    //xmit_buffer[0] = RRC_NAS_DC2_IN;
    break;

  default:
    printk("nasrg_ASCTL_write - ERROR - Invalid SAP %d\n", sap);
    return 0;
    break;
  }

  memcpy (&((char*)xmit_buffer)[1], data_buffer, data_length);
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_write - Sending data to RRC, sap %d, length %d (including NETLINK SAP value)\n", sap, data_length+1);
  nasrg_TOOL_print_buffer(xmit_buffer,data_length+1);
#endif
  bytes_wrote = nasrg_netlink_send(xmit_buffer,data_length+1, NASNL_DEST_RRC);
#else
  //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);  //original version
  bytes_wrote = rtf_put(sap, data_buffer, data_length+1);
#endif //PDCP_USE_NETLINK
  return bytes_wrote-1;
}

//---------------------------------------------------------------------------
//For demo, add automatically a radio bearer
//Equivalent to rb add send 0 5 2
void nasrg_ASCTL_start_default_rb(struct cx_entity *cx)
{
  //---------------------------------------------------------------------------
#ifdef DEMO_3GSM
  struct rb_entity *rb;
  int status;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_start_default_rb - begin \n");
#endif

  if (!cx) {
    printk("nasrg_ASCTL_start_default_rb - input parameter cx is NULL \n");
    return;
  }

  // End debug information

  rb = nasrg_COMMON_add_rb(cx, NASRG_DEFAULTRAB_RBID, NASRG_DEFAULTRAB_QoS);

  if (rb!=NULL) {
    rb->cnxid = (NAS_RB_MAX_NUM * cx->lcr)+1;
    //rb->default_rab =1;
    rb->dscp = NASRG_DEFAULTRAB_DSCP;
    rb->dscp_ul = NASRG_DEFAULTRAB_DSCP;
    status=nasrg_ASCTL_DC_send_rb_establish_request(cx, rb);
  } else
    status=-NAS_ERROR_NOMEMORY;

#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_start_default_rb - end %d \n",status);
#endif
}
#endif

//---------------------------------------------------------------------------
//For demo, add automatically a classifier
//Equivalent to class add send 0 -f qos <x> -cr 0
void nasrg_ASCTL_start_default_sclassifier(struct cx_entity *cx,struct rb_entity *rb)
{
  //---------------------------------------------------------------------------
  struct classifier_entity *gc;

  // Start debug information
#ifdef NAS_DEBUG_CLASS
  printk("nasrg_ASCTL_start_default_sclassifier - begin \n");
#endif

  if (!cx || !rb) {
    printk("nasrg_ASCTL_start_default_sclassifier - input parameter is NULL \n");
    return;
  }

  // End debug information
  gc=nasrg_CLASS_add_sclassifier(cx, NAS_DSCP_DEFAULT, gpriv->next_sclassref);

  if (gc==NULL) {
    printk("nasrg_ASCTL_start_default_sclassifier - Classifier pointer is null : not added \n");
    return;
  }

  gc->fct = nasrg_COMMON_QOS_send;
  gc->rab_id =rb->rab_id;
  gc->rb= rb;
  gc->version = NASRG_DEFAULTRAB_IPVERSION;
  gc->protocol= NAS_PROTOCOL_DEFAULT;
  gc->dplen= NAS_DEFAULT_IPv6_PREFIX_LENGTH;
  // copy locator part of address into the gc structure
  //gc->daddr.ipv6.s6_addr32[0] = 0;
  //gc->daddr.ipv6.s6_addr32[1] = 0;
  //gc->daddr.ipv6.s6_addr32[2] = 0;
  //gc->daddr.ipv6.s6_addr32[3] = 0;

#ifdef NAS_DEBUG_CLASS
  printk("nasrg_ASCTL_start_default_sclassifier - end \n");
  nasrg_TOOL_print_classifier(gc);
#endif
}

//---------------------------------------------------------------------------
//Add automatically a classifier on DSCP
//Equivalent to class add send 0 -f qos <x> -cr 0
void nasrg_ASCTL_start_sclassifier(struct cx_entity *cx,struct rb_entity *rb)
{
  //---------------------------------------------------------------------------
  struct classifier_entity *gc;

  // Start debug information
#ifdef NAS_DEBUG_CLASS
  printk("nasrg_ASCTL_start_sclassifier - begin \n");
#endif

  if (!cx || !rb) {
    printk("nasrg_ASCTL_start_sclassifier - input parameter is NULL \n");
    return;
  }

  // End debug information
  gc=nasrg_CLASS_add_sclassifier(cx, rb->dscp, gpriv->next_sclassref);

  if (gc==NULL) {
    printk("nasrg_ASCTL_start_sclassifier - Classifier pointer is null : not added \n");
    return;
  }

  gc->fct = nasrg_COMMON_QOS_send;
  gc->rab_id =rb->rab_id;
  gc->rb= rb;
  gc->version = NASRG_DEFAULTRAB_IPVERSION;
  gc->protocol= NAS_PROTOCOL_DEFAULT;
  gc->dplen= NAS_DEFAULT_IPv6_PREFIX_LENGTH;
  // copy locator part of address into the gc structure
  gc->daddr.ipv6.s6_addr32[2] = cx->iid6[0];
  gc->daddr.ipv6.s6_addr32[3] = cx->iid6[1];

#ifdef NAS_DEBUG_CLASS
  printk("nasrg_ASCTL_start_sclassifier - end \n");
  nasrg_TOOL_print_classifier(gc);
#endif
}

//---------------------------------------------------------------------------
//Add automatically a classifier for mbms
void nasrg_ASCTL_start_mbmsclassifier(int mbms_ix,struct rb_entity *mbms_rb)
{
  //---------------------------------------------------------------------------
  struct classifier_entity *gc;

  // Start debug information
#ifdef NAS_DEBUG_CLASS
  printk("nasrg_ASCTL_start_mbmsclassifier - begin \n");
#endif

  if (!mbms_rb) {
    printk("nasrg_ASCTL_start_mbmsclassifier - input parameter is NULL \n");
    return;
  }

  // End debug information
  //
  //  gc=nasrg_CLASS_add_mbmsclassifier(mbms_ix, gpriv->next_mbmsclassref);
  gc=nasrg_CLASS_add_mbmsclassifier(mbms_ix, gpriv->next_sclassref++);

  if (gc==NULL) {
    printk("nasrg_ASCTL_start_mbmsclassifier - Classifier pointer is null : not added \n");
    return;
  }

  gc->fct = nasrg_COMMON_QOS_send;
  gc->rab_id =mbms_rb->mbms_rbId;
  gc->rb= mbms_rb;
  gc->version = NASRG_DEFAULTRAB_IPVERSION;
  gc->protocol= NAS_PROTOCOL_DEFAULT;
#ifdef NAS_DEBUG_CLASS
  printk("nasrg_ASCTL_start_mbmsclassifier - end \n");
  nasrg_TOOL_print_classifier(gc);
#endif
}

//---------------------------------------------------------------------------
//Notifies automatically UE on default MBMS service
void nasrg_ASCTL_start_default_ue_notification(struct cx_entity *cx)
{
  //---------------------------------------------------------------------------
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_start_default_ue_notification - begin \n");
#endif

  if (!cx) {
    printk("\nERROR nasrg_ASCTL_start_default_ue_notification - input parameter is NULL \n");
    return;
  }

  // End debug information
  // set value for default MBMS service
  cx->requested_joined_services[0] = (NAS_CX_MAX*NAS_RB_MAX_NUM) +1;
  nasrg_ASCTL_DC_send_mbms_ue_notify_req(cx);
}

//---------------------------------------------------------------------------
//Notifies automatically UE on default MBMS service
void nasrg_ASCTL_start_default_mbms_service(void)
{
  //---------------------------------------------------------------------------
  int mbms_ix=0; // should allocate index based on Service_id /cnxid / MC IP address
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_start_default_mbms_service - begin \n");
#endif

  // Establish MBMS bearer if not already done
  if ((gpriv->mbms_rb[mbms_ix].state != NAS_CX_DCH)&&(gpriv->mbms_rb[mbms_ix].state != NAS_RB_ESTABLISHING)) {
    gpriv->mbms_rb[mbms_ix].cnxid = (NAS_CX_MAX*NAS_RB_MAX_NUM) +1;
    gpriv->mbms_rb[mbms_ix].serviceId = (NAS_CX_MAX*NAS_RB_MAX_NUM) +1;
    gpriv->mbms_rb[mbms_ix].sessionId = NASRG_TEMP_MBMS_SESSION_ID; //Temp hard coded
    gpriv->mbms_rb[mbms_ix].mbms_rbId = NASRG_DEFAULTRAB_RBID;
    gpriv->mbms_rb[mbms_ix].sapi = NAS_DC_INPUT_SAPI;
#ifdef NAS_DEBUG_MBMS_PROT
    gpriv->mbms_rb[mbms_ix].sapi = NAS_DRB_INPUT_SAPI; //Only one RT-FIFO is used
#endif
    gpriv->mbms_rb[mbms_ix].qos = 20; //from first version of MBMS traces
    gpriv->mbms_rb[mbms_ix].dscp = 0; //maybe 7
    gpriv->mbms_rb[mbms_ix].duration = NASRG_TEMP_MBMS_DURATION; //Temp hard coded
    //memcpy ((char *)&(gpriv->mbms_rb[mbms_ix].mcast_address),(char *)&(msgreq->mcast_group), 16);
    gpriv->mbms_rb[mbms_ix].mcast_address[0] = 0xFF;
    gpriv->mbms_rb[mbms_ix].mcast_address[1] = 0x0E;
    nasrg_ASCTL_GC_send_mbms_bearer_establish_req(mbms_ix);
  } else {
#ifdef NAS_DEBUG_DC
    printk("nasrg_ASCTL_start_default_mbms_service - skipped, mbms_rb.state = %d \n", gpriv->mbms_rb[mbms_ix].state);
#endif
  }
}
//---------------------------------------------------------------------------
void nasrg_ASCTL_timer(unsigned long data)
{
  //---------------------------------------------------------------------------
  uint8_t cxi;
  struct cx_entity *cx;
  struct rb_entity *rb;
  spin_lock(&gpriv->lock);
#ifdef NAS_DEBUG_TIMER
  printk("nasrg_ASCTL_timer - begin \n");
#endif
  (gpriv->timer).function = nasrg_ASCTL_timer;
  (gpriv->timer).expires=jiffies+NAS_TIMER_TICK;
  (gpriv->timer).data=0L;

  for (cxi=0; cxi<NAS_CX_MAX ; ++cxi) {
    cx=gpriv->cx+cxi;

    if (!cx) {
      printk("nasrg_ASCTL_timer - No pointer for connection %d \n", cxi);
      continue;
    }

    for (rb=cx->rb; rb!=NULL; rb=rb->next) {
      if (rb->countimer!=NAS_TIMER_IDLE) {
#ifdef NAS_DEBUG_TIMER
        printk("nasrg_ASCTL_timer : rb countimer %d, rb state %d\n", rb->countimer, rb->state);
#endif

        if (rb->countimer==0) {
          switch (rb->state) {
          case NAS_CX_CONNECTING:
          case NAS_CX_CONNECTING_FAILURE:  // MW - 15/01/07 Useless, currently no retry if failure
            if (rb->retry<gpriv->retry_limit) {
              printk("nasrg_ASCTL_timer: Retry RB establishment %d\n", rb->retry);
              nasrg_ASCTL_DC_send_rb_establish_request(cx, rb);
            } else {
              printk("nasrg_ASCTL_timer: RB Establishment failure\n");
              rb->state=NAS_IDLE;
              rb->countimer=NAS_TIMER_IDLE;
            }

            break;

          case NAS_CX_DCH:
#ifdef DEMO_3GSM
            if (cx->num_rb == 1) {
              nasrg_ASCTL_start_default_sclassifier(cx, rb);
            }

#endif
            nasrg_ASCTL_start_sclassifier(cx, rb);
            rb->countimer=NAS_TIMER_IDLE;
            break;

          case NAS_CX_RELEASING_FAILURE:
            nasrg_ASCTL_DC_send_rb_release_request(cx, rb);
            break;

          default:
            rb->countimer=NAS_TIMER_IDLE;
          }
        } else {
          --rb->countimer;
          printk("nasrg_ASCTL_timer : rb countimer-- %d, rb state %d\n", rb->countimer, rb->state);
        }
      }
    }
  }

  add_timer(&gpriv->timer);
#ifdef NAS_DEBUG_TIMER
  printk("nasrg_ASCTL_timer - end \n");
#endif
  spin_unlock(&gpriv->lock);
}


/***************************************************************************
     Transmission side
 ***************************************************************************/
//---------------------------------------------------------------------------
// Encode INFO_BROADCAST_REQ message
int nasrg_ASCTL_GC_send_broadcast_request(uint8_t category)
{
  //---------------------------------------------------------------------------
  char *xmit_data = "TESTING BROADCASTING ROUTER ADVERTISEMENT. TESTING BROADCASTING ROUTER ADVERTISEMENT. BROADCASTING ROUTER.\0";
  int bytes_wrote = 0;
#ifdef PDCP_USE_NETLINK
  unsigned char xbuffer[NAS_MESSAGE_MAXLEN];
  int count=0;
#endif

  struct nas_rg_gc_element *p;
  p= (struct nas_rg_gc_element *)(gpriv->xbuffer);
  p->type = INFO_BROADCAST_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASInfoBroadcastReq);
  //
  p->nasRGGCPrimitive.broadcast_req.period = 0;
  p->nasRGGCPrimitive.broadcast_req.category = category;
  p->nasRGGCPrimitive.broadcast_req.nasDataLength = strlen(xmit_data)+1;  // TBD

  //bytes_wrote = rtf_put(gpriv->sap[NAS_GC_SAPI], p, p->length); //original version
#ifdef PDCP_USE_NETLINK
  memcpy(xbuffer,(unsigned char *)p, p->length);
  count = p->length;
  bytes_wrote = count;
#else
  bytes_wrote = nasrg_ASCTL_write(gpriv->sap[NAS_GC_SAPI], (unsigned char *)p, p->length);
#endif
  //printk("nasrg_ASCTL_GC_send_broadcast_request - Wrote %d bytes to RRC NAS_GC_SAPII\n", bytes_wrote);

  //bytes_wrote += rtf_put(gpriv->sap[NAS_GC_SAPI], xmit_data, p->nasRGGCPrimitive.broadcast_req.nasDataLength);
#ifdef PDCP_USE_NETLINK
  memcpy(&(xbuffer[count]),(unsigned char *)xmit_data, p->nasRGGCPrimitive.broadcast_req.nasDataLength);
  count += p->nasRGGCPrimitive.broadcast_req.nasDataLength;
  bytes_wrote += nasrg_ASCTL_write(gpriv->sap[NAS_GC_SAPI], xbuffer, count);
#else
  bytes_wrote += nasrg_ASCTL_write((gpriv->sap[NAS_GC_SAPI], xmit_data, p->nasRGGCPrimitive.broadcast_req.nasDataLength);
#endif

  if (bytes_wrote==p->length+p->nasRGGCPrimitive.broadcast_req.nasDataLength) {
#ifdef NAS_DEBUG_GC
    printk("nasrg_ASCTL_GC_send_broadcast: INFO_BROADCAST_REQ primitive sent successfully in GC-FIFO\n");
#endif
  } else {
    printk("nasrg_ASCTL_GC_send_broadcast: Message sent failure in GC-FIFO\n");
  }
  return bytes_wrote;
}


//---------------------------------------------------------------------------
// Encode INFO_BROADCAST_REQ message for RRC SIB1
int nasrg_ASCTL_GC_send_SIB1_broadcast_request(struct sk_buff *skb)
{
  //---------------------------------------------------------------------------
  struct nas_rg_gc_element *p;
  char sib1_flag; // will be used for reception in nas_ue
  int bytes_wrote = 0;
#ifdef PDCP_USE_NETLINK
  unsigned char xbuffer[NAS_MESSAGE_MAXLEN];
  int count=0;
#endif

  // Start debug information
#ifdef NAS_DEBUG_GC
  printk("nasrg_ASCTL_GC_send_SIB1_broadcast_request - begin \n");
#endif

  if (!skb) {
    printk("nasrg_ASCTL_GC_send_SIB1_broadcast_request - input parameter is NULL \n");
    return 0;
  }

  // End debug information
  p= (struct nas_rg_gc_element *)(gpriv->xbuffer);
  p->type = INFO_BROADCAST_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASInfoBroadcastReq);
  //
  p->nasRGGCPrimitive.broadcast_req.period = 10; // to be checked
  p->nasRGGCPrimitive.broadcast_req.category = 1;
  p->nasRGGCPrimitive.broadcast_req.nasDataLength = skb->len+1;  // TBD
  sib1_flag = 1;
  // send header
  //bytes_wrote = rtf_put(gpriv->sap[NAS_GC_SAPI], p, p->length); //original version
#ifdef PDCP_USE_NETLINK
  memcpy(xbuffer,(unsigned char *)p, p->length);
  count = p->length;
  bytes_wrote = count;
#else
  bytes_wrote = nasrg_ASCTL_write(gpriv->sap[NAS_GC_SAPI], (unsigned char *)p, p->length);
#endif
  //printk("nasrg_ASCTL_GC_send_broadcast_request - Wrote %d bytes to RRC NAS_GC_SAPII\n", bytes_wrote);


  if (bytes_wrote!=p->length) {
    printk("nasrg_ASCTL_GC_send_SIB1_broadcast_request: Header send failure in GC-FIFO\n");
    return bytes_wrote;
  }

  // send sib1_flag
  //bytes_wrote +=  rtf_put(gpriv->sap[NAS_GC_SAPI], &sib1_flag, 1);
#ifdef PDCP_USE_NETLINK
  memcpy(&(xbuffer[count]),(unsigned char *)&sib1_flag, 1);
  count += 1;
  bytes_wrote = count;
#else
  bytes_wrote += nasrg_ASCTL_write(gpriv->sap[NAS_GC_SAPI], (unsigned char *)&sib1_flag, 1);
#endif

  if (bytes_wrote!=p->length+1) {
    printk("nasrg_ASCTL_GC_send_SIB1_broadcast_request: sib1_flag send failure in GC-FIFO\n");
    return bytes_wrote;
  }

  // send data
  //bytes_wrote += rtf_put(gpriv->sap[NAS_GC_SAPI], skb->data, skb->len);
#ifdef PDCP_USE_NETLINK
  memcpy(&(xbuffer[count]),skb->data, skb->len);
  count += skb->len;
  bytes_wrote = nasrg_ASCTL_write(gpriv->sap[NAS_GC_SAPI], xbuffer, count);
#else
  bytes_wrote += nasrg_ASCTL_write(gpriv->sap[NAS_GC_SAPI], skb->data, skb->len);
#endif


  if (bytes_wrote!=p->length+skb->len+1) {
    printk("nasrg_ASCTL_GC_send_SIB1_broadcast_request: Data send failure in GC-FIFO\n");
    return bytes_wrote;
  }

#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_GC_send_SIB1_broadcast_request - end \n");
#endif
  return bytes_wrote;
}

//-----------------------------------------------------------------------------
// Encode MBMS_BEARER_ESTABLISH_REQ message
int nasrg_ASCTL_GC_send_mbms_bearer_establish_req(int mbms_ix )
{
  //-----------------------------------------------------------------------------
  struct nas_rg_gc_element *p;
  int bytes_wrote = 0;

  p= (struct nas_rg_gc_element *)(gpriv->xbuffer);
  p->type = MBMS_BEARER_ESTABLISH_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASMBMSBearerEstablishReq);
  //
  p->nasRGGCPrimitive.mbms_establish_req.mbms_serviceId = gpriv->mbms_rb[mbms_ix].serviceId;
  p->nasRGGCPrimitive.mbms_establish_req.mbms_sessionId = gpriv->mbms_rb[mbms_ix].sessionId;
  p->nasRGGCPrimitive.mbms_establish_req.mbms_rbId = gpriv->mbms_rb[mbms_ix].mbms_rbId;
  p->nasRGGCPrimitive.mbms_establish_req.mbms_sapId = gpriv->mbms_rb[mbms_ix].sapi;
  p->nasRGGCPrimitive.mbms_establish_req.mbms_QoSclass = gpriv->mbms_rb[mbms_ix].qos;
  p->nasRGGCPrimitive.mbms_establish_req.mbms_duration = gpriv->mbms_rb[mbms_ix].duration;
  gpriv->mbms_rb[mbms_ix].state = NAS_RB_ESTABLISHING;
  //
  //bytes_wrote = rtf_put(gpriv->sap[NAS_GC_SAPI], p, p->length);
  bytes_wrote = nasrg_ASCTL_write(gpriv->sap[NAS_GC_SAPI], (unsigned char *)p, p->length);

  if (bytes_wrote==p->length) {
#ifdef NAS_DEBUG_GC
    printk(" nasrg_ASCTL_GC_send_mbms_bearer_establish: MBMS_BEARER_ESTABLISH_REQ primitive sent successfully in GC-FIFO\n");
    printk(" ServiceId %d, RB_Id %d , qos class %d \n",
           p->nasRGGCPrimitive.mbms_establish_req.mbms_serviceId,
           p->nasRGGCPrimitive.mbms_establish_req.mbms_rbId,
           p->nasRGGCPrimitive.mbms_establish_req.mbms_QoSclass);
#endif
  } else {
    printk("nasrg_ASCTL_GC_send_mbms_bearer_establish: Message sent failure in GC-FIFO\n");
  }

  return bytes_wrote;
}

//-----------------------------------------------------------------------------
// HNN - Encode MBMS_BEARER_RELEASE_REQ message
int nasrg_ASCTL_GC_send_mbms_bearer_release_req(int mbms_ix)
{
  //-----------------------------------------------------------------------------
  struct nas_rg_gc_element *p;
  uint16_t classref=0;
  int bytes_wrote = 0;

  p= (struct nas_rg_gc_element *)(gpriv->xbuffer);

  p->type = MBMS_BEARER_RELEASE_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASMBMSBearerReleaseReq);
  //
  p->nasRGGCPrimitive.mbms_release_req.mbms_serviceId = gpriv->mbms_rb[mbms_ix].serviceId;
  p->nasRGGCPrimitive.mbms_release_req.mbms_sessionId = gpriv->mbms_rb[mbms_ix].sessionId;
  p->nasRGGCPrimitive.mbms_release_req.mbms_rbId = gpriv->mbms_rb[mbms_ix].mbms_rbId;
  //gpriv->mbms_rb[mbms_ix].state = NAS_RB_RELEASING;
  //
  //bytes_wrote = rtf_put(gpriv->sap[NAS_GC_SAPI], p, p->length);
  bytes_wrote = nasrg_ASCTL_write(gpriv->sap[NAS_GC_SAPI], (unsigned char *)p, p->length);

  if (bytes_wrote==p->length) {
#ifdef NAS_DEBUG_GC
    printk(" nasrg_ASCTL_GC_send_mbms_bearer_release: MBMS_BEARER_RELEASE_REQ primitive sent successfully in GC-FIFO\n");
    printk(" ServiceId %d, RB_Id %d \n", p->nasRGGCPrimitive.mbms_establish_req.mbms_serviceId, p->nasRGGCPrimitive.mbms_establish_req.mbms_rbId);
#endif
    // clean NASRG private structures
    classref =  (gpriv->mbmsclassifier[mbms_ix])->classref;
    nasrg_CLASS_del_mbmsclassifier(mbms_ix, classref);
    //nasrg_CLASS_flush_mbmsclassifier();
    //gpriv->mbms_rb[mbms_ix].state = NAS_IDLE;
    memset (&(gpriv->mbms_rb[mbms_ix]),0,sizeof (struct rb_entity));
  } else {
    printk("nasrg_ASCTL_GC_send_mbms_bearer_release: Message sent failure in GC-FIFO\n");
  }

  return bytes_wrote;
}

//---------------------------------------------------------------------------
// Confirm the establishment of a connection (DC channel)
int nasrg_ASCTL_DC_send_cx_establish_confirm(struct cx_entity *cx, uint8_t response)
{
  //---------------------------------------------------------------------------
  struct nas_rg_dc_element *p;
  int bytes_wrote = 0;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_send_cx_establish - begin \n");
#endif

  if (!cx) {
    printk("nasrg_ASCTL_DC_send_cx_establish - input parameter cx is NULL \n");
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information
  p= (struct nas_rg_dc_element *)(gpriv->xbuffer);
  p->type = CONN_ESTABLISH_CNF;
  p->length =  NAS_TL_SIZE + sizeof(struct NASConnEstablishConf);
  p->nasRGDCPrimitive.conn_establish_conf.localConnectionRef = cx->lcr;
  p->nasRGDCPrimitive.conn_establish_conf.status = response;  // can be ACCEPTED  or FAILURE
  p->nasRGDCPrimitive.conn_establish_conf.num_RBs = 0; // Hard coded in first step
  //
  //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);
  bytes_wrote = nasrg_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)p, p->length);

  if (bytes_wrote==p->length) {
#ifdef NAS_DEBUG_DC
    printk("nasrg_ASCTL_DC_send_cx_establish: CONN_ESTABLISH_CNF primitive sent successfully in DC-FIFO\n");
    printk(" lcr (Mobile_id) %u\n",p->nasRGDCPrimitive.conn_establish_conf.localConnectionRef);
    printk(" Status %u\n",p->nasRGDCPrimitive.conn_establish_conf.status);
#endif
  } else
    printk("nasrg_ASCTL_DC_send_cx_establish: Message transmission failure to DC-FIFO\n");

  return bytes_wrote;
}

//---------------------------------------------------------------------------
// Request the establishment of a radio bearer
int nasrg_ASCTL_DC_send_rb_establish_request(struct cx_entity *cx, struct rb_entity *rb)
{
  //---------------------------------------------------------------------------
  struct nas_rg_dc_element *p;
  int bytes_wrote = 0;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_send_rb_establish - begin \n");
#endif

  if (!cx || !rb) {
    printk("nasrg_ASCTL_DC_send_rb_establish - input parameter is NULL \n");
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information
  switch(rb->state) {
  case NAS_CX_CONNECTING:
  case NAS_CX_CONNECTING_FAILURE:
  case NAS_IDLE:
    ++rb->retry;
    rb->countimer=gpriv->timer_establishment;

    if (cx->state==NAS_CX_DCH) {
      p= (struct nas_rg_dc_element *)(gpriv->xbuffer);
      p->type = RB_ESTABLISH_REQ;
      p->length =  NAS_TL_SIZE + sizeof(struct NASrbEstablishReq);
      p->nasRGDCPrimitive.rb_establish_req.localConnectionRef = cx->lcr;
      p->nasRGDCPrimitive.rb_establish_req.rbId = rb->rab_id + (NAS_RB_MAX_NUM * cx->lcr);
      p->nasRGDCPrimitive.rb_establish_req.QoSclass = rb->qos;
      p->nasRGDCPrimitive.rb_establish_req.dscp = rb->dscp_ul;
      //
      //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);
      bytes_wrote = nasrg_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)p, p->length);

      if (bytes_wrote==p->length) {
        rb->state=NAS_CX_CONNECTING;
#ifdef NAS_DEBUG_DC
        printk(" nasrg_ASCTL_DC_send_rb_establish: RB_ESTABLISH_REQ primitive sent successfully in DC-FIFO\n");
        printk(" lcr (Mobile_id) %u\n",p->nasRGDCPrimitive.rb_establish_req.localConnectionRef);
        printk(" Radio Bearer identification %u\n",p->nasRGDCPrimitive.rb_establish_req.rbId);
        printk(" QoS %u\n",p->nasRGDCPrimitive.rb_establish_req.QoSclass);
#endif
      } else {
        rb->state=NAS_CX_CONNECTING_FAILURE;
        printk("nasrg_ASCTL_DC_send_rb_establish: Message sent failure in DC-FIFO\n");
      }

      return bytes_wrote;
    } else {
      rb->state=NAS_CX_CONNECTING_FAILURE;
      printk("nasrg_ASCTL_DC_send_rb_establish: Failure \n");
      return 0;
    }

  default:
    return -NAS_ERROR_NOTIDLE;
  }
}

//---------------------------------------------------------------------------
// Request the release of a radio bearer
int nasrg_ASCTL_DC_send_rb_release_request(struct cx_entity *cx, struct rb_entity *rb)
{
  //---------------------------------------------------------------------------
  struct nas_rg_dc_element *p;
  int bytes_wrote = 0;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_send_rb_release - begin \n");
#endif

  if (!cx || !rb) {
    printk("nasrg_ASCTL_DC_send_rb_release - input parameter is NULL \n");
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information
  switch (rb->state) {
  case NAS_CX_RELEASING_FAILURE:
  case NAS_CX_DCH:
    p= (struct nas_rg_dc_element *)(gpriv->xbuffer);
    p->type = RB_RELEASE_REQ;
    p->length =  NAS_TL_SIZE + sizeof(struct NASrbReleaseReq);
    p->nasRGDCPrimitive.rb_release_req.localConnectionRef = cx->lcr;
    p->nasRGDCPrimitive.rb_release_req.rbId = rb->rab_id + (NAS_RB_MAX_NUM * cx->lcr);
    //
    //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);
    bytes_wrote = nasrg_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)p, p->length);

    if (bytes_wrote) {
      rb->state=NAS_IDLE;
#ifdef NAS_DEBUG_DC
      printk("nasrg_ASCTL_DC_send_rb_release: RB_RELEASE_REQ primitive sent successfully in DC-FIFO\n");
#endif
    } else {
      ++rb->retry;
      rb->countimer=gpriv->timer_release;
      rb->state=NAS_CX_RELEASING_FAILURE;
      printk("nasrg_ASCTL_DC_send_rb_release: Message sent failure in DC-FIFO\n");
    }

    return bytes_wrote;

  default:
    return -NAS_ERROR_NOTCONNECTED;
  }
}

//-----------------------------------------------------------------------------
// Request the notification of a UE to joined or left services
int nasrg_ASCTL_DC_send_mbms_ue_notify_req(struct cx_entity *cx)
{
  //-----------------------------------------------------------------------------
  struct nas_rg_dc_element *p;
  int i;
  int bytes_wrote = 0;

#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_send_mbms_ue_notify_req - begin \n");
#endif

  if (!cx) {
    printk("nasrg_ASCTL_DC_send_mbms_ue_notify_req - input parameter is NULL \n");
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information
  p= (struct nas_rg_dc_element *)(gpriv->xbuffer);
  p->type = MBMS_UE_NOTIFY_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASMBMSUENotifyReq);
  //
  p->nasRGDCPrimitive.mbms_ue_notify_req.localConnectionRef = cx->lcr;

  // joined/left services are lists of MAX_MBMS_SERVICES
  // -1 means the end of the list
  for (i = 0; i < MAX_MBMS_SERVICES; i++) {
    p->nasRGDCPrimitive.mbms_ue_notify_req.joined_services[i].mbms_serviceId = (nasMBMSServiceId_t)cx->requested_joined_services[i];
    p->nasRGDCPrimitive.mbms_ue_notify_req.left_services[i].mbms_serviceId = (nasMBMSServiceId_t)cx->requested_left_services[i];
  }

  //
  //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);
  bytes_wrote = nasrg_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)p, p->length);

  if (bytes_wrote==p->length) {
#ifdef NAS_DEBUG_DC
    printk("nasrg_ASCTL_DC_send_mbms_ue_notify_req: MBMS_UE_NOTIFY_REQ primitive sent successfully in DC-FIFO\n");
    printk(" lcr (Mobile_id) %u\n",p->nasRGDCPrimitive.mbms_ue_notify_req.localConnectionRef);
    printk(" joined service %d, left service %d\n",cx->requested_joined_services[0], cx->requested_left_services[0] );
#endif
  } else
    printk("nasrg_ASCTL_DC_send_mbms_ue_notify_req: Message transmission failure to DC-FIFO\n");

  return bytes_wrote;
}

//---------------------------------------------------------------------------
// Request the transfer of data (DC SAP)
void nasrg_ASCTL_DC_send_sig_data_request(struct sk_buff *skb, struct cx_entity *cx, struct classifier_entity *gc)
{
  //---------------------------------------------------------------------------
  struct nas_rg_dc_element *p;
  int bytes_wrote = 0;
#ifdef PDCP_USE_NETLINK
  unsigned char xbuffer[NAS_MESSAGE_MAXLEN];
  int count=0;
#endif

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_send_sig_data - begin \n");
#endif

  if (!skb || !gc || !cx) {
    printk("nasrg_ASCTL_DC_send_sig_data - input parameter is NULL \n");
    return;
  }

  // End debug information
  if (cx->state!=NAS_CX_DCH) {
    printk("nasrg_ASCTL_DC_send_sig_data: Not connected, so the message is dropped\n");
    ++gpriv->stats.tx_dropped;
    return;
  }

  p = (struct nas_rg_dc_element *)(gpriv->xbuffer);
  p->type = DATA_TRANSFER_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASDataReq);
  p->nasRGDCPrimitive.data_transfer_req.localConnectionRef = cx->lcr;
  p->nasRGDCPrimitive.data_transfer_req.priority = NAS_SIG_SRB3;
  p->nasRGDCPrimitive.data_transfer_req.nasDataLength = skb->len;
  //
  //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);
#ifdef PDCP_USE_NETLINK
  memcpy(xbuffer,(unsigned char *)p, p->length);
  count = p->length;
  bytes_wrote = count;
#else
  bytes_wrote = nasrg_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)p, p->length);
#endif

  if (bytes_wrote!=p->length) {
    printk("nasrg_ASCTL_DC_send_sig_data: Header sent failure in DC-FIFO\n");
    return;
  }

  //bytes_wrote += rtf_put(cx->sap[NAS_DC_INPUT_SAPI], skb->data, skb->len);
#ifdef PDCP_USE_NETLINK
  memcpy(&(xbuffer[count]),(unsigned char *)skb->data, skb->len);
  count += skb->len;
  bytes_wrote = nasrg_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], xbuffer, count);
#else
  bytes_wrote += nasrg_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)skb->data, skb->len);
#endif

  if (bytes_wrote!=p->length+skb->len) {
    printk("nasrg_ASCTL_DC_send_sig_data: Data sent failure in DC-FIFO\n");
    return;
  }

  gpriv->stats.tx_bytes   += skb->len;
  gpriv->stats.tx_packets ++;
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_send_sig_data - end \n");
#endif
}

//---------------------------------------------------------------------------
// Confirm the establishment of a connection (DC channel)
int nasrg_ASCTL_DC_send_eNBmeasurement_req(struct cx_entity *cx)
{
  //---------------------------------------------------------------------------
  struct nas_rg_dc_element *p;
  int bytes_wrote = 0;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_send_eNBmeasurement_req - begin \n");
#endif

  if (!cx) {
    printk("nasrg_ASCTL_DC_send_eNBmeasurement_req - input parameter cx is NULL \n");
    return NAS_ERROR_NOTCORRECTVALUE;
  }

  // End debug information
  p= (struct nas_rg_dc_element *)(gpriv->xbuffer);
  p->type = ENB_MEASUREMENT_REQ;
  p->length =  NAS_TL_SIZE + sizeof(struct NASENbMeasureReq);
  p->nasRGDCPrimitive.eNBmeasurement_req.cell_id = cx->cellid;
  //
  //bytes_wrote = rtf_put(cx->sap[NAS_DC_INPUT_SAPI], p, p->length);
  bytes_wrote = nasrg_ASCTL_write(cx->sap[NAS_DC_INPUT_SAPI], (unsigned char *)p, p->length);

  if (bytes_wrote==p->length) {
#ifdef NAS_DEBUG_DC
    printk("nasrg_ASCTL_DC_send_eNBmeasurement_req: ENB_MEASUREMENT_REQ primitive sent successfully in DC-FIFO\n");
    printk(" cell_id %u\n",p->nasRGDCPrimitive.eNBmeasurement_req.cell_id);
#endif
  } else
    printk("nasrg_ASCTL_DC_send_eNBmeasurement_req: Message transmission failure to DC-FIFO\n");

  return bytes_wrote;
}

/***************************************************************************
     Reception side
 ***************************************************************************/

//---------------------------------------------------------------------------
// Decode CONN_ESTABLISH_IND message from RRC
void nasrg_ASCTL_DC_decode_cx_establish_ind(struct cx_entity *cx, struct nas_rg_dc_element *p)
{
  //---------------------------------------------------------------------------
  int i;
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_cx_establish - begin \n");
#endif

  if (!cx || !p) {
    printk("nasrg_ASCTL_DC_decode_cx_establish - input parameter is NULL \n");
    return;
  }

  // End debug information
  if (nasrg_ASCTL_DC_send_cx_establish_confirm(cx, ACCEPTED)>0) {
    nasrg_TOOL_imei2iid(p->nasRGDCPrimitive.conn_establish_ind.InterfaceIMEI, (uint8_t *)cx->iid6);
    cx->iid4=97;  // A AUTOMATISER
    cx->lcr = p->nasRGDCPrimitive.conn_establish_ind.localConnectionRef;
    cx->state=NAS_CX_DCH;
#ifdef NAS_DEBUG_DC
    printk("nasrg_ASCTL_DC_decode_cx_establish: CONN_ESTABLISH_IND reception\n");
    printk(" primitive length %d\n",p->length);
    printk(" Local Connection reference %d\n",p->nasRGDCPrimitive.conn_establish_ind.localConnectionRef);
    printk(" IMEI ");

    for (i=0; i<14; ++i)
      printk("%u",p->nasRGDCPrimitive.conn_establish_ind.InterfaceIMEI[i]);

    printk(" state ");
    nasrg_TOOL_print_state(cx->state);
#endif
  }
}

//---------------------------------------------------------------------------
// Decode CONN_RELEASE_IND message from RRC
void nasrg_ASCTL_DC_decode_cx_release_ind(struct cx_entity *cx, struct nas_rg_dc_element *p)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_cx_release - begin \n");
#endif

  if (!cx || !p) {
    printk("nasrg_ASCTL_DC_decode_cx_release - input parameter is NULL \n");
    return;
  }

  // End debug information
  cx->state=NAS_IDLE;
  cx->iid4=0;
  nasrg_TOOL_imei2iid(NAS_NULL_IMEI, (uint8_t *)cx->iid6);
  nasrg_COMMON_flush_rb(cx);
  nasrg_CLASS_flush_sclassifier(cx);
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_cx_release: CONN_RELEASE_IND reception\n");
  printk(" Primitive length %u\n",p->length);
  printk(" Local Connection reference %u\n",p->nasRGDCPrimitive.conn_release_ind.localConnectionRef);
  printk(" Release cause %u\n",p->nasRGDCPrimitive.conn_release_ind.releaseCause);
  nasrg_TOOL_print_state(cx->state);
#endif
}

//---------------------------------------------------------------------------
// Decode CONN_LOSS_IND message from RRC
void nasrg_ASCTL_DC_decode_cx_loss_ind(struct cx_entity *cx, struct nas_rg_dc_element *p)
{
  //---------------------------------------------------------------------------
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_cx_loss - begin \n");
#endif

  if (!cx || !p) {
    printk("nasrg_ASCTL_DC_decode_cx_loss - input parameter is NULL \n");
    return;
  }

  // End debug information
  cx->state = NAS_IDLE;
  cx->iid4=0;
  nasrg_TOOL_imei2iid(NAS_NULL_IMEI, (uint8_t *)cx->iid6);
  nasrg_COMMON_flush_rb(cx);
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_cx_loss: CONN_LOSS_IND reception\n");
  printk(" Primitive length %u\n",(int)(p->length));
  printk(" Local Connection reference %u\n",p->nasRGDCPrimitive.conn_loss_ind.localConnectionRef);
  nasrg_TOOL_print_state(cx->state);
#endif
}
//---------------------------------------------------------------------------
// Decode RB_ESTABLISH_CNF message from RRC
void nasrg_ASCTL_DC_decode_rb_establish_cnf(struct cx_entity *cx, struct nas_rg_dc_element *p)
{
  //---------------------------------------------------------------------------
  struct rb_entity *rb;
  int rb_id;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_rb_establish - begin \n");
#endif

  if (!cx || !p) {
    printk("nasrg_ASCTL_DC_decode_rb_establish - input parameter is NULL \n");
    return;
  }

  // End debug information
  rb_id = p->nasRGDCPrimitive.rb_establish_conf.rbId;
  rb=nasrg_COMMON_search_rb(cx, rb_id - (NAS_RB_MAX_NUM * cx->lcr));

  //
  if (rb!=NULL) {
    if (rb->state==NAS_CX_CONNECTING) {
#ifdef NAS_DEBUG_DC
      printk("nasrg_ASCTL_DC_decode_rb_establish: RB_ESTABLISH_CNF received\n");
      printk(" Primitive length %u\n", p->length);
      printk(" Local Connection reference %u\n",p->nasRGDCPrimitive.rb_establish_conf.localConnectionRef);
      printk(" RB Id %u\n",p->nasRGDCPrimitive.rb_establish_conf.rbId);
      printk(" SAP Id %u\n",p->nasRGDCPrimitive.rb_establish_conf.sapId);
      printk(" Status %u, Failure code %d, Cx state, RB state\n",p->nasRGDCPrimitive.rb_establish_conf.status, p->nasRGDCPrimitive.rb_establish_conf.fail_code);
      nasrg_TOOL_print_state(cx->state);
      nasrg_TOOL_print_state(rb->state);
#endif

      switch (p->nasRGDCPrimitive.rb_establish_conf.status) {
      case ACCEPTED:
        rb->state = NAS_CX_DCH;
        rb->countimer=1;
        break;

      case FAILURE:
        printk("nasrg_ASCTL_DC_decode_rb_establish: RB_ESTABLISH_CNF rejected\n");
        rb->state = NAS_CX_CONNECTING_FAILURE;
        //delete rb
        break;

      default:
        printk("nasrg_ASCTL_DC_decode_rb_establish: RB_ESTABLISH_CNF reception, invalid status\n");
      }
    } else
      printk("nasrg_ASCTL_DC_decode_rb_establish: invalid state %u\n", cx->state);
  } else
    printk("nasrg_ASCTL_DC_decode_rb_establish: RB_ESTABLISH_CNF, No corresponding radio bearer\n");
}

//---------------------------------------------------------------------------
// Decode DATA_TRANSFER_IND message from RRC
void nasrg_ASCTL_DC_decode_data_transfer_ind(struct cx_entity *cx, struct nas_rg_dc_element *p, char *buffer)
{
  //---------------------------------------------------------------------------
  uint8_t nasrg_data[10];
  unsigned int nas_length;
  char data_type;
  int bytes_read=0;
  unsigned int peer_command;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_data_transfer - begin \n");
#endif

  if (!cx || !p) {
    printk("nasrg_ASCTL_DC_decode_data_transfer - input parameter is NULL \n");
    return;
  }

  // End debug information

  bytes_read=p->length;
  // Get first character
  nas_length = (p->nasRGDCPrimitive.data_transfer_ind.nasDataLength) -1;
#ifndef PDCP_USE_NETLINK
  bytes_read += rtf_get(cx->sap[NAS_DC_OUTPUT_SAPI], &data_type, 1);
#else
  memcpy (&data_type, (char *)(&buffer[bytes_read]), 1);
  bytes_read ++;
#endif

  //check if peer message
  if (data_type =='A') {
    // receive in a skbuff
    //nasrg_COMMON_receive((p->length) + 1, nas_length, cx->sap[NAS_DC_OUTPUT_SAPI]); // original
#ifndef PDCP_USE_NETLINK
    //void nasrg_COMMON_receive(uint16_t bytes_read, uint16_t payload_length, void *data_buffer, int rb_id, int sap);
    // data_buffer is NULL because FIFO should be read directly in the skbuff (LITE has an intermediary buffer)
    nasrg_COMMON_receive((p->length) + 1, nas_length, NULL, 2, cx->sap[NAS_DC_OUTPUT_SAPI]);
#else
    //nasrg_COMMON_receive((p->length) + 1, nas_length, (unsigned char *)p+p->length+1, 2, 0);
    nasrg_COMMON_receive((p->length) + 1, nas_length, (unsigned char *)(&buffer[bytes_read]), 2, 0);
#endif
  } else {
    // if FIFO, empty remaining data
#ifndef PDCP_USE_NETLINK
    bytes_read += rtf_get(cx->sap[NAS_DC_OUTPUT_SAPI], (gpriv->rbuffer)+ (p->length), nas_length);
#endif

    if (data_type=='Z') {
#ifndef PDCP_USE_NETLINK
      memcpy (&nasrg_data, (char *)(gpriv->rbuffer)+ (p->length), 10);
#else
      memcpy (&nasrg_data, (char *)(&buffer[bytes_read]), 10);
#endif
      peer_command = (int)nasrg_data[0];
      printk("nasrg_ASCTL_DC_decode_data_transfer: Received peer message %d %d \n", nasrg_data[0], peer_command);
      nasrg_TOOL_print_buffer(nasrg_data, 10);

      if (nasrg_data[0]== NAS_CMD_OPEN_RB) {
        // open radio bearer
        printk("nasrg_ASCTL_DC_decode_data_transfer: Opening Default Radio Bearer\n");
        nasrg_ASCTL_start_default_rb(cx);
      } else if (nasrg_data[0]== NAS_CMD_ENTER_SLEEP) {
        printk("nasrg_ASCTL_DC_decode_data_transfer: Entering Sleep Mode\n");
        cx->state = NAS_CX_RELEASING;
      } else if (nasrg_data[0]== NAS_CMD_LEAVE_SLEEP) {
        printk("nasrg_ASCTL_DC_decode_data_transfer: Leaving Sleep Mode\n");
        cx->state = NAS_CX_DCH;
      } else {
        printk("\n\nnasrg_ASCTL_DC_decode_data_transfer: Unknown peer command\n");
      }
    } else
      printk("nasrg_ASCTL_DC_decode_data_transfer: Error during reception of the message - Dropped\n");
  }

#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_data_transfer: DATA_TRANSFER_IND reception\n");
  printk(" Primitive length %u\n", p->length);
  printk(" Local Connection reference %u\n",p->nasRGDCPrimitive.data_transfer_ind.localConnectionRef);
  printk(" Data Length %u\n", p->nasRGDCPrimitive.data_transfer_ind.nasDataLength);
  nasrg_TOOL_print_state(cx->state);
#endif
}

//---------------------------------------------------------------------------
// Decode MBMS_BEARER_ESTABLISH_CNF message from RRC
void nasrg_ASCTL_DC_decode_mbms_bearer_establish_cnf(struct nas_rg_dc_element *p)
{
  //---------------------------------------------------------------------------
  int mbms_ix;
  int rb_id;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_mbms_bearer_establish - begin \n");
#endif

  if (!p) {
    printk("nasrg_ASCTL_DC_decode_mbms_bearer_establish - input parameter is NULL \n");
    return;
  }

  // End debug information
  rb_id = p->nasRGDCPrimitive.mbms_establish_cnf.rbId;
  mbms_ix = 0;  // A revoir - find using cnxid...

  if (rb_id == gpriv->mbms_rb[mbms_ix].mbms_rbId) {
    switch (p->nasRGDCPrimitive.mbms_establish_cnf.status) {
    case ACCEPTED:
      gpriv->mbms_rb[mbms_ix].state = NAS_CX_DCH;
      gpriv->mbms_rb[mbms_ix].rab_id = gpriv->mbms_rb[mbms_ix].mbms_rbId;
      nasrg_ASCTL_start_mbmsclassifier(mbms_ix,&(gpriv->mbms_rb[mbms_ix]));
      break;

    case FAILURE:
      printk("nasrg_ASCTL_DC_decode_mbms_bearer_establish: MBMS_BEARER_ESTABLISH_CNF rejected\n");
      gpriv->mbms_rb[mbms_ix].state = NAS_CX_CONNECTING_FAILURE; //supprimer l'entree
      break;

    default:
      printk("nasrg_ASCTL_DC_decode_mbms_bearer_establish: MBMS_BEARER_ESTABLISH_CNF reception, invalid status\n");
    }
  } else
    printk(" nasrg_ASCTL_DC_decode_mbms_bearer_establish: invalid RB_Id %d\n", rb_id);

#ifdef NAS_DEBUG_DC
  printk(" nasrg_ASCTL_DC_decode_mbms_bearer_establish: MBMS_BEARER_ESTABLISH_CNF reception\n");
  printk(" Primitive length %u\n",p->length);
  printk(" rb_id %d, status %d\n",p->nasRGDCPrimitive.mbms_establish_cnf.rbId, p->nasRGDCPrimitive.mbms_establish_cnf.status);
  nasrg_TOOL_print_state(gpriv->mbms_rb[mbms_ix].state);
#endif
}

//---------------------------------------------------------------------------
// Decode MBMS_UE_NOTIFY_CNF message from RRC
void nasrg_ASCTL_DC_decode_mbms_ue_notify_cnf(struct cx_entity *cx, struct nas_rg_dc_element *p)
{
  //---------------------------------------------------------------------------
  int i, j, k;

  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_mbms_ue_notify - begin \n");
#endif

  if (!cx || !p) {
    printk("nasrg_ASCTL_DC_decode_mbms_ue_notify - input parameter is NULL \n");
    return;
  }

  // End debug information
  if (p->nasRGDCPrimitive.mbms_ue_notify_cnf.mbmsStatus == ACCEPTED) {
    for (i = 0; i<NASRG_MBMS_SVCES_MAX; i++) {
      if (cx->requested_joined_services[i] >=0) {
        for (j = 0; j<NASRG_MBMS_SVCES_MAX; j++) {
          if (cx->joined_services[j] ==-1) {
            cx->joined_services[j]= cx->requested_joined_services[i];
            cx->requested_joined_services[i]=-1;
            break;
          }
        }
      }

      if (cx->requested_left_services[i] >=0) {
        for (k = 0; k<NASRG_MBMS_SVCES_MAX; k++) {
          if (cx->joined_services[k] == cx->requested_left_services[i]) {
            cx->joined_services[k]=-1;
            cx->requested_left_services[i]=-1;
            break;
          }
        }
      }
    }
  }

#ifdef NAS_DEBUG_DC
  printk(" nasrg_ASCTL_DC_decode_mbms_ue_notify: MBMS_UE_NOTIFY_CNF reception\n");
  printk(" Primitive length %u\n",p->length);
  printk(" Local Connection reference %u\n",p->nasRGDCPrimitive.mbms_ue_notify_cnf.localConnectionRef);
  printk(" MBMS Status: %d\n", p->nasRGDCPrimitive.mbms_ue_notify_cnf.mbmsStatus);
  printk(" UE services currently joined \n");

  for (i = 0; i<NASRG_MBMS_SVCES_MAX; i++)
    printk ("%d * ", cx->joined_services[i]);

  nasrg_TOOL_print_state(cx->state);
#endif
}
/*
  msgToBuild->nasRgPrimitive.dc_sap_prim.nasRGDCPrimitive.eNBmeasurement_ind.cell_id = protocol_bs->rrc.rg_cell_id;
  // next values are temp hard coded, to be replaced by real values
  msgToBuild->nasRgPrimitive.dc_sap_prim.nasRGDCPrimitive.eNBmeasurement_ind.num_UEs = num_connected_UEs;
  for (ix=0; ix<ralpriv->num_connected_UEs; ix++){
    msgToBuild->nasRgPrimitive.dc_sap_prim.nasRGDCPrimitive.eNBmeasurement_ind.measures[ix].rlcBufferOccupancy = 100 - (30*ix);
    msgToBuild->nasRgPrimitive.dc_sap_prim.nasRGDCPrimitive.eNBmeasurement_ind.measures[ix].scheduledPRB = 500 - (200*ix);
    msgToBuild->nasRgPrimitive.dc_sap_prim.nasRGDCPrimitive.eNBmeasurement_ind.measures[ix].totalDataVolume = 640000 + (160000*ix);
  }
  msgToBuild->nasRgPrimitive.dc_sap_prim.nasRGDCPrimitive.eNBmeasurement_ind.totalNumPRBs = 1000;

*/
//---------------------------------------------------------------------------
// Decode ENB_MEASUREMENT_IND message from RRC
void nasrg_ASCTL_DC_decode_eNBmeasurement_ind(struct nas_rg_dc_element *p)
{
  //---------------------------------------------------------------------------
  uint8_t i;
  // Start debug information
#ifdef NAS_DEBUG_DC
  printk("nasrg_ASCTL_DC_decode_eNBmeasurement_ind - begin \n");
#endif

  if (!p) {
    printk("nasrg_ASCTL_DC_decode_eNBmeasurement_ind - input parameter p is NULL \n");
    return;
  }

  // End debug information
#ifdef NAS_DEBUG_DC_DETAIL
  printk(" nasrg_ASCTL_DC_decode_eNBmeasurement_ind : ENB_MEASUREMENT_IND reception\n");
  printk(" Measured Cell: %u\n", p->nasRGDCPrimitive.eNBmeasurement_ind.cell_id);
  printk(" Number of Connected Mobiles: %u\n", p->nasRGDCPrimitive.eNBmeasurement_ind.num_UEs);

  for (i=0; i<p->nasRGDCPrimitive.eNBmeasurement_ind.num_UEs; ++i) {
    printk(" UE[%u]:  rlcBufferOccupancy %u, scheduledPRB: %u, totalDataVolume: %u\n", i,
           p->nasRGDCPrimitive.eNBmeasurement_ind.measures[i].rlcBufferOccupancy,
           p->nasRGDCPrimitive.eNBmeasurement_ind.measures[i].scheduledPRB,
           p->nasRGDCPrimitive.eNBmeasurement_ind.measures[i].totalDataVolume);
  }

  printk(" Total number of PRBs: %u\n", p->nasRGDCPrimitive.eNBmeasurement_ind.totalNumPRBs);
#endif
  // store Measures
  gpriv->measured_cell_id = p->nasRGDCPrimitive.eNBmeasurement_ind.cell_id;
  gpriv->num_UEs = p->nasRGDCPrimitive.eNBmeasurement_ind.num_UEs;

  for (i=0; i<gpriv-> num_UEs; ++i) {
    gpriv->rlcBufferOccupancy[i] = p->nasRGDCPrimitive.eNBmeasurement_ind.measures[i].rlcBufferOccupancy;
    gpriv->scheduledPRB[i] += p->nasRGDCPrimitive.eNBmeasurement_ind.measures[i].scheduledPRB;
    gpriv->totalDataVolume[i] += p->nasRGDCPrimitive.eNBmeasurement_ind.measures[i].totalDataVolume;
  }

  gpriv->totalNumPRBs += p->nasRGDCPrimitive.eNBmeasurement_ind.totalNumPRBs;
}

//---------------------------------------------------------------------------
// Check if anything in DC FIFO and process it (RG Finite State Machine)
int nasrg_ASCTL_DC_receive(struct cx_entity *cx, char *buffer)
{
  //---------------------------------------------------------------------------
  int bytes_read=0;

  // Start debug information
#ifdef NAS_DEBUG_DC_DETAIL
  printk("nasrg_ASCTL_DC_receive - begin \n");
#endif

  if (!cx) {
    printk("nasrg_ASCTL_DC_receive - input parameter cx is NULL \n");
    return 0;
  }

  // End debug information

#ifndef PDCP_USE_NETLINK
  bytes_read = rtf_get(cx->sap[NAS_DC_OUTPUT_SAPI] , gpriv->rbuffer, NAS_TL_SIZE);
#else
  bytes_read = NAS_TL_SIZE;
#endif

  if (bytes_read>0) {
    struct nas_rg_dc_element *p;
#ifndef PDCP_USE_NETLINK
    p= (struct nas_rg_dc_element *)(gpriv->rbuffer);
    //get the rest of the primitive
    bytes_read += rtf_get(cx->sap[NAS_DC_OUTPUT_SAPI], (uint8_t *)p+NAS_TL_SIZE, p->length-NAS_TL_SIZE);

    if (bytes_read!=p->length) {
      printk("nasrg_ASCTL_DC_receive: Problem while reading primitive's header\n");
      return bytes_read;
    }

#else
    p= (struct nas_rg_dc_element *)(buffer);
    bytes_read=p->length;
#endif

    switch (p->type) {
    case CONN_ESTABLISH_IND :
      if (p->nasRGDCPrimitive.conn_establish_ind.localConnectionRef!=cx->lcr)
        printk("nasrg_ASCTL_DC_receive: CONN_ESTABLISH_IND reception, Local connection reference not correct %u\n", p->nasRGDCPrimitive.conn_establish_ind.localConnectionRef);
      else {
        switch(cx->state) {
        case NAS_IDLE:
          nasrg_ASCTL_DC_decode_cx_establish_ind(cx,p);
          break;

        default:
          printk("nasrg_ASCTL_DC_receive: CONN_ESTABLISH_IND reception, invalid state %u\n", cx->state);
        }
      }

      break;

    case CONN_RELEASE_IND :
      if (p->nasRGDCPrimitive.conn_release_ind.localConnectionRef!=cx->lcr)
        printk("nasrg_ASCTL_DC_receive: CONN_RELEASE_IND reception, Local connection reference not correct %u\n", p->nasRGDCPrimitive.conn_release_ind.localConnectionRef);
      else {
        switch(cx->state) {
        case NAS_CX_DCH:
          nasrg_ASCTL_DC_decode_cx_release_ind(cx,p);
          break;

        default:
          printk("nasrg_ASCTL_DC_receive: CONN_RELEASE_IND reception, invalid state %u\n", cx->state);
        }
      }

      break;

    case CONN_LOSS_IND:
      if (p->nasRGDCPrimitive.conn_loss_ind.localConnectionRef!=cx->lcr)
        printk("nasrg_ASCTL_DC_receive: CONN_LOSS_IND reception, Local connection reference not correct %u\n", p->nasRGDCPrimitive.conn_loss_ind.localConnectionRef);
      else {
        switch(cx->state) {
        case NAS_CX_DCH:
          nasrg_ASCTL_DC_decode_cx_loss_ind(cx,p);
          break;

        default:
          printk("nasrg_ASCTL_DC_receive: CONN_LOSS_IND reception, invalid state %u\n", cx->state);
        }
      }

      break;

    case RB_ESTABLISH_CNF:
      if (p->nasRGDCPrimitive.rb_establish_conf.localConnectionRef!=cx->lcr)
        printk("nasrg_ASCTL_DC_receive: RB_ESTABLISH_CNF reception, Local connection reference not correct %u\n", p->nasRGDCPrimitive.rb_establish_conf.localConnectionRef);
      else {
        switch(cx->state) {
        case NAS_CX_DCH:
          nasrg_ASCTL_DC_decode_rb_establish_cnf(cx,p);
#ifdef NAS_AUTO_MBMS
          nasrg_ASCTL_start_default_ue_notification(cx);
#endif
          break;

        default:
          printk("nasrg_ASCTL_DC_receive: RB_ESTABLISH_CNF reception, invalid state %u\n", cx->state);
        }
      }

      break;

    case DATA_TRANSFER_IND:
      if (p->nasRGDCPrimitive.data_transfer_ind.localConnectionRef!=cx->lcr)
        printk("nasrg_ASCTL_DC_receive: DATA_TRANSFER_IND reception, Local connection reference not correct %u\n", p->nasRGDCPrimitive.rb_establish_conf.localConnectionRef);
      else {
        switch(cx->state) {
        case NAS_CX_DCH:
        case NAS_CX_RELEASING:
          nasrg_ASCTL_DC_decode_data_transfer_ind(cx,p,buffer);
          break;

        default:
          printk("nasrg_ASCTL_DC_receive: DATA_TRANSFER_IND reception, invalid state %u\n", cx->state);
        }
      }

      break;

      // Temp - Should be in uplink GC-SAP
    case MBMS_BEARER_ESTABLISH_CNF:
      //      if (p->nasRGDCPrimitive.mbms_ue_notify_cnf.localConnectionRef!=cx->lcr)
      //        printk("nasrg_ASCTL_DC_receive: MBMS_BEARER_ESTABLISH_CNF reception, Local connection reference not correct %u\n", p->nasRGDCPrimitive.rb_establish_conf.localConnectionRef);
      //      else
      nasrg_ASCTL_DC_decode_mbms_bearer_establish_cnf(p);
      break;

    case MBMS_UE_NOTIFY_CNF:
      if (p->nasRGDCPrimitive.mbms_ue_notify_cnf.localConnectionRef!=cx->lcr)
        printk("nasrg_ASCTL_DC_receive: MBMS_UE_NOTIFY_CNF reception, Local connection reference not correct %u\n", p->nasRGDCPrimitive.rb_establish_conf.localConnectionRef);
      else {
        switch(cx->state) {
        case NAS_CX_DCH:
          nasrg_ASCTL_DC_decode_mbms_ue_notify_cnf(cx,p);
          /* //Temp
          nasrg_ASCTL_start_default_mbms_service();*/
          break;

        default:
          printk("nasrg_ASCTL_DC_receive: MBMS_UE_NOTIFY_CNF reception, invalid state %u\n", cx->state);
        }
      }

      break;

      // Temp - Should be in uplink GC-SAP
    case ENB_MEASUREMENT_IND :
      //      if (p->nasRGDCPrimitive.eNBmeasurement_ind.localConnectionRef!=cx->lcr)
      //        printk("nasrg_ASCTL_DC_receive: ENB_MEASUREMENT_IND reception, Local connection reference not correct %u\n", p->nasRGDCPrimitive.eNBmeasurement_ind.localConnectionRef);
      //      else
      nasrg_ASCTL_DC_decode_eNBmeasurement_ind(p);
      break;

    default :
      printk("nasrg_ASCTL_DC_receive: Invalid message received\n");
    }
  }

  return bytes_read;
}

#endif
