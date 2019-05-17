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

#include "LTE_UL-DCCH-Message.h"

/* Up link procedures */

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
  struct rrc_ue_s1ap_ids_s *c1_pP,
  struct rrc_ue_s1ap_ids_s *c2_pP
);

struct rrc_rnti_tree_s;

RB_PROTOTYPE(rrc_rnti_tree_s, rrc_ue_s1ap_ids_s, entries, rrc_eNB_S1AP_compare_ue_ids);

struct rrc_ue_s1ap_ids_s *
rrc_eNB_S1AP_get_ue_ids(
  eNB_RRC_INST *const rrc_instance_pP,
  const uint16_t ue_initial_id,
  const uint32_t eNB_ue_s1ap_id
);

void
rrc_eNB_S1AP_remove_ue_ids(
  eNB_RRC_INST              *const rrc_instance_pP,
  struct rrc_ue_s1ap_ids_s *const ue_ids_pP
);

void
rrc_eNB_generate_dedicatedRRCConnectionReconfiguration(const protocol_ctxt_t *const ctxt_pP,
    rrc_eNB_ue_context_t          *const ue_context_pP,
                                                     const uint8_t                ho_state
                                                     );

int
rrc_eNB_modify_dedicatedRRCConnectionReconfiguration(const protocol_ctxt_t *const ctxt_pP,
    rrc_eNB_ue_context_t          *const ue_context_pP,
                                                     const uint8_t                ho_state
                                                     );

/*! \fn void rrc_eNB_send_S1AP_INITIAL_CONTEXT_SETUP_RESP(uint8_t mod_id, uint8_t ue_index)
 *\brief create a S1AP_INITIAL_CONTEXT_SETUP_RESP for S1AP.
 *\param ctxt_pP       Running context.
 *\param ue_context_pP RRC UE context.
 */
void
rrc_eNB_send_S1AP_INITIAL_CONTEXT_SETUP_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP
);

/*! \fn void rrc_eNB_send_S1AP_UPLINK_NAS(const protocol_ctxt_t   * const ctxt_pP, eNB_RRC_UE_t * const ue_context_pP, UL_DCCH_Message_t * const ul_dcch_msg)
 *\brief create a S1AP_UPLINK_NAS to transfer a NAS message to S1AP.
 *\param ctxt_pP       Running context.
 *\param ue_context_pP UE context.
 *\param ul_dcch_msg The message receive by RRC holding the NAS message.
 */
void
rrc_eNB_send_S1AP_UPLINK_NAS(
  const protocol_ctxt_t    *const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP,
  LTE_UL_DCCH_Message_t *const ul_dcch_msg
);

/*! \fn void rrc_eNB_send_S1AP_UE_CAPABILITIES_IND(const protocol_ctxt_t   * const ctxt_pP, eNB_RRC_UE_t * const ue_context_pP, UL_DCCH_Message_t *ul_dcch_msg)
 *\brief create a S1AP_UE_CAPABILITIES_IND to transfer a NAS message to S1AP.
 *\param ctxt_pP       Running context.
 *\param ue_context_pP UE context.
 *\param ul_dcch_msg The message receive by RRC holding the NAS message.
 */
void rrc_eNB_send_S1AP_UE_CAPABILITIES_IND(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP,
  LTE_UL_DCCH_Message_t *ul_dcch_msg
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
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP,
  LTE_RRCConnectionSetupComplete_r8_IEs_t *rrcConnectionSetupComplete
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
  const rrc_eNB_ue_context_t        *const ue_context_pP,
  const s1ap_Cause_t                       causeP,
  const long                               cause_valueP
);

/*! \fn rrc_eNB_send_S1AP_UE_CONTEXT_RELEASE_CPLT(const module_id_t enb_mod_idP, const struct rrc_eNB_ue_context_s *const ue_context_pP)
 *\brief create a S1AP_UE_CONTEXT_RELEASE_COMPLETE message, the message is sent by the eNB to S1AP task to acknowledge/complete the release of the UE-associated S1-logical connection over the S1 interface. .
 *\param enb_mod_idP Instance ID of eNB.
 *\param eNB_ue_s1ap_id UE's S1AP ID in the eNB.
 */
