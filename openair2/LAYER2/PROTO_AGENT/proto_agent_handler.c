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

//#include "enb_agent_mac.h" // Do we need this?

#include "log.h"

#include "assertions.h"

proto_agent_message_decoded_callback agent_messages_callback[][3] = {
  {proto_agent_hello, 0, 0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_HELLO_MSG*/
};

proto_agent_message_destruction_callback message_destruction_callback[] = {
  proto_agent_destroy_hello,
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
  
  Protocol__FlexsplitMessage *decoded_message, *reply_message;
  err_code_t err_code;
  DevAssert(data != NULL);

  if (proto_agent_deserialize_message(data, size, &decoded_message) < 0) {
    err_code= PROTOCOL__FLEXSPLIT_ERR__MSG_DECODING;
    goto error; 
  }
  
  // Undestand why these calculations take place
  if ((decoded_message->msg_case > sizeof(agent_messages_callback) / (3*sizeof(proto_agent_message_decoded_callback))) || 
      (decoded_message->msg_dir > PROTOCOL__FLEXSPLIT_DIRECTION__UNSUCCESSFUL_OUTCOME)){
    err_code= PROTOCOL__FLEXSPLIT_ERR__MSG_NOT_HANDLED;
      goto error;
  }
    
  if (agent_messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1] == NULL) {
    err_code= PROTOCOL__FLEXSPLIT_ERR__MSG_NOT_SUPPORTED;
    goto error;

  }

  err_code = ((*agent_messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1])(mod_id, (void *) decoded_message, &reply_message));
  if ( err_code < 0 ){
    goto error;
  } else if (err_code == 1) { //If err_code > 1, we do not want to dispose the message yet
    protocol__flexsplit_message__free_unpacked(decoded_message, NULL);
  }
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
  
  // free the msg --> later keep this in the data struct and just update the values
  //TODO call proper destroy function
  err_code = ((*message_destruction_callback[msg->msg_case-1])(msg));
  
  DevAssert(buffer !=NULL);
  
  LOG_D(PROTO_AGENT,"Serilized the enb mac stats reply (size %d)\n", *size);
  
  return buffer;
  
 error : 
  LOG_E(PROTO_AGENT,"errno %d occured\n",err_code);
  
  return NULL;   
}

Protocol__FlexsplitMessage* proto_agent_process_timeout(long timer_id, void* timer_args){
    
  struct proto_agent_timer_element_s *found = get_timer_entry(timer_id);
 
  if (found == NULL ) goto error;
//  LOG_I(PROTO_AGENT, "Found the entry (%p): timer_id is 0x%lx  0x%lx\n", found, timer_id, found->timer_id);
  
  if (timer_args == NULL)
    LOG_W(PROTO_AGENT,"null timer args\n");
  
//  return found->cb(timer_args);
    return 1;
 error:
  LOG_E(PROTO_AGENT, "can't get the timer element\n");
  return TIMER_ELEMENT_NOT_FOUND;
}

err_code_t proto_agent_destroy_flexsplit_message(Protocol__FlexsplitMessage *msg) {
  return ((*message_destruction_callback[msg->msg_case-1])(msg));
}
