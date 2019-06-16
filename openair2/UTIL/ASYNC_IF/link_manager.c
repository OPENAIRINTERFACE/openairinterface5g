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

/*! \file link_manager.c
 * \brief this is the implementation of a link manager
 * \author Cedric Roux
 * \date November 2015
 * \version 1.0
 * \email: cedric.roux@eurecom.fr
 * @ingroup _mac
 */

#include "link_manager.h"
#include "common/utils/LOG/log.h"
#include <common/utils/assertions.h>
#include <common/utils/system.h>

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* that thread reads messages in the queue and sends them to the link */
static void *link_manager_sender_thread(void *_manager)
{
  link_manager_t *manager = _manager;
  void           *data;
  int            size;
  int            priority;

  LOG_D(MAC, "starting link manager sender thread\n");

  while (manager->run) {
    while ((size = message_get(manager->send_queue, &data, &priority)) > 0) {
      link_send_packet(manager->socket_link, data, size, manager->peer_addr, manager->peer_port);
      free(data);
    }
  }

  LOG_D(MAC, "link manager sender thread quits\n");

  return NULL;

  //error:
  //LOG_E(MAC, "%s: error\n", __FUNCTION__);
  //return NULL;
}

/* that thread receives messages from the link and puts them in the queue */
static void *link_manager_receiver_thread(void *_manager)
{
  link_manager_t *manager = _manager;
  void           *data;
  int            size;

  LOG_D(MAC, "starting link manager receiver thread\n");

  while (manager->run) {
    if (link_receive_packet(manager->socket_link, &data, &size))
      goto error;
    /* todo: priority */
    if (message_put(manager->receive_queue, data, size, 0))
      goto error;
  }

  LOG_D(MAC, "link manager receiver thread quits\n");

  return NULL;

error:
  LOG_E(MAC, "%s: error\n", __FUNCTION__);
  return NULL;
}

link_manager_t *create_link_manager(
        message_queue_t *send_queue,
        message_queue_t *receive_queue,
        socket_link_t   *link)
{
  link_manager_t *ret = NULL;
  //pthread_attr_t attr;
  pthread_t      t;

  LOG_D(MAC, "create new link manager\n");

  AssertFatal( (ret=calloc(1, sizeof(link_manager_t))) != NULL,"");

  ret->send_queue = send_queue;
  ret->receive_queue = receive_queue;
  ret->socket_link = link;
  ret->run = 1;

  threadCreate(&t, link_manager_sender_thread, ret, "flexranSender", -1, OAI_PRIORITY_RT_LOW);
  ret->sender = t;

  threadCreate(&t, link_manager_receiver_thread, ret, "flexranReceiver", -1, OAI_PRIORITY_RT_LOW);
  ret->receiver = t;

  return ret;

}

void destroy_link_manager(link_manager_t *manager)
{
  manager->run = 0;
  message_get_unlock(manager->send_queue);
  pthread_join(manager->sender, NULL);
  /* cancel aborts the read performed in the receiver, then cancels the thread */
  pthread_cancel(manager->receiver);
}

#ifdef SERVER_TEST

#include <string.h>

int main(void)
{
  socket_link_t   *link;
  message_queue_t *send_queue;
  message_queue_t *receive_queue;
  link_manager_t  *manager;
  void            *data;
  int             size;
  int             priority;

  printf("starting server\n");

  link = new_link_server(2210);
  if (link == NULL) goto error;

  send_queue = new_message_queue();
  if (send_queue == NULL) goto error;
  receive_queue = new_message_queue();
  if (receive_queue == NULL) goto error;

  manager = create_link_manager(send_queue, receive_queue, link);
  if (manager == NULL) goto error;

  data = strdup("hello"); if (data == NULL) goto error;
  if (message_put(send_queue, data, 6, 100)) goto error;

  if ((size = message_get(receive_queue, &data, &priority)) <= 0) goto error;
  printf("received message:\n");
  printf("    data: %s\n", (char *)data);
  printf("    size: %d\n", size);
  printf("    priority: %d\n", priority);

  printf("server ends\n");
  return 0;

error:
  printf("there was an error\n");
  return 1;
}

#endif

#ifdef CLIENT_TEST

#include <string.h>
#include <unistd.h>

int main(void)
{
  socket_link_t   *link;
  message_queue_t *send_queue;
  message_queue_t *receive_queue;
  link_manager_t  *manager;
  void            *data;
  int             size;
  int             priority;

  printf("starting client\n");

  link = new_link_client("127.0.0.1", 2210);
  if (link == NULL) goto error;

  send_queue = new_message_queue();
  if (send_queue == NULL) goto error;
  receive_queue = new_message_queue();
  if (receive_queue == NULL) goto error;

  manager = create_link_manager(send_queue, receive_queue, link);
  if (manager == NULL) goto error;

  if ((size = message_get(receive_queue, &data, &priority)) <= 0) goto error;
  printf("received message:\n");
  printf("    data: %s\n", (char *)data);
  printf("    size: %d\n", size);
  printf("    priority: %d\n", priority);

  data = strdup("world"); if (data == NULL) goto error;
  if (message_put(send_queue, data, 6, 200)) goto error;

  /* let's wait for the message to be sent (unreliable sleep, but does it for the test) */
  sleep(1);

  printf("client ends\n");
  return 0;

error:
  printf("there was an error\n");
  return 1;
}

#endif
