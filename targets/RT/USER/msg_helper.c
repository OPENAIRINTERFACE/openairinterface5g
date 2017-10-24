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

#include <rtai_sem.h>

#include "msg_many.h"

/*
 * Add a task task to ri->worker.
 * Return the used index in ri->worker on success
 * Return -TM_WORKER_FULL_ERROR if ri->worker full
 * Return -TM_WORKER_ERROR else
*/
int tm_add_task(RT_TASK *task, run_info_t *ri)
{
  if(ri->used >= NUM_THREADS)
    return -TM_WORKER_FULL_ERROR;

  rt_sem_wait(ri->update_sem);
  (*ri->worker)[ri->used] = task;
  ri->used++;
  rt_sem_signal(ri->update_sem);

  return ri->used - 1;
}

/*
 * Remove the a hole in worker at index index by
 * shift the next slots by one.
 * This function assume that update_sem are locked.
 * Return the new used counter.
*/
int _tm_clean_worker(RT_TASK *(*worker)[], int index, int used)
{
  int i;

  for(i = index; i < used; i++)
    (*worker)[i] = (*worker)[i + 1];

  return used - 1;
}

/*
 * Remove a task from ri->worker a index task_index
 * Return 0 on success
 * Return -TM_WORKER_ERROR else
*/
int tm_del_task(int task_index, run_info_t *ri)
{
  rt_sem_wait(ri->update_sem);
  ri->used = _tm_clean_worker(ri->worker, task_index, ri->used);
  rt_sem_signal(ri->update_sem);

  return 0;
}

/*
 * Return next index to use on ri->worker.
 * It depends on the old_index and ri->used.
 * Return -TM_WORKER_ERROR if ri->used == 0.
*/
inline int tm_get_next_task_index(int old_index, run_info_t *ri)
{
  if(ri->used == 0)
    return -TM_WORKER_ERROR;

  return (old_index + 1) % ri->used;
}

