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
int message_get(message_queue_t *queue, void **data, int *size, int *priority);
message_queue_t destroy_message_queue(message_queue_t *queue);

#endif /* RINGBUFFER_QUEUE_H */
