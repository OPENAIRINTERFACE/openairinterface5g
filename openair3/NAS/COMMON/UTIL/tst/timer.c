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

/*****************************************************************************
Source    timer.c

Version   0.1

Date    2012/11/27

Product   Test

Subsystem Timer utility main test

Author    Frederic Maurel

Description Tests the timer utility functions

*****************************************************************************/

#include "nas_timer.h"
#include "commonDef.h"

#include <pthread.h>
#include <stdio.h>  // printf
#include <stdlib.h> // exit
#include <time.h> // clock_gettime
#include <poll.h> // poll

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

#define TIMER_1S  1 /* 1 second  */
#define TIMER_2S  2 /* 2 seconds */
#define TIMER_3S  3 /* 3 seconds */
#define TIMER_4S  4 /* 4 seconds */

typedef struct {
  unsigned int id;
  unsigned int sec;
  struct timespec start;
} _timer_t;

static void* _timer_callback(void* args);
static void* _timer_callback_joinable(void* args);

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned int _nb_timers = 0;
static int _start(_timer_t* timer, unsigned int sec);
static int _stop(_timer_t* timer);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

int main (int argc, const char* argv[])
{
  int rc;
  _timer_t timer1, timer2, timer3, timer4;

#define NB_TIMERS_MAX 10
  _timer_t timer[NB_TIMERS_MAX];

  /* Initialize timer utility */
  rc = timer_init();

  if (rc == RETURNerror) {
    printf("ERROR: timer_init() failed\n");
    exit (EXIT_FAILURE);
  }

  /* Start NB_TIMERS_MAX timers to expire at time interval of 1s */
  for (int i=0; i < NB_TIMERS_MAX; i++) {
    if (_start(&timer[i], i) != RETURNok) {
      printf("ERROR: timer_start(i=%u) failed\n", i);
    }
  }

  /* Start timer 1 to expire in 1s */
  if (_start(&timer1, TIMER_1S) != RETURNok) {
    printf("ERROR: timer_start() failed\n");
  }

  /* Start timer 2 to expire in 3s */
  if (_start(&timer2, TIMER_3S) != RETURNok) {
    printf("ERROR: timer_start() failed\n");
  }

  /* Start timer 3 to expire in 2s */
  if (_start(&timer3, TIMER_2S) != RETURNok) {
    printf("ERROR: timer_start() failed\n");
  }

  /* Stop timer 1 */
  if (_stop(&timer1) != RETURNok) {
    printf("ERROR: timer_stop(id=%u) failed\n", timer3.id);
  }

  /* Wait for the first timer to expire */
  poll(0, 0, -1);

  /* Start timer 4 to expire in 4s */
  if (_start(&timer4, TIMER_4S) != RETURNok) {
    printf("ERROR: timer_start() failed\n");
  }

  /* Wait for all timers to expire */
  while (_nb_timers > 0) {
    poll(0, 0, 10000);
  }

  exit(EXIT_SUCCESS);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

static int _timespec_sub(struct timespec* a, struct timespec* b, struct timespec* result)
{
  if (a->tv_sec <  b->tv_sec) return -1;
  else if (a->tv_nsec <  b->tv_nsec) return -1;

  result->tv_sec = a->tv_sec - b->tv_sec;
  result->tv_nsec = a->tv_nsec - b->tv_nsec;

  if (result->tv_nsec < 0) {
    result->tv_sec--;
    result->tv_nsec += 1000000000;
  }

  return 0;
}

static void* _timer_callback(void* args)
{
  _timer_t* timer = (_timer_t*) args;
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  if (_timespec_sub(&ts, &timer->start, &ts) < 0) {
    printf("ERROR:_timespec_sub() failed: %ld.%.9ld > %ld.%.9ld\n",
           timer->start.tv_sec, timer->start.tv_nsec,
           ts.tv_sec, ts.tv_nsec);
  } else {
    printf("%s\t: Timer %u expired after %ld.%.9ld seconds (%u s)\n",
           __FUNCTION__, timer->id, ts.tv_sec, ts.tv_nsec, timer->sec);
    timer->id = timer_stop(timer->id);
    pthread_mutex_lock(&_mutex);
    _nb_timers -= 1;
    pthread_mutex_unlock(&_mutex);
  }

  return NULL;
}

static void* _timer_callback_joinable(void* args)
{
  _timer_t* timer = (_timer_t*) args;
  struct timespec ts;
  int* rc = (int*)malloc(sizeof(int));

  clock_gettime(CLOCK_MONOTONIC, &ts);

  if (_timespec_sub(&ts, &timer->start, &ts) < 0) {
    printf("ERROR:_timespec_sub() failed: %ld.%.9ld > %ld.%.9ld\n",
           timer->start.tv_sec, timer->start.tv_nsec,
           ts.tv_sec, ts.tv_nsec);
    *rc = RETURNerror;
  } else {
    printf("%s\t: Timer %u expired after %ld.%.9ld seconds (%u s)\n",
           __FUNCTION__, timer->id, ts.tv_sec, ts.tv_nsec, timer->sec);
    timer->id = timer_stop(timer->id);
    pthread_mutex_lock(&_mutex);
    _nb_timers -= 1;
    pthread_mutex_unlock(&_mutex);
    *rc = timer->id;
  }

  return rc;
}

static int _start(_timer_t* timer, unsigned int sec)
{
  int rc = RETURNerror;

  timer->sec = sec;
  clock_gettime(CLOCK_MONOTONIC, &timer->start);
  timer->id = timer_start(timer->sec, _timer_callback_joinable,
                          (void*) timer);

  if (timer->id != TIMER_INACTIVE_ID) {
    printf("Timer id=%u scheduled to expire in %u seconds\n",
           timer->id, timer->sec);
    _nb_timers += 1;
    rc = RETURNok;
  }

  return (rc);
}

static int _stop(_timer_t* timer)
{
  int rc = RETURNerror;

  printf("Stop timer id=%u\n", timer->id);
  timer->id = timer_stop(timer->id);

  if (timer->id == TIMER_INACTIVE_ID) {
    _nb_timers -= 1;
    rc = RETURNok;
  }

  return (rc);
}

