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

#ifndef TIMER_H_
#define TIMER_H_

#include <signal.h>

#define SIGTIMER SIGRTMIN

typedef enum s1ap_timer_type_s {
  S1AP_TIMER_PERIODIC,
  S1AP_TIMER_ONE_SHOT,
  S1AP_TIMER_TYPE_MAX,
} s1ap_timer_type_t;

typedef struct {
  void    *arg;
  long    timer_id;
  int32_t timer_kind;
} s1ap_timer_has_expired_t;


int s1ap_timer_timeout(sigval_t info);

/** \brief Request a new timer
 *  \param interval_sec timer interval in seconds
 *  \param interval_us  timer interval in micro seconds
 *  \param task_id      task id of the task requesting the timer
 *  \param instance     instance of the task requesting the timer
 *  \param type         timer type
 *  \param timer_id     unique timer identifier
 *  @returns -1 on failure, 0 otherwise
 **/
int s1ap_timer_setup(
  uint32_t      interval_sec,
  uint32_t      interval_us,
  task_id_t     task_id,
  int32_t       instance,
  uint32_t      timer_kind,
  timer_type_t  type,
  void         *timer_arg,
  long         *timer_id);

/** \brief Remove the timer from list
 *  \param timer_id unique timer id
 *  @returns -1 on failure, 0 otherwise
 **/

int s1ap_timer_remove(long timer_id);

/** \brief Initialize timer task and its API
 *  \param mme_config MME common configuration
 *  @returns -1 on failure, 0 otherwise
 **/
int s1ap_timer_init(void);

#define S1AP_MMEIND     0x80000000
#define S1AP_UEIND      0x00000000
#define S1_SETRSP_WAIT  0x00010000
#define S1_SETREQ_WAIT  0x00020000
#define SCTP_REQ_WAIT   0x00030000
#define S1AP_LINEIND    0x0000ffff
#define S1AP_TIMERIND   0x00ff0000

#define S1AP_TIMERID_INIT   0xffffffffffffffff

#endif
