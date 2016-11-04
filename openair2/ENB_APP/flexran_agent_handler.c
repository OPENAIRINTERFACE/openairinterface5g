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

/*! \file flexran_agent_handler.c
 * \brief FlexRAN agent tx and rx message handler 
 * \author Xenofon Foukas and Navid Nikaein
 * \date 2016
 * \version 0.1
 */


#include "flexran_agent_common.h"
#include "flexran_agent_mac.h"
#include "log.h"

#include "assertions.h"

flexran_agent_message_decoded_callback agent_messages_callback[][3] = {
  {flexran_agent_hello, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_HELLO_MSG*/
  {flexran_agent_echo_reply, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REQUEST_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_ECHO_REPLY_MSG*/ //Must add handler when receiving echo reply
  {flexran_agent_mac_handle_stats, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REQUEST_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_STATS_REPLY_MSG*/
  {0, 0, 0}, /*PROTOCOK__FLEXRAN_MESSAGE__MSG_SF_TRIGGER_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_UL_SR_INFO_MSG*/
  {flexran_agent_enb_config_reply, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REQUEST_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_ENB_CONFIG_REPLY_MSG*/
  {flexran_agent_ue_config_reply, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_CONFIG_REQUEST_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_CONFIG_REPLY_MSG*/
  {flexran_agent_lc_config_reply, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_LC_CONFIG_REQUEST_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_LC_CONFIG_REPLY_MSG*/
  {flexran_agent_mac_handle_dl_mac_config, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_DL_MAC_CONFIG_MSG*/
  {0, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_UE_STATE_CHANGE_MSG*/
  {flexran_agent_control_delegation, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_CONTROL_DELEGATION_MSG*/
  {flexran_agent_reconfiguration, 0, 0}, /*PROTOCOL__FLEXRAN_MESSAGE__MSG_AGENT_RECONFIGURATION_MSG*/
};

flexran_agent_message_destruction_callback message_destruction_callback[] = {
  flexran_agent_destroy_hello,
  flexran_agent_destroy_echo_request,
  flexran_agent_destroy_echo_reply,
  flexran_agent_mac_destroy_stats_request,
  flexran_agent_mac_destroy_stats_reply,
  flexran_agent_mac_destroy_sf_trigger,
  flexran_agent_mac_destroy_sr_info,
  flexran_agent_destroy_enb_config_request,
  flexran_agent_destroy_enb_config_reply,
  flexran_agent_destroy_ue_config_request,
  flexran_agent_destroy_ue_config_reply,
  flexran_agent_destroy_lc_config_request,
  flexran_agent_destroy_lc_config_reply,
  flexran_agent_mac_destroy_dl_config,
  flexran_agent_destroy_ue_state_change,
  flexran_agent_destroy_control_delegation,
  flexran_agent_destroy_agent_reconfiguration,
};

static const char *flexran_agent_direction2String[] = {
  "", /* not_set  */
  "originating message", /* originating message */
  "successfull outcome", /* successfull outcome */
  "unsuccessfull outcome", /* unsuccessfull outcome */
};


Protocol__FlexranMessage* flexran_agent_handle_message (mid_t mod_id,
							uint8_t *data, 
							uint32_t size){
  
  Protocol__FlexranMessage *decoded_message, *reply_message;
  err_code_t err_code;
  DevAssert(data != NULL);

  if (flexran_agent_deserialize_message(data, size, &decoded_message) < 0) {
    err_code= PROTOCOL__FLEXRAN_ERR__MSG_DECODING;
    goto error; 
  }
  
  if ((decoded_message->msg_case > sizeof(agent_messages_callback) / (3 * sizeof(flexran_agent_message_decoded_callback))) || 
      (decoded_message->msg_dir > PROTOCOL__FLEXRAN_DIRECTION__UNSUCCESSFUL_OUTCOME)){
    err_code= PROTOCOL__FLEXRAN_ERR__MSG_NOT_HANDLED;
    goto error;
  }
    
  if (agent_messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1] == NULL) {
    err_code= PROTOCOL__FLEXRAN_ERR__MSG_NOT_SUPPORTED;
    goto error;

  }

  err_code = ((*agent_messages_callback[decoded_message->msg_case-1][decoded_message->msg_dir-1])(mod_id, (void *) decoded_message, &reply_message));
  if ( err_code < 0 ){
    goto error;
  } else if (err_code == 1) { //If err_code > 1, we do not want to dispose the message yet
    protocol__flexran_message__free_unpacked(decoded_message, NULL);
  }
  return reply_message;
  
error:
  LOG_E(FLEXRAN_AGENT,"errno %d occured\n",err_code);
  return NULL;

}



void * flexran_agent_pack_message(Protocol__FlexranMessage *msg, 
				  uint32_t * size){

  void * buffer;
  err_code_t err_code = PROTOCOL__FLEXRAN_ERR__NO_ERR;
  
  if (flexran_agent_serialize_message(msg, &buffer, size) < 0 ) {
    err_code = PROTOCOL__FLEXRAN_ERR__MSG_ENCODING;
    goto error;
  }
  
  // free the msg --> later keep this in the data struct and just update the values
  //TODO call proper destroy function
  err_code = ((*message_destruction_callback[msg->msg_case-1])(msg));
  
  DevAssert(buffer !=NULL);
  
  LOG_D(FLEXRAN_AGENT,"Serilized the enb mac stats reply (size %d)\n", *size);
  
  return buffer;
  
 error : 
  LOG_E(FLEXRAN_AGENT,"errno %d occured\n",err_code);
  
  return NULL;   
}

Protocol__FlexranMessage *flexran_agent_handle_timed_task(void *args) {
  err_code_t err_code;
  flexran_agent_timer_args_t *timer_args = (flexran_agent_timer_args_t *) args;

  Protocol__FlexranMessage *timed_task, *reply_message;
  timed_task = timer_args->msg;
  err_code = ((*agent_messages_callback[timed_task->msg_case-1][timed_task->msg_dir-1])(timer_args->mod_id, (void *) timed_task, &reply_message));
  if ( err_code < 0 ){
    goto error;
  }

  return reply_message;
  
 error:
  LOG_E(FLEXRAN_AGENT,"errno %d occured\n",err_code);
  return NULL;
}

Protocol__FlexranMessage* flexran_agent_process_timeout(long timer_id, void* timer_args){
    
  struct flexran_agent_timer_element_s *found = get_timer_entry(timer_id);
 
  if (found == NULL ) goto error;
  LOG_I(FLEXRAN_AGENT, "Found the entry (%p): timer_id is 0x%lx  0x%lx\n", found, timer_id, found->timer_id);
  
  if (timer_args == NULL)
    LOG_W(FLEXRAN_AGENT,"null timer args\n");
  
  return found->cb(timer_args);

 error:
  LOG_E(FLEXRAN_AGENT, "can't get the timer element\n");
  return TIMER_ELEMENT_NOT_FOUND;
}

err_code_t flexran_agent_destroy_flexran_message(Protocol__FlexranMessage *msg) {
  return ((*message_destruction_callback[msg->msg_case-1])(msg));
}
