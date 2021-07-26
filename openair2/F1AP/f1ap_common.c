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

static f1ap_cudu_inst_t *f1_du_inst[NUMBER_OF_eNB_MAX]= {0};
static f1ap_cudu_inst_t *f1_cu_inst[NUMBER_OF_eNB_MAX]= {0};

#if defined(EMIT_ASN_DEBUG_EXTERN)
int asn_debug = 0;
int asn1_xer_print = 0;

inline void ASN_DEBUG(const char *fmt, ...) {
  if (asn_debug) {
    int adi = asn_debug_indent;
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[ASN1]");

    while(adi--)
      fprintf(stderr, " ");

    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
  }
}
#endif

uint8_t F1AP_get_next_transaction_identifier(module_id_t enb_mod_idP, module_id_t cu_mod_idP) {
  static uint8_t transaction_identifier[NUMBER_OF_eNB_MAX];
  transaction_identifier[enb_mod_idP+cu_mod_idP] =
    (transaction_identifier[enb_mod_idP+cu_mod_idP] + 1) % F1AP_TRANSACTION_IDENTIFIER_NUMBER;
  //LOG_T(F1AP,"generated xid is %d\n",transaction_identifier[enb_mod_idP+cu_mod_idP]);
  return transaction_identifier[enb_mod_idP+cu_mod_idP];
}

f1ap_cudu_inst_t *getCxt(bool isCU, module_id_t module_idP) {
  static pid_t t=-1;
  pid_t tNew=gettid();
  AssertFatal ( t==-1 || t==tNew, "This is not thread safe\n");
  t=tNew;
  AssertFatal( module_idP < sizeofArray(f1_cu_inst), "");
  return isCU? f1_cu_inst[ module_idP]:  f1_du_inst[ module_idP];
}

void createF1inst(bool isCU, module_id_t module_idP, f1ap_setup_req_t *req) {
  if (isCU) {
    AssertFatal(f1_cu_inst[module_idP] == NULL, "Double call to F1 CU init\n");
    f1_cu_inst[module_idP]=( f1ap_cudu_inst_t *) calloc(1, sizeof( f1ap_cudu_inst_t));
    //memcpy(f1_cu_inst[module_idP]->setupReq, req, sizeof(f1ap_setup_req_t) );
  } else {
    AssertFatal(f1_du_inst[module_idP] == NULL, "Double call to F1 DU init\n");
    f1_du_inst[module_idP]=( f1ap_cudu_inst_t *) calloc(1,  sizeof(f1ap_cudu_inst_t));
    memcpy(&f1_du_inst[module_idP]->setupReq, req, sizeof(f1ap_setup_req_t) );
  }
}

int f1ap_add_ue(bool isCu,
                module_id_t          module_idP,
                int                  CC_idP,
                int                  UE_id,
                rnti_t               rntiP) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (f1_inst->f1ap_ue[i].rnti == rntiP) {
      f1_inst->f1ap_ue[i].f1ap_uid = i;
      f1_inst->f1ap_ue[i].mac_uid = UE_id;
      LOG_I(F1AP, "Updating the index of UE with RNTI %x and du_ue_f1ap_id %d\n", f1_inst->f1ap_ue[i].rnti, f1_inst->f1ap_ue[i].du_ue_f1ap_id);
      return i;
    }
  }

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (f1_inst->f1ap_ue[i].rnti == 0 ) {
      f1_inst->f1ap_ue[i].rnti = rntiP;
      f1_inst->f1ap_ue[i].f1ap_uid = i;
      f1_inst->f1ap_ue[i].mac_uid = UE_id;
      f1_inst->f1ap_ue[i].du_ue_f1ap_id = rntiP;
      f1_inst->f1ap_ue[i].cu_ue_f1ap_id = rntiP;
      f1_inst->num_ues++;
      LOG_I(F1AP, "Adding a new UE with RNTI %x and cu/du ue_f1ap_id %d\n", f1_inst->f1ap_ue[i].rnti, f1_inst->f1ap_ue[i].du_ue_f1ap_id);
      return i;
    }
  }

  return -1;
}


int f1ap_remove_ue(bool isCu, module_id_t module_idP,
                   rnti_t            rntiP) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (f1_inst->f1ap_ue[i].rnti == rntiP) {
      f1_inst->f1ap_ue[i].rnti = 0;
      break;
    }
  }

  f1_inst->num_ues--;
  return 0;
}

