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

/*! \file lte-softmodem.c
* \brief main program to control HW and scheduling
* \author R. Knopp, F. Kaltenberger
* \date 2012
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <execinfo.h>
#include <getopt.h>
#include <pthread.h>

#include "rt_wrapper.h"

#define TIMER_ONESHOT_MODE
#define FRAME_PERIOD 10000000ULL
#define DAQ_PERIOD 500000ULL
#define LTE_SLOTS_PER_FRAME  20

#ifdef RTAI
static SEM *mutex;
//static CND *cond;

static long int thread0;
static long int thread1;
//static long int sync_thread;
#else
#define OPENAIR_THREAD_STACK_SIZE    8192
#define OPENAIR_THREAD_PRIORITY        255
pthread_t thread0;
//pthread_t thread1;
pthread_attr_t attr_dlsch_threads;
struct sched_param sched_param_dlsch;
#endif

int oai_exit = 0;

struct timing_info_t {
  //unsigned int frame, hw_slot, last_slot, next_slot;
  RTIME time_min, time_max, time_avg, time_last, time_now;
  //unsigned int mbox0, mbox1, mbox2, mbox_target;
  unsigned int n_samples;
} timing_info;

void signal_handler(int sig)
{
  void *array[10];
  size_t size;

  if (sig==SIGSEGV) {
    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, 2);
    exit(-1);
  } else {
    oai_exit=1;
  }
}

void exit_fun(const char* s)
{
  void *array[10];
  size_t size;

  printf("Exiting: %s\n",s);

  oai_exit=1;
  //rt_sleep_ns(FRAME_PERIOD);

  //exit (-1);
}

int frame=0,slot=0;

/* This is the main eNB thread. It gets woken up by the kernel driver using the RTAI message mechanism (rt_send and rt_receive). */
static void *eNB_thread(void *arg)
{
#ifdef RTAI
  RT_TASK *task;
  RTIME now;
#endif
  unsigned char last_slot, next_slot;
  RTIME time_in, time_diff;
  int ret;

#ifdef RTAI
  task = rt_task_init_schmod(nam2num("TASK0"), 0, 0, 0, SCHED_FIFO, 0xF);
  printf("Started eNB thread (id %p)\n",task);
#ifndef TIMER_ONESHOT_MODE
  now = rt_get_time();
  ret = rt_task_make_periodic(task, now, nano2count(DAQ_PERIOD));

  if (ret!=0)
    printf("Problem with periodic timer\n");

#endif
#endif

#ifdef HARD_RT
  rt_make_hard_real_time();
#endif

  mlockall(MCL_CURRENT | MCL_FUTURE);

  timing_info.time_min = 100000000ULL;
  timing_info.time_max = 0;
  timing_info.time_avg = 0;
  timing_info.n_samples = 0;

  while (!oai_exit) {
    time_in = rt_get_time_ns();
#ifdef TIMER_ONESHOT_MODE
    ret = rt_sleep_ns(DAQ_PERIOD);

    if (ret)
      printf("eNB Frame %d, time %llu: rt_sleep_ns returned %d\n",frame,time_in,ret);

#else
    rt_task_wait_period();
#endif
    time_diff = rt_get_time_ns() - time_in;

    if (time_diff > timing_info.time_max)
      timing_info.time_max = time_diff;

    if (time_diff < timing_info.time_min)
      timing_info.time_min = time_diff;

    timing_info.time_avg = (timing_info.time_avg*timing_info.n_samples + time_diff)/(timing_info.n_samples+1);
    timing_info.n_samples++;

    last_slot = (slot)%LTE_SLOTS_PER_FRAME;

    if (last_slot <0)
      last_slot+=20;

    next_slot = (slot+3)%LTE_SLOTS_PER_FRAME;

    slot++;

    if (slot==20) {
      slot=0;
      frame++;
    }
  }

  printf("eNB_thread: finished, ran %d times.\n",frame);

#ifdef HARD_RT
  rt_make_soft_real_time();
#endif

  // clean task
#ifdef RTAI
  rt_task_delete(task);
#endif
  printf("Task deleted. returning\n");
  return 0;
}


int main(int argc, char **argv)
{

#ifdef RTAI
  RT_TASK *task;
  RTIME period;
#else
  int error_code;
#endif
  int i,j,aa;
  void *status;

  // to make a graceful exit when ctrl-c is pressed
  signal(SIGSEGV, signal_handler);
  signal(SIGINT, signal_handler);

#ifndef RTAI
  check_clock();
#endif

  mlockall(MCL_CURRENT | MCL_FUTURE);

#ifdef RTAI
  // make main thread LXRT soft realtime
  task = rt_task_init_schmod(nam2num("MYTASK"), 9, 0, 0, SCHED_FIFO, 0xF);

  // start realtime timer and scheduler
#ifdef TIMER_ONESHOT_MODE
  rt_set_oneshot_mode();
  start_rt_timer(0);
  printf("started RTAI timer in oneshot mode\n");
#else
  rt_set_periodic_mode();
  period = start_rt_timer(nano2count(500000));
  printf("started RTAI timer with period %llu ns\n",count2nano(period));
#endif

  printf("Init mutex\n");
  //mutex = rt_get_adr(nam2num("MUTEX"));
  mutex = rt_sem_init(nam2num("MUTEX"), 1);

  if (mutex==0) {
    printf("Error init mutex\n");
    exit(-1);
  } else
    printf("mutex=%p\n",mutex);

#endif

  rt_sleep_ns(10*FRAME_PERIOD);

#ifndef RTAI
  pthread_attr_init (&attr_dlsch_threads);
  pthread_attr_setstacksize(&attr_dlsch_threads,OPENAIR_THREAD_STACK_SIZE);
  //attr_dlsch_threads.priority = 1;
  sched_param_dlsch.sched_priority = sched_get_priority_max(SCHED_FIFO); //OPENAIR_THREAD_PRIORITY;
  pthread_attr_setschedparam  (&attr_dlsch_threads, &sched_param_dlsch);
  pthread_attr_setschedpolicy (&attr_dlsch_threads, SCHED_FIFO);
#endif

#ifdef RTAI
  thread0 = rt_thread_create(eNB_thread, NULL, 100000000);
#else
  error_code = pthread_create(&thread0, &attr_dlsch_threads, eNB_thread, NULL);

  if (error_code!= 0) {
    printf("[lte-softmodem.c] Could not allocate eNB_thread, error %d\n",error_code);
    return(error_code);
  } else {
    printf("[lte-softmodem.c] Allocate eNB_thread successful\n");
  }

#endif
  printf("eNB threads created\n");



  // wait for end of program
  printf("TYPE <CTRL-C> TO TERMINATE\n");

  //getchar();
  while (oai_exit==0) {

    printf("eNB Frame %d, hw_slot %d (time %llu): period %llu, sleep time (avg/min/max/samples) %llu / %llu / %llu / %d, ratio %f\n",frame,slot,rt_get_time_ns(),DAQ_PERIOD,timing_info.time_avg,
           timing_info.time_min,timing_info.time_max,timing_info.n_samples,(double)timing_info.time_avg/DAQ_PERIOD);


    rt_sleep_ns(100*FRAME_PERIOD);
  }

  // stop threads
#ifdef RTAI
  rt_thread_join(thread0);
#else
  pthread_join(thread0,&status);
#endif

#ifdef RTAI
  stop_rt_timer();
#endif

  return 0;
}

