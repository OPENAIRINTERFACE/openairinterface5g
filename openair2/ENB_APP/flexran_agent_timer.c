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

/*! \file flexran_agent_timer.c
 * \brief FlexRAN Timer
 * \author Robert Schmidt
 * \date 2019
 * \version 0.2
 */

#include "flexran_agent_timer.h"
#include "flexran_agent_extern.h"
#include <pthread.h>
#include <sys/timerfd.h>

#define MAX_NUM_TIMERS 10

typedef struct flexran_agent_timer_element_s {
  mid_t mod_id;
  xid_t xid; /* The id of the task as received by the controller message*/
  uint32_t sf;
  uint64_t next;
  flexran_agent_timer_callback_t cb;
  Protocol__FlexranMessage *msg;
} flexran_agent_timer_element_t;

typedef struct flexran_agent_timer_extern_source_s {
  pthread_mutex_t mutex_sync;
  pthread_cond_t  cond;
  int             wait;
} flexran_agent_timer_extern_source_t;

struct timesync {
  void      *param_sync;
  void     (*fsync)(void *);
  pthread_t  thread;

  uint64_t current;
  uint64_t next;

  int                            timer_num;
  flexran_agent_timer_element_t *timer[MAX_NUM_TIMERS];

  int                            add_num;
  flexran_agent_timer_element_t *add_list[MAX_NUM_TIMERS];
  int                            remove_num;
  int                            remove_list[MAX_NUM_TIMERS];
  pthread_mutex_t                mutex_timer;

  int             exit;
};
struct timesync timesync[NUM_MAX_ENB];

int  flexran_agent_timer_signal_init(struct timesync *sync);
int  flexran_agent_timer_source_setup(struct timesync *sync);
void *flexran_agent_timer_thread(void *args);
void flexran_agent_timer_remove_internal(struct timesync *sync, int index);
void flexran_agent_timer_process(flexran_agent_timer_element_t *t);


err_code_t flexran_agent_timer_init(mid_t mod_id) {
  struct timesync *sync = &timesync[mod_id];
  sync->current = 0;
  sync->next    = 0;
  sync->timer_num    = 0;
  sync->add_num = 0;
  sync->remove_num = 0;
  for (int i = 0; i < MAX_NUM_TIMERS; ++i) {
    sync->timer[i] = NULL;
    sync->add_list[i] = NULL;
  }
  pthread_mutex_init(&sync->mutex_timer, NULL);
  sync->exit   = 0;

  /* if there is a MAC, we assume we can have a tick from the MAC interface
   * (external tick source). Otherwise, generate a tick internally via the
   * timerfd linux library. The init functions set everything up and the thread
   * will use whatever is available. */
  int (*init)(struct timesync *) = flexran_agent_get_mac_xface(mod_id) ?
          flexran_agent_timer_signal_init : flexran_agent_timer_source_setup;
  if (init(timesync) < 0
      || pthread_create(&sync->thread, NULL, flexran_agent_timer_thread, sync) != 0) {
    sync->thread = 0;
    if (sync->param_sync)
      free(sync->param_sync);
    LOG_E(FLEXRAN_AGENT, "could not start timer thread\n");
    return TIMER_SETUP_FAILED;
  }
  return PROTOCOL__FLEXRAN_ERR__NO_ERR;
}

void flexran_agent_timer_exit(mid_t mod_id) {
  timesync[mod_id].exit = 1;
}

void flexran_agent_timer_source_sync(void *sync) {
  int fd = *(int *)sync;
  uint64_t occ;
  int rc __attribute__((unused)) = read(fd, &occ, sizeof(occ));
}

int flexran_agent_timer_source_setup(struct timesync *sync) {
  sync->param_sync = malloc(sizeof(int));
  if (!sync->param_sync)
    return -1;
  int fd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (fd < 0)
    return fd;

  const uint64_t period_ns = 1000000; // 1ms
  struct itimerspec t;
  t.it_interval.tv_sec = period_ns / 1000000000;
  t.it_interval.tv_nsec = period_ns % 1000000000;
  t.it_value.tv_sec = period_ns / 1000000000;
  t.it_value.tv_nsec = period_ns % 1000000000;
  int rc = timerfd_settime(fd, 0, &t, NULL);
  if (rc != 0)
    return -1;
  *(int *)sync->param_sync = fd;
  sync->fsync = flexran_agent_timer_source_sync;
  return 0;
}

void flexran_agent_timer_signal_sync(void *param_sync) {
  flexran_agent_timer_extern_source_t *sync = param_sync;
  pthread_mutex_lock(&sync->mutex_sync);
  while (sync->wait) {
    pthread_cond_wait(&sync->cond, &sync->mutex_sync);
  }
  sync->wait = 1;
  pthread_mutex_unlock(&sync->mutex_sync);
}

int flexran_agent_timer_signal_init(struct timesync *sync) {
  flexran_agent_timer_extern_source_t *s = malloc(sizeof(flexran_agent_timer_extern_source_t));
  if (!s)
    return TIMER_SETUP_FAILED;
  if (pthread_mutex_init(&s->mutex_sync, NULL) < 0)
    return TIMER_SETUP_FAILED;
  if (pthread_cond_init(&s->cond, NULL) < 0)
    return TIMER_SETUP_FAILED;
  s->wait    = 1;
  sync->param_sync = s;
  sync->fsync = flexran_agent_timer_signal_sync;
  return 0;
}

