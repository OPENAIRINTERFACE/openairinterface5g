/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2015 Eurecom

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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/
/*! \file link_manager.c
 * \brief this is the implementation of a link manager
 * \author Cedric Roux
 * \date November 2015
 * \version 1.0
 * \email: cedric.roux@eurecom.fr
 * @ingroup _mac
 */
//#ifndef SERVER_TEST
//#define SERVER_TEST
//#endif

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
      link_send_packet(manager->socket_link, data, size, manager->type, manager->peer_addr, manager->port);
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

error:
  LOG_E(MAC, "%s: error\n", __FUNCTION__);
  return NULL;
}

/* that thread receives messages from the link and puts them in the queue */
static void *link_manager_receiver_thread(void *_manager)
{
  link_manager_t *manager = _manager;
  void           *data;
  int            size;

  LOG_D(MAC, "starting link manager receiver thread\n");

  while (manager->run) {
    if (link_receive_packet(manager->socket_link, &data, &size, manager->type, manager->peer_addr, manager->port))
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
        socket_link_t   *link,
	uint16_t 	type,
	char 		*peer_addr,
 	int		port	  )
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
  ret->type = type;
  ret->peer_addr = peer_addr;
  ret->port = port;
  ret->run = 1;

  if (pthread_attr_init(&attr))
    goto error;

  // Make the async interface threads real-time
  //#ifndef LOWLATENCY
  struct sched_param sched_param_recv_thread;
  struct sched_param sched_param_send_thread;

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
