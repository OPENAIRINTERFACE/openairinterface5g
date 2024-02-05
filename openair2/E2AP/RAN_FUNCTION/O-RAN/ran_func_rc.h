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

#ifndef RAN_FUNC_SM_RAN_CTRL_READ_WRITE_AGENT_H
#define RAN_FUNC_SM_RAN_CTRL_READ_WRITE_AGENT_H

#include "openair2/E2AP/flexric/src/agent/../sm/sm_io.h"

void read_rc_setup_sm(void* data);

sm_ag_if_ans_t write_subs_rc_sm(void const* src);

sm_ag_if_ans_t write_ctrl_rc_sm(void const* data);

bool read_rc_sm(void *);

#endif
