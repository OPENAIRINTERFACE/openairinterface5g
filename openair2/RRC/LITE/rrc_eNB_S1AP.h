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
/*! \file rrc_eNB_S1AP.h
 * \brief rrc S1AP procedures for eNB
 * \author Laurent Winckel and Sebastien ROUX and Navid Nikaein and Lionel GAUTHIER
 * \date 2013
 * \version 1.0
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr
 */

#ifndef RRC_ENB_S1AP_H_
#define RRC_ENB_S1AP_H_

# if defined(ENABLE_USE_MME)

#include "UL-DCCH-Message.h"

/* Up link procedures */

#   if defined(ENABLE_ITTI)
typedef struct rrc_ue_s1ap_ids_s {
  /* Tree related data */
  RB_ENTRY(rrc_ue_s1ap_ids_s) entries;

  // keys
  uint16_t ue_initial_id;
  uint32_t eNB_ue_s1ap_id;

  // value
  rnti_t   ue_rnti;
} rrc_ue_s1ap_ids_t;

int
rrc_eNB_S1AP_compare_ue_ids(
  struct rrc_ue_s1ap_ids_s* c1_pP,
  struct rrc_ue_s1ap_ids_s* c2_pP
);

struct rrc_rnti_tree_s;

RB_PROTOTYPE(rrc_rnti_tree_s, rrc_ue_s1ap_ids_s, entries, rrc_eNB_S1AP_compare_ue_ids);

struct rrc_ue_s1ap_ids_s*
rrc_eNB_S1AP_get_ue_ids(
  eNB_RRC_INST* const rrc_instance_pP,
  const uint16_t ue_initial_id,
  const uint32_t eNB_ue_s1ap_id
);

void
rrc_eNB_S1AP_remove_ue_ids(
  eNB_RRC_INST*              const rrc_instance_pP,
  struct rrc_ue_s1ap_ids_s* const ue_ids_pP
);
/*! \fn void rrc_eNB_send_S1AP_INITIAL_CONTEXT_SETUP_RESP(uint8_t mod_id, uint8_t ue_index)
 *\brief create a S1AP_INITIAL_CONTEXT_SETUP_RESP for S1AP.
 *\param ctxt_pP       Running context.
 *\param ue_context_pP RRC UE context.
 */
void
rrc_eNB_send_S1AP_INITIAL_CONTEXT_SETUP_RESP(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP
);

/*! \fn void rrc_eNB_send_S1AP_UPLINK_NAS(const protocol_ctxt_t   * const ctxt_pP, eNB_RRC_UE_t * const ue_context_pP, UL_DCCH_Message_t * const ul_dcch_msg)
 *\brief create a S1AP_UPLINK_NAS to transfer a NAS message to S1AP.
 *\param ctxt_pP       Running context.
 *\param ue_context_pP UE context.
 *\param ul_dcch_msg The message receive by RRC holding the NAS message.
 */
void
rrc_eNB_send_S1AP_UPLINK_NAS(
  const protocol_ctxt_t*    const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  UL_DCCH_Message_t* const ul_dcch_msg
);

/*! \fn void rrc_eNB_send_S1AP_UE_CAPABILITIES_IND(const protocol_ctxt_t   * const ctxt_pP, eNB_RRC_UE_t * const ue_context_pP, UL_DCCH_Message_t *ul_dcch_msg)
 *\brief create a S1AP_UE_CAPABILITIES_IND to transfer a NAS message to S1AP.
 *\param ctxt_pP       Running context.
 *\param ue_context_pP UE context.
 *\param ul_dcch_msg The message receive by RRC holding the NAS message.
 */
void rrc_eNB_send_S1AP_UE_CAPABILITIES_IND(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  UL_DCCH_Message_t* ul_dcch_msg
);

/*! \fn rrc_eNB_send_S1AP_NAS_FIRST_REQ(const protocol_ctxt_t* const ctxt_pP,eNB_RRC_UE_t *const ue_context_pP, RRCConnectionSetupComplete_r8_IEs_t *rrcConnectionSetupComplete)
 *\brief create a S1AP_NAS_FIRST_REQ to indicate that RRC has completed its first connection setup to S1AP.
 *\brief eventually forward a NAS message to S1AP.
 *\param ctxt_pP       Running context.
 *\param ue_context_pP RRC UE context.
 *\param rrcConnectionSetupComplete The message receive by RRC that may hold the NAS message.
 */