int f1ap_get_du_ue_f1ap_id(bool isCu, module_id_t module_idP,
                           rnti_t            rntiP) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (f1_inst->f1ap_ue[i].rnti == rntiP) {
      return f1_inst->f1ap_ue[i].du_ue_f1ap_id;
    }
  }

  return -1;
}

int f1ap_get_cu_ue_f1ap_id(bool isCu, module_id_t module_idP,
                           rnti_t            rntiP) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (f1_inst->f1ap_ue[i].rnti == rntiP) {
      return f1_inst->f1ap_ue[i].cu_ue_f1ap_id;
    }
  }

  return -1;
}

int f1ap_get_rnti_by_du_id(bool isCu, module_id_t module_idP,
                           module_id_t       du_ue_f1ap_id ) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (f1_inst->f1ap_ue[i].du_ue_f1ap_id == du_ue_f1ap_id) {
      return f1_inst->f1ap_ue[i].rnti;
    }
  }

  return -1;
}

int f1ap_get_rnti_by_cu_id(bool isCu, module_id_t module_idP,
                           module_id_t       cu_ue_f1ap_id ) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (f1_inst->f1ap_ue[i].cu_ue_f1ap_id == cu_ue_f1ap_id) {
      return f1_inst->f1ap_ue[i].rnti;
    }
  }

  return -1;
}

int f1ap_get_du_uid(bool isCu, module_id_t module_idP,
                    module_id_t       du_ue_f1ap_id ) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (f1_inst->f1ap_ue[i].du_ue_f1ap_id == du_ue_f1ap_id) {
      return i;
    }
  }

  return -1;
}

int f1ap_get_cu_uid(bool isCu, module_id_t module_idP,
                    module_id_t       cu_ue_f1ap_id ) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (f1_inst->f1ap_ue[i].cu_ue_f1ap_id == cu_ue_f1ap_id) {
      return i;
    }
  }

  return -1;
}

int f1ap_get_uid_by_rnti(bool isCu, module_id_t module_idP,
                         rnti_t            rntiP ) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);

  for (int i = 0; i < MAX_MOBILES_PER_ENB; i++) {
    if (f1_inst->f1ap_ue[i].rnti == rntiP) {
      return i;
    }
  }

  return -1;
}

int f1ap_du_add_cu_ue_id(bool isCu, module_id_t module_idP,
                         module_id_t       du_ue_f1ap_id,
                         module_id_t       cu_ue_f1ap_id) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);
  module_id_t f1ap_uid = f1ap_get_du_uid(isCu, module_idP,du_ue_f1ap_id);

  if (f1ap_uid < 0 || f1ap_uid >= MAX_MOBILES_PER_ENB)
    return -1;

  f1_inst->f1ap_ue[f1ap_uid].cu_ue_f1ap_id = cu_ue_f1ap_id;
  LOG_I(F1AP, "Adding cu_ue_f1ap_id %d for UE with RNTI %x\n", cu_ue_f1ap_id, f1_inst->f1ap_ue[f1ap_uid].rnti);
  return 0;
}

int f1ap_cu_add_du_ue_id(bool isCu, module_id_t module_idP,
                         module_id_t       cu_ue_f1ap_id,
                         module_id_t       du_ue_f1ap_id) {
  f1ap_cudu_inst_t *f1_inst=getCxt(isCu, module_idP);
  module_id_t f1ap_uid = f1ap_get_cu_uid(isCu,module_idP,cu_ue_f1ap_id);

  if (f1ap_uid < 0 || f1ap_uid >= MAX_MOBILES_PER_ENB)
    return -1;

  f1_inst->f1ap_ue[f1ap_uid].du_ue_f1ap_id = du_ue_f1ap_id;
  LOG_I(F1AP, "Adding du_ue_f1ap_id %d for UE with RNTI %x\n", du_ue_f1ap_id, f1_inst->f1ap_ue[f1ap_uid].rnti);
  return 0;
}

int f1ap_assoc_id(bool isCu, module_id_t module_idP) {
  f1ap_setup_req_t *f1_inst=f1ap_req(isCu, module_idP);
  return f1_inst->assoc_id;
}
