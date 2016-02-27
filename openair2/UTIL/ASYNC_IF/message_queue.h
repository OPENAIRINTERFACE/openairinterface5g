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
} message_queue_t;

message_queue_t *new_message_queue(void);
int message_put(message_queue_t *queue, void *data, int size, int priority);
int message_get(message_queue_t *queue, void **data, int *size, int *priority);
void destroy_message_queue(message_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif /* MESSAGE_QUEUE_H */