void rrc_eNB_send_S1AP_UE_CONTEXT_RELEASE_CPLT(
  module_id_t enb_mod_idP,
  uint32_t eNB_ue_s1ap_id
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
int rrc_eNB_send_S1AP_E_RAB_SETUP_RESP(const protocol_ctxt_t *const ctxt_pP, rrc_eNB_ue_context_t  *const ue_context_pP, uint8_t xid );

/*! \fn rrc_eNB_process_S1AP_E_RAB_MODIFY_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance);
 *\brief process a S1AP dedicated E_RAB modify request message received from S1AP.
 *\param msg_p Message received by RRC.
 *\param msg_name Message name.
 *\param instance Message instance.
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_process_S1AP_E_RAB_MODIFY_REQ(MessageDef *msg_p, const char *msg_name, instance_t instance);

/*! \fn rrc_eNB_send_S1AP_E_RAB_MODIFY_RESP(const protocol_ctxt_t* const ctxt_pP, rrc_eNB_ue_context_t*  const ue_context_pP, uint8_t xid )
 *\brief send a S1AP dedicated E_RAB modify response
 *\param ctxt_pP contxt infirmation
 *\param e_contxt_pP ue specific context at the eNB
 *\param xid transaction identifier
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_send_S1AP_E_RAB_MODIFY_RESP(const protocol_ctxt_t *const ctxt_pP, rrc_eNB_ue_context_t  *const ue_context_pP, uint8_t xid );

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

/*! \fn rrc_eNB_process_PAGING_IND(MessageDef *msg_p, const char *msg_name, instance_t instance)
 *\brief process a S1AP_PAGING_IND message received from S1AP.
 *\param msg_p Message received by RRC.
 *\param msg_name Message name.
 *\param instance Message instance.
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_process_PAGING_IND(MessageDef *msg_p, const char *msg_name, instance_t instance);

void
rrc_pdcp_config_security(
  const protocol_ctxt_t *const ctxt_pP,
  rrc_eNB_ue_context_t          *const ue_context_pP,
  const uint8_t send_security_mode_command
);
/*! \fn rrc_eNB_process_S1AP_E_RAB_RELEASE_COMMAND(MessageDef *msg_p, const char *msg_name, instance_t instance);
 *\brief process a S1AP dedicated E_RAB release command message received from S1AP.
 *\param msg_p Message received by RRC.
 *\param msg_name Message name.
 *\param instance Message instance.
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_process_S1AP_E_RAB_RELEASE_COMMAND(MessageDef *msg_p, const char *msg_name, instance_t instance);

/*! \fn rrc_eNB_send_S1AP_E_RAB_RELEASE_RESPONSE(const protocol_ctxt_t* const ctxt_pP, rrc_eNB_ue_context_t*  const ue_context_pP, uint8_t xid )
 *\brief send a S1AP dedicated E_RAB release response
 *\param ctxt_pP contxt infirmation
 *\param e_contxt_pP ue specific context at the eNB
 *\param xid transaction identifier
 *\return 0 when successful, -1 if the UE index can not be retrieved.
 */
int rrc_eNB_send_S1AP_E_RAB_RELEASE_RESPONSE(const protocol_ctxt_t *const ctxt_pP, rrc_eNB_ue_context_t  *const ue_context_pP, uint8_t xid );

int rrc_eNB_send_PATH_SWITCH_REQ(const protocol_ctxt_t *const ctxt_pP,
                                 rrc_eNB_ue_context_t          *const ue_context_pP);
int rrc_eNB_process_S1AP_PATH_SWITCH_REQ_ACK (MessageDef *msg_p, const char *msg_name, instance_t instance);

int rrc_eNB_send_X2AP_UE_CONTEXT_RELEASE(const protocol_ctxt_t* const ctxt_pP, rrc_eNB_ue_context_t* const ue_context_pP);

int s1ap_ue_context_release(instance_t instance, const uint32_t eNB_ue_s1ap_id);

#endif /* RRC_ENB_S1AP_H_ */
