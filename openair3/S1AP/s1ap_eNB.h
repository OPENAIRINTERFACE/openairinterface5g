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

typedef struct s1ap_eNB_config_s {
  // MME related params
  unsigned char mme_enabled;          ///< MME enabled ?
} s1ap_eNB_config_t;

extern s1ap_eNB_config_t s1ap_config;

#define EPC_MODE_ENABLED       s1ap_config.mme_enabled

void *s1ap_eNB_process_itti_msg(void*);
void  s1ap_eNB_init(void);
void *s1ap_eNB_task(void *arg);

uint32_t s1ap_generate_eNB_id(void);

#endif /* S1AP_ENB_H_ */

/**
 * @}
 */
