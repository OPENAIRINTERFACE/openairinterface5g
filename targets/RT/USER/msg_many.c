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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>

#include <rtai_lxrt.h>
#include <rtai_sem.h>
#include <rtai_msg.h>

#include "msg_many.h"

static void *msg_test_thread(void *arg)
{
  RT_TASK *self;
  RT_TASK *sender_task;
  thread_info_t *ti = (thread_info_t *)arg;
  run_info_t *ri = ti->ri;
  unsigned int msg;
  int counter = 0;
  char name[6];
  int task_index;

  /* Build custom task name for each thread */
  snprintf(name, 6, "%s%03i", THREAD_NAME_PREFIX, ti->thread_num);

  self = rt_task_init_schmod(nam2num(name), 0, 0, 0, SCHED_FIFO, 0xF);
  mlockall(MCL_CURRENT | MCL_FUTURE);

  rt_make_hard_real_time();

  task_index = tm_add_task(self, ri);

  if(task_index != -TM_WORKER_FULL_ERROR) {
    while (*(ri->exit_condition) == 0) {
      // wait only one PERIOD to avoid wait for ever
      sender_task = rt_receive_timed(ri->sender, &msg, ri->period);

      //sender_task = rt_receive(0, &msg);
      if(sender_task == ri->sender)
        counter++;
    }

    rt_printk("%s: counter == %i\n", name, counter);
    tm_del_task(task_index, ri);
  } else {
    rt_printk("%s: Worker array full!\n", name);
  }

  rt_make_soft_real_time();

  rt_task_delete(self);

  free((void *) ti);

  return 0;
}


int main(void)
{
  RT_TASK *self;
  RT_TASK *tmp_worker_task;
  RT_TASK *worker_tasks[NUM_THREADS];
  int worker_threads[NUM_THREADS];
  RTIME now;
  unsigned int i, ii;
  run_info_t ri;
  thread_info_t *ti;
  uint8_t exit_condition = 0;
  int fail_count = 0;
  int old_index = 0;
  uint8_t lost;

  // make main thread LXRT soft realtime
  self = rt_task_init_schmod(nam2num("MAINTK"), 10, 0, 0, SCHED_FIFO, 0xF);
  mlockall(MCL_CURRENT | MCL_FUTURE);

  rt_make_hard_real_time();
  rt_set_periodic_mode();
  start_rt_timer(0);

  /* Build all information need to run in main and the thread */
  ri.sender = self;
  ri.exit_condition = &exit_condition;
  ri.period = nano2count(PERIOD);
  ri.update_sem = rt_sem_init(nam2num("MUTEX"), 1);

  if(ri.update_sem == 0)
    exit(-1);

  ri.used = 0;
  ri.worker = &worker_tasks;

  now = rt_get_time() + ri.period;
  rt_task_make_periodic(self, now, ri.period);

  /* start all threads */
  for(i = 0; i < NUM_THREADS; i++) {
    ti = (thread_info_t *)malloc(sizeof(thread_info_t));

    if(ti == NULL) {
      rt_printk("MAINTK: can't get memory!\n");
      exit(-1);
    }

    ti->ri = &ri;
    ti->thread_num = i;
    worker_threads[i] = rt_thread_create(msg_test_thread, ti , 10000);
  }

  rt_sleep(NUM_THREADS * ri.period);

  rt_printk("start\n");

  for(i = 0; i < THRESHOLD; i++) {
    rt_task_wait_period();
    lost = 1;

    for(ii = 0; ii < ri.used; ii++) {
      old_index = tm_get_next_task_index(old_index, &ri);

      if(old_index == -TM_WORKER_ERROR) {
        rt_printk("MAINTK: No Tasks!\n");
        break;
      }

      tmp_worker_task = rt_send_if(worker_tasks[old_index], i);

      if(tmp_worker_task == worker_tasks[old_index]) {
        lost = 0;
        break;
      }
    }

    if(lost)
      fail_count++;
  }

  rt_printk("fail_count == %i\n", fail_count);

  // cleanup
  exit_condition = 1;
  rt_make_soft_real_time();

  rt_task_delete(self);

  /* wait for the worker threadss  */
  for(ii = 0; ii < ri.used; ii++) {
    rt_thread_join(worker_threads[ii]);
  }

  stop_rt_timer();
  return 0;
}
