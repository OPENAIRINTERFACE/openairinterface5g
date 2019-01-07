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
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>
#include <sched.h>

#include "thread_ipc.h"

g_thread_ipc_t thread_ipc = {0};

void loop_buffer_reset(buffer_t *loop_buf)
{
  int i;

  for (i = 0; i < BUFFERMAX; i++) {
    loop_buf[i].subframe_num = -1;
  }

  return;
}

static void loop_buffer_init(loop_buffer_op_t *loop_buffer)
{
  loop_buffer->packet_num = 0;
  loop_buffer->isfull = 0;
  loop_buffer->isempty = 1;

  pthread_mutex_init(&loop_buffer->buffer_mutex, NULL);

  pthread_cond_init(&loop_buffer->full_cond, NULL);
  pthread_cond_init(&loop_buffer->empty_cond, NULL);

  loop_buffer_reset(loop_buffer->loop_buf);

  return;
}

static void sync_buffer_init(sync_buffer_t *sync_buffer)
{
  sync_buffer->decoding_subframe_num = 0;
  pthread_mutex_init(&sync_buffer->buffer_mutex, NULL);

  return;
}

int thread_ipc_init(void)
{
  //printf("recv %d\n", thread_ipc.sync_buffer.decoding_subframe_num);
  thread_ipc.ue_sync_state = 0;
  thread_ipc.rx_timestamp = 0;
  thread_ipc.tx_timestamp = 0;
  thread_ipc.current_subframe = 0;

  pthread_mutex_init(&thread_ipc.dl_decode_mutex, NULL);
  pthread_mutex_lock(&thread_ipc.dl_decode_mutex);

  pthread_mutex_init(&thread_ipc.ul_send_mutex, NULL);
  pthread_mutex_lock(&thread_ipc.ul_send_mutex);

  pthread_mutex_init(&thread_ipc.sync_mutex, NULL);
  pthread_mutex_lock(&thread_ipc.sync_mutex);

  loop_buffer_init(&thread_ipc.loop_buffer);
  sync_buffer_init(&thread_ipc.sync_buffer);

  return 0;
}

int thread_ipc_deinit(void)
{
  pthread_mutex_destroy(&thread_ipc.ul_send_mutex);
  pthread_mutex_destroy(&thread_ipc.sync_mutex);
  pthread_mutex_destroy(&thread_ipc.dl_decode_mutex);

  pthread_mutex_destroy(&thread_ipc.loop_buffer.buffer_mutex);
  pthread_cond_destroy(&thread_ipc.loop_buffer.full_cond);
  pthread_cond_destroy(&thread_ipc.loop_buffer.empty_cond);

  pthread_mutex_destroy(&thread_ipc.sync_buffer.buffer_mutex);

  return 0;
}

int set_thread_attr(pthread_attr_t *attr, int policy, int priority, int cpuid)
{
  struct sched_param param;
  cpu_set_t cpu_info;

  pthread_attr_init(attr);

  if (pthread_attr_setschedpolicy(attr, policy) != 0) {
    perror("pthread_attr_setschedpolicy");
    return -1;
  }

  param.sched_priority = priority;

  if (pthread_attr_setschedparam(attr, &param) != 0) {
    perror("pthread_attr_setschedparam");
    return -1;
  }

  CPU_ZERO(&cpu_info);
  CPU_SET(cpuid, &cpu_info);

  if (pthread_attr_setaffinity_np(attr,sizeof(cpu_set_t),&cpu_info)) {
    perror("pthread_attr_setaffinity_np");
    return -1;
  }

  if (pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED) != 0) {
    perror("pthread_attr_setinheritsched");
    return -1;
  }

  return 0;
}


int find_subframe_num(unsigned long long current_subframe_num, buffer_t *buf, int *flag)
{
  long long tmp;
  int i;

  tmp = current_subframe_num;

  for ( i = 0; i < HIGHBUFFER + 1; i++) {
    if(tmp == buf[i].subframe_num) {
      return i;
    } else if (tmp < buf[i].subframe_num) {
      *flag = 1;
    }
  }

  return -1;
}

int ue_unsync_thread_ipc_reset(void)
{
  thread_ipc.ue_sync_state = 0;

  pthread_mutex_lock(&thread_ipc.loop_buffer.buffer_mutex);

  if (thread_ipc.loop_buffer.isempty) {
    pthread_cond_signal(&thread_ipc.loop_buffer.empty_cond);
  }

  if (thread_ipc.loop_buffer.isfull) {
    pthread_cond_signal(&thread_ipc.loop_buffer.full_cond);
  }

  thread_ipc.loop_buffer.packet_num = 0;
  thread_ipc.loop_buffer.isfull = 0;
  thread_ipc.loop_buffer.isempty = 1;

  loop_buffer_reset(thread_ipc.loop_buffer.loop_buf);
  pthread_mutex_unlock(&thread_ipc.loop_buffer.buffer_mutex);

  thread_ipc.current_subframe = 0;

  return 0;
}
void bind_thread2kernel(int cpu_id)
{
  cpu_set_t mask;
  cpu_set_t get;
  int i;
  int num = sysconf(_SC_NPROCESSORS_CONF);
  //printf("system has %d processor(s) by super\n", num);
  CPU_ZERO(&mask);
  CPU_SET(cpu_id, &mask);

  if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
    fprintf(stderr, "set thread affinity failed\n");
  }

  /*CPU_ZERO(&get);
  if (pthread_getaffinity_np(pthread_self(), sizeof(get), &get) < 0) {
          fprintf(stderr, "get thread affinity failed\n");
  }
  for (i = 0; i < num; i++) {
          if (CPU_ISSET(i, &get)) {
                  printf("thread %d is running in processor %d\n", (int)pthread_self(), i);
          }
  }
  if (CPU_ISSET(cpu_id, &get)) {
    printf("thread %d is running in processor %d by super\n", (int)pthread_self(), cpu_id);
    }*/
}
void get_thread2kernel(void)
{
  cpu_set_t get;
  int i;
  int num = sysconf(_SC_NPROCESSORS_CONF);
  printf("system has %d processor(s) by super\n", num);
  CPU_ZERO(&get);

  if (pthread_getaffinity_np(pthread_self(), sizeof(get), &get) < 0) {
    fprintf(stderr, "get thread affinity failed\n");
  }

  for (i = 0; i < num; i++) {
    if (CPU_ISSET(i, &get)) {
      printf("The thread %d is running in processor %d by super\n", (int)pthread_self(), i);
    }
  }
}

