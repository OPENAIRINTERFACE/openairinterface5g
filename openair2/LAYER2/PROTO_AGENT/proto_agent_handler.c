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

/*! \file enb_agent_handler.c
 * \brief enb agent tx and rx message handler 
 * \author Navid Nikaein and Xenofon Foukas 
 * \date 2016
 * \version 0.1
 */


#include "proto_agent_common.h"
#include "log.h"
#include "assertions.h"

proto_agent_message_decoded_callback agent_messages_callback[][3] = {
  {proto_agent_hello, 0, 0},
  {proto_agent_echo_reply, 0, 0},
  {0, just_print, 0},
  {proto_agent_pdcp_data_req_ack, 0, 0},
  {0, proto_agent_get_ack_result, 0},
  {proto_agent_pdcp_data_ind_ack, 0, 0},
  {0, just_print, 0},
};

proto_agent_message_destruction_callback message_destruction_callback[] = {
  proto_agent_destroy_hello,
  proto_agent_destroy_echo_request,
  proto_agent_destroy_echo_reply,
  proto_agent_destroy_pdcp_data_req,
  proto_agent_destroy_pdcp_data_req_ack,
  proto_agent_destroy_pdcp_data_ind,
  proto_agent_destroy_pdcp_data_ind_ack,

};

static const char *proto_agent_direction2String[] = {
  "", /* not_set  */
  "originating message", /* originating message */
  "successfull outcome", /* successfull outcome */
  "unsuccessfull outcome", /* unsuccessfull outcome */
};


Protocol__FlexsplitMessage* proto_agent_handle_message (mid_t mod_id,
						    uint8_t *data, 
						    uint32_t size){
  
  Protocol__FlexsplitMessage *decoded_message = NULL;
  Protocol__FlexsplitMessage *reply_message = NULL;
  err_code_t err_code;
  DevAssert(data != NULL);

  LOG_D(PROTO_AGENT, "Deserializing message with size %u \n", size);
  if (proto_agent_deserialize_message(data, (int) size, &decoded_message) < 0) {
    err_code= PROTOCOL__FLEXSPLIT_ERR__MSG_DECODING;
    goto error; 
  }
  Protocol__FspHeader *header = (Protocol__FspHeader*) decoded_message;
  if (header->has_type)
   {
    LOG_D(PROTO_AGENT, "Deserialized MSG type is %d and %u\n", decoded_message->msg_case, decoded_message->msg_dir);
   }

  if ((decoded_message->msg_case > sizeof(agent_messages_callback) / (3*sizeof(proto_agent_message_decoded_callback))) || 
      (decoded_message->msg_dir > PROTOCOL__FLEXSPLIT_DIRECTION__UNSUCCESSFUL_OUTCOME))
  {
      err_code= PROTOCOL__FLEXSPLIT_ERR__MSG_NOT_HANDLED;
      LOG_D(PROTO_AGENT,"Handling message: MSG NOT handled, going to error\n");
      goto error;
  }
  
  err_code = ((*agent_messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1])(mod_id, (void *) decoded_message, &reply_message));

  if ( err_code < 0 )
  {
    LOG_I(PROTO_AGENT, "decoded_message case : %d, direction : %d \n", decoded_message->msg_case-1, decoded_message->msg_dir-1);
    goto error;
  }
  else if (err_code == 1) 
  {
    protocol__flexsplit_message__free_unpacked(decoded_message, NULL);
  }
  LOG_D(PROTO_AGENT,"Returning REPLY message after the callback\n");
  return reply_message;
  
 error:
  LOG_E(PROTO_AGENT,"errno %d occured\n",err_code);
  return NULL;
}



void * proto_agent_pack_message(Protocol__FlexsplitMessage *msg, 
			      uint32_t * size){
  void * buffer;
  err_code_t err_code = PROTOCOL__FLEXSPLIT_ERR__NO_ERR;
  
  if (proto_agent_serialize_message(msg, &buffer, size) < 0 ) {
    err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENCODING;
    goto error;
  }
  
  //TODO call proper destroy function
  
  err_code = ((*message_destruction_callback[msg->msg_case-1])(msg));
  
  DevAssert(buffer !=NULL);
  
  LOG_D(PROTO_AGENT,"Serialized the enb mac stats reply (size %d)\n", *size);
  return buffer;
  
 error : 
  LOG_E(PROTO_AGENT,"errno %d occured\n",err_code);
  
  return NULL;   
}

err_code_t proto_agent_destroy_flexsplit_message(Protocol__FlexsplitMessage *msg) {
  return ((*message_destruction_callback[msg->msg_case-1])(msg));
}