void flexran_agent_timer_signal(mid_t mod_id) {
  flexran_agent_timer_extern_source_t *sync = timesync[mod_id].param_sync;
  pthread_mutex_lock(&sync->mutex_sync);
  sync->wait = 0;
  pthread_mutex_unlock(&sync->mutex_sync);
  pthread_cond_signal(&sync->cond);
}

void *flexran_agent_timer_thread(void *args) {
  struct timesync *sync = args;
  while (1) {
    sync->fsync(sync->param_sync);
    if (sync->exit)
      break;

    pthread_mutex_lock(&sync->mutex_timer);
    for (int i = 0; i < sync->add_num; ++i) {
      sync->timer[sync->timer_num] = sync->add_list[i];
      sync->timer_num++;
    }
    sync->add_num = 0;
    for (int i = 0; i < sync->remove_num; ++i)
      flexran_agent_timer_remove_internal(sync, sync->remove_list[i]);
    sync->remove_num = 0;
    pthread_mutex_unlock(&sync->mutex_timer);

    sync->current++;
    if (sync->current < sync->next)
      continue;

    for (int i = 0; i < sync->timer_num; ++i) {
      flexran_agent_timer_element_t *t = sync->timer[i];
      if (sync->current == t->next) {
        flexran_agent_timer_process(t);
        t->next += t->sf;
      }
      if (sync->next == sync->current || t->next < sync->next)
        sync->next = t->next;
    }
  }
  LOG_W(FLEXRAN_AGENT, "terminated timer thread\n");
  return NULL;
}

void flexran_agent_timer_process(flexran_agent_timer_element_t *t) {
  Protocol__FlexranMessage *msg = t->cb(t->mod_id, t->msg);
  if (!msg)
    return;

  int size = 0;
  void *data = flexran_agent_pack_message(msg, &size);
  if (flexran_agent_msg_send(t->mod_id, FLEXRAN_AGENT_DEFAULT, data, size, 0) < 0)
    LOG_E(FLEXRAN_AGENT, "error while sending message for timer xid %d\n", t->xid);
}


err_code_t flexran_agent_create_timer(mid_t    mod_id,
                                      uint32_t sf,
                                      flexran_agent_timer_type_t timer_type,
                                      xid_t    xid,
                                      flexran_agent_timer_callback_t cb,
                                      Protocol__FlexranMessage *msg) {
  if (sf == 0)
    return TIMER_NULL;
  if (cb == NULL)
    return TIMER_SETUP_FAILED;
  AssertFatal(timer_type != FLEXRAN_AGENT_TIMER_TYPE_ONESHOT,
              "one shot timer not yet implemented\n");

  flexran_agent_timer_element_t *t = malloc(sizeof(flexran_agent_timer_element_t));
  if (!t) {
    LOG_E(FLEXRAN_AGENT, "no memory for new timer %d\n", xid);
    return TIMER_SETUP_FAILED;
  }
  t->mod_id = mod_id;
  t->xid    = xid;
  t->sf     = sf;
  t->cb     = cb;
  t->msg    = msg;

  struct timesync *sync = &timesync[mod_id];
  pthread_mutex_lock(&sync->mutex_timer);
  if (sync->timer_num + sync->add_num >= MAX_NUM_TIMERS) {
    pthread_mutex_unlock(&sync->mutex_timer);
    LOG_E(FLEXRAN_AGENT, "maximum number of timers (%d) reached while adding timer %d\n",
          sync->timer_num + sync->add_num, xid);
    free(t);
    return TIMER_SETUP_FAILED;
  }
  /* TODO check that xid does not exist? */
  t->next = sync->current + sf;
  if (sync->next <= sync->current || t->next < sync->next)
    sync->next = t->next;
  sync->add_list[sync->add_num] = t;
  sync->add_num++;
  pthread_mutex_unlock(&sync->mutex_timer);
  LOG_I(FLEXRAN_AGENT, "added new timer xid %d for agent %d\n", xid, mod_id);
  return 0;
}

err_code_t flexran_agent_destroy_timer(mid_t mod_id, xid_t xid) {
  struct timesync *sync = &timesync[mod_id];
  for (int i = 0; i < sync->timer_num; ++i) {
    if (sync->timer[i]->xid == xid) {
      pthread_mutex_lock(&sync->mutex_timer);
      sync->remove_list[sync->remove_num] = i;
      sync->remove_num++;
      pthread_mutex_unlock(&sync->mutex_timer);
      return 0;
    }
  }
  LOG_E(FLEXRAN_AGENT, "could not find timer %d\n", xid);
  return TIMER_ELEMENT_NOT_FOUND;
}

/* this function assumes that the timer lock is held */
void flexran_agent_timer_remove_internal(struct timesync *sync, int index) {
  LOG_I(FLEXRAN_AGENT, "remove timer xid %d (index %d) for agent %d\n",
        sync->timer[index]->xid, index, sync->timer[index]->mod_id);
  if (sync->timer[index]->msg)
    flexran_agent_destroy_flexran_message(sync->timer[index]->msg);
  free(sync->timer[index]);
  for (int i = index + 1; i < sync->timer_num; ++i)
    sync->timer[i - 1] = sync->timer[i];
  sync->timer_num--;
  sync->timer[sync->timer_num] = NULL;
}
