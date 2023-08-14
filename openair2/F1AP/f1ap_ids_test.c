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
#include <stddef.h>
#include "common/utils/assertions.h"
#include "f1ap_ids.h"

int main()
{
  du_init_f1_ue_data();
  int rnti = 13;
  f1_ue_data_t data = {.secondary_ue = 1};
  bool ret = du_add_f1_ue_data(rnti, &data);
  DevAssert(ret);
  ret = du_add_f1_ue_data(rnti, &data);
  DevAssert(!ret);
  bool exists = du_exists_f1_ue_data(rnti);
  DevAssert(exists);
  f1_ue_data_t rdata = du_get_f1_ue_data(rnti);
  DevAssert(rdata.secondary_ue == data.secondary_ue);
  return 0;
}
