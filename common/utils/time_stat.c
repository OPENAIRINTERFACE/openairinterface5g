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

#include "time_stat.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "common/utils/LOG/log.h"

time_average_t *time_average_new(int duration, int initial_size)
{
  time_average_t *ret;

  /* let's only accept power of two initial_size */
  if (initial_size & (initial_size - 1)) {
    LOG_E(UTIL, "time_average_new: illegal initial_size %d, use power of two\n", initial_size);
    exit(1);
  }

  ret = calloc(1, sizeof(time_average_t));
  if (ret == NULL) {
    LOG_E(UTIL, "out of memory\n");
    exit(1);
  }

  ret->duration = duration;
  ret->r.head = initial_size - 1;
  ret->r.maxsize = initial_size;
  ret->r.buffer = calloc(initial_size, sizeof(time_value_t));
  if (ret->r.buffer == NULL) {
    LOG_E(UTIL, "out of memory\n");
    exit(1);
  }

  return ret;
}

void time_average_free(time_average_t *t)
{
  free(t->r.buffer);
  free(t);
}

static void remove_old(time_average_t *t, uint64_t time)
{
  /* remove old events */
  while (t->r.size && t->r.buffer[t->r.tail].time < time - t->duration) {
    t->accumulated_value -= t->r.buffer[t->r.tail].value;
    t->r.size--;
    t->r.tail++;
    t->r.tail %= t->r.maxsize;
  }
}

void time_average_add(time_average_t *t, uint64_t time, uint64_t value)
{
  remove_old(t, time);

  if (t->r.size == t->r.maxsize) {
    t->r.maxsize *= 2;
    t->r.buffer = realloc(t->r.buffer, t->r.maxsize * sizeof(time_value_t));
    if (t->r.buffer == NULL) {
      LOG_E(UTIL, "out of memory\n");
      exit(1);
    }
    if (t->r.head < t->r.tail) {
      memcpy(&t->r.buffer[t->r.size], &t->r.buffer[0], (t->r.head + 1) * sizeof(time_value_t));
      t->r.head += t->r.size;
    }
  }

  t->r.head++;
  t->r.head %= t->r.maxsize;
  t->r.buffer[t->r.head].time = time;
  t->r.buffer[t->r.head].value = value;

  t->r.size++;

  t->accumulated_value += value;
}

double time_average_get_average(time_average_t *t, uint64_t time)
{
  remove_old(t, time);

  return (double)t->accumulated_value / t->r.size;
}

uint64_t time_average_now(void)
{
  struct timespec t;
  uint64_t ret;

  if (clock_gettime(CLOCK_REALTIME, &t)) {
    LOG_E(UTIL, "clock_gettime failed\n");
    exit(1);
  }

  ret = (uint64_t)t.tv_sec * (uint64_t)1000000 + t.tv_nsec / 1000;
  /* round up if necessary */
  if (t.tv_nsec % 1000 >= 500)
    ret++;

  return ret;
}
