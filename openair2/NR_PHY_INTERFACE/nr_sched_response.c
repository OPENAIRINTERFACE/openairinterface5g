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

/* sched response memory management */

/* A system with reference counting is implemented.
 * It is needed because several threads will access the structure,
 * which has to be released only when all the threads are done with it.
 * So the main thread allocates a sched_response with allocate_sched_response()
 * which allocates a sched_response with a reference counter == 1
 * and then for each other thread that will access sched_response, a
 * call to inc_ref_sched_response() is done. When a thread has finished
 * using the sched_response, it calls deref_sched_response() which will
 * in turn call release_sched_response() when the reference counter becomes 0.
 *
 * The several threads in question are accessing the _same_ sched_response,
 * it has not to be confused with the various TX processes that may run in
 * parallel and which are accessing _different_ sched_response (the maximum
 * number of parallel processes is N_RESP).
 */

#include "nr_sched_response.h"

#include <pthread.h>
#include <stdlib.h>

#include "common/utils/LOG/log.h"
#include "common/utils/assertions.h"

#define N_RESP 3
static NR_Sched_Rsp_t resp[N_RESP];
static int resp_refcount[N_RESP];
static int resp_freelist_next[N_RESP];
static int resp_freelist_head;
static pthread_mutex_t resp_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool resp_freelist_inited = false;

void init_sched_response(void)
{
  /* init only once */
  if (pthread_mutex_lock(&resp_mutex))
    AssertFatal(0, "pthread_mutex_lock failed\n");
  if (resp_freelist_inited) {
    if (pthread_mutex_unlock(&resp_mutex))
      AssertFatal(0, "pthread_mutex_unlock failed\n");
    return;
  }
  resp_freelist_inited = true;
  if (pthread_mutex_unlock(&resp_mutex))
    AssertFatal(0, "pthread_mutex_unlock failed\n");

  int i;
  for (i = 0; i < N_RESP - 1; i++)
    resp_freelist_next[i] = i + 1;
  resp_freelist_next[N_RESP - 1] = -1;
  resp_freelist_head = 0;
}

NR_Sched_Rsp_t *allocate_sched_response(void)
{
  NR_Sched_Rsp_t *ret;
  int new_head;

  if (pthread_mutex_lock(&resp_mutex))
    AssertFatal(0, "pthread_mutex_lock failed\n");

  AssertFatal(resp_freelist_inited, "sched_response used before init\n");

  if (resp_freelist_head == -1) {
    LOG_E(PHY, "fatal: sched_response cannot be allocated, increase N_RESP\n");
    exit(1);
  }

  ret = &resp[resp_freelist_head];
  ret->sched_response_id = resp_freelist_head;
  resp_refcount[resp_freelist_head] = 1;

  new_head = resp_freelist_next[resp_freelist_head];
  resp_freelist_next[resp_freelist_head] = -1;
  resp_freelist_head = new_head;

  if (pthread_mutex_unlock(&resp_mutex))
    AssertFatal(0, "pthread_mutex_unlock failed\n");

  return ret;
}

static void release_sched_response(int sched_response_id)
{
  LOG_D(NR_MAC,"Releasing sched_response %d\n",sched_response_id);	
  resp_freelist_next[sched_response_id] = resp_freelist_head;
  resp_freelist_head = sched_response_id;
}

void deref_sched_response(int sched_response_id)
{
  /* simulators (ulsim/dlsim) deal with their own sched_response but call
   * functions that call this one, let's handle this case with a special
   * value -1 where we do nothing (yes it's a hack)
   */
  if (sched_response_id == -1)
    return;

  if (pthread_mutex_lock(&resp_mutex))
    AssertFatal(0, "pthread_mutex_lock failed\n");

  AssertFatal(resp_freelist_inited, "sched_response used before init\n");
  AssertFatal(resp_refcount[sched_response_id] > 0, "sched_reponse decreased too much\n");

  LOG_D(NR_MAC,"resp_refcount[%d] %d\n",sched_response_id,resp_refcount[sched_response_id]);
  resp_refcount[sched_response_id]--;
  if (resp_refcount[sched_response_id] == 0)
    release_sched_response(sched_response_id);

  if (pthread_mutex_unlock(&resp_mutex))
    AssertFatal(0, "pthread_mutex_unlock failed\n");
}

void inc_ref_sched_response(int sched_response_id)
{
  /* simulators (ulsim/dlsim) deal with their own sched_response but call
   * functions that call this one, let's handle this case with a special
   * value -1 where we do nothing (yes it's a hack)
   */
  if (sched_response_id == -1)
    return;

  LOG_D(NR_MAC,"Incrementing sched_resp resp_refounct[%d] = %d\n",
		  sched_response_id,resp_refcount[sched_response_id]);
  if (pthread_mutex_lock(&resp_mutex))
    AssertFatal(0, "pthread_mutex_lock failed\n");

  AssertFatal(resp_freelist_inited, "sched_response used before init\n");

  resp_refcount[sched_response_id]++;

  if (pthread_mutex_unlock(&resp_mutex))
    AssertFatal(0, "pthread_mutex_unlock failed\n");
}