void
rrc_eNB_send_S1AP_NAS_FIRST_REQ(
  const protocol_ctxt_t* const ctxt_pP,
  rrc_eNB_ue_context_t*          const ue_context_pP,
  RRCConnectionSetupComplete_r8_IEs_t* rrcConnectionSetupComplete
);


/*! \fn rrc_eNB_send_S1AP_UE_CONTEXT_RELEASE_REQ(const module_id_t enb_mod_idP, const struct rrc_eNB_ue_context_s *const ue_context_pP, const s1ap_Cause_t causeP, const long cause_valueP)
 *\brief create a S1AP_UE_CONTEXT_RELEASE_REQ message, the message is sent by the eNB to S1AP task to request the release of
the UE-associated S1-logical connection over the S1 interface. .
 *\param enb_mod_idP Instance ID of eNB.
 *\param ue_context_pP UE context in the eNB.
 *\param causeP   Origin of the cause for the UE removal.
 *\param cause_valueP Contextual value (in regard of the origin) of the cause.
 */
void rrc_eNB_send_S1AP_UE_CONTEXT_RELEASE_REQ (
  const module_id_t                        enb_mod_idP,
  const rrc_eNB_ue_context_t*        const ue_context_pP,
  const s1ap_Cause_t                       causeP,
  const long                               cause_valueP
);

/* Down link procedures */

/*! \fn rrc_eNB_process_S1AP_DOWNLINK_NAS(MessageDef *msg_p, const char *msg_name, instance_t instance, mui_t *rrc_eNB_mui)
 *\brief process a S1AP_DOWNLINK_NAS message received from S1AP and transfer the embedded NAS message to UE.
 *\param msg_p Message received by RRC.
 *\param msg_name Message name.
 *\param instance Message instance.
 *\param rrc_eNB_mui Counter for lower level message identification.
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_process_S1AP_DOWNLINK_NAS(MessageDef *msg_p, const char *msg_name, instance_t instance, mui_t *rrc_eNB_mui);

/*! \fn rrc_eNB_process_S1AP_INITIAL_CONTEXT_SETUP_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance)
 *\brief process a S1AP_INITIAL_CONTEXT_SETUP_REQ message received from S1AP.
 *\param msg_p Message received by RRC.
 *\param msg_name Message name.
 *\param instance Message instance.
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_process_S1AP_INITIAL_CONTEXT_SETUP_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance);


/*! \fn rrc_eNB_process_S1AP_E_RAB_SETUP_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance);
 *\brief process a S1AP dedicated E_RAB setup request message received from S1AP.
 *\param msg_p Message received by RRC.
 *\param msg_name Message name.
 *\param instance Message instance.
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_process_S1AP_E_RAB_SETUP_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance);

/*! \fn rrc_eNB_send_S1AP_E_RAB_SETUP_RESP(const protocol_ctxt_t* const ctxt_pP, rrc_eNB_ue_context_t*  const ue_context_pP, uint8_t xid )
 *\brief send a S1AP dedicated E_RAB setup response
 *\param ctxt_pP contxt infirmation
 *\param e_contxt_pP ue specific context at the eNB
 *\param xid transaction identifier 
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_send_S1AP_E_RAB_SETUP_RESP(const protocol_ctxt_t* const ctxt_pP, rrc_eNB_ue_context_t*  const ue_context_pP, uint8_t xid );  

/*! \fn rrc_eNB_process_S1AP_UE_CTXT_MODIFICATION_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance)
 *\brief process a S1AP_UE_CTXT_MODIFICATION_REQ message received from S1AP.
 *\param msg_p Message received by RRC.
 *\param msg_name Message name.
 *\param instance Message instance.
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_process_S1AP_UE_CTXT_MODIFICATION_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance);

/*! \fn rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance)
 *\brief process a S1AP_UE_CONTEXT_RELEASE_REQ message received from S1AP.
 *\param msg_p Message received by RRC.
 *\param msg_name Message name.
 *\param instance Message instance.
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_REQ (MessageDef *msg_p, const char *msg_name, instance_t instance);

/*! \fn rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_COMMAND(MessageDef *msg_p, const char *msg_name, instance_t instance)
 *\brief process a rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_COMMAND message received from S1AP.
 *\param msg_p Message received by RRC.
 *\param msg_name Message name.
 *\param instance Message instance.
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_process_S1AP_UE_CONTEXT_RELEASE_COMMAND (MessageDef *msg_p, const char *msg_name, instance_t instance);

#   endif
# endif /* defined(ENABLE_USE_MME) */
#endif /* RRC_ENB_S1AP_H_ */
