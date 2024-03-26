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
#include <assert.h>
#include <stdlib.h>

#if defined(E2AP_V2) || defined(E2AP_V3)
// NGAP
static e2ap_node_component_config_add_t fill_ngap_e2ap_node_component_config_add(void)
{
  e2ap_node_component_config_add_t dst = {0};

  // Mandatory
  // 9.2.26
  dst.e2_node_comp_interface_type = NG_E2AP_NODE_COMP_INTERFACE_TYPE;
  // Bug!! Optional in the standard, mandatory in ASN.1
  // 9.2.32
  dst.e2_node_comp_id.type = NG_E2AP_NODE_COMP_INTERFACE_TYPE;
 
  const char ng_msg[] = "Dummy message";
  dst.e2_node_comp_id.ng_amf_name = cp_str_to_ba(ng_msg); 
 
  // Mandatory
  // 9.2.27
  const char req[] = "NGAP Request Message sent";
  const char res[] = "NGAP Response Message reveived";

  dst.e2_node_comp_conf.request = cp_str_to_ba(req); 
  dst.e2_node_comp_conf.response = cp_str_to_ba(res); 
  return dst;
}

// F1AP
static e2ap_node_component_config_add_t fill_f1ap_e2ap_node_component_config_add(void)
{
  e2ap_node_component_config_add_t dst = {0};

  // Mandatory
  // 9.2.26
  dst.e2_node_comp_interface_type = F1_E2AP_NODE_COMP_INTERFACE_TYPE;
  // Bug!! Optional in the standard, mandatory in ASN.1
  // 9.2.32
  dst.e2_node_comp_id.type = F1_E2AP_NODE_COMP_INTERFACE_TYPE;

  dst.e2_node_comp_id.f1_gnb_du_id = 1023;

  // Mandatory
  // 9.2.27
  const char req[] = "F1AP Request Message sent";
  const char res[] = "F1AP Response Message reveived";

  dst.e2_node_comp_conf.request = cp_str_to_ba(req);
  dst.e2_node_comp_conf.response = cp_str_to_ba(res);
  return dst;
}

// E1AP
static e2ap_node_component_config_add_t fill_e1ap_e2ap_node_component_config_add(void)
{
  e2ap_node_component_config_add_t dst = {0};

  // Mandatory
  // 9.2.26
  dst.e2_node_comp_interface_type = E1_E2AP_NODE_COMP_INTERFACE_TYPE;
  // Bug!! Optional in the standard, mandatory in ASN.1
  // 9.2.32
  dst.e2_node_comp_id.type = E1_E2AP_NODE_COMP_INTERFACE_TYPE;

  dst.e2_node_comp_id.e1_gnb_cu_up_id = 1025;

  // Mandatory
  // 9.2.27
  const char req[] = "E1AP Request Message sent";
  const char res[] = "E1AP Response Message reveived";

  dst.e2_node_comp_conf.request = cp_str_to_ba(req);
  dst.e2_node_comp_conf.response = cp_str_to_ba(res);
  return dst;
}

// S1AP
static e2ap_node_component_config_add_t fill_s1ap_e2ap_node_component_config_add(void)
{
  e2ap_node_component_config_add_t dst = {0};

  // Mandatory
  // 9.2.26
  dst.e2_node_comp_interface_type = S1_E2AP_NODE_COMP_INTERFACE_TYPE;
  // Bug!! Optional in the standard, mandatory in ASN.1
  // 9.2.32
  dst.e2_node_comp_id.type = S1_E2AP_NODE_COMP_INTERFACE_TYPE;

  const char str[] = "S1 NAME";
  dst.e2_node_comp_id.s1_mme_name = cp_str_to_ba(str);

  // Mandatory
  // 9.2.27
  const char req[] = "S1AP Request Message sent";
  const char res[] = "S1AP Response Message reveived";

  dst.e2_node_comp_conf.request = cp_str_to_ba(req);
  dst.e2_node_comp_conf.response = cp_str_to_ba(res);
  return dst;
}
#endif

void read_setup_ran(void* data, const ngran_node_t node_type)
{
  assert(data != NULL);
#ifdef E2AP_V1
  (void)node_type;
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
