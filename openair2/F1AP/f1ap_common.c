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

/*! \file f1ap_common.c
 * \brief f1ap procedures for both CU and DU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"

static f1ap_cudu_inst_t *f1_du_inst[NUMBER_OF_gNB_MAX]= {0};
static f1ap_cudu_inst_t *f1_cu_inst[NUMBER_OF_gNB_MAX]= {0};

uint8_t F1AP_get_next_transaction_identifier(instance_t mod_idP, instance_t cu_mod_idP) {
  static uint8_t transaction_identifier[NUMBER_OF_gNB_MAX] = {0};
  transaction_identifier[mod_idP+cu_mod_idP] =
    (transaction_identifier[mod_idP+cu_mod_idP] + 1) % F1AP_TRANSACTION_IDENTIFIER_NUMBER;
  return transaction_identifier[mod_idP+cu_mod_idP];
}

f1ap_cudu_inst_t *getCxt(F1_t isCU, instance_t instanceP) {
  //Fixme: 4G F1 has race condtions, someone may debug it, using this test
  /*
    static pid_t t=-1;
    pid_t tNew=gettid();
    AssertFatal ( t==-1 || t==tNew, "This is not thread safe\n");
    t=tNew;
  */
  AssertFatal( instanceP < sizeofArray(f1_cu_inst), "");
  return isCU == CUtype ? f1_cu_inst[ instanceP]:  f1_du_inst[ instanceP];
}

void createF1inst(F1_t isCU, instance_t instanceP, f1ap_setup_req_t *req) {
  if (isCU == CUtype) {
    AssertFatal(f1_cu_inst[instanceP] == NULL, "Double call to F1 CU init\n");
    f1_cu_inst[instanceP]=( f1ap_cudu_inst_t *) calloc(1, sizeof( f1ap_cudu_inst_t));
    //memcpy(f1_cu_inst[instanceP]->setupReq, req, sizeof(f1ap_setup_req_t) );
  } else {
    AssertFatal(f1_du_inst[instanceP] == NULL, "Double call to F1 DU init\n");
    f1_du_inst[instanceP]=( f1ap_cudu_inst_t *) calloc(1,  sizeof(f1ap_cudu_inst_t));
    memcpy(&f1_du_inst[instanceP]->setupReq, req, sizeof(f1ap_setup_req_t) );
  }
}


int f1ap_assoc_id(F1_t isCu, instance_t instanceP) {
  f1ap_setup_req_t *f1_inst=f1ap_req(isCu, instanceP);
  return f1_inst->assoc_id;
}
