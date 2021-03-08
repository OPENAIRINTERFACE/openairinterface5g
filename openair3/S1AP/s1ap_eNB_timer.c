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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <signal.h>
#include <time.h>
#include <errno.h>

#include "assertions.h"
#include "intertask_interface.h"
#include "s1ap_eNB_timer.h"
#include "common/utils/LOG/log.h"
#include "queue.h"
#include "s1ap_common.h"


int s1ap_timer_setup(
  uint32_t      interval_sec,
  uint32_t      interval_us,
  task_id_t     task_id,
  int32_t       instance,
  uint32_t      timer_kind,
  timer_type_t  type,
  void         *timer_arg,
  long         *timer_id)
{
  uint32_t *timeoutArg=NULL;
  int ret=0;
  timeoutArg=malloc(sizeof(uint32_t));
  *timeoutArg=timer_kind;
  ret=timer_setup(interval_sec,
                interval_us,
                task_id,
                instance,
                type,
                (void*)timeoutArg,
                timer_id);
  return ret;
}

int s1ap_timer_remove(long timer_id)
{
  int ret;
  ret=timer_remove(timer_id);
  return ret;
}

