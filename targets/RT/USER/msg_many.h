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

