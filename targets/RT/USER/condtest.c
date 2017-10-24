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

#define PERIOD 100000000
#define THRESHOLD 1

static SEM *mutex;
static CND *cond;

static int thread0;
static int thread1;

static int instance_cnt=-1; //0 means worker is busy, -1 means its free

static void *fun0(void *arg)
{
  RT_TASK *task;
  int done1=0, cnt1=0;
  RTIME right_now;
  int ret;

  task = rt_task_init_schmod(nam2num("TASK0"), 0, 0, 0, SCHED_FIFO, 0xF);
  mlockall(MCL_CURRENT | MCL_FUTURE);

  rt_make_hard_real_time();

  while (cnt1 < THRESHOLD) {
    rt_sem_wait(mutex);
    rt_printk("fun0: Hello World %d, instance_cnt %d!\n",cnt1,instance_cnt);

    while (instance_cnt<0) {
      ret = rt_cond_wait(cond, mutex);

      if (ret != 0) {
        rt_printk("error in rt_cond_wait, code %d\n",ret);

        switch (ret) {
        case RTE_PERM:
          rt_printk("error RTE_PERM %d\n",ret);
          break;

        case RTE_UNBLKD:
          rt_printk("error RTE_UNBLKD %d\n",ret);
          break;

        case RTE_OBJREM:
          rt_printk("error RTE_OBJREM %d\n",ret);
          break;

        default:
          rt_printk("unknown error code %d\n",ret);
          break;
        }

        break;
      }
    }

    rt_sem_signal(mutex);

    // do some work here
    cnt1++;

    rt_sem_wait(mutex);
    instance_cnt--;
    rt_sem_signal(mutex);

  }

  // makes task soft real time
  rt_make_soft_real_time();

  printf("Back to soft RT\n");
  // clean task
  rt_task_delete(task);
  printf("Task deleted. returning\n");
  return 0;
}


int main(void)
{
  RT_TASK *task;
  RTIME now;
  int cnt=0;

  // make main thread LXRT soft realtime
  task = rt_task_init_schmod(nam2num("MYTASK"), 9, 0, 0, SCHED_FIFO, 0xF);
  mlockall(MCL_CURRENT | MCL_FUTURE);

  // start realtime timer and scheduler
  //rt_set_oneshot_mode();
  rt_set_periodic_mode();
  start_rt_timer(0);

  now = rt_get_time() + 10*PERIOD;
  rt_task_make_periodic(task, now, PERIOD);

  printf("Init mutex and cond.\n");
  mutex = rt_sem_init(nam2num("MUTEX"), 1);

  if (mutex==0)
    printf("Error init mutex\n");

  cond = rt_cond_init(nam2num("CONDITION"));

  if (cond==0)
    printf("Error init cond\n");

  thread0 = rt_thread_create(fun0, NULL, 10000);
  //thread1 = rt_thread_create(fun1, NULL, 20000);

  //rt_sleep(PERIOD);

  while (cnt < THRESHOLD) {
    rt_task_wait_period();
    rt_printk("main: Hello World %d!\n",cnt);
    rt_sem_wait(mutex); //now the mutex should have value 0

    if (instance_cnt==0) {
      rt_sem_signal(mutex); //now the mutex should have vaule 1
      rt_printk("worker thread busy!\n");
    } else {
      instance_cnt++;
      rt_cond_signal(cond);
      rt_sem_signal(mutex); //now the mutex should have vaule 1
      rt_printk("signaling worker thread to start!\n");
    }

    cnt++;
  }

  // wait for end of program
  printf("TYPE <ENTER> TO TERMINATE\n");
  getchar();

  // cleanup
  stop_rt_timer();
  return 0;
}
