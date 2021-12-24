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

#include "shared_buffers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_buffers(shared_buffers *s)
{
  int slot;

  memset(s, 0, sizeof(*s));

  for (slot = 0; slot < 20; slot++) {
    if (pthread_mutex_init(&s->m_dl[slot], NULL) != 0 ||
        pthread_cond_init(&s->c_dl[slot], NULL) != 0  ||
        pthread_mutex_init(&s->m_ul[slot], NULL) != 0 ||
        pthread_cond_init(&s->c_ul[slot], NULL) != 0) {
      printf("%s: error initializing mutex/cond\n", __FUNCTION__);
      exit(1);
    }
  }

  /* the gNB's first transmitted DL slot is 6 but the device
   * needs to have slots 1, 2 and 3, 4 and 5 ready. Let's pretend
   * they are ready.
   */
  s->dl_busy[0][1] = 0x3fff;
  s->dl_busy[0][2] = 0x3fff;
  s->dl_busy[0][3] = 0x3fff;
  s->dl_busy[0][4] = 0x3fff;
  s->dl_busy[0][5] = 0x3fff;
  s->dl_busy[1][1] = 0x3fff;
  s->dl_busy[1][2] = 0x3fff;
  s->dl_busy[1][3] = 0x3fff;
  s->dl_busy[1][4] = 0x3fff;
  s->dl_busy[1][5] = 0x3fff;
}

void lock_dl_buffer(shared_buffers *s, int slot)
{
  if (pthread_mutex_lock(&s->m_dl[slot]) != 0) {
    printf("%s: fatal: lock fails\n", __FUNCTION__);
    exit(1);
  }
}

void unlock_dl_buffer(shared_buffers *s, int slot)
{
  if (pthread_mutex_unlock(&s->m_dl[slot]) != 0) {
    printf("%s: fatal: unlock fails\n", __FUNCTION__);
    exit(1);
  }
}

void wait_dl_buffer(shared_buffers *s, int slot)
{
  if (pthread_cond_wait(&s->c_dl[slot], &s->m_dl[slot]) != 0) {
    printf("%s: fatal: cond_wait fails\n", __FUNCTION__);
    exit(1);
  }
}

void signal_dl_buffer(shared_buffers *s, int slot)
{
  if (pthread_cond_broadcast(&s->c_dl[slot]) != 0) {
    printf("%s: fatal: cond_broadcast fails\n", __FUNCTION__);
    exit(1);
  }
}

void lock_ul_buffer(shared_buffers *s, int slot)
{
  if (pthread_mutex_lock(&s->m_ul[slot]) != 0) {
    printf("%s: fatal: lock fails\n", __FUNCTION__);
    printf("%s: fatal: lock fails slot %d\n", __FUNCTION__, slot);
    exit(1);
  }
}

void unlock_ul_buffer(shared_buffers *s, int slot)
{
  if (pthread_mutex_unlock(&s->m_ul[slot]) != 0) {
    printf("%s: fatal: unlock fails\n", __FUNCTION__);
    exit(1);
  }
}

void wait_ul_buffer(shared_buffers *s, int slot)
{
  if (pthread_cond_wait(&s->c_ul[slot], &s->m_ul[slot]) != 0) {
    printf("%s: fatal: cond_wait fails\n", __FUNCTION__);
    exit(1);
  }
}

void signal_ul_buffer(shared_buffers *s, int slot)
{
  if (pthread_cond_broadcast(&s->c_ul[slot]) != 0) {
    printf("%s: fatal: cond_broadcast fails\n", __FUNCTION__);
    exit(1);
  }
}
