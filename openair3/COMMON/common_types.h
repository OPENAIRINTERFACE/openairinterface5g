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

#include "commonDef.h"

#include "security_types.h"

#include "queue.h"

#ifndef COMMON_TYPES_H_
#define COMMON_TYPES_H_

#define MSISDN_LENGTH      (15)
#define IMSI_DIGITS_MAX    (15)
#define APN_MAX_LENGTH     (100)
#define PRIORITY_LEVEL_MAX (15)
#define PRIORITY_LEVEL_MIN (1)
#define BEARERS_PER_UE     (11)
#define MAX_APN_PER_UE     (5)

typedef enum {
  RAT_WLAN           = 0,
  RAT_VIRTUAL        = 1,
  RAT_UTRAN          = 1000,
  RAT_GERAN          = 1001,
  RAT_GAN            = 1002,
  RAT_HSPA_EVOLUTION = 1003,
  RAT_EUTRAN         = 1004,
  RAT_CDMA2000_1X    = 2000,
  RAT_HRPD           = 2001,
  RAT_UMB            = 2002,
  RAT_EHRPD          = 2003,
} rat_type_t;

#define NUMBER_OF_RAT_TYPE 11

typedef enum {
  SS_SERVICE_GRANTED = 0,
  SS_OPERATOR_DETERMINED_BARRING = 1,
  SS_MAX,
} subscriber_status_t;

typedef enum {
  NAM_PACKET_AND_CIRCUIT = 0,
  NAM_RESERVED           = 1,
  NAM_ONLY_PACKET        = 2,
  NAM_MAX,
} network_access_mode_t;

typedef uint64_t bitrate_t;

typedef uint8_t  ebi_t;
typedef char*    APN_t;
typedef uint8_t  APNRestriction_t;
typedef uint8_t  DelayValue_t;
typedef uint8_t  priority_level_t;
typedef uint32_t Teid_t;
typedef uint32_t SequenceNumber_t;
typedef uint32_t access_restriction_t;
typedef uint32_t context_identifier_t;
typedef uint32_t rau_tau_timer_t;
//typedef uint32_t in_addr_t; is network byte order

typedef uint32_t ard_t;
#define ARD_UTRAN_NOT_ALLOWED               (1U)
#define ARD_GERAN_NOT_ALLOWED               (1U << 1)
#define ARD_GAN_NOT_ALLOWED                 (1U << 2)
#define ARD_I_HSDPA_EVO_NOT_ALLOWED         (1U << 3)
#define ARD_E_UTRAN_NOT_ALLOWED             (1U << 4)
#define ARD_HO_TO_NON_3GPP_NOT_ALLOWED      (1U << 5)
#define ARD_MAX                             (1U << 6)

typedef struct {
  plmn_t   plmn;
  unsigned cell_identity:28;
} cgi_t;

typedef union {
  struct {
    uint8_t imei[13];
    uint8_t sotfware_version[2];
  } id;
  uint8_t imeisv[15];
} me_identity_t;

typedef struct {
  bitrate_t br_ul;
  bitrate_t br_dl;
} ambr_t;

typedef enum {
  IPv4 = 0,
  IPv6 = 1,
  IPv4_AND_v6 = 2,
  IPv4_OR_v6 = 3,
  IP_MAX,
} pdn_type_t;

typedef struct {
  pdn_type_t pdn_type;
  struct {
    uint8_t ipv4_address[4];
    uint8_t ipv6_address[16];
  } address;
} ip_address_t;

typedef enum {
  QCI_1 = 1,
  QCI_2 = 2,
  QCI_3 = 3,
  QCI_4 = 4,
  QCI_5 = 5,
  QCI_6 = 6,
  QCI_7 = 7,
  QCI_8 = 8,
  QCI_9 = 9,
  /* Values from 128 to 254 are operator specific.
   * Other are reserved.
   */
  QCI_MAX,
} qci_t;

