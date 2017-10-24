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

#ifndef __MSG_MANY_H__
#define __MSG_MANY_H__
#include <stdint.h>

#define PERIOD 500000 /* in nano seconds */
#define THRESHOLD 1000000
#define THREAD_NAME_PREFIX "TK"
#define NUM_THREADS 10

#define TM_WORKER_FULL_ERROR 1
#define TM_WORKER_ERROR 2

typedef struct run_info {
  RT_TASK *sender;
  uint8_t *exit_condition;
  long long period;
  SEM *update_sem; /* protect the task array */
  int used; /* counter of used slots in worker */
  RT_TASK *(*worker)[]; /* declare worker as pointer to array of pointer to RT_TASK */
} run_info_t;

typedef struct thread_info {
  run_info_t *ri;
  uint8_t thread_num;
} thread_info_t;

int tm_add_task(RT_TASK *task, run_info_t *ri);
int tm_del_task(int task_index, run_info_t *ri);
inline int tm_get_next_task_index(int old_index, run_info_t *ri);

#endif

