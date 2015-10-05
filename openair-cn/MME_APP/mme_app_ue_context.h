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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/


/*! \file mme_app_ue_context.h
 *  \brief MME applicative layer
 *  \author Sebastien ROUX
 *  \date 2013
 *  \version 1.0
 *  @defgroup _mme_app_impl_ MME applicative layer
 *  @ingroup _ref_implementation_
 *  @{
 */

#include <stdint.h>
#include <inttypes.h>   /* For sscanf formats */
#include <time.h>       /* to provide time_t */

#include "s1ap_messages_types.h"
#include "nas_messages_types.h"
#include "s6a_messages_types.h"
#include "security_types.h"
#include "tree.h"

#ifndef MME_APP_UE_CONTEXT_H_
#define MME_APP_UE_CONTEXT_H_

typedef enum {
  ECM_IDLE,
  ECM_CONNECTED,
  ECM_DEREGISTERED,
} mm_state_t;

typedef uint64_t mme_app_imsi_t;

#define IMSI_FORMAT SCNu64

/* Convert the IMSI contained by a char string NULL terminated to uint64_t */
#define MME_APP_STRING_TO_IMSI(sTRING, iMSI) sscanf(sTRING, "%"IMSI_FORMAT, iMSI)
#define MME_APP_IMSI_TO_STRING(iMSI, sTRING) snprintf(sTRING, IMSI_DIGITS_MAX+1, "%"IMSI_FORMAT, iMSI)

/** @struct bearer_context_t
 *  @brief Parameters that should be kept for an eps bearer.
 */
typedef struct bearer_context_s {
  /* S-GW Tunnel Endpoint for User-Plane */
  Teid_t       s_gw_teid;

  /* S-GW IP address for User-Plane */
  ip_address_t s_gw_address;

  /* P-GW Tunnel Endpoint for User-Plane */
  Teid_t       p_gw_teid;

  /* P-GW IP address for User-Plane */
  ip_address_t p_gw_address;

  /* QoS for this bearer */
  qci_t                   qci;
  priority_level_t        prio_level;
  pre_emp_vulnerability_t pre_emp_vulnerability;
  pre_emp_capability_t    pre_emp_capability;

  /* TODO: add TFT */
} bearer_context_t;

/** @struct ue_context_t
 *  @brief Useful parameters to know in MME application layer. They are set
 * according to 3GPP TS.23.401 #5.7.2
 */
typedef struct ue_context_s {
  /* Tree entry */
  RB_ENTRY(ue_context_s) rb_entry;

  /* Basic identifier for ue. IMSI is encoded on maximum of 15 digits of 4 bits,
   * so usage of an unsigned integer on 64 bits is necessary.
   */
  mme_app_imsi_t         imsi;                        // set by nas_auth_param_req_t
#define IMSI_UNAUTHENTICATED  (0x0)
#define IMSI_AUTHENTICATED    (0x1)
  /* Indicator to show the IMSI authentication state */
  unsigned               imsi_auth:1;                 // set by nas_auth_resp_t

  unsigned               eNB_ue_s1ap_id:24;
  uint32_t               mme_ue_s1ap_id;

  /* ue_id is equal to mme_ue_s1ap_id */
  uint32_t               ue_id;                       // set by nas_auth_param_req_t

  uint8_t                nb_of_vectors;               // updated by S6A AUTHENTICATION ANSWER
  /* List of authentication vectors for E-UTRAN */
  eutran_vector_t       *vector_list;                 // updated by S6A AUTHENTICATION ANSWER
  // pointer in vector_list
  eutran_vector_t       *vector_in_use;               // updated by S6A AUTHENTICATION ANSWER

#define SUBSCRIPTION_UNKNOWN    0x0
#define SUBSCRIPTION_KNOWN      0x1
  unsigned               subscription_known:1;        // set by S6A UPDATE LOCATION ANSWER
  uint8_t                msisdn[MSISDN_LENGTH+1];     // set by S6A UPDATE LOCATION ANSWER
  uint8_t                msisdn_length;               // set by S6A UPDATE LOCATION ANSWER

  mm_state_t             mm_state;                    // not set/read
  /* Globally Unique Temporary Identity */
  GUTI_t                 guti;                        // guti.gummei.plmn set by nas_auth_param_req_t
  // read by S6A UPDATE LOCATION REQUEST
  me_identity_t          me_identity;                 // not set/read except read by display utility

  /* TODO: Add TAI list */

  /* Last known cell identity */
  cgi_t                  e_utran_cgi;                 // set by nas_attach_req_t
  // read for S11 CREATE_SESSION_REQUEST
  /* Time when the cell identity was acquired */
  time_t                 cell_age;                    // set by nas_auth_param_req_t

  /* TODO: add csg_id */
  /* TODO: add csg_membership */

  network_access_mode_t  access_mode;                  // set by S6A UPDATE LOCATION ANSWER

  /* TODO: add ue radio cap, ms classmarks, supported codecs */

  /* TODO: add ue network capability, ms network capability */
  /* TODO: add selected NAS algorithm */

  /* TODO: add DRX parameter */

  apn_config_profile_t   apn_profile;                  // set by S6A UPDATE LOCATION ANSWER
  ard_t                  access_restriction_data;      // set by S6A UPDATE LOCATION ANSWER
  subscriber_status_t    sub_status;                   // set by S6A UPDATE LOCATION ANSWER
  ambr_t                 subscribed_ambr;              // set by S6A UPDATE LOCATION ANSWER
  ambr_t                 used_ambr;

  rau_tau_timer_t        rau_tau_timer;               // set by S6A UPDATE LOCATION ANSWER

  /* Store the radio capabilities as received in S1AP UE capability indication
   * message.
   */
  char                  *ue_radio_capabilities;       // not set/read
  int                    ue_radio_cap_length;         // not set/read

  Teid_t                 mme_s11_teid;                // set by mme_app_send_s11_create_session_req
  Teid_t                 sgw_s11_teid;                // set by S11 CREATE_SESSION_RESPONSE
  PAA_t                  paa;                         // set by S11 CREATE_SESSION_RESPONSE

  // temp
  char                   pending_pdn_connectivity_req_imsi[16];
  uint8_t                pending_pdn_connectivity_req_imsi_length;
  OctetString            pending_pdn_connectivity_req_apn;
  OctetString            pending_pdn_connectivity_req_pdn_addr;
  int                    pending_pdn_connectivity_req_pti;
  unsigned               pending_pdn_connectivity_req_ue_id;
  network_qos_t          pending_pdn_connectivity_req_qos;
  pco_flat_t             pending_pdn_connectivity_req_pco;
  void                  *pending_pdn_connectivity_req_proc_data;
  int                    pending_pdn_connectivity_req_request_type;

  ebi_t                  default_bearer_id;
  bearer_context_t       eps_bearers[BEARERS_PER_UE];
} ue_context_t;

