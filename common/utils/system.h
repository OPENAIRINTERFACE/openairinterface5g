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

#ifndef _SYSTEM_H_OAI_
#define _SYSTEM_H_OAI_
#include <stdint.h>
#include <pthread.h>
#ifdef __cplusplus
extern "C" {
#endif


/****************************************************
 * send a command to the background process
 *     return -1 on error, 0 on success
 ****************************************************/

int background_system(char *command);

/****************************************************
 * initialize the background process
 *     to be called very early
 ****************************************************/

void start_background_system(void);

void set_latency_target(void);
void configure_linux(void);

void threadCreate(pthread_t* t, void * (*func)(void*), void * param, char* name, int affinity, int priority);
 
#define SCHED_OAI SCHED_RR
#define OAI_PRIORITY_RT_LOW sched_get_priority_min(SCHED_OAI)
#define OAI_PRIORITY_RT ((sched_get_priority_min(SCHED_OAI)+sched_get_priority_max(SCHED_OAI))/2)
#define OAI_PRIORITY_RT_MAX sched_get_priority_max(SCHED_OAI)-2

void thread_top_init(char *thread_name,
                     int affinity,
                     uint64_t runtime,
                     uint64_t deadline,
                     uint64_t period);

/****************************************************
 * Functions to check system at runtime.
 ****************************************************/

int checkIfFedoraDistribution(void);
int checkIfGenericKernelOnFedora(void);
int checkIfInsideContainer(void);
  int rt_sleep_ns (uint64_t x);
#ifdef __cplusplus
}
#endif


#endif /* _SYSTEM_H_OAI_ */
