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

void read_setup_ran(void* data, const ngran_node_t node_type)
{
  assert(data != NULL);
#ifdef E2AP_V1

#elif defined(E2AP_V2) || defined(E2AP_V3) 

  arr_node_component_config_add_t* dst = (arr_node_component_config_add_t*)data;
  if(node_type == ngran_gNB){
    dst->len_cca = 1;
    dst->cca = calloc(1, sizeof(e2ap_node_component_config_add_t));
    assert(dst->cca != NULL);
    // NGAP
    dst->cca[0] = fill_ngap_e2ap_node_component_config_add();
  } else if(node_type == ngran_gNB_CU){
    dst->len_cca = 2;
    dst->cca = calloc(2, sizeof(e2ap_node_component_config_add_t));
    assert(dst->cca != NULL);
    // NGAP
    dst->cca[0] = fill_ngap_e2ap_node_component_config_add();
    // F1AP
    dst->cca[1] = fill_f1ap_e2ap_node_component_config_add();
  } else if(node_type == ngran_gNB_DU){
    dst->len_cca = 1;
    dst->cca = calloc(1, sizeof(e2ap_node_component_config_add_t));
    assert(dst->cca != NULL);
    // F1AP
    dst->cca[0] = fill_f1ap_e2ap_node_component_config_add();
  } else if(node_type == ngran_gNB_CUCP){
    dst->len_cca = 3;
    dst->cca = calloc(3, sizeof(e2ap_node_component_config_add_t));
    assert(dst->cca != NULL);
    // NGAP
    dst->cca[0] = fill_ngap_e2ap_node_component_config_add();
    // F1AP
    dst->cca[1] = fill_f1ap_e2ap_node_component_config_add();
    // E1AP
    dst->cca[2] = fill_e1ap_e2ap_node_component_config_add();
  } else if(node_type == ngran_gNB_CUUP){
    dst->len_cca = 3;
    dst->cca = calloc(3, sizeof(e2ap_node_component_config_add_t));
    assert(dst->cca != NULL);
    // NGAP
    dst->cca[0] = fill_ngap_e2ap_node_component_config_add();
    // F1AP
    dst->cca[1] = fill_f1ap_e2ap_node_component_config_add();
    // E1AP
    dst->cca[2] = fill_e1ap_e2ap_node_component_config_add();
  } else if(node_type == ngran_eNB){
    dst->len_cca = 1;
    dst->cca = calloc(1, sizeof(e2ap_node_component_config_add_t));
    assert(dst->cca != NULL);
    // S1AP
    dst->cca[0] = fill_s1ap_e2ap_node_component_config_add();
  } else {
    assert(0 != 0 && "Not implemented");
  }

#else
  static_assert(0!=0, "Unknown E2AP version");
#endif

}
