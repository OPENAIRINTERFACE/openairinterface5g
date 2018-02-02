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
