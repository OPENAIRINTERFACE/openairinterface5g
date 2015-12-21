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


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "enb_agent_common.h"
#include "link_manager.h"
#include "log.h"
#include "enb_agent.h"

typedef uint8_t xid_t;  

// tx and rx shared context 
typedef struct {
  message_queue_t       *tx_mq;
  message_queue_t       *rx_mq;
  xid_t                 tx_xid;
  xid_t                 rx_xid; 
} msg_context_t;
msg_context_t shared_ctxt;

void *send_thread(void *arg);
void *receive_thread(void *arg);
pthread_t new_thread(void *(*f)(void *), void *b);

void *send_thread(void *arg) {

  msg_context_t         *d = arg;
  void                  *data;
  int                   size;
  int                   priority;

  
  while (1) {
    // need logic for the timer, and 
    usleep(10);
    if (message_put(d->tx_mq, data, size, priority)) goto error;
  }

  return NULL;

error:
  printf("receive_thread: there was an error\n");
  return NULL;
}

void *receive_thread(void *arg) {

  msg_context_t         *d = arg;
  void                  *data;
  int                   size;
  int                   priority;
  err_code_t             err_code;

  Protocol__ProgranMessage *msg;
  
  while (1) {
    if (message_get(d->rx_mq, &data, &size, &priority)){
      err_code = PROTOCOL__PROGRAN_ERR__MSG_DEQUEUING;
      goto error;
    }
    LOG_D(ENB_APP,"received message with size %d\n", size);
  
    
    msg=enb_agent_handle_message(d->rx_xid, data, size);

    free(data);
    
    d->rx_xid = ((d->rx_xid)+1)%4;
    d->tx_xid = d->rx_xid;
  
    // check if there is something to send back to the controller
    if (msg != NULL){
      data=enb_agent_send_message(d->tx_xid,msg,&size);
     
      if (message_put(d->tx_mq, data, size, priority)){
	err_code = PROTOCOL__PROGRAN_ERR__MSG_ENQUEUING;
	goto error;
      }
      LOG_D(ENB_APP,"sent message with size %d\n", size);
    }
    
  }

  return NULL;

error:
  printf("receive_thread: error %d occured\n",err_code);
  return NULL;
}


/* utility function to create a thread */
pthread_t new_thread(void *(*f)(void *), void *b) {
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att)){ 
    fprintf(stderr, "pthread_attr_init err\n"); 
    exit(1); 
  }
  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED)) { 
    fprintf(stderr, "pthread_attr_setdetachstate err\n"); 
    exit(1); 
  }
  if (pthread_create(&t, &att, f, b)) { 
    fprintf(stderr, "pthread_create err\n"); 
    exit(1); 
  }
  if (pthread_attr_destroy(&att)) { 
    fprintf(stderr, "pthread_attr_destroy err\n"); 
    exit(1); 
  }

  return t;
}

int enb_agent_start(){

  socket_link_t   *link;
  message_queue_t *send_queue;
  message_queue_t *receive_queue;
  link_manager_t  *manager;

  LOG_I(ENB_APP,"starting enb agent client\n");

  link = new_link_client("127.0.0.1", 2210);
  if (link == NULL) goto error;

  send_queue = new_message_queue();
  if (send_queue == NULL) goto error;
  receive_queue = new_message_queue();
  if (receive_queue == NULL) goto error;

  manager = create_link_manager(send_queue, receive_queue, link);
  if (manager == NULL) goto error;


  memset(&shared_ctxt, 0, sizeof(msg_context_t));
  
  shared_ctxt.tx_mq = send_queue;
  shared_ctxt.rx_mq = receive_queue;

  new_thread(receive_thread, &shared_ctxt);
  
  //  new_thread(send_thread, &shared_ctxt);

  // while (1) pause();

  printf("client ends\n");
  return 0;

error:
  printf("there was an error\n");
  return 1;

}



int enb_agent_stop(){


}
