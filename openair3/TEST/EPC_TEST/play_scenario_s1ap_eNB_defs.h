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

#include <stdint.h>

#include "queue.h"
#include "tree.h"

#include "sctp_eNB_defs.h"

#ifndef PLAY_SCENARIO_S1AP_ENB_DEFS_H_
#define PLAY_SCENARIO_S1AP_ENB_DEFS_H_

#define ENB_TAC (1)
#define ENB_MCC (208)
#define ENB_MNC (92)

#define ENB_NAME        "Eurecom ENB"
#define ENB_NAME_FORMAT (ENB_NAME" %u")

#define S1AP_PORT_NUMBER        (36412)
#define S1AP_SCTP_PPID          (18)

#define X2AP_PORT_NUMBER        (36422)
#define X2AP_SCTP_PPID          (27)

#define S1AP_ENB_NAME_LENGTH_MAX    (150)

typedef enum {
  /* Disconnected state: initial state for any association. */
  S1AP_ENB_STATE_DISCONNECTED = 0x0,
  /* State waiting for S1 Setup response message if eNB is MME accepted or
   * S1 Setup failure if MME rejects the eNB.
   */
  S1AP_ENB_STATE_WAITING     = 0x1,
  /* The eNB is successfully connected to MME, UE contexts can be created. */
  S1AP_ENB_STATE_CONNECTED   = 0x2,
  /* The MME has sent an overload start message. Once the MME disables the
   * OVERLOAD marker, the state of the association will be
   * S1AP_ENB_STATE_CONNECTED.
   */
  S1AP_ENB_OVERLOAD          = 0x3,
  /* Max number of states available */
  S1AP_ENB_STATE_MAX,
} s1ap_eNB_state_t;


/* If the Overload Action IE in the OVERLOAD START message is set to
 * - “reject all RRC connection establishments for non-emergency mobile
 *    originated data transfer “ (i.e. reject traffic corresponding to RRC cause
 *    “mo-data “ (TS 36.331 [16])), or
 * - “reject all RRC connection establishments for signalling “ (i.e. reject
 *    traffic corresponding to RRC cause “modata” and “mo-signalling”
 *    (TS 36.331 [16])),or
 * - “only permit RRC connection establishments for emergency sessions and
 *    mobile terminated services” (i.e. only permit traffic corresponding to RRC
 *    cause “emergency” and “mt-Access” (TS 36.331 [16])).
 *
 * NOTE: When the Overload Action IE is set to “only permit RRC connection
 * establishments for emergency sessions and mobile terminated services”,
 * emergency calls with RRC cause “highPriorityAcess” from high priority users
 * are rejected (TS 24.301 [24]).
 */
typedef enum {
  S1AP_OVERLOAD_REJECT_MO_DATA        = 0x0,
  S1AP_OVERLOAD_REJECT_ALL_SIGNALLING = 0x1,
  S1AP_OVERLOAD_ONLY_EMERGENCY_AND_MT = 0x2,
  S1AP_NO_OVERLOAD                    = 0x3,
  S1AP_OVERLOAD_MAX,
} s1ap_overload_state_t;

/* Served PLMN identity element */
struct plmn_identity_s {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mnc_digit_length;
  STAILQ_ENTRY(plmn_identity_s) next;
};

/* Served group id element */
struct served_group_id_s {
  uint16_t mme_group_id;
  STAILQ_ENTRY(served_group_id_s) next;
};

/* Served mme code for a particular MME */
struct mme_code_s {
  uint8_t mme_code;
  STAILQ_ENTRY(mme_code_s) next;
};

/* Served gummei element */
struct served_gummei_s {
  /* Number of MME served PLMNs */
  uint8_t nb_served_plmns;
  /* List of served PLMNs by MME */
  STAILQ_HEAD(served_plmns_s, plmn_identity_s) served_plmns;

  /* Number of group id in list */
  uint8_t nb_group_id;
  /* Served group id list */
  STAILQ_HEAD(served_group_ids_s, served_group_id_s) served_group_ids;

  /* Number of MME code */
  uint8_t nb_mme_code;
  /* MME Code to uniquely identify an MME within an MME pool area */
  STAILQ_HEAD(mme_codes_s, mme_code_s) mme_codes;

