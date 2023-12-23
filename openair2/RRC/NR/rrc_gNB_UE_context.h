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

/*! \file rrc_eNB_UE_context.h
 * \brief rrc procedures for UE context
 * \author Lionel GAUTHIER
 * \date 2015
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */
#ifndef __RRC_GNB_UE_CONTEXT_H__
#define __RRC_GNB_UE_CONTEXT_H__

#include "collection/tree.h"
#include "common/platform_types.h"
#include "nr_rrc_defs.h"

int rrc_gNB_compare_ue_rnti_id(rrc_gNB_ue_context_t* c1_pP, rrc_gNB_ue_context_t* c2_pP);

RB_PROTOTYPE(rrc_nr_ue_tree_s, rrc_gNB_ue_context_s, entries, rrc_gNB_compare_ue_rnti_id);

rrc_gNB_ue_context_t* rrc_gNB_allocate_new_ue_context(gNB_RRC_INST* rrc_instance_pP);

rrc_gNB_ue_context_t* rrc_gNB_get_ue_context(gNB_RRC_INST* rrc_instance_pP, ue_id_t ue);
rrc_gNB_ue_context_t* rrc_gNB_get_ue_context_by_rnti(gNB_RRC_INST* rrc_instance_pP, sctp_assoc_t assoc_id, rnti_t rntiP);
rrc_gNB_ue_context_t* rrc_gNB_get_ue_context_by_rnti_any_du(gNB_RRC_INST* rrc_instance_pP, rnti_t rntiP);

void rrc_gNB_free_mem_ue_context(rrc_gNB_ue_context_t* const ue_context_pP);

void rrc_gNB_remove_ue_context(gNB_RRC_INST* rrc_instance_pP, rrc_gNB_ue_context_t* ue_context_pP);

rrc_gNB_ue_context_t* rrc_gNB_ue_context_random_exist(gNB_RRC_INST* rrc_instance_pP, const uint64_t ue_identityP);

rrc_gNB_ue_context_t* rrc_gNB_ue_context_5g_s_tmsi_exist(gNB_RRC_INST* rrc_instance_pP, const uint64_t s_TMSI);
rrc_gNB_ue_context_t* rrc_gNB_create_ue_context(sctp_assoc_t assoc_id,
                                                rnti_t rnti,
                                                gNB_RRC_INST* rrc_instance_pP,
                                                const uint64_t ue_identityP,
                                                uint32_t du_ue_id);

#endif
