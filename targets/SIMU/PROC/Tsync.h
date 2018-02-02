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

/*
 * Tsync.h
 *
 */
#include <pthread.h>
#ifndef TSYNC_H_
#define TSYNC_H_

#define COMPILER_BARRIER() __asm__ __volatile__ ("" ::: "memory")
int mycount;
pthread_mutex_t exclusive;
pthread_mutex_t downlink_mutex[MAX_eNB][MAX_UE];
pthread_mutex_t downlink_mutex_channel;
pthread_cond_t downlink_cond[MAX_eNB][MAX_UE];
pthread_cond_t downlink_cond_channel;

pthread_mutex_t uplink_mutex[MAX_UE][MAX_eNB];
pthread_mutex_t uplink_mutex_channel;
pthread_cond_t uplink_cond[MAX_UE][MAX_eNB];
pthread_cond_t uplink_cond_channel;

int COT;
int COT_U;

volatile int _COT;
volatile int _COT_U;


int NUM_THREAD_DOWNLINK;
int NUM_THREAD_UPLINK;

ch_thread *e2u_t[MAX_UE][MAX_eNB];
ch_thread *u2e_t[MAX_UE][MAX_eNB];

pthread_t cthr_u[MAX_eNB][MAX_UE];
pthread_t cthr_d[MAX_eNB][MAX_UE];

int fd_NB[MAX_eNB+MAX_UE];
int fd_channel;


#endif /* TSYNC_H_ */
