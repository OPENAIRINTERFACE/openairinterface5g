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

/*! \file rrc_eNB_GTPV1U.h
 * \brief rrc GTPV1U procedures for eNB
 * \author Lionel GAUTHIER
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */

#ifndef RRC_ENB_GTPV1U_H_
#define RRC_ENB_GTPV1U_H_


/*! \fn rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP(const protocol_ctxt_t* const ctxt_pP, const gtpv1u_enb_create_tunnel_resp_t * const create_tunnel_resp_pP)
 *\brief Process GTPV1U_ENB_CREATE_TUNNEL_RESP message received from GTPV1U, retrieve the enb teid created.
 *\param ctxt_pP Running context
 *\param create_tunnel_resp_pP Message received by RRC.
 *\return 0 when successful, -1 if the UE index can not be retrieved. */
int
rrc_eNB_process_GTPV1U_CREATE_TUNNEL_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  const gtpv1u_enb_create_tunnel_resp_t *const create_tunnel_resp_pP,
  uint8_t                         *inde_list
);

int
rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  const gtpv1u_enb_create_tunnel_resp_t *const create_tunnel_resp_pP,
  uint8_t                         *inde_list
);

/*! \fn rrc_eNB_send_GTPV1U_ENB_DELETE_TUNNEL_REQ(module_id_t enb_mod_idP, const rrc_eNB_ue_context_t* const ue_context_pP)
 *\brief Send GTPV1U_ENB_DELETE_TUNNEL_REQ message to GTPV1U to destroy all UE-related tunnels.
 *\param module_id Instance ID of eNB.
 *\param ue_context_pP UE context in the eNB.
 */
void rrc_eNB_send_GTPV1U_ENB_DELETE_TUNNEL_REQ(
  module_id_t enb_mod_idP,
  rrc_eNB_ue_context_t* ue_context_pP
);

bool gtpv_data_req(const protocol_ctxt_t*   const ctxt_pP,
                   const rb_id_t                  rb_idP,
                   const mui_t                    muiP,
                   const confirm_t                confirmP,
                   const sdu_size_t               sdu_sizeP,
                   uint8_t*                 const buffer_pP,
                   const pdcp_transmission_mode_t modeP,
                   uint32_t task_id);

#endif /* RRC_ENB_GTPV1U_H_ */
