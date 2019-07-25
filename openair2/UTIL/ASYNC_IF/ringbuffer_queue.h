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

/*! \file ringbuffer_queue.h
 * \brief Lock-free ringbuffer used for async message passing of agent
 * \author Xenofon Foukas
 * \date March 2016
 * \version 1.0
 * \email: x.foukas@sms.ed.ac.uk
 * @ingroup _mac
 */

#ifndef RINGBUFFER_QUEUE_H
#define RINGBUFFER_QUEUE_H

#include "liblfds700.h"

typedef struct message_s {
  void *data;
  int size;
  int priority;
} message_t;

typedef struct {
  struct lfds700_misc_prng_state ps;
  struct lfds700_ringbuffer_element *ringbuffer_array;
  struct lfds700_ringbuffer_state ringbuffer_state;
} message_queue_t;

message_queue_t * new_message_queue(int size);
int message_put(message_queue_t *queue, void *data, int size, int priority);
int message_get(message_queue_t *queue, void **data, int *priority);
void message_get_unlock(message_queue_t *queue);
void destroy_message_queue(message_queue_t *queue);

#endif /* RINGBUFFER_QUEUE_H */
