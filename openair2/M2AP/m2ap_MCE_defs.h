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

/*! \file m2ap_MCE_defs.h
 * \brief m2ap struct definitions for MCE
 * \author Javier Morgade  <javier.morgade@ieee.org>
 * \date 2019
 * \version 0.1
 */

#include <stdint.h>

#include "queue.h"
#include "tree.h"

#include "sctp_eNB_defs.h"

#include "m2ap_ids.h" //looks X2AP specific for HO
#include "m2ap_timers.h"

#ifndef M2AP_MCE_DEFS_H_
#define M2AP_MCE_DEFS_H_

#define M2AP_MCE_NAME_LENGTH_MAX    (150)

typedef enum {
  /* Disconnected state: initial state for any association. */
  M2AP_MCE_STATE_DISCONNECTED = 0x0,

  /* State waiting for m2 Setup response message if the target MCE accepts or
   * M2 Setup failure if rejects the MCE.
   */
  M2AP_MCE_STATE_WAITING     = 0x1,

  /* The MCE is successfully connected to another MCE. */
  M2AP_MCE_STATE_CONNECTED   = 0x2,

  /* M2AP is ready, and the MCE is successfully connected to another MCE. */
  M2AP_MCE_STATE_READY             = 0x3,

  M2AP_MCE_STATE_OVERLOAD          = 0x4,

  M2AP_MCE_STATE_RESETTING         = 0x5,

  /* Max number of states available */
  M2AP_MCE_STATE_MAX,
} m2ap_MCE_state_t;

/* Served PLMN identity element */
/*struct plmn_identity_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_digit_length;
  STAILQ_ENTRY(plmn_identity_s) next;
};*/

/* Served group id element */
/*struct served_group_id_s {
  uint16_t mce_group_id;
  STAILQ_ENTRY(served_group_id_s) next;
};*/

/* Served enn code for a particular MCE */
/*struct mce_code_s {
  uint8_t mce_code;
  STAILQ_ENTRY(mce_code_s) next;
};*/

struct m2ap_MCE_instance_s;

/* This structure describes association of a MCE to another MCE */
typedef struct m2ap_MCE_data_s {
  /* MCE descriptors tree, ordered by sctp assoc id */
  RB_ENTRY(m2ap_MCE_data_s) entry;

  /* This is the optional name provided by the MME */
  char *MCE_name;

  /*  target MCE ID */
  uint32_t MCE_id;

  /* Current MCE load information (if any). */
  //m2ap_load_state_t overload_state;

  /* Current MCE->MCE M2AP association state */
  m2ap_MCE_state_t state;

  /* Next usable stream for UE signalling */
  int32_t nextstream;

  /* Number of input/ouput streams */
  uint16_t in_streams;
  uint16_t out_streams;

  /* Connexion id used between SCTP/M2AP */
  uint16_t cnx_id;

  /* SCTP association id */
  int32_t  assoc_id;

  /* Nid cells */
  uint32_t                Nid_cell[MAX_NUM_CCs];
  int                     num_cc;

  /* Only meaningfull in virtual mode */
  struct m2ap_MCE_instance_s *m2ap_MCE_instance;
} m2ap_MCE_data_t;

typedef struct m2ap_MCE_instance_s {
  /* used in simulation to store multiple MCE instances*/
  STAILQ_ENTRY(m2ap_MCE_instance_s) m2ap_MCE_entries;

  /* Number of target MCEs requested by MCE (tree size) */
  uint32_t m2_target_mce_nb;
  /* Number of target MCEs for which association is pending */
  uint32_t m2_target_mce_pending_nb;
  /* Number of target MCE successfully associated to MCE */
  uint32_t m2_target_mce_associated_nb;
  /* Tree of M2AP MCE associations ordered by association ID */
  RB_HEAD(m2ap_mce_map, m2ap_MCE_data_s) m2ap_mce_head;

  /* Tree of UE ordered by MCE_ue_m2ap_id's */
  //  RB_HEAD(m2ap_ue_map, m2ap_MCE_ue_context_s) m2ap_ue_head;

  /* For virtual mode, mod_id as defined in the rest of the L1/L2 stack */
  instance_t instance;

  /* Displayable name of MCE */
  char *MCE_name;

  /* Unique MCE_id to identify the MCE within EPC.
   * In our case the MCE is a macro MCE so the id will be 20 bits long.
   * For Home MCE id, this field should be 28 bits long.
   */
  uint32_t MCE_id;
  /* The type of the cell */
  cell_type_t cell_type;

  /* Tracking area code */
  uint16_t tac;

  /* Mobile Country Code
   * Mobile Network Code
   */
  uint16_t  mcc;
  uint16_t  mnc;
  uint8_t   mnc_digit_length;

  /* CC params */
  int16_t                 eutra_band[MAX_NUM_CCs];
  uint32_t                downlink_frequency[MAX_NUM_CCs];
  int32_t                 uplink_frequency_offset[MAX_NUM_CCs];
  uint32_t                Nid_cell[MAX_NUM_CCs];
  int16_t                 N_RB_DL[MAX_NUM_CCs];
  lte_frame_type_t        frame_type[MAX_NUM_CCs];
  uint32_t                fdd_earfcn_DL[MAX_NUM_CCs];
  uint32_t                fdd_earfcn_UL[MAX_NUM_CCs];
  int                     num_cc;

  net_ip_address_t target_mce_m2_ip_address[M2AP_MAX_NB_MCE_IP_ADDRESS];
  uint8_t          nb_m2;
  net_ip_address_t mce_m2_ip_address;
  uint16_t         sctp_in_streams;
  uint16_t         sctp_out_streams;
  uint32_t         mce_port_for_M2C;
  int              multi_sd;

  m2ap_id_manager  id_manager;
  m2ap_timers_t    timers;
} m2ap_MCE_instance_t;

typedef struct {
  /* List of served MCEs
   * Only used for virtual mode
   */
  STAILQ_HEAD(m2ap_MCE_instances_head_s, m2ap_MCE_instance_s) m2ap_MCE_instances_head;
  /* Nb of registered MCEs */
  uint8_t nb_registered_MCEs;

  /* Generate a unique connexion id used between M2AP and SCTP */
  uint16_t global_cnx_id;
} m2ap_MCE_internal_data_t;

int m2ap_MCE_compare_assoc_id(struct m2ap_MCE_data_s *p1, struct m2ap_MCE_data_s *p2);

/* Generate the tree management functions */
struct m2ap_MCE_map;
struct m2ap_MCE_data_s;
RB_PROTOTYPE(m2ap_MCE_map, m2ap_MCE_data_s, entry, m2ap_MCE_compare_assoc_id);


#endif /* M2AP_MCE_DEFS_H_ */
