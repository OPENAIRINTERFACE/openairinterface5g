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

/*! \file x2ap_eNB_defs.h
 * \brief x2ap struct definitions for eNB
 * \author Konstantinos Alexandris <Konstantinos.Alexandris@eurecom.fr>, Cedric Roux <Cedric.Roux@eurecom.fr>, Navid Nikaein <Navid.Nikaein@eurecom.fr>
 * \date 2018
 * \version 1.0
 */

#include <stdint.h>

#include "queue.h"
#include "tree.h"

#include "sctp_eNB_defs.h"

#include "x2ap_ids.h"
#include "x2ap_timers.h"

#ifndef X2AP_ENB_DEFS_H_
#define X2AP_ENB_DEFS_H_

#define X2AP_ENB_NAME_LENGTH_MAX    (150)

typedef enum {
  /* Disconnected state: initial state for any association. */
  X2AP_ENB_STATE_DISCONNECTED = 0x0,

  /* State waiting for x2 Setup response message if the target eNB accepts or
   * X2 Setup failure if rejects the eNB.
   */
  X2AP_ENB_STATE_WAITING     = 0x1,

  /* The eNB is successfully connected to another eNB. */
  X2AP_ENB_STATE_CONNECTED   = 0x2,

  /* X2AP is ready, and the eNB is successfully connected to another eNB. */
  X2AP_ENB_STATE_READY             = 0x3,

  X2AP_ENB_STATE_OVERLOAD          = 0x4,

  X2AP_ENB_STATE_RESETTING         = 0x5,

  /* Max number of states available */
  X2AP_ENB_STATE_MAX,
} x2ap_eNB_state_t;

/* Served PLMN identity element */
struct plmn_identity_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_digit_length;
  STAILQ_ENTRY(plmn_identity_s) next;
};

/* Served group id element */
struct served_group_id_s {
  uint16_t enb_group_id;
  STAILQ_ENTRY(served_group_id_s) next;
};

/* Served enn code for a particular eNB */
struct enb_code_s {
  uint8_t enb_code;
  STAILQ_ENTRY(enb_code_s) next;
};

struct x2ap_eNB_instance_s;

/* This structure describes association of a eNB to another eNB */
typedef struct x2ap_eNB_data_s {
  /* eNB descriptors tree, ordered by sctp assoc id */
  RB_ENTRY(x2ap_eNB_data_s) entry;

  /* This is the optional name provided by the MME */
  char *eNB_name;

  /*  target eNB ID */
  uint32_t eNB_id;

  /* Current eNB load information (if any). */
  //x2ap_load_state_t overload_state;

  /* Current eNB->eNB X2AP association state */
  x2ap_eNB_state_t state;

  /* Next usable stream for UE signalling */
  int32_t nextstream;

  /* Number of input/ouput streams */
  uint16_t in_streams;
  uint16_t out_streams;

  /* Connexion id used between SCTP/X2AP */
  uint16_t cnx_id;

  /* SCTP association id */
  int32_t  assoc_id;

  /* Nid cells */
  uint32_t                Nid_cell[MAX_NUM_CCs];
  int                     num_cc;

  /* Only meaningfull in virtual mode */
  struct x2ap_eNB_instance_s *x2ap_eNB_instance;
} x2ap_eNB_data_t;

typedef struct x2ap_eNB_instance_s {
  /* used in simulation to store multiple eNB instances*/
  STAILQ_ENTRY(x2ap_eNB_instance_s) x2ap_eNB_entries;

  /* Number of target eNBs requested by eNB (tree size) */
  uint32_t x2_target_enb_nb;
  /* Number of target eNBs for which association is pending */
  uint32_t x2_target_enb_pending_nb;
  /* Number of target eNB successfully associated to eNB */
  uint32_t x2_target_enb_associated_nb;
  /* Tree of X2AP eNB associations ordered by association ID */
  RB_HEAD(x2ap_enb_map, x2ap_eNB_data_s) x2ap_enb_head;

  /* Tree of UE ordered by eNB_ue_x2ap_id's */
  //  RB_HEAD(x2ap_ue_map, x2ap_eNB_ue_context_s) x2ap_ue_head;

  /* For virtual mode, mod_id as defined in the rest of the L1/L2 stack */
  instance_t instance;

  /* Displayable name of eNB */
  char *eNB_name;

  /* Unique eNB_id to identify the eNB within EPC.
   * In our case the eNB is a macro eNB so the id will be 20 bits long.
   * For Home eNB id, this field should be 28 bits long.
   */
  uint32_t eNB_id;
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

  net_ip_address_t target_enb_x2_ip_address[X2AP_MAX_NB_ENB_IP_ADDRESS];
  uint8_t          nb_x2;
  net_ip_address_t enb_x2_ip_address;
  uint16_t         sctp_in_streams;
  uint16_t         sctp_out_streams;
  uint32_t         enb_port_for_X2C;
  int              multi_sd;

  x2ap_id_manager  id_manager;
  x2ap_timers_t    timers;
} x2ap_eNB_instance_t;

typedef struct {
  /* List of served eNBs
   * Only used for virtual mode
   */
  STAILQ_HEAD(x2ap_eNB_instances_head_s, x2ap_eNB_instance_s) x2ap_eNB_instances_head;
  /* Nb of registered eNBs */
  uint8_t nb_registered_eNBs;

  /* Generate a unique connexion id used between X2AP and SCTP */
  uint16_t global_cnx_id;
} x2ap_eNB_internal_data_t;

int x2ap_eNB_compare_assoc_id(struct x2ap_eNB_data_s *p1, struct x2ap_eNB_data_s *p2);

/* Generate the tree management functions */
struct x2ap_eNB_map;
struct x2ap_eNB_data_s;
RB_PROTOTYPE(x2ap_eNB_map, x2ap_eNB_data_s, entry, x2ap_eNB_compare_assoc_id);


#endif /* X2AP_ENB_DEFS_H_ */