typedef struct {
  uint32_t nb_ue_managed;
  uint32_t nb_ue_idle;

  uint32_t nb_bearers_managed;

  uint32_t nb_ue_since_last_stat;
  uint32_t nb_bearers_since_last_stat;

  /* Entry to the root */
  RB_HEAD(ue_context_map, ue_context_s) ue_context_tree;
} mme_ue_context_t;

/** \brief Retrieve an UE context by selecting the provided IMSI
 * \param imsi Imsi to find in UE map
 * @returns an UE context matching the IMSI or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_imsi(mme_ue_context_t *mme_ue_context,
    mme_app_imsi_t imsi);

/** \brief Retrieve an UE context by selecting the provided S11 teid
 * \param teid The tunnel endpoint identifier used between MME and S-GW
 * @returns an UE context matching the teid or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_s11_teid(mme_ue_context_t *mme_ue_context,
    uint32_t teid);

/** \brief Retrieve an UE context by selecting the provided mme_ue_s1ap_id
 * \param mme_ue_s1ap_id The UE id identifier used in S1AP MME (and NAS)
 * @returns an UE context matching the mme_ue_s1ap_id or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_mme_ue_s1ap_id(mme_ue_context_t *mme_ue_context,
    uint32_t mme_ue_s1ap_id);

/** \brief Retrieve an UE context by selecting the provided nas_ue_id
 * \param nas_ue_id The UE id identifier used in S1AP MME and NAS
 * @returns an UE context matching the nas_ue_id or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_nas_ue_id(mme_ue_context_t *mme_ue_context,
    uint32_t nas_ue_id);

/** \brief Retrieve an UE context by selecting the provided guti
 * \param guti The GUTI used by the UE
 * @returns an UE context matching the guti or NULL if the context doesn't exists
 **/
ue_context_t *mme_ue_context_exists_guti(mme_ue_context_t *mme_ue_context,
    GUTI_t guti);

/** \brief Insert a new UE context in the tree of known UEs.
 * At least the IMSI should be known to insert the context in the tree.
 * \param ue_context_p The UE context to insert
 * @returns 0 in case of success, -1 otherwise
 **/
int mme_insert_ue_context(mme_ue_context_t *mme_ue_context,
                          struct ue_context_s *ue_context_p);

/** \brief Remove a UE context of the tree of known UEs.
 * \param ue_context_p The UE context to remove
 **/
void mme_remove_ue_context(mme_ue_context_t *mme_ue_context,
		                   struct ue_context_s *ue_context_p);


/** \brief Allocate memory for a new UE context
 * @returns Pointer to the new structure, NULL if allocation failed
 **/
ue_context_t *mme_create_new_ue_context(void);

/** \brief Dump the UE contexts present in the tree
 **/
void mme_app_dump_ue_contexts(mme_ue_context_t *mme_ue_context);


void mme_app_handle_s1ap_ue_context_release_req(const s1ap_ue_context_release_req_t const *s1ap_ue_context_release_req);

#endif /* MME_APP_UE_CONTEXT_H_ */

/* @} */
