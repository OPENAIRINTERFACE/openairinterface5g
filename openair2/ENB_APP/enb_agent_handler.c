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
#include "enb_agent_mac.h"
#include "log.h"

#include "assertions.h"

enb_agent_message_decoded_callback messages_callback[][3] = {
  {enb_agent_hello, 0, 0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_HELLO_MSG*/
  {enb_agent_echo_reply, 0, 0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REQUEST_MSG*/
  {0, 0, 0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_ECHO_REPLY_MSG*/ //Must add handler when receiving echo reply
  {enb_agent_mac_reply, 0, 0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_STATS_REQUEST_MSG*/
  {0,0,0}, /*PROTOCOL__PROGRAN_MESSAGE__MSG_STATS_REPLY_MSG*/

};

static const char *enb_agent_direction2String[] = {
  "", /* not_set  */
  "originating message", /* originating message */
  "successfull outcome", /* successfull outcome */
  "unsuccessfull outcome", /* unsuccessfull outcome */
};


Protocol__ProgranMessage* enb_agent_handle_message (uint32_t xid, 
						    uint8_t *data, 
						    uint32_t size){
  
  Protocol__ProgranMessage *decoded_message, *reply_message;
  err_code_t err_code;
  DevAssert(data != NULL);

  if (enb_agent_deserialize_message(data, size, &decoded_message) < 0) {
    err_code= PROTOCOL__PROGRAN_ERR__MSG_DECODING;
    goto error; 
  }
  
  if ((decoded_message->msg_case > sizeof(messages_callback) / (3*sizeof(enb_agent_message_decoded_callback))) || 
      (decoded_message->msg_dir > PROTOCOL__PROGRAN_DIRECTION__UNSUCCESSFUL_OUTCOME)){
    err_code= PROTOCOL__PROGRAN_ERR__MSG_NOT_HANDLED;
      goto error;
  }
    
  if (messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1] == NULL) {
    err_code= PROTOCOL__PROGRAN_ERR__MSG_NOT_SUPPORTED;
    goto error;

  }

  err_code= ((*messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1])(xid, &reply_message));
  if ( err_code < 0 ){
    goto error;
  }
  
  protocol__progran_message__free_unpacked(decoded_message, NULL);

  return reply_message;
  
error:
  LOG_E(ENB_APP,"errno %d occured\n",err_code);
  return err_code;

}



void * enb_agent_send_message(uint32_t xid, 
			   Protocol__ProgranMessage *msg, 
			   uint32_t * size){

  void * buffer;
  err_code_t err_code = PROTOCOL__PROGRAN_ERR__NO_ERR;
  
  if (enb_agent_serialize_message(msg, &buffer, size) < 0 ) {
    err_code = PROTOCOL__PROGRAN_ERR__MSG_ENCODING;
    goto error;
  }
  
  // free the msg --> later keep this in the data struct and just update the values
  //TODO call proper destroy function
  //  enb_agent_mac_destroy_stats_reply(msg);
  
  DevAssert(buffer !=NULL);

  LOG_D(ENB_APP,"Serilized the enb mac stats reply (size %d)\n", *size);

  return buffer;

error :
LOG_E(ENB_APP,"errno %d occured\n",err_code);
return NULL; 
  
}








