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
/*! \file ringbuffer_queue.c
 * \brief Lock-free ringbuffer used for async message passing of agent
 * \author Xenofon Foukas
 * \date March 2016
 * \version 1.0
 * \email: x.foukas@sms.ed.ac.uk
 * @ingroup _mac
 */

#include "ringbuffer_queue.h"
#include "log.h"

message_queue_t * new_message_queue(int size) {
  
  message_queue_t *ret = NULL;

  ret = calloc(1, sizeof(message_queue_t));
  if (ret == NULL)
    goto error;

  lfds700_misc_library_init_valid_on_current_logical_core();
  lfds700_misc_prng_init(&(ret->ps));
  ret->ringbuffer_array = malloc(sizeof(struct lfds700_ringbuffer_element) * size);
  lfds700_ringbuffer_init_valid_on_current_logical_core(&(ret->ringbuffer_state),
							ret->ringbuffer_array,
							size,
							&(ret->ps),
							NULL);

  return ret;
  
 error:
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  if (ret != NULL) {
    free(ret->ringbuffer_array);
    memset(ret, 0, sizeof(message_queue_t));
    free(ret);
  }
  return NULL;
}

int message_put(message_queue_t *queue, void *data, int size, int priority) {

  struct lfds700_misc_prng_state ls;
  enum lfds700_misc_flag overwrite_occurred_flag;
  message_t *overwritten_msg;
  message_t *m = NULL;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);
  
  m = calloc(1, sizeof(message_t));
  if (m == NULL)
    goto error;

  m->data = data;
  m->size = size;
  m->priority = priority;

  lfds700_ringbuffer_write(&(queue->ringbuffer_state),
			   NULL,
			   (void *) m,
			   &overwrite_occurred_flag,
			   NULL,
			   (void **) &overwritten_msg,
			   &ls);

  if (overwrite_occurred_flag == LFDS700_MISC_FLAG_RAISED) {
    free(overwritten_msg->data);
    free(overwritten_msg);
  }

  return 0;

 error:
  free(m);
  LOG_E(MAC, "%s: an error occured\n", __FUNCTION__);
  return -1;
}

int message_get(message_queue_t *queue, void **data, int *size, int *priority) {
  message_t *m;
  struct lfds700_misc_prng_state ls;
  
  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);

  if (lfds700_ringbuffer_read(&(queue->ringbuffer_state), NULL, (void **) &m, &ls) == 0) {
    return -1;
  }

  *data = m->data;
  *size = m->size;
  *priority = m->priority;
  free(m);
  return 0;
}

message_queue_t destroy_message_queue(message_queue_t *queue) {
  struct lfds700_misc_prng_state ls;

  message_t *m;

  LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE;
  lfds700_misc_prng_init(&ls);

  while (lfds700_ringbuffer_read(&(queue->ringbuffer_state), NULL, (void **) &m, &ls) != 0) {
    free(m->data);
    memset(m, 0, sizeof(message_t));
    free(m);
  }
  free(queue->ringbuffer_array);
  memset(queue, 0, sizeof(message_queue_t));
  free(queue);
}
