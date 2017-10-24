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

/*! \file link_manager.h
 * \brief this is the implementation of a link manager
 * \author Cedric Roux
 * \date November 2015
 * \version 1.0
 * \email: cedric.roux@eurecom.fr
 * @ingroup _mac
 */

#ifndef LINK_MANAGER_H
#define LINK_MANAGER_H

//#include "message_queue.h"
#include "ringbuffer_queue.h"
#include "socket_link.h"

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  message_queue_t *send_queue;
  message_queue_t *receive_queue;
  socket_link_t   *socket_link;
  pthread_t       sender;
  pthread_t       receiver;
  volatile int    run;
} link_manager_t;

link_manager_t *create_link_manager(
        message_queue_t *send_queue,
        message_queue_t *receive_queue,
        socket_link_t   *link);
void destroy_link_manager(link_manager_t *);

#ifdef __cplusplus
}
#endif

#endif /* LINK_MANAGER_H */
