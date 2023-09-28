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

#include "read_setup_ran.h"
#include "../../E2AP/flexric/src/lib/e2ap/e2ap_node_component_config_add_wrapper.h"
#include "../../E2AP/flexric/test/rnd/fill_rnd_data_e2_setup_req.h"
#include <assert.h>
#include <stdlib.h>

void read_setup_ran(void* data)
{
  assert(data != NULL);
#ifdef E2AP_V1

#elif defined(E2AP_V2) || defined(E2AP_V3) 
  *((e2ap_node_component_config_add_t*)data) = fill_e2ap_node_component_config_add();
#else
  static_assert(0!=0, "Unknown E2AP version");
#endif

}



