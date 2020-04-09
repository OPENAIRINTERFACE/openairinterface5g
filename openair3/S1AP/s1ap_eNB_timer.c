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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <signal.h>
#include <time.h>
#include <errno.h>

#include "assertions.h"
#include "intertask_interface.h"
#include "s1ap_eNB_timer.h"
#include "common/utils/LOG/log.h"
#include "queue.h"
#include "s1ap_common.h"

struct s1ap_timer_elm_s {
  task_id_t                 task_id;    ///< Task ID which has requested the timer
  int32_t                   instance;   ///< Instance of the task which has requested the timer
  uint32_t                  timer_kind; ///< Instance of the task which has requested the timer
  timer_t                   timer;      ///< Unique timer id
  timer_type_t              type;       ///< Timer type
  void                     *timer_arg;  ///< Optional argument that will be passed when timer expires
  STAILQ_ENTRY(s1ap_timer_elm_s) entries;    ///< Pointer to next element
};

typedef struct timer_desc_s {
  STAILQ_HEAD(timer_list_head, s1ap_timer_elm_s) timer_queue;
  pthread_mutex_t timer_list_mutex;
  struct timespec timeout;
} timer_desc_t;

static timer_desc_t timer_desc;

#define TIMER_SEARCH(vAR, tIMERfIELD, tIMERvALUE, tIMERqUEUE)   \
do {                                                            \
    STAILQ_FOREACH(vAR, tIMERqUEUE, entries) {                  \
    if (((vAR)->tIMERfIELD == tIMERvALUE))                  \
            break;                                              \
    }                                                           \
} while(0)

int s1ap_timer_timeout(sigval_t info)
{
  struct s1ap_timer_elm_s   *timer_p;
  MessageDef                *message_p;
  s1ap_timer_has_expired_t  *timer_expired_p;
  task_id_t                 task_id;
  int32_t                   instance;
  uint32_t                  timer_kind;
  
  timer_kind = info.sival_int;
  if( pthread_mutex_lock(&timer_desc.timer_list_mutex) != 0 )
  {
    S1AP_ERROR("Failed to mutex lock timeout=%x\n", timer_kind);
    
    return -1;
  }
  TIMER_SEARCH(timer_p, timer_kind, timer_kind, &timer_desc.timer_queue);
  
  if( pthread_mutex_unlock(&timer_desc.timer_list_mutex) != 0 )
  {
    S1AP_ERROR("Failed to mutex unlock timeout=0x%x\n", timer_kind);
  }
  if (timer_p == NULL)
  {
    S1AP_ERROR("Didn't find timer 0x%x in list\n", timer_kind);
    
    return -1;
  }
  S1AP_DEBUG("Timer kind 0x%x has expired\n", timer_kind);
  
  task_id = timer_p->task_id;
  instance = timer_p->instance;
  message_p = itti_alloc_new_message(TASK_UNKNOWN, TIMER_HAS_EXPIRED);
  
  timer_expired_p = &message_p->ittiMsg.timer_has_expired;
  
  timer_expired_p->timer_id   = (long)timer_p->timer;
  timer_expired_p->timer_kind = timer_p->timer_kind;
  timer_expired_p->arg        = timer_p->timer_arg;
  
  /* Timer is a one shot timer, remove it */
  if( timer_p->type == S1AP_TIMER_ONE_SHOT )
  {
    if( s1ap_timer_remove((long)timer_p->timer) != 0 )
    {
      S1AP_DEBUG("Failed to delete timer 0x%lx\n", (long)timer_p->timer);
    }
  }
  
  /* Notify task of timer expiry */
  if( itti_send_msg_to_task(task_id, instance, message_p) < 0 )
  {
    S1AP_ERROR("Failed to send msg TIMER_HAS_EXPIRED to task %u\n", task_id);
    free(message_p);
    return -1;
  }
  
  return 0;
}

