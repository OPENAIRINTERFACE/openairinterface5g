/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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
