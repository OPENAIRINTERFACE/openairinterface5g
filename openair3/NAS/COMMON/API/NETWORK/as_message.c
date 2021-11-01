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

/*****************************************************************************

Source    as_message.c

Version   0.1

Date    2012/11/06

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Defines the messages supported by the Access Stratum sublayer
    protocol (usually RRC and S1AP for E-UTRAN) and functions used
    to encode and decode

*****************************************************************************/

#include "as_message.h"
#include "commonDef.h"
#include "nas_log.h"

#include <string.h> // memcpy
#include <stdlib.h> // free

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  as_message_decode()                                       **
 **                                                                        **
 ** Description: Decode AS message and accordingly fills data structure    **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing the       **
 **       message                                    **
 **      length:  Number of bytes that should be decoded     **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   msg:   AS message structure to be filled          **
 **      Return:  The AS message identifier when the buffer  **
 **       has been successfully decoded;             **
 **       RETURNerror otherwise.                     **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int as_message_decode(const char* buffer, as_message_t* msg, int length)
{
  LOG_FUNC_IN;

  int bytes;
  /* pointers to msg fields possibly not aligned because msg  points to a packed structure
 * Using these possibly unaligned pointers in a function call may trigger alignment errors at run time and
 * gcc, from v9,  now warns about it. fix these warnings by removing the indirection on data
 * (in fact i don't understand this code data seems to be useless...)
 */
  Byte_t *data = NULL;

  /* Get the message type */
  msg->msgID = *(uint16_t*)(buffer);
  bytes = sizeof(uint16_t);

  switch (msg->msgID) {
  case AS_NAS_ESTABLISH_REQ:
    /* NAS signalling connection establish request */
    bytes += sizeof(nas_establish_req_t) - sizeof(Byte_t*);
    data = msg->msg.nas_establish_req.initialNasMsg.data;
    break;

  case AS_NAS_ESTABLISH_IND:
    /* NAS signalling connection establishment indication */
    bytes += sizeof(nas_establish_ind_t) - sizeof(Byte_t*);
    data = msg->msg.nas_establish_ind.initialNasMsg.data;
    break;

  case AS_NAS_ESTABLISH_RSP:
    /* NAS signalling connection establishment response */
    bytes += sizeof(nas_establish_rsp_t) - sizeof(Byte_t*);
    data = msg->msg.nas_establish_rsp.nasMsg.data;
    break;

  case AS_NAS_ESTABLISH_CNF:
    /* NAS signalling connection establishment confirm */
    bytes += sizeof(nas_establish_cnf_t) - sizeof(Byte_t*);
    data = msg->msg.nas_establish_cnf.nasMsg.data;
    break;

  case AS_UL_INFO_TRANSFER_REQ:
    /* Uplink L3 data transfer request */
    bytes += sizeof(ul_info_transfer_req_t) - sizeof(Byte_t*);
    data = msg->msg.ul_info_transfer_req.nasMsg.data;
    break;

  case AS_UL_INFO_TRANSFER_IND:
    /* Uplink L3 data transfer indication */
    bytes += sizeof(ul_info_transfer_ind_t) - sizeof(Byte_t*);
    data = msg->msg.ul_info_transfer_ind.nasMsg.data;
    break;

  case AS_DL_INFO_TRANSFER_REQ:
    /* Downlink L3 data transfer request */
    bytes += sizeof(dl_info_transfer_req_t) - sizeof(Byte_t*);
    data = msg->msg.dl_info_transfer_req.nasMsg.data;
    break;

  case AS_DL_INFO_TRANSFER_IND:
    /* Downlink L3 data transfer indication */
    bytes += sizeof(dl_info_transfer_ind_t) - sizeof(Byte_t*);
    data = msg->msg.dl_info_transfer_ind.nasMsg.data;
    break;

  case AS_BROADCAST_INFO_IND:
  case AS_CELL_INFO_REQ:
  case AS_CELL_INFO_CNF:
  case AS_CELL_INFO_IND:
  case AS_PAGING_REQ:
  case AS_PAGING_IND:
  case AS_NAS_RELEASE_REQ:
  case AS_UL_INFO_TRANSFER_CNF:
  case AS_DL_INFO_TRANSFER_CNF:
  case AS_NAS_RELEASE_IND:
  case AS_RAB_ESTABLISH_REQ:
  case AS_RAB_ESTABLISH_IND:
  case AS_RAB_ESTABLISH_RSP:
  case AS_RAB_ESTABLISH_CNF:
  case AS_RAB_RELEASE_REQ:
  case AS_RAB_RELEASE_IND:
    /* Messages without dedicated NAS information */
    bytes = length;
    break;

  default:
    bytes = 0;
    LOG_TRACE(WARNING, "NET-API   - AS message 0x%x is not valid",
              msg->msgID);
    break;
  }

  if (bytes > 0) {
    if (data) {
      /* Set the pointer to dedicated NAS information */
      /* wasn't data already computed above for specific cases here we override ?? */
      data = (Byte_t *)buffer + bytes;
    }

    /* Decode the message */
    memcpy(msg, (as_message_t*)buffer, bytes);
    LOG_FUNC_RETURN (msg->msgID);
  }

  LOG_TRACE(WARNING, "NET-API   - Failed to decode AS message 0x%x",
            msg->msgID);
  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:  as_message_encode()                                       **
 **                                                                        **
 ** Description: Encode AS message                                         **
 **                                                                        **
 ** Inputs:  msg:   AS message structure to encode             **
 **      length:  Maximal capacity of the output buffer      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of characters in the buffer     **
 **       when data have been successfully encoded;  **
 **       RETURNerror otherwise.                     **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int as_message_encode(char* buffer, as_message_t* msg, int length)
{
  LOG_FUNC_IN;

  int bytes = sizeof(msg->msgID);
  as_nas_info_t nas_msg;
  Byte_t *dataptr=NULL;
  uint32_t len=0;
  
  switch (msg->msgID) {
  case AS_BROADCAST_INFO_IND:
    /* Broadcast information */
    bytes += sizeof(broadcast_info_ind_t);
    break;

  case AS_CELL_INFO_REQ:
    /* Cell information request */
    bytes += sizeof(cell_info_req_t);
    break;

  case AS_CELL_INFO_CNF:
    /* Cell information response */
    bytes += sizeof(cell_info_cnf_t);
    break;

  case AS_CELL_INFO_IND:
    /* Cell information indication */
    bytes += sizeof(cell_info_ind_t);
    break;

  case AS_PAGING_REQ:
    /* Paging information request */
    bytes += sizeof(paging_req_t);
    break;

  case AS_PAGING_IND:
    /* Paging information indication */
    bytes += sizeof(paging_ind_t);
    break;

  case AS_NAS_ESTABLISH_REQ:
    /* NAS signalling connection establish request */
    bytes += sizeof(nas_establish_req_t) - sizeof(Byte_t*);
    nas_msg = msg->msg.nas_establish_req.initialNasMsg;
    break;

  case AS_NAS_ESTABLISH_IND:
    /* NAS signalling connection establish indication */
    bytes += sizeof(nas_establish_ind_t) - sizeof(Byte_t*);
    nas_msg = msg->msg.nas_establish_ind.initialNasMsg;
    dataptr=(Byte_t *)&(msg->msg.nas_establish_ind.initialNasMsg.data);
    len=msg->msg.nas_establish_ind.initialNasMsg.length;
    break;

  case AS_NAS_ESTABLISH_RSP:
    /* NAS signalling connection establish response */
    bytes += sizeof(nas_establish_rsp_t) - sizeof(Byte_t*);
    nas_msg = msg->msg.nas_establish_rsp.nasMsg;

    break;

  case AS_NAS_ESTABLISH_CNF:
    /* NAS signalling connection establish confirm */
    bytes += sizeof(nas_establish_cnf_t) - sizeof(Byte_t*);
    nas_msg = msg->msg.nas_establish_cnf.nasMsg;
    dataptr=(Byte_t *)&(msg->msg.nas_establish_cnf.nasMsg.data);
    len=msg->msg.nas_establish_ind.initialNasMsg.length;
    break;

  case AS_NAS_RELEASE_REQ:
    /* NAS signalling connection release request */
    bytes += sizeof(nas_release_req_t);
    break;

  case AS_NAS_RELEASE_IND:
    /* NAS signalling connection release indication */
    bytes += sizeof(nas_release_ind_t);
    break;

  case AS_UL_INFO_TRANSFER_REQ:
    /* Uplink L3 data transfer request */
    bytes += sizeof(ul_info_transfer_req_t) - sizeof(Byte_t*);
    nas_msg = msg->msg.ul_info_transfer_req.nasMsg;
    break;

  case AS_UL_INFO_TRANSFER_CNF:
    /* Uplink L3 data transfer confirm */
    bytes += sizeof(ul_info_transfer_cnf_t);
    break;

  case AS_UL_INFO_TRANSFER_IND:
    /* Uplink L3 data transfer indication */
    bytes += sizeof(ul_info_transfer_ind_t) - sizeof(Byte_t*);
    nas_msg = msg->msg.ul_info_transfer_ind.nasMsg;
    break;

  case AS_DL_INFO_TRANSFER_REQ:
    /* Downlink L3 data transfer */
    bytes += sizeof(dl_info_transfer_req_t) - sizeof(Byte_t*);
    nas_msg = msg->msg.dl_info_transfer_req.nasMsg;
    break;

  case AS_DL_INFO_TRANSFER_CNF:
    /* Downlink L3 data transfer confirm */
    bytes += sizeof(dl_info_transfer_cnf_t);
    break;

  case AS_DL_INFO_TRANSFER_IND:
    /* Downlink L3 data transfer indication */
    bytes += sizeof(dl_info_transfer_ind_t) - sizeof(Byte_t*);
    nas_msg = msg->msg.dl_info_transfer_ind.nasMsg;
 
    break;

  case AS_RAB_ESTABLISH_REQ:
    /* Radio Access Bearer establishment request */
    bytes += sizeof(rab_establish_req_t);
    break;

  case AS_RAB_ESTABLISH_IND:
    /* Radio Access Bearer establishment indication */
    bytes += sizeof(rab_establish_ind_t);
    break;

  case AS_RAB_ESTABLISH_RSP:
    /* Radio Access Bearer establishment response */
    bytes += sizeof(rab_establish_rsp_t);
    break;

  case AS_RAB_ESTABLISH_CNF:
    /* Radio Access Bearer establishment confirm */
    bytes += sizeof(rab_establish_cnf_t);
    break;

  case AS_RAB_RELEASE_REQ:
    /* Radio Access Bearer release request */
    bytes += sizeof(rab_release_req_t);
    break;

  case AS_RAB_RELEASE_IND:
    /* Radio Access Bearer release indication */
    bytes += sizeof(rab_release_ind_t);
    break;

  default:
    LOG_TRACE(WARNING, "NET-API   - AS message 0x%x is not valid",
              msg->msgID);
    bytes = length;
    break;
  }

  if (length > bytes) {
    /* Encode the AS message */
    memcpy(buffer, (unsigned char*)msg, bytes);

    if ( (dataptr!=NULL) && (len > 0) ) {
      /* Copy the NAS message */
      memcpy(buffer + bytes, nas_msg.data, nas_msg.length);
      bytes += len;
      /* Release NAS message memory */
      free(nas_msg.data);
      len=0;
      dataptr = NULL;
    }

    LOG_FUNC_RETURN (bytes);
  }

  LOG_TRACE(WARNING, "NET-API   - Failed to encode AS message 0x%x",
            msg->msgID);
  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

