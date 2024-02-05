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

#ifndef RAN_FUNC_SM_RAN_CTRL_SUBSCRIPTION_AGENT_H
#define RAN_FUNC_SM_RAN_CTRL_SUBSCRIPTION_AGENT_H

#include "common/utils/hashtable/hashtable.h"
#include "common/utils/collection/tree.h"

typedef enum {
  RRC_STATE_CHANGED_TO_E2SM_RC_RAN_PARAM_ID = 202,   // 8.2.4  RAN Parameters for Report Service Style 4

  END_E2SM_RC_RAN_PARAM_ID
} ran_param_id_e;

typedef struct{
  size_t len;
  ran_param_id_e* ran_param_id;
} arr_ran_param_id_t;

typedef struct ric_req_id_s {
  RB_ENTRY(ric_req_id_s) entries;
  uint32_t ric_req_id;
} rb_ric_req_id_t;

typedef struct {
  RB_HEAD(ric_id_2_param_id_trees, ric_req_id_s) rb[END_E2SM_RC_RAN_PARAM_ID];  //  1 RB tree = (1 RAN Parameter ID) : (n RIC Request ID) => m RB tree = (m RAN Parameter ID) : (n RIC Request ID)
  hash_table_t* htable;    // 1 Hash table = (n RIC Request ID) : (m RAN Parameter ID)
} rc_subs_data_t;


int cmp_ric_req_id(struct ric_req_id_s *c1, struct ric_req_id_s *c2);

void init_rc_subs_data(rc_subs_data_t* rc_subs_data);
void insert_rc_subs_data(rc_subs_data_t* rc_subs_data, uint32_t ric_req_id, arr_ran_param_id_t* arr_ran_param_id);
void remove_rc_subs_data(rc_subs_data_t* rc_subs_data, uint32_t ric_req_id);

#endif