typedef enum {
  PRE_EMPTION_CAPABILITY_ENABLED  = 0,
  PRE_EMPTION_CAPABILITY_DISABLED = 1,
  PRE_EMPTION_CAPABILITY_MAX,
} pre_emp_capability_t;

typedef enum {
  PRE_EMPTION_VULNERABILITY_ENABLED  = 0,
  PRE_EMPTION_VULNERABILITY_DISABLED = 1,
  PRE_EMPTION_VULNERABILITY_MAX,
} pre_emp_vulnerability_t;

typedef struct {
  priority_level_t priority_level;
  pre_emp_vulnerability_t pre_emp_vulnerability;
  pre_emp_capability_t    pre_emp_capability;
} allocation_retention_priority_t;

typedef struct eps_subscribed_qos_profile_s {
  qci_t qci;
  allocation_retention_priority_t allocation_retention_priority;
} eps_subscribed_qos_profile_t;

typedef struct apn_configuration_s {
  context_identifier_t context_identifier;

  /* Each APN configuration can have 0, 1, or 2 ip address:
   * - 0 means subscribed is dynamically allocated by P-GW depending on the
   * pdn_type
   * - 1 Only one type of IP address is returned by HSS
   * - 2 IPv4 and IPv6 address are returned by HSS and are statically
   * allocated
   */
  uint8_t nb_ip_address;
  ip_address_t ip_address[2];

  pdn_type_t  pdn_type;
  char        service_selection[APN_MAX_LENGTH];
  int         service_selection_length;
  eps_subscribed_qos_profile_t subscribed_qos;
  ambr_t ambr;
} apn_configuration_t;

typedef enum {
  ALL_APN_CONFIGURATIONS_INCLUDED = 0,
  MODIFIED_ADDED_APN_CONFIGURATIONS_INCLUDED = 1,
  ALL_APN_MAX,
} all_apn_conf_ind_t;

typedef struct {
  context_identifier_t context_identifier;
  all_apn_conf_ind_t   all_apn_conf_ind;
  /* Number of APNs provided */
  uint8_t nb_apns;
  /* List of APNs configuration 1 to n elements */
  struct apn_configuration_s apn_configuration[MAX_APN_PER_UE];
} apn_config_profile_t;

typedef struct {
  subscriber_status_t   subscriber_status;
  char                  msisdn[MSISDN_LENGTH + 1];
  uint8_t               msisdn_length;
  network_access_mode_t access_mode;
  access_restriction_t  access_restriction;
  ambr_t                subscribed_ambr;
  apn_config_profile_t  apn_config_profile;
  rau_tau_timer_t       rau_tau_timer;
} subscription_data_t;

typedef struct authentication_info_s {
  uint8_t         nb_of_vectors;
  eutran_vector_t eutran_vector;
} authentication_info_t;

typedef enum {
  DIAMETER_AUTHENTICATION_DATA_UNAVAILABLE = 4181,
  DIAMETER_ERROR_USER_UNKNOWN              = 5001,
  DIAMETER_ERROR_ROAMING_NOT_ALLOWED       = 5004,
  DIAMETER_ERROR_UNKNOWN_EPS_SUBSCRIPTION  = 5420,
  DIAMETER_ERROR_RAT_NOT_ALLOWED           = 5421,
  DIAMETER_ERROR_EQUIPMENT_UNKNOWN         = 5422,
  DIAMETER_ERROR_UNKOWN_SERVING_NODE       = 5423,
} s6a_experimental_result_t;

typedef enum {
  DIAMETER_SUCCESS = 2001,
} s6a_base_result_t;

typedef struct {
#define S6A_RESULT_BASE         0x0
#define S6A_RESULT_EXPERIMENTAL 0x1
  unsigned present:1;

  union {
    /* Experimental result as defined in 3GPP TS 29.272 */
    s6a_experimental_result_t experimental;
    /* Diameter basics results as defined in RFC 3588 */
    s6a_base_result_t         base;
  } choice;
} s6a_result_t;

#endif /* COMMON_TYPES_H_ */
