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

#include "nr_pdcp_timer_thread.h"

#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "LOG/log.h"

static pthread_mutex_t   timer_thread_mutex   = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t    timer_thread_cond    = PTHREAD_COND_INITIALIZER;
static volatile uint64_t timer_thread_curtime = 0;

static void *nr_pdcp_timer_thread(void *_nr_pdcp_ue_manager)
{
  nr_pdcp_ue_manager_t *nr_pdcp_ue_manager = (nr_pdcp_ue_manager_t *)_nr_pdcp_ue_manager;
  nr_pdcp_ue_t         **ue_list;
  int                  ue_count;
  int                  i;
  int                  j;
  uint64_t             curtime = 0;

  while (1) {
    if (pthread_mutex_lock(&timer_thread_mutex) != 0) abort();
    while (curtime == timer_thread_curtime)
      if (pthread_cond_wait(&timer_thread_cond, &timer_thread_mutex) != 0) abort();
    curtime = timer_thread_curtime;
    if (pthread_mutex_unlock(&timer_thread_mutex) != 0) abort();

    nr_pdcp_manager_lock(nr_pdcp_ue_manager);

    ue_list  = nr_pdcp_manager_get_ue_list(nr_pdcp_ue_manager);
    ue_count = nr_pdcp_manager_get_ue_count(nr_pdcp_ue_manager);
    for (i = 0; i < ue_count; i++) {
      for (j = 0; j < 2; j++) {
        if (ue_list[i]->srb[j] != NULL)
          ue_list[i]->srb[j]->set_time(ue_list[i]->srb[j], curtime);
      }
      for (j = 0; j < 5; j++) {
        if (ue_list[i]->drb[j] != NULL)
          ue_list[i]->drb[j]->set_time(ue_list[i]->drb[j], curtime);
      }
    }

    nr_pdcp_manager_unlock(nr_pdcp_ue_manager);
  }

  return NULL;
}

void nr_pdcp_init_timer_thread(nr_pdcp_ue_manager_t *nr_pdcp_ue_manager)
{
  pthread_t t;

  if (pthread_create(&t, NULL, nr_pdcp_timer_thread, nr_pdcp_ue_manager) != 0) {
    LOG_E(PDCP, "%s:%d:%s: fatal\n", __FILE__, __LINE__, __FUNCTION__);
    exit(1);
  }
}

void nr_pdcp_wakeup_timer_thread(uint64_t time)
{
  if (pthread_mutex_lock(&timer_thread_mutex)) abort();
  timer_thread_curtime = time;
  if (pthread_cond_broadcast(&timer_thread_cond)) abort();
  if (pthread_mutex_unlock(&timer_thread_mutex)) abort();
}