  /* Next GUMMEI element */
  STAILQ_ENTRY(served_gummei_s) next;
};

struct s1ap_eNB_instance_s;

/* This structure describes association of a eNB to a MME */
typedef struct s1ap_eNB_mme_data_s {
  /* MME descriptors tree, ordered by sctp assoc id */
  RB_ENTRY(s1ap_eNB_mme_data_s) entry;

  /* This is the optional name provided by the MME */
  char *mme_name;
  net_ip_address_t    mme_net_ip_address; // useful for joining assoc_id and ip address of packets

  /* List of served GUMMEI per MME. There is one GUMMEI per RAT with a max
   * number of 8 RATs but in our case only one is used. The LTE related pool
   * configuration is included on the first place in the list.
   */
  STAILQ_HEAD(served_gummeis_s, served_gummei_s) served_gummei;

  /* Relative processing capacity of an MME with respect to the other MMEs
   * in the pool in order to load-balance MMEs within a pool as defined
   * in TS 23.401.
   */
  uint8_t relative_mme_capacity;

  /* Current MME overload information (if any). */
  s1ap_overload_state_t overload_state;
  /* Current eNB->MME S1AP association state */
  s1ap_eNB_state_t state;

  /* Next usable stream for UE signalling */
  int32_t nextstream;

  /* Number of input/ouput streams */
  uint16_t in_streams;
  uint16_t out_streams;

  /* Connexion id used between SCTP/S1AP */
  uint16_t cnx_id;

  /* SCTP association id */
  int32_t  assoc_id;

  /* Only meaningfull in virtual mode */
  struct s1ap_eNB_instance_s *s1ap_eNB_instance;
} s1ap_eNB_mme_data_t;

typedef struct s1ap_eNB_instance_s {
  /* Next s1ap eNB association.
   * Only used for virtual mode.
   */
  STAILQ_ENTRY(s1ap_eNB_instance_s) s1ap_eNB_entries;

  /* Number of MME requested by eNB (tree size) */
  uint32_t s1ap_mme_nb;
  /* Number of MME for which association is pending */
  uint32_t s1ap_mme_pending_nb;
  /* Number of MME successfully associated to eNB */
  uint32_t s1ap_mme_associated_nb;
  /* Tree of S1AP MME associations ordered by association ID */
  RB_HEAD(s1ap_mme_map, s1ap_eNB_mme_data_s) s1ap_mme_head;

  /* TODO: add a map ordered by relative MME capacity */

  /* Tree of UE ordered by eNB_ue_s1ap_id's */
  RB_HEAD(s1ap_ue_map, s1ap_eNB_ue_context_s) s1ap_ue_head;

  /* For virtual mode, mod_id as defined in the rest of the L1/L2 stack */
  instance_t instance;

  /* Displayable name of eNB */
  char *eNB_name;
  net_ip_address_t    s1c_net_ip_address;

  /* Unique eNB_id to identify the eNB within EPC.
   * In our case the eNB is a macro eNB so the id will be 20 bits long.
   * For Home eNB id, this field should be 28 bits long.
   */
  uint32_t eNB_id;
  /* The type of the cell */
  enum cell_type_e cell_type;

  /* Tracking area code */
  uint16_t tac;

  /* Mobile Country Code
   * Mobile Network Code
   */
  uint16_t  mcc;
  uint16_t  mnc;
  uint8_t   mnc_digit_length;

} s1ap_eNB_instance_t;

typedef struct {
  /* List of served eNBs
   * Only used for virtual mode
   */
  STAILQ_HEAD(s1ap_eNB_instances_head_s, s1ap_eNB_instance_s) s1ap_eNB_instances_head;
  /* Nb of registered eNBs */
  uint8_t nb_registered_eNBs;

  /* Generate a unique connexion id used between S1AP and SCTP */
  uint16_t global_cnx_id;
} s1ap_eNB_internal_data_t;

int s1ap_eNB_compare_assoc_id(
  struct s1ap_eNB_mme_data_s *p1, struct s1ap_eNB_mme_data_s *p2);

/* Generate the tree management functions */
RB_PROTOTYPE(s1ap_mme_map, s1ap_eNB_mme_data_s, entry,
             s1ap_eNB_compare_assoc_id);

#endif /* PLAY_SCENARIO_S1AP_ENB_DEFS_H_ */
