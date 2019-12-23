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

#ifndef CREATE_TASKS_H_
#define CREATE_TASKS_H_

#if defined(ENABLE_ITTI)
/* External declaration of L2L1 task that depend on the target */
extern void *l2l1_task(void *arg);

int create_tasks(uint32_t enb_nb);
int create_tasks_ue(uint32_t ue_nb);
int create_tasks_mbms(uint32_t enb_nb);
#endif

#endif /* CREATE_TASKS_H_ */
