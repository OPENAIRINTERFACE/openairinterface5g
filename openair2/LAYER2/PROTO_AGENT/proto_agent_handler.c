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

/*! \file enb_agent_handler.c
 * \brief enb agent tx and rx message handler 
 * \author Navid Nikaein and Xenofon Foukas 
 * \date 2016
 * \version 0.1
 */


#include "proto_agent_common.h"
#include "common/utils/LOG/log.h"
#include "assertions.h"

proto_agent_message_decoded_callback proto_agent_messages_callback[][3] = {
  {proto_agent_hello, 0, 0},                 /* agent hello */
  {proto_agent_echo_reply, 0, 0},            /* echo */
  {0, just_print, 0},                        /* just print */
  {proto_agent_pdcp_data_req_process, proto_agent_pdcp_data_req_process, 0}, /* PDCP data REQ */
  {0, proto_agent_get_ack_result, 0},        /* get ACK result */
  {proto_agent_pdcp_data_ind_process, proto_agent_pdcp_data_ind_process, 0}, /* PDCP data IND */
  {0, just_print, 0},                        /* just print */
};

proto_agent_message_destruction_callback proto_message_destruction_callback[] = {
  proto_agent_destroy_hello,
  proto_agent_destroy_echo_request,
  proto_agent_destroy_echo_reply,
  proto_agent_destroy_pdcp_data_req,
  0,
  proto_agent_destroy_pdcp_data_ind,
  0,
};

//static const char *proto_agent_direction2String[] = {
//  "", /* not_set  */
//  "originating message", /* originating message */
//  "successfull outcome", /* successfull outcome */
//  "unsuccessfull outcome", /* unsuccessfull outcome */
//};


Protocol__FlexsplitMessage* proto_agent_handle_message (mod_id_t mod_id,
						    uint8_t *data, 
						    int size){
  
  Protocol__FlexsplitMessage *decoded_message = NULL;
  Protocol__FlexsplitMessage *reply_message = NULL;
  err_code_t err_code;
  DevAssert(data != NULL);

  LOG_D(PROTO_AGENT, "Deserializing message with size %u \n", size);
  if (proto_agent_deserialize_message(data, (int) size, &decoded_message) < 0) {
    err_code= PROTOCOL__FLEXSPLIT_ERR__MSG_DECODING;
    goto error; 
  }
  /* after deserialization, we don't need the original data memory anymore */
  free(data);
  Protocol__FspHeader *header = (Protocol__FspHeader*) decoded_message;
  if (header->has_type)
   {
    LOG_D(PROTO_AGENT, "Deserialized MSG type is %d and %u\n", decoded_message->msg_case, decoded_message->msg_dir);
   }

  if ((decoded_message->msg_case > sizeof(proto_agent_messages_callback) / (3*sizeof(proto_agent_message_decoded_callback))) || 
      (decoded_message->msg_dir > PROTOCOL__FLEXSPLIT_DIRECTION__UNSUCCESSFUL_OUTCOME))
  {
      err_code= PROTOCOL__FLEXSPLIT_ERR__MSG_NOT_HANDLED;
      LOG_D(PROTO_AGENT,"Handling message: MSG NOT handled, going to error\n");
      goto error;
  }

  
  err_code = ((*proto_agent_messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1])(mod_id, (void *) decoded_message, &reply_message));

  if ( err_code < 0 )
  {
    LOG_I(PROTO_AGENT, "decoded_message case : %d, direction : %d \n", decoded_message->msg_case-1, decoded_message->msg_dir-1);
    goto error;
  }

  protocol__flexsplit_message__free_unpacked(decoded_message, NULL);
  LOG_D(PROTO_AGENT,"Returning REPLY message after the callback\n");
  return reply_message;
  
 error:
  LOG_E(PROTO_AGENT,"errno %d occured\n",err_code);
  return NULL;
}



uint8_t *proto_agent_pack_message(Protocol__FlexsplitMessage *msg, int *size)
{
  uint8_t *buffer;
  err_code_t err_code = PROTOCOL__FLEXSPLIT_ERR__NO_ERR;
  
  if (proto_agent_serialize_message(msg, &buffer, size) < 0 ) {
    err_code = PROTOCOL__FLEXSPLIT_ERR__MSG_ENCODING;
    goto error;
  }
  
  if (proto_message_destruction_callback[msg->msg_case-1])
    err_code = ((*proto_message_destruction_callback[msg->msg_case-1])(msg));
  
  DevAssert(buffer !=NULL);
  
  LOG_D(PROTO_AGENT,"Serialized the enb mac stats reply (size %d)\n", *size);
  return buffer;
  
 error : 
  LOG_E(PROTO_AGENT,"errno %d occured\n",err_code);
  
  return NULL;   
}

err_code_t proto_agent_destroy_flexsplit_message(Protocol__FlexsplitMessage *msg) {
  return ((*proto_message_destruction_callback[msg->msg_case-1])(msg));
}
