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

#ifndef _NR_PDCP_TIMER_THREAD_H_
#define _NR_PDCP_TIMER_THREAD_H_

#include "nr_pdcp_ue_manager.h"

#include <stdint.h>

void nr_pdcp_init_timer_thread(nr_pdcp_ue_manager_t *nr_pdcp_ue_manager);
void nr_pdcp_wakeup_timer_thread(uint64_t time);

#endif /* _NR_PDCP_TIMER_THREAD_H_ */