int s1ap_timer_setup(
  uint32_t      interval_sec,
  uint32_t      interval_us,
  task_id_t     task_id,
  int32_t       instance,
  uint32_t      timer_kind,
  timer_type_t  type,
  void         *timer_arg,
  long         *timer_id)
{
  struct sigevent          evp;
  struct itimerspec        its;
  struct s1ap_timer_elm_s  *timer_p;
  timer_t                  timer;
  
  if( timer_id == NULL )
  {
    S1AP_ERROR("Invalid timer_id\n");
    return -1;
  }
  
  if( type >= S1AP_TIMER_TYPE_MAX )
  {
    S1AP_ERROR("Invalid timer type (%d/%d)!\n", type, TIMER_TYPE_MAX);
    return -1;
  }
  
  if( *timer_id != S1AP_TIMERID_INIT )
  {
    if( s1ap_timer_remove(*timer_id) != 0 )
    {
      S1AP_ERROR("Failed to delete timer when the timer start 0x%lx\n", *timer_id);
    }
  }
  
  /* Allocate new timer list element */
  timer_p = malloc(sizeof(struct s1ap_timer_elm_s));
  
  if( timer_p == NULL )
  {
    S1AP_ERROR("Failed to create new timer element\n");
    return -1;
  }
  
  memset(&timer, 0, sizeof(timer_t));
  memset(&evp, 0, sizeof(evp));
  
  timer_p->task_id    = task_id;
  timer_p->instance   = instance;
  timer_p->timer_kind = timer_kind;
  timer_p->type       = type;
  timer_p->timer_arg  = timer_arg;
  
  evp.sigev_notify            = (int)SIGEV_THREAD;
  evp.sigev_notify_function   = (void *)s1ap_timer_timeout;
  evp.sigev_signo             = SIGRTMIN;
  evp.sigev_notify_attributes = NULL;
  evp.sigev_value.sival_int   = timer_kind;
  
  /* At the timer creation, the timer structure will be filled in with timer_id,
   * which is unique for this process. This id is allocated by kernel and the
   * value might be used to distinguish timers.
   */
  if( timer_create(CLOCK_REALTIME, &evp, &timer) < 0 )
  {
    S1AP_ERROR("Failed to create timer: (%s:%d)\n", strerror(errno), errno);
    free(timer_p);
    return -1;
  }
  
  /* Fill in the first expiration value. */
  its.it_value.tv_sec  = interval_sec;
  its.it_value.tv_nsec = interval_us * 1000;
  
  if( type == S1AP_TIMER_PERIODIC )
  {
    /* Asked for periodic timer. We set the interval time */
    its.it_interval.tv_sec  = interval_sec;
    its.it_interval.tv_nsec = interval_us * 1000;
  }
  else
  {
    /* Asked for one-shot timer. Do not set the interval field */
    its.it_interval.tv_sec  = 0;
    its.it_interval.tv_nsec = 0;
  }
  
  if( timer_settime(timer, 0, &its, NULL) )
  {
    S1AP_ERROR("Failed to Settimer: (%s:%d)\n", strerror(errno), errno);
    free(timer_p);
    return -1;
  }
  
  /* Simply set the timer_id argument. so it can be used by caller */
  *timer_id = (long)timer;
  
  timer_p->timer = timer;
  
  /* Lock the queue and insert the timer at the tail */
  if( pthread_mutex_lock(&timer_desc.timer_list_mutex) != 0 )
  {
    S1AP_ERROR("Failed to mutex lock\n");
    if( timer_delete(timer_p->timer) < 0 )
    {
      S1AP_ERROR("Failed to delete timer 0x%lx\n", (long)timer_p->timer);
    }
    free(timer_p);
    timer_p = NULL;
    
    return -1;
  }
  
  STAILQ_INSERT_TAIL(&timer_desc.timer_queue, timer_p, entries);
  if( pthread_mutex_unlock(&timer_desc.timer_list_mutex) != 0 )
  {
    S1AP_ERROR("Failed to mutex unlock\n");
  }
  
  return 0;
}

int s1ap_timer_remove(long timer_id)
{
  int rc = 0;
  struct s1ap_timer_elm_s *timer_p;
  
  
  if( pthread_mutex_lock(&timer_desc.timer_list_mutex) != 0 )
  {
    S1AP_ERROR("Failed to mutex lock\n");
    if( timer_delete(timer_id) < 0 )
    {
      S1AP_ERROR("Failed to delete timer 0x%lx\n", (long)timer_id);
    }
    
    return -1;
  }
  
  TIMER_SEARCH(timer_p, timer, ((timer_t)timer_id), &timer_desc.timer_queue);
  
  /* We didn't find the timer in list */
  if (timer_p == NULL)
  {
    S1AP_ERROR("Didn't find timer 0x%lx in list\n", timer_id);
    if( pthread_mutex_unlock(&timer_desc.timer_list_mutex) != 0 )
    {
      S1AP_ERROR("Failed to mutex unlock\n");
    }
    
    return -1;
  }
  
  timer_delete(timer_p->timer);
  
  STAILQ_REMOVE(&timer_desc.timer_queue, timer_p, s1ap_timer_elm_s, entries);
  
  if( pthread_mutex_unlock(&timer_desc.timer_list_mutex) != 0 )
  {
    S1AP_ERROR("Failed to mutex unlock\n");
  }
  
  free(timer_p);
  timer_p = NULL;
  return rc;
}

int s1ap_timer_init(void)
{
  memset(&timer_desc, 0, sizeof(timer_desc_t));
  
  STAILQ_INIT(&timer_desc.timer_queue);
  pthread_mutex_init(&timer_desc.timer_list_mutex, NULL);
  
  return 0;
}
