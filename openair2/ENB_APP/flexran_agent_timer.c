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
 * \author shahab SHARIAT BAGHERI
 * \date 2017
 * \version 0.1
 */

/*
 * timer primitives
 */

#include "flexran_agent_timer.h"

//struct flexran_agent_map agent_map;
flexran_agent_timer_instance_t timer_instance;
int agent_timer_init = 0;
err_code_t flexran_agent_init_timer(void) {
  LOG_I(FLEXRAN_AGENT, "init RB tree\n");

  if (!agent_timer_init) {
    RB_INIT(&timer_instance.flexran_agent_head);
    agent_timer_init = 1;
  }

  return PROTOCOL__FLEXRAN_ERR__NO_ERR;
}

RB_GENERATE(flexran_agent_map, flexran_agent_timer_element_s, entry, flexran_agent_compare_timer);

/* The timer_id might not be the best choice for the comparison */
int flexran_agent_compare_timer(struct flexran_agent_timer_element_s *a, struct flexran_agent_timer_element_s *b) {
  if (a->timer_id < b->timer_id) return -1;

  if (a->timer_id > b->timer_id) return 1;

  // equal timers
  return 0;
}

err_code_t flexran_agent_create_timer(uint32_t interval_sec,
                                      uint32_t interval_usec,
                                      agent_id_t     agent_id,
                                      instance_t     instance,
                                      uint32_t timer_type,
                                      xid_t xid,
                                      flexran_agent_timer_callback_t cb,
                                      void    *timer_args,
                                      long *timer_id) {
  struct flexran_agent_timer_element_s *e = calloc(1, sizeof(*e));
  DevAssert(e != NULL);
  //uint32_t timer_id;
  int ret=-1;

  if ((interval_sec == 0) && (interval_usec == 0 )) {
    free(e);
    return TIMER_NULL;
  }

  if (timer_type >= FLEXRAN_AGENT_TIMER_TYPE_MAX) {
    free(e);
    return TIMER_TYPE_INVALIDE;
  }

  if (timer_type  ==   FLEXRAN_AGENT_TIMER_TYPE_ONESHOT) {
    ret = timer_setup(interval_sec,
                      interval_usec,
                      TASK_FLEXRAN_AGENT,
                      instance,
                      TIMER_ONE_SHOT,
                      timer_args,
                      timer_id);

    e->type = TIMER_ONE_SHOT;
  } else if (timer_type  ==   FLEXRAN_AGENT_TIMER_TYPE_PERIODIC ) {
    ret = timer_setup(interval_sec,
                      interval_usec,
                      TASK_FLEXRAN_AGENT,
                      instance,
                      TIMER_PERIODIC,
                      timer_args,
                      timer_id);
    e->type = TIMER_PERIODIC;
  }

  if (ret < 0 ) {
    free(e);
    return TIMER_SETUP_FAILED;
  }

  e->agent_id = agent_id;
  e->instance = instance;
  e->state = FLEXRAN_AGENT_TIMER_STATE_ACTIVE;
  e->timer_id = *timer_id;
  e->xid = xid;
  e->timer_args = timer_args;
  e->cb = cb;
  /*element should be a real pointer*/
  RB_INSERT(flexran_agent_map, &timer_instance.flexran_agent_head, e);
  LOG_I(FLEXRAN_AGENT,"Created a new timer with id 0x%lx for agent %d, instance %d \n",
        e->timer_id, e->agent_id, e->instance);
  return 0;
}

err_code_t flexran_agent_destroy_timer(long timer_id) {
  struct flexran_agent_timer_element_s *e = get_timer_entry(timer_id);

  if (e != NULL ) {
    RB_REMOVE(flexran_agent_map, &timer_instance.flexran_agent_head, e);
    flexran_agent_destroy_flexran_message(e->timer_args->msg);
    free(e);
  }

  if (timer_remove(timer_id) < 0 )
    goto error;

  return 0;
error:
  LOG_E(FLEXRAN_AGENT, "timer can't be removed\n");
  return TIMER_REMOVED_FAILED ;
}

err_code_t flexran_agent_destroy_timer_by_task_id(xid_t xid) {
  struct flexran_agent_timer_element_s *e = NULL;
  long timer_id;
  RB_FOREACH(e, flexran_agent_map, &timer_instance.flexran_agent_head) {
    if (e->xid == xid) {
      timer_id = e->timer_id;
      RB_REMOVE(flexran_agent_map, &timer_instance.flexran_agent_head, e);
      flexran_agent_destroy_flexran_message(e->timer_args->msg);
      free(e);

      if (timer_remove(timer_id) < 0 ) {
        goto error;
      }
    }
  }
  return 0;
error:
  LOG_E(FLEXRAN_AGENT, "timer can't be removed\n");
  return TIMER_REMOVED_FAILED ;
}

err_code_t flexran_agent_destroy_timers(void) {
  struct flexran_agent_timer_element_s *e = NULL;
  RB_FOREACH(e, flexran_agent_map, &timer_instance.flexran_agent_head) {
    RB_REMOVE(flexran_agent_map, &timer_instance.flexran_agent_head, e);
    timer_remove(e->timer_id);
    flexran_agent_destroy_flexran_message(e->timer_args->msg);
    free(e);
  }
  return 0;
}

void flexran_agent_sleep_until(struct timespec *ts, int delay) {
  ts->tv_nsec += delay;

  if(ts->tv_nsec >= 1000*1000*1000) {
    ts->tv_nsec -= 1000*1000*1000;
    ts->tv_sec++;
  }

  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ts,  NULL);
}


err_code_t flexran_agent_stop_timer(long timer_id) {
  struct flexran_agent_timer_element_s *e=NULL;
  struct flexran_agent_timer_element_s search;
  memset(&search, 0, sizeof(struct flexran_agent_timer_element_s));
  search.timer_id = timer_id;
  e = RB_FIND(flexran_agent_map, &timer_instance.flexran_agent_head, &search);

  if (e != NULL ) {
    e->state =  FLEXRAN_AGENT_TIMER_STATE_STOPPED;
  }

  timer_remove(timer_id);
  return 0;
}

struct flexran_agent_timer_element_s *get_timer_entry(long timer_id) {
  struct flexran_agent_timer_element_s search;
  memset(&search, 0, sizeof(struct flexran_agent_timer_element_s));
  search.timer_id = timer_id;
  return  RB_FIND(flexran_agent_map, &timer_instance.flexran_agent_head, &search);
}
