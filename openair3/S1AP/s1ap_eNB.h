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

#include <stdio.h>
#include <stdint.h>

/** @defgroup _s1ap_impl_ S1AP Layer Reference Implementation for eNB
 * @ingroup _ref_implementation_
 * @{
 */

#ifndef S1AP_ENB_H_
#define S1AP_ENB_H_

#define S1AP_MMEIND     0x80000000
#define S1AP_UEIND      0x00000000
#define S1_SETRSP_WAIT  0x00010000
#define S1_SETREQ_WAIT  0x00020000
#define SCTP_REQ_WAIT   0x00030000
#define S1AP_LINEIND    0x0000ffff
#define S1AP_TIMERIND   0x00ff0000

#define S1AP_TIMERID_INIT   0xffffffffffffffff

typedef enum s1ap_timer_type_s {
  S1AP_TIMER_PERIODIC,
  S1AP_TIMER_ONE_SHOT,
  S1AP_TIMER_TYPE_MAX,
} s1ap_timer_type_t;

typedef struct s1ap_eNB_config_s {
  // MME related params
  unsigned char mme_enabled;          ///< MME enabled ?
} s1ap_eNB_config_t;

extern s1ap_eNB_config_t s1ap_config;

#define EPC_MODE_ENABLED       s1ap_config.mme_enabled

void *s1ap_eNB_process_itti_msg(void*);
void  s1ap_eNB_init(void);
void *s1ap_eNB_task(void *arg);

int s1ap_timer_remove(long timer_id);
uint32_t s1ap_generate_eNB_id(void);

#endif /* S1AP_ENB_H_ */

/**
 * @}
 */
