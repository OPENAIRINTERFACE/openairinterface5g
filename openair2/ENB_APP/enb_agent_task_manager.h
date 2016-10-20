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
/*! \file enb_agent_task_manager.h
 * \brief Implementation of scheduled tasks manager for the enb agent
 * \author Xenofon Foukas
 * \date January 2016
 * \version 0.1
 * \email: x.foukas@sms.ed.ac.uk
 * @ingroup _mac
 */

#ifndef ENB_AGENT_TASK_MANAGER_
#define ENB_AGENT_TASK_MANAGER_

#include <stdint.h>
#include <pthread.h>

#include "flexran.pb-c.h"

#include "enb_agent_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_CAPACITY 512
  
/**
 * The structure containing the enb agent task to be executed
 */
typedef struct enb_agent_task_s {
  /* The frame in which the task needs to be executed */
  uint16_t frame_num;
  /* The subframe in which the task needs to be executed */
  uint8_t subframe_num;
  /* The task to be executed in the form of a Protocol__FlexranMessage */
  Protocol__FlexranMessage *task;
} enb_agent_task_t;

/**
 * Priority Queue Structure for tasks
 */
typedef struct enb_agent_task_queue_s {
  mid_t mod_id;
  /* The amount of allocated memory for agent tasks in the heap*/
  volatile size_t capacity;
  /* The actual size of the tasks heap at a certain time */
  volatile size_t count;
  /* The earliest frame that has a pending task */
  volatile uint16_t first_frame;
  /* The earliest subframe within the frame that has a pending task */
  volatile uint8_t first_subframe;
  /* An array of prioritized tasks stored in a heap */
  enb_agent_task_t **task;
  /* A pointer to a comparator function, used to prioritize elements */
  int (*cmp)(mid_t mod_id, const enb_agent_task_t *t1, const enb_agent_task_t *t2);
  pthread_mutex_t *mutex;
} enb_agent_task_queue_t;

typedef enum {
  HEAP_OK = 0,
  HEAP_EMPTY,
  HEAP_FAILED,
  HEAP_REALLOCERROR,
  HEAP_NOREALLOC,
  HEAP_FATAL,
} heapstatus_e;
  
/**
 * Allocate memory for a task in the queue
 */
enb_agent_task_t *enb_agent_task_create(Protocol__FlexranMessage *msg,
				      uint16_t frame_num, uint8_t subframe_num);
  
/**
 * Free memory for a task of the queue
 */
void enb_agent_task_destroy(enb_agent_task_t *task);
  
/**
 * Allocate initial memory for storing the tasks
 */
  enb_agent_task_queue_t *enb_agent_task_queue_init(mid_t mod_id, size_t capacity,
						    int (*cmp)(mid_t mod_id, const enb_agent_task_t *t1, const enb_agent_task_t *t2));

/**
 * Allocate initial memory for storing the tasks using default parameters
 */
enb_agent_task_queue_t *enb_agent_task_queue_default_init(mid_t mod_id);  
  
/**
 * De-allocate memory for the tasks queue
 */
void enb_agent_task_queue_destroy(enb_agent_task_queue_t *queue);
  
/**
 * Insert task into the queue
 */
int enb_agent_task_queue_put(enb_agent_task_queue_t *queue, enb_agent_task_t *task); 

/**
 * Remove the task with the highest priority from the queue
 * task becomes NULL if there is no task for the current frame and subframe
 */
int enb_agent_task_queue_get_current_task(enb_agent_task_queue_t *queue, enb_agent_task_t **task);

/**
 * Check if the top priority task is for a specific frame and subframe
 */
int enb_agent_task_queue_has_upcoming_task (enb_agent_task_queue_t *queue,
					    const uint16_t frame, const uint8_t subframe);

/**
 * Restructure heap after modifications
 */
void _enb_agent_task_queue_heapify(enb_agent_task_queue_t *queue, size_t idx);

/**
 * Reallocate memory once the heap reaches max size
 */
int _enb_agent_task_queue_realloc_heap(enb_agent_task_queue_t *queue);

/**
 * Compare two agent tasks based on frame and subframe
 * returns 0 if tasks t1, t2 have the same priority
 * return negative value if t1 needs to be executed after t2
 * return positive value if t1 preceeds t2
 * Need to give eNB id for the comparisson based on the current frame-subframe
 */
  int _enb_agent_task_queue_cmp(mid_t mod_id, const enb_agent_task_t *t1, const enb_agent_task_t *t2);

#ifdef __cplusplus
}
#endif
  
#endif  /*ENB_AGENT_TASK_MANAGER_*/
