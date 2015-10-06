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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

#include "tree.h"
#include "queue.h"

#include "s1ap_eNB_defs.h"

#ifndef S1AP_ENB_UE_CONTEXT_H_
#define S1AP_ENB_UE_CONTEXT_H_

// Forward declarations
struct s1ap_eNB_mme_data_s;
struct s1ap_ue_map;
struct eNB_mme_desc_s;

typedef enum {
  /* UE has not been registered to a MME or UE association has failed. */
  S1AP_UE_DECONNECTED = 0x0,
  /* UE s1ap state is waiting for initial context setup request message. */
  S1AP_UE_WAITING_CSR = 0x1,
  /* UE association is ready and bearers are established. */
  S1AP_UE_CONNECTED   = 0x2,
  S1AP_UE_STATE_MAX,
} s1ap_ue_state;

typedef struct s1ap_eNB_ue_context_s {
  /* Tree related data */
  RB_ENTRY(s1ap_eNB_ue_context_s) entries;

  /* Uniquely identifies the UE between MME and eNB within the eNB.
   * This id is encoded on 24bits.
   */
  unsigned eNB_ue_s1ap_id:24;

  /* UE id for initial connection to S1AP */
  uint16_t ue_initial_id;

  /* Uniquely identifies the UE within MME. Encoded on 32 bits. */
  uint32_t mme_ue_s1ap_id;

  /* Stream used for this particular UE */
  int32_t tx_stream;
  int32_t rx_stream;

  /* Current UE state. */
  s1ap_ue_state ue_state;

  /* Reference to MME data this UE is attached to */
  struct s1ap_eNB_mme_data_s *mme_ref;

  /* Reference to eNB data this UE is attached to */
  s1ap_eNB_instance_t *eNB_instance;
} s1ap_eNB_ue_context_t;

int s1ap_eNB_compare_eNB_ue_s1ap_id(
  struct s1ap_eNB_ue_context_s *p1, struct s1ap_eNB_ue_context_s *p2);

/* Generate the tree management functions prototypes */
RB_PROTOTYPE(s1ap_ue_map, s1ap_eNB_ue_context_s, entries,
             s1ap_eNB_compare_eNB_ue_s1ap_id);

struct s1ap_eNB_ue_context_s *s1ap_eNB_allocate_new_UE_context(void);

struct s1ap_eNB_ue_context_s *s1ap_eNB_get_ue_context(
  s1ap_eNB_instance_t *instance_p,
  uint32_t eNB_ue_s1ap_id);

void s1ap_eNB_free_ue_context(struct s1ap_eNB_ue_context_s *ue_context_p);


#endif /* S1AP_ENB_UE_CONTEXT_H_ */
