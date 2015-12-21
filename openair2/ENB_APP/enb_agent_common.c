/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file 
 * \brief 
 * \author 
 * \date 2016
 * \version 0.1
 */


#include "enb_agent_common.h"
#include "log.h"


int enb_agent_serialize_message(Protocol__ProgranMessage *msg, void **buf, int *size) {

  *size = protocol__progran_message__get_packed_size(msg);
  
  *buf = malloc(*size);
  if (buf == NULL)
    goto error;
  
  protocol__progran_message__pack(msg, *buf);
  
  return 0;
  
 error:
  LOG_E(ENB_APP, "an error occured\n"); // change the com
  return -1;
}



/* We assume that the buffer size is equal to the message size.
   Should be chekced durint Tx/Rx */
int enb_agent_deserialize_message(void *data, int size, Protocol__ProgranMessage **msg) {
  *msg = protocol__progran_message__unpack(NULL, size, data);
  if (*msg == NULL)
    goto error;
  
  return 0;
  
 error:
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}





int prp_create_header(uint32_t xid, Protocol__PrpType type,  Protocol__PrpHeader **header) {
  
  *header = malloc(sizeof(Protocol__PrpHeader));
  if(*header == NULL)
    goto error;
  
  protocol__prp_header__init(*header);
  (*header)->version = PROGRAN_VERSION;
  (*header)->has_version = 1; 
  // check if the type is set
  (*header)->type = type;
  (*header)->has_type = 1;
  (*header)->xid = xid;
  (*header)->has_xid = 1;
  return 0;

 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int enb_agent_hello(uint32_t xid, Protocol__ProgranMessage **msg) {
 
  Protocol__PrpHeader *header;
  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_HELLO, &header) != 0)
    goto error;

  Protocol__PrpHello *hello_msg;
  hello_msg = malloc(sizeof(Protocol__PrpHello));
  if(hello_msg == NULL)
    goto error;
  protocol__prp_hello__init(hello_msg);
  hello_msg->header = header;

  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_HELLO_MSG;
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->hello_msg = hello_msg;
  return 0;
  
 error:
  if(header != NULL)
    free(header);
  if(hello_msg != NULL)
    free(hello_msg);
  if(*msg != NULL)
    free(*msg);
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int enb_agent_destroy_hello_message(Protocol__ProgranMessage *msg) {
  
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_HELLO_MSG)
    goto error;
  
  free(msg->hello_msg->header);
  free(msg->hello_msg);
  free(msg);
  return 0;

 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int enb_agent_echo_request(uint32_t xid, Protocol__ProgranMessage **msg) {
  Protocol__PrpHeader *header;
  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_ECHO_REQUEST, &header) != 0)
    goto error;

  Protocol__PrpEchoRequest *echo_request_msg;
  echo_request_msg = malloc(sizeof(Protocol__PrpEchoRequest));
  if(echo_request_msg == NULL)
    goto error;
  protocol__prp_echo_request__init(echo_request_msg);
  echo_request_msg->header = header;

  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REQUEST_MSG;
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__INITIATING_MESSAGE;
  (*msg)->echo_request_msg = echo_request_msg;
  return 0;

 error:
  if(header != NULL)
    free(header);
  if(echo_request_msg != NULL)
    free(echo_request_msg);
  if(*msg != NULL)
    free(*msg);
  //LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int enb_agent_destroy_echo_request(Protocol__ProgranMessage *msg) {
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REQUEST_MSG)
    goto error;
  
  free(msg->echo_request_msg->header);
  free(msg->echo_request_msg);
  free(msg);
  return 0;
  
 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}



int enb_agent_echo_reply(uint32_t xid, Protocol__ProgranMessage **msg) {
  Protocol__PrpHeader *header;
  if (prp_create_header(xid, PROTOCOL__PRP_TYPE__PRPT_ECHO_REPLY, &header) != 0)
    goto error;

  Protocol__PrpEchoReply *echo_reply_msg;
  echo_reply_msg = malloc(sizeof(Protocol__PrpEchoReply));
  if(echo_reply_msg == NULL)
    goto error;
  protocol__prp_echo_reply__init(echo_reply_msg);
  echo_reply_msg->header = header;

  *msg = malloc(sizeof(Protocol__ProgranMessage));
  if(*msg == NULL)
    goto error;
  protocol__progran_message__init(*msg);
  (*msg)->msg_case = PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REPLY_MSG;
  (*msg)->msg_dir = PROTOCOL__PROGRAN_DIRECTION__SUCCESSFUL_OUTCOME;
  (*msg)->echo_reply_msg = echo_reply_msg;
  return 0;

 error:
  if(header != NULL)
    free(header);
  if(echo_reply_msg != NULL)
    free(echo_reply_msg);
  if(*msg != NULL)
    free(*msg);
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}


int enb_agent_destroy_echo_reply(Protocol__ProgranMessage *msg) {
  if(msg->msg_case != PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REPLY_MSG)
    goto error;
  
  free(msg->echo_reply_msg->header);
  free(msg->echo_reply_msg);
  free(msg);
  return 0;
  
 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}
