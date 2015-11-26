/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

   Contact Information
   OpenAirInterface Admin: openair_admin@eurecom.fr
   OpenAirInterface Tech : openair_tech@eurecom.fr
   OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

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
