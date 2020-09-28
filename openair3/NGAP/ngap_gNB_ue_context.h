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
 
/*! \file ngap_gNB_ue_context.h
 * \brief ngap UE context management within gNB
 * \author Yoshio INOUE, Masayuki HARADA
 * \date 2020
 * \version 0.1
 * \email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 */
 
#include "tree.h"
#include "queue.h"

#include "ngap_gNB_defs.h"

#ifndef NGAP_GNB_UE_CONTEXT_H_
#define NGAP_GNB_UE_CONTEXT_H_

// Forward declarations
struct ngap_gNB_amf_data_s;
struct ngap_ue_map;
struct gNB_amf_desc_s;

typedef enum {
  /* UE has not been registered to a AMF or UE association has failed. */
  NGAP_UE_DECONNECTED = 0x0,
  /* UE ngap state is waiting for initial context setup request message. */
  NGAP_UE_WAITING_CSR = 0x1,
  /* UE association is ready and bearers are established. */
  NGAP_UE_CONNECTED   = 0x2,
  NGAP_UE_STATE_MAX,
} ngap_ue_state;

typedef struct ngap_gNB_ue_context_s {
  /* Tree related data */
  RB_ENTRY(ngap_gNB_ue_context_s) entries;

  /* Uniquely identifies the UE between AMF and gNB within the gNB.
   * This id is encoded on 32bits.
   */
  uint32_t gNB_ue_ngap_id;

  /* UE id for initial connection to NGAP */
  uint16_t ue_initial_id;

  /* Uniquely identifies the UE within AMF. Encoded on 32 bits. */
  uint64_t amf_ue_ngap_id:40;

  /* Stream used for this particular UE */
  int32_t tx_stream;
  int32_t rx_stream;

  /* Current UE state. */
  ngap_ue_state ue_state;

  /* Reference to AMF data this UE is attached to */
  struct ngap_gNB_amf_data_s *amf_ref;

  /* Signaled by the UE in RRC Connection Setup Complete and used in NAS Uplink
   * to route NAS messages correctly. 0-based, not 1-based as in TS 36.331
   * 6.2.2 RRC Connection Setup Complete! */
  int selected_plmn_identity;

  /* Reference to gNB data this UE is attached to */
  ngap_gNB_instance_t *gNB_instance;
} ngap_gNB_ue_context_t;

int ngap_gNB_compare_gNB_ue_ngap_id(
  struct ngap_gNB_ue_context_s *p1, struct ngap_gNB_ue_context_s *p2);

/* Generate the tree management functions prototypes */
RB_PROTOTYPE(ngap_ue_map, ngap_gNB_ue_context_s, entries,
             ngap_gNB_compare_gNB_ue_ngap_id);

struct ngap_gNB_ue_context_s *ngap_gNB_allocate_new_UE_context(void);

struct ngap_gNB_ue_context_s *ngap_gNB_get_ue_context(
  ngap_gNB_instance_t *instance_p,
  uint32_t gNB_ue_ngap_id);

void ngap_gNB_free_ue_context(struct ngap_gNB_ue_context_s *ue_context_p);


#endif /* NGAP_GNB_UE_CONTEXT_H_ */
