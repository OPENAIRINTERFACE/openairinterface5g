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

/* those 2 lines for CPU_SET */
#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "low.h"

int dpdk_main(int argc, char **argv, benetel_t *);

void *dpdk_thread(void *_bs)
{
  benetel_t *bs = _bs;
  int n = 0;
  char **v = NULL; // = { "softmodem", "-n", "2", "--file-prefix", "pg2", "-l", "6", "--", "-p", "0x1" };
  char *s = bs->dpdk_main_command_line;
  char *tok;

  while ((tok = strtok(s, " ")) != NULL) {
    n++;
    v = realloc(v, n * sizeof(char *));
    if (v == NULL) {
      printf("%s: out of memory\n", __FUNCTION__);
      exit(1);
    }
    v[n-1] = tok;
    s = NULL;
  }

  dpdk_main(n, v, bs);
  exit(1);
  return 0;
}

void *benetel_start_dpdk(char *ifname, shared_buffers *buffers, char *dpdk_main_command_line)
{
  benetel_t *bs;

  bs = calloc(1, sizeof(benetel_t));
  if (bs == NULL) {
    printf("%s: out of memory\n", __FUNCTION__);
    exit(1);
  }

  bs->buffers = buffers;

  bs->expected_benetel_frame[0] = 255;
  bs->expected_benetel_frame[1] = 255;

  bs->dpdk_main_command_line = dpdk_main_command_line;

  pthread_attr_t attr;

  if (pthread_attr_init(&attr) != 0) {
    printf("pthread_attr_init failed\n");
    exit(1);
  }

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(10,&cpuset);
  if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset) != 0) {
    printf("pthread_attr_setaffinity_np failed\n");
    exit(1);
  }

  if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
    printf("pthread_attr_setschedpolicy failed\n");
    exit(1);
  }

  struct sched_param params;
  params.sched_priority = sched_get_priority_max(SCHED_FIFO);
  if (sched_get_priority_max(SCHED_FIFO) == -1) {
    printf("sched_get_priority_max failed\n");
    exit(1);
  }
  if (pthread_attr_setschedparam(&attr, &params) != 0) {
    printf("pthread_setschedparam failed\n");
    exit(1);
  }

  pthread_t t;
  if (pthread_create(&t, &attr, dpdk_thread, bs) != 0) {
    printf("%s: thread creation failed\n", __FUNCTION__);
    exit(1);
  }

  if (pthread_attr_destroy(&attr) != 0) {
    printf("pthread_attr_init failed\n");
    exit(1);
  }

  return bs;
}
