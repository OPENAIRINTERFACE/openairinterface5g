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

/*! \file message_queue.h
 * \brief this is the implementation of a message queue
 * \author Cedric Roux
 * \date November 2015
 * \version 1.0
 * \email: cedric.roux@eurecom.fr
 * @ingroup _mac
 */

#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct message_t {
  void *data;
  int  size;
  int  priority;

  struct message_t *next;
} message_t;

typedef struct {
  message_t       *head;
  message_t       *tail;
  volatile int    count;
  pthread_mutex_t *mutex;
  pthread_cond_t  *cond;
  int             exit;
} message_queue_t;

message_queue_t *new_message_queue(void);
int message_put(message_queue_t *queue, void *data, int size, int priority);
int message_get(message_queue_t *queue, void **data, int *priority);
void message_get_unlock(message_queue_t *queue);
void destroy_message_queue(message_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif /* MESSAGE_QUEUE_H */
