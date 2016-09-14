/*! \file rrc_eNB_UE_context.h
 * \brief rrc procedures for UE context
 * \author Lionel GAUTHIER
 * \date 2015
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */
#ifndef __RRC_ENB_UE_CONTEXT_H__
#include "collection/tree.h"
#include "COMMON/platform_types.h"
#include "defs.h"


void
uid_linear_allocator_init(
  uid_allocator_t* const uid_pP
);

uid_t
uid_linear_allocator_new(
  eNB_RRC_INST* rrc_instance_pP
);


void
uid_linear_allocator_free(
  eNB_RRC_INST* rrc_instance_pP,
  uid_t uidP
);




int rrc_eNB_compare_ue_rnti_id(
  struct rrc_eNB_ue_context_s* c1_pP,
  struct rrc_eNB_ue_context_s* c2_pP
);

RB_PROTOTYPE(rrc_ue_tree_s, rrc_eNB_ue_context_s, entries, rrc_eNB_compare_ue_rnti_id);

struct rrc_eNB_ue_context_s*
rrc_eNB_allocate_new_UE_context(
  eNB_RRC_INST* rrc_instance_pP
);

struct rrc_eNB_ue_context_s*
rrc_eNB_get_ue_context(
  eNB_RRC_INST* rrc_instance_pP,
  rnti_t rntiP
);

void rrc_eNB_remove_ue_context(
  const protocol_ctxt_t* const ctxt_pP,
  eNB_RRC_INST*                rrc_instance_pP,
  struct rrc_eNB_ue_context_s* ue_context_pP
);

#endif
