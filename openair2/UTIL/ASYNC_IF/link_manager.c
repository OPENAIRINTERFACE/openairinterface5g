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
#include "log.h"

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
    while (message_get(manager->send_queue, &data, &size, &priority) == 0) {
      link_send_packet(manager->socket_link, data, size);
      free(data);
    }
    //    if (message_get(manager->send_queue, &data, &size, &priority))
    //  goto error;
    //if (link_send_packet(manager->socket_link, data, size))
    //  goto error;
    //free(data);
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
  pthread_attr_t attr;
  pthread_t      t;

  LOG_D(MAC, "create new link manager\n");

  ret = calloc(1, sizeof(link_manager_t));
  if (ret == NULL)
    goto error;

  ret->send_queue = send_queue;
  ret->receive_queue = receive_queue;
  ret->socket_link = link;
  ret->run = 1;

  if (pthread_attr_init(&attr))
    goto error;

  // Make the async interface threads real-time
  //#ifndef LOWLATENCY
  struct sched_param sched_param_recv_thread;

  sched_param_recv_thread.sched_priority = sched_get_priority_max(SCHED_RR) - 1;
  pthread_attr_setschedparam(&attr, &sched_param_recv_thread);
  pthread_attr_setschedpolicy(&attr, SCHED_RR);
  //#endif

  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
    goto error;

  if (pthread_create(&t, &attr, link_manager_sender_thread, ret))
    goto error;
  ret->sender = t;

  if (pthread_create(&t, &attr, link_manager_receiver_thread, ret))
    /* we should destroy the other thread here */
    goto error;
  ret->receiver = t;

  if (pthread_attr_destroy(&attr))
    /* to be clean we should destroy the threads at this point,
     * even if in practice we never reach it */
    goto error;

  return ret;

error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  free(ret);
  return NULL;
}

void destroy_link_manager(link_manager_t *manager)
{
  LOG_D(MAC, "destroying link manager\n");
  manager->run = 0;
  /* todo: force threads to stop (using a dummy message?) */
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

  if (message_get(receive_queue, &data, &size, &priority)) goto error;
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

  if (message_get(receive_queue, &data, &size, &priority)) goto error;
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
